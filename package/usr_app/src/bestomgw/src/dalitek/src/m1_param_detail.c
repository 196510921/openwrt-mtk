#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "m1_param_detail.h"
#include "m1_common_log.h"
#include "sql_operate.h"

int app_set_param_descrip(payload_t data)
{
	M1_LOG_DEBUG("app_set_param_descrip\n");
	int ret = M1_PROTOCOL_OK;
	cJSON* devIdJson = NULL;
	cJSON* typeJson = NULL;
	cJSON* valueJson = NULL;
	cJSON* descripJson = NULL;
	sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

	if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    } 

    /*获取json内容*/
    devIdJson = cJSON_GetObjectItem(data.pdu, "devId");
    if(devIdJson == NULL)
    {
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    M1_LOG_DEBUG("devId:%s\n", devIdJson->valuestring);

    typeJson = cJSON_GetObjectItem(data.pdu, "type");
    if(typeJson == NULL)
    {
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    M1_LOG_DEBUG("type:%d\n", typeJson->valueint);

    valueJson = cJSON_GetObjectItem(data.pdu, "value");
    if(typeJson == NULL)
    {
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    M1_LOG_DEBUG("value:%d\n", typeJson->valueint);

	descripJson = cJSON_GetObjectItem(data.pdu, "descrip");
    if(descripJson == NULL)
    {
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    M1_LOG_DEBUG("descrip:%s\n", descripJson->valuestring);    

    /*写入数据库*/
    db = data.db;
    if(sql_begin_transaction(db)){
    	
    }

    Finish:

    return ret;
}










