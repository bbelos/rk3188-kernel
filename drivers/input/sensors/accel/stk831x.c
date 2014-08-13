/* drivers/input/sensors/access/stk831x.c
/* drivers/input/sensors/access/stk831x.c
 *
 * Copyright (C) 2012-2015 ROCKCHIP.
 * Author: luowei <lw@rock-chips.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <mach/gpio.h>
#include <mach/board.h> 
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/sensor-dev.h>

#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/fs.h> 
#include <linux/fcntl.h>
#include <linux/workqueue.h>
#include <linux/syscalls.h>

#define CONFIG_GS_STK8312
//#define CONFIG_GS_STK8313

#define STK_ACC_DRIVER_VERSION	"1.8.1 20140324"

#ifdef CONFIG_GS_STK8312
	#include "stk8312.h"
	#define CONFIG_SENSORS_STK8312
#elif defined(CONFIG_GS_STK8313)
	#include "stk8313.h"
	#define CONFIG_SENSORS_STK8313
#else
	#error "What's your stk accelerometer?"	
#endif

#if 0//0
//#define SENSOR_DEBUG_TYPE SENSOR_TYPE_ACCEL
#define DBG(x...)  printk(x)
#else
#define DBG(x...)
#endif

#define STK831X_ENABLE		1

#ifdef CONFIG_SENSORS_STK8312
	#define STK831X_REG_X_OUT       0x0
	#define STK831X_REG_Y_OUT       0x1
	#define STK831X_REG_Z_OUT       0x2
	#define STK831X_REG_TILT        0x3
	#define STK831X_REG_SRST        0x4
	#define STK831X_REG_SPCNT       0x5
	#define STK831X_REG_INTSU       0x6
	#define STK831X_REG_MODE        0x7
	#define STK831X_REG_SR          0x8
	#define STK831X_REG_PDET        0x9
	
	#define STK831X_RANGE			6000000	//1500000
	#define STK831X_PRECISION       8	//6
#elif defined(CONFIG_SENSORS_STK8313)
	#define STK831X_REG_X_OUT       0x0
	#define STK831X_REG_Y_OUT       0x2
	#define STK831X_REG_Z_OUT       0x4
	#define STK831X_REG_TILT        0x6
	#define STK831X_REG_SRST        0x7
	#define STK831X_REG_SPCNT       0x8
	#define STK831X_REG_INTSU       0x9
	#define STK831X_REG_MODE        0xA	
	#define STK831X_REG_SR          0xB
	#define STK831X_REG_PDET        0xC
	
	#define STK831X_RANGE			8000000	
	#define STK831X_PRECISION       12	//6
#endif

#define STK831X_SPEED           200 * 1000


#define STK831X_BOUNDARY		(0x1 << (STK831X_PRECISION - 1))
#define STK831X_GRAVITY_STEP		(STK831X_RANGE / STK831X_BOUNDARY)

#define STK831X_COUNT_AVERAGE	2

/*choose polling or interrupt mode*/
#define STK_ACC_POLLING_MODE	1

struct stk831x_data 
{
	struct input_dev *input_dev;
	struct work_struct stk_work;
	int irq;	
	int raw_data[3]; 
	atomic_t enabled;
	unsigned char delay;	
	struct mutex read_lock;
	bool first_enable;
	
#if STK_ACC_POLLING_MODE
	atomic_t run_thread;
#endif	//#if STK_ACC_POLLING_MODE
};
static int suspend_flag = 0;
static int is_reading_reg = 0;
#define STK831X_INIT_ODR		2		//2:100Hz, 3:50Hz, 4:25Hz
const static int STK831X_SAMPLE_TIME[6] = {2500, 5000, 10000, 20000, 40000, 80000};
static int event_since_en = 0;
static int event_since_en_limit = 20;

//#define STK_DEBUG_CALI
//#define STORE_OFFSET_IN_FILE
//#define STORE_OFFSET_IN_IC

#define POSITIVE_Z_UP		0
#define NEGATIVE_Z_UP	1
#define POSITIVE_X_UP		2
#define NEGATIVE_X_UP	3
#define POSITIVE_Y_UP		4
#define NEGATIVE_Y_UP	5


#define STK_SAMPLE_NO				10
#define STK_ACC_CALI_VER0			0x3D
#define STK_ACC_CALI_VER1			0x01
#define STK_ACC_CALI_FILE 			"/data/misc/stkacccali.conf"
#define STK_ACC_CALI_FILE_SIZE 		10

#define STK_K_SUCCESS_TUNE			0x04
#define STK_K_SUCCESS_FT2			0x03
#define STK_K_SUCCESS_FT1			0x02
#define STK_K_SUCCESS_FILE			0x01
#define STK_K_NO_CALI				0xFF
#define STK_K_RUNNING				0xFE
#define STK_K_FAIL_LRG_DIFF			0xFD
#define STK_K_FAIL_OPEN_FILE			0xFC
#define STK_K_FAIL_W_FILE				0xFB
#define STK_K_FAIL_R_BACK				0xFA
#define STK_K_FAIL_R_BACK_COMP		0xF9
#define STK_K_FAIL_I2C				0xF8
#define STK_K_FAIL_K_PARA				0xF7
#define STK_K_FAIL_OTP_OUT_RG		0xF6
#define STK_K_FAIL_ENG_I2C			0xF5
#define STK_K_FAIL_FT1_USD			0xF4
#define STK_K_FAIL_FT2_USD			0xF3
#define STK_K_FAIL_WRITE_NOFST		0xF2
#define STK_K_FAIL_OTP_5T				0xF1
#define STK_K_FAIL_PLACEMENT			0xF0

#define STK_PERMISSION_THREAD

//#define STK_LOWPASS
#define STK_FIR_LEN 6

#define STK_TUNE
#ifdef CONFIG_SENSORS_STK8312
	#define STK_TUNE_XYOFFSET 5
	#define STK_TUNE_ZOFFSET 6
	#define STK_TUNE_NOISE 5
#elif defined (CONFIG_SENSORS_STK8313)
	#define STK_TUNE_XYOFFSET 35
	#define STK_TUNE_ZOFFSET 75
	#define STK_TUNE_NOISE 20	
#endif
#define STK_TUNE_NUM 100
#define STK_TUNE_DELAY 100

//#define STK_ZG_FILTER
#ifdef CONFIG_SENSORS_STK8312
	#define STK_ZG_COUNT	1
#elif defined (CONFIG_SENSORS_STK8313)
	#define STK_ZG_COUNT	4
#endif

#ifdef STK_TUNE
static char stk_tune_offset_record[3] = {0};
static int stk_tune_offset[3] = {0};
static int stk_tune_sum[3] = {0};
static int stk_tune_max[3] = {0};
static int stk_tune_min[3] = {0};
static int stk_tune_index = 0;
static int stk_tune_done = 0;
static atomic_t cali_status;
static bool first_enable;
#endif

#if defined(STK_LOWPASS)
#define MAX_FIR_LEN 32
struct data_filter {
	s16 raw[MAX_FIR_LEN][3];
	int sum[3];
	int num;
	int idx;
};

	int fir_en;
	struct data_filter fir;
	int firlength = STK_FIR_LEN;
#endif

/*noCreateAttr:the initial is 1-->no create attr. if created, change noCreateAttr to 0.*/
static int noCreateAttr = 1;

static int stk831x_placement = POSITIVE_Z_UP;
static int cali_raw_data[3] = {0};
static int show_raw_data[3] = {0};
static char getenable = 0;
static char recv_reg = 0;
static int getdelay = 40;

static int sensor_report_value(struct i2c_client *client);
static int sensor_active(struct i2c_client *client, int enable, int rate);
static int STK831x_ReadByteOTP(char rReg, char *value);
static int STK831x_WriteByteOTP(char wReg, char value);
static int32_t stk_get_ic_content(struct i2c_client *client);
static int STK831x_Init(struct i2c_client *client);
static int STK831x_SetVD(struct i2c_client *client);
static int stk_store_in_ic(char otp_offset[], char FT_index, unsigned char stk831x_placement);


struct sensor_axis_average {
		int x_average;
		int y_average;
		int z_average;
		int count;
};

struct i2c_client *this_client;


static int STK_i2c_Tx(struct i2c_client *client,char *txData, int length)
{
        int ret = 0;
        char reg = txData[0];
        ret = i2c_master_reg8_send(client, reg, &txData[1], length-1, STK831X_SPEED);
		
		if(reg >= 0x21 && reg <= 0x3E)
			ret = i2c_master_reg8_send(client, reg, &txData[1], length-1, STK831X_SPEED);
		
        return (ret > 0)? 0 : ret;

}

static int STK831x_SetOffset(char buf[])
{
	int result;
	char buffer[4] = "";
	
	buffer[0] = STK831X_OFSX;	
	buffer[1] = buf[0];
	buffer[2] = buf[1];
	buffer[3] = buf[2];
	result = STK_i2c_Tx(this_client, buffer, 4);
	if (result < 0) 
	{
		DBG("%s:failed\n", __func__);
		return result;
	}	
	return 0;
}

static int STK831x_GetOffset(char buf[])
{	
	buf[0] = sensor_read_reg(this_client, STK831X_OFSX);
	buf[1] = sensor_read_reg(this_client, STK831X_OFSX+1);
	buf[2] = sensor_read_reg(this_client, STK831X_OFSX+2);
	return 0;
}

#ifdef STK_PERMISSION_THREAD
static struct task_struct *STKPermissionThread = NULL;

static int stk_permission_thread(void *data)
{
	int ret = 0;
	int retry = 0;
	mm_segment_t fs = get_fs();
	set_fs(KERNEL_DS);	
	msleep(10000);
	do{
		msleep(5000);
		ret = sys_chmod(STK_ACC_CALI_FILE , 0666);	
		ret = sys_fchmodat(AT_FDCWD, STK_ACC_CALI_FILE , 0666);
		//if(ret < 0)
		//	printk("fail to execute sys_fchmodat, ret = %d\n", ret);
		if(retry++ > 10)
			break;
	}while(ret == -ENOENT);
	set_fs(fs);
	DBG(KERN_INFO "%s exit, retry=%d\n", __func__, retry);
	return 0;
}
#endif	/*	#ifdef STK_PERMISSION_THREAD	*/

