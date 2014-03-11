#include "nmi_wlan_if.h"
#include "nmi_wlan.h"
#include <linux/delay.h>

extern int linux_spi_write(uint8_t* b, uint32_t len);
extern int linux_spi_read(unsigned char*rb, unsigned long rlen);
extern int linux_spi_write_read(unsigned char*wb, unsigned char*rb, unsigned int rlen);

static int custom_crc_off;
static int custom_spi_read(uint32_t, uint8_t *, uint32_t);
static int custom_spi_write(uint32_t, uint8_t *, uint32_t);

/********************************************

	Crc7

********************************************/

static const uint8_t custom_crc7_syndrome_table[256] = {
	0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
	0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
	0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
	0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
	0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
	0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
	0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
	0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
	0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
	0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
	0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
	0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
	0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
	0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
	0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
	0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
	0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
	0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
	0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
	0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
	0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
	0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
	0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
	0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
	0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
	0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
	0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
	0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
	0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
	0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
	0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
	0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
};

static uint8_t custom_crc7_byte(uint8_t crc, uint8_t data)
{
	return custom_crc7_syndrome_table[(crc << 1) ^ data];
}

static uint8_t custom_crc7(uint8_t crc, const uint8_t *buffer, uint32_t len)
{
	while (len--)
		crc = custom_crc7_byte(crc, *buffer++);
	return crc;
}

/********************************************

	Spi protocol Function

********************************************/

#define CUSTOME_CMD_DMA_WRITE				0xc1
#define CUSTOME_CMD_DMA_READ				0xc2
#define CUSTOME_CMD_INTERNAL_WRITE		0xc3
#define CUSTOME_CMD_INTERNAL_READ		0xc4
#define CUSTOME_CMD_TERMINATE				0xc5
#define CUSTOME_CMD_REPEAT					0xc6
#define CUSTOME_CMD_DMA_EXT_WRITE		0xc7
#define CUSTOME_CMD_DMA_EXT_READ		0xc8
#define CUSTOME_CMD_SINGLE_WRITE			0xc9
#define CUSTOME_CMD_SINGLE_READ			0xca
#define CUSTOME_CMD_RESET						0xcf

#define _OK								1
#define _FAIL								0
#define _RESET							-1
#define _RETRY							-2

#define PKT_SZ_256 			256
#define PKT_SZ_512			512
#define PKT_SZ_1K				1024
#define PKT_SZ_4K				(4 * 1024)
#define PKT_SZ_8K				(8 * 1024)
#define PKT_SZ					PKT_SZ_8K

static int custom_spi_cmd(uint8_t cmd, uint32_t adr, uint32_t data, uint32_t sz, uint8_t clockless)
{
	uint8_t bc[9];
	int len = 5;
	int result = _OK;

	bc[0] = cmd;
	switch (cmd) {
	case CUSTOME_CMD_SINGLE_READ:				/* single word (4 bytes) read */
		bc[1] = (uint8_t)(adr >> 16);
		bc[2] = (uint8_t)(adr >> 8);
		bc[3] = (uint8_t)adr;
		len = 5;
		break; 
	case CUSTOME_CMD_INTERNAL_READ:			/* internal register read */ 
		bc[1] = (uint8_t)(adr >> 8);
		if(clockless)  bc[1] |= (1 << 7);
		bc[2] = (uint8_t)adr;
		bc[3] = 0x00;
		len = 5;
		break;
	case CUSTOME_CMD_TERMINATE:					/* termination */
		bc[1] = 0x00;
		bc[2] = 0x00;
		bc[3] = 0x00;
		len = 5;
		break;
	case CUSTOME_CMD_REPEAT:						/* repeat */
		bc[1] = 0x00;
		bc[2] = 0x00;
		bc[3] = 0x00;
		len = 5;
		break;
	case CUSTOME_CMD_RESET:							/* reset */
		bc[1] = 0xff;
		bc[2] = 0xff;
		bc[3] = 0xff;
		len = 5;
		break;
	case CUSTOME_CMD_DMA_WRITE:					/* dma write */
	case CUSTOME_CMD_DMA_READ:					/* dma read */
		bc[1] = (uint8_t)(adr >> 16);
		bc[2] = (uint8_t)(adr >> 8);
		bc[3] = (uint8_t)adr;
		bc[4] = (uint8_t)(sz >> 8);
		bc[5] = (uint8_t)(sz);
		len = 7;
		break;
	case CUSTOME_CMD_DMA_EXT_WRITE:		/* dma extended write */
	case CUSTOME_CMD_DMA_EXT_READ:			/* dma extended read */
		bc[1] = (uint8_t)(adr >> 16);
		bc[2] = (uint8_t)(adr >> 8);
		bc[3] = (uint8_t)adr;
		bc[4] = (uint8_t)(sz >> 16);
		bc[5] = (uint8_t)(sz >> 8);
		bc[6] = (uint8_t)(sz);
		len = 8;
		break;
	case CUSTOME_CMD_INTERNAL_WRITE:		/* internal register write */
		bc[1] = (uint8_t)(adr >> 8);
		if(clockless)  bc[1] |= (1 << 7);
		bc[2] = (uint8_t)(adr);
		bc[3] = (uint8_t)(data >> 24);
		bc[4] = (uint8_t)(data >> 16);
		bc[5] = (uint8_t)(data >> 8);
		bc[6] = (uint8_t)(data);
		len = 8;
		break;
	case CUSTOME_CMD_SINGLE_WRITE:			/* single word write */
		bc[1] = (uint8_t)(adr >> 16);
		bc[2] = (uint8_t)(adr >> 8);
		bc[3] = (uint8_t)(adr);
		bc[4] = (uint8_t)(data >> 24);
		bc[5] = (uint8_t)(data >> 16);
		bc[6] = (uint8_t)(data >> 8);
		bc[7] = (uint8_t)(data);
		len = 9;
		break;
	default:
		result = _FAIL;
		break;
	}
	
	if (result) {
		if (!custom_crc_off)
			bc[len-1] = (custom_crc7(0x7f, (const uint8_t *)&bc[0], len-1)) << 1;
		else
			len-=1;

		if (!linux_spi_write(bc, len)) {
			printk( "[custom spi]: Failed cmd write, bus error...\n");
			result = _FAIL;
		}
	}

	return result;
}

