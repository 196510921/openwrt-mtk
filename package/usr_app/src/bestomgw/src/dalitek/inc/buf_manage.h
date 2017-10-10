#ifndef _BUF_MANAGE_H_
#define _BUF_MANAGE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
 
typedef struct _fifo_t {
    uint32_t* buffer;
    uint32_t len;
    uint32_t wptr;
    uint32_t rptr;
}fifo_t;

void fifo_init(fifo_t* fifo, uint32_t* buffer, uint32_t len);
void fifo_write(fifo_t* fifo, uint32_t d);
uint32_t fifo_read(fifo_t* fifo, uint32_t* d);

uint32_t mem_poll_malloc(uint32_t len);

#endif //_BUF_MANAGE_H_