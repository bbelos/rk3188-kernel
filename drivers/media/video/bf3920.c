
#include "generic_sensor.h"

/*
*      Driver Version Note
*v0.0.1: this driver is compatible with generic_sensor
*v0.0.3:
*        add sensor_focus_af_const_pause_usr_cb;
*/
static int version = KERNEL_VERSION(0,0,3);
module_param(version, int, S_IRUGO);


static int debug;
module_param(debug, int, S_IRUGO|S_IWUSR);

#define dprintk(level, fmt, arg...) do {			\
	if (debug >= level) 					\
	printk(KERN_WARNING fmt , ## arg); } while (0)

/* Sensor Driver Configuration Begin */
#define SENSOR_NAME RK29_CAM_SENSOR_BF3920
#define SENSOR_V4L2_IDENT V4L2_IDENT_BF3920
#define SENSOR_ID 0x39
#define SENSOR_BUS_PARAM					 (SOCAM_MASTER |\
											 SOCAM_PCLK_SAMPLE_RISING|SOCAM_HSYNC_ACTIVE_HIGH| SOCAM_VSYNC_ACTIVE_HIGH|\
											 SOCAM_DATA_ACTIVE_HIGH | SOCAM_DATAWIDTH_8  |SOCAM_MCLK_24MHZ)
#define SENSOR_PREVIEW_W					 800
#define SENSOR_PREVIEW_H					 600
#define SENSOR_PREVIEW_FPS					 15000	   // 15fps 
#define SENSOR_FULLRES_L_FPS				 7500	   // 7.5fps
#define SENSOR_FULLRES_H_FPS				 7500	   // 7.5fps
#define SENSOR_720P_FPS 					 0
#define SENSOR_1080P_FPS					 0

#define SENSOR_REGISTER_LEN 				 1		   // sensor register address bytes
#define SENSOR_VALUE_LEN					 1		   // sensor register value bytes
static unsigned int SensorConfiguration = (CFG_WhiteBalance|CFG_Effect|CFG_Scene);
static unsigned int SensorChipID[] = {SENSOR_ID};
/* Sensor Driver Configuration End */


#define SENSOR_NAME_STRING(a) STR(CONS(SENSOR_NAME, a))
#define SENSOR_NAME_VARFUN(a) CONS(SENSOR_NAME, a)

#define SensorRegVal(a,b) CONS4(SensorReg,SENSOR_REGISTER_LEN,Val,SENSOR_VALUE_LEN)(a,b)
#define sensor_write(client,reg,v) CONS4(sensor_write_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_read(client,reg,v) CONS4(sensor_read_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_write_array generic_sensor_write_array

struct sensor_parameter
{
	unsigned int PreviewDummyPixels;
	unsigned int CaptureDummyPixels;
	unsigned int preview_exposure;
	unsigned short int preview_line_width;
	unsigned short int preview_gain;

	unsigned short int PreviewPclk;
	unsigned short int CapturePclk;
	char awb[6];
};

struct specific_sensor{
	struct generic_sensor common_sensor;
	//define user data below
	struct sensor_parameter parameter;
	u16 shutter;

};

/*
*  The follow setting need been filled.
*  
*  Must Filled:
*  sensor_init_data :				Sensor initial setting;
*  sensor_fullres_lowfps_data : 	Sensor full resolution setting with best auality, recommand for video;
*  sensor_preview_data :			Sensor preview resolution setting, recommand it is vga or svga;
*  sensor_softreset_data :			Sensor software reset register;
*  sensor_check_id_data :			Sensir chip id register;
*
*  Optional filled:
*  sensor_fullres_highfps_data: 	Sensor full resolution setting with high framerate, recommand for video;
*  sensor_720p: 					Sensor 720p setting, it is for video;
*  sensor_1080p:					Sensor 1080p setting, it is for video;
*
*  :::::WARNING:::::
*  The SensorEnd which is the setting end flag must be filled int the last of each setting;
*/

