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
