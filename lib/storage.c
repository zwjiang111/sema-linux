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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <eapi.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <common.h>
#include "conv.h"

#define SMC_FLASH_ALIGNMENT 4

#define EAPI_STOR_LOCK       _IOWR('a', 1, unsigned long)
#define EAPI_STOR_UNLOCK     _IOWR('a', 2, unsigned long)
#define EAPI_STOR_REGION     _IOWR('a', 3, unsigned long)

#define BLOCK_SIZE 4
#define EEPROM_USER_SIZE 1024
#define EEPROM_SCRE_SIZE 2048
#define EEPROM_ODM_SIZE  1024

pthread_mutex_t storage_mutex = PTHREAD_MUTEX_INITIALIZER;

struct secure {
        uint8_t Region;
        uint8_t permission;
        char passcode[8];
};
static char NVMEM_DEVICE[285];

static int initialize_nvmem()
{
   const struct dirent *de;
    DIR *dr = opendir("/sys/bus/platform/devices/adl-bmc-nvmem");

    if (dr == NULL)  // opendir returns NULL if couldn't open directory 
            return -1;
        
    memset(NVMEM_DEVICE, 0, sizeof(NVMEM_DEVICE));
    while ((de = readdir(dr)) != NULL) {
        if(strncmp(de->d_name, "nvmem_adl", strlen("nvmem_adl")) == 0) {
                sprintf(NVMEM_DEVICE, "/sys/bus/nvmem/devices/%s/nvmem", de->d_name);
                closedir(dr);
                return EAPI_STATUS_SUCCESS;
        }
    }
    closedir(dr);
    return -1;
}

#define NVMEM_INIT() if(initialize_nvmem() < 0) return EAPI_STATUS_NOT_INITIALIZED;

static int initialize_nvmem_sec()
{
   const struct dirent *de;
    DIR *dr = opendir("/sys/bus/platform/devices/adl-bmc-nvmem-sec");

    if (dr == NULL)  // opendir returns NULL if couldn't open directory 
        return -1;
    memset(NVMEM_DEVICE, 0, sizeof(NVMEM_DEVICE));
    while ((de = readdir(dr)) != NULL) {
        if(strncmp(de->d_name, "nvmem-sec", strlen("nvmem-sec")) == 0) {
                       
		sprintf(NVMEM_DEVICE, "/sys/bus/nvmem/devices/%s/nvmem", de->d_name);
                       
		closedir(dr);
                return 0;
        }
    }
    closedir(dr);

    return -1;
}

#define NVMEM_SEC_INIT() if(initialize_nvmem_sec() < 0) return EAPI_STATUS_NOT_INITIALIZED;



uint32_t EApiStorageCap(uint32_t Id, uint32_t *pStorageSize, uint32_t *pBlockLength)
{
    uint32_t status = EAPI_STATUS_SUCCESS;
    char buf[128] = { 0 };

    if (pStorageSize == NULL && pBlockLength == NULL)
    {
        return EAPI_STATUS_INVALID_PARAMETER;
    }

    if (is_bmc_board)
    {
        if (Id != EAPI_ID_STORAGE_STD)
        {
            return EAPI_STATUS_UNSUPPORTED;
        }
    }

    if (Id == EAPI_ID_STORAGE_STD)
    {
        char sysfile[256];
	char *saveptr;

        snprintf(sysfile, sizeof(sysfile),
                 "/sys/bus/platform/devices/adl-bmc-nvmem/capabilities/nvmemcap");

        status = read_sysfs_file(sysfile, buf, sizeof(buf));
        if (status)
            return EAPI_STATUS_READ_ERROR;

       
        char *token = strtok_r(buf, " \n",&saveptr);
        if (token == NULL || strstr(token, "StorageSize") == NULL)
            return EAPI_STATUS_READ_ERROR;

        token = strtok_r(NULL, " \n",&saveptr);
        if (token == NULL || pStorageSize == NULL)
            return EAPI_STATUS_READ_ERROR;
        *pStorageSize = (uint32_t)atoi(token);

        token = strtok_r(NULL, " \n",&saveptr);
        if (token == NULL || strstr(token, "BlockLength") == NULL)
            return EAPI_STATUS_READ_ERROR;

        token = strtok_r(NULL, " \n",&saveptr);
        if (token == NULL || pBlockLength == NULL)
            return EAPI_STATUS_READ_ERROR;
        *pBlockLength = (uint32_t)atoi(token);
    }
    else if (Id == EAPI_ID_STORAGE_SCR)
    {
        if (pStorageSize) *pStorageSize = EEPROM_SCRE_SIZE;
        if (pBlockLength) *pBlockLength = BLOCK_SIZE;
    }
    else if (Id == EAPI_ID_STORAGE_ODM)
    {
        if (pStorageSize) *pStorageSize = EEPROM_ODM_SIZE;
        if (pBlockLength) *pBlockLength = BLOCK_SIZE;
    }
    else
    {
        return EAPI_STATUS_UNSUPPORTED;
    }

    return status;
}

