#ifndef _LCD_TX23D88VM_H_
#define _LCD_TX23D88VM_H_

/* Base */
#define SCREEN_TYPE		SCREEN_LVDS//SCREEN_RGB
#define LVDS_FORMAT		LVDS_8BIT_1//LVDS_6BIT//LVDS_8BIT_1
#define OUT_FACE		OUT_P888//OUT_P666
#define DCLK			 55687500//64000000
#define LCDC_ACLK        500000000           //29 lcdc axi DMA ÆµÂÊ

/* Timing */
#define H_PW			100
#define H_BP			150
#define H_VD			1024
#define H_FP			170

#define V_PW			10
#define V_BP			15
#define V_VD			600
#define V_FP			20

#define LCD_WIDTH       223
#define LCD_HEIGHT      125
/* Other */
#define DCLK_POL		0
#define DEN_POL			0
#define VSYNC_POL		0
#define HSYNC_POL		0

#define SWAP_RB			0
#define SWAP_RG			0
#define SWAP_GB			0


#endif
