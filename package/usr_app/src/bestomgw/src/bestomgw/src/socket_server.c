/**************************************************************************************************
 * Filename:       socket_server.c
 * Description:    Socket Remote Procedure Call Interface - sample device application.
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

/*********************************************************************
 * INCLUDES
 */
#define _GNU_SOURCE  1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <errno.h>
#include <malloc.h>

#include "thpool.h"
#include "socket_server.h"
#include "m1_protocol.h"

#define MAX_CLIENTS 50

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * TYPEDEFS
 */
typedef struct {
	void *next;
	int socketFd;
	socklen_t clilen;
	struct sockaddr_in cli_addr;
} socketRecord_t;

socketRecord_t *socketRecordHead = NULL;

socketServerCb_t socketServerRxCb;
socketServerCb_t socketServerConnectCb;

/*********************************************************************
 * LOCAL FUNCTION PROTOTYPES
 */
//static void deleteSocketRec(int rmSocketFd);
static int createSocketRec(void);
static void deleteSocketRec(int rmSocketFd);

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn      createSocketRec
 *
 * @brief   create a socket and add a rec fto the list.
 *
 * @param   table
 * @param   rmTimer
 *
 * @return  new clint fd
 */
int createSocketRec(void)
{
	int tr = 1;
	socketRecord_t *srchRec;

	socketRecord_t *newSocket = malloc(sizeof(socketRecord_t));

	//open a new client connection with the listening socket (at head of list)
	newSocket->clilen = sizeof(newSocket->cli_addr);

	//Head is always the listening socket
	newSocket->socketFd = accept(socketRecordHead->socketFd,
			(struct sockaddr *) &(newSocket->cli_addr), &(newSocket->clilen));

	//fprintf(stdout,"connected\n");

	if (newSocket->socketFd < 0) fprintf(stdout,"ERROR on accept");

	// Set the socket option SO_REUSEADDR to reduce the chance of a
	// "Address Already in Use" error on the bind
	setsockopt(newSocket->socketFd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int));
	// Set the fd to none blocking
	fcntl(newSocket->socketFd, F_SETFL, O_NONBLOCK);

	//fprintf(stdout,"New Client Connected fd:%d - IP:%s\n", newSocket->socketFd, inet_ntoa(newSocket->cli_addr.sin_addr));

	newSocket->next = NULL;

	//find the end of the list and add the record
	srchRec = socketRecordHead;
	// Stop at the last record
	while (srchRec->next)
		srchRec = srchRec->next;

	// Add to the list
	srchRec->next = newSocket;

	return (newSocket->socketFd);
}

/*********************************************************************
 * @fn      deleteSocketRec
 *
 * @brief   Delete a rec from list.
 *
 * @param   table
 * @param   rmTimer
 *
 * @return  none
 */
void deleteSocketRec(int rmSocketFd)
{
	socketRecord_t *srchRec, *prevRec = NULL;

	// Head of the timer list
	srchRec = socketRecordHead;

	// Stop when rec found or at the end
	while ((srchRec->socketFd != rmSocketFd) && (srchRec->next))
	{
		prevRec = srchRec;
		// over to next
		srchRec = srchRec->next;
	}

	if (srchRec->socketFd != rmSocketFd)
	{
		fprintf(stdout,"deleteSocketRec: record not found\n");
		return;
	}

	// Does the record exist
	if (srchRec)
	{
		// delete the timer from the list
		if (prevRec == NULL)
		{
			//trying to remove first rec, which is always the listining socket
			fprintf(stdout,
					"deleteSocketRec: removing first rec, which is always the listining socket\n");
			return;
		}

		//remove record from list
		prevRec->next = srchRec->next;

		close(srchRec->socketFd);
		free(srchRec);
	}
}

/***************************************************************************************************
 * @fn      serverSocketInit
 *
 * @brief   initialises the server.
 * @param   
 *
 * @return  Status
 */
