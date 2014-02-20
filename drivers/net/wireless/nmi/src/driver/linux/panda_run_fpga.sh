adb  push ../../core/wifi_v111/FPGA_485t/wifi_firmware.bin /system/etc/firmware/wifi_firmware.bin
adb  push binary/linux/ARM-3.0/nmi_wifi.ko  /sdcard/nmi_wifi.ko
#adb  push ../../../../../AdelShare/android4.0/softap.conf /data/misc/wifi/softap.conf
adb  shell insmod /sdcard/nmi_wifi.ko 
