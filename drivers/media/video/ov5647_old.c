/*
 * Driver for OV5642 CMOS Image Sensor from OmniVision
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
#include <linux/hardirq.h>
#include <linux/circ_buf.h>
#include <linux/miscdevice.h>
#include <media/v4l2-common.h>
#include <media/v4l2-chip-ident.h>
#include <media/soc_camera.h>
#include <plat/rk_camera.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include "ov5647.h"
#include "a3907.h"
#include "Ov5647_AE.c"

static int debug;
module_param(debug, int, S_IRUGO|S_IWUSR);
static int saturation=20;
module_param(saturation, int, S_IRUGO|S_IWUSR|S_IWGRP);
static int contrast =0;
module_param(contrast, int, S_IRUGO|S_IWUSR|S_IWGRP);
static int bright =0;
module_param(bright, int, S_IRUGO|S_IWUSR|S_IWGRP);

#define dprintk(level, fmt, arg...) do {			\
	if (debug >= level) 					\
	printk(KERN_WARNING fmt , ## arg); } while (0)

#define SENSOR_TR(format, ...) printk(KERN_ERR format, ## __VA_ARGS__)
#define SENSOR_DG(format, ...) dprintk(1, format, ## __VA_ARGS__)

#define _CONS(a,b) a##b
#define CONS(a,b) _CONS(a,b)

#define __STR(x) #x
#define _STR(x) __STR(x)
#define STR(x) _STR(x)

/* Sensor Driver Configuration */
#define SENSOR_NAME RK29_CAM_SENSOR_OV5647
#define SENSOR_V4L2_IDENT V4L2_IDENT_OV5647
#define SENSOR_ID 0x5647
#define SENSOR_MIN_WIDTH    176
#define SENSOR_MIN_HEIGHT   144
#define SENSOR_MAX_WIDTH    2592
#define SENSOR_MAX_HEIGHT   1944 
#define SENSOR_INIT_WIDTH    2592			/* Sensor pixel size for sensor_init_data array */
#define SENSOR_INIT_HEIGHT	 1944
#define SENSOR_INIT_WINSEQADR  sensor_qsxga
#define SENSOR_AE_LIST_INDEX	SENSOR_AE_LIST_INDEX_QSXGA
#define SENSOR_INIT_PIXFMT    V4L2_MBUS_FMT_SBGGR10_1X10
#define SENSOR_BUS_PARAM      (SOCAM_MASTER | SOCAM_PCLK_SAMPLE_RISING|SOCAM_HSYNC_ACTIVE_HIGH\
                              |SOCAM_VSYNC_ACTIVE_LOW |SOCAM_DATA_ACTIVE_HIGH | SOCAM_DATAWIDTH_10 \
                              |SOCAM_MCLK_24MHZ)

#define CONFIG_SENSOR_WhiteBalance	1
#define CONFIG_SENSOR_Brightness	0
#define CONFIG_SENSOR_Contrast      0
#define CONFIG_SENSOR_Saturation    0
#define CONFIG_SENSOR_Effect        1
#define CONFIG_SENSOR_Scene         1
#define CONFIG_SENSOR_DigitalZoom   0
#define CONFIG_SENSOR_Exposure      0
#define CONFIG_SENSOR_Flash         1
#define CONFIG_SENSOR_Mirror        0
#define CONFIG_SENSOR_Flip          0
#ifdef CONFIG_OV5647_AUTOFOCUS
#define CONFIG_SENSOR_Focus         1
#else
#define CONFIG_SENSOR_Focus         0
#endif

#define CONFIG_SENSOR_I2C_SPEED     100000       /* Hz */
/* Sensor write register continues by preempt_disable/preempt_enable for current process not be scheduled */
#define CONFIG_SENSOR_I2C_NOSCHED   0
#define CONFIG_SENSOR_I2C_RDWRCHK   0

#define COLOR_TEMPERATURE_CLOUDY_DN  6500
#define COLOR_TEMPERATURE_CLOUDY_UP    8000
#define COLOR_TEMPERATURE_CLEARDAY_DN  5000
#define COLOR_TEMPERATURE_CLEARDAY_UP    6500
#define COLOR_TEMPERATURE_OFFICE_DN     3500
#define COLOR_TEMPERATURE_OFFICE_UP     5000
#define COLOR_TEMPERATURE_HOME_DN       2500
#define COLOR_TEMPERATURE_HOME_UP       3500

#define SENSOR_NAME_STRING(a) STR(CONS(SENSOR_NAME, a))
#define SENSOR_NAME_VARFUN(a) CONS(SENSOR_NAME, a)

#define SENSOR_AF_IS_ERR    (0x00<<0)
#define SENSOR_AF_IS_OK		(0x01<<0)
#define SENSOR_INIT_IS_ERR   (0x00<<28)
#define SENSOR_INIT_IS_OK    (0x01<<28)

#if CONFIG_SENSOR_Focus
#define SENSOR_AF_MODE_INFINITY    0
#define SENSOR_AF_MODE_MACRO       1
#define SENSOR_AF_MODE_FIXED       2
#define SENSOR_AF_MODE_AUTO        3
#define SENSOR_AF_MODE_CONTINUOUS  4
#define SENSOR_AF_MODE_CLOSE       5
#define SENSOR_AF_MODE_TEST       6
#endif

#define SENSOR_S_R_GAIN           0x00
#define SENSOR_S_G_GAIN           0x01
#define SENSOR_S_B_GAIN           0x02
#define SENSOR_G_R_GAIN           0x80
#define SENSOR_G_G_GAIN           0x81
#define SENSOR_G_B_GAIN           0x82

/* Sensor Register address */
#define SENSOR_R_GAIN_MSB         0x5186
#define SENSOR_R_GAIN_LSB         0x5187
#define SENSOR_G_GAIN_MSB         0x5188
#define SENSOR_G_GAIN_LSB         0x5189
#define SENSOR_B_GAIN_MSB         0x518A
#define SENSOR_B_GAIN_LSB         0x518b

/********AE parament************/
#define AE_GOAL 110
#define AE_TABLE_COUNT 331
#define AE_INDEX_INIT  30
#define AE_VALUE_INIT  0x6f00
#define AG_VALUE_INIT  0x6f
/********gamma***********/
struct gamma_line_t{
	int A;
	int B;
	int C;
};

struct gamma_info_t{
	int gamma_count;
	int *gamma_in_array;
	int *gamma_out_array;
	struct gamma_line_t *gamma_line;
	int gamma_status;
	
};

static struct gamma_info_t sensor_gamma_info;
//static int gamma_in_array[] = {0,4,8,16,32,40,48,56,64,72,80,96,112,144,176,208,255};
//static int gamma_out_array[] = {0,7,14,28,53,66,79,91,103,116,127,150,171,209,234,248,255};
static int gamma_in_array[] = {2,4,8,16,24,32,40,48,56,64,80,96,112,128,144,160,192,208,224,240};
static int gamma_out_array[] = {1,3,6,14,22,31,43,61,77,94,120,145,163,180,193,206,224,232,240,248}; //ok
/**************lens correction parament*********/
#define LENS_CENTRE_X 	(1300)
#define LENS_CENTRE_Y	(1000)
#define LENS_R_B2		(8)
#define LENS_R_B4		(-1)
#define LENS_G_B2		(7)
#define LENS_G_B4		(-4)
#define LENS_B_B2		(5)
#define LENS_B_B4		(-5)
/****************ccm matrix ****************/
static int ccm_matrix[] = {
1500,  -540,  64,   0,
-212,  1140,  96,   0,
-16,   -360,  1400, 0,
 0,       0,   0,   1024,
};


/* init 2592X1944 QSXGA */
static struct reginfo sensor_init_data[] =
{
    {0x0100,0x00},
    {0x0103,0x01},
    {SEQUENCE_WAIT_MS,0x05},
    {0x370c,0x03},
    {0x5000,0x06},
    {0x5003,0x08},
    {0x5a00,0x08},
    {0x3000,0xff},
    {0x3001,0xff},
    {0x3002,0xff},
    {0x301d,0xf0},
    {0x3a18,0x00},
    {0x3a19,0xf8},
    {0x3c01,0x80},
    {0x3b07,0x0c},
    {0x3630,0x2e},
    {0x3632,0xe2},
    {0x3633,0x23},
    {0x3634,0x44},
    {0x3620,0x64},
    {0x3621,0xe0},
    {0x3600,0x37},
    {0x3704,0xa0},
    {0x3703,0x5a},
    {0x3715,0x78},
    {0x3717,0x01},
    {0x3731,0x02},
    {0x370b,0x60},
    {0x3705,0x1a},
    {0x3f05,0x02},
    {0x3f06,0x10},
    {0x3f01,0x0a},
    {0x3a0f,0x58},
    {0x3a10,0x50},
    {0x3a1b,0x58},
    {0x3a1e,0x50},
    {0x3a11,0x60},
    {0x3a1f,0x28},
    {0x4001,0x02},
    {0x4000,0x09},
    {0x3503,0x03},//manual,0xAE
    {0x3501,0x6f},//
    {0x3502,0x00},//
    {0x350a,0x00},//
    {0x350b,0x6f},//
    {0x5001,0x01},//manual,0xAWB
    {0x5180,0x08},//
    {0x5186,0x06},//
    {0x5187,0x00},//
    {0x5188,0x04},//
    {0x5189,0x00},//
    {0x518a,0x04},//
    {0x518b,0x00},//
    {0x5000,0x00},//lenc WBC on  
    {0x3011,0x62},
	{0x3035,0x11},
	{0x3036,0x64},
	{0x303c,0x11},
	{0x3821,0x06},
	{0x3820,0x00},
	{0x3612,0x5b},
	{0x3618,0x04},
	{0x380c,0x0a},
	{0x380d,0x8c},
	{0x380e,0x07},
	{0x380f,0xb6},
	{0x3814,0x11},
	{0x3815,0x11},
	{0x3708,0x64},
	{0x3709,0x12},
	{0x3808,0x0a},
	{0x3809,0x20},
	{0x380a,0x07},
	{0x380b,0x98},
	{0x3800,0x00},
	{0x3801,0x0c},
	{0x3802,0x00},
	{0x3803,0x04},
	{0x3804,0x0a},
	{0x3805,0x33},
	{0x3806,0x07},
	{0x3807,0xa3},
	{0x3a08,0x01},
	{0x3a09,0x28},
	{0x3a0a,0x00},
	{0x3a0b,0xf6},
	{0x3a0d,0x08},
	{0x3a0e,0x06},
	{0x4004,0x04},
	{0x4837,0x19},
	{0x0100,0x01},
	{SEQUENCE_END ,0x00}

};

/* 720p 15fps @ 1280x720 */

static struct reginfo sensor_720p[]=
{
#if 0
    {0x0100,0x00},
    {0x3035,0x21},
    {0x3036,0x64},
    {0x303c,0x12},
    {0x3821,0x07},
    {0x3820,0x41},
    {0x3612,0x59},
    {0x3618,0x00},
    {0x380c,0x07},
    {0x380d,0x00},
    {0x380e,0x02},
    {0x380f,0xe8},
    {0x3814,0x31},
    {0x3815,0x31},
    {0x3708,0x64},
    {0x3709,0x52},
    {0x3808,0x05},
    {0x3809,0x00},
    {0x380a,0x02},
    {0x380b,0xd0},
    {0x3800,0x00},
    {0x3801,0x18},
    {0x3802,0x00},
    {0x3803,0xf8},
    {0x3804,0x0a},
    {0x3805,0x27},
    {0x3806,0x06},
    {0x3807,0xa7},
    {0x3a08,0x01},
    {0x3a09,0xbe},
    {0x3a0a,0x01},
    {0x3a0b,0x74},
    {0x3a0d,0x02},
    {0x3a0e,0x01},
    {0x4004,0x02},
    {0x4837,0x32},
    {0x0100,0x01},
#endif
	{SEQUENCE_END ,0x00}

};
static struct reginfo sensor_960p[]=
{
	#if 0
    {0x0100,0x00},
    {0x3035,0x11},
    {0x3036,0x46},
    {0x303c,0x11},
    {0x3821,0x07},
    {0x3820,0x41},
    {0x3612,0x59},
    {0x3618,0x00},
    {0x380c,0x07},
    {0x380d,0x68},
    {0x380e,0x03},
    {0x380f,0xd8},
    {0x3814,0x31},
    {0x3815,0x31},
    {0x3708,0x64},
    {0x3709,0x52},
    {0x3808,0x05},
    {0x3809,0x00},
    {0x380a,0x03},
    {0x380b,0xc0},
    {0x3800,0x00},
    {0x3801,0x18},
    {0x3802,0x00},
    {0x3803,0x0e},
    {0x3804,0x0a},
    {0x3805,0x27},
    {0x3806,0x07},
    {0x3807,0x95},
    {0x3a08,0x01},
    {0x3a09,0x27},
    {0x3a0a,0x00},
    {0x3a0b,0xf6},
    {0x3a0d,0x04},
    {0x3a0e,0x03},
    {0x4004,0x02},
    {0x4837,0x24},
    {0x0100,0x01},
    #endif
	{SEQUENCE_END ,0x00}

};    
/* 	1080p, 0x15fps, 0xyuv @1920x1080 */

