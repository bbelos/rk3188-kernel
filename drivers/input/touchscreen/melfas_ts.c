/* drivers/input/touchscreen/melfas_ts.c
 *
 * Copyright (C) 2010 Melfas, Inc.
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

#include <linux/module.h>
#include <linux/delay.h>
//#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/melfas_ts.h>

#include <linux/firmware.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>

#include <linux/input/mt.h> // slot

#include <linux/miscdevice.h>
#include <asm/uaccess.h>

#include "bin_isc.h"
#include "bin_tsp.c"

//_IOR(type, nr, size)

#define PSENSOR_IOCTL_MAGIC 'c'
#define PSENSOR_IOCTL_GET_ENABLED _IOR(PSENSOR_IOCTL_MAGIC, 1, int *)
#define PSENSOR_IOCTL_ENABLE _IOW(PSENSOR_IOCTL_MAGIC, 2, int *)


#define MCS6K                       1
#define MMS100A                     2
#define MMS100S                     3

#define MCSCHIP_ID                   MMS100A

#define BASEBAND_MTK_SUPPORT  0

#if BASEBAND_MTK_SUPPORT
#define BURST_PACKET_SIZE     8
#endif



#define SLOT_TYPE	1 // touch event type choise

#define I2C_RETRY_CNT 5 //Fixed value
#define DOWNLOAD_RETRY_CNT 5 //Fixed value
#define MELFAS_DOWNLOAD 1 //Fixed value
#define ISC_DOWNLOAD_ENABLE 0
#define NO_CHECK_VERSION_DBG 1

#define TS_READ_LEN_ADDR 0x0F //Fixed value
#define TS_READ_START_ADDR 0x10 //Fixed value
#define TS_READ_REGS_LEN 66 //Fixed value
#define TS_WRITE_REGS_LEN 16 //Fixed value


#define TS_MAX_TOUCH 	5 //Model Dependent
#define TS_READ_HW_VER_ADDR 0xC1 //Model Dependent
#define TS_READ_SW_VER_ADDR 0xC3 //Model Dependent



#define MELFAS_HW_REVISON 0x01 //Model Dependent
#define MELFAS_FW_VERSION 0x04 //Model Dependent

#define DEBUG_PRINT 0 //Model Dependent

#if SLOT_TYPE
#define REPORT_MT(touch_number, x, y, area, pressure) \
do {     \
	input_mt_slot(ts->input_dev, touch_number);	\
	input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);	\
	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, 1024-y);             \
	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, x);             \
	input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 1);         \
	/*input_report_abs(ts->input_dev, ABS_MT_PRESSURE, pressure); */\
} while (0)
#else
#define REPORT_MT(touch_number, x, y, area, pressure) \
do {     \
	input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, touch_number);\
	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);             \
	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);             \
	input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, area);         \
	input_report_abs(ts->input_dev, ABS_MT_PRESSURE, pressure); \
	input_mt_sync(ts->input_dev);                                      \
} while (0)
#endif


static struct muti_touch_info g_Mtouch_info[TS_MAX_TOUCH];

static int ps_opened=0;
static int ps_active=0;


#ifdef CONFIG_HAS_EARLYSUSPEND
static void melfas_ts_early_suspend(struct early_suspend *h);
static void melfas_ts_late_resume(struct early_suspend *h);
#endif

#if ( MCSCHIP_ID == MMS100A )
extern int isc_fw_download(struct melfas_ts_data *info, const u8 *data, size_t len);
extern int isp_fw_download(const u8 *data, size_t len);
#elif ( MCSCHIP_ID == MMS100S )
extern int mms100S_download();
#endif

static int melfas_i2c_write(struct i2c_client *client, char *buf, int length);

static int melfas_i2c_read(struct i2c_client *client, u16 addr, u16 length, u8 *value)
{
    struct i2c_adapter *adapter = client->adapter;
    struct i2c_msg msg;
    int ret = -1;

	char buf = addr; 

	melfas_i2c_write(client,&buf,1);

	return i2c_master_recv(client, value, length);
#if 0
    msg.addr = client->addr;
    msg.flags = 0x00;
    msg.len = 1;
    msg.buf = (u8 *) & addr;
    
    
    msg.scl_rate=100000;
    msg.udelay=100;

    ret = i2c_transfer(adapter, &msg, 1);

    if (ret >= 0)
    {
        msg.addr = client->addr;
        msg.flags = I2C_M_RD;
        msg.len = length;
        msg.buf = (u8 *) value;

        ret = i2c_transfer(adapter, &msg, 1);
    }

    if (ret < 0)
    {
        pr_err("[TSP] : read error : [%d]", ret);
    }

    return ret;
#endif
}

