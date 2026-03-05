// SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
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

/**
 * @file adl-bmc.h 
 * @author 
 * @brief File containing the SEMA Linux 4.0 driver Read/Write Function Definitions
 *
 *
 */

#ifndef _ADL_BMC_H
#define _ADL_BMC_H

#define	MAX_BUFFER_SIZE		32
/* BMC Capabilities */
#define ADL_BMC_CAP_UPTIME 		(1 << 0)
#define ADL_BMC_CAP_RESTSRTEVT 		(1 << 1)
#define ADL_BMC_CAP_MEM			(1 << 2)
#define ADL_BMC_CAP_WD 			(1 << 3)
#define ADL_BMC_CAP_TEMP	        (1 << 4)
#define ADL_BMC_CAP_VM			(1 << 5)
#define ADL_BMC_CAP_BKLIGHT			(1 << 8)
#define ADL_BMC_CAP_CURRENTS 		(1 << 10)
#define ADL_BMC_CAP_BOOT_COUNTER 	(1 << 11)
#define ADL_BMC_CAP_FAN_CPU             (1 << 18)
#define ADL_BMC_SYS_FAN1_CAP            (1 << 19)
#define ADL_BMC_SYS_FAN2_CAP            (1 << 26)
#define ADL_BMC_SYS_FAN3_CAP            (1 << 27)
#define ADL_BMC_CAP_GPIOS               (1 << 28)



#define ADL_BMC_CAP_TEMP_1                   (1 << 0)
#define ADL_BMC_ERR_LOG_CAP 		     (1 << 3)	
#define ADL_BMC_SOFT_FAN_PWM_INTERPOLATION_CAP          (1 << 7)

/* BMC Commands*/
#define ADL_BMC_CMD_GET_BOARDERRLOG       	0x1c            ///< Get Board Error Log
#define ADL_BMC_CMD_SET_WD                      0x20            ///< Set/Clear Watchdog-timer
#define ADL_BMC_CMD_SET_PWD                     0x22            ///< Set/Clear PowerUp Watchdog-timer
#define ADL_BMC_CMD_SYSCFG                      0x27  		///< Get/Set system config register
#define ADL_BMC_CMD_CAPABILITIES		0x2F		///< Get BMC Capabilities
#define ADL_BMC_CMD_RD_VERSION1			0x30		///< Read version string 1
#define ADL_BMC_CMD_RD_VERSION2			0x31		///< Read version string 2
#define ADL_BMC_CMD_RD_TOM			0x32		///< Read total uptime minutes
#define ADL_BMC_CMD_RD_PWRUP_SECS		0x33		///< Read total seconds since power up
#define ADL_BMC_CMD_RD_PWRCYCLES		0x34		///< Read number of power cycles
#define ADL_BMC_CMD_RD_BMC_FLAGS		0x35		///< Read BMC flags
#define ADL_BMC_CMD_RD_RESTARTEVT		0x36		///< Read last system restart event
#define ADL_BMC_CMD_RD_CPU_TEMP			0x37		///< Read CPU Temperature
#define ADL_BMC_CMD_RD_SYSTEM_TEMP		0x38		///< Read System/Board Temperature
#define ADL_BMC_CMD_RD_MINMAX_TEMP		0x39		///< Read CPU Minimum/Maximum CPU and Board temperatures
#define ADL_BMC_CMD_RD_STARTUP_TEMP		0x3A		///< Read CPU Start Up temperature of CPU and Board
#define ADL_BMC_CMD_RD_BMC_STATUS		0x3D		///< Read BMC status
#define ADL_BMC_CMD_RD_BOOT_COUNTER_VAL		0x3E		///< Read number of boots
#define ADL_BMC_CMD_RD_BLVERSION		0x3F		///< Read boot loader version
#define ADL_BMC_CMD_SET_ADDRESS        		0x40            ///< Set address and length for flash access
#define ADL_BMC_CMD_WRITE_DATA                  0x41            ///< Write data to user flash
#define ADL_BMC_CMD_READ_DATA                   0x42            ///< Read data from user flash
#define ADL_BMC_CMD_CLEAR_DATA                  0x43            ///< Clear data from user flash
#define ADL_BMC_CMD_RD_AIN0			0x60		///< Read analog input Ch0
#define ADL_BMC_CMD_RD_CPU_FAN			0x68		///< Read CPU fan speed
#define ADL_BMC_CMD_RD_SYSTEM_FAN_1		0x6A		///< Read system fan 1 speed
#define ADL_BMC_CMD_RD_SYSTEM_FAN_2		0x6B		///< Read system fan 2 speed
#define ADL_BMC_CMD_RD_SYSTEM_FAN_3		0x6C		///< Read system fan 3 speed
#define ADL_BMC_CMD_RD_MPCURRENT		0x69		///< Read main power current 
#define ADL_BMC_CMD_EXC_CODE_TABLE		0x6F		///< Read exception code table
#define ADL_BMC_CMD_RD_MF_DATA_HW_REV		0x70		///< Read hardware revision string
#define ADL_BMC_CMD_RD_MF_DATA_SR_NO		0x71		///< Read board serial number
#define ADL_BMC_CMD_RD_MF_DATA_LR_DATA		0x72		///< Read board last repair date
#define ADL_BMC_CMD_RD_MF_DATA_MF_DATE		0x73		///< Read board manufactured date
#define ADL_BMC_CMD_RD_MF_DATA_2HW_REV		0x74		///< Read board 2nd hardware revision string
#define ADL_BMC_CMD_RD_MF_DATA_2SR_NO		0x75		///< Read board 2nd serial number
#define ADL_BMC_CMD_RD_MF_DATA_MAC_ID		0x76		///< Read board MAC id
#define ADL_BMC_CMD_RD_MF_DATA_RES		0x77		///< Read for future data 
#define ADL_BMC_CMD_RD_MF_DATA_PLATFORM_ID	0x78		///< Read platform id string
#define ADL_BMC_CMD_RD_MF_DATA_PLATFORM_SPEC_VER	0x79	///< Read platform specification version
#define ADL_BMC_CMD_SET_BKLITE                  0x80            ///< Set Backlight Brightness
#define ADL_BMC_CMD_GET_BKLITE                  0x81            ///< Get Backlight Brightness

