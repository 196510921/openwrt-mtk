#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"
#include "sqlite3.h"
#include "account_config.h"
#include "m1_common_log.h"
#include "m1_protocol.h"

int app_req_account_info_handle(payload_t data)
{
    M1_LOG_DEBUG("account_info_handle”\n");
    
    int rc                  = 0;
    int ret                 = M1_PROTOCOL_OK;
    int pduType             = TYPE_M1_REPORT_ACCOUNT_INFO;
	char* sql               = NULL;
    char* account           = NULL;
    cJSON* pJsonRoot        = NULL;
	cJSON* pduJsonObject    = NULL;
	cJSON* accountJson      = NULL;
	cJSON* devDataJsonArray = NULL;
	sqlite3* db             = NULL;
    sqlite3_stmt* stmt      = NULL;
    
    sql = "select ACCOUNT from account_table order by ID";
    db = data.db;
    /*get sql data json*/
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
    M1_LOG_DEBUG("%s\n", sql);

    if(sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db)); 
        ret = M1_PROTOCOL_FAILED; 
        goto Finish; 
    }
    while(sqlite3_step(stmt) == SQLITE_ROW){
	    /*create pdu object*/	    
	    account = sqlite3_column_text(stmt,0);
	    M1_LOG_DEBUG("account:%s\n",account);
	    accountJson = cJSON_CreateString(account);
	    if(NULL == pduJsonObject)
	    {
	        // create object faild, exit
	        cJSON_Delete(accountJson);
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
	    }
	    cJSON_AddItemToArray(devDataJsonArray, accountJson);
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
    socketSeverSend((unsigned char*)p, strlen(p), data.clientFd);
    
    Finish:
    if(stmt)
        sqlite3_finalize(stmt);
    cJSON_Delete(pJsonRoot);

    return ret;
}

