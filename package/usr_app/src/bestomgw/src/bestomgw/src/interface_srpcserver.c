/**************************************************************************************************
 * Filename:       interface_srpcserver.c
 
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
#include <malloc.h>

#include "interface_srpcserver.h"
#include "socket_server.h"
#include "m1_common_log.h"
#include "hal_defs.h"
#include "utils.h"
#include "m1_protocol.h"

static char tcpRxBuf[1024*60] = {0};

void SRPC_RxCB(int clientFd);
void SRPC_ConnectCB(int status);


/*********************************************************************
 * @fn          SRPC_ProcessIncoming
 *
 * @brief       This function processes incoming messages.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */

void SRPC_ProcessIncoming(uint8_t *pBuf, unsigned int nlen, uint32_t clientFd)
{
	M1_LOG_DEBUG("SRPC_ProcessIncoming:%s\n",pBuf);
	//extern threadpool thpool;
	//m1_package_t* package = malloc(sizeof(m1_package_t));
	//package->clientFd = clientFd;
	//package->data = malloc(nlen);
	
	//puts("Adding task to threadpool\n");
	//thpool_add_work(thpool, (void*)data_handle, (void*)&package);

	//data_handle(package);
}


void error(const char *msg)
{
	perror(msg);
	exit(1);
}

		
/***************************************************************************************************
 * @fn      SRPC_Init
 *
 * @brief   initialises the RPC interface and waitsfor a client to connect.
 * @param   
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_Init(void)
{
	if (socketSeverInit(SRPC_TCP_PORT) == -1)
	{
		//exit if the server does not start
		exit(-1);
	}

	serverSocketConfig(SRPC_RxCB, SRPC_ConnectCB);
}

/***************************************************************************************************
 * @fn      SRPC_ConnectCB
 *
 * @brief   Callback for connecting SRPC clients.
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_ConnectCB(int clientFd)
{
	printf("SRPC_ConnectCB++ \n");

	printf("connected success !\n");
}


/***************************************************************************************************
 * @fn      SRPC_RxCB
 *
 * @brief   Callback for Rx'ing SRPC messages.
 *
 * @return  Status
 ***************************************************************************************************/
#if 0
static int json_checker(char* str, int len)
{
	int i;
	int left = 0, right = 0;

	for(i = 0; i < len; i++){
		if(str+i == NULL)
			return 0;
		if(*(str + i) ==  '{') // '{':123
			left++;
		else if(*(str + i) ==  '}') // '}'125
			right++;
	}
	if((left == right) && left != 0 && right != 0)
		return 1;

	return 0;
}
#endif

/*接收包header高低字节转换*/
int msg_header_check(uint16_t header)
{
	int ret = 0;
	uint16_t TransHeader = 0;

	M1_LOG_INFO("rx header:%x\n",header);
	TransHeader = (((header >> 8) & 0xff) | ((header<< 8) & 0xff00));
	M1_LOG_DEBUG("TransHeader:%x\n",TransHeader);
	if(TransHeader == 0xFEFD){
		ret = 1;
	}

	return ret;
}
/*接收长度高低字节转换*/
uint16_t msg_len_get(uint16_t len)
{
	uint16_t TransLen = 0;
	
	TransLen = (((len >> 8) & 0xff) | ((len<< 8) & 0xff00));
	M1_LOG_DEBUG("TransLen:%x\n",TransLen);

	return TransLen;	
}