static int stk_store_in_file(char offset[], char mode)
{
	struct file  *cali_file;
	char r_buf[STK_ACC_CALI_FILE_SIZE] = {0};
	char w_buf[STK_ACC_CALI_FILE_SIZE] = {0};	
	mm_segment_t fs;	
	ssize_t ret;
	int8_t i;
	
	w_buf[0] = STK_ACC_CALI_VER0;
	w_buf[1] = STK_ACC_CALI_VER1;
	w_buf[2] = offset[0];
	w_buf[3] = offset[1];
	w_buf[4] = offset[2];
	w_buf[5] = mode;
	
	cali_file = filp_open(STK_ACC_CALI_FILE, O_CREAT | O_RDWR,0666);
	
	if(IS_ERR(cali_file))
	{
		DBG("%s: filp_open error!\n", __func__);
		return -STK_K_FAIL_OPEN_FILE;
	}
	else
	{
		fs = get_fs();
		set_fs(get_ds());
		
		ret = cali_file->f_op->write(cali_file,w_buf,STK_ACC_CALI_FILE_SIZE,&cali_file->f_pos);
		if(ret != STK_ACC_CALI_FILE_SIZE)
		{
			DBG("%s: write error!\n", __func__);
			filp_close(cali_file,NULL);
			return -STK_K_FAIL_W_FILE;
		}
		cali_file->f_pos=0x00;
		ret = cali_file->f_op->read(cali_file,r_buf, STK_ACC_CALI_FILE_SIZE,&cali_file->f_pos);
		if(ret < 0)
		{
			DBG("%s: read error!\n", __func__);
			filp_close(cali_file,NULL);
			return -STK_K_FAIL_R_BACK;
		}		
		set_fs(fs);
		
		DBG("%s: read ret=%d!\n", __func__, ret);
		for(i=0;i<STK_ACC_CALI_FILE_SIZE;i++)
		{
			if(r_buf[i] != w_buf[i])
			{
				DBG("%s: read back error, r_buf[%x](0x%x) != w_buf[%x](0x%x)\n", 
					__func__, i, r_buf[i], i, w_buf[i]);				
				filp_close(cali_file,NULL);
				return -STK_K_FAIL_R_BACK_COMP;
			}
		}
	}
	filp_close(cali_file,NULL); 
	
#ifdef STK_PERMISSION_THREAD
	fs = get_fs();
	set_fs(KERNEL_DS);
	ret = sys_chmod(STK_ACC_CALI_FILE , 0666);
	ret = sys_fchmodat(AT_FDCWD, STK_ACC_CALI_FILE , 0666);
	set_fs(fs);		
#endif

	DBG("%s successfully\n", __func__);
	return 0;		
}

static int getcode(int a)
{
	int i=0,j=0,b=0,d=0,n=0,dd=0;
	for(i=0;i*i<a;i++)
	{
		j = (i+1)*10;
	}
	i = (i-1)*10;
	d = 100;	
	a = a*d;
	while(d>10)	
	{
		b = (i+j)>>1;
		if(b*b>a)
		{
			j = b;
		}
		else
		{
			i = b;
		}
		d = a-i*i;
		if(dd == d)
		{
			n++;
			if(n>=3)
				break;
		}
		else
		{
			n = 0;
		}
		dd = d;
	}
	if((i%10)>=5)
		return (i/10)+1;
	else
		return (i/10);
}

static int STK831x_SetVD(struct i2c_client *client)
{
//	struct sensor_private_data *sensor =
//		(struct sensor_private_data *) i2c_get_clientdata(client);	
	int result = 0;
	unsigned char status = 0;
	unsigned char readvalue = 0;
	unsigned int count = 10;  

/*
	result = sensor_write_reg(client, 0x3D, 0x70);
	if(result)
	{
		DBG("%s:line=%d,error\n",__func__,__LINE__);
		return result;
	}
	result = sensor_write_reg(client, 0x3D, 0x70);
	if(result)
	{
		DBG("%s:line=%d,error\n",__func__,__LINE__);
		return result;
	}
	
	result = sensor_write_reg(client, 0x3F, 0x02);
	if(result)
	{
		DBG("%s:line=%d,error\n",__func__,__LINE__);
		return result;
	}
	result = sensor_write_reg(client, 0x3F, 0x02);
	if(result)
	{
		DBG("%s:line=%d,error\n",__func__,__LINE__);
		return result;
	}

	while((status>>7)!=1 && count!=0)
	{
		status = sensor_read_reg(client, 0x3F);
		--count;
		msleep(2);
	}
	
	readvalue = sensor_read_reg(client, 0x3E);
*/
	msleep(2);
	result = STK831x_ReadByteOTP(0x70, &readvalue);
	
	if(readvalue != 0x00)
	{
		result = sensor_write_reg(client, 0x24, readvalue);
		if(result)
		{
			DBG("%s:line=%d,error\n",__func__,__LINE__);
			return result;
		}
		//printk("%s: readvalue = 0x%x \n",__func__, readvalue);
	}
	else
	{
		//printk("%s: reg24=0, do nothing\n", __func__);
		return 0;		
	}
	
#if 0
/*
	result = sensor_write_reg(client, 0x24, 0xdc);
	if(result)
	{
		DBG("%s:line=%d,error\n",__func__,__LINE__);
		return result;
	}
*/
#endif
	
	status = sensor_read_reg(client, 0x24);
	if(status != readvalue)
	{
		printk(KERN_ERR "%s: error, reg24=0x%x, read=0x%x\n", __func__, readvalue, status);
		return -1;
	}
	//printk(KERN_INFO "%s: successfully", __func__);	
	
	//msleep(200);	
	return result;
}

static int STK831x_WriteOffsetOTP(struct i2c_client *client, int FT, char offsetData[])
{
	char regR[6], reg_comp[3];
	char mode; 
	int result;
	char buffer[2] = "";
	int ft_pre_trim = 0;
	
//Check FT1
	if(FT==1)
	{
		result = STK831x_ReadByteOTP(0x7F, &regR[0]);
		if(result < 0)
			goto eng_i2c_err;
		
		if(regR[0]&0x10)
		{
			printk(KERN_ERR "%s: 0x7F=0x%x\n", __func__, regR[0]);
			return -STK_K_FAIL_FT1_USD;
		}
	}
	else if (FT == 2)
	{
		result = STK831x_ReadByteOTP(0x7F, &regR[0]);
		if(result < 0)
			goto eng_i2c_err;
			
		if(regR[0]&0x20)
		{
			printk(KERN_ERR "%s: 0x7F=0x%x\n", __func__, regR[0]);
			return -STK_K_FAIL_FT2_USD;
		}		
	}
//Check End
	
	buffer[0] = sensor_read_reg(client, STK831X_MODE);
	mode = buffer[0];
	buffer[1] = (mode | 0x01);
	buffer[0] = STK831X_MODE;	
	result = STK_i2c_Tx(client, buffer, 2);
	if (result < 0) 
	{
		goto common_i2c_error;
	}
	msleep(2);


	if(FT == 1)
	{
		result = STK831x_ReadByteOTP(0x40, &reg_comp[0]);
		if(result < 0)
			goto eng_i2c_err;
		result = STK831x_ReadByteOTP(0x41, &reg_comp[1]);
		if(result < 0)
			goto eng_i2c_err;
		result = STK831x_ReadByteOTP(0x42, &reg_comp[2]);
		if(result < 0)
			goto eng_i2c_err;	
	}
	else if (FT == 2)
	{
		result = STK831x_ReadByteOTP(0x50, &reg_comp[0]);
		if(result < 0)
			goto eng_i2c_err;
		result = STK831x_ReadByteOTP(0x51, &reg_comp[1]);
		if(result < 0)
			goto eng_i2c_err;
		result = STK831x_ReadByteOTP(0x52, &reg_comp[2]);
		if(result < 0)
			goto eng_i2c_err;					
	}

	result = STK831x_ReadByteOTP(0x30, &regR[0]);
	if(result < 0)
		goto eng_i2c_err;
	result = STK831x_ReadByteOTP(0x31, &regR[1]);
	if(result < 0)
		goto eng_i2c_err;
	result = STK831x_ReadByteOTP(0x32, &regR[2]);
	if(result < 0)
		goto eng_i2c_err;
		
	if(reg_comp[0] == regR[0] && reg_comp[1] == regR[1] && reg_comp[2] == regR[2])
	{
		printk(KERN_INFO "%s: ft pre-trimmed\n", __func__);
		ft_pre_trim = 1;
	}
	
	if(!ft_pre_trim)
	{
		if(FT == 1)
		{		
			result = STK831x_WriteByteOTP(0x40, regR[0]);
			if(result < 0)
				goto eng_i2c_err;
			result = STK831x_WriteByteOTP(0x41, regR[1]);
			if(result < 0)
				goto eng_i2c_err;		
			result = STK831x_WriteByteOTP(0x42, regR[2]);
			if(result < 0)
				goto eng_i2c_err;		
		}
		else if (FT == 2)
		{
			result = STK831x_WriteByteOTP(0x50, regR[0]);
			if(result < 0)
				goto eng_i2c_err;
			result = STK831x_WriteByteOTP(0x51, regR[1]);
			if(result < 0)
				goto eng_i2c_err;		
			result = STK831x_WriteByteOTP(0x52, regR[2]);
			if(result < 0)
				goto eng_i2c_err;		
		}
	}
#ifdef STK_DEBUG_CALI
	printk(KERN_INFO "%s:OTP step1 Success!\n", __func__);
#endif
	buffer[0] = sensor_read_reg(client, 0x2A);
	regR[0] = buffer[0];

	buffer[0] = sensor_read_reg(client, 0x2B);
	regR[1] = buffer[0];

	buffer[0] = sensor_read_reg(client, 0x2E);
	regR[2] = buffer[0];

	buffer[0] = sensor_read_reg(client, 0x2F);
	regR[3] = buffer[0];

	buffer[0] = sensor_read_reg(client, 0x32);
	regR[4] = buffer[0];

	buffer[0] = sensor_read_reg(client, 0x33);
	regR[5] = buffer[0];

	
	regR[1] = offsetData[0];
	regR[3] = offsetData[2];
	regR[5] = offsetData[1];
	if(FT==1)
	{
		result = STK831x_WriteByteOTP(0x44, regR[1]);
		if(result < 0)
			goto eng_i2c_err;		
		result = STK831x_WriteByteOTP(0x46, regR[3]);
		if(result < 0)
			goto eng_i2c_err;				
		result = STK831x_WriteByteOTP(0x48, regR[5]);
		if(result < 0)
			goto eng_i2c_err;				
		
		if(!ft_pre_trim)
		{
			result = STK831x_WriteByteOTP(0x43, regR[0]);
			if(result < 0)
				goto eng_i2c_err;		
			result = STK831x_WriteByteOTP(0x45, regR[2]);
			if(result < 0)
				goto eng_i2c_err;				
			result = STK831x_WriteByteOTP(0x47, regR[4]);
			if(result < 0)
				goto eng_i2c_err;				
		}
	}
	else if (FT == 2)
	{
		result = STK831x_WriteByteOTP(0x54, regR[1]);
		if(result < 0)
			goto eng_i2c_err;
		result = STK831x_WriteByteOTP(0x56, regR[3]);
		if(result < 0)
			goto eng_i2c_err;				
		result = STK831x_WriteByteOTP(0x58, regR[5]);
		if(result < 0)
			goto eng_i2c_err;				

		if(!ft_pre_trim)
		{		
			result = STK831x_WriteByteOTP(0x53, regR[0]);
			if(result < 0)
				goto eng_i2c_err;								
			result = STK831x_WriteByteOTP(0x55, regR[2]);
			if(result < 0)
				goto eng_i2c_err;				
			result = STK831x_WriteByteOTP(0x57, regR[4]);
			if(result < 0)
				goto eng_i2c_err;				
		}
	}
#ifdef STK_DEBUG_CALI	
	printk(KERN_INFO "%s:OTP step2 Success!\n", __func__);
#endif
	result = STK831x_ReadByteOTP(0x7F, &regR[0]);
	if(result < 0)
		goto eng_i2c_err;
	
	if(FT==1)
		regR[0] = regR[0]|0x10;
	else if(FT==2)
		regR[0] = regR[0]|0x20;

	result = STK831x_WriteByteOTP(0x7F, regR[0]);
	if(result < 0)
		goto eng_i2c_err;
#ifdef STK_DEBUG_CALI	
	printk(KERN_INFO "%s:OTP step3 Success!\n", __func__);
#endif	
	return 0;
	
eng_i2c_err:
	printk(KERN_ERR "%s: read/write eng i2c error, result=0x%x\n", __func__, result);	
	return result;
	
common_i2c_error:
	printk(KERN_ERR "%s: read/write common i2c error, result=0x%x\n", __func__, result);
	return result;	
}

