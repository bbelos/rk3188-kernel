/*
 * NewportMedia WiFi chipset driver test tool
 * Copyright (c) 2013 NewportMedia Inc.
 *
 * Author: System Software Division (sswd@nmisemi.com)
 * This program is free Software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include "linux_wlan_cdev.h"

#ifdef NMI_SDIO
#define SDIO_BUS
#else
#define SPI_BUS
#endif

extern int custom_spi_write_reg(uint32_t addr, uint32_t data);
extern  int custom_spi_read_reg(uint32_t addr, uint32_t *data);
extern int custom_spi_write(uint32_t addr, uint8_t *buf, uint32_t size);

#define PRINT_DEBUG		1

#define PERR(fmt, args...) 		printk( KERN_ERR "nmc1000_cdev: " fmt, ## args)
#define PINFO(fmt, args...) 	printk("nmc1000_cdev: " fmt, ## args)
#if ( PRINT_DEBUG == 1)
#define PDBG(fmt, args...)		printk("nmc1000_cdev: " fmt, ## args)
#else
#define PDBG(fmt, args...)
#endif

typedef struct {
	
	unsigned int read_write:1;
	unsigned int function:3;
	unsigned int block_mode:1;
	unsigned int increment:1;
	unsigned int address:17;
	unsigned int count:9;
	unsigned int block_size;
}sdio_cmd53_ioctl_t;

typedef struct {
	unsigned char *buf;
	unsigned long adr;
	unsigned long val;
}spi_rw_param;

/* SDIO */
static sdio_cmd53_t gcmd53;
static unsigned char *cmd53_buf;

/* SPI */
static unsigned char *spi_rwbuf;
static spi_rw_param gspi;

#define NMC_MAJOR		0
#define NMC_MINOR		0

#define NMC_DEV_NAME		"nmc1000"

int nmc_cdev_major = NMC_MAJOR;
int nmc_cdev_minor = NMC_MINOR;
static struct cdev nmc_device_cdev;
static struct class *nmc_device_class;

#define MAX_ARG_BUF_LEN 128*1024 /* bytes */
#define MAX_SPI_ARG_BUF_LEN	8*1024

void  deinit_cdev(void);

int nmi_sdio_cmd52(sdio_cmd52_t *cmd)
{
	return 0;
}

int nmi_sdio_cmd53(sdio_cmd53_t *cmd)
{
	return 0;
}

int nmi_spi_read(spi_rw_param *spi)
{
	return 0;
}

int nmi_spi_write(spi_rw_param *spi)
{
	return 0;
}

int nmc_dev_open(struct inode *inode, struct file *filp)
{	
	PINFO("%s\n", __FUNCTION__);
#ifdef SDIO_BUS
	PDBG("%s BUS_SDIO\n", __FUNCTION__);
	cmd53_buf = kmalloc(MAX_ARG_BUF_LEN, GFP_KERNEL);	
	if(!cmd53_buf) {		
		PERR("no memory for cmd53_buf\n");		
		return -ENOMEM;	
	}		
#endif

#if 0//def SPI_BUS
	PDBG("%s BUS_SPI\n", __FUNCTION__);
	spi_rwbuf = kmalloc(MAX_SPI_ARG_BUF_LEN, GFP_KERNEL);
	if(!spi_rwbuf) {		
		PERR("no memory for spi rw buf\n");		
		return -ENOMEM;	
	}	
#endif	
	custom_chip_wakeup();
	g_mptool_running = 1;

	return 0;
}

int nmc_dev_release(struct inode *inode, struct file *filp)
{	
	PINFO("%s\n", __FUNCTION__);	
	g_mptool_running = 0;

#ifdef SDIO_BUS
	/* SDIO */
	kfree(cmd53_buf);	
#endif

#if	0//def SPI_BUS
	kfree(spi_rwbuf);
#endif
	return 0;
}

