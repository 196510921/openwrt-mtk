#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "m1_project.h"

/*app获取项目信息*/
int app_get_project_info(payload_t data)
{
	fprintf(stdout,"app_get_project_info\n");

    int rc,ret = M1_PROTOCOL_OK;
    int pduType = TYPE_M1_REPORT_PROJECT_NUMBER;
    char* pNumber = NULL;
    char* sql = NULL;
    cJSON * pJsonRoot = NULL; 
    cJSON * pduJsonObject = NULL;
    cJSON * devDataObject= NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    db = data.db;
    /*get sql data json*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        fprintf(stdout,"pJsonRoot NULL\n");
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
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
        return M1_PROTOCOL_FAILED;
    }
    /*add pdu to root*/
    cJSON_AddItemToObject(pJsonRoot, "pdu", pduJsonObject);
    /*add pdu type to pdu object*/
    cJSON_AddNumberToObject(pduJsonObject, "pduType", pduType);
    /*获取项目信息*/
    sql = "select P_NUMBER from project_table";
    fprintf(stdout, "%s\n", sql);
    //sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    rc = thread_sqlite3_step(&stmt, db);
    if(rc == SQLITE_ERROR){
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }
    /*add devData to pdu*/
    pNumber = sqlite3_column_text(stmt, 0);
    if(pNumber == SQLITE_ERROR){
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;	
    }
    cJSON_AddStringToObject(pduJsonObject, "devData", pNumber);

    char* p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }

    fprintf(stdout,"string:%s\n",p);
    socketSeverSend((unsigned char*)p, strlen(p), data.clientFd);

	Finish:
    sqlite3_finalize(stmt);
    cJSON_Delete(pJsonRoot);

    return ret;

}

/*验证项目信息*/
int app_confirm_project(payload_t data)
{
	fprintf(stdout,"app_confirm_project\n");
	int rc,row_n;
    char* sql = (char*)malloc(300);
    char* key = NULL;
    const char* ap_id = NULL;
    cJSON* pNumberJson = NULL;
    cJSON* pKeyJson = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    db = data.db;
    pNumberJson = cJSON_GetObjectItem(data.pdu, "pNumber");
    if(pNumberJson == NULL)
        return M1_PROTOCOL_FAILED;   
    fprintf(stdout,"pNumber:%s\n",pNumberJson->valuestring);
    pKeyJson = cJSON_GetObjectItem(data.pdu, "pKey");   
    if(pKeyJson == NULL)
        return M1_PROTOCOL_FAILED;   
    fprintf(stdout,"pKey:%s\n",pKeyJson->valuestring);

    sprintf(sql,"select P_KEY from project_table where P_NUMBER = \"%s\";",pNumberJson->valuestring);
    fprintf(stdout, "%s\n", sql);
    //sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    thread_sqlite3_step(&stmt, db);
    key =  sqlite3_column_text(stmt, 0);

    if(strcmp(key,pKeyJson->valuestring) == 0)
 		rc = M1_PROTOCOL_OK;
 	else
 		rc = M1_PROTOCOL_FAILED;

    Finish:
    free(sql);
 	sqlite3_finalize(stmt);

    return rc;
}

