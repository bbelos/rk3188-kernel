#include "NMI_WFI_NetDevice.h"

#include <linux/mmc/sdio_func.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/host.h>



#if defined (NM73131_0_BOARD)
#define SDIO_MODALIAS "nmi_sdio"
#else
#define SDIO_MODALIAS "nmc1000_sdio"
#endif

#ifdef NMC_ASIC_A0
#if defined (NM73131_0_BOARD)
 #define MAX_SPEED 50000000
#elif defined (PLAT_ALLWINNER_A10)
 #define MAX_SPEED 50000000/*15000000*/
#elif defined (PLAT_ALLWINNER_A20)
 #define MAX_SPEED 45000000 //40000000 //50000000
#elif defined (PLAT_ALLWINNER_A23)
 #define MAX_SPEED 6000000
#elif defined (PLAT_PANDA_ES_OMAP4460)
 #define MAX_SPEED 25000000 //johnny change
#elif defined(PLAT_WM8880)
 #define MAX_SPEED /*20000000*/50000000	/* tony increased to 50000000 */	
#elif defined (PLAT_UBUNTU_X86)  // tony
 #define MAX_SPEED 25000000
#elif defined (PLAT_RKXXXX)
#define MAX_SPEED 50000000
#else
#define MAX_SPEED 50000000
#endif
#else /* NMC_ASIC_A0 */
/* Limit clk to 6MHz on FPGA. */
#define MAX_SPEED 6000000
#endif /* NMC_ASIC_A0 */


struct sdio_func* local_sdio_func = NULL;
extern linux_wlan_t* g_linux_wlan;
extern int nmc_netdev_init(void);
extern int sdio_clear_int(void);
extern void nmi_handle_isr(void);

static unsigned int sdio_default_speed=0;

#define SDIO_VENDOR_ID_NMI 0x0296
#define SDIO_DEVICE_ID_NMI 0x5347

static const struct sdio_device_id nmi_sdio_ids[] = {
	{ SDIO_DEVICE(SDIO_VENDOR_ID_NMI,SDIO_DEVICE_ID_NMI) },
};


static void nmi_sdio_interrupt(struct sdio_func *func)
{
#ifndef NMI_SDIO_IRQ_GPIO
	sdio_release_host(func);
	nmi_handle_isr();
	sdio_claim_host(func);
#endif
}


int linux_sdio_cmd52(sdio_cmd52_t *cmd){
	struct sdio_func *func = g_linux_wlan->nmc_sdio_func;
	int ret;
	u8 data;

	sdio_claim_host(func);

	func->num = cmd->function;
	if (cmd->read_write) {	/* write */
		if (cmd->raw) {
			sdio_writeb(func, cmd->data, cmd->address, &ret);
			data = sdio_readb(func, cmd->address, &ret);
			cmd->data = data;
		} else {
			sdio_writeb(func, cmd->data, cmd->address, &ret);
		}
	} else {	/* read */
		data = sdio_readb(func, cmd->address, &ret);
		cmd->data = data;
	}

	sdio_release_host(func);

	if (ret < 0) {
		printk("nmi_sdio_cmd52..failed, err(%d)\n", ret);
		return 0;
	}
	return 1;
}


 int linux_sdio_cmd53(sdio_cmd53_t *cmd){
	struct sdio_func *func = g_linux_wlan->nmc_sdio_func;
	int size, ret;

	sdio_claim_host(func);

	func->num = cmd->function;
	func->cur_blksize = cmd->block_size;
	if (cmd->block_mode)
		size = cmd->count * cmd->block_size;
	else
		size = cmd->count;

	if (cmd->read_write) {	/* write */
		ret = sdio_memcpy_toio(func, cmd->address, (void *)cmd->buffer, size);		
	} else {	/* read */
		ret = sdio_memcpy_fromio(func, (void *)cmd->buffer, cmd->address,  size);		
	}

	sdio_release_host(func);


	if (ret < 0) {
		printk("nmi_sdio_cmd53..failed, err(%d)\n", ret);
		return 0;
	}

	return 1;
}

