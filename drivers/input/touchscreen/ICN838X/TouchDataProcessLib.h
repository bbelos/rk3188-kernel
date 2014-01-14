/*++
 
 Copyright (c) 2012-2022 ChipOne Technology (Beijing) Co., Ltd. All Rights Reserved.
 This PROPRIETARY SOFTWARE is the property of ChipOne Technology (Beijing) Co., Ltd. 
 and may contains trade secrets and/or other confidential information of ChipOne 
 Technology (Beijing) Co., Ltd. This file shall not be disclosed to any third party,
 in whole or in part, without prior written consent of ChipOne.  
 THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. CHIPONE DISCLAIMS ALL EXPRESS OR 
 IMPLIED WARRANTIES.  
 
 File Name:    data process.c
 Abstract:
    All buf action invisble for user
    
 Author:       JunFu Zhang
               Zhimin Tian
 Date :        12 24,2012
 Version:      1.0 for sensor only
 History :
     03 28,2012,   0.1[first revision]
     
 --*/


#ifndef _TOUCHDATAPROCESSLIB_H_
#define _TOUCHDATAPROCESSLIB_H_

//#include <intrins.h>
#include "Config.h"


//Lib setting 1: please enable this macro when ALU is ready.
//#define ALU_ENABLE    
//Lib setting 2: Auto detect mode select
//#define AUTO_DETECT_MODE_PARALL

//Lib setting 3: Noise detect times
#define NOISE_DECTECTION_TIMES    0xa
//HFSS




//-----------------------------------------------------------------------------
// Pin Declarations
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Global CONSTANTS
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Macro DEFINITIONS
//-----------------------------------------------------------------------------

#define  BUF_BLOCK_RAW_ADDR                  0x1000
#define  BUF_BLOCK_BAS_ADDR                  0x2000
#define  BUF_BLOCK_DIF_ADDR                  0x3000
#define  BUF_BLOCK_DIFLMT_ADDR               0x2438



#define  BUF_BLOCK_RAW_ALU_ADDR              0x1026
#define  BUF_BLOCK_BAS_ALU_ADDR              0x2026
#define  BUF_BLOCK_DIF_ALU_ADDR              0x3026
#define  BUF_BLOCK_DIFLMT_ALU_ADDR           0x245E

#define  RAWDATA_BLOCK_0_ADDR                0x4000
#define  RAWDATA_BLOCK_1_ADDR                0x5000
#define  RAWDATA_BLOCK_2_ADDR                0x6000
#define  RAWDATA_BLOCK_3_ADDR                0x7000
#define  RAWDATA_BLOCK_4_ADDR                0x8000
#define  RAWDATA_BLOCK_5_ADDR                0x9000


#define  BUF_BLOCK_A_INDEX                   0x1
#define  BUF_BLOCK_B_INDEX                   0x2
#define  BUF_BLOCK_C_INDEX                   0x3

//#define  RAWDATA_BLOCK_0_INDEX               0x4
//#define  RAWDATA_BLOCK_1_INDEX               0x5
//#define  RAWDATA_BLOCK_2_INDEX               0x6
//#define  RAWDATA_BLOCK_3_INDEX               0x7
//#define  RAWDATA_BLOCK_4_INDEX               0x8
//#define  RAWDATA_BLOCK_5_INDEX               0x9
#define  RAWDATA_BLOCK_0_INDEX               0x0
#define  RAWDATA_BLOCK_1_INDEX               0x1

#define  RCU_AB_MODE                         BIT1

#define  BlockIndexToBlockAddr(BlockIndex)   (BlockIndex << 12)
#define  BlockAddrToBlockIndex(BlockAddr)    (BlockAddr >> 12)

#define BUF_PHASE_A                           0x01
#define BUF_PHASE_B                           0x02

#define INVALID_POINT_ID                      0xff

