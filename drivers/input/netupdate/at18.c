/*
 * linux/drivers/input/encryption/at18.c
 *
 * Copyright (C) 2010-2011 T-Chip Inc
 *
 * AT18 I2C Entryption device driver, used in aml8726m SoC.
 *
 * Written by zhpy <zpy@t-chip.com.cn>
 * Date     : 2010.12.20
 * log      : 2010.12.20  test version.
 * log      : 2011.03.16  Porting to rk2918 platform <gouwa>
 */
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <mach/iomux.h>
#include <linux/reboot.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <mach/gpio.h>
#include "at18.h"

static int debug=0;
module_param(debug, int, S_IRUGO|S_IWUSR);

#define ATX8_TAG "ATX8"
#define dprintk(level, fmt, arg...) do {            \
    if (debug >= level)                     \
    printk(KERN_WARNING ATX8_TAG "[%s]: " fmt , __FUNCTION__, ## arg); } while (0)

#define ATX8_TR(format, ...) printk(KERN_ERR ATX8_TAG "[%s]: " format, __FUNCTION__, ## __VA_ARGS__)

#define ATX8_DG(format, ...) dprintk(1, format, ## __VA_ARGS__)


#define AT18_DRIVER_NAME    "netupdate"
#define AT18_SCL_RATE       (50 * 1000)
#define AT18_OPS_MAXNR      50
#define AT18_REG_WRITE      0x02
#define AT18_REG_READ       0x00

enum verify_t
{
    VERIFY_UNINIT,
    VERIFY_PASS,
    VERIFY_FAIL,
    VERIFY_ERROR,
};

struct at18_data
{
    struct i2c_client *client;
    struct atx8_platform_data *pdata;
};

extern void atx8_encrypt(uint8_t *buf, int type);
struct at18_data *atx8_info;
static struct i2c_driver at18_driver;
int at18_verify(void);
static int checkok = 0;

int at18_filp_exist = 0;

void set_at18_file_status(int file_status)
{
    at18_filp_exist = file_status;
}

int get_at18_file_status()
{
    return at18_filp_exist;
}

static ssize_t at18_show(struct device_driver *driver, char *buf)
{
    int ret;

    struct file* filp;
    //    mutex_lock(&(atx8_info->at18_read_mutex));
    if (checkok == 1)
    {
        ATX8_DG("AT18 Check OK file exit\n");
        ret = sprintf(buf, "successful\n"); // gModify.Ques snprintf
        return ret;
    }
    else
    {
        if (get_at18_file_status())
        {
            ATX8_DG("AT18 Check file exit\n");
            checkok = 1;
            ret = sprintf(buf, "successful\n"); // gModify.Ques snprintf
        }
        else
        {
            ATX8_DG("AT18 Check file fail\n");
            ret = sprintf(buf, "fail\n");
        }
    }

    return ret;
}

static ssize_t at18_store(struct device_driver *driver, const char *buf,
                          size_t count)
{
    ATX8_TR("Verify start\n");

    /* Verify AT18 Device present or not */
    at18_verify();

    return count;
}
static DRIVER_ATTR(netupdate, 0666, at18_show, at18_store);

/**
 * AT18 read/write method
 * @client: I2C client to be operated.
 * @buf:
 * read  buf[0] return the value read from at18
 * write buf[0] must put the address of the register to be write;
 *       buf[1-2] put the data to be writen
 *
 * Returns 0 if success, else negative errno.
 */

/******************************************************************************I2C Process***********************************************/
void at18_enable()
{
    struct atx8_platform_data *pdata = atx8_info->pdata;

    gpio_set_value(pdata->cs_gpio.io, pdata->cs_gpio.enable);
}
void at18_disable()
{
    struct atx8_platform_data *pdata = atx8_info->pdata;

    gpio_set_value(pdata->cs_gpio.io, !pdata->cs_gpio.enable);
}

int at18_set_io_mode()
{
    struct atx8_platform_data *pdata = atx8_info->pdata;

    pdata->iomux_set(&pdata->sda_gpio, pdata->sda_gpio.iomux.fgpio);

    if (gpio_request(pdata->sda_gpio.io, pdata->sda_gpio.name))
    {
        ATX8_TR("line %d: request at18_sda fail\n",__LINE__);
        return -1;
    }

    pdata->iomux_set(&pdata->scl_gpio,pdata->scl_gpio.iomux.fgpio);
    if (gpio_request(pdata->scl_gpio.io, pdata->scl_gpio.name))
    {
        ATX8_TR("line %d: request at18_scl fail\n",__LINE__);
        return -1;
    }
    return 0;
}

void at18_set_i2c0_mode()
{
    struct atx8_platform_data *pdata = atx8_info->pdata;

    gpio_free(pdata->sda_gpio.io);
    gpio_free(pdata->scl_gpio.io);

    pdata->iomux_set(&pdata->sda_gpio, pdata->sda_gpio.iomux.fi2c);
    pdata->iomux_set(&pdata->scl_gpio, pdata->scl_gpio.iomux.fi2c);
}


void DelayXus(unsigned int X)
{
    udelay(10*X);
}


void SDA_IN(void)
{
    struct atx8_platform_data *pdata = atx8_info->pdata;

    gpio_direction_input(pdata->sda_gpio.io);
}

unsigned int READ_SDA(void)
{
    struct atx8_platform_data *pdata = atx8_info->pdata;

    return gpio_get_value(pdata->sda_gpio.io);
}


void SDA_OUT(void)
{
    //struct atx8_platform_data *pdata = atx8_info->pdata;
    //gpio_direction_output(pdata->sda_gpio.io,gpio_get_value(pdata->sda_gpio.io));
}

void SCL_OUT(void)
{
    //struct atx8_platform_data *pdata = atx8_info->pdata;
    //gpio_direction_output(GPIO_SCL,gpio_get_value(GPIO_SCL));
}

void SCL_HIGH(void)
{
    struct atx8_platform_data *pdata = atx8_info->pdata;

    gpio_direction_output(pdata->scl_gpio.io, pdata->scl_gpio.enable);
}

void SCL_LOW(void)
{
    struct atx8_platform_data *pdata = atx8_info->pdata;

    gpio_direction_output(pdata->scl_gpio.io, !pdata->scl_gpio.enable);
}

void SDA_HIGH(void)
{
    struct atx8_platform_data *pdata = atx8_info->pdata;

    gpio_direction_output(pdata->sda_gpio.io, pdata->sda_gpio.enable);
}

void SDA_LOW(void)
{
    struct atx8_platform_data *pdata = atx8_info->pdata;

    gpio_direction_output(pdata->sda_gpio.io, !pdata->sda_gpio.enable);
}

void SDA_SCL_HIGH(void)
{
    struct atx8_platform_data *pdata = atx8_info->pdata;

    gpio_direction_output(pdata->scl_gpio.io, pdata->scl_gpio.enable);
    gpio_direction_output(pdata->sda_gpio.io, pdata->sda_gpio.enable);
}

void I2C_Start(void)
{
    SDA_OUT();
    SCL_OUT();
    SDA_SCL_HIGH();
    DelayXus(1);
    SDA_LOW();
    DelayXus(1);
    SCL_LOW();
    DelayXus(2);
}

void I2C_Stop(void)
{
    SDA_OUT();
    SCL_OUT();
    SDA_LOW();
    DelayXus(2);
    SCL_HIGH();
    DelayXus(2);
    SDA_HIGH();
    DelayXus(2);
}

void I2C_SendAck(unsigned int ack)
{
    SCL_OUT();
    SDA_OUT();
    if (ack)
        SDA_HIGH();
    else
        SDA_LOW();
    DelayXus(2);

    SCL_HIGH();
    DelayXus(2);
    SCL_LOW();
    DelayXus(2);
}

unsigned int I2C_SendByte(unsigned int byte)
{
    unsigned int i;
    unsigned int ack = 0;

    SCL_OUT();
    SDA_OUT();
    SCL_LOW();
    DelayXus(50);
    for (i = 0x80; i > 0; i >>= 1)
    {

        if ((i & byte) > 0)
            SDA_HIGH();
        else
            SDA_LOW();
        DelayXus(2);

        SCL_HIGH();
        DelayXus(2);
        SCL_LOW();

    }

    DelayXus(2);
    SDA_IN(); //daozhi 3US
    SCL_HIGH();

    i = 0;
    while (i++ < 200)
    {
        ack = READ_SDA();
        if (ack == 0)
            break;
        DelayXus(1);
    }

    DelayXus(1);
    SCL_LOW();
    DelayXus(2);

    if (0 != ack)
        return 0;
    else
        return 1;
}

uint8_t I2C_ReadByte(void)
{
    int i;
    uint8_t byte = 0;

    SCL_LOW();
    SDA_HIGH();
    SCL_OUT();
    SDA_OUT();
    DelayXus(1);
    SDA_IN();
    DelayXus(1);

    for (i = 0x80; i > 0; i >>= 1)
    {
        SCL_HIGH();
        DelayXus(2);
        if (READ_SDA() > 0)
        {
            byte |= i;

        }
        DelayXus(1);
        SCL_LOW();
        DelayXus(2);
    }
    return byte;
}

static int at18_write_reg(struct i2c_client *client, uint8_t *buf)
{
    unsigned int ack;
    int i;

    I2C_Start();
    for(i=0; i < 2; i++)
    {
        ack = I2C_SendByte(client->addr<<1);
        if (ack > 0)
            break;
    }
    if (ack == 0)
    {
        ATX8_DG("Send i2c address error\n");
        return -1;
    }

    for(i=0; i < 2; i++)
    {
        ack = I2C_SendByte(buf[0]);
        if (ack > 0)
            break;
    }
    if (ack == 0)
    {
        ATX8_DG("Send buf[0] error\n");
        return -1;
    }

    for(i=0; i < 2; i++)
    {
        ack = I2C_SendByte(buf[1]);
        if (ack > 0)
            break;
    }
    if (ack == 0)
    {
        ATX8_DG("Send buf[1] error\n");
        return -1;
    }

    for(i=0; i < 2; i++)
    {
        ack = I2C_SendByte(buf[2]);
        if (ack > 0)
            break;
    }
    if (ack == 0)
    {
        ATX8_DG("Send buf[2] error\n");
        return -1;
    }

    I2C_Stop();

    ATX8_DG("Write successfully\n");
    return 1;
}

static int at18_read_reg(struct i2c_client *client, uint8_t *buf)
{
    unsigned int ack;
    int i = 0;
    uint8_t *read=buf;

    I2C_Start();
    for(i=0; i < 4; i++)
    {
        ack = I2C_SendByte((client->addr<<1)| 0x01);
        if (ack > 0)
            break;
    }

    if (ack == 0)
    {
        ATX8_DG("Send read address error\n");
        return -1;
    }

    // SwIIC_SendRegister
    *read = I2C_ReadByte();
    I2C_SendAck(0);
    I2C_Stop();

    ATX8_DG("Read successfully, value=%#04x\n", *read);
    return 1;
}

#define Tchip_accetp_NOIC 1
int at18_verify(void)
{
    struct atx8_platform_data *pdata = atx8_info->pdata;
    struct i2c_client *client=atx8_info->client;
#if Tchip_accetp_NOIC
    int ret = VERIFY_PASS;
#else
    int ret = VERIFY_FAIL;
#endif
    uint8_t buf[4];
    int i, j;
    int type=0;
	
    if (at18_set_io_mode() != 0)
    {
        ret = VERIFY_ERROR;
        goto check_return;
    }

    for (type=0; type<4; type++)
    {
    
    for (i = 0; i < AT18_OPS_MAXNR; i++)
    {
        if (i % 20 == 0)
        {
            buf[0] = AT18_REG_WRITE;
            atx8_encrypt(buf, 8+type*10);
            ATX8_TR("at%d buf = [%#04x] [%#04x] [%#04x] -> [%#04x]\n",
                    pdata->type, buf[0], buf[1], buf[2], buf[3]);

            ATX8_DG("Reset ATX8\n");
            at18_disable();
            mdelay(5);
            at18_enable();
            mdelay(20);
        }
        if (at18_write_reg(client, buf) > 0)
        {
            for (j = 0; j < 20; j++)
            {
                if (at18_read_reg(client, buf) > 0)
                {
                    ATX8_DG("Should be %#x, actual value=%#x\n",
                            buf[3], buf[0]);
                    if (buf[3] == buf[0])
                    {
                        ret = VERIFY_PASS;
                        goto check_return;
                    }
					#if Tchip_accetp_NOIC // add by ruan for accetp no atx8 ic
					else{
						ret= VERIFY_FAIL;
					}
					#endif
                }
            }
        }
    }
    
    }

check_return:

    if (VERIFY_PASS == ret)
        ATX8_TR("Verify successfully type=AT %d \n",8+type*10);
    else
        ATX8_TR("Verify fail \n");
    at18_disable();
    at18_set_i2c0_mode();

    return ret;
}
EXPORT_SYMBOL(at18_verify);

static int __devinit
at18_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct atx8_platform_data *pdata = client->dev.platform_data;

    atx8_info = kzalloc(sizeof(struct at18_data), GFP_KERNEL);

    pdata->iomux_set(&pdata->cs_gpio,pdata->cs_gpio.iomux.fgpio);

    if (gpio_request(pdata->cs_gpio.io, pdata->cs_gpio.name) != 0)
    {
        gpio_free(pdata->cs_gpio.io);
        ATX8_TR("AT18 failed to request gpio[%s].\n", pdata->cs_gpio.name);
        return -EIO;
    }
    gpio_direction_output(pdata->cs_gpio.io, !pdata->cs_gpio.enable);

    atx8_info->client = client;
    atx8_info->pdata = pdata;

    ATX8_TR("############3 atx8 type= mixing \n");
    i2c_set_clientdata(client, atx8_info);
	
#if 1 //add by ruan
			if( at18_verify() == VERIFY_PASS )
			{
				set_at18_file_status(1);
            	checkok = 1;
            	ATX8_TR("############# AT18 Check pass \n");
			}else{
            	ATX8_TR("############# AT18 Check fail\n");
			}
#endif
    /* Create sysfs file for Android */
    return driver_create_file(&at18_driver.driver, &driver_attr_netupdate);
}

