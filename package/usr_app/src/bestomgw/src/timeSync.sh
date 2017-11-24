#!/bin/sh

timesync()
{
	ntpdate cn.pool.ntp.org
	hwclock -w
}

while true
do

	timesync
	 sleep 1  
done


