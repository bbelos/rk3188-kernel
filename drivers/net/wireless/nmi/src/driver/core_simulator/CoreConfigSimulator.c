
/*!  
*  @file		CoreConfigSimulator.c
*  @briefCoreConfigSimulatorDeInit	
*  @author	
*  @sa		CoreConfigSimulator.h 
*  @date	1 Mar 2012
*  @version	1.0
*/


/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/
#include "NMI_OsWrapper/include/NMI_OSWrapper.h"
#include "itypes.h"
#include "driver/include/FIFO_Buffer.h"
#include "nmi_wlan_if.h"

/*****************************************************************************/
/* Constants                                                                 */
/*****************************************************************************/
#define INLINE static __inline

#define SIMULATOR_MSG_EXIT			((NMI_Uint32)100)

//#define CONFIG_PRINT

#define MAX_BUFFER_LEN					1596
#define MAX_FIFO_BUFF_LEN				2000


#define MAX_CFG_LEN						256
#define MAX_STRING_LEN					256
#define SCAN_COMPLETE_MSG_LEN			8
#define FCS_LEN							4

/* Number of basic WIDS */
#define NUM0_CHAR_WID        45
#define NUM0_SHORT_WID       18
#define NUM0_INT_WID         26
#define NUM0_STR_WID         30

/* Number of WIDS for this platform */
#define NUM1_CHAR_WID        (NUM0_CHAR_WID  + 1)
#define NUM1_SHORT_WID       NUM0_SHORT_WID
#define NUM1_INT_WID         NUM0_INT_WID
#define NUM1_STR_WID         NUM0_STR_WID

/* Number of WIDS with the current configuration */
#define NUM2_CHAR_WID        NUM1_CHAR_WID
#define NUM2_SHORT_WID       NUM1_SHORT_WID
#define NUM2_INT_WID         NUM1_INT_WID
#define NUM2_STR_WID         NUM1_STR_WID

/* Final number if WIDs */
#define NUM_CHAR_WID        NUM2_CHAR_WID
#define NUM_SHORT_WID       NUM2_SHORT_WID
#define NUM_INT_WID         NUM2_INT_WID
#define NUM_STR_WID         NUM2_STR_WID
#define NUM_BIN_DATA_WID           9

#define NUM_WID             (NUM_CHAR_WID + NUM_SHORT_WID\
                           + NUM_INT_WID + NUM_STR_WID + NUM_BIN_DATA_WID)

#define MSG_DATA_OFFSET           8
#define WID_VALUE_OFFSET          3
#define WID_LENGTH_OFFSET         2
#define WRITE_RSP_LEN             4
#define MAX_QRSP_LEN              1000
#define MAX_WRSP_LEN              20
#define MSG_HDR_LEN               8
#define MAX_ADDRESS_LEN           6
#define WRSP_SUCCESS              1
#define WRSP_ERR_MSG              (-1)
#define WRSP_ERR_SEQ              (-2)
#define WRSP_ERR_LEN              (-3)
#define WRSP_ERR_WID              (-4)
#define MAX_SUPRATE_LEN           34
#define MAX_PROD_VER_LEN          10
#define MAX_GRPADDR_LEN           38
#define SITE_SURVEY_ELM_LEN       (MAX_SSID_LEN + 1 + 1 + 1)
#define WID_BIN_DATA_LEN_MASK     0x3FFF
#define STATUS_MSG_LEN            12
#define MAC_CONNECTED			1
#define MAC_DISCONNECTED   		0

/*****************************************************************************/
/* Function Macros                                                           */
/*****************************************************************************/
#define MAKE_WORD16(lsb, msb) (((NMI_Uint16)(msb) << 8) & 0xFF00) | (lsb)
#define MAKE_WORD32(lsw, msw) (((NMI_Uint32)(msw) << 16) & 0xFFFF0000) | (lsw)

/*****************************************************************************/
/* Type definitions                                                          */
/*****************************************************************************/
#if 0
WID_SHORT    = 1,
WID_INT      = 2,
WID_STR      = 3,
WID_BIN_DATA = 4,
} WID_TYPE_T;

/* WLAN Identifiers */
typedef enum {
	        WID_NIL                            = -1,
			WID_BSS_TYPE                       = 0x0000,
			WID_CURRENT_TX_RATE                = 0x0001,
			WID_CURRENT_CHANNEL                = 0x0002,
			WID_PREAMBLE                       = 0x0003,
			WID_11G_OPERATING_MODE             = 0x0004,
			WID_STATUS                         = 0x0005,
			WID_11G_PROT_MECH                  = 0x0006,

#ifdef MAC_HW_UNIT_TEST_MODE
            WID_GOTO_SLEEP                     = 0x0007,
#else /* MAC_HW_UNIT_TEST_MODE */
			WID_SCAN_TYPE                      = 0x0007,
#endif /* MAC_HW_UNIT_TEST_MODE */
			WID_PRIVACY_INVOKED                = 0x0008,
			WID_KEY_ID                         = 0x0009,
			WID_QOS_ENABLE                     = 0x000A,
			WID_POWER_MANAGEMENT               = 0x000B,
			WID_802_11I_MODE                   = 0x000C,
			WID_AUTH_TYPE                      = 0x000D,
			WID_SITE_SURVEY                    = 0x000E,
			WID_LISTEN_INTERVAL                = 0x000F,
			WID_DTIM_PERIOD                    = 0x0010,
			WID_ACK_POLICY                     = 0x0011,
            WID_RESET                          = 0x0012,
			WID_PCF_MODE                       = 0x0013,
			WID_CFP_PERIOD                     = 0x0014,
			WID_BCAST_SSID                     = 0x0015,

#ifdef MAC_HW_UNIT_TEST_MODE
			WID_PHY_TEST_PATTERN               = 0x0016,
#else /* MAC_HW_UNIT_TEST_MODE */
            WID_DISCONNECT                     = 0x0016,
#endif /* MAC_HW_UNIT_TEST_MODE */

			WID_READ_ADDR_SDRAM                = 0x0017,
			WID_TX_POWER_LEVEL_11A             = 0x0018,
			WID_REKEY_POLICY                   = 0x0019,
			WID_SHORT_SLOT_ALLOWED             = 0x001A,
			WID_PHY_ACTIVE_REG                 = 0x001B,
			WID_PHY_ACTIVE_REG_VAL             = 0x001C,
			WID_TX_POWER_LEVEL_11B             = 0x001D,
			WID_START_SCAN_REQ                 = 0x001E,
			WID_RSSI                           = 0x001F,
			WID_JOIN_REQ                       = 0x0020,
			WID_ANTENNA_SELECTION              = 0x0021,
			WID_USER_CONTROL_ON_TX_POWER       = 0x0027,
            WID_MEMORY_ACCESS_8BIT             = 0x0029,
            WID_CURRENT_MAC_STATUS             = 0x0031,
            WID_AUTO_RX_SENSITIVITY            = 0x0032,
            WID_DATAFLOW_CONTROL               = 0x0033,
            WID_SCAN_FILTER                    = 0x0036,
            WID_LINK_LOSS_THRESHOLD            = 0x0037,
            WID_AUTORATE_TYPE                  = 0x0038,
            WID_CCA_THRESHOLD                  = 0x0039,

  			WID_UAPSD_SUPPORT_AP               = 0x002A,

            WID_802_11H_DFS_MODE               = 0x003B,
            WID_802_11H_TPC_MODE               = 0x003C,

			WID_RTS_THRESHOLD                  = 0x1000,
			WID_FRAG_THRESHOLD                 = 0x1001,
			WID_SHORT_RETRY_LIMIT              = 0x1002,
			WID_LONG_RETRY_LIMIT               = 0x1003,
			WID_CFP_MAX_DUR                    = 0x1004,
			WID_PHY_TEST_FRAME_LEN             = 0x1005,
			WID_BEACON_INTERVAL                = 0x1006,
            WID_MEMORY_ACCESS_16BIT            = 0x1008,

			WID_RX_SENSE                       = 0x100B,
			WID_ACTIVE_SCAN_TIME               = 0x100C,
			WID_PASSIVE_SCAN_TIME              = 0x100D,
			WID_SITE_SURVEY_SCAN_TIME          = 0x100E,
			WID_JOIN_TIMEOUT                   = 0x100F,
			WID_AUTH_TIMEOUT                   = 0x1010,
			WID_ASOC_TIMEOUT                   = 0x1011,
			WID_11I_PROTOCOL_TIMEOUT           = 0x1012,
			WID_EAPOL_RESPONSE_TIMEOUT         = 0x1013,
			WID_CCA_BUSY_STATUS                = 0x1014,

			WID_FAILED_COUNT                   = 0x2000,
			WID_RETRY_COUNT                    = 0x2001,
			WID_MULTIPLE_RETRY_COUNT           = 0x2002,
			WID_FRAME_DUPLICATE_COUNT          = 0x2003,
			WID_ACK_FAILURE_COUNT              = 0x2004,
			WID_RECEIVED_FRAGMENT_COUNT        = 0x2005,
			WID_MULTICAST_RECEIVED_FRAME_COUNT = 0x2006,
			WID_FCS_ERROR_COUNT                = 0x2007,
			WID_SUCCESS_FRAME_COUNT            = 0x2008,
			WID_PHY_TEST_PKT_CNT               = 0x2009,
			WID_PHY_TEST_TXD_PKT_CNT           = 0x200A,
			WID_TX_FRAGMENT_COUNT              = 0x200B,
			WID_TX_MULTICAST_FRAME_COUNT       = 0x200C,
			WID_RTS_SUCCESS_COUNT              = 0x200D,
			WID_RTS_FAILURE_COUNT              = 0x200E,
			WID_WEP_UNDECRYPTABLE_COUNT        = 0x200F,
			WID_REKEY_PERIOD                   = 0x2010,
			WID_REKEY_PACKET_COUNT             = 0x2011,
#ifdef MAC_HW_UNIT_TEST_MODE
            WID_Q_ENABLE_INFO                  = 0x2012,
#else /* MAC_HW_UNIT_TEST_MODE */
			WID_802_1X_SERV_ADDR               = 0x2012,
#endif /* MAC_HW_UNIT_TEST_MODE */
			WID_STACK_IP_ADDR                  = 0x2013,
			WID_STACK_NETMASK_ADDR             = 0x2014,
			WID_HW_RX_COUNT                    = 0x2015,
			WID_MEMORY_ADDRESS                 = 0x201E,
			WID_MEMORY_ACCESS_32BIT            = 0x201F,
            WID_RF_REG_VAL                     = 0x2021,
            WID_FIRMWARE_INFO                  = 0x2023,

			WID_SSID                           = 0x3000,
			WID_FIRMWARE_VERSION               = 0x3001,
			WID_OPERATIONAL_RATE_SET           = 0x3002,
			WID_BSSID                          = 0x3003,
			WID_WEP_KEY_VALUE0                 = 0x3004,
			WID_WEP_KEY_VALUE1                 = 0x3005,
			WID_WEP_KEY_VALUE2                 = 0x3006,
			WID_WEP_KEY_VALUE3                 = 0x3007,
			WID_802_11I_PSK                    = 0x3008,
			WID_HCCA_ACTION_REQ                = 0x3009,
			WID_802_1X_KEY                     = 0x300A,
			WID_HARDWARE_VERSION               = 0x300B,
			WID_MAC_ADDR                       = 0x300C,
			WID_PHY_TEST_DEST_ADDR             = 0x300D,
			WID_PHY_TEST_STATS                 = 0x300E,
			WID_PHY_VERSION                    = 0x300F,
			WID_SUPP_USERNAME                  = 0x3010,
			WID_SUPP_PASSWORD                  = 0x3011,
			WID_SITE_SURVEY_RESULTS            = 0x3012,
			WID_RX_POWER_LEVEL                 = 0x3013,

			WID_ADD_WEP_KEY                    = 0x3019,
			WID_REMOVE_WEP_KEY                 = 0x301A,
			WID_ADD_PTK                        = 0x301B,
			WID_ADD_RX_GTK                     = 0x301C,
			WID_ADD_TX_GTK                     = 0x301D,
			WID_REMOVE_KEY                     = 0x301E,
			WID_ASSOC_REQ_INFO                 = 0x301F,
			WID_ASSOC_RES_INFO                 = 0x3020,
			WID_UPDATE_RF_SUPPORTED_INFO       = 0x3021,
			WID_COUNTRY_IE                     = 0x3022,
			WID_MANUFACTURER                   = 0x3026, /*Added for CAPI tool */
			WID_MODEL_NAME	   				   = 0x3027, /*Added for CAPI tool */
			WID_MODEL_NUM                      = 0x3028, /*Added for CAPI tool */
			WID_DEVICE_NAME					   = 0x3029, /*Added for CAPI tool */
			WID_PMKID_INFO                     = 0x3082,



			WID_CONFIG_HCCA_ACTION_REQ         = 0x4000,
			WID_UAPSD_CONFIG                   = 0x4001,
			WID_UAPSD_STATUS                   = 0x4002,
			WID_WMM_AP_AC_PARAMS               = 0x4003,
			WID_WMM_STA_AC_PARAMS              = 0x4004,
            WID_NEWORK_INFO                    = 0x4005,
            WID_STA_JOIN_INFO                  = 0x4006,
            WID_CONNECTED_STA_LIST             = 0x4007,
            WID_HUT_STATS                      = 0x4082,
			WID_SCAN_CHANNEL_LIST			   = 0x4084,
			/*BugID_3746 WID to add IE to be added in next probe request*/
              WID_INFO_ELEMENT_PROBE	= 0x4085,
		/*BugID_3746 WID to add IE to be added in next associate request*/
              WID_INFO_ELEMENT_ASSOCIATE	= 0x4086,
			WID_ALL                            = 0x7FFE,
			WID_MAX                            = 0xFFFF
} WID_T;

typedef enum {

DONT_RESET   = 0,
DO_RESET     = 1,
NO_REQUEST   = 2

} RESET_REQ;

#endif
typedef struct _tstrSimulatorMsg
{
	NMI_Uint32 u32SimThreadCmd;
	NMI_Uint8 au8PktData[MAX_BUFFER_LEN];
	NMI_Sint32 s32PktDataLen;
}tstrSimulatorMsg;

typedef struct
{
    UWORD16 id;    /* WID Identifier       */
    BOOL_T  rsp;   /* WID_ALL response     */
    BOOL_T  reset; /* Reset MAC required   */
    void    *get;  /* Get Function Pointer */
    void    *set;  /* Set Function Pointer */
} host_wid_struct_t;


#ifdef CONNECT_DIRECT
typedef struct _tstrWidJoinReqExt
{
	NMI_Char   SSID[MAX_SSID_LEN];
	NMI_Uint8   u8channel;
	NMI_Uint8   BSSID[6];
}tstrWidJoinReqExt;
#endif


static void SimulatorThread(void* pvUsrArg);
static void SendMacStatus(void* pvUsrArg);

extern void send_host_rsp(NMI_Uint8 *host_rsp, NMI_Uint16 host_rsp_len);

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
static NMI_MsgQueueHandle gMsgQSimulator;

static NMI_ThreadHandle ThrdHandleSimulator = NMI_NULL;
static NMI_ThreadHandle ThrdHandleScan = NMI_NULL;
static NMI_ThreadHandle ThrdHandleMacStatus = NMI_NULL;
static NMI_SemaphoreHandle SemHandleScanReq ;
static NMI_SemaphoreHandle SemHandleScanExit ;
static NMI_SemaphoreHandle SemHandleMacStsExit;
static NMI_Bool gbTermntMacStsThrd = NMI_FALSE;
BOOL_T  bscan_code    			= NMI_FALSE;

BOOL_T  g_reset_mac                      = BFALSE;
UWORD16 g_current_len                    = 0;
UWORD8  g_current_settings[MAX_QRSP_LEN] = {0};
UWORD8  g_cfg_val[MAX_CFG_LEN]           = {0};
UWORD8  g_phy_ver[MAX_PROD_VER_LEN + 1]  = {0};
UWORD32 g_last_w_seq                     = 0x1FF;
UWORD8  g_info_id                        = 0;
UWORD8  g_network_info_id                = 0;
UWORD8	g_scan_complete_id				= 0;
RESET_REQ_T g_reset_req_from_user          = NO_REQUEST;
UWORD16 Rsp_Len;

BOOL_T g_short_slot_allowed = BTRUE;
BOOL_T g_reset_mac_in_progress = BFALSE;
static UWORD8 g_current_mac_status = MAC_DISCONNECTED;

static tHANDLE ghTxFifoBuffer;
static int g_connected_bssIdx = 0xFFFF;

/*****************************************************************************/
/* Inline Functions                                                          */
/*****************************************************************************/
//#include "host_if.h"
INLINE UWORD16 config_host_hdr_offset(BOOL_T more_frag, UWORD16 frag_offset)
{
#ifdef ETHERNET_HOST
    if((frag_offset == 0) && (more_frag == BFALSE))
    {
        return CONFIG_PKT_HDR_LEN;
    }
    else
    {
        return (CONFIG_PKT_HDR_LEN - UDP_HDR_OFFSET);
    }
#endif /* ETHERNET_HOST */




    return 0;
}

/* The purpose of dot11StationID is to allow a manager to identify a station */
/* for its own purposes. This attribute provides for that eventuality while  */
/* keeping the true MAC address independent. The default value is the STA's  */
/* assigned, unique MAC address.                                             */
UWORD8 mac[6] = {0x11,0x22,0x33,0x44,0x55,0x66};
INLINE UWORD8* mget_StationID(void)
{
    return mac;
}

INLINE void mset_StationID(UWORD8* inp)
{
    
}

/* This attribute shall specify the number of TUs that requesting STA should */
/* wait for a response to a transmitted association-request MMPDU.           */
INLINE UWORD32 mget_AssociationResponseTimeOut(void)
{
    return 0;
}

INLINE void mset_AssociationResponseTimeOut(UWORD32 inp)
{
    
}


INLINE UWORD32 get_enable_ch(void);



INLINE void set_reset_req(UWORD8 val)
{
    g_reset_req_from_user =(RESET_REQ_T) val;
}
NMI_Uint8* au8WepKey[30];
INLINE void set_wep_key()
{
	PRINT_INFO(CORECONFIG_DBG,"In set wep key \n");	
}

/* Function to update PMKID Information */
INLINE void set_pmkid_info(UWORD8 *val)
{
#ifdef IBSS_BSS_STATION_MODE
    set_pmkid_cache(val);
#endif /* IBSS_BSS_STATION_MODE */
}

/* Function to query PMKID Information */
INLINE UWORD8 *get_pmkid_info(void)
{
    UWORD8 *retval = NULL;

#ifdef IBSS_BSS_STATION_MODE
    retval = get_pmkid_cache();
#endif /* IBSS_BSS_STATION_MODE */

    return retval;
}

#ifdef IBSS_BSS_STATION_MODE
INLINE UWORD8* get_assoc_req_info(void)
{
        return ( ( UWORD8* )g_assoc_req_info );
}
INLINE UWORD8* get_assoc_res_info(void)
{
        return ( ( UWORD8* )g_assoc_res_info );
}
#endif /* IBSS_BSS_STATION_MODE */

/* Set the Short Slot option                                */
/* This function needs to be here because it is used in the */
/* subsequent functions                                     */
INLINE void set_short_slot_allowed(UWORD8 val)
{
   
}

/* This function returns the current slot option */
//INLINE UWORD8 get_short_slot_allowed(void)
//{
//    return g_short_slot_allowed;
//}

/* This function returns the SSID in the format required by the host. */
//INLINE WORD8* get_DesiredSSID(void)
//{
//    WORD8 *ssid_temp = mget_DesiredSSID();
//
//    g_cfg_val[0] = strlen((const char*)ssid_temp);
//    strcpy( (char *)( g_cfg_val + 1 ), (const char*)ssid_temp);
//
//    return ( ( WORD8* )g_cfg_val );
//}

/* This function returns the group address in the format required by host. */
//INLINE UWORD8* get_GroupAddress(void)
//{
//    UWORD8 *grpa_temp = mget_GroupAddress();
//
//    g_cfg_val[0] = grpa_temp[GROUP_ADDRESS_SIZE_OFFSET]*6;
//    strcpy((char *)(g_cfg_val + 1), (const char*)grpa_temp);
//
//    return g_cfg_val;
//}

