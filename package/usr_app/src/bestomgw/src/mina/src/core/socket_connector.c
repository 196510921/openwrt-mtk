/*
 * socket_connector.c
 *
 *  Created on: 2014年3月26日
 *      Author: Admin
 */


#include "socket_connector.h"
#include <assert.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/util.h>
#include <event2/thread.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "socket_handler.h"
#if ZLOG_ENABLE
#include <zlog.h>
#endif
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/sysinfo.h>
#include <errno.h>

static void worker_thread_run(void *);
static void _mina_socket_connector_init(mina_socket_connector_t *connector){
	int i=0;
	for(i=0;i<connector->threadNum;i++){
		pthread_create(&(connector->workers[i].thread),NULL,(void *)(&worker_thread_run),(void *)&connector->workers[i]);
	}

	//等待所有线程结束
	void *retval;
	for (i = 0; i < connector->threadNum; i++) {
		pthread_join(connector->workers[i].thread, &retval);
	}

	mina_socket_connector_destroy(connector);

}

static void close_connection(mina_socket_connector_t *connector,iosession_t *session){
	int fd = session->connfd;
	if(connector->socketHandler!=NULL && connector->socketHandler->sessionClosed!=NULL) connector->socketHandler->sessionClosed(session);
	event_free(session->readEvent);
	event_free(session->writeEvent);
	session->readEvent = NULL;
	session->writeEvent = NULL;
	iosession_close(session);
	iosession_destroy(session);
	connector->sessions[fd] = NULL;
}
static void read_cb(int fd, short event_type, void *arg){
	struct mina_connector_thread_t *worker = (struct mina_connector_thread_t *)arg;
	mina_socket_connector_t *connector = worker->connector;
	iosession_t *session = connector->sessions[fd];
	if(session == NULL) return;

	iobuffer_compact(session->readBuf);
	int ret = iobuffer_put_by_read(session->readBuf, fd, connector->sessionConfig->readBufferSize);
	iobuffer_flip(session->readBuf);
	if(ret > 0){
		if (iobuffer_remaining(session->readBuf) > 0) {
			if (connector->socketHandler != NULL && connector->socketHandler->messageReceived != NULL) {
				connector->socketHandler->messageReceived(session,session->readBuf);
			}
		}
	}else if(ret == 0){
		//read idle
		if (connector->socketHandler != NULL && connector->socketHandler->sessionIdle != NULL) {
			connector->socketHandler->sessionIdle(session, MINA_IDLE_READ);
		}
	}

	if (ret == -1) {
		close_connection(connector,session);
	} else {
		iosession_add_read_event(session);
	}
}

static void write_cb(int fd, short event_type, void *arg){
	struct mina_connector_thread_t *worker = (struct mina_connector_thread_t *)arg;
	mina_socket_connector_t *connector = worker->connector;
	iosession_t *session = connector->sessions[fd];
	if(session == NULL) return;

	int ret = iosession_flush(session);
	if(ret == 0){
		if (connector->socketHandler != NULL && connector->socketHandler->sessionIdle != NULL) {
			connector->socketHandler->sessionIdle(session, MINA_IDLE_WRITE);
		}
	}else if (ret == -1) {
		close_connection(connector,session);
	}
}

static void pipe_cb(int fd, short event_type, void *arg){
	struct mina_connector_thread_t *worker = (struct mina_connector_thread_t *)arg;
	mina_socket_connector_t *connector = worker->connector;

	int confd;
	int sizeInt = sizeof(int);
	while(!connector->stoped && read(fd, &confd, sizeof(int)) == sizeInt) {
	}
}

static void worker_thread_run(void *arg){
	struct mina_connector_thread_t *worker = (struct mina_connector_thread_t *)arg;
	event_base_dispatch(worker->base);
	event_free(worker->pipeEv);
	event_base_free(worker->base);
}


static int _socket_connect_with_timeout(int confd, struct sockaddr *addr,int timeoutInSec){
	struct timeval tm;
	fd_set set;
	int error;
	int len = sizeof(int);

	int ret = 0;
	if (connect(confd, addr, sizeof(struct sockaddr)) == -1) {
		if (errno == EINPROGRESS){// it is in the connect process
			tm.tv_sec = timeoutInSec;
			tm.tv_usec = 0;
			FD_ZERO(&set);
			FD_SET(confd, &set);
			if (select(confd + 1, NULL, &set, NULL, &tm) > 0) {
				getsockopt(confd, SOL_SOCKET, SO_ERROR, &error,(socklen_t *)&len);
				if (error == 0)
					ret = 1;
				else
					ret = 0;
			} else{
				ret = 0;
			}
		}else{
			ret = 0;
		}
	} else{
		ret = 1;
	}
	return ret;
}

