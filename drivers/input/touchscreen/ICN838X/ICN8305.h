/*++
 
 Copyright (c) 2012-2022 ChipOne Technology (Beijing) Co., Ltd. All Rights Reserved.
 This PROPRIETARY SOFTWARE is the property of ChipOne Technology (Beijing) Co., Ltd. 
 and may contains trade secrets and/or other confidential information of ChipOne 
 Technology (Beijing) Co., Ltd. This file shall not be disclosed to any third party,
 in whole or in part, without prior written consent of ChipOne.  
 THIS PROPRIETARY SOFTWARE & ANY RELATED DOCUMENTATION ARE PROVIDED AS IS, 
 WITH ALL FAULTS, & WITHOUT WARRANTY OF ANY KIND. CHIPONE DISCLAIMS ALL EXPRESS OR 
 IMPLIED WARRANTIES.  
 
 File Name:    icn8305.h
 Abstract:
     definition.
 Author:       JunFu Zhang
               Zhimin Tian
 Date :        12 24,2012
 Version:      1.0 for sensor only
 History :
     03 28,2012,   0.1[first revision]
    
 
 --*/


#ifndef _ICN8305_H_
#define  _ICN8305_H_

#include "constant.h"


#ifdef _MAIN_C_

U8   CHIP_VER          ;//  (0x0000);       //Chip Version Register.
U8   SFRSTCFG          ;//  (0x0001);       //Software Reset Configuration
U8   OSCRTC_CFG3       ;//  (0x0003);       //OSCRTC Configuration Register 0
U8   SYSCLKCFG         ;//  (0x0004);       //Clock gating and Invert Control Register
U8   DIVA              ;//  (0x0005);       //Clock Divider A Setting 
U8   BISTCFG1          ;//  (0x0008);       //Memory Bist Configuration 1
U8   CLKGATECFG        ;//  (0x0009);       //Module Clock gating Configuration Register
U8   DIVC1             ;//  (0x000A);       //OSCRTC Configuration Register 1
U8   DIVD              ;//  (0x000D);       //OSCRTC Configuration Register 1
U8   DIVE              ;//  (0x000E);       //OSCRTC Configuration Register 1
U8   CLKSEL            ;//  (0x000F);       //OSCRTC Configuration Register 1
U8   SPI_PRGM_DEGL_EN  ;//  (0x0019);       //OSCRTC Configuration Register 1
U8   OSCRTC_CFG1       ;//  (0x001a);       //OSCRTC Configuration Register 1
U8   OSCRTC_CFG2       ;//  (0x001b);       //OSCRTC Configuration Register 1
U8   BISTCFG2          ;//  (0x001d);        //Memory Bist Configuration Register 2
U8   DIG_RST_CFG       ;//  (0x001e);       //Digital Self Reset Configuration
U8   ENDIAN_CFG        ;//  (0x001f);        //Data Memory Endian Configuration Register
U8   RW_CFG            ;//  (0x0020);        //Read/Write Configuration Register
U8   DIV_LOAD          ;//  (0x0021);        //Div-Registers Configuration Register
U8   I2C_CFG           ;//  (0x0022);        //I2c Configuration Register
U8   D_PD              ;//  (0x0023);        //Digital Power-down Register

U8   I2C_WATDOG_NUM_H  ;//  (0x0024);        //The high 8-bit threshold of I2C watchdog
U8   I2C_WATDOG_NUM_L  ;//  (0x0025);        //The low 8-bit threshold of I2C watchdog.
U8   SINGLE_READ       ;//  (0x0026);        //Single Read Mode Configuration
U8   REDUND0           ;//  (0x0050);
U8   OSC_LDO_SEL       ;//  (0x0051);
U8   REDUND2           ;//  (0x0052);
U8   REDUND3           ;//  (0x0053);


