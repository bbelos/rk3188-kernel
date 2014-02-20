/*!  
*  @file	NMI_host_AP.c
*  @brief	code related to AP mode on driver
*  @author	Abd Al-Rahman Diab
*  @date	09 APRIL 2013
*  @version	1.0
*/



#ifndef SIMULATION
#include "linux_wlan_common.h"
#endif
#include "NMI_OsWrapper/include/NMI_OSWrapper.h"
#include "NMI_host_AP.h"
#ifdef NMI_FULLY_HOSTING_AP


extern void NMI_WFI_monitor_rx(uint8_t *buff, uint32_t size);
void linux_wlan_free(void* vp);

struct tx_complete_data{
	struct tx_complete_data* next;
	int size;
	void* buff;
	struct sk_buff *skb;	
};

/*****************************************************************************/
/* Static Global Variables                                                   */
/*****************************************************************************/

static const NMI_Uint8  g_bmap[8] = {1, 2, 4, 8, 16, 32, 64, 128}; /* Bit map */

/*****************************************************************************/
/* Global variables                                                          */
/*****************************************************************************/

table_t g_sta_table                         = {0,};
beacon_info strBeaconInfo = {0};
NMI_Uint8   g_snap_header[SNAP_HDR_ID_LEN] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00};

/* Power management related globals */
NMI_Uint8       g_num_sta_ps              = 0; /* Num of STA in PS */
NMI_Uint32      g_num_mc_bc_pkt           = 0; /* Num of BC/MC pkts in PSQ     */
NMI_Uint32		 g_num_ps_pkt     		   = 0; /* Num of UC/BC/MC pkts in PSQ  */
NMI_Uint16      g_num_mc_bc_qd_pkt    	   = 0; /* Num of BC/MC pkts in Hw  	*/
list_buff_t  g_mc_q;
//NMI_Uint8       g_vbmap[VBMAP_SIZE]       = {0,};
//NMI_Uint8       g_tim_element_index       = 0;
//NMI_Uint16      g_tim_element_trailer_len = 0;
q_head_t     g_ps_pending_q            = {0,};
NMI_Uint16      g_aging_thresh_in_sec     = AGING_THR_IN_SEC;
NMI_ieee80211_sta *g_max_ps_ae = NULL;      /* Association entry of STA, with max num of PS pkts */


void* find_entry(NMI_Uint8* key);
void add_entry(void* entry, NMI_Uint8* key);
void delete_entry(NMI_Uint8* key);
void frmw_to_linux(uint8_t *buff, uint32_t size);
void ap_enabled_rx_data( NMI_Uint8 *msg);
REQUEUE_STATUS_T requeue_ps_packet(NMI_ieee80211_sta* ae, list_buff_t *qh,
                                   NMI_Bool ps_q_legacy, NMI_Bool eosp);
NMI_Bool buffer_tx_packet(NMI_Uint8 *entry, NMI_Uint8 *da, NMI_Uint8 priority,
                         struct tx_complete_data *tx_dscr);
void handle_ps_tx_comp_ap(struct tx_complete_data *tx_dscr );
void check_and_reset_tim_bit(NMI_Uint16 asoc_id);
NMI_Bool handle_ps_poll(wlan_rx_t    *wlan_rx );


/* This function returns whether the Multicast bit is set in the given beacon*/
/* frame                                                                     */
inline NMI_Bool get_mc_bit_bcn(void)
{
    NMI_Bool ret_value;
    ret_value = (NMI_Bool)(strBeaconInfo.u8beacon_frame[strBeaconInfo.u8tim_element_index + BMAP_CTRL_OFFSET] &
            0x01);
    return (ret_value);
}

/* This function returns whether the Multicast bit is set in the given beacon*/
/* frame                                                                     */
inline void reset_mc_bit_bcn(void)
{
    strBeaconInfo.u8beacon_frame[strBeaconInfo.u8tim_element_index + TIM_OFFSET] &= 0xFE;
    strBeaconInfo.u8beacon_frame[strBeaconInfo.u8tim_element_index + BMAP_CTRL_OFFSET] &= 0xFE;
} 


/*
*  @brief 			NMI_beacon_tx_complete
*  @details 	   	call back function for beacon transmission through vmm , does nothing at the moment
*  @return 	    	
*  @author		Abd Al-Rahman Diab
*  @date			09 APRIL 2013
*  @version		1.0
*/	
static void NMI_beacon_tx_complete(void* priv, int status)
{
	 // Do nothing 
	 
	/*
	 beacon_data* pv_data = (beacon_data*)priv;
	 NMI_Uint8 * buf=  pv_data->buff;

	if(status == 1)
	{
		if(INFO || buf[0] == 0x80 || buf[0] == 0xb0)
		PRINT_D(HOSTAPD_DBG,"Packet sent successfully - Size = %d - Address = %p.\n",pv_data->size,pv_data->buff);
	}
	else
	{
			PRINT_D(HOSTAPD_DBG,"Couldn't send packet - Size = %d - Address = %p.\n",pv_data->size,pv_data->buff);
	}
	*/
}


/*
*  @brief 			NMI_beacon_xmit
*  @details 	   	send beacon frame to firmware
*  @param[in]    	u8 *buf : pointer to the beacon frame
*				size_t len : beacon frame length
*  @return 	    	Error code.
*  @author		Abd Al-Rahman Diab
*  @date			09 APRIL 2013
*  @version		1.0
*/	
static int NMI_beacon_xmit(const u8 *buf, size_t len)
{
	beacon_data *strBeacon_tx =NULL;

	/* allocate the beacon_data struct */
	strBeacon_tx = (beacon_data*)kmalloc(sizeof(beacon_data),GFP_ATOMIC);
	if(strBeacon_tx == NULL){
		PRINT_ER("Failed to allocate memory for mgmt_tx structure\n");
	     	return NMI_FAIL;
	}

	/* allocate buffer for beacon data within the beacon_data struct */
	strBeacon_tx->buff= (char*)kmalloc(len,GFP_ATOMIC);
	if(strBeacon_tx->buff == NULL)
	{
		PRINT_ER("Failed to allocate memory for mgmt_tx buff\n");
	     	return NMI_FAIL;
	
	}

	/* fill in the beacon data stuct */
	memcpy(strBeacon_tx->buff,buf,len);
	strBeacon_tx->size=len;

	/* the actual transmission of beacon to firmware*/
	//NMI_PRINTF("--IN beacon_tx: Sending beacon Pkt to tx queue--\n");
	nmi_wlan_txq_add_mgmt_pkt(strBeacon_tx,strBeacon_tx->buff,strBeacon_tx->size,NMI_beacon_tx_complete);

	return 0;
}


/**
*  @brief 			host_add_beacon
*  @details 	   	Setting add beacon params in message queue 
*  @param[in] 	NMI_WFIDrvHandle hWFIDrv, NMI_Uint32 u32Interval,
			 	NMI_Uint32 u32DTIMPeriod,NMI_Uint32 u32HeadLen, NMI_Uint8* pu8Head,
			   	NMI_Uint32 u32TailLen, NMI_Uint8* pu8Tail
*  @return 	    	Error code.
*  @author		Abd Al-Rahman Diab
*  @date	
*  @version	1.0
*/
NMI_Sint32 host_add_beacon(NMI_WFIDrvHandle hWFIDrv, NMI_Uint32 u32Interval,
									 NMI_Uint32 u32DTIMPeriod,
									 NMI_Uint32 u32HeadLen, NMI_Uint8* pu8Head,
									 NMI_Uint32 u32TailLen, NMI_Uint8* pu8Tail)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	NMI_Bool bIsBeaconSet = NMI_FALSE;
	NMI_Uint8 *pu8BeaconFrame;
	NMI_Uint16  u16size=0;
	nmi_wlan_dev_t* strWlan = Get_wlan_context(&u16size); 

	if(u16size != sizeof(nmi_wlan_dev_t))
		PRINT_ER("size of nmi_wlan_dev_t in nmi_wlan != it's size in NMI_HOST_AP\n");


	//NMI_PRINTF("--IN host_add_beacon--\n");

	/* calculate beacon length */
	strBeaconInfo.u16beacon_len = u32HeadLen + u32TailLen + DEFAULT_TIM_LEN + 2 ;
	PRINT_D(HOSTAPD_DBG,"beacon_len=%d\n",strBeaconInfo.u16beacon_len);


	/* set beacon interval in chip */
	if(u32Interval>0)
	{
		strBeaconInfo.u16Beacon_Period = u32Interval;
		PRINT_D(HOSTAPD_DBG,"Beacon_Period=%d\n",strBeaconInfo.u16Beacon_Period );
		strWlan->hif_func.hif_write_reg(rMAC_BEACON_PERIOD, u32Interval);
		
	}

	/* allocate beacon frame */
	if(strBeaconInfo.u8beacon_frame == NULL)
	{
		PRINT_D(HOSTAPD_DBG,"beacon_frame wasn't allocated. allocating new buffers\n");
		/* Allocate 2 global beacon buffers as the beacon for AP may be modfied  */
		/* for which 2 buffers are needed.                                       */
		strBeaconInfo.u8beacon_frame  = (NMI_Uint8 *)kmalloc(strBeaconInfo.u16beacon_len,GFP_ATOMIC);
		if(strBeaconInfo.u8beacon_frame  == NULL)
		{
			PRINT_ER("Couldn't allocate beacon_frame\n");
			/* Exception - no memory for beacons */
			s32Error = NMI_FAIL;
			return s32Error;
		}
	}
	else
	{
		bIsBeaconSet = NMI_TRUE;
	}

	/* set beacon DTIM period in chip */
	if(u32DTIMPeriod>0)
	{
		strBeaconInfo.u8DTIMPeriod= u32DTIMPeriod;
		PRINT_D(HOSTAPD_DBG,"DTIMPeriod=%d\n",strBeaconInfo.u8DTIMPeriod);
		strWlan->hif_func.hif_write_reg(rMAC_DTIM_PERIOD, u32DTIMPeriod);
	}

	/* save TIM element location within the beacon */
	strBeaconInfo.u8tim_element_index = u32HeadLen;
	strBeaconInfo.u16tim_element_trailer_len = u32TailLen;

	/* Copy beacon head part*/
	pu8BeaconFrame = strBeaconInfo.u8beacon_frame;
	NMI_memcpy(pu8BeaconFrame, pu8Head, u32HeadLen);
	pu8BeaconFrame += u32HeadLen;

	/* Set the TIM element field with default parameters and update the      */
	/* index value with the default length.                                  */
	*(pu8BeaconFrame++) = ITIM;
	*(pu8BeaconFrame++) = DEFAULT_TIM_LEN;
	*(pu8BeaconFrame++) = 0;
	*(pu8BeaconFrame++) = strBeaconInfo.u8DTIMPeriod;
	*(pu8BeaconFrame++) = 0;
	*(pu8BeaconFrame++) = 0;

	NMI_memcpy(pu8BeaconFrame, pu8Tail, u32TailLen);
	pu8BeaconFrame+=u32TailLen;

	NMI_beacon_xmit(strBeaconInfo.u8beacon_frame,strBeaconInfo.u16beacon_len);

	PRINT_D(HOSTAPD_DBG,"Starting TSF timer \n");
	//printk("sizeof(struct tx_complete_data) = %d \n--------------------------------------\n",sizeof(struct tx_complete_data));

	/* Initialize the virtual bitmap */
	    strBeaconInfo.u8vbmap[TYPE_OFFSET]        		= ITIM;                  		/* Element ID       */
	    strBeaconInfo.u8vbmap[LENGTH_OFFSET]      		= DEFAULT_TIM_LEN;       	/* Element Length   */
	    strBeaconInfo.u8vbmap[DTIM_CNT_OFFSET]    		= 0;                     			/* Dtim Count       */
	    strBeaconInfo.u8vbmap[DTIM_PERIOD_OFFSET] 	= strBeaconInfo.u8DTIMPeriod;     /* Dtim Period      */
	    strBeaconInfo.u8vbmap[BMAP_CTRL_OFFSET]   		= 0;                     			/* Bitmap Control   */
	    strBeaconInfo.u8vbmap[TIM_OFFSET]         		= 0;                     			/* Copy TIM element */


	/* Start TSF timer in chip to start sending beacons*/
	strWlan->hif_func.hif_write_reg(rMAC_TSF_CON, BIT1 | BIT0);

	return s32Error;

}

/*
*  @brief 			host_del_beacon
*  @details 	   	delete the allocated beacon
*  @param[in]    	NMI_WFIDrvHandle hWFIDrv
*  @return 	    	Error code.
*  @author		Abd Al-Rahman Diab
*  @date			09 APRIL 2013
*  @version		1.0
*/
NMI_Sint32 host_del_beacon(NMI_WFIDrvHandle hWFIDrv)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	PRINT_D(HOSTAPD_DBG,"host_del_beacon \n");
	
	if(strBeaconInfo.u8beacon_frame != NULL)
	{
		kfree(strBeaconInfo.u8beacon_frame);
		strBeaconInfo.u8beacon_frame=NULL;
		strBeaconInfo.u16beacon_len = 0;
	}
		
	NMI_ERRORCHECK(s32Error);

	NMI_CATCH(s32Error)
	{
	}
	return s32Error;
}

/*
*  @brief 			handle tbtt ISR and send updated beacon to chip
*  @details 	   	
*  @param[in]    	
*  @return 	    	
*  @author		Abd Al-Rahman Diab
*  @date			09 APRIL 2013
*  @version		1.0
*/
void process_tbtt_isr(void)
{

       NMI_Uint16 i           = 0;
	NMI_Uint8 u8OldTimLen = 0;
	NMI_Uint32 dtim_count  = 0;
	NMI_Uint16  u16size=0;
	nmi_wlan_dev_t* strWlan = Get_wlan_context(&u16size); 

	//Warning : This is a development only print to notify the developer, it should be removed later on 
	if(u16size != sizeof(nmi_wlan_dev_t))
		PRINT_ER("size of nmi_wlan_dev_t in nmi_wlan != it's size in NMI_HOST_AP\n");

	//PRINT_D(HOSTAPD_DBG,"--IN process_tbtt_isr--\n");

        /* Read the current DTIM count from H/W */
	strWlan->hif_func.hif_read_reg(rMAC_DTIM_COUNT_ADDR, &dtim_count);
		
       //NMI_PRINTF("dtim_count = %d",dtim_count);

        /* If the beacon pointer has been updated to a new value, the free   */
        /* beacon buffer index is updated to the other buffer.               */
        if(dtim_count == 0)
        {
            dtim_count = strBeaconInfo.u8DTIMPeriod - 1;

		
            /* The beacon transmitted at this TBTT is the DTIM. Requeue all  */
            /* MC/BC packets to the high priority queue.                     */
            /* Check if the beacon that is being transmitted has the MC bit  */
            /* set                                                           */
            if(NMI_TRUE == get_mc_bit_bcn())
                while(requeue_ps_packet(NULL, &g_mc_q, NMI_TRUE, NMI_FALSE)
                      == PKT_REQUEUED);
        
        }
        else
        {
            /* Do nothing */
            dtim_count--;
        }

	/* If the buffer that is not currently in use has not been freed*/
	if(strBeaconInfo.u8beacon_frame != NULL)
	{
	
#ifdef NMI_AP_EXTERNAL_MLME

			
		/* Shift the position of the trailer after the TIM element if needed*/
		u8OldTimLen = strBeaconInfo.u8beacon_frame[strBeaconInfo.u8tim_element_index + LENGTH_OFFSET];
		if(u8OldTimLen >   strBeaconInfo.u8vbmap[LENGTH_OFFSET])
		{
			NMI_Uint32 u32ShiftOffset = u8OldTimLen - strBeaconInfo.u8vbmap[LENGTH_OFFSET];
			NMI_Uint32 u32TrailerIndex = strBeaconInfo.u8tim_element_index+u8OldTimLen+2;
			/*need to shrink the old TIM; i.e. shift the trailer backwards*/
			for(i=u32TrailerIndex;  i<(u32TrailerIndex+strBeaconInfo.u16tim_element_trailer_len); i++)
			{
				strBeaconInfo.u8beacon_frame[i-u32ShiftOffset] = strBeaconInfo.u8beacon_frame[i];
			}
		}
		else if(u8OldTimLen <   strBeaconInfo.u8vbmap[LENGTH_OFFSET])
		{
			NMI_Uint32 u32ShiftOffset = strBeaconInfo.u8vbmap[LENGTH_OFFSET] - u8OldTimLen;
			NMI_Uint32 u32TrailerIndex = strBeaconInfo.u8tim_element_index+u8OldTimLen+2;
			/*need to enlarge the old TIM; i.e. shift the trailer forward*/
			for(i=(u32TrailerIndex+strBeaconInfo.u16tim_element_trailer_len-1);  i>=u32TrailerIndex; i--)
			{
				strBeaconInfo.u8beacon_frame[i+u32ShiftOffset] = strBeaconInfo.u8beacon_frame[i];
			}
		}
		/* Other wise TIM element has the same length, and no shifting is required*/
#endif /*NMI_AP_EXTERNAL_MLME*/	
	        strBeaconInfo.u8vbmap[DTIM_CNT_OFFSET] = dtim_count;
		
	        for(i = 0; i < strBeaconInfo.u8vbmap[LENGTH_OFFSET] + 2; i++)
	        {
	            strBeaconInfo.u8beacon_frame[strBeaconInfo.u8tim_element_index + i] =
	                                                                    strBeaconInfo.u8vbmap[i];
	        }
	        strBeaconInfo.u16beacon_len=  strBeaconInfo.u8tim_element_index +  strBeaconInfo.u16tim_element_trailer_len +
	                                          strBeaconInfo.u8vbmap[LENGTH_OFFSET] + 2 ;

		
	        /* The status of BC/MC packets queued for PS should be updated only  */
	        /* in DTIM beacon dtim_count==0. For the rest of the becons reset    */
	        /* the BC/MC bit in TIM                                              */
	        if(dtim_count != 0)
	            reset_mc_bit_bcn();

		/* send the updated beacon to firmware */
		NMI_beacon_xmit(strBeaconInfo.u8beacon_frame,strBeaconInfo.u16beacon_len);
		

	}


}