static int STK831X_VerifyCali(struct i2c_client *client, unsigned char en_dis)
{
	unsigned char axis, state;	
	int acc_ave[3] = {0, 0, 0};
	const unsigned char verify_sample_no = 3;		
#ifdef CONFIG_SENSORS_STK8313
	const unsigned char verify_diff = 25;	
#elif defined CONFIG_SENSORS_STK8312
	const unsigned char verify_diff = 2;		
#endif		
	int result;
	char buffer[2] = "";
	int ret = 0;
	
	if(en_dis)
	{
		buffer[0] = sensor_read_reg(client, STK831X_MODE);
	
		buffer[1] = (buffer[0] & 0xF8) | 0x01;
		buffer[0] = STK831X_MODE;	
		result = STK_i2c_Tx(client, buffer, 2);
		if (result < 0) 
		{
			printk(KERN_ERR "%s:failed, result=0x%x\n", __func__, result);			
			return -STK_K_FAIL_I2C;
		}
		STK831x_SetVD(client);			
		msleep(getdelay*20);	
	}
	
	for(state=0;state<verify_sample_no;state++)
	{
		sensor_report_value(client);
		for(axis=0;axis<3;axis++)			
			acc_ave[axis] += cali_raw_data[axis];	
#ifdef STK_DEBUG_CALI				
		printk(KERN_INFO "%s: acc=%d,%d,%d\n", __func__, cali_raw_data[0], cali_raw_data[1], cali_raw_data[2]);	
#endif
		msleep(20);		
	}		
	
	for(axis=0;axis<3;axis++)
		acc_ave[axis] /= verify_sample_no;
	
	switch(stk831x_placement)
	{
	case POSITIVE_X_UP:
		acc_ave[0] -= STK_LSB_1G;
		break;
	case NEGATIVE_X_UP:
		acc_ave[0] += STK_LSB_1G;		
		break;
	case POSITIVE_Y_UP:
		acc_ave[1] -= STK_LSB_1G;
		break;
	case NEGATIVE_Y_UP:
		acc_ave[1] += STK_LSB_1G;
		break;
	case POSITIVE_Z_UP:
		acc_ave[2] -= STK_LSB_1G;
		break;
	case NEGATIVE_Z_UP:
		acc_ave[2] += STK_LSB_1G;
		break;
	default:
		printk("%s: invalid stk831x_placement=%d\n", __func__, stk831x_placement);
		ret = -STK_K_FAIL_PLACEMENT;
		break;
	}	
	if(abs(acc_ave[0]) > verify_diff || abs(acc_ave[1]) > verify_diff || abs(acc_ave[2]) > verify_diff)
	{
		printk(KERN_INFO "%s:Check data x:%d, y:%d, z:%d\n", __func__,acc_ave[0],acc_ave[1],acc_ave[2]);		
		printk(KERN_ERR "%s:Check Fail, Calibration Fail\n", __func__);
		ret = -STK_K_FAIL_LRG_DIFF;
	}	
#ifdef STK_DEBUG_CALI
	else
		printk(KERN_INFO "%s:Check data pass\n", __func__);
#endif	
	if(en_dis)
	{
		buffer[0] = sensor_read_reg(client, STK831X_MODE);
				
		buffer[1] = (buffer[0] & 0xF8);
		buffer[0] = STK831X_MODE;	
		result = STK_i2c_Tx(client, buffer, 2);
		if (result < 0) 
		{
			printk(KERN_ERR "%s:failed, result=0x%x\n", __func__, result);			
			return -STK_K_FAIL_I2C;
		}		
	}	
	
	return ret;
}

static int STK831x_SetCali(struct i2c_client *client, char sstate)
{
	char org_enable;
	int acc_ave[3] = {0, 0, 0};
	int state, axis;
	int new_offset[3];
	char char_offset[3] = {0};
	int result;
	char buffer[2] = "";
	char reg_offset[3] = {0};
	char store_location = sstate;
	char offset[3];	
	
	atomic_set(&cali_status, STK_K_RUNNING);	
	//sstate=1, STORE_OFFSET_IN_FILE
	//sstate=2, STORE_OFFSET_IN_IC		
#ifdef STK_DEBUG_CALI		
	printk(KERN_INFO "%s:store_location=%d\n", __func__, store_location);
#endif	
	if((store_location != 3 && store_location != 2 && store_location != 1) || (stk831x_placement < 0 || stk831x_placement > 5) )
	{
		printk(KERN_ERR "%s, invalid parameters\n", __func__);
		atomic_set(&cali_status, STK_K_FAIL_K_PARA);	
		return -STK_K_FAIL_K_PARA;
	}	
	
	org_enable = getenable;
	if(org_enable)
		sensor_active(client, 0, 0);
	STK831x_SetOffset(reg_offset);
	
	buffer[0] = sensor_read_reg(client, STK831X_MODE);
	
	buffer[1] = (buffer[0] & 0xF8) | 0x01;
	buffer[0] = STK831X_MODE;	
	result = STK_i2c_Tx(client, buffer, 2);
	if (result < 0) 
	{
		goto err_i2c_rw;
	}

	STK831x_SetVD(client);
	if(store_location >= 2)
	{
		buffer[0] = 0x2B;	
		buffer[1] = 0x0;
		result = STK_i2c_Tx(client, buffer, 2);
		if (result < 0) 
		{
			goto err_i2c_rw;
		}
		buffer[0] = 0x2F;	
		buffer[1] = 0x0;
		result = STK_i2c_Tx(client, buffer, 2);
		if (result < 0) 
		{
			goto err_i2c_rw;
		}
		buffer[0] = 0x33;	
		buffer[1] = 0x0;
		result = STK_i2c_Tx(client, buffer, 2);
		if (result < 0) 
		{
			goto err_i2c_rw;
		}
	}	

	msleep(getdelay*20);				
	for(state=0;state<STK_SAMPLE_NO;state++)
	{
		sensor_report_value(client);
		for(axis=0;axis<3;axis++)			
			acc_ave[axis] += cali_raw_data[axis];	
#ifdef STK_DEBUG_CALI				
		printk(KERN_INFO "%s: acc=%d,%d,%d\n", __func__, cali_raw_data[0], cali_raw_data[1], cali_raw_data[2]);	
#endif		
		msleep(20);		
	}		
	
	buffer[0] = sensor_read_reg(client, STK831X_MODE);
		
	buffer[1] = (buffer[0] & 0xF8);
	buffer[0] = STK831X_MODE;	
	result = STK_i2c_Tx(client, buffer, 2);
	if (result < 0) 
	{
		goto err_i2c_rw;
	}	
	
	for(axis=0;axis<3;axis++)
	{
		if((abs(acc_ave[axis]) % STK_SAMPLE_NO) >= (STK_SAMPLE_NO / 2))
		{
			acc_ave[axis] /= STK_SAMPLE_NO;
			if(acc_ave[axis] >= 1)
				acc_ave[axis] += 1;
			else if(acc_ave[axis] <= -1)
				acc_ave[axis] -= 1;			
		}
		else
		{
			acc_ave[axis] /= STK_SAMPLE_NO;
		}		
	}
	if(acc_ave[2] <= -1)
		stk831x_placement = NEGATIVE_Z_UP;
	else if((acc_ave[2] >= 1))
		stk831x_placement = POSITIVE_Z_UP;
#ifdef STK_DEBUG_CALI		
	printk(KERN_INFO "%s:stk831x_placement=%d\n", __func__, stk831x_placement);
#endif	
	
	switch(stk831x_placement)
	{
	case POSITIVE_X_UP:
		acc_ave[0] -= STK_LSB_1G;
		break;
	case NEGATIVE_X_UP:
		acc_ave[0] += STK_LSB_1G;		
		break;
	case POSITIVE_Y_UP:
		acc_ave[1] -= STK_LSB_1G;
		break;
	case NEGATIVE_Y_UP:
		acc_ave[1] += STK_LSB_1G;
		break;
	case POSITIVE_Z_UP:
		acc_ave[2] -= STK_LSB_1G;
		break;
	case NEGATIVE_Z_UP:
		acc_ave[2] += STK_LSB_1G;
		break;
	default:
		printk("%s: invalid stk831x_placement=%d\n", __func__, stk831x_placement);
		atomic_set(&cali_status, STK_K_FAIL_PLACEMENT);	
		return -STK_K_FAIL_K_PARA;
		break;
	}		
	
	for(axis=0;axis<3;axis++)
	{
		acc_ave[axis] = -acc_ave[axis];
		new_offset[axis] = acc_ave[axis];
		char_offset[axis] = new_offset[axis];
	}				
#ifdef STK_DEBUG_CALI	
	printk(KERN_INFO "%s: New offset:%d,%d,%d\n", __func__, new_offset[0], new_offset[1], new_offset[2]);	
#endif	
	if(store_location == 1)
	{
		STK831x_SetOffset(char_offset);
		msleep(1);
		STK831x_GetOffset(reg_offset);
		for(axis=0;axis<3;axis++)
		{
			if(char_offset[axis] != reg_offset[axis])		
			{
				printk(KERN_ERR "%s: set offset to register fail!, char_offset[%d]=%d,reg_offset[%d]=%d\n", 
					__func__, axis,char_offset[axis], axis, reg_offset[axis]);
				atomic_set(&cali_status, STK_K_FAIL_WRITE_NOFST);				
				return -STK_K_FAIL_WRITE_NOFST;
			}
		}
		
		
		result = STK831X_VerifyCali(client, 1);
		if(result)
		{
			printk(KERN_ERR "%s: calibration check fail, result=0x%x\n", __func__, result);
			atomic_set(&cali_status, -result);
		}
		else
		{
			result = stk_store_in_file(char_offset, STK_K_SUCCESS_FILE);
			if(result)
			{
				printk(KERN_INFO "%s:write calibration failed\n", __func__);
				atomic_set(&cali_status, -result);				
			}
			else
			{
				printk(KERN_INFO "%s successfully\n", __func__);
				atomic_set(&cali_status, STK_K_SUCCESS_FILE);
			}		
			
		}
	}
	else if(store_location >= 2)
	{
		for(axis=0; axis<3; axis++)
		{
#ifdef CONFIG_SENSORS_STK8313
			new_offset[axis]>>=2;
#endif				
			char_offset[axis] = (char)new_offset[axis];
			if( (char_offset[axis]>>7)==0)
			{
				if(char_offset[axis] >= 0x20 )
				{
					printk(KERN_ERR "%s: offset[%d]=0x%x is too large, limit to 0x1f\n", 
									__func__, axis, char_offset[axis] );
					char_offset[axis] = 0x1F;
					//atomic_set(&cali_status, STK_K_FAIL_OTP_OUT_RG);						
					//return -STK_K_FAIL_OTP_OUT_RG;
				}
			}	
			else
			{
				if(char_offset[axis] <= 0xDF)
				{
					printk(KERN_ERR "%s: offset[%d]=0x%x is too large, limit to 0x20\n", 
									__func__, axis, char_offset[axis]);				
					char_offset[axis] = 0x20;
					//atomic_set(&cali_status, STK_K_FAIL_OTP_OUT_RG);			
					//return -STK_K_FAIL_OTP_OUT_RG;					
				}
				else
					char_offset[axis] = char_offset[axis] & 0x3f;
			}			
		}

		printk(KERN_INFO "%s: OTP offset:0x%x,0x%x,0x%x\n", __func__, char_offset[0], char_offset[1], char_offset[2]);
		
		if(store_location == 2)
		{
			result = stk_store_in_ic( char_offset, 1, stk831x_placement);
			if(result == 0)
			{
				printk(KERN_INFO "%s successfully\n", __func__);
				atomic_set(&cali_status, STK_K_SUCCESS_FT1);
			}
			else
			{
				printk(KERN_ERR "%s fail, result=%d\n", __func__, result);
			}
		}
		else if(store_location == 3)
		{
			result = stk_store_in_ic( char_offset, 2, stk831x_placement);
			if(result == 0)
			{
				printk(KERN_INFO "%s successfully\n", __func__);
				atomic_set(&cali_status, STK_K_SUCCESS_FT2);
			}
			else
			{
				printk(KERN_ERR "%s fail, result=%d\n", __func__, result);
			}
		}
		
		offset[0] = offset[1] = offset[2] = 0;
		stk_store_in_file(offset, store_location);				
	}
#ifdef STK_TUNE	
	stk_tune_offset_record[0] = 0;
	stk_tune_offset_record[1] = 0;
	stk_tune_offset_record[2] = 0;
	stk_tune_done = 1;
#endif	
	first_enable = false;		
	
	if(org_enable)
		sensor_active(client, 1, 0);		
	return 0;
	
err_i2c_rw:
	first_enable = false;		
	if(org_enable)
		sensor_active(client, 1, 0);				
	printk(KERN_ERR "%s: i2c read/write error, err=0x%x\n", __func__, result);
	atomic_set(&cali_status, STK_K_FAIL_I2C);	
	return result;
}

