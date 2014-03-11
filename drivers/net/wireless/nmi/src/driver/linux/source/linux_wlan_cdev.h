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
#ifndef LINUX_WLAN_CDEV_H
#define LINUX_WLAN_CDEV_H 
enum {
	_IOPWRON,
	_IOPWROFF,
	_IORWCMD52,
	_IOWCMD53IOCTL,
	_IORWCMD53BUF,
	_IOWSPIWREG,		/* spi write reg */
	_IOWRSPIRREG,		/* spi read reg */
	_IOWSPIWBRST,	/* spi burst write */
	_IOWSPIWBRSTBUF,	/* spi burst write buf */
	_IOMAXNR
};

typedef struct {
	unsigned long adr;
	unsigned long val;
}spi_ioc_t;

#define NMC_IOC_MAGIC  'N'

/* SDIO */
#define NMC_IOWRCMD52 		_IOWR(NMC_IOC_MAGIC, _IORWCMD52, sdio_cmd52_t)
#define NMC_IOWCMD53 		_IOW(NMC_IOC_MAGIC, _IOWCMD53IOCTL, sdio_cmd53_ioctl_t)
#define NMC_IOWRCMD53BUF	_IOWR(NMC_IOC_MAGIC, _IORWCMD53BUF, unsigned char)

/* SPI */
/* write reg */
#define NMC_IOWSPIWREG		_IOW(NMC_IOC_MAGIC, _IOWSPIWREG, spi_ioc_t)
/* read reg */
#define NMC_IOWSPIRREG		_IOWR(NMC_IOC_MAGIC, _IOWRSPIRREG, spi_ioc_t)
/* spi burst write, adr and size */
#define NMC_IOWSPIWBRST		_IOW(NMC_IOC_MAGIC, _IOWSPIWBRST, spi_ioc_t)
/* spi burst write, buf */
#define NMC_IOWSPIWBRSTBUF	_IOW(NMC_IOC_MAGIC, _IOWSPIWBRSTBUF, unsigned char)

#define NMC_IOPWRON 		_IO(NMC_IOC_MAGIC, _IOPWRON)			/* power on */
#define NMC_IOPWROFF		_IO(NMC_IOC_MAGIC, _IOPWROFF)			/* power off */

#define NMC_IOC_MAXNR	_IOMAXNR
#endif


