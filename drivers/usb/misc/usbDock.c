/*
 * USB Dock driver
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <asm/uaccess.h>


#define DRIVER_AUTHOR "chenzy czy@t-chip.com.cn"
#define DRIVER_DESC "USB Dock Driver"

enum out_command {
    VALUE_MID2DOCK_START_COMM = 0x50,
    VALUE_MID2DOCK_REQ_SPK_MODE = 0x51,
    VALUE_MID2DOCK_SEND_TEL_NUM = 0x52,
    VALUE_MID2DOCK_SEND_KAYPAD = 0x53,
    VALUE_MID2DOCK_SET_VOL = 0x54,
    VALUE_MID2DOCK_SET_MUTE = 0x55,
    VALUE_MID2DOCK_POLL_VERS = 0x56,
    VALUE_GET_BASE_INFO = 0xFE,
};


#define MID2DOCK_START_COMM 's'
// #define MID2DOCK_START_COMM 'a'			//for test
#define MID2DOCK_REQ_SPK_MODE 'r'
#define MID2DOCK_SEND_TEL_NUM 't'
#define MID2DOCK_SEND_KAYPAD 'k'
#define MID2DOCK_SET_VOL 'v'
#define GET_BASE_INFO 'i'
#define MID2DOCK_MUTE_MIC 'm'
#define GET_DOCK_SOFT_VER 'f'
#define MID2DOCK_CLEAR_POP_FLAG 'p'
#define MID2DOCK_CLEAR_CID_NUM 'c'

enum in_command {
    DOCK2MID_SEND_BASE_INFO = 0x60,
    DOCK2MID_SEND_CID_NUM = 0x61,
    DOCK2MID_SEND_SOFT_VER = 0x62,
};

#define DOCK_VENDOR_ID 0x0a81
#define DOCK_PRODUCT_ID 0x0205

#define TIMER_MS_COUNTS  1000

#define TIMER_6S_COUNTS  msecs_to_jiffies(6000)// 6000


#define DOCK_USB_CONTROL_MSG_TIMEOUT	100		//ms

unsigned int isDockon = 0;

/* table of devices that work with this driver */
static const struct usb_device_id id_table[] = {
	{ USB_DEVICE(DOCK_VENDOR_ID, DOCK_PRODUCT_ID) },
	{ },
};
MODULE_DEVICE_TABLE (usb, id_table);

struct base_station_info_bit {
    unsigned char reserve1:1;
    unsigned char handle_hook:1;
    unsigned char reserve2:2;
    unsigned char hook:1;
    unsigned char speaker:1;
    unsigned char vol:2;
};

#define HANDLE_STATE
#define SPK_STATE

struct base_station_info {
	//head is 'i'
	unsigned char head;
    //挂摘机状态：0、为挂机  1、为摘机 
    //摘机状态可以进行拨号等操作，反之则否
    unsigned char hook;
    #ifdef HANDLE_STATE
    //手柄状态：1、手柄摘机 0、手机挂机
    unsigned char handle;
    #endif
    #ifdef SPK_STATE
    unsigned char spk;
    #endif
    //话机当前音机 0～2
    unsigned char vol;
    
    unsigned char need_update;
    //是否有新电话打进来
    unsigned char is_call_in;
    //当有新电话打进来时的来电号码
    char cid_num[30];
    //用来隔开电话号码和软件版本信息 固定是'v'
	unsigned char ver;
    //当前电话机底座的软件版本信息
    char dock_soft_ver[20];
};

struct usb_dock {
	struct workqueue_struct *wq;
	struct delayed_work 	    delay_work;	
	struct mutex wq_lock;
	struct usb_device *	udev;
	struct urb *irq;
	struct timer_list timer;
	unsigned char *data;
	dma_addr_t data_dma;
	struct base_station_info base_info;
};

struct usb_dock *g_usb_dock;
struct mutex usbTran_lock;
struct mutex usbClearCID_lock;


#define VENDOR_WRITE_REQUEST_TYPE 0x21
#define VENDOR_WRITE_REQUEST 0x09

#define GET_WVALUE(value) ((0x03<<8)|value)
//wValue

static unsigned int gProbedFlag = 0;

static int send_start_communication(struct usb_dock *dock)
{
	unsigned char data=0;
    int res;
	unsigned char* zxfdata = kmalloc(10,GFP_KERNEL);
	unsigned char* zxfdata_4 = (unsigned char*)(((__u64)(zxfdata) / 4) * 4);
	zxfdata_4[0] = 0;

	// printk("enter here %s \n",__func__);
	__u16 value = GET_WVALUE(VALUE_MID2DOCK_START_COMM);
	
	//mutex_lock(&usbTran_lock);
	res = usb_control_msg(dock->udev, usb_sndctrlpipe(dock->udev, 0),
			VENDOR_WRITE_REQUEST, VENDOR_WRITE_REQUEST_TYPE,
			value, 0, zxfdata_4, 1, DOCK_USB_CONTROL_MSG_TIMEOUT);
	//mutex_unlock(&usbTran_lock);
	//printk("%s:0x%x:0x%x:0x%x:0x%x  %d \n", __FUNCTION__, VENDOR_WRITE_REQUEST_TYPE,
	//		VENDOR_WRITE_REQUEST, value, zxfdata_4[0], res);
			
	kfree(zxfdata);
	return res;
}