#define INT_BASE                          0x100
U8   INT0CTRL_L           ;//  (INT_BASE+0x0000);       //0x0000 ~ 0x0001 16 UPINT0 Interrupt Enable register 
U8   INT0CTRL_H           ;//  (INT_BASE+0x0001);
U8   INT0STS_L            ;//  (INT_BASE+0x0002);       //0x0002 ~ 0x0003 16 UPINT0 Interrupt Status register 
U8   INT0STS_H            ;//  (INT_BASE+0x0003); 
U8   INT1CTRL_L           ;//  (INT_BASE+0x0004);       //0x0004 ~ 0x0005 16 UPINT1 Interrupt Enable register 
U8   INT1CTRL_H           ;//  (INT_BASE+0x0005);
U8   INT1STS_L            ;//  (INT_BASE+0x0006);       //0x0006 ~ 0x0007 16 UPINT1 Interrupt Status register 
U8   INT1STS_H            ;//  (INT_BASE+0x0007); 
U8   INT1DEGEN_L          ;//  (INT_BASE+0x0008);       //0x0008 ~ 0x0009 16 UPINT1 Interrupt Deglitch Configuration register 
U8   INT1DEGEN_H          ;//  (INT_BASE+0x0009);

//GPIO 
#define GPIO_BASE                         0xF00
U8   BOND_NORMSTRAPOP     ;//  (GPIO_BASE+0x0000);     //Bonding / Normal Strapping Option Register   
U8   PINMUX_L             ;//  (GPIO_BASE+0x0002);     //0x0002 ~0x0003 16 Debug Signal Output Enable Register 
U8   DBGENCFG             ;//  (GPIO_BASE+0x0004);     //Debug Signal Output Enable Register 
U8   WATLVL1              ;//  (GPIO_BASE+0x0005);     //Debug Signal Select Level 1 Register 
U8   WATLVL2              ;//  (GPIO_BASE+0x0006);     //Debug Signal Select Level 2 Register 
U8   GIOENCFG_L           ;//  (GPIO_BASE+0x0007);     //0x0007 ~ 0x0008 16 GPIO Output Enable Register  
U8   GIODIRCFG_L          ;//  (GPIO_BASE+0x0009);     //0x0009 ~ 0x000A 16 GPIO Input/Output Direction Select Register  
U8   GIOOUTDATA_L         ;//  (GPIO_BASE+0x000B);     //0x000B ~ 0x000C 16 GPIO Output Data Register  
U8   GIOINDATA_L          ;//  (GPIO_BASE+0x000D);     //0x000D ~ 0x000E  16 GPIO Input Data Register   
U8   PADPD_L              ;//  (GPIO_BASE+0x000F);     //0x000F ~ 0x0010 16 Pad Pull Down Register  
U8   PADPD_H              ;//  (GPIO_BASE+0x000F);     //0x000F ~ 0x0010 16 Pad Pull Down Register  
U8   WAKEUPEN_L           ;//  (GPIO_BASE+0x0011);     //0X0011 ~ 0x0012 16 Wake Up Enable Register  
U8   WAKEPOLCFG_L         ;//  (GPIO_BASE+0x0013);     //0X0013 ~ 0x0014 16 Wake Up Pad Polarity Register 
U8   MISC_CFG             ;//  (GPIO_BASE+0x0017);     //Miscellaneous Configuration 
U8   OSC_FT_CNT_L         ;//  (GPIO_BASE+0x0018);     //0X0018 ~ 0x0019  16  Oscillator Function Test Counter Register 
U8   OSC_FT_CNT_H         ;//  (GPIO_BASE+0x0019);

