/*
 * iosession.c
 *
 *  Created on: 2014年3月25日
 *      Author: Admin
 */


#include "iosession.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
//#include <zlog.h>
#include <time.h>

extern iosession_t *iosession_create(int connfd, mina_session_config_t *sessionConfig){
	iosession_t *o = (iosession_t *)malloc(sizeof(iosession_t));
	if(o == NULL) return NULL;
	o->connfd = connfd;
	o->readBuf = iobuffer_create(sessionConfig->readBufferSize);
	o->writeBuf = iobuffer_create(sessionConfig->writeBufferSize);
	if(o->readBuf == NULL || o->writeBuf == NULL){
		free(o);
		return NULL;
	}
	o->readBuf->minSize = sessionConfig->minReadBufferSize;
	o->readBuf->maxSize = sessionConfig->maxReadBufferSize;

	o->writeBuf->minSize = sessionConfig->minWriteBufferSize;
	o->writeBuf->maxSize = sessionConfig->maxWriteBufferSize;

	o->writeLock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(o->writeLock, NULL);

	o->writeBufLock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(o->writeBufLock, NULL);

	o->writeBufCond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
	pthread_cond_init(o->writeBufCond, NULL);

	o->writeCallback = NULL;
	o->exceptionCallback = NULL;
	o->closed = 0;
	o->readerIdleTime =	NULL;
	o->writerIdleTime =	NULL;
	o->connected = 1;

	if(sessionConfig->readerIdleTime>0){
		o->readerIdleTime = (struct timeval *) malloc(sizeof(struct timeval));
		o->readerIdleTime->tv_sec = sessionConfig->readerIdleTime;
		o->readerIdleTime->tv_usec = 0;
	}
	if (sessionConfig->writerIdleTime > 0) {
		o->writerIdleTime = (struct timeval *) malloc(sizeof(struct timeval));
		o->writerIdleTime->tv_sec = sessionConfig->writerIdleTime;
		o->writerIdleTime->tv_usec = 0;
	}
	return o;
}

extern void iosession_destroy(iosession_t *session){
	if(session == NULL) return;
	pthread_mutex_destroy(session->writeLock);
	pthread_mutex_destroy(session->writeBufLock);
	pthread_cond_destroy(session->writeBufCond);

	free(session->writeLock);
	free(session->writeBufLock);
	free(session->writeBufCond);
	if(session->readerIdleTime!=NULL) free(session->readerIdleTime);
	if(session->writerIdleTime!=NULL) free(session->writerIdleTime);

	iobuffer_destroy(session->readBuf);
	iobuffer_destroy(session->writeBuf);
	free(session);
}

static int _iosession_send(iosession_t *session, iobuffer_t *buf,int remaining){
	int ret = iobuffer_get_and_write(buf, session->connfd, remaining);
	if(ret > 0){
		if (session->writeCallback != NULL) {
			int pos = iobuffer_get_position(buf);
			int limit = iobuffer_get_limit(buf);
			iobuffer_set_position(buf, pos - ret);
			session->writeCallback(session, buf);
			iobuffer_set_position(buf, pos);
			iobuffer_set_limit(buf, limit);
		}
	}
	return ret;
}

