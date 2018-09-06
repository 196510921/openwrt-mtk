#include "m1_cloud.h"
#include "m1_protocol.h"
#include "socket_server.h"
#include "m1_common_log.h"
#include "tcp_client.h"

void m1_report_id_to_cloud(int clientFd)
{
	M1_LOG_INFO("m1_report_id_to_cloud\n");

    int sn = 2;
    int ret = M1_PROTOCOL_OK;
    int pduType = TYPE_M1_REPORT_ID_TO_CLOUD;
    char* mac_addr = get_eth0_mac_addr();
    cJSON* pJsonRoot = NULL;
    cJSON* pduJsonObject = NULL;
    cJSON* devDataJson = NULL;
    
    /*get sql data json*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_DEBUG("pJsonRoot NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    cJSON_AddNumberToObject(pJsonRoot, "sn", 1);
    cJSON_AddStringToObject(pJsonRoot, "version", "1.0");
    cJSON_AddNumberToObject(pJsonRoot, "netFlag", 2);
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
    devDataJson = cJSON_CreateObject();
    if(NULL == devDataJson)
    {
        cJSON_Delete(devDataJson);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataJson);
    cJSON_AddStringToObject(devDataJson, "proId", mac_addr);

    char * p = cJSON_PrintUnformatted(pJsonRoot);
        
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;  
    }

    M1_LOG_DEBUG("string:%s\n",p);
    /*response to client*/
    //socketSeverSend((uint8*)p, strlen(p), clientFd);
    socket_client_tx((uint8*)p, strlen(p));
    Finish:
    cJSON_Delete(pJsonRoot);
    return ret;
}

void m1_heartbeat_to_cloud(void)
{

    M1_LOG_INFO("m1_heartbeat_to_cloud\n");

    int sn = 2;
    int clientFd = 0;
    int ret = M1_PROTOCOL_OK;
    int pduType = TYPE_M1_HEARTBEAT_TO_CLOUD;
    char curTime[30];
    cJSON* pJsonRoot = NULL;
    cJSON* pduJsonObject = NULL;
    cJSON* devDataJson = NULL;
 
    if(get_connect_flag() == TCP_DISCONNECTED)
        return;

    /*get sql data json*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_DEBUG("pJsonRoot NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    cJSON_AddNumberToObject(pJsonRoot, "sn", 1);
    cJSON_AddStringToObject(pJsonRoot, "version", "1.0");
    cJSON_AddNumberToObject(pJsonRoot, "netFlag", 2);
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
    devDataJson = cJSON_CreateObject();
    if(NULL == devDataJson)
    {
        cJSON_Delete(devDataJson);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataJson);
    getNowTime(curTime);
    cJSON_AddStringToObject(devDataJson, "time", curTime);

    char * p = cJSON_PrintUnformatted(pJsonRoot);
        
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;  
    }

    M1_LOG_DEBUG("string:%s\n",p);
    /*response to client*/
    socket_client_tx((uint8*)p, strlen(p));

    Finish:
    cJSON_Delete(pJsonRoot);
    return ret;
}



