#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"
#include "sqlite3.h"
#include "account_config.h"
#include "m1_protocol.h"

int app_req_account_info_handle(payload_t data, int sn)
{
	cJSON* pJsonRoot = NULL;
	cJSON* pduJsonObject = NULL;
	cJSON* accountJson = NULL;
	cJSON* devDataJsonArray = NULL;

	char* account = NULL;
	int pduType = TYPE_M1_REPORT_ACCOUNT_INFO;

	fprintf(stdout,"account_info_handle”\n");
	
	sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int rc;
    char* sql = "select ACCOUNT from account_table order by ID";

    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    /*get sql data json*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        fprintf(stdout,"pJsonRoot NULL\n");
        cJSON_Delete(pJsonRoot);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
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
        sqlite3_finalize(stmt);
        sqlite3_close(db);
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
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return M1_PROTOCOL_FAILED;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataJsonArray);
    fprintf(stdout,"%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
	    /*create pdu object*/	    
	    account = sqlite3_column_text(stmt,0);
	    fprintf(stdout,"account:%s\n",account);
	    accountJson = cJSON_CreateString(account);
	    if(NULL == pduJsonObject)
	    {
	        // create object faild, exit
	        cJSON_Delete(accountJson);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
	        return M1_PROTOCOL_FAILED;
	    }
	    cJSON_AddItemToArray(devDataJsonArray, accountJson);
	}

	sqlite3_finalize(stmt);
    sqlite3_close(db);

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    fprintf(stdout,"string:%s\n",p);
    /*response to client*/
    socketSeverSend((unsigned char*)p, strlen(p), data.clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;

}

