/*++
 
 Copyright (c) 2012-2022 ChipOne Technology (Beijing) Co., Ltd. All Rights Reserved.
 This PROPRIETARY SOFTWARE is the property of ChipOne Technology (Beijing) Co., Ltd. 
 and may contains trade secrets and/or other confidential information of ChipOne 
 Technology (Beijing) Co., Ltd. This file shall not be disclosed to any third party,
 in whole or in part, without prior written consent of ChipOne.  
 THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. CHIPONE DISCLAIMS ALL EXPRESS OR 
 IMPLIED WARRANTIES.  
 
 File Name:    icn838x_iic_ts.c
 Abstract:
               input driver.
 Author:       Zhimin Tian
 Date :        01,17,2013
 Version:      1.0
 History :
     2012,10,30, V0.1 first version  
 --*/

#include "icn838x_iic_ts.h"
#include "icn838x_slave.h"
#include "AllInclude.h"

   static struct i2c_client *this_client;
extern U8 u8ResFlag;   //from lib
extern int tp_screen_type ;
// add by  ruan
static int is_timer_running = 0;
struct icn83xx_ts_hw_data { 
	int reset_gpio;
	int touch_en_gpio;
	int (*init_platform_hw)(void);
	int revert_x;
	int revert_y;
	int revert_xy;
	int screen_max_x;
	int screen_max_y;
};
static struct icn83xx_ts_hw_data * hw_pdata=NULL;
static int tp_icn838x_dbg_level = 0;
module_param_named(dbg_level, tp_icn838x_dbg_level, int, 0644);
#define tchip_dbg_pr( args...) \
	do { \
		if (tp_icn838x_dbg_level) { \
			printk(args); \
		} \
	} while (0)
// end add

#if defined(CONFIG_TCHIP_MACH_TR7088)
#define POWER_PORT          RK30_PIN1_PB5
#else
#define POWER_PORT          INVALID_GPIO
#endif

#if SUPPORT_SYSFS
static enum hrtimer_restart chipone_timer_func(struct hrtimer *timer);

