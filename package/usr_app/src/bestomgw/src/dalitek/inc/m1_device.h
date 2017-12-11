#ifndef _M1_DEVICE_H_
#define _M1_DEVICE_H_

#include "cJSON.h"
#include "sqlite3.h"

int m1_del_dev_from_ap(sqlite3* db, char* devId);
int m1_del_ap(sqlite3* db, char* apId);


#endif //_M1_DEVICE_H_

