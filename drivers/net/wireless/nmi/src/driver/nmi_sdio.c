////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Newport Media Inc.  All rights reserved.
//
// Module Name:  nmi_sdio.c
//
//
//////////////////////////////////////////////////////////////////////////////

#include "nmi_wlan_if.h"
#include "nmi_wlan.h"


#ifdef NMC1000_SINGLE_TRANSFER
#define NMI_SDIO_BLOCK_SIZE 256
#else
 #if defined(PLAT_AML8726_M3) //johnny
	#define NMI_SDIO_BLOCK_SIZE 512
	#define MAX_SEG_SIZE (1<<12) //4096
 #else
	#define NMI_SDIO_BLOCK_SIZE 512
 #endif
#endif

typedef struct {
	void *os_context;
	nmi_wlan_os_func_t os_func;
	uint32_t block_size;
	int (*sdio_cmd52)(sdio_cmd52_t *);
	int (*sdio_cmd53)(sdio_cmd53_t *);
	int (*sdio_set_max_speed)(void);
	int (*sdio_set_default_speed)(void);
	nmi_debug_func dPrint;
#if DMA_VER == DMA_VER_2
	int nint;
	int has_thrpt_enh;
	int has_thrpt_enh2;
	int reg_0xf6_not_clear_bug_fix;
#endif /* DMA_VER == DMA_VER_2 */
} nmi_sdio_t;

static nmi_sdio_t g_sdio;

#ifdef NMI_SDIO_IRQ_GPIO
static int sdio_write_reg(uint32_t addr, uint32_t data);
static int sdio_read_reg(uint32_t addr, uint32_t *data);
#endif
extern unsigned int int_clrd;

/********************************************

	Function 0

********************************************/

static int sdio_set_func0_csa_address(uint32_t adr)
{
	sdio_cmd52_t cmd;

	/**
		Review: BIG ENDIAN
	**/
	cmd.read_write = 1;
	cmd.function = 0;
	cmd.raw = 0;
	cmd.address = 0x10c;
	cmd.data = (uint8_t)adr;
	if (!g_sdio.sdio_cmd52(&cmd)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd52, set 0x10c data...\n");
		goto _fail_;
	}

	cmd.address = 0x10d;
	cmd.data = (uint8_t)(adr>>8);
	if (!g_sdio.sdio_cmd52(&cmd)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd52, set 0x10d data...\n");
		goto _fail_;
	}

	cmd.address = 0x10e;
	cmd.data = (uint8_t)(adr>>16);
	if (!g_sdio.sdio_cmd52(&cmd)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd52, set 0x10e data...\n");
		goto _fail_;
	}

	return 1;
_fail_:
	return 0;
}

static int sdio_set_func0_csa_address_byte0(uint32_t adr)
{
	sdio_cmd52_t cmd;


	/**
		Review: BIG ENDIAN
	**/
	cmd.read_write = 1;
	cmd.function = 0;
	cmd.raw = 0;
	cmd.address = 0x10c;
	cmd.data = (uint8_t)adr;
	if (!g_sdio.sdio_cmd52(&cmd)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd52, set 0x10c data...\n");
		goto _fail_;
	}

	return 1;
_fail_:
	return 0;
}
static int sdio_set_func0_block_size(uint32_t block_size)
{
	sdio_cmd52_t cmd;

	cmd.read_write = 1;
	cmd.function = 0;
	cmd.raw = 0;
	cmd.address = 0x10;
	cmd.data = (uint8_t)block_size;
	if (!g_sdio.sdio_cmd52(&cmd)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd52, set 0x10 data...\n");
		goto _fail_;
	}

	cmd.address = 0x11;
	cmd.data = (uint8_t)(block_size>>8);
	if (!g_sdio.sdio_cmd52(&cmd)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd52, set 0x11 data...\n");
		goto _fail_;
	}

	return 1;
_fail_:
	return 0;
}

/********************************************

	Function 1

********************************************/

static int sdio_set_func1_block_size(uint32_t block_size)
{
	sdio_cmd52_t cmd;

	cmd.read_write = 1;
	cmd.function = 0;
	cmd.raw = 0;
	cmd.address = 0x110;
	cmd.data = (uint8_t)block_size;
	if (!g_sdio.sdio_cmd52(&cmd)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd52, set 0x110 data...\n");
		goto _fail_;
	}
	cmd.address = 0x111;
	cmd.data = (uint8_t)(block_size>>8);
	if (!g_sdio.sdio_cmd52(&cmd)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd52, set 0x111 data...\n");
		goto _fail_;
	}

	return 1;
_fail_:
	return 0;
}

static int sdio_clear_int(void)
{
#ifndef NMI_SDIO_IRQ_GPIO
	//uint32_t sts;
	sdio_cmd52_t cmd;
	cmd.read_write = 0;
	cmd.function = 1;
	cmd.raw = 0;
	cmd.address = 0x4;
	cmd.data = 0;
	g_sdio.sdio_cmd52(&cmd);
	int_clrd++;
	//printk("intSts=%d\n",cmd.data);
	return cmd.data;
#else
	uint32_t reg;
	if (!sdio_read_reg(NMI_HOST_RX_CTRL_0, &reg)) {
		g_sdio.dPrint(N_ERR, "[nmi spi]: Failed read reg (%08x)...\n", NMI_HOST_RX_CTRL_0);
		return 0;
	}
	reg &= ~0x1;
	sdio_write_reg(NMI_HOST_RX_CTRL_0, reg);
		int_clrd++;
	return 1;
#endif

}

