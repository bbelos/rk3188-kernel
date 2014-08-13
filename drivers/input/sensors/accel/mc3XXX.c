/*
 *  MCube mc3XXX acceleration sensor driver
 *
 *  Copyright (C) 2013 MCube Inc.,
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * *****************************************************************************/

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>

#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/mc3XXX.h>
#include <mach/gpio.h>
#include <mach/board.h> 

#include <linux/sensor-dev.h> 


/***********************************************
 *** REGISTER MAP
 ***********************************************/
#define MC3XXX_REG_XOUT                    			0x00
#define MC3XXX_REG_YOUT                    			0x01
#define MC3XXX_REG_ZOUT                    			0x02
#define MC3XXX_REG_TILT_STATUS             		0x03
#define MC3XXX_REG_SAMPLE_RATE_STATUS      	0x04
#define MC3XXX_REG_SLEEP_COUNT             		0x05
#define MC3XXX_REG_INTERRUPT_ENABLE        		0x06
#define MC3XXX_REG_MODE_FEATURE            		0x07
#define MC3XXX_REG_SAMPLE_RATE             		0x08
#define MC3XXX_REG_TAP_DETECTION_ENABLE    	0x09
#define MC3XXX_REG_TAP_DWELL_REJECT        	0x0A
#define MC3XXX_REG_DROP_CONTROL            		0x0B
#define MC3XXX_REG_SHAKE_DEBOUNCE          		0x0C
#define MC3XXX_REG_XOUT_EX_L               		0x0D
#define MC3XXX_REG_XOUT_EX_H              		 	0x0E
#define MC3XXX_REG_YOUT_EX_L               		0x0F
#define MC3XXX_REG_YOUT_EX_H               		0x10
#define MC3XXX_REG_ZOUT_EX_L               			0x11
#define MC3XXX_REG_ZOUT_EX_H               		0x12
#define MC3XXX_REG_RANGE_CONTROL           		0x20
#define MC3XXX_REG_SHAKE_THRESHOLD         	0x2B
#define MC3XXX_REG_UD_Z_TH                 			0x2C
#define MC3XXX_REG_UD_X_TH                 			0x2D
#define MC3XXX_REG_RL_Z_TH                 			0x2E
#define MC3XXX_REG_RL_Y_TH                 			0x2F
#define MC3XXX_REG_FB_Z_TH                 			0x30
#define MC3XXX_REG_DROP_THRESHOLD          		0x31
#define MC3XXX_REG_TAP_THRESHOLD           		0x32
#define MC3XXX_REG_PRODUCT_CODE            		0x3B


#define MC3XXX_MODE_SLEEP					0x03
#define MC3XXX_MODE_WAKEUP					0x01
#define MC3XXX_MODE_BITS					0x03


#define MC3XXX_PRECISION       8
#define MC3XXX_RANGE						2000000
#define MC3XXX_BOUNDARY        (0x1 << (MC3XXX_PRECISION - 1))
#define MC3XXX_GRAVITY_STEP    MC3XXX_RANGE/MC3XXX_BOUNDARY

/*rate*/
#define MC3XXX_RATE_1          0x07
#define MC3XXX_RATE_2          0x06
#define MC3XXX_RATE_4          0x05
#define MC3XXX_RATE_8          0x04
#define MC3XXX_RATE_16        0x03
#define MC3XXX_RATE_32        0x02
#define MC3XXX_RATE_64        0x01
#define MC3XXX_RATE_128      0x00

#define MC3XXX_AXIS_X		   0
#define MC3XXX_AXIS_Y		   1
#define MC3XXX_AXIS_Z		   2
#define MC3XXX_AXES_NUM 	   3
#define MC3XXX_DATA_LEN 	   6
#define MC3XXX_DEV_NAME 	   "MC3XXX"


/***********************************************
 *** PRODUCT ID
 ***********************************************/
#define MC3XXX_PCODE_3210     	0x90
#define MC3XXX_PCODE_3230     	0x19
#define MC3XXX_PCODE_3250     	0x88
#define MC3XXX_PCODE_3410     	0xA8
#define MC3XXX_PCODE_3410N   0xB8
#define MC3XXX_PCODE_3430     	0x29
#define MC3XXX_PCODE_3430N   0x39
#define MC3XXX_PCODE_3510B   0x40
#define MC3XXX_PCODE_3530B   0x30
#define MC3XXX_PCODE_3510C   0x10
#define MC3XXX_PCODE_3530C   0x60

//=============================================================================
#define IS_MC3230 		1	//8bit
#define IS_MC3210 		2	//14bit
#define IS_NEW_MC34x0 	3
#define IS_MC3250 		4
#define IS_MC35XX 		5



static unsigned char mc3XXX_type;
static unsigned char  Sensor_Accuracy= 0;

#define MCUBE_1_5G_8BIT    	0x01	
#define MCUBE_8G_14BIT     	0x02	

#define ZERO_FIR            5
//#define CONFIG_HAS_LOW_PASS_FILTER
/*----------------------------------------------------------------------------*/

#define CALIB_PATH			"/data/data/com.mcube.acc/files/mcube-calib.txt"
#define DATA_PATH			 "/sdcard/mcube-register-map.txt"
//MCUBE_BACKUP_FILE
#define BACKUP_CALIB_PATH		"/data/misc/mcube-calib.txt"
static char backup_buf[64];
//MCUBE_BACKUP_FILE


static GSENSOR_VECTOR3D gsensor_gain;

static int fd_file = -1;
static int load_cali_flg = 0;
//MCUBE_BACKUP_FILE
static bool READ_FROM_BACKUP = false;
//MCUBE_BACKUP_FILE
static mm_segment_t oldfs;
//add by Liang for storage offset data
static unsigned char offset_buf[9]; 
static signed int offset_data[3];
s16 G_RAW_DATA[3];
static signed int gain_data[3];
static signed int enable_RBM_calibration = 0;


#if 0
#define mcprintkreg(x...) printk(x)
#define mcprintkfunc(x...) printk(x)
#else
#define mcprintkreg(x...)
#define mcprintkfunc(x...)
#endif


#if 0
#define GSE_ERR(x...) 	printk(x)
#define GSE_LOG(x...) 	printk(x)
#endif

#if 0
#define GSE_TAG 				 "[Gsensor] "
#define GSE_FUN(f)				 printk(KERN_INFO GSE_TAG"%s\n", __FUNCTION__)
#define GSE_ERR(fmt, args...)	 printk(KERN_INFO GSE_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define GSE_LOG(fmt, args...)	 printk(KERN_INFO GSE_TAG fmt, ##args)
#else
#define GSE_TAG
#define GSE_FUN(f)			 do {} while (0)
#define GSE_ERR(fmt, args...)	 do {} while (0)
#define GSE_LOG(fmt, args...)	 do {} while (0)
#endif


#define MC3XXX_SPEED		200 * 1000
#define MC3XXX_DEVID		0x01
/* Addresses to scan -- protected by sense_data_mutex */
//static char sense_data[RBUFF_SIZE + 1];
static struct i2c_client *this_client;

static DECLARE_WAIT_QUEUE_HEAD(data_ready_wq);

//static int revision = -1;
static const char* vendor = "Mcube";

/*status*/
#define MC3XXX_OPEN           1
#define MC3XXX_CLOSE          0

typedef char status_t;
struct hwmsen_convert {
	s8 sign[3];
	u8 map[3];
};

struct mc3XXX_data {
	struct sensor_private_data *g_sensor_private_data;

	status_t status;
	char  curr_rate;
	s16 				offset[MC3XXX_AXES_NUM+1];	/*+1: for 4-byte alignment*/
	s16 				data[MC3XXX_AXES_NUM+1]; 

	struct hwmsen_convert   cvt;
};
static int MC3XXX_WriteCalibration(struct i2c_client *client, int dat[MC3XXX_AXES_NUM]);

static int mc3XXX_write_reg(struct i2c_client *client,int addr,int value);
static char mc3XXX_read_reg(struct i2c_client *client,int addr);
static int mc3XXX_rx_data(struct i2c_client *client, char *rxData, int length);
static int mc3XXX_tx_data(struct i2c_client *client, char *txData, int length);
static int mc3XXX_read_block(struct i2c_client *client, char reg, char *rxData, int length);
static int mc3XXX_write_block(struct i2c_client *client, char reg, char *txData, int length);
static int mc3XXX_active(struct i2c_client *client,int enable);
static void MC3XXX_rbm(struct i2c_client *client, int enable);

