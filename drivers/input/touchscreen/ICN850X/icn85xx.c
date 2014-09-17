/*++
 
 Copyright (c) 2012-2022 ChipOne Technology (Beijing) Co., Ltd. All Rights Reserved.
 This PROPRIETARY SOFTWARE is the property of ChipOne Technology (Beijing) Co., Ltd. 
 and may contains trade secrets and/or other confidential information of ChipOne 
 Technology (Beijing) Co., Ltd. This file shall not be disclosed to any third party,
 in whole or in part, without prior written consent of ChipOne.  
 THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. CHIPONE DISCLAIMS ALL EXPRESS OR 
 IMPLIED WARRANTIES.  
 
 File Name:    icn85xx.c
 Abstract:
               input driver.
 Author:       Zhimin Tian
 Date :        08,14,2013
 Version:      1.0
 History :
     2012,10,30, V0.1 first version  
 --*/

#include "icn85xx.h"
#include <mach/board.h>

#if COMPILE_FW_WITH_DRIVER

#if defined(CONFIG_TCHIP_MACH_TR7078) || defined(CONFIG_TCHIP_MACH_TR7088) 
    #if defined(CONFIG_LCD_RK2926_V86)
        #include "tr7088_icn85xx_fw_TN.h"
    #else
        #include "tr7088_icn85xx_fw_IPS.h"
    #endif
#else
    #include "icn85xx_fw.h"
#endif

#endif

static struct i2c_client *this_client;
short log_rawdata[COL_NUM][ROW_NUM] = {{0}};
short log_diffdata[COL_NUM][ROW_NUM] = {{0}};

#if SUPPORT_ALLWINNER_A13
static char firmware[128] = {"/system/vendor/modules/ICN8505.BIN"};
//if file system not ready,you can use inner array 
//static char firmware[128] = "icn85xx_firmware";
#endif

#if SUPPORT_ROCKCHIP
//static char firmware[128] = {"/system/etc/firmware/icn850x_para_u25gt_600p.bin"};
//if file system not ready,you can use inner array 
static char firmware[128] = "icn85xx_firmware";
#endif

#if SUPPORT_SPREADTRUM
static char firmware[128] = {"/system/sps/ICN85XX/ko/ICN8505.BIN"};
//if file system not ready,you can use inner array 
//static char firmware[128] = "icn85xx_firmware";
#endif

#if SUPPORT_ACTION
static char firmware[128] = {"/misc/modules/ICN8505.bin"};
//if file system not ready,you can use inner array 
//static char firmware[128] = "icn85xx_firmware";
static struct regulator *tp_regulator = NULL;
volatile int current_val = 0;
static struct regulator *regulator_init(const char *name, int minvol, int maxvol);
static inline void regulator_deinit(struct regulator *power);

#endif

// charger support
#if SUPPORT_CHARGER_DETECTER
    #if SUPPORT_ACTION
    extern void atc260x_set_adjust_tp_para(void *ptr);
    extern int atc260x_get_charge_status();
    void icn85xx_ts_charger_mode();
    void icn85xx_ts_charger_mode_disable();
    #endif
#endif

#if SUPPORT_SYSFS
static enum hrtimer_restart chipone_timer_func(struct hrtimer *timer);

static ssize_t icn85xx_show_update(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t icn85xx_store_update(struct device* cd, struct device_attribute *attr, const char* buf, size_t len);
static ssize_t icn85xx_show_process(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t icn85xx_store_process(struct device* cd, struct device_attribute *attr,const char* buf, size_t len);

static DEVICE_ATTR(update, S_IRUGO | S_IWUSR, icn85xx_show_update, icn85xx_store_update);
static DEVICE_ATTR(process, S_IRUGO | S_IWUSR, icn85xx_show_process, icn85xx_store_process);

static ssize_t icn85xx_show_process(struct device* cd,struct device_attribute *attr, char* buf)
{
    ssize_t ret = 0;
    sprintf(buf, "icn85xx process\n");
    ret = strlen(buf) + 1;
    return ret;
}

static ssize_t icn85xx_store_process(struct device* cd, struct device_attribute *attr,
               const char* buf, size_t len)
{
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    unsigned long on_off = simple_strtoul(buf, NULL, 10); 
    if(on_off == 0)
    {
        icn85xx_ts->work_mode = 0;
    }
    else if((on_off == 1) || (on_off == 2))
    {
        if((icn85xx_ts->work_mode == 0) && (icn85xx_ts->use_irq == 1))
        {
            hrtimer_init(&icn85xx_ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
            icn85xx_ts->timer.function = chipone_timer_func;
            hrtimer_start(&icn85xx_ts->timer, ktime_set(CTP_START_TIMER/1000, (CTP_START_TIMER%1000)*1000000), HRTIMER_MODE_REL);
        }
        icn85xx_ts->work_mode = on_off;
    }
    return len;
}

static ssize_t icn85xx_show_update(struct device* cd,
                     struct device_attribute *attr, char* buf)
{
    ssize_t ret = 0;       
    sprintf(buf, firmware);
    ret = strlen(buf) + 1;
    printk("firmware: %s, ret: %d\n", firmware, ret);  

    return ret;
}

static ssize_t icn85xx_store_update(struct device* cd, struct device_attribute *attr, const char* buf, size_t len)
{
    printk("len: %d, update: %s\n", len, buf);  
    memset(firmware, 0, 128);
    memcpy(firmware, buf, len-1);
    if(R_OK == icn85xx_fw_update("/mnt/sdcard/ICN8505.bin"))
    {
        printk("update ok\n");
    }
    else
    {
        printk("update error\n");   
    }    
    return len;
}

static int icn85xx_create_sysfs(struct i2c_client *client)
{
    int err;
    struct device *dev = &(client->dev);
    icn85xx_trace("%s: \n",__func__);    
    err = device_create_file(dev, &dev_attr_update);
    err = device_create_file(dev, &dev_attr_process);
    return err;
}

static void icn85xx_remove_sysfs(struct i2c_client *client)
{
    struct device *dev = &(client->dev);
    icn85xx_trace("%s: \n",__func__);    
    device_remove_file(dev, &dev_attr_update);
    device_remove_file(dev, &dev_attr_process);
}
#endif

#if SUPPORT_PROC_FS

pack_head cmd_head;
static struct proc_dir_entry *icn85xx_proc_entry;
int  DATA_LENGTH = 0;
static int icn85xx_tool_write(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
    int ret = 0;
    
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    proc_info("%s \n",__func__);  
    if(down_interruptible(&icn85xx_ts->sem))  
    {  
        return -1;   
    }     
    ret = copy_from_user(&cmd_head, buff, CMD_HEAD_LENGTH);
    if(ret)
    {
        proc_error("copy_from_user failed.\n");
        goto write_out;
    }  
    else
    {
        ret = CMD_HEAD_LENGTH;
    }
    
    proc_info("wr  :0x%02x.\n", cmd_head.wr);
    proc_info("flag:0x%02x.\n", cmd_head.flag);
    proc_info("circle  :%d.\n", (int)cmd_head.circle);
    proc_info("times   :%d.\n", (int)cmd_head.times);
    proc_info("retry   :%d.\n", (int)cmd_head.retry);
    proc_info("data len:%d.\n", (int)cmd_head.data_len);
    proc_info("addr len:%d.\n", (int)cmd_head.addr_len);
    proc_info("addr:0x%02x%02x.\n", cmd_head.addr[0], cmd_head.addr[1]);
    proc_info("len:%d.\n", (int)len);
    proc_info("data:0x%02x%02x.\n", buff[CMD_HEAD_LENGTH], buff[CMD_HEAD_LENGTH+1]);
    if (1 == cmd_head.wr)  // write para
    {
        ret = copy_from_user(&cmd_head.data[0], &buff[CMD_HEAD_LENGTH], cmd_head.data_len);
        if(ret)
        {
            proc_error("copy_from_user failed.\n");
            goto write_out;
        }
        //need copy to g_structPanelPara

        //memcpy(sPtr, &cmd_head.data[0], cmd_head.data_len);
        //todo write para to tp

        ret = cmd_head.data_len + CMD_HEAD_LENGTH;
        icn85xx_ts->work_mode = 3; //reinit
        goto write_out;

    }
    else if(3 == cmd_head.wr)   //set update file
    {
        ret = copy_from_user(&cmd_head.data[0], &buff[CMD_HEAD_LENGTH], cmd_head.data_len);
        if(ret)
        {
            proc_error("copy_from_user failed.\n");
            goto write_out;
        }
        ret = cmd_head.data_len + CMD_HEAD_LENGTH;
        memset(firmware, 0, 128);
        memcpy(firmware, &cmd_head.data[0], cmd_head.data_len);
        proc_info("firmware : %s\n", firmware);
    }
    else if(5 == cmd_head.wr)  //start update 
    {        
        icn85xx_update_status(1);  
        ret = kernel_thread(icn85xx_fw_update,firmware,CLONE_KERNEL);
        icn85xx_trace("the kernel_thread result is:%d\n", ret);    
    }
    else if(7 == cmd_head.wr)
    { 
       
    }

write_out:
    up(&icn85xx_ts->sem); 
    return len;
    
}
static int icn85xx_tool_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
    int i;
    int ret = 0;
    int data_len = 0;
    int len = 0;
    int loc = 0;
    char row, column, retvalue;
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    if(down_interruptible(&icn85xx_ts->sem))  
    {  
        return -1;   
    }     
    proc_info("%s: count:%d, off:%d, cmd_head.data_len: %d\n",__func__, count, off, cmd_head.data_len); 
    if (cmd_head.wr % 2)
    {
        ret = 0;
        goto read_out;
    }
    else if (0 == cmd_head.wr)   //read para
    {
        //read para
        ret = icn85xx_i2c_rxdata(0x8000, &page[0], cmd_head.data_len);
        if (ret < 0) {
            icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
        }
        goto read_out;

    }
    else if(2 == cmd_head.wr)  //get update status
    {
        page[0] = icn85xx_get_status();
        proc_info("status: %d\n", page[0]); 
    }
    else if(4 == cmd_head.wr)  //read rawdata
    {
        //icn85xx_read_reg(0x8004, &row);
        //icn85xx_read_reg(0x8005, &column);        
        row = cmd_head.addr[1];
        column = cmd_head.addr[0];
        //scan tp rawdata
        icn85xx_write_reg(4, 0x20); 
        mdelay(1);
        icn85xx_read_reg(2, &retvalue);
        while(retvalue != 1)
        {
            mdelay(1);
            icn85xx_read_reg(2, &retvalue);
        }      
        
        for(i=0; i<row; i++)
        { 
            ret = icn85xx_i2c_rxdata(0x2000 + i*(COL_NUM)*2, &log_rawdata[i][0], column*2);
            if (ret < 0) {
                icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
            } 
            //icn85xx_rawdatadump(&log_rawdata[i][0], column, COL_NUM); 
            memcpy(&page[column*2*i], &log_rawdata[i][0], column*2);
            
        } 
    
        //finish scan tp rawdata
        icn85xx_write_reg(2, 0x0);    
        icn85xx_write_reg(4, 0x21); 
        goto read_out;        
    }
    else if(6 == cmd_head.wr)  //read diffdata
    {   
    
        //icn85xx_read_reg(0x8004, &row);
        //icn85xx_read_reg(0x8005, &column); 
        row = cmd_head.addr[1];
        column = cmd_head.addr[0];
                
        //scan tp rawdata
        icn85xx_write_reg(4, 0x20); 
        mdelay(1);
        icn85xx_read_reg(2, &retvalue);
        while(retvalue != 1)
        {
            mdelay(1);
            icn85xx_read_reg(2, &retvalue);
        }      

        for(i=0; i<row; i++)
        { 
            ret = icn85xx_i2c_rxdata(0x3000 + i*(COL_NUM+2)*2 + 2, &log_diffdata[i][0], column*2);
            if (ret < 0) {
                icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
            } 
            //icn85xx_rawdatadump(&log_diffdata[i][0], column, COL_NUM); 
            memcpy(&page[column*2*i], &log_diffdata[i][0], column*2);
        }      
        //finish scan tp rawdata
        icn85xx_write_reg(2, 0x0);         
        icn85xx_write_reg(4, 0x21); 
        
        goto read_out;          
    }
    else if(8 == cmd_head.wr)  //change TxVol, read diff
    {   
    /*    sPtr = TP_GetPanelPara(); 
        sPtr->u8TxVol = 3;
        TP_ReInit();
        TP_NormalWork();
        TP_Scan_Rawdata();
        p_RawData = GetRawdata();
        memcpy(&cmd_head.data[0], p_RawData, cmd_head.data_len);
        sPtr->u8TxVol = 5;
        TP_ReInit();
        TP_NormalWork();
        TP_Scan_Rawdata();
        p_RawData = GetRawdata();
        for(i=0; i<cmd_head.data_len; i=i+2)
        {
            *(short*)&cmd_head.data[i] = *(short*)&cmd_head.data[i] - *(short *)&p_RawData[i];
        }
        memcpy(&page[0], &cmd_head.data[0], cmd_head.data_len);
        sPtr->u8TxVol = 3;
        TP_ReInit();
        TP_NormalWork();  */ 
        goto read_out;          
    }    
read_out:
    up(&icn85xx_ts->sem);   
    proc_info("%s out: %d, cmd_head.data_len: %d\n\n",__func__, count, cmd_head.data_len); 
    return cmd_head.data_len;
}

int init_proc_node()
{
    int i;
    memset(&cmd_head, 0, sizeof(cmd_head));
    cmd_head.data = NULL;

    i = 5;
    while ((!cmd_head.data) && i)
    {
        cmd_head.data = kzalloc(i * DATA_LENGTH_UINT, GFP_KERNEL);
        if (NULL != cmd_head.data)
        {
            break;
        }
        i--;
    }
    if (i)
    {
        //DATA_LENGTH = i * DATA_LENGTH_UINT + GTP_ADDR_LENGTH;
        DATA_LENGTH = i * DATA_LENGTH_UINT;
        icn85xx_trace("alloc memory size:%d.\n", DATA_LENGTH);
    }
    else
    {
        proc_error("alloc for memory failed.\n");
        return 0;
    }

    icn85xx_proc_entry = create_proc_entry(ICN83XX_ENTRY_NAME, 0666, NULL);
    if (icn85xx_proc_entry == NULL)
    {
        proc_error("Couldn't create proc entry!\n");
        return 0;
    }
    else
    {
        icn85xx_trace("Create proc entry success!\n");
        icn85xx_proc_entry->write_proc = icn85xx_tool_write;
        icn85xx_proc_entry->read_proc = icn85xx_tool_read;
    }

    return 1;
}

void uninit_proc_node(void)
{
    kfree(cmd_head.data);
    cmd_head.data = NULL;
    remove_proc_entry(ICN83XX_ENTRY_NAME, NULL);
}
    
#endif


#if TOUCH_VIRTUAL_KEYS
static ssize_t virtual_keys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf,
     __stringify(EV_KEY) ":" __stringify(KEY_MENU) ":100:1030:50:60"
     ":" __stringify(EV_KEY) ":" __stringify(KEY_HOME) ":280:1030:50:60"
     ":" __stringify(EV_KEY) ":" __stringify(KEY_BACK) ":470:1030:50:60"
     ":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":900:1030:50:60"
     "\n");
}