static int send_poll_dock_version(struct usb_dock *dock)
{
	unsigned char data=0;
    int res;
	unsigned char* zxfdata = kmalloc(10,GFP_KERNEL);
	unsigned char* zxfdata_4 = (unsigned char*)(((__u64)(zxfdata) / 4) * 4);
	zxfdata_4[0] = 0;

	// printk("enter here %s \n",__func__);
	__u16 value = GET_WVALUE(VALUE_MID2DOCK_POLL_VERS);
	
	//mutex_lock(&usbTran_lock);
	res = usb_control_msg(dock->udev, usb_sndctrlpipe(dock->udev, 0),
			VENDOR_WRITE_REQUEST, VENDOR_WRITE_REQUEST_TYPE,
			value, 0, zxfdata_4, 1, DOCK_USB_CONTROL_MSG_TIMEOUT);
	//mutex_unlock(&usbTran_lock);
	//printk("%s:0x%x:0x%x:0x%x:0x%x  %d \n", __FUNCTION__, VENDOR_WRITE_REQUEST_TYPE,
	//		VENDOR_WRITE_REQUEST, value, zxfdata_4[0], res);
			
	kfree(zxfdata);
	return res;
}

static int set_spk_phone_mode(struct usb_dock *dock, int is_spk_on)
{
	static int status = 0;
    int res;
	unsigned char* zxfdata = kmalloc(10,GFP_KERNEL);
	unsigned char* zxfdata_4 = (unsigned char*)(((__u64)(zxfdata) / 4) * 4);
	zxfdata_4[0] = is_spk_on?0x55:0xaa;

	__u16 value = GET_WVALUE(VALUE_MID2DOCK_REQ_SPK_MODE);
	
	mutex_lock(&usbTran_lock);
	res = usb_control_msg(dock->udev, usb_sndctrlpipe(dock->udev, 0),
			VENDOR_WRITE_REQUEST, VENDOR_WRITE_REQUEST_TYPE,
			value, 0, zxfdata_4, 1, DOCK_USB_CONTROL_MSG_TIMEOUT);
	mutex_unlock(&usbTran_lock);
	printk("%s:0x%x:0x%x:0x%x:0x%x  %d \n", __FUNCTION__, VENDOR_WRITE_REQUEST_TYPE,
			VENDOR_WRITE_REQUEST, value, zxfdata_4[0], res);
	kfree(zxfdata);

	return res;
}

static int set_mute_mic(struct usb_dock *dock, int mute)
{
	static int status = 0;
    int res;
	unsigned char* zxfdata = kmalloc(10,GFP_KERNEL);
	unsigned char* zxfdata_4 = (unsigned char*)(((__u64)(zxfdata) / 4) * 4);
	zxfdata_4[0] = mute?0x55:0xaa;

	__u16 value = GET_WVALUE(VALUE_MID2DOCK_SET_MUTE);
	
	mutex_lock(&usbTran_lock);
	res = usb_control_msg(dock->udev, usb_sndctrlpipe(dock->udev, 0),
			VENDOR_WRITE_REQUEST, VENDOR_WRITE_REQUEST_TYPE,
			value, 0, zxfdata_4, 1, DOCK_USB_CONTROL_MSG_TIMEOUT);
	mutex_unlock(&usbTran_lock);
	printk("%s:0x%x:0x%x:0x%x:0x%x  %d \n", __FUNCTION__, VENDOR_WRITE_REQUEST_TYPE,
			VENDOR_WRITE_REQUEST, value, zxfdata_4[0], res);
	kfree(zxfdata);

	return res;
}

