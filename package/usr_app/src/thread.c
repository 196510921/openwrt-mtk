#include "thread.h"

#define THREAD_NUM  10

pthread_t thread[THREAD_NUM];
pthread_mutex_t mut;
static int thread_count;

void thread_create(void* func, void* argc)
{
	int tmp;
    
    if(thread_count >= THREAD_NUM){
        printf("no thread resource!");
    }else{

    	if(tmp = pthread_create(&thread[thread_count], NULL, func, argc) != 0)
    		printf("thread%x create failed!\n", thread_count);
    	else
    		printf("thread%x create success!\n", thread_count);
    
        thread_count++;
    }
	
}

void thread_wait(void)
{
    int i;

    for(i = 0; i < thread_count; i++){
    	if(thread[i] != 0)
    	{
    		pthread_join(thread[i], NULL);
    		printf("thread%x end!\n", i);
    	}
        printf("thread wait...\n");
    }
	
}

void thread_init(void)
{
	pthread_mutex_init(&mut, NULL);
	
    thread_count = 0;

    memset(&thread, 0, sizeof(thread));
	printf("thread init!\n");
	
	
}



