
#include "generic_sensor.h"
#include "a3907.h"
#include "Ov5647_AE.c"
#include <asm/smp.h>

/*
*      Driver Version Note
*v0.0.1: this driver is compatible with generic_sensor
*v0.1.1:
*        add sensor_focus_af_const_pause_usr_cb;
*/
static int version = KERNEL_VERSION(0,1,1);
module_param(version, int, S_IRUGO);
static int debug =1;
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

/* Sensor Driver Configuration Begin */
#define SENSOR_NAME RK29_CAM_SENSOR_OV5647
#define SENSOR_V4L2_IDENT V4L2_IDENT_OV5647
#define SENSOR_ID 0x5647
#define SENSOR_BUS_PARAM      (SOCAM_MASTER | SOCAM_PCLK_SAMPLE_RISING|SOCAM_HSYNC_ACTIVE_HIGH\
                              |SOCAM_VSYNC_ACTIVE_LOW |SOCAM_DATA_ACTIVE_HIGH | SOCAM_DATAWIDTH_10 \
                              |SOCAM_MCLK_24MHZ)
#define SENSOR_PREVIEW_W                     1296
#define SENSOR_PREVIEW_H                     972
#define OV5647_USE_30_OR_15_FPS              0 // 1: 30fps,preview picture bad 
                                               // 0: 15fps,preview picture better
#define SENSOR_PREVIEW_FPS                   10000     // 15fps 
#define SENSOR_FULLRES_L_FPS                 5000      // 5fps
#define SENSOR_FULLRES_H_FPS                 10000      // 10fps
#define SENSOR_720P_FPS                      10000
#define SENSOR_1080P_FPS                     10000

#define SENSOR_REGISTER_LEN                  2         // sensor register address bytes
#define SENSOR_VALUE_LEN                     1         // sensor register value bytes

#if CONFIG_OV5647_AUTOFOCUS
static unsigned int SensorConfiguration = {CFG_Focus|CFG_FocusZone};
#else
static unsigned int SensorConfiguration = {0};
#endif

static unsigned int SensorChipID[] = {SENSOR_ID};

/* Sensor Driver Configuration End */


#define SENSOR_NAME_STRING(a) STR(CONS(SENSOR_NAME, a))
#define SENSOR_NAME_VARFUN(a) CONS(SENSOR_NAME, a)

#define SensorRegVal(a,b) CONS4(SensorReg,SENSOR_REGISTER_LEN,Val,SENSOR_VALUE_LEN)(a,b)
#define sensor_write(client,reg,v) CONS4(sensor_write_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_read(client,reg,v) CONS4(sensor_read_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_write_array generic_sensor_write_array

struct ae_work_struct{
	int exposure_line;
	int gain;	
	int lumi;
	int compensate;
	int n;
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
};


struct sensor_parameter
{
	unsigned int PreviewDummyPixels;
	unsigned int CaptureDummyPixels;
	unsigned int preview_exposure;
	unsigned int preview_gain;
	unsigned short int preview_line_width;	

	unsigned short int PreviewPclk;
	unsigned short int CapturePclk;
	char awb[6];
};
/**********af info******************/
#define CONFIG_SENSOR_FocusCenterInCapture (0)
struct af_zone_info{
    int centre_w;
    int centre_h;
};

struct sensor_af_info{
    struct af_zone_info zone_info;
    int af_vcm_state;
    int is_capture;
    int af_command;
};

//af vcm state
#define vcm_state_idle (0)
#define vcm_state_busy (1)
#define vcm_state_focus_complete (2)

static struct sensor_af_info g_af_info;
/*******************/

struct specific_sensor{
	struct generic_sensor common_sensor;
	//define user data below
	struct sensor_parameter parameter;

	struct ae_work_struct ae_work;
	struct vcm_work_struct vcm_work_wait;
	struct workqueue_struct *vcm_wq;
	

	unsigned char gamma_table[255];
};

/********AE parament************/
#if OV5647_USE_30_OR_15_FPS
#define AE_GOAL 110
#define AE_TABLE_COUNT 331
#define AE_INDEX_INIT  30
#define AE_VALUE_INIT  0x37f0
#define AG_VALUE_INIT  0x6b
#define SENSOR_AE_LIST_INDEX	SENSOR_AE_LIST_INDEX_960P
#else
#define AE_GOAL 110
#define AE_TABLE_COUNT 331
#define AE_INDEX_INIT  30
#define AE_VALUE_INIT  0x6f00
#define AG_VALUE_INIT  0x6f
#define SENSOR_AE_LIST_INDEX	SENSOR_AE_LIST_INDEX_QSXGA
#endif

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
/****************ov5647 orginal gamma******************/
//static int gamma_in_array[] = {0,4,8,16,32,40,48,56,64,72,80,96,112,144,176,208,255};
//static int gamma_out_array[] = {0,7,14,28,53,66,79,91,103,116,127,150,171,209,234,248,255};
  //static int gamma_out_array[] = {0,3,8, 15,30,50,60,80,103,120,133,165,185,210,235,250,255};
/****************ov5647 new gamma*******************/
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
#define LENS_FULL_WIDTH     (2592)
#define LENS_FULL_HEIGHT    (1944)

/****************ccm matrix ****************/
static int ccm_matrix[] = {
1500,  -540,  64,   0,  //ccm_matrix 4x4 array
-212,  1140,  96,   0,
-16,   -360,  1400, 0,
 0,       0,   0,   1024,
};

