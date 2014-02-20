#ARCH = arm
MAKE = make
#CC	=  arm-linux-gcc
#AR	=  arm-linux-ar
ifeq ($(TARGET),PANDA)
CC	=  arm-eabi-gcc
AR	=  arm-eabi-ar
else
CC	=  gcc
AR	=  ar
endif
RM	= rm
MV  = mv
ECHO	= echo
CFLAGS	= -g
ifeq ($(TARGET),PANDA)
CFLAGS	+= -mabi=aapcs-linux
endif
PROGS   = liboswrapper.a
OBJECTS = NMI_Event.o NMI_FileOps.o NMI_Math.o NMI_Memory.o NMI_MsgQueue.o  NMI_Semaphore.o  NMI_Sleep.o NMI_Socket.o  NMI_StrUtils.o  NMI_Thread.o  NMI_Time.o  NMI_Timer.o


PWD = $(shell pwd)
ifeq ($(TARGET),PANDA)
OBJDIR = $(PWD)/../../binary/linux/panda
else
OBJDIR = $(PWD)/../../binary/linux/x86
endif

INCHDR = -I$(PWD)/../
INCHDR += -I$(PWD).
INCHDR += -I$(PWD)/../../
INCHDR += -I$(PWD)/../../../
INCHDR += -I$(PWD)/include
INCHDR += -I$(PWD)/source
ifeq ($(TARGET),PANDA)
INCHDR += -I/home/npminc/pandaboard/android4.0/system/core/include/
INCHDR += -I/home/npminc/pandaboard/android4.0/prebuilt/ndk/android-ndk-r4/platforms/android-3/arch-arm/usr/include
endif
INCHDR += -DNMI_PLATFORM=NMI_LINUX

VPATH = $(OBJDIR)
VPATH  += source


$(OBJDIR)/%.o : %.c
	@$(ECHO) "###########################################"
	@$(ECHO) "#### Building" $<
	$(CC) $(INCHDR) $(CFLAGS) -c $< -o $@

OBJPROG = $(addprefix $(OBJDIR)/, $(PROGS))

$(OBJPROG): $(addprefix $(OBJDIR)/, $(OBJECTS))
	@$(ECHO) "###########################################"
	@$(ECHO) "#### Linking library" $@
	$(AR) rcs $@ $^

all:build
Debug:build
Release:build
cleanDebug:clean
cleanRelease:clean

clean:
	$(RM) -f $(OBJPROG) $(addprefix $(OBJDIR)/, $(OBJECTS))

build: $(OBJPROG)