uint32_t sdio_xfer_cnt(void)
{
	uint32_t cnt = 0;
	sdio_cmd52_t cmd;
	cmd.read_write = 0;
	cmd.function = 1;
	cmd.raw = 0;
	cmd.address = 0x1C;
	cmd.data = 0;
	g_sdio.sdio_cmd52(&cmd);
	cnt = cmd.data;

	cmd.read_write = 0;
	cmd.function = 1;
	cmd.raw = 0;
	cmd.address = 0x1D;
	cmd.data = 0;
	g_sdio.sdio_cmd52(&cmd);
	cnt |= (cmd.data<<8);	

	cmd.read_write = 0;
	cmd.function = 1;
	cmd.raw = 0;
	cmd.address = 0x1E;
	cmd.data = 0;
	g_sdio.sdio_cmd52(&cmd);
	cnt |= (cmd.data<<16);		
	
	return cnt;
	

}

/********************************************

	Sdio interfaces

********************************************/
int sdio_check_bs(void)
{
	sdio_cmd52_t cmd;

	/**
		poll until BS is 0
	**/
	cmd.read_write = 0;
	cmd.function = 0;
	cmd.raw = 0;
	cmd.address = 0xc;
	cmd.data = 0;
	if (!g_sdio.sdio_cmd52(&cmd)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Fail cmd 52, get BS register...\n");
		goto _fail_;
	}

	return 1;

_fail_:

	return 0;
}

static int sdio_write_reg(uint32_t addr, uint32_t data)
{
#ifdef BIG_ENDIAN
	data = BYTE_SWAP(data);
#endif

	if((addr >= 0xf0) && (addr <= 0xff))
	{
		sdio_cmd52_t cmd;
		cmd.read_write = 1;
		cmd.function = 0;
		cmd.raw = 0;
		cmd.address = addr;
		cmd.data = data;
		if (!g_sdio.sdio_cmd52(&cmd)) {
			g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd 52, read reg (%08x) ...\n", addr);
			goto _fail_;
		}
	}
	else
	{
		sdio_cmd53_t cmd;

		/**
			set the AHB address
		**/
		if (!sdio_set_func0_csa_address(addr)) 
			goto _fail_;

		cmd.read_write = 1;
		cmd.function = 0;
		cmd.address = 0x10f;
		cmd.block_mode = 0;
		cmd.increment = 1;
		cmd.count = 4;
		cmd.buffer = (uint8_t *)&data;
		cmd.block_size = g_sdio.block_size; //johnny : prevent it from setting unexpected value
	
		if (!g_sdio.sdio_cmd53(&cmd)) {
			g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd53, write reg (%08x)...\n", addr);
			goto _fail_;
		}

#if 0
		if (!sdio_check_bs())
			goto _fail_;
#else
		//g_sdio.os_func.os_sleep(1);
#endif
	}

	return 1; 

_fail_:

	return 0;
}

