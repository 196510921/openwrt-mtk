/*
 * iosession.h
 *
 *  Created on: 2014年3月25日
 *      Author: Admin
 */

#ifndef IOSESSION_H_
#define IOSESSION_H_

#include "iobuffer.h"
#include <event2/event.h>
#include <event2/util.h>
#include "session_config.h"
#include <pthread.h>
#include <time.h>

typedef enum mina_iosession_idle_t{
	MINA_IDLE_READ,
	MINA_IDLE_WRITE
}mina_iosession_idle_t;

typedef struct mina_exception_t{
	int errorCode;
	char *errorMessage;
}mina_exception_t;

typedef struct iosession_t iosession_t;
typedef void (*_messageSentFn)(iosession_t *,iobuffer_t *);
typedef void (*_exceptionFn)(iosession_t *,mina_exception_t *);

struct iosession_t{
	int connected;
	int connfd;
	int closed;
	iobuffer_t *readBuf;
	iobuffer_t *writeBuf;
	pthread_mutex_t *writeLock;
	pthread_mutex_t *writeBufLock;
	pthread_cond_t *writeBufCond;
	_messageSentFn writeCallback;
	_exceptionFn exceptionCallback;
	struct event *readEvent;
	struct event *writeEvent;

	struct timeval *readerIdleTime;
	struct timeval *writerIdleTime;
};

extern iosession_t *iosession_create(int connfd, mina_session_config_t *sessionConfig);
extern void iosession_destroy(iosession_t *session);
extern int iosession_write(iosession_t *session, iobuffer_t *buf);
extern int iosession_write_wait(iosession_t *session, iobuffer_t *buf,int seconds);
extern int iosession_close(iosession_t *session);
extern int iosession_flush(iosession_t *session);
extern int iosession_add_read_event(iosession_t *session);
extern int iosession_add_write_event(iosession_t *session);

#endif /* IOSESSION_H_ */