static struct reginfo sensor_1080p[]=
{
#if 0
    {0x0100,0x00},
    {0x3035,0x21},
    {0x3036,0x64},
    {0x303c,0x11},
    {0x3821,0x06},
    {0x3820,0x00},
    {0x3612,0x5b},
    {0x3618,0x04},
    {0x380c,0x09},
    {0x380d,0x70},
    {0x380e,0x04},
    {0x380f,0x50},
    {0x3814,0x11},
    {0x3815,0x11},
    {0x3708,0x64},
    {0x3709,0x12},
    {0x3808,0x07},
    {0x3809,0x80},
    {0x380a,0x04},
    {0x380b,0x38},
    {0x3800,0x01},
    {0x3801,0x5c},
    {0x3802,0x01},
    {0x3803,0xb2},
    {0x3804,0x08},
    {0x3805,0xe3},
    {0x3806,0x05},
    {0x3807,0xf1},
    {0x3a08,0x01},
    {0x3a09,0x4b},
    {0x3a0a,0x01},
    {0x3a0b,0x13},
    {0x3a0d,0x04},
    {0x3a0e,0x03},
    {0x4004,0x04},
    {0x4837,0x19},
    {0x0100,0x01},
#endif    
	{SEQUENCE_END ,0x00}

};

/* 2592X1944 QSXGA */
static struct reginfo sensor_qsxga[] =
{
			{0x0100,0x00},
			{0x3035,0x11},
			{0x3036,0x64},
			{0x303c,0x11},
			{0x3821,0x06},
			{0x3820,0x00},
			{0x3612,0x5b},
			{0x3618,0x04},
			{0x380c,0x0a},
			{0x380d,0x8c},
			{0x380e,0x07},
			{0x380f,0xb6},
			{0x3814,0x11},
			{0x3815,0x11},
			{0x3708,0x64},
			{0x3709,0x12},
			{0x3808,0x0a},
			{0x3809,0x20},
			{0x380a,0x07},
			{0x380b,0x98},
			{0x3800,0x00},
			{0x3801,0x0c},
			{0x3802,0x00},
			{0x3803,0x04},
			{0x3804,0x0a},
			{0x3805,0x33},
			{0x3806,0x07},
			{0x3807,0xa3},
			{0x3a08,0x01},
			{0x3a09,0x28},
			{0x3a0a,0x00},
			{0x3a0b,0xf6},
			{0x3a0d,0x08},
			{0x3a0e,0x06},
			{0x4004,0x04},
			{0x4837,0x19},
			{0x0100,0x01},
			{SEQUENCE_END ,0x00}

};
/* 2048*1536 QXGA */
static struct reginfo sensor_qxga[] =
{

	{SEQUENCE_END ,0x00}

};

/* 1600X1200 UXGA */
static struct reginfo sensor_uxga[] =
{

	{SEQUENCE_END ,0x00}

};
/* 1280X1024 SXGA */
static struct reginfo sensor_sxga[] =
{

	{SEQUENCE_END ,0x00}

};
/*  1024X768 XGA */
static struct reginfo sensor_xga[] =
{
	{SEQUENCE_END ,0x00}

};

/* 800X600 SVGA*/
static struct reginfo sensor_svga[] =
{
	{SEQUENCE_END ,0x00}

};

/* 640X480 VGA */
static struct reginfo sensor_vga[] =
{
#if 0
    {0x0100,0x00},
    {0x3035,0x21},
    {0x3036,0x46},
    {0x303c,0x12},
    {0x3821,0x07},
    {0x3820,0x41},
    {0x3612,0x59},
    {0x3618,0x00},
    {0x380c,0x07},
    {0x380d,0x3c},
    {0x380e,0x01},
    {0x380f,0xf8},
    {0x3814,0x71},
    {0x3815,0x71},
    {0x3708,0x64},
    {0x3709,0x52},
    {0x3808,0x02},
    {0x3809,0x80},
    {0x380a,0x01},
    {0x380b,0xe0},
    {0x3800,0x00},
    {0x3801,0x10},
    {0x3802,0x00},
    {0x3803,0x00},
    {0x3804,0x0a},
    {0x3805,0x2f},
    {0x3806,0x07},
    {0x3807,0x9f},
    {0x3a08,0x01},
    {0x3a09,0x2e},
    {0x3a0a,0x00},
    {0x3a0b,0xfb},
    {0x3a0d,0x02},
    {0x3a0e,0x01},
    {0x4004,0x02},
    {0x4837,0x47},
    {0x0100,0x01},
#endif    
    {SEQUENCE_END,0x00},
    {0x0000,0x00}
};

/* 352X288 CIF */
static struct reginfo sensor_cif[] =
{

	{SEQUENCE_END ,0x00}

};

/* 320*240 QVGA */
static  struct reginfo sensor_qvga[] =
{
#if 0
    {0x0100,0x00},
    {0x3035,0x41},
    {0x3036,0x46},
    {0x303c,0x14},
    {0x3821,0x07},
    {0x3820,0x41},
    {0x3612,0x59},
    {0x3618,0x00},
    {0x380c,0x06},
    {0x380d,0xe6},
    {0x380e,0x01},
    {0x380f,0x08},
    {0x3814,0xf1},
    {0x3815,0xf1},
    {0x3708,0x24},
    {0x3709,0x52},
    {0x3808,0x01},
    {0x3809,0x40},
    {0x380a,0x00},
    {0x380b,0xf0},
    {0x3800,0x00},
    {0x3801,0x00},
    {0x3802,0x00},
    {0x3803,0x00},
    {0x3804,0x0a},
    {0x3805,0x3f},
    {0x3806,0x07},
    {0x3807,0x9f},
    {0x3a08,0x00},
    {0x3a09,0x9e},
    {0x3a0a,0x00},
    {0x3a0b,0x84},
    {0x3a0d,0x02},
    {0x3a0e,0x01},
    {0x4004,0x02},
    {0x4837,0x8f},
    {0x0100,0x01},
#endif    
	{SEQUENCE_END ,0x00}

};

