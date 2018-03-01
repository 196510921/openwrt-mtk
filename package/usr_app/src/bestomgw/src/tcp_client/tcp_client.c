#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <poll.h>

#include "tcp_client.h"
#include "interface_srpcserver.h"
#include "socket_server.h"
#include "m1_common_log.h"
//#include "thpool.h"

#include "hal_defs.h"

#include "zbSocCmd.h"
#include "utils.h"
#include "m1_protocol.h"
#include "buf_manage.h"

#define SERVER_IP  "172.16.200.1"
#define SERV_PORT 6666

static int client_sockfd = 0;
// static char clientTcpRxBuf[1024*60] = {0};

static int socket_client_handle(int clientFd, int revent);
// static void client_rx_cb(int clientFd);

int tcp_client_connect(void)
{
	struct sockaddr_in  servaddr;
   	client_sockfd = socket(AF_INET,SOCK_STREAM,0);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET,SERVER_IP,&servaddr.sin_addr);
	if(connect(client_sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1){
		perror("server connect fialed:\n");
		return TCP_CLIENT_FAILED;
	}else{
	 	printf("server connect ok\n");
	}

	return TCP_CLIENT_SUCCESS;
}

void socket_client_poll(void)
{
	while (1)
	{
		
		M1_LOG_INFO("socket_client_poll\n");
	
		int pollFdIdx;
		int ret;
		//socket client FD's + zllSoC serial port FD
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
				if(ret = socket_client_handle(pollFds[0].fd, pollFds[0].revents) != TCP_CLIENT_SUCCESS){
					client_sockfd = ret;
					pollFds[0].fd = client_sockfd;
					pollFds[0].events = POLLIN | POLLRDHUP;
				}
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
		//remove the record and close the socket
		delete_account_conn_info(clientFd);
		client_block_destory(clientFd);
		/*reconnect*/
		return tcp_client_connect();
	}

	return 0;
}





