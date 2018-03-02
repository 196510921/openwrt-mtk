#define _GNU_SOURCE  1
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <poll.h>

#include "tcp_client.h"
#include "interface_srpcserver.h"
#include "socket_server.h"
#include "m1_common_log.h"
#include "hal_defs.h"
#include "buf_manage.h"
#include "m1_cloud.h"

#define LOCAL_IP  1

#ifdef LOCAL_IP
	#define SERVER_IP  "172.16.200.1"
	#define SERV_PORT 6666
#else
	#define SERVER_IP  "server.natappfree.cc"
	#define SERV_PORT 44809
#endif

static int conn_flag = TCP_DISCONNECTED;
static int client_sockfd = 0;
static char clientTcpRxBuf[1024*60] = {0};

static int socket_client_handle(int clientFd, int revent);
static void client_rx_cb(int clientFd);

static void set_connect_flag(int d)
{
	conn_flag = d;
}

static int get_connect_flag(void)
{
	return conn_flag;
}

void tcp_client_disconnect_cb(int sockfd)
{
	if(sockfd == client_sockfd){
		set_connect_flag(TCP_DISCONNECTED);
		delete_account_conn_info(sockfd);
		client_block_destory(sockfd);
	}
}

int tcp_client_connect(void)
{
	struct sockaddr_in  servaddr;
	struct hostent *host;
	char* server_ip = SERVER_IP;
	int i;

#ifndef LOCAL_IP
	host = gethostbyname(server_ip);
	if(NULL == host){
		perror("can not get host by hostname");
		set_connect_flag(TCP_DISCONNECTED);
		return TCP_CLIENT_FAILED;
	}

	server_ip = inet_ntoa(*(struct in_addr*)host->h_addr);
	printf("IP:%s\n", server_ip);	
#endif
   	client_sockfd = socket(AF_INET,SOCK_STREAM,0);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, server_ip, &servaddr.sin_addr);
	if(connect(client_sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1){
		perror("server connect fialed:\n");
		set_connect_flag(TCP_DISCONNECTED);
		return TCP_CLIENT_FAILED;
	}else{
	 	printf("server connect ok\n");
	 	set_connect_flag(TCP_CONNECTED);
	 	m1_report_id_to_cloud(client_sockfd);
	}

	return TCP_CLIENT_SUCCESS;
}

void socket_client_poll(void)
{
	while (1)
	{
		if(get_connect_flag() == TCP_DISCONNECTED){
			printf("reconnect...\n");
			//sleep(5);
			/*reconnect*/ 
			if(tcp_client_connect() != TCP_CLIENT_SUCCESS)
				continue;
		}
		M1_LOG_INFO("socket_client_poll\n");
	
		int pollFdIdx;
		int ret;
		//socket client FD
		struct pollfd *pollFds = malloc(sizeof(struct pollfd));

		if(pollFds)
		{
			/*set client port in the poll file descriptors*/
			pollFds[0].fd = client_sockfd;
			pollFds[0].events = POLLIN | POLLRDHUP;

			M1_LOG_INFO("client waiting for poll()\n");

			poll(pollFds, 1, -1);
			//poll(pollFds, numClientFds, -1);
			M1_LOG_INFO("client poll out\n");
			/*client*/
			if ((pollFds[0].revents))
			{
				M1_LOG_INFO("Message from Socket Sever\n");
				socket_client_handle(pollFds[0].fd, pollFds[0].revents);
			}

			free(pollFds);
			M1_LOG_INFO("free client\n");
		}
		
	}

}

static int socket_client_handle(int clientFd, int revent)
{
	M1_LOG_DEBUG("pollClientSocket++, revent: %d\n",revent);

	//this is a client socket is it a input or shutdown event
	if (revent & POLLIN)
	{
		uint32 pakcetCnt = 0;
		//its a Rx event
		M1_LOG_DEBUG("got Rx on fd %d, pakcetCnt=%d\n", clientFd, pakcetCnt++);

		client_rx_cb(clientFd);
	}

	if (revent & POLLRDHUP)
	{
		M1_LOG_DEBUG("POLLRDHUP\n");
		//its a shut down close the socket
		M1_LOG_INFO("Client fd:%d disconnected\n", clientFd);
		tcp_client_disconnect_cb(client_sockfd);
		return TCP_CLIENT_FAILED;
	}

	return TCP_CLIENT_SUCCESS;
}

static void client_rx_cb(int clientFd)
{
	int byteToRead = 0;
	int byteRead = 0;
	int rtn = 0;
	int JsonComplete = 0;
	int rc = 0;
	static int len = 0;
	static uint16_t exLen = 0;

	client_block_t* client_block = NULL;

	M1_LOG_DEBUG("SRPC_RxCB++[%x]\n", clientFd);

	rtn = ioctl(clientFd, FIONREAD, &byteToRead);

	if (rtn != 0)
	{
		M1_LOG_ERROR("SRPC_RxCB: Socket error\n");
	}
	M1_LOG_DEBUG("byteToRead:%d\n",byteToRead);

	if(byteToRead > 60*1024){
		M1_LOG_ERROR("SRPC_RxCB: out of rx buffer\n");
		return;
	}
	while(byteToRead > 0)
	{
		byteRead = read(clientFd, clientTcpRxBuf + len, 1024);
		if(byteRead > 0){
			/*判断是否是头*/
			if(msg_header_check(*(uint16_t*)(clientTcpRxBuf + len)) == 1){
				exLen = msg_len_get(*(uint16_t*)(clientTcpRxBuf + len + 2));
				M1_LOG_DEBUG("exLen:%05d\n",exLen);
				if(len > 0){
					goto Finish;
				}
			}else{
				M1_LOG_DEBUG("%x,clientTcpRxBuf:%x,%x.%x.%x,\n",*(uint16_t*)(clientTcpRxBuf + len + 2),clientTcpRxBuf[len],clientTcpRxBuf[len+1],clientTcpRxBuf[len+2],clientTcpRxBuf[len+3]);
			}
			len += byteRead;
			byteToRead -= byteRead;		
		}					
	}

	if(len - 4 < exLen){
		M1_LOG_INFO("waiting msg...\n");
		return;
	}
	
	exLen = 0;
	client_block = client_stack_block_req(clientFd);
	if(NULL == client_block){
		M1_LOG_ERROR( "client_block null\n");
		return;
	}
	
	M1_LOG_INFO("rx len:%05d, rx data:%s\n",len, clientTcpRxBuf+4);
	rc = client_write(&client_block->stack_block, clientTcpRxBuf, len);
	if(rc != TCP_SERVER_SUCCESS)
		M1_LOG_ERROR("client_write failed\n");
	
	Finish:
	len = 0;
	memset(clientTcpRxBuf, 0, 1024*60);
	M1_LOG_DEBUG("SRPC_RxCB--\n");

	return;
}

