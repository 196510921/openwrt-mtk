#!/usr/bin/python
# -*-coding:UTF-8 -*-
import os
cmd = './dalitek/src'
print cmd
os.chdir(cmd)
cmd1 = 'rm *.o'
print cmd1
os.system(cmd1)

cmd2 = '../../bestomgw/src'
print cmd2
os.chdir(cmd2)
cmd3 = 'rm *.o'
print cmd3
os.system(cmd3)

cmd1_1 = '../../uart/src'
print cmd1_1
os.chdir(cmd1_1)
cmd1_2 = 'rm *.o'
print cmd1_2
os.system(cmd1_2)

cmd1_3 = '../../protocol_dev/src'
print cmd1_3
os.chdir(cmd1_3)
cmd1_4 = 'rm *.o'
print cmd1_4
os.system(cmd1_4)

cmd2_3 = '../../tcp_client/src'
print cmd2_3
os.chdir(cmd2_3)
cmd3_3 = 'rm *.o'
print cmd3_3
os.system(cmd3_3)

cmd2_4 = '../../http_client/src'
print cmd2_4
os.chdir(cmd2_4)
cmd3_4 = 'rm *.o'
print cmd3_4
os.system(cmd3_4)

cmd4 = '../../'
print cmd4
os.chdir(cmd4)

cmd5 = 'cp -r dalitek/ ~/share/openwrt_m1/package/ralink/applications/bestomgw/src/'
print cmd5
os.system(cmd5)

cmd5_1 = 'cp -r bestomgw/ ~/share/openwrt_m1/package/ralink/applications/bestomgw/src/'
print cmd5_1
os.system(cmd5_1)

cmd5_2 = 'cp Makefile ~/share/openwrt_m1/package/ralink/applications/bestomgw/src/'
print cmd5_2
os.system(cmd5_2)


cmd5_5 = 'cp -r tcp_client/ ~/share/openwrt_m1/package/ralink/applications/bestomgw/src/'
print cmd5_5
os.system(cmd5_5)

cmd5_6 = 'cp -r uart/ ~/share/openwrt_m1/package/ralink/applications/bestomgw/src/'
print cmd5_6
os.system(cmd5_6)

cmd5_7 = 'cp -r protocol_dev/ ~/share/openwrt_m1/package/ralink/applications/bestomgw/src/'
print cmd5_7
os.system(cmd5_7)

cmd5_7 = 'cp -r http_client/ ~/share/openwrt_m1/package/ralink/applications/bestomgw/src/'
print cmd5_7
os.system(cmd5_7)

cmd6 = '/home/ubuntu/share/openwrt_m1'
print cmd6
os.chdir(cmd6)

cmd7 = 'make package/ralink/applications/bestomgw/compile V=s'
print cmd7
output = os.popen(cmd7)
print output.read()
