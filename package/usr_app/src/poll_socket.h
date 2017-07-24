#ifndef _POLL_SOCKET_H_
#define _POLL_SOCKET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <sys/types.h>

void* server_init(void* argc);

#endif //_POLL_SOCKET_H_

