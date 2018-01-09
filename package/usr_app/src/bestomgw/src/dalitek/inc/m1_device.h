#ifndef _M1_DEVICE_H_
#define _M1_DEVICE_H_

#include "cJSON.h"
#include "sqlite3.h"

/*更新param_able参数*/
typedef struct _update_param_table_t{
	char* devId;
	int type;
	int value;
}update_param_tb_t;

int m1_del_dev_from_ap(sqlite3* db, char* devId);
int m1_del_ap(sqlite3* db, char* apId);
void app_update_param_table(update_param_tb_t data, sqlite3* db);
void clear_ap_related_linkage(char* ap_id, sqlite3* db);

#endif //_M1_DEVICE_H_

