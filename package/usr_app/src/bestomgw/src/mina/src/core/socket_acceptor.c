/*
 * socket_acceptor.c
 *
 *  Created on: 2014年3月25日
 *      Author: Admin
 */


#include "socket_acceptor.h"
#include "session_config.h"
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
#include <string.h>
#include <stdio.h>
#include <sys/sysinfo.h>

static void accept_thread_run(void *);
static void worker_thread_run(void *);
static void _mina_socket_acceptor_bind_run(mina_socket_acceptor_t *acceptor);

extern mina_socket_acceptor_t* mina_socket_acceptor_create(){
	mina_socket_acceptor_t *o = (mina_socket_acceptor_t *) malloc(sizeof(mina_socket_acceptor_t));
	if (o == NULL) return NULL;
	o->threadNum = get_nprocs();
	o->workerThreads = NULL;
	o->sessionConfig = mina_session_config_create();
	o->socketHandler = mina_socket_handler_create();
	o->stoped = 0;
	o->addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr));
	return o;
}

extern void mina_socket_acceptor_destroy(mina_socket_acceptor_t *acceptor){
	if(acceptor==NULL) return;
	mina_socket_handler_destroy(acceptor->socketHandler);
	mina_session_config_destroy(acceptor->sessionConfig);
	if(acceptor->workerThreads!=NULL) free(acceptor->workerThreads);
	free(acceptor->addr);
	free(acceptor);
}

extern void mina_socket_acceptor_unbind(mina_socket_acceptor_t *acceptor){
	acceptor->stoped = 1;
	int i=0;
	for(i=0;i<acceptor->threadNum;i++){
		event_base_loopexit(acceptor->workerThreads[i].base,NULL);
	}
	event_base_loopexit(acceptor->acceptThread.base,NULL);
#if ZLOG_ENABLE
	dzlog_debug("结束bind,退出");
#endif
	printf("结束bind,退出\n");
}

extern int mina_socket_acceptor_bind(mina_socket_acceptor_t *acceptor,struct sockaddr_in *addr){
	if(acceptor==NULL) return -1;
	memcpy(acceptor->addr,addr,sizeof(struct sockaddr_in));
	evthread_use_pthreads();

	acceptor->workerThreads = (mina_worker_thread_t *)malloc(acceptor->threadNum*sizeof(mina_worker_thread_t));
	pthread_t thread;
	pthread_create(&thread, NULL,(void *) (&_mina_socket_acceptor_bind_run), (void *) acceptor);
	pthread_detach(thread);

	return 0;
}

static void _mina_socket_acceptor_bind_run(mina_socket_acceptor_t *acceptor){
	//1.创建accept线程
	pthread_create(&acceptor->acceptThread.thread,NULL,(void *)(&accept_thread_run),(void *)acceptor);

	//2.创建worker线程
	int i=0;
	for(i=0;i<acceptor->threadNum;i++){
		acceptor->workerThreads[i].acceptor = acceptor;
		pthread_create(&(acceptor->workerThreads[i].thread),NULL,(void *)(&worker_thread_run),(void *)&acceptor->workerThreads[i]);
	}

	//3.等待所有线程结束
	void *retval;
	pthread_join(acceptor->acceptThread.thread, &retval);
	for (i = 0; i < acceptor->threadNum; i++) {
		pthread_join(acceptor->workerThreads[i].thread, &retval);
	}

	mina_socket_acceptor_destroy(acceptor);
}

