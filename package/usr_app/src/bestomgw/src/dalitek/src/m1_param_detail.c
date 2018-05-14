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
    char* time = (char*)malloc(30);
    char* sql = (char*)malloc(300);
    char* sql_1 = (char*)malloc(300);
	cJSON* devIdJson = NULL;
	cJSON* typeJson = NULL;
	cJSON* valueJson = NULL;
	cJSON* descripJson = NULL;
	sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* stmt_1 = NULL;

	if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    } 

    getNowTime(time);
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
    M1_LOG_DEBUG("value:%d\n", valueJson->valueint);

	descripJson = cJSON_GetObjectItem(data.pdu, "descrip");
    if(descripJson == NULL)
    {
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    M1_LOG_DEBUG("descrip:%s\n", descripJson->valuestring);    

    /*写入数据库*/
    db = data.db;
    /*删除表中的历史数据*/
    sprintf(sql,"delete from param_detail_table where dev_id = \"%s\" and type = %05d and value = %d;",devIdJson->valuestring,
        typeJson->valueint,valueJson->valueint);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    thread_sqlite3_step(&stmt, db);
    /*插入新数据*/
    sprintf(sql_1,"insert into param_detail_table(ID, DEV_ID, TYPE, VALUE, DESCRIP, TIME)values(?,?,?,?,?,?);");
    if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    thread_sqlite3_step(&stmt, db);
        

    Finish:

    return ret;
}