static int melfas_i2c_write(struct i2c_client *client, char *buf, int length)
{
    int i;
    char data[TS_WRITE_REGS_LEN];

    if (length > TS_WRITE_REGS_LEN)
    {
        pr_err("[TSP] %s :size error \n", __FUNCTION__);
        return -EINVAL;
    }

    for (i = 0; i < length; i++)
        data[i] = *buf++;

    i = i2c_master_send(client, (char *) data, length);

    if (i == length)
        return length;
    else
    {
        pr_err("[TSP] :write error : [%d]", i);
        return -EIO;
    }
}

#if BASEBAND_MTK_SUPPORT
static int i2c_read_for_mtk(struct i2c_client *client, u16 addr, u16 length, u8 *value)
{
    uint16_t StartIdx, Remains, ReadBytes;
    int ret = -1;

    StartIdx = 0; Remains = length; ReadBytes = 0;

    do{
        ReadBytes = ( Remains > BURST_PACKET_SIZE ) ? BURST_PACKET_SIZE : Remains;

        ret = melfas_i2c_read( client, (addr + StartIdx), ReadBytes, (u8 *)(value + StartIdx) );

        if( ret < 0 )
        {
            pr_info("[TSP] : i2c_read Fail addr %d length \n", (addr + StartIdx, ReadBytes);
            break; // i2c fail
        }
        
        StartIdx += ReadBytes;
        Remains -= ReadBytes;
    } while( Remains > 0 );

    return ret;
}
#endif

#if SLOT_TYPE
static void melfas_ts_release_all_finger(struct melfas_ts_data *ts)
{
#if DEBUG_PRINT
    pr_info("[TSP] %s\n", __func__);
#endif
	int i;
	for (i = 0; i < TS_MAX_TOUCH; i++)
	{
		input_mt_slot(ts->input_dev, i);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
	}
	input_sync(ts->input_dev);
}
#else	
static void melfas_ts_release_all_finger(struct melfas_ts_data *ts)
{
#if DEBUG_PRINT
    pr_info("[TSP] %s\n", __func__);
#endif

	int i;
	for(i=0; i<TS_MAX_TOUCH; i++)
	{
		if(-1 == g_Mtouch_info[i].pressure)
			continue;

		if(g_Mtouch_info[i].pressure == 0)
			input_mt_sync(ts->input_dev);

		if(0 == g_Mtouch_info[i].pressure)
			g_Mtouch_info[i].pressure = -1;
	}
	input_sync(ts->input_dev);
}
#endif

static int check_firmware(struct melfas_ts_data *ts, u8 *val)
{
    int ret = 0;
    uint8_t i = 0;
    uint8_t buf[10] = {0};
    #if 0
    ret = melfas_i2c_read(ts->client, 0xf1, 5, &buf[0]);
    for (i=0; i<5; i++)
    {
    	pr_info("REG 0xF%d=0x%02X \n", i+1, buf[i]);
    }
    
    ret = melfas_i2c_read(ts->client, 0xc1, 5, &buf[0]);
    for (i=0; i<5; i++)
    {
    	pr_info("REG 0xC%d=0x%02X \n", i+1, buf[i]);
    }
    #endif
    for (i = 0; i < I2C_RETRY_CNT; i++)
    {
        ret = melfas_i2c_read(ts->client, 0xc1, 1, &val[0]);
        ret = melfas_i2c_read(ts->client, 0xc3, 1, &val[1]);

        if (ret >= 0)
        {
            pr_info("[TSP] : HW Revision[0x%02x] SW Version[0x%02x] \n", val[0], val[1]);
            break; // i2c success
        }
    }

    if (ret < 0)
    {
        pr_info("[TSP] %s,%d: i2c read fail[%d] \n", __FUNCTION__, __LINE__, ret);
        return ret;
    }
}


#if ( MCSCHIP_ID == MMS100A )
#ifdef DOWNLOAD_FIRMWARE_BIN
#include "stdio.h"
static int check_firmware_bin_download(void)
{
	//==================================================
	//
	//	1. Read '.bin file'
	//   2. *pBinary       : Binary data
	//	   nBinary_length : Firmware size
    //   3. Run mms100_ISC_download(*pBianry,unLength, nMode)
    //       nMode : 0 (Core download)
    //       nMode : 1 (Private Custom download)
    //       nMode : 2 (Public Custom download)
	//
	//==================================================

	// TO DO : File Process & Get file Size(== Binary size)
	//			This is just a simple sample

	//FILE *fp;
	INT  nRead;
	int ret = 1;

	//------------------------------
	// Open a file
	//------------------------------

    long fd = sys_open("MELFAS_FIRMWARE.bin", O_RDONLY,0777); 
    
	if( fd < 0 ){
		printk("Con't get melfas firmware!\r\n");
		return MCSDL_RET_FILE_ACCESS_FAILED;
	}
	else
	{
		printk("Get melfas firmware!\r\n");
	}

	//------------------------------
	// Get Binary Size
	//------------------------------

	fseek( fp, 0, SEEK_END );

	nBinary_length = (UINT16)ftell(fp);

	//------------------------------
	// Memory allocation
	//------------------------------

	pBinary = (UINT8*)malloc( (INT)nBinary_length );

	if( pBinary == NULL ){

		return MCSDL_RET_FILE_ACCESS_FAILED;
	}

	//------------------------------
	// Read binary file
	//------------------------------

	fseek( fp, 0, SEEK_SET );

	nRead = fread( pBinary, 1, (INT)nBinary_length, fp );		// Read binary file

	if( nRead != (INT)nBinary_length ){

		fclose(fp);												// Close file

		if( pBinary != NULL )										// free memory alloced.
			free(pBinary);

		return MCSDL_RET_FILE_ACCESS_FAILED;
	}

	//------------------------------
	// Close file
	//------------------------------

	fclose(fp);
	
	ret = 0;
	//return 0

MCSDL_RET_FILE_ACCESS_FAILED:

	return ret;

}
#endif

static int firmware_update(struct melfas_ts_data *ts)
{
    int ret = 0;
    uint8_t fw_ver[2] = {0, };

    struct melfas_ts_data *info = ts;
    struct i2c_client *client = info->client;
    struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);


    ret = check_firmware(ts, fw_ver);
    if (ret < 0)
    {
        printk("[TSP] check_firmware fail! [%d]", ret);
		ret = isp_fw_download(MELFAS_binary, MELFAS_binary_nLength);
		if (ret != 0)
		{
			printk("[TSP] error updating firmware to version 0x%02x \n", MELFAS_FW_VERSION);
			ret = -1;
		}
    }
    else
    {
#if MELFAS_DOWNLOAD

#if NO_CHECK_VERSION_DBG == 1
        if (fw_ver[1] != MELFAS_FW_VERSION)
#endif
        {
            int ver;

            printk("[TSP] %s: \n", __func__);
            #if (ISC_DOWNLOAD_ENABLE == 1)
            ret = isc_fw_download(info, MELFAS_binary, MELFAS_binary_nLength);
            if (ret < 0)
            #endif
            {
                ret = isp_fw_download(MELFAS_binary, MELFAS_binary_nLength);
                if (ret != 0)
                {
                    printk("[TSP] error updating firmware to version 0x%02x \n", MELFAS_FW_VERSION);
                    ret = -1;
                }
                else
                {
            		#if (ISC_DOWNLOAD_ENABLE == 1)
                    ret = isc_fw_download(info, MELFAS_binary, MELFAS_binary_nLength);
                    #endif
                }
            }
            printk("Finish firmware download!!!\r\n");
        }
#if NO_CHECK_VERSION_DBG == 1
        else
        	printk("no higher fw ver.\r\n");
#endif
#endif
    }
    
#ifdef DOWNLOAD_FIRMWARE_BIN
    check_firmware_bin_download();
#endif
    return ret;
}
#elif ( MCSCHIP_ID == MMS100S )
static int firmware_update(struct melfas_ts_data *ts)
{
    int ret = 0;
    uint8_t fw_ver[2] = {0, };
    
    //struct melfas_ts_data *info = context;
    //struct i2c_client *client = info->client;
    //struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);


    ret = check_firmware(ts, fw_ver);
    if (ret < 0)
        pr_err("[TSP] check_firmware fail! [%d]", ret);
    else
    {
#if MELFAS_DOWNLOAD
#if NO_CHECK_VERSION_DBG
        if (fw_ver[1] < MELFAS_FW_VERSION)
#endif
        {
            int ver;

            printk("[TSP] %s: \n", __func__);
            ret = mms100S_download();
        }
#endif
    }
    return ret;
}
#endif