static ssize_t icn83xx_show_update(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t icn83xx_store_update(struct device* cd, struct device_attribute *attr, const char* buf, size_t len);
static ssize_t icn83xx_show_process(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t icn83xx_store_process(struct device* cd, struct device_attribute *attr,const char* buf, size_t len);

static DEVICE_ATTR(update, S_IRUGO | S_IWUSR, icn83xx_show_update, icn83xx_store_update);
static DEVICE_ATTR(process, S_IRUGO | S_IWUSR, icn83xx_show_process, icn83xx_store_process);

static ssize_t icn83xx_show_process(struct device* cd,struct device_attribute *attr, char* buf)
{
    ssize_t ret = 0;
    sprintf(buf, "icn83xx process\n");
    ret = strlen(buf) + 1;
    return ret;
}

static ssize_t icn83xx_store_process(struct device* cd, struct device_attribute *attr,
               const char* buf, size_t len)
{
    struct icn83xx_ts_data *icn83xx_ts = i2c_get_clientdata(this_client);
    unsigned long on_off = simple_strtoul(buf, NULL, 10); 
    if(on_off == 0)
    {
        icn83xx_ts->work_mode = on_off;
    }
    else if((on_off == 1) || (on_off == 2))
    {
        icn83xx_ts->work_mode = on_off;
    }
    return len;
}

static char update_cmd = UPDATE_CMD_NONE;
static unsigned short reg_addr = 0;
static char reg_value = 0;
static int update_para_error = UPDATE_PARA_OK;

static ssize_t icn83xx_show_update(struct device* cd,
                     struct device_attribute *attr, char* buf)
{
    ssize_t ret = 0;
    sprintf(buf, "%02x:%04x:%02x\n",update_para_error,reg_addr,reg_value);
    ret = strlen(buf) + 1;
    return ret;

}

static ssize_t icn83xx_store_update(struct device* cd, struct device_attribute *attr, const char* buf, size_t len)
{
    struct icn83xx_ts_data *icn83xx_ts = i2c_get_clientdata(this_client);
    int i=0,j=0;
    char para_state = PARA_STATE_CMD;
    char cmd[32] = {0};
    char addr[32] = {0};
    char value[32] = {0};
    char * p;
    p = cmd;
    update_para_error = UPDATE_PARA_OK;
    //analyze para
    for(i=0; i<len; i++)
    {
        if(buf[i] == ':')
        {
            if(PARA_STATE_CMD == para_state)
            {
                p[j] = '\0';
                para_state = PARA_STATE_ADDR;
                p = addr;
                j = 0;
            }
            else if(PARA_STATE_ADDR == para_state)
            {
                p[j] = '\0';
                para_state = PARA_STATE_DATA;
                p = value;
                j = 0;
            }
        }
        else
        {
            p[j++] =  buf[i];
        }
    }
    p[j] = '\0';
    //check para
    update_cmd = simple_strtoul(cmd, NULL, 16);
    reg_addr = simple_strtoul(addr, NULL, 16);
    reg_value = simple_strtoul(value, NULL, 16);

    if(UPDATE_CMD_WRITE_REG == update_cmd)
    {
        if((reg_addr < 0x10000) && (reg_value < 0x100))
        {
            icn83xx_ts->work_mode = 2;            
        }
        else
        {
            update_para_error = UPDATE_PARA_OUT_RANGE;
            icn83xx_ts->work_mode = 0;
            
        }
    }
    else if(UPDATE_CMD_READ_REG == update_cmd)
    {
        if(reg_addr == 0xffff)
        {
            g_updatebase = TRUE;
        }
        
        if((reg_addr < 0x10000))
        {
            icn83xx_ts->work_mode = 2;
        }
        else
        {
            update_para_error = UPDATE_PARA_OUT_RANGE;
            icn83xx_ts->work_mode = 0;
        }
        
    }
    else
    {
        update_para_error = UPDATE_PARA_UNDEFINE_CMD;
        icn83xx_ts->work_mode = 0;
    }
    return len;
   
}

static int icn83xx_create_sysfs(struct i2c_client *client)
{
    int err;
    struct device *dev = &(client->dev);
    icn83xx_trace("%s: \n",__func__);    
    err = device_create_file(dev, &dev_attr_update);
    err = device_create_file(dev, &dev_attr_process);
    return err;
}

static void icn83xx_remove_sysfs(struct i2c_client *client)
{
    struct device *dev = &(client->dev);
    icn83xx_trace("%s: \n",__func__);    
    device_remove_file(dev, &dev_attr_update);
    device_remove_file(dev, &dev_attr_process);
}

#endif

#if SUPPORT_PROC_FS

pack_head cmd_head;
static struct proc_dir_entry *icn83xx_proc_entry;
int  DATA_LENGTH = 0;
static int icn83xx_tool_write(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
    int ret = 0;
    STRUCT_PANEL_PARA *sPtr;
    struct icn83xx_ts_data *icn83xx_ts = i2c_get_clientdata(this_client);
    proc_info("%s \n",__func__);  
    if(down_interruptible(&icn83xx_ts->sem))  
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
        sPtr = TP_GetPanelPara(); 
        ret = copy_from_user(&cmd_head.data[0], &buff[CMD_HEAD_LENGTH], cmd_head.data_len);
        if(ret)
        {
            proc_error("copy_from_user failed.\n");
            goto write_out;
        }
        //need copy to g_structPanelPara
        if(cmd_head.data_len != sizeof(STRUCT_PANEL_PARA))
        {
            proc_error("write para length error.\n");
            goto write_out;
        }
        else
        {
            memcpy(sPtr, &cmd_head.data[0], cmd_head.data_len);
            proc_info("sPtr->u8RowNum :  %d\n", sPtr->u8RowNum);
            proc_info("sPtr->u8ColNum :  %d\n", sPtr->u8ColNum);
            proc_info("sPtr->u8TXOrder[0] :  %d\n", sPtr->u8TXOrder[0]);
            proc_info("sPtr->u8RXOrder[0] :  %d\n", sPtr->u8RXOrder[0]);
            proc_info("sPtr->u8TxVol :  %d\n", sPtr->u8TxVol);
            proc_info("sPtr->u8TransMode :  %d\n", sPtr->u8TransMode);
            proc_info("sPtr->s16RCULimitUp :  %d\n", sPtr->s16RCULimitUp);
            proc_info("sPtr->s16RCULimitDn :  %d\n", sPtr->s16RCULimitDn);
           
        }
        ret = cmd_head.data_len + CMD_HEAD_LENGTH;
        icn83xx_ts->work_mode = 1; //reinit
        goto write_out;

    }
    else if(3 == cmd_head.wr)   //set workmode
    {

    }
    else if(5 == cmd_head.wr)
    {        
   
    }
    else if(7 == cmd_head.wr)
    { 
       
    }

write_out:
    up(&icn83xx_ts->sem); 
    return len;
    
}
static int icn83xx_tool_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
    int i;
    int ret = 0;
    char *p_RawData;
    STRUCT_PANEL_PARA *sPtr;
    struct icn83xx_ts_data *icn83xx_ts = i2c_get_clientdata(this_client);
    if(down_interruptible(&icn83xx_ts->sem))  
    {  
        return -1;   
    }     
    proc_info("%s: count:%d, off:%d, cmd_head.data_len: %d\n",__func__, count, (int)off, cmd_head.data_len); 
    if (cmd_head.wr % 2)
    {
        ret = 0;
        goto read_out;
    }
    else if (0 == cmd_head.wr)   //read para
    {
        sPtr = TP_GetPanelPara(); 
        memcpy(&page[0], sPtr, cmd_head.data_len);
        goto read_out;

    }
    else if(2 == cmd_head.wr)  //read IC type
    {
        page[0] = g_structSystemStatus.u8IcVersionMain;
        page[1] = g_structSystemStatus.u8IcVersionSub;
        page[2] = g_structSystemStatus.u8FirmWareMainVersion;
        page[3] = g_structSystemStatus.u8FirmWareSubVersion;

    }
    else if(4 == cmd_head.wr)  //read rawdata
    {
        //mdelay(cmd_head.times);
        p_RawData = GetRawdata();
        memcpy(&page[0], p_RawData, cmd_head.data_len);
        goto read_out;        
    }
    else if(6 == cmd_head.wr)  //read diffdata
    {   
        //mdelay(cmd_head.times);
        p_RawData = GetDiffdata();
        memcpy(&page[0], p_RawData, cmd_head.data_len);
        goto read_out;          
    }
    else if(8 == cmd_head.wr)  //change TxVol, read diff
    {   
        sPtr = TP_GetPanelPara(); 
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
        TP_NormalWork();   
        goto read_out;          
    }    
read_out:
    up(&icn83xx_ts->sem);   
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
        icn83xx_trace("alloc memory size:%d.\n", DATA_LENGTH);
    }
    else
    {
        proc_error("alloc for memory failed.\n");
        return 0;
    }

    icn83xx_proc_entry = create_proc_entry(ICN83XX_ENTRY_NAME, 0666, NULL);
    if (icn83xx_proc_entry == NULL)
    {
        proc_error("Couldn't create proc entry!\n");
        return 0;
    }
    else
    {
        icn83xx_trace("Create proc entry success!\n");
        icn83xx_proc_entry->write_proc = icn83xx_tool_write;
        icn83xx_proc_entry->read_proc = icn83xx_tool_read;
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

static void icn83xx_ts_virtual_keys_init(void)
{
    int ret;
    struct kobject *properties_kobj;      
    properties_kobj = kobject_create_and_add("board_properties", NULL);
    if (properties_kobj)
        ret = sysfs_create_group(properties_kobj,
                     &properties_attr_group);
    if (!properties_kobj || ret)
        icn83xx_error("failed to create board_properties\n");    
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
        icn83xx_info("pen down. \n");
    }else{ //touch panel is free up
        state = FREE_UP;
        icn83xx_info("free up. \n");
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
        icn83xx_info("==CTP_IRQ_NO=\n");              
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
    icn83xx_trace("%s: config gpio to int mode. \n", __func__);
#ifndef SYSCONFIG_GPIO_ENABLE
#else
    if(gpio_int_hdle){
        gpio_release(gpio_int_hdle, 2);
    }
    gpio_int_hdle = gpio_request_ex(major_key, subkey);
    if(!gpio_int_hdle){
        icn83xx_error("request tp_int_port failed. \n");
        ret = -1;
        goto request_tp_int_port_failed;
    }
    gpio_get_one_pin_status(gpio_int_hdle, gpio_int_info, subkey, 1);
    icn83xx_trace("%s, %d: gpio_int_info, port = %d, port_num = %d. \n", __func__, __LINE__, \
        gpio_int_info[0].port, gpio_int_info[0].port_num);
#endif

#ifdef AW_GPIO_INT_API_ENABLE

#else
    icn83xx_trace(" INTERRUPT CONFIG\n");
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
    icn83xx_trace("%s: config gpio to io mode. \n", __func__);
#ifndef SYSCONFIG_GPIO_ENABLE
#else
    if(gpio_int_hdle){
        gpio_release(gpio_int_hdle, 2);
    }
    gpio_int_hdle = gpio_request_ex("ctp_para", "ctp_io_port");
    if(!gpio_int_hdle){
        icn83xx_error("request ctp_io_port failed. \n");
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
        icn83xx_error("%s: tp_wakeup request gpio fail!\n", __func__);
        gpio_wakeup_enable = 0;
    }

    gpio_reset_hdle = gpio_request_ex("ctp_para", "ctp_reset");
    if(!gpio_reset_hdle) {
        icn83xx_error("%s: tp_reset request gpio fail!\n", __func__);
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

    icn83xx_trace("%s. \n", __func__);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_used", &ctp_used, 1)){
        icn83xx_error("%s: script_parser_fetch err. \n", __func__);
        goto script_parser_fetch_err;
    }
    if(1 != ctp_used){
        icn83xx_error("%s: ctp_unused. \n",  __func__);
        //ret = 1;
        return ret;
    }

    if(SCRIPT_PARSER_OK != script_parser_fetch_ex("ctp_para", "ctp_name", (int *)(&name), &type, sizeof(name)/sizeof(int))){
        icn83xx_error("%s: script_parser_fetch err. \n", __func__);
        goto script_parser_fetch_err;
    }
    if(strcmp(CTP_NAME, name)){
        icn83xx_error("%s: name %s does not match CTP_NAME. \n", __func__, name);
        icn83xx_error(CTP_NAME);
        //ret = 1;
        return ret;
    }

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_twi_addr", &twi_addr, sizeof(twi_addr)/sizeof(__u32))){
        icn83xx_error("%s: script_parser_fetch err. \n", name);
        goto script_parser_fetch_err;
    }
    //big-endian or small-endian?
    //pr_info("%s: before: ctp_twi_addr is 0x%x, dirty_addr_buf: 0x%hx. dirty_addr_buf[1]: 0x%hx \n", __func__, twi_addr, u_i2c_addr.dirty_addr_buf[0], u_i2c_addr.dirty_addr_buf[1]);
    u_i2c_addr.dirty_addr_buf[0] = twi_addr;
    u_i2c_addr.dirty_addr_buf[1] = I2C_CLIENT_END;
    icn83xx_trace("%s: after: ctp_twi_addr is 0x%x, dirty_addr_buf[0]: 0x%hx. dirty_addr_buf[1]: 0x%hx \n", __func__, twi_addr, u_i2c_addr.dirty_addr_buf[0], u_i2c_addr.dirty_addr_buf[1]);
    //pr_info("%s: after: ctp_twi_addr is 0x%x, u32_dirty_addr_buf: 0x%hx. u32_dirty_addr_buf[1]: 0x%hx \n", __func__, twi_addr, u32_dirty_addr_buf[0],u32_dirty_addr_buf[1]);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_twi_id", &twi_id, sizeof(twi_id)/sizeof(__u32))){
        icn83xx_error("%s: script_parser_fetch err. \n", name);
        goto script_parser_fetch_err;
    }
    icn83xx_trace("%s: ctp_twi_id is %d. \n", __func__, twi_id);
    
    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_screen_max_x", &screen_max_x, 1)){
        icn83xx_error("%s: script_parser_fetch err. \n", __func__);
        goto script_parser_fetch_err;
    }
    icn83xx_trace("%s: screen_max_x = %d. \n", __func__, screen_max_x);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_screen_max_y", &screen_max_y, 1)){
        icn83xx_error("%s: script_parser_fetch err. \n", __func__);
        goto script_parser_fetch_err;
    }
    icn83xx_trace("%s: screen_max_y = %d. \n", __func__, screen_max_y);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_revert_x_flag", &revert_x_flag, 1)){
        icn83xx_error("%s: script_parser_fetch err. \n", __func__);
        goto script_parser_fetch_err;
    }
    icn83xx_trace("%s: revert_x_flag = %d. \n", __func__, revert_x_flag);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_revert_y_flag", &revert_y_flag, 1)){
        icn83xx_error("%s: script_parser_fetch err. \n", __func__);
        goto script_parser_fetch_err;
    }
    icn83xx_trace("%s: revert_y_flag = %d. \n", __func__, revert_y_flag);

    if(SCRIPT_PARSER_OK != script_parser_fetch("ctp_para", "ctp_exchange_x_y_flag", &exchange_x_y_flag, 1)){
        icn83xx_error("%s: script_parser_fetch err. \n", __func__);
        goto script_parser_fetch_err;
    }
    icn83xx_trace("%s: exchange_x_y_flag = %d. \n", __func__, exchange_x_y_flag);

    return 0;

