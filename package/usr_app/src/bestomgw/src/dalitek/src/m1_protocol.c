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
#include "m1_project.h"

/*Macro**********************************************************************************************************/
#define M1_PROTOCOL_DEBUG    1
#define HEAD_LEN    3
/*Private function***********************************************************************************************/
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
static int create_sql_table(void);
static int app_change_device_name(payload_t data);
/*variable******************************************************************************************************/
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
    create_sql_table();
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
    int rc, ret;
    payload_t pdu;
    rsp_data_t rspData;
    cJSON* rootJson = NULL;
    cJSON* pduJson = NULL;
    cJSON* pduTypeJson = NULL;
    cJSON* snJson = NULL;
    cJSON* pduDataJson = NULL;
    int pduType;

    uint32_t* msg = NULL;
    while(1){
        ret = fifo_read(&msg_fifo, &msg);
        if(ret == 0){
            continue;
        }
        rc = M1_PROTOCOL_NO_RSP;
        fprintf(stdout,"data_handle\n");
        m1_package_t* package = (m1_package_t*)msg;
        fprintf(stdout,"Rx message:%s\n",package->data);
        rootJson = cJSON_Parse(package->data);
        if(NULL == rootJson){
            fprintf(stdout,"rootJson null\n");
            //return;
            continue;   
        }
        pduJson = cJSON_GetObjectItem(rootJson, "pdu");
        if(NULL == pduJson){
            fprintf(stdout,"pdu null\n");
            //return;
            continue;
        }
        pduTypeJson = cJSON_GetObjectItem(pduJson, "pduType");
        if(NULL == pduTypeJson){
            fprintf(stdout,"pduType null\n");
            //return;
            continue;
        }
        pduType = pduTypeJson->valueint;
        rspData.pduType = pduType;

        snJson = cJSON_GetObjectItem(rootJson, "sn");
        if(NULL == snJson){
            fprintf(stdout,"sn null\n");
            //return;
            continue;
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
            case TYPE_REQ_ACCOUNT_CONFIG_INFO: rc = app_req_account_config_handle(pdu, rspData.sn);break;
            case TYPE_LINK_ENABLE_SET: rc = app_linkage_enable(pdu);break;
            case TYPE_APP_LOGIN: rc = user_login_handle(pdu);break;
            case TYPE_SEND_ACCOUNT_CONFIG_INFO: rc = app_account_config_handle(pdu);break;
            case TYPE_GET_PORJECT_NUMBER: rc = app_get_project_info(rspData.clientFd, rspData.sn); break;
            case TYPE_REQ_DIS_SCEN_NAME: rc = app_req_dis_scen_name(pdu, rspData.sn); break;
            case TYPE_REQ_DIS_NAME: rc = app_req_dis_name(pdu, rspData.sn); break;
            case TYPE_REQ_DIS_DEV: rc = app_req_dis_dev(pdu, rspData.sn); break;
            case TYPE_APP_CREATE_PROJECT: rc = app_create_project(pdu);break;
            case TYPE_PROJECT_KEY_CHANGE: rc = app_change_project_key(pdu);break;
            case TYPE_GET_PROJECT_INFO: rc = app_get_project_config(rspData.clientFd, rspData.sn);break;
            case TYPE_PROJECT_INFO_CHANGE:rc = app_change_project_config(pdu);break;
            case TYPE_APP_CONFIRM_PROJECT: rc = app_confirm_project(pdu);break;
            case TYPE_APP_CHANGE_DEV_NAME: rc = app_change_device_name(pdu);break;
            case TYPE_APP_EXEC_SCEN: rc = app_exec_scenario(pdu);break;

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
    int id;
    int i,j;
    int number1, number2;
    int rc,ret = M1_PROTOCOL_OK;
    char* time = (char*)malloc(30);
    char* errorMsg = NULL;
    char* sql = "select ID from param_table order by ID desc limit 1";
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    /*Json*/
    cJSON* devDataJson = NULL;
    cJSON* devNameJson = NULL;
    cJSON* devIdJson = NULL;
    cJSON* paramJson = NULL;
    cJSON* paramDataJson = NULL;
    cJSON* typeJson = NULL;
    cJSON* valueJson = NULL;

    fprintf(stdout,"AP_report_data_handle\n");
    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;   
    }
    /*获取系统当前时间*/
    getNowTime(time);

    rc = sqlite3_open(db_path, &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        ret =  M1_PROTOCOL_FAILED;  
        goto Finish;
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
            if(devDataJson == NULL){
                ret =  M1_PROTOCOL_FAILED;  
                goto Finish;    
            }
            devNameJson = cJSON_GetObjectItem(devDataJson, "devName");
            if(devNameJson == NULL){
                ret =  M1_PROTOCOL_FAILED;  
                goto Finish;    
            }
            fprintf(stdout,"devName:%s\n",devNameJson->valuestring);
            devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
            if(devIdJson == NULL){
                ret =  M1_PROTOCOL_FAILED;  
                goto Finish;    
            }
            fprintf(stdout,"devId:%s\n",devIdJson->valuestring);
            paramJson = cJSON_GetObjectItem(devDataJson, "param");
            if(paramJson == NULL){
                ret =  M1_PROTOCOL_FAILED;  
                goto Finish;    
            }
            number2 = cJSON_GetArraySize(paramJson);
            fprintf(stdout," number2:%d\n",number2);

            for(j = 0; j < number2; j++){
                paramDataJson = cJSON_GetArrayItem(paramJson, j);
                if(paramDataJson == NULL){
                    ret =  M1_PROTOCOL_FAILED;  
                    goto Finish;    
                }   
                typeJson = cJSON_GetObjectItem(paramDataJson, "type");
                if(typeJson == NULL){
                    ret =  M1_PROTOCOL_FAILED;  
                    goto Finish;    
                }
                fprintf(stdout,"  type:%d\n",typeJson->valueint);
                valueJson = cJSON_GetObjectItem(paramDataJson, "value");
                if(valueJson == NULL){
                    ret =  M1_PROTOCOL_FAILED;  
                    goto Finish;    
                }
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

    Finish:
    free(time);
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    trigger_cb_handle();

    return ret;
}

/*AP report device information to M1*/
static int AP_report_dev_handle(payload_t data)
{
    int i;
    int id;
    int number;
    int rc, ret = M1_PROTOCOL_OK;
    char* time = (char*)malloc(30);
    char* errorMsg = NULL;
    char* sql = NULL;
    char* sql_1 = (char*)malloc(300);
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* stmt_1 = NULL;
    cJSON* apIdJson = NULL;
    cJSON* apNameJson = NULL;
    cJSON* pIdJson = NULL;
    cJSON* devJson = NULL;
    cJSON* paramDataJson = NULL;    
    cJSON* idJson = NULL;    
    cJSON* nameJson = NULL;   

    fprintf(stdout,"AP_report_dev_handle\n");
    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    } 

    getNowTime(time);

    rc = sqlite3_open(db_path, &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
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
        sql = "insert or replace into all_dev(ID, DEV_NAME, DEV_ID, AP_ID, PID, ADDED, NET, STATUS, ACCOUNT,TIME) values(?,?,?,?,?,?,?,?,?,?);";
        sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);

        for(i = 0; i< number; i++){
            paramDataJson = cJSON_GetArrayItem(devJson, i);
            if(paramDataJson == NULL){
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            idJson = cJSON_GetObjectItem(paramDataJson, "devId");
            if(idJson == NULL){
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            fprintf(stdout,"devId:%s\n", idJson->valuestring);
            nameJson = cJSON_GetObjectItem(paramDataJson, "devName");
            if(nameJson == NULL){
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            fprintf(stdout,"devName:%s\n", nameJson->valuestring);
            pIdJson = cJSON_GetObjectItem(paramDataJson, "pId");
            if(pIdJson == NULL){
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
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
            sqlite3_bind_text(stmt, 9,  "Dalitek", -1, NULL);
            sqlite3_bind_text(stmt, 10,  time, -1, NULL);
            rc = thread_sqlite3_step(&stmt,db);
        }

        if(sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg) == SQLITE_OK){
            fprintf(stdout,"END\n");
        }
    }else{
        fprintf(stdout,"errorMsg:");
    }   

    Finish:
    free(time);
    free(sql_1);
    sqlite3_free(errorMsg);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
    sqlite3_close(db);  

    return ret;  
}

/*AP report information to M1*/
static int AP_report_ap_handle(payload_t data)
{
    int id;
    int rc,ret = M1_PROTOCOL_OK;
    char* sql = NULL; 
    char* sql_1 = (char*)malloc(300);
    char* time = (char*)malloc(30);
    char* errorMsg = NULL;
    cJSON* pIdJson = NULL;
    cJSON* apIdJson = NULL;
    cJSON* apNameJson = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* stmt_1 = NULL;

    fprintf(stdout,"AP_report_ap_handle\n");
    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;  
        goto Finish;
    }

    getNowTime(time);
    /*sqlite3*/
    rc = sqlite3_open(db_path, &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;  
        goto Finish;
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    /*添加update/insert/delete监察*/
    rc = sqlite3_update_hook(db, trigger_cb, "AP_report_ap_handle");
    if(rc){
        fprintf(stderr, "sqlite3_update_hook falied: %s\n", sqlite3_errmsg(db));  
    }

    pIdJson = cJSON_GetObjectItem(data.pdu,"pId");
    if(pIdJson == NULL){
        ret = M1_PROTOCOL_FAILED;  
        goto Finish;
    }
    fprintf(stdout,"pId:%05d\n",pIdJson->valueint);
    apIdJson = cJSON_GetObjectItem(data.pdu,"apId");
    if(apIdJson == NULL){
        ret = M1_PROTOCOL_FAILED;  
        goto Finish;
    }
    fprintf(stdout,"APId:%s\n",apIdJson->valuestring);
    apNameJson = cJSON_GetObjectItem(data.pdu,"apName");
    if(apNameJson == NULL){
        ret = M1_PROTOCOL_FAILED;  
        goto Finish;
    }
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
        if(rc == SQLITE_ERROR){
            ret = M1_PROTOCOL_FAILED;  
            goto Finish;       
        };

        /*insert sql*/
        sql = "insert into all_dev(ID, DEV_NAME, DEV_ID, AP_ID, PID, ADDED, NET, STATUS, ACCOUNT,TIME) values(?,?,?,?,?,?,?,?,?,?);";
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
        sqlite3_bind_text(stmt, 9,  "Dalitek", -1, NULL);
        sqlite3_bind_text(stmt, 10,  time, -1, NULL);
        rc = thread_sqlite3_step(&stmt,db);
        if(sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg) == SQLITE_OK){
           fprintf(stdout,"END\n");
        }
    }else{
        fprintf(stdout,"errorMsg:");
    }    
    
    Finish:
    free(time);
    free(sql_1);
    sqlite3_free(errorMsg);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
    sqlite3_close(db);  

    return ret;  
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
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL, *stmt_1 = NULL;
    int pduType = TYPE_REPORT_DATA;
    int rc, ret = M1_PROTOCOL_OK;
    char* sql = (char*)malloc(300);

    fprintf(stdout,"APP_read_handle\n");
    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;
    
    rc = sqlite3_open(db_path, &db); 
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        ret =  M1_PROTOCOL_FAILED;  
        goto Finish;
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    /*get sql data json*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        fprintf(stdout,"pJsonRoot NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
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
        //sprintf(sql, "select DEV_NAME from param_table where DEV_ID  = \"%s\" order by ID desc limit 1;", dev_id);
        sprintf(sql, "select DEV_NAME from all_dev where DEV_ID  = \"%s\" order by ID desc limit 1;", dev_id);
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
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
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
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
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
                    ret = M1_PROTOCOL_FAILED;
                    goto Finish;
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

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    fprintf(stdout,"string:%s\n",p);
    socketSeverSend((uint8*)p, strlen(p), data.clientFd);
    Finish:
    free(sql);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
    sqlite3_close(db);
    cJSON_Delete(pJsonRoot);

    return ret;
}

static int M1_write_to_AP(cJSON* data)
{
    fprintf(stdout,"M1_write_to_AP\n");
    int sn = 2;
    int row_n;
    int clientFd;
    int rc,ret = M1_PROTOCOL_OK;
    const char* ap_id = NULL;
    char* sql = (char*)malloc(300);
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
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

    rc = sqlite3_open(db_path, &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    } 

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

    char * p = cJSON_PrintUnformatted(data);
    
    if(NULL == p)
    {    
        cJSON_Delete(data);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;  
    }

    fprintf(stdout,"string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), clientFd);

    Finish:
    free(sql);
    sqlite3_close(db);
    sqlite3_finalize(stmt);

    return ret;
}

static int APP_write_handle(payload_t data)
{
    int i,j;
    int number1,number2;
    int row_n,id;
    int rc, ret = M1_PROTOCOL_OK;
    char* errorMsg = NULL;
    char* time = NULL;
    char* sql = NULL;
    char* sql_1 = NULL;
    char* sql_2 = NULL;
    const char* dev_name = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL,*stmt_1 = NULL;
    cJSON* devDataJson = NULL;
    cJSON* devIdJson = NULL;
    cJSON* paramDataJson = NULL;
    cJSON* paramArrayJson = NULL;
    cJSON* valueTypeJson = NULL;
    cJSON* valueJson = NULL;

    fprintf(stdout,"APP_write_handle\n");
    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    };

    /*获取系统时间*/
    time = (char*)malloc(30);
    getNowTime(time);
    /*打开数据库*/
    rc = sqlite3_open(db_path, &db);
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;  
        goto Finish;
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }
    /*添加update/insert/delete监察*/
    rc = sqlite3_update_hook(db, trigger_cb, "APP_write_handle");
    if(rc){
        fprintf(stderr, "sqlite3_update_hook falied: %s\n", sqlite3_errmsg(db));  
    }

    sql = "select ID from param_table order by ID desc limit 1";
    id = sql_id(db, sql);
    fprintf(stdout,"id:%d\n",id);
    if(sqlite3_exec(db, "BEGIN", NULL, NULL, &errorMsg)==SQLITE_OK){
        fprintf(stdout,"BEGIN\n");
        /*insert data*/
        sql_1 = (char*)malloc(300);
        sql_2 = "insert into param_table(ID, DEV_NAME,DEV_ID,TYPE,VALUE,TIME) values(?,?,?,?,?,?);";
        number1 = cJSON_GetArraySize(data.pdu);
        fprintf(stdout,"number1:%d\n",number1);
        for(i = 0; i < number1; i++){
            devDataJson = cJSON_GetArrayItem(data.pdu, i);
            if(devDataJson == NULL)
            {
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
            if(devIdJson == NULL)
            {
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            fprintf(stdout,"devId:%s\n",devIdJson->valuestring);
            paramDataJson = cJSON_GetObjectItem(devDataJson, "param");
            if(paramDataJson == NULL)
            {
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
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
                        if(dev_name == NULL)
                        {
                            ret = M1_PROTOCOL_FAILED;
                            goto Finish;
                        }           
                        fprintf(stdout,"dev_name:%s\n",dev_name);
                    }
                    for(j = 0; j < number2; j++){
                        paramArrayJson = cJSON_GetArrayItem(paramDataJson, j);
                        if(paramArrayJson == NULL)
                        {
                            ret = M1_PROTOCOL_FAILED;
                            goto Finish;
                        }
                        valueTypeJson = cJSON_GetObjectItem(paramArrayJson, "type");
                        if(valueTypeJson == NULL)
                        {
                            ret = M1_PROTOCOL_FAILED;
                            goto Finish;
                        }
                        fprintf(stdout,"  type%d:%d\n",j,valueTypeJson->valueint);
                        valueJson = cJSON_GetObjectItem(paramArrayJson, "value");
                        if(valueJson == NULL)
                        {
                            ret = M1_PROTOCOL_FAILED;
                            goto Finish;
                        }
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

    Finish:
    free(time);
    free(sql_1);
    sqlite3_free(errorMsg);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1); 
    sqlite3_close(db);

    return ret;
}

static int APP_echo_dev_info_handle(payload_t data)
{
    int i,j;
    int number,number_1;
    int rc,ret = M1_PROTOCOL_OK;
    char* err_msg = NULL;
    char* sql = NULL;
    char* errorMsg = NULL;
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    cJSON* devDataJson = NULL;
    cJSON* devdataArrayJson = NULL;
    cJSON* devArrayJson = NULL;
    cJSON* APIdJson = NULL;

    fprintf(stdout,"APP_echo_dev_info_handle\n");
    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    } 

    rc = sqlite3_open(db_path, &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    sql = (char*)malloc(300);
    number = cJSON_GetArraySize(data.pdu);
    fprintf(stdout,"number:%d\n",number);  
    for(i = 0; i < number; i++){
        devdataArrayJson = cJSON_GetArrayItem(data.pdu, i);
        if(devdataArrayJson == NULL){
            fprintf(stderr,"devdataArrayJson NULL\n");
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }
        APIdJson = cJSON_GetObjectItem(devdataArrayJson, "apId");
        if(APIdJson == NULL){
            fprintf(stderr,"APIdJson NULL\n");
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }
        devDataJson = cJSON_GetObjectItem(devdataArrayJson,"devId");

        fprintf(stdout,"AP_ID:%s\n",APIdJson->valuestring);            
        sprintf(sql, "update all_dev set ADDED = 1 where DEV_ID = \"%s\" and AP_ID = \"%s\";",APIdJson->valuestring,APIdJson->valuestring);
        fprintf(stdout,"sql:%s\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errorMsg);
            if(rc != SQLITE_OK){
            fprintf(stderr,"update all_dev fail: %s\n",errorMsg);
        }

        if(devDataJson != NULL){
            number_1 = cJSON_GetArraySize(devDataJson);
            fprintf(stdout,"number_1:%d\n",number_1);
            for(j = 0; j < number_1; j++){
                devArrayJson = cJSON_GetArrayItem(devDataJson, j);
                fprintf(stdout,"  devId:%s\n",devArrayJson->valuestring);

                sprintf(sql, "update all_dev set ADDED = 1 where DEV_ID = \"%s\" and AP_ID = \"%s\";",devArrayJson->valuestring,APIdJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sqlite3_reset(stmt);
                sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
                thread_sqlite3_step(&stmt, db);
            }
        }
    }  

    Finish:
    free(sql);
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return ret;
}

static int APP_req_added_dev_info_handle(int clientFd, int sn)
{
    /*cJSON*/
    int row_n;
    int rc,ret = M1_PROTOCOL_OK;
    int pduType = TYPE_M1_REPORT_ADDED_INFO;
    char* account = NULL;
    char* sql = (char*)malloc(300);
    char* sql_1 = (char*)malloc(300);;
    char* sql_2 = (char*)malloc(300);;
    cJSON*  devDataObject= NULL;
    cJSON* devArray = NULL;
    cJSON*  devObject = NULL;
    cJSON* pJsonRoot = NULL;
    cJSON * devDataJsonArray = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL, *stmt_1 = NULL,*stmt_2 = NULL;

    fprintf(stdout,"APP_req_added_dev_info_handle\n");
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        fprintf(stdout,"pJsonRoot NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
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
    /*sqlite3*/
    rc = sqlite3_open(db_path, &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    } 
    /*获取当前账户*/
    sprintf(sql,"select ACCOUNT from account_info where CLIENT_FD = %03d order by ID desc limit 1;",clientFd);
    fprintf(stdout, "%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
        account =  sqlite3_column_text(stmt, 0);
        if(account == NULL){
            fprintf(stderr, "user account do not exist\n");    
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }else{
            fprintf(stdout,"account:%s\n",account);
        }
    }
    
    sprintf(sql_1,"select * from all_dev where DEV_ID  = AP_ID and ADDED = 1 and ACCOUNT = \"%s\";",account);
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
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
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
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            /*add devData array to pdu pbject*/
            cJSON_AddItemToObject(devDataObject, "dev", devArray);
            /*sqlite3*/
            sprintf(sql_2,"select * from all_dev where AP_ID  = \"%s\" and AP_ID != DEV_ID and ADDED = 1 and ACCOUNT = \"%s\";",sqlite3_column_text(stmt_1, 3),account);
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
                        ret = M1_PROTOCOL_FAILED;
                        goto Finish;
                    }
                    cJSON_AddItemToArray(devArray, devObject); 
                    cJSON_AddNumberToObject(devObject, "pId", sqlite3_column_int(stmt_2, 4));
                    cJSON_AddStringToObject(devObject, "devId", (const char*)sqlite3_column_text(stmt_2, 1));
                    cJSON_AddStringToObject(devObject, "devName", (const char*)sqlite3_column_text(stmt_2, 2));
                }
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

    fprintf(stdout,"string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), clientFd);
    
    Finish:
    free(sql);
    free(sql_1);
    free(sql_2);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
    sqlite3_finalize(stmt_2);
    sqlite3_close(db);
    cJSON_Delete(pJsonRoot);

    return ret;
}

static int APP_net_control(payload_t data)
{
    fprintf(stdout,"APP_net_control\n");
    int clientFd, row_n; 
    int rc,ret = M1_PROTOCOL_OK; 
    int pduType = TYPE_DEV_NET_CONTROL;
    char* sql = (char*)malloc(300); 
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    cJSON * pJsonRoot = NULL;
    cJSON* apIdJson = NULL;
    cJSON* valueJson = NULL;
    cJSON * pduJsonObject = NULL;

    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    };

    apIdJson = cJSON_GetObjectItem(data.pdu, "apId");
    if(apIdJson == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    };
    fprintf(stdout,"apId:%s\n",apIdJson->valuestring);
    valueJson = cJSON_GetObjectItem(data.pdu, "value");
    if(valueJson == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    };
    fprintf(stdout,"value:%d\n",valueJson->valueint);  

    rc = sqlite3_open(db_path, &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
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
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }

        cJSON_AddNumberToObject(pJsonRoot, "sn", 1);
        cJSON_AddStringToObject(pJsonRoot, "version", "1.0");
        cJSON_AddNumberToObject(pJsonRoot, "netFlag", 1);
        cJSON_AddNumberToObject(pJsonRoot, "cmdType", 2);
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
        /*add dev data to pdu object*/
        cJSON_AddNumberToObject(pduJsonObject, "devData", valueJson->valueint);

    }else{
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    fprintf(stdout,"string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), clientFd);
    
    Finish:
    free(sql);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    cJSON_Delete(pJsonRoot);

    return ret;
}

static int M1_report_ap_info(int clientFd, int sn)
{
    fprintf(stdout," M1_report_ap_info\n");

    int row_n;
    int rc, ret = M1_PROTOCOL_OK;
    int pduType = TYPE_M1_REPORT_AP_INFO;
    char* account = NULL;
    char* sql = (char*)malloc(300);
    cJSON*  devDataObject= NULL;
    cJSON * pJsonRoot = NULL;
    cJSON * pduJsonObject = NULL;
    cJSON * devDataJsonArray = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        fprintf(stdout,"pJsonRoot NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
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
    rc = sqlite3_open(db_path, &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db)); 
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    } 
    /*获取用户账户信息*/
    sprintf(sql,"select ACCOUNT from account_info where CLIENT_FD = %03d order by ID desc limit 1;",clientFd);
    fprintf(stdout, "%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
        account =  sqlite3_column_text(stmt, 0);
        if(account == NULL){
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }else{
            fprintf(stdout,"clientFd:%03d,account:%s\n",clientFd, account);
        }
    }

    sprintf(sql,"select * from all_dev where DEV_ID  = AP_ID and ACCOUNT = \"%s\";",account);
    row_n = sql_row_number(db, sql);
    fprintf(stdout,"row_n:%d\n",row_n);
    if(row_n > 0){ 
        sqlite3_reset(stmt);
        sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
        while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
            /*add ap infomation: port,ap_id,ap_name,time */
            devDataObject = cJSON_CreateObject();
            if(NULL == devDataObject)
            {
                // create object faild, exit
                fprintf(stdout,"devDataObject NULL\n");
                cJSON_Delete(devDataObject);
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            cJSON_AddItemToArray(devDataJsonArray, devDataObject);
            cJSON_AddNumberToObject(devDataObject, "pId", sqlite3_column_int(stmt, 4));
            cJSON_AddStringToObject(devDataObject, "apId", (const char*)sqlite3_column_text(stmt, 3));
            cJSON_AddStringToObject(devDataObject, "apName", (const char*)sqlite3_column_text(stmt, 2));
            
        }
    }

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    fprintf(stdout,"string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), clientFd);
    Finish:
    free(sql);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    cJSON_Delete(pJsonRoot);

    return ret;
}

static int M1_report_dev_info(payload_t data, int sn)
{
    fprintf(stdout," M1_report_dev_info\n");
    int row_n;
    int pduType = TYPE_M1_REPORT_DEV_INFO;
    int rc, ret = M1_PROTOCOL_OK;
    char* ap = NULL;
    char* account = NULL;
    char* sql = (char*)malloc(300);
    cJSON * pJsonRoot = NULL;
    cJSON * pduJsonObject = NULL;
    cJSON * devDataJsonArray = NULL;
    cJSON*  devDataObject= NULL;
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    ap = data.pdu->valuestring;
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        fprintf(stdout,"pJsonRoot NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
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
    /*sqlite3*/
    rc = sqlite3_open(db_path, &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    } 

    /*获取用户账户信息*/
    sprintf(sql,"select ACCOUNT from account_info where CLIENT_FD = %03d order by ID desc limit 1;",data.clientFd);
    fprintf(stdout, "%s\n", sql);
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
        account =  sqlite3_column_text(stmt, 0);
        if(account == NULL){
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }else{
            fprintf(stdout,"clientFd:%03d,account:%s\n",data.clientFd, account);
        }
    }

    sprintf(sql,"select * from all_dev where AP_ID != DEV_ID and AP_ID = \"%s\" and  ACCOUNT = \"%s\";", ap, account);
    fprintf(stdout,"string:%s\n",sql);
    row_n = sql_row_number(db, sql);
    fprintf(stdout,"row_n:%d\n",row_n);
    if(row_n > 0){ 
        sqlite3_reset(stmt);
        sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
        while(thread_sqlite3_step(&stmt,db) == SQLITE_ROW){
            /*add ap infomation: port,ap_id,ap_name,time */
            devDataObject = cJSON_CreateObject();
            if(NULL == devDataObject)
            {
                // create object faild, exit
                fprintf(stdout,"devDataObject NULL\n");
                cJSON_Delete(devDataObject);
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            cJSON_AddItemToArray(devDataJsonArray, devDataObject);
            cJSON_AddNumberToObject(devDataObject, "pId", sqlite3_column_int(stmt, 4));
            cJSON_AddStringToObject(devDataObject, "devId", (const char*)sqlite3_column_text(stmt, 1));
            cJSON_AddStringToObject(devDataObject, "devName", (const char*)sqlite3_column_text(stmt, 2));
            
        }
    }

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    fprintf(stdout,"string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), data.clientFd);
    
    Finish:
    free(sql);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    cJSON_Delete(pJsonRoot);

    return ret;
}

/*子设备/AP/联动/场景/区域/-启动/停止/删除*/
static int common_operate(payload_t data)
{
    fprintf(stdout,"common_operate\n");

    int id;
    int rc,ret = M1_PROTOCOL_OK; 
    int number1,i;
    int row_number = 0;
    char*scen_name = NULL;
    char* time = (char*)malloc(30);
    char* sql = (char*)malloc(300);
    char* errorMsg = NULL;
    cJSON* typeJson = NULL;
    cJSON* idJson = NULL;
    cJSON* operateJson = NULL;    
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;


    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    
    getNowTime(time);

    typeJson = cJSON_GetObjectItem(data.pdu, "type");
    if(typeJson == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    fprintf(stdout,"type:%s\n",typeJson->valuestring);
    idJson = cJSON_GetObjectItem(data.pdu, "id");   
    if(idJson == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    fprintf(stdout,"id:%s\n",idJson->valuestring);
    operateJson = cJSON_GetObjectItem(data.pdu, "operate");   
    if(operateJson == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    fprintf(stdout,"operate:%s\n",operateJson->valuestring);

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
                sprintf(sql,"delete from all_dev where DEV_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除scenario_table中的子设备*/
                sprintf(sql,"delete from scenario_table where DEV_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除link_trigger_table中的子设备*/
                sprintf(sql,"delete from link_trigger_table where DEV_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除link_exec_table中的子设备*/
                sprintf(sql,"delete from link_exec_table where DEV_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
            }else if(strcmp(operateJson->valuestring, "on") == 0){
                sprintf(sql,"update all_dev set STATUS = \"ON\" where DEV_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
            }else if(strcmp(operateJson->valuestring, "off") == 0){
                sprintf(sql,"update all_dev set STATUS = \"OFF\" where DEV_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
            }

        }else if(strcmp(typeJson->valuestring, "linkage") == 0){
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*删除联动表linkage_table中相关内容*/
                sprintf(sql,"delete from linkage_table where LINK_NAME = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除联动触发表link_trigger_table相关内容*/
                sprintf(sql,"delete from link_trigger_table where LINK_NAME = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除联动触发表link_exec_table相关内容*/
                sprintf(sql,"delete from link_exec_table where LINK_NAME = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
            }
        }else if(strcmp(typeJson->valuestring, "scenario") == 0){
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*删除场景表相关内容*/
                sprintf(sql,"delete from scenario_table where SCEN_NAME = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除场景定时相关内容*/
                sprintf(sql,"delete from scen_alarm_table where SCEN_NAME = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
            }
        }else if(strcmp(typeJson->valuestring, "district") == 0){
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*删除区域相关内容*/
                sprintf(sql,"delete from district_table where DIS_NAME = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除区域下的联动表中相关内容*/
                sprintf(sql,"delete from linkage_table where DISTRICT = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);    
                /*删除区域下的联动触发表中相关内容*/
                sprintf(sql,"delete from link_trigger_table where DISTRICT = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);    
                /*删除区域下的联动触发表中相关内容*/
                sprintf(sql,"delete from link_exec_table where DISTRICT = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql); 
                /*删除场景定时相关内容*/
                sprintf(sql,"select SCEN_NAME from scenario_table where DISTRICT = \"%s\" limit 1;",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sqlite3_reset(stmt);
                sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
                rc = thread_sqlite3_step(&stmt, db);
                fprintf(stdout,"step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");
                if(rc == SQLITE_ROW){
                    scen_name = sqlite3_column_text(stmt,0);
                    if(scen_name == NULL){
                        ret = M1_PROTOCOL_FAILED;
                        goto Finish;
                    }
                }
                sprintf(sql,"delete from scen_alarm_table where SCEN_NAME = \"%s\";",scen_name);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql); 
                /*删除区域下场景表相关内容*/   
                sprintf(sql,"delete from scenario_table where DISTRICT = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql); 
            }
        }else if(strcmp(typeJson->valuestring, "account") == 0){
            if(strcmp(idJson->valuestring,"Dalitek") == 0){
                goto Finish;
            }
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*删除account_table中的信息*/
                sprintf(sql,"delete from account_table where ACCOUNT = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除all_dev中的信息*/
                sprintf(sql,"delete from all_dev where ACCOUNT = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除district_table中的信息*/
                sprintf(sql,"delete from district_table where ACCOUNT = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除scenario_table中的信息*/
                sprintf(sql,"delete from scenario_table where ACCOUNT = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
            }
        }else if(strcmp(typeJson->valuestring, "ap") == 0){
            /*删除all_dev中设备*/
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*删除all_dev中的子设备*/
                sprintf(sql,"delete from all_dev where AP_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除scenario_table中的子设备*/
                sprintf(sql,"delete from scenario_table where AP_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除link_trigger_table中的子设备*/
                sprintf(sql,"delete from link_trigger_table where AP_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除link_exec_table中的子设备*/
                sprintf(sql,"delete from link_exec_table where AP_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除district_table中的子设备*/
                sprintf(sql,"delete from district_table where AP_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
            }else if(strcmp(operateJson->valuestring, "on") == 0){
                sprintf(sql,"update all_dev set STATUS = \"ON\" where AP_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
            }else if(strcmp(operateJson->valuestring, "off") == 0){
                sprintf(sql,"update all_dev set STATUS = \"OFF\" where AP_ID = \"%s\";",idJson->valuestring);
                fprintf(stdout,"sql:%s\n",sql);
                sql_exec(db, sql);
            }
        }
        if(sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg) == SQLITE_OK){
            fprintf(stdout,"END\n");
        }
    }else{
        fprintf(stdout,"errorMsg:");
    }

    Finish:    
    free(time);
    free(sql);
    sqlite3_free(errorMsg);
    sqlite3_finalize(stmt);
    sqlite3_close(db);

     return ret;
}

static int ap_heartbeat_handle(payload_t data)
{
    return M1_PROTOCOL_OK;
}

static int common_rsp(rsp_data_t data)
{
    fprintf(stdout," common_rsp\n");
    
    int ret = M1_PROTOCOL_OK;
    int pduType = TYPE_COMMON_RSP;
    cJSON * pJsonRoot = NULL;
    cJSON * pduJsonObject = NULL;
    cJSON * devDataObject = NULL;

    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        fprintf(stdout,"pJsonRoot NULL\n");
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
        cJSON_Delete(pduJsonObject);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;    
    }
    /*add pdu to root*/
    cJSON_AddItemToObject(pJsonRoot, "pdu", pduJsonObject);
    /*add pdu type to pdu object*/
    cJSON_AddNumberToObject(pduJsonObject, "pduType", pduType);
    /*devData*/
    devDataObject = cJSON_CreateObject();
    if(NULL == devDataObject)
    {
        cJSON_Delete(devDataObject);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;    
    }
    /*add devData to pdu*/
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataObject);
    cJSON_AddNumberToObject(devDataObject, "sn", data.sn);
    cJSON_AddNumberToObject(devDataObject, "pduType", data.pduType);
    cJSON_AddNumberToObject(devDataObject, "result", data.result);

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;    
    }

    fprintf(stdout,"string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), data.clientFd);

    Finish:
    cJSON_Delete(pJsonRoot);

    return ret;
}

void delete_account_conn_info(int clientFd)
{
    int rc;
    char* sql = (char*)malloc(300);
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    sprintf(sql,"delete from account_info where CLIENT_FD = %03d;",clientFd);
    fprintf(stdout,"string:%s\n",sql);
    rc = sqlite3_open("dev_info.db", &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        goto Finish;
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }
    sqlite3_reset(stmt);
    sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    thread_sqlite3_step(&stmt, db);
    
    Finish:
    free(sql);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

/*app修改设备名称*/
static int app_change_device_name(payload_t data)
{
    int rc, ret = M1_PROTOCOL_OK;
    char* errorMsg = NULL;
    char* sql = (char*)malloc(300);
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    cJSON* devIdObject = NULL;
    cJSON* devNameObject = NULL;

    rc = sqlite3_open("dev_info.db", &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    devIdObject = cJSON_GetObjectItem(data.pdu, "devId");   
    if(devIdObject == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    fprintf(stdout,"devId:%s\n",devIdObject->valuestring);
    devNameObject = cJSON_GetObjectItem(data.pdu, "devName");   
    if(devNameObject == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    fprintf(stdout,"devId:%s\n",devNameObject->valuestring);

    /*修改all_dev设备名称*/
    sprintf(sql,"update all_dev set DEV_NAME = \"%s\" where DEV_ID = \"%s\";",devNameObject->valuestring, devIdObject->valuestring);
    fprintf(stdout,"sql:%s\n",sql);
    if(sqlite3_exec(db, "BEGIN", NULL, NULL, &errorMsg)==SQLITE_OK){
        fprintf(stdout,"BEGIN:\n");
        sqlite3_reset(stmt);
        sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        rc = thread_sqlite3_step(&stmt, db);
        if(rc == SQLITE_ERROR)
        {
            ret = M1_PROTOCOL_FAILED;
            goto Finish;   
        }
        if(sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg) == SQLITE_OK){
            fprintf(stdout,"END\n");
        }
    }else{
        fprintf(stdout,"errorMsg\n");
    }

    Finish:
    free(sql);
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return ret;
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
    // mon = atoi(time) / 1000000 - 1;
    // day = atoi(&time[2]) / 10000;
    // hour = atoi(&time[4]) / 100; 
    // min = atoi(&time[6]);
    mon = 10;
    day = 31;
    hour = 17; 
    min = 55;
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
    //sqlite3_mutex_enter(sqlite3_db_mutex(db));
    rc = sqlite3_get_table(db, sql, &p_result,&n_row, &n_col, &errmsg);
    /*sqlite3 unlock*/
    //sqlite3_mutex_leave(sqlite3_db_mutex(db));
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

    rc = sqlite3_step(*stmt);   
    fprintf(stdout,"step() return %s, number:%03d\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
    return rc;
}

static int create_sql_table(void)
{
    char* sql = (char*)malloc(600);
    int rc,ret = M1_PROTOCOL_OK;
    char* errmsg = NULL;

    sqlite3* db = 0;
    rc = sqlite3_open("dev_info.db", &db);
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        goto Finish;
        ret = M1_PROTOCOL_FAILED;
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }
    /*account_info*/
    sprintf(sql,"create table account_info(ID INT PRIMARY KEY NOT NULL, ACCOUNT TEXT NOT NULL, CLIENT_FD INT NOT NULL);");
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK){
        fprintf(stderr,"create account_info fail: %s\n",errmsg);
    }
    sqlite3_free(errmsg);
    /*account_table*/
    sprintf(sql,"create table account_table(ID INT PRIMARY KEY NOT NULL, ACCOUNT TEXT NOT NULL, KEY TEXT NOT NULL,KEY_AUTH TEXT NOT NULL,REMOTE_AUTH TEXT NOT NULL,TIME TEXT NOT NULL);");
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK){
        fprintf(stderr,"create account_table fail: %s\n",errmsg);
    }
    sqlite3_free(errmsg);
    /*all_dev*/
    sprintf(sql,"create table all_dev(ID INT PRIMARY KEY NOT NULL, DEV_ID TEXT NOT NULL, DEV_NAME TEXT NOT NULL,AP_ID TEXT NOT NULL,PID INT NOT NULL,ADDED INT NOT NULL,NET INT NOT NULL, STATUS TEXT NOT NULL, ACCOUNT TEXT NOT NULL, TIME TEXT NOT NULL);");
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK){
        fprintf(stderr,"create all_dev fail: %s\n",errmsg);
    }
    sqlite3_free(errmsg);
    /*conn_info*/
    sprintf(sql,"create table conn_info(ID INT PRIMARY KEY NOT NULL, AP_ID TEXT NOT NULL, CLIENT_FD INT NOT NULL);");
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK){
        fprintf(stderr,"create conn_info fail: %s\n",errmsg);
    }
    sqlite3_free(errmsg);
    /*district_table*/
    sprintf(sql,"CREATE TABLE district_table(ID INT PRIMARY KEY NOT NULL, DIS_NAME TEXT NOT NULL, AP_ID TEXT NOT NULL, ACCOUNT TEXT NOT NULL, TIME TEXT NOT NULL);");
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK){
        fprintf(stderr,"create district_table fail: %s\n",errmsg);
    }
    sqlite3_free(errmsg);
    /*link_exec_table*/
    sprintf(sql,"CREATE TABLE link_exec_table(ID INT PRIMARY KEY NOT NULL, LINK_NAME TEXT NOT NULL, DISTRICT TEXT NOT NULL, AP_ID TEXT NOT NULL, DEV_ID TEXT NOT NULL, TYPE INT NOT NULL, VALUE INT NOT NULL, DELAY INT NOT NULL, TIME TEXT NOT NULL);");
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK){
        fprintf(stderr,"create link_exec_table fail: %s\n",errmsg);
    }
    sqlite3_free(errmsg);
    /*link_trigger_table*/
    sprintf(sql,"CREATE TABLE link_trigger_table(ID INT PRIMARY KEY NOT NULL, LINK_NAME TEXT NOT NULL,DISTRICT TEXT NOT NULL, AP_ID TEXT NOT NULL, DEV_ID TEXT NOT NULL, TYPE INT NOT NULL, THRESHOLD INT NOT NULL,CONDITION TEXT NOT NULL, LOGICAL TEXT NOT NULL, STATUS TEXT NOT NULL, TIME TEXT NOT NULL);");
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK){
        fprintf(stderr,"create link_trigger_table fail: %s\n",errmsg);
    }
    sqlite3_free(errmsg);
    /*linkage_table*/
    sprintf(sql,"CREATE TABLE linkage_table(ID INT PRIMARY KEY NOT NULL, LINK_NAME TEXT NOT NULL, DISTRICT TEXT NOT NULL, EXEC_TYPE TEXT NOT NULL, EXEC_ID TEXT NOT NULL, STATUS TEXT NOT NULL, ENABLE TEXT NOT NULL, TIME TEXT NOT NULL);");
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK){
        fprintf(stderr,"create linkage_table fail: %s\n",errmsg);
    }
    sqlite3_free(errmsg);
    /*param_table*/
    sprintf(sql,"CREATE TABLE param_table(ID INT PRIMARY KEY NOT NULL, DEV_ID TEXT NOT NULL, DEV_NAME TEXT NOT NULL, TYPE INT NOT NULL, VALUE INT NOT NULL, TIME TEXT NOT NULL);");
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK){
        fprintf(stderr,"create param_table fail: %s\n",errmsg);
    }
    sqlite3_free(errmsg);
    /*scen_alarm_table*/
    sprintf(sql,"CREATE TABLE scen_alarm_table(ID INT PRIMARY KEY NOT NULL, SCEN_NAME TEXT NOT NULL, HOUR INT NOT NULL,MINUTES INT NOT NULL, WEEK TEXT NOT NULL, STATUS TEXT NOT NULL, TIME TEXT NOT NULL);");
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK){
        fprintf(stderr,"create scen_alarm_table fail: %s\n",errmsg);
    }
    sqlite3_free(errmsg);
    /*scenario_table*/
    sprintf(sql,"CREATE TABLE scenario_table(ID INT PRIMARY KEY NOT NULL, SCEN_NAME TEXT NOT NULL, DISTRICT TEXT NOT NULL, AP_ID TEXT NOT NULL, DEV_ID TEXT NOT NULL, TYPE INT NOT NULL, VALUE INT NOT NULL, DELAY INT NOT NULL, ACCOUNT TEXT NOT NULL, TIME TEXT NOT NULL);");
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK){
        fprintf(stderr,"create scenario_table fail: %s\n",errmsg);
    }
    sqlite3_free(errmsg);
    /*project_table*/
    sprintf(sql,"CREATE TABLE project_table(ID INT PRIMARY KEY NOT NULL, P_NAME TEXT NOT NULL, P_NUMBER TEXT NOT NULL, P_CREATOR TEXT NOT NULL, P_MANAGER TEXT NOT NULL, P_EDITOR TEXT NOT NULL, P_TEL TEXT NOT NULL, P_ADD TEXT NOT NULL, P_BRIEF TEXT NOT NULL, P_KEY TEXT NOT NULL, ACCOUNT TEXT NOT NULL, TIME TEXT NOT NULL);");
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK){
        fprintf(stderr,"create project_table fail: %s\n",errmsg);
    }
    sqlite3_free(errmsg);
    /*插入Dalitek账户*/
    sprintf(sql,"insert into account_table(ID, ACCOUNT, KEY, KEY_AUTH, REMOTE_AUTH, TIME)values(1,\"Dalitek\",\"root\",\"on\",\"on\",\"20171023110000\");");
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK){
        fprintf(stderr,"insert into account_table fail: %s\n",errmsg);
    }
    sqlite3_free(errmsg);
    /*插入项目信息*/
    sprintf(sql,"insert into project_table(ID, P_NAME, P_NUMBER, P_CREATOR, P_MANAGER, P_EDITOR, P_TEL, P_ADD, P_BRIEF, P_KEY, ACCOUNT, TIME)values(1,\"M1\",\"00000001\",\"Dalitek\",\"Dalitek\",\"Dalitek\",\"123456789\",\"ShangHai\",\"Brief\",\"123456\",\"Dalitek\",\"20171031161900\");");
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK){
        fprintf(stderr,"insert into project_table fail: %s\n",errmsg);
    }
    sqlite3_free(errmsg);

    Finish:
    sqlite3_close(db);

    return ret;
}