U16 P_CHIP_VER          = (0x0000);       //Chip Version Register.
U16 P_SFRSTCFG          = (0x0001);       //Software Reset Configuration
U16 P_OSCRTC_CFG3       = (0x0003);       //OSCRTC Configuration Register 0
U16 P_SYSCLKCFG         = (0x0004);       //Clock gating and Invert Control Register
U16 P_DIVA              = (0x0005);       //Clock Divider A Setting 
U16 P_BISTCFG1          = (0x0008);       //Memory Bist Configuration 1
U16 P_CLKGATECFG        = (0x0009);       //Module Clock gating Configuration Register
U16 P_DIVC1             = (0x000A);       //OSCRTC Configuration Register 1
U16 P_DIVD              = (0x000D);       //OSCRTC Configuration Register 1
U16 P_DIVE              = (0x000E);       //OSCRTC Configuration Register 1
U16 P_CLKSEL            = (0x000F);       //OSCRTC Configuration Register 1
U16 P_SPI_PRGM_DEGL_EN  = (0x0019);       //OSCRTC Configuration Register 1
U16 P_OSCRTC_CFG1       = (0x001a);       //OSCRTC Configuration Register 1
U16 P_OSCRTC_CFG2       = (0x001b);       //OSCRTC Configuration Register 1
U16 P_BISTCFG2          = (0x001d);       //Memory Bist Configuration Register 2
U16 P_DIG_RST_CFG       = (0x001e);       //Digital Self Reset Configuration

U16 P_ENDIAN_CFG        = (0x001f);       //Data Memory Endian Configuration Register

U16 P_RW_CFG            = (0x0020);       //Read/Write Configuration Register
U16 P_DIV_LOAD          = (0x0021);       //Div-Registers Configuration Register
U16 P_I2C_CFG           = (0x0022);       //I2c Configuration Register
U16 P_D_PD              = (0x0023);       //Digital Power-down Register

U16 P_I2C_WATDOG_NUM_H  = (0x0024);       //  (0x0024);        //The high 8-bit threshold of I2C watchdog
U16 P_I2C_WATDOG_NUM_L  = (0x0025);       //  (0x0025);        //The low 8-bit threshold of I2C watchdog.
U16 P_SINGLE_READ       = (0x0026);       //  (0x0026);        //Single Read Mode Configuration
U16 P_REDUND0           = (0x0050);       //  (0x0050);
U16 P_OSC_LDO_SEL       = (0x0051);       //  (0x0051);
U16 P_REDUND2           = (0x0052);       //  (0x0052);
U16 P_REDUND3           = (0x0053);       //  (0x0053);


//#define INT_BASE     0x100
U16 P_INT0CTRL_L           = (INT_BASE+0x0000);       //0x0000 ~ 0x0001 16 UPINT0 Interrupt Enable register 
U16 P_INT0CTRL_H           = (INT_BASE+0x0001);
U16 P_INT0STS_L            = (INT_BASE+0x0002);       //0x0002 ~ 0x0003 16 UPINT0 Interrupt Status register 
U16 P_INT0STS_H            = (INT_BASE+0x0003); 
U16 P_INT1CTRL_L           = (INT_BASE+0x0004);       //0x0004 ~ 0x0005 16 UPINT1 Interrupt Enable register 
U16 P_INT1CTRL_H           = (INT_BASE+0x0005);
U16 P_INT1STS_L            = (INT_BASE+0x0006);       //0x0006 ~ 0x0007 16 UPINT1 Interrupt Status register 
U16 P_INT1STS_H            = (INT_BASE+0x0007); 
U16 P_INT1DEGEN_L          = (INT_BASE+0x0008);       //0x0008 ~ 0x0009 16 UPINT1 Interrupt Deglitch Configuration register 
U16 P_INT1DEGEN_H          = (INT_BASE+0x0009);