static struct kobj_attribute virtual_keys_attr = {
    .attr = {
        .name = "virtualkeys.chipone-ts",
        .mode = S_IRUGO,
    },
    .show = &virtual_keys_show,
};

static struct attribute *properties_attrs[] = {
    &virtual_keys_attr.attr,
    NULL
};

static struct attribute_group properties_attr_group = {
    .attrs = properties_attrs,
};

static void icn85xx_ts_virtual_keys_init(void)
{
    int ret;
    struct kobject *properties_kobj;      
    properties_kobj = kobject_create_and_add("board_properties", NULL);
    if (properties_kobj)
        ret = sysfs_create_group(properties_kobj,
                     &properties_attr_group);
    if (!properties_kobj || ret)
        pr_err("failed to create board_properties\n");    
}
#endif


#if SUPPORT_ALLWINNER_A13

#define CTP_IRQ_NO             (gpio_int_info[0].port_num)

static void* __iomem gpio_addr = NULL;
static int gpio_int_hdle = 0;
static int gpio_wakeup_hdle = 0;
static int gpio_reset_hdle = 0;
static int gpio_wakeup_enable = 1;
static int gpio_reset_enable = 1;
static user_gpio_set_t  gpio_int_info[1];
static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;

static int  int_cfg_addr[]={PIO_INT_CFG0_OFFSET,PIO_INT_CFG1_OFFSET,
            PIO_INT_CFG2_OFFSET, PIO_INT_CFG3_OFFSET};
/* Addresses to scan */
static union{
    unsigned short dirty_addr_buf[2];
    const unsigned short normal_i2c[2];
}u_i2c_addr = {{0x00},};

static __u32 twi_id = 0;

/*
 * ctp_get_pendown_state  : get the int_line data state, 
 * 
 * return value:
 *             return PRESS_DOWN: if down
 *             return FREE_UP: if up,
 *             return 0: do not need process, equal free up.
 */
static int ctp_get_pendown_state(void)
{
    unsigned int reg_val;
    static int state = FREE_UP;

    //get the input port state
    reg_val = readl(gpio_addr + PIOH_DATA);
    //pr_info("reg_val = %x\n",reg_val);
    if(!(reg_val & (1<<CTP_IRQ_NO))){
        state = PRESS_DOWN;
        icn85xx_info("pen down. \n");
    }else{ //touch panel is free up
        state = FREE_UP;
        icn85xx_info("free up. \n");
    }
    return state;
}

/**
 * ctp_clear_penirq - clear int pending
 *
 */
static void ctp_clear_penirq(void)
{
    int reg_val;
    //clear the IRQ_EINT29 interrupt pending
    //pr_info("clear pend irq pending\n");
    reg_val = readl(gpio_addr + PIO_INT_STAT_OFFSET);
    //writel(reg_val,gpio_addr + PIO_INT_STAT_OFFSET);
    //writel(reg_val&(1<<(IRQ_EINT21)),gpio_addr + PIO_INT_STAT_OFFSET);
    if((reg_val = (reg_val&(1<<(CTP_IRQ_NO))))){
        icn85xx_info("==CTP_IRQ_NO=\n");              
        writel(reg_val,gpio_addr + PIO_INT_STAT_OFFSET);
    }
    return;
}

/**
 * ctp_set_irq_mode - according sysconfig's subkey "ctp_int_port" to config int port.
 * 
 * return value: 
 *              0:      success;
 *              others: fail; 
 */
static int ctp_set_irq_mode(char *major_key , char *subkey, ext_int_mode int_mode)
{
    int ret = 0;
    __u32 reg_num = 0;
    __u32 reg_addr = 0;
    __u32 reg_val = 0;
    //config gpio to int mode
    icn85xx_trace("%s: config gpio to int mode. \n", __func__);
#ifndef SYSCONFIG_GPIO_ENABLE
#else
    if(gpio_int_hdle){
        gpio_release(gpio_int_hdle, 2);
    }
    gpio_int_hdle = gpio_request_ex(major_key, subkey);
    if(!gpio_int_hdle){
        icn85xx_error("request tp_int_port failed. \n");
        ret = -1;
        goto request_tp_int_port_failed;
    }
    gpio_get_one_pin_status(gpio_int_hdle, gpio_int_info, subkey, 1);
    icn85xx_trace("%s, %d: gpio_int_info, port = %d, port_num = %d. \n", __func__, __LINE__, \
        gpio_int_info[0].port, gpio_int_info[0].port_num);
#endif

#ifdef AW_GPIO_INT_API_ENABLE

#else
    icn85xx_trace(" INTERRUPT CONFIG\n");
    reg_num = (gpio_int_info[0].port_num)%8;
    reg_addr = (gpio_int_info[0].port_num)/8;
    reg_val = readl(gpio_addr + int_cfg_addr[reg_addr]);
    reg_val &= (~(7 << (reg_num * 4)));
    reg_val |= (int_mode << (reg_num * 4));
    writel(reg_val,gpio_addr+int_cfg_addr[reg_addr]);
                                                               
    ctp_clear_penirq();
                                                               
    reg_val = readl(gpio_addr+PIO_INT_CTRL_OFFSET); 
    reg_val |= (1 << (gpio_int_info[0].port_num));
    writel(reg_val,gpio_addr+PIO_INT_CTRL_OFFSET);

    udelay(1);
#endif

request_tp_int_port_failed:
    return ret;  
}

/**
 * ctp_set_gpio_mode - according sysconfig's subkey "ctp_io_port" to config io port.
 *
 * return value: 
 *              0:      success;
 *              others: fail; 
 */
static int ctp_set_gpio_mode(void)
{
    //int reg_val;
    int ret = 0;
    //config gpio to io mode
    icn85xx_trace("%s: config gpio to io mode. \n", __func__);
#ifndef SYSCONFIG_GPIO_ENABLE
#else
    if(gpio_int_hdle){
        gpio_release(gpio_int_hdle, 2);
    }
    gpio_int_hdle = gpio_request_ex("ctp_para", "ctp_io_port");
    if(!gpio_int_hdle){
        icn85xx_error("request ctp_io_port failed. \n");
        ret = -1;
        goto request_tp_io_port_failed;
    }
#endif
    return ret;

request_tp_io_port_failed:
    return ret;
}

