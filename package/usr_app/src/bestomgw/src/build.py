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

cmd2_1 = '../../sql_operate'
print cmd2_1
os.chdir(cmd2_1)
cmd3_1 = 'rm *.o'
print cmd3_1
os.system(cmd3_1)

cmd2_2 = '../sql_table'
print cmd2_2
os.chdir(cmd2_2)
cmd3_2 = 'rm *.o'
print cmd3_2
os.system(cmd3_2)


cmd4 = '../'
print cmd4
os.chdir(cmd4)

cmd5 = 'cp -r dalitek/ ~/share/openwrt-mtk/package/ramips/applications/bestomgw/src/'
print cmd5
os.system(cmd5)

cmd5_1 = 'cp -r bestomgw/ ~/share/openwrt-mtk/package/ramips/applications/bestomgw/src/'
print cmd5_1
os.system(cmd5_1)

cmd5_2 = 'cp Makefile ~/share/openwrt-mtk/package/ramips/applications/bestomgw/src/'
print cmd5_2
os.system(cmd5_2)

cmd5_3 = 'cp -r sql_operate/ ~/share/openwrt-mtk/package/ramips/applications/bestomgw/src/'
print cmd5_3
os.system(cmd5_3)

cmd5_4 = 'cp -r sql_table/ ~/share/openwrt-mtk/package/ramips/applications/bestomgw/src/'
print cmd5_4
os.system(cmd5_4)

cmd6 = '../../../../../'
print cmd6
os.chdir(cmd6)

cmd7 = 'make package/ramips/applications/bestomgw/compile V=s'
print cmd7
output = os.popen(cmd7)
print output.read()