static int sdio_write(uint32_t addr, uint8_t *buf, uint32_t size)
{
	uint32_t block_size = g_sdio.block_size;
	sdio_cmd53_t cmd;
	int nblk, nleft;

	cmd.read_write = 1;
	if (addr > 0) {
		/**
			has to be word aligned...
		**/
		if (size & 0x3) {
			size += 4;
			size &= ~0x3;
		}

		/**
			func 0 access	
		**/
		cmd.function = 0;
		cmd.address = 0x10f;
	} else {
#ifdef NMC1000_SINGLE_TRANSFER
		/**
			has to be block aligned...
		**/
		nleft = size%block_size;
		if (nleft > 0) {
			size += block_size;
			size &= ~(block_size-1);
		}
#else
		/**
			has to be word aligned...
		**/
		if (size & 0x3) {
			size += 4;
			size &= ~0x3;
		}
#endif

		/**
			func 1 access	
		**/
		cmd.function = 1;
		cmd.address = 0;
	}

	nblk = size/block_size;
	nleft = size%block_size;

	if (nblk > 0) {
  
#if defined(PLAT_AML8726_M3_BACKUP) //johnny // rachel
		int i;

		for(i=0; i<nblk; i++)
		{
			cmd.block_mode = 0; //1;
			cmd.increment = 1;
			cmd.count = block_size; //nblk;
			cmd.buffer = buf;
			cmd.block_size = block_size;
			if (addr > 0) {
				if (!sdio_set_func0_csa_address(addr))
					goto _fail_;
			}
			if (!g_sdio.sdio_cmd53(&cmd)) {
				g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd53 [%x], block send...\n",addr);
				goto _fail_;
			}
			
			if (addr > 0)	
				addr += block_size;	//addr += nblk*block_size;
			
			buf += block_size;		//buf += nblk*block_size;
		}

#elif defined(PLAT_AML8726_M3) //johnny

	    int i;
	    int rest;
	    int seg_cnt;

	    seg_cnt = (nblk*block_size)/MAX_SEG_SIZE;
	    rest = (nblk*block_size) & (MAX_SEG_SIZE-1);

	    //printk("[Johnny W] seg_cnt = %d, rest = %d\n", seg_cnt, rest);

	    for ( i = 0 ; i < seg_cnt ; i ++ )
	    {
	      cmd.block_mode = 1;
	      cmd.increment = 1;
	      cmd.count = MAX_SEG_SIZE/block_size;
	      cmd.buffer = buf;
	      cmd.block_size = block_size;

	      //printk("[Johnny W] cmd.count = %d, cmd.block_size = %d\n", cmd.count, cmd.block_size);
      
	      if (addr > 0) {
	        if (!sdio_set_func0_csa_address(addr))
	          goto _fail_;
	      }
	      if (!g_sdio.sdio_cmd53(&cmd)) {
	        g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd53 [%x], block send...\n",addr);
	        goto _fail_;
	      }

	      if (addr > 0) 
	        addr += MAX_SEG_SIZE;
      
	      buf += MAX_SEG_SIZE;

	    }


	  	if ( rest > 0 ) {
	  		cmd.block_mode = 1;
	  		cmd.increment = 1;
	  		cmd.count = rest/block_size;
	  		cmd.buffer = buf;
	  		cmd.block_size = block_size; //johnny : prevent it from setting unexpected value

	  		if (addr > 0) {
	  			if (!sdio_set_func0_csa_address(addr))
	  				goto _fail_;
	  		}
	  		if (!g_sdio.sdio_cmd53(&cmd)) {
	  			g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd53 [%x], bytes send...\n",addr);
	  			goto _fail_;
	  		}

	      if (addr > 0) 
	        addr += rest;
      
	      buf += rest;

	    }

#else

		cmd.block_mode = 1;
		cmd.increment = 1;
		cmd.count = nblk;
		cmd.buffer = buf;
		cmd.block_size = block_size;
		if (addr > 0) {
			if (!sdio_set_func0_csa_address(addr))
				goto _fail_;
		}
		if (!g_sdio.sdio_cmd53(&cmd)) {
			g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd53 [%x], block send...\n",addr);
			goto _fail_;
		}
		if (addr > 0)	
			addr += nblk*block_size;
		buf += nblk*block_size;

#endif //platform

#if 0
		if (!sdio_check_bs())
			goto _fail_;
#else
		//g_sdio.os_func.os_sleep(1);
#endif

	}		


	if (nleft > 0) {
		cmd.block_mode = 0;
		cmd.increment = 1;
		cmd.count = nleft;
		cmd.buffer = buf;

		cmd.block_size = block_size; //johnny : prevent it from setting unexpected value

		if (addr > 0) {
			if (!sdio_set_func0_csa_address(addr))
				goto _fail_;
		}
		if (!g_sdio.sdio_cmd53(&cmd)) {
			g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd53 [%x], bytes send...\n",addr);
			goto _fail_;
		}

#if 0
		if (!sdio_check_bs())
			goto _fail_;
#else
		//g_sdio.os_func.os_sleep(1);
#endif
	}

	return 1; 

_fail_:

	return 0;
}

static int sdio_read_reg(uint32_t addr, uint32_t *data)
{
	if((addr >= 0xf0) && (addr <= 0xff))
	{
		sdio_cmd52_t cmd;
		cmd.read_write = 0;
		cmd.function = 0;
		cmd.raw = 0;
		cmd.address= addr;
		if (!g_sdio.sdio_cmd52(&cmd)) {
			g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd 52, read reg (%08x) ...\n", addr);
			goto _fail_;
		}
		*data = cmd.data;
	}
	else
	{
		sdio_cmd53_t cmd;

		if (!sdio_set_func0_csa_address(addr))
			goto _fail_;

		cmd.read_write = 0;
		cmd.function = 0;
		cmd.address = 0x10f;
		cmd.block_mode = 0;
		cmd.increment = 1;
		cmd.count = 4;
		cmd.buffer = (uint8_t *)data;

		cmd.block_size = g_sdio.block_size; //johnny : prevent it from setting unexpected value

		if (!g_sdio.sdio_cmd53(&cmd)) {
			g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd53, read reg (%08x)...\n", addr);
			goto _fail_;
		}

#if 0
		if (!sdio_check_bs())
			goto _fail_;
#else
		//g_sdio.os_func.os_sleep(1);
#endif
	}

#ifdef BIG_ENDIAN
	*data = BYTE_SWAP(*data);
#endif

	return 1; 

_fail_:

	return 0;
}