/* Sensor initial setting */
static struct rk_sensor_reg sensor_init_data[] =
{
	{0x1b,0x2c},
	{0x11,0x30},//2e
	{0x8a,0x78},//0xc1
	{0x2a,0x0c},
	{0x2b,0x90},
	{0x92,0x02},
	{0x93,0x00},
	/*
	{0x15,0x82},//----00      	            
	{0x4a,0x82},
	{0xcc,0x00},
	{0xca,0x00},
	{0xcb,0x00},
	{0xcf,0x00},
	{0xcd,0x00},
	{0xce,0x00},
	{0xc0,0xa1},
	{0xc1,0x84},
	{0xc2,0xe4},
	{0xc3,0x11},
	{0xc4,0x3f},
	{0xb5,0x3f},
	{0x03,0x20},
	{0x17,0x00},
	{0x18,0x80},
	{0x10,0x10},
	{0x19,0x00},
	{0x1a,0xe0},
	{0x0B,0x00},
	*/
	
	{0x09,0x42},
        {0x21,0x15},//0x1f
	{0x15,0x82},//----00
	{0x3a,0x02},
	
	//initial AE and AWB
	{0x13,0x00},
	//{0x01,0x12},
	//{0x02,0x22},
	{0x24,0xc8},
	{0x87,0x2d},
	{0x13,0x07},
	
	//black level  
	{0x27,0x98},
	{0x28,0xff},
	{0x29,0x60},
	
	//black target  
	{0x1F,0x20},
	{0x22,0x20},
	{0x20,0x20},
	{0x26,0x20},
	
	//analog
	{0xe2,0xc4},
	{0xe7,0x4e},
	{0x08,0x16},
	//{0x16,0x28},
	//{0x1c,0x00},
	{0x16,0xfa},//new para
	{0x1c,0xc0},//new para
	{0x1d,0xf1},
	{0xbb,0x30},
	{0xf9,0x00},
	{0xed,0x8f},
	{0x3e,0x80},
	
	//Shading
	{0x1e,0x46},
	{0x35,0x30},
	{0x65,0x30},
	{0x66,0x2a},
	{0xbc,0xd3},
	{0xbd,0x5c},
	{0xbe,0x24},
	{0xbf,0x13},
	{0x9b,0x5c},
	{0x9c,0x24},
	{0x36,0x13},
	{0x37,0x5c},
	{0x38,0x24},
	
	//denoise
	{0x70,0x01},
	{0x72,0x62},//62
	{0x78,0x37},
	{0x7a,0x28},//0x29
	{0x7d,0x85},
	
	// AE
	{0x13,0x07},
	{0x24,0x4f},//48
	{0x25,0x88},
	{0x97,0x30},
	{0x98,0x0a},
	{0x80,0x96},//0x92
	{0x81,0xe0},
	{0x82,0x30},
	{0x83,0x60},
	{0x84,0x73},//65//78
	{0x85,0x90},
	{0x86,0xc0},
	{0x89,0x55},
	{0x94,0x82},//42
	
	//Gamma default
	{0x3f,0x2c},//20
	{0x39,0xac},//a0
	
	{0x40,0x20},
	{0x41,0x25},
	{0x42,0x23},
	{0x43,0x1f},
	{0x44,0x18},
	{0x45,0x15},
	{0x46,0x12},
	{0x47,0x10},
	{0x48,0x10},
	{0x49,0x0f},
	{0x4b,0x0e},
	{0x4c,0x0c},
	{0x4e,0x0b},
	{0x4f,0x0a},
	{0x50,0x07},
	
