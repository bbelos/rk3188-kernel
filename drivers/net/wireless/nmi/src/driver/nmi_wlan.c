////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Newport Media Inc.  All rights reserved.
//
// Module Name:  nmi_wlan.c
//
//
//////////////////////////////////////////////////////////////////////////////

#include "nmi_wlan_if.h"
#include "nmi_wlan.h"
#define INLINE static __inline

/********************************************

	Global

********************************************/
extern unsigned int int_clrd;
extern nmi_hif_func_t hif_sdio;
extern nmi_hif_func_t hif_spi;
extern nmi_cfg_func_t mac_cfg;
extern void NMI_WFI_mgmt_rx(uint8_t *buff, uint32_t size);
extern void frmw_to_linux(uint8_t *buff, uint32_t size);
int sdio_xfer_cnt(void);
uint32_t nmi_get_chipid(uint8_t update);

//static uint32_t vmm_table[NMI_VMM_TBL_SIZE];
//static uint32_t vmm_table_rbk[NMI_VMM_TBL_SIZE];

//static uint32_t vmm_table_rbk[NMI_VMM_TBL_SIZE];

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

static uint32_t NMI_INT_STATS = 0;
static uint32_t NMI_GP_REG_1 = 0;

static nmi_wlan_dev_t g_wlan; 

/********************************************

	Debug

********************************************/

static uint32_t dbgflag = N_INIT|N_ERR|N_INTR|N_TXQ|N_RXQ;

static void nmi_debug(uint32_t flag, char *fmt, ...)
{
	char buf[256];
	va_list args;
	int len;

	if (flag & dbgflag) {
		va_start(args, fmt);
		len = vsprintf(buf, fmt, args);
		va_end(args);
		
		if (g_wlan.os_func.os_debug)
			g_wlan.os_func.os_debug(buf);
	}

	return;
}

/********************************************

	Queue

********************************************/

static void nmi_wlan_txq_remove(struct txq_entry_t *tqe)
{
	
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	unsigned long flags;
	//p->os_func.os_spin_lock(p->txq_spinlock, &flags);
	if (tqe==p->txq_head) 
	{
		
		p->txq_head = tqe->next;
		if(p->txq_head)
		{
			p->txq_head->prev=NULL;
		}
		
	
	}
	else if(tqe==p->txq_tail)
	{
		p->txq_tail=(tqe->prev);
		if(p->txq_tail)
		{
			p->txq_tail->next=NULL;
		}
	}else
	{
		tqe->prev->next=tqe->next;
		tqe->next->prev=tqe->prev;
			
	}
	p->txq_entries-=1;
	//p->os_func.os_spin_unlock(p->txq_spinlock, &flags);
		
}

static struct txq_entry_t *nmi_wlan_txq_remove_from_head(void)
{
	struct txq_entry_t * tqe;
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	unsigned long flags;
	p->os_func.os_spin_lock(p->txq_spinlock, &flags);
	if (p->txq_head) 
	{
		//p->os_func.os_enter_cs(p->txq_lock);
		tqe = p->txq_head;
		p->txq_head = tqe->next;
		if(p->txq_head)
		{
			p->txq_head->prev=NULL;
		}
		p->txq_entries-=1;

		/*Added by Amr - BugID_4720*/
		
		
		//p->os_func.os_leave_cs(p->txq_lock);
			
	}
	else
	{
		tqe = NULL;
	}
	p->os_func.os_spin_unlock(p->txq_spinlock, &flags);
	return tqe;		
}

static void nmi_wlan_txq_add_to_tail(struct txq_entry_t *tqe)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
		unsigned long flags;
	/*Added by Amr - BugID_4720*/
	p->os_func.os_spin_lock(p->txq_spinlock, &flags);

//	p->os_func.os_enter_cs(p->txq_lock);
	if (p->txq_head == NULL) 
	{
		tqe->next = NULL;
		tqe->prev= NULL;
		p->txq_head = tqe;
		p->txq_tail = tqe;	
		//p->os_func.os_signal(p->txq_wait);
	} else {
		tqe->next = NULL;
		tqe->prev=p->txq_tail;
		p->txq_tail->next = tqe;
		p->txq_tail = tqe;
	}
	p->txq_entries+=1;
	PRINT_D(TX_DBG,"Number of entries in TxQ = %d\n",p->txq_entries);
//	p->os_func.os_leave_cs(p->txq_lock);

	/*Added by Amr - BugID_4720*/
	p->os_func.os_spin_unlock(p->txq_spinlock, &flags);

	/**
		wake up TX queue
	**/
	PRINT_D(TX_DBG,"Wake the txq_handling\n");

	p->os_func.os_signal(p->txq_wait);
		

}

static int nmi_wlan_txq_add_to_head(struct txq_entry_t *tqe)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
//printk("### nmi_wlan_txq_add_to_head() locking txq_lock\n");
		unsigned long flags;
	/*Added by Amr - BugID_4720*/
	if(p->os_func.os_wait(p->txq_add_to_head_lock, CFG_PKTS_TIMEOUT))
		return -1;
	
	p->os_func.os_spin_lock(p->txq_spinlock, &flags);
	
	//p->os_func.os_enter_cs(p->txq_lock);
	if (p->txq_head == NULL) {
		tqe->next = NULL;
		tqe->prev= NULL;
		p->txq_head = tqe;
		p->txq_tail = tqe;
	} else {
		tqe->next = p->txq_head;
		tqe->prev= NULL;
		p->txq_head->prev=tqe;
		p->txq_head = tqe;
	}
	p->txq_entries+=1;
	PRINT_D(TX_DBG,"Number of entries in TxQ = %d\n",p->txq_entries);
	//p->os_func.os_leave_cs(p->txq_lock);
	
	/*Added by Amr - BugID_4720*/
	p->os_func.os_spin_unlock(p->txq_spinlock, &flags);
	p->os_func.os_signal(p->txq_add_to_head_lock);
	

	/**
		wake up TX queue
	**/
	p->os_func.os_signal(p->txq_wait);
	PRINT_D(TX_DBG,"Wake up the txq_handler\n");
//	complete(p->txq_wait);

	/*Added by Amr - BugID_4720*/
	return 0;

}

uint32_t Statisitcs_totalAcks=0,Statisitcs_DroppedAcks=0;

#ifdef	TCP_ACK_FILTER
struct Ack_session_info;
typedef struct Ack_session_info
{
	uint32_t Ack_seq_num;
	uint32_t Bigger_Ack_num;
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t status;
	//struct Ack_session_info * next;
	//struct Ack_session_info * prev;
} Ack_session_info_t;

typedef struct {
	uint32_t ack_num;
	//uint32_t seq_num;
	//uint16_t src_port;
	//uint16_t dst_port;
	//uint32_t dst_ip_addr;
	uint32_t Session_index;
	struct txq_entry_t  * txqe;
	//Ack_session_info * Ack_session;
} Pending_Acks_info_t/*Ack_info_t*/;



	
struct Ack_session_info * Free_head=NULL;
struct Ack_session_info * Alloc_head=NULL;
	
#define TCP_FIN_MASK 		(1<<0)
#define TCP_SYN_MASK 		(1<<1)
#define TCP_Ack_MASK 		(1<<4)
#define NOT_TCP_ACK			(-1)

#define MAX_TCP_SESSION		25
#define MAX_PENDING_ACKS		256
Ack_session_info_t 	Acks_keep_track_info[2*MAX_TCP_SESSION];
Pending_Acks_info_t 	Pending_Acks_info[MAX_PENDING_ACKS];

uint32_t PendingAcks_arrBase=0;
uint32_t Opened_TCP_session=0;
uint32_t Pending_Acks=0;



static int inline Init_TCP_tracking()
{
	
	/*uint32_t i;
	Free_head=&Acks_keep_track_info[0];
	 i=1;
	 Acks_keep_track_info[0].next=&Acks_keep_track_info[1];
	for(i=1<;i<MAX_TCP_SESSION-1;i++)
	{
		Acks_keep_track_info[i].next=&Acks_keep_track_info[i+1];
		Acks_keep_track_info[i].prev=&Acks_keep_track_info[i-1];
	}
	Acks_keep_track_info[49].prev=&Acks_keep_track_info[48];
	*/
	return 0;
	
}
static int inline add_TCP_track_session(uint32_t src_prt,uint32_t dst_prt,uint32_t seq)
{
	Acks_keep_track_info[Opened_TCP_session].Ack_seq_num=seq;
	Acks_keep_track_info[Opened_TCP_session].Bigger_Ack_num=0;
	Acks_keep_track_info[Opened_TCP_session].src_port=src_prt;
	Acks_keep_track_info[Opened_TCP_session].dst_port=dst_prt;
	Opened_TCP_session++;

	PRINT_D(TCP_ENH,"TCP Session %d to Ack %d\n",Opened_TCP_session,seq);
	return 0;
}

static int inline Update_TCP_track_session(uint32_t index,uint32_t Ack)
{
	
	if(Ack>Acks_keep_track_info[index].Bigger_Ack_num)
	{
		Acks_keep_track_info[index].Bigger_Ack_num=Ack;
	}
	return 0;
	
}
static int inline add_TCP_Pending_Ack(uint32_t Ack,uint32_t Session_index,struct txq_entry_t  * txqe)
{
	Statisitcs_totalAcks++;
	if(Pending_Acks<MAX_PENDING_ACKS)
	{
		Pending_Acks_info[PendingAcks_arrBase+Pending_Acks].ack_num=Ack;
		Pending_Acks_info[PendingAcks_arrBase+Pending_Acks].txqe=txqe;
		Pending_Acks_info[PendingAcks_arrBase+Pending_Acks].Session_index=Session_index;
		txqe->tcp_PendingAck_index=PendingAcks_arrBase+Pending_Acks;
		Pending_Acks++;
		
	}
	else
	{
		
	}
	return 0;
}
static int inline remove_TCP_related()
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	unsigned long flags;
	p->os_func.os_spin_lock(p->txq_spinlock, &flags);

	p->os_func.os_spin_unlock(p->txq_spinlock, &flags);
	return 0;
}

static int inline tcp_process(struct txq_entry_t * tqe)
{
	int ret;
	uint8_t *eth_hdr_ptr;
	uint8_t * buffer=tqe->buffer;
	unsigned short h_proto;
	int i;
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	unsigned long flags;
	p->os_func.os_spin_lock(p->txq_spinlock, &flags);
	
	eth_hdr_ptr = &buffer[0];
	h_proto = ntohs(*((unsigned short*)&eth_hdr_ptr[12]));
	if(h_proto == 0x0800) { /* IP */
		uint8_t * ip_hdr_ptr;
		uint8_t protocol;
	
		ip_hdr_ptr = &buffer[ETHERNET_HDR_LEN];
		protocol = ip_hdr_ptr[9];

	
		if(protocol == 0x06) {
			uint8_t * tcp_hdr_ptr;
			uint32_t IHL,Total_Length,Data_offset;
			tcp_hdr_ptr = &ip_hdr_ptr[IP_HDR_LEN];
			IHL=(ip_hdr_ptr[0]&0xf)<<2;
			Total_Length=(((uint32_t)ip_hdr_ptr[2])<<8)+((uint32_t)ip_hdr_ptr[3]);
			Data_offset=(((uint32_t)tcp_hdr_ptr[12]&0xf0)>>2);
			if(Total_Length==(IHL+Data_offset)) /*we want to recognize the clear Acks(packet only carry Ack infos not with data) so data size must be equal zero*/
			{	
				uint32_t seq_no,Ack_no;
				seq_no	=(((uint32_t)tcp_hdr_ptr[4])<<24)+(((uint32_t)tcp_hdr_ptr[5])<<16)+(((uint32_t)tcp_hdr_ptr[6])<<8)+((uint32_t)tcp_hdr_ptr[7]);

				Ack_no	=(((uint32_t)tcp_hdr_ptr[8])<<24)+(((uint32_t)tcp_hdr_ptr[9])<<16)+(((uint32_t)tcp_hdr_ptr[10])<<8)+((uint32_t)tcp_hdr_ptr[11]);

				
				for(i=0;i<Opened_TCP_session;i++)
				{	
					if(Acks_keep_track_info[i].Ack_seq_num == seq_no)
					{
						Update_TCP_track_session(i,Ack_no);
						break;
					}
				}
				if(i==Opened_TCP_session)
				{
					add_TCP_track_session( 0,  0,  seq_no);
				}
				add_TCP_Pending_Ack(Ack_no,i,tqe);
				

			}
			
		} else {
			ret = 0;
		}		
	} else {
		ret = 0;
	}
	p->os_func.os_spin_unlock(p->txq_spinlock, &flags);
	return ret;
}


