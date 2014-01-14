/*++
 
 Copyright (c) 2012-2022 ChipOne Technology (Beijing) Co., Ltd. All Rights Reserved.
 This PROPRIETARY SOFTWARE is the property of ChipOne Technology (Beijing) Co., Ltd. 
 and may contains trade secrets and/or other confidential information of ChipOne 
 Technology (Beijing) Co., Ltd. This file shall not be disclosed to any third party,
 in whole or in part, without prior written consent of ChipOne.  
 THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. CHIPONE DISCLAIMS ALL EXPRESS OR 
 IMPLIED WARRANTIES.  
 
 File Name:    icn838x_iic_slave.c
 Abstract:
    
 //Data format :
 //For example :Read:   
 //                 MOSI:  |0xF1|Addr[15:8]|Addr[7:0]| xxx | xxx |..........
 //                 MISO:  | xx |    xx    |   xx    |Data0|Data1|......|Datan|    
 //             Write:  
 //                 MOSI:  |0xF0|Addr[15:8]|Addr[7:0]|Data0|Data1|......|Datan|
 //                 MISO:  | xx |    xx    |   xx    | xxx | xxx |..........        

 Author:       Zhimin Tian
 Date :        12 24,2012
 Version:      1.0 for sensor only
 History :
  

 --*/


#include "icn838x_slave.h"
#include "icn838x_iic_ts.h"
#include "constant.h"
#include "HostComm.h"
#include "TouchCommon.h"
#include "WorkModeNormal.h"


#if SUPPORT_ALLWINNER_A13
static char para[128] = {"/system/vendor/modules/icn838x_para.bin"};
//static char para[128] = {"inner_para"};
#endif

#if SUPPORT_ROCKCHIP
int tp_screen_type = 0;  
// duodian=3, ruishensi=2 , feichu=1 , hd=600, md=480, for example hd-fc TP set tp_screen_type= 6001
module_param_named(tp_screen_type, tp_screen_type, int, 0644);
void tp_screen_type_setup(char *str)
{
	int len ;
	int num;

	len = strlen(str);
	printk (KERN_ERR "tp_screen_type str len = %d\n",len);
	if( len < 4 )
		return ;
	strict_strtoul(str, 0, &num);	
	
	if( num > 4800 && num < 7000 )
		tp_screen_type = num;

	printk(KERN_INFO "######## num = %d, str= %s, tp_screen_type =  %d\n", num,str,tp_screen_type);
}
__setup("tp_screen=",tp_screen_type_setup);
static char para_fc_hd[128] = {"/system/etc/firmware/icn838x_para_fc_600p.bin"};
static char para_rs_hd[128] = {"/system/etc/firmware/icn838x_para_rs284_600p.bin"};
static char para_dd_hd[128] = {"/system/etc/firmware/icn838x_para_duodian_626p.bin"};
static char para_dd_md[128] = {"/system/etc/firmware/icn838x_para_duodian_506p.bin"};
static char para_rs_md[128] = {"/system/etc/firmware/icn838x_para_rs284_480p.bin"};
static char para_fc_md[128] = {"/system/etc/firmware/icn838x_para_fc_480p.bin"};
static char para_u25gt_hd[128] = {"/system/etc/firmware/icn838x_para_u25gt_600p.bin"};

#if defined(CONFIG_TCHIP_MACH_TR736_HD)
static char para[128] = {"/system/etc/firmware/icn838x_para_736d_hd.bin"};
#elif defined(CONFIG_TCHIP_MACH_TR736D)
static char para[128] = {"/system/etc/firmware/icn838x_para_736d.bin"};
/*
#elif defined(CONFIG_TOUCHSCREEN_ICN83XX_PARA_RS284_480P)
static char para[128] = {"/system/etc/firmware/icn838x_para_rs284_480p.bin"};
#elif defined(CONFIG_TOUCHSCREEN_ICN83XX_PARA_RS284_600P)
static char para[128] = {"/system/etc/firmware/icn838x_para_rs284_600p.bin"};
#elif defined(CONFIG_TOUCHSCREEN_ICN83XX_PARA_FC_600P)
static char para[128] = {"/system/etc/firmware/icn838x_para_fc_600p.bin"};
#elif defined(CONFIG_TOUCHSCREEN_ICN83XX_PARA_FC_480P)
static char para[128] = {"/system/etc/firmware/icn838x_para_fc_480p.bin"};
#elif defined(CONFIG_TOUCHSCREEN_ICN83XX_PARA_DUODIAN_626P)
static char para[128] = {"/system/etc/firmware/icn838x_para_duodian_626p.bin"};
#else //CONFIG_TOUCHSCREEN_ICN83XX_PARA_DEFAULT
static char para[128] = {"/system/etc/firmware/icn838x_para.bin"};
#endif
*/
#elif defined(CONFIG_TCHIP_MACH_600P) //base tr737V02
	#if defined(CONFIG_TCHIP_MACH_TP_SCREEN_FC)