//ALU op code
#define ALU_OPCODE_NOP                        0x00
#define ALU_OPCODE_HP                         0x01
#define ALU_OPCODE_MV                         0x02
#define ALU_OPCODE_SUB                        0x03
#define ALU_OPCODE_CMV                        0x04
#define ALU_OPCODE_USUB                       0x05
#define ALU_OPCODE_MEAN                       0x06
#define ALU_OPCODE_MED                        0x07
#define ALU_OPCODE_MUL                        0x08
#define ALU_OPCODE_DIV                        0x09
#define ALU_OPCODE_VMV                        0x0a
#define ALU_OPCODE_VSUB                       0x0b
#define ALU_OPCODE_VCMV                       0x0c
#define ALU_OPCODE_VUSUB                      0x0d
#define ALU_OPCODE_SUM                        0x0E
#define ALU_OPCODE_AVG2                       0x0F
#define ALU_OPCODE_LMT                        0x10
#define ALU_OPCODE_ACC                        0x11
#define ALU_OPCODE_W0                         0x12
#define ALU_OPCODE_WF                         0x13
#define ALU_OPCODE_WR                         0x14
#define ALU_OPCODE_PACC                       0x21
#define ALU_OPCODE_NACC                       0x31


#define ALU_START_ALU_EN                      BIT4
#define ALU_START_SEARCH_EN                   BIT3
#define ALU_START_PEAK_START                  BIT2
#define ALU_START_GRP_START                   BIT1
#define ALU_START_ALU_START                   BIT0

#define SRCH_CFG_UP_WAIT_EN                   BIT7
#define ALU_CFG_UP_WAIT_EN                    BIT5
#define ALU_CFG_HP_BUSY_TIMER_EN              BIT3
#define ALU_CFG_ALU_BUSY_TIMER_EN             BIT2
#define ALU_CFG_LIMIT_DN_EN                   BIT1
#define ALU_CFG_LIMIT_UP_EN                   BIT0



#define FREQ_INDEX_FREQ1                      (0x00)
#define FREQ_INDEX_FREQ2                      (FREQ_INDEX_FREQ1 + 1)
#define FREQ_INDEX_FREQ3                      (FREQ_INDEX_FREQ1 + 2)
#define AUTO_DETECT_USED_FREQ_ALL             (FREQ_INDEX_FREQ1 + 0x10)



#define  OVERSAMPLING_RATE_16X                 (00)
#define  OVERSAMPLING_RATE_32X                 (BIT1)
#define  OVERSAMPLING_RATE_64X                 (BIT2)
#define  OVERSAMPLING_RATE_128X                (BIT1|BIT2)
#define INVALID_POINT_POS                       0x7FFF

#define GESTURE_NONE                            0x00
#define GESTURE_VK_KEY1                         BIT0
#define GESTURE_VK_KEY2                         BIT1
#define GESTURE_VK_KEY3                         BIT2
#define GESTURE_VK_KEY4                         BIT3


#define EVENT_ID_NONE                           0
#define EVENT_ID_DOWN                           1
#define EVENT_ID_MOVE                           2
#define EVENT_ID_STAY                           3
#define EVENT_ID_UP                             4


#define TEMPERATURE_DRIFT_STATE_NONE            0
#define TEMPERATURE_DRIFT_STATE_INC             1
#define TEMPERATURE_DRIFT_STATE_DEC             2


#define BASE_STATE_STATBLE                      0
#define BASE_STATE_ERROR_POSSIBLE               1
#define BASE_STATE_ERROR                        2


#define RX_OFFSET_MAX_CNT                       15


//#define  OVERSAMPLING_RATE                      OVERSAMPLING_RATE_128X//OVERSAMPLING_RATE_128X
//#define  DE_MODULATION_WINDOW                   RECTANGULAR_WINDOW

