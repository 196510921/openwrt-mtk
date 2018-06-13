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
extern sqlite3* db;
void trigger_cb(void* udp, int type, char const* db_name, char const* table_name, sqlite3_int64 rowid);

static int device_exec(char* data, sqlite3* db)
{
	int rc                  = 0;
	int ret                 = M1_PROTOCOL_OK;
    int clientFd            = 0;
    int pduType             = TYPE_DEV_WRITE;
	int type                = 0;
	int value               = 0;
	int delay               = 0;
	char *p                 = NULL;
	/*Json*/
	cJSON *pJsonRoot        = NULL; 
    cJSON *pduJsonObject    = NULL;
    cJSON *devDataJsonArray = NULL;
    cJSON *devDataObject    = NULL;
    cJSON *paramArray       = NULL;
    cJSON *paramObject      = NULL;
    /*sql*/
	char *sql               = NULL;
	char *sql_1             = NULL;
	char *sql_2             = NULL;
	char *sql_3             = NULL;
	char *ap_id             = NULL;
	char *dev_id            = NULL;
	sqlite3_stmt *stmt      = NULL;
	sqlite3_stmt *stmt_1    = NULL;
	sqlite3_stmt *stmt_2    = NULL;
	sqlite3_stmt *stmt_3    = NULL;
 
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
	{
		sql = "select distinct AP_ID from link_exec_table where LINK_NAME = ?;";
		M1_LOG_DEBUG("sql:%s\n",sql);
		if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
		{
	        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = M1_PROTOCOL_FAILED;
	        goto Finish; 
	    }
	}
	/*获取设备信息*/
	{
		sql_1 = "select distinct DEV_ID from link_exec_table where LINK_NAME = ? and AP_ID = ?;";
		M1_LOG_DEBUG("sql_1:%s\n",sql_1);
		if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
		{
	        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = M1_PROTOCOL_FAILED;
	        goto Finish; 
	    }
	}
	/*获取参数信息*/
	{
		sql_2 = "select TYPE,VALUE,DELAY from link_exec_table where LINK_NAME = ? and DEV_ID = ?;";
		M1_LOG_DEBUG("sql_1:%s\n",sql_2);
		if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK)
		{
	        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = M1_PROTOCOL_FAILED;
	        goto Finish; 
	    }
	}
	/*clientFd*/
	{
		sql_3 = "select CLIENT_FD from conn_info where AP_ID = ?;";
		M1_LOG_DEBUG("%s\n",sql_3);
		if(sqlite3_prepare_v2(db, sql_3, strlen(sql_3), &stmt_3, NULL) != SQLITE_OK)
		{
	        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = M1_PROTOCOL_FAILED;
	        goto Finish; 
	    }
	}

    sqlite3_bind_text(stmt, 1,  data, -1, NULL);
	while(sqlite3_step(stmt) == SQLITE_ROW)
	{
		ap_id = sqlite3_column_text(stmt, 0);	
		
		/*获取设备信息*/
		sqlite3_bind_text(stmt_1, 1, data, -1, NULL);
		sqlite3_bind_text(stmt_1, 2, ap_id, -1, NULL);
		while(sqlite3_step(stmt_1) == SQLITE_ROW)
		{
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
			sqlite3_bind_text(stmt_2, 1, data, -1, NULL);
			sqlite3_bind_text(stmt_2, 2, dev_id, -1, NULL);
			while(sqlite3_step(stmt_2) == SQLITE_ROW)
			{
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

			p = cJSON_PrintUnformatted(pJsonRoot);
			if(NULL == p)
	    	{    
	    		M1_LOG_ERROR("p NULL\n");
	        	cJSON_Delete(pJsonRoot);
	        	ret = M1_PROTOCOL_FAILED;
	        	goto Finish;
	    	}

	    	/*get clientfd*/
    		sqlite3_bind_text(stmt_3, 1, ap_id, -1, NULL);
	    	rc = sqlite3_step(stmt_3);
            if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
                if(rc == SQLITE_CORRUPT)
                    exit(0);
            }
	    
	    	if(rc == SQLITE_ROW)
	    	{
				clientFd = sqlite3_column_int(stmt_3,0);
			}		
	    	
	    	M1_LOG_DEBUG("string:%s\n",p);

	    	sqlite3_reset(stmt_2);
        	sqlite3_clear_bindings(stmt_2);

        	sqlite3_reset(stmt_3);
        	sqlite3_clear_bindings(stmt_3);
		}   	
		delay_send(p, delay, clientFd);

		sqlite3_reset(stmt_1);
        sqlite3_clear_bindings(stmt_1);
	}

	Finish:
	if(stmt)
		sqlite3_finalize(stmt);
	if(stmt_1)
		sqlite3_finalize(stmt_1);
   	if(stmt_2)
   		sqlite3_finalize(stmt_2);
   	if(stmt_3)
   		sqlite3_finalize(stmt_3);
	
	cJSON_Delete(pJsonRoot);
	
	return ret;
}

