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
     Common.
 Author:       JunFu Zhang
               Zhimin Tian
 Date :        12 24,2012
 Version:      1.0 for sensor only
 History :
     03 28,2012,   0.1[first revision]
    

 --*/


#ifndef _MEMORYMAPCHIPONE_H_
#define _MEMORYMAPCHIPONE_H_

#include "WorkModeNormal.h"

//-----------------------------------------------------------------------------
// Pin Declarations
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Global CONSTANTS
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Macro DEFINITIONS
//-----------------------------------------------------------------------------

typedef enum{
    CMD_MCU_RESET = 1,CMD_ENTER_HIBERNATE,CMD_WRITE_PARA_TO_FLASH,
    CMD_SET_TX_VOL_603 = 0x10,CMD_SET_TX_VOL_633,CMD_SET_TX_VOL_665,CMD_SET_TX_VOL_701,
    CMD_SET_TX_VOL_741,CMD_SET_TX_VOL_786,CMD_SET_TX_VOL_837,CMD_SET_TX_VOL_895,
    CMD_UPDATE_RAW_NORMAL = 0x20,CMD_UPDATE_BASELINE = 0x30,CMD_CHARGER_PLUG_IN = 0x55,CMD_CHARGER_PLUG_OUT = 0x66
}COMMAND;

typedef enum{
    POWER_MODE_ACTIVE = 0,POWER_MODE_MONITOR,POWER_MODE_HIBERNATE
}_POWER_MODE;


#define     SYS_BUSY    1
#define     SYS_IDLE    0


#define LENGTH_CHIPONE_MEMORY_HEADER                   0x10
#define LENGTH_CHIPONE_MEMORY_BODY                     0xd0
#define LENGTH_CHIPONE_MEMORY_DEBUG_TAIL               0x20



#define CHIPONE_MEMORY_HEADER_TOP_OFFSET               0x10
#define CHIPONE_MEMORY_BODY_ABBUFFER_TOP_OFFSET        0x80
#define CHIPONE_MEMORY_BODY_RAW_NORMAL_OFFSET          0xa0
#define CHIPONE_MEMORY_BODY_BLOCK_TOP_OFFSET           0xe0

//#define CHIPONE_MEMORY_BLOCK_BUF_BOTTOM_OFFSET 0xa0

//-----------------------------------------------------------------------------
// Struct, Union and Enum DEFINITIONS
//-----------------------------------------------------------------------------
//Every IIC page contain 256 bytes = (header(16 bytes) + body_A(208 bytes)+ debug(32 bytes))
typedef struct _STRUCT_COMMON_HEADER            //0ffset : 0x00~0x0f ,length 16
{
    _WORK_MODE WorkMode;  // wr
    U8 u8SystemBusyFlag;  // ro
    U8 u8DataReadyFlag;   //ro
    U8 u8RowIndex;        //wr
    COMMAND Cmd; 
    _POWER_MODE PowerMode;
    
    U8 u8FirstPowerOn;
    U8 u8ChargerPlugIn;
    U8 u8FirstPowerOnOrResume;
    U8 u8LibVersion;
    U8 u8IcVersionMain;
    U8 u8IcVersionSub;
    U8 u8FirmWareMainVersion;
    U8 u8FirmWareSubVersion;
    U8 u8CumstomerId;
    U8 u8ProductId;
    
}PACKED STURCT_COMMON_HEADER;

typedef struct _STRUCT_NORMAL_MODE_MEMORY       //0ffset : 0x10~0xdf,length 208
{ 
    U8 u8PointerPosAreaBufA[112];             //0ffset : 0x10~0x9f,length 144(20 touch pos maxima)
    U8 u8PointerPosAreaBufB[112];             //0ffset : 0x10~0x9f,length 144(20 touch pos maxima),u8PointerPosAreaBufA and u8PointerPosAreaBufB used to implement AB buffer state machine
    U8 u8RawNormal[32];
    U8 u8RegisterArea[64];                    //0ffset : 0xa0~0xdf,length 64
}PACKED STRUCT_NORMAL_MODE_MEMORY;

typedef struct _STRUCT_FACTORY_MODE_MEMORY      //0ffset : 0x10~0xdf,length 208
{
    U8 u8HostCommMemoryMap[144];
    U8 u8BlockArea[64];
}PACKED STRUCT_FACTORY_MODE_MEMORY;

typedef struct _STRUCT_CONFIG_MODE_MEMORY       //0ffset : 0x10~0xdf,length 208
{
    U8 u8HostCommMemoryMap[144];
    U8 u8BlockArea[64];
}PACKED STRUCT_CONFIG_MODE_MEMORY;

typedef union _UNION_HOST_COMM_MEMORY
{
    STRUCT_NORMAL_MODE_MEMORY NormalModeMeory;    
    STRUCT_FACTORY_MODE_MEMORY FactoryModeMeory;    
    STRUCT_CONFIG_MODE_MEMORY ConfigModeMeory; 
}PACKED UNION_HOST_COMM_MEMORY;

typedef struct _STRUCT_DEBUG_INTERFACE //0ffset : 0xe0~0xff,length 32
{
    U16 u16SramAddr;
    U8  u8SramValue;
    U16 u16FlashCodeAddr;
    U8  u8FlashCodeValue;
    U16 u16FlashInfoAddr;
    U8  u8FlashInfoValue;
    U8  u8SpaceHolder[22];
    U8  u8LogGate;
}PACKED STURCT_DEBUG_INTERFACE;

//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------

extern STURCT_COMMON_HEADER  g_structSystemStatus;   //Mapping 0ffset : 0x00~0x0f ,length 16
extern UNION_HOST_COMM_MEMORY  g_unionMemoryBody;    //Mapping 0ffset : 0x10~0xdf,length 208 (Actually length is 208 + 144 = 352)
extern STURCT_DEBUG_INTERFACE  g_structDebugInfo;    //Mapping 0ffset : 0xe0~0xff,length 32
extern U8 g_updatebase;


//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------
void ChipOneMemoryInit(void);
U8P GetChipOneBufferAAddr(void);
U8P GetChipOneBufferBAddr(void);
void WriteChipOneMemory(U8 u8Offset,U8 u8data);
U8 ReadChipOneMemory(U8 u8Offset);

#endif


