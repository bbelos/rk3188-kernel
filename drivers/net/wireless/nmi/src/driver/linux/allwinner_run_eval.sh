adb   push ../../core/wifi_v111/ASIC_A0/wifi_firmware.bin /system/vendor/firmware/wifi_firmware.bin
adb   push binary/linux/ARM-3.0/nmi_wifi.ko  /system/vendor/modules/nmi_wifi.ko
adb shell "echo 0 >  /proc/driver/sunxi-mmc.3/insert"
adb shell "echo 1 >  /proc/driver/sunxi-mmc.3/insert"