uint32_t EApiStorageAreaRead(uint32_t Id, uint32_t Offset, void* pBuffer, uint32_t BufLen, uint32_t  ByteCnt)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	int ret = 0;
	int fd;
	
	if(is_bmc_board)
	{
		if(Id != EAPI_ID_STORAGE_STD)
                {
                        return EAPI_STATUS_UNSUPPORTED;
                }
	}

	if (Id == EAPI_ID_STORAGE_STD)
	{
		NVMEM_INIT();
		if (Offset + ByteCnt > EEPROM_USER_SIZE)
		{
			return EAPI_STATUS_INVALID_BLOCK_LENGTH;
		}
	}
	else if (Id == EAPI_ID_STORAGE_SCR )
	{
		NVMEM_SEC_INIT();
		if (Offset + ByteCnt > EEPROM_SCRE_SIZE)
		{
			return EAPI_STATUS_INVALID_BLOCK_LENGTH;
		}
	}
	else if ( Id == EAPI_ID_STORAGE_ODM)
	{
		NVMEM_SEC_INIT();
		if (Offset + ByteCnt > EEPROM_ODM_SIZE)
		{
			return EAPI_STATUS_INVALID_BLOCK_LENGTH;
		}
	}
	else
	{
		return EAPI_STATUS_UNSUPPORTED;
	}

	if (pBuffer == NULL || ByteCnt == 0 || BufLen == 0)
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}
	if (ByteCnt > BufLen)
	{
		return EAPI_STATUS_MORE_DATA;
	}
	pthread_mutex_lock(&storage_mutex);

	if(!is_bmc_board){
		uint32_t BytecntTemp = ByteCnt;
		uint32_t temp = (ByteCnt % 4) ? ByteCnt + 4 - (ByteCnt % 4) : ByteCnt;
		uint8_t pBufferTemp[temp];
		struct secure data;

		data.Region = Id;

		if ((fd = open("/dev/bmc-nvmem-eapi", O_RDWR)) < 0)
		{
			pthread_mutex_unlock(&storage_mutex);
			return -1;
		}
		if (ioctl(fd, EAPI_STOR_REGION, &data) < 0)
		{
			close(fd);
			pthread_mutex_unlock(&storage_mutex);
			return EAPI_STATUS_READ_ERROR;
		}
		else
			close(fd);

		fd = open(NVMEM_DEVICE, O_RDONLY);

		if (fd < 0)
		{
			pthread_mutex_unlock(&storage_mutex);
			return EAPI_STATUS_READ_ERROR;
		}
	
		memset(pBufferTemp, 0, sizeof(pBufferTemp));
		lseek(fd, Offset, SEEK_SET);

		if (ByteCnt % 4 != 0)
		{
			ByteCnt += 4 - (ByteCnt % 4);
		}
		pBufferTemp[sizeof(pBufferTemp)]='\0';
		
		ret = read(fd, pBufferTemp, ByteCnt);
	
		memcpy(pBuffer, pBufferTemp, BytecntTemp);
	}
	else
	{
		fd = open(NVMEM_DEVICE, O_RDONLY);
        	if (fd < 0){
			pthread_mutex_unlock(&storage_mutex);
                	return -1;
		}

        	lseek(fd, Offset, SEEK_SET);

        	ret = read(fd, pBuffer, ByteCnt);
	}
	if (ret > 0)
		close(fd);
	else {
		close(fd);
		pthread_mutex_unlock(&storage_mutex);
		return EAPI_STATUS_READ_ERROR;
	}
	pthread_mutex_unlock(&storage_mutex);
	return status;
}

