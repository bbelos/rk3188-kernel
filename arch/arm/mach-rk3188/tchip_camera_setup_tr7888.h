#ifndef __TCHIP_CAMERA_SETUP__
#define __TCHIP_CAMERA_SETUP__


static struct rkcamera_platform_data new_camera[] = {         
	new_camera_device_ex(RK29_CAM_SENSOR_GC2035,
						back,
						180,
						INVALID_VALUE,
						INVALID_VALUE,
						INVALID_VALUE,
						INVALID_VALUE,
						RK30_PIN3_PB4,
						(CONS(RK29_CAM_SENSOR_GC2035,_PWRDN_ACTIVE)&0x10)|0x01,
						0,
						CONS(RK29_CAM_SENSOR_GC2035,_FULL_RESOLUTION),
						0x00,
						3,
						100000,
						CONS(RK29_CAM_SENSOR_GC2035,_I2C_ADDR),
						0,
						24),
						
	new_camera_device_ex(RK29_CAM_SENSOR_GC0329,
                        front,
                        180,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        RK30_PIN3_PB5,
                        (CONS(RK29_CAM_SENSOR_GC0329,_PWRDN_ACTIVE)&0x10)|0x01,
                        0,
                        CONS(RK29_CAM_SENSOR_GC0329,_FULL_RESOLUTION),
                        0x00,
                        3,
                        100000,
                        CONS(RK29_CAM_SENSOR_GC0329,_I2C_ADDR),
                        0,
                        24),
    new_camera_device_end
};
#endif


