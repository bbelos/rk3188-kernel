/*!  
*  @file	NMI_WFI_NetDevice.c
*  @brief	File Operations OS wrapper functionality
*  @author	mdaftedar
*  @sa		NMI_WFI_NetDevice.h 
*  @date	01 MAR 2012
*  @version	1.0
*/

#ifdef SIMULATION

#include "linux/include/NMI_WFI_CfgOperations.h"
#include "host_interface.h"


MODULE_AUTHOR("Mai Daftedar");
MODULE_LICENSE("Dual BSD/GPL");


struct net_device *NMI_WFI_devs[2];

/*
* Transmitter lockup simulation, normally disabled.
*/
static int lockup = 0;
module_param(lockup, int, 0);
  
static int timeout = NMI_WFI_TIMEOUT;
module_param(timeout, int, 0);
  
/*
* Do we run in NAPI mode?
*/
static int use_napi = 0;
module_param(use_napi, int, 0);
  
  
/*
* A structure representing an in-flight packet.
*/
struct NMI_WFI_packet {
     struct NMI_WFI_packet *next;
     struct net_device *dev;
     int     datalen;
     u8 data[ETH_DATA_LEN];
};



int pool_size = 8;
module_param(pool_size, int, 0);
  

static void NMI_WFI_TxTimeout(struct net_device *dev);
static void (*NMI_WFI_Interrupt)(int, void *, struct pt_regs *);

/**
*  @brief 	NMI_WFI_SetupPool
*  @details 	Set up a device's packet pool.
*  @param[in] 	struct net_device *dev : Network Device Pointer
*  @return 	NONE	
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
 void NMI_WFI_SetupPool(struct net_device *dev)
 {
         struct NMI_WFI_priv *priv = netdev_priv(dev);
         int i;
         struct NMI_WFI_packet *pkt;
 
         priv->ppool = NULL;
         for (i = 0; i < pool_size; i++) {
                 pkt = kmalloc (sizeof (struct NMI_WFI_packet), GFP_KERNEL);
                 if (pkt == NULL) {
                         printk (KERN_NOTICE "Ran out of memory allocating packet pool\n");
                         return;
                 }
                 pkt->dev = dev;
                 pkt->next = priv->ppool;
                 priv->ppool = pkt;
         }
 }

/**
*  @brief 	NMI_WFI_TearDownPool
*  @details 	Internal cleanup function that's called after the network device
		driver is unregistered
*  @param[in] 	struct net_device *dev : Network Device Driver
*  @return 	NONE
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/ 
void NMI_WFI_TearDownPool(struct net_device *dev)
 {
         struct NMI_WFI_priv *priv = netdev_priv(dev);
         struct NMI_WFI_packet *pkt;
     
         while ((pkt = priv->ppool)) {
                 priv->ppool = pkt->next;
                 kfree (pkt);
                 /* FIXME - in-flight packets ? */
         }
 } 
   