struct file *openFile(char *path,int flag,int mode) 
{ 
	struct file *fp; 
	 
	fp=filp_open(path, flag, mode); 
	if (IS_ERR(fp) || !fp->f_op) 
	{
		GSE_LOG("Calibration File filp_open return NULL\n");
		return NULL; 
	}
	else 
	{
		return fp; 
	}
} 
 
int readFile(struct file *fp,char *buf,int readlen) 
{ 
	if (fp->f_op && fp->f_op->read) 
		return fp->f_op->read(fp,buf,readlen, &fp->f_pos); 
	else 
		return -1; 
} 

int writeFile(struct file *fp,char *buf,int writelen) 
{ 
	if (fp->f_op && fp->f_op->write) 
		return fp->f_op->write(fp,buf,writelen, &fp->f_pos); 
	else 
		return -1; 
}
 
int closeFile(struct file *fp) 
{ 
	filp_close(fp,NULL); 
	return 0; 
} 

void initKernelEnv(void) 
{ 
	oldfs = get_fs(); 
	set_fs(KERNEL_DS);
	printk(KERN_INFO "initKernelEnv\n");
} 
struct mc3XXX_data g_mc3XXX_data = {0};
static struct mc3XXX_data *get_3XXX_ctl_data(void)
{
	return &g_mc3XXX_data;
}

static int mcube_read_cali_file(struct i2c_client *client)
{
	int cali_data[3];
	int err =0;

	printk("%s %d\n",__func__,__LINE__);
	//MCUBE_BACKUP_FILE
	READ_FROM_BACKUP = false;
	//MCUBE_BACKUP_FILE
	initKernelEnv();
	
	fd_file = openFile(CALIB_PATH,O_RDONLY,0); 
	//MCUBE_BACKUP_FILE
	if (fd_file == NULL) 
	{
		fd_file = openFile(BACKUP_CALIB_PATH, O_RDONLY, 0); 
		if(fd_file != NULL)
		{
				READ_FROM_BACKUP = true;
		}
	}
	//MCUBE_BACKUP_FILE
	if (fd_file == NULL) 
	{
		printk("fail to open\n");
		cali_data[0] = 0;
		cali_data[1] = 0;
		cali_data[2] = 0;
		return 1;
	}
	else
	{
		printk("%s %d\n",__func__,__LINE__);
		memset(backup_buf,0,64); 
		if ((err = readFile(fd_file,backup_buf,128))>0) 
			GSE_LOG("buf:%s\n",backup_buf); 
		else 
			GSE_LOG("read file error %d\n",err); 
		printk("%s %d\n",__func__,__LINE__);

		set_fs(oldfs); 
		closeFile(fd_file); 

		sscanf(backup_buf, "%d %d %d",&cali_data[MC3XXX_AXIS_X], &cali_data[MC3XXX_AXIS_Y], &cali_data[MC3XXX_AXIS_Z]);
		GSE_LOG("cali_data: %d %d %d\n", cali_data[MC3XXX_AXIS_X], cali_data[MC3XXX_AXIS_Y], cali_data[MC3XXX_AXIS_Z]); 	
				
		//GSE_LOG("cali_data1: %d %d %d\n", cali_data1[MC3XXX_AXIS_X], cali_data1[MC3XXX_AXIS_Y], cali_data1[MC3XXX_AXIS_Z]); 	
		printk("%s %d\n",__func__,__LINE__);	  
		MC3XXX_WriteCalibration(client, cali_data);
	}
	return 0;
}


static void MC3XXX_rbm(struct i2c_client *client, int enable)
{
	int err; 

	if(enable == 1 )
	{
		err = mc3XXX_write_reg(client,0x07,0x43);
		err = mc3XXX_write_reg(client,0x14,0x02);
		err = mc3XXX_write_reg(client,0x07,0x41);

		enable_RBM_calibration =1;
		
		GSE_LOG("set rbm!!\n");

		msleep(220);
	}
	else if(enable == 0 )  
	{
		err = mc3XXX_write_reg(client,0x07,0x43);
		err = mc3XXX_write_reg(client,0x14,0x00);
		err = mc3XXX_write_reg(client,0x07,0x41);
		enable_RBM_calibration =0;

		GSE_LOG("clear rbm!!\n");

		msleep(220);
	}
}

/*----------------------------------------------------------------------------*/

static int MC3XXX_ReadData_RBM(struct i2c_client *client, int data[MC3XXX_AXES_NUM])
{   
	//u8 uData;
	u8 addr = 0x0d;
	u8 rbm_buf[MC3XXX_DATA_LEN] = {0};
	int err = 0;
	if(NULL == client)
	{
		err = -EINVAL;
		return err;
	}

	err = mc3XXX_read_block(client, addr, rbm_buf, 0x06);

	data[MC3XXX_AXIS_X] = (s16)((rbm_buf[0]) | (rbm_buf[1] << 8));
	data[MC3XXX_AXIS_Y] = (s16)((rbm_buf[2]) | (rbm_buf[3] << 8));
	data[MC3XXX_AXIS_Z] = (s16)((rbm_buf[4]) | (rbm_buf[5] << 8));

	GSE_LOG("rbm_buf<<<<<[%02x %02x %02x %02x %02x %02x]\n",rbm_buf[0], rbm_buf[2], rbm_buf[2], rbm_buf[3], rbm_buf[4], rbm_buf[5]);
	GSE_LOG("RBM<<<<<[%04x %04x %04x]\n", data[MC3XXX_AXIS_X], data[MC3XXX_AXIS_Y], data[MC3XXX_AXIS_Z]);
	GSE_LOG("RBM<<<<<[%04d %04d %04d]\n", data[MC3XXX_AXIS_X], data[MC3XXX_AXIS_Y], data[MC3XXX_AXIS_Z]);		
	return err;
}

/* AKM HW info */
static ssize_t gsensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	// sprintf(buf, "%#x\n", revision);
	sprintf(buf, "%s.\n", vendor);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(vendor, 0444, gsensor_vendor_show, NULL);


static int mc3XXX_read_block(struct i2c_client *client, char reg, char *rxData, int length)
{
	int ret = 0;
	ret = i2c_master_reg8_recv(client, reg, rxData, length, MC3XXX_SPEED);
	return (ret > 0)? 0 : ret;
}

static int mc3XXX_write_block(struct i2c_client *client, char reg, char *txData, int length)
{
	int ret = 0;
	ret = i2c_master_reg8_send(client, reg, &txData[0], length, MC3XXX_SPEED);
	return (ret > 0)? 0 : ret;
}

static int mc3XXX_rx_data(struct i2c_client *client, char *rxData, int length)
{
	int ret = 0;
	char reg = rxData[0];
	ret = i2c_master_reg8_recv(client, reg, rxData, length, MC3XXX_SPEED);
	return (ret > 0)? 0 : ret;
}

static int mc3XXX_tx_data(struct i2c_client *client, char *txData, int length)
{
	int ret = 0;
	char reg = txData[0];
	ret = i2c_master_reg8_send(client, reg, &txData[1], length-1, MC3XXX_SPEED);
	return (ret > 0)? 0 : ret;
}

static char mc3XXX_read_reg(struct i2c_client *client,int addr)
{
	char tmp;
	int ret = 0;

	tmp = addr;
	ret = mc3XXX_rx_data(client, &tmp, 1);
	return tmp;
}

static int mc3XXX_write_reg(struct i2c_client *client,int addr,int value)
{
	char buffer[3];
	int ret = 0;

	buffer[0] = addr;
	buffer[1] = value;
	ret = mc3XXX_tx_data(client, &buffer[0], 2);
	return ret;
}


static char mc3XXX_get_devid(struct i2c_client *client)
{
	mcprintkreg("mc3XXX devid:%x\n",mc3XXX_read_reg(client,MC3XXX_REG_PRODUCT_CODE));
	return mc3XXX_read_reg(client,MC3XXX_REG_PRODUCT_CODE);
}

