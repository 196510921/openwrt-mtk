#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "m1_protocol.h"
#include "socket_server.h"


/*场景执行*/
int scenario_exec(char* data, sqlite3* db)
{
	cJSON * pJsonRoot = NULL; 
    cJSON * pduJsonObject = NULL;
    cJSON * devDataJsonArray = NULL;
    cJSON * devDataObject= NULL;
    cJSON * paramArray = NULL;
    cJSON*  paramObject = NULL;

	char sql[200],sql_1[200],sql_2[200],sql_3[200];
	char*ap_id = NULL,*dev_id = NULL,*status = NULL;
	int type,value,delay;
	sqlite3_stmt* stmt = NULL,*stmt_1 = NULL,*stmt_2 = NULL,*stmt_3 = NULL;
 
 	printf("scenario_exec\n");
    int rc;
    int clientFd;
    char * p = NULL;
    int pduType = TYPE_DEV_WRITE;

    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        printf("pJsonRoot NULL\n");
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
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

	sprintf(sql,"select distinct AP_ID from scenario_table where SCEN_NAME = \"%s\";", data);	
	
	printf("sql:%s\n",sql);
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
		ap_id = sqlite3_column_text(stmt,0);
		sqlite3_reset(stmt_1);
		sprintf(sql_1,"select distinct DEV_ID from scenario_table where SCEN_NAME = \"%s\" and AP_ID = \"%s\";",data, ap_id);	
		sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
		printf("sql_1:%s\n",sql_1);
		while(thread_sqlite3_step(&stmt_1, db) == SQLITE_ROW){
			dev_id = sqlite3_column_text(stmt_1,0);
			/*检查设备启/停状态*/
		 	sprintf(sql_2,"select STATUS from all_dev where DEV_ID = \"%s\";",dev_id);
		 	sqlite3_reset(stmt_2);
		 	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
		 	rc = thread_sqlite3_step(&stmt_1,db);
	
		   	if(rc == SQLITE_ROW){
				status = sqlite3_column_text(stmt_2,0);
			}		
		    if(strcmp(status,"ON") == 0){
				/*create device data object*/
				devDataObject = cJSON_CreateObject();
		        if(NULL == devDataObject)
		        {
		            // create object faild, exit
		            printf("devDataObject NULL\n");
		            cJSON_Delete(devDataObject);
		            return M1_PROTOCOL_FAILED;
		        }
		        cJSON_AddItemToArray(devDataJsonArray, devDataObject);
		       	cJSON_AddStringToObject(devDataObject,"devId",dev_id);
		        /*create param array*/
			    paramArray = cJSON_CreateArray();
			    if(NULL == paramArray)
			    {
			    	printf("paramArray NULL\n");
			        cJSON_Delete(paramArray);
			        return M1_PROTOCOL_FAILED;
			    }
				cJSON_AddItemToObject(devDataObject, "param", paramArray);
				sprintf(sql_2,"select TYPE, VALUE, DELAY from scenario_table where SCEN_NAME = \"%s\" and DEV_ID = \"%s\";",data, dev_id);
				printf("sql_2:%s\n",sql_2);
				sqlite3_reset(stmt_2);
				sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
				while(thread_sqlite3_step(&stmt_2,db) == SQLITE_ROW){
					type = sqlite3_column_int(stmt_2,0);
					value = sqlite3_column_int(stmt_2,1);
					delay = sqlite3_column_int(stmt_2,2);
					printf("type:%d,value:%d,delay:%d\n", type, value, delay);
				    paramObject = cJSON_CreateObject();
		            if(NULL == paramObject)
		            {
		                // create object faild, exit
		                printf("devDataObject NULL\n");
		                cJSON_Delete(paramObject);
		                return M1_PROTOCOL_FAILED;
		            }
		            cJSON_AddItemToArray(paramArray, paramObject);
		            cJSON_AddNumberToObject(paramObject, "type", type);
		            cJSON_AddNumberToObject(paramObject, "value", value);

	        	}
        	}
		}
    	p = cJSON_PrintUnformatted(pJsonRoot);
    	if(NULL == p)
    	{    
    		printf("p NULL\n");
        	cJSON_Delete(pJsonRoot);
        	return M1_PROTOCOL_FAILED;
    	}
    	/*get clientfd*/
    	sprintf(sql_3,"select CLIENT_FD from conn_info where AP_ID = \"%s\";",ap_id);
    	printf("sql_3:%s\n", sql_3);
    	sqlite3_reset(stmt_3);
    	sqlite3_prepare_v2(db, sql_3, strlen(sql_3), &stmt_3, NULL);
    	rc = thread_sqlite3_step(&stmt_3,db);
    
    	if(rc == SQLITE_ROW){
			clientFd = sqlite3_column_int(stmt_3,0);
		}		
    	
    	printf("string:%s\n",p);
    	socketSeverSend((uint8*)p, strlen(p), clientFd);
    	
	}

	sqlite3_finalize(stmt);
	sqlite3_finalize(stmt_1);
   	sqlite3_finalize(stmt_2);
   	sqlite3_finalize(stmt_3);
	cJSON_Delete(pJsonRoot);
	return M1_PROTOCOL_OK;
}

