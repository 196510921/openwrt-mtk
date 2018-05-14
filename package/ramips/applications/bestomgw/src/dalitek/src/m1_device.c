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
	int rc = 0,ret = M1_PROTOCOL_OK;
	int pduType = TYPE_COMMON_OPERATE;
	char* apId = NULL;
	char* sql = (char*)malloc(300);
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

	/*获取AP ID*/
	sprintf(sql, "select AP_ID from all_dev where DEV_ID = \"%s\" order by ID desc limit 1;",devId);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    rc = thread_sqlite3_step(&stmt, db);
    if(rc == SQLITE_ROW){
        apId = sqlite3_column_text(stmt, 0);
        if(apId == NULL){
        	M1_LOG_ERROR("m1_del_dev_from_ap failed\n");
        	ret = M1_PROTOCOL_FAILED;
        	goto Finish;
        }
    }else{
    	M1_LOG_ERROR("m1_del_dev_from_ap failed\n");
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }
    /*获取clientFd*/
    sprintf(sql, "select CLIENT_FD from conn_info where AP_ID = \"%s\" order by ID desc limit 1;",apId);
    sqlite3_finalize(stmt);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    rc = thread_sqlite3_step(&stmt, db);
    if(rc == SQLITE_ROW){
        clientFd = sqlite3_column_int(stmt, 0);
    }else{
    	M1_LOG_ERROR("m1_del_dev_from_ap failed\n");
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
    sqlite3_finalize(stmt);
	free(sql);

	return ret;
}