//-----------------------------------------------------------------------------
// Struct, Union and Enum DEFINITIONS
//-----------------------------------------------------------------------------
typedef struct _STURCT_INPUT_PARAMETER
{
    U16 u16DiffAddr;             
    U8  u8MaxTouchNum;
    U8  u8Row;               //Search range
    U8  u8Col;               //Search range
    U8  u8PeakRow;           //group search used as peak start point
    U8  u8PeakCol;           //group search used as peak start point
    U8  u8RerverseFlag;      //ReverseFlag  used when search negative peak
    S16 u16PositivePeakTh;
    S16 u16OutGroupThreshold;
    S16 u16AreaThreshold;
    U16 u16PeakAddr;
    U16 u16GroupInfoAddr;
    U8 u8CalCentroidEnable;
    U8 u8CalCentroidThreshold;
}PACKED STURCT_INPUT_PARAMETER;

typedef struct _STRUCT_GROUP_INFO
{
    U16 u16GroupRowNum;
    U16 u16AreaSize;
    U16 u16MaxPressure;
    U32 u32TotalPressure;
    U8 u8GroupRow[PHYSICAL_MAX_NUM_ROW];
    U8 u8GroupFirstCol[PHYSICAL_MAX_NUM_ROW];
    U8 u8GroupLastCol[PHYSICAL_MAX_NUM_ROW];
    U32 u32RowWeightSum;
    U32 u32ColWeightSum;
}PACKED STRUCT_GROUP_INFO;

typedef struct _STRUCT_PEAK_POS
{
    U8 u8Row;     //row number of row
    U8 u8Col;     //col number of col
}PACKED STRUCT_PEAK_POS;

typedef struct _STRUCT_PEAK
{
    U16 u16PeakNum;
    STRUCT_PEAK_POS PeakPos[PHYSICAL_MAX_TOUCH_NUM<<1];
}PACKED STRUCT_PEAK;

typedef struct _STRUCT_POINT_INFO
{
    U8  u8PosId;            // Touch ID
    U16 u16PosX;        // coordinate X, plus 4 LSBs for precision extension
    U16 u16PosY;        // coordinate Y, plus 4 LSBs for precision extension
    S16 s16TouchWeight;
    //U8  u8BorderAccelFlag;
    //U16 u16TouchArea;
    //U32 u32TotalPressure;
}PACKED STRUCT_POINT_INFO;

typedef struct _STRUCT_TOUCH
{
    U8 u8PointNum;
    STRUCT_POINT_INFO PointInfo[PHYSICAL_MAX_TOUCH_NUM];
}PACKED STRUCT_TOUCH;

typedef struct _STRUCT_SUSPECT_PEAK_INFO
{
    U8              u8Cnt;
    U8              u8Valid; 
    STRUCT_PEAK_POS Pos;
}PACKED STRUCT_SUSPECT_PEAK_INFO;

typedef struct _STRUCT_SUSPECT_PEAK
{
    U8 u8Num;
    STRUCT_SUSPECT_PEAK_INFO PeakSuspectInfo[PHYSICAL_MAX_TOUCH_NUM<<1];
}PACKED STRUCT_SUSPECT_PEAK;

typedef struct _STRUCT_POINT_POS
{
   U16 u16PosX;
   U16 u16PosY;
   S16 s16TouchWeight;
   U8  u8BorderAccelFlag;
   U8  u8Leave;
}PACKED STRUCT_POINT_POS;