static char para[128] = {"/system/etc/firmware/icn838x_para_fc_600p.bin"};
	#elif defined(CONFIG_TCHIP_MACH_TP_SCREEN_RS284)
static char para[128] = {"/system/etc/firmware/icn838x_para_rs284_600p.bin"};
	#elif defined(CONFIG_TCHIP_MACH_TP_SCREEN_DD)
static char para[128] = {"/system/etc/firmware/icn838x_para_duodian_626p.bin"};
	#else
static char para[128] = {"/system/etc/firmware/icn838x_para.bin"};
	#endif

#elif defined (CONFIG_TCHIP_MACH_480P)//base tr737V02
	#if defined(CONFIG_TCHIP_MACH_TP_SCREEN_RS284)
static char para[128] = {"/system/etc/firmware/icn838x_para_rs284_480p.bin"};
	#elif defined(CONFIG_TCHIP_MACH_TP_SCREEN_FC)
static char para[128] = {"/system/etc/firmware/icn838x_para_fc_480p.bin"};
	#else
static char para[128] = {"/system/etc/firmware/icn838x_para.bin"};
	#endif

#else //CONFIG_TOUCHSCREEN_ICN83XX_PARA_DEFAULT
static char para[128] = {"/system/etc/firmware/icn838x_para.bin"};
#endif

//static char para[128] = {"inner_para"};
#endif

#if SUPPORT_SPREADTRUM
static char para[128] = {"/system/sps/ICN838X/para/icn838x_para.bin"};
//static char para[128] = {"inner_para"};
#endif

static struct file  *fp; 
static char cp_mode = 0;
int force_use_inner_para = 0;
STRUCT_RWFUNC g_RWFunc; 

#if 1 //default para bin
static unsigned char icn83xx_cp[] = { 
0x0f,0x0a,0x00,0x00,0x07,0x05,0x00,0x00,0x03,0x09,0x05,0x01,0x00,0x00,0x00,0x00,
0x00,0x5e,0x01,0xc8,0x00,0xaf,0x00,0x78,0x00,0x5e,0x01,0x4a,0x01,0x0e,0x01,0x03,
0x00,0x0b,0x00,0x00,0xe0,0x01,0x20,0x03,0x14,0x13,0x0d,0x0c,0x0b,0x0a,0x08,0x07,
0x06,0x05,0x04,0x03,0x02,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x09,0x08,0x07,
0x06,0x05,0x04,0x03,0x02,0x01,0x00,0x0a,0x0b,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0x30,0x30,0x30,0x30,0x30,
0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xa8,
0xe4,0xf4,0x01,0x01,0x02,0x01,0x00,0x04,0x01,0x0a,0x20,0x40,0x00,0x50,0x00,0x00,
0x0f,0x5a,0x01,0x01,0x00,0x46,0x14,0x96,0x00,0xf4,0x01,0x32,0x00,0x0a,0x00,0x50,
0x00,0x00,0x00,0x00,0x44,0x00,0x02,0x00,0x91,0xe8,0x03,0x01,0x01,0x83,0x00,0x06,
0xc2,0x01,0xbc,0x02,0x00,0x64,0x00,0x9c,0xff,0x00,0x00,0x08,0x64,0x00,0x0a,0xf4,
0x01,0xf4,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00
};
#endif 




void icn83xx_memdump(char *mem, int size)
{
    int i;
    for(i=0;i<size; i++)
    {
        if(i%16 == 0)
            printk("\n");
        printk(" 0x%2x", mem[i]);
    }
    printk("\n");   
} 

void RwFuncInit()
{
    printk("init rw func\n");
    g_RWFunc.ReadRegU8 = ReadRegU8;
    g_RWFunc.WriteRegU8 = WriteRegU8;
    g_RWFunc.ReadRegU16 = ReadRegU16;
    g_RWFunc.WriteRegU16 = WriteRegU16;
    g_RWFunc.ReadMem = ReadMem;
    g_RWFunc.WriteMem = WriteMem;
    g_RWFunc.ReadDumpU8 = ReadDumpU8;

    HostRWFuncInit(&g_RWFunc);  

}