void SRPC_RxCB(int clientFd)
{
	int byteToRead = 0;
	int byteRead = 0;
	int rtn = 0;
	int rc = 0;
	static int len = 0;
	static uint16_t exLen = 0;

	m1_package_t msg;
	//client_block_t* client_block = NULL;

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
		byteRead = read(clientFd, tcpRxBuf+len, 1024);
		if(byteRead > 0){
			/*判断是否是头*/
			if(msg_header_check(*(uint16_t*)(tcpRxBuf + len)) == 1){
				exLen = msg_len_get(*(uint16_t*)(tcpRxBuf + len + 2));
				M1_LOG_DEBUG("exLen:%05d\n",exLen);
				if(len > 0){
					goto Finish;
				}
			}else{
				M1_LOG_DEBUG("%x,tcpRxBuf:%x,%x.%x.%x,\n",*(uint16_t*)(tcpRxBuf + len + 2),tcpRxBuf[len],tcpRxBuf[len+1],tcpRxBuf[len+2],tcpRxBuf[len+3]);
			}

			len += byteRead;
			byteToRead -= byteRead;		
		}					
	}

	if(len - 4 < exLen)
	{
		M1_LOG_INFO("waiting msg...\n");
		return;
	}

	M1_LOG_DEBUG("clientFd:%d,rx len:%05d, rx header:%x,%x,%x,%x, rx data:%s\n",clientFd, \
		len, tcpRxBuf[0],tcpRxBuf[1],tcpRxBuf[2],tcpRxBuf[3],tcpRxBuf+4);
	
	msg.clientFd = clientFd;
	msg.len = len;
	msg.data = &tcpRxBuf[4];

	data_handle(&msg);

	Finish:
	len = 0;
	exLen = 0;
	memset(tcpRxBuf, 0, 1024*60);

	M1_LOG_DEBUG("SRPC_RxCB--\n");

	return;
}
#if 0
void SRPC_RxCB(int clientFd)
{
	int byteToRead = 0;
	int byteRead = 0;
	int rtn = 0;
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
		byteRead = read(clientFd, tcpRxBuf+len, 1024);
		if(byteRead > 0){
			/*判断是否是头*/
			if(msg_header_check(*(uint16_t*)(tcpRxBuf + len)) == 1){
				exLen = msg_len_get(*(uint16_t*)(tcpRxBuf + len + 2));
				M1_LOG_DEBUG("exLen:%05d\n",exLen);
				if(len > 0){
					goto Finish;
				}
			}else{
				M1_LOG_DEBUG("%x,tcpRxBuf:%x,%x.%x.%x,\n",*(uint16_t*)(tcpRxBuf + len + 2),tcpRxBuf[len],tcpRxBuf[len+1],tcpRxBuf[len+2],tcpRxBuf[len+3]);
			}

			len += byteRead;
			byteToRead -= byteRead;		
		}					
	}

	if(len - 4 < exLen)
	{
		M1_LOG_INFO("waiting msg...\n");
		return;
	}

	client_block = client_stack_block_req(clientFd);
	if(NULL == client_block){
		M1_LOG_ERROR("client_block null\n");
		//return;
		goto Finish;
	}
	M1_LOG_DEBUG("clientFd:%d,rx len:%05d, rx header:%x,%x,%x,%x, rx data:%s\n",clientFd, \
		len, tcpRxBuf[0],tcpRxBuf[1],tcpRxBuf[2],tcpRxBuf[3],tcpRxBuf+4);
	rc = client_write(&client_block->stack_block, tcpRxBuf, len);
	if(rc != TCP_SERVER_SUCCESS)
		M1_LOG_ERROR("client_write failed\n");

	Finish:
	len = 0;
	exLen = 0;
	memset(tcpRxBuf, 0, 1024*60);

	M1_LOG_DEBUG("SRPC_RxCB--\n");

	return;
}
#endif

static void client_read_to_data_handle(char* data, int len, int clientFd)
{
	M1_LOG_INFO( "client_read_to_data_handle:  len:%05d, data:%s\n",len, data);

	m1_package_t msg;

	msg.clientFd = clientFd;
	msg.len = len;
	msg.data = data;

	data_handle(&msg);

}

/*clientfd stack block request****************************************************************************************/
extern pthread_mutex_t mutex_lock_sock;
static client_block_t client_block[STACK_BLOCK_NUM];

static int client_block_get_fd(int i);
static void client_block_set_fd(int clientFd, int i);

int client_block_init(void)
{
	M1_LOG_DEBUG( "client_block_init\n");
	int i;

	stack_block_init();

	for(i = 0; i < STACK_BLOCK_NUM; i++){
		client_block[i].clientFd = 0;
		// client_block[i].stack_block = NULL;
	}

	return TCP_SERVER_SUCCESS;
}