int distance=1;

extern int hdmi_xscale;
extern int hdmi_yscale;

extern int rk_fb_disp_scale_xpos;
extern int rk_fb_disp_scale_ypos;

//#include "../../video/rockchip/hdmi/rk30_hdmi.h"
//#include "../../video/rockchip/hdmi/rk30_hdmi_hw.h"

static void melfas_ts_get_data(struct melfas_ts_data *ts)
{
    int ret = 0, i;
    uint8_t buf[TS_READ_REGS_LEN] = { 0, };
    int read_num, fingerID, Touch_Type = 0, keyID = 0, touchState = 0;

#if DEBUG_PRINT
    pr_info("[TSP] %s : \n", __FUNCTION__);
#endif
    if (ts == NULL)
        pr_info("[TSP] %s: ts data is NULL \n", __FUNCTION__);

    if (ps_active)
    {
		for (i = 0; i < I2C_RETRY_CNT; i++)
		{
		    ret = melfas_i2c_read(ts->client, TS_READ_START_ADDR, 1, buf);

		    if (ret >= 0)
		    {
		        //pr_info("[TSP] : TS_READ_XXX_STATE [%d] \n", ret);
		        break; // i2c success
		    }
		}
		i = ((buf[0]&0x10)&&(buf[0]&0x80))?0:1;
		//pr_info("reg 0x10=%x, bit4=%d\r\n", buf[0], (buf[0]&0x10)?1:0);
		if (distance!=i)
		{
			distance = i;
			printk("report distance:%d\r\n", distance);
			if (distance == 1)
			{
				//queue_delayed_work(ts->wq, &ts->delay_work, msecs_to_jiffies(TIMER_MS_COUNTS));
				msleep(500);
			}
			else
			{
			
			}
			input_report_abs(ts->ps_input_dev, ABS_DISTANCE, distance);
			input_sync(ts->ps_input_dev);
			
			for (i = 0; i < TS_MAX_TOUCH; i++)
            {
                if (g_Mtouch_info[i].pressure == -1)
                    continue;
                g_Mtouch_info[i].pressure = 0;
			    input_mt_slot(ts->input_dev, i);
			    input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false); 
            }
			input_sync(ts->ps_input_dev);
			return 0;
		}
		buf[0] = 0;
	}
	
    for (i = 0; i < I2C_RETRY_CNT; i++)
    {
        ret = melfas_i2c_read(ts->client, TS_READ_LEN_ADDR, 1, buf);

        if (ret >= 0)
        {
#if DEBUG_PRINT
            pr_info("[TSP] : TS_READ_LEN_ADDR [%d] \n", ret);
#endif
            break; // i2c success
        }
    }

    if (ret < 0)
    {
        pr_info("[TSP] %s,%d: i2c read fail[%u] \n", __FUNCTION__, __LINE__, ret);
        return;
    }
    else
    {
        read_num = buf[0];
#if DEBUG_PRINT
        pr_info("[TSP] %s,%d: read_num[%d] \n", __FUNCTION__, __LINE__, read_num);
#endif
    }

    if (read_num > 0)
    {
        for (i = 0; i < I2C_RETRY_CNT; i++)
        {
 #if BASEBAND_MTK_SUPPORT
            ret = i2c_read_for_mtk(ts->client, TS_READ_START_ADDR, read_num, buf);
#else
            ret = melfas_i2c_read(ts->client, TS_READ_START_ADDR, read_num, buf);
#endif
           if (ret >= 0)
            {
#if DEBUG_PRINT
                pr_info("[TSP] melfas_ts_get_data : TS_READ_START_ADDR [%d] \n", ret);
#endif
                break; // i2c success
            }
        }

        if (ret < 0)
        {
            pr_info("[TSP] %s,%d: i2c read fail[%u] \n", __FUNCTION__, __LINE__, ret);
            return;
        }
        else
        {
            for (i = 0; i < read_num; i = i + 6)
            {
                Touch_Type = (buf[i] >> 5) & 0x03;

                /* touch type is panel */
                if (Touch_Type == TOUCH_SCREEN)
                {
                    fingerID = (buf[i] & 0x0F) - 1;
                    touchState = (buf[i] & 0x80);
                    g_Mtouch_info[fingerID].posX = (uint16_t)(buf[i + 1] & 0x0F) << 8 | buf[i + 2];
                    g_Mtouch_info[fingerID].posY = (uint16_t)(buf[i + 1] & 0xF0) << 4 | buf[i + 3];
                    g_Mtouch_info[fingerID].area = buf[i + 4];
					//if (hotplug  == HDMI_HPD_ACTIVED)
					{
						#define CENTER_X rk_fb_disp_scale_xpos//(1024/2)
						#define CENTER_Y rk_fb_disp_scale_ypos//(600/2)
						//g_Mtouch_info[fingerID].posX = ((g_Mtouch_info[fingerID].posX - CENTER_X)*hdmi_xscale)/100 + CENTER_X;
						//g_Mtouch_info[fingerID].posY = ((g_Mtouch_info[fingerID].posY - CENTER_Y)*hdmi_yscale)/100 + CENTER_Y;
						//printk("xscale:%d yscale:%d ,xpos:%d ypos:%d\r\n", hdmi_xscale, hdmi_yscale, rk_fb_disp_scale_xpos, rk_fb_disp_scale_ypos);
					}
					//printk("x:%d y:%d\r\n", g_Mtouch_info[fingerID].posX, g_Mtouch_info[fingerID].posY);
                    if (touchState)
                        g_Mtouch_info[fingerID].pressure = buf[i + 5];
                    else
                        g_Mtouch_info[fingerID].pressure = 0;
                }
            }


            for (i = 0; i < TS_MAX_TOUCH; i++)
            {
                if (g_Mtouch_info[i].pressure == -1)
                    continue;

#if SLOT_TYPE
		  if(g_Mtouch_info[i].pressure == 0)
		  {
			  // release event
			input_mt_slot(ts->input_dev, i);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false); 
			//printk("release event [%d]\r\n", i);
		  }
		  else
		  {
			//printk("press event [%d]\r\n", i);
			REPORT_MT(i, g_Mtouch_info[i].posX, g_Mtouch_info[i].posY, g_Mtouch_info[i].area, g_Mtouch_info[i].pressure);
		  }
#else
		  if(g_Mtouch_info[i].pressure == 0)
		  {
			  // release event    
		  	input_mt_sync(ts->input_dev);  
		  }
		  else
		  {
			REPORT_MT(i, g_Mtouch_info[i].posX, g_Mtouch_info[i].posY, g_Mtouch_info[i].area, g_Mtouch_info[i].pressure);
		  }
#endif
#if DEBUG_PRINT
                pr_info("[TSP] %s: Touch ID: %d, State : %d, x: %d, y: %d, z: %d w: %d\n", __FUNCTION__,
                        i, (g_Mtouch_info[i].pressure > 0), g_Mtouch_info[i].posX, g_Mtouch_info[i].posY, g_Mtouch_info[i].pressure, g_Mtouch_info[i].area);

#endif

                if (g_Mtouch_info[i].pressure == 0)
                    g_Mtouch_info[i].pressure = -1;
            }
            input_sync(ts->input_dev);
        }
    }
}

