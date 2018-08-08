/*
 * echo_socket_handler.c
 *
 *  Created on: 2014年3月26日
 *      Author: Admin
 */

#include "socket_handler.h"
#include <stdlib.h>
#include <stdio.h>

extern mina_socket_handler_t *mina_socket_handler_create(){
	mina_socket_handler_t *handler = (mina_socket_handler_t *) malloc(sizeof(mina_socket_handler_t));
	handler->messageReceived = NULL;
	handler->exceptionCaught = NULL;
	handler->messageSent = NULL;
	handler->sessionClosed = NULL;
	handler->sessionCreated = NULL;
	handler->sessionOpened = NULL;
	handler->sessionIdle = NULL;
	return handler;
}

extern void mina_socket_handler_destroy(mina_socket_handler_t *handler){
	if(handler != NULL)
	free(handler);
}
