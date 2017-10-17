#ifndef _BUF_MANAGE_H_
#define _BUF_MANAGE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "cJSON.h" 

typedef struct _fifo_t {
    uint32_t* buffer;
    uint32_t len;
    uint32_t wptr;
    uint32_t rptr;
}fifo_t;

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

uint32_t mem_poll_malloc(uint32_t len);

void Init_PQueue(PQueue pQueue);
void Push(PQueue pQueue, Item item);
bool Pop(PQueue pQueue, Item *pItem);
void Queue_delay_decrease(PQueue pQueue);
bool IsEmpty(PQueue pQueue);
void Destroy(PQueue pQueue);

#endif //_BUF_MANAGE_H_