/**
*  @brief 	NMI_WFI_GetTxBuffer
*  @details 	Buffer/pool management
*  @param[in] 	net_device *dev : Network Device Driver Structure
*  @return 	struct NMI_WFI_packet
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/ 
 struct NMI_WFI_packet *NMI_WFI_GetTxBuffer(struct net_device *dev)
 {
         struct NMI_WFI_priv *priv = netdev_priv(dev);
         unsigned long flags;
         struct NMI_WFI_packet *pkt;
     
         spin_lock_irqsave(&priv->lock, flags);
         pkt = priv->ppool;
         priv->ppool = pkt->next;
         if (priv->ppool == NULL) {
                 printk (KERN_INFO "Pool empty\n");
                 netif_stop_queue(dev);
         }
         spin_unlock_irqrestore(&priv->lock, flags);
         return pkt;
 }
/**
*  @brief 	NMI_WFI_ReleaseBuffer
*  @details 	Buffer/pool management
*  @param[in] 	NMI_WFI_packet *pkt : Structure holding in-flight packet
*  @return 	NONE
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/ 
void NMI_WFI_ReleaseBuffer(struct NMI_WFI_packet *pkt)
{
         unsigned long flags;
         struct NMI_WFI_priv *priv = netdev_priv(pkt->dev);
         
         spin_lock_irqsave(&priv->lock, flags);
         pkt->next = priv->ppool;
         priv->ppool = pkt;
         spin_unlock_irqrestore(&priv->lock, flags);
         if (netif_queue_stopped(pkt->dev) && pkt->next == NULL)
                 netif_wake_queue(pkt->dev);
}

/**
*  @brief 	NMI_WFI_EnqueueBuf
*  @details 	Enqueuing packets in an RX buffer queue
*  @param[in] 	NMI_WFI_packet *pkt : Structure holding in-flight packet
*  @param[in]   net_device *dev : Network Device Driver Structure
*  @return 	NONE
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
void NMI_WFI_EnqueueBuf(struct net_device *dev, struct NMI_WFI_packet *pkt)
{
         unsigned long flags;
         struct NMI_WFI_priv *priv = netdev_priv(dev);
 
         spin_lock_irqsave(&priv->lock, flags);
         pkt->next = priv->rx_queue;  /* FIXME - misorders packets */
         priv->rx_queue = pkt;
         spin_unlock_irqrestore(&priv->lock, flags);
 }

/**
*  @brief 	NMI_WFI_DequeueBuf
*  @details 	Dequeuing packets from the RX buffer queue
*  @param[in]   net_device *dev : Network Device Driver Structure
*  @return 	NMI_WFI_packet *pkt : Structure holding in-flight pac
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
struct NMI_WFI_packet *NMI_WFI_DequeueBuf(struct net_device *dev)
{
         struct NMI_WFI_priv *priv = netdev_priv(dev);
         struct NMI_WFI_packet *pkt;
         unsigned long flags;
 
         spin_lock_irqsave(&priv->lock, flags);
         pkt = priv->rx_queue;
         if (pkt != NULL)
                 priv->rx_queue = pkt->next;
         spin_unlock_irqrestore(&priv->lock, flags);
         return pkt;
 }
/**
*  @brief 	NMI_WFI_RxInts
*  @details 	Enable and disable receive interrupts.
*  @param[in]   net_device *dev : Network Device Driver Structure
*  @param[in]	enable : Enable/Disable flag
*  @return 	NONE
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static void NMI_WFI_RxInts(struct net_device *dev, int enable)
{
	struct NMI_WFI_priv *priv = netdev_priv(dev);
	priv->rx_int_enabled = enable;
}

/**
*  @brief 	NMI_WFI_Open
*  @details 	Open Network Device Driver, called when the network 
		interface is opened. It starts the interface's transmit queue.
*  @param[in]   net_device *dev : Network Device Driver Structure
*  @param[in]	enable : Enable/Disable flag
*  @return 	int : Returns 0 upon success. 
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/ 
int NMI_WFI_Open(struct net_device *dev)
{
	/* request_region(), request_irq(), ....  (like fops->open) */
	/* 
         * Assign the hardware address of the board: use "\0SNULx", where
         * x is 0 or 1. The first byte is '\0' to avoid being a multicast
         * address (the first byte of multicast addrs is odd).
         */
	memcpy(dev->dev_addr, "\0WLAN0", ETH_ALEN);
	if (dev == NMI_WFI_devs[1])
		dev->dev_addr[ETH_ALEN-1]++; /* \0SNUL1 */

	NMI_WFI_InitHostInt(dev);
	netif_start_queue(dev);
	return 0;
}
/**
*  @brief 	NMI_WFI_Release
*  @details 	Release Network Device Driver, called when the network 
		interface is stopped or brought down. This function marks
		the network driver as not being able to transmit 
*  @param[in]   net_device *dev : Network Device Driver Structure
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/ 
int NMI_WFI_Release(struct net_device *dev)
{
	/* release ports, irq and such -- like fops->close */
 
	netif_stop_queue(dev); /* can't transmit any more */
	
	return 0;
 }