static int sdio_read(uint32_t addr, uint8_t *buf, uint32_t size)
{
	uint32_t block_size = g_sdio.block_size;
	sdio_cmd53_t cmd;
	int nblk, nleft;

	cmd.read_write = 0;
	if (addr > 0) {
		/**
			has to be word aligned...
		**/
		if (size & 0x3) {
			size += 4;
			size &= ~0x3;
		}

		/**
			func 0 access	
		**/
		cmd.function = 0;
		cmd.address = 0x10f;
	} else {
#ifdef NMC1000_SINGLE_TRANSFER
		/**
			has to be block aligned...
		**/
		nleft = size%block_size;
		if (nleft > 0) {
			size += block_size;
			size &= ~(block_size-1);
		}
#else
		/**
			has to be word aligned...
		**/
		if (size & 0x3) {
			size += 4;
			size &= ~0x3;
		}
#endif

		/**
			func 1 access	
		**/
		cmd.function = 1;
		cmd.address = 0;
	}

	nblk = size/block_size;
	nleft = size%block_size;

	if (nblk > 0) {

#if defined(PLAT_AML8726_M3_BACKUP) //johnny // rachel

		int i;

		for(i=0; i<nblk; i++)
		{
			cmd.block_mode = 0; //1;
			cmd.increment = 1;
			cmd.count = block_size; //nblk;
			cmd.buffer = buf;
			cmd.block_size = block_size;
			if (addr > 0) {
				if (!sdio_set_func0_csa_address(addr))
					goto _fail_;
			}
			if (!g_sdio.sdio_cmd53(&cmd)) {
				g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd53 [%x], block read...\n",addr);
				goto _fail_;
			}
			if (addr > 0)	
				addr += block_size;		//addr += nblk*block_size;
			buf += block_size;		//buf += nblk*block_size;
		}

#elif defined(PLAT_AML8726_M3) //johnny

	    int i;
	    int rest;
	    int seg_cnt;

	    seg_cnt = (nblk*block_size)/MAX_SEG_SIZE;
	    rest = (nblk*block_size) & (MAX_SEG_SIZE-1);

	    //printk("[Johnny R] seg_cnt = %d, rest = %d\n", seg_cnt, rest);

	    for ( i = 0 ; i < seg_cnt ; i ++ )
	    {
	      cmd.block_mode = 1;
	      cmd.increment = 1;
	      cmd.count = MAX_SEG_SIZE/block_size;
	      cmd.buffer = buf;
	      cmd.block_size = block_size;

	      //printk("[Johnny R] cmd.count = %d, cmd.block_size = %d\n", cmd.count, cmd.block_size);
      
	      if (addr > 0) {
	        if (!sdio_set_func0_csa_address(addr))
	          goto _fail_;
	      }
	      if (!g_sdio.sdio_cmd53(&cmd)) {
	        g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd53 [%x], block read...\n",addr);
	        goto _fail_;
	      }

	      if (addr > 0) 
	        addr += MAX_SEG_SIZE;
      
	      buf += MAX_SEG_SIZE;

	    }


	    if ( rest > 0 ) {
	      cmd.block_mode = 1;
	      cmd.increment = 1;
	      cmd.count = rest/block_size;
	      cmd.buffer = buf;
	      cmd.block_size = block_size; //johnny : prevent it from setting unexpected value

	      if (addr > 0) {
	        if (!sdio_set_func0_csa_address(addr))
	          goto _fail_;
	      }
	      if (!g_sdio.sdio_cmd53(&cmd)) {
	        g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd53 [%x], block read...\n",addr);
	        goto _fail_;
	      }

	      if (addr > 0) 
	        addr += rest;
      
	      buf += rest;

	    }

#else

		cmd.block_mode = 1;
		cmd.increment = 1;
		cmd.count = nblk;
		cmd.buffer = buf;
		cmd.block_size = block_size;
		if (addr > 0) {
			if (!sdio_set_func0_csa_address(addr))
				goto _fail_;
		}
		if (!g_sdio.sdio_cmd53(&cmd)) {
			g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd53 [%x], block read...\n",addr);
			goto _fail_;
		}
		if (addr > 0)	
			addr += nblk*block_size;
		buf += nblk*block_size;

#endif //platform

#if 0
		if (!sdio_check_bs())
			goto _fail_;
#else
		//g_sdio.os_func.os_sleep(1);
#endif

	}	//if (nblk > 0)

	if (nleft > 0) {
		cmd.block_mode = 0;
		cmd.increment = 1;
		cmd.count = nleft;
		cmd.buffer = buf;

		cmd.block_size = block_size; //johnny : prevent it from setting unexpected value
    
		if (addr > 0) {
			if (!sdio_set_func0_csa_address(addr))
				goto _fail_;
		}
		if (!g_sdio.sdio_cmd53(&cmd)) {
			g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd53 [%x], bytes read...\n",addr);
			goto _fail_;
		}

#if 0
		if (!sdio_check_bs())
			goto _fail_;
#else
		//g_sdio.os_func.os_sleep(1);
#endif
	}

	return 1; 

_fail_:

	return 0;
}

/********************************************

	Bus interfaces

********************************************/

static int sdio_deinit(void *pv)
{
	return 1;	
}

static int sdio_sync(void)
{
	uint32_t reg;

	/**
		Disable power sequencer
	**/
	if (!sdio_read_reg(NMI_MISC, &reg)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed read misc reg...\n");
		return 0;
	}
	
	reg &= ~(1 << 8);
	if (!sdio_write_reg(NMI_MISC, reg)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed write misc reg...\n");
		return 0;
	}

#ifdef NMI_SDIO_IRQ_GPIO
	{
		uint32_t reg;
		int ret;

		/**
			interrupt pin mux select
		**/
		ret = sdio_read_reg(NMI_PIN_MUX_0, &reg);
		if (!ret) {
			g_sdio.dPrint(N_ERR, "[nmi spi]: Failed read reg (%08x)...\n", NMI_PIN_MUX_0);
			return 0;
		}
		reg |= (1<<8);
		ret = sdio_write_reg(NMI_PIN_MUX_0, reg);
		if (!ret) {
			g_sdio.dPrint(N_ERR, "[nmi spi]: Failed write reg (%08x)...\n", NMI_PIN_MUX_0);
			return 0;
		}

		/**
			interrupt enable
		**/
		ret = sdio_read_reg(NMI_INTR_ENABLE, &reg);
		if (!ret) {
			g_sdio.dPrint(N_ERR, "[nmi spi]: Failed read reg (%08x)...\n", NMI_INTR_ENABLE);
			return 0;
		}
		reg |= (1<<16);
		ret = sdio_write_reg(NMI_INTR_ENABLE, reg);
		if (!ret) {
			g_sdio.dPrint(N_ERR, "[nmi spi]: Failed write reg (%08x)...\n", NMI_INTR_ENABLE);
			return 0;
		}
	}
#endif

	return 1;
}