/*****************************************************************************/
/* Inline Functions                                                          */
/*****************************************************************************/

/* List buffer is a queue maintained as a singly-linked link list of the   */
/* elements to be stored in the queue. It uses 4 bytes from the element    */
/* itself to store the link address of the next element. This has low      */
/* processing overhead & memory requirements. However, it assumes the      */
/* availability of the extra 4 Bytes in the element for creating the       */
/* link. These 4 bytes are specified by the user during the                */
/* initialization phase & the assigned bytes should not be modified by     */
/* the user.                                                               */
/* Note that no critical sections are currently used in this module. Hence */
/* a List Buffer should be accessed from only one context/thread.          */

/* This function initializes the List Buffer handle. The offset within the */
/* element which can be used for storing the link is specified as input.   */
inline void init_list_buffer(list_buff_t *lbuff, NMI_Uint32 lnk_byte_ofst)
{
    lbuff->head          = NULL;
    lbuff->tail          = NULL;
    lbuff->count         = 0;
    lbuff->lnk_byte_ofst = lnk_byte_ofst;
}

/* This function adds a new element to the end of the queue. */
inline void add_list_element(list_buff_t *lbuff, void *elem)
{
    NEXT_ELEMENT_ADDR(elem, lbuff->lnk_byte_ofst) = 0;

    if(lbuff->tail == NULL)
        lbuff->head = elem;
    else
        NEXT_ELEMENT_ADDR(lbuff->tail, lbuff->lnk_byte_ofst) = (NMI_Uint32)elem;

    lbuff->tail = elem;
    lbuff->count++;
}

/* This function removes the first element from the head of the queue */
inline void *remove_list_element_head(list_buff_t *lbuff)
{
    void *retval = NULL;

    if(lbuff->head == NULL)
        return NULL;

    retval      = lbuff->head;
    lbuff->head = (void *)NEXT_ELEMENT_ADDR(lbuff->head, lbuff->lnk_byte_ofst);

    if(lbuff->head == NULL)
        lbuff->tail = NULL;

    lbuff->count--;

    return retval;
}

/* This function removes a given element from the queue                      */
/* The previous element also needs to be given. The previous element ptr     */
/* will be NULL if the curr element is the head                              */
/* CAUTION: There is no check done if the element exist in the link list     */
inline void remove_list_element(list_buff_t *lbuff, void *prev_el,
                                    void *curr_el)
{

    void *next_el;
    next_el = (void *)NEXT_ELEMENT_ADDR(curr_el, lbuff->lnk_byte_ofst);

    if(prev_el == NULL)
    {
        /* The current element is the head */
        lbuff->head = next_el;
    }
    else
    {
        NEXT_ELEMENT_ADDR(prev_el, lbuff->lnk_byte_ofst) = (NMI_Uint32)next_el;
    }

    if(next_el == NULL)
    {
        /* The current element is the tail */
        lbuff->tail = prev_el;
    }

    lbuff->count--;

}

/* This function is used to check the head of the queue */
inline void *peek_list(list_buff_t *lbuff)
{
    return (lbuff->head);
}

/* This function is used to get the pointer to the next element in the list  */
/* The reference is the current element                                      */
inline void *next_element_list(list_buff_t *lbuff, void *curr_el)
{
    if(curr_el == NULL)
        return lbuff->head;
    else
        return (void *)NEXT_ELEMENT_ADDR(curr_el, lbuff->lnk_byte_ofst);
}


/* This function merges 2 queues of the SAME type  */
/* Queue 2 is reset                                */
inline void merge_list(list_buff_t *q1, list_buff_t *q2)
{
    if(q1->head == NULL)
    {
        q1->head = q2->head;
    }
    else
    {
        NEXT_ELEMENT_ADDR(q1->tail, q1->lnk_byte_ofst)
            = (NMI_Uint32) q2->head;
    }

    if(q2->tail != NULL)
        q1->tail = q2->tail;

    q1->count += q2->count;

    /* empty the 2nd Q */
    q2->head = NULL;
    q2->tail = NULL;
    q2->count = 0;
}


/*!
 *  @fn		void NMI_AP_AddSta(u8 *mac, struct station_parameters *params)
 *  @brief		add a station entry in the stations' table
 *  @details	Stations are identified by the SSID
 *  @return
 *  @todo
 *  @sa
 *  @author	Adham Abozaeid
 *  @date		9 May 2013
 *  @version	1.1 moved to driver 
 */
inline void NMI_AP_AddSta(u8 *mac, struct station_parameters *params)
{
	NMI_Uint8 au8Sa[6];
	NMI_Uint8 * pu8CurrData;
	NMI_ieee80211_sta* pstrAsocEntry = NULL;
	NMI_Uint32 i;
	NMI_Bool bIsPrevAsoc = NMI_FALSE;
	NMI_Uint8 ht_info_mod = 0;
	NMI_Uint16 u16CapInfo = 0;
	
	
	NMI_memcpy(au8Sa, mac, ETH_ALEN);

	PRINT_D(HOSTAPD_DBG,"Add station BSSID: %x, %x, %x,%x,%x,%x,\n",
			au8Sa[0], au8Sa[1], au8Sa[2], au8Sa[3], au8Sa[4], au8Sa[5]);

	pstrAsocEntry = (NMI_ieee80211_sta*)find_entry(au8Sa);


	pstrAsocEntry = (NMI_ieee80211_sta *)kmalloc(sizeof(NMI_ieee80211_sta),GFP_ATOMIC);	

	
	if(pstrAsocEntry == NULL)
	{
		PRINT_ER("No memory available to add new station\n");
		return;
	}

	/* Reset the AE Handle */
	NMI_memset(pstrAsocEntry, 0, sizeof(NMI_ieee80211_sta));

	// TODO: Implement inactivity timer
	/*Add timer for the first new station entry*/
	/*  if(g_inactive_timer == 0)
		{
		  PRINTK("FIRST STA: CREATE TIMER\n");
		  g_inactive_timer = create_alarm(ap_inactive_timeout, 0,"sta_inactive");
		  start_alarm(g_inactive_timer,1000);
		}
	*/

	add_entry((void*)pstrAsocEntry, au8Sa);
	pstrAsocEntry->aging_cnt = 0;



	/* Set the power save state to default ACTIVE. Will be checked/updated   */
	/* as required later.                                                    */
	pstrAsocEntry->ps_state         = ACTIVE_PS;
	pstrAsocEntry->ps_poll_rsp_qed  = NMI_FALSE;
	pstrAsocEntry->num_ps_pkt       = 0;
	pstrAsocEntry->num_qd_pkt       = 0;
	init_list_buffer(&(pstrAsocEntry->ps_q_lgcy),
	0); 
			
	


	pstrAsocEntry->u16AID = params->aid;	
	PRINT_D(HOSTAPD_DBG,"Assoc ID: %x\n", pstrAsocEntry->u16AID);
	

	// TODO: Really ?
	/* The 2 MSB bits of Association ID is set to 1 as required by the       */
	/* standard.                                                             */
	//pstrAsocEntry->asoc_id = (pstrAsocEntry->sta_index | 0xc000);


	pstrAsocEntry->op_rates.num_rates = params->supported_rates_len;
	//strStaParams.pu8Rates = params->supported_rates;

	if(pstrAsocEntry->op_rates.num_rates > MAX_RATES_SUPPORTED)
	{
		PRINT_ER("number of rates %d exceeds MAX_NUM_RATES %d. will use max!\n",
		pstrAsocEntry->op_rates.num_rates, MAX_RATES_SUPPORTED);
		pstrAsocEntry->op_rates.num_rates = MAX_RATES_SUPPORTED;
	}
	PRINT_D(HOSTAPD_DBG,"Num rates: %d \n",pstrAsocEntry->op_rates.num_rates);
		
	for(i=0; i<pstrAsocEntry->op_rates.num_rates; i++)
	{
		pstrAsocEntry->op_rates.rates[i] = (params->supported_rates)[i]&0x7F;
		PRINT_D(HOSTAPD_DBG,"%d ",pstrAsocEntry->op_rates.rates[i]);
	}
	PRINT_D(HOSTAPD_DBG,"\n");
	

	
	
	/* TODO: Rates should be sorted by here, if not, the code should sort it*/
#if 0
	for(i = 0; i < num_rates; i++)
	{
		NMI_Uint8 min = (ae->op_rates.rates[i] & 0x7F);
		NMI_Uint8 mid = i;
		NMI_Uint8 tmp = 0;

		for(j = i + 1; j < num_rates; j++)
		{
			if(min > (ae->op_rates.rates[j] & 0x7F))
			{
				min = (ae->op_rates.rates[j] & 0x7F);
				mid = j;
			}
		}

		tmp = ae->op_rates.rates[i];
		ae->op_rates.rates[i] = ae->op_rates.rates[mid];
		ae->op_rates.rates[mid] = tmp;
	}
#endif

	/* TODO: If the STA does not support all the basic rates respond with  */
	/* failure (unsupported rates) status.                           */
#if 0
	for(i = 0; i < get_num_basic_rates(); i++)
	{
		NMI_Bool found = NMI_FALSE;
		NMI_Uint8 mac_br = get_mac_basic_rate(i);

		for(j = 0; j < pstrAsocEntry->op_rates.num_rates; j++)
		{
			if((pstrAsocEntry->op_rates.rates[j] & 0x7F) == (mac_br & 0x7F))
			{
				found = NMI_TRUE;
				break;
			}
		}

		/* Return unsupported rates if the joining station does not  */
		/* support any of the basic rates.                           */
		if(found == NMI_FALSE)
			return UNSUP_RATE;
	}
#endif

	// TODO: Add 11n support
	pstrAsocEntry->ht_cap.ht_supported = (params->ht_capa != NMI_NULL);
	PRINT_D(HOSTAPD_DBG,"HT supported: %d\n", pstrAsocEntry->ht_cap.ht_supported);
	if(pstrAsocEntry->ht_cap.ht_supported)
	{
	/*
	pu8CurrData += NMI_AP_ParseHTCap(pu8CurrData, &pstrAsocEntry->ht_hdl,
			bIsPrevAsoc, &ht_info_mod);*/
	u16CapInfo = params->ht_capa->cap_info;
	pstrAsocEntry->ht_cap.cap = params->ht_capa->cap_info;
	}
	/* If the STA is not HT capable disallow association if 11n operating    */
	/* mode is not HT_MIXED. If it is HT_MIXED, update the Operating Mode to */
	/* 3. Also update the global number of non-HT STAs                       */
	//if(pstrAsocEntry->ht_cap.ht_supported  == 0)
	//{
		// TODO: update this according to the FSM
		/*if(get_11n_op_mode() == HT_MIXED_MODE)
		{
			if(mget_HTOperatingMode() != 3)
			{
				ht_info_mod = 1;
				set_ht_operating_mode(3);
				disable_rifs();
			}

			g_num_sta_nonht_asoc++;

			/* If auto protection is set, and neither ERP nor HT protection  */
			/* is currently in use, enable HT protection.                    */
			/*if((is_autoprot_enabled() == NMI_TRUE) &&
					(get_protection() != ERP_PROT) &&
					(get_protection() != HT_PROT))
			{
				set_protection(HT_PROT);
			}
		}
		else
		{
			// return STA_NOT_HTCAP;
			PRINT_D(HOSTAPD_DBG,"Station is not HT capable and AP mode don't support HT_MIXED_MODE");
		}*/
	//}

	
	
	

	/* Print the STA's HT-Capabilities */
	/*PRINT_D(HOSTAPD_DBG,"STA HT-Capabilities:\n\r");
	PRINT_D(HOSTAPD_DBG,"ht_capable             = %x\n\r", pstrAsocEntry->ht_hdl.ht_capable);
	PRINT_D(HOSTAPD_DBG,"ldpc_cod_cap           = %x\n\r", pstrAsocEntry->ht_hdl.ldpc_cod_cap);
	PRINT_D(HOSTAPD_DBG,"smps_mode              = %x\n\r", pstrAsocEntry->ht_hdl.smps_mode);
	PRINT_D(HOSTAPD_DBG,"greenfield             = %x\n\r", pstrAsocEntry->ht_hdl.greenfield);
	PRINT_D(HOSTAPD_DBG,"short_gi_20            = %x\n\r", pstrAsocEntry->ht_hdl.short_gi_20);
	PRINT_D(HOSTAPD_DBG,"short_gi_40            = %x\n\r", pstrAsocEntry->ht_hdl.short_gi_40);
	PRINT_D(HOSTAPD_DBG,"rx_stbc                = %x\n\r", pstrAsocEntry->ht_hdl.rx_stbc);
	PRINT_D(HOSTAPD_DBG,"max_rx_ampdu_factor    = %x\n\r", pstrAsocEntry->ht_hdl.max_rx_ampdu_factor);
	PRINT_D(HOSTAPD_DBG,"min_mpdu_start_spacing = %x\n\r", pstrAsocEntry->ht_hdl.min_mpdu_start_spacing);
	PRINT_D(HOSTAPD_DBG,"htc_support            = %x\n\r", pstrAsocEntry->ht_hdl.htc_support);
	PRINT_D(HOSTAPD_DBG,"sta_amsdu_maxsize      = %x\n\r", pstrAsocEntry->ht_hdl.sta_amsdu_maxsize);
	PRINT_D(HOSTAPD_DBG,"chan_width             = %x\n\r", pstrAsocEntry->ht_hdl.chan_width);
	PRINT_D(HOSTAPD_DBG,"dsss_cck_40mhz         = %x\n\r", pstrAsocEntry->ht_hdl.dsss_cck_40mhz);*/
	//PRINT_D(HOSTAPD_DBG,"cipher_type            = %x\n\r", pstrAsocEntry->cipher_type);

	// return SUCCESSFUL_STATUSCODE;

	// TODO: Update This in the firmware
	/* Update the tx rate index */
	//pstrAsocEntry->tx_rate_index = pstrAsocEntry->op_rates.num_rates - 1;

	/* Assign the index to the maximum rate supported by the station */
	//pstrAsocEntry->tx_rate_mbps = get_user_rate((NMI_Uint8)
			//(pstrAsocEntry->op_rates.rates[pstrAsocEntry->op_rates.num_rates- 1] & 0x7F));

	

	// TODO: Do we need this
	#if 0
	/* If the associated STA does not support Short Slot Time option,    */
	/* the AP needs to use Long Slot Time from the next beacon interval. */
	if((u16CapInfo & SHORTSLOT) != SHORTSLOT)
	{
		/* If STA is already associated as a long slot STA, don't update */
		/* g_num_sta_no_short_slot                                       */
		if((pstrAsocEntry->state != ASOC) || (pstrAsocEntry->short_slot == 1))
		{
			/* Set the short slot supported field to 0. Also increment   */
			/* the global indicating number of stations not supporting   */
			/* ShortSlot                                                 */
			pstrAsocEntry->short_slot = 0;
			g_num_sta_no_short_slot++;

			disable_short_slot();
			set_machw_long_slot_select();

		}
	}
	else
	{
		/* If STA is already associated as a long slot STA, but trying   */
		/* to reassociate as a short slot STA, update the global         */
		/* g_num_sta_no_short_slot                                       */
		if((pstrAsocEntry->state == ASOC) && (pstrAsocEntry->short_slot == 0))
		{
			g_num_sta_no_short_slot--;
			if(g_num_sta_no_short_slot == 0)
			{
				if(g_short_slot_allowed == NMI_TRUE)
				{
					enable_short_slot();
					set_machw_short_slot_select();
				}
			}

		}
			pstrAsocEntry->short_slot = 1;
	}

	/* Check if the station is 11b station and enable the ERP protection */
	/* If the running mode is G_ONLY_MODE, it would have returned        */
	/* UNSUP_RATE and not allow the station to associate with AP.        */
		if(pstrAsocEntry->op_rates.num_rates <= NUM_BR_PHY_802_11G_11B_1)
		{
			/* If STA is already associated as an non ERP STA, do not update */
			/* g_num_sta_non_erp                                             */
			if((pstrAsocEntry->state != ASOC) ||
					(u8NumRatesOld > NUM_BR_PHY_802_11G_11B_1))
			{
				/* The associated STA is 11b (non-ERP) station */
				g_num_sta_non_erp++;

				/* Enable protection in case of auto protection */
				if(is_autoprot_enabled() == NMI_TRUE)
				{
					set_protection(ERP_PROT);
					set_machw_prot_control();
				}
			}
		}
		else
		{
			/* If STA is already associated as a non ERP STA, but trying to */
			/* reassociate as an ERP STA, update g_num_sta_non_erp          */
			if((pstrAsocEntry->state == ASOC) &&
					(u8NumRatesOld <= NUM_BR_PHY_802_11G_11B_1))
			{
				g_num_sta_non_erp--;

				if(g_num_sta_non_erp == 0)
				{
					if(is_autoprot_enabled() == NMI_TRUE)
					{
						set_protection(ERP_PROT);
						set_machw_prot_control();
					}
				}
			}
		}

		/* Check if the station supports only long preamble */
		if((u16CapInfo & SHORTPREAMBLE) != SHORTPREAMBLE)
		{
			/* If STA is already associated as a long preamble STA, do not   */
			/* update g_num_sta_no_short_pream                               */
			if((pstrAsocEntry->state != ASOC) || (pstrAsocEntry->short_preamble == 1))
			{
				/* Set the short preamble supported field to 0 and increment */
				/* the global indicating number of stations not supporting   */
				/* short preamble.                                           */
				pstrAsocEntry->short_preamble = 0;
				g_num_sta_no_short_pream++;
				g_short_preamble_enabled = NMI_FALSE;
				set_machw_prot_pream(1);
			}
		}
		else
		{
			/* If STA is already associated as a long preamble STA, but      */
			/* trying to reassociate as a short preamble STA, update         */
			/* g_num_sta_no_short_pream                                      */
			if((pstrAsocEntry->state == ASOC) && (pstrAsocEntry->short_preamble == 0))
			{
				g_num_sta_no_short_pream--;
				if(g_num_sta_no_short_pream == 0)
				{
					g_short_preamble_enabled = NMI_TRUE;
					set_machw_prot_pream(0);
				}
			}
			pstrAsocEntry->short_preamble = 1;
		}
	#endif

	/* Update the protocol capability in association entry for the STA */

	// TODO: Update This in the firmware
	/* Update the retry rate set table for this station based on the current */
	/* transmit rate of this station                                         */
	//update_entry_retry_rate_set((void *)pstrAsocEntry, get_phy_rate(get_tx_rate_ap(pstrAsocEntry)));

	// TODO: Autorate is not enabled in our case
#if 0
	/* Update the minimum and maximum rate index in the global auto  */
	/* rate table for this STA                                       */
	update_min_rate_idx_ap(pstrAsocEntry);
	update_max_rate_idx_ap(pstrAsocEntry);

	/* Update the current transmit rate index to minimum supported   */
	/* rate index in case of auto rate                               */
	init_tx_rate_idx_ap(pstrAsocEntry);
	reinit_tx_rate_idx_ap(pstrAsocEntry);

	ar_stats_init(&(pstrAsocEntry->ar_stats));
#endif /* AUTORATE_FEATURE */

	pstrAsocEntry->state = ASOC;
}


