#!/bin/sh
while true
do
    hh=$(date +%H)
    if [ $hh -eq 14 ]; then
        echo 'cp ./dev_info.db ./backup.db'
        cp ./dev_info.db ./backup.db
    else
	echo $hh
    fi
    sleep 3600
done

