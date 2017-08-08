#include "m1_protocol.h"
#include "thread.h"
#include "sqlite.h"
#include "cJSON.h"

#define M1_PROTOCOL_DEBUG    1

#define HEAD_LEN    3

extern pthread_mutex_t mut;
static char sql_buf[200];


static void* report_data_handle(cJSON* data);
static void* dev_read_handle(cJSON* data);
static void* dev_write_handle(cJSON*data);
static void* dev_echo_dev_info_handle(cJSON*data);
static void* req_added_dev_info_handle(void);
static void* dev_access_net_control(cJSON*data);
static void* report_dev_info(cJSON*data);
static void* report_ap_info(void);
static void ByteToHexStr(const unsigned char* source, char* dest, int sourceLen);


void data_handle(char* data)
{
    cJSON* rootJson = NULL;
    cJSON* pduJson = NULL;
    cJSON* pduTypeJson = NULL;
    cJSON* pduDataJson = NULL;
    int pduType;

    rootJson = cJSON_Parse(data);
    if(NULL == rootJson){
        printf("rootJson null\n");

    }
    pduJson = cJSON_GetObjectItem(rootJson, "pdu");
    if(NULL == pduJson){
        printf("pdu null\n");

    }
    pduTypeJson = cJSON_GetObjectItem(pduJson, "pduType");
    if(NULL == pduTypeJson){
        printf("pduType null\n");

    }
    pduType = pduTypeJson->valueint;
    
    pduDataJson = cJSON_GetObjectItem(pduJson, "devData");
    if(NULL == pduDataJson){
        printf("devData null”\n");

    }
            
    printf("pduType:%x\n",pduType);
    switch(pduType){
                    case TYPE_REPORT_DATA: report_data_handle(pduDataJson);         break;
                    case TYPE_DEV_READ: dev_read_handle(pduDataJson);               break;
                    case TYPE_DEV_WRITE: dev_write_handle(pduDataJson);             break;
                    case TYPE_ECHO_DEV_INFO: dev_echo_dev_info_handle(pduDataJson); break;
                    case TYPE_REQ_ADDED_INFO: req_added_dev_info_handle();          break;
                    case TYPE_DEV_NET_CONTROL: dev_access_net_control(pduDataJson); break;
                    case TYPE_REQ_AP_INFO: report_ap_info();                        break;
                    case TYPE_REQ_DEV_INFO: report_dev_info(pduDataJson);             break;

        default: printf("pdu type not match\n");break;
    }
    /*thread_create(pdu1_handle, pduDataJson); break;*/
    cJSON_Delete(rootJson);
    //thread_wait();
}

/*AP report device data to M1*/
static void* report_data_handle(cJSON* data)
{

    //pthread_mutex_lock(&mut);

    int i,j, count1,count2;

    cJSON* devDataJson = NULL;
    cJSON* devNameJson = NULL;
    cJSON* devIdJson = NULL;
    cJSON* paramJson = NULL;
    cJSON* paramDataJson = NULL;
    cJSON* typeJson = NULL;
    cJSON* valueJson = NULL;

    count1 = cJSON_GetArraySize(data);
    printf("count1:%d\n",count1);
    for(i = 0; i < count1; i++){
        devDataJson = cJSON_GetArrayItem(data, i);
        devNameJson = cJSON_GetObjectItem(devDataJson, "devName");
        printf("devName:%s\n",devNameJson->valuestring);
        devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
        printf("devId:%s\n",devIdJson->valuestring);
        paramJson = cJSON_GetObjectItem(devDataJson, "param");
        count2 = cJSON_GetArraySize(paramJson);
        printf(" count2:%d\n",count2);
        for(j = 0; j < count2; j++){
            paramDataJson = cJSON_GetArrayItem(paramJson, j);
            typeJson = cJSON_GetObjectItem(paramDataJson, "type");
            printf("  type:%d\n",typeJson->valueint);
            valueJson = cJSON_GetObjectItem(paramDataJson, "value");
            printf("  value:%d\n",valueJson->valueint);
        }
    }
    
    //pthread_mutex_unlock(&mut);
    //pthread_exit(NULL);
    
}