script_parser_fetch_err:
    icn83xx_trace("=========script_parser_fetch_err============\n");
    return ret;
}

/**
 * ctp_reset - function
 *
 */
static void ctp_reset(void)
{
    if(gpio_reset_enable){
        icn83xx_trace("%s. \n", __func__);
        if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_reset_hdle, 0, "ctp_reset")){
            icn83xx_error("%s: err when operate gpio. \n", __func__);
        }
        mdelay(CTP_RESET_LOW_PERIOD);
        if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_reset_hdle, 1, "ctp_reset")){
            icn83xx_error("%s: err when operate gpio. \n", __func__);
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
        icn83xx_trace("%s. \n", __func__);
        if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 0, "ctp_wakeup")){
            icn83xx_error("%s: err when operate gpio. \n", __func__);
        }
        mdelay(CTP_WAKEUP_LOW_PERIOD);
        if(EGPIO_SUCCESS != gpio_write_one_pin_value(gpio_wakeup_hdle, 1, "ctp_wakeup")){
            icn83xx_error("%s: err when operate gpio. \n", __func__);
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
        icn83xx_trace("%s: Detected chip %s at adapter %d, address 0x%02x\n",
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
Name    :   icn83xx_ts_wakeup 
Input   :   void
Output  :   ret
function    : this function is used to wakeup tp
***********************************************************************************************/
void icn83xx_ts_wakeup(void)
{
#if SUPPORT_ALLWINNER_A13
//    ctp_ops.ts_wakeup();
#endif    

}

/***********************************************************************************************
Name    :   icn83xx_ts_reset 
Input   :   void
Output  :   ret
function    : this function is used to reset tp, you should not delete it
***********************************************************************************************/
void icn83xx_ts_reset(void)
{
    //set reset func
#if SUPPORT_ALLWINNER_A13
//    ctp_ops.ts_reset();    
#endif

#if SUPPORT_ROCKCHIP
    if( hw_pdata && hw_pdata->reset_gpio){
    	gpio_direction_output(hw_pdata->reset_gpio, 1);
    	gpio_set_value(hw_pdata->reset_gpio, 0);
    	mdelay(CTP_RESET_LOW_PERIOD);
    	gpio_set_value(hw_pdata->reset_gpio,1);
   	 mdelay(CTP_RESET_HIGH_PERIOD);
    }
#endif

#if SUPPORT_SPREADTRUM
    gpio_direction_output(sprd_3rdparty_gpio_tp_rst, 1);
    gpio_set_value(sprd_3rdparty_gpio_tp_rst, 0);
    mdelay(CTP_RESET_LOW_PERIOD);
    gpio_set_value(sprd_3rdparty_gpio_tp_rst,1);
    mdelay(CTP_RESET_HIGH_PERIOD);
#endif    
}

/***********************************************************************************************
Name    :   icn83xx_prog_i2c_rxdata 
Input   :   addr
            *rxdata
            length
Output  :   ret
function    : read data from icn83xx, prog mode 
***********************************************************************************************/
int icn83xx_prog_i2c_rxdata(unsigned short addr, char *rxdata, int length)
{
    int ret = -1;
    int retries = 0;
#if 0   
    struct i2c_msg msgs[] = {   
        {
            .addr   = ICN83XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = I2C_M_RD,
            .len    = length,
            .buf    = rxdata,
#if SUPPORT_ROCKCHIP            
            .scl_rate = ICN83XX_I2C_SCL,
#endif            
        },
    };
        
    icn83xx_prog_i2c_txdata(addr, NULL, 0);
    while(retries < IIC_RETRY_NUM)
    {    
        ret = i2c_transfer(this_client->adapter, msgs, 1);
        if(ret == 1)break;
        retries++;
    }
    if (retries >= IIC_RETRY_NUM)
    {
        icn83xx_error("%s i2c read error: %d\n", __func__, ret); 
        icn83xx_ts_reset();
    }    
#else
    unsigned char tmp_buf[2];
    struct i2c_msg msgs[] = {
        {
            .addr   = ICN83XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = 0,
            .len    = 2,
            .buf    = tmp_buf,
#if SUPPORT_ROCKCHIP            
            .scl_rate = ICN83XX_I2C_SCL,
#endif            
        },
        {
            .addr   = ICN83XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = I2C_M_RD,
            .len    = length,
            .buf    = rxdata,
#if SUPPORT_ROCKCHIP            
            .scl_rate = ICN83XX_I2C_SCL,
#endif            
        },
    };
    tmp_buf[0] = U16HIBYTE(addr);
    tmp_buf[1] = U16LOBYTE(addr);  

    while(retries < IIC_RETRY_NUM)
    {
        ret = i2c_transfer(this_client->adapter, msgs, 2);
        if(ret == 2)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn83xx_error("%s i2c read error: %d\n", __func__, ret); 
//        icn83xx_ts_reset();
       u8ResFlag = 0x5a;
    }
#endif      
    return ret;
}
/***********************************************************************************************
Name    :   icn83xx_prog_i2c_txdata 
Input   :   addr
            *rxdata
            length
Output  :   ret
function    : send data to icn83xx , prog mode
***********************************************************************************************/
int icn83xx_prog_i2c_txdata(unsigned short addr, char *txdata, int length)
{
    int ret = -1;
    char tmp_buf[128];
    int retries = 0; 
    struct i2c_msg msg[] = {
        {
            .addr   = ICN83XX_PROG_IIC_ADDR,//this_client->addr,
            .flags  = 0,
            .len    = length + 2,
            .buf    = tmp_buf,
#if SUPPORT_ROCKCHIP            
            .scl_rate = ICN83XX_I2C_SCL,
#endif            
        },
    };
    
    if (length > 125)
    {
        icn83xx_error("%s too big datalen = %d!\n", __func__, length);
        return -1;
    }
    
    tmp_buf[0] = U16HIBYTE(addr);
    tmp_buf[1] = U16LOBYTE(addr);

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
        icn83xx_error("%s i2c write error: %d\n", __func__, ret); 
//        icn83xx_ts_reset();
        u8ResFlag = 0x5a;
    }
    return ret;
}
/***********************************************************************************************
Name    :   icn83xx_prog_write_reg
Input   :   addr -- address
            para -- parameter
Output  :   
function    :   write register of icn83xx, prog mode
***********************************************************************************************/
int icn83xx_prog_write_reg(unsigned short addr, char para)
{
    char buf[3];
    int ret = -1;

    buf[0] = para;
    ret = icn83xx_prog_i2c_txdata(addr, buf, 1);
    if (ret < 0) {
        icn83xx_error("write reg failed! %#x ret: %d\n", buf[0], ret);
        return -1;
    }
    
    return ret;
}


/***********************************************************************************************
Name    :   icn83xx_prog_read_reg 
Input   :   addr
            pdata
Output  :   
function    :   read register of icn83xx, prog mode
***********************************************************************************************/
int icn83xx_prog_read_reg(unsigned short addr, char *pdata)
{
    int ret = -1;
    ret = icn83xx_prog_i2c_rxdata(addr, pdata, 1);  
    return ret;    
}

/***********************************************************************************************
Name    :   icn83xx_i2c_rxdata 
Input   :   addr
            *rxdata
            length
Output  :   ret
function    : read data from icn83xx, normal mode   
***********************************************************************************************/
int icn83xx_i2c_rxdata(unsigned char addr, char *rxdata, int length)
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
            .scl_rate = ICN83XX_I2C_SCL,
#endif            
        },
    };
        
    icn83xx_i2c_txdata(addr, NULL, 0);
    while(retries < IIC_RETRY_NUM)
    {

        ret = i2c_transfer(this_client->adapter, msgs, 1);
        if(ret == 1)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn83xx_error("%s i2c read error: %d\n", __func__, ret); 
        icn83xx_ts_reset();
    }

