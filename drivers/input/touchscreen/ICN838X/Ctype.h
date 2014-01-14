/*++

THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,
WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. VIA TECHNOLOGIES, INC. 
DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES.  

File Name:
              Ctype.h

Abstract:
              Types definition

Author:       JunFu Zhang
              Zhimin Tian
Date :        12 24,2012
Version:      1.0 for sensor only
History :
    03 28,2012,   0.1[first revision]
   

*/

#ifndef _CTYPE_H_
#define _CTYPE_H_
#include "Config.h"

typedef unsigned char   BIT;
typedef unsigned char   U8;
typedef unsigned short  U16;
typedef unsigned int    U32;
typedef U8 BOOL;

typedef unsigned char  * U8P;
typedef unsigned char  * U8XP;
typedef unsigned short * U16XP;
typedef unsigned int   * U32XP;

typedef unsigned char  * U8CP;
typedef unsigned short * U16CP;
typedef unsigned int   * U32CP;


typedef signed char S8;
typedef signed short S16;
typedef signed int S32;

typedef char  * S8P;
typedef short * S16P;
typedef int   * S32P;


#define PACKED __attribute__((packed, aligned(1)))


typedef union UU16
{
   U16 U16;
   S16 S16;
   U8 U8[2];
   S8 S8[2];
} UU16;

typedef struct _STRUCT_SCAN_MODE
{
    U8 ScanMode:4;
    U8 AutoDetct:1;
    U8 Reserved:3;
}STRUCT_SCAN_MODE;

