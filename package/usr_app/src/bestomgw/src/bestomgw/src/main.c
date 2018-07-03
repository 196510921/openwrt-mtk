/**************************************************************************************************
 * Filename:       zll_controller.c
 * Description:    This file contains the interface to the UART.
 *
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#define _GNU_SOURCE  1
#define MCHECK       0

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h> 
#include "tcp_client.h"
#include "m1_protocol.h"
#include "socket_server.h"
#include "m1_common_log.h"
#include "interface_srpcserver.h"

#define MAX_DB_FILENAMR_LEN 255
#define TCP_CLIENT_ENABLE   0
/*全局变量***********************************************************************************************/	
pthread_mutex_t mutex_lock;
pthread_mutex_t mutex_lock_sock;
/*静态变量****************************************************************************************/

/*静态局部函数****************************************************************************************/
static void socket_poll(void);

#if 0
int main(int argc, char* argv[])
{
	M1_LOG_INFO("%s -- %s %s\n", argv[0], __DATE__, __TIME__);
	int retval = 0;
	pthread_t t1,t2,t3,t4,t5;

	M1_LOG_ERROR("error log testing!!!");
	SRPC_Init();
#if TCP_CLIENT_ENABLE
	tcp_client_connect();
#endif
	m1_protocol_init();

	pthread_mutex_init(&mutex_lock, NULL);
	pthread_mutex_init(&mutex_lock_sock, NULL);
	pthread_create(&t1,NULL,socket_poll,NULL);
	pthread_create(&t2,NULL,client_read,NULL);
	pthread_create(&t3,NULL,delay_send_task,NULL);
	pthread_create(&t4,NULL,scenario_alarm_select,NULL);
#if TCP_CLIENT_ENABLE
	pthread_create(&t5,NULL,socket_client_poll,NULL);
#endif

	pthread_join(t1,NULL);
	pthread_join(t2,NULL);
	pthread_join(t3, NULL);
	pthread_join(t4, NULL);
#if TCP_CLIENT_ENABLE
	pthread_join(t5, NULL);
#endif
	
	pthread_mutex_destroy(&mutex_lock);
	pthread_mutex_destroy(&mutex_lock_sock);
	return retval;
}

static void socket_poll(void)
{
	while (1)
	{
		int numClientFds = socketSeverGetNumClients();

		if (numClientFds)
		{
			M1_LOG_DEBUG("numClientFds:%d\n",numClientFds);
		
			int pollFdIdx;
			int *client_fds = malloc(numClientFds * sizeof(int));
			//socket client FD's + zllSoC serial port FD
			struct pollfd *pollFds = malloc(
					((numClientFds) * sizeof(struct pollfd)));

			if (client_fds && pollFds)
			{
				//Set the socket file descriptors
				socketSeverGetClientFds(client_fds, numClientFds);
	
				for (pollFdIdx = 0; pollFdIdx < numClientFds; pollFdIdx++)
				{
					pollFds[pollFdIdx].fd = client_fds[pollFdIdx];
					pollFds[pollFdIdx].events = POLLIN | POLLRDHUP;
					//M1_LOG_DEBUG("zllMain: adding fd %d to poll()\n", pollFds[pollFdIdx].fd);
					M1_LOG_DEBUG("zllMain: adding fd %d to poll()\n", pollFds[pollFdIdx].fd);
				}

				M1_LOG_DEBUG("zllMain: waiting for poll()\n");

				poll(pollFds, (numClientFds), -1);
				M1_LOG_INFO("poll out\n");
				/*server*/
				for (pollFdIdx = 0; pollFdIdx < numClientFds; pollFdIdx++)
				{
					if ((pollFds[pollFdIdx].revents))
					{
						M1_LOG_DEBUG("Message from Socket Client\n");
						socketSeverPoll(pollFds[pollFdIdx].fd, pollFds[pollFdIdx].revents);
					}
				}

				free(client_fds);
				free(pollFds);
				M1_LOG_DEBUG("free client\n");
			}
		}
	}

}

#else
extern void uart_485_test(void);
int main(int argc, char* argv[])
{
	//uart_485_test();
	dev_common_testing();
	return 0;
}
#endif


