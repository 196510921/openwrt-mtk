#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "sqlite3.h"

#include "m1_protocol.h"
#include "socket_server.h"

#define M1_PROTOCOL_DEBUG    1

#define HEAD_LEN    3

extern pthread_mutex_t mut;

static int AP_report_data_handle(payload_t data);
static int APP_read_handle(payload_t data);
static int APP_write_handle(payload_t data);
static int M1_write_to_AP(cJSON* data);
static int APP_echo_dev_info_handle(payload_t data);
static int APP_req_added_dev_info_handle(int clientFd);
static int APP_net_control(payload_t data);
static int M1_report_dev_info(payload_t data);
static int M1_report_ap_info(int clientFd);
static int AP_report_dev_handle(payload_t data);
static int AP_report_ap_handle(payload_t data);
static int common_rsp(rsp_data_t data);
static int common_rsp_handle(payload_t data);

static void getNowTime(char* time);
static int sql_id(sqlite3* db, char* sql);
static int sql_row_number(sqlite3* db, char*sql);
static int sql_exec(sqlite3* db, char*sql);

char* db_path = "dev_info.db";

void data_handle(m1_package_t package)
{
    int rc = M1_PROTOCOL_NO_RSP;
    payload_t pdu;
    rsp_data_t rspData;
    cJSON* rootJson = NULL;
    cJSON* pduJson = NULL;
    cJSON* pduTypeJson = NULL;
    cJSON* snJson = NULL;
    cJSON* pduDataJson = NULL;
    int pduType;

    rootJson = cJSON_Parse(package.data);
    if(NULL == rootJson){
        printf("rootJson null\n");
        return;
    }
    pduJson = cJSON_GetObjectItem(rootJson, "pdu");
    if(NULL == pduJson){
        printf("pdu null\n");
        return;
    }
    pduTypeJson = cJSON_GetObjectItem(pduJson, "pduType");
    if(NULL == pduTypeJson){
        printf("pduType null\n");
        return;
    }
    pduType = pduTypeJson->valueint;
    rspData.pduType = pduType;

    snJson = cJSON_GetObjectItem(rootJson, "sn");
    if(NULL == snJson){
        printf("sn null\n");
        return;
    }
    rspData.sn = snJson->valueint;

    pduDataJson = cJSON_GetObjectItem(pduJson, "devData");
    if(NULL == pduDataJson){
        printf("devData null”\n");

    }
    /*pdu*/ 
    pdu.clientFd = package.clientFd;
    pdu.pdu = pduDataJson;

    rspData.clientFd = package.clientFd;
    printf("pduType:%x\n",pduType);
    switch(pduType){
                    case TYPE_REPORT_DATA: rc = AP_report_data_handle(pdu); break;
                    case TYPE_DEV_READ: APP_read_handle(pdu); break;
                    case TYPE_DEV_WRITE: rc = APP_write_handle(pdu); if(rc != M1_PROTOCOL_FAILED) M1_write_to_AP(rootJson);break;
                    case TYPE_ECHO_DEV_INFO: rc = APP_echo_dev_info_handle(pdu); break;
                    case TYPE_REQ_ADDED_INFO: APP_req_added_dev_info_handle( package.clientFd); break;
                    case TYPE_DEV_NET_CONTROL: rc = APP_net_control(pdu); break;
                    case TYPE_REQ_AP_INFO: M1_report_ap_info(package.clientFd); break;
                    case TYPE_REQ_DEV_INFO: M1_report_dev_info(pdu); break;
                    case TYPE_AP_REPORT_DEV_INFO: rc = AP_report_dev_handle(pdu); break;
                    case TYPE_AP_REPORT_AP_INFO: rc = AP_report_ap_handle(pdu); break;
                    case TYPE_COMMON_RSP: common_rsp_handle(pdu);break;

        default: printf("pdu type not match\n"); rc = M1_PROTOCOL_FAILED;break;
    }

    if(rc != M1_PROTOCOL_NO_RSP){
        if(rc == M1_PROTOCOL_OK)
            rspData.result = RSP_OK;
        else
            rspData.result = RSP_FAILED;
        common_rsp(rspData);
    }

    cJSON_Delete(rootJson);
}

