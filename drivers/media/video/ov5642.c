/*
o* Driver for MT9M001 CMOS Image Sensor from Micron
 *
 * Copyright (C) 2008, Guennadi Liakhovetski <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/videodev2.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/log2.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/circ_buf.h>
#include <linux/miscdevice.h>
#include <media/v4l2-common.h>
#include <media/v4l2-chip-ident.h>
#include <media/soc_camera.h>

#define _CONS(a,b) a##b
#define CONS(a,b) _CONS(a,b)

#define __STR(x) #x
#define _STR(x) __STR(x)
#define STR(x) _STR(x)

/* Sensor Driver Configuration */
#define SENSOR_NAME ov5642
#define SENSOR_V4L2_IDENT V4L2_IDENT_OV5642
#define SENSOR_ID 0x5642
#define SENSOR_MIN_WIDTH    176
#define SENSOR_MIN_HEIGHT   144
#define SENSOR_MAX_WIDTH    2592
#define SENSOR_MAX_HEIGHT   1944
#define SENSOR_INIT_WIDTH	640			/* Sensor pixel size for sensor_init_data array */
#define SENSOR_INIT_HEIGHT  480
#define SENSOR_INIT_WINSEQADR sensor_vga
#define SENSOR_INIT_PIXFMT V4L2_PIX_FMT_UYVY

#define CONFIG_SENSOR_WhiteBalance	1
#define CONFIG_SENSOR_Brightness	0
#define CONFIG_SENSOR_Contrast      0
#define CONFIG_SENSOR_Saturation    0
#define CONFIG_SENSOR_Effect        1
#define CONFIG_SENSOR_Scene         1
#define CONFIG_SENSOR_DigitalZoom   0
#define CONFIG_SENSOR_Focus         1
#if CONFIG_SENSOR_Focus
#define CONFIG_SENSOR_AutoFocus     0
#else
#undef CONFIG_SENSOR_AutoFocus
#endif
#define CONFIG_SENSOR_Exposure      0
#define CONFIG_SENSOR_Flash         0
#define CONFIG_SENSOR_Mirror        0
#define CONFIG_SENSOR_Flip          0

#define CONFIG_SENSOR_I2C_SPEED     400000       /* Hz */

#define CONFIG_SENSOR_TR      1
#define CONFIG_SENSOR_DEBUG	  1

#define SENSOR_NAME_STRING(a) STR(CONS(SENSOR_NAME, a))
#define SENSOR_NAME_VARFUN(a) CONS(SENSOR_NAME, a)

#define MIN(x,y)   ((x<y) ? x: y)
#define MAX(x,y)    ((x>y) ? x: y)

#if (CONFIG_SENSOR_TR)
	#define SENSOR_TR(format, ...)      printk(format, ## __VA_ARGS__)
	#if (CONFIG_SENSOR_DEBUG)
	#define SENSOR_DG(format, ...)      printk(format, ## __VA_ARGS__)
	#else
	#define SENSOR_DG(format, ...)
	#endif
#else
	#define SENSOR_TR(format, ...)
#endif

#define SENSOR_BUS_PARAM  (SOCAM_MASTER | SOCAM_PCLK_SAMPLE_RISING |\
                          SOCAM_HSYNC_ACTIVE_HIGH | SOCAM_VSYNC_ACTIVE_LOW |\
                          SOCAM_DATA_ACTIVE_HIGH | SOCAM_DATAWIDTH_8  |SOCAM_MCLK_24MHZ)

#define COLOR_TEMPERATURE_CLOUDY_DN  6500
#define COLOR_TEMPERATURE_CLOUDY_UP    8000
#define COLOR_TEMPERATURE_CLEARDAY_DN  5000
#define COLOR_TEMPERATURE_CLEARDAY_UP    6500
#define COLOR_TEMPERATURE_OFFICE_DN     3500
#define COLOR_TEMPERATURE_OFFICE_UP     5000
#define COLOR_TEMPERATURE_HOME_DN       2500
#define COLOR_TEMPERATURE_HOME_UP       3500

#define SENSOR_AF_IS_ERR    (0x00<<0)
#define SENSOR_AF_IS_OK		(0x01<<0)

#if (CONFIG_SENSOR_Focus == 1)
/* ov5642 VCM Command and Status Registers */
#define CMD_MAIN_Reg      0x3024
#define CMD_TAG_Reg       0x3025
#define CMD_PARA0_Reg     0x5085
#define CMD_PARA1_Reg     0x5084
#define CMD_PARA2_Reg     0x5083
#define CMD_PARA3_Reg     0x5082
#define STA_ZONE_Reg      0x3026
#define STA_FOCUS_Reg     0x3027

/* ov5642 VCM Command  */
#define OverlayEn_Cmd     0x01
#define OverlayDis_Cmd    0x02
#define SingleFocus_Cmd   0x03
#define ConstFocus_Cmd    0x04
#define StepMode_Cmd      0x05
#define ReturnIdle_Cmd    0x08
#define SetZone_Cmd       0x10
#define UpdateZone_Cmd    0x12
#define SetMotor_Cmd      0x20

/* ov5642 Focus State */
#define S_FIRWRE          0xFF
#define S_STARTUP         0xfa
#define S_ERROR           0xfe
#define S_DRVICERR        0xee
#define S_IDLE            0x00
#define S_FOCUSING        0x01
#define S_FOCUSED         0x02
#define S_CAPTURE         0x12
#define S_STEP            0x20

/* ov5642 Zone State */
#define Zone_Is_Focused(a, zone_val)    (zone_val&(1<<(a-3)))
#define Zone_Get_ID(zone_val)           (zone_val&0x03)

#define Zone_CenterMode   0x01
#define Zone_5xMode       0x02
#define Zone_5PlusMode    0x03
#define Zone_4fMode       0x04

#define ZoneSel_Auto      0x0b
#define ZoneSel_SemiAuto  0x0c
#define ZoneSel_Manual    0x0d
#define ZoneSel_Rotate    0x0e

/* ov5642 Step Focus Commands */
#define StepFocus_Near_Tag       0x01
#define StepFocus_Far_Tag        0x02
#define StepFocus_Furthest_Tag   0x03
#define StepFocus_Nearest_Tag    0x04
#define StepFocus_Spec_Tag       0x10
#endif

struct reginfo
{
    u16 reg;
    u8 val;
};

