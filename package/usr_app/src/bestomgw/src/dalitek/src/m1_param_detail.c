#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "m1_param_detail.h"
#include "m1_common_log.h"

static int param_detail_tb_search(param_detail_tb_t param_detail, sqlite3* db);
static int param_detail_tb_insert(param_detail_tb_t param_detail, sqlite3* db);
static int param_detail_tb_update(param_detail_tb_t param_detail, sqlite3* db);
static int param_detail_tb_delete(param_detail_tb_t param_detail, sqlite3* db);
static int param_detail_tb_select(char* dev_id, int type, cJSON* param, sqlite3* db);

void app_create_param_detail_table(sqlite3* db)
{
    int rc       = 0;
    char *errmsg = NULL;
    char *sql    = NULL;

    sql = "CREATE TABLE param_detail_table (                   \
                ID        INTEGER PRIMARY KEY AUTOINCREMENT,   \
                DEV_ID    TEXT,                                \
                TYPE      INT,                                 \
                VALUE     INT,                                 \
                DESCRIP   TEXT,                                \
                TIME      TIME    NOT NULL                     \
                                  DEFAULT CURRENT_TIMESTAMP    \
            );";

    M1_LOG_DEBUG("%s:\n",sql);        
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if(rc != SQLITE_OK)
    {
        sqlite3_free(errmsg);
        M1_LOG_WARN("param_detail_table already exit: %s\n",errmsg);
    }
}

int app_write_param_detail(payload_t data)
{
	M1_LOG_DEBUG("app_set_param_descrip\n");
	int ret            = 0;
	cJSON* devIdJson   = NULL;
	cJSON* typeJson    = NULL;
	cJSON* valueJson   = NULL;
	cJSON* descripJson = NULL;
    param_detail_tb_t param_detail;

	if(data.pdu == NULL)
    {
       return -1;
    } 
    /*获取json内容*/
    devIdJson = cJSON_GetObjectItem(data.pdu, "devId");
    if(devIdJson == NULL)
    {
        return -1;
    }
    M1_LOG_DEBUG("devId:%s\n", devIdJson->valuestring);
    param_detail.devId = devIdJson->valuestring;

    typeJson = cJSON_GetObjectItem(data.pdu, "type");
    if(typeJson == NULL)
    {
        return -1;
    }
    M1_LOG_DEBUG("type:%d\n", typeJson->valueint);
    param_detail.type = typeJson->valueint;

    valueJson = cJSON_GetObjectItem(data.pdu, "value");
    if(typeJson == NULL)
    {
        return -1;
    }
    M1_LOG_DEBUG("value:%d\n", valueJson->valueint);
    param_detail.value = valueJson->valueint;

	descripJson = cJSON_GetObjectItem(data.pdu, "description");
    if(descripJson == NULL)
    {
        return -1;
    }
    M1_LOG_DEBUG("description:%s\n", descripJson->valuestring);    
    param_detail.descrip = descripJson->valuestring;

    /*search param_detail_table*/
    ret = param_detail_tb_search(param_detail, data.db);
    if(ret < 0)
    {
        /*insert into param_detail_table */
        ret = param_detail_tb_insert(param_detail, data.db);
        return ret;    
    }
    /*update param_detail_table*/
    ret = param_detail_tb_update(param_detail, data.db);
    
    return ret;
}