/*新建项目*/
int app_create_project(payload_t data)
{
	fprintf(stdout,"app_create_project\n");
    int rc,ret = M1_PROTOCOL_OK;
    int row_n,id;
    char* account = NULL;
	char* sql = NULL;
    char* time = (char*)malloc(30);
	char* sql_1 = (char*)malloc(300);
	cJSON* pNameJson = NULL;
	cJSON* pNumberJson = NULL;
	cJSON* pCreatorJson = NULL;
	cJSON* pManagerJson = NULL;
	cJSON* pTelJson = NULL;
	cJSON* pAddJson = NULL;
	cJSON* pBriefJson = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL,*stmt_1 = NULL;

	getNowTime(time);
	pNameJson = cJSON_GetObjectItem(data.pdu, "pName");   
    fprintf(stdout,"pName:%s\n",pNameJson->valuestring);
	pNumberJson = cJSON_GetObjectItem(data.pdu, "pNumber");   
    fprintf(stdout,"pNumber:%s\n",pNumberJson->valuestring);
    pCreatorJson = cJSON_GetObjectItem(data.pdu, "pCreator");   
    fprintf(stdout,"pCreator:%s\n",pCreatorJson->valuestring);
    pManagerJson = cJSON_GetObjectItem(data.pdu, "pManager");   
    fprintf(stdout,"pManager:%s\n",pManagerJson->valuestring);
    pTelJson = cJSON_GetObjectItem(data.pdu, "pTel");   
    fprintf(stdout,"pTel:%s\n",pTelJson->valuestring);
    pAddJson = cJSON_GetObjectItem(data.pdu, "pAdd");   
    fprintf(stdout,"pAdd:%s\n",pAddJson->valuestring);
    pBriefJson = cJSON_GetObjectItem(data.pdu, "pBrief");   
    fprintf(stdout,"pBrief:%s\n",pBriefJson->valuestring);
    /*获取数据路*/
    db = data.db;

    sql = "select ID from project_table order by ID desc limit 1";
    id = sql_id(db, sql);
    fprintf(stdout, "get id end\n");
    /*获取账户信息*/
    sprintf(sql_1,"select ACCOUNT from account_info where CLIENT_FD = %03d;", data.clientFd);
    fprintf(stdout, "%s\n", sql_1);
    //sqlite3_reset(stmt_1);
    sqlite3_finalize(stmt_1);
    sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
    if(thread_sqlite3_step(&stmt_1, db) == SQLITE_ERROR){
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }
    account = sqlite3_column_text(stmt_1, 0);
    fprintf(stdout, "account:%s\n", account);
    /*插入到项目表中*/
    sql = "insert into project_table(ID,P_NAME,P_NUMBER,P_CREATOR,P_MANAGER,P_EDITOR,P_TEL,P_ADD,P_BRIEF,P_KEY,ACCOUNT,TIME)values(?,?,?,?,?,?,?,?,?,?,?,?);";
    fprintf(stdout, "%s\n", sql);
    //sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, pNameJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt, 3, pNumberJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt, 4, pCreatorJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt, 5, pManagerJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt, 6, pCreatorJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt, 7, pTelJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt, 8, pAddJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt, 9, pBriefJson->valuestring, -1, NULL);
   	sqlite3_bind_text(stmt, 10, "123456", -1, NULL);
    sqlite3_bind_text(stmt, 11, account, -1, NULL);
    sqlite3_bind_text(stmt, 12,  time, -1, NULL);
    
    thread_sqlite3_step(&stmt,db);

    Finish:
    free(time);
    free(sql_1);
 	sqlite3_finalize(stmt);
 	sqlite3_finalize(stmt_1);

    return ret;	
}

/*获取项目配置信息*/
int app_get_project_config(payload_t data)
{
	fprintf(stdout,"app_get_project_config\n");
    
    int rc,ret = M1_PROTOCOL_OK;
    int pduType = TYPE_M1_REPORT_PROJECT_CONFIG_INFO;
    char* pName = NULL,*pNumber = NULL,*pCreator = NULL,*pManager = NULL,*pTel = NULL;
    char *pAdd = NULL,*pBrief = NULL,*pEditor = NULL,*pEditTime = NULL;
    char* sql = NULL;
    cJSON * pJsonRoot = NULL; 
    cJSON * pduJsonObject = NULL;
    cJSON * devDataObject= NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    db = data.db;
    /*get sql data json*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        fprintf(stdout,"pJsonRoot NULL\n");
        ret =  M1_PROTOCOL_FAILED;
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
    /*获取项目信息*/
    sql = "select P_NAME,P_NUMBER,P_CREATOR,P_MANAGER,P_TEL,P_ADD,P_BRIEF,P_EDITOR,TIME from project_table order by ID desc limit 1";
    fprintf(stdout, "%s\n", sql);
    //sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(thread_sqlite3_step(&stmt, db) == SQLITE_ERROR){
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }
    /*获取sql中项目配置信息*/
    pName = sqlite3_column_text(stmt, 0);
    pNumber = sqlite3_column_text(stmt, 1);
    pCreator = sqlite3_column_text(stmt, 2);
    pManager = sqlite3_column_text(stmt, 3);
    pTel = sqlite3_column_text(stmt, 4);
    pAdd = sqlite3_column_text(stmt, 5);
    pBrief = sqlite3_column_text(stmt, 6);
    pEditor = sqlite3_column_text(stmt, 7);
    pEditTime = sqlite3_column_text(stmt, 8);
    /*将配置信息加入到json*/
    devDataObject = cJSON_CreateObject();
    if(NULL == devDataObject)
    {
        // create object faild, exit
        cJSON_Delete(devDataObject);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataObject);
    cJSON_AddStringToObject(devDataObject, "pName", pName);
    cJSON_AddStringToObject(devDataObject, "pNumber", pNumber);
    cJSON_AddStringToObject(devDataObject, "pCreator", pCreator);
    cJSON_AddStringToObject(devDataObject, "pManager", pManager);
    cJSON_AddStringToObject(devDataObject, "pTel", pTel);
    cJSON_AddStringToObject(devDataObject, "pAdd", pAdd);
    cJSON_AddStringToObject(devDataObject, "pBrief", pBrief);
    cJSON_AddStringToObject(devDataObject, "pEditor", pEditor);
    cJSON_AddStringToObject(devDataObject, "pEditTime", pEditTime);

    char* p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }

    fprintf(stdout,"string:%s\n",p);
    socketSeverSend((unsigned char*)p, strlen(p), data.clientFd);

    Finish:
    sqlite3_finalize(stmt);
    cJSON_Delete(pJsonRoot);

    return ret;
}