static int send_tel_num(struct usb_dock *dock, int len, char *tel_num)
{
	__u16 value = GET_WVALUE(VALUE_MID2DOCK_SEND_TEL_NUM);
	int res;
	int i;
	
	if (tel_num==NULL)
	{
		return 1;
	}
	
	unsigned char* zxfdata = kmalloc(len+4,GFP_KERNEL);
	unsigned char* zxfdata_4 = (unsigned char*)(((__u64)(zxfdata) / 4) * 4);
	memcpy(zxfdata_4,tel_num,len);

    //解决pause键在重拨时，功能不正常的问题
    for (i=0; i<len; i++)
    {
        if (zxfdata_4[i] == 'p')
        {
            zxfdata_4[i] = 'P';
        }
    }
    //截断超个60个字节的号码
    if (len>60)
    {
        len = 60;
    }
    
	mutex_lock(&usbTran_lock);
	res = usb_control_msg(dock->udev, usb_sndctrlpipe(dock->udev, 0),
			VENDOR_WRITE_REQUEST, VENDOR_WRITE_REQUEST_TYPE,
			value, 0, zxfdata_4, len, DOCK_USB_CONTROL_MSG_TIMEOUT);
	mutex_unlock(&usbTran_lock);
	printk("%s:0x%x:0x%x:0x%x:0x%x num:%s  %d\n", __FUNCTION__, VENDOR_WRITE_REQUEST_TYPE,
			VENDOR_WRITE_REQUEST, value, len, tel_num,res);
	kfree(zxfdata);

	return res;
}

static int send_keypad_value(struct usb_dock *dock, char keypad_value)
{
	__u16 value = GET_WVALUE(VALUE_MID2DOCK_SEND_KAYPAD);
	int res;

	unsigned char* zxfdata = kmalloc(8,GFP_KERNEL);
	unsigned char* zxfdata_4 = (unsigned char*)(((__u64)(zxfdata) / 4) * 4);
	zxfdata_4[0] = keypad_value;

	mutex_lock(&usbTran_lock);
	res = usb_control_msg(dock->udev, usb_sndctrlpipe(dock->udev, 0),
			VENDOR_WRITE_REQUEST, VENDOR_WRITE_REQUEST_TYPE,
			value, 0, zxfdata_4, 1, DOCK_USB_CONTROL_MSG_TIMEOUT);
	mutex_unlock(&usbTran_lock);
	printk("%s:0x%x:0x%x:%c %d\n", __FUNCTION__, VENDOR_WRITE_REQUEST_TYPE,
			VENDOR_WRITE_REQUEST, keypad_value, res);
	kfree(zxfdata);

	return res;
}



//vol 0-2 3个等级
static ssize_t dock_set_vol(struct usb_dock *dock, int vol)
{
	unsigned char* zxfdata = kmalloc(10,GFP_KERNEL);
	unsigned char* zxfdata_4 = (unsigned char*)(((__u64)(zxfdata) / 4) * 4);
	zxfdata_4[0] = vol;

	__u16 value = GET_WVALUE(VALUE_MID2DOCK_SET_VOL);
    int res;
    
	mutex_lock(&usbTran_lock);
    res = usb_control_msg(dock->udev, usb_sndctrlpipe(dock->udev, 0),
			VENDOR_WRITE_REQUEST, VENDOR_WRITE_REQUEST_TYPE,
			value, 0, zxfdata_4, 1, DOCK_USB_CONTROL_MSG_TIMEOUT);
	mutex_unlock(&usbTran_lock);
	printk("%s:0x%x:0x%x:0x%x:0x%x  %d", __FUNCTION__, VENDOR_WRITE_REQUEST_TYPE,
			VENDOR_WRITE_REQUEST, value, vol, res);
	kfree(zxfdata);

	return res;
}

static unsigned char base_info=0;
static unsigned char actual_length=0;
static unsigned char cmd;
static unsigned char cid_lenth=0;
static unsigned char cid_num_buf[32]={0};
static unsigned char cid_buf_index=0;
static unsigned char get_dock_soft_ver=0;
static unsigned char dock_soft_ver_buf[20]={0};
static unsigned char dock_soft_ver_buf_lenth=0;
static unsigned char dock_soft_ver_buf_index=0;
//static unsigned char new_cid_num=0;
//static unsigned char new_call_in=0;
static unsigned char ringer_state=0;

static int count_1s=0;
static int count_1s_2=0;