static int32_t stk_get_ic_content(struct i2c_client *client)
{
	int result;
	char regR;
		
	result = STK831x_ReadByteOTP(0x7F, &regR);
	if(result < 0)
	{
		printk(KERN_ERR "%s: read/write eng i2c error, result=0x%x\n", __func__, result);	
		return result;
	}
	
	if(regR&0x20)
	{
		atomic_set(&cali_status, STK_K_SUCCESS_FT2);	
		printk(KERN_INFO "%s: OTP 2 used\n", __func__);
		return 2;	
	}
	if(regR&0x10)	
	{
		atomic_set(&cali_status, STK_K_SUCCESS_FT1);	
		printk(KERN_INFO "%s: OTP 1 used\n", __func__);		
		return 1;	
	}
	return 0;
}

static int stk_store_in_ic(char otp_offset[], char FT_index, unsigned char stk831x_placement)
{
	int result;
	char buffer[2] = "";

	buffer[0] = sensor_read_reg(this_client, STK831X_MODE);			
	buffer[1] = (buffer[0] & 0xF8) | 0x01;
	buffer[0] = STK831X_MODE;	
	result = STK_i2c_Tx(this_client, buffer, 2);
	if (result < 0) 
	{
		goto ic_err_i2c_rw;
	}		
	STK831x_SetVD(this_client);
	
	buffer[0] = 0x2B;	
	buffer[1] = otp_offset[0];
	result = STK_i2c_Tx(this_client, buffer, 2);
	if (result < 0) 
	{
		goto ic_err_i2c_rw;
	}
	buffer[0] = 0x2F;	
	buffer[1] = otp_offset[2];
	result = STK_i2c_Tx(this_client, buffer, 2);
	if (result < 0) 
	{
		goto ic_err_i2c_rw;
	}
	buffer[0] = 0x33;	
	buffer[1] = otp_offset[1];
	result = STK_i2c_Tx(this_client, buffer, 2);
	if (result < 0) 
	{
		goto ic_err_i2c_rw;
	}		
	

#ifdef STK_DEBUG_CALI	
	//printk(KERN_INFO "%s:Check All OTP Data after write 0x2B 0x2F 0x33\n", __func__);
	//STK831x_ReadAllOTP();
#endif	
	
	msleep(getdelay*20);		
	result = STK831X_VerifyCali(this_client, 0);
	if(result)
	{
		printk(KERN_ERR "%s: calibration check1 fail, FT_index=%d\n", __func__, FT_index);				
		goto ic_err_misc;
	}
#ifdef STK_DEBUG_CALI		
	//printk(KERN_INFO "\n%s:Check All OTP Data before write OTP\n", __func__);
	//STK831x_ReadAllOTP();

#endif	
	//Write OTP	
	printk(KERN_INFO "\n%s:Write offset data to FT%d OTP\n", __func__, FT_index);
	result = STK831x_WriteOffsetOTP(this_client, FT_index, otp_offset);
	if(result < 0)
	{
		printk(KERN_INFO "%s: write OTP%d fail\n", __func__, FT_index);
		goto ic_err_misc;
	}
	
	buffer[0] = sensor_read_reg(this_client, STK831X_MODE);			
	buffer[1] = (buffer[0] & 0xF8);
	buffer[0] = STK831X_MODE;	
	result = STK_i2c_Tx(this_client, buffer, 2);
	if (result < 0) 
	{
		goto ic_err_i2c_rw;
	}	
	
	msleep(1);
	STK831x_Init(this_client);
#ifdef STK_DEBUG_CALI		
	//printk(KERN_INFO "\n%s:Check All OTP Data after write OTP and reset\n", __func__);
	//STK831x_ReadAllOTP();
#endif
		
	result = STK831X_VerifyCali(this_client, 1);
	if(result)
	{
		printk(KERN_ERR "%s: calibration check2 fail\n", __func__);
		goto ic_err_misc;
	}
	return 0;

ic_err_misc:
	STK831x_Init(this_client);	
	msleep(1);
	atomic_set(&cali_status, -result);	
	return result;
	
ic_err_i2c_rw:	
	printk(KERN_ERR "%s: i2c read/write error, err=0x%x\n", __func__, result);
	msleep(1);
	STK831x_Init(this_client);	
	atomic_set(&cali_status, STK_K_FAIL_I2C);	
	return result;	
}

#ifdef STK_TUNE
static int32_t stk_get_file_content(char * r_buf, int8_t buf_size)
{
	struct file  *cali_file;
	mm_segment_t fs;	
	ssize_t ret;
	
    cali_file = filp_open(STK_ACC_CALI_FILE, O_RDONLY,0);
    if(IS_ERR(cali_file))
	{
        DBG("%s: filp_open error, no offset file!\n", __func__);
        return -ENOENT;
	}
	else
	{
		fs = get_fs();
		set_fs(get_ds());
		ret = cali_file->f_op->read(cali_file,r_buf, STK_ACC_CALI_FILE_SIZE,&cali_file->f_pos);
		if(ret < 0)
		{
			DBG("%s: read error, ret=%d\n", __func__, ret);
			filp_close(cali_file,NULL);
			return -EIO;
		}		
		set_fs(fs);
    }
	
    filp_close(cali_file,NULL);	
	return 0;	
}