/*更改项目配置*/
int app_change_project_config(payload_t data)
{
	fprintf(stdout,"app_change_project_config\n");

    int row_n,id;
    int rc, ret = M1_PROTOCOL_OK;
	char* time = (char*)malloc(30);
	char* sql_1 = (char*)malloc(300);
    char* account = NULL, *pKey = NULL;
    char* sql = NULL;
	cJSON* pNameJson = NULL;
	cJSON* pNumberJson = NULL;
	cJSON* pCreatorJson = NULL;
	cJSON* pManagerJson = NULL;
	cJSON* pTelJson = NULL;
	cJSON* pAddJson = NULL;
	cJSON* pBriefJson = NULL;
	cJSON* pEditorJson = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL,*stmt_1 = NULL,*stmt_2 = NULL;

	getNowTime(time);
	pNameJson = cJSON_GetObjectItem(data.pdu, "pName");   
    fprintf(stdout,"pName:%s\n",pNameJson->valuestring);
	pNumberJson = cJSON_GetObjectItem(data.pdu, "pNumber");   
    fprintf(stdout,"pNumber:%s\n",pNumberJson->valuestring);
    pCreatorJson = cJSON_GetObjectItem(data.pdu, "pCreator");   
    fprintf(stdout,"pCreator:%s\n",pCreatorJson->valuestring);
    pManagerJson = cJSON_GetObjectItem(data.pdu, "pManager");   
    fprintf(stdout,"pManager:%s\n",pManagerJson->valuestring);
    pTelJson = cJSON_GetObjectItem(data.pdu, "pTel");   
    fprintf(stdout,"pTel:%s\n",pTelJson->valuestring);
    pAddJson = cJSON_GetObjectItem(data.pdu, "pAdd");   
    fprintf(stdout,"pAdd:%s\n",pAddJson->valuestring);
    pBriefJson = cJSON_GetObjectItem(data.pdu, "pBrief");   
    fprintf(stdout,"pBrief:%s\n",pBriefJson->valuestring);
    pEditorJson = cJSON_GetObjectItem(data.pdu, "pEditor");   
    fprintf(stdout,"pBrief:%s\n",pEditorJson->valuestring);

    db = data.db;
    sql = "select ID, P_KEY from project_table order by ID desc limit 1";
    sqlite3_prepare_v2(db, sql, strlen(sql), & stmt, NULL);
    //sqlite3_reset(stmt);
    rc = thread_sqlite3_step(&stmt, db);
    if(rc == SQLITE_ROW){
        id = (sqlite3_column_int(stmt, 0) + 1);
    }else{
        id = 1;
    }
    pKey = sqlite3_column_text(stmt, 1);
    if(pKey == NULL)
    {
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }
    /*获取账户信息*/
    sprintf(sql_1,"select ACCOUNT from account_info where CLIENT_FD = %3d;", data.clientFd);
    fprintf(stdout, "%s\n", sql_1);
    //sqlite3_reset(stmt_1);
    sqlite3_finalize(stmt_1);
    sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
    if(thread_sqlite3_step(&stmt_1, db) == SQLITE_ERROR){
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }
    account = sqlite3_column_text(stmt_1, 0);
    fprintf(stdout, "account:%s\n", account);
    /*插入到项目表中*/
    sql = "insert into project_table(ID,P_NAME,P_NUMBER,P_CREATOR,P_MANAGER,P_EDITOR,P_TEL,P_ADD,P_BRIEF,P_KEY,ACCOUNT,TIME)values(?,?,?,?,?,?,?,?,?,?,?,?);";
    fprintf(stdout, "%s\n", sql);
    //sqlite3_reset(stmt_2);
    sqlite3_finalize(stmt_2);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt_2, NULL);
    sqlite3_bind_int(stmt_2, 1, id);
    sqlite3_bind_text(stmt_2, 2, pNameJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt_2, 3, pNumberJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt_2, 4, pCreatorJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt_2, 5, pManagerJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt_2, 6, pEditorJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt_2, 7, pTelJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt_2, 8, pAddJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt_2, 9, pBriefJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt_2, 10, pKey, -1, NULL);
    sqlite3_bind_text(stmt_2, 11, account, -1, NULL);
    sqlite3_bind_text(stmt_2, 12,  time, -1, NULL);
    
    thread_sqlite3_step(&stmt_2,db);
    Finish:
    free(time);
    free(sql_1);
 	sqlite3_finalize(stmt);
 	sqlite3_finalize(stmt_1);
    sqlite3_finalize(stmt_2);

    return ret;	
}