/* init 640X480 SVGA */
static struct reginfo sensor_init_data[] =
{
    {0x3103 , 0x93},
    {0x3008 , 0x82},
    {0x3017 , 0x7f},
    {0x3018 , 0xfc},
    {0x3810 , 0xc2},
    {0x3615 , 0xf0},
    {0x3000 , 0x00},
    {0x3001 , 0x00},
    {0x3002 , 0x00},
    {0x3003 , 0x00},
    {0x3000 , 0xf8},
    {0x3001 , 0x48},
    {0x3002 , 0x5c},
    {0x3003 , 0x02},
    {0x3004 , 0x07},
    {0x3005 , 0xb7},
    {0x3006 , 0x43},
    {0x3007 , 0x37},
    {0x3011 , 0x08},
    {0x3010 , 0x10},
    {0x460c , 0x22},
    {0x3815 , 0x04},
    {0x370d , 0x06},
    {0x370c , 0xa0},
    {0x3602 , 0xfc},
    {0x3612 , 0xff},
    {0x3634 , 0xc0},
    {0x3613 , 0x00},
    {0x3605 , 0x7c},
    {0x3621 , 0x09},
    {0x3622 , 0x00},
    {0x3604 , 0x40},
    {0x3603 , 0xa7},
    {0x3603 , 0x27},
    {0x4000 , 0x21},
    {0x401d , 0x02},
    {0x3600 , 0x54},
    {0x3605 , 0x04},
    {0x3606 , 0x3f},
    {0x3c01 , 0x80},
    {0x5000 , 0x4f},
    {0x5020 , 0x04},
    {0x5181 , 0x79},
    {0x5182 , 0x00},
    {0x5185 , 0x22},
    {0x5197 , 0x01},
    {0x5001 , 0xff},
    {0x5500 , 0x0a},
    {0x5504 , 0x00},
    {0x5505 , 0x7f},
    {0x5080 , 0x08},
    {0x300e , 0x18},
    {0x4610 , 0x00},
    {0x471d , 0x05},
    {0x4708 , 0x06},
    {0x3710 , 0x10},
    {0x3632 , 0x41},
    {0x3702 , 0x40},
    {0x3620 , 0x37},
    {0x3631 , 0x01},
    {0x3808 , 0x02},
    {0x3809 , 0x80},
    {0x380a , 0x01},
    {0x380b , 0xe0},
    {0x380e , 0x07},
    {0x380f , 0xd0},
    {0x501f , 0x00},
    {0x5000 , 0x4f},
    {0x4300 , 0x32},   //UYVY
    {0x3503 , 0x07},
    {0x3501 , 0x73},
    {0x3502 , 0x80},
    {0x350b , 0x00},
    {0x3503 , 0x07},
    {0x3824 , 0x11},
    {0x3501 , 0x1e},
    {0x3502 , 0x80},
    {0x350b , 0x7f},
    {0x380c , 0x0c},
    {0x380d , 0x80},
    {0x380e , 0x03},
    {0x380f , 0xe8},
    {0x3a0d , 0x04},
    {0x3a0e , 0x03},
    {0x3818 , 0xc1},
    {0x3705 , 0xdb},
    {0x370a , 0x81},
    {0x3801 , 0x80},
    {0x3621 , 0xc7},
    {0x3801 , 0x50},
    {0x3803 , 0x08},
    {0x3827 , 0x08},
    {0x3810 , 0xc0},
    {0x3804 , 0x05},
    {0x3805 , 0x00},
    {0x5682 , 0x05},
    {0x5683 , 0x00},
    {0x3806 , 0x03},
    {0x3807 , 0xc0},
    {0x5686 , 0x03},
    {0x5687 , 0xc0},
    {0x3a00 , 0x78},
    {0x3a1a , 0x04},
    {0x3a13 , 0x30},
    {0x3a18 , 0x00},
    {0x3a19 , 0x7c},
    {0x3a08 , 0x12},
    {0x3a09 , 0xc0},
    {0x3a0a , 0x0f},
    {0x3a0b , 0xa0},
    {0x3004 , 0xff},
    {0x350c , 0x07},
    {0x350d , 0xd0},
    {0x3500 , 0x00},
    {0x3501 , 0x00},
    {0x3502 , 0x00},
    {0x350a , 0x00},
    {0x350b , 0x00},
    {0x3503 , 0x00},
    {0x528a , 0x02},
    {0x528b , 0x04},
    {0x528c , 0x08},
    {0x528d , 0x08},
    {0x528e , 0x08},
    {0x528f , 0x10},
    {0x5290 , 0x10},
    {0x5292 , 0x00},
    {0x5293 , 0x02},
    {0x5294 , 0x00},
    {0x5295 , 0x02},
    {0x5296 , 0x00},
    {0x5297 , 0x02},
    {0x5298 , 0x00},
    {0x5299 , 0x02},
    {0x529a , 0x00},
    {0x529b , 0x02},
    {0x529c , 0x00},
    {0x529d , 0x02},
    {0x529e , 0x00},
    {0x529f , 0x02},
    {0x3a0f , 0x3c},
    {0x3a10 , 0x30},
    {0x3a1b , 0x3c},
    {0x3a1e , 0x30},
    {0x3a11 , 0x70},
    {0x3a1f , 0x10},
    {0x3030 , 0x0b},
    {0x3a02 , 0x00},
    {0x3a03 , 0x7d},
    {0x3a04 , 0x00},
    {0x3a14 , 0x00},
    {0x3a15 , 0x7d},
    {0x3a16 , 0x00},
    {0x3a00 , 0x78},
    {0x3a08 , 0x09},
    {0x3a09 , 0x60},
    {0x3a0a , 0x07},
    {0x3a0b , 0xd0},
    {0x3a0d , 0x08},
    {0x3a0e , 0x06},
    {0x5193 , 0x70},
    {0x3620 , 0x57},
    {0x3703 , 0x98},
    {0x3704 , 0x1c},
    {0x589b , 0x04},
    {0x589a , 0xc5},
    {0x528a , 0x00},
    {0x528b , 0x02},
    {0x528c , 0x08},
    {0x528d , 0x10},
    {0x528e , 0x20},
    {0x528f , 0x28},
    {0x5290 , 0x30},
    {0x5292 , 0x00},
    {0x5293 , 0x00},
    {0x5294 , 0x00},
    {0x5295 , 0x02},
    {0x5296 , 0x00},
    {0x5297 , 0x08},
    {0x5298 , 0x00},
    {0x5299 , 0x10},
    {0x529a , 0x00},
    {0x529b , 0x20},
    {0x529c , 0x00},
    {0x529d , 0x28},
    {0x529e , 0x00},
    {0x529f , 0x30},
    {0x5282 , 0x00},
    {0x5300 , 0x00},
    {0x5301 , 0x20},
    {0x5302 , 0x00},
    {0x5303 , 0x7c},
    {0x530c , 0x00},
    {0x530d , 0x0c},
    {0x530e , 0x20},
    {0x530f , 0x80},
    {0x5310 , 0x20},
    {0x5311 , 0x80},
    {0x5308 , 0x20},
    {0x5309 , 0x40},
    {0x5304 , 0x00},
    {0x5305 , 0x30},
    {0x5306 , 0x00},
    {0x5307 , 0x80},
    {0x5314 , 0x08},
    {0x5315 , 0x20},
    {0x5319 , 0x30},
    {0x5316 , 0x10},
    {0x5317 , 0x08},
    {0x5318 , 0x02},
    {0x5380 , 0x01},
    {0x5381 , 0x00},
    {0x5382 , 0x00},
    {0x5383 , 0x4e},
    {0x5384 , 0x00},
    {0x5385 , 0x0f},
    {0x5386 , 0x00},
    {0x5387 , 0x00},
    {0x5388 , 0x01},
    {0x5389 , 0x15},
    {0x538a , 0x00},
    {0x538b , 0x31},
    {0x538c , 0x00},
    {0x538d , 0x00},
    {0x538e , 0x00},
    {0x538f , 0x0f},
    {0x5390 , 0x00},
    {0x5391 , 0xab},
    {0x5392 , 0x00},
    {0x5393 , 0xa2},
    {0x5394 , 0x08},
    {0x5480 , 0x14},
    {0x5481 , 0x21},
    {0x5482 , 0x36},
    {0x5483 , 0x57},
    {0x5484 , 0x65},
    {0x5485 , 0x71},
    {0x5486 , 0x7d},
    {0x5487 , 0x87},
    {0x5488 , 0x91},
    {0x5489 , 0x9a},
    {0x548a , 0xaa},
    {0x548b , 0xb8},
    {0x548c , 0xcd},
    {0x548d , 0xdd},
    {0x548e , 0xea},
    {0x548f , 0x10},
    {0x5490 , 0x05},
    {0x5491 , 0x00},
    {0x5492 , 0x04},
    {0x5493 , 0x20},
    {0x5494 , 0x03},
    {0x5495 , 0x60},
    {0x5496 , 0x02},
    {0x5497 , 0xb8},
    {0x5498 , 0x02},
    {0x5499 , 0x86},
    {0x549a , 0x02},
    {0x549b , 0x5b},
    {0x549c , 0x02},
    {0x549d , 0x3b},
    {0x549e , 0x02},
    {0x549f , 0x1c},
    {0x54a0 , 0x02},
    {0x54a1 , 0x04},
    {0x54a2 , 0x01},
    {0x54a3 , 0xed},
    {0x54a4 , 0x01},
    {0x54a5 , 0xc5},
    {0x54a6 , 0x01},
    {0x54a7 , 0xa5},
    {0x54a8 , 0x01},
    {0x54a9 , 0x6c},
    {0x54aa , 0x01},
    {0x54ab , 0x41},
    {0x54ac , 0x01},
    {0x54ad , 0x20},
    {0x54ae , 0x00},
    {0x54af , 0x16},
    {0x3406 , 0x00},
    {0x5192 , 0x04},
    {0x5191 , 0xf8},
    {0x5193 , 0x70},
    {0x5194 , 0xf0},
    {0x5195 , 0xf0},
    {0x518d , 0x3d},
    {0x518f , 0x54},
    {0x518e , 0x3d},
    {0x5190 , 0x54},
    {0x518b , 0xc0},
    {0x518c , 0xbd},
    {0x5187 , 0x18},
    {0x5188 , 0x18},
    {0x5189 , 0x6e},
    {0x518a , 0x68},
    {0x5186 , 0x1c},
    {0x5181 , 0x50},
    {0x5184 , 0x25},
    {0x5182 , 0x11},
    {0x5183 , 0x14},
    {0x5184 , 0x25},
    {0x5185 , 0x24},
    {0x5025 , 0x82},
    {0x3a0f , 0x7e},
    {0x3a10 , 0x72},
    {0x3a1b , 0x80},
    {0x3a1e , 0x70},
    {0x3a11 , 0xd0},
    {0x3a1f , 0x40},
    {0x5583 , 0x40},
    {0x5584 , 0x40},
    {0x5580 , 0x02},
    {0x3633 , 0x07},
    {0x3702 , 0x10},
    {0x3703 , 0xb2},
    {0x3704 , 0x18},
    {0x370b , 0x40},
    {0x370d , 0x02},
    {0x3620 , 0x52},
    {0X0000, 0X00}

};
/* 2592X1944 QSXGA */
static struct reginfo sensor_qsxga[] =
{
       {0x3000 ,0x00},     //F8
       {0x3001 ,0x00},     //48
       {0x3002 ,0x00},     //5C
       {0x3003 ,0x00},     //02
       {0x3005 ,0xff},     //b7
       {0x3006 ,0xff},     //43
       {0x3007 ,0x3f},     //37
       {0x350c ,0x07},     //08
       {0x350d ,0xd0},     //30
       {0x3602 ,0xe4},     //FC   ?
       {0x3612 ,0xac},     //FF   ?
       {0x3613 ,0x44},     //00   ?
       {0x3621 ,0x27},     //C7
       {0x3622 ,0x08},     //00   ?
       {0x3623 ,0x22},     //01   ?
       {0x3604 ,0x60},     //40   ?
       {0x3705 ,0xda},     //DB   ?
       {0x370a ,0x80},     //81   ?
       {0x3801 ,0x8a},     //50
       {0x3803 ,0x0a},     //08
       {0x3804 ,0x0a},     //05
       {0x3805 ,0x20},     //00
       {0x3806 ,0x07},     //03
       {0x3807 ,0x98},     //C0
       {0x3808 ,0x0a},     //02
       {0x3809 ,0x20},     //80
       {0x380a ,0x07},     //01
       {0x380b ,0x98},     //E0
       {0x380c ,0x0c},
       {0x380d ,0x80},
       {0x380e ,0x07},     //03
       {0x380f ,0xd0},     //E8
       {0x3810 ,0xc2},     //C0
       {0x3815 ,0x44},     //04
       {0x3818 ,0xc0},     //0xc8},     //C1
       {0x3824 ,0x01},     //11
       {0x3827 ,0x0a},     //08
       {0x3a00 ,0x78},     //7C    night mode close
       {0x3a0d ,0x10},     //08
       {0x3a0e ,0x0d},     //06
       {0x3a10 ,0x32},     //72
       {0x3a1b ,0x3c},     //80
       {0x3a1e ,0x32},     //70
       {0x3a11 ,0x80},     //D0
       {0x3a1f ,0x20},     //40
       {0x3a00 ,0x78},     //7C   night mode close
       {0x460b ,0x35},     //37
       {0x471d ,0x00},     //05
       {0x4713 ,0x03},     //02
       {0x471c ,0x50},     //D0
       {0x5682 ,0x0a},     //05
       {0x5683 ,0x20},     //00
       {0x5686 ,0x07},     //03
       {0x5687 ,0x98},     //C0
       {0x5001 ,0x4f},     //FF
       {0x589b ,0x00},     //04
       {0x589a ,0xc0},     //C5
       {0x4407 ,0x04},     //0C
       {0x589b ,0x00},     //04
       {0x589a ,0xc0},     //C5
       {0x3002 ,0x0c},     //5C
       {0x3002 ,0x00},     //5C
       {0x3012, 0x02},

       {0x460c ,0x00},     //jpeg mode change to YUV mode
       {0x460b ,0x37},
       {0x471c ,0xd0},
       {0x471d ,0x05},
       {0x3815 ,0x01},
       {0x3818 ,0xc0},
       {0x501f ,0x00},
       {0x4300 ,0x32},
       {0x3002 ,0x1c},
       {0x0000 ,0x00}
};
/* 2048*1536 QXGA */
static struct reginfo sensor_qxga[] =
{
    {0x3012, 0x02},
    {0x3800 ,0x1 },
    {0x3801 ,0x8A},
    {0x3802 ,0x0 },
    {0x3803 ,0xA },
    {0x3804 ,0xA },
    {0x3805 ,0x20},
    {0x3806 ,0x7 },
    {0x3807 ,0x98},
    {0x3808 ,0x8 },
    {0x3809 ,0x0 },
    {0x380a ,0x6 },
    {0x380b ,0x0 },
    {0x380c ,0xc },
    {0x380d ,0x80},
    {0x380e ,0x7 },
    {0x380f ,0xd0},
    {0x5001 ,0x7f},
    {0x5680 ,0x0 },
    {0x5681 ,0x0 },
    {0x5682 ,0xA },
    {0x5683 ,0x20},
    {0x5684 ,0x0 },
    {0x5685 ,0x0 },
    {0x5686 ,0x7 },
    {0x5687 ,0x98},
    {0x0000 ,0x00}
};

/* 1600X1200 UXGA */
static struct reginfo sensor_uxga[] =
{
    {0x3012, 0x02},
    {0x3800 ,0x1 },
    {0x3801 ,0x8A},
    {0x3802 ,0x0 },
    {0x3803 ,0xA },
    {0x3804 ,0xA },
    {0x3805 ,0x20},
    {0x3806 ,0x7 },
    {0x3807 ,0x98},
    {0x3808 ,0x6 },
    {0x3809 ,0x40},
    {0x380a ,0x4 },
    {0x380b ,0xb0},
    {0x380c ,0xc },
    {0x380d ,0x80},
    {0x380e ,0x7 },
    {0x380f ,0xd0},
    {0x5001 ,0x7f},
    {0x5680 ,0x0 },
    {0x5681 ,0x0 },
    {0x5682 ,0xA },
    {0x5683 ,0x20},
    {0x5684 ,0x0 },
    {0x5685 ,0x0 },
    {0x5686 ,0x7 },
    {0x5687 ,0x98},
    {0x0000 ,0x00}
};

/* 1280X1024 SXGA */
static struct reginfo sensor_sxga[] =
{
	{0x300E, 0x34},
	{0x3011, 0x01},
	{0x3012, 0x00},
	{0x302a, 0x05},
	{0x302b, 0xCB},
	{0x306f, 0x54},
	{0x3362, 0x80},

	{0x3070, 0x5d},
	{0x3072, 0x5d},
	{0x301c, 0x0f},
	{0x301d, 0x0f},

	{0x3020, 0x01},
	{0x3021, 0x18},
	{0x3022, 0x00},
	{0x3023, 0x0A},
	{0x3024, 0x06},
	{0x3025, 0x58},
	{0x3026, 0x04},
	{0x3027, 0xbc},
	{0x3088, 0x05},
	{0x3089, 0x00},
	{0x308A, 0x04},
	{0x308B, 0x00},
	{0x3316, 0x64},
	{0x3317, 0x4B},
	{0x3318, 0x00},
	{0x3319, 0x6C},
	{0x331A, 0x50},
	{0x331B, 0x40},
	{0x331C, 0x00},
	{0x331D, 0x6C},
	{0x3302, 0x11},
	{0x0000,0x00}
};

/* 800X600 SVGA*/
static struct reginfo sensor_svga[] =
{
	{0x300E, 0x34},
	{0x3011, 0x01},
	{0x3012, 0x10},
	{0x302a, 0x02},
	{0x302b, 0xE6},
	{0x306f, 0x14},
	{0x3362, 0x90},

	{0x3070, 0x5d},
	{0x3072, 0x5d},
	{0x301c, 0x07},
	{0x301d, 0x07},

	{0x3020, 0x01},
	{0x3021, 0x18},
	{0x3022, 0x00},
	{0x3023, 0x06},
	{0x3024, 0x06},
	{0x3025, 0x58},
	{0x3026, 0x02},
	{0x3027, 0x5E},
	{0x3088, 0x03},
	{0x3089, 0x20},
	{0x308A, 0x02},
	{0x308B, 0x58},
	{0x3316, 0x64},
	{0x3317, 0x25},
	{0x3318, 0x80},
	{0x3319, 0x08},
	{0x331A, 0x64},
	{0x331B, 0x4B},
	{0x331C, 0x00},
	{0x331D, 0x38},
	{0x3302, 0x11},
	{0x0000,0x00}
};

/* 640X480 VGA */
static struct reginfo sensor_vga[] =
{