int app_req_account_config_handle(payload_t data)
{
	M1_LOG_DEBUG("app_req_account_config_handle\n");
    
    int pId               = 0;
    int rc                = 0;
    int ret               = M1_PROTOCOL_OK;
	int pduType           = TYPE_M1_REPORT_ACCOUNT_CONFIG_INFO;
    char *account         = NULL;
    char *key             = NULL;
    char *key_auth        = NULL;
    char *remote_auth     = NULL;
    char *district        = NULL;
    char *scenario        = NULL;
    char *dev_name        = NULL;
    char *dev_id          = NULL;
    /*Json*/
    cJSON* pJsonRoot      = NULL;
	cJSON* pduJsonObject  = NULL;
	cJSON* devDataObject  = NULL;
	cJSON* accountJson    = NULL;
	cJSON* userDataArray  = NULL;
    cJSON* userDataObject = NULL;
	cJSON* districtObject = NULL;
	cJSON* scenArray      = NULL;
	cJSON* scenJson       = NULL;
	cJSON* devArray       = NULL;
	cJSON* devObject      = NULL;
	/*sql*/
    sqlite3* db           = NULL;
    char* sql             = NULL;
    char* sql_1           = NULL;
    char* sql_2           = NULL;
    sqlite3_stmt *stmt    = NULL;
    sqlite3_stmt *stmt_1  = NULL;
    sqlite3_stmt *stmt_2  = NULL;

    db = data.db;
    /*get sql data json*/
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
    devDataObject = cJSON_CreateObject();
    if(NULL == devDataObject)
    {
        cJSON_Delete(devDataObject);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataObject);
    /*获取账户名*/
    accountJson = data.pdu;
    {
        sql = "select KEY,KEY_AUTH,REMOTE_AUTH from account_table where ACCOUNT = ?;";
        M1_LOG_DEBUG("%s\n", sql);

        if(sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db)); 
            ret = M1_PROTOCOL_FAILED; 
            goto Finish; 
        }

        sqlite3_bind_text(stmt, 1, accountJson->valuestring, -1, NULL);

        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
    	    /*create pdu object*/	    
    	    key = sqlite3_column_text(stmt,0);
    	    key_auth = sqlite3_column_text(stmt,1);
    	    remote_auth = sqlite3_column_text(stmt,2);
            if(key == NULL || key_auth == NULL || remote_auth == NULL){
                M1_LOG_ERROR( "key NULL\n"); 
                ret = M1_PROTOCOL_FAILED; 
                goto Finish;       
            }
    	    M1_LOG_DEBUG("key:%s,key_auth:%s, remote_auth:%s\n",key, key_auth, remote_auth);
    	    cJSON_AddStringToObject(devDataObject, "account", accountJson->valuestring);
            cJSON_AddStringToObject(devDataObject, "key", key);
    	    cJSON_AddStringToObject(devDataObject, "keyAuthority", key_auth);
    	    cJSON_AddStringToObject(devDataObject, "remoteAuthority", remote_auth);
    	}

        if(stmt)
        {
            sqlite3_finalize(stmt);
            stmt = NULL;
        }
    }
    /*获取区域信息*/
	userDataArray = cJSON_CreateArray();
    if(NULL == userDataArray)
    {
        cJSON_Delete(userDataArray);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    cJSON_AddItemToObject(devDataObject, "userData", userDataArray);
    {
        /*add devData array to pdu Object*/
        sql = "select distinct DIS_NAME from district_table where ACCOUNT = ?;";
        M1_LOG_DEBUG("%s\n", sql);

        if(sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
    }

    {
        sql_1 = "select distinct SCEN_NAME from scenario_table where ACCOUNT = ? and DISTRICT = ?;";
        M1_LOG_DEBUG("%s\n", sql_1);
        
        if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1),&stmt_1, NULL) != SQLITE_OK){
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
    }

    {
        sql_2 = "select PID, DEV_NAME, DEV_ID from all_dev where ACCOUNT = ?;";
        M1_LOG_DEBUG("%s\n", sql_2);

        if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2),&stmt_2, NULL) != SQLITE_OK){
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
    }

    sqlite3_bind_text(stmt, 1, accountJson->valuestring, -1, NULL);
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        {
            userDataObject = cJSON_CreateObject();
            if(NULL == userDataObject)
            {
                // create object faild, exit
                cJSON_Delete(userDataObject);
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            cJSON_AddItemToArray(userDataArray,userDataObject);
    	    /*添加区域信息*/	    
    	    district = sqlite3_column_text(stmt,0);
    	   	M1_LOG_DEBUG("district:%s\n", district);
    	    cJSON_AddStringToObject(userDataObject, "district", district);
            /*获取场景信息*/
            scenArray = cJSON_CreateArray();
            if(NULL == scenArray)
            {
                cJSON_Delete(scenArray);
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
        }

        {
            /*add devData array to pdu pbject*/
            cJSON_AddItemToObject(userDataObject, "scenario", scenArray);
           
            sqlite3_bind_text(stmt_1, 1, accountJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt_1, 2, district, -1, NULL);
            while(sqlite3_step(stmt_1) == SQLITE_ROW)
            {
                /*添加区域信息*/      
                scenario = sqlite3_column_text(stmt_1,0);
                M1_LOG_DEBUG("scenario:%s\n", scenario);
                scenJson = cJSON_CreateString(scenario);
                if(NULL == scenJson)
                {
                    // create object faild, exit
                    cJSON_Delete(scenJson);
                    ret = M1_PROTOCOL_FAILED;
                    goto Finish;
                }
                cJSON_AddItemToArray(scenArray, scenJson);
            }
            sqlite3_reset(stmt_1);
            sqlite3_clear_bindings(stmt_1);
        }
        /*获取设备信息*/
        devArray = cJSON_CreateArray();
        if(NULL == devArray)
        {
            cJSON_Delete(devArray);
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }
        cJSON_AddItemToObject(userDataObject, "device", devArray);
        
        sqlite3_bind_text(stmt_2, 1, accountJson->valuestring, -1, NULL);
        while(sqlite3_step(stmt_2) == SQLITE_ROW)
        {
            /*添加区域信息*/      
            pId = sqlite3_column_int(stmt_1,0);
            if(pId == NULL){
                cJSON_Delete(devObject);
                ret = M1_PROTOCOL_FAILED;
                goto Finish;;
            }
            dev_name = sqlite3_column_text(stmt_1,1);
            if(pId == NULL){
                cJSON_Delete(devObject);
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            dev_id = sqlite3_column_text(stmt_1,2);
            M1_LOG_DEBUG("pid:%05d,dev_name:%s, dev_id:%s\n", pId, dev_name,dev_id);
            devObject = cJSON_CreateObject();
            if(NULL == devObject)
            {
                // create object faild, exit
                cJSON_Delete(devObject);
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            cJSON_AddNumberToObject(devObject, "pId", pId);
            cJSON_AddStringToObject(devObject, "devName", dev_name);
            cJSON_AddStringToObject(devObject, "devId", dev_id);
            cJSON_AddItemToArray(devArray, devObject);
        }
        sqlite3_reset(stmt_2);
        sqlite3_clear_bindings(stmt_2);
	}

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);
    /*response to client*/
    socketSeverSend((unsigned char*)p, strlen(p), data.clientFd);

    Finish:
    if(stmt)
        sqlite3_finalize(stmt);
    if(stmt_1)
        sqlite3_finalize(stmt_1);
    if(stmt_2)
        sqlite3_finalize(stmt_2);

    cJSON_Delete(pJsonRoot);

    return ret;
}

int app_account_config_handle(payload_t data)
{
    M1_LOG_DEBUG("app_account_config_handle\n");
    
    int i                 = 0;
    int number            = 0;
    int rc                = 0;
    int ret               = M1_PROTOCOL_OK;
    char *sql             = NULL;
    char *sql_1           = NULL;
    char *sql_2           = NULL;
    char *errorMsg        = NULL;
    char *ap_id           = NULL;
    char *dev_id          = NULL;
    char *district        = NULL;
    char *dev_name        = NULL;
    char *status          = NULL;
    char *dis_pic         = NULL;
    char *scen_pic        = NULL;
    int type              = 0;
    int value             = 0;
    int delay             = 0;
    int pid               = 0;
    int added             = 0;
    int net               = 0;
    cJSON *districtArray  = NULL;
    cJSON *districtObject = NULL;
    cJSON *scenArray      = NULL;
    cJSON *scenObject     = NULL;
    cJSON *devArray       = NULL;
    cJSON *devObject      = NULL;
    cJSON *devIdObject    = NULL;
    cJSON *accountJson    = NULL;
    cJSON *keyJson        = NULL;
    cJSON *keyAuthJson    = NULL;
    cJSON *remoteAuthJson = NULL;
    sqlite3 *db           = NULL;
    sqlite3_stmt *stmt    = NULL;
    sqlite3_stmt *stmt_1  = NULL;
    sqlite3_stmt *stmt_2  = NULL;

    /*获取数据库*/
    db = data.db;

    accountJson = cJSON_GetObjectItem(data.pdu,"account");
    if(accountJson->valuestring == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    keyJson = cJSON_GetObjectItem(data.pdu,"key");
    if(keyJson->valuestring == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;    
    }
    keyAuthJson = cJSON_GetObjectItem(data.pdu,"keyAuthority");
    if(keyAuthJson->valuestring == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;   
    }
    remoteAuthJson = cJSON_GetObjectItem(data.pdu,"remoteAuthority");
    if(remoteAuthJson->valuestring == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;   
    }
    M1_LOG_DEBUG( "account:%s,key:%s,keyAuthority:%s,remoteAuthority:%s\n", accountJson->valuestring,\
        keyJson->valuestring,keyAuthJson->valuestring,remoteAuthJson->valuestring);

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        M1_LOG_DEBUG("BEGIN IMMEDIATE\n");
        /*插入到account_table*/
        {
            
            sql = "insert or replace into account_table(ACCOUNT, KEY, KEY_AUTH, REMOTE_AUTH)values(?,?,?,?);";
            M1_LOG_DEBUG("sql:%s\n",sql);

            if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
            {
                ret = M1_PROTOCOL_FAILED;
                M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
                goto Finish; 
            }

            sqlite3_bind_text(stmt, 1,  accountJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt, 2,  keyJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt, 3,  keyAuthJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt, 4,  remoteAuthJson->valuestring, -1, NULL);

            rc = sqlite3_step(stmt);   
            M1_LOG_DEBUG("step() return %s, number:%03d\n", \
                rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
                    
            if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            }
            
            if(stmt)
            {
                sqlite3_finalize(stmt);
                stmt = NULL;
            }
        }

        /*添加用户区域信息,删除重复用户信息*/
        {
            if(strcmp(accountJson->valuestring,"Dalitek") == 0)
            {
                ret = M1_PROTOCOL_FAILED;
                goto Finish;   
            }
         
            sql_1 = "select AP_ID, DIS_PIC from district_table where DIS_NAME = ? and ACCOUNT = \"Dalitek\";";
            M1_LOG_DEBUG("sql:%s\n",sql_1);

            if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
            {
                M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
                ret = M1_PROTOCOL_FAILED;
                goto Finish; 
            }

            sql_2 = "insert or replace into district_table(DIS_NAME, DIS_PIC, AP_ID, ACCOUNT)values(?,?,?,?);";
            M1_LOG_DEBUG("sql:%s\n",sql_2);

            if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK)
            {
                ret = M1_PROTOCOL_FAILED;
                M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
                goto Finish; 
            }

            districtArray = cJSON_GetObjectItem(data.pdu,"district");
            number = cJSON_GetArraySize(districtArray);
            for(i = 0; i < number; i++)
            {
                districtObject = cJSON_GetArrayItem(districtArray, i);
                M1_LOG_DEBUG("district:%s\n",districtObject->valuestring);
                
                sqlite3_bind_text(stmt_1, 1, districtObject->valuestring, -1, NULL);
                while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW)
                {
                    ap_id = sqlite3_column_text(stmt_1, 0);
                    dis_pic = sqlite3_column_text(stmt_1, 1);

        
                    sqlite3_bind_text(stmt_2, 1, districtObject->valuestring, -1, NULL);
                    sqlite3_bind_text(stmt_2, 2, dis_pic, -1, NULL);
                    sqlite3_bind_text(stmt_2, 3, ap_id, -1, NULL);
                    sqlite3_bind_text(stmt_2, 4, accountJson->valuestring, -1, NULL);
                    
                    rc = sqlite3_step(stmt);   
                    M1_LOG_DEBUG("step() return %s, number:%03d\n", \
                        rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
                    
                    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
                    {
                        M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
                    }
                    sqlite3_reset(stmt_2);
                    sqlite3_clear_bindings(stmt_2);
                }

                sqlite3_reset(stmt_1);
                sqlite3_clear_bindings(stmt_1);
            }

            if(stmt_1)
            {
                sqlite3_finalize(stmt_1);
                stmt_1 = NULL;
            }

            if(stmt_2)
            {
                sqlite3_finalize(stmt_2);
                stmt_2 = NULL;
            }
        }

        /*添加用户场景信息*/
        /*删除重复用户信息*/
        {
            
            sql = "select DISTRICT,AP_ID,DEV_ID,TYPE,VALUE,DELAY,SCEN_PIC from scenario_table where \
                   SCEN_NAME = ? and ACCOUNT = \"Dalitek\";";
            M1_LOG_DEBUG("sql:%s\n",sql);
            if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
            {
                M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
                ret = M1_PROTOCOL_FAILED;
                goto Finish; 
            } 

            sql_1 = "insert or replace into scenario_table(SCEN_NAME, SCEN_PIC, DISTRICT, AP_ID, DEV_ID, TYPE, \
                    VALUE, DELAY, ACCOUNT)values(?,?,?,?,?,?,?,?,?);";
            M1_LOG_DEBUG("sql_1:%s\n",sql_1);
            if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK){
                M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
                ret = M1_PROTOCOL_FAILED;
                goto Finish; 
            }

            scenArray = cJSON_GetObjectItem(data.pdu,"scenario");
            number = cJSON_GetArraySize(scenArray);
            for(i = 0; i < number; i++)
            {
                scenObject = cJSON_GetArrayItem(scenArray, i);
                M1_LOG_DEBUG("scenario:%s\n",scenObject->valuestring);
                
                sqlite3_bind_text(stmt, 1, scenObject->valuestring, -1, NULL);
                while(sqlite3_step(stmt) == SQLITE_ROW)
                {
                    district = sqlite3_column_text(stmt, 0);
                    ap_id = sqlite3_column_text(stmt, 1);
                    dev_id = sqlite3_column_text(stmt, 2);
                    type = sqlite3_column_int(stmt, 3);
                    value = sqlite3_column_int(stmt, 4);
                    delay = sqlite3_column_int(stmt, 5);
                    scen_pic = sqlite3_column_text(stmt, 6);
        
                    sqlite3_bind_text(stmt_1, 1,  scenObject->valuestring, -1, NULL);
                    sqlite3_bind_text(stmt_1, 2,  scen_pic, -1, NULL);
                    sqlite3_bind_text(stmt_1, 3,  district, -1, NULL);
                    sqlite3_bind_text(stmt_1, 4,  ap_id, -1, NULL);
                    sqlite3_bind_text(stmt_1, 5,  dev_id, -1, NULL);
                    sqlite3_bind_int(stmt_1, 6, type);
                    sqlite3_bind_int(stmt_1, 7, value);
                    sqlite3_bind_int(stmt_1, 8, delay);
                    sqlite3_bind_text(stmt_1, 9, accountJson->valuestring, -1, NULL);

                    rc = sqlite3_step(stmt_1);   
                    M1_LOG_DEBUG("step() return %s, number:%03d\n", \
                        rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
                        
                    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
                    {
                        M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
                    }
                    sqlite3_reset(stmt_1);
                    sqlite3_clear_bindings(stmt_1);
                }        
                sqlite3_reset(stmt);
                sqlite3_clear_bindings(stmt);
            }
            if(stmt)
            {
                sqlite3_finalize(stmt);
            }
            if(stmt_1)
            {
                sqlite3_finalize(stmt_1);
            }

        }

        /*添加用户设备信息*/
        {
            sql = "select DEV_NAME,AP_ID,PID,ADDED,NET,STATUS from all_dev where DEV_ID = ? and ACCOUNT = \"Dalitek\";";
            M1_LOG_DEBUG("sql:%s\n",sql);
            if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
            {
                M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
                ret = M1_PROTOCOL_FAILED;
                goto Finish; 
            } 

            sql_1 = "insert into all_dev(DEV_ID,DEV_NAME,AP_ID,PID,ADDED,NET,STATUS,ACCOUNT)values(?,?,?,?,?,?,?,?);";
            M1_LOG_DEBUG("sql_1:%s\n",sql_1);
            if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK){
                M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
                ret = M1_PROTOCOL_FAILED;
                goto Finish; 
            }

            devArray = cJSON_GetObjectItem(data.pdu,"device");
            number = cJSON_GetArraySize(devArray);
            M1_LOG_DEBUG("number:%05d\n",number);
            for(i = 0; i < number; i++)
            {
                devObject = cJSON_GetArrayItem(devArray, i);
                devIdObject = devObject;
                if(devIdObject->valuestring == NULL){
                    M1_LOG_ERROR("devIdObject->valuestring NULL\n");
                    continue;
                }
                M1_LOG_DEBUG("devIdObject->valuestring:%s\n",devIdObject->valuestring);

                sqlite3_bind_text(stmt, 1, devIdObject->valuestring, -1, NULL);
                while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW)
                {
                    dev_name = sqlite3_column_text(stmt, 0);
                    ap_id = sqlite3_column_text(stmt, 1);
                    pid = sqlite3_column_int(stmt, 2);
                    added = sqlite3_column_int(stmt, 3);
                    net = sqlite3_column_int(stmt, 4);
                    status = sqlite3_column_text(stmt, 5);
        
                    sqlite3_bind_text(stmt_1, 1, devIdObject->valuestring, -1, NULL);
                    sqlite3_bind_text(stmt_1, 2, dev_name, -1, NULL);
                    sqlite3_bind_text(stmt_1, 3, ap_id, -1, NULL);
                    sqlite3_bind_int(stmt_1, 4, pid);
                    sqlite3_bind_int(stmt_1, 5, added);
                    sqlite3_bind_int(stmt_1, 6, net);
                    sqlite3_bind_text(stmt_1, 7, status, -1, NULL);
                    sqlite3_bind_text(stmt_1, 8, accountJson->valuestring, -1, NULL);
            
                    rc = sqlite3_step(stmt_1);   
                    M1_LOG_DEBUG("step() return %s, number:%03d\n", \
                        rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
                        
                    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
                    {
                        M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
                    }
                    sqlite3_reset(stmt_1);
                    sqlite3_clear_bindings(stmt_1);
                }        
                sqlite3_reset(stmt);
                sqlite3_clear_bindings(stmt);
            }
            if(stmt)
            {
                sqlite3_finalize(stmt);
            }
            if(stmt_1)
            {
                sqlite3_finalize(stmt_1);
            }

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
    if(stmt_2)
        sqlite3_finalize(stmt_2);

    return  ret;
}

int app_req_dis_name(payload_t data)
{
    M1_LOG_DEBUG("app_req_dis_name\n"); 
    int pdu_type = TYPE_M1_REPORT_DIS_NAME;
    int rc,ret = M1_PROTOCOL_OK;
    char* district = NULL, *dis_pic = NULL;
    char* sql = (char*)malloc(300);
    char* sql_1 = (char*)malloc(300);
    char* sql_2 = (char*)malloc(300);
    cJSON * pJsonRoot = NULL; 
    cJSON * pduJsonObject = NULL;
    cJSON * devDataObject= NULL;
    cJSON * districtObject= NULL;
    cJSON * devDataJsonArray = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt *stmt = NULL,*stmt_1 = NULL,*stmt_2 = NULL;

    db = data.db;
    /*get sql data json*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_ERROR("pJsonRoot NULL\n");
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
    cJSON_AddNumberToObject(pduJsonObject, "pduType", pdu_type);
    /*添加devData*/
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
    char* account = NULL;
    sprintf(sql_1,"select ACCOUNT from account_info where CLIENT_FD = %03d order by ID desc limit 1;",data.clientFd);
    M1_LOG_DEBUG( "%s\n", sql_1);
    if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    if(thread_sqlite3_step(&stmt_1, db) == SQLITE_ROW){
        account =  sqlite3_column_text(stmt_1, 0);
    }
    if(account == NULL){
        M1_LOG_ERROR( "user account do not exist\n");    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }else{
        M1_LOG_DEBUG("clientFd:%03d,account:%s\n",data.clientFd, account);
    }

    /*获取项目信息*/
    sprintf(sql,"select distinct DIS_NAME from district_table where ACCOUNT = \"%s\";",account);
    M1_LOG_DEBUG("sql:%s\n", sql);
    sqlite3_finalize(stmt);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
        districtObject = cJSON_CreateObject();
        if(NULL == districtObject)
        {
            // create object faild, exit
            cJSON_Delete(districtObject);
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }

        /*获取区域名称*/
        district = sqlite3_column_text(stmt, 0);
        if(district == NULL){
            M1_LOG_ERROR( "%s\n", district);
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }
        M1_LOG_DEBUG("district:%s\n", district);
        cJSON_AddStringToObject(districtObject, "disName", district);
        sprintf(sql_2,"select DIS_PIC from district_table where DIS_NAME = \"%s\" order by ID desc limit 1;", district);
        M1_LOG_DEBUG("sql:%s\n", sql_2);
        sqlite3_finalize(stmt_2);
        if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK){
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        rc = thread_sqlite3_step(&stmt_2, db);
        if(rc != SQLITE_ROW){
            continue;
        }
        dis_pic = sqlite3_column_text(stmt_2, 0);
        if(dis_pic == NULL){
            continue;
        }
        M1_LOG_DEBUG("dis_pic:%s\n", dis_pic);
        cJSON_AddStringToObject(districtObject, "disPic", dis_pic);
        //districtObject = cJSON_CreateString(district);
        cJSON_AddItemToArray(devDataJsonArray, districtObject);
    }

    char* p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);
    socketSeverSend((unsigned char*)p, strlen(p), data.clientFd);
    Finish:
    free(sql);
    free(sql_1);
    free(sql_2);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
    sqlite3_finalize(stmt_2);
    cJSON_Delete(pJsonRoot);

    return  ret;
}