static int sdio_init(nmi_wlan_inp_t *inp, nmi_debug_func func)
{
	sdio_cmd52_t cmd;
	int loop;
	uint32_t chipid;
	memset(&g_sdio, 0, sizeof(nmi_sdio_t));

	g_sdio.dPrint = func;
	g_sdio.os_context = inp->os_context.os_private;
	memcpy((void *)&g_sdio.os_func, (void *)&inp->os_func, sizeof(nmi_wlan_os_func_t));

	if (inp->io_func.io_init) {	
		if (!inp->io_func.io_init(g_sdio.os_context)) {
			g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed io init bus...\n");
			return 0;
		}
	} else {
		return 0;
	}

	g_sdio.sdio_cmd52 	= inp->io_func.u.sdio.sdio_cmd52;
	g_sdio.sdio_cmd53 	= inp->io_func.u.sdio.sdio_cmd53;
	g_sdio.sdio_set_max_speed 	= inp->io_func.u.sdio.sdio_set_max_speed;
	g_sdio.sdio_set_default_speed 	= inp->io_func.u.sdio.sdio_set_default_speed;
	
	/**
		function 0 csa enable 
	**/
	cmd.read_write = 1;
	cmd.function = 0;
	cmd.raw = 1;
	cmd.address = 0x100;
	cmd.data = 0x80;
	if (!g_sdio.sdio_cmd52(&cmd)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Fail cmd 52, enable csa...\n");
		goto _fail_;
	}

	/**
		function 0 block size
	**/
	if (!sdio_set_func0_block_size(NMI_SDIO_BLOCK_SIZE)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Fail cmd 52, set func 0 block size...\n");
		goto _fail_;
	}
	g_sdio.block_size = NMI_SDIO_BLOCK_SIZE;

	/**
		enable func1 IO
	**/
	cmd.read_write = 1;
	cmd.function = 0;
	cmd.raw = 1;
	cmd.address = 0x2;
	cmd.data = 0x2;
	if (!g_sdio.sdio_cmd52(&cmd)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio] Fail cmd 52, set IOE register...\n");
		goto _fail_;
	}

	/**
		make sure func 1 is up
	**/
	cmd.read_write = 0;
	cmd.function = 0;
	cmd.raw = 0;
	cmd.address = 0x3;
	loop = 3;
	do {
		cmd.data = 0;
		if (!g_sdio.sdio_cmd52(&cmd)) {
			g_sdio.dPrint(N_ERR, "[nmi sdio]: Fail cmd 52, get IOR register...\n");
			goto _fail_;
		}
		if (cmd.data == 0x2)
			break;
	} while (loop--);

	if (loop <= 0) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Fail func 1 is not ready...\n");
		goto _fail_;
	}

	/**
		func 1 is ready, set func 1 block size
	**/
	if (!sdio_set_func1_block_size(NMI_SDIO_BLOCK_SIZE)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Fail set func 1 block size...\n");
		goto _fail_;
	}

	/**
		func 1 interrupt enable
	**/
	cmd.read_write = 1;
	cmd.function = 0;
	cmd.raw = 1;
	cmd.address = 0x4;
	cmd.data = 0x3;
	if (!g_sdio.sdio_cmd52(&cmd)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Fail cmd 52, set IEN register...\n");
		goto _fail_;
	}

	/**
		make sure can read back chip id correctly
	**/
	if (!sdio_read_reg(0x1000, &chipid)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Fail cmd read chip id...\n");
		goto _fail_;
	}

	g_sdio.dPrint(N_ERR, "[nmi sdio]: chipid (%08x)\n", chipid);

#if DMA_VER == DMA_VER_2
	if( (chipid & 0xfff) > 0x0d0) {
		g_sdio.has_thrpt_enh = 1;
		if( (chipid & 0xfff) > 0x0f0) {
			g_sdio.has_thrpt_enh2 = 1;
#ifdef REG_0XF6_NOT_CLEAR_BUG_FIX /* bug 4456 and 4557 */
			g_sdio.reg_0xf6_not_clear_bug_fix = 1;
#else /* REG_0XF6_NOT_CLEAR_BUG_FIX */
			g_sdio.reg_0xf6_not_clear_bug_fix = 0;
#endif /* REG_0XF6_NOT_CLEAR_BUG_FIX */
		} else {
			g_sdio.has_thrpt_enh2 = 0;
			g_sdio.reg_0xf6_not_clear_bug_fix = 1;
		}
	} else {
		g_sdio.has_thrpt_enh = 0;
		g_sdio.has_thrpt_enh2 = 0;
		g_sdio.reg_0xf6_not_clear_bug_fix = 0;
	}
	g_sdio.dPrint(N_ERR, "[nmi sdio]: has_thrpt_enh = %d...\n", g_sdio.has_thrpt_enh);
	g_sdio.dPrint(N_ERR, "[nmi sdio]: has_thrpt_enh2 = %d...\n", g_sdio.has_thrpt_enh2);
	g_sdio.dPrint(N_ERR, "[nmi sdio]: reg_0xf6_not_clear_bug_fix = %d...\n", g_sdio.reg_0xf6_not_clear_bug_fix);
