#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

#include "m1_protocol.h"
#include "socket_server.h"
#include "buf_manage.h"

#define M1_PROTOCOL_DEBUG    1

#define HEAD_LEN    3


static int AP_report_data_handle(payload_t data);
static int APP_read_handle(payload_t data, int sn);
static int APP_write_handle(payload_t data);
static int M1_write_to_AP(cJSON* data);
static int APP_echo_dev_info_handle(payload_t data);
static int APP_req_added_dev_info_handle(int clientFd, int sn);
static int APP_net_control(payload_t data);
static int M1_report_dev_info(payload_t data, int sn);
static int M1_report_ap_info(int clientFd, int sn);
static int AP_report_dev_handle(payload_t data);
static int AP_report_ap_handle(payload_t data);
static int common_operate(payload_t data);
static int common_rsp(rsp_data_t data);
static int ap_heartbeat_handle(payload_t data);
static int common_rsp_handle(payload_t data);

char* db_path = "dev_info.db";
fifo_t dev_data_fifo;
fifo_t link_exec_fifo;
fifo_t msg_fifo;
fifo_t tx_fifo;
/*优先级队列*/
PNode head;
static uint32_t dev_data_buf[256];
static uint32_t link_exec_buf[256];
static uint32_t msg_buf[256];
static uint32_t tx_buf[256];

void m1_protocol_init(void)
{
    sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
    fifo_init(&dev_data_fifo, dev_data_buf, 256);
    fifo_init(&link_exec_fifo, link_exec_buf, 256);
    fifo_init(&msg_fifo, msg_buf, 256);
    fifo_init(&tx_fifo, tx_buf, 256);
    Init_PQueue(&head);
}

//void data_handle(m1_package_t* package)
void data_handle(void)
{
    fprintf(stdout,"data_handle\n");
    int rc = M1_PROTOCOL_NO_RSP;
    payload_t pdu;
    rsp_data_t rspData;
    cJSON* rootJson = NULL;
    cJSON* pduJson = NULL;
    cJSON* pduTypeJson = NULL;
    cJSON* snJson = NULL;
    cJSON* pduDataJson = NULL;
    int pduType;

    uint32_t* msg = NULL;
    fifo_read(&msg_fifo, &msg);
    m1_package_t* package = (m1_package_t*)msg;
    fprintf(stdout,"Rx message:%s\n",package->data);
    rootJson = cJSON_Parse(package->data);
    if(NULL == rootJson){
        fprintf(stdout,"rootJson null\n");
        return;
    }
    pduJson = cJSON_GetObjectItem(rootJson, "pdu");
    if(NULL == pduJson){
        fprintf(stdout,"pdu null\n");
        return;
    }
    pduTypeJson = cJSON_GetObjectItem(pduJson, "pduType");
    if(NULL == pduTypeJson){
        fprintf(stdout,"pduType null\n");
        return;
    }
    pduType = pduTypeJson->valueint;
    rspData.pduType = pduType;

    snJson = cJSON_GetObjectItem(rootJson, "sn");
    if(NULL == snJson){
        fprintf(stdout,"sn null\n");
        return;
    }
    rspData.sn = snJson->valueint;

    pduDataJson = cJSON_GetObjectItem(pduJson, "devData");
    if(NULL == pduDataJson){
        fprintf(stdout,"devData null”\n");

    }
    /*pdu*/ 
    pdu.clientFd = package->clientFd;
    pdu.pdu = pduDataJson;

    rspData.clientFd = package->clientFd;
    fprintf(stdout,"pduType:%x\n",pduType);
    switch(pduType){
        case TYPE_REPORT_DATA: rc = AP_report_data_handle(pdu); break;
        case TYPE_DEV_READ: APP_read_handle(pdu, rspData.sn); break;
        case TYPE_DEV_WRITE: rc = APP_write_handle(pdu); if(rc != M1_PROTOCOL_FAILED) M1_write_to_AP(rootJson);break;
        case TYPE_ECHO_DEV_INFO: rc = APP_echo_dev_info_handle(pdu); break;
        case TYPE_REQ_ADDED_INFO: APP_req_added_dev_info_handle(rspData.clientFd , rspData.sn); break;
        case TYPE_DEV_NET_CONTROL: rc = APP_net_control(pdu); break;
        case TYPE_REQ_AP_INFO: M1_report_ap_info(rspData.clientFd , rspData.sn); break;
        case TYPE_REQ_DEV_INFO: M1_report_dev_info(pdu, rspData.sn); break;
        case TYPE_AP_REPORT_DEV_INFO: rc = AP_report_dev_handle(pdu); break;
        case TYPE_AP_REPORT_AP_INFO: rc = AP_report_ap_handle(pdu); break;
        case TYPE_COMMON_RSP: common_rsp_handle(pdu);break;
        case TYPE_CREATE_LINKAGE: rc = linkage_msg_handle(pdu);break;
        case TYPE_CREATE_SCENARIO: rc = scenario_create_handle(pdu);break;
        case TYPE_CREATE_DISTRICT: rc = district_create_handle(pdu);break;
        case TYPE_SCENARIO_ALARM: rc = scenario_alarm_create_handle(pdu);break;
        case TYPE_COMMON_OPERATE: rc = common_operate(pdu);break;
        case TYPE_REQ_SCEN_INFO: rc = app_req_scenario(rspData.clientFd, rspData.sn);break;
        case TYPE_REQ_LINK_INFO: rc = app_req_linkage(rspData.clientFd, rspData.sn);break;
        case TYPE_REQ_DISTRICT_INFO: rc = app_req_district(rspData.clientFd, rspData.sn); break;
        case TYPE_REQ_SCEN_NAME_INFO: rc = app_req_scenario_name(rspData.clientFd, rspData.sn);break;
        case TYPE_AP_HEARTBEAT_INFO: rc = ap_heartbeat_handle(pdu);break;
        case TYPE_REQ_ACCOUNT_INFO: rc = app_req_account_info_handle(pdu, rspData.sn);break;

        default: fprintf(stdout,"pdu type not match\n"); rc = M1_PROTOCOL_FAILED;break;
    }

    if(rc != M1_PROTOCOL_NO_RSP){
        if(rc == M1_PROTOCOL_OK)
            rspData.result = RSP_OK;
        else
            rspData.result = RSP_FAILED;
        common_rsp(rspData);
    }

    cJSON_Delete(rootJson);
    linkage_task();
}