int app_req_dis_scen_name(payload_t data)
{
    M1_LOG_DEBUG("app_req_dis_scen_name\n"); 

    int pduType = TYPE_M1_REPORT_DIS_SCEN_NAME;
    int number, i;
    int rc, ret = M1_PROTOCOL_OK;
    char* scenario = NULL, *scen_pic = NULL;
    char* sql = (char*)malloc(300);
    char* sql_1 = (char*)malloc(300);
    char* sql_2 = (char*)malloc(300);
    cJSON* devDataJson = NULL;
    cJSON* pJsonRoot = NULL;
    cJSON* pduJsonObject = NULL;
    cJSON* devDataJsonArray = NULL;
    cJSON* devDataObject = NULL;
    cJSON* scenArray = NULL;
    cJSON* scenJson = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt *stmt = NULL,*stmt_1 = NULL,*stmt_2 = NULL;

    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    db = data.db;
    /*get sql data json*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_ERROR("pJsonRoot NULL\n");
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
    /*获取区域*/
    number = cJSON_GetArraySize(data.pdu);
    M1_LOG_DEBUG("number:%d\n",number);

    /*获取用户账户信息*/
    char* account = NULL;
    sprintf(sql_1,"select ACCOUNT from account_info where CLIENT_FD = %03d order by ID desc limit 1;",data.clientFd);
    M1_LOG_DEBUG( "%s\n", sql_1);
    if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    if(thread_sqlite3_step(&stmt_1, db) == SQLITE_ROW){
        account =  sqlite3_column_text(stmt_1, 0);
    }
    if(account == NULL){
        M1_LOG_ERROR( "user account do not exist\n");    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }else{
        M1_LOG_DEBUG("clientFd:%03d,account:%s\n",data.clientFd, account);
    }

    for(i = 0; i < number; i++){
        devDataJson = cJSON_GetArrayItem(data.pdu, i);
        M1_LOG_DEBUG("district:%s\n",devDataJson->valuestring);
        /*添加区域*/
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
        cJSON_AddStringToObject(devDataObject, "district", devDataJson->valuestring);
        /*添加场景*/
        scenArray = cJSON_CreateArray();
        if(NULL == scenArray)
        {
            // create object faild, exit
            M1_LOG_ERROR("scenArray NULL\n");
            cJSON_Delete(scenArray);
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }
        cJSON_AddItemToObject(devDataObject, "scenario", scenArray);
        /*获取场景*/
        sprintf(sql,"select distinct SCEN_NAME from scenario_table where DISTRICT = \"%s\" and ACCOUNT = \"%s\";",devDataJson->valuestring, account);
        M1_LOG_DEBUG( "%s\n", sql);
        sqlite3_finalize(stmt);
        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){

            scenJson = cJSON_CreateObject();
            if(NULL == scenJson)
            {
                // create object faild, exit
                M1_LOG_ERROR("devDataObject NULL\n");
                cJSON_Delete(scenJson);
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            /*添加场景名称到组中*/
            scenario = sqlite3_column_text(stmt, 0);
            if(scenario == NULL){
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            M1_LOG_DEBUG( "scenario:%s\n", scenario);
            cJSON_AddStringToObject(scenJson, "scenName", scenario);
            /*添加场景标识到组中*/
            sprintf(sql_2,"select SCEN_PIC from scenario_table where SCEN_NAME = \"%s\" order by ID desc limit 1;", scenario);
            sqlite3_finalize(stmt_2);
            if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK){
                M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
                ret = M1_PROTOCOL_FAILED;
                goto Finish; 
            }
            rc = thread_sqlite3_step(&stmt_2, db);
            if(rc != SQLITE_ROW)
                continue;
            scen_pic = sqlite3_column_text(stmt_2, 0);
            M1_LOG_DEBUG( "scen_pic:%s\n", scen_pic);
            cJSON_AddStringToObject(scenJson, "scenPic", scen_pic);

            cJSON_AddItemToArray(scenArray, scenJson);
        }

    }

    char* p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    M1_LOG_DEBUG("string:%s\n",p);
    socketSeverSend((unsigned char*)p, strlen(p), data.clientFd);
    
    Finish:
    free(sql);
    free(sql_1);
    free(sql_2);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
    sqlite3_finalize(stmt_2);
    cJSON_Delete(pJsonRoot);

    return  ret;
}