#else
    unsigned char tmp_buf[1];
    struct i2c_msg msgs[] = {
        {
            .addr   = (0x80>>1), //this_client->addr,
            .flags  = 0,
            .len    = 1,
            .buf    = tmp_buf,
#if SUPPORT_ROCKCHIP            
            .scl_rate = ICN83XX_I2C_SCL,
#endif            
        },
        {
            .addr   = this_client->addr,
            .flags  = I2C_M_RD,
            .len    = length,
            .buf    = rxdata,
#if SUPPORT_ROCKCHIP            
            .scl_rate = ICN83XX_I2C_SCL,
#endif
        },
    };
    tmp_buf[0] = addr; 
    
    while(retries < IIC_RETRY_NUM)
    {
        ret = i2c_transfer(this_client->adapter, msgs, 2);
        if(ret == 2)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn83xx_error("%s i2c read error: %d\n", __func__, ret); 
//        icn83xx_ts_reset();
    }    
#endif

    return ret;
}
/***********************************************************************************************
Name    :   icn83xx_i2c_txdata 
Input   :   addr
            *rxdata
            length
Output  :   ret
function    : send data to icn83xx , normal mode
***********************************************************************************************/
int icn83xx_i2c_txdata(unsigned char addr, char *txdata, int length)
{
    int ret = -1;
    unsigned char tmp_buf[128];
    int retries = 0;

    struct i2c_msg msg[] = {
        {
            .addr   = (0x80>>1), //this_client->addr,
            .flags  = 0,
            .len    = length + 1,
            .buf    = tmp_buf,
#if SUPPORT_ROCKCHIP             
            .scl_rate = ICN83XX_I2C_SCL,
#endif            
        },
    };
    
    if (length > 125)
    {
        icn83xx_error("%s too big datalen = %d!\n", __func__, length);
        return -1;
    }
    
    tmp_buf[0] = addr;

    if (length != 0 && txdata != NULL)
    {
        memcpy(&tmp_buf[1], txdata, length);
    }   
    
    while(retries < IIC_RETRY_NUM)
    {
        ret = i2c_transfer(this_client->adapter, msg, 1);
        if(ret == 1)break;
        retries++;
    }

    if (retries >= IIC_RETRY_NUM)
    {
        icn83xx_error("%s i2c write error: %d\n", __func__, ret); 
//        icn83xx_ts_reset();
    }

    return ret;
}


/***********************************************************************************************
Name    :   icn83xx_iic_test 
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
static int icn83xx_iic_test(void)
{
    int  ret = -1;
    char value = 0;
    int  retry = 0;
    while(retry++ < 3)
    {        
        //ret = icn83xx_read_reg(0, &value);
        ret = icn83xx_prog_read_reg(0, &value);
        if(ret > 0)
        {
            if(value == 0x31)
                return ret;
        }
        icn83xx_error("iic test error! %d\n", retry);
        msleep(3);
    }
    return -1;    
}
/***********************************************************************************************
Name    :   icn83xx_ts_release 
Input   :   void
Output  :   
function    : touch release
***********************************************************************************************/
int release_counter = 0;
static void icn83xx_ts_release(void)
{
    struct icn83xx_ts_data *icn83xx_ts = i2c_get_clientdata(this_client);
    if(release_counter == 0)
    {
        icn83xx_point_info("==icn83xx_ts_release ==\n");
        input_report_abs(icn83xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
        input_sync(icn83xx_ts->input_dev);
        release_counter ++;
    }
}

static int icn83xx_ts_button_coordinate[4][2]={
    {120,1050},  /*Menu*/
    {300,1050},  /*Home*/
    {490,1050},  /*Back*/
    {0,0},       /*Search*/
};


/***********************************************************************************************
Name    :   icn83xx_report_value_A
Input   :   void
Output  :   
function    : reprot touch ponit
***********************************************************************************************/
static int icn83xx_report_value_A(void)
{    
    struct icn83xx_ts_data *icn83xx_ts = i2c_get_clientdata(this_client);
    unsigned char *buf;
    int i,tmp;
#if TOUCH_VIRTUAL_KEYS
    unsigned char button;
    static unsigned char button_last;
#endif
    icn83xx_info("==icn83xx_report_value_A ==\n");

/*
    ret = icn83xx_i2c_rxdata(16, buf, POINT_NUM*POINT_SIZE+2);
    if (ret < 0) {
        icn83xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
        return ret;
    }
*/
    TP_Scan();
    buf = TP_ReadPoint();

#if TOUCH_VIRTUAL_KEYS    
    button = buf[0];    
    icn83xx_info("%s: button=%d\n",__func__, button);

    if((button_last != 0) && (button == 0))
    {
        icn83xx_ts_release();
        button_last = button;
        return 1;       
    }
    if(button != 0)
    {
        switch(button)
        {
            case ICN_VIRTUAL_BUTTON_MENU:
                icn83xx_point_info("ICN_VIRTUAL_BUTTON_MENU down\n");
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_TRACKING_ID, 5);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 200);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_X, icn83xx_ts_button_coordinate[0][0]);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_Y, icn83xx_ts_button_coordinate[0][1]);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
                input_mt_sync(icn83xx_ts->input_dev);
                input_sync(icn83xx_ts->input_dev);    
                release_counter = 0;
                break;             
            case ICN_VIRTUAL_BUTTON_HOME:
                icn83xx_point_info("ICN_VIRTUAL_BUTTON_HOME down\n");
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_TRACKING_ID, 5);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 200);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_X, icn83xx_ts_button_coordinate[1][0]);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_Y, icn83xx_ts_button_coordinate[1][1]);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
                input_mt_sync(icn83xx_ts->input_dev);
                input_sync(icn83xx_ts->input_dev);
                release_counter = 0;
                break;                
            case ICN_VIRTUAL_BUTTON_BACK:
                icn83xx_point_info("ICN_VIRTUAL_BUTTON_BACK down\n");
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_TRACKING_ID, 5);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 200);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_X, icn83xx_ts_button_coordinate[2][0]);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_Y, icn83xx_ts_button_coordinate[2][1]);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
                input_mt_sync(icn83xx_ts->input_dev);
                input_sync(icn83xx_ts->input_dev);   
                release_counter = 0;
                break;
            case ICN_VIRTUAL_BUTTON_SEARCH:
                icn83xx_point_info("ICN_VIRTUAL_BUTTON_SEARCH down\n");
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_TRACKING_ID, 5);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 200);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_X, icn83xx_ts_button_coordinate[3][0]);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_Y, icn83xx_ts_button_coordinate[3][1]);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
                input_mt_sync(icn83xx_ts->input_dev);
                input_sync(icn83xx_ts->input_dev);  
                release_counter = 0;
                break;                      
            default:
                icn83xx_point_info("other gesture: 0x%x\n", button);
                break;          
        }
        button_last = button;
        return 1;
    }        