static void melfas_ts_work_func(struct work_struct *work)
{
	struct melfas_ts_data *ts = container_of(work, struct melfas_ts_data, work);
	
	melfas_ts_get_data(ts);
	
	//if(ts->use_irq)
		enable_irq(ts->client->irq);
}

static struct workqueue_struct *melfas_wq;

static irqreturn_t melfas_ts_irq_handler(int irq, void *handle)
{
    struct melfas_ts_data *ts = (struct melfas_ts_data *) handle;
#if DEBUG_PRINT
    pr_info("[TSP] %s\n", __FUNCTION__);
#endif

	disable_irq_nosync(ts->client->irq);
	queue_work(melfas_wq, &ts->work);
    //melfas_ts_get_data(ts);
    return IRQ_HANDLED;
}

#define LTR501_DBG	   0
#if LTR501_DBG
#define LTR501_DBG(x...) printk(x)
#else
#define LTR501_DBG(x...)
#endif

struct melfas_ts_data *melfas_ts;
static int ltr501_als_enable(int gainrange)
{
	if (ps_active == 0)
	{
		distance = 1;
		if (melfas_ts)
		{
			input_report_abs(melfas_ts->ps_input_dev, ABS_DISTANCE, distance);
			input_sync(melfas_ts->ps_input_dev);
		}
	}
}