/**
 * ctp_judge_int_occur - whether interrupt occur.
 *
 * return value: 
 *              0:      int occur;
 *              others: no int occur; 
 */
static int ctp_judge_int_occur(void)
{
    //int reg_val[3];
    int reg_val;
    int ret = -1;

    reg_val = readl(gpio_addr + PIO_INT_STAT_OFFSET);
    if(reg_val&(1<<(CTP_IRQ_NO))){
        ret = 0;
    }
    return ret;     
}

/**
 * ctp_free_platform_resource - corresponding with ctp_init_platform_resource
 *
 */
static void ctp_free_platform_resource(void)
{
    if(gpio_addr){
        iounmap(gpio_addr);
    }
    
    if(gpio_int_hdle){
        gpio_release(gpio_int_hdle, 2);
    }
    
    if(gpio_wakeup_hdle){
        gpio_release(gpio_wakeup_hdle, 2);
    }
    
    if(gpio_reset_hdle){
        gpio_release(gpio_reset_hdle, 2);
    }

    return;
}


/**
 * ctp_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int ctp_init_platform_resource(void)
{
    int ret = 0;

    gpio_addr = ioremap(PIO_BASE_ADDRESS, PIO_RANGE_SIZE);
    //pr_info("%s, gpio_addr = 0x%x. \n", __func__, gpio_addr);
    if(!gpio_addr) {
        ret = -EIO;
        goto exit_ioremap_failed;   
    }
    //    gpio_wakeup_enable = 1;
    gpio_wakeup_hdle = gpio_request_ex("ctp_para", "ctp_wakeup");
    if(!gpio_wakeup_hdle) {
        icn85xx_error("%s: tp_wakeup request gpio fail!\n", __func__);
        gpio_wakeup_enable = 0;
    }

    gpio_reset_hdle = gpio_request_ex("ctp_para", "ctp_reset");
    if(!gpio_reset_hdle) {
        icn85xx_error("%s: tp_reset request gpio fail!\n", __func__);
        gpio_reset_enable = 0;
    }

    return ret;

exit_ioremap_failed:
    ctp_free_platform_resource();
    return ret;
}


/**
 * ctp_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int ctp_fetch_sysconfig_para(void)
{
    int ret = -1;
    int ctp_used = -1;
    char name[I2C_NAME_SIZE];
    __u32 twi_addr = 0;
    //__u32 twi_id = 0;
    script_parser_value_type_t type = SCIRPT_PARSER_VALUE_TYPE_STRING;

    icn85xx_trace("%s. \n", __func__);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_used", &ctp_used, 1)){
        icn85xx_error("%s: script_parser_fetch err. \n", __func__);
        goto script_parser_fetch_err;
    }
    if(1 != ctp_used){
        icn85xx_error("%s: ctp_unused. \n",  __func__);
        //ret = 1;
        return ret;
    }

    if(SCRIPT_PARSER_OK != script_parser_fetch_ex("ctp_para", "ctp_name", (int *)(&name), &type, sizeof(name)/sizeof(int))){
        icn85xx_error("%s: script_parser_fetch err. \n", __func__);
        goto script_parser_fetch_err;
    }
    if(strcmp(CTP_NAME, name)){
        icn85xx_error("%s: name %s does not match CTP_NAME. \n", __func__, name);
        icn85xx_error(CTP_NAME);
        //ret = 1;
        return ret;
    }

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_twi_addr", &twi_addr, sizeof(twi_addr)/sizeof(__u32))){
        icn85xx_error("%s: script_parser_fetch err. \n", name);
        goto script_parser_fetch_err;
    }
    //big-endian or small-endian?
    //pr_info("%s: before: ctp_twi_addr is 0x%x, dirty_addr_buf: 0x%hx. dirty_addr_buf[1]: 0x%hx \n", __func__, twi_addr, u_i2c_addr.dirty_addr_buf[0], u_i2c_addr.dirty_addr_buf[1]);
    u_i2c_addr.dirty_addr_buf[0] = twi_addr;
    u_i2c_addr.dirty_addr_buf[1] = I2C_CLIENT_END;
    icn85xx_trace("%s: after: ctp_twi_addr is 0x%x, dirty_addr_buf[0]: 0x%hx. dirty_addr_buf[1]: 0x%hx \n", __func__, twi_addr, u_i2c_addr.dirty_addr_buf[0], u_i2c_addr.dirty_addr_buf[1]);
    //pr_info("%s: after: ctp_twi_addr is 0x%x, u32_dirty_addr_buf: 0x%hx. u32_dirty_addr_buf[1]: 0x%hx \n", __func__, twi_addr, u32_dirty_addr_buf[0],u32_dirty_addr_buf[1]);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_twi_id", &twi_id, sizeof(twi_id)/sizeof(__u32))){
        icn85xx_error("%s: script_parser_fetch err. \n", name);
        goto script_parser_fetch_err;
    }
    icn85xx_trace("%s: ctp_twi_id is %d. \n", __func__, twi_id);
    
    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_screen_max_x", &screen_max_x, 1)){
        icn85xx_error("%s: script_parser_fetch err. \n", __func__);
        goto script_parser_fetch_err;
    }
    icn85xx_trace("%s: screen_max_x = %d. \n", __func__, screen_max_x);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_screen_max_y", &screen_max_y, 1)){
        icn85xx_error("%s: script_parser_fetch err. \n", __func__);
        goto script_parser_fetch_err;
    }
    icn85xx_trace("%s: screen_max_y = %d. \n", __func__, screen_max_y);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_revert_x_flag", &revert_x_flag, 1)){
        icn85xx_error("%s: script_parser_fetch err. \n", __func__);
        goto script_parser_fetch_err;
    }
    icn85xx_trace("%s: revert_x_flag = %d. \n", __func__, revert_x_flag);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_revert_y_flag", &revert_y_flag, 1)){
        icn85xx_error("%s: script_parser_fetch err. \n", __func__);
        goto script_parser_fetch_err;
    }
    icn85xx_trace("%s: revert_y_flag = %d. \n", __func__, revert_y_flag);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_exchange_x_y_flag", &exchange_x_y_flag, 1)){
        icn85xx_error("%s: script_parser_fetch err. \n", __func__);
        goto script_parser_fetch_err;
    }
    icn85xx_trace("%s: exchange_x_y_flag = %d. \n", __func__, exchange_x_y_flag);

    return 0;

script_parser_fetch_err:
    icn85xx_trace("=========script_parser_fetch_err============\n");
    return ret;
}

/**
 * ctp_reset - function
 *
 */
static void ctp_reset(void)
{
    if(gpio_reset_enable){
        icn85xx_trace("%s. \n", __func__);
        if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_reset_hdle, 0, "ctp_reset")){
            icn85xx_error("%s: err when operate gpio. \n", __func__);
        }
        mdelay(CTP_RESET_LOW_PERIOD);
        if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_reset_hdle, 1, "ctp_reset")){
            icn85xx_error("%s: err when operate gpio. \n", __func__);
        }
        mdelay(CTP_RESET_HIGH_PERIOD);
    }
}

/**
 * ctp_wakeup - function
 *
 */
static void ctp_wakeup(void)
{
    if(1 == gpio_wakeup_enable){  
        icn85xx_trace("%s. \n", __func__);
        if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 0, "ctp_wakeup")){
            icn85xx_error("%s: err when operate gpio. \n", __func__);
        }
        mdelay(CTP_WAKEUP_LOW_PERIOD);
        if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 1, "ctp_wakeup")){
            icn85xx_error("%s: err when operate gpio. \n", __func__);
        }
        mdelay(CTP_WAKEUP_HIGH_PERIOD);

    }
    return;
}
/**
 * ctp_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = client->adapter;

    if(twi_id == adapter->nr)
    {
        icn85xx_trace("%s: Detected chip %s at adapter %d, address 0x%02x\n",
             __func__, CTP_NAME, i2c_adapter_id(adapter), client->addr);

        strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
        return 0;
    }else{
        return -ENODEV;
    }
}
////////////////////////////////////////////////////////////////

static struct ctp_platform_ops ctp_ops = {
    .get_pendown_state                = ctp_get_pendown_state,
    .clear_penirq                     = ctp_clear_penirq,
    .set_irq_mode                     = ctp_set_irq_mode,
    .set_gpio_mode                    = ctp_set_gpio_mode,  
    .judge_int_occur                  = ctp_judge_int_occur,
    .init_platform_resource           = ctp_init_platform_resource,
    .free_platform_resource           = ctp_free_platform_resource,
    .fetch_sysconfig_para             = ctp_fetch_sysconfig_para,
    .ts_reset                         = ctp_reset,
    .ts_wakeup                        = ctp_wakeup,
    .ts_detect                        = ctp_detect,
};

#endif


/* ---------------------------------------------------------------------
*
*   Chipone panel related driver
*
*
----------------------------------------------------------------------*/
/***********************************************************************************************
Name    :   icn85xx_ts_wakeup 
Input   :   void
Output  :   ret
function    : this function is used to wakeup tp
***********************************************************************************************/
void icn85xx_ts_wakeup(void)
{
#if SUPPORT_ALLWINNER_A13
    ctp_ops.ts_wakeup();
#endif    


}

/***********************************************************************************************
Name    :   icn85xx_ts_reset 
Input   :   void
Output  :   ret
function    : this function is used to reset tp, you should not delete it
***********************************************************************************************/
void icn85xx_ts_reset(void)
{
    //set reset func
#if SUPPORT_ALLWINNER_A13
    ctp_ops.ts_reset();    
#endif

#if SUPPORT_ROCKCHIP

#endif

#if SUPPORT_SPREADTRUM
    gpio_direction_output(sprd_3rdparty_gpio_tp_rst, 1);
    gpio_set_value(sprd_3rdparty_gpio_tp_rst, 0);
    mdelay(CTP_RESET_LOW_PERIOD);
    gpio_set_value(sprd_3rdparty_gpio_tp_rst,1);
    mdelay(CTP_RESET_HIGH_PERIOD);
#endif 
#if SUPPORT_ACTION
    //set reset func
    gpio_set_value_cansleep(CTP_RST_PORT, 0);
    msleep(20);
    gpio_set_value_cansleep(CTP_RST_PORT, 1);
    msleep(200);
#endif

}