#define UPDATA_CALL_IN_STATE_IN_DRIVER 1
extern void rk28_send_wakeup_key(void);
/* 
解析dock上报上来的数据，将其填充到dock->base_info中。 
dock->base_info中的数据，在show_keypad中，上报给应用程序。
*/
static ssize_t proces_msg_packet(struct urb *urb, struct usb_dock *dock, int len, char *buf)
{
    int i;
    int debug = 0;
    static int count=0;

	if ((dock->data[0]==0xa1)  \
		&& (dock->data[1]==0x01)  \ 
		&& (dock->data[3]==0x03))
	{//令牌包
	    cmd = dock->data[2];
        switch (cmd)
        {
            case DOCK2MID_SEND_BASE_INFO:
                urb->transfer_buffer_length = 0x01;
                // dock->base_info.is_call_in = 0 + '0';
                break;
            case DOCK2MID_SEND_CID_NUM:
                //urb->transfer_buffer_length = (maxp > 8 ? 8 : maxp);
                cid_lenth = buf[6];
                //printk("revc cid num packet, len=%d\r\n", cid_lenth);
                for (i=0; i<sizeof(cid_num_buf); i++)
                {
                    cid_num_buf[i] = 0;
                }
//                memset(cid_num_buf, 32, 0);
                i = cid_lenth > 8 ? 8 : cid_lenth;
                cid_lenth -= i;
                urb->transfer_buffer_length = i;
                // dock->base_info.is_call_in = 1 + '0';
                break;
            case DOCK2MID_SEND_SOFT_VER:
                dock_soft_ver_buf_lenth = buf[6];//lenth 0xa
                
                for (i=0; i<sizeof(dock_soft_ver_buf); i++)
                {
                    dock_soft_ver_buf[i] = 0;
                }
                i = dock_soft_ver_buf_lenth > 8 ? 8 : dock_soft_ver_buf_lenth;
                dock_soft_ver_buf_lenth -= i;
                urb->transfer_buffer_length = i;
                break;
            default:
                printk("not support cmd!\r\n");
                cmd = 0;
                break;
        }
	}
    else 
    {//数据包
		// printk("send data_info \n");

        if (cmd == DOCK2MID_SEND_BASE_INFO)
        {
            if ((len == 1) && (base_info != buf[0]))
            {
                char hook_tmp;
                base_info = buf[0];
                rk28_send_wakeup_key();
                
                //dock->base_info.hook = ((base_info & 0x04) ? 1:0) + '0';
                hook_tmp = ((base_info & 0x08) ? 1:0) + '0';
                if ((hook_tmp == '1') && (dock->base_info.is_call_in == '0') && (dock->base_info.hook == '0'))
                {
                    //printk("Dock pop=1\r\n");
                    dock->base_info.need_update = '1';
                }
                
                if ((dock->base_info.hook == '1') && (hook_tmp == '0'))
                {
                    if (dock->base_info.is_call_in == '1')
                    {
	                    mutex_lock(&usbClearCID_lock);
                        dock->base_info.is_call_in = '0';
                        for (i=0; i<sizeof(dock->base_info.cid_num); i++)
                        {
                            dock->base_info.cid_num[i] = 0;
                        }
	                    mutex_unlock(&usbClearCID_lock);
	                    count_1s=0;
	                    count_1s_2=0;
                    }
                }
                
                dock->base_info.hook = ((base_info & 0x08) ? 1:0) + '0';
                #ifdef HANDLE_STATE
                dock->base_info.handle = ((base_info & 0x40) ? 1:0) + '0';
                #endif
                #ifdef SPK_STATE
                dock->base_info.spk = ((base_info & 0x04) ? 1:0) + '0';
                #endif
                dock->base_info.vol = (base_info & 0x03) + '0';
                #if (UPDATA_CALL_IN_STATE_IN_DRIVER == 0)
                dock->base_info.is_call_in = ((base_info & 0x80) ? 1:0) + '0';
                #else
                if (ringer_state != (base_info & 0x80) )
                {
                    ringer_state = (base_info & 0x80);
                    //dock->base_info.is_call_in = ((base_info & 0x80) ? 1:0) + '0';
                    if (base_info & 0x80)
                        dock->base_info.is_call_in = '1';
	                //mod_timer(&dock->timer, jiffies + TIMER_6S_COUNTS);
	                count_1s=0;
	                count_1s_2=0;
	                //printk("+++++++++ ringer state updata!\r\n");
                }
                #endif
      		    printk("SEND_BASE_INFO 0x%02x\r\n", buf[0]);
            }
            urb->transfer_buffer_length = 0x08;

            // dock->base_info.is_call_in = 0 + '0';
        }
        else if (cmd == DOCK2MID_SEND_CID_NUM)
        {
        	// printk("send cid_num %s \n", buf);
            //有实际数据
            if (len>0)
            {
                for (i=0; i<len; i++)
                {
                    cid_num_buf[cid_buf_index++] = buf[i];
                }
	        	// printk("send cid_num %s \n", cid_num_buf);
                //cid_num_buf
                if (cid_lenth>0)
                {
                    i = cid_lenth > 8 ? 8 : cid_lenth;
                    cid_lenth -= i;
                    urb->transfer_buffer_length = i;
                }
                else
                {
                    cid_buf_index=0;
                    urb->transfer_buffer_length = 0x0;
                    printk("cid:%s\r\n", cid_num_buf);
	                //count_1s=0;
	                count_1s_2=0;
                    //#if (UPDATA_CALL_IN_STATE_IN_DRIVER != 0)
                    //dock->base_info.is_call_in = 1 + '0';
                    //#endif
                    
	                mutex_lock(&usbClearCID_lock);
                    strcpy(dock->base_info.cid_num, cid_num_buf);
                    mutex_unlock(&usbClearCID_lock);
                    //memset(cid_num_buf, 32, 0);
                    for (i=0; i<sizeof(cid_num_buf); i++)
                    {
                        cid_num_buf[i] = 0;
                    }

                }
            }
            else
            {
                    cid_buf_index=0;
                    for (i=0; i<sizeof(cid_num_buf); i++)
                    {
                        cid_num_buf[i] = 0;
                    }
    
		//memset(cid_num_buf, 32, 0);
            }
        }
        else if (cmd == DOCK2MID_SEND_SOFT_VER)
        {
            //有实际数据
            if (len>0)
            {
                for (i=0; i<len; i++)
                {
                    dock_soft_ver_buf[dock_soft_ver_buf_index++] = buf[i];
                }
                if (dock_soft_ver_buf_lenth>0)
                {
                    i = dock_soft_ver_buf_lenth > 8 ? 8 : dock_soft_ver_buf_lenth;
                    dock_soft_ver_buf_lenth -= i;
                    urb->transfer_buffer_length = i;
                }
                else
                {
                    dock_soft_ver_buf[dock_soft_ver_buf_index] = 0;
                    dock_soft_ver_buf_index=0;
                    urb->transfer_buffer_length = 0x0;
                    strcpy(dock->base_info.dock_soft_ver, dock_soft_ver_buf);
                    get_dock_soft_ver = 1;
                    printk("dock_soft_ver:%s\r\n", dock_soft_ver_buf);
                }
            }
        }
        else
        {
            //not dock packet
            cmd = 0;
        }
    }
}

