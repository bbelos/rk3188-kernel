/*
 *  MCube mc3XXX acceleration sensor driver
 *
 *  Copyright (C) 2013 MCube Inc.,
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * *****************************************************************************/

#ifndef MC3XXX_H
#define MC3XXX_H

#include <linux/ioctl.h>



#define MCIO				0xA1
#define RBUFF_SIZE		12
/* IOCTLs for MC3XXX library */
#define MC_IOCTL_INIT                  _IO(MCIO, 0x01)
#define MC_IOCTL_RESET      	  _IO(MCIO, 0x04)
#define MC_IOCTL_CLOSE		  _IO(MCIO, 0x02)
#define MC_IOCTL_START		  _IO(MCIO, 0x03)
#define MC_IOCTL_GETDATA          _IOR(MCIO, 0x08, char[RBUFF_SIZE+1])

/* IOCTLs for APPs */
#define MC_IOCTL_APP_SET_RATE		_IOW(MCIO, 0x10, char)


/*sniffr mode rate Samples/Second (3~4)*/
#define MC3XXX_SNIFFR_RATE_32		0
#define MC3XXX_SNIFFR_RATE_16		1
#define MC3XXX_SNIFFR_RATE_8		2
#define MC3XXX_SNIFFR_RATE_1		3
#define MC3XXX_SNIFFR_RATE_SHIFT	3


struct mc3XXX_axis {
	int x;
	int y;
	int z;
};


//add accel calibrate IO
typedef struct {
	unsigned short	x;		/**< X axis */
	unsigned short	y;		/**< Y axis */
	unsigned short	z;		/**< Z axis */
} GSENSOR_VECTOR3D;

typedef struct{
	int x;
	int y;
	int z;
}SENSOR_DATA;


#define GSENSOR						   	0x85
#define GSENSOR_IOCTL_INIT                  _IO(GSENSOR,  0x01)
#define GSENSOR_IOCTL_READ_CHIPINFO         _IOR(GSENSOR, 0x02, int)
#define GSENSOR_IOCTL_READ_SENSORDATA       _IOR(GSENSOR, 0x03, int)
#define GSENSOR_IOCTL_READ_OFFSET			_IOR(GSENSOR, 0x04, GSENSOR_VECTOR3D)
#define GSENSOR_IOCTL_READ_GAIN				_IOR(GSENSOR, 0x05, GSENSOR_VECTOR3D)
#define GSENSOR_IOCTL_READ_RAW_DATA			_IOR(GSENSOR, 0x06, int)
#define GSENSOR_IOCTL_SET_CALI				_IOW(GSENSOR, 0x06, SENSOR_DATA)
#define GSENSOR_IOCTL_GET_CALI				_IOW(GSENSOR, 0x07, SENSOR_DATA)
#define GSENSOR_IOCTL_CLR_CALI				_IO(GSENSOR, 0x08)
#define GSENSOR_MCUBE_IOCTL_READ_RBM_DATA		_IOR(GSENSOR, 0x09, SENSOR_DATA)
#define GSENSOR_MCUBE_IOCTL_SET_RBM_MODE		_IO(GSENSOR, 0x0a)
#define GSENSOR_MCUBE_IOCTL_CLEAR_RBM_MODE		_IO(GSENSOR, 0x0b)
#define GSENSOR_MCUBE_IOCTL_SET_CALI			_IOW(GSENSOR, 0x0c, SENSOR_DATA)
#define GSENSOR_MCUBE_IOCTL_REGISTER_MAP		_IO(GSENSOR, 0x0d)
#define GSENSOR_IOCTL_SET_CALI_MODE   			_IOW(GSENSOR, 0x0e,int)


extern long mc3XXX_ioctl( struct file *file, unsigned int cmd,unsigned long arg,struct i2c_client *client);
#endif

