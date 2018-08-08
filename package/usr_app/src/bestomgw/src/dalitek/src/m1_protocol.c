#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#include "m1_protocol.h"
#include "account_config.h"
#include "socket_server.h"
#include "tcp_client.h"
#include "buf_manage.h"
#include "m1_project.h"
#include "sql_backup.h"
#include "m1_common_log.h"
#include "m1_device.h"
#include "dev_common.h"
#include "interface_srpcserver.h"

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
static void check_offline_dev(sqlite3*db);
/*variable******************************************************************************************************/
extern pthread_mutex_t mutex_lock;
static int sql_error_flag = 0;
const sqlite3* db = NULL;
const char* db_path = "/bin/dev_info.db";
const char* sql_back_path = "/bin/sql_restore.sh";
// const char* db_path = "dev_info.db";
// const char* sql_back_path = "sql_restore.sh";
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
    int rc             = 0;
    int pduType        = 0;
    cJSON* rootJson    = NULL;
    cJSON* pduJson     = NULL;
    cJSON* pduTypeJson = NULL;
    cJSON* snJson      = NULL;
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
        case TYPE_DEV_READ:                 APP_read_handle(pdu); break;
        case TYPE_REQ_ADDED_INFO:           APP_req_added_dev_info_handle(pdu); break;
        case TYPE_DEV_NET_CONTROL:          rc = APP_net_control(pdu); break;
        case TYPE_REQ_AP_INFO:              M1_report_ap_info(pdu); break;
        case TYPE_REQ_DEV_INFO:             M1_report_dev_info(pdu); break;
        case TYPE_COMMON_RSP:               common_rsp_handle(pdu);rc = M1_PROTOCOL_NO_RSP;break;
        case TYPE_REQ_SCEN_INFO:            rc = app_req_scenario(pdu);break;
        case TYPE_REQ_LINK_INFO:            rc = app_req_linkage(pdu);break;
        case TYPE_REQ_DISTRICT_INFO:        rc = app_req_district(pdu); break;
        case TYPE_REQ_SCEN_NAME_INFO:       rc = app_req_scenario_name(pdu);break;
        case TYPE_REQ_ACCOUNT_INFO:         rc = app_req_account_info_handle(pdu);break;
        case TYPE_REQ_ACCOUNT_CONFIG_INFO:  rc = app_req_account_config_handle(pdu);break;
        case TYPE_GET_PORJECT_NUMBER:       rc = app_get_project_info(pdu); break;
        case TYPE_REQ_DIS_SCEN_NAME:        rc = app_req_dis_scen_name(pdu); break;
        case TYPE_REQ_DIS_NAME:             rc = app_req_dis_name(pdu); break;
        case TYPE_APP_REQ_USER_DIS:         rc = app_req_user_dis(pdu); break;
        case TYPE_REQ_DIS_DEV:              rc = app_req_dis_dev(pdu); break;
        case TYPE_GET_PROJECT_INFO:         rc = app_get_project_config(pdu);break;
        case TYPE_APP_CONFIRM_PROJECT:      rc = app_confirm_project(pdu);break;
        case TYPE_APP_EXEC_SCEN:            rc = app_exec_scenario(pdu);break;
        case TYPE_DEBUG_INFO:               debug_switch(pduDataJson->valuestring);break;
        case TYPE_DEV_VERSION_READ_CONFIG:  m1_ap_version_read(pdu);rc = M1_PROTOCOL_NO_RSP;break;
        /*write*/
        case TYPE_REPORT_DATA:              rc = AP_report_data_handle(pdu); break;
        case TYPE_DEV_WRITE:                rc = M1_write_to_AP(rootJson, db);/*APP_write_handle(pdu);*/break;
        case TYPE_ECHO_DEV_INFO:            rc = APP_echo_dev_info_handle(pdu); break;
        case TYPE_AP_REPORT_DEV_INFO:       rc = AP_report_dev_handle(pdu); break;
        case TYPE_AP_REPORT_AP_INFO:        rc = AP_report_ap_handle(pdu); break;
        case TYPE_CREATE_LINKAGE:           rc = linkage_msg_handle(pdu);break;
        case TYPE_CREATE_SCENARIO:          rc = scenario_create_handle(pdu);break;
        case TYPE_CREATE_DISTRICT:          rc = district_create_handle(pdu);break;
        case TYPE_SCENARIO_ALARM:           rc = scenario_alarm_create_handle(pdu);break;
        case TYPE_COMMON_OPERATE:           rc = common_operate(pdu);break;
        case TYPE_AP_HEARTBEAT_INFO:        rc = ap_heartbeat_handle(pdu);break;
        case TYPE_LINK_ENABLE_SET:          rc = app_linkage_enable(pdu);break;
        case TYPE_APP_LOGIN:                rc = user_login_handle(pdu);break;
        case TYPE_SEND_ACCOUNT_CONFIG_INFO: rc = app_account_config_handle(pdu);break;
        case TYPE_APP_CREATE_PROJECT:       rc = app_create_project(pdu);break;
        case TYPE_PROJECT_KEY_CHANGE:       rc = app_change_project_key(pdu);break;
        case TYPE_PROJECT_INFO_CHANGE:      rc = app_change_project_config(pdu);break;
        case TYPE_APP_CHANGE_DEV_NAME:      rc = app_change_device_name(pdu);break;
        case TYPE_APP_USER_KEY_CHANGE:      rc = app_change_user_key(pdu);break;
        case TYPE_APP_DOWNLOAD_TESTING_INFO: rc = app_download_testing_to_ap(rootJson,db); break;
        case TYPE_AP_UPLOAD_TESTING_INFO:    rc = ap_upload_testing_to_app(rootJson,db);break;
        case TYPE_DEV_UPDATE_CONFIG:         rc = m1_ap_update(rootJson);break;
        case TYPE_REPORT_VERSION_INFO:       rc = ap_report_version_handle(pdu);break;

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
    /*485设备操作*/
    app_conditioner_db_handle();

    sql_close();
    /*检查是否要更新应用程序*/
    m1_update_check();

    Finish:
    cJSON_Delete(rootJson);
    linkage_task();
}

static int common_rsp_handle(payload_t data)
{
    cJSON* resultJson = NULL;
    cJSON* pduTypeJson = NULL;
    M1_LOG_DEBUG("common_rsp_handle\n");
    if(data.pdu == NULL) return M1_PROTOCOL_FAILED;

    resultJson = cJSON_GetObjectItem(data.pdu, "result");
    M1_LOG_DEBUG("result:%d\n",resultJson->valueint);
    pduTypeJson = cJSON_GetObjectItem(data.pdu, "pduType");

    tcp_client_timeout_reset(pduTypeJson->valueint);

    return M1_PROTOCOL_OK;
}

