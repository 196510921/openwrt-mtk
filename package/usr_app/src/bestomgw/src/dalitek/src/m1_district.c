#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "m1_protocol.h"
#include "socket_server.h"
#include "m1_common_log.h"


int district_create_handle(payload_t data)
{
	M1_LOG_DEBUG("district_create_handle\n");
    int rc, ret = M1_PROTOCOL_OK;
    int number1,i;
    int row_number = 0;
    char* sql_1 = (char*)malloc(300);
    char* errorMsg = NULL;
    char* sql = NULL;
    cJSON* districtNameJson = NULL;
    cJSON* disPicJson = NULL;
	cJSON* apIdJson = NULL;
	cJSON* apIdArrayJson = NULL;
	sqlite3* db = NULL;
	sqlite3_stmt* stmt = NULL;

	if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;  
        goto Finish;
    } 

    /*获取收到数据包信息*/
    districtNameJson = cJSON_GetObjectItem(data.pdu, "districtName");
    if(districtNameJson == NULL){
        M1_LOG_DEBUG("districtNameJson NULL\n");
        goto Finish;
    }
    M1_LOG_DEBUG("districtName:%s\n",districtNameJson->valuestring);
    disPicJson = cJSON_GetObjectItem(data.pdu, "disPic");
    if(disPicJson == NULL){
        M1_LOG_DEBUG("disPicJson NULL\n");
        goto Finish;
    }
    M1_LOG_DEBUG("dis_pic:%s\n",disPicJson->valuestring);
    apIdArrayJson = cJSON_GetObjectItem(data.pdu, "apId");
    if(apIdArrayJson == NULL){
        M1_LOG_DEBUG("apIdArrayJson NULL\n");
        goto Finish;
    }
    number1 = cJSON_GetArraySize(apIdArrayJson);
    M1_LOG_DEBUG("number1:%d\n",number1);

    /*获取数据路*/
    db = data.db;

	sql = "insert or replace into district_table(DIS_NAME, DIS_PIC, AP_ID, ACCOUNT) values(?,?,?,?);";
    M1_LOG_DEBUG("sql:%s\n",sql);

    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

	/*linkage_table*/
    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        M1_LOG_DEBUG("BEGIN IMMEDIATE\n");
        /*存取到数据表scenario_table中*/
        for(i = 0; i < number1; i++)
        {
    		apIdJson = cJSON_GetArrayItem(apIdArrayJson, i);
    		M1_LOG_DEBUG("apId:%s\n",apIdJson->valuestring);

    		sqlite3_bind_text(stmt, 1, districtNameJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt, 2, disPicJson->valuestring, -1, NULL);
    		sqlite3_bind_text(stmt, 3, apIdJson->valuestring, -1, NULL);
    		sqlite3_bind_text(stmt, 4, "Dalitek", -1, NULL);

    		rc = sqlite3_step(stmt);   
            M1_LOG_DEBUG("step() return %s, number:%03d\n",\
                rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
                
            if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            }

            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt); 
        }
        rc = sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg);
        if(rc == SQLITE_OK)
        {
            M1_LOG_DEBUG("COMMIT OK\n");
        }
        else if(rc == SQLITE_BUSY)
        {
            M1_LOG_WARN("等待再次提交\n");
        }
        else
        {
            M1_LOG_WARN("COMMIT errorMsg:%s\n",errorMsg);
            sqlite3_free(errorMsg);
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

int app_req_district(payload_t data)
{
	M1_LOG_DEBUG("app_req_district\n");
    int pduType               = TYPE_M1_REPORT_DISTRICT_INFO;
    int rc                    = 0;
    int ret                   = M1_PROTOCOL_OK;
    char *dis_pic             = NULL;
    char *account             = NULL;
    char *dist_name           = NULL;
    char *ap_id               = NULL;
    char *ap_name             = NULL;
    int pId                   = 0;
    /*cJSON*/
    cJSON * pJsonRoot         = NULL;
    cJSON * pduJsonObject     = NULL;
    cJSON * devDataJsonArray  = NULL;
    cJSON*  devDataObject     = NULL;
    cJSON*  apInfoObject      = NULL;
    cJSON*  apInfoArrayObject = NULL;
    /*sql*/
    char *sql                 = NULL;
    char *sql_1               = NULL;
    char *sql_2               = NULL;
    char *sql_3               = NULL;
    char *sql_4               = NULL;
    sqlite3* db               = NULL;
    sqlite3_stmt *stmt        = NULL;
    sqlite3_stmt *stmt_1      = NULL;
    sqlite3_stmt *stmt_2      = NULL;
    sqlite3_stmt *stmt_3      = NULL;
    sqlite3_stmt *stmt_4      = NULL;

    db = data.db;
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_ERROR("pJsonRoot NULL\n");
        cJSON_Delete(pJsonRoot);
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
    /*create devData array*/
    devDataJsonArray = cJSON_CreateArray();
    if(NULL == devDataJsonArray)
    {
        cJSON_Delete(devDataJsonArray);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataJsonArray);
    /*获取用户账户信息*/
    {
        sql = "select ACCOUNT from account_info where CLIENT_FD = ? order by ID desc limit 1;";
        M1_LOG_DEBUG("sql:%s\n", sql);

        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }

        sqlite3_bind_int(stmt, 1, data.clientFd);
        if(sqlite3_step(stmt) == SQLITE_ROW)
        {
            account = sqlite3_column_text(stmt, 0);
        }
        if(account == NULL)
        {
            M1_LOG_ERROR( "user account do not exist\n");    
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }
        else
        {
            M1_LOG_DEBUG("clientFd:%03d,account:%s\n",data.clientFd, account);
        }
    }

    {
        /*取区域名称*/
       	sql_1 = "select distinct DIS_NAME from district_table where ACCOUNT = ?;";
        M1_LOG_DEBUG("sql_1:%s\n", sql_1);
        
        if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        /*添加区域图片*/
        sql_2 = "select DIS_PIC from district_table where DIS_NAME = ? order by ID desc limit 1;";
        M1_LOG_DEBUG("sql_2:%s\n", sql_2);
        if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        /*获取ap id*/
        sql_3 = "select AP_ID from district_table where DIS_NAME = ? and ACCOUNT = ?;";
        M1_LOG_DEBUG("sql_3:%s\n", sql_3);
        if(sqlite3_prepare_v2(db, sql_3, strlen(sql_3), &stmt_3, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }     
        /*取出apName*/
        sql_4 = "select DEV_NAME,pId from all_dev where DEV_ID = ?;";
        M1_LOG_DEBUG("sql_4:%s\n", sql_4);
        if(sqlite3_prepare_v2(db, sql_4, strlen(sql_4), &stmt_4, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }    
    }

    sqlite3_bind_text(stmt_1, 1, account, -1, NULL);
    while(sqlite3_step(stmt_1) == SQLITE_ROW)
    {
    	dist_name = sqlite3_column_text(stmt_1,0);
	    devDataObject = cJSON_CreateObject();
	    if(NULL == devDataObject)
	    {
	        cJSON_Delete(devDataObject);
	        ret = M1_PROTOCOL_FAILED;
            goto Finish;
	    }
	    cJSON_AddItemToArray(devDataJsonArray, devDataObject);
	    cJSON_AddStringToObject(devDataObject, "district", dist_name);
        
        /*添加区域图片*/
        sqlite3_bind_text(stmt_2, 1, dist_name, -1, NULL);
	    rc = sqlite3_step(stmt_2);
        if(rc != SQLITE_ROW)
        {
            sqlite3_reset(stmt_2);
            sqlite3_clear_bindings(stmt_2);
            continue;   
        }
        dis_pic = sqlite3_column_text(stmt_2, 0);
        cJSON_AddStringToObject(devDataObject, "disPic", dis_pic);
        M1_LOG_DEBUG("dis_pic:%s\n", dis_pic);
        /*创建ap_info数组*/
	    apInfoArrayObject = cJSON_CreateArray();
	    if(NULL == apInfoArrayObject)
	    {
	        cJSON_Delete(apInfoArrayObject);
	        ret = M1_PROTOCOL_FAILED;
            goto Finish;
	    }
	    cJSON_AddItemToObject(devDataObject, "apInfo", apInfoArrayObject);
	    
        sqlite3_bind_text(stmt_3, 1, dist_name, -1, NULL);
        sqlite3_bind_text(stmt_3, 2, account, -1, NULL);
	    while(sqlite3_step(stmt_3) == SQLITE_ROW)
        {
	    	ap_id = sqlite3_column_text(stmt_3,0);
	    	M1_LOG_DEBUG("ap_id:%s\n",ap_id);
		    apInfoObject = cJSON_CreateObject();
		    if(NULL == apInfoObject)
		    {
		        cJSON_Delete(apInfoObject);
		        ret = M1_PROTOCOL_FAILED;
                goto Finish;
		    }
		    cJSON_AddItemToArray(apInfoArrayObject, apInfoObject);
		    cJSON_AddStringToObject(apInfoObject, "apId", ap_id);
		    
            /*取出apName*/
            sqlite3_bind_text(stmt_4, 1, ap_id, -1, NULL);
		    rc = sqlite3_step(stmt_4); 
			M1_LOG_DEBUG("step() return %s\n", \
                rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
			if(rc == SQLITE_ROW)
            {
				ap_name = sqlite3_column_text(stmt_4,0);
                pId = sqlite3_column_int(stmt_4,1);
				M1_LOG_DEBUG("ap_name:%s\n, pId:%05d\n",ap_name, pId);
				cJSON_AddStringToObject(apInfoObject, "apName", ap_name);
                cJSON_AddNumberToObject(apInfoObject, "pId", pId);
			}

            sqlite3_reset(stmt_4);
            sqlite3_clear_bindings(stmt_4);    
	    }
        sqlite3_reset(stmt_2);
        sqlite3_clear_bindings(stmt_2);
        sqlite3_reset(stmt_3);
        sqlite3_clear_bindings(stmt_3);
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
    socketSeverSend((uint8*)p, strlen(p), data.clientFd);
    
    Finish:
    if(stmt)
        sqlite3_finalize(stmt);
    if(stmt_1)
        sqlite3_finalize(stmt_1);
    if(stmt_2)
        sqlite3_finalize(stmt_2);
    if(stmt_3)
        sqlite3_finalize(stmt_3);
    if(stmt_4)
        sqlite3_finalize(stmt_4);
    
    cJSON_Delete(pJsonRoot);
    return ret;

}