client_block_t* client_stack_block_req(int clientFd)
{
	//pthread_mutex_lock(&mutex_lock_sock);
	M1_LOG_DEBUG( "block_req\n");
	int i;
	int j = -1;
#if 0
	for(i = 0; i <  STACK_BLOCK_NUM; i++){
		if(-1 == j)
			if(0 == client_block[i].clientFd)
				j = i;

		if(clientFd == client_block[i].clientFd){
			//pthread_mutex_unlock(&mutex_lock_sock);
			return &client_block[i];
		}
	}

	client_block[j].clientFd = clientFd;
	if(TCP_SERVER_FAILED == stack_block_req(&client_block[j].stack_block)){
		M1_LOG_ERROR( "block_req failed\n");
		//pthread_mutex_unlock(&mutex_lock_sock);
		return NULL;
	}

	//pthread_mutex_unlock(&mutex_lock_sock);
	return &client_block[j];
#endif
	for(i = 0; i <  STACK_BLOCK_NUM; i++){
		if(-1 == j)
			if(0 == client_block_get_fd(i))
				j = i;

		if(clientFd == client_block_get_fd(i)){
			return &client_block[i];
		}
	}

	client_block_set_fd(clientFd, j);
	if(TCP_SERVER_FAILED == stack_block_req(&client_block[j].stack_block)){
		M1_LOG_ERROR( "block_req failed\n");
		return NULL;
	}

	return &client_block[j];
} 

int client_block_destory(int clientFd)
{
	M1_LOG_INFO( "block_destory\n");
	int i;

	for(i = 0; i <  STACK_BLOCK_NUM; i++)
	{

		if(clientFd == client_block_get_fd(i))
		{
			M1_LOG_INFO( "clientFd:%d destory\n",clientFd);	
			client_block_set_fd(0, i);
			if(TCP_SERVER_FAILED == stack_block_destroy(client_block[i].stack_block)){
				M1_LOG_ERROR("block_destroy failed\n");
				return TCP_SERVER_FAILED;
			}
		}
	}
	return TCP_SERVER_SUCCESS;
}

static int client_block_get_fd(int i)
{
	int clientFd = 0;
	pthread_mutex_lock(&mutex_lock_sock);
	clientFd =  client_block[i].clientFd;
	pthread_mutex_unlock(&mutex_lock_sock);
	return clientFd;
}

static void client_block_set_fd(int clientFd, int i)
{
	pthread_mutex_lock(&mutex_lock_sock);
	client_block[i].clientFd = clientFd;
	pthread_mutex_unlock(&mutex_lock_sock);
}

/*client write/read**************************************************************************************/
int client_write(stack_mem_t* d, char* data, int len)
{
	//pthread_mutex_lock(&mutex_lock_sock);
	M1_LOG_DEBUG("write begin: num:%d\n, d->wPtr:%x, d->rPtr:%05d,d->start:%05d,len:%05d, d->end:%x\n",d->blockNum,d->wPtr, d->rPtr, d->start, len, d->end);
	//M1_LOG_DEBUG("header:%x,%x,%x,%x,str:%s\n",*(uint8_t*)&data[0],*(uint8_t*)&data[1],data[2],data[3],&data[4]);
	if(NULL == d){
		M1_LOG_ERROR( "NULL == d\n");
		return TCP_SERVER_FAILED;
	}

	int rc = 0;
	uint16_t header = 0;
	uint16_t distance = 0;

	while(len > 0)
	{
		header = *(uint16_t*)data;
		header = (uint16_t)(((header << 8) & 0xff00) | ((header >> 8) & 0xff)) & 0xffff;
		if(header == MSG_HEADER)
		{
			distance = (*(uint16_t*)(data + BLOCK_LEN_OFFSET)) & 0xFFFF;
			distance = (((distance << 8) & 0xff00) | ((distance >> 8) & 0xff)) & 0xffff;
		}
		else
		{
			//distance = 0;
			return -1;
		}

		M1_LOG_DEBUG("len:%05d, distance:%05d\n",distance + 4, distance);
		rc = stack_push(d, data, distance + 4 ,distance);
		if(rc != TCP_SERVER_SUCCESS)
			M1_LOG_WARN( "client write failed\n");	
	
		data = data + distance + 4;
		len = len - distance - 4;	
	}
	

#if 0
	header = *(uint16_t*)data;
	header = (uint16_t)(((header << 8) & 0xff00) | ((header >> 8) & 0xff)) & 0xffff;
	if(header == MSG_HEADER){
		distance = (*(uint16_t*)(data + BLOCK_LEN_OFFSET)) & 0xFFFF;
		distance = (((distance << 8) & 0xff00) | ((distance >> 8) & 0xff)) & 0xffff;
	}

	M1_LOG_DEBUG("len:%05d, distance:%05d\n",len, distance);
	rc = stack_push(d, data, len ,distance);
	if(rc != TCP_SERVER_SUCCESS)
		M1_LOG_WARN( "client write failed\n");
#endif	
	M1_LOG_DEBUG("write end: num:%d\n, d->wPtr:%05d, d->rPtr:%05d,d->start:%05d,len:%05d, d->end:%05d\n",d->blockNum,d->wPtr, d->rPtr, d->start, len, d->end);
	//pthread_mutex_unlock(&mutex_lock_sock);
	return rc;
}

