#include "m1_protocol.h"
#include "thread.h"
#include "sqlite.h"
#include "cJSON.h"

#define M1_PROTOCOL_DEBUG    1

#define HEAD_LEN    3

extern pthread_mutex_t mut;
static char sql_buf[200];


static void* AP_report_data_handle(cJSON* data);
static void* dev_read_handle(cJSON* data);
static void* dev_write_handle(cJSON*data);
static void* dev_echo_dev_info_handle(cJSON*data);
static void* req_added_dev_info_handle(void);
static void* dev_access_net_control(cJSON* data);
static void* report_dev_info(cJSON*data);
static void* report_ap_info(void);
static void* ap_report_dev_handle(cJSON* data);
static void* ap_report_ap_handle(cJSON* data);
void getNowTime(char* time);
static int get_table_id(sqlite3* db, char* sql);
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
                    case TYPE_REPORT_DATA: AP_report_data_handle(pduDataJson);      break;
                    case TYPE_DEV_READ: dev_read_handle(pduDataJson);               break;
                    case TYPE_DEV_WRITE: dev_write_handle(pduDataJson);             break;
                    case TYPE_ECHO_DEV_INFO: dev_echo_dev_info_handle(pduDataJson); break;
                    case TYPE_REQ_ADDED_INFO: req_added_dev_info_handle();          break;
                    case TYPE_DEV_NET_CONTROL: dev_access_net_control(pduDataJson); break;
                    case TYPE_REQ_AP_INFO: report_ap_info();                        break;
                    case TYPE_REQ_DEV_INFO: report_dev_info(pduDataJson);           break;
                    case TYPE_AP_REPORT_DEV_INFO: ap_report_dev_handle(pduDataJson);break;
                    case TYPE_AP_REPORT_AP_INFO: ap_report_ap_handle(pduDataJson);  break;

        default: printf("pdu type not match\n");break;
    }
    /*thread_create(pdu1_handle, pduDataJson); break;*/
    cJSON_Delete(rootJson);
    //thread_wait();
}

/*AP report device data to M1*/
static void* AP_report_data_handle(cJSON* data)
{

    int i,j, count1, count2, rc;
    char time[30];

    cJSON* devDataJson = NULL;
    cJSON* devNameJson = NULL;
    cJSON* devIdJson = NULL;
    cJSON* paramJson = NULL;
    cJSON* paramDataJson = NULL;
    cJSON* typeJson = NULL;
    cJSON* valueJson = NULL;

    getNowTime(time);
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    char* sql = "select ID from param_table order by ID desc limit 1";
    char* sql_2 = "insert into param_table(ID, DEV_NAME,DEV_ID,TYPE,VALUE,TIME) values(?,?,?,?,?,?);";
    int id;

    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return NULL;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

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
        id = get_table_id(db, sql);

        for(j = 0; j < count2; j++){
            paramDataJson = cJSON_GetArrayItem(paramJson, j);
            typeJson = cJSON_GetObjectItem(paramDataJson, "type");
            printf("  type:%d\n",typeJson->valueint);
            valueJson = cJSON_GetObjectItem(paramDataJson, "value");
            printf("  value:%d\n",valueJson->valueint);

            sqlite3_reset(stmt); 
            sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt, NULL);
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
    
}

/*AP report device information to M1*/
static void* ap_report_dev_handle(cJSON* data)
{
    int i, number, rc;
    char time[30];
    cJSON* portJson = NULL;
    cJSON* apIdJson = NULL;
    cJSON* apNameJson = NULL;
    cJSON* devJson = NULL;
    cJSON* devArrayJson = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    printf("ap_report_dev_handle\n");
    getNowTime(time);
    /*sqlite3*/
    char* sql = "select ID from all_dev order by ID desc limit 1";
    char* sql_2 = "insert into all_dev(ID, DEV_NAME, DEV_ID, AP_ID, PORT, ADDED, TIME) values(?,?,?,?,?,?,?);";
    int id;

    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return NULL;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    portJson = cJSON_GetObjectItem(data,"port");
    printf("port:%d\n",portJson->valueint);
    apIdJson = cJSON_GetObjectItem(data,"APId");
    printf("APId:%s\n",apIdJson->valuestring);
    apNameJson = cJSON_GetObjectItem(data,"APName");
    printf("APName:%s\n",apNameJson->valuestring);
    devJson = cJSON_GetObjectItem(data,"dev");
    number = cJSON_GetArraySize(devJson);
    
    cJSON* paramDataJson = NULL;    
    cJSON* idJson = NULL;    
    cJSON* nameJson = NULL;    

    id = get_table_id(db, sql);
    for(i = 0; i< number; i++){
        paramDataJson = cJSON_GetArrayItem(devJson, i);
        idJson = cJSON_GetObjectItem(paramDataJson, "devId");
        printf("devId:%s\n", idJson->valuestring);
        nameJson = cJSON_GetObjectItem(paramDataJson, "devName");
        printf("devName:%s\n", nameJson->valuestring);
         sqlite3_reset(stmt); 
        sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt, NULL);
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
}