long nmc_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{	
	int result = 0, retval = 0;	
	int gcmd53_buf_size;
	spi_ioc_t spi_data;
	
	PINFO("%s\n", __FUNCTION__);	

	if(_IOC_TYPE(cmd) != NMC_IOC_MAGIC) 
		return -EINVAL;	
	if(_IOC_NR(cmd) > NMC_IOC_MAXNR) 
		return -EINVAL;	
	if(_IOC_DIR(cmd) & _IOC_READ) 
	{		
		if(!access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd))) 
		{		
			PERR("%s: cmd: !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd))\n", __FUNCTION__);			
			return -EINVAL;		
		}	
	} else if(_IOC_DIR(cmd) & _IOC_WRITE) {		
		if(!access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd))) 
		{			
			PERR("%s: cmd: !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd))\n", __FUNCTION__);			
			return -EINVAL;		
		}	
	}	

	switch(cmd)	
	{		
		case NMC_IOPWRON:			
			/* device power on and reset high */			
			PINFO("%s: ************ POWER ON ****************\n", __FUNCTION__);			
			break;		
		case NMC_IOPWROFF:			
			/* device power off and reset low */			
			PINFO("%s: ************ POWER OFF ****************\n", __FUNCTION__);			
			break;		
		/* SDIO */
		case NMC_IOWRCMD52:	/* sdio cmd52 */			
			{				
				sdio_cmd52_t cmd52;				
				PDBG("%s: NMC_IOWRCMD52\n", __FUNCTION__);				
				if(copy_from_user(&cmd52, (sdio_cmd52_t*)arg, sizeof(sdio_cmd52_t)) != 0)
				{					
					PERR("%s: fail copy_from_user(&cmd52, (sdio_cmd52_t*)arg, sizeof(sdio_cmd52_t)) != 0\n", __FUNCTION__);				
					return -EINVAL;				
				}	
				nmi_sdio_cmd52(&cmd52);				
				copy_to_user((void*)arg, &cmd52, sizeof(sdio_cmd52_t));			
			} break;		
		case NMC_IOWCMD53:	/* sdio cmd53 */			
			{				
				sdio_cmd53_ioctl_t cmd53_ioctl;				
				PDBG("%s: NMC_IOWCMD53\n", __FUNCTION__);				
				if(copy_from_user(&cmd53_ioctl, (sdio_cmd53_ioctl_t*)arg, sizeof(sdio_cmd53_ioctl_t)) != 0) 
				{					
					PERR("%s: fail copy_from_user(&cmd53, (sdio_cmd53_t*)arg, sizeof(sdio_cmd53_t)) != 0\n", __FUNCTION__);					
					return -EINVAL;				
				}				
				gcmd53.read_write = cmd53_ioctl.read_write;				
				gcmd53.address = cmd53_ioctl.address;				
				gcmd53.block_mode = cmd53_ioctl.block_mode;				
				gcmd53.block_size = cmd53_ioctl.block_size;				
				gcmd53.count = cmd53_ioctl.count;				
				gcmd53.function = cmd53_ioctl.function;				
				gcmd53.increment = cmd53_ioctl.increment;								
				gcmd53_buf_size = gcmd53.count ;				
				gcmd53_buf_size *= gcmd53.block_mode?gcmd53.block_size:1;								

				PDBG("gcmd53.read_write = %d\n", gcmd53.read_write);				
				PDBG("gcmd53.function = %d\n", gcmd53.function);				
				PDBG("gcmd53.block_mode = %d\n", gcmd53.block_mode);				
				PDBG("gcmd53.increment = %d\n", gcmd53.increment);				
				PDBG("gcmd53.address = %d\n", gcmd53.address);				
				PDBG("gcmd53.count = %d\n", gcmd53.count);				
				PDBG("gcmd53.block_size= %d\n", gcmd53.block_size);				
				PDBG("gcmd53_buf_size= %d\n", gcmd53_buf_size);			
			} break;		
		case NMC_IOWRCMD53BUF:			
			PDBG("%s: NMC_IOWRCMD53BUF\n", __FUNCTION__);			
			if(copy_from_user(cmd53_buf, (unsigned char*)arg, gcmd53_buf_size) != 0) 
			{				
				PERR("%s: fail copy_from_user(buf, (unsigned char*)arg, gcmd53_buf_size) != 0\n", __FUNCTION__);				
				return -EINVAL;			
			}			
			gcmd53.buffer = cmd53_buf;			
			nmi_sdio_cmd53(&gcmd53);			
			copy_to_user((void*)arg, cmd53_buf, gcmd53_buf_size);			
			break;		
		/* SPI */
		case NMC_IOWSPIWREG:	/* spi write reg */
			PDBG("%s: NMC_IOWSPIWREG\n", __FUNCTION__);	
			if(copy_from_user(&spi_data, (spi_ioc_t*)arg, sizeof(spi_ioc_t)) != 0) 
			{					
				PERR("%s: fail copy_from_user(&spi_data, (spi_ioc_t*)arg, sizeof(spi_ioc_t)) != 0\n", __FUNCTION__);					
				return -EINVAL;				
			}		

			PDBG("spi adr = %x, val = %x\n", spi_data.adr, spi_data.val);		
			gspi.adr = spi_data.adr;
			gspi.val = spi_data.val;
			custom_spi_write_reg(spi_data.adr, spi_data.val);
			break;
		case NMC_IOWSPIRREG:	/* spi read reg */
			{
				unsigned long data = 0;
				PDBG("%s: NMC_IOWSPIRREG\n", __FUNCTION__);	
				if(copy_from_user(&spi_data, (spi_ioc_t*)arg, sizeof(spi_ioc_t)) != 0) 
				{				
					PERR("%s: fail copy_from_user(&spi_data, (spi_ioc_t*)arg, sizeof(spi_ioc_t)) != 0\n", __FUNCTION__);				
					return -EINVAL;			
				}		
				
				gspi.adr = spi_data.adr;
				gspi.val = spi_data.val;
				custom_spi_read_reg(spi_data.adr, &data);
				spi_data.val = data;
				PDBG("spi adr = %x, val = %x\n", spi_data.adr, spi_data.val);	
				copy_to_user((void*)arg, &spi_data, sizeof(spi_ioc_t));	
			}
			break;
		case NMC_IOWSPIWBRST: 	/* spi burst write, adr and size */
			PDBG("%s: NMC_IOWSPIWBRST\n", __FUNCTION__);	
			if(copy_from_user(&spi_data, (spi_ioc_t*)arg, sizeof(spi_ioc_t)) != 0) 
			{					
				PERR("%s: fail copy_from_user(&spi_data, (spi_ioc_t*)arg, sizeof(spi_ioc_t)) != 0\n", __FUNCTION__);					
				return -EINVAL;				
			}		

			PDBG("spi adr = %x, val = %x\n", spi_data.adr, spi_data.val);		
			gspi.adr = spi_data.adr;
			gspi.val = spi_data.val;	/* size */
			break;
		case NMC_IOWSPIWBRSTBUF: 	/* spi burst write, buf */
			PDBG("%s: NMC_IOWSPIWBRSTBUF\n", __FUNCTION__);

			spi_rwbuf = kmalloc(gspi.val, GFP_KERNEL);
			if(!spi_rwbuf) {		
				PERR("no memory for spi rw buf\n");		
				return -ENOMEM;	
			}
			
			if(copy_from_user(spi_rwbuf, (unsigned char*)arg, gspi.val) != 0) 
			{				
				PERR("%s: fail copy_from_user(spi_rwbuf, (unsigned char*)arg, gspi.val) != 0\n", __FUNCTION__);				
				return -EINVAL;			
			}		
			custom_spi_write(gspi.adr, spi_rwbuf, gspi.val);

			kfree(spi_rwbuf);
			break;
		default:			
			PERR("%s: unknown command\n", __FUNCTION__);			
			break;	
	}	
	return result;
}


