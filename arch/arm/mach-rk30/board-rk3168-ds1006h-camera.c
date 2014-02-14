#ifdef CONFIG_VIDEO_RK29
#include <plat/rk_camera.h>
/* Notes:

Simple camera device registration:

       new_camera_device(sensor_name,\       // sensor name, it is equal to CONFIG_SENSOR_X
                          face,\              // sensor face information, it can be back or front
                          pwdn_io,\           // power down gpio configuration, it is equal to CONFIG_SENSOR_POWERDN_PIN_XX
                          flash_attach,\      // sensor is attach flash or not
                          mir,\               // sensor image mirror and flip control information
                          i2c_chl,\           // i2c channel which the sensor attached in hardware, it is equal to CONFIG_SENSOR_IIC_ADAPTER_ID_X
                          cif_chl)  \         // cif channel which the sensor attached in hardware, it is equal to CONFIG_SENSOR_CIF_INDEX_X

Comprehensive camera device registration:

      new_camera_device_ex(sensor_name,\
                             face,\
                             ori,\            // sensor orientation, it is equal to CONFIG_SENSOR_ORIENTATION_X
                             pwr_io,\         // sensor power gpio configuration, it is equal to CONFIG_SENSOR_POWER_PIN_XX
                             pwr_active,\     // sensor power active level, is equal to CONFIG_SENSOR_RESETACTIVE_LEVEL_X
                             rst_io,\         // sensor reset gpio configuration, it is equal to CONFIG_SENSOR_RESET_PIN_XX
                             rst_active,\     // sensor reset active level, is equal to CONFIG_SENSOR_RESETACTIVE_LEVEL_X
                             pwdn_io,\
                             pwdn_active,\    // sensor power down active level, is equal to CONFIG_SENSOR_POWERDNACTIVE_LEVEL_X
                             flash_attach,\
                             res,\            // sensor resolution, this is real resolution or resoltuion after interpolate
                             mir,\
                             i2c_chl,\
                             i2c_spd,\        // i2c speed , 100000 = 100KHz
                             i2c_addr,\       // the i2c slave device address for sensor
                             cif_chl,\
                             mclk)\           // sensor input clock rate, 24 or 48
                          
*/

#if defined(CONFIG_TCHIP_MACH_TR1088)
#include "../mach-rk3188/tchip_camera_setup_tr1088.h"
#elif defined(CONFIG_TCHIP_MACH_TR7088) || defined(CONFIG_TCHIP_MACH_TR7078)
#include "../mach-rk3188/tchip_camera_setup_tr7088.h"
#elif defined(CONFIG_TCHIP_MACH_TRQ7_LJ)
#include "../mach-rk3188/tchip_camera_setup_trq7_lj.h"
#else
static struct rkcamera_platform_data new_camera[] = {
    new_camera_device_ex(RK29_CAM_SENSOR_OV5642,
                        back,
                        90,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        RK30_PIN3_PB4,
                        (CONS(RK29_CAM_SENSOR_OV5642,_PWRDN_ACTIVE)&0x10)|0x01,
                        0,
                        CONS(RK29_CAM_SENSOR_OV5642,_FULL_RESOLUTION),
                        0x00,
                        3,
                        100000,
                        CONS(RK29_CAM_SENSOR_OV5642,_I2C_ADDR),
                        0,
                        24),     
    new_camera_device_ex(RK29_CAM_SENSOR_SP2518,
                        front,
                        270, //CONFIG_SENSOR_ORIENTATION_X
                        INVALID_VALUE, //CONFIG_SENSOR_POWER_PIN_
                        INVALID_VALUE,//CONFIG_SENSOR_POWERACTIVE_LEVEL_
                        INVALID_VALUE, //CONFIG_SENSOR_RESET_PIN
                        INVALID_VALUE,//CONFIG_SENSOR_RESETACTIVE_LEVEL
                        RK30_PIN3_PB5,//CONFIG_SENSOR_POWERDN_PIN
                        1,//CONFIG_SENSOR_POWERDNACTIVE_LEVEL_X
                        0,//flash led 
                        sp2518_FULL_RESOLUTION,// resolution ,define use gc0329_FULL_RESOLUTION  in arch/arm/plat-rk/include/plat/rk_camera.h
                        0,//bit0=mirror[0/1] and bit1=flip[0/1]
                        3,// i2c changel  == CONFIG_SENSOR_IIC_ADAPTER_ID_X
                        100000,//// i2c speed , 100000 = 100KHz
                        0x60,//  define use gc0329_I2C_ADDR in arch/arm/plat-rk/include/plat/rk_camera.h ,too
                        0,//cif == CONFIG_SENSOR_CIF_INDEX_X
            24), //sensor input clock rate, 24 or 48 
/*    new_camera_device(RK29_CAM_SENSOR_SP2518,
                        front,
                        RK30_PIN3_PB4,
                        0,
                        0,
                        3, 0), */
    new_camera_device_end
};
#endif