//GPIO 
//#define GPIO_BASE         0xF00
U16 P_BOND_NORMSTRAPOP     = (GPIO_BASE+0x0000);     //Bonding / Normal Strapping Option Register   
U16 P_PINMUX_L             = (GPIO_BASE+0x0002);     //0x0002 ~0x0003 16 Debug Signal Output Enable Register 
U16 P_DBGENCFG             = (GPIO_BASE+0x0004);     //Debug Signal Output Enable Register 
U16 P_WATLVL1              = (GPIO_BASE+0x0005);     //Debug Signal Select Level 1 Register 
U16 P_WATLVL2              = (GPIO_BASE+0x0006);     //Debug Signal Select Level 2 Register 
U16 P_GIOENCFG_L           = (GPIO_BASE+0x0007);     //0x0007 ~ 0x0008 16 GPIO Output Enable Register  
U16 P_GIODIRCFG_L          = (GPIO_BASE+0x0009);     //0x0009 ~ 0x000A 16 GPIO Input/Output Direction Select Register  
U16 P_GIOOUTDATA_L         = (GPIO_BASE+0x000B);     //0x000B ~ 0x000C 16 GPIO Output Data Register  
U16 P_GIOINDATA_L          = (GPIO_BASE+0x000D);     //0x000D ~ 0x000E  16 GPIO Input Data Register 
U16 P_PADPD_L              = (GPIO_BASE+0x000F);     //0x000F ~ 0x0010 16 Pad Pull Down Register 
U16 P_PADPD_H              = (GPIO_BASE+0x0010);     //0x000F ~ 0x0010 16 Pad Pull Down Register 
U16 P_WAKEUPEN_L           = (GPIO_BASE+0x0011);     //0X0011 ~ 0x0012 16 Wake Up Enable Register  
U16 P_WAKEPOLCFG_L         = (GPIO_BASE+0x0013);     //0X0013 ~ 0x0014 16 Wake Up Pad Polarity Register 
U16 P_MISC_CFG             = (GPIO_BASE+0x0017);     //Miscellaneous Configuration 
U16 P_OSC_FT_CNT_L        = (GPIO_BASE+0x0018);     //0X0018 ~ 0x0019  16  Oscillator Function Test Counter Register 
U16 P_OSC_FT_CNT_H         = (GPIO_BASE+0x0019);

#else

extern U8   CHIP_VER          ;//;//  (0x0000);       //Chip Version Register.
extern U8   SFRSTCFG          ;//;//  (0x0001);       //Software Reset Configuration
extern U8   OSCRTC_CFG3       ;//;//  (0x0003);       //OSCRTC Configuration Register 0
extern U8   SYSCLKCFG         ;//;//  (0x0004);       //Clock gating and Invert Control Register
extern U8   DIVA              ;//;//  (0x0005);       //Clock Divider A Setting 
extern U8   BISTCFG1          ;//;//  (0x0008);       //Memory Bist Configuration 1
extern U8   CLKGATECFG        ;//;//  (0x0009);       //Module Clock gating Configuration Register
extern U8   DIVC1             ;//;//  (0x000A);       //OSCRTC Configuration Register 1
extern U8   DIVD              ;//;//  (0x000D);       //OSCRTC Configuration Register 1
extern U8   DIVE              ;//;//  (0x000E);       //OSCRTC Configuration Register 1
extern U8   CLKSEL            ;//  (0x000F);       //OSCRTC Configuration Register 1
extern U8   SPI_PRGM_DEGL_EN  ;//;//  (0x0019);       //OSCRTC Configuration Register 1
extern U8   OSCRTC_CFG1       ;//;//  (0x001a);       //OSCRTC Configuration Register 1
extern U8   OSCRTC_CFG2       ;//;//  (0x001b);       //OSCRTC Configuration Register 1
extern U8   BISTCFG2          ;//;//  (0x001d);        //Memory Bist Configuration Register 2
extern U8   DIG_RST_CFG       ;//;//  (0x001e);       //Digital Self Reset Configuration
extern U8   ENDIAN_CFG        ;//;//  (0x001f);        //Data Memory Endian Configuration Register
extern U8   RW_CFG            ;//;//  (0x0020);        //Read/Write Configuration Register
extern U8   DIV_LOAD          ;//;//  (0x0021);        //Div-Registers Configuration Register
extern U8   I2C_CFG           ;//;//  (0x0022);        //I2c Configuration Register
extern U8   D_PD              ;//;//  (0x0023);        //Digital Power-down Register
extern U8   I2C_WATDOG_NUM_H     ;//  (0x0024);        //The high 8-bit threshold of I2C watchdog
extern U8   I2C_WATDOG_NUM_L     ;//  (0x0025);        //The low 8-bit threshold of I2C watchdog.
extern U8   SINGLE_READ          ;//  (0x0026);        //Single Read Mode Configuration
extern U8   REDUND0              ;//  (0x0050);
extern U8   OSC_LDO_SEL              ;//  (0x0051);
extern U8   REDUND2              ;//  (0x0052);
extern U8   REDUND3              ;//  (0x0053);


