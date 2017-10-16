#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "account_config.h"


int app_req_account_info_handle(payload_t data, int sn)
{
	cJSON* pJsonRoot = NULL;
	cJSON* pduJsonObject = NULL;
	cJSON* accountJson = NULL;
	cJSON* devDataJsonArray = NULL;

	char* account = NULL;
	int pduType = TYPE_M1_REPORT_ACCOUNT_INFO;

	fprintf(stdout,"account_info_handle”\n");
	
	sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int rc;
    char* sql = "select ACCOUNT from account_table order by ID";

    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }


    /*get sql data json*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        fprintf(stdout,"pJsonRoot NULL\n");
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }
    cJSON_AddNumberToObject(pJsonRoot, "sn", sn);
    cJSON_AddStringToObject(pJsonRoot, "version", "1.0");
    cJSON_AddNumberToObject(pJsonRoot, "netFlag", 1);
    cJSON_AddNumberToObject(pJsonRoot, "cmdType", 1);
    /*create pdu object*/
    pduJsonObject = cJSON_CreateObject();
    if(NULL == pduJsonObject)
    {
        // create object faild, exit
        cJSON_Delete(pduJsonObject);
        return M1_PROTOCOL_FAILED;
    }
    /*add pdu to root*/
    cJSON_AddItemToObject(pJsonRoot, "pdu", pduJsonObject);
    /*add pdu type to pdu object*/
    cJSON_AddNumberToObject(pduJsonObject, "pduType", pduType);
    /*create devData array*/
    devDataJsonArray = cJSON_CreateArray();
    if(NULL == devDataJsonArray)
    {
        cJSON_Delete(devDataJsonArray);
        return M1_PROTOCOL_FAILED;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataJsonArray);
    fprintf(stdout,"%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
	    /*create pdu object*/	    
	    account = sqlite3_column_text(stmt,0);
	    fprintf(stdout,"account:%s\n",account);
	    accountJson = cJSON_CreateString(account);
	    if(NULL == pduJsonObject)
	    {
	        // create object faild, exit
	        cJSON_Delete(accountJson);
	        return M1_PROTOCOL_FAILED;
	    }
	    cJSON_AddItemToArray(devDataJsonArray, accountJson);
	}

	sqlite3_finalize(stmt);
    sqlite3_close(db);

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    fprintf(stdout,"string:%s\n",p);
    /*response to client*/
    socketSeverSend((unsigned char*)p, strlen(p), data.clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;

}

int app_req_account_config_handle(payload_t data, int sn)
{
	fprintf(stdout,"app_req_account_config_handle\n");
	cJSON* pJsonRoot = NULL;
	cJSON* pduJsonObject = NULL;
	cJSON* devDataObject = NULL;
	cJSON* accountJson = NULL;
	cJSON* districtArray = NULL;
	cJSON* districtJson = NULL;
	cJSON* scenArray = NULL;
	cJSON* scenJson = NULL;

	char* account = NULL;
	int pduType = TYPE_M1_REPORT_ACCOUNT_CONFIG_INFO;

	sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int rc;
    char sql[200];

    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    /*get sql data json*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        fprintf(stdout,"pJsonRoot NULL\n");
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }
    cJSON_AddNumberToObject(pJsonRoot, "sn", sn);
    cJSON_AddStringToObject(pJsonRoot, "version", "1.0");
    cJSON_AddNumberToObject(pJsonRoot, "netFlag", 1);
    cJSON_AddNumberToObject(pJsonRoot, "cmdType", 1);
    /*create pdu object*/
    pduJsonObject = cJSON_CreateObject();
    if(NULL == pduJsonObject)
    {
        // create object faild, exit
        cJSON_Delete(pduJsonObject);
        return M1_PROTOCOL_FAILED;
    }
    /*add pdu to root*/
    cJSON_AddItemToObject(pJsonRoot, "pdu", pduJsonObject);
    /*add pdu type to pdu object*/
    cJSON_AddNumberToObject(pduJsonObject, "pduType", pduType);
    /*create devData array*/
    devDataObject = cJSON_CreateObject();
    if(NULL == devDataObject)
    {
        cJSON_Delete(devDataObject);
        return M1_PROTOCOL_FAILED;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataObject);
    /*获取账户名*/
    char *key = NULL, *key_auth = NULL, *remote_auth = NULL, *district = NULL,*scenario = NULL;
    accountJson = cJSON_GetObjectItem(data.pdu, "devData");
    sprintf(sql,"select KEY,KEY_AUTH,REMOTE_AUTH from account_table where ACCOUNT = %s", accountJson->valuestring);
    fprintf(stdout,"%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
	    /*create pdu object*/	    
	    key = sqlite3_column_text(stmt,0);
	    key_auth = sqlite3_column_text(stmt,1);
	    remote_auth = sqlite3_column_text(stmt,2);
	    fprintf(stdout,"key:%s,key_auth:%s, remote_auth:%s\n",key, key_auth, remote_auth);
	    cJSON_AddStringToObject(devDataObject, "key", key);
	    cJSON_AddStringToObject(devDataObject, "key_auth", key_auth);
	    cJSON_AddStringToObject(devDataObject, "remote_auth", remote_auth);
	}
    /*获取区域信息*/
	districtArray = cJSON_CreateArray();
    if(NULL == districtArray)
    {
        cJSON_Delete(districtArray);
        return M1_PROTOCOL_FAILED;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(devDataObject, "district", districtArray);
    sprintf(sql,"select DIS_NAME from district_table where ACCOUNT = %s", accountJson->valuestring);
    fprintf(stdout,"%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
	    /*添加区域信息*/	    
	    district = sqlite3_column_text(stmt,0);
	   	fprintf(stdout,"district:%s\n", district);
	    districtJson = cJSON_CreateString(district);
	    if(NULL == districtJson)
	    {
	        // create object faild, exit
	        cJSON_Delete(districtJson);
	        return M1_PROTOCOL_FAILED;
	    }
	    cJSON_AddItemToArray(districtArray, districtJson);
	}
	/*获取场景信息*/
	scenArray = cJSON_CreateArray();
    if(NULL == scenArray)
    {
        cJSON_Delete(scenArray);
        return M1_PROTOCOL_FAILED;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(devDataObject, "district", scenArray);
    sprintf(sql,"select SCEN_NAME from scenario_table where ACCOUNT = %s", accountJson->valuestring);
    fprintf(stdout,"%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
	    /*添加区域信息*/	    
	    scenario = sqlite3_column_text(stmt,0);
	   	fprintf(stdout,"scenario:%s\n", scenario);
	    scenJson = cJSON_CreateString(scenario);
	    if(NULL == scenJson)
	    {
	        // create object faild, exit
	        cJSON_Delete(scenJson);
	        return M1_PROTOCOL_FAILED;
	    }
	    cJSON_AddItemToArray(districtArray, scenJson);
	}
	/*获取设备信息*/

	


	sqlite3_finalize(stmt);
    sqlite3_close(db);

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    fprintf(stdout,"string:%s\n",p);
    /*response to client*/
    socketSeverSend((unsigned char*)p, strlen(p), data.clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;

}