// Put PS into Standby mode
static int ltr501_ps_disable(void)
{
	int error=0;
	distance = 1;
	if (melfas_ts)
	{
		input_report_abs(melfas_ts->ps_input_dev, ABS_DISTANCE, distance);
		input_sync(melfas_ts->ps_input_dev);
	}
	ps_active = 0;
		
	return error;
}
static int ltr501_ps_read_status(void)
{
	int val;
	
	val = ps_active;
	
	return val;
}


static int ltr501_ps_open(struct inode *inode, struct file *file)
{
	if (ps_opened) {
		LTR501_DBG(KERN_ALERT "%s: already opened\n", __func__);
		return -EBUSY;
	}
	ps_opened = 1;
	return 0;
}


static int ltr501_ps_release(struct inode *inode, struct file *file)
{
	LTR501_DBG(KERN_ALERT "%s\n", __func__);
	ps_opened = 0;
	return ltr501_ps_disable();
}


static int ltr501_ps_ioctl( struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int buffer = 0;
	void __user *argp = (void __user *)arg;
//
	LTR501_DBG("----------->>%s, cmd:%X, type: %d, nr: %d \n",__func__, cmd, _IOC_TYPE(cmd), _IOC_NR(cmd));
	switch (cmd){
		//case LTR501_IOCTL_PS_ENABLE:
		case PSENSOR_IOCTL_ENABLE:
			if (copy_from_user(&buffer, argp, 1))
			{
				ret = -EFAULT;
				break;
			}
			LTR501_DBG("LTR501_IOCTL_PS_ENABLE = %d<<<<<<<<<<<<<<<<<<<<<<<<<%s\n\n\n",buffer ,__func__);
			ps_active = buffer;
			
			break;
		//case LTR501_IOCTL_READ_PS_STATUS:
		case PSENSOR_IOCTL_GET_ENABLED:
			buffer = ltr501_ps_read_status();
			LTR501_DBG("LTR501_IOCTL_READ_PS_STATUS=%d<<<<<<<<<<<<<<<<<<<<<<<<<\n",buffer);
			if (&buffer < 0)
			{
				ret = -EFAULT;
				break;
			}
			ret = copy_to_user(argp, &buffer, 1);
			break;
	}
	return ret;
}