typedef struct _STRUCT_TOUCH_STATUS 
{
    STRUCT_PEAK  PosistiveRawPeaks;  // Row, Col  of touch peaks in a frame

    STRUCT_PEAK  PosistiveValidPeaks;  // Row, Col  of touch peaks in a frame
    STRUCT_PEAK  NegativePeaks;   // Row, Col  of touch peaks in a frame

    STRUCT_SUSPECT_PEAK CurSuspectPeak;
    STRUCT_SUSPECT_PEAK LastSuspectPeak;

    STRUCT_TOUCH RawPoints;       //Raw frame points info(may be with some fake points)
    STRUCT_TOUCH CurPoints;       //Current frame points info, compact mode(fixed addr is slower when touch is less)
    STRUCT_TOUCH LastPoints;      //Last frame points info, compact mode(fixed addr is slower when touch is less)
    
    STRUCT_TOUCH FilterPoints;    //Filter Points infor  after filters, compact mode(fixed addr is slower when touch is less)
    STRUCT_POINT_POS RealPoints[PHYSICAL_MAX_TOUCH_NUM];  //fixed mode , record real position of all points
    STRUCT_POINT_POS SingleTouchDownPos;
    
    U8 u8EventID[PHYSICAL_MAX_TOUCH_NUM];
    
    U8 u8Gesture;                 //Getsture and VK
    U8 u8LastGesture;                 //Getsture and VK

    //para used by Num filter
    U8 u8PointTargetNum;          //Target num ,when num changing
    U8 u8PointNumFilterCnt;
    U8 u8PointNumFilterMaxCnt;

    U8 u8GridFilterCnt[PHYSICAL_MAX_TOUCH_NUM];

    U8 u8AllTouchFreeze;

    
    U8 bPalmFlag;   
    U8 u8PalmCnt;
        
    U16 u16MaxPressure;
    U16 u16MaxNagPressure;

    U16 u16NagPresSum;
    U16 u16PosPresSum;

    U8 u8ClipFlag;
    U16 u16SingleMoveDistanceSum;

    U8 u8FilterFrameCnt;

    U8 u8EnterBorder;

    U8 u8ReportLastPosFlag;
    /*
    U8 ucCurrentPointNum;                   // number of touch of current frame 
    U8 ucLastPointNum;                       //

    U8 bUngroundFlag;                       // unground flag handle

    U8 ucPosPeakCnt;
    U8 ucNegPeakCnt;

    U8 ucFrameCnt;
    U8 ucLongStableCnt;

    U8 ucNegFrameCnt;
    U8 ucNegLongStableCnt;

    U8 ucCurrentNegPointNum;*/
    
}PACKED STRUCT_TOUCH_STATUS;


typedef struct _STRUCT_TP_CONFIG
{
    U8 u8TscreenRowNum;
    U8 u8TscreenColNum;
    U8 u8VkRowIndex[PHYSICAL_MAX_VK_NUM];
    U8 u8VkColIndex[PHYSICAL_MAX_VK_NUM];
}PACKED STRUCT_TP_CONFIG;

typedef struct _STRUCT_TEMPERATURE_STATE
{
    U8 u8RowStep;
    U8 u8ColStep;
    U8 u8MaxRow;
    U8 u8MaxCol;
    U8 u8RowStart;
    U8 u8ColStart;
    U8 u8State;
    U8 u8TempIncCounter ;
    U8 u8TempDecCounter ;
    //U32 u32WorstTh;
}PACKED STRUCT_TEMPERATURE_STATE;



typedef struct _STRUCT_BASE_STATE
{
    U8 u8PowerOnFloatPalmCnt;
    U8 u8NagPeakCnt;
    U16 u16IdleCnt;
    U8 u8NeedUpdate;
    U8 u8BaseBufPhase;
}PACKED STRUCT_BASE_STATE;


//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------
void PowerOnRx(void);
void PowerDownRx(void);
void SetTxVol(U8 u8Vol);
void ScanInit(STRUCT_PANEL_PARA * structPanelParaPtr);
void AutoAdjFbCap(void);
void AutoAdjPhaseDelay(void);
void AutoAdjRxOffset(void);
void UpdateBaseLine(void);
void ScanStartWithReload(void);
void ScanStartWithoutReload(void);
void WaitScanCompleteWithReload(void);
void WaitScanCompleteWithoutReload(void);
void RawDataPreProcess(void);
void SetScanMode(STRUCT_SCAN_MODE * structScanModePtr);
U16 * GetCurRawDataRowAddr(U8 u8RowIndex);
void ReadFrameInfo(STRUCT_FRAME_INFO * structFrameInfoPtr);
void TouchDataProcess(void);
void Filters(void);
U8* GetRawdata(void);
U8* GetDiffdata(void);
void AutoWork(void);
void MemoryDump(U16 u16MemoryAddr);
void TouchStatusInit(void);

#endif


