#ifndef NMI_WLAN_H
#define NMI_WLAN_H

#include "nmi_type.h"


#define ISNMC1000(id)   (((id & 0xfffff000) == 0x100000) ? 1 : 0) 

/**
	DMA_VER_1: Old DMA interface introduced since A0. It exists in all 
	later versions and all versions are backward compatible with this interface. 
**/
#define DMA_VER_1 (1) 
/**
	DMA_VER_21: DMA interface introduced in D0/E0 versions and later. It uses 
	fewer number of registers to setup the TX/RX DMA and makes used of SDIO 
	CMD52 mapped registers and SPI internal registers for faster SDIO and SPI 
	DMAs respectively.  	
**/
#define DMA_VER_2 (2) 

#ifdef USE_DMA_VER_1
#define DMA_VER DMA_VER_1
#else
#define DMA_VER DMA_VER_2
#endif


/********************************************

	Mac eth header length 

********************************************/
#define DRIVER_HANDLER_SIZE 4
#define MAX_MAC_HDR_LEN         26 //QOS_MAC_HDR_LEN
#define SUB_MSDU_HEADER_LENGTH  14
#define SNAP_HDR_LEN            8
#define ETHERNET_HDR_LEN          14
#define WORD_ALIGNMENT_PAD        0		

#define ETH_ETHERNET_HDR_OFFSET   (MAX_MAC_HDR_LEN + SUB_MSDU_HEADER_LENGTH + \
                                   SNAP_HDR_LEN - ETHERNET_HDR_LEN + WORD_ALIGNMENT_PAD)
     
/*Bug3959: transmitting mgmt frames received from host*/
#define HOST_HDR_OFFSET		4
#define ETHERNET_HDR_LEN          14
#define IP_HDR_LEN                20
#define IP_HDR_OFFSET             ETHERNET_HDR_LEN
#define UDP_HDR_OFFSET            (IP_HDR_LEN + IP_HDR_OFFSET)
#define UDP_HDR_LEN               8
#define UDP_DATA_OFFSET           (UDP_HDR_OFFSET + UDP_HDR_LEN)
#define ETH_CONFIG_PKT_HDR_LEN    UDP_DATA_OFFSET

#define ETH_CONFIG_PKT_HDR_OFFSET (ETH_ETHERNET_HDR_OFFSET + \
                                   ETH_CONFIG_PKT_HDR_LEN)
#define   ACTION         0xD0
#define   PROBE_REQ   0x40
#ifdef NMI_FULLY_HOSTING_AP
#define	FH_TX_HOST_HDR_OFFSET	24
#endif

/********************************************

	Endian Conversion 

********************************************/

#define BYTE_SWAP(val) ((((val) & 0x000000FF) << 24) + \
                                   		(((val) & 0x0000FF00) << 8)  + \
                                   		(((val) & 0x00FF0000) >> 8)   + \
                                   		(((val) & 0xFF000000) >> 24))

/********************************************

	Register Defines

********************************************/
#define NMI_PERIPH_REG_BASE 0x1000
/*BugID_5137*/
#define NMI_CHANGING_VIR_IF                     (0x108c)
#define NMI_CHIPID	(NMI_PERIPH_REG_BASE)
#define NMI_GLB_RESET_0 (NMI_PERIPH_REG_BASE + 0x400)
#define NMI_PIN_MUX_0 (NMI_PERIPH_REG_BASE + 0x408)
#define NMI_HOST_TX_CTRL (NMI_PERIPH_REG_BASE + 0x6c)
#define NMI_HOST_RX_CTRL_0 (NMI_PERIPH_REG_BASE + 0x70)
#define NMI_HOST_RX_CTRL_1 (NMI_PERIPH_REG_BASE + 0x74)
#define NMI_HOST_VMM_CTL	(NMI_PERIPH_REG_BASE + 0x78)
#define NMI_HOST_RX_CTRL	(NMI_PERIPH_REG_BASE + 0x80)
#define NMI_HOST_RX_EXTRA_SIZE	(NMI_PERIPH_REG_BASE + 0x84)
#define NMI_HOST_TX_CTRL_1	(NMI_PERIPH_REG_BASE + 0x88)
#define NMI_MISC	(NMI_PERIPH_REG_BASE+0x428)
#define NMI_INTR_REG_BASE (NMI_PERIPH_REG_BASE+0xa00)
#define NMI_INTR_ENABLE (NMI_INTR_REG_BASE)
#define NMI_INTR2_ENABLE (NMI_INTR_REG_BASE+4)

#define NMI_INTR_POLARITY (NMI_INTR_REG_BASE+0x10)
#define NMI_INTR_TYPE (NMI_INTR_REG_BASE+0x20)
#define NMI_INTR_CLEAR (NMI_INTR_REG_BASE+0x30)
#define NMI_INTR_STATUS (NMI_INTR_REG_BASE+0x40)

#define NMI_VMM_TBL_SIZE 64
#define NMI_VMM_TX_TBL_BASE (0x150400)
#define NMI_VMM_RX_TBL_BASE (0x150500)