static int custom_spi_cmd_rsp(uint8_t cmd)
{
	uint8_t rsp;
	int result = _OK;

	/**
		Command/Control response
	**/
	if ((cmd == CUSTOME_CMD_RESET) ||
		 (cmd == CUSTOME_CMD_TERMINATE) ||
		 (cmd == CUSTOME_CMD_REPEAT)) {
		if (!linux_spi_read(&rsp, 1)) {
			result = _FAIL;
			goto _fail_;
		}
	}	

	if (!linux_spi_read(&rsp, 1)) {
		printk( "[custom spi]: Failed cmd response read, bus error...\n");
		result = _FAIL;
		goto _fail_;
	}

	if (rsp != cmd) {
		printk( "[custom spi]: Failed cmd response, cmd (%02x), resp (%02x)\n", cmd, rsp);
		result = _FAIL;
		goto _fail_;
	} 

	/**
		State response
	**/
	if (!linux_spi_read(&rsp, 1)) {
		printk( "[custom spi]: Failed cmd state read, bus error...\n");
		result = _FAIL;
		goto _fail_;
	}

	if (rsp != 0x00) {
		printk( "[custom spi]: Failed cmd state response state (%02x)\n", rsp);
		result = _FAIL;
	}

_fail_:

	return result;
}

static int custom_spi_cmd_complete(uint8_t cmd, uint32_t adr, uint8_t *b, uint32_t sz, uint8_t clockless)
{
	uint8_t wb[32], rb[32];
	uint8_t wix, rix;
	uint32_t len2;
	uint8_t rsp;
	int len = 0;
	int result = _OK;

	wb[0] = cmd;
	switch (cmd) {
	case CUSTOME_CMD_SINGLE_READ:				/* single word (4 bytes) read */
		wb[1] = (uint8_t)(adr >> 16);
		wb[2] = (uint8_t)(adr >> 8);
		wb[3] = (uint8_t)adr;
		len = 5;
		break; 
	case CUSTOME_CMD_INTERNAL_READ:			/* internal register read */ 
		wb[1] = (uint8_t)(adr >> 8);
		if(clockless == 1)  wb[1] |= (1 << 7);
		wb[2] = (uint8_t)adr;
		wb[3] = 0x00;
		len = 5;
		break;
	case CUSTOME_CMD_TERMINATE:					/* termination */
		wb[1] = 0x00;
		wb[2] = 0x00;
		wb[3] = 0x00;
		len = 5;
		break;
	case CUSTOME_CMD_REPEAT:						/* repeat */
		wb[1] = 0x00;
		wb[2] = 0x00;
		wb[3] = 0x00;
		len = 5;
		break;
	case CUSTOME_CMD_RESET:							/* reset */
		wb[1] = 0xff;
		wb[2] = 0xff;
		wb[3] = 0xff;
		len = 5;
		break;
	case CUSTOME_CMD_DMA_WRITE:					/* dma write */
	case CUSTOME_CMD_DMA_READ:					/* dma read */
		wb[1] = (uint8_t)(adr >> 16);
		wb[2] = (uint8_t)(adr >> 8);
		wb[3] = (uint8_t)adr;
		wb[4] = (uint8_t)(sz >> 8);
		wb[5] = (uint8_t)(sz);
		len = 7;
		break;
	case CUSTOME_CMD_DMA_EXT_WRITE:		/* dma extended write */
	case CUSTOME_CMD_DMA_EXT_READ:			/* dma extended read */
		wb[1] = (uint8_t)(adr >> 16);
		wb[2] = (uint8_t)(adr >> 8);
		wb[3] = (uint8_t)adr;
		wb[4] = (uint8_t)(sz >> 16);
		wb[5] = (uint8_t)(sz >> 8);
		wb[6] = (uint8_t)(sz);
		len = 8;
		break;
	case CUSTOME_CMD_INTERNAL_WRITE:		/* internal register write */
		wb[1] = (uint8_t)(adr >> 8);
		if(clockless == 1)  wb[1] |= (1 << 7);
		wb[2] = (uint8_t)(adr);
		wb[3] = b[3];
		wb[4] = b[2];
		wb[5] = b[1];
		wb[6] = b[0];
		len = 8;
		break;
	case CUSTOME_CMD_SINGLE_WRITE:			/* single word write */
		wb[1] = (uint8_t)(adr >> 16);
		wb[2] = (uint8_t)(adr >> 8);
		wb[3] = (uint8_t)(adr);
		wb[4] = b[3];
		wb[5] = b[2];
		wb[6] = b[1];
		wb[7] = b[0];
		len = 9;
		break;
	default:
		result = _FAIL;
		break;
	}

	if (result != _OK) {
		return result;
	}

	if (!custom_crc_off) {
		wb[len-1] = (custom_crc7(0x7f, (const uint8_t *)&wb[0], len-1)) << 1;
	} else {
		len -=1;
	}

#define _SKIP_BYTES (1)
#define _RSP_BYTES (2)
#define _DATA_HDR_BYTES (1)
#define _DATA_BYTES (4)
#define _CRC_BYTES (2)
#define _DUMMY_BYTES (3)
	if ((cmd == CUSTOME_CMD_RESET) ||
		(cmd == CUSTOME_CMD_TERMINATE) ||
		(cmd == CUSTOME_CMD_REPEAT)) {
			len2 = len + (_SKIP_BYTES + _RSP_BYTES + _DUMMY_BYTES);
	} else if ((cmd == CUSTOME_CMD_INTERNAL_READ) || (cmd == CUSTOME_CMD_SINGLE_READ)) {
		if (!custom_crc_off) {
			len2 = len + (_RSP_BYTES + _DATA_HDR_BYTES + _DATA_BYTES 
			+ _CRC_BYTES + _DUMMY_BYTES);	
		} else {
			len2 = len + (_RSP_BYTES + _DATA_HDR_BYTES + _DATA_BYTES 
			+ _DUMMY_BYTES);
		}
	} else {
		len2 = len + (_RSP_BYTES + _DUMMY_BYTES);
	}
#undef _DUMMY_BYTES

	if(len2 > (sizeof(wb)/sizeof(wb[0]))) {
		printk( "[custom spi]: spi buffer size too small (%d) (%d)\n",
			len2, (sizeof(wb)/sizeof(wb[0])));
		result = _FAIL;
		return result;
	}
	/* zero spi write buffers. */
	for(wix = len; wix< len2; wix++) {
		wb[wix] = 0;
	}
	rix = len;

	if (!linux_spi_write_read(wb, rb, len2)) {
		printk( "[custom spi]: Failed cmd write, bus error...\n");
		result = _FAIL;
		return result;
	}

#if 0
	{
		int jj;
		printk("--- cnd = %x, len=%d, len2=%d\n", cmd, len, len2);
		for(jj=0; jj<sizeof(wb)/sizeof(wb[0]); jj++) {

			if(jj >= len2) break;
			if(((jj+1)%16) != 0) {
				if((jj%16) == 0) {
					printk("wb[%02x]: %02x ", jj, wb[jj]);
				} else {
					printk("%02x ", wb[jj]);
				}
			} else {
				printk("%02x\n", wb[jj]);
			}
		}
		printk("\n");

		for(jj=0; jj<sizeof(rb)/sizeof(rb[0]); jj++) {

				if(jj >= len2) break;
				if(((jj+1)%16) != 0) {
					if((jj%16) == 0) {
						printk("rb[%02x]: %02x ", jj, rb[jj]);
					} else {
						printk("%02x ", rb[jj]);
					}
				} else {
					printk("%02x\n", rb[jj]);
				}
			}
		printk("\n");
	}
#endif

	/**
	Command/Control response
	**/
	if ((cmd == CUSTOME_CMD_RESET) ||
		(cmd == CUSTOME_CMD_TERMINATE) ||
		(cmd == CUSTOME_CMD_REPEAT)) {
			rix++; /* skip 1 byte */
	}

	//do {	
	rsp = rb[rix++];
	//	if(rsp == cmd) break;
	//} while(&rptr[1] <= &rb[len2]);

	if (rsp != cmd) {
		printk( "[custom spi]: Failed cmd response, cmd (%02x)"
			", resp (%02x)\n", cmd, rsp);
		result = _FAIL;
		return result;
	}

	/**
	State response
	**/
	rsp = rb[rix++];
	if (rsp != 0x00) {
		printk( "[custom spi]: Failed cmd state response "
			"state (%02x)\n", rsp);
		result = _FAIL;
		return result;
	}

	if ((cmd == CUSTOME_CMD_INTERNAL_READ) || (cmd == CUSTOME_CMD_SINGLE_READ)
		|| (cmd == CUSTOME_CMD_DMA_READ) || (cmd == CUSTOME_CMD_DMA_EXT_READ)) {
			int retry;
			//uint16_t crc1, crc2;
			uint8_t crc[2];
			/**
			Data Respnose header
			**/
			retry = 100;
			do {
				/* ensure there is room in buffer later to read data and crc */
				if(rix < len2) { 
					rsp = rb[rix++];
				} else {
					retry = 0;
					break;
				}
				if (((rsp >> 4) & 0xf) == 0xf)
					break;
			} while (retry--);

			if (retry <= 0) {
				printk( "[custom spi]: Error, data read "
					"response (%02x)\n", rsp);
				result = _RESET;
				return result;
			}

			if ((cmd == CUSTOME_CMD_INTERNAL_READ) || (cmd == CUSTOME_CMD_SINGLE_READ)) {
				/**
				Read bytes
				**/
				if((rix+3) < len2) { 
					b[0] = rb[rix++];
					b[1] = rb[rix++];
					b[2] = rb[rix++];
					b[3] = rb[rix++];
				} else {
					printk( "[custom spi]: buffer overrun when reading data.\n");
					result = _FAIL;
					return result;
				}

				if (!custom_crc_off) {						
					/**
					Read Crc
					**/
					if((rix+1) < len2) { 
						crc[0] = rb[rix++];
						crc[1] = rb[rix++];
					} else {
						printk( "[custom spi]: buffer overrun when reading crc.\n");
						result = _FAIL;
						return result;
					}
				}
			} else if((cmd == CUSTOME_CMD_DMA_READ) || (cmd == CUSTOME_CMD_DMA_EXT_READ)) {
				int ix;

				/* some data may be read in response to dummy bytes. */
				for(ix=0; (rix < len2) && (ix < sz);) {
					b[ix++] = rb[rix++];				
				}
#if 0
				if(ix) printk("ttt %d %d\n", sz, ix);
#endif
				sz -= ix;

				if(sz > 0) {
					int nbytes;
					
					if (sz <= (PKT_SZ-ix)) {
						nbytes = sz;
					} else {
						nbytes = PKT_SZ-ix;
					}

					/**
					Read bytes
					**/
					if (!linux_spi_read(&b[ix], nbytes)) {
						printk( "[custom spi]: Failed data block read, bus error...\n");
						result = _FAIL;
						goto _error_;
					}

					/**
					Read Crc
					**/
					if (!custom_crc_off) {
						if (!linux_spi_read(crc, 2)) {
							printk( "[custom spi]: Failed data block crc read, bus error...\n");
							result = _FAIL;
							goto _error_;
						}
					}

					
					ix += nbytes;
					sz -= nbytes;
				}

				/*  if any data in left unread, then read the rest using normal DMA code.*/	
				while(sz > 0) {
					int nbytes;

#if 0
					printk("rrr %d %d\n", sz, ix);
#endif				
					if (sz <= PKT_SZ) {
						nbytes = sz;
					} else {
						nbytes = PKT_SZ;
					}

					/** 
					read data response only on the next DMA cycles not 
					the first DMA since data response header is already 
					handled above for the first DMA.
					**/
					/**
					Data Respnose header
					**/
					retry = 10;
					do {
						if (!linux_spi_read(&rsp, 1)) {
							printk( "[custom spi]: Failed data response read, bus error...\n");
							result = _FAIL;
							break;
						}
						if (((rsp >> 4) & 0xf) == 0xf)
							break;
					} while (retry--);

					if (result == _FAIL)
						break;


					/**
					Read bytes
					**/
					if (!linux_spi_read(&b[ix], nbytes)) {
						printk( "[custom spi]: Failed data block read, bus error...\n");
						result = _FAIL;
						break;
					}

					/**
					Read Crc
					**/
					if (!custom_crc_off) {
						if (!linux_spi_read(crc, 2)) {
							printk( "[custom spi]: Failed data block crc read, bus error...\n");
							result = _FAIL;
							break;
						}
					}

					ix += nbytes;
					sz -= nbytes;
				}
			}
	}
_error_:
	return result;
}