static ssize_t show_keypad(struct device *dev, struct device_attribute *attr, char *buf)
{								
	struct usb_interface *intf = to_usb_interface(dev);	
	struct usb_dock *dock = usb_get_intfdata(intf);	
	int i;
    
//#define __CID_NUM_TEST_1__
#ifdef __CID_NUM_TEST_1__	
    static int cid_clear_flag=0;
    if (count_1s>4)
    {
	    if (dock->base_info.is_call_in == '1')
	    {
            dock->base_info.is_call_in = '0';
            printk("is_call_in = '0'\r\n");
            cid_clear_flag=0;
	    }
    }
    else if (count_1s>2)
    {
	    if (dock->base_info.is_call_in == '1' && cid_clear_flag == 0)
	    {
	        mutex_lock(&usbClearCID_lock);
            for (i=0; i<sizeof(dock->base_info.cid_num); i++)
            {
                dock->base_info.cid_num[i] = 0;
            }
	        mutex_unlock(&usbClearCID_lock);
	        cid_clear_flag = 1;
            printk("clear cid num\r\n");
	    }
    }
#endif
	dock->base_info.head = 'i';
	
	dock->base_info.ver = 'v';
    
	sprintf(buf, "%s", (char*)(&(dock->base_info)));
	strcat(buf, (char*)(&(dock->base_info.ver)));

	return sizeof(struct base_station_info);
}

//count 一定要为本次命令的实际字节数
//nr = write(dock_fd,&buf, count);
//buf：为JNI层用write接口写进来的数据
static ssize_t send_keypad(struct device *dev, struct device_attribute *attr, char *buf, size_t count)
{
	struct usb_interface *intf = to_usb_interface(dev);	
	struct usb_dock *dock = usb_get_intfdata(intf);	
	
	unsigned char cmd;
	int datalen;

	int i;

	cmd = buf[0];
	switch (cmd) {
	case MID2DOCK_START_COMM:
		break;
	case MID2DOCK_REQ_SPK_MODE:
		printk("dock_ioctl: set_spk_phone_mode:%d\r\n", buf[1]);
		set_spk_phone_mode(dock, (int)(buf[1]-'0'));
		break;
	case MID2DOCK_SEND_TEL_NUM:
		datalen = strlen(&buf[1])-1;
		printk("dock_ioctl: send_tel_num:len=%d,num=%s\r\n", datalen, (char*)&buf[1]);
		send_tel_num(dock, datalen, (char*)&buf[1]);
		break;
	case MID2DOCK_SEND_KAYPAD:
		printk("dock_ioctl: send_keypad_value:%s\r\n", buf[1]);
		send_keypad_value(dock, buf[1]);
		break;
	case MID2DOCK_SET_VOL:
		printk("dock_ioctl: dock_set_vol:%d\r\n", (int)buf[1]);
		dock_set_vol(dock, (int)buf[1]);
		break;
	case GET_BASE_INFO:

	    break;
	case MID2DOCK_MUTE_MIC:
	    set_mute_mic(dock, (int)(buf[1]-'0'));
	    break;
	case MID2DOCK_CLEAR_POP_FLAG:
		printk("dock_ioctl: dock clear pop flag\r\n");
        dock->base_info.need_update = '0';
	    break;
	case MID2DOCK_CLEAR_CID_NUM:	    
	    /*
	    mutex_lock(&usbClearCID_lock);
        for (i=0; i<sizeof(dock->base_info.cid_num); i++)
        {
            dock->base_info.cid_num[i] = 0;
        }
	    mutex_unlock(&usbClearCID_lock);
	    */
	    break;
	default:
		printk("%s not supported = 0x%04x", __func__, cmd);
		break;
	}
	return count;
}

