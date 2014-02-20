#ifndef __NMI_HOST_AP__
#define __NMI_HOST_AP__

#include "nmi_wlan_if.h"
#include "nmi_wlan.h"
#include "host_interface.h"
#include "../linux/include/NMI_WFI_NetDevice.h"

#ifdef NMI_FULLY_HOSTING_AP
/*!  
*  @file	NMI_host_AP.h
*  @brief	code related to AP fully hosting mode on NMI driver
*  @author	Abd Al-Rahman Diab
*  @date	09 APRIL 2013
*  @version	1.0
*/


#define ITIM                			 5   /* Traffic Information Map        */
#define DEFAULT_TIM_LEN        	 4
#define FCS_LEN                 		 4
#define VBMAP_SIZE              		 256

#define MAX_LISTEN_INT          10
#define VBMAP_SIZE              256
#define MIN_TIM_LEN             4
#define PS_PKT_Q_MAX_LEN        16
#define PS_BUFF_TO              500 /* Time out for discarding a packets in  */
                                    /* PS queues in millisecs                */
#define AGING_THR_IN_SEC           60    /*  60 Seconds */

#define WORD_OFFSET_4           4
#define TX_DSCR_NEXT_ADDR_WORD_OFFSET                WORD_OFFSET_4


#define WIFI_PERIPH_BASE		0x00000000
#define WIFI_PA_BASE			(WIFI_PERIPH_BASE+0x9800)
#define PA_BASE 				WIFI_PA_BASE

#define rMAC_DTIM_COUNT_ADDR						(( NMI_Uint32 )(PA_BASE + 0x021C))
#define rMAC_BEACON_PERIOD           					(( NMI_Uint32 )(PA_BASE + 0x020C))
#define rMAC_DTIM_PERIOD              					(( NMI_Uint32 )(PA_BASE + 0x0210))
#define rMAC_TSF_CON                 				 		(( NMI_Uint32 )(PA_BASE + 0x0200))


#define 		BEST_EFFORT_PRIORITY 		0

#define 		NORMAL_ACK				0
#define 	 	BCAST_NO_ACK				4


#define PROTOCOL_VERSION        0x00
#define MAX_MSDU_LEN            1596
#define PS_POLL_LEN             20


#define QOS_CTRL_FIELD_LEN    2
#define HT_CTRL_FIELD_LEN        4     /* Length of HT Control Field        */

#define IS_MANAGMEMENT 				0x100
#define IS_MANAGMEMENT_CALLBACK 		0x080
#define IS_MGMT_STATUS_SUCCES			0x040
#define GET_PKT_OFFSET(a) (((a) >> 22) & 0x1ff)


/* Maximum number of hash values possible for the Sta Entry Table */
#define MAX_HASH_VALUES      16


#define SNAP_HDR_ID_LEN         6

#define ETH_PKT_TYPE_OFFSET       12
#define IP_TYPE                   0x0800
#define ARP_TYPE                  0x0806
#define ONE_X_TYPE                0x888E
#define VLAN_TYPE                 0x8100
#define LLTD_TYPE                 0x88D9
#define UDP_TYPE                  0x11
#define TCP_TYPE                  0x06


/*****************************************************************************/
/* Function Macros                                                           */
/*****************************************************************************/

#define NEXT_ELEMENT_ADDR(base, ofst) (*((NMI_Uint32 *)((NMI_Uint8 *)base + ofst)))



/*****************************************************************************/
/* Enums                                                                     */
/*****************************************************************************/

/* Basic Frame Type Codes (2-bit) */
typedef enum {CONTROL               = 0x04,
              DATA_BASICTYPE        = 0x08,
              MANAGEMENT            = 0x00,
              RESERVED              = 0x0C
} BASICTYPE_T;

typedef enum {ACTIVE_PS      = 0,
              POWER_SAVE     = 1
} STA_PS_STATE_T;


typedef enum {AID0_BIT           = 0,
              DTIM_BIT           = 1
} TIMBIT_T;