static int custom_spi_data_read(uint8_t *b, uint32_t sz)
{
	int retry, ix, nbytes;
	int result = _OK;
	uint8_t crc[2];
	uint8_t rsp;

	/**
		Data
	**/
	ix = 0;
	do {
		if (sz <= PKT_SZ)
			nbytes = sz;
		else
			nbytes = PKT_SZ;

		/**
			Data Respnose header
		**/
		retry = 10;
		do {
			if (!linux_spi_read(&rsp, 1)) {
				printk( "[custom spi]: Failed data response read, bus error...\n");
				result = _FAIL;
				break;
			}
			if (((rsp >> 4) & 0xf) == 0xf)
				break;
		} while (retry--);
		
		if (result == _FAIL)
			break;

		if (retry <= 0) {
			printk( "[custom spi]: Failed data response read...(%02x)\n", rsp);
			result = _FAIL;
			break;
		}

		/**
			Read bytes
		**/
		if (!linux_spi_read(&b[ix], nbytes)) {
			printk( "[custom spi]: Failed data block read, bus error...\n");
			result = _FAIL;
			break;
		}

		/**
			Read Crc
		**/
		if (!custom_crc_off) {
			if (!linux_spi_read(crc, 2)) {
				printk( "[custom spi]: Failed data block crc read, bus error...\n");
				result = _FAIL;
				break;
			}
		}

		ix += nbytes;
		sz -= nbytes;

	} while (sz);

	return result;
}

