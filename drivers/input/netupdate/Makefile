#
# Makefile for the input core drivers.
#

# Each configuration option enables a list of files.
atx8-objs    += at18.o at18_encrypt.o

obj-$(CONFIG_ENCRYPTION_DEVICE)		+= atx8.o

#$(obj)/atx8_encrypt.o: $(obj)/atx8_encrypt.uu
#	@echo "UUDE atx8_encrypt.uu"
#	@uudecode $(obj)/atx8_encrypt.uu -o $(obj)/atx8_encrypt.o
	
#$(obj)/atx8.o: $(obj)/at18.uu
#	@echo "UUDE at18.uu"
#	@uudecode $(obj)/at18.uu -o $(obj)/atx8.o

