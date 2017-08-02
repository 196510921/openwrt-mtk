#include "m1_protocol.h"
#include "thread.h"
#include "sqlite.h"
#include "cjson.h"

#define M1_PROTOCOL_DEBUG    1

#define HEAD_LEN    3

extern pthread_mutex_t mut;
static char sql_buf[200];


static void* pdu1_handle(pdu_t* data);
static void* pdu2_handle(pdu_t* data);
static void* pdu3_handle(pdu_t* data);
static void * pdu4_handle(pdu_t* data);
static void * pdu5_handle(pdu_t* data);
static void* pdu6_handle(pdu_t* data);
static void unit1_handle(unit1_data_t* data, uint8_t len, uint8_t* dev_id);
static void unit4_handle(unit4_data_t* data, uint8_t len);
static void print(uint8_t* data, uint8_t len);
static void ByteToHexStr(const unsigned char* source, char* dest, int sourceLen);


void data_handle(uint8_t* _data, uint16_t len)
{

    pdu_t* data = (pdu_t*)_data;

#if M1_PROTOCOL_DEBUG

    printf("data.p_type:%x\n",data->p_type);
    printf("data.p_len:%d",data->p_len);  
    printf("\n\n");
#endif
    switch(data->p_type){
        case TYPE_PDU1: thread_create(pdu1_handle, data); break;
        case TYPE_PDU2: thread_create(pdu2_handle, data); break;
        case TYPE_PDU3: thread_create(pdu3_handle, data); break;
        case TYPE_PDU4: thread_create(pdu4_handle, data); break;
        case TYPE_PDU5: thread_create(pdu5_handle, data); break;
        case TYPE_PDU6: thread_create(pdu6_handle, data);break;
        default: printf("pdu type not match\n");break;
    }

    thread_wait();
}

/*AP report device data to M1*/
static void* pdu1_handle(pdu_t* data)
{
    int len, tmp = 0;
    static int id = 4;
    char d_id_buf[8]; 

    pdu1_data_t* p_data = (pdu1_data_t*)data->p_data;
    len = data->p_len;

    pthread_mutex_lock(&mut);

    while(len > 0){
#if M1_PROTOCOL_DEBUG
    //实际处理时要做device type检查
        printf("pdu1_handle \n p_data \n d_type:%x\n d_len:%d \n remain len:%d,\n d_id:%x,%x,%x,%x,%x,%x,%x,%x\n",p_data->d_type, p_data->d_len, len,
        p_data->d_id[0], p_data->d_id[1], p_data->d_id[2], p_data->d_id[3], p_data->d_id[4],
        p_data->d_id[5], p_data->d_id[6], p_data->d_id[7]);
#endif
        id++;
        ByteToHexStr(p_data->d_id, d_id_buf, 8);
        //sprintf(sql_buf,"INSERT INTO DEV_TABLE(ID, TYPE, DEV_ID) VALUES(%d,%2d,%s);",id, p_data->d_type, d_id_buf);
        //sqlite_exec(sql_buf);

        unit1_handle((unit1_data_t*)&p_data->u_data, p_data->d_len, d_id_buf);//p_data->d_id);
        p_data = (pdu1_data_t*)((uint8_t*)p_data + (p_data->d_len + 11));
        len -= (p_data->d_len + 11);
    }
    
    pthread_mutex_unlock(&mut);
    pthread_exit(NULL);
    
}

static void unit1_handle(unit1_data_t* data, uint8_t len, uint8_t* dev_id)
{
    int tmp;
    char* time = "2017073116305923";
    static int id = 4; 
    unit1_data_t* u_data = data;

    while(len > 0){
        //实际处理时要做参数id检查
#if M1_PROTOCOL_DEBUG
        printf("u_data:\n param_id:%x\n v_len:%d\n",u_data->param_id, u_data->v_len);
#endif
        id++;
        sprintf(sql_buf,"INSERT INTO PARAM_TABLE(ID, DEV_ID, PARAM_ID, V_LEN ,VALUE, TIME) VALUES(%d,%s,%2d,%d,%2d,%s);", id, dev_id, u_data->param_id, u_data->v_len,*(uint16_t*)u_data->val, time);
        sqlite_exec(sql_buf);
        print(u_data->val, u_data->v_len);
        u_data = (unit1_data_t*)((uint8_t*)data + (u_data->v_len + 3));

        len -= (u_data->v_len + 3);
    }
    
}

