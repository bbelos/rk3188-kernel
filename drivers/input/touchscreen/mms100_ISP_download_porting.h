
//--------------------------------------------------------
//
//
//	Melfas MMS100 Series Download base v1.0
//
//
//--------------------------------------------------------

#ifndef __MELFAS_DOWNLOAD_PORTING_H_INCLUDED__
#define __MELFAS_DOWNLOAD_PORTING_H_INCLUDED__

#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <linux/jiffies.h>
#include <mach/iomux.h>

//============================================================
//
//	Type define
//
//============================================================

typedef char				INT8;
typedef unsigned char		UINT8;
typedef short				INT16;
typedef unsigned short		UINT16;
typedef int					INT32;
typedef unsigned int		UINT32;
typedef unsigned char		BOOLEAN;


//============================================================
//
//	Porting Download Options
//
//============================================================
#define MCSDL_USE_CE_CONTROL 1
#define MCSDL_USE_VDD_CONTROL 0
#define MCSDL_USE_RESETB_CONTROL 1

#define MELFAS_ENABLE_DBG_PRINT 0
#define MELFAS_ENABLE_DBG_PROGRESS_PRINT 0

//tchip
//GPIO3A_I2C2_SCL
//GPIO3A_GPIO3A1
//GPIO3A1_I2C2SCL_NAME

//GPIO3A_I2C2_SDA
//GPIO3A_GPIO3A0
//GPIO3A0_I2C2SDA_NAME
#define GPIO_TSP_SDA_28V RK30_PIN1_PD4
#define GPIO_TSP_SCL_28V RK30_PIN1_PD5

#define GPIO_TSP_LDO_ON INVALID_GPIO
#define GPIO_TOUCH_INT RK30_PIN1_PB2

#define TOUCH_EN  RK30_PIN0_PD4

////////////////////////////////////////////////////
#define TOUCH_ENABLE_PIN        RK30_PIN1_PB4
//#define TOUCH_INT_PIN           RK30_PIN1_PB2
#define TOUCH_INT_PIN           RK30_PIN0_PD4
#define TOUCH_RESET_PIN         RK30_PIN0_PD4
//----------------
// VDD
//----------------
#if MCSDL_USE_VDD_CONTROL
//#define mcsdl_vdd_on()                  
//#define mcsdl_vdd_off()                 

#define MCSDL_VDD_SET_HIGH()            gpio_set_value(GPIO_TSP_LDO_ON, 1)
#define MCSDL_VDD_SET_LOW()             gpio_set_value(GPIO_TSP_LDO_ON, 0)
#else
#define MCSDL_VDD_SET_HIGH()            // Nothing
#define MCSDL_VDD_SET_LOW()             // Nothing
#endif

//----------------
// CE
//----------------
#if MCSDL_USE_CE_CONTROL
#define MCSDL_CE_SET_HIGH()   	       gpio_set_value(TOUCH_EN, 1)
#define MCSDL_CE_SET_LOW()      	   gpio_set_value(TOUCH_EN, 0)
#define MCSDL_CE_SET_OUTPUT()   	   gpio_direction_output(TOUCH_EN, 0)
#define MCSDL_CE_IOMUX()               iomux_set(GPIO1_B4)
#else
#define MCSDL_CE_SET_HIGH()				// Nothing
#define MCSDL_CE_SET_LOW()				// Nothing
#define MCSDL_CE_SET_OUTPUT()			// Nothing
#endif


//----------------
// RESETB
//----------------
#if MCSDL_USE_RESETB_CONTROL
#define MCSDL_RESETB_SET_HIGH()         gpio_set_value(GPIO_TOUCH_INT, 1)
#define MCSDL_RESETB_SET_LOW()          gpio_set_value(GPIO_TOUCH_INT, 0)
#define MCSDL_RESETB_SET_OUTPUT()       gpio_direction_output(GPIO_TOUCH_INT, 0);\
										
#define MCSDL_RESETB_SET_INPUT()        gpio_direction_input(GPIO_TOUCH_INT)
#define MCSDL_RESETB_SET_ALT()  	    gpio_direction_input(GPIO_TOUCH_INT)
#define MCSDL_RESETB_IOMUX()               iomux_set(GPIO0_D4)
#else
#define MCSDL_RESETB_SET_HIGH()
#define MCSDL_RESETB_SET_LOW()
#define MCSDL_RESETB_SET_OUTPUT()
#define MCSDL_RESETB_SET_INPUT()
#endif


//------------------
// I2C SCL & SDA
//------------------
#define MCSDL_GPIO_SCL_SET_HIGH()		gpio_set_value(GPIO_TSP_SCL_28V, 1)
#define MCSDL_GPIO_SCL_SET_LOW()		gpio_set_value(GPIO_TSP_SCL_28V, 0)

#define MCSDL_GPIO_SDA_SET_HIGH()		gpio_set_value(GPIO_TSP_SDA_28V, 1)
#define MCSDL_GPIO_SDA_SET_LOW()		gpio_set_value(GPIO_TSP_SDA_28V, 0)

#define MCSDL_GPIO_SCL_SET_OUTPUT()	   iomux_set(GPIO1_D5);\
										gpio_direction_output(GPIO_TSP_SCL_28V, 0)
#define MCSDL_GPIO_SCL_SET_INPUT()	    iomux_set(GPIO1_D5);\
										gpio_direction_input(GPIO_TSP_SCL_28V)
#define MCSDL_GPIO_SCL_SET_ALT()		 iomux_set(I2C2_SCL);

#define MCSDL_GPIO_SDA_SET_OUTPUT()	     iomux_set(GPIO1_D4);\
										gpio_direction_output(GPIO_TSP_SDA_28V, 0)
#define MCSDL_GPIO_SDA_SET_INPUT()	      iomux_set(GPIO1_D4);\
										gpio_direction_input(GPIO_TSP_SDA_28V)
#define MCSDL_GPIO_SDA_SET_ALT()		   iomux_set(I2C2_SDA);

#define MCSDL_GPIO_SDA_IS_HIGH()		((gpio_get_value(GPIO_TSP_SDA_28V) > 0) ? 1 : 0)

#endif

