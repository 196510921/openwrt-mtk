#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "m1_protocol.h"
#include "socket_server.h"
#include "buf_manage.h"
#include "m1_common_log.h"

extern fifo_t dev_data_fifo;
extern fifo_t link_exec_fifo;
void trigger_cb(void* udp, int type, char const* db_name, char const* table_name, sqlite3_int64 rowid);

static int device_exec(char* data, sqlite3* db)
{
	int rc,ret = M1_PROTOCOL_OK;
    int clientFd;
    int pduType = TYPE_DEV_WRITE;
	int type,value,delay;
	char * p = NULL;
	char* sql = (char*)malloc(300);
	char* sql_1 = (char*)malloc(300);
	char* sql_2 = (char*)malloc(300);
	char* sql_3 = (char*)malloc(300);
	char*ap_id = NULL,*dev_id = NULL;

	cJSON * pJsonRoot = NULL; 
    cJSON * pduJsonObject = NULL;
    cJSON * devDataJsonArray = NULL;
    cJSON * devDataObject= NULL;
    cJSON * paramArray = NULL;
    cJSON*  paramObject = NULL;
    cJSON* dup_data = NULL;
	sqlite3_stmt* stmt = NULL,*stmt_1 = NULL,*stmt_2 = NULL,*stmt_3 = NULL;
 
 	M1_LOG_DEBUG("device_exec\n");

    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_DEBUG("pJsonRoot NULL\n");
        cJSON_Delete(pJsonRoot);
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
    /*获取AP信息*/
	sprintf(sql,"select distinct AP_ID from link_exec_table where LINK_NAME = \"%s\";", data);	
	M1_LOG_DEBUG("sql:%s\n",sql);
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
		ap_id = sqlite3_column_text(stmt,0);
		//sqlite3_reset(stmt_1);
		sqlite3_finalize(stmt_1);
		/*获取设备信息*/
		sprintf(sql_1,"select distinct DEV_ID from link_exec_table where LINK_NAME = \"%s\" and AP_ID = \"%s\";",data, ap_id);	
		sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
		M1_LOG_DEBUG("sql_1:%s\n",sql_1);
		while(thread_sqlite3_step(&stmt_1, db) == SQLITE_ROW){
			dev_id = sqlite3_column_text(stmt_1,0);
			/*create device data object*/
			devDataObject = cJSON_CreateObject();
	        if(NULL == devDataObject)
	        {
	            // create object faild, exit
	            M1_LOG_ERROR("devDataObject NULL\n");
	            cJSON_Delete(devDataObject);
	            ret = M1_PROTOCOL_FAILED;
        		goto Finish;
	        }
	        cJSON_AddItemToArray(devDataJsonArray, devDataObject);
	       	cJSON_AddStringToObject(devDataObject,"devId",dev_id);
	      	/*create param array*/
		    paramArray = cJSON_CreateArray();
		    if(NULL == paramArray)
		    {
		    	M1_LOG_ERROR("paramArray NULL\n");
		        cJSON_Delete(paramArray);
		        ret = M1_PROTOCOL_FAILED;
        		goto Finish;
		    }
			cJSON_AddItemToObject(devDataObject, "param", paramArray);
			/*获取参数信息*/
			sprintf(sql_2,"select TYPE,VALUE,DELAY from link_exec_table where LINK_NAME = \"%s\" and DEV_ID = \"%s\";",data, dev_id);	
			//sqlite3_reset(stmt_2);
			sqlite3_finalize(stmt_2);
			sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
			M1_LOG_DEBUG("sql_2:%s\n",sql_2);
			while(thread_sqlite3_step(&stmt_2, db) == SQLITE_ROW){
				type = sqlite3_column_int(stmt_2,0);
				value = sqlite3_column_int(stmt_2,1);
				delay = sqlite3_column_int(stmt_2,2);	
				M1_LOG_DEBUG("type:%d,value:%d,delay:%d\n", type, value, delay);
			    paramObject = cJSON_CreateObject();
		        if(NULL == paramObject)
		        {
		            // create object faild, exit
		            M1_LOG_ERROR("devDataObject NULL\n");
		            cJSON_Delete(paramObject);
		            ret = M1_PROTOCOL_FAILED;
        			goto Finish;
		        }
		        cJSON_AddItemToArray(paramArray, paramObject);
		        cJSON_AddNumberToObject(paramObject, "type", type);
		        cJSON_AddNumberToObject(paramObject, "value", value);

			}

			dup_data = cJSON_Duplicate(pJsonRoot, 1);
			if(NULL == dup_data)
	    	{    
	    		M1_LOG_ERROR("dup_data NULL\n");
	        	cJSON_Delete(dup_data);
	        	ret = M1_PROTOCOL_FAILED;
	        	goto Finish;
	    	}
			p = cJSON_PrintUnformatted(dup_data);
			if(NULL == p)
	    	{    
	    		M1_LOG_ERROR("p NULL\n");
	        	cJSON_Delete(pJsonRoot);
	        	ret = M1_PROTOCOL_FAILED;
	        	goto Finish;
	    	}
	    	/*get clientfd*/
	    	sprintf(sql_3,"select CLIENT_FD from conn_info where AP_ID = \"%s\";",ap_id);
	    	M1_LOG_DEBUG("sql_3:%s\n", sql_3);
	    	//sqlite3_reset(stmt_3);
	    	sqlite3_finalize(stmt_3);
	    	sqlite3_prepare_v2(db, sql_3, strlen(sql_3), &stmt_3, NULL);
	    	rc = thread_sqlite3_step(&stmt_3,db);
	    
	    	if(rc == SQLITE_ROW){
				clientFd = sqlite3_column_int(stmt_3,0);
			}		
	    	
	    	M1_LOG_DEBUG("string:%s\n",p);
	    	delay_send(dup_data, delay, clientFd);
		}
#if 0	
	    p = cJSON_PrintUnformatted(pJsonRoot);
    	if(NULL == p)
    	{    
    		M1_LOG_ERROR("p NULL\n");
        	cJSON_Delete(pJsonRoot);
        	ret = M1_PROTOCOL_FAILED;
        	goto Finish;
    	}
    	/*get clientfd*/
    	sprintf(sql_3,"select CLIENT_FD from conn_info where AP_ID = \"%s\";",ap_id);
    	M1_LOG_DEBUG("sql_3:%s\n", sql_3);
    	//sqlite3_reset(stmt_3);
    	sqlite3_finalize(stmt_3);
    	sqlite3_prepare_v2(db, sql_3, strlen(sql_3), &stmt_3, NULL);
    	rc = thread_sqlite3_step(&stmt_3,db);
    
    	if(rc == SQLITE_ROW){
			clientFd = sqlite3_column_int(stmt_3,0);
		}		
    	
    	M1_LOG_DEBUG("string:%s\n",p);
    	socketSeverSend((uint8*)p, strlen(p), clientFd);
#endif    	
	}

	Finish:
	free(sql);
	free(sql_1);
	free(sql_2);
	free(sql_3);
	sqlite3_finalize(stmt);
	sqlite3_finalize(stmt_1);
   	sqlite3_finalize(stmt_2);
   	sqlite3_finalize(stmt_3);
	cJSON_Delete(pJsonRoot);
	
	return ret;
}