uint32_t EApiStorageAreaWrite(uint32_t Id, uint32_t Offset, void* pBuffer, uint32_t ByteCnt)
{
    uint32_t status = EAPI_STATUS_SUCCESS;
    int ret, fd;
    unsigned char* buffer = NULL;
    
    pthread_mutex_lock(&lib_mutex);
    if (is_bmc_board)
    {
        if (Id != EAPI_ID_STORAGE_STD)
        {
            return EAPI_STATUS_UNSUPPORTED;
        }
    }

    if (Id == EAPI_ID_STORAGE_STD)
    {
        NVMEM_INIT();
        if (Offset + ByteCnt > EEPROM_USER_SIZE)
        {
            return EAPI_STATUS_INVALID_BLOCK_LENGTH;
        }
    }
    else if (Id == EAPI_ID_STORAGE_SCR)
    {
        NVMEM_SEC_INIT();
        if (Offset + ByteCnt > EEPROM_SCRE_SIZE)
        {
            return EAPI_STATUS_INVALID_BLOCK_LENGTH;
        }
    }
    else if (Id == EAPI_ID_STORAGE_ODM)
    {
        NVMEM_SEC_INIT();
        if (Offset + ByteCnt > EEPROM_ODM_SIZE)
        {
            return EAPI_STATUS_INVALID_BLOCK_LENGTH;
        }
    }
    else
    {
        return EAPI_STATUS_UNSUPPORTED;
    }

    if (ByteCnt == 0 || pBuffer == NULL)
    {
        return EAPI_STATUS_INVALID_PARAMETER;
    }

    if (is_bmc_board)
    {
        uint32_t pBlockLength = SMC_FLASH_ALIGNMENT;
        if ((Offset % pBlockLength) != 0)
	{
	    pthread_mutex_unlock(&lib_mutex);
            return EAPI_STATUS_INVALID_BLOCK_ALIGNMENT;
	}
    }

    if ((ByteCnt % 4) != 0)
    {
        int rem = 4 - (ByteCnt % 4);
        ByteCnt = ByteCnt + rem;
        buffer = calloc(ByteCnt, sizeof(unsigned char));
        if (buffer == NULL)
        {
	    pthread_mutex_unlock(&lib_mutex);
            return EAPI_STATUS_READ_ERROR;
        }

        memcpy(buffer, pBuffer, ByteCnt);
        EApiStorageAreaRead(Id, Offset + (ByteCnt - rem), buffer + (ByteCnt - rem), rem, rem);
    }
    else
    {
        buffer = (unsigned char*)pBuffer;
    }

    if (!is_bmc_board)
    {
        struct secure data = { 0 };
        data.Region = Id;
        if ((fd = open("/dev/bmc-nvmem-eapi", O_RDWR)) < 0)
        {
            if (buffer != (unsigned char*)pBuffer && buffer != NULL)
                free(buffer);
	    pthread_mutex_unlock(&lib_mutex);
            return -1;
        }
        if (ioctl(fd, EAPI_STOR_REGION, &data) < 0)
        {
            close(fd);
            if (buffer != (unsigned char*)pBuffer && buffer != NULL)
                free(buffer);

	    pthread_mutex_unlock(&lib_mutex);
            return EAPI_STATUS_UNSUPPORTED;
        }
        else
        {
            close(fd);
        }
    }

    fd = open(NVMEM_DEVICE, O_WRONLY);
    
    if (fd < 0)
    {
        if (buffer != (unsigned char*)pBuffer && buffer != NULL)
            free(buffer);

	pthread_mutex_unlock(&lib_mutex);
        return EAPI_STATUS_WRITE_ERROR;
    }
    lseek(fd, Offset, SEEK_SET);
    ret = write(fd, buffer, ByteCnt);
    close(fd);

    if (ret <= 0)
    {
        if (buffer != (unsigned char*)pBuffer && buffer != NULL)
            free(buffer);
	pthread_mutex_unlock(&lib_mutex);
        return EAPI_STATUS_WRITE_ERROR;
    }

    if (buffer != (unsigned char*)pBuffer && buffer != NULL)
        free(buffer);

    pthread_mutex_unlock(&lib_mutex);
    return status;
}