//-----------------------------------------------------------------------------
// Struct, Union and Enum Definitions
//-----------------------------------------------------------------------------
//This para saving in flash (A or B)
typedef struct _STRUCT_PANEL_PARA
{
    
    U8   u8RowNum;       // Row total number (Tp + vk)
    U8   u8ColNum;       // Column total number (Tp + vk)
    
    U8   u8RowNum1;       // Row total number (Tp + vk)
    U8   u8ColNum1;       // Column total number (Tp + vk)

    
    U8   u8TxVol;        //Diver voltage of chip
    U8   u8MaxTouchNum;  //max touch  support

    U8   u8NumVKey;
    U8   u8NumVKey1;
    
    U8   u8VKeyMode;
    U8   u8TpVkOrder[PHYSICAL_MAX_VK_NUM];
    U8   u8TpVkOrder1[PHYSICAL_MAX_VK_NUM];
    S16  s16VKDownThreshold;
    S16  s16VKUpThreshold;

    S16  u16PositivePeakTh;
    S16  u16OutGroupThreshold;
    S16  u16AreaThreshold;

    S16  s16TouchDownThresold;
    S16  s16TouchUpThresold;

    STRUCT_SCAN_MODE   structScanMode;
    U8   u8HwAverageMode;
    U8   u8FrqDiv[3];

    U16  u16ResX;        //Row of resolution
    U16  u16ResY;        //Col of resolution

    U8   u8TXOrder[PHYSICAL_MAX_NUM_ROW];           //TX Order, start from zero
    U8   u8RXOrder[PHYSICAL_MAX_NUM_COL];           //TX Order, start from zero
    U8   u8TXOrder1[PHYSICAL_MAX_NUM_ROW];          //TX Order, start from zero
    U8   u8RXOrder1[PHYSICAL_MAX_NUM_COL];          //TX Order, start from zero

    U8   u8AdcGain;                                 //Control the differ value for touching

    U8   u8ColFbCap[PHYSICAL_MAX_NUM_COL];          //Charge Amplifier feedback Capacitance of columns
    U8   u8ColDcOffset[PHYSICAL_MAX_NUM_COL];     //DC offset
    U8   u8ColPhaseDelay[PHYSICAL_MAX_NUM_COL];     //Phase delay

    U8   u8ColFbCap1[PHYSICAL_MAX_NUM_COL];         //Charge Amplifier feedback Capacitance of columns
    U8   u8ColDcOffset1[PHYSICAL_MAX_NUM_COL];    //DC offset
    U8   u8ColPhaseDelay1[PHYSICAL_MAX_NUM_COL];    //Phase delay


    U8   u8RowFbCap[PHYSICAL_MAX_NUM_ROW*2];          //Charge Amplifier feedback Capacitance of columns
    U8   u8RowDcOffset[PHYSICAL_MAX_NUM_ROW*2];       //DC offset
    U8   u8RowPhaseDelay[PHYSICAL_MAX_NUM_ROW*2];     //Phase delay

    U8   u8RowFbCap1[PHYSICAL_MAX_NUM_ROW*2];         //Charge Amplifier feedback Capacitance of columns
    U8   u8RowDcOffset1[PHYSICAL_MAX_NUM_ROW*2];      //DC offset
    U8   u8RowPhaseDelay1[PHYSICAL_MAX_NUM_ROW*2];    //Phase delay

    
    S16  s16AdcTarget;
    U16  u16RawDataAdjBandWidth;

    U8   u8NumFilterDecCnt;
    U8   u8NumFilterIncCnt;

    U8   u8DistanceFilterEnable;
    U16  u16DistanceFilterThreshold;
    
    U8   u8GridFilterEnable;
    U8   u8GridFilterCntMax;
    U8   u8GridFilterSize;
    U8   u8GridFilterMultiSize;
    
    U32  u32AutoDetectBkNoiseTh;
    U8   u8PalmDetectionThreshold;
    U8   u8WaterShedPersent;    //0 -100

    U8   u8XySwap;
    U8   u8Scale;

    U8   u8IntMode;

    U8   u8TemperatureDriftThreshold;
    U8   u8TemperatureDebunceInterval;

    S16  u16NagetiveThreshold;
    U16  u16BaseLineErrorMaxWaitCnt;

    U8   u8WaitTouchLeaveCnt;

    U8   u8WakeUpPol;
    U8   u8EnterMonitorCnt;
    
    U8   u8AutoAdjRxOffsetEn;
    U8   u8ReportRate;
    U8   u8UpdateRawNormal;
    U8   u8GpioVol;
    U8   u8AutoAdjPhaseDelay;
    U8   u8PalmRejectionEn;
    U8   u8BorderProachStregth;
    U8   u8WaterDetectEn;
    U8   u8RefTxIndex;
    U8   u8AutoAdjFbCapEn;

    U8   u8BorderCompMode;

    U16  u16MaxMoveDistance;
    U8   u8PowerOnFloatPalmMode;
    U8   u8FloatPalmThreshold;
    U8   u8WindowTypeBandWidth;
    U8   u8AutoCalibMode;
    U8   u8AutoCalibStep;
    U16  u16BorderMinPress;
    U16  u16BorderMaxPress;
    
    U8   u8TransMode;
    S16  s16RCULimitUp;
    S16  s16RCULimitDn;

    U8   u8UartOut;
    
    U8   u8EsdCheckEnable;
    U8   u8EsdCheckRxNum;
    U16  u16EsdCheckThreshold;
    U8   u8EsdFilterFrameCnt;
    S16  s16TouchChargerThresold;

    S16  s16ScanWaitCnt;
    
    U8   u8DistortionFilterEn;
    U8   u8DistortionStrength;

    U8   u8SpaceHolder[48];

}PACKED STRUCT_PANEL_PARA;

typedef struct _STRUCT_OUTPUT_POINT_INFO
{
    U16 u16PosX;        // coordinate X, plus 4 LSBs for precision extension
    U16 u16PosY;        // coordinate Y, plus 4 LSBs for precision extension
    U8 u8Pressure;
    U8 u8EventId;
}PACKED STRUCT_OUTPUT_POINT_INFO;

typedef struct _STRUCT_FRAME_INFO
{
    U8 u8CurNum;    //number of points in current frame
    U8 u8LastNum;   //number of points in last frame
    U8 u8Gesture;
    U8 u8LastGesture;
    STRUCT_OUTPUT_POINT_INFO PointInfo[PHYSICAL_MAX_TOUCH_NUM];
}PACKED STRUCT_FRAME_INFO;


//-----------------------------------------------------------------

#endif  //end of _CONSTANT_H_

