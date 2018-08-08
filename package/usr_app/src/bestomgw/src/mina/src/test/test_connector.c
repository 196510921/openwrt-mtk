/*
 * test_connector.c
 *
 *  Created on: 2014年3月26日
 *      Author: Admin
 */


#include "minac.h"
#include "socket_connector.h"
#include "common.h"
#if ZLOG_ENABLE
#include <zlog.h>
#endif
#include <string.h>

static void messageReceived(iosession_t *session,iobuffer_t *buf){
	#if ZLOG_ENABLE
	dzlog_info("接收到一行,size:%d",iobuffer_remaining(buf));
	#endif
/*	int32_t remaining = iobuffer_remaining(buf);
	if(remaining>0){
		iobuffer_t *rbuf = iobuffer_create(remaining);
		iobuffer_compact(rbuf);
		iobuffer_put_iobuffer(rbuf,buf);
		iobuffer_flip(rbuf);
		iosession_write(session,rbuf);
		iobuffer_destroy(rbuf);
	}*/
	iosession_close(session);
}

static void messageSent(iosession_t *session,iobuffer_t *buf){
	#if ZLOG_ENABLE
	dzlog_info("写了数据,size:%d",iobuffer_remaining(buf));
	#endif
}
static void sessionClosed(iosession_t *session){
	#if ZLOG_ENABLE
	dzlog_info("session is closed.%d",session->connfd);
	#endif
}

static void sessionOpened(iosession_t *session){
	#if ZLOG_ENABLE
	dzlog_info("session is opened.%d",session->connfd);
	#endif
}
static void exceptionCaught(iosession_t *session,mina_exception_t *exception){
	#if ZLOG_ENABLE
	dzlog_info("exception met, code:%d, messag:%s",exception->errorCode,exception->errorMessage);
	#endif
	iosession_close(session);
}
static void sessionIdle(iosession_t *session,mina_iosession_idle_t idleType){
	//dzlog_info("session idle, fd:%d, idleType:%d",session->connfd,idleType);
}
static void exit_handler(void *arg){
	mina_socket_connector_t *connector = (mina_socket_connector_t *)arg;
	mina_socket_connector_stop(connector);
}

extern void test_connector(){
	mina_socket_connector_t *connector = mina_socket_connector_create();
	connector->connectTimeout = 10;

	connector->sessionConfig->readerIdleTime = 2;
	connector->sessionConfig->writerIdleTime = 2;

	connector->socketHandler->messageReceived = messageReceived;
	connector->socketHandler->messageSent = messageSent;
	connector->socketHandler->sessionOpened = sessionOpened;
	connector->socketHandler->sessionClosed = sessionClosed;
	connector->socketHandler->exceptionCaught = exceptionCaught;
	connector->socketHandler->sessionIdle = sessionIdle;
	#if ZLOG_ENABLE
	dzlog_info("准备连接");
	#endif

	struct hostent *server = gethostbyname("192.168.0.10");
	if (server == NULL) {
		printf("找不到host\n");
		return;
	}
	struct sockaddr_in server_addr;

	bzero((char *) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *) &server_addr.sin_addr.s_addr,server->h_length);
	server_addr.sin_port = htons(6666);

	iosession_t *session = mina_socket_connector_connect(connector,(struct sockaddr *)&server_addr);

	char *hello="say hello from dylan";
	iobuffer_t *rbuf = iobuffer_create(strlen(hello));
	iobuffer_compact(rbuf);
	iobuffer_put(rbuf,hello,strlen(hello));
	iobuffer_flip(rbuf);
	iosession_write(session, rbuf);
	iobuffer_destroy(rbuf);


	register_exit_handler(exit_handler,(void *)connector);
	#if ZLOG_ENABLE
	dzlog_info("准备连接");
	#endif
}
