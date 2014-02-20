/*!  
*  @file	NMI_WFI_CfgOpertaions.c
*  @brief	CFG80211 Function Implementation functionality
*  @author	aabouzaeid
*  			mabubakr
*  			mdaftedar
*  			zsalah
*  @sa		NMI_WFI_CfgOpertaions.h top level OS wrapper file
*  @date	31 Aug 2010
*  @version	1.0
*/

#include "linux/include/NMI_WFI_CfgOperations.h"

#include "linux_wlan_sdio.h"    //tony

NMI_SemaphoreHandle SemHandleUpdateStats;
static NMI_SemaphoreHandle hSemScanReq;
static NMI_Bool gbAutoRateAdjusted = NMI_FALSE;

#define IS_MANAGMEMENT 				0x100
#define IS_MANAGMEMENT_CALLBACK 		0x080
#define IS_MGMT_STATUS_SUCCES			0x040
#define GET_PKT_OFFSET(a) (((a) >> 22) & 0x1ff)

extern void linux_wlan_free(void* vp);
extern int linux_wlan_get_firmware(linux_wlan_t* p_nic);
#ifdef NMI_P2P
extern tstrNMI_WFIDrv* gWFiDrvHandle;
#endif

extern int mac_open(struct net_device *ndev);
extern int mac_close(struct net_device *ndev);


#define CHAN2G(_channel, _freq, _flags) {        \
	.band             = IEEE80211_BAND_2GHZ, \
	.center_freq      = (_freq),             \
	.hw_value         = (_channel),          \
	.flags            = (_flags),            \
	.max_antenna_gain = 0,                   \
	.max_power        = 30,                  \
}

/*Frequency range for channels*/
static struct ieee80211_channel NMI_WFI_2ghz_channels[] = {
	CHAN2G(1,  2412, 0),
	CHAN2G(2,  2417, 0),
	CHAN2G(3,  2422, 0),
	CHAN2G(4,  2427, 0),
	CHAN2G(5,  2432, 0),
	CHAN2G(6,  2437, 0),
	CHAN2G(7,  2442, 0),
	CHAN2G(8,  2447, 0),
	CHAN2G(9,  2452, 0),
	CHAN2G(10, 2457, 0),
	CHAN2G(11, 2462, 0),
	CHAN2G(12, 2467, 0),
	CHAN2G(13, 2472, 0),
	CHAN2G(14, 2484, 0),
};

#define RATETAB_ENT(_rate, _hw_value, _flags) { \
	.bitrate  = (_rate),                    \
	.hw_value = (_hw_value),                \
	.flags    = (_flags),                   \
}


/* Table 6 in section 3.2.1.1 */
static struct ieee80211_rate NMI_WFI_rates[] = {
	RATETAB_ENT(10,  0,  0),
	RATETAB_ENT(20,  1,  0),
	RATETAB_ENT(55,  2,  0),
	RATETAB_ENT(110, 3,  0),
	RATETAB_ENT(60,  9,  0),
	RATETAB_ENT(90,  6,  0),
	RATETAB_ENT(120, 7,  0),
	RATETAB_ENT(180, 8,  0),
	RATETAB_ENT(240, 9,  0),
	RATETAB_ENT(360, 10, 0),
	RATETAB_ENT(480, 11, 0),
	RATETAB_ENT(540, 12, 0),
};

#ifdef NMI_P2P
struct p2p_mgmt_data{
	int size;
	void* buff;
};
#endif

static struct ieee80211_supported_band NMI_WFI_band_2ghz = {
	.channels = NMI_WFI_2ghz_channels,
	.n_channels = ARRAY_SIZE(NMI_WFI_2ghz_channels),
	.bitrates = NMI_WFI_rates,
	.n_bitrates = ARRAY_SIZE(NMI_WFI_rates),
};


static NMI_TimerHandle hAgingTimer;
#define AGING_TIME	9*1000

void clear_shadow_scan(void* pUserVoid){
 	struct NMI_WFI_priv* priv;
	int i;
 	priv = (struct NMI_WFI_priv*)pUserVoid;

	for(i = 0; i < priv->u32LastScannedNtwrksCountShadow; i++){
		if(priv->astrLastScannedNtwrksShadow[priv->u32LastScannedNtwrksCountShadow].pu8IEs != NULL)
			NMI_FREE(priv->astrLastScannedNtwrksShadow[i].pu8IEs);
	}
	priv->u32LastScannedNtwrksCountShadow = 0;

	NMI_TimerDestroy(&hAgingTimer,NMI_NULL);
}
void refresh_scan(void* pUserVoid,uint8_t all){
 	struct NMI_WFI_priv* priv;	
	struct wiphy* wiphy;
	struct cfg80211_bss* bss = NULL;
	int i;
	
 	priv = (struct NMI_WFI_priv*)pUserVoid;
	wiphy = priv->dev->ieee80211_ptr->wiphy;

	for(i = 0; i < priv->u32LastScannedNtwrksCountShadow; i++)
	{
		tstrNetworkInfo* pstrNetworkInfo;
		pstrNetworkInfo = &(priv->astrLastScannedNtwrksShadow[i]);
		
		if(!pstrNetworkInfo->u8Found || all){
			NMI_Sint32 s32Freq;
			struct ieee80211_channel *channel;

			if(pstrNetworkInfo != NMI_NULL)
			{

				#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
					s32Freq = ieee80211_channel_to_frequency((NMI_Sint32)pstrNetworkInfo->u8channel, IEEE80211_BAND_2GHZ);
				#else
					s32Freq = ieee80211_channel_to_frequency((NMI_Sint32)pstrNetworkInfo->u8channel);
				#endif

				channel = ieee80211_get_channel(wiphy, s32Freq);


				bss = cfg80211_inform_bss(wiphy, channel, pstrNetworkInfo->au8bssid, 0, pstrNetworkInfo->u16CapInfo,
									pstrNetworkInfo->u16BeaconPeriod, (const u8*)pstrNetworkInfo->pu8IEs,
									(size_t)pstrNetworkInfo->u16IEsLen, (((NMI_Sint32)pstrNetworkInfo->s8rssi) * 100), GFP_KERNEL);
				cfg80211_put_bss(bss);
			}
			
		}				
	}
}

void reset_shadow_found(void* pUserVoid){
 	struct NMI_WFI_priv* priv;	
	int i;
 	priv = (struct NMI_WFI_priv*)pUserVoid;
	for(i=0;i<priv->u32LastScannedNtwrksCountShadow;i++){
		priv->astrLastScannedNtwrksShadow[i].u8Found = 0;
		}
}

void update_scan_time(void* pUserVoid){
	struct NMI_WFI_priv* priv;	
	int i;
 	priv = (struct NMI_WFI_priv*)pUserVoid;
	for(i=0;i<priv->u32LastScannedNtwrksCountShadow;i++){
		priv->astrLastScannedNtwrksShadow[i].u32TimeRcvdInScan = jiffies;
		}
}

void remove_network_from_shadow(void* pUserVoid){
 	struct NMI_WFI_priv* priv;	
	unsigned long now = jiffies;
	int i,j;

 	priv = (struct NMI_WFI_priv*)pUserVoid;
	
	for(i=0;i<priv->u32LastScannedNtwrksCountShadow;i++){
		if(time_after(now, priv->astrLastScannedNtwrksShadow[i].u32TimeRcvdInScan + (unsigned long)(SCAN_RESULT_EXPIRE))){
			PRINT_D(CFG80211_DBG,"Network expired in ScanShadow: %s \n",priv->astrLastScannedNtwrksShadow[i].au8ssid);

			if(priv->astrLastScannedNtwrksShadow[i].pu8IEs != NULL)
				NMI_FREE(priv->astrLastScannedNtwrksShadow[i].pu8IEs);
			
			host_int_freeJoinParams(priv->astrLastScannedNtwrksShadow[i].pJoinParams);
			
			for(j=i;(j<priv->u32LastScannedNtwrksCountShadow-1);j++){
				priv->astrLastScannedNtwrksShadow[j] = priv->astrLastScannedNtwrksShadow[j+1];
			}
			priv->u32LastScannedNtwrksCountShadow--;
		}
	}

	PRINT_D(CFG80211_DBG,"Number of cached networks: %d\n",priv->u32LastScannedNtwrksCountShadow);
	if(priv->u32LastScannedNtwrksCountShadow != 0)
		NMI_TimerStart(&hAgingTimer, AGING_TIME, pUserVoid, NMI_NULL);
	else
		PRINT_D(CFG80211_DBG,"No need to restart Aging timer\n");
}

int8_t is_network_in_shadow(tstrNetworkInfo* pstrNetworkInfo,void* pUserVoid){
 	struct NMI_WFI_priv* priv;	
	int8_t state = -1;
	int i;

 	priv = (struct NMI_WFI_priv*)pUserVoid;
	if(priv->u32LastScannedNtwrksCountShadow== 0){
		PRINT_D(CFG80211_DBG,"Starting Aging timer\n");
		NMI_TimerStart(&hAgingTimer, AGING_TIME, pUserVoid, NMI_NULL);
		state = -1;
	}else{
		/* Linear search for now */
		for(i=0;i<priv->u32LastScannedNtwrksCountShadow;i++){
			if(NMI_memcmp(priv->astrLastScannedNtwrksShadow[i].au8bssid,
				  pstrNetworkInfo->au8bssid, 6) == 0){
								  state = i;
								  break;
			}
		}
	}
	return state;
}

void add_network_to_shadow(tstrNetworkInfo* pstrNetworkInfo,void* pUserVoid, void* pJoinParams){
 	struct NMI_WFI_priv* priv;	
	int8_t ap_found = is_network_in_shadow(pstrNetworkInfo,pUserVoid);
	uint32_t ap_index = 0;
 	priv = (struct NMI_WFI_priv*)pUserVoid;

	if(priv->u32LastScannedNtwrksCountShadow >= MAX_NUM_SCANNED_NETWORKS_SHADOW){
		PRINT_D(CFG80211_DBG,"Shadow network reached its maximum limit\n");
		return;
	}

	if(ap_found == -1){
			ap_index = priv->u32LastScannedNtwrksCountShadow;
			priv->u32LastScannedNtwrksCountShadow++;
		}else{
			ap_index = ap_found;
			}

		priv->astrLastScannedNtwrksShadow[ap_index].s8rssi = pstrNetworkInfo->s8rssi;
		priv->astrLastScannedNtwrksShadow[ap_index].u16CapInfo = pstrNetworkInfo->u16CapInfo;
		
		priv->astrLastScannedNtwrksShadow[ap_index].u8SsidLen = pstrNetworkInfo->u8SsidLen;
		NMI_memcpy(priv->astrLastScannedNtwrksShadow[ap_index].au8ssid,
				  	  pstrNetworkInfo->au8ssid, pstrNetworkInfo->u8SsidLen);

		NMI_memcpy(priv->astrLastScannedNtwrksShadow[ap_index].au8bssid,
				  	  pstrNetworkInfo->au8bssid, ETH_ALEN);

		priv->astrLastScannedNtwrksShadow[ap_index].u16BeaconPeriod = pstrNetworkInfo->u16BeaconPeriod;
		priv->astrLastScannedNtwrksShadow[ap_index].u8DtimPeriod = pstrNetworkInfo->u8DtimPeriod;
		priv->astrLastScannedNtwrksShadow[ap_index].u8channel = pstrNetworkInfo->u8channel;
		
		priv->astrLastScannedNtwrksShadow[ap_index].u16IEsLen = pstrNetworkInfo->u16IEsLen;
		priv->astrLastScannedNtwrksShadow[ap_index].pu8IEs = 
			(NMI_Uint8*)NMI_MALLOC(pstrNetworkInfo->u16IEsLen); /* will be deallocated 
																   by the NMI_WFI_CfgScan() function */
		NMI_memcpy(priv->astrLastScannedNtwrksShadow[ap_index].pu8IEs,
				  	  pstrNetworkInfo->pu8IEs, pstrNetworkInfo->u16IEsLen);
						
		priv->astrLastScannedNtwrksShadow[ap_index].u32TimeRcvdInScan = jiffies;
		priv->astrLastScannedNtwrksShadow[ap_index].u32TimeRcvdInScanCached = jiffies;
		priv->astrLastScannedNtwrksShadow[ap_index].u8Found = 1;
		priv->astrLastScannedNtwrksShadow[ap_index].pJoinParams = pJoinParams;

}


/**
*  @brief 	CfgScanResult
*  @details  Callback function which returns the scan results found
*
*  @param[in] tenuScanEvent enuScanEvent: enum, indicating the scan event triggered, whether that is
*  			  SCAN_EVENT_NETWORK_FOUND or SCAN_EVENT_DONE
*  			  tstrNetworkInfo* pstrNetworkInfo: structure holding the scan results information
*  			  void* pUserVoid: Private structure associated with the wireless interface
*  @return 	NONE
*  @author	mabubakr
*  @date
*  @version	1.0
*/
static void CfgScanResult(tenuScanEvent enuScanEvent, tstrNetworkInfo* pstrNetworkInfo, void* pUserVoid, void* pJoinParams)
 {
 	struct NMI_WFI_priv* priv;
 	struct wiphy* wiphy;
 	NMI_Sint32 s32Freq;
 	struct ieee80211_channel *channel;
	NMI_Sint32 s32Error = NMI_SUCCESS;
	struct cfg80211_bss* bss = NULL;
	
 	priv = (struct NMI_WFI_priv*)pUserVoid;


 	if(priv->bCfgScanning == NMI_TRUE)
 	{


 		if(enuScanEvent == SCAN_EVENT_NETWORK_FOUND)
 		{

			wiphy = priv->dev->ieee80211_ptr->wiphy;

			NMI_NULLCHECK(s32Error, wiphy);

			if(wiphy->signal_type == CFG80211_SIGNAL_TYPE_UNSPEC 
				&&
				( (((NMI_Sint32)pstrNetworkInfo->s8rssi) * 100) < 0 
				||
				(((NMI_Sint32)pstrNetworkInfo->s8rssi) * 100) > 100)
				)
				{
					NMI_ERRORREPORT(s32Error, NMI_FAIL);
				}


			
			if(pstrNetworkInfo != NMI_NULL)
			{
				#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
				s32Freq = ieee80211_channel_to_frequency((NMI_Sint32)pstrNetworkInfo->u8channel, IEEE80211_BAND_2GHZ);
				#else
				s32Freq = ieee80211_channel_to_frequency((NMI_Sint32)pstrNetworkInfo->u8channel);
				#endif
				channel = ieee80211_get_channel(wiphy, s32Freq);

				NMI_NULLCHECK(s32Error, channel);

				PRINT_INFO(CFG80211_DBG,"Network Info:: CHANNEL Frequency: %d, RSSI: %d, CapabilityInfo: %d,"
						"BeaconPeriod: %d \n",channel->center_freq, (((NMI_Sint32)pstrNetworkInfo->s8rssi) * 100),
						pstrNetworkInfo->u16CapInfo, pstrNetworkInfo->u16BeaconPeriod);


				if(pstrNetworkInfo->bNewNetwork == NMI_TRUE)
				{
					if(priv->u32RcvdChCount < MAX_NUM_SCANNED_NETWORKS) //TODO: mostafa: to be replaced by
																     //               max_scan_ssids
					{
						PRINT_D(CFG80211_DBG,"Network %s found\n",pstrNetworkInfo->au8ssid);
						
						
							priv->u32RcvdChCount++;
							


							if(pJoinParams == NULL){
								printk(">> Something really bad happened\n");
							}
							add_network_to_shadow(pstrNetworkInfo,priv,pJoinParams);

							

							bss = cfg80211_inform_bss(wiphy, channel, pstrNetworkInfo->au8bssid, 0, pstrNetworkInfo->u16CapInfo,
											pstrNetworkInfo->u16BeaconPeriod, (const u8*)pstrNetworkInfo->pu8IEs,
											(size_t)pstrNetworkInfo->u16IEsLen, (((NMI_Sint32)pstrNetworkInfo->s8rssi) * 100), GFP_KERNEL);
							cfg80211_put_bss(bss);
							
						
						}
						else
						{
							PRINT_ER("Discovered networks exceeded the max limit\n");
						}
				}
				else
				{
					NMI_Uint32 i;
					/* So this network is discovered before, we'll just update its RSSI */
					for(i = 0; i < priv->u32RcvdChCount; i++)
					{
						if(NMI_memcmp(priv->astrLastScannedNtwrksShadow[i].au8bssid, pstrNetworkInfo->au8bssid, 6) == 0)
						{					
							PRINT_D(CFG80211_DBG,"Update RSSI of %s \n",priv->astrLastScannedNtwrksShadow[i].au8ssid);
							priv->astrLastScannedNtwrksShadow[i].s8rssi = pstrNetworkInfo->s8rssi;
							priv->astrLastScannedNtwrksShadow[i].u32TimeRcvdInScan = jiffies;
							break;							
						}
					}
				}
			}
 		}
 		else if(enuScanEvent == SCAN_EVENT_DONE)
 		{
 			PRINT_D(CFG80211_DBG,"Scan Done \n");

 			PRINT_D(CFG80211_DBG,"Refreshing Scan ... \n");
			refresh_scan(priv,0);

 			if(priv->u32RcvdChCount > 0)
 			{
 				PRINT_D(CFG80211_DBG,"%d Network(s) found \n", priv->u32RcvdChCount);
 			}
 			else
 			{
 				PRINT_D(CFG80211_DBG,"No networks found \n");
 			}

			NMI_SemaphoreAcquire(&hSemScanReq, NULL);

			if(priv->pstrScanReq != NMI_NULL)
   			{
 				cfg80211_scan_done(priv->pstrScanReq, NMI_FALSE);
				priv->u32RcvdChCount = 0;
				priv->bCfgScanning = NMI_FALSE;
				priv->pstrScanReq = NMI_NULL;
			}
			NMI_SemaphoreRelease(&hSemScanReq, NULL);

 		}
		/*Aborting any scan operation during mac close*/
		else if(enuScanEvent == SCAN_EVENT_ABORTED)
 		{
			NMI_SemaphoreAcquire(&hSemScanReq, NULL);
			
	 		PRINT_D(CFG80211_DBG,"Aborting scan \n");	
			if(priv->pstrScanReq != NMI_NULL)
   			{

				update_scan_time(priv);
				refresh_scan(priv,1);

				cfg80211_scan_done(priv->pstrScanReq,NMI_FALSE);
				priv->pstrScanReq = NMI_NULL;
  			}
			NMI_SemaphoreRelease(&hSemScanReq, NULL);
		}
 	}


	NMI_CATCH(s32Error)
	{
	}
 }


