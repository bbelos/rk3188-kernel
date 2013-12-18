#ifndef MKLINUXLOG

#ifndef _SCREEN_H
#define _SCREEN_H
#include <mach/board.h>

#ifdef CONFIG_HDMI_DUAL_DISP
/* Scaler PLL CONFIG */
#define S_PLL_NO_1	0
#define S_PLL_NO_2	1
#define S_PLL_NO_4	2
#define S_PLL_NO_8	3
#define S_PLL_M(x)  (((x)&0xff)<<8)
#define S_PLL_N(x)  (((x)&0xf)<<4)
#define S_PLL_NO(x) ((S_PLL_NO_##x)&0x3)

enum{
    HDMI_RATE_148500000,
    HDMI_RATE_74250000,
    HDMI_RATE_27000000,
};
/*     Scaler   clk setting */
#define SCALE_PLL(_parent_rate,_rate,_m,_n,_no) \
        HDMI_RATE_ ## _parent_rate ##_S_RATE_ ## _rate \
        =  S_PLL_M(_m) | S_PLL_N(_n) | S_PLL_NO(_no)    
#define SCALE_RATE(_parent_rate , _rate) \
        (HDMI_RATE_ ## _parent_rate ## _S_RATE_ ## _rate)
        
enum{
    SCALE_PLL(148500000,    66000000,   16, 9,  4),
    SCALE_PLL(148500000,    57375000,   17, 11, 4),
    SCALE_PLL(148500000,    54000000,   16, 11, 4),    
    SCALE_PLL(148500000,    33000000,   16, 9,  8),
    SCALE_PLL(148500000,    30375000,   18, 11, 8),
    SCALE_PLL(148500000,    29700000,   16, 10, 8),
    SCALE_PLL(148500000,    25312500,   15, 11, 8),

    SCALE_PLL(74250000,     66000000,   32, 9,  4),
    SCALE_PLL(74250000,     57375000,   34, 11, 4),
    SCALE_PLL(74250000,     54000000,   32, 11, 4),
    SCALE_PLL(74250000,     33000000,   32, 9,  8),
    SCALE_PLL(74250000,     30375000,   36, 11, 8),
    SCALE_PLL(74250000,     25312500,   30, 11, 8),

    SCALE_PLL(27000000,     75000000,   100, 9,  4),
    SCALE_PLL(27000000,     72000000,   32, 3,  4),
    SCALE_PLL(27000000,     63281250,   75, 4,  8),
    SCALE_PLL(27000000,     54375000,   145, 9,  8),
    SCALE_PLL(27000000,     31500000,   28, 3,  8),
    SCALE_PLL(27000000,     30000000,   80, 9,  8),
};
#endif
typedef enum _SCREEN_TYPE {
    SCREEN_NULL = 0,
    SCREEN_RGB,
    SCREEN_LVDS,
	SCREEN_MCU,
    SCREEN_TVOUT,
    SCREEN_HDMI,
} SCREEN_TYPE;

typedef enum _REFRESH_STAGE {
    REFRESH_PRE = 0,
    REFRESH_END,

} REFRESH_STAGE;


typedef enum _MCU_IOCTL {
    MCU_WRCMD = 0,
    MCU_WRDATA,
    MCU_SETBYPASS,

} MCU_IOCTL;


typedef enum _MCU_STATUS {
    MS_IDLE = 0,
    MS_MCU,
    MS_EBOOK,
    MS_EWAITSTART,
    MS_EWAITEND,
    MS_EEND,

} MCU_STATUS;



/* Screen description */
typedef struct rk29fb_screen {
    /* screen type & hardware connect format & out face */
    u16 type;
    u16 hw_format;
    u16 face;

	/* Screen size */
	u16 x_res;
	u16 y_res;
    u16 width;
    u16 height;

    u32 mode;
    /* Timing */
	u32 pixclock;
	u16 left_margin;
	u16 right_margin;
	u16 hsync_len;
	u16 upper_margin;
	u16 lower_margin;
	u16 vsync_len;
#ifdef CONFIG_HDMI_DUAL_DISP
    /* Scaler mode Timing */
	u32 s_pixclock;
	u16 s_left_margin;
	u16 s_right_margin;
	u16 s_hsync_len;
	u16 s_upper_margin;
	u16 s_lower_margin;
	u16 s_vsync_len; 
	u16 s_hsync_st;
	u16 s_vsync_st;
	bool s_den_inv;
	bool s_hv_sync_inv;
	bool s_clk_inv;
#endif
	u8 hdmi_resolution;
    /* mcu need */
	u8 mcu_wrperiod;
    u8 mcu_usefmk;
    u8 mcu_frmrate;

	/* Pin polarity */
	u8 pin_hsync;
	u8 pin_vsync;
	u8 pin_den;
	u8 pin_dclk;
    u32 lcdc_aclk;
	u8 pin_dispon;

	/* Swap rule */
    u8 swap_rb;
    u8 swap_rg;
    u8 swap_gb;
    u8 swap_delta;
    u8 swap_dumy;

    /* Operation function*/
    int (*init)(void);
    int (*standby)(u8 enable);
    int (*refresh)(u8 arg);
    int (*scandir)(u16 dir);
    int (*disparea)(u8 area);
    int (*sscreen_get)(struct rk29fb_screen *screen, u8 resolution);
    int (*sscreen_set)(struct rk29fb_screen *screen, bool type);// 1: use scaler 0:bypass
} rk_screen;

extern void set_lcd_info(struct rk29fb_screen *screen, struct rk29lcd_info *lcd_info);
extern void set_tv_info(struct rk29fb_screen *screen);
extern void set_hdmi_info(struct rk29fb_screen *screen);

#endif

#else

/* 输往屏的数据格式 */
#define OUT_P888            0
#define OUT_P666            1    //666的屏, 接DATA0-17
#define OUT_P565            2    //565的屏, 接DATA0-15
#define OUT_S888x           4
#define OUT_CCIR656         6
#define OUT_S888            8
#define OUT_S888DUMY        12
#define OUT_P16BPP4         24  //模拟方式,控制器并不支持
#define OUT_D888_P666       0x21  //666的屏, 接DATA2-7, DATA10-15, DATA17-22
#define OUT_D888_P565       0x22  //565的屏, 接DATA3-7, DATA10-15, DATA18-22

 #if defined (CONFIG_LCD_BP082WX1_100)
#include "lcd_BP082WX1_100.h"
 #else

/* Base */
#define OUT_TYPE	    SCREEN_RGB

#define OUT_FACE	    OUT_D888_P666


#define OUT_CLK	          86000000
#define LCDC_ACLK        300000000           //29 lcdc axi DMA Ƶ��

/* Timing */
#define H_PW			10
#define H_BP			100
#define H_VD			1280
#define H_FP			18

#define V_PW			2
#define V_BP			8
#define V_VD			800
#define V_FP			6

#define LCD_WIDTH          216
#define LCD_HEIGHT         135
/* Other */
#define DCLK_POL		0
#define SWAP_RB		0

 #endif

#endif
