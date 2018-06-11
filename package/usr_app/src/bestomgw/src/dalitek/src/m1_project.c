#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "m1_project.h"
#include "m1_common_log.h"

/*app获取项目信息*/
int app_get_project_info(payload_t data)
{
	M1_LOG_DEBUG("app_get_project_info\n");

    int rc               = 0;
    int ret              = M1_PROTOCOL_OK;
    int pduType          = TYPE_M1_REPORT_PROJECT_NUMBER;
    char *pNumber        = NULL;
    char *pName          = NULL;
    char *sql            = NULL;
    cJSON *pJsonRoot     = NULL; 
    cJSON *pduJsonObject = NULL;
    cJSON *devDataObject = NULL;
    sqlite3 *db          = NULL;
    sqlite3_stmt *stmt   = NULL;

    db = data.db;
    /*get sql data json*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_DEBUG("pJsonRoot NULL\n");
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
    /*添加设备信息*/
    devDataObject = cJSON_CreateObject();
    if(NULL == devDataObject)
    {
        M1_LOG_DEBUG("devDataObject NULL\n");
        cJSON_Delete(devDataObject);
        return M1_PROTOCOL_FAILED;
    }
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataObject);
    /*获取项目信息*/
    sql = "select P_NUMBER,P_NAME from project_table order by ID desc limit 1;";
    M1_LOG_DEBUG( "%s\n", sql);
    
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    rc = sqlite3_step(stmt);     
    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        if(rc == SQLITE_CORRUPT)
            exit(0);
    }
    if(rc == SQLITE_ERROR)
    {
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }
    /*add devData to pdu*/
    pNumber = sqlite3_column_text(stmt, 0);
    if(pNumber == SQLITE_ERROR)
    {
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;	
    }
    pName = sqlite3_column_text(stmt, 1);
    if(pName == SQLITE_ERROR)
    {
        ret = M1_PROTOCOL_FAILED;
        goto Finish;    
    }
    cJSON_AddStringToObject(devDataObject, "proId", pNumber);
    cJSON_AddStringToObject(devDataObject, "proName", pName);

    char* p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);
    socketSeverSend((unsigned char*)p, strlen(p), data.clientFd);

	Finish:
    if(stmt)
        sqlite3_finalize(stmt);
    cJSON_Delete(pJsonRoot);

    return ret;
}

/*验证项目信息*/
int app_confirm_project(payload_t data)
{
	M1_LOG_DEBUG("app_confirm_project\n");
	int rc             = 0;
    int row_n          = 0;
    int ret            = M1_PROTOCOL_OK;
    char* key          = NULL;
    const char* ap_id  = NULL;
    cJSON* pNumberJson = NULL;
    cJSON* pKeyJson    = NULL;
    /*sql*/
    char* sql          = NULL;
    sqlite3* db        = NULL;
    sqlite3_stmt* stmt = NULL;

    db = data.db;
    pNumberJson = cJSON_GetObjectItem(data.pdu, "pNumber");
    if(pNumberJson == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;   
    }
    M1_LOG_DEBUG("pNumber:%s\n",pNumberJson->valuestring);
    pKeyJson = cJSON_GetObjectItem(data.pdu, "pKey");   
    if(pKeyJson == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;   
    }
    M1_LOG_DEBUG("pKey:%s\n",pKeyJson->valuestring);

    sql = "select P_KEY from project_table where P_NUMBER = ? order by ID desc limit 1;";
    M1_LOG_DEBUG( "%s\n", sql);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    sqlite3_bind_text(stmt, 1, pNumberJson->valuestring, -1, NULL);
    rc = sqlite3_step(stmt);    
    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        if(rc == SQLITE_CORRUPT)
            exit(0);
    }
    if(rc != SQLITE_ROW)
    {
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    key = sqlite3_column_text(stmt, 0);

    if(strcmp(key,pKeyJson->valuestring) != 0)
    {
        M1_LOG_INFO("Key not match\n");        
 		ret = M1_PROTOCOL_FAILED;
    }

    Finish:
    if(stmt)
 	  sqlite3_finalize(stmt);

    return ret;
}