int linkage_msg_handle(payload_t data)
{
	M1_LOG_DEBUG("linkage_msg_handle\n");

	int i,j;
	int delay;
	int id, id_1;
	int rc, ret = M1_PROTOCOL_OK;
	int number1, number2;
	int row_number = 0;
	char* time = (char*)malloc(30);
	char* sql_1 = (char*)malloc(300);
	char* errorMsg = NULL;
	cJSON* linkNameJson = NULL;
	cJSON* districtJson = NULL;
	cJSON* logicalJson = NULL;
	cJSON* execTypeJson = NULL;
	cJSON* triggerJson = NULL;
	cJSON* triggerArrayJson = NULL;
	cJSON* executeJson = NULL;
	cJSON* executeArrayJson = NULL;
	cJSON* scenNameJson = NULL;
	cJSON* apIdJson = NULL;
	cJSON* devIdJson = NULL;
	cJSON* paramJson = NULL;
	cJSON* paramArrayJson = NULL;
	cJSON* typeJson = NULL;
	cJSON* conditionJson = NULL;
	cJSON* valueJson = NULL;
	cJSON* delayJson = NULL;
	cJSON* delayArrayJson = NULL;
	sqlite3* db = NULL;
	sqlite3_stmt* stmt = NULL, *stmt_1 = NULL;

	if(data.pdu == NULL){
		ret = M1_PROTOCOL_FAILED;	
		goto Finish;
	} 
	getNowTime(time);

	/*获取收到数据包信息*/
    linkNameJson = cJSON_GetObjectItem(data.pdu, "linkName");
    districtJson = cJSON_GetObjectItem(data.pdu, "district");
    logicalJson = cJSON_GetObjectItem(data.pdu, "logical");
    execTypeJson = cJSON_GetObjectItem(data.pdu, "execType");
    triggerJson = cJSON_GetObjectItem(data.pdu, "trigger");

    if(strcmp( execTypeJson->valuestring, "scenario") == 0){
    	executeJson = cJSON_GetObjectItem(data.pdu, "exeScen");
    	executeArrayJson = cJSON_GetArrayItem(executeJson, 0);
    	scenNameJson = cJSON_GetObjectItem(executeArrayJson, "scenName");
	}else{
		executeJson = cJSON_GetObjectItem(data.pdu, "exeDev");
    	executeArrayJson = cJSON_GetArrayItem(executeJson, 0);
		scenNameJson = cJSON_GetObjectItem(executeArrayJson, "devId");
	}
    M1_LOG_DEBUG("linkName:%s, district:%s, logical:%s, execType:%s, scenName:%s\n",linkNameJson->valuestring,
    	districtJson->valuestring, logicalJson->valuestring, execTypeJson->valuestring, scenNameJson->valuestring);
    /*获取sqlite3*/
    db = data.db;
    if(sqlite3_exec(db, "BEGIN", NULL, NULL, &errorMsg)==SQLITE_OK){
        M1_LOG_DEBUG("BEGIN\n");
	   	/*检查联动是否存在*/
		sprintf(sql_1,"select ID from linkage_table where LINK_NAME = \"%s\" order by ID desc limit 1;",linkNameJson->valuestring);	
		row_number = sql_row_number(db, sql_1);
		M1_LOG_DEBUG("row_number:%d\n",row_number);
		if(row_number > 0){
			/*delete linkage_table member*/
			sprintf(sql_1,"delete from linkage_table where LINK_NAME = \"%s\";",linkNameJson->valuestring);
			//sqlite3_reset(stmt);
			sqlite3_finalize(stmt);
			sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
			while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW);
			/*delete link_trigger_table member*/
			sprintf(sql_1,"delete from link_trigger_table where LINK_NAME = \"%s\";",linkNameJson->valuestring);
			//sqlite3_reset(stmt);
			sqlite3_finalize(stmt);
			sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
			while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW);
			/*delete link_exec_table member*/
			sprintf(sql_1,"delete from link_exec_table where LINK_NAME = \"%s\";",linkNameJson->valuestring);
			//sqlite3_reset(stmt);
			sqlite3_finalize(stmt);
			sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
			while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW);
		}

		/*获取table id*/
		char* sql = "select ID from linkage_table order by ID desc limit 1";
	    /*linkage_table*/
	    id = sql_id(db, sql);
	    sql = "insert into linkage_table(ID, LINK_NAME, DISTRICT, EXEC_TYPE, EXEC_ID, STATUS, ENABLE,TIME) values(?,?,?,?,?,?,?,?);";
	    M1_LOG_DEBUG("sql:%s\n",sql);
	    //sqlite3_reset(stmt);
	    sqlite3_finalize(stmt);
	    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
		sqlite3_bind_int(stmt, 1, id);
		id++;
		sqlite3_bind_text(stmt, 2,  linkNameJson->valuestring, -1, NULL);
		sqlite3_bind_text(stmt, 3,  districtJson->valuestring, -1, NULL);
		sqlite3_bind_text(stmt, 4,  execTypeJson->valuestring, -1, NULL);
		sqlite3_bind_text(stmt, 5,  scenNameJson->valuestring, -1, NULL);

		sqlite3_bind_text(stmt, 6,  "OFF", -1, NULL);
		sqlite3_bind_text(stmt, 7,  "on", -1, NULL);
		sqlite3_bind_text(stmt, 8,  time, -1, NULL);
		rc = thread_sqlite3_step(&stmt, db);

	    /*link_trigger_table*/
	    sql = "select ID from link_trigger_table order by ID desc limit 1";
	    id_1 = sql_id(db, sql);
	    sql = "insert into link_trigger_table(ID, LINK_NAME, DISTRICT, AP_ID, DEV_ID, TYPE, THRESHOLD, CONDITION,LOGICAL,STATUS,TIME) values(?,?,?,?,?,?,?,?,?,?,?);";
	    M1_LOG_DEBUG("sql:%s\n",sql);
	    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt_1, NULL);

	    number1 = cJSON_GetArraySize(triggerJson);
	    for(i = 0; i < number1; i++){
	    	triggerArrayJson = cJSON_GetArrayItem(triggerJson, i);
	    	apIdJson = cJSON_GetObjectItem(triggerArrayJson, "apId");
	    	devIdJson = cJSON_GetObjectItem(triggerArrayJson, "devId");
	    	paramJson = cJSON_GetObjectItem(triggerArrayJson, "param");
	    	number2 = cJSON_GetArraySize(paramJson);
	    	for(j = 0; j < number2; j++){	
	    		paramArrayJson = cJSON_GetArrayItem(paramJson, j);
	    		typeJson = cJSON_GetObjectItem(paramArrayJson, "type");
	    		conditionJson = cJSON_GetObjectItem(paramArrayJson, "condition");
	    		valueJson = cJSON_GetObjectItem(paramArrayJson, "value");
	    		M1_LOG_DEBUG("type:%d, condition:%s, value:%d\n",typeJson->valueint,conditionJson->valuestring,valueJson->valueint);
	    	
		    	sqlite3_reset(stmt_1);
			   	sqlite3_bind_int(stmt_1, 1, id_1);
			   	id_1++;
			   	sqlite3_bind_text(stmt_1, 2,  linkNameJson->valuestring, -1, NULL);
			   	sqlite3_bind_text(stmt_1, 3,  districtJson->valuestring, -1, NULL);
			   	sqlite3_bind_text(stmt_1, 4,  apIdJson->valuestring, -1, NULL);
			   	sqlite3_bind_text(stmt_1, 5,  devIdJson->valuestring, -1, NULL);
			   	sqlite3_bind_int(stmt_1, 6,  typeJson->valueint);
			   	sqlite3_bind_int(stmt_1, 7,  valueJson->valueint);
			   	sqlite3_bind_text(stmt_1, 8,  conditionJson->valuestring, -1, NULL);
			   	sqlite3_bind_text(stmt_1, 9,  logicalJson->valuestring, -1, NULL);
			   	sqlite3_bind_text(stmt_1, 10,  "OFF", -1, NULL);
			   	sqlite3_bind_text(stmt_1, 11,  time, -1, NULL);
			   	rc = thread_sqlite3_step(&stmt_1, db);
			   	M1_LOG_DEBUG("step2() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");
		    	
		    }

	    }
	    /*联动执行*/
	    if(strcmp( execTypeJson->valuestring, "scenario") != 0){
		    sql = "select ID from link_exec_table order by ID desc limit 1";
		    id_1 = sql_id(db, sql);
		    M1_LOG_DEBUG("id_1:%05d\n",id_1);
		    sql = "insert into link_exec_table(ID, LINK_NAME, DISTRICT, AP_ID, DEV_ID, TYPE, VALUE, DELAY, TIME) values(?,?,?,?,?,?,?,?,?);";
		    M1_LOG_DEBUG("sql:%s\n",sql);
		    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt_1, NULL);
		    number1 = cJSON_GetArraySize(executeJson);
		    for(i = 0; i < number1; i++){
		    	executeArrayJson = cJSON_GetArrayItem(executeJson, i);
		    	apIdJson = cJSON_GetObjectItem(executeArrayJson, "apId");
		    	devIdJson = cJSON_GetObjectItem(executeArrayJson, "devId");
		    	paramJson = cJSON_GetObjectItem(executeArrayJson, "param");
		    	delayArrayJson = cJSON_GetObjectItem(executeArrayJson, "delay");
		    	number2 = cJSON_GetArraySize(delayArrayJson);
		    	for(j = 0, delay = 0; j < number2; j++){
		    		delayJson = cJSON_GetArrayItem(delayArrayJson, j);
		    		delay += delayJson->valueint;
		    	}
		    	number2 = cJSON_GetArraySize(paramJson);
		    	for(j = 0; j < number2; j++){
		    		paramArrayJson = cJSON_GetArrayItem(paramJson, j);
		    		typeJson = cJSON_GetObjectItem(paramArrayJson, "type");
		    		valueJson = cJSON_GetObjectItem(paramArrayJson, "value");
		    		sqlite3_reset(stmt_1);
				   	sqlite3_bind_int(stmt_1, 1, id_1);
				   	id_1++;
				   	sqlite3_bind_text(stmt_1, 2,  linkNameJson->valuestring, -1, NULL);
				   	sqlite3_bind_text(stmt_1, 3,  districtJson->valuestring, -1, NULL);
				   	sqlite3_bind_text(stmt_1, 4,  apIdJson->valuestring, -1, NULL);
				   	sqlite3_bind_text(stmt_1, 5,  devIdJson->valuestring, -1, NULL);
				   	sqlite3_bind_int(stmt_1, 6,  typeJson->valueint);
				   	sqlite3_bind_int(stmt_1, 7,  valueJson->valueint);
				   	//sqlite3_bind_int(stmt_1, 8,  delayJson->valueint);
				   	sqlite3_bind_int(stmt_1, 8,  delay);
				   	sqlite3_bind_text(stmt_1, 9,  time, -1, NULL);
				   	rc = thread_sqlite3_step(&stmt_1, db);
		    	}
		    }
		}
		if(sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg) == SQLITE_OK){
            M1_LOG_DEBUG("END\n");
        }else{
            M1_LOG_DEBUG("ROLLBACK\n");
            if(sqlite3_exec(db, "ROLLBACK", NULL, NULL, &errorMsg) == SQLITE_OK){
                M1_LOG_DEBUG("ROLLBACK OK\n");
                sqlite3_free(errorMsg);
            }else{
                M1_LOG_ERROR("ROLLBACK FALIED\n");
            }
        }
    }else{
        M1_LOG_ERROR("errorMsg:");
    }    
    sqlite3_free(errorMsg);

    Finish:
    free(time);
    free(sql_1);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);

    return ret;
}