/*AP report information to M1*/
static void* ap_report_ap_handle(cJSON* data)
{
    int rc;
    char time[30];
    cJSON* portJson = NULL;
    cJSON* apIdJson = NULL;
    cJSON* apNameJson = NULL;
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;

    printf("ap_report_dev_handle\n");
    getNowTime(time);
    /*sqlite3*/
    char* sql = "select ID from all_dev order by ID desc limit 1";
    char* sql_2 = "insert into all_dev(ID, DEV_NAME, DEV_ID, AP_ID, PORT, ADDED, TIME) values(?,?,?,?,?,?,?);";
    int id;

    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return NULL;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

    portJson = cJSON_GetObjectItem(data,"port");
    printf("port:%d\n",portJson->valueint);
    apIdJson = cJSON_GetObjectItem(data,"APId");
    printf("APId:%s\n",apIdJson->valuestring);
    apNameJson = cJSON_GetObjectItem(data,"APName");
    printf("APName:%s\n",apNameJson->valuestring);
    
    id = get_table_id(db, sql);
 
    sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt, NULL);
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
}

static void* dev_read_handle(cJSON* data)
{   
    /*read json*/
    int i,j, count1,count2, param_type;
    char* dev_id = NULL;
    cJSON* devDataJson = NULL;
    cJSON* devIdJson = NULL;
    cJSON* paramTypeJson = NULL;
    cJSON* paramJson = NULL;

    /*get sql data json*/
    int pduType = TYPE_REPORT_DATA;
    cJSON * pJsonRoot = NULL;
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt_1 = NULL,*stmt_2 = NULL;
    char sql_1[100];
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

    sqlite3_open("dev_info.db", &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return NULL;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    } 

    const char* dev_name_1 = NULL;
    int value, total_column_1, total_column_2;

    cJSON*  devDataObject= NULL;
    cJSON * devArray = NULL;
    cJSON*  devObject = NULL;

    count1 = cJSON_GetArraySize(data);
    printf("count1:%d\n",count1);
    for(i = 0; i < count1; i++){
        /*read json*/
        devDataJson = cJSON_GetArrayItem(data, i);
        devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
        printf("devId:%s\n",devIdJson->valuestring);
        dev_id = devIdJson->valuestring;
        paramTypeJson = cJSON_GetObjectItem(devDataJson, "paramType");
        count2 = cJSON_GetArraySize(paramTypeJson);
        /*get sql data json*/
        sprintf(sql_1, "select DEV_NAME from param_table where DEV_ID  = %s order by time asc limit 1;", dev_id);
        printf("%s\n", sql_1);
        sqlite3_prepare_v2(db, sql_1, strlen(sql_1),&stmt_1, NULL);
        total_column_1 = sqlite3_column_count(stmt_1);
        printf("total_column_1 = %d\n", total_column_1); 
        if(total_column_1 > 0){
            sqlite3_step(stmt_1);
            dev_name_1 = sqlite3_column_text(stmt_1,0);
            printf("dev_name:%s, dev_id:%s\n",dev_name_1, dev_id);
            devDataObject = cJSON_CreateObject();

            if(NULL == devDataObject)
            {
                // create object faild, exit
                printf("devDataObject NULL\n");
                cJSON_Delete(devDataObject);
                return NULL;
            }
            cJSON_AddItemToArray(devDataJsonArray, devDataObject);

            cJSON_AddStringToObject(devDataObject, "devName", dev_name_1);
            cJSON_AddStringToObject(devDataObject, "devId", dev_id);

            devArray = cJSON_CreateArray();
            if(NULL == devArray)
            {
                printf("devArry NULL\n");
                cJSON_Delete(devArray);
                return NULL;
            }
            /*add devData array to pdu pbject*/
            cJSON_AddItemToObject(devDataObject, "param", devArray);
        }

        for(j = 0; j < count2; j++){
            /*read json*/
            paramJson = cJSON_GetArrayItem(paramTypeJson, j);
            printf("  param%d:%d\n",j,paramJson->valueint);
            param_type = paramJson->valueint;
            /*get sql data json*/
            sprintf(sql_2, "select VALUE from param_table where DEV_ID  = %s and TYPE = %5d order by time asc limit 1;", dev_id, param_type);
            printf("%s\n", sql_2);
            sqlite3_prepare_v2(db, sql_2, strlen(sql_2),&stmt_2, NULL);
            total_column_2 = sqlite3_column_count(stmt_2);
            printf("total_column_2 = %d\n", total_column_2); 
            if(total_column_2 > 0){
                sqlite3_step(stmt_2);
                value = sqlite3_column_int(stmt_2,0);
            }
            devObject = cJSON_CreateObject();
            if(NULL == devObject)
            {
                printf("devObject NULL\n");
                cJSON_Delete(devObject);
                return NULL;
            }
            cJSON_AddItemToArray(devArray, devObject); 
            cJSON_AddNumberToObject(devObject, "type", param_type);
            cJSON_AddNumberToObject(devObject, "value", value);

        }
    }

    sqlite3_finalize(stmt_1);
    sqlite3_finalize(stmt_2);
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

static void* dev_write_handle(cJSON*data)
{
    int i,j, count1,count2;
    char time[30];
    cJSON* devDataJson = NULL;
    cJSON* devIdJson = NULL;
    cJSON* paramDataJson = NULL;
    cJSON* paramArrayJson = NULL;
    cJSON* valueTypeJson = NULL;
    cJSON* valueJson = NULL;

    printf("dev_write_handle\n");
    getNowTime(time);
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL, * stmt_1 = NULL;
    int rc,total_column,id;
    const char* dev_name = NULL;
    char* sql = "select ID from param_table order by ID desc limit 1";

    rc = sqlite3_open("dev_info.db", &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return NULL;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    } 
    id = get_table_id(db, sql);
    printf("id:%d\n",id);
    /*cJSON*/
    /*insert data*/
    char sql_1[100] ;
    char* sql_2 = "insert into param_table(ID, DEV_NAME,DEV_ID,TYPE,VALUE,TIME) values(?,?,?,?,?,?);";
    count1 = cJSON_GetArraySize(data);
    printf("count1:%d\n",count1);
    for(i = 0; i < count1; i++){
        devDataJson = cJSON_GetArrayItem(data, i);
        devIdJson = cJSON_GetObjectItem(devDataJson, "devId");
        printf("devId:%s\n",devIdJson->valuestring);
        paramDataJson = cJSON_GetObjectItem(devDataJson, "param");
        count2 = cJSON_GetArraySize(paramDataJson);
        sqlite3_reset(stmt_1); 
        sprintf(sql_1,"select DEV_NAME from all_dev where DEV_ID = \"%s\" limit 1;", devIdJson->valuestring);
        printf("sql_1:%s\n",sql_1);
        sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
        total_column = sqlite3_column_count(stmt_1);
        printf("total_column = %d\n", total_column);
        rc = sqlite3_step(stmt_1);
        printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
        dev_name = sqlite3_column_text(stmt_1, 0);
        printf("dev_name:%s\n",dev_name);
            
        for(j = 0; j < count2; j++){
            paramArrayJson = cJSON_GetArrayItem(paramDataJson, j);
            valueTypeJson = cJSON_GetObjectItem(paramArrayJson, "type");
            printf("  type%d:%d\n",j,valueTypeJson->valueint);
            valueJson = cJSON_GetObjectItem(paramArrayJson, "value");
            printf("  value%d:%d\n",j,valueJson->valueint);

            sqlite3_reset(stmt); 
            sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt, NULL);
            sqlite3_bind_int(stmt, 1, id);
            id++;
            sqlite3_bind_text(stmt, 2,  dev_name, -1, NULL);
            sqlite3_bind_text(stmt, 3, devIdJson->valuestring, -1, NULL);
            sqlite3_bind_int(stmt, 4, valueTypeJson->valueint);
            sqlite3_bind_int(stmt, 5, valueJson->valueint);
            sqlite3_bind_text(stmt, 6,  time, -1, NULL);
            rc = sqlite3_step(stmt);
            printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
        }
    }

    sqlite3_finalize(stmt); 
}

