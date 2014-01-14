/*++
 
 Copyright (c) 2012-2022 ChipOne Technology (Beijing) Co., Ltd. All Rights Reserved.
 This PROPRIETARY SOFTWARE is the property of ChipOne Technology (Beijing) Co., Ltd. 
 and may contains trade secrets and/or other confidential information of ChipOne 
 Technology (Beijing) Co., Ltd. This file shall not be disclosed to any third party,
 in whole or in part, without prior written consent of ChipOne.  
 THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. CHIPONE DISCLAIMS ALL EXPRESS OR 
 IMPLIED WARRANTIES.  
 
 File Name:    TouchCommon.h
 Abstract:
     Flash.
 Author:       JunFu Zhang
               Zhimin Tian
 Date :        12 24,2012
 Version:      1.0 for sensor only
 History :
     03 28,2012,   0.1[first revision]
    

 --*/


#ifndef _TOUCHCOMMON_H_
#define _TOUCHCOMMON_H_

#include "constant.h"

//=================================================================================
//================================TP Config Start =======================================
//=================================================================================

/*
#define TP_TX_ORDER  {TX01,TX02,TX03,TX04,TX05,TX06,TX07,TX08,TX09,TX10,TX11,TX12,TX13,TX14,TX15,TX16,TX17,TX18, \
                        TX19,TX20,TX21,TX22,TX23,TX24,TX25,TX26,TX27,TX28}//chip TX orde
                        
#define TP_RX_ORDER  {RX01,RX02,RX03,RX04,RX05,RX06,RX07,RX08,RX09,RX10,RX11,RX12,RX13,RX14,RX15,RX16}//chip RX order

#define TP_TX_ORDER  {TX28,TX27,TX26,TX25,TX24,TX23,TX22,TX21,TX20,TX19,TX18,TX17,TX16,TX15,TX14,TX13,TX12,TX11, \
                        TX10,TX09,TX08,TX07,TX06,TX05,TX04,TX03,TX02,TX01}//chip TX orde
                        
#define TP_RX_ORDER  {RX16,RX15,RX14,RX13,RX12,RX11,RX10,RX09,RX08,RX07,RX06,RX05,RX04,RX03,RX02,RX01}//chip RX order

*/
#ifdef DOUBLE_DIE_SUPPORT

#define NUM_ROW_USED                              10
#define NUM_COL_USED                              6
#define TP_TX_ORDER  {TX07,TX08,TX09,TX10,TX11,TX12,TX13,TX14,TX15,TX16}//chip TX orde
#define TP_RX_ORDER  {RX12,RX11,RX10,RX09,RX08,RX07}//chip RX order


#define NUM_ROW_USED1                              5
#define NUM_COL_USED1                              4
#define TP_TX_ORDER1  {TX17,TX18,TX19,TX20,TX21}//chip TX orde
#define TP_RX_ORDER1  {RX06,RX05,RX04,RX03}//chip RX order


#else


#define NUM_ROW_USED                              19
#define NUM_COL_USED                              11
#define TP_TX_ORDER  {TX02,TX03,TX04,TX05,TX06,TX07,TX08,TX09,TX10,TX11,TX12,TX13,TX14,TX15,TX16,TX17,TX18,TX19,TX20,}
#define TP_RX_ORDER  {RX11,RX10,RX09,RX08,RX07,RX06,RX05,RX04,RX03,RX02,RX01}//chip RX order


//#define NUM_ROW_USED                              15
//#define NUM_COL_USED                              10
//#define TP_TX_ORDER  {TX21,TX20,TX14,TX13,TX12,TX11,TX09,TX08,TX07,TX06,TX05,TX04,TX03,TX02,TX01}//chip TX orde
//#define TP_RX_ORDER  {RX01,RX02,RX03,RX04,RX05,RX06,RX07,RX08,RX09,RX10}

#define NUM_ROW_USED1                              15
#define NUM_COL_USED1                              10
#define TP_TX_ORDER1  {TX21,TX20,TX19,TX18,TX17,TX16,TX15,TX14,TX13,TX12,TX11,TX10,TX09,TX08,TX07}//chip TX orde
#define TP_RX_ORDER1  {RX12,RX11,RX10,RX09,RX08,RX07,RX06,RX05,RX04,RX03}//chip RX order

#endif


#if (NUM_ROW_USED >  PHYSICAL_MAX_NUM_ROW)
    #error"please check NUM_ROW_USED which is bigger than PHYSICAL_MAX_NUM_ROW"
#endif

#if (NUM_COL_USED >  PHYSICAL_MAX_NUM_COL)
    #error"please check NUM_COL_USED which is bigger than PHYSICAL_MAX_NUM_COL"
#endif

//TP Config 2: Voltage setting
#define DEFAULT_TX_VOL                            TX_VOL_786

//TP Config 3: Max surpport touch num setting
#define MAX_SURPPORT_TOUCH_NUM                    5

