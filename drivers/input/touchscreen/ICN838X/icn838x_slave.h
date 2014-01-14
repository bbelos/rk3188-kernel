/*++
 
 Copyright (c) 2012-2022 ChipOne Technology (Beijing) Co., Ltd. All Rights Reserved.
 This PROPRIETARY SOFTWARE is the property of ChipOne Technology (Beijing) Co., Ltd. 
 and may contains trade secrets and/or other confidential information of ChipOne 
 Technology (Beijing) Co., Ltd. This file shall not be disclosed to any third party,
 in whole or in part, without prior written consent of ChipOne.  
 THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. CHIPONE DISCLAIMS ALL EXPRESS OR 
 IMPLIED WARRANTIES.  
 
 File Name:    icn838x_slave.h
 Abstract:
    
 //Data format :
 //For example :Read:   
 //                 MOSI:  |0xF1|Addr[15:8]|Addr[7:0]| xxx | xxx |..........
 //                 MISO:  | xx |    xx    |   xx    |Data0|Data1|......|Datan|    
 //             Write:  
 //                 MOSI:  |0xF0|Addr[15:8]|Addr[7:0]|Data0|Data1|......|Datan|
 //                 MISO:  | xx |    xx    |   xx    | xxx | xxx |..........        

 Author:       Zhimin Tian
 Date :        12 24,2012
 Version:      1.0 for sensor only
 History :
     

 --*/
#ifndef _ICN838X_SLAVE_H_
#define _ICN838X_SLAVE_H_


#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/i2c.h>

#include <linux/slab.h>
#include <asm/uaccess.h>

#include <linux/delay.h>
#include <linux/platform_device.h>

#define UPDATE_CMD_NONE           0x00
#define UPDATE_CMD_WRITE_REG      0x60
#define UPDATE_CMD_READ_REG       0x61

#define PARA_STATE_CMD            0X00
#define PARA_STATE_ADDR           0X01
#define PARA_STATE_DATA           0X02

#define UPDATE_PARA_FORMAT_ERROR  0xFFFFFFFF
#define UPDATE_PARA_OUT_RANGE     0xFFFFFFFE
#define UPDATE_PARA_UNDEFINE_CMD  0xFFFFFFFD
#define UPDATE_PARA_OK            0x00000000

//#define _SLAVE_DEBUG
#ifdef _SLAVE_DEBUG
#define slave_printk(x...)    printk(x)
#else
#define slave_printk(x...)
#endif

void RwFuncInit();
void icn83xx_memdump(char *mem, int size);
unsigned char  ReadRegU8(char chip, unsigned short RegAddr, unsigned char *Value);
int WriteRegU8(char chip, unsigned short RegAddr, unsigned char Value);
unsigned short ReadRegU16(char chip, unsigned short RegAddr, unsigned short *Value);
int WriteRegU16(char chip, unsigned short RegAddr, unsigned short Value);
int ReadMem(char chip, unsigned short StartAddr, unsigned char *Buffer, unsigned short Length);
int WriteMem(char chip, unsigned short StartAddr, unsigned char *Buffer, unsigned short Length);
unsigned char  ReadDumpU8(char chip, unsigned short RegAddr, unsigned char *Value);

int TP_SetPanelPara(void);

typedef struct _STRUCT_RWFUNC
{
    unsigned char  (*ReadRegU8)(char chip, unsigned short RegAddr, unsigned char *Value);
    int (*WriteRegU8)(char chip, unsigned short RegAddr, unsigned char Value);
    unsigned short (*ReadRegU16)(char chip, unsigned short RegAddr, unsigned short *Value);
    int (*WriteRegU16)(char chip, unsigned short RegAddr, unsigned short Value);
    int (*ReadMem)(char chip, unsigned short StartAddr, unsigned char *Buffer, unsigned short Length);
    int (*WriteMem)(char chip, unsigned short StartAddr, unsigned char *Buffer, unsigned short Length);
    unsigned char  (*ReadDumpU8)(char chip, unsigned short RegAddr, unsigned char *Value);

}STRUCT_RWFUNC;



#endif