static void accept_cb(evutil_socket_t sockfd, short event_type, void *arg){
	mina_socket_acceptor_t *acceptor = (mina_socket_acceptor_t *)arg;
	static long accept_count=0;
	static int size_int = sizeof(int);
	struct sockaddr_in cliaddr;
	socklen_t len = sizeof(cliaddr);

	int connfd = accept(sockfd, (struct sockaddr *) &cliaddr, &len);
	if (connfd <= 0) {
		perror("accept error");
		return;
	}
	evutil_make_socket_nonblocking(connfd);
	int so_tcpnodelay = acceptor->sessionConfig->tcpNoDelay;
	int so_sndbufsize = acceptor->sessionConfig->sendBufSize;
	int so_rcvbufsize = acceptor->sessionConfig->revBufSize;

	if (so_tcpnodelay > 0) setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, &so_tcpnodelay,sizeof(so_tcpnodelay));
	if (so_sndbufsize > 0) setsockopt(connfd, SOL_SOCKET, SO_SNDBUF, &so_sndbufsize,sizeof(so_sndbufsize));
	if (so_rcvbufsize > 0) setsockopt(connfd, SOL_SOCKET, SO_RCVBUF, &so_rcvbufsize,sizeof(so_rcvbufsize));


	int idx = accept_count%acceptor->threadNum;
	accept_count++;

	if(write(acceptor->workerThreads[idx].pipeFd[1], &connfd, sizeof(int)) < size_int){
		#if ZLOG_ENABLE
		dzlog_info("写入pipe失败");
		#endif
		printf("写入pipe失败\n");
		close(connfd);
		return;
	}
}

static void accept_thread_run(void *arg){
	mina_socket_acceptor_t *acceptor = (mina_socket_acceptor_t *)arg;
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);

	struct sockaddr_in *servaddr =(struct sockaddr_in *) acceptor->addr;
	evutil_make_listen_socket_reuseable(listenfd);
	if(bind(listenfd,(struct sockaddr*)acceptor->addr,sizeof(struct sockaddr_in)) < 0){
		#if ZLOG_ENABLE
		dzlog_info("bind fail.\n");
		#endif
		printf("bind fail.\n");
		return;
	}
	#if ZLOG_ENABLE
	dzlog_info("开始listen,端口:%d",ntohs(servaddr->sin_port));
	#endif
	printf("开始listen,端口:%d\n",ntohs(servaddr->sin_port));
	listen(listenfd,4096);
	evutil_make_socket_nonblocking(listenfd);

	int so_tcpnodelay = acceptor->sessionConfig->tcpNoDelay;
	int so_sndbufsize = acceptor->sessionConfig->sendBufSize;
	int so_rcvbufsize = acceptor->sessionConfig->revBufSize;

	if (so_tcpnodelay > 0) setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, &so_tcpnodelay,sizeof(so_tcpnodelay));
	if (so_sndbufsize > 0) setsockopt(listenfd, SOL_SOCKET, SO_SNDBUF, &so_sndbufsize,sizeof(so_sndbufsize));
	if (so_rcvbufsize > 0) setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, &so_rcvbufsize,sizeof(so_rcvbufsize));

	struct event_base *base = event_base_new();
	assert(base != NULL);
	acceptor->acceptThread.base = base;

	struct event *listen_event =  event_new(base,listenfd,EV_READ|EV_PERSIST,accept_cb,arg);
	event_add(listen_event, NULL);
	event_base_dispatch(base);
	#if ZLOG_ENABLE
	dzlog_info("结束accept线程的base");
	#endif
	printf("结束accept线程的base\n");
	event_free(listen_event);
	event_base_free(base);
}

static void close_connection(mina_socket_acceptor_t *acceptor,iosession_t *session){
	int fd = session->connfd;
	if(acceptor->socketHandler!=NULL && acceptor->socketHandler->sessionClosed!=NULL) acceptor->socketHandler->sessionClosed(session);
	event_free(session->readEvent);
	event_free(session->writeEvent);
	session->readEvent = NULL;
	session->writeEvent = NULL;
	iosession_close(session);
	iosession_destroy(session);
	acceptor->sessions[fd] = NULL;
}