/*!
 *  @fn		void NMI_AP_EditSta(u8 *mac, struct station_parameters *params)
 *  @brief		Edits a station entry in the stations' table
 *  @details	Stations are identified by the SSID
 *  @return
 *  @todo
 *  @sa
 *  @author	Adham Abozaeid
 *  @date		9 May 2013
 *  @version	1.1 moved to driver 
 */
inline void NMI_AP_EditSta(u8 *mac, struct station_parameters *params)
{
	NMI_Uint8 au8Sa[6];
	NMI_ieee80211_sta* pstrAsocEntry = NULL;
	NMI_Uint8 u8NumRatesOld = 0;
	NMI_Uint16 u16Mask, u16Set;
	
	NMI_memcpy(au8Sa, mac, ETH_ALEN);

	PRINT_D(HOSTAPD_DBG,"change station : BSSID: %x, %x, %x,%x,%x,%x,\n",
			au8Sa[0], au8Sa[1], au8Sa[2], au8Sa[3], au8Sa[4], au8Sa[5]);

	pstrAsocEntry = (NMI_ieee80211_sta*)find_entry(au8Sa);


	if(pstrAsocEntry->state == ASOC)
	{
		u8NumRatesOld = pstrAsocEntry->op_rates.num_rates;

		// TODO: add security key
		#if 0
		/*Associated Station*/
		 if(is_wep_allowed() == NMI_TRUE)
		{
			PRINTK("SETTING WEP ENTRY\n");
			add_wep_entry(pstrAsocEntry->sta_index,mget_WEPDefaultKeyID(),au8Sa);
		}
		 #endif
	}

	u16Mask = params->sta_flags_mask;
	u16Set = params->sta_flags_set;
	
}


/*!
 *  @fn			inline void NMI_AP_RemoveSta(NMI_Uint8 * pu8StaInfo)
 *  @brief			Removes station from the stations' table
 *  @details		Stations are identified by the SSID
 *  @return
 *  @todo
 *  @sa
 *  @author		Adham Abozaeid
 *  @date			4 July 2012
 *  @version		1.1 moved to driver 
 */
inline void NMI_AP_RemoveSta(u8 *mac)
{
	NMI_ieee80211_sta* pstrAsocEntry = NULL;
	NMI_Uint8 au8Sa[6];
	NMI_Uint8 * pu8CurrData;
	table_elmnt_t *tbl_elm = 0;
	NMI_Uint8 elem_numb=0;
	NMI_Uint32 i;
	

	NMI_memcpy(au8Sa, mac, ETH_ALEN);
	
	PRINT_D(HOSTAPD_DBG,"Removing stationg BSSID: %x, %x, %x,%x,%x,%x,\n",
			au8Sa[0], au8Sa[1], au8Sa[2], au8Sa[3], au8Sa[4], au8Sa[5]);
	pstrAsocEntry = (NMI_ieee80211_sta*)find_entry(au8Sa);
	
	if(pstrAsocEntry != NULL)
	{
		delete_entry(au8Sa);
	}
	else
	{
		PRINT_ER("couldn't find station in table\n");
	}

	// TODO: Implement inactivity timer
	#if 0
	 for(i = 0; i < MAX_HASH_VALUES; i++)
    		    {
       			 tbl_elm = g_sta_table[i];
       			 if(tbl_elm)
        			{
        				PRINTK("There exists assoc stats dont DELETE\n");
        				elem_numb++;
           				break;
        			}
   		  }
		  if(elem_numb==0)
		  {
		  	if(g_inactive_timer)
			{
		  		PRINTK("DELETING ALARM\n");
		  		delete_alarm(g_inactive_timer);
		  		}
		  }
	#endif 
		
	return;
}

/*****************************************************************************/
/* Inline functions                                               									  */
/*****************************************************************************/

/* This function checks whether SNAP header is present in the frame */
inline NMI_Bool is_snap_header_present(wlan_rx_t *strWlan_rx)
{
    NMI_Uint8 *data = NULL;

    if(strWlan_rx->u16data_len < SNAP_HDR_LEN)
        return NMI_FALSE;

    data = strWlan_rx->u8msa + strWlan_rx->u8hdr_len;

    if(NMI_memcmp(data, g_snap_header, SNAP_HDR_ID_LEN) != 0)
            return NMI_FALSE;

    return NMI_TRUE;
}


/* This function sets the 'frame control' bits in the MAC header of the      */
/* input frame to the given 16-bit value.                                    */
inline void set_frame_control(NMI_Uint8* header, NMI_Uint16 fc)
{
   header[0] = (NMI_Uint8)(fc & 0x00FF);
   header[1] = (NMI_Uint8)(fc >> 8);
}

inline NMI_Uint8 set_mac_hdr(NMI_Uint8 *mac_hdr)
{
    set_frame_control(mac_hdr, DATA);

    return MAC_HDR_LEN;
}

/* Update the MAC header depending on the protocol */
inline NMI_Uint8 set_mac_hdr_prot(NMI_Uint8 *mac_hdr, NMI_Uint8 priority,
                               NMI_Uint8 service_class, NMI_Bool qos, NMI_Bool is_ht,
                               NMI_Uint8 is_amsdu)
{
	// TODO: implement 11n
#if 0
    if(NMI_TRUE == qos)
    {
        NMI_Uint8 mac_hdr_len = 0;

        mac_hdr_len = set_mac_hdr_11e(mac_hdr, priority, service_class);

        if(NMI_TRUE == is_ht)
            mac_hdr_len += set_ht_control(mac_hdr, mac_hdr_len);

        if(NMI_TRUE == is_amsdu)
            advt_amsdu_frame(mac_hdr);

        return mac_hdr_len;
    }
#endif 
    return set_mac_hdr(mac_hdr);
}


/* This function adjusts the frame descriptor data length and offset to      */
/* account for presence of SNAP header                                       */
inline void adjust_for_snap_header(wlan_rx_t *strWlan_rx)
{
    strWlan_rx->u16data_len    -= SNAP_HDR_LEN;
    strWlan_rx->u8hdr_len += SNAP_HDR_LEN;
}

/* This function sets the 'from ds' bit in the MAC header of the input frame */
/* to the given value stored in the LSB bit.                                 */
/* The bit position of the 'from ds' in the 'frame control field' of the MAC */
/* header is represented by the bit pattern 0x00000010.                      */
inline void set_from_ds(NMI_Uint8* header, NMI_Uint8 from_ds)
{
    header[1] &= 0xFD;
    header[1] |= (from_ds << 1);
}


/* This function sets the 'address1' field in the MAC header of the input    */
/* frame to the input MAC Address 'addr'. The 16 LSB bits of 'addr' are      */
/* ignored.                                                                  */
inline  void set_address1(NMI_Uint8* msa, NMI_Uint8* addr)
{
    NMI_memcpy(msa + 4, addr, ETH_ALEN);
}

/* This function sets the 'address2' field in the MAC header of the input    */
/* frame to the input MAC Address 'addr'. The 16 LSB bits of 'addr' are      */
/* ignored.                                                                  */
inline  void set_address2(NMI_Uint8* msa, NMI_Uint8* addr)
{
    NMI_memcpy(msa + 10, addr, ETH_ALEN);
}


/* This function sets the 'address3' field in the MAC header of the input    */
/* frame to the input MAC Address 'addr'. The 16 LSB bits of 'addr' are      */
/* ignored.                                                                  */
inline  void set_address3(NMI_Uint8* msa, NMI_Uint8* addr)
{
    NMI_memcpy(msa + 16, addr, ETH_ALEN);
}


/* This function extracts the MAC Address in 'address1' field of the MAC     */
/* header and updates the MAC Address in the allocated 'addr' variable.      */
inline void get_address1(NMI_Uint8* msa, NMI_Uint8* addr)
{
    NMI_memcpy(addr, msa + 4, ETH_ALEN);
}

/* This function returns the pointer to the MAC Address in 'Address-1" field */
/* of the MAC header.                                                        */
inline NMI_Uint8 *get_address1_ptr(NMI_Uint8* msa)
{
    return (msa + 4);
}

/* This function extracts the MAC Address in 'address2' field of the MAC     */
/* header and updates the MAC Address in the allocated 'addr' variable.      */
inline void get_address2(NMI_Uint8* msa, NMI_Uint8* addr)
{
    NMI_memcpy(addr, msa + 10, ETH_ALEN);
}

/* This function extracts the MAC Address in 'address3' field of the MAC     */
/* header and updates the MAC Address in the allocated 'addr' variable.      */
inline void get_address3(NMI_Uint8* msa, NMI_Uint8* addr)
{
    NMI_memcpy(addr, msa + 16, ETH_ALEN);
}

/* This function returns the pointer to the MAC Address in 'Address-3" field */
/* of the MAC header.                                                        */
inline NMI_Uint8 *get_address3_ptr(NMI_Uint8* msa)
{
    return (msa + 16);
}

/* This function extracts the 'frame type' bits from the MAC header of the   */
/* input frame.                                                              */
/* Returns the value in the LSB of the returned value.                       */
inline BASICTYPE_T get_type(NMI_Uint8* header)
{
    return ((BASICTYPE_T)(header[0] & 0x0C));
}

/* This function extracts the 'frame type and sub type' bits from the MAC    */
/* header of the input frame.                                                */
/* Returns the value in the LSB of the returned value.                       */
inline TYPESUBTYPE_T get_sub_type(NMI_Uint8* header)
{
    return ((TYPESUBTYPE_T)(header[0] & 0xFC));
}

/* This function extracts the 'to ds' bit from the MAC header of the input   */
/* frame.                                                                    */
/* Returns the value in the LSB of the returned value.                       */
inline NMI_Uint8 get_to_ds(NMI_Uint8* header)
{
    return (header[1] & 0x01);
}

/* This function extracts the 'from ds' bit from the MAC header of the input */
/* frame.                                                                    */
/* Returns the value in the LSB of the returned value.                       */
inline NMI_Uint8 get_from_ds(NMI_Uint8* header)
{
    return ((header[1] & 0x02) >> 1);
}

/* This function extracts the 'power management' bit from the MAC header of  */
/* the input frame.                                                          */
/* Returns the value in the LSB of the returned value.                       */
inline NMI_Uint8 get_pwr_mgt(NMI_Uint8* header)
{
    return ((header[1] & 0x10) >> 4);
}

/* This function compares the address with the (last bit on air) BIT24 to    */
/* determine if the address is a group address.                              */
/* Returns NMI_TRUE if the input address has the group bit set.                 */
inline NMI_Bool is_group(NMI_Uint8* addr)
{
    if((addr[0] & BIT0) != 0)
        return NMI_TRUE;

    return NMI_FALSE;
}

inline NMI_Bool is_sub_type_null_prot(NMI_Uint16 frm_type)
{

    if(frm_type == QOS_NULL_FRAME)
        return NMI_TRUE;


    return NMI_FALSE;
}