/*新建项目*/
int app_create_project(payload_t data)
{
	M1_LOG_DEBUG("app_create_project\n");
    int rc              = 0;
    int ret             = M1_PROTOCOL_OK;
    /*Json*/
	cJSON* pNameJson    = NULL;
	cJSON* pNumberJson  = NULL;
	cJSON* pCreatorJson = NULL;
	cJSON* pManagerJson = NULL;
	cJSON* pTelJson     = NULL;
	cJSON* pAddJson     = NULL;
	cJSON* pBriefJson   = NULL;
    /*sql*/
    char* sql           = NULL;
    char* errorMsg      = NULL;
    sqlite3* db         = NULL;
    sqlite3_stmt* stmt  = NULL;

	pNameJson = cJSON_GetObjectItem(data.pdu, "pName");   
    M1_LOG_DEBUG("pName:%s\n",pNameJson->valuestring);
	pNumberJson = cJSON_GetObjectItem(data.pdu, "pNumber");   
    M1_LOG_DEBUG("pNumber:%s\n",pNumberJson->valuestring);
    pCreatorJson = cJSON_GetObjectItem(data.pdu, "pCreator");   
    M1_LOG_DEBUG("pCreator:%s\n",pCreatorJson->valuestring);
    pManagerJson = cJSON_GetObjectItem(data.pdu, "pManager");   
    M1_LOG_DEBUG("pManager:%s\n",pManagerJson->valuestring);
    pTelJson = cJSON_GetObjectItem(data.pdu, "pTel");   
    M1_LOG_DEBUG("pTel:%s\n",pTelJson->valuestring);
    pAddJson = cJSON_GetObjectItem(data.pdu, "pAdd");   
    M1_LOG_DEBUG("pAdd:%s\n",pAddJson->valuestring);
    pBriefJson = cJSON_GetObjectItem(data.pdu, "pBrief");   
    M1_LOG_DEBUG("pBrief:%s\n",pBriefJson->valuestring);
    /*获取数据路*/
    db = data.db;

    {
        sql = "insert or replace into project_table(P_NAME,P_NUMBER,P_CREATOR,P_MANAGER,P_EDITOR,P_TEL,P_ADD,P_BRIEF,P_KEY,ACCOUNT)\
        values(?,?,?,?,?,?,?,?,?,(select ACCOUNT from account_info where CLIENT_FD = ?));";
        M1_LOG_DEBUG( "%s\n", sql);
        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }    
    }

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, pNameJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 2, pNumberJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 3, pCreatorJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 4, pManagerJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 5, pCreatorJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 6, pTelJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 7, pAddJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 8, pBriefJson->valuestring, -1, NULL);
       	sqlite3_bind_text(stmt, 9, "123456", -1, NULL);
    
        rc = sqlite3_step(stmt);     
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            if(rc == SQLITE_CORRUPT)
                exit(0);
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

/*获取项目配置信息*/
int app_get_project_config(payload_t data)
{
	M1_LOG_DEBUG("app_get_project_config\n");
    
    int rc               = 0;
    int ret              = M1_PROTOCOL_OK;
    int pduType          = TYPE_M1_REPORT_PROJECT_CONFIG_INFO;
    char *pName          = NULL;
    char *pNumber        = NULL;
    char *pCreator       = NULL;
    char *pManager       = NULL;
    char *pTel           = NULL;
    char *pAdd           = NULL;
    char *pBrief         = NULL;
    char *pEditor        = NULL;
    char *pEditTime      = NULL;
    /*Json*/
    cJSON *pJsonRoot     = NULL; 
    cJSON *pduJsonObject = NULL;
    cJSON *devDataObject = NULL;
    /*sql*/
    char *sql = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    db = data.db;
    /*get sql data json*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_DEBUG("pJsonRoot NULL\n");
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
    sql = "select P_NAME,P_NUMBER,P_CREATOR,P_MANAGER,P_TEL,P_ADD,P_BRIEF,P_EDITOR,TIME from project_table order by ID desc limit 1;";
    M1_LOG_DEBUG( "%s\n", sql);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    if(sqlite3_step(stmt) == SQLITE_ERROR)
    {
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

    M1_LOG_DEBUG("string:%s\n",p);
    socketSeverSend((unsigned char*)p, strlen(p), data.clientFd);

    Finish:
    sqlite3_finalize(stmt);
    cJSON_Delete(pJsonRoot);

    return ret;
}

/*更改项目配置*/
int app_change_project_config(payload_t data)
{
	M1_LOG_DEBUG("app_change_project_config\n");

    int rc              = 0;
    int ret             = M1_PROTOCOL_OK;
    /*Json*/
    cJSON *pNameJson    = NULL;
    cJSON *pNumberJson  = NULL;
    cJSON *pCreatorJson = NULL;
    cJSON *pManagerJson = NULL;
    cJSON *pTelJson     = NULL;
    cJSON *pAddJson     = NULL;
    cJSON *pBriefJson   = NULL;
    cJSON *pEditorJson  = NULL;
    char* errorMsg      = NULL;
    /*sql*/
    char *sql           = NULL;
    sqlite3 *db         = NULL;
    sqlite3_stmt *stmt  = NULL;

    getNowTime(time);
    pNameJson = cJSON_GetObjectItem(data.pdu, "pName");   
    M1_LOG_DEBUG("pName:%s\n",pNameJson->valuestring);
    pNumberJson = cJSON_GetObjectItem(data.pdu, "pNumber");   
    M1_LOG_DEBUG("pNumber:%s\n",pNumberJson->valuestring);
    pCreatorJson = cJSON_GetObjectItem(data.pdu, "pCreator");   
    M1_LOG_DEBUG("pCreator:%s\n",pCreatorJson->valuestring);
    pManagerJson = cJSON_GetObjectItem(data.pdu, "pManager");   
    M1_LOG_DEBUG("pManager:%s\n",pManagerJson->valuestring);
    pTelJson = cJSON_GetObjectItem(data.pdu, "pTel");   
    M1_LOG_DEBUG("pTel:%s\n",pTelJson->valuestring);
    pAddJson = cJSON_GetObjectItem(data.pdu, "pAdd");   
    M1_LOG_DEBUG("pAdd:%s\n",pAddJson->valuestring);
    pBriefJson = cJSON_GetObjectItem(data.pdu, "pBrief");   
    M1_LOG_DEBUG("pBrief:%s\n",pBriefJson->valuestring);
    pEditorJson = cJSON_GetObjectItem(data.pdu, "pEditor");   
    M1_LOG_DEBUG("pBrief:%s\n",pEditorJson->valuestring);

    db = data.db;

    {
        sql = "insert into project_table(P_NAME,P_NUMBER,P_CREATOR,P_MANAGER,P_EDITOR,P_TEL,P_ADD,P_BRIEF,P_KEY,ACCOUNT)\
        values(?,?,?,?,?,?,?,?,(select P_KEY from project_table order by ID desc limit 1),(select ACCOUNT from account_info where CLIENT_FD = ?));";
        M1_LOG_DEBUG("sql:%s\n", sql);
        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }

    }
    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, pNameJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 2, pNumberJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 3, pCreatorJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 4, pManagerJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 5, pEditorJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 6, pTelJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 7, pAddJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 8, pBriefJson->valuestring, -1, NULL);
        sqlite3_bind_int(stmt, 9,  data.clientFd);
    
        rc = sqlite3_step(stmt);     
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            if(rc == SQLITE_CORRUPT)
                exit(0);
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