/***********************************************************************************************
Name    :   icn85xx_irq_disable 
Input   :   void
Output  :   ret
function    : this function is used to disable irq
***********************************************************************************************/
void icn85xx_irq_disable(void)
{
    unsigned long irqflags;
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);

    spin_lock_irqsave(&icn85xx_ts->irq_lock, irqflags);
    if (!icn85xx_ts->irq_is_disable)
    {
        icn85xx_ts->irq_is_disable = 1; 
        disable_irq_nosync(icn85xx_ts->irq);
        //disable_irq(icn85xx_ts->irq);
    }
    spin_unlock_irqrestore(&icn85xx_ts->irq_lock, irqflags);

}

/***********************************************************************************************
Name    :   icn85xx_irq_enable 
Input   :   void
Output  :   ret
function    : this function is used to enable irq
***********************************************************************************************/
void icn85xx_irq_enable(void)
{
    unsigned long irqflags = 0;
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);

    spin_lock_irqsave(&icn85xx_ts->irq_lock, irqflags);
    if (icn85xx_ts->irq_is_disable) 
    {
        enable_irq(icn85xx_ts->irq);
        icn85xx_ts->irq_is_disable = 0; 
    }
    spin_unlock_irqrestore(&icn85xx_ts->irq_lock, irqflags);

}

/***********************************************************************************************
Name    :   icn85xx_prog_i2c_rxdata 
Input   :   addr
            *rxdata
            length
Output  :   ret
function    : read data from icn85xx, prog mode 
***********************************************************************************************/
int icn85xx_prog_i2c_rxdata(unsigned int addr, char *rxdata, int length)
{
    int ret = -1;
    int retries = 0;
#if 0 
    struct i2c_msg msgs[] = {   
        {
            .addr   = ICN85XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = I2C_M_RD,
            .len    = length,
            .buf    = rxdata,
#if SUPPORT_ROCKCHIP            
            .scl_rate = ICN85XX_I2C_SCL,
#endif            
        },
    };
        
    icn85xx_prog_i2c_txdata(addr, NULL, 0);
    while(retries < IIC_RETRY_NUM)
    {    
        ret = i2c_transfer(this_client->adapter, msgs, 1);
        if(ret == 1)break;
        retries++;
    }
    if (retries >= IIC_RETRY_NUM)
    {
        icn85xx_error("%s i2c read error: %d\n", __func__, ret); 
//        icn85xx_ts_reset();
    }    
#else



    unsigned char tmp_buf[3];
    struct i2c_msg msgs[] = {
        {
            .addr   = ICN85XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = 0,
            .len    = 3,
            .buf    = tmp_buf,
#if SUPPORT_ROCKCHIP            
            .scl_rate = ICN85XX_I2C_SCL,
#endif            
        },
        {
            .addr   = ICN85XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = I2C_M_RD,
            .len    = length,
            .buf    = rxdata,
#if SUPPORT_ROCKCHIP            
            .scl_rate = ICN85XX_I2C_SCL,
#endif            
        },
    };

    tmp_buf[0] = (unsigned char)(addr>>16);
    tmp_buf[1] = (unsigned char)(addr>>8);
    tmp_buf[2] = (unsigned char)(addr);
    

    while(retries < IIC_RETRY_NUM)
    {
        ret = i2c_transfer(this_client->adapter, msgs, 2);
        if(ret == 2)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn85xx_error("%s i2c read error: %d\n", __func__, ret); 
//        icn85xx_ts_reset();
    }
#endif      
    return ret;
}
/***********************************************************************************************
Name    :   icn85xx_prog_i2c_txdata 
Input   :   addr
            *rxdata
            length
Output  :   ret
function    : send data to icn85xx , prog mode
***********************************************************************************************/
int icn85xx_prog_i2c_txdata(unsigned int addr, char *txdata, int length)
{
    int ret = -1;
    char tmp_buf[128];
    int retries = 0; 
    struct i2c_msg msg[] = {
        {
            .addr   = ICN85XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = 0,
            .len    = length + 3,
            .buf    = tmp_buf,
#if SUPPORT_ROCKCHIP            
            .scl_rate = ICN85XX_I2C_SCL,
#endif            
        },
    };
    
    if (length > 125)
    {
        icn85xx_error("%s too big datalen = %d!\n", __func__, length);
        return -1;
    }
    
    tmp_buf[0] = (unsigned char)(addr>>16);
    tmp_buf[1] = (unsigned char)(addr>>8);
    tmp_buf[2] = (unsigned char)(addr);


    if (length != 0 && txdata != NULL)
    {
        memcpy(&tmp_buf[3], txdata, length);
    }   
    
    while(retries < IIC_RETRY_NUM)
    {
        ret = i2c_transfer(this_client->adapter, msg, 1);
        if(ret == 1)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn85xx_error("%s i2c write error: %d\n", __func__, ret); 
//        icn85xx_ts_reset();
    }
    return ret;
}
/***********************************************************************************************
Name    :   icn85xx_prog_write_reg
Input   :   addr -- address
            para -- parameter
Output  :   
function    :   write register of icn85xx, prog mode
***********************************************************************************************/
int icn85xx_prog_write_reg(unsigned int addr, char para)
{
    char buf[3];
    int ret = -1;

    buf[0] = para;
    ret = icn85xx_prog_i2c_txdata(addr, buf, 1);
    if (ret < 0) {
        icn85xx_error("write reg failed! %#x ret: %d\n", buf[0], ret);
        return -1;
    }
    
    return ret;
}


/***********************************************************************************************
Name    :   icn85xx_prog_read_reg 
Input   :   addr
            pdata
Output  :   
function    :   read register of icn85xx, prog mode
***********************************************************************************************/
int icn85xx_prog_read_reg(unsigned int addr, char *pdata)
{
    int ret = -1;
    ret = icn85xx_prog_i2c_rxdata(addr, pdata, 1);  
    return ret;    
}

/***********************************************************************************************
Name    :   icn85xx_i2c_rxdata 
Input   :   addr
            *rxdata
            length
Output  :   ret
function    : read data from icn85xx, normal mode   
***********************************************************************************************/
int icn85xx_i2c_rxdata(unsigned short addr, char *rxdata, int length)
{
    int ret = -1;
    int retries = 0;
#if 0
    struct i2c_msg msgs[] = {   
        {
            .addr   = this_client->addr,
            .flags  = I2C_M_RD,
            .len    = length,
            .buf    = rxdata,
#if SUPPORT_ROCKCHIP            
            .scl_rate = ICN85XX_I2C_SCL,
#endif            
        },
    };
        
    icn85xx_i2c_txdata(addr, NULL, 0);
    while(retries < IIC_RETRY_NUM)
    {

        ret = i2c_transfer(this_client->adapter, msgs, 1);
        if(ret == 1)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn85xx_error("%s i2c read error: %d\n", __func__, ret); 
//        icn85xx_ts_reset();
    }

#else
    unsigned char tmp_buf[2];
    struct i2c_msg msgs[] = {
        {
            .addr   = this_client->addr,
            .flags  = 0,
            .len    = 2,
            .buf    = tmp_buf,
#if SUPPORT_ROCKCHIP            
            .scl_rate = ICN85XX_I2C_SCL,
#endif            
        },
        {
            .addr   = this_client->addr,
            .flags  = I2C_M_RD,
            .len    = length,
            .buf    = rxdata,
#if SUPPORT_ROCKCHIP            
            .scl_rate = ICN85XX_I2C_SCL,
#endif
        },
    };
    
    tmp_buf[0] = (unsigned char)(addr>>8);
    tmp_buf[1] = (unsigned char)(addr);
   

    while(retries < IIC_RETRY_NUM)
    {
        ret = i2c_transfer(this_client->adapter, msgs, 2);
        if(ret == 2)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn85xx_error("%s i2c read error: %d\n", __func__, ret); 
    }    
#endif

    return ret;
}
/***********************************************************************************************
Name    :   icn85xx_i2c_txdata 
Input   :   addr
            *rxdata
            length
Output  :   ret
function    : send data to icn85xx , normal mode
***********************************************************************************************/
int icn85xx_i2c_txdata(unsigned short addr, char *txdata, int length)
{
    int ret = -1;
    unsigned char tmp_buf[128];
    int retries = 0;
    struct i2c_msg msg[] = {
        {
            .addr   = this_client->addr,
            .flags  = 0,
            .len    = length + 2,
            .buf    = tmp_buf,
#if SUPPORT_ROCKCHIP             
            .scl_rate = ICN85XX_I2C_SCL,
#endif            
        },
    };
    
    if (length > 125)
    {
        icn85xx_error("%s too big datalen = %d!\n", __func__, length);
        return -1;
    }
    
    tmp_buf[0] = (unsigned char)(addr>>8);
    tmp_buf[1] = (unsigned char)(addr);

    if (length != 0 && txdata != NULL)
    {
        memcpy(&tmp_buf[2], txdata, length);
    }   
    
    while(retries < IIC_RETRY_NUM)
    {
        ret = i2c_transfer(this_client->adapter, msg, 1);
        if(ret == 1)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn85xx_error("%s i2c write error: %d\n", __func__, ret); 
    }

    return ret;
}

/***********************************************************************************************
Name    :   icn85xx_write_reg
Input   :   addr -- address
            para -- parameter
Output  :   
function    :   write register of icn85xx, normal mode
***********************************************************************************************/
int icn85xx_write_reg(unsigned short addr, char para)
{
    char buf[3];
    int ret = -1;

    buf[0] = para;
    ret = icn85xx_i2c_txdata(addr, buf, 1);
    if (ret < 0) {
        icn85xx_error("write reg failed! %#x ret: %d\n", buf[0], ret);
        return -1;
    }
    
    return ret;
}


/***********************************************************************************************
Name    :   icn85xx_read_reg 
Input   :   addr
            pdata
Output  :   
function    :   read register of icn85xx, normal mode
***********************************************************************************************/
int icn85xx_read_reg(unsigned short addr, char *pdata)
{
    int ret = -1;
    ret = icn85xx_i2c_rxdata(addr, pdata, 1);
//    printk("addr: 0x%x: 0x%x\n", addr, *pdata);  
    return ret;    
}