/* This function extracts the 'protocol version' bits from the MAC header of */
/* the input frame.                                                          */
/* Returns the value in the LSB of the returned value.                       */
inline NMI_Uint8 get_protocol_version(NMI_Uint8* header)
{
    return header[0] & 0x03;
}

/* This function is used to handle the Address4 field in the header.         */
/* This checks the from and to DS bits in the header and if both are set     */
/* returns NMI_TRUE, NMI_FALSE otherwise                                           */
inline NMI_Bool check_from_to_ds(wlan_rx_t *wlan_rx)
{
    if(wlan_rx->u8frm_ds && wlan_rx->u8to_ds)
        return NMI_TRUE;

    return NMI_FALSE;
}

/* This function compares two given MAC addresses (m1 and m2).               */
/* Returns NMI_Bool, NMI_TRUE if the two addresses are same and NMI_FALSE otherwise. */
inline NMI_Bool mac_addr_cmp(NMI_Uint8* m1, NMI_Uint8* m2)
{
    if(memcmp(m1, m2, 6) == 0)
        return NMI_TRUE;

    return NMI_FALSE;
}

/* In AP/STA mode, check if the packet is from this BSS, once state is ENABLED */
inline NMI_Bool check_bssid_match(wlan_rx_t *strWlan_rx)
{

	linux_wlan_t *pd = g_linux_wlan;
	NMI_Uint8*  u8macaddr = NULL;  

	// TODO:  add FSM	
    //if((get_mac_state() == ENABLED) && (strWlan_rx->type != CONTROL))
    if(strWlan_rx->enumType != CONTROL)
    {
        /* Management frames are processed (probe request may have broadcast */
        /* cast BSSID, coalescing may be required)                           */
        if(strWlan_rx->enumType == MANAGEMENT)
            return NMI_TRUE;

	// Warning: temperorary untill FSM is implemented
	if((pd->nmc_netdev )== NULL)
		return NMI_FALSE;

	/**
	get the bssid address, since we are th AP , the dev address is the BSSID
		**/
	//PRINT_D(HOSTAPD_DBG,"Calling cfg_get to get MAC_ADDR\n");
	u8macaddr =(NMI_Uint8*) pd->nmc_netdev->dev_addr;
	//PRINT_D(HOSTAPD_DBG,"%2x.%2x.%2x.%2x.%2x.%2x.\n",u8macaddr[0],u8macaddr[1],u8macaddr[2],u8macaddr[3],u8macaddr[4],u8macaddr[5]);

        if(mac_addr_cmp(strWlan_rx->u8bssid, u8macaddr) == NMI_FALSE)
            return NMI_FALSE;
    }

    return NMI_TRUE;
}

/* This function extracts the updates the SA, DA & BSSID address pointers to */
/* addr1, addr2 & addr3 fields in the WLAN RX structure.                     */
inline void set_SA_DA_BSSID_ptr(wlan_rx_t *strWlan_rx)
{
    NMI_Uint8 frm_ds = strWlan_rx->u8frm_ds;
    NMI_Uint8 to_ds  = strWlan_rx->u8to_ds;
    if((to_ds == 0) && (frm_ds == 0))
    {
        strWlan_rx->u8sa    = strWlan_rx->u8addr2;
        strWlan_rx->u8da    = strWlan_rx->u8addr1;
        strWlan_rx->u8bssid = strWlan_rx->u8addr3;
    }
    else if((to_ds == 0) && (frm_ds == 1))
    {
        strWlan_rx->u8sa    = strWlan_rx->u8addr3;
        strWlan_rx->u8da    = strWlan_rx->u8addr1;
        strWlan_rx->u8bssid = strWlan_rx->u8addr2;
    }
    else if((to_ds == 1) && (frm_ds == 0))
    {
        strWlan_rx->u8sa    = strWlan_rx->u8addr2;
        strWlan_rx->u8da    = strWlan_rx->u8addr3;
        strWlan_rx->u8bssid = strWlan_rx->u8addr1;
    }

    strWlan_rx->u8ta    = strWlan_rx->u8addr2;
}


/* This function checks if QoS bit is set in the given QoS frame */
inline NMI_Bool is_qos_bit_set(NMI_Uint8* msa)
{
    return (msa[0] & 0x80)?NMI_TRUE:NMI_FALSE;
}

/* This function sets the 'more data' bit in the MAC header of the input     */
/* frame to the LSB of the given value.                                      */
/* The bit position of the 'more data' bit in the 'frame control field' of   */
/* the MAC header is represented by the bit pattern 0x00100000.              */
inline void set_more_data(NMI_Uint8* header, NMI_Uint8 more_data)
{
    header[1] &= 0xDF;
    header[1] |= (more_data << 5);
}

/* This function sets the end of service period bit */
inline void set_qos_prot(NMI_Uint8 *msa)
{
#if 0 //def MAC_WMM
    set_eosp(msa);
#endif /* MAC_WMM */
}

/* This function sets the AID0 bit or the Bit 7 in bit map control according */
/* to the type given as input.                                               */
inline void set_dtim_bit(NMI_Uint8 type)
{
    if(type == AID0_BIT)
        strBeaconInfo.u8vbmap[TIM_OFFSET]       |= 0x1;  /* AID = 0 */
    else
        strBeaconInfo.u8vbmap[BMAP_CTRL_OFFSET] |= 0x1;  /* Set multicast bit */
}

/* This function resets the AID0 bit or Bit 7 in bit map control according   */
/* to the type given as input.                                               */
inline void reset_dtim_bit(NMI_Uint8 type)
{
    if(type == AID0_BIT)
        strBeaconInfo.u8vbmap[TIM_OFFSET]       &= 0xFE; /* AID = 0 */
    else
        strBeaconInfo.u8vbmap[BMAP_CTRL_OFFSET] &= 0xFE; /* Reset multicast bit */
}


/* This function extracts the 'order' bit from the frame control field  */
/* within the MAC header.                                               */
inline NMI_Uint8 get_order_bit(NMI_Uint8 *header)
{
    return ((header[1] & 0x80) >> 7);

}

/* This function check whether the MAC header contains HT control field */
inline NMI_Bool is_ht_frame(NMI_Uint8 *header)
{
    if((NMI_TRUE == is_qos_bit_set(header)) && (1 == get_order_bit(header)))
        return NMI_TRUE;

    return NMI_FALSE;
}


/* This function extracts the destination MAC Address from the incoming WLAN */
/* packet based on the 'to ds' bit,and updates the MAC Address in the        */
/* allocated 'addr' variable.                                                */
inline void get_DA(NMI_Uint8* data, NMI_Uint8* da)
{
    if(get_to_ds(data) == 1)
        get_address3(data, da);
    else
        get_address1(data, da);
}

/* This function extracts the destination MAC Address from the incoming WLAN */
/* packet based on the 'to ds' bit and returns the pointer to the same.      */
inline NMI_Uint8 *get_DA_ptr(NMI_Uint8* data)
{
    if(get_to_ds(data) == 1)
        return get_address3_ptr(data);

    return get_address1_ptr(data);
}


/* Get the MAC header length depending on the protocol used */
inline NMI_Uint8 get_mac_hdr_len(NMI_Uint8 *msa)
{
    NMI_Uint8 mac_hdr_len = MAC_HDR_LEN;

    /* The MAC Header len is 26 only when in QOD Data frames */
    if((is_qos_bit_set(msa) == NMI_TRUE) && (get_type(msa) == DATA_BASICTYPE))
        mac_hdr_len += QOS_CTRL_FIELD_LEN;


    if(NMI_TRUE == is_ht_frame(msa))
        mac_hdr_len += HT_CTRL_FIELD_LEN;

    return mac_hdr_len;
}


/* update the appropriate PS queue counters after a Tx complete interrupt */
/* returns NMI_TRUE if TIM needs to be reset                                 */
inline NMI_Bool update_ps_counts_txcomp(NMI_ieee80211_sta *ae, NMI_Uint8 *msa)
{
    NMI_Bool retval = NMI_FALSE;

    /* TBD: Device a method avoid all this processing in every Tx comp    */
    /* interrupt                                                          */
#if 0//def MAC_WMM
    BOOL_T del_ena = NMI_FALSE;
    if(NMI_TRUE == is_qos_bit_set(msa))
    {
        NMI_Uint8 priority = get_tid_value(msa);

        if(NMI_TRUE == check_ac_is_del_en(ae, priority))
            del_ena = NMI_TRUE;
    }

    if(NMI_TRUE == del_ena)
    {
        /* Check the no. of packets queued for a USP, if an USP is in        */
        /* progress. Clear the flag if all packets queued in an USP are      */
        /* transmitted                                                       */
        /* This is done to ignore multiple trigger frames during an USP      */
        /* "USP_in_progress" flag is cleared when the STA wakes up           */

        if(ae->USP_in_progress == NMI_TRUE)
        {
            ae->num_USP_pkts_qed--;
            if(ae->num_USP_pkts_qed == 0)
                ae->USP_in_progress = NMI_FALSE;
        }
    }
    else
#endif /* MAC_WMM */
    {
        /* Non Delivery PS Q Frame are sent only on PS poll */
        /* Clear the Flag for PS poll response              */
        //ae->ps_poll_rsp_qed = NMI_FALSE;
    }

            if(ae->num_qd_pkt)
            {
                ae->num_qd_pkt--;
            }

#if 0//def MAC_WMM
    if(is_all_ac_del(ae) == NMI_TRUE)
    {
        if( (ae->num_ps_pkt + ae->num_qd_pkt) == 0)
            retval = NMI_TRUE;
    }
    else
#endif /* MAC_WMM */
    {
        /* If no packets are queued in Sw PSQ or Hw for the */
        /* station, reset TIM bit                           */
        if( (ae->num_ps_pkt + ae->num_qd_pkt) == 0)
        {
            retval = NMI_TRUE;
        }
    }

    return retval;
}


/* Adjust length and data offset of the  received packet */
/* Returns the length of the frame header.                            */
inline NMI_Uint16 modify_frame_length( NMI_Uint8 *msa, NMI_Uint16 *rx_len, NMI_Uint16 *data_len)
{ 
    NMI_Uint32 mac_hdr_len = get_mac_hdr_len(msa);



    /* Start of data is end of mac header offseted by the security header */
    *data_len = *rx_len - mac_hdr_len;

    /* If MAC header length is not a multiple of 4 (QOS is Enabled), then  */
    /* the H/w will be offsetting two extra bytes after the header to make */
    /* it word aligned                                                     */
    if(mac_hdr_len & 3)
        mac_hdr_len += 2;

    return (mac_hdr_len );
}


inline void flush_ps_queues(NMI_ieee80211_sta *ae)
{
    struct tx_complete_data* tx_dscr = NULL;

    /* Free the elements in Legacy PS queue */
    while(NULL != (tx_dscr = remove_list_element_head(&(ae->ps_q_lgcy))))
    {
    	     
	if(tx_dscr->skb)
		dev_kfree_skb(tx_dscr->skb);	
		
	linux_wlan_free(tx_dscr);
	
        g_num_ps_pkt--;
    }

#if 0//def MAC_WMM
    /* Free the elements in Delivery enabled PS queue */
    while(NULL != (tx_dscr = remove_list_element_head(&(ae->ps_q_del_ac))))
    {
        /* Free the buffers associated with the transmit descriptor */
        free_tx_dscr(tx_dscr);
        g_num_ps_pkt--;
    }
    ae->num_ps_pkt_del_ac = 0;
    ae->USP_in_progress   = NMI_FALSE;
#endif /* MAC_WMM */
}


/* This function handles the change of state of a station from power save to */
/* active state by requeing all buffered packets to the H/w queue.           */
/* It also reduces the global count of power save stations and requeues all  */
/* multicast/broadcast packets if this count becomes 0.                      */
inline void handle_ps_sta_change_to_active(NMI_ieee80211_sta *ae)
{
    REQUEUE_STATUS_T status = RE_Q_ERROR;

    //handle_ba_active_ap_prot(ae);

    /* Continue to requeue buffered packets from the station's power save    */
    /* queue to the MAC H/w queue till the power save queue is empty.        */

    //status = handle_ps_sta_change_to_active_prot(ae);
/*
    if(status == PKT_NOT_REQUEUED)
    {
        return;
    }
    else*/
    {
        while((status=requeue_ps_packet(ae, &(ae->ps_q_lgcy), NMI_TRUE, NMI_FALSE))
            == PKT_REQUEUED);
#if 0 //def PS_DSCR_JIT
        /* This path is not valid currently                     */
        /* May be valid if descriptor are not buffered and are  */
        /* created just in time before transmission             */
        if(status == PKT_NOT_REQUEUED)
        {
            handle_requeue_pending_packet(ae, &g_ps_pending_q, NMI_TRUE);
        }
#endif /* PS_DSCR_JIT */
    }

    /* To be safe flush all the PS queues. Though they should have been      */
    /* handled in the requeuing functions                                    */
    /* Reset other variables and TIM                                         */
    flush_ps_queues(ae);
    ae->ps_poll_rsp_qed = NMI_FALSE;
    check_and_reset_tim_bit(ae->u16AID);


    /* Decrement the global power save station count */
    g_num_sta_ps--;

    /* If there are no stations in Power save mode, requeue all multicast/   */
    /* broadcast packets.                                                    */
    if(g_num_sta_ps == 0)
    {
        while(requeue_ps_packet(NULL, &g_mc_q, NMI_TRUE, NMI_FALSE) == PKT_REQUEUED);
    }
}


/* This function handles the change of state of a station from active to     */
/* power save state by removing all packets from the H/w queue and buffering */
/* these in the S/w power save queue. Also ii increments the global count of */
/* power save stations. In case this is the only station in power save state */
/* the multicast/broadcast packets also need to be removed from the H/w      */
/* queues and buffered in the multicast queue.                               */
inline void handle_ps_sta_change_to_powersave(NMI_ieee80211_sta *ae)
{
	NMI_Bool ps_del_en_ac = NMI_FALSE;
	
    /* TBD. Remove packets from all H/w queue destined for this station and  */
    /* add to the ae queue.                                                  */

    if(g_num_sta_ps == 0)
    {
    	// TODO: add mc/bc frames from hardware tx queue to ps queue
        /* TBD - no stations were in power save. mc/bc packets need to be    */
        /* removed and added to g_mc_q */
    }

    /* To be safe flush all the PS queues. Though they should have been      */
    /* handled in the requeuing functions                                    */
    /* Reset other variables and TIM                                         */
    flush_ps_queues(ae);
    ae->ps_poll_rsp_qed = NMI_FALSE;

	// TODO: decide wither to set flags here
    //update_ps_flags_ap(ae, NMI_FALSE, num_elements_moved,ps_del_en_ac);

    /* Power Save buffering for 11n protocol related */
    //handle_ps_sta_change_to_powersave_prot(ae);

    /* Increment the global power save station count */
    g_num_sta_ps++;
}


/*
*  @brief 			This function checks the power save state of the station 
*                  		from all received packets. 
*
*  @details 		The power management bit in the received packet is      
*                  		checked and the association entry of the corresponding  
*                  		transmitting station is updated with the power save      
*                 		state. Also if there is any change in power save state   
*                  		the appropriate handling function is called.    	
*
*  @param[in]    	1)  Pointer to association entry
*				2) Power Save value
*
*  @return 	    	
*  @author		Abd Al-Rahman Diab
*  @date			5 june 2013
*  @version		1.0
*/
void check_ps_state(NMI_ieee80211_sta* ae, STA_PS_STATE_T ps)
{
    /* 1 - POWER_SAVE, 0 - ACTIVE */
    if((ae == NULL ) || (ae->ps_state == ps))
    {
        /* There is no change in station power save mode. Do nothing. */
        return;
    }

    /* Station has changed power save state. Update the entry and handle the */
    /* change as required.                                                   */
    ae->ps_state = ps;

    if(ps == ACTIVE_PS)
    {
        handle_ps_sta_change_to_active(ae);
    }
    else
    {
        handle_ps_sta_change_to_powersave(ae);
    }

}