typedef enum {RE_Q_ERROR       = 0,
              NO_PKT_IN_QUEUE  = 1,
              PKT_REQUEUED     = 2,
              END_OF_QUEUE     = 3,
              PKT_NOT_REQUEUED = 4
} REQUEUE_STATUS_T;


/* Data Pathes */
typedef enum {HOST_TO_WLAN               = 0,
              WLAN_TO_WLAN        = 1,
              MONITOR_TO_WLAN        = 2
} Data_Interface;

/* Capability Information field bit assignments  */
typedef enum{ESS            = 0x01,   /* ESS capability               */
             IBSS           = 0x02,   /* IBSS mode                    */
             POLLABLE       = 0x04,   /* CF Pollable                  */
             POLLREQ        = 0x08,   /* Request to be polled         */
             PRIVACY        = 0x10,   /* WEP encryption supported     */
             SHORTPREAMBLE  = 0x20,   /* Short Preamble is supported  */
             SHORTSLOT      = 0x400,  /* Short Slot is supported      */
             PBCC           = 0x40,   /* PBCC                         */
             CHANNELAGILITY = 0x80,   /* Channel Agility              */
             SPECTRUM_MGMT  = 0x100,  /* Spectrum Management          */
             DSSS_OFDM      = 0x2000  /* DSSS-OFDM                    */
} CAPABILITY_T;

/* Authentication/Association states of STAs maintained by the Access Point  */
typedef enum {AUTH_COMPLETE      = 1,   /* Authenticated                     */
              AUTH_KEY_SEQ1      = 2,   /* Shared Key Auth (Sequence 1) done */
              ASOC               = 3    /* Associated                        */
} STATIONSTATE_T;

typedef enum {TYPE_OFFSET        = 0,
              LENGTH_OFFSET      = 1,
              DTIM_CNT_OFFSET    = 2,
              DTIM_PERIOD_OFFSET = 3,
              BMAP_CTRL_OFFSET   = 4,
              TIM_OFFSET         = 5
} OFFSET_T;

/* Frame Type and Subtype Codes (6-bit) */
typedef enum {ASSOC_REQ             = 0x00,
              ASSOC_RSP             = 0x10,
              REASSOC_REQ           = 0x20,
              REASSOC_RSP           = 0x30,
              PROBE_REQ             = 0x40,
              PROBE_RSP             = 0x50,
              BEACON                = 0x80,
              ATIM                  = 0x90,
              DISASOC               = 0xA0,
              AUTH                  = 0xB0,
              DEAUTH                = 0xC0,
              ACTION                = 0xD0,
              PS_POLL               = 0xA4,
              RTS                   = 0xB4,
              CTS                   = 0xC4,
              ACK                   = 0xD4,
              CFEND                 = 0xE4,
              CFEND_ACK             = 0xF4,
              DATA                  = 0x08,
              DATA_ACK              = 0x18,
              DATA_POLL             = 0x28,
              DATA_POLL_ACK         = 0x38,
              NULL_FRAME            = 0x48,
              CFACK                 = 0x58,
              CFPOLL                = 0x68,
              CFPOLL_ACK            = 0x78,
              QOS_DATA              = 0x88,
              QOS_DATA_ACK          = 0x98,
              QOS_DATA_POLL         = 0xA8,
              QOS_DATA_POLL_ACK     = 0xB8,
              QOS_NULL_FRAME        = 0xC8,
              QOS_CFPOLL            = 0xE8,
              QOS_CFPOLL_ACK        = 0xF8,
              BLOCKACK_REQ          = 0x84,
              BLOCKACK              = 0x94
} TYPESUBTYPE_T;


/*****************************************************************************/
/* Structures                                                                */
/*****************************************************************************/

typedef struct _q_entry_t
{
    void              *fa;          /* Pointer to the element in the queue   */
    struct _q_entry_t *tqe_next;    /* Pointer to the next element           */
    struct _q_entry_t *tqe_prev;    /* Pointer to the previous element       */
} q_entry_t;

typedef struct
{
    q_entry_t *head;
    q_entry_t *tail;
} q_head_t;