/***********************************************************************************************
Name    :   icn85xx_log
Input   :   0: rawdata, 1: diff data
Output  :   err type
function    :   calibrate param
***********************************************************************************************/
int  icn85xx_log(char diff)
{
    char row = 0;
    char column = 0;
    int i, j, ret;
    char retvalue;
    icn85xx_read_reg(0x8004, &row);
    icn85xx_read_reg(0x8005, &column);

    //scan tp rawdata
    icn85xx_write_reg(4, 0x20); 
    mdelay(1);
    icn85xx_read_reg(2, &retvalue);
    while(retvalue != 1)
    {
        mdelay(1);
        icn85xx_read_reg(2, &retvalue);        
    }      

    if(diff == 1)
    {
        for(i=0; i<row; i++)
        { 
            ret = icn85xx_i2c_rxdata(0x3000 + i*(COL_NUM+2)*2 + 2, &log_diffdata[i][0], column*2);
            if (ret < 0) {
                icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
            } 
            icn85xx_rawdatadump(&log_diffdata[i][0], column, COL_NUM);   
            
        }           

    }
    else
    {
        for(i=0; i<row; i++)
        {       
            ret = icn85xx_i2c_rxdata(0x2000 + i*COL_NUM*2, &log_rawdata[i][0], column*2);
            if (ret < 0) {
                icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
            } 
            icn85xx_rawdatadump(&log_rawdata[i][0], column, COL_NUM);  

        }     
        
    }
    
    //finish scan tp rawdata
    icn85xx_write_reg(2, 0x0);      
    icn85xx_write_reg(4, 0x21); 
}


/***********************************************************************************************
Name    :   icn85xx_iic_test 
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
static int icn85xx_iic_test(void)
{
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    int  ret = -1;
    char value = 0;
    int  retry = 0;
    int  flashid;
    icn85xx_ts->ictype = 0;

//    icn85xx_ts->ictype = ICN85XX_WITHOUT_FLASH;
//    return 1;
    
    while(retry++ < 3)
    {        
        ret = icn85xx_read_reg(0xa, &value);
        if(ret > 0)
        {
        	if(value == 0x85)
        	{
                icn85xx_ts->ictype = ICN85XX_WITH_FLASH;
            	return ret;
        	}
        }
        
        icn85xx_error("iic test error! %d\n", retry);
        msleep(3);
    }

    // add junfuzhang 20131211
    // force ic enter progmode
    icn85xx_goto_progmode();
    msleep(10);
    
    retry = 0;
    while(retry++ < 3)
    {    	
	    ret = icn85xx_prog_i2c_rxdata(0x040002, &value, 1);
	    icn85xx_info("icn85xx_check_progmod: 0x%x\n", value);
	    if(ret > 0)
	    {
	    	if(value == 0x85)
	    	{
                flashid = icn85xx_read_flashid();
                printk("flash id:%x\n", flashid);
                if((MD25D40_ID1 == flashid) || (MD25D40_ID2 == flashid)
                    ||(MD25D20_ID1 == flashid) || (MD25D20_ID1 == flashid)
                    ||(GD25Q10_ID == flashid) || (MX25L512E_ID == flashid) || (MD25D10_ID1 == flashid) || (MD25D05_ID1 == flashid))
                {
                    icn85xx_ts->ictype = ICN85XX_WITH_FLASH;
                }
                else
                {
                    icn85xx_ts->ictype = ICN85XX_WITHOUT_FLASH;
                }
	        	return ret;
	    	}
	    }  	       
        icn85xx_error("iic2 test error! %d\n", retry);
        msleep(3);
    }    
    
    return ret;    
}
/***********************************************************************************************
Name    :   icn85xx_ts_release 
Input   :   void
Output  :   
function    : touch release
***********************************************************************************************/
static void icn85xx_ts_release(void)
{
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    icn85xx_info("==icn85xx_ts_release ==\n");
    input_report_abs(icn85xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
    input_sync(icn85xx_ts->input_dev);
}

/***********************************************************************************************
Name    :   icn85xx_report_value_A
Input   :   void
Output  :   
function    : reprot touch ponit
***********************************************************************************************/
static void icn85xx_report_value_A(void)
{
    icn85xx_info("==icn85xx_report_value_A ==\n");
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    char buf[POINT_NUM*POINT_SIZE+3]={0};
    int ret = -1;
    int i;
#if TOUCH_VIRTUAL_KEYS
    unsigned char button;
    static unsigned char button_last;
#endif

    ret = icn85xx_i2c_rxdata(0x1000, buf, POINT_NUM*POINT_SIZE+2);
    if (ret < 0) {
        icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
        return ret;
    }
#if TOUCH_VIRTUAL_KEYS    
    button = buf[0];    
    icn85xx_info("%s: button=%d\n",__func__, button);

    if((button_last != 0) && (button == 0))
    {
        icn85xx_ts_release();
        button_last = button;
        return 1;       
    }
    if(button != 0)
    {
        switch(button)
        {
            case ICN_VIRTUAL_BUTTON_HOME:
                icn85xx_info("ICN_VIRTUAL_BUTTON_HOME down\n");
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 200);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_X, 280);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_Y, 1030);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
                input_mt_sync(icn85xx_ts->input_dev);
                input_sync(icn85xx_ts->input_dev);            
                break;
            case ICN_VIRTUAL_BUTTON_BACK:
                icn85xx_info("ICN_VIRTUAL_BUTTON_BACK down\n");
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 200);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_X, 470);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_Y, 1030);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
                input_mt_sync(icn85xx_ts->input_dev);
                input_sync(icn85xx_ts->input_dev);                
                break;
            case ICN_VIRTUAL_BUTTON_MENU:
                icn85xx_info("ICN_VIRTUAL_BUTTON_MENU down\n");
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 200);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_X, 100);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_Y, 1030);
                input_report_abs(icn85xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
                input_mt_sync(icn85xx_ts->input_dev);
                input_sync(icn85xx_ts->input_dev);            
                break;                      
            default:
                icn85xx_info("other gesture\n");
                break;          
        }
        button_last = button;
        return 1;
    }        
#endif
 
    icn85xx_ts->point_num = buf[1];    
    if (icn85xx_ts->point_num == 0) {
        icn85xx_ts_release();
        return 1; 
    }   
    for(i=0;i<icn85xx_ts->point_num;i++){
        if(buf[8 + POINT_SIZE*i]  != 4)
        {
          break ;
        }
        else
        {
        
        }
    }
    
    if(i == icn85xx_ts->point_num) {
        icn85xx_ts_release();
        return 1; 
    }   

    for(i=0; i<icn85xx_ts->point_num; i++)
    {
        icn85xx_ts->point_info[i].u8ID = buf[2 + POINT_SIZE*i];
        icn85xx_ts->point_info[i].u16PosX = (buf[4 + POINT_SIZE*i]<<8) + buf[3 + POINT_SIZE*i];
        icn85xx_ts->point_info[i].u16PosY = (buf[6 + POINT_SIZE*i]<<8) + buf[5 + POINT_SIZE*i];
        icn85xx_ts->point_info[i].u8Pressure = 200;//buf[7 + POINT_SIZE*i];
        icn85xx_ts->point_info[i].u8EventId = buf[8 + POINT_SIZE*i];    

        if(1 == icn85xx_ts->revert_x_flag)
        {
            icn85xx_ts->point_info[i].u16PosX = icn85xx_ts->screen_max_x- icn85xx_ts->point_info[i].u16PosX;
        }
        if(1 == icn85xx_ts->revert_y_flag)
        {
            icn85xx_ts->point_info[i].u16PosY = icn85xx_ts->screen_max_y- icn85xx_ts->point_info[i].u16PosY;
        }
        
        icn85xx_info("u8ID %d\n", icn85xx_ts->point_info[i].u8ID);
        icn85xx_info("u16PosX %d\n", icn85xx_ts->point_info[i].u16PosX);
        icn85xx_info("u16PosY %d\n", icn85xx_ts->point_info[i].u16PosY);
        icn85xx_info("u8Pressure %d\n", icn85xx_ts->point_info[i].u8Pressure);
        icn85xx_info("u8EventId %d\n", icn85xx_ts->point_info[i].u8EventId);  


        input_report_abs(icn85xx_ts->input_dev, ABS_MT_TRACKING_ID, icn85xx_ts->point_info[i].u8ID);    
        input_report_abs(icn85xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, icn85xx_ts->point_info[i].u8Pressure);
        input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_X, icn85xx_ts->point_info[i].u16PosX);
        input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_Y, icn85xx_ts->point_info[i].u16PosY);
        input_report_abs(icn85xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
        input_mt_sync(icn85xx_ts->input_dev);
        icn85xx_point_info("point: %d ===x = %d,y = %d, press = %d ====\n",i, icn85xx_ts->point_info[i].u16PosX,icn85xx_ts->point_info[i].u16PosY, icn85xx_ts->point_info[i].u8Pressure);
    }

    input_sync(icn85xx_ts->input_dev);
  
}
/***********************************************************************************************
Name    :   icn85xx_report_value_B
Input   :   void
Output  :   
function    : reprot touch ponit
***********************************************************************************************/
#if CTP_REPORT_PROTOCOL
static void icn85xx_report_value_B(void)
{
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    char buf[POINT_NUM*POINT_SIZE+3]={0};
    static unsigned char finger_last[POINT_NUM + 1]={0};
    unsigned char  finger_current[POINT_NUM + 1] = {0};
    unsigned int position = 0;
    int temp = 0;
    int ret = -1;
    icn85xx_info("==icn85xx_report_value_B ==\n");



    ret = icn85xx_i2c_rxdata(0x1000, buf, POINT_NUM*POINT_SIZE+2);
    if (ret < 0) {
        icn85xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
        return ret;
    }
    
  
    icn85xx_ts->point_num = buf[1];

    if(icn85xx_ts->point_num > 0)
    {
        for(position = 0; position<icn85xx_ts->point_num; position++)
        {       
            temp = buf[2 + POINT_SIZE*position] + 1;
            finger_current[temp] = 1;
            icn85xx_ts->point_info[temp].u8ID = buf[2 + POINT_SIZE*position];
            icn85xx_ts->point_info[temp].u16PosX = (buf[4 + POINT_SIZE*position]<<8) + buf[3 + POINT_SIZE*position];
            icn85xx_ts->point_info[temp].u16PosY = (buf[6 + POINT_SIZE*position]<<8) + buf[5 + POINT_SIZE*position];
            icn85xx_ts->point_info[temp].u8Pressure = buf[7 + POINT_SIZE*position];
            icn85xx_ts->point_info[temp].u8EventId = buf[8 + POINT_SIZE*position];
            
            if(icn85xx_ts->point_info[temp].u8EventId == 4)
                finger_current[temp] = 0;
                            
            if(1 == icn85xx_ts->revert_x_flag)
            {
                icn85xx_ts->point_info[temp].u16PosX = icn85xx_ts->screen_max_x- icn85xx_ts->point_info[temp].u16PosX;
            }
            if(1 == icn85xx_ts->revert_y_flag)
            {
                icn85xx_ts->point_info[temp].u16PosY = icn85xx_ts->screen_max_y- icn85xx_ts->point_info[temp].u16PosY;
            }
            icn85xx_info("temp %d\n", temp);
            icn85xx_info("u8ID %d\n", icn85xx_ts->point_info[temp].u8ID);
            icn85xx_info("u16PosX %d\n", icn85xx_ts->point_info[temp].u16PosX);
            icn85xx_info("u16PosY %d\n", icn85xx_ts->point_info[temp].u16PosY);
            icn85xx_info("u8Pressure %d\n", icn85xx_ts->point_info[temp].u8Pressure);
            icn85xx_info("u8EventId %d\n", icn85xx_ts->point_info[temp].u8EventId);             
            //icn85xx_info("u8Pressure %d\n", icn85xx_ts->point_info[temp].u8Pressure*16);
        }
    }   
    else
    {
        for(position = 1; position < POINT_NUM+1; position++)
        {
            finger_current[position] = 0;
        }
        icn85xx_info("no touch\n");
    }

    for(position = 1; position < POINT_NUM + 1; position++)
    {
        if((finger_current[position] == 0) && (finger_last[position] != 0))
        {
            input_mt_slot(icn85xx_ts->input_dev, position-1);
            input_mt_report_slot_state(icn85xx_ts->input_dev, MT_TOOL_FINGER, false);
            icn85xx_point_info("one touch up: %d\n", position);
        }
        else if(finger_current[position])
        {
            input_mt_slot(icn85xx_ts->input_dev, position-1);
            input_mt_report_slot_state(icn85xx_ts->input_dev, MT_TOOL_FINGER, true);
            input_report_abs(icn85xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 1);
            //input_report_abs(icn85xx_ts->input_dev, ABS_MT_PRESSURE, icn85xx_ts->point_info[position].u8Pressure);
            input_report_abs(icn85xx_ts->input_dev, ABS_MT_PRESSURE, 200);
            input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_X, icn85xx_ts->point_info[position].u16PosX);
            input_report_abs(icn85xx_ts->input_dev, ABS_MT_POSITION_Y, icn85xx_ts->point_info[position].u16PosY);
            icn85xx_point_info("===position: %d, x = %d,y = %d, press = %d ====\n", position, icn85xx_ts->point_info[position].u16PosX,icn85xx_ts->point_info[position].u16PosY, icn85xx_ts->point_info[position].u8Pressure);
        }

    }
    input_sync(icn85xx_ts->input_dev);

    for(position = 1; position < POINT_NUM + 1; position++)
    {
        finger_last[position] = finger_current[position];
    }
    
}
#endif