static int custom_spi_data_write(uint8_t *b, uint32_t sz)
{
	int ix, nbytes;
	int result = 1;
	uint8_t cmd, order, crc[2] = {0};
	//uint8_t rsp;

	/**
		Data
	**/
	ix = 0;
	do {
		if (sz <= PKT_SZ)
			nbytes = sz;
		else
			nbytes = PKT_SZ;

		/**
			Write command
		**/
		cmd = 0xf0;
		if (ix == 0)  {
			if (sz <= PKT_SZ)

				order = 0x3;
			else
				order = 0x1;
		} else {
			if (sz <= PKT_SZ)
				order = 0x3;
			else
				order = 0x2;
		}
		cmd |= order;	
		if (!linux_spi_write(&cmd, 1)) {
			printk( "[custom spi]: Failed data block cmd write, bus error...\n");
			result = _FAIL;
			break;
		}

		/**
			Write data
		**/
		if (!linux_spi_write(&b[ix], nbytes)) {
			printk( "[custom spi]: Failed data block write, bus error...\n");
			result = _FAIL;
			break;
		}

		/**
			Write Crc
		**/
		if (!custom_crc_off) {
			if (!linux_spi_write(crc, 2)) {
				printk( "[custom spi]: Failed data block crc write, bus error...\n");
				result = _FAIL;
				break;
			}
		}

		/**
			No need to wait for response
		**/
#if 0  
		/**
			Respnose
		**/
		if (!linux_spi_read(&rsp, 1)) {
			printk( "[custom spi]: Failed data block write, response read, bus error...\n");
			result = _FAIL;
			break;
		}

		if (((rsp >> 4) & 0xf) != 0xc) {
			result = _FAIL;
			printk( "[custom spi]: Failed data block write response...(%02x)\n", rsp);
			break; 
		}
	
		/**
			State
		**/
		if (!linux_spi_read(&rsp, 1)) {
			printk( "[custom spi]: Failed data block write, read state, bus error...\n");
			result = _FAIL;
			break;
		}
#endif

		ix += nbytes;
		sz -= nbytes;
	} while (sz);


	return result;
}