static int common_rsp_handle(payload_t data)
{
    cJSON* resultJson = NULL;
    printf("common_rsp_handle\n");
    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;

    resultJson = cJSON_GetObjectItem(data.pdu, "result");
    printf("result:%d\n",resultJson->valueint);
}

/*AP report device data to M1*/
static int AP_report_data_handle(payload_t data)
{

    int i,j, number1, number2, rc;
    char time[30];

    cJSON* devDataJson = NULL;
    cJSON* devNameJson = NULL;
    cJSON* devIdJson = NULL;
    cJSON* paramJson = NULL;
    cJSON* paramDataJson = NULL;
    cJSON* typeJson = NULL;
    cJSON* valueJson = NULL;

    printf("AP_report_data_handle\n");
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

    id = sql_id(db, sql);

    sql = "insert into param_table(ID, DEV_NAME,DEV_ID,TYPE,VALUE,TIME) values(?,?,?,?,?,?);";
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);

    number1 = cJSON_GetArraySize(data.pdu);
    printf("number1:%d\n",number1);
    for(i = 0; i < number1; i++){
        devDataJson = cJSON_GetArrayItem(data.pdu, i);
        devNameJson = cJSON_GetObjectItem(devDataJson, "devName");
        printf("devName:%s\n",devNameJson->valuestring);
        devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
        printf("devId:%s\n",devIdJson->valuestring);
        paramJson = cJSON_GetObjectItem(devDataJson, "param");
        number2 = cJSON_GetArraySize(paramJson);
        printf(" number2:%d\n",number2);

        for(j = 0; j < number2; j++){
            paramDataJson = cJSON_GetArrayItem(paramJson, j);
            typeJson = cJSON_GetObjectItem(paramDataJson, "type");
            printf("  type:%d\n",typeJson->valueint);
            valueJson = cJSON_GetObjectItem(paramDataJson, "value");
            printf("  value:%d\n",valueJson->valueint);

            sqlite3_reset(stmt); 
            sqlite3_bind_int(stmt, 1, id);
            id++;
            sqlite3_bind_text(stmt, 2,  devNameJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt, 3, devIdJson->valuestring, -1, NULL);
            sqlite3_bind_int(stmt, 4,typeJson->valueint);
            sqlite3_bind_int(stmt, 5, valueJson->valueint);
            sqlite3_bind_text(stmt, 6,  time, -1, NULL);
            rc = sqlite3_step(stmt);
            printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return M1_PROTOCOL_OK;
}

/*AP report device information to M1*/
static int AP_report_dev_handle(payload_t data)
{
    int i, number, rc;
    char time[30];
    cJSON* portJson = NULL;
    cJSON* apIdJson = NULL;
    cJSON* apNameJson = NULL;
    cJSON* devJson = NULL;
    cJSON* paramDataJson = NULL;    
    cJSON* idJson = NULL;    
    cJSON* nameJson = NULL;   

    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    printf("AP_report_dev_handle\n");
    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;

    getNowTime(time);
    /*sqlite3*/
    char* sql = "select ID from all_dev order by ID desc limit 1";
    int id;

    rc = sqlite3_open(db_path, &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }
    
    portJson = cJSON_GetObjectItem(data.pdu,"port");
    printf("port:%d\n",portJson->valueint);
    apIdJson = cJSON_GetObjectItem(data.pdu,"apId");
    printf("APId:%s\n",apIdJson->valuestring);
    apNameJson = cJSON_GetObjectItem(data.pdu,"apName");
    printf("APName:%s\n",apNameJson->valuestring);
    devJson = cJSON_GetObjectItem(data.pdu,"dev");
    number = cJSON_GetArraySize(devJson); 

    id = sql_id(db, sql);
    sql = "insert into all_dev(ID, DEV_NAME, DEV_ID, AP_ID, PORT, ADDED, TIME) values(?,?,?,?,?,?,?);";
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);

    for(i = 0; i< number; i++){
        paramDataJson = cJSON_GetArrayItem(devJson, i);
        idJson = cJSON_GetObjectItem(paramDataJson, "devId");
        printf("devId:%s\n", idJson->valuestring);
        nameJson = cJSON_GetObjectItem(paramDataJson, "devName");
        printf("devName:%s\n", nameJson->valuestring);
        
        sqlite3_reset(stmt); 
        sqlite3_bind_int(stmt, 1, id);
        id++;
        sqlite3_bind_text(stmt, 2,  nameJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 3, idJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 4,apIdJson->valuestring, -1, NULL);
        sqlite3_bind_int(stmt, 5, portJson->valueint);
        sqlite3_bind_int(stmt, 6, 0);
        sqlite3_bind_text(stmt, 7,  time, -1, NULL);
        rc = sqlite3_step(stmt);
        printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);  

    return M1_PROTOCOL_OK;  
}