    {0x3012, 0x00},
    {0x3800 ,0x1 },
    {0x3801 ,0x8A},
    {0x3802 ,0x0 },
    {0x3803 ,0xA },
    {0x3804 ,0xA },
    {0x3805 ,0x20},
    {0x3806 ,0x7 },
    {0x3807 ,0x98},
    {0x3808 ,0x2 },
    {0x3809 ,0x80},
    {0x380a ,0x1 },
    {0x380b ,0xe0},
    {0x380c ,0xc },
    {0x380d ,0x80},
    {0x380e ,0x7 },
    {0x380f ,0xd0},
    {0x5001 ,0x7f},
    {0x5680 ,0x0 },
    {0x5681 ,0x0 },
    {0x5682 ,0xA },
    {0x5683 ,0x20},
    {0x5684 ,0x0 },
    {0x5685 ,0x0 },
    {0x5686 ,0x7 },
    {0x5687 ,0x98},
    {0x0000,0x00}
};

/* 352X288 CIF */
static struct reginfo sensor_cif[] =
{
    {0x3800 ,0x1 },
    {0x3801 ,0x50},
    {0x3802 ,0x0 },
    {0x3803 ,0x8 },
    {0x3804 ,0x4 },
    {0x3805 ,0x96},
    {0x3806 ,0x3 },
    {0x3807 ,0xc0},
    {0x3808 ,0x1 },
    {0x3809 ,0x60},
    {0x380a ,0x1 },
    {0x380b ,0x20},
    {0x380c ,0xc },
    {0x380d ,0x80},
    {0x380e ,0x3 },
    {0x380f ,0xe8},
    {0x5001 ,0x7f},
    {0x5680 ,0x0 },
    {0x5681 ,0x0 },
    {0x5682 ,0x4 },
    {0x5683 ,0x96},
    {0x5684 ,0x0 },
    {0x5685 ,0x0 },
    {0x5686 ,0x3 },
    {0x5687 ,0xc0},
    {0x0000,0x00}
};

/* 320*240 QVGA */
static  struct reginfo sensor_qvga[] =
	{

	{0x300E, 0x34},
	{0x3011, 0x01},
	{0x3012, 0x10},
	{0x302a, 0x02},
	{0x302b, 0xE6},
	{0x306f, 0x14},
	{0x3362, 0x90},

	{0x3070, 0x5D},
	{0x3072, 0x5D},
	{0x301c, 0x07},
	{0x301d, 0x07},

	{0x3020, 0x01},
	{0x3021, 0x18},
	{0x3022, 0x00},
	{0x3023, 0x06},
	{0x3024, 0x06},
	{0x3025, 0x58},
	{0x3026, 0x02},
	{0x3027, 0x61},
	{0x3088, 0x01},
	{0x3089, 0x40},
	{0x308A, 0x00},
	{0x308B, 0xf0},
	{0x3316, 0x64},
	{0x3317, 0x25},
	{0x3318, 0x80},
	{0x3319, 0x08},
	{0x331A, 0x14},
	{0x331B, 0x0f},
	{0x331C, 0x00},
	{0x331D, 0x38},
	{0x3302, 0x11},
	{0x0000,0x00}
};

/* 176X144 QCIF*/
static struct reginfo sensor_qcif[] =
{
	{0x300E, 0x34},
	{0x3011, 0x01},
	{0x3012, 0x10},
	{0x302a, 0x02},
	{0x302b, 0xE6},
	{0x306f, 0x14},
	{0x3362, 0x90},

	{0x3070, 0x5d},
	{0x3072, 0x5d},
	{0x301c, 0x07},
	{0x301d, 0x07},

	{0x3020, 0x01},
	{0x3021, 0x18},
	{0x3022, 0x00},
	{0x3023, 0x06},
	{0x3024, 0x06},
	{0x3025, 0x58},
	{0x3026, 0x02},
	{0x3027, 0x61},
	{0x3088, 0x00},
	{0x3089, 0xa0},
	{0x308a, 0x00},
	{0x308b, 0x78},
	{0x3316, 0x64},
	{0x3317, 0x25},
	{0x3318, 0x80},
	{0x3319, 0x08},
	{0x331a, 0x0a},
	{0x331b, 0x07},
	{0x331c, 0x80},
	{0x331d, 0x38},
	{0x3100, 0x00},
	{0x3302, 0x11},
	{0x0000,0x00}
};

static struct reginfo sensor_af_firmware[] =
{
	{0x0000,0x00}
};
#if 0
/* 160X120 QQVGA*/
static struct reginfo ov2655_qqvga[] =
{

    {0x300E, 0x34},
    {0x3011, 0x01},
    {0x3012, 0x10},
    {0x302a, 0x02},
    {0x302b, 0xE6},
    {0x306f, 0x14},
    {0x3362, 0x90},

    {0x3070, 0x5d},
    {0x3072, 0x5d},
    {0x301c, 0x07},
    {0x301d, 0x07},

    {0x3020, 0x01},
    {0x3021, 0x18},
    {0x3022, 0x00},
    {0x3023, 0x06},
    {0x3024, 0x06},
    {0x3025, 0x58},
    {0x3026, 0x02},
    {0x3027, 0x61},
    {0x3088, 0x00},
    {0x3089, 0xa0},
    {0x308a, 0x00},
    {0x308b, 0x78},
    {0x3316, 0x64},
    {0x3317, 0x25},
    {0x3318, 0x80},
    {0x3319, 0x08},
    {0x331a, 0x0a},
    {0x331b, 0x07},
    {0x331c, 0x80},
    {0x331d, 0x38},
    {0x3100, 0x00},
    {0x3302, 0x11},

    {0x0, 0x0},
};



static  struct reginfo ov2655_Sharpness_auto[] =
{
    {0x3306, 0x00},
};

static  struct reginfo ov2655_Sharpness1[] =
{
    {0x3306, 0x08},
    {0x3371, 0x00},
};

static  struct reginfo ov2655_Sharpness2[][3] =
{
    //Sharpness 2
    {0x3306, 0x08},
    {0x3371, 0x01},
};

static  struct reginfo ov2655_Sharpness3[] =
{
    //default
    {0x3306, 0x08},
    {0x332d, 0x02},
};
static  struct reginfo ov2655_Sharpness4[]=
{
    //Sharpness 4
    {0x3306, 0x08},
    {0x332d, 0x03},
};

static  struct reginfo ov2655_Sharpness5[] =
{
    //Sharpness 5
    {0x3306, 0x08},
    {0x332d, 0x04},
};
#endif

static  struct reginfo sensor_ClrFmt_YUYV[]=
{
    {0x4300, 0x30},
    {0x0000, 0x00}
};

static  struct reginfo sensor_ClrFmt_UYVY[]=
{
    {0x4300, 0x32},
    {0x0000, 0x00}
};


#if CONFIG_SENSOR_WhiteBalance
static  struct reginfo sensor_WhiteB_Auto[]=
{
    {0x3406,0x00},
	{0x5183,0x80},
	{0x5191,0xff},
	{0x5192,0x00},
	{0x0000,0x00}
};
/* Cloudy Colour Temperature : 6500K - 8000K  */
static  struct reginfo sensor_WhiteB_Cloudy[]=
{
    {0x3406, 0x01},
    {0x3400, 0x07},
    {0x3401, 0x88},
    {0x3402, 0x04},
    {0x3403, 0x00},
    {0x3404, 0x05},
    {0x3405, 0x00},
    {0x0000, 0x00}
};
/* ClearDay Colour Temperature : 5000K - 6500K  */
static  struct reginfo sensor_WhiteB_ClearDay[]=
{
    //Sunny
    {0x3406, 0x01},
    {0x3400, 0x07},
    {0x3401, 0x32},
    {0x3402, 0x04},
    {0x3403, 0x00},
    {0x3404, 0x05},
    {0x3405, 0x36},
    {0x0000, 0x00}
};
/* Office Colour Temperature : 3500K - 5000K  */
static  struct reginfo sensor_WhiteB_TungstenLamp1[]=
{
    //Office
    {0x3406, 0x01},
    {0x3400, 0x06},
    {0x3401, 0x13},
    {0x3402, 0x04},
    {0x3403, 0x00},
    {0x3404, 0x07},
    {0x3405, 0xe2},
    {0x0000, 0x00}

};
/* Home Colour Temperature : 2500K - 3500K  */
static  struct reginfo sensor_WhiteB_TungstenLamp2[]=
{
    //Home
    {0x3406, 0x01},
    {0x3400, 0x04},
    {0x3401, 0x88},
    {0x3402, 0x04},
    {0x3403, 0x00},
    {0x3404, 0x08},
    {0x3405, 0xb6},
    {0x0000, 0x00}
};
static struct reginfo *sensor_WhiteBalanceSeqe[] = {sensor_WhiteB_Auto, sensor_WhiteB_TungstenLamp1,sensor_WhiteB_TungstenLamp2,
    sensor_WhiteB_ClearDay, sensor_WhiteB_Cloudy,NULL,
};
#endif