/*
*  @brief 			filter_wlan_rx_frame	
*  @details 		This function filters the incoming wlan rx packet and    
*                  		sends the results of the operation.  	
*  @param[in]    	1) MAC header of incoming packet. 
*				2) Pointer to rx descriptor
*				3) Pointer to wlan_rx structure 
*  @return 	    	True if the frame is to be discarded, False otherwise
*  @author		Abd Al-Rahman Diab
*  @date			21 APRIL 2013
*  @version		1.0
*/
NMI_Bool filter_wlan_rx_frame(wlan_rx_t *strWlan_rx)
{
    NMI_ieee80211_sta *ae = (NMI_ieee80211_sta *)strWlan_rx->u8sa_entry;
    NMI_Uint8 *msa      = strWlan_rx->u8msa;

    if(strWlan_rx->enumType == DATA_BASICTYPE)
    {
        if(ae == 0)
        {
	     // TODO: handle this station either by reporting to HOSTAPD or sending deauth frame
            /* Send the De-authentication Frame to the station */
            //send_deauth_frame(wlan_rx->ta, error);
	
            /* Return TRUE for filter frame */
            return NMI_TRUE;
        }
        else if (ae->state != ASOC)
        {

	     // TODO: handle this station either by reporting to HOSTAPD or sending disassoc frame
            /* Send the Disassociation Frame to the station */
            //send_disasoc_frame(wlan_rx->ta, error);

            /* Return TRUE for filter frame */
            return NMI_TRUE;
        }
		
	
        /* This STA is active, set the count to 0 */
        ae->aging_cnt = 0;
	

    }
    else
    {
        if(ae != 0)
        {
	
            /* This STA is active, set the count to 0 */
            ae->aging_cnt = 0;

        }
    }
    return NMI_FALSE;
}


/*
*  @brief 			NMI_Process_rx_frame	
*  @details 		handle incoming data & mgmt frames	   	
*  @param[in]    	u8Buff : pointer to frame 
*				u32Size : size of frame buffer
*  @return 	    	1 if it's data frame, 0 otherwise
*  @author		Abd Al-Rahman Diab
*  @date			21 APRIL 2013
*  @version		1.0
*/
void NMI_Process_rx_frame(NMI_Uint8 *u8Buff, NMI_Uint32 u32Size)
{
	wlan_rx_t strWlan_rx = {0};
	NMI_Uint32      min_pkt_len = MAC_HDR_LEN ; //null frame
	NMI_Uint8 * mac_addr = g_linux_wlan->nmc_netdev->dev_addr;

	//skip the host header offset
	u8Buff +=HOST_HDR_OFFSET;

	/* Extract Addresses from the MAC Header */
	get_address1(u8Buff, strWlan_rx.u8addr1);
	get_address2(u8Buff, strWlan_rx.u8addr2);
	get_address3(u8Buff, strWlan_rx.u8addr3);

	strWlan_rx.enumType     = get_type(u8Buff);
	strWlan_rx.u8frm_ds   = get_from_ds(u8Buff);
	strWlan_rx.u8to_ds    = get_to_ds(u8Buff);
	strWlan_rx.u8Sub_type = get_sub_type(u8Buff);
	strWlan_rx.bIs_grp_addr = is_group(strWlan_rx.u8addr1);
	
	strWlan_rx.u16rx_len = u32Size;
	strWlan_rx.u8msa = u8Buff;


	/* Update the SA, DA & BSSID pointers to corresponding addr1, addr2 or   */
	/* addr3 fields of wlan_rx structure.                                    */
	set_SA_DA_BSSID_ptr(&strWlan_rx);

	// TODO:   Update the statistics for get_station
	//update_debug_rx_stats(status);

	/* Exception case 1: Maximum length exceeded */
	if(strWlan_rx.u16rx_len > MAX_MSDU_LEN) 
	{
    		PRINT_D(HOSTAPD_DBG,"HwEr:RxMaxLenExc:%d > %d\n\r",strWlan_rx.u16rx_len, MAX_MSDU_LEN);
    		//g_mac_stats.pwrx_maxlenexc++;
    		/* Do nothing and return */
    		return;
	}


	/* Exception case 2: Protocol Version Match Fail */
	if(get_protocol_version(u8Buff) != PROTOCOL_VERSION) 
	{
		PRINT_D(HOSTAPD_DBG,"HwEr:RxFrmHdrProtVerFail\n\r");
		//g_mac_stats.rxfrmhdrprotverfail++;
    		/* Do nothing and return */
    		return;
	}

    /* Exception case 3: Address-4 Field Present */
	if(check_from_to_ds(&strWlan_rx) == NMI_TRUE) 
	{
		  PRINT_D(HOSTAPD_DBG,"HwEr:RxFrmHdrAddr4Prsnt\n\r");
		  //g_mac_stats.rxfrmhdraddr4prsnt++;
		  /* Do nothing and return */
		  return;
	}



	/* Exception case 4: BSSID Match Fail */
	if(check_bssid_match(&strWlan_rx) == NMI_FALSE) 
	{
	   	 if(strWlan_rx.bIs_grp_addr == NMI_FALSE)
	    		PRINT_D(HOSTAPD_DBG,"HwEr:RxFrmHdrBssidChkFail\n\r");
	    		//g_mac_stats.rxfrmhdrbssidchkfail++;
			/* Do nothing and return */
			return;
	}

	if(strWlan_rx.u8Sub_type == PS_POLL)
	      min_pkt_len = PS_POLL_LEN - FCS_LEN;

	/* Exception case 6: Less than minimum acceptable length */
	if(strWlan_rx.u16rx_len < min_pkt_len) 
	{
		    PRINT_D(HOSTAPD_DBG,"HwEr:RxMinLenExc: %d < %d\n\r", strWlan_rx.u16rx_len, min_pkt_len);
		    //g_mac_stats.pwrx_minlenexc++;
		    /* Do nothing and return */
		    return;
	}

	  // TODO: decide if i need these 2 elements
	  /* Extract required parameters from the frame header */
	  //wlan_rx.priority_val  = get_priority_value(msa);
	  //wlan_rx.service_class = get_ack_policy(msa, wlan_rx.addr1);


	  strWlan_rx.u8sa_entry      = find_entry(strWlan_rx.u8addr2);


	  /* Exception case 8: Not a BC/MC frame and not directed */
	if((strWlan_rx.bIs_grp_addr == NMI_FALSE) &&
	 (mac_addr_cmp(strWlan_rx.u8addr1,mac_addr) == NMI_FALSE) ) 
	 {
		  NMI_Uint8 *addr1 = strWlan_rx.u8addr1;
		  PRINT_D(HOSTAPD_DBG,"HwEr:RxUnexpFrm:%x:%x:%x:%x:%x:%x\n\r",addr1[0],addr1[1],addr1[2],
		  addr1[3],addr1[4],addr1[5]);
		  //g_mac_stats.rxunexpfrm++;
		  /* Do nothing and return */
		  return;
	}

	
	/* Filter the frame based on mode of operation */
	if(filter_wlan_rx_frame(&strWlan_rx) == NMI_TRUE) 
	{
		  //g_mac_stats.pewrxft++;
		  return;
	}

	/* Modify the frame length and get the frame header length depending on  */
  	/* the QoS and HT features enabled.                            */
  	strWlan_rx.u8hdr_len = modify_frame_length( strWlan_rx.u8msa,
                                     	&(strWlan_rx.u16rx_len), &(strWlan_rx.u16data_len));

	
  /* Check for the type of frame and process accordingly */
  if((strWlan_rx.bIs_grp_addr == NMI_FALSE) && (strWlan_rx.enumType == CONTROL)) 
  {
  	// TODO:  we don't handle BA control frames , so we only need to  handle PS POLL frames with Power managemnet
    	    if(handle_ps_poll(&strWlan_rx) == NMI_FALSE)
	    {
	    	// TODO: add support for 11n
	        /* Handle control frames other than PS Poll */
	       // ap_enabled_rx_control((NMI_Uint8 *)req);
	    }
  } 
  else if(strWlan_rx.enumType == MANAGEMENT) 
  {
  	NMI_Uint32* u32header=0;

  	// TODO: update statistics
	    /* Update receive MIB counters */
	   // mincr_ReceivedFragmentCount();

	//Get NMI header
	//u32header = (strWlan_rx.u8msa)-HOST_HDR_OFFSET;
	
#ifdef USE_WIRELESS
	    NMI_WFI_monitor_rx(strWlan_rx.u8msa,strWlan_rx.u16rx_len);
#endif

  } 
  else if(strWlan_rx.enumType == DATA_BASICTYPE) 
  {
  
  // TODO: MEASURE_PROCESSING_DELAY
  /*
#ifdef MEASURE_PROCESSING_DELAY
	    g_delay_stats.numrxdscr += num_dscr;
#endif /* MEASURE_PROCESSING_DELAY */
  

	// TODO: update statistics
	    /* Update receive MIB counters */
	   /* if(wlan_rx.is_grp_addr == NMI_TRUE) {
	      print_log_debug_level_1("\n[DL1][INFO][Rx] {Multicast WLAN Rx}");
	      mincr_MulticastReceivedFrameCount();
	    } else {
	      print_log_debug_level_1("\n[DL1][INFO][Rx] {Unicast WLAN Rx}");
	      mincr_ReceivedFragmentCount();
	    }

	    update_rx_mib_prot(msa, wlan_rx.data_len);*/

	ap_enabled_rx_data(&strWlan_rx);
	
  }

}


/*
*  @brief 			ap_enabled_rx_data
*  @details 	   	This function handles the incoming DATA frame to send to the host.                                   
*  @author		Abd Al-Rahman Diab
*  @date			09 APRIL 2013
*  @version		1.0
*/
void ap_enabled_rx_data(NMI_Uint8* msg)
{
    //NMI_Uint8             priority_rx   = 0;
    STA_PS_STATE_T     ps            = ACTIVE_PS;
    NMI_Uint8             data_trailer  = 0;
    NMI_Uint8*		 buffer = 0 ;
    NMI_Uint8*		host_hdr =0;
    wlan_rx_t          *wlan_rx      = (wlan_rx_t *)msg;
    NMI_Uint8             *msa          = wlan_rx->u8msa;
    NMI_ieee80211_sta       *da_ae        = 0;
    NMI_ieee80211_sta       *sa_ae        = 0;
    struct tx_complete_data* tx_data = NULL;
   // msdu_desc_t        *frame_desc   = 0;
    //CIPHER_T           ct            = (CIPHER_T)wlan_rx->ct;
    //MSDU_PROC_STATUS_T status        = PROC_ERROR;
    TYPESUBTYPE_T      frm_type      = DATA;
    //msdu_indicate_t    msdu          = {{0},};
    //msdu_proc_state_t  msdu_state    = {0};

    /* Get association entry for the source address */
    sa_ae = (NMI_ieee80211_sta *)wlan_rx->u8sa_entry;

	
    /* Power management checks */
    ps       = (STA_PS_STATE_T)get_pwr_mgt(msa);
    frm_type = (TYPESUBTYPE_T)wlan_rx->u8Sub_type;

	// TODO: add PS
    /* Check if the received function is the Null function Packet */
    if((frm_type == NULL_FRAME) ||
       (is_sub_type_null_prot(frm_type) == NMI_TRUE))
    {
        check_ps_state(sa_ae, ps);

//#ifdef DEBUG_MODE
        //g_mac_stats.pewrxnf++;
//#endif /* DEBUG_MODE */
       return;
    }

    /* Get the priority of the incoming frame */
    //priority_rx   = wlan_rx->priority_val;
    //msdu.priority = priority_rx;


    /* Check for WMM-PS trigger frame and process accordingly */
    //pwr_mgt_handle_prot(sa_ae, ps, priority_rx, msa);
    
	// TODO: add PS
    /* Check the Power Save Bit in the receive frame */
   check_ps_state(sa_ae, ps);

    /* Create the MSDU descriptors for the received frame */
	
        /* Create the MSDU decsriptor */
        //status = update_msdu_info(wlan_rx, &msdu, &msdu_state);


        /* Get the frame descriptor pointer */
        //frame_desc = &(msdu.frame_desc);

        /* Get association entry based on the source address to determine    */
        /* the next path of the packet.        */
        sa_ae  = (NMI_ieee80211_sta* )find_entry(wlan_rx->u8sa);
        da_ae  = (NMI_ieee80211_sta* )find_entry(wlan_rx->u8da);

        if(da_ae != 0)
        {
            //NMI_Uint8 key_type        = 0;
            //NMI_Bool wlan2wlan       = NMI_FALSE;
            NMI_Bool use_same_buffer = NMI_TRUE;

            /* CipherType is reset to no encryption and is set as per the    */
            /* policy used for the out going sta                             */
            //ct  = NO_ENCRYP;

            /* Set the Key type required for transmission */
            //key_type = UCAST_KEY_TYPE;

            /* If the station is associated with the AP, the packet is put   */
            /* onto the wireless network.                                    */
            if(da_ae->state == ASOC)
            {

				// TODO: recheck this 
				/* If the received frame is not a QoS frame, create a    */
				/* copy of the frame in a separate buffer to fit the QoS */
				/* header.                                               */
				//if(is_qos_bit_set(msa) == NMI_FALSE)
					//use_same_buffer = NMI_FALSE;

                /* The MAC Header offset field in TX-Dscr is 8 bits long. */
                /* Hence a frame copy is created if the MAC Header field  */
                /* exceeds 256 when it is created in place.               */
                //if(frame_desc->data_offset > 255)
                       // use_same_buffer = NMI_FALSE;


		// TODO: security stuff
                /* Before forwarding the packet onto the WLAN interface      */
                /* security checks needs to performed on the states of the   */
                /* transmitting and receiving stations                       */
                //wlan2wlan = check_sec_ucast_wlan_2_wlan_ap(da_ae, sa_ae, &ct,
                                                           //&data_trailer);

                /* Update length of the frame to accommodate the security    */
                /* trailers if any.                                          */
                //update_frame_length(frame_desc, data_trailer);

            //if(wlan2wlan == NMI_TRUE)
            //{

		// send this packet to wlan interface
           	tx_data = (struct tx_complete_data*)kmalloc(sizeof(struct tx_complete_data),GFP_ATOMIC);

			if(tx_data == NULL){
				PRINT_ER("Failed to allocate memory for tx_data structure\n");
		        return ;		
			}	

	        /* Set Address1 field in the WLAN Header with destination address */
	        set_address1(msa, wlan_rx->u8da);

	        /* Set Address2 field in the WLAN Header with the BSSID */
	        set_address2(msa, wlan_rx->u8bssid);

	        /* Set Address2 field in the WLAN Header with the source address */
	        set_address3(msa, wlan_rx->u8sa);

			/*{int i ;
				for(i = 0; i < wlan_rx->u16rx_len; i++)
				    {
			
				      	printk("msa[%d] = %2x \n", i, msa[i]);
				    }
			}*/
			
		    	tx_data->buff =msa;
			tx_data->size = wlan_rx->u16rx_len;
			tx_data->skb  = NULL;
                NMI_Xmit_data((void*)tx_data, WLAN_TO_WLAN);
				
               // }

		// TODO: update stat
                //g_mac_stats.pewrxu++;


            }

        }
        else
        {
            /* Broadcast/Multicast frames need to be forwarded on WLAN also */
            if(is_group(wlan_rx->u8da) == NMI_TRUE)
            {
                //CIPHER_T grp_ct    = NO_ENCRYP;

                /* Before forwarding the packet on WLAN and HOST interface   */
                /* security checks needs to performed on the states of the   */
                /* transmitting station                                      */
               // if(ap_check_sec_tx_sta_state(sa_ae) != NMI_TRUE)
                //{
                  //  continue;
                //}

//#ifdef DEBUG_MODE
               // g_mac_stats.pewrxb++;
//#endif /* DEBUG_MODE */

                /* Before forwarding the packet on the WLAN interface        */
                /* security checks needs to performed on the states of the   */
                /* transmitting station                                      */
                //data_trailer = check_sec_bcast_wlan_2_wlan_ap(&grp_ct);

                /* Update the length of the frame to accommodate the         */
                /* security trailers if any.                                 */
                //update_frame_length(frame_desc, data_trailer);


                tx_data = (struct tx_complete_data*)kmalloc(sizeof(struct tx_complete_data),GFP_ATOMIC);

			if(tx_data == NULL){
				PRINT_ER("Failed to allocate memory for tx_data structure\n");
		        return ;		
			}	

	        /* Set Address1 field in the WLAN Header with destination address */
	        set_address1(msa, wlan_rx->u8da);

	        /* Set Address2 field in the WLAN Header with the BSSID */
	        set_address2(msa, wlan_rx->u8bssid);

	        /* Set Address2 field in the WLAN Header with the source address */
	        set_address3(msa, wlan_rx->u8sa);

			/*{int i ;
				for(i = 0; i < wlan_rx->u16rx_len; i++)
				    {
			
				      	printk("msa[%d] = %2x \n", i, msa[i]);
				    }
			}*/

    
		    	tx_data->buff =msa;
			tx_data->size = wlan_rx->u16rx_len;
			tx_data->skb  = NULL;
                NMI_Xmit_data((void*)tx_data, WLAN_TO_WLAN);
				
            }
            //else
            //{
                /* Packet is a Unicast packet to the AP */
//#ifdef DEBUG_MODE
                //g_mac_stats.pewrxu++;
//#endif /* DEBUG_MODE */
            //}

            /* Check for SNAP header at beginning of the data and set the    */
            /* data pointer and length accordingly.                          */
            if(NMI_TRUE == is_snap_header_present(wlan_rx))
            {
                /* If received packet is a security handshake packet process */
                /* it in the security layer                                  */
                /*if(is_sec_handshake_pkt_ap(sa_ae, frame_desc->buffer_addr,
                                            frame_desc->data_offset,
                                            frame_desc->data_len,
                                            (CIPHER_T)wlan_rx->ct) == NMI_TRUE)
                {
                    continue;
                }*/

                /* Adjust the frame to account for the SNAP header */
                adjust_for_snap_header(wlan_rx);
            }
            //else
            //{
                /* Before forwarding the packet on WLAN and HOST interface   */
                /* security checks needs to performed on the states of the   */
                /* transmitting station                                      */
                //if(ap_check_sec_tx_sta_state(sa_ae) != NMI_TRUE)
               // {
                    //continue;
                //}
           // }

            /* Call MSDU Indicate API with the MSDU to be sent to the host */

            //msdu_indicate_ap(&msdu);
            host_hdr   = msa  + ((wlan_rx->u8hdr_len) - ETHERNET_HDR_LEN);

		NMI_memcpy(host_hdr, wlan_rx->u8da, ETH_ALEN);
        	NMI_memcpy(host_hdr+6, wlan_rx->u8sa, ETH_ALEN);
			
            buffer = msa + wlan_rx->u8hdr_len - ETHERNET_HDR_LEN ; 

  		frmw_to_linux(buffer, (wlan_rx->u16data_len) + ETHERNET_HDR_LEN);

        }

}


