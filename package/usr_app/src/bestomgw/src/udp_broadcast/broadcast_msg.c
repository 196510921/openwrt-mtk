#include <net/if.h> 
#include <arpa/inet.h> 
#include <sys/types.h>  
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include<sys/ioctl.h>
#include "broadcast_msg.h"
#include "cJSON.h"

/*request message*/
typedef struct 
{
    cJSON* project;     //项目名称
} broadcast_req_ip_t;

typedef struct 
{
    cJSON*  pduType;  //命令类型
    cJSON*  devData;  //payload
}udp_req_pdu_t;

typedef struct 
{
    cJSON*  sn;        //序列号
    cJSON*  version;   //协议版本号
    cJSON*  netFlag;   //网络标识
    cJSON*  cmdType;   //网络标识
    cJSON*  pdu;       //payload
}udp_req_msg_t;

/*response message*/
typedef struct 
{
    char* project;     //项目名称
    char* ip;          //主机IP
} broadcast_rsp_ip_t;

typedef struct 
{
    int  pduType;  //命令类型
    cJSON*  devData;  //payload
}udp_rsp_pdu_t;

typedef struct 
{
    int    sn;        //序列号
    char*  version;   //协议版本号
    int    netFlag;   //网络标识
    int    cmdType;   //网络标识
    cJSON*  pdu;       //payload
}udp_rsp_msg_t;


static int project_check(char* project);

int broadcast_msg_proc(char* msg_in, char* msg_out, int* len, int sockfd)
{
    if(msg_in == NULL || msg_out == NULL || len == NULL)
        return -1;
    
    int ret         = 0;
    char * p        = NULL;
    cJSON* req_json = NULL;
    cJSON* rsp_json = NULL;
    /*rx*/
    udp_req_msg_t req_msg;
    udp_req_pdu_t req_pdu;
    broadcast_req_ip_t req_data;
    /*tx*/
    udp_rsp_msg_t rsp_msg;
    udp_rsp_pdu_t rsp_pdu;
    broadcast_rsp_ip_t rsp_data;
    struct ifreq ifr;

    /*json*/
    req_json = cJSON_Parse(msg_in);
    if(NULL == req_json)
    {
        printf("rootJson null\n");
        ret = -1;
        goto Finish;   
    }
    /*pdu json*/
    req_msg.pdu = cJSON_GetObjectItem(req_json, "pdu");
    if(NULL == req_msg.pdu)
    {
        printf("pdu null\n");
        ret = -1;
        goto Finish;
    }
    /*devData json*/
    req_pdu.devData = cJSON_GetObjectItem(req_msg.pdu, "devData");
    if(NULL == req_pdu.devData)
    {
        printf("devData null\n");
        ret = -1;
        goto Finish;
    }
    /*project*/
    req_data.project = cJSON_GetObjectItem(req_pdu.devData, "project");
    if(NULL == req_data.project)
    {
        printf("pdu null\n");
        ret = -1;
        goto Finish;
    }
    /*判断是否为本项目名称*/
    if(project_check(req_data.project->valuestring) == -1)
    {
        ret = -1;
        goto Finish; 
    }

    /*封装回复包*/
    rsp_json = cJSON_CreateObject();
    if(NULL == rsp_json)
    {
        printf("rsp_json NULL\n");
        ret = -1;
        goto Finish;
    }

    {
        /*sn,version,netFlag,cmdType*/
        rsp_msg.sn = 1;
        rsp_msg.version = "1.0";
        rsp_msg.netFlag = 2;
        rsp_msg.cmdType = 3;
        /*pdu*/
        rsp_msg.pdu = cJSON_CreateObject();
        if(NULL == rsp_msg.pdu)
        {
            printf("rsp_msg.pdu NULL\n");
            cJSON_Delete(rsp_msg.pdu);
            ret = -1;
            goto Finish;
        }
        /*pduType*/
        rsp_pdu.pduType = 0x1001;
        /*devData*/
        rsp_pdu.devData = cJSON_CreateObject();
        if(NULL == rsp_pdu.devData)
        {
            printf("rsp_pdu.devData NULL\n");
            cJSON_Delete(rsp_pdu.devData);
            ret = -1;
            goto Finish;
        }
        /*project*/
        rsp_data.project = "DOM100";
        /*获取主机IP*/
        strcpy(ifr.ifr_name,"br-lan");
        if(ioctl(sockfd,SIOCGIFADDR,&ifr) != 0)
        perror("ioctl");
        printf("host:%s\n",(char *)inet_ntoa( ((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr));
        rsp_data.ip = (char *)inet_ntoa( ((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr);
    }

    {
        cJSON_AddNumberToObject(rsp_json, "sn", rsp_msg.sn);
        cJSON_AddStringToObject(rsp_json, "version", rsp_msg.version);
        cJSON_AddNumberToObject(rsp_json, "netFlag", rsp_msg.netFlag);
        cJSON_AddNumberToObject(rsp_json, "cmdType", rsp_msg.cmdType);
        cJSON_AddItemToObject(rsp_json, "pdu", rsp_msg.pdu);
        cJSON_AddNumberToObject(rsp_msg.pdu, "pduType", rsp_pdu.pduType);
        cJSON_AddItemToObject(rsp_msg.pdu, "devData", rsp_pdu.devData);
        cJSON_AddStringToObject(rsp_pdu.devData, "version", rsp_data.project);
        cJSON_AddStringToObject(rsp_pdu.devData, "ip", rsp_data.ip);

        p = cJSON_PrintUnformatted(rsp_json);
        
        if(NULL == p)
        {    
            ret = -1;
            goto Finish;
        }
     
    }               
    *len = strlen(p);
    memcpy(msg_out, p, *len);

    Finish:
    if(req_json)
        cJSON_Delete(req_json);
    if(rsp_json)
        cJSON_Delete(rsp_json);

    return ret;
}

/*判断项目是否为DOM100*/
static int project_check(char* project)
{
    int ret = 0;

    if(strcmp("DOM100", project) != 0)
    {
        printf("project:%s not match!\n", project);
        ret = -1;
    }

    return ret;
}