/**
*  @brief 	NMI_WFI_Config
*  @details 	Configuration changes (passed on by ifconfig)
*  @param[in]   net_device *dev : Network Device Driver Structure
*  @param[in]	struct ifmap *map : Contains the ioctl implementation for the 
		network driver.
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/ 
int NMI_WFI_Config(struct net_device *dev, struct ifmap *map)
{
	if (dev->flags & IFF_UP) /* can't act on a running interface */
		return -EBUSY;
 
	/* Don't allow changing the I/O address */
	if (map->base_addr != dev->base_addr) {
		printk(KERN_WARNING "NMI_WFI: Can't change I/O address\n");
		return -EOPNOTSUPP;
	}
 
	/* Allow changing the IRQ */
	if (map->irq != dev->irq) {
		dev->irq = map->irq;
		/* request_irq() is delayed to open-time */
	}
 
         /* ignore other fields */
         return 0;
}
/**
*  @brief 	NMI_WFI_Rx
*  @details 	Receive a packet: retrieve, encapsulate and pass over to upper 
		levels
*  @param[in]   net_device *dev : Network Device Driver Structure
*  @param[in]	NMI_WFI_packet : 
*  @return 	NONE
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/ 
void NMI_WFI_Rx(struct net_device *dev, struct NMI_WFI_packet *pkt)
{
		int i;
         struct sk_buff *skb;
         struct NMI_WFI_priv *priv = netdev_priv(dev);
         s8 rssi;
         /*
          * The packet has been retrieved from the transmission
          * medium. Build an skb around it, so upper layers can handle it
          */


         skb = dev_alloc_skb(pkt->datalen + 2);
         if (!skb) {
                 if (printk_ratelimit())
                         printk(KERN_NOTICE "NMI_WFI rx: low on mem - packet dropped\n");
                 priv->stats.rx_dropped++;
                 goto out;
         }
         skb_reserve(skb, 2); /* align IP on 16B boundary */  
         memcpy(skb_put(skb, pkt->datalen), pkt->data, pkt->datalen);

         if(priv->monitor_flag)
         {
        	 PRINT_INFO(RX_DBG,"In monitor device name %s\n", dev->name);
        	 priv = wiphy_priv(priv->dev->ieee80211_ptr->wiphy);
        	 printk("VALUE PASSED IN OF HRWD %p\n", priv->hNMIWFIDrv);
        	// host_int_get_rssi(priv->hNMIWFIDrv, &(rssi));
        	// NMI_PRINTF("RSSI value is %d\n", rssi);
        	if(INFO)
        	{
        	 for (i=14 ; i<skb->len; i++)
        		 PRINT_INFO(RX_DBG,"RXdata[%d] %02x\n",i,skb->data[i]);
        	}
        	 NMI_WFI_monitor_rx(dev, skb);
        	 return;
         }
#if 0
         NMI_PRINTF("In RX NORMAl Device name %s\n", dev->name);
         /* Write metadata, and then pass to the receive level */
         skb->dev = dev;
         skb->protocol = eth_type_trans(skb, dev);
         skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
         NMI_WFI_update_stats(priv->dev->ieee80211_ptr->wiphy ,pkt->datalen,NMI_WFI_RX_PKT);
         netif_rx(skb);
#endif
out:
	return;
}
     
