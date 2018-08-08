/*
 * socket_handler.h
 *
 *  Created on: 2014年3月25日
 *      Author: Admin
 */

#ifndef SOCKET_HANDLER_H_
#define SOCKET_HANDLER_H_
#include "iosession.h"

typedef void (*_messageReceived)(iosession_t *,iobuffer_t *);
typedef void (*_messageSent)(iosession_t *,iobuffer_t *);
typedef void (*_sessionCreated)(iosession_t *);
typedef void (*_sessionOpened)(iosession_t *);
typedef void (*_sessionClosed)(iosession_t *);
typedef void (*_exceptionCaught)(iosession_t *,mina_exception_t *);
typedef void (*_sessionIdle)(iosession_t *,mina_iosession_idle_t );

typedef struct mina_socket_handler_t{
	_messageReceived messageReceived;
	_messageSent messageSent;
	_sessionCreated sessionCreated;
	_sessionOpened sessionOpened;
	_sessionClosed sessionClosed;
	_exceptionCaught exceptionCaught;
	_sessionIdle sessionIdle;
}mina_socket_handler_t;

extern mina_socket_handler_t *mina_socket_handler_create();
extern void mina_socket_handler_destroy(mina_socket_handler_t *handler);

#endif /* SOCKET_HANDLER_H_ */
