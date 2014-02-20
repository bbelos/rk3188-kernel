/*

* $Id: nmi_custom_gpio.c, Exp $
*/
#include <linux/mmc/host.h>
#include <linux/gpio.h>
#include <linux/delay.h>


#include <mach/gpio_customize_ease.h>
#include <mach/hardware.h>
#include <mach/wmt_gpio.h>

#include <linux/gpio.h>
#include <mach/wmt_iomux.h>


static int gpio_chipen_ctrl = -1;
static int gpio_reset_ctrl = -1;


static void wifi_gpio_reset(int high_or_low)
{
	printk("%s======hi= %d\n",__FUNCTION__,high_or_low);

    gpio_direction_output(gpio_reset_ctrl, high_or_low);
}

static void wifi_card_init_gpios(void)
{
	int ret;

	gpio_chipen_ctrl = WMT_PIN_GP62_SUSGPIO1; // ??? simon
	gpio_reset_ctrl = WMT_PIN_GP1_GPIO14;

	ret = gpio_request(gpio_chipen_ctrl, "nmc1000 chipen pin");
	if (ret < 0) {
		printk("reques gpio:%x failed!!! for nmc1000\n", gpio_chipen_ctrl);
		return ;
	} else {
		printk("request gpio:%d for nmc1000 success!!!\n", gpio_chipen_ctrl);
	}
	gpio_direction_output(gpio_chipen_ctrl, 0);

	ret = gpio_request(gpio_reset_ctrl, "nmc1000 reset pin");
	if (ret < 0) {
		printk("reques gpio:%x failed!!! for nmc1000\n", gpio_reset_ctrl);
		return ;
	} else {
		printk("request gpio:%d for nmc1000 success!!!\n", gpio_reset_ctrl);
	}
	gpio_direction_output(gpio_reset_ctrl, 0);	
}

static void wifi_card_release_gpios(void)
{
	gpio_free(gpio_chipen_ctrl);
	gpio_free(gpio_reset_ctrl);
}

static void wifi_card_chipen_gpios(int high_or_low)
{
	gpio_direction_output(gpio_chipen_ctrl, high_or_low);
}

/* Customer function to control hw specific wlan gpios */
void NmiWifiCardPower(int power)
{
	printk("%s, power=%d\n", __FUNCTION__, power);

	if(power){
        wifi_card_init_gpios();
		wifi_gpio_reset(0);
        wifi_card_chipen_gpios (power);
		msleep(35);
		wifi_gpio_reset(1);
		msleep(20);
	}else{
		wifi_gpio_reset(0);
		msleep(10);
		wifi_card_chipen_gpios (power);
		msleep(10);
		wifi_card_release_gpios();
	}
}