/*AP report device data to M1*/
static int AP_report_data_handle(payload_t data)
{
    int i                  = 0;
    int j                  = 0;
    int number1            = 0;
    int number2            = 0;
    int rc                 = 0;
    int ret                = M1_PROTOCOL_OK;
    int sql_commit_flag    = 0;
    char* errorMsg         = NULL;
    char* sql              = NULL;
    char* sql_0_1          = NULL;
    char* sql_0_2          = NULL;
    /*sqlite3*/
    sqlite3* db            = NULL;
    sqlite3_stmt* stmt     = NULL;
    sqlite3_stmt* stmt_0_1 = NULL;
    sqlite3_stmt* stmt_0_2 = NULL;
    /*Json*/
    cJSON* devDataJson     = NULL;
    cJSON* devNameJson     = NULL;
    cJSON* devIdJson       = NULL;
    cJSON* paramJson       = NULL;
    cJSON* paramDataJson   = NULL;
    cJSON* typeJson        = NULL;
    cJSON* valueJson       = NULL;

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

    {
        sql_0_1 = "select ID from param_table where DEV_ID = ? and TYPE = ? limit 1;"; 
        M1_LOG_DEBUG("%s\n",sql_0_1);
        rc = sqlite3_prepare_v2(db, sql_0_1, strlen(sql_0_1), &stmt_0_1, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
            sql_error_set();   
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }
    }

    {
        sql_0_2 = "update param_table set VALUE = ?,TIME = datetime('now') where DEV_ID = ? and TYPE = ?;";
        M1_LOG_DEBUG("%s\n",sql_0_2);
        rc = sqlite3_prepare_v2(db, sql_0_2, strlen(sql_0_2), &stmt_0_2, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
            sql_error_set(); 
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();   
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }
    }

    {
        sql = "insert into param_table(DEV_NAME,DEV_ID,TYPE,VALUE)values(?,?,?,?);";
        rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
            sql_error_set(); 
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();   
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }
    }

    /*开启事物*/
    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        M1_LOG_DEBUG("BEGIN IMMEDIATE\n");
        sql_commit_flag = 1;

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
                
                /*先检查*/
                sqlite3_bind_text(stmt_0_1, 1, devIdJson->valuestring, -1, NULL);
                sqlite3_bind_int(stmt_0_1, 2,typeJson->valueint);
                rc = sqlite3_step(stmt_0_1);   
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
                
                sqlite3_reset(stmt_0_1); 
                sqlite3_clear_bindings(stmt_0_1);
                
                if(rc == SQLITE_ROW)
                {
                    /*更新*/
                    sqlite3_bind_int(stmt_0_2, 1, valueJson->valueint);
                    sqlite3_bind_text(stmt_0_2, 2, devIdJson->valuestring, -1, NULL);
                    sqlite3_bind_int(stmt_0_2, 3,typeJson->valueint);
                    rc = sqlite3_step(stmt_0_2);   
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

                    sqlite3_reset(stmt_0_2); 
                    sqlite3_clear_bindings(stmt_0_2);
                }
                else
                {
                    /*再插入*/
                    sqlite3_bind_text(stmt, 1,  devNameJson->valuestring, -1, NULL);
                    sqlite3_bind_text(stmt, 2, devIdJson->valuestring, -1, NULL);
                    sqlite3_bind_int(stmt, 3,typeJson->valueint);
                    sqlite3_bind_int(stmt, 4, valueJson->valueint);
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

                    sqlite3_reset(stmt); 
                    sqlite3_clear_bindings(stmt);

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
    if(sql_commit_flag)
    {
        rc = sql_commit(db);
        if(rc == SQLITE_OK)
        {
            M1_LOG_DEBUG("COMMIT OK\n");
        }
    }  

    if(stmt != NULL)
        sqlite3_finalize(stmt);  
    if(stmt_0_1 != NULL)
        sqlite3_finalize(stmt_0_1);
    if(stmt_0_2 != NULL)
        sqlite3_finalize(stmt_0_2);

    M1_LOG_DEBUG("AP_report_data_handle end\n");

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
    int sql_commit_flag  = 0;
    /*sql*/
    char *errorMsg       = NULL;
    char *sql            = NULL;
    char *sql_1          = NULL;
    char *sql_2          = NULL;
    sqlite3 *db          = NULL;
    sqlite3_stmt *stmt   = NULL;
    sqlite3_stmt *stmt_1 = NULL;
    sqlite3_stmt *stmt_2 = NULL;
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

    sql = "select ID from all_dev where DEV_ID = ? limit 1;";
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
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

    sql_1 = "update all_dev set AP_ID = ?,PID = ?, TIME = datetime('now') where DEV_ID = ?;";
    rc = sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
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

    sql_2 = "insert into all_dev(DEV_NAME, DEV_ID, AP_ID, PID, ADDED, NET, STATUS, ACCOUNT)values(?,?,?,?,?,?,?,?);";
    rc = sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
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

    /*事物开始*/
    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        M1_LOG_DEBUG("BEGIN IMMEDIATE\n");
        sql_commit_flag = 1;

        for(i = 0; i< number; i++)
        {
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
           
            /*查找数据库中是否存在*/
            sqlite3_bind_text(stmt, 1, idJson->valuestring, -1, NULL);
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
            sqlite3_reset(stmt); 
            sqlite3_clear_bindings(stmt);            
            if(rc == SQLITE_ROW)
            {
                sqlite3_bind_text(stmt_1, 1,apIdJson->valuestring, -1, NULL);
                sqlite3_bind_int(stmt_1, 2, pIdJson->valueint);
                sqlite3_bind_text(stmt_1, 3, idJson->valuestring, -1, NULL);
                rc = sqlite3_step(stmt_1);   
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
                sqlite3_reset(stmt_1); 
                sqlite3_clear_bindings(stmt_1);
            }
            else
            {
                sqlite3_bind_text(stmt_2, 1,  nameJson->valuestring, -1, NULL);
                sqlite3_bind_text(stmt_2, 2, idJson->valuestring, -1, NULL);
                sqlite3_bind_text(stmt_2, 3,apIdJson->valuestring, -1, NULL);
                sqlite3_bind_int(stmt_2, 4, pIdJson->valueint);
                sqlite3_bind_int(stmt_2, 5, 0);
                sqlite3_bind_int(stmt_2, 6, 1);
                sqlite3_bind_text(stmt_2, 7,"ON", -1, NULL);
                sqlite3_bind_text(stmt_2, 8,  "Dalitek", -1, NULL);

                rc = sqlite3_step(stmt_2);   
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
                sqlite3_reset(stmt_2); 
                sqlite3_clear_bindings(stmt_2);
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

    if(stmt != NULL)
        sqlite3_finalize(stmt);
    if(stmt_1 != NULL)
        sqlite3_finalize(stmt_1);
    if(stmt_2 != NULL)
        sqlite3_finalize(stmt_2);
    return ret;  
}

/*AP report information to M1*/
static int AP_report_ap_handle(payload_t data)
{
    int rc                 = 0;
    int ret                = M1_PROTOCOL_OK;
    int sql_commit_flag    = 0;
    int clientFd           = 0;
    char* sql              = NULL; 
    char* sql_0_1          = NULL;
    char* sql_0_2          = NULL;
    char* sql_1            = NULL;
    char* sql_1_1          = NULL;
    char* sql_1_2          = NULL;
    char* sql_2            = NULL;
    char* sql_2_1          = NULL;
    char* sql_2_2          = NULL;
    /*sql*/
    char* errorMsg         = NULL;
    sqlite3* db            = NULL;
    sqlite3_stmt* stmt     = NULL;
    sqlite3_stmt* stmt_0_1 = NULL;
    sqlite3_stmt* stmt_0_2 = NULL;
    sqlite3_stmt* stmt_1   = NULL;
    sqlite3_stmt* stmt_1_1 = NULL;
    sqlite3_stmt* stmt_1_2 = NULL;
    sqlite3_stmt* stmt_2   = NULL;
    sqlite3_stmt* stmt_2_1 = NULL;
    sqlite3_stmt* stmt_2_2 = NULL;
    /*Json*/
    cJSON* pIdJson         = NULL;
    cJSON* apIdJson        = NULL;
    cJSON* apNameJson      = NULL;

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
    
    /*更新conn_info表信息*/
    {
        //sql = "insert or replace into conn_info(AP_ID, CLIENT_FD)values(?,?);";
        sql = "insert into conn_info(AP_ID, CLIENT_FD)values(?,?);";
        rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            sql_error_set(); 
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }

        sql_0_1 = "select CLIENT_FD from conn_info where AP_ID = ?;";
        M1_LOG_DEBUG("%s\n",sql_0_1);
        rc = sqlite3_prepare_v2(db, sql_0_1, strlen(sql_0_1), &stmt_0_1, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
            sql_error_set();   
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }

        sql_0_2 = "update conn_info set CLIENT_FD = ? where AP_ID = ?;";
        M1_LOG_DEBUG("%s\n",sql_0_2);
        rc = sqlite3_prepare_v2(db, sql_0_2, strlen(sql_0_2), &stmt_0_2, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
            sql_error_set();   
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }
    }

    /*更新all_dev表信息*/
    {
        sql_1 = "select ID from all_dev where DEV_ID = ? limit 1;";
        rc = sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
            sql_error_set();   
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }

        sql_1_1 = "update all_dev set PID = ?, TIME = datetime('now') where DEV_ID = ?;";
        rc = sqlite3_prepare_v2(db, sql_1_1, strlen(sql_1_1), &stmt_1_1, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db)); 
            sql_error_set(); 
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle(); 
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }

        sql_1_2 = "insert into all_dev(DEV_NAME, DEV_ID, AP_ID, PID, ADDED, NET, STATUS, ACCOUNT)values(?,?,?,?,?,?,?,?);";
        rc = sqlite3_prepare_v2(db, sql_1_2, strlen(sql_1_2), &stmt_1_2, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db)); 
            sql_error_set(); 
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle(); 
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }
    }

    /*更新param_table表信息*/
    {
        {
            sql_2_1 = "select ID from param_table where DEV_ID = ? and TYPE = ? limit 1;"; 
            M1_LOG_DEBUG("%s\n",sql_2_1);   
            rc = sqlite3_prepare_v2(db, sql_2_1, strlen(sql_2_1), &stmt_2_1, NULL);
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
        }

        {
            sql_2_2 = "update param_table set VALUE = ?,TIME = datetime('now') where DEV_ID = ? and TYPE = ?;";
            M1_LOG_DEBUG("%s\n",sql_2_2);
            rc = sqlite3_prepare_v2(db, sql_2_2, strlen(sql_2_2), &stmt_2_2, NULL);
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
        }

        {
            sql_2 = "insert into param_table(DEV_ID, DEV_NAME, TYPE, VALUE)values(?,?,?,?);";
            rc = sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
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
        }
    }

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {   
        sql_commit_flag = 1;
        /*更新conn_info表信息*/
        {
            /*查询是否存在ap_id*/
            sqlite3_bind_text(stmt_0_1, 1, apIdJson->valuestring, -1, NULL);
            rc = sqlite3_step(stmt_0_1);   
            if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                sql_error_set();
                M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
                if(rc == SQLITE_CORRUPT)
                    m1_error_handle();
            }
            else
            {
                sql_error_clear();
            }

            if(rc == SQLITE_ROW)
            {
                clientFd = sqlite3_column_int(stmt_0_1, 0);
                if(clientFd != data.clientFd)
                {
                    sqlite3_bind_int(stmt_0_2, 1, data.clientFd);
                    sqlite3_bind_text(stmt_0_2, 2, apIdJson->valuestring, -1, NULL);
                    rc = sqlite3_step(stmt_0_2);   
                    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
                    {
                        sql_error_set();
                        M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
                        if(rc == SQLITE_CORRUPT)
                            m1_error_handle();
                    }
                    else
                    {
                        sql_error_clear();
                    }

                }   
            }
            else
            {
                sqlite3_bind_text(stmt, 1, apIdJson->valuestring, -1, NULL);
                sqlite3_bind_int(stmt, 2, data.clientFd);

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
            }         
        }

        /*查找all_dev表信息*/
        sqlite3_bind_text(stmt_1, 1, apIdJson->valuestring, -1, NULL);
        rc = sqlite3_step(stmt_1);   
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            sql_error_set();
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();
        }
        else
        {
            sql_error_clear();
        }
        if(rc == SQLITE_ROW)
        {
            /*更新all_dev表信息*/
            sqlite3_bind_int(stmt_1_1, 1, pIdJson->valueint);
            sqlite3_bind_text(stmt_1_1, 2, apIdJson->valuestring, -1, NULL);
            rc = sqlite3_step(stmt_1_1);   
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
        }
        else
        {
            /*插入新信息*/
            sqlite3_bind_text(stmt_1_2, 1,  apNameJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt_1_2, 2, apIdJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt_1_2, 3, apIdJson->valuestring, -1, NULL);
            sqlite3_bind_int(stmt_1_2, 4, pIdJson->valueint);
            sqlite3_bind_int(stmt_1_2, 5, 0);
            sqlite3_bind_int(stmt_1_2, 6, 1);
            sqlite3_bind_text(stmt_1_2, 7,  "ON", -1, NULL);
            sqlite3_bind_text(stmt_1_2, 8,  "Dalitek", -1, NULL);
        
            rc = sqlite3_step(stmt_1_2);   
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
        }

        /*先检查*/
        sqlite3_bind_text(stmt_2_1, 1, apIdJson->valuestring, -1, NULL);
        sqlite3_bind_int(stmt_2_1, 2, 16404);
        rc = sqlite3_step(stmt_2_1);   
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            sql_error_set();
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();
        }
        else
        {
            sql_error_clear();
        }
                
        if(rc == SQLITE_ROW)
        {
            /*更新*/
            sqlite3_bind_int(stmt_2_2, 1, 1);
            sqlite3_bind_text(stmt_2_2, 2, apIdJson->valuestring, -1, NULL);
            sqlite3_bind_int(stmt_2_2, 3, 16404);
            rc = sqlite3_step(stmt_2_2);   
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

        }
        else
        {
            /*更新param_table表信息*/
            sqlite3_bind_text(stmt_2, 1,  apIdJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt_2, 2,  apNameJson->valuestring, -1, NULL);
            sqlite3_bind_int(stmt_2, 3, 16404);
            sqlite3_bind_int(stmt_2, 4, 1);
    
            rc = sqlite3_step(stmt_2);   
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

    if(stmt != NULL)
        sqlite3_finalize(stmt);
    if(stmt_0_1 != NULL)
        sqlite3_finalize(stmt_0_1);
    if(stmt_0_2 != NULL)
        sqlite3_finalize(stmt_0_2);
    if(stmt_1 != NULL)
        sqlite3_finalize(stmt_1);
    if(stmt_1_1 != NULL)
        sqlite3_finalize(stmt_1_1);
    if(stmt_1_2 != NULL)
        sqlite3_finalize(stmt_1_2);
    if(stmt_2 != NULL)
        sqlite3_finalize(stmt_2);
    if(stmt_2 != NULL)
        sqlite3_finalize(stmt_2_1);
    if(stmt_2 != NULL)
        sqlite3_finalize(stmt_2_2);
    return ret;  
}

