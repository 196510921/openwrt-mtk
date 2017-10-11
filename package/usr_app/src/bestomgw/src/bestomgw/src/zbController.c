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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <pthread.h>
#include "thpool.h"
#include <unistd.h>

#include "zbSocCmd.h"
#include "interface_devicelist.h"
#include "interface_grouplist.h"
#include "interface_scenelist.h"
#include "m1_protocol.h"

#define MAX_DB_FILENAMR_LEN 255

//全局变量	
threadpool thpool;//线程池
threadpool tx_thpool;//线程池
//int fdserwrite, fdread; //串口 写,读
 // long MAXLEN = 10*1024;//10KB
 // char sexepath[PATH_MAX];
 // char slogpath[PATH_MAX]; //log 文件路径
 // char sdbpath[PATH_MAX]; //db 文件路径
 // char sinipath[PATH_MAX]; //db 文件路径 

 // sqlite3 *gdb=NULL;
 	
// uint8_t tlIndicationCb(epInfo_t *epInfo);
// uint8_t newDevIndicationCb(epInfo_t *epInfo);
// uint8_t zclGetStateCb(uint8_t state, uint16_t nwkAddr, uint8_t endpoint);
// uint8_t zclGetLevelCb(uint8_t level, uint16_t nwkAddr, uint8_t endpoint);
// uint8_t zclGetHueCb(uint8_t hue, uint16_t nwkAddr, uint8_t endpoint);
// uint8_t zclGetSatCb(uint8_t sat, uint16_t nwkAddr, uint8_t endpoint);
// uint8_t zdoSimpleDescRspCb(epInfo_t *epInfo);
// uint8_t zdoLeaveIndCb(uint16_t nwkAddr);
static void printf_redirect(void);
static void socket_poll(void);

// static zbSocCallbacks_t zbSocCbs =
// { tlIndicationCb, // pfnTlIndicationCb - TouchLink Indication callback
// 		newDevIndicationCb, // pfnNewDevIndicationCb - New Device Indication callback
// 		zclGetStateCb, //pfnZclGetStateCb - ZCL response callback for get State
// 		zclGetLevelCb, //pfnZclGetLevelCb_t - ZCL response callback for get Level
// 		zclGetHueCb, // pfnZclGetHueCb - ZCL response callback for get Hue
// 		zclGetSatCb, //pfnZclGetSatCb - ZCL response callback for get Sat
// 		zdoSimpleDescRspCb, //pfnzdoSimpleDescRspCb - ZDO simple desc rsp
// 		zdoLeaveIndCb, // pfnZdoLeaveIndCb - ZDO Leave indication
// 		NULL,
// 		NULL
// 		};

#include "interface_srpcserver.h"
#include "socket_server.h"

// void usage(char* exeName)
// {
// 	fprintf(stdout,"Usage: ./%s <port>\n", exeName);
// 	fprintf(stdout,"Eample: ./%s /dev/ttyACM0\n", exeName);
// }

int main(int argc, char* argv[])
{
	int retval = 0;
	pthread_t t1,t2,t3;
	//int zbSoc_fd;
	//char dbFilename[MAX_DB_FILENAMR_LEN];

	//printf_redirect();
	fprintf(stdout,"%s -- %s %s\n", argv[0], __DATE__, __TIME__);
	SRPC_Init();
	m1_protocol_init();
	printf("1\n");
	int i;

	pthread_create(&t1,NULL,socket_poll,NULL);
	pthread_create(&t2,NULL,thread_socketSeverSend,NULL);
	pthread_create(&t3,NULL,delay_send_task,NULL);
	pthread_join(t1,NULL);
	pthread_join(t2,NULL);
	pthread_join(t3, NULL);

	/*init thread pool*/
	//puts("Making threadpool with 1 threads");
	/*接收线程*/
	//thpool = thpool_init(1);
	/*发送线程*/
	//tx_thpool = thpool_init(2);
	//puts("Killing threadpool");
	//thpool_destroy(thpool);
	
	return retval;
}

static void socket_poll(void)
{
	while (1)
	{
		int numClientFds = socketSeverGetNumClients();
		fprintf(stdout,stdout,"numClientFds:%d\n",numClientFds);
		//poll on client socket fd's and the ZllSoC serial port for any activity
		if (numClientFds)
		{
			fprintf(stdout,"numClientFds:%d\n",numClientFds);
			int pollFdIdx;
			int *client_fds = malloc(numClientFds * sizeof(int));
			//socket client FD's + zllSoC serial port FD
			struct pollfd *pollFds = malloc(
					((numClientFds + 1) * sizeof(struct pollfd)));

			if (client_fds && pollFds)
			{
				//Set the socket file descriptors
				socketSeverGetClientFds(client_fds, numClientFds);
				for (pollFdIdx = 0; pollFdIdx < numClientFds; pollFdIdx++)
				{
					pollFds[pollFdIdx].fd = client_fds[pollFdIdx];
					pollFds[pollFdIdx].events = POLLIN | POLLRDHUP;
					fprintf(stdout,"zllMain: adding fd %d to poll()\n", pollFds[pollFdIdx].fd);
				}

				fprintf(stdout,"zllMain: waiting for poll()\n");

				poll(pollFds, (numClientFds), -1);
				fprintf(stdout,"poll out\n");

				for (pollFdIdx = 0; pollFdIdx < numClientFds; pollFdIdx++)
				{
					if ((pollFds[pollFdIdx].revents))
					{
						fprintf(stdout,"Message from Socket Sever\n");
						socketSeverPoll(pollFds[pollFdIdx].fd, pollFds[pollFdIdx].revents);
					}
				}

				free(client_fds);
				free(pollFds);
				fprintf(stdout,"free client\n");

			}
		}
	}

}

static void printf_redirect(void)
{
	 fflush(stdout);  
     setvbuf(stdout,NULL,_IONBF,0);  
     printf("test stdout\n");  
     int save_fd = dup(STDOUT_FILENO); 
     int fd = open("test1.txt",(O_RDWR | O_CREAT), 0644);  
     dup2(fd,STDOUT_FILENO); 
     printf("test file\n");  
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

	fprintf(stdout,"zdoSimpleDescRspCb: NwkAddr:0x%04x\n End:0x%02x ", epInfo->nwkAddr,
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

	fprintf(stdout,"zdoSimpleDescRspCb: NwkAddr:0x%04x Ep:0x%02x Type:0x%02x ", epInfo->nwkAddr,
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

