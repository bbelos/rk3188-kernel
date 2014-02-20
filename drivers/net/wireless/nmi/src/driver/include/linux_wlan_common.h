#ifndef LINUX_WLAN_COMMON_H
#define LINUX_WLAN_COMMON_H

enum debug_region{
	Generic_debug = 0,
	Hostapd_debug,
	Hostinf_debug,
	CFG80211_debug,
	Coreconfig_debug,
	Interrupt_debug,
	TX_debug,
	RX_debug,
	Lock_debug,
	Tcp_enhance,
	/*Added by amr - BugID_4720*/
	Spin_debug,
	
	Init_debug,
	Bus_debug,
	Mem_debug,
	Firmware_debug,
	COMP = 0xFFFFFFFF,
};

#define GENERIC_DBG	  		(1<<Generic_debug)
#define HOSTAPD_DBG       	(1<<Hostapd_debug)
#define HOSTINF_DBG	  		(1<<Hostinf_debug)
#define CORECONFIG_DBG  	(1<<Coreconfig_debug)
#define CFG80211_DBG      	(1<<CFG80211_debug)
#define INT_DBG		  		(1<<Interrupt_debug)
#define TX_DBG		 		(1<<TX_debug)
#define RX_DBG		 		(1<<RX_debug)
#define LOCK_DBG	  		(1<<Lock_debug)
#define TCP_ENH	  			(1<<Tcp_enhance)


/*Added by Amr - BugID_4720*/
#define SPIN_DEBUG 			(1<<Spin_debug)

#define INIT_DBG	  	  		(1<<Init_debug)
#define BUS_DBG		  		(1<<Bus_debug)
#define MEM_DBG		  		(1<<Mem_debug)
#define FIRM_DBG	  		(1<<Firmware_debug)

#if defined (NMC_DEBUGFS)
extern void kmsgdump_write(char *fmt, ...);
extern int nmc_debugfs_init(void);
extern void nmc_debugfs_remove(void);

extern atomic_t REGION;
extern atomic_t DEBUG_LEVEL;

#define DEBUG		(1 << 0)
#define INFO		(1 << 1)
#define WRN			(1 << 2)
#define ERR			(1 << 3)

#define PRINT_D(region,...)	do{ if((atomic_read(&DEBUG_LEVEL) & DEBUG) && ((atomic_read(&REGION))&(region))){printk("DBG [%s: %d]",__FUNCTION__,__LINE__);\
							kmsgdump_write("DBG [%s: %d]",__FUNCTION__,__LINE__);\
							kmsgdump_write(__VA_ARGS__);\
							printk(__VA_ARGS__);}}while(0)
							
#define PRINT_INFO(region,...) do{ if((atomic_read(&DEBUG_LEVEL) & INFO) && ((atomic_read(&REGION))&(region))){printk("INFO [%s]",__FUNCTION__);\
							kmsgdump_write("INFO [%s]",__FUNCTION__);\
							kmsgdump_write(__VA_ARGS__);\
							printk(__VA_ARGS__);}}while(0)

#define PRINT_WRN(region,...) do{ if((atomic_read(&DEBUG_LEVEL) & WRN) && ((atomic_read(&REGION))&(region))){printk("WRN [%s: %d]",__FUNCTION__,__LINE__);\
							kmsgdump_write("WRN [%s: %d]",__FUNCTION__,__LINE__);\
							kmsgdump_write(__VA_ARGS__);\
							printk(__VA_ARGS__);}}while(0)

#define PRINT_ER(...)	do{ if((atomic_read(&DEBUG_LEVEL) & ERR)) {	\
							printk("ERR [%s: %d]",__FUNCTION__,__LINE__);\
							kmsgdump_write("ERR [%s: %d]",__FUNCTION__,__LINE__);\
							kmsgdump_write(__VA_ARGS__);\
							printk(__VA_ARGS__);}}while(0)

#else

#define REGION	 INIT_DBG | GENERIC_DBG | CFG80211_DBG | FIRM_DBG

#define DEBUG	    1
#define INFO        0
#define WRN         0
#define PRINT_D(region,...)	do{ if(DEBUG == 1 && ((REGION)&(region))){printk("DBG [%s: %d]",__FUNCTION__,__LINE__);\
							printk(__VA_ARGS__);}}while(0)
							
#define PRINT_INFO(region,...) do{ if(INFO == 1 && ((REGION)&(region))){printk("INFO [%s]",__FUNCTION__);\
							printk(__VA_ARGS__);}}while(0)

#define PRINT_WRN(region,...) do{ if(WRN == 1 && ((REGION)&(region))){printk("WRN [%s: %d]",__FUNCTION__,__LINE__);\
							printk(__VA_ARGS__);}}while(0)

#define PRINT_ER(...)	do{ printk("ERR [%s: %d]",__FUNCTION__,__LINE__);\
							printk(__VA_ARGS__);}while(0)
#endif

#define FN_IN	//PRINT_D(">>> \n")
#define FN_OUT	//PRINT_D("<<<\n")

//#define LINUX_RX_SIZE	(8*1024)
#define LINUX_TX_SIZE	(64*1024)

#if defined (NM73131_0_BOARD)

#define MODALIAS "nmc_spi"
#define GPIO_NUM	IRQ_NMC1000_GPIO

#elif defined (BEAGLE_BOARD)
	#define SPI_CHANNEL	4
	
	#if SPI_CHANNEL == 4
		#define MODALIAS 	"nmc_spi4"
		#define GPIO_NUM	162
	#else
		#define MODALIAS 	"nmc_spi3"
		#define GPIO_NUM	133
	#endif
#elif defined(PANDA_BOARD)
	#define MODALIAS 	"NMI_SPI"
	#define GPIO_NUM	139
#elif defined(PLAT_WMS8304)		// rachel
	#define MODALIAS 	"nmi_spi"
	#define GPIO_NUM	139
#elif defined (PLAT_RKXXXX)
#define MODALIAS	"NMI_IRQ"
#define GPIO_NUM	RK30_PIN3_PD2 //RK30_PIN3_PA1
//RK30_PIN3_PD2
//RK2928_PIN1_PA7
#else
	#define MODALIAS 	"NMI_SPI"
	#define GPIO_NUM	139

#endif
	

void linux_wlan_enable_irq(void);
#endif
