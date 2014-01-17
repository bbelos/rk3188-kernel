/*
 * include/linux/melfas_ts.h - platform data structure for MCS Series sensor
 *
 * Copyright (C) 2010 Melfas, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_MELFAS_TS_H
#define _LINUX_MELFAS_TS_H

#include <linux/earlysuspend.h>
#define MELFAS_TS_NAME "melfas-ts"

enum
{
	None = 0,
	TOUCH_SCREEN,
	TOUCH_KEY
};

struct muti_touch_info
{
	int pressure;
	int area;	
	int posX;
	int posY;
};

struct melfas_tsi_platform_data {
	uint32_t version;
	int max_x;
	int max_y;
	int max_pressure;
	int max_area;
	int gpio_scl;
	int gpio_sda;
	void (*power_enable)(int en);
	
	void (*init_platform_hw)(void);
};


struct melfas_ts_data
{
	uint16_t addr;
	struct i2c_client *client; 
	struct input_dev *input_dev;
	struct input_dev *ps_input_dev;
	struct melfas_tsi_platform_data *pdata;
	struct work_struct work;
	struct workqueue_struct *wq;
	struct delayed_work 	    delay_work;
	uint32_t flags;
	int (*power)(int onoff);
	struct early_suspend early_suspend;
	
};

#endif /* _LINUX_MELFAS_TS_H */
