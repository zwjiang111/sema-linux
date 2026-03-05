// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright (c) 2022, ADLINK Technology, Inc
// All rights reserved.
//
// Redistribution and use of this software in source and binary forms,
// with or without modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Neither the name of ADLINK Technology nor the names of its contributors may be used
//   to endorse or promote products derived from this software without specific
//   prior written permission of ADLINK Technology, Inc.
/*
 * SEMA Library APIs for EC & BMC I2C
 *
 * Copyright (C) 2020 ADLINK Technology Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include <dirent.h>
#include <eapi.h>
#include <common.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define EAPI_TRXN 	_IOWR('a', 1, unsigned long)
#define BMC_I2C_STS 	_IOWR('a', 2, unsigned long)
#define PROBE_DEV       _IOWR('a', 3, unsigned long)
#define SMBUS_IOCTL_TRANS _IOWR('a', 4, unsigned long)

#define I2CTIMEOUTSTATUS(x) (x)&0x80
#define I2CADDACKSTATUS(x) (x)&0x04

struct eapi_txn {
	int Bus;
	int Type;
	int Length;
	unsigned char tBuffer[50];
};

struct smbus_data {
	uint8_t addr;
	tTransType Type;
	int Length;
	unsigned char Buffer[32];
};

#define MAX_BLOCK 32

#define SEMA_EXT_IIC_READ               0x01
#define SEMA_EXT_IIC_BLOCK              0x02
#define SEMA_EXT_IIC_WRITE_READ		0x03
#define SEMA_EXT_IIC_EXT_COMMAND        0x10

#define SEMA_SMBUS_READ            0x04
#define SEMA_SMBUS_WRITE           0x05

#define EAPI_I2C_STD_CMD          EAPI_UINT32_C(0x00)
#define EAPI_I2C_NO_CMD           EAPI_UINT32_C(0x01)
#define EAPI_I2C_EXT_CMD          EAPI_UINT32_C(0x02)
#define EAPI_I2C_CMD_TYPE_MASK    EAPI_UINT32_C(0x03) 

#define EAPI_CMD_TYPE(x)          ((EAPI_UINT32_C(x) >> 30) & (EAPI_I2C_CMD_TYPE_MASK))
#define EAPI_I2C_IS_EXT_CMD(x)    (EAPI_UINT32_C(EAPI_CMD_TYPE((x))&(EAPI_I2C_CMD_TYPE_MASK))==EAPI_I2C_EXT_CMD)
#define EAPI_I2C_IS_STD_CMD(x)    (EAPI_UINT32_C(EAPI_CMD_TYPE((x))&(EAPI_I2C_CMD_TYPE_MASK))==EAPI_I2C_STD_CMD)
#define EAPI_I2C_IS_NO_CMD(x)     (EAPI_UINT32_C(EAPI_CMD_TYPE((x))&(EAPI_I2C_CMD_TYPE_MASK))==EAPI_I2C_NO_CMD)

static int get_i2c_dev (char *i2c_dev)
{
        const struct dirent *de;
        DIR *dr = opendir("/sys/class/i2c-adapter");

        if (dr == NULL)  // opendir returns NULL if couldn't open directory
                return -1;

        while ((de = readdir(dr)) != NULL)
        {
                if(strncmp(de->d_name, "i2c", strlen("i2c")) == 0)
                {
                        int fd;
                        char I2C_ADAPTER[512];
                        sprintf(I2C_ADAPTER, "/sys/class/i2c-adapter/%s/name", de->d_name);

                        if((fd = open(I2C_ADAPTER, O_RDONLY)) > 0)
                        {
                                if(read(fd, I2C_ADAPTER, sizeof(I2C_ADAPTER)) > 0)
                                {
                                        if(strncmp(I2C_ADAPTER, "ADLINK BMC I2C adapter", strlen("ADLINK BMC I2C adapter")) == 0)
                                        {
                                                sprintf(i2c_dev, "/dev/%s", de->d_name);
                                                close(fd);
                                                closedir(dr);
                                                return 0;
                                        }
                                }
                                close(fd);
                        }
                }
        }

        closedir(dr);
	return -ENODEV;
}

static inline int i2c_smbus_access(int file, char read_write, unsigned char command,
                int size, union i2c_smbus_data *data)
{
        struct i2c_smbus_ioctl_data args;

        args.read_write = read_write;
        args.command = command;
        args.size = size;
        args.data = data;
        return ioctl(file,I2C_SMBUS,&args);
}

static inline int i2c_smbus_read_i2c_block_data(int file, unsigned char command,
                unsigned char length, unsigned char *values)
{
        union i2c_smbus_data data;       

        if (length > 32)
                length = 32;
        data.block[0] = length;
        if (i2c_smbus_access(file,I2C_SMBUS_READ,command,
                                length == 32 ? I2C_SMBUS_I2C_BLOCK_BROKEN :
                                I2C_SMBUS_I2C_BLOCK_DATA,&data)) {
                return -1;
        }
        else {
			    int i;
                for (i = 1; i <= data.block[0]; i++)
                        values[i-1] = data.block[i];
                return data.block[0];
        }
}

static inline int i2c_smbus_write_i2c_block_data(int file, unsigned char command,
                unsigned char length, const unsigned char *values)
{
        union i2c_smbus_data data;
        int i;
        if (length > 32)
                length = 32;
        for (i = 1; i <= length; i++)
                data.block[i] = values[i-1];
        data.block[0] = length;
        return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
                        I2C_SMBUS_I2C_BLOCK_BROKEN, &data);
}

static int open_i2c_dev (uint8_t address)
{
        int file, ret;
        char i2c_dev[64];

        if((ret = get_i2c_dev(i2c_dev)) < 0)
                return ret;

        if((file = open(i2c_dev, O_RDWR)) < 0)
                return file;

        if((ret = ioctl(file, I2C_SLAVE, address>>1)) < 0)
        {
                close(file);
                return ret;
        }

        return file;
}

uint32_t Write_Xfer(uint32_t Id, uint32_t Addr, uint32_t Cmd, void *pBuffer, uint32_t ByteCnt, bool lock_needed)
{
        uint32_t MaxBlkSize;
        int fd;
        struct eapi_txn trxn;
        unsigned char data_off;

        if (pBuffer == NULL || ByteCnt == 0)
        {
                return EAPI_STATUS_INVALID_PARAMETER;
        }

        if (EApiI2CGetBusCap(Id, &MaxBlkSize) != 0) {
                return EAPI_STATUS_INVALID_PARAMETER;
        }

        if (EAPI_I2C_IS_10BIT_ADDR(Addr)) {
                return EAPI_STATUS_INVALID_PARAMETER;
        }

        if (ByteCnt > MAX_BLOCK) {
                return EAPI_STATUS_INVALID_BLOCK_LENGTH;
        }

        if (Id == EAPI_ID_I2C_EXTERNAL) {
                trxn.Bus = 1;
        }
        else if (Id == EAPI_ID_I2C_LVDS_1) {
                trxn.Bus = 2;
        }
        else if (Id == EAPI_ID_I2C_LVDS_2)
        {
                trxn.Bus = 3;
        }
        else if (Id == SEMA_EAPI_ID_I2C_EXTERNAL_2) {
                trxn.Bus = 4;
        }
        else
                return EAPI_STATUS_INVALID_PARAMETER;

        memset(trxn.tBuffer, 0, sizeof(unsigned char)*50);

        if(lock_needed)
                pthread_mutex_lock(&lib_mutex);

	 if((fd = open("/dev/bmc-i2c-eapi", O_RDWR)) < 0)
        {
                if(lock_needed)
                        pthread_mutex_unlock(&lib_mutex);
                return EAPI_STATUS_READ_ERROR;
        }

        trxn.tBuffer[0] = 0x4;
        trxn.tBuffer[1] = 0x2;
        trxn.tBuffer[2] = ByteCnt + 1;
        trxn.tBuffer[3] = trxn.Bus;
        trxn.tBuffer[4] = (Addr >> 8) & 0x7;
        trxn.tBuffer[5] = Addr;
        if(Cmd & (2 << 30))
        {
            trxn.tBuffer[2] = ByteCnt + 2;
            trxn.Length = ByteCnt + 2;
            trxn.tBuffer[6] = Cmd & 0xFF;
            trxn.tBuffer[7] = (Cmd >> 8) & 0xff;
            data_off = 8;
        }
        else if(Cmd & (1 << 30))
        {
            trxn.tBuffer[2] = ByteCnt;
            trxn.Length = ByteCnt;
            data_off = 6;
        }
        else
        {
            trxn.tBuffer[2] = ByteCnt + 1;
            trxn.tBuffer[6] = Cmd & 0xFF;
            data_off = 7;
            trxn.Length = ByteCnt + 1;
        }

	for (uint32_t i = 0; i < ByteCnt; i++) {
           trxn.tBuffer[i+data_off] = ((unsigned char*)pBuffer)[i];
        }

        trxn.Type = SEMA_EXT_IIC_BLOCK;

        if(ioctl(fd, EAPI_TRXN, &trxn) < 0)
        {
                close(fd);
                if(lock_needed)
                        pthread_mutex_unlock(&lib_mutex);
                return EAPI_STATUS_UNSUPPORTED;
        }

        close(fd);
        if(lock_needed)
                pthread_mutex_unlock(&lib_mutex);
        return EAPI_STATUS_SUCCESS;
}

uint32_t EApiI2CReadTransfer(uint32_t Id, uint32_t Addr, uint32_t Cmd, void* pBuffer, uint32_t BufLen, uint32_t ByteCnt)
{
	uint32_t i,ret;
	uint32_t MaxBlkSize;
	int fd;
	uint8_t write_data[8];

	struct eapi_txn trxn;

 	if (pBuffer == NULL || ByteCnt == 0 || BufLen == 0)
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	if (EApiI2CGetBusCap(Id, &MaxBlkSize) != 0)
       	{
		return EAPI_STATUS_UNSUPPORTED;
	}

	if (EAPI_I2C_IS_10BIT_ADDR(Addr))
       	{
		return EAPI_STATUS_UNSUPPORTED;
	}

	if (ByteCnt > MAX_BLOCK)
       	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	if (ByteCnt > BufLen)
       	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	if (Id == EAPI_ID_I2C_EXTERNAL)
       	{
		trxn.Bus = 1;
	}
	else if (Id == EAPI_ID_I2C_LVDS_1)
       	{
		trxn.Bus = 2;
	}
	else if (Id == EAPI_ID_I2C_LVDS_2)
        {
                trxn.Bus = 3;
        }
	else if (Id == SEMA_EAPI_ID_I2C_EXTERNAL_2) {
                trxn.Bus = 4;
	}
	else
		return EAPI_STATUS_UNSUPPORTED;

	memset(trxn.tBuffer, 0, sizeof(unsigned char)*50);

	pthread_mutex_lock(&lib_mutex);
	if((fd = open("/dev/bmc-i2c-eapi", O_RDWR)) < 0)
	{
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_READ_ERROR;
	}

#if 1	
	//check whether 2 byte command
        if(Cmd & (2 << 30))
	{
	    write_data[0] = Cmd & 0xff;
	    write_data[1] = (Cmd >> 8) & 0xff;
	    ret = Write_Xfer(Id, Addr, (1 << 30), write_data, 2, false);
	}
	//check whether 1 byte command
	else 
	{
	    write_data[0] = Cmd & 0xff;
	    ret = Write_Xfer(Id, Addr, (1 << 30), write_data, 1, false);
	}

	if(ret != EAPI_STATUS_SUCCESS)
	{
		close(fd);
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_ERROR;
	}
		
#endif
#if 1 
	memset(trxn.tBuffer, 0, sizeof(unsigned char) * 50);

	trxn.tBuffer[0] = 0x4; //I/F type
	trxn.tBuffer[1] = 0x1; //I2C read
	trxn.tBuffer[2] = ByteCnt;//BufLen; //read buffer length
	trxn.tBuffer[3] = trxn.Bus; 
	trxn.tBuffer[4] = (Addr >> 8) & 0x7;
	trxn.tBuffer[5] = (uint8_t)Addr;
	trxn.Type = SEMA_EXT_IIC_READ;
	trxn.Length = ByteCnt;//BufLen;

	if(ioctl(fd, EAPI_TRXN, &trxn) < 0)
	{
		close(fd);
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_UNSUPPORTED;
	}

	for (i = 0; i < ByteCnt; i++) {
		((unsigned char*)pBuffer)[i] = trxn.tBuffer[i];
	}
#endif
	close(fd);
	pthread_mutex_unlock(&lib_mutex);
	return EAPI_STATUS_SUCCESS;
}

uint32_t EApiI2CWriteTransfer(uint32_t Id, uint32_t Addr, uint32_t Cmd, void *pBuffer, uint32_t ByteCnt)
{
	return Write_Xfer(Id, Addr, Cmd, pBuffer, ByteCnt, true);
}

uint32_t EApiI2CGetBusCap(uint32_t Id, uint32_t *pMaxBlkLen)
{
	if (pMaxBlkLen == NULL)
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	*pMaxBlkLen = MAX_BLOCK;

	int fd = open("/sys/bus/platform/devices/adl-bmc-boardinfo/information/capabilities", O_RDONLY);
	if(fd < 0)
	{
		return EAPI_STATUS_UNSUPPORTED;
	}

	char buffer[10] = {0};

	uint32_t m_nSemaCaps;
	if(read(fd, buffer, 10) < 0)
	{
		close(fd);
		return EAPI_STATUS_READ_ERROR;
	}

	m_nSemaCaps = atoi(buffer);

	switch (Id)
	{
		case SEMA_EXT_IIC_BUS_1:
			if (m_nSemaCaps & SEMA_C_I2C1)
			{
				close(fd);
				return EAPI_STATUS_SUCCESS;
			}
			break;

		case SEMA_EXT_IIC_BUS_2:
			if (m_nSemaCaps & SEMA_C_I2C2)
			{
				close(fd);
				return EAPI_STATUS_SUCCESS;
			}
			break;
		
		case SEMA_EXT_IIC_BUS_3:
                        if (m_nSemaCaps & SEMA_C_I2C3)
                        {
				close(fd);
                                return EAPI_STATUS_SUCCESS;
                        }
                        break;
		case SEMA_EXT_IIC_BUS_4:
			if(m_nSemaCaps & SEMA_C_I2C4)
			{
				close(fd);
				return EAPI_STATUS_SUCCESS;
			}
			break;

		default:
			*pMaxBlkLen = 0;
			close(fd);
			return EAPI_STATUS_UNSUPPORTED;
	}
	*pMaxBlkLen = 0;
	close(fd);
	return EAPI_STATUS_UNSUPPORTED;
}


uint32_t EApiI2CGetBusSts(uint32_t Id, uint8_t *Bus_Sts)
{
	(void)Id;
	int fd;

	struct eapi_txn trxn;

	if(Bus_Sts == NULL)
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}
	
	pthread_mutex_lock(&lib_mutex);
	if((fd = open("/dev/bmc-i2c-eapi", O_RDWR)) < 0)
	{
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_UNSUPPORTED;
	}

	if(ioctl(fd, BMC_I2C_STS, &trxn) < 0)
	{
		close(fd);
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_UNSUPPORTED;
	}

	*Bus_Sts = trxn.tBuffer[0];
	close(fd);
	pthread_mutex_unlock(&lib_mutex);
	return EAPI_STATUS_SUCCESS;
}

uint32_t EApiI2CProbeDevice(uint32_t Id, uint32_t Addr)
{
	int fd;

	struct eapi_txn trxn;

	if (Id == EAPI_ID_I2C_EXTERNAL) {
		trxn.Bus = 1;
	}
	else if (Id == EAPI_ID_I2C_LVDS_1) {
		trxn.Bus = 2;
	}
	else if (Id == EAPI_ID_I2C_LVDS_2) {
		trxn.Bus = 3;
        }
	else if (Id == SEMA_EAPI_ID_I2C_EXTERNAL_2) {
	       	trxn.Bus = 4;
        }
	else
	{
		return EAPI_STATUS_UNSUPPORTED;
	}

	pthread_mutex_lock(&lib_mutex);
	if((fd = open("/dev/bmc-i2c-eapi", O_RDWR)) < 0)
	{
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_READ_ERROR;
	}

	trxn.tBuffer[0] = 0x4;
	trxn.tBuffer[1] = 0x2;
	trxn.tBuffer[2] = 0;
	trxn.tBuffer[3] = trxn.Bus;
	trxn.tBuffer[4] = (Addr >> 8) & 0x7;
	trxn.tBuffer[5] = (uint8_t)Addr;

	trxn.Type = SEMA_EXT_IIC_EXT_COMMAND;

	trxn.Length = 0;

	if(ioctl(fd, PROBE_DEV, &trxn) < 0)
	{
		close(fd);
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_UNSUPPORTED;
	}

        if(trxn.tBuffer[1] & ~2)
	{
		close(fd);
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_UNSUPPORTED;
	}

	if (trxn.tBuffer[0] != 0)
	{
		close(fd);
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_UNSUPPORTED;
	}
	close(fd);
	pthread_mutex_unlock(&lib_mutex);
	return EAPI_STATUS_SUCCESS;
}


uint32_t EApiI2CWriteReadRaw(uint32_t Id, uint8_t Addr, void *pWBuffer, uint32_t WriteBCnt, void *pRBuffer, uint32_t RBufLen, uint32_t ReadBCnt)
{
	if (ReadBCnt > 1 && RBufLen == 0) {
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	if (ReadBCnt > RBufLen) {
		return EAPI_STATUS_MORE_DATA;
	}

	if (WriteBCnt == 0 && ReadBCnt == 0) {
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	if (WriteBCnt > MAX_BLOCK + 1) {
		return EAPI_STATUS_INVALID_BLOCK_LENGTH;
	}

	if (ReadBCnt > MAX_BLOCK + 1) {
		return EAPI_STATUS_INVALID_BLOCK_LENGTH;
	}

	pthread_mutex_lock(&lib_mutex);
	if(is_bmc_board)
	{
		static int file = 0;
	        static uint32_t sema_capability = 0;
		if(Id!= EAPI_ID_I2C_EXTERNAL && Id != EAPI_ID_I2C_LVDS_1)
		{
		
			if(sema_capability == 0)
			{
				static uint32_t id = 18;
				if (EApiBoardGetValue(id, &sema_capability) != EAPI_STATUS_SUCCESS){
					pthread_mutex_unlock(&lib_mutex);
					return EAPI_STATUS_UNSUPPORTED;
				}
			}

			if( Id == (SEMA_EAPI_ID_I2C_EXTERNAL_2))                // I2C Bus 3
			{
				if (!(sema_capability & SEMA_C_I2C3)){
					pthread_mutex_unlock(&lib_mutex);
					return EAPI_STATUS_UNSUPPORTED;
				}
			}
			else if(Id == (SEMA_EAPI_ID_I2C_EXTERNAL_3))    // I2C Bus 4
			{
				if (!(sema_capability & SEMA_C_I2C4))
				{
					pthread_mutex_unlock(&lib_mutex);
					return EAPI_STATUS_UNSUPPORTED;
				}
			}
			else
			{
				pthread_mutex_unlock(&lib_mutex);
				return EAPI_STATUS_UNSUPPORTED;
			}
		}
		if(file == 0)
			if((file = open_i2c_dev(Addr)) < 0){
				pthread_mutex_unlock(&lib_mutex);
				return EAPI_STATUS_UNSUPPORTED;
			}

		int ret;
		if(WriteBCnt > 0)
		{
			unsigned char *buf = pWBuffer;
			unsigned char addr = buf[0];
			ret = i2c_smbus_write_i2c_block_data(file, addr, WriteBCnt - 1, &buf[1]);
			if(ret < 0){
				pthread_mutex_unlock(&lib_mutex);
				return EAPI_STATUS_WRITE_ERROR;
			}
		}

		if(ReadBCnt > 0)
		{
			const unsigned char *buf = pWBuffer;
			if(WriteBCnt > 0)
			{
				unsigned char addr = buf[0];
				ret = i2c_smbus_read_i2c_block_data(file, addr, ReadBCnt, pRBuffer);
				if(ret < 0){
					pthread_mutex_unlock(&lib_mutex);
					return EAPI_STATUS_READ_ERROR;
				}
			}
		}
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_SUCCESS;
	}
	int fd;
        struct eapi_txn trxn;

	if (Id == EAPI_ID_I2C_EXTERNAL) {
		trxn.Bus = 1;
	}
	else if (Id == EAPI_ID_I2C_LVDS_1) {
		trxn.Bus = 2;
	}
	else if (Id == EAPI_ID_I2C_LVDS_2)
	{
		trxn.Bus = 3;
	}
	else if (Id == SEMA_EAPI_ID_I2C_EXTERNAL_2) {
		trxn.Bus = 4;
	}
	else{
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_UNSUPPORTED;
	}
	memset(trxn.tBuffer, 0, sizeof(unsigned char) * 50);
	if((fd = open("/dev/bmc-i2c-eapi", O_RDWR)) < 0)
	{
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_READ_ERROR;
	}
	trxn.tBuffer[0] = 0x4;      /*IF TYPE*/
	trxn.tBuffer[1] = 0x3;      /*RW TYPE*/
	trxn.tBuffer[2] = ReadBCnt; /*Read Len*/
	trxn.tBuffer[3] = trxn.Bus; /*BUS ID*/
	trxn.tBuffer[4] = (Addr >> 8) & 0x7;
	trxn.tBuffer[5] = (uint8_t)Addr;
	trxn.tBuffer[6] = WriteBCnt; /*Write Len*/

	if(ReadBCnt == 0)
		trxn.Length = WriteBCnt;
	else
		trxn.Length = ReadBCnt;

	/*Writing the data based on write byte count*/
	for (uint32_t i = 0; i < WriteBCnt; i++) {
		trxn.tBuffer[i + 7] = ((unsigned char*)pWBuffer)[i];
	}

	trxn.Type = SEMA_EXT_IIC_WRITE_READ;
	if(ioctl(fd, EAPI_TRXN, &trxn) < 0)
	{
		close(fd);
		pthread_mutex_unlock(&lib_mutex);
		return -1;
	}
	/*Read data in buffer as per ReadBCnt*/

	if(pRBuffer != NULL)
	{
		for (uint32_t i = 0; i < ReadBCnt; i++)
		{
			((unsigned char*)pRBuffer)[i] = trxn.tBuffer[i];
		}
	}
	close(fd);
	pthread_mutex_unlock(&lib_mutex);
	return EAPI_STATUS_SUCCESS;
}