	/*
	//gamma 过曝过度好，高亮                                                       
	{0x40,0x28},            
	{0x41,0x28},            
	{0x42,0x30},            
	{0x43,0x29},            
	{0x44,0x23},            
	{0x45,0x1b},            
	{0x46,0x17},
	{0x47,0x0f},
	{0x48,0x0d},
	{0x49,0x0b},
	{0x4b,0x09},
	{0x4c,0x08},
	{0x4e,0x07},
	{0x4f,0x05},
	{0x50,0x04},
	{0x39,0xa0},
	{0x3f,0x20},
	    
	//gamma 清晰亮丽                                                           
	{0x40,0x28},
	{0x41,0x26},
	{0x42,0x24},
	{0x43,0x22},
	{0x44,0x1c},
	{0x45,0x19},
	{0x46,0x15},
	{0x47,0x11},
	{0x48,0x0f},
	{0x49,0x0e},
	{0x4b,0x0c},
	{0x4c,0x0a},
	{0x4e,0x09},
	{0x4f,0x08},
	{0x50,0x04},
	{0x39,0xa0},
	{0x3f,0x20},
	*/
	
	
	{0x5c,0x80},
	{0x51,0x22},
	{0x52,0x00},
	{0x53,0x96},
	{0x54,0x8C},
	{0x57,0x7F},
	{0x58,0x0B},
	{0x5a,0x14},
	/*
	//Color default
	{0x5c,0x00},
	{0x51,0x32},
	{0x52,0x17},
	{0x53,0x8C},
	{0x54,0x79},
	{0x57,0x6E},
	{0x58,0x01},
	{0x5a,0x36},
	{0x5e,0x38},
	
	*/                                                                                           
	//color 绿色好 
	{0x5c,0x00},                
	{0x51,0x24},                
	{0x52,0x0b},                
	{0x53,0x77},                
	{0x54,0x65},                
	{0x57,0x5e},                
	{0x58,0x19},                               
	{0x5a,0x16},                
	{0x5e,0x38},     
	/*
		
	//color 艳丽
	{0x5c,0x00},               
	{0x51,0x2b},               
	{0x52,0x0e},               
	{0x53,0x9f},               
	{0x54,0x7d},               
	{0x57,0x91},               
	{0x58,0x21},                              
	{0x5a,0x16},               
	{0x5e,0x38},
		                         
	
	//color 色彩淡                         
	{0x5c,0x00},             
	{0x51,0x24},             
	{0x52,0x0b},             
	{0x53,0x77},             
	{0x54,0x65},             
	{0x57,0x5e},             
	{0x58,0x19},                            
	{0x5a,0x16},             
	{0x5e,0x38},
	*/
	
	//AWB
	{0x6a,0x81},	
	{0x23,0x66},
	{0xa1,0x31},
	{0xa2,0x0b},
	{0xa3,0x25},
	{0xa4,0x09},
	{0xa5,0x26},
	{0xa7,0x9a},
	{0xa8,0x15},
	{0xa9,0x13},
	{0xaa,0x12},
	{0xab,0x16},
	{0xc8,0x10},
	{0xc9,0x15},
	{0xd2,0x78},
	{0xd4,0x20},
	
	//saturation
	{0x56,0x28},
	{0xb0,0x8d},
     //add
	{0x55,0x00},
	{0xb1,0x95},//0x95
	{0xb4,0x40},//default
     //add
	{0x56,0xe8},
	{0x55,0xf0},
	{0xb0,0xb8},
	
	{0x56,0xa8},
	{0x55,0xc8},
	{0xb0,0x98},

	{0x89,0x7d},
	//skin
	{0xee,0x2a},
	{0xef,0x1b}, 
	SensorEnd
};
/* Senor full resolution setting: recommand for capture */
static struct rk_sensor_reg sensor_fullres_lowfps_data[] ={
	//1600*1200
	{0x1b,0x2c},
	{0x11,0x30},//38
	{0x8a,0x78},//0XBF
	{0x2a,0x0c},
	{0x2b,0x90},
	{0x92,0x02},
	{0x93,0x00},
						
						
	//window 		
	{0x4a,0x80}, 
	{0xcc,0x00}, 
	{0xca,0x00}, 
	{0xcb,0x00}, 
	{0xcf,0x00}, 
	{0xcd,0x00}, 
	{0xce,0x00}, 
	{0xc0,0x00}, 
	{0xc1,0x00}, 
	{0xc2,0x00}, 
	{0xc3,0x00}, 
	{0xc4,0x00}, 
	{0xb5,0x00}, 
	{0x03,0x60}, 
	{0x17,0x00}, 
	{0x18,0x40}, 
	{0x10,0x40}, 
	{0x19,0x00}, 
	{0x1a,0xb0}, 
	{0x0B,0x00},
         SensorEnd 


};

/* Senor full resolution setting: recommand for video */
static struct rk_sensor_reg sensor_fullres_highfps_data[] ={
	SensorEnd
};
/* Preview resolution setting*/
static struct rk_sensor_reg sensor_preview_data[] =
{
	 //800*600
	{0x1b,0x2c},
	{0x11,0x30},//38
	{0x8a,0x78},//0XBF
	{0x2a,0x0c},
	{0x2b,0x90},
	{0x92,0x02},
	{0x93,0x00},
	     