#endif  //#ifdef CONFIG_VIDEO_RK29
/*---------------- Camera Sensor Configuration Macro End------------------------*/
#include "../../../drivers/media/video/rk30_camera.c"
/*---------------- Camera Sensor Macro Define End  ---------*/

#define PMEM_CAM_SIZE PMEM_CAM_NECESSARY
/*****************************************************************************************
 * camera  devices
 * author: ddl@rock-chips.com
 *****************************************************************************************/
#ifdef CONFIG_VIDEO_RK29
#define CONFIG_SENSOR_POWER_IOCTL_USR	   1 //define this refer to your board layout
#define CONFIG_SENSOR_RESET_IOCTL_USR	   0
#define CONFIG_SENSOR_POWERDOWN_IOCTL_USR	   0
#define CONFIG_SENSOR_FLASH_IOCTL_USR	   0

static void rk_cif_power(int on)
{
    struct regulator *ldo_18,*ldo_28;
	ldo_28 = regulator_get(NULL, "ldo7");	// vcc28_cif
	ldo_18 = regulator_get(NULL, "ldo1");	// vcc18_cif
	if (ldo_28 == NULL || IS_ERR(ldo_28) || ldo_18 == NULL || IS_ERR(ldo_18)){
        printk("get cif ldo failed!\n");
        if(on ==0 )
        {
        #if defined(CONFIG_TCHIP_MACH_TR7088)
            gpio_set_value(RK30_PIN3_PB4,GPIO_HIGH);
        #endif
        }
        else
        {
        #if defined(CONFIG_TCHIP_MACH_TR7088)
            gpio_set_value(RK30_PIN3_PB4,GPIO_LOW);
        #endif
        }
		return;
	    }
    if(on == 0){	
    	regulator_disable(ldo_28);
    	regulator_put(ldo_28);
    	regulator_disable(ldo_18);
    	regulator_put(ldo_18);
    	mdelay(500);
        }
    else{
    	regulator_set_voltage(ldo_28, 2800000, 2800000);
    	regulator_enable(ldo_28);
   // 	printk("%s set ldo7 vcc28_cif=%dmV end\n", __func__, regulator_get_voltage(ldo_28));
    	regulator_put(ldo_28);
#if defined(CONFIG_TCHIP_MACH_TRQ7_LJ)
    	regulator_set_voltage(ldo_18, 1200000, 1200000);
#else
    	regulator_set_voltage(ldo_18, 1800000, 1800000);
#endif
    //	regulator_set_suspend_voltage(ldo, 1800000);
    	regulator_enable(ldo_18);
    //	printk("%s set ldo1 vcc18_cif=%dmV end\n", __func__, regulator_get_voltage(ldo_18));
    	regulator_put(ldo_18);
        }
}

#if CONFIG_SENSOR_POWER_IOCTL_USR
static int sensor_power_usr_cb (struct rk29camera_gpio_res *res,int on)
{
	//#error "CONFIG_SENSOR_POWER_IOCTL_USR is 1, sensor_power_usr_cb function must be writed!!";
    rk_cif_power(on);
	return 0;
}
#endif