int app_req_dis_dev(payload_t data)
{
    M1_LOG_DEBUG("app_req_dis_dev\n");

    int pduType = TYPE_M1_REPORT_DIS_DEV; 
    int number, i;
    int rc,ret = M1_PROTOCOL_OK;
    char* account = NULL;
    char* sql = (char*)malloc(300);
    char* sql_1 = (char*)malloc(300);
    char* sql_2 = (char*)malloc(300);
    char* sql_3 = (char*)malloc(300);
    char* apId = NULL, *pId = NULL, *devName = NULL, *devId = NULL;
    cJSON* devDataJson = NULL;
    cJSON* pJsonRoot = NULL;
    cJSON* pduJsonObject = NULL;
    cJSON* devDataJsonArray = NULL;
    cJSON* devDataObject = NULL;
    cJSON* apInfoArray = NULL;
    cJSON* apInfoObject = NULL;
    cJSON* devArray = NULL;
    cJSON* devObject = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt *stmt = NULL,*stmt_1 = NULL,*stmt_2 = NULL,*stmt_3 = NULL;

    db = data.db;
    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*get sql data json*/
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
    /*获取区域*/
    number = cJSON_GetArraySize(data.pdu);
    M1_LOG_DEBUG("number:%d\n",number);

    /*获取用户账户信息*/
    sprintf(sql_3,"select ACCOUNT from account_info where CLIENT_FD = %03d order by ID desc limit 1;",data.clientFd);
    M1_LOG_DEBUG( "%s\n", sql_3);
    if(sqlite3_prepare_v2(db, sql_3, strlen(sql_3), &stmt_3, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    if(thread_sqlite3_step(&stmt_3, db) == SQLITE_ROW){
        account =  sqlite3_column_text(stmt_3, 0);
    }
    if(account == NULL){
        M1_LOG_ERROR( "user account do not exist\n");    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }else{
        M1_LOG_DEBUG("clientFd:%03d,account:%s\n",data.clientFd, account);
    }

    for(i = 0; i < number; i++){
        devDataJson = cJSON_GetArrayItem(data.pdu, i);
        if(devDataJson->valuestring == NULL){
            continue;
        }
        M1_LOG_DEBUG("district:%s\n",devDataJson->valuestring);
        /*添加区域*/
        devDataObject = cJSON_CreateObject();
        if(NULL == devDataObject)
        {
            // create object faild, exit
            M1_LOG_ERROR("devDataObject NULL\n");
            cJSON_Delete(devDataObject);
            ret =  M1_PROTOCOL_FAILED;
            goto Finish;
        }
        cJSON_AddItemToArray(devDataJsonArray, devDataObject);
        cJSON_AddStringToObject(devDataObject, "district", devDataJson->valuestring);
        /*添加场景*/
        apInfoArray = cJSON_CreateArray();
        if(NULL == apInfoArray)
        {
            // create object faild, exit
            M1_LOG_ERROR("apInfoArray NULL\n");
            cJSON_Delete(apInfoArray);
            ret =  M1_PROTOCOL_FAILED;
            goto Finish;
        }
        cJSON_AddItemToObject(devDataObject, "apInfo", apInfoArray);
        /*获取场景*/
        sprintf(sql,"select distinct AP_ID from district_table where DIS_NAME = \"%s\" and ACCOUNT = \"%s\";",devDataJson->valuestring, account);
        M1_LOG_DEBUG( "%s\n", sql);
        sqlite3_finalize(stmt);
        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
            apInfoObject = cJSON_CreateObject();
            if(NULL == apInfoObject)
            {
                // create object faild, exit
                M1_LOG_ERROR("apInfoObject NULL\n");
                cJSON_Delete(apInfoObject);
                ret =  M1_PROTOCOL_FAILED;
                goto Finish;
            }
            cJSON_AddItemToArray(apInfoArray, apInfoObject);
            /*apId*/
            apId = sqlite3_column_text(stmt, 0);
            if(apId == NULL){
                ret =  M1_PROTOCOL_FAILED;
                goto Finish;
            }
            M1_LOG_DEBUG( "ap_id:%s\n", apId);
            cJSON_AddStringToObject(apInfoObject, "apId", apId);
            /*pId,apName,apId*/
            sprintf(sql_1,"select PID, DEV_NAME from all_dev where DEV_ID = \"%s\" order by ID desc limit 1;", apId);
            M1_LOG_DEBUG( "%s\n", sql_1);
            sqlite3_finalize(stmt_1);
            if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK){
                M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
                ret = M1_PROTOCOL_FAILED;
                goto Finish; 
            }
            rc = thread_sqlite3_step(&stmt_1, db);
            //if(rc != SQLITE_ERROR){
            if(rc == SQLITE_ROW){
                pId = sqlite3_column_text(stmt_1, 0);
                if(pId != NULL)
                    cJSON_AddStringToObject(apInfoObject, "pId", pId);
                devName = sqlite3_column_text(stmt_1, 1);
                if(devName != NULL)
                    cJSON_AddStringToObject(apInfoObject, "apName", devName);
            }
            devArray = cJSON_CreateArray();
            if(NULL == devArray)
            {
                // create object faild, exit
                M1_LOG_ERROR("devArray NULL\n");
                cJSON_Delete(devArray);
                ret =  M1_PROTOCOL_FAILED;
                goto Finish;
            }
            cJSON_AddItemToObject(apInfoObject, "dev", devArray);
            sprintf(sql_2,"select PID, DEV_NAME, DEV_ID from all_dev where AP_ID = \"%s\" and ACCOUNT = \"%s\";",apId, account);
            M1_LOG_DEBUG( "%s\n", sql_2);
            sqlite3_finalize(stmt_2);
            if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK){
                M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
                ret = M1_PROTOCOL_FAILED;
                goto Finish; 
            }
            while(thread_sqlite3_step(&stmt_2, db) == SQLITE_ROW){
                /*添加设备*/
                devObject = cJSON_CreateObject();
                if(NULL == devObject)
                {
                    // create object faild, exit
                    M1_LOG_ERROR("devObject NULL\n");
                    cJSON_Delete(devObject);
                    ret =  M1_PROTOCOL_FAILED;
                    goto Finish;
                }
                cJSON_AddItemToArray(devArray, devObject);
                pId = sqlite3_column_text(stmt_2, 0);
                if(pId != NULL)
                    cJSON_AddStringToObject(devObject, "pId", pId);
                devName = sqlite3_column_text(stmt_2, 1);
                if(devName != NULL)
                    cJSON_AddStringToObject(devObject, "devName", devName);
                devId = sqlite3_column_text(stmt_2, 2);
                if(devId != NULL)
                    cJSON_AddStringToObject(devObject, "devId", devId);
            }
        }

    }

    char* p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        ret =  M1_PROTOCOL_FAILED;
        goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);
    socketSeverSend((unsigned char*)p, strlen(p), data.clientFd);

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

    return  ret;
}