/********************************************

	Spi Internal Read/Write Function

********************************************/

static int custom_spi_internal_write(uint32_t adr, uint32_t dat)
{
	int result;

#if defined USE_OLD_SPI_SW
	/**
		Command 
	**/
	result = custom_spi_cmd(CUSTOME_CMD_INTERNAL_WRITE, adr, dat, 4,0);
	if (result != _OK) {
		printk( "[custom spi]: Failed internal write cmd...\n");
		return 0;		
	}
	 
	result = custom_spi_cmd_rsp(CUSTOME_CMD_INTERNAL_WRITE,0);
	if (result != _OK) {
		printk( "[custom spi]: Failed internal write cmd response...\n");		
	}
#else

#ifdef BIG_ENDIAN
	dat = BYTE_SWAP(dat);
#endif
	result = custom_spi_cmd_complete(CUSTOME_CMD_INTERNAL_WRITE, adr, (uint8_t*)&dat, 4,0);
	if (result != _OK) {
		printk( "[custom spi]: Failed internal write cmd...\n");
	}

#endif
	return result;
}

static int custom_spi_internal_read(uint32_t adr, uint32_t *data)
{
	int result;

#if defined USE_OLD_SPI_SW
	result = custom_spi_cmd(CUSTOME_CMD_INTERNAL_READ, adr, 0, 4, 0);
	if (result != _OK) {
		printk( "[custom spi]: Failed internal read cmd...\n");
		return 0;		
	}
 
	result = custom_spi_cmd_rsp(CUSTOME_CMD_INTERNAL_READ, 0);
	if (result != _OK) {
		printk( "[custom spi]: Failed internal read cmd response...\n");
		return 0;
	}

	/**
		Data
	**/
	result = custom_spi_data_read((uint8_t *)data, 4);
	if (result != _OK) {
		printk( "[custom spi]: Failed internal read data...\n");
		return 0;
	}
#else
	result = custom_spi_cmd_complete(CUSTOME_CMD_INTERNAL_READ, adr, (uint8_t*)data, 4, 0);
	if (result != _OK) {
		printk( "[custom spi]: Failed internal read cmd...\n");
		return 0;
	}
#endif


#ifdef BIG_ENDIAN
	*data = BYTE_SWAP(*data);
#endif

	return 1;
}

