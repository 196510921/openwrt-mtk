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

extern sqlite3* db;
/*函数定义*************************************************************************************/
static int scenario_alarm_check(scen_alarm_t alarm_time);

/*场景执行*/
int scenario_exec(char* data, sqlite3* db)
{
	cJSON *pJsonRoot        = NULL; 
    cJSON *pduJsonObject    = NULL;
    cJSON *devDataJsonArray = NULL;
    cJSON *devDataObject    = NULL;
    cJSON *paramArray       = NULL;
    cJSON *paramObject      = NULL;
    char *p                 = NULL;
	char *ap_id             = NULL;
	char *dev_id            = NULL;
	char *status            = NULL;
	int type                = 0;
	int value               = 0;
	int delay               = 0;
	char *sql               = NULL;
	char *sql_1             = NULL;
	char *sql_2             = NULL;
	char *sql_3             = NULL;
	char *sql_4             = NULL;
	sqlite3_stmt *stmt      = NULL;
	sqlite3_stmt *stmt_1    = NULL;
	sqlite3_stmt *stmt_2    = NULL;
	sqlite3_stmt *stmt_3    = NULL;
	sqlite3_stmt *stmt_4    = NULL;
 
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
    /*查询AP_ID*/
    {
	    sql = "select distinct AP_ID from scenario_table where SCEN_NAME = ?;";
		M1_LOG_DEBUG("sql:%s\n",sql);
		if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
		{
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = M1_PROTOCOL_FAILED;
		    goto Finish; 
		}
	}
	/*查询DEV_ID*/
	{
		sql_1 = "select distinct DEV_ID from scenario_table where SCEN_NAME = ? and AP_ID = ?;";
		M1_LOG_DEBUG("sql_1:%s\n",sql_1);
		if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
		{
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = M1_PROTOCOL_FAILED;
		    goto Finish; 
		}
	}
	/*查询status*/
	{
		sql_2 = "select STATUS from all_dev where DEV_ID = ?;";
		M1_LOG_DEBUG("sql_2:%s\n",sql_2);
		if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK)
		{
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = M1_PROTOCOL_FAILED;
		    goto Finish; 
		}
	}
	/*查询参数*/
	{
		sql_3 = "select TYPE, VALUE, DELAY from scenario_table where SCEN_NAME = ? and DEV_ID = ?;";
		M1_LOG_DEBUG("sql_3:%s\n",sql_3);
		if(sqlite3_prepare_v2(db, sql_3, strlen(sql_3), &stmt_3, NULL) != SQLITE_OK)
		{
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = M1_PROTOCOL_FAILED;
		    goto Finish; 
		}
	}
	/*查询clientFd*/
	{
		sql_4 = "select CLIENT_FD from conn_info where AP_ID = ?;";
		M1_LOG_DEBUG("sql_4:%s\n",sql_4);
		if(sqlite3_prepare_v2(db, sql_4, strlen(sql_4), &stmt_4, NULL) != SQLITE_OK)
		{
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = M1_PROTOCOL_FAILED;
		    goto Finish; 
		}
	}	

	sqlite3_bind_text(stmt, 1, data, -1, NULL);
	while(sqlite3_step(stmt) == SQLITE_ROW)
	{
		ap_id = sqlite3_column_text(stmt,0);
		
		sqlite3_bind_text(stmt_1, 1, data, -1, NULL);
		sqlite3_bind_text(stmt_1, 2, ap_id, -1, NULL);
		// /*单个子设备数据*/
		while(sqlite3_step(stmt_1) == SQLITE_ROW)
		{
			dev_id = sqlite3_column_text(stmt_1,0);
			/*检查设备启/停状态*/
			sqlite3_bind_text(stmt_2, 1, dev_id, -1, NULL);
		 	rc = sqlite3_step(stmt_2);
		   	if(rc == SQLITE_ROW)
		   	{
				status = sqlite3_column_text(stmt_2,0);
			}
			M1_LOG_DEBUG("status:%s\n",status);		
		    if(strcmp(status,"ON") == 0)
		    {
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

				sqlite3_bind_text(stmt_3, 1, data, -1, NULL);
				sqlite3_bind_text(stmt_3, 2, dev_id, -1, NULL);
				while(sqlite3_step(stmt_3) == SQLITE_ROW)
				{
					type = sqlite3_column_int(stmt_3,0);
					value = sqlite3_column_int(stmt_3,1);
					delay = sqlite3_column_int(stmt_3,2);
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
  			 	M1_LOG_INFO("p:%s\n",p);
		    	/*get clientfd*/
		    	sqlite3_bind_text(stmt_4, 1, ap_id, -1, NULL);
		    	rc = sqlite3_step(stmt_4);
		    	if(rc == SQLITE_ROW)
		    	{
					clientFd = sqlite3_column_int(stmt_4,0);
				}		
		    	
		    	M1_LOG_DEBUG("add msg to delay send\n");
		    	//delay_send(dup_data, delay, clientFd);
		    	delay_send(p, delay, clientFd);
		    	
		    	sqlite3_reset(stmt_3);
    			sqlite3_clear_bindings(stmt_3);
    			sqlite3_reset(stmt_4);
    			sqlite3_clear_bindings(stmt_4);
		    }
			sqlite3_reset(stmt_2);
    		sqlite3_clear_bindings(stmt_2);    
		}
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
   	if(stmt_4)
   		sqlite3_finalize(stmt_4);

	cJSON_Delete(pJsonRoot);
	return ret;
}

int scenario_create_handle(payload_t data)
{
	M1_LOG_DEBUG("scenario_create_handle\n");
	
	int i                 = 0;
	int j                 = 0;
	int delay             = 0;
	int rc                = 0;
	int ret               = M1_PROTOCOL_OK;
	int number1           = 0;
	int number2           = 0;
	char* time            = 0;
	char* sql             = NULL;
	char* sql_1           = NULL;
	char* sql_1_1         = NULL;
	char* sql_1_2         = NULL;
	char* errorMsg        = NULL;
	cJSON* scenNameJson   = NULL;
	cJSON* scenPicJson    = NULL;
	cJSON* districtJson   = NULL;
	cJSON* alarmJson      = NULL;
	cJSON* devJson        = NULL;
	cJSON* devArrayJson   = NULL;
	cJSON* apIdJson       = NULL;
	cJSON* devIdJson      = NULL;
	cJSON* paramJson      = NULL;
	cJSON* paramArrayJson = NULL;
	cJSON* typeJson       = NULL;
	cJSON* valueJson      = NULL;
	cJSON* delayArrayJson = NULL;
	cJSON* delayJson      = NULL;
	cJSON* hourJson       = NULL;
	cJSON* minutesJson    = NULL;
	cJSON* weekJson       = NULL;
	cJSON* statusJson     = NULL;
	sqlite3* db           = NULL;
	sqlite3_stmt* stmt    = NULL;
	sqlite3_stmt* stmt_1  = NULL;
	sqlite3_stmt* stmt_1_1= NULL;
	sqlite3_stmt* stmt_1_2= NULL;

	if(data.pdu == NULL)
	{
		ret = M1_PROTOCOL_FAILED;
		goto Finish;
	};

	/*获取数据库*/
	db = data.db;
    /*获取场景名称*/
    scenNameJson = cJSON_GetObjectItem(data.pdu, "scenName");
    if(scenNameJson == NULL)
    {
    	ret = M1_PROTOCOL_FAILED;
		goto Finish;	
    }
    M1_LOG_DEBUG("scenName:%s\n",scenNameJson->valuestring);
	/*获取场景图标*/
	scenPicJson = cJSON_GetObjectItem(data.pdu, "scenPic");
    if(scenPicJson == NULL)
    {
    	ret = M1_PROTOCOL_FAILED;
		goto Finish;	
    }
    M1_LOG_DEBUG("scenPic:%s\n",scenPicJson->valuestring);
	/*获取数据包中的alarm信息*/
	alarmJson = cJSON_GetObjectItem(data.pdu, "alarm");
	if(alarmJson == NULL)
	{
    	ret = M1_PROTOCOL_FAILED;
		goto Finish;	
    }
	/*将alarm信息存入alarm表中*/
	{
		sql = "insert or replace into scen_alarm_table(SCEN_NAME, HOUR, MINUTES, WEEK, STATUS) values(?,?,?,?,?);";
		M1_LOG_DEBUG("%s\n",sql);
		if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
		{
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = M1_PROTOCOL_FAILED;
		    goto Finish; 
		}
	}

	{
		sql_1 = "insert or replace into scenario_table(SCEN_NAME, SCEN_PIC, DISTRICT, AP_ID, DEV_ID, TYPE, VALUE, DELAY, ACCOUNT)\
		values(?,?,?,?,?,?,?,?,?);";
		M1_LOG_DEBUG("%s\n",sql_1);
		if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
		{
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = M1_PROTOCOL_FAILED;
		    goto Finish; 
		}
	}
	/*查询场景历史数据时间*/
	{
		sql_1_1 = "select TIME from scenario_table where SCEN_NAME = ? and DISTRICT = ? order by ID desc limit 1;";
	    M1_LOG_DEBUG("%s\n",sql_1_1);
	    if(sqlite3_prepare_v2(db, sql_1_1, strlen(sql_1_1), &stmt_1_1, NULL) != SQLITE_OK)
	    {
    	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    	    ret = M1_PROTOCOL_FAILED;
    	    goto Finish; 
    	}
	}
	/*删除场景无效历史数据*/
	{
		sql_1_2 = "delete from scenario_table where SCEN_NAME = ? and DISTRICT = ? and TIME = ?;";
	    M1_LOG_DEBUG("%s\n",sql_1_2);
	    if(sqlite3_prepare_v2(db, sql_1_2, strlen(sql_1_2), &stmt_1_2, NULL) != SQLITE_OK)
	    {
    	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    	    ret = M1_PROTOCOL_FAILED;
    	    goto Finish; 
    	}
	}

	/*事物开启*/
	if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
	{
        M1_LOG_DEBUG("BEGIN IMMEDIATE\n");
		if(alarmJson != NULL)
		{	
			/*获取收到数据包信息*/
		   	hourJson = cJSON_GetObjectItem(alarmJson, "hour");
		   	if(hourJson == NULL){
				ret = M1_PROTOCOL_FAILED;
				goto Finish;	   		
		   	}
		    M1_LOG_DEBUG("hour:%d\n",hourJson->valueint);
		    minutesJson = cJSON_GetObjectItem(alarmJson, "minutes");
		    if(minutesJson == NULL){
				ret = M1_PROTOCOL_FAILED;
				goto Finish;	   		
		   	}
		    M1_LOG_DEBUG("minutes:%d\n",minutesJson->valueint);
		    weekJson = cJSON_GetObjectItem(alarmJson, "week");
		    if(weekJson == NULL){
				ret = M1_PROTOCOL_FAILED;
				goto Finish;	   		
		   	}
		    M1_LOG_DEBUG("week:%s\n",weekJson->valuestring);
		    statusJson = cJSON_GetObjectItem(alarmJson, "status");
		    if(statusJson == NULL){
				ret = M1_PROTOCOL_FAILED;
				goto Finish;	   		
		   	}
		    M1_LOG_DEBUG("status:%s\n",statusJson->valuestring);
		
			sqlite3_bind_text(stmt, 1, scenNameJson->valuestring, -1, NULL);
			sqlite3_bind_int(stmt, 2, hourJson->valueint);
			sqlite3_bind_int(stmt, 3, minutesJson->valueint);
			sqlite3_bind_text(stmt, 4, weekJson->valuestring, -1, NULL);
			sqlite3_bind_text(stmt, 5, statusJson->valuestring, -1, NULL);
			
			rc = sqlite3_step(stmt);
			if((rc != SQLITE_ROW) && (rc!= SQLITE_DONE)) 
				M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
		}
		
		/*获取联动表 id*/
	    districtJson = cJSON_GetObjectItem(data.pdu, "district");
	    if(districtJson == NULL){
			ret = M1_PROTOCOL_FAILED;
			goto Finish;	   		
		}
	    M1_LOG_DEBUG("district:%s\n",districtJson->valuestring);
	    devArrayJson = cJSON_GetObjectItem(data.pdu, "device");
	    number1 = cJSON_GetArraySize(devArrayJson);
	    M1_LOG_DEBUG("number1:%d\n",number1);
	    /*查询历史数据时间*/
	    {
			sqlite3_bind_text(stmt_1_1, 1,  scenNameJson->valuestring, -1, NULL);
			sqlite3_bind_text(stmt_1_1, 2,  districtJson->valuestring, -1, NULL);

			rc = sqlite3_step(stmt_1_1);
			M1_LOG_DEBUG("step() return %s, number:%03d\n",\
	            rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
	               
	        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
	        {
	            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
	        }
	        else
	        {
	        	time = sqlite3_column_text(stmt_1_1, 0);
	        }
		}

	    /*存取到数据表scenario_table中*/
	    for(i = 0; i < number1; i++)
	    {
	    	devJson = cJSON_GetArrayItem(devArrayJson, i);
	    	if(devJson == NULL)
	    	{
	    		M1_LOG_WARN("devJson NULL\n");
				ret = M1_PROTOCOL_FAILED;
				goto Finish;	   		
			}
	    	apIdJson = cJSON_GetObjectItem(devJson, "apId");
	    	if(apIdJson == NULL)
	    	{
	    		M1_LOG_WARN("apIdJson NULL\n");
				ret = M1_PROTOCOL_FAILED;
				goto Finish;	   		
			}
	    	devIdJson = cJSON_GetObjectItem(devJson, "devId");
	    	if(devIdJson == NULL)
	    	{
	    		M1_LOG_WARN("devIdJson NULL\n");
				ret = M1_PROTOCOL_FAILED;
				goto Finish;	   		
			}
	    	delayArrayJson = cJSON_GetObjectItem(devJson, "delay");
	    	if(delayArrayJson != NULL)
	    	{
				number2 = cJSON_GetArraySize(delayArrayJson);
		    	for(j = 0, delay = 0; j < number2; j++)
		    	{
		    		delayJson = cJSON_GetArrayItem(delayArrayJson, j);
		    		delay += delayJson->valueint;
		    	}
	    	}
	    	else
	    	{
	    		M1_LOG_WARN("delay NULL\n");
	    	}
	    	M1_LOG_DEBUG("apId:%s, devId:%s\n",apIdJson->valuestring, devIdJson->valuestring);
	    	paramArrayJson = cJSON_GetObjectItem(devJson, "param");
	    	number2 = cJSON_GetArraySize(paramArrayJson);
	    	for(j = 0; j < number2; j++)
	    	{
	    		paramJson = cJSON_GetArrayItem(paramArrayJson, j);
	    		if(paramJson == NULL)
	    		{
	    			M1_LOG_WARN("paramJson NULL\n");
					ret = M1_PROTOCOL_FAILED;
					goto Finish;	   		
				}
	    		typeJson = cJSON_GetObjectItem(paramJson, "type");
	    		if(typeJson == NULL)
	    		{
	    			M1_LOG_WARN("typeJson NULL\n");
					ret = M1_PROTOCOL_FAILED;
					goto Finish;	   		
				}
	    		valueJson = cJSON_GetObjectItem(paramJson, "value");
	    		if(valueJson == NULL)
	    		{
	    			M1_LOG_WARN("valueJson NULL\n");
					ret = M1_PROTOCOL_FAILED;
					goto Finish;	   		
				}
	    		M1_LOG_DEBUG("type:%d, value:%d\n",typeJson->valueint, valueJson->valueint);
	    		
				sqlite3_bind_text(stmt_1, 1, scenNameJson->valuestring, -1, NULL);
				sqlite3_bind_text(stmt_1, 2, scenPicJson->valuestring, -1, NULL);
				sqlite3_bind_text(stmt_1, 3, districtJson->valuestring, -1, NULL);
				sqlite3_bind_text(stmt_1, 4, apIdJson->valuestring, -1, NULL);
				sqlite3_bind_text(stmt_1, 5, devIdJson->valuestring, -1, NULL);
				sqlite3_bind_int(stmt_1, 6, typeJson->valueint);
				sqlite3_bind_int(stmt_1, 7, valueJson->valueint);
				sqlite3_bind_int(stmt_1, 8, delay);
				sqlite3_bind_text(stmt_1, 9, "Dalitek",-1,NULL);

				rc = sqlite3_step(stmt_1);
				if((rc != SQLITE_ROW) && (rc!= SQLITE_DONE))  
					M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);

				sqlite3_reset(stmt_1);
				sqlite3_clear_bindings(stmt_1);
	    	}	
	    }

	    if(time != NULL)
	    {
	    	sqlite3_bind_text(stmt_1_2, 1,  scenNameJson->valuestring, -1, NULL);
			sqlite3_bind_text(stmt_1_2, 2,  districtJson->valuestring, -1, NULL);
			sqlite3_bind_text(stmt_1_2, 3,  time, -1, NULL);

			rc = sqlite3_step(stmt_1_2);
			M1_LOG_DEBUG("step() return %s, number:%03d\n",\
		           rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
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
    if(stmt_1)
    	sqlite3_finalize(stmt_1);
     if(stmt_1_1)
    	sqlite3_finalize(stmt_1_1);
     if(stmt_1_2)
    	sqlite3_finalize(stmt_1_2);
    return ret;
    
}

int scenario_alarm_create_handle(payload_t data)
{
	M1_LOG_DEBUG("scenario_alarm_create_handle\n");
	int rc              = 0;
	int ret             = M1_PROTOCOL_OK;
	char* errorMsg      = NULL;
	cJSON* scenNameJson = NULL;
	cJSON* hourJson     = NULL;
	cJSON* minutesJson  = NULL;
	cJSON* weekJson     = NULL;
	cJSON* statusJson   = NULL;
	char* sql           = NULL;
	sqlite3* db         = NULL;
	sqlite3_stmt* stmt  = NULL;

	if(data.pdu == NULL)
	{
		ret = M1_PROTOCOL_FAILED;
	}
	/*获取数据库*/
	db = data.db;

	if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
	{
        M1_LOG_DEBUG("BEGIN IMMEDIATE\n");
		/*获取收到数据包信息*/
	    scenNameJson = cJSON_GetObjectItem(data.pdu, "scenarioName");
	    if(scenNameJson == NULL)
	    {
	    	M1_LOG_DEBUG("scenName NULL\n");
	    	ret = M1_PROTOCOL_FAILED;
		    goto Finish;
	    }
	    M1_LOG_DEBUG("scenName:%s\n",scenNameJson->valuestring);
	    hourJson = cJSON_GetObjectItem(data.pdu, "hour");
	    M1_LOG_DEBUG("hour:%d\n",hourJson->valueint);
	    minutesJson = cJSON_GetObjectItem(data.pdu, "minutes");
	    M1_LOG_DEBUG("minutes:%d\n",minutesJson->valueint);
	    weekJson = cJSON_GetObjectItem(data.pdu, "week");
	    if(weekJson == NULL)
	    {
	    	M1_LOG_DEBUG("weekJson NULL\n");
	    	ret = M1_PROTOCOL_FAILED;
		    goto Finish;
	    }
	    M1_LOG_DEBUG("week:%s\n",weekJson->valuestring);
	    statusJson = cJSON_GetObjectItem(data.pdu, "status");
	    if(statusJson == NULL)
	    {
	    	M1_LOG_DEBUG("statusJson NULL\n");
	    	ret = M1_PROTOCOL_FAILED;
		    goto Finish;
	    }
	    	M1_LOG_DEBUG("status:%s\n",statusJson->valuestring);

	    sql = "insert or replace into scen_alarm_table(SCEN_NAME, HOUR, MINUTES, WEEK, STATUS) values(?,?,?,?,?);";
	    M1_LOG_DEBUG("sql:%s\n",sql);
	  
	    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	    {
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = M1_PROTOCOL_FAILED;
		    goto Finish; 
		}
		
		sqlite3_bind_text(stmt, 1, scenNameJson->valuestring, -1, NULL);
		sqlite3_bind_int(stmt, 2, hourJson->valueint);
		sqlite3_bind_int(stmt, 3, minutesJson->valueint);
		sqlite3_bind_text(stmt, 4, weekJson->valuestring, -1, NULL);
		sqlite3_bind_text(stmt, 5, statusJson->valuestring, -1, NULL);
		
		rc = sqlite3_step(stmt);

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
    
    if(rc == SQLITE_ERROR)
    	ret =  M1_PROTOCOL_FAILED;

    Finish:
    if(stmt)
    	sqlite3_finalize(stmt);

    return ret;
}

int app_req_scenario(payload_t data)
{
	M1_LOG_DEBUG("app_req_scenario\n");
	/*cJSON*/
	int i                    = 0;
	int rc                   = 0;
	int ret                  = M1_PROTOCOL_OK;
    int pduType              = TYPE_M1_REPORT_SCEN_INFO;
    int hour                 = 0;
    int minutes              = 0;
    int delay                = 0;
    int pId                  = 0;
    int type                 = 0;
    int value                = 0;
    char* account            = NULL;
    char *scen_name          = NULL;
    char *district           = NULL;
    char *week               = NULL;
    char *alarm_status       = NULL;
   	char* ap_id              = NULL;
   	char *dev_id             = "devId";
   	char *dev_name           = NULL;
   	char *scen_pic           = NULL;
    cJSON * pJsonRoot        = NULL;
    cJSON * pduJsonObject    = NULL;
    cJSON * devDataJsonArray = NULL;
    cJSON*  devDataObject    = NULL;
    cJSON*  alarmObject      = NULL;
    cJSON*  deviceObject     = NULL;
    cJSON*  deviceArrayObject= NULL;
    cJSON*  paramArrayObject = NULL;
    cJSON*  paramObject      = NULL;
    cJSON*  delayArrayObject = NULL;
    cJSON*  delayObject      = NULL;
    char* sql                = NULL;
    char* sql_1              = NULL;
    char* sql_2              = NULL;
    char* sql_3              = NULL;
    char* sql_4              = NULL;
    char* sql_5              = NULL;
    char* sql_6              = NULL;
    char* sql_7              = NULL;
    sqlite3* db              = NULL;
    sqlite3_stmt *stmt       = NULL;
    sqlite3_stmt *stmt_1     = NULL;
    sqlite3_stmt *stmt_2     = NULL;
    sqlite3_stmt *stmt_3     = NULL;
    sqlite3_stmt *stmt_4     = NULL;
    sqlite3_stmt *stmt_5     = NULL;
    sqlite3_stmt *stmt_6     = NULL;
    sqlite3_stmt *stmt_7     = NULL;

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
    sql = "select ACCOUNT from account_info where CLIENT_FD = ? order by ID desc limit 1;";
    M1_LOG_DEBUG( "%s\n", sql);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
    {
	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	    ret = M1_PROTOCOL_FAILED;
	    goto Finish; 
	}
	sqlite3_bind_int(stmt, 1, data.clientFd);
    if(sqlite3_step(stmt) == SQLITE_ROW)
    {
        account =  sqlite3_column_text(stmt, 0);
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

    /*取场景名称*/
    {
    	sql_1 = "select distinct SCEN_NAME from scenario_table where ACCOUNT = ?";
	    M1_LOG_DEBUG("sql_1:%s\n",sql_1);
	    if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
	    {
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = M1_PROTOCOL_FAILED;
		    goto Finish; 
		}
    }
    /*根据场景名称选出隶属区域*/
    {
    	sql_2 = "select DISTRICT, SCEN_PIC from scenario_table where SCEN_NAME = ? order by ID desc limit 1;";
    	M1_LOG_DEBUG("sql_2:%s\n",sql_2);
	    if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK)
	    {
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = M1_PROTOCOL_FAILED;
		    goto Finish; 
		}
    }
    /*选择区域定时执行信息*/
    {
    	sql_3 = "select HOUR, MINUTES, WEEK, STATUS from scen_alarm_table where SCEN_NAME = ? order by ID desc limit 1;";
    	M1_LOG_DEBUG("sql_3:%s\n",sql_3);
	    if(sqlite3_prepare_v2(db, sql_3, strlen(sql_3), &stmt_3, NULL) != SQLITE_OK)
	    {
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = M1_PROTOCOL_FAILED;
		    goto Finish; 
		}
    }
    /*从场景表scenario_table中选出设备相关信息*/
    {
    	sql_4 = "select DISTINCT DEV_ID from scenario_table where SCEN_NAME = ? order by ID asc;";
    	M1_LOG_DEBUG("sql_4:%s\n",sql_4);
	    if(sqlite3_prepare_v2(db, sql_4, strlen(sql_4), &stmt_4, NULL) != SQLITE_OK)
	    {
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = M1_PROTOCOL_FAILED;
		    goto Finish; 
		}
    }
    /*获取AP_ID*/
    {
    	sql_5 = "select AP_ID, DELAY from scenario_table where SCEN_NAME = ? and DEV_ID = ? order by ID desc limit 1;";
    	M1_LOG_DEBUG("sql_5:%s\n",sql_5);
	    if(sqlite3_prepare_v2(db, sql_5, strlen(sql_5), &stmt_5, NULL) != SQLITE_OK)
	    {
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = M1_PROTOCOL_FAILED;
		    goto Finish; 
		}
    }
    /*获取设备名称*/
    {
    	sql_6 = "select DEV_NAME, PID from all_dev where DEV_ID = ? order by ID desc limit 1;";
    	M1_LOG_DEBUG("sql_6:%s\n",sql_6);
	    if(sqlite3_prepare_v2(db, sql_6, strlen(sql_6), &stmt_6, NULL) != SQLITE_OK)
	    {
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = M1_PROTOCOL_FAILED;
		    goto Finish; 
		}
    }

    {
    	sql_7 = "select TYPE, VALUE from scenario_table where SCEN_NAME = ? and DEV_ID = ? and ACCOUNT = ?;";
    	M1_LOG_DEBUG("sql_7:%s\n",sql_7);
	    if(sqlite3_prepare_v2(db, sql_7, strlen(sql_7), &stmt_7, NULL) != SQLITE_OK)
	    {
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = M1_PROTOCOL_FAILED;
		    goto Finish; 
		}
    }

	sqlite3_bind_text(stmt_1, 1, account, -1, NULL);
    while(sqlite3_step(stmt_1) == SQLITE_ROW)
    {
	    devDataObject = cJSON_CreateObject();
	    if(NULL == devDataObject)
	    {
	        cJSON_Delete(devDataObject);
	        ret = M1_PROTOCOL_FAILED;
        	goto Finish;
	    }
	    cJSON_AddItemToArray(devDataJsonArray, devDataObject);
	    scen_name = sqlite3_column_text(stmt_1, 0);
	    cJSON_AddStringToObject(devDataObject, "scenName", scen_name);
	    /*根据场景名称选出隶属区域*/
		sqlite3_bind_text(stmt_2, 1, scen_name, -1, NULL);
	    rc = sqlite3_step(stmt_2); 
		if(rc == SQLITE_ROW)
		{
			district = sqlite3_column_text(stmt_2, 0);
			if(district == NULL){
				ret = M1_PROTOCOL_FAILED;
        		goto Finish;
			}
			cJSON_AddStringToObject(devDataObject, "district", district);
			scen_pic = sqlite3_column_text(stmt_2, 1);
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

	    sqlite3_bind_text(stmt_3, 1, scen_name, -1, NULL);
	    rc = sqlite3_step(stmt_3); 
		if(rc == SQLITE_ROW)
		{
			hour = sqlite3_column_int(stmt_3, 0);
			cJSON_AddNumberToObject(alarmObject, "hour", hour);
			minutes = sqlite3_column_int(stmt_3, 1);
			cJSON_AddNumberToObject(alarmObject, "minutes", minutes);
			week = sqlite3_column_text(stmt_3, 2);
			cJSON_AddStringToObject(alarmObject, "week", week);
			alarm_status = sqlite3_column_text(stmt_3, 3);
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
		sqlite3_bind_text(stmt_4, 1, scen_name, -1, NULL);
	    while(sqlite3_step(stmt_4) == SQLITE_ROW)
	    {
		    deviceObject = cJSON_CreateObject();
		    if(NULL == deviceObject)
		    {
		        cJSON_Delete(deviceObject);
		        ret = M1_PROTOCOL_FAILED;
        		goto Finish;
		    }
		    cJSON_AddItemToArray(deviceArrayObject, deviceObject);
		    
		   	dev_id = sqlite3_column_text(stmt_4, 0);
		   	M1_LOG_DEBUG("devId:%s\n",dev_id);
		   	cJSON_AddStringToObject(deviceObject, "devId", dev_id);
		   	/*获取AP_ID*/
			sqlite3_bind_text(stmt_5, 1, scen_name, -1, NULL);
			sqlite3_bind_text(stmt_5, 6, dev_id, -1, NULL);
	    	rc = sqlite3_step(stmt_5); 
			if(rc == SQLITE_ROW)
			{
				ap_id = sqlite3_column_text(stmt_5, 0);
			   	cJSON_AddStringToObject(deviceObject, "apId", ap_id);
			   	M1_LOG_DEBUG("apId:%s\n",ap_id);
			   	delay = sqlite3_column_int(stmt_5, 1);
			   	
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
			   	for(i = 0; i < (delay / SCENARIO_DELAY_TOP); i++)
			   	{
					delayObject = cJSON_CreateNumber(SCENARIO_DELAY_TOP);
				    if(NULL == delayObject)
				    {
				        cJSON_Delete(delayObject);
				        ret = M1_PROTOCOL_FAILED;
        				goto Finish;
				    }
					cJSON_AddItemToArray(delayArrayObject, delayObject);
			   	}
			   	if((delay % SCENARIO_DELAY_TOP) > 0)
			   	{
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
			sqlite3_bind_text(stmt_6, 1, dev_id, -1, NULL);
	    	rc = sqlite3_step(stmt_6); 
			if(rc == SQLITE_ROW)
			{
				dev_name = sqlite3_column_text(stmt_6, 0);
				pId = sqlite3_column_int(stmt_6, 1);
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
			
			sqlite3_bind_text(stmt_7, 1, scen_name, -1, NULL);
			sqlite3_bind_text(stmt_7, 2, dev_id, -1, NULL);
			sqlite3_bind_text(stmt_7, 3, account, -1, NULL);
		   	while(sqlite3_step(stmt_7) == SQLITE_ROW)
		   	{
			    paramObject = cJSON_CreateObject();
			    if(NULL == paramObject)
			    {
			        cJSON_Delete(paramObject);
			        ret = M1_PROTOCOL_FAILED;
        			goto Finish;
			    }
			    cJSON_AddItemToArray(paramArrayObject, paramObject);
				type = sqlite3_column_int(stmt_7, 0);
			   	cJSON_AddNumberToObject(paramObject, "type", type);
			   	M1_LOG_DEBUG("type:%05d\n");
			   	value = sqlite3_column_int(stmt_7, 1);
			   	cJSON_AddNumberToObject(paramObject, "value", value);
			   	M1_LOG_DEBUG("value:%05d\n",value);
		   	}
			sqlite3_reset(stmt_5);
			sqlite3_clear_bindings(stmt_5);
			sqlite3_reset(stmt_6);
			sqlite3_clear_bindings(stmt_6);
			sqlite3_reset(stmt_7);
			sqlite3_clear_bindings(stmt_7);
		}
		sqlite3_reset(stmt_2);
		sqlite3_clear_bindings(stmt_2);
		sqlite3_reset(stmt_3);
		sqlite3_clear_bindings(stmt_3);
		sqlite3_reset(stmt_4);
		sqlite3_clear_bindings(stmt_4);
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
    if(stmt_5)
    	sqlite3_finalize(stmt_5);
    if(stmt_6)
    	sqlite3_finalize(stmt_6);
    if(stmt_7)
    	sqlite3_finalize(stmt_7);

	cJSON_Delete(pJsonRoot);

    return ret;

}

int app_req_scenario_name(payload_t data)
{
	M1_LOG_DEBUG("app_req_scenario_name\n");
	int rc                   = 0;
	int ret                  = M1_PROTOCOL_OK;
    int pduType              = TYPE_M1_REPORT_DISTRICT_INFO;
    char* sql                = NULL;
    char* dist_name          = NULL;
    char *ap_id              = NULL;
    char *ap_name            = NULL;
    char *scen_name          = NULL;
    cJSON * pJsonRoot        = NULL;
    cJSON * pduJsonObject    = NULL;
    cJSON * devDataJsonArray = NULL;
    cJSON*  devData          = NULL;
    sqlite3* db              = NULL;
    sqlite3_stmt* stmt       = NULL;

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
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
    {
	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	    ret = M1_PROTOCOL_FAILED;
	    goto Finish; 
	}
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW)
    {
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
  	if(stmt)
		sqlite3_finalize(stmt);
	
    cJSON_Delete(pJsonRoot);

    return ret;
}

/*定时执行场景检查*/
void scenario_alarm_select(void)
{
 	M1_LOG_DEBUG("scenario_alarm_select\n");
 	int rc;
 	char* sql = NULL;
 	scen_alarm_t alarm;
 	sqlite3_stmt* stmt = NULL;
 	sql = "select SCEN_NAME, HOUR, MINUTES, WEEK, STATUS from scen_alarm_table;";
 	while(1)
 	{
 		rc = sql_open();    
	    if(rc)
	    {  
	         M1_LOG_ERROR( "Can't open database: %s\n", sqlite3_errmsg(db));  
	         return M1_PROTOCOL_FAILED;  
	     }
	     else
	     {  
	         M1_LOG_DEBUG( "Opened database successfully\n");  
	     }

	    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	    {
		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    goto Finish; 
		}
	    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW)
	    {
	    	alarm.scen_name = sqlite3_column_text(stmt, 0);
	    	alarm.hour = sqlite3_column_int(stmt, 1);
	    	alarm.week = sqlite3_column_text(stmt, 3);
	    	alarm.minutes = sqlite3_column_int(stmt, 2);
	    	alarm.status = sqlite3_column_text(stmt, 4);
	    	rc = scenario_alarm_check(alarm);
	    	if(rc)
	    		scenario_exec(alarm.scen_name, db);
	    }

	    Finish:
	    sqlite3_finalize(stmt);
 		sql_close(); 

	    sleep(60);
 	}
}

 static int scenario_alarm_check(scen_alarm_t alarm_time)
 {
 	M1_LOG_DEBUG("scenario_alarm_check\n");
 	int on_time_flag = 0;
 	char* week = NULL;
 	struct tm nowTime;
    struct timespec time;
 	
    clock_gettime(CLOCK_REALTIME, &time);  //获取相对于1970到现在的秒数
    localtime_r(&time.tv_sec, &nowTime);
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