static DEVICE_ATTR(dock_intf, 0666, show_keypad, send_keypad);

//static CLASS_ATTR(dock_class_intf, 0666, 0/*show_keypad*/, send_keypad);

static void usb_dock_irq(struct urb *urb)
{
	struct usb_dock *dock = urb->context;
	unsigned char *data = urb->transfer_buffer;
	unsigned int actual_length = urb->actual_length;

	int i;

    // int i;
    // int debug = 0;
    // static int count=0;
    // if(++count >500)
    // {
    // 	count = 0;
    // 	debug = 1; 
	   //  for(i=0;i<8;i++)
	   //  {
	   //  	printk("0x%02x\t",data[i]);
	   //  }

	   //  printk("\n");
    // }

	switch (urb->status) {
	case 0:			/* success */
		break;
	case -ECONNRESET:	/* unlink */
	case -ENOENT:
	case -ESHUTDOWN:
		return;
	/* -EPIPE:  should clear the halt */
	default:		/* error */
		goto resubmit;
	}
	
	if (actual_length<=0)
	{
		goto resubmit;
	}

    proces_msg_packet(urb, dock, actual_length, data);

resubmit:
	i = usb_submit_urb (urb, GFP_ATOMIC);
	if (i)
		printk("can't resubmit intr, %s-%s, status %d",
			dock->udev->bus->bus_name,
			dock->udev->devpath, i);
}

#if 0
static void dock_timer(unsigned long _data)
{
	struct usb_dock *dock = (struct usb_dock *) _data;
	int status;
    int i;
    
	mutex_lock(&usbClearCID_lock);
    dock->base_info.is_call_in = '0';
    for (i=0; i<sizeof(dock->base_info.cid_num); i++)
    {
        dock->base_info.cid_num[i] = 0;
    }
	mutex_unlock(&usbClearCID_lock);
    
    //printk("dock_timer call func\r\n");
    
	//del_timer_sync(&dock->timer);
}

static int dock_ioctl(struct usb_interface *intf, unsigned int cmd,
                        void *buf)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_dock *dock = usb_get_intfdata (intf);
	
	printk("%s cmd = 0x%04x", __func__, cmd);

	switch (cmd) {
	case MID2DOCK_START_COMM:
		printk("dock_ioctl: send_start_communication\r\n");
		send_start_communication(dock);
		break;
	case MID2DOCK_REQ_SPK_MODE:
		printk("dock_ioctl: set_spk_phone_mode:%d\r\n", (int)buf);
		set_spk_phone_mode(dock, (int)buf);
		break;
	case MID2DOCK_SEND_TEL_NUM:
		printk("dock_ioctl: send_tel_num:len=%d,num=%s\r\n", strlen(buf), (char*)buf);
		send_tel_num(dock, strlen(buf)-1, (char*)buf);
		break;
	case MID2DOCK_SEND_KAYPAD:
		printk("dock_ioctl: send_keypad_value:%s\r\n", buf);
		send_keypad_value(dock, buf);
		break;
	case MID2DOCK_SET_VOL:
		printk("dock_ioctl: dock_set_vol:%d\r\n", (int)buf);
		dock_set_vol(dock, (int)buf);
		break;
	case GET_BASE_INFO:
	    if (buf!=NULL)
	    {
	        copy_to_user(buf, &dock->base_info, sizeof(struct base_station_info));
	        //刷新来电数据
	        if (dock->base_info.is_call_in == 1)
	        {
	            dock->base_info.is_call_in = 0;
	            memset(dock->base_info.cid_num, 30, 0);
	        }
	    }
	    break;
	default:
		printk("%s not supported = 0x%04x", __func__, cmd);
		break;
	}
	return -ENOIOCTLCMD;
}
#endif

static int usb_dock_alloc_mem(struct usb_device *dev, struct usb_dock *dock)
{
	if (!(dock->irq = usb_alloc_urb(0, GFP_KERNEL)))
		return -1;

	if (!(dock->data = usb_alloc_coherent(dev, 8, GFP_ATOMIC, &dock->data_dma)))
		return -1;

	return 0;
}

static void usb_dock_free_mem(struct usb_device *dev, struct usb_dock *dock)
{
	usb_free_urb(dock->irq);
	usb_free_coherent(dev, 8, dock->data, dock->data_dma);
}