//#define INT_BASE                          0x100
extern U8   INT0CTRL_L           ;//;//  (INT_BASE+0x0000);       //0x0000 ~ 0x0001 16 UPINT0 Interrupt Enable register 
extern U8   INT0CTRL_H           ;//;//  (INT_BASE+0x0001);
extern U8   INT0STS_L            ;//;//  (INT_BASE+0x0002);       //0x0002 ~ 0x0003 16 UPINT0 Interrupt Status register 
extern U8   INT0STS_H            ;//;//  (INT_BASE+0x0003); 
extern U8   INT1CTRL_L           ;//;//  (INT_BASE+0x0004);       //0x0004 ~ 0x0005 16 UPINT1 Interrupt Enable register 
extern U8   INT1CTRL_H           ;//;//  (INT_BASE+0x0005);
extern U8   INT1STS_L            ;//;//  (INT_BASE+0x0006);       //0x0006 ~ 0x0007 16 UPINT1 Interrupt Status register 
extern U8   INT1STS_H            ;//;//  (INT_BASE+0x0007); 
extern U8   INT1DEGEN_L          ;//;//  (INT_BASE+0x0008);       //0x0008 ~ 0x0009 16 UPINT1 Interrupt Deglitch Configuration register 
extern U8   INT1DEGEN_H          ;//;//  (INT_BASE+0x0009);


//GPIO 
//#define GPIO_BASE                         0xF00
extern U8   BOND_NORMSTRAPOP     ;//;//  (GPIO_BASE+0x0000);     //Bonding / Normal Strapping Option Register   
extern U8   PINMUX_L             ;//;//  (GPIO_BASE+0x0002);     //0x0002 ~0x0003 16 Debug Signal Output Enable Register 
extern U8   DBGENCFG             ;//;//  (GPIO_BASE+0x0004);     //Debug Signal Output Enable Register 
extern U8   WATLVL1              ;//;//  (GPIO_BASE+0x0005);     //Debug Signal Select Level 1 Register 
extern U8   WATLVL2              ;//;//  (GPIO_BASE+0x0006);     //Debug Signal Select Level 2 Register 
extern U8   GIOENCFG_L           ;//;//  (GPIO_BASE+0x0007);     //0x0007 ~ 0x0008 16 GPIO Output Enable Register  
extern U8   GIODIRCFG_L          ;//;//  (GPIO_BASE+0x0009);     //0x0009 ~ 0x000A 16 GPIO Input/Output Direction Select Register  
extern U8   GIOOUTDATA_L         ;//;//  (GPIO_BASE+0x000B);     //0x000B ~ 0x000C 16 GPIO Output Data Register  
extern U8   GIOINDATA_L          ;//;//  (GPIO_BASE+0x000D);     //0x000D ~ 0x000E  16 GPIO Input Data Register 
extern U8   PADPD_L              ;//;//  (GPIO_BASE+0x000F);     //0x000F ~ 0x0010 16 Pad Pull Down Register  
extern U8   PADPD_H              ;//;//  (GPIO_BASE+0x0010);     //0x000F ~ 0x0010 16 Pad Pull Down Register  
extern U8   WAKEUPEN_L           ;//;//  (GPIO_BASE+0x0011);     //0X0011 ~ 0x0012 16 Wake Up Enable Register  
extern U8   WAKEPOLCFG_L         ;//;//  (GPIO_BASE+0x0013);     //0X0013 ~ 0x0014 16 Wake Up Pad Polarity Register 
extern U8   MISC_CFG             ;//;//  (GPIO_BASE+0x0017);     //Miscellaneous Configuration 
extern U8   OSC_FT_CNT_L         ;//;//  (GPIO_BASE+0x0018);     //0X0018 ~ 0x0019  16  Oscillator Function Test Counter Register 
extern U8   OSC_FT_CNT_H         ;//;//  (GPIO_BASE+0x0019);

