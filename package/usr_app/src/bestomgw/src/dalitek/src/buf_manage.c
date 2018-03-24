#include <malloc.h>
#include "buf_manage.h"
#include "m1_common_log.h"

/*全局变量定义*************************************************************************************************/
#define FIXED_BUF2_LEN    (1024 * 100)

char fixed_buf[FIXED_BUF_LEN];
char fixed_buf2[FIXED_BUF2_LEN];
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
	M1_LOG_DEBUG("stack_block_init\n");
	int i;
	
	for(i = 0; i < STACK_BLOCK_NUM; i++)
	{
		block[i].blockNum = i;
		block[i].status = IDLE;
	}
}

int stack_block_req(stack_mem_t* d)
{
	M1_LOG_DEBUG("stack_block_req\n");
	int i;

	for(i = 0; i < STACK_BLOCK_NUM; i++){
		if(IDLE == block[i].status){
			block[i].status = BUSY;
			d->blockNum = i;
			d->ringFlag = RING_IDLE;
			d->unitCount = 0;
			d->start = &fixed_buf[i * STACK_BLOCK_LEN];
			d->end = &fixed_buf[(i + 1) * STACK_BLOCK_LEN];
			d->rPtr = d->start;
			d->wPtr = d->rPtr;
			return BUF_MANAGE_SUCCESS;		
		}
	}

	return BUF_MANAGE_FAILED;
}

int stack_block_destroy(stack_mem_t d)
{
	M1_LOG_DEBUG("stack_block_destroy\n");
	int i = 0;

	i = d.blockNum;
	if(i >= STACK_BLOCK_NUM)
		return BUF_MANAGE_FAILED;

	block[i].status = IDLE;
	memset(d.start, 0, STACK_BLOCK_LEN);

	return BUF_MANAGE_SUCCESS;
}

int stack_push(stack_mem_t* d, char* data, uint16_t len, uint16_t distance)
{
	int i = 0;
	int ret = BUF_MANAGE_SUCCESS;
	int count;
	int exp_count = 0;
	int remain_count = 0;

	M1_LOG_INFO( "push begin: d->unitCount:%d\n", d->unitCount);
	if(NULL == d){
		M1_LOG_ERROR("NULL == d\n");
		ret = BUF_MANAGE_FAILED;
		goto Finish;
	}
	if(d->unitCount == STACK_UNIT_CAPACITY){
		M1_LOG_WARN("d->unitCount == STACK_UNIT_CAPACITY\n");
		ret = BUF_MANAGE_FAILED;
		goto Finish;
	}

	if(distance > 0)
		exp_count = (distance / STACK_UNIT) + (((distance % STACK_UNIT) > 0) ? 1 : 0);

	if((d->wPtr + distance) > d->end){
		d->unitCount += ((d->end - d->wPtr) / STACK_UNIT);
		d->wPtr = d->start;
	}
	//M1_LOG_DEBUG( "middle:%05d\n", d->unitCount);

	remain_count = STACK_UNIT_CAPACITY - d->unitCount;
	if(exp_count > remain_count){
		M1_LOG_ERROR("exp_count > remain_count\n");
		ret = BUF_MANAGE_FAILED;
		goto Finish;		
	}

	count = (len / STACK_UNIT);// + (((len % STACK_UNIT) > 0)? 1 : 0);

	for(i = 0; i < count; i++)
	{
		if(d->unitCount == STACK_UNIT_CAPACITY){
			ret = BUF_MANAGE_FAILED;
			goto Finish;
		}
		if(d->wPtr == d->end)
			d->wPtr = d->start;
		memcpy(d->wPtr, data, STACK_UNIT);
		d->wPtr += STACK_UNIT;
		data += STACK_UNIT;
		d->unitCount++;
	}

	count = len % STACK_UNIT;
	for(i = 0; i < count; i++)
	{
		if(d->wPtr == d->end)
			d->wPtr = d->start;
		d->wPtr[i] = data[i];
	}	
	if(count > 0){
		d->unitCount++;
		d->wPtr += STACK_UNIT;
	}

	Finish:
	M1_LOG_INFO( "push end: d->unitCount:%d\n", d->unitCount);
	return ret;
}

int stack_pop(stack_mem_t* d, char* data, int len)
{
	int i = 0;
	int j = 0;
	int ret = BUF_MANAGE_SUCCESS;
	int count = 0;

	//M1_LOG_INFO( "pop begin: d->unitCount:%d\n", d->unitCount);
	if( NULL == d){
		ret = BUF_MANAGE_FAILED;
		goto Finish;
	}
	if(d->unitCount == 0){
		ret = BUF_MANAGE_FAILED;
		goto Finish;
	}

	count = (len / STACK_UNIT);
	printf("count:%d\n",count);
	for(i = 0; i < count; i++)
	{
		printf("POP :d->unitCount:%d\n",d->unitCount);
		if(d->unitCount == 0){
			ret = BUF_MANAGE_FAILED;
			goto Finish;
		}
		/*ring buf 读指针返回头部*/
		if(d->rPtr >= d->end)
			d->rPtr = d->start;
		memcpy(&data[i * STACK_UNIT], d->rPtr, STACK_UNIT);
		d->rPtr += STACK_UNIT;
		//d->rPtr += 256;
		d->unitCount--;
	}

	count = len % STACK_UNIT;
	printf("count:%d\n",count);
	for(j = 0; j < count; j++)
	{
		if(d->rPtr >= d->end)
			d->rPtr = d->start;
		data[(i * STACK_UNIT) + j] = d->rPtr[j];
	}
	if(count > 0){
		d->unitCount--;	
		d->rPtr += STACK_UNIT;
	}

	Finish:
	if(ret != BUF_MANAGE_SUCCESS)
		d->unitCount = d->unitCount + i + j;
	else
		printf("d->rPtr:%05d\n",d->rPtr);
	//M1_LOG_INFO( "pop end: d->unitCount:%d\n", d->unitCount);
	return ret;
}

/*ring buf*/
/*固定长度内存空间分配*/
char* mem_poll_malloc(uint32_t len)
{
	static char* wptr = NULL;

	if(((wptr + len) > (fixed_buf2 + FIXED_BUF2_LEN)) || ((wptr + len) < fixed_buf2)){
		wptr = fixed_buf2;
	}else{
			wptr += len;
	}
	printf("wptr:%05d\n",wptr);
	return wptr;
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
		//free(pTmp->item.data);
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





