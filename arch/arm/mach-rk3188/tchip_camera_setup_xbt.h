#ifndef __TCHIP_CAMERA_SETUP__
#define __TCHIP_CAMERA_SETUP__

#define new_camera_device_xbt(sensor_name,\
                             face,\
                             ori,\
                             pwr_io,\
                             pwr_active,\
                             rst_io,\
                             rst_active,\
                             pwdn_io,\
                             pwdn_active,\
                             flash_attach,\
                             flash_io,\
                             af_io,\
                             res,\
                             mir,\
                             i2c_chl,\
                             i2c_spd,\
                             i2c_addr,\
                             cif_chl,\
                             mclk)\
        {\
            .dev = {\
                .i2c_cam_info = {\
                    I2C_BOARD_INFO(STR(sensor_name), i2c_addr>>1),\
                },\
                .link_info = {\
                	.bus_id= RK29_CAM_PLATFORM_DEV_ID+cif_chl,\
                	.i2c_adapter_id = i2c_chl,\
                	.module_name	= STR(sensor_name),\
                },\
                .device_info = {\
                	.name = "soc-camera-pdrv",\
                	.dev	= {\
                		.init_name = STR(CONS(_CONS(sensor_name,_),face)),\
                	},\
                },\
            },\
            .io = {\
                .gpio_power = pwr_io,\
                .gpio_reset = rst_io,\
                .gpio_powerdown = pwdn_io,\
                .gpio_af = af_io,\
                .gpio_flash = flash_io,\
                .gpio_flag = ((pwr_active&0x01)<<RK29_CAM_POWERACTIVE_BITPOS)|((rst_active&0x01)<<RK29_CAM_RESETACTIVE_BITPOS)|((pwdn_active&0x01)<<RK29_CAM_POWERDNACTIVE_BITPOS)|RK29_CAM_FLASHACTIVE_H|RK29_CAM_AFACTIVE_H,\
            },\
            .orientation = ori,\
            .resolution = res,\
            .mirror = mir,\
            .i2c_rate = i2c_spd,\
            .flash = flash_attach,\
            .pwdn_info = ((pwdn_active&0x10)|0x01),\
            .powerup_sequence = CONS(sensor_name,_PWRSEQ),\
            .mclk_rate = mclk,\
        }

static struct rkcamera_platform_data new_camera[] = {
    new_camera_device_xbt(RK29_CAM_SENSOR_OV5640,
                        back,
                        0,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        RK30_PIN3_PB4,
                        (CONS(RK29_CAM_SENSOR_OV5640,_PWRDN_ACTIVE)&0x10)|0x01,
                        0,
                        RK30_PIN0_PC1,
                        RK30_PIN0_PC7,
                        CONS(RK29_CAM_SENSOR_OV5640,_FULL_RESOLUTION),
                        0x00,
                        3,
                        100000,
                        CONS(RK29_CAM_SENSOR_OV5640,_I2C_ADDR),
                        0,
                        24),
    new_camera_device_xbt(RK29_CAM_SENSOR_OV5647,
                        back,
                        0,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        RK30_PIN3_PB4,
                        (CONS(RK29_CAM_SENSOR_OV5647,_PWRDN_ACTIVE)&0x10)|0x01,
                        0,
                        RK30_PIN0_PC1,
                        RK30_PIN0_PC7,
                        CONS(RK29_CAM_SENSOR_OV5647,_FULL_RESOLUTION),
                        0x00,
                        3,
                        100000,
                        0x6c,//CONS(RK29_CAM_SENSOR_OV5647,_I2C_ADDR),
                        0,
                        24),
    new_camera_device_ex(RK29_CAM_SENSOR_SP2518,
                        front,
                        0,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        RK30_PIN3_PB5,
                        (CONS(RK29_CAM_SENSOR_SP2518,_PWRDN_ACTIVE)&0x10)|0x01,
                        0,
                        CONS(RK29_CAM_SENSOR_SP2518,_FULL_RESOLUTION),
                        0x00,
                        3,
                        100000,
                        CONS(RK29_CAM_SENSOR_SP2518,_I2C_ADDR),
                        0,
                        24),              
    new_camera_device_ex(RK29_CAM_SENSOR_GC2145,
                        front,
                        0,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        RK30_PIN3_PB5,
                        (CONS(RK29_CAM_SENSOR_GC2145,_PWRDN_ACTIVE)&0x10)|0x01,
                        0,
                        CONS(RK29_CAM_SENSOR_GC2145,_FULL_RESOLUTION),
                        0x00,
                        3,
                        100000,
                        CONS(RK29_CAM_SENSOR_GC2145,_I2C_ADDR),
                        0,
                        24),              
    new_camera_device_ex(RK29_CAM_SENSOR_GC2155,
                        front,
                        0,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        INVALID_VALUE,
                        RK30_PIN3_PB5,
                        (CONS(RK29_CAM_SENSOR_GC2155,_PWRDN_ACTIVE)&0x10)|0x01,
                        0,
                        CONS(RK29_CAM_SENSOR_GC2155,_FULL_RESOLUTION),
                        0x00,
                        3,
                        100000,
                        CONS(RK29_CAM_SENSOR_GC2155,_I2C_ADDR),
                        0,
                        24),              
    new_camera_device_end
};
#endif