#if CONFIG_SENSOR_Brightness
static  struct reginfo sensor_Brightness0[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Brightness1[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Brightness2[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Brightness3[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Brightness4[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Brightness5[]=
{
    {0x0000, 0x00}
};
static struct reginfo *sensor_BrightnessSeqe[] = {sensor_Brightness0, sensor_Brightness1, sensor_Brightness2, sensor_Brightness3,
    sensor_Brightness4, sensor_Brightness5,NULL,
};

#endif

#if CONFIG_SENSOR_Effect
static  struct reginfo sensor_Effect_Normal[] =
{
    {0x5001, 0x7f},
	{0x5580, 0x00},
    {0x0000, 0x00}
};

static  struct reginfo sensor_Effect_WandB[] =
{
    {0x5001, 0xff},
	{0x5580, 0x18},
	{0x5585, 0x80},
	{0x5586, 0x80},
    {0x0000, 0x00}
};

static  struct reginfo sensor_Effect_Sepia[] =
{
    {0x5001, 0xff},
	{0x5580, 0x18},
	{0x5585, 0x40},
	{0x5586, 0xa0},
    {0x0000, 0x00}
};

static  struct reginfo sensor_Effect_Negative[] =
{
    //Negative
    {0x5001, 0xff},
	{0x5580, 0x40},
	{0x0000, 0x00}
};
static  struct reginfo sensor_Effect_Bluish[] =
{
    // Bluish
    {0x5001, 0xff},
	{0x5580, 0x18},
	{0x5585, 0xa0},
	{0x5586, 0x40},
    {0x0000, 0x00}
};

static  struct reginfo sensor_Effect_Green[] =
{
    //  Greenish
    {0x5001, 0xff},
	{0x5580, 0x18},
	{0x5585, 0x60},
	{0x5586, 0x60},
    {0x0000, 0x00}
};
static struct reginfo *sensor_EffectSeqe[] = {sensor_Effect_Normal, sensor_Effect_WandB, sensor_Effect_Negative,sensor_Effect_Sepia,
    sensor_Effect_Bluish, sensor_Effect_Green,NULL,
};
#endif
#if CONFIG_SENSOR_Exposure
static  struct reginfo sensor_Exposure0[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Exposure1[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Exposure2[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Exposure3[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Exposure4[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Exposure5[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Exposure6[]=
{
    {0x0000, 0x00}
};

static struct reginfo *sensor_ExposureSeqe[] = {sensor_Exposure0, sensor_Exposure1, sensor_Exposure2, sensor_Exposure3,
    sensor_Exposure4, sensor_Exposure5,sensor_Exposure6,NULL,
};
#endif
#if CONFIG_SENSOR_Saturation
static  struct reginfo sensor_Saturation0[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Saturation1[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Saturation2[]=
{
    {0x0000, 0x00}
};
static struct reginfo *sensor_SaturationSeqe[] = {sensor_Saturation0, sensor_Saturation1, sensor_Saturation2, NULL,};

#endif
#if CONFIG_SENSOR_Contrast
static  struct reginfo sensor_Contrast0[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Contrast1[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Contrast2[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Contrast3[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Contrast4[]=
{
    {0x0000, 0x00}
};


static  struct reginfo sensor_Contrast5[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_Contrast6[]=
{
    {0x0000, 0x00}
};
static struct reginfo *sensor_ContrastSeqe[] = {sensor_Contrast0, sensor_Contrast1, sensor_Contrast2, sensor_Contrast3,
    sensor_Contrast4, sensor_Contrast5, sensor_Contrast6, NULL,
};

#endif
#if CONFIG_SENSOR_Mirror
static  struct reginfo sensor_MirrorOn[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_MirrorOff[]=
{
    {0x0000, 0x00}
};
static struct reginfo *sensor_MirrorSeqe[] = {sensor_MirrorOff, sensor_MirrorOn,NULL,};
#endif
#if CONFIG_SENSOR_Flip
static  struct reginfo sensor_FlipOn[]=
{
    {0x0000, 0x00}
};

static  struct reginfo sensor_FlipOff[]=
{
    {0x0000, 0x00}
};
static struct reginfo *sensor_FlipSeqe[] = {sensor_FlipOff, sensor_FlipOn,NULL,};

#endif
#if CONFIG_SENSOR_Scene
static  struct reginfo sensor_SceneAuto[] =
{
	{0x3a00, 0x78},
	{0x3a03, 0x7D},
	{0x0000, 0x00}
};

static  struct reginfo sensor_SceneNight[] =
{

    //15fps ~ 3.75fps night mode for 60/50Hz light environment, 24Mhz clock input,24Mzh pclk
    {0x3011, 0x08},
    {0x3012, 0x00},
    {0x3010, 0x10},
    {0x460c, 0x22},
    {0x380c, 0x0c},
    {0x380d, 0x80},
    {0x3a00, 0x7c},
    {0x3a08, 0x09},
    {0x3a09, 0x60},
    {0x3a0a, 0x07},
    {0x3a0b, 0xd0},
    {0x3a0d, 0x08},
    {0x3a0e, 0x06},
    {0x3a03, 0xfa},
    {0x0000, 0x00}

};
static struct reginfo *sensor_SceneSeqe[] = {sensor_SceneAuto, sensor_SceneNight,NULL,};

#endif
#if CONFIG_SENSOR_DigitalZoom
static struct reginfo sensor_Zoom0[] =
{
    {0x0, 0x0},
};

static struct reginfo sensor_Zoom1[] =
{
     {0x0, 0x0},
};

static struct reginfo sensor_Zoom2[] =
{
    {0x0, 0x0},
};


static struct reginfo sensor_Zoom3[] =
{
    {0x0, 0x0},
};
static struct reginfo *sensor_ZoomSeqe[] = {sensor_Zoom0, sensor_Zoom1, sensor_Zoom2, sensor_Zoom3, NULL};
#endif
static const struct v4l2_querymenu sensor_menus[] =
{
	#if CONFIG_SENSOR_WhiteBalance
    { .id = V4L2_CID_DO_WHITE_BALANCE,  .index = 0,  .name = "auto",  .reserved = 0, }, {  .id = V4L2_CID_DO_WHITE_BALANCE,  .index = 1, .name = "incandescent",  .reserved = 0,},
    { .id = V4L2_CID_DO_WHITE_BALANCE,  .index = 2,  .name = "fluorescent", .reserved = 0,}, {  .id = V4L2_CID_DO_WHITE_BALANCE, .index = 3,  .name = "daylight", .reserved = 0,},
    { .id = V4L2_CID_DO_WHITE_BALANCE,  .index = 4,  .name = "cloudy-daylight", .reserved = 0,},
    #endif

	#if CONFIG_SENSOR_Effect
    { .id = V4L2_CID_EFFECT,  .index = 0,  .name = "none",  .reserved = 0, }, {  .id = V4L2_CID_EFFECT,  .index = 1, .name = "mono",  .reserved = 0,},
    { .id = V4L2_CID_EFFECT,  .index = 2,  .name = "negative", .reserved = 0,}, {  .id = V4L2_CID_EFFECT, .index = 3,  .name = "sepia", .reserved = 0,},
    { .id = V4L2_CID_EFFECT,  .index = 4, .name = "posterize", .reserved = 0,} ,{ .id = V4L2_CID_EFFECT,  .index = 5,  .name = "aqua", .reserved = 0,},
    #endif

	#if CONFIG_SENSOR_Scene
    { .id = V4L2_CID_SCENE,  .index = 0, .name = "auto", .reserved = 0,} ,{ .id = V4L2_CID_SCENE,  .index = 1,  .name = "night", .reserved = 0,},
    #endif

	#if CONFIG_SENSOR_Flash
    { .id = V4L2_CID_FLASH,  .index = 0,  .name = "off",  .reserved = 0, }, {  .id = V4L2_CID_FLASH,  .index = 1, .name = "auto",  .reserved = 0,},
    { .id = V4L2_CID_FLASH,  .index = 2,  .name = "on", .reserved = 0,}, {  .id = V4L2_CID_FLASH, .index = 3,  .name = "torch", .reserved = 0,},
    #endif
};

static const struct v4l2_queryctrl sensor_controls[] =
{
	#if CONFIG_SENSOR_WhiteBalance
    {
        .id		= V4L2_CID_DO_WHITE_BALANCE,
        .type		= V4L2_CTRL_TYPE_MENU,
        .name		= "White Balance Control",
        .minimum	= 0,
        .maximum	= 4,
        .step		= 1,
        .default_value = 0,
    },
    #endif

	#if CONFIG_SENSOR_Brightness
	{
        .id		= V4L2_CID_BRIGHTNESS,
        .type		= V4L2_CTRL_TYPE_INTEGER,
        .name		= "Brightness Control",
        .minimum	= -3,
        .maximum	= 2,
        .step		= 1,
        .default_value = 0,
    },
    #endif

	#if CONFIG_SENSOR_Effect
	{
        .id		= V4L2_CID_EFFECT,
        .type		= V4L2_CTRL_TYPE_MENU,
        .name		= "Effect Control",
        .minimum	= 0,
        .maximum	= 5,
        .step		= 1,
        .default_value = 0,
    },
	#endif

	#if CONFIG_SENSOR_Exposure
	{
        .id		= V4L2_CID_EXPOSURE,
        .type		= V4L2_CTRL_TYPE_INTEGER,
        .name		= "Exposure Control",
        .minimum	= 0,
        .maximum	= 6,
        .step		= 1,
        .default_value = 0,
    },
	#endif

	#if CONFIG_SENSOR_Saturation
	{
        .id		= V4L2_CID_SATURATION,
        .type		= V4L2_CTRL_TYPE_INTEGER,
        .name		= "Saturation Control",
        .minimum	= 0,
        .maximum	= 2,
        .step		= 1,
        .default_value = 0,
    },
    #endif

	#if CONFIG_SENSOR_Contrast
	{
        .id		= V4L2_CID_CONTRAST,
        .type		= V4L2_CTRL_TYPE_INTEGER,
        .name		= "Contrast Control",
        .minimum	= -3,
        .maximum	= 3,
        .step		= 1,
        .default_value = 0,
    },
	#endif

	#if CONFIG_SENSOR_Mirror
	{
        .id		= V4L2_CID_HFLIP,
        .type		= V4L2_CTRL_TYPE_BOOLEAN,
        .name		= "Mirror Control",
        .minimum	= 0,
        .maximum	= 1,
        .step		= 1,
        .default_value = 1,
    },
    #endif

	#if CONFIG_SENSOR_Flip
	{
        .id		= V4L2_CID_VFLIP,
        .type		= V4L2_CTRL_TYPE_BOOLEAN,
        .name		= "Flip Control",
        .minimum	= 0,
        .maximum	= 1,
        .step		= 1,
        .default_value = 1,
    },
    #endif

	#if CONFIG_SENSOR_Scene
    {
        .id		= V4L2_CID_SCENE,
        .type		= V4L2_CTRL_TYPE_MENU,
        .name		= "Scene Control",
        .minimum	= 0,
        .maximum	= 1,
        .step		= 1,
        .default_value = 0,
    },
    #endif

	#if CONFIG_SENSOR_DigitalZoom
    {
        .id		= V4L2_CID_ZOOM_RELATIVE,
        .type		= V4L2_CTRL_TYPE_INTEGER,
        .name		= "DigitalZoom Control",
        .minimum	= -1,
        .maximum	= 1,
        .step		= 1,
        .default_value = 0,
    }, {
        .id		= V4L2_CID_ZOOM_ABSOLUTE,
        .type		= V4L2_CTRL_TYPE_INTEGER,
        .name		= "DigitalZoom Control",
        .minimum	= 0,
        .maximum	= 3,
        .step		= 1,
        .default_value = 0,
    },
    #endif

	#if CONFIG_SENSOR_Focus
	{
        .id		= V4L2_CID_FOCUS_RELATIVE,
        .type		= V4L2_CTRL_TYPE_INTEGER,
        .name		= "Focus Control",
        .minimum	= -1,
        .maximum	= 1,
        .step		= 1,
        .default_value = 0,
    }, {
        .id		= V4L2_CID_FOCUS_ABSOLUTE,
        .type		= V4L2_CTRL_TYPE_INTEGER,
        .name		= "Focus Control",
        .minimum	= 0,
        .maximum	= 255,
        .step		= 1,
        .default_value = 125,
    },
	#if CONFIG_SENSOR_AutoFocus
	{
        .id		= V4L2_CID_FOCUS_AUTO,
        .type		= V4L2_CTRL_TYPE_BOOLEAN,
        .name		= "Focus Control",
        .minimum	= 0,
        .maximum	= 1,
        .step		= 1,
        .default_value = 0,
    },
    #endif
    #endif

	#if CONFIG_SENSOR_Flash
	{
        .id		= V4L2_CID_FLASH,
        .type		= V4L2_CTRL_TYPE_MENU,
        .name		= "Flash Control",
        .minimum	= 0,
        .maximum	= 3,
        .step		= 1,
        .default_value = 0,
    },
	#endif
};

static int sensor_probe(struct i2c_client *client, const struct i2c_device_id *did);
static int sensor_video_probe(struct soc_camera_device *icd, struct i2c_client *client);
static int sensor_g_control(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int sensor_s_control(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int sensor_g_ext_controls(struct v4l2_subdev *sd,  struct v4l2_ext_controls *ext_ctrl);
static int sensor_s_ext_controls(struct v4l2_subdev *sd,  struct v4l2_ext_controls *ext_ctrl);
static int sensor_suspend(struct soc_camera_device *icd, pm_message_t pm_msg);
static int sensor_resume(struct soc_camera_device *icd);
static int sensor_set_bus_param(struct soc_camera_device *icd,unsigned long flags);
static unsigned long sensor_query_bus_param(struct soc_camera_device *icd);

static struct soc_camera_ops sensor_ops =
{
    .suspend                     = sensor_suspend,
    .resume                       = sensor_resume,
    .set_bus_param		= sensor_set_bus_param,
    .query_bus_param	= sensor_query_bus_param,
    .controls		= sensor_controls,
    .menus                         = sensor_menus,
    .num_controls		= ARRAY_SIZE(sensor_controls),
    .num_menus		= ARRAY_SIZE(sensor_menus),
};

#define COL_FMT(_name, _depth, _fourcc, _colorspace) \
	{ .name = _name, .depth = _depth, .fourcc = _fourcc, \
	.colorspace = _colorspace }

#define JPG_FMT(_name, _depth, _fourcc) \
	COL_FMT(_name, _depth, _fourcc, V4L2_COLORSPACE_JPEG)

static const struct soc_camera_data_format sensor_colour_formats[] = {
	JPG_FMT(SENSOR_NAME_STRING(UYVY), 16, V4L2_PIX_FMT_UYVY),
	JPG_FMT(SENSOR_NAME_STRING(YUYV), 16, V4L2_PIX_FMT_YUYV),
};

typedef struct sensor_info_priv_s
{
    int whiteBalance;
    int brightness;
    int contrast;
    int saturation;
    int effect;
    int scene;
    int digitalzoom;
    int focus;
	int auto_focus;
    int flash;
    int exposure;
    unsigned char mirror;                                        /* HFLIP */
    unsigned char flip;                                          /* VFLIP */
    unsigned int winseqe_cur_addr;
	unsigned int pixfmt;
	unsigned int funmodule_state;
} sensor_info_priv_t;

struct sensor
{
    struct v4l2_subdev subdev;
    struct i2c_client *client;
    sensor_info_priv_t info_priv;
    int model;	/* V4L2_IDENT_OV* codes from v4l2-chip-ident.h */
};

static struct sensor* to_sensor(const struct i2c_client *client)
{
    return container_of(i2c_get_clientdata(client), struct sensor, subdev);
}

/* sensor register write */
static int sensor_write(struct i2c_client *client, u16 reg, u8 val)
{
    int err,cnt;
    u8 buf[3];
    struct i2c_msg msg[1];

    buf[0] = reg >> 8;
    buf[1] = reg & 0xFF;
    buf[2] = val;

    msg->addr = client->addr;
    msg->flags = client->flags;
    msg->buf = buf;
    msg->len = sizeof(buf);
    msg->scl_rate = CONFIG_SENSOR_I2C_SPEED;         /* ddl@rock-chips.com : 100kHz */
    msg->read_type = 0;               /* fpga i2c:0==I2C_NORMAL : direct use number not enum for don't want include spi_fpga.h */

    cnt = 1;
    err = -EAGAIN;

    while ((cnt--) && (err < 0)) {                       /* ddl@rock-chips.com :  Transfer again if transent is failed   */
        err = i2c_transfer(client->adapter, msg, 1);

        if (err >= 0) {
            return 0;
        } else {
            SENSOR_TR("\n %s write reg failed, try to write again!\n",SENSOR_NAME_STRING());
            udelay(10);
        }
    }

    return err;
}

/* sensor register read */
static int sensor_read(struct i2c_client *client, u16 reg, u8 *val)
{
    int err,cnt;
    u8 buf[2];
    struct i2c_msg msg[2];

    buf[0] = reg >> 8;
    buf[1] = reg & 0xFF;

    msg[0].addr = client->addr;
    msg[0].flags = client->flags;
    msg[0].buf = buf;
    msg[0].len = sizeof(buf);
    msg[0].scl_rate = CONFIG_SENSOR_I2C_SPEED;       /* ddl@rock-chips.com : 100kHz */
    msg[0].read_type = 2;   /* fpga i2c:0==I2C_NO_STOP : direct use number not enum for don't want include spi_fpga.h */

    msg[1].addr = client->addr;
    msg[1].flags = client->flags|I2C_M_RD;
    msg[1].buf = buf;
    msg[1].len = 1;
    msg[1].scl_rate = CONFIG_SENSOR_I2C_SPEED;                       /* ddl@rock-chips.com : 100kHz */
    msg[1].read_type = 2;                             /* fpga i2c:0==I2C_NO_STOP : direct use number not enum for don't want include spi_fpga.h */

    cnt = 1;
    err = -EAGAIN;
    while ((cnt--) && (err < 0)) {                       /* ddl@rock-chips.com :  Transfer again if transent is failed   */
        err = i2c_transfer(client->adapter, msg, 2);

        if (err >= 0) {
            *val = buf[0];
            return 0;
        } else {
        	SENSOR_TR("\n %s read reg failed, try to read again! reg:0x%x \n",SENSOR_NAME_STRING(),(unsigned int)val);
            udelay(10);
        }
    }

    return err;
}

/* write a array of registers  */
static int sensor_write_array(struct i2c_client *client, struct reginfo *regarray)
{
    int err, cnt;
    int i = 0;

	cnt = 0;
    while (regarray[i].reg != 0)
    {
        err = sensor_write(client, regarray[i].reg, regarray[i].val);
        if (err < 0)
        {
            if (cnt-- > 0) {
			    SENSOR_TR("%s..write failed current reg:0x%x, Write array again !\n", SENSOR_NAME_STRING(),regarray[i].reg);
				i = 0;
				continue;
            } else {
                SENSOR_TR("%s..write array failed!!!\n", SENSOR_NAME_STRING());
                return -EPERM;
            }
        }
        i++;
    }
    return 0;
}
#if CONFIG_SENSOR_Focus
struct af_cmdinfo
{
	char cmd_tag;
	char cmd_para[4];
	char validate_bit;
};
static int sensor_af_cmdset(struct i2c_client *client, int cmd_main, struct af_cmdinfo *cmdinfo)
{
	int i;

	if (cmdinfo) {
		if (cmdinfo->validate_bit & 0x80) {
			if (sensor_write(client, CMD_TAG_Reg, cmdinfo->cmd_tag)) {
				SENSOR_TR("%s write CMD_TAG_Reg(cmd:0x%x) error!\n",SENSOR_NAME_STRING(),cmd_main);
				goto sensor_af_cmdset_err;
			}
		}
		for (i=0; i<4; i++) {
			if (cmdinfo->validate_bit & (1<<i)) {
				if (sensor_write(client, CMD_PARA0_Reg-i, cmdinfo->cmd_para[i])) {
					SENSOR_TR("%s write CMD_PARA_Reg(0x%x, cmd:0x%x) error!\n",SENSOR_NAME_STRING(),i,cmd_main);
					goto sensor_af_cmdset_err;
				}
			}
		}
	}

	if (sensor_write(client, CMD_MAIN_Reg, cmd_main)) {
		SENSOR_TR("%s write CMD_MAIN_Reg(cmd:0x%x) error!\n",SENSOR_NAME_STRING(),cmd_main);
		goto sensor_af_cmdset_err;
	}

	return 0;
sensor_af_cmdset_err:
	return -1;
}
static int sensor_af_single(struct i2c_client *client)
{
	int ret = 0;
	struct sensor *sensor = to_sensor(client);
	char state,cnt;

	if ((sensor->info_priv.funmodule_state & SENSOR_AF_IS_OK) == 0)
		return -EACCES;

	if (sensor_af_cmdset(client, SingleFocus_Cmd, NULL)) {
		SENSOR_TR("%s single focus mode set error!\n",SENSOR_NAME_STRING());
		ret = -1;
		goto sensor_af_single_end;
	}

	cnt = 0;
    do
    {
    	if (cnt != 0) {
			msleep(1);
    	}
    	cnt++;
		ret = sensor_read(client, STA_FOCUS_Reg, &state);
		if (ret != 0){
		   SENSOR_TR("%s[%d] read focus_status failed\n",SENSOR_NAME_STRING(),__LINE__);
		   ret = -1;
		   goto sensor_af_single_end;
		}
    }while((state == S_FOCUSING) && (cnt<100));

	if (state != S_FOCUSED) {
        SENSOR_TR("%s[%d] focus state(0x%x) is error!\n",SENSOR_NAME_STRING(),__LINE__,state);
		ret = -1;
		goto sensor_af_single_end;
    }

	sensor_af_cmdset(client, ReturnIdle_Cmd, NULL);
sensor_af_single_end:
	return ret;
}
static int sensor_af_zoneupdate(struct i2c_client *client)
{
	int ret = 0;
	struct sensor *sensor = to_sensor(client);

	if ((sensor->info_priv.funmodule_state & SENSOR_AF_IS_OK) == 0)
		return -EACCES;

	if (sensor_af_cmdset(client, UpdateZone_Cmd, NULL)) {
		SENSOR_TR("%s update zone fail!\n",SENSOR_NAME_STRING());
		ret = -1;
		goto sensor_af_zoneupdate_end;
	}

sensor_af_zoneupdate_end:
	return ret;
}
static int sensor_af_init(struct i2c_client *client)
{
	int ret = 0;
	char state,cnt;

	ret = sensor_write_array(client, sensor_af_firmware);
    if (ret != 0) {
       SENSOR_TR("%s Download firmware failed\n",SENSOR_NAME_STRING());
       ret = -1;
	   goto sensor_af_init_end;
    }

	cnt = 0;
    do
    {
    	if (cnt != 0) {
			msleep(1);
    	}
    	cnt++;
		ret = sensor_read(client, STA_FOCUS_Reg, &state);
		if (ret != 0){
		   SENSOR_TR("%s[%d] read focus_status failed\n",SENSOR_NAME_STRING(),__LINE__);
		   ret = -1;
		   goto sensor_af_init_end;
		}
    }while((state == S_STARTUP) && (cnt<50));

    if (state != S_IDLE) {
        SENSOR_TR("%s focus state(0x%x) is error!\n",SENSOR_NAME_STRING(),state);
		ret = -1;
		goto sensor_af_init_end;
    }

	if (sensor_af_single(client)) {
		ret = -1;
		goto sensor_af_init_end;
	}

sensor_af_init_end:
	return ret;
}
#endif
static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
    struct i2c_client *client = sd->priv;
    struct soc_camera_device *icd = client->dev.platform_data;
    struct sensor *sensor = to_sensor(client);
	const struct v4l2_queryctrl *qctrl;
    char value;
    int ret,pid = 0;

    SENSOR_DG("\n%s..%s.. \n",SENSOR_NAME_STRING(),__FUNCTION__);

    /* soft reset */
    ret = sensor_write(client, 0x3008, 0x80);
    if (ret != 0) {
        SENSOR_TR("%s soft reset sensor failed\n",SENSOR_NAME_STRING());
        ret = -ENODEV;
		goto sensor_INIT_ERR;
    }

    mdelay(5);  //delay 5 microseconds
	/* check if it is an sensor sensor */
    ret = sensor_read(client, 0x300a, &value);
    if (ret != 0) {
        SENSOR_TR("read chip id high byte failed\n");
        ret = -ENODEV;
        goto sensor_INIT_ERR;
    }

    pid |= (value << 8);

    ret = sensor_read(client, 0x300b, &value);
    if (ret != 0) {
        SENSOR_TR("read chip id low byte failed\n");
        ret = -ENODEV;
        goto sensor_INIT_ERR;
    }

    pid |= (value & 0xff);
    SENSOR_DG("\n %s  pid = 0x%x\n", SENSOR_NAME_STRING(), pid);
    if (pid == SENSOR_ID) {
        sensor->model = SENSOR_V4L2_IDENT;
    } else {
        SENSOR_TR("error: %s mismatched   pid = 0x%x\n", SENSOR_NAME_STRING(), pid);
        ret = -ENODEV;
        goto sensor_INIT_ERR;
    }

    ret = sensor_write_array(client, sensor_init_data);
    if (ret != 0)
    {
        SENSOR_TR("error: %s initial failed\n",SENSOR_NAME_STRING());
        goto sensor_INIT_ERR;
    }

    icd->user_width = SENSOR_INIT_WIDTH;
    icd->user_height = SENSOR_INIT_HEIGHT;
    sensor->info_priv.winseqe_cur_addr  = (int)SENSOR_INIT_WINSEQADR;
	sensor->info_priv.pixfmt = SENSOR_INIT_PIXFMT;

    /* sensor sensor information for initialization  */
	qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_DO_WHITE_BALANCE);
	if (qctrl)
    	sensor->info_priv.whiteBalance = qctrl->default_value;
	qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_BRIGHTNESS);
	if (qctrl)
    	sensor->info_priv.brightness = qctrl->default_value;
	qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_EFFECT);
	if (qctrl)
    	sensor->info_priv.effect = qctrl->default_value;
	qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_EXPOSURE);
	if (qctrl)
        sensor->info_priv.exposure = qctrl->default_value;

	qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_SATURATION);
	if (qctrl)
        sensor->info_priv.saturation = qctrl->default_value;
	qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_CONTRAST);
	if (qctrl)
        sensor->info_priv.contrast = qctrl->default_value;
	qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_HFLIP);
	if (qctrl)
        sensor->info_priv.mirror = qctrl->default_value;
	qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_VFLIP);
	if (qctrl)
        sensor->info_priv.flip = qctrl->default_value;
	qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_SCENE);
	if (qctrl)
        sensor->info_priv.scene = qctrl->default_value;
	qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_ZOOM_ABSOLUTE);
	if (qctrl)
        sensor->info_priv.digitalzoom = qctrl->default_value;

    /* ddl@rock-chips.com : if sensor support auto focus and flash, programer must run focus and flash code  */
	#if CONFIG_SENSOR_Focus
	if (sensor_af_init(client)) {
		sensor->info_priv.funmodule_state &= (~SENSOR_AF_IS_OK);
	} else {
		sensor->info_priv.funmodule_state |= SENSOR_AF_IS_OK;
		qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_FOCUS_ABSOLUTE);
		if (qctrl)
	        sensor->info_priv.focus = qctrl->default_value;

	}
	#endif

	#if CONFIG_SENSOR_Flash
	qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_FLASH);
	if (qctrl)
        sensor->info_priv.flash = qctrl->default_value;
    #endif

    SENSOR_DG("\n%s..%s.. icd->width = %d.. icd->height %d\n",SENSOR_NAME_STRING(),__FUNCTION__,icd->user_width,icd->user_height);

    return 0;
sensor_INIT_ERR:
    return ret;
}

static  struct reginfo sensor_power_down_sequence[]=
{
    {0x00,0x00}
};
static int sensor_suspend(struct soc_camera_device *icd, pm_message_t pm_msg)
{
    int ret;
    struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
    struct soc_camera_link *icl;


    if (pm_msg.event == PM_EVENT_SUSPEND)
    {
        SENSOR_DG("\n %s Enter Suspend.. \n", SENSOR_NAME_STRING());
        ret = sensor_write_array(client, sensor_power_down_sequence) ;
        if (ret != 0)
        {
            SENSOR_TR("\n %s..%s WriteReg Fail.. \n", SENSOR_NAME_STRING(),__FUNCTION__);
            return ret;
        }
        else
        {
            icl = to_soc_camera_link(icd);
            if (icl->power) {
                ret = icl->power(icd->pdev, 0);
                if (ret < 0) {
				    SENSOR_TR("\n %s suspend fail for turn on power!\n", SENSOR_NAME_STRING());
                    return -EINVAL;
                }
            }
        }
    }
    else
    {
        SENSOR_TR("\n %s cann't suppout Suspend..\n",SENSOR_NAME_STRING());
        return -EINVAL;
    }
    return 0;
}

static int sensor_resume(struct soc_camera_device *icd)
{
    struct soc_camera_link *icl;
    int ret;

    icl = to_soc_camera_link(icd);
    if (icl->power) {
        ret = icl->power(icd->pdev, 1);
        if (ret < 0) {
			SENSOR_TR("\n %s resume fail for turn on power!\n", SENSOR_NAME_STRING());
            return -EINVAL;
        }
    }

	SENSOR_DG("\n %s Enter Resume.. \n", SENSOR_NAME_STRING());

    return 0;

}

static int sensor_set_bus_param(struct soc_camera_device *icd,
                                unsigned long flags)
{

    return 0;
}

static unsigned long sensor_query_bus_param(struct soc_camera_device *icd)
{
    struct soc_camera_link *icl = to_soc_camera_link(icd);
    unsigned long flags = SENSOR_BUS_PARAM;

    return soc_camera_apply_sensor_flags(icl, flags);
}

static int sensor_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *f)
{
    struct i2c_client *client = sd->priv;
    struct soc_camera_device *icd = client->dev.platform_data;
    struct sensor *sensor = to_sensor(client);
    struct v4l2_pix_format *pix = &f->fmt.pix;

    pix->width		= icd->user_width;
    pix->height		= icd->user_height;
    pix->pixelformat	= sensor->info_priv.pixfmt;
    pix->field		= V4L2_FIELD_NONE;
    pix->colorspace		= V4L2_COLORSPACE_JPEG;

    return 0;
}
static int sensor_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *f)
{
    struct i2c_client *client = sd->priv;
    struct sensor *sensor = to_sensor(client);
    struct v4l2_pix_format *pix = &f->fmt.pix;
    struct reginfo *winseqe_set_addr=NULL;
    int ret, set_w,set_h;

	if (sensor->info_priv.pixfmt != pix->pixelformat) {
		switch (pix->pixelformat)
		{
			case V4L2_PIX_FMT_YUYV:
			{
				winseqe_set_addr = sensor_ClrFmt_YUYV;
				break;
			}
			case V4L2_PIX_FMT_UYVY:
			{
				winseqe_set_addr = sensor_ClrFmt_UYVY;
				break;
			}
			default:
				break;
		}
		if (winseqe_set_addr != NULL) {
            sensor_write_array(client, winseqe_set_addr);
			sensor->info_priv.pixfmt = pix->pixelformat;

			SENSOR_DG("%s Pixelformat(0x%x) set success!\n", SENSOR_NAME_STRING(),pix->pixelformat);
		} else {
			SENSOR_TR("%s Pixelformat(0x%x) is invalidate!\n", SENSOR_NAME_STRING(),pix->pixelformat);
		}
	}

    set_w = pix->width;
    set_h = pix->height;

	if (((set_w <= 176) && (set_h <= 144)) && sensor_qcif[0].reg)
	{
		winseqe_set_addr = sensor_qcif;
        set_w = 176;
        set_h = 144;
	}
	else if (((set_w <= 320) && (set_h <= 240)) && sensor_qvga[0].reg)
    {
        winseqe_set_addr = sensor_qvga;
        set_w = 320;
        set_h = 240;
    }
    else if (((set_w <= 352) && (set_h<= 288)) && sensor_cif[0].reg)
    {
        winseqe_set_addr = sensor_cif;
        set_w = 352;
        set_h = 288;
    }
    else if (((set_w <= 640) && (set_h <= 480)) && sensor_vga[0].reg)
    {
        winseqe_set_addr = sensor_vga;
        set_w = 640;
        set_h = 480;
    }
    else if (((set_w <= 800) && (set_h <= 600)) && sensor_svga[0].reg)
    {
        winseqe_set_addr = sensor_svga;
        set_w = 800;
        set_h = 600;
    }
    else if (((set_w <= 1280) && (set_h <= 1024)) && sensor_sxga[0].reg)
    {
        winseqe_set_addr = sensor_sxga;
        set_w = 1280;
        set_h = 1024;
    }
    else if (((set_w <= 1600) && (set_h <= 1200)) && sensor_uxga[0].reg)
    {
        winseqe_set_addr = sensor_uxga;
        set_w = 1600;
        set_h = 1200;
    }
	else if (((set_w <= 2048) && (set_h <= 1536)) && sensor_qxga[0].reg)
    {
        winseqe_set_addr = sensor_qxga;
        set_w = 2048;
        set_h = 1536;
    }
	else if (((set_w <= 2592) && (set_h <= 1944)) && sensor_qsxga[0].reg)
    {
        winseqe_set_addr = sensor_qsxga;
        set_w = 2592;
        set_h = 1944;
    }
    else if (sensor_qvga[0].reg)
    {
        winseqe_set_addr = sensor_qvga;               /* ddl@rock-chips.com : Sensor output smallest size if  isn't support app  */
        set_w = 320;
        set_h = 240;
    }
	else
	{
		SENSOR_TR("\n %s..%s Format is Invalidate. pix->width = %d.. pix->height = %d\n",SENSOR_NAME_STRING(),__FUNCTION__,pix->width,pix->height);
		return -EINVAL;
	}

    if ((int)winseqe_set_addr  != sensor->info_priv.winseqe_cur_addr)
    {
        ret = sensor_write_array(client, winseqe_set_addr);
        if (ret != 0)
        {
            SENSOR_TR("%s set format capability failed\n", SENSOR_NAME_STRING());
            return ret;
        }

        sensor->info_priv.winseqe_cur_addr  = (int)winseqe_set_addr;
        #if CONFIG_SENSOR_Focus
		sensor_af_zoneupdate(client);
		#endif
        SENSOR_DG("\n%s..%s.. icd->width = %d.. icd->height %d\n",SENSOR_NAME_STRING(),__FUNCTION__,set_w,set_h);
    }
    else
    {
        SENSOR_TR("\n %s .. Current Format is validate. icd->width = %d.. icd->height %d\n",SENSOR_NAME_STRING(),set_w,set_h);
    }

    return 0;
}

static int sensor_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *f)
{
    struct v4l2_pix_format *pix = &f->fmt.pix;
    bool bayer = pix->pixelformat == V4L2_PIX_FMT_UYVY ||
        pix->pixelformat == V4L2_PIX_FMT_YUYV;

    /*
    * With Bayer format enforce even side lengths, but let the user play
    * with the starting pixel
    */

    if (pix->height > SENSOR_MAX_HEIGHT)
        pix->height = SENSOR_MAX_HEIGHT;
    else if (pix->height < SENSOR_MIN_HEIGHT)
        pix->height = SENSOR_MIN_HEIGHT;
    else if (bayer)
        pix->height = ALIGN(pix->height, 2);

    if (pix->width > SENSOR_MAX_WIDTH)
        pix->width = SENSOR_MAX_WIDTH;
    else if (pix->width < SENSOR_MIN_WIDTH)
        pix->width = SENSOR_MIN_WIDTH;
    else if (bayer)
        pix->width = ALIGN(pix->width, 2);

    return 0;
}

 static int sensor_g_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *id)
{
    struct i2c_client *client = sd->priv;

    if (id->match.type != V4L2_CHIP_MATCH_I2C_ADDR)
        return -EINVAL;

    if (id->match.addr != client->addr)
        return -ENODEV;

    id->ident = SENSOR_V4L2_IDENT;      /* ddl@rock-chips.com :  Return OV2655  identifier */
    id->revision = 0;

    return 0;
}
#if CONFIG_SENSOR_Brightness
static int sensor_set_brightness(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value)
{
    struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

    if ((value >= qctrl->minimum) && (value <= qctrl->maximum))
    {
        if (sensor_BrightnessSeqe[value - qctrl->minimum] != NULL)
        {
            if (sensor_write_array(client, sensor_BrightnessSeqe[value - qctrl->minimum]) != 0)
            {
                SENSOR_TR("%s..%s WriteReg Fail.. \n",SENSOR_NAME_STRING(), __FUNCTION__);
                return -EINVAL;
            }
            SENSOR_DG("%s..%s : %x\n",SENSOR_NAME_STRING(),__FUNCTION__, value);
            return 0;
        }
    }
	SENSOR_TR("\n %s..%s valure = %d is invalidate..    \n",SENSOR_NAME_STRING(),__FUNCTION__,value);
    return -EINVAL;
}
#endif
#if CONFIG_SENSOR_Effect
static int sensor_set_effect(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value)
{
    struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

    if ((value >= qctrl->minimum) && (value <= qctrl->maximum))
    {
        if (sensor_EffectSeqe[value - qctrl->minimum] != NULL)
        {
            if (sensor_write_array(client, sensor_EffectSeqe[value - qctrl->minimum]) != 0)
            {
                SENSOR_TR("%s..%s WriteReg Fail.. \n",SENSOR_NAME_STRING(), __FUNCTION__);
                return -EINVAL;
            }
            SENSOR_DG("%s..%s : %x\n",SENSOR_NAME_STRING(),__FUNCTION__, value);
            return 0;
        }
    }
	SENSOR_TR("\n %s..%s valure = %d is invalidate..    \n",SENSOR_NAME_STRING(),__FUNCTION__,value);
    return -EINVAL;
}
#endif
#if CONFIG_SENSOR_Exposure
static int sensor_set_exposure(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value)
{
    struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

    if ((value >= qctrl->minimum) && (value <= qctrl->maximum))
    {
        if (sensor_ExposureSeqe[value - qctrl->minimum] != NULL)
        {
            if (sensor_write_array(client, sensor_ExposureSeqe[value - qctrl->minimum]) != 0)
            {
                SENSOR_TR("%s..%s WriteReg Fail.. \n",SENSOR_NAME_STRING(), __FUNCTION__);
                return -EINVAL;
            }
            SENSOR_DG("%s..%s : %x\n",SENSOR_NAME_STRING(),__FUNCTION__, value);
            return 0;
        }
    }
	SENSOR_TR("\n %s..%s valure = %d is invalidate..    \n",SENSOR_NAME_STRING(),__FUNCTION__,value);
    return -EINVAL;
}
#endif
#if CONFIG_SENSOR_Saturation
static int sensor_set_saturation(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value)
{
    struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

    if ((value >= qctrl->minimum) && (value <= qctrl->maximum))
    {
        if (sensor_SaturationSeqe[value - qctrl->minimum] != NULL)
        {
            if (sensor_write_array(client, sensor_SaturationSeqe[value - qctrl->minimum]) != 0)
            {
                SENSOR_TR("%s..%s WriteReg Fail.. \n",SENSOR_NAME_STRING(), __FUNCTION__);
                return -EINVAL;
            }
            SENSOR_DG("%s..%s : %x\n",SENSOR_NAME_STRING(),__FUNCTION__, value);
            return 0;
        }
    }
    SENSOR_TR("\n %s..%s valure = %d is invalidate..    \n",SENSOR_NAME_STRING(),__FUNCTION__,value);
    return -EINVAL;
}
#endif
#if CONFIG_SENSOR_Contrast
static int sensor_set_contrast(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value)
{
    struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

    if ((value >= qctrl->minimum) && (value <= qctrl->maximum))
    {
        if (sensor_ContrastSeqe[value - qctrl->minimum] != NULL)
        {
            if (sensor_write_array(client, sensor_ContrastSeqe[value - qctrl->minimum]) != 0)
            {
                SENSOR_TR("%s..%s WriteReg Fail.. \n",SENSOR_NAME_STRING(), __FUNCTION__);
                return -EINVAL;
            }
            SENSOR_DG("%s..%s : %x\n",SENSOR_NAME_STRING(),__FUNCTION__, value);
            return 0;
        }
    }
    SENSOR_TR("\n %s..%s valure = %d is invalidate..    \n",SENSOR_NAME_STRING(),__FUNCTION__,value);
    return -EINVAL;
}
#endif
#if CONFIG_SENSOR_Mirror
static int sensor_set_mirror(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value)
{
    struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

    if ((value >= qctrl->minimum) && (value <= qctrl->maximum))
    {
        if (sensor_MirrorSeqe[value - qctrl->minimum] != NULL)
        {
            if (sensor_write_array(client, sensor_MirrorSeqe[value - qctrl->minimum]) != 0)
            {
                SENSOR_TR("%s..%s WriteReg Fail.. \n",SENSOR_NAME_STRING(), __FUNCTION__);
                return -EINVAL;
            }
            SENSOR_DG("%s..%s : %x\n",SENSOR_NAME_STRING(),__FUNCTION__, value);
            return 0;
        }
    }
    SENSOR_TR("\n %s..%s valure = %d is invalidate..    \n",SENSOR_NAME_STRING(),__FUNCTION__,value);
    return -EINVAL;
}
#endif
#if CONFIG_SENSOR_Flip
static int sensor_set_flip(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value)
{
    struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

    if ((value >= qctrl->minimum) && (value <= qctrl->maximum))
    {
        if (sensor_FlipSeqe[value - qctrl->minimum] != NULL)
        {
            if (sensor_write_array(client, sensor_FlipSeqe[value - qctrl->minimum]) != 0)
            {
                SENSOR_TR("%s..%s WriteReg Fail.. \n",SENSOR_NAME_STRING(), __FUNCTION__);
                return -EINVAL;
            }
            SENSOR_DG("%s..%s : %x\n",SENSOR_NAME_STRING(),__FUNCTION__, value);
            return 0;
        }
    }
    SENSOR_TR("\n %s..%s valure = %d is invalidate..    \n",SENSOR_NAME_STRING(),__FUNCTION__,value);
    return -EINVAL;
}
#endif
#if CONFIG_SENSOR_Scene
static int sensor_set_scene(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value)
{
    struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

    if ((value >= qctrl->minimum) && (value <= qctrl->maximum))
    {
        if (sensor_SceneSeqe[value - qctrl->minimum] != NULL)
        {
            if (sensor_write_array(client, sensor_SceneSeqe[value - qctrl->minimum]) != 0)
            {
                SENSOR_TR("%s..%s WriteReg Fail.. \n",SENSOR_NAME_STRING(), __FUNCTION__);
                return -EINVAL;
            }
            SENSOR_DG("%s..%s : %x\n",SENSOR_NAME_STRING(),__FUNCTION__, value);
            return 0;
        }
    }
    SENSOR_TR("\n %s..%s valure = %d is invalidate..    \n",SENSOR_NAME_STRING(),__FUNCTION__,value);
    return -EINVAL;
}
#endif
#if CONFIG_SENSOR_WhiteBalance
static int sensor_set_whiteBalance(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value)
{
    struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

    if ((value >= qctrl->minimum) && (value <= qctrl->maximum))
    {
        if (sensor_WhiteBalanceSeqe[value - qctrl->minimum] != NULL)
        {
            if (sensor_write_array(client, sensor_WhiteBalanceSeqe[value - qctrl->minimum]) != 0)
            {
                SENSOR_TR("%s..%s WriteReg Fail.. \n",SENSOR_NAME_STRING(), __FUNCTION__);
                return -EINVAL;
            }
            SENSOR_DG("%s..%s : %x\n",SENSOR_NAME_STRING(),__FUNCTION__, value);
            return 0;
        }
    }
	SENSOR_TR("\n %s..%s valure = %d is invalidate..    \n",SENSOR_NAME_STRING(),__FUNCTION__,value);
    return -EINVAL;
}
#endif
#if CONFIG_SENSOR_DigitalZoom
static int sensor_set_digitalzoom(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int *value)
{
    struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
    struct sensor *sensor = to_sensor(client);
	const struct v4l2_queryctrl *qctrl_info;
    int digitalzoom_cur, digitalzoom_total;

	qctrl_info = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_ZOOM_ABSOLUTE);
	if (qctrl_info)
		return -EINVAL;

    digitalzoom_cur = sensor->info_priv.digitalzoom;
    digitalzoom_total = qctrl_info->maximum;

    if ((*value > 0) && (digitalzoom_cur >= digitalzoom_total))
    {
        SENSOR_TR("%s digitalzoom is maximum - %x\n", SENSOR_NAME_STRING(), digitalzoom_cur);
        return -EINVAL;
    }

    if  ((*value < 0) && (digitalzoom_cur <= qctrl_info->minimum))
    {
        SENSOR_TR("%s digitalzoom is minimum - %x\n", SENSOR_NAME_STRING(), digitalzoom_cur);
        return -EINVAL;
    }

    if ((*value > 0) && ((digitalzoom_cur + *value) > digitalzoom_total))
    {
        *value = digitalzoom_total - digitalzoom_cur;
    }

    if ((*value < 0) && ((digitalzoom_cur + *value) < 0))
    {
        *value = 0 - digitalzoom_cur;
    }

    digitalzoom_cur += *value;

    if (sensor_ZoomSeqe[digitalzoom_cur] != NULL)
    {
        if (sensor_write_array(client, sensor_ZoomSeqe[digitalzoom_cur]) != 0)
        {
            SENSOR_TR("%s..%s WriteReg Fail.. \n",SENSOR_NAME_STRING(), __FUNCTION__);
            return -EINVAL;
        }
        SENSOR_DG("%s..%s : %x\n",SENSOR_NAME_STRING(),__FUNCTION__, *value);
        return 0;
    }

    return -EINVAL;
}
#endif
#if CONFIG_SENSOR_Focus
static int sensor_set_focus_absolute(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
    struct sensor *sensor = to_sensor(client);
	const struct v4l2_queryctrl *qctrl_info;
	struct af_cmdinfo cmdinfo;
	int ret = 0;

	qctrl_info = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_FOCUS_ABSOLUTE);
	if (qctrl_info)
		return -EINVAL;

	if (sensor->info_priv.funmodule_state & SENSOR_AF_IS_OK) {
		if ((value >= qctrl->minimum) && (value <= qctrl->maximum)) {
			cmdinfo.cmd_tag = StepFocus_Spec_Tag;
			cmdinfo.cmd_para[0] = value;
			cmdinfo.validate_bit = 0x81;
			ret = sensor_af_cmdset(client, StepMode_Cmd, &cmdinfo);
			SENSOR_DG("%s..%s : %x\n",SENSOR_NAME_STRING(),__FUNCTION__, value);
		} else {
			ret = -EINVAL;
			SENSOR_TR("\n %s..%s valure = %d is invalidate..    \n",SENSOR_NAME_STRING(),__FUNCTION__,value);
		}
	} else {
		ret = -EACCES;
		SENSOR_TR("\n %s..%s AF module state is error!\n",SENSOR_NAME_STRING(),__FUNCTION__);
	}

	return ret;
}
static int sensor_set_focus_relative(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
	struct sensor *sensor = to_sensor(client);
	const struct v4l2_queryctrl *qctrl_info;
	struct af_cmdinfo cmdinfo;
	int ret = 0;

	qctrl_info = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_FOCUS_RELATIVE);
	if (qctrl_info)
		return -EINVAL;

	if (sensor->info_priv.funmodule_state & SENSOR_AF_IS_OK) {
		if ((value >= qctrl->minimum) && (value <= qctrl->maximum)) {
			if (value > 0) {
				cmdinfo.cmd_tag = StepFocus_Near_Tag;
			} else if (value < 0) {
				cmdinfo.cmd_tag = StepFocus_Far_Tag;
			}
			cmdinfo.validate_bit = 0x80;
			ret = sensor_af_cmdset(client, StepMode_Cmd, &cmdinfo);
			SENSOR_DG("%s..%s : %x\n",SENSOR_NAME_STRING(),__FUNCTION__, value);
		} else {
			ret = -EINVAL;
			SENSOR_TR("\n %s..%s valure = %d is invalidate..    \n",SENSOR_NAME_STRING(),__FUNCTION__,value);
		}
	} else {
		ret = -EACCES;
		SENSOR_TR("\n %s..%s AF module state is error!\n",SENSOR_NAME_STRING(),__FUNCTION__);
	}

	return ret;
}
#if CONFIG_SENSOR_AutoFocus
static int sensor_set_focus_auto(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
	struct sensor *sensor = to_sensor(client);
	const struct v4l2_queryctrl *qctrl_info;
	struct af_cmdinfo cmdinfo;
	int ret = 0;

	qctrl_info = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_FOCUS_AUTO);
	if (qctrl_info)
		return -EINVAL;

	if (sensor->info_priv.funmodule_state & SENSOR_AF_IS_OK) {

	} else {
		ret = -EACCES;
		SENSOR_TR("\n %s..%s AF module state is error!\n",SENSOR_NAME_STRING(),__FUNCTION__);
	}

	return ret;
}
#endif

