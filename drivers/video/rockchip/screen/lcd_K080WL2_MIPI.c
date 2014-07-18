#ifndef __LCD_K080WL2_MIPI__
#define __LCD_K080WL2_MIPI__

#if defined(CONFIG_MIPI_DSI)
#include "../transmitter/mipi_dsi.h"
#endif
#include <mach/io.h>
#include <mach/gpio.h>
#include <mach/iomux.h>
#include <linux/delay.h>

#define SCREEN_TYPE	    	SCREEN_MIPI
#define LVDS_FORMAT         0//mipi lcd don't need it, so 0 would be ok.
#define OUT_FACE	    	OUT_P888

#define DCLK_RK3188_T	    80*1000000//60*1000000//150*1000000
#define DCLK_RK3188         75*1000000
#define LCDC_ACLK         	300000000           //29 lcdc axi DMA ÆµÂÊ

/* Timing */
#define H_PW			20
#define H_BP			140
#define H_VD			800
#define H_FP			32

#define V_PW			4
#define V_BP		        6
#define V_VD			1280
#define V_FP			8

#define LCD_WIDTH          	107
#define LCD_HEIGHT         	172
/* Other */
#define DCLK_POL	1
#define DEN_POL		0
#define VSYNC_POL	0
#define HSYNC_POL	0

#define SWAP_RB		0
#define SWAP_RG		0
#define SWAP_GB		0

#define RK_SCREEN_INIT 	1

/* about mipi */
#define MIPI_DSI_LANE 4
#define MIPI_DSI_HS_CLK  460*1000000//1000*1000000

#if defined(RK_SCREEN_INIT)
static struct rk29lcd_info *gLcd_info = NULL;

int rk_lcd_init(void) {

	int ret = 0;
	u8 dcs[32] = {0};
	if(dsi_is_active() != 1)
		return -1;

	/*below is changeable*/
//	msleep(50);	
	dsi_enable_hs_clk(1);
//    	msleep(20);

	dcs[0] = LPDT;
	dcs[1] = dcs_exit_sleep_mode; 
	dsi_send_dcs_packet(dcs, 2);
	//msleep(120);

	dcs[0] = LPDT;
	dcs[1] = dcs_set_display_on;
	dsi_send_dcs_packet(dcs, 2);
	//msleep(200);    
    
    	dcs[0] = LPDT;
    	dcs[1] = 0xF0;
    	dcs[2] = 0x5A;
    	dcs[3] = 0x5A;
    	dsi_send_dcs_packet(dcs, 4);

    	dcs[0] = LPDT;
    	dcs[1] = 0xF1;
    	dcs[2] = 0x5A;
    	dcs[3] = 0x5A;
    	dsi_send_dcs_packet(dcs, 4);

   	dcs[0] = LPDT;
    	dcs[1] = 0xFC;
    	dcs[2] = 0xA5;
    	dcs[3] = 0xA5;
    	dsi_send_dcs_packet(dcs, 4);

    	dcs[0] = LPDT;
    	dcs[1] = 0xD0;
    	dcs[2] = 0x00;
    	dcs[3] = 0x10;
    	dsi_send_dcs_packet(dcs, 4);

    	dcs[0] = LPDT;
    	dcs[1] = 0x35;
    	dsi_send_dcs_packet(dcs, 2);

    	dcs[0] = LPDT;
    	dcs[1] = 0xC3;
    	dcs[2] = 0x40;
    	dcs[3] = 0x00;
    	dcs[4] = 0x28;
    	dsi_send_dcs_packet(dcs, 5);
    	msleep(20);

    	/*0xF6, 0x63, 0x20, 0x86, 0x00, 0x00, 0x10*/
    	dcs[0] = LPDT;
    	dcs[1] = 0xF6;
    	dcs[2] = 0x63;
    	dcs[3] = 0x20;
    	dcs[4] = 0x86;
    	dcs[5] = 0x00;
    	dcs[6] = 0x00;
    	dcs[7] = 0x10;
    	dsi_send_dcs_packet(dcs, 8);

	/* MIPI Video on, no need care about it */
	/* 0x11. Sleep out */
    	dcs[0] = LPDT;
    	dcs[1] = 0x11;
    	dsi_send_dcs_packet(dcs, 2);
    	msleep(120);
    	//msleep(150);

    	dcs[0] = LPDT;
    	dcs[1] = 0x36;
    	dcs[2] = 0x00;
    	dsi_send_dcs_packet(dcs, 3);

    	dcs[0] = LPDT;
    	dcs[1] = 0x29;
    	dsi_send_dcs_packet(dcs, 2);
    	msleep(200);

   	//dsi_enable_command_mode(0);
	dsi_enable_video_mode(1);
	
	printk("++++++++++++++++%s:%d\n", __func__, __LINE__);
	return 0;
}

int rk_lcd_standby(u8 enable) {

	u8 dcs[16] = {0};
	if(dsi_is_active() != 1)
		return -1;
		
	if(enable) {
	/*below is changeable*/
		dcs[0] = LPDT;
		dcs[1] = dcs_set_display_off; 
		dsi_send_dcs_packet(dcs, 2);
		msleep(1);
		dcs[0] = LPDT;
		dcs[1] = dcs_enter_sleep_mode; 
		dsi_send_dcs_packet(dcs, 2);
		msleep(1);
	} else {
		/*below is changeable*/
		rk_lcd_init();
		printk("++++++++++++++++%s:%d\n", __func__, __LINE__);	
	}
}
#endif  

#endif  
