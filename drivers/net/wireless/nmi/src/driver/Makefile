MAKE	= make
RM		= rm
MV		= mv
ECHO	= echo
CP		= cp
PWD 	= $(shell pwd)

#EXTRA_CFLAGS += -DNMI_HOST_HAS_RTC
#EXTRA_CFLAGS += -DREG_0XF6_NOT_CLEAR_BUG_FIX
EXTRA_CFLAGS += -DDISABLE_PWRSAVE_AND_SCAN_DURING_IP
EXTRA_CFLAGS += -DAGING_ALG

obj-m := lib.a
ifeq ($(FULLY_HOSTING_AP),Y)
lib-y := NMI_host_AP.o core_configurator/CoreConfigurator.o nmi_driver_interface/host_interface.o\
  core_simulator/FIFO_Buffer.o nmi_sdio.o\
  nmi_spi.o nmi_wlan_cfg.o nmi_wlan.o
  
else
lib-y := core_configurator/CoreConfigurator.o nmi_driver_interface/host_interface.o\
  core_simulator/FIFO_Buffer.o nmi_sdio.o\
  nmi_spi.o nmi_wlan_cfg.o nmi_wlan.o 
endif


ccflags-y  = -I$(src)/include
ccflags-y += -I$(src)/../NMI_OsWrapper/include
ccflags-y += -I$(src)/..
ccflags-y += -DNMI_PLATFORM=NMI_LINUXKERNEL
ccflags-y += -Wno-unused-function

#Use external 1.4V VCO
EXTRA_CFLAGS += -DVCO_14_SUPPLY
#Default to NMI=EVAL if NMI isn't defined
ifeq ($(strip $(NMI)),)
NMI=EVAL
endif

ifeq ($(NMI),EVAL)
EXTRA_CFLAGS += -DNMC_ASIC_A0 -DPLL_WORKAROUND
else ifeq ($(NMI),FPGA)
endif

EXTRA_CFLAGS += -DCONNECT_DIRECT -DNMI_PARSE_SCAN_IN_HOST


ifeq ($(BUS),SDIO)
EXTRA_CFLAGS += -DNMI_SDIO 
#EXTRA_CFLAGS += -DNMI_OPTIMIZE_SLEEP_INT
endif

ifeq ($(SDIO_IRQ),GPIO)
EXTRA_CFLAGS += -DNMI_SDIO_IRQ_GPIO
endif

ifeq ($(DMA_VER),VER_1)
EXTRA_CFLAGS += -DUSE_DMA_VER_1
endif

ifeq ($(TARGET),SIMULATION)
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
lib-y += core_simulator/CoreConfigSimulator.o transport/Packet_Tx_Rx.o
ccflags-y += -DSIMULATION
EXTRA_CFLAGS += -DCONNECT_DIRECT
OUT_ARCH = x86
else ifeq ($(TARGET),PANDA)
#lib-y += core_simulator/CoreConfigSimulator.o transport/Packet_Tx_Rx.o
#ccflags-y += -DSIMULATION
EXTRA_CFLAGS += -DPANDA_BOARD -DUSE_WIRELESS
KERNELDIR ?= $(DEV_TREE)/kernel/omap
ifeq ($(ANDR_VER),4.3)
MAKE_FLAGS := CROSS_COMPILE=$(DEV_TREE)/prebuilts/gcc/linux-x86/arm/arm-eabi-4.7/bin/arm-eabi- ARCH=arm
else
MAKE_FLAGS := CROSS_COMPILE=$(DEV_TREE)/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- ARCH=arm
endif
OUT_ARCH = ARM-3.0
else ifeq ($(TARGET),BEAGLE)
KERNELDIR ?= $(DEV_TREE)
OUT_ARCH = ARM-2.6.39
MAKE_FLAGS := CROSS_COMPILE=arm-arago-linux-gnueabi- ARCH=arm
EXTRA_CFLAGS += -DBEAGLE_BOARD
else ifeq ($(TARGET),NM73131)
KERNELDIR ?= $(DEV_TREE_LPC)
OUT_ARCH = ARM-2.6.28.2
MAKE_FLAGS := CROSS_COMPILE=arm-linux- ARCH=arm
EXTRA_CFLAGS += -DNM73131_0_BOARD
else ifeq ($(TARGET),ALLWINNER)
KERNELDIR ?= $(DEV_TREE)/linux-3.0
MAKE_FLAGS := CROSS_COMPILE=$(DEV_TREE)/buildroot/output/external-toolchain/bin/arm-none-linux-gnueabi- ARCH=arm
EXTRA_CFLAGS += -DALLWINNER_BOARD -DUSE_WIRELESS
OUT_ARCH = ARM-3.0
else ifeq ($(TARGET),ALLWINNER_A20)
KERNELDIR ?= $(DEV_TREE)/linux-3.3
MAKE_FLAGS := CROSS_COMPILE=$(DEV_TREE)/out/android/common/buildroot/external-toolchain/bin/arm-linux-gnueabi- ARCH=arm
EXTRA_CFLAGS += -DPLAT_ALLWINNER_A20 -DUSE_WIRELESS
OUT_ARCH = ARM-3.3
else ifeq ($(TARGET),ALLWINNER_A23)
KERNELDIR ?= $(DEV_TREE)/linux-3.4
MAKE_FLAGS := CROSS_COMPILE=$(DEV_TREE)/out/android/common/buildroot/external-toolchain/bin/arm-linux-gnueabi- ARCH=arm
EXTRA_CFLAGS += -DPLAT_ALLWINNER_A23 -DUSE_WIRELESS
OUT_ARCH = ARM-3.4
else
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
lib-y += core_simulator/CoreConfigSimulator.o transport/Packet_Tx_Rx.o
ccflags-y += -DSIMULATION
EXTRA_CFLAGS += -DCONNECT_DIRECT
OUT_ARCH = x86
endif

ifeq ($(FIRMLOG),HOST)
	EXTRA_CFLAGS += -DSWITCH_LOG_TERMINAL
endif

EXTRA_CFLAGS += -DTCP_ACK_FILTER
all:	

	$(MAKE) TARGET=$(TARGET) -C ../NMI_OsWrapper/source/linuxkernel
	@$(ECHO) "###########################################"
	@$(ECHO) "####### Building NMI WiFi Driver lib ######"
	@$(ECHO) "##### TARGET =" $(TARGET) "######"
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules $(MAKE_FLAGS)
	mkdir -p binary/linux/$(OUT_ARCH)
	$(MV) $(PWD)/lib.a binary/linux/$(OUT_ARCH)/NMI_WiFi_Driver.a




clean:
	$(MAKE) TARGET=$(TARGET) -C ../NMI_OsWrapper/source/linuxkernel clean
	@$(ECHO) "###########################################"
	@$(ECHO) "####### Cleaning NMI WiFi Driver lib ######"
	@$(ECHO) "##### TARGET =" $(TARGET) "######"
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) clean
	$(RM) -f $(OBJPROG)
	$(RM) -f binary/linux/$(OUT_ARCH)/NMI_OsWrapper.a 



