#include "ap_config.h"
#include "m1_common_log.h"

static int ap_cfg_sql_opt(sqlite3* db, char* ap_id, char* p_router, char* p_channel);
static int ap_cfg_send_to_ap(sqlite3* db, char* ap_id, char* p);

/*0x0032 APP下发AP路由配置*/
/*0x0033 APP下发AP Zigbee网络信道*/
int ap_cfg_router(cJSON* data, sqlite3* db)
{
	int ret              = 0;
    int sql_flag         = 0;
	char * p             = NULL;
	cJSON *pdu_json      = NULL;
	cJSON *data_json     = NULL;
	cJSON *ap_id_json    = NULL;
	cJSON *pdu_type_json = NULL;

    /*获取Json内信息*/    
   	/*pdu*/
   	pdu_json = cJSON_GetObjectItem(data, "pdu");
   	if(NULL == pdu_json)
   	{
       	M1_LOG_ERROR("pdu null\n");
        ret = -1;
        goto Finish;
   	}
   	/*devData*/
   	data_json = cJSON_GetObjectItem(pdu_json, "devData");
   	if(NULL == data_json)
   	{
   	    M1_LOG_ERROR("devData null\n");
        ret = -1;
        goto Finish;
   	}
 	/*pduType*/
   	pdu_type_json = cJSON_GetObjectItem(pdu_json, "pduType");
   	if(NULL == pdu_type_json)
   	{
   	    M1_LOG_ERROR("devData null\n");
        ret = -1;
        goto Finish;
   	}
   	/*apId*/
   	ap_id_json = cJSON_GetObjectItem(data_json, "apId");
   	if(NULL == ap_id_json)
   	{
   	    M1_LOG_ERROR("apId null\n");
        ret = -1;
        goto Finish;
   	}

   	
   	p = cJSON_PrintUnformatted(data);
    
    if(NULL == p)
    {    
        ret = -1;
        goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);

    /*写入数据库*/
    if( pdu_type_json->valueint == TYPE_AP_REPORT_AP_ROUTER)
    {
        ret = ap_cfg_sql_opt(db, ap_id_json->valuestring, p, NULL);
    }
    else if(pdu_type_json->valueint == TYPE_AP_REPORT_ZIGBEE)
    {
        ret = ap_cfg_sql_opt(db, ap_id_json->valuestring, NULL, p);
    }

   	/*发给AP*/
   	if(pdu_type_json->valueint == TYPE_APP_CFG_AP_ROUTER  || pdu_type_json->valueint == TYPE_APP_CFG_AP_ZIGBEE)
   	{
        sql_flag = 1;
   		ret = ap_cfg_send_to_ap(db, ap_id_json->valuestring, p);
   	}
    
   	Finish:
    if(!sql_flag)
        sql_close();
   	return ret;
}