#endif
static int sensor_g_control(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
    struct i2c_client *client = sd->priv;
    struct sensor *sensor = to_sensor(client);
    const struct v4l2_queryctrl *qctrl;

    qctrl = soc_camera_find_qctrl(&sensor_ops, ctrl->id);

    if (!qctrl)
    {
        SENSOR_TR("\n %s ioctrl id = %d  is invalidate \n", SENSOR_NAME_STRING(), ctrl->id);
        return -EINVAL;
    }

    switch (ctrl->id)
    {
        case V4L2_CID_BRIGHTNESS:
            {
                ctrl->value = sensor->info_priv.brightness;
                break;
            }
        case V4L2_CID_SATURATION:
            {
                ctrl->value = sensor->info_priv.saturation;
                break;
            }
        case V4L2_CID_CONTRAST:
            {
                ctrl->value = sensor->info_priv.contrast;
                break;
            }
        case V4L2_CID_DO_WHITE_BALANCE:
            {
                ctrl->value = sensor->info_priv.whiteBalance;
                break;
            }
        case V4L2_CID_EXPOSURE:
            {
                ctrl->value = sensor->info_priv.exposure;
                break;
            }
        case V4L2_CID_HFLIP:
            {
                ctrl->value = sensor->info_priv.mirror;
                break;
            }
        case V4L2_CID_VFLIP:
            {
                ctrl->value = sensor->info_priv.flip;
                break;
            }
        default :
                break;
    }
    return 0;
}



