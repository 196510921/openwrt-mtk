#include "buf_manage.h"

/*宏定义*******************************************************************************************************/
#define FIXED_BUF_LEN  ((300*100)/1024)*1024   // 支持100组300字节的有效数据同时存储
/*全局变量定义*************************************************************************************************/
char fixed_buf[FIXED_BUF_LEN];

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