/**
*  @brief 	NMI_WFI_Set_PMKSA
*  @details  Check if pmksa is cached and set it.
*  @param[in]
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012
*  @version	1.0
*/
int NMI_WFI_Set_PMKSA(NMI_Uint8 * bssid,struct NMI_WFI_priv* priv)
{
	NMI_Uint32 i;
	NMI_Sint32 s32Error = NMI_SUCCESS;

	
	for (i = 0; i < priv->pmkid_list.numpmkid; i++)
		{

			if (!NMI_memcmp(bssid,priv->pmkid_list.pmkidlist[i].bssid,
						ETH_ALEN))
			{
				PRINT_D(CFG80211_DBG,"PMKID successful comparison");

				/*If bssid is found, set the values*/
				s32Error = host_int_set_pmkid_info(priv->hNMIWFIDrv,&priv->pmkid_list);

				if(s32Error != NMI_SUCCESS)
					PRINT_ER("Error in pmkid\n");

				break;
			}
		}

	return s32Error;


}


/**
*  @brief 	CfgConnectResult
*  @details
*  @param[in] tenuConnDisconnEvent enuConnDisconnEvent: Type of connection response either
*  			  connection response or disconnection notification.
*  			  tstrConnectInfo* pstrConnectInfo: COnnection information.
*  			  NMI_Uint8 u8MacStatus: Mac Status from firmware
*  			  tstrDisconnectNotifInfo* pstrDisconnectNotifInfo: Disconnection Notification
*  			  void* pUserVoid: Private data associated with wireless interface
*  @return 	NONE
*  @author	mabubakr
*  @date	01 MAR 2012
*  @version	1.0
*/
static void CfgConnectResult(tenuConnDisconnEvent enuConnDisconnEvent, 
							   tstrConnectInfo* pstrConnectInfo, 
							   NMI_Uint8 u8MacStatus,
							   tstrDisconnectNotifInfo* pstrDisconnectNotifInfo,
							   void* pUserVoid)
{
	struct NMI_WFI_priv* priv;
	struct net_device* dev;

	if(enuConnDisconnEvent == CONN_DISCONN_EVENT_CONN_RESP)
	{
		/*Initialization*/
		NMI_Uint16 u16ConnectStatus = WLAN_STATUS_SUCCESS;
		
		priv = (struct NMI_WFI_priv*)pUserVoid;
		
		dev = priv->dev;
		
		u16ConnectStatus = pstrConnectInfo->u16ConnectStatus;
			
		PRINT_D(CFG80211_DBG," Connection response received = %d\n",u8MacStatus);

		if((u8MacStatus == MAC_DISCONNECTED) &&
		    (pstrConnectInfo->u16ConnectStatus == SUCCESSFUL_STATUSCODE))
		{
			/* The case here is that our station was waiting for association response frame and has just received it containing status code
			    = SUCCESSFUL_STATUSCODE, while mac status is MAC_DISCONNECTED (which means something wrong happened) */
			u16ConnectStatus = WLAN_STATUS_UNSPECIFIED_FAILURE;
			PRINT_ER("Unspecified failure: Connection status %d : MAC status = %d \n",u16ConnectStatus,u8MacStatus);
		}
		
		if(u16ConnectStatus == WLAN_STATUS_SUCCESS)
		{
			NMI_Bool bNeedScanRefresh = NMI_FALSE;
			NMI_Uint32 i;
			
			PRINT_INFO(CFG80211_DBG,"Connection Successful:: BSSID: %x%x%x%x%x%x\n",pstrConnectInfo->au8bssid[0],
				pstrConnectInfo->au8bssid[1],pstrConnectInfo->au8bssid[2],pstrConnectInfo->au8bssid[3],pstrConnectInfo->au8bssid[4],pstrConnectInfo->au8bssid[5]);
			NMI_memcpy(priv->au8AssociatedBss, pstrConnectInfo->au8bssid, ETH_ALEN);

			

			/* BugID_4209: if this network has expired in the scan results in the above nl80211 layer, refresh them here by calling 
			    cfg80211_inform_bss() with the last Scan results before calling cfg80211_connect_result() to avoid 
			    Linux kernel warning generated at the nl80211 layer */

			for(i = 0; i < priv->u32LastScannedNtwrksCountShadow; i++)
			{	
				if(NMI_memcmp(priv->astrLastScannedNtwrksShadow[i].au8bssid,
								  pstrConnectInfo->au8bssid, ETH_ALEN) == 0)
				{
					unsigned long now = jiffies;
					
					if(time_after(now, 
						priv->astrLastScannedNtwrksShadow[i].u32TimeRcvdInScanCached + (unsigned long)(nl80211_SCAN_RESULT_EXPIRE - (1 * HZ))))
					{
						bNeedScanRefresh = NMI_TRUE;
					}
					
					break;
				}
			}
			
			if(bNeedScanRefresh == NMI_TRUE)
			{
				//RefreshScanResult(priv);
				refresh_scan(priv, 1);

			}
			
		}


		PRINT_D(CFG80211_DBG,"Association request info elements length = %d\n", pstrConnectInfo->ReqIEsLen);
		
		PRINT_D(CFG80211_DBG,"Association response info elements length = %d\n", pstrConnectInfo->u16RespIEsLen);
		
		cfg80211_connect_result(dev, pstrConnectInfo->au8bssid,
								pstrConnectInfo->pu8ReqIEs, pstrConnectInfo->ReqIEsLen,
								pstrConnectInfo->pu8RespIEs, pstrConnectInfo->u16RespIEsLen,
								u16ConnectStatus, GFP_KERNEL); //TODO: mostafa: u16ConnectStatus to
															   // be replaced by pstrConnectInfo->u16ConnectStatus
	}
	else if(enuConnDisconnEvent == CONN_DISCONN_EVENT_DISCONN_NOTIF)
	{
		priv = (struct NMI_WFI_priv*)pUserVoid;
		dev = priv->dev;

		PRINT_ER("Received MAC_DISCONNECTED from firmware with reason %d\n",
		pstrDisconnectNotifInfo->u16reason);

		NMI_memset(priv->au8AssociatedBss, 0, ETH_ALEN);

		cfg80211_disconnected(dev, pstrDisconnectNotifInfo->u16reason, pstrDisconnectNotifInfo->ie, 
						      pstrDisconnectNotifInfo->ie_len, GFP_KERNEL);

	}

}


/**
*  @brief 	NMI_WFI_CfgSetChannel
*  @details 	Set channel for a given wireless interface. Some devices
*      		may support multi-channel operation (by channel hopping) so cfg80211
*      		doesn't verify much. Note, however, that the passed netdev may be
*      		%NULL as well if the user requested changing the channel for the
*      		device itself, or for a monitor interface.
*  @param[in]   
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
/*
*	struct changed from v3.8.0
*	tony, sswd, NMI-KR, 2013-10-29
*	struct cfg80211_chan_def {
         struct ieee80211_channel *chan;
         enum nl80211_chan_width width;
         u32 center_freq1;
         u32 center_freq2;
 	};
*/
static int NMI_WFI_CfgSetChannel(struct wiphy *wiphy, 
		struct cfg80211_chan_def *chandef)
#else
static int NMI_WFI_CfgSetChannel(struct wiphy *wiphy,
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32)
		struct net_device *netdev,
	#endif
		struct ieee80211_channel *channel,
		enum nl80211_channel_type channel_type)
#endif
{

	NMI_Uint32 channelnum = 0;
	struct NMI_WFI_priv* priv;
	NMI_Sint32 s32Error = NMI_SUCCESS;
	priv = wiphy_priv(wiphy);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)	/* tony for v3.8.0 support */
	channelnum = ieee80211_frequency_to_channel(chandef->chan->center_freq);
	PRINT_D(CFG80211_DBG,"Setting channel %d with frequency %d\n", channelnum, chandef->chan->center_freq );
#else
	channelnum = ieee80211_frequency_to_channel(channel->center_freq);
	PRINT_D(CFG80211_DBG,"Setting channel %d with frequency %d\n", channelnum, channel->center_freq );
#endif

	if(priv->u8CurrChannel != channelnum)
	{
		priv->u8CurrChannel = channelnum;
		s32Error   = host_int_set_mac_chnl_num(priv->hNMIWFIDrv,channelnum);

		if(s32Error != NMI_SUCCESS)
			PRINT_ER("Error in setting channel %d\n", channelnum);
	}

	return s32Error;
}

/**
*  @brief 	NMI_WFI_CfgScan
*  @details 	Request to do a scan. If returning zero, the scan request is given
*      		the driver, and will be valid until passed to cfg80211_scan_done().
*      		For scan results, call cfg80211_inform_bss(); you can call this outside
*      		the scan/scan_done bracket too.
*  @param[in]   
*  @return 	int : Return 0 on Success
*  @author	mabubakr
*  @date	01 MAR 2012	
*  @version	1.0
*/

/*
*	kernel version 3.8.8 supported 
*	tony, sswd, NMI-KR, 2013-10-29
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)
static int NMI_WFI_CfgScan(struct wiphy *wiphy, struct cfg80211_scan_request *request)
#else
static int NMI_WFI_CfgScan(struct wiphy *wiphy,struct net_device *dev, struct cfg80211_scan_request *request)
#endif
{
	struct NMI_WFI_priv* priv;
	NMI_Uint32 i;
	NMI_Sint32 s32Error = NMI_SUCCESS;
	NMI_Uint8 au8ScanChanList[MAX_NUM_SCANNED_NETWORKS];
	tstrHiddenNetwork strHiddenNetwork;

	priv = wiphy_priv(wiphy);
	/*BugID_4800: if in AP mode, return.*/
	/*This check is to handle the situation when user*/
	/*requests "create group" during a running scan*/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)	/* tony for v3.8.0 support */
	if(priv->dev->ieee80211_ptr->iftype == NL80211_IFTYPE_AP)
	{
		PRINT_D(GENERIC_DBG,"Required scan while in AP mode");
		return s32Error;
	}
#else
	if(dev->ieee80211_ptr->iftype == NL80211_IFTYPE_AP)
	{
		PRINT_D(GENERIC_DBG,"Required scan while in AP mode");
		return s32Error;
	}
#endif
	priv->pstrScanReq = request;

	priv->u32RcvdChCount = 0;



	reset_shadow_found(priv);

	priv->bCfgScanning = NMI_TRUE;
	priv->u8CurrChannel =-1;
	if(request->n_channels <= MAX_NUM_SCANNED_NETWORKS) //TODO: mostafa: to be replaced by
														//               max_scan_ssids
	{
		for(i = 0; i < request->n_channels; i++)
		{
			au8ScanChanList[i] = (NMI_Uint8)ieee80211_frequency_to_channel(request->channels[i]->center_freq);
			PRINT_INFO(CFG80211_DBG, "ScanChannel List[%d] = %d,",i,au8ScanChanList[i]);
		}

		PRINT_D(CFG80211_DBG,"Requested num of scan channel %d\n",request->n_channels);
		PRINT_D(CFG80211_DBG,"Scan Request IE len =  %d\n",request->ie_len);
		
		PRINT_D(CFG80211_DBG,"Number of SSIDs %d\n",request->n_ssids);
		
		if(request->n_ssids >= 1)
		{
		
			
			strHiddenNetwork.pstrHiddenNetworkInfo = NMI_MALLOC(request->n_ssids * sizeof(tstrHiddenNetwork));
			strHiddenNetwork.u8ssidnum = request->n_ssids;

		
			/*BugID_4156*/
			for(i=0;i<request->n_ssids;i++)
			{
				
				if(request->ssids[i].ssid != NULL && request->ssids[i].ssid_len!=0)
				{
					strHiddenNetwork.pstrHiddenNetworkInfo[i].pu8ssid= NMI_MALLOC( request->ssids[i].ssid_len);
					NMI_memcpy(strHiddenNetwork.pstrHiddenNetworkInfo[i].pu8ssid,request->ssids[i].ssid,request->ssids[i].ssid_len);
					strHiddenNetwork.pstrHiddenNetworkInfo[i].u8ssidlen = request->ssids[i].ssid_len;
				}
				else
				{
							PRINT_D(CFG80211_DBG,"Received one NULL SSID \n");
							strHiddenNetwork.u8ssidnum-=1;
				}
			}
			PRINT_D(CFG80211_DBG,"Trigger Scan Request \n");
			s32Error = host_int_scan(priv->hNMIWFIDrv, USER_SCAN, ACTIVE_SCAN,
					  au8ScanChanList, request->n_channels,
					  (const NMI_Uint8*)request->ie, request->ie_len,
					  CfgScanResult, (void*)priv,&strHiddenNetwork);
		}
		else
		{
			PRINT_D(CFG80211_DBG,"Trigger Scan Request \n");
			s32Error = host_int_scan(priv->hNMIWFIDrv, USER_SCAN, ACTIVE_SCAN,
					  au8ScanChanList, request->n_channels,
					  (const NMI_Uint8*)request->ie, request->ie_len,
					  CfgScanResult, (void*)priv,NULL);
		}	
		
	}
	else
	{
		PRINT_ER("Requested num of scanned channels is greater than the max, supported"
				  " channels \n");
	}

	if(s32Error != NMI_SUCCESS)
	{
		s32Error = -EBUSY;
		PRINT_WRN(CFG80211_DBG,"Device is busy: Error(%d)\n",s32Error);
	}

	return s32Error;
}

