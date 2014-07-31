#ifndef __RK_SWISP_DEV_H_
#define __RK_SWISP_DEV_H_

#include <linux/ioctl.h>

enum swisp_buffer_type {
    swisp_buf_image,
    swisp_buf_raw,
    swisp_buf_rawdst,
    
    swisp_buf_inval
};

enum swisp_k2u_cmd {
    swisp_k2u_cmd_demosaic,
    swisp_k2u_cmd_denoise,

    swisp_k2u_cmd_exit,
    swisp_k2u_cmd_fillred,
    swisp_k2u_cmd_inval
};


struct swisp_buffer {
    enum swisp_buffer_type type;
    unsigned int phy_addr;
    unsigned int size;
};

struct swisp_k2u_cmdinfo {
    unsigned short img_w;
    unsigned short img_h;
    unsigned int rawbuf_offset;
    enum swisp_k2u_cmd cmd;
    unsigned int quality;  
    int result;
};


struct swisp_imageinfo {
    unsigned short srcw;
    unsigned short srch;
    unsigned short dstw;
    unsigned short dsth;
	unsigned short dstw_m;
	unsigned short dsth_m;
    unsigned short raw_bits;
    unsigned int filter;
    unsigned int color;
};

/*
 * Ioctl definitions
 */

/* Use 'k' as magic number */
#define SWISP_IOC_MAGIC  'k'
#define SWISP_IOC_MAXNR  5

#define SWISPIOC_QUERYBUF     _IOWR(SWISP_IOC_MAGIC,  0, struct swisp_buffer)
#define SWISPIOC_USRWAITCMD     _IOWR(SWISP_IOC_MAGIC,  1, struct swisp_k2u_cmdinfo)
#define SWISPIOC_USRDONECMD     _IOWR(SWISP_IOC_MAGIC,  2, struct swisp_k2u_cmdinfo)
#define SWISPIOC_G_IMGINFO     _IOWR(SWISP_IOC_MAGIC,  0, struct swisp_imageinfo)
#endif
