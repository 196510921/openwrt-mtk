#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

#include "m1_device.h"
#include "m1_protocol.h"
#include "m1_common_log.h"
/*M1删除设备命令到AP*/
int m1_del_dev_from_ap(sqlite3* db, char* devId)
{
	M1_LOG_DEBUG("m1_del_dev_from_ap\n");
	int clientFd = 0;
	int rc = 0;
    int ret = M1_PROTOCOL_OK;
	int pduType = TYPE_COMMON_OPERATE;
	char* sql = NULL;
	sqlite3_stmt* stmt = NULL;
	cJSON* pJsonRoot = NULL;
	cJSON* pduJsonObject = NULL;
	cJSON* devDataObject = NULL;

	pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_ERROR("pJsonRoot NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    cJSON_AddNumberToObject(pJsonRoot, "sn", 1);
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
    /*创建devData*/
    devDataObject = cJSON_CreateObject();
    if(NULL == devDataObject)
    {
        // create object faild, exit
        cJSON_Delete(devDataObject);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataObject);
    cJSON_AddStringToObject(devDataObject, "type", "device");
    cJSON_AddStringToObject(devDataObject, "id", devId);
    cJSON_AddStringToObject(devDataObject, "operate", "delete");

    /*获取clientFd*/
    {
        sql = "select CLIENT_FD from conn_info where AP_ID \
        in(select AP_ID from all_dev where DEV_ID = ? order by ID desc limit 1) order by ID desc limit 1;";
        M1_LOG_DEBUG("sql:%s\n",sql);

        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }

        sqlite3_bind_text(stmt, 1, devId, -1, NULL);
        rc = sqlite3_step(stmt);
        if(rc == SQLITE_ROW)
        {
            clientFd = sqlite3_column_int(stmt, 0);
        }
        else
        {
            M1_LOG_ERROR("m1_del_dev_from_ap failed\n");
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }
    }

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8_t*)p, strlen(p), clientFd);

    Finish:
    if(stmt)
        sqlite3_finalize(stmt);

	return ret;
}

/*M1删除ap命令到AP*/
int m1_del_ap(sqlite3* db, char* apId)
{
	M1_LOG_DEBUG("m1_del_ap\n");
	int clientFd         = 0;
	int rc               = 0;
    int ret              = M1_PROTOCOL_OK;
	int pduType          = TYPE_COMMON_OPERATE;
	char* sql            = NULL;
	sqlite3_stmt* stmt   = NULL;
	cJSON* pJsonRoot     = NULL;
	cJSON* pduJsonObject = NULL;
	cJSON* devDataObject = NULL;

	pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_ERROR("pJsonRoot NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    cJSON_AddNumberToObject(pJsonRoot, "sn", 1);
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
    /*创建devData*/
    devDataObject = cJSON_CreateObject();
    if(NULL == devDataObject)
    {
        // create object faild, exit
        cJSON_Delete(devDataObject);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataObject);
    cJSON_AddStringToObject(devDataObject, "type", "ap");
    cJSON_AddStringToObject(devDataObject, "id", apId);
    cJSON_AddStringToObject(devDataObject, "operate", "delete");

    /*获取clientFd*/
    //sprintf(sql, "select CLIENT_FD from conn_info where AP_ID = \"%s\" order by ID desc limit 1;",apId);
    sql = "select CLIENT_FD from conn_info where AP_ID = ? order by ID desc limit 1;";
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    sqlite3_bind_text(stmt, 1, apId, -1, NULL);
    rc = sqlite3_step(stmt);
    if(rc == SQLITE_ROW)
    {
        clientFd = sqlite3_column_int(stmt, 0);
    }else
    {
    	M1_LOG_ERROR("m1_del_ap failed\n");
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8_t*)p, strlen(p), clientFd);

    Finish:
    if(stmt)
        sqlite3_finalize(stmt);

	return ret;
}