/**
*  @brief 	NMI_WFI_CfgConnect
*  @details 	Connect to the ESS with the specified parameters. When connected,
*      		call cfg80211_connect_result() with status code %WLAN_STATUS_SUCCESS.
*      		If the connection fails for some reason, call cfg80211_connect_result()
*      		with the status from the AP.
*  @param[in]   
*  @return 	int : Return 0 on Success
*  @author	mabubakr 
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_CfgConnect(struct wiphy *wiphy, struct net_device *dev,
	struct cfg80211_connect_params *sme)
{	
	NMI_Sint32 s32Error = NMI_SUCCESS;
	NMI_Uint32 i;
	//SECURITY_T  tenuSecurity_t = NO_SECURITY;
	NMI_Uint8 u8security = NO_ENCRYPT;
	AUTHTYPE_T tenuAuth_type = ANY;
	NMI_Char * pcgroup_encrypt_val;
	NMI_Char * pccipher_group;
	NMI_Char * pcwpa_version;
	
	struct NMI_WFI_priv* priv;
	tstrNetworkInfo* pstrNetworkInfo = NULL;
	
	priv = wiphy_priv(wiphy);

	PRINT_D(CFG80211_DBG,"Connecting to SSID [%s]\n",sme->ssid);
	#ifdef NMI_P2P
	if(!(NMI_strncmp(sme->ssid,"DIRECT-", 7)))
	{
			PRINT_D(CFG80211_DBG,"Connected to Direct network,OBSS disabled\n");
			gWFiDrvHandle->u8P2PConnect =1;
	}
	else
		gWFiDrvHandle->u8P2PConnect= 0;
	#endif
	PRINT_INFO(CFG80211_DBG,"Required SSID = %s\n , AuthType = %d \n", sme->ssid,sme->auth_type);
	
	for(i = 0; i < priv->u32LastScannedNtwrksCountShadow; i++)
	{		
		if((sme->ssid_len == priv->astrLastScannedNtwrksShadow[i].u8SsidLen) && 
			NMI_memcmp(priv->astrLastScannedNtwrksShadow[i].au8ssid, 
						sme->ssid, 
						sme->ssid_len) == 0)
		{
			PRINT_INFO(CFG80211_DBG,"Network with required SSID is found %s\n", sme->ssid);
			if(sme->bssid == NULL)
			{
				/* BSSID is not passed from the user, so decision of matching
				 * is done by SSID only */
				PRINT_INFO(CFG80211_DBG,"BSSID is not passed from the user\n");
				break;
			}
			else
			{
				/* BSSID is also passed from the user, so decision of matching
				 * should consider also this passed BSSID */
				if(NMI_memcmp(priv->astrLastScannedNtwrksShadow[i].au8bssid, 
							       sme->bssid, 
							       ETH_ALEN) == 0)
				{
					PRINT_INFO(CFG80211_DBG,"BSSID is passed from the user and matched\n");
					break;
				}
			}
		}
	}			

	if(i < priv->u32LastScannedNtwrksCountShadow)
	{
		PRINT_D(CFG80211_DBG, "Required bss is in scan results\n");

		pstrNetworkInfo = &(priv->astrLastScannedNtwrksShadow[i]);
		
		PRINT_INFO(CFG80211_DBG,"network BSSID to be associated: %x%x%x%x%x%x\n",
						pstrNetworkInfo->au8bssid[0], pstrNetworkInfo->au8bssid[1], 
						pstrNetworkInfo->au8bssid[2], pstrNetworkInfo->au8bssid[3], 
						pstrNetworkInfo->au8bssid[4], pstrNetworkInfo->au8bssid[5]);		
	}
	else
	{
		s32Error = -ENOENT;
		if(priv->u32LastScannedNtwrksCountShadow == 0)
			PRINT_D(CFG80211_DBG,"No Scan results yet\n");
		else
			PRINT_D(CFG80211_DBG,"Required bss not in scan results: Error(%d)\n",s32Error);

		goto done;
	}	
	
	priv->NMI_WFI_wep_default = 0;
	NMI_memset(priv->NMI_WFI_wep_key, 0, sizeof(priv->NMI_WFI_wep_key));
	NMI_memset(priv->NMI_WFI_wep_key_len, 0, sizeof(priv->NMI_WFI_wep_key_len));	
	
	PRINT_INFO(CFG80211_DBG,"sme->crypto.wpa_versions=%x\n", sme->crypto.wpa_versions);
	PRINT_INFO(CFG80211_DBG,"sme->crypto.cipher_group=%x\n", sme->crypto.cipher_group);

	PRINT_INFO(CFG80211_DBG,"sme->crypto.n_ciphers_pairwise=%d\n", sme->crypto.n_ciphers_pairwise);

	if(INFO)
	{
		for(i=0 ; i < sme->crypto.n_ciphers_pairwise ; i++)
			NMI_PRINTF("sme->crypto.ciphers_pairwise[%d]=%x\n", i, sme->crypto.ciphers_pairwise[i]);
	}
	
	if(sme->crypto.cipher_group != NO_ENCRYPT)
	{
		/* To determine the u8security value, first we check the group cipher suite then {in case of WPA or WPA2}
		    we will add to it the pairwise cipher suite(s) */
		//switch(sme->crypto.wpa_versions)
		//{
			//printk(">> sme->crypto.wpa_versions: %x\n",sme->crypto.wpa_versions);
			//case NL80211_WPA_VERSION_1:
			if( sme->crypto.wpa_versions & NL80211_WPA_VERSION_2)
				{
						//printk("> wpa_version: NL80211_WPA_VERSION_2\n");
						//case NL80211_WPA_VERSION_2:
						if (sme->crypto.cipher_group == WLAN_CIPHER_SUITE_TKIP)
						{
							//printk("> WPA2-TKIP\n");
							//tenuSecurity_t = WPA2_TKIP;
							u8security = ENCRYPT_ENABLED | WPA2 | TKIP;
							pcgroup_encrypt_val = "WPA2_TKIP";
							pccipher_group = "TKIP";
						}
						else //TODO: mostafa: here we assume that any other encryption type is AES
						{
							//printk("> WPA2-AES\n");
							//tenuSecurity_t = WPA2_AES;
							u8security = ENCRYPT_ENABLED | WPA2 | AES;
							pcgroup_encrypt_val = "WPA2_AES";
							pccipher_group = "AES";
						}				
						pcwpa_version = "WPA_VERSION_2";
					}
			else if( sme->crypto.wpa_versions & NL80211_WPA_VERSION_1)
				{
				//printk("> wpa_version: NL80211_WPA_VERSION_1\n");
				if (sme->crypto.cipher_group == WLAN_CIPHER_SUITE_TKIP)
				{
					//printk("> WPA-TKIP\n");
					//tenuSecurity_t = WPA_TKIP;
					u8security = ENCRYPT_ENABLED | WPA | TKIP;
					pcgroup_encrypt_val = "WPA_TKIP";
					pccipher_group = "TKIP";
				}
				else //TODO: mostafa: here we assume that any other encryption type is AES
				{
					//printk("> WPA-AES\n");
					//tenuSecurity_t = WPA_AES;
					u8security = ENCRYPT_ENABLED | WPA | AES;
					pcgroup_encrypt_val = "WPA_AES";
					pccipher_group = "AES";

				}				
				pcwpa_version = "WPA_VERSION_1";

				//break;
			}
					else{
						//break;
					//default:
						
						pcwpa_version = "Default";
						PRINT_D(CFG80211_DBG,"Adding key with cipher group = %x\n",sme->crypto.cipher_group);
						if (sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP40)
						{
							//printk("> WEP\n");
							//tenuSecurity_t = WEP_40;
							u8security = ENCRYPT_ENABLED | WEP;
							pcgroup_encrypt_val = "WEP40";
							pccipher_group = "WLAN_CIPHER_SUITE_WEP40";
						}
						else if (sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP104)
						{
							//printk("> WEP-WEP_EXTENDED\n");
							//tenuSecurity_t = WEP_104;
							u8security = ENCRYPT_ENABLED | WEP | WEP_EXTENDED;
							pcgroup_encrypt_val = "WEP104";
							pccipher_group = "WLAN_CIPHER_SUITE_WEP104";
						}
						else
						{
							s32Error = -ENOTSUPP;
							PRINT_ER("Not supported cipher: Error(%d)\n",s32Error);
							//PRINT_ER("Cipher-Group: %x\n",sme->crypto.cipher_group);

					goto done;
				}
				
				PRINT_INFO(CFG80211_DBG, "WEP Default Key Idx = %d\n",sme->key_idx);

				if(INFO)
				{
					for(i=0;i<sme->key_len;i++)
						NMI_PRINTF("WEP Key Value[%d] = %d\n", i, sme->key[i]);
				}
				
				priv->NMI_WFI_wep_default = sme->key_idx;
				priv->NMI_WFI_wep_key_len[sme->key_idx] = sme->key_len;
				NMI_memcpy(priv->NMI_WFI_wep_key[sme->key_idx], sme->key, sme->key_len);
				host_int_set_WEPDefaultKeyID(priv->hNMIWFIDrv,sme->key_idx);
				host_int_add_wep_key_bss_sta(priv->hNMIWFIDrv, sme->key,sme->key_len,sme->key_idx);
						//break;
		}

		/* After we set the u8security value from checking the group cipher suite, {in case of WPA or WPA2} we will 
		     add to it the pairwise cipher suite(s) */		
		if((sme->crypto.wpa_versions & NL80211_WPA_VERSION_1) 
		    || (sme->crypto.wpa_versions & NL80211_WPA_VERSION_2))
		{
			for(i=0 ; i < sme->crypto.n_ciphers_pairwise ; i++)
			{
				if (sme->crypto.ciphers_pairwise[i] == WLAN_CIPHER_SUITE_TKIP)
				{					
					u8security = u8security | TKIP;					
				}
				else //TODO: mostafa: here we assume that any other encryption type is AES 
				{					
					u8security = u8security | AES;
				}
			}
		}
		
	}
	PRINT_D(CFG80211_DBG, "Authentication Type = %d\n",sme->auth_type);
	switch(sme->auth_type)
	{
		case NL80211_AUTHTYPE_OPEN_SYSTEM:
			PRINT_D(CFG80211_DBG, "In OPEN SYSTEM\n");
			tenuAuth_type = OPEN_SYSTEM;
			break;
		case NL80211_AUTHTYPE_SHARED_KEY:
			tenuAuth_type = SHARED_KEY;
   			PRINT_D(CFG80211_DBG, "In SHARED KEY\n");
			break;
		default:
			PRINT_D(CFG80211_DBG, "Automatic Authentation type = %d\n",sme->auth_type);
	}


	/* ai: key_mgmt: enterprise case */
	if (sme->crypto.n_akm_suites) 
	{
		switch (sme->crypto.akm_suites[0]) 
		{
			case WLAN_AKM_SUITE_8021X:
				tenuAuth_type = IEEE8021;
				break;
			default:
				//PRINT_D(CFG80211_DBG, "security unhandled [%x] \n",sme->crypto.akm_suites[0]);
				break;
		}
	}
	
	
	PRINT_INFO(CFG80211_DBG, "Required Channel = %d\n", pstrNetworkInfo->u8channel);
	
	PRINT_INFO(CFG80211_DBG, "Group encryption value = %s\n Cipher Group = %s\n WPA version = %s\n",
							      pcgroup_encrypt_val, pccipher_group,pcwpa_version);

	/*Added by Amr - BugID_4793*/
	if(priv->u8CurrChannel != pstrNetworkInfo->u8channel)
		priv->u8CurrChannel = pstrNetworkInfo->u8channel;
		
	s32Error = host_int_set_join_req(priv->hNMIWFIDrv, pstrNetworkInfo->au8bssid, sme->ssid,
							  	    sme->ssid_len, sme->ie, sme->ie_len,
							  	    CfgConnectResult, (void*)priv, u8security, 
							  	    tenuAuth_type, pstrNetworkInfo->u8channel,
							  	    pstrNetworkInfo->pJoinParams);
	if(s32Error != NMI_SUCCESS)
	{
		PRINT_ER("host_int_set_join_req(): Error(%d) \n", s32Error);
		s32Error = -ENOENT;
		goto done;
	}

done:
	
	return s32Error;
}