/********************************************

	Spi interfaces

********************************************/

int custom_spi_write_reg(uint32_t addr, uint32_t data)
{
	int result = _OK;
	uint8_t cmd = CUSTOME_CMD_SINGLE_WRITE;
	uint8_t clockless = 0;
	

#if defined USE_OLD_SPI_SW
	{
		result = custom_spi_cmd(cmd, addr, data, 4, 0);
		if (result != _OK) {
			printk( "[custom spi]: Failed cmd, write reg (%08x)...\n", addr);	
			return 0;
		}
		 
		result = custom_spi_cmd_rsp(cmd, 0);
		if (result != _OK) {
			printk( "[custom spi]: Failed cmd response, write reg (%08x)...\n", addr);	
			return 0;
		}

		return 1;
	}
#else
#ifdef BIG_ENDIAN
		data= BYTE_SWAP(data);
#endif
	if(addr < 0x30)
	{
		/* Clockless register*/
		cmd = CUSTOME_CMD_INTERNAL_WRITE;
		clockless = 1;
	}
	
	result = custom_spi_cmd_complete(cmd, addr, (uint8_t*)&data, 4, clockless);
	if (result != _OK) {
		printk( "[custom spi]: Failed cmd, write reg (%08x)...\n", addr);		
	}

	return result;
#endif

}

static int custom_spi_write(uint32_t addr, uint8_t *buf, uint32_t size)
{
	int result;
	uint8_t cmd = CUSTOME_CMD_DMA_EXT_WRITE;

	/**
		has to be greated than 4
	**/
	if (size <= 4)
		return 0;

#if defined USE_OLD_SPI_SW
	/**
		Command 
	**/
	result = custom_spi_cmd(cmd, addr, 0, size, 0);
	if (result != _OK) {
		printk( "[custom spi]: Failed cmd, write block (%08x)...\n", addr);		
		return 0;
	}
 
	result = custom_spi_cmd_rsp(cmd, 0);
	if (result != _OK) {
		printk( "[custom spi ]: Failed cmd response, write block (%08x)...\n", addr);
		return 0;		
	}
#else
	result = custom_spi_cmd_complete(cmd, addr, NULL, size, 0);
	if (result != _OK) {
		printk( "[custom spi]: Failed cmd, write block (%08x)...\n", addr);		
		return 0;
	}
#endif

	/**
		Data
	**/
	result = custom_spi_data_write(buf, size);
	if (result != _OK) {
		printk( "[custom spi]: Failed block data write...\n");
	}
		
	return 1;
}

 int custom_spi_read_reg(uint32_t addr, uint32_t *data)
{
	int result = _OK;
	uint8_t cmd = CUSTOME_CMD_SINGLE_READ;
	uint8_t clockless = 0;

#if defined USE_OLD_SPI_SW
	result = custom_spi_cmd(cmd, addr, 0, 4, 0);
	if (result != _OK) {
		printk( "[custom spi]: Failed cmd, read reg (%08x)...\n", addr);
		return 0;
	}  
	result = custom_spi_cmd_rsp(cmd, 0);
	if (result != _OK) {
		printk( "[custom spi]: Failed cmd response, read reg (%08x)...\n", addr);
		return 0;
	}

	result = custom_spi_data_read((uint8_t *)data, 4);
	if (result != _OK) {
		printk( "[custom spi]: Failed data read...\n");
		return 0;
	}
#else
	if(addr < 0x30)
	{
		//printk("***** read addr %d\n\n", addr);
		/* Clockless register*/
		cmd = CUSTOME_CMD_INTERNAL_READ;
		clockless = 1;
	}
	
	result = custom_spi_cmd_complete(cmd, addr, (uint8_t*)data, 4, clockless);
	if (result != _OK) {
		printk( "[custom spi]: Failed cmd, read reg (%08x)...\n", addr);
		return 0;
	}  
#endif


#ifdef BIG_ENDIAN
	*data = BYTE_SWAP(*data);	
#endif
	
	return 1;
}

