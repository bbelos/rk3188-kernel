/*
 * Copyright (c) 2013, T-CHIP Corporation.
 *
 */


#ifndef __ATX8_H
#define __ATX8_H

#include <linux/types.h>

struct atx8_iomux {
	char	*name;
    int     fgpio;
    int     fi2c;
};

struct atx8_gpio {
    int     io;
    char    *name;
    int     enable; // disable = !enable
    struct atx8_iomux  iomux;
};

struct atx8_platform_data {
    char                    *name;
	void (*iomux_set)(struct atx8_gpio *gpio, int iomux);
    int                 type;
    struct atx8_gpio   cs_gpio;
    struct atx8_gpio   sda_gpio;
    struct atx8_gpio   scl_gpio;
};


#endif /* __ATX8_H */