/**
*  @brief 	NMI_WFI_Poll
*  @details 	The poll implementation
*  @param[in]   struct napi_struct *napi : 
*  @param[in]	int budget :
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/  
static int NMI_WFI_Poll(struct napi_struct *napi, int budget)
{
         int npackets = 0;
         struct sk_buff *skb;
         struct NMI_WFI_priv  *priv = container_of(napi, struct NMI_WFI_priv, napi);
         struct net_device  *dev =  priv->dev;
         struct NMI_WFI_packet *pkt;
     
         while (npackets < budget && priv->rx_queue) {
                 pkt = NMI_WFI_DequeueBuf(dev);
                 skb = dev_alloc_skb(pkt->datalen + 2);
                 if (! skb) {
                         if (printk_ratelimit())
                                 printk(KERN_NOTICE "NMI_WFI: packet dropped\n");
                         priv->stats.rx_dropped++;
                         NMI_WFI_ReleaseBuffer(pkt);
                         continue;
                 }
                 skb_reserve(skb, 2); /* align IP on 16B boundary */  
                 memcpy(skb_put(skb, pkt->datalen), pkt->data, pkt->datalen);
                 skb->dev = dev;
                 skb->protocol = eth_type_trans(skb, dev);
                 skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
                 netif_receive_skb(skb);
                 /* Maintain stats */
                 npackets++;
                 NMI_WFI_update_stats(priv->dev->ieee80211_ptr->wiphy,pkt->datalen,NMI_WFI_RX_PKT);
                 NMI_WFI_ReleaseBuffer(pkt);
         }
         /* If we processed all packets, we're done; tell the kernel and re-enable ints */
         if (npackets < budget) {
                 napi_complete(napi);
                 NMI_WFI_RxInts(dev, 1);
         }
         return npackets;
 }
             
/**
*  @brief 	NMI_WFI_Poll
*  @details 	The typical interrupt entry point
*  @param[in]   struct napi_struct *napi : 
*  @param[in]	int budget :
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/         
static void NMI_WFI_RegularInterrupt(int irq, void *dev_id, struct pt_regs *regs)
{
         int statusword;
         struct NMI_WFI_priv *priv;
         struct NMI_WFI_packet *pkt = NULL;
         /*
          * As usual, check the "device" pointer to be sure it is
          * really interrupting.
          * Then assign "struct device *dev"
          */
         struct net_device *dev = (struct net_device *)dev_id;
         /* ... and check with hw if it's really ours */
 
         /* paranoid */
         if (!dev)
                 return;
 
         /* Lock the device */
         priv = netdev_priv(dev);
         spin_lock(&priv->lock);
 
         /* retrieve statusword: real netdevices use I/O instructions */
         statusword = priv->status;
         priv->status = 0;
         if (statusword & NMI_WFI_RX_INTR) {
                 /* send it to NMI_WFI_rx for handling */
                 pkt = priv->rx_queue;
                 if (pkt) {
                         priv->rx_queue = pkt->next;
                         NMI_WFI_Rx(dev, pkt);
                 }
         }
         if (statusword & NMI_WFI_TX_INTR) {
                 /* a transmission is over: free the skb */
        	 NMI_WFI_update_stats(priv->dev->ieee80211_ptr->wiphy,priv->tx_packetlen,NMI_WFI_TX_PKT);
                 dev_kfree_skb(priv->skb);
         }
 
         /* Unlock the device and we are done */
         spin_unlock(&priv->lock);
         if (pkt) NMI_WFI_ReleaseBuffer(pkt); /* Do this outside the lock! */
         return;
}
/**
*  @brief 	NMI_WFI_NapiInterrupt
*  @details 	A NAPI interrupt handler
*  @param[in]   irq: 
*  @param[in]	dev_id:
*  @param[in]	pt_regs: 
*  @return 	NONE
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/  
static void NMI_WFI_NapiInterrupt(int irq, void *dev_id, struct pt_regs *regs)
{
         int statusword;
         struct NMI_WFI_priv *priv;
 
         /*
          * As usual, check the "device" pointer for shared handlers.
          * Then assign "struct device *dev"
          */
         struct net_device *dev = (struct net_device *)dev_id;
         /* ... and check with hw if it's really ours */
 
         /* paranoid */
         if (!dev)
                 return;
 
         /* Lock the device */
         priv = netdev_priv(dev);
         spin_lock(&priv->lock);
 
         /* retrieve statusword: real netdevices use I/O instructions */
         statusword = priv->status;
         priv->status = 0;
         if (statusword & NMI_WFI_RX_INTR) {
                 NMI_WFI_RxInts(dev, 0);  /* Disable further interrupts */
                 napi_schedule(&priv->napi);
         }
         if (statusword & NMI_WFI_TX_INTR) {
                 /* a transmission is over: free the skb */

        	 NMI_WFI_update_stats(priv->dev->ieee80211_ptr->wiphy,priv->tx_packetlen,NMI_WFI_TX_PKT);
                 dev_kfree_skb(priv->skb);
         }
 
         /* Unlock the device and we are done */
         spin_unlock(&priv->lock);
         return;
}
 