volatile int probe = 0; //COMPLEMENT_BOOT
static int linux_sdio_probe(struct sdio_func *func, const struct sdio_device_id *id){	
	PRINT_D(INIT_DBG,"SDIO probe \n");

#ifdef COMPLEMENT_BOOT
	if(local_sdio_func != NULL)
	{		
		local_sdio_func = func;
		probe = 1;
		PRINT_D(INIT_DBG,"local_sdio_func isn't NULL\n");
		return 0;
	}
#endif
	PRINT_D(INIT_DBG,"Initializing netdev\n");
	local_sdio_func = func;

    if(nmc_netdev_init()){
		PRINT_ER("Couldn't initialize netdev\n");
		return -1;
	}

	return 0;
}

static void linux_sdio_remove(struct sdio_func *func)
{
	/**
		TODO
	**/
	
}

struct sdio_driver nmc_bus = {
	.name		= SDIO_MODALIAS,
	.id_table	= nmi_sdio_ids,
	.probe		= linux_sdio_probe,
	.remove		= linux_sdio_remove,
};

void enable_sdio_interrupt(void){
	int ret;

#ifndef NMI_SDIO_IRQ_GPIO
	
	sdio_claim_host(local_sdio_func);
	ret = sdio_claim_irq(local_sdio_func, nmi_sdio_interrupt);
	sdio_release_host(local_sdio_func);

	if (ret < 0) {
		PRINT_ER("can't claim sdio_irq, err(%d)\n", ret);
	}
#endif
}

void disable_sdio_interrupt(void){

#ifndef NMI_SDIO_IRQ_GPIO
	int ret;

	PRINT_D(INIT_DBG,"disable_sdio_interrupt IN\n");

	sdio_claim_host(local_sdio_func);

	ret = sdio_release_irq(local_sdio_func);

	if (ret < 0) {
		PRINT_ER("can't release sdio_irq, err(%d)\n", ret);
	}
	sdio_release_host(local_sdio_func);
	
	PRINT_D(INIT_DBG,"disable_sdio_interrupt OUT\n");
#endif
}

static int linux_sdio_set_speed(int speed)
{
#if defined(PLAT_AML8726_M3)

#else
	struct mmc_ios ios;
	sdio_claim_host(local_sdio_func);
	
	memcpy((void *)&ios,(void *)&local_sdio_func->card->host->ios,sizeof(struct mmc_ios));
	local_sdio_func->card->host->ios.clock = speed;
	ios.clock = speed;
	local_sdio_func->card->host->ops->set_ios(local_sdio_func->card->host,&ios);
	sdio_release_host(local_sdio_func);
	PRINT_ER("@@@@@@@@@@@@ change SDIO speed to %d @@@@@@@@@\n", speed);
#endif
	return 1;
}

static int linux_sdio_get_speed(void)
{
	return local_sdio_func->card->host->ios.clock;
}

int linux_sdio_init(void* pv){

#if defined(PLAT_AML8726_M3)
//[[ rachel - copy from v.3.0 (by johnny)
 #ifdef NMC1000_SINGLE_TRANSFER
 #define NMI_SDIO_BLOCK_SIZE 256
 #else
 #define NMI_SDIO_BLOCK_SIZE 512
 #endif

	int err;

	sdio_claim_host(local_sdio_func);

	printk("johnny linux_sdio_init %d\n", local_sdio_func->num );

	local_sdio_func->num = 0;
	sdio_set_block_size(local_sdio_func, NMI_SDIO_BLOCK_SIZE);
	
	local_sdio_func->num = 1;
	sdio_enable_func(local_sdio_func);
	sdio_set_block_size(local_sdio_func, NMI_SDIO_BLOCK_SIZE);
	
	sdio_release_host(local_sdio_func);

	printk("johnny linux_sdio_init\n");
#endif

	sdio_default_speed = linux_sdio_get_speed();
	return 1;
}

void linux_sdio_deinit(void *pv){
	sdio_unregister_driver(&nmc_bus);
}

int linux_sdio_set_max_speed(void)
{
	return linux_sdio_set_speed(MAX_SPEED);
}

int linux_sdio_set_default_speed(void)
{
	return linux_sdio_set_speed(sdio_default_speed);
}



