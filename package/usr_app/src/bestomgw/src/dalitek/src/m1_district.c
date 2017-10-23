#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "m1_protocol.h"
#include "socket_server.h"


int district_create_handle(payload_t data)
{
	fprintf(stdout,"district_create_handle\n");
	cJSON* districtNameJson = NULL;
	cJSON* apIdJson = NULL;
	cJSON* apIdArrayJson = NULL;

	sqlite3* db = NULL;
	sqlite3_stmt* stmt = NULL;
	int id, rc, number1,i;
	int row_number = 0;
	char time[30];
	char sql_1[200];
    char* errorMsg = NULL;

	if(data.pdu == NULL) return M1_PROTOCOL_FAILED;
	getNowTime(time);

    rc = sqlite3_open("dev_info.db", &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

	/*获取table id*/
	char* sql = "select ID from district_table order by ID desc limit 1";
	/*linkage_table*/
	id = sql_id(db, sql);
    if(sqlite3_exec(db, "BEGIN", NULL, NULL, &errorMsg)==SQLITE_OK){
        fprintf(stdout,"BEGIN\n");
    	/*获取收到数据包信息*/
        districtNameJson = cJSON_GetObjectItem(data.pdu, "districtName");
        fprintf(stdout,"districtName:%s\n",districtNameJson->valuestring);
        apIdArrayJson = cJSON_GetObjectItem(data.pdu, "apId");
        number1 = cJSON_GetArraySize(apIdArrayJson);
        fprintf(stdout,"number1:%d\n",number1);

       	/*删除原有表scenario_table中的旧scenario*/
    	sprintf(sql_1,"select ID from district_table where DIS_NAME = \"%s\";",districtNameJson->valuestring);	
    	row_number = sql_row_number(db, sql_1);
    	fprintf(stdout,"row_number:%d\n",row_number);
    	if(row_number > 0){
    		sprintf(sql_1,"delete from district_table where DIS_NAME = \"%s\";",districtNameJson->valuestring);				
    		sqlite3_reset(stmt);
    		sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
    		while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW);
    	}
        
        /*存取到数据表scenario_table中*/
        for(i = 0; i < number1; i++){
    		apIdJson = cJSON_GetArrayItem(apIdArrayJson, i);
    		fprintf(stdout,"apId:%s\n",apIdJson->valuestring);

    		sql = "insert into district_table(ID, DIS_NAME, AP_ID, ACCOUNT,TIME) values(?,?,?,?,?);";
    		fprintf(stdout,"sql:%s\n",sql);
    		sqlite3_reset(stmt);
    		sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    		sqlite3_bind_int(stmt, 1, id);
    		id++;
    		sqlite3_bind_text(stmt, 2, districtNameJson->valuestring, -1, NULL);
    		sqlite3_bind_text(stmt, 3, apIdJson->valuestring, -1, NULL);
    		sqlite3_bind_text(stmt, 4, "Dalitek", -1, NULL);
            sqlite3_bind_text(stmt, 5, time, -1, NULL);
    		rc = thread_sqlite3_step(&stmt, db); 
    		fprintf(stdout,"step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
        }
        if(sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg) == SQLITE_OK){
            fprintf(stdout,"END\n");
        }
    }else{
        fprintf(stdout,"errorMsg:");
    }    
    sqlite3_free(errorMsg);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return M1_PROTOCOL_OK;
    
}

int app_req_district(int clientFd, int sn)
{
	fprintf(stdout,"app_req_district\n");
	/*cJSON*/
    int pduType = TYPE_M1_REPORT_DISTRICT_INFO;

    cJSON * pJsonRoot = NULL;
    cJSON * pduJsonObject = NULL;
    cJSON * devDataJsonArray = NULL;
    cJSON*  devDataObject= NULL;
    cJSON*  apInfoObject= NULL;
    cJSON*  apInfoArrayObject= NULL;
    
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL, *stmt_1 = NULL,*stmt_2 = NULL;
    char sql[200];
    char sql_1[200],sql_2[200];

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
    /*sqlite3*/
    int rc;
    rc = sqlite3_open("dev_info.db", &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    } 

    /*获取用户账户信息*/
    /*获取用户账户信息*/
    char* account = NULL;
    sprintf(sql,"select ACCOUNT from account_info where CLIENT_FD = %03d order by ID desc limit 1;",clientFd);
    fprintf(stdout, "%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
        account =  sqlite3_column_text(stmt, 0);
    }
    if(account == NULL){
        fprintf(stderr, "user account do not exist\n");    
        return M1_PROTOCOL_FAILED;
    }else{
        fprintf(stdout,"clientFd:%03d,account:%s\n",clientFd, account);
    }

    /*取区域名称*/
    char* dist_name = NULL, *ap_id = NULL, *ap_name = NULL;
    int pId;
    sprintf(sql,"select distinct DIS_NAME from district_table where ACCOUNT = \"%s\";",account);
   	fprintf(stdout,"sql:%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
    	dist_name = sqlite3_column_text(stmt,0);
	    devDataObject = cJSON_CreateObject();
	    if(NULL == devDataObject)
	    {
	        cJSON_Delete(devDataObject);
	        return M1_PROTOCOL_FAILED;
	    }
	    cJSON_AddItemToArray(devDataJsonArray, devDataObject);
	    cJSON_AddStringToObject(devDataObject, "district", dist_name);

	    /*创建ap_info数组*/
	    apInfoArrayObject = cJSON_CreateArray();
	    if(NULL == apInfoArrayObject)
	    {
	        cJSON_Delete(apInfoArrayObject);
	        return M1_PROTOCOL_FAILED;
	    }
	    cJSON_AddItemToObject(devDataObject, "apInfo", apInfoArrayObject);
	    sprintf(sql_1,"select AP_ID from district_table where DIS_NAME = \"%s\" ;",dist_name);
	    fprintf(stdout,"sql_1:%s\n", sql_1);
	    sqlite3_reset(stmt_1);
	    sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
	    while(thread_sqlite3_step(&stmt_1, db) == SQLITE_ROW){
	    	ap_id = sqlite3_column_text(stmt_1,0);
	    	fprintf(stdout,"ap_id:%s\n",ap_id);
		    apInfoObject = cJSON_CreateObject();
		    if(NULL == apInfoObject)
		    {
		        cJSON_Delete(apInfoObject);
		        return M1_PROTOCOL_FAILED;
		    }
		    cJSON_AddItemToArray(apInfoArrayObject, apInfoObject);
		    cJSON_AddStringToObject(apInfoObject, "apId", ap_id);
		    /*取出apName*/
		    sprintf(sql_2,"select DEV_NAME,pId from all_dev where DEV_ID = \"%s\" ;",ap_id);
		    fprintf(stdout,"sql_2:%s\n", sql_2);
		    sqlite3_reset(stmt_2);
		    sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
		    rc = thread_sqlite3_step(&stmt_2, db); 
			fprintf(stdout,"step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
			if(rc == SQLITE_ROW){
				ap_name = sqlite3_column_text(stmt_2,0);
                pId = sqlite3_column_int(stmt_2,1);
				fprintf(stdout,"ap_name:%s\n, pId:%05d\n",ap_name, pId);
				cJSON_AddStringToObject(apInfoObject, "apName", ap_name);
                cJSON_AddNumberToObject(apInfoObject, "pId", pId);
			}
	    }
    }

    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
	sqlite3_finalize(stmt_2);
    sqlite3_close(db);

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    fprintf(stdout,"string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;

}