extern U16 P_CHIP_VER          ;//= (0x0000);       //Chip Version Register.
extern U16 P_SFRSTCFG          ;//= (0x0001);       //Software Reset Configuration
extern U16 P_OSCRTC_CFG3       ;//= (0x0003);       //OSCRTC Configuration Register 0
extern U16 P_SYSCLKCFG         ;//= (0x0004);       //Clock gating and Invert Control Register
extern U16 P_DIVA              ;//= (0x0005);       //Clock Divider A Setting 
extern U16 P_BISTCFG1          ;//= (0x0008);       //Memory Bist Configuration 1
extern U16 P_CLKGATECFG        ;//= (0x0009);       //Module Clock gating Configuration Register
extern U16 P_DIVC1             ;//= (0x000A);       //OSCRTC Configuration Register 1
extern U16 P_DIVD              ;//= (0x000D);       //OSCRTC Configuration Register 1
extern U16 P_DIVE              ;//= (0x000E);       //OSCRTC Configuration Register 1
extern U16 P_CLKSEL            ;//= (0x000F);       //OSCRTC Configuration Register 1
extern U16 P_SPI_PRGM_DEGL_EN  ;//;//  (0x0019);       //OSCRTC Configuration Register 1
extern U16 P_OSCRTC_CFG1       ;//= (0x001a);       //OSCRTC Configuration Register 1
extern U16 P_OSCRTC_CFG2       ;//= (0x001b);       //OSCRTC Configuration Register 1
extern U16 P_FETCH_DELAY_CFG   ;//= (0x001c);       //OSCRTC Configuration Register 1
extern U16 P_BISTCFG2          ;//= (0x001d);        //Memory Bist Configuration Register 2
extern U16 P_DIG_RST_CFG       ;//= (0x001e);          //Digital Self Reset Configuration
extern U16 P_ENDIAN_CFG        ;//= (0x001f);        //Data Memory Endian Configuration Register
extern U16 P_RW_CFG            ;//= (0x0020);        //Read/Write Configuration Register
extern U16 P_DIV_LOAD          ;//= (0x0021);        //Div-Registers Configuration Register
extern U16 P_I2C_CFG           ;//= (0x0022);        //I2c Configuration Register
extern U16 P_D_PD              ;//= (0x0023);        //Digital Power-down Register
extern U16 P_I2C_WATDOG_NUM_H  ;//= (0x0024);        //  (0x0024);        //The high 8-bit threshold of I2C watchdog
extern U16 P_I2C_WATDOG_NUM_L  ;//= (0x0025);        //  (0x0025);        //The low 8-bit threshold of I2C watchdog.
extern U16 P_SINGLE_READ       ;//= (0x0026);   //  (0x0026);        //Single Read Mode Configuration
extern U16 P_REDUND0           ;//= (0x0050);   //  (0x0050);
extern U16 P_OSC_LDO_SEL       ;//= (0x0051);   //  (0x0051);
extern U16 P_REDUND2           ;//= (0x0052);   //  (0x0052);
extern U16 P_REDUND3           ;//= (0x0053);   //  (0x0053);