/**
*  @brief 	MI_WFI_HwTx
*  @details 	Transmit a packet (low level interface)
*  @param[in]   buf: 
*  @param[in]	len:
*  @param[in]	net_device *dev: 
*  @return 	NONE
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/   
 void NMI_WFI_HwTx(char *buf, int len, struct net_device *dev)
{
         /*
          * This function deals with hw details. This interface loops
          * back the packet to the other NMI_WFI interface (if any).
          * In other words, this function implements the NMI_WFI behaviour,
          * while all other procedures are rather device-independent
          */
         struct iphdr *ih;
         struct net_device *dest;
         struct NMI_WFI_priv *priv;
         u32 *saddr, *daddr;
         struct NMI_WFI_packet *tx_buffer;
     	 

         /* I am paranoid. Ain't I? */
         if (len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
                 printk("NMI_WFI: Hmm... packet too short (%i octets)\n",
                                 len);
                 return;
         }
 
         if (0) { /* enable this conditional to look at the data */
                 int i;
                 printk("len is %i",len);
                 for (i=14 ; i<len; i++)
                         printk("TXdata[%d] %02x\n",i,buf[i]&0xff);
              //   printk("\n");
         }
         /*
          * Ethhdr is 14 bytes, but the kernel arranges for iphdr
          * to be aligned (i.e., ethhdr is unaligned)
          */
         ih = (struct iphdr *)(buf+sizeof(struct ethhdr));
         saddr = &ih->saddr;
         daddr = &ih->daddr;
 
         ((u8 *)saddr)[2] ^= 1; /* change the third octet (class C) */
         ((u8 *)daddr)[2] ^= 1;
 
         ih->check = 0;         /* and rebuild the checksum (ip needs it) */
         ih->check = ip_fast_csum((unsigned char *)ih,ih->ihl);
 

         if (dev == NMI_WFI_devs[0])
                 PDEBUGG("%08x:%05i --> %08x:%05i\n",
                                 ntohl(ih->saddr),ntohs(((struct tcphdr *)(ih+1))->source),
                                 ntohl(ih->daddr),ntohs(((struct tcphdr *)(ih+1))->dest));
         else
                 PDEBUGG("%08x:%05i <-- %08x:%05i\n",
                                 ntohl(ih->daddr),ntohs(((struct tcphdr *)(ih+1))->dest),
                                 ntohl(ih->saddr),ntohs(((struct tcphdr *)(ih+1))->source));
 
         /*
          * Ok, now the packet is ready for transmission: first simulate a
          * receive interrupt on the twin device, then  a
          * transmission-done on the transmitting device
          */
         dest = NMI_WFI_devs[dev == NMI_WFI_devs[0] ? 1 : 0];
         priv = netdev_priv(dest);

         tx_buffer = NMI_WFI_GetTxBuffer(dev);
         tx_buffer->datalen = len;
         memcpy(tx_buffer->data, buf, len);
         NMI_WFI_EnqueueBuf(dest, tx_buffer);
         if (priv->rx_int_enabled) {
                 priv->status |= NMI_WFI_RX_INTR;
                 NMI_WFI_Interrupt(0, dest, NULL);
         }
 
         priv = netdev_priv(dev);
         priv->tx_packetlen = len;
         priv->tx_packetdata = buf;
         priv->status |= NMI_WFI_TX_INTR;
         if (lockup && ((priv->stats.tx_packets + 1) % lockup) == 0) {
                 /* Simulate a dropped transmit interrupt */
                 netif_stop_queue(dev);
                 PDEBUG("Simulate lockup at %ld, txp %ld\n", jiffies,
                                 (unsigned long) priv->stats.tx_packets);
         }
         else
                 NMI_WFI_Interrupt(0, dev, NULL);

}

