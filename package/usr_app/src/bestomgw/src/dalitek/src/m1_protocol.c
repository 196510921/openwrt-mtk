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
#include "sql_backup.h"
#include "m1_common_log.h"
#include "m1_device.h"

/*Macro**********************************************************************************************************/
#define SQL_HISTORY_DEL      1
#define M1_PROTOCOL_DEBUG    1
#define HEAD_LEN             3
#define AP_HEARTBEAT_HANDLE  1
/*Private function***********************************************************************************************/
static int AP_report_data_handle(payload_t data);
static int APP_read_handle(payload_t data);
static int APP_write_handle(payload_t data);
static int M1_write_to_AP(cJSON* data, sqlite3* db);
static int APP_echo_dev_info_handle(payload_t data);
static int APP_req_added_dev_info_handle(payload_t data);
static int APP_net_control(payload_t data);
static int M1_report_dev_info(payload_t data);
static int M1_report_ap_info(payload_t data);
static int AP_report_dev_handle(payload_t data);
static int AP_report_ap_handle(payload_t data);
static int common_operate(payload_t data);
static int common_rsp(rsp_data_t data);
static int ap_heartbeat_handle(payload_t data);
static int common_rsp_handle(payload_t data);
static int create_sql_table(void);
static int app_change_device_name(payload_t data);
static uint8_t hex_to_uint8(int h);
static void check_offline_dev(sqlite3*db);
static void delete_client_db(void);
/*variable******************************************************************************************************/
extern pthread_mutex_t mutex_lock;
sqlite3* db = NULL;
// const char* db_path = "/bin/dev_info.db";
// const char* sql_back_path = "/bin/sql_restore.sh";
const char* db_path = "dev_info.db";
const char* sql_back_path = "sql_restore.sh";
fifo_t dev_data_fifo;
fifo_t link_exec_fifo;
fifo_t msg_rd_fifo;
fifo_t msg_wt_fifo;
fifo_t client_delete_fifo;
fifo_t tx_fifo;
/*优先级队列*/
PNode head;
static uint32_t dev_data_buf[256];
static uint32_t link_exec_buf[256];
static uint32_t client_delete_buf[10];

void m1_protocol_init(void)
{
    create_sql_table();
    M1_LOG_DEBUG("threadsafe:%d\n",sqlite3_threadsafe());
    fifo_init(&dev_data_fifo, dev_data_buf, 256);
    /*linkage execution fifo*/
    fifo_init(&link_exec_fifo, link_exec_buf, 256);
    /*delete cleint*/
    fifo_init(&client_delete_fifo, client_delete_buf, 10);

    Init_PQueue(&head);
    /*初始化接收buf*/
    client_block_init();
}

void data_handle(m1_package_t* package)
{
    M1_LOG_DEBUG("data_handle\n");
    int rc, ret;
    int pduType;
    uint32_t* msg = NULL;
    cJSON* rootJson = NULL;
    cJSON* pduJson = NULL;
    cJSON* pduTypeJson = NULL;
    cJSON* snJson = NULL;
    cJSON* pduDataJson = NULL;

    payload_t pdu;
    rsp_data_t rspData;

    rc = M1_PROTOCOL_NO_RSP;
    M1_LOG_DEBUG("Rx message:%s\n",package->data);
    rootJson = cJSON_Parse(package->data);
    if(NULL == rootJson){
        M1_LOG_ERROR("rootJson null\n");
        goto Finish;   
    }
    pduJson = cJSON_GetObjectItem(rootJson, "pdu");
    if(NULL == pduJson){
        M1_LOG_ERROR("pdu null\n");
        goto Finish;
    }
    pduTypeJson = cJSON_GetObjectItem(pduJson, "pduType");
    if(NULL == pduTypeJson){
        M1_LOG_ERROR("pduType null\n");
        goto Finish;
    }
    pduType = pduTypeJson->valueint;
    rspData.pduType = pduType;

    snJson = cJSON_GetObjectItem(rootJson, "sn");
    if(NULL == snJson){
        M1_LOG_ERROR("sn null\n");
        goto Finish;
    }
    rspData.sn = snJson->valueint;

    pduDataJson = cJSON_GetObjectItem(pduJson, "devData");
    if(NULL == pduDataJson){
        M1_LOG_INFO("devData null”\n");
    }

    /*打开读数据库*/
    rc = sql_open();
    if( rc != SQLITE_OK){  
        M1_LOG_ERROR( "Can't open database\n");  
        goto Finish;
    }else{  
        M1_LOG_DEBUG( "Opened database successfully\n");  
    }

    /*pdu*/ 
    pdu.clientFd = package->clientFd;
    pdu.sn = snJson->valueint;
    pdu.db = db;
    pdu.pdu = pduDataJson;

    rspData.clientFd = package->clientFd;
    M1_LOG_DEBUG("pduType:%x\n",pduType);

    switch(pduType){
        case TYPE_DEV_READ: APP_read_handle(pdu); break;
        case TYPE_REQ_ADDED_INFO: APP_req_added_dev_info_handle(pdu); break;
        case TYPE_DEV_NET_CONTROL: rc = APP_net_control(pdu); break;
        case TYPE_REQ_AP_INFO: M1_report_ap_info(pdu); break;
        case TYPE_REQ_DEV_INFO: M1_report_dev_info(pdu); break;
        case TYPE_COMMON_RSP: common_rsp_handle(pdu);rc = M1_PROTOCOL_NO_RSP;break;
        case TYPE_REQ_SCEN_INFO: rc = app_req_scenario(pdu);break;
        case TYPE_REQ_LINK_INFO: rc = app_req_linkage(pdu);break;
        case TYPE_REQ_DISTRICT_INFO: rc = app_req_district(pdu); break;
        case TYPE_REQ_SCEN_NAME_INFO: rc = app_req_scenario_name(pdu);break;
        case TYPE_REQ_ACCOUNT_INFO: rc = app_req_account_info_handle(pdu);break;
        case TYPE_REQ_ACCOUNT_CONFIG_INFO: rc = app_req_account_config_handle(pdu);break;
        case TYPE_GET_PORJECT_NUMBER: rc = app_get_project_info(pdu); break;
        case TYPE_REQ_DIS_SCEN_NAME: rc = app_req_dis_scen_name(pdu); break;
        case TYPE_REQ_DIS_NAME: rc = app_req_dis_name(pdu); break;
        case TYPE_REQ_DIS_DEV: rc = app_req_dis_dev(pdu); break;
        case TYPE_GET_PROJECT_INFO: rc = app_get_project_config(pdu);break;
        case TYPE_APP_CONFIRM_PROJECT: rc = app_confirm_project(pdu);break;
        case TYPE_APP_EXEC_SCEN: rc = app_exec_scenario(pdu);break;
        case TYPE_DEBUG_INFO: debug_switch(pduDataJson->valuestring);break;
        /*write*/
        case TYPE_REPORT_DATA: rc = AP_report_data_handle(pdu); break;
        case TYPE_DEV_WRITE: rc = M1_write_to_AP(rootJson, db);/*APP_write_handle(pdu);*/break;
        case TYPE_ECHO_DEV_INFO: rc = APP_echo_dev_info_handle(pdu); break;
        case TYPE_AP_REPORT_DEV_INFO: rc = AP_report_dev_handle(pdu); break;
        case TYPE_AP_REPORT_AP_INFO: rc = AP_report_ap_handle(pdu); break;
        case TYPE_CREATE_LINKAGE: rc = linkage_msg_handle(pdu);break;
        case TYPE_CREATE_SCENARIO: rc = scenario_create_handle(pdu);break;
        case TYPE_CREATE_DISTRICT: rc = district_create_handle(pdu);break;
        case TYPE_SCENARIO_ALARM: rc = scenario_alarm_create_handle(pdu);break;
        case TYPE_COMMON_OPERATE: rc = common_operate(pdu);break;
        case TYPE_AP_HEARTBEAT_INFO: rc = ap_heartbeat_handle(pdu);break;
        case TYPE_LINK_ENABLE_SET: rc = app_linkage_enable(pdu);break;
        case TYPE_APP_LOGIN: rc = user_login_handle(pdu);break;
        case TYPE_SEND_ACCOUNT_CONFIG_INFO: rc = app_account_config_handle(pdu);break;
        case TYPE_APP_CREATE_PROJECT: rc = app_create_project(pdu);break;
        case TYPE_PROJECT_KEY_CHANGE: rc = app_change_project_key(pdu);break;
        case TYPE_PROJECT_INFO_CHANGE:rc = app_change_project_config(pdu);break;
        case TYPE_APP_CHANGE_DEV_NAME: rc = app_change_device_name(pdu);break;
        case TYPE_APP_USER_KEY_CHANGE: rc = app_change_user_key(pdu);break;
        case TYPE_APP_DOWNLOAD_TESTING_INFO: rc = app_download_testing_to_ap(rootJson,db); break;
        case TYPE_AP_UPLOAD_TESTING_INFO: rc = ap_upload_testing_to_app(rootJson,db);break;

        default: M1_LOG_ERROR("pdu type not match\n"); rc = M1_PROTOCOL_FAILED;break;
    }

    if(rc != M1_PROTOCOL_NO_RSP){
        if(rc == M1_PROTOCOL_OK)
            rspData.result = RSP_OK;
        else
            rspData.result = RSP_FAILED;
        common_rsp(rspData);
    }
    check_offline_dev(db);

    //delete_client_db();

    sql_close();

    Finish:
    cJSON_Delete(rootJson);
    linkage_task();
}

static int common_rsp_handle(payload_t data)
{
    cJSON* resultJson = NULL;
    M1_LOG_DEBUG("common_rsp_handle\n");
    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;

    resultJson = cJSON_GetObjectItem(data.pdu, "result");
    M1_LOG_DEBUG("result:%d\n",resultJson->valueint);
}