static void* dev_read_handle(cJSON* data)
{
    int i,j, count1,count2;
    cJSON* devDataJson = NULL;
    cJSON* devIdJson = NULL;
    cJSON* paramTypeJson = NULL;
    cJSON* paramJson = NULL;

    count1 = cJSON_GetArraySize(data);
    printf("count1:%d\n",count1);
    for(i = 0; i < count1; i++){
        devDataJson = cJSON_GetArrayItem(data, i);
        devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
        printf("devId:%s\n",devIdJson->valuestring);
        paramTypeJson = cJSON_GetObjectItem(devDataJson, "paramType");
        count2 = cJSON_GetArraySize(paramTypeJson);
        for(j = 0; j < count2; j++){
            paramJson = cJSON_GetArrayItem(paramTypeJson, j);
            printf("  param%d:%d\n",j,paramJson->valueint);

        }
    }
}

static void* dev_write_handle(cJSON*data)
{
    int i,j, count1,count2;
    cJSON* devDataJson = NULL;
    cJSON* devIdJson = NULL;
    cJSON* paramDataJson = NULL;
    cJSON* paramArrayJson = NULL;
    cJSON* valueTypeJson = NULL;
    cJSON* valueJson = NULL;

    count1 = cJSON_GetArraySize(data);
    printf("count1:%d\n",count1);
    for(i = 0; i < count1; i++){
        devDataJson = cJSON_GetArrayItem(data, i);
        devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
        printf("devId:%s\n",devIdJson->valuestring);
        paramDataJson = cJSON_GetObjectItem(devDataJson, "param");
        count2 = cJSON_GetArraySize(paramDataJson);
        for(j = 0; j < count2; j++){
            paramArrayJson = cJSON_GetArrayItem(paramDataJson, j);
            valueTypeJson = cJSON_GetObjectItem(paramArrayJson, "type");
            printf("  type%d:%d\n",j,valueTypeJson->valueint);
            valueJson = cJSON_GetObjectItem(paramArrayJson, "value");
            printf("  value%d:%d\n",j,valueJson->valueint);

        }
    }
}

static void* dev_echo_dev_info_handle(cJSON*data)
{
    int i,count1;
    cJSON* devDataJson = NULL;
    
    count1 = cJSON_GetArraySize(data);
    printf("count1:%d\n",count1);
    for(i = 0; i < count1; i++){
        devDataJson = cJSON_GetArrayItem(data, i);
        printf("  devId:%s\n",devDataJson->valuestring);
    }
}