/*更改项目密码*/
int app_change_project_key(payload_t data)
{
	fprintf(stdout,"app_change_project_key\n");
    /*sqlite3*/
    int id;
    int rc, ret = M1_PROTOCOL_OK;
    char* key = NULL;
    char* time = (char*)malloc(30);
    char* sql= (char*)malloc(300);
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
	cJSON* pKeyJson = NULL;
	cJSON* newKeyJson = NULL;
	cJSON* confirmKeyJson = NULL;
	
	getNowTime(time);
	pKeyJson = cJSON_GetObjectItem(data.pdu, "pKey");
	if(pKeyJson == NULL){
		ret = M1_PROTOCOL_FAILED;
		goto Finish;   
	}
    fprintf(stdout,"pKey:%s\n",pKeyJson->valuestring);
	newKeyJson = cJSON_GetObjectItem(data.pdu, "newKey");   
	if(newKeyJson == NULL){
		ret = M1_PROTOCOL_FAILED;
		goto Finish;
	}
    fprintf(stdout,"newKey:%s\n",newKeyJson->valuestring);
    confirmKeyJson = cJSON_GetObjectItem(data.pdu, "confirmKey");   
    if(confirmKeyJson == NULL){
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }
    fprintf(stdout,"confirmKey:%s\n",confirmKeyJson->valuestring);
    if(strcmp(confirmKeyJson->valuestring, newKeyJson->valuestring) != 0){
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;	
    }
    /*获取数据库*/
    db = data.db;
    /*获取账户信息*/
    sprintf(sql,"select P_KEY,ID from project_table order by ID desc limit 1;");
    fprintf(stdout, "%s\n", sql);
    //sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(thread_sqlite3_step(&stmt, db) == SQLITE_ERROR){
        fprintf(stderr, "SQLITE_ERROR\n");
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }
    key = sqlite3_column_text(stmt, 0);
    if(key == NULL){
        fprintf(stderr, "get key error\n");
    	ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    fprintf(stdout, "key:%s\n", key);
    id = sqlite3_column_int(stmt, 1);
    if(id == NULL){
        fprintf(stderr, "get id error\n");
    	ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    if(strcmp(key, pKeyJson->valuestring) != 0){
        fprintf(stderr, "key not match\n");
    	ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    sprintf(sql,"update project_table set P_KEY = \"%s\" where ID = %05d;",newKeyJson->valuestring, id);
    fprintf(stdout, "%s\n", sql);
    //sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(thread_sqlite3_step(&stmt, db) == SQLITE_ERROR){
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }

    Finish:
    free(time);
    free(sql);
 	sqlite3_finalize(stmt);

    return ret;	
}