uint32_t EApiStorageHexRead(uint32_t Id, uint32_t Offset, void* pBuffer, uint32_t BufLen, uint32_t  ByteCnt)
{
	int ret = 0;
	ret = EApiStorageAreaRead(Id,Offset, pBuffer, BufLen, ByteCnt);

	if (ret)
		return ret;
	else
		return EAPI_STATUS_SUCCESS;
}

uint32_t EApiStorageHexWrite(uint32_t Id, uint32_t Offset, void* pBuffer, uint32_t ByteCnt)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	int ret = 0, fd = 0;
	struct secure data = { 0 };
	char* hex_buf = NULL;
	unsigned char* buf=pBuffer;
	char result[2048] = { 0 };  // Initialize to ensure no uninitialized use
        uint32_t i;

	pthread_mutex_lock(&lib_mutex);
	for(i=0;i<ByteCnt*2;i++)
	{
		if(isxdigit(buf[i])==0)
		{
			printf("Please provide the hex data only 'A'-'F','a'-'f' and '0'-'9'\n");
			pthread_mutex_unlock(&lib_mutex);
			return EAPI_STATUS_INVALID_PARAMETER;
		}
	}

	Conv_String2HexByte(pBuffer,result);
	hex_buf = result;

	if (Id == EAPI_ID_STORAGE_STD)
	{
		NVMEM_INIT();
		if (Offset + ByteCnt > EEPROM_USER_SIZE)
		{
			pthread_mutex_unlock(&lib_mutex);
			return EAPI_STATUS_INVALID_BLOCK_LENGTH;
		}
	}
	else if (Id == EAPI_ID_STORAGE_SCR)
	{
		NVMEM_SEC_INIT();
		if (Offset + ByteCnt > EEPROM_SCRE_SIZE)
		{
			pthread_mutex_unlock(&lib_mutex);
			return EAPI_STATUS_INVALID_BLOCK_LENGTH;
		}
	}
	else if (Id == EAPI_ID_STORAGE_ODM)
	{
		NVMEM_SEC_INIT();
		if (Offset + ByteCnt > EEPROM_ODM_SIZE)
		{
			pthread_mutex_unlock(&lib_mutex);
			return EAPI_STATUS_INVALID_BLOCK_LENGTH;
		}
	}
	else
	{
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_UNSUPPORTED;
	}

        const unsigned char *buffer;
	
	buffer = (unsigned char*)hex_buf;

	data.Region = Id;

    if((fd = open("/dev/bmc-nvmem-eapi", O_RDWR)) < 0)
    {
	pthread_mutex_unlock(&lib_mutex);
        return -1;
    }
    if(ioctl(fd, EAPI_STOR_REGION, &data ) < 0)
    {
        close(fd);
	pthread_mutex_unlock(&lib_mutex);
        return EAPI_STATUS_UNSUPPORTED;
    }
    else
    {
        close(fd);
    }
	
	fd = open(NVMEM_DEVICE, O_WRONLY);

    if (fd < 0)
    {
	pthread_mutex_unlock(&lib_mutex);
	return EAPI_STATUS_WRITE_ERROR;
    }
    lseek(fd,Offset,SEEK_SET);
        
	ret = write(fd,buffer,ByteCnt);
    if (ret > 0)
    {
        close(fd);
    }
    else
    {
        close(fd);
	pthread_mutex_unlock(&lib_mutex);
        return EAPI_STATUS_WRITE_ERROR;
    }

    pthread_mutex_unlock(&lib_mutex);
    return status;
}
uint32_t EApiGUIDWrite(uint32_t Id, uint32_t Offset, void* pBuffer, uint32_t ByteCnt)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	int ret, fd;
	struct secure data;
	const char* hex_buf;
	char result[2048];

	NVMEM_SEC_INIT();

	pthread_mutex_lock(&lib_mutex);
	if (Offset + ByteCnt > EEPROM_ODM_SIZE)
	{
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_INVALID_BLOCK_LENGTH;
	}

	Conv_String2HexByte(pBuffer,result);
	hex_buf = result;

	data.Region = Id;

    if((fd = open("/dev/bmc-nvmem-eapi", O_RDWR)) < 0)
    {
	    pthread_mutex_unlock(&lib_mutex);
            return -1;
    }
    if(ioctl(fd, EAPI_STOR_REGION, &data ) < 0)
    {
            close(fd);
	    pthread_mutex_unlock(&lib_mutex);
            return EAPI_STATUS_UNSUPPORTED;
    }
    else
    {
            close(fd);
    }
	
	fd = open(NVMEM_DEVICE, O_WRONLY);

    if (fd < 0)
    {
	    pthread_mutex_unlock(&lib_mutex);
            return EAPI_STATUS_WRITE_ERROR;
    }

    lseek(fd,Offset,SEEK_SET);
    ret = write(fd,hex_buf,ByteCnt);
    if (ret > 0)
    {
            close(fd);
    }
    else
    {
            close(fd);
	    pthread_mutex_unlock(&lib_mutex);
            return EAPI_STATUS_WRITE_ERROR;
    }

    pthread_mutex_unlock(&lib_mutex);
    return status;
}