#define ADL_BMC_CPU_FAN_TEMP_THRE_REG           0xA0
#define ADL_BMC_CPU_FAN_PWM_THRE_REG            0xA1
#define ADL_BMC_SYS_FAN1_TEMP_THRE_REG          0xA2
#define ADL_BMC_SYS_FAN1_PWM_THRE_REG           0xA3
#define ADL_BMC_SYS_FAN2_TEMP_THRE_REG          0xA8
#define ADL_BMC_SYS_FAN3_TEMP_THRE_REG          0xAA
#define ADL_BMC_SYS_FAN2_PWM_THRE_REG           0xA9
#define ADL_BMC_SYS_FAN3_PWM_THRE_REG           0xAB

#define ADL_BMC_CMD_GET_ADC_SCALE		0xA4
#define ADL_BMC_CMD_GET_VOLT_DESC		0xA5
#define ADL_BMC_CMD_EXT_HW_DESC			0xA6

#define ADL_BMC_CMD_RD_AIN8			0xD0
#define ADL_BMC_CMD_RD_SYSTEM2_TEMP		0xE0		///< Read current board 2nd Temperature
#define ADL_BMC_CMD_RD_SYSTEM2_MINMAX_TEMP	0xE1		///< Read minimum/maximum 2nd cpu and 2nd board temperatures
#define ADL_BMC_CMD_RD_SYSTEM2_STARTUP_TEMP	0xE2		///< Read 2nd startup temperaures of cpu and board

typedef enum
{
        TT_WB,
        TT_WBB,
        TT_WBW,
        TT_WWB,
        TT_RB,
        TT_RBB,
        TT_RBW,
        TT_RWB,
}tTransType;

#define TT_IsReadAccess(x)      (((Type==TT_RB) || (Type==TT_RBB) || (Type==TT_RBW) || (Type==TT_RWB))?1:0)

typedef struct SMBusInfo
{
	unsigned short Vendor_id;			// PCI Vendor ID
	unsigned short Device_id;			// PCI Device ID
	const char* Description;			// Descriptive text
}SMBusInfo;

/*****************************BMC Board Details********************************/
#define SEMA_FCH_Info	{0x1022, 0x1510,										\
	"ATI Technologies Inc SBx00 SMBus Controller"}

// FCH SMBus controller (D20:F4:R20)
// Boards:
//	- EXPRESS-BE
#define SEMA_BE_Info	{0x1022, 0x1422,										\
	"AMD Bolton Fusion SMBus Controller"}

#define SEMA_ICH7_Info	{0x8086u,0x27dau,										\
	"Intel(R) ICH7 Family SMBus Controller - 27da"}

// ICH9 SMBus controller (D31:F3:R20)
// Boards:
//	- CXR-GS45
#define SEMA_ICH9_Info	{0x8086u, 0x2930u,										\
	"Intel(R) ICH9 Family SMBus Controller - 2930"}

// ICH10 SMBus controller (D31:F3:R20)
// Boards:
//	- Test
#define SEMA_ICH10_Info	{0x8086u, 0x3A30u,										\
	"Intel(R) ICH10 Family SMBus Controller - 3A30"}

// PCH (QM57) SMBus controller (D31:F3:R20)
// Boards:
//	- H-QM57
//	- T-QM57
#define SEMA_PCH_Info	{0x8086u, 0x3b30u,										\
	"Intel(R) PCH Chipset SMBus Controller - 3b30"}

// 6 Series Chipset Family SMBus controller (D31:F3:R20)
// Boards:
//	- NuPRO-A40H
#define SEMA_S6_Info	{0x8086u, 0x1c22u,										\
	"Intel(R) Ser. 6 Chipset Family SMBus - 1c22"}

// 7 Series Chipset Family SMBus controller (D31:F3:R20)
// Boards:
//	- EXPRESS-IBE2
//	- Toucan-QM77
#define SEMA_S7_Info	{0x8086u, 0x1e22u,										\
	"Intel(R) Ser. 7 Chipset Family SMBus - 1e22"}