//TP Config4: Virtual Key setting
#define NUM_VK_USED                               3
#define VK_MODE                                   VK_MODE_1TX_NRX_REUSE
#define TP_VK_ORDER                               {RX10,RX06,RX02}
//#define TP_VK_ORDER                                                   {TX28,TX24,TX19,RX14}
#define VK_DOWN_THRESHOLD                         350
#define VK_UP_THRESHOLD                           200

#define NUM_VK_USED1                              0
#define TP_VK_ORDER1                              {RX12,RX11,RX10}

#define DEFAULT_POSITIVE_PEAK_THRESHOLD           150
#define DEFAULT_OUT_GROUP_THRESHOLD               100

#define DEFAULT_TOUCH_DOWN_THRESHOLD              300
#define DEFAULT_TOUCH_UP_THRESHOLD                250

#define DEFAULT_TOUCH_CHARGER_THRESHOLD           500

#define DEFAULT_SCAN_WAIT_CNT                     500

//TP Config6: Scan mode setting
#define DEFAULT_SCAN_MODE                         SCAN_MODE_1F_ONCE
#define DEFAULT_HD_AVERAGE_MODE                   HD_AVERAGE_MODE_OFF//HD_AVERAGE_MODE_OFF
#define DEFAULT_SCAN_F1                           SCAN_FREQUENCE_93K_OSR128//SCAN_FREQUENCE_234K_OSR128 

#define DEFAULT_FILTER_BANDWIDTH                  BANDWIDTH_02
#define DEFAULT_DE_MODULATION_WINDOW              WINDOW_RECTANGULAR

//TP Config7: Resolution setting
#define RESOLUTION_X                              540    //x,col
#define RESOLUTION_Y                              960   //y,row

#define DEFAULT_X_Y_SWAP                          FALSE
#define DEFAULT_SCALE                             TRUE

//TP Config8: Analog Circuit  setting
#define DEFAULT_RX_OFFSET_AUTO_ADJ                FALSE
#define DEFAULT_PHASE_DELAY_AUTO_ADJ              FALSE
#define DEFAULT_FB_CAP_AUTO_ADJ                   FALSE

#define ADC_DEFAULT_GAIN                          0x7f
//#define ADC_DEFAULT_BW_FL                         0x02
//#define DE_MODULATION_WINDOW                      WINDOW_RECTANGULAR


//should be logical order
//#define TP_RX_FB_CAP    {0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30} 
//#define TP_RX_FB_CAP    {0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28} 
//#define TP_RX_FB_CAP    {0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20} 
//#define TP_RX_FB_CAP    {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}
//#define TP_RX_FB_CAP    {0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14}
//#define TP_RX_FB_CAP    {0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12} 
#define TP_RX_FB_CAP    {0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10} 
//#define TP_RX_FB_CAP    {0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08} 

//should be logical order
//#define TP_RX_DC_OFFSET {0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03} 
//#define TP_RX_DC_OFFSET {0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05} 
#define TP_RX_DC_OFFSET {0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06} 
//#define TP_RX_DC_OFFSET {0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07,0x07}
//#define TP_RX_DC_OFFSET {0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08}
//#define TP_RX_DC_OFFSET {0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09} 
//#define TP_RX_DC_OFFSET {0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a} 
//#define TP_RX_DC_OFFSET {0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b} 
//#define TP_RX_DC_OFFSET {0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c} 
//#define TP_RX_DC_OFFSET {0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d} 
//#define TP_RX_DC_OFFSET {0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e} 

//should be logical order
#define TP_RX_PHASE_DELAY {0x07,0x00,0x03,0x05,0x05,0x05,0x04,0x02,0x06,0x02,0x05,0x00} 

//should be logical order
#define TP_TX_FB_CAP       {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} 
#define TP_TX_DC_OFFSET    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} 
#define TP_TX_PHASE_DELAY  {0x00,0x00,0x01,0x01,0x01,0x02,0x02,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x02,0x02,0x04,0x00,0x00,\
                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} 
/*
#define TP_TX_PHASE_DELAY  {0x12,0x12,0x10,0x10,0x10,0x08,0x08,0x08,0x06,0x06,0x06,0x04,0x04,0x04,0x04,0x00,0x00,0x00,0x00,0x00,0x00,\
                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} 
*/

//if support two chips, should config the define below
//should be logical order
#define TP_RX_FB_CAP1      {0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10} 
#define TP_RX_DC_OFFSET1   {0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06} 
#define TP_RX_PHASE_DELAY1 {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} 

#define TP_TX_FB_CAP1       {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
                             0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} 
#define TP_TX_DC_OFFSET1    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
                             0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} 
#define TP_TX_PHASE_DELAY1  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
                             0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} 

#define ADC_DEFAULT_TARGET                        -7000
#define ADC_DEFAULT_RAWDATA_BANDWIDTH             500
#define DEFAULT_REF_TX_INDEX                      2        //start from 1,0xff: average mode