#endif
 
    icn83xx_ts->point_num = buf[1];  
    //icn83xx_point_info("icn83xx_ts->point_num: %d, release_counter: %d\n", icn83xx_ts->point_num, release_counter);

    if (icn83xx_ts->point_num == 0) 
    {
        icn83xx_ts_release();
        return 1; 
    }   
    for(i=0;i<icn83xx_ts->point_num;i++){
        if(buf[8 + POINT_SIZE*i]  != 4) break ;
    }
    
    if(i == icn83xx_ts->point_num) {
        icn83xx_ts_release();
        return 1; 
    }   

    for(i=0; i<icn83xx_ts->point_num; i++)
    {
        icn83xx_ts->point_info[i].u8ID = buf[2 + POINT_SIZE*i];
        icn83xx_ts->point_info[i].u16PosX = (buf[4 + POINT_SIZE*i]<<8) + buf[3 + POINT_SIZE*i];
        icn83xx_ts->point_info[i].u16PosY = (buf[6 + POINT_SIZE*i]<<8) + buf[5 + POINT_SIZE*i];
        icn83xx_ts->point_info[i].u8Pressure = 200;//buf[7 + POINT_SIZE*i];
        icn83xx_ts->point_info[i].u8EventId = buf[8 + POINT_SIZE*i];    

        if(1 == icn83xx_ts->exchange_x_y_flag)
        {
	    tmp =icn83xx_ts->point_info[i].u16PosX; 
            icn83xx_ts->point_info[i].u16PosX = icn83xx_ts->point_info[i].u16PosY;
            icn83xx_ts->point_info[i].u16PosY = tmp;
        }
        if(1 == icn83xx_ts->revert_x_flag)
        {
            icn83xx_ts->point_info[i].u16PosX = icn83xx_ts->screen_max_x- icn83xx_ts->point_info[i].u16PosX;
        }
        if(1 == icn83xx_ts->revert_y_flag)
        {
            icn83xx_ts->point_info[i].u16PosY = icn83xx_ts->screen_max_y- icn83xx_ts->point_info[i].u16PosY;
        }
        
        icn83xx_info("u8ID %d\n", icn83xx_ts->point_info[i].u8ID);
        icn83xx_info("u16PosX %d\n", icn83xx_ts->point_info[i].u16PosX);
        icn83xx_info("u16PosY %d\n", icn83xx_ts->point_info[i].u16PosY);
        icn83xx_info("u8Pressure %d\n", icn83xx_ts->point_info[i].u8Pressure);
        icn83xx_info("u8EventId %d\n", icn83xx_ts->point_info[i].u8EventId);  


        input_report_abs(icn83xx_ts->input_dev, ABS_MT_TRACKING_ID, icn83xx_ts->point_info[i].u8ID);    
        input_report_abs(icn83xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, icn83xx_ts->point_info[i].u8Pressure);
        input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_X, icn83xx_ts->point_info[i].u16PosX);
        input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_Y, icn83xx_ts->point_info[i].u16PosY);
        input_report_abs(icn83xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
        input_mt_sync(icn83xx_ts->input_dev);
        icn83xx_point_info("point: %d ===x = %d,y = %d, press = %d ====\n",i, icn83xx_ts->point_info[i].u16PosX,icn83xx_ts->point_info[i].u16PosY, icn83xx_ts->point_info[i].u8Pressure);
        tchip_dbg_pr("point: %d ===x = %d,y = %d, press = %d ====\n",i, icn83xx_ts->point_info[i].u16PosX,icn83xx_ts->point_info[i].u16PosY, icn83xx_ts->point_info[i].u8Pressure);
	
    }

    input_sync(icn83xx_ts->input_dev);
    release_counter = 0;
    return 0;
}
/***********************************************************************************************
Name    :   icn83xx_report_value_B
Input   :   void
Output  :   
function    : reprot touch ponit
***********************************************************************************************/
#if CTP_REPORT_PROTOCOL
    #if defined(CONFIG_TCHIP_MACH_TR726C) && defined(CONFIG_NMC1XXX_WIFI_MODULE)
extern int nmc1000_charging_status ;
    #endif
static int icn83xx_report_value_B(void)
{
    struct icn83xx_ts_data *icn83xx_ts = i2c_get_clientdata(this_client);
    unsigned char *buf;
    static unsigned char finger_last[POINT_NUM + 1]={0};
    unsigned char  finger_current[POINT_NUM + 1] = {0};
    unsigned int position = 0;
    int temp = 0;
    int ret = -1;
    int tmp;
    icn83xx_info("==icn83xx_report_value_B ==\n");
    
    #if defined(CONFIG_TCHIP_MACH_TR726C) && defined(CONFIG_NMC1XXX_WIFI_MODULE)
    g_structSystemStatus.u8ChargerPlugIn = nmc1000_charging_status;
		//printk("CHARGERSTATUS=%d\n",g_structSystemStatus.u8ChargerPlugIn);
    #endif
/*    
    ret = icn83xx_i2c_rxdata(16, buf, POINT_NUM*POINT_SIZE+2);
    if (ret < 0) {
        icn83xx_error("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
        return ret;
    }
*/    
    TP_Scan();
    buf = TP_ReadPoint();

    icn83xx_ts->point_num = buf[1];    
    if(icn83xx_ts->point_num > 0)
    {
        for(position = 0; position<icn83xx_ts->point_num; position++)
        {       
            temp = buf[2 + POINT_SIZE*position] + 1;
            finger_current[temp] = 1;
            icn83xx_ts->point_info[temp].u8ID = buf[2 + POINT_SIZE*position];
            icn83xx_ts->point_info[temp].u16PosX = (buf[4 + POINT_SIZE*position]<<8) + buf[3 + POINT_SIZE*position];
            icn83xx_ts->point_info[temp].u16PosY = (buf[6 + POINT_SIZE*position]<<8) + buf[5 + POINT_SIZE*position];

	 #ifdef CONFIG_TOUCHSCREEN_ICN83XX_PARA_DUODIAN_626P
		// printk("1111u16posy=%d\n",icn83xx_ts->point_info[temp].u16PosY);
                 if(icn83xx_ts->point_info[temp].u16PosY < 300)
                {
                        icn83xx_ts->point_info[temp].u16PosY=icn83xx_ts->point_info[temp].u16PosY-(icn83xx_ts->screen_max_y-icn83xx_ts->point_info[temp].u16PosY)/30;
                 //      printk("222222u16posy=%d\n",icn83xx_ts->point_info[temp].u16PosY);
                }
                 if(icn83xx_ts->point_info[temp].u16PosY < 0||icn83xx_ts->point_info[temp].u16PosY>626)
                {
                        icn83xx_ts->point_info[temp].u16PosY=0;
		}
	#endif		 
               
           icn83xx_ts->point_info[temp].u8Pressure = buf[7 + POINT_SIZE*position];
            icn83xx_ts->point_info[temp].u8EventId = buf[8 + POINT_SIZE*position];
            
            if(icn83xx_ts->point_info[temp].u8EventId == 4)
                finger_current[temp] = 0;
            
	    if(1 == icn83xx_ts->exchange_x_y_flag)
            {
	    	tmp =icn83xx_ts->point_info[temp].u16PosX; 
            	icn83xx_ts->point_info[temp].u16PosX = icn83xx_ts->point_info[temp].u16PosY;
            	icn83xx_ts->point_info[temp].u16PosY = tmp;
            } 
            if(1 == icn83xx_ts->revert_x_flag)
            {
                icn83xx_ts->point_info[temp].u16PosX = icn83xx_ts->screen_max_x- icn83xx_ts->point_info[temp].u16PosX;
            }
            if(1 == icn83xx_ts->revert_y_flag)
            {
                icn83xx_ts->point_info[temp].u16PosY = icn83xx_ts->screen_max_y- icn83xx_ts->point_info[temp].u16PosY;
            }
            icn83xx_info("temp %d\n", temp);
            icn83xx_info("u8ID %d\n", icn83xx_ts->point_info[temp].u8ID);
            icn83xx_info("u16PosX %d\n", icn83xx_ts->point_info[temp].u16PosX);
            icn83xx_info("u16PosY %d\n", icn83xx_ts->point_info[temp].u16PosY);
            icn83xx_info("u8Pressure %d\n", icn83xx_ts->point_info[temp].u8Pressure);
            icn83xx_info("u8EventId %d\n", icn83xx_ts->point_info[temp].u8EventId);             
            //icn83xx_info("u8Pressure %d\n", icn83xx_ts->point_info[temp].u8Pressure*16);
        }
    }   
    else
    {
        for(position = 1; position < POINT_NUM+1; position++)
        {
            finger_current[position] = 0;
        }
        icn83xx_info("no touch\n");
    }

    for(position = 1; position < POINT_NUM + 1; position++)
    {
        if((finger_current[position] == 0) && (finger_last[position] != 0))
        {
            input_mt_slot(icn83xx_ts->input_dev, position-1);
            input_mt_report_slot_state(icn83xx_ts->input_dev, MT_TOOL_FINGER, false);
            icn83xx_point_info("one touch up: %d\n", position);
        }
        else if(finger_current[position])
        {
        #if defined(CONFIG_TCHIP_MACH_TR736_HD) 
            if (icn83xx_ts->point_info[position].u16PosY <= 600) { //ignore pointer greater than 600
                input_mt_slot(icn83xx_ts->input_dev, position-1);
                input_mt_report_slot_state(icn83xx_ts->input_dev, MT_TOOL_FINGER, true);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 1);
                //input_report_abs(icn83xx_ts->input_dev, ABS_MT_PRESSURE, icn83xx_ts->point_info[position].u8Pressure);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_PRESSURE, 200);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_X, icn83xx_ts->point_info[position].u16PosX);
                if (icn83xx_ts->point_info[position].u16PosY > 590) { //fix pointer greater than 588 to 588
                    icn83xx_ts->point_info[position].u16PosY = 590;
                }
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_Y, (600 - icn83xx_ts->point_info[position].u16PosY));
                icn83xx_point_info("===position: %d, x = %d,y = %d, press = %d ====\n", position, icn83xx_ts->point_info[position].u16PosX,icn83xx_ts->point_info[position].u16PosY, icn83xx_ts->point_info[position].u8Pressure);
                tchip_dbg_pr("===position: %d, x = %d,y = %d, press = %d ====\n", position, icn83xx_ts->point_info[position].u16PosX,icn83xx_ts->point_info[position].u16PosY, icn83xx_ts->point_info[position].u8Pressure);
            }
        #elif defined(CONFIG_TCHIP_MACH_TR736D)
            if (icn83xx_ts->point_info[position].u16PosY <= 480) { //ignore pointer greater than 600
                input_mt_slot(icn83xx_ts->input_dev, position-1);
                input_mt_report_slot_state(icn83xx_ts->input_dev, MT_TOOL_FINGER, true);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 1);
                //input_report_abs(icn83xx_ts->input_dev, ABS_MT_PRESSURE, icn83xx_ts->point_info[position].u8Pressure);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_PRESSURE, 200);
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_X, icn83xx_ts->point_info[position].u16PosX);
                if (icn83xx_ts->point_info[position].u16PosY > 470) { //fix pointer greater than 588 to 588
                    icn83xx_ts->point_info[position].u16PosY = 470;
                }
                input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_Y, (480 - icn83xx_ts->point_info[position].u16PosY));
                icn83xx_point_info("===position: %d, x = %d,y = %d, press = %d ====\n", position, icn83xx_ts->point_info[position].u16PosX,icn83xx_ts->point_info[position].u16PosY, icn83xx_ts->point_info[position].u8Pressure);
                tchip_dbg_pr("===position: %d, x = %d,y = %d, press = %d ====\n", position, icn83xx_ts->point_info[position].u16PosX,icn83xx_ts->point_info[position].u16PosY, icn83xx_ts->point_info[position].u8Pressure);
            }
        #else
            input_mt_slot(icn83xx_ts->input_dev, position-1);
            input_mt_report_slot_state(icn83xx_ts->input_dev, MT_TOOL_FINGER, true);
            input_report_abs(icn83xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 1);
            //input_report_abs(icn83xx_ts->input_dev, ABS_MT_PRESSURE, icn83xx_ts->point_info[position].u8Pressure);
            input_report_abs(icn83xx_ts->input_dev, ABS_MT_PRESSURE, 200);
            input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_X, icn83xx_ts->point_info[position].u16PosX);
            input_report_abs(icn83xx_ts->input_dev, ABS_MT_POSITION_Y, icn83xx_ts->point_info[position].u16PosY);
            icn83xx_point_info("===position: %d, x = %d,y = %d, press = %d ====\n", position, icn83xx_ts->point_info[position].u16PosX,icn83xx_ts->point_info[position].u16PosY, icn83xx_ts->point_info[position].u8Pressure);
            tchip_dbg_pr("===position: %d, x = %d,y = %d, press = %d ====\n", position, icn83xx_ts->point_info[position].u16PosX,icn83xx_ts->point_info[position].u16PosY, icn83xx_ts->point_info[position].u8Pressure);
        #endif  
        }
    }
    input_sync(icn83xx_ts->input_dev);

    for(position = 1; position < POINT_NUM + 1; position++)
    {
        finger_last[position] = finger_current[position];
    }
    return 0;
}
#endif

