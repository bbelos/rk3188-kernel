MAKE	= make
RM		= rm
MV  	= mv
ECHO	= echo
CP		= cp
PWD 	= $(shell pwd)


obj-m :=  lib.a
lib-y := source/NMI_MsgQueue.o source/NMI_StrUtils.o source/NMI_Memory.o\
 source/NMI_Sleep.o source/NMI_Thread.o  source/NMI_Time.o\
  source/NMI_Semaphore.o source/NMI_Timer.o\

ccflags-y += -I$(src)/../../../
ccflags-y += -DNMI_PLATFORM=NMI_LINUXKERNEL
ccflags-y += -Wno-unused-function





ifeq ($(TARGET),SIMULATION)
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
OUT_ARCH = x86
else ifeq ($(TARGET),PANDA)
KERNELDIR ?= $(DEV_TREE)/kernel/omap
MAKE_FLAGS := CROSS_COMPILE=$(DEV_TREE)/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- ARCH=arm
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
#EXTRA_CFLAGS += -DNM73131_0_BOARD -DUSE_WIRELESS
EXTRA_CFLAGS += -DNM73131_0_BOARD
else ifeq ($(TARGET),ALLWINNER)
KERNELDIR ?= $(DEV_TREE)/linux-3.0
MAKE_FLAGS := CROSS_COMPILE=$(DEV_TREE)/buildroot/output/external-toolchain/bin/arm-none-linux-gnueabi- ARCH=arm
OUT_ARCH = ARM-3.0
else ifeq ($(TARGET),ALLWINNER_A20)
KERNELDIR ?= $(DEV_TREE)/linux-3.3
MAKE_FLAGS := CROSS_COMPILE=$(DEV_TREE)/out/android/common/buildroot/external-toolchain/bin/arm-linux-gnueabi- ARCH=arm
EXTRA_CFLAGS += -DPLAT_ALLWINNER_A20 -DUSE_WIRELESS
OUT_ARCH = ARM-3.3
else
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
OUT_ARCH = x86
TARGET=SIMULATION
endif



all:
	@$(ECHO) "###########################################"
	@$(ECHO) "###### Building NMI OS Wrapper lib #######"
	@$(ECHO) "##### TARGET =" $(TARGET) "######"	
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules $(MAKE_FLAGS)
	mkdir -p ../../binary/linux/$(OUT_ARCH)
	$(MV) $(PWD)/lib.a ../../binary/linux/$(OUT_ARCH)/NMI_OsWrapper.a



clean:
	@$(ECHO) "###########################################"
	@$(ECHO) "###### Cleaning NMI OS Wrapper lib #######"
	@$(ECHO) "##### TARGET =" $(TARGET) "######"
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) clean
	$(RM) -f $(OBJPROG) 
	$(RM) -f ../../binary/linux/$(OUT_ARCH)/NMI_OsWrapper.a