static int nmi_wlan_txq_filter_dup_tcp_ack(void)
{
	
	uint32_t i=0;
	uint32_t Dropped=0;
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;

	p->os_func.os_spin_lock(p->txq_spinlock, &p->txq_spinlock_flags);
	for(i=PendingAcks_arrBase;i<(PendingAcks_arrBase+Pending_Acks);i++) {
		if(Pending_Acks_info[i].ack_num < Acks_keep_track_info[Pending_Acks_info[i].Session_index].Bigger_Ack_num) {
			struct txq_entry_t *tqe;
			PRINT_D(TCP_ENH, "DROP ACK: %u \n", Pending_Acks_info[i].ack_num);
			tqe = Pending_Acks_info[i].txqe;
			if(tqe)
			{
				nmi_wlan_txq_remove(tqe);
				Statisitcs_DroppedAcks++;
				tqe->status = 1;				/* mark the packet send */
				if (tqe->tx_complete_func) tqe->tx_complete_func(tqe->priv, tqe->status);
				p->os_func.os_free(tqe);
				Dropped++;
				//p->txq_entries -= 1;
			}
		} 
	}
	Pending_Acks=0;
	Opened_TCP_session=0;

	if(PendingAcks_arrBase==0)
	{
		PendingAcks_arrBase=MAX_TCP_SESSION;
	}else
	{
		PendingAcks_arrBase=0;
	}


	p->os_func.os_spin_unlock(p->txq_spinlock, &p->txq_spinlock_flags);

	while(Dropped>0)
	{
		/*consume the semaphore count of the removed packet*/
		p->os_func.os_wait(p->txq_wait,1);
		Dropped--;
	}

	return 1;
}
#endif
static int nmi_wlan_txq_add_cfg_pkt(uint8_t *buffer, uint32_t buffer_size)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	struct txq_entry_t *tqe;

	PRINT_D(TX_DBG,"Adding config packet ...\n");
	if (p->quit){
		PRINT_D(TX_DBG,"Return due to clear function\n");
		p->os_func.os_signal(p->cfg_wait);
		return 0;
		}

	tqe = (struct txq_entry_t *)p->os_func.os_malloc_atomic(sizeof(struct txq_entry_t));
	if (tqe == NULL){
		PRINT_ER("Failed to allocate memory\n");
		return 0;
		}

	tqe->type = NMI_CFG_PKT;
	tqe->buffer = buffer;
	tqe->buffer_size = buffer_size;
	tqe->tx_complete_func = NULL;
	tqe->priv = NULL;
#ifdef TCP_ACK_FILTER
	tqe->tcp_PendingAck_index=NOT_TCP_ACK;
#endif
	/**
		Configuration packet always at the front
	**/
	PRINT_D(TX_DBG,"Adding the config packet at the Queue tail\n");

	/*Edited by Amr - BugID_4720*/
	if(nmi_wlan_txq_add_to_head(tqe))
		return 0;
	//nmi_wlan_txq_add_to_tail(tqe);
	return 1;
}

static int nmi_wlan_txq_add_net_pkt(void *priv, uint8_t *buffer, uint32_t buffer_size, nmi_tx_complete_func_t func)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	struct txq_entry_t *tqe;

	if (p->quit)
		return 0;

	tqe = (struct txq_entry_t *)p->os_func.os_malloc_atomic(sizeof(struct txq_entry_t));

	if (tqe == NULL)
		return 0;
	tqe->type = NMI_NET_PKT;
	tqe->buffer = buffer;
	tqe->buffer_size = buffer_size;
	tqe->tx_complete_func = func;
	tqe->priv = priv;

	PRINT_D(TX_DBG,"Adding mgmt packet at the Queue tail\n");	
#ifdef TCP_ACK_FILTER
	tqe->tcp_PendingAck_index=NOT_TCP_ACK;
	tcp_process(tqe);
#endif
	nmi_wlan_txq_add_to_tail(tqe);
	/*return number of itemes in the queue*/
	return p->txq_entries;
}
/*Bug3959: transmitting mgmt frames received from host*/
#if defined(NMI_AP_EXTERNAL_MLME) || defined(NMI_P2P)
int nmi_wlan_txq_add_mgmt_pkt(void *priv, uint8_t *buffer, uint32_t buffer_size, nmi_tx_complete_func_t func)
{
	
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	struct txq_entry_t *tqe;

	if (p->quit)
		return 0;

	tqe = (struct txq_entry_t *)p->os_func.os_malloc_atomic(sizeof(struct txq_entry_t));

	if (tqe == NULL)
		return 0;
	tqe->type = NMI_MGMT_PKT;
	tqe->buffer = buffer;
	tqe->buffer_size = buffer_size;
	tqe->tx_complete_func = func;
	tqe->priv = priv;
#ifdef TCP_ACK_FILTER
	tqe->tcp_PendingAck_index=NOT_TCP_ACK;
#endif
	PRINT_D(TX_DBG,"Adding Network packet at the Queue tail\n");	
	nmi_wlan_txq_add_to_tail(tqe);
	return 1;
}

#ifdef NMI_FULLY_HOSTING_AP
int nmi_FH_wlan_txq_add_net_pkt(void *priv, uint8_t *buffer, uint32_t buffer_size, nmi_tx_complete_func_t func)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	struct txq_entry_t *tqe;

	if (p->quit)
		return 0;

	tqe = (struct txq_entry_t *)p->os_func.os_malloc_atomic(sizeof(struct txq_entry_t));

	if (tqe == NULL)
		return 0;
	tqe->type = NMI_FH_DATA_PKT;
	tqe->buffer = buffer;
	tqe->buffer_size = buffer_size;
	tqe->tx_complete_func = func;
	tqe->priv = priv;
	PRINT_D(TX_DBG,"Adding mgmt packet at the Queue tail\n");	
	nmi_wlan_txq_add_to_tail(tqe);
	/*return number of itemes in the queue*/
	return p->txq_entries;
}
#endif	/* NMI_FULLY_HOSTING_AP*/
#endif /*NMI_AP_EXTERNAL_MLME*/
static struct txq_entry_t *nmi_wlan_txq_get_first(void)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	struct txq_entry_t *tqe;
	unsigned long flags;

	/*Added by Amr - BugID_4720*/
	p->os_func.os_spin_lock(p->txq_spinlock, &flags);

	//p->os_func.os_enter_cs(p->txq_lock);
	tqe = p->txq_head;

	/*Added by Amr - BugID_4720*/
	p->os_func.os_spin_unlock(p->txq_spinlock, &flags);
	
	//p->os_func.os_leave_cs(p->txq_lock);

	return tqe;
}

static struct txq_entry_t *nmi_wlan_txq_get_next(struct txq_entry_t *tqe)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	unsigned long flags;
	/*Added by Amr - BugID_4720*/
	p->os_func.os_spin_lock(p->txq_spinlock,&flags);

	//p->os_func.os_enter_cs(p->txq_lock);
	tqe = tqe->next;

	/*Added by Amr - BugID_4720*/
	p->os_func.os_spin_unlock(p->txq_spinlock, &flags);
	
	//p->os_func.os_leave_cs(p->txq_lock);

	return tqe;
}
 	
static int nmi_wlan_rxq_add(struct rxq_entry_t *rqe)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;

	if (p->quit)
		return 0;

	p->os_func.os_enter_cs(p->rxq_lock);
	if (p->rxq_head == NULL) {
		PRINT_D(RX_DBG,"Add to Queue head\n");
		rqe->next = NULL;
		p->rxq_head = rqe;
		p->rxq_tail = rqe;
	} else {
		PRINT_D(RX_DBG,"Add to Queue tail\n");
		p->rxq_tail->next = rqe;
		rqe->next = NULL;
		p->rxq_tail = rqe;
	}
	p->rxq_entries+=1;
	PRINT_D(RX_DBG,"Number of queue entries: %d\n",p->rxq_entries);
	//printk("Number of queue entries: %d\n",p->rxq_entries);
	p->os_func.os_leave_cs(p->rxq_lock);
	return p->rxq_entries;
}

static struct rxq_entry_t *nmi_wlan_rxq_remove(void)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;

	PRINT_D(RX_DBG,"Getting rxQ element\n");
	if (p->rxq_head) {
		struct rxq_entry_t *rqe;

		p->os_func.os_enter_cs(p->rxq_lock);
		rqe = p->rxq_head;
		p->rxq_head = p->rxq_head->next;
		p->rxq_entries-=1;
		PRINT_D(RX_DBG,"RXQ entries decreased\n");
		p->os_func.os_leave_cs(p->rxq_lock);
		return rqe;		
	}
	PRINT_D(RX_DBG,"Nothing to get from Q\n");	
	return NULL;
}


/********************************************

	Power Save handle functions 

********************************************/

static CHIP_PS_STATE_T genuChipPSstate = CHIP_WAKEDUP;

#ifdef NMI_OPTIMIZE_SLEEP_INT
	
INLINE void chip_allow_sleep()
{
	uint32_t reg=0;

	/* Clear bit 1 */
	g_wlan.hif_func.hif_read_reg(0xf0, &reg);
	
	g_wlan.hif_func.hif_write_reg(0xf0, reg & ~(1 << 0));
}