void linkage_task(void)
{
	int rc, rc1;
	uint32_t rowid;
	char* sql = NULL;
	sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
	char *exec_type = NULL,*exec_id = NULL, *link_name =  NULL;
    //while(1){
    rc1 = fifo_read(&link_exec_fifo, &rowid);
    if(rc1 > 0){
	    sql = (char*)malloc(300);
		rc = sqlite3_open("dev_info.db", &db);  
		if(rc){  
		    M1_LOG_ERROR( "Can't open database: %s\n", sqlite3_errmsg(db));  
		    goto Finish;
		}else{  
		    M1_LOG_DEBUG( "Opened database successfully\n");  
		}

	   	do{
	   		if(rc1 > 0){
	   			M1_LOG_DEBUG("linkage_task\n");
		    	sprintf(sql,"select EXEC_TYPE, EXEC_ID, LINK_NAME from linkage_table where rowid = %05d and ENABLE = \"on\";",rowid);
				M1_LOG_DEBUG("sql:%s\n", sql);
				//sqlite3_reset(stmt);
				sqlite3_finalize(stmt);
				sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
				rc = thread_sqlite3_step(&stmt, db);
				if(rc == SQLITE_ROW){
					exec_type = sqlite3_column_text(stmt,0);
					exec_id = sqlite3_column_text(stmt,1);
					link_name = sqlite3_column_text(stmt,2);
					if(strcmp(exec_type, "scenario") == 0){
						scenario_exec(exec_id, db);
					}else{
						device_exec(link_name, db);			
					}
				}
	   			rc1 = fifo_read(&link_exec_fifo, &rowid);
	   		}
	   	}while(rc1 > 0);

		Finish:
		free(sql);
		sqlite3_finalize(stmt);
		sqlite3_close(db);
	}

}