int app_req_account_config_handle(payload_t data, int sn)
{
	fprintf(stdout,"app_req_account_config_handle\n");
	cJSON* pJsonRoot = NULL;
	cJSON* pduJsonObject = NULL;
	cJSON* devDataObject = NULL;
	cJSON* accountJson = NULL;
	cJSON* districtArray = NULL;
	cJSON* districtJson = NULL;
	cJSON* scenArray = NULL;
	cJSON* scenJson = NULL;
	cJSON* devArray = NULL;
	cJSON* devObject = NULL;

	char* account = NULL;
	int pduType = TYPE_M1_REPORT_ACCOUNT_CONFIG_INFO;

	sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int rc;
    char sql[200];

    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    /*get sql data json*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        fprintf(stdout,"pJsonRoot NULL\n");
        cJSON_Delete(pJsonRoot);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
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
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return M1_PROTOCOL_FAILED;
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
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return M1_PROTOCOL_FAILED;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataObject);
    /*获取账户名*/
    char *key = NULL, *key_auth = NULL, *remote_auth = NULL, *district = NULL,*scenario = NULL,
    *dev_name = NULL,*dev_id = NULL;
    int pId;
    accountJson = data.pdu;//cJSON_GetObjectItem(data.pdu, "devData");
    sprintf(sql,"select KEY,KEY_AUTH,REMOTE_AUTH from account_table where ACCOUNT = \"%s\";", accountJson->valuestring);
    fprintf(stdout,"%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
	    /*create pdu object*/	    
	    key = sqlite3_column_text(stmt,0);
	    key_auth = sqlite3_column_text(stmt,1);
	    remote_auth = sqlite3_column_text(stmt,2);
	    fprintf(stdout,"key:%s,key_auth:%s, remote_auth:%s\n",key, key_auth, remote_auth);
	    cJSON_AddStringToObject(devDataObject, "key", key);
	    cJSON_AddStringToObject(devDataObject, "key_auth", key_auth);
	    cJSON_AddStringToObject(devDataObject, "remote_auth", remote_auth);
	}
    /*获取区域信息*/
	districtArray = cJSON_CreateArray();
    if(NULL == districtArray)
    {
        cJSON_Delete(districtArray);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return M1_PROTOCOL_FAILED;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(devDataObject, "district", districtArray);
    sprintf(sql,"select distinct DIS_NAME from district_table where ACCOUNT = \"%s\";", accountJson->valuestring);
    fprintf(stdout,"%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
	    /*添加区域信息*/	    
	    district = sqlite3_column_text(stmt,0);
	   	fprintf(stdout,"district:%s\n", district);
	    districtJson = cJSON_CreateString(district);
	    if(NULL == districtJson)
	    {
	        // create object faild, exit
	        cJSON_Delete(districtJson);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
	        return M1_PROTOCOL_FAILED;
	    }
	    cJSON_AddItemToArray(districtArray, districtJson);
	}
	/*获取场景信息*/
	scenArray = cJSON_CreateArray();
    if(NULL == scenArray)
    {
        cJSON_Delete(scenArray);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return M1_PROTOCOL_FAILED;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(devDataObject, "scenario", scenArray);
    sprintf(sql,"select distinct SCEN_NAME from scenario_table where ACCOUNT = \"%s\";", accountJson->valuestring);
    fprintf(stdout,"%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
	    /*添加区域信息*/	    
	    scenario = sqlite3_column_text(stmt,0);
	   	fprintf(stdout,"scenario:%s\n", scenario);
	    scenJson = cJSON_CreateString(scenario);
	    if(NULL == scenJson)
	    {
	        // create object faild, exit
	        cJSON_Delete(scenJson);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
	        return M1_PROTOCOL_FAILED;
	    }
	    cJSON_AddItemToArray(scenArray, scenJson);
	}
	/*获取设备信息*/
	devArray = cJSON_CreateArray();
    if(NULL == devArray)
    {
        cJSON_Delete(devArray);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return M1_PROTOCOL_FAILED;
    }
    cJSON_AddItemToObject(devDataObject, "device", devArray);
    sprintf(sql,"select PID, DEV_NAME, DEV_ID from all_dev where ACCOUNT = \"%s\";", accountJson->valuestring);
    fprintf(stdout,"%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
	    /*添加区域信息*/	    
	    pId = sqlite3_column_int(stmt,0);
	    dev_name = sqlite3_column_text(stmt,1);
	    dev_id = sqlite3_column_text(stmt,2);
	   	fprintf(stdout,"pid:%05d,dev_name:%s, dev_id:%s\n", pId, dev_name,dev_id);
	    devObject = cJSON_CreateObject();
	    if(NULL == devObject)
	    {
	        // create object faild, exit
	        cJSON_Delete(devObject);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
	        return M1_PROTOCOL_FAILED;
	    }
	    cJSON_AddNumberToObject(devObject, "pId", pId);
	    cJSON_AddStringToObject(devObject, "devName", dev_name);
	    cJSON_AddStringToObject(devObject, "devId", dev_id);
	    cJSON_AddItemToArray(devArray, devObject);
	}

	sqlite3_finalize(stmt);
    sqlite3_close(db);

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    fprintf(stdout,"string:%s\n",p);
    /*response to client*/
    socketSeverSend((unsigned char*)p, strlen(p), data.clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;

}

int app_account_config_handle(payload_t data)
{
    fprintf(stdout,"app_account_config_handle\n");
    cJSON* districtArray = NULL;
    cJSON* districtObject = NULL;
    cJSON* scenArray = NULL;
    cJSON* scenObject = NULL;
    cJSON* devArray = NULL;
    cJSON* devObject = NULL;
    cJSON* accountJson = NULL;
    cJSON* keyJson = NULL;
    cJSON* keyAuthJson = NULL;
    cJSON* remoteAuthJson = NULL;
    
    int i,rc,id,number;
    char* sql;
    char sql_1[200];
    char time[30];
    char* ap_id = NULL,*dev_id = NULL, *district = NULL,
        *dev_name = NULL,*status = NULL;
    int type, value, delay, pid,added,net;
    /*获取时间*/
    getNowTime(time);

    sqlite3* db = NULL;
    sqlite3_stmt *stmt = NULL, *stmt_1 = NULL;
    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    accountJson = cJSON_GetObjectItem(data.pdu,"account");
    keyJson = cJSON_GetObjectItem(data.pdu,"key");
    keyAuthJson = cJSON_GetObjectItem(data.pdu,"keyAuthority");
    remoteAuthJson = cJSON_GetObjectItem(data.pdu,"remoteAuthority");
    fprintf(stdout, "account:%s,key:%s,keyAuthority:%s,remoteAuthority:%s\n", accountJson->valuestring,
        keyJson->valuestring,keyAuthJson->valuestring,remoteAuthJson->valuestring);

    sql = "select ID from account_table order by ID desc limit 1;";
    id = sql_id(db, sql);
    /*删除重复用户信息*/
    sprintf(sql_1,"delete from account_table where ACCOUNT = \"%s\";",accountJson->valuestring);
    fprintf(stdout,"sql:%s\n",sql_1);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql_1, strlen(sql), &stmt, NULL);
    thread_sqlite3_step(&stmt, db);
    
    /*添加账户信息*/
    sql = "insert into account_table(ID, ACCOUNT, KEY, KEY_AUTH, REMOTE_AUTH, TIME)values(?,?,?,?,?,?);";
    fprintf(stdout,"sql:%s\n",sql);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_text(stmt, 2,  accountJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt, 3,  keyJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt, 4,  keyAuthJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt, 5,  remoteAuthJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt, 6,  time, -1, NULL);
    rc = thread_sqlite3_step(&stmt,db);
    if(rc == SQLITE_ERROR){
        sqlite3_finalize(stmt);
        sqlite3_finalize(stmt_1);
        sqlite3_close(db);
        return M1_PROTOCOL_FAILED;
    }

    /*添加用户区域信息*/
    /*删除重复用户信息*/
    sprintf(sql_1,"delete from district_table where ACCOUNT = \"%s\";",accountJson->valuestring);
    fprintf(stdout,"sql:%s\n",sql_1);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
    thread_sqlite3_step(&stmt, db);
    //while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW);

    sql = "select ID from district_table order by ID desc limit 1;";
    id = sql_id(db, sql);
    districtArray = cJSON_GetObjectItem(data.pdu,"district");
    number = cJSON_GetArraySize(districtArray);
    for(i = 0; i < number; i++){
        districtObject = cJSON_GetArrayItem(districtArray, i);
        fprintf(stdout,"district:%s\n",districtObject->valuestring);
        sprintf(sql_1,"select AP_ID from district_table where DIS_NAME = \"%s\" and ACCOUNT = \"Dalitek\";",districtObject->valuestring);
        fprintf(stdout,"sql:%s\n",sql_1);
        sqlite3_reset(stmt);
        sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
        while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
            ap_id = sqlite3_column_text(stmt, 0);
            sql = "insert into district_table(ID, DIS_NAME, AP_ID, ACCOUNT, TIME)values(?,?,?,?,?);";
            fprintf(stdout,"sql:%s\n",sql);
            sqlite3_reset(stmt_1);
            sqlite3_prepare_v2(db, sql, strlen(sql), &stmt_1, NULL);

            sqlite3_bind_int(stmt_1, 1, id);
            sqlite3_bind_text(stmt_1, 2,  districtObject->valuestring, -1, NULL);
            sqlite3_bind_text(stmt_1, 3, ap_id, -1, NULL);
            sqlite3_bind_text(stmt_1, 4,accountJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt_1, 5,  time, -1, NULL);
            rc = thread_sqlite3_step(&stmt_1, db);
            if(rc == M1_PROTOCOL_FAILED){
                sqlite3_finalize(stmt);
                sqlite3_finalize(stmt_1);
                sqlite3_close(db);
                return M1_PROTOCOL_FAILED;
            }
            id++;
        }        
    
    }
    /*添加用户场景信息*/
    /*删除重复用户信息*/
    sprintf(sql_1,"delete from scenario_table where ACCOUNT = \"%s\";",accountJson->valuestring);
    fprintf(stdout,"sql:%s\n",sql_1);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
    thread_sqlite3_step(&stmt, db);
    //while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW);

    sql = "select ID from scenario_table order by ID desc limit 1;";
    id = sql_id(db, sql);
    scenArray = cJSON_GetObjectItem(data.pdu,"scenario");
    number = cJSON_GetArraySize(scenArray);
    for(i = 0; i < number; i++){
        scenObject = cJSON_GetArrayItem(scenArray, i);
        fprintf(stdout,"scenario:%s\n",scenObject->valuestring);
        sprintf(sql_1,"select DISTRICT,AP_ID,DEV_ID,TYPE,VALUE,DELAY from scenario_table where SCEN_NAME = \"%s\" and ACCOUNT = \"Dalitek\";",scenObject->valuestring);
        fprintf(stdout,"sql:%s\n",sql_1);
        sqlite3_reset(stmt);
        sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
        while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
            district = sqlite3_column_text(stmt, 0);
            ap_id = sqlite3_column_text(stmt, 1);
            dev_id = sqlite3_column_text(stmt, 2);
            type = sqlite3_column_int(stmt, 3);
            value = sqlite3_column_int(stmt, 4);
            delay = sqlite3_column_int(stmt, 5);
            /*插入到场景表中*/
            sql = "insert into scenario_table(ID, SCEN_NAME, DISTRICT, AP_ID, DEV_ID, TYPE, VALUE, DELAY, ACCOUNT, TIME)values(?,?,?,?,?,?,?,?,?,?);";
            fprintf(stdout,"sql:%s\n",sql);
            sqlite3_reset(stmt_1);
            sqlite3_prepare_v2(db, sql, strlen(sql), &stmt_1, NULL);

            sqlite3_bind_int(stmt_1, 1, id);
            sqlite3_bind_text(stmt_1, 2,  scenObject->valuestring, -1, NULL);
            sqlite3_bind_text(stmt_1, 3,  district, -1, NULL);
            sqlite3_bind_text(stmt_1, 4,  ap_id, -1, NULL);
            sqlite3_bind_text(stmt_1, 5,  dev_id, -1, NULL);
            sqlite3_bind_int(stmt_1, 6, type);
            sqlite3_bind_int(stmt_1, 7, value);
            sqlite3_bind_int(stmt_1, 8, delay);
            sqlite3_bind_text(stmt_1, 9, accountJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt_1, 10, time, -1, NULL);
            rc = thread_sqlite3_step(&stmt_1, db);
            if(rc == M1_PROTOCOL_FAILED){
                sqlite3_finalize(stmt);
                sqlite3_finalize(stmt_1);
                sqlite3_close(db);
                return M1_PROTOCOL_FAILED;
            }
            id++;
        }        

    }
    /*添加用户设备信息*/
    sprintf(sql_1,"delete from all_dev where ACCOUNT = \"%s\";",accountJson->valuestring);
    fprintf(stdout,"sql:%s\n",sql_1);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
    thread_sqlite3_step(&stmt, db);
    //while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW);

    sql = "select ID from all_dev order by ID desc limit 1;";
    id = sql_id(db, sql);
    devArray = cJSON_GetObjectItem(data.pdu,"device");
    number = cJSON_GetArraySize(devArray);
    for(i = 0; i < number; i++){
        devObject = cJSON_GetArrayItem(devArray, i);
        fprintf(stdout,"device:%s\n",devObject->valuestring);
        sprintf(sql_1,"select DEV_NAME,AP_ID,PID,ADDED,NET,STATUS from all_dev where DEV_ID = \"%s\" and ACCOUNT = \"Dalitek\";",devObject->valuestring);
        fprintf(stdout,"sql:%s\n",sql_1);
        sqlite3_reset(stmt);
        sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
        while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
            dev_name = sqlite3_column_text(stmt, 0);
            ap_id = sqlite3_column_text(stmt, 1);
            pid = sqlite3_column_int(stmt, 2);
            added = sqlite3_column_int(stmt, 3);
            net = sqlite3_column_int(stmt, 4);
            status = sqlite3_column_text(stmt, 5);
            /*插入到场景表中*/
            sql = "insert into all_dev(ID,DEV_ID,DEV_NAME,AP_ID,PID,ADDED,NET,STATUS,ACCOUNT,TIME)values(?,?,?,?,?,?,?,?,?,?);";
            fprintf(stdout,"sql:%s\n",sql);
            sqlite3_reset(stmt_1);
            sqlite3_prepare_v2(db, sql, strlen(sql), &stmt_1, NULL);

            sqlite3_bind_int(stmt_1, 1, id);
            sqlite3_bind_text(stmt_1, 2,  devObject->valuestring, -1, NULL);
            sqlite3_bind_text(stmt_1, 3,  dev_name, -1, NULL);
            sqlite3_bind_text(stmt_1, 4,  ap_id, -1, NULL);
            sqlite3_bind_int(stmt_1, 5, pid);
            sqlite3_bind_int(stmt_1, 6, added);
            sqlite3_bind_int(stmt_1, 7, net);
            sqlite3_bind_text(stmt_1, 8, status, -1, NULL);
            sqlite3_bind_text(stmt_1, 9, accountJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt_1, 10, time, -1, NULL);
            rc = thread_sqlite3_step(&stmt_1, db);
            if(rc == M1_PROTOCOL_FAILED){
                sqlite3_finalize(stmt);
                sqlite3_finalize(stmt_1);
                sqlite3_close(db);
                return M1_PROTOCOL_FAILED;
            }
            id++;
        }        

    }

    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
    sqlite3_close(db);

    return  M1_PROTOCOL_OK;
}