void* client_read(void)
{
	M1_LOG_DEBUG( "client_read\n");
	int i = 0;
	int rc = TCP_SERVER_SUCCESS;
	uint16_t len = 0;
	uint16_t header = 0;
	char* headerP = NULL;
	char data[60*1024] = {0};
	volatile stack_mem_t* d = NULL;

	while(1){
		//M1_LOG_DEBUG("-------------------------%d read----------------------------\n",i);
		// if(client_block[i].clientFd == 0){
		// 	goto Finish;
		// }
		if(client_block_get_fd(i) == 0)
		{
			goto Finish;
		}
		d = &client_block[i].stack_block;
		//M1_LOG_DEBUG( "read begin:d->rPtr:%05d, d->wPtr:%05d\n",d->rPtr, d->wPtr);
		//pthread_mutex_lock(&mutex_lock_sock);
		do{
			headerP = d->rPtr;
			rc = stack_pop(d, data, STACK_UNIT);
			if(rc != TCP_SERVER_SUCCESS){
				goto Finish;
			}
			header = *(uint16_t*)data;
			header = (uint16_t)(((header << 8) & 0xff00) | ((header >> 8) & 0xff)) & 0xffff;
			M1_LOG_DEBUG("read header:%x\n",header);
		}while(header != MSG_HEADER);


		len = *(uint16_t*)&data[2];
		len = (uint16_t)(((len << 8) & 0xff00) | ((len >> 8) & 0xff)) & 0xffff;

		M1_LOG_INFO( "clientFd: %d read,data[2]:%x,data[3]:%x,len:%05d,data:%s\n", client_block_get_fd(i), data[2], data[3],len,&data[4]);
	
		if((len+4) <= STACK_UNIT){
			client_read_to_data_handle(data + 4, len, client_block[i].clientFd);
			goto Finish;
		}

		rc = stack_pop(d, data + STACK_UNIT, len - STACK_UNIT + 4);
		if(rc != TCP_SERVER_SUCCESS){
			goto Finish;
		}
		client_read_to_data_handle(data + 4, len, client_block[i].clientFd);
		//pthread_mutex_unlock(&mutex_lock_sock);
		Finish:
		if(rc != TCP_SERVER_SUCCESS){
			d->rPtr = headerP;
		}
		i = (i + 1) % STACK_BLOCK_NUM;
		memset(data, 0, 60*1024);
		//M1_LOG_DEBUG( "read end:d->rPtr:%05d, d->wPtr:%05d\n",d->rPtr, d->wPtr);
		usleep(1000);
	}
}

/***************************************************************************************************
 * @fn      Closes the TCP port
 *
 * @brief   Send a message over SRPC.
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_Close(void)
{
	socketSeverClose();
}