extern mina_socket_connector_t* mina_socket_connector_create(){
	evthread_use_pthreads();
	mina_socket_connector_t *o = (mina_socket_connector_t *) malloc(sizeof(mina_socket_connector_t));
	if (o == NULL) return NULL;
	o->threadNum = get_nprocs();
	o->workers = (mina_connector_thread_t *)malloc(o->threadNum*sizeof(mina_connector_thread_t));
	o->sessionConfig = mina_session_config_create();
	o->socketHandler = mina_socket_handler_create();
	o->connectTimeout = 60;
	o->stoped = 0;

	int i;
	for(i=0;i<CONN_MAXFD;i++){
		o->sessions[i] = NULL;
	}
	for(i=0;i<o->threadNum;i++){
		o->workers[i].connector = o;
		struct event_base *base = event_base_new();
		assert(base != NULL);
		o->workers[i].base = base;

		if (pipe(o->workers[i].pipeFd) != 0) {
			#if ZLOG_ENABLE
			dzlog_error("pipe error!");
			#endif
			return NULL;
		}
		evutil_make_socket_nonblocking(o->workers[i].pipeFd[0]);
		evutil_make_socket_nonblocking(o->workers[i].pipeFd[1]);

		struct event *ev = event_new(base, o->workers[i].pipeFd[0],EV_READ | EV_PERSIST, pipe_cb, (void *)&o->workers[i]);
		event_add(ev, NULL);
		o->workers[i].pipeEv = ev;
	}
	pthread_create(&o->mainThread,NULL,(void *)(&_mina_socket_connector_init),(void *)o);
	pthread_detach(o->mainThread);
	return o;
}

extern void mina_socket_connector_destroy(mina_socket_connector_t *connector){
	if(connector==NULL) return;
	int i;
	for(i=0;i<CONN_MAXFD;i++){
		if(connector->sessions[i]!=NULL && !connector->sessions[i]->closed){
			close_connection(connector,connector->sessions[i]);
		}
	}
	mina_socket_handler_destroy(connector->socketHandler);
	mina_session_config_destroy(connector->sessionConfig);
	free(connector->workers);
	free(connector);
}

extern void mina_socket_connector_stop(mina_socket_connector_t *connector){
	if(connector==NULL) return;
	connector->stoped = 1;
	int i=0;
	for(i=0;i<connector->threadNum;i++){
		event_base_loopexit(connector->workers[i].base,NULL);
	}
	#if ZLOG_ENABLE
	dzlog_debug("结束connector,退出");
	#endif
}

extern iosession_t *mina_socket_connector_connect(mina_socket_connector_t *connector,struct sockaddr *addr){
	static long accept_count=0;
	int confd = socket(AF_INET,SOCK_STREAM,0);
	evutil_make_socket_nonblocking(confd);
	int timeout = connector->connectTimeout;
	if(!_socket_connect_with_timeout(confd,addr,timeout)){
		perror("连接失败\n");
		return NULL;
	}
	int so_tcpnodelay = connector->sessionConfig->tcpNoDelay;
	int so_sndbufsize = connector->sessionConfig->sendBufSize;
	int so_rcvbufsize = connector->sessionConfig->revBufSize;

	if (so_tcpnodelay > 0) setsockopt(confd, IPPROTO_TCP, TCP_NODELAY, &so_tcpnodelay,sizeof(so_tcpnodelay));
	if (so_sndbufsize > 0) setsockopt(confd, SOL_SOCKET, SO_SNDBUF, &so_sndbufsize,sizeof(so_sndbufsize));
	if (so_rcvbufsize > 0) setsockopt(confd, SOL_SOCKET, SO_RCVBUF, &so_rcvbufsize,sizeof(so_rcvbufsize));

	int idx = accept_count%connector->threadNum;
	accept_count++;

	mina_connector_thread_t *worker = &connector->workers[idx];

	iosession_t *session = iosession_create(confd,connector->sessionConfig);
	connector->sessions[confd] = session;
	if (connector->socketHandler != NULL){
		session->writeCallback = connector->socketHandler->messageSent;
		session->exceptionCallback = connector->socketHandler->exceptionCaught;
	}

	if (connector->socketHandler != NULL&& connector->socketHandler->sessionCreated != NULL) {
		connector->socketHandler->sessionCreated(session);
	}

	struct event *read_io = event_new(worker->base, confd, EV_READ, read_cb,(void *) worker);
	session->readEvent = read_io;
	iosession_add_read_event(session);

	struct event *write_io = event_new(worker->base, confd, EV_WRITE,write_cb, (void *) worker);
	session->writeEvent = write_io;
	iosession_add_write_event(session);

	if (connector->socketHandler != NULL && connector->socketHandler->sessionOpened != NULL) {
		connector->socketHandler->sessionOpened(session);
	}

	write(worker->pipeFd[1], &confd, sizeof(int));
	return session;
}