int linkage_msg_handle(payload_t data)
{
	M1_LOG_DEBUG("linkage_msg_handle\n");

	int i                   = 0;
	int j                   = 0;
	int delay               = 0;
	int rc                  = 0;
	int ret                 = M1_PROTOCOL_OK;
	int number1             = 0;
	int number2             = 0;
	char* errorMsg          = NULL;
	char* time_0            = NULL;
	char* time_1            = NULL;
	char* time_2            = NULL;
	/*Json*/
	cJSON* linkNameJson     = NULL;
	cJSON* districtJson     = NULL;
	cJSON* logicalJson      = NULL;
	cJSON* execTypeJson     = NULL;
	cJSON* triggerJson      = NULL;
	cJSON* triggerArrayJson = NULL;
	cJSON* executeJson      = NULL;
	cJSON* executeArrayJson = NULL;
	cJSON* scenNameJson     = NULL;
	cJSON* apIdJson         = NULL;
	cJSON* devIdJson        = NULL;
	cJSON* paramJson        = NULL;
	cJSON* paramArrayJson   = NULL;
	cJSON* typeJson         = NULL;
	cJSON* conditionJson    = NULL;
	cJSON* valueJson        = NULL;
	cJSON* delayJson        = NULL;
	cJSON* delayArrayJson   = NULL;
	/*sql*/
	char* sql               = NULL;
	char* sql_1             = NULL;
	char* sql_2             = NULL;
	char* sql_0_1           = NULL;
	char* sql_1_1           = NULL;
	char* sql_2_1           = NULL;
	char* sql_0_2           = NULL;
	char* sql_1_2           = NULL;
	char* sql_2_2           = NULL;
	sqlite3* db             = NULL;
	sqlite3_stmt *stmt      = NULL;
	sqlite3_stmt *stmt_1    = NULL;
	sqlite3_stmt *stmt_2    = NULL;
	sqlite3_stmt *stmt_0_1  = NULL;
	sqlite3_stmt *stmt_1_1  = NULL;
	sqlite3_stmt *stmt_2_1  = NULL;
	sqlite3_stmt *stmt_0_2  = NULL;
	sqlite3_stmt *stmt_1_2  = NULL;
	sqlite3_stmt *stmt_2_2  = NULL;

	if(data.pdu == NULL){
		ret = M1_PROTOCOL_FAILED;	
		goto Finish;
	} 

	/*获取收到数据包信息*/
    linkNameJson = cJSON_GetObjectItem(data.pdu, "linkName");
    districtJson = cJSON_GetObjectItem(data.pdu, "district");
    logicalJson = cJSON_GetObjectItem(data.pdu, "logical");
    execTypeJson = cJSON_GetObjectItem(data.pdu, "execType");
    triggerJson = cJSON_GetObjectItem(data.pdu, "trigger");

    if(strcmp( execTypeJson->valuestring, "scenario") == 0)
    {
    	executeJson = cJSON_GetObjectItem(data.pdu, "exeScen");
    	executeArrayJson = cJSON_GetArrayItem(executeJson, 0);
    	scenNameJson = cJSON_GetObjectItem(executeArrayJson, "scenName");
	}
	else
	{
		executeJson = cJSON_GetObjectItem(data.pdu, "exeDev");
    	executeArrayJson = cJSON_GetArrayItem(executeJson, 0);
		scenNameJson = cJSON_GetObjectItem(executeArrayJson, "devId");
	}
    M1_LOG_DEBUG("linkName:%s, district:%s, logical:%s, execType:%s, scenName:%s\n",linkNameJson->valuestring,
    	districtJson->valuestring, logicalJson->valuestring, execTypeJson->valuestring, scenNameJson->valuestring);
    
    /*获取sqlite3*/
    db = data.db;
    /*linkage_table*/
    {
    	sql = "insert or replace into linkage_table(LINK_NAME, DISTRICT, EXEC_TYPE, EXEC_ID, STATUS, ENABLE)values(?,?,?,?,?,?);";
	    M1_LOG_DEBUG("sql:%s\n",sql);
	    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	    {
    	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    	    ret = M1_PROTOCOL_FAILED;
    	    goto Finish; 
    	}

    }
    /*link_trigger_table*/
    {
    	sql_1 = "insert or replace into link_trigger_table(LINK_NAME, DISTRICT, AP_ID, DEV_ID, TYPE, THRESHOLD, CONDITION,LOGICAL,STATUS)values(?,?,?,?,?,?,?,?,?);";
	    M1_LOG_DEBUG("sql_1:%s\n",sql_1);
	    if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
	    {
    	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    	    ret = M1_PROTOCOL_FAILED;
    	    goto Finish; 
    	}

    }
    /*link_exec_table*/
    {
    	sql_2 = "insert or replace into link_exec_table(LINK_NAME, DISTRICT, AP_ID, DEV_ID, TYPE, VALUE, DELAY) values(?,?,?,?,?,?,?);";
	    M1_LOG_DEBUG("sql_2:%s\n",sql_2);
	    if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK)
	    {
    	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    	    ret = M1_PROTOCOL_FAILED;
    	    goto Finish; 
    	}

    }
    /*查询linkage_table历史数据时间*/
    {
    	sql_0_1 = "select DISTINCT TIME from linkage_table where LINK_NAME = ? and DISTRICT = ? ;";
	    M1_LOG_DEBUG("%s\n",sql_0_1);
	    if(sqlite3_prepare_v2(db, sql_0_1, strlen(sql_0_1), &stmt_0_1, NULL) != SQLITE_OK)
	    {
    	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    	    ret = M1_PROTOCOL_FAILED;
    	    goto Finish; 
    	}
    }
    /*删除linkage_table没被跟新数据*/
    {
    	sql_0_2 = "delete from linkage_table where LINK_NAME = ? and DISTRICT = ? and TIME = ?;";
	    M1_LOG_DEBUG("%s\n",sql_0_2);
	    if(sqlite3_prepare_v2(db, sql_0_2, strlen(sql_0_2), &stmt_0_2, NULL) != SQLITE_OK)
	    {
    	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    	    ret = M1_PROTOCOL_FAILED;
    	    goto Finish; 
    	}
    }
    /*查询link_trigger_table历史数据时间*/
    {
    	sql_1_1 = "select DISTINCT TIME from link_trigger_table where LINK_NAME = ? and DISTRICT = ?;";
	    M1_LOG_DEBUG("%s\n",sql_1_1);
	    if(sqlite3_prepare_v2(db, sql_1_1, strlen(sql_1_1), &stmt_1_1, NULL) != SQLITE_OK)
	    {
    	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    	    ret = M1_PROTOCOL_FAILED;
    	    goto Finish; 
    	}
    }
    /*删除link_trigger_table没被跟新数据*/
    {
    	sql_1_2 = "delete from link_trigger_table where LINK_NAME = ? and DISTRICT = ? and TIME = ?;";
	    M1_LOG_DEBUG("%s\n",sql_1_2);
	    if(sqlite3_prepare_v2(db, sql_1_2, strlen(sql_1_2), &stmt_1_2, NULL) != SQLITE_OK)
	    {
    	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    	    ret = M1_PROTOCOL_FAILED;
    	    goto Finish; 
    	}
    }
    /*查询link_exec_table历史数据时间*/
    {
    	sql_2_1 = "select DISTINCT TIME from link_exec_table where LINK_NAME = ? and DISTRICT = ?;";
	    M1_LOG_DEBUG("%s\n",sql_2_1);
	    if(sqlite3_prepare_v2(db, sql_2_1, strlen(sql_2_1), &stmt_2_1, NULL) != SQLITE_OK)
	    {
    	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    	    ret = M1_PROTOCOL_FAILED;
    	    goto Finish; 
    	}
    }
    /*删除link_exec_table没被跟新数据*/
    {
    	sql_2_2 = "delete from link_exec_table where LINK_NAME = ? and DISTRICT = ? and TIME = ?;";
	    M1_LOG_DEBUG("%s\n",sql_2_2);
	    if(sqlite3_prepare_v2(db, sql_2_2, strlen(sql_2_2), &stmt_2_2, NULL) != SQLITE_OK)
	    {
    	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    	    ret = M1_PROTOCOL_FAILED;
    	    goto Finish; 
    	}
    }

    /*查询历史数据时间*/
    {
    	/*linkage table*/
    	sqlite3_bind_text(stmt_0_1, 1,  linkNameJson->valuestring, -1, NULL);
		sqlite3_bind_text(stmt_0_1, 2,  districtJson->valuestring, -1, NULL);

		rc = sqlite3_step(stmt_0_1);
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            if(rc == SQLITE_CORRUPT)
                exit(0);
        }
        else
        {
        	time_0 = sqlite3_column_text(stmt_0_1, 0);
        }

        /*linkage_trigger_table*/
		sqlite3_bind_text(stmt_1_1, 1,  linkNameJson->valuestring, -1, NULL);
		sqlite3_bind_text(stmt_1_1, 2,  districtJson->valuestring, -1, NULL);

		rc = sqlite3_step(stmt_1_1);    
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            if(rc == SQLITE_CORRUPT)
                exit(0);
        }
        else
        {
        	time_1 = sqlite3_column_text(stmt_1_1, 0);
        }      

        /*linkage_exec_table*/
		sqlite3_bind_text(stmt_2_1, 1,  linkNameJson->valuestring, -1, NULL);
		sqlite3_bind_text(stmt_2_1, 2,  districtJson->valuestring, -1, NULL);

		rc = sqlite3_step(stmt_2_1);    
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            if(rc == SQLITE_CORRUPT)
                exit(0);
        }
        else
        {
        	time_2 = sqlite3_column_text(stmt_2_1, 0);
        }  
    }

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        M1_LOG_DEBUG("BEGIN IMMEDIATE\n");

		sqlite3_bind_text(stmt, 1,  linkNameJson->valuestring, -1, NULL);
		sqlite3_bind_text(stmt, 2,  districtJson->valuestring, -1, NULL);
		sqlite3_bind_text(stmt, 3,  execTypeJson->valuestring, -1, NULL);
		sqlite3_bind_text(stmt, 4,  scenNameJson->valuestring, -1, NULL);

		sqlite3_bind_text(stmt, 5,  "OFF", -1, NULL);
		sqlite3_bind_text(stmt, 6,  "on", -1, NULL);
		rc = sqlite3_step(stmt);     
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            if(rc == SQLITE_CORRUPT)
                exit(0);
        }

	    /*link_trigger_table*/
	    number1 = cJSON_GetArraySize(triggerJson);
	    for(i = 0; i < number1; i++)
	    {
	    	triggerArrayJson = cJSON_GetArrayItem(triggerJson, i);
	    	apIdJson = cJSON_GetObjectItem(triggerArrayJson, "apId");
	    	devIdJson = cJSON_GetObjectItem(triggerArrayJson, "devId");
	    	paramJson = cJSON_GetObjectItem(triggerArrayJson, "param");
	    	number2 = cJSON_GetArraySize(paramJson);
	    	for(j = 0; j < number2; j++)
	    	{	
	    		paramArrayJson = cJSON_GetArrayItem(paramJson, j);
	    		typeJson = cJSON_GetObjectItem(paramArrayJson, "type");
	    		conditionJson = cJSON_GetObjectItem(paramArrayJson, "condition");
	    		valueJson = cJSON_GetObjectItem(paramArrayJson, "value");
	    		M1_LOG_DEBUG("type:%d, condition:%s, value:%d\n",\
	    			typeJson->valueint,conditionJson->valuestring,valueJson->valueint);
	    	
			   	sqlite3_bind_text(stmt_1, 1,  linkNameJson->valuestring, -1, NULL);
			   	sqlite3_bind_text(stmt_1, 2,  districtJson->valuestring, -1, NULL);
			   	sqlite3_bind_text(stmt_1, 3,  apIdJson->valuestring, -1, NULL);
			   	sqlite3_bind_text(stmt_1, 4,  devIdJson->valuestring, -1, NULL);
			   	sqlite3_bind_int(stmt_1, 5,  typeJson->valueint);
			   	sqlite3_bind_int(stmt_1, 6,  valueJson->valueint);
			   	sqlite3_bind_text(stmt_1, 7,  conditionJson->valuestring, -1, NULL);
			   	sqlite3_bind_text(stmt_1, 8,  logicalJson->valuestring, -1, NULL);
			   	sqlite3_bind_text(stmt_1, 9,  "OFF", -1, NULL);
			   
			   	rc = sqlite3_step(stmt_1);  
            	if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            	{
            	    M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            	    if(rc == SQLITE_CORRUPT)
            	        exit(0);
            	}
		    	
		    	sqlite3_reset(stmt_1);
		    	sqlite3_clear_bindings(stmt_1);	
		    }

	    }
	    /*联动执行信息*/
	    if(strcmp( execTypeJson->valuestring, "scenario") != 0)
	    {
		    number1 = cJSON_GetArraySize(executeJson);
		    for(i = 0; i < number1; i++)
		    {
		    	executeArrayJson = cJSON_GetArrayItem(executeJson, i);
		    	apIdJson = cJSON_GetObjectItem(executeArrayJson, "apId");
		    	devIdJson = cJSON_GetObjectItem(executeArrayJson, "devId");
		    	paramJson = cJSON_GetObjectItem(executeArrayJson, "param");
		    	delayArrayJson = cJSON_GetObjectItem(executeArrayJson, "delay");
		    	number2 = cJSON_GetArraySize(delayArrayJson);
		    	for(j = 0, delay = 0; j < number2; j++)
		    	{
		    		delayJson = cJSON_GetArrayItem(delayArrayJson, j);
		    		delay += delayJson->valueint;
		    	}
		    	number2 = cJSON_GetArraySize(paramJson);
		    	for(j = 0; j < number2; j++)
		    	{
		    		paramArrayJson = cJSON_GetArrayItem(paramJson, j);
		    		typeJson = cJSON_GetObjectItem(paramArrayJson, "type");
		    		valueJson = cJSON_GetObjectItem(paramArrayJson, "value");
		    		
				   	
				   	sqlite3_bind_text(stmt_2, 1,  linkNameJson->valuestring, -1, NULL);
				   	sqlite3_bind_text(stmt_2, 2,  districtJson->valuestring, -1, NULL);
				   	sqlite3_bind_text(stmt_2, 3,  apIdJson->valuestring, -1, NULL);
				   	sqlite3_bind_text(stmt_2, 4,  devIdJson->valuestring, -1, NULL);
				   	sqlite3_bind_int(stmt_2, 5,  typeJson->valueint);
				   	sqlite3_bind_int(stmt_2, 6,  valueJson->valueint);
				   	sqlite3_bind_int(stmt_2, 7,  delay);
	
				   	rc = sqlite3_step(stmt_2);     
            		if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            		{
            		    M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            		    if(rc == SQLITE_CORRUPT)
            		        exit(0);
            		}
		    	
		    		sqlite3_reset(stmt_2);
		    		sqlite3_clear_bindings(stmt_2); 
		    	}
		    }
		}

		/*删除表中历史无效数据*/
		{
			/*linkage_trigger_table*/
			if(time_0 != NULL)
			{
				sqlite3_bind_text(stmt_0_2, 1,  linkNameJson->valuestring, -1, NULL);
				sqlite3_bind_text(stmt_0_2, 2,  districtJson->valuestring, -1, NULL);
				sqlite3_bind_text(stmt_0_2, 3,  time_0, -1, NULL);

				rc = sqlite3_step(stmt_0_2);   
            	if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            	{
            	    M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            	    if(rc == SQLITE_CORRUPT)
            	        exit(0);
            	}
		               	
			}
			/*linkage_trigger_table*/
			if(time_1 != NULL)
			{
				sqlite3_bind_text(stmt_1_2, 1,  linkNameJson->valuestring, -1, NULL);
				sqlite3_bind_text(stmt_1_2, 2,  districtJson->valuestring, -1, NULL);
				sqlite3_bind_text(stmt_1_2, 3,  time_1, -1, NULL);

				rc = sqlite3_step(stmt_1_2);   
            	if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            	{
            	    M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            	    if(rc == SQLITE_CORRUPT)
            	        exit(0);
            	}
		               	
			}

			/*linkage_exec_table*/
			if(time_2 != NULL)
			{
				sqlite3_bind_text(stmt_2_2, 1,  linkNameJson->valuestring, -1, NULL);
				sqlite3_bind_text(stmt_2_2, 2,  districtJson->valuestring, -1, NULL);
				sqlite3_bind_text(stmt_2_2, 3,  time_2, -1, NULL);

				rc = sqlite3_step(stmt_2_2);   
            	if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            	{
            	    M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            	    if(rc == SQLITE_CORRUPT)
            	        exit(0);
            	}
		               	
			}
			
		}

    }
    else
    {
        M1_LOG_WARN("BEGIN IMMEDIATE errorMsg:%s",errorMsg);
        sqlite3_free(errorMsg);
    }

    Finish:

    rc = sql_commit(db);
    if(rc == SQLITE_OK)
    {
        M1_LOG_DEBUG("COMMIT OK\n");
    }

    if(stmt)
    	sqlite3_finalize(stmt);
    if(stmt_1)
    	sqlite3_finalize(stmt_1);
    if(stmt_2)
    	sqlite3_finalize(stmt_2);
    if(stmt_0_1)
    	sqlite3_finalize(stmt_0_1);
    if(stmt_1_1)
    	sqlite3_finalize(stmt_1_1);
    if(stmt_2_1)
    	sqlite3_finalize(stmt_2_1);
    if(stmt_0_2)
    	sqlite3_finalize(stmt_0_2);
    if(stmt_1_2)
    	sqlite3_finalize(stmt_1_2);
    if(stmt_2_2)
    	sqlite3_finalize(stmt_2_2);

    return ret;
}