/***********************************************************************************************
Name    :   icn83xx_ts_pen_irq_work
Input   :   void
Output  :   
function    : work_struct
***********************************************************************************************/
static int icn83xx_ts_pen_irq_work(struct work_struct *work)
{
    struct icn83xx_ts_data *icn83xx_ts = i2c_get_clientdata(this_client); 
    int i;
#if SUPPORT_PROC_FS
    if(down_interruptible(&icn83xx_ts->sem))  
    {  
    	is_timer_running = 0;
        return -1;   
    }  
#endif

    if(u8ResFlag == 0x5a)
    {
       #if SUPPORT_SPREADTRUM 
        //reset tp still get nak ,we have to power off tp and power on 
        gpio_direction_output(sprd_3rdparty_gpio_tp_rst, 1);
        gpio_set_value(sprd_3rdparty_gpio_tp_rst, 0);
        for(i=0;i<20;i++)  //force the ldo turn off
        {
            LDO_TurnOffLDO(LDO_LDO_SIM2);
        }
        msleep(100);
        
        for(i=0;i<20;i++)  //force the ldo turn on
        {
            LDO_SetVoltLevel(LDO_LDO_SIM2, LDO_VOLT_LEVEL0);
            LDO_TurnOnLDO(LDO_LDO_SIM2);
        }
        msleep(50);
        gpio_set_value(sprd_3rdparty_gpio_tp_rst,1);
        msleep(100);
        //add end
        #endif
        TP_IicClkTune(425000);
        TP_Init();
        TP_SetPanelPara();
        TP_NormalWork();
        u8ResFlag = 0;
    }
    if(icn83xx_ts->work_mode == 0)
    {
#if CTP_REPORT_PROTOCOL
        icn83xx_report_value_B();
#else
        icn83xx_report_value_A();
#endif 

    }   
    else if(icn83xx_ts->work_mode == 1)
    {
        TP_ReInit();
        TP_NormalWork();
        TP_Scan();
        icn83xx_ts->work_mode = 0;
    }
    else if(icn83xx_ts->work_mode == 2) // update reg
    {
        printk("addr : 0x%x\n",reg_addr);
        printk("value: 0x%x\n",reg_value);
        icn83xx_ts->work_mode = 0;
        if(UPDATE_CMD_WRITE_REG == update_cmd)
        {
            icn83xx_prog_i2c_txdata(reg_addr, &reg_value, 1);
            g_updatebase = TRUE;
            TP_Scan();
        }
        else if(UPDATE_CMD_READ_REG == update_cmd)
        {
            icn83xx_prog_i2c_rxdata(reg_addr, &reg_value, 1);
        }
        else
        {
            //do nothing 
        }
    }

#if SUPPORT_PROC_FS
    up(&icn83xx_ts->sem);
#endif

    	is_timer_running = 0;
    return 0;
}
/***********************************************************************************************
Name    :   chipone_timer_func
Input   :   void
Output  :   
function    : Timer interrupt service routine.
***********************************************************************************************/
extern int is_backligth_closed;
static enum hrtimer_restart chipone_timer_func(struct hrtimer *timer)
{
    struct icn83xx_ts_data *icn83xx_ts = container_of(timer, struct icn83xx_ts_data, timer);
    if( is_timer_running == 0 && is_backligth_closed == 0){
	is_timer_running =1;
    	queue_work(icn83xx_ts->ts_workqueue, &icn83xx_ts->pen_event_work);
    }
    hrtimer_start(&icn83xx_ts->timer, ktime_set(CTP_POLL_TIMER/1000, (CTP_POLL_TIMER%1000)*1000000), HRTIMER_MODE_REL);

    return HRTIMER_NORESTART;
}


//#ifdef CONFIG_HAS_EARLYSUSPEND
/***********************************************************************************************
Name    :   icn83xx_ts_suspend
Input   :   void
Output  :   
function    : tp enter sleep mode
***********************************************************************************************/
static void icn83xx_ts_suspend(struct early_suspend *handler)
{
    struct icn83xx_ts_data *icn83xx_ts = i2c_get_clientdata(this_client);
    icn83xx_trace("==icn83xx_ts_suspend==\n");

#if 1
    icn83xx_ts->work_mode = 0x5a;   //set work queue idle
    hrtimer_cancel(&icn83xx_ts->timer);
    cancel_work_sync(&icn83xx_ts->pen_event_work);//
    is_timer_running = 0;//stop the timer until the resume is run

    TP_Sleep();
#else
    hrtimer_cancel(&icn83xx_ts->timer);
    cancel_work_sync(&icn83xx_ts->pen_event_work);
    is_timer_running = 0;
#endif
}