/***********************************************************************************************
Name    :   icn85xx_ts_pen_irq_work
Input   :   void
Output  :   
function    : work_struct
***********************************************************************************************/
static void icn85xx_ts_pen_irq_work(struct work_struct *work)
{
    int ret = -1;
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);  
#if SUPPORT_PROC_FS
    if(down_interruptible(&icn85xx_ts->sem))  
    {  
        return -1;   
    }  
#endif
      
    if(icn85xx_ts->work_mode == 0)
    {
#if CTP_REPORT_PROTOCOL
        icn85xx_report_value_B();
#else
        icn85xx_report_value_A();
#endif 

#if (SUPPORT_ALLWINNER_A13 == 0)  
        if(icn85xx_ts->use_irq)
        {
            icn85xx_irq_enable();
        }
#endif
    }
    else if(icn85xx_ts->work_mode == 1)
    {
        printk("raw data\n");
        icn85xx_log(0);   //raw data
    }
    else if(icn85xx_ts->work_mode == 2)
    {
        printk("diff data\n");
        icn85xx_log(1);   //diff data
    }
    else if(icn85xx_ts->work_mode == 3)  //write para, reinit
    {
        printk("reinit tp\n");
        icn85xx_write_reg(0, 1); 
        mdelay(100);
        icn85xx_write_reg(0, 0);            
        icn85xx_ts->work_mode = 0;
    }

#if SUPPORT_PROC_FS
    up(&icn85xx_ts->sem);
#endif


}
/***********************************************************************************************
Name    :   chipone_timer_func
Input   :   void
Output  :   
function    : Timer interrupt service routine.
***********************************************************************************************/
static enum hrtimer_restart chipone_timer_func(struct hrtimer *timer)
{
    struct icn85xx_ts_data *icn85xx_ts = container_of(timer, struct icn85xx_ts_data, timer);
    queue_work(icn85xx_ts->ts_workqueue, &icn85xx_ts->pen_event_work);
    //icn85xx_info("chipone_timer_func\n"); 
    if(icn85xx_ts->use_irq == 1)
    {
        if((icn85xx_ts->work_mode == 1) || (icn85xx_ts->work_mode == 2))
        {
            hrtimer_start(&icn85xx_ts->timer, ktime_set(CTP_POLL_TIMER/1000, (CTP_POLL_TIMER%1000)*1000000), HRTIMER_MODE_REL);
        }
    }
    else
    {
        hrtimer_start(&icn85xx_ts->timer, ktime_set(CTP_POLL_TIMER/1000, (CTP_POLL_TIMER%1000)*1000000), HRTIMER_MODE_REL);
    }
    return HRTIMER_NORESTART;
}
/***********************************************************************************************
Name    :   icn85xx_ts_interrupt
Input   :   void
Output  :   
function    : interrupt service routine
***********************************************************************************************/
static irqreturn_t icn85xx_ts_interrupt(int irq, void *dev_id)
{
    struct icn85xx_ts_data *icn85xx_ts = dev_id;
       
    icn85xx_info("==========------icn85xx_ts TS Interrupt-----============\n"); 

#if SUPPORT_ALLWINNER_A13
// irq share, can not disable irq?
    if(!ctp_ops.judge_int_occur()){
        icn85xx_info("==IRQ_EINT21=\n");
        ctp_ops.clear_penirq();
        if (!work_pending(&icn85xx_ts->pen_event_work)) 
        {
            icn85xx_info("Enter work\n");
            queue_work(icn85xx_ts->ts_workqueue, &icn85xx_ts->pen_event_work);

        }
    }else{
        icn85xx_info("Other Interrupt\n");
        return IRQ_NONE;
    }
#else
    if(icn85xx_ts->use_irq)
        icn85xx_irq_disable();
    if (!work_pending(&icn85xx_ts->pen_event_work)) 
    {
        icn85xx_info("Enter work\n");
        queue_work(icn85xx_ts->ts_workqueue, &icn85xx_ts->pen_event_work);

    }
#endif

    return IRQ_HANDLED;
}

static int icn850x_shutdown_low(void)
{
	if(CTP_RST_PORT != INVALID_GPIO)
	{
		gpio_direction_output(CTP_RST_PORT, 0);
		gpio_set_value(CTP_RST_PORT,GPIO_LOW);
	}
	return 0;
}

static int icn850x_shutdown_high(void)
{
	if(CTP_RST_PORT != INVALID_GPIO)
	{
		gpio_direction_output(CTP_RST_PORT, 0);
		gpio_set_value(CTP_RST_PORT,GPIO_HIGH);
	}
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
/***********************************************************************************************
Name    :   icn85xx_ts_suspend
Input   :   void
Output  :   
function    : tp enter sleep mode
***********************************************************************************************/
static void icn85xx_ts_suspend(struct early_suspend *handler)
{
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    icn85xx_trace("icn85xx_ts_suspend\n");
    if (icn85xx_ts->use_irq)
    {
        icn85xx_irq_disable();
    }
    else
    {
        hrtimer_cancel(&icn85xx_ts->timer);
    }  
    
#if SUSPEND_POWER_OFF
    //power off

    #if SUPPORT_ACTION
    if ( tp_regulator ){        
        current_val = regulator_get_voltage(tp_regulator);        
        printk("current_val is %d \n",current_val);            
        //disable_power(tp_regulator);        
        regulator_deinit(tp_regulator);    
    }
    #endif
    
    //reset flag if ic is flashless when power off
    if(icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH)
    {
        icn85xx_ts->code_loaded_flag = 0;
    }      
#else
    icn85xx_write_reg(ICN83XX_REG_PMODE, PMODE_HIBERNATE); 
#endif
icn850x_shutdown_low();    
}

/***********************************************************************************************
Name    :   icn85xx_ts_resume
Input   :   void
Output  :   
function    : wakeup tp or reset tp
***********************************************************************************************/
static void icn85xx_ts_resume(struct early_suspend *handler)
{
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(this_client);
    int i;
	int err;
    icn85xx_trace("==icn85xx_ts_resume== \n");
    icn850x_shutdown_high();
    msleep(200);
    //report touch release    
#if CTP_REPORT_PROTOCOL
    for(i = 0; i < POINT_NUM; i++)
    {
        input_mt_slot(icn85xx_ts->input_dev, i);
        input_mt_report_slot_state(icn85xx_ts->input_dev, MT_TOOL_FINGER, false);
    }
#else
    icn85xx_ts_release();
#endif 

#if SUSPEND_POWER_OFF
    //power on tp
    #if SUPPORT_ACTION
    tp_regulator = regulator_init(CTP_POWER_ID, CTP_POWER_MIN_VOL, CTP_POWER_MAX_VOL);		
    if ( !tp_regulator ) {		
        //printk(KERN_ERR "icn83xx tp init power failed");      
        err = -EINVAL;      
        return err;     
    }

    #endif
#else
    icn85xx_ts_wakeup();
    icn85xx_ts_reset();
#endif

    //reload code
    if((icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH) && (icn85xx_ts->code_loaded_flag == 0))
    {
        if(R_OK == icn85xx_fw_update(firmware))
        {
            icn85xx_ts->code_loaded_flag = 1;
            icn85xx_trace("ICN85XX_WITHOUT_FLASH, reload code ok\n"); 
        }
        else
        {
            icn85xx_ts->code_loaded_flag = 0;
            icn85xx_trace("ICN85XX_WITHOUT_FLASH, reload code error\n"); 
        }
    }   
    if (icn85xx_ts->use_irq)
    {
        icn85xx_irq_enable();
    }
    else
    {
        hrtimer_start(&icn85xx_ts->timer, ktime_set(CTP_START_TIMER/1000, (CTP_START_TIMER%1000)*1000000), HRTIMER_MODE_REL);
    }

    #if SUPPORT_ACTION
    if(atc260x_get_charge_status() == 1){		
        //printk("atc260x_get_charge_status====1\n");     
        icn85xx_ts_charger_mode();  
    }   
    else {       
        icn85xx_ts_charger_mode_disable(); 
    }
    #endif

    
}
#endif

/***********************************************************************************************
Name    :   icn85xx_request_io_port
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
static int icn85xx_request_io_port(struct icn85xx_ts_data *icn85xx_ts)
{
    int err = 0;
#if SUPPORT_ALLWINNER_A13
    icn85xx_ts->screen_max_x = screen_max_x;
    icn85xx_ts->screen_max_y = screen_max_y;
    icn85xx_ts->revert_x_flag = revert_x_flag;
    icn85xx_ts->revert_y_flag = revert_y_flag;
    icn85xx_ts->exchange_x_y_flag = exchange_x_y_flag;
    icn85xx_ts->irq = CTP_IRQ_PORT;
#endif 

#if SUPPORT_ROCKCHIP
    icn85xx_ts->screen_max_x = SCREEN_MAX_X;
    icn85xx_ts->screen_max_y = SCREEN_MAX_Y;
    //icn85xx_ts->irq = CTP_IRQ_PORT; //maybe need changed
    icn85xx_ts->irq = this_client->irq;
    /*#if defined(CONFIG_LCD_RK2926_V86)
    icn85xx_ts->revert_x_flag = 1;
    icn85xx_ts->revert_y_flag = 1;
    #endif
*/
#endif
#if SUPPORT_SPREADTRUM
    icn85xx_ts->screen_max_x = SCREEN_MAX_X;
    icn85xx_ts->screen_max_y = SCREEN_MAX_Y;
    icn85xx_ts->irq = CTP_IRQ_PORT;
#endif
#if SUPPORT_ACTION
    icn85xx_ts->screen_max_x = SCREEN_MAX_X;
    icn85xx_ts->screen_max_y = SCREEN_MAX_Y;
    icn85xx_ts->irq = CTP_IRQ_PORT;
#endif


    return err;

}

