#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_
#include <stdint.h>

#define CLIENT_TIMEOUT        4  //30 * 4 = 120s

enum block_status_t{
	TCP_CLIENT_FAILED = 0,
	TCP_CLIENT_SUCCESS
};

enum conn_flag_t{
	TCP_CONNECTED = 0,
	TCP_DISCONNECTED
};

/*tcp client connection timeout*/
void tcp_client_timeout_tick(int d);
void tcp_client_timeout_reset(int pdutype);
/*client connect*/
int tcp_client_connect(void);
void* socket_client_poll(void);
int get_local_clientFd(void);
int get_connect_flag(void);
void socket_client_tx(unsigned char* buf, int len);

#endif //_TCP_CLIENT_H_