/**
*  @brief 	NMI_WFI_Tx
*  @details 	Transmit a packet (called by the kernel)
*  @param[in]   sk_buff *skb: 
*  @param[in]	net_device *dev: 
*  @return 	NONE
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/ 
int NMI_WFI_Tx(struct sk_buff *skb, struct net_device *dev)
{
         int len;
         char *data, shortpkt[ETH_ZLEN];
         struct NMI_WFI_priv *priv = netdev_priv(dev);
         
        // priv = wiphy_priv(priv->dev->ieee80211_ptr->wiphy);

      //  if(priv->monitor_flag)
        //	 mac80211_hwsim_monitor_rx(skb);


         data = skb->data;
         len = skb->len;

         if (len < ETH_ZLEN) {
                 memset(shortpkt, 0, ETH_ZLEN);
                 memcpy(shortpkt, skb->data, skb->len);
                 len = ETH_ZLEN;
                 data = shortpkt;
         }
         dev->trans_start = jiffies; /* save the timestamp */
 
         /* Remember the skb, so we can free it at interrupt time */
         priv->skb = skb;
 
         /* actual deliver of data is device-specific, and not shown here */
         NMI_WFI_HwTx(data, len, dev);
 
         return 0; /* Our simple device can not fail */
 }

/**
*  @brief 	NMI_WFI_TxTimeout
*  @details 	Deal with a transmit timeout.
*  @param[in]	net_device *dev: 
*  @return 	NONE
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/  
void NMI_WFI_TxTimeout(struct net_device *dev)
{
         struct NMI_WFI_priv *priv = netdev_priv(dev);
 
         PDEBUG("Transmit timeout at %ld, latency %ld\n", jiffies,
                         jiffies - dev->trans_start);
         /* Simulate a transmission interrupt to get things moving */
         priv->status = NMI_WFI_TX_INTR;
         NMI_WFI_Interrupt(0, dev, NULL);
         priv->stats.tx_errors++;
         netif_wake_queue(dev);
         return;
}
 
/**
*  @brief 	NMI_WFI_Ioctl
*  @details 	Ioctl commands 
*  @param[in]	net_device *dev:
*  @param[in]	ifreq *rq
*  @param[in]	cmd: 
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/ 
int NMI_WFI_Ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
         PDEBUG("ioctl\n");
         return 0;
}

/**
*  @brief 	NMI_WFI_Stat
*  @details 	Return statistics to the caller
*  @param[in]	net_device *dev:
*  @return 	NMI_WFI_Stats : Return net_device_stats stucture with the
		network device driver private data contents.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/  
struct net_device_stats *NMI_WFI_Stats(struct net_device *dev)
{
         struct NMI_WFI_priv *priv = netdev_priv(dev);
         return &priv->stats;
}

/**
*  @brief 	NMI_WFI_RebuildHeader
*  @details 	This function is called to fill up an eth header, since arp is not
* 		available on the interface
*  @param[in]	sk_buff *skb: 
*  @return 	int : Return 0 on Success 
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
int NMI_WFI_RebuildHeader(struct sk_buff *skb)
{
         struct ethhdr *eth = (struct ethhdr *) skb->data;
         struct net_device *dev = skb->dev;
     
         memcpy(eth->h_source, dev->dev_addr, dev->addr_len);
         memcpy(eth->h_dest, dev->dev_addr, dev->addr_len);
         eth->h_dest[ETH_ALEN-1]   ^= 0x01;   /* dest is us xor 1 */
         return 0;
}
/**
*  @brief 	NMI_WFI_RebuildHeader
*  @details 	This function is called to fill up an eth header, since arp is not
* 		available on the interface
*  @param[in]	sk_buff *skb: 
*  @param[in]	struct net_device *dev:
*  @param[in]   unsigned short type:  
*  @param[in]   const void *saddr,
*  @param[in]   const void *daddr:
*  @param[in] 		unsigned int len              
*  @return 	int : Return 0 on Success 
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/  
int NMI_WFI_Header(struct sk_buff *skb, struct net_device *dev,
                  unsigned short type, const void *daddr, const void *saddr,
                 unsigned int len)
{
         struct ethhdr *eth = (struct ethhdr *)skb_push(skb,ETH_HLEN);
 
         eth->h_proto = htons(type);
         memcpy(eth->h_source, saddr ? saddr : dev->dev_addr, dev->addr_len);
         memcpy(eth->h_dest,   daddr ? daddr : dev->dev_addr, dev->addr_len);
         eth->h_dest[ETH_ALEN-1]   ^= 0x01;   /* dest is us xor 1 */
         return (dev->hard_header_len);
}
 
