#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "m1_project.h"

/*app获取项目信息*/
int app_get_project_info(int clientFd, int sn)
{
	fprintf(stdout,"app_get_project_info\n");
    cJSON * pJsonRoot = NULL; 
    cJSON * pduJsonObject = NULL;
    cJSON * devDataObject= NULL;

    int pduType = TYPE_M1_REPORT_PROJECT_NUMBER;
    char* project = NULL;
    char* sql = NULL;
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int rc;

    rc = sqlite3_open("dev_info.db", &db);  
    if( rc ){  
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
    /*获取项目信息*/
    sql = "select P_NUMBER from project_table";
    fprintf(stdout, "%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    thread_sqlite3_step(&stmt, db);
    /*add devData to pdu*/
    cJSON_AddStringToObject(pduJsonObject, "devData", project);

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return M1_PROTOCOL_OK;
}

/*验证项目信息*/
int app_confirm_project(payload_t data)
{
	fprintf(stdout,"app_confirm_project\n");
	cJSON* pNumberJson = NULL;
    cJSON* pKeyJson = NULL;
    char sql[200];
    char* key = NULL;

    pNumberJson = cJSON_GetObjectItem(data.pdu, "pNumber");   
    fprintf(stdout,"pNumber:%s\n",pNumberJson->valuestring);
    pKeyJson = cJSON_GetObjectItem(data.pdu, "pKey");   
    fprintf(stdout,"pKey:%s\n",pKeyJson->valuestring);

	/*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int rc,row_n;
    const char* ap_id = NULL;

    rc = sqlite3_open("dev_info.db", &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    sprintf(sql,"select P_KEY from project_table where P_NUMBER = %s\n;",pNumberJson->valuestring);
    fprintf(stdout, "%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    thread_sqlite3_step(&stmt, db);
    key =  sqlite3_column_text(stmt, 0);

    if(strcmp(key,pKeyJson->valuestring) == 0)
 		rc = M1_PROTOCOL_OK;
 	else
 		rc = M1_PROTOCOL_FAILED;

 	sqlite3_finalize(stmt);
    sqlite3_close(db);   

    return rc;
}