static int common_rsp_handle(payload_t data)
{
    cJSON* resultJson = NULL;
    fprintf(stdout,"common_rsp_handle\n");
    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;

    resultJson = cJSON_GetObjectItem(data.pdu, "result");
    fprintf(stdout,"result:%d\n",resultJson->valueint);
}

/*AP report device data to M1*/
static int AP_report_data_handle(payload_t data)
{

    int i,j, number1, number2, rc;
    char time[30];
    char* errorMsg = NULL;

    cJSON* devDataJson = NULL;
    cJSON* devNameJson = NULL;
    cJSON* devIdJson = NULL;
    cJSON* paramJson = NULL;
    cJSON* paramDataJson = NULL;
    cJSON* typeJson = NULL;
    cJSON* valueJson = NULL;

    fprintf(stdout,"AP_report_data_handle\n");
    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;

    getNowTime(time);
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int id;
    char* sql = "select ID from param_table order by ID desc limit 1";

    rc = sqlite3_open(db_path, &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }
    /*添加update/insert/delete监察*/
    rc = sqlite3_update_hook(db, trigger_cb, "AP_report_data_handle");
    if(rc){
        fprintf(stderr, "sqlite3_update_hook falied: %s\n", sqlite3_errmsg(db));  
    }
    if(sqlite3_exec(db, "BEGIN", NULL, NULL, &errorMsg)==SQLITE_OK){
        fprintf(stdout,"BEGIN\n");
        sqlite3_free(errorMsg);

        id = sql_id(db, sql);
        fprintf(stdout,"id:%d\n",id);
        sql = "insert into param_table(ID, DEV_NAME,DEV_ID,TYPE,VALUE,TIME) values(?,?,?,?,?,?);";
        fprintf(stdout,"sql:%s\n",sql);
        sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);

        number1 = cJSON_GetArraySize(data.pdu);
        fprintf(stdout,"number1:%d\n",number1);
        for(i = 0; i < number1; i++){
            devDataJson = cJSON_GetArrayItem(data.pdu, i);
            devNameJson = cJSON_GetObjectItem(devDataJson, "devName");
            fprintf(stdout,"devName:%s\n",devNameJson->valuestring);
            devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
            fprintf(stdout,"devId:%s\n",devIdJson->valuestring);
            paramJson = cJSON_GetObjectItem(devDataJson, "param");
            number2 = cJSON_GetArraySize(paramJson);
            fprintf(stdout," number2:%d\n",number2);

            for(j = 0; j < number2; j++){
                paramDataJson = cJSON_GetArrayItem(paramJson, j);
                typeJson = cJSON_GetObjectItem(paramDataJson, "type");
                fprintf(stdout,"  type:%d\n",typeJson->valueint);
                valueJson = cJSON_GetObjectItem(paramDataJson, "value");
                fprintf(stdout,"  value:%d\n",valueJson->valueint);

                sqlite3_reset(stmt); 
                sqlite3_bind_int(stmt, 1, id);
                id++;
                sqlite3_bind_text(stmt, 2,  devNameJson->valuestring, -1, NULL);
                sqlite3_bind_text(stmt, 3, devIdJson->valuestring, -1, NULL);
                sqlite3_bind_int(stmt, 4,typeJson->valueint);
                sqlite3_bind_int(stmt, 5, valueJson->valueint);
                sqlite3_bind_text(stmt, 6,  time, -1, NULL);
            
                thread_sqlite3_step(&stmt, db);
            }
        }
        if(sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg) == SQLITE_OK){
            fprintf(stdout,"END\n");
            sqlite3_free(errorMsg);
        }
 
     }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    trigger_cb_handle();

    return M1_PROTOCOL_OK;
}

/*AP report device information to M1*/
static int AP_report_dev_handle(payload_t data)
{
    int i, number, rc;
    char time[30];
    char* errorMsg = NULL;
    cJSON* apIdJson = NULL;
    cJSON* apNameJson = NULL;
    cJSON* pIdJson = NULL;
    cJSON* devJson = NULL;
    cJSON* paramDataJson = NULL;    
    cJSON* idJson = NULL;    
    cJSON* nameJson = NULL;   

    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* stmt_1 = NULL;

    fprintf(stdout,"AP_report_dev_handle\n");
    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;

    getNowTime(time);
    /*sqlite3*/
    char* sql = NULL;
    char sql_1[200];
    int id;

    rc = sqlite3_open(db_path, &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    /*添加update/insert/delete监察*/
    rc = sqlite3_update_hook(db, trigger_cb, "AP_report_dev_handle");
    if(rc){
        fprintf(stderr, "sqlite3_update_hook falied: %s\n", sqlite3_errmsg(db));  
    }

    apIdJson = cJSON_GetObjectItem(data.pdu,"apId");
    fprintf(stdout,"APId:%s\n",apIdJson->valuestring);
    apNameJson = cJSON_GetObjectItem(data.pdu,"apName");
    fprintf(stdout,"APName:%s\n",apNameJson->valuestring);
    devJson = cJSON_GetObjectItem(data.pdu,"dev");
    number = cJSON_GetArraySize(devJson); 

    /*事物开始*/
    if(sqlite3_exec(db, "BEGIN", NULL, NULL, &errorMsg)==SQLITE_OK){
        fprintf(stdout,"BEGIN\n");
        sql = "insert or replace into all_dev(ID, DEV_NAME, DEV_ID, AP_ID, PID, ADDED, NET, STATUS, TIME) values(?,?,?,?,?,?,?,?,?);";
        sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);

        for(i = 0; i< number; i++){
            paramDataJson = cJSON_GetArrayItem(devJson, i);
            idJson = cJSON_GetObjectItem(paramDataJson, "devId");
            fprintf(stdout,"devId:%s\n", idJson->valuestring);
            nameJson = cJSON_GetObjectItem(paramDataJson, "devName");
            fprintf(stdout,"devName:%s\n", nameJson->valuestring);
            pIdJson = cJSON_GetObjectItem(paramDataJson, "pId");
            fprintf(stdout,"pId:%05d\n", pIdJson->valueint);
            /*判断该设备是否存在*/
            sprintf(sql_1,"select ID from all_dev where DEV_ID = \"%s\";",idJson->valuestring);
            /*get id*/
            sqlite3_reset(stmt_1); 
            sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
            rc = thread_sqlite3_step(&stmt_1, db);
            if(rc == SQLITE_ROW){
                    id = (sqlite3_column_int(stmt_1, 0));
            }else{
                sprintf(sql_1,"select ID from all_dev order by ID desc limit 1");
                id = sql_id(db, sql_1);
            }

            sqlite3_reset(stmt); 
            sqlite3_bind_int(stmt, 1, id);
            sqlite3_bind_text(stmt, 2,  nameJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt, 3, idJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt, 4,apIdJson->valuestring, -1, NULL);
            sqlite3_bind_int(stmt, 5, pIdJson->valueint);
            sqlite3_bind_int(stmt, 6, 0);
            sqlite3_bind_int(stmt, 7, 1);
            sqlite3_bind_text(stmt, 8,"ON", -1, NULL);
            sqlite3_bind_text(stmt, 9,  time, -1, NULL);
            rc = thread_sqlite3_step(&stmt,db);
        }

        if(sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg) == SQLITE_OK){
            fprintf(stdout,"END\n");
        }
    }else{
        fprintf(stdout,"errorMsg:");
    }    
    sqlite3_free(errorMsg);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
    sqlite3_close(db);  

    return M1_PROTOCOL_OK;  
}