void linkage_task(void)
{
	int rc             = 0;
	int rc1            = 0;
	uint32_t rowid     = 0;
	char *exec_type    = NULL;
	char *exec_id      = NULL;
	char *link_name    = NULL;
	char *sql          = NULL;
    sqlite3_stmt *stmt = NULL;

    rc1 = fifo_read(&link_exec_fifo, &rowid);
    if(rc1 > 0)
    { 
		rc = sql_open();
		if(rc){  
		    M1_LOG_ERROR( "Can't open database: %s\n", sqlite3_errmsg(db));  
		    return;
		}else{  
		    M1_LOG_DEBUG( "Opened database successfully\n");  
		}

		sql = "select EXEC_TYPE, EXEC_ID, LINK_NAME from linkage_table where rowid = ? and ENABLE = ?;";
		M1_LOG_DEBUG("sql:%s\n", sql);
		if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
		{
    	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    	    goto Finish; 
    	}

	   	do{
	   		M1_LOG_DEBUG("linkage_task\n");
			sqlite3_bind_int(stmt, 1, rowid);
			sqlite3_bind_text(stmt, 2, "on", -1, NULL);
			rc = sqlite3_step(stmt);     
            if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
                if(rc == SQLITE_CORRUPT)
                    exit(0);
            }
			if(rc == SQLITE_ROW)
			{
				exec_type = sqlite3_column_text(stmt,0);
				exec_id = sqlite3_column_text(stmt,1);
				link_name = sqlite3_column_text(stmt,2);
				if(strcmp(exec_type, "scenario") == 0){
					scenario_exec(exec_id, db);
				}else{
					device_exec(link_name, db);			
				}
			}
			sqlite3_reset(stmt);
			sqlite3_clear_bindings(stmt);

	   		rc1 = fifo_read(&link_exec_fifo, &rowid);
	   	}while(rc1 > 0);

		Finish:
		if(stmt)
			sqlite3_finalize(stmt);
		sql_close();
	}

}

