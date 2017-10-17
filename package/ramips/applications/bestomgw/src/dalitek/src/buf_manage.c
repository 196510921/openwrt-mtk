#include <malloc.h>
#include "buf_manage.h"

/*宏定义*******************************************************************************************************/
#define FIXED_BUF_LEN  ((300*100)/1024)*1024   // 支持100组300字节的有效数据同时存储
/*全局变量定义*************************************************************************************************/

char fixed_buf[FIXED_BUF_LEN];
/*静态函数声明*************************************************************************************************/
static PNode* Buy_Node(Item item);
static int GetPQueueLen(PQueue pQueue);
static bool GetFront(PQueue pQueue, Item *pItem);
/*函数定义*****************************************************************************************************/

void fifo_init(fifo_t* fifo, uint32_t* buffer, uint32_t len)
{
    fifo->buffer = buffer;
    fifo->len = len;
    fifo->wptr = fifo->rptr = 0;
}

void fifo_write(fifo_t* fifo, uint32_t d)
{
    fifo->buffer[fifo->wptr] = d;
    fifo->wptr = (fifo->wptr + 1) % fifo->len;
}

uint32_t fifo_read(fifo_t* fifo, uint32_t* d)
{
    if (fifo->wptr == fifo->rptr) return 0;
    
    *d = fifo->buffer[fifo->rptr];
    fifo->rptr = (fifo->rptr + 1) % fifo->len;
    
    return 1;
}

/*固定长度内存空间分配*/
uint32_t mem_poll_malloc(uint32_t len)
{
	static uint32_t wptr = 0;

	if(((wptr + len) > (fixed_buf + FIXED_BUF_LEN)) || ((wptr + len) < fixed_buf)){
		wptr = fixed_buf;
	}else{
			wptr += len;
	}
	printf("wptr:%05d\n",wptr);
	return wptr;
}

//初始化队列
void Init_PQueue(PQueue pQueue)
{
	if (NULL == pQueue)
	return;
	printf("Init_PQueue\n");
	pQueue->next = NULL;
}

//从堆中申请一个节点的内存空间
static PNode* Buy_Node(Item item)
{
	PNode *pTmp = (PNode*)malloc(sizeof(PNode));
	pTmp->item = item;
	pTmp->next = NULL;
	return pTmp;
}

//入队
void Push(PQueue pQueue, Item item)
{
	PNode *pTmp = Buy_Node(item);
	PNode *pPre = pQueue;

	PNode *pCur = pQueue->next;
	while (NULL != pCur)
	{
		if (pCur->item.prio > item.prio)
		{
			pTmp->next = pCur;
			pPre->next = pTmp;
			return;
		}
		else
		{
			pPre = pCur;
			pCur = pCur->next;
		}
	}
	pPre->next = pTmp;
}

//出队，从队首(front)出
bool Pop(PQueue pQueue, Item *pItem)
{
	if (!IsEmpty(pQueue))
	{
		PNode *pTmp = pQueue->next;
		*pItem = pTmp->item;
		pQueue->next = pTmp->next;
		free(pTmp);
		return true;
	}
	return false;
}

//获取队列长度
static int GetPQueueLen(PQueue pQueue)
{
	int iCount = 0;
	PNode *pCur = pQueue->next;
	while (NULL != pCur)
	{
		++iCount;
		pCur = pCur->next;
	}
	return iCount;
}

//输出队列所有元素
void Queue_delay_decrease(PQueue pQueue)
{
	PNode *pCur = pQueue->next;
	while (NULL != pCur)
	{
		/*1s自减*/
		pCur->item.prio--;
		pCur = pCur->next;
	}
}

//队列为空则返回true
bool IsEmpty(PQueue pQueue)
{
	return pQueue->next == NULL;
}

//获取队首元素
static bool GetFront(PQueue pQueue, Item *pItem)
{
	if (!IsEmpty(pQueue))
	{
		*pItem = pQueue->item;
		return true;
	}
	return false;
}

//销毁队列(释放所有节点的内存空间)
void Destroy(PQueue pQueue)
{
	PNode *pCur = pQueue->next;
	while (NULL != pCur)
	{
		pQueue = pCur->next;
		free(pCur);
		pCur = pQueue;
	}
}