static int sensor_s_control(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
    struct i2c_client *client = sd->priv;
    struct sensor *sensor = to_sensor(client);
    struct soc_camera_device *icd = client->dev.platform_data;
    const struct v4l2_queryctrl *qctrl;


    qctrl = soc_camera_find_qctrl(&sensor_ops, ctrl->id);

    if (!qctrl)
    {
        SENSOR_TR("\n %s ioctrl id = %d  is invalidate \n", SENSOR_NAME_STRING(), ctrl->id);
        return -EINVAL;
    }

    switch (ctrl->id)
    {
#if CONFIG_SENSOR_Brightness
        case V4L2_CID_BRIGHTNESS:
            {
                if (ctrl->value != sensor->info_priv.brightness)
                {
                    if (sensor_set_brightness(icd, qctrl,ctrl->value) != 0)
                    {
                        return -EINVAL;
                    }
                    sensor->info_priv.brightness = ctrl->value;
                }
                break;
            }
#endif
#if CONFIG_SENSOR_Exposure
        case V4L2_CID_EXPOSURE:
            {
                if (ctrl->value != sensor->info_priv.exposure)
                {
                    if (sensor_set_exposure(icd, qctrl,ctrl->value) != 0)
                    {
                        return -EINVAL;
                    }
                    sensor->info_priv.exposure = ctrl->value;
                }
                break;
            }
#endif
#if CONFIG_SENSOR_Saturation
        case V4L2_CID_SATURATION:
            {
                if (ctrl->value != sensor->info_priv.saturation)
                {
                    if (sensor_set_saturation(icd, qctrl,ctrl->value) != 0)
                    {
                        return -EINVAL;
                    }
                    sensor->info_priv.saturation = ctrl->value;
                }
                break;
            }
#endif
#if CONFIG_SENSOR_Contrast
        case V4L2_CID_CONTRAST:
            {
                if (ctrl->value != sensor->info_priv.contrast)
                {
                    if (sensor_set_contrast(icd, qctrl,ctrl->value) != 0)
                    {
                        return -EINVAL;
                    }
                    sensor->info_priv.contrast = ctrl->value;
                }
                break;
            }
#endif
#if CONFIG_SENSOR_WhiteBalance
        case V4L2_CID_DO_WHITE_BALANCE:
            {
                if (ctrl->value != sensor->info_priv.whiteBalance)
                {
                    if (sensor_set_whiteBalance(icd, qctrl,ctrl->value) != 0)
                    {
                        return -EINVAL;
                    }
                    sensor->info_priv.whiteBalance = ctrl->value;
                }
                break;
            }
#endif
#if CONFIG_SENSOR_Mirror
        case V4L2_CID_HFLIP:
            {
                if (ctrl->value != sensor->info_priv.mirror)
                {
                    if (sensor_set_mirror(icd, qctrl,ctrl->value) != 0)
                        return -EINVAL;
                    sensor->info_priv.mirror = ctrl->value;
                }
                break;
            }
#endif
#if CONFIG_SENSOR_Flip
        case V4L2_CID_VFLIP:
            {
                if (ctrl->value != sensor->info_priv.flip)
                {
                    if (sensor_set_flip(icd, qctrl,ctrl->value) != 0)
                        return -EINVAL;
                    sensor->info_priv.flip = ctrl->value;
                }
                break;
            }
#endif
        default:
            break;
    }

    return 0;
}
static int sensor_g_ext_control(struct soc_camera_device *icd , struct v4l2_ext_control *ext_ctrl)
{
    const struct v4l2_queryctrl *qctrl;
    struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
    struct sensor *sensor = to_sensor(client);

    qctrl = soc_camera_find_qctrl(&sensor_ops, ext_ctrl->id);

    if (!qctrl)
    {
        SENSOR_TR("\n %s ioctrl id = %d  is invalidate \n", SENSOR_NAME_STRING(), ext_ctrl->id);
        return -EINVAL;
    }

    switch (ext_ctrl->id)
    {
        case V4L2_CID_SCENE:
            {
                ext_ctrl->value = sensor->info_priv.scene;
                break;
            }
        case V4L2_CID_EFFECT:
            {
                ext_ctrl->value = sensor->info_priv.effect;
                break;
            }
        case V4L2_CID_ZOOM_ABSOLUTE:
            {
                ext_ctrl->value = sensor->info_priv.digitalzoom;
                break;
            }
        case V4L2_CID_ZOOM_RELATIVE:
            {
                return -EINVAL;
            }
        case V4L2_CID_FOCUS_ABSOLUTE:
            {
                return -EINVAL;
            }
        case V4L2_CID_FOCUS_RELATIVE:
            {
                return -EINVAL;
            }
        case V4L2_CID_FLASH:
            {
                ext_ctrl->value = sensor->info_priv.flash;
                break;
            }
        default :
            break;
    }
    return 0;
}
static int sensor_s_ext_control(struct soc_camera_device *icd, struct v4l2_ext_control *ext_ctrl)
{
    const struct v4l2_queryctrl *qctrl;
    struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
    struct sensor *sensor = to_sensor(client);
    int val_offset;

    qctrl = soc_camera_find_qctrl(&sensor_ops, ext_ctrl->id);

    if (!qctrl)
    {
        SENSOR_TR("\n %s ioctrl id = %d  is invalidate \n", SENSOR_NAME_STRING(), ext_ctrl->id);
        return -EINVAL;
    }

	val_offset = 0;
    switch (ext_ctrl->id)
    {
#if CONFIG_SENSOR_Scene
        case V4L2_CID_SCENE:
            {
                if (ext_ctrl->value != sensor->info_priv.scene)
                {
                    if (sensor_set_scene(icd, qctrl,ext_ctrl->value) != 0)
                        return -EINVAL;
                    sensor->info_priv.scene = ext_ctrl->value;
                }
                break;
            }
#endif
#if CONFIG_SENSOR_Effect
        case V4L2_CID_EFFECT:
            {
                if (ext_ctrl->value != sensor->info_priv.effect)
                {
                    if (sensor_set_effect(icd, qctrl,ext_ctrl->value) != 0)
                        return -EINVAL;
                    sensor->info_priv.effect= ext_ctrl->value;
                }
                break;
            }
#endif
#if CONFIG_SENSOR_DigitalZoom
        case V4L2_CID_ZOOM_ABSOLUTE:
            {
                if ((ext_ctrl->value < qctrl->minimum) || (ext_ctrl->value > qctrl->maximum))
                    return -EINVAL;

                if (ext_ctrl->value != sensor->info_priv.digitalzoom)
                {
                    val_offset = ext_ctrl->value -sensor->info_priv.digitalzoom;

                    if (sensor_set_digitalzoom(icd, qctrl,&val_offset) != 0)
                        return -EINVAL;
                    sensor->info_priv.digitalzoom += val_offset;

                    SENSOR_DG("%s digitalzoom is %x\n",SENSOR_NAME_STRING(),  sensor->info_priv.digitalzoom);
                }

                break;
            }
        case V4L2_CID_ZOOM_RELATIVE:
            {
                if (ext_ctrl->value)
                {
                    if (sensor_set_digitalzoom(icd, qctrl,&ext_ctrl->value) != 0)
                        return -EINVAL;
                    sensor->info_priv.digitalzoom += ext_ctrl->value;

                    SENSOR_DG("%s digitalzoom is %x\n", SENSOR_NAME_STRING(), sensor->info_priv.digitalzoom);
                }
                break;
            }
#endif
#if CONFIG_SENSOR_Focus
        case V4L2_CID_FOCUS_ABSOLUTE:
            {
                if ((ext_ctrl->value < qctrl->minimum) || (ext_ctrl->value > qctrl->maximum))
                    return -EINVAL;

                if (ext_ctrl->value != sensor->info_priv.focus) {
					sensor_set_focus_absolute(icd, qctrl,ext_ctrl->value);
                    //val_offset = ext_ctrl->value -sensor->info_priv.focus;
                    //sensor->info_priv.focus += val_offset;
                }

                break;
            }
        case V4L2_CID_FOCUS_RELATIVE:
            {
                if (ext_ctrl->value) {
                	sensor_set_focus_relative(icd, qctrl,ext_ctrl->value);
                    //sensor->info_priv.focus += ext_ctrl->value;
                    //SENSOR_DG("%s focus is %x\n", SENSOR_NAME_STRING(), sensor->info_priv.focus);
                }
                break;
            }
#endif
#if CONFIG_SENSOR_Flash
        case V4L2_CID_FLASH:
            {
                sensor->info_priv.flash = ext_ctrl->value;

                SENSOR_DG("%s flash is %x\n",SENSOR_NAME_STRING(), sensor->info_priv.flash);
                break;
            }
#endif
        default:
            break;
    }

    return 0;
}