int scenario_create_handle(payload_t data)
{
	printf("scenario_create_handle\n");
	cJSON* scenNameJson = NULL;
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
	int id,id_1,rc, number1,number2,i,j,delay = 0;
	int row_number = 0;
	char time[30];
	char sql_1[200],sql_2[200];

	if(data.pdu == NULL) return M1_PROTOCOL_FAILED;
	getNowTime(time);

    rc = sqlite3_open("dev_info.db", &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }
    /*获取场景名称*/
    scenNameJson = cJSON_GetObjectItem(data.pdu, "scenName");
    printf("scenName:%s\n",scenNameJson->valuestring);
	/*获取数据包中的alarm信息*/
	alarmJson = cJSON_GetObjectItem(data.pdu, "alarm");
	/*将alarm信息存入alarm表中*/
	/*获取table id*/
	char* sql = "select ID from scen_alarm_table order by ID desc limit 1";
	/*获取alarm表中id*/
	id = sql_id(db, sql);
	/*获取收到数据包信息*/
    hourJson = cJSON_GetObjectItem(alarmJson, "hour");
    printf("hour:%d\n",hourJson->valueint);
    minutesJson = cJSON_GetObjectItem(alarmJson, "minutes");
    printf("minutes:%d\n",minutesJson->valueint);
    weekJson = cJSON_GetObjectItem(alarmJson, "week");
    printf("week:%s\n",weekJson->valuestring);
    statusJson = cJSON_GetObjectItem(alarmJson, "status");
    printf("status:%s\n",statusJson->valuestring);
   	/*删除原有表scenario_table中的旧scenario*/
	sprintf(sql_1,"select ID from scen_alarm_table where SCEN_NAME = \"%s\";",scenNameJson->valuestring);	
	row_number = sql_row_number(db, sql_1);
	printf("row_number:%d\n",row_number);
	if(row_number > 0){
		sprintf(sql_1,"delete from scen_alarm_table where SCEN_NAME = \"%s\";",scenNameJson->valuestring);				
		sqlite3_reset(stmt);
		sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
		while(thread_sqlite3_step(&stmt,db) == SQLITE_ROW);
	}
	
    sql = "insert into scen_alarm_table(ID, SCEN_NAME, HOUR, MINUTES, WEEK, STATUS, TIME) values(?,?,?,?,?,?,?);";
    printf("sql:%s\n",sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	sqlite3_bind_int(stmt, 1, id);
	sqlite3_bind_text(stmt, 2, scenNameJson->valuestring, -1, NULL);
	sqlite3_bind_int(stmt, 3, hourJson->valueint);
	sqlite3_bind_int(stmt, 4, minutesJson->valueint);
	sqlite3_bind_text(stmt, 5, weekJson->valuestring, -1, NULL);
	sqlite3_bind_text(stmt, 6, statusJson->valuestring, -1, NULL);
	sqlite3_bind_text(stmt, 7, time, -1, NULL);
	
	rc = thread_sqlite3_step(&stmt, db); 
	
	/*获取联动表 id*/
	sql = "select ID from scenario_table order by ID desc limit 1";
	/*linkage_table*/
	id = sql_id(db, sql);
   	/*删除原有表scenario_table中的旧scenario*/
	sprintf(sql_1,"select ID from scenario_table where SCEN_NAME = \"%s\";",scenNameJson->valuestring);	
	row_number = sql_row_number(db, sql_1);
	printf("row_number:%d\n",row_number);
	if(row_number > 0){
		sprintf(sql_1,"delete from scenario_table where SCEN_NAME = \"%s\";",scenNameJson->valuestring);				
		sqlite3_reset(stmt);
		sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
		while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW);
	}

    districtJson = cJSON_GetObjectItem(data.pdu, "district");
    printf("district:%s\n",districtJson->valuestring);
    devArrayJson = cJSON_GetObjectItem(data.pdu, "device");
    number1 = cJSON_GetArraySize(devArrayJson);
    printf("number1:%d\n",number1);
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
    	printf("apId:%s, devId:%s, delay:%05d\n",apIdJson->valuestring, devIdJson->valuestring, delayJson->valueint);
    	paramArrayJson = cJSON_GetObjectItem(devJson, "param");
    	number2 = cJSON_GetArraySize(paramArrayJson);
    	for(j = 0; j < number2; j++){
    		paramJson = cJSON_GetArrayItem(paramArrayJson, j);
    		typeJson = cJSON_GetObjectItem(paramJson, "type");
    		valueJson = cJSON_GetObjectItem(paramJson, "value");
    		printf("type:%d, value:%d\n",typeJson->valueint, valueJson->valueint);
    		
		    sql = "insert into scenario_table(ID, SCEN_NAME, DISTRICT, AP_ID, DEV_ID, TYPE, VALUE, DELAY, TIME) values(?,?,?,?,?,?,?,?,?);";
		    printf("sql:%s\n",sql);
		    sqlite3_reset(stmt);
		    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
			sqlite3_bind_int(stmt, 1, id);
			sqlite3_bind_text(stmt, 2, scenNameJson->valuestring, -1, NULL);
			sqlite3_bind_text(stmt, 3, districtJson->valuestring, -1, NULL);
			sqlite3_bind_text(stmt, 4, apIdJson->valuestring, -1, NULL);
			sqlite3_bind_text(stmt, 5, devIdJson->valuestring, -1, NULL);
			sqlite3_bind_int(stmt, 6, typeJson->valueint);
			sqlite3_bind_int(stmt, 7, valueJson->valueint);
			sqlite3_bind_int(stmt, 8, delay);
			sqlite3_bind_text(stmt, 9, time, -1, NULL);
			id++;
			rc = thread_sqlite3_step(&stmt, db); 
	
    	}	
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return M1_PROTOCOL_OK;
    
}