static void read_cb(int fd, short event_type, void *arg){
	struct mina_worker_thread_t *worker = (struct mina_worker_thread_t *)arg;
	mina_socket_acceptor_t *acceptor = worker->acceptor;
	iosession_t *session = acceptor->sessions[fd];
	if(session == NULL) return;

	iobuffer_compact(session->readBuf);
	int ret = iobuffer_put_by_read(session->readBuf, fd, acceptor->sessionConfig->readBufferSize);
	iobuffer_flip(session->readBuf);
	if(ret > 0){
		if (iobuffer_remaining(session->readBuf) > 0) {
			if (acceptor->socketHandler != NULL && acceptor->socketHandler->messageReceived != NULL) {
				acceptor->socketHandler->messageReceived(session,session->readBuf);
			}
		}
	}else if(ret == 0){
		//read idle
		if (acceptor->socketHandler != NULL && acceptor->socketHandler->sessionIdle != NULL) {
			acceptor->socketHandler->sessionIdle(session, MINA_IDLE_READ);
		}
	}

	if (ret == -1) {
		close_connection(acceptor,session);
	} else {
		iosession_add_read_event(session);
	}
}

static void write_cb(int fd, short event_type, void *arg){
	struct mina_worker_thread_t *worker = (struct mina_worker_thread_t *)arg;
	mina_socket_acceptor_t *acceptor = worker->acceptor;
	iosession_t *session = acceptor->sessions[fd];
	if(session == NULL) return;

	int ret = iosession_flush(session);
	if(ret == 0){
		if (acceptor->socketHandler != NULL && acceptor->socketHandler->sessionIdle != NULL) {
			acceptor->socketHandler->sessionIdle(session, MINA_IDLE_WRITE);
		}
	}else if (ret == -1) {
		close_connection(acceptor,session);
	}
}

static void pipe_cb(int fd, short event_type, void *arg){
	struct mina_worker_thread_t *worker = (struct mina_worker_thread_t *)arg;
	mina_socket_acceptor_t *acceptor = worker->acceptor;

	int confd;
	int sizeInt = sizeof(int);
	while(!acceptor->stoped && read(fd, &confd, sizeof(int)) == sizeInt) {
		iosession_t *session = iosession_create(confd,acceptor->sessionConfig);
		acceptor->sessions[confd] = session;
		if (acceptor->socketHandler != NULL){
			session->writeCallback = acceptor->socketHandler->messageSent;
			session->exceptionCallback = acceptor->socketHandler->exceptionCaught;
		}

		if (acceptor->socketHandler != NULL&& acceptor->socketHandler->sessionCreated != NULL) {
			acceptor->socketHandler->sessionCreated(session);
		}

		struct event *read_io = event_new(worker->base, confd, EV_READ, read_cb,(void *) arg);
		session->readEvent = read_io;
		iosession_add_read_event(session);

		struct event *write_io = event_new(worker->base, confd, EV_WRITE,
				write_cb, (void *) arg);
		session->writeEvent = write_io;
		iosession_add_write_event(session);

		if (acceptor->socketHandler != NULL && acceptor->socketHandler->sessionOpened != NULL) {
			acceptor->socketHandler->sessionOpened(session);
		}
	}
}

static void worker_thread_run(void *arg){
	struct mina_worker_thread_t *worker = (struct mina_worker_thread_t *)arg;

	struct event_base *base = event_base_new();
	assert(base != NULL);
	worker->base = base;

	if (pipe(worker->pipeFd) != 0) {
		#if ZLOG_ENABLE
		dzlog_error("pipe error!");
		#endif
		printf("pipe error!\n");
		return;
	}
	evutil_make_socket_nonblocking(worker->pipeFd[0]);
	evutil_make_socket_nonblocking(worker->pipeFd[1]);

	struct event *ev = event_new(base, worker->pipeFd[0], EV_READ | EV_PERSIST, pipe_cb, arg);
	event_add(ev, NULL);

	event_base_dispatch(base);

	event_free(ev);
	event_base_free(base);
}
