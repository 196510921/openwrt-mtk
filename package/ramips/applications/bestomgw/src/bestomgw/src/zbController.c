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
#if MCHECK
	#include <mcheck.h>
#endif
#include "m1_common_log.h"
#include "zbSocCmd.h"
#include "interface_devicelist.h"
#include "interface_grouplist.h"
#include "interface_scenelist.h"
#include "m1_protocol.h"
#include "interface_srpcserver.h"
#include "socket_server.h"
//#include "sql_operate.h"
//#include "sql_table.h"
#include "tcp_client.h"

#define MAX_DB_FILENAMR_LEN 255
#define DEBUG_LOG_OUTPUT_TO_FD   1
#define TCP_CLIENT_ENABLE   0
/*全局变量***********************************************************************************************/	
pthread_mutex_t mutex_lock;
pthread_mutex_t mutex_lock_sock;
/*静态变量****************************************************************************************/

/*静态局部函数****************************************************************************************/
static void printf_redirect(void);
static void socket_poll(void);
static void sql_test(void);

int main(int argc, char* argv[])
{
	int retval = 0;
	pthread_t t1,t2,t3,t4,t5;

	M1_LOG_INFO("%s -- %s %s\n", argv[0], __DATE__, __TIME__);
	//sql_test();
#if DEBUG_LOG_OUTPUT_TO_FD
	printf_redirect();
#endif
	/*屏蔽信号*/
	signal(SIGPIPE,SIG_IGN);
#if MCHECK
	/*追踪malloc*/
	setenv("MALLOC_TRACE", "malloc_user.log", 1);
	mtrace();
#endif

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
		//poll on client socket fd's and the ZllSoC serial port for any activity
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

				M1_LOG_INFO("zllMain: waiting for poll()\n");

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

static void printf_redirect(void)
{
	 fflush(stdout);  
     setvbuf(stdout,NULL,_IONBF,0);  
     printf("log to: /mnt/m1_debug_log.txt\n");  
     int save_fd = dup(STDOUT_FILENO); 
     //int fd = open("/home/ubuntu/share/test1.txt",(O_RDWR | O_CREAT), 0644);  
     int fd = open("/mnt/m1_debug_log.txt",(O_RDWR | O_CREAT), 0644);  
     if(fd == -1)
     	M1_LOG_ERROR( " open file failed\n");
     dup2(fd,STDOUT_FILENO); 
}

static void sql_test(void)
{
	//system("./sql_restore.sh");
}

uint8_t tlIndicationCb(epInfo_t *epInfo)
{
	epInfoExtended_t epInfoEx;

	epInfoEx.epInfo = epInfo;
	epInfoEx.type = EP_INFO_TYPE_NEW;

	devListAddDevice(epInfo);
	SRPC_SendEpInfo(&epInfoEx);

	return 0;
}

uint8_t newDevIndicationCb(epInfo_t *epInfo)
{
	//Just add to device list to store
	devListAddDevice(epInfo);

	return 0;
}

uint8_t zdoSimpleDescRspCb(epInfo_t *epInfo)
{
	epInfo_t *ieeeEpInfo;
	epInfo_t* oldRec;
	epInfoExtended_t epInfoEx;

	M1_LOG_DEBUG("zdoSimpleDescRspCb: NwkAddr:0x%04x\n End:0x%02x ", epInfo->nwkAddr,
			epInfo->endpoint);

	//find the IEEE address. Any ep (0xFF), if the is the first simpleDesc for this nwkAddr
	//then devAnnce will enter a dummy entry with ep=0, other wise get IEEE from a previous EP
	ieeeEpInfo = devListGetDeviceByNaEp(epInfo->nwkAddr, 0xFF);
	memcpy(epInfo->IEEEAddr, ieeeEpInfo->IEEEAddr, Z_EXTADDR_LEN);

	//remove dummy ep, the devAnnce will enter a dummy entry with ep=0,
	//this is only used for storing the IEEE address until the  first real EP
	//is enter.
	devListRemoveDeviceByNaEp(epInfo->nwkAddr, 0x00);

	//is this a new device or an update
	oldRec = devListGetDeviceByIeeeEp(epInfo->IEEEAddr, epInfo->endpoint);

	if (oldRec != NULL)
	{
		if (epInfo->nwkAddr != oldRec->nwkAddr)
		{
			epInfoEx.type = EP_INFO_TYPE_UPDATED;
			epInfoEx.prevNwkAddr = oldRec->nwkAddr;
			devListRemoveDeviceByNaEp(oldRec->nwkAddr, oldRec->endpoint); //theoretically, update the database record in place is possible, but this other approach is selected to provide change logging. Records that are marked as deleted soes not have to be phisically deleted (e.g. by avoiding consilidation) and thus the database can be used as connection log
		}
		else
		{
			//not checking if any of the records has changed. assuming that for a given device (ieee_addr+endpoint_number) nothing will change except the network address.
			epInfoEx.type = EP_INFO_TYPE_EXISTING;
		}
	}
	else
	{
		epInfoEx.type = EP_INFO_TYPE_NEW;
	}

	M1_LOG_DEBUG("zdoSimpleDescRspCb: NwkAddr:0x%04x Ep:0x%02x Type:0x%02x ", epInfo->nwkAddr,
			epInfo->endpoint, epInfoEx.type);

	if (epInfoEx.type != EP_INFO_TYPE_EXISTING)
	{
		devListAddDevice(epInfo);
		epInfoEx.epInfo = epInfo;
		SRPC_SendEpInfo(&epInfoEx);
	}

	return 0;
}

uint8_t zdoLeaveIndCb(uint16_t nwkAddr)
{
	epInfoExtended_t removeEpInfoEx;

	removeEpInfoEx.epInfo = devListRemoveDeviceByNaEp(nwkAddr, 0xFF);

	while(removeEpInfoEx.epInfo)
	{
		removeEpInfoEx.type = EP_INFO_TYPE_REMOVED;
		SRPC_SendEpInfo(&removeEpInfoEx);
		removeEpInfoEx.epInfo = devListRemoveDeviceByNaEp(nwkAddr, 0xFF);
	}

	return 0;
}

uint8_t zclGetStateCb(uint8_t state, uint16_t nwkAddr, uint8_t endpoint)
{
	SRPC_CallBack_getStateRsp(state, nwkAddr, endpoint, 0);
	return 0;
}

uint8_t zclGetLevelCb(uint8_t level, uint16_t nwkAddr, uint8_t endpoint)
{
	SRPC_CallBack_getLevelRsp(level, nwkAddr, endpoint, 0);
	return 0;
}

uint8_t zclGetHueCb(uint8_t hue, uint16_t nwkAddr, uint8_t endpoint)
{
	SRPC_CallBack_getHueRsp(hue, nwkAddr, endpoint, 0);
	return 0;
}

uint8_t zclGetSatCb(uint8_t sat, uint16_t nwkAddr, uint8_t endpoint)
{
	SRPC_CallBack_getSatRsp(sat, nwkAddr, endpoint, 0);
	return 0;
}


#if 0
static void test(void)
{
	stack_mem_t d[20];
	char buf[1024];
	int rc = 0;

	stack_block_init();
	int i;
	for(i = 0; i < 21; i++){
		rc = stack_block_req(&d[i]);
		if(rc == 0){
			M1_LOG_DEBUG("req failed\n");
			break;
		}
		M1_LOG_DEBUG("d.blockNum:%03d,d.start:%x,d.end:%x,d.wPtr:%x,d.rPtr:%x\n",d[i].blockNum,d[i].start,d[i].end,d[i].wPtr,d[i].rPtr);
		sprintf(d[i].wPtr,"--------------------------test:%d\n------------------------",i);
	}
	for(i = 0; i < 20; i++){
		M1_LOG_DEBUG("msg:%s\n",d[i].rPtr);
	}
	for(i = 0; i < 20; i+=2){
		rc = stack_block_destroy(d[i]);
		if(rc == 0){
			M1_LOG_DEBUG("destroy failed\n");
			break;
		}
	}
	for(i = 0; i < 21; i++){
		rc = stack_block_req(&d[i]);
		if(rc == 0){
			M1_LOG_DEBUG("req failed\n");
			continue;
		}
		M1_LOG_DEBUG("d.blockNum:%03d,d.start:%x,d.end:%x,d.wPtr:%x,d.rPtr:%x\n",d[i].blockNum,d[i].start,d[i].end,d[i].wPtr,d[i].rPtr);
		sprintf(d[i].wPtr,"--------------------------test:%d\n------------------------",i);
	}
	for(i = 0; i < 20; i++){
		M1_LOG_DEBUG("msg:%s\n",d[i].rPtr);
	}
}
#endif