int app_read_param_detail(payload_t data)
{
    int ret               = 0;
    int i                 = 0;
    int num               = 0;
    int type[10]          = {0};
    char *devId           = NULL;
    int pduType           = TYPE_M1_RSP_DEV_DYNAMIC_PARAM;
    char *p               = NULL; 
    cJSON* pJsonRoot      = NULL;
    cJSON* pduJsonObject  = NULL;
    cJSON* devDataObject  = NULL;
    cJSON* devIdJson      = NULL;
    cJSON* typeJson       = NULL; 
    cJSON* paramTypeJson  = NULL;
    cJSON* paramArrayJson = NULL;

    if(data.pdu == NULL)
    {
       return -1;
    } 
    /*解包*/
    /*获取json内容*/
    devIdJson = cJSON_GetObjectItem(data.pdu, "devId");
    if(devIdJson == NULL)
    {
        return -1;
    }
    M1_LOG_DEBUG("devId:%s\n", devIdJson->valuestring);
    devId = devIdJson->valuestring;


    paramTypeJson = cJSON_GetObjectItem(data.pdu, "paramType");
    if(paramTypeJson == NULL)
    {
        return -1;
    }

    num = cJSON_GetArraySize(paramTypeJson);
    for(i = 0; i < num; i++)
    {
        typeJson = cJSON_GetArrayItem(paramTypeJson, i);
        type[i] = typeJson->valueint; 
    }

    /*封包*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_DEBUG("pJsonRoot NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    cJSON_AddNumberToObject(pJsonRoot, "sn", data.sn);
    cJSON_AddStringToObject(pJsonRoot, "version", "1.0");
    cJSON_AddNumberToObject(pJsonRoot, "netFlag", 1);
    cJSON_AddNumberToObject(pJsonRoot, "cmdType", 1);
    /*create pdu object*/
    pduJsonObject = cJSON_CreateObject();
    if(NULL == pduJsonObject)
    {
        // create object faild, exit
        cJSON_Delete(pduJsonObject);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*add pdu to root*/
    cJSON_AddItemToObject(pJsonRoot, "pdu", pduJsonObject);
    /*add pdu type to pdu object*/
    cJSON_AddNumberToObject(pduJsonObject, "pduType", pduType);

    devDataObject = cJSON_CreateObject();
    if(NULL == devDataObject)
    {
        // create object faild, exit
        cJSON_Delete(devDataObject);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataObject);

    paramArrayJson = cJSON_CreateArray();
    if(NULL == paramArrayJson)
    {
        M1_LOG_ERROR("devArry NULL\n");
        cJSON_Delete(paramArrayJson);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(devDataObject, "param", paramArrayJson);
    
    for(i = 0; i < num; i++)
    {
        ret = param_detail_tb_select(devId, type[i], paramArrayJson, data.db);
    }

    p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);

    Finish:

    sql_close();

    if(p)
        socketSeverSend((uint8*)p, strlen(p), data.clientFd);

    cJSON_Delete(pJsonRoot);
    return ret;
}

int app_delete_param_detail(payload_t data)
{
    int ret          = 0;
    cJSON* devIdJson = NULL;
    cJSON* typeJson  = NULL;
    cJSON* valueJson = NULL;
    param_detail_tb_t param_detail;

    if(data.pdu == NULL)
    {
       return -1;
    } 
    /*解包*/
    devIdJson = cJSON_GetObjectItem(data.pdu, "devId");
    if(devIdJson == NULL)
    {
        return -1;
    }
    M1_LOG_DEBUG("devId:%s\n", devIdJson->valuestring);
    param_detail.devId = devIdJson->valuestring;

    typeJson = cJSON_GetObjectItem(data.pdu, "type");
    if(typeJson == NULL)
    {
        return -1;
    }
    param_detail.type = typeJson->valueint;

    valueJson = cJSON_GetObjectItem(data.pdu, "value");
    if(valueJson == NULL)
    {
        return -1;
    }
    param_detail.value = valueJson->valueint;

    ret = param_detail_tb_delete(param_detail, data.db);

    return ret;
}

/*insert into param_detail_table*/
static int param_detail_tb_insert(param_detail_tb_t param_detail, sqlite3* db)
{
    int ret            = 0;
    int rc             = 0;
    char* errorMsg     = NULL;
    char* sql          = NULL;
    sqlite3_stmt* stmt = NULL;

    sql = "insert into param_detail_table(DEV_ID, TYPE, VALUE, DESCRIP)values(?,?,?,?);";
    M1_LOG_DEBUG("sql:%s\n",sql);
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();  
        ret = -1;
        goto Finish; 
    }

    sqlite3_bind_text(stmt, 1, param_detail.devId, -1, NULL);
    sqlite3_bind_int(stmt, 2, param_detail.type);
    sqlite3_bind_int(stmt, 3, param_detail.value);
    sqlite3_bind_text(stmt, 4, param_detail.descrip, -1, NULL);

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        M1_LOG_DEBUG("BEGIN IMMEDIATE\n");

        rc = sqlite3_step(stmt);
        M1_LOG_DEBUG("step() return %s, number:%03d\n",\
            rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
                    
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        }

        rc = sql_commit(db);
        if(rc == SQLITE_OK)
        {
            M1_LOG_DEBUG("COMMIT OK\n");
        }
    }
    else
    {
        M1_LOG_WARN("BEGIN IMMEDIATE errorMsg:%s",errorMsg);
        sqlite3_free(errorMsg);
    }

    Finish:
    if(stmt)
        sqlite3_finalize(stmt);

    return ret;
}