unsigned char  ReadRegU8(char chip, unsigned short RegAddr, unsigned char *Value)
{
    int ret;
    ret = icn83xx_prog_i2c_rxdata(RegAddr, Value, 1);  
    if (ret < 0) {
        pr_err("ReadRegU8 failed! RegAddr:  0x%x, ret: %d\n", RegAddr, ret);
        return -1;
    }   
    slave_printk("ReadRegU8: [0x%x]= 0x%x\n",RegAddr, *Value);
    return *Value;  
}

int WriteRegU8(char chip, unsigned short RegAddr, unsigned char Value)
{
    char buf[3];
    int ret = -1;
    slave_printk("WriteRegU8: [0x%x]= 0x%x\n", RegAddr, Value);
    buf[0] = Value;
    ret = icn83xx_prog_i2c_txdata(RegAddr, buf, 1);
    if (ret < 0) {
        pr_err("WriteRegU8 failed! RegAddr 0x%x, Value: 0x%x, ret: %d\n", RegAddr, Value, ret);
        return -1;
    }
    return 0;
}


unsigned short ReadRegU16(char chip, unsigned short RegAddr, unsigned short *Value)
{
    char buf[3];
    int ret;
    ret = icn83xx_prog_i2c_rxdata(RegAddr, buf, 2); 
    if (ret < 0) {
        pr_err("ReadRegU16 failed! RegAddr:  0x%x, ret: %d\n", RegAddr, ret);
        return -1;
    }   
//  *Value = (buf[0]<<8)|buf[1];
    *Value = (buf[1]<<8)|buf[0];
    slave_printk("ReadRegU16: [0x%x]= 0x%x\n", RegAddr, *Value);
    return *Value;  
}

int WriteRegU16(char chip, unsigned short RegAddr, unsigned short Value)
{
    char buf[3];
    int ret = -1;
    slave_printk("WriteRegU16: [0x%x]= 0x%x\n", RegAddr, Value);
//    buf[0] = U16HIBYTE(Value);
//    buf[1] = U16LOBYTE(Value);
    buf[1] = U16HIBYTE(Value);
    buf[0] = U16LOBYTE(Value);  
    ret = icn83xx_prog_i2c_txdata(RegAddr, buf, 2);
    if (ret < 0) {
        pr_err("WriteRegU16 failed! RegAddr 0x%x, Value: 0x%x, ret: %d\n", RegAddr, Value, ret);
        return -1;
    }
    return 0;
}

int ReadMem(char chip, unsigned short StartAddr, unsigned char *Buffer, unsigned short Length)
{
    int ret;
    //int i;
    
    ret = icn83xx_prog_i2c_rxdata(StartAddr, Buffer, Length); 
    /*
    for(i=0;i<Length;i++)
    {
       ReadRegU8(chip,StartAddr+i,Buffer+i);
    }
    ret = 0;
    */
    if (ret < 0) {
        pr_err("\nReadMem failed! StartAddr:  0x%x, ret: %d\n", StartAddr, ret);
        return -1;
    }   
    slave_printk("\nReadMem, StartAddr 0x%x, Length: %d\n", StartAddr, Length);
    
//  icn83xx_memdump(Buffer, Length);
    return ret;  
}


int WriteMem(char chip, unsigned short StartAddr, unsigned char *Buffer, unsigned short Length)
{
    int ret = -1;
    slave_printk("WriteMem, StartAddr 0x%x, Length: %d\n", StartAddr, Length);
    ret = icn83xx_prog_i2c_txdata(StartAddr, Buffer, Length);
    if (ret < 0) {
        pr_err("\nWriteMem failed! StartAddr 0x%x, Value: 0x%x, ret: %d\n", StartAddr, Buffer[0], ret);
        return -1;
    }
    return 0;

}
unsigned char  ReadDumpU8(char chip, unsigned short RegAddr, unsigned char *Value)
{
    #if 0
    int ret;
    ret = icn83xx_i2c_rxdata((U8)RegAddr, Value, 1);     
    if (ret < 0) {
        pr_err("ReadDumpU8 failed! RegAddr:  0x%x, ret: %d\n", RegAddr, ret);
        return -1;
    }   
    slave_printk("ReadDumpU8: [0x%x]= 0x%x\n",RegAddr, *Value);
    return *Value;
    #endif
    return 1;  
}