INLINE void chip_wakeup(void)
{
	uint32_t reg, clk_status_reg, trials=0; 
	uint32_t sleep_time;
	
	if ((g_wlan.io_func.io_type & 0x1) == HIF_SPI) 
	{
		do
		{
			g_wlan.hif_func.hif_read_reg(1, &reg);
			/* Set bit 1 */
			g_wlan.hif_func.hif_write_reg(1, reg | (1 << 1));
			
			/* Clear bit 1*/
			g_wlan.hif_func.hif_write_reg(1, reg & ~(1 << 1));
			
			do
			{
				/* Wait for the chip to stabilize*/
				NMI_Sleep(2);			
				// Make sure chip is awake. This is an extra step that can be removed
				// later to avoid the bus access overhead
				if((nmi_get_chipid(NMI_TRUE) == 0))
				{
					nmi_debug(N_ERR, "Couldn't read chip id. Wake up failed\n");
				}
			}while((nmi_get_chipid(NMI_TRUE) == 0) && ((++trials %3) == 0));
		
		}while(nmi_get_chipid(NMI_TRUE) == 0);
	}
	else if ((g_wlan.io_func.io_type & 0x1) == HIF_SDIO) 
	{
		g_wlan.hif_func.hif_read_reg(0xf0, &reg);	
		do
		{
			/* Set bit 1 */
			g_wlan.hif_func.hif_write_reg(0xf0, reg | (1 << 0));

			// Check the clock status
			g_wlan.hif_func.hif_read_reg(0xf1, &clk_status_reg);
			
			// in case of clocks off, wait 2ms, and check it again.
			// if still off, wait for another 2ms, for a total wait of 6ms.
			// If still off, redo the wake up sequence
			while( ((clk_status_reg & 0x1) == 0) && (((++trials) %3) == 0))
			{
				/* Wait for the chip to stabilize*/
				NMI_Sleep(2);			

				// Make sure chip is awake. This is an extra step that can be removed
				// later to avoid the bus access overhead
				g_wlan.hif_func.hif_read_reg(0xf1, &clk_status_reg);
				
				if((clk_status_reg & 0x1) == 0)
				{
					nmi_debug(N_ERR, "clocks still OFF. Wake up failed\n");
				}
			}
			// in case of failure, Reset the wakeup bit to introduce a new edge on the next loop
			if((clk_status_reg & 0x1) == 0)
			{
				// Reset bit 0
				g_wlan.hif_func.hif_write_reg(0xf0, reg & (~ (1 << 0)));
			}
		}while((clk_status_reg & 0x1) == 0);
	}


	if(genuChipPSstate == CHIP_SLEEPING_MANUAL)
	{
		g_wlan.hif_func.hif_read_reg(0x1C0C, &reg);
		reg &= ~(1<<0);
		g_wlan.hif_func.hif_write_reg(0x1C0C, reg);	

		if(nmi_get_chipid(NMI_FALSE) >= 0x1002a0)
		{
			/* Switch PMU clock back to external */
			g_wlan.hif_func.hif_read_reg(0x1e48, &reg);
			reg &= ~(1<<31);
			g_wlan.hif_func.hif_write_reg(0x1e48, reg);
		}	
	}
	genuChipPSstate = CHIP_WAKEDUP;
}
#else
INLINE void chip_wakeup(void)
{
	uint32_t reg, trials=0; 
	do
	{
		if ((g_wlan.io_func.io_type & 0x1) == HIF_SPI) 
		{
			g_wlan.hif_func.hif_read_reg(1, &reg) ;
			/* Make sure bit 1 is 0 before we start. */
			g_wlan.hif_func.hif_write_reg(1, reg & ~(1 << 1));
			/* Set bit 1 */
			 g_wlan.hif_func.hif_write_reg(1, reg | (1 << 1)) ;
			/* Clear bit 1*/
			 g_wlan.hif_func.hif_write_reg(1, reg  & ~(1 << 1));
		}
		else if ((g_wlan.io_func.io_type & 0x1) == HIF_SDIO) 
		{
			/* Make sure bit 0 is 0 before we start. */
			 g_wlan.hif_func.hif_read_reg(0xf0, &reg);
			 g_wlan.hif_func.hif_write_reg(0xf0, reg & ~(1 << 0));
			/* Set bit 1 */
			 g_wlan.hif_func.hif_write_reg(0xf0, reg | (1 << 0));
			/* Clear bit 1 */
			 g_wlan.hif_func.hif_write_reg(0xf0, reg  & ~(1 << 0));  
		}

		do
		{
			/* Wait for the chip to stabilize*/
			NMI_Sleep(2);			
			
			// Make sure chip is awake. This is an extra step that can be removed
			// later to avoid the bus access overhead
			if((nmi_get_chipid(NMI_TRUE) == 0))
			{
				nmi_debug(N_ERR, "Couldn't read chip id. Wake up failed\n");
			}
		}while((nmi_get_chipid(NMI_TRUE) == 0) && ((++trials %3) == 0));
		
	}while(nmi_get_chipid(NMI_TRUE) == 0);

	if(genuChipPSstate == CHIP_SLEEPING_MANUAL)
		{
			g_wlan.hif_func.hif_read_reg(0x1C0C, &reg);
			reg &= ~(1<<0);
			g_wlan.hif_func.hif_write_reg(0x1C0C, reg);	

			if(nmi_get_chipid(NMI_FALSE) >= 0x1002a0)
			{
				/* Switch PMU clock back to external */
				g_wlan.hif_func.hif_read_reg(0x1e48, &reg);
				reg &= ~(1<<31);
				g_wlan.hif_func.hif_write_reg(0x1e48, reg);
			}	
		}
	genuChipPSstate = CHIP_WAKEDUP;
}
#endif
void chip_sleep_manually(NMI_Uint32 u32SleepTime)
{
	uint32_t val32;

	if(genuChipPSstate != CHIP_WAKEDUP)
	{
		/* chip is already sleeping. Do nothing */
		return;
	}
#ifdef NMI_OPTIMIZE_SLEEP_INT
	chip_allow_sleep();
#endif

	g_wlan.os_func.os_enter_cs(g_wlan.hif_lock);

	if(nmi_get_chipid(NMI_FALSE) >= 0x1002a0)
	{
		// Switch PMU clock source to internal.
		 g_wlan.hif_func.hif_read_reg(0x1e48, &val32);
		val32 |= (1<<31);
		g_wlan.hif_func.hif_write_reg(0x1e48, val32);
	}
	
	g_wlan.hif_func.hif_write_reg(0x1C08, u32SleepTime);

	g_wlan.hif_func.hif_read_reg(0x1C0C, &val32);
	val32 |= (1<<0);
	g_wlan.hif_func.hif_write_reg(0x1C0C, val32);

	genuChipPSstate = CHIP_SLEEPING_MANUAL;
	g_wlan.os_func.os_leave_cs(g_wlan.hif_lock);

}


/********************************************

	Tx, Rx queue handle functions 

********************************************/

#ifdef NMC1000_SINGLE_TRANSFER
static int nmi_wlan_handle_txq(void)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	uint32_t reg, dma_addr, buf_offset;
	uint8_t *txb = p->tx_buffer;
	int tp_size, skip = 0, ret = 0;
	struct txq_entry_t *tqe;

	p->txq_exit = 0;
	do {

		if (p->quit)
			break;

		if (!skip) {
			tqe = nmi_wlan_txq_remove_from_head();
			if (tqe == NULL)
				break;
		}

		reg = 0;
		if (tqe->type == NMI_CFG_PKT) {
			buf_offset = tp_size = ETH_CONFIG_PKT_HDR_OFFSET;
			reg = (1<<2);
		} else {
			buf_offset = tp_size = ETH_ETHERNET_HDR_OFFSET;
		}
		tp_size += tqe->buffer_size;
		if (tp_size & 0x3) {
			tp_size += 4;
			tp_size &= ~0x3;
		}

		p->os_func.os_enter_cs(p->hif_lock);
		reg |= (1<<1);
		ret = p->hif_func.hif_write_reg(NMI_HOST_VMM_CTL, reg);
		if (!ret) {
			nmi_debug(N_ERR, "[nmi txq]: fail can't write reg host_vmm_ctl..\n");
			break;
			p->os_func.os_leave_cs(p->hif_lock);
		}

		do {
			ret = p->hif_func.hif_read_reg(NMI_HOST_VMM_CTL, &reg);
			if (!ret) {
				nmi_debug(N_ERR, "[nmi txq]: fail can't read reg host_vmm_ctl..\n");
				break;
			}
			if (!(reg & 0x2)) {
				ret = p->hif_func.hif_read_reg(NMI_VMM_TX_TBL_BASE, &dma_addr);
				break;
			} 
		} while(1);


		if (!ret) {
			nmi_debug(N_ERR, "[nmi txq]: fail can't read reg vmm_tx_tbl_base...\n");
			p->os_func.os_leave_cs(p->hif_lock);
			break;
		}
		p->os_func.os_leave_cs(p->hif_lock);
		
		if (dma_addr != 0) {
			uint32_t header;

			header = (tqe->type << 31)|(tqe->buffer_size << 15)|(tp_size);
#ifdef BIG_ENDIAN
			header = BYTE_SWAP(header);
#endif
			memcpy(txb, &header, 4);
			memcpy(&txb[buf_offset], tqe->buffer, tqe->buffer_size);

			p->os_func.os_enter_cs(p->hif_lock);
			if ((p->io_func.io_type & 0x1) == HIF_SPI) {
				if (tqe->type) {
					dma_addr += NMI_AHB_DATA_MEM_BASE;
				} else {
					dma_addr += NMI_AHB_SHARE_MEM_BASE;
				}
				ret = p->hif_func.hif_block_tx(dma_addr, txb, tp_size);
				if (ret) {
					/**
						done, interrupt firmware
					**/
					reg = dma_addr << 2;
					reg |= (1 << 1);
					ret = p->hif_func.hif_write_reg(NMI_HOST_TX_CTRL, reg);
				}
			} else if ((p->io_func.io_type & 0x1) == HIF_SDIO) {
				ret = p->hif_func.hif_block_tx(0, txb, tp_size);
			}


			p->os_func.os_leave_cs(p->hif_lock);
			if (ret) {
				tqe->status = 1;
				if (tqe->tx_complete_func)
					tqe->tx_complete_func(tqe->priv, tqe->status);
				p->os_func.os_free(tqe);
				skip = 0;
			} else {
				nmi_debug(N_ERR, "[nmi txq]: fail block tx...\n");
				break;
			}
		} else {
			p->os_func.os_sleep(30);	/* wait 30 ms and start over */
			skip = 1;
		}
	} while (1);

	p->txq_exit = 1;	
	return ret;
}
#else