/*
*  The follow setting need been filled.
*  
*  Must Filled:
*  sensor_init_data :               Sensor initial setting;
*  sensor_fullres_lowfps_data :     Sensor full resolution setting with best auality, recommand for video;
*  sensor_preview_data :            Sensor preview resolution setting, recommand it is vga or svga;
*  sensor_softreset_data :          Sensor software reset register;
*  sensor_check_id_data :           Sensir chip id register;
*
*  Optional filled:
*  sensor_fullres_highfps_data:     Sensor full resolution setting with high framerate, recommand for video;
*  sensor_720p:                     Sensor 720p setting, it is for video;
*  sensor_1080p:                    Sensor 1080p setting, it is for video;
*
*  :::::WARNING:::::
*  The SensorEnd which is the setting end flag must be filled int the last of each setting;
*/

/* Sensor initial setting */
static struct rk_sensor_reg sensor_init_data[] ={
	{0x0100,0x00},
    {0x0103,0x01},
    SensorWaitMs(0x05),
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
    {0x3500,0x00},
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
	SensorEnd
};
/* Senor full resolution setting: recommand for capture */
static struct rk_sensor_reg sensor_fullres_lowfps_data[] ={
    {0x0100,0x00},
    {0x3035,0x11},
	{0x3036,0x64},
	{0x303c,0x11},
	//{0x3821,0x06},
	//{0x3820,0x00},
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
	SensorEnd
};
/* Senor full resolution setting: recommand for video */
static struct rk_sensor_reg sensor_fullres_highfps_data[] ={
    {0x0100,0x00},
    {0x3035,0x11},
	{0x3036,0x64},
	{0x303c,0x11},
	//{0x3821,0x06},
	//{0x3820,0x00},
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
	SensorEnd
};


/* Preview resolution setting*/
#if OV5647_USE_30_OR_15_FPS
//Preview 1296x972 30fps, 24MHz XVCLK, 56MHz PCLK
static struct rk_sensor_reg sensor_preview_data[] ={
    {0x0100,0x00},// software standby
    {0x3035,0x11},// PLL
    {0x3036,0x46},// PLL
    {0x303c,0x11},// PLL
    {0x3821,0x07},// ISP mirror on, Sensor mirror on, H bin on
    {0x3820,0x41},// ISP flip off, Sensor flip off, V bin on
    {0x3612,0x59},// analog control
    {0x3618,0x00},// analog control
    {0x380c,0x07},// HTS = 1896
    {0x380d,0x68},// HTS
    {0x380e,0x03},// VTS = 984
    {0x380f,0xd8},// VTS
    {0x3814,0x31},// X INC
    {0x3815,0x31},// X INC
    {0x3708,0x64},// analog control
    {0x3709,0x52},// analog control
    {0x3808,0x05},// DVPHO = 1296
    {0x3809,0x10},// DVPHO
    {0x380a,0x03},// DVPVO = 972
    {0x380b,0xcc},// DVPVO
    {0x3800,0x00},// X Start
    {0x3801,0x08},// X Start
    {0x3802,0x00},// Y Start
    {0x3803,0x02},// Y Start
    {0x3804,0x0a},// X End
    {0x3805,0x37},// X End
    {0x3806,0x07},// Y End
    {0x3807,0xa1},// Y End
    //; banding filter
    {0x3a08,0x01},// B50
    {0x3a09,0x27},// B50
    {0x3a0a,0x00},// B60
    {0x3a0b,0xf6},// B60
    {0x3a0d,0x04},// B60 max
    {0x3a0e,0x03},// B50 max
    {0x4004,0x02},// black line number
    {0x4837,0x24},// MIPI pclk period
    {0x0100,0x01},// wake up from software standby
     SensorEnd
};
#else
//Preview 1296x972 15fps, 24MHz XVCLK, 56MHz PCLK
static struct rk_sensor_reg sensor_preview_data[] =
{
    {0x0100,0x00},// software standby
    {0x3035,0x11},// PLL
    {0x3036,0x46},// PLL
    {0x303c,0x11},// PLL
    {0x3821,0x07},// ISP mirror on, Sensor mirror on, H bin on
    {0x3820,0x41},// ISP flip off, Sensor flip off, V bin on
    {0x3612,0x59},// analog control
    {0x3618,0x00},// analog control
    {0x380c,0x07},// HTS = 1896
    {0x380d,0x68},// HTS
    {0x380e,0x07},// VTS = 1968
    {0x380f,0xb0},// VTS
    {0x3814,0x31},// X INC
    {0x3815,0x31},// X INC
    {0x3708,0x64},// analog control
    {0x3709,0x52},// analog control
    {0x3808,0x05},// DVPHO = 1296
    {0x3809,0x10},// DVPHO
    {0x380a,0x03},// DVPVO = 972
    {0x380b,0xcc},// DVPVO
    {0x3800,0x00},// X Start
    {0x3801,0x08},// X Start
    {0x3802,0x00},// Y Start
    {0x3803,0x02},// Y Start
    {0x3804,0x0a},// X End
    {0x3805,0x37},// X End
    {0x3806,0x07},// Y End
    {0x3807,0xa1},// Y End
    /* banding filter*/
    {0x3a08,0x01},// B50
    {0x3a09,0x27},// B50
    {0x3a0a,0x00},// B60
    {0x3a0b,0xf6},// B60
    {0x3a0d,0x04},// B60 max
    {0x3a0e,0x03},// B50 max
    {0x4004,0x02},// black line number
    {0x4837,0x24},// MIPI pclk period
    {0x0100,0x01},// wake up from software standby
    SensorEnd
};
#endif

/* 1280x720 */
static struct rk_sensor_reg sensor_720p[]={
	SensorEnd
};

/* 1920x1080 */
static struct rk_sensor_reg sensor_1080p[]={
	SensorEnd
};