static int sensor_g_ext_controls(struct v4l2_subdev *sd, struct v4l2_ext_controls *ext_ctrl)
{
    struct i2c_client *client = sd->priv;
    struct soc_camera_device *icd = client->dev.platform_data;
    int i, error_cnt=0, error_idx=-1;


    for (i=0; i<ext_ctrl->count; i++) {
        if (sensor_g_ext_control(icd, &ext_ctrl->controls[i]) != 0) {
            error_cnt++;
            error_idx = i;
        }
    }

    if (error_cnt > 1)
        error_idx = ext_ctrl->count;

    if (error_idx != -1) {
        ext_ctrl->error_idx = error_idx;
        return -EINVAL;
    } else {
        return 0;
    }
}

static int sensor_s_ext_controls(struct v4l2_subdev *sd, struct v4l2_ext_controls *ext_ctrl)
{
    struct i2c_client *client = sd->priv;
    struct soc_camera_device *icd = client->dev.platform_data;
    int i, error_cnt=0, error_idx=-1;


    for (i=0; i<ext_ctrl->count; i++) {
        if (sensor_s_ext_control(icd, &ext_ctrl->controls[i]) != 0) {
            error_cnt++;
            error_idx = i;
        }
    }

    if (error_cnt > 1)
        error_idx = ext_ctrl->count;

    if (error_idx != -1) {
        ext_ctrl->error_idx = error_idx;
        return -EINVAL;
    } else {
        return 0;
    }
}