uint32_t EApiSMBWriteTrans(uint32_t Addr, uint32_t Cmd, void *pBuffer, uint32_t ByteCnt)
{
	struct smbus_data trxn;
        int ret,i2c_handle;
	
	if (pBuffer == NULL || ByteCnt == 0)
        {
               	return EAPI_STATUS_INVALID_PARAMETER;
        }

	if(ByteCnt > MAX_BLOCK)
        {
                return EAPI_STATUS_INVALID_BLOCK_LENGTH;
        }
	
	pthread_mutex_lock(&lib_mutex);
        i2c_handle = open("/dev/bmc-i2c-eapi", O_RDWR);
	
        if (i2c_handle < 0) {
		pthread_mutex_unlock(&lib_mutex);
                return EAPI_STATUS_UNSUPPORTED;
        }

    	trxn.addr = Addr;
	trxn.Buffer[0] = Cmd;
	trxn.Length = ByteCnt;

	if(ByteCnt == 0x01)
	{
     		trxn.Type = TT_WBB;
   		trxn.Buffer[1] = ((unsigned char*)pBuffer)[0];
	}
	else if(ByteCnt == 0x02)
	{
     		trxn.Type = TT_WBW;
   		trxn.Buffer[1] = ((unsigned char*)pBuffer)[0];
  		trxn.Buffer[2] = ((unsigned char*)pBuffer)[1];
    	}
	else
	{
		close(i2c_handle);
		pthread_mutex_unlock(&lib_mutex);
               	return EAPI_STATUS_UNSUPPORTED;
	}
	
	ret = ioctl(i2c_handle, SMBUS_IOCTL_TRANS, &trxn);
        if(ret < 0){
                close(i2c_handle);
		pthread_mutex_unlock(&lib_mutex);
                return EAPI_STATUS_WRITE_ERROR;
        }
	
	close(i2c_handle);
	pthread_mutex_unlock(&lib_mutex);
	return EAPI_STATUS_SUCCESS;
}