static int nmi_wlan_handle_txq(uint32_t* pu32TxqCount)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	int i, entries = 0;
	uint32_t sum;
	uint32_t reg;
	uint8_t *txb = p->tx_buffer;
	uint32_t offset = 0;
	int vmm_sz = 0;
	struct txq_entry_t *tqe;
	int ret = 0;
	uint32_t vmm_table[NMI_VMM_TBL_SIZE];
	//printk("T");
	p->txq_exit = 0;
	do {
		if (p->quit)
			break;
		
		/*Added by Amr - BugID_4720*/
		p->os_func.os_wait(p->txq_add_to_head_lock, CFG_PKTS_TIMEOUT);
#ifdef	TCP_ACK_FILTER
		nmi_wlan_txq_filter_dup_tcp_ack();
#endif
		/**
			build the vmm list
		**/
		PRINT_D(TX_DBG,"Getting the head of the TxQ\n");
		tqe = nmi_wlan_txq_get_first();
		i = 0;
		sum = 0;
		do {
			//if ((tqe != NULL) && (i < (8)) &&
			//if ((tqe != NULL) && (i < (NMI_VMM_TBL_SIZE-1)) &&			
			if ((tqe != NULL) && (i < (NMI_VMM_TBL_SIZE-1)) /* reserve last entry to 0 */) 
			{

				if (tqe->type == NMI_CFG_PKT) {
					vmm_sz = ETH_CONFIG_PKT_HDR_OFFSET;
				}
				/*Bug3959: transmitting mgmt frames received from host*/
				/*vmm_sz will only be equal to tqe->buffer_size + 4 bytes (HOST_HDR_OFFSET)*/
				/* in other cases NMI_MGMT_PKT and NMI_DATA_PKT_MAC_HDR*/
				else if (tqe->type == NMI_NET_PKT){
					vmm_sz = ETH_ETHERNET_HDR_OFFSET;
				}
#ifdef NMI_FULLY_HOSTING_AP
				else if (tqe->type == NMI_FH_DATA_PKT)
				{
					vmm_sz = FH_TX_HOST_HDR_OFFSET;
				}
#endif
#ifdef NMI_AP_EXTERNAL_MLME
				else
					{
						vmm_sz = HOST_HDR_OFFSET;
					}
#endif
				vmm_sz += tqe->buffer_size;
				PRINT_D(TX_DBG,"VMM Size before alignment = %d\n",vmm_sz);
				if (vmm_sz & 0x3) {													/* has to be word aligned */
					vmm_sz = (vmm_sz + 4) & ~0x3;
				}
				if((sum+vmm_sz) > p->tx_buffer_size) {
					break;
				}
				PRINT_D(TX_DBG,"VMM Size AFTER alignment = %d\n",vmm_sz);
				vmm_table[i] = vmm_sz/4;										/* table take the word size */
				PRINT_D(TX_DBG,"VMMTable entry size = %d\n",vmm_table[i]);				
				
				if (tqe->type == NMI_CFG_PKT){
					vmm_table[i] |= (1 << 10);
					PRINT_D(TX_DBG,"VMMTable entry changed for CFG packet = %d\n",vmm_table[i]);									
					}
#ifdef BIG_ENDIAN
				vmm_table[i] = BYTE_SWAP(vmm_table[i]);
#endif			
				//p->hif_func.hif_write_reg(0x1160,vmm_table[0]);
						
				//nmi_debug(N_TXQ, "[nmi txq]: vmm table[%d] = %08x\n", i, vmm_table[i]);
				i++;
				sum += vmm_sz;
				PRINT_D(TX_DBG,"sum = %d\n",sum);
				tqe = nmi_wlan_txq_get_next(tqe);
			} else {
				break;
			}
		} while (1);

		if (i == 0){		/* nothing in the queue */
			PRINT_D(TX_DBG,"Nothing in TX-Q\n");
			break;
		} else{
			PRINT_D(TX_DBG,"Mark the last entry in VMM table - number of previous entries = %d\n",i);
			vmm_table[i] = 0x0;	/* mark the last element to 0 */
		}
		p->os_func.os_enter_cs(p->hif_lock);
	#ifndef NMI_OPTIMIZE_SLEEP_INT	
		if(genuChipPSstate != CHIP_WAKEDUP)
	#endif	
		{
			chip_wakeup();
		}
		
		do {

			ret = p->hif_func.hif_read_reg(NMI_HOST_TX_CTRL, &reg);
			if (!ret) {
				nmi_debug(N_ERR, "[nmi txq]: fail can't read reg vmm_tbl_entry..\n");
				break;
			}
			
			if ((reg&0x1) == 0) {
				/**
					write to vmm table
				**/
				PRINT_D(TX_DBG,"Writing VMM table ... with Size = %d\n",((i+1)*4));
				break;
			} else {
				/**
					wait for vmm table is ready
				**/
				PRINT_WRN(GENERIC_DBG, "[nmi txq]: warn, vmm table not clear yet, wait... \n");
				p->os_func.os_leave_cs(p->hif_lock);
				p->os_func.os_sleep(3);	/* wait 3 ms */
				p->os_func.os_enter_cs(p->hif_lock);
		#ifndef NMI_OPTIMIZE_SLEEP_INT
				if(genuChipPSstate != CHIP_WAKEDUP)
				{
					chip_wakeup();
				}
		#endif
			}
		} while (!p->quit);

		if(!ret) {
			goto _end_;
		}
		int timeout = 200;
		do {

			/**
			write to vmm table 
			**/
			ret = p->hif_func.hif_block_tx(NMI_VMM_TBL_RX_SHADOW_BASE, (uint8_t *)vmm_table, ((i+1)*4)); /* Bug 4477 fix */
			if (!ret) {
				nmi_debug(N_ERR, "ERR block TX of VMM table.\n");
				break;				
			}


			/**
			interrupt firmware
			**/
			ret = p->hif_func.hif_write_reg(NMI_HOST_VMM_CTL, 0x2);	
			if (!ret) {
				nmi_debug(N_ERR, "[nmi txq]: fail can't write reg host_vmm_ctl..\n");
				break;
			}

			/**
				wait for confirm...
			**/

			do {
				ret = p->hif_func.hif_read_reg(NMI_HOST_VMM_CTL, &reg);
				if (!ret) {
					nmi_debug(N_ERR, "[nmi txq]: fail can't read reg host_vmm_ctl..\n");
					break;
				}
				if ((reg>>2)&0x1) {
					/**
						Get the entries
					**/
					entries = ((reg>>3)&0x3f);
					//entries = ((reg>>3)&0x2f);
					break;
				} else{
					p->os_func.os_leave_cs(p->hif_lock);
					p->os_func.os_sleep(3);	/* wait 3 ms */
					p->os_func.os_enter_cs(p->hif_lock);
				#ifndef NMI_OPTIMIZE_SLEEP_INT
					if(genuChipPSstate != CHIP_WAKEDUP)
					{
						chip_wakeup();
					}
				#endif
					PRINT_WRN(GENERIC_DBG, "Can't get VMM entery - reg = %2x\n",reg);
				}
			} while (--timeout);
			if(timeout <= 0)
			{
				ret = p->hif_func.hif_write_reg(NMI_HOST_VMM_CTL, 0x0);
				break;
			}

			if (!ret) {
				break;
			}

			if (entries == 0) {
				PRINT_WRN(GENERIC_DBG, "[nmi txq]: no more buffer in the chip (reg: %08x), retry later [[ %d, %x ]] \n",reg, i, vmm_table[i-1]);

				/* undo the transaction. */
				ret = p->hif_func.hif_read_reg(NMI_HOST_TX_CTRL, &reg);
				if (!ret) {
					nmi_debug(N_ERR, "[nmi txq]: fail can't read reg NMI_HOST_TX_CTRL..\n");
					break;
				}
				reg &= ~(1ul << 0);
				ret = p->hif_func.hif_write_reg(NMI_HOST_TX_CTRL, reg);
				if (!ret) {
					nmi_debug(N_ERR, "[nmi txq]: fail can't write reg NMI_HOST_TX_CTRL..\n");
					break;
				}
				break;
			} else {
				break;
			}
		} while (1);

		if (!ret) {
			goto _end_;
		}
		if(entries == 0) {
			ret = NMI_TX_ERR_NO_BUF;
			goto _end_;
		}

		/* since copying data into txb takes some time, then 
		allow the bus lock to be released let the RX task go. */
		p->os_func.os_leave_cs(p->hif_lock); 

		/**
			Copy data to the TX buffer
		**/
		offset = 0;
		i = 0;
		do {
			tqe = nmi_wlan_txq_remove_from_head();
			if (tqe != NULL && (vmm_table[i] != 0)) {
				uint32_t header, buffer_offset;

#ifdef BIG_ENDIAN
				vmm_table[i] = BYTE_SWAP(vmm_table[i]);
#endif
				vmm_sz = (vmm_table[i] & 0x3ff);	/* in word unit */
				vmm_sz *= 4;
				header = (tqe->type << 31)|(tqe->buffer_size<<15)|vmm_sz;
				/*Bug3959: transmitting mgmt frames received from host*/
				/*setting bit 30 in the host header to indicate mgmt frame*/
#ifdef NMI_AP_EXTERNAL_MLME				
				if(tqe->type == NMI_MGMT_PKT)
				{
					header |= (1<< 30);
				}
#endif
				/*else if(tqe->type == NMI_DATA_PKT_MAC_HDR)
				{
					header |= (1<< 29);
				}*/
				//nmi_debug(N_TXQ, "[nmi txq]: header (%08x), real size (%d), vmm size (%d)\n", header, tqe->buffer_size, vmm_sz);

#ifdef BIG_ENDIAN
				header = BYTE_SWAP(header);
#endif
				memcpy(&txb[offset], &header, 4);
				if (tqe->type == NMI_CFG_PKT) {
					buffer_offset = ETH_CONFIG_PKT_HDR_OFFSET;
				} 
				/*Bug3959: transmitting mgmt frames received from host*/
				/*buffer offset = HOST_HDR_OFFSET in other cases: NMI_MGMT_PKT*/
				/* and NMI_DATA_PKT_MAC_HDR*/
				else if (tqe->type == NMI_NET_PKT){
					buffer_offset = ETH_ETHERNET_HDR_OFFSET;
				}
#ifdef NMI_FULLY_HOSTING_AP
				else if (tqe->type == NMI_FH_DATA_PKT)
				{
					buffer_offset = FH_TX_HOST_HDR_OFFSET;
				}
#endif
				else{
					buffer_offset = HOST_HDR_OFFSET;
					}
				memcpy(&txb[offset+buffer_offset], tqe->buffer, tqe->buffer_size);
				offset += vmm_sz;				
				i++;
				tqe->status = 1;				/* mark the packet send */
				if (tqe->tx_complete_func) 
					tqe->tx_complete_func(tqe->priv, tqe->status);
				#ifdef TCP_ACK_FILTER
				if(tqe->tcp_PendingAck_index != NOT_TCP_ACK)
				{
					Pending_Acks_info[tqe->tcp_PendingAck_index].txqe=NULL;
				}
				#endif
				p->os_func.os_free(tqe);
			} else {
				break;
			}
		} while (--entries);

		/**
			lock the bus
		**/
		//PRINT_D(GENERIC_DBG,"Locking hif_lock\n");
		p->os_func.os_enter_cs(p->hif_lock);
	#ifndef NMI_OPTIMIZE_SLEEP_INT	
		if(genuChipPSstate != CHIP_WAKEDUP)
		{
			chip_wakeup();
		}
	#endif //NMI_OPTIMIZE_SLEEP_INT
#if DMA_VER == DMA_VER_2
	if(g_wlan.use_dma_v2) {
			ret = p->hif_func.hif_clear_int_ext(ENABLE_TX_VMM);
			if (!ret) {
				nmi_debug(N_ERR, "[nmi txq]: fail can't start tx VMM ...\n");
				goto _end_;
			}

			/**
				transfer
			**/
			ret = p->hif_func.hif_block_tx_ext(0, txb, offset);
			if(!ret) {
				nmi_debug(N_ERR, "[nmi txq]: fail can't block tx ext...\n");
				goto _end_;
			}

	} else 
#endif /* DMA_VER == DMA_VER_2 */
	{

	/**
				enable table 0 
			**/
			reg = 0x1;
			ret = p->hif_func.hif_write_reg(NMI_VMM_TBL_CTL, reg);
			if (!ret) {
				nmi_debug(N_ERR, "[nmi txq]: fail can't write reg vmm_tbl_ctl...\n");
				goto _end_;
			}

			/**
				enable core
			**/
	#ifdef OLD_FPGA_BITFILE
			ret = p->hif_func.hif_read_reg(NMI_VMM_CORE_CTL, &reg);
			if (!ret) {
				nmi_debug(N_ERR, "[nmi txq]: fail can't read reg vmm_core_ctl...\n");
				goto _end_;
			}
			reg |= 0x1;		
			ret = p->hif_func.hif_write_reg(NMI_VMM_CORE_CTL, reg);
			if (!ret) {
				nmi_debug(N_ERR, "[nmi txq]: fail can't write reg vmm_core_ctl...\n");
				goto _end_;
			}
	#else
			reg = 0x1;
			ret = p->hif_func.hif_write_reg(NMI_VMM_CORE_CTL, reg);
			if (!ret) {
				nmi_debug(N_ERR, "[nmi txq]: fail can't write reg vmm_core_ctl...\n");
				goto _end_;
			}
	#endif

			/**
				transfer
			**/
	
			ret = p->hif_func.hif_block_tx(0, txb, offset);
			if (!ret) 
				nmi_debug(N_ERR, "[nmi txq]: fail can't block tx...\n");
	}
_end_:	 
	#ifdef NMI_OPTIMIZE_SLEEP_INT
		chip_allow_sleep();
	#endif
		p->os_func.os_leave_cs(p->hif_lock);
		if (ret != 1)
			break;
	} while(0);
	//remove_TCP_related();
	/*Added by Amr - BugID_4720*/
	p->os_func.os_signal(p->txq_add_to_head_lock);

	p->txq_exit = 1;
	PRINT_D(TX_DBG,"THREAD: Exiting txq\n");
	//return tx[]q count
	*pu32TxqCount = p->txq_entries;
	return ret;
}
#endif

static void nmi_wlan_handle_rxq(void)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	int offset = 0, size, has_packet = 0;
	uint8_t *buffer;
	struct rxq_entry_t *rqe;

	p->rxq_exit = 0;

	
	
	
	do {
//		printk("[%s:%d]in 1st do-while\n",__FUNCTION__,__LINE__);
		if (p->quit){
			PRINT_D(RX_DBG,"exit 1st do-while due to Clean_UP function \n");
			p->os_func.os_signal(p->cfg_wait);
			break;
		}
		rqe = nmi_wlan_rxq_remove();
		if (rqe == NULL){
			PRINT_D(RX_DBG,"nothing in the queue - exit 1st do-while\n");
			break;
		}
		buffer = rqe->buffer;
		size = rqe->buffer_size;
		PRINT_D(RX_DBG,"rxQ entery Size = %d - Address = %p\n",size,buffer);		
		offset = 0;
	
		
		
		do {
			uint32_t header;
			int i;
			uint32_t pkt_len, pkt_offset, tp_len;
			int is_cfg_packet;
			PRINT_D(RX_DBG,"In the 2nd do-while\n");
			memcpy(&header, &buffer[offset], 4);
#ifdef BIG_ENDIAN
			header = BYTE_SWAP(header);
#endif
			PRINT_D(RX_DBG,"Header = %04x - Offset = %d\n",header,offset);

			
			
			is_cfg_packet = (header >> 31) & 0x1;
			pkt_offset = (header >> 22) & 0x1ff;
			tp_len = (header >> 11)&0x7ff; 			
			pkt_len = header & 0x7ff;
			
			if (pkt_len == 0 || tp_len == 0) {
				nmi_debug(N_RXQ, "[nmi rxq]: data corrupt, packet len or tp_len is 0 [%d][%d]\n", pkt_len, tp_len);
				break;
			}

/*bug 3887: [AP] Allow Management frames to be passed to the host*/
			#if defined(NMI_AP_EXTERNAL_MLME) || defined(NMI_P2P)
			#define IS_MANAGMEMENT 				0x100
			#define IS_MANAGMEMENT_CALLBACK 		0x080
			#define IS_MGMT_STATUS_SUCCES			0x040

			
			if(pkt_offset & IS_MANAGMEMENT)
				{
					//PRINT_D(GENERIC_DBG,"Mgmt FRAME Received at host--\n\n");
					//reset mgmt indicator bit, to use pkt_offeset in furthur calculations
					pkt_offset &= ~(IS_MANAGMEMENT | IS_MANAGMEMENT_CALLBACK | IS_MGMT_STATUS_SUCCES);
					
#ifdef USE_WIRELESS
					NMI_WFI_mgmt_rx(&buffer[offset+HOST_HDR_OFFSET],pkt_len);
					
#endif
						
				}
			
			//BUG4530 fix
			else 
			#endif
		{			
			//nmi_debug(N_RXQ, "[nmi rxq]: packet, tp len(%d), len (%d), offset (%d), cfg (%d)\n", tp_len, pkt_len, pkt_offset, is_cfg_packet);
					
			if (!is_cfg_packet) {
				
				if (p->net_func.rx_indicate) {
					if (pkt_len > 0) {
						p->net_func.rx_indicate(&buffer[pkt_offset+offset], pkt_len);
						has_packet = 1;
					}
				}
			} else {
				nmi_cfg_rsp_t rsp;
				
				
				
				p->cif_func.rx_indicate(&buffer[pkt_offset+offset], pkt_len, &rsp);
				if (rsp.type == NMI_CFG_RSP) {
					/**
						wake up the waiting task...
					**/
				PRINT_D(RX_DBG,"p->cfg_seq_no = %d - rsp.seq_no = %d\n",p->cfg_seq_no,rsp.seq_no);
					if (p->cfg_seq_no == rsp.seq_no) {
						//PRINT_D(GENERIC_DBG, "Unlocking cfg_wait\n");
						p->os_func.os_signal(p->cfg_wait);
					}
					//p->os_func.os_signal(p->cfg_wait);	
				} else if (rsp.type == NMI_CFG_RSP_STATUS) {
					/**
						Call back to indicate status...
					**/
					if (p->indicate_func.mac_indicate) {
						p->indicate_func.mac_indicate(NMI_MAC_INDICATE_STATUS);
					}
					
				} else if (rsp.type == NMI_CFG_RSP_SCAN) {
					if (p->indicate_func.mac_indicate)
						p->indicate_func.mac_indicate(NMI_MAC_INDICATE_SCAN);
				}
			}
		}
			offset += tp_len;
			if (offset >= size)
				break;
		} while (1);
	

#ifndef MEMORY_STATIC
		if (buffer != NULL)
			p->os_func.os_free((void *)buffer); 
#endif
		if (rqe != NULL)
			p->os_func.os_free((void *)rqe);

		if (has_packet) {
			if (p->net_func.rx_complete)
				p->net_func.rx_complete();
		}
	} while(1);

	p->rxq_exit = 1;
	PRINT_D(RX_DBG,"THREAD: Exiting RX thread \n");
	return;
}