static void stk_handle_first_en(struct i2c_client *client)
{
	char r_buf[STK_ACC_CALI_FILE_SIZE] = {0};
	char offset[3];	
	char mode;

	if(stk_tune_offset_record[0]!=0 || stk_tune_offset_record[1]!=0 || stk_tune_offset_record[2]!=0)
	{
		STK831x_SetOffset(stk_tune_offset_record);
		stk_tune_done = 1;				
		atomic_set(&cali_status, STK_K_SUCCESS_TUNE);	
		DBG("%s: set offset:%d,%d,%d\n", __func__, stk_tune_offset_record[0], stk_tune_offset_record[1],stk_tune_offset_record[2]);	
	}		
	else if ((stk_get_file_content(r_buf, STK_ACC_CALI_FILE_SIZE)) == 0)
	{
		if(r_buf[0] == STK_ACC_CALI_VER0 && r_buf[1] == STK_ACC_CALI_VER1)
		{
			offset[0] = r_buf[2];
			offset[1] = r_buf[3];
			offset[2] = r_buf[4];
			mode = r_buf[5];
			STK831x_SetOffset(offset);
//#ifdef STK_TUNE			
			stk_tune_offset_record[0] = offset[0];
			stk_tune_offset_record[1] = offset[1];
			stk_tune_offset_record[2] = offset[2];
//#endif 			
			DBG("%s: set offset:%d,%d,%d, mode=%d\n", __func__, offset[0], offset[1], offset[2], mode);
			atomic_set(&cali_status, mode);								
		}
		else
		{
			DBG("%s: cali version number error! r_buf=0x%x,0x%x,0x%x,0x%x,0x%x\n", 
				__func__, r_buf[0], r_buf[1], r_buf[2], r_buf[3], r_buf[4]);						
			//return -EINVAL;
		}
	}
	else
	{
		offset[0] = offset[1] = offset[2] = 0;
		stk_store_in_file(offset, STK_K_NO_CALI);
		atomic_set(&cali_status, STK_K_NO_CALI);			
	}
	return;
}


static void STK831x_ResetPara(void)
{
	int ii;
	for(ii=0;ii<3;ii++)
	{
		stk_tune_sum[ii] = 0;
		stk_tune_min[ii] = 4096;
		stk_tune_max[ii] = -4096;
	}
	return;
}

static void STK831x_Tune(struct i2c_client *client, int acc[])
{	
	int ii;
	char offset[3];		
	char mode_reg;
	int result;
	char buffer[2] = "";
	
	if (stk_tune_done==0)
	{	
		if( event_since_en >= STK_TUNE_DELAY)
		{	
			if ((abs(acc[0]) <= STK_TUNE_XYOFFSET) && (abs(acc[1]) <= STK_TUNE_XYOFFSET)
				&& (abs(abs(acc[2])-STK_LSB_1G) <= STK_TUNE_ZOFFSET))				
				stk_tune_index++;
			else
				stk_tune_index = 0;

			if (stk_tune_index==0)			
				STK831x_ResetPara();			
			else
			{
				for(ii=0;ii<3;ii++)
				{
					stk_tune_sum[ii] += acc[ii];
					if(acc[ii] > stk_tune_max[ii])
						stk_tune_max[ii] = acc[ii];
					if(acc[ii] < stk_tune_min[ii])
						stk_tune_min[ii] = acc[ii];						
				}	
			}			

			if(stk_tune_index == STK_TUNE_NUM)
			{
				for(ii=0;ii<3;ii++)
				{
					if((stk_tune_max[ii] - stk_tune_min[ii]) > STK_TUNE_NOISE)
					{
						stk_tune_index = 0;
						STK831x_ResetPara();
						return;
					}
				}
				buffer[0] = sensor_read_reg(client, STK831X_MODE);

				mode_reg = buffer[0];
				buffer[1] = mode_reg & 0xF8;
				buffer[0] = STK831X_MODE;	
				result = STK_i2c_Tx(this_client, buffer, 2);
				if (result < 0) 
				{
					DBG("%s:failed, result=0x%x\n", __func__, result);			
					return;
				}				
				
				stk_tune_offset[0] = stk_tune_sum[0]/STK_TUNE_NUM;
				stk_tune_offset[1] = stk_tune_sum[1]/STK_TUNE_NUM;
				if (acc[2] > 0)
					stk_tune_offset[2] = stk_tune_sum[2]/STK_TUNE_NUM - STK_LSB_1G;
				else
					stk_tune_offset[2] = stk_tune_sum[2]/STK_TUNE_NUM - (-STK_LSB_1G);				
				
				offset[0] = (char) (-stk_tune_offset[0]);
				offset[1] = (char) (-stk_tune_offset[1]);
				offset[2] = (char) (-stk_tune_offset[2]);
				STK831x_SetOffset(offset);
				stk_tune_offset_record[0] = offset[0];
				stk_tune_offset_record[1] = offset[1];
				stk_tune_offset_record[2] = offset[2];
				
				buffer[1] = mode_reg | 0x1;
				buffer[0] = STK831X_MODE;	
				result = STK_i2c_Tx(this_client, buffer, 2);
				if (result < 0) 
				{
					DBG("%s:failed, result=0x%x\n", __func__, result);			
					return;
				}
				
				STK831x_SetVD(client);			
				stk_store_in_file(offset, STK_K_SUCCESS_TUNE);		
				stk_tune_done = 1;				
				atomic_set(&cali_status, STK_K_SUCCESS_TUNE);				
				event_since_en = 0;				
				DBG("%s:TUNE done, %d,%d,%d\n", __func__, 
					offset[0], offset[1],offset[2]);		
			}	
		}		
	}

	return;
}
#endif

static int show_all_reg(struct i2c_client *client)
{
	struct sensor_private_data *sensor =
		(struct sensor_private_data *) i2c_get_clientdata(client);	
	char reg[40] = {0};
	int i = 0;
	int ret;

	*reg = sensor->ops->read_reg;
	ret = sensor_rx_data(client, reg, 40);
	for(i=0;i<5;i++)
		DBG("%x, %x, %x, %x, %x, %x, %x, %x\n", reg[i*8+0], reg[i*8+1], reg[i*8+2], reg[i*8+3], reg[i*8+4], reg[i*8+5], reg[i*8+6], reg[i*8+7]);

	return 0;
}

static int STK831x_ReadByteOTP(char rReg, char *value)
{
	int redo = 0;
	int result;
	char buffer[2] = "";
	*value = 0;
	
	buffer[0] = 0x3D;
	buffer[1] = rReg;
	result = STK_i2c_Tx(this_client, buffer, 2);
	if (result < 0) 
	{
		printk(KERN_ERR "%s:failed\n", __func__);
		goto eng_i2c_r_err;
	}
	buffer[0] = 0x3F;
	buffer[1] = 0x02;
	result = STK_i2c_Tx(this_client, buffer, 2);
	if (result < 0) 
	{
		printk(KERN_ERR "%s:failed\n", __func__);
		goto eng_i2c_r_err;
	}
	
	//msleep(1);	
	do {
		msleep(2);
		buffer[0] = sensor_read_reg(this_client, 0x3F);	
		if(buffer[0]& 0x80)
		{
			break;
		}		
		redo++;
	}while(redo < 10);
	
	if(redo == 10)
	{
		printk(KERN_ERR "%s:OTP read repeat read 10 times! Failed!\n", __func__);
		return -STK_K_FAIL_OTP_5T;
	}	
	buffer[0] = sensor_read_reg(this_client, 0x3E);	
	*value = buffer[0];
#ifdef STK_DEBUG_CALI		
	printk(KERN_INFO "%s: read 0x%x=0x%x\n", __func__, rReg, *value);
#endif	
	return 0;
	
eng_i2c_r_err:	
	return -STK_K_FAIL_ENG_I2C;	
}

static int STK831x_WriteByteOTP(char wReg, char value)
{
	int finish_w_check = 0;
	int result;
	char buffer[2] = "";
	char read_back, value_xor = value;
	int re_write = 0;
	
	do
	{
		finish_w_check = 0;
		buffer[0] = 0x3D;
		buffer[1] = wReg;
		result = STK_i2c_Tx(this_client, buffer, 2);
		if (result < 0) 
		{
			printk(KERN_ERR "%s:failed, err=0x%x\n", __func__, result);
			goto eng_i2c_w_err;
		}
		buffer[0] = 0x3E;
		buffer[1] = value_xor;
		result = STK_i2c_Tx(this_client, buffer, 2);
		if (result < 0) 
		{
			printk(KERN_ERR "%s:failed, err=0x%x\n", __func__, result);
			goto eng_i2c_w_err;
		}				
		buffer[0] = 0x3F;
		buffer[1] = 0x01;
		result = STK_i2c_Tx(this_client, buffer, 2);
		if (result < 0) 
		{
			printk(KERN_ERR "%s:failed, err=0x%x\n", __func__, result);			
			goto eng_i2c_w_err;
		}				
		
		do 
		{
			msleep(1);
			buffer[0] = sensor_read_reg(this_client, 0x3F);	
			if(buffer[0]& 0x80)
			{
				result = STK831x_ReadByteOTP(wReg, &read_back);
				if(result < 0)
				{
					printk(KERN_ERR "%s: read back error, result=%d\n", __func__, result);
					goto eng_i2c_w_err;
				}
				
				if(read_back == value)				
				{
#ifdef STK_DEBUG_CALI					
					printk(KERN_INFO "%s: write 0x%x=0x%x successfully\n", __func__, wReg, value);
#endif			
					re_write = 0xFF;
					break;
				}
				else
				{
					printk(KERN_ERR "%s: write 0x%x=0x%x, read 0x%x=0x%x, try again\n", __func__, wReg, value_xor, wReg, read_back);
					value_xor = read_back ^ value;
					re_write++;
					break;
				}
			}
			finish_w_check++;		
		} while (finish_w_check < 5);
	} while(re_write < 10);
	
	if(re_write == 10)
	{
		printk(KERN_ERR "%s: write 0x%x fail, read=0x%x, write=0x%x, target=0x%x\n", __func__, wReg, read_back, value_xor, value);
		return -STK_K_FAIL_OTP_5T;
	}	
	
	return 0;

eng_i2c_w_err:	
	return -STK_K_FAIL_ENG_I2C;
}