#define NMI_VMM_BASE 0x150000
#define NMI_VMM_CORE_CTL (NMI_VMM_BASE)
#define NMI_VMM_TBL_CTL (NMI_VMM_BASE+0x4)
#define NMI_VMM_TBL_ENTRY (NMI_VMM_BASE+0x8)
#define NMI_VMM_TBL0_SIZE (NMI_VMM_BASE+0xc)
#define NMI_VMM_TO_HOST_SIZE (NMI_VMM_BASE+0x10)
#define NMI_VMM_CORE_CFG (NMI_VMM_BASE+0x14)
#define NMI_VMM_TBL_ACTIVE (NMI_VMM_BASE+040)
#define NMI_VMM_TBL_STATUS (NMI_VMM_BASE+0x44)

#define NMI_SPI_REG_BASE 0xe800
#define NMI_SPI_CTL (NMI_SPI_REG_BASE)
#define NMI_SPI_MASTER_DMA_ADDR (NMI_SPI_REG_BASE+0x4)
#define NMI_SPI_MASTER_DMA_COUNT (NMI_SPI_REG_BASE+0x8)
#define NMI_SPI_SLAVE_DMA_ADDR (NMI_SPI_REG_BASE+0xc)
#define NMI_SPI_SLAVE_DMA_COUNT (NMI_SPI_REG_BASE+0x10)
#define NMI_SPI_TX_MODE (NMI_SPI_REG_BASE+0x20)
#define NMI_SPI_PROTOCOL_CONFIG (NMI_SPI_REG_BASE+0x24)
#define NMI_SPI_INTR_CTL (NMI_SPI_REG_BASE+0x2c)

#define NMI_SPI_PROTOCOL_OFFSET (NMI_SPI_PROTOCOL_CONFIG-NMI_SPI_REG_BASE)

#define NMI_AHB_DATA_MEM_BASE 0x30000
#define NMI_AHB_SHARE_MEM_BASE 0xd0000

#define NMI_VMM_TBL_RX_SHADOW_BASE NMI_AHB_SHARE_MEM_BASE /* Bug 4477 fix */
#define NMI_VMM_TBL_RX_SHADOW_SIZE (256) /* Bug 4477 fix */

#define _NMI_INT_STATS_PRE_D0	    0x1488
#define _NMI_INT_STATS_D0_AND_LATER	0x149c

/* 
 * General purpose register 1
 * Bit 0: SDIO interrupt type: (This bit is ignored in case of SPI bus.)
 *    1: SDIO uses a GPIO as interrupt. 
 *    0, SDIO native interrupt SDIO_DAT1 used.
 * Bit 1: CLK type used
 *	1: RTC clock is supplied externally from host
 *	0: Use internal 80Mhz clock
 */ 

#define _NMI_GP_REG_1_PRE_D0         0x148c
#define _NMI_GP_REG_1_D0_AND_LATER   0x14a0

#define NMI_HAVE_SDIO_IRQ_GPIO (1 << 0)
#define NMI_HAVE_RTC           (1 << 1)
#define NMI_HAVE_DMA_VER_2     (1 << 2)
#define NMI_HAVE_VCO_14_SUPPLY (1 << 3)



/********************************************

	Wlan Defines

********************************************/
#define NMI_CFG_PKT	1
#define NMI_NET_PKT 0
/*Bug3959: transmitting mgmt frames received from host*/
#ifdef NMI_AP_EXTERNAL_MLME
#define NMI_MGMT_PKT 2

#ifdef NMI_FULLY_HOSTING_AP
#define NMI_FH_DATA_PKT 4
#endif

#endif /*NMI_AP_EXTERNAL_MLME*/
#define NMI_CFG_SET 1
#define NMI_CFG_QUERY 0

#define NMI_CFG_RSP	1
#define NMI_CFG_RSP_STATUS 2
#define NMI_CFG_RSP_SCAN 3

#ifdef NMI_SDIO
#define NMI_PLL_TO	4
#else
#define NMI_PLL_TO	2
#endif

/*******************************************/
/*        CA Interrupt flags.              */
/*******************************************/
#define PLL_INT	(1<<0)
#define DATA_INT	(1<<1)
#define SLEEP_INT	(1<<2)
#define ABORT_INT   (1<<31)

#if DMA_VER == DMA_VER_2
/*******************************************/
/*        E0 and later Interrupt flags.    */
/*******************************************/
/*******************************************/
/*        E0 and later Interrupt flags.    */
/*           IRQ Status word               */
/* 15:0 = DMA count in words.              */
/* 16: INT0 flag                           */
/* 17: INT1 flag                           */
/* 18: INT2 flag                           */
/* 19: INT3 flag                           */
/* 20: INT4 flag                           */
/* 21: INT5 flag                           */
/*******************************************/
#define                 IRG_FLAGS_OFFSET 16
#define IRQ_DMA_WD_CNT_MASK ((1ul << IRG_FLAGS_OFFSET) - 1)
#define INT_0           (1<<(IRG_FLAGS_OFFSET))
#define INT_1           (1<<(IRG_FLAGS_OFFSET+1))
#define INT_2           (1<<(IRG_FLAGS_OFFSET+2))
#define INT_3           (1<<(IRG_FLAGS_OFFSET+3))
#define INT_4           (1<<(IRG_FLAGS_OFFSET+4))
#define INT_5           (1<<(IRG_FLAGS_OFFSET+5))
#define MAX_NUM_INT     (6)