/*ap router配置信息发送给AP*/
static int ap_cfg_send_to_ap(sqlite3* db, char* ap_id, char* p)
{
	int rc             = 0;
	int ret            = 0;
	int clientFd       = 0;
	char* sql          = NULL;
	sqlite3_stmt* stmt = NULL;

    sql = "select CLIENT_FD from conn_info where AP_ID = ?;";
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
        if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
            m1_error_handle();
        ret = -1;
        goto Finish; 
    }
   
    M1_LOG_DEBUG("%s\n",sql);
    sqlite3_bind_text(stmt, 1, ap_id, -1, NULL);
    rc = sqlite3_step(stmt);   
    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();
    }
    clientFd = sqlite3_column_int(stmt, 0);

    if(rc != SQLITE_ROW)
    {
        ret = -1;
        goto Finish;
    }

    Finish:
    if(stmt)
    	sqlite3_finalize(stmt);

    sql_close();

    if(clientFd != 0)
        socketSeverSend((uint8*)p, strlen(p), clientFd);

    return ret;
}
/*写入表ap_router_cfg_table*/
static int ap_cfg_sql_opt(sqlite3* db, char* ap_id, char* p_router, char* p_channel)
{
	int rc               = 0;
	int ret              = 0;
    int sql_commit_flag  = 0;
    char* errorMsg       = NULL;
    char* sql            = NULL;
    char* sql_1          = NULL;
    char* sql_2          = NULL;
    sqlite3_stmt* stmt   = NULL;
    sqlite3_stmt* stmt_1 = NULL;
    sqlite3_stmt* stmt_2 = NULL;

    sql = "select ID from ap_router_cfg_table where DEV_ID = ?;";
   	rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
        if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
            m1_error_handle();
        ret = -1;
        goto Finish; 
    }
     
    sql_1 = "insert into ap_router_cfg_table(DEV_ID,PARAM,ZIGBEE)values(?,?,?)";
    rc = sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
        if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
            m1_error_handle();
        ret = -1;
        goto Finish; 
    }

    if(p_channel == NULL)
	    sql_2 = "update ap_router_cfg_table set PARAM = ? where DEV_ID = ?;";
	else
		sql_2 = "update ap_router_cfg_table set ZIGBEE = ? where DEV_ID = ?;";

    rc = sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
        if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
            m1_error_handle();
        ret = -1;
        goto Finish; 
    }
    	
    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        M1_LOG_DEBUG("BEGIN IMMEDIATE\n");
        sql_commit_flag = 1;
            
        M1_LOG_DEBUG("%s\n",sql);
        sqlite3_bind_text(stmt, 1, ap_id, -1, NULL);
        rc = sqlite3_step(stmt);   
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            if(rc == SQLITE_CORRUPT)
                m1_error_handle();
        }
    	
        if(rc != SQLITE_ROW)
        {
        	M1_LOG_DEBUG("%s\n",sql_1);

        	sqlite3_bind_text(stmt_1, 1, ap_id, -1, NULL);
        	sqlite3_bind_text(stmt_1, 2, p_router, -1, NULL);
        	sqlite3_bind_text(stmt_1, 3, p_channel, -1, NULL);

        	rc = sqlite3_step(stmt_1);   
	        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
	        {
	            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
	            if(rc == SQLITE_CORRUPT)
	                m1_error_handle();
	        }
        }
    	else
    	{
    		M1_LOG_DEBUG("%s\n",sql_2);
    		if(p_channel == NULL)
        		sqlite3_bind_text(stmt_2, 1, p_router, -1, NULL);
        	else
        		sqlite3_bind_text(stmt_2, 1, p_channel, -1, NULL);

        	sqlite3_bind_text(stmt_2, 2, ap_id, -1, NULL);
        	rc = sqlite3_step(stmt_2);   
	        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
	        {
	            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
	            if(rc == SQLITE_CORRUPT)
	                m1_error_handle();
	        }
    	}
    }
    else
    {
        M1_LOG_WARN("BEGIN IMMEDIATE errorMsg:%s",errorMsg);
        sqlite3_free(errorMsg);
    }

    Finish:
    if(sql_commit_flag)
    {
        rc = sql_commit(db);
        if(rc == SQLITE_OK)
        {
            M1_LOG_DEBUG("COMMIT OK\n");
        }
    }

    if(stmt)
    	sqlite3_finalize(stmt);
    if(stmt_1)
    	sqlite3_finalize(stmt_1);
    if(stmt_2)
    	sqlite3_finalize(stmt_2);

    sql_close();

    return ret;
}

/*app read ap config*/
int app_read_ap_router_cfg(payload_t data)
{
	int ret              = 0;
	int rc               = 0;
	char* dev_id         = NULL;
	char* p              = NULL;
    char sendMsg[500]    = {0};
	char* sql            = NULL;
	sqlite3 *db          = NULL;
	sqlite3_stmt* stmt   = NULL;
	cJSON *pdu_type_json = NULL;
	cJSON *dev_data_json = NULL;

	if(data.pdu == NULL)
    {
        M1_LOG_ERROR("data NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    db = data.db;   

   	dev_id = data.pdu->valuestring;

    sql = "select PARAM from ap_router_cfg_table where DEV_ID = ?;";
   
   	rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
        if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
            m1_error_handle();
        ret = -1;
        goto Finish; 
    }

    M1_LOG_DEBUG("%s\n",sql);
    sqlite3_bind_text(stmt, 1, dev_id, -1, NULL);
    rc = sqlite3_step(stmt);   
    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();
    }

    p = sqlite3_column_text(stmt, 0);
    if(NULL == p)
    {
    	ret = -1;
    	M1_LOG_ERROR("p NULL!\n");
    	goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);

    strcpy(sendMsg, p);

    Finish:
    if(stmt)
    	sqlite3_finalize(stmt);

    sql_close();

    if(p)
    {
        socketSeverSend((uint8*)sendMsg, strlen(sendMsg), data.clientFd);
    }

    return ret;
}