extern int iosession_write_wait(iosession_t *session, iobuffer_t *buf,int seconds){
	if(session == NULL || session->writeBuf == NULL || buf == NULL) return -1;
	if(session->closed) return -1;
	int remaining = iobuffer_remaining(buf);
	if (remaining <= 0)  return remaining;

	pthread_mutex_lock(session->writeLock);

	pthread_mutex_lock(session->writeBufLock);
	if (iobuffer_remaining(session->writeBuf) <= 0) {
		int ret = _iosession_send(session, buf, remaining);
		if(ret < 0 || ret == remaining){
			pthread_mutex_unlock(session->writeBufLock);
			pthread_mutex_unlock(session->writeLock);
			return ret;
		}
		//buf里面内容未消费完
	}
	int ret = 0;
	int secPass = 0;
	struct timespec timeout;
	int interval = 2;

	do{
		if (!session->closed && ret >= 0 && iobuffer_remaining(buf) > 0) {
			iobuffer_compact(session->writeBuf);
			ret = iobuffer_put_iobuffer(session->writeBuf, buf);
			iobuffer_flip(session->writeBuf);
			if (iobuffer_remaining(session->writeBuf) && session->writeEvent != NULL) iosession_add_write_event(session);
			if (ret >= 0 && iobuffer_remaining(buf) > 0) {
				clock_gettime(CLOCK_REALTIME, &timeout);
				timeout.tv_sec += interval;

				if (pthread_cond_timedwait(session->writeBufCond,session->writeBufLock, &timeout) != 0) {
					secPass += interval;
					if (seconds > 0 && secPass > seconds) {
						ret = -1;
						break;
					}
				}
			}
		}
	}while(!session->closed && ret >= 0 && iobuffer_remaining(buf) > 0);

	pthread_mutex_unlock(session->writeBufLock);
	pthread_mutex_unlock(session->writeLock);
	if (iobuffer_remaining(buf)>0 && session->exceptionCallback != NULL) {
		mina_exception_t exception = {100,"写socket不完整"};
		session->exceptionCallback(session,&exception);
	}
	return ret;
}

extern int iosession_write(iosession_t *session, iobuffer_t *buf){
	if(session == NULL || session->writeBuf == NULL || buf == NULL) return -1;
	if(session->closed) return -1;
	int remaining = iobuffer_remaining(buf);
	if (remaining <= 0)  return remaining;

	int ret = 0;
	pthread_mutex_lock(session->writeLock);
	pthread_mutex_lock(session->writeBufLock);
	if (iobuffer_remaining(session->writeBuf) <= 0) {
		ret = _iosession_send(session, buf, remaining);
		if(ret < 0 || iobuffer_remaining(buf)<=0){
			pthread_mutex_unlock(session->writeBufLock);
			pthread_mutex_unlock(session->writeLock);
			return ret;
		}
	}

	iobuffer_compact(session->writeBuf);
	ret += iobuffer_put_iobuffer(session->writeBuf, buf);
	iobuffer_flip(session->writeBuf);
	if (iobuffer_remaining(session->writeBuf) && session->writeEvent != NULL) iosession_add_write_event(session);

	pthread_mutex_unlock(session->writeBufLock);
	pthread_mutex_unlock(session->writeLock);

	if (iobuffer_remaining(buf)>0 && session->exceptionCallback != NULL) {
		mina_exception_t exception = {100,"写socket不完整"};
		session->exceptionCallback(session,&exception);
	}
	return ret;
}

extern int iosession_flush(iosession_t *session){
	if (session == NULL) return 0;
	pthread_mutex_lock(session->writeBufLock);
	int remaining = iobuffer_remaining(session->writeBuf);
	if (remaining <= 0) {
		pthread_cond_broadcast(session->writeBufCond);
		pthread_mutex_unlock(session->writeBufLock);
		return 0;
	}
	int ret = _iosession_send(session,session->writeBuf,remaining);
	pthread_cond_broadcast(session->writeBufCond);
	pthread_mutex_unlock(session->writeBufLock);
	if(ret > 0 && iobuffer_remaining(session->writeBuf)>0){
		iosession_add_write_event(session);
	}
	return ret;
}

extern int iosession_add_read_event(iosession_t *session){
	if(session == NULL) return -1;
	if (session->readEvent != NULL) event_add(session->readEvent, session->readerIdleTime);
	return 0;
}

extern int iosession_add_write_event(iosession_t *session){
	if(session == NULL) return -1;
	if (session->writeEvent != NULL) event_add(session->writeEvent, session->writerIdleTime);
	return 0;
}

extern int iosession_close(iosession_t *session){
	if(session == NULL) return -1;
	if(session->closed > 0) return -1;
	if(!session->connected) return -1;

	session->closed = 1;
	int ret = close(session->connfd);
	if (session->readEvent != NULL){
		event_active(session->readEvent,EV_READ, 0);
	}
	pthread_cond_broadcast(session->writeBufCond);
	return 0;
}