/* This function returns the product version in the format required by host. */
char firmver[] = "10.0";
INLINE UWORD8* get_manufacturerProductVersion(void)
{
    UWORD8 *prod_ver_temp = firmver; /* mget_manufacturerProductVersion(); */

    g_cfg_val[0] = strlen((const char*)prod_ver_temp);
    NMI_memcpy((char *)(g_cfg_val + 1), (const char*)prod_ver_temp,NMI_strlen(firmver));

    return g_cfg_val;
}

/* This function returns the product version in the format required by host. */
//INLINE UWORD8* get_hardwareProductVersion(void)
//{
//    UWORD32 hard_ver_temp = get_machw_pa_ver();
//    UWORD8  index         = 1;
//    UWORD8  temp          = 0;
//
//    /* Format version as 'x.x.x' */
//    temp = ((hard_ver_temp & 0xF0000000) >> 28) * 16 +
//        ((hard_ver_temp & 0x0F000000) >> 24);
//    if(temp > 9)
//    {
//        g_cfg_val[index++] = (UWORD8)(temp/10) + '0';
//    }
//    g_cfg_val[index++] = temp - (((UWORD8)(temp/10))*10) + '0';
//    g_cfg_val[index++] = '.';
//    temp = ((hard_ver_temp & 0x00F00000) >> 20) * 16 +
//        ((hard_ver_temp & 0x000F0000) >> 16);
//    if(temp > 9)
//    {
//        g_cfg_val[index++] = (UWORD8)(temp/10) + '0';
//    }
//    g_cfg_val[index++] = temp - (((UWORD8)(temp/10))*10) + '0';
//    g_cfg_val[index++] = '.';
//    temp = ((hard_ver_temp & 0x0000F000) >> 12) * 16 +
//        ((hard_ver_temp & 0x00000F00) >> 8);
//    if(temp > 9)
//    {
//        g_cfg_val[index++] = (UWORD8)(temp/10) + '0';
//    }
//    g_cfg_val[index++] = temp - (((UWORD8)(temp/10))*10) + '0';
//    g_cfg_val[0] = index - 1;
//    return g_cfg_val;
//}


/* This function returns the product version in the format required by host. */
INLINE UWORD8* get_phyProductVersion(void)
{
	PRINT_INFO(CORECONFIG_DBG,"Product version\n");
    return g_phy_ver;
}

/* This function returns the BSSID in the format required by the host.*/
NMI_Uint8 bssid_addr [6] = {0x66,0x55,0x44,0x33,0x22,0x11};
INLINE UWORD8* get_bssid(void)
{
  
	UWORD8 *bssid_temp = bssid_addr;//mget_bssid();

    g_cfg_val[0] = MAX_ADDRESS_LEN;
    NMI_memcpy(g_cfg_val + 1, bssid_temp, MAX_ADDRESS_LEN);

    return g_cfg_val;

}

/* This function returns the MAC Address in the format required by the host.*/
UWORD8 mac_address[] = {0x11,0x22,0x33,0x44,0x55,0x66};
INLINE UWORD8* get_mac_addr(void)
{
#ifndef MAC_HW_UNIT_TEST_MODE
    UWORD8 *mac_addr_temp = mac_address; /* mget_StationID(); */

    g_cfg_val[0] = MAX_ADDRESS_LEN;
	NMI_memcpy(g_cfg_val + 1, mac_addr_temp, MAX_ADDRESS_LEN);


    return g_cfg_val;
#else /* MAC_HW_UNIT_TEST_MODE */
    g_cfg_val[0] = MAX_ADDRESS_LEN;
    NMI_memcpy(&g_cfg_val[1], g_test_params.mac_address, MAX_ADDRESS_LEN);
    return g_cfg_val;
#endif /* MAC_HW_UNIT_TEST_MODE */
}

/* This function returns the Supported Rates in the format required by host. */
INLINE UWORD8* get_supp_rates(void)
{

#ifdef PHY_802_11b
    UWORD8 *str = (UWORD8 *)"1,2,5.5,11";
    g_cfg_val[0] = strlen((const char*)str);
    NMI_memcpy(g_cfg_val + 1, str, g_cfg_val[0]);
#endif /* PHY_802_11b */

#ifdef PHY_802_11a
    UWORD8 *str = (UWORD8 *)"6,9,12,18,24,36,48,54";
    g_cfg_val[0] = strlen((const char*)str);
    NMI_memcpy(g_cfg_val + 1, str, g_cfg_val[0]);
#endif /* PHY_802_11a */

#ifdef PHY_802_11g
    UWORD8 *str = (UWORD8 *)"1,2,5.5,11,6,9,12,18,24,36,48,54";
    g_cfg_val[0] = strlen((const char*)str);
    NMI_memcpy(g_cfg_val + 1, str, g_cfg_val[0]);
#endif /* PHY_802_11g */

    return g_cfg_val;
}

/*  BSS Type formats for Host and MAC                                        */
/*  --------------------------------------------------------------------     */
/*                  Infrastructure    Independent    Access Point            */
/*  Host :                 0               1            2                    */
/*  MIB  :                 1               2            3                    */
/*  --------------------------------------------------------------------     */

/* This function returns the BSS Type in the format required by the host. */
INLINE UWORD8 get_DesiredBSSType(void)
{
    

    return 0;
}

/* This function returns the BSS Type in the format required by the MAC.*/
INLINE void set_DesiredBSSType(UWORD8 bss_type)
{

}

/*  Channel formats for Host and MAC                                         */
/*  --------------------------------------------------------------------     */
/*          CHANNEL1      CHANNEL2 ....                     CHANNEL14        */
/*  Host :         1             2                                 14        */
/*  MIB  :         0             1                                 13        */
/*  --------------------------------------------------------------------     */

/* This function returns the Channel in the format required by the host.*/
int g_chnum = 0;
INLINE UWORD8 get_host_chnl_num(void)
{
	PRINT_INFO(CORECONFIG_DBG,"Get MAC channel [%d]\n",g_chnum);
	return g_chnum;    
}

/* This function returns the Channel in the format required by the MAC.*/
INLINE void set_mac_chnl_num(UWORD8 cnum)
{
	PRINT_INFO(CORECONFIG_DBG,"Set MAC channel to [%d]\n",cnum);
	g_chnum = cnum;
}

static NMI_Uint8 au8SrvyResFrstFrag[91] = {90, 88, 0, 'b', 'o', 'o', 'n', 'd', 'o', 'g', 'g', 'l',
									'e', '\0', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
									0, 0, 0, 0, 0, 0, 0, 0, 0x02, 0x0b, 0x51,
									0x00, 0x1a, 0x70, 0xfc, 0xc0, 0x6f, 0x17, 1,
									'N', 'P', 'M', 'E', 'D', 'C', '-', 'B', 'G', '\0', 0,
									0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
									0, 0, 0, 0, 0x52, 0x04, 0x13,
									0x00, 0x1a, 0xc1, 0x26, 0x11, 0xc0, 0x42, 0x30};

static NMI_Uint8 au8SrvyResScndFrag[47] = {46, 44, 1, 'n', 'w', 'i', 'f', 'i', '\0', 0, 0, 0, 0, 0,
									0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
									0, 0, 0, 0x02, 0x0b, 0x51, 0x1c, 0x7e, 0xe5, 0x49, 0x76, 
									0x31, 0x17, 1};

NMI_Uint32 gu32ReqSrvyResFrag = 0;
/* This function returns the site survey results */
INLINE UWORD8 *get_site_survey_results(void)
{
#ifdef IBSS_BSS_STATION_MODE
    return get_site_survey_results_sta();
#else  /* IBSS_BSS_STATION_MODE */
	if((gu32ReqSrvyResFrag >= 0) && (gu32ReqSrvyResFrag < 1))
	{
		gu32ReqSrvyResFrag++;
		return au8SrvyResFrstFrag;
	}
	else if(gu32ReqSrvyResFrag == 1)
	{
		gu32ReqSrvyResFrag = 0;
		return au8SrvyResScndFrag;
	}
	else
	{
		NMI_ERROR("invalid fragment num of Site Survey Results \n");
		return NULL;
	}
#endif /* IBSS_BSS_STATION_MODE */
}


static NMI_Uint8 g_assoc_res_info[] = {71, 0x21, 0x04, 0x00, 0x00, 0x02, 0xc0, 0x01, 0x08, 0x82, 0x84, 0x8b, 0x0c, 
								     0x12, 0x96, 0x18, 0x24, 0x32, 0x04, 0x30, 0x48, 0x60, 0x6c, 0xdd, 0x18, 
								     0x00, 0x50, 0xf2, 0x02, 0x01, 0x01, 0x83, 0x00, 0x03, 0xa4, 0x00, 0x00, 
								     0x27, 0xa4, 0x00, 0x00, 0x42, 0x43, 0x5e, 0x00, 0x62, 0x32, 0x2f, 0x00, 
								     0xdd, 0x09, 0x00, 0x03, 0x7f, 0x01, 0x01, 0x00, 0x00, 0xff, 0x7f, 0xdd, 
								     0x0a, 0x00, 0x03, 0x7f, 	0x04, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00};

INLINE UWORD8* get_assoc_res_info(void)
{
        return g_assoc_res_info;
}


/*  Rate formats for Host and MAC                                            */
/*  -----------------------------------------------------------------------  */
/*          1   2   5.5   11   6  9  12  18  24  36  48   54  Auto           */
/*  Host :  1   2     3    4   5  6   7   8   9  10  11   12  0              */

/*  MAC  :  1   2     5   11   6  9  12  18  24  36  48   54  Not supported  */
/*  -----------------------------------------------------------------------  */

/* This function returns the transmission rate to the user.*/
int g_txrate = 0;
INLINE UWORD8 get_tx_rate(void)
{
	return g_txrate;
}

/* This function sets the transmission rate as requested by the user.*/
INLINE void set_tx_rate(UWORD8 rate)
{
	/*NMI_PRINTF("\nRate is set: %d",rate);*/
	g_txrate = rate;

}

/*  Preamble formats for Host and MAC                                        */
/*  -----------------------------------------------------------------------  */
/*          Long          Short                                              */
/*  Host :  1              0                                                 */
/*  MAC  :  1              0                                                 */
/*  -----------------------------------------------------------------------  */

/* This function returns the preamble type as set by the user.*/
INLINE UWORD8 get_preamble_type(void)
{
    //Whether auto or long only
    return 0;
}

/* This function sets the preamble type as requested by the user.*/
INLINE void set_preamble_type(UWORD8 idx)
{  
}

/* This function sets the RTS threshold as requested by the user. The MAC    */
/* H/w register is also set accordingly.                                     */
INLINE void set_RTSThreshold(UWORD16 limit)
{
    
}

/* This function sets the Fragmentation threshold as requested by the user.  */
/* The MAC H/w register is also set accordingly.                             */
INLINE void set_FragmentationThreshold(UWORD16 limit)
{
      #if 0
      set_machw_frag_thresh(limit);
      #endif
}

/* This function sets the Short Retry Limit as requested by the user. The    */
/* MAC H/w register is also set accordingly.                                 */
INLINE void set_ShortRetryLimit(UWORD8 limit)
{
 
}

/* This function sets the Long Retry Limit as requested by the user. The     */
/* MAC H/w register is also set accordingly.                                 */
INLINE void set_LongRetryLimit(UWORD8 limit)
{
 
}

/* This function returns the current protection mode */
INLINE UWORD8 get_protection_mode(void)
{

    return 0;
}

/* This function sets the protection mode */
INLINE void set_protection_mode(UWORD8 prot_mode)
{


}

/* Station power save modes */
typedef enum {MIB_ACTIVE    = 1,
              MIB_POWERSAVE = 2
} PS_MODE_T;
/* Get the current value of power management mode */
INLINE UWORD8 get_PowerManagementMode(void)
{
	return 0;
}

/* Set the power management mode according to value */
INLINE void set_PowerManagementMode(UWORD8 val)
{

}


/* Get the current value of ack policy type */
INLINE UWORD8 get_ack_type(void)
{
    return 0;
}

/* Set the value of the Ack Policy to the value */
INLINE void set_ack_type(UWORD8 val)
{
    
}

/* Get the current value of PCF mode supported */
INLINE UWORD8 get_pcf_mode(void)
{
    return 0;
}

/* Set the value of the of PCF mode to the value */
INLINE void set_pcf_mode(UWORD8 val)
{
   
}

/* Get the current value of PCF mode supported */
INLINE UWORD8 get_bcst_ssid(void)
{
    return 0;
}

/* Set the value of the of PCF mode to the value */
INLINE void set_bcst_ssid(UWORD8 val)
{
    
}

/* This function sets the MAC Address required for the MH/PHY Test.*/
INLINE UWORD16 get_phy_test_frame_len(void)
{
#ifdef MAC_HW_UNIT_TEST_MODE
#ifndef MAC_WMM
    return g_test_config.tx.nor.frame_len;
#else /* MAC_WMM */
    return g_test_config.tx.ac_vo.frame_len;
#endif /* MAC_WMM */
#else /* MAC_HW_UNIT_TEST_MODE */
    return 0;
#endif /* MAC_HW_UNIT_TEST_MODE */

}

INLINE void set_phy_test_frame_len(UWORD16 val)
{
#ifdef MAC_HW_UNIT_TEST_MODE
    g_test_stats.txci = 0;
    g_test_stats.rxd.type.ack = 0;
#ifndef MAC_WMM
    g_test_config.tx.nor.frame_len = val;
#else /* MAC_WMM */
    g_test_config.tx.ac_vo.frame_len = val;
#endif /* MAC_WMM */
#endif /* MAC_HW_UNIT_TEST_MODE */
}

/* This function sets the MAC Address required for the MH/PHY Test.*/
INLINE UWORD8* get_phy_test_dst_addr(void)
{
#ifdef MAC_HW_UNIT_TEST_MODE
    g_cfg_val[0] = MAX_ADDRESS_LEN;
    NMI_memcpy(&g_cfg_val[1], g_test_config.tx.da, MAX_ADDRESS_LEN);
    return g_cfg_val;
#else /* MAC_HW_UNIT_TEST_MODE */
    return 0;
#endif /* MAC_HW_UNIT_TEST_MODE */
}

INLINE void set_phy_test_dst_addr(UWORD8* val)
{
#ifdef MAC_HW_UNIT_TEST_MODE
    NMI_memcpy(g_test_config.tx.da, val, MAX_ADDRESS_LEN);
#endif /* MAC_HW_UNIT_TEST_MODE */
}

INLINE UWORD32 get_q_enable_info(UWORD32 val)
{
#ifdef MAC_HW_UNIT_TEST_MODE
    return (UWORD32) g_test_config.tx.q_enable_info;
#else /* MAC_HW_UNIT_TEST_MODE */
    return 0;
#endif /* MAC_HW_UNIT_TEST_MODE */
}

INLINE void set_q_enable_info(UWORD32 val)
{
#ifdef MAC_HW_UNIT_TEST_MODE
    g_test_config.tx.q_enable_info = val;
#endif /* MAC_HW_UNIT_TEST_MODE */
}

/* This function sets the MAC Address required for the MH/PHY Test.*/
INLINE UWORD32 get_phy_test_pkt_cnt(void)
{
#ifdef MAC_HW_UNIT_TEST_MODE
    return g_test_config.tx.num_pkts;
#else /* MAC_HW_UNIT_TEST_MODE */
    return 0;
#endif /* MAC_HW_UNIT_TEST_MODE */
}

INLINE void set_phy_test_pkt_cnt(UWORD32 val)
{
#ifdef MAC_HW_UNIT_TEST_MODE
    g_test_config.tx.num_pkts = val;
    g_test_stats.txci = 0;
    g_test_stats.rxd.type.ack = 0;
#endif /* MAC_HW_UNIT_TEST_MODE */
}

/* This function sets the MAC Address required for the MH/PHY Test.*/
INLINE UWORD32 get_phy_test_txd_pkt_cnt(void)
{
#ifdef MAC_HW_UNIT_TEST_MODE
    return g_test_stats.txci;
#else /* MAC_HW_UNIT_TEST_MODE */
    return 0;
#endif /* MAC_HW_UNIT_TEST_MODE */
}

INLINE void set_phy_test_txd_pkt_cnt(UWORD32 val)
{
#ifdef MAC_HW_UNIT_TEST_MODE
    g_test_stats.txci = 0;
#endif /* MAC_HW_UNIT_TEST_MODE */
}

/* This function sets the MAC Address required for the MH/PHY Test.*/
INLINE UWORD8 get_phy_test_pattern(void)
{
#ifdef MAC_HW_UNIT_TEST_MODE
    return g_test_config.tx.pattern_type;
#else /* MAC_HW_UNIT_TEST_MODE */
    return 0;
#endif /* MAC_HW_UNIT_TEST_MODE */
}

INLINE void set_phy_test_pattern(UWORD8 val)
{
#ifdef MAC_HW_UNIT_TEST_MODE
    g_test_config.tx.pattern_type = val;
#endif /* MAC_HW_UNIT_TEST_MODE */
}

/* This function gets the current test statistics. To interpret these stats at */
/* the host, structure test_stats_struct_t should be the same at the host and  */
/* device side.                                                                */
INLINE UWORD8* get_hut_stats(void)
{
#ifdef MAC_HW_UNIT_TEST_MODE

    UWORD8 trigger_stat_lsb = 0;
    UWORD8 trigger_stat_msb = 0;
    UWORD16 stats_size      = 0;

    read_phy_reg(rAGCTRIGSTATLSB, &trigger_stat_lsb);
    read_phy_reg(rAGCTRIGSTATMSB, &trigger_stat_msb);
    g_test_stats.rxd.agc_trig_stats = (trigger_stat_msb << 8) + trigger_stat_lsb;

    stats_size = MIN(sizeof(test_stats_struct_t),(MAX_STATS_BUFFER_LEN - 1));

    g_stats_buf[0] = stats_size & 0x00FF;
    g_stats_buf[1] = (stats_size & 0xFF00) >> 8;

    NMI_memcpy(&g_stats_buf[2], &g_test_stats, stats_size);

    return g_stats_buf;
#else  /* MAC_HW_UNIT_TEST_MODE */
    return 0;
#endif /* MAC_HW_UNIT_TEST_MODE */
}

/* This function clears the current test statistics. */
INLINE void set_hut_stats(UWORD8* val)
{
#ifdef MAC_HW_UNIT_TEST_MODE
    memset(&g_test_stats, 0, sizeof(test_stats_struct_t));
#endif /* MAC_HW_UNIT_TEST_MODE */
}

/* This function sets the MAC Address required for the MH/PHY Test.*/
INLINE UWORD8* get_phy_test_stats(void)
{
#ifdef MAC_HW_UNIT_TEST_MODE
    g_cfg_val[0] = MIN(sizeof(test_stats_struct_t),(MAX_CFG_LEN - 1));
    NMI_memcpy(&g_cfg_val[1], &g_test_stats, g_cfg_val[0]);
    return g_cfg_val;
#else /* MAC_HW_UNIT_TEST_MODE */
    return 0;
#endif /* MAC_HW_UNIT_TEST_MODE */
}

INLINE void set_phy_test_stats(UWORD8* val)
{


}

/* Get/Set the read MAC Address from the SDRAM */
INLINE void set_read_addr_sdram(UWORD8 val)
{
 
}

INLINE UWORD8 get_read_addr_sdram(void)
{
    return 0;
}

/* Get/Set the Tx Power Level */
INLINE void set_tx_power_level_11a(UWORD8 val)
{
   
}

INLINE UWORD8 get_tx_power_level_11a(void)
{
    return 0;
}

/* Set the new DTIM Period */
INLINE void set_dtim_period(UWORD8 val)
{
   
}

/* Set the local Stack IP Address */
INLINE void set_stack_ip_addr(UWORD32 val)
{
   
}

INLINE UWORD32 get_stack_ip_addr(void)
{
   
    return 0;
}


/* Set the local Stack netmask */
INLINE void set_stack_netmask_addr(UWORD32 val)
{
   
}

INLINE UWORD32 get_stack_netmask_addr(void)
{
    
    return 0;
}