#if CONFIG_SENSOR_RESET_IOCTL_USR
static int sensor_reset_usr_cb (struct rk29camera_gpio_res *res,int on)
{
	#error "CONFIG_SENSOR_RESET_IOCTL_USR is 1, sensor_reset_usr_cb function must be writed!!";
}
#endif

#if CONFIG_SENSOR_POWERDOWN_IOCTL_USR
static int sensor_powerdown_usr_cb (struct rk29camera_gpio_res *res,int on)
{
	#error "CONFIG_SENSOR_POWERDOWN_IOCTL_USR is 1, sensor_powerdown_usr_cb function must be writed!!";
}
#endif

#if CONFIG_SENSOR_FLASH_IOCTL_USR

#define CONFIG_SENSOR_FALSH_EN_PIN_0		  RK30_PIN0_PD5   //high:enable
#define CONFIG_SENSOR_FALSH_EN_MUX_0		  GPIO0D5_SPI1TXD_NAME
#define CONFIG_SENSOR_FALSH_MODE_PIN_0		  RK30_PIN0_PD4   //high:FLASH, low:torch
#define CONFIG_SENSOR_FALSH_MODE_MUX_0		  GPIO0D4_SPI1RXD_NAME

static int sensor_init_flags = 0;
static int sensor_flash_usr_cb (struct rk29camera_gpio_res *res,int on)
{
        if(sensor_init_flags == 0){
                rk30_mux_api_set(CONFIG_SENSOR_FALSH_MODE_MUX_0, 0);
                int ret = gpio_request(CONFIG_SENSOR_FALSH_MODE_PIN_0, "camera_flash_mode");
                if (ret != 0) {
                    printk(">>>>gpio request camera_flash_mode faile !!\n");
                }
                
                gpio_direction_output(CONFIG_SENSOR_FALSH_MODE_PIN_0, 0);
                sensor_init_flags = 1 ;
        }
        switch (on) {
		case Flash_Off: {
			gpio_set_value(CONFIG_SENSOR_FALSH_EN_PIN_0, 0);
			gpio_set_value(CONFIG_SENSOR_FALSH_MODE_PIN_0, 0);
			break;
		}

		case Flash_On: {
			gpio_set_value(CONFIG_SENSOR_FALSH_EN_PIN_0, 1);
			gpio_set_value(CONFIG_SENSOR_FALSH_MODE_PIN_0, 1);
			break;
		}

		case Flash_Torch: {
			gpio_set_value(CONFIG_SENSOR_FALSH_EN_PIN_0, 1);
			gpio_set_value(CONFIG_SENSOR_FALSH_MODE_PIN_0, 0);
			break;
		}

		default: {
			printk("%s..Flash command(%d) is invalidate \n",__FUNCTION__, on);
			gpio_set_value(CONFIG_SENSOR_FALSH_EN_PIN_0, 0);
			break;
		}
	}
	return 0;
}
#endif

static struct rk29camera_platform_ioctl_cb	sensor_ioctl_cb = {
	#if CONFIG_SENSOR_POWER_IOCTL_USR
	.sensor_power_cb = sensor_power_usr_cb,
	#else
	.sensor_power_cb = NULL,
	#endif

	#if CONFIG_SENSOR_RESET_IOCTL_USR
	.sensor_reset_cb = sensor_reset_usr_cb,
	#else
	.sensor_reset_cb = NULL,
	#endif

	#if CONFIG_SENSOR_POWERDOWN_IOCTL_USR
	.sensor_powerdown_cb = sensor_powerdown_usr_cb,
	#else
	.sensor_powerdown_cb = NULL,
	#endif

	#if CONFIG_SENSOR_FLASH_IOCTL_USR
	.sensor_flash_cb = sensor_flash_usr_cb,
	#else
	.sensor_flash_cb = NULL,
	#endif
};

#include "../../../drivers/media/video/rk30_camera.c"

#endif /* CONFIG_VIDEO_RK29 */