/***********************************************************************************************
Name    :   icn83xx_ts_resume
Input   :   void
Output  :   
function    : wakeup tp or reset tp
***********************************************************************************************/
static void icn83xx_ts_resume(struct early_suspend *handler)
{
    struct icn83xx_ts_data *icn83xx_ts = i2c_get_clientdata(this_client);
    icn83xx_trace("==icn83xx_ts_resume== \n");
#if 1
//    icn83xx_ts_wakeup();
    icn83xx_ts_reset();

    TP_WakeUp();
    TP_ReInit();
    //TP_SetPanelPara();
    //TP_NormalWork();

    icn83xx_ts->work_mode = 0x0;   //set work queue normal
    //hrtimer_start(&icn83xx_ts->timer, ktime_set(CTP_START_TIMER/1000, (CTP_START_TIMER%1000)*1000000), HRTIMER_MODE_REL);
    //is_timer_running = 1;
    chipone_timer_func(&icn83xx_ts->timer);
#else
    chipone_timer_func(&icn83xx_ts->timer);
#endif
   
}
//#endif

/***********************************************************************************************
Name    :   icn83xx_request_io_port
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
static int icn83xx_request_io_port(struct icn83xx_ts_data *icn83xx_ts)
{
#if SUPPORT_ALLWINNER_A13
    icn83xx_ts->screen_max_x = screen_max_x;
    icn83xx_ts->screen_max_y = screen_max_y;
    icn83xx_ts->revert_x_flag = revert_x_flag;
    icn83xx_ts->revert_y_flag = revert_y_flag;
    icn83xx_ts->exchange_x_y_flag = exchange_x_y_flag;
    icn83xx_ts->irq = CTP_IRQ_PORT;
#endif 

#if SUPPORT_ROCKCHIP
    if( hw_pdata){
    	icn83xx_ts->screen_max_x = hw_pdata->screen_max_x;
    	icn83xx_ts->screen_max_y = hw_pdata->screen_max_y;
    	icn83xx_ts->revert_x_flag = hw_pdata->revert_x;
    	icn83xx_ts->revert_y_flag = hw_pdata->revert_y;
    	icn83xx_ts->exchange_x_y_flag = hw_pdata->revert_xy;
    	printk(KERN_ERR "########### hw_pdata: pix=%dx%d,revert_flay=%d,%d,%d\n",icn83xx_ts->screen_max_x,icn83xx_ts->screen_max_y,
				icn83xx_ts->revert_x_flag,icn83xx_ts->revert_y_flag,icn83xx_ts->exchange_x_y_flag); 
    }else{
    	printk(KERN_ERR "###########   no hw_pdata found \n"); 
	return -1;
    }
    //icn83xx_ts->irq = CTP_IRQ_PORT;
   #if 1
    if( tp_screen_type >= 6000 ){
    	icn83xx_ts->screen_max_x = 1024;
    	icn83xx_ts->screen_max_y = 600;
	if( tp_screen_type== 6003 ) //duo dian hd tp
    		icn83xx_ts->screen_max_y = 626;
    }else if( tp_screen_type >= 4800 ){
    	icn83xx_ts->screen_max_x = 800;
    	icn83xx_ts->screen_max_y = 480;
    }
   #endif
#endif
#if SUPPORT_SPREADTRUM
    icn83xx_ts->screen_max_x = SCREEN_MAX_X;
    icn83xx_ts->screen_max_y = SCREEN_MAX_Y;
    icn83xx_ts->irq = CTP_IRQ_PORT;
#endif

    return 0;
}

/***********************************************************************************************
Name    :   icn83xx_free_io_port
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
static int icn83xx_free_io_port(struct icn83xx_ts_data *icn83xx_ts)
{    
#if SUPPORT_ALLWINNER_A13    
    ctp_ops.free_platform_resource();
#endif

#if SUPPORT_ROCKCHIP

#endif

#if SUPPORT_SPREADTRUM

#endif    
    return 0;
}

/***********************************************************************************************
Name    :   icn83xx_request_irq
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
static int icn83xx_request_irq(struct icn83xx_ts_data *icn83xx_ts)
{
    int err = -1;
    icn83xx_ts->use_irq = 0;  //no irq
    return err;
   
}

/***********************************************************************************************
Name    :   icn83xx_request_input_dev
Input   :   void
Output  :   
function    : 0 success,
***********************************************************************************************/
static int icn83xx_request_input_dev(struct icn83xx_ts_data *icn83xx_ts)
{
    int ret = -1;    
    struct input_dev *input_dev;

    input_dev = input_allocate_device();
    if (!input_dev) {
        icn83xx_error("failed to allocate input device\n");
        return -ENOMEM;
    }
    icn83xx_ts->input_dev = input_dev;

    icn83xx_ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
#if CTP_REPORT_PROTOCOL
    __set_bit(INPUT_PROP_DIRECT, icn83xx_ts->input_dev->propbit);
    input_mt_init_slots(icn83xx_ts->input_dev, 255);
#else
    set_bit(ABS_MT_TOUCH_MAJOR, icn83xx_ts->input_dev->absbit);
    set_bit(ABS_MT_POSITION_X, icn83xx_ts->input_dev->absbit);
    set_bit(ABS_MT_POSITION_Y, icn83xx_ts->input_dev->absbit);
    set_bit(ABS_MT_WIDTH_MAJOR, icn83xx_ts->input_dev->absbit); 
    set_bit(ABS_MT_TRACKING_ID, icn83xx_ts->input_dev->absbit);
#endif
    input_set_abs_params(icn83xx_ts->input_dev, ABS_MT_POSITION_X, 0, icn83xx_ts->screen_max_x, 0, 0);
    input_set_abs_params(icn83xx_ts->input_dev, ABS_MT_POSITION_Y, 0, icn83xx_ts->screen_max_y, 0, 0);
    input_set_abs_params(icn83xx_ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(icn83xx_ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);  
    input_set_abs_params(icn83xx_ts->input_dev, ABS_MT_TRACKING_ID, 0, POINT_NUM, 0, 0);

    __set_bit(KEY_MENU,  input_dev->keybit);
    __set_bit(KEY_BACK,  input_dev->keybit);
    __set_bit(KEY_HOME,  input_dev->keybit);
    __set_bit(KEY_SEARCH,  input_dev->keybit);

    set_bit(EV_ABS, input_dev->evbit); //add by wuye
    set_bit(EV_KEY, input_dev->evbit);

    input_dev->name = CTP_NAME;
    ret = input_register_device(input_dev);
    if (ret) {
        icn83xx_error("Register %s input device failed\n", input_dev->name);
        input_free_device(input_dev);
        return -ENODEV;        
    }
    
//#ifdef CONFIG_HAS_EARLYSUSPEND
    icn83xx_trace("==register_early_suspend =\n");
    icn83xx_ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    icn83xx_ts->early_suspend.suspend = icn83xx_ts_suspend;
    icn83xx_ts->early_suspend.resume  = icn83xx_ts_resume;
    register_early_suspend(&icn83xx_ts->early_suspend);
//#endif

    return 0;
}

#if SUPPORT_DELAYED_WORK
extern int force_use_inner_para;
static void icn_delayedwork_fun(struct delayed_work *icn_delayed_work)
{
    STRUCT_PANEL_PARA *sPtr;
    int err = 0;
    static int cnt = 0;

	struct icn83xx_ts_data *icn83xx_ts = i2c_get_clientdata(this_client);
    TP_IicClkTune(425000);
    TP_Init();
    if ( 0 != TP_SetPanelPara() && cnt < 10){
	    cnt ++;
	    printk(KERN_ERR "########  icn83xx delayed_work retry load fw, cnt=%d\n",cnt);
    	schedule_delayed_work(icn_delayed_work, msecs_to_jiffies(1000));
	return;
    }
#if 1
    if(cnt >=10 ){
	force_use_inner_para = 1;
	err = TP_SetPanelPara();
	printk(KERN_ERR "#############  %s : TP_SetPanelPara failed , use defined ,return %d\n",__func__,err);
    }
#endif
    TP_NormalWork();
    sPtr = TP_GetPanelPara();   
    icn83xx_memdump(&sPtr->u8ColFbCap[0], 16);
    icn83xx_memdump(&sPtr->u8ColDcOffset[0], 8);
#if !SUPPORT_ROCKCHIP
    err = icn83xx_request_irq(icn83xx_ts);
    if (err != 0)
    {
#endif
        icn83xx_error("request irq error, use timer\n");
        icn83xx_ts->use_irq = 0;
        hrtimer_init(&icn83xx_ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        icn83xx_ts->timer.function = chipone_timer_func;
		is_timer_running =0;
        hrtimer_start(&icn83xx_ts->timer, ktime_set(CTP_START_TIMER/1000, (CTP_START_TIMER%1000)*1000000), HRTIMER_MODE_REL);
#if !SUPPORT_ROCKCHIP
    }
#endif

}
#endif

static int icn83xx_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct icn83xx_ts_data *icn83xx_ts;
    STRUCT_PANEL_PARA *sPtr;
    int err = 0;

    icn83xx_trace("====%s begin=====.  \n", __func__);

    if(gpio_request(POWER_PORT,NULL) != 0){
	gpio_free(POWER_PORT);
	printk("zhongchu_init_platform_hw gpio_request error\n");
	return -EIO;
    }
    gpio_direction_output(POWER_PORT, 0);
    gpio_set_value(POWER_PORT,GPIO_LOW);
    msleep(20);

#if 1 //add by ruan 
    hw_pdata = client->dev.platform_data;   
    if (hw_pdata && hw_pdata->init_platform_hw)                              
	hw_pdata->init_platform_hw();
	//add by zt
	icn83xx_ts_reset();
#endif

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
    {
        icn83xx_error("I2C check functionality failed.\n");
        return -ENODEV;
    }

    icn83xx_ts = kzalloc(sizeof(*icn83xx_ts), GFP_KERNEL);
    if (!icn83xx_ts)
    {
        icn83xx_error("Alloc icn83xx_ts memory failed.\n");
        return -ENOMEM;
    }
    memset(icn83xx_ts, 0, sizeof(*icn83xx_ts));

    this_client = client;
    this_client->addr = client->addr;
    i2c_set_clientdata(client, icn83xx_ts);

    icn83xx_ts->work_mode = 0;
    spin_lock_init(&icn83xx_ts->irq_lock);
//    icn83xx_ts->irq_lock = SPIN_LOCK_UNLOCKED;

#if SUPPORT_SPREADTRUM
    LDO_SetVoltLevel(LDO_LDO_SIM2, LDO_VOLT_LEVEL0);
    LDO_TurnOnLDO(LDO_LDO_SIM2);
    msleep(5);
    icn83xx_ts_reset();

    sc8810_i2c_set_clk(2,550000);
#endif

    err = icn83xx_iic_test();
    if (err < 0)
    {
        icn83xx_error("icn83xx_iic_test  failed.\n");
        kfree(icn83xx_ts);
        return -EBUSY;
    }
    else
    {
        icn83xx_trace("iic communication ok\n"); 
    }

    err = icn83xx_request_io_port(icn83xx_ts);
    if (err != 0)
    {
        icn83xx_error("icn83xx_request_io_port failed.\n");
        kfree(icn83xx_ts);
        return err;
    }

    INIT_WORK(&icn83xx_ts->pen_event_work, icn83xx_ts_pen_irq_work);
    icn83xx_ts->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
    if (!icn83xx_ts->ts_workqueue) {
        icn83xx_error("create_singlethread_workqueue failed.\n");
        kfree(icn83xx_ts);
        return -ESRCH;
    }

    err= icn83xx_request_input_dev(icn83xx_ts);
    if (err < 0)
    {
        icn83xx_error("request input dev failed\n");
        kfree(icn83xx_ts);
        return err;        
    }


#if TOUCH_VIRTUAL_KEYS
    icn83xx_ts_virtual_keys_init();
#endif

    //RwFuncInit();
#if ! SUPPORT_DELAYED_WORK
    TP_IicClkTune(425000);
    TP_Init();
    TP_SetPanelPara();
    TP_NormalWork();
    sPtr = TP_GetPanelPara();   
    icn83xx_memdump(&sPtr->u8ColFbCap[0], 16);
    icn83xx_memdump(&sPtr->u8ColDcOffset[0], 8);

    err = icn83xx_request_irq(icn83xx_ts);
    if (err != 0)
    {
        icn83xx_error("request irq error, use timer\n");
        icn83xx_ts->use_irq = 0;
        hrtimer_init(&icn83xx_ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        icn83xx_ts->timer.function = chipone_timer_func;
	is_timer_running =0;
        hrtimer_start(&icn83xx_ts->timer, ktime_set(CTP_START_TIMER/1000, (CTP_START_TIMER%1000)*1000000), HRTIMER_MODE_REL);
    }
#endif

#if SUPPORT_SYSFS
    icn83xx_create_sysfs(client);
#endif

#if SUPPORT_PROC_FS
    sema_init(&icn83xx_ts->sem, 1);
    init_proc_node();
#endif

#if SUPPORT_DELAYED_WORK
	INIT_DELAYED_WORK(&icn83xx_ts->icn_delayed_work, icn_delayedwork_fun);
	schedule_delayed_work(&icn83xx_ts->icn_delayed_work, msecs_to_jiffies(11000));//delay unitl display android logo
#endif

    icn83xx_trace("==%s over =\n", __func__);    
    return 0;
}

static int __devexit icn83xx_ts_remove(struct i2c_client *client)
{
    struct icn83xx_ts_data *icn83xx_ts = i2c_get_clientdata(client);  
    icn83xx_trace("==icn83xx_ts_remove=\n");

	hrtimer_cancel(&icn83xx_ts->timer);	
#if SUPPORT_DELAYED_WORK
	cancel_delayed_work_sync(&icn83xx_ts->icn_delayed_work);
#endif

#if SUPPORT_PROC_FS
    uninit_proc_node();
#endif

#if SUPPORT_SYSFS
    icn83xx_remove_sysfs(client);
#endif

#if TOUCH_VIRTUAL_KEYS
    //do something clean up virtual keys
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&icn83xx_ts->early_suspend);
#endif

    input_unregister_device(icn83xx_ts->input_dev);
    //input_free_device(icn83xx_ts->input_dev);
    cancel_work_sync(&icn83xx_ts->pen_event_work);
    destroy_workqueue(icn83xx_ts->ts_workqueue);

    icn83xx_free_io_port(icn83xx_ts);
    kfree(icn83xx_ts);    
    i2c_set_clientdata(client, NULL);
    return 0;
}

static const struct i2c_device_id icn83xx_ts_id[] = {
    { CTP_NAME, 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, icn83xx_ts_id);

static struct i2c_driver icn83xx_ts_driver = {
    .class      = I2C_CLASS_HWMON,
    .probe      = icn83xx_ts_probe,
    .remove     = __devexit_p(icn83xx_ts_remove),
//#ifndef CONFIG_HAS_EARLYSUSPEND
//    .suspend    = icn83xx_ts_suspend,
//    .resume     = icn83xx_ts_resume,
//#endif
    .id_table   = icn83xx_ts_id,
    .driver = {
        .name   = CTP_NAME,
        .owner  = THIS_MODULE,
    },
#if SUPPORT_ALLWINNER_A13    
    .address_list   = u_i2c_addr.normal_i2c,
#endif    
};

static int __init icn83xx_ts_init(void)
{ 
    int ret = -1;
    icn83xx_trace("===========================%s=====================\n", __func__);

#if SUPPORT_ALLWINNER_A13
    if (ctp_ops.fetch_sysconfig_para)
    {
        if(ctp_ops.fetch_sysconfig_para()){
            pr_info("%s: err.\n", __func__);
            return -1;
        }
    }
    icn83xx_trace("%s: after fetch_sysconfig_para:  normal_i2c: 0x%hx. normal_i2c[1]: 0x%hx \n", \
    __func__, u_i2c_addr.normal_i2c[0], u_i2c_addr.normal_i2c[1]);

    ret = ctp_ops.init_platform_resource();
    if(0 != ret){
        icn83xx_error("%s:ctp_ops.init_platform_resource err. \n", __func__);    
    }
    //reset
    ctp_ops.ts_reset();
    //wakeup
    ctp_ops.ts_wakeup();      
    icn83xx_ts_driver.detect = ctp_ops.ts_detect;
#endif     
    RwFuncInit();
    ret = i2c_add_driver(&icn83xx_ts_driver);
    icn83xx_trace("=========================== ret:%d=====================\n",ret);
    return ret;
}

static void __exit icn83xx_ts_exit(void)
{
    icn83xx_trace("==icn83xx_ts_exit==\n");
    i2c_del_driver(&icn83xx_ts_driver);
}


module_init(icn83xx_ts_init);
module_exit(icn83xx_ts_exit);

MODULE_AUTHOR("<zmtian@chiponeic.com>");
MODULE_DESCRIPTION("Chipone icn83xx TouchScreen driver");
MODULE_LICENSE("GPL");
