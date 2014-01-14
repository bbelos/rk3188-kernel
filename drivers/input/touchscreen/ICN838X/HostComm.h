/*++
 
 Copyright (c) 2012-2022 ChipOne Technology (Beijing) Co., Ltd. All Rights Reserved.
 This PROPRIETARY SOFTWARE is the property of ChipOne Technology (Beijing) Co., Ltd. 
 and may contains trade secrets and/or other confidential information of ChipOne 
 Technology (Beijing) Co., Ltd. This file shall not be disclosed to any third party,
 in whole or in part, without prior written consent of ChipOne.  
 THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. CHIPONE DISCLAIMS ALL EXPRESS OR 
 IMPLIED WARRANTIES.  
 
 File Name:    HostComm.h
 Abstract:
                    Hostcomm state machine.
 Author:       JunFu Zhang
               Zhimin Tian
 Date :        12 24,2012
 Version:      1.0 for sensor only
 History :
     03 28,2012,   0.1[first revision]
 

 --*/


#ifndef _HOSTCOMM_H_
#define _HOSTCOMM_H_

#include "Ctype.h"

//-----------------------------------------------------------------------------
// Pin Declarations
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Global CONSTANTS
//-----------------------------------------------------------------------------

#define MASTER_CHIPONE                 (0x01)
#define MASTER_OTHER_VENDOR            (0x02)

#define  ROPORT_WITH_PRESSURE

//Auto means EVENT_ID_DOWN,EVENT_ID_UP must send , EVENT_ID_MOVE,EVENT_ID_STAY overwriteable
#define TRANX_MODE_AUTO                 0
#define TRANX_MODE_SYNC                 1
#define TRANX_MODE_ASYNC                2

#define HOSTCOMM_TRANX_MODE             TRANX_MODE_AUTO


#define PARALLEL_SIMPLE_MODE            0x00
#define PARALLEL_STANDARD_MODE          0x01
#define PARALLEL_ADVANCED_MODE          0x02

#define SERIAL_MODE                     0x10

#define HOSTCOM_REPORT_MODE             PARALLEL_STANDARD_MODE   


#define FT_EVENT_ID_DOWN                0x00 
#define FT_EVENT_ID_UP                  0x40 
#define FT_EVENT_ID_CONTACT             0x80 


//-----------------------------------------------------------------------------
// Macro DEFINITIONS
//-----------------------------------------------------------------------------
typedef enum{
    BUF_NULL = 0x00,BUF_A = 0x0a,BUF_B
}BUF_NAME;
typedef enum{
    BUF_S_IDLE = 0,BUF_S_WRITTING,BUF_S_READY,BUF_S_READING
}BUF_STATE;
typedef enum{
    BUF_P_OVERWRITABLE = 0,BUF_P_MUST_SEND
}BUF_PRIORITY;
//-----------------------------------------------------------------------------
// Struct, Union and Enum DEFINITIONS
//-----------------------------------------------------------------------------


typedef struct _STRUCT_HOSTCOMM_STATE
{
    U8   u8Master;         //IICMaster (CHIPONE , others)
    U8   u8BufNeedTranx;
    U8   u8LastBufReady;
    U8   u8ReportWindowOpen;
}PACKED STRUCT_HOSTCOMM_STATE;


//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------
extern STRUCT_HOSTCOMM_STATE structHostCommState;
extern STRUCT_FRAME_INFO  structFrameInfo; //Output Points infor  after filters, fixed addr mode(comfortable when tranx)


//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------
void main_loop(void);   



void HostRWFuncInit(STRUCT_RWFUNC * FuncPtr);

void HostCommInit(void);
void OutputFrameInfo(void);

extern STRUCT_RWFUNC *RWFunc; 
#endif