static char* linkage_status(char* condition, int threshold, int value)
{	
	char* status = "OFF";

	if (0 == strcmp(condition, "<=")){ 
		if(value <= threshold)
			status = "ON";
	}else if(strcmp(condition, "=") == 0){
		if(value == threshold)
			status = "ON";
	}else{
		if(value >= threshold)
			status = "ON";
	}
	M1_LOG_DEBUG("linkage_status:%s, value:%d, threshold:%d\n",status,value, threshold);

	return status;
}

static void linkage_check(sqlite3* db, char* link_name)
{
	M1_LOG_DEBUG("linkage_check\n");
	sqlite3_stmt* stmt = NULL;
	int link_flag = 1, rc, rowid;
	char *status, *logical;
	char sql[200];

	sprintf(sql,"select STATUS, LOGICAL from link_trigger_table where LINK_NAME = \"%s\";",link_name);
	M1_LOG_DEBUG("sql:%s\n",sql);
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	while(thread_sqlite3_step(&stmt,db) == SQLITE_ROW){
		status = sqlite3_column_text(stmt,0);
		logical = sqlite3_column_text(stmt,1);
		if((strcmp(logical, "&") == 0)){
			if((strcmp(status, "OFF") == 0)){
				link_flag = 0;
				break;
			}
		}else{
			if((strcmp(status, "ON") == 0)){
				link_flag = 1;
				break;
			}
		}
	}
	M1_LOG_DEBUG("link_flag:%d\n",link_flag);
	if(link_flag){
		/*执行设备*/
		sprintf(sql,"update linkage_table set STATUS = \"ON\" where LINK_NAME = \"%s\";", link_name);
		//sqlite3_reset(stmt);
		sqlite3_finalize(stmt);
		sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
		rc = thread_sqlite3_step(&stmt, db);
		
		sprintf(sql,"select rowid from linkage_table where LINK_NAME = \"%s\";", link_name);
		sqlite3_finalize(stmt);
		sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
		rc = thread_sqlite3_step(&stmt, db);
		if(rc == SQLITE_ROW){
			rowid = sqlite3_column_int(stmt,0);
			M1_LOG_DEBUG("rowid:%d\n",rowid);
			fifo_write(&link_exec_fifo, rowid);
		}
	}

	sqlite3_finalize(stmt);

}

