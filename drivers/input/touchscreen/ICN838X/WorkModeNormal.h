/*++
 
 Copyright (c) 2012-2022 ChipOne Technology (Beijing) Co., Ltd. All Rights Reserved.
 This PROPRIETARY SOFTWARE is the property of ChipOne Technology (Beijing) Co., Ltd. 
 and may contains trade secrets and/or other confidential information of ChipOne 
 Technology (Beijing) Co., Ltd. This file shall not be disclosed to any third party,
 in whole or in part, without prior written consent of ChipOne.  
 THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. CHIPONE DISCLAIMS ALL EXPRESS OR 
 IMPLIED WARRANTIES.  
 
 File Name:    WorkModeNormal.h
 Abstract:
               Normal work flow
 Author:       JunFu Zhang
               Zhimin Tian
 Date :        12 24,2012
 Version:      1.0 for sensor only
 History :
     03 28,2012,   0.1[first revision]
    

 --*/


#ifndef _WORKMODENORMAL_H_
#define _WORKMODENORMAL_H_
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

//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------
void NormalWork(void);
void TP_IicClkTune(U32 iicclk);
void TP_SpiClkTune(U32 spiclk);
void TP_Init(void);
void TP_ReInit(void);
void TP_ChargerState(U8 state);
void TP_Sleep(void);
void TP_WakeUp(void);
void TP_Scan(void);
void TP_Scan_Rawdata(void);
void TP_NormalWork(void);
unsigned char * TP_ReadPoint(void);
STRUCT_PANEL_PARA *  TP_GetPanelPara(void);


#endif


