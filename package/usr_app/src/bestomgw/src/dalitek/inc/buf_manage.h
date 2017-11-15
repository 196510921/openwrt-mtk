#ifndef _BUF_MANAGE_H_
#define _BUF_MANAGE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "cJSON.h" 


enum block_status{
	IDLE = 0,
	BUSY
};

enum buf_manage_status{
	BUF_MANAGE_FAILED = 0,
	BUF_MANAGE_SUCCESS
};

typedef struct _fifo_t {
    uint32_t* buffer;
    uint32_t len;
    uint32_t wptr;
    uint32_t rptr;
}fifo_t;


typedef struct _stack_mem_t{
	uint8_t blockNum;
	char* start;
	char* end;
	char* wPtr;
	char* rPtr;
}stack_mem_t;
/*静态栈块状态*/
typedef struct _block_status_t{
	uint8_t blockNum;
	uint8_t status;
}block_status_t;

/*优先级队列*/
typedef struct _Item
{
cJSON* data;  //数据
int prio;  //优先级，值越小，优先级越高
int clientFd;
}Item;
typedef struct _PNode
{
Item item;
struct _PNode *next;
}PNode, *PQueue;

void fifo_init(fifo_t* fifo, uint32_t* buffer, uint32_t len);
void fifo_write(fifo_t* fifo, uint32_t d);
uint32_t fifo_read(fifo_t* fifo, uint32_t* d);


/*stack动态分配*/
void stack_block_init(void);
int stack_block_req(stack_mem_t* d);
int stack_block_destroy(stack_mem_t d);
/*队列*/
void Init_PQueue(PQueue pQueue);
void Push(PQueue pQueue, Item item);
bool Pop(PQueue pQueue, Item *pItem);
void Queue_delay_decrease(PQueue pQueue);
bool IsEmpty(PQueue pQueue);
void Destroy(PQueue pQueue);

#endif //_BUF_MANAGE_H_