	{0x4a,0x80},
	{0xcc,0x00},
	{0xca,0x00},
	{0xcb,0x00},
	{0xcf,0x00},
	{0xcd,0x00},
	{0xce,0x00},
	{0xc0,0x00},
	{0xc1,0x00},
	{0xc2,0x00},
	{0xc3,0x00},
	{0xc4,0x00},
	{0xb5,0x00},
	{0x03,0x60},
	{0x17,0x00},
	{0x18,0x40},
	{0x10,0x40},
	{0x19,0x00},
	{0x1a,0xb0},
	{0x0B,0x10},
	SensorEnd
};
/* 1280x720 */
static struct rk_sensor_reg sensor_720p[]={
	SensorEnd
};

/* 1920x1080 */
static struct rk_sensor_reg sensor_1080p[]={
	SensorEnd
};


static struct rk_sensor_reg sensor_softreset_data[]={
	
	SensorRegVal(0x12,80),
	SensorWaitMs(5),
	SensorEnd
};

static struct rk_sensor_reg sensor_check_id_data[]={
	SensorRegVal(0xfC,0),
	//SensorRegVal(0xfD,0),
	SensorEnd
};
/*
*  The following setting must been filled, if the function is turn on by CONFIG_SENSOR_xxxx
*/
static struct rk_sensor_reg sensor_WhiteB_Auto[]=
{
	{0x13, 0x07},
	SensorEnd
};
/* Cloudy Colour Temperature : 6500K - 8000K  */
static	struct rk_sensor_reg sensor_WhiteB_Cloudy[]=
{
	{0x13,0x05},
	{0x01,0x0e},
	{0x02,0x24},	
	SensorEnd
};
/* ClearDay Colour Temperature : 5000K - 6500K	*/
static	struct rk_sensor_reg sensor_WhiteB_ClearDay[]=
{
	{0x13,0x05},
	{0x01,0x10},
	{0x02,0x20},
	SensorEnd
};
/* Office Colour Temperature : 3500K - 5000K  */
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp1[]=
{
	{0x13,0x05},
	{0x01,0x1d},
	{0x02,0x16},
	SensorEnd

};
/* Home Colour Temperature : 2500K - 3500K	*/
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp2[]=
{
	{0x13,0x05},
	{0x01,0x28},
	{0x02,0x0a},
	SensorEnd
};
static struct rk_sensor_reg *sensor_WhiteBalanceSeqe[] = {sensor_WhiteB_Auto, sensor_WhiteB_TungstenLamp1,sensor_WhiteB_TungstenLamp2,
	sensor_WhiteB_ClearDay, sensor_WhiteB_Cloudy,NULL,
};

static	struct rk_sensor_reg sensor_Brightness0[]=
{
	// Brightness -2
	
