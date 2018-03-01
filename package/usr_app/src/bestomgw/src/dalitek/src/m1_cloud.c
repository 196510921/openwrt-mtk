#include "m1_cloud.h"
#include "m1_protocol.h"
#include "socket_server.h"

void m1_report_id_to_cloud(void)
{
	M1_LOG_INFO("m1_report_id_to_cloud\n");

    int sn = 2;
    int row_n;
    int clientFd;
    int rc,ret = M1_PROTOCOL_OK;
    const char* mac_addr = get_eth0_mac_addr();
    char* sql = (char*)malloc(300);
    sqlite3_stmt* stmt = NULL,*stmt_1 = NULL;
    cJSON* snJson = NULL;
    cJSON* pduJson = NULL;
    cJSON* devDataJson = NULL;
    cJSON* dataArrayJson = NULL;
    cJSON* devIdJson = NULL;
    
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
    pduJson = cJSON_GetObjectItem(data, "pdu");
    devDataJson = cJSON_GetObjectItem(pduJson, "devData");
    dataArrayJson = cJSON_GetArrayItem(devDataJson, 0);
    devIdJson = cJSON_GetObjectItem(dataArrayJson, "devId");
    M1_LOG_DEBUG("devId:%s\n",devIdJson->valuestring);
    /*get apId*/
    sprintf(sql,"select AP_ID from all_dev where DEV_ID = \"%s\" order by ID desc limit 1;",devIdJson->valuestring);
    if(sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2 failed\n");  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    rc = thread_sqlite3_step(&stmt,db);
    if(rc == SQLITE_ROW){
        ap_id = sqlite3_column_text(stmt,0);
        if(ap_id == NULL){
            M1_LOG_ERROR( "ap_id NULL\n");  
            ret = M1_PROTOCOL_FAILED;
            goto Finish;
        }
        M1_LOG_DEBUG("ap_id%s\n",ap_id);
    }else{
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    /*get clientFd*/
    sprintf(sql,"select CLIENT_FD from conn_info where AP_ID = \"%s\" order by ID desc limit 1;",ap_id);
    if(sqlite3_prepare_v2(db, sql, strlen(sql),&stmt_1, NULL) != SQLITE_OK){
        M1_LOG_ERROR( "sqlite3_prepare_v2 failed\n");  
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    rc = thread_sqlite3_step(&stmt_1, db);
    if(rc == SQLITE_ROW){
        clientFd = sqlite3_column_int(stmt_1,0);

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
    free(sql);
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_1);

    return ret;

}


