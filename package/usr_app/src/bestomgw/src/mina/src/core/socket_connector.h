/*
 * socket_connector.h
 *
 *  Created on: 2014年3月26日
 *      Author: Admin
 */

#ifndef SOCKET_CONNECTOR_H_
#define SOCKET_CONNECTOR_H_
#define CONN_MAXFD 65536
#include "iosession.h"
#include <sys/socket.h>
#include "session_config.h"
#include "socket_handler.h"

typedef struct mina_socket_connector_t mina_socket_connector_t;
typedef struct mina_connector_thread_t{
	pthread_t thread;
	struct event_base *base;
	mina_socket_connector_t *connector;
	struct event *pipeEv;
	int pipeFd[2];
}mina_connector_thread_t;

struct mina_socket_connector_t{
	int threadNum;
	iosession_t *sessions[CONN_MAXFD];
	mina_session_config_t *sessionConfig;
	mina_socket_handler_t *socketHandler;
	struct sockaddr *addr;
	int connectTimeout;
	struct event_base *base;
	pthread_t mainThread;
	mina_connector_thread_t *workers;
	int stoped;
};

extern mina_socket_connector_t* mina_socket_connector_create();
extern void mina_socket_connector_destroy(mina_socket_connector_t *acceptor);
extern iosession_t *mina_socket_connector_connect(mina_socket_connector_t *acceptor,struct sockaddr *addr);
extern void mina_socket_connector_stop(mina_socket_connector_t *acceptor);

#endif /* SOCKET_CONNECTOR_H_ */