static char* linkage_status(char* condition, int threshold, int value)
{	
	char* status = "OFF";

	if (0 == strcmp(condition, "<="))
	{ 
		if(value <= threshold)
			status = "ON";
	}
	else if(strcmp(condition, "=") == 0)
	{
		if(value == threshold)
			status = "ON";
	}
	else
	{
		if(value >= threshold)
			status = "ON";
	}
	M1_LOG_DEBUG("linkage_status:%s, value:%d, threshold:%d\n",status,value, threshold);

	return status;
}

static void linkage_check(sqlite3* db, char* link_name)
{
	M1_LOG_DEBUG("linkage_check\n");
	int link_flag        = 1;
	int rc               = 0;
	int rowid            = 0;
	char *status         = NULL;
	char *logical        = NULL;
	char* errorMsg       = NULL;
	char* sql            = NULL;
	char* sql_1          = NULL;
	char* sql_2          = NULL;
	sqlite3_stmt* stmt   = NULL;
	sqlite3_stmt* stmt_1 = NULL;
	sqlite3_stmt* stmt_2 = NULL;

	sql = "select STATUS, LOGICAL from link_trigger_table where LINK_NAME = ?;";
	M1_LOG_DEBUG("sql:%s\n",sql);

	if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        goto Finish; 
    }

    sqlite3_bind_text(stmt, 1, link_name, -1, NULL);
	while(sqlite3_step(stmt) == SQLITE_ROW)
	{
		status = sqlite3_column_text(stmt,0);
		logical = sqlite3_column_text(stmt,1);
		if((strcmp(logical, "&") == 0))
		{
			if((strcmp(status, "OFF") == 0))
			{
				link_flag = 0;
				break;
			}
		}
		else
		{
			if((strcmp(status, "ON") == 0))
			{
				link_flag = 1;
				break;
			}
		}
		sqlite3_reset(stmt);
		sqlite3_clear_bindings(stmt);
	}

	if(stmt)
	{
		sqlite3_finalize(stmt);
		stmt = NULL;
	}

	M1_LOG_DEBUG("link_flag:%d\n",link_flag);

	if(link_flag)
	{
		//if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
		//{
			/*执行设备*/
			sql_1 = "update linkage_table set STATUS = ? where LINK_NAME = ?;";
		    M1_LOG_DEBUG("sql_1:%s\n",sql_1);	
		    if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
		    {
	            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	            goto Finish; 
	        }

	    	sqlite3_bind_text(stmt_1, 1, "ON", -1, NULL);
	    	sqlite3_bind_text(stmt_1, 2, link_name, -1, NULL);
			rc = sqlite3_step(stmt_1);   
            if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
                if(rc == SQLITE_CORRUPT)
                    exit(0);
            }
			
			if(stmt_1)
			{
				sqlite3_finalize(stmt_1);
				stmt_1 = NULL;
			}
			
			// rc = sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg);
   //      	if(rc == SQLITE_OK)
   //      	{
   //      	    M1_LOG_DEBUG("COMMIT OK\n");
   //      	}
   //      	else if(rc == SQLITE_BUSY)
   //      	{
   //      	    M1_LOG_WARN("等待再次提交\n");
   //      	}
   //      	else
   //      	{
   //      	    M1_LOG_WARN("COMMIT errorMsg:%s\n",errorMsg);
   //      	    sqlite3_free(errorMsg);
   //      	}

   //  	}
   //  	else
   //  	{
   //  	    M1_LOG_WARN("BEGIN IMMEDIATE errorMsg:%s",errorMsg);
   //  	    sqlite3_free(errorMsg);
    	// }

		sql_2 = "select rowid from linkage_table where LINK_NAME = ?";
		M1_LOG_DEBUG("sql_2:%s\n",sql_2);
		if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK)
		{
    	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    	    goto Finish; 
    	}
    	sqlite3_bind_text(stmt_2, 1, link_name, -1, NULL);
		rc = sqlite3_step(stmt_2);  
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            if(rc == SQLITE_CORRUPT)
                exit(0);
        }
		
		if(rc == SQLITE_ROW)
		{
			rowid = sqlite3_column_int(stmt_2,0);
			M1_LOG_DEBUG("rowid:%d\n",rowid);
			fifo_write(&link_exec_fifo, rowid);
		}

		if(stmt_2)
		{
			sqlite3_finalize(stmt_2);
			stmt_2 = NULL;
		}
	}

	Finish:
	if(stmt)
		sqlite3_finalize(stmt);
	if(stmt_1)
		sqlite3_finalize(stmt_1);
	if(stmt_2)
		sqlite3_finalize(stmt_2);

}