/*AP report information to M1*/
static int AP_report_ap_handle(payload_t data)
{
    int rc;
    char time[30];
    char* errorMsg = NULL;
    cJSON* pIdJson = NULL;
    cJSON* apIdJson = NULL;
    cJSON* apNameJson = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* stmt_1 = NULL;

    fprintf(stdout,"AP_report_ap_handle\n");
    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;

    getNowTime(time);
    /*sqlite3*/
    char sql_1[200];
    char* sql = NULL; 
    int id;
    rc = sqlite3_open(db_path, &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    /*添加update/insert/delete监察*/
    rc = sqlite3_update_hook(db, trigger_cb, "AP_report_ap_handle");
    if(rc){
        fprintf(stderr, "sqlite3_update_hook falied: %s\n", sqlite3_errmsg(db));  
    }

    pIdJson = cJSON_GetObjectItem(data.pdu,"pId");
    fprintf(stdout,"pId:%05d\n",pIdJson->valueint);
    apIdJson = cJSON_GetObjectItem(data.pdu,"apId");
    fprintf(stdout,"APId:%s\n",apIdJson->valuestring);
    apNameJson = cJSON_GetObjectItem(data.pdu,"apName");
    fprintf(stdout,"APName:%s\n",apNameJson->valuestring);
    if(sqlite3_exec(db, "BEGIN", NULL, NULL, &errorMsg)==SQLITE_OK){
        fprintf(stdout,"BEGIN\n");
        /*update clientFd*/
        sprintf(sql_1, "select ID from conn_info where AP_ID  = \"%s\"", apIdJson->valuestring);
        rc = sql_row_number(db, sql_1);
        fprintf(stdout,"rc:%d\n",rc);
        if(rc > 0){
            sprintf(sql_1, "update conn_info set CLIENT_FD = %d where AP_ID  = \"%s\"", data.clientFd, apIdJson->valuestring);
            fprintf(stdout,"%s\n",sql_1);
        }else{
            sql = "select ID from conn_info order by ID desc limit 1";
            id = sql_id(db, sql);
            sprintf(sql_1, " insert into conn_info(ID, AP_ID, CLIENT_FD) values(%d,\"%s\",%d);", id, apIdJson->valuestring, data.clientFd);
            fprintf(stdout,"%s\n",sql_1);
        }
        rc = sql_exec(db, sql_1);
        fprintf(stdout,"exec:%s\n",rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");
        if(rc == SQLITE_ERROR) return M1_PROTOCOL_FAILED;

        /*insert sql*/
        sql = "insert into all_dev(ID, DEV_NAME, DEV_ID, AP_ID, PID, ADDED, NET, STATUS,TIME) values(?,?,?,?,?,?,?,?,?);";
        sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        /*判断该设备是否存在*/
        sprintf(sql_1,"delete from all_dev where DEV_ID = \"%s\";",apIdJson->valuestring);
        /*get id*/
        sqlite3_reset(stmt_1); 
        sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
        while(thread_sqlite3_step(&stmt_1, db) == SQLITE_ROW);
        
        sprintf(sql_1,"select ID from all_dev order by ID desc limit 1");
        id = sql_id(db, sql_1);
        

        sqlite3_bind_int(stmt, 1, id);
        sqlite3_bind_text(stmt, 2,  apNameJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 3, apIdJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 4,apIdJson->valuestring, -1, NULL);
        sqlite3_bind_int(stmt, 5, pIdJson->valueint);
        sqlite3_bind_int(stmt, 6, 0);
        sqlite3_bind_int(stmt, 7, 1);
        sqlite3_bind_text(stmt, 8,  "ON", -1, NULL);
        sqlite3_bind_text(stmt, 9,  time, -1, NULL);
        rc = thread_sqlite3_step(&stmt,db);
        if(sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg) == SQLITE_OK){
           fprintf(stdout,"END\n");
        }
    }else{
        fprintf(stdout,"errorMsg:");
    }    
    
    sqlite3_free(errorMsg);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
    sqlite3_close(db);  

    return M1_PROTOCOL_OK;  
}

static int APP_read_handle(payload_t data, int sn)
{   
    /*read json*/
    cJSON* devDataJson = NULL;
    cJSON* devIdJson = NULL;
    cJSON* paramTypeJson = NULL;
    cJSON* paramJson = NULL;
    /*write json*/
    cJSON * pJsonRoot = NULL; 
    cJSON * pduJsonObject = NULL;
    cJSON * devDataJsonArray = NULL;
    cJSON * devDataObject= NULL;
    cJSON * devArray = NULL;
    cJSON*  devObject = NULL;
    int pduType = TYPE_REPORT_DATA;

    fprintf(stdout,"APP_read_handle\n");
    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;

    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL, *stmt_1 = NULL;
    char sql[200];
    int rc;

    //rc = sqlite3_open(db_path, &db);  
    rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX, NULL);
    if( rc ){  
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

    int i,j, number1,number2,row_n;
    char* dev_id = NULL;

    number1 = cJSON_GetArraySize(data.pdu);
    fprintf(stdout,"number1:%d\n",number1);

    for(i = 0; i < number1; i++){
        /*read json*/
        devDataJson = cJSON_GetArrayItem(data.pdu, i);
        devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
        fprintf(stdout,"devId:%s\n",devIdJson->valuestring);
        dev_id = devIdJson->valuestring;
        paramTypeJson = cJSON_GetObjectItem(devDataJson, "paramType");
        number2 = cJSON_GetArraySize(paramTypeJson);
        /*get sql data json*/
        sprintf(sql, "select DEV_NAME from param_table where DEV_ID  = \"%s\" order by ID desc limit 1;", dev_id);
        fprintf(stdout,"%s\n", sql);
        row_n = sql_row_number(db, sql);
        fprintf(stdout,"row_n:%d\n",row_n);
        if(row_n > 0){
            devDataObject = cJSON_CreateObject();
            if(NULL == devDataObject)
            {
                // create object faild, exit
                fprintf(stdout,"devDataObject NULL\n");
                cJSON_Delete(devDataObject);
                return M1_PROTOCOL_FAILED;
            }
            cJSON_AddItemToArray(devDataJsonArray, devDataObject);
            cJSON_AddStringToObject(devDataObject, "devId", dev_id);
            /*取出devName*/
            sqlite3_reset(stmt);
            sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
            rc = thread_sqlite3_step(&stmt, db);
            if(rc == SQLITE_ROW){
                cJSON_AddStringToObject(devDataObject, "devName", (const char*)sqlite3_column_text(stmt,0));
            }
            /*添加PID*/
            sprintf(sql, "select PID from all_dev where DEV_ID  = \"%s\" order by ID desc limit 1;", dev_id);
            fprintf(stdout,"%s\n", sql);
            sqlite3_reset(stmt);
            sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
            rc = thread_sqlite3_step(&stmt, db);
            if(rc == SQLITE_ROW){
                cJSON_AddStringToObject(devDataObject, "pId", (const char*)sqlite3_column_text(stmt,0));
            }

            devArray = cJSON_CreateArray();
            if(NULL == devArray)
            {
                fprintf(stdout,"devArry NULL\n");
                cJSON_Delete(devArray);
                return M1_PROTOCOL_FAILED;
            }
            /*add devData array to pdu pbject*/
            cJSON_AddItemToObject(devDataObject, "param", devArray);
        }

        for(j = 0; j < number2; j++){
            /*read json*/
            paramJson = cJSON_GetArrayItem(paramTypeJson, j);
            /*get sql data json*/
            sprintf(sql, "select VALUE from param_table where DEV_ID  = \"%s\" and TYPE = %05d order by ID desc limit 1;", dev_id, paramJson->valueint);
            fprintf(stdout,"%s\n", sql);
            row_n = sql_row_number(db, sql);
            fprintf(stdout,"row_n:%d\n",row_n);
            if(row_n > 0){
                devObject = cJSON_CreateObject();
                if(NULL == devObject)
                {
                    fprintf(stdout,"devObject NULL\n");
                    cJSON_Delete(devObject);
                    return M1_PROTOCOL_FAILED;
                }
                cJSON_AddItemToArray(devArray, devObject); 
                cJSON_AddNumberToObject(devObject, "type", paramJson->valueint);

                sqlite3_reset(stmt_1);
                sqlite3_prepare_v2(db, sql, strlen(sql),&stmt_1, NULL);
                rc = thread_sqlite3_step(&stmt_1,db);
                if(rc == SQLITE_ROW){
                    cJSON_AddNumberToObject(devObject, "value", sqlite3_column_int(stmt_1,0));
                }
            }

        }
    }

    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
    sqlite3_close(db);

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    fprintf(stdout,"string:%s\n",p);
    socketSeverSend((uint8*)p, strlen(p), data.clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;
}

static int M1_write_to_AP(cJSON* data)
{
    fprintf(stdout,"M1_write_to_AP\n");
    int sn = 2;
    cJSON* snJson = NULL;
    cJSON* pduJson = NULL;
    cJSON* devDataJson = NULL;
    cJSON* dataArrayJson = NULL;
    cJSON* devIdJson = NULL;
    /*更改sn*/
    snJson = cJSON_GetObjectItem(data, "sn");
    cJSON_SetIntValue(snJson, sn);
    /*获取clientFd*/
    pduJson = cJSON_GetObjectItem(data, "pdu");
    devDataJson = cJSON_GetObjectItem(pduJson, "devData");
    dataArrayJson = cJSON_GetArrayItem(devDataJson, 0);
    devIdJson = cJSON_GetObjectItem(dataArrayJson, "devId");
    fprintf(stdout,"devId:%s\n",devIdJson->valuestring);

    int clientFd;
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int rc,row_n;
    const char* ap_id = NULL;

    rc = sqlite3_open(db_path, &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    } 

    char sql[200];
    /*get apId*/
    sprintf(sql,"select AP_ID from all_dev where DEV_ID = \"%s\" limit 1;",devIdJson->valuestring);
    row_n = sql_row_number(db, sql);
    fprintf(stdout,"row_n:%d\n",row_n);
    if(row_n > 0){ 
        sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
        rc = thread_sqlite3_step(&stmt,db);
        if(rc == SQLITE_ROW){
            ap_id = sqlite3_column_text(stmt,0);
            fprintf(stdout,"ap_id%s\n",ap_id);
        }
    }

    /*get clientFd*/
    sprintf(sql,"select CLIENT_FD from conn_info where AP_ID = \"%s\" limit 1;",ap_id);
    row_n = sql_row_number(db, sql);
    fprintf(stdout,"row_n:%d\n",row_n);
    if(row_n > 0){
        sqlite3_reset(stmt); 
        sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
        rc = thread_sqlite3_step(&stmt, db);
        if(rc == SQLITE_ROW){
            clientFd = sqlite3_column_int(stmt,0);
        }
    }

    sqlite3_finalize(stmt);

    char * p = cJSON_PrintUnformatted(data);
    
    if(NULL == p)
    {    
        cJSON_Delete(data);
        return M1_PROTOCOL_FAILED;
    }

    fprintf(stdout,"string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), clientFd);

    sqlite3_close(db);

    return M1_PROTOCOL_OK;
}

static int APP_write_handle(payload_t data)
{
    int i,j, number1,number2;
    char time[30];
    char* errorMsg = NULL;
    cJSON* devDataJson = NULL;
    cJSON* devIdJson = NULL;
    cJSON* paramDataJson = NULL;
    cJSON* paramArrayJson = NULL;
    cJSON* valueTypeJson = NULL;
    cJSON* valueJson = NULL;

    fprintf(stdout,"APP_write_handle\n");
    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;

    getNowTime(time);
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL,*stmt_1 = NULL;
    int rc,row_n,id;
    const char* dev_name = NULL;
    char* sql = "select ID from param_table order by ID desc limit 1";

    //rc = sqlite3_open(db_path, &db);  
    rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, NULL);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }
    /*添加update/insert/delete监察*/
    rc = sqlite3_update_hook(db, trigger_cb, "APP_write_handle");
    if(rc){
        fprintf(stderr, "sqlite3_update_hook falied: %s\n", sqlite3_errmsg(db));  
    }

    id = sql_id(db, sql);
    fprintf(stdout,"id:%d\n",id);
    if(sqlite3_exec(db, "BEGIN", NULL, NULL, &errorMsg)==SQLITE_OK){
        fprintf(stdout,"BEGIN\n");
        /*cJSON*/
        /*insert data*/
        char sql_1[200] ;
        char* sql_2 = "insert into param_table(ID, DEV_NAME,DEV_ID,TYPE,VALUE,TIME) values(?,?,?,?,?,?);";
        number1 = cJSON_GetArraySize(data.pdu);
        fprintf(stdout,"number1:%d\n",number1);
        for(i = 0; i < number1; i++){
            devDataJson = cJSON_GetArrayItem(data.pdu, i);
            devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
            fprintf(stdout,"devId:%s\n",devIdJson->valuestring);
            paramDataJson = cJSON_GetObjectItem(devDataJson, "param");
            number2 = cJSON_GetArraySize(paramDataJson);
            fprintf(stdout,"number2:%d\n",number2);
    
                sprintf(sql_1,"select DEV_NAME from all_dev where DEV_ID = \"%s\" limit 1;", devIdJson->valuestring);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                row_n = sql_row_number(db, sql);
                fprintf(stdout,"row_n:%d\n",row_n);
                if(row_n > 0){        
                    sqlite3_reset(stmt);
                    sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
                    rc = thread_sqlite3_step(&stmt, db);
                    if(rc == SQLITE_ROW){
                        dev_name = (const char*)sqlite3_column_text(stmt, 0);
                        fprintf(stdout,"dev_name:%s\n",dev_name);
                    }
                    for(j = 0; j < number2; j++){
                        paramArrayJson = cJSON_GetArrayItem(paramDataJson, j);
                        valueTypeJson = cJSON_GetObjectItem(paramArrayJson, "type");
                        fprintf(stdout,"  type%d:%d\n",j,valueTypeJson->valueint);
                        valueJson = cJSON_GetObjectItem(paramArrayJson, "value");
                        fprintf(stdout,"  value%d:%d\n",j,valueJson->valueint);
    
                        sqlite3_reset(stmt_1); 
                        sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_1, NULL);
                        sqlite3_bind_int(stmt_1, 1, id);
                        id++;
                        sqlite3_bind_text(stmt_1, 2,  dev_name, -1, NULL);
                        sqlite3_bind_text(stmt_1, 3, devIdJson->valuestring, -1, NULL);
                        sqlite3_bind_int(stmt_1, 4, valueTypeJson->valueint);
                        sqlite3_bind_int(stmt_1, 5, valueJson->valueint);
                        sqlite3_bind_text(stmt_1, 6,  time, -1, NULL);
                        rc = thread_sqlite3_step(&stmt_1, db);
                    }
                }
        }
        if(sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg) == SQLITE_OK){
            fprintf(stdout,"END\n");
        }
    }else{
        fprintf(stdout,"errorMsg:");
    }    
    sqlite3_free(errorMsg);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1); 
    sqlite3_close(db);

    return M1_PROTOCOL_OK;
}

