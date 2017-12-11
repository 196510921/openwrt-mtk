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
	int pduType = TYPE_M1_REPORT_DEV_INFO;
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
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
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
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
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
	int pduType = TYPE_M1_REPORT_DEV_INFO;
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
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
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