/********************************************

	Isr

********************************************/
static void nmi_unknown_isr(void){
	g_wlan.hif_func.hif_clear_int();
	linux_wlan_enable_irq();
	g_wlan.os_func.os_leave_cs(g_wlan.hif_lock);		
}
static void nmi_pllupdate_isr(uint32_t int_stats1){
#if (defined NMI_SDIO) && (!defined NMI_SDIO_IRQ_GPIO)

	
	//g_wlan.hif_func.hif_write_reg(NMI_INT_STATS,(int_stats1 & ~PLL_INT));	
	g_wlan.hif_func.hif_write_reg(NMI_INT_STATS,0);	
	
	/* Waiting for PLL */
	g_wlan.os_func.os_atomic_sleep(NMI_PLL_TO);

	if(!(int_stats1 & DATA_INT)){
		g_wlan.os_func.os_leave_cs(g_wlan.hif_lock);	
	}
	else	{
		printk("[PLL]Pending Data wait\n");
	}
#else
	uint32_t int_stats;
	#ifndef NMI_SDIO
	int trials= 10;
	#endif
	
	if((!(int_stats1 & DATA_INT)) &&  (!(int_stats1 & SLEEP_INT))){
		g_wlan.hif_func.hif_clear_int();
		linux_wlan_enable_irq();
	}
	g_wlan.hif_func.hif_read_reg(NMI_INT_STATS,&int_stats);	
	g_wlan.hif_func.hif_write_reg(NMI_INT_STATS,(int_stats & ~PLL_INT));	
	
	/* Waiting for PLL */
	g_wlan.os_func.os_atomic_sleep(NMI_PLL_TO);

#ifndef NMI_SDIO
	//poll till read a valid data
	while(!(ISNMC1000(nmi_get_chipid(NMI_TRUE))&&--trials)) {
		printk("SPI retrying\n");
		g_wlan.os_func.os_atomic_sleep(1);		
	}
#endif	

	if((!(int_stats1 & DATA_INT)) &&  (!(int_stats1 & SLEEP_INT))){
		g_wlan.os_func.os_leave_cs(g_wlan.hif_lock);	
	}
	else
		{printk("[PLL]Pending Data wait\n");}
#endif

}

static void nmi_sleeptimer_isr(uint32_t int_stats1){
	uint32_t int_stats;

	
	g_wlan.hif_func.hif_clear_int();
	linux_wlan_enable_irq();

	g_wlan.hif_func.hif_read_reg(NMI_INT_STATS,&int_stats);	
	g_wlan.hif_func.hif_write_reg(NMI_INT_STATS,(int_stats & ~SLEEP_INT));	
	/* Wait 1ms tomake sure chip went into sleep
	   Accessing the clockless registers in the transit state fails */
	if(nmi_get_chipid(NMI_FALSE) < 0x1000D0)
	{
		NMI_SleepMicrosec(1200); // failed at 1 ms
	}
#ifndef NMI_OPTIMIZE_SLEEP_INT
	genuChipPSstate = CHIP_SLEEPING_AUTO;
#endif	

	g_wlan.os_func.os_leave_cs(g_wlan.hif_lock);	
}

#ifdef NMC1000_SINGLE_TRANSFER
static void nmi_wlan_handle_isr(uint32_t int_status1)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
#ifdef MEMORY_STATIC
	uint32_t offset = p->rx_buffer_offset;
#endif
	uint8_t *buffer = NULL;
	uint32_t size, ctl0;
	int ret;
	struct rxq_entry_t *rqe;

	/**
		clear the chip's interrupt		
	**/
	int_stats1 &= ~(DATA_INT);
	if(p->hif_func.hif_write_reg(NMI_INT_STATS,int_stats1)!= 1)
		
	if(!(int_stats1 & SLEEP_INT)){
		PRINT_D(RX_DBG,"Clearing RX interrupt\n");
		p->hif_func.hif_clear_int();
		linux_wlan_enable_irq();
	}

	/**
		read the rx size
	**/
	ret = p->hif_func.hif_read_reg(NMI_HOST_RX_CTRL_0, &ctl0);
	size = (ctl0 >> 2) & 0xfff;

	if (!ret) {
		nmi_debug(N_ERR, "[nmi isr]: fail can't read reg host_rx_ctrl_0...\n");
		p->os_func.os_leave_cs(p->hif_lock);
		return;
	} 

	if (size > 0) {
		uint32_t address = 0;

#ifdef MEMORY_STATIC
		if (p->rx_buffer_size - offset < (2*1024)/*size*/)
			offset = 0;
		buffer = &p->rx_buffer[offset];
#else
		buffer = p->os_func.os_malloc(2*1024/*size*/);
		if (buffer == NULL) {
			nmi_debug(N_ERR, "[nmi isr]: fail alloc host memory...drop the packets (%d)\n", size);
			p->os_func.os_leave_cs(p->hif_lock);
			return;
		}
#endif

	
		/**
			start bus transfer
		**/
		if ((p->io_func.io_type & 0x1) == HIF_SPI) {
			/**
				read the spi dam address
			**/
			ret = p->hif_func.hif_read_reg(NMI_HOST_RX_CTRL_1, &address);
			if (!ret) {
				nmi_debug(N_ERR, "[nmi isr]: fail can't read reg host_rx_ctrl_1...\n");
				goto _end_;
			}
			if (address & 0x1) {
				address = NMI_AHB_DATA_MEM_BASE+(address&~0x1);
			} else {
				address = NMI_AHB_SHARE_MEM_BASE+address;	
			}
		} else if ((p->io_func.io_type & 0x1) == HIF_SDIO) {
			address = 0;
		}
		ret = p->hif_func.hif_block_rx(address, buffer, size);
 		if (!ret) {
			nmi_debug(N_ERR, "[nmi isr]: fail block rx...\n");
		} else {
			if ((p->io_func.io_type & 0x1) == HIF_SPI) {
				/**
					indicate rx done to firmware
				**/
				ctl0 |= (1<<1);
				ret = p->hif_func.hif_write_reg(NMI_HOST_RX_CTRL_0, ctl0);
			}
		}

_end_:
		if(!(int_stats1 & SLEEP_INT)){
			p->os_func.os_leave_cs(p->hif_lock);
		}
		if (ret) {
#ifdef MEMORY_STATIC
			offset += size;
			p->rx_buffer_offset = offset;
#endif
			/**
				add to rx queue
			**/
#if 0
			{
				BYTE *bt = (BYTE *)buffer;
				int i;

				nmi_debug(N_ERR, "*** Receive Packet ***\n"); 

				for (i = 0; i < size; i++) {
					if ((i!=0) &&((i%8)==0))
						nmi_debug(N_ERR, "\n", bt[i]);
					nmi_debug(N_ERR, "%02x ", bt[i]);
				}
				nmi_debug(N_ERR, "\n");
			}
#endif
			rqe = (struct rxq_entry_t *)p->os_func.os_malloc(sizeof(struct rxq_entry_t));
			if (rqe) {
				rqe->buffer = buffer;
				rqe->buffer_size = size;
				nmi_wlan_rxq_add(rqe);
				p->os_func.os_signal(p->rxq_wait);
			}
		} else {
#ifndef MEMORY_STATIC
			if (buffer != NULL)
				p->os_func.os_free(buffer);
#endif
		}
	}
}

#else
#if defined NMI_SDIO && !defined NMI_SDIO_IRQ_GPIO
static void nmi_wlan_handle_isr(uint32_t xfer_cnt, uint32_t int_stats1)
#else
#ifdef PLL_WORKAROUND
static void nmi_wlan_handle_isr(uint32_t int_stats1)
#else /* PLL_WORKAROUND */
static void nmi_wlan_handle_isr(void)
#endif /* PLL_WORKAROUND */
#endif
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
#ifdef MEMORY_STATIC
	uint32_t offset = p->rx_buffer_offset;
#endif
	uint8_t *buffer = NULL;
	uint32_t size, reg;
	int ret;
	struct rxq_entry_t *rqe;

	/**
		clear the chip's interrupt		
	**/
#if defined PLL_WORKAROUND && ( (!defined NMI_SDIO) || (defined NMI_SDIO_IRQ_GPIO))
	int_stats1 &= ~(DATA_INT);
	p->hif_func.hif_write_reg(NMI_INT_STATS,int_stats1);
	
	if(!(int_stats1 & SLEEP_INT)){
		p->hif_func.hif_clear_int();	
		linux_wlan_enable_irq();
	}

#elif (!defined PLL_WORKAROUND) &&  ( (!defined NMI_SDIO) || (defined NMI_SDIO_IRQ_GPIO))
	p->hif_func.hif_clear_int();	
	linux_wlan_enable_irq();
