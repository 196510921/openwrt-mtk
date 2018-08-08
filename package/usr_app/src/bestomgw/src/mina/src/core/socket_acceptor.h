/*
 * socket_acceptor.h
 *
 *  Created on: 2014年3月25日
 *      Author: Admin
 */


#ifndef SOCKET_ACCEPTOR_H_
#define SOCKET_ACCEPTOR_H_

#include "session_config.h"
#include "socket_handler.h"
#include "iosession.h"
#include <sys/socket.h>

#define CONN_MAXFD 65536

typedef struct mina_socket_acceptor_t mina_socket_acceptor_t;
typedef struct mina_worker_thread_t{
	pthread_t thread;
	struct event_base *base;
	mina_socket_acceptor_t *acceptor;
	int pipeFd[2];
}mina_worker_thread_t;

struct mina_socket_acceptor_t{
	int reuseAddress;
	int threadNum;
	iosession_t *sessions[CONN_MAXFD];
	mina_session_config_t *sessionConfig;
	mina_socket_handler_t *socketHandler;
	mina_worker_thread_t *workerThreads;
	mina_worker_thread_t acceptThread;
	struct sockaddr_in *addr;
	int stoped;
};



extern mina_socket_acceptor_t* mina_socket_acceptor_create();
extern void mina_socket_acceptor_set_worker_num(mina_socket_acceptor_t *acceptor,int num);
extern void mina_socket_acceptor_destroy(mina_socket_acceptor_t *acceptor);
extern int mina_socket_acceptor_bind(mina_socket_acceptor_t *acceptor,struct sockaddr_in *addr);
extern void mina_socket_acceptor_unbind(mina_socket_acceptor_t *acceptor);

#endif /* SOCKET_ACCEPTOR_H_ */
