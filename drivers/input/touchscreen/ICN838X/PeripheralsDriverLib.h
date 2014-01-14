/*++
 
 Copyright (c) 2012-2022 ChipOne Technology (Beijing) Co., Ltd. All Rights Reserved.
 This PROPRIETARY SOFTWARE is the property of ChipOne Technology (Beijing) Co., Ltd. 
 and may contains trade secrets and/or other confidential information of ChipOne 
 Technology (Beijing) Co., Ltd. This file shall not be disclosed to any third party,
 in whole or in part, without prior written consent of ChipOne.  
 THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. CHIPONE DISCLAIMS ALL EXPRESS OR 
 IMPLIED WARRANTIES.  
 
 File Name:    PeripheralsDriverLib.h
 Abstract:
     Flash.
 Author:       JunFu Zhang
               Zhimin Tian
 Date :        12 24,2012
 Version:      1.0 for sensor only
 History :
     03 28,2012,   0.1[first revision]
    

 --*/
 


#ifndef _PERIPHERALDRIVERLIB_H_
#define _PERIPHERALDRIVERLIB_H_

#include "constant.h"


//-----------------------------------------------------------------------------
// Pin Declarations
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Global CONSTANTS
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macro DEFINITIONS
//-----------------------------------------------------------------------------

#define WATCH_DOG_HW_MODE                (BIT5)
#define WATCH_DOG_SW_MODE                (BIT6)
#define WATCH_DOG_COMP_MODE              (BIT5|BIT6)

#define WATCH_DOG_PERIOD_100MS           (BIT0)
#define WATCH_DOG_PERIOD_200MS           (BIT1)
#define WATCH_DOG_PERIOD_400MS           (BIT2)
#define WATCH_DOG_PERIOD_800MS           (BIT3)
#define WATCH_DOG_PERIOD_1600MS          (BIT4)


#define SUSPEND_MODE_POWERDOWN           0
#define SUSPEND_MODE_RTC_MODE            1


#define SUSPEND_MODE                     SUSPEND_MODE_POWERDOWN

//system setting 2: Uart baudrate
#define BAUDRATE                         57600


#define SOFT_LOCK                        0x3d
#define SOFT_UNLOCK                      0x4B


#define FLASH_CMD_READ                   0x40
#define FLASH_CMD_WRITE                  0x01
#define FLASH_CMD_ERASE                  0x02

//-----------------------------------------------------------------------------
// Struct, Union and Enum DEFINITIONS
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Function PROTOTYPES

//-----------------------------------------------------------------------------

void SetGpioVoltage(U8 u8GpioVol);
void GpioEnable    (U8 bIndex) ;
void GpioDisable   (U8 bIndex);
void GpioSetDir    (U8 bIndex, U8 bOutput) ;
void GpioSetValue  (U8 bIndex, U8 bValue) ;
void GpioSetPinMux (U8 bIndex, U8 bValue) ;
U8   GpioGetValue  (U8 bIndex) ;


void WakeupEnable (U8 bIndex, U8 bValue);
void EnterHibernateMode(void);

void McuReset(void);
void Patch(void);

#endif