/*AP report device data to M1*/
static int AP_report_data_handle(payload_t data)
{
    int i                = 0;
    int j                = 0;
    int number1          = 0;
    int number2          = 0;
    int rc               = 0;
    int ret              = M1_PROTOCOL_OK;
    char* errorMsg       = NULL;
    char* sql            = NULL;
    /*sqlite3*/
    sqlite3* db          = NULL;
    sqlite3_stmt* stmt   = NULL;
    /*Json*/
    cJSON* devDataJson   = NULL;
    cJSON* devNameJson   = NULL;
    cJSON* devIdJson     = NULL;
    cJSON* paramJson     = NULL;
    cJSON* paramDataJson = NULL;
    cJSON* typeJson      = NULL;
    cJSON* valueJson     = NULL;

    db = data.db;   
    M1_LOG_DEBUG("AP_report_data_handle\n");
    if(data.pdu == NULL)
    {
        ret = M1_PROTOCOL_FAILED;
        goto Finish;   
    }
    /*关闭写同步*/
    // if(sqlite3_exec(db,"PRAGMA synchronous = OFF; ",0,0,0) != SQLITE_OK)
    // {
    //     M1_LOG_ERROR("PRAGMA synchronous = OFF falied\n");
    //     ret = M1_PROTOCOL_FAILED;
    //     goto Finish;
    // }

    /*添加update/insert/delete监察*/
    rc = sqlite3_update_hook(db, trigger_cb, "AP_report_data_handle");
    if(rc)
    {
        M1_LOG_ERROR( "sqlite3_update_hook falied: %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*开启事物*/
    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        M1_LOG_DEBUG("BEGIN IMMEDIATE\n");

        sql = "insert or replace into param_table(DEV_NAME,DEV_ID,TYPE,VALUE)values(?,?,?,?);";
        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));   
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }

        number1 = cJSON_GetArraySize(data.pdu);
        M1_LOG_DEBUG("number1:%d\n",number1);
        for(i = 0; i < number1; i++)
        {
            devDataJson = cJSON_GetArrayItem(data.pdu, i);
            if(devDataJson == NULL)
            {
                M1_LOG_WARN("devDataJson NULL\n");
                ret =  M1_PROTOCOL_FAILED;  
                goto Finish;    
            }
            devNameJson = cJSON_GetObjectItem(devDataJson, "devName");
            if(devNameJson == NULL)
            {
                M1_LOG_WARN("devNameJson NULL\n");
                ret =  M1_PROTOCOL_FAILED;  
                goto Finish;    
            }
            M1_LOG_DEBUG("devName:%s\n",devNameJson->valuestring);
            devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
            if(devIdJson == NULL)
            {
                M1_LOG_WARN("devIdJson NULL\n");
                ret =  M1_PROTOCOL_FAILED;  
                goto Finish;    
            }
            M1_LOG_DEBUG("devId:%s\n",devIdJson->valuestring);
            paramJson = cJSON_GetObjectItem(devDataJson, "param");
            if(paramJson == NULL)
            {
                M1_LOG_WARN("paramJson NULL\n");
                ret =  M1_PROTOCOL_FAILED;  
                goto Finish;    
            }
            number2 = cJSON_GetArraySize(paramJson);
            M1_LOG_DEBUG(" number2:%d\n",number2);
            for(j = 0; j < number2; j++)
            {
                paramDataJson = cJSON_GetArrayItem(paramJson, j);
                if(paramDataJson == NULL)
                {
                    M1_LOG_WARN("paramDataJson NULL\n");
                    ret =  M1_PROTOCOL_FAILED;  
                    goto Finish;    
                }   
                typeJson = cJSON_GetObjectItem(paramDataJson, "type");
                if(typeJson == NULL)
                {
                    M1_LOG_WARN("typeJson NULL\n");
                    ret =  M1_PROTOCOL_FAILED;  
                    goto Finish;    
                }
                M1_LOG_DEBUG("  type:%d\n",typeJson->valueint);
                valueJson = cJSON_GetObjectItem(paramDataJson, "value");
                if(valueJson == NULL)
                {
                    M1_LOG_WARN("valueJson NULL\n");
                    ret =  M1_PROTOCOL_FAILED;  
                    goto Finish;    
                }
                M1_LOG_DEBUG("  value:%d\n",valueJson->valueint);
                if(typeJson->valueint == 16404)
                {
                    M1_LOG_DEBUG("type:%05d,value:%d,dev_id:%s\n",typeJson->valueint,\
                                                                valueJson->valueint,\
                                                                devIdJson->valuestring);    
                }
                M1_LOG_DEBUG("AP data insert\n");
                
                /*再插入*/
                sqlite3_bind_text(stmt, 1,  devNameJson->valuestring, -1, NULL);
                sqlite3_bind_text(stmt, 2, devIdJson->valuestring, -1, NULL);
                sqlite3_bind_int(stmt, 3,typeJson->valueint);
                sqlite3_bind_int(stmt, 4, valueJson->valueint);
                rc = sqlite3_step(stmt);   
                M1_LOG_DEBUG("step() return %s, number:%03d\n", \
                    rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
                
                if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)){
                    M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
                }
                sqlite3_reset(stmt); 
                sqlite3_clear_bindings(stmt);

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
    if(stmt != NULL)
        sqlite3_finalize(stmt);

    trigger_cb_handle(db);
    return ret;
}

