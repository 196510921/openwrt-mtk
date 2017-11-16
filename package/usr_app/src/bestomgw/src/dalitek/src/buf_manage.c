#include <malloc.h>
#include "buf_manage.h"

/*全局变量定义*************************************************************************************************/
char fixed_buf[FIXED_BUF_LEN];
static block_status_t block[STACK_BLOCK_NUM];
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

/*静态栈内存动态分配*******************************************************************************************/

void stack_block_init(void)
{
	fprintf(stdout,"stack_block_init\n");
	int i;
	
	for(i = 0; i < STACK_BLOCK_NUM; i++)
	{
		block[i].blockNum = i;
		block[i].status = IDLE;
	}
}

int stack_block_req(stack_mem_t* d)
{
	fprintf(stdout,"stack_block_req\n");
	int i;

	for(i = 0; i < STACK_BLOCK_NUM; i++){
		if(IDLE == block[i].status){
			block[i].status = BUSY;
			d->blockNum = i;
			d->start = &fixed_buf[i * STACK_BLOCK_LEN];
			d->end = &fixed_buf[(i + 1) * STACK_BLOCK_LEN];
			d->wPtr = d->start;
			d->rPtr = d->wPtr;
			return BUF_MANAGE_SUCCESS;		
		}
	}

	return BUF_MANAGE_FAILED;
}

int stack_block_destroy(stack_mem_t d)
{
	fprintf(stdout,"stack_block_destroy\n");
	int i = 0;

	i = d.blockNum;
	if(i >= STACK_BLOCK_NUM)
		return BUF_MANAGE_FAILED;

	block[i].status = IDLE;
	memset(d.start, 0, STACK_BLOCK_LEN);

	return BUF_MANAGE_SUCCESS;
}
/*队列*********************************************************************************************************/
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