/**
*  @brief 	NMI_WFI_disconnect
*  @details 	Disconnect from the BSS/ESS.
*  @param[in]   
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_disconnect(struct wiphy *wiphy, struct net_device *dev,u16 reason_code)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	struct NMI_WFI_priv* priv;
	


	priv = wiphy_priv(wiphy);

	PRINT_D(CFG80211_DBG,"Disconnecting with reason code(%d)\n", reason_code);

	#ifdef NMI_P2P
	gWFiDrvHandle->u64P2p_MgmtTimeout = 0;
	#endif

	s32Error = host_int_disconnect(priv->hNMIWFIDrv, reason_code);
	if(s32Error != NMI_SUCCESS)
	{
		PRINT_ER("Error in disconnecting: Error(%d)\n", s32Error);
		s32Error = -EINVAL;
	}
	
	return s32Error;
}

/**
*  @brief 	NMI_WFI_add_key
*  @details 	Add a key with the given parameters. @mac_addr will be %NULL
*      		when adding a group key.
*  @param[in] key : key buffer; TKIP: 16-byte temporal key, 8-byte Tx Mic key, 8-byte Rx Mic Key
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_add_key(struct wiphy *wiphy, struct net_device *netdev,u8 key_index,
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
		bool pairwise,
	#endif
		const u8 *mac_addr,struct key_params *params)

{
	NMI_Sint32 s32Error = NMI_SUCCESS,KeyLen = params->key_len;
	NMI_Uint32 i;
	struct NMI_WFI_priv* priv;
	NMI_Uint8* pu8RxMic = NULL;
	NMI_Uint8* pu8TxMic = NULL;
	NMI_Uint8 u8mode = NO_ENCRYPT;
	#ifdef NMI_AP_EXTERNAL_MLME	
	NMI_Uint8 u8gmode = NO_ENCRYPT;
	NMI_Uint8 u8pmode = NO_ENCRYPT;
	AUTHTYPE_T tenuAuth_type = ANY;
	#endif
	
	priv = wiphy_priv(wiphy);

	PRINT_D(CFG80211_DBG,"Adding key with cipher suite = %x\n",params->cipher);		
	
	switch(params->cipher)
		{
			case WLAN_CIPHER_SUITE_WEP40:
			case WLAN_CIPHER_SUITE_WEP104:
				#ifdef NMI_AP_EXTERNAL_MLME
				if(priv->wdev->iftype == NL80211_IFTYPE_AP)
				{
				
					priv->NMI_WFI_wep_default = key_index;
					priv->NMI_WFI_wep_key_len[key_index] = params->key_len;
					NMI_memcpy(priv->NMI_WFI_wep_key[key_index], params->key, params->key_len);

					PRINT_D(CFG80211_DBG, "Adding AP WEP Default key Idx = %d\n",key_index);
					PRINT_D(CFG80211_DBG, "Adding AP WEP Key len= %d\n",params->key_len);
					
					for(i=0;i<params->key_len;i++)
						PRINT_D(CFG80211_DBG, "WEP AP key val[%d] = %x\n", i, params->key[i]);

					tenuAuth_type = OPEN_SYSTEM;

					if(params->cipher == WLAN_CIPHER_SUITE_WEP40)
						u8mode = ENCRYPT_ENABLED | WEP;
					else
						u8mode = ENCRYPT_ENABLED | WEP | WEP_EXTENDED;
				
					host_int_add_wep_key_bss_ap(priv->hNMIWFIDrv,params->key,params->key_len,key_index,u8mode,tenuAuth_type);
				break;
				}
				#endif
				if(NMI_memcmp(params->key,priv->NMI_WFI_wep_key[key_index],params->key_len))
				{
					priv->NMI_WFI_wep_default = key_index;
					priv->NMI_WFI_wep_key_len[key_index] = params->key_len;
					NMI_memcpy(priv->NMI_WFI_wep_key[key_index], params->key, params->key_len);

					PRINT_D(CFG80211_DBG, "Adding WEP Default key Idx = %d\n",key_index);
					PRINT_D(CFG80211_DBG, "Adding WEP Key length = %d\n",params->key_len);
					if(INFO)
					{
						for(i=0;i<params->key_len;i++)
							PRINT_INFO(CFG80211_DBG, "WEP key value[%d] = %d\n", i, params->key[i]);
					}
					host_int_add_wep_key_bss_sta(priv->hNMIWFIDrv,params->key,params->key_len,key_index);
				}

				break;
			case WLAN_CIPHER_SUITE_TKIP:
			case WLAN_CIPHER_SUITE_CCMP:
				#ifdef NMI_AP_EXTERNAL_MLME				
				if(priv->wdev->iftype == NL80211_IFTYPE_AP)
				{

					if(priv->nmi_gtk[key_index] == NULL)
						priv->nmi_gtk[key_index] = (struct nmi_wfi_key *)NMI_MALLOC(sizeof(struct nmi_wfi_key));
					if(priv->nmi_ptk[key_index] == NULL)	
						priv->nmi_ptk[key_index] = (struct nmi_wfi_key *)NMI_MALLOC(sizeof(struct nmi_wfi_key));
					
					#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
					if (!pairwise)
			 		#else
				 	if (!mac_addr || is_broadcast_ether_addr(mac_addr))
					#endif
				 	{
					if(params->cipher == WLAN_CIPHER_SUITE_TKIP)
						u8gmode = ENCRYPT_ENABLED |WPA | TKIP;
					else
						u8gmode = ENCRYPT_ENABLED |WPA2 | AES;
					
					 priv->nmi_groupkey = u8gmode;
				 	 
					if(params->key_len > 16 && params->cipher == WLAN_CIPHER_SUITE_TKIP)
				 	{
				 		
				 		pu8TxMic = params->key+24;
						pu8RxMic = params->key+16;
						KeyLen = params->key_len - 16;
				 	}

					priv->nmi_gtk[key_index]->key = (NMI_Uint8 *)NMI_MALLOC(params->key_len);
					NMI_memcpy(priv->nmi_gtk[key_index]->key,params->key,params->key_len);
					
					if(priv->nmi_gtk[key_index]->seq == NULL)
						priv->nmi_gtk[key_index]->seq = (NMI_Uint8 *)NMI_MALLOC(params->seq_len);
					
					NMI_memcpy(priv->nmi_gtk[key_index]->seq,params->seq,params->seq_len);

					priv->nmi_gtk[key_index]->cipher= params->cipher;
					priv->nmi_gtk[key_index]->key_len=params->key_len;
					priv->nmi_gtk[key_index]->seq_len=params->seq_len;
					
				if(INFO)
				{	  
					for(i=0;i<params->key_len;i++)
						PRINT_INFO(CFG80211_DBG, "Adding group key value[%d] = %x\n", i, params->key[i]);
					for(i=0;i<params->seq_len;i++)
						PRINT_INFO(CFG80211_DBG, "Adding group seq value[%d] = %x\n", i, params->seq[i]);
				}
					
					
					host_int_add_rx_gtk(priv->hNMIWFIDrv,params->key,KeyLen,
						key_index,params->seq_len,params->seq,pu8RxMic,pu8TxMic,AP_MODE,u8gmode);
			
				}
					else
					{
					     PRINT_INFO(CFG80211_DBG,"STA Address: %x%x%x%x%x\n",mac_addr[0],mac_addr[1],mac_addr[2],mac_addr[3],mac_addr[4]);
					
					if(params->cipher == WLAN_CIPHER_SUITE_TKIP)
						u8pmode = ENCRYPT_ENABLED | WPA | TKIP;
					else
						u8pmode = priv->nmi_groupkey | AES;

					
					if(params->key_len > 16 && params->cipher == WLAN_CIPHER_SUITE_TKIP)
					 {
					 	
					 	pu8TxMic = params->key+24;
						pu8RxMic = params->key+16;
						KeyLen = params->key_len - 16;
					 }

					
					priv->nmi_ptk[key_index]->key = (NMI_Uint8 *)NMI_MALLOC(params->key_len);
					priv->nmi_ptk[key_index]->seq = (NMI_Uint8 *)NMI_MALLOC(params->seq_len);

					if(INFO)
					{
					for(i=0;i<params->key_len;i++)
						PRINT_INFO(CFG80211_DBG, "Adding pairwise key value[%d] = %x\n", i, params->key[i]);
					
					for(i=0;i<params->seq_len;i++)
						PRINT_INFO(CFG80211_DBG, "Adding group seq value[%d] = %x\n", i, params->seq[i]);
					}
						
					NMI_memcpy(priv->nmi_ptk[key_index]->key,params->key,params->key_len);
					NMI_memcpy(priv->nmi_ptk[key_index]->seq,params->seq,params->seq_len);
					priv->nmi_ptk[key_index]->cipher= params->cipher;
					priv->nmi_ptk[key_index]->key_len=params->key_len;
					priv->nmi_ptk[key_index]->seq_len=params->seq_len;
				
					host_int_add_ptk(priv->hNMIWFIDrv,params->key,KeyLen,mac_addr,
					pu8RxMic,pu8TxMic,AP_MODE,u8pmode,key_index);
					}
					break;
				}
				#endif
			
				{
				u8mode=0;
			 #if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
				if (!pairwise)
			 #else
				 if (!mac_addr || is_broadcast_ether_addr(mac_addr))
			#endif
				 {				 	
				 	if(params->key_len > 16 && params->cipher == WLAN_CIPHER_SUITE_TKIP)
				 	{
				 		//swap the tx mic by rx mic
				 		pu8RxMic = params->key+24;
						pu8TxMic = params->key+16;
						KeyLen = params->key_len - 16;
				 	}
					host_int_add_rx_gtk(priv->hNMIWFIDrv,params->key,KeyLen,
						key_index,params->seq_len,params->seq,pu8RxMic,pu8TxMic,STATION_MODE,u8mode);
					//host_int_add_tx_gtk(priv->hNMIWFIDrv,params->key_len,params->key,key_index);
				 }				 
				 else
				 {
				 	if(params->key_len > 16 && params->cipher == WLAN_CIPHER_SUITE_TKIP)
					 	{
					 		//swap the tx mic by rx mic
					 		pu8RxMic = params->key+24;
							pu8TxMic = params->key+16;
							KeyLen = params->key_len - 16;
					 	}					  	
				 	host_int_add_ptk(priv->hNMIWFIDrv,params->key,KeyLen,mac_addr,
					pu8RxMic,pu8TxMic,STATION_MODE,u8mode,key_index);
					 PRINT_D(CFG80211_DBG,"Adding pairwise key\n");
					if(INFO)
					{
						 for(i=0;i<params->key_len;i++)
							PRINT_INFO(CFG80211_DBG, "Adding pairwise key value[%d] = %d\n", i, params->key[i]);
					}
				 }
				}
				break;
			default:
				PRINT_ER("Not supported cipher: Error(%d)\n",s32Error);
				s32Error = -ENOTSUPP;

		}

	return s32Error;
}

/**
*  @brief 	NMI_WFI_del_key
*  @details 	Remove a key given the @mac_addr (%NULL for a group key)
*      		and @key_index, return -ENOENT if the key doesn't exist.
*  @param[in]   
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_del_key(struct wiphy *wiphy, struct net_device *netdev,
	u8 key_index,
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
		bool pairwise,
	#endif
		const u8 *mac_addr)
{
	struct NMI_WFI_priv* priv;
	NMI_Sint32 s32Error = NMI_SUCCESS;

	priv = wiphy_priv(wiphy);

		if(key_index >= 0 && key_index <=3)
		{
			NMI_memset(priv->NMI_WFI_wep_key[key_index], 0, priv->NMI_WFI_wep_key_len[key_index]);
			priv->NMI_WFI_wep_key_len[key_index] = 0;

			PRINT_D(CFG80211_DBG, "Removing WEP key with index = %d\n",key_index);
			host_int_remove_wep_key(priv->hNMIWFIDrv,key_index);
		}
		else
		{
			PRINT_D(CFG80211_DBG, "Removing all installed keys\n");
			host_int_remove_key(priv->hNMIWFIDrv,mac_addr);
		}

	return s32Error;
}

/**
*  @brief 	NMI_WFI_get_key
*  @details 	Get information about the key with the given parameters.
*      		@mac_addr will be %NULL when requesting information for a group
*      		key. All pointers given to the @callback function need not be valid
*      		after it returns. This function should return an error if it is
*      		not possible to retrieve the key, -ENOENT if it doesn't exist.
*  @param[in]   
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_get_key(struct wiphy *wiphy, struct net_device *netdev,u8 key_index,
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
		bool pairwise,
	#endif
	const u8 *mac_addr,void *cookie,void (*callback)(void* cookie, struct key_params*))
{	
	
		NMI_Sint32 s32Error = NMI_SUCCESS;	
			
		struct NMI_WFI_priv* priv;	
		struct  key_params key_params;	
		NMI_Uint32 i;
		priv= wiphy_priv(wiphy);

		
		#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36)
		if (!pairwise)
		#else
		if (!mac_addr || is_broadcast_ether_addr(mac_addr))
		#endif
		{
			PRINT_D(CFG80211_DBG,"Getting group key idx: %x\n",key_index);	
			
			key_params.key=priv->nmi_gtk[key_index]->key;
			key_params.cipher=priv->nmi_gtk[key_index]->cipher;	
			key_params.key_len=priv->nmi_gtk[key_index]->key_len;
			key_params.seq=priv->nmi_gtk[key_index]->seq;	
			key_params.seq_len=priv->nmi_gtk[key_index]->seq_len;
			if(INFO)
			{
			for(i=0;i<key_params.key_len;i++)
				PRINT_INFO(CFG80211_DBG,"Retrieved key value %x\n",key_params.key[i]);
			}	
		}
		else
		{
			PRINT_D(CFG80211_DBG,"Getting pairwise  key\n");

			key_params.key=priv->nmi_ptk[key_index]->key;
			key_params.cipher=priv->nmi_ptk[key_index]->cipher;	
			key_params.key_len=priv->nmi_ptk[key_index]->key_len;
			key_params.seq=priv->nmi_ptk[key_index]->seq;	
			key_params.seq_len=priv->nmi_ptk[key_index]->seq_len;
		}
		
		callback(cookie,&key_params);		

		return s32Error;//priv->nmi_gtk->key_len ?0 : -ENOENT;
}

/**
*  @brief 	NMI_WFI_set_default_key
*  @details 	Set the default management frame key on an interface
*  @param[in]   
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_set_default_key(struct wiphy *wiphy,struct net_device *netdev,u8 key_index
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,37)
		,bool unicast, bool multicast
	#endif
		)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	struct NMI_WFI_priv* priv;


	priv = wiphy_priv(wiphy);
	
	PRINT_D(CFG80211_DBG,"Setting default key with idx = %d \n", key_index);

	if(key_index!= priv->NMI_WFI_wep_default)
	{

		host_int_set_WEPDefaultKeyID(priv->hNMIWFIDrv,key_index);
	}

	return s32Error;
}

/**
*  @brief 	NMI_WFI_dump_survey
*  @details 	Get site survey information
*  @param[in]   
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_dump_survey(struct wiphy *wiphy, struct net_device *netdev,
	int idx, struct survey_info *info)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	

	if (idx != 0)
	{
		s32Error = -ENOENT;
		PRINT_ER("Error Idx value doesn't equal zero: Error(%d)\n",s32Error);
		
	}

	return s32Error;
}


/**
*  @brief 	NMI_WFI_get_station
*  @details 	Get station information for the station identified by @mac
*  @param[in]   NONE
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/

extern  uint32_t Statisitcs_totalAcks,Statisitcs_DroppedAcks;
static int NMI_WFI_get_station(struct wiphy *wiphy, struct net_device *dev,
	u8 *mac, struct station_info *sinfo)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	struct NMI_WFI_priv* priv;
	linux_wlan_t* nic;
	#ifdef NMI_AP_EXTERNAL_MLME
	NMI_Uint32 i =0;
	NMI_Uint32 associatedsta = 0;
	NMI_Uint32 inactive_time=0;
	#endif
	priv = wiphy_priv(wiphy);
	nic = netdev_priv(dev);
	
	#ifdef NMI_AP_EXTERNAL_MLME
	if(nic->iftype == AP_MODE)
	{
		PRINT_D(HOSTAPD_DBG,"Getting station parameters\n");

		PRINT_INFO(HOSTAPD_DBG, ": %x%x%x%x%x\n",mac[0],mac[1],mac[2],mac[3],mac[4]);
		
		for(i=0; i<NUM_STA_ASSOCIATED; i++)
		{

			if (!(memcmp(mac, priv->assoc_stainfo.au8Sta_AssociatedBss[i], ETH_ALEN)))
			{
			   associatedsta = i;
			   break;
			}
			
		}

		if(associatedsta == -1)
		{
			s32Error = -ENOENT;
			PRINT_ER("Station required is not associated : Error(%d)\n",s32Error);
			
			return s32Error;
		}
		
		sinfo->filled |= STATION_INFO_INACTIVE_TIME;

		host_int_get_inactive_time(priv->hNMIWFIDrv, mac,&(inactive_time));
		sinfo->inactive_time = 1000 * inactive_time;
		PRINT_D(CFG80211_DBG,"Inactive time %d\n",sinfo->inactive_time);
		
	}
	#endif
	
	if(nic->iftype == STATION_MODE)
	{
		tstrStatistics strStatistics;
		#if 0
		NMI_Sint8 s8lnkspd = 0;
		static NMI_Uint32 u32FirstLnkSpdCnt = 0;

		
		sinfo->filled |= STATION_INFO_SIGNAL;

		s32Error = host_int_get_rssi(priv->hNMIWFIDrv, &(sinfo->signal));
		if(s32Error)
			PRINT_ER("Failed to send get host channel param's message queue ");

		PRINT_D(CFG80211_DBG,"Rssi value = %d		%d,%d\n",sinfo->signal,Statisitcs_DroppedAcks,Statisitcs_totalAcks);

		sinfo->filled |=  STATION_INFO_TX_BITRATE;
		
		/* To get realistic Link Speed, let it fixed for first 10 times to give chance for Autorate to ramp up as traffic starts */
		
		u32FirstLnkSpdCnt++;
		
		if(u32FirstLnkSpdCnt > 10)
		{
			gbAutoRateAdjusted = NMI_TRUE;
		}

		if(gbAutoRateAdjusted == NMI_FALSE)
		{
			sinfo->txrate.legacy = 54 * 10;
		}
		else
		{	
			s32Error = host_int_get_link_speed(priv->hNMIWFIDrv, &s8lnkspd);
			if(s32Error)
			{
				PRINT_ER("Failed to send get host channel param's message queue ");
			}
			else
			{
				PRINT_D(CFG80211_DBG,"link_speed=%d\n", s8lnkspd);
				sinfo->txrate.legacy = s8lnkspd * 10;
			}
		}
		#else
		host_int_get_statistics(priv->hNMIWFIDrv,&strStatistics);
		
        /*
         * tony: 2013-11-13
         * tx_failed introduced more than 
         * kernel version 3.0.0
         */
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0) 
		sinfo->filled |= STATION_INFO_SIGNAL | STATION_INFO_RX_PACKETS | STATION_INFO_TX_PACKETS
			| STATION_INFO_TX_FAILED | STATION_INFO_TX_BITRATE;
    #else
        sinfo->filled |= STATION_INFO_SIGNAL | STATION_INFO_RX_PACKETS | STATION_INFO_TX_PACKETS
            | STATION_INFO_TX_BITRATE;
    #endif
		
		sinfo->signal  		=  strStatistics.s8RSSI;
		sinfo->rx_packets   =  strStatistics.u32RxCount;
		sinfo->tx_packets   =  strStatistics.u32TxCount + strStatistics.u32TxFailureCount;
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)	
        sinfo->tx_failed	=  strStatistics.u32TxFailureCount;   
    #endif
		sinfo->txrate.legacy = strStatistics.u8LinkSpeed*10;
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
		printk("*** stats[%d][%d][%d][%d][%d]\n",sinfo->signal,sinfo->rx_packets,sinfo->tx_packets,
			sinfo->tx_failed,sinfo->txrate.legacy);
    #else
        printk("*** stats[%d][%d][%d][%d]\n",sinfo->signal,sinfo->rx_packets,sinfo->tx_packets,
            sinfo->txrate.legacy);
    #endif
			
		#endif
				
	}
	
	
	
	return s32Error;
}