static int __devexit
at18_remove(struct i2c_client *client)
{
    struct atx8_platform_data *pdata = atx8_info->pdata;
    ATX8_TR("Enter\n");

    gpio_free(pdata->cs_gpio.io);
    driver_remove_file(&at18_driver.driver, &driver_attr_netupdate);
    kfree(atx8_info);
    atx8_info=NULL;

    return 0;
}

static struct i2c_device_id at18_idtable[] =
{
  { AT18_DRIVER_NAME, 0 },
  {}
};
MODULE_DEVICE_TABLE(i2c, at18_idtable);

static struct i2c_driver at18_driver = {
    .driver =
        {
          .owner = THIS_MODULE,
          .name = AT18_DRIVER_NAME,
        },
    .probe = at18_probe,
    .remove = __devexit_p(at18_remove),
    .id_table = at18_idtable,
};

static int __init
at18_i2c_init(void)
{
    return i2c_add_driver(&at18_driver);
}

static void __exit
at18_i2c_exit(void)
{
    i2c_del_driver(&at18_driver);
}

module_init(at18_i2c_init);
module_exit(at18_i2c_exit);

EXPORT_SYMBOL(get_at18_file_status);
EXPORT_SYMBOL(set_at18_file_status);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhpy <zpy@t-chip.com.cn>");
MODULE_DESCRIPTION("T-Chip ATX8 driver");