static int custom_spi_read(uint32_t addr, uint8_t *buf, uint32_t size)
{
	uint8_t cmd = CUSTOME_CMD_DMA_EXT_READ;
	int result;

	if (size <= 4)
		return 0;

#if defined USE_OLD_SPI_SW
	/**
		Command 
	**/
	result = custom_spi_cmd(cmd, addr, 0, size, 0);
	if (result != _OK) {
		printk( "[custom spi]: Failed cmd, read block (%08x)...\n", addr);
		return 0;
	}  
 
	result = custom_spi_cmd_rsp(cmd, 0);
	if (result != _OK) {
		printk( "[custom spi]: Failed cmd response, read block (%08x)...\n", addr);
		return 0;
	}

	/**
		Data
	**/
	result = custom_spi_data_read(buf, size);
	if (result != _OK) {
		printk( "[custom spi]: Failed block data read...\n");
		return 0;
	}
#else
		result = custom_spi_cmd_complete(cmd, addr, buf, size, 0);
		if (result != _OK) {
			printk( "[custom spi]: Failed cmd, read block (%08x)...\n", addr);
			return 0;
		}  
#endif

		
	return 1;
}

/********************************************

	Bus interfaces

********************************************/
#if 0
static int custom_spi_clear_int(void)
{
	uint32_t reg;
	if (!custom_spi_read_reg(NMI_HOST_RX_CTRL_0, &reg)) {
		printk( "[custom spi]: Failed read reg (%08x)...\n", NMI_HOST_RX_CTRL_0);
		return 0;
	}
	reg &= ~0x1;
	custom_spi_internal_read(NMI_HOST_RX_CTRL_0, reg);
		int_clrd++;
	return 1;
}
#endif
#define CUSTOM_NMI_SPI_REG_BASE 0xe800
#define CUSTOM_NMI_SPI_PROTOCOL_CONFIG (CUSTOM_NMI_SPI_REG_BASE+0x24)
#define CUSTOM_NMI_SPI_PROTOCOL_OFFSET (CUSTOM_NMI_SPI_PROTOCOL_CONFIG-CUSTOM_NMI_SPI_REG_BASE)
uint32_t g_spi_already_init = 0;
extern volatile int gbCrashRecover;
int custom_spi_init(void)
{
	uint32_t reg;
	uint32_t chipid;

	static int isinit = 0;
#if 0
	if(isinit && !gbCrashRecover) {

		if (!custom_spi_read_reg(0x1000, &chipid)) {
			printk( "[custom spi]: Fail cmd read chip id...\n");
			return 0;
		}
		printk("<< Custom SPI INIT RETURN\n");
		return 1;
	}
#endif
printk("Custom INIT ...\n");
	/**
		configure protocol 
	**/
	custom_crc_off = 0;
	
	// TODO: We can remove the CRC trials if there is a definite way to reset 
	// the SPI to it's initial value.
	if (!custom_spi_internal_read(CUSTOM_NMI_SPI_PROTOCOL_OFFSET, &reg)) {
		/* Read failed. Try with CRC off. This might happen when module 
		is removed but chip isn't reset*/
		custom_crc_off = 1;
		printk( "[custom spi]: Failed internal read protocol with CRC on, retyring with CRC off...\n");
		if (!custom_spi_internal_read(CUSTOM_NMI_SPI_PROTOCOL_OFFSET, &reg)){
			// Reaad failed with both CRC on and off, something went bad
			printk( "[custom spi]: Failed internal read protocol...\n");
			return 0;
		}
	}
	if(custom_crc_off == 0)
	{
		reg &= ~0xc;	/* disable crc checking */
		reg &= ~0x70;
		reg |= (0x5 << 4);
		if (!custom_spi_internal_write(CUSTOM_NMI_SPI_PROTOCOL_OFFSET, reg)) {
			printk( "[custom spi]: Failed internal write protocol reg...\n");
			return 0;
		}
		custom_crc_off = 1;
	}

	printk(">> CUSTOM_CRC: %d <<\n",custom_crc_off);

	/**
		make sure can read back chip id correctly
	**/
	if (!custom_spi_read_reg(0x1000, &chipid)) {
		printk( "[custom spi]: Fail cmd read chip id...\n");
		return 0;
	}
	
	isinit = 1;
	g_spi_already_init = 1;
	return 1;
}