static int mc3XXX_active(struct i2c_client *client,int enable)
{
	int tmp;
	int ret = 0;
	if(enable)
		tmp = 0x01;
	else
		tmp = 0x03;
	mcprintkreg("mc3XXX_active %s (0x%x)\n",enable?"active":"standby",tmp);	
	ret = mc3XXX_write_reg(client,MC3XXX_REG_MODE_FEATURE,tmp);
	return ret;
}

//=============================================================================
void MC3XXX_Reset(struct i2c_client *client) 
{
	u8 buf[3] = { 0 };
	u8 data = 0;
	s16 tmp, x_gain, y_gain, z_gain;
	s32 x_off, y_off, z_off;
	int err = 0;

	u8  bMsbFilter       = 0x3F;
	s16 wSignBitMask     = 0x2000;
	s16 wSignPaddingBits = 0xC000;
	s32 dwRangePosLimit  = 0x1FFF;
	s32 dwRangeNegLimit  = -0x2000;

	
	
	
	buf[0] = 0x43;
  	sensor_write_reg(client, 0x07, buf[0]);

	buf[0] = sensor_read_reg(client,0x04);

	if (0x00 == (buf[0] & 0x40))
	{
		buf[0] = 0x6d;
		sensor_write_reg(client, 0x1b, buf[0]);
		
		buf[0] = 0x43;
		sensor_write_reg(client, 0x1b, buf[0]);
	}
	
	msleep(5);
	
	buf[0] = 0x43;
  	sensor_write_reg(client, 0x07, buf[0]);

	buf[0] = 0x80;
  	sensor_write_reg(client, 0x1c, buf[0]);

	buf[0] = 0x80;
  	sensor_write_reg(client, 0x17, buf[0]);

	msleep(5);

	buf[0] = 0x00;
  	sensor_write_reg(client, 0x1c, buf[0]);

	buf[0] = 0x00;
  	sensor_write_reg(client, 0x17, buf[0]);

	msleep(5);

	memset(offset_buf, 0, sizeof(offset_buf));

	offset_buf[0] = 0x21;
	err = sensor_rx_data(client, offset_buf, 9);
	if(err)
	{
		GSE_ERR("read sensor offset error: %d\n", err);
		return err;
	}
	
	
	data = sensor_read_reg(client,MC3XXX_REG_PRODUCT_CODE);
	printk("MC3XXX_chip_init PRODUCT_CODE=%d\n", data);

	if((data == MC3XXX_PCODE_3230)||(data == MC3XXX_PCODE_3430)
		||(data == MC3XXX_PCODE_3430N)||(data == MC3XXX_PCODE_3530B)
		||(data == MC3XXX_PCODE_3530C))
		Sensor_Accuracy = MCUBE_1_5G_8BIT;	//8bit
	else if((data == MC3XXX_PCODE_3210)||(data == MC3XXX_PCODE_3410)
		||(data == MC3XXX_PCODE_3250)||(data == MC3XXX_PCODE_3410N)
		||(data == MC3XXX_PCODE_3510B)||(data == MC3XXX_PCODE_3510C))
		Sensor_Accuracy = MCUBE_8G_14BIT;		//14bit
	
	if( MC3XXX_PCODE_3230 == data)
		mc3XXX_type = IS_MC3230;
	else if ( MC3XXX_PCODE_3210 ==data)
		mc3XXX_type = IS_MC3210;
	else if ( MC3XXX_PCODE_3250 ==data)
		mc3XXX_type = IS_MC3250;
	else if ((data == MC3XXX_PCODE_3430N)||(data == MC3XXX_PCODE_3410N))	
		mc3XXX_type = IS_NEW_MC34x0;
	else if((MC3XXX_PCODE_3510B == data) || (MC3XXX_PCODE_3510C == data)
		||(data == MC3XXX_PCODE_3530B)||(data == MC3XXX_PCODE_3530C))
		mc3XXX_type = IS_MC35XX;
	
	
	if (IS_MC35XX== mc3XXX_type)
	{
	    bMsbFilter       = 0x7F;
	    wSignBitMask     = 0x4000;
	    wSignPaddingBits = 0x8000;
	    dwRangePosLimit  = 0x3FFF;
	    dwRangeNegLimit  = -0x4000;
	}
	
	// get x,y,z offset
	tmp = ((offset_buf[1] & bMsbFilter) << 8) + offset_buf[0];
		if (tmp & wSignBitMask)
			tmp |= wSignPaddingBits;
		x_off = tmp;
					
	tmp = ((offset_buf[3] & bMsbFilter) << 8) + offset_buf[2];
		if (tmp & wSignBitMask)
			tmp |= wSignPaddingBits;
		y_off = tmp;
					
	tmp = ((offset_buf[5] & bMsbFilter) << 8) + offset_buf[4];
		if (tmp & wSignBitMask)
			tmp |= wSignPaddingBits;
		z_off = tmp;
					
	// get x,y,z gain
	x_gain = ((offset_buf[1] >> 7) << 8) + offset_buf[6];
	y_gain = ((offset_buf[3] >> 7) << 8) + offset_buf[7];
	z_gain = ((offset_buf[5] >> 7) << 8) + offset_buf[8];
								


	//add for over range 
	if( x_off > dwRangePosLimit) 
	{
		x_off = dwRangePosLimit;
	}
	else if( x_off < dwRangeNegLimit)
	{
		x_off = dwRangeNegLimit;
	}

	if( y_off > dwRangePosLimit) 
	{
		y_off = dwRangePosLimit;
	}
	else if( y_off < dwRangeNegLimit)
	{
		y_off = dwRangeNegLimit;
	}

	if( z_off > dwRangePosLimit) 
	{
		z_off = dwRangePosLimit;
	}
	else if( z_off < dwRangeNegLimit)
	{
		z_off = dwRangeNegLimit;
	}
	
	//storege the cerrunt offset data with DOT format
	offset_data[0] = x_off;
	offset_data[1] = y_off;
	offset_data[2] = z_off;

	//storege the cerrunt Gain data with GOT format
	gain_data[0] = 256*8*128/3/(40+x_gain);
	gain_data[1] = 256*8*128/3/(40+y_gain);
	gain_data[2] = 256*8*128/3/(40+z_gain);
	//printk("%d %d ======================\n\n ",gain_data[0],x_gain);
	
	

	buf[0] = sensor_read_reg(client,0x04);
	if (0x00 == (buf[0] & 0x40))
	{
		buf[0] = 0x6d;
		sensor_write_reg(client, 0x1b, buf[0]);
		
		buf[0] = 0x43;
		sensor_write_reg(client, 0x1b, buf[0]);
	}

	buf[0] = 0x41;
	sensor_write_reg(client, 0x07, buf[0]);
	
}