static void* req_added_dev_info_handle(void)
{
    /*cJSON*/
    int pduType = 0x1004;
    cJSON * pJsonRoot = NULL;
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt_1 = NULL,*stmt_2 = NULL;
    char* sql_1 = NULL;
    char sql_2[100];

    printf("req_added_dev_info_handle\n");
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        printf("pJsonRoot NULL\n");
        cJSON_Delete(pJsonRoot);
        return NULL;
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
        return NULL;
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
        return NULL;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataJsonArray);
    /*sqlite3*/
    int rc;
    rc = sqlite3_open("dev_info.db", &db);  
  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return NULL;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    } 

    int id_1, port_1, total_column_1;
    const char* dev_name_1 = NULL, *dev_id_1 = NULL, *ap_id_1 = NULL, *time_1 = NULL;
    int id_2, port_2,total_column_2;
    const char* dev_id_2 = NULL,*dev_name_2 = NULL,*ap_id_2 = NULL, *time_2 = NULL;
    cJSON*  devDataObject= NULL;
    cJSON * devArray = NULL;
    cJSON*  devObject = NULL;

    sql_1 = "select * from ADDED_DEV where DEV_ID  = AP_ID;";
    sqlite3_prepare_v2(db, sql_1, strlen(sql_1),&stmt_1, NULL);
    total_column_1 = sqlite3_column_count(stmt_1);
    printf("total_column_1 = %d\n", total_column_1); 
    if(total_column_1 > 0){
        while(sqlite3_step(stmt_1) == SQLITE_ROW){
            id_1 = sqlite3_column_int(stmt_1, 0);
            dev_name_1 = sqlite3_column_text(stmt_1, 1);
            dev_id_1 = sqlite3_column_text(stmt_1, 2);
            ap_id_1 = sqlite3_column_text(stmt_1, 3);
            port_1 = sqlite3_column_int(stmt_1, 4);
            time_1 = sqlite3_column_text(stmt_1, 5);
            printf("id: %x, dev_name: %s, dev_id: %s, ap_id: %s, port: %x, time: %s\n", 
                id_1, dev_name_1, dev_id_1, ap_id_1, port_1, time_1);
            /*add ap infomation: port,ap_id,ap_name,time */
            devDataObject = cJSON_CreateObject();

            if(NULL == devDataObject)
            {
                // create object faild, exit
                printf("devDataObject NULL\n");
                cJSON_Delete(devDataObject);
                return NULL;
            }
            cJSON_AddItemToArray(devDataJsonArray, devDataObject);

            cJSON_AddNumberToObject(devDataObject, "PORT", port_1);
            cJSON_AddStringToObject(devDataObject, "AP_ID", ap_id_1);
            cJSON_AddStringToObject(devDataObject, "DEV_NAME", dev_name_1);
            
            /*create devData array*/
             devArray = cJSON_CreateArray();
             if(NULL == devArray)
             {
                 printf("devArry NULL\n");
                 cJSON_Delete(devArray);
                 return NULL;
             }
            /*add devData array to pdu pbject*/
             cJSON_AddItemToObject(devDataObject, "dev", devArray);
             /*sqlite3*/
             sprintf(sql_2,"select * from ADDED_DEV where AP_ID  = %s AND AP_ID != DEV_ID;",ap_id_1);
             printf("sql_2:%s\n",sql_2);
             sqlite3_prepare_v2(db, sql_2, strlen(sql_2),&stmt_2, NULL);
             total_column_2 = sqlite3_column_count(stmt_2);
             printf("total_column_2 = %d\n", total_column_2); 
             if(total_column_2 > 0){
                while(sqlite3_step(stmt_2) == SQLITE_ROW){
                     id_2 = sqlite3_column_int(stmt_2, 0);
                     dev_name_2 = sqlite3_column_text(stmt_2, 1);
                     dev_id_2 = sqlite3_column_text(stmt_2, 2);
                     ap_id_2 = sqlite3_column_text(stmt_2, 3);
                     port_2 = sqlite3_column_int(stmt_2, 4);
                     time_2 = sqlite3_column_text(stmt_2, 5);
                     printf("id: %x, dev_name: %s, dev_id: %s, ap_id: %s, port: %x, time: %s\n", 
                 id_2, dev_name_2, dev_id_2, ap_id_2, port_2, time_2);
                     /*add ap infomation: port,ap_id,ap_name,time */
                    devObject = cJSON_CreateObject();
                    if(NULL == devObject)
                    {
                        printf("devObject NULL\n");
                        cJSON_Delete(devObject);
                        return NULL;
                    }
                    cJSON_AddItemToArray(devArray, devObject); 
                    cJSON_AddStringToObject(devObject, "DEV_ID", dev_id_2);
                    cJSON_AddStringToObject(devObject, "DEV_NAME", dev_name_2);
                }
            }

        }
    }

    sqlite3_close(db);

    char * p = cJSON_Print(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        return NULL;
    }

    printf("string:%s\n",p);
    cJSON_Delete(pJsonRoot);

}

static void* dev_access_net_control(cJSON*data)
{
    
    printf("  net control:%d\n",data->valueint);
    
}