static struct file_operations ltr501_ps_fops = {
	.owner		= THIS_MODULE,
	.open		= ltr501_ps_open,
	.release	= ltr501_ps_release,
	.unlocked_ioctl		= ltr501_ps_ioctl,
};


static struct miscdevice ltr501_ps_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "psensor",
	.fops	= &ltr501_ps_fops,
};

static int ps_delay_work(struct work_struct *work)
{
	if (ps_active)
	{
		distance = 1;
		//input_report_abs(ts->ps_input_dev, ABS_DISTANCE, distance);
		//input_sync(ts->ps_input_dev);
	}
	return 0;
}

#define TIMER_MS_COUNTS  1000
static int melfas_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct melfas_ts_data *ts;
	struct input_dev *input_dev;
    int ret = 0, i;
    uint8_t buf[4] = {0, };


    pr_info("[TSP] %s\n", __FUNCTION__);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        pr_info("melfas_ts_probe: need I2C_FUNC_I2C\n");
        ret = -ENODEV;
        goto err_check_functionality_failed;
    }

    ts = kmalloc(sizeof(struct melfas_ts_data), GFP_KERNEL);
    if (ts == NULL)
    {
        pr_info("[TSP] %s: failed to create a state of melfas-ts\n", __FUNCTION__);
        ret = -ENOMEM;
        goto err_alloc_data_failed;
    }

	melfas_ts = ts;
	ts->pdata = client->dev.platform_data;
	
	if (ts->pdata->init_platform_hw)
	    ts->pdata->init_platform_hw();
	
	if (ts->pdata->power_enable)
		ts->power = ts->pdata->power_enable;
	

    ts->power(1);
	