/* 176X144 QCIF*/
static struct reginfo sensor_qcif[] =
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_ClrFmt_YUYV[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_ClrFmt_UYVY[]=
{

	{SEQUENCE_END ,0x00}

};


#if CONFIG_SENSOR_WhiteBalance
static  struct reginfo sensor_WhiteB_Auto[]=
{

	{SEQUENCE_END ,0x00}

};
/* Cloudy Colour Temperature : 6500K - 8000K  */
static  struct reginfo sensor_WhiteB_Cloudy[]=
{

	{SEQUENCE_END ,0x00}

};
/* ClearDay Colour Temperature : 5000K - 6500K  */
static  struct reginfo sensor_WhiteB_ClearDay[]=
{

	{SEQUENCE_END ,0x00}

};
/* Office Colour Temperature : 3500K - 5000K  */
static  struct reginfo sensor_WhiteB_TungstenLamp1[]=
{

	{SEQUENCE_END ,0x00}

};
/* Home Colour Temperature : 2500K - 3500K  */
static  struct reginfo sensor_WhiteB_TungstenLamp2[]=
{

	{SEQUENCE_END ,0x00}

};
static struct reginfo *sensor_WhiteBalanceSeqe[] = {sensor_WhiteB_Auto, sensor_WhiteB_TungstenLamp1,sensor_WhiteB_TungstenLamp2,
    sensor_WhiteB_ClearDay, sensor_WhiteB_Cloudy,NULL,
};
#endif

#if CONFIG_SENSOR_Brightness
static  struct reginfo sensor_Brightness0[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Brightness1[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Brightness2[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Brightness3[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Brightness4[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Brightness5[]=
{

	{SEQUENCE_END ,0x00}

};
static struct reginfo *sensor_BrightnessSeqe[] = {sensor_Brightness0, sensor_Brightness1, sensor_Brightness2, sensor_Brightness3,
    sensor_Brightness4, sensor_Brightness5,NULL,
};

#endif

#if CONFIG_SENSOR_Effect
static  struct reginfo sensor_Effect_Normal[] =
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Effect_WandB[] =
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Effect_Sepia[] =
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Effect_Negative[] =
{

	{SEQUENCE_END ,0x00}

};
static  struct reginfo sensor_Effect_Bluish[] =
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Effect_Green[] =
{

	{SEQUENCE_END ,0x00}

};
static struct reginfo *sensor_EffectSeqe[] = {sensor_Effect_Normal, sensor_Effect_WandB, sensor_Effect_Negative,sensor_Effect_Sepia,
    sensor_Effect_Bluish, sensor_Effect_Green,NULL,
};
#endif
#if CONFIG_SENSOR_Exposure
static  struct reginfo sensor_Exposure0[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Exposure1[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Exposure2[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Exposure3[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Exposure4[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Exposure5[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Exposure6[]=
{

	{SEQUENCE_END ,0x00}

};

static struct reginfo *sensor_ExposureSeqe[] = {sensor_Exposure0, sensor_Exposure1, sensor_Exposure2, sensor_Exposure3,
    sensor_Exposure4, sensor_Exposure5,sensor_Exposure6,NULL,
};
#endif
#if CONFIG_SENSOR_Saturation
static  struct reginfo sensor_Saturation0[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Saturation1[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Saturation2[]=
{

	{SEQUENCE_END ,0x00}

};
static struct reginfo *sensor_SaturationSeqe[] = {sensor_Saturation0, sensor_Saturation1, sensor_Saturation2, NULL,};

#endif
#if CONFIG_SENSOR_Contrast
static  struct reginfo sensor_Contrast0[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Contrast1[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Contrast2[]=
{

	{SEQUENCE_END ,0x00}

};
static  struct reginfo sensor_Contrast3[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Contrast4[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Contrast5[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_Contrast6[]=
{

	{SEQUENCE_END ,0x00}

};
static struct reginfo *sensor_ContrastSeqe[] = {sensor_Contrast0, sensor_Contrast1, sensor_Contrast2, sensor_Contrast3,
    sensor_Contrast4, sensor_Contrast5, sensor_Contrast6, NULL,
};

#endif
#if CONFIG_SENSOR_Mirror
static  struct reginfo sensor_MirrorOn[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_MirrorOff[]=
{

	{SEQUENCE_END ,0x00}

};
static struct reginfo *sensor_MirrorSeqe[] = {sensor_MirrorOff, sensor_MirrorOn,NULL,};
#endif
#if CONFIG_SENSOR_Flip
static  struct reginfo sensor_FlipOn[]=
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_FlipOff[]=
{

	{SEQUENCE_END ,0x00}

};
static struct reginfo *sensor_FlipSeqe[] = {sensor_FlipOff, sensor_FlipOn,NULL,};

#endif
#if CONFIG_SENSOR_Scene
static  struct reginfo sensor_SceneAuto[] =
{

	{SEQUENCE_END ,0x00}

};

static  struct reginfo sensor_SceneNight[] =
{

	{SEQUENCE_END ,0x00}

};
static struct reginfo *sensor_SceneSeqe[] = {sensor_SceneAuto, sensor_SceneNight,NULL,};

#endif
#if CONFIG_SENSOR_DigitalZoom
static struct reginfo sensor_Zoom0[] =
{

	{SEQUENCE_END ,0x00}

};

static struct reginfo sensor_Zoom1[] =
{

	{SEQUENCE_END ,0x00}

};

static struct reginfo sensor_Zoom2[] =
{

	{SEQUENCE_END ,0x00}

};


static struct reginfo sensor_Zoom3[] =
{

	{SEQUENCE_END ,0x00}

};
static struct reginfo *sensor_ZoomSeqe[] = {sensor_Zoom0, sensor_Zoom1, sensor_Zoom2, sensor_Zoom3, NULL};
#endif
static  struct v4l2_querymenu sensor_menus[] =
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
	{
        .id		= V4L2_CID_FOCUS_AUTO,
        .type		= V4L2_CTRL_TYPE_BOOLEAN,
        .name		= "Focus Control",
        .minimum	= 0,
        .maximum	= 1,
        .step		= 1,
        .default_value = 0,
    },{
        .id		= V4L2_CID_FOCUS_CONTINUOUS,
        .type		= V4L2_CTRL_TYPE_BOOLEAN,
        .name		= "Focus Control",
        .minimum	= 0,
        .maximum	= 1,
        .step		= 1,
        .default_value = 0,
    },
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
static int sensor_set_effect(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value);
static int sensor_set_whiteBalance(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value);
static int sensor_deactivate(struct i2c_client *client);
static bool sensor_fmt_capturechk(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf);
static bool sensor_fmt_videochk(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf);
static int gamma_init(void);


static struct soc_camera_ops sensor_ops =
{
    .suspend                     = sensor_suspend,
    .resume                       = sensor_resume,
    .set_bus_param		= sensor_set_bus_param,
    .query_bus_param	= sensor_query_bus_param,
    .controls		= sensor_controls,
    .menus          = sensor_menus,
    .num_controls		= ARRAY_SIZE(sensor_controls),
    .num_menus		= ARRAY_SIZE(sensor_menus),
};

/* only one fixed colorspace per pixelcode */
struct sensor_datafmt {
	enum v4l2_mbus_pixelcode code;
	enum v4l2_colorspace colorspace;
};

/* Find a data format by a pixel code in an array */
static const struct sensor_datafmt *sensor_find_datafmt(
	enum v4l2_mbus_pixelcode code, const struct sensor_datafmt *fmt,
	int n)
{
	int i;
	for (i = 0; i < n; i++)
		if (fmt[i].code == code)
			return fmt + i;

	return NULL;
}

static const struct sensor_datafmt sensor_colour_fmts[] = {
    {V4L2_MBUS_FMT_SBGGR10_1X10, V4L2_COLORSPACE_JPEG}
};
enum sensor_wq_cmd
{
    WqCmd_af_init,
    WqCmd_af_single,
    WqCmd_af_special_pos,
    WqCmd_af_far_pos,
    WqCmd_af_near_pos,
    WqCmd_af_continues,
    WqCmd_af_update_zone
};
enum sensor_wq_result
{
    WqRet_success = 0,
    WqRet_fail = -1,
    WqRet_inval = -2
};
struct sensor_work
{
	struct i2c_client *client;
	struct delayed_work dwork;
	enum sensor_wq_cmd cmd;
    wait_queue_head_t done;
    enum sensor_wq_result result;
    bool wait;
    int var;    
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
	int affm_reinit;
    int flash;
    int exposure;
    unsigned char mirror;                                        /* HFLIP */
    unsigned char flip;                                          /* VFLIP */
	bool snap2preview;
	bool video2preview;
    struct reginfo *winseqe_cur_addr;
	struct sensor_datafmt fmt;
	unsigned int enable;
	unsigned int funmodule_state;
} sensor_info_priv_t;



struct sensor_parameter
{
	unsigned short int preview_maxlines;
	unsigned short int preview_exposure;
	unsigned short int preview_line_width;
	unsigned short int preview_gain;

	unsigned short int capture_framerate;
	unsigned short int preview_framerate;
};

struct vcm_work_struct{
	int max_mft;
	int max_current;
	long engry[20];
	int engry_cnt;
		
	long EV;
	long fix_EV;
	int i_current;
	int count;

	struct i2c_client *vcm_client;
	struct work_struct vcm_work;

	int wait_flag;
	int wait_cnt;
	//struct timeval vcm_time;

};

struct ae_work_struct{
	int exposure_line;
	int gain;
	int y[150];
	int index;
	int max_time;
	int band_line;
	
	int stable_flag; //0-unstable  1-stable
	int lumi;
	int compensate;
	int n;
	int count;
	int unstable_count;
	int compensate_flag;
};


struct awb_info_s {
    unsigned short int r_gain;
    unsigned short int g_gain;
    unsigned short int b_gain;
};

struct sensor
{
    struct v4l2_subdev subdev;
    struct i2c_client *client;
    sensor_info_priv_t info_priv;
	struct sensor_parameter parameter;
	struct workqueue_struct *sensor_wq;
	
	#if CONFIG_SENSOR_Focus/* oyyf@rockchip.com:support vcm motor for ov5647*/
	struct vcm_work_struct vcm_work_wait;
	struct workqueue_struct *vcm_wq;
	#endif

	struct ae_work_struct ae_work;
	struct awb_info_s awb_info;
     
	struct mutex wq_lock;
    int model;	/* V4L2_IDENT_OV* codes from v4l2-chip-ident.h */
#if CONFIG_SENSOR_I2C_NOSCHED
	atomic_t tasklock_cnt; 
#endif
	struct rk29camera_platform_data *sensor_io_request;
    struct rk29camera_gpio_res *sensor_gpio_res;

};

static struct sensor* to_sensor(const struct i2c_client *client)
{
    return container_of(i2c_get_clientdata(client), struct sensor, subdev);
}

static int sensor_task_lock(struct i2c_client *client, int lock)
{
#if CONFIG_SENSOR_I2C_NOSCHED
	int cnt = 3;
    struct sensor *sensor = to_sensor(client);

	if (lock) {
		if (atomic_read(&sensor->tasklock_cnt) == 0) {
			while ((atomic_read(&client->adapter->bus_lock.count) < 1) && (cnt>0)) {
				SENSOR_TR("\n %s will obtain i2c in atomic, but i2c bus is locked! Wait...\n",SENSOR_NAME_STRING());
				msleep(35);
				cnt--;
			}
			if ((atomic_read(&client->adapter->bus_lock.count) < 1) && (cnt<=0)) {
				SENSOR_TR("\n %s obtain i2c fail in atomic!!\n",SENSOR_NAME_STRING());
				goto sensor_task_lock_err;
			}
			preempt_disable();
		}

		atomic_add(1, &sensor->tasklock_cnt);
	} else {
		if (atomic_read(&sensor->tasklock_cnt) > 0) {
			atomic_sub(1, &sensor->tasklock_cnt);

			if (atomic_read(&sensor->tasklock_cnt) == 0)
				preempt_enable();
		}
	}
	return 0;
sensor_task_lock_err:
	return -1;   
#else
    return 0;
#endif

}

/* sensor register write */
static int sensor_write(struct i2c_client *client, u16 reg, u8 val)
{
    int err=0,cnt;
    u8 buf[3];
    struct i2c_msg msg[1];

    switch (reg)
	{
		case SEQUENCE_WAIT_MS:
		{
			if (in_atomic())
				mdelay(val);
			else
				msleep(val);
			break;
		}

		case SEQUENCE_WAIT_US:
		{
			udelay(val);
			break;
		}
		case SEQUENCE_PROPERTY:
		{
			break;
		}
		default:
		{
		    buf[0] = reg >> 8;
            buf[1] = reg & 0xFF;
            buf[2] = val;

            msg->addr = client->addr;
            msg->flags = client->flags;
            msg->buf = buf;
            msg->len = sizeof(buf);
            msg->scl_rate = CONFIG_SENSOR_I2C_SPEED;         /* ddl@rock-chips.com : 100kHz */
            msg->read_type = 0;               /* fpga i2c:0==I2C_NORMAL : direct use number not enum for don't want include spi_fpga.h */

            cnt = 3;
            err = -EAGAIN;

            while ((cnt-- > 0) && (err < 0)) {                       /* ddl@rock-chips.com :  Transfer again if transent is failed   */
                err = i2c_transfer(client->adapter, msg, 1);

                if (err >= 0) {
                    return 0;
                } else {
                    SENSOR_TR("\n %s write reg(0x%x, val:0x%x) failed, try to write again!\n",SENSOR_NAME_STRING(),reg, val);
                    udelay(10);
                }
            }
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

    cnt = 3;
    err = -EAGAIN;
    while ((cnt-- > 0) && (err < 0)) {                       /* ddl@rock-chips.com :  Transfer again if transent is failed   */
        err = i2c_transfer(client->adapter, msg, 2);

        if (err >= 0) {
            *val = buf[0];
            return 0;
        } else {
        	SENSOR_TR("\n %s read reg(0x%x val:0x%x) failed, try to read again! \n",SENSOR_NAME_STRING(),reg, *val);
            udelay(10);
        }
    }

    return err;
}

/* write a array of registers  */
static int sensor_write_array(struct i2c_client *client, struct reginfo *regarray)
{
    int err = 0, cnt;
    int i = 0;
#if CONFIG_SENSOR_I2C_RDWRCHK
	char valchk;
#endif

	cnt = 0;
	if (sensor_task_lock(client, 1) < 0)
		goto sensor_write_array_end;
    while (regarray[i].reg != SEQUENCE_END)
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
                err = -EPERM;
				goto sensor_write_array_end;
            }
        } else {
        #if CONFIG_SENSOR_I2C_RDWRCHK
			sensor_read(client, regarray[i].reg, &valchk);
			if (valchk != regarray[i].val)
				SENSOR_TR("%s Reg:0x%x write(0x%x, 0x%x) fail\n",SENSOR_NAME_STRING(), regarray[i].reg, regarray[i].val, valchk);
		#endif
        }

        i++;
    }

sensor_write_array_end:
	sensor_task_lock(client,0);
    return err;
}
#if CONFIG_SENSOR_I2C_RDWRCHK
static int sensor_readchk_array(struct i2c_client *client, struct reginfo *regarray)
{
    int cnt;
    int i = 0;
	char valchk;

	cnt = 0;
	valchk = 0;
    while (regarray[i].reg != 0)
    {
		sensor_read(client, regarray[i].reg, &valchk);
		if (valchk != regarray[i].val)
			SENSOR_TR("%s Reg:0x%x read(0x%x, 0x%x) error\n",SENSOR_NAME_STRING(), regarray[i].reg, regarray[i].val, valchk);

        i++;
    }
    return 0;
}
#endif

static int sensor_parameter_record(struct i2c_client *client)
{
	u8 ret_l,ret_m,ret_h;
	u8 tp_l,tp_m,tp_h;
	struct sensor *sensor = to_sensor(client);

	sensor_write(client,0x3503,0x07);	//stop AE/AG
	sensor_write(client,0x3406,0x01);   //stop AWB

	sensor_read(client,0x3500,&ret_h);
	sensor_read(client,0x3501, &ret_m);
	sensor_read(client,0x3502, &ret_l);
	tp_l = ret_l;
	tp_m = ret_m;
	tp_h = ret_h;
	SENSOR_DG(" %s Read 0x3500 = 0x%02x  0x3501 = 0x%02x 0x3502=0x%02x \n",SENSOR_NAME_STRING(), ret_h, ret_m, ret_l);
	sensor->parameter.preview_exposure = (tp_h<<12)+(tp_m<<4)+(tp_l>>4);
	sensor_read(client,0x350c, &ret_h);
	sensor_read(client,0x350d, &ret_l);
	sensor->parameter.preview_line_width = ret_h & 0xff;
	sensor->parameter.preview_line_width = (sensor->parameter.preview_line_width << 8) +ret_l;
	//Read back AGC Gain for preview
	sensor_read(client,0x350b, &tp_l);
	sensor->parameter.preview_gain = tp_l;

	sensor->parameter.capture_framerate = 900;
	sensor->parameter.preview_framerate = 1500;

	SENSOR_DG(" %s Read 0x350c = 0x%02x  0x350d = 0x%02x 0x350b=0x%02x \n",SENSOR_NAME_STRING(), ret_h, ret_l, sensor->parameter.preview_gain);
	return 0;
}
static int sensor_ae_transfer(struct i2c_client *client)
{
	u8  ExposureLow;
	u8  ExposureMid;
	u8  ExposureHigh;
	u16 ulCapture_Exposure;
	u32 ulCapture_Exposure_Gain;
	u16  iCapture_Gain;
	u8   Lines_10ms;
	bool m_60Hz = 0;
	u8  reg_l = 0,reg_h =0;
	u16 Preview_Maxlines;
	u8  Gain;
	u32  Capture_MaxLines;
	struct sensor *sensor = to_sensor(client);

	Preview_Maxlines = sensor->parameter.preview_line_width;
	Gain = sensor->parameter.preview_gain;
	sensor_read(client,0x350c, &reg_h);
	sensor_read(client,0x350d, &reg_l);
	Capture_MaxLines = reg_h & 0xff;
	Capture_MaxLines = (Capture_MaxLines << 8) + reg_l;

	if(m_60Hz== 1) {
		Lines_10ms = sensor->parameter.capture_framerate * Capture_MaxLines/12000;
	} else {
		Lines_10ms = sensor->parameter.capture_framerate * Capture_MaxLines/10000;
	}

	if(Preview_Maxlines == 0)
		Preview_Maxlines = 1;

	ulCapture_Exposure =
		(sensor->parameter.preview_exposure*(sensor->parameter.capture_framerate)*(Capture_MaxLines))/(((Preview_Maxlines)*(sensor->parameter.preview_framerate)));
	iCapture_Gain = (Gain & 0x0f) + 16;
	if (Gain & 0x10) {
		iCapture_Gain = iCapture_Gain << 1;
	}
	if (Gain & 0x20) {
		iCapture_Gain = iCapture_Gain << 1;
	}
	if (Gain & 0x40) {
		iCapture_Gain = iCapture_Gain << 1;
	}
	if (Gain & 0x80) {
		iCapture_Gain = iCapture_Gain << 1;
	}
	ulCapture_Exposure_Gain =(u32) (11 * ulCapture_Exposure * iCapture_Gain/5);   //0ld value 2.5, 解决过亮
	if(ulCapture_Exposure_Gain < Capture_MaxLines*16) {
		ulCapture_Exposure = ulCapture_Exposure_Gain/16;
		if (ulCapture_Exposure > Lines_10ms)
		{
			//ulCapture_Exposure *= 1.7;
		 	ulCapture_Exposure /= Lines_10ms;
		 	ulCapture_Exposure *= Lines_10ms;
		}
	} else {
		ulCapture_Exposure = Capture_MaxLines;
		//ulCapture_Exposure_Gain *= 1.5;
	}
	if(ulCapture_Exposure == 0)
		ulCapture_Exposure = 1;
	iCapture_Gain = (ulCapture_Exposure_Gain*2/ulCapture_Exposure + 1)/2;
	ExposureLow = ((unsigned char)ulCapture_Exposure)<<4;
	ExposureMid = (unsigned char)(ulCapture_Exposure >> 4) & 0xff;
	ExposureHigh = (unsigned char)(ulCapture_Exposure >> 12);

	Gain = 0;
	if (iCapture_Gain > 31) {
		Gain |= 0x10;
		iCapture_Gain = iCapture_Gain >> 1;
	}
	if (iCapture_Gain > 31) {
		Gain |= 0x20;
		iCapture_Gain = iCapture_Gain >> 1;
	}
	if (iCapture_Gain > 31) {
		Gain |= 0x40;
		iCapture_Gain = iCapture_Gain >> 1;
	}
	if (iCapture_Gain > 31) {
		Gain |= 0x80;
		iCapture_Gain = iCapture_Gain >> 1;
	}
	if (iCapture_Gain > 16)
		Gain |= ((iCapture_Gain -16) & 0x0f);
	if(Gain == 0x10)
		Gain = 0x11;
	// write the gain and exposure to 0x350* registers
	//m_iWrite0x350b=Gain;
	sensor_write(client,0x350b, Gain);
	//m_iWrite0x3502=ExposureLow;
	sensor_write(client,0x3502, ExposureLow);
	//m_iWrite0x3501=ExposureMid;
	sensor_write(client,0x3501, ExposureMid);
	//m_iWrite0x3500=ExposureHigh;
	sensor_write(client,0x3500, ExposureHigh);
	// SendToFile("Gain = 0x%x\r\n", Gain);
	// SendToFile("ExposureLow = 0x%x\r\n", ExposureLow);
	// SendToFile("ExposureMid = 0x%x\r\n", ExposureMid);
	// SendToFile("ExposureHigh = 0x%x\r\n", ExposureHigh);
	//加长延时，避免暗处拍照时的明暗分界问题
	//camera_timed_wait(200);
	//linzhk camera_timed_wait(500);

	SENSOR_DG(" %s Write 0x350b = 0x%02x  0x3502 = 0x%02x 0x3501=0x%02x 0x3500 = 0x%02x\n",SENSOR_NAME_STRING(), Gain, ExposureLow, ExposureMid, ExposureHigh);
	mdelay(100);
	return 0;
}

static void sensor_rgb_gain(struct i2c_client* client, int cmd, unsigned short int *val)
{
    char read_val;
    
    if (cmd & 0x80) {
        sensor_read(client, SENSOR_R_GAIN_MSB+((cmd&0x7f)<<1), &read_val);
        *val = read_val;
        sensor_read(client, SENSOR_R_GAIN_LSB+((cmd&0x7f)<<1), &read_val);
        *val = read_val | (*val<<8);
    } else {
        sensor_write(client, SENSOR_R_GAIN_MSB+((cmd&0x7f)<<1), (*val&0xff00)>>8);
        sensor_write(client, SENSOR_R_GAIN_LSB+((cmd&0x7f)<<1), *val&0xff);
    }
}

static int sensor_ioctrl(struct soc_camera_device *icd,enum rk29sensor_power_cmd cmd, int on)
{
	struct soc_camera_link *icl = to_soc_camera_link(icd);
	int ret = 0;

    SENSOR_DG("%s %s  cmd(%d) on(%d)\n",SENSOR_NAME_STRING(),__FUNCTION__,cmd,on);

	switch (cmd)
	{
		case Sensor_PowerDown:
		{
			if (icl->powerdown) {
				ret = icl->powerdown(icd->pdev, on);
				if (ret == RK29_CAM_IO_SUCCESS) {
					if (on == 0) {
						mdelay(2);
						if (icl->reset)
							icl->reset(icd->pdev);
					}
				} else if (ret == RK29_CAM_EIO_REQUESTFAIL) {
					ret = -ENODEV;
					goto sensor_power_end;
				}
			}
			break;
		}
		case Sensor_Flash:
		{
			struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
    		struct sensor *sensor = to_sensor(client);

			if (sensor->sensor_io_request && sensor->sensor_io_request->sensor_ioctrl) {
				sensor->sensor_io_request->sensor_ioctrl(icd->pdev,Cam_Flash, on);
			}
			break;
		}
		default:
		{
			SENSOR_TR("%s cmd(0x%x) is unknown!",SENSOR_NAME_STRING(),cmd);
			break;
		}
	}

sensor_power_end:
	return ret;
}
static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct soc_camera_device *icd = client->dev.platform_data;
    struct sensor *sensor = to_sensor(client);
	const struct v4l2_queryctrl *qctrl;
    const struct sensor_datafmt *fmt;
    char value;
    int ret,pid = 0;

    SENSOR_DG("\n%s..%s.. \n",SENSOR_NAME_STRING(),__FUNCTION__);

	if (sensor_ioctrl(icd, Sensor_PowerDown, 0) < 0) {
		ret = -ENODEV;
        SENSOR_TR("%s turn off PowerDown failed!\n",SENSOR_NAME_STRING());
		goto sensor_INIT_ERR;
	}

    /* soft reset */
	if (sensor_task_lock(client,1)<0)
		goto sensor_INIT_ERR;
    ret = sensor_write(client, 0x0103, 0x01);
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
    SENSOR_DG("\n %s  pid = 0x%x \n", SENSOR_NAME_STRING(), pid);

    if (pid == SENSOR_ID) {
        sensor->model = SENSOR_V4L2_IDENT;
    } else {
        SENSOR_TR("error: %s mismatched   pid = 0x%x\n", SENSOR_NAME_STRING(), pid);
        ret = -ENODEV;
        goto sensor_INIT_ERR;
    }

    ret = sensor_write_array(client, sensor_init_data);
    if (ret != 0) {
        SENSOR_TR("error: %s initial failed\n",SENSOR_NAME_STRING());
        goto sensor_INIT_ERR;
    }
	sensor_task_lock(client,0);
    sensor->info_priv.winseqe_cur_addr  = SENSOR_INIT_WINSEQADR;
    fmt = sensor_find_datafmt(SENSOR_INIT_PIXFMT,sensor_colour_fmts, ARRAY_SIZE(sensor_colour_fmts));
    if (!fmt) {
        SENSOR_TR("error: %s initial array colour fmts(0x%x) is not support!!",SENSOR_NAME_STRING(),SENSOR_INIT_PIXFMT);
        ret = -EINVAL;
        goto sensor_INIT_ERR;
    }
	sensor->info_priv.fmt = *fmt;

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
	qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_FOCUS_ABSOLUTE);
	if (qctrl)
        sensor->info_priv.focus = qctrl->default_value;

	#if CONFIG_SENSOR_Flash
	qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_FLASH);
	if (qctrl)
        sensor->info_priv.flash = qctrl->default_value;
    #endif
    SENSOR_DG("\n%s..%s.. icd->width = %d.. icd->height %d\n",SENSOR_NAME_STRING(),((val == 0)?__FUNCTION__:"sensor_reinit"),icd->user_width,icd->user_height);

    sensor_rgb_gain(client,SENSOR_G_R_GAIN, &sensor->awb_info.r_gain);
    sensor_rgb_gain(client,SENSOR_G_G_GAIN, &sensor->awb_info.g_gain);
    sensor_rgb_gain(client,SENSOR_G_B_GAIN, &sensor->awb_info.b_gain);
    sensor->ae_work.n = AE_INDEX_INIT;
	sensor->ae_work.exposure_line = AE_VALUE_INIT;
	sensor->ae_work.gain = AG_VALUE_INIT;
	gamma_init();
	//ccm_matrix_init();
    sensor->info_priv.funmodule_state = SENSOR_INIT_IS_OK;

#if CONFIG_SENSOR_Focus
    if (sensor->vcm_work_wait.vcm_client && sensor->vcm_work_wait.vcm_client->driver)
        sensor->info_priv.funmodule_state |= SENSOR_AF_IS_OK;
#endif
        
    return 0;
sensor_INIT_ERR:
    sensor->info_priv.funmodule_state &= ~SENSOR_INIT_IS_OK;
	sensor_task_lock(client,0);
	sensor_deactivate(client);
    return ret;
}
static int sensor_deactivate(struct i2c_client *client)
{
	struct soc_camera_device *icd = client->dev.platform_data;
    struct sensor *sensor = to_sensor(client);
    u8 reg_val;
	
	SENSOR_DG("\n%s..%s.. Enter\n",SENSOR_NAME_STRING(),__FUNCTION__);    
    
	/* ddl@rock-chips.com : all sensor output pin must change to input for other sensor */
	if (sensor->info_priv.funmodule_state & SENSOR_INIT_IS_OK) {
		sensor_read(client,0x3000,&reg_val);
		sensor_write(client, 0x3000, reg_val&0xfc);
		sensor_write(client, 0x3001, 0x00);
		sensor_write(client, 0x3002, 0x00);
		sensor->info_priv.funmodule_state &= ~SENSOR_INIT_IS_OK;
	}
    sensor_ioctrl(icd, Sensor_PowerDown, 1);
    msleep(100); 
    
	/* ddl@rock-chips.com : sensor config init width , because next open sensor quickly(soc_camera_open -> Try to configure with default parameters) */
	icd->user_width = SENSOR_INIT_WIDTH;
    icd->user_height = SENSOR_INIT_HEIGHT;
    sensor->info_priv.funmodule_state = 0;
	return 0;
}
static  struct reginfo sensor_power_down_sequence[]=
{
    {0x00,0x00}
};
static int sensor_suspend(struct soc_camera_device *icd, pm_message_t pm_msg)
{
    int ret;
    struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

    if (pm_msg.event == PM_EVENT_SUSPEND) {
        SENSOR_DG("\n %s Enter Suspend.. \n", SENSOR_NAME_STRING());
        ret = sensor_write_array(client, sensor_power_down_sequence) ;
        if (ret != 0) {
            SENSOR_TR("\n %s..%s WriteReg Fail.. \n", SENSOR_NAME_STRING(),__FUNCTION__);
            return ret;
        } else {
            ret = sensor_ioctrl(icd, Sensor_PowerDown, 1);
            if (ret < 0) {
			    SENSOR_TR("\n %s suspend fail for turn on power!\n", SENSOR_NAME_STRING());
                return -EINVAL;
            }
        }
    } else {
        SENSOR_TR("\n %s cann't suppout Suspend..\n",SENSOR_NAME_STRING());
        return -EINVAL;
    }

    return 0;
}

static int sensor_resume(struct soc_camera_device *icd)
{
	int ret;

    ret = sensor_ioctrl(icd, Sensor_PowerDown, 0);
    if (ret < 0) {
		SENSOR_TR("\n %s resume fail for turn on power!\n", SENSOR_NAME_STRING());
        return -EINVAL;
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
static int sensor_g_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct soc_camera_device *icd = client->dev.platform_data;
    struct sensor *sensor = to_sensor(client);

    mf->width	= icd->user_width;
	mf->height	= icd->user_height;
	mf->code	= sensor->info_priv.fmt.code;
	mf->colorspace	= sensor->info_priv.fmt.colorspace;
	mf->field	= V4L2_FIELD_NONE;

    return 0;
}
static bool sensor_fmt_capturechk(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{
    bool ret = false;

	if ((mf->width == 1024) && (mf->height == 768)) {
		ret = true;
	} else if ((mf->width == 1280) && (mf->height == 1024)) {
		ret = true;
	} else if ((mf->width == 1600) && (mf->height == 1200)) {
		ret = true;
	} else if ((mf->width == 2048) && (mf->height == 1536)) {
		ret = true;
	} else if ((mf->width == 2592) && (mf->height == 1944)) {
		ret = true;
	}

	if (ret == true)
		SENSOR_DG("%s %dx%d is capture format\n", __FUNCTION__, mf->width, mf->height);
	return ret;
}

static bool sensor_fmt_videochk(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{
    bool ret = false;

	if ((mf->width == 1280) && (mf->height == 720)) {
		ret = true;
	} else if ((mf->width == 1920) && (mf->height == 1080)) {
		ret = true;
	}

	if (ret == true)
		SENSOR_DG("%s %dx%d is video format\n", __FUNCTION__, mf->width, mf->height);
	return ret;
}
static int sensor_s_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    const struct sensor_datafmt *fmt;
    struct sensor *sensor = to_sensor(client);
	const struct v4l2_queryctrl *qctrl;
	struct soc_camera_device *icd = client->dev.platform_data;
    struct reginfo *winseqe_set_addr=NULL;
    int ret = 0, set_w,set_h;

	fmt = sensor_find_datafmt(mf->code, sensor_colour_fmts,
				   ARRAY_SIZE(sensor_colour_fmts));
	if (!fmt) {
        ret = -EINVAL;
        goto sensor_s_fmt_end;
    }

	if (sensor->info_priv.fmt.code != mf->code) {
		switch (mf->code)
		{
			case V4L2_MBUS_FMT_YUYV8_2X8:
			{
				winseqe_set_addr = sensor_ClrFmt_YUYV;
				break;
			}
			case V4L2_MBUS_FMT_UYVY8_2X8:
			{
				winseqe_set_addr = sensor_ClrFmt_UYVY;
				break;
			}
			default:
				break;
		}
		if (winseqe_set_addr != NULL) {
            sensor_write_array(client, winseqe_set_addr);
			sensor->info_priv.fmt.code = mf->code;
            sensor->info_priv.fmt.colorspace= mf->colorspace;            
			SENSOR_DG("%s v4l2_mbus_code:%d set success!\n", SENSOR_NAME_STRING(),mf->code);
		} else {
			SENSOR_TR("%s v4l2_mbus_code:%d is invalidate!\n", SENSOR_NAME_STRING(),mf->code);
		}
	}

    set_w = mf->width;
    set_h = mf->height;

	if (((set_w <= 176) && (set_h <= 144)) && (sensor_qcif[0].reg != SEQUENCE_END))
	{
		winseqe_set_addr = sensor_qcif;
        set_w = 176;
        set_h = 144;
	}
	else if (((set_w <= 320) && (set_h <= 240)) && (sensor_qvga[0].reg != SEQUENCE_END))
    {
        winseqe_set_addr = sensor_qvga;
        set_w = 320;
        set_h = 240;
    }
    else if (((set_w <= 352) && (set_h<= 288)) && (sensor_cif[0].reg != SEQUENCE_END))
    {
        winseqe_set_addr = sensor_cif;
        set_w = 352;
        set_h = 288;
    }
    else if (((set_w <= 640) && (set_h <= 480)) && (sensor_vga[0].reg != SEQUENCE_END))
    {
        winseqe_set_addr = sensor_vga;
        set_w = 640;
        set_h = 480;
    }
    else if (((set_w <= 800) && (set_h <= 600)) && (sensor_svga[0].reg != SEQUENCE_END))
    {
        winseqe_set_addr = sensor_svga;
        set_w = 800;
        set_h = 600;
    }
	else if (((set_w <= 1024) && (set_h <= 768)) && (sensor_xga[0].reg != SEQUENCE_END))
    {
        winseqe_set_addr = sensor_xga;
        set_w = 1024;
        set_h = 768;
    }
	else if (((set_w <= 1280) && (set_h <= 720)) && (sensor_720p[0].reg != SEQUENCE_END))
    {
        winseqe_set_addr = sensor_720p;
        set_w = 1280;
        set_h = 720;
    }
    else if (((set_w <= 1280) && (set_h <= 960)) && (sensor_960p[0].reg != SEQUENCE_END) )
    {
        winseqe_set_addr = sensor_960p;
        set_w = 1280;
        set_h = 960;
    }
    else if (((set_w <= 1280) && (set_h <= 1024)) && (sensor_sxga[0].reg != SEQUENCE_END))
    {
        winseqe_set_addr = sensor_sxga;
        set_w = 1280;
        set_h = 1024;
    }
    else if (((set_w <= 1600) && (set_h <= 1200)) && (sensor_uxga[0].reg != SEQUENCE_END))
    {
        winseqe_set_addr = sensor_uxga;
        set_w = 1600;
        set_h = 1200;
    }
    else if (((set_w <= 1920) && (set_h <= 1080)) && (sensor_1080p[0].reg != SEQUENCE_END))
    {
        winseqe_set_addr = sensor_1080p;
        set_w = 1920;
        set_h = 1080;
    }
	else if (((set_w <= 2048) && (set_h <= 1536)) && (sensor_qxga[0].reg != SEQUENCE_END))
    {
        winseqe_set_addr = sensor_qxga;
        set_w = 2048;
        set_h = 1536;
    }
	else if (((set_w <= 2592) && (set_h <= 1944)) && (sensor_qsxga[0].reg != SEQUENCE_END))
    {
        winseqe_set_addr = sensor_qsxga;
        set_w = 2592;
        set_h = 1944;
    }
    else
    {
        winseqe_set_addr = SENSOR_INIT_WINSEQADR;               /* ddl@rock-chips.com : Sensor output smallest size if  isn't support app  */
        set_w = SENSOR_INIT_WIDTH;
        set_h = SENSOR_INIT_HEIGHT;
		SENSOR_TR("\n %s..%s..%d Format is Invalidate. pix->width = %d.. pix->height = %d\n",SENSOR_NAME_STRING(),__FUNCTION__,__LINE__,mf->width,mf->height);
    }

    if (winseqe_set_addr  != sensor->info_priv.winseqe_cur_addr)
    {
		if (sensor_fmt_capturechk(sd,mf) == true) {					/* ddl@rock-chips.com : Capture */
			sensor_parameter_record(client);
        #if CONFIG_SENSOR_Flash
            if ((sensor->info_priv.flash == 1) || (sensor->info_priv.flash == 2)) {
                sensor_ioctrl(icd, Sensor_Flash, Flash_On);
                SENSOR_DG("%s flash on in capture!\n", SENSOR_NAME_STRING());
            }
        #endif
		}else {                                        /* ddl@rock-chips.com : Video */
		#if CONFIG_SENSOR_Flash
            if ((sensor->info_priv.flash == 1) || (sensor->info_priv.flash == 2)) {
                sensor_ioctrl(icd, Sensor_Flash, Flash_Off);
                SENSOR_DG("%s flash off in preivew!\n", SENSOR_NAME_STRING());
            }
        #endif
        }

		if ((sensor->info_priv.winseqe_cur_addr->reg == SEQUENCE_PROPERTY) && (sensor->info_priv.winseqe_cur_addr->val == SEQUENCE_INIT)) {
			if (((winseqe_set_addr->reg == SEQUENCE_PROPERTY) && (winseqe_set_addr->val == SEQUENCE_NORMAL))
				|| (winseqe_set_addr->reg != SEQUENCE_PROPERTY)) {
				ret |= sensor_write_array(client,sensor_init_data);
				SENSOR_DG("\n%s reinit ret:0x%x \n",SENSOR_NAME_STRING(), ret);
			}
		}

        ret |= sensor_write_array(client, winseqe_set_addr);
        if (ret != 0) {
            SENSOR_TR("%s set format capability failed\n", SENSOR_NAME_STRING());
            #if CONFIG_SENSOR_Flash
            if (sensor_fmt_capturechk(sd,mf) == true) {
                if ((sensor->info_priv.flash == 1) || (sensor->info_priv.flash == 2)) {
                    sensor_ioctrl(icd, Sensor_Flash, Flash_Off);
                    SENSOR_TR("%s Capture format set fail, flash off !\n", SENSOR_NAME_STRING());
                }
            }
            #endif
            goto sensor_s_fmt_end;
        }

        sensor->info_priv.winseqe_cur_addr  = winseqe_set_addr;

		if (sensor_fmt_capturechk(sd,mf) == true) {				    /* ddl@rock-chips.com : Capture */
			sensor_ae_transfer(client);
			qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_EFFECT);
			sensor_set_effect(icd, qctrl,sensor->info_priv.effect);
			if (sensor->info_priv.whiteBalance != 0) {
				qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_DO_WHITE_BALANCE);
				sensor_set_whiteBalance(icd, qctrl,sensor->info_priv.whiteBalance);
			}
			
			sensor->info_priv.snap2preview = true;
		} else if (sensor_fmt_videochk(sd,mf) == true) {			/* ddl@rock-chips.com : Video */
			qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_EFFECT);
			sensor_set_effect(icd, qctrl,sensor->info_priv.effect);
            
			sensor->info_priv.video2preview = true;
		} else if ((sensor->info_priv.snap2preview == true) || (sensor->info_priv.video2preview == true)) {
			qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_EFFECT);
			sensor_set_effect(icd, qctrl,sensor->info_priv.effect);
			if (sensor->info_priv.snap2preview == true) {
				qctrl = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_DO_WHITE_BALANCE);
				sensor_set_whiteBalance(icd, qctrl,sensor->info_priv.whiteBalance);
			}
            
			sensor->info_priv.video2preview = false;
			sensor->info_priv.snap2preview = false;
		}

        
		
        SENSOR_DG("\n%s..%s..%d.. icd->width = %d.. icd->height %d\n",SENSOR_NAME_STRING(),__FUNCTION__,__LINE__,set_w,set_h);
    }
    else
    {
        SENSOR_DG("\n %s..%s..%d.. Current Format is validate. icd->width = %d.. icd->height %d\n",SENSOR_NAME_STRING(),__FUNCTION__, __LINE__,set_w,set_h);
    }

	mf->width = set_w;
	mf->height = set_h;
sensor_s_fmt_end:
    return ret;
}

static int sensor_try_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct sensor *sensor = to_sensor(client);
    const struct sensor_datafmt *fmt;
    int ret = 0,set_w,set_h;
   
	fmt = sensor_find_datafmt(mf->code, sensor_colour_fmts,
				   ARRAY_SIZE(sensor_colour_fmts));
	if (fmt == NULL) {
		fmt = &sensor->info_priv.fmt;
        mf->code = fmt->code;
	} 

    if (mf->height > SENSOR_MAX_HEIGHT)
        mf->height = SENSOR_MAX_HEIGHT;
    else if (mf->height < SENSOR_MIN_HEIGHT)
        mf->height = SENSOR_MIN_HEIGHT;

    if (mf->width > SENSOR_MAX_WIDTH)
        mf->width = SENSOR_MAX_WIDTH;
    else if (mf->width < SENSOR_MIN_WIDTH)
        mf->width = SENSOR_MIN_WIDTH;

    set_w = mf->width;
    set_h = mf->height;

	if (((set_w <= 176) && (set_h <= 144)) && (sensor_qcif[0].reg != SEQUENCE_END))
	{
        set_w = 176;
        set_h = 144;
	}
	else if (((set_w <= 320) && (set_h <= 240)) && (sensor_qvga[0].reg != SEQUENCE_END))
    {
        set_w = 320;
        set_h = 240;
    }
    else if (((set_w <= 352) && (set_h<= 288)) && (sensor_cif[0].reg != SEQUENCE_END))
    {
        set_w = 352;
        set_h = 288;
    }
    else if (((set_w <= 640) && (set_h <= 480)) && (sensor_vga[0].reg != SEQUENCE_END))
    {
        set_w = 640;
        set_h = 480;
    }
    else if (((set_w <= 800) && (set_h <= 600)) && (sensor_svga[0].reg != SEQUENCE_END))
    {
        set_w = 800;
        set_h = 600;
    }
	else if (((set_w <= 1024) && (set_h <= 768)) && (sensor_xga[0].reg != SEQUENCE_END))
    {
        set_w = 1024;
        set_h = 768;
    }
	else if (((set_w <= 1280) && (set_h <= 720)) && (sensor_720p[0].reg != SEQUENCE_END))
    {
        set_w = 1280;
        set_h = 720;
    }
    else if (((set_w <= 1280) && (set_h <= 960)) && (sensor_960p[0].reg != SEQUENCE_END))
    {
        set_w = 1280;
        set_h = 960;
    }
    else if (((set_w <= 1280) && (set_h <= 1024)) && (sensor_sxga[0].reg != SEQUENCE_END))
    {
        set_w = 1280;
        set_h = 1024;
    }
    else if (((set_w <= 1600) && (set_h <= 1200)) && (sensor_uxga[0].reg != SEQUENCE_END))
    {
        set_w = 1600;
        set_h = 1200;
    }
    else if (((set_w <= 1920) && (set_h <= 1080)) && (sensor_1080p[0].reg != SEQUENCE_END))
    {
        set_w = 1920;
        set_h = 1080;
    }
	else if (((set_w <= 2048) && (set_h <= 1536)) && (sensor_qxga[0].reg != SEQUENCE_END))
    {
        set_w = 2048;
        set_h = 1536;
    }
	else if (((set_w <= 2592) && (set_h <= 1944)) && (sensor_qsxga[0].reg != SEQUENCE_END))
    {
        set_w = 2592;
        set_h = 1944;
    }
    else
    {
        set_w = SENSOR_INIT_WIDTH;
        set_h = SENSOR_INIT_HEIGHT;
    }

    mf->width = set_w;
    mf->height = set_h;
    
    mf->colorspace = fmt->colorspace;
    
    return ret;
}

 static int sensor_g_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *id)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);

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
	int ret = 0;

	qctrl_info = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_FOCUS_ABSOLUTE);
	if (!qctrl_info)
		return -EINVAL;
    
	if ((sensor->info_priv.funmodule_state & SENSOR_AF_IS_OK) && (sensor->info_priv.affm_reinit == 0)) {
		if ((value >= qctrl_info->minimum) && (value <= qctrl_info->maximum)) {
            //ret = sensor_af_workqueue_set(icd, WqCmd_af_special_pos, value, true);
			SENSOR_DG("%s..%s : %d  ret:0x%x\n",SENSOR_NAME_STRING(),__FUNCTION__, value,ret);
		} else {
			ret = -EINVAL;
			SENSOR_TR("\n %s..%s valure = %d is invalidate..    \n",SENSOR_NAME_STRING(),__FUNCTION__,value);
		}
	} else {
		ret = -EACCES;
		SENSOR_TR("\n %s..%s AF module state(0x%x, 0x%x) is error!\n",SENSOR_NAME_STRING(),__FUNCTION__,
			sensor->info_priv.funmodule_state,sensor->info_priv.affm_reinit);
	}

	return ret;
}
static int sensor_set_focus_relative(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
	struct sensor *sensor = to_sensor(client);
	const struct v4l2_queryctrl *qctrl_info;
	int ret = 0;

	qctrl_info = soc_camera_find_qctrl(&sensor_ops, V4L2_CID_FOCUS_RELATIVE);
	if (!qctrl_info)
		return -EINVAL;    

	if ((sensor->info_priv.funmodule_state & SENSOR_AF_IS_OK) && (sensor->info_priv.affm_reinit == 0)) {
		if ((value >= qctrl_info->minimum) && (value <= qctrl_info->maximum)) {            
            //if (value > 0) {
                //ret = sensor_af_workqueue_set(icd, WqCmd_af_near_pos, 0, true);
            //} else {
                //ret = sensor_af_workqueue_set(icd, WqCmd_af_far_pos, 0, true);
            //}
			SENSOR_DG("%s..%s : %d  ret:0x%x\n",SENSOR_NAME_STRING(),__FUNCTION__, value,ret);
		} else {
			ret = -EINVAL;
			SENSOR_TR("\n %s..%s valure = %d is invalidate..    \n",SENSOR_NAME_STRING(),__FUNCTION__,value);
		}
	} else {
		ret = -EACCES;
		SENSOR_TR("\n %s..%s AF module state(0x%x, 0x%x) is error!\n",SENSOR_NAME_STRING(),__FUNCTION__,
			sensor->info_priv.funmodule_state,sensor->info_priv.affm_reinit);
	}
	return ret;
}

static int sensor_set_focus_mode(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
	struct sensor *sensor = to_sensor(client); 
	int ret = 0;

	if ((sensor->info_priv.funmodule_state & SENSOR_AF_IS_OK)  && (sensor->info_priv.affm_reinit == 0)) {
		switch (value)
		{
			case SENSOR_AF_MODE_AUTO:
			{
				
				break;
			}

			case SENSOR_AF_MODE_MACRO:
			{
				ret = sensor_set_focus_absolute(icd, qctrl, 0xff);
				break;
			}

			case SENSOR_AF_MODE_INFINITY:
			{
				ret = sensor_set_focus_absolute(icd, qctrl, 0x00);
				break;
			}

			case SENSOR_AF_MODE_CONTINUOUS:
			{
				
				break;
			}
			default:
				SENSOR_TR("\n %s..%s AF value(0x%x) is error!\n",SENSOR_NAME_STRING(),__FUNCTION__,value);
				break;

		}

		SENSOR_DG("%s..%s : %d  ret:0x%x\n",SENSOR_NAME_STRING(),__FUNCTION__, value,ret);
	} else {
		ret = -EACCES;
		SENSOR_TR("\n %s..%s AF module state(0x%x, 0x%x) is error!\n",SENSOR_NAME_STRING(),__FUNCTION__,
			sensor->info_priv.funmodule_state,sensor->info_priv.affm_reinit);
	}

	return ret;
}
#endif

#if CONFIG_SENSOR_Flash
static int sensor_set_flash(struct soc_camera_device *icd, const struct v4l2_queryctrl *qctrl, int value)
{    
    if ((value >= qctrl->minimum) && (value <= qctrl->maximum)) {
        if (value == 3) {       /* ddl@rock-chips.com: torch */
            sensor_ioctrl(icd, Sensor_Flash, Flash_Torch);   /* Flash On */
        } else {
            sensor_ioctrl(icd, Sensor_Flash, Flash_Off);
        }
        SENSOR_DG("%s..%s : %d\n",SENSOR_NAME_STRING(),__FUNCTION__, value);
        return 0;
    }
    
	SENSOR_TR("\n %s..%s valure = %d is invalidate..    \n",SENSOR_NAME_STRING(),__FUNCTION__,value);
    return -EINVAL;
}
#endif

static int gamma_line_calculate(struct gamma_info_t* gamma_info)
{
	int i;
	struct gamma_line_t* line = gamma_info->gamma_line;
	int* in = gamma_info->gamma_in_array;
	int* out = gamma_info->gamma_out_array;
	

	for(i=0; i<gamma_info->gamma_count-1; i++){
		line->A = out[i+1] - out[i];
		line->B = in[i+1] - in[i];
		line->C = out[i]*in[i+1] - out[i+1]*in[i]; 
		line->A = ((line->A)<<10)/(line->B);
		line->C = ((line->C)<<10)/(line->B);
		line++;
	}
	
	return 0;
}

static int gamma_init(void)
{
	int ret = 0;
	//int i;

	sensor_gamma_info.gamma_status = 1;
	sensor_gamma_info.gamma_count = sizeof(gamma_in_array)/sizeof(int);
	if(sensor_gamma_info.gamma_count <= 0)
		sensor_gamma_info.gamma_status = 0;
	else{	
	sensor_gamma_info.gamma_in_array = gamma_in_array;
	sensor_gamma_info.gamma_out_array = gamma_out_array;
	sensor_gamma_info.gamma_line = (struct gamma_line_t*)kzalloc(sensor_gamma_info.gamma_count*sizeof(struct gamma_line_t),GFP_KERNEL);
	}
	
	if(sensor_gamma_info.gamma_line==NULL || sensor_gamma_info.gamma_in_array==NULL || sensor_gamma_info.gamma_out_array==NULL)
		sensor_gamma_info.gamma_status = 0;
		
	if(sensor_gamma_info.gamma_status != 0){
		gamma_line_calculate(&sensor_gamma_info);
	}

	#if 0
	for(i=0; i< sensor_gamma_info.gamma_count; i++){
		printk("in(%d) out(%d)\n",sensor_gamma_info.gamma_in_array[i], sensor_gamma_info.gamma_out_array[i]);
	}
	printk("gamma sta(%d)\n", sensor_gamma_info.gamma_status);
	#endif
    
    return ret;	
}


static int gamma_cb(rk_cb_para_t *para)
{
	int i,j;
	int temp;
	unsigned char* src;
    int imgWidth = para->dstw;
    int imgHeight = para->dsth;
	int odd_even = para->odd_even;
	int nucleus = para->nucleus;
	int index,k;
	int left,right,mid;
	struct gamma_line_t *line;
	int *in = sensor_gamma_info.gamma_in_array;
	int mid_fix = (sensor_gamma_info.gamma_count-1)/2;

	#if 0
	if(sensor_gamma_info.gamma_status==0)
		return 0;
	#endif
		
	src = para->raw_d + odd_even*imgWidth;
	
	for(i=0+odd_even; i<imgHeight; i+=nucleus){
		for(j=0; j<imgWidth; j++){
			temp = *src;
			#if 0
			//10bit
			if(temp <= 384)
				temp = (404*temp)>>8;
			else if(temp>384 && temp<=576)
				temp = (312*temp+33792)>>8;
			else if(temp>576 && temp<=832)
				temp = (156*temp+126976)>>8;
			else 
				temp = ((41*temp+219392)>>8);
			#endif

			#if 0
			//8bit
			if(temp <= 80)
				temp = (407*temp+364)>>8;
			else if(temp>80 && temp<=144)
				temp = (326*temp+6845)>>8;
			else if(temp>144 && temp<=208)
				temp = (156*temp+31513)>>8;
			else if(temp>208&& temp<255)
				temp = ((38*temp+55552)>>8);
			#endif

			#if 0
			if(temp>2 && temp<=40)
				temp = (275*temp - 560)>>8;
			else if(temp>40 && temp<=96)
				temp = (463*temp - 6594)>>8;
			else if(temp>96 && temp<=192)
				temp = (210*temp + 18178)>>8;
			else if(temp>192 && temp<=240)
				temp = (128*temp + 32768)>>8;

			#endif

			
			if(temp>2 && temp<=24)
				temp = (243*temp - 279)>>8;
			else if(temp>24 && temp<=32)
				temp = (271*temp - 807)>>8;
			else if(temp>32 && temp<=64)
				temp = (510*temp - 8874)>>8;
			else if(temp>64 && temp<=96)
				temp = (406*temp - 1197)>>8;
			else if(temp>96 && temp<=144)
				temp = (257*temp + 12367)>>8;
			else if(temp>144 && temp<=194)
				temp = (161*temp + 26239)>>8;
			else if(temp>194 && temp<=240)
				temp = (128*temp + 32768)>>8;


			#if 0
			left =0;
			right = sensor_gamma_info.gamma_count-1;
			mid = mid2;
			
			while(left<right && temp!=in[mid]){
				if(temp<in[mid]){
					right = mid -1;	
				}else if(temp>in[mid]){
					left = mid + 1;
				}
				mid = (left + right)/2;
			}

			if(temp<in[mid])
				index = mid -1;
			else
				index = mid;

			if(index>=0 && index<)
			line = sensor_gamma_info.gamma_line + index;
			temp = ((line->A*temp + line->C)>>10);
			#endif

			#if 0
			left =0;
			right = sensor_gamma_info.gamma_count-1;

			if(temp<in[mid_fix])
				right = mid_fix;
			else 
				left = mid_fix;	
			mid = (left+right)>>1;

			if(temp<in[mid])
				right = mid;
			else 
				left = mid ;

			for(k=left; k<right; k++){
				if(temp>=in[k] && temp<=in[k+1]){
					line = sensor_gamma_info.gamma_line + k;
					temp = ((line->A*temp + line->C)>>10);
					break;
				}
			}
			#endif
			
			if(temp>0xff)
				temp=0xff;
			else if(temp<0)
				temp=0;
			
			*src = temp;		
			src++;
		}
		src += ((nucleus-1)*imgWidth);
	}

    return 0;
}

static int ag_ctrl(struct sensor* sensor, struct i2c_client* client, int now_ae)
{
	int GainVal;
	u8 GainVal0, GainVal1;
	int dst_gain;

	sensor_read(client, 0x350a, &GainVal0);
	sensor_read(client, 0x350b, &GainVal1);
	GainVal = (((GainVal0 & 0x03)<<8) + GainVal1);

	dst_gain = sensor_ae_list_seqe[SENSOR_AE_LIST_INDEX][sensor->ae_work.n].gain;

	if(dst_gain > 0x3ff)
		dst_gain = 0x3ff;

	GainVal0 = dst_gain>>8;
	GainVal1 = dst_gain&0xff;

	if(GainVal != dst_gain){
		sensor_write(client, 0x350a, GainVal0);
		sensor_write(client, 0x350b, GainVal1);
	}
	//printk("now gain(0x%x) next gain(0x%x)\n", GainVal, dst_gain);

	return 0;
}

static int ae_ctrl(struct sensor* sensor, struct i2c_client* client, int now_ae)
{
	int ExpRegVal;
	u8 ExpRegVal0, ExpRegVal1, ExpRegVal2;
	int dst_exp_val;
	
	sensor_read(client, 0x3500, &ExpRegVal0);
	sensor_read(client, 0x3501, &ExpRegVal1);
	sensor_read(client, 0x3502, &ExpRegVal2);
	ExpRegVal = ((ExpRegVal0 & 0x0f)<<12) + (ExpRegVal1<<4) + ((ExpRegVal2 & 0xf0)>>4);

	dst_exp_val = sensor_ae_list_seqe[SENSOR_AE_LIST_INDEX][sensor->ae_work.n].exposure_line;
	ExpRegVal0 = (dst_exp_val & 0xff0000)>>16;
	ExpRegVal1 = (dst_exp_val & 0xff00)>>8;
	ExpRegVal2 = (dst_exp_val & 0xff);

	//printk("now exposure line (%d), next exposure line (%d)\n", ExpRegVal, dst_exp_val>>4);

	if(dst_exp_val>>4 != ExpRegVal){
		sensor_write(client, 0x3500, ExpRegVal0);
		sensor_write(client, 0x3501, ExpRegVal1);
		sensor_write(client, 0x3502, ExpRegVal2);
	}

	//do_gettimeofday(&(sensor->ae_work.t));
	return 0;
	
}
/*
static int ae_ctrl_test(struct sensor* sensor, struct i2c_client* client, int value)
{
	int i;
	
	printk("Y(%d): value(%d) \n", sensor->ae_work.ae_ctrl_test, value);
	sensor->ae_work.ae_ctrl_test++;
	
	if(sensor->ae_work.ae_ctrl_test == 10)
	{
		//ae record Y
		sensor->ae_work.y[sensor->ae_work.ae_count] = value;
	}
	
	if(sensor->ae_work.ae_ctrl_test == 15)
	{
		sensor->ae_work.ae_ctrl_test = 0;
		sensor->ae_work.ae_count++;
		//ae control
		ae_ctrl(sensor, client, value);
	}

	if(sensor->ae_work.ae_count == 60)
	{
		for(i=0; i<60; i++)
		{
			printk("%d,", sensor->ae_work.y[i]);
		}
	}
	
}

static int ag_ctrl_test(struct sensor* sensor, struct i2c_client* client, int value)
{
	int i;
	
	printk("Y(%d): value(%d) \n", sensor->ae_work.ae_ctrl_test, value);
	sensor->ae_work.ae_ctrl_test++;
	
	if(sensor->ae_work.ae_ctrl_test == 10)
	{
		//ae record Y
		sensor->ae_work.y[sensor->ae_work.ae_count] = value;
	}
	
	if(sensor->ae_work.ae_ctrl_test == 15)
	{
		sensor->ae_work.ae_ctrl_test = 0;
		sensor->ae_work.ae_count++;
		//ae control
		ag_ctrl(sensor, client, value);
	}

	if(sensor->ae_work.ae_count == 50)
	{
		for(i=0; i<50; i++)
		{
			printk("%d,", sensor->ae_work.y[i]);
		}
	}
}

*/

static int aeag_index(struct sensor* sensor,int value)
{    
    int diff;
    int flag = 0;
    int n=0;
    int temp_index= sensor->ae_work.n;

    //printk("lumi %d\n", value);
    diff = AE_GOAL - value;

    if(diff>0){
        flag =1;
    }else{
        flag =0;
        diff = -diff;
    }

    if(diff>0 && diff<=5)
        n=2;
    if(diff>5&& diff<=10)
        n=3;
    if(diff>10 && diff<=15)
        n=6;
    if(diff>15 && diff<=20)
        n=8;
    if(diff>20 && diff<=25)
        n=10;
    if(diff>25 && diff<=30)
        n=12;
    if(diff>30 && diff<=35)
        n=15;
    if(diff>35 && diff<=40)
        n=17;
    if(diff>40)
        n=20;

    if(flag)
        temp_index -= n;
    else
        temp_index += n;

    if(temp_index > AE_TABLE_COUNT)
        temp_index = AE_TABLE_COUNT;

    if(temp_index< 0)
        temp_index = 0;


    if(temp_index == sensor->ae_work.n) {
        return 0;
    } else {
        sensor->ae_work.n = temp_index;
        return temp_index;
    }

}



static int sensor_g_control(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct sensor *sensor = to_sensor(client);
    const struct v4l2_queryctrl *qctrl;

    qctrl = soc_camera_find_qctrl(&sensor_ops, ctrl->id);

    if (!qctrl)
    {
        SENSOR_TR("\n %s ioctrl id = 0x%x  is invalidate \n", SENSOR_NAME_STRING(), ctrl->id);
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
    struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct sensor *sensor = to_sensor(client);
    struct soc_camera_device *icd = client->dev.platform_data;
    const struct v4l2_queryctrl *qctrl;


    qctrl = soc_camera_find_qctrl(&sensor_ops, ctrl->id);

    if (!qctrl)
    {
        SENSOR_TR("\n %s ioctrl id = 0x%x  is invalidate \n", SENSOR_NAME_STRING(), ctrl->id);
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
        SENSOR_TR("\n %s ioctrl id = 0x%x  is invalidate \n", SENSOR_NAME_STRING(), ext_ctrl->id);
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
    int val_offset,ret;
	

    qctrl = soc_camera_find_qctrl(&sensor_ops, ext_ctrl->id);

    if (!qctrl)
    {
        SENSOR_TR("\n %s ioctrl id = 0x%x  is invalidate \n", SENSOR_NAME_STRING(), ext_ctrl->id);
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

                ret = sensor_set_focus_absolute(icd, qctrl,ext_ctrl->value);
				if ((ret == 0) || (0 == (sensor->info_priv.funmodule_state & SENSOR_AF_IS_OK))) {
					if (ext_ctrl->value == qctrl->minimum) {
						sensor->info_priv.auto_focus = SENSOR_AF_MODE_INFINITY;
					} else if (ext_ctrl->value == qctrl->maximum) {
						sensor->info_priv.auto_focus = SENSOR_AF_MODE_MACRO;
					} else {
						sensor->info_priv.auto_focus = SENSOR_AF_MODE_FIXED;
					}
				}

                break;
            }
        case V4L2_CID_FOCUS_RELATIVE:
            {
                if ((ext_ctrl->value < qctrl->minimum) || (ext_ctrl->value > qctrl->maximum))
                    return -EINVAL;

                sensor_set_focus_relative(icd, qctrl,ext_ctrl->value);
                break;
            }
		case V4L2_CID_FOCUS_AUTO:
			{
				if (ext_ctrl->value == 1) {
					if (sensor_set_focus_mode(icd, qctrl,SENSOR_AF_MODE_AUTO) != 0) {
						if(0 == (sensor->info_priv.funmodule_state & SENSOR_AF_IS_OK)) {
							sensor->info_priv.auto_focus = SENSOR_AF_MODE_AUTO;
						}
						return -EINVAL;
					}
					sensor->info_priv.auto_focus = SENSOR_AF_MODE_AUTO;
				} else if (SENSOR_AF_MODE_AUTO == sensor->info_priv.auto_focus){
					if (ext_ctrl->value == 0)
						sensor->info_priv.auto_focus = SENSOR_AF_MODE_CLOSE;
				}
				break;
			}
		case V4L2_CID_FOCUS_CONTINUOUS:
			{
				if (SENSOR_AF_MODE_CONTINUOUS != sensor->info_priv.auto_focus) {
					if (ext_ctrl->value == 1) {
						if (sensor_set_focus_mode(icd, qctrl,SENSOR_AF_MODE_CONTINUOUS) != 0) {
							if(0 == (sensor->info_priv.funmodule_state & SENSOR_AF_IS_OK)) {
								sensor->info_priv.auto_focus = SENSOR_AF_MODE_CONTINUOUS;
							}
							return -EINVAL;
						}
						sensor->info_priv.auto_focus = SENSOR_AF_MODE_CONTINUOUS;
					}
				} else {
					if (ext_ctrl->value == 0)
						sensor->info_priv.auto_focus = SENSOR_AF_MODE_CLOSE;
				}
				break;
			}		
#endif
#if CONFIG_SENSOR_Flash
        case V4L2_CID_FLASH:
            {
                if (sensor_set_flash(icd, qctrl,ext_ctrl->value) != 0)
                    return -EINVAL;
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
    struct i2c_client *client = v4l2_get_subdevdata(sd);
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
    struct i2c_client *client = v4l2_get_subdevdata(sd);
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

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct sensor *sensor = to_sensor(client);
    #if CONFIG_SENSOR_Focus
	struct soc_camera_device *icd = client->dev.platform_data;
	struct v4l2_mbus_framefmt mf;
    #endif

	if (enable == 1) {
		sensor->info_priv.enable = 1;
		#if CONFIG_SENSOR_Focus
        mf.width	= icd->user_width;
    	mf.height	= icd->user_height;
    	mf.code	= sensor->info_priv.fmt.code;
    	mf.colorspace	= sensor->info_priv.fmt.colorspace;
    	mf.field	= V4L2_FIELD_NONE;
		/* If auto focus firmware haven't download success, must download firmware again when in video or preview stream on */
		if (sensor_fmt_capturechk(sd, &mf) == false) {
			if ((sensor->info_priv.affm_reinit == 1) || ((sensor->info_priv.funmodule_state & SENSOR_AF_IS_OK)==0)) {				                   
                //sensor_af_workqueue_set(icd, WqCmd_af_init, 0, false);
				sensor->info_priv.affm_reinit = 0;
			}
		}
		#endif
	} else if (enable == 0) {	
        sensor->info_priv.enable = 0;
        #if CONFIG_SENSOR_Focus	
        if (sensor->sensor_wq) 
            flush_workqueue(sensor->sensor_wq);
        if (sensor->vcm_wq) 
		    flush_workqueue(sensor->vcm_wq);
		sensor->vcm_work_wait.fix_EV = 0;      
		#endif
	}
	return 0;
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

	if (sensor_ioctrl(icd, Sensor_PowerDown, 0) < 0) {
		ret = -ENODEV;
		goto sensor_video_probe_err;
	}
    /* soft reset */
    ret = sensor_write(client, 0x0103, 0x01);
    if (ret != 0) {
        SENSOR_TR("soft reset %s failed\n",SENSOR_NAME_STRING());
        ret = -ENODEV;
		goto sensor_video_probe_err;
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

    return 0;

sensor_video_probe_err:

    return ret;
}
static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct soc_camera_device *icd = client->dev.platform_data;    
    struct sensor *sensor = to_sensor(client);
    //char *file_name;
    int ret = 0,i;
    long fix_ev, tmp_ev, differ_ev;
    struct rk_raw_ret  *raw_ret;
    rk29_camera_sensor_cb_s *icd_cb =NULL;
	int lumi;
	int *lens_para,*ccm_matrix_para;
	
	SENSOR_DG("\n%s..%s..cmd:%x \n",SENSOR_NAME_STRING(),__FUNCTION__,cmd);
	switch (cmd)
	{
		case RK29_CAM_SUBDEV_DEACTIVATE:
		{
			sensor_deactivate(client);
			break;
		}
		case RK29_CAM_SUBDEV_IOREQUEST:
		{
			sensor->sensor_io_request = (struct rk29camera_platform_data*)arg;    
            
            if (sensor->sensor_io_request != NULL) { 
			int j = 0;
			for(j = 0;j < RK_CAM_NUM;j++){
				if (sensor->sensor_io_request->gpio_res[j].dev_name && 
					(strcmp(sensor->sensor_io_request->gpio_res[j].dev_name, dev_name(icd->pdev)) == 0)) {
					sensor->sensor_gpio_res = (struct rk29camera_gpio_res*)&sensor->sensor_io_request->gpio_res[j];
					break;
				  } 
			}
			if(j == RK_CAM_NUM){
				SENSOR_TR("%s %s RK_CAM_SUBDEV_IOREQUEST fail\n",SENSOR_NAME_STRING(),__FUNCTION__);
				ret = -EINVAL;
				goto sensor_ioctl_end;
				}
            } 
            
            /* ddl@rock-chips.com : if gpio_flash havn't been set in board-xxx.c, sensor driver must notify is not support flash control 
               for this project */
            #if CONFIG_SENSOR_Flash	
        	if (sensor->sensor_gpio_res) {
                printk("flash io:%d\n",sensor->sensor_gpio_res->gpio_flash);
                if (sensor->sensor_gpio_res->gpio_flash == INVALID_GPIO) {
                    for (i = 0; i < icd->ops->num_controls; i++) {
                		if (V4L2_CID_FLASH == icd->ops->controls[i].id) {
                			memset((char*)&icd->ops->controls[i],0x00,sizeof(struct v4l2_queryctrl));                			
                		}
                    }
                    sensor->info_priv.flash = 0xff;
                    SENSOR_DG("%s flash gpio is invalidate!\n",SENSOR_NAME_STRING());
                }
        	}
            #endif
			break;
		}

        case RK29_CAM_SUBDEV_CALIBRATION_IMG:
        {
			#if 0
            file_name = (char*)arg;
            memset(file_name,0x00, 30);
            sprintf(file_name,"%s_black.bin",SENSOR_NAME_STRING());
            file_name += 30;
            memset(file_name,0x00, 30);
            sprintf(file_name,"%s_white.bin",SENSOR_NAME_STRING());
			#endif

			lens_para = (int *)arg;
			lens_para[0] = LENS_CENTRE_X;
			lens_para[1] = LENS_CENTRE_Y;
			lens_para[2] = LENS_R_B2;
			lens_para[3] = LENS_R_B4;
			lens_para[4] = LENS_G_B2;
			lens_para[5] = LENS_G_B4;
			lens_para[6] = LENS_B_B2;
			lens_para[7] = LENS_B_B4;
            break;
        }
        case RK29_CAM_SUBDEV_GAMMA:
		{
			icd_cb = (rk29_camera_sensor_cb_s*)(arg);
            icd_cb->gamma.cb = gamma_cb;
			break;
        }
		case RK29_CAM_SUBDEV_COLOR_CORRECTION:
		{
		    #if 0
			icd_cb = (rk29_camera_sensor_cb_s*)(arg);
            icd_cb->color_correction.cb = color_correction_cb;
            #endif
            ccm_matrix_para = (int *)arg;
            memcpy(ccm_matrix_para, ccm_matrix, 20*sizeof(int));
			break;
		}

        case RK29_CAM_SUBDEV_AE:
		{
			raw_ret = (struct rk_raw_ret*)arg;
			lumi = raw_ret->ae_lumi;
			//int compensate = raw_ret->ae_compensate;
		
			if(lumi>AE_GOAL-3 && lumi<AE_GOAL+3){
				break;
			}else{
				ret = aeag_index(sensor,lumi); 
				if(ret != 0){
					ret = ae_ctrl(sensor, client, lumi);	
					ret = ag_ctrl(sensor, client, lumi);
				}
			}
			break;
		}

        case RK29_CAM_SUBDEV_AF:
		{
            /* ddl@rock-chips.com : Don't reponse AF command when the AF module is error */
            if(0 == (sensor->info_priv.funmodule_state & SENSOR_AF_IS_OK)) { 
                SENSOR_TR("%s(%d): af module is error, V4L2_CID_FOCUS_TEST is cancel",__FUNCTION__,__LINE__);
                return -EINVAL;
            }
            raw_ret = (struct rk_raw_ret*)arg;
            tmp_ev = (int)raw_ret->af_EV;
			fix_ev = sensor->vcm_work_wait.fix_EV;
			differ_ev = fix_ev-tmp_ev;
			differ_ev = abs64(differ_ev);
			//SENSOR_DG("%s..%s..%d  abs(%ld)\n", SENSOR_NAME_STRING(), __FUNCTION__, __LINE__, differ_ev);
			if(differ_ev < (fix_ev/3) && sensor->vcm_work_wait.wait_flag == -1)
			{
				sensor->vcm_work_wait.wait_cnt = 0;
				break;
			}
			
			if(differ_ev > (fix_ev/3)|| sensor->vcm_work_wait.wait_flag != -1)
			{
				if(differ_ev > (fix_ev/3))
					sensor->vcm_work_wait.wait_cnt++;

				if(sensor->vcm_work_wait.wait_cnt>10)
				{
					sensor->vcm_work_wait.EV = tmp_ev;
					queue_work(sensor->vcm_wq, &(sensor->vcm_work_wait.vcm_work));	
				}
				break;
			}

            break;
		}

        case RK29_CAM_SUBDEV_AWB:
        {   
			break;            
        }
		default:
		{
			SENSOR_TR("%s %s cmd(0x%x) is unknown !\n",SENSOR_NAME_STRING(),__FUNCTION__,cmd);
			break;
		}
		
	}

sensor_ioctl_end:
	return ret;

}
static int sensor_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			    enum v4l2_mbus_pixelcode *code)
{
	if (index >= ARRAY_SIZE(sensor_colour_fmts))
		return -EINVAL;

	*code = sensor_colour_fmts[index].code;
	return 0;
}
static struct v4l2_subdev_core_ops sensor_subdev_core_ops = {
	.init		= sensor_init,
	.g_ctrl		= sensor_g_control,
	.s_ctrl		= sensor_s_control,
	.g_ext_ctrls          = sensor_g_ext_controls,
	.s_ext_ctrls          = sensor_s_ext_controls,
	.g_chip_ident	= sensor_g_chip_ident,
	.ioctl = sensor_ioctl,
};

static struct v4l2_subdev_video_ops sensor_subdev_video_ops = {
	.s_mbus_fmt	= sensor_s_fmt,
	.g_mbus_fmt	= sensor_g_fmt,
	.try_mbus_fmt	= sensor_try_fmt,
	.enum_mbus_fmt	= sensor_enum_fmt,
	.s_stream   = sensor_s_stream,
};
static struct v4l2_subdev_ops sensor_subdev_ops = {
	.core	= &sensor_subdev_core_ops,
	.video = &sensor_subdev_video_ops,
};

static int vcm_coarse_control(struct vcm_work_struct *my_work, struct motor_parameter *param)
{
	int ret, i;
	int step = SMALL_CURRENT_STEP;
	struct i2c_client *vcm_client = my_work->vcm_client;	
	struct timeval t0, t1; 

	do_gettimeofday(&t0);

	my_work->count++;
	
	if(my_work->count == 1){
		
		do{
			ret = vcm_client->driver->command(vcm_client, StepFocus_Nearest_Tag, 0);
		}while(ret);
				
		my_work->wait_flag = 0;		
		my_work->engry_cnt = 0;
		SENSOR_DG("%s..%d.. count(%d)\n", __FUNCTION__, __LINE__, my_work->count);
	
		goto FUNCRET;
	}

	if(my_work->count>5 &&((my_work->count)&0x01)!=0)
	{			
		my_work->engry[my_work->engry_cnt] = my_work->EV;
		my_work->engry_cnt++;
		
		if(my_work->EV > my_work->max_mft){
			my_work->max_mft = my_work->EV;
			my_work->max_current = param->present_code;
		}

		if(param->present_code < MAX_CURRENT){
			do{
				ret = vcm_client->driver->command(vcm_client, StepFocus_Far_Tag, 0);
			}while(ret);

			goto FUNCRET;
		}

		my_work->i_current= my_work->max_current;
		SENSOR_DG("%s..%s..%d MAX_I(%d)!!!!!!!!!!!!!!!!!!!!!!\n", SENSOR_NAME_STRING(), __FUNCTION__, __LINE__, my_work->i_current);

		do{
			ret = vcm_client->driver->command(vcm_client, StepFocus_Set_CurrentStep_Tag, &step);
		}while(ret);
		
		my_work->fix_EV = my_work->max_mft;
		my_work->count = 0;
		my_work->max_mft = 0;
		my_work->engry_cnt = 0;

		SENSOR_DG("coarse control++++++++++++++++++\n\n");
		for(i=0; i<15; i++)
		{
			SENSOR_DG("%ld, ", my_work->engry[i]);
		}
		SENSOR_DG("\n\ncoarse control++++++++++++++++++\n\n");
		
	}
				
FUNCRET:
	do_gettimeofday(&t1);  
	//SENSOR_DG("coarse control (%ld)us\n", (t1.tv_sec*1000000 + t1.tv_usec) - (t0.tv_sec*1000000 + t0.tv_usec));

	return ret;
}

static int vcm_fine_control(struct vcm_work_struct *my_work, struct motor_parameter *param)
{
	int ret, icurrent, i;
	int step = BIG_CURRENT_STEP;
	struct i2c_client *vcm_client = my_work->vcm_client;
	struct timeval t0, t1; 

	do_gettimeofday(&t0);

	my_work->count++;
	
	if(my_work->count == 1){
		icurrent = my_work->i_current - 60;
		do{ 
			ret = vcm_client->driver->command(vcm_client, StepFocus_Set_Current_Tag, &(icurrent));  
		}while(ret);

		if(my_work->i_current + 60 > MAX_CURRENT)
			icurrent = MAX_CURRENT;
		
		SENSOR_DG("%s..%d.. count(%d)\n", __FUNCTION__, __LINE__, my_work->count);
		goto FUNCRET;
	}

	if(my_work->count>5 &&((my_work->count)&0x01)!=0){
		my_work->engry[my_work->engry_cnt] = my_work->EV;
		my_work->engry_cnt++;
		
		if(my_work->EV > my_work->max_mft)
		{
			my_work->max_mft = my_work->EV;
			my_work->max_current = param->present_code;
		}	
		
		if(param->present_code < icurrent){	
			do{
				ret = vcm_client->driver->command(vcm_client, StepFocus_Far_Tag, 0);
			}while(ret);
			goto FUNCRET;
		} 			

		do{ 
			ret = vcm_client->driver->command(vcm_client, StepFocus_Set_Current_Tag, &(my_work->max_current));
		}while(ret);
		SENSOR_DG("%s..%s..%d turn to coarse control I(%d)\n", SENSOR_NAME_STRING(), __FUNCTION__, __LINE__, my_work->max_current);

		my_work->fix_EV = my_work->max_mft;
		my_work->wait_flag = -1;
		my_work->max_mft = 0;
		my_work->max_current = 0;
		my_work->EV = 0;
		my_work->i_current = 0;
		my_work->count = 0;
		my_work->wait_cnt = 0;

		do{
			ret = vcm_client->driver->command(vcm_client, StepFocus_Set_CurrentStep_Tag, &step);
		}while(ret);

		SENSOR_DG("fine control-----------------\n\n");
		for(i=0; i<10; i++)
		{
			SENSOR_DG("%ld, ", my_work->engry[i]);
		}
		SENSOR_DG("\n\nfine control-----------------\n\n");
	
	}

FUNCRET:	
	do_gettimeofday(&t1);  
	SENSOR_DG("fine control (%ld)us\n", (t1.tv_sec*1000000 + t1.tv_usec) - (t0.tv_sec*1000000 + t0.tv_usec));
	
	return ret;
}

void func_vcm_control(struct work_struct* work)
{
	int ret;
	struct motor_parameter param;
	struct vcm_work_struct *my_work = container_of(work, struct vcm_work_struct, vcm_work);
	struct i2c_client *vcm_client = my_work->vcm_client;	
	struct timeval t0, t1; 

	do_gettimeofday(&t0);
		
	do{
		ret = vcm_client->driver->command(vcm_client, StepFocus_Read_Tag, &param);
	}while(ret);

	if(param.current_step == BIG_CURRENT_STEP){
		ret = vcm_coarse_control(my_work, &param);					
	}
	else if(param.current_step == SMALL_CURRENT_STEP){
		ret = vcm_fine_control(my_work, &param);	
	}

	do_gettimeofday(&t1);  
	SENSOR_DG("VCM: %ld us\n", (t1.tv_sec*1000000 + t1.tv_usec) - (t0.tv_sec*1000000 + t0.tv_usec));
	
}

static int sensor_probe(struct i2c_client *client,
			 const struct i2c_device_id *did)
{
    struct sensor *sensor;
	struct i2c_adapter *vcm_adapter;
	//struct i2c_client *vcm_client;
    struct soc_camera_device *icd = client->dev.platform_data;
    struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
    struct soc_camera_link *icl;
    struct rkcamera_platform_data *sensor_device=NULL,*new_camera;
	int ret;
	
	#if CONFIG_SENSOR_Focus
	//struct soc_camera_host *host = to_soc_camera_host(icd->dev.parent);
	struct rk29camera_platform_data *pdata=NULL;
	struct rk_vcm_device_info* vcm_info=NULL;
	#endif
	 
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
    pdata = icl->priv_usr;
    if (pdata) {
        new_camera = pdata->register_dev_new; 
        while (strstr(new_camera->dev_name,"end")==NULL) { 
            if (strcmp(dev_name(icd->pdev), new_camera->dev_name) == 0) { 
                sensor_device = new_camera; 
                break; 
            } 
            new_camera++; 
        } 

        if (sensor_device) {
            vcm_info = &sensor_device->vcm_dev;
        }
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
    sensor->info_priv.fmt = sensor_colour_fmts[0];

	#if CONFIG_SENSOR_I2C_NOSCHED
	atomic_set(&sensor->tasklock_cnt,0);
	#endif

    ret = sensor_video_probe(icd, client);
    if (ret < 0) {
        icd->ops = NULL;
        i2c_set_clientdata(client, NULL);
        kfree(sensor);
		sensor = NULL;
    } else {
		#if CONFIG_SENSOR_Focus		
		if (vcm_info == NULL) {
            SENSOR_TR(KERN_ERR "%s(%d): vcm_info is NULL!",SENSOR_NAME_STRING(),__LINE__);
            BUG();
		} else {
            if (vcm_info->vcm_i2c_board_info.addr == 0) {
                SENSOR_DG("%s(%d): this ov5647 module is not build with vcm",__FUNCTION__,__LINE__);
                goto end;
            }
		}
		sensor->sensor_wq = create_workqueue(SENSOR_NAME_STRING(_af_workqueue));
		if (sensor->sensor_wq == NULL)
			SENSOR_TR("%s create fail!", SENSOR_NAME_STRING(_af_workqueue));
		/*<<<<<<<<<<<<<<<<<<<<<< ouyang yafeng : create a3907 i2c client for ov5647>>>>>>>>>>>>>>>>>>>*/
		sensor->vcm_wq = create_workqueue(SENSOR_NAME_STRING(_vcm_workqueue));	
		if (sensor->vcm_wq == NULL)
			SENSOR_TR("%s create fail!", SENSOR_NAME_STRING(_vcm_workqueue)); 
		
		vcm_adapter = i2c_get_adapter(vcm_info->vcm_i2c_adapter_id);
		if(!vcm_adapter)
			SENSOR_TR("%s..%s..%d get vcm-adapter fail!", SENSOR_NAME_STRING(), __FUNCTION__, __LINE__);
		
		sensor->vcm_work_wait.vcm_client = i2c_new_device(vcm_adapter, &(vcm_info->vcm_i2c_board_info));
 		if(sensor->vcm_work_wait.vcm_client == NULL) 
			SENSOR_TR("%s..%s..%d vcm client create fail!", SENSOR_NAME_STRING(), __FUNCTION__, __LINE__);

        if (sensor->vcm_work_wait.vcm_client->driver == NULL)
        	SENSOR_TR("%s..%s..%d vcm driver attach failed!", SENSOR_NAME_STRING(), __FUNCTION__, __LINE__);
        
		INIT_WORK(&(sensor->vcm_work_wait.vcm_work), func_vcm_control);		
		/*<<<<<<<<<<<<<<<<<<<<<< ouyang yafeng : create a3907 i2c client for ov5647>>>>>>>>>>>>>>>>>>>*/   
		mutex_init(&sensor->wq_lock);
        
		#endif
    }
end:
    SENSOR_DG("\n%s..%s..%d  ret = %x \n",__FUNCTION__,__FILE__,__LINE__,ret);
    return ret;
}

static int sensor_remove(struct i2c_client *client)
{
    struct sensor *sensor = to_sensor(client);
    struct soc_camera_device *icd = client->dev.platform_data;

	#if CONFIG_SENSOR_Focus
	if (sensor->sensor_wq) {
		destroy_workqueue(sensor->sensor_wq);
		sensor->sensor_wq = NULL;
	}

	if (sensor->vcm_wq) {
		destroy_workqueue(sensor->vcm_wq);
		sensor->vcm_wq = NULL;
	}
	#endif

    icd->ops = NULL;
    i2c_set_clientdata(client, NULL);
    client->driver = NULL;
    kfree(sensor);
	sensor = NULL;
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


