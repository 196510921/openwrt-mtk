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

/*函数定义*************************************************************************************/
static int scenario_alarm_check(scen_alarm_t alarm_time);

/*场景执行*/
int scenario_exec(char* data, sqlite3* db)
{
	cJSON * pJsonRoot = NULL; 
    cJSON * pduJsonObject = NULL;
    cJSON * devDataJsonArray = NULL;
    cJSON * devDataObject= NULL;
    cJSON * paramArray = NULL;
    cJSON*  paramObject = NULL;

	char* sql = (char*)malloc(300);
	char* sql_1 = (char*)malloc(300);
	char* sql_2 = (char*)malloc(300);
	char* sql_3 = (char*)malloc(300);
	char*ap_id = NULL,*dev_id = NULL,*status = NULL;
	int type,value,delay;
	sqlite3_stmt* stmt = NULL,*stmt_1 = NULL,*stmt_2 = NULL,*stmt_3 = NULL;
 
 	M1_LOG_DEBUG("scenario_exec\n");
    int rc,ret = M1_PROTOCOL_OK;
    int clientFd;
    int pduType = TYPE_DEV_WRITE;

    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_ERROR("pJsonRoot NULL\n");
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

	sprintf(sql,"select distinct AP_ID from scenario_table where SCEN_NAME = \"%s\";", data);	
	
	M1_LOG_DEBUG("sql:%s\n",sql);
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
		ap_id = sqlite3_column_text(stmt,0);
		//sqlite3_reset(stmt_1);
		sqlite3_finalize(stmt_1);
		sprintf(sql_1,"select distinct DEV_ID from scenario_table where SCEN_NAME = \"%s\" and AP_ID = \"%s\";",data, ap_id);	
		sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
		M1_LOG_DEBUG("sql_1:%s\n",sql_1);
		// /*单个子设备数据*/
		while(thread_sqlite3_step(&stmt_1, db) == SQLITE_ROW){
			dev_id = sqlite3_column_text(stmt_1,0);
			/*检查设备启/停状态*/
		 	sprintf(sql_2,"select STATUS from all_dev where DEV_ID = \"%s\";",dev_id);
		 	printf("sql_2:%s\n",sql_2);
		 	//sqlite3_reset(stmt_2);
		 	sqlite3_finalize(stmt_2);
		 	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
		 	rc = thread_sqlite3_step(&stmt_2,db);
	
		   	if(rc == SQLITE_ROW){
				status = sqlite3_column_text(stmt_2,0);
			}
			printf("status:%s\n",status);		
		    if(strcmp(status,"ON") == 0){
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
		        cJSON_DetachItemFromArray(devDataJsonArray, 0);
		        cJSON_AddItemToArray(devDataJsonArray, devDataObject);
		    	//cJSON_ReplaceItemInArray(devDataJsonArray, 1, devDataObject);
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
			    cJSON_AddItemToObject(devDataObject,"param", paramArray);
				//cJSON_ReplaceItemInObject(devDataObject, "param", paramArray);
				sprintf(sql_2,"select TYPE, VALUE, DELAY from scenario_table where SCEN_NAME = \"%s\" and DEV_ID = \"%s\";",data, dev_id);
				M1_LOG_DEBUG("sql_2:%s\n",sql_2);
				//sqlite3_reset(stmt_2);
				sqlite3_finalize(stmt_2);
				sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
				while(thread_sqlite3_step(&stmt_2,db) == SQLITE_ROW){
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
	        	
	        	cJSON* dup_data = cJSON_Duplicate(pJsonRoot, 1);
	        	if(NULL == dup_data)
		    	{    
		    		M1_LOG_ERROR("dup_data NULL\n");
		        	cJSON_Delete(dup_data);
		        	ret = M1_PROTOCOL_FAILED;
		        	goto Finish;
		    	}
	         	char* p = cJSON_PrintUnformatted(dup_data);
  			 	if(NULL == p)
  			 	{    
  			 		M1_LOG_ERROR("p NULL\n");
  			     	cJSON_Delete(dup_data);
  			     	ret = M1_PROTOCOL_FAILED;
        			goto Finish;
  			 	}
  			 	printf("p:%s\n",p);
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
		    	
		    	M1_LOG_DEBUG("add msg to delay send\n");
		    	delay_send(dup_data, delay, clientFd);
		    }
		}
    	
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

int scenario_create_handle(payload_t data)
{
	M1_LOG_DEBUG("scenario_create_handle\n");
	
	int i,j;
	int delay = 0;
	int row_number = 0;
	int id,id_1;
	int rc, ret = M1_PROTOCOL_OK;
	int number1,number2;
	char* sql = NULL;
	char* time = (char*)malloc(30);
	char* sql_1 = (char*)malloc(300);
	char* sql_2 = (char*)malloc(300);
	char* errorMsg = NULL;
	cJSON* scenNameJson = NULL;
	cJSON* scenPicJson = NULL;
	cJSON* districtJson = NULL;
	cJSON* alarmJson = NULL;
	cJSON* devJson = NULL;
	cJSON* devArrayJson = NULL;
	cJSON* apIdJson = NULL;
	cJSON* devIdJson = NULL;
	cJSON* paramJson = NULL;
	cJSON* paramArrayJson = NULL;
	cJSON* typeJson = NULL;
	cJSON* valueJson = NULL;
	cJSON* delayArrayJson = NULL;
	cJSON* delayJson = NULL;
	cJSON* hourJson = NULL;
	cJSON* minutesJson = NULL;
	cJSON* weekJson = NULL;
	cJSON* statusJson = NULL;
	sqlite3* db = NULL;
	sqlite3_stmt* stmt = NULL;

	if(data.pdu == NULL){
		ret = M1_PROTOCOL_FAILED;
		goto Finish;
	};

	getNowTime(time);
	/*获取数据库*/
	db = data.db;
    /*获取场景名称*/
    scenNameJson = cJSON_GetObjectItem(data.pdu, "scenName");
    if(scenNameJson == NULL){
    	ret = M1_PROTOCOL_FAILED;
		goto Finish;	
    }
    M1_LOG_DEBUG("scenName:%s\n",scenNameJson->valuestring);
	/*获取场景图标*/
	scenPicJson = cJSON_GetObjectItem(data.pdu, "scenPic");
    if(scenPicJson == NULL){
    	ret = M1_PROTOCOL_FAILED;
		goto Finish;	
    }
    M1_LOG_DEBUG("scenPic:%s\n",scenPicJson->valuestring);
	/*获取数据包中的alarm信息*/
	alarmJson = cJSON_GetObjectItem(data.pdu, "alarm");
	if(alarmJson == NULL){
    	ret = M1_PROTOCOL_FAILED;
		goto Finish;	
    }
	/*将alarm信息存入alarm表中*/
	/*获取table id*/
	sql = "select ID from scen_alarm_table order by ID desc limit 1";
	/*获取alarm表中id*/
	id = sql_id(db, sql);
	/*事物开启*/
	if(sqlite3_exec(db, "BEGIN", NULL, NULL, &errorMsg)==SQLITE_OK){
        M1_LOG_DEBUG("BEGIN\n");
	   	/*删除原有表scenario_table中的旧scenario*/
	   	if(scenNameJson != NULL){
			sprintf(sql_1,"select ID from scen_alarm_table where SCEN_NAME = \"%s\";",scenNameJson->valuestring);	
			row_number = sql_row_number(db, sql_1);
			M1_LOG_DEBUG("row_number:%d\n",row_number);
			if(row_number > 0){
				sprintf(sql_1,"delete from scen_alarm_table where SCEN_NAME = \"%s\";",scenNameJson->valuestring);				
				//sqlite3_reset(stmt);
				sqlite3_finalize(stmt);
				sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
				while(thread_sqlite3_step(&stmt,db) == SQLITE_ROW);
			}
		}
		if(alarmJson != NULL){	
			/*获取收到数据包信息*/
		   	hourJson = cJSON_GetObjectItem(alarmJson, "hour");
		    M1_LOG_DEBUG("hour:%d\n",hourJson->valueint);
		    minutesJson = cJSON_GetObjectItem(alarmJson, "minutes");
		    M1_LOG_DEBUG("minutes:%d\n",minutesJson->valueint);
		    weekJson = cJSON_GetObjectItem(alarmJson, "week");
		    M1_LOG_DEBUG("week:%s\n",weekJson->valuestring);
		    statusJson = cJSON_GetObjectItem(alarmJson, "status");
		    M1_LOG_DEBUG("status:%s\n",statusJson->valuestring);
		

		    sql = "insert into scen_alarm_table(ID, SCEN_NAME, HOUR, MINUTES, WEEK, STATUS, TIME) values(?,?,?,?,?,?,?);";
		    M1_LOG_DEBUG("sql:%s\n",sql);
		    //sqlite3_reset(stmt);
		    sqlite3_finalize(stmt);
		    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
			sqlite3_bind_int(stmt, 1, id);
			sqlite3_bind_text(stmt, 2, scenNameJson->valuestring, -1, NULL);
			sqlite3_bind_int(stmt, 3, hourJson->valueint);
			sqlite3_bind_int(stmt, 4, minutesJson->valueint);
			sqlite3_bind_text(stmt, 5, weekJson->valuestring, -1, NULL);
			sqlite3_bind_text(stmt, 6, statusJson->valuestring, -1, NULL);
			sqlite3_bind_text(stmt, 7, time, -1, NULL);
			
			rc = thread_sqlite3_step(&stmt, db); 
		}
		
		/*获取联动表 id*/
		sql = "select ID from scenario_table order by ID desc limit 1";
		/*linkage_table*/
		id = sql_id(db, sql);
	   	/*删除原有表scenario_table中的旧scenario*/
		sprintf(sql_1,"select ID from scenario_table where SCEN_NAME = \"%s\";",scenNameJson->valuestring);	
		row_number = sql_row_number(db, sql_1);
		M1_LOG_DEBUG("row_number:%d\n",row_number);
		if(row_number > 0){
			sprintf(sql_1,"delete from scenario_table where SCEN_NAME = \"%s\";",scenNameJson->valuestring);				
			//sqlite3_reset(stmt);
			sqlite3_finalize(stmt);
			sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
			while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW);
		}

	    districtJson = cJSON_GetObjectItem(data.pdu, "district");
	    M1_LOG_DEBUG("district:%s\n",districtJson->valuestring);
	    devArrayJson = cJSON_GetObjectItem(data.pdu, "device");
	    number1 = cJSON_GetArraySize(devArrayJson);
	    M1_LOG_DEBUG("number1:%d\n",number1);
	    /*存取到数据表scenario_table中*/
	    for(i = 0; i < number1; i++){
	    	devJson = cJSON_GetArrayItem(devArrayJson, i);
	    	apIdJson = cJSON_GetObjectItem(devJson, "apId");
	    	devIdJson = cJSON_GetObjectItem(devJson, "devId");
	    	delayArrayJson = cJSON_GetObjectItem(devJson, "delay");
	    	number2 = cJSON_GetArraySize(delayArrayJson);
	    	for(j = 0, delay = 0; j < number2; j++){
	    		delayJson = cJSON_GetArrayItem(delayArrayJson, j);
	    		delay += delayJson->valueint;
	    	}
	    	M1_LOG_DEBUG("apId:%s, devId:%s\n",apIdJson->valuestring, devIdJson->valuestring);
	    	paramArrayJson = cJSON_GetObjectItem(devJson, "param");
	    	number2 = cJSON_GetArraySize(paramArrayJson);
	    	for(j = 0; j < number2; j++){
	    		paramJson = cJSON_GetArrayItem(paramArrayJson, j);
	    		typeJson = cJSON_GetObjectItem(paramJson, "type");
	    		valueJson = cJSON_GetObjectItem(paramJson, "value");
	    		M1_LOG_DEBUG("type:%d, value:%d\n",typeJson->valueint, valueJson->valueint);
	    		
			    sql = "insert into scenario_table(ID, SCEN_NAME, SCEN_PIC, DISTRICT, AP_ID, DEV_ID, TYPE, VALUE, DELAY, ACCOUNT,TIME) values(?,?,?,?,?,?,?,?,?,?,?);";
			    M1_LOG_DEBUG("sql:%s\n",sql);
			    //sqlite3_reset(stmt);
			    sqlite3_finalize(stmt);
			    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
				sqlite3_bind_int(stmt, 1, id);
				sqlite3_bind_text(stmt, 2, scenNameJson->valuestring, -1, NULL);
				sqlite3_bind_text(stmt, 3, scenPicJson->valuestring, -1, NULL);
				sqlite3_bind_text(stmt, 4, districtJson->valuestring, -1, NULL);
				sqlite3_bind_text(stmt, 5, apIdJson->valuestring, -1, NULL);
				sqlite3_bind_text(stmt, 6, devIdJson->valuestring, -1, NULL);
				sqlite3_bind_int(stmt, 7, typeJson->valueint);
				sqlite3_bind_int(stmt, 8, valueJson->valueint);
				sqlite3_bind_int(stmt, 9, delay);
				sqlite3_bind_text(stmt, 10, "Dalitek",-1,NULL);
				sqlite3_bind_text(stmt, 11, time, -1, NULL);
				id++;
				rc = thread_sqlite3_step(&stmt, db); 
		
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
    free(sql_2);
    sqlite3_finalize(stmt);
    return ret;
    
}

int scenario_alarm_create_handle(payload_t data)
{
	M1_LOG_DEBUG("scenario_alarm_create_handle\n");
	int id;
	int rc, ret = M1_PROTOCOL_OK;
	int row_number = 0;
	char* time = (char*)malloc(30);
	char* sql_1 = (char*)malloc(300);
	char* sql = NULL;
	char* errorMsg = NULL;
	cJSON* scenNameJson = NULL;
	cJSON* hourJson = NULL;
	cJSON* minutesJson = NULL;
	cJSON* weekJson = NULL;
	cJSON* statusJson = NULL;
	sqlite3* db = NULL;
	sqlite3_stmt* stmt = NULL;

	if(data.pdu == NULL){
		ret = M1_PROTOCOL_FAILED;
	}
	getNowTime(time);
	/*获取数据库*/
	db = data.db;
	/*获取table id*/
	sql = "select ID from scen_alarm_table order by ID desc limit 1";
	/*linkage_table*/
	id = sql_id(db, sql);
	if(sqlite3_exec(db, "BEGIN", NULL, NULL, &errorMsg)==SQLITE_OK){
        M1_LOG_DEBUG("BEGIN\n");
		/*获取收到数据包信息*/
	    scenNameJson = cJSON_GetObjectItem(data.pdu, "scenarioName");
	    M1_LOG_DEBUG("scenName:%s\n",scenNameJson->valuestring);
	    hourJson = cJSON_GetObjectItem(data.pdu, "hour");
	    M1_LOG_DEBUG("hour:%d\n",hourJson->valueint);
	    minutesJson = cJSON_GetObjectItem(data.pdu, "minutes");
	    M1_LOG_DEBUG("minutes:%d\n",minutesJson->valueint);
	    weekJson = cJSON_GetObjectItem(data.pdu, "week");
	    M1_LOG_DEBUG("week:%s\n",weekJson->valuestring);
	    statusJson = cJSON_GetObjectItem(data.pdu, "status");
	    M1_LOG_DEBUG("status:%s\n",statusJson->valuestring);
	   	/*删除原有表scenario_table中的旧scenario*/
		sprintf(sql_1,"select ID from scen_alarm_table where SCEN_NAME = \"%s\";",scenNameJson->valuestring);	
		row_number = sql_row_number(db, sql_1);
		M1_LOG_DEBUG("row_number:%d\n",row_number);
		if(row_number > 0){
			sprintf(sql_1,"delete from scen_alarm_table where SCEN_NAME = \"%s\";",scenNameJson->valuestring);				
			//sqlite3_reset(stmt);
			sqlite3_finalize(stmt);
			sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
			while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW);
		}
		
	    sql = "insert into scen_alarm_table(ID, SCEN_NAME, HOUR, MINUTES, WEEK, STATUS, TIME) values(?,?,?,?,?,?,?);";
	    M1_LOG_DEBUG("sql:%s\n",sql);
	    //sqlite3_reset(stmt);
	    sqlite3_finalize(stmt);
	    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
		sqlite3_bind_int(stmt, 1, id);
		sqlite3_bind_text(stmt, 2, scenNameJson->valuestring, -1, NULL);
		sqlite3_bind_int(stmt, 3, hourJson->valueint);
		sqlite3_bind_int(stmt, 4, minutesJson->valueint);
		sqlite3_bind_text(stmt, 5, weekJson->valuestring, -1, NULL);
		sqlite3_bind_text(stmt, 6, statusJson->valuestring, -1, NULL);
		sqlite3_bind_text(stmt, 7, time, -1, NULL);
		
		rc = thread_sqlite3_step(&stmt, db); 
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
    
    if(rc == SQLITE_ERROR)
    	ret =  M1_PROTOCOL_FAILED;

    Finish:
    free(time);
    free(sql_1);
    sqlite3_free(errorMsg);
    sqlite3_finalize(stmt);

    return ret;
}

int app_req_scenario(payload_t data)
{
	M1_LOG_DEBUG("app_req_scenario\n");
	/*cJSON*/
	int i;
	int rc,ret = M1_PROTOCOL_OK;
    int pduType = TYPE_M1_REPORT_SCEN_INFO;
    int hour,minutes;
    int delay, pId;
    int type, value;
    char* account = NULL;
    char* sql = (char*)malloc(300);
    char* sql_1 = (char*)malloc(300);
    char* sql_2 = (char*)malloc(300);
    char* scen_name = NULL,*district = NULL,*week = NULL, *alarm_status = NULL;
   	char* ap_id = NULL, *dev_id = "devId", *dev_name = NULL, *scen_pic = NULL;
    cJSON * pJsonRoot = NULL;
    cJSON * pduJsonObject = NULL;
    cJSON * devDataJsonArray = NULL;
    cJSON*  devDataObject= NULL;
    cJSON*  alarmObject= NULL;
    cJSON*  deviceObject= NULL;
    cJSON*  deviceArrayObject= NULL;
    cJSON*  paramArrayObject= NULL;
    cJSON*  paramObject= NULL;
    cJSON*  delayArrayObject= NULL;
    cJSON*  delayObject= NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL, *stmt_1 = NULL,*stmt_2 = NULL;

    db = data.db;
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_DEBUG("pJsonRoot NULL\n");
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
    sprintf(sql,"select ACCOUNT from account_info where CLIENT_FD = %03d order by ID desc limit 1;",data.clientFd);
    M1_LOG_DEBUG( "%s\n", sql);
    //sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
        account =  sqlite3_column_text(stmt, 0);
    }
    if(account == NULL){
        M1_LOG_ERROR( "user account do not exist\n");    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }else{
        M1_LOG_DEBUG("clientFd:%03d,account:%s\n",data.clientFd, account);
    }

    /*取场景名称*/
    sprintf(sql,"select distinct SCEN_NAME from scenario_table where ACCOUNT = \"%s\";",account);
    M1_LOG_DEBUG("sql:%s\n",sql);
    //sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
	    devDataObject = cJSON_CreateObject();
	    if(NULL == devDataObject)
	    {
	        cJSON_Delete(devDataObject);
	        ret = M1_PROTOCOL_FAILED;
        	goto Finish;
	    }
	    cJSON_AddItemToArray(devDataJsonArray, devDataObject);
	    scen_name = sqlite3_column_text(stmt, 0);
	    cJSON_AddStringToObject(devDataObject, "scenName", scen_name);
	    /*根据场景名称选出隶属区域*/
	    sprintf(sql_1,"select DISTRICT, SCEN_PIC from scenario_table where SCEN_NAME = \"%s\" limit 1;",scen_name);
	    M1_LOG_DEBUG("sql_1:%s\n",sql_1);
	    //sqlite3_reset(stmt_1);
	    sqlite3_finalize(stmt_1);
	    sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
	    rc = thread_sqlite3_step(&stmt_1, db); 

		if(rc == SQLITE_ROW){
			district = sqlite3_column_text(stmt_1, 0);
			if(district == NULL){
				ret = M1_PROTOCOL_FAILED;
        		goto Finish;
			}
			cJSON_AddStringToObject(devDataObject, "district", district);
			scen_pic = sqlite3_column_text(stmt_1, 1);
			if(scen_pic == NULL){
				ret = M1_PROTOCOL_FAILED;
        		goto Finish;
			}
			cJSON_AddStringToObject(devDataObject, "scenPic", scen_pic);
		}
		
    	/*选择区域定时执行信息*/
	    alarmObject = cJSON_CreateObject();
	    if(NULL == alarmObject)
	    {
	        cJSON_Delete(alarmObject);
	        ret = M1_PROTOCOL_FAILED;
        	goto Finish;
	    }
	    cJSON_AddItemToObject(devDataObject, "alarm", alarmObject);
	    sprintf(sql_1,"select HOUR, MINUTES, WEEK, STATUS from scen_alarm_table where SCEN_NAME = \"%s\" limit 1;",scen_name);
	    M1_LOG_DEBUG("sql_1:%s\n",sql_1);
	    //sqlite3_reset(stmt_1);
	    sqlite3_finalize(stmt_1);
	    sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
	    rc = thread_sqlite3_step(&stmt_1, db); 
		if(rc == SQLITE_ROW){
			hour = sqlite3_column_int(stmt_1, 0);
			cJSON_AddNumberToObject(alarmObject, "hour", hour);
			minutes = sqlite3_column_int(stmt_1, 1);
			cJSON_AddNumberToObject(alarmObject, "minutes", minutes);
			week = sqlite3_column_text(stmt_1, 2);
			cJSON_AddStringToObject(alarmObject, "week", week);
			alarm_status = sqlite3_column_text(stmt_1, 3);
			cJSON_AddStringToObject(alarmObject, "status", alarm_status);
		} 

		/*选择场景内的设备配置信息*/
	    deviceArrayObject = cJSON_CreateArray();
	    if(NULL == deviceArrayObject)
	    {
	        cJSON_Delete(deviceArrayObject);
	        ret = M1_PROTOCOL_FAILED;
        	goto Finish;
	    }
	    cJSON_AddItemToObject(devDataObject, "device", deviceArrayObject);
	    /*从场景表scenario_table中选出设备相关信息*/
	    sprintf(sql_1,"select DISTINCT DEV_ID from scenario_table where SCEN_NAME = \"%s\" order by ID asc;",scen_name);
	    M1_LOG_DEBUG("sql_1:%s\n",sql_1);
	    //sqlite3_reset(stmt_1);
	    sqlite3_finalize(stmt_1);
	    sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
	    while(thread_sqlite3_step(&stmt_1,db) == SQLITE_ROW){
		    deviceObject = cJSON_CreateObject();
		    if(NULL == deviceObject)
		    {
		        cJSON_Delete(deviceObject);
		        ret = M1_PROTOCOL_FAILED;
        		goto Finish;
		    }
		    cJSON_AddItemToArray(deviceArrayObject, deviceObject);
		    
		   	dev_id = sqlite3_column_text(stmt_1, 0);
		   	M1_LOG_DEBUG("devId:%s\n",dev_id);
		   	cJSON_AddStringToObject(deviceObject, "devId", dev_id);
		   	/*获取AP_ID*/
			sprintf(sql_2,"select AP_ID, DELAY from scenario_table where SCEN_NAME = \"%s\" and DEV_ID = \"%s\" limit 1;",scen_name, dev_id);		   	
		   	printf("sql_2:%s\n",sql_2);
		   	//sqlite3_reset(stmt_2);
		   	sqlite3_finalize(stmt_2);
	    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
	    	rc = thread_sqlite3_step(&stmt_2, db); 
			
			if(rc == SQLITE_ROW){
				ap_id = sqlite3_column_text(stmt_2, 0);
			   	cJSON_AddStringToObject(deviceObject, "apId", ap_id);
			   	M1_LOG_DEBUG("apId:%s\n",ap_id);
			   	delay = sqlite3_column_int(stmt_2, 1);
			   	
			   	/*设备延时信息数组*/
			   	delayArrayObject = cJSON_CreateArray();
			   	if(NULL == delayArrayObject)
			    {
			        cJSON_Delete(delayArrayObject);
			        ret = M1_PROTOCOL_FAILED;
        			goto Finish;
			    }
			    cJSON_AddItemToObject(deviceObject, "delay", delayArrayObject);
			    M1_LOG_DEBUG("delay:%05d\n",delay);
			    M1_LOG_DEBUG("delay/SCENARIO_DELAY_TOP:%d\n",delay / SCENARIO_DELAY_TOP);
			    M1_LOG_DEBUG("delay \"%\" SCENARIO_DELAY_TOP:%d\n",delay % SCENARIO_DELAY_TOP);
			   	for(i = 0; i < (delay / SCENARIO_DELAY_TOP); i++){
					delayObject = cJSON_CreateNumber(SCENARIO_DELAY_TOP);
				    if(NULL == delayObject)
				    {
				        cJSON_Delete(delayObject);
				        ret = M1_PROTOCOL_FAILED;
        				goto Finish;
				    }
					cJSON_AddItemToArray(delayArrayObject, delayObject);
			   	}
			   	if((delay % SCENARIO_DELAY_TOP) > 0){
			   		delayObject = cJSON_CreateNumber(delay % SCENARIO_DELAY_TOP);
				    if(NULL == delayObject)
				    {
				        cJSON_Delete(delayObject);
				        ret = M1_PROTOCOL_FAILED;
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
	    	rc = thread_sqlite3_step(&stmt_2, db); 

			if(rc == SQLITE_ROW){
				dev_name = sqlite3_column_text(stmt_2, 0);
				pId = sqlite3_column_int(stmt_2, 1);
		   		M1_LOG_DEBUG("dev_name:%s, pId:%05d\n",dev_name, pId);
		   		cJSON_AddStringToObject(deviceObject, "devName", dev_name);
		   		cJSON_AddNumberToObject(deviceObject, "pId", pId);	
			}
		   	
		   	/*设备参数信息*/
		   	paramArrayObject = cJSON_CreateArray();
		   	if(NULL == paramArrayObject)
		    {
		        cJSON_Delete(paramArrayObject);
		        ret = M1_PROTOCOL_FAILED;
        		goto Finish;
		    }
		    cJSON_AddItemToObject(deviceObject, "param", paramArrayObject);
			
			sprintf(sql_2,"select TYPE, VALUE from scenario_table where SCEN_NAME = \"%s\" and DEV_ID = \"%s\";",scen_name, dev_id);
	    	//sqlite3_reset(stmt_2);
	    	sqlite3_finalize(stmt_2);
	    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
		   	while(thread_sqlite3_step(&stmt_2, db) == SQLITE_ROW){
			    paramObject = cJSON_CreateObject();
			    if(NULL == paramObject)
			    {
			        cJSON_Delete(paramObject);
			        ret = M1_PROTOCOL_FAILED;
        			goto Finish;
			    }
			    cJSON_AddItemToArray(paramArrayObject, paramObject);
				type = sqlite3_column_int(stmt_2, 0);
			   	cJSON_AddNumberToObject(paramObject, "type", type);
			   	M1_LOG_DEBUG("type:%05d\n");
			   	value = sqlite3_column_int(stmt_2, 1);
			   	cJSON_AddNumberToObject(paramObject, "value", value);
			   	M1_LOG_DEBUG("value:%05d\n",value);
		   	}
		
		}

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
    free(sql);
    free(sql_1);
    free(sql_2);
    sqlite3_finalize(stmt);
	sqlite3_finalize(stmt_1);
	sqlite3_finalize(stmt_2);
	cJSON_Delete(pJsonRoot);

    return ret;

}

int app_req_scenario_name(payload_t data)
{
	M1_LOG_DEBUG("app_req_scenario_name\n");
	int rc, ret = M1_PROTOCOL_OK;
    int pduType = TYPE_M1_REPORT_DISTRICT_INFO;
    char* sql = NULL;
    char* sql_1 = (char*)malloc(300);
    char* sql_2 = (char*)malloc(300);
    char* dist_name = NULL, *ap_id = NULL, *ap_name = NULL, *scen_name = NULL;
    cJSON * pJsonRoot = NULL;
    cJSON * pduJsonObject = NULL;
    cJSON * devDataJsonArray = NULL;
    cJSON*  devData= NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL, *stmt_1 = NULL,*stmt_2 = NULL;

    db = data.db;
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_DEBUG("pJsonRoot NULL\n");
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
    /*取区域名称*/
    sql = "select distinct SCEN_NAME from scenario_table;";
   	M1_LOG_DEBUG("sql:%s\n", sql);
    //sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
		scen_name = sqlite3_column_text(stmt, 0);
		M1_LOG_DEBUG("scen_name:%s\n",scen_name);
		devData = cJSON_CreateString(scen_name);
	    if(NULL == devData)
	    {
	        cJSON_Delete(devData);
	        ret = M1_PROTOCOL_FAILED;
        	goto Finish;
	    }
		cJSON_AddItemToArray(devDataJsonArray, devData);
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
    free(sql_1);
    free(sql_2);
	sqlite3_finalize(stmt);
	sqlite3_finalize(stmt_1);
	sqlite3_finalize(stmt_2);
    cJSON_Delete(pJsonRoot);

    return ret;

}

/*定时执行场景检查*/
void scenario_alarm_select(void)
{
 	printf("scenario_alarm_select\n");
 	int rc;
 	char* sql = NULL;
 	scen_alarm_t alarm;
 	sqlite3_stmt* stmt = NULL;
 	sqlite3* db = NULL;
 	sql = "select SCEN_NAME, HOUR, MINUTES, WEEK, STATUS from scen_alarm_table;";
 	while(1){
	 	rc = sqlite3_open("dev_info.db", &db);  
	     if( rc ){  
	         M1_LOG_ERROR( "Can't open database: %s\n", sqlite3_errmsg(db));  
	         return M1_PROTOCOL_FAILED;  
	     }else{  
	         M1_LOG_DEBUG( "Opened database successfully\n");  
	     }

	     sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	     while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
	     	alarm.scen_name = sqlite3_column_text(stmt, 0);
	     	alarm.hour = sqlite3_column_int(stmt, 1);
	     	alarm.week = sqlite3_column_text(stmt, 3);
	     	alarm.minutes = sqlite3_column_int(stmt, 2);
	     	alarm.status = sqlite3_column_text(stmt, 4);
	     	rc = scenario_alarm_check(alarm);
	     	if(rc)
	     		scenario_exec(alarm.scen_name, db);
	     }
	     sqlite3_finalize(stmt);
	     sqlite3_close(db);

	     sleep(60);
 	}
}

 static int scenario_alarm_check(scen_alarm_t alarm_time)
 {
 	int on_time_flag = 0;
 	char* week = NULL;
 	struct tm nowTime;
    struct timespec time;
 	
    clock_gettime(CLOCK_REALTIME, &time);  //获取相对于1970到现在的秒数
    localtime_r(&time.tv_sec, &nowTime);
    //printf("%04d-%02d-%02d %02d:%02d:%02d\n",nowTime.tm_year + 1900, nowTime.tm_mon + 1, nowTime.tm_mday,
    //    nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec);
     if(strcmp(alarm_time.status,"on") == 0){
     	switch(nowTime.tm_wday){
     		case 0: week = "sunday";break;
     		case 1: week = "monday";break;
     		case 2: week = "tuesday";break;
     		case 3: week = "wednessday";break;
     		case 4: week = "thursday";break;
     		case 5: week = "friday";break;
     		case 6: week = "saturday";break;	
     	}
     	/*周一到周日*/
     	if((strcmp(alarm_time.week, week) == 0) || (strcmp(alarm_time.week, "all") == 0)){
     		on_time_flag = 1;
     	}else if((nowTime.tm_wday != 6) && (nowTime.tm_wday != 0) && (strcmp(alarm_time.week, "workDay") == 0)){               //周一到周五
     		on_time_flag = 1;
     	}else{
     		on_time_flag = 0;
     	}
     	/*小时、分钟*/
     	if((alarm_time.hour == nowTime.tm_hour) && (alarm_time.minutes == nowTime.tm_min) && on_time_flag){
     		on_time_flag = 1;
     	}else{
     		on_time_flag = 0;
     	}
     	
    }
    return on_time_flag;
 }

/*APP执行场景*/
int app_exec_scenario(payload_t data)
{
	M1_LOG_DEBUG("app_exec_scenario\n");
	char* scenario = NULL;
	int ret = M1_PROTOCOL_OK,rc;
    sqlite3* db = NULL;

    if(data.pdu == NULL){
    	M1_LOG_DEBUG( "data NULL\n");	
    	ret = M1_PROTOCOL_FAILED;
		goto Finish;
    }
	scenario = data.pdu->valuestring;
	if(scenario == NULL){
		ret = M1_PROTOCOL_FAILED;
		goto Finish;
	}
	M1_LOG_DEBUG( "scenario:%s\n", scenario);
	
	db = data.db;
	ret = scenario_exec(scenario, db);

	Finish:
	return ret;
}