static int APP_read_handle(payload_t data)
{   
    int i                    = 0;
    int j                    = 0;
    int number1              = 0;
    int number2              = 0;
    int pduType              = TYPE_REPORT_DATA;
    int rc                   = 0;
    int ret                  = M1_PROTOCOL_OK;
    const char *devName      = NULL;
    const char *pId          = NULL;
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
    dev485Opt_t dev485cmd;    

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
        rc = sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            sql_error_set();
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }
    }

    {
        sql_1 = "select VALUE from param_table where DEV_ID = ? and TYPE = ? order by ID desc limit 1;";
        M1_LOG_DEBUG("%s\n", sql_1);
        rc = sqlite3_prepare_v2(db, sql_1, strlen(sql_1),&stmt_1, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            sql_error_set();
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        else
        {
            sql_error_clear();
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

        /*485设备参数读取*/
        {     
            dev485cmd.devId   = dev_id;
            dev485cmd.cmdType = DEV_READ_STATUS;
            dev_485_operate(dev485cmd);
        }

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
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            sql_error_set();
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();
        }
        else
        {
            sql_error_clear();
        }
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
    cJSON* dataArrayJson  = NULL;
    cJSON* devIdJson      = NULL;
    dev485Opt_t dev485cmd;
    
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
    /*485写入检查*/
    {
        dev485cmd.devId     = devIdJson->valuestring;
        dev485cmd.cmdType   = DEV_WRITE;
        dev485cmd.paramJson = dataArrayJson;
        rc = dev_485_operate(dev485cmd);
        if(rc == M1_PROTOCOL_OK)
            return M1_PROTOCOL_OK;
    }
    
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
    sqlite3_bind_text(stmt, 1, devIdJson->valuestring, -1, NULL);
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
    int sql_commit_flag   = 0;
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

    sql = "insert into param_table(DEV_NAME,DEV_ID,TYPE,VALUE)values((select DEV_NAME from all_dev where DEV_ID = ? limit 1),?,?,?);";
    M1_LOG_DEBUG("%s\n", sql);
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
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

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        M1_LOG_DEBUG("BEGIN IMMEDIATE\n");
        sql_commit_flag = 1;

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

                sqlite3_reset(stmt);   
                sqlite3_clear_bindings(stmt);
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
    int sql_commit_flag     = 0;
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

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        sql_commit_flag = 1;
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

            sqlite3_bind_int(stmt, 1, 1);
            sqlite3_bind_text(stmt, 2, "ON", -1, NULL);
            sqlite3_bind_text(stmt, 3, APIdJson->valuestring, -1, NULL);
            sqlite3_bind_text(stmt, 4, APIdJson->valuestring, -1, NULL);

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

                    sqlite3_bind_int(stmt, 1, 1);
                    sqlite3_bind_text(stmt, 2, "ON", -1, NULL);
                    sqlite3_bind_text(stmt, 3, devArrayJson->valuestring, -1, NULL);
                    sqlite3_bind_text(stmt, 4, APIdJson->valuestring, -1, NULL);
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

                    sqlite3_reset(stmt);   
                    sqlite3_clear_bindings(stmt);
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
    if(sql_commit_flag)
    {
        rc = sql_commit(db);
        if(rc == SQLITE_OK)
        {
            M1_LOG_DEBUG("COMMIT OK\n");
        }
    }

    if(stmt != NULL)
        sqlite3_finalize(stmt);
    return ret;
}

static int APP_req_added_dev_info_handle(payload_t data)
{
    int rc                       = 0;
    int ret                      = M1_PROTOCOL_OK;
    int pduType                  = TYPE_M1_REPORT_ADDED_INFO;
    /*rx data*/
    const char *account          = "Dalitek";
    /*tx data*/
    const unsigned char *apId    = NULL;
    const unsigned char *apName  = NULL;
    const unsigned char *devId   = NULL;
    const unsigned char *devName = NULL;
    int pId                      = 0;
    /*sql*/
    // char *sql               = NULL;
    char *sql_1                  = NULL;
    char *sql_2                  = NULL;
    sqlite3 *db                  = NULL;
    // sqlite3_stmt *stmt      = NULL;
    sqlite3_stmt *stmt_1         = NULL;
    sqlite3_stmt *stmt_2         = NULL;
    /*Json*/
    cJSON *devDataObject         = NULL;
    cJSON *devArray              = NULL;
    cJSON *devObject             = NULL;
    cJSON *pJsonRoot             = NULL;
    cJSON *devDataJsonArray      = NULL;
    cJSON *pduJsonObject         = NULL;

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
    #if 0
    sql = "select ACCOUNT from account_info where CLIENT_FD = ? order by ID desc limit 1;";
    M1_LOG_DEBUG( "%s\n", sql);
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db)); 
        if(rc == SQLITE_CORRUPT)
            m1_error_handle(); 
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    sqlite3_bind_int(stmt, 1, data.clientFd);
    rc = sqlite3_step(stmt);   
    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();
    }   
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
    #endif

    sql_1 = "select DEV_ID, DEV_NAME, PID from all_dev where DEV_ID = AP_ID and ACCOUNT = ?;";
    rc = sqlite3_prepare_v2(db, sql_1, strlen(sql_1),&stmt_1, NULL);
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

    sql_2 = "select DEV_ID, DEV_NAME, PID from all_dev where AP_ID = ? and AP_ID != DEV_ID and ACCOUNT = ?;";
    M1_LOG_DEBUG("sql_2:%s\n",sql_2);
    rc = sqlite3_prepare_v2(db, sql_2, strlen(sql_2),&stmt_2, NULL);
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

    //sqlite3_bind_text(stmt_1, 1, account, -1, NULL);
    sqlite3_bind_text(stmt_1, 1, "Dalitek", -1, NULL);
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
        sqlite3_reset(stmt_2);
        sqlite3_clear_bindings(stmt_2); 
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
#if 0
    if(stmt != NULL)
        sqlite3_finalize(stmt);