static struct rk_sensor_reg sensor_softreset_data[]={
    SensorRegVal(0x0103,0x01),
	SensorEnd
};

static struct rk_sensor_reg sensor_check_id_data[]={
    SensorRegVal(0x300A,0x56),
    SensorRegVal(0x300B,0x47),
	SensorEnd
};
/*
*  The following setting must been filled, if the function is turn on by CONFIG_SENSOR_xxxx
*/
static struct rk_sensor_reg sensor_WhiteB_Auto[]=
{
	SensorEnd
};
/* Cloudy Colour Temperature : 6500K - 8000K  */
static	struct rk_sensor_reg sensor_WhiteB_Cloudy[]=
{
	SensorEnd
};
/* ClearDay Colour Temperature : 5000K - 6500K	*/
static	struct rk_sensor_reg sensor_WhiteB_ClearDay[]=
{
	//Sunny
	SensorEnd
};
/* Office Colour Temperature : 3500K - 5000K  */
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp1[]=
{
	//Office
	SensorEnd

};
/* Home Colour Temperature : 2500K - 3500K	*/
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp2[]=
{
	//Home
	SensorEnd
};
static struct rk_sensor_reg *sensor_WhiteBalanceSeqe[] = {sensor_WhiteB_Auto, sensor_WhiteB_TungstenLamp1,sensor_WhiteB_TungstenLamp2,
	sensor_WhiteB_ClearDay, sensor_WhiteB_Cloudy,NULL,
};