typedef struct
{
    void    *head;           /* Head Element of the List                    */
    void    *tail;           /* Tail Element of the List                    */
    NMI_Uint16 count;           /* Number of Elements in the List              */
    NMI_Uint16 lnk_byte_ofst;   /* Offset to Link Pointer in the List in Bytes */
} list_buff_t;


/* Basic rate set. This contains the set of basic rates supported by any     */
/* IBSS.                                                                     */
typedef struct
{
    NMI_Uint8 rates[MAX_RATES_SUPPORTED];
    NMI_Uint8 num_rates;
} rate_t;
	
typedef struct {
	int size;
	void* buff;
}beacon_data;

typedef struct {
	NMI_Uint16		u16beacon_len;
	NMI_Uint8             *u8beacon_frame;
	NMI_Uint8		u8tim_element_index;
	NMI_Uint16      	u16tim_element_trailer_len;
	NMI_Uint8       	u8vbmap[VBMAP_SIZE];
	NMI_Uint8             	u8DTIMPeriod;
	NMI_Uint16            u16Beacon_Period;
}beacon_info;

typedef struct
{
    NMI_Uint8   service_class; /* Service class extracted from header       */
    NMI_Uint8   priority_val;  /* Priority value extracted from header      */
    NMI_Uint8   u8hdr_len;       /* Length of header including offset         */
    NMI_Uint16  u16data_len;      /* Length of the frame excluding headers     */
    NMI_Uint16  u16rx_len;        /* Length of the frame including the headers */
    NMI_Uint8   *u8sa_entry;     /* Source Address station entry              */
    NMI_Uint8   *u8msa;          /* MAC header start address                  */
    NMI_Uint8   u8addr1[6];      /* Address1 field extracted from header      */
    NMI_Uint8   u8addr2[6];      /* Address2 field extracted from header      */
    NMI_Uint8   u8addr3[6];      /* Address3 field extracted from header      */
    NMI_Uint8   *u8sa;           /* Pointer to source address                 */
    NMI_Uint8   *u8da;           /* Pointer to destination address            */
    NMI_Uint8   *u8bssid;        /* Pointer to BSSID                          */
    NMI_Uint8   *u8ta;           /* Pointer to Transmitter address            */
    BASICTYPE_T enumType;       /* Packet type extracted from the header     */
    NMI_Uint8   u8Sub_type;      /* Packet Sub type extracted from the header */
    NMI_Uint8   u8frm_ds;        /* From DS field of MAC header               */
    NMI_Uint8   u8to_ds;         /* To DS field of MAC header                 */
    NMI_Bool 	bIs_grp_addr;   /* Group address flag                        */
} wlan_rx_t;

typedef struct {
	int quit;

	/**
		input interface functions
	**/
	nmi_wlan_os_func_t os_func;
	nmi_wlan_io_func_t io_func;
	nmi_wlan_net_func_t net_func;
	nmi_wlan_indicate_func_t indicate_func;

	/**
		host interface functions
	**/
	nmi_hif_func_t hif_func;
	void *hif_lock;

	/**
		configuration interface functions
	**/
	nmi_cfg_func_t cif_func;
	int cfg_frame_in_use;
	nmi_cfg_frame_t cfg_frame;
	uint32_t cfg_frame_offset;
	int cfg_seq_no;
	void *cfg_wait;

	/**
		RX buffer
	**/
	uint32_t rx_buffer_size;
	//uint8_t *rx_buffer;
	//uint32_t rx_buffer_offset;

	/**
		TX buffer
	**/
	uint32_t tx_buffer_size;
	uint8_t *tx_buffer;
	uint32_t tx_buffer_offset;
	
	/**
		TX queue
	**/
	void *txq_lock;
	
	/*Added by Amr - BugID_4720*/
	void *txq_add_to_head_lock;
	void *txq_spinlock;
	unsigned long txq_spinlock_flags;
	
	struct txq_entry_t *txq_head;
	struct txq_entry_t *txq_tail;
	int txq_entries;
	void *txq_wait;
	int txq_exit;

	/**
		RX queue
	**/
	void *rxq_lock;		
	struct rxq_entry_t *rxq_head;
	struct rxq_entry_t *rxq_tail;	
	int rxq_entries;
	void *rxq_wait;
	int rxq_exit;

#if DMA_VER == DMA_VER_2
	int use_dma_v2; 
#endif
} nmi_wlan_dev_t;