#define _ISNMC1000(id)   (((id & 0xfffff000) == 0x100000) ? 1 : 0) 
uint32_t custom_get_chipid(void)
{	
	uint32_t chipid = 0;
	// SDIO can't read into global variables
	// Use this variable as a temp, then copy to the global
	uint32_t tempchipid = 0;
	uint32_t rfrevid;
	
	if(chipid == 0){		
		custom_spi_read_reg(0x1000,&tempchipid);
		custom_spi_read_reg(0x13f4,&rfrevid);
		if(!_ISNMC1000(tempchipid)) {
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

extern unsigned int g_mac_open;
extern CHIP_PS_STATE_T genuChipPSstate;
void custom_chip_wakeup(void)
{
	uint32_t reg, trials=0, trials2 = 0; 
	do
	{
		custom_spi_read_reg(1, &reg) ;
		/* Make sure bit 1 is 0 before we start. */
		custom_spi_write_reg(1, reg & ~(1 << 1));
		/* Set bit 1 */
		 custom_spi_write_reg(1, reg | (1 << 1)) ;
		/* Clear bit 1*/
		 custom_spi_write_reg(1, reg  & ~(1 << 1));
				
		do
		{
			/* Wait for the chip to stabilize*/
			mdelay(25);
			//usleep_range(20000, 20000);
			
			
			// Make sure chip is awake. This is an extra step that can be removed
			// later to avoid the bus access overhead
			if((custom_get_chipid() == 0))
			{
				custom_spi_read_reg(1, &reg) ;
				/* Make sure bit 1 is 0 before we start. */
				custom_spi_write_reg(1, reg & ~(1 << 1));
				/* Set bit 1 */
				 custom_spi_write_reg(1, reg | (1 << 1)) ;
				/* Clear bit 1*/
				 custom_spi_write_reg(1, reg  & ~(1 << 1));
				printk("Couldn't read chip id. Wake up failed\n");
			}
		}while((custom_get_chipid() == 0) && ((++trials %3) == 0));
		
	}while(custom_get_chipid() == 0 && (++trials > 3) );

	#if 1
	if(genuChipPSstate == CHIP_SLEEPING_MANUAL)
	{

		custom_spi_read_reg(0x1C0C, &reg);
		reg &= ~(1<<0);
		custom_spi_write_reg(0x1C0C, reg);	

		if(custom_get_chipid() >= 0x1002a0)
		{
			// Switch PMU clock back to externall.
			custom_spi_read_reg(0x1e48, &reg);
			reg &= ~(1<<31);
			custom_spi_write_reg(0x1e48, reg);
		}
	}
	#endif
	genuChipPSstate =CHIP_WAKEDUP;
}

/* The following function should be called from bus protected context */
void custom_chip_sleep_manually(void)
{
	uint32_t val32;
	NMI_Uint32 u32SleepTime = INFINITE_SLEEP_TIME;

	printk(">> Custom Chip Sleep manually\n");
	if(genuChipPSstate != CHIP_WAKEDUP)
	{
		/* chip is already sleeping. Do nothing */
		printk("\n>>Already sleeping  ...\n");
		return;
	}
#ifdef NMI_OPTIMIZE_SLEEP_INT
	chip_allow_sleep();
#endif


	if(custom_get_chipid() >= 0x1002a0)
	{
		// Switch PMU clock source to internal.
		 custom_spi_read_reg(0x1e48, &val32);
		val32 |= (1<<31);
		custom_spi_write_reg(0x1e48, val32);
	}
	
	custom_spi_write_reg(0x1C08, u32SleepTime);

	custom_spi_read_reg(0x1C0C, &val32);
	val32 |= (1<<0);
	custom_spi_write_reg(0x1C0C, val32);

	genuChipPSstate = CHIP_SLEEPING_MANUAL;
}