static int STK831x_GetCali(struct i2c_client *client)
{
	char r_buf[STK_ACC_CALI_FILE_SIZE] = {0};
	char offset[3], mode;	
	int cnt, result;
	char regR[6];
	
#ifdef STK_TUNE		
	printk(KERN_INFO "%s: stk_tune_done=%d, stk_tune_index=%d, stk_tune_offset=%d,%d,%d\n", __func__, 
		stk_tune_done, stk_tune_index, stk_tune_offset_record[0], stk_tune_offset_record[1], 
		stk_tune_offset_record[2]);
#endif		
	if ((stk_get_file_content(r_buf, STK_ACC_CALI_FILE_SIZE)) == 0)
	{
		if(r_buf[0] == STK_ACC_CALI_VER0 && r_buf[1] == STK_ACC_CALI_VER1)
		{
			offset[0] = r_buf[2];
			offset[1] = r_buf[3];
			offset[2] = r_buf[4];
			mode = r_buf[5];
			printk(KERN_INFO "%s:file offset:%#02x,%#02x,%#02x,%#02x\n", 
				__func__, offset[0], offset[1], offset[2], mode);							
		}
		else
		{
			printk(KERN_ERR "%s: cali version number error! r_buf=0x%x,0x%x,0x%x,0x%x,0x%x\n", 
				__func__, r_buf[0], r_buf[1], r_buf[2], r_buf[3], r_buf[4]);						
		}
	}
	else
		printk(KERN_INFO "%s: No file offset\n", __func__);
		
	for(cnt=0x43;cnt<0x49;cnt++)
	{
		result = STK831x_ReadByteOTP(cnt, &(regR[cnt-0x43]));
		if(result < 0)
			printk(KERN_ERR "%s: STK831x_ReadByteOTP failed, ret=%d\n", __func__, result);		
	}
	printk(KERN_INFO "%s: OTP 0x43-0x49:%#02x,%#02x,%#02x,%#02x,%#02x,%#02x\n", __func__, regR[0], 
		regR[1], regR[2],regR[3], regR[4], regR[5]);
		
	for(cnt=0x53;cnt<0x59;cnt++)
	{
		result = STK831x_ReadByteOTP(cnt, &(regR[cnt-0x53]));
		if(result < 0)
			printk(KERN_ERR "%s: STK831x_ReadByteOTP failed, ret=%d\n", __func__, result);
	}
	printk(KERN_INFO "%s: OTP 0x53-0x59:%#02x,%#02x,%#02x,%#02x,%#02x,%#02x\n", __func__, regR[0], 
		regR[1], regR[2],regR[3], regR[4], regR[5]);
		
	return 0;
}

//SYSFS----------------------------------------------------
static ssize_t stk831x_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE,  "%d\n", getenable);
}

static ssize_t stk831x_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
	{
		printk(KERN_ERR "%s: strict_strtoul failed, error=0x%x\n", __func__, error);
		return error;
	}
	if ((data == 0)||(data==1)) 
		sensor_active(this_client, data, 0);	
	else
		printk(KERN_ERR "%s: invalud argument, data=%ld\n", __func__, data);
	return count;
}

static ssize_t stk831x_value_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ddata[3];

	printk(KERN_INFO "driver version:%s\n",STK_ACC_DRIVER_VERSION);	

	if(getenable)
	{
		ddata[0]= show_raw_data[0];
		ddata[1]= show_raw_data[1];
		ddata[2]= show_raw_data[2];
	}
	else
	{
		ddata[0]= 0;
		ddata[1]= 0;
		ddata[2]= 0;
	}
	return scnprintf(buf, PAGE_SIZE,  "%d %d %d\n", ddata[0], ddata[1], ddata[2]);
}

static ssize_t stk831x_cali_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int status = atomic_read(&cali_status);
	printk(KERN_INFO "%s: event_since_en_limit=%d\n",__func__,event_since_en_limit);
	
	if(status != STK_K_RUNNING)
		STK831x_GetCali(this_client);
	return scnprintf(buf, PAGE_SIZE,  "%02x\n", status);	
}

static ssize_t stk831x_cali_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	error = strict_strtoul(buf, 10, &data);
	if (error)
	{
		printk(KERN_ERR "%s: strict_strtoul failed, error=0x%x\n", __func__, error);
		return error;
	}
	printk("%s: data = %d\n", __func__, (int)data);
	STK831x_SetCali(this_client, data);
	return count;
}

static ssize_t stk831x_send_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int error, i;
	char *token[2];	
	int w_reg[2];
	char buffer[2] = "";
	
	for (i = 0; i < 2; i++)
		token[i] = strsep((char **)&buf, " ");
	if((error = strict_strtoul(token[0], 16, (unsigned long *)&(w_reg[0]))) < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, error=0x%x\n", __func__, error);
		return error;	
	}
	if((error = strict_strtoul(token[1], 16, (unsigned long *)&(w_reg[1]))) < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, error=0x%x\n", __func__, error);
		return error;	
	}
	printk(KERN_INFO "%s: reg[0x%x]=0x%x\n", __func__, w_reg[0], w_reg[1]);	
	buffer[0] = w_reg[0];
	buffer[1] = w_reg[1];
	error = STK_i2c_Tx(this_client, buffer, 2);
	if (error < 0) 
	{
		printk(KERN_ERR "%s:failed\n", __func__);
		return error;
	}		
	return count;
}

static ssize_t stk831x_recv_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE,  "%02x\n", recv_reg);	
}

static ssize_t stk831x_recv_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char buffer[2] = "";
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 16, &data);
	if (error)
	{
		printk(KERN_ERR "%s: strict_strtoul failed, error=0x%x\n", __func__, error);
		return error;
	}
	
	buffer[0] = sensor_read_reg(this_client, data);	
	recv_reg = buffer[0];
	printk(KERN_INFO "%s: reg[0x%x]=0x%x\n", __func__, (int)data , (int)buffer[0]);		
	return count;
}

static ssize_t stk831x_allreg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int error;	
	char buffer[16] = "";
	char show_buffer[11] = "";
	int aa,bb, no, show_no = 0;
	
	for(bb=0;bb<4;bb++)
	{
		buffer[0] = bb * 0x10;
		error = sensor_rx_data(this_client, buffer, 16);	
		if (error < 0) 
		{
			printk(KERN_ERR "%s:failed\n", __func__);
			return error;
		}
		for(aa=0;aa<16;aa++)
		{
			no = bb*0x10+aa;
			printk(KERN_INFO "stk reg[0x%x]=0x%x\n", no, buffer[aa]);
			switch(no)
			{
				case 0x0:	
				case 0x1:	
				case 0x2:	
				case 0x6:	
				case 0x7:	
				case 0x8:	
				case 0xC:	
				case 0xD:	
				case 0xE:	
				case 0x13:	
				case 0x24:	
				show_buffer[show_no] = buffer[aa];
				show_no++;
				break;
				default:
				break;
			}
		}
	}	
	return scnprintf(buf, PAGE_SIZE,  "0x0=%02x,0x1=%02x,0x2=%02x,0x6=%02x,0x7=%02x,0x8=%02x,0xC=%02x,0xD=%02x,0xE=%02x,0x13=%02x,0x24=%02x\n", 
			show_buffer[0], show_buffer[1], show_buffer[2], show_buffer[3], show_buffer[4], 
			show_buffer[5], show_buffer[6], show_buffer[7], show_buffer[8], show_buffer[9], 
			show_buffer[10]);
}

static ssize_t stk831x_sendo_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int error, i;
	char *token[2];	
	int w_reg[2];
	char buffer[2] = "";
	
	for (i = 0; i < 2; i++)
		token[i] = strsep((char **)&buf, " ");
	if((error = strict_strtoul(token[0], 16, (unsigned long *)&(w_reg[0]))) < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, error=0x%x\n", __func__, error);
		return error;	
	}
	if((error = strict_strtoul(token[1], 16, (unsigned long *)&(w_reg[1]))) < 0)
	{
		printk(KERN_ERR "%s:strict_strtoul failed, error=0x%x\n", __func__, error);
		return error;	
	}
	printk(KERN_INFO "%s: reg[0x%x]=0x%x\n", __func__, w_reg[0], w_reg[1]);	

	buffer[0] = w_reg[0];
	buffer[1] = w_reg[1];
	error = STK831x_WriteByteOTP(buffer[0], buffer[1]);
	if (error < 0) 
	{
		printk(KERN_ERR "%s:failed\n", __func__);
		return error;
	}		
	return count;
}


static ssize_t stk831x_recvo_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	char buffer[2] = "";
	unsigned long data;
	int error;
	
	error = strict_strtoul(buf, 16, &data);
	if (error)
	{
		printk(KERN_ERR "%s: strict_strtoul failed, error=0x%x\n", __func__, error);
		return error;
	}
	
	buffer[0] = data;
	error = STK831x_ReadByteOTP(buffer[0], &buffer[1]);	
	if (error < 0) 
	{
		printk(KERN_ERR "%s:failed\n", __func__);
		return error;
	}		
	printk(KERN_INFO "%s: reg[0x%x]=0x%x\n", __func__, buffer[0] , buffer[1]);		
	return count;
}

static ssize_t stk831x_firlen_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
#ifdef STK_LOWPASS
	int len = firlength;
	
	if(firlength)
	{
		printk(KERN_INFO "len = %2d, idx = %2d\n", fir.num, fir.idx);			
		printk(KERN_INFO "sum = [%5d %5d %5d]\n", fir.sum[0], fir.sum[1], fir.sum[2]);
		printk(KERN_INFO "avg = [%5d %5d %5d]\n", fir.sum[0]/len, fir.sum[1]/len, fir.sum[2]/len);
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", firlength);
#else
	return snprintf(buf, PAGE_SIZE, "not support\n");
#endif	
}

static ssize_t stk831x_firlen_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
#ifdef STK_LOWPASS
	struct stk831x_data *stk = i2c_get_clientdata(this_client);	
	int error;
	unsigned long data;
	
	error = strict_strtoul(buf, 10, &data);
	if (error)
	{
		printk(KERN_ERR "%s: strict_strtoul failed, error=%d\n", __func__, error);
		return error;
	}			
	
	if(data > MAX_FIR_LEN)
	{
		printk(KERN_ERR "%s: firlen exceed maximum filter length\n", __func__);
	}
	else if (data < 1)
	{
		firlength = 1;
		fir_en = 0;
		memset(&fir, 0x00, sizeof(fir));
	}
	else
	{ 
		firlength = data;
		memset(&fir, 0x00, sizeof(fir));
		fir_en = 1;
	}
#else
	printk(KERN_ERR "%s: firlen is not supported\n", __func__);
#endif    
	return count;	
}

static DEVICE_ATTR(enable, 0666, stk831x_enable_show, stk831x_enable_store);
static DEVICE_ATTR(value, 0444, stk831x_value_show, NULL);
static DEVICE_ATTR(cali, 0666, stk831x_cali_show, stk831x_cali_store);
static DEVICE_ATTR(send, 0222, NULL, stk831x_send_store);
static DEVICE_ATTR(recv, 0666, stk831x_recv_show, stk831x_recv_store);
static DEVICE_ATTR(allreg, 0444, stk831x_allreg_show, NULL);
static DEVICE_ATTR(sendo, 0222, NULL, stk831x_sendo_store);
static DEVICE_ATTR(recvo, 0222, NULL, stk831x_recvo_store);
static DEVICE_ATTR(firlen, 0666, stk831x_firlen_show, stk831x_firlen_store);