#endif
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
    dev485Opt_t dev485cmd;

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
#if 0
    /*搜索485空调设备在线状态*/
    {
        dev485cmd.devId   = apIdJson->valuestring;
        dev485cmd.cmdType = DEV_READ_ONLINE;
        rc = dev_485_operate(dev485cmd);
        if(rc == M1_PROTOCOL_OK)
            return M1_PROTOCOL_OK;
    }
#endif
    sql = "select CLIENT_FD from conn_info where AP_ID = ?;";
    M1_LOG_DEBUG("sql:%s\n",sql);
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

    sqlite3_bind_text(stmt, 1, apIdJson->valuestring, -1, NULL);

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

    int rc                       = 0;
    int ret                      = M1_PROTOCOL_OK;
    int pduType                  = TYPE_M1_REPORT_AP_INFO;
    /*Tx data*/
    const unsigned char *apId    = NULL;
    const unsigned char *apName  = NULL;
    const unsigned char *account = "Dalitek";
    int pId                      = 0;
    /*Json*/
    char *sql                    = NULL;
    cJSON *devDataObject         = NULL;
    cJSON *pJsonRoot             = NULL;
    cJSON *pduJsonObject         = NULL;
    cJSON *devDataJsonArray      = NULL;
    /*sql*/
    sqlite3 *db                  = NULL;
    sqlite3_stmt *stmt           = NULL;

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

    /*搜索485设备*/
    ret = app_gw_search();
    printf("app_gw_search :%d\n",ret);

    //sql = "select a.DEV_ID,a.DEV_NAME,a.PID from all_dev as a,account_info as b where a.DEV_ID = a.AP_ID and a.ACCOUNT = b.ACCOUNT and b.CLIENT_FD = ?;";
    sql = "select DEV_ID,DEV_NAME,PID from all_dev where DEV_ID = AP_ID and ACCOUNT = ?;";
    M1_LOG_DEBUG( "%s\n", sql);
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
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

    //sqlite3_bind_int(stmt, 1, data.clientFd);
    sqlite3_bind_text(stmt, 1, account, -1, NULL);
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
    int pduType                  = TYPE_M1_REPORT_DEV_INFO;
    int rc                       = 0;
    int ret                      = M1_PROTOCOL_OK;
    const char *apId             = NULL;
    /*Tx data*/
    const unsigned char *devId   = NULL;
    const unsigned char *devName = NULL;
    const unsigned char *account = "Dalitek";
    int pId                      = 0;
    /*Json*/
    cJSON *pJsonRoot             = NULL;
    cJSON *pduJsonObject         = NULL;
    cJSON *devDataJsonArray      = NULL;
    cJSON *devDataObject         = NULL;
    /*sql*/
    char *sql                    = NULL;
    sqlite3 *db                  = NULL;
    sqlite3_stmt *stmt           = NULL;

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

    sql = "select a.DEV_ID,a.DEV_NAME,a.PID from all_dev as a,account_info as b where a.DEV_ID != a.AP_ID and a.AP_ID = ? and a.ACCOUNT = ?;";
    M1_LOG_DEBUG("sql:%s", sql);
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
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

    sqlite3_bind_text(stmt, 1, apId, -1, NULL);
    //sqlite3_bind_int(stmt, 2, data.clientFd);
    sqlite3_bind_text(stmt, 2, account, -1, NULL);
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
    int rc                         = 0;
    int ret                        = M1_PROTOCOL_OK; 
    int sql_commit_flag            = 0;
    const unsigned char *scen_name = NULL;
    char *sql                      = (char*)malloc(300);
    char *errorMsg                 = NULL;
    cJSON *typeJson                = NULL;
    cJSON *idJson                  = NULL;
    cJSON *operateJson             = NULL;    
    sqlite3 *db                    = NULL;
    sqlite3_stmt *stmt             = NULL;

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
        sql_commit_flag = 1;

        if(strcmp(typeJson->valuestring, "device") == 0){
            if(strcmp(operateJson->valuestring, "delete") == 0){
                /*通知到ap*/
                if(m1_del_dev_from_ap(db, idJson->valuestring) != M1_PROTOCOL_OK)
                    M1_LOG_ERROR("m1_del_dev_from_ap error\n");
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
                
                rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
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

    free(sql);
    if(stmt != NULL)
        sqlite3_finalize(stmt);

    return ret;
}

/*查询离线设备*/
static void check_offline_dev(sqlite3*db)
{
    M1_LOG_DEBUG("check_offline_dev\n");
    int rc                    = 0;
    int sql_commit_flag       = 0;
    const unsigned char *time = NULL;
    int curTime               = 0;
    int sqlTime               = 0;
    int interval              = 0;
    int clientFd              = 0;
    const unsigned char* apId = NULL;
    char* errorMsg            = NULL;
    char* sql                 = NULL;
    char* sql_1               = NULL;
    char* sql_2               = NULL;
    char* sql_3               = NULL;
    char* sql_4_0             = NULL;
    char* sql_4               = NULL;
    char* sql_5               = NULL;
    char* sql_5_1             = NULL;
    sqlite3_stmt* stmt        = NULL;
    sqlite3_stmt* stmt_1      = NULL;
    sqlite3_stmt* stmt_2      = NULL;
    sqlite3_stmt* stmt_3      = NULL;
    sqlite3_stmt* stmt_4_0    = NULL;
    sqlite3_stmt* stmt_4      = NULL;
    sqlite3_stmt* stmt_5      = NULL;
    sqlite3_stmt* stmt_5_1    = NULL;

    /*获取AP ID*/
    sql = "select AP_ID from all_dev where DEV_ID = AP_ID;";
    M1_LOG_DEBUG("string:%s\n",sql);
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
        sql_error_set();
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();  
        goto Finish; 
    }
    else
    {
        sql_error_clear();
    }

    /*查询数据库时间*/
    sql_1 = "select TIME from param_table where DEV_ID = ? and TYPE = 16404 order by ID desc limit 1;";
    M1_LOG_DEBUG("string:%s\n",sql_1);
    
    rc = sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));
        sql_error_set();
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();  
        goto Finish; 
    }
    else
    {
        sql_error_clear();
    }
    /*当前时间格式化*/
    sql_2 = "select strftime('%s','now');";
    M1_LOG_DEBUG("%s\n",sql_2);
            
    rc = sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db)); 
        sql_error_set();
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();   
        goto Finish; 
    }
    else
    {
        sql_error_clear();
    }
    /*数据库时间格式化*/
    sql_3 = "select strftime('%s',?);";
    M1_LOG_DEBUG("%s\n",sql_3);
            
    rc = sqlite3_prepare_v2(db, sql_3, strlen(sql_3), &stmt_3, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db)); 
        sql_error_set(); 
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();   
        goto Finish; 
    }
    else
    {
        sql_error_clear();
    }

     /*更新AP在线状态*/
    {
        sql_4_0 = "select ID from param_table where VALUE = ? and DEV_ID = ? and TYPE = ?;";
        rc = sqlite3_prepare_v2(db, sql_4_0, strlen(sql_4_0), &stmt_4_0, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            sql_error_set();
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();   
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }
    }

    /*更新AP在线状态*/
    {
        sql_4 = "update param_table set VALUE = ? where DEV_ID = ? and TYPE = ?;";
        rc = sqlite3_prepare_v2(db, sql_4, strlen(sql_4), &stmt_4, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            sql_error_set();
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();   
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }
    }

    /*删除离线设备相关资源*/
    {
        sql_5 = "select CLIENT_FD from conn_info where AP_ID = ?;";
        rc = sqlite3_prepare_v2(db, sql_5, strlen(sql_5), &stmt_5, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db)); 
            sql_error_set(); 
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();   
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }
    }
    /*从conn_info中删除ap相关资源*/
    {
        sql_5_1 = "delete from conn_info where CLIENT_FD = ?;";
        rc = sqlite3_prepare_v2(db, sql_5_1, strlen(sql_5_1), &stmt_5_1, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db)); 
            sql_error_set(); 
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();   
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }
    }

    {
        /*当前时间格式化*/
        rc = sqlite3_step(stmt_2);
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            sql_error_set();
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();
        }
        else
        {
            sql_error_clear();
        }

        curTime = sqlite3_column_int(stmt_2, 0);
        M1_LOG_DEBUG("curTime:%x\n",curTime);
    }

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        M1_LOG_DEBUG("BEGIN IMMEDIATE:\n");
        sql_commit_flag = 1;

        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            apId = sqlite3_column_text(stmt, 0);
            /*检查时间*/
            sqlite3_bind_text(stmt_1, 1, apId, -1, NULL);
            rc = sqlite3_step(stmt_1); 
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

            if(rc != SQLITE_ROW)
            {
                M1_LOG_WARN("rc != SQLITE_ROW\n");
                continue;
            }
            time = sqlite3_column_text(stmt_1, 0);
            if(time == NULL)
            {
                M1_LOG_DEBUG("time == NULL\n");
                continue;
            }
            M1_LOG_DEBUG("time: %s\n",time);
           
            {
                /*数据库时间格式化*/
                sqlite3_bind_text(stmt_3, 1, time, -1, NULL);
                rc = sqlite3_step(stmt_3);  
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

                sqlTime = sqlite3_column_int(stmt_3, 0);
                M1_LOG_DEBUG("sqlTime:%x\n",sqlTime);
            }
            interval = curTime - sqlTime;
            interval = abs(interval);
            M1_LOG_DEBUG("interval:%x\n",interval);
            if(interval > AP_HEART_BEAT_INTERVAL)
            {
                /*查看离线状态*/
                sqlite3_bind_int(stmt_4_0, 1, 0x01);
                sqlite3_bind_text(stmt_4_0, 2, apId, -1, NULL);
                sqlite3_bind_int(stmt_4_0, 3, 16404);
                rc = sqlite3_step(stmt_4_0);  
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

                if(rc == SQLITE_ERROR)
                {
                    M1_LOG_WARN("update failed\n");
                    continue;
                }
                sqlite3_reset(stmt_4_0);
                sqlite3_clear_bindings(stmt_4_0); 

                if(rc == SQLITE_ROW)
                {
                    /*更新离线状态*/
                    sqlite3_bind_int(stmt_4, 1, 0);
                    sqlite3_bind_text(stmt_4, 2, apId, -1, NULL);
                    sqlite3_bind_int(stmt_4, 3, 16404);
                    rc = sqlite3_step(stmt_4);  
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
                    if(rc == SQLITE_ERROR)
                    {
                        M1_LOG_WARN("update failed\n");
                        continue;
                    }
                    sqlite3_reset(stmt_4);
                    sqlite3_clear_bindings(stmt_4);    

                    /*删除离线设备相关资源*/
                    sqlite3_bind_text(stmt_5, 1, apId, -1, NULL);
                    rc = sqlite3_step(stmt_5);  
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

                    if(rc == SQLITE_ERROR)
                    {
                        M1_LOG_WARN("update failed\n");
                        continue;
                    }
                    if(rc == SQLITE_ROW)
                    {
                        clientFd = sqlite3_column_int(stmt_5, 0);
                        {
                            /*删除conn_info中ap相关*/
                            sqlite3_bind_int(stmt_5_1, 1, clientFd);
                            rc = sqlite3_step(stmt_5_1);  
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
                            if(rc == SQLITE_ERROR)
                            {
                                M1_LOG_WARN("update failed\n");
                                continue;
                            }
                            sqlite3_reset(stmt_5_1);
                            sqlite3_clear_bindings(stmt_5_1); 
                        }
                        client_block_destory(clientFd);
                        M1_LOG_WARN("delete socket ++\n");
                        deleteSocketRec(clientFd);
                        M1_LOG_WARN("delete socket --\n");
                    }
                    sqlite3_reset(stmt_5);
                    sqlite3_clear_bindings(stmt_5);  
                }  
            }
            sqlite3_reset(stmt_1);
            sqlite3_clear_bindings(stmt_1);
            sqlite3_reset(stmt_3);
            sqlite3_clear_bindings(stmt_3);
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
    if(stmt_3)
        sqlite3_finalize(stmt_3);
    if(stmt_4)
        sqlite3_finalize(stmt_4);
    if(stmt_4_0)
        sqlite3_finalize(stmt_4_0);
    if(stmt_5)
        sqlite3_finalize(stmt_5);
    if(stmt_5_1)
        sqlite3_finalize(stmt_5_1);
}