static int APP_echo_dev_info_handle(payload_t data)
{
    int i,j,number,number_1,rc;
    cJSON* devDataJson = NULL;
    cJSON* devdataArrayJson = NULL;
    cJSON* devArrayJson = NULL;
    cJSON* APIdJson = NULL;

    fprintf(stdout,"APP_echo_dev_info_handle\n");
    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;
    /*sqlite3*/
    sqlite3* db = NULL;
    char* err_msg = NULL;
    char sql_1[200];
    char* errorMsg = NULL;

    sqlite3_open(db_path,&db);

    number = cJSON_GetArraySize(data.pdu);
    fprintf(stdout,"number:%d\n",number);
    if(sqlite3_exec(db, "BEGIN", NULL, NULL, &errorMsg)==SQLITE_OK){
        fprintf(stdout,"BEGIN\n");    
        for(i = 0; i < number; i++){
            devdataArrayJson = cJSON_GetArrayItem(data.pdu, i);
            APIdJson = cJSON_GetObjectItem(devdataArrayJson, "apId");
            devDataJson = cJSON_GetObjectItem(devdataArrayJson,"devId");
            fprintf(stdout,"AP_ID:%s\n",APIdJson->valuestring);

            sprintf(sql_1, "update all_dev set ADDED = 1 where DEV_ID = \"%s\" and AP_ID = \"%s\";",APIdJson->valuestring,APIdJson->valuestring);
            fprintf(stdout,"sql_1:%s\n",sql_1);
            rc = sqlite3_exec(db, sql_1, NULL, 0, &err_msg);
            if(rc != SQLITE_OK){
                fprintf(stdout,"SQL error:%s\n",err_msg);
                sqlite3_free(err_msg);
                return M1_PROTOCOL_FAILED;
            }

            if(devDataJson != NULL){
                number_1 = cJSON_GetArraySize(devDataJson);
                fprintf(stdout,"number_1:%d\n",number_1);
                for(j = 0; j < number_1; j++){
                    devArrayJson = cJSON_GetArrayItem(devDataJson, j);
                    fprintf(stdout,"  devId:%s\n",devArrayJson->valuestring);

                    sprintf(sql_1, "update all_dev set ADDED = 1 where DEV_ID = \"%s\" and AP_ID = \"%s\";",devArrayJson->valuestring,APIdJson->valuestring);
                    fprintf(stdout,"sql_1:%s\n",sql_1);
                    rc = sqlite3_exec(db, sql_1, NULL, 0, &err_msg);
                    if(rc != SQLITE_OK){
                        fprintf(stdout,"SQL error:%s\n",err_msg);
                        sqlite3_free(err_msg);
                        return M1_PROTOCOL_FAILED;
                    }
                }
            }
        }
        if(sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg) == SQLITE_OK){
            fprintf(stdout,"END\n");
        }
    }else{
        fprintf(stdout,"errorMsg:");
    }    
    sqlite3_close(db);

    return M1_PROTOCOL_OK;
}