static struct attribute *stk831x_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_value.attr,
	&dev_attr_cali.attr,
	&dev_attr_send.attr,
	&dev_attr_recv.attr,
	&dev_attr_allreg.attr,
	&dev_attr_sendo.attr,
	&dev_attr_recvo.attr,
	&dev_attr_firlen.attr,
	NULL
};

static struct attribute_group stk831x_attribute_group = {
	//.name = "driver",
	.attrs = stk831x_attributes,
};
//SYSFS----------------------------------------------------

/****************operate according to sensor chip:start************/

static int sensor_active(struct i2c_client *client, int enable, int rate)
{
	struct sensor_private_data *sensor =
	    (struct sensor_private_data *) i2c_get_clientdata(client);	
	int result = 0;
	int status = 0;

	int k_status = atomic_read(&cali_status);
	getenable = enable;
	printk("%s enable = %d\n", __func__, enable);
	
#ifdef STK_TUNE
	if(first_enable)
		stk_handle_first_en(client);
		
#endif
		
	sensor->ops->ctrl_data = sensor_read_reg(client, sensor->ops->ctrl_reg);
	
	//register setting according to chip datasheet		
	if(enable)
	{	
		status = STK831X_ENABLE;	//stk831x
		sensor->ops->ctrl_data |= status;	
	}
	else
	{
		status = ~STK831X_ENABLE;	//stk831x
		sensor->ops->ctrl_data &= status;
	}

	printk("%s:reg=0x%x,reg_ctrl=0x%x,enable=%d\n",__func__,sensor->ops->ctrl_reg, sensor->ops->ctrl_data, enable);
	result = sensor_write_reg(client, sensor->ops->ctrl_reg, sensor->ops->ctrl_data);
	if(result)
		printk("%s:fail to active sensor\n",__func__);

	if(first_enable && k_status != STK_K_RUNNING)
	{
		first_enable = false;	
		msleep(2);
		result = stk_get_ic_content(client);			
	}
	
	if(enable)
	{	
			event_since_en = 0;

		result = STK831x_SetVD(client);
		if(result)
			printk("%s:fail to SetVD\n",__func__);
		else
			printk("%s:SetVD successfully\n",__func__);
			//event_since_en = 0;

#if defined(STK_LOWPASS)
		fir.num = 0;
		fir.idx = 0;
		fir.sum[0] = 0;
		fir.sum[1] = 0;
		fir.sum[2] = 0;
#endif

		//===========================printk reg==============
		show_all_reg(client);
	
    }
   return result;
}

static int STK831x_Init(struct i2c_client *client)
{
	char buffer[2];
    //    struct sensor_private_data *sensor =
    //        (struct sensor_private_data *) i2c_get_clientdata(client);
        int result = 0;
		int ret = 0;

	//printk("%s\n", __func__);

	this_client = client;
	
       /* result = sensor->ops->active(client,0,0);
        if(result)
        {
                printk("%s:line=%d,error\n",__func__,__LINE__);
                return result;
        }*/
/*
	printk("%s: driver version:%s\n", __func__,STK_ACC_DRIVER_VERSION);
#ifdef CONFIG_SENSORS_STK8312
	printk("%s: Initialize stk8312\n", __func__);
#elif defined CONFIG_SENSORS_STK8313
	printk("%s: Initialize stk8313\n", __func__);
#endif		
*/	
	buffer[0] = STK831X_RESET;
	buffer[1] = 0x00;
	result = STK_i2c_Tx(client, buffer, 2);
	if (result < 0) 
	{
		printk("%s:failed\n", __func__);
		return result;
	}		

	buffer[0] = STK831X_MODE;
	buffer[1] = 0xC0;//0xC0;
	result = STK_i2c_Tx(client, buffer, 2);
	if (result < 0) 
	{
		printk("%s:failed\n", __func__);
		return result;
	}		
	
	buffer[0] = STK831X_SR;
	buffer[1] = STK831X_INIT_ODR;	
	result = STK_i2c_Tx(client, buffer, 2);
	if (result < 0) 
	{
		printk("%s:failed\n", __func__);
		return result;
	}	

#if (!STK_ACC_POLLING_MODE)
	buffer[0] = STK831X_INTSU;
	buffer[1] = 0x10;
	result = STK_i2c_Tx(client, buffer, 2);
	if (result < 0) 
	{
		printk("%s:interrupt init failed\n", __func__);
		return result;
	}	
#endif 

	buffer[0] = STK831X_STH;
#ifdef CONFIG_SENSORS_STK8312	
	buffer[1] = 0x42;
#elif defined CONFIG_SENSORS_STK8313
	buffer[1] = 0x82;
#endif	
	result = STK_i2c_Tx(client, buffer, 2);
	if (result < 0) 
	{
		printk("%s:set range failed\n", __func__);	
		return result;
	}	

#ifdef STK_TUNE
	first_enable = true;
#endif
	event_since_en = 0;

#ifdef STK_LOWPASS
		memset(&fir, 0x00, sizeof(fir));	
		fir_en = 1;
#endif
	
    /* Sys Attribute Register */
    if(noCreateAttr)
    {
        struct input_dev* pInputDev;
        pInputDev = input_allocate_device();
		if (!pInputDev) {
			dev_err(&client->dev,
				"Failed to allocate input device %s\n", pInputDev->name);
			return -ENOMEM;	
		}

		pInputDev->name = "stk831x_attr";
		set_bit(EV_ABS, pInputDev->evbit);	
			
		ret = input_register_device(pInputDev);
		if (ret) {
			dev_err(&client->dev,
				"Unable to register input device %s\n", pInputDev->name);
			return -ENOMEM;	
		}	
		
		DBG("Sys Attribute Register here %s is called for DA311.\n", __func__);
		ret = sysfs_create_group(&pInputDev->dev.kobj, &stk831x_attribute_group);
		if (ret) {
			DBG("stk831x sysfs_create_group Error err=%d..", ret);
			ret = -EINVAL;
		}
		noCreateAttr = 0;
    }

	return 0;
}

/*
static int sensor_convert_data(struct i2c_client *client, char high_byte, char low_byte)
{
    s64 result;
	//struct sensor_private_data *sensor =
	//    (struct sensor_private_data *) i2c_get_clientdata(client);	
	//int precision = sensor->ops->precision;
		
	result = (int)low_byte;
	if(result==255||result==254||result==1||result==2)//by ldp 20130426
		result=0;

	if (result < STK831X_BOUNDARY)
		result = result* STK831X_GRAVITY_STEP;
	else
		result = ~( ((~result & (0x7fff>>(16-STK831X_PRECISION)) ) + 1) 
	   			* STK831X_GRAVITY_STEP) + 1;    

    	return (int)result;
}
*/

static int gsensor_report_value(struct i2c_client *client, struct sensor_axis *axis)
{
	struct sensor_private_data *sensor =
	    (struct sensor_private_data *) i2c_get_clientdata(client);	

	if(event_since_en < 1200)
		event_since_en++;	
	
	if(event_since_en < event_since_en_limit)
		return 0;		
	
	/* Report acceleration sensor information */
	input_report_abs(sensor->input_dev, ABS_X, axis->x);
	input_report_abs(sensor->input_dev, ABS_Y, axis->y);
	input_report_abs(sensor->input_dev, ABS_Z, axis->z);
	input_sync(sensor->input_dev);
	DBG("Gsensor x==%d  y==%d z==%d\n",axis->x,axis->y,axis->z);

	return 0;
}

#ifdef CONFIG_SENSORS_STK8312
static int STK831x_CheckReading(int acc[], bool clear)
{	
	static int check_result = 0;
	
	if(acc[0] == 127 || acc[0] == -128 || acc[1] == 127 || acc[1] == -128 || 
			acc[2] == 127 || acc[2] == -128)
	{
		printk(KERN_INFO "%s: acc:%o,%o,%o\n", __func__, acc[0], acc[1], acc[2]);
		check_result++;		
	}	
	if(clear)
	{
		if(check_result == 3)
		{
			if(acc[0] != 127 && acc[0] != -128 && acc[1] != 127 && acc[1] != -128)
			{
				event_since_en_limit = 21;
			}
			else
			{
				event_since_en_limit = 10000;
			}
			printk(KERN_INFO "%s: incorrect reading\n", __func__);		
			check_result = 0;
			return 1;
		}
		check_result = 0;
	}
	return 0;		
}
#else
static int STK831x_CheckReading(int acc[], bool clear)
{
	static int check_result = 0;
	
	if(acc[0] == 2047 || acc[0] == -2048 || acc[1] == 2047 || acc[1] == -2048 || 
	acc[2] == 2047 || acc[2] == -2048)
	{
		printk(KERN_INFO "%s: acc:%o,%o,%o\n", __func__, acc[0], acc[1], acc[2]);
		check_result++;		
	}	
	if(clear)
	{
		if(check_result == 3)
		{
			event_since_en_limit = 10000;
			printk(KERN_INFO "%s: incorrect reading\n", __func__);		
			check_result = 0;
			return 1;
		}
		check_result = 0;
	}
	return 0;
}
#endif	