/* Interface active, can use i2c. If it fails, it can indeed mean, that
 * this wasn't our capture interface, so, we wait for the right one */
static int sensor_video_probe(struct soc_camera_device *icd,
			       struct i2c_client *client)
{
    char value;
    int ret,pid = 0;
    struct sensor *sensor = to_sensor(client);

    /* We must have a parent by now. And it cannot be a wrong one.
     * So this entire test is completely redundant. */
    if (!icd->dev.parent ||
	    to_soc_camera_host(icd->dev.parent)->nr != icd->iface)
		return -ENODEV;

    /* soft reset */
    ret = sensor_write(client, 0x3012, 0x80);
    if (ret != 0)
    {
        SENSOR_TR("soft reset %s failed\n",SENSOR_NAME_STRING());
        return -ENODEV;
    }
    mdelay(5);          //delay 5 microseconds

    /* check if it is an sensor sensor */
    ret = sensor_read(client, 0x300a, &value);
    if (ret != 0) {
        SENSOR_TR("read chip id high byte failed\n");
        ret = -ENODEV;
        goto sensor_video_probe_err;
    }

    pid |= (value << 8);

    ret = sensor_read(client, 0x300b, &value);
    if (ret != 0) {
        SENSOR_TR("read chip id low byte failed\n");
        ret = -ENODEV;
        goto sensor_video_probe_err;
    }

    pid |= (value & 0xff);
    SENSOR_DG("\n %s  pid = 0x%x\n", SENSOR_NAME_STRING(), pid);
    if (pid == SENSOR_ID) {
        sensor->model = SENSOR_V4L2_IDENT;
    } else {
        SENSOR_TR("error: %s mismatched   pid = 0x%x\n", SENSOR_NAME_STRING(), pid);
        ret = -ENODEV;
        goto sensor_video_probe_err;
    }

    icd->formats = sensor_colour_formats;
    icd->num_formats = ARRAY_SIZE(sensor_colour_formats);

    return 0;

sensor_video_probe_err:

    return ret;
}

static struct v4l2_subdev_core_ops sensor_subdev_core_ops = {
	.init		= sensor_init,
	.g_ctrl		= sensor_g_control,
	.s_ctrl		= sensor_s_control,
	.g_ext_ctrls          = sensor_g_ext_controls,
	.s_ext_ctrls          = sensor_s_ext_controls,
	.g_chip_ident	= sensor_g_chip_ident,
};

static struct v4l2_subdev_video_ops sensor_subdev_video_ops = {
	.s_fmt		= sensor_s_fmt,
	.g_fmt		= sensor_g_fmt,
	.try_fmt	= sensor_try_fmt,
};

static struct v4l2_subdev_ops sensor_subdev_ops = {
	.core	= &sensor_subdev_core_ops,
	.video = &sensor_subdev_video_ops,
};

static int sensor_probe(struct i2c_client *client,
			 const struct i2c_device_id *did)
{
    struct sensor *sensor;
    struct soc_camera_device *icd = client->dev.platform_data;
    struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
    struct soc_camera_link *icl;
    int ret;

    SENSOR_DG("\n%s..%s..%d..\n",__FUNCTION__,__FILE__,__LINE__);
    if (!icd) {
        dev_err(&client->dev, "%s: missing soc-camera data!\n",SENSOR_NAME_STRING());
        return -EINVAL;
    }

    icl = to_soc_camera_link(icd);
    if (!icl) {
        dev_err(&client->dev, "%s driver needs platform data\n", SENSOR_NAME_STRING());
        return -EINVAL;
    }

    if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
        dev_warn(&adapter->dev,
        	 "I2C-Adapter doesn't support I2C_FUNC_I2C\n");
        return -EIO;
    }

    sensor = kzalloc(sizeof(struct sensor), GFP_KERNEL);
    if (!sensor)
        return -ENOMEM;

    v4l2_i2c_subdev_init(&sensor->subdev, client, &sensor_subdev_ops);

    /* Second stage probe - when a capture adapter is there */
    icd->ops		= &sensor_ops;
    icd->y_skip_top		= 0;

    ret = sensor_video_probe(icd, client);
    if (ret) {
        icd->ops = NULL;
        i2c_set_clientdata(client, NULL);
        kfree(sensor);
    }
    SENSOR_DG("\n%s..%s..%d  ret = %x \n",__FUNCTION__,__FILE__,__LINE__,ret);
    return ret;
}

static int sensor_remove(struct i2c_client *client)
{
    struct sensor *sensor = to_sensor(client);
    struct soc_camera_device *icd = client->dev.platform_data;

    icd->ops = NULL;
    i2c_set_clientdata(client, NULL);
    client->driver = NULL;
    kfree(sensor);

    return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME_STRING(), 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_i2c_driver = {
	.driver = {
		.name = SENSOR_NAME_STRING(),
	},
	.probe		= sensor_probe,
	.remove		= sensor_remove,
	.id_table	= sensor_id,
};

static int __init sensor_mod_init(void)
{
    SENSOR_DG("\n%s..%s.. \n",__FUNCTION__,SENSOR_NAME_STRING());
    return i2c_add_driver(&sensor_i2c_driver);
}

static void __exit sensor_mod_exit(void)
{
    i2c_del_driver(&sensor_i2c_driver);
}

device_initcall_sync(sensor_mod_init);
module_exit(sensor_mod_exit);

MODULE_DESCRIPTION(SENSOR_NAME_STRING(Camera sensor driver));
MODULE_AUTHOR("ddl <kernel@rock-chips>");
MODULE_LICENSE("GPL");