static int MC3XXX_chip_init(struct i2c_client *client)
{
	unsigned char data = 0;	

	struct mc3XXX_data* mc3XXX = get_3XXX_ctl_data();
	
	load_cali_flg = 30;
	
	mcprintkfunc("%s enter\n",__FUNCTION__);

	mc3XXX->g_sensor_private_data = (struct sensor_private_data *) i2c_get_clientdata(client);
	mc3XXX->curr_rate = MC3XXX_RATE_16;
	mc3XXX->status = MC3XXX_CLOSE;
		
	mc3XXX_active(client,0);  // 1:awake  0:standby   

	data = sensor_read_reg(client,MC3XXX_REG_PRODUCT_CODE);
	printk("MC3XXX_chip_init PRODUCT_CODE=%d\n", data);

	if((data == MC3XXX_PCODE_3230)||(data == MC3XXX_PCODE_3430)
		||(data == MC3XXX_PCODE_3430N)||(data == MC3XXX_PCODE_3530B)
		||(data == MC3XXX_PCODE_3530C))
		Sensor_Accuracy = MCUBE_1_5G_8BIT;	//8bit
	else if((data == MC3XXX_PCODE_3210)||(data == MC3XXX_PCODE_3410)
		||(data == MC3XXX_PCODE_3250)||(data == MC3XXX_PCODE_3410N)
		||(data == MC3XXX_PCODE_3510B)||(data == MC3XXX_PCODE_3510C))
		Sensor_Accuracy = MCUBE_8G_14BIT;		//14bit
	
	if( MC3XXX_PCODE_3230 == data)
		mc3XXX_type = IS_MC3230;
	else if ( MC3XXX_PCODE_3210 ==data)
		mc3XXX_type = IS_MC3210;
	else if ( MC3XXX_PCODE_3250 ==data)
		mc3XXX_type = IS_MC3250;
	else if ((data == MC3XXX_PCODE_3430N)||(data == MC3XXX_PCODE_3410N))	
		mc3XXX_type = IS_NEW_MC34x0;
	else if((MC3XXX_PCODE_3510B == data) || (MC3XXX_PCODE_3510C == data)
		||(data == MC3XXX_PCODE_3530B)||(data == MC3XXX_PCODE_3530C))
		mc3XXX_type = IS_MC35XX;
	

	if(MCUBE_8G_14BIT == Sensor_Accuracy)
	{
		data = 0x43;
	  	sensor_write_reg(client, MC3XXX_REG_MODE_FEATURE, data);
		data = 0x00;
	  	sensor_write_reg(client, MC3XXX_REG_SLEEP_COUNT, data);
		data = 0x00;
	  	sensor_write_reg(client, MC3XXX_REG_SAMPLE_RATE, data);
		data = 0x00;
	  	sensor_write_reg(client, MC3XXX_REG_TAP_DETECTION_ENABLE, data);
		data = 0x3F;

		if (IS_MC35XX == mc3XXX_type)
		{	
			data = 0xA5;
		}	
		
	  	sensor_write_reg(client, MC3XXX_REG_RANGE_CONTROL, data);
		data = 0x00;
		sensor_write_reg(client, MC3XXX_REG_INTERRUPT_ENABLE, data);	

		gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = 1024;	//14bit
		
	}
	else if(MCUBE_1_5G_8BIT == Sensor_Accuracy)
	{		
		data = 0x43;
		sensor_write_reg(client, MC3XXX_REG_MODE_FEATURE, data);
		data = 0x00;
		sensor_write_reg(client, MC3XXX_REG_SLEEP_COUNT, data);
		data = 0x00;
		sensor_write_reg(client, MC3XXX_REG_SAMPLE_RATE, data);
		data = 0x02;
		sensor_write_reg(client, MC3XXX_REG_RANGE_CONTROL,data);
		data = 0x00;
		sensor_write_reg(client, MC3XXX_REG_TAP_DETECTION_ENABLE, data);
		data = 0x00;
		sensor_write_reg(client, MC3XXX_REG_INTERRUPT_ENABLE, data);

            	gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = 86;

		if (IS_MC35XX == mc3XXX_type)
		{
			gsensor_gain.x = gsensor_gain.y = gsensor_gain.z = 64;
		}

	}

	data = 0x41;
	sensor_write_reg(client, MC3XXX_REG_MODE_FEATURE, data);	

	return 0;
		
}

static int mc3XXX_start_dev(struct i2c_client *client, char rate)
{
	int ret = 0;
	struct mc3XXX_data* mc3XXX= get_3XXX_ctl_data();

	mcprintkfunc("-------------------------mc3XXX start dev------------------------\n");	
	/* standby */
	mc3XXX_active(client,0);
	mcprintkreg("mc3XXX MC3XXX_REG_SYSMOD:%x\n",mc3XXX_read_reg(client,MC3XXX_REG_MODE_FEATURE));

	/*data rate*/
	ret = mc3XXX_write_reg(client,MC3XXX_REG_SAMPLE_RATE,rate);
	mc3XXX->curr_rate = rate;
	mcprintkreg("mc3XXX MC3XXX_REG_SAMPLE_RATE:%x  rate=%d\n",mc3XXX_read_reg(client,MC3XXX_REG_SAMPLE_RATE),rate);
	/*wake*/
	mc3XXX_active(client,1);
	mcprintkreg("mc3XXX MC3XXX_REG_SYSMOD:%x\n",mc3XXX_read_reg(client,MC3XXX_REG_MODE_FEATURE));
	
	enable_irq(client->irq);
	return ret;

}

static int mc3XXX_start(struct i2c_client *client, char rate)
{ 
	struct mc3XXX_data* mc3XXX= get_3XXX_ctl_data();
	mcprintkfunc("%s::enter\n",__FUNCTION__); 
	if (mc3XXX->status == MC3XXX_OPEN) {
		return 0;      
	}
	mc3XXX->status = MC3XXX_OPEN;
	rate = 0;
	return mc3XXX_start_dev(client, rate);
}

static int mc3XXX_close_dev(struct i2c_client *client)
{    	
	disable_irq_nosync(client->irq);
	return mc3XXX_active(client,0);
}