/*AP report device data to M1*/
static void* pdu6_handle(pdu_t* data)
{
    int len, tmp = 0;
    static int id = 2;
    char d_id_buf[8],ap_id_buf[8]; 

    pdu6_data_t* p_data = (pdu6_data_t*)data->p_data;
    len = data->p_len;

    pthread_mutex_lock(&mut);

    while(len > 0){
#if M1_PROTOCOL_DEBUG
    //实际处理时要做device type检查
        printf("pdu6_handle \n d_type:%x\n name:%s\n remain len:%d,\n ap_id:%x,%x,%x,%x,%x,%x,%x,%x\n d_id:%x,%x,%x,%x,%x,%x,%x,%x\n",p_data->d_type, p_data->name, len, p_data->ap_id[0], p_data->ap_id[1], p_data->ap_id[2], p_data->ap_id[3], p_data->ap_id[4],p_data->ap_id[5], p_data->ap_id[6], p_data->ap_id[7],
        p_data->d_id[0], p_data->d_id[1], p_data->d_id[2], p_data->d_id[3], p_data->d_id[4],p_data->d_id[5], p_data->d_id[6], p_data->d_id[7]);
#endif
        id++;
        ByteToHexStr(p_data->d_id, d_id_buf, 8);
        ByteToHexStr(p_data->ap_id, ap_id_buf, 8);
        sprintf(sql_buf,"INSERT INTO DEV_TABLE(ID, TYPE, DEV_ID, NAME_LEN, NAME, AP_ID, PORT) VALUES(%d,%2d,%s,%d,%s,%s,%2d);",id, p_data->d_type, d_id_buf,p_data->n_len, p_data->name, ap_id_buf, p_data->port);
        printf("%s\n", sql_buf);
        sqlite_exec(sql_buf);

        len -= (p_data->n_len + 21);
    }
    
    pthread_mutex_unlock(&mut);
    pthread_exit(NULL);
    
}

/*APP request AP/device information */
static void *pdu2_handle(pdu_t* data)
{
    pthread_mutex_lock(&mut);

#if M1_PROTOCOL_DEBUG  
    printf("pdu2_handle, data: %x", data->p_data[0]);
#endif    
    
    pthread_mutex_unlock(&mut);
    pthread_exit(NULL);
}

/*APP enable/disable device access into net*/
static void *pdu3_handle(pdu_t* data)
{
    pthread_mutex_lock(&mut);

#if M1_PROTOCOL_DEBUG  
    printf("pdu3_handle, data: %x", data->p_data[0]);
#endif

    pthread_mutex_unlock(&mut);
    pthread_exit(NULL);
}

/*device write*/
static void * pdu4_handle(pdu_t* data)
{
    pthread_mutex_lock(&mut);
    int len;

#if M1_PROTOCOL_DEBUG
    printf("pdu4_handle\n");
#endif
    pdu4_data_t* p_data = (pdu4_data_t*) data->p_data;
    len = data->p_len;
    len -= (p_data->d_len + 9);
#if M1_PROTOCOL_DEBUG    
    printf("d_id:%x %x %x %x %x %x %x %x \n d_len:%d\n remain len: %d\n",p_data->d_id[0], p_data->d_id[1], p_data->d_id[2], p_data->d_id[3], 
                                             p_data->d_id[4], p_data->d_id[5], p_data->d_id[6], p_data->d_id[7], p_data->d_len, len);
  
#endif
    unit4_handle((unit4_data_t*)&p_data->u_data, p_data->d_len);

    while(len > 0){
        p_data = (pdu4_data_t*)((uint8_t*)p_data + p_data->d_len + 9);
        len -= (p_data->d_len + 9);
#if M1_PROTOCOL_DEBUG    
         printf("d_id:%x %x %x %x %x %x %x %x \n d_len:%d\n remain len: %d\n",p_data->d_id[0], p_data->d_id[1], p_data->d_id[2], p_data->d_id[3], 
                                             p_data->d_id[4], p_data->d_id[5], p_data->d_id[6], p_data->d_id[7], p_data->d_len, len);
#endif
        unit4_handle((unit4_data_t*)&p_data->u_data, p_data->d_len);        
    }

    pthread_mutex_unlock(&mut);
    pthread_exit(NULL);
}