static void dock_startCom_work(struct work_struct *work)
{
	struct usb_dock *dock = container_of(work, struct usb_dock, delay_work);
    int i;
    static int count=0;
//#define __CID_NUM_CHANGE__
#ifdef __CID_NUM_CHANGE__
    static int test = 0;
#endif
    //printk("dock_startCom_work  count_1s:%d\r\n", count_1s);
	// mutex_lock(&dock->wq_lock);
	//mutex_lock(&usbTran_lock);
	if (dock==NULL)
	    return;

#ifdef __CID_NUM_CHANGE__
    test++;
    if (test>3)
    {
	    if (dock->base_info.is_call_in == '1' && (dock->base_info.cid_num[0] != 0))
	    {
            dock->base_info.cid_num[0] = '9';
            printk("%s\r\n", dock->base_info.cid_num);
        }
        if (test>6)
            test = 0;
    }
    else
    {
	    if (dock->base_info.is_call_in == '1' && (dock->base_info.cid_num[0] != 0))
	    {
            dock->base_info.cid_num[0] = '8';
            printk("%s\r\n", dock->base_info.cid_num);
        }
    }
#endif

    if (count>=1)
    {
	    mutex_lock(&usbTran_lock);
	    send_start_communication(dock);
	    mutex_unlock(&usbTran_lock);
	    count = 0;
	}
	else
	{
	    count++;
	}
	
	if (get_dock_soft_ver == 0)
	    send_poll_dock_version(dock);
    
	count_1s++;
	count_1s_2++;
	
	//这次计时用来判断响铃后，未接电话之前对方挂线的判断
    if (count_1s > 6)//1＊4 ＝ 4 S
    {
        count_1s=0;
	    mutex_lock(&usbClearCID_lock);
	    //if (dock->base_info.is_call_in == '1')
	    {
            dock->base_info.is_call_in = '0';
            for (i=0; i<sizeof(dock->base_info.cid_num); i++)
            {
                dock->base_info.cid_num[i] = 0;
            }
            //printk("After 6S between the last ringer\r\n");
        }
	    mutex_unlock(&usbClearCID_lock);
        
    }
    //这次计时用于响铃前来的号码
    
    if (count_1s_2 > 2)
    {	           
        if (dock->base_info.is_call_in == '0')   
        {      
            mutex_lock(&usbClearCID_lock);
            for (i=0; i<sizeof(dock->base_info.cid_num); i++)
            {
                dock->base_info.cid_num[i] = 0;
            }
            mutex_unlock(&usbClearCID_lock);
            count_1s_2 = 0;
        }
    }
    
	queue_delayed_work(dock->wq, &dock->delay_work, msecs_to_jiffies(TIMER_MS_COUNTS));
	//mutex_unlock(&usbTran_lock);
}

static struct class *usb_dock_class = NULL;
static int dock_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(interface);
	struct usb_dock *dock = NULL;	
	struct usb_host_interface *iface;
	struct usb_endpoint_descriptor *endpoint;
	int pipe, maxp;
	int retval = -ENOMEM;

	printk("USB Dock probe!\r\n");
	
	/*获取当前接口*/
	iface = interface->cur_altsetting;
	/*接口总数？*/
	if (iface->desc.bNumEndpoints != 1)
		return -ENODEV;
	/*接口端点*/
	endpoint = &iface->endpoint[0].desc;
	/*是否是INT端点*/
	if (!usb_endpoint_is_int_in(endpoint))
	{
	    printk("dock error out not int ep.");
		return -ENODEV;
	}
	/*中断接收通道*/
	pipe = usb_rcvintpipe(udev, endpoint->bEndpointAddress);
	printk("pipe:%d, ep:%d\r\n", pipe, usb_endpoint_num(endpoint));
	
	if (usb_endpoint_num(endpoint) != 1)
	    return 0;

	/*端点传输的包数据大小*/
	maxp = usb_maxpacket(udev, pipe, usb_pipeout(pipe));
	printk("maxp:%d\r\n", maxp);
	
	//if (g_usb_dock != NULL)
	//    return 0;
	
	g_usb_dock = dock = kzalloc(sizeof(struct usb_dock), GFP_KERNEL);
	if (dock == NULL) {
		dev_err(&interface->dev, "out of memory\n");
		goto error_mem;
	}

	dock->udev = usb_get_dev(udev);

	if (usb_dock_alloc_mem(udev, dock)) {
		goto error_mem;
	}

	usb_set_intfdata (interface, dock);
    
    
	retval = device_create_file(&interface->dev, &dev_attr_dock_intf);
	if (retval)
		goto error;

	//usb_dock_class = class_create(THIS_MODULE, "usb_dock");
    //if(usb_dock_class == NULL){
    //    printk("create class usb_dock failed!\n");
    //    return -1;
    //}
    //retval = class_create_file(usb_dock_class, &class_attr_dock_class_intf);
    //if(retval != 0){
    //    printk("create usb_dock_class class file failed!\n");
    //    goto failed_create_class;
    //}
    
	usb_fill_int_urb(dock->irq, udev, pipe,
			 dock->data, (maxp > 8 ? 8 : maxp),
			 usb_dock_irq, dock, endpoint->bInterval);
	dock->irq->transfer_dma = dock->data_dma;
	dock->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	retval = usb_submit_urb (dock->irq, GFP_ATOMIC);
	if (retval)
		printk("can't resubmit intr, %s-%s, status %d",
			dock->udev->bus->bus_name,
			dock->udev->devpath, retval);


	memset(&(dock->base_info),'0',sizeof(struct base_station_info));
    memset(&(dock->base_info.dock_soft_ver), 0, sizeof(dock->base_info.dock_soft_ver));
    
    {
        int i;
        for (i=0; i<sizeof(dock->base_info.cid_num); i++)
        {
            dock->base_info.cid_num[i] = 0;
        }
    }
    dock->base_info.need_update = '0';
    
    base_info = 0;
	// mutex_init(&dock->wq_lock);
	mutex_init(&usbTran_lock);	
	mutex_init(&usbClearCID_lock);

	//send_start_communication(dock);
	//send_poll_dock_version(dock);
	
	//if(gProbedFlag == 0)
	{
		gProbedFlag = 1;
		dock->wq = create_singlethread_workqueue("dock_wq");
		INIT_DELAYED_WORK(&dock->delay_work, dock_startCom_work);
		queue_delayed_work(dock->wq, &dock->delay_work, msecs_to_jiffies(TIMER_MS_COUNTS));
	    //setup_timer(&dock->timer, dock_timer, (unsigned long)dock);
	}

	isDockon = 1;
	count_1s = 0;
	count_1s_2=0;

    #ifdef HANDLE_STATE
    printk("USB Dock report handle state!!\r\n");
    #endif 
    #ifdef SPK_STATE
    printk("USB Dock report speaker state!!\r\n");
    #endif

	dev_info(&interface->dev, "USB Dock device now attached\r\n");
	
	rk28_send_wakeup_key();
	
	return 0;
	