static int mc3XXX_close(struct i2c_client *client)
{
	struct mc3XXX_data *mc3XXX = (struct mc3XXX_data *)i2c_get_clientdata(client);
	mcprintkfunc("%s::enter\n",__FUNCTION__); 
	mc3XXX->status = MC3XXX_CLOSE;

	return mc3XXX_close_dev(client);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mc3XXX_suspend(struct i2c_client *client)
{
	struct mc3XXX_data *mc3XXX = (struct mc3XXX_data *)i2c_get_clientdata(client);
	mcprintkreg("Gsensor mc3XXX enter suspend mc3XXX->status %d\n",mc3XXX->status);
	if(mc3XXX->status == MC3XXX_OPEN)
	{
		mc3XXX_close(client);
	}
}

static void mc3XXX_resume(struct i2c_client *client)
{
   	struct mc3XXX_data *mc3XXX = (struct mc3XXX_data *)i2c_get_clientdata(client);
	mcprintkreg("Gsensor mc3XXX resume!! mc3XXX->status %d\n",mc3XXX->status);
	if (mc3XXX->status == MC3XXX_CLOSE)
	{
		mc3XXX_start(client,mc3XXX->curr_rate);
	}
}
#endif

static inline int mc3XXX_convert_to_int(s16 value)
{
	int result;

	//printk("mc3XXX_convert_to_int value=%d ",value );
	if(value>-3 && value<3)value=0;

	if (value < MC3XXX_BOUNDARY) 
	{	
		result = value * MC3XXX_GRAVITY_STEP;
		//printk("value < MC3XXX_BOUNDARY result=%d ",result );
		
	} else 
	{
		result = ~(((~value & 0x7f) + 1)* MC3XXX_GRAVITY_STEP) + 1;
		//printk("value > MC3XXX_BOUNDARY result=%d ",result );
	}
		

    return result;
}



static void mc3XXX_report_value(struct i2c_client *client, struct mc3XXX_axis *axis)
{
	struct sensor_private_data *mc3XXX = (struct sensor_private_data*)i2c_get_clientdata(client);

	int x = 1;
	int y = 1;
	int z = -1;
	int temp = 0;

	/* Report acceleration sensor information */
//	input_report_abs(mc3XXX->input_dev, ABS_X, -(axis->x)/1000);
//	input_report_abs(mc3XXX->input_dev, ABS_Y, (axis->y)/1000);
//	input_report_abs(mc3XXX->input_dev, ABS_Z, -(axis->z)/1000);
	
	input_report_abs(mc3XXX->input_dev, ABS_X, -axis->x);
	input_report_abs(mc3XXX->input_dev, ABS_Y, axis->y);
	input_report_abs(mc3XXX->input_dev, ABS_Z, -axis->z);
	input_sync(mc3XXX->input_dev);
  // mcprintkfunc("Gsensor x==%d  y==%d z==%d\n",axis->x,axis->y,axis->z);
}

static int MC3XXX_ReadRBMData(struct i2c_client *client, char *buf)
{
	struct mc3XXX_data *mc3XXX = (struct mc3XXX_data*)i2c_get_clientdata(client);
	int res = 0;
	int data[3];

	if (!buf || !client)
	{
		return EINVAL;
	}

	if(mc3XXX->status == MC3XXX_CLOSE)
	{
		res = mc3XXX_start(client, 0);
		if(res)
		{
			GSE_ERR("Power on mc3XXX error %d!\n", res);
		}
	}
	
	if(res = MC3XXX_ReadData_RBM(client, data))
	{        
		GSE_ERR("%s I2C error: ret value=%d",__func__, res);
		return EIO;
	}
	else
	{
		sprintf(buf, "%04x %04x %04x", data[MC3XXX_AXIS_X], 
			data[MC3XXX_AXIS_Y], data[MC3XXX_AXIS_Z]);
	
	}
	
	return 0;
}
static int MC3XXX_ReadOffset(struct i2c_client *client, s16 ofs[MC3XXX_AXES_NUM])
{    
	int err;
	u8 off_data[6];
	
	if ( mc3XXX_type == IS_MC3210 )
	{
		off_data[0]=MC3XXX_REG_XOUT_EX_L;
		if(err = sensor_rx_data(client, off_data, MC3XXX_DATA_LEN))
    		{
    			GSE_ERR("error: %d\n", err);
    			return err;
    		}
		ofs[MC3XXX_AXIS_X] = ((s16)(off_data[0]))|((s16)(off_data[1])<<8);
		ofs[MC3XXX_AXIS_Y] = ((s16)(off_data[2]))|((s16)(off_data[3])<<8);
		ofs[MC3XXX_AXIS_Z] = ((s16)(off_data[4]))|((s16)(off_data[5])<<8);
	}
	else if (mc3XXX_type == IS_MC3230) 
	{
		off_data[0]=MC3XXX_REG_XOUT;
		if(err = sensor_rx_data(client, off_data, 3))
    		{
    			GSE_ERR("error: %d\n", err);
    			return err;
    		}
		ofs[MC3XXX_AXIS_X] = (s8)off_data[0];
		ofs[MC3XXX_AXIS_Y] = (s8)off_data[1];
		ofs[MC3XXX_AXIS_Z] = (s8)off_data[2];			
	}

	GSE_LOG("MC3XXX_ReadOffset %d %d %d \n",ofs[MC3XXX_AXIS_X] ,ofs[MC3XXX_AXIS_Y],ofs[MC3XXX_AXIS_Z]);

    return 0;  
}
/*----------------------------------------------------------------------------*/
static int MC3XXX_ResetCalibration(struct i2c_client *client)
{
	struct mc3XXX_data *mc3XXX = get_3XXX_ctl_data();
	s16 tmp,i;

	u8  bMsbFilter       = 0x3F;
	s16 wSignBitMask     = 0x2000;
	s16 wSignPaddingBits = 0xC000;
	
	if (IS_MC35XX== mc3XXX_type)
	{
	    bMsbFilter       = 0x7F;
	    wSignBitMask     = 0x4000;
	    wSignPaddingBits = 0x8000;
	}
	
	sensor_write_reg(client,0x07,0x43);

	for(i=0;i<6;i++)
	{	
		sensor_write_reg(client, 0x21+i, offset_buf[i]);
		msleep(10);
	}	

	sensor_write_reg(client,0x07,0x41);

	msleep(20);

	tmp = ((offset_buf[1] & bMsbFilter) << 8) + offset_buf[0];
	if (tmp & wSignBitMask)
		tmp |= wSignPaddingBits;
	offset_data[0] = tmp;
					
	tmp = ((offset_buf[3] & bMsbFilter) << 8) + offset_buf[2];
	if (tmp & wSignBitMask)
			tmp |= wSignPaddingBits;
	offset_data[1] = tmp;
					
	tmp = ((offset_buf[5] & bMsbFilter) << 8) + offset_buf[4];
	if (tmp & wSignBitMask)
		tmp |= wSignPaddingBits;
	offset_data[2] = tmp;	

	return 0;  

}
/*----------------------------------------------------------------------------*/
static int MC3XXX_ReadCalibration(struct i2c_client *client, int dat[MC3XXX_AXES_NUM])
{

	struct mc3XXX_data *mc3XXX = get_3XXX_ctl_data();
	int err;

	if ((err = MC3XXX_ReadOffset(client, mc3XXX->offset))) {
	    GSE_ERR("read offset fail, %d\n", err);
	    return err;
	}    

	dat[MC3XXX_AXIS_X] = mc3XXX->offset[MC3XXX_AXIS_X];
	dat[MC3XXX_AXIS_Y] = mc3XXX->offset[MC3XXX_AXIS_Y];
	dat[MC3XXX_AXIS_Z] = mc3XXX->offset[MC3XXX_AXIS_Z];  
	//GSE_LOG("MC3XXX_ReadCalibration %d %d %d \n",dat[mc3XXX->cvt.map[MC3XXX_AXIS_X]] ,dat[mc3XXX->cvt.map[MC3XXX_AXIS_Y]],dat[mc3XXX->cvt.map[MC3XXX_AXIS_Z]]);
	                                  
	return 0;
}

/*----------------------------------------------------------------------------*/
static int MC3XXX_WriteCalibration(struct i2c_client *client, int dat[MC3XXX_AXES_NUM])
{
	int err;
	u8 buf[9],i;
	s16 tmp, x_gain, y_gain, z_gain ;
	s32 x_off, y_off, z_off;
	int temp_cali_dat[MC3XXX_AXES_NUM] = { 0 };

	u8  bMsbFilter       = 0x3F;
	s16 wSignBitMask     = 0x2000;
	s16 wSignPaddingBits = 0xC000;
	s32 dwRangePosLimit  = 0x1FFF;
	s32 dwRangeNegLimit  = -0x2000;

	if (IS_MC35XX== mc3XXX_type)
	{
	    bMsbFilter       = 0x7F;
	    wSignBitMask     = 0x4000;
	    wSignPaddingBits = 0x8000;
	    dwRangePosLimit  = 0x3FFF;
	    dwRangeNegLimit  = -0x4000;
	}

	temp_cali_dat[MC3XXX_AXIS_X] = dat[MC3XXX_AXIS_X];
	temp_cali_dat[MC3XXX_AXIS_Y] = dat[MC3XXX_AXIS_Y];
	temp_cali_dat[MC3XXX_AXIS_Z] = dat[MC3XXX_AXIS_Z];

	if ((IS_NEW_MC34x0== mc3XXX_type)||(IS_MC35XX== mc3XXX_type))
	{
	    temp_cali_dat[MC3XXX_AXIS_X] = -temp_cali_dat[MC3XXX_AXIS_X];
	    temp_cali_dat[MC3XXX_AXIS_Y] = -temp_cali_dat[MC3XXX_AXIS_Y];
	}
	else if(IS_MC3250== mc3XXX_type)
	{
	    s16    temp = 0;

	    temp = temp_cali_dat[MC3XXX_AXIS_X];

	    temp_cali_dat[MC3XXX_AXIS_X] = -temp_cali_dat[MC3XXX_AXIS_Y];
	    temp_cali_dat[MC3XXX_AXIS_Y] = temp;
	}

	dat[MC3XXX_AXIS_X] = temp_cali_dat[MC3XXX_AXIS_X];
	dat[MC3XXX_AXIS_Y] = temp_cali_dat[MC3XXX_AXIS_Y];
	dat[MC3XXX_AXIS_Z] = temp_cali_dat[MC3XXX_AXIS_Z];

	//read register 0x21~0x29
	buf[0] = 0x21;
	err = sensor_rx_data(client, &buf[0], 3);
	buf[3] = 0x24;
	err = sensor_rx_data(client, &buf[3], 3);
	buf[6] = 0x27;
	err = sensor_rx_data(client, &buf[6], 3);

	// get x,y,z offset
	tmp = ((buf[1] & bMsbFilter) << 8) + buf[0];
		if (tmp & wSignBitMask)
			tmp |= wSignPaddingBits;
		x_off = tmp;
					
	tmp = ((buf[3] & bMsbFilter) << 8) + buf[2];
		if (tmp & wSignBitMask)
			tmp |= wSignPaddingBits;
		y_off = tmp;
					
	tmp = ((buf[5] & bMsbFilter) << 8) + buf[4];
		if (tmp & wSignBitMask)
			tmp |= wSignPaddingBits;
		z_off = tmp;
					
	// get x,y,z gain
	x_gain = ((buf[1] >> 7) << 8) + buf[6];
	y_gain = ((buf[3] >> 7) << 8) + buf[7];
	z_gain = ((buf[5] >> 7) << 8) + buf[8];
								
	// prepare new offset
	x_off = x_off + 16 * dat[MC3XXX_AXIS_X] * 256 * 128 / 3 / gsensor_gain.x / (40 + x_gain);
	y_off = y_off + 16 * dat[MC3XXX_AXIS_Y] * 256 * 128 / 3 / gsensor_gain.y / (40 + y_gain);
	z_off = z_off + 16 * dat[MC3XXX_AXIS_Z] * 256 * 128 / 3 / gsensor_gain.z / (40 + z_gain);

	//add for over range 
	if( x_off > dwRangePosLimit) 
	{
		x_off = dwRangePosLimit;
	}
	else if( x_off < dwRangeNegLimit)
	{
		x_off = dwRangeNegLimit;
	}

	if( y_off > dwRangePosLimit) 
	{
		y_off = dwRangePosLimit;
	}
	else if( y_off < dwRangeNegLimit)
	{
		y_off = dwRangeNegLimit;
	}

	if( z_off > dwRangePosLimit) 
	{
		z_off = dwRangePosLimit;
	}
	else if( z_off < dwRangeNegLimit)
	{
		z_off = dwRangeNegLimit;
	}
	
	//storege the cerrunt offset data with DOT format
	offset_data[0] = x_off;
	offset_data[1] = y_off;
	offset_data[2] = z_off;

	//storege the cerrunt Gain data with GOT format
	gain_data[0] = 256*8*128/3/(40+x_gain);
	gain_data[1] = 256*8*128/3/(40+y_gain);
	gain_data[2] = 256*8*128/3/(40+z_gain);
	//printk("%d %d ======================\n\n ",gain_data[0],x_gain);

	sensor_write_reg(client,0x07,0x43);
					
	buf[0] = x_off & 0xff;
	buf[1] = ((x_off >> 8) & 0x3f) | (x_gain & 0x0100 ? 0x80 : 0);
	buf[2] = y_off & 0xff;
	buf[3] = ((y_off >> 8) & 0x3f) | (y_gain & 0x0100 ? 0x80 : 0);
	buf[4] = z_off & 0xff;
	buf[5] = ((z_off >> 8) & 0x3f) | (z_gain & 0x0100 ? 0x80 : 0);

	for(i=0;i<6;i++)
	{	
		sensor_write_reg(client, 0x21+i, buf[i]);
		msleep(10);
	}	
	sensor_write_reg(client,0x07,0x41);

    return err;

}


static int MC3XXX_ReadData(struct i2c_client *client, s16 buffer[MC3XXX_AXES_NUM])
{
	s8 buf[6];
	char rbm_buf[6];
	int ret;
	int err = 0;

	if(NULL == client)
	{
		err = -EINVAL;
		return err;
	}
	mcprintkfunc("MC3XXX_ReadData enable_RBM_calibration = %d\n", enable_RBM_calibration);

	if ( enable_RBM_calibration == 0)
	{
		//err = hwmsen_read_block(client, addr, buf, 0x06);
	}
	else if (enable_RBM_calibration == 1)
	{		
		memset(rbm_buf, 0, sizeof(rbm_buf));
        	rbm_buf[0] = MC3XXX_REG_XOUT_EX_L;
        	ret = sensor_rx_data(client, &rbm_buf[0], 2);
        	rbm_buf[2] = MC3XXX_REG_YOUT_EX_L;
        	ret = sensor_rx_data(client, &rbm_buf[2], 2);
        	rbm_buf[4] = MC3XXX_REG_ZOUT_EX_L;
        	ret = sensor_rx_data(client, &rbm_buf[4], 2);
	}

	mcprintkfunc("MC3XXX_ReadData %d %d %d %d %d %d\n", rbm_buf[0], rbm_buf[1], rbm_buf[2], rbm_buf[3], rbm_buf[4], rbm_buf[5]);

	memset(buf, 0, sizeof(buf));
	if ( enable_RBM_calibration == 0)
	{
		if(Sensor_Accuracy == MCUBE_8G_14BIT)
		{
			buf[0] = MC3XXX_REG_XOUT_EX_L;
			ret = sensor_rx_data(client,&buf[0], 6);
			
			buffer[0] = (s16)((buf[0])|(buf[1]<<8));
			buffer[1] = (s16)((buf[2])|(buf[3]<<8));
			buffer[2] = (s16)((buf[4])|(buf[5]<<8));
		}
		else if(Sensor_Accuracy == MCUBE_1_5G_8BIT)
		{
			buf[0] = MC3XXX_REG_XOUT;
			ret = sensor_rx_data(client,&buf[0], 3);
				
			buffer[0]=(s16)buf[0];
			buffer[1]=(s16)buf[1];
			buffer[2]=(s16)buf[2];
		}

		mcprintkfunc("0x%02x 0x%02x 0x%02x \n",buffer[0],buffer[1],buffer[2]);

		#ifdef CONFIG_HAS_LOW_PASS_FILTER
		if(abs (buffer[MC3XXX_AXIS_X])<=ZERO_FIR)
		buffer[MC3XXX_AXIS_X]=0;

		if(abs (buffer[MC3XXX_AXIS_Y])<=ZERO_FIR)
			buffer[MC3XXX_AXIS_Y]=0;
		#endif
	
	}
	else if (enable_RBM_calibration == 1)
	{
		buffer[MC3XXX_AXIS_X] = (s16)((rbm_buf[0]) | (rbm_buf[1] << 8));
		buffer[MC3XXX_AXIS_Y] = (s16)((rbm_buf[2]) | (rbm_buf[3] << 8));
		buffer[MC3XXX_AXIS_Z] = (s16)((rbm_buf[4]) | (rbm_buf[5] << 8));

		mcprintkfunc("%s RBM<<<<<[%08d %08d %08d]\n", __func__,buffer[MC3XXX_AXIS_X], buffer[MC3XXX_AXIS_Y], buffer[MC3XXX_AXIS_Z]);
		if(gain_data[0] == 0)
		{
			buffer[MC3XXX_AXIS_X] = 0;
			buffer[MC3XXX_AXIS_Y] = 0;
			buffer[MC3XXX_AXIS_Z] = 0;
			return 0;
		}
		buffer[MC3XXX_AXIS_X] = (buffer[MC3XXX_AXIS_X] + offset_data[0]/2)*gsensor_gain.x/gain_data[0];
		buffer[MC3XXX_AXIS_Y] = (buffer[MC3XXX_AXIS_Y] + offset_data[1]/2)*gsensor_gain.y/gain_data[1];
		buffer[MC3XXX_AXIS_Z] = (buffer[MC3XXX_AXIS_Z] + offset_data[2]/2)*gsensor_gain.z/gain_data[2];
		
		mcprintkfunc("%s offset_data <<<<<[%d %d %d]\n", __func__,offset_data[0], offset_data[1], offset_data[2]);

		mcprintkfunc("%s gsensor_gain <<<<<[%d %d %d]\n", __func__,gsensor_gain.x, gsensor_gain.y, gsensor_gain.z);
		
		mcprintkfunc("%s gain_data <<<<<[%d %d %d]\n", __func__,gain_data[0], gain_data[1], gain_data[2]);

		mcprintkfunc("%s RBM->RAW <<<<<[%d %d %d]\n", __func__,buffer[MC3XXX_AXIS_X], buffer[MC3XXX_AXIS_Y], buffer[MC3XXX_AXIS_Z]);
	}

	return 0;
}
static int MC3XXX_ReadRawData(struct i2c_client *client, char * buf)
{
	struct mc3XXX_data *obj = get_3XXX_ctl_data();
	int res = 0;
	s16 raw_buf[3];

	if (!buf || !client)
	{
		return EINVAL;
	}

	if(obj->status == MC3XXX_CLOSE)
	{
		res = mc3XXX_start(client, 0);
		if(res)
		{
			GSE_ERR("Power on mc3XXX error %d!\n", res);
		}
	}
	
	if(res = MC3XXX_ReadData(client, &raw_buf[0]))
	{     
		mcprintkreg("%s %d\n",__FUNCTION__, __LINE__);
		GSE_ERR("I2C error: ret value=%d", res);
		return EIO;
	}
	else
	{
		GSE_LOG("UPDATE dat: (%+3d %+3d %+3d)\n", 
		raw_buf[MC3XXX_AXIS_X], raw_buf[MC3XXX_AXIS_Y], raw_buf[MC3XXX_AXIS_Z]);

		 if ((IS_NEW_MC34x0== mc3XXX_type)||(IS_MC35XX== mc3XXX_type))
	        {
	            raw_buf[MC3XXX_AXIS_X] = -raw_buf[MC3XXX_AXIS_X];
	            raw_buf[MC3XXX_AXIS_Y] = -raw_buf[MC3XXX_AXIS_Y];
	        }
	        else if(IS_MC3250== mc3XXX_type)
	        {
	            s16    temp = 0;

	            temp = raw_buf[MC3XXX_AXIS_X];

	            raw_buf[MC3XXX_AXIS_X] = raw_buf[MC3XXX_AXIS_Y];
	            raw_buf[MC3XXX_AXIS_Y] = -temp;
	        }
			
		G_RAW_DATA[MC3XXX_AXIS_X] = raw_buf[0];
		G_RAW_DATA[MC3XXX_AXIS_Y] = raw_buf[1];
		G_RAW_DATA[MC3XXX_AXIS_Z] = raw_buf[2];
		G_RAW_DATA[MC3XXX_AXIS_Z]= G_RAW_DATA[MC3XXX_AXIS_Z]+gsensor_gain.z;
		
		//printk("%s %d\n",__FUNCTION__, __LINE__);
		sprintf(buf, "%04x %04x %04x", G_RAW_DATA[MC3XXX_AXIS_X], 
			G_RAW_DATA[MC3XXX_AXIS_Y], G_RAW_DATA[MC3XXX_AXIS_Z]);
		GSE_LOG("G_RAW_DATA: (%+3d %+3d %+3d)\n", 
		G_RAW_DATA[MC3XXX_AXIS_X], G_RAW_DATA[MC3XXX_AXIS_Y], G_RAW_DATA[MC3XXX_AXIS_Z]);
	}
	return 0;
}

//MCUBE_BACKUP_FILE
static void mcube_copy_file(const char *dstFilePath)
{
	int err =0;
	initKernelEnv();

	fd_file = openFile(dstFilePath,O_RDWR,0); 
	if (fd_file == NULL) 
	{
		GSE_LOG("open %s fail\n",dstFilePath);  
		return;
	}

	

		if ((err = writeFile(fd_file,backup_buf,64))>0) 
			GSE_LOG("buf:%s\n",backup_buf); 
		else 
			GSE_LOG("write file error %d\n",err);

		set_fs(oldfs); ; 
		closeFile(fd_file);

}
//MCUBE_BACKUP_FILE

 long mc3XXX_ioctl( struct file *file, unsigned int cmd,unsigned long arg, struct i2c_client *client)
{
	void __user *argp = (void __user *)arg;

	char strbuf[256];
	void __user *data;
	SENSOR_DATA sensor_data;
	int err = 0;
	int cali[3];

	struct mc3XXX_axis sense_data = {0};
	int ret = -1;
	char rate;
	struct sensor_private_data* this = (struct sensor_private_data *)i2c_get_clientdata(client);  

	mcprintkreg("mc3XXX_ioctl cmd is %d.", cmd);
		
	switch (cmd) {
	
	case GSENSOR_IOCTL_READ_SENSORDATA:	
	case GSENSOR_IOCTL_READ_RAW_DATA:
		GSE_LOG("fwq GSENSOR_IOCTL_READ_RAW_DATA\n");
		data = (void __user*)arg;
		MC3XXX_ReadRawData(client, &strbuf);
		if(copy_to_user(data, &strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}
		break;
	case GSENSOR_IOCTL_SET_CALI:
		
			GSE_LOG("fwq GSENSOR_IOCTL_SET_CALI!!\n");

			break;

	case GSENSOR_MCUBE_IOCTL_SET_CALI:
			GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_SET_CALI!!\n");
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			if(copy_from_user(&sensor_data, data, sizeof(sensor_data)))
			{
				err = -EFAULT;
				break;	  
			}
			else
			{	
				cali[MC3XXX_AXIS_X] = sensor_data.x;
				cali[MC3XXX_AXIS_Y] = sensor_data.y;
				cali[MC3XXX_AXIS_Z] = sensor_data.z;	

			  	GSE_LOG("GSENSOR_MCUBE_IOCTL_SET_CALI %d  %d  %d  %d  %d  %d!!\n", cali[MC3XXX_AXIS_X], cali[MC3XXX_AXIS_Y],cali[MC3XXX_AXIS_Z] ,sensor_data.x, sensor_data.y ,sensor_data.z);
				
				err = MC3XXX_WriteCalibration(client, cali);			 
			}
			break;
	case GSENSOR_IOCTL_CLR_CALI:
			GSE_LOG("fwq GSENSOR_IOCTL_CLR_CALI!!\n");
			err = MC3XXX_ResetCalibration(client);
			break;

	case GSENSOR_IOCTL_GET_CALI:
			GSE_LOG("fwq mc3XXX GSENSOR_IOCTL_GET_CALI\n");
			data = (void __user*)arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			if((err = MC3XXX_ReadCalibration(client, cali)))
			{
				GSE_LOG("fwq mc3XXX MC3XXX_ReadCalibration error!!!!\n");
				break;
			}
			
			sensor_data.x = cali[MC3XXX_AXIS_X];
			sensor_data.y = cali[MC3XXX_AXIS_Y];
			sensor_data.z = cali[MC3XXX_AXIS_Z];
			if(copy_to_user(data, &sensor_data, sizeof(sensor_data)))
			{
				err = -EFAULT;
				break;
			}		
			break;	

	case GSENSOR_IOCTL_SET_CALI_MODE:
			GSE_LOG("fwq mc3XXX GSENSOR_IOCTL_SET_CALI_MODE\n");
			break;

	case GSENSOR_MCUBE_IOCTL_READ_RBM_DATA:
			GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_READ_RBM_DATA\n");
			data = (void __user *) arg;
			if(data == NULL)
			{
				err = -EINVAL;
				break;	  
			}
			MC3XXX_ReadRBMData(client, (char *)&strbuf);
			if(copy_to_user(data, &strbuf, strlen(strbuf)+1))
			{
				err = -EFAULT;
				break;	  
			}
			break;

	case GSENSOR_MCUBE_IOCTL_SET_RBM_MODE:
			printk("fwq GSENSOR_MCUBE_IOCTL_SET_RBM_MODE\n");
			//MCUBE_BACKUP_FILE
			if (READ_FROM_BACKUP == true)
			{
				mcube_copy_file(CALIB_PATH);
				READ_FROM_BACKUP = false;
			}
			//MCUBE_BACKUP_FILE
			MC3XXX_rbm(client,1);

			break;

	case GSENSOR_MCUBE_IOCTL_CLEAR_RBM_MODE:
			GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_SET_RBM_MODE\n");

			MC3XXX_rbm(client,0);

			break;

	case GSENSOR_MCUBE_IOCTL_REGISTER_MAP:
			GSE_LOG("fwq GSENSOR_MCUBE_IOCTL_REGISTER_MAP\n");

			//MC3XXX_Read_Reg_Map(client);

			break;
			
	default:
		return -ENOTTY;
	}

	switch (cmd) {

	case MC_IOCTL_GETDATA:
		if ( copy_to_user(argp, &sense_data, sizeof(sense_data) ) ) {
		    printk("failed to copy sense data to user space.");
			return -EFAULT;
		}
		break;
	case GSENSOR_IOCTL_READ_RAW_DATA:
	case GSENSOR_IOCTL_READ_SENSORDATA:
		if (copy_to_user(argp, &strbuf, strlen(strbuf)+1)) {
			printk("failed to copy sense data to user space.");
			return -EFAULT;
		}
		
		break;

	default:
		break;
	}

	return 0;
}

static int sensor_active(struct i2c_client *client, int enable, int rate)
{
	struct sensor_private_data *sensor =
	    (struct sensor_private_data *) i2c_get_clientdata(client);	
	int result = 0;
	int mc3XXX_rate = 0;

	mcprintkfunc("%s enter\n",__FUNCTION__);
# if 0
	mc3XXX_rate = 0xf8 | (0x07 & rate);

	result = sensor_write_reg(client, MC3XXX_REG_SAMPLE_RATE, mc3XXX_rate);	
 
	if(result)
	{
		printk("%s:line=%d,error\n",__func__,__LINE__);
		return result;
	}
		
	sensor->ops->ctrl_data = sensor_read_reg(client, sensor->ops->ctrl_reg);

	mcprintkfunc("SENSOR %s sensor_ctldata_original[0x%x].\n", __FUNCTION__,sensor->ops->ctrl_data);
	
	if(!enable)
	{	
		sensor->ops->ctrl_data &= ~MC3XXX_MODE_BITS;
		sensor->ops->ctrl_data |= MC3XXX_MODE_SLEEP;
	}
	else
	{
		sensor->ops->ctrl_data &= ~MC3XXX_MODE_BITS;
		sensor->ops->ctrl_data |= MC3XXX_MODE_WAKEUP;
	}

	mcprintkfunc("SENSOR %s sensor_ctldata_current[0x%x].\n", __FUNCTION__,sensor->ops->ctrl_data);
	
	result = sensor_write_reg(client, sensor->ops->ctrl_reg, sensor->ops->ctrl_data);
	if(result)
		printk("%s:fail to active sensor\n",__func__);
#endif	
if(!enable)
	{	
		mc3XXX_suspend(client);
	}
	else
	{
		mc3XXX_resume(client);
	}
	return result;

}

static int MC3XXX_is_init = 0;
static int sensor_init(struct i2c_client *client)
{	
	struct sensor_private_data *sensor =
	    (struct sensor_private_data *) i2c_get_clientdata(client);	
	int result = 0;

	mcprintkfunc(" %s entry.\n", __FUNCTION__);
	if(MC3XXX_is_init == 0)
	{
		MC3XXX_Reset(client);
		MC3XXX_chip_init(client);
	}	
	MC3XXX_is_init = 1;
	
	result = sensor->ops->active(client,0,0);
	if(result)
	{
		printk("%s:line=%d,error\n",__func__,__LINE__);
		return result;
	}
	
	sensor->status_cur = SENSOR_OFF;

	result = sensor_write_reg(client, MC3XXX_REG_INTERRUPT_ENABLE, 0x10);	
	if(result)
	{
		printk("%s:line=%d,error\n",__func__,__LINE__);
		return result;
	}

	result = sensor->ops->active(client,1,MC3XXX_RATE_32);
	if(result)
	{
		printk("%s:line=%d,error\n",__func__,__LINE__);
		return result;
	}

	return result;
}

static void _MC3XXX_LowResFilter(s16 nAxis, s16 naData[MC3XXX_AXES_NUM])
{
    #define _LRF_DIFF_COUNT_POS       1//2
    static s16 s_wLRF_RepValue[3] = {0};
    signed int    _nDiff    = 0;
    
    _nDiff = (naData[nAxis] - s_wLRF_RepValue[nAxis]);
    if(_nDiff>_LRF_DIFF_COUNT_POS)
     s_wLRF_RepValue[nAxis]= naData[nAxis]-_LRF_DIFF_COUNT_POS;
    if(_nDiff<-_LRF_DIFF_COUNT_POS)
     s_wLRF_RepValue[nAxis]= naData[nAxis]+_LRF_DIFF_COUNT_POS;
    
    naData[nAxis]= s_wLRF_RepValue[nAxis];
    #undef _LRF_DIFF_COUNT_POS
}

static int sensor_report_value(struct i2c_client *client)
{
	struct sensor_private_data* mc3XXX = (struct sensor_private_data *) i2c_get_clientdata(client);
	struct sensor_platform_data *pdata = pdata = client->dev.platform_data;
	struct mc3XXX_axis axis;
	s16 buffer[6];
	int ret;
	int x,y,z;
	
     	if( load_cali_flg > 0)
	{
		ret =mcube_read_cali_file(client);
		if(ret == 0)
			load_cali_flg = ret;
		else 
			load_cali_flg --;
		printk("load_cali %d %d\n",ret, load_cali_flg); 
	}  	
		
	if(ret = MC3XXX_ReadData(client, buffer))
	{    
		
		GSE_ERR("%s I2C error: ret value=%d", __func__,ret);
		return EIO;
	}
	
	mcprintkfunc("%s %d %d %d \n",__func__,buffer[0],buffer[1],buffer[2]);

	if ((IS_NEW_MC34x0== mc3XXX_type)||(IS_MC35XX== mc3XXX_type))
        {
            buffer[MC3XXX_AXIS_X] = -buffer[MC3XXX_AXIS_X];
            buffer[MC3XXX_AXIS_Y] = -buffer[MC3XXX_AXIS_Y];
        }
        else if(IS_MC3250== mc3XXX_type)
        {
            s16    temp = 0;

            temp = buffer[MC3XXX_AXIS_X];

            buffer[MC3XXX_AXIS_X] = buffer[MC3XXX_AXIS_Y];
            buffer[MC3XXX_AXIS_Y] = -temp;
        }

_MC3XXX_LowResFilter(MC3XXX_AXIS_X,buffer);
_MC3XXX_LowResFilter(MC3XXX_AXIS_Y,buffer);
//	printk("mc3XXX_convert_to_int buffer[0]=%d ",buffer[0]);
	x = mc3XXX_convert_to_int(buffer[0]);
//	printk("mc3XXX_convert_to_int buffer[1]=%d ",buffer[1]);
	y = mc3XXX_convert_to_int(buffer[1]);
//	printk("mc3XXX_convert_to_int buffer[2]=%d ",buffer[2]);
	z = mc3XXX_convert_to_int(buffer[2]);
	
	axis.x = (pdata->orientation[0])*x + (pdata->orientation[1])*y + (pdata->orientation[2])*z;
	axis.y = (pdata->orientation[3])*x + (pdata->orientation[4])*y + (pdata->orientation[5])*z;	
	axis.z = (pdata->orientation[6])*x + (pdata->orientation[7])*y + (pdata->orientation[8])*z;

	     
	mc3XXX_report_value(client, &axis);

    
	mutex_lock(&mc3XXX->data_mutex);
	memcpy(&axis, &mc3XXX->axis, sizeof(mc3XXX->axis));	
	mutex_unlock(&mc3XXX->data_mutex);

	atomic_set(&(mc3XXX->data_ready), 1);
	wake_up(&(mc3XXX->data_ready_wq) );

	return 0;
}


struct sensor_operate gsensor_ops = {
	.name			= "gs_mc3XXX",
	.type				= SENSOR_TYPE_ACCEL,	//sensor type and it should be correct
	.id_i2c			= ACCEL_ID_MC3XXX,	//i2c id number
	.read_reg			= MC3XXX_REG_XOUT,	//read data
	.read_len			= 3,					//data length
	.id_reg			= SENSOR_UNKNOW_DATA,					//read device id from this register,but mc3XXX has no id register
	.id_data 			= SENSOR_UNKNOW_DATA,					//device id
	.precision			= 8,								//6 bits
	.ctrl_reg 			= MC3XXX_REG_MODE_FEATURE,		//enable or disable 
	.int_status_reg 		= MC3XXX_REG_INTERRUPT_ENABLE,	//intterupt status register
	.range			= {-MC3XXX_RANGE, MC3XXX_RANGE},	//range
	.trig				= (IRQF_TRIGGER_LOW|IRQF_ONESHOT),		
	.active			= sensor_active,	
	.init				= sensor_init,
	.report			= sensor_report_value,
};

/****************operate according to sensor chip:end************/

//function name should not be changed
struct sensor_operate *gsensor_get_ops(void)
{
	return &gsensor_ops;
}

EXPORT_SYMBOL(gsensor_get_ops);

static int __init gsensor_init(void)
{
	printk("%s txx_senser_01\n",__func__);
	struct sensor_operate *ops = gsensor_get_ops();
	int result = 0;
	int type = ops->type;
	result = sensor_register_slave(type, NULL, NULL, gsensor_get_ops);
	printk("%s\n",__func__);
	return result;
}

static void __exit gsensor_exit(void)
{
	struct sensor_operate *ops = gsensor_get_ops();
	int type = ops->type;
	sensor_unregister_slave(type, NULL, NULL, gsensor_get_ops);
}


module_init(gsensor_init);
module_exit(gsensor_exit);