static void* report_ap_info(void)
{
    printf(" report_ap_info\n");
    /*cJSON*/
    int pduType = TYPE_REPORT_AP_INFO;
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
        return NULL;
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
        return NULL;
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
        return NULL;
    }
    /*add devData array to pdu pbject*/
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataJsonArray);
    /*sqlite3*/
    int rc;
    rc = sqlite3_open("dev_info.db", &db);  
  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return NULL;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    } 

    int id, port, total_column;
    const char* ap_name = NULL,*ap_id = NULL, *time = NULL;
    cJSON*  devDataObject= NULL;
    cJSON * devArray = NULL;
    cJSON*  devObject = NULL;

    sql = "select * from ap_dev;";
    sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
    total_column = sqlite3_column_count(stmt);
    printf("total_column = %d\n", total_column); 
    if(total_column > 0){
        while(sqlite3_step(stmt) == SQLITE_ROW){
            id = sqlite3_column_int(stmt, 0);
            ap_name = sqlite3_column_text(stmt, 1);
            ap_id = sqlite3_column_text(stmt, 2);
            port = sqlite3_column_int(stmt, 3);
            time = sqlite3_column_text(stmt, 4);
            printf("id: %x, ap_name: %s, ap_id: %s,port: %x, time: %s\n", 
                id, ap_name, ap_id, port, time);
            /*add ap infomation: port,ap_id,ap_name,time */
            devDataObject = cJSON_CreateObject();

            if(NULL == devDataObject)
            {
                // create object faild, exit
                printf("devDataObject NULL\n");
                cJSON_Delete(devDataObject);
                return NULL;
            }
            cJSON_AddItemToArray(devDataJsonArray, devDataObject);

            cJSON_AddNumberToObject(devDataObject, "PORT", port);
            cJSON_AddStringToObject(devDataObject, "AP_ID", ap_id);
            cJSON_AddStringToObject(devDataObject, "DEV_NAME", ap_name);
            
            

        }
    }

    sqlite3_close(db);

    char * p = cJSON_Print(pJsonRoot);
    
    if(NULL == p)
    {    
        cJSON_Delete(pJsonRoot);
        return NULL;
    }

    printf("string:%s\n",p);
    cJSON_Delete(pJsonRoot);

}

static void* report_dev_info(cJSON*data)
{
    printf("  request type:%d\n",data->valueint);
}

