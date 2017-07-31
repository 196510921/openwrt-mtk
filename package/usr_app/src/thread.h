#ifndef _THREAD_H_
#define _THREAD_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

void thread_init(void);
void thread_create(void* func, void* argc);
void thread_wait(void);


#endif //_THREAD_H_