static void linux_FH_wlan_tx_complete(void* priv, int status){

	struct tx_complete_data* pv_data = (struct tx_complete_data*)priv;
	if(status == 1){
		PRINT_D(TX_DBG,"Packet sent successfully - Size = %d - Address = %p - SKB = %p\n",pv_data->size,pv_data->buff, pv_data->skb);
	} else {
		PRINT_D(TX_DBG,"Couldn't send packet - Size = %d - Address = %p - SKB = %p\n",pv_data->size,pv_data->buff, pv_data->skb);
	}

	
	/*{
	int i;
	printk("tx_data = %x\n",pv_data);
	printk("data_len = %d\n",pv_data->size);
	for(i = 0 ; i < pv_data->size ; i++)
		printk("msa3[%d] = %x\n",i,((NMI_Uint8*)(pv_data->buff))[i]);
	}	*/

	/* Power Management */
       handle_ps_tx_comp_ap(pv_data);
	
    /* Free the SK Buffer, its work is done */
	if(pv_data->skb)
		dev_kfree_skb(pv_data->skb);	
		
	linux_wlan_free(pv_data);
}

/*
*  @brief 			NMI_Xmit_data
*  @details 	   	This function transmit the data frames to firmware. 
*  @Processing       This function computes hash value of a given MAC address. 
*				This value is used to access the hash table. 
*  @return 	    	hash value                                    
*  @author		Abd Al-Rahman Diab
*  @date			09 APRIL 2013
*  @version		1.0
*/
int NMI_Xmit_data(void* ptx_data, Data_Interface InterfaceType)
{
	struct tx_complete_data* tx_data = (struct tx_complete_data*)ptx_data;
	struct sk_buff 		*skb = tx_data-> skb;
	struct sk_buff 		*xmit_skb; 
	struct ethhdr 			*eth;
	NMI_ieee80211_sta	*asoc_entry = NULL;
	NMI_Uint8 			ethsaddr[ETH_ALEN];
	NMI_Uint8			ethdaddr[ETH_ALEN];
	NMI_Uint8			mac_hdr_len;
	NMI_Uint8*			da;
	NMI_Uint8	 		service_class, is_qos,is_htc, is_amsdu;
	NMI_Uint8			priority = BEST_EFFORT_PRIORITY; // TODO: this should not be fixed
	NMI_Uint8* 			BSSID = g_linux_wlan->nmc_netdev->dev_addr;
	NMI_Uint8* 			snap_hdr;
	NMI_Uint8* 			mac_hdr;
	NMI_Uint8* 			data_start;
	NMI_Uint16 			eth_type;
	NMI_Uint16 			pkt_len = 0;
	NMI_Uint16 			data_len =0;
	NMI_Uint16			u16size=0;

	int 					QueueCount =0;

	//if(nic->oup.wlan_add_to_tx_que == NULL){printk("g_wlan == null return \n\n");
		//return;}
	
	/*int i;
	printk("skb->len = %d\n",skb->len);
	for(i = 0 ; i < skb->len ; i++)
		printk("%x\n",(skb->data)[i]);*/
		
	if (InterfaceType == HOST_TO_WLAN)
	{
		pkt_len = skb->len;
		
		eth = (struct ethhdr *)(skb->data);

		/* Set the destination address field in the WLAN Tx Request structure. */
		NMI_memcpy(ethdaddr, &eth->h_dest, ETH_ALEN);

	    	/* For AP, the source address should be sent with the frame. */
		NMI_memcpy(ethsaddr, &eth->h_source, ETH_ALEN);


		/* Here check whether the destination is associated,   */
	    	/* If its not associated, then drop the packet              */
	   
		/* For unicast packets, check if the destination STA is associated. */
	    	if(is_group(ethdaddr) == NMI_FALSE)
	    	{
		        /* Get association entry for association state */
		        asoc_entry = (NMI_ieee80211_sta*)find_entry(ethdaddr);

		        /* If the destination STA is not associated free the data packet and */
		        /* return.                                                           */
	        	if((asoc_entry == 0) || (asoc_entry->state != ASOC))
	        	{
	        		//Wait for station to be associated
	        		
	            		/* Free the SK Buffer */
	    			dev_kfree_skb(skb);	
				linux_wlan_free(tx_data);
	            		return ;
	        	}

			service_class = NORMAL_ACK;
	    	}
		else
		{
			service_class = BCAST_NO_ACK;
		}
		
		// allocate new skb to accomodate the MAC header
		xmit_skb = dev_alloc_skb((skb->len) +ETH_ETHERNET_HDR_OFFSET);

		
		 /* Check whether QoS option is implemented */
		 // TODO: 11n is not enabled yet
	        is_qos = 0; //is_qos_required(sta_index);

	        /* Check whether receiver is HT capable */
		// HTC is not supported yet
	        is_htc  = 0;

		// we currently don't support amsdu
		is_amsdu = 0;

		mac_hdr = xmit_skb->data;

		
	}
	else if(InterfaceType == WLAN_TO_WLAN)
	{
		// TODO: try to eliminate the copying here to improve performance
		
		skb=dev_alloc_skb( tx_data->size );

		memcpy(skb->data , tx_data->buff, tx_data->size );


		/* frames coming from wlan are freed within vmm handle rxq
			null frames from ps-poll are freed here 
			*/
		if(get_sub_type(tx_data->buff) == NULL_FRAME)
		{
			kfree(tx_data->buff);
		}

		 /* Check whether receiver is HT capable */
		// HTC is not supported yet
	        is_htc  = 0;

		// we currently don't support amsdu
		is_amsdu = 0;

		mac_hdr = skb->data;

		#if 0
		if(is_group(da) == NMI_FALSE)
	    	{
			service_class = NORMAL_ACK;
	    	}
		else
		{
			service_class = BCAST_NO_ACK;
		}
		#endif
		
		tx_data->buff = skb->data;
		tx_data->skb  = skb;

		/*{int i ;
				for(i = 0; i < tx_data->size; i++)
				    {
			
				      	printk("msa2[%d] = %2x \n", i, mac_hdr[i]);
				    }
			}*/

	}
	else if(InterfaceType == MONITOR_TO_WLAN)
	{

		mac_hdr = tx_data->buff;

		
		/* Get the Destination address */
    		da = get_DA_ptr(mac_hdr);

		/* Get association entry for association state */
		asoc_entry = (NMI_ieee80211_sta*)find_entry(da);

		
		if(buffer_tx_packet(asoc_entry, da, priority , tx_data) == NMI_FALSE)
	    	{
			nmi_wlan_txq_add_mgmt_pkt((void*)tx_data,tx_data->buff,tx_data->size,linux_FH_wlan_tx_complete);
		}

		return;
			
	}

        mac_hdr_len = set_mac_hdr_prot(mac_hdr, priority,
                                   service_class, is_qos,
                                   is_htc, is_amsdu);
		
	set_from_ds(mac_hdr, 1);

	if (InterfaceType == HOST_TO_WLAN)
	{

		 // TODO: add security 
	        //if(ct != NO_ENCRYP)
	            //set_wep(mac_hdr, 1);

	        /* Set Address1 field in the WLAN Header with destination address */
	        set_address1(mac_hdr, ethdaddr);

	        /* Set Address2 field in the WLAN Header with the BSSID */
	        set_address2(mac_hdr, BSSID);

	        /* Set Address2 field in the WLAN Header with the source address */
	        set_address3(mac_hdr, ethsaddr);

		/* Get the Destination address */
    		da = get_DA_ptr(mac_hdr);

		
		/* SNAP header needs to be set for IP/ARP packets. Note that there is    */
	    	/* sufficient space allocated for SNAP and MAC header in the buffer.     */

	    	/* Extract the type of the ethernet packet and set the SNAP header       */
	    	/* contents. Also set the data pointer field in the WLAN Tx Request      */
	    	/* structure, as required, to the correct value.                         */

		#ifdef LITTLE_ENDIAN
	   	eth_type= eth->h_proto;
		#else 
		eth_type	= ( eth->h_proto>>8) | ( eth->h_proto<<8);
		#endif
		//printk("eth->h_proto= %x \n or eth_type =%x \n",eth->h_proto, ( eth->h_proto>>8) | ( eth->h_proto<<8));
		//printk("eth_type =%x \n",eth_type);

		if((eth_type == ARP_TYPE)   ||
	       (eth_type == IP_TYPE)    ||
	       (eth_type == ONE_X_TYPE) ||
	       (eth_type == VLAN_TYPE)  ||
	       (eth_type == LLTD_TYPE))
	    {
	        /* The SNAP header is set before the ethernet payload.               */
	        /*                                                                   */
	        /* +--------+--------+--------+----------+---------+---------------+ */
	        /* | DSAP   | SSAP   | UI     | OUI      | EthType | EthPayload    | */
	        /* +--------+--------+--------+----------+---------+---------------+ */
	        /* | 1 byte | 1 byte | 1 byte | 3 bytes  | 2 bytes | x bytes       | */
	        /* +--------+--------+--------+----------+---------+---------------+ */
	        /* <----------------  SNAP Header  ---------------->                 */
	        /* <------------------------ 802.11 Payload -----------------------> */
	        snap_hdr = xmit_skb->data +  mac_hdr_len;


	        *snap_hdr++ = 0xAA;
	        *snap_hdr++ = 0xAA;
	        *snap_hdr++ = 0x03;
	        *snap_hdr++ = 0x00;
	        *snap_hdr++ = 0x00;
	        *snap_hdr++ = 0x00;
		 *snap_hdr++ = (NMI_Uint8)((eth_type>>8) &0x00ff);
		 *snap_hdr++ = (NMI_Uint8)(eth_type &0x00ff);

	        /* Set the data length parameter to the MAC data length only (does   */
	        /* not include headers)                                              */
	       data_len = pkt_len + mac_hdr_len - ETHERNET_HDR_LEN + SNAP_HDR_LEN;

		data_start = snap_hdr;
		
	        /* Note that the Ethernet Type field is already set in the ethernet  */
	        /* header and follows this.                                          */
	    }
	    else
	    {
	        /* Set the data length parameter to the MAC data length only (does   */
	        /* not include headers)                                              */
	        data_len = pkt_len+ mac_hdr_len  - ETHERNET_HDR_LEN;

		data_start = xmit_skb->data +  mac_hdr_len;
	    }

		NMI_memcpy(data_start, (skb->data) + ETHERNET_HDR_LEN , pkt_len - ETHERNET_HDR_LEN);

		dev_kfree_skb(skb);

		tx_data->buff = xmit_skb->data;
		tx_data->size = data_len;
		tx_data->skb  = xmit_skb;

	}
	else
	{	
		/* Get the Destination address */
    		da = get_DA_ptr(mac_hdr);
		
		/* Get association entry for association state */
		asoc_entry = (NMI_ieee80211_sta*)find_entry(da);
	}
	
	/*if (InterfaceType == WLAN_TO_WLAN)
	{
	int i;
	printk("tx_data = %x\n",tx_data);
	printk("data_len = %d\n",tx_data->size);
	for(i = 0 ; i < tx_data->size ; i++)
		printk("msa2[%d] = %x\n",i,((NMI_Uint8*)(tx_data->buff))[i]);
	}	*/
	
	/*QueueCount = nmi_wlan_txq_add_mgmt_pkt((void*)tx_data,
									tx_data->buff,
									tx_data->size,
									linux_FH_wlan_tx_complete);*/

	//QueueCount = nmi_FH_wlan_txq_add_net_pkt(tx_data,tx_data->buff,tx_data->size,linux_FH_wlan_tx_complete);

	
		  
	if(buffer_tx_packet(asoc_entry, da, priority , tx_data) == NMI_FALSE)
    	{
		QueueCount = nmi_FH_wlan_txq_add_net_pkt((void*)tx_data,
										tx_data->buff,
										tx_data->size,
										linux_FH_wlan_tx_complete);
	}

	return QueueCount;
}



/*
*  @brief 			hash
*  @details 	   	This function computes the hash value for given address. 
*  @Processing       This function computes hash value of a given MAC address. 
*				This value is used to access the hash table. 
*  @return 	    	hash value                                    
*  @author		Abd Al-Rahman Diab
*  @date			09 APRIL 2013
*  @version		1.0
*/
NMI_Uint32 hash(NMI_Uint8* addr)
{
    NMI_Uint32 sum = 0;
    NMI_Uint32  i   = 0;	

    for(i = 0; i < 6; i++)
        sum += addr[i];

    return sum % MAX_HASH_VALUES;
}

/*
*  @brief 			find_entry
*  @details 	   	This function searches the table for an entry having the given key value.
*  @Processing       The hash value (val) for the given key is computed    
*                     	using the hash function. The table entry holding the  
*                    	given key is then searched.  If such an entry is      
*                     	found the pointer to the entry stored in the table    
*                     	entry is returned.  
*  @return 	    	The element of the corresponding table entry, if an entry , having a key
*				identical to the given key was found.                                        
*                    	Zero is returned if no matching entry is found.
*  @author		Abd Al-Rahman Diab
*  @date			09 APRIL 2013
*  @version		1.0
*/	
void* find_entry(NMI_Uint8* key)
{
    table_elmnt_t *tbl_ptr  = 0;
    NMI_Uint32        val      = 0;

    /* Zero entry is fixed for Bcast/Mcast address     */
    /* If the address is BCast/MCast Address, return 0 */
    if(key[0] & BIT0)
    {
        return 0;
    }


    /* Calculate hash value for the key */
    val = hash(key);
    tbl_ptr = g_sta_table[val];

    while(tbl_ptr)
    {
        if(mac_addr_cmp(tbl_ptr->key, key) == NMI_TRUE)
            return tbl_ptr->element;

        tbl_ptr = tbl_ptr->next_hash_elmnt;
    }

    return 0;
}