typedef struct   {
  u32 				supp_rates[IEEE80211_NUM_BANDS];
  NMI_Uint8 			addr[ETH_ALEN];				/* MAC address of the STA           */
  NMI_Uint16 			u16AID;          	 			/* Association ID of the STA           */
  STATIONSTATE_T 	state;             				/* Auth/Asoc state of the STA          */
  struct ieee80211_sta_ht_cap ht_cap;
  //struct ieee80211_sta_vht_cap vht_cap;
  NMI_Uint32        		aging_cnt;         				/* Aging cnt for STA                   */
  rate_t         			op_rates;        				/* Operational rates of the STA        */
  STA_PS_STATE_T 	ps_state;         				/* Power save state of the STA         */

  list_buff_t    ps_q_lgcy;         						/* Power save queue for the STA        */
  NMI_Uint16        num_ps_pkt;        					/* Number of UC pkts queued in PSQ     */
  NMI_Uint16        num_qd_pkt;        					/* Number of UC pkts queued in HW      */
  NMI_Bool	ps_poll_rsp_qed;   					/* Flag to indicate PS poll rsp is     */
                                      							/* already queued and pending          */

} NMI_ieee80211_sta;

#if 0
/* Association element structure. This is the element structure to which the */
/* element pointers of the association table entries point.                  */
typedef struct
{

   
    
												
    NMI_Uint16        listen_interval;   /* Listen Interval of the STA          */

    

} asoc_entry_t;
#endif

typedef struct table_elmnt_t
{
    NMI_Uint8                key[6];          /* Key, i.e. the MAC address      */
    void*                 element;         /* Pointer to the table element   */
    struct table_elmnt_t* next_hash_elmnt; /* Pointer to next bucket element */
} table_elmnt_t;

/*****************************************************************************/
/* Data Types                                                                */
/*****************************************************************************/

typedef table_elmnt_t* table_t[MAX_HASH_VALUES];

/*****************************************************************************/
/* Extern Variable Declarations                                              */
/*****************************************************************************/

extern table_t g_sta_table;
extern linux_wlan_t* g_linux_wlan;


 NMI_Sint32 host_add_beacon(NMI_WFIDrvHandle hWFIDrv, NMI_Uint32 u32Interval,
									 NMI_Uint32 u32DTIMPeriod,
									 NMI_Uint32 u32HeadLen, NMI_Uint8* pu8Head,
									 NMI_Uint32 u32TailLen, NMI_Uint8* pu8Tail);

 NMI_Sint32 host_del_beacon(NMI_WFIDrvHandle hWFIDrv);

inline void NMI_AP_AddSta(u8 *mac, struct station_parameters *params);
inline void NMI_AP_EditSta(u8 *mac, struct station_parameters *params);
inline void NMI_AP_RemoveSta(u8 *mac);

 void process_tbtt_isr(void);

 nmi_wlan_dev_t* Get_wlan_context(NMI_Uint16* pu16size);

 #ifdef NMI_AP_EXTERNAL_MLME
int nmi_wlan_txq_add_mgmt_pkt(void *priv, uint8_t *buffer, uint32_t buffer_size, nmi_tx_complete_func_t func);
#endif

int nmi_FH_wlan_txq_add_net_pkt(void *priv, uint8_t *buffer, uint32_t buffer_size, nmi_tx_complete_func_t func);

void NMI_Process_rx_frame(NMI_Uint8 *u8Buff, NMI_Uint32 u32Size);

int NMI_Xmit_data(void* ptx_data, Data_Interface InterfaceType);

NMI_Bool filter_monitor_data_frames(NMI_Uint8 *buf, NMI_Uint16 len);

#endif //  NMI_FULLY_HOSTING_AP

#endif 
