#include "m1_protocol.h"
#include "thread.h"
#include "sqlite.h"

#define M1_PROTOCOL_DEBUG    1

#define HEAD_LEN    3

extern pthread_mutex_t mut;
static char sql_buf[200];


static void* pdu1_handle(pdu_t* data);
static void* pdu2_handle(pdu_t* data);
static void* pdu3_handle(pdu_t* data);
static void * pdu4_handle(pdu_t* data);
static void * pdu5_handle(pdu_t* data);
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
        default: printf("pdu type not match\n");break;
    }

    thread_wait();
}

/*AP report device data to M1*/
static void* pdu1_handle(pdu_t* data)
{
    int len, tmp = 0;
    static int id = 16;
    char d_id_buf[8]; 

    pdu1_data_t* p_data = (pdu1_data_t*)data->p_data;
    len = data->p_len;

    pthread_mutex_lock(&mut);
    len -= (p_data->d_len + 11);

#if M1_PROTOCOL_DEBUG
    //实际处理时要做device type检查
    printf("pdu1_handle \n p_data \n d_type:%x\n d_len:%d \n remain len:%d,\n d_id:%x,%x,%x,%x,%x,%x,%x,%x\n",p_data->d_type, p_data->d_len, len,
        p_data->d_id[0], p_data->d_id[1], p_data->d_id[2], p_data->d_id[3], p_data->d_id[4],
        p_data->d_id[5], p_data->d_id[6], p_data->d_id[7]);
#endif

    id++;
    ByteToHexStr(p_data->d_id, d_id_buf, 8);
    sprintf(sql_buf,"INSERT INTO DEV_TABLE(ID, TYPE, DEV_ID) VALUES(%d,%2d,%s);",id, p_data->d_type, d_id_buf);//p_data->d_id);
    printf("%s\n",sql_buf);
    sqlite_exec(sql_buf);

    unit1_handle((unit1_data_t*)&p_data->u_data, p_data->d_len, d_id_buf);//p_data->d_id);

    while(len > 0){
        p_data = (pdu1_data_t*)((uint8_t*)p_data + p_data->d_len + 11);
        len -= (p_data->d_len + 11);
#if M1_PROTOCOL_DEBUG
    //实际处理时要做device type检查
        printf("pdu1_handle \n p_data \n d_type:%x\n d_len:%d \n remain len:%d,\n d_id:%x,%x,%x,%x,%x,%x,%x,%x\n",p_data->d_type, p_data->d_len, len,
        p_data->d_id[0], p_data->d_id[1], p_data->d_id[2], p_data->d_id[3], p_data->d_id[4],
        p_data->d_id[5], p_data->d_id[6], p_data->d_id[7]);
#endif
        id++;
        ByteToHexStr(p_data->d_id, d_id_buf, 8);
        sprintf(sql_buf,"INSERT INTO DEV_TABLE(ID, TYPE, DEV_ID) VALUES(%d,%2d,%s);",id, p_data->d_type, d_id_buf);
        sqlite_exec(sql_buf);

        unit1_handle((unit1_data_t*)&p_data->u_data, p_data->d_len, d_id_buf);//p_data->d_id);
    }
    
    pthread_mutex_unlock(&mut);
    pthread_exit(NULL);
    
}