struct file_operations nmc_dev_fops = {	
	.owner =    THIS_MODULE,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36) 	
	.ioctl = nmc_dev_ioctl,
#else	
	.unlocked_ioctl = nmc_dev_ioctl,
#endif	
	.open =     nmc_dev_open,	
	.release =  nmc_dev_release,
};

int init_cdev(void)
{
	int result = 0;	
	dev_t dev = 0;	
	struct device *nmc_dev;		
	
	PINFO("%s\n", __FUNCTION__);	
	/*	
	*	create the character device	
	*/	
	result = alloc_chrdev_region(&dev, nmc_cdev_minor, 1, NMC_DEV_NAME);	
	nmc_cdev_major = MAJOR(dev);		
	if (result < 0) {		
		PERR("%s: fail to register cdev\n", __FUNCTION__);		
		return result;	
	}	
	
	printk("%s: nmc_cdev_major %d\n", __FUNCTION__, nmc_cdev_major);		
	
	cdev_init(&nmc_device_cdev, &nmc_dev_fops);	
	result = cdev_add (&nmc_device_cdev, dev, 1);	
	if (result) {		
		PERR("%s: fail cdev_add, err=%d\n", __FUNCTION__, result);		
		goto fail;	
	}	
	/*
	*	create the device node	
	*/	
	nmc_device_class = class_create(THIS_MODULE, NMC_DEV_NAME);	
	if(IS_ERR(nmc_device_class)) {		
		PERR("%s:  class create failed\n", __FUNCTION__);		
		result = -EFAULT;		
		goto fail;	
	}	
	nmc_dev = device_create(nmc_device_class, NULL, MKDEV(nmc_cdev_major, nmc_cdev_minor), NULL, NMC_DEV_NAME);	
	if(IS_ERR(nmc_dev)) {		
		PERR("%s:  device create failed\n", __FUNCTION__);		
		result = -EFAULT;		
		goto fail;	
	}		
	
	return 0;
fail:	
	deinit_cdev();	
	return result;
}

void  deinit_cdev(void)
{	
	PINFO("%s\n", __FUNCTION__);	
	cdev_del(&nmc_device_cdev);	
	unregister_chrdev_region(MKDEV(nmc_cdev_major, nmc_cdev_minor), 1);	
	device_destroy(nmc_device_class, MKDEV(nmc_cdev_major, nmc_cdev_minor));	
	class_destroy(nmc_device_class);	
	return;
}