char * makeJson(void)
{
    int pduType = 0x1002;
    /*1*/
    cJSON * pJsonRoot = NULL;
           
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        //error happend here
        return NULL;
    }
    cJSON_AddNumberToObject(pJsonRoot, "sn", 1);
    cJSON_AddStringToObject(pJsonRoot, "version", "1.0");
    cJSON_AddNumberToObject(pJsonRoot, "netFlag", 1);
    cJSON_AddNumberToObject(pJsonRoot, "cmdType", 1);
    /*1.1*/
    cJSON * pduJsonObject = NULL;
    pduJsonObject = cJSON_CreateObject();
    if(NULL == pduJsonObject)
    {
        // create object faild, exit
        cJSON_Delete(pduJsonObject);
        return NULL;
    }
    cJSON_AddNumberToObject(pduJsonObject, "pduType", pduType);
    /*1.1.1*/
    cJSON * devDataJsonArray = NULL;
    devDataJsonArray = cJSON_CreateArray();
    if(NULL == devDataJsonArray)
    {
        // create object faild, exit
        cJSON_Delete(devDataJsonArray);
        return NULL;
    }
    /*1.1.1.1*/
    cJSON *  devDataObject= NULL;
    devDataObject = cJSON_CreateObject();
    if(NULL == devDataObject)
    {
        // create object faild, exit
        cJSON_Delete(devDataObject);
        return NULL;
    }
    cJSON_AddStringToObject(devDataObject, "devName", "device1");
    cJSON_AddStringToObject(devDataObject, "devId", "12345678");
    /*1.1.1.1.1*/
    cJSON *  paramArray= NULL;
    paramArray = cJSON_CreateArray();
    if(NULL == paramArray)
    {
        // create object faild, exit
        cJSON_Delete(paramArray);
        return NULL;
    }
    /*1.1.1.1.1.1*/
    cJSON *  paramObject= NULL;
    paramObject = cJSON_CreateObject();
    if(NULL == paramObject)
    {
        // create object faild, exit
        cJSON_Delete(paramObject);
        return NULL;
    }
    cJSON_AddNumberToObject(paramObject, "type", 0x4006);
    cJSON_AddNumberToObject(paramObject, "value", 20);

    cJSON_AddItemToArray(paramArray, paramObject);    
    /*1.1.1.1.1.2*/
    cJSON *  paramObject1= NULL;
    paramObject1 = cJSON_CreateObject();
    if(NULL == paramObject1)
    {
        // create object faild, exit
        cJSON_Delete(paramObject1);
        return NULL;
    }
    cJSON_AddNumberToObject(paramObject1, "type", 0x4007);
    cJSON_AddNumberToObject(paramObject1, "value", 80);

    cJSON_AddItemToArray(paramArray, paramObject1);

    cJSON_AddItemToObject(devDataObject, "param", paramArray);
    cJSON_AddItemToArray(devDataJsonArray, devDataObject);

     /*1.1.1.2*/
    cJSON *  devDataObject1= NULL;
    devDataObject1 = cJSON_CreateObject();
    if(NULL == devDataObject1)
    {
        // create object faild, exit
        cJSON_Delete(devDataObject1);
        return NULL;
    }
    cJSON_AddStringToObject(devDataObject1, "devName", "device2");
    cJSON_AddStringToObject(devDataObject1, "devId", "12345679");
    /*1.1.1.2.1*/
    cJSON *  paramArray1= NULL;
    paramArray1 = cJSON_CreateArray();
    if(NULL == paramArray1)
    {
        // create object faild, exit
        cJSON_Delete(paramArray1);
        return NULL;
    }


    /*1.1.1.2.1.1*/
    cJSON *  paramObject2= NULL;
    paramObject2 = cJSON_CreateObject();
    if(NULL == paramObject2)
    {
        // create object faild, exit
        cJSON_Delete(paramObject2);
        return NULL;
    }
    cJSON_AddNumberToObject(paramObject2, "type", 0x4007);
    cJSON_AddNumberToObject(paramObject2, "value", 60);

    cJSON_AddItemToArray(paramArray1, paramObject2);  

    paramObject2 = cJSON_CreateObject();
    if(NULL == paramObject2)
    {
        // create object faild, exit
        cJSON_Delete(paramObject2);
        return NULL;
    }
    cJSON_AddNumberToObject(paramObject2, "type", 0x4006);
    cJSON_AddNumberToObject(paramObject2, "value", 20);

    cJSON_AddItemToArray(paramArray1, paramObject2);   

    cJSON_AddItemToObject(devDataObject1, "param", paramArray1);    
    cJSON_AddItemToArray(devDataJsonArray, devDataObject1);
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataJsonArray);
    cJSON_AddItemToObject(pJsonRoot, "pdu", pduJsonObject);

    char * p = cJSON_Print(pJsonRoot);
    // else use : 
    // char * p = cJSON_PrintUnformatted(pJsonRoot);
    if(NULL == p)
    {
          //convert json list to string faild, exit
          //because sub json pSubJson han been add to pJsonRoot, so just delete pJsonRoot, if you also delete pSubJson, it will coredump, and error is : double free
        cJSON_Delete(pJsonRoot);
        return NULL;
    }
      //free(p);
      
    cJSON_Delete(pJsonRoot);
  
    return p;
}

static void ByteToHexStr(const unsigned char* source, char* dest, int sourceLen)  
{  
    short i;  
    unsigned char highByte, lowByte;  
  
    for (i = 0; i < sourceLen; i++)  
    {  
        highByte = source[i] >> 4;  
        lowByte = source[i] & 0x0f ;  
  
        highByte += 0x30;  
  
        if (highByte > 0x39)  
                dest[i * 2] = highByte + 0x07;  
        else  
                dest[i * 2] = highByte;  
  
        lowByte += 0x30;  
        if (lowByte > 0x39)  
            dest[i * 2 + 1] = lowByte + 0x07;  
        else  
            dest[i * 2 + 1] = lowByte;  
    }  
    return ;  
}  