static int APP_req_added_dev_info_handle(int clientFd, int sn)
{
    /*cJSON*/
    int pduType = TYPE_M1_REPORT_ADDED_INFO;
    cJSON * pJsonRoot = NULL;
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt_1 = NULL,*stmt_2 = NULL;
    char* sql_1 = NULL;
    char sql_2[200];

    fprintf(stdout,"APP_req_added_dev_info_handle\n");
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
    cJSON * pduJsonObject = NULL;
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
    cJSON * devDataJsonArray = NULL;
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
    rc = sqlite3_open(db_path, &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    } 

    int row_n;
    cJSON*  devDataObject= NULL;
    cJSON * devArray = NULL;
    cJSON*  devObject = NULL;

    sql_1 = "select * from all_dev where DEV_ID  = AP_ID and ADDED = 1;";
    row_n = sql_row_number(db, sql_1);
    fprintf(stdout,"row_n:%d\n",row_n);
    if(row_n > 0){ 
        sqlite3_prepare_v2(db, sql_1, strlen(sql_1),&stmt_1, NULL);
        while(thread_sqlite3_step(&stmt_1, db) == SQLITE_ROW){
            /*add ap infomation: port,ap_id,ap_name,time */
            devDataObject = cJSON_CreateObject();

            if(NULL == devDataObject)
            {
                // create object faild, exit
                fprintf(stdout,"devDataObject NULL\n");
                cJSON_Delete(devDataObject);
                return M1_PROTOCOL_FAILED;
            }
            cJSON_AddItemToArray(devDataJsonArray, devDataObject);

            cJSON_AddNumberToObject(devDataObject, "pId", sqlite3_column_int(stmt_1, 4));
            cJSON_AddStringToObject(devDataObject, "apId",  (const char*)sqlite3_column_text(stmt_1, 3));
            cJSON_AddStringToObject(devDataObject, "apName", (const char*)sqlite3_column_text(stmt_1, 2));
            
            /*create devData array*/
            devArray = cJSON_CreateArray();
            if(NULL == devArray)
            {
                 fprintf(stdout,"devArry NULL\n");
                 cJSON_Delete(devArray);
                 return M1_PROTOCOL_FAILED;
            }
            /*add devData array to pdu pbject*/
            cJSON_AddItemToObject(devDataObject, "dev", devArray);
            /*sqlite3*/
            sprintf(sql_2,"select * from all_dev where AP_ID  = \"%s\" and AP_ID != DEV_ID and ADDED = 1;",sqlite3_column_text(stmt_1, 3));
            fprintf(stdout,"sql_2:%s\n",sql_2);
            row_n = sql_row_number(db, sql_1);
            fprintf(stdout,"row_n:%d\n",row_n);
            if(row_n > 0){ 
                sqlite3_prepare_v2(db, sql_2, strlen(sql_2),&stmt_2, NULL);
                while(thread_sqlite3_step(&stmt_2, db) == SQLITE_ROW){
                     /*add ap infomation: port,ap_id,ap_name,time */
                    devObject = cJSON_CreateObject();
                    if(NULL == devObject)
                    {
                        fprintf(stdout,"devObject NULL\n");
                        cJSON_Delete(devObject);
                        return M1_PROTOCOL_FAILED;
                    }
                    cJSON_AddItemToArray(devArray, devObject); 
                    cJSON_AddNumberToObject(devObject, "pId", sqlite3_column_int(stmt_2, 4));
                    cJSON_AddStringToObject(devObject, "devId", (const char*)sqlite3_column_text(stmt_2, 1));
                    cJSON_AddStringToObject(devObject, "devName", (const char*)sqlite3_column_text(stmt_2, 2));
                }
            }

        }
    }
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

static int APP_net_control(payload_t data)
{
    fprintf(stdout,"APP_net_control\n");
    int pduType = TYPE_DEV_NET_CONTROL;
    cJSON * pJsonRoot = NULL;

    cJSON* apIdJson = NULL;
    cJSON* valueJson = NULL;

    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;

    apIdJson = cJSON_GetObjectItem(data.pdu, "apId");
    fprintf(stdout,"apId:%s\n",apIdJson->valuestring);
    valueJson = cJSON_GetObjectItem(data.pdu, "value");
    fprintf(stdout,"value:%d\n",valueJson->valueint);

    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    char sql[200]; 
    int rc, clientFd, row_n;   

    rc = sqlite3_open(db_path, &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    sprintf(sql,"select CLIENT_FD from conn_info where AP_ID = \"%s\";",apIdJson->valuestring);
    row_n = sql_row_number(db, sql);
    fprintf(stdout,"row_n:%d\n",row_n);
    if(row_n > 0){ 
        sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
        rc = thread_sqlite3_step(&stmt, db);
        fprintf(stdout,"step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");
        if(rc == SQLITE_ROW){
            clientFd = sqlite3_column_int(stmt,0);
        }
        /*create json*/
        pJsonRoot = cJSON_CreateObject();
        if(NULL == pJsonRoot)
        {
            fprintf(stdout,"pJsonRoot NULL\n");
            cJSON_Delete(pJsonRoot);
            return M1_PROTOCOL_FAILED;
        }

        cJSON_AddNumberToObject(pJsonRoot, "sn", 1);
        cJSON_AddStringToObject(pJsonRoot, "version", "1.0");
        cJSON_AddNumberToObject(pJsonRoot, "netFlag", 1);
        cJSON_AddNumberToObject(pJsonRoot, "cmdType", 2);
        /*create pdu object*/
        cJSON * pduJsonObject = NULL;
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
        /*add dev data to pdu object*/
        cJSON_AddNumberToObject(pduJsonObject, "devData", valueJson->valueint);

    }else{
        return M1_PROTOCOL_FAILED;        
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
    socketSeverSend((uint8*)p, strlen(p), clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;
}

static int M1_report_ap_info(int clientFd, int sn)
{
    fprintf(stdout," M1_report_ap_info\n");
    /*cJSON*/
    int pduType = TYPE_M1_REPORT_AP_INFO;
    cJSON * pJsonRoot = NULL;
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    char* sql = NULL;

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
    cJSON * pduJsonObject = NULL;
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
    cJSON * devDataJsonArray = NULL;
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

    rc = sqlite3_open(db_path, &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    } 

    int row_n;
    cJSON*  devDataObject= NULL;

    sql = "select * from all_dev where DEV_ID  = AP_ID";
    row_n = sql_row_number(db, sql);
    fprintf(stdout,"row_n:%d\n",row_n);
    if(row_n > 0){ 
        sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
        while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
            /*add ap infomation: port,ap_id,ap_name,time */
            devDataObject = cJSON_CreateObject();

            if(NULL == devDataObject)
            {
                // create object faild, exit
                fprintf(stdout,"devDataObject NULL\n");
                cJSON_Delete(devDataObject);
                return M1_PROTOCOL_FAILED;
            }
            cJSON_AddItemToArray(devDataJsonArray, devDataObject);

            cJSON_AddNumberToObject(devDataObject, "pId", sqlite3_column_int(stmt, 4));
            cJSON_AddStringToObject(devDataObject, "apId", (const char*)sqlite3_column_text(stmt, 3));
            cJSON_AddStringToObject(devDataObject, "apName", (const char*)sqlite3_column_text(stmt, 2));
            
            
        }
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
    socketSeverSend((uint8*)p, strlen(p), clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;
}

static int M1_report_dev_info(payload_t data, int sn)
{
     fprintf(stdout," M1_report_dev_info\n");
    /*cJSON*/
    int pduType = TYPE_M1_REPORT_DEV_INFO;
    char* ap = NULL;

    ap = data.pdu->valuestring;


    cJSON * pJsonRoot = NULL;
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    char sql[200];

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
    cJSON * pduJsonObject = NULL;
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
    cJSON * devDataJsonArray = NULL;
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
    rc = sqlite3_open(db_path, &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    } 

    int row_n;
    cJSON*  devDataObject= NULL;

    sprintf(sql,"select * from all_dev where AP_ID != DEV_ID and AP_ID = \"%s\";", ap);
    fprintf(stdout,"string:%s\n",sql);
    row_n = sql_row_number(db, sql);
    fprintf(stdout,"row_n:%d\n",row_n);
    if(row_n > 0){ 
        sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
        while(thread_sqlite3_step(&stmt,db) == SQLITE_ROW){
            /*add ap infomation: port,ap_id,ap_name,time */
            devDataObject = cJSON_CreateObject();

            if(NULL == devDataObject)
            {
                // create object faild, exit
                fprintf(stdout,"devDataObject NULL\n");
                cJSON_Delete(devDataObject);
                return M1_PROTOCOL_FAILED;
            }
            cJSON_AddItemToArray(devDataJsonArray, devDataObject);

            cJSON_AddNumberToObject(devDataObject, "pId", sqlite3_column_int(stmt, 4));
            cJSON_AddStringToObject(devDataObject, "devId", (const char*)sqlite3_column_text(stmt, 1));
            cJSON_AddStringToObject(devDataObject, "devName", (const char*)sqlite3_column_text(stmt, 2));
            
        }
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
    socketSeverSend((uint8*)p, strlen(p), data.clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;
}

/*子设备/AP/联动/场景/区域/-启动/停止/删除*/
static int common_operate(payload_t data)
{
    fprintf(stdout,"common_operate\n");
    char* errorMsg = NULL;
    cJSON* typeJson = NULL;
    cJSON* idJson = NULL;
    cJSON* operateJson = NULL;

    typeJson = cJSON_GetObjectItem(data.pdu, "type");   
    fprintf(stdout,"type:%s\n",typeJson->valuestring);
    idJson = cJSON_GetObjectItem(data.pdu, "id");   
    fprintf(stdout,"id:%s\n",idJson->valuestring);
    operateJson = cJSON_GetObjectItem(data.pdu, "operate");   
    fprintf(stdout,"operate:%s\n",operateJson->valuestring);
    /*sqlite3 相关操作*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int id, rc, number1,i;
    int row_number = 0;
    char*scen_name = NULL;
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

    if(sqlite3_exec(db, "BEGIN", NULL, NULL, &errorMsg)==SQLITE_OK){
        fprintf(stdout,"BEGIN\n");
        if(strcmp(typeJson->valuestring, "device") == 0){
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*通知到ap*/
                /*删除all_dev中的子设备*/
                sprintf(sql_1,"delete from all_dev where DEV_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                sql_exec(db, sql_1);
            }else if(strcmp(operateJson->valuestring, "on") == 0){
                sprintf(sql_1,"update all_dev set STATUS = \"ON\" where DEV_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                sql_exec(db, sql_1);
            }else if(strcmp(operateJson->valuestring, "off") == 0){
                sprintf(sql_1,"update all_dev set STATUS = \"OFF\" where DEV_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                sql_exec(db, sql_1);
            }

        }else if(strcmp(typeJson->valuestring, "linkage") == 0){
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*删除联动表linkage_table中相关内容*/
                sprintf(sql_1,"delete from linkage_table where LINK_NAME = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                sql_exec(db, sql_1);
                /*删除联动触发表link_trigger_table相关内容*/
                sprintf(sql_1,"delete from link_trigger_table where LINK_NAME = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                sql_exec(db, sql_1);
                /*删除联动触发表link_exec_table相关内容*/
                sprintf(sql_1,"delete from link_exec_table where LINK_NAME = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                sql_exec(db, sql_1);
            }
        }else if(strcmp(typeJson->valuestring, "scenario") == 0){
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*删除场景表相关内容*/
                sprintf(sql_1,"delete from scenario_table where SCEN_NAME = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                sql_exec(db, sql_1);
                /*删除场景定时相关内容*/
                sprintf(sql_1,"delete from scen_alarm_table where SCEN_NAME = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                sql_exec(db, sql_1);
            }
        }else if(strcmp(typeJson->valuestring, "district") == 0){
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*删除区域相关内容*/
                sprintf(sql_1,"delete from district_table where DIS_NAME = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                sql_exec(db, sql_1);
                /*删除区域下的联动表中相关内容*/
                sprintf(sql_1,"delete from linkage_table where DISTRICT = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                sql_exec(db, sql_1);    
                /*删除区域下的联动触发表中相关内容*/
                sprintf(sql_1,"delete from link_trigger_table where DISTRICT = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                sql_exec(db, sql_1);    
                /*删除区域下的联动触发表中相关内容*/
                sprintf(sql_1,"delete from link_exec_table where DISTRICT = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                sql_exec(db, sql_1); 
                /*删除场景定时相关内容*/
                sprintf(sql_1,"select SCEN_NAME from scenario_table where DISTRICT = \"%s\" limit 1;",idJson->valuestring);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                sqlite3_reset(stmt);
                sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
                rc = thread_sqlite3_step(&stmt, db);
                fprintf(stdout,"step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");
                if(rc == SQLITE_ROW){
                    scen_name = sqlite3_column_text(stmt,0);
                }
                sprintf(sql_1,"delete from scen_alarm_table where SCEN_NAME = \"%s\";",scen_name);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                sql_exec(db, sql_1); 
                /*删除区域下场景表相关内容*/   
                sprintf(sql_1,"delete from scenario_table where DISTRICT = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql_1:%s\n",sql_1);
                sql_exec(db, sql_1); 
            }
        }else if(strcmp(typeJson->valuestring, "ap") == 0){

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

static int ap_heartbeat_handle(payload_t data)
{
    return M1_PROTOCOL_OK;
}

static int common_rsp(rsp_data_t data)
{
    fprintf(stdout," common_rsp\n");
    /*cJSON*/
    int pduType = TYPE_COMMON_RSP;

    cJSON * pJsonRoot = NULL;

    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        fprintf(stdout,"pJsonRoot NULL\n");
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    cJSON_AddNumberToObject(pJsonRoot, "sn", data.sn);
    cJSON_AddStringToObject(pJsonRoot, "version", "1.0");
    cJSON_AddNumberToObject(pJsonRoot, "netFlag", 1);
    cJSON_AddNumberToObject(pJsonRoot, "cmdType", 1);
    /*create pdu object*/
    cJSON * pduJsonObject = NULL;
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
    /*devData*/
    cJSON * devDataObject = NULL;
    devDataObject = cJSON_CreateObject();
    if(NULL == devDataObject)
    {
        // create object faild, exit
        cJSON_Delete(devDataObject);
        return M1_PROTOCOL_FAILED;
    }
    /*add devData to pdu*/
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataObject);
    cJSON_AddNumberToObject(devDataObject, "sn", data.sn);
    cJSON_AddNumberToObject(devDataObject, "pduType", data.pduType);
    cJSON_AddNumberToObject(devDataObject, "result", data.result);

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    fprintf(stdout,"string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), data.clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;
}

void getNowTime(char* _time)
{

    struct tm nowTime;

    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);  //获取相对于1970到现在的秒数
    localtime_r(&time.tv_sec, &nowTime);
    
    sprintf(_time, "%04d%02d%02d%02d%02d%02d", nowTime.tm_year + 1900, nowTime.tm_mon+1, nowTime.tm_mday, 
      nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec);
}

void setLocalTime(char* time)
{
    struct tm local_tm, *time_1;
    struct timeval tv;
    struct timezone tz;
    int mon, day,hour,min;

    gettimeofday(&tv, &tz);
    mon = atoi(time) / 1000000 - 1;
    day = atoi(&time[2]) / 10000;
    hour = atoi(&time[4]) / 100; 
    min = atoi(&time[6]);
    printf("setLocalTime,time:%02d-%02d %02d:%02d\n",mon+1,day,hour,min);
    local_tm.tm_year = 2017 - 1900;
    local_tm.tm_mon = mon;
    local_tm.tm_mday = day;
    local_tm.tm_hour = hour;
    local_tm.tm_min = min;
    local_tm.tm_sec = 30;
    tv.tv_sec = mktime(&local_tm);
    tv.tv_usec = 0;
    settimeofday(&tv, &tz);
}

void delay_send(cJSON* d, int delay, int clientFd)
{
    printf("delay_send\n");
    Item item;

    item.data = d;
    item.prio = delay;
    item.clientFd = clientFd;
    Push(&head, item);
}

void delay_send_task(void)
{
    static uint32_t count = 0;
    Item item;
    char * p = NULL;
    char str[20];
    while(1){
        if(!IsEmpty(&head)){
            if(head.next->item.prio <= 0){
                Pop(&head, &item);
                p = cJSON_PrintUnformatted(item.data);
                printf("delay_send_task data:%s, addr:%x\n",p, item.data);
                socketSeverSend((uint8*)p, strlen(p), item.clientFd);
                cJSON_Delete(item.data);
            }
        }
        usleep(100000);
        count++;
        if(count >= 10){
            count = 0;
            Queue_delay_decrease(&head);
        }

    }
}

int sql_id(sqlite3* db, char* sql)
{
    fprintf(stdout,"sql_id\n");
    sqlite3_stmt* stmt = NULL;
    int id, total_column, rc;
    /*get id*/
    sqlite3_prepare_v2(db, sql, strlen(sql), & stmt, NULL);
    rc = thread_sqlite3_step(&stmt, db);
    if(rc == SQLITE_ROW){
            id = (sqlite3_column_int(stmt, 0) + 1);
    }else{
        id = 1;
    }

    sqlite3_finalize(stmt);
    return id;
}

int sql_row_number(sqlite3* db, char*sql)
{
    char** p_result;
    char* errmsg;
    int n_row, n_col, rc;
    /*sqlite3 lock*/
    sqlite3_mutex_enter(sqlite3_db_mutex(db));
    rc = sqlite3_get_table(db, sql, &p_result,&n_row, &n_col, &errmsg);
    /*sqlite3 unlock*/
    sqlite3_mutex_leave(sqlite3_db_mutex(db));
    fprintf(stdout,"n_row:%d\n",n_row);

    sqlite3_free(errmsg);
    sqlite3_free_table(p_result);

    return n_row;
}

int sql_exec(sqlite3* db, char*sql)
{
    sqlite3_stmt* stmt = NULL;
    int rc;
    /*get id*/
    sqlite3_prepare_v2(db, sql, strlen(sql), & stmt, NULL);
   
    do{
        rc = thread_sqlite3_step(&stmt, db);
    }while(rc == SQLITE_ROW);
   
    sqlite3_finalize(stmt);
    return rc;
}

int thread_sqlite3_step(sqlite3_stmt** stmt, sqlite3* db)
{
    int rc;
    /*sqlite3 lock*/
    //sqlite3_mutex_enter(sqlite3_db_mutex(db));
    //do{
        rc = sqlite3_step(*stmt);   
    //}while(rc == SQLITE_BUSY);
    /*sqlite3 unlock*/
    //sqlite3_mutex_leave(sqlite3_db_mutex(db));
    fprintf(stdout,"step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");
    return rc;
}