/**
*  @brief 	NMI_WFI_ChangeMtu
*  @details 	The "change_mtu" method is usually not needed.
* 		If you need it, it must be like this.
*  @param[in]	net_device *dev : Network Device Driver Structure
*  @param[in]	new_mtu : 
*  @return 	int : Returns 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
int NMI_WFI_ChangeMtu(struct net_device *dev, int new_mtu)
{
         unsigned long flags;
         struct NMI_WFI_priv *priv = netdev_priv(dev);
         spinlock_t *lock = &priv->lock;
     
         /* check ranges */
         if ((new_mtu < 68) || (new_mtu > 1500))
                 return -EINVAL;
         /*
          * Do anything you need, and the accept the value
          */
         spin_lock_irqsave(lock, flags);
         dev->mtu = new_mtu;
         spin_unlock_irqrestore(lock, flags);
         return 0; /* success */
}
 
static const struct header_ops NMI_WFI_header_ops = {
         .create  = NMI_WFI_Header,
         .rebuild = NMI_WFI_RebuildHeader,
         .cache   = NULL,  /* disable caching */
};
 

static const struct net_device_ops NMI_WFI_netdev_ops = {
         .ndo_open = NMI_WFI_Open,
         .ndo_stop = NMI_WFI_Release,
         .ndo_set_config = NMI_WFI_Config,
         .ndo_start_xmit = NMI_WFI_Tx,
         .ndo_do_ioctl = NMI_WFI_Ioctl,
         .ndo_get_stats = NMI_WFI_Stats,
         .ndo_change_mtu = NMI_WFI_ChangeMtu,
         .ndo_tx_timeout = NMI_WFI_TxTimeout,
 };

/**
*  @brief 	NMI_WFI_Init
*  @details 	The init function (sometimes called probe).
* 		It is invoked by register_netdev()
*  @param[in]	net_device *dev:
*  @return 	NONE
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/  
void NMI_WFI_Init(struct net_device *dev)
{
         struct NMI_WFI_priv *priv;
	

         /* 
          * Then, assign other fields in dev, using ether_setup() and some
          * hand assignments
          */
         ether_setup(dev); /* assign some of the fields */
 //1- Allocate space
	
         dev->netdev_ops      = &NMI_WFI_netdev_ops;
         dev->header_ops      = &NMI_WFI_header_ops;
         dev->watchdog_timeo = timeout;
         /* keep the default flags, just add NOARP */
         dev->flags           |= IFF_NOARP;
         dev->features        |= NETIF_F_NO_CSUM;
         /*
          * Then, initialize the priv field. This encloses the statistics
          * and a few private fields.
          */
         priv = netdev_priv(dev);
         memset(priv, 0, sizeof(struct NMI_WFI_priv));
         priv->dev = dev;
         netif_napi_add(dev, &priv->napi, NMI_WFI_Poll, 2);
         /* The last parameter above is the NAPI "weight". */
         spin_lock_init(&priv->lock);
         NMI_WFI_RxInts(dev, 1);          /* enable receive interrupts */
         NMI_WFI_SetupPool(dev);
 }