/***********************************************************************************************
Name    :   icn85xx_free_io_port
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
static int icn85xx_free_io_port(struct icn85xx_ts_data *icn85xx_ts)
{    
#if SUPPORT_ALLWINNER_A13    
    ctp_ops.free_platform_resource();
#endif

#if SUPPORT_ROCKCHIP

#endif

#if SUPPORT_SPREADTRUM

#endif    
}

/***********************************************************************************************
Name    :   icn85xx_request_irq
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
static int icn85xx_request_irq(struct icn85xx_ts_data *icn85xx_ts)
{
    int err = -1;
#if SUPPORT_ALLWINNER_A13

    err = ctp_ops.set_irq_mode("ctp_para", "ctp_int_port", CTP_IRQ_MODE);
    if(0 != err)
    {
        icn85xx_error("%s:ctp_ops.set_irq_mode err. \n", __func__);
        return err;
    }    
    err = request_irq(icn85xx_ts->irq, icn85xx_ts_interrupt, IRQF_TRIGGER_FALLING | IRQF_SHARED, "icn85xx_ts", icn85xx_ts);
    if (err < 0) 
    {
        icn85xx_error("icn85xx_ts_probe: request irq failed\n");
        return err;
    } 
    else
    {
        icn85xx_irq_disable();
        icn85xx_ts->use_irq = 1;        
    }
#endif

#if SUPPORT_ROCKCHIP

    err = gpio_request(icn85xx_ts->irq, "TS_INT"); //Request IO
    if (err < 0)
    {
        icn85xx_error("Failed to request GPIO:%d, ERRNO:%d\n", (int)icn85xx_ts->irq, err);
        return err;
    }
    gpio_direction_input(icn85xx_ts->irq);
    err = request_irq(icn85xx_ts->irq, icn85xx_ts_interrupt, IRQ_TYPE_EDGE_FALLING, "icn85xx_ts", icn85xx_ts);
    if (err < 0) 
    {
        icn85xx_error("icn85xx_ts_probe: request irq failed\n");
        return err;
    } 
    else
    {
        icn85xx_irq_disable();
        icn85xx_ts->use_irq = 1;        
    } 
#endif

#if SUPPORT_SPREADTRUM    
    eic_ctrl(EIC_ID_2, 1 , 1); 
    icn85xx_ts->irq = sprd_alloc_eic_irq(EIC_ID_2);
    err = request_irq(icn85xx_ts->irq, icn85xx_ts_interrupt, IRQF_TRIGGER_LOW | IRQF_DISABLED, "icn85xx_ts", icn85xx_ts);

    if (err < 0) 
    {
        icn85xx_error("icn85xx_ts_probe: request irq failed\n");
        return err;
    } 
    else
    {
        icn85xx_irq_disable();
        icn85xx_ts->use_irq = 1;        
    }    
#endif

#if SUPPORT_ACTION
    icn85xx_ts->irq = IRQ_SIRQ0;
    err = request_irq(icn85xx_ts->irq, icn85xx_ts_interrupt, IRQ_TYPE_EDGE_FALLING, "icn85xx_ts", icn85xx_ts);
    if (err < 0) 
    {
        icn85xx_error("icn85xx_ts_probe: request irq failed\n");
        return err;
    } 
    else
    {
        icn85xx_irq_disable();
        icn85xx_ts->use_irq = 1;        
    } 

#endif

    return 0;
}


/***********************************************************************************************
Name    :   icn85xx_free_irq
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
static void icn85xx_free_irq(struct icn85xx_ts_data *icn85xx_ts)
{
    if (icn85xx_ts) 
    {
        if (icn85xx_ts->use_irq)
        {
            free_irq(icn85xx_ts->irq, icn85xx_ts);
        }
        else
        {
            hrtimer_cancel(&icn85xx_ts->timer);
        }
    }
     
}

/***********************************************************************************************
Name    :   icn85xx_request_input_dev
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
static int icn85xx_request_input_dev(struct icn85xx_ts_data *icn85xx_ts)
{
    int ret = -1;    
    struct input_dev *input_dev;

    input_dev = input_allocate_device();
    if (!input_dev) {
        icn85xx_error("failed to allocate input device\n");
        return -ENOMEM;
    }
    icn85xx_ts->input_dev = input_dev;

    icn85xx_ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
#if CTP_REPORT_PROTOCOL
    __set_bit(INPUT_PROP_DIRECT, icn85xx_ts->input_dev->propbit);
    input_mt_init_slots(icn85xx_ts->input_dev, 5);
#else
    set_bit(ABS_MT_TOUCH_MAJOR, icn85xx_ts->input_dev->absbit);
    set_bit(ABS_MT_POSITION_X, icn85xx_ts->input_dev->absbit);
    set_bit(ABS_MT_POSITION_Y, icn85xx_ts->input_dev->absbit);
    set_bit(ABS_MT_WIDTH_MAJOR, icn85xx_ts->input_dev->absbit); 
#endif
    input_set_abs_params(icn85xx_ts->input_dev, ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
    input_set_abs_params(icn85xx_ts->input_dev, ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
    input_set_abs_params(icn85xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(icn85xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);  
    input_set_abs_params(icn85xx_ts->input_dev, ABS_MT_TRACKING_ID, 0, 255, 0, 0);

    __set_bit(KEY_MENU,  input_dev->keybit);
    __set_bit(KEY_BACK,  input_dev->keybit);
    __set_bit(KEY_HOME,  input_dev->keybit);
    __set_bit(KEY_SEARCH,  input_dev->keybit);

    input_dev->name = CTP_NAME;
    ret = input_register_device(input_dev);
    if (ret) {
        icn85xx_error("Register %s input device failed\n", input_dev->name);
        input_free_device(input_dev);
        return -ENODEV;        
    }
    
#ifdef CONFIG_HAS_EARLYSUSPEND
    icn85xx_trace("==register_early_suspend =\n");
    icn85xx_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    icn85xx_ts->early_suspend.suspend = icn85xx_ts_suspend;
    icn85xx_ts->early_suspend.resume  = icn85xx_ts_resume;
    register_early_suspend(&icn85xx_ts->early_suspend);
#endif

    return 0;
}

void icn85xx_ts_charger_mode()
{		
	icn85xx_write_reg(ICN83XX_REG_PMODE, 0x55);
}
EXPORT_SYMBOL_GPL(icn85xx_ts_charger_mode);

void icn85xx_ts_charger_mode_disable()
{
	icn85xx_write_reg(ICN83XX_REG_PMODE, 0x66);
}
EXPORT_SYMBOL_GPL(icn85xx_ts_charger_mode_disable);

static int icn85xx_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct icn85xx_ts_data *icn85xx_ts;
    short fwVersion = 0;
    short curVersion = 0;
    int err = 0;
    int retry;

    icn85xx_trace("====%s begin=====.  \n", __func__);
    
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
    {
        icn85xx_error("I2C check functionality failed.\n");
        return -ENODEV;
    }

    icn85xx_ts = kzalloc(sizeof(*icn85xx_ts), GFP_KERNEL);
    if (!icn85xx_ts)
    {
        icn85xx_error("Alloc icn85xx_ts memory failed.\n");
        return -ENOMEM;
    }
    memset(icn85xx_ts, 0, sizeof(*icn85xx_ts));

    this_client = client;
    this_client->addr = client->addr;
    i2c_set_clientdata(client, icn85xx_ts);

    icn85xx_ts->work_mode = 0;
    spin_lock_init(&icn85xx_ts->irq_lock);
//    icn85xx_ts->irq_lock = SPIN_LOCK_UNLOCKED;

#if SUPPORT_SPREADTRUM
    LDO_SetVoltLevel(LDO_LDO_SIM2, LDO_VOLT_LEVEL0);
    LDO_TurnOnLDO(LDO_LDO_SIM2);
    msleep(5);
    icn85xx_ts_reset();
#endif

    err = icn85xx_iic_test();
    if (err <= 0)
    {
        icn85xx_error("icn85xx_iic_test  failed.\n");
        return -1;
     
    }
    else
    {
        icn85xx_trace("iic communication ok: 0x%x\n", icn85xx_ts->ictype); 
    }

    err= icn85xx_request_input_dev(icn85xx_ts);
    if (err < 0)
    {
        icn85xx_error("request input dev failed\n");
        kfree(icn85xx_ts);
        return err;        
    }

    err = icn85xx_request_io_port(icn85xx_ts);
    if (err != 0)
    {
        icn85xx_error("icn85xx_request_io_port failed.\n");
        kfree(icn85xx_ts);
        return err;
    }

    if(icn85xx_ts->ictype == ICN85XX_WITHOUT_FLASH)
    {
        #if COMPILE_FW_WITH_DRIVER
            icn85xx_set_fw(sizeof(icn85xx_fw), &icn85xx_fw[0]);
        #endif    

        if(R_OK == icn85xx_fw_update(firmware))
        {
            icn85xx_ts->code_loaded_flag = 1;
            icn85xx_trace("ICN85XX_WITHOUT_FLASH, update ok\n"); 

        }
        else
        {
            icn85xx_ts->code_loaded_flag = 0;
            icn85xx_trace("ICN85XX_WITHOUT_FLASH, update error\n"); 
            return -1;
        }

    }
    else if(icn85xx_ts->ictype == ICN85XX_WITH_FLASH)
    {
        #if COMPILE_FW_WITH_DRIVER
            icn85xx_set_fw(sizeof(icn85xx_fw), &icn85xx_fw[0]);
        #endif
        
        fwVersion = icn85xx_read_fw_Ver(firmware);
        curVersion = icn85xx_readVersion();
        icn85xx_trace("fwVersion : 0x%x\n", fwVersion); 
        icn85xx_trace("current version: 0x%x\n", curVersion);  

    if(board_boot_mode() == BOOT_MODE_RECOVERY)
    {
        printk("BOOT_MODE_RECOVERY !  force update fw\n");
        retry = 5;
        while(retry > 0)
        {
		    if(icn85xx_goto_progmode() != 0)
		    {
				printk("icn85xx_goto_progmode() != 0 error\n");
		        return -1; 
		    } 
        	icn85xx_read_flashid();
            printk("begin to update\n");
            if(R_OK == icn85xx_fw_update(firmware))
            {
                break;
            }
            retry--;
            icn85xx_error("icn85xx_fw_update failed.\n");        
        }
    }

#if FORCE_UPDATA_FW
        retry = 5;
        while(retry > 0)
        {
		    if(icn85xx_goto_progmode() != 0)
		    {
				printk("icn85xx_goto_progmode() != 0 error\n");
		        return -1; 
		    } 
        	icn85xx_read_flashid();
            printk("begin to update\n");
            if(R_OK == icn85xx_fw_update(firmware))
            {
                break;
            }
            retry--;
            icn85xx_error("icn85xx_fw_update failed.\n");        
        }

#else
        if(fwVersion != curVersion)
        {
            retry = 5;
            while(retry > 0)
            {
                if(R_OK == icn85xx_fw_update(firmware))
                {
                    break;
                }
                retry--;
                icn85xx_error("icn85xx_fw_update failed.\n");   
            }
        }
#endif
    }

#if TOUCH_VIRTUAL_KEYS
    icn85xx_ts_virtual_keys_init();
#endif
    err = icn85xx_request_irq(icn85xx_ts);
    if (err != 0)
    {
        icn85xx_error("request irq error, use timer\n");
        icn85xx_ts->use_irq = 0;
        hrtimer_init(&icn85xx_ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        icn85xx_ts->timer.function = chipone_timer_func;
        hrtimer_start(&icn85xx_ts->timer, ktime_set(CTP_START_TIMER/1000, (CTP_START_TIMER%1000)*1000000), HRTIMER_MODE_REL);
    }
#if SUPPORT_SYSFS
    icn85xx_create_sysfs(client);
#endif

#if SUPPORT_PROC_FS
    sema_init(&icn85xx_ts->sem, 1);
    init_proc_node();
#endif

    INIT_WORK(&icn85xx_ts->pen_event_work, icn85xx_ts_pen_irq_work);
    icn85xx_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
    if (!icn85xx_ts->ts_workqueue) {
        icn85xx_error("create_singlethread_workqueue failed.\n");
        kfree(icn85xx_ts);
        return -ESRCH;
    }

    if(icn85xx_ts->use_irq)
        icn85xx_irq_enable();

#if SUPPORT_CHARGER_DETECTER
    #if SUPPORT_ACTION
    if(atc260x_get_charge_status() == 1) 
	{
//		printk("atc260x_get_charge_status====1\n");
		icn85xx_ts_charger_mode();
	}
	else 
	{
		icn85xx_ts_charger_mode_disable();
	}
	
	void callback(int status)
	{
//		printk("CALLBACK TP %d !!!!!!!!!\n",status);
		if(status ==1) 
		{
			icn85xx_ts_charger_mode();
		}
		else
		{
			icn85xx_ts_charger_mode_disable();
		}
	}
	atc260x_set_adjust_tp_para(callback); 
    #endif
#endif
    
    icn85xx_trace("==%s over =\n", __func__);    
    return 0;
}

static int __devexit icn85xx_ts_remove(struct i2c_client *client)
{
    struct icn85xx_ts_data *icn85xx_ts = i2c_get_clientdata(client);  
    icn85xx_trace("==icn85xx_ts_remove=\n");
    if(icn85xx_ts->use_irq)
        icn85xx_irq_disable();
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&icn85xx_ts->early_suspend);
#endif

#if SUPPORT_PROC_FS
    uninit_proc_node();
#endif

#if SUPPORT_SYSFS
    icn85xx_remove_sysfs(client);
#endif
    input_unregister_device(icn85xx_ts->input_dev);
    input_free_device(icn85xx_ts->input_dev);
    cancel_work_sync(&icn85xx_ts->pen_event_work);
    destroy_workqueue(icn85xx_ts->ts_workqueue);
    icn85xx_free_irq(icn85xx_ts);
    icn85xx_free_io_port(icn85xx_ts);
    kfree(icn85xx_ts);    
    i2c_set_clientdata(client, NULL);
    return 0;
}

static const struct i2c_device_id icn85xx_ts_id[] = {
    { CTP_NAME, 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, icn85xx_ts_id);

static struct i2c_driver icn85xx_ts_driver = {
    .class      = I2C_CLASS_HWMON,
    .probe      = icn85xx_ts_probe,
    .remove     = __devexit_p(icn85xx_ts_remove),
#ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend    = icn85xx_ts_suspend,
    .resume     = icn85xx_ts_resume,
#endif
    .id_table   = icn85xx_ts_id,
    .driver = {
        .name   = CTP_NAME,
        .owner  = THIS_MODULE,
    },
#if SUPPORT_ALLWINNER_A13    
    .address_list   = u_i2c_addr.normal_i2c,
#endif    
};

#if SUPPORT_ACTION

static struct i2c_board_info tp_info = {
    .type	= CTP_NAME,
    .addr	=  0x48,
};
////POWER

static inline void regulator_deinit(struct regulator *power)
{
    regulator_disable(power);
    
    regulator_put(power);
}

static struct regulator *regulator_init(const char *name, int minvol, int maxvol)
{

    struct regulator *power;

    power = regulator_get(NULL,"sensor28");// name);
    if (IS_ERR(power)) {
//        printk("Nova err,regulator_get fail\n!!!");
        return NULL;
    }

    if (regulator_set_voltage(power, minvol, maxvol)) {
//        printk("Nova err,cannot set voltage\n!!!");
        regulator_put(power);
        return NULL;
    }
    
    regulator_enable(power);
    return (power);
}

static int icn85xx_hw_init(void)
{
    int err = 0;
	err = gpio_request(CTP_RST_PORT, CTP_NAME);
	if ( err ) {
		err = -EINVAL;
		return err;
	}
	err = gpio_direction_output(CTP_RST_PORT, 1);
	if ( err ) {
		err = -EINVAL;
		return err;
    }
    
    if(gpio_request(POWER_PORT,NULL) != 0){
	    gpio_free(POWER_PORT);
	    printk("zhongchu_init_platform_hw gpio_request error\n");
	    return -EIO;
    }
    gpio_direction_output(POWER_PORT, 0);
    gpio_set_value(POWER_PORT,GPIO_LOW);
    msleep(20);

 	tp_regulator = regulator_init(CTP_POWER_ID, 
		CTP_POWER_MIN_VOL, CTP_POWER_MAX_VOL);
	if ( !tp_regulator ) {
//		printk(KERN_ERR "icn85xx tp init power failed");
		err = -EINVAL;
		return err;
	}
    
    msleep(100);

    gpio_set_value_cansleep(CTP_RST_PORT, 0);
	msleep(100);
	gpio_set_value_cansleep(CTP_RST_PORT, 1);
    msleep(300);
        
    return err;
}
#else
static int icn85xx_hw_init(void)
{
    int err = 0;
	err = gpio_request(CTP_RST_PORT, CTP_NAME);
	if ( err ) {
		err = -EINVAL;
		return err;
	}
	err = gpio_direction_output(CTP_RST_PORT, 1);
	if ( err ) {
		err = -EINVAL;
		return err;
    }
    
    if(gpio_request(POWER_PORT,NULL) != 0){
	    gpio_free(POWER_PORT);
	    printk("zhongchu_init_platform_hw gpio_request error\n");
	    return -EIO;
    }
    gpio_direction_output(POWER_PORT, 0);
    gpio_set_value(POWER_PORT,GPIO_LOW);
    msleep(20);

    gpio_set_value_cansleep(CTP_RST_PORT, 0);
	msleep(100);
	gpio_set_value_cansleep(CTP_RST_PORT, 1);
    msleep(300);
    
    return err;
}
#endif

static int __init icn85xx_ts_init(void)
{ 
    int ret = -1;
    icn85xx_trace("===========================%s=====================\n", __func__);
    
#if SUPPORT_ALLWINNER_A13
    if (ctp_ops.fetch_sysconfig_para)
    {
        if(ctp_ops.fetch_sysconfig_para()){
            pr_info("%s: err.\n", __func__);
            return -1;
        }
    }
    icn85xx_trace("%s: after fetch_sysconfig_para:  normal_i2c: 0x%hx. normal_i2c[1]: 0x%hx \n", \
    __func__, u_i2c_addr.normal_i2c[0], u_i2c_addr.normal_i2c[1]);

    ret = ctp_ops.init_platform_resource();
    if(0 != ret){
        icn85xx_error("%s:ctp_ops.init_platform_resource err. \n", __func__);    
    }
    //reset
    ctp_ops.ts_reset();
    //wakeup
    ctp_ops.ts_wakeup();      
    icn85xx_ts_driver.detect = ctp_ops.ts_detect;
#endif     

#if SUPPORT_ACTION
    icn85xx_hw_init();
	struct i2c_adapter *adap = i2c_get_adapter(1);
    //printk(KERN_ERR "==icn85xx_init %d!!!!!!!!\n",__LINE__);
	i2c_new_device(adap, &tp_info); 
#else
    icn85xx_hw_init();
#endif

    ret = i2c_add_driver(&icn85xx_ts_driver);
    return ret;
}

static void __exit icn85xx_ts_exit(void)
{
    icn85xx_trace("==icn85xx_ts_exit==\n");
    i2c_del_driver(&icn85xx_ts_driver);
}

module_init(icn85xx_ts_init);
module_exit(icn85xx_ts_exit);

MODULE_AUTHOR("<zmtian@chiponeic.com>");
MODULE_DESCRIPTION("Chipone icn85xx TouchScreen driver");
MODULE_LICENSE("GPL");