int32 socketSeverInit(uint32 port)
{
	struct sockaddr_in serv_addr;
	int stat, tr = 1;

	if (socketRecordHead == NULL)
	{
		// New record
		socketRecord_t *lsSocket = malloc(sizeof(socketRecord_t));

		lsSocket->socketFd = socket(AF_INET, SOCK_STREAM, 0);
		if (lsSocket->socketFd < 0)
		{
			fprintf(stdout,"ERROR opening socket");
			return -1;
		}

		// Set the socket option SO_REUSEADDR to reduce the chance of a
		// "Address Already in Use" error on the bind
		setsockopt(lsSocket->socketFd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int));
		// Set the fd to none blocking
		fcntl(lsSocket->socketFd, F_SETFL, O_NONBLOCK);

		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(port);
		stat = bind(lsSocket->socketFd, (struct sockaddr *) &serv_addr,
				sizeof(serv_addr));
		if (stat < 0)
		{
			fprintf(stdout,"ERROR on binding: %s\n", strerror(errno));
			return -1;
		}
		//will have 5 pending open client requests
		listen(lsSocket->socketFd, 5);

		lsSocket->next = NULL;
		//Head is always the listening socket
		socketRecordHead = lsSocket;
	}

	//printf("waiting for socket new connection\n");

	return 0;
}

/***************************************************************************************************
 * @fn      serverSocketConfig
 *
 * @brief   register the Rx Callback.
 * @param   
 *
 * @return  Status
 */
int32 serverSocketConfig(socketServerCb_t rxCb, socketServerCb_t connectCb)
{
	socketServerRxCb = rxCb;
	socketServerConnectCb = connectCb;

	return 0;
}
/*********************************************************************
 * @fn      socketSeverGetClientFds()
 *
 * @brief   get clients fd's.
 *
 * @param   none
 *
 * @return  list of Timerfd's
 */
void socketSeverGetClientFds(int *fds, int maxFds)
{
	uint32 recordCnt = 0;
	socketRecord_t *srchRec;

	// Head of the timer list
	srchRec = socketRecordHead;

	// Stop when at the end or max is reached
	while ((srchRec) && (recordCnt < maxFds))
	{
		//fprintf(stdout,"getClientFds: adding fd%d, to idx:%d \n", srchRec->socketFd, recordCnt);
		fds[recordCnt++] = srchRec->socketFd;

		srchRec = srchRec->next;
	}

	return;
}

/*********************************************************************
 * @fn      socketSeverGetNumClients()
 *
 * @brief   get clients fd's.
 *
 * @param   none
 *
 * @return  list of Timerfd's
 */
uint32 socketSeverGetNumClients(void)
{
	uint32 recordCnt = 0;
	socketRecord_t *srchRec;

	//printf("socketSeverGetNumClients++\n", recordCnt);

	// Head of the timer list
	srchRec = socketRecordHead;

	if (srchRec == NULL)
	{
		//fprintf(stdout,"socketSeverGetNumClients: socketRecordHead NULL\n");
		return -1;
	}

	// Stop when rec found or at the end
	while (srchRec)
	{
		//printf("socketSeverGetNumClients: recordCnt=%d\n", recordCnt);
		srchRec = srchRec->next;
		recordCnt++;
	}

	//fprintf(stdout,"socketSeverGetNumClients %d\n", recordCnt);
	return (recordCnt);
}

/*********************************************************************
 * @fn      socketSeverPoll()
 *
 * @brief   services the Socket events.
 *
 * @param   clinetFd - Fd to services
 * @param   revent - event to services
 *
 * @return  none
 */
void socketSeverPoll(int clinetFd, int revent)
{
	fprintf(stdout,"pollSocket++, revent: %d\n",revent);

	//is this a new connection on the listening socket
	if (clinetFd == socketRecordHead->socketFd)
	{
		int newClientFd = createSocketRec();

		if (socketServerConnectCb)
		{
			socketServerConnectCb(newClientFd);
		}
	}
	else
	{
		//this is a client socket is it a input or shutdown event
		if (revent & POLLIN)
		{
			uint32 pakcetCnt = 0;
			//its a Rx event
			fprintf(stdout,"got Rx on fd %d, pakcetCnt=%d\n", clinetFd, pakcetCnt++);
			if (socketServerRxCb)
			{
				socketServerRxCb(clinetFd);
			}

		}
		if (revent & POLLRDHUP)
		{
			fprintf(stdout,"POLLRDHUP\n");
			//its a shut down close the socket
			fprintf(stdout,"Client fd:%d disconnected\n", clinetFd);
			//remove the record and close the socket
			deleteSocketRec(clinetFd);
		}
	}

	//write(clinetFd,"I got your message",18);

	return;
}

/***************************************************************************************************
 * @fn      socketSeverSend
 *
 * @brief   Send a buffer to a clients.
 * @param   uint8* srpcMsg - message to be sent
 *          int32 fdClient - Client fd
 *
 * @return  Status
 */