/*******************************************/
/*        E0 and later Interrupt flags.    */
/*           IRQ Clear word                */
/* 0: Clear INT0                           */
/* 1: Clear INT1                           */
/* 2: Clear INT2                           */
/* 3: Clear INT3                           */
/* 4: Clear INT4                           */
/* 5: Clear INT5                           */
/* 6: Select VMM table 1                   */
/* 7: Select VMM table 2                   */
/* 8: Enable VMM                           */
/*******************************************/
#define CLR_INT0             (1 << 0)
#define CLR_INT1             (1 << 1)
#define CLR_INT2             (1 << 2)
#define CLR_INT3             (1 << 3)
#define CLR_INT4             (1 << 4)
#define CLR_INT5             (1 << 5)
#define SEL_VMM_TBL0         (1 << 6)
#define SEL_VMM_TBL1         (1 << 7)
#define EN_VMM               (1 << 8)

#define DATA_INT_EXT	INT_0
#define PLL_INT_EXT	    INT_1
#define SLEEP_INT_EXT	INT_2
#define ALL_INT_EXT     (DATA_INT_EXT|PLL_INT_EXT|SLEEP_INT_EXT)
#define NUM_INT_EXT     (3)

#define DATA_INT_CLR	CLR_INT0
#define PLL_INT_CLR	    CLR_INT1
#define SLEEP_INT_CLR	CLR_INT2

#define ENABLE_RX_VMM   (SEL_VMM_TBL1 | EN_VMM)
#define ENABLE_TX_VMM   (SEL_VMM_TBL0 | EN_VMM)

#endif /* ENABLE_FAST_DMA */

/*time for expiring the semaphores of cfg packets*/
#define CFG_PKTS_TIMEOUT	2000
/********************************************

	Debug Type

********************************************/
typedef void (*nmi_debug_func)(uint32_t, char *, ...);

/********************************************

	Tx/Rx Queue Structure

********************************************/

struct txq_entry_t {
	struct txq_entry_t *next;
	struct txq_entry_t *prev;
	int type;
	int tcp_PendingAck_index;
	uint8_t *buffer;
	int buffer_size;
	void *priv;
	int status;
	void (*tx_complete_func)(void *, int);
};

struct rxq_entry_t  {
	struct rxq_entry_t *next;
	uint8_t *buffer;
	int buffer_size;
};

/********************************************

	Host IF Structure

********************************************/

typedef struct {
	int (*hif_init)(nmi_wlan_inp_t *, nmi_debug_func);
	int (*hif_deinit)(void *);
	int (*hif_read_reg)(uint32_t, uint32_t *);
	int (*hif_write_reg)(uint32_t, uint32_t);
	int (*hif_block_rx)(uint32_t, uint8_t *, uint32_t);
	int (*hif_block_tx)(uint32_t, uint8_t *, uint32_t);	
	int (*hif_sync)(void);
	int (*hif_clear_int)(void);	
#if DMA_VER == DMA_VER_2
	int (*hif_read_int)(uint32_t *);
	int (*hif_clear_int_ext)(uint32_t);
	int (*hif_read_size)(uint32_t *);
	int (*hif_block_tx_ext)(uint32_t, uint8_t *, uint32_t);	
	int (*hif_block_rx_ext)(uint32_t, uint8_t *, uint32_t);	
	int (*hif_sync_ext)(int);	
#endif /* DMA_VER == DMA_VER_2 */
	void (*hif_set_max_bus_speed)(void);
	void (*hif_set_default_bus_speed)(void);
} nmi_hif_func_t;

/********************************************

	Configuration Structure

********************************************/

#define MAX_CFG_FRAME_SIZE 1468

typedef struct {
	uint8_t ether_header[14];
	uint8_t ip_header[20];
	uint8_t udp_header[8];
	uint8_t wid_header[8];
	uint8_t frame[MAX_CFG_FRAME_SIZE];
} nmi_cfg_frame_t;

typedef struct {
	int (*wlan_tx)(uint8_t *, uint32_t, nmi_tx_complete_func_t);
} nmi_wlan_cfg_func_t;

typedef struct {
	int type;
	uint32_t seq_no;
} nmi_cfg_rsp_t; 

typedef struct {
	int (*cfg_wid_set)(uint8_t *, uint32_t, uint16_t, uint8_t *, int);
	int (*cfg_wid_get)(uint8_t *, uint32_t, uint16_t);
	int (*cfg_wid_get_val)(uint16_t, uint8_t *, uint32_t);
	int (*rx_indicate)(uint8_t *, int, nmi_cfg_rsp_t *);
	int (*cfg_init)(nmi_debug_func);
} nmi_cfg_func_t;

#endif