/*app read ap config*/
int app_read_ap_zigbee_cfg(payload_t data)
{
	int ret              = 0;
	int rc               = 0;
	int pdu_type         = 0;
	char* dev_id         = NULL;
	char* p              = NULL;
    char sendMsg[300]    = {0};
	char* sql            = NULL;
	sqlite3 *db          = NULL;
	sqlite3_stmt* stmt   = NULL;
	cJSON *pdu_type_json = NULL;
	cJSON *dev_data_json = NULL;

	if(data.pdu == NULL)
    {
        M1_LOG_ERROR("data NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    db = data.db;   

    // dev_data_json = cJSON_GetObjectItem(data.pdu, "devData");
   	// if(NULL == dev_data_json)
   	// {
    //    	M1_LOG_ERROR("dev_data_json null\n");
    //     ret = -1;
    //     goto Finish;
   	// }

   	dev_id = data.pdu->valuestring;

   	sql = "select ZIGBEE from ap_router_cfg_table where DEV_ID = ?;";
   	
   	rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
        if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
            m1_error_handle();
        ret = -1;
        goto Finish; 
    }

    M1_LOG_DEBUG("%s\n",sql);
    sqlite3_bind_text(stmt, 1, dev_id, -1, NULL);
    rc = sqlite3_step(stmt);   
    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();
    }

    p = sqlite3_column_text(stmt, 0);
    if(NULL == p)
    {
    	ret = -1;
    	M1_LOG_ERROR("p NULL!\n");
    	goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);

    strcpy(sendMsg,p);

    Finish:
    if(stmt)
    	sqlite3_finalize(stmt);

    sql_close();

    if(p)
    {
        socketSeverSend((uint8*)sendMsg, strlen(sendMsg), data.clientFd);
    }

    return ret;
}


int M1_write_config_to_AP(cJSON* data, sqlite3* db)
{
    M1_LOG_DEBUG("M1_write_to_AP\n");
    int sn                = 2;
    int clientFd          = 0;
    int rc                = 0;
    int ret               = M1_PROTOCOL_OK;
    /*sql*/
    char *sql             = NULL;
    sqlite3_stmt *stmt    = NULL;
    /*Json*/
    cJSON* snJson         = NULL;
    cJSON* pduJson        = NULL;
    cJSON* devDataJson    = NULL;
    char * p              = NULL;

    
    if(data == NULL){
        M1_LOG_ERROR("data NULL");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    /*更改sn*/
    snJson = cJSON_GetObjectItem(data, "sn");
    if(snJson == NULL){
        M1_LOG_ERROR("snJson NULL");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;    
    }
    cJSON_SetIntValue(snJson, sn);
    /*获取clientFd*/
    pduJson       = cJSON_GetObjectItem(data, "pdu");
    if(pduJson == NULL)
        goto Finish;
    devDataJson   = cJSON_GetObjectItem(pduJson, "devData");
    if(devDataJson == NULL)
        goto Finish;
    M1_LOG_DEBUG("devId:%s\n",devDataJson->valuestring);
   
    sql = "select a.CLIENT_FD from conn_info as a, all_dev as b where a.AP_ID = b.AP_ID and b.DEV_ID = ? limit 1;";
    M1_LOG_DEBUG("%s\n", sql);
    rc = sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        sql_error_set();
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    else
    {
        sql_error_clear();
    }
    sqlite3_bind_text(stmt, 1, devDataJson->valuestring, -1, NULL);
    rc = sqlite3_step(stmt);   
    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        sql_error_set();
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();
    }
    else
    {
        sql_error_clear();
    }
    if(rc == SQLITE_ROW)
    {
        clientFd = sqlite3_column_int(stmt,0);
        p = cJSON_PrintUnformatted(data);
        if(NULL == p)
        {    
            cJSON_Delete(data);
            ret = M1_PROTOCOL_FAILED;
            goto Finish;  
        }

        M1_LOG_DEBUG("string:%s\n",p);
    }   

    Finish:
    if(stmt != NULL)
        sqlite3_finalize(stmt);

    sql_close();
    /*response to client*/
    if(p)
        socketSeverSend((uint8*)p, strlen(p), clientFd);

    return ret;
}