/**
*  @brief 	NMI_WFI_change_bss
*  @details 	Modify parameters for a given BSS.
*  @param[in]   
*   -use_cts_prot: Whether to use CTS protection
*	    (0 = no, 1 = yes, -1 = do not change)
 *  -use_short_preamble: Whether the use of short preambles is allowed
 *	    (0 = no, 1 = yes, -1 = do not change)
 *  -use_short_slot_time: Whether the use of short slot time is allowed
 *	    (0 = no, 1 = yes, -1 = do not change)
 *  -basic_rates: basic rates in IEEE 802.11 format
 *	    (or NULL for no change)
 *  -basic_rates_len: number of basic rates
 *  -ap_isolate: do not forward packets between connected stations
 *  -ht_opmode: HT Operation mode
 * 	   (u16 = opmode, -1 = do not change)
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int  NMI_WFI_change_bss(struct wiphy *wiphy, struct net_device *dev,
	struct bss_parameters *params)
{
	PRINT_D(CFG80211_DBG,"Changing Bss parametrs\n");
	return 0;
}

/**
*  @brief 	NMI_WFI_auth
*  @details 	Request to authenticate with the specified peer
*  @param[in]   
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_auth(struct wiphy *wiphy, struct net_device *dev,
	struct cfg80211_auth_request *req)
{
	PRINT_D(CFG80211_DBG,"In Authentication Function\n");
	return 0;
}

/**
*  @brief 	NMI_WFI_assoc
*  @details 	Request to (re)associate with the specified peer
*  @param[in]   
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_assoc(struct wiphy *wiphy, struct net_device *dev,
	struct cfg80211_assoc_request *req)
{
	PRINT_D(CFG80211_DBG,"In Association Function\n");
	return 0;
}

/**
*  @brief 	NMI_WFI_deauth
*  @details 	Request to deauthenticate from the specified peer
*  @param[in]   
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int  NMI_WFI_deauth(struct wiphy *wiphy, struct net_device *dev,
	struct cfg80211_deauth_request *req,void *cookie)
{
	PRINT_D(CFG80211_DBG,"In De-authentication Function\n");
	return 0;
}

/**
*  @brief 	NMI_WFI_disassoc
*  @details 	Request to disassociate from the specified peer
*  @param[in]   
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int  NMI_WFI_disassoc(struct wiphy *wiphy, struct net_device *dev,
	struct cfg80211_disassoc_request *req,void *cookie)
{
	PRINT_D(CFG80211_DBG, "In Disassociation Function\n");
	return 0;
}

/**
*  @brief 	NMI_WFI_set_wiphy_params
*  @details 	Notify that wiphy parameters have changed;
*  @param[in]   Changed bitfield (see &enum wiphy_params_flags) describes which values
*      		have changed.
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	tstrCfgParamVal pstrCfgParamVal;
	struct NMI_WFI_priv* priv;

	priv = wiphy_priv(wiphy);
//	priv = netdev_priv(priv->wdev->netdev);

	pstrCfgParamVal.u32SetCfgFlag = 0;
	PRINT_D(CFG80211_DBG,"Setting Wiphy params \n");
	
	if(changed & WIPHY_PARAM_RETRY_SHORT)
	{
		PRINT_D(CFG80211_DBG,"Setting WIPHY_PARAM_RETRY_SHORT %d\n",
			priv->dev->ieee80211_ptr->wiphy->retry_short);
		pstrCfgParamVal.u32SetCfgFlag  |= RETRY_SHORT;
		pstrCfgParamVal.short_retry_limit = priv->dev->ieee80211_ptr->wiphy->retry_short;
	}
	if(changed & WIPHY_PARAM_RETRY_LONG)
	{
		
		PRINT_D(CFG80211_DBG,"Setting WIPHY_PARAM_RETRY_LONG %d\n",priv->dev->ieee80211_ptr->wiphy->retry_long);
		pstrCfgParamVal.u32SetCfgFlag |= RETRY_LONG;
		pstrCfgParamVal.long_retry_limit = priv->dev->ieee80211_ptr->wiphy->retry_long;

	}
	if(changed & WIPHY_PARAM_FRAG_THRESHOLD)
	{
		PRINT_D(CFG80211_DBG,"Setting WIPHY_PARAM_FRAG_THRESHOLD %d\n",priv->dev->ieee80211_ptr->wiphy->frag_threshold);
		pstrCfgParamVal.u32SetCfgFlag |= FRAG_THRESHOLD;
		pstrCfgParamVal.frag_threshold = priv->dev->ieee80211_ptr->wiphy->frag_threshold;

	}

	if(changed & WIPHY_PARAM_RTS_THRESHOLD)
	{
		PRINT_D(CFG80211_DBG, "Setting WIPHY_PARAM_RTS_THRESHOLD %d\n",priv->dev->ieee80211_ptr->wiphy->rts_threshold);

		pstrCfgParamVal.u32SetCfgFlag |= RTS_THRESHOLD;
		pstrCfgParamVal.rts_threshold = priv->dev->ieee80211_ptr->wiphy->rts_threshold;

	}

	PRINT_D(CFG80211_DBG,"Setting CFG params in the host interface\n");
	s32Error = hif_set_cfg(priv->hNMIWFIDrv,&pstrCfgParamVal);
	if(s32Error)
		PRINT_ER("Error in setting WIPHY PARAMS\n");
		

	return s32Error;
}
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32)
/**
*  @brief 	NMI_WFI_set_bitrate_mask
*  @details 	set the bitrate mask configuration
*  @param[in]   
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_set_bitrate_mask(struct wiphy *wiphy,
	struct net_device *dev,const u8 *peer,
	const struct cfg80211_bitrate_mask *mask)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	tstrCfgParamVal pstrCfgParamVal;
	struct NMI_WFI_priv* priv;

	PRINT_D(CFG80211_DBG, "Setting Bitrate mask function\n");

	priv = wiphy_priv(wiphy);
	//priv = netdev_priv(priv->wdev->netdev);

	pstrCfgParamVal.curr_tx_rate = mask->control[IEEE80211_BAND_2GHZ].legacy;

	PRINT_D(CFG80211_DBG, "Tx rate = %d\n",pstrCfgParamVal.curr_tx_rate);
	s32Error = hif_set_cfg(priv->hNMIWFIDrv,&pstrCfgParamVal);

	if(s32Error)
		PRINT_ER("Error in setting bitrate\n");

	return s32Error;
	
}

/**
*  @brief 	NMI_WFI_set_pmksa
*  @details 	Cache a PMKID for a BSSID. This is mostly useful for fullmac
*      		devices running firmwares capable of generating the (re) association
*      		RSN IE. It allows for faster roaming between WPA2 BSSIDs.
*  @param[in]   
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_set_pmksa(struct wiphy *wiphy, struct net_device *netdev,
	struct cfg80211_pmksa *pmksa)
{
	NMI_Uint32 i;
	NMI_Sint32 s32Error = NMI_SUCCESS;
	NMI_Uint8 flag = 0;

	struct NMI_WFI_priv* priv = wiphy_priv(wiphy);
	
	PRINT_D(CFG80211_DBG, "Setting PMKSA\n");


	for (i = 0; i < priv->pmkid_list.numpmkid; i++)
	{
		if (!NMI_memcmp(pmksa->bssid,priv->pmkid_list.pmkidlist[i].bssid,
				ETH_ALEN))
		{
			/*If bssid already exists and pmkid value needs to reset*/
			flag = PMKID_FOUND;
			PRINT_D(CFG80211_DBG, "PMKID already exists\n");
			break;
		}
	}
	if (i < NMI_MAX_NUM_PMKIDS) {
		PRINT_D(CFG80211_DBG, "Setting PMKID in private structure\n");
		NMI_memcpy(priv->pmkid_list.pmkidlist[i].bssid, pmksa->bssid,
				ETH_ALEN);
		NMI_memcpy(priv->pmkid_list.pmkidlist[i].pmkid, pmksa->pmkid,
				PMKID_LEN);
		if(!(flag == PMKID_FOUND))
			priv->pmkid_list.numpmkid++;
	}
	else
	{
		PRINT_ER("Invalid PMKID index\n");
		s32Error = -EINVAL;
	}

	if(!s32Error)
	{
		PRINT_D(CFG80211_DBG, "Setting pmkid in the host interface\n");
		s32Error = host_int_set_pmkid_info(priv->hNMIWFIDrv, &priv->pmkid_list);
	}
	return s32Error;
}

/**
*  @brief 	NMI_WFI_del_pmksa
*  @details 	Delete a cached PMKID.
*  @param[in]   
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_del_pmksa(struct wiphy *wiphy, struct net_device *netdev,
	struct cfg80211_pmksa *pmksa)
{

	NMI_Uint32 i; 
	NMI_Uint8 flag = 0;
	NMI_Sint32 s32Error = NMI_SUCCESS;

	struct NMI_WFI_priv* priv = wiphy_priv(wiphy);
	//priv = netdev_priv(priv->wdev->netdev);

	PRINT_D(CFG80211_DBG, "Deleting PMKSA keys\n");

	for (i = 0; i < priv->pmkid_list.numpmkid; i++)
	{
		if (!NMI_memcmp(pmksa->bssid,priv->pmkid_list.pmkidlist[i].bssid,
					ETH_ALEN))
		{
			/*If bssid is found, reset the values*/
			PRINT_D(CFG80211_DBG, "Reseting PMKID values\n");
			NMI_memset(&priv->pmkid_list.pmkidlist[i], 0, sizeof(tstrHostIFpmkid));
			flag = PMKID_FOUND;
			break;
		}
	}

	if(i < priv->pmkid_list.numpmkid && priv->pmkid_list.numpmkid > 0)
	{
		for (; i < (priv->pmkid_list.numpmkid- 1); i++) {
				NMI_memcpy(priv->pmkid_list.pmkidlist[i].bssid,
					priv->pmkid_list.pmkidlist[i+1].bssid,
					ETH_ALEN);
				NMI_memcpy(priv->pmkid_list.pmkidlist[i].pmkid,
					priv->pmkid_list.pmkidlist[i].pmkid,
					PMKID_LEN);
			}
		priv->pmkid_list.numpmkid--;
	}
	else {
			s32Error = -EINVAL;
		}

	return s32Error;
}

/**
*  @brief 	NMI_WFI_flush_pmksa
*  @details 	Flush all cached PMKIDs.
*  @param[in]   
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int  NMI_WFI_flush_pmksa(struct wiphy *wiphy, struct net_device *netdev)
{
	struct NMI_WFI_priv* priv = wiphy_priv(wiphy);
	//priv = netdev_priv(priv->wdev->netdev);

	PRINT_D(CFG80211_DBG,  "Flushing  PMKID key values\n");

	/*Get cashed Pmkids and set all with zeros*/
	NMI_memset(&priv->pmkid_list, 0, sizeof(tstrHostIFpmkidAttr) );

	return 0;
}
#endif

#ifdef NMI_P2P

/**
*  @brief 			 NMI_WFI_p2p_rx
*  @details 		
*  @param[in]   	
				
*  @return 		None
*  @author		Mai Daftedar
*  @date			2 JUN 2013	
*  @version		1.0
*/

void NMI_WFI_p2p_rx (struct net_device *dev,uint8_t *buff, uint32_t size)
{

	struct NMI_WFI_priv* priv;
	NMI_Uint32 header,pkt_offset;

	priv= wiphy_priv(dev->ieee80211_ptr->wiphy);

	//Get NMI header
	memcpy(&header, (buff-HOST_HDR_OFFSET), HOST_HDR_OFFSET);


	//The packet offset field conain info about what type of managment frame 
	// we are dealing with and ack status
	pkt_offset = GET_PKT_OFFSET(header);
	if(pkt_offset & IS_MANAGMEMENT_CALLBACK)
	{
		if(buff[0]==IEEE80211_STYPE_PROBE_RESP)
		{
			PRINT_INFO(GENERIC_DBG,"Probe response ACK\n");
			cfg80211_mgmt_tx_status(dev,priv->u64tx_cookie,buff,size,true,GFP_KERNEL);
			return;
		}
		else
		{
        	if(pkt_offset & IS_MGMT_STATUS_SUCCES)
        	{		
				PRINT_INFO(GENERIC_DBG,"Success Ack\n");
        		cfg80211_mgmt_tx_status(dev,priv->u64tx_cookie,buff,size,true,GFP_KERNEL);
        	}
			else
			{
				PRINT_INFO(GENERIC_DBG,"Fail Ack\n");
				cfg80211_mgmt_tx_status(dev,priv->u64tx_cookie,buff,size,false,GFP_KERNEL);
			}
	  }
		
		
	}
	else
	{		
		if(priv->bCfgScanning == NMI_TRUE && jiffies >= gWFiDrvHandle->u64P2p_MgmtTimeout)
		{
			PRINT_D(GENERIC_DBG,"Receiving action frames from wrong channels\n");
			return;
		}
		
		PRINT_INFO(GENERIC_DBG,"Sending P2P to host\n");
		cfg80211_rx_mgmt(dev,priv->u32listen_freq,buff,size,GFP_ATOMIC);
	}

}
/**
*  @brief 			NMI_WFI_mgmt_tx_complete
*  @details 		Returns result of writing mgmt frame to VMM (Tx buffers are freed here)
*  @param[in]   	priv
				transmitting status
*  @return 		None
*  @author		Amr Abdelmoghny
*  @date			20 MAY 2013	
*  @version		1.0
*/
static void NMI_WFI_mgmt_tx_complete(void* priv, int status)
{
	struct p2p_mgmt_data* pv_data = (struct p2p_mgmt_data*)priv;

	 
	kfree(pv_data->buff);	
	kfree(pv_data);
}

/**
* @brief 		NMI_WFI_RemainOnChannelReady
*  @details 	Callback function, called from handle_remain_on_channel on being ready on channel 
*  @param   
*  @return 	none
*  @author	Amr abdelmoghny
*  @date		9 JUNE 2013	
*  @version	
*/

static void NMI_WFI_RemainOnChannelReady(void* pUserVoid)
{
	struct NMI_WFI_priv* priv;
	priv = (struct NMI_WFI_priv*)pUserVoid;	
	
	PRINT_D(HOSTINF_DBG, "Remain on channel ready \n");
	
	cfg80211_ready_on_channel(priv->dev,
							priv->strRemainOnChanParams.u64ListenCookie,
							priv->strRemainOnChanParams.pstrListenChan,
							priv->strRemainOnChanParams.tenuChannelType,
							priv->strRemainOnChanParams.u32ListenDuration,
							GFP_KERNEL);
}

/**
* @brief 		NMI_WFI_RemainOnChannelExpired
*  @details 	Callback function, called on expiration of remain-on-channel duration
*  @param   
*  @return 	none
*  @author	Amr abdelmoghny
*  @date		15 MAY 2013	
*  @version	
*/

static void NMI_WFI_RemainOnChannelExpired(void* pUserVoid)
{	
	struct NMI_WFI_priv* priv;
	priv = (struct NMI_WFI_priv*)pUserVoid;	
	
	PRINT_D(GENERIC_DBG, "Remain on channel expired \n");
       
	/*Inform wpas of remain-on-channel expiration*/
	cfg80211_remain_on_channel_expired(priv->dev,
		priv->strRemainOnChanParams.u64ListenCookie,
		priv->strRemainOnChanParams.pstrListenChan ,
		priv->strRemainOnChanParams.tenuChannelType,
		GFP_KERNEL);
	priv->u8CurrChannel =-1;
}


/**
*  @brief 	NMI_WFI_remain_on_channel
*  @details 	Request the driver to remain awake on the specified
*      		channel for the specified duration to complete an off-channel
*      		operation (e.g., public action frame exchange). When the driver is
*      		ready on the requested channel, it must indicate this with an event
*      		notification by calling cfg80211_ready_on_channel().
*  @param[in]   
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int  NMI_WFI_remain_on_channel(struct wiphy *wiphy,
	struct net_device *dev,struct ieee80211_channel *chan,
	enum nl80211_channel_type channel_type,
	unsigned int duration,u64 *cookie)
{
	NMI_Sint32 freq;
	NMI_Sint32 s32Error = NMI_SUCCESS;
	struct NMI_WFI_priv* priv;
	priv = wiphy_priv(wiphy);
	
	PRINT_D(GENERIC_DBG, "Remaining on channel %d\n",chan->hw_value);

	/*BugID_4800: if in AP mode, return.*/
	/*This check is to handle the situation when user*/
	/*requests "create group" during a running scan*/
	if(dev->ieee80211_ptr->iftype == NL80211_IFTYPE_AP)
	{
		PRINT_D(GENERIC_DBG,"Required remain-on-channel while in AP mode");
		return s32Error;
	}

	if(priv->u8CurrChannel != chan->hw_value)
		priv->u8CurrChannel = chan->hw_value;

	/*Setting params needed by NMI_WFI_RemainOnChannelExpired()*/
	priv->strRemainOnChanParams.pstrListenChan = chan;
	priv->strRemainOnChanParams.u64ListenCookie = *cookie;
	priv->strRemainOnChanParams.tenuChannelType = channel_type;

	#if LINUX_VERSION_CODE == KERNEL_VERSION(2, 6, 38) 
	freq = ieee80211_channel_to_frequency((NMI_Sint32)chan->hw_value);
	#else
	freq = ieee80211_channel_to_frequency((NMI_Sint32)chan->hw_value, IEEE80211_BAND_2GHZ);
        #endif
	priv->u32listen_freq= freq;

	s32Error = host_int_remain_on_channel(priv->hNMIWFIDrv, duration,chan->hw_value,NMI_WFI_RemainOnChannelExpired,NMI_WFI_RemainOnChannelReady,(void *)priv);
	
	return s32Error;
}

