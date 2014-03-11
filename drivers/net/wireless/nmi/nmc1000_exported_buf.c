#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>	
#include <linux/slab.h>		

#define LINUX_RX_SIZE	(96*1024)
#define LINUX_TX_SIZE	(64*1024)
#define NMC1000_FW_SIZE (4*1024)

#define DECLARE_NMC_BUFFER(name)	\
	void *exported_##name = NULL;

#define MALLOC_NMC_BUFFER(name, size)	\
	exported_##name = kmalloc(size, GFP_KERNEL);	\
	if(!exported_##name){	\
		printk("fail to alloc: %s memory\n", exported_##name);	\
		return -ENOBUFS;	\
	}

#define FREE_NMC_BUFFER(name)	\
	kfree(exported_##name);

/*
* Add necessary buffer pointers
*/
DECLARE_NMC_BUFFER(g_tx_buf)
DECLARE_NMC_BUFFER(g_rx_buf)
DECLARE_NMC_BUFFER(g_fw_buf)

void *get_tx_buffer(void)
{
	printk("[mem] tx_buf = 0x%x\n", exported_g_tx_buf);
	return exported_g_tx_buf;
}
EXPORT_SYMBOL(get_tx_buffer);

void *get_rx_buffer(void)
{
	printk("[mem] rx_buf = 0x%x\n", exported_g_rx_buf);
	return exported_g_rx_buf;
}
EXPORT_SYMBOL(get_rx_buffer);

void *get_fw_buffer(void)
{
	printk("[mem] fw_buf = 0x%x\n", exported_g_fw_buf);
	return exported_g_fw_buf;
}
EXPORT_SYMBOL(get_fw_buffer);

static int __init nmc_module_init(void)
{
	printk("nmc_module_init\n");	
	/*
	* alloc necessary memory
	*/
	MALLOC_NMC_BUFFER(g_tx_buf, LINUX_TX_SIZE)
	MALLOC_NMC_BUFFER(g_rx_buf, LINUX_RX_SIZE)
	MALLOC_NMC_BUFFER(g_fw_buf, NMC1000_FW_SIZE)
	
	return 0;
}

static void __exit nmc_module_deinit(void)
{
	printk("nmc_module_deinit\n");	
	FREE_NMC_BUFFER(g_tx_buf)
	FREE_NMC_BUFFER(g_rx_buf)
	FREE_NMC_BUFFER(g_fw_buf)
	
	return;
}

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Tony Cho");
MODULE_DESCRIPTION("NMC1xxx Memory Manager");
module_init(nmc_module_init);
module_exit(nmc_module_deinit);