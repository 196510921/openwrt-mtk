#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "m1_protocol.h"
#include "socket_server.h"

extern fifo_t dev_data_fifo;
extern fifo_t link_exec_fifo;
void trigger_cb(void* udp, int type, char const* db_name, char const* table_name, sqlite3_int64 rowid);

static int device_exec(char* data)
{

}

int linkage_msg_handle(payload_t data)
{
	printf("linkage_msg_handle\n");
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

	sqlite3* db = NULL;
	sqlite3_stmt* stmt = NULL, *stmt_1 = NULL;
	int id, id_1, rc, number1, i, j, number2;
	int row_number = 0;
	char time[30];
	char sql_1[200];

	if(data.pdu == NULL) return M1_PROTOCOL_FAILED;
	getNowTime(time);

	/*获取收到数据包信息*/
    linkNameJson = cJSON_GetObjectItem(data.pdu, "linkName");
    districtJson = cJSON_GetObjectItem(data.pdu, "district");
    logicalJson = cJSON_GetObjectItem(data.pdu, "logical");
    execTypeJson = cJSON_GetObjectItem(data.pdu, "execType");
    triggerJson = cJSON_GetObjectItem(data.pdu, "trigger");
    executeJson = cJSON_GetObjectItem(data.pdu, "execute");
    executeArrayJson = cJSON_GetArrayItem(executeJson, 0);
    if(strcmp( execTypeJson->valuestring, "scenario") == 0){
    	scenNameJson = cJSON_GetObjectItem(executeArrayJson, "scenName");
	}else{
		scenNameJson = cJSON_GetObjectItem(executeArrayJson, "devId");
	}
    printf("linkName:%s, district:%s, logical:%s, execType:%s, scenName:%s\n",linkNameJson->valuestring,
    	districtJson->valuestring, logicalJson->valuestring, execTypeJson->valuestring, scenNameJson->valuestring);
    /*打开sqlite3*/
    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

   	/*检查联动是否存在*/
	sprintf(sql_1,"select ID from linkage_table where LINK_NAME = \"%s\" order by ID desc limit 1;",linkNameJson->valuestring);	
	row_number = sql_row_number(db, sql_1);
	printf("row_number:%d\n",row_number);
	if(row_number > 0){
		/*delete linkage_table member*/
		sprintf(sql_1,"delete from linkage_table where LINK_NAME = \"%s\";",linkNameJson->valuestring);
		sqlite3_reset(stmt);
		sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
		while(sqlite3_step(stmt) == SQLITE_ROW);
		/*delete link_trigger_table member*/
		sprintf(sql_1,"delete from link_trigger_table where LINK_NAME = \"%s\";",linkNameJson->valuestring);
		sqlite3_reset(stmt);
		sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
		while(sqlite3_step(stmt) == SQLITE_ROW);
		/*delete link_exec_table member*/
		sprintf(sql_1,"delete from link_exec_table where LINK_NAME = \"%s\";",linkNameJson->valuestring);
		sqlite3_reset(stmt);
		sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
		while(sqlite3_step(stmt) == SQLITE_ROW);
	}

	/*获取table id*/
	char* sql = "select ID from linkage_table order by ID desc limit 1";
    /*linkage_table*/
    id = sql_id(db, sql);
    sql = "insert into linkage_table(ID, LINK_NAME, DISTRICT, EXEC_TYPE, EXEC_ID, STATUS, TIME) values(?,?,?,?,?,?,?);";
    printf("sql:%s\n",sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	sqlite3_bind_int(stmt, 1, id);
	id++;
	sqlite3_bind_text(stmt, 2,  linkNameJson->valuestring, -1, NULL);
	sqlite3_bind_text(stmt, 3,  districtJson->valuestring, -1, NULL);
	sqlite3_bind_text(stmt, 4,  execTypeJson->valuestring, -1, NULL);
	sqlite3_bind_text(stmt, 5,  scenNameJson->valuestring, -1, NULL);

	sqlite3_bind_text(stmt, 6,  "OFF", -1, NULL);
	sqlite3_bind_text(stmt, 7,  time, -1, NULL);
	rc = sqlite3_step(stmt);
	printf("step1() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");

    /*link_trigger_table*/
    sql = "select ID from link_trigger_table order by ID desc limit 1";
    id_1 = sql_id(db, sql);
    sql = "insert into link_trigger_table(ID, LINK_NAME, DISTRICT, AP_ID, DEV_ID, TYPE, THRESHOLD, CONDITION,LOGICAL,STATUS,TIME) values(?,?,?,?,?,?,?,?,?,?,?);";
    printf("sql:%s\n",sql);
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
    		printf("type:%d, condition:%s, value:%d\n",typeJson->valueint,conditionJson->valuestring,valueJson->valueint);
    	
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
		   	rc = sqlite3_step(stmt_1);
		   	printf("step2() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");
	    	
	    }

    }
    /*联动执行*/
    if(strcmp( execTypeJson->valuestring, "scenario") != 0){
	    sql = "select ID from link_trigger_table order by ID desc limit 1";
	    id_1 = sql_id(db, sql);
	    sql = "insert into link_exec_table(ID, LINK_NAME, DISTRICT, AP_ID, DEV_ID, TYPE, VALUE, DELAY, TIME) values(?,?,?,?,?,?,?,?,?);";
	    printf("sql:%s\n",sql);
	    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt_1, NULL);
	    number1 = cJSON_GetArraySize(executeJson);
	    for(i = 0; i < number1; i++){
	    	executeArrayJson = cJSON_GetArrayItem(executeJson, i);
	    	apIdJson = cJSON_GetObjectItem(executeArrayJson, "apId");
	    	devIdJson = cJSON_GetObjectItem(executeArrayJson, "devId");
	    	delayJson = cJSON_GetObjectItem(executeArrayJson, "delay");
	    	paramJson = cJSON_GetObjectItem(triggerArrayJson, "param");
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
			   	sqlite3_bind_text(stmt_1, 4,  apIdJson->valueint, -1, NULL);
			   	sqlite3_bind_text(stmt_1, 5,  devIdJson->valuestring, -1, NULL);
			   	sqlite3_bind_int(stmt_1, 6,  typeJson->valueint);
			   	sqlite3_bind_int(stmt_1, 7,  valueJson->valueint);
			   	sqlite3_bind_int(stmt_1, 8,  delayJson->valueint);
			   	sqlite3_bind_text(stmt_1, 9,  time, -1, NULL);
			   	rc = sqlite3_step(stmt_1);
			   	printf("step2() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");
	    	}
	    }
	}

    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
    sqlite3_close(db);

    return M1_PROTOCOL_OK;
}