/**
*  @brief 	NMI_WFI_cancel_remain_on_channel
*  @details 	Cancel an on-going remain-on-channel operation.
*      		This allows the operation to be terminated prior to timeout based on
*      		the duration value.
*  @param[in]   struct wiphy *wiphy, 
*  @param[in] 	struct net_device *dev
*  @param[in] 	u64 cookie,
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int   NMI_WFI_cancel_remain_on_channel(struct wiphy *wiphy, struct net_device *dev,
	u64 cookie)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	struct NMI_WFI_priv* priv;
	priv = wiphy_priv(wiphy);
	
	PRINT_D(CFG80211_DBG, "Cancel remain on channel\n");

	s32Error = host_int_ListenStateExpired(priv->hNMIWFIDrv);
	return 0;
}


/**
*  @brief 	NMI_WFI_mgmt_tx_frame
*  @details 
*     	
*  @param[in]
*  @return 	NONE.
*  @author	mdaftedar
*  @date	01 JUL 2012
*  @version
*/

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,37)
int  NMI_WFI_mgmt_tx(struct wiphy *wiphy, struct net_device *dev,
                                                  struct ieee80211_channel *chan, bool offchan,
                                                  enum nl80211_channel_type channel_type,
                                                  bool channel_type_valid, unsigned int wait,
                                                  const u8 *buf, size_t len, u64 *cookie)
#else
int  NMI_WFI_mgmt_tx(struct wiphy *wiphy, struct net_device *dev,
                     struct ieee80211_channel *chan,
                     enum nl80211_channel_type channel_type,
                     bool channel_type_valid,
                     const u8 *buf, size_t len, u64 *cookie)
#endif
{
	const struct ieee80211_mgmt *mgmt;
	linux_wlan_t* nic;
	struct p2p_mgmt_data *mgmt_tx;
	struct NMI_WFI_priv* priv;
	NMI_Uint16 fc;
	NMI_Sint32 s32Error = NMI_SUCCESS;
	
	
	priv = wiphy_priv(wiphy);	
	nic =netdev_priv(dev);

	*cookie = (unsigned long)buf;
	priv->u64tx_cookie = *cookie;
	mgmt = (const struct ieee80211_mgmt *) buf;
	fc = mgmt->frame_control;
	/*mgmt frame allocation*/
	mgmt_tx = (struct p2p_mgmt_data*)NMI_MALLOC(sizeof(struct p2p_mgmt_data));
	if(mgmt_tx == NULL){
		PRINT_ER("Failed to allocate memory for mgmt_tx structure\n");
	     	return NMI_FAIL;
	}	
	mgmt_tx->buff= (char*)NMI_MALLOC(len);
	if(mgmt_tx->buff == NULL)
	{
		PRINT_ER("Failed to allocate memory for mgmt_tx buff\n");
	     	return NMI_FAIL;
	}
	memcpy(mgmt_tx->buff,buf,len);
	mgmt_tx->size=len;	
	
	/*send mgmt frame to firmware*/

	//if(priv->u8CurrChannel != chan->hw_value)
	{
		priv->u8CurrChannel = chan->hw_value;
	       host_int_set_mac_chnl_num(priv->hNMIWFIDrv, chan->hw_value);
	}
	if(fc == IEEE80211_STYPE_ACTION)
	{
		PRINT_D(GENERIC_DBG,"TX: ACTION FRAME : Chan:%d\n",chan->hw_value);
		gWFiDrvHandle->u64P2p_MgmtTimeout = (jiffies + msecs_to_jiffies(wait));
		
		PRINT_D(GENERIC_DBG,"Current Jiffies: %lu Timeout:%lu\n",jiffies,gWFiDrvHandle->u64P2p_MgmtTimeout);
	}	
	else if(fc == IEEE80211_STYPE_PROBE_RESP)
	{
		PRINT_D(GENERIC_DBG,"TX: Probe Response\n");
	}
       
	nic->oup.wlan_add_mgmt_to_tx_que(mgmt_tx,mgmt_tx->buff,mgmt_tx->size,NMI_WFI_mgmt_tx_complete);
	
	return s32Error;	
}

int   NMI_WFI_mgmt_tx_cancel_wait(struct wiphy *wiphy,
                               struct net_device *dev,
                               u64 cookie)
{
	PRINT_D(GENERIC_DBG,"Tx Cancel wait :%lu\n",jiffies);
	gWFiDrvHandle->u64P2p_MgmtTimeout = jiffies;
	
	return 0;
}

/**
*  @brief 	NMI_WFI_action
*  @details Transmit an action frame
*  @param[in]
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 JUL 2012
*  @version	1.0
* */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)
int  NMI_WFI_action(struct wiphy *wiphy, struct net_device *dev,
        	struct ieee80211_channel *chan,enum nl80211_channel_type channel_type,
            const u8 *buf, size_t len, u64 *cookie)
{
	PRINT_D(HOSTAPD_DBG,"In action function\n");
	return NMI_SUCCESS;
}
#endif
#else

/**
*  @brief 	NMI_WFI_frame_register
*  @details Notify driver that a management frame type was
*     		registered. Note that this callback may not sleep, and cannot run
*      		concurrently with itself.
*  @param[in]
*  @return 	NONE.
*  @author	mdaftedar
*  @date	01 JUL 2012
*  @version
*/
void    NMI_WFI_frame_register(struct wiphy *wiphy,struct net_device *dev,
           u16 frame_type, bool reg)
{

	struct NMI_WFI_priv* priv;
	linux_wlan_t *nic;
	
	
	priv = wiphy_priv(wiphy);
	nic = netdev_priv(priv->wdev->netdev);

	/*If mac is closed, then return*/
	if(!nic->nmc1000_initialized)
	{
		PRINT_D(GENERIC_DBG,"Return since mac is closed\n");
		return;
	}

	
	PRINT_INFO(GENERIC_DBG,"Frame registering Frame Type: %x: Boolean: %d\n",frame_type,reg);
	host_int_frame_register(priv->hNMIWFIDrv,frame_type,reg);
	switch(frame_type)
	{
	case PROBE_REQ:
		{
			nic->g_struct_frame_reg[0].frame_type =frame_type;
			nic->g_struct_frame_reg[0].reg= reg;
		}
		break;
		
	case ACTION:
		{
			nic->g_struct_frame_reg[1].frame_type =frame_type;
			nic->g_struct_frame_reg[1].reg= reg;
		}
		break;

		default:
		{
				break;
		}
		
	}
	
}
#endif
#endif /*NMI_P2P*/

/**
*  @brief 	NMI_WFI_set_cqm_rssi_config
*  @details 	Configure connection quality monitor RSSI threshold.
*  @param[in]   struct wiphy *wiphy: 
*  @param[in]	struct net_device *dev:
*  @param[in]  	s32 rssi_thold: 
*  @param[in]	u32 rssi_hyst: 
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int    NMI_WFI_set_cqm_rssi_config(struct wiphy *wiphy,
	struct net_device *dev,  s32 rssi_thold, u32 rssi_hyst)
{
	PRINT_D(CFG80211_DBG, "Setting CQM RSSi Function\n");
	return 0;

}
/**
*  @brief 	NMI_WFI_dump_station
*  @details 	Configure connection quality monitor RSSI threshold.
*  @param[in]   struct wiphy *wiphy: 
*  @param[in]	struct net_device *dev
*  @param[in]  	int idx
*  @param[in]	u8 *mac 
*  @param[in]	struct station_info *sinfo
*  @return 	int : Return 0 on Success
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_dump_station(struct wiphy *wiphy, struct net_device *dev,
	int idx, u8 *mac, struct station_info *sinfo)
{
	struct NMI_WFI_priv* priv;
	PRINT_D(CFG80211_DBG, "Dumping station information\n");
	
	if (idx != 0)
		return -ENOENT;

	 priv = wiphy_priv(wiphy);
	//priv = netdev_priv(priv->wdev->netdev);

	sinfo->filled |= STATION_INFO_SIGNAL ;
	
	host_int_get_rssi(priv->hNMIWFIDrv, &(sinfo->signal));

#if 0
	sinfo->filled |= STATION_INFO_TX_BYTES |
			 STATION_INFO_TX_PACKETS |
			 STATION_INFO_RX_BYTES |
			 STATION_INFO_RX_PACKETS | STATION_INFO_SIGNAL | STATION_INFO_INACTIVE_TIME;
	
	NMI_SemaphoreAcquire(&SemHandleUpdateStats,NULL);
	sinfo->inactive_time = priv->netstats.rx_time > priv->netstats.tx_time ? jiffies_to_msecs(jiffies - priv->netstats.tx_time) : jiffies_to_msecs(jiffies - priv->netstats.rx_time);
	sinfo->rx_bytes = priv->netstats.rx_bytes;
	sinfo->tx_bytes = priv->netstats.tx_bytes;
	sinfo->rx_packets = priv->netstats.rx_packets;
	sinfo->tx_packets = priv->netstats.tx_packets;
	NMI_SemaphoreRelease(&SemHandleUpdateStats,NULL);
#endif
	return 0;

}


/**
*  @brief 	NMI_WFI_set_power_mgmt
*  @details 	
*  @param[in]   
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 JUL 2012
*  @version	1.0NMI_WFI_set_cqmNMI_WFI_set_cqm_rssi_configNMI_WFI_set_cqm_rssi_configNMI_WFI_set_cqm_rssi_configNMI_WFI_set_cqm_rssi_config_rssi_config
*/
int NMI_WFI_set_power_mgmt(struct wiphy *wiphy, struct net_device *dev,
                                  bool enabled, int timeout)
{
	struct NMI_WFI_priv* priv;
	PRINT_D(CFG80211_DBG," Power save Enabled= %d , TimeOut = %d\n",enabled,timeout);
	
	if (wiphy == NMI_NULL)
		return -ENOENT;

	 priv = wiphy_priv(wiphy);
	 if(priv->hNMIWFIDrv == NMI_NULL)
 	{
 		NMI_ERROR("Driver is NULL\n");
		 return -EIO; 
 	}

	host_int_set_power_mgmt(priv->hNMIWFIDrv, enabled, timeout);


	return NMI_SUCCESS;

}
#ifdef NMI_AP_EXTERNAL_MLME
/**
*  @brief 	NMI_WFI_change_virt_intf
*  @details 	Change type/configuration of virtual interface,
*      		keep the struct wireless_dev's iftype updated.
*  @param[in]   NONE
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_change_virt_intf(struct wiphy *wiphy,struct net_device *dev,
	enum nl80211_iftype type, u32 *flags,struct vif_params *params)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	struct NMI_WFI_priv* priv;
	//struct NMI_WFI_mon_priv* mon_priv;
	linux_wlan_t* nic;

	nic = netdev_priv(dev);
	
	PRINT_D(HOSTAPD_DBG,"In Change virtual interface function\n");

	priv = wiphy_priv(wiphy);

	PRINT_D(HOSTAPD_DBG,"Wireless interface name =%s\n", dev->name);

	switch(type)
	{
	case NL80211_IFTYPE_STATION:

		PRINT_D(HOSTAPD_DBG,"Interface type = NL80211_IFTYPE_STATION\n");

		dev->ieee80211_ptr->iftype = NL80211_IFTYPE_STATION;
		nic->iftype = STATION_MODE;

		#ifndef SIMULATION
		PRINT_D(HOSTAPD_DBG,"Downloading STATION firmware\n");
		linux_wlan_get_firmware(nic);
		#ifdef NMI_P2P
		/*If nmc is running, then close-open to actually get new firmware running (serves P2P)*/
		if(nic->nmc1000_initialized)
		{
			mac_close(dev);
			mac_open(dev);
		}
		#endif
		#endif
		break;

	case NL80211_IFTYPE_AP:

		PRINT_D(HOSTAPD_DBG,"Interface type = NL80211_IFTYPE_AP\n");

		//mon_priv = netdev_priv(dev);
		//mon_priv->real_ndev = dev;
		dev->ieee80211_ptr->iftype = NL80211_IFTYPE_AP;
		priv->wdev->iftype = type;
		nic->iftype = AP_MODE;

		#ifndef SIMULATION
		PRINT_D(HOSTAPD_DBG,"Downloading AP firmware\n");
		linux_wlan_get_firmware(nic);
		#ifdef NMI_P2P
		/*If nmc is running, then close-open to actually get new firmware running (serves P2P)*/
		if(nic->nmc1000_initialized)
		{
			mac_close(dev);
			mac_open(dev);
		}
		#endif
		#endif
		
		break;
	default:
		PRINT_ER("Unknown interface type= %d\n", type);
		s32Error = -EINVAL;
		return s32Error;
		break;
	}

	return s32Error;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
/* (austin.2013-07-23) 

	To support revised cfg80211_ops

		add_beacon --> start_ap
		set_beacon --> change_beacon
		del_beacon --> stop_ap

		beacon_parameters  -->	cfg80211_ap_settings
								cfg80211_beacon_data

	applicable for linux kernel 3.4+
*/

/* cache of cfg80211_ap_settings later use in change_beacon api. */
static struct cfg80211_ap_settings *_g_pApSettings = NULL;

/**
*  @brief 	NMI_WFI_start_ap
*  @details 	Add a beacon with given parameters, @head, @interval
*      		and @dtim_period will be valid, @tail is optional.
*  @param[in]   wiphy
*  @param[in]   dev	The net device structure
*  @param[in]   settings	cfg80211_ap_settings parameters for the beacon to be added
*  @return 	int : Return 0 on Success.
*  @author	austin
*  @date	23 JUL 2013	
*  @version	1.0
*/
static int NMI_WFI_start_ap(struct wiphy *wiphy, struct net_device *dev,
	struct cfg80211_ap_settings *settings)
{
	struct cfg80211_beacon_data* beacon = &(settings->beacon);
	struct NMI_WFI_priv* priv;
	NMI_Sint32 s32Error = NMI_SUCCESS;
	
	priv = wiphy_priv(wiphy);
	PRINT_D(HOSTAPD_DBG,"Starting ap\n");

	PRINT_D(HOSTAPD_DBG,"Interval = %d \n DTIM period = %d\n Head length = %d Tail length = %d\n",
		settings->beacon_interval , settings->dtim_period, beacon->head_len, beacon->tail_len );

	#ifndef NMI_FULLY_HOSTING_AP
	s32Error = host_int_add_beacon(	priv->hNMIWFIDrv,
									settings->beacon_interval,
									settings->dtim_period,
									beacon->head_len, beacon->head,
									beacon->tail_len, beacon->tail);
	#else
	s32Error = host_add_beacon(	priv->hNMIWFIDrv,
									settings->beacon_interval,
									settings->dtim_period,
									beacon->head_len, beacon->head,
									beacon->tail_len, beacon->tail);
	#endif

	_g_pApSettings = settings;

	return s32Error;
}

/**
*  @brief 	NMI_WFI_change_beacon
*  @details 	Add a beacon with given parameters, @head, @interval
*      		and @dtim_period will be valid, @tail is optional.
*  @param[in]   wiphy
*  @param[in]   dev	The net device structure
*  @param[in]   beacon	cfg80211_beacon_data for the beacon to be changed
*  @return 	int : Return 0 on Success.
*  @author	austin
*  @date	23 JUL 2013	
*  @version	1.0
*/
static int  NMI_WFI_change_beacon(struct wiphy *wiphy, struct net_device *dev,
	struct cfg80211_beacon_data *beacon)
{
	struct cfg80211_ap_settings *settings = _g_pApSettings;
	struct NMI_WFI_priv* priv;
	NMI_Sint32 s32Error = NMI_SUCCESS;
	
