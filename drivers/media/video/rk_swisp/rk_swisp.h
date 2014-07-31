#ifndef __RK_SWISP_H_
#define __RK_SWISP_H_

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/videodev2.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/v4l2-mediabus.h>
#include <media/v4l2-common.h>
#include "rk_swisp_dev.h"
#include <asm/smp.h>

/*
*			 Driver Version Note
*
*v0.0.1:  this is software isp driver, only support for rk3188;
*
*v0.1.0:  this support for rk3066 2 nuclues and rk3188 4 nucleus;
*
*v0.1.1:  awb run multiple internal;
*
*v0.1.3:  fill invalidate left colum data;
*
*/

#define RK_SWISP_VERSION_CODE KERNEL_VERSION(0, 0x01, 3)

#define CONFIG_RK_SWISP_IMG_W       2592
#define CONFIG_RK_SWISP_IMG_H       1944

#define CONFIG_SWISP_USER_FILLRED   0

#define FORC(cnt) for (c=0; c < cnt; c++)
#define FORC3 FORC(3)
#define FORC4 FORC(4)
#define FORCC FORC(colors)

#define SQR(x) ((x)*(x))
#define ABS(x) (((int)(x) ^ ((int)(x) >> 31)) - ((int)(x) >> 31))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define LIM(x,min,max) MAX(min,MIN(x,max))
#define ULIM(x,y,z) ((y) < (z) ? LIM(x,y,z) : LIM(x,z,y))
#define CLIP(x) LIM(x,0,65535)
#define SWAP(a,b) { a=a+b; b=a-b; a=a-b; }

#define FC_R        0
#define FC_G        1
#define FC_B        2
/*
   In order to inline this calculation, I make the risky
   assumption that all filter patterns can be described
   by a repeating pattern of eight rows and two columns

   Do not use the FC or BAYER macros with the Leaf CatchLight,
   because its pattern is 16x16, not 2x8.

   Return values are either 0/1/2/3 = G/M/C/Y or 0/1/2/3 = R/G1/B/G2

	PowerShot 600	PowerShot A50	PowerShot Pro70	Pro90 & G1
	0xe1e4e1e4:	0x1b4e4b1e:	0x1e4b4e1b:	0xb4b4b4b4:

	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5
	0 G M G M G M	0 C Y C Y C Y	0 Y C Y C Y C	0 G M G M G M
	1 C Y C Y C Y	1 M G M G M G	1 M G M G M G	1 Y C Y C Y C
	2 M G M G M G	2 Y C Y C Y C	2 C Y C Y C Y
	3 C Y C Y C Y	3 G M G M G M	3 G M G M G M
			4 C Y C Y C Y	4 Y C Y C Y C
	PowerShot A5	5 G M G M G M	5 G M G M G M
	0x1e4e1e4e:	6 Y C Y C Y C	6 C Y C Y C Y
			7 M G M G M G	7 M G M G M G
	  0 1 2 3 4 5
	0 C Y C Y C Y
	1 G M G M G M
	2 C Y C Y C Y
	3 M G M G M G

   All RGB cameras use one of these Bayer grids:

	0x16161616:	0x61616161:	0x49494949:	0x94949494:

	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5
	0 B G B G B G	0 G R G R G R	0 G B G B G B	0 R G R G R G
	1 G R G R G R	1 B G B G B G	1 R G R G R G	1 G B G B G B
	2 B G B G B G	2 G R G R G R	2 G B G B G B	2 R G R G R G
	3 G R G R G R	3 B G B G B G	3 R G R G R G	3 G B G B G B
 */
#define FC(row,col) \
	(filters >> ((((row) << 1 & 14) + ((col) & 1)) << 1) & 3)

#define BAYER(row,col) \
	image[(row)*width + (col)][FC(row,col)]

#define BAYER2(row,col) \
	image[(row)*width + (col)][fc(row,col)]



struct Raw_rectangle{
	int startx;
	int starty;
	int win_width;
	int win_height;
};
typedef struct rk_cb_para_s {
    unsigned short *raw_s;
    unsigned char *raw_d;
    unsigned char (*image)[4];
	unsigned char *yuv;
	unsigned char *gamma_table;
    
    unsigned short srcw;
    unsigned short srch;
    unsigned short dstw;
    unsigned short dsth;

	int odd_even;
	int nucleus;
	int scale_quality;

	int af_zone_centre_w;
	int af_zone_centre_h;

	
    struct rk_raw_ret *raw_ret;
    int internal_loop;
} rk_cb_para_t;

typedef struct rk_callback_s {
    int (*cb) (rk_cb_para_t *para);
    int (*cb_bh) (struct rk_raw_ret *ret0, struct rk_raw_ret *ret1, struct rk_raw_ret *ret2, struct rk_raw_ret *ret3);
    

    struct swisp_k2u_cmdinfo usrcmd;
    bool cb_in_usr;
} rk_callback_t;

typedef struct rk_camera_isp_mem_res_s {
	const char *name;
	unsigned int start;
	unsigned int size;
    void __iomem *vbase;
} rk_camera_isp_mem_t;

typedef struct rk_camera_isp_para_s {

    ushort srcw;
    ushort srch;
    ushort dstw;
    ushort dsth;
    unsigned int quality;
    unsigned int dstfmt;
    enum v4l2_mbus_pixelcode srcfmt;    
    char *black_image_file;
    char *white_image_file;
	rk_callback_t gamma;
	rk_callback_t color_correction;
	unsigned char *gamma_table;
	int *lens_para;
	rk_camera_isp_mem_t meminfo;
	rk_camera_isp_mem_t rawmem;
	int *ccm_matrix_para;
	rk_callback_t af_zone_update;

	unsigned int version;
} rk_camera_isp_para_t;