/* Set Phy active Register */
INLINE void set_phy_active_reg(UWORD8 val)
{
    
}

/* Set Phy active Register */
INLINE UWORD8 get_phy_active_reg(UWORD8 val)
{
    return 0;
}

/* Set Phy active Register value */
INLINE UWORD8 get_phy_active_reg_val(void)
{
	
    return 0;
	
}

INLINE void set_phy_active_reg_val(UWORD8 val)
{
   
}

INLINE void set_rf_reg_val(UWORD32 val)
{
    
}

/* Get/Set the Tx Power Level */
INLINE void set_tx_power_level_11b(UWORD8 val)
{
    
}

INLINE UWORD8 get_tx_power_level_11b(void)
{
    return 0;
}

/* Get the RSSI */
INLINE WORD8 get_rssi(void)
{
    return 0;
}


/* Get the Current MAC Status */
INLINE UWORD8 get_current_mac_status(void)
{
    return 0;
}

INLINE UWORD8  get_auto_rx_sensitivity(void)
{
    return 0;
}

INLINE void  set_auto_rx_sensitivity(UWORD8 input)
{

}

INLINE UWORD8  get_dataflow_control(void)
{
    return 0;
}

INLINE void  set_dataflow_control(UWORD8 input)
{
	
}

INLINE UWORD8 get_scan_filter(void)
{

    return 0;

}

INLINE void set_scan_filter(UWORD8 input)
{

}

INLINE UWORD8 get_link_loss_threshold(void)
{
	return 0;
}

INLINE void set_link_loss_threshold(UWORD8 input)
{

}

INLINE UWORD8 get_autorate_type(void)
{
    return 0;
}

INLINE void set_autorate_type(UWORD8 input)
{
    
}

INLINE UWORD8 get_cca_threshold(void)
{
    return 0;
}

INLINE void set_cca_threshold(UWORD8 input)
{
    
}

INLINE UWORD32 get_enable_ch(void)
{
    return 0;
}

INLINE void set_enable_ch(UWORD32 input)
{
   
}

/* This function returns the antenna selected */
INLINE UWORD8 get_antenna_selection(void)
{   
    return 0;
}

/* This function sets the antenna to be used */
INLINE void set_antenna_selection(UWORD8 mode)
{
    
}

INLINE void set_FailedCount_to_zero(UWORD32 val)
{
   
}

INLINE void set_RetryCount_to_zero(UWORD32 val)
{
   
}

INLINE void set_MultipleRetryCount_to_zero(UWORD32 val)
{
   
}

INLINE void set_FrameDuplicateCount_to_zero(UWORD32 val)
{
   
}

INLINE void set_ACKFailureCount_to_zero(UWORD32 val)
{
 
}

INLINE void set_ReceivedFragmentCount_to_zero(UWORD32 val)
{
 
}

INLINE void set_MulticastReceivedFrameCount_to_zero(UWORD32 val)
{
 
}

INLINE void set_FCSErrorCount_to_zero(UWORD32 val)
{
 
}

INLINE void set_TransmittedFrameCount_to_zero(UWORD32 val)
{
 
}

INLINE void set_config_HCCA_actionReq(UWORD8 *req)
{

}

INLINE UWORD8 get_user_control_enabled(void)
{
    return 0;
}

INLINE void set_user_control_enabled(UWORD8 val)
{
    
}
INLINE UWORD32 get_firmware_info(void)
{   
    return 0;
}

UWORD32 g_memory_address = 0x0; //just for testing Integer WIDs

INLINE void set_memory_address(UWORD32 input)
{
	PRINT_INFO(CORECONFIG_DBG,"Set mem addr to [%x] \n", input);
 	g_memory_address = input;
}
INLINE UWORD32 get_memory_address(void)
{
    PRINT_INFO(CORECONFIG_DBG,"Get mem addr [%x] \n", g_memory_address);
    return g_memory_address;
}
INLINE UWORD8 get_memory_access_8bit(void)
{
    
     return 0;
}
INLINE void set_memory_access_8bit(UWORD8 input)
{
 
}
INLINE UWORD16 get_memory_access_16bit(void)
{
 
     return 0;
 
}
INLINE void set_memory_access_16bit(UWORD16 input)
{
 
}
INLINE UWORD32 get_memory_access_32bit(void)
{
 
        return 0;
 
}
INLINE void set_memory_access_32bit(UWORD32 input)
{
    
}

/* This function sets the MAC Address required for the MH/PHY Test.*/
INLINE void set_mac_addr(UWORD8 *val)
{
}


/*
  Rx sensitivity parameter format (WID view)
   +----------------------+----------------------+
   |   PHYAGCMAXVGAGAIN   | PHYAGCFINALVGAGAINTH |
   +----------------------+----------------------+
    7                    0 15                   8 (bit)
*/
INLINE UWORD16 get_rx_sense(void)
{
    return 0;
}

INLINE void set_rx_sense(UWORD16 val)
{    
}

INLINE UWORD16 get_active_scan_time(void)
{

    return 0;

}

INLINE void set_active_scan_time(UWORD16 val)
{

}

INLINE UWORD16 get_passive_scan_time(void)
{

    return 0;

}

INLINE void set_passive_scan_time(UWORD16 val)
{

}

INLINE UWORD16 get_site_survey_scan_time(void)
{

    return 0;

}

INLINE void set_site_survey_scan_time(UWORD16 val)
{

}

INLINE UWORD16 get_join_timeout(void)
{
    return 0;
}

INLINE void set_join_timeout(UWORD16 val)
{
    
}

INLINE UWORD16 get_auth_timeout(void)
{
    return 0;
}

INLINE void set_auth_timeout(UWORD16 val)
{
    
}

INLINE UWORD16 get_asoc_timeout(void)
{
    return (UWORD16)mget_AssociationResponseTimeOut();
}

INLINE void set_asoc_timeout(UWORD16 val)
{
    mset_AssociationResponseTimeOut((UWORD32)val);
}

INLINE UWORD16 get_11i_protocol_timeout(void)
{
    return 0;
}

INLINE void set_11i_protocol_timeout(UWORD16 val)
{
    
}

INLINE UWORD16 get_eapol_response_timeout(void)
{

    return 0;
}

INLINE void set_eapol_response_timeout(UWORD16 val)
{
    
}

INLINE UWORD16 get_cca_busy_status(void)
{
    return 0;
}

INLINE void set_cca_busy_start(UWORD16 val)
{
}

/* Get the current value of site servey */
INLINE UWORD8 get_site_survey_status(void)
{

    return 0;

}

/* Set the site survey option according to the value */
INLINE void set_site_survey(UWORD8 val)
{

}


INLINE void set_preferred_bssid(UWORD8 *val)
{

}

NMI_Uint8 au8Psk[65] = {0};
INLINE void set_RSNAConfigPSKPassPhrase(UWORD8* psk)
{
	NMI_memcpy(au8Psk,psk,NMI_strlen(psk));
	au8Psk[NMI_strlen(au8Psk)] = '\0';
	PRINT_INFO(CORECONFIG_DBG,"\n[CoreSimulator] %s", au8Psk);
}
INLINE UWORD8* get_RSNAConfigPSKPassPhrase(void)
{
	return au8Psk;
}
/* Function to set g_sta_uapsd_config value */
NMI_Uint8 as8UapsdConfig[5]={0};
INLINE void set_uapsd_config(UWORD8 *val)
{
	NMI_memcpy(as8UapsdConfig,val,sizeof(as8UapsdConfig));

}

/* Function to set g_sta_uapsd_config value */
INLINE UWORD8* get_uapsd_config(void)
{
    return as8UapsdConfig;
}

/* Function to set g_sta_uapsd_status value */
INLINE UWORD8* get_uapsd_status(void)
{
    return 0;
}

/*****************************************************************************/
/* WMM configuration functions for AP mode                                   */
/*****************************************************************************/

/* Function to set AC parameter values to be used by the AP */
INLINE void set_wmm_ap_ac_params(UWORD8 *val)
{

}

/* Function to get AC parameter values in use by the AP */
INLINE UWORD8* get_wmm_ap_ac_params(void)
{

    return 0;
}

/* Function to set AC parameter values for STA associated with the AP */
INLINE void set_wmm_sta_ac_params(UWORD8 *val)
{

}

/* Function to get AC parameter values for STA associated with the AP */
INLINE UWORD8* get_wmm_sta_ac_params(void)
{

    return 0;
}

/* Function to set UAPSD SUPPORT in AP */
INLINE void set_uapsd_support_ap(UWORD8 val)
{

}

/* Function to get the UAPSD support of AP */
INLINE UWORD8 get_uapsd_support_ap(void)
{

    return BFALSE;
}

/**************** 802.11H related functions************************/

INLINE void set_802_11H_DFS_mode(BOOL_T val)
{

}

INLINE BOOL_T get_802_11H_DFS_mode(void)
{

	     return(BFALSE);
}


INLINE void set_802_11H_TPC_mode(BOOL_T val)
{

}

INLINE BOOL_T get_802_11H_TPC_mode(void)
{

         return(BFALSE);
}


INLINE UWORD8* get_rf_supported_info(UWORD8 ch_num)
{
	
	return 0;

}

INLINE void set_rf_supported_info(UWORD8 *info)
{	
}

/* Get the connected station list */
INLINE UWORD8* get_connected_sta_list(void)
{
    return 0;
}

/* Function to set the Country information element. Currently this is valid  */
/* only for 11h protocol and AP mode. Note that the incoming bytes are       */
/* directly stored in the global country information element. The assumption */
/* is that the setting will be in the format specified in the standard.      */
/* Make sure RF Supported Info is set before, whenever this is updated       */
INLINE void set_country_ie(UWORD8 *val)
{

}

/* Function to get the Country information element. Currently this is valid  */
/* only for 11h protocol and AP mode. The value returned will be in the      */
/* format specified in the standard.                                         */
INLINE UWORD8* get_country_ie(void)
{
    return 0;
}

INLINE void set_running_mode(UWORD8 mode)
{

}

/* This function returns the running mode for 802.11g PHY */
INLINE UWORD8 get_running_mode(void)
{
	return 0;
}


#ifdef DEBUG_MODE
INLINE void set_test_case_name(const char* input)
{
#ifdef DEBUG_PRINT
    if(g_reset_mac_in_progress == BFALSE)
        debug_print("\n ---- %s ---- \n",input);
#endif /* DEBUG_PRINT */
}
#endif /* DEBUG_MODE */


/* Scan Type that is to be used. 0 -> Passive Scan, 1 -> Active Scan         */
INLINE UWORD8 mget_scan_type(void)
{
    return 0;
}

INLINE void mset_scan_type(UWORD8 st)
{
    
}

INLINE UWORD8 mget_listen_interval(void)
{
    return 0;
}

INLINE void mset_listen_interval(UWORD8 li)
{
    
}


/* When this attribute is true, it shall indicate that the IEEE 802.11 WEP   */
/* mechanism is used for transmitting frames of type Data. The default value */
/* of this attribute shall be false.                                         */
INLINE UWORD8 mget_PrivacyInvoked(void)
{
    return 0;
}

/* This attribute shall indicate use of the first, second, third, or fourth  */
/* element of the WEPDefaultKeys array when set to values of zero, one, two, */
/* or three. The default value of this attribute shall be 0.                 */
/* REFERENCE "ISO/IEC 8802-11:1999, 8.3.2"                                   */
NMI_Uint8 u8WepKey;
INLINE UWORD8 mget_WEPDefaultKeyID(void)
{
	PRINT_INFO(CORECONFIG_DBG,"[CoreConfigSim]In get WEP default key: %d\n",u8WepKey);     
	return u8WepKey;
}
INLINE void mset_WEPDefaultKeyID(UWORD8 inp)
{
	u8WepKey = inp;
	PRINT_INFO(CORECONFIG_DBG,"In set WEP default key: %d \n",u8WepKey);     
}

INLINE UWORD8 get_qos_enable(void)
{
    return 0;
}
INLINE void set_qos_enable(UWORD8 val)
{

}
NMI_Uint8 u8Mode11I;
INLINE UWORD8 get_802_11I_mode(void)
{
	return u8Mode11I;
}

INLINE void set_802_11I_mode(UWORD8 val)
{
	u8Mode11I = val;
}

INLINE UWORD8 get_auth_type(void)
{
	return 0;
}
INLINE void set_auth_type(UWORD8 val)
{

}

/* This attribute shall specify the number of beacon intervals that shall    */
/* elapse between transmission of Beacons frames containing a TIM element    */
/* whose DTIM Count field is 0. This value is transmitted in the DTIM Period */
/* field of Beacon frames.                                                   */
INLINE UWORD8 mget_DTIMPeriod(void)
{
    return 0;
}

/* The attribute shall describe the number of DTIM intervals between the     */
/* start of CFPs. It is modified by MLME-START.request primitive.            */
INLINE UWORD8 mget_CFPPeriod(void)
{
    return 0;
}

INLINE void mset_CFPPeriod(UWORD8 inp)
{
   
}

void disconnect_station(UWORD8 assoc_id)
{

}

INLINE UWORD8 get_RSNAConfigGroupRekeyMethod(void)
{
	return 0;
}

INLINE void set_RSNAConfigGroupRekeyMethod(UWORD8 val)
{
}

INLINE UWORD8 get_short_slot_allowed(void)
{
	return 0;
}


/* our AP Beacon */

char aBeacon[] = {0x80,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x1a,0xc1,0x26,0x11,0xc0,
				  0x00,0x1a,0xc1,0x26,0x11,0xc0,0x30,0x41,0x81,0xc1,0x57,0xe3,0x1d,0x00,0x00,0x00,
				  0x64,0x00,0x11,0x04,0x00,0x09,0x4e,0x50,0x4d,0x45,0x44,0x43,0x2d,0x42,0x47,0x01,
				  0x08,0x82,0x84,0x8b,0x96,0x0c,0x18,0x30,0x48,0x03,0x01,0x04,0x05,0x04,0x00,0x01,
				  0x00,0x00,0x07,0x06,0x45,0x47,0x20,0x01,0x0b,0x14,0x2a,0x01,0x04,0x32,0x04,0x12,
				  0x24,0x60,0x6c,0x30,0x18,0x01,0x00,0x00,0x0f,0xac,0x02,0x02,0x00,0x00,0x0f,0xac,
				  0x02,0x00,0x0f,0xac,0x04,0x01,0x00,0x00,0x0f,0xac,0x02,0x00,0x00,0xdd,0x1c,0x00,
				  0x50,0xf2,0x01,0x01,0x00,0x00,0x50,0xf2,0x02,0x02,0x00,0x00,0x50,0xf2,0x04,0x00,
				  0x50,0xf2,0x02,0x01,0x00,0x00,0x50,0xf2,0x02,0x00,0x00,0xf0,0xa1,0x1e,0x32};


/* wepbeacon */

char wepbeacon[] = {0x80,0x00 ,0x00 ,0x00 ,0xff ,0xff ,0xff ,0xff,
					0xff ,0xff ,0x1c ,0x7e ,0xe5 ,0x49 ,0x76 ,0x31 ,0x1c ,0x7e ,0xe5 ,0x49 ,0x76 ,0x31 ,0x60 ,0x85 ,
					0x80 ,0x7d ,0xd4 ,0x0c ,0x00 ,0x00 ,0x00 ,0x00 ,0x64 ,0x00 ,0x31 ,0x04 ,0x00 ,0x05 ,0x6e ,0x77 ,
					0x69 ,0x66 ,0x69 ,0x01 ,0x08 ,0x82 ,0x84 ,0x8b ,0x96 ,0x0c ,0x12 ,0x18 ,0x24 ,0x03 ,0x01 ,0x0b ,
					0x05 ,0x04 ,0x62 ,0x64 ,0x00 ,0x00 ,0x2a ,0x01 ,0x00 ,0x32 ,0x04 ,0x30 ,0x48 ,0x60 ,0x6c ,0xdd ,
					0x18 ,0x00 ,0x50 ,0xf2 ,0x02 ,0x01 ,0x01 ,0x84 ,0x00 ,0x03 ,0xa4 ,0x00 ,0x00 ,0x27 ,0xa4 ,0x00 ,
					0x00 ,0x42 ,0x43 ,0x5e ,0x00 ,0x62 ,0x32 ,0x2f ,0x00 ,0xdd ,0x09 ,0x00 ,0x03 ,0x7f ,0x01 ,0x01 ,
					0x00 ,0x00 ,0xff ,0x7f ,0xdd ,0x0a ,0x00 ,0x03 ,0x7f ,0x04 ,0x01 ,0x00 ,0x00 ,0x00 ,0x40 ,0x00 ,
					0xdd ,0x27 ,0x00 ,0x50 ,0xf2 ,0x04 ,0x10 ,0x4a ,0x00 ,0x01 ,0x10 ,0x10 ,0x44 ,0x00 ,0x01 ,0x02 ,
					0x10 ,0x47 ,0x00 ,0x10 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x10 ,0x00 ,0x00 ,0x00 ,0x1c ,0x7e ,
					0xe5 ,0x49 ,0x76 ,0x31 ,0x10 ,0x3c ,0x00 ,0x01 ,0x03 ,0xdd ,0x05 ,0x00 ,0x50 ,0xf2 ,0x05, 0x00,
					0xb2, 0x98, 0xd4, 0x6a};

/* msa1 */

/*
char msa1[] = {0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x16, 0xb6, 0x19, 0x89, 0xde,
				  0x00, 0x16, 0xb6, 0x19, 0x89, 0xde, 0x00, 0x00, 0xf8, 0xf1, 0xd5, 0xe7, 0x01, 0x00, 0x00, 0x00,
				  0x64, 0x00, 0x11, 0x04, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x82, 0x84, 0x8b,
				  0x96, 0x24, 0x30, 0x48, 0x6c, 0x03, 0x01, 0x05, 0x05, 0x04, 0x00, 0x01, 0x00, 0x04, 0x2a, 0x01,
				  0x04, 0x2f, 0x01, 0x04, 0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f,
				  0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x00, 0x00, 0x32, 0x04, 0x0c, 0x12, 0x18, 0x60,
				  0xdd, 0x06, 0x00, 0x10, 0x18, 0x02, 0x03, 0x00};
*/

/* msa2 */

char msa2[] = {0x50, 0x00, 0x3a, 0x01, 0x00, 0x19, 0x7d, 0x1b, 0x03, 0xfa, 0x00, 0x1a, 0x70, 0xfc, 0xc0, 0x6f,
			   0x00, 0x1a, 0x70, 0xfc, 0xc0, 0x6f, 0x10, 0x00, 0x7e, 0x4b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			   0x64, 0x00, 0x01, 0x04, 0x00, 0x0a, 0x62, 0x6f, 0x6f, 0x6e, 0x64, 0x6f, 0x67, 0x67, 0x6c, 0x65,
			   0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, 0x03, 0x01, 0x0b, 0x2a, 0x01, 0x00,
			   0x2f, 0x01, 0x00, 0x32, 0x04, 0x0c, 0x12, 0x18, 0x60, 0xdd, 0x06, 0x00, 0x10, 0x18, 0x02, 0x00,
			   0x04};



#define BEACON_TEST_RSSI	19
void send_network_info_to_host(UWORD8 *msa, UWORD16 rx_len, signed char rssi);
void send_scan_complete_to_host(UWORD8 scan_done);
int gScanInProgress = 0;

static void SendScanResult(void* pvUsrArg)
{

	while(1)
	{
		NMI_SemaphoreAcquire(&SemHandleScanReq,NMI_NULL);
		if(bscan_code==NMI_TRUE)
		{
			NMI_Sleep(100);//100, 700, 2000
	
	//send_network_info_to_host(aBeacon,BEACON_TEST_LENGTH,BEACON_TEST_RSSI);
	/* TODO: Mostafa: uncomment the line: rx_len -= FCS_LEN; in the API send_network_info_to_host()
	   if its input argument rx_len includes FCS */
			send_network_info_to_host(wepbeacon, sizeof(wepbeacon), BEACON_TEST_RSSI);

			NMI_Sleep(100);

	//send_network_info_to_host(aBeacon,BEACON_TEST_LENGTH,BEACON_TEST_RSSI);
	/* TODO: Mostafa: uncomment the line: rx_len -= FCS_LEN; in the API send_network_info_to_host()
	   if its input argument rx_len includes FCS */
			send_network_info_to_host(msa2, sizeof(msa2), BEACON_TEST_RSSI);


			NMI_Sleep(100);

	//send_network_info_to_host(aBeacon,BEACON_TEST_LENGTH,BEACON_TEST_RSSI);
	/* TODO: Mostafa: uncomment the line: rx_len -= FCS_LEN; in the API send_network_info_to_host()
	   if its input argument rx_len includes FCS */
			send_network_info_to_host(aBeacon, sizeof(aBeacon), BEACON_TEST_RSSI);
			gScanInProgress = 0;
			send_scan_complete_to_host(NMI_TRUE);
		}
		else
		{
			NMI_SemaphoreRelease(&SemHandleScanExit,NMI_NULL);
			break;
		}	
			
	}
	
		
}	

