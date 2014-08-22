/*
 * ÉÁ¹âµÆ¸¨ÖúÇý¶¯
 * 2014-07-25
 */
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/proc_fs.h>
#include <mach/gpio.h>
#include <mach/board.h> 
#include <linux/l3g4200d.h>
#include <linux/sensor-dev.h>

#define FLASH_LIGHT_SENSOR_THRESHOLD 500

extern struct sensor_private_data *g_sensor[SENSOR_NUM_TYPES];

int sensor_get_data(struct i2c_client *client)
{
	struct sensor_private_data *sensor =
	    (struct sensor_private_data *) i2c_get_clientdata(client);	
	char data[2] = {0};
	unsigned short value = 0;
   
    sensor->ops->active(client, 1, 0);
    mutex_lock(&sensor->sensor_mutex);	
    data[0] = 50;//CM3232_ADDR_DATA;
	sensor_rx_data(sensor->client, data, 2);
    value = (data[1] << 8) | data[0] ;
	
    mutex_unlock(&sensor->sensor_mutex);
    return value;
}

int sensor_light_status()
{
	int value = 0;
	struct sensor_private_data *sensor = NULL;
	struct i2c_client *client = NULL;
	sensor = g_sensor[SENSOR_TYPE_LIGHT];
	if (sensor == NULL)
		return 0;
	client = sensor->client;

     value= sensor_get_data(client);

    if(value < FLASH_LIGHT_SENSOR_THRESHOLD)
        return 1;
    else
        return 0;
}
EXPORT_SYMBOL(sensor_light_status);

static int __init auto_flash_init(void)
{
    printk("AUTO Flash Init:%d \n",sensor_light_status());
    return 0;
}

static void __exit auto_flash_exit(void)
{
	struct sensor_private_data *sensor = g_sensor[SENSOR_TYPE_LIGHT];
	struct i2c_client *client = sensor->client;
    sensor->ops->active(client, 0, 0);
}


late_initcall(auto_flash_init);
module_exit(auto_flash_exit);

MODULE_AUTHOR("Tchip Corporation:wbj@t-chip.com.cn");
MODULE_LICENSE("GPL");