#endif


	return 1;

_fail_:

	return 0;
}

static void sdio_set_max_speed(void)
{
	g_sdio.sdio_set_max_speed();
}

static void sdio_set_default_speed(void)
{
	g_sdio.sdio_set_default_speed();
}
#if DMA_VER == DMA_VER_2

static int sdio_write_ext(uint32_t addr, uint8_t *buf, uint32_t size)
{
	int ret;

	ret = sdio_write(addr, buf, size);

	if(g_sdio.reg_0xf6_not_clear_bug_fix) {/* bug 4456 and 4557 */
		if(g_sdio.has_thrpt_enh) {
			sdio_cmd52_t cmd;
		
			cmd.read_write = 1;
			cmd.function = 0;
			cmd.raw = 0;
			cmd.address = 0xf6;
			cmd.data = 0;	
			if(!g_sdio.sdio_cmd52(&cmd)) {
				g_sdio.dPrint(N_ERR, "[nmi sdio]: Fail cmd 52, set 0xf6 register (%d) ...\n", __LINE__);
				goto _fail_;
			}
		}
	}
	return ret;	

_fail_:
	return 0;

}

static int sdio_read_ext(uint32_t addr, uint8_t *buf, uint32_t size)
{
	int ret;

	
	ret = sdio_read(addr, buf, size);

	if(g_sdio.reg_0xf6_not_clear_bug_fix) { /* bug 4456 and 4557 */
		if(g_sdio.has_thrpt_enh) {
			sdio_cmd52_t cmd;
		
			cmd.read_write = 1;
			cmd.function = 0;
			cmd.raw = 0;
			cmd.address = 0xf6;
			cmd.data = 0;
			if(!g_sdio.sdio_cmd52(&cmd)) {
				g_sdio.dPrint(N_ERR, "[nmi sdio]: Fail cmd 52, set 0xf6 register (%d) ...\n", __LINE__);
				goto _fail_;
			}
		}
	}

	return ret;

_fail_:
	return 0;
}



static int sdio_read_size(uint32_t * size)
{
	
	uint32_t tmp;	
	sdio_cmd52_t cmd;

	/**
		Read DMA count in words
	**/	
	if (g_sdio.has_thrpt_enh) {
		cmd.read_write = 0;
		cmd.function = 0;
		cmd.raw = 0;
		cmd.address = 0xf2;
		cmd.data = 0;
		g_sdio.sdio_cmd52(&cmd);
		tmp = cmd.data;
		//printk("%s @ %d: sdio read %x = %x\n", __FUNCTION__, __LINE__, cmd.address, cmd.data);

		//cmd.read_write = 0;
		//cmd.function = 0;
		//cmd.raw = 0;
		cmd.address = 0xf3;
		cmd.data = 0;
		g_sdio.sdio_cmd52(&cmd);
		tmp |= (cmd.data << 8);	
		//printk("%s @ %d: sdio read %x = %x\n", __FUNCTION__, __LINE__, cmd.address, cmd.data);
	} else {

		uint32_t byte_cnt;

#ifndef NMI_SDIO_IRQ_GPIO
		byte_cnt = sdio_xfer_cnt(); /* In DAT1 muxed interrupt case, read DMA count using fn0 regs. */
#else
		/*sdio_read_reg(0xf01c, &byte_cnt);*/ /*Bug 4534 */
		/*byte_cnt = sdio_xfer_cnt();*/ 		 /*Bug 4535 */
		sdio_read_reg(NMI_VMM_TO_HOST_SIZE, &byte_cnt); /* in IRQN interrupt case, read DMA count over AHB. */ 
		
		
#endif
		
		tmp = (byte_cnt >> 2) & IRQ_DMA_WD_CNT_MASK;
	}
	
	*size=tmp;	
	return 1;
}

static int sdio_read_int(uint32_t * int_status)
{
	
	uint32_t tmp;
	sdio_cmd52_t cmd;

	sdio_read_size(&tmp);

	/**
		Read IRQ flags
	**/	
#ifndef NMI_SDIO_IRQ_GPIO
	//cmd.read_write = 0;
	cmd.function = 1;
	//cmd.raw = 0;
	cmd.address = 0x04;
	cmd.data = 0;
	g_sdio.sdio_cmd52(&cmd);
	//printk("%s @ %d: sdio read %x = %x\n", __FUNCTION__, __LINE__, cmd.address, cmd.data);
	if(cmd.data & (1 << 0)) {
		tmp |= INT_0;
	} 
	if(cmd.data & (1 << 2)) {
		tmp |= INT_1;
	} 
	if(cmd.data & (1 << 3)) {
		tmp |= INT_2;
	} 
	if(cmd.data & (1 << 4)) {		
		tmp |= INT_3;
	}	
	if(cmd.data & (1 << 5)) {		
		tmp |= INT_4;
	}
	if(cmd.data & (1 << 6)) {		
		tmp |= INT_5;
	}
	{
		int i;
		for(i=g_sdio.nint; i<MAX_NUM_INT; i++) {
			if((tmp >> (IRG_FLAGS_OFFSET+i)) & 0x1) {
				g_sdio.dPrint(N_ERR, "[nmi sdio]: Unexpected interrupt (1) : tmp=%x, data=%x\n", tmp, cmd.data);
				break;
			}
		}
	}
#else
	if (g_sdio.has_thrpt_enh2) {
		uint32_t irq_flags;

		cmd.read_write = 0;
		cmd.function = 0;
		cmd.raw = 0;
		cmd.address = 0xf7;
		cmd.data = 0;
		g_sdio.sdio_cmd52(&cmd);
		irq_flags = cmd.data & 0x1f;
		tmp |= ((irq_flags >> 0) << IRG_FLAGS_OFFSET);		
	} else {
		int happended, j;

		j = 0;
		do {
			uint32_t irq_flags;

			happended = 0;

			sdio_read_reg(0x1a90, &irq_flags);		
			tmp |= ((irq_flags >> 27) << IRG_FLAGS_OFFSET);
			
			if(g_sdio.nint > 5) {
				sdio_read_reg(0x1a94, &irq_flags);		
				tmp |= (((irq_flags >> 0) & 0x7) << (IRG_FLAGS_OFFSET+5));
			}		
			
			{
				uint32_t unkmown_mask;
				
				unkmown_mask = ~((1ul << g_sdio.nint) - 1);
				
				if( (tmp >> IRG_FLAGS_OFFSET) & unkmown_mask) {
					g_sdio.dPrint(N_ERR, "[nmi sdio]: Unexpected interrupt (2): j=%d, tmp=%x, mask=%x\n", j, tmp, unkmown_mask);
					happended = 1;
				}
			}
			j++;
		} while(happended);
	}

#endif
	
	*int_status = tmp;

	//printk("int_status = %x\n", *int_status);

	return 1;
}

