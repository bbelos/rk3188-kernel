/*!  
*  @file	NMI_WFI_CfgOperations.h
*  @brief	Definitions for the network module
*  @author	syounan
*  @sa		NMI_OSWrapper.h top level OS wrapper file
*  @date	31 Aug 2010
*  @version	1.0
*/
#ifndef NM_WFI_CFGOPERATIONS
#define NM_WFI_CFGOPERATIONS
#include "linux/include/NMI_WFI_NetDevice.h"

#ifdef NMI_FULLY_HOSTING_AP
#include "NMI_host_AP.h"
#endif


/* The following macros describe the bitfield map used by the firmware to determine its 11i mode */
#define NO_ENCRYPT			0
#define ENCRYPT_ENABLED	(1 << 0)
#define WEP					(1 << 1)
#define WEP_EXTENDED		(1 << 2)
#define WPA					(1 << 3)
#define WPA2				(1 << 4)
#define AES					(1 << 5)
#define TKIP					(1 << 6)

#ifdef NMI_P2P
#define 	GO_INTENT_ATTR_ID       			0x04
#define 	CHANNEL_LIST_ATTR_ID  		0x0b
#define 	GO_NEG_REQ_ATTR_ID     		0x00
#define 	GO_NEG_RSP_ATTR_ID     		0x01
#define 	ACTION_FRAME       				0xd0
#define 	PUBLICACTION_CAT   			0x04
#define 	PUBLICACTION_FRAME 			0x09
#define 	DEFAULT_CHANNEL    			  7
#define   P2PELEM_ID          				0xdd
#define 	GO_INTENT_ATTR_ID        		0x04
#define 	CHANLIST_ATTR_ID                      	0x0b
#define 	OPERCHAN_ATTR_ID                      	0x11
#endif

#define nl80211_SCAN_RESULT_EXPIRE	(3 * HZ)
#define SCAN_RESULT_EXPIRE				(40 * HZ)

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
static const u32 cipher_suites[] = {
	WLAN_CIPHER_SUITE_WEP40,
	WLAN_CIPHER_SUITE_WEP104,
	WLAN_CIPHER_SUITE_TKIP,
	WLAN_CIPHER_SUITE_CCMP,
	WLAN_CIPHER_SUITE_AES_CMAC,
};
#endif



#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
static const struct ieee80211_txrx_stypes 
nmi_wfi_cfg80211_mgmt_types[NL80211_IFTYPE_MAX] = 
{
	[NL80211_IFTYPE_STATION] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
	},
	[NL80211_IFTYPE_AP] = {
	.tx = 0xffff,
	.rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		BIT(IEEE80211_STYPE_DISASSOC >> 4) |
		BIT(IEEE80211_STYPE_AUTH >> 4) |
		BIT(IEEE80211_STYPE_DEAUTH >> 4) |
		BIT(IEEE80211_STYPE_ACTION >> 4)
	},
	[NL80211_IFTYPE_P2P_CLIENT] = {
		.tx = 0xffff,
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
		BIT(IEEE80211_STYPE_DISASSOC >> 4) |
		BIT(IEEE80211_STYPE_AUTH >> 4) |
		BIT(IEEE80211_STYPE_DEAUTH >> 4) 
	}
};
#endif
/* Time to stay on the channel */
#define NMI_WFI_DWELL_PASSIVE 100
#define NMI_WFI_DWELL_ACTIVE  40

struct wireless_dev* NMI_WFI_CfgAlloc(void);
struct wireless_dev * NMI_WFI_WiphyRegister(struct net_device *net);
void NMI_WFI_WiphyFree(struct net_device *net);
int NMI_WFI_update_stats(struct wiphy *wiphy, u32 pktlen , u8 changed);
int NMI_WFI_DeInitHostInt(struct net_device *net);
int NMI_WFI_InitHostInt(struct net_device *net);
void NMI_WFI_monitor_rx(uint8_t *buff, uint32_t size);
int NMI_WFI_deinit_mon_interface(void);
struct net_device * NMI_WFI_init_mon_interface(char *name, struct net_device *real_dev );
#endif
