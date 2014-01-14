/*++

THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,
WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. VIA TECHNOLOGIES, INC. 
DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES.  

File Name:
    Config.h

Abstract:
    user  configuration 

Author:       JunFu Zhang
              Zhimin Tian
Date :        12 24,2012
Version:      1.0 for sensor only
History :
    03 28,2012,   0.1[first revision]
   


*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "constant.h"

//System setting 1: system clock
#define SYSCLK                          30000000
#define INTERFACE_CLK                   30000000

//System setting 2: package
#define ICN838X_PACKAGE                 ICN8380

//System setting 3: please enable this macro when you want to monitor system output via uar
//#define DEFAULT_UART_OUTPUT             OUTPUT_NOTHING//OUTPUT_RAWDATA|OUTPUT_DIFDATA//OUTPUT_RAWDATA//OUTPUT_NOTHING            

//System setting 4: Enable or disable wake up function.
#define WAKE_UP_MODE                    WAKE_UP_INTERFACE

//System setting 5: Choose Host Comm 
#define HOST_COMM                       HOST_COMM_IIC

//System setting 6: for large panel,use two chip
//#define DOUBLE_DIE_SUPPORT

//System setting 7: Can compatible with 1 or 2 bytes offset 
#define I2C_SLAVE_CHIPONE               (0x60)
#define CHIPONE_OFFSET_LENGTH           OFFSET_LENGTH_1BYTE 

//System setting 8:System info
//Please update following information when modified code
#define LIB_VERSION                                0x51
#define IC_MIAN_VERSION                            0x83
#define IC_SUB_VERSION                             ICN838X_PACKAGE
#define FIRMWARE_MAIN_VERSION                      0x01
#define FIRMWARE_SUB_VERSION                       0x00
#define CUSTOMER_ID                                CUSTOMER_ID_CHIPONE
#define PRODUCT_ID                                 PRODUCT_ID_CHIPONE

//System setting 9:Config uart outputing info,when DEBUG_MODE macro is enabled

#define COMMON_DEBUG
#ifdef COMMON_DEBUG
#define COMM_TRACE(x...)    printk(x)
#else 
#define COMM_TRACE(x...)
#endif

#define FLOW_DEBUG
#ifdef FLOW_DEBUG
#define FLOW_TRACE(x...)    printk(x)
#else 
#define FLOW_TRACE(x...)
#endif

//#define HOSTCOMM_DEBUG
#ifdef HOSTCOMM_DEBUG
#define HOSTCOMM_TRACE(x...)    printk(x)
#else 
#define HOSTCOMM_TRACE(x...)
#endif

#define LIB_DEBUG
#ifdef LIB_DEBUG
#define LIB_TRACE(x...) printk(x)
#else 
#define LIB_TRACE(x...)
#endif


//#define PM_DEBUG
#ifdef  PM_DEBUG
#define PM_TRACE(x...)  printk(x)
#else 
#define PM_TRACE(x...)

#endif


#endif
