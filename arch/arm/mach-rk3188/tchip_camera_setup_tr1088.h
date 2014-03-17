#ifndef __TCHIP_CAMERA_SETUP__
#define __TCHIP_CAMERA_SETUP__


static struct rkcamera_platform_data new_camera[] = {
    new_camera_device_ex(RK29_CAM_SENSOR_GC2035,
                        back,
                        0, //CONFIG_SENSOR_ORIENTATION_X
                        INVALID_VALUE, //CONFIG_SENSOR_POWER_PIN_
                        INVALID_VALUE,//CONFIG_SENSOR_POWERACTIVE_LEVEL_
                        INVALID_VALUE, //CONFIG_SENSOR_RESET_PIN
                        INVALID_VALUE,//CONFIG_SENSOR_RESETACTIVE_LEVEL
                        RK30_PIN3_PB5,//CONFIG_SENSOR_POWERDN_PIN
                        1,//CONFIG_SENSOR_POWERDNACTIVE_LEVEL_X
                        0,//flash led 
                        CONS(RK29_CAM_SENSOR_GC2035,_FULL_RESOLUTION),// resolution ,define use gc0329_FULL_RESOLUTION  in arch/arm/plat-rk/include/plat/rk_camera.h
                        0,//bit0=mirror[0/1] and bit1=flip[0/1]
                        3,// i2c changel  == CONFIG_SENSOR_IIC_ADAPTER_ID_X
                        100000,//// i2c speed , 100000 = 100KHz
                        CONS(RK29_CAM_SENSOR_GC2035,_I2C_ADDR),//  define use gc0329_I2C_ADDR in arch/arm/plat-rk/include/plat/rk_camera.h ,too
                        0,//cif == CONFIG_SENSOR_CIF_INDEX_X
            24), //sensor input clock rate, 24 or 48 
    new_camera_device_ex(RK29_CAM_SENSOR_GC0309,
                        front,
                        180,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        RK30_PIN3_PB4,
                        (CONS(RK29_CAM_SENSOR_GC0309,_PWRDN_ACTIVE)&0x10)|0x01,
                        0,
                        CONS(RK29_CAM_SENSOR_GC0309,_FULL_RESOLUTION),
                        0x00,
                        3,
                        100000,
                        CONS(RK29_CAM_SENSOR_GC0309,_I2C_ADDR),
                        0,
                        24), 
	new_camera_device_ex(RK29_CAM_SENSOR_GC0308,
                        front,
                        0,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        RK30_PIN3_PB4,
                        (CONS(RK29_CAM_SENSOR_GC0308,_PWRDN_ACTIVE)&0x10)|0x01,
                        0,
                        CONS(RK29_CAM_SENSOR_GC0308,_FULL_RESOLUTION),
                        0x00,
                        3,
                        100000,
                        CONS(RK29_CAM_SENSOR_GC0308,_I2C_ADDR),
                        0,
                        24),    
    new_camera_device_end
};
#endif