	priv = wiphy_priv(wiphy);
	PRINT_D(HOSTAPD_DBG,"Setting beacon\n");
	
	// should be call after start_ap.
	if (!settings)
		return NMI_INVALID_STATE;

#ifndef NMI_FULLY_HOSTING_AP
	s32Error = host_int_add_beacon(	priv->hNMIWFIDrv,
									settings->beacon_interval,
									settings->dtim_period,
									beacon->head_len, beacon->head,
									beacon->tail_len, beacon->tail);
#else
	s32Error = host_add_beacon(	priv->hNMIWFIDrv,
									settings->beacon_interval,
									settings->dtim_period,
									beacon->head_len, beacon->head,
									beacon->tail_len, beacon->tail);
#endif

	return s32Error;
}

/**
*  @brief 	NMI_WFI_stop_ap
*  @details 	Remove beacon configuration and stop sending the beacon.
*  @param[in]   
*  @return 	int : Return 0 on Success.
*  @author	austin
*  @date	23 JUL 2013	
*  @version	1.0
*/
static int  NMI_WFI_stop_ap(struct wiphy *wiphy, struct net_device *dev)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	struct NMI_WFI_priv* priv;
	
	
	NMI_NULLCHECK(s32Error, wiphy);
	
	priv = wiphy_priv(wiphy);

	PRINT_D(HOSTAPD_DBG,"Deleting beacon\n");
	
	#ifndef NMI_FULLY_HOSTING_AP
	s32Error = host_int_del_beacon(priv->hNMIWFIDrv);
	#else
	s32Error = host_del_beacon(priv->hNMIWFIDrv);
	#endif 

	_g_pApSettings = NULL;	// better to reset primitive regardless of error.
	
	NMI_ERRORCHECK(s32Error);
	
	NMI_CATCH(s32Error)
	{
	}
	return s32Error;
}

#else /* here belows are original for < kernel 3.3 (austin.2013-07-23) */

/**
*  @brief 	NMI_WFI_add_beacon
*  @details 	Add a beacon with given parameters, @head, @interval
*      		and @dtim_period will be valid, @tail is optional.
*  @param[in]   wiphy
*  @param[in]   dev	The net device structure
*  @param[in]   info	Parameters for the beacon to be added
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_add_beacon(struct wiphy *wiphy, struct net_device *dev,
	struct beacon_parameters *info)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	struct NMI_WFI_priv* priv;

	

	priv = wiphy_priv(wiphy);
	PRINT_D(HOSTAPD_DBG,"Adding Beacon\n");

	PRINT_D(HOSTAPD_DBG,"Interval = %d \n DTIM period = %d\n Head length = %d Tail length = %d\n",info->interval , info->dtim_period,info->head_len,info->tail_len );

	#ifndef NMI_FULLY_HOSTING_AP
	s32Error = host_int_add_beacon(priv->hNMIWFIDrv, info->interval,
									 info->dtim_period,
									 info->head_len, info->head,
									 info->tail_len, info->tail);

	#else
	s32Error = host_add_beacon(priv->hNMIWFIDrv, info->interval,
									 info->dtim_period,
									 info->head_len, info->head,
									 info->tail_len, info->tail);
	#endif

	return s32Error;
}

/**
*  @brief 	NMI_WFI_set_beacon
*  @details 	Change the beacon parameters for an access point mode
*      		interface. This should reject the call when no beacon has been
*      		configured.
*  @param[in]   
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int  NMI_WFI_set_beacon(struct wiphy *wiphy, struct net_device *dev,
	struct beacon_parameters *info)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;


	PRINT_D(HOSTAPD_DBG,"Setting beacon\n");

	s32Error = NMI_WFI_add_beacon(wiphy, dev, info);

	return s32Error;
}

/**
*  @brief 	NMI_WFI_del_beacon
*  @details 	Remove beacon configuration and stop sending the beacon.
*  @param[in]   
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int  NMI_WFI_del_beacon(struct wiphy *wiphy, struct net_device *dev)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	struct NMI_WFI_priv* priv;
	
	
	NMI_NULLCHECK(s32Error, wiphy);
	
	priv = wiphy_priv(wiphy);

	PRINT_D(HOSTAPD_DBG,"Deleting beacon\n");
	
	#ifndef NMI_FULLY_HOSTING_AP
	s32Error = host_int_del_beacon(priv->hNMIWFIDrv);
	#else
	s32Error = host_del_beacon(priv->hNMIWFIDrv);
	#endif 
	
	NMI_ERRORCHECK(s32Error);
	
	NMI_CATCH(s32Error)
	{
	}
	return s32Error;
}

#endif	/* linux kernel 3.4+ (austin.2013-07-23) */

/**
*  @brief 	NMI_WFI_add_station
*  @details 	Add a new station.
*  @param[in]   
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int  NMI_WFI_add_station(struct wiphy *wiphy, struct net_device *dev,
	u8 *mac, struct station_parameters *params)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	struct NMI_WFI_priv* priv;
	tstrNMI_AddStaParam strStaParams={{0}};
	linux_wlan_t* nic;

	
	NMI_NULLCHECK(s32Error, wiphy);
	
	priv = wiphy_priv(wiphy);
	nic = netdev_priv(dev);

	if(nic->iftype == AP_MODE)
	{
		#ifndef NMI_FULLY_HOSTING_AP

		NMI_memcpy(strStaParams.au8BSSID, mac, ETH_ALEN);
		NMI_memcpy(priv->assoc_stainfo.au8Sta_AssociatedBss[params->aid],mac,ETH_ALEN);
		strStaParams.u16AssocID = params->aid;
		strStaParams.u8NumRates = params->supported_rates_len;
		strStaParams.pu8Rates = params->supported_rates;

		PRINT_D(CFG80211_DBG,"Adding station parameters %d\n",params->aid);

		PRINT_D(CFG80211_DBG,"BSSID = %x%x%x%x%x%x\n",priv->assoc_stainfo.au8Sta_AssociatedBss[params->aid][0],priv->assoc_stainfo.au8Sta_AssociatedBss[params->aid][1],priv->assoc_stainfo.au8Sta_AssociatedBss[params->aid][2],priv->assoc_stainfo.au8Sta_AssociatedBss[params->aid][3],priv->assoc_stainfo.au8Sta_AssociatedBss[params->aid][4],
			priv->assoc_stainfo.au8Sta_AssociatedBss[params->aid][5]);
		PRINT_D(HOSTAPD_DBG,"ASSOC ID = %d\n",strStaParams.u16AssocID);
		PRINT_D(HOSTAPD_DBG,"Number of supported rates = %d\n",strStaParams.u8NumRates);

		if(params->ht_capa == NMI_NULL)
		{
			strStaParams.bIsHTSupported = NMI_FALSE;
		}
		else
		{
			strStaParams.bIsHTSupported = NMI_TRUE;
			strStaParams.u16HTCapInfo = params->ht_capa->cap_info;
			strStaParams.u8AmpduParams = params->ht_capa->ampdu_params_info;
			NMI_memcpy(strStaParams.au8SuppMCsSet, &params->ht_capa->mcs, NMI_SUPP_MCS_SET_SIZE);
			strStaParams.u16HTExtParams = params->ht_capa->extended_ht_cap_info;
			strStaParams.u32TxBeamformingCap = params->ht_capa->tx_BF_cap_info;
			strStaParams.u8ASELCap = params->ht_capa->antenna_selection_info;	
		}

		strStaParams.u16FlagsMask = params->sta_flags_mask;
		strStaParams.u16FlagsSet = params->sta_flags_set;

		PRINT_D(HOSTAPD_DBG,"IS HT supported = %d\n", strStaParams.bIsHTSupported);
		PRINT_D(HOSTAPD_DBG,"Capability Info = %d\n", strStaParams.u16HTCapInfo);
		PRINT_D(HOSTAPD_DBG,"AMPDU Params = %d\n",strStaParams.u8AmpduParams);
		PRINT_D(HOSTAPD_DBG,"HT Extended params = %d\n",strStaParams.u16HTExtParams);
		PRINT_D(HOSTAPD_DBG,"Tx Beamforming Cap = %d\n",strStaParams.u32TxBeamformingCap);
		PRINT_D(HOSTAPD_DBG,"Antenna selection info = %d\n",strStaParams.u8ASELCap);
		PRINT_D(HOSTAPD_DBG,"Flag Mask = %d\n",strStaParams.u16FlagsMask);
		PRINT_D(HOSTAPD_DBG,"Flag Set = %d\n",strStaParams.u16FlagsSet);

		s32Error = host_int_add_station(priv->hNMIWFIDrv, &strStaParams);
		NMI_ERRORCHECK(s32Error);

		#else
		PRINT_D(CFG80211_DBG,"Adding station parameters %d\n",params->aid);
		NMI_memcpy(priv->assoc_stainfo.au8Sta_AssociatedBss[params->aid],mac,ETH_ALEN);
		
		PRINT_D(CFG80211_DBG,"BSSID = %x%x%x%x%x%x\n",priv->assoc_stainfo.au8Sta_AssociatedBss[params->aid][0],priv->assoc_stainfo.au8Sta_AssociatedBss[params->aid][1],priv->assoc_stainfo.au8Sta_AssociatedBss[params->aid][2],priv->assoc_stainfo.au8Sta_AssociatedBss[params->aid][3],priv->assoc_stainfo.au8Sta_AssociatedBss[params->aid][4],
			priv->assoc_stainfo.au8Sta_AssociatedBss[params->aid][5]);
		
		NMI_AP_AddSta(mac, params);
		NMI_ERRORCHECK(s32Error);
		#endif //NMI_FULLY_HOSTING_AP
		
	}
	
	NMI_CATCH(s32Error)
	{
	}
	return s32Error;
}

/**
*  @brief 	NMI_WFI_del_station
*  @details 	Remove a station; @mac may be NULL to remove all stations.
*  @param[in]   
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_del_station(struct wiphy *wiphy, struct net_device *dev,
	u8 *mac)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	struct NMI_WFI_priv* priv;
	linux_wlan_t* nic;
	

	NMI_NULLCHECK(s32Error, wiphy);
	/*BugID_4795: mac may be null pointer to indicate deleting all stations, so avoid null check*/
	//NMI_NULLCHECK(s32Error, mac);
	
	priv = wiphy_priv(wiphy);
	nic = netdev_priv(dev);

	if(nic->iftype == AP_MODE)
	{
		PRINT_D(HOSTAPD_DBG,"Deleting station\n");

		
		if(mac == NMI_NULL)
			PRINT_D(HOSTAPD_DBG,"All associated stations \n");
		else
			PRINT_D(HOSTAPD_DBG,"With mac address: %x%x%x%x%x%x\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

		#ifndef NMI_FULLY_HOSTING_AP
		s32Error = host_int_del_station(priv->hNMIWFIDrv , mac);
		#else		
		NMI_AP_RemoveSta(mac);
		#endif //NMI_FULLY_HOSTING_AP
		NMI_ERRORCHECK(s32Error);
	}
	NMI_CATCH(s32Error)
	{
	}
	return s32Error;
}

/**
*  @brief 	NMI_WFI_change_station
*  @details 	Modify a given station.
*  @param[in]   
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
static int NMI_WFI_change_station(struct wiphy *wiphy, struct net_device *dev,
		u8 *mac, struct station_parameters *params)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	struct NMI_WFI_priv* priv;
	tstrNMI_AddStaParam strStaParams={{0}};
	linux_wlan_t* nic;


	PRINT_D(HOSTAPD_DBG,"Change station paramters\n");
	
	NMI_NULLCHECK(s32Error, wiphy);
	
	priv = wiphy_priv(wiphy);
	nic = netdev_priv(dev);

	if(nic->iftype == AP_MODE)
	{
		#ifndef NMI_FULLY_HOSTING_AP
		
		NMI_memcpy(strStaParams.au8BSSID, mac, ETH_ALEN);
		strStaParams.u16AssocID = params->aid;
		strStaParams.u8NumRates = params->supported_rates_len;
		strStaParams.pu8Rates = params->supported_rates;

		PRINT_D(HOSTAPD_DBG,"BSSID = %x%x%x%x%x%x\n",strStaParams.au8BSSID[0],strStaParams.au8BSSID[1],strStaParams.au8BSSID[2],strStaParams.au8BSSID[3],strStaParams.au8BSSID[4],
			strStaParams.au8BSSID[5]);
		PRINT_D(HOSTAPD_DBG,"ASSOC ID = %d\n",strStaParams.u16AssocID);
		PRINT_D(HOSTAPD_DBG,"Number of supported rates = %d\n",strStaParams.u8NumRates);
		
		if(params->ht_capa == NMI_NULL)
		{
			strStaParams.bIsHTSupported = NMI_FALSE;
		}
		else
		{
			strStaParams.bIsHTSupported = NMI_TRUE;
			strStaParams.u16HTCapInfo = params->ht_capa->cap_info;
			strStaParams.u8AmpduParams = params->ht_capa->ampdu_params_info;
			NMI_memcpy(strStaParams.au8SuppMCsSet, &params->ht_capa->mcs, NMI_SUPP_MCS_SET_SIZE);
			strStaParams.u16HTExtParams = params->ht_capa->extended_ht_cap_info;
			strStaParams.u32TxBeamformingCap = params->ht_capa->tx_BF_cap_info;
			strStaParams.u8ASELCap = params->ht_capa->antenna_selection_info;
			
		}

		strStaParams.u16FlagsMask = params->sta_flags_mask;
		strStaParams.u16FlagsSet = params->sta_flags_set;
		
		PRINT_D(HOSTAPD_DBG,"IS HT supported = %d\n", strStaParams.bIsHTSupported);
		PRINT_D(HOSTAPD_DBG,"Capability Info = %d\n", strStaParams.u16HTCapInfo);
		PRINT_D(HOSTAPD_DBG,"AMPDU Params = %d\n",strStaParams.u8AmpduParams);
		PRINT_D(HOSTAPD_DBG,"HT Extended params = %d\n",strStaParams.u16HTExtParams);
		PRINT_D(HOSTAPD_DBG,"Tx Beamforming Cap = %d\n",strStaParams.u32TxBeamformingCap);
		PRINT_D(HOSTAPD_DBG,"Antenna selection info = %d\n",strStaParams.u8ASELCap);
		PRINT_D(HOSTAPD_DBG,"Flag Mask = %d\n",strStaParams.u16FlagsMask);
		PRINT_D(HOSTAPD_DBG,"Flag Set = %d\n",strStaParams.u16FlagsSet);

		s32Error = host_int_edit_station(priv->hNMIWFIDrv, &strStaParams);
		NMI_ERRORCHECK(s32Error);

		#else	
		NMI_AP_EditSta(mac, params);
		NMI_ERRORCHECK(s32Error);
		#endif //NMI_FULLY_HOSTING_AP
		
	}
	NMI_CATCH(s32Error)
	{
	}
	return s32Error;
}


/**
*  @brief 	NMI_WFI_add_virt_intf
*  @details
*  @param[in]
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 JUL 2012
*  @version	1.0
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,7,0)		/* tony for v3.8 support */
struct wireless_dev * NMI_WFI_add_virt_intf(struct wiphy *wiphy, const char *name, 
				enum nl80211_iftype type, u32 *flags, 
				struct vif_params *params)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)	/* tony for v3.6 support */
struct wireless_dev * NMI_WFI_add_virt_intf(struct wiphy *wiphy, char *name,
                           	enum nl80211_iftype type, u32 *flags,
                           	struct vif_params *params) 
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
int NMI_WFI_add_virt_intf(struct wiphy *wiphy, char *name,
               		enum nl80211_iftype type, u32 *flags,
                      	struct vif_params *params)
#else
struct net_device *NMI_WFI_add_virt_intf(struct wiphy *wiphy, char *name,
  	                        enum nl80211_iftype type, u32 *flags,
        	                struct vif_params *params)