failed_create_class:
    class_destroy(usb_dock_class);
    
error:
	device_remove_file(&interface->dev, &dev_attr_dock_intf);
	usb_set_intfdata (interface, NULL);
	
	usb_dock_free_mem(udev, dock);
	
	usb_put_dev(dock->udev);
	kfree(dock);
error_mem:
	printk("USB Dock probe error out\r\n");
	return retval;
}

static void dock_disconnect(struct usb_interface *interface)
{
	struct usb_dock *dock;
	struct usb_device *udev = interface_to_usbdev(interface);
	struct usb_host_interface *iface;
	struct usb_endpoint_descriptor *endpoint;
	int pipe, maxp;
	int retval = -ENOMEM;

	printk("USB Dock disconnect!\r\n");
	
	
	//rk28_send_wakeup_key();
	
	/*获取当前接口*/
	iface = interface->cur_altsetting;
	/*接口总数？*/
	if (iface->desc.bNumEndpoints != 1)
		return -ENODEV;
	/*接口端点*/
	endpoint = &iface->endpoint[0].desc;
	
	if (usb_endpoint_num(endpoint) != 1)
	    return 0;
	    
	dock = usb_get_intfdata (interface);

	//if(gProbedFlag == 1)
	{
	    printk("enter %s\r\n", __func__);
	    mutex_lock(&usbTran_lock);
		cancel_delayed_work(&dock->delay_work);	
	    //del_timer_sync(&dock->timer);
		gProbedFlag = 0;
	    mutex_unlock(&usbTran_lock);
	}
    get_dock_soft_ver = 0;

	device_remove_file(&interface->dev, &dev_attr_dock_intf);

    class_destroy(usb_dock_class);

	/* first remove the files, then set the pointer to NULL */
	usb_set_intfdata (interface, NULL);
	
	usb_kill_urb(dock->irq);

	usb_put_dev(dock->udev);
	
	usb_dock_free_mem(udev, dock);
	
	kfree(dock);

	isDockon = 0;
	//g_usb_dock=0;

	dev_info(&interface->dev, "USB Dock now disconnected\n");
}

static struct usb_driver dock_driver = {
	.name =		"usbdock",
	.probe =	dock_probe,
	// .unlocked_ioctl =	dock_ioctl,
	.disconnect =	dock_disconnect,
	.id_table =	id_table,
};


static int __init usb_dock_init(void)
{
	int retval = 0;

	retval = usb_register(&dock_driver);
	if (retval)
		err("usb_register failed. Error number %d", retval);
	return retval;
}

static void __exit usb_dock_exit(void)
{
	usb_deregister(&dock_driver);
}

module_init (usb_dock_init);
module_exit (usb_dock_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