extern threadpool tx_thpool;
extern fifo_t tx_fifo;
static void thread_socketSeverSend(void)
{
	fprintf(stdout,"thread_socketSeverSend\n");
	int rtn;
	uint32_t* msg = NULL;

    if(fifo_read(&tx_fifo, &msg) == 0){
    	fprintf(stdout,"fifo read failed\n");
    	return;
    }
    if(msg == NULL){
    	fprintf(stdout,"msg NULL\n");
    	return ;
    }
    m1_package_t* package = (m1_package_t*)msg;
    fprintf(stdout,"socketSeverSend++: writing to socket fd %d\n", package->clientFd);
	if (package->clientFd)
	{
		rtn = write(package->clientFd, package->data, package->len);
		if (rtn < 0)
		{
			fprintf(stdout,"ERROR writing to socket %d\n", package->clientFd);
		}
	}

	fprintf(stdout,"socketSeverSend--\n");		
	/*free*/
    fprintf(stdout,"free\n");
    free(package->data);
    //free(package);
}

/*分配tx message地址*/
#define SOCKT_MSG_NUMBER    100
static m1_package_t socket_msg[SOCKT_MSG_NUMBER];
static m1_package_t* socket_msg_alloc(void)
{
	static uint32_t i = 0;
	m1_package_t* add = NULL;

	i %= SOCKT_MSG_NUMBER;
	add = &socket_msg[i];
	fprintf(stdout,"socket_msg_alloc:%d\n",i);
	i++;
	return add;
}

int32 socketSeverSend(uint8* buf, uint32 len, int32 fdClient)
{
	m1_package_t * msg = socket_msg_alloc();
	//m1_package_t * msg = (m1_package_t *)malloc(sizeof(m1_package_t));
	if(msg == NULL)
	{
		fprintf(stdout,"malloc failed\n");
		return ;
	}
	fprintf(stdout,"1.msg:%x\n", msg);
	msg->len = len;
	msg->clientFd = fdClient;
	msg->data = (char*)malloc(len);
	if(msg->data == NULL)
	{
		fprintf(stdout,"malloc failed\n");
		return ;
	}
	fprintf(stdout,"1.msg->data:%x\n", msg->data);
	memcpy(msg->data, buf, len);
	fifo_write(&tx_fifo, msg);
	puts("Adding task to threadpool\n");
	thpool_add_work(tx_thpool, (void*)thread_socketSeverSend, NULL);

	// int rtn;
	// if (fdClient)
	// {
	// 	rtn = write(fdClient, buf, len);
	// 	if (rtn < 0)
	// 	{
	// 		fprintf(stdout,"ERROR writing to socket %d\n", fdClient);
	// 	}
	// }

	//printf("socketSeverSend--\n");	

	return 0;
}

/***************************************************************************************************
 * @fn      socketSeverSendAllclients
 *
 * @brief   Send a buffer to all clients.
 * @param   uint8* srpcMsg - message to be sent
 *
 * @return  Status
 */
int32 socketSeverSendAllclients(uint8* buf, uint32 len)
{
	int rtn;
	socketRecord_t *srchRec;

	// first client socket
	srchRec = socketRecordHead->next;

	// Stop when at the end or max is reached
	while (srchRec)
	{
		//fprintf(stdout,"SRPC_Send: client %d\n", cnt++);
		rtn = write(srchRec->socketFd, buf, len);
		if (rtn < 0)
		{
			fprintf(stdout,"ERROR writing to socket %d\n", srchRec->socketFd);
			fprintf(stdout,"closing client socket\n");
			//remove the record and close the socket
			deleteSocketRec(srchRec->socketFd);

			return rtn;
		}
		srchRec = srchRec->next;
	}

	return 0;
}

/***************************************************************************************************
 * @fn      socketSeverClose
 *
 * @brief   Closes the client connections.
 *
 * @return  Status
 */
void socketSeverClose(void)
{
	int fds[MAX_CLIENTS], idx = 0;

	socketSeverGetClientFds(fds, MAX_CLIENTS);

	while (socketSeverGetNumClients() > 1)
	{
		fprintf(stdout,"socketSeverClose: Closing socket fd:%d\n", fds[idx]);
		deleteSocketRec(fds[idx++]);
	}

	//Now remove the listening socket
	if (fds[0])
	{
		fprintf(stdout,"socketSeverClose: Closing the listening socket\n");
		close(fds[0]);
	}
}

