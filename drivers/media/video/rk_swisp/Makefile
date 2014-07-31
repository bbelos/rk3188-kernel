#
# Makefile for rockchip isp driver
#
NEON_FLAGS := -mfloat-abi=softfp -mfpu=neon
AFLAGS_ccm_neon_pix.o += $(NEON_FLAGS)

rk_swisp_drv-objs += rk_swisp.o rk_swisp_dev.o denoise.o demosaic.o ae.o af.o awb.o\
                     defect_correction.o lens_correction.o scale.o color_correction.o ccm_neon_pix.o
obj-$(CONFIG_VIDEO_RKCIF_SWISP)	+= rk_swisp_drv.o