#endif


	/**
		read the rx size
	**/
	#if defined NMI_SDIO && !defined NMI_SDIO_IRQ_GPIO
		size = xfer_cnt;	
	#else
		ret = p->hif_func.hif_read_reg(NMI_VMM_TO_HOST_SIZE, &size);
		if (!ret) {
			nmi_debug(N_ERR, "[nmi isr]: fail can't read reg vmm_to_host_size...\n");
			goto _end_;
		} 
	#endif

	if (size > 0) {
#ifdef MEMORY_STATIC
		if (p->rx_buffer_size - offset < size)
			offset = 0;
		buffer = &p->rx_buffer[offset];
#else
		buffer = p->os_func.os_malloc(size);
		if (buffer == NULL) {
			nmi_debug(N_ERR, "[nmi isr]: fail alloc host memory...drop the packets (%d)\n", size);
			goto _end_;
		}
#endif

		/**
			lock bus, no interruption
		**/
		//p->os_func.os_enter_cs(p->hif_lock);

		/**
			enable vmm table 1
		**/
		reg = 0x2;
		ret = p->hif_func.hif_write_reg(NMI_VMM_TBL_CTL, reg);
		if (!ret) {
			nmi_debug(N_ERR, "[nmi isr]: fail write reg vmm_tbl_ctl...\n");
			goto _end_;
		}
		
		/**
			enable vmm transfer
		**/
#ifdef OLD_FPGA_BITFILE
		ret = p->hif_func.hif_read_reg(NMI_VMM_CORE_CTL, &reg);
		if (!ret) {
			nmi_debug(N_ERR, "[nmi isr]: fail read reg vmm_core_ctl...\n");
			goto _end_;
		}
		reg |= 0x1;		
		ret = p->hif_func.hif_write_reg(NMI_VMM_CORE_CTL, reg);
		if (!ret) {
			nmi_debug(N_ERR, "[nmi isr]: fail write reg vmm_core_ctl...\n");
			goto _end_;
		}
#else
		reg = 0x1;
		ret = p->hif_func.hif_write_reg(NMI_VMM_CORE_CTL, reg);
		if (!ret) {
			nmi_debug(N_ERR, "[nmi isr]: fail write reg vmm_core_ctl...\n");
			goto _end_;
		}

#endif

		ret = p->hif_func.hif_block_rx(0, buffer, size);

 		if (!ret) {
			nmi_debug(N_ERR, "[nmi isr]: fail block rx...\n");
			goto _end_;
		}
		
_end_:		
		/**
			unlock bus
		**/
#ifdef PLL_WORKAROUND
		if(!(int_stats1 & SLEEP_INT)){
			p->os_func.os_leave_cs(p->hif_lock);
		}
#else /* PLL_WORKAROUND */
		p->os_func.os_leave_cs(p->hif_lock);
#endif /* PLL_WORKAROUND */

		if (ret) {
#ifdef MEMORY_STATIC
			offset += size;
			p->rx_buffer_offset = offset;
#endif
			/**
				add to rx queue
			**/
			rqe = (struct rxq_entry_t *)p->os_func.os_malloc(sizeof(struct rxq_entry_t));
			if (rqe != NULL) {
				rqe->buffer = buffer;
				rqe->buffer_size = size;
				PRINT_D(RX_DBG,"rxq entery Size= %d - Address = %p\n",rqe->buffer_size,rqe->buffer);
				nmi_wlan_rxq_add(rqe);
				p->os_func.os_signal(p->rxq_wait);
			}
		} else {
#ifndef MEMORY_STATIC
			if (buffer != NULL)
				p->os_func.os_free(buffer);
#endif
		}
	}
}
#endif

#define UNKNOWN_INTERRUPT_WORKAROUND

void nmi_handle_isr_v1(void)
{
#if (defined NMI_SDIO) && (!defined NMI_SDIO_IRQ_GPIO)

		uint32_t int_status;
		uint32_t xfer_cnt = 0;		


		g_wlan.os_func.os_enter_cs(g_wlan.hif_lock);

		if(!g_wlan.hif_func.hif_clear_int()){
			printk("UNKNOWN INT\n");
			g_wlan.os_func.os_leave_cs(g_wlan.hif_lock);
			return;
		}
		xfer_cnt = sdio_xfer_cnt();

		if(xfer_cnt == 0){
			g_wlan.hif_func.hif_read_reg(NMI_INT_STATS,&int_status);
			if(int_status & PLL_INT){
				nmi_pllupdate_isr(int_status);
			}
			if(int_status & SLEEP_INT){
				nmi_sleeptimer_isr(int_status);
			}
		}
		if(xfer_cnt>0){
	#ifndef NMI_OPTIMIZE_SLEEP_INT
			/* Chip is up and talking*/
			genuChipPSstate = CHIP_WAKEDUP;
	#endif
			nmi_wlan_handle_isr(xfer_cnt, 0);
		}
#else /* ! NMI_SDIO */
	#ifdef PLL_WORKAROUND
	uint32_t int_status;
	g_wlan.os_func.os_enter_cs(g_wlan.hif_lock);
	g_wlan.hif_func.hif_read_reg(NMI_INT_STATS,&int_status);

	
	if(int_status & PLL_INT){
		nmi_pllupdate_isr(int_status);
	}
	if(int_status & DATA_INT){ 
	#ifndef NMI_OPTIMIZE_SLEEP_INT
		/* Chip is up and talking*/
		genuChipPSstate = CHIP_WAKEDUP;
	#endif
		nmi_wlan_handle_isr(int_status);
	}
	/* Sleep Int should be handled first because sometimes firmware sends both 
	interrupts, which caused chip to go to sleep while driver is getting data*/
	if(int_status & SLEEP_INT){
		nmi_sleeptimer_isr(int_status);
	}
	if(!(int_status & (PLL_INT | DATA_INT | SLEEP_INT | ABORT_INT)) ){
		printk(">> UNKNOWN_INTERRUPT - 0x%08x\n",int_status);
#ifdef UNKNOWN_INTERRUPT_WORKAROUND
	{
	#define TRIALS	2000
	
		uint32_t reread_trials = TRIALS;
		uint32_t vmm_size = 0;
		
		while(reread_trials-- && (int_status == 0) && (vmm_size == 0)){
			g_wlan.hif_func.hif_read_reg(NMI_INT_STATS,&int_status);
			g_wlan.hif_func.hif_read_reg(0x150010, &vmm_size);
			}

		if(int_status != 0 || vmm_size != 0){
			printk("Int Status Re-read successfully [Sts: %x - VMM_Size: %d - trials:%d]\n",int_status,vmm_size,(TRIALS - reread_trials));
			nmi_wlan_handle_isr(DATA_INT);
		}else{
			printk("> Int Status Re-Read FAILED: %x\n", int_status);
			nmi_unknown_isr();
		}

	}
#else
		nmi_unknown_isr();
#endif
	}

#else
	g_wlan.os_func.os_enter_cs(g_wlan.hif_lock);
	nmi_wlan_handle_isr();
#endif
#endif
}

#if DMA_VER == DMA_VER_2
/********************************************

	Fast DMA Isr

********************************************/
static void nmi_unknown_isr_ext(void){
	g_wlan.hif_func.hif_clear_int_ext(0);
}
static void nmi_pllupdate_isr_ext(uint32_t int_stats){

	int trials= 10;

	g_wlan.hif_func.hif_clear_int_ext(PLL_INT_CLR);	

	/* Waiting for PLL */
	g_wlan.os_func.os_atomic_sleep(NMI_PLL_TO);
	
	//poll till read a valid data
	while(!(ISNMC1000(nmi_get_chipid(NMI_TRUE))&&--trials)) {
		printk("PLL update retrying\n");
		g_wlan.os_func.os_atomic_sleep(1);		
	}
}

static void nmi_sleeptimer_isr_ext(uint32_t int_stats1)
{	
	g_wlan.hif_func.hif_clear_int_ext(SLEEP_INT_CLR);
#ifndef NMI_OPTIMIZE_SLEEP_INT
	genuChipPSstate = CHIP_SLEEPING_AUTO;
#endif
}

static void nmi_wlan_handle_isr_ext(uint32_t int_status)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
#ifdef MEMORY_STATIC
	uint32_t offset = p->rx_buffer_offset;
#endif
	uint8_t *buffer = NULL;
	uint32_t size;
	uint32_t retries=0;
	int ret;
	struct rxq_entry_t *rqe;



	#ifndef NMI_OPTIMIZE_SLEEP_INT	
	if(genuChipPSstate != CHIP_WAKEDUP)
	#endif	
	{
		chip_wakeup();
	}

	
	/**
		Get the rx size
	**/

	size = ((int_status & 0x7fff) << 2);
	
	while(!size && retries < 10)
	{
		uint32_t time=0;
		/*looping more secure*/
		/*zero size make a crashe because the dma will not happen and that will block the firmware*/
		nmi_debug(N_ERR, "RX Size equal zero ... Trying to read it again for %d time\n",time++);
		p->hif_func.hif_read_size(&size);
		size = ((size & 0x7fff) << 2);
		retries++;
		
	}

	if (size > 0) {
#ifdef MEMORY_STATIC
		if (p->rx_buffer_size - offset < size)
			offset = 0;
		buffer = &p->rx_buffer[offset];
#else
		buffer = p->os_func.os_malloc(size);
		if (buffer == NULL) {
			nmi_debug(N_ERR, "[nmi isr]: fail alloc host memory...drop the packets (%d)\n", size);
			NMI_Sleep(100);
			goto _end_;
		}
#endif

		/**
			clear the chip's interrupt	 after getting size some register getting corrupted after clear the interrupt
		**/
		p->hif_func.hif_clear_int_ext(DATA_INT_CLR|ENABLE_RX_VMM);
		

		/**
		start transfer
		**/	
		ret = p->hif_func.hif_block_rx_ext(0, buffer, size);

 		if (!ret) {
			nmi_debug(N_ERR, "[nmi isr]: fail block rx...\n");
			goto _end_;
		}		
_end_:		


		if (ret) {
#ifdef MEMORY_STATIC
			offset += size;
			p->rx_buffer_offset = offset;
#endif
			/**
				add to rx queue
			**/
			rqe = (struct rxq_entry_t *)p->os_func.os_malloc(sizeof(struct rxq_entry_t));
			if (rqe != NULL) {
				rqe->buffer = buffer;
				rqe->buffer_size = size;
				PRINT_D(RX_DBG,"rxq entery Size= %d - Address = %p\n",rqe->buffer_size,rqe->buffer);
				nmi_wlan_rxq_add(rqe);
				p->os_func.os_signal(p->rxq_wait);
			}
		} else {
#ifndef MEMORY_STATIC
			if (buffer != NULL)
				p->os_func.os_free(buffer);
#endif
		}
	}

	#ifdef NMI_OPTIMIZE_SLEEP_INT
		chip_allow_sleep();
	#endif
	
}

void nmi_handle_isr_v2(void)
{
	uint32_t int_status;

	g_wlan.os_func.os_enter_cs(g_wlan.hif_lock);
	g_wlan.hif_func.hif_read_int(&int_status);
	
	if(int_status & PLL_INT_EXT){
		nmi_pllupdate_isr_ext(int_status);
	}
	if(int_status & DATA_INT_EXT){
		nmi_wlan_handle_isr_ext(int_status);
	#ifndef NMI_OPTIMIZE_SLEEP_INT
		/* Chip is up and talking*/
		genuChipPSstate = CHIP_WAKEDUP;
	#endif
	}
	if(int_status & SLEEP_INT_EXT){
		nmi_sleeptimer_isr_ext(int_status);
	}
	
	if(!(int_status & (ALL_INT_EXT))) {	
		printk(">> UNKNOWN_INTERRUPT - 0x%08x\n",int_status);
		nmi_unknown_isr_ext();
	}
#if ((!defined NMI_SDIO) || (defined NMI_SDIO_IRQ_GPIO))
	linux_wlan_enable_irq();
#endif
	g_wlan.os_func.os_leave_cs(g_wlan.hif_lock);
}
#endif /* DMA_VER == DMA_VER_2 */

void nmi_handle_isr(void)
{
	
#if DMA_VER == DMA_VER_2		
	if(g_wlan.use_dma_v2) {
		nmi_handle_isr_v2();
	} else {
		nmi_handle_isr_v1();
	}	
#else /* DMA_VER == DMA_VER_2 */
	nmi_handle_isr_v1();
#endif /* DMA_VER == DMA_VER_2 */
}
/********************************************

	Firmware download

********************************************/
static int nmi_wlan_firmware_download(const uint8_t *buffer, uint32_t buffer_size)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	uint32_t offset;
	uint32_t addr, size, size2, blksz;
	uint8_t *dma_buffer;
	int ret;

	blksz = (1ul << 12); /* Bug 4703: 4KB Good enough size for most platforms = PAGE_SIZE. */
	/* Allocate a DMA coherent  buffer. */
	dma_buffer = (uint8_t *)g_wlan.os_func.os_malloc(blksz);
	if (dma_buffer == NULL) {
		/*EIO	5*/
		ret = -5;
		PRINT_ER("Can't allocate buffer for firmware download IO error\n ");
		goto _fail_1;
	}
	
	PRINT_D(GENERIC_DBG,"Downloading firmware size = %d ...\n",buffer_size);
	/**
		load the firmware
	**/
	offset = 0;
	do {
		memcpy(&addr, &buffer[offset], 4);
		memcpy(&size, &buffer[offset+4], 4);
#ifdef BIG_ENDIAN
		addr = BYTE_SWAP(addr);
		size = BYTE_SWAP(size);
#endif
		p->os_func.os_enter_cs(p->hif_lock);
		offset += 8;		
		while(((int)size) && (offset < buffer_size)) {
			if(size <= blksz) {
				size2 = size;
			} else {
				size2 = blksz;				
			}
			/* Copy firmware into a DMA coherent buffer */
			memcpy(dma_buffer, &buffer[offset], size2);
			ret = p->hif_func.hif_block_tx(addr, dma_buffer, size2);
			if (!ret) break;			

			addr += size2;
			offset += size2;
			size -= size2;			
		}
		p->os_func.os_leave_cs(p->hif_lock);

		if (!ret){ 
			/*EIO	5*/
			ret = -5;
			PRINT_ER("Can't download firmware IO error\n ");
			goto _fail_;
		}
		PRINT_D(GENERIC_DBG,"Offset = %d\n",offset);
	} while (offset < buffer_size); 