//    msleep(200);
//    msleep(300);

    ts->client = client;
    i2c_set_clientdata(client, ts);

    //ret = firmware_update(ts);
    ret = 1;// firmware_update(ts);

    if (ret < 0)
    {
        pr_info("[TSP] %s: firmware update fail\n", __FUNCTION__);
        msleep(50);
        ret = firmware_update(ts);
        //goto err_detect_failed;
    }
	else
		printk("not need update\n");


	//if (ts->pdata->init_platform_hw)
	//    ts->pdata->init_platform_hw();
	
	//if (ts->pdata->power_enable)
	//	ts->power = ts->pdata->power_enable;
	

    //ts->power(1);
    
	
	
	//ts->wq = create_singlethread_workqueue("melas_wq");
	//INIT_DELAYED_WORK(&ts->delay_work, ps_delay_work);
	//queue_delayed_work(ts->wq, &ts->delay_work, msecs_to_jiffies(TIMER_MS_COUNTS));
	
    ts->input_dev = input_allocate_device();
    if (!ts->input_dev)
    {
        pr_info("[TSP] %s: Not enough memory\n", __FUNCTION__);
        ret = -ENOMEM;
        goto err_input_dev_alloc_failed;
    }

    ts->input_dev->name = "melfas-ts";

	__set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
	__set_bit(EV_ABS, ts->input_dev->evbit);
	input_mt_init_slots(ts->input_dev, TS_MAX_TOUCH);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, 1024, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, 600, 0, 0);

    //__set_bit(EV_ABS,  ts->input_dev->evbit);

	//input_mt_init_slots(ts->input_dev, TS_MAX_TOUCH);
	//input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->pdata->max_x, 0, 0);
	//input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->pdata->max_y, 0, 0);
	//input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, ts->pdata->max_area, 0, 0);
	//input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE, 0, ts->pdata->max_pressure, 0, 0);
	//input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, TS_MAX_TOUCH-1, 0, 0);

    ret = input_register_device(ts->input_dev);
    if (ret)
    {
        pr_info("[TSP] %s: Failed to register device\n", __FUNCTION__);
        ret = -ENOMEM;
        goto err_input_register_device_failed;
    }
	// ps sensor
	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&client->dev,"%s: could not melfas fuck psensor allocate input device\n", __FUNCTION__);
		ret = -ENOMEM;
		return ret;
	}
	ts->ps_input_dev = input_dev;
	input_set_drvdata(input_dev, ts);
	input_dev->name = "proximity";
	set_bit(EV_ABS, input_dev->evbit);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 1, 0, 0);
	ret = input_register_device(input_dev);
	if (ret < 0) {
		printk("%s: could not register melfas fuck psensor input device\n", __FUNCTION__);
            goto err_request_irq;
	}
	distance = 1;
	input_report_abs(ts->ps_input_dev, ABS_DISTANCE, distance);
	input_sync(ts->ps_input_dev);
	
	ltr501_ps_dev.parent = &client->dev;
	ret = misc_register(&ltr501_ps_dev);
	if (ret) {
		printk(KERN_ALERT "%s: LTR-501ALS misc_register ps failed.\n", __func__);
            goto err_request_irq;
	}
	

    if (ts->client->irq)
    {
        ts->client->irq = client->irq = gpio_to_irq(client->irq);
        ret = request_threaded_irq(client->irq, NULL, melfas_ts_irq_handler, /*IRQF_TRIGGER_FALLING*/(IRQF_TRIGGER_LOW | IRQF_ONESHOT), ts->client->name, ts);

        if (ret > 0)
        {
            pr_info("[TSP] %s: Can't allocate irq %d, ret %d\n", __FUNCTION__, ts->client->irq, ret);
            ret = -EBUSY;
            goto err_request_irq;
        }
        
		melfas_wq = create_singlethread_workqueue("melfas_wq");		//create a work queue and worker thread
		if (!melfas_wq) {
			printk(KERN_ALERT "creat workqueue faiked\n");
            goto err_request_irq;
		}
		INIT_WORK(&ts->work, melfas_ts_work_func);		//init work_struct
	
    }

    for (i = 0; i < TS_MAX_TOUCH; i++) /* _SUPPORT_MULTITOUCH_ */
        g_Mtouch_info[i].pressure = -1;