static void* dev_echo_dev_info_handle(cJSON*data)
{
    int i,count1,total_column,rc;
    cJSON* devDataJson = NULL;
    cJSON* devArrayJson = NULL;
    cJSON* APIdJson = NULL;
    printf("dev_echo_dev_info_handle\n");
    /*sqlite3*/
    sqlite3* db = NULL;
    char* err_msg = NULL;
    char sql_1[100];

    sqlite3_open("dev_info.db",&db);

    APIdJson = cJSON_GetObjectItem(data, "APId");
    devDataJson = cJSON_GetObjectItem(data,"devId");
    printf("AP_ID:%s\n",APIdJson->valuestring);

    count1 = cJSON_GetArraySize(devDataJson);
    printf("count1:%d\n",count1);
    for(i = 0; i < count1; i++){
        devArrayJson = cJSON_GetArrayItem(devDataJson, i);
        printf("  devId:%s\n",devArrayJson->valuestring);

        sprintf(sql_1, "update all_dev set ADDED = 1 where DEV_ID = %s and AP_ID = %s;",devArrayJson->valuestring,APIdJson->valuestring);
        printf("sql_1:%s\n",sql_1);
        rc = sqlite3_exec(db, sql_1, NULL, 0, &err_msg);
        if(rc != SQLITE_OK){
            printf("SQL error:%s\n",err_msg);
            sqlite3_free(err_msg);
        }
    }
    sqlite3_close(db);
}