static int sdio_clear_int_ext(uint32_t val)
{
	int ret;

	//printk("sdio_clear_int_ext = %x\n", val);	

#ifdef NMI_SDIO_IRQ_GPIO
	if (g_sdio.has_thrpt_enh2) {
		/* see below. has_thrpt_enh2 uses register 0xf8 to clear interrupts. */
		/* Cannot clear multiple interrupts. Must clear each interrupt individually */
		uint32_t flags;
		flags = val & ((1 << MAX_NUM_INT) - 1);
		if(flags) {
			int i;

			ret = 1;			
			for(i=0; i<g_sdio.nint; i++) {
				if(flags & 1) {
					sdio_cmd52_t cmd;
					cmd.read_write = 1;
					cmd.function = 0;
					cmd.raw = 0;
					cmd.address = 0xf8;
					cmd.data = (1 << i);
					
					ret = g_sdio.sdio_cmd52(&cmd);
					if (!ret) {
						g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd52, set 0xf8 data (%d) ...\n", __LINE__);
						goto _fail_;
					}
				
				}
				if(!ret) break;
				flags >>= 1;
			}
			if (!ret) {
				goto _fail_;
			}
			for(i=g_sdio.nint; i<MAX_NUM_INT; i++) {
				if(flags & 1) g_sdio.dPrint(N_ERR, "[nmi sdio]: Unexpected interrupt cleared %d...\n", i);
				flags >>= 1;	
			}
		}
	} else {
		uint32_t flags;
		flags = val & ((1 << MAX_NUM_INT) - 1);
		if(flags) {
			int i;

			ret = 1;			
			for(i=0; i<g_sdio.nint; i++) {
				/* No matter what you write 1 or 0, it will clear interrupt. */
				if(flags & 1) ret = sdio_write_reg(0x10c8+i*4, 1); 
				if(!ret) break;
				flags >>= 1;
			}
			if (!ret) {
				g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed sdio_write_reg, set reg %x ...\n", 0x10c8+i*4);
				goto _fail_;
			}
			for(i=g_sdio.nint; i<MAX_NUM_INT; i++) {
				if(flags & 1) g_sdio.dPrint(N_ERR, "[nmi sdio]: Unexpected interrupt cleared %d...\n", i);
				flags >>= 1;	
			}
		}
	}
#endif /* NMI_SDIO_IRQ_GPIO */


	if(g_sdio.has_thrpt_enh) {
		uint32_t vmm_ctl;

		vmm_ctl = 0;		
		/* select VMM table 0 */
		if((val & SEL_VMM_TBL0) == SEL_VMM_TBL0) vmm_ctl |= (1 << 0);
		/* select VMM table 1 */
		if((val & SEL_VMM_TBL1) == SEL_VMM_TBL1) vmm_ctl |= (1 << 1);		
		/* enable VMM */
		if((val & EN_VMM) == EN_VMM)             vmm_ctl |= (1 << 2);

		if(vmm_ctl) {	
			sdio_cmd52_t cmd;

			cmd.read_write = 1;
			cmd.function = 0;
			cmd.raw = 0;
			cmd.address = 0xf6;
			cmd.data = vmm_ctl;
			//printk("%s @ %d: sdio read %x = %x\n", __FUNCTION__, __LINE__, cmd.address, cmd.data);
			ret = g_sdio.sdio_cmd52(&cmd);
			if (!ret) {
				g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd52, set 0xf6 data (%d) ...\n", __LINE__);
				goto _fail_;
			}
		}
	} else { /* g_sdio.has_thrpt_enh */
			uint32_t tbl_ctl;

			tbl_ctl = 0;
			/* select VMM table 0 */
			if((val & SEL_VMM_TBL0) == SEL_VMM_TBL0) tbl_ctl |= (1 << 0);
			/* select VMM table 1 */
			if((val & SEL_VMM_TBL1) == SEL_VMM_TBL1) tbl_ctl |= (1 << 1);
			
			if(tbl_ctl) { /* fix bug 4588 */
				ret = sdio_write_reg(NMI_VMM_TBL_CTL, tbl_ctl);
				if (!ret) {
					g_sdio.dPrint(N_ERR, "[nmi sdio]: fail write reg vmm_tbl_ctl...\n");
					goto _fail_;
				}
			}

			if((val & EN_VMM) == EN_VMM) {
				/**
					enable vmm transfer. 
				**/

			if(!tbl_ctl) { /* fix bug 4588 */
				ret = sdio_write_reg(NMI_VMM_CORE_CTL, 1);
				if (!ret) {
					g_sdio.dPrint(N_ERR, "[nmi sdio]: fail write reg vmm_core_ctl...\n");
					goto _fail_;
				}
			} else {
				uint32_t data;
				sdio_cmd53_t cmd;


				/**
					Write NMI_VMM_CORE_CTL the way below to save 2 CMD 52. 
					The code should look like #if 0'ed code below. After writing 
					NMI_VMM_TBL_CTL, then CSA address is now NMI_VMM_TBL_CTL+4,
					then the least significant byte of the CSA address should 
					be changed to 0.
				**/
					
					data = 1;
				/**
					set the AHB address
				**/
				if (!sdio_set_func0_csa_address_byte0(NMI_VMM_CORE_CTL)) 
					goto _fail_;
		
				cmd.read_write = 1;
				cmd.function = 0;
				cmd.address = 0x10f;
				cmd.block_mode = 0;
				cmd.increment = 1;
				cmd.count = 4;
				cmd.block_size = g_sdio.block_size; //johnny : prevent it from setting unexpected value
				cmd.buffer = (uint8_t *)&data;
				if (!g_sdio.sdio_cmd53(&cmd)) {
					g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed cmd53, write NMI_VMM_CORE_CTL...\n");
					goto _fail_;
				}	
			}
		}
	}

	return 1;
_fail_:
	return 0;
}