	{0x24, 0x30},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness1[]=
{
	// Brightness -1
	{0x24, 0x40},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness2[]=
{
	//	Brightness 0
	{0x24, 0x4f},//0x4a
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness3[]=
{
	// Brightness +1
	{0x24, 0x60},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness4[]=
{
	//	Brightness +2
	{0x24, 0x70},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness5[]=
{
	//	Brightness +3
	{0x24, 0x7f},
	SensorEnd
};
static struct rk_sensor_reg *sensor_BrightnessSeqe[] = {sensor_Brightness0, sensor_Brightness1, sensor_Brightness2, sensor_Brightness3,
	sensor_Brightness4, sensor_Brightness5,NULL,
};

static	struct rk_sensor_reg sensor_Effect_Normal[] =
{
	{0x98, 0x8a},
	{0x70, 0x00},
	{0x69, 0x00},
	{0x67, 0x80},	
	{0x68, 0x80},
	{0x56, 0x28},
	{0xb0, 0x8d},
	{0xb1, 0x95},//0x95 
	{0x56, 0xe8},
	{0xb4, 0x40},
	
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_WandB[] =
{
	{0x98, 0x8a},
	{0x70, 0x31},
	{0x69, 0x00},
	{0x67, 0x80},	
	{0x68, 0x80},
	{0x56, 0x28},
	{0xb0, 0x8d},
	{0xb1, 0x95},
	{0x56, 0xe8},
	{0xb4, 0x40},			
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Sepia[] =
{
	{0x98, 0x8a},
	{0x70, 0x00},
	{0x69, 0x20},
	{0x67, 0x58},	
	{0x68, 0xa0},
	{0x56, 0x28},
	{0xb0, 0x8d},
	{0xb1, 0x95},
	{0x56, 0xe8},
	{0xb4, 0x40},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Negative[] =
{
	//Negative
	{0x98, 0x8a},
	{0x70, 0x00},
	{0x69, 0x21},
	{0x67, 0x80},	
	{0x68, 0x80},
	{0x56, 0x28},
	{0xb0, 0x8d},
	{0xb1, 0x95},
	{0x56, 0xe8},
	{0xb4, 0x40},
	SensorEnd
};
static	struct rk_sensor_reg sensor_Effect_Bluish[] =
{
	// Bluish
	{0x98, 0x8a},
	{0x70, 0x00},
	{0x69, 0x20},
	{0x67, 0xc0},	
	{0x68, 0x58},
	{0x56, 0x28},
	{0xb0, 0x8d},
	{0xb1, 0x95},
	{0x56, 0xe8},
	{0xb4, 0x40},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Green[] =
{
	//	Greenish
	{0x98, 0x8a},
	{0x70, 0x00},
	{0x69, 0x20},
	{0x67, 0x68},	
	{0x68, 0x60},
	{0x56, 0x28},
	{0xb0, 0x8d},
	{0xb1, 0x95},
	{0x56, 0xe8},
	{0xb4, 0x40},
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
	//Contrast -3

	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast1[]=
{
	//Contrast -2

	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast2[]=
{
	// Contrast -1

	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast3[]=
{
	//Contrast 0

	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast4[]=
{
	//Contrast +1

	SensorEnd
};


static	struct rk_sensor_reg sensor_Contrast5[]=
{
	//Contrast +2

	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast6[]=
{
	//Contrast +3

	SensorEnd
};
static struct rk_sensor_reg *sensor_ContrastSeqe[] = {sensor_Contrast0, sensor_Contrast1, sensor_Contrast2, sensor_Contrast3,
	sensor_Contrast4, sensor_Contrast5, sensor_Contrast6, NULL,
};
static	struct rk_sensor_reg sensor_SceneAuto[] =
{
	{0x89,0x7d},
	SensorEnd
};

static	struct rk_sensor_reg sensor_SceneNight[] =
{
	{0x89,0xa3},
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
	{V4L2_MBUS_FMT_UYVY8_2X8, V4L2_COLORSPACE_JPEG} 
};
static struct soc_camera_ops sensor_ops;


/*
**********************************************************
* Following is local code:
* 
* Please codeing your program here 
**********************************************************
*/
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
	
	return 0;
}
/*
* the function is called in close sensor
*/
static int sensor_deactivate_cb(struct i2c_client *client)
{
	
	return 0;
}
/*
* the function is called before sensor register setting in VIDIOC_S_FMT  
*/
static int sensor_s_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{
	char value;
	unsigned   int pid=0,shutter,temp_reg;
	if(mf->width == 1600 && mf->height == 1200){
		
	}

	return 0;
}
/*
* the function is called after sensor register setting finished in VIDIOC_S_FMT  
*/
static int sensor_s_fmt_cb_bh (struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
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
static int sensor_try_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf)
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
	
	SENSOR_DG("mirror: %d",mirror);
	if (mirror) {

	} else {
		//do nothing
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

	SENSOR_DG("flip: %d",flip);

	if (flip) {
	

             } 
	else {
		//do nothing
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
/*
* the functions are focus callbacks
*/
static int sensor_focus_init_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_single_usr_cb(struct i2c_client *client){
	return 0;
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
    return 0;
}

/*
face defect call back
*/
static int	sensor_face_detect_usr_cb(struct i2c_client *client,int on){
	return 0;
}

/*
*	The function can been run in sensor_init_parametres which run in sensor_probe, so user can do some
* initialization in the function. 
*/
static void sensor_init_parameters_user(struct specific_sensor* spsensor,struct soc_camera_device *icd)
{
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





