#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_
#include <stdint.h>

enum block_status_t{
	TCP_CLIENT_FAILED = 0,
	TCP_CLIENT_SUCCESS
};

enum conn_flag_t{
	TCP_CONNECTED = 0,
	TCP_DISCONNECTED
};

int tcp_client_connect(void);
void socket_client_poll(void);
int get_local_clientFd(void);

#endif //_TCP_CLIENT_H_