uint32_t EApiSMBReadTrans(uint32_t Addr, uint32_t Cmd, void *pBuffer, uint32_t nByteCnt)
{
	struct smbus_data trxn;
        int ret,i2c_handle;

	if (pBuffer == NULL || nByteCnt == 0)
        {
               return EAPI_STATUS_INVALID_PARAMETER;
        }

	if (nByteCnt > MAX_BLOCK)
	{
       		return EAPI_STATUS_INVALID_PARAMETER;
        }
         
	pthread_mutex_lock(&lib_mutex);
        i2c_handle = open("/dev/bmc-i2c-eapi", O_RDWR);
                
	if (i2c_handle < 0) 
	{
		pthread_mutex_unlock(&lib_mutex);
                return EAPI_STATUS_UNSUPPORTED;
        }

	memset(trxn.Buffer, 0, sizeof(unsigned char) * 32);
	trxn.Buffer[0] = Cmd;
   	trxn.addr = Addr;
	trxn.Length = nByteCnt;
	
	if(nByteCnt == 0x01)
	{
		trxn.Type = TT_RBB;
	}
	else
	{
		trxn.Type = TT_RBW;
	}

	ret = ioctl(i2c_handle, SMBUS_IOCTL_TRANS, &trxn);
        if(ret < 0){
        	close(i2c_handle);
		pthread_mutex_unlock(&lib_mutex);
                return EAPI_STATUS_READ_ERROR;
        }
		
	if(nByteCnt == 0x01)
	{
		 ((unsigned char*)pBuffer)[0] = trxn.Buffer[0];
	}
	else if(nByteCnt == 0x02)
	{
		((unsigned char*)pBuffer)[0] = trxn.Buffer[0];
		((unsigned char*)pBuffer)[1] = trxn.Buffer[1];
	}
	else
	{
		close(i2c_handle);
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_UNSUPPORTED;
	}	
	
	close(i2c_handle);
	pthread_mutex_unlock(&lib_mutex);
	return EAPI_STATUS_SUCCESS;
}
