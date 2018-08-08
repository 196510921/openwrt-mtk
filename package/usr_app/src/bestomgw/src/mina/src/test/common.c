/*
 * common.c
 *
 *  Created on: 2014年3月3日
 *      Author: Admin
 */
#define ZLOG_ENABLE 0

#if ZLOG_ENABLE
#include <zlog.h>
#endif
#include "common.h"
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

static int zlogInstance=1;
static struct exit_func_t *funcList=NULL;
static int funcSize=0;
static pthread_mutex_t funcListLock;
static pthread_once_t once_block = PTHREAD_ONCE_INIT;

#if ZLOG_ENABLE
static void init_sys_log(){
	zlogInstance = dzlog_init("zlog.properties", "default");
	if (zlogInstance != 0) {
		printf("加载日志配置文件失败 \n");
		return;
	}
}
#endif

static void finit_exit_handler(){
	pthread_mutex_destroy(&funcListLock);
}
#if ZLOG_ENABLE
static void finit_sys_log(){
	if(zlogInstance==0) zlog_fini();
}
#endif

static void shut_server_handler(int signo){
	#if ZLOG_ENABLE
	dzlog_info("收到over信号,%d..\n",funcSize);
	#endif
	printf("收到over信号,%d..\n",funcSize);
	if(funcList==NULL) return;
	int i=0;
	for(i=0;i<funcSize;i++){
		#if ZLOG_ENABLE
		dzlog_info("执行第%d个退出handler",i);
		#endif
		printf("执行第%d个退出handler\n",i);
		struct exit_func_t t = funcList[i];
		t.func(t.args);
	}
	finit_exit_handler();
	#if ZLOG_ENABLE
	finit_sys_log();
	#endif
}
extern void register_exit_handler(exitFunc fn, void * args){
	pthread_mutex_lock(&funcListLock);
	if(funcList==NULL){
		funcList = malloc(sizeof(struct exit_func_t));
	}else{
		funcList = realloc(funcList,(1+funcSize)*sizeof(struct exit_func_t));
	}
	funcList[funcSize].func = fn;
	funcList[funcSize].args = args;
	funcSize++;
	pthread_mutex_unlock(&funcListLock);
}

static void init_exit_handler(){
	pthread_mutex_init(&funcListLock, NULL);
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = shut_server_handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
}


static void _init_common_once(){
	#if ZLOG_ENABLE
	init_sys_log();
	#endif
	init_exit_handler();
}

extern void init_common(){
	pthread_once (&once_block, _init_common_once);
}