/*AP report information to M1*/
static int AP_report_ap_handle(payload_t data)
{
    int rc;
    char time[30];
    cJSON* portJson = NULL;
    cJSON* apIdJson = NULL;
    cJSON* apNameJson = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    printf("AP_report_ap_handle\n");
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

    portJson = cJSON_GetObjectItem(data.pdu,"port");
    printf("port:%d\n",portJson->valueint);
    apIdJson = cJSON_GetObjectItem(data.pdu,"apId");
    printf("APId:%s\n",apIdJson->valuestring);
    apNameJson = cJSON_GetObjectItem(data.pdu,"apName");
    printf("APName:%s\n",apNameJson->valuestring);
    /*update clientFd*/
    sprintf(sql_1, "select ID from conn_info where AP_ID  = \"%s\"", apIdJson->valuestring);
    rc = sql_row_number(db, sql_1);
    printf("rc:%d\n",rc);
    if(rc > 0){
        sprintf(sql_1, "update conn_info set CLIENT_FD = %d  AP_ID  = \"%s\"", data.clientFd, apIdJson->valuestring);
        printf("%s\n",sql_1);
    }else{
        sql = "select ID from conn_info order by ID desc limit 1";
        id = sql_id(db, sql);
        sprintf(sql_1, " insert into conn_info(ID, AP_ID, CLIENT_FD) values(%d,\"%s\",%d);", id, apIdJson->valuestring, data.clientFd);
        printf("%s\n",sql_1);
    }
    rc = sql_exec(db, sql_1);
    printf("exec:%s\n",rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");
    if(rc == SQLITE_ERROR) return M1_PROTOCOL_FAILED;

    /*insert sql*/
    sql = "select ID from all_dev order by ID desc limit 1";
    id = sql_id(db, sql);
    sql = "insert into all_dev(ID, DEV_NAME, DEV_ID, AP_ID, PORT, ADDED, TIME) values(?,?,?,?,?,?,?);";
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    id++;
    sqlite3_bind_text(stmt, 2,  apNameJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt, 3, apIdJson->valuestring, -1, NULL);
    sqlite3_bind_text(stmt, 4,apIdJson->valuestring, -1, NULL);
    sqlite3_bind_int(stmt, 5, portJson->valueint);
    sqlite3_bind_int(stmt, 6, 0);
    sqlite3_bind_text(stmt, 7,  time, -1, NULL);
    rc = sqlite3_step(stmt);
    printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
 
    sqlite3_finalize(stmt);
    sqlite3_close(db);  

    return M1_PROTOCOL_OK;  
}

static int APP_read_handle(payload_t data)
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

    printf("APP_read_handle\n");
    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;

    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL, *stmt_1 = NULL;
    char sql[200];
    int rc;

    rc = sqlite3_open(db_path, &db);  
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

    int i,j, number1,number2,row_n;
    char* dev_id = NULL;

    number1 = cJSON_GetArraySize(data.pdu);
    printf("number1:%d\n",number1);
    for(i = 0; i < number1; i++){
        /*read json*/
        devDataJson = cJSON_GetArrayItem(data.pdu, i);
        devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
        printf("devId:%s\n",devIdJson->valuestring);
        dev_id = devIdJson->valuestring;
        paramTypeJson = cJSON_GetObjectItem(devDataJson, "paramType");
        number2 = cJSON_GetArraySize(paramTypeJson);
        /*get sql data json*/
        sprintf(sql, "select DEV_NAME from param_table where DEV_ID  = \"%s\" order by ID desc limit 1;", dev_id);
        printf("%s\n", sql);
        row_n = sql_row_number(db, sql);
        printf("row_n:%d\n",row_n);
        if(row_n > 0){
            sqlite3_reset(stmt);
            sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
        
            sqlite3_step(stmt);
            devDataObject = cJSON_CreateObject();
            if(NULL == devDataObject)
            {
                // create object faild, exit
                printf("devDataObject NULL\n");
                cJSON_Delete(devDataObject);
                return M1_PROTOCOL_FAILED;
            }
            cJSON_AddItemToArray(devDataJsonArray, devDataObject);
            cJSON_AddStringToObject(devDataObject, "devName", (const char*)sqlite3_column_text(stmt,0));
            cJSON_AddStringToObject(devDataObject, "devId", dev_id);

            devArray = cJSON_CreateArray();
            if(NULL == devArray)
            {
                printf("devArry NULL\n");
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
            printf("%s\n", sql);
            row_n = sql_row_number(db, sql);
            printf("row_n:%d\n",row_n);
            if(row_n > 0){
                sqlite3_reset(stmt_1);
                sqlite3_prepare_v2(db, sql, strlen(sql),&stmt_1, NULL);
                sqlite3_step(stmt_1);
            
                devObject = cJSON_CreateObject();
                if(NULL == devObject)
                {
                    printf("devObject NULL\n");
                    cJSON_Delete(devObject);
                    return M1_PROTOCOL_FAILED;
                }
                cJSON_AddItemToArray(devArray, devObject); 
                cJSON_AddNumberToObject(devObject, "type", paramJson->valueint);
                cJSON_AddNumberToObject(devObject, "value", sqlite3_column_int(stmt_1,0));
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

    printf("string:%s\n",p);
    socketSeverSend((uint8*)p, strlen(p), data.clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;
}

static int M1_write_to_AP(cJSON* data)
{
    printf("M1_write_to_AP\n");
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
    printf("devId:%s\n",devIdJson->valuestring);

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
    printf("row_n:%d\n",row_n);
    if(row_n > 0){ 
        sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
        rc = sqlite3_step(stmt);
        printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
        if(rc == SQLITE_ERROR) return M1_PROTOCOL_FAILED;
        ap_id = sqlite3_column_text(stmt,0);
        printf("ap_id%s\n",ap_id);
    }

    /*get clientFd*/
    sprintf(sql,"select CLIENT_FD from conn_info where AP_ID = \"%s\" limit 1;",ap_id);
    row_n = sql_row_number(db, sql);
    printf("row_n:%d\n",row_n);
    if(row_n > 0){
        sqlite3_reset(stmt); 
        sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
        rc = sqlite3_step(stmt);
        printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
        if(rc == SQLITE_ERROR) return M1_PROTOCOL_FAILED;
        clientFd = sqlite3_column_int(stmt,0);
    }

    sqlite3_finalize(stmt);

    char * p = cJSON_PrintUnformatted(data);
    
    if(NULL == p)
    {    
        cJSON_Delete(data);
        return M1_PROTOCOL_FAILED;
    }

    printf("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), clientFd);

    sqlite3_close(db);

    return M1_PROTOCOL_OK;
}

static int APP_write_handle(payload_t data)
{
    int i,j, number1,number2;
    char time[30];
    cJSON* devDataJson = NULL;
    cJSON* devIdJson = NULL;
    cJSON* paramDataJson = NULL;
    cJSON* paramArrayJson = NULL;
    cJSON* valueTypeJson = NULL;
    cJSON* valueJson = NULL;

    printf("APP_write_handle\n");
    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;

    getNowTime(time);
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL,*stmt_1 = NULL;
    int rc,row_n,id;
    const char* dev_name = NULL;
    char* sql = "select ID from param_table order by ID desc limit 1";

    rc = sqlite3_open(db_path, &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    } 
    id = sql_id(db, sql);
    printf("id:%d\n",id);
    /*cJSON*/
    /*insert data*/
    char sql_1[200] ;
    char* sql_2 = "insert into param_table(ID, DEV_NAME,DEV_ID,TYPE,VALUE,TIME) values(?,?,?,?,?,?);";
    number1 = cJSON_GetArraySize(data.pdu);
    printf("number1:%d\n",number1);
    for(i = 0; i < number1; i++){
        devDataJson = cJSON_GetArrayItem(data.pdu, i);
        devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
        printf("devId:%s\n",devIdJson->valuestring);
        paramDataJson = cJSON_GetObjectItem(devDataJson, "param");
        number2 = cJSON_GetArraySize(paramDataJson);
        printf("number2:%d\n",number2);

        sprintf(sql_1,"select DEV_NAME from all_dev where DEV_ID = \"%s\" limit 1;", devIdJson->valuestring);
        printf("sql_1:%s\n",sql_1);
        row_n = sql_row_number(db, sql);
        printf("row_n:%d\n",row_n);
        if(row_n > 0){        
            sqlite3_reset(stmt);
            sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
            rc = sqlite3_step(stmt);
            printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
            dev_name = (const char*)sqlite3_column_text(stmt, 0);
            printf("dev_name:%s\n",dev_name);
                
            for(j = 0; j < number2; j++){
                paramArrayJson = cJSON_GetArrayItem(paramDataJson, j);
                valueTypeJson = cJSON_GetObjectItem(paramArrayJson, "type");
                printf("  type%d:%d\n",j,valueTypeJson->valueint);
                valueJson = cJSON_GetObjectItem(paramArrayJson, "value");
                printf("  value%d:%d\n",j,valueJson->valueint);

                sqlite3_reset(stmt_1); 
                sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_1, NULL);
                sqlite3_bind_int(stmt_1, 1, id);
                id++;
                sqlite3_bind_text(stmt_1, 2,  dev_name, -1, NULL);
                sqlite3_bind_text(stmt_1, 3, devIdJson->valuestring, -1, NULL);
                sqlite3_bind_int(stmt_1, 4, valueTypeJson->valueint);
                sqlite3_bind_int(stmt_1, 5, valueJson->valueint);
                sqlite3_bind_text(stmt_1, 6,  time, -1, NULL);
                rc = sqlite3_step(stmt_1);
                printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
            }
        }
    }

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

    printf("APP_echo_dev_info_handle\n");
    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;
    /*sqlite3*/
    sqlite3* db = NULL;
    char* err_msg = NULL;
    char sql_1[200];

    sqlite3_open(db_path,&db);

    number = cJSON_GetArraySize(data.pdu);
    printf("number:%d\n",number);
    for(i = 0; i < number; i++){
        devdataArrayJson = cJSON_GetArrayItem(data.pdu, i);
        APIdJson = cJSON_GetObjectItem(devdataArrayJson, "apId");
        devDataJson = cJSON_GetObjectItem(devdataArrayJson,"devId");
        printf("AP_ID:%s\n",APIdJson->valuestring);

        sprintf(sql_1, "update all_dev set ADDED = 1 where DEV_ID = \"%s\" and AP_ID = \"%s\";",APIdJson->valuestring,APIdJson->valuestring);
        printf("sql_1:%s\n",sql_1);
        rc = sqlite3_exec(db, sql_1, NULL, 0, &err_msg);
        if(rc != SQLITE_OK){
            printf("SQL error:%s\n",err_msg);
            sqlite3_free(err_msg);
            return M1_PROTOCOL_FAILED;
        }

        if(devDataJson != NULL){
            number_1 = cJSON_GetArraySize(devDataJson);
            printf("number_1:%d\n",number_1);
            for(j = 0; j < number_1; j++){
                devArrayJson = cJSON_GetArrayItem(devDataJson, j);
                printf("  devId:%s\n",devArrayJson->valuestring);

                sprintf(sql_1, "update all_dev set ADDED = 1 where DEV_ID = \"%s\" and AP_ID = \"%s\";",devArrayJson->valuestring,APIdJson->valuestring);
                printf("sql_1:%s\n",sql_1);
                rc = sqlite3_exec(db, sql_1, NULL, 0, &err_msg);
                if(rc != SQLITE_OK){
                    printf("SQL error:%s\n",err_msg);
                    sqlite3_free(err_msg);
                    return M1_PROTOCOL_FAILED;
                }
            }
        }
    }
    sqlite3_close(db);

    return M1_PROTOCOL_OK;
}

static int APP_req_added_dev_info_handle(int clientFd)
{
    /*cJSON*/
    int pduType = TYPE_M1_REPORT_ADDED_INFO;
    cJSON * pJsonRoot = NULL;
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt_1 = NULL,*stmt_2 = NULL;
    char* sql_1 = NULL;
    char sql_2[200];

    printf("APP_req_added_dev_info_handle\n");
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
    printf("row_n:%d\n",row_n);
    if(row_n > 0){ 
        sqlite3_prepare_v2(db, sql_1, strlen(sql_1),&stmt_1, NULL);
        while(sqlite3_step(stmt_1) == SQLITE_ROW){
            /*add ap infomation: port,ap_id,ap_name,time */
            devDataObject = cJSON_CreateObject();

            if(NULL == devDataObject)
            {
                // create object faild, exit
                printf("devDataObject NULL\n");
                cJSON_Delete(devDataObject);
                return M1_PROTOCOL_FAILED;
            }
            cJSON_AddItemToArray(devDataJsonArray, devDataObject);

            cJSON_AddNumberToObject(devDataObject, "port", sqlite3_column_int(stmt_1, 4));
            cJSON_AddStringToObject(devDataObject, "apId",  (const char*)sqlite3_column_text(stmt_1, 3));
            cJSON_AddStringToObject(devDataObject, "apName", (const char*)sqlite3_column_text(stmt_1, 2));
            
            /*create devData array*/
            devArray = cJSON_CreateArray();
            if(NULL == devArray)
            {
                 printf("devArry NULL\n");
                 cJSON_Delete(devArray);
                 return M1_PROTOCOL_FAILED;
            }
            /*add devData array to pdu pbject*/
            cJSON_AddItemToObject(devDataObject, "dev", devArray);
            /*sqlite3*/
            sprintf(sql_2,"select * from all_dev where AP_ID  = \"%s\" and AP_ID != DEV_ID and ADDED = 1;",sqlite3_column_text(stmt_1, 3));
            printf("sql_2:%s\n",sql_2);
            row_n = sql_row_number(db, sql_1);
            printf("row_n:%d\n",row_n);
            if(row_n > 0){ 
                sqlite3_prepare_v2(db, sql_2, strlen(sql_2),&stmt_2, NULL);
                while(sqlite3_step(stmt_2) == SQLITE_ROW){
                     /*add ap infomation: port,ap_id,ap_name,time */
                    devObject = cJSON_CreateObject();
                    if(NULL == devObject)
                    {
                        printf("devObject NULL\n");
                        cJSON_Delete(devObject);
                        return M1_PROTOCOL_FAILED;
                    }
                    cJSON_AddItemToArray(devArray, devObject); 
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

    printf("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;
}

static int APP_net_control(payload_t data)
{
    printf("APP_net_control\n");
    int pduType = TYPE_DEV_NET_CONTROL;
    cJSON * pJsonRoot = NULL;

    cJSON* apIdJson = NULL;
    cJSON* valueJson = NULL;

    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;

    apIdJson = cJSON_GetObjectItem(data.pdu, "apId");
    printf("apId:%s\n",apIdJson->valuestring);
    valueJson = cJSON_GetObjectItem(data.pdu, "value");
    printf("value:%d\n",valueJson->valueint);

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
    printf("row_n:%d\n",row_n);
    if(row_n > 0){ 
        sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
        rc = sqlite3_step(stmt);
        printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
        if(rc == SQLITE_ERROR) return M1_PROTOCOL_FAILED;
        clientFd = sqlite3_column_int(stmt,0);
        /*create json*/
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

    printf("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;
}

static int M1_report_ap_info(int clientFd)
{
    printf(" M1_report_ap_info\n");
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
        printf("pJsonRoot NULL\n");
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    cJSON_AddNumberToObject(pJsonRoot, "sn", 1);
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
    printf("row_n:%d\n",row_n);
    if(row_n > 0){ 
        sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
        while(sqlite3_step(stmt) == SQLITE_ROW){
            /*add ap infomation: port,ap_id,ap_name,time */
            devDataObject = cJSON_CreateObject();

            if(NULL == devDataObject)
            {
                // create object faild, exit
                printf("devDataObject NULL\n");
                cJSON_Delete(devDataObject);
                return M1_PROTOCOL_FAILED;
            }
            cJSON_AddItemToArray(devDataJsonArray, devDataObject);

            cJSON_AddNumberToObject(devDataObject, "port", sqlite3_column_int(stmt, 4));
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

    printf("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;
}

static int M1_report_dev_info(payload_t data)
{
     printf(" M1_report_dev_info\n");
    /*cJSON*/
    int pduType = TYPE_AP_REPORT_DEV_INFO;
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
        printf("pJsonRoot NULL\n");
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    cJSON_AddNumberToObject(pJsonRoot, "sn", 1);
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
    printf("string:%s\n",sql);
    row_n = sql_row_number(db, sql);
    printf("row_n:%d\n",row_n);
    if(row_n > 0){ 
        sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
        while(sqlite3_step(stmt) == SQLITE_ROW){
            /*add ap infomation: port,ap_id,ap_name,time */
            devDataObject = cJSON_CreateObject();

            if(NULL == devDataObject)
            {
                // create object faild, exit
                printf("devDataObject NULL\n");
                cJSON_Delete(devDataObject);
                return M1_PROTOCOL_FAILED;
            }
            cJSON_AddItemToArray(devDataJsonArray, devDataObject);

            cJSON_AddNumberToObject(devDataObject, "port", sqlite3_column_int(stmt, 4));
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

    printf("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), data.clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;
}

static int common_rsp(rsp_data_t data)
{
    printf(" M1_report_dev_info\n");
    /*cJSON*/
    int pduType = TYPE_COMMON_RSP;

    cJSON * pJsonRoot = NULL;

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

    printf("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), data.clientFd);
    cJSON_Delete(pJsonRoot);

    return M1_PROTOCOL_OK;
}

static void getNowTime(char* _time)
{

    struct tm nowTime;

    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);  //获取相对于1970到现在的秒数
    localtime_r(&time.tv_sec, &nowTime);
    
    sprintf(_time, "%04d%02d%02d%02d%02d%02d", nowTime.tm_year + 1900, nowTime.tm_mon+1, nowTime.tm_mday, 
      nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec);
}
  
static int sql_id(sqlite3* db, char* sql)
{
    sqlite3_stmt* stmt = NULL;
    int id, total_column;
    /*get id*/
    sqlite3_prepare_v2(db, sql, strlen(sql), & stmt, NULL);
    total_column = sqlite3_column_count(stmt);
    if(total_column > 0){
        sqlite3_step(stmt);
        id = (sqlite3_column_int(stmt, 0) + 1);
    }else{
        id = 1;
    }

    sqlite3_finalize(stmt);
    return id;
}

static int sql_row_number(sqlite3* db, char*sql)
{
    char** p_result;
    char* errmsg;
    int n_row, n_col, rc;

    rc = sqlite3_get_table(db, sql, &p_result,&n_row, &n_col, &errmsg);
    printf("n_row:%d\n",n_row);

    sqlite3_free(errmsg);
    sqlite3_free_table(p_result);

    return n_row;
}

static int sql_exec(sqlite3* db, char*sql)
{
    sqlite3_stmt* stmt = NULL;
    int rc;
    /*get id*/
    sqlite3_prepare_v2(db, sql, strlen(sql), & stmt, NULL);
   
    rc = sqlite3_step(stmt);
    
    sqlite3_finalize(stmt);
    return rc;
}