static void unit1_handle(unit1_data_t* data, uint8_t len, uint8_t* dev_id)
{
    int tmp;
    char* time = "2017073116305923";
    static int id = 16; 
    unit1_data_t* u_data = data;

#if M1_PROTOCOL_DEBUG
    printf("u_data:\n param_id:%x\n v_len:%d\n",u_data->param_id, u_data->v_len);
#endif
    id++;
    sprintf(sql_buf,"INSERT INTO PARAM_TABLE(ID, DEV_ID, PARAM_ID, VALUE, TIME) VALUES(%d,%s,%2d,%2d,%s);", id, dev_id, u_data->param_id, *(uint16_t*)u_data->val, time);
    sqlite_exec(sql_buf);
    print(u_data->val, u_data->v_len);
    len -= (u_data->v_len + 3);

    while(len > 0){
        u_data = (unit1_data_t*)((uint8_t*)data + (u_data->v_len + 3));
        //实际处理时要做参数id检查
#if M1_PROTOCOL_DEBUG
        printf("u_data:\n param_id:%x\n v_len:%d\n",u_data->param_id, u_data->v_len);
#endif
      id++;
        sprintf(sql_buf,"INSERT INTO PARAM_TABLE(ID, DEV_ID, PARAM_ID, VALUE, TIME) VALUES(%d,%s,%2d,%2d,%s);", id, dev_id, u_data->param_id, *(uint16_t*)u_data->val, time);
        sqlite_exec(sql_buf);
        print(u_data->val, u_data->v_len);
        len -= (u_data->v_len + 3);
    }
    
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

/*AP report device data to M1*/
/*static void* pdu6_handle(pdu_t* data)
{
    int len, tmp = 0; 

    pdu1_data_t* p_data = (pdu1_data_t*)data->p_data;
    len = data->p_len;

    pthread_mutex_lock(&mut);
    len -= (p_data->d_len + 5);

#if M1_PROTOCOL_DEBUG
    //实际处理时要做device type检查
    printf("pdu1_handle \n p_data \n d_type:%x\n d_len:%d \n remain len:%d\n, d_id:%x,%x,%x,%x,%x,%x,%x,%x\n",p_data->d_type, p_data->d_len, len,
        p_data->d_id[0], p_data->d_id[1], p_data->d_id[2], p_data->d_id[3], p_data->d_id[4],
        p_data->d_id[5], p_data->d_id[6], p_data->d_id[7]);
#endif

    //sprintf(sql_buf,"INSERT INTO dev_sum(ID, TYPE, NAME, PORT, AP, TIME, LINK, SCEN, DISTRIC) VALUES('%s', %x, '%s', %x, '%s', '%s', '%s', '%s', '%s');",data->d_id,p_data->d_type,"FF",p_data->port,"FF","FF","FF","FF","FF");
    //tmp = sqlite_exec(sql_buf);

    //if(tmp != SQLITE_OK){
    //    memset(sql_buf, 0, 200);
    //    sprintf(sql_buf,"UPDATE dev_sum SET TYPE = %x, NAME = '%s', TIME = %s",p_data->d_type, );
    //    tmp = sqlite_exec(sql_buf);
    //}

    unit1_handle((unit1_data_t*)&p_data->u_data, p_data->d_len);

    while(len > 0){
        p_data = (pdu1_data_t*)((uint8_t*)p_data + p_data->d_len + 5);
        len -= (p_data->d_len + 5);
#if M1_PROTOCOL_DEBUG
        //实际处理时要做device type检查
        printf("pdu1_handle \n p_data \n d_type:%x\n d_len:%d \n remain len:%d\n", p_data->d_type, p_data->d_len, len);
#endif
        unit1_handle((unit1_data_t*)&p_data->u_data, p_data->d_len);
    }
    
    pthread_mutex_unlock(&mut);
    pthread_exit(NULL);
    
}

static void unit6_handle(unit1_data_t* data, uint8_t len)
{
    int tmp; 
    unit1_data_t* u_data = data;

#if M1_PROTOCOL_DEBUG
    printf("u_data:\n param_id:%x\n v_len:%d\n",u_data->param_id, u_data->v_len);
#endif
    sprintf(sql_buf,"SELECT * FROM dev_sum WHRER ID LIKE %s;","'FF'");
    //tmp = sqlite_like(sql_buf);
    if(tmp != true){

    }

    print(u_data->val, u_data->v_len);
    len -= (u_data->v_len + 3);

    while(len > 0){
        u_data = (unit1_data_t*)((uint8_t*)data + (u_data->v_len + 3));
        //实际处理时要做参数id检查
#if M1_PROTOCOL_DEBUG
        printf("u_data:\n param_id:%x\n v_len:%d\n",u_data->param_id, u_data->v_len);
#endif

        print(u_data->val, u_data->v_len);
        len -= (u_data->v_len + 3);
    }
    
}
*/
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





