/*

* $Id: nmi_custom_gpio.c, Exp $
*/
//#include <linux/mmc/core.h>
//#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
//#include <linux/mmc/sdio_func.h>
//#include <linux/mmc/sdio_ids.h>
//#include <linux/mmc/sdio.h>
#include <linux/gpio.h>
#include <linux/delay.h>

//via
#include <mach/gpio_customize_ease.h>
//johnny add
#include <mach/hardware.h>
#include <mach/wmt_gpio.h>



//static struct mmc_host *mmc_host = NULL;

static GPIO_CTRL gpio_pmu_ctrl;
static GPIO_CTRL gpio_reset_ctrl;
static GPIO_CTRL gpio_wake_ctrl;
GPIO_CTRL gpio_irq_ctrl;

//extern void sdhci_bus_scan(void);

//extern int sprd_3rdparty_gpio_wifi_reset;
//extern int sprd_3rdparty_gpio_wifi_pwd;
//extern int sprd_3rdparty_gpio_wifi_wake;
//extern int sprd_3rdparty_gpio_wifi_irq;
/*
int sprd_3rdparty_gpio_wifi_reset = 255;
int sprd_3rdparty_gpio_wifi_pwd = 255;
int sprd_3rdparty_gpio_wifi_wake = 255;
int sprd_3rdparty_gpio_wifi_irq =255;
*/

void Rubbit_Chk_gpio_irq(int value)
{
	printk("[%d] gpio_irq_ctrl.gpio_int_ctl:%x\n",value, REG8_VAL(gpio_irq_ctrl.gpio_int_ctl.address));
	printk("[%d] gpio_irq_ctrl.gpio_int_status:%x\n",value, REG8_VAL(gpio_irq_ctrl.gpio_int_status.address));
	printk("[%d] gpio_get_value_any: 0x%x\n", value, gpio_get_value_any(&gpio_irq_ctrl));
}

void irq_gpio_init(void)
{
	//if(!gpio_irq_isEnable_any(&gpio_irq_ctrl) || !gpio_irq_state_any(&gpio_irq_ctrl))

#if 1
	printk("[MMM] irq is Enable (%d), irq state (%d)\n" 
			, gpio_irq_isEnable_any(&gpio_irq_ctrl)
			, gpio_irq_state_any(&gpio_irq_ctrl));
			
#endif

	
	{
		/*init gpio interruption pin: input, low trigger, pull up, disable int*/
		gpio_enable_any(&gpio_irq_ctrl, INPUT);
		set_gpio_irq_triger_mode_any(&gpio_irq_ctrl, IRQ_LOW);
		gpio_pull_enable_any(&gpio_irq_ctrl,PULL_UP);

		gpio_clean_irq_status_any(&gpio_irq_ctrl);

		enable_gpio_int_any(&gpio_irq_ctrl,INT_DIS);
	}
	
}

void irq_gpio_deinit(void)
{
	//disable_irq_wake();
	printk("[MMM] IRQ GPIO DEINIT, called [2] ~~~~~~~~~~~~~~~~~~~\n");
	Rubbit_Chk_gpio_irq(3);
	/* below: no interrupt @ 2nd time
	gpio_enable_any(&gpio_irq_ctrl, INPUT);
	gpio_pull_enable_any(&gpio_irq_ctrl, PULL_DOWN);
	*/

	// below not working (no interrupt @ 2nd time)
	//gpio_disable_any(&gpio_irq_ctrl);
}



void wifi_gpio_reset(int high_or_low)
{
	printk("%s======hi= %d\n",__FUNCTION__,high_or_low);
/*
    if(sprd_3rdparty_gpio_wifi_reset > 0)
    {printk("wifi reset gpio = %d\n", sprd_3rdparty_gpio_wifi_reset);
        gpio_direction_output(sprd_3rdparty_gpio_wifi_reset, high_or_low);
        gpio_set_value(sprd_3rdparty_gpio_wifi_reset,high_or_low);
    }
 */

	gpio_enable_any(&gpio_reset_ctrl, OUTPUT);
	gpio_set_value_any(&gpio_reset_ctrl,high_or_low);

}
static void wifi_card_set_gpios(int high_or_low)
{
/*
	printk("%s**********hi=%d\n",__FUNCTION__,high_or_low);
    if(sprd_3rdparty_gpio_wifi_reset > 0)
    {printk("wifi reset gpio = %d\n", sprd_3rdparty_gpio_wifi_reset);
        gpio_direction_output(sprd_3rdparty_gpio_wifi_reset, high_or_low);
        gpio_set_value(sprd_3rdparty_gpio_wifi_reset,high_or_low);
    }

    if(sprd_3rdparty_gpio_wifi_pwd > 0)
    {printk("wifi pwd gpio = %d\n",sprd_3rdparty_gpio_wifi_pwd);
        gpio_direction_output(sprd_3rdparty_gpio_wifi_pwd,high_or_low);
        gpio_set_value(sprd_3rdparty_gpio_wifi_pwd,high_or_low);
    }
    if(sprd_3rdparty_gpio_wifi_wake > 0)
    { printk("wifi wake gpio = %d\n",sprd_3rdparty_gpio_wifi_wake);
        gpio_direction_output(sprd_3rdparty_gpio_wifi_wake,high_or_low);
        gpio_set_value(sprd_3rdparty_gpio_wifi_wake,high_or_low);
    }*/

	parse_gpio_ctrl_string("e3:4:5a:9a:da:49a:4da", &gpio_pmu_ctrl, "wmt.nmc.pmu");//kpadcolu0 pmu enable
	parse_gpio_ctrl_string("e3:5:5a:9a:da:49a:4da", &gpio_reset_ctrl, "wmt.nmc.reset");//kpadcolu1 reset
	parse_gpio_ctrl_string("e3:6:5a:9a:da:49a:4da", &gpio_wake_ctrl, "wmt.nmc.wake");//kpadcolu2 wakeup
	parse_gpio_ctrl_string("df:6:40:80:00:306:360:480:4c0", &gpio_irq_ctrl, "wmt.nmc.irq");//irq gpio6
	gpio_irq_ctrl.gpio_input_data.address = 0xfe110000;
	printk("=====================\n");
	printf_gpio_ctrl(&gpio_pmu_ctrl);
	printf_gpio_ctrl(&gpio_reset_ctrl);
	printf_gpio_ctrl(&gpio_wake_ctrl);
	printf_gpio_ctrl(&gpio_irq_ctrl);
	printk("=====================high_or_low:%x \n",high_or_low);

	gpio_enable_any(&gpio_pmu_ctrl, OUTPUT);
	gpio_set_value_any(&gpio_pmu_ctrl,high_or_low);

	gpio_enable_any(&gpio_wake_ctrl, OUTPUT);
	gpio_set_value_any(&gpio_wake_ctrl,high_or_low);

	gpio_enable_any(&gpio_reset_ctrl, OUTPUT);//enable output
	gpio_set_value_any(&gpio_reset_ctrl,high_or_low);//output hign

	//gpio_enable_any(&gpio_irq_ctrl, OUTPUT);
	//gpio_set_value_any(&gpio_irq_ctrl,high_or_low);

}


/* Customer function to control hw specific wlan gpios */
void NmiWifiCardPower(int power)
{
	printk("%s, power=%d\n", __FUNCTION__, power);

	wifi_card_set_gpios (power);
	if(power){
		wifi_gpio_reset(0);
		msleep(100);
		wifi_gpio_reset(1);
		msleep(100);
	}else{
		wifi_gpio_reset(0);
	}
}