uint32_t EApiStorageAreaClear(uint32_t Id)
{
	(void)Id; 
	return EAPI_STATUS_UNSUPPORTED;
}

uint32_t EApiStorageLock(uint32_t Id)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	int fd;
	struct secure data;
	if (Id != EAPI_ID_STORAGE_SCR && Id != EAPI_ID_STORAGE_ODM)
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}
	data.Region=Id;

	pthread_mutex_lock(&lib_mutex);

        if((fd = open("/dev/bmc-nvmem-eapi", O_RDWR)) < 0)
	{
		pthread_mutex_unlock(&lib_mutex);
		return -1;
	}

	if(ioctl(fd, EAPI_STOR_LOCK, &data ) < 0)
        {
		close(fd);
		pthread_mutex_unlock(&lib_mutex);
		return EAPI_STATUS_UNSUPPORTED;
	}

	close(fd);
	pthread_mutex_unlock(&lib_mutex);
	return status;
}

uint32_t EApiStorageUnLock(uint32_t Id, uint32_t Permission,const char* passcode)
{
	uint32_t status = EAPI_STATUS_SUCCESS;
	int fd;
	struct secure data;
	
	if (Id != EAPI_ID_STORAGE_SCR && Id != EAPI_ID_STORAGE_ODM)
	{
		return EAPI_STATUS_INVALID_PARAMETER;
	}

	data.Region = Id;
	data.permission = Permission;

	memset(data.passcode,0,sizeof(data.passcode));
	memcpy(data.passcode,passcode,strlen(passcode));

	pthread_mutex_lock(&lib_mutex);

	if((fd = open("/dev/bmc-nvmem-eapi", O_RDWR)) < 0)
    	{
	    pthread_mutex_unlock(&lib_mutex);
            return -1;
        }

    	if(ioctl(fd, EAPI_STOR_UNLOCK,&data ) < 0)
    	{
		close(fd);
		pthread_mutex_unlock(&lib_mutex);
            	return EAPI_STATUS_UNSUPPORTED;
    	}
	
	close(fd);
	pthread_mutex_unlock(&lib_mutex);
    	return status;
}