/***********************************************************************************************
Name    :   icn83xx_open_config 
Input   :   *config parameter
            
Output  :   file size
function    :   open the cp file, and return total size
***********************************************************************************************/
int  icn83xx_open_config( char *cp)
{
    int file_size;
    mm_segment_t fs; 
    struct inode *inode = NULL; 
    
    if(strcmp(cp, "inner_para") == 0 || force_use_inner_para == 1)
    {
	    printk(KERN_ERR "############# use inner para \n");
        cp_mode = 1;  //use inner array
        return sizeof(icn83xx_cp);
    }
    else
    {
        cp_mode = 0; //use file in file system
    }
        
    
    
    fp = filp_open(cp, O_RDONLY, 0); 
    if (IS_ERR(fp)) { 
        printk("read cp file error\n"); 
        return -1; 
    } 
    else
        printk("open cp file ok\n"); 
        
    inode = fp->f_dentry->d_inode;
    file_size = inode->i_size;  
    printk("file size: %d\n", file_size); 

    fs = get_fs(); 
    set_fs(KERNEL_DS); 
    
    return  file_size;
    
}


/***********************************************************************************************
Name    :   icn83xx_close_config 
Input   :   
Output  :   
function    :   close file
***********************************************************************************************/
int  icn83xx_close_config(void)
{   
    if(cp_mode == 0)
    {
        filp_close(fp, NULL); 
    }
    return 0;
}

/***********************************************************************************************
Name    :   icn83xx_read_config 
Input   :   offset
            length, read length
            buf, return buffer
Output  :   
function    :   read data to buffer
***********************************************************************************************/
int  icn83xx_read_config(int offset, int length, char *buf)
{
    loff_t  pos = offset;         

    if(cp_mode == 1)
    {
        memcpy(buf, &icn83xx_cp[offset], length);
    }
    else
    {                   
        vfs_read(fp, buf, length, &pos); 
    }            
            
    return 0;       
}