static void* req_added_dev_info_handle(void)
{
    /*cJSON*/
    int pduType = TYPE_M1_REPORT_ADDED_INFO;
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

    sql_1 = "select * from all_dev where DEV_ID  = AP_ID;";
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

    sqlite3_finalize(stmt_1);
    sqlite3_finalize(stmt_2);
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

static void* dev_access_net_control(cJSON *data)
{
    
    printf("  net control:%d\n",data->valueint);
    
}

static void* report_ap_info(void)
{
    printf(" report_ap_info\n");
    /*cJSON*/
    int pduType = TYPE_AP_REPORT_AP_INFO;
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

    sqlite3_finalize(stmt);
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
     printf(" report_dev_info\n");
    /*cJSON*/
    int pduType = TYPE_AP_REPORT_DEV_INFO;
    char* ap = NULL;

    ap = data->valuestring;


    cJSON * pJsonRoot = NULL;
    /*sqlite3*/
    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    char sql[100];

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
    const char* dev_name = NULL,*dev_id = NULL, *ap_id = NULL, *time = NULL;
    cJSON*  devDataObject= NULL;
    cJSON * devArray = NULL;
    cJSON*  devObject = NULL;

    sprintf(sql,"select * from sub_dev where AP_ID = %s;", ap);
    printf("string:%s\n",sql);
    sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
    total_column = sqlite3_column_count(stmt);
    printf("total_column = %d\n", total_column); 
    if(total_column > 0){
        while(sqlite3_step(stmt) == SQLITE_ROW){
            id = sqlite3_column_int(stmt, 0);
            dev_name = sqlite3_column_text(stmt, 1);
            dev_id = sqlite3_column_text(stmt, 2);
            ap_id = sqlite3_column_text(stmt, 3);
            port = sqlite3_column_int(stmt, 4);
            time = sqlite3_column_text(stmt, 5);
            printf("id: %x, dev_name: %s, dev_id: %s ,ap_id: %s,port: %x, time: %s\n", 
                id, dev_name, dev_id, ap_id, port, time);
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
            cJSON_AddStringToObject(devDataObject, "DEV_ID", dev_id);
            cJSON_AddStringToObject(devDataObject, "DEV_NAME", dev_name);
            
            
        }
    }

    sqlite3_finalize(stmt);
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

void getNowTime(char* _time)
{

    struct tm nowTime;

    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);  //获取相对于1970到现在的秒数
    localtime_r(&time.tv_sec, &nowTime);
    
    sprintf(_time, "%04d%02d%02d%02d%02d%02d", nowTime.tm_year + 1900, nowTime.tm_mon+1, nowTime.tm_mday, 
      nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec);
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


static int get_table_id(sqlite3* db, char* sql)
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



