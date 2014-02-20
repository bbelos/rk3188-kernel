/*!  
*  @file	NMI_WFI_NetDevice.h
*  @brief	Definitions for the network module
*  @author	mdaftedar
*  @date	01 MAR 2012
*  @version	1.0
*/
#ifndef NMI_WFI_NETDEVICE
#define NMI_WFI_NETDEVICE
/*
* Macros to help debugging
*/
   
#undef PDEBUG             /* undef it, just in case */
#ifdef SNULL_DEBUG
#  ifdef __KERNEL__
/* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "snull: " fmt, ## args)
#  else
/* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif
  
#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */
  
  
/* These are the flags in the statusword */
#define NMI_WFI_RX_INTR 0x0001
#define NMI_WFI_TX_INTR 0x0002
  
/* Default timeout period */
#define NMI_WFI_TIMEOUT 5   /* In jiffies */
#define NMI_MAX_NUM_PMKIDS  16
#define PMKID_LEN  16
#define PMKID_FOUND 1
 #define NUM_STA_ASSOCIATED 8

#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/interrupt.h> /* mark_bh */
#include <linux/time.h>
#include <linux/in.h>
#include <linux/netdevice.h>   /* struct device, and other headers */
#include <linux/etherdevice.h> /* eth_type_trans */
#include <linux/ip.h>          /* struct iphdr */
#include <linux/tcp.h>         /* struct tcphdr */
#include <linux/skbuff.h>

#include <linux/ieee80211.h>
#include <net/cfg80211.h>

#include <linux/ieee80211.h>
#include <net/cfg80211.h>
#include <net/ieee80211_radiotap.h>
#include <linux/if_arp.h>


#include <linux/in6.h>
#include <asm/checksum.h>
#include "host_interface.h"
#include "nmi_wlan.h"
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,30)
#include <net/wireless.h>
#else
#include <linux/wireless.h>	// tony, 2013-06-12
#endif


#define FLOW_CONTROL_LOWER_THRESHOLD	128
#define FLOW_CONTROL_UPPER_THRESHOLD	256

/*iftype*/


enum stats_flags 
{
	NMI_WFI_RX_PKT = 1 << 0,
	NMI_WFI_TX_PKT = 1 << 1,
};

struct NMI_WFI_stats
{

	unsigned long   rx_packets;
	unsigned long   tx_packets;
	unsigned long   rx_bytes;
	unsigned long   tx_bytes;
	u64   rx_time;
	u64   tx_time;

};

/*
* This structure is private to each device. It is used to pass
* packets in and out, so there is place for a packet
*/

#define RX_BH_KTHREAD 0 
#define RX_BH_WORK_QUEUE 1 
#define RX_BH_THREADED_IRQ 2
#define num_reg_frame 2
/* 
 * If you use RX_BH_WORK_QUEUE on LPC3131: You may lose the first interrupt on 
 * LPC3131 which is important to get the MAC start status when you are blocked inside
 * linux_wlan_firmware_download() which blocks mac_open().
 */
#if defined (NM73131_0_BOARD)
#define RX_BH_TYPE  RX_BH_KTHREAD
#else
#define RX_BH_TYPE  RX_BH_WORK_QUEUE
#endif

struct nmi_wfi_key {
  u8 * key;
  u8 * seq;
  int key_len;
  int seq_len;
  u32 cipher;
};  

struct sta_info
{
	NMI_Uint8 au8Sta_AssociatedBss[MAX_NUM_STA][ETH_ALEN];
};

#ifdef NMI_P2P
/*Parameters needed for host interface for  remaining on channel*/
struct nmi_wfi_p2pListenParams
{
	struct ieee80211_channel * pstrListenChan;
	enum nl80211_channel_type tenuChannelType;
	NMI_Uint32 u32ListenDuration;
	NMI_Uint64  u64ListenCookie;
};

#endif	/*NMI_P2P*/

struct NMI_WFI_priv {
	struct wireless_dev *wdev;
	struct cfg80211_scan_request* pstrScanReq;
	
