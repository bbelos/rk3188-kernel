#!/bin/sh
num_init_iterations=0
echo "* Starting Init/Deinit Test *"
while [ "$num_init_iterations" -lt "20" ]
do
echo "*****************************************"
echo "*       initialization trial no. $((num_init_iterations+1))      *"
echo "*****************************************"
echo "Bringing wlan0 down"
ret_cfg_iface=$(adb  shell ifconfig wlan0 down)
ret_cfg_iface_err=$(echo "$ret_cfg_iface" | grep 'error')
echo "$ret_cfg_iface_err"
if [ -n "${ret_cfg_iface_err}" ]; then
	echo "failed in ifconfig wlan1 down"
	echo "..Initialization failed.."
	exit
fi
sleep 1
echo "Bringing wlan0 up"
ret_cfg_iface=$(adb  shell ifconfig wlan0 up)
ret_cfg_iface_err=$(echo "$ret_cfg_iface" | grep 'error')
echo "$ret_cfg_iface_err"
if [ -n "${ret_cfg_iface_err}" ]; then
	echo "failed in ifconfig wlan1 up"
	echo "..Initialization failed.."
	exit
fi
sleep 1
echo "Disabling Wifi"
adb  shell svc wifi disable
sleep 5
echo "Enabling Wifi"
adb  shell svc wifi enable
sleep 10
cli_status=$(adb shell wpa_cli status)
echo cli_status
cli_status_check=$(echo "$cli_status" | grep ^wpa_state)

if [ -z "${cli_status_check}" ]; then
	echo "failed in wpa_cli status"
	echo "..Initialization failed.."
	exit
fi
echo "succeeded"
num_init_iterations=$((num_init_iterations+1))
adb shell ping -i.4 -c10  192.168.11.1
#adb  shell ping -i.4 -c10  192.168.1.1

done
echo "num of succeded init/deinit trials $num_init_iterations"
echo "* End of Init/Deinit Test *"