INLINE void set_start_scan_req(UWORD8 val)
{
	PRINT_INFO(CORECONFIG_DBG,"Scan Started \n");   
	if(gScanInProgress)
	{
		return;
	}
	
	gScanInProgress = 1;
	
	bscan_code = NMI_TRUE;
	NMI_SemaphoreRelease(&SemHandleScanReq,NMI_NULL);
	
}

INLINE UWORD8 get_start_scan_req(void)
{
	return 0;
}

INLINE void set_join_req(UWORD8 val)
{
	g_connected_bssIdx = val;
	PRINT_INFO(CORECONFIG_DBG,"Connected to Network with Bss Idx %d \n", g_connected_bssIdx);
	
	g_current_mac_status = MAC_CONNECTED;
}

/*BugID_4124*/
/*Setter function for the new WID WID_JOIN_REQ_EXTENDED*/
/*function should copy the given desired AP parameters{SSID, Ch num and BSSID} to */
/*the MIB database and to the desired AP global variables */
/* No getter function implemented*/
INLINE void set_join_req_ext(UWORD8* pu8JoinReq)
{
	tstrWidJoinReqExt* pstrWidJoinReqExt = (tstrWidJoinReqExt*)pu8JoinReq;
	
	PRINT_INFO(CORECONFIG_DBG,"[Core Simulator]Connected to Network with SSID: %s,BSSID: %2x%2x%2x%2x%2x%2x,channel: %d \n", 
				pstrWidJoinReqExt->SSID, 
				pstrWidJoinReqExt->BSSID[0],
				pstrWidJoinReqExt->BSSID[1], 
				pstrWidJoinReqExt->BSSID[2], 
				pstrWidJoinReqExt->BSSID[3], 
				pstrWidJoinReqExt->BSSID[4], 
				pstrWidJoinReqExt->BSSID[5], 
				pstrWidJoinReqExt->u8channel);
	
	g_current_mac_status = MAC_CONNECTED;
}

/*BugID_3736*/
/*Setter function for the New WID_SCAN_CHANNEL_LIST*/
/*function should copy given list to the g_host_scan_channel_list */
/* setter function should also 'or' CHANNEL_LIST_SCAN with the */
/* global variable g_scan_source*/

/*first byte of the message should contain num of channels*/
/* then the channels list*/
INLINE void set_scan_channel_list(UWORD8* chList)
{
	UWORD16 u16ChnlListLen = chList[0];

    	#ifdef IBSS_BSS_STATION_MODE
		/*host channel List to be scanned after checking CHANNEL_LIST_SCAN*/
		/*in the g_scan_source*/

		memcpy(g_host_scan_channel_list,chList, u16ChnlListLen+2);

		//g_scan_source |= CHANNEL_LIST_SCAN;

	#else
    		return ;
	#endif /* IBSS_BSS_STATION_MODE */
}

/*BugID_3736*/
/* Getter Function for New WID_SCAN_CHANNEL_LIST*/
/* this function should return the scan_channel list set*/
/* previusly from the host interface*/
/* first byte of the returned message contains the num of channels*/
INLINE UWORD8* get_scan_channel_list(void)
{
	#ifdef IBSS_BSS_STATION_MODE

        	return g_host_scan_channel_list;
	#else
   	 	return 0;
	#endif /* IBSS_BSS_STATION_MODE */
}

/*BugID_3746*/
/*Setter function for the New WID_INFO_ELEMENT_PROBE*/
INLINE void set_info_element_probe(UWORD8* IEprobeReq)
{
	NMI_Uint16 u16WidLen;
	NMI_Uint32 i;
	
	//NMI_PRINTF("[Core_Simulator] IEprobeReq[0] = %02x \n", IEprobeReq[0]);
	//NMI_PRINTF("[Core_Simulator] IEprobeReq[1] = %02x \n", IEprobeReq[1]);

	u16WidLen = MAKE_WORD16(IEprobeReq[0], IEprobeReq[1]);

	PRINT_INFO(CORECONFIG_DBG,"WidLen of Probe Req IE is %d \n", u16WidLen);
	
	/*
	for(i = 0; i < u16WidLen; i++)
	{
		NMI_PRINTF("IEprobeReq[%d] = %02x \n", 2+i, IEprobeReq[2+i]);
	}
	*/
    	#ifdef IBSS_BSS_STATION_MODE		
	NMI_Uint16 u16WidLen;
		
	u16WidLen = MAKE_WORD16(IEprobeReq[0], IEprobeReq[1]);

	PRINT_INFO(CORECONFIG_DBG,"WidLen of Probe Req IE is %d \n", u16WidLen);

	
	
	#else
    		return;
	#endif /* IBSS_BSS_STATION_MODE */
}


/*BugID_3746*/
/* Getter Function for New WID_INFO_ELEMENT_PROBE*/
INLINE UWORD8* get_info_element_probe(void)
{
	#ifdef IBSS_BSS_STATION_MODE

        	//return g_host_scan_channel_list;
	#else
   	 	return 0;
	#endif /* IBSS_BSS_STATION_MODE */
}

/*BugID_3746*/
/*Setter function for the New WID_INFO_ELEMENT_ASSOCIATE*/
INLINE void set_info_element_associate(UWORD8* IEAssocReq)
{
	NMI_Uint16 u16WidLen;
	NMI_Uint32 i;
	
	//NMI_PRINTF("[Core_Simulator] IEAssocReq[0] = %02x \n", IEAssocReq[0]);
	//PRINT_INFO(CORECONFIG_DBG,"[Core_Simulator] IEAssocReq[1] = %02x \n", IEAssocReq[1]);
	
	u16WidLen = MAKE_WORD16(IEAssocReq[0], IEAssocReq[1]);

	PRINT_INFO(CORECONFIG_DBG,"WidLen of Assoc Req IE is %d \n", u16WidLen);
	
	/*
	for(i = 0; i < u16WidLen; i++)
	{
		NMI_PRINTF("IEAssocReq[%d] = %02x \n", 2+i, IEAssocReq[2+i]);
	}
	*/

    	#ifdef IBSS_BSS_STATION_MODE		
	NMI_Uint16 u16WidLen;
		
	u16WidLen = MAKE_WORD16(IEAssocReq[0], IEAssocReq[1]);

	PRINT_INFO(CORECONFIG_DBG,"WidLen of Assoc Req IE is %d \n", u16WidLen);


	
	#else
    		return;
	#endif /* IBSS_BSS_STATION_MODE */
}


/*BugID_3746*/
/* Getter Function for New WID_INFO_ELEMENT_ASSOCIATE*/
INLINE UWORD8* get_info_element_associate(void)
{
	#ifdef IBSS_BSS_STATION_MODE

        	//return g_host_scan_channel_list;
	#else
   	 	return 0;
	#endif /* IBSS_BSS_STATION_MODE */
}


UWORD8 au8CurrentSsid[30]={0};

INLINE void mset_DesiredSSID(UWORD8* ps8DesiredSsid)
{
	NMI_memcpy(au8CurrentSsid,ps8DesiredSsid,NMI_strlen(ps8DesiredSsid)+1);
}
INLINE UWORD8* get_DesiredSSID(void)
{
	return au8CurrentSsid;
}

INLINE void set_manufacturer() {}
INLINE void set_model_name() {}
INLINE void set_model_num() {}
INLINE void set_device_name() {}


UWORD8 au8Manufacturer[20];
INLINE UWORD8* get_manufacturer()
{
	UWORD8 temp[]= "NMIEDC";
	au8Manufacturer[0] = (NMI_Uint8)NMI_strlen(temp);
	//au8Manufacturer[1] = '\0';
	//strcat(au8Manufacturer,temp);
	NMI_memcpy(&au8Manufacturer[1],temp,NMI_strlen(temp));


	return au8Manufacturer;
}

UWORD8 au8ModelName[5] = {0x03,'P','2','P'};
INLINE UWORD8* get_model_name()
{
	return au8ModelName;
}
UWORD8 au8ModelNum[] = {0x02,'1','1'};
INLINE UWORD8* get_model_num()
{
	return au8ModelNum;
}
UWORD8 au8DeviceName[] = {0x09,'\0'};
INLINE UWORD8* get_device_name()
{
	NMI_Sint8 temp[] = "NMIDirect";
	NMI_memcpy(au8DeviceName+1,temp,NMI_strlen(temp));
	return au8DeviceName;
}
/* Host WLAN Identifier Structure                                            */
/* ID, Type, Response, Reset, Get Function Pointer,  Set Function Pointer    */
#if 1
host_wid_struct_t g_char_wid_struct[NUM_CHAR_WID]=
{
    {WID_BSS_TYPE,                            /*WID_CHAR,*/
    BTRUE,                                    BTRUE,
    (void *)get_DesiredBSSType,               (void *)set_DesiredBSSType},

    {WID_CURRENT_TX_RATE,                     /*WID_CHAR,*/
#ifdef MAC_HW_UNIT_TEST_MODE
    BTRUE,                                    BTRUE,
#else /* MAC_HW_UNIT_TEST_MODE */
    BTRUE,                                    BFALSE,
#endif /* MAC_HW_UNIT_TEST_MODE */
   (void *)get_tx_rate,                      (void *)set_tx_rate},

    {WID_CURRENT_CHANNEL,                     /*WID_CHAR,*/
    BTRUE,                                    BTRUE,
   (void *)get_host_chnl_num,                (void *)set_mac_chnl_num},

    {WID_PREAMBLE,                            /*WID_CHAR,*/
#ifndef MAC_HW_UNIT_TEST_MODE
    BTRUE,                                    BFALSE,
#else /* MAC_HW_UNIT_TEST_MODE */
    BTRUE,                                    BTRUE,
#endif /* MAC_HW_UNIT_TEST_MODE */
    (void *)get_preamble_type,                (void *)set_preamble_type},

    {WID_11G_OPERATING_MODE,                  /*WID_CHAR,*/
    BTRUE,                                    BTRUE,
    (void *)get_running_mode,                 (void *)set_running_mode},

    {WID_STATUS,                              /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    0,                                        0},
#if 0
    {WID_11G_PROT_MECH,                       /*WID_CHAR,*/
    BTRUE,                                   BTRUE,
    (void *)get_protection_mode,              (void *)set_protection_mode},
#endif

#ifdef MAC_HW_UNIT_TEST_MODE
    {WID_GOTO_SLEEP,                         /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    (void *)0,                               (void *)set_machw_sleep},
#else /* MAC_HW_UNIT_TEST_MODE */
    {WID_SCAN_TYPE,                           /*WID_CHAR,*/
    BFALSE,                                   BTRUE,
    (void *)mget_scan_type,                   (void *)mset_scan_type},
#endif /* MAC_HW_UNIT_TEST_MODE */
#if 0
    {WID_PRIVACY_INVOKED,                     /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    (void *)mget_PrivacyInvoked,              (void *)0},
#endif
    {WID_KEY_ID,                              /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    (void *)mget_WEPDefaultKeyID,             (void *)mset_WEPDefaultKeyID},

    {WID_QOS_ENABLE,                         /*WID_CHAR,*/
    BTRUE,                                    BTRUE,
    (void *)get_qos_enable,                   (void *)set_qos_enable},

    {WID_POWER_MANAGEMENT,                    /*WID_CHAR,*/
    BTRUE,                                   BTRUE,
    (void *)get_PowerManagementMode,         (void *)set_PowerManagementMode},
#if 0
    {WID_802_11I_MODE,                        /*WID_CHAR,*/
    BTRUE,                                    BTRUE,
    (void *)get_802_11I_mode,                 (void *)set_802_11I_mode},
#endif
    {WID_AUTH_TYPE,                           /*WID_CHAR,*/
    BTRUE,                                    BTRUE,
    (void *)get_auth_type,                    (void *)set_auth_type},

    {WID_SITE_SURVEY,                         /*WID_CHAR,*/
    BTRUE,                                    BFALSE,
    (void *)get_site_survey_status,           (void *)set_site_survey},

    {WID_LISTEN_INTERVAL,                     /*WID_CHAR,*/
    BTRUE,                                    BTRUE,
    (void *)mget_listen_interval,             (void *)mset_listen_interval},

    {WID_DTIM_PERIOD,                         /*WID_CHAR,*/
    BTRUE,                                    BTRUE,
    (void *)mget_DTIMPeriod,                  (void *)set_dtim_period},

    {WID_ACK_POLICY,                          /*WID_CHAR,*/
    BTRUE,                                    BTRUE,
    (void *)get_ack_type,                     (void *)set_ack_type},

    {WID_RESET,                              /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    (void *)0,                               (void *)set_reset_req},
#if 0
    {WID_PCF_MODE,                            /*WID_CHAR,*/
    BFALSE,                                   BTRUE,
    (void *)get_pcf_mode,                     (void *)set_pcf_mode},

    {WID_CFP_PERIOD,                          /*WID_CHAR,*/
    BFALSE,                                   BTRUE,
    (void *)mget_CFPPeriod,                   (void *)mset_CFPPeriod},

    {WID_BCAST_SSID,                          /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    (void *)get_bcst_ssid,                    (void *)set_bcst_ssid},
#endif
#ifdef MAC_HW_UNIT_TEST_MODE
    {WID_PHY_TEST_PATTERN,                    /*WID_CHAR,*/
    BFALSE,                                   BTRUE,
    (void *)get_phy_test_pattern,             (void *)set_phy_test_pattern},
#else /* MAC_HW_UNIT_TEST_MODE */
    {WID_DISCONNECT,                          /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)disconnect_station},
#endif /* MAC_HW_UNIT_TEST_MODE */
#if 0
    {WID_READ_ADDR_SDRAM ,                    /*WID_CHAR,*/
    BFALSE,                                   BTRUE,
    (void *)get_read_addr_sdram,              (void *)set_read_addr_sdram},
#endif
    {WID_TX_POWER_LEVEL_11A,                  /*WID_CHAR,*/
    BTRUE,                                   BFALSE,
    (void *)get_tx_power_level_11a,           (void *)set_tx_power_level_11a},

    {WID_REKEY_POLICY,                        /*WID_CHAR,*/
    BTRUE,                                    BTRUE,
    (void *)get_RSNAConfigGroupRekeyMethod,   (void *)set_RSNAConfigGroupRekeyMethod},

    {WID_SHORT_SLOT_ALLOWED,                  /*WID_CHAR,*/
    BTRUE,                                    BTRUE,
    (void *)get_short_slot_allowed,           (void *)set_short_slot_allowed},

    {WID_PHY_ACTIVE_REG,                      /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    (void *)get_phy_active_reg,               (void *)set_phy_active_reg},
#if 0
    {WID_PHY_ACTIVE_REG_VAL,                  /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    (void *)get_phy_active_reg_val,           (void *)set_phy_active_reg_val},
#endif
    {WID_TX_POWER_LEVEL_11B,                  /*WID_CHAR,*/
    BTRUE,                                   BFALSE,
    (void *)get_tx_power_level_11b,           (void *)set_tx_power_level_11b},

    {WID_START_SCAN_REQ,                      /*WID_CHAR,*/
    BTRUE,                                   BTRUE,
    (void *)get_start_scan_req,               (void *)set_start_scan_req},

    {WID_RSSI,                                /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    (void *)get_rssi,                         (void *)0},

    {WID_JOIN_REQ,                            /*WID_CHAR,*/
    BFALSE,                                   BTRUE,
    (void *)0,                                (void *)set_join_req},
#if 0
    {WID_ANTENNA_SELECTION,                   /*WID_CHAR,*/
    BTRUE,                                    BFALSE,
    (void *)get_antenna_selection,            (void *)set_antenna_selection},
#endif

    {WID_USER_CONTROL_ON_TX_POWER,            /*WID_CHAR,*/
    BTRUE,                                    BFALSE,
    (void *)get_user_control_enabled,         (void *)set_user_control_enabled},

    {WID_MEMORY_ACCESS_8BIT,                  /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    (void *)get_memory_access_8bit,           (void *)set_memory_access_8bit},
#if 0
    {WID_CURRENT_MAC_STATUS,                  /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    (void *)get_current_mac_status,           (void *)0},
#endif
    {WID_AUTO_RX_SENSITIVITY,                 /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    (void *)get_auto_rx_sensitivity,           (void *)set_auto_rx_sensitivity},

    {WID_DATAFLOW_CONTROL,                    /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    (void *)get_dataflow_control,             (void *)set_dataflow_control},

    {WID_SCAN_FILTER,                         /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    (void *)get_scan_filter,                  (void *)set_scan_filter},

    {WID_LINK_LOSS_THRESHOLD,                 /*WID_CHAR,*/
    BFALSE,                                   BFALSE,
    (void *)get_link_loss_threshold,          (void *)set_link_loss_threshold},
#if 0
    {WID_AUTORATE_TYPE,                       /*WID_CHAR,*/
    BFALSE,                                   BTRUE,
    (void *)get_autorate_type,                (void *)set_autorate_type},
#endif
    {WID_CCA_THRESHOLD,                       /*WID_CHAR,*/
    BFALSE,                                   BTRUE,
    (void *)get_cca_threshold,                (void *)set_cca_threshold},

#if 0
    {WID_UAPSD_SUPPORT_AP,                    /*WID_CHAR,*/
    BTRUE,                                    BTRUE,
    (void *)get_uapsd_support_ap,             (void *)set_uapsd_support_ap},

    /* dot11H parameters */
    {WID_802_11H_DFS_MODE,                    /*WID_CHAR,*/
    BTRUE,                                    BTRUE,
    (void *)get_802_11H_DFS_mode,             (void *)set_802_11H_DFS_mode},

    {WID_802_11H_TPC_MODE,                    /*WID_CHAR,*/
	BTRUE,                                    BTRUE,
    (void *)get_802_11H_TPC_mode,             (void *)set_802_11H_TPC_mode},