int scenario_alarm_create_handle(payload_t data)
{
	printf("scenario_alarm_create_handle\n");
	cJSON* scenNameJson = NULL;
	cJSON* hourJson = NULL;
	cJSON* minutesJson = NULL;
	cJSON* weekJson = NULL;
	cJSON* statusJson = NULL;
	
	sqlite3* db = NULL;
	sqlite3_stmt* stmt = NULL;
	int id, rc;
	int row_number = 0;
	char time[30];
	char sql_1[200];

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
	char* sql = "select ID from scen_alarm_table order by ID desc limit 1";
	/*linkage_table*/
	id = sql_id(db, sql);

	/*获取收到数据包信息*/
    scenNameJson = cJSON_GetObjectItem(data.pdu, "scenarioName");
    printf("scenName:%s\n",scenNameJson->valuestring);
    hourJson = cJSON_GetObjectItem(data.pdu, "hour");
    printf("hour:%d\n",hourJson->valueint);
    minutesJson = cJSON_GetObjectItem(data.pdu, "minutes");
    printf("minutes:%d\n",minutesJson->valueint);
    weekJson = cJSON_GetObjectItem(data.pdu, "week");
    printf("week:%s\n",weekJson->valuestring);
    statusJson = cJSON_GetObjectItem(data.pdu, "status");
    printf("status:%s\n",statusJson->valuestring);
   	/*删除原有表scenario_table中的旧scenario*/
	sprintf(sql_1,"select ID from scen_alarm_table where SCEN_NAME = \"%s\";",scenNameJson->valuestring);	
	row_number = sql_row_number(db, sql_1);
	printf("row_number:%d\n",row_number);
	if(row_number > 0){
		sprintf(sql_1,"delete from scen_alarm_table where SCEN_NAME = \"%s\";",scenNameJson->valuestring);				
		sqlite3_reset(stmt);
		sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
		while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW);
	}
	
    sql = "insert into scen_alarm_table(ID, SCEN_NAME, HOUR, MINUTES, WEEK, STATUS, TIME) values(?,?,?,?,?,?,?);";
    printf("sql:%s\n",sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	sqlite3_bind_int(stmt, 1, id);
	sqlite3_bind_text(stmt, 2, scenNameJson->valuestring, -1, NULL);
	sqlite3_bind_int(stmt, 3, hourJson->valueint);
	sqlite3_bind_int(stmt, 4, minutesJson->valueint);
	sqlite3_bind_text(stmt, 5, weekJson->valuestring, -1, NULL);
	sqlite3_bind_text(stmt, 6, statusJson->valuestring, -1, NULL);
	sqlite3_bind_text(stmt, 7, time, -1, NULL);
	
	rc = thread_sqlite3_step(&stmt, db); 
   
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    if(rc == SQLITE_ERROR)
    	return M1_PROTOCOL_FAILED;

    return M1_PROTOCOL_OK;
    
}

