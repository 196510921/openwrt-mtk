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
cmd4 = '../../'
print cmd4
os.chdir(cmd4)
cmd5 = 'cp -r dalitek/ bestomgw ~/share/openwrt-mtk/package/ramips/applications/bestomgw/src/'
print cmd5
os.system(cmd5)
cmd5_1 = 'cp Makefile ~/share/openwrt-mtk/package/ramips/applications/bestomgw/src/'
print cmd5_1
os.system(cmd5_1)
cmd6 = '../../../../../'
print cmd6
os.chdir(cmd6)
cmd7 = 'make package/ramips/applications/bestomgw/compile V=s'
print cmd7
output = os.popen(cmd7)
print output.read()