/*AP report device information to M1*/
static int AP_report_dev_handle(payload_t data)
{
    int i                = 0;
    int number           = 0;
    int rc               = 0;
    int ret              = M1_PROTOCOL_OK;
    /*sql*/
    char *errorMsg       = NULL;
    char *sql            = NULL;
    sqlite3 *db          = NULL;
    sqlite3_stmt *stmt   = NULL;
    /*Json*/
    cJSON *apIdJson      = NULL;
    cJSON *apNameJson    = NULL;
    cJSON *pIdJson       = NULL;
    cJSON *devJson       = NULL;
    cJSON *paramDataJson = NULL;    
    cJSON *idJson        = NULL;    
    cJSON *nameJson      = NULL;   

    M1_LOG_DEBUG("AP_report_dev_handle\n");
    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    } 
    /*获取数据库*/
    db = data.db;
    /*关闭写同步*/
    // if(sqlite3_exec(db,"PRAGMA synchronous = OFF; ",0,0,0) != SQLITE_OK){
    //     M1_LOG_ERROR("PRAGMA synchronous = OFF falied\n");
    //     ret = M1_PROTOCOL_FAILED;
    //     goto Finish;
    // }

    apIdJson = cJSON_GetObjectItem(data.pdu,"apId");
    M1_LOG_DEBUG("APId:%s\n",apIdJson->valuestring);
    apNameJson = cJSON_GetObjectItem(data.pdu,"apName");
    M1_LOG_DEBUG("APName:%s\n",apNameJson->valuestring);
    devJson = cJSON_GetObjectItem(data.pdu,"dev");
    number = cJSON_GetArraySize(devJson); 

    /*事物开始*/
    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        M1_LOG_DEBUG("BEGIN IMMEDIATE\n");
        sql =   "insert or replace into all_dev(DEV_NAME, DEV_ID, AP_ID, PID, ADDED, NET, STATUS, ACCOUNT)\
                values(?,?,?,?,?,?,?,?);";
        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }

        for(i = 0; i< number; i++){
            paramDataJson = cJSON_GetArrayItem(devJson, i);
            if(paramDataJson == NULL)
            {
                M1_LOG_WARN("paramDataJson NULL");
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            idJson = cJSON_GetObjectItem(paramDataJson, "devId");
            if(idJson == NULL)
            {
                M1_LOG_WARN("idJson NULL");
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            M1_LOG_DEBUG("devId:%s\n", idJson->valuestring);
            nameJson = cJSON_GetObjectItem(paramDataJson, "devName");
            if(nameJson == NULL)
            {
                M1_LOG_WARN("nameJson NULL");
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            M1_LOG_DEBUG("devName:%s\n", nameJson->valuestring);
            pIdJson = cJSON_GetObjectItem(paramDataJson, "pId");
            if(pIdJson == NULL)
            {
                M1_LOG_WARN("pIdJson NULL\n");
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            M1_LOG_DEBUG("pId:%05d\n", pIdJson->valueint);
           
            sqlite3_bind_text(stmt, 1,  nameJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt, 2, idJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt, 3,apIdJson->valuestring, -1, NULL);
            sqlite3_bind_int(stmt, 4, pIdJson->valueint);
            sqlite3_bind_int(stmt, 5, 0);
            sqlite3_bind_int(stmt, 6, 1);
            sqlite3_bind_text(stmt, 7,"ON", -1, NULL);
            sqlite3_bind_text(stmt, 8,  "Dalitek", -1, NULL);

            rc = sqlite3_step(stmt);   
            M1_LOG_DEBUG("step() return %s, number:%03d\n",\
                rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
            
            if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)){
                M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            }

            sqlite3_reset(stmt); 
            sqlite3_clear_bindings(stmt);
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
    if(stmt != NULL)
        sqlite3_finalize(stmt);

    return ret;  
}

/*AP report information to M1*/
static int AP_report_ap_handle(payload_t data)
{
    int rc             = 0;
    int ret            = M1_PROTOCOL_OK;
    char* sql          = NULL; 
    /*sql*/
    char* errorMsg     = NULL;
    sqlite3* db        = NULL;
    sqlite3_stmt* stmt = NULL;
    /*Json*/
    cJSON* pIdJson     = NULL;
    cJSON* apIdJson    = NULL;
    cJSON* apNameJson  = NULL;

    M1_LOG_DEBUG("AP_report_ap_handle\n");
    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;  
        goto Finish;
    }

    /*获取数据库*/
    db = data.db;

    pIdJson = cJSON_GetObjectItem(data.pdu,"pId");
    if(pIdJson == NULL)
    {
        M1_LOG_WARN("pIdJson NULL\n");
        ret = M1_PROTOCOL_FAILED;  
        goto Finish;
    }
    M1_LOG_DEBUG("pId:%05d\n",pIdJson->valueint);
    apIdJson = cJSON_GetObjectItem(data.pdu,"apId");
    if(apIdJson == NULL)
    {
        M1_LOG_WARN("apIdJson NULL\n");
        ret = M1_PROTOCOL_FAILED;  
        goto Finish;
    }
    M1_LOG_DEBUG("APId:%s\n",apIdJson->valuestring);
    apNameJson = cJSON_GetObjectItem(data.pdu,"apName");
    if(apNameJson == NULL)
    {
        M1_LOG_WARN("apNameJson NULL\n");
        ret = M1_PROTOCOL_FAILED;  
        goto Finish;
    }
    M1_LOG_DEBUG("APName:%s\n",apNameJson->valuestring);
    
    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        /*更新conn_info表信息*/
        sql = "insert or replace into conn_info(AP_ID, CLIENT_FD) values(?,?)";
        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        
        sqlite3_bind_text(stmt, 1, apIdJson->valuestring, -1, NULL);
        sqlite3_bind_int(stmt, 2, data.clientFd);

        rc = sqlite3_step(stmt);   
        M1_LOG_DEBUG("step() return %s, number:%03d\n",\
            rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
            
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        }

        if(stmt != NULL)
        {
            sqlite3_finalize(stmt);
            stmt = NULL;
        }
        /*更新all_dev表信息*/
 
        sql = "insert or replace into all_dev(DEV_NAME, DEV_ID, AP_ID, PID, ADDED, NET, STATUS, ACCOUNT)\
              values(?,?,?,?,?,?,?,?);";
        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        
        sqlite3_bind_text(stmt, 1,  apNameJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 2, apIdJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 3, apIdJson->valuestring, -1, NULL);
        sqlite3_bind_int(stmt, 4, pIdJson->valueint);
        sqlite3_bind_int(stmt, 5, 0);
        sqlite3_bind_int(stmt, 6, 1);
        sqlite3_bind_text(stmt, 7,  "ON", -1, NULL);
        sqlite3_bind_text(stmt, 8,  "Dalitek", -1, NULL);
    
        rc = sqlite3_step(stmt);   
        M1_LOG_DEBUG("step() return %s, number:%03d\n",\
            rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
            
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        }

        if(stmt != NULL)
        {
            sqlite3_finalize(stmt);
            stmt = NULL;
        }

        /*更新param_table表信息*/
        sql = "insert or replace into param_table(DEV_ID, DEV_NAME, TYPE, VALUE)values(?,?,?,?);";
        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        
        sqlite3_bind_text(stmt, 1,  apIdJson->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 2,  apNameJson->valuestring, -1, NULL);
        sqlite3_bind_int(stmt, 3, 16404);
        sqlite3_bind_int(stmt, 4, 1);
    
        rc = sqlite3_step(stmt);   
        M1_LOG_DEBUG("step() return %s, number:%03d\n",\
            rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
            
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        }

        if(stmt != NULL)
        {
            sqlite3_finalize(stmt);
            stmt = NULL;
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
    
    if(stmt != NULL)
        sqlite3_finalize(stmt);

    return ret;  
}

static int APP_read_handle(payload_t data)
{   
    int i                    = 0;
    int j                    = 0;
    int number1              = 0;
    int number2              = 0;
    int row_n                = 0;
    int pduType              = TYPE_REPORT_DATA;
    int rc                   = 0;
    int ret                  = M1_PROTOCOL_OK;
    char *devName            = NULL;
    char *pId                = NULL;
    int value                = 0;
    int clientFd             = 0;
    char *dev_id             = NULL;
    /*Json*/
    cJSON *devDataJson       = NULL;
    cJSON *devIdJson         = NULL;
    cJSON *paramTypeJson     = NULL;
    cJSON *paramJson         = NULL;
    cJSON *pJsonRoot         = NULL; 
    cJSON *pduJsonObject     = NULL;
    cJSON *devDataJsonArray  = NULL;
    cJSON *devDataObject     = NULL;
    cJSON *devArray          = NULL;
    cJSON *devObject         = NULL;
    /*sql*/
    char* sql                = NULL;
    char* sql_1              = NULL;
    sqlite3* db              = NULL;
    sqlite3_stmt* stmt       = NULL;
    sqlite3_stmt* stmt_1     = NULL;

    M1_LOG_DEBUG("APP_read_handle\n");
    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    clientFd = data.clientFd;
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
    number1 = cJSON_GetArraySize(data.pdu);
    M1_LOG_DEBUG("number1:%d\n",number1);

    {
        sql = "select DEV_NAME,PID from all_dev where DEV_ID  = ? order by ID desc limit 1;";
        M1_LOG_DEBUG("%s\n", sql);

        /*取出devName,pid*/
        if(sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL) != SQLITE_OK){
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
    }

    {
        sql_1 = "select VALUE from param_table where DEV_ID = ? and TYPE = ? order by ID desc limit 1;";
        M1_LOG_DEBUG("%s\n", sql_1);
    
        if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1),&stmt_1, NULL) != SQLITE_OK){
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
    }

    for(i = 0; i < number1; i++)
    {
        /*read json*/
        devDataJson = cJSON_GetArrayItem(data.pdu, i);
        devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
        dev_id = devIdJson->valuestring;
        paramTypeJson = cJSON_GetObjectItem(devDataJson, "paramType");
        number2 = cJSON_GetArraySize(paramTypeJson);
        M1_LOG_DEBUG("devId:%s\n",devIdJson->valuestring);

        {
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
            cJSON_AddStringToObject(devDataObject, "devId", dev_id);
        }

        sqlite3_bind_text(stmt, 1, dev_id, -1, NULL);

        rc = sqlite3_step(stmt);
        if(rc == SQLITE_ROW)
        {
            /*devName*/
            devName = sqlite3_column_text(stmt,0);
            if(devName == NULL)
            {
                M1_LOG_WARN("devName NULL\n");
                ret = M1_PROTOCOL_FAILED;
                goto Finish;       
            }
            cJSON_AddStringToObject(devDataObject, "devName", devName);
            /*pid*/
            pId = sqlite3_column_text(stmt,1);
            if(pId == NULL)
            {
                M1_LOG_WARN("pId NULL\n");
                ret = M1_PROTOCOL_FAILED;
                goto Finish;   
            }
            cJSON_AddStringToObject(devDataObject, "pId", pId);
        }
        else
        {
            M1_LOG_WARN("devName,pid not exit");
            sqlite3_reset(stmt);   
            sqlite3_clear_bindings(stmt);
            continue;
        }

        sqlite3_reset(stmt);   
        sqlite3_clear_bindings(stmt);

        devArray = cJSON_CreateArray();
        if(NULL == devArray)
        {
            M1_LOG_ERROR("devArry NULL\n");
            cJSON_Delete(devArray);
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }
        /*add devData array to pdu pbject*/
        cJSON_AddItemToObject(devDataObject, "param", devArray);
    
        for(j = 0; j < number2; j++)
        {
            /*read json*/
            paramJson = cJSON_GetArrayItem(paramTypeJson, j);
            devObject = cJSON_CreateObject();
            if(NULL == devObject)
            {
                M1_LOG_ERROR("devObject NULL\n");
                cJSON_Delete(devObject);
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            cJSON_AddItemToArray(devArray, devObject); 

            sqlite3_bind_text(stmt_1, 1, dev_id, -1, NULL);
            sqlite3_bind_int(stmt_1, 2, paramJson->valueint);
            
            rc = sqlite3_step(stmt_1);
            if(rc == SQLITE_ROW)
            {
                /*date value*/
                value = sqlite3_column_int(stmt_1,0);
                cJSON_AddNumberToObject(devObject, "type", paramJson->valueint);
                cJSON_AddNumberToObject(devObject, "value", value);
            }
            else
            {
                M1_LOG_WARN("date value not exit");
                sqlite3_reset(stmt_1);   
                sqlite3_clear_bindings(stmt_1);
                continue;
            }
            sqlite3_reset(stmt_1);   
            sqlite3_clear_bindings(stmt_1);
        }
    }

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);
    socketSeverSend((uint8*)p, strlen(p), clientFd);

    Finish:
    if(stmt != NULL)
        sqlite3_finalize(stmt);
    if(stmt_1 != NULL)
        sqlite3_finalize(stmt_1);

    cJSON_Delete(pJsonRoot);
    return ret;
}

static int M1_write_to_AP(cJSON* data, sqlite3* db)
{
    M1_LOG_DEBUG("M1_write_to_AP\n");
    int sn               = 2;
    int clientFd         = 0;
    int rc               = 0;
    int ret              = M1_PROTOCOL_OK;
    /*sql*/
    char *sql            = NULL;
    sqlite3_stmt *stmt   = NULL;
    /*Json*/
    cJSON* snJson        = NULL;
    cJSON* pduJson       = NULL;
    cJSON* devDataJson   = NULL;
    cJSON* dataArrayJson = NULL;
    cJSON* devIdJson     = NULL;
    
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
    devDataJson   = cJSON_GetObjectItem(pduJson, "devData");
    dataArrayJson = cJSON_GetArrayItem(devDataJson, 0);
    devIdJson     = cJSON_GetObjectItem(dataArrayJson, "devId");
    M1_LOG_DEBUG("devId:%s\n",devIdJson->valuestring);

    sql = "select CLIENT_FD from conn_info where AP_ID in \
          (select AP_ID from all_dev where DEV_ID = ? order by ID desc limit 1) \
          order by ID desc limit 1;";
    M1_LOG_DEBUG("%s\n", sql);
    if(sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    sqlite3_bind_text(stmt, 1, devIdJson->valuestring, -1, NULL);
    rc = sqlite3_step(stmt);
    if(rc == SQLITE_ROW)
    {
        clientFd = sqlite3_column_int(stmt,0);
        char * p = cJSON_PrintUnformatted(data);
        if(NULL == p)
        {    
            cJSON_Delete(data);
            ret = M1_PROTOCOL_FAILED;
            goto Finish;  
        }

        M1_LOG_DEBUG("string:%s\n",p);
        /*response to client*/
        socketSeverSend((uint8*)p, strlen(p), clientFd);
    }   

    Finish:
    if(stmt != NULL)
        sqlite3_finalize(stmt);

    return ret;
}

static int APP_write_handle(payload_t data)
{
    int i                 = 0;
    int j                 = 0;
    int number1           = 0;
    int number2           = 0;
    int rc                = 0;
    int ret               = M1_PROTOCOL_OK;
    /*sql*/
    char* errorMsg        = NULL;
    char* sql             = NULL;
    sqlite3* db           = NULL;
    sqlite3_stmt* stmt    = NULL;
    /*Json*/
    cJSON* devDataJson    = NULL;
    cJSON* devIdJson      = NULL;
    cJSON* paramDataJson  = NULL;
    cJSON* paramArrayJson = NULL;
    cJSON* valueTypeJson  = NULL;
    cJSON* valueJson = NULL;

    M1_LOG_DEBUG("APP_write_handle\n");
    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    };

    /*获取数据库*/
    db = data.db;
    /*关闭写同步*/
    // if(sqlite3_exec(db,"PRAGMA synchronous = OFF;",NULL,NULL,&errorMsg) != SQLITE_OK){
    //     M1_LOG_ERROR("PRAGMA synchronous = OFF falied:%s\n",errorMsg);
    //     ret = M1_PROTOCOL_FAILED;
    //     goto Finish;  
    // }

    sql = "insert or replace into param_table(DEV_NAME,DEV_ID,TYPE,VALUE) \
               values((select DEV_NAME from all_dev where DEV_ID = ? order by ID desc limit 1),?,?,?);";
    M1_LOG_DEBUG("%s\n", sql);

    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        M1_LOG_DEBUG("BEGIN IMMEDIATE\n");
        number1 = cJSON_GetArraySize(data.pdu);
        M1_LOG_DEBUG("number1:%d\n",number1);
        for(i = 0; i < number1; i++)
        {
            devDataJson = cJSON_GetArrayItem(data.pdu, i);
            if(devDataJson == NULL)
            {
                M1_LOG_WARN("devDataJson NULL \n");
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
            if(devIdJson == NULL)
            {
                M1_LOG_WARN("devIdJson NULL \n");
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            M1_LOG_DEBUG("devId:%s\n",devIdJson->valuestring);

            paramDataJson = cJSON_GetObjectItem(devDataJson, "param");
            if(paramDataJson == NULL)
            {
                M1_LOG_WARN("paramDataJson NULL \n");
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            number2 = cJSON_GetArraySize(paramDataJson);
            M1_LOG_DEBUG("number2:%d\n",number2);
            
            for(j = 0; j < number2; j++)
            {
                paramArrayJson = cJSON_GetArrayItem(paramDataJson, j);
                if(paramArrayJson == NULL)
                {
                    M1_LOG_WARN("paramArrayJson NULL \n");
                    ret = M1_PROTOCOL_FAILED;
                    goto Finish;
                }
                valueTypeJson = cJSON_GetObjectItem(paramArrayJson, "type");
                if(valueTypeJson == NULL)
                {
                    M1_LOG_WARN("valueTypeJson NULL \n");
                    ret = M1_PROTOCOL_FAILED;
                    goto Finish;
                }
                M1_LOG_DEBUG("  type%d:%d\n",j,valueTypeJson->valueint);
                valueJson = cJSON_GetObjectItem(paramArrayJson, "value");
                if(valueJson == NULL)
                {
                    M1_LOG_WARN("valueJson NULL \n");
                    ret = M1_PROTOCOL_FAILED;
                    goto Finish;
                }
                M1_LOG_DEBUG("  value%d:%d\n",j,valueJson->valueint);

                sqlite3_bind_text(stmt, 1,  devIdJson->valuestring, -1, NULL);
                sqlite3_bind_text(stmt, 2, devIdJson->valuestring, -1, NULL);
                sqlite3_bind_int(stmt, 3, valueTypeJson->valueint);
                sqlite3_bind_int(stmt, 4, valueJson->valueint);

                rc = sqlite3_step(stmt);   
                M1_LOG_DEBUG("step() return %s, number:%03d\n",\
                    rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
            
                if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
                {
                    M1_LOG_WARN("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
                }

                sqlite3_reset(stmt);   
                sqlite3_clear_bindings(stmt);
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
    if(stmt != NULL)
        sqlite3_finalize(stmt);

    return ret;
}

static int APP_echo_dev_info_handle(payload_t data)
{
    int i                   = 0;
    int j                   = 0;
    int number              = 0;
    int number_1            = 0;
    int rc                  = 0;
    int ret                 = M1_PROTOCOL_OK;
    char* sql               = NULL;
    char* errorMsg          = NULL;
    /*sqlite3*/
    sqlite3* db             = NULL;
    sqlite3_stmt* stmt      = NULL;
    /*Json*/
    cJSON* devDataJson      = NULL;
    cJSON* devdataArrayJson = NULL;
    cJSON* devArrayJson     = NULL;
    cJSON* APIdJson         = NULL;

    M1_LOG_DEBUG("APP_echo_dev_info_handle\n");
    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    } 

    db = data.db;

    sql = "update all_dev set ADDED = ?, STATUS = ? where DEV_ID = ? and AP_ID = ?;";
    M1_LOG_DEBUG("sql:%s\n",sql);  
    
    if(sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        number = cJSON_GetArraySize(data.pdu);
        M1_LOG_DEBUG("number:%d\n",number);  
        for(i = 0; i < number; i++)
        {
            devdataArrayJson = cJSON_GetArrayItem(data.pdu, i);
            if(devdataArrayJson == NULL)
            {
                M1_LOG_ERROR("devdataArrayJson NULL\n");
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            APIdJson = cJSON_GetObjectItem(devdataArrayJson, "apId");
            if(APIdJson == NULL)
            {
                M1_LOG_ERROR("APIdJson NULL\n");
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            M1_LOG_DEBUG("AP_ID:%s\n",APIdJson->valuestring);\

            sqlite3_bind_int(stmt, 0, 1);
            sqlite3_bind_text(stmt, 1, "ON", -1, NULL);
            sqlite3_bind_text(stmt, 2, APIdJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt, 3, APIdJson->valuestring, -1, NULL);

            rc = sqlite3_step(stmt);   
            M1_LOG_DEBUG("step() return %s, number:%03d\n",\
                rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
            
            if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                M1_LOG_WARN("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            }

            sqlite3_reset(stmt);   
            sqlite3_clear_bindings(stmt);

            devDataJson = cJSON_GetObjectItem(devdataArrayJson,"devId");
            if(devDataJson != NULL)
            {
                number_1 = cJSON_GetArraySize(devDataJson);
                M1_LOG_DEBUG("number_1:%d\n",number_1);
                for(j = 0; j < number_1; j++)
                {
                    devArrayJson = cJSON_GetArrayItem(devDataJson, j);
                    M1_LOG_DEBUG("  devId:%s\n",devArrayJson->valuestring);

                    sqlite3_bind_int(stmt, 0, 1);
                    sqlite3_bind_text(stmt, 1, "ON", -1, NULL);
                    sqlite3_bind_text(stmt, 2, devArrayJson->valuestring, -1, NULL);
                    sqlite3_bind_text(stmt, 3, APIdJson->valuestring, -1, NULL);
                    rc = sqlite3_step(stmt);   
                    M1_LOG_DEBUG("step() return %s, number:%03d\n",\
                        rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
            
                    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
                    {
                        M1_LOG_WARN("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
                    }

                    sqlite3_reset(stmt);   
                    sqlite3_clear_bindings(stmt);
                }
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
    if(stmt != NULL)
        sqlite3_finalize(stmt);

    return ret;
}

static int APP_req_added_dev_info_handle(payload_t data)
{
    int rc                  = 0;
    int ret                 = M1_PROTOCOL_OK;
    int pduType             = TYPE_M1_REPORT_ADDED_INFO;
    /*rx data*/
    const char *account     = NULL;
    /*tx data*/
    const char *apId        = NULL;
    const char *apName      = NULL;
    const char *devId       = NULL;
    const char *devName     = NULL;
    int pId                 = 0;
    /*sql*/
    char *sql               = NULL;
    char *sql_1             = NULL;
    char *sql_2             = NULL;
    sqlite3 *db             = NULL;
    sqlite3_stmt *stmt      = NULL;
    sqlite3_stmt *stmt_1    = NULL;
    sqlite3_stmt *stmt_2    = NULL;
    /*Json*/
    cJSON *devDataObject    = NULL;
    cJSON *devArray         = NULL;
    cJSON *devObject        = NULL;
    cJSON *pJsonRoot        = NULL;
    cJSON *devDataJsonArray = NULL;
    cJSON *pduJsonObject    = NULL;

    M1_LOG_DEBUG("APP_req_added_dev_info_handle\n");
    db = data.db;
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
    /*获取当前账户*/
    sql = "select ACCOUNT from account_info where CLIENT_FD = ? order by ID desc limit 1;";
    M1_LOG_DEBUG( "%s\n", sql);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    sqlite3_bind_int(stmt, 1, data.clientFd);
    rc = sqlite3_step(stmt);   
    if(rc == SQLITE_ROW)
    {
        account =  sqlite3_column_text(stmt, 0);
        if(account == NULL){
            M1_LOG_ERROR( "user account do not exist\n");    
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }else{
            M1_LOG_DEBUG("account:%s\n",account);
        }
    }
    
    sql_1 = "select DEV_ID, DEV_NAME, PID from all_dev where DEV_ID  = AP_ID and ACCOUNT = ?;";
    if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1),&stmt_1, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    sql_2 = "select DEV_ID, DEV_NAME, PID from all_dev where AP_ID = ? and AP_ID != DEV_ID and ACCOUNT = ?;";
    M1_LOG_DEBUG("sql_2:%s\n",sql_2);

    if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2),&stmt_2, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    sqlite3_bind_text(stmt_1, 1, account, -1, NULL);
    while(sqlite3_step(stmt_1) == SQLITE_ROW)
    {
        apId   = sqlite3_column_text(stmt_1, 0);
        apName = sqlite3_column_text(stmt_1, 1);
        pId    = sqlite3_column_int(stmt_1, 2);
        /*add ap infomation: port,ap_id,ap_name,time */
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
        cJSON_AddNumberToObject(devDataObject, "pId", pId);
        cJSON_AddStringToObject(devDataObject, "apId", apId);
        cJSON_AddStringToObject(devDataObject, "apName", apName);
            
        /*create devData array*/
        devArray = cJSON_CreateArray();
        if(NULL == devArray)
        {
            M1_LOG_ERROR("devArry NULL\n");
            cJSON_Delete(devArray);
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }
        /*add devData array to pdu pbject*/
        cJSON_AddItemToObject(devDataObject, "dev", devArray);
        /*sqlite3*/
        sqlite3_bind_text(stmt_2, 1, apId, -1 , NULL);
        sqlite3_bind_text(stmt_2, 2, account, -1 , NULL);
        while(sqlite3_step(stmt_2) == SQLITE_ROW)
        {
            devId   = sqlite3_column_text(stmt_2, 0);
            devName = sqlite3_column_text(stmt_2, 1);
            pId    = sqlite3_column_int(stmt_2, 2);
             /*add ap infomation: port,ap_id,ap_name,time */
            devObject = cJSON_CreateObject();
            if(NULL == devObject)
            {
                M1_LOG_ERROR("devObject NULL\n");
                cJSON_Delete(devObject);
                ret = M1_PROTOCOL_FAILED;
                goto Finish;
            }
            cJSON_AddItemToArray(devArray, devObject); 
            cJSON_AddNumberToObject(devObject, "pId", pId);
            cJSON_AddStringToObject(devObject, "devId", devId);
            cJSON_AddStringToObject(devObject, "devName", devName);
        }
         
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
    if(stmt != NULL)
        sqlite3_finalize(stmt);
    if(stmt_1 != NULL)
        sqlite3_finalize(stmt_1);
    if(stmt_2 != NULL)
        sqlite3_finalize(stmt_2);
    
    cJSON_Delete(pJsonRoot);

    return ret;
}

static int APP_net_control(payload_t data)
{
    M1_LOG_DEBUG("APP_net_control\n");
    int clientFd         = 0; 
    int rc               = 0;
    int ret              = M1_PROTOCOL_OK; 
    int pduType          = TYPE_DEV_NET_CONTROL;
    char *sql            = NULL; 
    sqlite3 *db          = NULL;
    sqlite3_stmt *stmt   = NULL;
    cJSON *pJsonRoot     = NULL;
    cJSON *apIdJson      = NULL;
    cJSON *valueJson     = NULL;
    cJSON *pduJsonObject = NULL;

    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    };

    db = data.db;
    apIdJson = cJSON_GetObjectItem(data.pdu, "apId");
    if(apIdJson == NULL)
    {
        M1_LOG_WARN("apIdJson NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    M1_LOG_DEBUG("apId:%s\n",apIdJson->valuestring);
    valueJson = cJSON_GetObjectItem(data.pdu, "value");
    if(valueJson == NULL)
    {
        M1_LOG_WARN("valueJson NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    M1_LOG_DEBUG("value:%d\n",valueJson->valueint);  

    sql = "select CLIENT_FD from conn_info where AP_ID = ?;";
    M1_LOG_DEBUG("sql:%s\n",sql);
    if(sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    sqlite3_bind_text(stmt, 1, apIdJson->valuestring, -1, NULL);

    rc = sqlite3_step(stmt);
    M1_LOG_DEBUG("step() return %s\n", \
        rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");
    if(rc == SQLITE_ROW){
        clientFd = sqlite3_column_int(stmt,0);
        M1_LOG_DEBUG("clientFd:%05d\n",clientFd);          
    }else{
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*create json*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_ERROR("pJsonRoot NULL\n");
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

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), clientFd);
    
    Finish:
    if(stmt != NULL)
        sqlite3_finalize(stmt);

    cJSON_Delete(pJsonRoot);
    
    return ret;
}

static int M1_report_ap_info(payload_t data)
{
    M1_LOG_DEBUG(" M1_report_ap_info\n");

    int rc                  = 0;
    int ret                 = M1_PROTOCOL_OK;
    int pduType             = TYPE_M1_REPORT_AP_INFO;
    /*Tx data*/
    const char *apId       = NULL;
    const char *apName     = NULL;
    int pId                 = 0;
    /*Json*/
    char *sql               = NULL;
    cJSON *devDataObject    = NULL;
    cJSON *pJsonRoot        = NULL;
    cJSON *pduJsonObject    = NULL;
    cJSON *devDataJsonArray = NULL;
    /*sql*/
    sqlite3 *db             = NULL;
    sqlite3_stmt *stmt      = NULL;

    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_ERROR("pJsonRoot NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    db = data.db;
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

    sql = "select DEV_ID, DEV_NAME, PID from all_dev where DEV_ID = AP_ID and ACCOUNT in \
          (select ACCOUNT from account_info where CLIENT_FD = ? order by ID desc limit 1);";
    M1_LOG_DEBUG( "%s\n", sql);

    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    sqlite3_bind_int(stmt, 1, data.clientFd);
   
    while(sqlite3_step(stmt) == SQLITE_ROW){
        apId   = sqlite3_column_text(stmt, 0);
        apName = sqlite3_column_text(stmt, 1);
        pId    = sqlite3_column_int(stmt, 2);
        /*add ap infomation: port,ap_id,ap_name,time */
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
        cJSON_AddNumberToObject(devDataObject, "pId", pId);
        cJSON_AddStringToObject(devDataObject, "apId", apId);
        cJSON_AddStringToObject(devDataObject, "apName", apName);
        
    }

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), data.clientFd);
    Finish:
    if(stmt != NULL)
        sqlite3_finalize(stmt);
    cJSON_Delete(pJsonRoot);

    return ret;
}

static int M1_report_dev_info(payload_t data)
{
    M1_LOG_DEBUG(" M1_report_dev_info\n");
    int pduType             = TYPE_M1_REPORT_DEV_INFO;
    int rc                  = 0;
    int ret                 = M1_PROTOCOL_OK;
    char *apId              = NULL;
    char *account           = NULL;
    /*Tx data*/
    const char *devId       = NULL;
    const char *devName     = NULL;
    int pId                 = 0;
    /*Json*/
    cJSON *pJsonRoot        = NULL;
    cJSON *pduJsonObject    = NULL;
    cJSON *devDataJsonArray = NULL;
    cJSON *devDataObject    = NULL;
    /*sql*/
    char *sql               = NULL;
    sqlite3 *db             = NULL;
    sqlite3_stmt *stmt      = NULL;

    db = data.db;
    apId = data.pdu->valuestring;
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

    sql = "select DEV_ID, DEV_NAME, PID from all_dev where AP_ID != DEV_ID and AP_ID = ? and  ACCOUNT in \
          (select ACCOUNT from account_info where CLIENT_FD = ? order by ID desc limit 1);";
    M1_LOG_DEBUG("sql:%s", sql);

    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    sqlite3_bind_text(stmt, 1, apId, -1, NULL);
    sqlite3_bind_int(stmt, 2, data.clientFd);

    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        devId   = sqlite3_column_text(stmt, 0);
        devName = sqlite3_column_text(stmt, 1);
        pId     = sqlite3_column_int(stmt, 2);
        /*add ap infomation: port,ap_id,ap_name,time */
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
        cJSON_AddNumberToObject(devDataObject, "pId", pId);
        cJSON_AddStringToObject(devDataObject, "devId", devId);
        cJSON_AddStringToObject(devDataObject, "devName", devName);
        
    }

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }

    M1_LOG_DEBUG("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), data.clientFd);
    
    Finish:
    if(stmt != NULL)
        sqlite3_finalize(stmt);
    cJSON_Delete(pJsonRoot);

    return ret;
}

/*子设备/AP/联动/场景/区域/-启动/停止/删除*/
static int common_operate(payload_t data)
{
    M1_LOG_DEBUG("common_operate\n");
    int pduType        = TYPE_COMMON_OPERATE;
    int rc             = 0;
    int ret            = M1_PROTOCOL_OK; 
    int number1        = 0;
    int i              = 0;
    char *scen_name    = NULL;
    char *sql          = (char*)malloc(300);
    char *errorMsg     = NULL;
    cJSON *typeJson    = NULL;
    cJSON *idJson      = NULL;
    cJSON *operateJson = NULL;    
    sqlite3 *db        = NULL;
    sqlite3_stmt *stmt = NULL;

    if(data.pdu == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    typeJson = cJSON_GetObjectItem(data.pdu, "type");
    if(typeJson == NULL)
    {
        M1_LOG_WARN("typeJson NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    M1_LOG_DEBUG("type:%s\n",typeJson->valuestring);
    idJson = cJSON_GetObjectItem(data.pdu, "id");   
    if(idJson == NULL)
    {
        M1_LOG_WARN("idJson NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    M1_LOG_DEBUG("id:%s\n",idJson->valuestring);
    operateJson = cJSON_GetObjectItem(data.pdu, "operate");   
    if(operateJson == NULL)
    {
        M1_LOG_WARN("operateJson NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    M1_LOG_DEBUG("operate:%s\n",operateJson->valuestring);
    /*获取数据库*/
    db = data.db;
    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        M1_LOG_DEBUG("BEGIN IMMEDIATE\n");
        if(strcmp(typeJson->valuestring, "device") == 0){
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*通知到ap*/
                // if(m1_del_dev_from_ap(db, idJson->valuestring) != M1_PROTOCOL_OK)
                //     M1_LOG_ERROR("m1_del_dev_from_ap error\n");
                /*删除all_dev中的子设备*/
                sprintf(sql,"delete from all_dev where DEV_ID = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除scenario_table中的子设备*/
                sprintf(sql,"delete from scenario_table where DEV_ID = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除link_trigger_table中的子设备*/
                sprintf(sql,"delete from link_trigger_table where DEV_ID = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除link_exec_table中的子设备*/
                sprintf(sql,"delete from link_exec_table where DEV_ID = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
               
            }else if(strcmp(operateJson->valuestring, "on") == 0){
                sprintf(sql,"update all_dev set STATUS = \"ON\" where DEV_ID = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*更新启停状态*/
                update_param_tb_t dev_status;
                dev_status.devId = idJson->valuestring;
                dev_status.type = 0x2022;
                dev_status.value = 1;
                app_update_param_table(dev_status, db);
            }else if(strcmp(operateJson->valuestring, "off") == 0){
                sprintf(sql,"update all_dev set STATUS = \"OFF\" where DEV_ID = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*更新启停状态*/
                update_param_tb_t dev_status;
                dev_status.devId = idJson->valuestring;
                dev_status.type = 0x2022;
                dev_status.value = 0;
                app_update_param_table(dev_status, db);
            }

        }else if(strcmp(typeJson->valuestring, "linkage") == 0){
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*删除联动表linkage_table中相关内容*/
                sprintf(sql,"delete from linkage_table where LINK_NAME = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
      
                /*删除联动触发表link_trigger_table相关内容*/
                sprintf(sql,"delete from link_trigger_table where LINK_NAME = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除联动触发表link_exec_table相关内容*/
                sprintf(sql,"delete from link_exec_table where LINK_NAME = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
            }
        }else if(strcmp(typeJson->valuestring, "scenario") == 0){
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*删除场景表相关内容*/
                sprintf(sql,"delete from scenario_table where SCEN_NAME = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除场景定时相关内容*/
                sprintf(sql,"delete from scen_alarm_table where SCEN_NAME = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除联动表linkage_table中相关内容*/
                sprintf(sql,"delete from linkage_table where EXEC_ID = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
            }
        }else if(strcmp(typeJson->valuestring, "district") == 0){
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*删除区域相关内容*/
                sprintf(sql,"delete from district_table where DIS_NAME = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除区域下的联动表中相关内容*/
                sprintf(sql,"delete from linkage_table where DISTRICT = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);    
                /*删除区域下的联动触发表中相关内容*/
                sprintf(sql,"delete from link_trigger_table where DISTRICT = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);    
                /*删除区域下的联动触发表中相关内容*/
                sprintf(sql,"delete from link_exec_table where DISTRICT = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql); 
                /*删除场景定时相关内容*/
                sprintf(sql,"select SCEN_NAME from scenario_table where DISTRICT = \"%s\" order by ID desc limit 1;",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                
                if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
                    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
                    ret = M1_PROTOCOL_FAILED;
                    goto Finish; 
                }
                rc = sqlite3_step(stmt);
                M1_LOG_DEBUG("step() return %s\n", \
                    rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");
                if(rc == SQLITE_ROW){
                    scen_name = sqlite3_column_text(stmt,0);
                    if(scen_name == NULL)
                    {
                        M1_LOG_WARN("scen_name NULL \n");
                        ret = M1_PROTOCOL_FAILED;
                        goto Finish;
                    }
                }
                sprintf(sql,"delete from scen_alarm_table where SCEN_NAME = \"%s\";",scen_name);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql); 
                /*删除区域下场景表相关内容*/   
                sprintf(sql,"delete from scenario_table where DISTRICT = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql); 

                if(stmt != NULL){
                    sqlite3_finalize(stmt);
                    stmt = NULL;
                }
            }
        }else if(strcmp(typeJson->valuestring, "account") == 0){
            if(strcmp(idJson->valuestring,"Dalitek") == 0){
                goto Finish;
            }
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*删除account_table中的信息*/
                sprintf(sql,"delete from account_table where ACCOUNT = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除all_dev中的信息*/
                sprintf(sql,"delete from all_dev where ACCOUNT = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除district_table中的信息*/
                sprintf(sql,"delete from district_table where ACCOUNT = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除scenario_table中的信息*/
                sprintf(sql,"delete from scenario_table where ACCOUNT = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
            }
        }else if(strcmp(typeJson->valuestring, "ap") == 0){
            /*删除all_dev中设备*/
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*通知ap*/
                if(m1_del_ap(db, idJson->valuestring) != M1_PROTOCOL_OK)
                    M1_LOG_ERROR("m1_del_ap error\n");
                /*删除AP下子设备相关连的联动业务*/
                #if 0
                clear_ap_related_linkage(idJson->valuestring, db);
                #endif
                ///*删除all_dev中的子设备*/
                sprintf(sql,"delete from all_dev where AP_ID = \"%s\";",idJson->valuestring);
                //sprintf(sql,"update all_dev set ADDED = 0,NET = 0,STATUS = \"OFF\" where AP_ID = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除scenario_table中的子设备*/
                sprintf(sql,"delete from scenario_table where AP_ID = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除link_trigger_table中的子设备*/
                sprintf(sql,"delete from link_trigger_table where AP_ID = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除link_exec_table中的子设备*/
                sprintf(sql,"delete from link_exec_table where AP_ID = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*删除district_table中的子设备*/
                sprintf(sql,"delete from district_table where AP_ID = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
            }else if(strcmp(operateJson->valuestring, "on") == 0){
                sprintf(sql,"update all_dev set STATUS = \"ON\" where AP_ID = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*更新启停状态*/
                update_param_tb_t dev_status;
                dev_status.devId = idJson->valuestring;
                dev_status.type = 0x2022;
                dev_status.value = 1;
                app_update_param_table(dev_status, db);
            }else if(strcmp(operateJson->valuestring, "off") == 0){
                sprintf(sql,"update all_dev set STATUS = \"OFF\" where AP_ID = \"%s\";",idJson->valuestring);
                M1_LOG_DEBUG("sql:%s\n",sql);
                sql_exec(db, sql);
                /*更新启停状态*/
                update_param_tb_t dev_status;
                dev_status.devId = idJson->valuestring;
                dev_status.type = 0x2022;
                dev_status.value = 0;
                app_update_param_table(dev_status, db);
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
    free(sql);

    if(stmt != NULL)
        sqlite3_finalize(stmt);

    return ret;
}

#define AP_HEART_BEAT_INTERVAL   2   //min
/*查询离线设备*/
static void check_offline_dev(sqlite3*db)
{
    M1_LOG_DEBUG("check_offline_dev\n");
    int rc = 0;
    static char preTime[4] = {0};
    char* curTime = (char*)malloc(30);
    char *time = NULL;
    int u8CurTime = 0, u8Time = 0;
    char* apId = NULL;
    char* errorMsg = NULL;
    char* sql = NULL;
    char* sql_1 = (char*)malloc(300);
    char* sql_2 = (char*)malloc(300);
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* stmt_1 = NULL;
    sqlite3_stmt* stmt_2 = NULL;

    getNowTime(curTime);
    /*当前时间*/
    M1_LOG_DEBUG("curTime:%x,%x,%x,%x\n",curTime[8],curTime[9],curTime[10],curTime[11]);
    u8CurTime = (curTime[8] - preTime[0])*60*10 + (curTime[9] - preTime[1]) * 60  + (curTime[10] - preTime[2]) * 10 + curTime[11] - preTime[3];
    u8CurTime = abs(u8CurTime);
    M1_LOG_DEBUG("u8CurTime:%d\n",u8CurTime);
    
    if(u8CurTime < 2){
        goto Finish;
    }
    memcpy(preTime, &curTime[8], 4);
    M1_LOG_DEBUG("preTime:%x,%x,%x,%x\n",preTime[0],preTime[1],preTime[2],preTime[3]);
    /*获取AP ID*/
    sql = "select AP_ID from all_dev where DEV_ID = AP_ID;";
    M1_LOG_DEBUG("string:%s\n",sql);
    if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        goto Finish; 
    }
    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK){
        M1_LOG_DEBUG("BEGIN:\n");
        while(thread_sqlite3_step(&stmt, db) == SQLITE_ROW){
            apId = sqlite3_column_text(stmt, 0);
            /*检查时间*/
            sprintf(sql_1,"select TIME from param_table where DEV_ID = \"%s\" and TYPE = 16404 order by ID desc limit 1;", apId);
            M1_LOG_DEBUG("string:%s\n",sql_1);
            sqlite3_finalize(stmt_1);
            if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK){
                M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
                goto Finish; 
            }
            rc = thread_sqlite3_step(&stmt_1, db);
            if(rc != SQLITE_ROW){
                M1_LOG_DEBUG("rc != SQLITE_ROW\n");
                continue;
            }
            time = sqlite3_column_text(stmt_1, 0);
            if(time == NULL){
                M1_LOG_DEBUG("time == NULL\n");
                continue;
            }
             M1_LOG_DEBUG("time: %s\n",time);
            /*获取的上一次时间*/
            u8Time = (curTime[8] - time[8]) * 60 * 10 + (curTime[9] - time[9]) * 60 + (curTime[10] - time[10]) * 10 + curTime[11] - time[11];
            u8Time = abs(u8Time);
            M1_LOG_DEBUG("u8Time:%d\n",u8Time);
            if(u8Time > AP_HEART_BEAT_INTERVAL){
                //sprintf(sql_2,"update param_table set VALUE = 0 where DEV_ID = \"%s\" and TYPE = 16404;", apId);
                /*测试将主公你太全都改为在线*/
                sprintf(sql_2,"update param_table set VALUE = 1 where DEV_ID = \"%s\" and TYPE = 16404;", apId);
                sqlite3_finalize(stmt_2);
                if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK){
                    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
                    goto Finish; 
                }
                rc = thread_sqlite3_step(&stmt_2, db);
                if(rc == SQLITE_ERROR){
                    M1_LOG_WARN("update failed\n");
                    continue;
                }
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

    free(curTime);
    free(sql_1);
    free(sql_2);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);
    sqlite3_finalize(stmt_2);
}

static int ap_heartbeat_handle(payload_t data)
{
    M1_LOG_DEBUG("ap_heartbeat_handle\n");
    int rc             = 0;
    int ret            = M1_PROTOCOL_OK;
    int valueType      = 0x4014;
    char* errorMsg     = NULL;
    char* sql          = NULL;
    cJSON* apIdJson    = NULL;
    sqlite3* db        = NULL;
    sqlite3_stmt* stmt = NULL;

    if(data.pdu == NULL){
        M1_LOG_ERROR("data.pdu\n");
        goto Finish;
    }

    db = data.db;
    apIdJson = data.pdu;

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        sql = "update param_table set VALUE = 1 where DEV_ID = ? and TYPE = ?;";
        M1_LOG_DEBUG("string:%s\n",sql);

        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }

        sqlite3_bind_text(stmt, 1, apIdJson->valuestring, -1, NULL);
        sqlite3_bind_int(stmt, 2, valueType);

        rc = sqlite3_step(stmt);
        M1_LOG_DEBUG("step() return %s\n", \
            rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");

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
    if(stmt != NULL)
        sqlite3_finalize(stmt);

    return ret;
}

static int common_rsp(rsp_data_t data)
{
    M1_LOG_DEBUG(" common_rsp\n");
    
    int ret = M1_PROTOCOL_OK;
    int pduType = TYPE_COMMON_RSP;
    cJSON * pJsonRoot = NULL;
    cJSON * pduJsonObject = NULL;
    cJSON * devDataObject = NULL;

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

    M1_LOG_DEBUG("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), data.clientFd);

    Finish:
    cJSON_Delete(pJsonRoot);

    return ret;
}

void delete_account_conn_info(int clientFd)
{
    int rc             = 0;
    int ret            = M1_PROTOCOL_OK;
    char *errorMsg     = NULL;
    char *sql          = NULL;
    sqlite3_stmt* stmt = NULL;

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        {
            sql = "delete from account_info where CLIENT_FD = ?;";
            M1_LOG_DEBUG("string:%s\n",sql);

            if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
            {
                M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
                ret = M1_PROTOCOL_FAILED;
                goto Finish; 
            }

            sqlite3_bind_int(stmt, 1, clientFd);

            rc = sqlite3_step(stmt);
            M1_LOG_DEBUG("step() return %s\n", \
                rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");
            
            if(stmt)
                sqlite3_finalize(stmt);
        }

        {
            sql = "delete from conn_info where CLIENT_FD = ?;";
            M1_LOG_DEBUG("string:%s\n",sql);

            if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
            {
                M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
                ret = M1_PROTOCOL_FAILED;
                goto Finish; 
            }

            sqlite3_bind_int(stmt, 1, clientFd);

            rc = sqlite3_step(stmt);
            M1_LOG_DEBUG("step() return %s\n", \
                rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR");
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

}

static void delete_client_db(void)
{
    int clientFd = 0;
    while(fifo_read(&client_delete_fifo, &clientFd))
    {
        delete_account_conn_info(clientFd);
    }
}

/*app修改设备名称*/
static int app_change_device_name(payload_t data)
{
    int rc               = 0; 
    int ret              = M1_PROTOCOL_OK;
    char* errorMsg       = NULL;
    char* sql            = NULL;
    sqlite3* db          = NULL;
    sqlite3_stmt* stmt   = NULL;
    cJSON* devIdObject   = NULL;
    cJSON* devNameObject = NULL;

    if(data.pdu == NULL)
    {
        M1_LOG_ERROR("data NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    db = data.db;   

    /*获取devId*/
    devIdObject = cJSON_GetObjectItem(data.pdu, "devId");
    if(devIdObject == NULL)
    {
        M1_LOG_ERROR("devIdObject NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;   
    }
    M1_LOG_DEBUG("devId:%s\n",devIdObject->valuestring);
    devNameObject = cJSON_GetObjectItem(data.pdu, "devName");   
    if(devNameObject == NULL)
    {
        M1_LOG_WARN("devNameObject NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    M1_LOG_DEBUG("devName:%s\n",devNameObject->valuestring);

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        M1_LOG_DEBUG("BEGIN:\n");

        sql = "update all_dev set DEV_NAME = ? where DEV_ID = ?;";
        M1_LOG_DEBUG("sql:%s\n",sql);

        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }

        sqlite3_bind_text(stmt, 1, devNameObject->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 2, devIdObject->valuestring, -1, NULL);

        rc = sqlite3_step(stmt);
        if(rc == SQLITE_ERROR)
        {
            ret = M1_PROTOCOL_FAILED;
            goto Finish;   
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

/*Hex to int*/
static uint8_t hex_to_uint8(int h)
{
    if( (h>='0' && h<='9') ||  (h>='a' && h<='f') ||  (h>='A' && h<='F') )
        h += 9*(1&(h>>6));
    else{
        return 0;
    }

    return (uint8_t)(h & 0xf);
}

void setLocalTime(char* time)
{
    struct tm local_tm, *time_1;
    struct timeval tv;
    struct timezone tz;
    int mon, day,hour,min;

    gettimeofday(&tv, &tz);
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
    while(1){
        if(!IsEmpty(&head)){
            if(head.next->item.prio <= 0){
                Pop(&head, &item);
                p = item.data;
                //M1_LOG_INFO("delay_send_task data:%s\n",p);
                M1_LOG_WARN("delay_send_task data:%s\n",p);
                socketSeverSend((uint8*)p, strlen(p), item.clientFd);
            }
        }
        usleep(100000);
        count++;
        if(!(count % 10)){
           // count = 0;
            Queue_delay_decrease(&head);
        }
#if TCP_CLIENT_ENABLE
        /*M1心跳到云端*/
        if(!(count % 300))
            m1_heartbeat_to_cloud();
#endif
    }
}

int sql_id(sqlite3* db, char* sql)
{
    M1_LOG_DEBUG("sql_id\n");
    sqlite3_stmt* stmt = NULL;
    int id, total_column, rc;
    /*get id*/
    if(sqlite3_prepare_v2(db, sql, strlen(sql), & stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        goto Finish; 
    }
    rc = thread_sqlite3_step(&stmt, db);
    if(rc == SQLITE_ROW){
            id = (sqlite3_column_int(stmt, 0) + 1);
    }else{
        id = 1;
    }

    Finish:
    sqlite3_finalize(stmt);
    return id;
}

int sql_row_number(sqlite3* db, char*sql)
{
    char** p_result;
    char* errmsg;
    int n_row, n_col, rc;
    rc = sqlite3_get_table(db, sql, &p_result,&n_row, &n_col, &errmsg);
    if(rc != SQLITE_OK)
        M1_LOG_ERROR("sql_row_number failed:%s\n",errmsg);

    sqlite3_free(errmsg);
    sqlite3_free_table(p_result);

    return n_row;
}

int sql_exec(sqlite3* db, char*sql)
{
    sqlite3_stmt* stmt = NULL;
    int rc;
    /*get id*/
    if(sqlite3_prepare_v2(db, sql, strlen(sql), & stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        goto Finish; 
    }
    do{
        rc = thread_sqlite3_step(&stmt, db);
    }while(rc == SQLITE_ROW);
   
    Finish:
    sqlite3_finalize(stmt);
    return rc;
}

/*打开数据库*/
int sql_open(void)
{
    int rc;

    pthread_mutex_lock(&mutex_lock);
    M1_LOG_DEBUG( "pthread_mutex_lock\n");

    rc = sqlite3_open(db_path, &db);
    if( rc != SQLITE_OK){  
        M1_LOG_ERROR( "Can't open database\n");  
    }else{  
        M1_LOG_DEBUG( "Opened database successfully\n");  
    }

    return rc;
}

/*关闭数据库*/
int sql_close(void)
{
    int rc;
    
    M1_LOG_DEBUG( "Sqlite3 close\n");
    rc = sqlite3_close(db);

    pthread_mutex_unlock(&mutex_lock);
    M1_LOG_DEBUG( "pthread_mutex_unlock\n");

    return rc;
}

int thread_sqlite3_step(sqlite3_stmt** stmt, sqlite3* db)
{
    int sleep_acount = 0;
    int rc;
    char* errorMsg = NULL;

    rc = sqlite3_step(*stmt);   
    M1_LOG_DEBUG("step() return %s, number:%03d\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR",rc);
    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK)){
        M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
    }

    return rc;
}

static int create_sql_table(void)
{
    char* sql          = NULL;
    int rc             = 0;
    int ret            = M1_PROTOCOL_OK;
    char *mac_addr     = NULL;
    char *errmsg       = NULL;
    sqlite3_stmt *stmt = NULL;

    sqlite3* db = 0;
    rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if( rc ){  
        M1_LOG_ERROR( "Can't open database: %s\n", sqlite3_errmsg(db));  
        goto Finish;
        ret = M1_PROTOCOL_FAILED;
    }else{  
        M1_LOG_DEBUG( "Opened database successfully\n");  
    }
    /*account_info*/
    {
        sql = "CREATE TABLE account_info (                           \
                   ID        INTEGER PRIMARY KEY AUTOINCREMENT,      \
                   ACCOUNT   TEXT,                                   \
                   CLIENT_FD INTEGER                                 \
               );";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            M1_LOG_WARN("account_info already exit: %s\n",errmsg);
            // sql = "delete from account_info where ID > 0;";
            // M1_LOG_DEBUG("%s:\n",sql);
            // rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
            // if(rc != SQLITE_OK)
            // {
            //     M1_LOG_WARN("delete from account_info failed: %s\n",errmsg);
            //     sqlite3_free(errmsg);
            // }
        }
        /*account index*/
        sql = "CREATE UNIQUE INDEX appAcount ON account_info (\"ACCOUNT\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            M1_LOG_WARN("CREATE UNIQUE INDEX appAcount failed: %s\n",errmsg);
            sqlite3_free(errmsg);
        }
        /*client_fdindex*/
        sql = "CREATE UNIQUE INDEX appClient ON account_info (\"CLIENT_FD\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            M1_LOG_WARN("delete from account_info failed: %s\n",errmsg);
            sqlite3_free(errmsg);
        }
    }
    
    /*account_table*/
    {
        sql = "CREATE TABLE account_table (                        \
                ID          INTEGER PRIMARY KEY AUTOINCREMENT,     \
                ACCOUNT     TEXT,                                  \
                [KEY]       TEXT,                                  \
                KEY_AUTH    TEXT,                                  \
                REMOTE_AUTH TEXT,                                  \
                TIME        TIME    NOT NULL                       \
                                    DEFAULT CURRENT_TIMESTAMP      \
            );";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK){
            M1_LOG_WARN("account_table already exit: %s\n",errmsg);
        }
        sqlite3_free(errmsg);

        /*account index*/
        sql = "CREATE UNIQUE INDEX userAccount ON account_table (\"ACCOUNT\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            M1_LOG_WARN("delete from account_info failed: %s\n",errmsg);
            sqlite3_free(errmsg);
        }
    }
    /*all_dev*/
    { 
        sql = "CREATE TABLE all_dev (                           \
                ID       INTEGER PRIMARY KEY AUTOINCREMENT,     \
                DEV_ID   TEXT,                                  \
                DEV_NAME TEXT,                                  \
                ACCOUNT  TEXT,                                  \
                AP_ID    TEXT,                                  \
                PID      INTEGER,                               \
                ADDED    INTEGER,                               \
                NET      INTEGER,                               \
                STATUS   TEXT,                                  \
                TIME     TIME    DEFAULT CURRENT_TIMESTAMP      \
                                 NOT NULL                       \
            );"; 
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("all_dev: %s\n",errmsg);
        }
        /*dev_id,account,ap_id index*/
        sql = "CREATE UNIQUE INDEX userAccRecord ON all_dev (\"DEV_ID\" ASC,\"ACCOUNT\" ASC,\"AP_ID\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("CREATE UNIQUE INDEX: %s\n",errmsg);
        }              
    }
    /*conn_info*/
    {
        sql = "CREATE TABLE conn_info (                        \
                ID        INTEGER PRIMARY KEY AUTOINCREMENT,   \
                AP_ID     TEXT,                                \
                CLIENT_FD INTEGER                              \
            );";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            M1_LOG_WARN("conn_info: %s\n",errmsg);
            // sql = "delete from conn_info where ID > 0;";
            // M1_LOG_DEBUG("%s:\n",sql);
            // rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
            // if(rc != SQLITE_OK)
            // {
            //     M1_LOG_WARN("delete from conn_info:%s\n",errmsg);
            // }
            sqlite3_free(errmsg);   
        }
         
        /*AP_ID index*/
        sql = "CREATE UNIQUE INDEX apAccount ON conn_info (\"AP_ID\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("CREATE UNIQUE INDEX: %s\n",errmsg);
        }
        /*CLIENT_FD index*/
        sql = "CREATE UNIQUE INDEX apClient ON conn_info (\"CLIENT_FD\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("CREATE UNIQUE INDEX: %s\n",errmsg);
        }
    }
    /*district_table*/
    {
        sql = "CREATE TABLE district_table (                 \
                ID       INTEGER PRIMARY KEY AUTOINCREMENT,  \
                DIS_NAME TEXT,                               \
                DIS_PIC  TEXT,                               \
                AP_ID    TEXT,                               \
                ACCOUNT  TEXT,                               \
                TIME     TIME    NOT NULL                    \
                                 DEFAULT CURRENT_TIMESTAMP   \
            );";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            M1_LOG_WARN("district_table: %s\n",errmsg);
        }
        sqlite3_free(errmsg); 
        /*DIS_NAME,AP_ID,ACCOUNT UNION index*/
        sql = "CREATE UNIQUE INDEX district ON district_table (\"DIS_NAME\" ASC, \"AP_ID\" ASC, \"ACCOUNT\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("CREATE INDEX district: %s\n",errmsg);
        }   
    }
    
    /*link_exec_table*/
    {
        sql = "CREATE TABLE link_exec_table (             \
            ID        INTEGER PRIMARY KEY AUTOINCREMENT,  \
            LINK_NAME TEXT,                               \
            DISTRICT  TEXT,                               \
            AP_ID     TEXT,                               \
            DEV_ID    TEXT,                               \
            TYPE      INT,                                \
            VALUE     INT,                                \
            DELAY     INT,                                \
            TIME      TIME    DEFAULT CURRENT_TIMESTAMP   \
                              NOT NULL                    \
        );";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("link_exec_table already exit: %s\n",errmsg);
        }    
        /*LINK_NAME,DISTRICT,DEV_ID,TYPE UNION index*/
        sql = " CREATE UNIQUE INDEX linkExec ON link_exec_table \
               (\"LINK_NAME\" ASC,\"DISTRICT\" ASC,\"DEV_ID\" ASC,\"TYPE\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("link_exec_table already exit: %s\n",errmsg);
        }
    }
    /*link_trigger_table*/
    {
        sql = "CREATE TABLE link_trigger_table (               \
                ID        INTEGER PRIMARY KEY AUTOINCREMENT,   \
                LINK_NAME TEXT,                                \
                DISTRICT  TEXT,                                \
                AP_ID     TEXT,                                \
                DEV_ID    TEXT,                                \
                TYPE      INT,                                 \
                THRESHOLD INT,                                 \
                CONDITION TEXT,                                \
                LOGICAL   TEXT,                                \
                STATUS    TEXT,                                \
                TIME      TIME    NOT NULL                     \
                                  DEFAULT CURRENT_TIMESTAMP    \
            );";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("link_trigger_table already exit: %s\n",errmsg);
        }
        /*LINK_NAME,DISTRICT,DEV_ID,TYPE UNION index*/
        sql = "CREATE UNIQUE INDEX linkTrigger ON link_trigger_table \
               (\"LINK_NAME\" ASC,\"DISTRICT\" ASC,\"DEV_ID\" ASC,\"TYPE\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("link_trigger_table already exit: %s\n",errmsg);
        }
    }
    /*linkage_table*/
    {
        sql = "CREATE TABLE linkage_table (                   \
                ID        INTEGER PRIMARY KEY AUTOINCREMENT,  \
                LINK_NAME TEXT,                               \
                DISTRICT  TEXT,                               \
                EXEC_TYPE TEXT,                               \
                EXEC_ID   TEXT,                               \
                STATUS    TEXT,                               \
                ENABLE    TEXT,                               \
                TIME      TIME    NOT NULL                    \
                                  DEFAULT CURRENT_TIMESTAMP   \
            );";       
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("linkage_table: %s\n",errmsg);
        }
        /*LINK_NAME,DISTRICT,EXEC_TYPE,EXEC_ID index*/
        sql = "CREATE UNIQUE INDEX linkage ON linkage_table \
               (\"LINK_NAME\" ASC,\"DISTRICT\" ASC,\"EXEC_TYPE\" ASC,\"EXEC_ID\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("linkage_table: %s\n",errmsg);
        }
    }
    /*param_table*/
    {
        sql = "CREATE TABLE param_table (                     \
                ID       INTEGER PRIMARY KEY AUTOINCREMENT,   \
                DEV_ID   TEXT,                                \
                DEV_NAME TEXT,                                \
                TYPE     INT,                                 \
                VALUE    INT,                                 \
                TIME     TIME    NOT NULL                     \
                                 DEFAULT CURRENT_TIMESTAMP    \
            );";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("param_table already exit: %s\n",errmsg);
        }
        /*DEV_ID,DEV_NAME,TYPE UNION index*/
        sql = "CREATE UNIQUE INDEX param ON param_table (\"DEV_ID\" ASC,\"DEV_NAME\" ASC,\"TYPE\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("CREATE UNIQUE INDEX: %s\n",errmsg);
        }
    }
    /*scen_alarm_table*/
    {
        //sprintf(sql,"CREATE TABLE scen_alarm_table(ID INT PRIMARY KEY NOT NULL, SCEN_NAME TEXT NOT NULL, HOUR INT NOT NULL,MINUTES INT NOT NULL, WEEK TEXT NOT NULL, STATUS TEXT NOT NULL, TIME TEXT NOT NULL);");
        sql = "CREATE TABLE scen_alarm_table (                 \
                ID        INTEGER PRIMARY KEY AUTOINCREMENT,   \
                SCEN_NAME TEXT,                                \
                HOUR      INT,                                 \
                MINUTES   INT,                                 \
                WEEK      TEXT,                                \
                STATUS    TEXT,                                \
                TIME      TIME    NOT NULL                     \
                                  DEFAULT CURRENT_TIMESTAMP    \
            );";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("scen_alarm_table: %s\n",errmsg);
        }

        sql = "CREATE UNIQUE INDEX scenAlarm ON scen_alarm_table (\"SCEN_NAME\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("CREATE UNIQUE INDEX scenAlarm: %s\n",errmsg);
        }
    }
    /*scenario_table*/
    {
        sql = "CREATE TABLE scenario_table (                   \
                ID        INTEGER PRIMARY KEY AUTOINCREMENT,   \
                SCEN_NAME TEXT,                                \
                SCEN_PIC  TEXT,                                \
                DISTRICT  TEXT,                                \
                AP_ID     TEXT,                                \
                DEV_ID    TEXT,                                \
                TYPE      INT,                                 \
                VALUE     INT,                                 \
                DELAY     INT,                                 \
                ACCOUNT   TEXT,                                \
                TIME      TIME    NOT NULL                     \
                                  DEFAULT CURRENT_TIMESTAMP    \
            );";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("scenario_table already exit: %s\n",errmsg);
        }
        /*SCEN_NAME, DISTRICT,DEV_ID,TYPE,VALUE,ACCOUNT UNION index*/
        sql = "CREATE UNIQUE INDEX scenario ON scenario_table \
               (\"SCEN_NAME\" ASC,\"DISTRICT\" ASC,\"DEV_ID\" ASC,\"TYPE\" ASC,\"VALUE\" ASC,\"ACCOUNT\" ASC );";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("CREATE INDEX scenario: %s\n",errmsg);
        }
    }
    /*project_table*/
    {
        sql = "CREATE TABLE project_table (                    \
                ID        INTEGER PRIMARY KEY AUTOINCREMENT,   \
                P_NAME    TEXT,                                \
                P_NUMBER  TEXT,                                \
                P_CREATOR TEXT,                                \
                P_MANAGER TEXT,                                \
                P_EDITOR  TEXT,                                \
                P_TEL     TEXT,                                \
                P_ADD     TEXT,                                \
                P_BRIEF   TEXT,                                \
                P_KEY     TEXT,                                \
                ACCOUNT   TEXT,                                \
                TIME      TIME    NOT NULL                     \
                                  DEFAULT CURRENT_TIMESTAMP    \
            );";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("project_table already exit: %s\n",errmsg);
        }
        /*P_NUMBER,P_EDITOR UNION index*/
        sql = "CREATE UNIQUE INDEX project ON project_table (\"P_NUMBER\" ASC,\"P_EDITOR\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("CREATE UNIQUE INDEX project: %s\n",errmsg);
        }        
    }
    /*param_detail_table*/
    sql = "CREATE TABLE param_detail_table (                   \
                ID        INTEGER PRIMARY KEY AUTOINCREMENT,   \
                DEV_ID    TEXT,                                \
                TYPE      INT,                                 \
                VALUE     INT,                                 \
                DESCRIP   TEXT,                                \
                TIME      TIME    NOT NULL                     \
                                  DEFAULT CURRENT_TIMESTAMP    \
            );";
    M1_LOG_DEBUG("%s:\n",sql);        
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if(rc != SQLITE_OK)
    {
        sqlite3_free(errmsg);
        M1_LOG_WARN("param_detail_table already exit: %s\n",errmsg);
    }
    /*P_NUMBER,P_EDITOR UNION index*/
    sql = "CREATE UNIQUE INDEX paramDetail ON param_detail_table (\"DEV_ID\" ASC,\"TYPE\" ASC,\"DESCRIP\" ASC);";
    M1_LOG_DEBUG("%s:\n",sql);
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if(rc != SQLITE_OK)
    {
        sqlite3_free(errmsg);
        M1_LOG_WARN("CREATE UNIQUE INDEX paramDetail: %s\n",errmsg);
    }
    
    /*插入Dalitek账户*/
    sql = "insert or replace into account_table(ACCOUNT, KEY, KEY_AUTH, REMOTE_AUTH)\
        values(\"Dalitek\",\"root\",\"on\",\"on\");";
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if(rc != SQLITE_OK)
    {
        M1_LOG_WARN("insert into account_table fail: %s\n",errmsg);
    }
    sqlite3_free(errmsg);
    /*插入项目信息*/
    {
        mac_addr = get_eth0_mac_addr();
        sql = "insert or replace into project_table(P_NAME, P_NUMBER, P_CREATOR, P_MANAGER, P_EDITOR, P_TEL, P_ADD, P_BRIEF, P_KEY, ACCOUNT)\
        values(?,?,?,?,?,?,?,?,?,?);";
        if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK){
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }

        sqlite3_bind_text(stmt, 1, "M1", -1, NULL);
        sqlite3_bind_text(stmt, 2, mac_addr, -1, NULL);
        sqlite3_bind_text(stmt, 3, "Dalitek", -1, NULL);
        sqlite3_bind_text(stmt, 4, "Dalitek", -1, NULL);
        sqlite3_bind_text(stmt, 5, "Dalitek", -1, NULL);
        sqlite3_bind_text(stmt, 6, "123456789", -1, NULL);
        sqlite3_bind_text(stmt, 7, "ShangHai", -1, NULL);
        sqlite3_bind_text(stmt, 8, "Brief", -1, NULL);
        sqlite3_bind_text(stmt, 9, "123456", -1, NULL);
        sqlite3_bind_text(stmt, 10, "Dalitek", -1, NULL);
        
        rc = sqlite3_step(stmt);
        if(rc == SQLITE_ERROR)
        {
            ret = M1_PROTOCOL_FAILED;
            goto Finish;   
        }
    }

    Finish:
    if(stmt)
        sqlite3_finalize(stmt);
    sqlite3_close(db);

    return ret;
}


