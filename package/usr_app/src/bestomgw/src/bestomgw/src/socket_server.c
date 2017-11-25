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
#include <ifaddrs.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <errno.h>
#include <malloc.h>

//#include "thpool.h"
#include "socket_server.h"
#include "m1_protocol.h"
#include "buf_manage.h"

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
static int set_udp_broadcast_add(char* d);

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
			delete_account_conn_info(clinetFd);
			client_block_destory(clinetFd);
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
extern fifo_t tx_fifo;
void thread_socketSeverSend(void)
{
	int rtn;
	uint32_t* msg = NULL;
	while(1){
	    if(fifo_read(&tx_fifo, &msg) == 0){
	    	//fprintf(stdout,"fifo read failed\n");
	    	continue;
	    }
    
	    if(msg == NULL){
	    	//fprintf(stdout,"msg NULL\n");
	    	continue;
	    }
	    fprintf(stdout,"thread_socketSeverSend\n");	
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
	}

}

int32 socketSeverSend(uint8* buf, uint32 len, int32 fdClient)
{
fprintf(stdout, "socketSeverSend++\n");

	int rtn;
	uint16_t header = 0xFEFD;
	uint16_t msg_len = 0;
	char* send_buf = NULL;
	//char send_buf[4096] = {0};

	/*大端序*/
	header = (((header >> 8) & 0xff) | ((header << 8) & 0xff00)) & 0xffff;
	msg_len = (((len >> 8) & 0xff) | ((len << 8) & 0xff00)) & 0xffff;

	send_buf = (char*)malloc(len + 4);
	memcpy(send_buf, &header, 2);
	memcpy((send_buf+2), &msg_len, 2);
	memcpy((send_buf+4), buf, len);
	
	rtn = write(fdClient, send_buf, len + 4);
	// rtn = write(fdClient, buf, len);
	if (rtn < 0)
	{
		fprintf(stdout,"ERROR writing to socket %d\n", fdClient);
	}

	free(send_buf);
	fprintf(stdout, "socketSeverSend--\n");
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

/*获取本地ip*/
static int get_local_ip(char *ip)  
{  
    struct ifaddrs * ifAddrStruct=NULL;  
    void * tmpAddrPtr=NULL;  
  
    getifaddrs(&ifAddrStruct);  
  
    while (ifAddrStruct!=NULL)   
    {  
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET)  
        {   // check it is IP4  
            // is a valid IP4 Address  
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;    
            inet_ntop(AF_INET, tmpAddrPtr, ip, INET_ADDRSTRLEN);  
            fprintf(stdout,"%s IPV4 Address %s\n", ifAddrStruct->ifa_name, ip);   
        }  
        // else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6)  
        // {   // check it is IP6  
        //     // is a valid IP6 Address  
        //     tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;  
        //     char addressBuffer[INET6_ADDRSTRLEN];  
        //     inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);  
        //     printf("%s IPV6 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);   
        // }   
        ifAddrStruct = ifAddrStruct->ifa_next;  
    }  
    return 0;  
   
}  

/*udp 广播本机地址*/
int udp_broadcast_server(void)
{
	cJSON* pJsonRoot = NULL;
	pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        fprintf(stdout,"pJsonRoot NULL\n");
        cJSON_Delete(pJsonRoot);
        return M1_PROTOCOL_FAILED;
    }
	
	char msg[INET_ADDRSTRLEN];
	char ip[200];
	char udp_id[INET_ADDRSTRLEN];
  	int sock=-1;

  	get_local_ip(msg);

	if((sock=socket(AF_INET,SOCK_DGRAM,0))==-1)
	{
	   	fprintf(stderr, "udp socket error\n");  
	    return -1;
	}
	const int opt=-1;
	int nb=0;
	nb=setsockopt(sock,SOL_SOCKET,SO_BROADCAST,(char*)&opt,sizeof(opt));//设置套接字类型
	if(nb==-1)
	{
	    fprintf(stderr, "udp set socket error\n");  
	    return -1;
	}
	struct sockaddr_in addrto;
	bzero(&addrto,sizeof(struct sockaddr_in));
	addrto.sin_family=AF_INET;
	//addrto.sin_addr.s_addr=inet_addr("255.255.255.255");//htonl(INADDR_BROADCAST);//套接字地址为广播地址
	strcpy(udp_id, msg);
	set_udp_broadcast_add(udp_id);
	fprintf(stdout,"udp id:%s\n",udp_id);
	addrto.sin_addr.s_addr = inet_addr(udp_id);
	addrto.sin_port=htons(6000);//套接字广播端口号为6000
	int nlen=sizeof(addrto);
	/*创建Json*/
	cJSON_AddStringToObject(pJsonRoot, "ip", msg);
    cJSON_AddNumberToObject(pJsonRoot, "port", 11235);
    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        return;
    }
    strcpy(ip, p);
    cJSON_Delete(pJsonRoot);

	while(1)
	{
	    sleep(5);
	    int ret=sendto(sock,ip,strlen(ip),0,(struct sockaddr*)&addrto,nlen);//向广播地址发布消息
	    if(ret<0)
	    {
	        fprintf(stderr, "sendto failed with error\n");  
	        continue;
	    }
	}
	return 0;
}

static int set_udp_broadcast_add(char* d)
{
	char* str = ".";
	char* str1 = "255";
	char* p = NULL;
	int i = 0;

	p = d;
	for(i = 0; i < 3; i++){
		p = strstr(p,str);
		p+=1;
		if(p == NULL)
			return -1;
	}
	strcpy(p,str1);
	return 0;

}