static int ap_heartbeat_handle(payload_t data)
{
    M1_LOG_DEBUG("ap_heartbeat_handle\n");
    int rc              = 0;
    int ret             = M1_PROTOCOL_OK;
    int sql_commit_flag = 0;
    int valueType       = 0x4014;
    char* errorMsg      = NULL;
    char* sql           = NULL;
    cJSON* apIdJson     = NULL;
    sqlite3* db         = NULL;
    sqlite3_stmt* stmt  = NULL;

    if(data.pdu == NULL){
        M1_LOG_ERROR("data.pdu\n");
        goto Finish;
    }

    db = data.db;
    apIdJson = data.pdu;

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        sql_commit_flag = 1;
        sql = "update param_table set VALUE = ?,TIME = datetime('now') where DEV_ID = ? and TYPE = ?;";
        M1_LOG_DEBUG("string:%s\n",sql);

        rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            sql_error_set();
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }

        M1_LOG_DEBUG("string:%s\n",apIdJson->valuestring);
        sqlite3_bind_int(stmt, 1, 1);
        sqlite3_bind_text(stmt, 2, apIdJson->valuestring, -1, NULL);
        sqlite3_bind_int(stmt, 3, valueType);

        rc = sqlite3_step(stmt);   
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            sql_error_set();
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();
        }
        else
        {
            sql_error_clear();
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

int delete_account_conn_info(int clientFd)
{
    int rc              = 0;
    int sqlFlag         = 0;
    int ret             = M1_PROTOCOL_OK;
    char *errorMsg      = NULL;
    char *sql           = NULL;
    sqlite3_stmt* stmt  = NULL;


    sql = "delete from conn_info where CLIENT_FD = ?;";

    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_WARN( "sqlite3_prepare_v2:error %s, num:%d\n", sqlite3_errmsg(db), rc); 
        //sql_error_set();
        if(rc == SQLITE_CORRUPT)
            m1_error_handle(); 
        // ret = M1_PROTOCOL_FAILED;
        // goto Finish; 
    }
    // else
    // {
    //     sql_error_clear();
    // }

    /*打开数据库*/
    if(rc == SQLITE_MISUSE)
    {
        rc = sql_open();
        if( rc != SQLITE_OK)
        {  
            M1_LOG_ERROR( "Can't open database\n");  
            goto Finish;
        }
        else
        {  
            M1_LOG_DEBUG( "Opened database successfully\n");  
        }

        sqlFlag = 1;
        
        rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s, num:%d\n", sqlite3_errmsg(db), rc); 
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

    }

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)==SQLITE_OK)
    {
        
        sqlite3_bind_int(stmt, 1, clientFd);

        M1_LOG_DEBUG("string:%s\n",sql);
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
            
        if(stmt)
        {
            stmt = NULL;
            sqlite3_finalize(stmt);
        }
        

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
        ret = M1_PROTOCOL_FAILED;
    }

    M1_LOG_INFO("delete from conn_info where CLIENT_FD = %d\n", clientFd);
    Finish:

    /*数据库open的情况下关闭*/
    if(sqlFlag == 1)
        sql_close();
    if(stmt)
        sqlite3_finalize(stmt);

    return ret;
}