	#ifdef NMI_P2P
	NMI_Uint32 u32listen_freq;
	struct nmi_wfi_p2pListenParams strRemainOnChanParams;
	NMI_Uint64 u64tx_cookie;
	
	#endif

	/*Added by Amr - BugID_4793*/
	NMI_Uint8 u8CurrChannel;
	
	NMI_Bool bCfgScanning;
	NMI_Uint32 u32RcvdChCount;

	NMI_Uint32 u32LastScannedNtwrksCountShadow;
	tstrNetworkInfo astrLastScannedNtwrksShadow[MAX_NUM_SCANNED_NETWORKS_SHADOW];

	NMI_Uint8 au8AssociatedBss[ETH_ALEN];
	struct sta_info  assoc_stainfo;
       struct net_device_stats stats;
       NMI_Uint8 monitor_flag;
       int status;
       struct NMI_WFI_packet *ppool;
       struct NMI_WFI_packet *rx_queue;  /* List of incoming packets */
       int rx_int_enabled;
       int tx_packetlen;
       u8 *tx_packetdata;
       struct sk_buff *skb;
    spinlock_t lock;
    struct net_device *dev;
    struct napi_struct napi;
 	NMI_WFIDrvHandle hNMIWFIDrv;
 	tstrHostIFpmkidAttr pmkid_list;
	struct NMI_WFI_stats netstats;  
	NMI_Uint8 NMI_WFI_wep_default;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31)
#define WLAN_KEY_LEN_WEP104 13
#endif
	NMI_Uint8 NMI_WFI_wep_key[4][WLAN_KEY_LEN_WEP104];
	NMI_Uint8 NMI_WFI_wep_key_len[4];	
	struct net_device* real_ndev;	/* The real interface that the monitor is on */
	struct nmi_wfi_key* nmi_gtk[MAX_NUM_STA];
	struct nmi_wfi_key* nmi_ptk[MAX_NUM_STA];
	NMI_Uint8  nmi_groupkey;
};

typedef struct
{
	NMI_Uint16 frame_type;
	NMI_Bool      reg;

}struct_frame_reg;

typedef struct{
	int mac_status;
	int nmc1000_initialized;

	#ifdef NMI_P2P
	struct_frame_reg g_struct_frame_reg[num_reg_frame];
	#endif
	#if (!defined NMI_SDIO) || (defined NMI_SDIO_IRQ_GPIO)
	unsigned short dev_irq_num;
	#endif
	nmi_wlan_oup_t oup;
	int monitor_flag;
	int close;
	struct net_device_stats netstats; 
	struct mutex txq_cs;

	/*Added by Amr - BugID_4720*/
	struct mutex txq_add_to_head_cs;
	spinlock_t txq_spinlock;
	
	struct mutex rxq_cs;
	struct mutex hif_cs;

	//struct mutex txq_event;
	struct semaphore rxq_event;
	struct semaphore cfg_event;
	struct semaphore sync_event;

	struct semaphore txq_event;
	//struct completion txq_event;

#if (RX_BH_TYPE == RX_BH_WORK_QUEUE)
		struct work_struct rx_work_queue;
#elif (RX_BH_TYPE == RX_BH_KTHREAD)
		struct task_struct* rx_bh_thread;
		struct semaphore rx_sem;
#endif


	
	struct semaphore rxq_thread_started;
	struct semaphore txq_thread_started;

	struct task_struct* rxq_thread;
	struct task_struct* txq_thread;

	unsigned char eth_src_address[6];
	unsigned char eth_dst_address[6];

	const struct firmware* nmc_firmware; /* Bug 4703 */

	struct net_device* nmc_netdev;
	struct net_device* real_ndev;
#ifdef NMI_SDIO
	int already_claim;
	struct sdio_func* nmc_sdio_func;
#else
	struct spi_device* nmc_spidev;
#endif

	NMI_Uint8 iftype;
} linux_wlan_t;

struct NMI_WFI_mon_priv
{
	struct net_device* real_ndev;
};
extern struct net_device *NMI_WFI_devs[];

#endif