int user_login_handle(payload_t data)
{
	M1_LOG_DEBUG("user_login_handle\n");

    int rc,ret = M1_PROTOCOL_OK;
    int id;
    char* sql = NULL;
    char* sql_1 = NULL;
    char* account = NULL,*key = NULL;
	cJSON* accountJson = NULL;
	cJSON* keyJson = NULL;
	sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* stmt_1 = NULL;
    sqlite3_stmt* stmt_2 = NULL;

    accountJson = cJSON_GetObjectItem(data.pdu, "account");
    M1_LOG_DEBUG("account:%s\n",accountJson->valuestring);
    keyJson = cJSON_GetObjectItem(data.pdu, "key");
    M1_LOG_DEBUG("key:%s\n",keyJson->valuestring);
    /*获取数据库*/
    db = data.db;
    /*验证用户信息*/
    sql = (char*)malloc(300);
    sprintf(sql,"select KEY from account_table where ACCOUNT = \"%s\";",accountJson->valuestring);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
	if(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
		key = sqlite3_column_text(stmt, 0);
	}else{
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
	}

    if(key == NULL){
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
    }
	if(strcmp(key,keyJson->valuestring) != 0){
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

	sql_1 = "select ID from account_info order by ID desc limit 1;";
	id = sql_id(db, sql_1);
	/*删除重复用户信息*/
    sprintf(sql,"delete from account_info where ACCOUNT = \"%s\" and CLIENT_FD = %03d;",accountJson->valuestring, data.clientFd);
    M1_LOG_DEBUG("sql:%s\n",sql);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt_1, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
	if(thread_sqlite3_step(&stmt_1, db) == SQLITE_ROW){
		account = sqlite3_column_text(stmt_1, 0);
	}
	/*插入用户信息*/	
	sql_1 = "insert into account_info(ID,ACCOUNT,CLIENT_FD)values(?,?,?);";
    M1_LOG_DEBUG("sql_1:%s\n",sql_1);
    if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_2, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
	sqlite3_bind_int(stmt_2, 1, id);
    sqlite3_bind_text(stmt_2, 2,  accountJson->valuestring, -1, NULL);
    sqlite3_bind_int(stmt_2, 3, data.clientFd);
    rc = thread_sqlite3_step(&stmt_2,db);
    if(rc == SQLITE_ERROR){
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    Finish:
    free(sql);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
    sqlite3_finalize(stmt_2);
    
	return 	ret;
}

/*更改用户登录密码*/
int app_change_user_key(payload_t data)
{
    M1_LOG_DEBUG("app_change_user_key\n");
    /*sqlite3*/
    int id;
    int ret = M1_PROTOCOL_OK;
    int clientFd;
    char* key = NULL;
    char* keyAuth = NULL;
    char* account = NULL;
    char* time = (char*)malloc(30);
    char* sql= (char*)malloc(300);
    char* sql1 = (char*)malloc(300);
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* stmt1 = NULL;
    sqlite3_stmt* stmt2 = NULL;
    cJSON* KeyJson = NULL;
    cJSON* newKeyJson = NULL;
    cJSON* confirmKeyJson = NULL;
    
    clientFd = data.clientFd;
    getNowTime(time);
    KeyJson = cJSON_GetObjectItem(data.pdu, "Key");
    if(KeyJson == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;   
    }
    M1_LOG_DEBUG("Key:%s\n",KeyJson->valuestring);
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
    /*获取用户登录名称*/
    sprintf(sql1,"select ACCOUNT from account_info where CLIENT_FD = %03d order by ID desc limit 1;", clientFd);
    M1_LOG_DEBUG( "%s\n", sql1);
    if(sqlite3_prepare_v2(db, sql1, strlen(sql1), &stmt1, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    if(thread_sqlite3_step(&stmt1, db) == SQLITE_ERROR){
        M1_LOG_ERROR( "SQLITE_ERROR\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    account = sqlite3_column_text(stmt1, 0);
    if(account == NULL){
        M1_LOG_ERROR( "get account error\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*获取账户信息*/
    sprintf(sql,"select KEY, KEY_AUTH from account_table where account = \"%s\" order by ID desc limit 1;", account);
    M1_LOG_DEBUG( "%s\n", sql);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    if(thread_sqlite3_step(&stmt, db) == SQLITE_ERROR){
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
    // id = sqlite3_column_int(stmt, 1);
    // if(id == NULL){
    //     M1_LOG_ERROR( "get id error\n");
    //     ret = M1_PROTOCOL_FAILED;
    //     goto Finish;
    // }
    keyAuth = sqlite3_column_text(stmt, 1);
    if(keyAuth == NULL){
        M1_LOG_ERROR( "get keyAuth error\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*验证用户修改权限*/
    if(strcmp(keyAuth, "on") != 0){
        M1_LOG_ERROR( "keyAuth not permitied\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*验证密码是否匹配*/
    if(strcmp(key, KeyJson->valuestring) != 0){
        M1_LOG_ERROR( "key not match\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    sprintf(sql,"update account_table set KEY = \"%s\" where account = \"%s\";",newKeyJson->valuestring, account);
    M1_LOG_DEBUG( "%s\n", sql);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt2, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    if(thread_sqlite3_step(&stmt2, db) == SQLITE_ERROR){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    Finish:
    free(time);
    free(sql);
    free(sql1);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt1);
    sqlite3_finalize(stmt2);

    return ret; 
}