/*
*  @brief 			add_entry
*  @details 	   	This function adds an entry to asoc table..
*  @Processing       A table element is created. The entry and key  values 
*	                     are set in table element and the table element is     
*                    	inserted in the appropriate hash bucket of association
*                    	table.                                                
*                                                                           
*                    	There is no maximum limit set for creation of assoc   
*                    	entries. But there is a maximum limit for number of   
*                    	associated stations, MAX_ELEMENTS.                    
*  @author		Abd Al-Rahman Diab
*  @date			09 APRIL 2013
*  @version		1.0
*/	
void add_entry(void* entry, NMI_Uint8* key)
{
    NMI_Uint32       val              = 0;
    table_elmnt_t *new_elm         = 0;

    /* Create a buffer to hold table element */
    new_elm = (table_elmnt_t*)kmalloc( sizeof(table_elmnt_t),GFP_ATOMIC);	
             
    if(new_elm == NULL)
    {
        return;
    }

    /* Compute the hash value */
    val = hash(key);

    /* Insert the element at head of the hash */
    NMI_memcpy(new_elm->key, key, ETH_ALEN);
	
    new_elm->next_hash_elmnt = g_sta_table[val];
    new_elm->element         = entry;

    g_sta_table[val]        = new_elm;
}


/*
*  @brief 			delete_entry
*  @details 	   	This function deletes the entry having the given key  
*				value from the table. The space occupied by the entry 
*				is not freed here. 
*
*  @Processing       The hash value (val) for the given key is computed    
*                     	using the hash function. The table entry holding the  
*                    	given key is then searched.  If such an entry is     
*                    	found the corresponding table entry is removed from   
*                   	  	the hash list and freed. If no matching entry is      
*                     	found no action is taken.                      
*  @author		Abd Al-Rahman Diab
*  @date			09 APRIL 2013
*  @version		1.0
*/	
void delete_entry(NMI_Uint8* key)
{
    table_elmnt_t *tbl_ptr  = 0;
    table_elmnt_t *prev_ptr = 0;
    NMI_Uint32        val      = 0;

    /* Calculate hash value for the key */
    val = hash(key);

    /* Initialize temporary pointers to be used in the loop below */
    tbl_ptr  = g_sta_table[val];
    prev_ptr = g_sta_table[val];

    /* Given the hash bucket, find out the entry that has the key */
    while(tbl_ptr)
    {
        if(mac_addr_cmp(tbl_ptr->key, key) == NMI_TRUE)
        {
            /* Got a match. Modify the next pointer of the element pointing  */
            /* to this element.                                              */
            if(tbl_ptr == g_sta_table[val])
                /* The element to be freed is the first element */
                g_sta_table[val] = tbl_ptr->next_hash_elmnt;
            else
                /* The element to be freed is not the first element */
                prev_ptr->next_hash_elmnt = tbl_ptr->next_hash_elmnt;

		// TODO: recheck this
            /* Free the element referred to by this key */
            //delete_element(tbl_ptr->element);
		 kfree(tbl_ptr->element);
		
            /* Free this element */
            kfree(tbl_ptr);

            return;
        }

        prev_ptr = tbl_ptr;
        tbl_ptr = tbl_ptr->next_hash_elmnt;
    }
}


