#!/usr/bin/python
# -*-coding:UTF-8 -*-
import os,sys,time
cmd = 'cp dev_info.db sql_bkp.db'
print cmd
while True:
    os.system(cmd)
    time.sleep(10)
    print cmd