// Lynx Point SMBus controller (D31:F3:R20)
// Boards:
//	- EXPRESS-HL / Express-BD7
#define SEMA_HL_Info	{0x8086u, 0x8c22u,										\
	"Intel(R)Lynx Point SMBus Controller - 8c22"}

// Lynx Point SMBus controller (D31:F3:R20)
// Boards:
//	- cEXPRESS-HL
#define SEMA_CHL_Info	{0x8086u, 0x9c22u,										\
	"Intel(R)Lynx Point SMBus Controller - 9c22"}

// Bay Trail SMBus controller (D31:F3:R20)
// Boards:
//	- cEXPRESS-BT
#define SEMA_BT_Info	{0x8086u, 0x0F12u,										\
	"Intel(R)Bay Trail SMBus Controller - 0F12"}

// Board Well SMBus controller (D31:F3:R20)
// Boards:
//	- cEXPRESS-HL
#define SEMA_BL_Info	{0x8086u, 0x9CA2u,										\
	"Intel(R)Board Well SMBus Controller - 9CA2"}	

// Braswell SMBus controller (D31:F3:R20)
// Boards:
//	- cEXPRESS-BW
#define SEMA_BW_Info	{0x8086u, 0x2292u,										\
	"Intel(R)Braswell SMBus Controller - 2292"}

// Skylake SMBus controller (D31:F4:R20)
// Boards:
//	- EXPRESS-SL
#define SEMA_SL_Info	{0x8086u, 0xA123u,										\
	"Intel(R)Skylake SMBus Controller - A123"}

//	- cEXPRESS-SL
#define SEMA_cSL_Info	{0x8086u,0x9D23u,										\
	"Intel(R)cSkylake SMBus Controller - 9D23"}

// Broxton SMBus controller (D31:F1:R20)
// Boards:
//	- cEXPRESS-AL/EXPRESS-AL
#define SEMA_AL_Info	{0x8086u, 0x5AD4u,										\
	"Intel(R)Broxton SMBus Controller - 5AD4"}

// Cannon Lake and Coffee Lake SMBus controller (D31:F4)
// Boards:
//	- EXPRESS-CF
#define SEMA_CF_Info	{0x8086u,0xA323u,										\
	"Intel(R)Cannon/Coffee Lake SMBus Controller - A323"}

// Cormorant Lake / Denverton SMBus controller (D31:F4)
// Boards:
//	- EXPRESS-DN7
#define SEMA_DN7_Info	{0x8086u, 0x19DFu,										\
	"Intel(R)Denverton SMBus Controller - 19DF"}

// Whiskey Lake SMBus controller (D31:F4)
// Boards:
//	- cEXPRESS-WL
#define SEMA_WL_Info	{0x8086u, 0x9DA3u,										\
	"Intel(R) Whiskey Lake SMBus Controller - 9DA3"}

// Comet Lake SMBus controller (D31:F4)
// Boards:
//	- 
#define SEMA_CL_Info	{0x8086u, 0x06A3u,										\
	"Intel(R) Comet Lake SMBus Controller - 9DA3"}

#define NUM_SMBUS_CHIPSETS      20

SMBusInfo SMBUS_ChipsetInfo[NUM_SMBUS_CHIPSETS] =
{                                                                                       //##
        SEMA_ICH10_Info,                                                //00
        SEMA_ICH9_Info,                                                 //01
        SEMA_ICH7_Info,                                                 //02
        SEMA_PCH_Info,                                                  //03
        SEMA_S6_Info,                                                   //04
        SEMA_S7_Info,                                                   //05
        SEMA_FCH_Info,                                                  //06
        SEMA_HL_Info,                                                   //07
        SEMA_CHL_Info,                                                  //08
        SEMA_BT_Info,                                                   //09
        SEMA_BL_Info,                                                   //10
        SEMA_BE_Info,                                                   //11
        SEMA_SL_Info,                                                   //12
        SEMA_BW_Info,                                                   //13
        SEMA_cSL_Info,                                                  //14
        SEMA_AL_Info,                                                   //15
        SEMA_CF_Info,                                                   //16
        SEMA_DN7_Info,                                                  //17
        SEMA_WL_Info,                                                   //18
        SEMA_CL_Info
};

#define ADL_BMC_MAX_HW_MTR_INPUT			16 		//Max Number of supported voltages


#define SEMA_MAX_CAPABILITY_GROUP 8


#define DEBUG 0	

#if DEBUG
#define debug_printk(fmt...) printk(fmt)
#else
#define debug_printk(fmt...) 
#endif

// Collect capabilities
void CollectCapabilities(unsigned int *Capabilities, unsigned DataCount, unsigned char *SMBusDatas);

//Read 
int adl_bmc_i2c_read_device(struct adl_bmc_dev *adl_bmc, char reg, int bytes, void *dest);


//Write
int adl_bmc_i2c_write_device(struct adl_bmc_dev *adl_bmc, int reg, int bytes, void *src);

//SMBus Write/Reda
int adl_bmc_smbus_write_read_trans(int Address, int type, void *src);

#endif