#endif
{
	linux_wlan_t *nic;
	struct NMI_WFI_priv* priv;
	//struct NMI_WFI_mon_priv* mon_priv;
	NMI_Sint32 s32Error = NMI_SUCCESS;
	priv = wiphy_priv(wiphy);

	
	
	PRINT_D(HOSTAPD_DBG,"Adding monitor interface\n");

	nic = netdev_priv(priv->wdev->netdev);
	 

	if(type == NL80211_IFTYPE_MONITOR)
	{
		PRINT_D(HOSTAPD_DBG,"Monitor interface mode: Initializing mon interface virtual device driver\n");
		
		s32Error = NMI_WFI_init_mon_interface(name,nic->nmc_netdev);
		if(!s32Error)
		{
			PRINT_D(HOSTAPD_DBG,"Setting monitor flag in private structure\n");
			#ifdef SIMULATION
			priv = netdev_priv(priv->wdev->netdev);
			priv->monitor_flag = 1;
			#else
			nic = netdev_priv(priv->wdev->netdev);
			nic->monitor_flag = 1; 
			#endif
		}
		else
			PRINT_ER("Error in initializing monitor interface\n ");
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)	/* tony for v3.8 support */
	return priv->wdev;
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
	return s32Error;
#else
	return priv->wdev->netdev;
#endif
	
}

/**
*  @brief 	NMI_WFI_del_virt_intf
*  @details
*  @param[in]
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 JUL 2012
*  @version	1.0
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)
int NMI_WFI_del_virt_intf(struct wiphy *wiphy, struct wireless_dev *wdev)	/* tony for v3.8 support */
#else
int NMI_WFI_del_virt_intf(struct wiphy *wiphy,struct net_device *dev)
#endif
{
	PRINT_D(HOSTAPD_DBG,"Deleting virtual interface\n");
		return NMI_SUCCESS;
}



#endif /*NMI_AP_EXTERNAL_MLME*/
static struct cfg80211_ops NMI_WFI_cfg80211_ops = {

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)
	/* 
	*	replaced set_channel by set_monitor_channel 
	*	from v3.6
	*	tony, 2013-10-29
	*/
	.set_monitor_channel = NMI_WFI_CfgSetChannel,
#else
	.set_channel = NMI_WFI_CfgSetChannel,	
#endif
	.scan = NMI_WFI_CfgScan,
	.connect = NMI_WFI_CfgConnect,
	.disconnect = NMI_WFI_disconnect,
	.add_key = NMI_WFI_add_key,
	.del_key = NMI_WFI_del_key,
	.get_key = NMI_WFI_get_key,
	.set_default_key = NMI_WFI_set_default_key,
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32)
	//.dump_survey = NMI_WFI_dump_survey,
#endif
	#ifdef NMI_AP_EXTERNAL_MLME
	.add_virtual_intf = NMI_WFI_add_virt_intf,
	.del_virtual_intf = NMI_WFI_del_virt_intf,
	.change_virtual_intf = NMI_WFI_change_virt_intf,
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
	.add_beacon = NMI_WFI_add_beacon,
	.set_beacon = NMI_WFI_set_beacon,
	.del_beacon = NMI_WFI_del_beacon,
#else
	/* supports kernel 3.4+ change. austin.2013-07-23 */
	.start_ap = NMI_WFI_start_ap,
	.change_beacon = NMI_WFI_change_beacon,
	.stop_ap = NMI_WFI_stop_ap,
#endif
	.add_station = NMI_WFI_add_station,
	.del_station = NMI_WFI_del_station,
	.change_station = NMI_WFI_change_station,
	#endif /* NMI_AP_EXTERNAL_MLME*/
	#ifndef NMI_FULLY_HOSTING_AP
	.get_station = NMI_WFI_get_station,
	#endif
	.dump_station = NMI_WFI_dump_station,
	.change_bss = NMI_WFI_change_bss,
	//.auth = NMI_WFI_auth,
	//.assoc = NMI_WFI_assoc,
	//.deauth = NMI_WFI_deauth,
	//.disassoc = NMI_WFI_disassoc,
	.set_wiphy_params = NMI_WFI_set_wiphy_params,
	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32)
	.set_bitrate_mask = NMI_WFI_set_bitrate_mask,
	.set_pmksa = NMI_WFI_set_pmksa,
	.del_pmksa = NMI_WFI_del_pmksa,
	.flush_pmksa = NMI_WFI_flush_pmksa,
#ifdef NMI_P2P
	.remain_on_channel = NMI_WFI_remain_on_channel,
	.cancel_remain_on_channel = NMI_WFI_cancel_remain_on_channel,
	.mgmt_tx_cancel_wait = NMI_WFI_mgmt_tx_cancel_wait,
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)
	.action = NMI_WFI_action,
	#endif
	#else
	.mgmt_tx = NMI_WFI_mgmt_tx,
	.mgmt_frame_register = NMI_WFI_frame_register,
	#endif
#endif
	//.mgmt_tx_cancel_wait = NMI_WFI_mgmt_tx_cancel_wait,
	.set_power_mgmt = NMI_WFI_set_power_mgmt,
	.set_cqm_rssi_config = NMI_WFI_set_cqm_rssi_config,
#endif
	
};





/**
*  @brief 	NMI_WFI_update_stats
*  @details 	Modify parameters for a given BSS.
*  @param[in]   
*  @return 	int : Return 0 on Success.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0NMI_WFI_set_cqmNMI_WFI_set_cqm_rssi_configNMI_WFI_set_cqm_rssi_configNMI_WFI_set_cqm_rssi_configNMI_WFI_set_cqm_rssi_config_rssi_config
*/
int NMI_WFI_update_stats(struct wiphy *wiphy, u32 pktlen , u8 changed)
{

	struct NMI_WFI_priv *priv;
	
	priv = wiphy_priv(wiphy);
	//NMI_SemaphoreAcquire(&SemHandleUpdateStats,NULL);
#if 1
	switch(changed)
	{
	
		case NMI_WFI_RX_PKT:
		{
			//MI_PRINTF("In Rx Receive Packet\n");
			priv->netstats.rx_packets++;
			priv->netstats.rx_bytes += pktlen;
			priv->netstats.rx_time = get_jiffies_64();
		}
		break;
		case NMI_WFI_TX_PKT:
		{
			//NMI_PRINTF("In Tx Receive Packet\n");
			priv->netstats.tx_packets++;
			priv->netstats.tx_bytes += pktlen;
			priv->netstats.tx_time = get_jiffies_64();
	
		}
		break;

		default:
			break;
	}
	//NMI_SemaphoreRelease(&SemHandleUpdateStats,NULL);
#endif
	return 0;
}
/**
*  @brief 	NMI_WFI_InitPriv
*  @details 	Initialization of the net device, private data
*  @param[in]   NONE
*  @return 	NONE
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/
void NMI_WFI_InitPriv(struct net_device *dev)
{

	struct NMI_WFI_priv *priv;
	priv = netdev_priv(dev);

	priv->netstats.rx_packets = 0;
	priv->netstats.tx_packets = 0;
	priv->netstats.rx_bytes = 0;
	priv->netstats.rx_bytes = 0;
	priv->netstats.rx_time = 0;
	priv->netstats.tx_time = 0;


}
/**
*  @brief 	NMI_WFI_CfgAlloc
*  @details 	Allocation of the wireless device structure and assigning it 
*		to the cfg80211 operations structure.
*  @param[in]   NONE
*  @return 	wireless_dev : Returns pointer to wireless_dev structure.
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/  
struct wireless_dev* NMI_WFI_CfgAlloc(void)
{

	struct wireless_dev *wdev;


	PRINT_D(CFG80211_DBG,"Allocating wireless device\n");
 	/*Allocating the wireless device structure*/
 	wdev = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
	if (!wdev)
	{
		PRINT_ER("Cannot allocate wireless device\n");
		goto _fail_;
	}

	/*Creating a new wiphy, linking wireless structure with the wiphy structure*/
	wdev->wiphy = wiphy_new(&NMI_WFI_cfg80211_ops, sizeof(struct NMI_WFI_priv));
	if (!wdev->wiphy) 
	{
		PRINT_ER("Cannot allocate wiphy\n");
		goto _fail_mem_;
		
	}

	#ifdef NMI_AP_EXTERNAL_MLME
	//enable 802.11n HT
	NMI_WFI_band_2ghz.ht_cap.ht_supported = 1;
	NMI_WFI_band_2ghz.ht_cap.cap |= (1 << IEEE80211_HT_CAP_RX_STBC_SHIFT);
	NMI_WFI_band_2ghz.ht_cap.mcs.rx_mask[0] = 0xff;
	NMI_WFI_band_2ghz.ht_cap.ampdu_factor = IEEE80211_HT_MAX_AMPDU_8K;
	NMI_WFI_band_2ghz.ht_cap.ampdu_density = IEEE80211_HT_MPDU_DENSITY_NONE;
	#endif
		
	/*wiphy bands*/
	wdev->wiphy->bands[IEEE80211_BAND_2GHZ]= &NMI_WFI_band_2ghz;

	return wdev;

_fail_mem_:
	kfree(wdev);
_fail_:		
	return NULL;

} 
/**
*  @brief 	NMI_WFI_WiphyRegister
*  @details 	Registering of the wiphy structure and interface modes
*  @param[in]   NONE
*  @return 	NONE
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/  

struct wireless_dev* NMI_WFI_WiphyRegister(struct net_device *net)
{
	struct NMI_WFI_priv *priv;
	struct wireless_dev *wdev;
	NMI_Sint32 s32Error = NMI_SUCCESS;

	PRINT_D(CFG80211_DBG,"Registering wifi device\n");
	
	wdev = NMI_WFI_CfgAlloc();
	if(wdev == NULL){
		PRINT_ER("CfgAlloc Failed\n");
		return NULL;
		}

	NMI_SemaphoreCreate(&SemHandleUpdateStats,NULL);
	/*Return hardware description structure (wiphy)'s priv*/
	priv = wdev_priv(wdev);

	/*Added by Amr - BugID_4793*/
	priv->u8CurrChannel = -1;

	/*Link the wiphy with wireless structure*/
	priv->wdev = wdev;

	/*Maximum number of probed ssid to be added by user for the scan request*/
	wdev->wiphy->max_scan_ssids = MAX_NUM_PROBED_SSID;  
	#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32)
	/*Maximum number of pmkids to be cashed*/
	wdev->wiphy->max_num_pmkids = NMI_MAX_NUM_PMKIDS;
	PRINT_INFO(CFG80211_DBG,"Max number of PMKIDs = %d\n",wdev->wiphy->max_num_pmkids);
	#endif

	wdev->wiphy->max_scan_ie_len = 1000;

	/*signal strength in mBm (100*dBm) */
	wdev->wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;

	/*Set the availaible cipher suites*/
	wdev->wiphy->cipher_suites = cipher_suites;
	wdev->wiphy->n_cipher_suites = ARRAY_SIZE(cipher_suites);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)	
	/*Setting default managment types: for register action frame:  */
	wdev->wiphy->mgmt_stypes = nmi_wfi_cfg80211_mgmt_types;
#endif

#ifdef NMI_P2P
	wdev->wiphy->max_remain_on_channel_duration = 5000;
	/*Setting the wiphy interfcae mode and type before registering the wiphy*/
	wdev->wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) | BIT(NL80211_IFTYPE_AP) | BIT(NL80211_IFTYPE_MONITOR) | BIT(NL80211_IFTYPE_P2P_GO) |
		BIT(NL80211_IFTYPE_P2P_CLIENT);
#else
	wdev->wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) | BIT(NL80211_IFTYPE_AP) | BIT(NL80211_IFTYPE_MONITOR);
#endif
	wdev->iftype = NL80211_IFTYPE_STATION;



	PRINT_INFO(CFG80211_DBG,"Max scan ids = %d,Max scan IE len = %d,Signal Type = %d,Interface Modes = %d,Interface Type = %d\n",
			wdev->wiphy->max_scan_ssids,wdev->wiphy->max_scan_ie_len,wdev->wiphy->signal_type,
			wdev->wiphy->interface_modes, wdev->iftype);
    
    set_wiphy_dev(wdev->wiphy, &local_sdio_func->dev); //tony
	/*Register wiphy structure*/
	s32Error = wiphy_register(wdev->wiphy);
	if (s32Error){
		PRINT_ER("Cannot register wiphy device\n");
		/*should define what action to be taken in such failure*/
		}
	else
	{
		PRINT_D(CFG80211_DBG,"Successful Registering\n");
	}

#if 0	
	/*wdev[i]->wiphy->interface_modes =
			BIT(NL80211_IFTYPE_AP);
		wdev[i]->iftype = NL80211_IFTYPE_AP;
	*/

	/*Pointing the priv structure the netdev*/
    priv= netdev_priv(net);

	/*linking the wireless_dev structure with the netdevice*/
	priv->dev->ieee80211_ptr = wdev;
	priv->dev->ml_priv = priv;
	wdev->netdev = priv->dev;
#endif
	priv->dev = net;
#if 0
	ret = host_int_init(&priv->hNMIWFIDrv);
	if(ret)
	{
		NMI_PRINTF("Error Init Driver\n");
	}
#endif
	return wdev;


}
/**
*  @brief 	NMI_WFI_WiphyFree
*  @details 	Freeing allocation of the wireless device structure
*  @param[in]   NONE
*  @return 	NONE
*  @author	mdaftedar
*  @date	01 MAR 2012
*  @version	1.0
*/
int NMI_WFI_InitHostInt(struct net_device *net)
{

	NMI_Sint32 s32Error = NMI_SUCCESS;
	
	struct NMI_WFI_priv *priv;

	tstrNMI_SemaphoreAttrs strSemaphoreAttrs;
	s32Error = NMI_TimerCreate(&hAgingTimer, remove_network_from_shadow, NMI_NULL);

	if(s32Error < 0){
		PRINT_ER("Failed to creat refresh Timer\n");
		return s32Error;		
	}

	NMI_SemaphoreFillDefault(&strSemaphoreAttrs);	

	/////////////////////////////////////////
	//strSemaphoreAttrs.u32InitCount = 0;
	NMI_SemaphoreCreate(&hSemScanReq, &strSemaphoreAttrs);

	gbAutoRateAdjusted = NMI_FALSE;
	
	priv = wdev_priv(net->ieee80211_ptr);
	s32Error = host_int_init(&priv->hNMIWFIDrv);
	if(s32Error)
	{
		PRINT_ER("Error while initializing hostinterface\n");
	}
	return s32Error;
}

/**
*  @brief 	NMI_WFI_WiphyFree
*  @details 	Freeing allocation of the wireless device structure
*  @param[in]   NONE
*  @return 	NONE
*  @author	mdaftedar
*  @date	01 MAR 2012
*  @version	1.0
*/
int NMI_WFI_DeInitHostInt(struct net_device *net)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;

	struct NMI_WFI_priv *priv;
	priv = wdev_priv(net->ieee80211_ptr);



	/* Clear the Shadow scan */
	clear_shadow_scan(priv);

	NMI_SemaphoreDestroy(&hSemScanReq,NULL);

	gbAutoRateAdjusted = NMI_FALSE;
	
	s32Error = host_int_deinit(priv->hNMIWFIDrv);
	if(s32Error)
	{
		PRINT_ER("Error while deintializing host interface\n");
	}
	return s32Error;
}


/**
*  @brief 	NMI_WFI_WiphyFree
*  @details 	Freeing allocation of the wireless device structure
*  @param[in]   NONE
*  @return 	NONE
*  @author	mdaftedar
*  @date	01 MAR 2012	
*  @version	1.0
*/  
void NMI_WFI_WiphyFree(struct net_device *net)
{
	
	PRINT_D(CFG80211_DBG,"Unregistering wiphy\n");

	if(net == NULL){
		PRINT_D(INIT_DBG,"net_device is NULL\n");
		return;
		}

	if(net->ieee80211_ptr == NULL){
		PRINT_D(INIT_DBG,"ieee80211_ptr is NULL\n");
		return;
		}
	
	if(net->ieee80211_ptr->wiphy == NULL){
		PRINT_D(INIT_DBG,"wiphy is NULL\n");
		return;
		}

	wiphy_unregister(net->ieee80211_ptr->wiphy);
	
	PRINT_D(INIT_DBG,"Freeing wiphy\n");
	wiphy_free(net->ieee80211_ptr->wiphy);
	kfree(net->ieee80211_ptr);

}
