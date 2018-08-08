/*
 * session_config.c
 *
 *  Created on: 2014年3月25日
 *      Author: Admin
 */


#include "session_config.h"
#include <stdio.h>

extern mina_session_config_t *mina_session_config_create(){
	mina_session_config_t *o = (mina_session_config_t *)malloc(sizeof(mina_session_config_t));
	if(o == NULL) return NULL;
	o->readBufferSize = 1024*64;
	o->minReadBufferSize = o->readBufferSize;
	o->maxReadBufferSize = 1024*1024*10;

	o->writeBufferSize = 1024 * 64;
	o->minWriteBufferSize = o->writeBufferSize;
	o->maxWriteBufferSize = 1024 * 1024 * 10;

	o->tcpNoDelay = 0;
	o->sendBufSize = 0;
	o->revBufSize = 0;

	o->readerIdleTime = 0;
	o->writerIdleTime = 0;
	return o;
}

extern void mina_session_config_destroy(mina_session_config_t *sessionConfig){
	if(sessionConfig == NULL) return;
	free(sessionConfig);
}