/*更改项目密码*/
int app_change_project_key(payload_t data)
{
	M1_LOG_DEBUG("app_change_project_key\n");
    /*sqlite3*/
    int rc                = 0;
    int ret               = M1_PROTOCOL_OK;
    int id                = 0;
    char *key             = NULL;
    char* errorMsg        = NULL;
    char *sql             = NULL;
    char *sql_1           = NULL;
    sqlite3 *db           = NULL;
    sqlite3_stmt *stmt    = NULL;
    sqlite3_stmt *stmt_1  = NULL;
	cJSON *pKeyJson       = NULL;
	cJSON *newKeyJson     = NULL;
	cJSON *confirmKeyJson = NULL;
	
	pKeyJson = cJSON_GetObjectItem(data.pdu, "pKey");
	if(pKeyJson == NULL){
		ret = M1_PROTOCOL_FAILED;
		goto Finish;   
	}
    M1_LOG_DEBUG("pKey:%s\n",pKeyJson->valuestring);
	newKeyJson = cJSON_GetObjectItem(data.pdu, "newKey");   
	if(newKeyJson == NULL){
		ret = M1_PROTOCOL_FAILED;
		goto Finish;
	}
    M1_LOG_DEBUG("newKey:%s\n",newKeyJson->valuestring);
    confirmKeyJson = cJSON_GetObjectItem(data.pdu, "confirmKey");   
    if(confirmKeyJson == NULL){
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }
    M1_LOG_DEBUG("confirmKey:%s\n",confirmKeyJson->valuestring);
    if(strcmp(confirmKeyJson->valuestring, newKeyJson->valuestring) != 0){
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;	
    }
    /*获取数据库*/
    db = data.db;
    /*获取账户信息*/
    sql = "select P_KEY,ID from project_table order by ID desc limit 1;";
    M1_LOG_DEBUG( "%s\n", sql);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    if(sqlite3_step(stmt) == SQLITE_ERROR)
    {
        M1_LOG_ERROR( "SQLITE_ERROR\n");
    	ret = M1_PROTOCOL_FAILED;
    	goto Finish;
    }
    key = sqlite3_column_text(stmt, 0);
    if(key == NULL){
        M1_LOG_ERROR( "get key error\n");
    	ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    M1_LOG_DEBUG( "key:%s\n", key);
    id = sqlite3_column_int(stmt, 1);
    if(id == NULL){
        M1_LOG_ERROR( "get id error\n");
    	ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    if(strcmp(key, pKeyJson->valuestring) != 0){
        M1_LOG_ERROR( "key not match\n");
    	ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    sql_1 = "update project_table set P_KEY = ? where ID = ?;";
    M1_LOG_DEBUG("sql_1:%s\n", sql_1);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        sqlite3_bind_text(stmt_1, 1, newKeyJson->valuestring, -1, NULL);
        sqlite3_bind_int(stmt_1, 2, id);
        rc = sqlite3_step(stmt_1);
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            if(rc == SQLITE_CORRUPT)
                exit(0);
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