//TP Config9: Num Filter setting
#define NUM_FILTER_DEFAULT_DEC_CNT                0x01
#define NUM_FILTER_DEFAULT_INC_CNT                0x02 

//TP Config10: Distance Filter setting
#define DISTANCE_FILTER_DEFAULT_STATE             ENABLED
#define DISTANCE_FILTER_DEFAULT_THRESHOLD         1024

//TP Config11: Grid Filter setting
#define GRID_FILTER_DEFAULT_STATE                 ENABLED
#define GRID_FILTER_DEFAULT_CNT_MAX               20
#define GRID_FILTER_SINGLE_DEFAULT_SIZE           32
#define GRID_FILTER_MULTI_DEFAULT_SIZE            128           

//TP Config12:Palm rejection setting
#define DEFAULT_PALM_REJECTION                    DISABLED
#define PALM_DETECTION_THRESHOLD                  15
#define DEFAULT_PALM_THRESHOLD                    200

//TP Config13:并指区分强度
#define WATER_SHED_PERSENT                        70

//TP Config14:Auto Calib setting
#define DEFAULT_AUTO_CALIB_MODE                   AUTO_CALIB_MODE_CHIPONE  //AUTO_CALIB_MODE_APPLE

#define DEFAULT_TEMPERATURE_DF_TH                 70
#define DEFAULT_TEMPERATURE_DEBUNCE_INTERVAL      20

#define DEFAULT_NAGETIVE_THRESHOLD                100
#define DEFAULT_BASELINE_ERROR_MAX_WAIT_CNT       500

#define DEFAULT_WAIT_TOUCH_LEAVE_CNT              50

#define DEFAULT_ENTER_MONITOR_CNT                 10

#define DEFAULT_WATER_DETECT                      FALSE
#define DEFAULT_AUTO_CALIB_STEP                   6
//TP Config15:TP Interface  setting
#define DEFAULT_INT_MODE                          INT_MODE_LOW
#define DEFAULT_WAKE_UP_POL                       WAKE_UP_LOW_LEVEL
#define DEFAULT_REPORT_RATE                       80

#define DEFAULT_GPIO_VOLTAGE                      GPIO_VOLTAGE_33V

 
//TP Config16:Border involving
#define DEFAULT_BORDER_COMP_MODE                  BORDER_COMP_ONE_SENSOR//Border compensation mode 

#define DEFAULT_BORDER_MIN_PRESSURE               450
#define DEFAULT_BORDER_MAX_PRESSURE               700


#define DEFAULT_X_BORDER_PROACH_STREGTH           4       //2~7, x border proaching strength                            
#define DEFAULT_Y_BORDER_PROACH_STREGTH           4       //2~7, y border proaching strength


#define DEFAULT_X_BORDER_PROACH_ACCELERATE        FALSE  //TRUE    //border x proaching accelerating
#define DEFAULT_Y_BORDER_PROACH_ACCELERATE        FALSE  //TRUE    //border y proaching accelerating

#define DEFAULT_BORDER_TRIM_LENGTH                2 //0~7 : Delete some pixels , when blind zone is narrow;8~15: Add some pixels , when blind zone is wide. 

//TP Config17:Flying line setting
#define DEFAULT_MAX_MOVE_DISTANCE                 1000   //unti Flying line when two finger switching

//TP Config18:Float Palm 
//#define CALIB_WHEN_RESUME                         
#define DEFAULT_POWER_ON_FLOAT_PALM_MODE          POWER_ON_FLOAT_PALM_MODE_NAG//unti floating palm when power on or resume
#define DEFAULT_POWER_ON_FLOAT_PALM_TH            1//  2

//TP Config19:ESD 
#define DEFAULT_ESD_CHECK_EN                      FALSE
#define DEFAULT_ESD_CHECK_RX_NUM                  (NUM_COL_USED - 4)  
#define DEFAULT_ESD_CHECK_THRESHOLD               100
#define DEFAULT_ESD_FILTER_FRAME_CNT              10

//TP Config20:Correction of distortion effect 
#define DEFAULT_DISTORTION_EN                      FALSE
#define DEFAULT_DISTORTION_STRENGTH                1
#define DEFAULT_TRANS_MODE                        RAW_MODE
#define RCU_DEFAULT_LIMIT_UP                      100
#define RCU_DEFAULT_LIMIT_DN                      -100

#define DEFAULT_UART_OUTPUT                       OUTPUT_NOTHING

#if (DEFAULT_TRANS_MODE  ==  RCU_MODE)
    #ifdef DOUBLE_DIE_SUPPORT
    #error"Double die do not support RCU mode!!!!!!!!!!!!!!!!"
    #endif
#endif


//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------
extern STRUCT_PANEL_PARA  g_structPanelPara;

//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------
void PanelParaInit(void);

#endif