int TP_SetPanelPara(void)
{
    int file_size;
    int struct_size;
    STRUCT_PANEL_PARA *structPanelParaPtr;

#if 1
// duodian=3, ruishensi=2 , feichu=1 , hd=600, md=480, for example hd-fc TP set tp_screen_type= 6001
    char * para_file;
    if( tp_screen_type == 6001 ){
	para_file = para_fc_hd;
    }else if( tp_screen_type == 4801 ){
	para_file = para_fc_md;
    }else if( tp_screen_type == 6002 ){
	para_file = para_rs_hd;
    }else if( tp_screen_type == 4802 ){
	para_file = para_rs_md;
    }else if( tp_screen_type == 6003 ){
	para_file = para_dd_hd;
    }else if( tp_screen_type == 4803 ){
	para_file = para_dd_md;
    }else if( tp_screen_type == 6004 ){
	para_file = para_u25gt_hd;
    }else{
	para_file = para;
    }
    printk("#########  icn838x use para: %s \n",para_file);
    file_size = icn83xx_open_config(para_file);
#else 
    file_size = icn83xx_open_config(para);
#endif
    if(file_size < 0)
    {
        printk("read para.bin error\n");
        return -1;     
    }  
    structPanelParaPtr = TP_GetPanelPara();
    struct_size = sizeof(STRUCT_PANEL_PARA);
    printk("struct_size : %d\n", struct_size); 
    printk("file size : %d\n", file_size); 
    
    if(file_size == struct_size)
    {        
	printk(KERN_ERR "########## cp para to ic\n");
        icn83xx_read_config(0, struct_size, (char *)structPanelParaPtr);
    }
    icn83xx_close_config();
    HostCommInit();

    return 0;
/*    
    int i;
    STRUCT_PANEL_PARA *structPanelParaPtr;
    unsigned char G_TX_ORDER[PHYSICAL_MAX_NUM_ROW] = TP_TX_ORDER;
    unsigned char G_RX_ORDER[PHYSICAL_MAX_NUM_COL] = TP_RX_ORDER;
    unsigned char G_VK_ORDER[PHYSICAL_MAX_VK_NUM] = TP_VK_ORDER;        
    unsigned char G_RX_FB_CAP[PHYSICAL_MAX_NUM_ROW] = TP_RX_FB_CAP;        //RX  CAP_ADJ
    unsigned char G_RX_DC_OFFSET[PHYSICAL_MAX_NUM_COL] = TP_RX_DC_OFFSET; //TX  CAP_SMP
    unsigned char G_PHASE_DELAY[PHYSICAL_MAX_NUM_ROW][PHYSICAL_MAX_NUM_COL] = DEFAULT_PHASE_DELAY;

    structPanelParaPtr = TP_GetPanelPara();
    structPanelParaPtr->u8RowNum = NUM_ROW_USED;
    structPanelParaPtr->u8ColNum = NUM_COL_USED;
    structPanelParaPtr->u8NumVKey = NUM_VK_USED;
    structPanelParaPtr->u8VKeyMode = VK_MODE;
    structPanelParaPtr->u8TpVkOrder[0] = G_VK_ORDER[0];
    structPanelParaPtr->u8TpVkOrder[1] = G_VK_ORDER[1];
    structPanelParaPtr->u8TpVkOrder[2] = G_VK_ORDER[2];
    structPanelParaPtr->u8TpVkOrder[3] = G_VK_ORDER[3]; 
    structPanelParaPtr->s16VKDownThreshold = VK_DOWN_THRESHOLD;
    structPanelParaPtr->s16VKUpThreshold = VK_UP_THRESHOLD;
    structPanelParaPtr->u16PositivePeakTh = DEFAULT_POSITIVE_PEAK_THRESHOLD;
    structPanelParaPtr->u16OutGroupThreshold = DEFAULT_OUT_GROUP_THRESHOLD;
    structPanelParaPtr->u16AreaThreshold = DEFAULT_PALM_THRESHOLD;
    structPanelParaPtr->s16TouchDownThresold = DEFAULT_TOUCH_DOWN_THRESHOLD;
    structPanelParaPtr->s16TouchUpThresold = DEFAULT_TOUCH_UP_THRESHOLD;
    structPanelParaPtr->u8FrqDiv[0] = DEFAULT_SCAN_F1;
    structPanelParaPtr->u16ResX = RESOLUTION_X ;
    structPanelParaPtr->u16ResY = RESOLUTION_Y ;
    memcpy(&structPanelParaPtr->u8TXOrder[0], G_TX_ORDER, NUM_ROW_USED);
    memcpy(&structPanelParaPtr->u8RXOrder[0], G_RX_ORDER, NUM_COL_USED); 
    
    memcpy(&structPanelParaPtr->u8ColFbCap[0], G_RX_FB_CAP, PHYSICAL_MAX_NUM_COL);
    for(i=0; i<8; i++)
    {
        structPanelParaPtr->u8ColDcOffset[i] = ((G_RX_DC_OFFSET[i<<1]<<4) |  G_RX_DC_OFFSET[(i<<1)+1]); 
    }   
    
    
    structPanelParaPtr->u8NumFilterEnable = NUM_FILTER_DEFAULT_STATE;
    structPanelParaPtr->u8NumFilterCntMax = NUM_FILTER_DEFAULT_CNT_MAX;
    
    structPanelParaPtr->u8DistanceFilterEnable = DISTANCE_FILTER_DEFAULT_STATE;
    structPanelParaPtr->u16DistanceFilterThreshold = DISTANCE_FILTER_DEFAULT_THRESHOLD;    
    
    structPanelParaPtr->u8GridFilterEnable = GRID_FILTER_DEFAULT_STATE;
    structPanelParaPtr->u8GridFilterCntMax = GRID_FILTER_DEFAULT_CNT_MAX;
    structPanelParaPtr->u8GridFilterSize = GRID_FILTER_DEFAULT_SIZE;
    structPanelParaPtr->u8NoReportStayTouch = NO_REPORT_STAY_TOUCH;
    
    structPanelParaPtr->u32AutoDetectBkNoiseTh = AUTO_DETECT_BKGND_NOISE_TH;
    
    structPanelParaPtr->u8PalmDetectionThreshold = PALM_DETECTION_THRESHOLD;
    structPanelParaPtr->u8WaterShedPersent = WATER_SHED_PERSENT;
    
    structPanelParaPtr->u8XySwap = DEFAULT_X_Y_SWAP;
    structPanelParaPtr->u8Scale = DEFAULT_SCALE;    

    structPanelParaPtr->u8TemperatureDriftThreshold = DEFAULT_TEMPERATURE_DF_TH;
    structPanelParaPtr->u8TemperatureDebunceInterval = DEFAULT_TEMPERATURE_DEBUNCE_INTERVAL;
    
    structPanelParaPtr->u16NagetiveThreshold = DEFAULT_NAGETIVE_THRESHOLD;
    structPanelParaPtr->u16BaseLineErrorMaxWaitCnt = DEFAULT_BASELINE_ERROR_MAX_WAIT_CNT;
    
    structPanelParaPtr->u8WaitTouchLeaveCnt = DEFAULT_WAIT_TOUCH_LEAVE_CNT;

    structPanelParaPtr->u8PalmRejectionEn = DEFAULT_PALM_REJECTION;

    memcpy(&structPanelParaPtr->u8PhaseDelay[0][0],&G_PHASE_DELAY[0][0],PHYSICAL_MAX_NUM_ROW*PHYSICAL_MAX_NUM_COL);    
   */     
}

