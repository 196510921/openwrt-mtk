#include "m1_protocol.h"
#include "thread.h"

#define HEAD_LEN    3

extern pthread_mutex_t mut;

static void data_handle(pdu1_t* data);
static void* pdu1_handle(void* data);
static void unit1_handle(unit1_data_t* data);
static void* pdu2_handle(void* data);
static void* pdu3_handle(void* data);
static void print(uint8_t* data, uint8_t len);

void* test_data(void* argc)
{
    printf("into thread\n");
    pthread_mutex_lock(&mut);

    unit1_data_t u_data, u_data_1, u_data_2, u_data_3;
    pdu1_data_t p_data, p_data_1;
    pdu1_t data; 

	uint8_t buf[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
    uint8_t buf_1[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x09};
	uint8_t buf_2[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x0A};
    uint8_t buf_3[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x0B};

	u_data.param_id = 0x2001;
	u_data.v_len    = 0x08;
    u_data.val = buf;
    u_data.next = &u_data_1;
	
    u_data_1.param_id = 0x2002;
	u_data_1.v_len    = 0x08;
    u_data_1.val = buf_1;
    u_data_1.next = NULL;


	u_data_2.param_id = 0x2003;
	u_data_2.v_len    = 0x08;
    u_data_2.val = buf_2;
    u_data_2.next = &u_data_3;
	
    u_data_3.param_id = 0x2004;
	u_data_3.v_len    = 0x08;
    u_data_3.val = buf_3;
    u_data_3.next = NULL;

    p_data.d_type = 0x1001;
    p_data.d_len = HEAD_LEN + u_data.v_len 
                   + HEAD_LEN + u_data_1.v_len;
    p_data.u_data = &u_data;
    p_data.next = &p_data_1;

    p_data_1.d_type = 0x1002;
    p_data_1.d_len = HEAD_LEN + u_data_2.v_len 
                   + HEAD_LEN + u_data_3.v_len;
    p_data_1.u_data = &u_data_2;
    p_data_1.next = NULL;

	data.port = 0x6688;
    data.p_type = 0x0001;
    data.p_len = HEAD_LEN + p_data.d_len;
    data.p_data = &p_data;

    data_handle(&data);

    pthread_mutex_unlock(&mut);
    pthread_exit(NULL);

}

void* test_data_1(void* argc)
{
    printf("into thread\n");

    pthread_mutex_lock(&mut);
    unit1_data_t u_data, u_data_1, u_data_2, u_data_3;
    pdu1_data_t p_data, p_data_1;
    pdu1_t data; 

    uint8_t buf[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
    uint8_t buf_1[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x09};
    uint8_t buf_2[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x0A};
    uint8_t buf_3[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x0B};

    u_data.param_id = 0x2001;
    u_data.v_len    = 0x08;
    u_data.val = buf;
    u_data.next = &u_data_1;
    
    u_data_1.param_id = 0x2002;
    u_data_1.v_len    = 0x08;
    u_data_1.val = buf_1;
    u_data_1.next = NULL;


    u_data_2.param_id = 0x2003;
    u_data_2.v_len    = 0x08;
    u_data_2.val = buf_2;
    u_data_2.next = &u_data_3;
    
    u_data_3.param_id = 0x2004;
    u_data_3.v_len    = 0x08;
    u_data_3.val = buf_3;
    u_data_3.next = NULL;

    p_data.d_type = 0x1001;
    p_data.d_len = HEAD_LEN + u_data.v_len 
                   + HEAD_LEN + u_data_1.v_len;
    p_data.u_data = &u_data;
    p_data.next = &p_data_1;

    p_data_1.d_type = 0x1002;
    p_data_1.d_len = HEAD_LEN + u_data_2.v_len 
                   + HEAD_LEN + u_data_3.v_len;
    p_data_1.u_data = &u_data_2;
    p_data_1.next = NULL;

    data.port = 0x7799;
    data.p_type = 0x0001;
    data.p_len = HEAD_LEN + p_data.d_len;
    data.p_data = &p_data;

    data_handle(&data);

    pthread_mutex_unlock(&mut);
    pthread_exit(NULL);

}



static void data_handle(pdu1_t* data)
{

    pdu1_data_t* p_data = (pdu1_data_t*)data->p_data;

	printf("data.port%x\n",data->port);
    printf("data.p_type:%x\n",data->p_type);
    printf("data.p_len:%x\n",data->p_len);
    printf("\n\n");

    pdu1_handle(p_data);
}

/*1*/
static void* pdu1_handle(void* data)
{
    static int count;
    pdu1_data_t* p_data = (pdu1_data_t*)data;
    
    printf("p_data %d d_type:%x\n",count, p_data->d_type);
    printf("p_data %d d_len:%d\n",count, p_data->d_len);
    unit1_handle(p_data->u_data);

    if(p_data->next != NULL){
        p_data = p_data->next;
        pdu1_handle(p_data);
    }
    
}

static void unit1_handle(unit1_data_t* data)
{
    static int count;
    unit1_data_t* u_data = data;

    printf("u_data %d param_id:%x\n",count, u_data->param_id);
    printf("u_data %d v_len:%x\n",count, u_data->v_len);
    print(u_data->val, u_data->v_len);
    
    if(u_data->next != NULL){
        u_data = u_data->next;
        unit1_handle(u_data);
    }
}

/*pdu 2 handle*/
static void *pdu2_handle(void* data)
{
  
    printf("pdu2_handle %x", *(uint8_t*)data);
    
}

/*pdu 3 handle*/
static void *pdu3_handle(void* data)
{
  
    printf("pdu3_handle %x", *(uint8_t*)data);
    
}

static void print(uint8_t* data, uint8_t len)
{
    int i;

    printf("data:");
    for(i = 0; i < len; i++)
        printf(" %x",data[i]);
    
    printf("\n\n");

}