_fail_:
		if(dma_buffer) g_wlan.os_func.os_free(dma_buffer);
_fail_1:

	return (ret < 0)? ret:0;
}

/********************************************

	Common

********************************************/
static int nmi_wlan_start(void)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	uint32_t reg = 0;
	int ret;
	uint32_t chipid;

	/**
		Set the host interface
	**/
#ifdef OLD_FPGA_BITFILE
	p->os_func.os_enter_cs(p->hif_lock);
	ret = p->hif_func.hif_read_reg(NMI_VMM_CORE_CTL, &reg);
	if (!ret) {
		nmi_debug(N_ERR, "[nmi start]: fail read reg vmm_core_ctl...\n");
		p->os_func.os_leave_cs(p->hif_lock);
 		return ret;
	}
	reg |= (p->io_func.io_type<<2); 
	ret = p->hif_func.hif_write_reg(NMI_VMM_CORE_CTL, reg);
	if (!ret) {
		nmi_debug(N_ERR, "[nmi start]: fail write reg vmm_core_ctl...\n");
		p->os_func.os_leave_cs(p->hif_lock);
 		return ret;
	}
#else
	if (p->io_func.io_type == HIF_SDIO) {
		reg = 0;
#ifndef REG_0XF6_NOT_CLEAR_BUG_FIX /* bug 4456 and 4557 */
		reg |= (1 << 3);
#endif /* REG_0XF6_NOT_CLEAR_BUG_FIX */ /* bug 4456 and 4557 */
	} else if (p->io_func.io_type == HIF_SPI) {
		reg = 1;
	}
	p->os_func.os_enter_cs(p->hif_lock);
	ret = p->hif_func.hif_write_reg(NMI_VMM_CORE_CFG, reg);
	if (!ret) {
		nmi_debug(N_ERR, "[nmi start]: fail write reg vmm_core_cfg...\n");
		p->os_func.os_leave_cs(p->hif_lock);
		/* EIO  5*/
		ret = -5;
 		return ret;
	}
	reg = 0;
#ifdef NMI_SDIO_IRQ_GPIO
	reg |= NMI_HAVE_SDIO_IRQ_GPIO;
#endif

#if DMA_VER == DMA_VER_2
	if(p->use_dma_v2) {
		reg |= NMI_HAVE_DMA_VER_2;
	}
#endif

#ifdef VCO_14_SUPPLY
	if(nmi_get_chipid(NMI_FALSE) < 0x1002a0){
		reg |= NMI_HAVE_VCO_14_SUPPLY;
	}
#endif
	
#ifdef NMI_HOST_HAS_RTC
	reg |= NMI_HAVE_RTC;
#endif
	ret = p->hif_func.hif_write_reg(NMI_GP_REG_1, reg);
	if (!ret) {
		nmi_debug(N_ERR, "[nmi start]: fail write NMI_GP_REG_1 ...\n");
		p->os_func.os_unlock(p->hif_lock);
		/* EIO  5*/
		ret = -5;
 		return ret;
	}
#endif


	/**
		Bus related 
	**/
#if DMA_VER == DMA_VER_2
	if(p->use_dma_v2) {
		p->hif_func.hif_sync_ext(NUM_INT_EXT);
	} else {
		p->hif_func.hif_sync();
	}
#else
	p->hif_func.hif_sync();
#endif

	ret = p->hif_func.hif_read_reg(0x1000, &chipid);
	if (!ret) {
		nmi_debug(N_ERR, "[nmi start]: fail read reg 0x1000 ...\n");
		p->os_func.os_leave_cs(p->hif_lock);
		/* EIO  5*/
		ret = -5;
 		return ret;
	}

	/**
		Go...
	**/

	//p->hif_func.hif_write_reg(0x150014, reg);
	
	p->hif_func.hif_read_reg(NMI_GLB_RESET_0,&reg);
	if((reg & (1ul << 10)) == (1ul << 10)){
		reg &= ~(1ul << 10);
		p->hif_func.hif_write_reg(NMI_GLB_RESET_0, reg);	
		p->hif_func.hif_read_reg(NMI_GLB_RESET_0,&reg);
	}

	reg |= (1ul << 10);			
	ret = p->hif_func.hif_write_reg(NMI_GLB_RESET_0, reg);	
	p->hif_func.hif_read_reg(NMI_GLB_RESET_0,&reg);
	p->os_func.os_leave_cs(p->hif_lock);

	return (ret<0)?ret:0;
}

static int nmi_wlan_stop(void)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	uint32_t reg = 0;
	int ret;
	uint8_t timeout=10;
	/**
		TODO: stop the firmware, need a re-download
	**/
	p->os_func.os_enter_cs(p->hif_lock);
#ifndef NMI_OPTIMIZE_SLEEP_INT
	if(genuChipPSstate != CHIP_WAKEDUP)
#endif
	{
		chip_wakeup();
	}
	
	ret = p->hif_func.hif_read_reg(NMI_GLB_RESET_0, &reg);
	if (!ret) {
		PRINT_ER("Error while reading reg\n");
		p->os_func.os_leave_cs(p->hif_lock);
		return ret;
	}

	reg &= ~(1 << 10);
	

	ret = p->hif_func.hif_write_reg(NMI_GLB_RESET_0, reg);
	if (!ret) {
		PRINT_ER("Error while writing reg\n");
		p->os_func.os_leave_cs(p->hif_lock);
		return ret;
	}
	
	

	do
	{
		ret = p->hif_func.hif_read_reg(NMI_GLB_RESET_0, &reg);
		if (!ret) {
			PRINT_ER("Error while reading reg\n");
			p->os_func.os_leave_cs(p->hif_lock);
			return ret;
		}
		PRINT_D(GENERIC_DBG,"Read RESET Reg %x : Retry%d\n",reg,timeout);
        /*Workaround to ensure that the chip is actually reset*/
		if( (reg & (1 << 10) ) )
		{
			PRINT_D(GENERIC_DBG,"Bit 10 not reset : Retry %d\n",timeout);
			reg &= ~(1 << 10);
			ret = p->hif_func.hif_write_reg(NMI_GLB_RESET_0, reg);
			timeout--;
		}
		else
		{
			PRINT_D(GENERIC_DBG,"Bit 10 reset after : Retry %d\n",timeout);
			ret = p->hif_func.hif_read_reg(NMI_GLB_RESET_0, &reg);
			if (!ret) {
			PRINT_ER("Error while reading reg\n");
			p->os_func.os_leave_cs(p->hif_lock);
			return ret;
			}
			PRINT_D(GENERIC_DBG,"Read RESET Reg %x : Retry%d\n",reg,timeout);
		 	break;
		}

	}while(timeout)	;

/******************************************************************************/
/* This was add at Bug 4595 to reset the chip while maintaining the bus state */
/******************************************************************************/
	reg = ((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<8)|(1<<9)); 						/**/
																			/**/
	ret = p->hif_func.hif_write_reg(NMI_GLB_RESET_0, reg);					/**/
	reg = 0x7FFFFBFF&(~(1<<10));											/**/
																			/**/
	ret = p->hif_func.hif_write_reg(NMI_GLB_RESET_0, reg);					/**/
/******************************************************************************/


	p->os_func.os_leave_cs(p->hif_lock);

	return ret;
}

static void nmi_wlan_cleanup(void)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	struct txq_entry_t *tqe;
	struct rxq_entry_t *rqe;
	uint32_t reg = 0;
	int ret;

	p->quit = 1;
	/**
		wait for queue end
	**/
	//p->os_func.os_signal(p->txq_wait);
	//p->os_func.os_signal(p->rxq_wait);

	//complete(p->txq_wait);
	//complete(p->rxq_wait);
	/*do {
		if (p->txq_exit && p->rxq_exit)
			break;
	} while (1);*/

	/**
		clean up the queue
	**/
	do {
		tqe = nmi_wlan_txq_remove_from_head();
		if (tqe == NULL)
			break;
		if (tqe->tx_complete_func)
			tqe->tx_complete_func(tqe->priv, 0);
		p->os_func.os_free((void *)tqe);
	} while (1);

	do {
		rqe = nmi_wlan_rxq_remove();
		if (rqe == NULL)
			break;
#ifdef MEMORY_DYNAMIC
		p->os_func.os_free((void *)tqe->buffer);
#endif
		p->os_func.os_free((void *)rqe);
	} while (1);

	/**
		clean up buffer
	**/
	if (p->tx_buffer) {
		p->os_func.os_free(p->tx_buffer);
	}	
#if 0
	if (p->rx_buffer) {
		p->os_func.os_free(p->rx_buffer);
	}	
#endif	
	p->os_func.os_enter_cs(p->hif_lock);

#ifndef NMI_OPTIMIZE_SLEEP_INT
	if(genuChipPSstate != CHIP_WAKEDUP)
#endif
	{
		chip_wakeup();
	}
	
	ret = p->hif_func.hif_read_reg(NMI_INT_STATS,&reg); 
	if (!ret) {
		PRINT_ER("Error while reading reg\n");
		p->os_func.os_leave_cs(p->hif_lock);
	}
	PRINT_ER("Writing ABORT reg\n");
  	ret = p->hif_func.hif_write_reg(NMI_INT_STATS,(reg | ABORT_INT ));  
	if (!ret) {
		PRINT_ER("Error while writing reg\n");
		p->os_func.os_leave_cs(p->hif_lock);
	}
	p->os_func.os_leave_cs(p->hif_lock);
	/**
		io clean up
	**/
	p->hif_func.hif_deinit(NULL);

}

static int nmi_wlan_cfg_commit(int type)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	nmi_cfg_frame_t *cfg = &p->cfg_frame;
	int total_len = p->cfg_frame_offset+4;
	int seq_no = p->cfg_seq_no%256;

	/**
		Set up header
	**/
	if (type == NMI_CFG_SET) {		/* Set */
		cfg->wid_header[0] = 'W';		
	} else {					/* Query */
		cfg->wid_header[0] = 'Q';
	}
	cfg->wid_header[1] = seq_no;	/* sequence number */
	cfg->wid_header[2] = (uint8_t)total_len;
	cfg->wid_header[3] = (uint8_t)(total_len>>8);
	p->cfg_seq_no = seq_no;

	/**
		Add to TX queue
	**/

	/*Edited by Amr - BugID_4720*/
	if(!nmi_wlan_txq_add_cfg_pkt(&cfg->wid_header[0], total_len))
		return -1;
	
	return 0;
}

static int nmi_wlan_cfg_set(int start, uint32_t wid, uint8_t *buffer, uint32_t buffer_size, int commit)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	uint32_t offset;
	int ret_size;
	
	
	if (p->cfg_frame_in_use)
		return 0;

	if (start)
		p->cfg_frame_offset = 0;

	offset = p->cfg_frame_offset;
	ret_size = p->cif_func.cfg_wid_set(p->cfg_frame.frame, offset, (uint16_t)wid, buffer, buffer_size);
	offset += ret_size;
	p->cfg_frame_offset = offset;

	if (commit) {
		PRINT_D(TX_DBG,"[NMI]PACKET Commit with sequence number %d\n",p->cfg_seq_no);
		PRINT_D(RX_DBG,"Processing cfg_set()\n");
		p->cfg_frame_in_use = 1;

		/*Edited by Amr - BugID_4720*/
		if(nmi_wlan_cfg_commit(NMI_CFG_SET))
			return 0;
		
		if(p->os_func.os_wait(p->cfg_wait,CFG_PKTS_TIMEOUT))
		{
			printk("Set Timed Out\n");
			ret_size = 0;
		}
		p->cfg_frame_in_use = 0;
		p->cfg_frame_offset = 0;
		p->cfg_seq_no += 1;
	
	}
	
	return ret_size;	
}
static int nmi_wlan_cfg_get(int start, uint32_t wid, int commit)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	uint32_t offset;
	int ret_size;


	if (p->cfg_frame_in_use)
		return 0;

	if (start)
		p->cfg_frame_offset = 0;

	offset = p->cfg_frame_offset;
	ret_size = p->cif_func.cfg_wid_get(p->cfg_frame.frame, offset, (uint16_t)wid);		
	offset += ret_size;
	p->cfg_frame_offset = offset;

	if (commit) {
		p->cfg_frame_in_use = 1;

		/*Edited by Amr - BugID_4720*/
		if(nmi_wlan_cfg_commit(NMI_CFG_QUERY))
			return  0;

		
		if(p->os_func.os_wait(p->cfg_wait,CFG_PKTS_TIMEOUT))
		{
			printk("Get Timed Out\n");
			ret_size = 0;	
		}
		PRINT_D(GENERIC_DBG, "[NMI]Get Rsponse received\n");
		p->cfg_frame_in_use = 0;
		p->cfg_frame_offset = 0;
		p->cfg_seq_no += 1;
	}
	
	return ret_size;	
}

