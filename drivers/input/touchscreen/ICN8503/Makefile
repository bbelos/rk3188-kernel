KO_NAME = ctp_icn85xx

PWD := $(shell pwd)

ifeq ($(KERNELRELEASE),)
# current directory is driver
CFGDIR = $(PWD)/../../../config

include $(CFGDIR)/build_config.cfg
include $(CFGDIR)/rules.make

ifeq ($(R_BOARD),ATM7029_DEMO)
EXTRA_CFLAGS += -DLOCAL_GL5202_DEMO
else
EXTRA_CFLAGS += -DLOCAL_GL5202_EVB
endif

endif


#obj-m += $(KO_NAME).o
#$(KO_NAME)-objs	    += $(MOD_DIR)/icn85xx_flash.o

$(KO_NAME)-objs := icn85xx.o icn85xx_flash.o
obj-y +=$(KO_NAME).o 

default:
	$(MAKE) -C $(KERNEL_BUILD_PATH) ARCH=$(R_ARCH) CROSS_COMPILE=$(R_CROSS_COMPILE) \
		EXTRA_CFLAGS="$(EXTRA_CFLAGS)" M=$(PWD) modules
	cp $(KO_NAME).ko $(DEFAULT_MODULES_DIR)/$(KO_NAME).ko
	cp ICN8505.bin $(DEFAULT_MODULES_DIR)/ICN8505.bin

clean:
	$(MAKE) -C $(KERNEL_BUILD_PATH) ARCH=$(R_ARCH) CROSS_COMPILE=$(R_CROSS_COMPILE) \
		M=$(PWD) clean