int trigger_cb_handle(sqlite3* db)
{
	int rc,rc1;
	int value, param_type, threshold;
	uint32_t rowid;
	char* devId = NULL, *condition = NULL, *status = NULL, *link_name = NULL;
	char* sql = NULL, *sql_1 = NULL, *sql_2 = NULL;
	sqlite3_stmt* stmt = NULL,*stmt_1 = NULL, *stmt_2 = NULL;

   	rc1 = fifo_read(&dev_data_fifo, &rowid);
   	if(rc1 > 0){
   	M1_LOG_DEBUG("trigger_cb_handle\n");
   	sql = (char*)malloc(300);
   	sql_1 = (char*)malloc(300);
   	sql_2 = (char*)malloc(300);

   	/*失使能表更新回调*/
   	rc = sqlite3_update_hook(db, NULL, NULL);
    if(rc){
        M1_LOG_DEBUG( "sqlite3_update_hook falied: %s\n", sqlite3_errmsg(db));  
    }

    do{
    	/*check linkage table*/
		sprintf(sql,"select VALUE, DEV_ID, TYPE from param_table where rowid = %05d;",rowid);
		M1_LOG_DEBUG("sql:%s\n",sql);
		do{
			sqlite3_finalize(stmt);
			rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
			if(rc != SQLITE_OK){
				M1_LOG_ERROR( "sqlite3_prepare_v2 failed,errorNum:%05d\n",rc);  
				if(rc == SQLITE_LOCKED){
					M1_LOG_ERROR( "sqlite3 is locked\n");  
					usleep(100000);
				}else{
					goto Finish;
				}
			}else{
				M1_LOG_DEBUG("sqlite3_prepare_v2 ok\n");  		
			}
		}while(rc < 0);

		rc = thread_sqlite3_step(&stmt, db);
		if(rc == SQLITE_ROW){
		    value = sqlite3_column_int(stmt,0);
		    devId = sqlite3_column_text(stmt,1);
		    param_type = sqlite3_column_int(stmt,2);
		    M1_LOG_DEBUG("value:%05d, devId:%s, param_type%05d:\n",value, devId, param_type);
		}
	 	/*检查设备启/停状态*/
	 	sprintf(sql_1,"select STATUS from all_dev where DEV_ID = \"%s\";",devId);
	 	//sqlite3_reset(stmt_1);
	 	sqlite3_finalize(stmt_1);
	 	sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
	 	rc = thread_sqlite3_step(&stmt_1, db);
		if(rc == SQLITE_ROW){		
	    	status = sqlite3_column_text(stmt_1,0);
		    /*设备处于启动状态*/
		    if(strcmp(status,"ON") == 0){
			    /*get linkage table*/
			    sprintf(sql_1,"select THRESHOLD,CONDITION,LINK_NAME from link_trigger_table where DEV_ID = \"%s\" and TYPE = %05d;",devId,param_type);
				M1_LOG_DEBUG("sql_1:%s\n",sql_1);
				//sqlite3_reset(stmt_1);
				sqlite3_finalize(stmt_1);
				sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
				while(thread_sqlite3_step(&stmt_1, db) == SQLITE_ROW){
					threshold = sqlite3_column_int(stmt_1,0);
			        condition = sqlite3_column_text(stmt_1,1);
			        link_name = sqlite3_column_text(stmt_1,2);
			        M1_LOG_DEBUG("threshold:%05d, condition:%s\n, link_name:%s\n", threshold, condition, link_name);
			 		status = linkage_status(condition, threshold, value);
			 		/*set linkage table*/
			 		sprintf(sql_2,"update link_trigger_table set STATUS = \"%s\" where DEV_ID = \"%s\" and TYPE = %05d and LINK_NAME = \"%s\" ;",status,devId,param_type,link_name);
			 		M1_LOG_DEBUG("sql_2:%s\n",sql_2);
			 		//sqlite3_reset(stmt_2);
			 		sqlite3_finalize(stmt_2);
					sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
					rc = thread_sqlite3_step(&stmt_2, db);
					/*检查是否满足触发条件*/
					linkage_check(db, link_name);
			 	}
		 	}
	 	}	
    	rc1 = fifo_read(&dev_data_fifo, &rowid);
    }while(rc1 > 0);

	    Finish:
	    free(sql);
	    free(sql_1);
	    free(sql_2);
	    sqlite3_finalize(stmt);
	 	sqlite3_finalize(stmt_1);
	 	sqlite3_finalize(stmt_2);
   }
 
}

