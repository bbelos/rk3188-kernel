/*++
 
 Copyright (c) 2012-2022 ChipOne Technology (Beijing) Co., Ltd. All Rights Reserved.
 This PROPRIETARY SOFTWARE is the property of ChipOne Technology (Beijing) Co., Ltd. 
 and may contains trade secrets and/or other confidential information of ChipOne 
 Technology (Beijing) Co., Ltd. This file shall not be disclosed to any third party,
 in whole or in part, without prior written consent of ChipOne.  
 THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. CHIPONE DISCLAIMS ALL EXPRESS OR 
 IMPLIED WARRANTIES.  
 
 File Name:    Common.h
 Abstract:
     Common.
 Author:       JunFu Zhang
               Zhimin Tian
 Date :        12 24,2012
 Version:      1.0 for sensor only
 History :
     03 28,2012,   0.1[first revision]
     
 --*/


#ifndef _COMMON_H_
#define _COMMON_H_
//-----------------------------------------------------------------------------
// Pin Declarations
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Global CONSTANTS
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macro DEFINITIONS
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Struct, Union and Enum DEFINITIONS
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------

#if (ICN838X_PACKAGE ==  ICN8380)    
    #define GPIO_INT        GPIO1
    #define GPIO_WAKEUP     GPIO0   
    #define SPI_CS_WAKEUP   GPIO3  
#elif (ICN838X_PACKAGE == ICN8383) 
    #define GPIO_INT        GPIO1
    #define GPIO_WAKEUP     GPIO0   
    #define SPI_CS_WAKEUP   GPIO3      
#elif (ICN838X_PACKAGE == ICN8382) 
    #define GPIO_INT        GPIO1
    #define GPIO_WAKEUP     GPIO0   
    #define SPI_CS_WAKEUP   GPIO3      
#else 
    #error "the package is wrong"
#endif


//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------

void PeripheralInit(void);
void SetIntLine(U8 u8State);

void OscClkInit(U8 SpiOrIic, U8 edge, U32 clk);
void SysClkInit(void);
#endif


