#!/bin/sh
#while true
#do
	hh = $(date +%H)
	if[ $hh -eq 16]
	then
		echo 'cp /dev_info.db /backup.db'
		cp /share/openwrt-mtk/package/usr_app/src/bestomgw/src/dev_info.db /share/openwrt-mtk/package/usr_app/src/bestomgw/src/backup.db
	fi
	sleep 3600 
#done