void trigger_cb(void* udp, int type, char const* db_name, char const* table_name, sqlite3_int64 rowid)
{
	int rc;

	M1_LOG_DEBUG("trigger_cb\n");
	rc = strcmp(udp, "AP_report_data_handle");
	if((0 == rc) && rowid > 0)
	{
		fifo_write(&dev_data_fifo, rowid);
	}
	
}

int app_req_linkage(payload_t data)
{
	M1_LOG_DEBUG("app_req_linkage\n");
	/*数据包类型*/
	int rc,ret = M1_PROTOCOL_OK;
	int type, value, delay, pId;
	int pduType = TYPE_M1_REPORT_LINK_INFO;
	char* sql = NULL;
    char* sql_1 = (char*)malloc(300);
    char* sql_2 = (char*)malloc(300);
    char* link_name = NULL, *district = NULL, *logical = NULL;
    char *exec_type = NULL, *exec_id = NULL,*enable = NULL;
    char *ap_id = NULL, *dev_id = NULL, *condition = NULL, *dev_name = NULL;
	cJSON * pJsonRoot = NULL;
    cJSON * pduJsonObject = NULL;
    cJSON * devDataJsonArray = NULL;
    cJSON*  devDataObject= NULL;
    cJSON * triggerJsonArray = NULL;
    cJSON*  triggerObject= NULL;
    cJSON * execJsonArray = NULL;
    cJSON*  execObject= NULL;
    cJSON * paramJsonArray = NULL;
    cJSON*  paramObject= NULL;
    cJSON* delayArrayObject = NULL;
    cJSON* delayObject = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL, *stmt_1 = NULL,*stmt_2 = NULL;

    db = data.db;
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_ERROR("pJsonRoot NULL\n");
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
        ret =  M1_PROTOCOL_FAILED;
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
        ret =  M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataJsonArray);

    sql = "select LINK_NAME, DISTRICT, EXEC_TYPE, EXEC_ID, ENABLE from linkage_table;";
    //sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
    	devDataObject = cJSON_CreateObject();
	    if(NULL == devDataObject)
	    {
	        cJSON_Delete(devDataObject);
	        ret =  M1_PROTOCOL_FAILED;
        	goto Finish;
	    }
	    cJSON_AddItemToArray(devDataJsonArray, devDataObject);
	    link_name = sqlite3_column_text(stmt, 0);
	    cJSON_AddStringToObject(devDataObject, "linkName", link_name);
	    district = sqlite3_column_text(stmt, 1);
	    cJSON_AddStringToObject(devDataObject, "district", district);
	    exec_type = sqlite3_column_text(stmt, 2);
	    cJSON_AddStringToObject(devDataObject, "execType", exec_type);
	    exec_id = sqlite3_column_text(stmt, 3);
	    /*联动使能状态*/
	    enable = sqlite3_column_text(stmt, 4);
	    cJSON_AddStringToObject(devDataObject, "enable", enable);
	    /*获取logical*/
	    sprintf(sql_1,"select LOGICAL from link_trigger_table where LINK_NAME = \"%s\" limit 1;", link_name);
	    M1_LOG_DEBUG("sql_1:%s\n", sql_1);
    	//sqlite3_reset(stmt_1);
    	sqlite3_finalize(stmt_1);
    	sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
    	rc = thread_sqlite3_step(&stmt_1, db); 
		if(rc == SQLITE_ROW){
			logical = sqlite3_column_text(stmt_1, 0);
	    	cJSON_AddStringToObject(devDataObject, "logical", logical);
		}
	    /*获取联动触发信息*/
 	    triggerJsonArray = cJSON_CreateArray();
 	    if(NULL == triggerJsonArray)
 	    {
 	        cJSON_Delete(triggerJsonArray);
 	        ret =  M1_PROTOCOL_FAILED;
        	goto Finish;
 	    }
 	    cJSON_AddItemToObject(devDataObject, "trigger", triggerJsonArray);
 	    sprintf(sql_1,"select DISTINCT DEV_ID from link_trigger_table where LINK_NAME = \"%s\";", link_name);
 	    M1_LOG_DEBUG("sql_1:%s\n", sql_1);
    	//sqlite3_reset(stmt_1);
    	sqlite3_finalize(stmt_1);
    	sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
    	while(thread_sqlite3_step(&stmt_1, db) == SQLITE_ROW){
    		triggerObject = cJSON_CreateObject();
		    if(NULL == triggerObject)
		    {
		        cJSON_Delete(triggerObject);
		        ret =  M1_PROTOCOL_FAILED;
        		goto Finish;
		    }
		    cJSON_AddItemToArray(triggerJsonArray, triggerObject);
    		dev_id = sqlite3_column_text(stmt_1, 0);
    		cJSON_AddStringToObject(triggerObject, "devId", dev_id);
    		/*获取AP_ID*/
			sprintf(sql_2,"select AP_ID from link_trigger_table where LINK_NAME = \"%s\" and DEV_ID = \"%s\" limit 1;",link_name, dev_id);		   	
		   	M1_LOG_DEBUG("sql_2:%s\n", sql_2);
		   	//sqlite3_reset(stmt_2);
		   	sqlite3_finalize(stmt_2);
	    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
	    	rc = thread_sqlite3_step(&stmt_2, db); 
			if(rc == SQLITE_ROW){
			   	ap_id = sqlite3_column_text(stmt_2, 0);
			   	cJSON_AddStringToObject(triggerObject, "apId", ap_id);
			   	M1_LOG_DEBUG("apId:%s\n",ap_id);
		   	}
		   	/*获取设备名称*/
		   	sprintf(sql_2,"select DEV_NAME, PID from all_dev where DEV_ID = \"%s\" limit 1;",dev_id);		   	
		   	//sqlite3_reset(stmt_2);
		   	sqlite3_finalize(stmt_2);
	    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
	    	rc = thread_sqlite3_step(&stmt_2, db); 
			if(rc == SQLITE_ROW){ 
			   	dev_name = sqlite3_column_text(stmt_2, 0);
			   	pId = sqlite3_column_int(stmt_2, 1);
			   	cJSON_AddStringToObject(triggerObject, "devName", dev_name);
			   	cJSON_AddNumberToObject(triggerObject, "pId", pId);
		   	}
    		/*获取参数*/
    		paramJsonArray = cJSON_CreateArray();
 	    	if(NULL == paramJsonArray)
 	    	{
 	        	cJSON_Delete(paramJsonArray);
 	        	ret =  M1_PROTOCOL_FAILED;
        		goto Finish;
 	    	}
 	    	cJSON_AddItemToObject(triggerObject, "param", paramJsonArray);

 	    	sprintf(sql_2,"select TYPE, THRESHOLD, CONDITION from link_trigger_table where LINK_NAME = \"%s\" and DEV_ID = \"%s\";",link_name, dev_id);
	    	M1_LOG_DEBUG("sql_2:%s\n", sql_2);
	    	//sqlite3_reset(stmt_2);
	    	sqlite3_finalize(stmt_2);
	    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
		   	while(thread_sqlite3_step(&stmt_2, db) == SQLITE_ROW){
			    paramObject = cJSON_CreateObject();
			    if(NULL == paramObject)
			    {
			        cJSON_Delete(paramObject);
			        ret =  M1_PROTOCOL_FAILED;
        			goto Finish;
			    }
			    cJSON_AddItemToArray(paramJsonArray, paramObject);
				type = sqlite3_column_int(stmt_2, 0);
			   	cJSON_AddNumberToObject(paramObject, "type", type);
			   	M1_LOG_DEBUG("type:%05d\n");
			   	value = sqlite3_column_int(stmt_2, 1);
			   	cJSON_AddNumberToObject(paramObject, "value", value);
			   	M1_LOG_DEBUG("value:%05d\n",value);
			   	condition = sqlite3_column_text(stmt_2, 2);
			   	cJSON_AddStringToObject(paramObject, "condition", condition);
			   	M1_LOG_DEBUG("condition:%s\n",condition);
		   	}
    	}

    	execJsonArray = cJSON_CreateArray();
		if(NULL == execJsonArray)
		{
		    cJSON_Delete(execJsonArray);
		    ret =  M1_PROTOCOL_FAILED;
        	goto Finish;
		}
    	if(strcmp(exec_type,"scenario") != 0){
    		/*获取执行设备信息*/
		    cJSON_AddItemToObject(devDataObject, "exeDev", execJsonArray);

	 	    sprintf(sql_1,"select DISTINCT DEV_ID from link_exec_table where LINK_NAME = \"%s\" order by ID asc;", link_name);
	 	    M1_LOG_DEBUG("sql_1:%s\n", sql_1);
	    	//sqlite3_reset(stmt_1);
	    	sqlite3_finalize(stmt_1);
	    	sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
	    	while(thread_sqlite3_step(&stmt_1, db) == SQLITE_ROW){
	    		execObject = cJSON_CreateObject();
			    if(NULL == execObject)
			    {
			        cJSON_Delete(execObject);
			        ret =  M1_PROTOCOL_FAILED;
        			goto Finish;
			    }
			    cJSON_AddItemToArray(execJsonArray, execObject);
	    		dev_id = sqlite3_column_text(stmt_1, 0);
	    		cJSON_AddStringToObject(execObject, "devId", dev_id);
	    		/*获取AP_ID*/
				sprintf(sql_2,"select AP_ID,DELAY from link_exec_table where LINK_NAME = \"%s\" and DEV_ID = \"%s\" limit 1;",link_name, dev_id);		   	
			   	M1_LOG_DEBUG("sql_2:%s\n", sql_2);
			   	//sqlite3_reset(stmt_2);
			   	sqlite3_finalize(stmt_2);
		    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
		    	rc = thread_sqlite3_step(&stmt_2, db); 
				if(rc == SQLITE_ROW){
					int i;
				   	ap_id = sqlite3_column_text(stmt_2, 0);
				   	cJSON_AddStringToObject(execObject, "apId", ap_id);
				   	M1_LOG_DEBUG("apId:%s\n",ap_id);
				   	delay = sqlite3_column_int(stmt_2, 1);
				   	/*设备延时信息数组*/
				   	delayArrayObject = cJSON_CreateArray();
				   	if(NULL == delayArrayObject)
				    {
				        cJSON_Delete(delayArrayObject);
				        ret =  M1_PROTOCOL_FAILED;
        				goto Finish;
				    }
				    cJSON_AddItemToObject(execObject, "delay", delayArrayObject);
				    M1_LOG_DEBUG("delay:%05d\n",delay);
				    M1_LOG_DEBUG("delay/SCENARIO_DELAY_TOP:%d\n",delay / SCENARIO_DELAY_TOP);
				    M1_LOG_DEBUG("delay \"%\" SCENARIO_DELAY_TOP:%d\n",delay % SCENARIO_DELAY_TOP);
				   	for(i = 0; i < (delay / SCENARIO_DELAY_TOP); i++){
						delayObject = cJSON_CreateNumber(SCENARIO_DELAY_TOP);
					    if(NULL == delayObject)
					    {
					        cJSON_Delete(delayObject);
					        ret =  M1_PROTOCOL_FAILED;
        					goto Finish;
					    }
						cJSON_AddItemToArray(delayArrayObject, delayObject);
				   	}
				   	if((delay % SCENARIO_DELAY_TOP) > 0){
				   		delayObject = cJSON_CreateNumber(delay % SCENARIO_DELAY_TOP);
					    if(NULL == delayObject)
					    {
					        cJSON_Delete(delayObject);
					        ret =  M1_PROTOCOL_FAILED;
        					goto Finish;
					    }
						cJSON_AddItemToArray(delayArrayObject, delayObject);
					}
			   	}
			   	/*获取设备名称*/
			   	sprintf(sql_2,"select DEV_NAME, PID from all_dev where DEV_ID = \"%s\" limit 1;",dev_id);		   	
			   	//sqlite3_reset(stmt_2);
			   	sqlite3_finalize(stmt_2);
		    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
		    	rc = thread_sqlite3_step(&stmt_2,db); 
				if(rc == SQLITE_ROW){
				   	dev_name = sqlite3_column_text(stmt_2, 0);
				   	cJSON_AddStringToObject(execObject, "devName", dev_name);
				   	pId = sqlite3_column_int(stmt_2, 1);
				   	cJSON_AddNumberToObject(execObject, "pId", pId);
				}
	    		/*获取参数*/
	    		paramJsonArray = cJSON_CreateArray();
	 	    	if(NULL == paramJsonArray)
	 	    	{
	 	        	cJSON_Delete(paramJsonArray);
	 	        	ret =  M1_PROTOCOL_FAILED;
        			goto Finish;
	 	    	}
	 	    	cJSON_AddItemToObject(execObject, "param", paramJsonArray);

	 	    	sprintf(sql_2,"select TYPE, VALUE from link_exec_table where LINK_NAME = \"%s\" and DEV_ID = \"%s\";",link_name, dev_id);
		    	M1_LOG_DEBUG("sql_2:%s\n", sql_2);
		    	//sqlite3_reset(stmt_2);
		    	sqlite3_finalize(stmt_2);
		    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
			   	while(thread_sqlite3_step(&stmt_2, db) == SQLITE_ROW){
				    paramObject = cJSON_CreateObject();
				    if(NULL == paramObject)
				    {
				        cJSON_Delete(paramObject);
				        ret =  M1_PROTOCOL_FAILED;
        				goto Finish;
				    }
				    cJSON_AddItemToArray(paramJsonArray, paramObject);
					type = sqlite3_column_int(stmt_2, 0);
				   	cJSON_AddNumberToObject(paramObject, "type", type);
				   	printf("type:%05d\n");
				   	value = sqlite3_column_int(stmt_2, 1);
				   	cJSON_AddNumberToObject(paramObject, "value", value);
				   	M1_LOG_DEBUG("value:%05d\n",value);
			   	}
	    	}
    	}else{
    		/*获取执行设备信息*/
		    cJSON_AddItemToObject(devDataObject, "exeScen", execJsonArray);
    		/*获取场景信息*/
    		execObject = cJSON_CreateObject();
		    if(NULL == execObject)
		    {
		        cJSON_Delete(execObject);
		        ret =  M1_PROTOCOL_FAILED;
        		goto Finish;
		    }
		    cJSON_AddItemToArray(execJsonArray, execObject);
	    	cJSON_AddStringToObject(execObject, "scenName", exec_id);
			
			
    	}
 	}

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        ret =  M1_PROTOCOL_FAILED;
        goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), data.clientFd);
    Finish:
    free(sql_1);
    free(sql_2);
 	sqlite3_finalize(stmt);
 	sqlite3_finalize(stmt_1);
	sqlite3_finalize(stmt_2);
    cJSON_Delete(pJsonRoot);

    return ret;
}