int app_req_dis_name(int clientFd, int sn)
{
    fprintf(stdout,"app_req_dis_name\n"); 
    int pdu_type = TYPE_M1_REPORT_DIS_NAME;
    int rc;
    char* district = NULL;
    cJSON * pJsonRoot = NULL; 
    cJSON * pduJsonObject = NULL;
    cJSON * devDataObject= NULL;
    cJSON * districtObject= NULL;
    cJSON * devDataJsonArray = NULL;

    sqlite3* db = NULL;
    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }
    /*get sql data json*/
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
    cJSON_AddNumberToObject(pduJsonObject, "pduType", pdu_type);
    /*添加devData*/
    devDataJsonArray = cJSON_CreateArray();
    if(NULL == devDataJsonArray)
    {
        cJSON_Delete(devDataJsonArray);
        return M1_PROTOCOL_FAILED;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataJsonArray);

    /*获取项目信息*/
    char* sql = "select distinct DIS_NAME from district_table";
    fprintf(stdout, "%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
        district = sqlite3_column_text(stmt, 0);
        if(district == NULL){
            fprintf(stderr, "%s\n", district);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return M1_PROTOCOL_FAILED;
        }
        districtObject = cJSON_CreateString(district);
        cJSON_AddItemToArray(devDataJsonArray, districtObject);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    char* p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    fprintf(stdout,"string:%s\n",p);
    socketSeverSend((unsigned char*)p, strlen(p), clientFd);
    cJSON_Delete(pJsonRoot);

    return  M1_PROTOCOL_OK;
}