#define GSENSOR_MIN  		2
static int sensor_report_value(struct i2c_client *client)
{
	struct sensor_private_data *sensor =
		(struct sensor_private_data *) i2c_get_clientdata(client);	
	struct sensor_platform_data *pdata = sensor->pdata;
	int ret = 0;
	int x,y,z;
	struct sensor_axis axis;
#ifdef CONFIG_SENSORS_STK8312
	char buffer[3] = {0};	
#else
	char buffer[6] = {0};
#endif
	int acc_xyz[3] = {0};
	int k_status = 0;
	k_status = atomic_read(&cali_status);
#ifdef STK_LOWPASS
	int idx = 0;	 
#endif

#ifdef STK_ZG_FILTER	
	s16 zero_fir = 0;	
#endif	
	
//	if(sensor->ops->read_len < 3)	//sensor->ops->read_len = 3
//	{
//		DBG("%s:lenth is error,len=%d\n",__func__,sensor->ops->read_len);
//		return -1;
//	}
#if 1 //add by ruan
	if( suspend_flag )
		return 0;
	is_reading_reg = 1;
#endif

#ifdef CONFIG_SENSORS_STK8312	
	memset(buffer, 0, 3);
#else
	memset(buffer, 0, 6);
#endif
	
	/* Data bytes from hardware xL, xH, yL, yH, zL, zH */	
	do {
		*buffer = sensor->ops->read_reg;
		ret = sensor_rx_data(client, buffer, sensor->ops->read_len);
		if (ret < 0)
		return ret;
	} while (0);

//	DBG("data:   %d    %d    %d\n", buffer[0], buffer[1], buffer[2]);

#ifdef CONFIG_SENSORS_STK8312
	if (buffer[0] & 0x80)
		acc_xyz[0] = buffer[0] - 256;
	else
		acc_xyz[0] = buffer[0];
	if (buffer[1] & 0x80)
		acc_xyz[1] = buffer[1] - 256;
	else
		acc_xyz[1] = buffer[1];
	if (buffer[2] & 0x80)
		acc_xyz[2] = buffer[2] - 256;
	else
		acc_xyz[2] = buffer[2];
#else
	if (buffer[0] & 0x80)
		acc_xyz[0] = ((int)buffer[0]<<4) + (buffer[1]>>4) - 4096;
	else
		acc_xyz[0] = ((int)buffer[0]<<4) + (buffer[1]>>4);
	if (buffer[2] & 0x80)
		acc_xyz[1] = ((int)buffer[2]<<4) + (buffer[3]>>4) - 4096;
	else
		acc_xyz[1] = ((int)buffer[2]<<4) + (buffer[3]>>4);
	if (buffer[4] & 0x80)
		acc_xyz[2] = ((int)buffer[4]<<4) + (buffer[5]>>4) - 4096;
	else
		acc_xyz[2] = ((int)buffer[4]<<4) + (buffer[5]>>4);
#endif
		
	if(event_since_en == 16 || event_since_en == 17)
		STK831x_CheckReading(acc_xyz, false);
	else if(event_since_en == 18)
		STK831x_CheckReading(acc_xyz, true);		
		
	if(event_since_en_limit == 21)
	{
		acc_xyz[2] = getcode((21*21) - (acc_xyz[0]*acc_xyz[0]) - (acc_xyz[1]*acc_xyz[1]));
	}
		
	if(k_status == STK_K_RUNNING)
	{
		cali_raw_data[0] = acc_xyz[0];
		cali_raw_data[1] = acc_xyz[1];
		cali_raw_data[2] = acc_xyz[2];	
		return 0;
	}	
		
#ifdef STK_TUNE
	if((k_status&0xF0) != 0)
		STK831x_Tune(client, acc_xyz);		
#endif	

#ifdef STK_LOWPASS
	if(fir_en)
	{
		if(fir.num < firlength)
		{				 
			fir.raw[fir.num][0] = acc_xyz[0];
			fir.raw[fir.num][1] = acc_xyz[1];
			fir.raw[fir.num][2] = acc_xyz[2];
			fir.sum[0] += acc_xyz[0];
			fir.sum[1] += acc_xyz[1];
			fir.sum[2] += acc_xyz[2];
			fir.num++;
			fir.idx++;
		}
		else
		{
			idx = fir.idx % firlength;
			fir.sum[0] -= fir.raw[idx][0];
			fir.sum[1] -= fir.raw[idx][1];
			fir.sum[2] -= fir.raw[idx][2];
			fir.raw[idx][0] = acc_xyz[0];
			fir.raw[idx][1] = acc_xyz[1];
			fir.raw[idx][2] = acc_xyz[2];
			fir.sum[0] += acc_xyz[0];
			fir.sum[1] += acc_xyz[1];
			fir.sum[2] += acc_xyz[2];
			fir.idx++; 
			acc_xyz[0] = fir.sum[0]/firlength;
			acc_xyz[1] = fir.sum[1]/firlength;
			acc_xyz[2] = fir.sum[2]/firlength;				
		}
	}
#ifdef STK_DEBUG_RAWDATA
	DBG("%s:After FIR	%4d,%4d,%4d\n", __func__, acc_xyz[0], acc_xyz[1], acc_xyz[2]);	
#endif

#endif		/* #ifdef STK_LOWPASS */

	show_raw_data[0] = acc_xyz[0];
	show_raw_data[1] = acc_xyz[1];
	show_raw_data[2] = acc_xyz[2];
	
#ifdef STK_ZG_FILTER
	if( abs(acc_xyz[0]) <= STK_ZG_COUNT)	
		acc_xyz[0] = (acc_xyz[0]*zero_fir);	
	if( abs(acc_xyz[1]) <= STK_ZG_COUNT)
		acc_xyz[1] = (acc_xyz[1]*zero_fir);
	if( abs(acc_xyz[2]) <= STK_ZG_COUNT)
		acc_xyz[2] = (acc_xyz[2]*zero_fir);
#endif 	/* #ifdef STK_ZG_FILTER */	

	
	DBG("acc :   %d    %d    %d\n", acc_xyz[0], acc_xyz[1], acc_xyz[2]);
	x = acc_xyz[0] * STK831X_GRAVITY_STEP;
	y = acc_xyz[1] * STK831X_GRAVITY_STEP;
	z = acc_xyz[2] * STK831X_GRAVITY_STEP;

	axis.x = (pdata->orientation[0])*x + (pdata->orientation[1])*y + (pdata->orientation[2])*z;
	axis.y = (pdata->orientation[3])*x + (pdata->orientation[4])*y + (pdata->orientation[5])*z; 
	axis.z = (pdata->orientation[6])*x + (pdata->orientation[7])*y + (pdata->orientation[8])*z;
	
//	if(axis.z<0)
//		axis.z = -axis.z;

       gsensor_report_value(client, &axis);

       mutex_lock(&(sensor->data_mutex) );
       sensor->axis = axis;
       mutex_unlock(&(sensor->data_mutex) );

//===========================printk reg==============
	//show_all_reg(client);
//================================================
	is_reading_reg = 0;
	return ret;
}

void stk8312_suspend()
{
	//sensor_active(this_client, 0, 0);	
	char buffer[2];
	struct i2c_client *client;
    //    struct sensor_private_data *sensor =
    //        (struct sensor_private_data *) i2c_get_clientdata(client);
        int result = 0;
		int ret = 0;

#if 1 // add by ruan
	suspend_flag = 1;
	while(is_reading_reg){
		mdelay(50);
	}
#endif
	printk(KERN_ERR "###### %s >>>>>>>>>>>> \n",__func__);

	client = this_client;
	
	buffer[0] = STK831X_RESET;
	buffer[1] = 0x00;
	result = STK_i2c_Tx(client, buffer, 2);
	if (result < 0) 
	{
		printk("%s:#######3 failed\n", __func__);
	}
	return 0;
}
void stk8312_resume()
{
	printk(KERN_ERR "###### %s >>>>>>>>>>>> \n",__func__);
	suspend_flag = 0;
	//sensor_active(this_client, 1, 0);	
	STK831x_Init(this_client);
}

#ifdef CONFIG_SENSORS_STK8312
struct sensor_operate gsensor_stk831x_ops = {
	.name				= "gs_stk831x",
	.type				= SENSOR_TYPE_ACCEL,			//sensor type and it should be correct
	.id_i2c				= ACCEL_ID_STK831X,			//i2c id number
	.read_reg			= STK831X_REG_X_OUT,			//read data
	.read_len			= 3,					//data length
	.id_reg				= SENSOR_UNKNOW_DATA,			//read device id from this register
	.id_data 			= SENSOR_UNKNOW_DATA,			//device id
	.precision			= STK831X_PRECISION,			//8 bit
	.ctrl_reg 			= STK831X_REG_MODE,			//enable or disable 	
	.int_status_reg 		= SENSOR_UNKNOW_DATA,			//intterupt status register
	.range				= {-STK831X_RANGE,STK831X_RANGE},	//range
	//.trig				= IRQF_TRIGGER_RISING,	
	.trig				= IRQF_TRIGGER_LOW|IRQF_ONESHOT,
	.active				= sensor_active,	
	.init				= STK831x_Init,//sensor_init,
	.report 			= sensor_report_value,
	//.suspend			= stk8312_suspend,
	//.resume				= stk8312_resume,
};
#else
	struct sensor_operate gsensor_stk831x_ops = {
		.name				= "gs_stk8313",
		.type				= SENSOR_TYPE_ACCEL,			//sensor type and it should be correct
		.id_i2c				= ACCEL_ID_STK8313,			//i2c id number
		.read_reg			= STK831X_REG_X_OUT,			//read data
		.read_len			= 6,					//data length
		.id_reg				= SENSOR_UNKNOW_DATA,			//read device id from this register
		.id_data 			= SENSOR_UNKNOW_DATA,			//device id
		.precision			= STK831X_PRECISION,			//12 bit
		.ctrl_reg 			= STK831X_REG_MODE,			//enable or disable 	
		.int_status_reg 		= SENSOR_UNKNOW_DATA,			//intterupt status register
		.range				= {-STK831X_RANGE,STK831X_RANGE},	//range
		//.trig				= IRQF_TRIGGER_RISING,	
		.trig				= IRQF_TRIGGER_LOW|IRQF_ONESHOT,
		.active				= sensor_active,	
		.init				= STK831x_Init,//sensor_init,
		.report 			= sensor_report_value,
	};
	
#endif
/****************operate according to sensor chip:end************/

//function name should not be changed
static struct sensor_operate *gsensor_get_ops(void)
{
	return &gsensor_stk831x_ops;
}


static int __init gsensor_stk831x_init(void)
{
	struct sensor_operate *ops = gsensor_get_ops();
	int result = 0;
	int type = ops->type;
	int odr = STK831X_INIT_ODR;
	if(odr == 1)
		getdelay = 5;
	else if(odr == 2)
		getdelay = 10;
	else if(odr == 3)
		getdelay = 20;
	else
		getdelay = 40;
	
	event_since_en_limit = 20;
	result = sensor_register_slave(type, NULL, NULL, gsensor_get_ops);	
	
#ifdef STK_PERMISSION_THREAD
	STKPermissionThread = kthread_run(stk_permission_thread,"stk","Permissionthread");
	if(IS_ERR(STKPermissionThread))
		STKPermissionThread = NULL;
#endif // STK_PERMISSION_THREAD	

	return result;
}

static void __exit gsensor_stk831x_exit(void)
{
	struct sensor_operate *ops = gsensor_get_ops();
	int type = ops->type;
	sensor_unregister_slave(type, NULL, NULL, gsensor_get_ops);
	
#ifdef STK_PERMISSION_THREAD
	if(STKPermissionThread)
		STKPermissionThread = NULL;
#endif // STK_PERMISSION_THREAD	
}


module_init(gsensor_stk831x_init);
module_exit(gsensor_stk831x_exit);