/*删除用户登录历史数据*/
void delete_client_db(void)
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
    int sql_commit_flag  = 0;
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
        sql_commit_flag = 1;

        sql = "update all_dev set DEV_NAME = ? where DEV_ID = ?;";
        M1_LOG_DEBUG("sql:%s\n",sql);

        rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
        if(rc != SQLITE_OK)
        {
            M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
            sql_error_set();
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();
            ret = M1_PROTOCOL_FAILED;
            goto Finish; 
        }
        else
        {
            sql_error_clear();
        }

        sqlite3_bind_text(stmt, 1, devNameObject->valuestring, -1, NULL);
        sqlite3_bind_text(stmt, 2, devIdObject->valuestring, -1, NULL);

        rc = sqlite3_step(stmt);   
        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
        {
            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
            sql_error_set();
            if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                m1_error_handle();
        }
        else
        {
            sql_error_clear();
        }

        if(rc == SQLITE_ERROR)
        {
            ret = M1_PROTOCOL_FAILED;
            goto Finish;   
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

    return ret;
}

void getNowTime(char* _time)
{

    struct tm nowTime;

    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);  //获取相对于1970到现在的秒数
    localtime_r(&time.tv_sec, &nowTime);
    
    sprintf(_time, "%04d%02d%02d%02d%02d%02d", nowTime.tm_year + 1900, nowTime.tm_mon+1, nowTime.tm_mday, \
      nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec);
}