int app_req_dis_scen_name(payload_t data, int sn)
{
    fprintf(stdout,"app_req_dis_scen_name\n"); 
    cJSON* devDataJson = NULL;
    cJSON* pJsonRoot = NULL;
    cJSON* pduJsonObject = NULL;
    cJSON* devDataJsonArray = NULL;
    cJSON* devDataObject = NULL;
    cJSON* scenArray = NULL;
    cJSON* scenJson = NULL;

    int pduType = TYPE_M1_REPORT_DIS_SCEN_NAME;
    int number, i, rc;
    char* scenario = NULL;
    char sql[200];

    /*get sql data json*/
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

    sqlite3* db = NULL;
    sqlite3_stmt *stmt = NULL;
    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }
    /*获取区域*/
    number = cJSON_GetArraySize(data.pdu);
    fprintf(stdout,"number:%d\n",number);
    for(i = 0; i < number; i++){
        devDataJson = cJSON_GetArrayItem(data.pdu, i);
        fprintf(stdout,"district:%s\n",devDataJson->valuestring);
        /*添加区域*/
        devDataObject = cJSON_CreateObject();
        if(NULL == devDataObject)
        {
            // create object faild, exit
            fprintf(stdout,"devDataObject NULL\n");
            cJSON_Delete(devDataObject);
            return M1_PROTOCOL_FAILED;
        }
        cJSON_AddItemToArray(devDataJsonArray, devDataObject);
        cJSON_AddStringToObject(devDataObject, "district", devDataJson->valuestring);
        /*添加场景*/
        scenArray = cJSON_CreateArray();
        if(NULL == scenArray)
        {
            // create object faild, exit
            fprintf(stdout,"scenArray NULL\n");
            cJSON_Delete(scenArray);
            return M1_PROTOCOL_FAILED;
        }
        cJSON_AddItemToObject(devDataObject, "scenario", scenArray);
        /*获取场景*/
        sprintf(sql,"select distinct SCEN_NAME from scenario_table where DISTRICT = \"%s\";",devDataJson->valuestring);
        fprintf(stdout, "%s\n", sql);
        sqlite3_reset(stmt);
        sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
            scenario = sqlite3_column_text(stmt, 0);
            if(scenario == NULL){
                sqlite3_finalize(stmt);
                sqlite3_close(db);
                return M1_PROTOCOL_FAILED;
            }
            fprintf(stdout, "scenario:%s\n", scenario);
            scenJson = cJSON_CreateString(scenario);
            cJSON_AddItemToArray(scenArray, scenJson);
        }

    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    char* p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    fprintf(stdout,"string:%s\n",p);
    socketSeverSend((unsigned char*)p, strlen(p), data.clientFd);
    cJSON_Delete(pJsonRoot);

    return  M1_PROTOCOL_OK;
}