/*M1删除ap命令到AP*/
int m1_del_ap(sqlite3* db, char* apId)
{
	M1_LOG_DEBUG("m1_del_ap\n");

	int clientFd = 0;
	int rc = 0, ret = M1_PROTOCOL_OK;
	int pduType = TYPE_COMMON_OPERATE;
	char* sql = (char*)malloc(300);
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
    cJSON_AddStringToObject(devDataObject, "type", "ap");
    cJSON_AddStringToObject(devDataObject, "id", apId);
    cJSON_AddStringToObject(devDataObject, "operate", "delete");

    /*获取clientFd*/
    sprintf(sql, "select CLIENT_FD from conn_info where AP_ID = \"%s\" order by ID desc limit 1;",apId);
    sqlite3_finalize(stmt);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    rc = thread_sqlite3_step(&stmt, db);
    if(rc == SQLITE_ROW){
        clientFd = sqlite3_column_int(stmt, 0);
    }else{
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
    sqlite3_finalize(stmt);
	free(sql);

	return ret;
}

/*APP下发测试命令到AP*/
int app_download_testing_to_ap(cJSON* devData, sqlite3* db)
{
    M1_LOG_DEBUG("app_download_testing_to_ap\n");
    int clientFd = 0;
    int rc,ret = M1_PROTOCOL_OK;
    char* ap_id = NULL;
    char* sql = (char*)malloc(300);
    char* sql_1 = (char*)malloc(300);
    cJSON* pduJson = NULL;
    cJSON* pduDataJson = NULL;
    cJSON* devIdJson = NULL;
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* stmt_1 = NULL;

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

    /*数据库查询ap id*/
    sprintf(sql,"select AP_ID from all_dev where DEV_ID = \"%s\" order by ID desc limit 1;",devIdJson->valuestring);
    M1_LOG_DEBUG("sql:%s\n",sql);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    rc = thread_sqlite3_step(&stmt, db);
    if(rc == SQLITE_ROW){
        ap_id = sqlite3_column_text(stmt, 0);
        if(ap_id == NULL){
            M1_LOG_ERROR("ap_id null”\n");
            goto Finish;       
        }
    }else{
        M1_LOG_ERROR("select AP_ID failed\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }    
    /*数据库*/
    sprintf(sql_1,"select CLIENT_FD from conn_info where AP_ID = \"%s\" order by ID desc limit 1;", ap_id);
    M1_LOG_DEBUG("sql_1:%s\n",sql_1);
    if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    rc = thread_sqlite3_step(&stmt_1, db);
    if(rc == SQLITE_ROW){
        clientFd = sqlite3_column_int(stmt_1, 0);
    }else{
        M1_LOG_ERROR("select CLIENT_ID failed\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
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
    free(sql);
    free(sql_1);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
    
    return ret;

}

/*AP上传测试命令到APP*/
int ap_upload_testing_to_app(cJSON* devData, sqlite3* db)
{
    M1_LOG_DEBUG("ap_upload_testing_to_app\n");
    
    int clientFd = 0;
    int rc,ret = M1_PROTOCOL_OK;
    char* p = NULL;
    char* sql = (char*)malloc(300);
    cJSON* pduJson = NULL;
    cJSON* pduDataJson = NULL;
    cJSON* devIdJson = NULL;
    sqlite3_stmt* stmt = NULL;

    if(devData == NULL){
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

    sprintf(sql,"select CLIENT_FD from account_info");
    M1_LOG_DEBUG("sql:%s\n",sql);
    //sqlite3_finalize(stmt_1);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
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
    
    int rc = 0;
    int id = 0;
    char* sql = (char*)malloc(300);
    char* sql_1 = (char*)malloc(300);
    char* sql_2 = (char*)malloc(300);
    char* time = (char*)malloc(30);
    char* dev_name = NULL;
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* stmt_1 = NULL;
    sqlite3_stmt* stmt_2 = NULL;

    getNowTime(time);
    /*删除参数表中的参数*/
    sprintf(sql,"select ID from param_table where DEV_ID = \"%s\" and TYPE = %05d order by ID desc limit 1;",data.devId, data.type);
    M1_LOG_DEBUG("sql:%s\n",sql);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        goto Finish; 
    }
    rc = thread_sqlite3_step(&stmt, db);
    if(rc == SQLITE_ROW){
        sprintf(sql_1,"update param_table set VALUE = %05d where DEV_ID = \"%s\" and TYPE = %05d;",data.value, data.devId, data.type);
        M1_LOG_DEBUG("sql_1:%s\n",sql_1);
        if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK){
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            goto Finish; 
        }
        thread_sqlite3_step(&stmt_1, db);
    }else{
        /*获取param_table 中ID*/
        sprintf(sql, "select ID from param_table order by ID desc limit 1;");
        id = sql_id(db, sql);
        /*获取all_dev设备名称*/
        sprintf(sql_1,"select DEV_NAME from all_dev where DEV_ID = \"%s\" order by ID desc limit 1;",data.devId);
        M1_LOG_DEBUG("sql_1:%s\n",sql_1);
        sqlite3_finalize(stmt_1);
        if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK){
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            goto Finish; 
        }
        thread_sqlite3_step(&stmt_1, db);
        dev_name = sqlite3_column_text(stmt_1, 0);
        if(dev_name == NULL)
            goto Finish;
        M1_LOG_DEBUG("dev_name:%s\n",dev_name);
        /*插入参数*/
        sprintf(sql_2,"insert into param_table(ID, DEV_NAME,DEV_ID,TYPE,VALUE,TIME) values(?,?,?,?,?,?);");
        M1_LOG_DEBUG("sql_2:%s\n",sql_2);
        if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK){
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            goto Finish; 
        }

        sqlite3_bind_int(stmt_2, 1, id);
        sqlite3_bind_text(stmt_2, 2,  dev_name, -1, NULL);
        sqlite3_bind_text(stmt_2, 3, data.devId, -1, NULL);
        sqlite3_bind_int(stmt_2, 4, data.type);
        sqlite3_bind_int(stmt_2, 5, data.value);
        sqlite3_bind_text(stmt_2, 6,  time, -1, NULL);
        
        rc = thread_sqlite3_step(&stmt_2, db);
        if(rc != SQLITE_ROW){
            M1_LOG_ERROR("insert failed\n");
        }
    }

    Finish:
    free(sql);
    sqlite3_finalize(stmt);
    free(sql_1);
    sqlite3_finalize(stmt_1);
    free(sql_2);
    sqlite3_finalize(stmt_2);
    free(time);
}

/*删除AP下子设备联动业务*/
void clear_ap_related_linkage(char* ap_id, sqlite3* db)
{
    M1_LOG_DEBUG("clear_ap_related_linkage\n");
    char* dev_id = NULL;
    char* sql = (char*)malloc(300);
    char* sql_1 = (char*)malloc(300);
    sqlite3_stmt* stmt = NULL;

    sprintf(sql,"select DEV_ID from all_dev where AP_ID = \"%s\" and AP_ID != DEV_ID and ACCOUNT = \"Dalitek\";", ap_id);
    M1_LOG_DEBUG("sql:%s\n",sql);    
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        goto Finish; 
    }
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
        dev_id = sqlite3_column_text(stmt, 0);
        if(dev_id == NULL){
            M1_LOG_ERROR("dev_id NULL\n");
            continue;
        }
        M1_LOG_DEBUG("dev_id:%s\n",dev_id);
        sprintf(sql_1,"delete from linkage_table where EXEC_ID = \"%s\";",dev_id);
        M1_LOG_DEBUG("sql_1:%s\n",sql_1);
        sql_exec(db, sql_1);
    }

    Finish:
    free(sql);
    sqlite3_finalize(stmt);
    free(sql_1);
}