//#define INT_BASE             0x100
extern U16 P_INT0CTRL_L           ;//= (INT_BASE+0x0000);       //0x0000 ~ 0x0001 16 UPINT0 Interrupt Enable register 
extern U16 P_INT0CTRL_H           ;//= (INT_BASE+0x0001);
extern U16 P_INT0STS_L            ;//= (INT_BASE+0x0002);       //0x0002 ~ 0x0003 16 UPINT0 Interrupt Status register 
extern U16 P_INT0STS_H            ;//= (INT_BASE+0x0003); 
extern U16 P_INT1CTRL_L           ;//= (INT_BASE+0x0004);       //0x0004 ~ 0x0005 16 UPINT1 Interrupt Enable register 
extern U16 P_INT1CTRL_H           ;//= (INT_BASE+0x0005);
extern U16 P_INT1STS_L            ;//= (INT_BASE+0x0006);       //0x0006 ~ 0x0007 16 UPINT1 Interrupt Status register 
extern U16 P_INT1STS_H            ;//= (INT_BASE+0x0007); 
extern U16 P_INT1DEGEN_L          ;//= (INT_BASE+0x0008);       //0x0008 ~ 0x0009 16 UPINT1 Interrupt Deglitch Configuration register 
extern U16 P_INT1DEGEN_H          ;//= (INT_BASE+0x0009);

//GPIO 
//#define GPIO_BASE                         0xF00
extern U16 P_BOND_NORMSTRAPOP     ;//= (GPIO_BASE+0x0000);     //Bonding / Normal Strapping Option Register   
extern U16 P_PINMUX_L             ;//= (GPIO_BASE+0x0002);     //0x0002 ~0x0003 16 Debug Signal Output Enable Register 
extern U16 P_DBGENCFG             ;//= (GPIO_BASE+0x0004);     //Debug Signal Output Enable Register 
extern U16 P_WATLVL1              ;//= (GPIO_BASE+0x0005);     //Debug Signal Select Level 1 Register 
extern U16 P_WATLVL2              ;//= (GPIO_BASE+0x0006);     //Debug Signal Select Level 2 Register
extern U16 P_GIOENCFG_L           ;//= (GPIO_BASE+0x0007);     //0x0007 ~ 0x0008 16 GPIO Output Enable Register  
extern U16 P_GIODIRCFG_L          ;//= (GPIO_BASE+0x0009);     //0x0009 ~ 0x000A 16 GPIO Input/Output Direction Select Register  
extern U16 P_GIOOUTDATA_L         ;//= (GPIO_BASE+0x000B);     //0x000B ~ 0x000C 16 GPIO Output Data Register  
extern U16 P_GIOINDATA_L          ;//= (GPIO_BASE+0x000D);     //0x000D ~ 0x000E  16 GPIO Input Data Register   
extern U16 P_PADPD_L              ;//= (GPIO_BASE+0x000F);     //0x000F ~ 0x0010 16 Pad Pull Down Register  
extern U16 P_PADPD_H              ;//= (GPIO_BASE+0x0010);     //0x000F ~ 0x0010 16 Pad Pull Down Register  
extern U16 P_WAKEUPEN_L           ;//= (GPIO_BASE+0x0011);     //0X0011 ~ 0x0012 16 Wake Up Enable Register  
extern U16 P_WAKEPOLCFG_L         ;//= (GPIO_BASE+0x0013);     //0X0013 ~ 0x0014 16 Wake Up Pad Polarity Register 
extern U16 P_MISC_CFG             ;//= (GPIO_BASE+0x0017);     //Miscellaneous Configuration 
extern U16 P_OSC_FT_CNT_L         ;//= (GPIO_BASE+0x0018);     //0X0018 ~ 0x0019  16  Oscillator Function Test Counter Register 
extern U16 P_OSC_FT_CNT_H         ;//= (GPIO_BASE+0x0019);

#endif

#endif   //end of 
