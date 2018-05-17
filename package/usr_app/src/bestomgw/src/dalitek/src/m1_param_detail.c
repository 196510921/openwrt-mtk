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
	int ret            = M1_PROTOCOL_OK;
    char* errorMsg     = NULL;
    char* sql          = NULL;
	cJSON* devIdJson   = NULL;
	cJSON* typeJson    = NULL;
	cJSON* valueJson   = NULL;
	cJSON* descripJson = NULL;
	sqlite3* db        = NULL;
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

    sql = "insert or replace into param_detail_table(DEV_ID, TYPE, VALUE, DESCRIP)values(?,?,?,?);";
    M1_LOG_DEBUG("sql:%s\n",sql);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    // if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    // {
        sqlite3_bind_text(stmt, 1, devIdJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 2, typeJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 3, valueJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 4, descripJson->valuestring, -1, NULL);
        rc = sqlite3_step(stmt);
        M1_LOG_DEBUG("step() return %s, number:%03d\n",\
            rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
                    
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        }

        // rc = sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg);
        // if(rc == SQLITE_OK)
        // {
        //     M1_LOG_DEBUG("COMMIT OK\n");
        // }
        // else if(rc == SQLITE_BUSY)
        // {
        //     M1_LOG_WARN("等待再次提交\n");
        // }
        // else
        // {
        //     M1_LOG_WARN("COMMIT errorMsg:%s\n",errorMsg);
        //     sqlite3_free(errorMsg);
        // }

    // }
    // else
    // {
    //     M1_LOG_WARN("BEGIN IMMEDIATE errorMsg:%s",errorMsg);
    //     sqlite3_free(errorMsg);
    // }   

    Finish:
    if(stmt)
        sqlite3_finalize(stmt);


    return ret;
}