void setLocalTime(char* time)
{
    struct tm local_tm;
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

void delay_send(char* d, int delay, int clientFd)
{
    Item item;

    item.data = d;
    item.prio = delay;
    item.clientFd = clientFd;
    Push(&head, item);
}

void* delay_send_task(void)
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
#if 1
        if(!(count % 300))
        {
            /*查看连接是否超时*/
            tcp_client_timeout_tick(1);
            /*M1心跳到云端*/
            m1_heartbeat_to_cloud();
        }
#endif
    }
}

int sql_exec(sqlite3* db, char*sql)
{
    sqlite3_stmt* stmt = NULL;
    int rc;
    /*get id*/
    rc = sqlite3_prepare_v2(db, sql, strlen(sql), & stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        sql_error_set();
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();
        goto Finish; 
    }
    else
    {
        sql_error_clear();
    }

    do{
        rc = sqlite3_step(stmt);
    }while(rc == SQLITE_ROW);
   
    Finish:
    if(stmt)
        sqlite3_finalize(stmt);
    return rc;
}

/*打开数据库*/
int sql_open(void)
{
    //static int sql_open_once = 0;
    int rc = SQLITE_OK;

    // if(sql_open_once != 0)
    //     return SQLITE_OK;

    // sql_open_once = 1;

    pthread_mutex_lock(&mutex_lock);
    M1_LOG_DEBUG( "pthread_mutex_lock\n");

    //rc = sqlite3_open(db_path, &db);
    rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX | SQLITE_OPEN_SHAREDCACHE, NULL);
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
    //return SQLITE_OK;
    int rc;
    
    M1_LOG_DEBUG( "Sqlite3 close\n");
    rc = sqlite3_close(db);

    pthread_mutex_unlock(&mutex_lock);
    M1_LOG_DEBUG( "pthread_mutex_unlock\n");

    return rc;
}

int sql_commit(sqlite3* db)
{
    int rc         = 0;
    char* errorMsg = NULL;

    rc = sqlite3_exec(db, "COMMIT", NULL, NULL, &errorMsg);
    if(rc != SQLITE_OK)
    {
        if(rc == SQLITE_BUSY)
        {
            M1_LOG_WARN("等待再次提交\n");
        }
        else
        {
            M1_LOG_WARN("COMMIT errorMsg:%s\n",errorMsg);
            sqlite3_free(errorMsg);
        }
    }

    return rc;
}

void m1_error_handle(void)
{
    sql_close();
    system("/bin/sql_restore.sh");
    exit(12);
}