/**
*  @brief 	NMI_WFI_Stat
*  @details 	Return statistics to the caller
*  @param[in]	net_device *dev:
*  @return 	NMI_WFI_Stats : Return net_device_stats stucture with the
		network device driver private data contents.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/ 

void NMI_WFI_Cleanup(void)
{
	int i;
	struct NMI_WFI_priv *priv[2];

 	/*if(hwsim_mon!=NULL)
 	{
 		printk("Freeing monitor interface\n");
 		unregister_netdev(hwsim_mon);
 		free_netdev(hwsim_mon);
 	}*/
        for (i = 0; i < 2;i++)
	    {
		priv[i] = netdev_priv(NMI_WFI_devs[i]);

		if (NMI_WFI_devs[i]) 
		{
			NMI_PRINTF("Unregistering\n");
            unregister_netdev(NMI_WFI_devs[i]);
            NMI_WFI_TearDownPool(NMI_WFI_devs[i]);
            free_netdev(NMI_WFI_devs[i]);
			NMI_PRINTF("[NETDEV]Stopping interface\n");
			NMI_WFI_DeInitHostInt(NMI_WFI_devs[i]);
            NMI_WFI_WiphyFree(NMI_WFI_devs[i]);
          }
		
      }
         //unregister_netdev(hwsim_mon);
         NMI_WFI_deinit_mon_interface();
         return;
}
 
 
void StartConfigSim(void);







/**
*  @brief 	NMI_WFI_Stat
*  @details 	Return statistics to the caller
*  @param[in]	net_device *dev:
*  @return 	NMI_WFI_Stats : Return net_device_stats stucture with the
		network device driver private data contents.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/ 
int NMI_WFI_InitModule(void)
{
	
	int result, i, ret = -ENOMEM;
	struct NMI_WFI_priv *priv[2],*netpriv;
	struct wireless_dev *wdev;
    NMI_WFI_Interrupt = use_napi ? NMI_WFI_NapiInterrupt : NMI_WFI_RegularInterrupt;
    char buf[IFNAMSIZ];

	for (i = 0; i < 2;  i++)
	{
		
		/* Allocate the net devices */
		NMI_WFI_devs[i] = alloc_netdev(sizeof(struct NMI_WFI_priv), "wlan%d",
                         NMI_WFI_Init);
		if(NMI_WFI_devs[i] == NULL)
			goto out;
		//priv[i] = netdev_priv(NMI_WFI_devs[i]);
		
		wdev = NMI_WFI_WiphyRegister(NMI_WFI_devs[i]);
		NMI_WFI_devs[i]->ieee80211_ptr = wdev;
		netpriv = netdev_priv(NMI_WFI_devs[i]);
		netpriv->dev->ieee80211_ptr = wdev;
		netpriv->dev->ml_priv = netpriv;
		wdev->netdev = netpriv->dev;
		
		/*Registering the net device*/
        if ((result = register_netdev(NMI_WFI_devs[i])))
        	printk("NMI_WFI: error %i registering device \"%s\"\n",
        			result, NMI_WFI_devs[i]->name);
        else
        	ret = 0;
	}
	

	/*init NMi driver */
	priv[0] = netdev_priv(NMI_WFI_devs[0]);
	priv[1] = netdev_priv(NMI_WFI_devs[1]);
	//printk("Net dev handler in int %lu\n",&priv[0]->hNMIWFIDrv);

	if(priv[1]->dev->ieee80211_ptr->wiphy->interface_modes && BIT(NL80211_IFTYPE_MONITOR) )
	{
		//snprintf(buf, IFNAMSIZ, "mon.%s",  priv[1]->dev->name);
	//	printk("Initializing mon interface %s\n", buf);
	//	NMI_WFI_init_mon_interface();
	//	priv[1]->monitor_flag = 1;

	}
	priv[0]->bCfgScanning = NMI_FALSE;
	priv[0]->u32RcvdChCount = 0;

	NMI_memset(priv[0]->au8AssociatedBss, 0xFF, ETH_ALEN);


	//ret = host_int_init(&priv[0]->hNMIWFIDrv);
	/*copy handle to the other driver*/
	//priv[1]->hNMIWFIDrv = priv[0]->hNMIWFIDrv;
	if(ret)
	{
		NMI_PRINTF("Error Init Driver\n");
	}

	
    out:
         if (ret) 
                 NMI_WFI_Cleanup();
         return ret;


}
 
 
module_init(NMI_WFI_InitModule);
module_exit(NMI_WFI_Cleanup);
 
#endif
