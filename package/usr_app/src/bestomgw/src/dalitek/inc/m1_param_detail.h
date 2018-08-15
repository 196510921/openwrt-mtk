#ifndef _M1_PARAM_DETAIL_H_
#define _M1_PARAM_DETAIL_H_

#include "m1_protocol.h"

/*app 查询设备动态参数*/
#define TYPE_APP_REQ_DEV_DYNAMIC_PARAM       0x0036
/*app 新增设备动态参数*/
#define TYPE_APP_ADD_DEV_DYNAMIC_PARAM       0x0037
/*app 删除设备动态参数*/
#define TYPE_APP_DEL_DEV_DYNAMIC_PARAM       0x0038
/*M1回复设备动态参数*/
#define TYPE_M1_RSP_DEV_DYNAMIC_PARAM        0x1018

typedef struct 
{
  char* devId;
  int type;
  int value;
  char* descrip;
} param_detail_tb_t;

void app_create_param_detail_table(sqlite3* db);
int app_write_param_detail(payload_t data);
int app_read_param_detail(payload_t data);
int app_delete_param_detail(payload_t data);

#endif //_M1_PARAM_DETAIL_H_







