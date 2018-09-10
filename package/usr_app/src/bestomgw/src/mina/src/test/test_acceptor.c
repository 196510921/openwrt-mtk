/*
 * test_acceptor.c
 *
 *  Created on: 2014年3月26日
 *      Author: Admin
 */


#include "minac.h"
#include "common.h"
#if ZLOG_ENABLE
#include <zlog.h>
#endif

static void messageReceived(iosession_t *session,iobuffer_t *buf){
	//dzlog_info("接收到一行,size:%d",iobuffer_remaining(buf));
	printf("接收到一行,size:%d,buf:%s\n",iobuffer_remaining(buf),buf->buf);
	printf("session->connfd:%d,session->readBuf->buf:%s, session->readBuf->position:%d\n",\
											                              session->connfd,\
		                                                                  session->readBuf->buf,\
		                                                                  session->readBuf->position);

	if(strcmp(session->readBuf->buf,"Test") != 0)
	{
		printf("not match\n");
		return;
	}

	session->readBuf->position += buf->capacity;

	int32_t remaining = iobuffer_remaining(buf);
	printf("remaining:%d,buf->buf:%s\n",remaining,buf->buf);
	if(remaining>0){
		iobuffer_t *rbuf = iobuffer_create(remaining);
		iobuffer_compact(rbuf);
		iobuffer_put_iobuffer(rbuf,buf);
		iobuffer_flip(rbuf);
		iosession_write(session,rbuf);
		iobuffer_destroy(rbuf);
	}

}

static void messageSent(iosession_t *session,iobuffer_t *buf){
	//dzlog_info("写了数据,size:%d",iobuffer_remaining(buf));
	printf("写了数据,size:%d\n",iobuffer_remaining(buf));
}
static void sessionClosed(iosession_t *session){
	//dzlog_info("session is closed.%d",session->connfd);
	printf("session is closed.%d\n",session->connfd);
}
static void sessionOpened(iosession_t *session){
	//dzlog_info("session is opened.%d",session->connfd);
	printf("session is opened.%d\n",session->connfd);
}
static void exceptionCaught(iosession_t *session,mina_exception_t *exception){
	//dzlog_info("exception met, code:%d, messag:%s",exception->errorCode,exception->errorMessage);
	printf("exception met, code:%d, messag:%s\n",exception->errorCode,exception->errorMessage);
	iosession_close(session);
}
static void sessionIdle(iosession_t *session,mina_iosession_idle_t idleType){
	//dzlog_info("session idle, fd:%d, idleType:%d",session->connfd,idleType);
	printf("session idle, fd:%d, idleType:%d\n",session->connfd,idleType);
}
static void exit_handler(void *arg){
	mina_socket_acceptor_t *acceptor = (mina_socket_acceptor_t *)arg;
	mina_socket_acceptor_unbind(acceptor);
}
extern void test_acceptor(){
	mina_socket_acceptor_t *acceptor = mina_socket_acceptor_create();
	acceptor->sessionConfig->tcpNoDelay = 1;
	acceptor->sessionConfig->readerIdleTime = 2;
	acceptor->sessionConfig->writerIdleTime = 2;

	acceptor->socketHandler->messageReceived = messageReceived;
	acceptor->socketHandler->messageSent = messageSent;
	acceptor->socketHandler->sessionOpened = sessionOpened;
	acceptor->socketHandler->sessionClosed = sessionClosed;
	acceptor->socketHandler->exceptionCaught = exceptionCaught;
	acceptor->socketHandler->sessionIdle = sessionIdle;

	register_exit_handler(exit_handler,(void *)acceptor);

	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(6666);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	mina_socket_acceptor_bind(acceptor,&servaddr);
}