/*****************************************************************************/
/*                                                                           */
/*  Function Name : handle_ps_tx_comp_ap                                     */
/*                                                                           */
/*  Description   : This function handles transmit complete interrupt for    */
/*                  power management purpose.                                */
/*                                                                           */
/*  Inputs        : 1) Pointer to the transmit descriptor                    */
/*                                                                           */
/*  Globals       : g_num_mc_bc_pkt                                          */
/*                                                                           */
/*  Processing    : This function checks if the frame transmitted was a      */
/*                  broadcast/multicast frame. If so it reduces the global   */
/*                  bc/mc packet count in case it is non-zero (indicating    */
/*                  that the packets were queued). If it is unicast the      */
/*                  association entry is searched and corresponding number   */
/*                  of packets are decremented if non-zero (indicating they  */
/*                  were queued).                                            */
/*                                                                           */
/*  Outputs       : None                                                     */
/*  Returns       : None                                                     */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         24 10 2005   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
void handle_ps_tx_comp_ap(struct tx_complete_data *tx_dscr )
{
    NMI_ieee80211_sta *ae;
    NMI_Uint8 *msa  = tx_dscr->buff;
    NMI_Uint8  *da   = NULL;

    /* Get the Destination address */
    da = get_DA_ptr(msa);

    ae = (NMI_ieee80211_sta*)find_entry(da);
	
    if(is_group(da) ==  NMI_TRUE)
    {

        if(g_num_mc_bc_qd_pkt > 0)
        {
            g_num_mc_bc_qd_pkt--;

            /* If no packets are queued in Sw PSQ or Hw for the */
            /* station, reset DTIM bit                          */
            if((g_num_mc_bc_pkt + g_num_mc_bc_qd_pkt) == 0)
            {
                reset_dtim_bit(AID0_BIT);
                reset_dtim_bit(DTIM_BIT);
            }
        }
    }
    else
    {
        if(ae == 0)
        {
            /* Exception. Should not happen */
            return;
        }

        /* Reset the age counter as this STA is active */
        ae->aging_cnt = 0;

        /* Get the type of frame transmitted */
        if(get_type(msa) == (BASICTYPE_T)(DATA))
        {
            if(NMI_TRUE == update_ps_counts_txcomp(ae, msa))
            {
                check_and_reset_tim_bit(ae->u16AID);
            }
        }

    }
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : check_and_reset_tim_bit                                  */
/*                                                                           */
/*  Description   : This function sets the TIM bit corresponding to the      */
/*                  association ID in virtual bit map.                       */
/*                                                                           */
/*  Inputs        : 1) Association ID                                        */
/*                                                                           */
/*  Globals       : g_vbmap                                                  */
/*                                                                           */
/*  Processing    : This function is called whenever a packet is removed     */
/*                  from the power save queue for a sleeping station. Given  */
/*                  the association ID, set the bit corresponding to that.   */
/*                  Recalculate length and offset fields and update the      */
/*                  the virtual bit map.                                     */
/*                                                                           */
/*  Outputs       : None                                                     */
/*  Returns       : None                                                     */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         24 10 2005   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/

void check_and_reset_tim_bit(NMI_Uint16 asoc_id)
{
    NMI_Uint8  byte_offset    = 0;
    NMI_Uint8  bit_offset     = 0;
    NMI_Uint8  pvb_offset     = 0;
    NMI_Uint8  length         = 0;
    NMI_Uint8  new_pvb_offset = 0;
    int16_t	 	i              = 0;
    NMI_Uint8  new_length     = MIN_TIM_LEN;

    /* Traffic Indication Virtual Bit Map within the AP, generates the TIM   */
    /* such that if a station has buffered packets, then the corresponding   */
    /* bit (which can be found from the association ID) is set. The byte     */
    /* offset is obtained by dividing the association ID by '8' and the bit  */
    /* offset is the remainder of the association ID when divided by '8'.    */
    byte_offset = (asoc_id & (~0xC000)) >> 3;
    bit_offset  = (asoc_id & (~0xC000)) & 0x07;

    /* Calculate the current byte offset in vbmap */
    pvb_offset  = (strBeaconInfo.u8vbmap[BMAP_CTRL_OFFSET] & 0xFE) >> 1;
    length      = strBeaconInfo.u8vbmap[LENGTH_OFFSET];

    /* Reset the TIM bit */
    strBeaconInfo.u8vbmap[TIM_OFFSET + byte_offset] &= ~g_bmap[bit_offset];

    /* Calculate new offset using the following algorithm:                   */
    /* The new offset will be equal to or greater than the current offset as */
    /* the TIM bit is reset. Also the new TIM length cannot be more the      */
    /* existing length. So the algorithm is starting from old offset to      */
    /* old offset + TIM length, find out the first occurance of non-zero byte*/
    /* in the TIM element array. This will be the new offset. If no non-zero */
    /* bytes are found, new offset is zero (as initialized).                 */
    for(i = pvb_offset; i < pvb_offset + length - MIN_TIM_LEN; i++)
    {
        if(strBeaconInfo.u8vbmap[TIM_OFFSET + i])
        {
            new_pvb_offset = i;
            break;
        }
    }

    /* Calculate new length using the following algorithm                    */
    /* The new TIM length will be less than or equal to old length as TIM bit*/
    /* is reset. Having new offset calculated, the new length can be         */
    /* calculated as the last occurance of non-zero byte from new offset to  */
    /* new offset+old length in the TIM element array. (In other words the   */
    /* first occurance of non-zero byte in the reverse direction)            */
    for(i = pvb_offset + length - MIN_TIM_LEN; i >= 0; i--)
    {
        if(strBeaconInfo.u8vbmap[TIM_OFFSET + i])
        {
            new_length += (i - new_pvb_offset);
            break;
        }
    }

    /* Assign the new offset and length to the vbmap */
    strBeaconInfo.u8vbmap[BMAP_CTRL_OFFSET] |= (new_pvb_offset & 0xEF) << 1;
    strBeaconInfo.u8vbmap[LENGTH_OFFSET]     = new_length;
}



/*
*  @brief 			delete_entry
*  @details 	   	This function requeues a packet in the H/w queue for     
*                 		transmission after removing it from the S/w queue where  
*                  		it was buffered for power management purpose. 
*
*  @Processing       First the H/w queue is checked to see if it is full.     
*                  		If H/w queue is full then pkt is not requeued.           
*                  		If H/w queue is available then pkt is requeued.          
*                  		A packet is removed from the head of the given power     
*                  		save queue. The transmit descriptor is extracted and     
*                  		added to the H/w queue. The power save queue element     
*                  		buffer is freed.                     
*  @author		Abd Al-Rahman Diab
*  @date			05 june 2013
*  @version		1.0
*/	
REQUEUE_STATUS_T requeue_ps_packet(NMI_ieee80211_sta* ae, list_buff_t *qh,
                                   NMI_Bool ps_q_legacy, NMI_Bool eosp)
{
    NMI_Uint8   q_num        = 0;
    NMI_Uint8   *mac_hdr     = 0;
    NMI_Uint8   addr1[6]     = {0};
    NMI_Uint8   tx_rate      = 0;
    NMI_Uint8   pream        = 0;
    NMI_Uint8   srv_cls      = 0;
    NMI_Uint8   tid          = 0;
    NMI_Uint32  phy_tx_mode  = 0;
    NMI_Bool   is_qos       = NMI_FALSE;
    NMI_Uint32  retry_set[2] = {0};
    struct tx_complete_data   *tx_dscr     = 0;

    /* Remove the power save queue element structure from the head of the    */
    /* given queue.                                                          */
    tx_dscr = remove_list_element_head(qh);

    /* If the queue has no packets return NMI_FALSE to indicate that no packets */
    /* were available to be requeued.                                        */
    if(tx_dscr == 0)
    {
        return NO_PKT_IN_QUEUE;
    }

    /* Extract the pointer to the MAC header from the Tx-Descriptor */
    mac_hdr = tx_dscr->buff;



    /* Extract DA, TID and QoS from the MAC header of the MSDU */
    get_address1(mac_hdr, addr1);
    tid = BEST_EFFORT_PRIORITY;//get_priority_value(mac_hdr);
    is_qos = is_qos_bit_set(mac_hdr);



    /* Peek and check if the queue is empty. If so, set the more data bit to */
    /* 0. Otherwise set it to 1.                                             */
    if(peek_list(qh) == NULL)
    {

        if(is_qos == NMI_TRUE)
        {
        // TODO: look at this comment if you will handle addba & delba
	// make sure this is not a management frame with Action subtype and BA category
	//if((is_qos == NMI_TRUE) && ( !((get_sub_type(mac_hdr) == ACTION ) && (mac_hdr[MAC_HDR_LEN+CATEGORY_OFFSET] == BA_CATEGORY))))
            set_qos_prot(mac_hdr);
        }

        set_more_data(mac_hdr, 0);
    }
    else
    {
        set_more_data(mac_hdr, 1);
    }

#if 0
    /* Currently disabled */
    /* If current sp length = max sp length-1 eosp bit set and END_OF_QUEUE  */
    /* is returned                                                           */
    if(ps_q_legacy == NMI_FALSE)
    {
        if(is_end_prot(ae) == NMI_TRUE)
        {
            set_qos_prot(mac_hdr);
            end_of_q = NMI_TRUE;
        }
    }
#endif

    /* If current sp length = max sp length-1 eosp bit set and END_OF_QUEUE  */
    /* is returned                                                           */
    if(NMI_TRUE == eosp)
        set_qos_prot(mac_hdr);



    /* Update PS queue counters */
    if(ps_q_legacy == NMI_TRUE)
    {
	    if(is_group(addr1) == NMI_TRUE)
	    {
	        g_num_mc_bc_pkt--;
	    }
	    else
	    {
	        ae->num_ps_pkt--;
	    }
    }
    else
    {
	    //ae->num_ps_pkt_del_ac--;
    }
	g_num_ps_pkt--;

    /* Check if the packet needs to be buffered for BA. If Block ACK session */
    /* is active then donot queue in the HW queue.                           */
    /* When STA has come out of sleep then BA is !HALTED                     */

	// TODO: add this check when enabling 11n
    //if(NMI_FALSE == is_serv_cls_buff_pkt((NMI_Uint8 *)ae, q_num, tid, tx_dscr))
    {	

	// TODO: add EAPOL and other monitor types check here
	if(get_sub_type(tx_dscr->buff) == NULL_FRAME)
	{
		nmi_wlan_txq_add_mgmt_pkt((void*)tx_dscr,
									 tx_dscr->buff,
									 tx_dscr->size,
									 linux_FH_wlan_tx_complete);
	}
	else
	{
	    	 nmi_FH_wlan_txq_add_net_pkt((void*)tx_dscr,
										tx_dscr->buff,
										tx_dscr->size,
										linux_FH_wlan_tx_complete);
	}
    	//PRINTK("QMU ADD PS PKT\n");
        /* Queue the frame for transmission */
        //if(qmu_add_tx_packet(&g_q_handle.tx_handle, q_num, tx_dscr) != QMU_OK)
        //{
            /* Exception. Do nothing. */
//#ifdef DEBUG_MODE
        //    g_mac_stats.qaexc++;
//#endif /* DEBUG_MODE */
           // free_tx_dscr((NMI_Uint32 *)tx_dscr);

           // return RE_Q_ERROR;
       // }

		/* Update UC/BC/MC Hw queue packet count */
        if(is_group(addr1) == NMI_TRUE)
        {
            g_num_mc_bc_qd_pkt++;
        }
        else
        {
            ae->num_qd_pkt++;
        }

    }

    return PKT_REQUEUED;
}


/*
*  @brief 			set_tim_bit
*  @details 	   	This function sets the TIM bit corresponding to the    
*                  		association ID in virtual bit map.  
*
*  @Processing       This function is called whenever a new packet is added   
*                  		 to the power save queue for a sleeping station. Given    
*                  		 the association ID, set the bit corresponding to that.   
*                 		 If this bit is not already included in the offset and    
*                  		 length fields of virtual bit map adjust offset and       
*                  		 length fields.   
*                  		              
*  @author		Abd Al-Rahman Diab
*  @date			05 june 2013
*  @version		1.0
*/	
void set_tim_bit(NMI_Uint16 asoc_id)
{
    NMI_Uint8 byte_offset = 0;
    NMI_Uint8 bit_offset  = 0;
    NMI_Uint8 pvb_offset  = 0;
    NMI_Uint8 length      = 0;

    /* Traffic Indication Virtual Bit Map within the AP, generates the TIM   */
    /* such that if a station has buffered packets, then the corresponding   */
    /* bit (which can be found from the association ID) is set. The byte     */
    /* offset is obtained by dividing the association ID by '8' and the bit  */
    /* offset is the remainder of the association ID when divided by '8'.    */
    byte_offset = (asoc_id & (~0xC000)) >> 3;
    bit_offset  = (asoc_id & (~0xC000)) & 0x07;

    /* Calculate the current byte offset in vbmap */
    pvb_offset  = (strBeaconInfo.u8vbmap[BMAP_CTRL_OFFSET] & 0xFE) >> 1;
    length      = strBeaconInfo.u8vbmap[LENGTH_OFFSET];

    /* Compare the existing offset and the offset for the new STA. Create    */
    /* new length and offset and add that to vbmap.                          */
    if(byte_offset < pvb_offset)
    {
        pvb_offset = byte_offset;
        length     = pvb_offset - byte_offset;
    }
    else if(byte_offset > pvb_offset + length - MIN_TIM_LEN)
    {
        length += byte_offset - pvb_offset;
    }

    /* Set the TIM bit and length */
    strBeaconInfo.u8vbmap[TIM_OFFSET + byte_offset] |= g_bmap[bit_offset];
    strBeaconInfo.u8vbmap[LENGTH_OFFSET] = length;
}


/*
*  @brief 			update_ps_flags_ap
*  @details 	   	This function updates the flags after a packet in enqued 
*                  		in the power save buffer 
*
*  @Processing       This function updates the flags after a packet in enqued 
*                  		 in the power save buffer    
*                  		              
*  @author		Abd Al-Rahman Diab
*  @date			05 june 2013
*  @version		1.0
*/	
void update_ps_flags_ap(NMI_ieee80211_sta *ae, NMI_Bool bc_mc_pkt,
                        NMI_Uint8 num_buff_added, NMI_Bool ps_add_del_ac)
{
    if(num_buff_added == 0)
        return;

    if(bc_mc_pkt == NMI_TRUE)
    {
        /* The global count for queued BC/MC packets is incremented here.    */
        /* This count is checked for resetting the AID0 bit in the TIM       */
        /* element once all buffered BC/MC packets have been transmitted. It */
        /* is decremented in Tx complete for BC/MC packets.                  */
        g_num_mc_bc_pkt += num_buff_added;
        g_num_ps_pkt++;

        /* Set the AID0 bit. This is reset BC/MC packet queue becomes empty. */
        /* It is updated in transmit complete processing.                    */
        set_dtim_bit(AID0_BIT);
        set_dtim_bit(DTIM_BIT);
    }
    else
    {
        ae->num_ps_pkt += num_buff_added;
   	 g_num_ps_pkt   += num_buff_added;
	 
    	  /* Set the TIM bit for this station */
         set_tim_bit(ae->u16AID);
    }
}


/*****************************************************************************/
/*                                                                           */
/*  Function Name : is_ps_buff_pkt_ap                                        */
/*                                                                           */
/*  Description   : This function checks if a packet requries buffering      */
/*                  based on the power save state of the destination.        */
/*                                                                           */
/*  Inputs        : 1) Pointer to the association table                      */
/*                  2) Pointer to the destination address                    */
/*                  3) Pointer to the Q-number                               */
/*                  4) Pointer to the Q-head to which the packet must be q-d */
/*                                                                           */
/*  Globals       : g_mc_q                                                   */
/*                  g_num_mc_bc_pkt                                          */
/*                  g_num_sta_ps                                             */
/*                                                                           */
/*  Processing    : In case the packet is a broadcast/multicast packet, the  */
/*                  global g_num_sta_ps is checked to determine if any of    */
/*                  the associated stations are in power save mode.          */
/*                  In case of a unicast packet the association              */
/*                  entry is checked to determine the power save state of    */
/*                  the station. The queue limits are checked and if the     */
/*                  maximum is reached the queue header pointer is set to    */
/*                  zero so that the calling function can free the buffer.   */
/*                  Otherwise the queue details are set appropriately.       */
/*                                                                           */
/*  Outputs       : Q-Num to which the packet must be queued and the Queue   */
/*                  pointer to which the packet must be queued.              */
/*                                                                           */
/*  Returns       : NMI_TRUE, if the packet should be buffered for power save   */
/*                  NMI_FALSE, if the packet should not be buffered             */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         24 10 2005   			          Draft                                */
/*                                                                           */
/*****************************************************************************/

NMI_Bool is_ps_buff_pkt_ap(NMI_ieee80211_sta *ae, NMI_Uint8 *da, struct tx_complete_data* dscr)
{
    NMI_Bool  ps_del_en_ac    = NMI_FALSE;
    NMI_Bool  bc_mc_pkt       = is_group(da);
    NMI_Uint8  priority        = 0;
    list_buff_t *qh         = NULL;


	/* check if pkt is BC/MC or Unicast */
    if(bc_mc_pkt == NMI_TRUE)
    {
        if(g_num_sta_ps == 0)
        {
            /* All stations are in Active mode. No buffering required. */
            return NMI_FALSE;
        }

        /* If the global BC/MC queue has not reached the maximum size */
        /* received BC/MC packet should be queued in it. Else packet  */
        /* should be dropped 			  */

        if(g_num_ps_pkt < PS_PKT_Q_MAX_LEN)
        {
            qh = &g_mc_q;
        }
    }
    else
    {
        if(ae == 0)
        {
            /* Exception. Should not occur. */
            return NMI_FALSE;
        }

        if(ae->ps_state == ACTIVE_PS)
        {
            /* Station is in Active mode. No buffering required. */
            return NMI_FALSE;
        }

        if(g_num_ps_pkt < PS_PKT_Q_MAX_LEN)
        {
            qh = (&(ae->ps_q_lgcy));

        }
    }


    if( qh != NULL)
    {
        add_list_element(qh, dscr);
        update_ps_flags_ap(ae, bc_mc_pkt, 1, NMI_FALSE);
    }
    else
    {
        if(dscr->skb)
		dev_kfree_skb(dscr->skb);	
		
	linux_wlan_free(dscr);
    }

    return NMI_TRUE;
}


/*****************************************************************************/
/*                                                                           */
/*  Function Name : buffer_tx_packet                                         */
/*                                                                           */
/*  Description   : This function buffers a packet ready for transmission    */
/*                                                                           */
/*  Inputs        : 1) Pointer to the transmit descriptor                    */
/*                  2) Number of the queue to which the packet belongs       */
/*                  3) Destination address                                   */
/*                                                                           */
/*  Globals       : g_mc_q                                                   */
/*                  g_num_mc_bc_pkt                                          */
/*                  g_num_sta_ps                                             */
/*                                                                           */
/*  Processing    : The packet is buffered based on the service class or the */
/*                  power save mode of the receiving STA                     */
/*                                                                           */
/*  Outputs       : None                                                     */
/*                                                                           */
/*  Returns       : NMI_TRUE, if the given packet is buffered in any queue      */
/*                  NMI_FALSE, if the packet is not buffered and can be sent    */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         24 10 2005   		          Draft                                */
/*                                                                           */
/*****************************************************************************/

NMI_Bool buffer_tx_packet(NMI_Uint8 *entry, NMI_Uint8 *da, NMI_Uint8 priority,
                         struct tx_complete_data *tx_dscr)
{
    if(NMI_TRUE == is_ps_buff_pkt_ap((NMI_ieee80211_sta*)entry, da, tx_dscr))
    {
#ifdef DEBUG_MODE
        g_mac_stats.psbuff++;
#endif /* DEBUG_MODE */
    }
#if 0 // open this if you handle Block Ack
    else if(NMI_TRUE == is_serv_cls_buff_pkt(entry, priority, tx_dscr))
    {
#ifdef DEBUG_MODE
        g_mac_stats.bapendingtx++;
#endif /* DEBUG_MODE */
    }
#endif
    else
    {
        return NMI_FALSE;
    }

    /* If the packet is buffered return NMI_TRUE */
    return NMI_TRUE;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : tx_null_frame                                            */
/*                                                                           */
/*  Description   : This function prepares and sends a NULL frame to the     */
/*                  given station.                                           */
/*                                                                           */
/*  Inputs        : 1) Address of station to which NULL frame is directed    */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : The NULL frame is prepared and added to the H/w queue    */
/*                  with the required descriptor.                            */
/*                                                                           */
/*  Outputs       : None                                                     */
/*  Returns       : None                                                     */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         24 10 2005   			          Draft                                */
/*                                                                           */
/*****************************************************************************/

void tx_null_frame(NMI_Uint8 *sa, NMI_ieee80211_sta *ae, NMI_Bool is_qos, NMI_Uint8 priority,NMI_Uint8 more_data)
{
    NMI_Uint8  q_num          = 0;
    NMI_Uint8  tx_rate        = 0;
    NMI_Uint8  pream          = 0;
    struct tx_complete_data* tx_data;
    NMI_Uint8  *msa           = 0;
    NMI_Uint8  len            = 0;
   // UWORD32 phy_tx_mode    = 0;
    //UWORD32 retry_set[2]   = {0};

    /* Allocate buffer for the NULL frame and set its contents */
   tx_data = (struct tx_complete_data*)kmalloc(sizeof(struct tx_complete_data),GFP_ATOMIC);
   
	if(tx_data == NULL)
		PRINT_ER("Failed to allocate memory for tx_data structure\n");

		
    if(is_qos == NMI_TRUE)
    {
        //len = set_frame_ctrl_qos_null_ap(msa, priority, 0);
       // q_num = get_txq_num(priority);
    }
    else
    {

	len = MAC_HDR_LEN ;//+ FCS_LEN;
    	
       msa = (NMI_Uint8*)kmalloc(len,GFP_ATOMIC);

	 if(msa == NULL)
	    {
		PRINT_ER("No Mem for NULL Tx DSCR\n");
	        kfree(tx_data);
	        return;
	    }

	tx_data->buff = msa;
	tx_data->size = len;
	tx_data->skb  = NULL;
               
	set_frame_control(msa, (NMI_Uint16)NULL_FRAME);
	  
        /* NULL frames will be put in Normal priority queue */
        //q_num = NORMAL_PRI_Q;
    }

    /* Set the from ds bit */
    set_from_ds(msa, 1);

    /* Set the address fields */
    set_address1(msa, sa);
    set_address2(msa, g_linux_wlan->nmc_netdev->dev_addr);
    set_address3(msa, g_linux_wlan->nmc_netdev->dev_addr);

    /* Get the transmit rate for the associated station based on the         */
    /* auto-rate, multi-rate or user-rate settings. The preamble must be     */
    /* set accordingly.                                                      */
    //tx_rate     = get_tx_rate_to_sta(ae);
    //pream       = get_preamble(tx_rate);

    /* Update the retry set information for this frame */
    //update_retry_rate_set(1, tx_rate, ae, retry_set);

    /* Get the PHY transmit mode based on the transmit rate and preamble */
    //phy_tx_mode = get_dscr_phy_tx_mode(tx_rate, pream, (void *)ae);

    /* Create the transmit descriptor and set the contents */
    //tx_dscr = create_default_tx_dscr(0, 0, 0);

	
 NMI_Xmit_data((void*)tx_data, WLAN_TO_WLAN);
	
#if 0
    /* Set various transmit descriptor parameters */
    set_tx_params(tx_dscr, tx_rate, pream, NORMAL_ACK, phy_tx_mode, retry_set);
    set_tx_buffer_details(tx_dscr, msa, 0, len-FCS_LEN, 0);
    set_tx_dscr_q_num((UWORD32 *)tx_dscr, q_num);
    /* This is already done in create_default_tx_dscr */
    /* set_tx_security(tx_dscr, NO_ENCRYP, 0, se->sta_index); */
    set_ht_ps_params(tx_dscr, (void *)ae, tx_rate);
    set_ht_ra_lut_index(tx_dscr, NULL, 0, tx_rate);
    update_tx_dscr_tsf_ts((UWORD32 *)tx_dscr);


    if(qmu_add_tx_packet(&g_q_handle.tx_handle, q_num, tx_dscr) != QMU_OK)
    {
//#ifdef DEBUG_MODE
        //g_mac_stats.qaexc++;
//#endif /* DEBUG_MODE */

        /* Exception. Free the transmit descriptor and packet buffers if it  */
        /* cannot be added to the H/w queue.                                 */
        free_tx_dscr((UWORD32 *)tx_dscr);
    }
    else if(ae != NULL)
    {
        ae->num_qd_pkt++;
    }
#endif
}


/*****************************************************************************/
/*                                                                           */
/*  Function Name : handle_ps_poll                                           */
/*                                                                           */
/*  Description   : This function handles a received PS-Poll frame in the    */
/*                  access point mode.                                       */
/*                                                                           */
/*  Inputs        : 1) Pointer to the PS-Poll frame                          */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : This function checks if a previous PS-Poll frame is yet  */
/*                  to be processed (checks a flag in the association entry) */
/*                  If not, the first frame from the power save queue is     */
/*                  requeued. If requeuing is not successful, a null frame   */
/*                  is transmitted to the station.                           */
/*                                                                           */
/*  Outputs       : None                                                     */
/*                                                                           */
/*  Returns       : BOOL_T, BTRUE, If it was a PS Poll frame                 */
/*                          BFALSE, otherwise                                */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         24 10 2005   			          Draft                                */
/*                                                                           */
/*****************************************************************************/

NMI_Bool handle_ps_poll(wlan_rx_t    *wlan_rx )
{

    NMI_ieee80211_sta *ae       = (NMI_ieee80211_sta*)(wlan_rx->u8sa_entry);

    /* Check if the frame is a PS Poll frame and return BFALSE if not */
    if(wlan_rx->u8Sub_type != PS_POLL)
        return NMI_FALSE;


    /* PS-Poll frames are processed only if the station is associated to AP. */
    if((ae == 0) || (ae->state != ASOC))
    {
    	// TODO: find a way to make hostapd send a deauth frame
        /* Send a De-authentication Frame to the station as PS-Poll is a     */
        /* Class 3 frame.                                                    */
        //send_deauth_frame(wlan_rx->sa, (UWORD16)CLASS3_ERR);

        /* No further processing is required */
        return NMI_TRUE;
    }

    if(ae->ps_poll_rsp_qed == NMI_TRUE)
    {
        /* If a frame has been queued in response to a previous PS Poll      */
        /* received, further PS Poll frames will not be honoured till the    */
        /* response frame exchange is complete (indicated by resetting this  */
        /* flag in the Tx Complete for this frame).                          */
        return NMI_TRUE;
    }

    /* Requeue one buffered packet from the station's non-delivery enabled Q */
    /* to the MAC H/w queue                                                  */
    /* If the non-delivery enabled Q is empty queue a NULL frame             */
    /* ASSUMPTION: legacy queue should be empty if all queues are delivery   */
    /* enabled                                                               */
    if(peek_list(&(ae->ps_q_lgcy)) == NULL)
        tx_null_frame(wlan_rx->u8sa, ae, NMI_FALSE, 0, 0);
    else
        requeue_ps_packet(ae, &(ae->ps_q_lgcy), NMI_TRUE, NMI_FALSE) ;

    ae->ps_poll_rsp_qed = NMI_TRUE;

    return NMI_TRUE;
}


/*
*  @brief 			filter_monitor_data_frames
*  @details 	   	This function check for data frames sent by hostapd and
*                  		transmit it through the data path
*                  		              
*  @author		Abd Al-Rahman Diab
*  @date			05 june 2013
*  @version		1.0
*/	
NMI_Bool filter_monitor_data_frames(NMI_Uint8 *buf, NMI_Uint16 len)
{
	NMI_Bool ret = NMI_FALSE;
	struct tx_complete_data* tx_data = NULL;
	
	if(get_type(buf)== DATA_BASICTYPE)
	{
		tx_data = (struct tx_complete_data*)kmalloc(sizeof(struct tx_complete_data),GFP_ATOMIC);

		if(tx_data == NULL)
		{
			PRINT_ER("Failed to allocate memory for tx_data structure\n");
		        return ret;		
		}	

		tx_data->buff =buf;
		tx_data->size = len;
		tx_data->skb  = NULL;
                NMI_Xmit_data((void*)tx_data, MONITOR_TO_WLAN);
	
		ret = NMI_TRUE;
	}

	return ret;
}


#endif 