int app_req_scenario(int clientFd, int sn)
{
	printf("app_req_scenario\n");
	/*cJSON*/
    int pduType = TYPE_M1_REPORT_SCEN_INFO;

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
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL, *stmt_1 = NULL,*stmt_2 = NULL;
    char* sql = NULL;
    char sql_1[200],sql_2[200];

    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        printf("pJsonRoot NULL\n");
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

    char* scen_name = NULL,*district = NULL,*week = NULL, *alarm_status = NULL;
    int hour,minutes;
    /*取场景名称*/
    sql = "select distinct SCEN_NAME from scenario_table;";
    printf("sql:%s\n",sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
	    devDataObject = cJSON_CreateObject();
	    if(NULL == devDataObject)
	    {
	        cJSON_Delete(devDataObject);
	        return M1_PROTOCOL_FAILED;
	    }
	    cJSON_AddItemToArray(devDataJsonArray, devDataObject);
	    scen_name = sqlite3_column_text(stmt, 0);
	    cJSON_AddStringToObject(devDataObject, "scenName", scen_name);
	    /*根据场景名称选出隶属区域*/
	    sprintf(sql_1,"select DISTRICT from scenario_table where SCEN_NAME = \"%s\" limit 1;",scen_name);
	    printf("sql_1:%s\n",sql_1);
	    sqlite3_reset(stmt_1);
	    sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
	    rc = thread_sqlite3_step(&stmt_1, db); 

		if(rc == SQLITE_ROW){
			district = sqlite3_column_text(stmt_1, 0);
			cJSON_AddStringToObject(devDataObject, "district", district);
		}
		
    	/*选择区域定时执行信息*/
	    alarmObject = cJSON_CreateObject();
	    if(NULL == alarmObject)
	    {
	        cJSON_Delete(alarmObject);
	        return M1_PROTOCOL_FAILED;
	    }
	    cJSON_AddItemToObject(devDataObject, "alarm", alarmObject);
	    sprintf(sql_1,"select HOUR, MINUTES, WEEK, STATUS from scen_alarm_table where SCEN_NAME = \"%s\" limit 1;",scen_name);
	    printf("sql_1:%s\n",sql_1);
	    sqlite3_reset(stmt_1);
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
	        return M1_PROTOCOL_FAILED;
	    }
	    cJSON_AddItemToObject(devDataObject, "device", deviceArrayObject);
	    /*设备*/
	    char* ap_id = NULL, *dev_id = "devId", *dev_name = NULL;
	    int delay;
	    /*从场景表scenario_table中选出设备相关信息*/
	    sprintf(sql_1,"select DISTINCT DEV_ID from scenario_table where SCEN_NAME = \"%s\" order by ID asc;",scen_name);
	    printf("sql_1:%s\n",sql_1);
	    sqlite3_reset(stmt_1);
	    sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
	    while(thread_sqlite3_step(&stmt_1,db) == SQLITE_ROW){
		    deviceObject = cJSON_CreateObject();
		    if(NULL == deviceObject)
		    {
		        cJSON_Delete(deviceObject);
		        return M1_PROTOCOL_FAILED;
		    }
		    cJSON_AddItemToArray(deviceArrayObject, deviceObject);
		    
		   	dev_id = sqlite3_column_text(stmt_1, 0);
		   	printf("devId:%s\n",dev_id);
		   	cJSON_AddStringToObject(deviceObject, "devId", dev_id);
		   	/*获取AP_ID*/
			sprintf(sql_2,"select AP_ID, DELAY from scenario_table where SCEN_NAME = \"%s\" and DEV_ID = \"%s\" limit 1;",scen_name, dev_id);		   	
		   	printf("sql_2:%s\n",sql_2);
		   	sqlite3_reset(stmt_2);
	    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
	    	rc = thread_sqlite3_step(&stmt_2, db); 
			
			if(rc == SQLITE_ROW){
				int i;
				ap_id = sqlite3_column_text(stmt_2, 0);
			   	cJSON_AddStringToObject(deviceObject, "apId", ap_id);
			   	printf("apId:%s\n",ap_id);
			   	delay = sqlite3_column_int(stmt_2, 1);
			   	
			   	/*设备延时信息数组*/
			   	delayArrayObject = cJSON_CreateArray();
			   	if(NULL == delayArrayObject)
			    {
			        cJSON_Delete(delayArrayObject);
			        return M1_PROTOCOL_FAILED;
			    }
			    cJSON_AddItemToObject(deviceObject, "delay", delayArrayObject);
			    printf("delay:%05d\n",delay);
			    printf("delay/SCENARIO_DELAY_TOP:%d\n",delay / SCENARIO_DELAY_TOP);
			    printf("delay \"%\" SCENARIO_DELAY_TOP:%d\n",delay % SCENARIO_DELAY_TOP);
			   	for(i = 0; i < (delay / SCENARIO_DELAY_TOP); i++){
					delayObject = cJSON_CreateNumber(SCENARIO_DELAY_TOP);
				    if(NULL == delayObject)
				    {
				        cJSON_Delete(delayObject);
				        return M1_PROTOCOL_FAILED;
				    }
					cJSON_AddItemToArray(delayArrayObject, delayObject);
			   	}
			   	if((delay % SCENARIO_DELAY_TOP) > 0){
			   		delayObject = cJSON_CreateNumber(delay % SCENARIO_DELAY_TOP);
				    if(NULL == delayObject)
				    {
				        cJSON_Delete(delayObject);
				        return M1_PROTOCOL_FAILED;
				    }
					cJSON_AddItemToArray(delayArrayObject, delayObject);
				}
			
			}
		   	/*获取设备名称*/
		   	sprintf(sql_2,"select DEV_NAME from all_dev where DEV_ID = \"%s\" limit 1;",dev_id);		   	
		   	sqlite3_reset(stmt_2);
	    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
	    	rc = thread_sqlite3_step(&stmt_2, db); 

			if(rc == SQLITE_ROW){
				dev_name = sqlite3_column_text(stmt_2, 0);
		   		printf("dev_name:%s\n",dev_name);
		   		cJSON_AddStringToObject(deviceObject, "devName", dev_name);	
			}
		   	
		   	/*设备参数信息*/
		   	paramArrayObject = cJSON_CreateArray();
		   	if(NULL == paramArrayObject)
		    {
		        cJSON_Delete(paramArrayObject);
		        return M1_PROTOCOL_FAILED;
		    }
		    cJSON_AddItemToObject(deviceObject, "device", paramArrayObject);
			
			sprintf(sql_2,"select TYPE, VALUE from scenario_table where SCEN_NAME = \"%s\" and DEV_ID = \"%s\";",scen_name, dev_id);
	    	sqlite3_reset(stmt_2);
	    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
		   	int type, value;
		   	while(thread_sqlite3_step(&stmt_2, db) == SQLITE_ROW){
			    paramObject = cJSON_CreateObject();
			    if(NULL == paramObject)
			    {
			        cJSON_Delete(paramObject);
			        return M1_PROTOCOL_FAILED;
			    }
			    cJSON_AddItemToArray(paramArrayObject, paramObject);
				type = sqlite3_column_int(stmt_2, 0);
			   	cJSON_AddNumberToObject(paramObject, "type", type);
			   	printf("type:%05d\n");
			   	value = sqlite3_column_int(stmt_2, 1);
			   	cJSON_AddNumberToObject(paramObject, "value", value);
			   	printf("value:%05d\n",value);
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

    printf("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;

}

int app_req_scenario_name(int clientFd, int sn)
{
	printf("app_req_scenario_name\n");
	/*cJSON*/
    int pduType = TYPE_M1_REPORT_DISTRICT_INFO;

    cJSON * pJsonRoot = NULL;
    cJSON * pduJsonObject = NULL;
    cJSON * devDataJsonArray = NULL;
    cJSON*  devData= NULL;
    
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL, *stmt_1 = NULL,*stmt_2 = NULL;
    char* sql = NULL;
    char sql_1[200],sql_2[200];

    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        printf("pJsonRoot NULL\n");
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
    /*取区域名称*/
    char* dist_name = NULL, *ap_id = NULL, *ap_name = NULL, *scen_name = NULL;
    sql = "select distinct SCEN_NAME from scenario_table;";
   	printf("sql:%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
		scen_name = sqlite3_column_text(stmt, 0);
		printf("scen_name:%s\n",scen_name);
		devData = cJSON_CreateString(scen_name);
	    if(NULL == devData)
	    {
	        cJSON_Delete(devData);
	        return M1_PROTOCOL_FAILED;
	    }
		cJSON_AddItemToArray(devDataJsonArray, devData);
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

    printf("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;

}