int app_linkage_enable(payload_t data)
{
	M1_LOG_DEBUG( "app_linkage_enable\n");
	int rc,ret = M1_PROTOCOL_OK;
	char* sql = (char*)malloc(300);
	sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
	cJSON* linkObject = NULL;
	cJSON* enableObject = NULL;

    /*获取数据库*/
    db = data.db;
	linkObject = cJSON_GetObjectItem(data.pdu, "linkName");
	if(linkObject == NULL){
		ret = M1_PROTOCOL_FAILED;
		goto Finish;
	}

	M1_LOG_DEBUG("link_name:%s\n",linkObject->valuestring);
	enableObject = cJSON_GetObjectItem(data.pdu, "enable");
	if(enableObject == NULL){
		ret = M1_PROTOCOL_FAILED;
		goto Finish;
	}
	M1_LOG_DEBUG("enable:%s\n",enableObject->valuestring);

	sprintf(sql,"update linkage_table set ENABLE = \"%s\" where LINK_NAME = \"%s\";",enableObject->valuestring,linkObject->valuestring);
	M1_LOG_DEBUG("sql_2:%s\n", sql);
  	//sqlite3_reset(stmt);
  	sqlite3_finalize(stmt);
  	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
  	while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW);

  	Finish:
  	free(sql);
  	sqlite3_finalize(stmt);

    return ret;
}