static	struct rk_sensor_reg sensor_Brightness0[]=
{
	// Brightness -2
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness1[]=
{
	// Brightness -1

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness2[]=
{
	//	Brightness 0

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness3[]=
{
	// Brightness +1

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness4[]=
{
	//	Brightness +2

	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness5[]=
{
	//	Brightness +3

	SensorEnd
};
static struct rk_sensor_reg *sensor_BrightnessSeqe[] = {sensor_Brightness0, sensor_Brightness1, sensor_Brightness2, sensor_Brightness3,
	sensor_Brightness4, sensor_Brightness5,NULL,
};

static	struct rk_sensor_reg sensor_Effect_Normal[] =
{
	{0x507b, 0x00},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_WandB[] =
{
	{0x507b, 0x20},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Sepia[] =
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Negative[] =
{
	//Negative
	SensorEnd
};
static	struct rk_sensor_reg sensor_Effect_Bluish[] =
{
	// Bluish
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Green[] =
{
	//	Greenish
	SensorEnd
};
static struct rk_sensor_reg *sensor_EffectSeqe[] = {sensor_Effect_Normal, sensor_Effect_WandB, sensor_Effect_Negative,sensor_Effect_Sepia,
	sensor_Effect_Bluish, sensor_Effect_Green,NULL,
};

static	struct rk_sensor_reg sensor_Exposure0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure2[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure3[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure4[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure5[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure6[]=
{
	SensorEnd
};

static struct rk_sensor_reg *sensor_ExposureSeqe[] = {sensor_Exposure0, sensor_Exposure1, sensor_Exposure2, sensor_Exposure3,
	sensor_Exposure4, sensor_Exposure5,sensor_Exposure6,NULL,
};

static	struct rk_sensor_reg sensor_Saturation0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation2[]=
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_SaturationSeqe[] = {sensor_Saturation0, sensor_Saturation1, sensor_Saturation2, NULL,};

static	struct rk_sensor_reg sensor_Contrast0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast2[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast3[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast4[]=
{
	SensorEnd
};


static	struct rk_sensor_reg sensor_Contrast5[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast6[]=
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_ContrastSeqe[] = {sensor_Contrast0, sensor_Contrast1, sensor_Contrast2, sensor_Contrast3,
	sensor_Contrast4, sensor_Contrast5, sensor_Contrast6, NULL,
};
static	struct rk_sensor_reg sensor_SceneAuto[] =
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_SceneNight[] =
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_SceneSeqe[] = {sensor_SceneAuto, sensor_SceneNight,NULL,};

static struct rk_sensor_reg sensor_Zoom0[] =
{
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom1[] =
{
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom2[] =
{
	SensorEnd
};


static struct rk_sensor_reg sensor_Zoom3[] =
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_ZoomSeqe[] = {sensor_Zoom0, sensor_Zoom1, sensor_Zoom2, sensor_Zoom3, NULL,};

/*
* User could be add v4l2_querymenu in sensor_controls by new_usr_v4l2menu
*/
static struct v4l2_querymenu sensor_menus[] =
{
};
/*
* User could be add v4l2_queryctrl in sensor_controls by new_user_v4l2ctrl
*/
static struct sensor_v4l2ctrl_usr_s sensor_controls[] =
{
};

//MUST define the current used format as the first item   
static struct rk_sensor_datafmt sensor_colour_fmts[] = {
	{V4L2_MBUS_FMT_SBGGR10_1X10, V4L2_COLORSPACE_JPEG}
};
static struct soc_camera_ops sensor_ops;


/*
**********************************************************
* Following is local code:
* 
* Please codeing your program here 
**********************************************************
*/
	
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
static __attribute__((always_inline)) unsigned char gamma_pixel(unsigned int tmp)
{
    #if 0
    if (tmp>96) {
	    if (tmp<=144) {
		    tmp = (257*tmp + 12367)>>8;
		} else if (tmp<=194) {
            tmp = (161*tmp + 26239)>>8;	
		} else if(tmp<=240) {
		    tmp = (128*tmp + 32768)>>8;
		}
	} else {							
	    if(tmp>64) {			    
		    tmp = (406*tmp - 1197)>>8;
		} else if(tmp>32) {
		    tmp = (510*tmp - 8874)>>8;
			} else if(tmp>24) {
		    tmp = (271*tmp - 807)>>8;
		} else if(tmp>2) {
		    tmp = (243*tmp - 279)>>8;
		}
    }

	if(tmp>0xff)
		tmp=0xff;
	return tmp;
	#endif

    int left,right,mid,k;
    struct gamma_line_t *line;
	int* in = sensor_gamma_info.gamma_in_array;
	int mid_fix = (sensor_gamma_info.gamma_count-1)/2;
	
	left =0;
	right = sensor_gamma_info.gamma_count-1;

	if(tmp<in[mid_fix])
		right = mid_fix;
	else 
		left = mid_fix;	
	mid = (left+right)>>1;

	if(tmp<in[mid])
		right = mid;
	else 
		left = mid ;

	for(k=left; k<right; k++){
		if(tmp>=in[k] && tmp<=in[k+1]){
			line = sensor_gamma_info.gamma_line + k;
			tmp = ((line->A*tmp + line->C)>>10);
			break;
		}
	}
	
	if(tmp>0xff)
		tmp=0xff;

    return tmp;
	
}
static int gamma_init(unsigned char *gamma_table)
{
	int ret = 0;
	int i;

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

	for (i=0; i<256; i++) {	    
        gamma_table[i] = gamma_pixel(i);
	}
    
    return ret;	
}
static int ag_ctrl(struct specific_sensor *spsensor, struct i2c_client* client, int now_ae)
{
	int GainVal;
	u8 GainVal0, GainVal1;
	int dst_gain;
	int ret=0,i,rec;
	struct rk_sensor_reg *init_data;

	sensor_read(client, 0x350a, &GainVal0);
	sensor_read(client, 0x350b, &GainVal1);
	GainVal = (((GainVal0 & 0x03)<<8) + GainVal1);

	dst_gain = sensor_ae_list_seqe[SENSOR_AE_LIST_INDEX][spsensor->ae_work.n].gain;

	if(dst_gain > 0x3ff)
		dst_gain = 0x3ff;

	GainVal0 = dst_gain>>8;
	GainVal1 = dst_gain&0xff;

	if(GainVal != dst_gain){
		sensor_write(client, 0x350a, GainVal0);
		sensor_write(client, 0x350b, GainVal1);
	}

	init_data = sensor_init_data;
    rec =0;
    for (i=0;i<sizeof(sensor_init_data)/sizeof(struct rk_sensor_reg);i++,init_data++) {
        if (init_data->reg == 0x350a) {
            init_data->val = GainVal0;
            rec++;
        }

        if (init_data->reg == 0x350b) {
            init_data->val = GainVal1;
            rec++;
        }
        if (rec>=2)
            break;
    }


	return 0;
}

static int ae_ctrl(struct specific_sensor *spsensor, struct i2c_client* client, int now_ae)
{
	int ExpRegVal;
	u8 ExpRegVal0, ExpRegVal1, ExpRegVal2;
	int dst_exp_val;
	int ret=0,i,rec;
	struct rk_sensor_reg *init_data;
	
	sensor_read(client, 0x3500, &ExpRegVal0);
	sensor_read(client, 0x3501, &ExpRegVal1);
	sensor_read(client, 0x3502, &ExpRegVal2);
	ExpRegVal = ((ExpRegVal0 & 0x0f)<<12) + (ExpRegVal1<<4) + ((ExpRegVal2 & 0xf0)>>4);

	dst_exp_val = sensor_ae_list_seqe[SENSOR_AE_LIST_INDEX][spsensor->ae_work.n].exposure_line;
	ExpRegVal0 = (dst_exp_val & 0xff0000)>>16;
	ExpRegVal1 = (dst_exp_val & 0xff00)>>8;
	ExpRegVal2 = (dst_exp_val & 0xff);

	//printk("now exposure line (%d), next exposure line (%d)\n", ExpRegVal, dst_exp_val>>4);

	if(dst_exp_val>>4 != ExpRegVal){
		sensor_write(client, 0x3500, ExpRegVal0);
		sensor_write(client, 0x3501, ExpRegVal1);
		sensor_write(client, 0x3502, ExpRegVal2);
	}

	init_data = sensor_init_data;
    rec =0;
    for (i=0;i<sizeof(sensor_init_data)/sizeof(struct rk_sensor_reg);i++,init_data++) {
        if (init_data->reg == 0x3500) {
            init_data->val = ExpRegVal0;
            rec++;
        }

        if (init_data->reg == 0x3501) {
            init_data->val = ExpRegVal1;
            rec++;
        }

        if (init_data->reg == 0x3502) {
            init_data->val = ExpRegVal2;
            rec++;
        }
        if (rec>=3)
            break;
    }
	return 0;	
}

static int aeag_index(struct specific_sensor *spsensor,int value)
{    
    int diff;
    int flag = 0;
    int n=0;
    int temp_index= spsensor->ae_work.n;

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

    spsensor->ae_work.n = temp_index;
    return temp_index;

}

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
		//SENSOR_DG("%s..%d.. count(%d)\n", __FUNCTION__, __LINE__, my_work->count);
	
		goto FUNCRET;
	}

	if(my_work->count>3 &&((my_work->count)&0x01)!=0)
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
		//SENSOR_DG("%s..%s..%d MAX_I(%d)!!!!!!!!!!!!!!!!!!!!!!\n", SENSOR_NAME_STRING(), __FUNCTION__, __LINE__, my_work->i_current);

		do{
			ret = vcm_client->driver->command(vcm_client, StepFocus_Set_CurrentStep_Tag, &step);
		}while(ret);
		
		my_work->fix_EV = my_work->max_mft;
		my_work->count = 0;
		my_work->max_mft = 0;
		my_work->engry_cnt = 0;

        #if 0
		SENSOR_DG("coarse control++++++++++++++++++\n\n");
		for(i=0; i<15; i++)
		{
			SENSOR_DG("%ld, ", my_work->engry[i]);
		}
		SENSOR_DG("\n\ncoarse control++++++++++++++++++\n\n");
		#endif
		
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
		icurrent = my_work->i_current - 40;
		do{ 
			ret = vcm_client->driver->command(vcm_client, StepFocus_Set_Current_Tag, &(icurrent));  
		}while(ret);

		if(my_work->i_current + 40 > MAX_CURRENT)
			my_work->i_current  = MAX_CURRENT;
		else
		    my_work->i_current = my_work->i_current + 40;
		
		//SENSOR_DG("%s..%d.. count(%d)\n", __FUNCTION__, __LINE__, my_work->count);
		goto FUNCRET;
	}

	if(my_work->count>3 &&((my_work->count)&0x01)!=0){
		my_work->engry[my_work->engry_cnt] = my_work->EV;
		my_work->engry_cnt++;
		
		if(my_work->EV > my_work->max_mft)
		{
			my_work->max_mft = my_work->EV;
			my_work->max_current = param->present_code;
		}	
		
		if(param->present_code < my_work->i_current ){	
			do{
				ret = vcm_client->driver->command(vcm_client, StepFocus_Far_Tag, 0);
			}while(ret);
			goto FUNCRET;
		} 			

		do{ 
			ret = vcm_client->driver->command(vcm_client, StepFocus_Set_Current_Tag, &(my_work->max_current));
		}while(ret);
		
		//SENSOR_DG("%s..%s..%d turn to coarse control I(%d)\n", SENSOR_NAME_STRING(), __FUNCTION__, __LINE__, my_work->max_current);

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

		g_af_info.af_vcm_state = vcm_state_focus_complete;

        #if 0
		SENSOR_DG("fine control-----------------\n\n");
		for(i=0; i<10; i++)
		{
			SENSOR_DG("%ld, ", my_work->engry[i]);
		}
		SENSOR_DG("\n\nfine control-----------------\n\n");
		#endif
	
	}

FUNCRET:
    #if 0
	do_gettimeofday(&t1);  
	SENSOR_DG("fine control (%ld)us\n", (t1.tv_sec*1000000 + t1.tv_usec) - (t0.tv_sec*1000000 + t0.tv_usec));
	#endif
	
	return ret;
}

static void func_vcm_control(struct work_struct* work)
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
	//SENSOR_DG("VCM: %ld us\n", (t1.tv_sec*1000000 + t1.tv_usec) - (t0.tv_sec*1000000 + t0.tv_usec));
	
}

static int isp_af_zoneupdate_cb(rk_cb_para_t *para)
{
    para->af_zone_centre_w = g_af_info.zone_info.centre_w;
    para->af_zone_centre_h = g_af_info.zone_info.centre_h;
    return 0;
}

static int sensor_ioctl_ext_cb(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	int lumi;
	int *lens_para;
    long fix_ev, tmp_ev, differ_ev;
	rk29_camera_sensor_cb_s *icd_cb =NULL;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct generic_sensor *sensor = to_generic_sensor(client);  
	struct specific_sensor *spsensor = to_specific_sensor(sensor);
    struct rk_raw_ret  *raw_ret;
    int *ccm_matrix_para;
    u8 ExpRegVal0,ExpRegVal1,ExpRegVal2,GainVal0,GainVal1;
    int ExpRegVal,GainVal;
	switch (cmd)
	{
        case RK29_CAM_SUBDEV_CALIBRATION_IMG:
        {
			lens_para = (int *)arg;
			lens_para[0] = LENS_CENTRE_X;
			lens_para[1] = LENS_CENTRE_Y;
			lens_para[2] = LENS_R_B2;
			lens_para[3] = LENS_R_B4;
			lens_para[4] = LENS_G_B2;
			lens_para[5] = LENS_G_B4;
			lens_para[6] = LENS_B_B2;
			lens_para[7] = LENS_B_B4;
			lens_para[8] = LENS_FULL_WIDTH;
			lens_para[9] = LENS_FULL_HEIGHT;
            break;
        }
        case RK29_CAM_SUBDEV_GAMMA:
		{
			icd_cb = (rk29_camera_sensor_cb_s*)(arg);
            //icd_cb->gamma.cb = gamma_cb;

            icd_cb->gamma_table = spsensor->gamma_table;
            
			break;
        }
		case RK29_CAM_SUBDEV_COLOR_CORRECTION:
		{
		    #if 0
			icd_cb = (rk29_camera_sensor_cb_s*)(arg);
            icd_cb->color_correction.cb = color_correction_cb;
            #endif

            ccm_matrix_para = (int *)arg;
            ccm_matrix_para[0] =  saturation;
            ccm_matrix_para[1] =  contrast;
            ccm_matrix_para[2] =  bright;
            ccm_matrix_para[3] =  0;
            memcpy(ccm_matrix_para+4, ccm_matrix, 16*sizeof(int));
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
				ret = aeag_index(spsensor,lumi); 
			    ret = ae_ctrl(spsensor, client, lumi);	
				ret = ag_ctrl(spsensor, client, lumi);
			}
			break;
		}

        case RK29_CAM_SUBDEV_AF:
		{
		    #if CONFIG_OV5647_VCM_DRIVER_A3907
		    if(g_af_info.af_vcm_state ==vcm_state_busy && g_af_info.af_command == StepFocus_Single_Tag){
                raw_ret = (struct rk_raw_ret*)arg;
                tmp_ev = (int)raw_ret->af_EV;
    			spsensor->vcm_work_wait.EV = tmp_ev;
    			queue_work(spsensor->vcm_wq, &(spsensor->vcm_work_wait.vcm_work));	

			}
			#endif
			
            break;
		}

        case RK29_CAM_SUBDEV_AF_ZONE:
        {
            icd_cb = (rk29_camera_sensor_cb_s*)(arg);
            icd_cb->af_zone_update.cb = isp_af_zoneupdate_cb;
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


/*
**********************************************************
* Following is callback
* If necessary, you could coding these callback
**********************************************************
*/
/*
* the function is called in open sensor  
*/
static int sensor_activate_cb(struct i2c_client *client)
{
    //SENSOR_DG("%s",__FUNCTION__);
	
	
	return 0;
}
/*
* the function is called in close sensor
*/
static int sensor_deactivate_cb(struct i2c_client *client)
{
	u8 reg_val;
	struct generic_sensor *sensor = to_generic_sensor(client);

    //SENSOR_DG("%s",__FUNCTION__);

	/* ddl@rock-chips.com : all sensor output pin must switch into Hi-Z */
	if (sensor->info_priv.funmodule_state & SENSOR_INIT_IS_OK) {
		sensor_read(client,0x3000,&reg_val);
		sensor_write(client, 0x3000, reg_val&0xfc);
		sensor_write(client, 0x3001, 0x00);
		sensor_write(client, 0x3002, 0x00);
		sensor->info_priv.funmodule_state &= ~SENSOR_INIT_IS_OK;
	}
	
	return 0;
}

static int sensor_parameter_record(struct i2c_client *client)
{
	u8 ret_l,ret_m,ret_h;
	int tp_l,tp_m,tp_h;
	int gain_l,gain_h;
	struct generic_sensor*sensor = to_generic_sensor(client);
	struct specific_sensor *spsensor = to_specific_sensor(sensor);
	//sensor_read(client,0x3a00, &ret_l);
	//sensor_write(client,0x3a00, ret_l&0xfb);

	sensor_write(client,0x3503,0x03);	//stop AE/AG
	//sensor_read(client,0x3406, &ret_l);
	//sensor_write(client,0x3406, ret_l|0x01);

	sensor_read(client,0x3500,&ret_h);
	sensor_read(client,0x3501, &ret_m);
	sensor_read(client,0x3502, &ret_l);
	tp_l = ret_l;
	tp_m = ret_m;
	tp_h = ret_h;
	spsensor->parameter.preview_exposure = ((tp_h<<12) & 0xF000) | ((tp_m<<4) & 0x0FF0) | ((tp_l>>4) & 0x0F);
	
	//Read back AGC Gain for preview
	sensor_read(client,0x350a, &ret_h);
	gain_h = ret_h;
	sensor_read(client,0x350b, &ret_l);
	gain_l = ret_l;
	spsensor->parameter.preview_gain = (((gain_h<<8)&0x300) | (gain_l&0xff))>>4;
	
	//printk(" %s Read preview_gain:%x 0x350a=0x%02x 0x350b=0x%02x  PreviewExposure:%x 0x3500=0x%02x  0x3501=0x%02x 0x3502=0x%02x \n",
	 //SENSOR_NAME_STRING(), spsensor->parameter.preview_gain,gain_h,gain_l,spsensor->parameter.preview_exposure,tp_h, tp_m, tp_l);
	return 0;
}
#define OV5640_FULL_PERIOD_PIXEL_NUMS_HTS		  (2700) 
#define OV5640_FULL_PERIOD_LINE_NUMS_VTS		  (1974) 
#define OV5640_PV_PERIOD_PIXEL_NUMS_HTS 		  (1896) 
#define OV5640_PV_PERIOD_LINE_NUMS_VTS			  (984) 
static int sensor_ae_transfer(struct i2c_client *client)
{
	u8	ExposureLow;
	u8	ExposureMid;
	u8	ExposureHigh;
	u16 ulCapture_Exposure;
	u8	GainLow,GainHigh;
	u16 ulCapture_Gain;
	int ratio;
	struct generic_sensor*sensor = to_generic_sensor(client);
	struct specific_sensor *spsensor = to_specific_sensor(sensor);

	ulCapture_Gain = spsensor->parameter.preview_gain;
	
	ulCapture_Exposure = (spsensor->parameter.preview_exposure*OV5640_PV_PERIOD_PIXEL_NUMS_HTS)/OV5640_FULL_PERIOD_PIXEL_NUMS_HTS;
	ulCapture_Exposure = (ulCapture_Exposure*spsensor->parameter.CapturePclk)/spsensor->parameter.PreviewPclk;
	//SENSOR_TR("cap shutter calutaed = %d, 0x%x\n", ulCapture_Exposure,ulCapture_Exposure);
	

    if(ulCapture_Exposure>OV5640_FULL_PERIOD_PIXEL_NUMS_HTS)
        ulCapture_Exposure = OV5640_FULL_PERIOD_PIXEL_NUMS_HTS -4;

    ratio = OV5640_FULL_PERIOD_PIXEL_NUMS_HTS/ulCapture_Exposure;

    if(ratio>=8){
        ulCapture_Gain >>= 3;
        ulCapture_Exposure <<= 3;
    }else if(ratio>=4){
        ulCapture_Gain >>= 2;
        ulCapture_Exposure <<= 2;
    }else if(ratio>=2){
        ulCapture_Gain >>= 1;
        ulCapture_Exposure <<= 1;
    }	

    ulCapture_Gain <<= 4; 
    ulCapture_Exposure <<= 4;
    GainLow = (ulCapture_Gain)&0xff;
	GainHigh = (ulCapture_Gain>>8)&0x03;
    ExposureLow = (ulCapture_Exposure)&0xff;
	ExposureMid = (ulCapture_Exposure>>8)&0xff;
	ExposureHigh = (ulCapture_Exposure>>16)&0xff; 

	sensor_write(client,0x3502, ExposureLow);
	sensor_write(client,0x3501, ExposureMid);
	sensor_write(client,0x3500, ExposureHigh);
	sensor_write(client,0x350b, GainLow);
	sensor_write(client,0x350a, GainHigh);

	//SENSOR_TR(" %s Write gain:0x%x 0x350b=0x%02x  0x350a=0x%2x  exposure:0x%x 0x3502=0x%02x 0x3501=0x%02x 0x3500=0x%02x\n",SENSOR_NAME_STRING(), ulCapture_Gain,GainLow, GainHigh,ulCapture_Exposure,ExposureLow, ExposureMid, ExposureHigh);
	mdelay(100);
	return 0;
}

/*
* the function is called before sensor register setting in VIDIOC_S_FMT  
*/
static int sensor_s_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{
	//struct generic_sensor*sensor = to_generic_sensor(client);

	#if OV5647_USE_30_OR_15_FPS
	if (capture) {
		sensor_parameter_record(client);
	}
	#endif

	g_af_info.is_capture = capture;
	return 0;
}
/*
* the function is called after sensor register setting finished in VIDIOC_S_FMT  
*/
static int sensor_s_fmt_cb_bh (struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{
    struct generic_sensor *sensor = to_generic_sensor(client);  
    struct specific_sensor *spsensor = to_specific_sensor(sensor);

    #if OV5647_USE_30_OR_15_FPS
	if (capture) {
		sensor_ae_transfer(client);
	}else{
        ae_ctrl(spsensor, client, 0);
        ag_ctrl(spsensor, client, 0);
	}
	#endif
	
	return 0;
}

static int sensor_try_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf)
{
	return 0;
}

static int sensor_softrest_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{
	
	return 0;
}
static int sensor_check_id_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{
	return 0;
}

static int sensor_suspend(struct soc_camera_device *icd, pm_message_t pm_msg)
{
	//struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
		
	if (pm_msg.event == PM_EVENT_SUSPEND) {
		SENSOR_DG("Suspend");
		
	} else {
		SENSOR_TR("pm_msg.event(0x%x) != PM_EVENT_SUSPEND\n",pm_msg.event);
		return -EINVAL;
	}
	return 0;
}

static int sensor_resume(struct soc_camera_device *icd)
{

	SENSOR_DG("Resume");

	return 0;

}
static int sensor_mirror_cb (struct i2c_client *client, int mirror)
{
	char val;
	int err = 0;
    
    //SENSOR_DG("mirror: %d",mirror);
	if (mirror) {
		err = sensor_read(client, 0x3821, &val);
		if (err == 0) {
			val |= 0x06;
			err = sensor_write(client, 0x3821, val);
		}
	} else {
		err = sensor_read(client, 0x3821, &val);
		if (err == 0) {
			val &= 0xf9;
			err = sensor_write(client, 0x3821, val);
		}
	}

	return err;    
}
/*
* the function is v4l2 control V4L2_CID_HFLIP callback  
*/
static int sensor_v4l2ctrl_mirror_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info, 
                                                     struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

    if (sensor_mirror_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_mirror failed, value:0x%x",ext_ctrl->value);
	
	SENSOR_DG("sensor_mirror success, value:0x%x",ext_ctrl->value);
	return 0;
}

static int sensor_flip_cb(struct i2c_client *client, int flip)
{
	char val;
	int err = 0;	


    //SENSOR_DG("flip: %d",flip);
	if (flip) {
		err = sensor_read(client, 0x3820, &val);
		if (err == 0) {
			val |= 0x06;
			err = sensor_write(client, 0x3820, val);
		}
	} else {
		err = sensor_read(client, 0x3820, &val);
		if (err == 0) {
			val &= 0xf9;
			err = sensor_write(client, 0x3820, val);
		}
	}

	return err;       
}
/*
* the function is v4l2 control V4L2_CID_VFLIP callback  
*/
static int sensor_v4l2ctrl_flip_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info, 
                                                     struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

    if (sensor_flip_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_flip failed, value:0x%x",ext_ctrl->value);
	
	SENSOR_DG("sensor_flip success, value:0x%x",ext_ctrl->value);
	return 0;
}

static int sensor_focus_init_usr_cb(struct i2c_client *client){
    struct generic_sensor *sensor = to_generic_sensor(client);
    struct specific_sensor *spsensor = to_specific_sensor(sensor);

    g_af_info.af_vcm_state = vcm_state_idle;
    g_af_info.zone_info.centre_w=400;
    g_af_info.zone_info.centre_h=300;
    
	return 0;
}

static int sensor_focus_af_single_usr_cb(struct i2c_client *client){
    int ret=0;
    int cnt = 0;
    struct generic_sensor *sensor = to_generic_sensor(client);
    struct specific_sensor *spsensor = to_specific_sensor(sensor);
    
    if((g_af_info.af_vcm_state == vcm_state_idle) && (g_af_info.is_capture==0)){
        g_af_info.af_vcm_state = vcm_state_busy;
        g_af_info.af_command = StepFocus_Single_Tag;
    }else{
        ret=-1;
        goto af_signal_end;
    }

    cnt = 0;
    do{
        msleep(10);
        cnt++;
    }while(g_af_info.af_vcm_state==vcm_state_busy && cnt<400);

    if(g_af_info.af_vcm_state!=vcm_state_focus_complete){
        ret=-1;
    }
    g_af_info.af_vcm_state = vcm_state_idle;
    spsensor->vcm_work_wait.count = 0;
    spsensor->vcm_work_wait.wait_cnt = 0;
    spsensor->vcm_work_wait.wait_flag = -1;
    
 af_signal_end:     
	return ret;
}

static int sensor_focus_af_near_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_far_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_specialpos_usr_cb(struct i2c_client *client,int pos){
	return 0;
}

static int sensor_focus_af_const_usr_cb(struct i2c_client *client){
	return 0;
}
static int sensor_focus_af_const_pause_usr_cb(struct i2c_client *client)
{
    return 0;
}
static int sensor_focus_af_close_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_zoneupdate_usr_cb(struct i2c_client *client, int *zone_tm_pos)
{
    int ret = 0;  
    int centre_w;
    int centre_h;
    struct generic_sensor *sensor = to_generic_sensor(client);
    struct specific_sensor *spsensor = to_specific_sensor(sensor);
	
	if (zone_tm_pos) {
		zone_tm_pos[0] += 1000;
		zone_tm_pos[1] += 1000;
		zone_tm_pos[2]+= 1000;
		zone_tm_pos[3] += 1000;
		centre_w = ((zone_tm_pos[0] + zone_tm_pos[2])>>1)*800/2000;
		centre_h = ((zone_tm_pos[1] + zone_tm_pos[3])>>1)*600/2000;
	} else {
#if CONFIG_SENSOR_FocusCenterInCapture
		centre_w = 400;
		centre_h = 300;
#else
		centre_w = -1;
		centre_h = -1;
#endif
	}

	if(g_af_info.af_vcm_state==vcm_state_idle){
        g_af_info.zone_info.centre_w = centre_w;
        g_af_info.zone_info.centre_h = centre_h;
        ret = 0;
	}else{
        ret = -1;
	}
	
sensor_af_zone_end:
	return ret;
}

/*
face defect call back
*/
static int 	sensor_face_detect_usr_cb(struct i2c_client *client,int on){
	return 0;
}

/*
*   The function can been run in sensor_init_parametres which run in sensor_probe, so user can do some
* initialization in the function. 
*/
static void sensor_init_parameters_user(struct specific_sensor* spsensor,struct soc_camera_device *icd)
{
	struct i2c_adapter *vcm_adapter;
    struct soc_camera_link *icl;
    struct rkcamera_platform_data *sensor_device=NULL,*new_camera;
	struct rk29camera_platform_data *pdata=NULL;
	struct rk_vcm_device_info* vcm_info=NULL;
	int ret,i;

	spsensor->common_sensor.sensor_cb.sensor_ioctl_ext_cb = sensor_ioctl_ext_cb;
	
	spsensor->ae_work.n = AE_INDEX_INIT;
	spsensor->ae_work.exposure_line = AE_VALUE_INIT;
	spsensor->ae_work.gain = AG_VALUE_INIT;
	spsensor->parameter.CapturePclk = 96;
	spsensor->parameter.PreviewPclk = 56;

	g_af_info.zone_info.centre_h = -1;
	g_af_info.zone_info.centre_w = -1;

	gamma_init(spsensor->gamma_table); 
	
	icl = to_soc_camera_link(icd);
    if (!icl) {
        SENSOR_TR("%s driver needs platform data\n", SENSOR_NAME_STRING());
		return;
    }

	#if CONFIG_OV5647_VCM_DRIVER_A3907
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
	
	if (vcm_info == NULL) {
        SENSOR_TR(KERN_ERR "%s(%d): vcm_info is NULL!",SENSOR_NAME_STRING(),__LINE__);
        BUG();
	} else {
        if (vcm_info->vcm_i2c_board_info.addr == 0) {
            SENSOR_DG("%s(%d): this ov5647 module is not build with vcm",__FUNCTION__,__LINE__);
            goto end;
        }
	}
		
	spsensor->vcm_wq = create_workqueue(SENSOR_NAME_STRING(_vcm_workqueue));	
	if (spsensor->vcm_wq == NULL)
		SENSOR_TR("%s create fail!", SENSOR_NAME_STRING(_vcm_workqueue)); 
		
	vcm_adapter = i2c_get_adapter(vcm_info->vcm_i2c_adapter_id);
	if(!vcm_adapter)
		SENSOR_TR("%s..%s..%d get vcm-adapter fail!", SENSOR_NAME_STRING(), __FUNCTION__, __LINE__);
		
	spsensor->vcm_work_wait.vcm_client = i2c_new_device(vcm_adapter, &(vcm_info->vcm_i2c_board_info));
 	if(spsensor->vcm_work_wait.vcm_client == NULL) 
		SENSOR_TR("%s..%s..%d vcm client create fail!", SENSOR_NAME_STRING(), __FUNCTION__, __LINE__);

    if (spsensor->vcm_work_wait.vcm_client->driver == NULL)
        SENSOR_TR("%s..%s..%d vcm driver attach failed!", SENSOR_NAME_STRING(), __FUNCTION__, __LINE__);
        
	INIT_WORK(&(spsensor->vcm_work_wait.vcm_work), func_vcm_control);
	#endif

end:        
    return;
}

/*
* :::::WARNING:::::
* It is not allowed to modify the following code
*/

sensor_init_parameters_default_code();

sensor_v4l2_struct_initialization();

sensor_probe_default_code();

sensor_remove_default_code();

sensor_driver_default_module_code();