typedef struct rk_camera_isp_init_s {
    rk_camera_isp_mem_t meminfo;
    rk_camera_isp_mem_t rawmem;
} rk_camera_isp_init_t;

typedef struct rk_camera_ispctrl_info_s {
    char calibration_img[2][30];
    unsigned int awb_ts;
	unsigned int ae_ts;
	unsigned int af_ts;
	int lens_para[10];
	int ccm_matrix_para[20];
}rk_camera_ispctrl_info_t;

struct rk_raw_ret{
    
	long af_EV;
	int ae_lumi;
	int ae_compensate;
    int y_avg;
    int b_avg;
    int r_avg;
    int resolution;

    void *arg_cb0;
    
    bool af_en;
    bool ae_en;
    bool awb_en;
};

typedef struct swisp_mem_s {
    rk_camera_isp_mem_t mem;     
    int vma_use_cnt;
} swisp_mem_t;


typedef struct swisp_ops_s
{
    int (*swisp_process)(short int *raw, short int *rgb, struct rk_raw_ret* raw_ret, void *rev);
    int (*swisp_streamon)(rk_camera_isp_para_t *info, void *rev);
    int (*swisp_streamoff)(rk_camera_isp_para_t *info, void *rev);
} swisp_ops_t;

typedef struct rk_swisp_fusion_para{
    int row;
    int col;
    int width_dst;
    int height_dst;
    int lens_x;
    int lens_y;
    int x_ratio;
    int y_ratio;
    int awb_gain[3];
    int lens_b2_gain[3];
    int lens_b4_gain[3];
    int c;
    int temp;
    unsigned short* raw_s;
    unsigned char*  raw_d; 
    long r2;
    long r4;
}rk_swisp_fusion_para_t;


extern int gfilters;

static inline int fc (int row, int col)
{
	return (gfilters >> ((((row) << 1 & 14) + ((col) & 1)) << 1) & 3);
}

//extern inline int lens_channel_correct_noscale_fusion(rk_swisp_fusion_para_t* para);
#define NOSCALE_DEFECT_PIXEL_TH (120)
#define SCALE_DEFECT_PIXEL_TH  (40)

static __attribute__((always_inline)) void defect_correction_noscale(rk_swisp_fusion_para_t* para)
{
	int d[5];
	int sum[4];
	int max, min,temp;
	int coff = 1024/3;
	int imgwidth = para->width_dst;
    unsigned short* src = para->raw_s;
    int *awb_gain = para->awb_gain;
    int c = para->c;
    
	temp = *src;
	d[0] = *(src-4);
	d[1] = *(src-3);
	d[2] = *(src-2);
	d[3] = *(src-1);
	d[4] = *(src+1);
	sum[0] = (d[0] - (d[1]<<1) - d[1] + (d[2]<<1) + (d[3]<<1) + d[4]);

	d[0] = *(src+4);
	d[1] = *(src+3);
	d[2] = *(src+2);
	d[3] = *(src+1);
	d[4] = *(src-1);
	sum[1] = (d[0] - (d[1]<<1) - d[1] + (d[2]<<1) + (d[3]<<1) + d[4]);

    sum[2] = (src[2*imgwidth]+src[-2*imgwidth]) + (src[2*imgwidth]+src[-2*imgwidth])>>1;
            
	if(sum[0]>sum[1]){
		max = sum[0];
	    min = sum[1];
	}else{
		max = sum[1];
		min = sum[0];
	}

	if(max<sum[2])
		max = sum[2];
	if(min>sum[2])
	    min = sum[2];
			    
	if(temp*3 > (max+NOSCALE_DEFECT_PIXEL_TH))
		temp = (max*coff)>>10;
    else if(temp*3 < (min-NOSCALE_DEFECT_PIXEL_TH))
		temp = (min*coff)>>10;
	else{
        para->temp = temp<<2;
	}

    temp = (temp*awb_gain[c])>>12;
    if(unlikely(temp<0))
		temp = 0;
	else if(temp>0xff)
		temp = 0xff;
		
    para->temp = temp;
}

static __attribute__((always_inline)) void lens_noscale_fusion_bh(rk_swisp_fusion_para_t* para)
{
    int tmp_x,tmp_y;
	long r2,r4;
    int lens_x = para->lens_x;
    int lens_y = para->lens_y;
    int x_ratio = para->x_ratio;
    int y_ratio = para->y_ratio;
    int row = para->row;
    int col = para->col;
	            
	tmp_y = ((row-lens_y)*y_ratio)>>7;
	tmp_x = ((col-lens_x)*x_ratio)>>7;
	r2 = tmp_y*tmp_y + tmp_x*tmp_x;	
	r4 = (r2>>16)*(r2>>16);
	r2 = (r2>>8);	

	para->r2 = r2;
	para->r4 = r4;
}

static __attribute__((always_inline)) void lens_noscale_fusion_th(rk_swisp_fusion_para_t* para)
{
    int imgwidth = para->width_dst;
    int* b2 = para->lens_b2_gain;
    int* b4 = para->lens_b4_gain;
    long r2 = para->r2;
    long r4 = para->r4;
    long gain;
    int c = para->c;
    int temp = para->temp;

    c = fc(para->row,para->col);    
	gain = 65536 + (b2[c]*r2) + (b4[c]*r4);	
	temp = (temp*gain)>>16;

	para->temp = temp;

}


#endif