/*APP下发测试命令到AP*/
int app_download_testing_to_ap(cJSON* devData, sqlite3* db)
{
    M1_LOG_DEBUG("app_download_testing_to_ap\n");
    int clientFd       = 0;
    int rc             = 0;
    int ret            = M1_PROTOCOL_OK;
    char* sql          = NULL;
    cJSON* pduJson     = NULL;
    cJSON* pduDataJson = NULL;
    cJSON* devIdJson   = NULL;
    sqlite3_stmt* stmt = NULL;

    if(devData == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*获取pdu*/
    pduJson = cJSON_GetObjectItem(devData, "pdu");
    if(NULL == pduJson){
        M1_LOG_ERROR("pdu null\n");
        goto Finish;
    }
    /*获取devData*/
    pduDataJson = cJSON_GetObjectItem(pduJson, "devData");
    if(NULL == pduDataJson){
        M1_LOG_ERROR("devData null”\n");
        goto Finish;
    }
    /*获取AP ID*/
    devIdJson = cJSON_GetObjectItem(pduDataJson,"devId");
    if(NULL == devIdJson){
        M1_LOG_ERROR("apIdJson null”\n");
        goto Finish;
    }
    M1_LOG_DEBUG("apId:%s”\n",devIdJson->valuestring);

    {
        sql = "select CLIENT_FD from conn_info where AP_ID \
        in(select AP_ID from all_dev where DEV_ID = ? order by ID desc limit 1) order by ID desc limit 1;";
        M1_LOG_DEBUG("sql:%s\n",sql);

        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }

        sqlite3_bind_text(stmt, 1, devIdJson->valuestring, -1, NULL);
        rc = sqlite3_step(stmt);
        if(rc == SQLITE_ROW)
        {
            clientFd = sqlite3_column_int(stmt, 0);
        }
        else
        {
            M1_LOG_ERROR("m1_del_dev_from_ap failed\n");
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }
    }

    /*发送到AP*/
    char * p = cJSON_PrintUnformatted(devData);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);
    socketSeverSend((uint8_t*)p, strlen(p), clientFd);

    Finish:
    if(stmt)
        sqlite3_finalize(stmt);
    
    return ret;
}