int app_req_dis_dev(payload_t data, int sn)
{
    fprintf(stdout,"app_req_dis_dev\n");
    cJSON* devDataJson = NULL;
    cJSON* pJsonRoot = NULL;
    cJSON* pduJsonObject = NULL;
    cJSON* devDataJsonArray = NULL;
    cJSON* devDataObject = NULL;
    cJSON* apInfoArray = NULL;
    cJSON* apInfoObject = NULL;
    cJSON* devArray = NULL;
    cJSON* devObject = NULL;
    int pduType = TYPE_M1_REPORT_DIS_DEV; 

    int number, i, rc;
    char* apId = NULL, *pId = NULL, *devName = NULL, *devId = NULL;
    char sql[200],sql_1[200],sql_2[200];

    /*get sql data json*/
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

    sqlite3* db = NULL;
    sqlite3_stmt *stmt = NULL,*stmt_1 = NULL,*stmt_2 = NULL;
    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }
    /*获取区域*/
    number = cJSON_GetArraySize(data.pdu);
    fprintf(stdout,"number:%d\n",number);
    for(i = 0; i < number; i++){
        devDataJson = cJSON_GetArrayItem(data.pdu, i);
        fprintf(stdout,"district:%s\n",devDataJson->valuestring);
        /*添加区域*/
        devDataObject = cJSON_CreateObject();
        if(NULL == devDataObject)
        {
            // create object faild, exit
            fprintf(stdout,"devDataObject NULL\n");
            cJSON_Delete(devDataObject);
            return M1_PROTOCOL_FAILED;
        }
        cJSON_AddItemToArray(devDataJsonArray, devDataObject);
        cJSON_AddStringToObject(devDataObject, "district", devDataJson->valuestring);
        /*添加场景*/
        apInfoArray = cJSON_CreateArray();
        if(NULL == apInfoArray)
        {
            // create object faild, exit
            fprintf(stdout,"apInfoArray NULL\n");
            cJSON_Delete(apInfoArray);
            return M1_PROTOCOL_FAILED;
        }
        cJSON_AddItemToObject(devDataObject, "apInfo", apInfoArray);
        /*获取场景*/
        sprintf(sql,"select distinct AP_ID from district_table where DIS_NAME = \"%s\";",devDataJson->valuestring);
        fprintf(stdout, "%s\n", sql);
        sqlite3_reset(stmt);
        sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
            apInfoObject = cJSON_CreateObject();
            if(NULL == apInfoObject)
            {
                // create object faild, exit
                fprintf(stdout,"apInfoObject NULL\n");
                cJSON_Delete(apInfoObject);
                return M1_PROTOCOL_FAILED;
            }
            cJSON_AddItemToArray(apInfoArray, apInfoObject);
            /*apId*/
            apId = sqlite3_column_text(stmt, 0);
            if(apId == NULL){
                sqlite3_finalize(stmt);
                sqlite3_close(db);
                return M1_PROTOCOL_FAILED;
            }
            fprintf(stdout, "ap_id:%s\n", apId);
            cJSON_AddStringToObject(apInfoObject, "apId", apId);
            /*pId,apName,apId*/
            sprintf(sql_1,"select PID, DEV_NAME from all_dev where DEV_ID = \"%s\" order by ID desc limit 1;", apId);
            fprintf(stdout, "%s\n", sql_1);
            sqlite3_reset(stmt_1);
            sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
            rc = thread_sqlite3_step(&stmt_1, db);
            if(rc != SQLITE_ERROR){
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
                fprintf(stdout,"devArray NULL\n");
                cJSON_Delete(devArray);
                return M1_PROTOCOL_FAILED;
            }
            cJSON_AddItemToObject(apInfoObject, "dev", devArray);
            /*添加设备*/
            devObject = cJSON_CreateObject();
            if(NULL == devObject)
            {
                // create object faild, exit
                fprintf(stdout,"devObject NULL\n");
                cJSON_Delete(devObject);
                return M1_PROTOCOL_FAILED;
            }
            cJSON_AddItemToArray(apInfoArray, devObject);
            sprintf(sql_2,"select PID, DEV_NAME, DEV_ID from all_dev where AP_ID = \"%s\" and ACCOUNT = \"Dalitek\";");
            fprintf(stdout, "%s\n", sql_2);
            sqlite3_reset(stmt_2);
            sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
            while(thread_sqlite3_step(&stmt_2, db) == SQLITE_ROW){
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

    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
    sqlite3_finalize(stmt_2);
    sqlite3_close(db);

    char* p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    fprintf(stdout,"string:%s\n",p);
    socketSeverSend((unsigned char*)p, strlen(p), data.clientFd);
    cJSON_Delete(pJsonRoot);   

    return  M1_PROTOCOL_OK;
}

int user_login_handle(payload_t data)
{
	fprintf(stdout,"user_login_handle\n");
	cJSON* accountJson = NULL;
	cJSON* keyJson = NULL;
	char* account = NULL,*key = NULL;
	sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int rc,id;
    char sql[200];

    accountJson = cJSON_GetObjectItem(data.pdu, "account");
    fprintf(stdout,"account:%s\n",accountJson->valuestring);
    keyJson = cJSON_GetObjectItem(data.pdu, "key");
    fprintf(stdout,"key:%s\n",keyJson->valuestring);

    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    /*验证用户信息*/
    sprintf(sql,"select KEY from account_table where ACCOUNT = \"%s\";",accountJson->valuestring);
    sqlite3_reset(stmt);
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
		key = sqlite3_column_text(stmt, 0);
	}else{
        sqlite3_finalize(stmt);
        sqlite3_close(db);
		return M1_PROTOCOL_FAILED;  
	}
	if(strcmp(key,keyJson->valuestring) != 0){
        sqlite3_finalize(stmt);
        sqlite3_close(db);
		return M1_PROTOCOL_FAILED;  	
    }

	char* sql_1 = "select ID from account_info order by ID desc limit 1;";
	id = sql_id(db, sql_1);
	/*删除重复用户信息*/
    sprintf(sql,"delete from account_info where ACCOUNT = \"%s\";",accountJson->valuestring);
    fprintf(stdout,"sql:%s\n",sql);
    sqlite3_reset(stmt);
	sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
	if(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
		account = sqlite3_column_text(stmt, 0);
	}
	/*插入用户信息*/	
	sql_1 = "insert into account_info(ID,ACCOUNT,CLIENT_FD)values(?,?,?);";
    fprintf(stdout,"sql_1:%s\n",sql_1);
    sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
	sqlite3_reset(stmt);
	sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_text(stmt, 2,  accountJson->valuestring, -1, NULL);
    sqlite3_bind_int(stmt, 3, data.clientFd);
    rc = thread_sqlite3_step(&stmt,db);
    if(rc == SQLITE_ERROR){
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    	return M1_PROTOCOL_FAILED;
    }


	return 	M1_PROTOCOL_OK;
}