#endif
};
#endif 
const host_wid_struct_t g_short_wid_struct[NUM_SHORT_WID];
#if 0
=
{
    {WID_RTS_THRESHOLD,                       /*WID_SHORT,*/
    BTRUE,                                    BFALSE,
    (void *)mget_RTSThreshold,                (void *)set_RTSThreshold},

    {WID_FRAG_THRESHOLD,                      /*WID_SHORT,*/
    BTRUE,                                    BFALSE,
    (void *)mget_FragmentationThreshold,      (void *)set_FragmentationThreshold},

    {WID_SHORT_RETRY_LIMIT,                   /*WID_SHORT,*/
    BTRUE,                                    BFALSE,
    (void *)mget_ShortRetryLimit,             (void *)set_ShortRetryLimit},

    {WID_LONG_RETRY_LIMIT,                    /*WID_SHORT,*/
    BTRUE,                                    BFALSE,
    (void *)mget_LongRetryLimit,              (void *)set_LongRetryLimit},

    {WID_CFP_MAX_DUR,                         /*WID_SHORT,*/
    BFALSE,                                   BTRUE,
    (void *)mget_CFPMaxDuration,              (void *)mset_CFPMaxDuration},

    {WID_PHY_TEST_FRAME_LEN,                  /*WID_SHORT,*/
    BFALSE,                                   BTRUE,
    (void *)get_phy_test_frame_len,           (void *)set_phy_test_frame_len},

    {WID_BEACON_INTERVAL,                     /*WID_SHORT,*/
    BTRUE,                                    BTRUE,
    (void *)mget_BeaconPeriod,                (void *)mset_BeaconPeriod},



    {WID_MEMORY_ACCESS_16BIT,                 /*WID_SHORT,*/
    BFALSE,                                   BFALSE,
    (void *)get_memory_access_16bit,          (void *)set_memory_access_16bit},

    {WID_RX_SENSE,                            /*WID_SHORT,*/
    BFALSE,                                   BTRUE,
    (void *)get_rx_sense,                     (void *)set_rx_sense},

    {WID_ACTIVE_SCAN_TIME,                    /*WID_SHORT,*/
    BFALSE,                                   BFALSE,
    (void *)get_active_scan_time,             (void *)set_active_scan_time},

    {WID_PASSIVE_SCAN_TIME,                   /*WID_SHORT,*/
    BFALSE,                                   BFALSE,
    (void *)get_passive_scan_time,            (void *)set_passive_scan_time},

    {WID_SITE_SURVEY_SCAN_TIME,               /*WID_SHORT,*/
    BFALSE,                                   BFALSE,
    (void *)get_site_survey_scan_time,        (void *)set_site_survey_scan_time},

    {WID_JOIN_TIMEOUT,                        /*WID_SHORT,*/
    BFALSE,                                   BFALSE,
    (void *)get_join_timeout,                 (void *)set_join_timeout},

    {WID_AUTH_TIMEOUT,                        /*WID_SHORT,*/
    BFALSE,                                   BFALSE,
    (void *)get_auth_timeout,                 (void *)set_auth_timeout},

    {WID_ASOC_TIMEOUT,                        /*WID_SHORT,*/
    BFALSE,                                   BFALSE,
    (void *)get_asoc_timeout,                 (void *)set_asoc_timeout},

    {WID_11I_PROTOCOL_TIMEOUT,                /*WID_SHORT,*/
    BFALSE,                                   BFALSE,
    (void *)get_11i_protocol_timeout,         (void *)set_11i_protocol_timeout},

    {WID_EAPOL_RESPONSE_TIMEOUT,               /*WID_SHORT,*/
    BFALSE,                                   BFALSE,
    (void *)get_eapol_response_timeout,       (void *)set_eapol_response_timeout},
};
#endif
const host_wid_struct_t g_int_wid_struct[NUM_INT_WID];
#if 0
=
{

    {WID_FAILED_COUNT,                        /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)mget_FailedCount,                 (void *)set_FailedCount_to_zero},

    {WID_RETRY_COUNT,                         /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)mget_RetryCount,                  (void *)set_RetryCount_to_zero},

    {WID_MULTIPLE_RETRY_COUNT,                /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)mget_MultipleRetryCount,          (void *)set_MultipleRetryCount_to_zero},

    {WID_FRAME_DUPLICATE_COUNT,               /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)mget_FrameDuplicateCount,         (void *)set_FrameDuplicateCount_to_zero},

    {WID_ACK_FAILURE_COUNT,                   /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)mget_ACKFailureCount,             (void *)set_ACKFailureCount_to_zero},

    {WID_RECEIVED_FRAGMENT_COUNT,             /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)mget_ReceivedFragmentCount,       (void *)set_ReceivedFragmentCount_to_zero},

    {WID_MULTICAST_RECEIVED_FRAME_COUNT,      /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)mget_MulticastReceivedFrameCount, (void *)set_MulticastReceivedFrameCount_to_zero},

    {WID_FCS_ERROR_COUNT,                     /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)mget_FCSErrorCount,               (void *)set_FCSErrorCount_to_zero},

    {WID_SUCCESS_FRAME_COUNT,                 /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)mget_TransmittedFrameCount,       (void *)set_TransmittedFrameCount_to_zero},

    {WID_PHY_TEST_PKT_CNT,                    /*WID_INT,*/
    BFALSE,                                   BTRUE,
    (void *)get_phy_test_pkt_cnt,             (void *)set_phy_test_pkt_cnt},

    {WID_PHY_TEST_TXD_PKT_CNT,                /*WID_INT,*/

    BFALSE,                                   BTRUE,
    (void *)get_phy_test_txd_pkt_cnt,         (void *)set_phy_test_txd_pkt_cnt},

    {WID_TX_FRAGMENT_COUNT,                   /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)mget_TransmittedFragmentCount,    (void *)0},

    {WID_TX_MULTICAST_FRAME_COUNT,               /*WID_INT,*/
    BFALSE,                                      BFALSE,
    (void *)mget_MulticastTransmittedFrameCount, (void *)0},

    {WID_RTS_SUCCESS_COUNT,                   /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)mget_RTSSuccessCount,             (void *)0},

    {WID_RTS_FAILURE_COUNT,                   /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)mget_RTSFailureCount,             (void *)0},

    {WID_WEP_UNDECRYPTABLE_COUNT,             /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)mget_WEPICVErrorCount,            (void *)0},

    {WID_REKEY_PERIOD,                        /*WID_INT,*/
    BTRUE,                                    BTRUE,
    (void *)get_RSNAConfigGroupRekeyTime ,    (void *)set_RSNAConfigGroupRekeyTime},

    {WID_REKEY_PACKET_COUNT,                  /*WID_INT,*/
    BTRUE,                                    BTRUE,
    (void *)get_RSNAConfigGroupRekeyPackets,  (void *)set_RSNAConfigGroupRekeyPackets},

#ifdef MAC_HW_UNIT_TEST_MODE
    {WID_Q_ENABLE_INFO,                       /*WID_INT,*/
    BTRUE,                                    BTRUE,
    (void *)get_q_enable_info,               (void *)set_q_enable_info},
#else /* MAC_HW_UNIT_TEST_MODE */
    {WID_802_1X_SERV_ADDR,                    /*WID_INT,*/
    BTRUE,                                    BTRUE,
    (void *)get_1x_serv_addr,                 (void *)set_1x_serv_addr},
#endif /* MAC_HW_UNIT_TEST_MODE */

    {WID_STACK_IP_ADDR,                       /*WID_INT,*/
    BTRUE,                                    BFALSE,
    (void *)get_stack_ip_addr,                (void *)set_stack_ip_addr},

    {WID_STACK_NETMASK_ADDR,                  /*WID_INT,*/
    BTRUE,                                    BFALSE,
    (void *)get_stack_netmask_addr,           (void *)set_stack_netmask_addr},

    {WID_HW_RX_COUNT,                         /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)get_machw_rx_end_count,           (void *)0},

    {WID_RF_REG_VAL,                          /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)set_rf_reg_val},

    {WID_MEMORY_ADDRESS,                      /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)get_memory_address,               (void *)set_memory_address},


    {WID_MEMORY_ACCESS_32BIT,                 /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)get_memory_access_32bit,          (void *)set_memory_access_32bit},

    {WID_FIRMWARE_INFO,                      /*WID_INT,*/
    BFALSE,                                   BFALSE,
    (void *)get_firmware_info,               (void *)0},

};
#endif

const host_wid_struct_t g_str_wid_struct[NUM_STR_WID]=
{
    /*BugID_3736*/
	/*New WID Added to set a channel list*/
	/*the input message should have the length of channel list in the fist */
	/* byte and then the channels list*/
	/*setter function should copy given list to the g_host_scan_channel_list */
	/* setter function should also 'or' CHANNEL_LIST_SCAN with the */
	/* global variable g_scan_source in order to handle both cases exiting way*/
	/* of channel scanning and this new way*/
   {WID_SCAN_CHANNEL_LIST,                      /* WID_STR, */
     BFALSE,                                   BFALSE,
     (void *)get_scan_channel_list,               (void *)set_scan_channel_list},
	
   {WID_MAC_ADDR,                            /*WID_STR,*/
        BTRUE,                                    BTRUE,
        (void *)get_mac_addr,                     (void *)set_mac_addr},


    {WID_SSID,                                /*WID_STR,*/
    BTRUE,                                    BTRUE,
    (void *)get_DesiredSSID,                  (void *)mset_DesiredSSID},

    
	{WID_FIRMWARE_VERSION,                    /*WID_STR,*/
    BTRUE,                                    BFALSE,
#ifdef DEBUG_MODE
    (void *)get_manufacturerProductVersion,   (void *)set_test_case_name},
#else /* DEBUG_MODE */
    (void *)get_manufacturerProductVersion,   0},
#endif /* DEBUG_MODE */

#if 0
   {WID_OPERATIONAL_RATE_SET,                /*WID_STR,*/
    BTRUE,                                    BFALSE,
   (void *)get_supp_rates,                   0},
#endif
    {WID_BSSID,                               /*WID_STR,*/
    BTRUE,                                    BTRUE,
    (void *)get_bssid,                        (void *)set_preferred_bssid},
#if 0

    {WID_WEP_KEY_VALUE0,                      /*WID_STR,*/
    BTRUE,                                    BTRUE,
    (void *)get_WEP_key,                      (void *)set_WEP_key},

    {WID_WEP_KEY_VALUE1,                      /*WID_STR,*/
    BTRUE,                                    BFALSE,
    (void *)get_WEP_key,                      (void *)set_WEP_key},

    {WID_WEP_KEY_VALUE2,                      /*WID_STR,*/
    BTRUE,                                    BFALSE,
    (void *)get_WEP_key,                      (void *)set_WEP_key},

    {WID_WEP_KEY_VALUE3,                      /*WID_STR,*/
    BTRUE,                                    BFALSE,
    (void *)get_WEP_key,                      (void *)set_WEP_key},
#endif
    {WID_11I_PSK,                         /*WID_STR,*/
    BTRUE,                                    BTRUE,
    (void *)get_RSNAConfigPSKPassPhrase,     (void *)set_RSNAConfigPSKPassPhrase},
#if 0
    {WID_HCCA_ACTION_REQ,                     /*WID_STR,*/
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)set_action_request},

    {WID_802_1X_KEY,                          /*WID_STR,*/
    BTRUE,                                    BTRUE,
    (void *)get_1x_key,                       (void *)set_1x_key},

    {WID_HARDWARE_VERSION,                    /*WID_STR,*/
    BTRUE,                                    BFALSE,
    (void *)get_hardwareProductVersion,       0},
#endif
    {WID_MAC_ADDR,                            /*WID_STR,*/
    BTRUE,                                    BTRUE,
    (void *)get_mac_addr,                     (void *)set_mac_addr},
#if 0

    {WID_PHY_TEST_DEST_ADDR,                  /*WID_STR,*/
    BFALSE,                                   BTRUE,
    (void *)get_phy_test_dst_addr,            (void *)set_phy_test_dst_addr},

    {WID_PHY_TEST_STATS,                      /*WID_STR,*/
    BFALSE,                                   BFALSE,
    (void *)get_phy_test_stats,               (void *)set_phy_test_stats},

    {WID_PHY_VERSION,                         /*WID_STR,*/
    BTRUE,                                    BFALSE,
    (void *)get_phyProductVersion,            0},

    {WID_SUPP_USERNAME,                       /*WID_STR,*/
    BTRUE,                                    BTRUE,
    (void *)get_supp_username,                (void *)set_supp_username},

    {WID_SUPP_PASSWORD,                       /*WID_STR,*/
    BTRUE,                                    BTRUE,
    (void *)get_supp_password,                (void *)set_supp_password},
#endif
    {WID_SITE_SURVEY_RESULTS,                 /*WID_STR,*/ // TODO: mostafa: I'm not sure if WID_all flag is correct to be BTRUE,
														   //                and also not sure of reset_mac flag to be BFALSE
     BFALSE,                                   BFALSE,
    (void *)get_site_survey_results,          (void *)0},
    
    {WID_ADD_WEP_KEY,
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)set_wep_key},

    {WID_PMKID_INFO,                          /* WID_STR, */
    BFALSE,                                   BFALSE,
    (void *)get_pmkid_info,                   (void *)set_pmkid_info},
#if 0
    {WID_RX_POWER_LEVEL,                      /*WID_STR,*/
    BFALSE,                                   BFALSE,
    (void *)get_rx_power_level,               (void *)0},

#endif

#ifdef IBSS_BSS_STATION_MODE
  #if 0
    {WID_ADD_WEP_KEY,
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)add_wep_key_sta},

    {WID_REMOVE_WEP_KEY,
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)remove_wep_key_sta},

    {WID_ADD_PTK,
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)add_ptk_sta},

    {WID_ADD_RX_GTK,
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)add_rx_gtk_sta},

    {WID_ADD_TX_GTK,
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)add_tx_gtk},

    {WID_REMOVE_KEY,
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)remove_key_sta},

    {WID_ASSOC_REQ_INFO,
    BFALSE,                                   BFALSE,
    (void *)get_assoc_req_info,               (void *)0},

    {WID_ASSOC_RES_INFO,
    BFALSE,                                   BFALSE,
    (void *)get_assoc_res_info,               (void *)0},
   #endif
#else /* IBSS_BSS_STATION_MODE */
  #if 0
    {WID_REMOVE_WEP_KEY,
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)0},

    {WID_ADD_PTK,
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)0},

    {WID_ADD_RX_GTK,
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)0},

    {WID_ADD_TX_GTK,
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)0},

    {WID_REMOVE_KEY,
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)0},

    {WID_ASSOC_REQ_INFO,
    BFALSE,                                   BFALSE,
    (void *)0,                                (void *)0},
   #endif
   
    {WID_ASSOC_RES_INFO,
    BFALSE,                                   BFALSE,
    (void *)get_assoc_res_info,  (void *)0},
    
#endif /* IBSS_BSS_STATION_MODE */
#if 0
	{WID_UPDATE_RF_SUPPORTED_INFO,
	BFALSE,                                   BTRUE,
	(void *)get_rf_supported_info,            (void *)set_rf_supported_info},

    {WID_COUNTRY_IE,                          /*WID_BIN_DATA,*/
    BFALSE,                                   BFALSE,
    (void *)get_country_ie,                   (void *)set_country_ie},
#endif
	{WID_MANUFACTURER,						/*WID_STR,*/
	BFALSE,									BFALSE,
	(void*) get_manufacturer,				(void*)set_manufacturer},

	{WID_MODEL_NAME,						/*WID_STR,*/
	BFALSE,									BFALSE,
	(void*)get_model_name,					(void*) set_model_name},

	{WID_MODEL_NUM,							/*WID_STR,*/
	BFALSE,									BFALSE,
	(void*)get_model_num,					(void*)set_model_num},

	{WID_DEVICE_NAME,						/*WID_STR*/
	BFALSE,									BFALSE,
	(void*) get_device_name,				(void*) set_device_name},

	 /*BugID_4124*/
        /*New WID Added to trigger modified Join Request using SSID, BSSID and channel num */
        /*of the required AP instead of using bssListIdx (used by WID_JOIN_REQ)*/
        /*The input message should have in the first 32 bytes the SSID, then the following byte */
        /* should have the channel number and finally the following 6 bytes should have the BSSID*/
        /*setter function should copy the given desired AP parameters{SSID, Ch num and BSSID} to */
        /*the MIB database and to the desired AP global variables */
        /* No getter function implemented*/
        {WID_JOIN_REQ_EXTENDED,                      /* WID_STR, */
        BFALSE,                                   BTRUE,
        (void *)0,               (void *)set_join_req_ext}
};

const host_wid_struct_t g_binary_data_wid_struct[NUM_BIN_DATA_WID]
=
{
#if 0
    {WID_CONFIG_HCCA_ACTION_REQ,              /*WID_BIN_DATA,*/
     BFALSE,                                   BFALSE,
    (void *)0,                                (void *)set_config_HCCA_actionReq},
#endif


    {WID_UAPSD_CONFIG,                        /*WID_BIN_DATA,*/
    BFALSE,                                   BTRUE,
    (void *)get_uapsd_config,                 (void *)set_uapsd_config},
#if 0


    {WID_UAPSD_STATUS,                        /*WID_BIN_DATA,*/
    BFALSE,                                   BFALSE,
    (void *)get_uapsd_status,                 (void *)0},

    {WID_WMM_AP_AC_PARAMS,                    /*WID_BIN_DATA,*/
    BFALSE,                                   BFALSE,
    (void *)get_wmm_ap_ac_params,             (void *)set_wmm_ap_ac_params},

    {WID_WMM_STA_AC_PARAMS,                   /*WID_BIN_DATA,*/
    BFALSE,                                   BFALSE,
    (void *)get_wmm_sta_ac_params,            (void *)get_wmm_sta_ac_params},

    {WID_CONNECTED_STA_LIST,                   /*WID_BIN_DATA,*/
    BFALSE,                                   BFALSE,
    (void *)get_connected_sta_list,           (void *)0},

    {WID_HUT_STATS,                           /* WID_BIN_DATA, */
    BFALSE,                                   BFALSE,
    (void *)get_hut_stats,                   (void *)set_hut_stats},
#endif    
	    
	/* BugID_3746*/
    {WID_INFO_ELEMENT_PROBE,                      /* WID_BIN_DATA, */
    BFALSE,                                   BTRUE,
    (void *)get_info_element_probe,               (void *)set_info_element_probe},
	/* BugID_3746*/
    {WID_INFO_ELEMENT_ASSOCIATE,                      /* WID_BIN_DATA, */
    BFALSE,                                   BTRUE,
    (void *)get_info_element_associate,               (void *)set_info_element_associate},
};



NMI_Sint32 CoreConfigSimulatorInit(void)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;	
	tstrNMI_ThreadAttrs strThrdSimulatorAttrs;        
	tstrNMI_SemaphoreAttrs strSemaphoreAttrs;
	
	PRINT_INFO(CORECONFIG_DBG,"CoreConfigSimulatorInit() \n");

	
	g_current_mac_status = MAC_DISCONNECTED;	
	gbTermntMacStsThrd = NMI_FALSE;
	g_connected_bssIdx = 0xFFFF;	
	gScanInProgress = 0;
	gu32ReqSrvyResFrag = 0;

	FIFO_InitBuffer(&ghTxFifoBuffer, MAX_FIFO_BUFF_LEN);
	NMI_MsgQueueCreate(&gMsgQSimulator, NMI_NULL);

	NMI_SemaphoreFillDefault(&strSemaphoreAttrs);
	strSemaphoreAttrs.u32InitCount = 0;

	NMI_SemaphoreCreate(&SemHandleScanReq,&strSemaphoreAttrs);
	NMI_SemaphoreCreate(&SemHandleScanExit, &strSemaphoreAttrs);

	NMI_SemaphoreCreate(&SemHandleMacStsExit, &strSemaphoreAttrs);

	
	NMI_ThreadFillDefault(&strThrdSimulatorAttrs);	
	NMI_ThreadCreate(&ThrdHandleSimulator, SimulatorThread, NMI_NULL, &strThrdSimulatorAttrs);
	NMI_ThreadCreate(&ThrdHandleScan, SendScanResult, NMI_NULL, &strThrdSimulatorAttrs);
	NMI_ThreadCreate(&ThrdHandleMacStatus, SendMacStatus, NMI_NULL, &strThrdSimulatorAttrs);						
	return s32Error;
}

NMI_Sint32 SimConfigPktReceived(NMI_Sint8* pspacket, NMI_Sint32 s32PacketLen)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;
	NMI_Sint32 s32Err = NMI_SUCCESS;
		
	tstrSimulatorMsg* pstrSimulatorMsg = NMI_NULL;	
	
	pstrSimulatorMsg = (tstrSimulatorMsg*)NMI_MALLOC(sizeof(tstrSimulatorMsg));
	NMI_memset((void*)(pstrSimulatorMsg), 0, sizeof(tstrSimulatorMsg));
	
	s32Err = FIFO_WriteBytes(ghTxFifoBuffer, pspacket, s32PacketLen, NMI_TRUE);
	if(s32Err != NMI_SUCCESS)
	{
		NMI_ERROR("SimConfigPktReceived(): FIFO_WriteBytes() returned error \n");
		s32Error = NMI_FAIL;
		pstrSimulatorMsg->s32PktDataLen = -1;
		goto Send_Message;
	}
	
	pstrSimulatorMsg->s32PktDataLen = s32PacketLen;

Send_Message:
	NMI_MsgQueueSend(&gMsgQSimulator, pstrSimulatorMsg, sizeof(tstrSimulatorMsg), NMI_NULL);
	
	if(pstrSimulatorMsg != NMI_NULL)
	{
		NMI_FREE(pstrSimulatorMsg);
		pstrSimulatorMsg = NMI_NULL;
	}

	return s32Error;
}


