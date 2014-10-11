#ifndef __TCHIP_CAMERA_SETUP__
#define __TCHIP_CAMERA_SETUP__

#ifdef CONFIG_TCHIP_MACH_DOUBLE_CAMEAR 
#define CONFIG_TCHIP_MACH_SINGLE_FRONT_CAMERA
#define CONFIG_TCHIP_MACH_SINGLE_BACK_CAMERA
#endif

static struct rkcamera_platform_data new_camera[] = {
    new_camera_device_ex(RK29_CAM_SENSOR_SP2518,
                        back,
                        0, //CONFIG_SENSOR_ORIENTATION_X
                        INVALID_VALUE, //CONFIG_SENSOR_POWER_PIN_
                        INVALID_VALUE,//CONFIG_SENSOR_POWERACTIVE_LEVEL_
                        INVALID_VALUE, //CONFIG_SENSOR_RESET_PIN
                        INVALID_VALUE,//CONFIG_SENSOR_RESETACTIVE_LEVEL
                        RK30_PIN3_PB4,//CONFIG_SENSOR_POWERDN_PIN
                        1,//CONFIG_SENSOR_POWERDNACTIVE_LEVEL_X
                        0,//flash led 
                        sp2518_FULL_RESOLUTION,// resolution ,define use gc0329_FULL_RESOLUTION  in arch/arm/plat-rk/include/plat/rk_camera.h
                        0,//bit0=mirror[0/1] and bit1=flip[0/1]
                        3,// i2c changel  == CONFIG_SENSOR_IIC_ADAPTER_ID_X
                        100000,//// i2c speed , 100000 = 100KHz
                        0x60,//  define use gc0329_I2C_ADDR in arch/arm/plat-rk/include/plat/rk_camera.h ,too
                        0,//cif == CONFIG_SENSOR_CIF_INDEX_X
            			24),
    new_camera_device_ex(RK29_CAM_SENSOR_SP2518,
                        front,
                        0, //CONFIG_SENSOR_ORIENTATION_X
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
            			24),

    new_camera_device_end
};
#endif