static void unit4_handle(unit4_data_t* data, uint8_t len)
{
    unit4_data_t* u_data = data;

    len -= (u_data->v_len + 3);
#if M1_PROTOCOL_DEBUG
    printf("u_data:\n param_id:%x\n v_len:%d\n",u_data->param_id, u_data->v_len);
    print(u_data->val, u_data->v_len);
#endif   

    while(len > 0){
        u_data = (unit4_data_t*)((uint8_t*)data + (u_data->v_len + 3));
        //实际处理时要做参数id检查
#if M1_PROTOCOL_DEBUG
        printf("u_data:\n param_id:%x\n v_len:%d\n",u_data->param_id, u_data->v_len);
#endif

        print(u_data->val, u_data->v_len);
        len -= (u_data->v_len + 3);
    }
}

/*device read*/
static void * pdu5_handle(pdu_t* data)
{
    int i, len;

    pthread_mutex_lock(&mut);
#if M1_PROTOCOL_DEBUG
    printf("pdu5_handle\n");
#endif
    pdu5_data_t* p_data = (pdu5_data_t*) data->p_data;
    len = data->p_len;
    len -= (p_data->d_len + 9);
#if M1_PROTOCOL_DEBUG    
    printf("d_id:%x %x %x %x %x %x %x %x \n",p_data->d_id[0], p_data->d_id[1], p_data->d_id[2], p_data->d_id[3], 
                                             p_data->d_id[4], p_data->d_id[5], p_data->d_id[6], p_data->d_id[7]);
    printf("d_len:%d\n",p_data->d_len);
#endif

    for(i = 0; i < p_data->d_len; i++)
    {
        printf("param_id:%x\n", p_data->u_data[i]);

    }

    while(len > 0)
    {
        p_data  = (pdu5_data_t*)((uint8_t*)p_data + p_data->d_len + 9);   
        len -= (p_data->d_len + 9);
#if M1_PROTOCOL_DEBUG    
        printf("d_id:%x %x %x %x %x %x %x %x \n",p_data->d_id[0], p_data->d_id[1], p_data->d_id[2], p_data->d_id[3], 
                                             p_data->d_id[4], p_data->d_id[5], p_data->d_id[6], p_data->d_id[7]);
        printf("d_len:%d\n",p_data->d_len);
#endif

        for(i = 0; i < p_data->d_len ; i++)
        {
            printf("param_id:%x\n", p_data->u_data[i]);

        }
    }
    
    pthread_mutex_unlock(&mut);
    pthread_exit(NULL);

}

char * makeJson(void)
{
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
    cJSON_AddNumberToObject(pduJsonObject, "pduType", 1002);
    cJSON_AddItemToObject(pJsonRoot, "pdu", pduJsonObject);
    /*1.1.1*/
    cJSON * devDataJsonArray = NULL;
    devDataJsonArray = cJSON_CreateArray();
    if(NULL == devDataJsonArray)
    {
        // create object faild, exit
        cJSON_Delete(devDataJsonArray);
        return NULL;
    }
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataJsonArray);
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
    cJSON *  paramDataObject= NULL;
    paramDataObject = cJSON_CreateObject();
    if(NULL == paramDataObject)
    {
        // create object faild, exit
        cJSON_Delete(paramDataObject);
        return NULL;
    }
    cJSON_AddNumberToObject(paramDataObject, "4006", 20);
    cJSON_AddNumberToObject(paramDataObject, "4007", 80);

    cJSON_AddItemToObject(devDataObject, "paramData", paramDataObject);
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
    cJSON *  paramDataObject2= NULL;
    paramDataObject2 = cJSON_CreateObject();
    if(NULL == paramDataObject2)
    {
        // create object faild, exit
        cJSON_Delete(paramDataObject2);
        return NULL;
    }
    cJSON_AddNumberToObject(paramDataObject2, "4008", 30);
    cJSON_AddNumberToObject(paramDataObject2, "4009", 70);

    cJSON_AddItemToObject(devDataObject1, "paramData", paramDataObject2);
    cJSON_AddItemToArray(devDataJsonArray, devDataObject1);

    char * p = cJSON_Print(pJsonRoot, 0);
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

static void print(uint8_t* data, uint8_t len)
{
    int i;

    printf("data:");
    for(i = 0; i < len; i++)
        printf(" %x",data[i]);
    
    printf("\n\n");

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