/*****************************************************************************/
/*                                                                           */
/*  Function Name : get_wid_index                                            */
/*                                                                           */
/*  Description   : This function gets the WID index from WID ID & Type      */
/*                                                                           */
/*  Inputs        : 1) WID Identifier                                        */
/*                  2) WID Type                                              */
/*                                                                           */
/*  Globals       : 1) g_char_wid_struct                                     */
/*                  2) g_short_wid_struct                                    */
/*                  2) g_int_wid_struct                                      */
/*                  2) g_str_wid_struct                                      */
/*                  2) g_binary_data_wid_struct                              */
/*                                                                           */
/*  Processing    : This function searches for the WID ID in the relevent    */
/*                  global structure to find the index of the perticular WID */
/*                                                                           */
/*  Outputs       : None                                                     */
/*                                                                           */
/*  Returns       : Length of the query response                             */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         21 05 2005   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
UWORD8 get_wid_index(UWORD16 wid_id,UWORD8 wid_type)
{
    UWORD8 count;

    switch(wid_type)
    {
    case WID_CHAR:
        for(count = 0; count < NUM_CHAR_WID; count++)
        {
            if(g_char_wid_struct[count].id == wid_id)
                return count;
        }
        break;
    case WID_SHORT:
        for(count = 0; count < NUM_SHORT_WID; count++)
        {
            if(g_short_wid_struct[count].id == wid_id)
                return count;
        }
        break;
    case WID_INT:
        for(count = 0; count < NUM_INT_WID; count++)
        {
            if(g_int_wid_struct[count].id == wid_id)
                return count;
        }
        break;
    case WID_STR:
        for(count = 0; count < NUM_STR_WID; count++)
        {
            if(g_str_wid_struct[count].id == wid_id)
                return count;
        }
        break;
    case WID_BIN_DATA:
        for(count = 0; count < NUM_BIN_DATA_WID; count++)
        {
            if(g_binary_data_wid_struct[count].id == wid_id)
                return count;
        }
        break;
    }

    return 0xFF;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : get_wid                                                  */
/*                                                                           */
/*  Description   : This function gets the WID value from the global WID     */
/*                  array and updates the response message contents.         */
/*                                                                           */
/*  Inputs        : 1) Pointer to the WID field                              */
/*                  2) Index of the global WID array                         */
/*                  3) The WID type                                          */
/*                                                                           */
/*  Globals       : g_wid_struct                                             */
/*                                                                           */
/*  Processing    : This function sets the WID ID, Length and Value in the   */
/*                  the incoming packet.                                     */
/*                                                                           */
/*  Outputs       : None                                                     */
/*                                                                           */
/*  Returns       : Length of WID response                                   */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         21 05 2005   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/

UWORD16 get_wid(UWORD8 *wid, UWORD8 count, UWORD8 wid_type)
{
    UWORD8  wid_len  = 0;
    UWORD16 wid_id   = 0;
    UWORD8  *wid_val = 0;
    /* NULL response is corresponded */
    BOOL_T  null_rsp = BFALSE;

    /* The WID is set in the following format:                               */
    /* +-------------------------------------------------------------------+ */
    /* | WID Type  | WID Length | WID Value                                | */
    /* +-------------------------------------------------------------------+ */
    /* | 2 Bytes   | 1 Byte     | WID length                               | */
    /* +-------------------------------------------------------------------+ */

    /* Set the WID value with the value returned by the get function         */
    /* associated with the particular WID type.                              */
    wid_val = (wid + WID_VALUE_OFFSET);

    /* If the member function is not NULL, get the value, else discard read */
    switch(wid_type)
    {
    case WID_CHAR:
        {
            wid_id   = g_char_wid_struct[count].id;
            if(g_char_wid_struct[count].get)
            {
                UWORD8 val =
                    ((UWORD8 (*)(void))g_char_wid_struct[count].get)();

                wid_len = 1;

                wid_val[0] = val;
            }
            else
            {
                wid_len = 0;
            }
        }
        break;

    case WID_SHORT:
        {
            wid_id   = g_short_wid_struct[count].id;
            if(g_short_wid_struct[count].get)
            {
                UWORD16 val =
                    ((UWORD16 (*)(void))g_short_wid_struct[count].get)();

                wid_len = 2;

                wid_val[0] = (UWORD8)(val & 0x00FF);
                wid_val[1] = (UWORD8)((val >> 8) & 0x00FF);
            }
            else
            {
                wid_len = 0;
            }
        }
        break;

    case WID_INT:
        {
            wid_id   = g_int_wid_struct[count].id;
            if(g_int_wid_struct[count].get)
            {
                UWORD32 val =
                    ((UWORD32 (*)(void))g_int_wid_struct[count].get)();

                wid_len = 4;

                wid_val[0] = (UWORD8)(val & 0x000000FF);
                wid_val[1] = (UWORD8)((val >> 8) & 0x000000FF);
                wid_val[2] = (UWORD8)((val >> 16) & 0x000000FF);
                wid_val[3] = (UWORD8)((val >> 24) & 0x000000FF);
            }
            else
            {
                wid_len = 0;
            }
        }
        break;

    case WID_STR:
        {
            wid_id   = g_str_wid_struct[count].id;
            if(g_str_wid_struct[count].get)
            {
                UWORD8 *val =
                    ((UWORD8 *(*)(void))g_str_wid_struct[count].get)();

                if(val != NULL)
                {
                    wid_len = val[0];

                    NMI_memcpy(wid_val, (val + 1), wid_len);
                }
                else
                {
                    /* NULL response is corresponded */
                    null_rsp = BTRUE;
                    wid_len = 0;
                }
            }
            else
            {
                wid_len = 0;
            }
        }
        break;

    case WID_BIN_DATA:
        {
            /* WID_BIN_DATA is Set in the following format */
            /* +---------------------------------------------------+ */
            /* | WID    | WID Length | WID Value        | Checksum | */
            /* +---------------------------------------------------+ */
            /* | 2Bytes | 2Bytes     | WID Length Bytes | 1Byte    | */
            /* +---------------------------------------------------+ */

            wid_id  = g_binary_data_wid_struct[count].id;
            if(g_binary_data_wid_struct[count].get)
            {
                UWORD16 length      = 0;
                UWORD8  checksum    = 0;
                UWORD16 i           = 0;
                UWORD8  *val        =
                    ((UWORD8 *(*)(void))g_binary_data_wid_struct[count].get)();

                if(val == NULL)
                {
                    return 0;
                }

                /* First 2 Bytes are length field and rest of the bytes are */
                /* actual data                                              */

                /* Length field encoding */
                /* +----------------------------------+ */
                /* | BIT15  | BIT14  | BIT13 - BIT0   | */
                /* +----------------------------------+ */
                /* | First  | Last   | Message Length | */
                /* +----------------------------------+ */
                length  = (val[0] & 0x00FF);
                length |= ((UWORD16) val[1] << 8) & 0xFF00;
                length &= WID_BIN_DATA_LEN_MASK;

                if(length != 0 )
                {
                    /* Set the WID type. */
                    wid[0] = wid_id & 0xFF;
                    wid[1] = (wid_id & 0xFF00) >> 8;

                    /* Copy the data including the length field */
                    NMI_memcpy((wid + WID_LENGTH_OFFSET), val, (length + 2));

                    /* Compute Checksum for the data field & append it at    */
                    /* the end of the packet                                 */
                    for(i = 0;i < length;i++)
                    {
                        checksum += val[2 + i];
                    }
                    *(wid + WID_LENGTH_OFFSET + 2 + length) = checksum;

                    /* Return the WID response length */
                    /*WID_TYPE        WID_Length  WID_Value  Checksum*/
                    return (WID_LENGTH_OFFSET + 2         + length   + 1);
                }
                else
                {
                    return 0;
                }
            }
        }
        break;
    } /* end of switch(wid_len) */

    /* NULL response is corresponded */
    if((wid_len != 0) || (BTRUE == null_rsp))
    {
        /* Set the WID type. The WID length field is set at the end after        */
        /* getting the value of the WID. In case of variable length WIDs the     */
        /* length s known only after the value is obtained.                      */
        wid[0] = wid_id & 0xFF;
        wid[1] = (wid_id & 0xFF00) >> 8;

        /* Set the length of the WID */
        wid[2] = wid_len;

        /* Return the WID response length */
        return (wid_len + WID_VALUE_OFFSET);
    }
    else
    {
        return (0);
    }
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : set_wid                                                  */
/*                                                                           */
/*  Description   : This function sets the WID value with the incoming WID   */
/*                  value.                                                   */
/*                                                                           */
/*  Inputs        : 1) Pointer to the WID value                              */
/*                  2) Length of the WID value                               */
/*                  3) Index of the global WID array                         */
/*                  4) The WID type                                          */
/*                                                                           */
/*  Globals       : g_wid_struct                                             */
/*                                                                           */
/*  Processing    : This function sets the WID Value with the incoming value.*/
/*                                                                           */
/*  Outputs       : The corresponding MAC parameter is set                   */
/*                                                                           */
/*  Returns       : None                                                     */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         21 05 2005   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/

void set_wid(UWORD8 *wid, UWORD16 len, UWORD8 count, UWORD8 wid_type)
{
    switch(wid_type)
    {
    case WID_CHAR:
        {
            if(g_char_wid_struct[count].set)
            {
                ((void (*)(UWORD8))g_char_wid_struct[count].set)(wid[0]);
            }
            else
            {
                /* WID cannot be set. Do nothing. */
            }
        }
        break;

    case WID_SHORT:
        {
            UWORD16 val = 0;

            val = wid[1];
            val = (val << 8) | wid[0];

            if(g_short_wid_struct[count].set)
            {
                ((void (*)(UWORD16))g_short_wid_struct[count].set)(val);
            }
            else
            {
                /* WID cannot be set. Do nothing. */
            }
        }
        break;

    case WID_INT:
        {
            UWORD32 val = 0;

            val  = wid[3];
            val  = (val << 8) | wid[2];
            val  = (val << 8) | wid[1];
            val  = (val << 8) | wid[0];

            if(g_int_wid_struct[count].set)
            {
                ((void (*)(UWORD32))g_int_wid_struct[count].set)(val);
            }
            else
            {
                /* WID cannot be set. Do nothing. */
            }
        }
        break;

    case WID_STR:
        {
            UWORD8 val[MAX_STRING_LEN];

            NMI_memcpy(val, wid, len);
            val[len] = '\0';

            if(g_str_wid_struct[count].set)
            {
                ((void (*)(UWORD8*))g_str_wid_struct[count].set)(val);
            }
            else
            {
                /* WID cannot be set. Do nothing. */
            }
        }
        break;

    case WID_BIN_DATA:
        {
            if(g_binary_data_wid_struct[count].set)
            {
                ((void (*)(UWORD8*))g_binary_data_wid_struct[count].set)(wid);
            }
            else
            {
                /* WID cannot be set. Do nothing. */
            }
        }
        break;

    } /* end of switch(wid_type) */
}
/* This function copies the MAC address from a source to destination */
/*
INLINE void mac_addr_cpy(UWORD8 *dst, UWORD8 *src)
{
    UWORD8 i = 0;

    for(i = 0; i < 6; i++)
        dst[i] = src[i];
}
*/

/* This function copies the IP address from a source to destination */
/*
INLINE void ip_addr_cpy(UWORD8 *dst, UWORD8 *src)
{
    UWORD8 i = 0;

    for(i = 0; i < 4; i++)
        dst[i] = src[i];
}
*/

/*****************************************************************************/
/*                                                                           */
/*  Function Name : checksum                                                 */
/*                                                                           */
/*  Description   : This function computes the 16-bit CRC.                   */
/*                                                                           */
/*  Inputs        : 1) Pointer to the buffer                                 */
/*                  2) Length of the buffer                                  */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : This function computes 16-bit CRC for the given packet.  */
/*                                                                           */
/*  Outputs       : None                                                     */
/*                                                                           */
/*  Returns       : 16-bit CRC checksum                                      */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         21 05 2005   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/

UWORD16 checksum(UWORD8 *buffer, UWORD32 size)
{
    UWORD32 cksum = 0;

    while (size > 1)
    {
        cksum += *buffer + (*(buffer+1)  << 8);
        buffer += 2 ;
        size -= 2;
    }

    if (size)
    {
        cksum += *buffer;
    }

    cksum = (cksum >> 16) + (cksum & 0xFFFF);
    cksum += (cksum >> 16);

    return (UWORD16)(~cksum);
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : process_query                                            */
/*                                                                           */
/*  Description   : This function processes the host query message.          */
/*                                                                           */
/*  Inputs        : 1) Pointer to the query request message                  */
/*                  2) Pointer to the query response message                 */
/*                  3) Length of the query request                           */
/*                                                                           */
/*  Globals       : g_wid_struct                                             */
/*                                                                           */
/*  Processing    : This function reads the WID value in the Query message   */
/*                  and gets the corresponding WID length and value. It sets */
/*                  the WID, WID Length and value in the response message.   */
/*                  The process is repeated for all the WID types requested  */
/*                                                                           */
/*  Outputs       : None                                                     */
/*                                                                           */
/*  Returns       : Length of the query response                             */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         21 05 2005   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/

UWORD16 process_query(UWORD8* wid_req, UWORD8* wid_rsp, UWORD16 req_len)
{
    UWORD8  count   = 0;
    UWORD16 wid_id  = 0;
    UWORD16 req_idx = 0;
    UWORD16 rsp_idx = 0;

    /* The format of a message body of a message type 'Q' is:                */
    /* +-------------------------------------------------------------------+ */
    /* | WID0          | WID1         | WID2         | ................... | */
    /* +-------------------------------------------------------------------+ */
    /* |     2 Bytes   |    2 Bytes   |    2 Bytes   | ................... | */
    /* +-------------------------------------------------------------------+ */

    /* The format of a message body of a message type 'R' is:                */
    /* +-------------------------------------------------------------------+ */
    /* | WID0      | WID0 Length | WID0 Value  | ......................... | */
    /* +-------------------------------------------------------------------+ */
    /* | 2 Bytes   | 1 Byte      | WID Length  | ......................... | */
    /* +-------------------------------------------------------------------+ */

    /* The processing of a Query message consists of the following steps:    */
    /* 1) Read the WID value in the Query message                            */
    /* 2) Get the corresponding WID length and value                         */
    /* 3) Set the WID, WID Length and value in the response message          */
    /* 4) Repeat for all the WID types requested by the host                 */
    /* 5) Return the length of the response message                          */

#ifdef CONFIG_PRINT
    if(g_reset_mac_in_progress == BFALSE)
        debug_print("\nWID Q: ");
#endif /* CONFIG_PRINT */

    while(req_idx < req_len)
    {
        /* Read the WID type (2 Bytes) from the Query message and increment  */
        /* the Request Index by 2 to point to the next WID.                  */
        wid_id = wid_req[req_idx + 1];
        wid_id = (wid_id << 8) | wid_req[req_idx];
        req_idx += 2;

#ifdef CONFIG_PRINT
        if(g_reset_mac_in_progress == BFALSE)
              debug_print("%x ",wid_id);
#endif /* CONFIG_PRINT */

        if(wid_id == WID_ALL)
        {
            /* If the WID type is WID_ALL all WID values need to be returned */
            /* to the host. Thus all the array elements of the global WID    */
            /* array are accessed one by one.                                */
            for(count = 0; count < NUM_CHAR_WID; count++)
            {
                if(g_char_wid_struct[count].rsp == BTRUE)
                    rsp_idx += get_wid(&wid_rsp[rsp_idx], count, WID_CHAR);
            }

            for(count = 0; count < NUM_SHORT_WID; count++)
            {
                if(g_short_wid_struct[count].rsp == BTRUE)
                    rsp_idx += get_wid(&wid_rsp[rsp_idx], count, WID_SHORT);
            }

            for(count = 0; count < NUM_INT_WID; count++)
            {
                if(g_int_wid_struct[count].rsp == BTRUE)
                    rsp_idx += get_wid(&wid_rsp[rsp_idx], count, WID_INT);
            }

            for(count = 0; count < NUM_STR_WID; count++)
            {
                if(g_str_wid_struct[count].rsp == BTRUE)
                    rsp_idx += get_wid(&wid_rsp[rsp_idx], count, WID_STR);
            }
            for(count = 0; count < NUM_BIN_DATA_WID; count++)
            {
                if(g_binary_data_wid_struct[count].rsp == BTRUE)
                    rsp_idx += get_wid(&wid_rsp[rsp_idx], count, WID_BIN_DATA);
            }

        }
        else
        {
            /* In case of any other WID, the static global WID array is      */
            /* searched to find a matching WID type for its corresponding    */
            /* length and value.                                             */
            UWORD8 wid_type = (wid_id & 0xF000) >> 12;
            count           = get_wid_index(wid_id,wid_type);

            if(count == 0xFF)
            {
                /* If the Queried WID type is not found it is an exception.  */
                /* Skip this and continue.                                   */
                continue;
            }
            else
            {
                /* In case of a valid WID, fill up the WID type, WID length  */
                /* and WID values in the Response message.                   */
                rsp_idx += get_wid(&wid_rsp[rsp_idx], count, wid_type);
            }
        }
    } /* end of while(index < req_len) */

#ifdef CONFIG_PRINT
    if(g_reset_mac_in_progress == BFALSE)
        debug_print("||\n");
#endif /* CONFIG_PRINT */

    /* Return Response length (given by the updated response index value)    */
    return rsp_idx;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : process_write                                            */
/*                                                                           */
/*  Description   : This function processes the host write message.          */
/*                                                                           */
/*  Inputs        : 1) Pointer to the write request message                  */
/*                  2) Length of the write request                           */
/*                                                                           */
/*  Globals       : g_wid_struct                                             */
/*                  g_reset_mac                                              */
/*                                                                           */
/*  Processing    : This function reads the WID value in the Write message   */
/*                  and sets the corresponding WID length and value. The     */
/*                  process is repeated for all the WID types requested by   */
/*                  the host. In case the setting of any WID requires the    */
/*                  reset of MAC, the global g_reset_mac is updated.         */
/*                                                                           */
/*  Outputs       : g_reset_mac is updated to indicate if a reset is needed  */
/*                                                                           */
/*  Returns       : None                                                     */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         21 05 2005   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/

void process_write(UWORD8* wid_req, UWORD16 req_len)
{
    UWORD8  count    = 0;
    UWORD16 req_idx  = 0;
    UWORD16 wid_len  = 0;
    UWORD16 wid_id   = 0;
    UWORD8  *wid_val = 0;
    UWORD32  wid_type = 0;

    /* Set the global flag for reset MAC to BFALSE. This will be updated to  */
    /* BTRUE if any parameter setting calls for a reset.                     */
    g_reset_mac = BFALSE;

    /* The format of a message body of a message type 'W' is:                */
    /* +-------------------------------------------------------------------+ */
    /* | WID0      | WID0 Length | WID0 Value  | ......................... | */
    /* +-------------------------------------------------------------------+ */
    /* | 2 Bytes   | 1 Byte      | WID Length  | ......................... | */
    /* +-------------------------------------------------------------------+ */

    /* The processing of a Write message consists of the following steps:    */
    /* 1) Read the WID value in the Write message                            */
    /* 2) Set the corresponding WID length and value                         */
    /* 3) Repeat for all the WID types requested by the host                 */

#ifdef CONFIG_PRINT
    if(g_reset_mac_in_progress == BFALSE)
        debug_print("\nWID S: ");
#endif /* CONFIG_PRINT */

    while(req_idx+3 < req_len)
    {

        /* Read the WID ID (2 Bytes) and the WID length from the Write       */
        /* message.                                                          */
        wid_id   = wid_req[req_idx + 1];
        wid_id   = (wid_id << 8) | wid_req[req_idx];
        wid_type = (wid_id & 0xF000) >> 12;
        count    = get_wid_index(wid_id,wid_type);

	 //NMI_PRINTF("  wid_id = %02x , wid_type = %d \n", wid_id, wid_type);
	
#ifdef CONFIG_PRINT
        if(g_reset_mac_in_progress == BFALSE)
            debug_print("%x %x",wid_id,count);
#endif /* CONFIG_PRINT */

        if(wid_type != WID_BIN_DATA)
        {
            wid_len = wid_req[req_idx + 2];

            /* Set the 'wid' pointer to point to the WID value (skipping */
            /* the ID and Length fields). This is used in the set_wid    */
            /* function. It is assumed that all 'set' functions use the  */
            /* value directly as input.                                  */
            wid_val = &wid_req[req_idx + WID_VALUE_OFFSET];

            /* The Request Index is incremented by (WID Length + 3) to   */
            /* point to the next WID.                                    */
            req_idx += (wid_len + WID_VALUE_OFFSET);
        }
        else
        {
            UWORD16 i        = 0;
            UWORD8  checksum = 0;

            /* WID_BIN_DATA format */
            /* +---------------------------------------------------+ */
            /* | WID    | WID Length | WID Value        | Checksum | */
            /* +---------------------------------------------------+ */
            /* | 2Bytes | 2Bytes     | WID Length Bytes | 1Byte    | */
            /* +---------------------------------------------------+ */

            wid_len  = wid_req[req_idx + 2];
            wid_len |= ((UWORD16)wid_req[req_idx + 3] << 8) & 0xFF00 ;
            wid_len &= WID_BIN_DATA_LEN_MASK;

            /* Set the 'wid' pointer to point to the WID length (skipping */
            /* the ID field). This is used in the set_wid function. It    */
            /* is assumed that 'set' function for Binary data uses the    */
            /* length field for getting the WID Value                     */
            wid_val  = &wid_req[req_idx + WID_LENGTH_OFFSET];

            /* The Request Index is incremented by (WID Length + 5) to   */
            /* point to the next WID.                                    */
            req_idx += (wid_len + WID_LENGTH_OFFSET + 2 + 1);

            /* Compute checksum on the Data field */
            for(i = 0;i < wid_len;i++)
            {
                checksum += wid_val[i + 2];
            }
            /* Drop the packet, if checksum failed */
            if(checksum != wid_val[wid_len + 2])
            {
                continue;
            }
        }

        if(count == 0xFF)
        {
            /* If the Queried WID type is not found it is an exception.  */
            /* Skip this and continue.                                   */
            continue;
        }
        else
        {
            /* In case of a valid WID, set the corresponding parameter with  */
            /* the incoming value.                                           */
            set_wid(wid_val, wid_len, count, wid_type);

            /* Check if writing this WID requires resetting of MAC. The      */
            /* global g_reset_mac is 'OR'ed with the reset value obtained    */
            /* for this WID.                                                 */
            switch(wid_type)
            {
            case WID_CHAR:
                g_reset_mac |= g_char_wid_struct[count].reset;
                break;
            case WID_SHORT:
                g_reset_mac |= g_short_wid_struct[count].reset;
                break;
            case WID_INT:
                g_reset_mac |= g_int_wid_struct[count].reset;
                break;
            case WID_STR:
                g_reset_mac |= g_str_wid_struct[count].reset;
                break;
            case WID_BIN_DATA:
                g_reset_mac |= g_binary_data_wid_struct[count].reset;
                break;
            }
        }
    } /* end of while(index < req_len) */

#ifdef CONFIG_PRINT
    if(g_reset_mac_in_progress == BFALSE)
        debug_print("||\n");
#endif /* CONFIG_PRINT */

    /* Overwrite the Reset Flag with User request  */
    if(g_reset_req_from_user == DONT_RESET)
    {
        g_reset_mac = BFALSE;
    }
    else if (g_reset_req_from_user == DO_RESET)
    {
        g_reset_mac = BTRUE;
    }

    /* Re-initialize user request variable */
    g_reset_req_from_user = NO_REQUEST;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : prepare_config_hdr                                       */
/*                                                                           */
/*  Description   : This function prepares a configuration header as per the */
/*                  host protocol in use.                                    */
/*                                                                           */
/*  Inputs        : 1) Pointer to the configuration message buffer           */
/*                  2) More fragment BOOL value                              */
/*                  3) Fragment offset                                       */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : For the ethernet host, a UDP packet with the given source*/
/*                  and destination port is by protocol a configuration      */
/*                  message. Hence the host response is packed into a UDP    */
/*                  packet. For ther hosts, sepaerate host header is defined */
/*                                                                           */
/*  Outputs       : None                                                     */
/*                                                                           */
/*  Returns       : Length of the configuration message                      */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */

/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         20 01 2009   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
UWORD8   g_first_config_pkt      = 1;
UWORD8   g_config_eth_addr[6]    = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
UWORD8   g_config_ip_addr[6]     = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
UWORD8   g_src_eth_addr[6]       = { 0x00, 0x50, 0xC2, 0x5E, 0x10, 0x92 };
UWORD8   g_src_ip_addr[6]        = {192,168,20,221};
UWORD8   g_src_netmask_addr[6]   = {0};


UWORD16 prepare_config_hdr(UWORD8* data, UWORD16 len,
                           BOOL_T more_frag, UWORD16 frag_offset)
{
    UWORD16 ret_len = 0;
#ifdef ETHERNET_HOST
    UWORD8  index   = 0;
    UWORD16 csum    = 0;
    UWORD16 udp_len = 0;
    UWORD16 ip_len  = 0;
#endif /* ETHERNET_HOST */

#ifdef ETHERNET_HOST
    /* Prepare the Ethernet Header - 14 bytes (format is shown below)        */
    /*                                                                       */
    /* +---------------------+-------------------+-------------+             */
    /* | Destination address | Source address    | Packet Type |             */
    /* +---------------------+-------------------+-------------+             */
    /* |  0                  | 6                 |12           |             */
    /* +---------------------+-------------------+-------------+             */

    /* Set the Ethernet destination address and Source address */
    mac_addr_cpy(data, g_config_eth_addr);
    mac_addr_cpy(data + 6, g_src_eth_addr);

    /* Set the type to IP */
    data[12] = (IP_TYPE & 0xFF00) >> 8;
    data[13] = (IP_TYPE & 0xFF);

    /* Prepare the IP Header - 20 bytes (format is shown below)              */
    /*                                                                       */
    /* --------------------------------------------------------------------- */
    /* | Version | IHL | TOS | Total length | Identification | Flags         */
    /* --------------------------------------------------------------------- */
    /*  0               1      2              4                6             */
    /* --------------------------------------------------------------------- */
    /* | Fragment offset | TTL | Protocol   | Header checksum |              */
    /* --------------------------------------------------------------------- */
    /*  7                  8     9           10                              */
    /* --------------------------------------------------------------------- */
    /* | Source IP address            | Destination IP address             | */
    /* --------------------------------------------------------------------- */
    /*  12                              16                                   */
    /* --------------------------------------------------------------------- */

    index = IP_HDR_OFFSET;

    if(frag_offset == 0)
    {
        ip_len = len + UDP_HDR_LEN + IP_HDR_LEN;
    }
    else
    {
        ip_len = len + IP_HDR_LEN;
    }

    data[index]     = 0x45;
    data[index + 1] = 0x00;
    data[index + 2] = (ip_len & 0xFF00) >> 8;
    data[index + 3] = (ip_len & 0xFF);
    data[index + 4] = 0x00;
    data[index + 5] = 0x00;

    /* Flags & Fragment Offset */
    if(more_frag == BTRUE)
    {
        data[index + 6] = 0x20;
    }
    else
    {
        data[index + 6] = 0x00;
    }

    data[index + 7] = frag_offset;

    /* TTL */
    data[index + 8] = 0x80;

    /* Set the IP protocol to UDP */
    data[index + 9] = UDP_TYPE;

    /* Set the checksum to zero */
    data[index + 10] = 0;
    data[index + 11] = 0;

    index += 12;

    /* Set the IP source address to some value with different subnet */
    ip_addr_cpy(data + index, g_src_ip_addr);
    index += 4;

    /* Set the IP destination address */
    ip_addr_cpy(data + index, g_config_ip_addr);
    index += 4;

    /* Compute the checksum and set the field */
    csum = checksum(&data[IP_HDR_OFFSET], IP_HDR_LEN);
    data[IP_HDR_OFFSET + 11] = (csum & 0xFF00) >> 8;
    data[IP_HDR_OFFSET + 10] = (csum & 0xFF);

    if((frag_offset == 0) && (more_frag == BFALSE))
    {
        /* Prepare the UDP Header - 8 bytes (format is shown below) */
        /*                                                          */
        /* +--------------+--------------+--------+----------+      */
        /* | UDP SRC Port | UDP DST Port | Length | Checksum |      */
        /* +--------------+--------------+--------+----------+      */
        /* |  0           | 2            |4       | 6        |      */
        /* +--------------+--------------+--------+----------+      */

        index = UDP_HDR_OFFSET;

        /* Set the UDP source and destination ports */
        data[index]     = (WLAN_UDP_PORT & 0xFF00) >> 8;
        data[index + 1] = (WLAN_UDP_PORT & 0xFF);
        data[index + 2] = (HOST_UDP_PORT & 0xFF00) >> 8;
        data[index + 3] = (HOST_UDP_PORT & 0xFF);
        index += 4;

        /* Set the length to the UDP data + UDP header */
        udp_len         = len + UDP_HDR_LEN;
        data[index]     = (udp_len & 0xFF00) >> 8;
        data[index + 1] = (udp_len & 0xFF);
        index += 2;

        /* Set the checksum to zero - currently not set as it is not mandatory */
        data[index] = 0;
        data[index + 1] = 0;

        ret_len = (len + CONFIG_PKT_HDR_LEN);
    }
    else
    {
        ret_len = (len + CONFIG_PKT_HDR_LEN - UDP_HDR_OFFSET);
    }
#endif /* ETHERNET_HOST */




    return ret_len;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : send_mac_status                                          */
/*                                                                           */
/*  Description   : This function sends the status of MAC to host            */
/*                                                                           */
/*  Inputs        : MAC Status                                               */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : This function prepares an Information frame having the   */
/*                  MAC Status and calls send_host_rsp function to send the  */
/*                  prepared frame to host                                   */
/*                                                                           */
/*  Outputs       : None                                                     */
/*  Returns       : None                                                     */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         11 04 2008   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
void send_mac_status(UWORD8 mac_status)
{
//#ifdef SEND_MAC_STATUS    
    UWORD8       *info_msg    = NULL;
    UWORD16 offset = config_host_hdr_offset(BFALSE,0);


#ifdef HOST_RX_LOCAL_MEM
	   if(g_eth2sram_trfr == BTRUE)
	       mem_handle = (void *) g_shared_mem_handle;
	   else
	       mem_handle = (void *) g_local_mem_handle;
#endif /* HOST_RX_LOCAL_MEM */

	    info_msg = (UWORD8*)NMI_MALLOC(MAX_BUFFER_LEN);
	    if(info_msg == NULL)
	    {
	        return;
	    }

	    info_msg[offset + 0] = 'I';
	    info_msg[offset + 1] = g_info_id;
	    /* Increment the id sequence counter by 1*/
	    g_info_id++;

	    info_msg[offset + 2] = (STATUS_MSG_LEN) & 0xFF;
	    info_msg[offset + 3] = ((STATUS_MSG_LEN) & 0xFF00) >> 8;

	    /* Set the WID_STATUS, Length to 1, Value to mac_status */
	    info_msg[offset + 4] = WID_STATUS & 0xFF;
	    info_msg[offset + 5] = (WID_STATUS & 0xFF00) >> 8;
	    info_msg[offset + 6] = 1;
	    info_msg[offset + 7] = mac_status;

	    /* The response is sent back to the host */
	    send_host_rsp(info_msg, STATUS_MSG_LEN);       
		
		if(info_msg != NULL)
		{
			NMI_FREE(info_msg);
		}    
//#endif /* SEND_MAC_STATUS */
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : send_wake_status                                         */
/*                                                                           */
/*  Description   : This function sends the status of PS condition to host   */
/*                                                                           */
/*  Inputs        : PS Status                                                */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : This function prepares an Information frame having the   */
/*                  PS Status and calls send_host_rsp function to send the   */
/*                  prepared frame to host                                   */
/*                                                                           */
/*  Outputs       : None                                                     */
/*  Returns       : None                                                     */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         30 09 2008   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
void send_wake_status(UWORD8 wake_status)
{
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : send_network_info_to_host                                */
/*                                                                           */
/*  Description   : This function sends the beacon/probe response from scan  */
/*                                                                           */
/*  Inputs        : BSSID, RSSI & InfoElements                               */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : This function prepares an Information frame having the   */
/*                  BSSID. RSSI value and Information elements and sends the */
/*                  prepared frame to host                                   */
/*                                                                           */
/*  Outputs       : None                                                     */
/*  Returns       : None                                                     */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         17 08 2009   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
void send_network_info_to_host(UWORD8 *msa, UWORD16 rx_len, signed char rssi)
{
#ifndef ETHERNET_HOST
    //mem_handle_t *mem_handle  = g_shared_mem_handle;
    UWORD8       *info_msg    = NULL;
    UWORD16      ninfo_data_len= 0;
    UWORD16 offset = config_host_hdr_offset(BFALSE,0);

	info_msg = (UWORD8*)NMI_MALLOC(MAX_BUFFER_LEN);
    if(info_msg == NULL)
    {
#ifdef DEBUG_PRINT
        debug_print("No buffer to send Beacon/ProbeRsp\n");
#endif /* DEBUG_PRINT */
        return;
    }
    
	/* TODO: Mostafa: uncomment the following line if the input argument rx_len includes FCS */
	//rx_len -= FCS_LEN;		///////////////
    
	ninfo_data_len = (rx_len + 9);


    info_msg[offset + 0] = 'N';
    info_msg[offset + 1] = g_network_info_id;
    /* Increment the id sequence counter by 1*/
    g_network_info_id++;

    info_msg[offset + 2] = ninfo_data_len & 0xFF;
    info_msg[offset + 3] = (ninfo_data_len & 0xFF00) >> 8;

    info_msg[offset + 4] = WID_NETWORK_INFO & 0xFF;
    info_msg[offset + 5] = (WID_NETWORK_INFO & 0xFF00) >> 8;

    info_msg[offset + 6] = (rx_len + 1) & 0xFF;
    info_msg[offset + 7] = ((rx_len + 1) & 0xFF00) >> 8;

    info_msg[offset + 8] = rssi;

    NMI_memcpy(&info_msg[offset + 9],msa,rx_len);

    send_host_rsp(info_msg, ninfo_data_len);	
	
	if(info_msg != NULL)
	{
		NMI_FREE(info_msg);
		info_msg = NULL;
	}

#endif /* ETHERNET_HOST */
}

/*bug3819: Add Scan acomplete notification to host*/
void send_scan_complete_to_host(UWORD8 scan_done)
{

#ifndef ETHERNET_HOST
    UWORD8 *info_msg = NULL;
    UWORD8 *info_buf = NULL;
	
	info_buf = (UWORD8*)NMI_MALLOC(MAX_BUFFER_LEN);

        if(info_buf == NULL) {
			PRINT_ER("Can't alloc memory...\n");	
            return;
        }

        info_msg = info_buf + config_host_hdr_offset(BFALSE,0);

        info_msg[0] = 'S';
        info_msg[1] = g_scan_complete_id;

        /* Increment the id sequence counter by 1*/
        g_scan_complete_id++;

        info_msg[2] = (SCAN_COMPLETE_MSG_LEN) & 0xFF;
	
        info_msg[3] = ((SCAN_COMPLETE_MSG_LEN) & 0xFF00) >> 8;

        /* Set the WID_SCAN_COMPLETE, Length to 1, Value to mac_status */
        info_msg[4] = WID_SCAN_COMPLETE & 0xFF;
        info_msg[5] = (WID_SCAN_COMPLETE & 0xFF00) >> 8;
        info_msg[6] = 1;
        info_msg[7] = scan_done;

        /* The response is sent back to the host */
	  PRINT_INFO(CORECONFIG_DBG,"Sending  scan complete notification\n");
      send_host_rsp(info_buf, SCAN_COMPLETE_MSG_LEN);

	  if(info_buf != NULL)
		{
		NMI_FREE(info_buf);
		info_buf = NULL;
		}

#endif
}


/*****************************************************************************/
/*                                                                           */
/*  Function Name : send_sta_join_info_to_host                               */
/*                                                                           */
/*  Description   : This function sends info of a joining/leaving sta to host*/
/*                                                                           */
/*  Inputs        : station address                                          */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : This function prepares an Information frame having the   */
/*                  information about a joining/leaving station.             */
/*                                                                           */
/*  Outputs       : None                                                     */
/*  Returns       : None                                                     */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         08 06 2010   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
void send_join_leave_info_to_host(UWORD16 aid, UWORD8* sta_addr, BOOL_T joining)
{
#if 0

#ifndef ETHERNET_HOST
    //mem_handle_t *mem_handle  = g_shared_mem_handle;
    UWORD8       *info_msg    = NULL;
    UWORD16      info_len     = 16;
    UWORD16 offset = config_host_hdr_offset(BFALSE,0);

    info_msg = (UWORD8*)NMI_MALLOC(MAX_BUFFER_LEN);
    if(info_msg == NULL)
    {
#ifdef DEBUG_PRINT
        debug_print("No buffer to send STA join/leave info to host\n");
#endif /* DEBUG_PRINT */
        return;
    }

    /* Get the infromation of joining station */
    if(joining == BTRUE)
    {
        info_len = get_sta_join_info(sta_addr, &info_msg[offset + 8]);
    }
    /* prepare the information of leaving station */
    else
    {
        /* Reset all the fields */
        memset(&info_msg[offset + 8], 0, info_len);

        /* Copy the association id of the leaving station */
        info_msg[offset + 8] = aid & 0xFF;

        /* Copy the MAC address of the leaving station */
        NMI_memcpy(&info_msg[offset + 9], sta_addr, 6);
    }

    if(info_len)
    {
        info_msg[offset + 0] = 'N';
        info_msg[offset + 1] =  g_info_id;

        /* Increment the id sequence counter by 1*/
        g_info_id++;

        /* Prepare the frame */
        info_msg[offset + 2] =  (info_len + 8) & 0xFF;
        info_msg[offset + 3] = ((info_len + 8) & 0xFF00) >> 8;

        info_msg[offset + 4] = WID_STA_JOIN_INFO & 0xFF;
        info_msg[offset + 5] = (WID_STA_JOIN_INFO & 0xFF00) >> 8;

        info_msg[offset + 6] =  info_len & 0xFF;
        info_msg[offset + 7] = (info_len & 0xFF00) >> 8;

        send_host_rsp(info_msg, (info_len + 8));
    }
    else
    {
		//NMI_FREE(g_shared_mem_handle, info_msg);
	}
#endif /* ETHERNET_HOST */

#endif
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : save_wids                                                */
/*                                                                           */
/*  Description   : This function saves all the wid values to a global arry  */
/*                                                                           */
/*  Inputs        : None                                                     */
/*                                                                           */
/*  Globals       : g_char_wid_struct                                        */
/*                  g_short_wid_struct                                       */
/*                  g_int_wid_struct                                         */
/*                  g_str_wid_struct                                         */
/*                  g_binary_data_wid_struct                                 */
/*                  g_current_settings                                       */
/*                  g_current_len                                            */
/*                                                                           */
/*  Processing    : Checks if the WID has a get and set function and stores  */
/*                  the values of all such WIDs in to global array for       */
/*                  restoring from it later                                  */
/*                                                                           */
/*  Outputs       : None                                                     */
/*  Returns       : None                                                     */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         21 05 2005   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
void save_wids(void)
{
    UWORD8 count = 0;
    UWORD16 idx   = 0;

    /* Save all the settings in the global array. This is saved as a */
    /* dummy Write message with all WID values. It is used to set    */
    /* the parameters once reset is done.                            */


    /* The global WID array is accessed and all the current WID  */
    /* values are saved in the format of WID ID, Length, Value.  */
    for(count = 0; count < NUM_CHAR_WID; count++)
    {
        if((g_char_wid_struct[count].set) &&
           (g_char_wid_struct[count].get))
            idx += get_wid(&g_current_settings[idx], count,
                           WID_CHAR);
    }

    for(count = 0; count < NUM_SHORT_WID; count++)
    {
        if((g_short_wid_struct[count].set)&&
           (g_short_wid_struct[count].get))
            idx += get_wid(&g_current_settings[idx], count,
                           WID_SHORT);
    }

    for(count = 0; count < NUM_INT_WID; count++)
    {
        if((g_int_wid_struct[count].set)&&
           (g_int_wid_struct[count].get))
            idx += get_wid(&g_current_settings[idx], count,
                           WID_INT);
    }

    for(count = 0; count < NUM_STR_WID; count++)
    {
        if((g_str_wid_struct[count].set)&&
           (g_str_wid_struct[count].get))
            idx += get_wid(&g_current_settings[idx], count,
                           WID_STR);

        /*To make sure that it doesnt cross g_current_settings array boundry*/
        if(idx >= (MAX_QRSP_LEN - 50))
            break;
    }

   /* There may not be enought memory for this operation, so     */
   /* disabling it for now                                       */
#if 0
    for(count = 0; count < NUM_BIN_DATA_WID; count++)
    {
        if((g_binary_data_wid_struct[count].set)&&
           (g_binary_data_wid_struct[count].get))
            idx += get_wid(&g_current_settings[idx], count,
                           WID_BIN_DATA);
    }
#endif
    /* Set the length of global current configuration settings */
    g_current_len = idx;

#ifdef DEBUG_MODE
    /* Check whether query response length exceeded max length */
    if(g_current_len > MAX_QRSP_LEN)
    {
        g_mac_stats.max_query_rsp_len_exceeded = BTRUE;
    }
#endif /* DEBUG_MODE */
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : restore_wids                                             */
/*                                                                           */
/*  Description   : This function restores all the wids from the global arry */
/*                                                                           */
/*  Inputs        : None                                                     */
/*                                                                           */
/*  Globals       : g_current_settings                                       */
/*                  g_current_len                                            */
/*                                                                           */
/*  Processing    : Calls the process write function for restoring all the   */
/*                  WID values                                               */
/*                                                                           */
/*  Outputs       : None                                                     */
/*  Returns       : None                                                     */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         21 05 2005   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
void restore_wids(void)
{
    /* Set all the required parameters from the saved configuration  */
    /* settings.                                                     */
    process_write(g_current_settings, g_current_len);
}



/*****************************************************************************/
/*                                                                           */
/*  Function Name : make_wid_rsp                                             */
/*                                                                           */
/*  Description   : This function make response massage to host              */
/*                                                                           */
/*  Inputs        : 1) buffer to response for wid                            */
/*                  2) massage id of wid                                     */
/*                  3) massage data of wid                                   */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : set massage data of response                             */
/*                                                                           */
/*  Outputs       : None                                                     */
/*  Returns       : None                                                     */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         15 04 2007                   Draft                                */
/*                                                                           */
/*****************************************************************************/
void make_wid_rsp(UWORD8 *wid_rsp_data, UWORD8 msg_id, UWORD8 wid_massage)
{
    wid_rsp_data[0] = 'R';
    wid_rsp_data[1] = msg_id;
    wid_rsp_data[2] = (WRITE_RSP_LEN + MSG_HDR_LEN) & 0xFF;
    wid_rsp_data[3] = ((WRITE_RSP_LEN + MSG_HDR_LEN) & 0xFF00) >> 8;

    /* Set the WID_STATUS, Length to 1, Value to SUCCESS */
    wid_rsp_data[4] = WID_STATUS & 0xFF;
    wid_rsp_data[5] = (WID_STATUS & 0xFF00) >> 8;
    wid_rsp_data[6] = 1;
    wid_rsp_data[7] = wid_massage;

    return;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : parse_config_message                                     */
/*                                                                           */
/*  Description   : This function parses the host configuration messages.    */
/*                                                                           */
/*  Inputs        : 1) Pointer to the MAC structure                          */
/*                  2) Pointer to configuration message                      */
/*                  3) Pointer to Buffer address                             */
/*                                                                           */
/*  Globals       : g_reset_mac                                              */
/*                  g_current_settings                                       */
/*                  g_current_len                                            */
/*                                                                           */
/*  Processing    : This function processes all the configuration messages.  */
/*                  from the host based on the SME type. Currently only      */
/*                  SME types 'Test' and 'Auto' are supported. In case of a  */
/*                  Query request the query response is prepared using the   */
/*                  values of the queried parameters. No MAC state change    */
/*                  occurs in this case. In case of a Write request the      */
/*                  parameters are set and the Write response prepared. If   */
/*                  any parameter writing requires the MAC to be reset, the  */
/*                  appropriate function is called to reset MAC. The         */
/*                  paramters are saved in a global list. After MAC is reset */
/*                  these parameters are set once again. Thereafter, a Scan  */
/*                  Request is prepared and sent to the MAC. Response to the */
/*                  host is sent in the required format based on the SME     */
/*                  mode.                                                    */
/*                                                                           */
/*  Outputs       : None                                                     */
/*  Returns       : None                                                     */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision History:                                                        */
/*                                                                           */
/*         DD MM YYYY   Author(s)       Changes (Describe the changes made)  */
/*         21 05 2005   Ittiam          Draft                                */
/*                                                                           */
/*****************************************************************************/
void parse_config_message(UWORD8* host_req,
                          UWORD8 *buffer_addr, BOOL_T send_write_rsp)
{
    UWORD8  *rtn_buff = NULL;
    UWORD16 rtn_len = 0;
    UWORD8  msg_type  = 0;
    UWORD8  msg_id    = 0;
    UWORD16 msg_len   = 0;
    BOOL_T  free_flag = BTRUE;
    //mem_handle_t *mem_handle  = g_shared_mem_handle;
    UWORD16 offset = config_host_hdr_offset(BFALSE,0);
	UWORD8  *query_rsp = NULL;
	UWORD8  *write_rsp = NULL;
	UWORD8  *error_rsp = NULL;

#ifdef HOST_RX_LOCAL_MEM
    if(g_eth2sram_trfr == BTRUE)
        mem_handle = (void *) g_shared_mem_handle;
    else
        mem_handle = (void *) g_local_mem_handle;
#endif /* HOST_RX_LOCAL_MEM */

    /* Extract the Type, Length and ID of the incoming host message. The     */
    /* format of the message is:                                             */
    /* +-------------------------------------------------------------------+ */
    /* | Message Type | Message ID |  Message Length |Message body         | */
    /* +-------------------------------------------------------------------+ */
    /* |     1 Byte   |   1 Byte   |     2 Bytes     | Message Length      | */
    /* +-------------------------------------------------------------------+ */

    msg_type = host_req[0];
    msg_id   = host_req[1];
    msg_len  = host_req[3];
    msg_len  = (msg_len << 8) | host_req[2];
    msg_len -= MSG_HDR_LEN;

    /* The  valid types of incoming messages are 'Q' (Query) and 'W' (Write) */
    switch(msg_type)
    {
    case 'Q':
        {
		
            UWORD16 rsp_len    = 0;            

            /* To a Query message a Response message needs to be sent. This      */
            /* message has a type 'R' and should have an ID equal to the         */
            /* incoming Query message.                                           */
            query_rsp = (UWORD8*)NMI_MALLOC(MAX_BUFFER_LEN);
            if(query_rsp == NULL)
            {
#ifdef DEBUG_PRINT            	
				debug_print("WID Q MemAlloc Fail\n");
#endif /* DEBUG_PRINT */                
                return;
            }

            /* Set the Message Type and ID. The Message Length is set after the  */
            /* message contents are written. The length is known only after that.*/
            /* Process the Query message and set the contents as required. */
	
            rsp_len = process_query(&host_req[MSG_DATA_OFFSET],
                &query_rsp[offset + MSG_DATA_OFFSET], msg_len);

            /* The length of the message returned is set in the Message length   */
            /* field.                                                            */
            if (0 == rsp_len)
            {
            	make_wid_rsp((query_rsp + offset),msg_id,(UWORD8)WRSP_SUCCESS);
            	rsp_len = 4;
        	}
        	else
        	{
            	/* The length of the message returned is set in the Message length   */
            	/* field.                                                            */
            	query_rsp[offset + 0] = 'R';
            	query_rsp[offset + 1] = msg_id;
            	query_rsp[offset + 2] = (rsp_len + MSG_HDR_LEN) & 0xFF;
            	query_rsp[offset + 3] = ((rsp_len + MSG_HDR_LEN) & 0xFF00) >> 8;
        	}
            /* The response is sent back to the host. No MAC state changes occur */
            /* on reception of a Query message.                                  */
            rtn_buff = query_rsp;
            rtn_len  = (UWORD16)(rsp_len + MSG_HDR_LEN);
			Rsp_Len = rtn_len;
						
        }
        break;

    case 'W':
        {
	
#ifdef BURST_TX_MODE
            if(get_DesiredBSSType() != 4)
            {
                g_burst_tx_mode_enabled_earlier = BFALSE;
            }
            else
            {
                g_burst_tx_mode_enabled_earlier = BTRUE;
            }
#endif /* BURST_TX_MODE */

            /* Process the request only if it's sequence number is correct */
            if((g_last_w_seq != msg_id)
            /* Exception for Debug mode as automation program sends seq no:0 always */
            || (msg_id == 0)
            )
            {
                /* Process the Write message and set the parameters with the given   */
                /* values.                                                           */
                process_write(host_req + MSG_DATA_OFFSET, msg_len);

            }
            else
            {
                g_reset_mac = BFALSE;
            }
            g_last_w_seq = msg_id;


            /* Change MAC states as required. If it is already in Enabled state  */
            /* reset MAC and start again. The previous configuration values are  */
            /* retained.                                                         */
            if(g_reset_mac == BTRUE)
#ifndef MAC_HW_UNIT_TEST_MODE
            //if(mac->state != DISABLED)
#endif /* MAC_HW_UNIT_TEST_MODE */
            {
		
#ifdef DEBUG_MODE
	            g_reset_stats.configchange++;
#endif /* DEBUG_MODE */

                g_reset_mac_in_progress    = BTRUE;                
                
                free_flag = BFALSE;

                /* Save the current configuration before going for Reset         */
                save_wids();

                /* Reset MAC - Bring down PHY and MAC H/W, disable MAC           */
                /* interrupts and release all OS structures. Further, this       */
                /* function restarts the MAC again from start.                   */
              //  reset_mac(mac);

                /* Restore the saved configuration before resetting              */
                restore_wids();

                g_reset_mac_in_progress       = BFALSE;

#ifdef BURST_TX_MODE
                /* Start scaning only when burst tx mode is disabled */
                if(g_burst_tx_mode_enabled == BTRUE)
                {
                    init_mac_fsm_bt(&g_mac);
                }
#endif /* BURST_TX_MODE */
        }

#ifndef MAC_HW_UNIT_TEST_MODE
        //if(mac->state == DISABLED)
#else /* MAC_HW_UNIT_TEST_MODE */
        if(g_test_start == BTRUE)
#endif /* MAC_HW_UNIT_TEST_MODE */
        {
            //start_mac_and_phy(&g_mac);
        }
		
		if(BTRUE == send_write_rsp)
		{
	        /* To a Write message a Response message needs to be sent. This      */
	        /* message has a type 'R' and should have an ID equal to the         */
	        /* incoming Write message. The Message contents contain 'Y' to       */
	        /* indicate Success.                                                 */
			write_rsp = (UWORD8*)NMI_MALLOC(MAX_BUFFER_LEN);
	        if(write_rsp == NULL)
	        {
#ifdef DEBUG_PRINT            	
				debug_print("WID S MemAlloc Fail\n");
#endif /* DEBUG_PRINT */	        		
	            
	            return;
	        }

        	make_wid_rsp((write_rsp + offset),msg_id,WRSP_SUCCESS);
	        rtn_buff = write_rsp;
    	    rtn_len  = WRITE_RSP_LEN + MSG_HDR_LEN;
			Rsp_Len = rtn_len;
		}

    }
    break;

    default:
        {
            /* Unknown message type. Do nothing. */

#ifdef DEBUG_PRINT
		debug_print("WID: Neither Set Not Query\n");
#endif /* DEBUG_PRINT */

        /* To a Write message a Response message needs to be sent. This      */
        /* message has a type 'R' and should have an ID equal to the         */
        /* incoming Write message. The Message contents contain 'Y' to       */
        /* indicate Success.                                                 */
        error_rsp = (UWORD8*)NMI_MALLOC(MAX_BUFFER_LEN);
        if(error_rsp == NULL)
        {            
            return;
        }
        /* make returned data to host */
        	make_wid_rsp((error_rsp + offset),msg_id,WRSP_SUCCESS);
        	rtn_buff = error_rsp;
        	rtn_len  = WRITE_RSP_LEN + MSG_HDR_LEN;
			Rsp_Len = rtn_len;
        }
    } /* end of switch(msg_type) */

    /* send returned data to host */
    if(rtn_buff != NULL)    
	{
		NMI_memcpy(buffer_addr,rtn_buff+ offset,rtn_len);
	}

	if(query_rsp != NULL)
	{
		NMI_FREE(query_rsp);
		query_rsp = NULL;
	}
	
	if(write_rsp != NULL)
	{
		NMI_FREE(write_rsp);
		write_rsp = NULL;
	}

	if(error_rsp != NULL)
	{
		NMI_FREE(write_rsp);
		write_rsp = NULL;
	}

}



static void SendMacStatus(void* pvUsrArg)
{
	while(NMI_TRUE)
	{	
		NMI_Sleep(100);

		if(gbTermntMacStsThrd == NMI_TRUE)
		{
			NMI_SemaphoreRelease(&SemHandleMacStsExit, NMI_NULL);
			break;
		}
	
		send_mac_status(g_current_mac_status);
	}
}


static void SimulatorThread(void* pvUsrArg)
{	
	tstrSimulatorMsg* pstrSimulatorMsg = NMI_NULL;
	NMI_Uint32 u32Ret;
	NMI_Sint32 s32Err = NMI_SUCCESS;
	NMI_Uint8* pu8ConfigPkt = NULL;
	NMI_Sint32 s32PktDataLen = 0;
	NMI_Uint8* pu8PktResponse = NULL;
	NMI_Sint32 s32PktResponseLen = -1;
	NMI_Sint32 s32RcvdPktLen = 0;
	
	pstrSimulatorMsg = (tstrSimulatorMsg*)NMI_MALLOC(sizeof(tstrSimulatorMsg));	

	pu8ConfigPkt = (NMI_Uint8*)NMI_MALLOC(MAX_BUFFER_LEN);
	if(pu8ConfigPkt == NULL)
	{
		PRINT_D(CORECONFIG_DBG,"Failed in pu8ConfigPkt allocation \n");
	}
		
	pu8PktResponse = (NMI_Uint8*)NMI_MALLOC(MAX_BUFFER_LEN);
	if(pu8PktResponse == NULL)
	{
		PRINT_D(CORECONFIG_DBG,"Failed in pu8PktResponse allocation \n");
	}

	while(NMI_TRUE)
	{	
		NMI_memset((void*)(pstrSimulatorMsg), 0, sizeof(tstrSimulatorMsg));
	
		NMI_MsgQueueRecv(&gMsgQSimulator, pstrSimulatorMsg, 
						 sizeof(tstrSimulatorMsg), &u32Ret, NMI_NULL);				
		
		if(pstrSimulatorMsg->u32SimThreadCmd == SIMULATOR_MSG_EXIT)
		{	
			/* exit the Thread function immediately without going to Send_Response
			   because the Semaphores to be used there are expected to be destroyed  
			   because a Deinitialization mechanism is in progress */
									
			goto Exit_Thread;
		}			
				
		NMI_memset((void*)pu8ConfigPkt, 0, MAX_BUFFER_LEN);
		NMI_memset((void*)pu8PktResponse, 0, MAX_BUFFER_LEN);

		if(pstrSimulatorMsg->s32PktDataLen < 0)
		{			
			s32PktResponseLen = -1;
			goto Send_Response;
		}


		s32PktDataLen = pstrSimulatorMsg->s32PktDataLen;
		
		s32Err = FIFO_ReadBytes(ghTxFifoBuffer, pu8ConfigPkt, s32PktDataLen, &s32RcvdPktLen);
		if(s32Err != NMI_SUCCESS)
		{
			NMI_ERROR("SimulatorThread(): FIFO_ReadBytes() returned error \n");
			s32PktResponseLen = -1;
			goto Send_Response;
		}
		
		#if 0
		/* The following lines are just for testing the architecture functionality of the FIFO Buffers, 
		   the threads and the Message Queue */

		ProcessMessage(pu8ConfigPkt, s32PktDataLen);
		PrepareResponse(pu8ConfigPkt, s32PktDataLen, pu8PktResponse, &s32PktResponseLen);
		
		#else
		
		/* Passing data to parser to process it */		
		parse_config_message(pu8ConfigPkt, pu8PktResponse, 1);
		s32PktResponseLen = Rsp_Len;
		
		#endif
		
		
		/* The following lines are just for testing sending asynchronous msg containing network scan results 
		   via the API send_network_info_to_host(), such that expextin thread at the other side receiving this 
		   msg and parses it */
		#if 0
		{
			WORD8 rssi = 0;
			UWORD16 rx_len = 0;

			
			
			rx_len = sizeof(msa2);
			
			NMI_PRINTF("rx_len = sizeof(msa2) = %d \n", rx_len);
			
			rssi = (WORD8)-123;
			
			/* TODO: Mostafa: uncomment the line: rx_len -= FCS_LEN; if the input argument rx_len includes FCS */				
			send_network_info_to_host(msa2, rx_len, rssi);
		}

		
		#else						

Send_Response:						
		send_host_rsp(pu8PktResponse, (UWORD16)s32PktResponseLen);	
		
		#endif
			
	}


Exit_Thread:
	
	if(pstrSimulatorMsg != NMI_NULL)
	{
		NMI_FREE(pstrSimulatorMsg);
		pstrSimulatorMsg = NMI_NULL;
	}	
	
	if(pu8ConfigPkt != NULL)
	{
		NMI_FREE(pu8ConfigPkt);
		pu8ConfigPkt = NULL;
	}

	if(pu8PktResponse != NULL)
	{
		NMI_FREE(pu8PktResponse);
		pu8PktResponse = NULL;
	}
		
}

NMI_Sint32 CoreConfigSimulatorDeInit(void)
{
	NMI_Sint32 s32Error = NMI_SUCCESS;		
	tstrSimulatorMsg* pstrSimulatorMsg = NMI_NULL;				

	PRINT_D(CORECONFIG_DBG,"CoreConfigSimulatorDeInit() \n");
	
	pstrSimulatorMsg = (tstrSimulatorMsg*)NMI_MALLOC(sizeof(tstrSimulatorMsg));
	NMI_memset((void*)(pstrSimulatorMsg), 0, sizeof(tstrSimulatorMsg));	

	pstrSimulatorMsg->u32SimThreadCmd = SIMULATOR_MSG_EXIT;
	NMI_MsgQueueSend(&gMsgQSimulator, pstrSimulatorMsg, sizeof(tstrSimulatorMsg), NMI_NULL);
	
	if(pstrSimulatorMsg != NMI_NULL)
	{
		NMI_FREE(pstrSimulatorMsg);
		pstrSimulatorMsg = NMI_NULL;
	}	

	NMI_ThreadDestroy(&ThrdHandleSimulator, NMI_NULL);
					
	NMI_MsgQueueDestroy(&gMsgQSimulator, NMI_NULL);

	FIFO_DeInit(ghTxFifoBuffer);

	bscan_code = NMI_FALSE;

	NMI_SemaphoreRelease(&SemHandleScanReq,NMI_NULL);
	NMI_SemaphoreAcquire(&SemHandleScanExit,NMI_NULL);

	NMI_ThreadDestroy(&ThrdHandleScan, NMI_NULL);
	NMI_SemaphoreDestroy(&SemHandleScanReq, NMI_NULL);	
	NMI_SemaphoreDestroy(&SemHandleScanExit, NMI_NULL);
	
	gbTermntMacStsThrd = NMI_TRUE;
	NMI_SemaphoreAcquire(&SemHandleMacStsExit, NMI_NULL);
	NMI_ThreadDestroy(&ThrdHandleMacStatus, NMI_NULL);
	NMI_SemaphoreDestroy(&SemHandleMacStsExit, NMI_NULL);
	
	return s32Error;
}