int trigger_cb_handle(sqlite3* db)
{
	int rc               = 0;
	int rc1              = 0;
	int value            = 0;
	int param_type       = 0;
	int threshold        = 0;
	uint32_t rowid       = 0;
	char *devId          = NULL;
	char *condition      = NULL;
	char *status         = NULL;
	char *link_name      = NULL;
	char *errorMsg       = NULL;
	char *sql            = NULL;
	char *sql_1          = NULL;
	char *sql_2          = NULL;
	char *sql_3          = NULL;
	sqlite3_stmt *stmt   = NULL;
	sqlite3_stmt *stmt_1 = NULL;
	sqlite3_stmt *stmt_2 = NULL;
	sqlite3_stmt *stmt_3 = NULL;

   	rc1 = fifo_read(&dev_data_fifo, &rowid);
   	if(rc1 > 0)
   	{
	   	M1_LOG_DEBUG("trigger_cb_handle\n");

	   	/*失使能表更新回调*/
	   	rc = sqlite3_update_hook(db, NULL, NULL);
	    if(rc){
	        M1_LOG_DEBUG( "sqlite3_update_hook falied: %s\n", sqlite3_errmsg(db));  
	    }
	    /*check linkage table*/
	    {
	    	//sql = "select VALUE, DEV_ID, TYPE from param_table where rowid = ?;";
	    	sql = "select a.VALUE,a.DEV_ID,a.TYPE from param_table as a,link_trigger_table as b where a.TYPE = b.TYPE and a.rowid = ? limit 1;";
	    	M1_LOG_DEBUG("sql:%s\n",sql);
	    	if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	    	{
    		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    		    goto Finish; 
    		}
	    }
	    /*检查设备启/停状态*/
	    {
	    	sql_1 = "select STATUS from all_dev where DEV_ID = ? order by ID desc limit 1;";
	    	M1_LOG_DEBUG("sql_1:%s\n",sql_1);
	    	if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
	    	{
    		    M1_LOG_ERROR("sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    		    goto Finish; 
    		}
	    }
	    /*get linkage table*/
	    {
	    	sql_2 = "select THRESHOLD,CONDITION,LINK_NAME from link_trigger_table where DEV_ID = ? and TYPE = ?";
	    	M1_LOG_DEBUG("sql_2:%s\n",sql_2);
	    	if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK)
	    	{
    		    M1_LOG_ERROR("sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    		    goto Finish; 
    		}
	    }
	    /*set linkage table*/
	    {
	    	sql_3 = "update link_trigger_table set STATUS = ? where DEV_ID = ? and TYPE = ? and LINK_NAME = ? ;";
	    	M1_LOG_DEBUG("sql_3:%s\n",sql_3);
	    	if(sqlite3_prepare_v2(db, sql_3, strlen(sql_3), &stmt_3, NULL) != SQLITE_OK)
	    	{
    		    M1_LOG_ERROR("sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    		    goto Finish; 
    		}	
	    }

	    do{
	    	/*check linkage table*/
			sqlite3_bind_int(stmt, 1, rowid);
			rc = sqlite3_step(stmt);     
            if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
                if(rc == SQLITE_CORRUPT)
                    exit(0);
            }
			if(rc == SQLITE_ROW)
			{
			    value = sqlite3_column_int(stmt,0);
			    devId = sqlite3_column_text(stmt,1);
			    param_type = sqlite3_column_int(stmt,2);
			    M1_LOG_DEBUG("value:%05d, devId:%s, param_type%05d:\n",value, devId, param_type);
			 	
			 	/*检查设备启/停状态*/
    			sqlite3_bind_text(stmt_1, 1, devId, -1, NULL);
			 	rc = sqlite3_step(stmt_1);   
            	if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            	{
            	    M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            	    if(rc == SQLITE_CORRUPT)
            	        exit(0);
            	}
				if(rc == SQLITE_ROW)
				{		
			    	status = sqlite3_column_text(stmt_1,0);
				    /*设备处于启动状态*/
				    if(strcmp(status,"ON") == 0)
				    {
				    	if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    					{
						    /*get linkage table*/
	    					sqlite3_bind_text(stmt_2, 1, devId, -1, NULL);
	    					sqlite3_bind_int(stmt_2, 2, param_type);
							while(sqlite3_step(stmt_2) == SQLITE_ROW)
							{
									threshold = sqlite3_column_int(stmt_2,0);
							        condition = sqlite3_column_text(stmt_2,1);
							        link_name = sqlite3_column_text(stmt_2,2);
							        M1_LOG_DEBUG("threshold:%05d, condition:%s\n, link_name:%s\n", threshold, condition, link_name);
							 		status = linkage_status(condition, threshold, value);
							 		/*set linkage table*/
		    						sqlite3_bind_text(stmt_3, 1, status, -1, NULL);
		    						sqlite3_bind_text(stmt_3, 2, devId, -1, NULL);
		    						sqlite3_bind_int(stmt_3, 3, param_type);
		    						sqlite3_bind_text(stmt_3, 4, link_name, -1, NULL);
									rc = sqlite3_step(stmt_3);    
            						if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            						{
            						    M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            						    if(rc == SQLITE_CORRUPT)
            						        exit(0);
            						}
									/*检查是否满足触发条件*/
									linkage_check(db, link_name);
						 		
							 		sqlite3_reset(stmt_3);
				 					sqlite3_clear_bindings(stmt_3);

			 					
					 		}
					 		sqlite3_reset(stmt_2);
		 					sqlite3_clear_bindings(stmt_2);
		 					
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
				 	}
			 	}
			 	sqlite3_reset(stmt_1);
		 		sqlite3_clear_bindings(stmt_1);	
		 	}
		 	sqlite3_reset(stmt);
		 	sqlite3_clear_bindings(stmt);

	    	rc1 = fifo_read(&dev_data_fifo, &rowid);
	    }while(rc1 > 0);

		Finish:
		if(stmt)
			sqlite3_finalize(stmt);
		if(stmt_1)
			sqlite3_finalize(stmt_1);
		if(stmt_2)
			sqlite3_finalize(stmt_2);
		if(stmt_3)
			sqlite3_finalize(stmt_3);

		M1_LOG_DEBUG("trigger_cb_handle end\n");
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
	int rc                   = 0;
	int ret                  = M1_PROTOCOL_OK;
	int i                    = 0;
	int type                 = 0;
	int value                = 0;
	int delay                = 0;
	int pId                  = 0;
	int pduType              = TYPE_M1_REPORT_LINK_INFO;
    char* link_name          = NULL;
    char *district           = NULL;
    char *logical            = NULL;
    char *exec_type          = NULL;
    char *exec_id            = NULL;
    char *enable             = NULL;
    char *ap_id              = NULL;
    char *dev_id             = NULL;
    char *condition          = NULL;
    char *dev_name           = NULL;
	/*Json*/
	cJSON * pJsonRoot        = NULL;
    cJSON * pduJsonObject    = NULL;
    cJSON * devDataJsonArray = NULL;
    cJSON*  devDataObject    = NULL;
    cJSON * triggerJsonArray = NULL;
    cJSON*  triggerObject    = NULL;
    cJSON * execJsonArray    = NULL;
    cJSON*  execObject       = NULL;
    cJSON * paramJsonArray   = NULL;
    cJSON*  paramObject      = NULL;
    cJSON* delayArrayObject  = NULL;
    cJSON* delayObject       = NULL;
    /*sql*/
    char* sql                = NULL;
    char* sql_1              = NULL;
    char* sql_2              = NULL;
    char* sql_3              = NULL;
    char* sql_4              = NULL;
    char* sql_5              = NULL;
    char* sql_6              = NULL;
    char* sql_7              = NULL;
    char* sql_8              = NULL;
    char* sql_9              = NULL;
    sqlite3* db              = NULL;
    sqlite3_stmt *stmt       = NULL;
    sqlite3_stmt *stmt_1     = NULL;
    sqlite3_stmt *stmt_2     = NULL;
    sqlite3_stmt *stmt_3     = NULL;
    sqlite3_stmt *stmt_4     = NULL;
    sqlite3_stmt *stmt_5     = NULL;
    sqlite3_stmt *stmt_6     = NULL;
    sqlite3_stmt *stmt_7     = NULL;
    sqlite3_stmt *stmt_8     = NULL;
    sqlite3_stmt *stmt_9     = NULL;

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

    {
	    sql = "select LINK_NAME, DISTRICT, EXEC_TYPE, EXEC_ID, ENABLE from linkage_table;";
	    M1_LOG_DEBUG("sql:%s\n", sql);
	    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	    {
	        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = M1_PROTOCOL_FAILED;
	        goto Finish; 
	    }
	}
	/*获取logical*/
	{
		sql_1 = "select LOGICAL from link_trigger_table where LINK_NAME = ? order by ID desc limit 1;";
		M1_LOG_DEBUG("sql_1:%s\n", sql_1);
		if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
	    {
	        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = M1_PROTOCOL_FAILED;
	        goto Finish; 
	    }	
	}
	/*获取dev_id*/
	{
		sql_2 = "select DISTINCT DEV_ID from link_trigger_table where LINK_NAME = ?;";
		M1_LOG_DEBUG("sql_2:%s\n", sql_2);
		if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK)
	    {
	        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = M1_PROTOCOL_FAILED;
	        goto Finish; 
	    }
	}
	/*获取AP_ID*/
	{
		sql_3 = "select AP_ID from link_trigger_table where LINK_NAME = ? and DEV_ID = ? order by ID desc limit 1;";
		M1_LOG_DEBUG("sql_3:%s\n", sql_3);
		if(sqlite3_prepare_v2(db, sql_3, strlen(sql_3), &stmt_3, NULL) != SQLITE_OK)
	    {
	        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = M1_PROTOCOL_FAILED;
	        goto Finish; 
	    }
	}
	/*获取设备名称*/
	{
		sql_4 = "select DEV_NAME, PID from all_dev where DEV_ID = ? order by ID desc limit 1;";
		M1_LOG_DEBUG("sql_4:%s\n", sql_4);
		if(sqlite3_prepare_v2(db, sql_4, strlen(sql_4), &stmt_4, NULL) != SQLITE_OK)
	    {
	        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = M1_PROTOCOL_FAILED;
	        goto Finish; 
	    }
	}
	/*获取参数*/
	{
		sql_5 = "select TYPE, THRESHOLD, CONDITION from link_trigger_table where LINK_NAME = ? and DEV_ID = ?;";
		M1_LOG_DEBUG("sql_5:%s\n", sql_5);
		if(sqlite3_prepare_v2(db, sql_5, strlen(sql_5), &stmt_5, NULL) != SQLITE_OK)
	    {
	        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = M1_PROTOCOL_FAILED;
	        goto Finish; 
	    }
	}
	/*获取执行设备信息*/
	{
		sql_6 = "select DISTINCT DEV_ID from link_exec_table where LINK_NAME = ? order by ID asc;";
		M1_LOG_DEBUG("sql_6:%s\n", sql_6);
		if(sqlite3_prepare_v2(db, sql_6, strlen(sql_6), &stmt_6, NULL) != SQLITE_OK)
	    {
	        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = M1_PROTOCOL_FAILED;
	        goto Finish; 
	    }
	}
	/*获取AP_ID*/
	{
		sql_7 = "select AP_ID,DELAY from link_exec_table where LINK_NAME = ? and DEV_ID = ? order by ID desc limit 1;";
		M1_LOG_DEBUG("sql_7:%s\n", sql_7);
		if(sqlite3_prepare_v2(db, sql_7, strlen(sql_7), &stmt_7, NULL) != SQLITE_OK)
	    {
	        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = M1_PROTOCOL_FAILED;
	        goto Finish; 
	    }
	}
	/*获取设备名称*/
	{
		sql_8 = "select DEV_NAME, PID from all_dev where DEV_ID = ? order by ID desc limit 1;";
		M1_LOG_DEBUG("sql_8:%s\n", sql_8);
		if(sqlite3_prepare_v2(db, sql_8, strlen(sql_8), &stmt_8, NULL) != SQLITE_OK)
	    {
	        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = M1_PROTOCOL_FAILED;
	        goto Finish; 
	    }	
	}
	/*获取设备参数*/
	{
		sql_9 = "select TYPE, VALUE from link_exec_table where LINK_NAME = ? and DEV_ID = ?;";
		M1_LOG_DEBUG("sql_9:%s\n", sql_9);
		if(sqlite3_prepare_v2(db, sql_9, strlen(sql_9), &stmt_9, NULL) != SQLITE_OK)
	    {
	        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = M1_PROTOCOL_FAILED;
	        goto Finish; 
	    }
	}

    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
    	M1_LOG_DEBUG("1\n");
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
    	sqlite3_bind_text(stmt_1, 1, link_name, -1, NULL);
    	rc = sqlite3_step(stmt_1); 
		if(rc == SQLITE_ROW)
		{
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
 	    
    	sqlite3_bind_text(stmt_2, 1, link_name, -1, NULL);
    	while(sqlite3_step(stmt_2) == SQLITE_ROW)
    	{
    		triggerObject = cJSON_CreateObject();
		    if(NULL == triggerObject)
		    {
		        cJSON_Delete(triggerObject);
		        ret =  M1_PROTOCOL_FAILED;
        		goto Finish;
		    }
		    cJSON_AddItemToArray(triggerJsonArray, triggerObject);
    		dev_id = sqlite3_column_text(stmt_2, 0);
    		cJSON_AddStringToObject(triggerObject, "devId", dev_id);

    		/*获取AP_ID*/
	    	sqlite3_bind_text(stmt_3, 1, link_name, -1, NULL);
	    	sqlite3_bind_text(stmt_3, 2, dev_id, -1, NULL);
	    	rc = sqlite3_step(stmt_3); 
            if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
                if(rc == SQLITE_CORRUPT)
                    exit(0);
            }
			if(rc == SQLITE_ROW)
			{
			   	ap_id = sqlite3_column_text(stmt_3, 0);
			   	cJSON_AddStringToObject(triggerObject, "apId", ap_id);
			   	M1_LOG_DEBUG("apId:%s\n",ap_id);
		   	}

		   	/*获取设备名称*/
	    	sqlite3_bind_text(stmt_4, 1, dev_id, -1, NULL);
	    	rc = sqlite3_step(stmt_4); 
            if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
                if(rc == SQLITE_CORRUPT)
                    exit(0);
            }
			if(rc == SQLITE_ROW)
			{ 
			   	dev_name = sqlite3_column_text(stmt_4, 0);
			   	pId = sqlite3_column_int(stmt_4, 1);
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

	    	sqlite3_bind_text(stmt_5, 1, link_name, -1, NULL);
	    	sqlite3_bind_text(stmt_5, 2, dev_id, -1, NULL);
		   	while(sqlite3_step(stmt_5) == SQLITE_ROW)
		   	{
			    paramObject = cJSON_CreateObject();
			    if(NULL == paramObject)
			    {
			        cJSON_Delete(paramObject);
			        ret =  M1_PROTOCOL_FAILED;
        			goto Finish;
			    }
			    cJSON_AddItemToArray(paramJsonArray, paramObject);
				type = sqlite3_column_int(stmt_5, 0);
			   	cJSON_AddNumberToObject(paramObject, "type", type);
			   	M1_LOG_DEBUG("type:%05d\n");
			   	value = sqlite3_column_int(stmt_5, 1);
			   	cJSON_AddNumberToObject(paramObject, "value", value);
			   	M1_LOG_DEBUG("value:%05d\n",value);
			   	condition = sqlite3_column_text(stmt_5, 2);
			   	cJSON_AddStringToObject(paramObject, "condition", condition);
			   	M1_LOG_DEBUG("condition:%s\n",condition);
		   	}
		   	sqlite3_reset(stmt_3);
    		sqlite3_clear_bindings(stmt_3);
    		sqlite3_reset(stmt_4);
    		sqlite3_clear_bindings(stmt_4);
    		sqlite3_reset(stmt_5);
    		sqlite3_clear_bindings(stmt_5);
    	}
    	sqlite3_reset(stmt_1);
    	sqlite3_clear_bindings(stmt_1);
    	sqlite3_reset(stmt_2);
    	sqlite3_clear_bindings(stmt_2);

    	execJsonArray = cJSON_CreateArray();
		if(NULL == execJsonArray)
		{
		    cJSON_Delete(execJsonArray);
		    ret =  M1_PROTOCOL_FAILED;
        	goto Finish;
		}
    	if(strcmp(exec_type,"scenario") != 0)
    	{
    		/*获取执行设备信息*/
		    cJSON_AddItemToObject(devDataObject, "exeDev", execJsonArray);

	    	sqlite3_bind_text(stmt_6, 1, link_name, -1, NULL);
	    	while(sqlite3_step(stmt_6) == SQLITE_ROW)
	    	{
	    		execObject = cJSON_CreateObject();
			    if(NULL == execObject)
			    {
			        cJSON_Delete(execObject);
			        ret =  M1_PROTOCOL_FAILED;
        			goto Finish;
			    }
			    cJSON_AddItemToArray(execJsonArray, execObject);
	    		dev_id = sqlite3_column_text(stmt_6, 0);
	    		cJSON_AddStringToObject(execObject, "devId", dev_id);

	    		/*获取AP_ID*/
	    		sqlite3_bind_text(stmt_7, 1, link_name, -1, NULL);
	    		sqlite3_bind_text(stmt_7, 2, dev_id, -1, NULL);
		    	rc = sqlite3_step(stmt_7);   
            	if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            	{
            	    M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            	    if(rc == SQLITE_CORRUPT)
            	        exit(0);
            	} 
				if(rc == SQLITE_ROW)
				{
				   	ap_id = sqlite3_column_text(stmt_7, 0);
				   	cJSON_AddStringToObject(execObject, "apId", ap_id);
				   	M1_LOG_DEBUG("apId:%s\n",ap_id);
				   	delay = sqlite3_column_int(stmt_7, 1);
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
	    		sqlite3_bind_text(stmt_8, 1, dev_id, -1, NULL);
		    	rc = sqlite3_step(stmt_8);     
            	if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            	{
            	    M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            	    if(rc == SQLITE_CORRUPT)
            	        exit(0);
            	}
				if(rc == SQLITE_ROW)
				{
				   	dev_name = sqlite3_column_text(stmt_8, 0);
				   	cJSON_AddStringToObject(execObject, "devName", dev_name);
				   	pId = sqlite3_column_int(stmt_8, 1);
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
	 	    	/*获取参数*/
	    		sqlite3_bind_text(stmt_9, 1, link_name, -1, NULL);
	    		sqlite3_bind_text(stmt_9, 2, dev_id, -1, NULL);
			   	while(sqlite3_step(stmt_9) == SQLITE_ROW)
			   	{
				    paramObject = cJSON_CreateObject();
				    if(NULL == paramObject)
				    {
				        cJSON_Delete(paramObject);
				        ret =  M1_PROTOCOL_FAILED;
        				goto Finish;
				    }
				    cJSON_AddItemToArray(paramJsonArray, paramObject);
					type = sqlite3_column_int(stmt_9, 0);
				   	cJSON_AddNumberToObject(paramObject, "type", type);
				   	M1_LOG_DEBUG("type:%05d\n");
				   	value = sqlite3_column_int(stmt_9, 1);
				   	cJSON_AddNumberToObject(paramObject, "value", value);
				   	M1_LOG_DEBUG("value:%05d\n",value);
			   	}
				sqlite3_reset(stmt_7);
	    		sqlite3_clear_bindings(stmt_7);
	    		sqlite3_reset(stmt_8);
	    		sqlite3_clear_bindings(stmt_8);			   	
	    		sqlite3_reset(stmt_9);
	    		sqlite3_clear_bindings(stmt_9);			   	
	    	}

	    	sqlite3_reset(stmt_6);
	    	sqlite3_clear_bindings(stmt_6);
    	}
    	else
    	{
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
 	if(stmt_5)
 		sqlite3_finalize(stmt_5);
 	if(stmt_6)
 		sqlite3_finalize(stmt_6);
 	if(stmt_7)
 		sqlite3_finalize(stmt_7);
 	if(stmt_8)
 		sqlite3_finalize(stmt_8);
 	if(stmt_9)
 		sqlite3_finalize(stmt_9);
 	
    cJSON_Delete(pJsonRoot);

    return ret;
}

int app_linkage_enable(payload_t data)
{
	M1_LOG_DEBUG( "app_linkage_enable\n");
	int rc              = 0;
	int ret             = M1_PROTOCOL_OK;
	char* errorMsg      = NULL;
	char* sql           = NULL;
	sqlite3* db         = NULL;
    sqlite3_stmt* stmt  = NULL;
	cJSON* linkObject   = NULL;
	cJSON* enableObject = NULL;

    /*获取数据库*/
    db = data.db;
	linkObject = cJSON_GetObjectItem(data.pdu, "linkName");
	if(linkObject == NULL)
	{
		ret = M1_PROTOCOL_FAILED;
		goto Finish;
	}

	M1_LOG_DEBUG("link_name:%s\n",linkObject->valuestring);
	enableObject = cJSON_GetObjectItem(data.pdu, "enable");
	if(enableObject == NULL)
	{
		ret = M1_PROTOCOL_FAILED;
		goto Finish;
	}
	M1_LOG_DEBUG("enable:%s\n",enableObject->valuestring);

	sql = "update linkage_table set ENABLE = ? where LINK_NAME = ?;";
	M1_LOG_DEBUG("sql:%s\n", sql);
  	if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
  	{
	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	    ret = M1_PROTOCOL_FAILED;
	    goto Finish; 
	}

	if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
		sqlite3_bind_text(stmt, 1, enableObject->valuestring, -1, NULL);
		sqlite3_bind_text(stmt, 2, linkObject->valuestring, -1, NULL);
	  	while(sqlite3_step(stmt) == SQLITE_ROW);
		
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