static int nmi_wlan_cfg_get_val(uint32_t wid, uint8_t *buffer, uint32_t buffer_size)
{
	nmi_wlan_dev_t *p = (nmi_wlan_dev_t *)&g_wlan;
	int ret;

	ret = p->cif_func.cfg_wid_get_val((uint16_t)wid, buffer, buffer_size);

	return ret;
}

void nmi_bus_set_max_speed(void){
	
	/* Increase bus speed to max possible.  */
	g_wlan.hif_func.hif_set_max_bus_speed();
}

void nmi_bus_set_default_speed(void){
	
	/* Restore bus speed to default.  */
	g_wlan.hif_func.hif_set_default_bus_speed();		
}
uint32_t init_chip(void)
{
	uint32_t chipid;
	uint32_t reg,ret=0;


	g_wlan.os_func.os_enter_cs(g_wlan.hif_lock);
	chipid = nmi_get_chipid(NMI_TRUE);


	printk("ChipID = %x\n",chipid);

	
	if((chipid & 0xfff) >= 0xd0) {		
		NMI_INT_STATS = _NMI_INT_STATS_D0_AND_LATER;
		NMI_GP_REG_1 = _NMI_GP_REG_1_D0_AND_LATER;
	} else {
		NMI_INT_STATS = _NMI_INT_STATS_PRE_D0;
		NMI_GP_REG_1 = _NMI_GP_REG_1_PRE_D0;
	}

	
#if DMA_VER == DMA_VER_2
	if((chipid & 0xfff) >= 0xe0) {
		g_wlan.use_dma_v2 = 1; 
	} else {
		g_wlan.use_dma_v2 = 0; 
	}
#endif /* DMA_VER == DMA_VER_2 */

	if((chipid & 0xfff) != 0xa0) {
		/**
		Avoid booting from boot ROM. Make sure that Drive IRQN [SDIO platform]
		or SD_DAT3 [SPI platform] to ?1?
		**/
		/* Set cortus reset register to register control. */
		ret = g_wlan.hif_func.hif_read_reg(0x1118, &reg);
		if (!ret) {
			nmi_debug(N_ERR, "[nmi start]: fail read reg 0x1118 ...\n");
			return ret;
		}
		reg |= (1 << 0);
		ret = g_wlan.hif_func.hif_write_reg(0x1118, reg);
		if (!ret) {
			nmi_debug(N_ERR, "[nmi start]: fail write reg 0x1118 ...\n");
			return ret;
		}
		/**
		Write branch intruction to IRAM (0x71 trap) at location 0xFFFF0000
		(Cortus map) or C0000 (AHB map).
		**/
		ret = g_wlan.hif_func.hif_write_reg(0xc0000, 0x71);
		if (!ret) {
			nmi_debug(N_ERR, "[nmi start]: fail write reg 0xc0000 ...\n");
			return ret;
		}
	}


	#if 0
	if((chipid& 0xfff) < 0xf0) {
		/* Setting MUX to probe sleep signal on pin 6 of J216*/
		g_wlan.hif_func.hif_write_reg(0x1060, 0x1);
		g_wlan.hif_func.hif_write_reg(0x1180, 0x33333333);
		g_wlan.hif_func.hif_write_reg(0x1184, 0x33333333);
		g_wlan.hif_func.hif_read_reg(0x1408, &reg);
		/* set MUX for GPIO_4 (pin 4) to cortus GPIO*/
		reg &= ~((0x7 << 16));
		g_wlan.hif_func.hif_write_reg(0x1408, (reg|(0x7 << 12)));	
	}else {
		/* Enable test bus*/
		g_wlan.hif_func.hif_write_reg(0x1060, 0x1);
		/* Rotate bus signals to get sleep signal on pin 6 like it was on previous chips*/
		g_wlan.hif_func.hif_write_reg(0x1188, 0x70);
		/* Set output of pin 6 to test bus 0x1*/
		/* Set output of pin 9 to test bus 0x2*/
		g_wlan.hif_func.hif_write_reg(0x1180, 0x200100);
		g_wlan.hif_func.hif_read_reg(0x1408, &reg);

		/* set MUX for GPIO_4 (pin 4) to cortus GPIO*/
		reg &= ~((0x7 << 16));
		/* set MUX for GPIO_3 (pin 6) to test bus*/
		reg |= (0x7 << 12) | (0x7 << 24);
		g_wlan.hif_func.hif_write_reg(0x1408, reg);
	}
	#endif



	g_wlan.os_func.os_leave_cs(g_wlan.hif_lock);

	return ret;

}

uint32_t nmi_get_chipid(uint8_t update)
{	
	static uint32_t chipid = 0;
	// SDIO can't read into global variables
	// Use this variable as a temp, then copy to the global
	uint32_t tempchipid = 0;
	uint32_t rfrevid;
	
	if(chipid == 0 || update != 0) {		
		g_wlan.hif_func.hif_read_reg(0x1000,&tempchipid);
		g_wlan.hif_func.hif_read_reg(0x13f4,&rfrevid);
		if(!ISNMC1000(tempchipid)) {
			chipid = 0;
			goto _fail_;
		} 
		if (chipid == 0x1002a0)  { 
			if (rfrevid == 0x1) {
				tempchipid = 0x1002a0; 
			} else if (rfrevid == 0x2) {
				tempchipid = 0x1002a1; 
			}
		} 

		chipid = tempchipid;
	}
_fail_:
	return chipid;
}

#ifdef COMPLEMENT_BOOT
uint8_t core_11b_ready(void)
{	
	uint32_t reg_val;

	g_wlan.os_func.os_enter_cs(g_wlan.hif_lock);	
	g_wlan.hif_func.hif_write_reg(0x16082c,1);
	g_wlan.hif_func.hif_write_reg(0x161600,0x90);
	g_wlan.hif_func.hif_read_reg(0x161600,&reg_val);
	g_wlan.os_func.os_leave_cs(g_wlan.hif_lock);
		
	if(reg_val == 0x90)
		return 0;		
	else
		return 1;
}
#endif

int nmi_wlan_init(nmi_wlan_inp_t *inp, nmi_wlan_oup_t *oup)
{

	int ret = 0;
	
	printk("Initializing NMI_Wlan ...\n");

	memset((void *)&g_wlan, 0, sizeof(nmi_wlan_dev_t));

	/**
		store the input
	**/
	memcpy((void *)&g_wlan.os_func, (void *)&inp->os_func, sizeof(nmi_wlan_os_func_t));
	memcpy((void *)&g_wlan.io_func, (void *)&inp->io_func, sizeof(nmi_wlan_io_func_t));
	memcpy((void *)&g_wlan.net_func, (void *)&inp->net_func, sizeof(nmi_wlan_net_func_t));
	memcpy((void *)&g_wlan.indicate_func, (void *)&inp->indicate_func, sizeof(nmi_wlan_net_func_t));
	g_wlan.hif_lock = inp->os_context.hif_critical_section;
	g_wlan.txq_lock = inp->os_context.txq_critical_section;

	/*Added by Amr - BugID_4720*/
	g_wlan.txq_add_to_head_lock = inp->os_context.txq_add_to_head_critical_section;

	/*Added by Amr - BugID_4720*/
	g_wlan.txq_spinlock = inp->os_context.txq_spin_lock;
	
	g_wlan.rxq_lock = inp->os_context.rxq_critical_section;
	g_wlan.txq_wait = inp->os_context.txq_wait_event;
	g_wlan.rxq_wait = inp->os_context.rxq_wait_event;
	g_wlan.cfg_wait = inp->os_context.cfg_wait_event;
	g_wlan.tx_buffer_size = inp->os_context.tx_buffer_size;
	//g_wlan.rx_buffer_size = inp->os_context.rx_buffer_size;

	//g_wlan.os_func.os_lock(g_wlan.cfg_wait);
	/***
		host interface init
	**/
	
	if ((inp->io_func.io_type & 0x1) == HIF_SDIO) {
		if (!hif_sdio.hif_init(inp, nmi_debug)) {
			/* EIO	5 */
			ret = -5;
			goto _fail_;
		}		
		memcpy((void *)&g_wlan.hif_func, &hif_sdio, sizeof(nmi_hif_func_t));
	} else{ 	
	if ((inp->io_func.io_type & 0x1) == HIF_SPI) {
		/**
			TODO:
		**/
		if (!hif_spi.hif_init(inp, nmi_debug)) {
			/* EIO	5 */			
			ret = -5;
			goto _fail_;
		}		
		memcpy((void *)&g_wlan.hif_func, &hif_spi, sizeof(nmi_hif_func_t));
	} else {
		/* EIO	5 */
		ret = -5;
		goto _fail_;
	}
	}

	/***
		mac interface init
	**/
	if (!mac_cfg.cfg_init(nmi_debug)) {
		/* ENOBUFS	105 */
		ret = -105;
		goto _fail_;
	}		
	memcpy((void *)&g_wlan.cif_func, &mac_cfg, sizeof(nmi_cfg_func_t));

	
	/**
		alloc tx, rx buffer
	**/
	g_wlan.tx_buffer = (uint8_t *)g_wlan.os_func.os_malloc(g_wlan.tx_buffer_size);
	if (g_wlan.tx_buffer == NULL) {
		/* ENOBUFS	105 */
		ret = -105;
		goto _fail_;
		}
		
/* rx_buffer is not used unless we activate USE_MEM STATIC which is not applicable, allocating such memory is useless*/	
#if 0	
	g_wlan.rx_buffer = (uint8_t *)g_wlan.os_func.os_malloc(g_wlan.rx_buffer_size);
	if (g_wlan.rx_buffer == NULL) 
		goto _fail_;
#endif

	/**
		export functions
	**/
	oup->wlan_firmware_download = nmi_wlan_firmware_download;
	oup->wlan_start = nmi_wlan_start;
	oup->wlan_stop = nmi_wlan_stop;
	oup->wlan_add_to_tx_que = nmi_wlan_txq_add_net_pkt;
	oup->wlan_handle_tx_que = nmi_wlan_handle_txq;
	oup->wlan_handle_rx_que = nmi_wlan_handle_rxq;
	//oup->wlan_handle_rx_isr = nmi_wlan_handle_isr;
	oup->wlan_handle_rx_isr = nmi_handle_isr;
	oup->wlan_cleanup = nmi_wlan_cleanup;
	oup->wlan_cfg_set = nmi_wlan_cfg_set;
	oup->wlan_cfg_get = nmi_wlan_cfg_get;
	oup->wlan_cfg_get_value = nmi_wlan_cfg_get_val;

	/*Bug3959: transmitting mgmt frames received from host*/
	#if defined(NMI_AP_EXTERNAL_MLME) || defined(NMI_P2P)
	oup->wlan_add_mgmt_to_tx_que = nmi_wlan_txq_add_mgmt_pkt;

	#ifdef NMI_FULLY_HOSTING_AP
	oup->wlan_add_data_to_tx_que = nmi_FH_wlan_txq_add_net_pkt;
	#endif
	#endif

	if(!init_chip()){
		/* EIO	5 */
		ret = -5;
		goto _fail_;
	}
#ifdef	TCP_ACK_FILTER
 	Init_TCP_tracking();
#endif
	return 1;

_fail_:	
	
	if (g_wlan.tx_buffer)
		g_wlan.os_func.os_free(g_wlan.tx_buffer);

#if 0
	if (g_wlan.rx_buffer)
		g_wlan.os_func.os_free(g_wlan.rx_buffer);
#endif	
	return ret;

}
#ifdef NMI_FULLY_HOSTING_AP
nmi_wlan_dev_t* Get_wlan_context(NMI_Uint16* pu16size)
{
	*pu16size = sizeof(nmi_wlan_dev_t);
	return &g_wlan;
}
#endif

