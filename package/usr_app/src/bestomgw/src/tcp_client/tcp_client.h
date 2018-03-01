#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_
#include <stdint.h>

enum block_status_t{
	TCP_CLIENT_FAILED = 0,
	TCP_CLIENT_SUCCESS
};


int tcp_client_connect(void);
void socket_client_poll(void);

#endif //_TCP_CLIENT_H_