static int sdio_sync_ext(int nint /*  how mant interrupts to enable. */)
{
	uint32_t reg;


	if(nint > MAX_NUM_INT) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Too many interupts (%d)...\n", nint);
		return 0;
	}
	if(g_sdio.has_thrpt_enh2) {
		if(nint > 5) {
			g_sdio.dPrint(N_ERR, "[nmi sdio]: Error: Cannot support more than 5 interrupts when has_thrpt_enh2=1.\n");
			return 0;
		}
	}

	g_sdio.nint = nint;

	/**
		Disable power sequencer
	**/
	if (!sdio_read_reg(NMI_MISC, &reg)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed read misc reg...\n");
		return 0;
	}
	
	reg &= ~(1 << 8);
	if (!sdio_write_reg(NMI_MISC, reg)) {
		g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed write misc reg...\n");
		return 0;
	}

#ifdef NMI_SDIO_IRQ_GPIO
	{
		uint32_t reg;
		int ret, i;


		/**
			interrupt pin mux select
		**/
		ret = sdio_read_reg(NMI_PIN_MUX_0, &reg);
		if (!ret) {
			g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed read reg (%08x)...\n", NMI_PIN_MUX_0);
			return 0;
		}
		reg |= (1<<8);
		ret = sdio_write_reg(NMI_PIN_MUX_0, reg);
		if (!ret) {
			g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed write reg (%08x)...\n", NMI_PIN_MUX_0);
			return 0;
		}

		/**
			interrupt enable
		**/
		ret = sdio_read_reg(NMI_INTR_ENABLE, &reg);
		if (!ret) {
			g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed read reg (%08x)...\n", NMI_INTR_ENABLE);
			return 0;
		}

		for(i=0; (i < 5) && (nint > 0); i++, nint--) {
			reg |= (1 << (27+i)); 
		}
		ret = sdio_write_reg(NMI_INTR_ENABLE, reg);
		if (!ret) {
			g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed write reg (%08x)...\n", NMI_INTR_ENABLE);
			return 0;
		}
		if(nint) {
			ret = sdio_read_reg(NMI_INTR2_ENABLE, &reg);
			if (!ret) {
				g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed read reg (%08x)...\n", NMI_INTR2_ENABLE);
				return 0;
			}
			
			for(i=0; (i < 3) && (nint > 0); i++, nint--) {
				reg |= (1 << i); 
			}				
			
			ret = sdio_read_reg(NMI_INTR2_ENABLE, &reg);
			if (!ret) {
				g_sdio.dPrint(N_ERR, "[nmi sdio]: Failed write reg (%08x)...\n", NMI_INTR2_ENABLE);
				return 0;
			}
		}
	}
#endif /* NMI_SDIO_IRQ_GPIO */
	return 1;
}

#endif /* DMA_VER == DMA_VER_2 */
/********************************************

	Global sdio HIF function table

********************************************/

nmi_hif_func_t hif_sdio = {
	sdio_init,
	sdio_deinit,
	sdio_read_reg,
	sdio_write_reg,
	sdio_read,
	sdio_write,
	sdio_sync,
	sdio_clear_int,
#if DMA_VER == DMA_VER_2
	sdio_read_int,
	sdio_clear_int_ext,
	sdio_read_size,
	sdio_write_ext,
	sdio_read_ext,
	sdio_sync_ext,
#endif /* DMA_VER == DMA_VER_2 */
	sdio_set_max_speed,
	sdio_set_default_speed,
};