/*AP上传测试命令到APP*/
int ap_upload_testing_to_app(cJSON* devData, sqlite3* db)
{
    M1_LOG_DEBUG("ap_upload_testing_to_app\n");
    
    int clientFd       = 0;
    int rc             = 0;
    int ret            = M1_PROTOCOL_OK;
    char *p            = NULL;
    char *sql          = NULL;
    cJSON *pduJson     = NULL;
    cJSON *pduDataJson = NULL;
    cJSON *devIdJson   = NULL;
    sqlite3_stmt *stmt = NULL;

    if(devData == NULL)
    {
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*获取pdu*/
    pduJson = cJSON_GetObjectItem(devData, "pdu");
    if(NULL == pduJson){
        M1_LOG_ERROR("pdu null\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*获取devData*/
    pduDataJson = cJSON_GetObjectItem(pduJson, "devData");
    if(NULL == pduDataJson){
        M1_LOG_ERROR("devData null”\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*获取dev ID*/
    devIdJson = cJSON_GetObjectItem(pduDataJson,"devId");
    if(NULL == devIdJson){
        M1_LOG_ERROR("apIdJson null”\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    M1_LOG_DEBUG("apId:%s”\n",devIdJson->valuestring);

    p = cJSON_PrintUnformatted(devData);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    M1_LOG_DEBUG("string:%s\n",p);

    sql = "select CLIENT_FD from account_info";
    M1_LOG_DEBUG("sql:%s\n",sql);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        clientFd = sqlite3_column_int(stmt, 0);
        socketSeverSend((uint8_t*)p, strlen(p), clientFd);
    }

    Finish:
    free(sql);
    sqlite3_finalize(stmt);
    
    return ret;
}

/*更新参数表中设备启停信息*/
void app_update_param_table(update_param_tb_t data, sqlite3* db)
{
    M1_LOG_DEBUG("app_update_param_table\n");
    
    int rc             = 0;
    char* errorMsg     = NULL;
    char* sql          = NULL;
    sqlite3_stmt* stmt = NULL;
    
    /*更新设备气筒信息*/
    // if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    // {
        sql = "insert or replace into param_table(DEV_NAME,DEV_ID,TYPE,VALUE) values\
        ((select DEV_NAME from all_dev where DEV_ID = ? order by ID desc limit 1),?,?,?);";
        M1_LOG_DEBUG("sql:%s\n",sql);
        
        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            goto Finish; 
        }
        sqlite3_bind_text(stmt, 1, data.devId, -1, NULL);
        sqlite3_bind_text(stmt, 2, data.devId, -1, NULL);
        sqlite3_bind_int(stmt, 3, data.type);
        sqlite3_bind_int(stmt, 4, data.value);
        rc = sqlite3_step(stmt);   
        M1_LOG_DEBUG("step() return %s, number:%03d\n",\
            rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
            
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        }

    //     rc = sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg);
    //     if(rc == SQLITE_OK)
    //     {
    //         M1_LOG_DEBUG("COMMIT OK\n");
    //     }
    //     else if(rc == SQLITE_BUSY)
    //     {
    //         M1_LOG_WARN("等待再次提交\n");
    //     }
    //     else
    //     {
    //         M1_LOG_WARN("COMMIT errorMsg:%s\n",errorMsg);
    //         sqlite3_free(errorMsg);
    //     }

    // }
    // else
    // {
    //     M1_LOG_WARN("BEGIN IMMEDIATE errorMsg:%s",errorMsg);
    //     sqlite3_free(errorMsg);
    // } 
    

    Finish:
    if(stmt)
        sqlite3_finalize(stmt);
}

/*删除AP下子设备联动业务*/
void clear_ap_related_linkage(char* ap_id, sqlite3* db)
{
    M1_LOG_DEBUG("clear_ap_related_linkage\n");
    int  rc              = 0;
    char *errorMsg       = NULL;
    char *dev_id         = NULL;
    char *sql            = NULL;
    char *sql_1          = NULL;
    sqlite3_stmt *stmt   = NULL;
    sqlite3_stmt *stmt_1 = NULL;

    sql = "select DEV_ID from all_dev where AP_ID = ? and AP_ID != DEV_ID and ACCOUNT = ?;";
    M1_LOG_DEBUG("sql:%s\n",sql);    

    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        goto Finish; 
    }

    sql_1 = "delete from linkage_table where EXEC_ID = ?";
    M1_LOG_DEBUG("sql_1:%s\n",sql_1);

    if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        goto Finish; 
    }


    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, ap_id, -1, NULL);
        sqlite3_bind_text(stmt, 2, "Dalitek", -1, NULL);
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            dev_id = sqlite3_column_text(stmt, 0);
            if(dev_id == NULL)
            {
                M1_LOG_ERROR("dev_id NULL\n");
                continue;
            }
            M1_LOG_DEBUG("dev_id:%s\n",dev_id);
            
            sqlite3_bind_text(stmt_1, 1, dev_id, -1, NULL);
            
            rc = sqlite3_step(stmt_1);   
            M1_LOG_DEBUG("step() return %s, number:%03d\n",\
                rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
                
            if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            }

            sqlite3_reset(stmt_1);
            sqlite3_clear_bindings(stmt_1);
        }
    }
    else
    {
        M1_LOG_WARN("BEGIN IMMEDIATE errorMsg:%s",errorMsg);
        sqlite3_free(errorMsg);
    }

    Finish:
    rc = sql_commit(db);
    if(rc == SQLITE_OK)
    {
        M1_LOG_DEBUG("COMMIT OK\n");
    }
    if(stmt)
        sqlite3_finalize(stmt);
    if(stmt_1)
        sqlite3_finalize(stmt_1);
}