/*累计sqlite3操作错误*/
void sql_error_set(void)
{
    sql_error_flag++;
    if(sql_error_flag > 20)
    {
        m1_error_handle();
    }
}
/*清除sqlite3操作错误*/
void sql_error_clear(void)
{
    sql_error_flag = 0;
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
        // sql = "CREATE UNIQUE INDEX appAcount ON account_info (\"ACCOUNT\" ASC);";
        // M1_LOG_DEBUG("%s:\n",sql);
        // rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        // if(rc != SQLITE_OK)
        // {
        //     M1_LOG_WARN("CREATE UNIQUE INDEX appAcount failed: %s\n",errmsg);
        //     sqlite3_free(errmsg);
        // }
        /*client_fdindex*/
        // sql = "CREATE UNIQUE INDEX appClient ON account_info (\"CLIENT_FD\" ASC);";
        // M1_LOG_DEBUG("%s:\n",sql);
        // rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        // if(rc != SQLITE_OK)
        // {
        //     M1_LOG_WARN("delete from account_info failed: %s\n",errmsg);
        //     sqlite3_free(errmsg);
        // }
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
        // sql = "CREATE UNIQUE INDEX userAccount ON account_table (\"ACCOUNT\" ASC);";
        // M1_LOG_DEBUG("%s:\n",sql);
        // rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        // if(rc != SQLITE_OK)
        // {
        //     M1_LOG_WARN("delete from account_info failed: %s\n",errmsg);
        //     sqlite3_free(errmsg);
        // }
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
        //sql = "CREATE UNIQUE INDEX userAccRecord ON all_dev (\"DEV_ID\" ASC,\"ACCOUNT\" ASC,\"AP_ID\" ASC);";
        // sql = "CREATE UNIQUE INDEX userAccRecord ON all_dev (\"DEV_ID\" ASC,\"ACCOUNT\" ASC);";
        // M1_LOG_DEBUG("%s:\n",sql);
        // rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        // if(rc != SQLITE_OK)
        // {
        //     sqlite3_free(errmsg);
        //     M1_LOG_WARN("CREATE UNIQUE INDEX: %s\n",errmsg);
        // }              
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
        // sql = "CREATE UNIQUE INDEX apAccount ON conn_info (\"AP_ID\" ASC);";
        // M1_LOG_DEBUG("%s:\n",sql);
        // rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        // if(rc != SQLITE_OK)
        // {
        //     sqlite3_free(errmsg);
        //     M1_LOG_WARN("CREATE UNIQUE INDEX: %s\n",errmsg);
        // }
        /*CLIENT_FD index*/
        // sql = "CREATE UNIQUE INDEX apClient ON conn_info (\"CLIENT_FD\" ASC);";
        // M1_LOG_DEBUG("%s:\n",sql);
        // rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        // if(rc != SQLITE_OK)
        // {
        //     sqlite3_free(errmsg);
        //     M1_LOG_WARN("CREATE UNIQUE INDEX: %s\n",errmsg);
        // }
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
        // sql = "CREATE UNIQUE INDEX district ON district_table (\"DIS_NAME\" ASC, \"AP_ID\" ASC, \"ACCOUNT\" ASC);";
        // M1_LOG_DEBUG("%s:\n",sql);
        // rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        // if(rc != SQLITE_OK)
        // {
        //     sqlite3_free(errmsg);
        //     M1_LOG_WARN("CREATE INDEX district: %s\n",errmsg);
        // }   
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
        #if 0  
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
        #endif
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
        #if 0
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
        #endif
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
        #if 0
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
        #endif
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
        #if 0
        /*DEV_ID,DEV_NAME,TYPE UNION index*/
        sql = "CREATE UNIQUE INDEX param ON param_table (\"DEV_ID\" ASC,\"DEV_NAME\" ASC,\"TYPE\" ASC);";
        sql = "CREATE UNIQUE INDEX param ON param_table (\"DEV_ID\" ASC,\"TYPE\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("CREATE UNIQUE INDEX: %s\n",errmsg);
        }
        #endif
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
        #if 0
        sql = "CREATE UNIQUE INDEX scenAlarm ON scen_alarm_table (\"SCEN_NAME\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("CREATE UNIQUE INDEX scenAlarm: %s\n",errmsg);
        }
        #endif
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
        #if 0
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
        #endif
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
        else
        {
            
            mac_addr = get_eth0_mac_addr();
            sql = "insert into project_table(P_NAME, P_NUMBER, P_CREATOR, P_MANAGER, P_EDITOR, P_TEL, P_ADD, P_BRIEF, P_KEY, ACCOUNT)values(?,?,?,?,?,?,?,?,?,?);";
            rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
            if(rc != SQLITE_OK)
            {
                M1_LOG_ERROR( "sqlite3_prepare_v2:error %s, rc:%d\n", sqlite3_errmsg(db),rc);  
                sql_error_set();
                if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                    m1_error_handle();
                ret = M1_PROTOCOL_FAILED;
                goto Finish; 
            }
            else
            {
                sql_error_clear();
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
            if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
            {
                M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
                sql_error_set();
                if(rc == SQLITE_CORRUPT || rc == SQLITE_NOTADB)
                    m1_error_handle();
            }
            else
            {
                sql_error_clear();
            }
            
            if(rc == SQLITE_ERROR)
            {
                ret = M1_PROTOCOL_FAILED;
                goto Finish;   
            }
    
        }
        #if 0
        /*P_NUMBER,P_EDITOR UNION index*/
        sql = "CREATE UNIQUE INDEX project ON project_table (\"P_NUMBER\" ASC,\"P_EDITOR\" ASC);";
        M1_LOG_DEBUG("%s:\n",sql);
        rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
        if(rc != SQLITE_OK)
        {
            sqlite3_free(errmsg);
            M1_LOG_WARN("CREATE UNIQUE INDEX project: %s\n",errmsg);
        } 
        #endif       
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
    #if 0
    /*P_NUMBER,P_EDITOR UNION index*/
    sql = "CREATE UNIQUE INDEX paramDetail ON param_detail_table (\"DEV_ID\" ASC,\"TYPE\" ASC,\"DESCRIP\" ASC);";
    M1_LOG_DEBUG("%s:\n",sql);
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if(rc != SQLITE_OK)
    {
        sqlite3_free(errmsg);
        M1_LOG_WARN("CREATE UNIQUE INDEX paramDetail: %s\n",errmsg);
    }
    #endif
    /*version_table*/
    sql = "CREATE TABLE version_table (                        \
                ID        INTEGER PRIMARY KEY AUTOINCREMENT,   \
                DEV_ID    TEXT,                                \
                VERSION   TEXT,                                \
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

    /*插入Dalitek账户*/
    sql = "delete from account_table where ACCOUNT = \"Dalitek\";";
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if(rc != SQLITE_OK)
    {
        M1_LOG_WARN("insert into account_table fail: %s\n",errmsg);
        sqlite3_free(errmsg);
    }

    sql = "insert into account_table(ACCOUNT, KEY, KEY_AUTH, REMOTE_AUTH)values(\"Dalitek\",\"root\",\"on\",\"on\");";
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if(rc != SQLITE_OK)
    {
        M1_LOG_WARN("insert into account_table fail: %s\n",errmsg);
        sqlite3_free(errmsg);
    } 
    

    Finish:
    if(stmt)
        sqlite3_finalize(stmt);
    sqlite3_close(db);

    return ret;
}