/*update param_detail_table*/
static int param_detail_tb_update(param_detail_tb_t param_detail, sqlite3* db)
{
    int ret            = 0;
    int rc             = 0;
    char* errorMsg     = NULL;
    char* sql          = NULL;
    sqlite3_stmt* stmt = NULL;

    sql = "update param_detail_table set DESCRIP = ? where DEV_ID = ? and TYPE = ? and VALUE = ?;";
    M1_LOG_DEBUG("sql:%s\n",sql);
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();  
        ret = -1;
        goto Finish; 
    }

    sqlite3_bind_text(stmt, 1, param_detail.descrip, -1, NULL);
    sqlite3_bind_text(stmt, 2, param_detail.devId, -1, NULL);
    sqlite3_bind_text(stmt, 3, param_detail.type, -1, NULL);
    sqlite3_bind_text(stmt, 4, param_detail.value, -1, NULL);

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        M1_LOG_DEBUG("BEGIN IMMEDIATE\n");

        rc = sqlite3_step(stmt);
        M1_LOG_DEBUG("step() return %s, number:%03d\n",\
            rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
                    
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        }

        rc = sql_commit(db);
        if(rc == SQLITE_OK)
        {
            M1_LOG_DEBUG("COMMIT OK\n");
        }
    }
    else
    {
        M1_LOG_WARN("BEGIN IMMEDIATE errorMsg:%s",errorMsg);
        sqlite3_free(errorMsg);
    }

    Finish:
    if(stmt)
        sqlite3_finalize(stmt);

    return ret;
}

/*select param_detail_table*/
static int param_detail_tb_select(char* dev_id, int type, cJSON* param, sqlite3* db)
{
    int ret              = 0;
    int rc               = 0;
    char* sql            = NULL;
    int value            = NULL;
    char* descrip        = NULL;
    cJSON* devDataObject = NULL;
    sqlite3_stmt* stmt   = NULL;

    sql = "select VALUE, DESCRIP from param_detail_table where DEV_ID = ? and TYPE = ?;";
    M1_LOG_DEBUG("sql:%s\n",sql);
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();  
        ret = -1;
        goto Finish; 
    }

    sqlite3_bind_text(stmt, 1, dev_id, -1, NULL);
    sqlite3_bind_int(stmt, 2, type);

    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        value = sqlite3_column_int(stmt, 0);
        descrip = sqlite3_column_text(stmt, 1);

        devDataObject = cJSON_CreateObject();
        if(NULL == devDataObject)
        {
            // create object faild, exit
            M1_LOG_ERROR("devDataObject NULL\n");
            cJSON_Delete(devDataObject);
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }
        cJSON_AddItemToArray(param, devDataObject);
        cJSON_AddNumberToObject(devDataObject, "type", type);
        cJSON_AddNumberToObject(devDataObject, "value", value);
        cJSON_AddStringToObject(devDataObject, "description", descrip);
    }

    Finish:
    if(stmt)
        sqlite3_finalize(stmt);

    return ret;
}

/*search param_detail_table*/
static int param_detail_tb_search(param_detail_tb_t param_detail, sqlite3* db)
{
    int ret              = 0;
    int rc               = 0;
    char* sql            = NULL;
    sqlite3_stmt* stmt   = NULL;

    sql = "select ID from param_detail_table where DEV_ID = ? and TYPE = ? and VALUE = ?;";
    M1_LOG_DEBUG("sql:%s\n",sql);
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();  
        ret = -1;
        goto Finish; 
    }

    sqlite3_bind_text(stmt, 1, param_detail.devId, -1, NULL);
    sqlite3_bind_int(stmt, 2, param_detail.type);
    sqlite3_bind_int(stmt, 3, param_detail.value);

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_ROW)
    {
        ret = -1;
    }

    Finish:
    if(stmt)
        sqlite3_finalize(stmt);

    return ret;
}

/*delete from param_detail_table*/
static int param_detail_tb_delete(param_detail_tb_t param_detail, sqlite3* db)
{
    int ret              = 0;
    int rc               = 0;
    char* sql            = NULL;
    sqlite3_stmt* stmt   = NULL;

    sql = "delete from param_detail_table where DEV_ID = ? and TYPE = ? and VALUE = ?;";
    M1_LOG_DEBUG("sql:%s\n",sql);
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();  
        ret = -1;
        goto Finish; 
    }

    sqlite3_bind_text(stmt, 1, param_detail.devId, -1, NULL);
    sqlite3_bind_int(stmt, 2, param_detail.type);
    sqlite3_bind_int(stmt, 3, param_detail.value);

    rc = sqlite3_step(stmt);
    if(rc != SQLITE_ROW)
    {
        ret = -1;
    }

    Finish:
    if(stmt)
        sqlite3_finalize(stmt);

    return ret;
}