#ifdef CONFIG_HAS_EARLYSUSPEND
    ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ts->early_suspend.suspend = melfas_ts_early_suspend;
    ts->early_suspend.resume = melfas_ts_late_resume;
    register_early_suspend(&ts->early_suspend);
#endif
	
    return 0;

err_request_irq:
    pr_info("[TSP] %s: err_request_irq failed\n", __func__);
    free_irq(client->irq, ts);
err_input_register_device_failed:
    pr_info("[TSP] %s: err_input_register_device failed\n", __func__);
    input_free_device(ts->input_dev);
err_input_dev_alloc_failed:
    pr_info("[TSP] %s: err_input_dev_alloc failed\n", __func__);
err_alloc_data_failed:
    pr_info("[TSP] %s: err_after_get_regulator failed_\n", __func__);
err_detect_failed:
    pr_info("[TSP] %s: err_after_get_regulator failed_\n", __func__);

    kfree(ts);

err_check_functionality_failed:
    pr_info("[TSP] %s: err_check_functionality failed_\n", __func__);

	misc_deregister(&ltr501_ps_dev);
    return ret;
}

static int melfas_ts_remove(struct i2c_client *client)
{
    struct melfas_ts_data *ts = i2c_get_clientdata(client);

    unregister_early_suspend(&ts->early_suspend);
    free_irq(client->irq, ts);
    ts->power(0);
    input_unregister_device(ts->input_dev);
    input_unregister_device(ts->ps_input_dev);
	misc_deregister(&ltr501_ps_dev);
    kfree(ts);
    return 0;
}

static int melfas_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int ret;
    struct melfas_ts_data *ts = i2c_get_clientdata(client);

#if DEBUG_PRINT
    pr_info("[TSP] %s\n", __func__);
#endif

	if (ps_active)
	{
		return 0;
	}
    melfas_ts_release_all_finger(ts);
    disable_irq(client->irq);

    ts->power(0);
    return 0;
}

static int melfas_ts_resume(struct i2c_client *client)
{
    int ret;
    struct melfas_ts_data *ts = i2c_get_clientdata(client);

#if DEBUG_PRINT
    pr_info("[TSP] %s\n", __func__);
#endif

	if (ps_active)
	{
		distance = 1;
		msleep(1000);
		input_report_abs(ts->ps_input_dev, ABS_DISTANCE, distance);
		input_sync(ts->ps_input_dev);
		return 0;
	}
	
    ts->power(1);
    msleep(200);
    enable_irq(client->irq); // scl wave
    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void melfas_ts_early_suspend(struct early_suspend *h)
{
    struct melfas_ts_data *ts;
    ts = container_of(h, struct melfas_ts_data, early_suspend);
    melfas_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void melfas_ts_late_resume(struct early_suspend *h)
{
    struct melfas_ts_data *ts;
    ts = container_of(h, struct melfas_ts_data, early_suspend);
    melfas_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id melfas_ts_id[] =
{
    { "melfas_mms100_MIP", 0 },
    { }
};

static struct i2c_driver melfas_ts_driver =
{
    .driver =
    {
        .name = "melfas_mms100_MIP",
    }, 
    .id_table = melfas_ts_id, 
    .probe = melfas_ts_probe, 
    .remove = __devexit_p(melfas_ts_remove),
#ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend = melfas_ts_suspend, 
    .resume = melfas_ts_resume,
#endif
};

static int __devinit melfas_ts_init(void)
{
    return i2c_add_driver(&melfas_ts_driver);
}

static void __exit melfas_ts_exit(void)
{
    i2c_del_driver(&melfas_ts_driver);
	if (melfas_wq)
		destroy_workqueue(melfas_wq);		//release our work queue
}

MODULE_DESCRIPTION("Driver for Melfas MIP Touchscreen Controller");
MODULE_AUTHOR("MinSang, Kim <kimms@melfas.com>");
MODULE_VERSION("0.2");
MODULE_LICENSE("GPL");

module_init(melfas_ts_init);
module_exit(melfas_ts_exit);