int linkage_task(void)
{
	printf("linkage_task\n");
	int rc;
	uint32_t rowid;
	char sql[200], *exec_type = NULL,*exec_id = NULL, *link_name =  NULL;

	sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

	while(fifo_read(&link_exec_fifo, &rowid)){
		sprintf(sql,"select EXEC_TYPE, EXEC_ID, LINK_NAME from linkage_table where rowid = %05d;",rowid);
		printf("sql:%s\n", sql);
		sqlite3_reset(stmt);
		sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
		rc = sqlite3_step(stmt);
		printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 		
		if(rc == SQLITE_ROW){
			exec_type = sqlite3_column_text(stmt,0);
			exec_id = sqlite3_column_text(stmt,1);
			link_name = sqlite3_column_text(stmt,2);
			if(strcmp(exec_type, "scenario") == 0){
				scenario_exec(exec_id, db);
			}else{
				device_exec(link_name);			
			}
		}
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return M1_PROTOCOL_OK;
}

static char* linkage_status(char* condition, int threshold, int value)
{	
	char* status = NULL;
	int tmp;

	tmp = strcmp(condition, "<");
	if (0 == tmp){ 
		if(value < threshold)
			status = "OFF";
		else 
			status = "ON";
	}
	else {
		if(value >= threshold)
			status = "ON";
		else
			status = "OFF";
	}
	printf("linkage_status:%s, value:%d, threshold:%d\n",status,value, threshold);

	return status;
}

static void linkage_check(sqlite3* db, char* link_name)
{
	printf("linkage_check\n");
	sqlite3_stmt* stmt = NULL;
	int link_flag = 1, rc, rowid;
	char *status, *logical;
	char sql[200];

	sprintf(sql,"select STATUS, LOGICAL from link_trigger_table where LINK_NAME = \"%s\";",link_name);
	printf("sql:%s\n",sql);
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	while(sqlite3_step(stmt) == SQLITE_ROW){
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
	printf("link_flag:%d\n",link_flag);
	if(link_flag){
		/*执行设备*/
		sprintf(sql,"update linkage_table set STATUS = \"ON\" where LINK_NAME = \"%s\";", link_name);
		sqlite3_reset(stmt);
		sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
		rc = sqlite3_step(stmt);
		printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
		
		sprintf(sql,"select rowid from linkage_table where LINK_NAME = \"%s\";", link_name);
		sqlite3_reset(stmt);
		sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
		rc = sqlite3_step(stmt);
		printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
		if(rc == SQLITE_ROW){
			rowid = sqlite3_column_int(stmt,0);
			printf("rowid:%d\n",rowid);
			fifo_write(&link_exec_fifo, rowid);
		}
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);

}

int trigger_cb_handle(void)
{
	printf("trigger_cb_handle\n");
	int rc ,value, param_type, threshold;
	uint32_t rowid;
	char* devId = NULL, *condition = NULL, *status = NULL, *link_name = NULL;
	char sql[200], sql_1[200], sql_2[200];

	sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL,*stmt_1 = NULL, *stmt_2 = NULL;

    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    while(fifo_read(&dev_data_fifo, &rowid)){
		/*check linkage table*/
		sprintf(sql,"select VALUE, DEV_ID, TYPE from param_table where rowid = %05d;",rowid);
		//printf("sql:%s\n",sql);
		sqlite3_reset(stmt); 
		rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
		if(rc){
			printf("sqlite3_prepare_v2 failed\n");
			return M1_PROTOCOL_FAILED;
		}else{
			printf("sqlite3_prepare_v2 ok\n");  		
		}
		rc = sqlite3_step(stmt);
		printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
		if(rc == SQLITE_ROW){
		    value = sqlite3_column_int(stmt,0);
		    devId = sqlite3_column_text(stmt,1);
		    param_type = sqlite3_column_int(stmt,2);
		    printf("value:%05d, devId:%s, param_type:\n",value, devId, param_type);
		}
	 	/*检查设备启/停状态*/
	 	sprintf(sql_1,"select STATUS from all_dev where DEV_ID = \"%s\";",devId);
	 	sqlite3_reset(stmt_1);
	 	sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
	 	rc = sqlite3_step(stmt_1);
		printf("step2() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
		if(rc == SQLITE_ROW){		
	    	status = sqlite3_column_text(stmt_1,0);
		}
	    /*设备处于启动状态*/
	    if(strcmp(status,"ON") == 0){
		    /*get linkage table*/
		    sprintf(sql_1,"select THRESHOLD,CONDITION,LINK_NAME from link_trigger_table where DEV_ID = \"%s\" and TYPE = %05d;",devId,param_type);
			printf("sql_1:%s\n",sql_1);
			sqlite3_reset(stmt_1);
			sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
			while(sqlite3_step(stmt_1) == SQLITE_ROW){
				threshold = sqlite3_column_int(stmt_1,0);
		        condition = sqlite3_column_text(stmt_1,1);
		        link_name = sqlite3_column_text(stmt_1,2);
		        printf("threshold:%05d, condition:%s\n, link_name:%s\n", threshold, condition, link_name);
		 		status = linkage_status(condition, threshold, value);
		 		/*set linkage table*/
		 		sprintf(sql_2,"update link_trigger_table set STATUS = \"%s\" where DEV_ID = \"%s\" and TYPE = %05d and LINK_NAME = \"%s\" ;",status,devId,param_type,link_name);
		 		//printf("sql_2:%s\n",sql_2);
		 		sqlite3_reset(stmt_2);
				sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
				rc = sqlite3_step(stmt_2);
				printf("step2() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 		
				/*检查是否满足触发条件*/
				linkage_check(db, link_name);
		 	}
	 	}
 	}

 	sqlite3_finalize(stmt);
 	sqlite3_finalize(stmt_1);
 	sqlite3_finalize(stmt_2);
    sqlite3_close(db);
     
}

void trigger_cb(void* udp, int type, char const* db_name, char const* table_name, sqlite3_int64 rowid)
{
	int rc;

	printf("trigger_cb\n");
	rc = strcmp(udp, "AP_report_data_handle");
	if(0 == rc)
	{
		fifo_write(&dev_data_fifo, rowid);
	}
	
}

int app_req_linkage(int clientFd, int sn)
{
	printf("app_req_linkage\n");
	/*数据包类型*/
	int pduType = TYPE_M1_REPORT_LINK_INFO;
	/*Json对象*/
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

    char* link_name = NULL, *district = NULL, *logical = NULL, *exec_type = NULL, *exec_id = NULL,
    *ap_id = NULL, *dev_id = NULL, *condition = NULL, *dev_name = NULL;
    int type, value, delay;
    sql = "select LINK_NAME, DISTRICT, EXEC_TYPE, EXEC_ID from linkage_table;";
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    while(sqlite3_step(stmt) == SQLITE_ROW){
    	devDataObject = cJSON_CreateObject();
	    if(NULL == devDataObject)
	    {
	        cJSON_Delete(devDataObject);
	        return M1_PROTOCOL_FAILED;
	    }
	    cJSON_AddItemToArray(devDataJsonArray, devDataObject);
	    link_name = sqlite3_column_text(stmt, 0);
	    cJSON_AddStringToObject(devDataObject, "linkName", link_name);
	    district = sqlite3_column_text(stmt, 1);
	    cJSON_AddStringToObject(devDataObject, "district", district);
	    exec_type = sqlite3_column_text(stmt, 2);
	    cJSON_AddStringToObject(devDataObject, "execType", exec_type);
	    exec_id = sqlite3_column_text(stmt, 3);
	    /*获取logical*/
	    sprintf(sql_1,"select LOGICAL from link_trigger_table where LINK_NAME = \"%s\" limit 1;", link_name);
	    printf("sql_1:%s\n", sql_1);
    	sqlite3_reset(stmt_1);
    	sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
    	rc = sqlite3_step(stmt_1); 
		printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
		if(rc == SQLITE_ROW){
			logical = sqlite3_column_text(stmt_1, 0);
	    	cJSON_AddStringToObject(devDataObject, "logical", logical);
		}
	    /*获取联动触发信息*/
 	    triggerJsonArray = cJSON_CreateArray();
 	    if(NULL == triggerJsonArray)
 	    {
 	        cJSON_Delete(triggerJsonArray);
 	        return M1_PROTOCOL_FAILED;
 	    }
 	    cJSON_AddItemToObject(devDataObject, "trigger", triggerJsonArray);
 	    sprintf(sql_1,"select DISTINCT DEV_ID from link_trigger_table where LINK_NAME = \"%s\";", link_name);
 	    printf("sql_1:%s\n", sql_1);
    	sqlite3_reset(stmt_1);
    	sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
    	while(sqlite3_step(stmt_1) == SQLITE_ROW){
    		triggerObject = cJSON_CreateObject();
		    if(NULL == triggerObject)
		    {
		        cJSON_Delete(triggerObject);
		        return M1_PROTOCOL_FAILED;
		    }
		    cJSON_AddItemToArray(triggerJsonArray, triggerObject);
    		dev_id = sqlite3_column_text(stmt_1, 0);
    		cJSON_AddStringToObject(triggerObject, "devId", dev_id);
    		/*获取AP_ID*/
			sprintf(sql_2,"select AP_ID from link_trigger_table where LINK_NAME = \"%s\" and DEV_ID = \"%s\" limit 1;",link_name, dev_id);		   	
		   	printf("sql_2:%s\n", sql_2);
		   	sqlite3_reset(stmt_2);
	    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
	    	rc = sqlite3_step(stmt_2); 
			printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
			if(rc == SQLITE_ROW){
			   	ap_id = sqlite3_column_text(stmt_2, 0);
			   	cJSON_AddStringToObject(triggerObject, "apId", ap_id);
			   	printf("apId:%s\n",ap_id);
		   	}
		   	/*获取设备名称*/
		   	sprintf(sql_2,"select DEV_NAME from all_dev where DEV_ID = \"%s\" limit 1;",dev_id);		   	
		   	sqlite3_reset(stmt_2);
	    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
	    	rc = sqlite3_step(stmt_2); 
			printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");
			if(rc == SQLITE_ROW){ 
			   	dev_name = sqlite3_column_text(stmt_2, 0);
			   	cJSON_AddStringToObject(triggerObject, "devName", dev_name);
		   	}
    		/*获取参数*/
    		paramJsonArray = cJSON_CreateArray();
 	    	if(NULL == paramJsonArray)
 	    	{
 	        	cJSON_Delete(paramJsonArray);
 	        	return M1_PROTOCOL_FAILED;
 	    	}
 	    	cJSON_AddItemToObject(triggerObject, "param", paramJsonArray);

 	    	sprintf(sql_2,"select TYPE, THRESHOLD, CONDITION from link_trigger_table where LINK_NAME = \"%s\" and DEV_ID = \"%s\";",link_name, dev_id);
	    	printf("sql_2:%s\n", sql_2);
	    	sqlite3_reset(stmt_2);
	    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
		   	while(sqlite3_step(stmt_2) == SQLITE_ROW){
			    paramObject = cJSON_CreateObject();
			    if(NULL == paramObject)
			    {
			        cJSON_Delete(paramObject);
			        return M1_PROTOCOL_FAILED;
			    }
			    cJSON_AddItemToArray(paramJsonArray, paramObject);
				type = sqlite3_column_int(stmt_2, 0);
			   	cJSON_AddNumberToObject(paramObject, "type", type);
			   	printf("type:%05d\n");
			   	value = sqlite3_column_int(stmt_2, 1);
			   	cJSON_AddNumberToObject(paramObject, "value", value);
			   	printf("value:%05d\n",value);
			   	condition = sqlite3_column_text(stmt_2, 2);
			   	cJSON_AddStringToObject(paramObject, "condition", condition);
			   	printf("condition:%s\n",condition);
		   	}
    	}
    	/*获取执行设备信息*/
	    execJsonArray = cJSON_CreateArray();
	    if(NULL == execJsonArray)
	    {
	        cJSON_Delete(execJsonArray);
	        return M1_PROTOCOL_FAILED;
	    }
	    cJSON_AddItemToObject(devDataObject, "execute", execJsonArray);
    	if(strcmp(exec_type,"scenario") != 0){
	 	    sprintf(sql_1,"select DISTINCT DEV_ID from link_exec_table where LINK_NAME = \"%s\";", link_name);
	 	    printf("sql_1:%s\n", sql_1);
	    	sqlite3_reset(stmt_1);
	    	sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
	    	while(sqlite3_step(stmt_1) == SQLITE_ROW){
	    		execObject = cJSON_CreateObject();
			    if(NULL == execObject)
			    {
			        cJSON_Delete(execObject);
			        return M1_PROTOCOL_FAILED;
			    }
			    cJSON_AddItemToArray(execJsonArray, execObject);
	    		dev_id = sqlite3_column_text(stmt_1, 0);
	    		cJSON_AddStringToObject(execObject, "devId", dev_id);
	    		/*获取AP_ID*/
				sprintf(sql_2,"select AP_ID DELAY from link_exec_table where LINK_NAME = \"%s\" and DEV_ID = \"%s\" limit 1;",link_name, dev_id);		   	
			   	printf("sql_2:%s\n", sql_2);
			   	sqlite3_reset(stmt_2);
		    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
		    	rc = sqlite3_step(stmt_2); 
				printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
				if(rc == SQLITE_ROW){
				   	ap_id = sqlite3_column_text(stmt_2, 0);
				   	cJSON_AddStringToObject(execObject, "apId", ap_id);
				   	printf("apId:%s\n",ap_id);
				   	delay = sqlite3_column_int(stmt_2, 1);
				   	cJSON_AddNumberToObject(execObject, "delay", delay);
				   	printf("delay:%s\n",delay);
			   	}
			   	/*获取设备名称*/
			   	sprintf(sql_2,"select DEV_NAME from all_dev where DEV_ID = \"%s\" limit 1;",dev_id);		   	
			   	sqlite3_reset(stmt_2);
		    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
		    	rc = sqlite3_step(stmt_2); 
				printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
				if(rc == SQLITE_ROW){
				   	dev_name = sqlite3_column_text(stmt_2, 0);
				   	cJSON_AddStringToObject(execObject, "devName", dev_name);
				}
	    		/*获取参数*/
	    		paramJsonArray = cJSON_CreateArray();
	 	    	if(NULL == paramJsonArray)
	 	    	{
	 	        	cJSON_Delete(paramJsonArray);
	 	        	return M1_PROTOCOL_FAILED;
	 	    	}
	 	    	cJSON_AddItemToObject(execObject, "param", paramJsonArray);

	 	    	sprintf(sql_2,"select TYPE, VALUE from link_exec_table where LINK_NAME = \"%s\" and DEV_ID = \"%s\";",link_name, dev_id);
		    	printf("sql_2:%s\n", sql_2);
		    	sqlite3_reset(stmt_2);
		    	sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
			   	while(sqlite3_step(stmt_2) == SQLITE_ROW){
				    paramObject = cJSON_CreateObject();
				    if(NULL == paramObject)
				    {
				        cJSON_Delete(paramObject);
				        return M1_PROTOCOL_FAILED;
				    }
				    cJSON_AddItemToArray(paramJsonArray, paramObject);
					type = sqlite3_column_int(stmt_2, 0);
				   	cJSON_AddNumberToObject(paramObject, "type", type);
				   	printf("type:%05d\n");
				   	value = sqlite3_column_int(stmt_2, 1);
				   	cJSON_AddNumberToObject(paramObject, "value", value);
				   	printf("value:%05d\n",value);
			   	}
	    	}
    	}else{
    		/*获取场景信息*/
    		execObject = cJSON_CreateObject();
		    if(NULL == execObject)
		    {
		        cJSON_Delete(execObject);
		        return M1_PROTOCOL_FAILED;
		    }
		    cJSON_AddItemToArray(execJsonArray, execObject);
	    	cJSON_AddStringToObject(execObject, "scenName", exec_id);
			
			
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

void fifo_init(fifo_t* fifo, uint32_t* buffer, uint32_t len)
{
    fifo->buffer = buffer;
    fifo->len = len;
    fifo->wptr = fifo->rptr = 0;
}

void fifo_write(fifo_t* fifo, uint32_t d)
{
    fifo->buffer[fifo->wptr] = d;
    fifo->wptr = (fifo->wptr + 1) % fifo->len;
}

uint32_t fifo_read(fifo_t* fifo, uint32_t* d)
{
    if (fifo->wptr == fifo->rptr) return 0;
    
    *d = fifo->buffer[fifo->rptr];
    fifo->rptr = (fifo->rptr + 1) % fifo->len;
    
    return 1;
}



