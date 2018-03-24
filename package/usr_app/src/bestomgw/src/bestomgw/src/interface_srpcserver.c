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
#include "interface_devicelist.h"
#include "interface_grouplist.h"
#include "interface_scenelist.h"
#include "m1_common_log.h"
//#include "thpool.h"

#include "hal_defs.h"

#include "zbSocCmd.h"
#include "utils.h"
#include "m1_protocol.h"
#include "buf_manage.h"

#define M1_DBG  1

void SRPC_RxCB(int clientFd);
void SRPC_ConnectCB(int status);

static uint8_t SRPC_setDeviceState(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_setDeviceLevel(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_setDeviceColor(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDeviceState(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDeviceLevel(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDeviceHue(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDeviceSat(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_bindDevices(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getGroups(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_addGroup(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getScenes(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_storeScene(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_recallScene(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_identifyDevice(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_close(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_getDevices(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_permitJoin(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_changeDeviceName(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_notSupported(uint8_t *pBuf, uint32_t clientFd);


//RPSC ZLL Interface call back functions
static void SRPC_CallBack_addGroupRsp(uint16_t groupId, char *nameStr,
		uint32_t clientFd);
static void SRPC_CallBack_addSceneRsp(uint16_t groupId, uint8_t sceneId,
		char *nameStr, uint32_t clientFd);

static uint8_t* srpcParseEpInfo(epInfoExtended_t* epInfoEx);

typedef uint8_t (*srpcProcessMsg_t)(uint8_t *pBuf, uint32_t clientFd);

srpcProcessMsg_t rpcsProcessIncoming[] =
{ SRPC_close, //SRPC_CLOSE
		SRPC_getDevices, //SRPC_GET_DEVICES
		SRPC_setDeviceState, //SRPC_SET_DEV_STATE
		SRPC_setDeviceLevel, //SRPC_SET_DEV_LEVEL
		SRPC_setDeviceColor, //SRPC_SET_DEV_COLOR
		SRPC_getDeviceState, //SRPC_GET_DEV_STATE
		SRPC_getDeviceLevel, //SRPC_GET_DEV_LEVEL
		SRPC_getDeviceHue, //SRPC_GET_DEV_HUE
		SRPC_getDeviceSat, //SRPC_GET_DEV_HUE
		SRPC_bindDevices, //SRPC_BIND_DEVICES
		SRPC_notSupported, //reserved
		SRPC_notSupported, //reserved
		SRPC_notSupported, //reserved
		SRPC_notSupported, //reserved
		SRPC_getGroups, //SRPC_GET_GROUPS
		SRPC_addGroup, //SRPC_ADD_GROUP
		SRPC_getScenes, //SRPC_GET_SCENES
		SRPC_storeScene, //SRPC_STORE_SCENE
		SRPC_recallScene, //SRPC_RECALL_SCENE
		SRPC_identifyDevice, //SRPC_IDENTIFY_DEVICE
		SRPC_changeDeviceName, //SRPC_CHANGE_DEVICE_NAME
		SRPC_notSupported, //SRPC_REMOVE_DEVICE
		SRPC_notSupported, //SRPC_RESERVED_10
		SRPC_notSupported, //SRPC_RESERVED_11
		SRPC_notSupported, //SRPC_RESERVED_12
		SRPC_notSupported, //SRPC_RESERVED_13
		SRPC_notSupported, //SRPC_RESERVED_14
		SRPC_notSupported, //SRPC_RESERVED_15
		SRPC_permitJoin, //SRPC_PERMIT_JOIN
		};
		
#define PROCESSINCOMINGLEN 29
		
char *cmdProcessIncoming[PROCESSINCOMINGLEN] = 
{
	"close",//SRPC_close, //SRPC_CLOSE
	"queryalldevice",//	SRPC_getDevices, //SRPC_GET_DEVICES
		SRPC_setDeviceState, //SRPC_SET_DEV_STATE
		SRPC_setDeviceLevel, //SRPC_SET_DEV_LEVEL
		SRPC_setDeviceColor, //SRPC_SET_DEV_COLOR
		SRPC_getDeviceState, //SRPC_GET_DEV_STATE
		SRPC_getDeviceLevel, //SRPC_GET_DEV_LEVEL
		SRPC_getDeviceHue, //SRPC_GET_DEV_HUE
		SRPC_getDeviceSat, //SRPC_GET_DEV_HUE
		SRPC_bindDevices, //SRPC_BIND_DEVICES
		SRPC_notSupported, //reserved
		SRPC_notSupported, //reserved
		SRPC_notSupported, //reserved
		SRPC_notSupported, //reserved
		SRPC_getGroups, //SRPC_GET_GROUPS
		SRPC_addGroup, //SRPC_ADD_GROUP
		SRPC_getScenes, //SRPC_GET_SCENES
		SRPC_storeScene, //SRPC_STORE_SCENE
		SRPC_recallScene, //SRPC_RECALL_SCENE
		SRPC_identifyDevice, //SRPC_IDENTIFY_DEVICE
		SRPC_changeDeviceName, //SRPC_CHANGE_DEVICE_NAME
		SRPC_notSupported, //SRPC_REMOVE_DEVICE
		SRPC_notSupported, //SRPC_RESERVED_10
		SRPC_notSupported, //SRPC_RESERVED_11
		SRPC_notSupported, //SRPC_RESERVED_12
		SRPC_notSupported, //SRPC_RESERVED_13
		SRPC_notSupported, //SRPC_RESERVED_14
		SRPC_notSupported, //SRPC_RESERVED_15
		SRPC_permitJoin, //SRPC_PERMIT_JOIN
};		

static uint8_t SRPC_get_gateway_information(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_get_zigbee_network_information(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_permit_join_network(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_reset_gateway(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_switch_online_state_of_gateway(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_get_short_address_of_all_device(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_quiry_all_device_detail_of_the_current_connection(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_report_new_device(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_heartbeat_packets(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_reqest_device_information(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_require_rssi(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_remove_device(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_update_device_name(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_set_device_switch_state(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_require_device_switch_state(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_set_light_brightness(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_require_light_brightness(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_set_light_hue_and_saturation(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_require_light_device_hue_and_saturation(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_require_light_color_temperature(uint8_t *pBuf, uint32_t clientFd);
static uint8_t SRPC_set_light_color_temperature(uint8_t *pBuf, uint32_t clientFd);

srpcProcessMsg_t myrpcsProcessIncoming[] =
{ 
		SRPC_get_gateway_information,  //srpc_get_gateway_information              					0x80;
		SRPC_get_zigbee_network_information,//        					0x81;
		SRPC_permit_join_network,	//       								0x82;	
		SRPC_reset_gateway,      			//						0x83;	
		SRPC_switch_online_state_of_gateway,//      					0x84;
		SRPC_get_short_address_of_all_device,//      					0x85;	
		SRPC_quiry_all_device_detail_of_the_current_connection,//      0x86;	
		SRPC_report_new_device,//        								0x87;
		SRPC_heartbeat_packets,//        								0x88;
		SRPC_notSupported, //srpc_reserved_1 											0x89;


		SRPC_reqest_device_information,//  							0x8a;
		SRPC_require_rssi,	// 										0x8b;
		SRPC_remove_device,//   									0x8c;
		SRPC_update_device_name,//         							0x8d;
		SRPC_set_device_switch_state,//        	 						0x8e;	
		SRPC_require_device_switch_state,//          					0x8f;	
		SRPC_set_light_brightness,//         							0x90;	
		SRPC_require_light_brightness,//        							0x91;	
		SRPC_set_light_hue_and_saturation,//       					0x92;	
		SRPC_require_light_device_hue_and_saturation,//    			0x93;	
		SRPC_notSupported,//srpc_reserved_2, 											0x94;
		SRPC_require_light_color_temperature,//      					0x95;
		SRPC_set_light_color_temperature, 			//				0x96;
};

static void srpcSend(uint8_t* srpcMsg, int fdClient);
static void srpcSendAll(uint8_t* srpcMsg);

/***************************************************************************************************
 * @fn      srpcParseEpInfp - Parse epInfo and prepare the SRPC message.
 *
 * @brief   Parse epInfo and prepare the SRPC message.
 * @param   epInfo_t* epInfo
 *
 * @return  pSrpcMessage
 ***************************************************************************************************/
static uint8_t* srpcParseEpInfo(epInfoExtended_t* epInfoEx)
{
	uint8_t i;
	uint8_t *pSrpcMessage, *pTmp, devNameLen = 0, pSrpcMessageLen;

	//printf("srpcParseEpInfo++\n");

	//RpcMessage contains function ID param Data Len and param data
	if (epInfoEx->epInfo->deviceName)
	{
		devNameLen = strlen(epInfoEx->epInfo->deviceName);
	}

	//sizre of EP infor - the name char* + num bytes of device name
	//pSrpcMessageLen = sizeof(epInfo_t) - sizeof(char*) + devNameLen;
	pSrpcMessageLen = 2 /* network address */
	+ 1 /* endpoint */
	+ 2 /* profile ID */
	+ 2 /* device ID */
	+ 1 /* version */
	+ 1 /* device name length */
	+ devNameLen /* device name */
	+ 1 /* status */
	+ 8 /* IEEE address */
	+ 1 /* type */
	+ 2 /* previous network address */
	+ 1; /* flags */
	pSrpcMessage = malloc(pSrpcMessageLen + 2);

	pTmp = pSrpcMessage;

	if (pSrpcMessage)
	{
		//Set func ID in SRPC buffer
		*pTmp++ = SRPC_NEW_DEVICE;
		//param size
		*pTmp++ = pSrpcMessageLen;

		*pTmp++ = LO_UINT16(epInfoEx->epInfo->nwkAddr);
		*pTmp++ = HI_UINT16(epInfoEx->epInfo->nwkAddr);
		*pTmp++ = epInfoEx->epInfo->endpoint;
		*pTmp++ = LO_UINT16(epInfoEx->epInfo->profileID);
		*pTmp++ = HI_UINT16(epInfoEx->epInfo->profileID);
		*pTmp++ = LO_UINT16(epInfoEx->epInfo->deviceID);
		*pTmp++ = HI_UINT16(epInfoEx->epInfo->deviceID);
		*pTmp++ = epInfoEx->epInfo->version;

		if (devNameLen > 0)
		{
			*pTmp++ = devNameLen;
			for (i = 0; i < devNameLen; i++)
			{
				*pTmp++ = epInfoEx->epInfo->deviceName[i];
			}
			printf("srpcParseEpInfo: name:%s\n", epInfoEx->epInfo->deviceName);
		}
		else
		{
			*pTmp++ = 0;
		}
		*pTmp++ = epInfoEx->epInfo->status;

		for (i = 0; i < 8; i++)
		{
			//printf("srpcParseEpInfp: IEEEAddr[%d] = %x\n", i, epInfo->IEEEAddr[i]);
			*pTmp++ = epInfoEx->epInfo->IEEEAddr[i];
		}

		*pTmp++ = epInfoEx->type;
		*pTmp++ = LO_UINT16(epInfoEx->prevNwkAddr);
		*pTmp++ = HI_UINT16(epInfoEx->prevNwkAddr);
		*pTmp++ = epInfoEx->epInfo->flags; //bit 0 : start, bit 1: end

	}
	//printf("srpcParseEpInfp--\n");

//  printf("srpcParseEpInfo %0x:%0x\n", epInfo->nwkAddr, epInfo->endpoint);   

	printf(
			"srpcParseEpInfo: %s device, nwkAddr=0x%04X, endpoint=0x%X, profileID=0x%04X, deviceID=0x%04X, flags=0x%02X",
			epInfoEx->type == EP_INFO_TYPE_EXISTING ? "EXISTING" :
			epInfoEx->type == EP_INFO_TYPE_NEW ? "NEW" : "UPDATED",
			epInfoEx->epInfo->nwkAddr, epInfoEx->epInfo->endpoint,
			epInfoEx->epInfo->profileID, epInfoEx->epInfo->deviceID,
			epInfoEx->epInfo->flags);

	if (epInfoEx->type == EP_INFO_TYPE_UPDATED)
	{
		printf(", prevNwkAddr=0x%04X\n", epInfoEx->prevNwkAddr);
	}

	printf("\n");

	return pSrpcMessage;
}

/***************************************************************************************************
 * @fn      srpcSend
 *
 * @brief   Send a message over SRPC to a clients.
 * @param   uint8_t* srpcMsg - message to be sent
 *
 * @return  Status
 ***************************************************************************************************/
static void srpcSend(uint8_t* srpcMsg, int fdClient)
{
	int rtn;

	rtn = socketSeverSend(srpcMsg, (srpcMsg[SRPC_MSG_LEN] + 2), fdClient);
	if (rtn < 0)
	{
		printf("ERROR writing to socket\n");
	}

	return;
}

/***************************************************************************************************
 * @fn      srpcSendAll
 *
 * @brief   Send a message over SRPC to all clients.
 * @param   uint8_t* srpcMsg - message to be sent
 *
 * @return  Status
 ***************************************************************************************************/
static void srpcSendAll(uint8_t* srpcMsg)
{
	int rtn;

	rtn = socketSeverSendAllclients(srpcMsg, (srpcMsg[SRPC_MSG_LEN] + 2));
	if (rtn < 0)
	{
		printf("ERROR writing to socket\n");
	}

	return;
}

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


/*********************************************************************
 * @fn          SRPC_addGroup
 *
 * @brief       This function exposes an interface to add a devices to a group.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_addGroup(uint8_t *pBuf, uint32_t clientFd)
{
	uint16_t dstAddr;
	uint8_t addrMode;
	uint8_t endpoint;
	char *nameStr;
	uint8_t nameLen;
	uint16_t groupId;

	//printf("SRPC_addGroup++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	nameLen = *pBuf++;
	nameStr = malloc(nameLen + 1);
	int i;
	for (i = 0; i < nameLen; i++)
	{
		nameStr[i] = *pBuf++;
	}
	nameStr[nameLen] = '\0';

	printf("SRPC_addGroup++: %x:%x:%x name[%d] %s \n", dstAddr, addrMode,
			endpoint, nameLen, nameStr);

	groupId = groupListAddDeviceToGroup(nameStr, dstAddr, endpoint);
	zbSocAddGroup(groupId, dstAddr, endpoint, addrMode);

	SRPC_CallBack_addGroupRsp(groupId, nameStr, clientFd);

	free(nameStr);

	//printf("SRPC_addGroup--\n");

	return 0;
}

/*********************************************************************
 * @fn          uint8_t SRPC_storeScene(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to store a scene.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_storeScene(uint8_t *pBuf, uint32_t clientFd)
{
	uint16_t dstAddr;
	uint8_t addrMode;
	uint8_t endpoint;
	char *nameStr;
	uint8_t nameLen;
	uint16_t groupId;
	uint8_t sceneId;

	//printf("SRPC_storeScene++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	groupId = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += 2;

	nameLen = *pBuf++;
	nameStr = malloc(nameLen + 1);
	int i;
	for (i = 0; i < nameLen; i++)
	{
		nameStr[i] = *pBuf++;
	}
	nameStr[nameLen] = '\0';

	printf("SRPC_storeScene++: name[%d] %s, group %d \n", nameLen, nameStr, groupId);

	sceneId = sceneListAddScene(nameStr, groupId);
	zbSocStoreScene(groupId, sceneId, dstAddr, endpoint, addrMode);
	SRPC_CallBack_addSceneRsp(groupId, sceneId, nameStr, clientFd);

	free(nameStr);

	//printf("SRPC_storeScene--: id:%x\n", sceneId);

	return 0;
}

/*********************************************************************
 * @fn          uint8_t SRPC_recallScene(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to recall a scene.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_recallScene(uint8_t *pBuf, uint32_t clientFd)
{
	uint16_t dstAddr;
	uint8_t addrMode;
	uint8_t endpoint;
	char *nameStr;
	uint8_t nameLen;
	uint16_t groupId;
	uint8_t sceneId;

	//printf("SRPC_recallScene++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	groupId = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += 2;

	nameLen = *pBuf++;
	nameStr = malloc(nameLen + 1);
	int i;
	for (i = 0; i < nameLen; i++)
	{
		nameStr[i] = *pBuf++;
	}
	nameStr[nameLen] = '\0';

	printf("SRPC_recallScene++: name[%d] %s, group %d \n", nameLen, nameStr, groupId);

	sceneId = sceneListGetSceneId(nameStr, groupId);
	zbSocRecallScene(groupId, sceneId, dstAddr, endpoint, addrMode);

	free(nameStr);

	//printf("SRPC_recallScene--: id:%x\n", sceneId);

	return 0;
}

/*********************************************************************
 * @fn          uint8_t SRPC_identifyDevice(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to make a device identify.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_identifyDevice(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;
	uint16_t identifyTime;

	//printf("SRPC_identifyDevice++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	identifyTime = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += 2;

	zbSocSendIdentify(identifyTime, dstAddr, endpoint, addrMode);

	return 0;
}

/*********************************************************************
 * @fn          SRPC_bindDevices
 *
 * @brief       This function exposes an interface to set a bind devices.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
uint8_t SRPC_bindDevices(uint8_t *pBuf, uint32_t clientFd)
{
	uint16_t srcNwkAddr;
	uint8_t srcEndpoint;
	uint8 srcIEEE[8];
	uint8_t dstEndpoint;
	uint8 dstIEEE[8];
	uint16 clusterId;

	//printf("SRPC_bindDevices++\n");

	//increment past SRPC header
	pBuf += 2;

	/* Src Address */
	srcNwkAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
	pBuf += 2;

	srcEndpoint = *pBuf++;

	memcpy(srcIEEE, pBuf, Z_EXTADDR_LEN);
	pBuf += Z_EXTADDR_LEN;

	dstEndpoint = *pBuf++;

	memcpy(dstIEEE, pBuf, Z_EXTADDR_LEN);
	pBuf += Z_EXTADDR_LEN;

	clusterId = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += 2;

	zbSocBind(srcNwkAddr, srcEndpoint, srcIEEE, dstEndpoint, dstIEEE, clusterId);

	return 0;
}

/*********************************************************************
 * @fn          SRPC_setDeviceState
 *
 * @brief       This function exposes an interface to set a devices on/off attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_setDeviceState(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;
	bool state;

	//printf("SRPC_setDeviceState++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	state = (bool) *pBuf;

	//printf("SRPC_setDeviceState: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x, state=%x\n", dstAddr, endpoint, addrMode, state);

	// Set light state on/off
	zbSocSetState(state, dstAddr, endpoint, addrMode);

	//printf("SRPC_setDeviceState--\n");

	return 0;
}

/*********************************************************************
 * @fn          SRPC_setDeviceLevel
 *
 * @brief       This function exposes an interface to set a devices level attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_setDeviceLevel(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;
	uint8_t level;
	uint16_t transitionTime;

	//printf("SRPC_setDeviceLevel++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	level = *pBuf++;

	transitionTime = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += 2;

	//printf("SRPC_setDeviceLevel: dstAddr.addr.shortAddr=%x ,level=%x, tr=%x \n", dstAddr, level, transitionTime);

	zbSocSetLevel(level, transitionTime, dstAddr, endpoint, addrMode);

	//printf("SRPC_setDeviceLevel--\n");

	return 0;
}

/*********************************************************************
 * @fn          SRPC_setDeviceColor
 *
 * @brief       This function exposes an interface to set a devices hue and saturation attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_setDeviceColor(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;
	uint8_t hue, saturation;
	uint16_t transitionTime;

	//printf("SRPC_setDeviceColor++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	hue = *pBuf++;

	saturation = *pBuf++;

	transitionTime = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += 2;

//  printf("SRPC_setDeviceColor: dstAddr=%x ,hue=%x, saturation=%x, tr=%x \n", dstAddr, hue, saturation, transitionTime); 

	zbSocSetHueSat(hue, saturation, transitionTime, dstAddr, endpoint, addrMode);

	//printf("SRPC_setDeviceColor--\n");

	return 0;
}

/*********************************************************************
 * @fn          SRPC_getDeviceState
 *
 * @brief       This function exposes an interface to get a devices on/off attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDeviceState(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;

	//printf("SRPC_getDeviceState++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	//printf("SRPC_getDeviceState: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x", dstAddr, endpoint, addrMode);

	// Get light state on/off
	zbSocGetState(dstAddr, endpoint, addrMode);

	//printf("SRPC_getDeviceState--\n");

	return 0;
}

/*********************************************************************
 * @fn          SRPC_getDeviceLevel
 *
 * @brief       This function exposes an interface to get a devices level attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDeviceLevel(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;

	//printf("SRPC_getDeviceLevel++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	//printf("SRPC_getDeviceLevel: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x\n", dstAddr, endpoint, addrMode);

	// Get light level
	zbSocGetLevel(dstAddr, endpoint, addrMode);

	//printf("SRPC_getDeviceLevel--\n");

	return 0;
}

/*********************************************************************
 * @fn          SRPC_getDeviceHue
 *
 * @brief       This function exposes an interface to get a devices hue attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDeviceHue(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;

	//printf("SRPC_getDeviceHue++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	//printf("SRPC_getDeviceHue: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x\n", dstAddr, endpoint, addrMode);

	// Get light hue
	zbSocGetHue(dstAddr, endpoint, addrMode);

	//printf("SRPC_getDeviceHue--\n");

	return 0;
}

/*********************************************************************
 * @fn          SRPC_getDeviceSat
 *
 * @brief       This function exposes an interface to get a devices sat attribute.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDeviceSat(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;

	//printf("SRPC_getDeviceSat++\n");

	//increment past SRPC header
	pBuf += 2;

	addrMode = (afAddrMode_t) *pBuf++;
	dstAddr = BUILD_UINT16(pBuf[0], pBuf[1]);
	pBuf += Z_EXTADDR_LEN;
	endpoint = *pBuf++;
	// index past panId
	pBuf += 2;

	//printf("SRPC_getDeviceSat: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x\n", dstAddr, endpoint, addrMode);

	// Get light sat
	zbSocGetSat(dstAddr, endpoint, addrMode);

	//printf("SRPC_getDeviceSat--\n");

	return 0;
}

/*********************************************************************
 * @fn          SRPC_changeDeviceName
 *
 * @brief       This function exposes an interface to set a bind devices.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_changeDeviceName(uint8_t *pBuf, uint32_t clientFd)
{
	uint16_t devNwkAddr;
	uint8_t devEndpoint;
	uint8_t nameLen;
	epInfo_t * epInfo;

	printf("RSPC_ZLL_changeDeviceName++\n");

	//increment past SRPC header
	pBuf += 2;

	// Src Address
	devNwkAddr = BUILD_UINT16( pBuf[0], pBuf[1] );
	pBuf += 2;

	devEndpoint = *pBuf++;

	nameLen = *pBuf++;
	if (nameLen > MAX_SUPPORTED_DEVICE_NAME_LENGTH)
	{
		nameLen = MAX_SUPPORTED_DEVICE_NAME_LENGTH;
	}

	printf("RSPC_ZLL_changeDeviceName: renaming device %s, len=%d\n", pBuf,
			nameLen);
	printf("RSPC_ZLL_changeDeviceName: devNwkAddr=%x, devEndpoint=%x\n",
			devNwkAddr, devEndpoint);

	epInfo = devListRemoveDeviceByNaEp(devNwkAddr, devEndpoint);
	if (epInfo != NULL)
	{
		/*
		 if(epInfo->deviceName != NULL)
		 {
		 free(epInfo->deviceName);
		 }*/
		epInfo->deviceName = malloc(nameLen + 1);
		printf("RSPC_ZLL_changeDeviceName: renamed device %s, len=%d\n", pBuf,
				nameLen);
		strncpy(epInfo->deviceName, (char *) pBuf, nameLen);
		epInfo->deviceName[nameLen] = '\0';
		devListAddDevice(epInfo);
	}

	return 0;
}

/*********************************************************************
 * @fn          SRPC_close
 *
 * @brief       This function exposes an interface to allow an upper layer to close the interface.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
uint8_t SRPC_close(uint8_t *pBuf, uint32_t clientFd)
{
	uint16_t magicNumber;

	//printf("SRPC_close++\n");

	//increment past SRPC header
	pBuf += 2;

	magicNumber = BUILD_UINT16(pBuf[0], pBuf[1]);

	if (magicNumber == CLOSE_AUTH_NUM)
	{
		//Close the application
		socketSeverClose();
		//TODO: Need to create API's and close other fd's

		exit(EXIT_SUCCESS);
	}

	return 0;
}

/*********************************************************************
 * @fn          uint8_t SRPC_permitJoin(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to permit other devices to join the network.
 *
 * @param       pBuf - incomin messages
 *
 * @return      none
 */
static uint8_t SRPC_permitJoin(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t duration;
	uint16_t magicNumber;

	//increment past SRPC header
	pBuf += 2;

	duration = *pBuf++;
	magicNumber = BUILD_UINT16(pBuf[0], pBuf[1]);

	if (magicNumber == JOIN_AUTH_NUM)
	{
		//Open the network for joining
		zbSocOpenNwk(duration);
	}

	return 0;
}

/*********************************************************************
 * @fn          uint8_t SRPC_getGroups(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to get the group list.
 *
 * @param       pBuf - incomin messages
 *
 * @return      none
 */
static uint8_t SRPC_getGroups(uint8_t *pBuf, uint32_t clientFd)
{
	uint32_t context = 0;
	groupRecord_t *group = groupListGetNextGroup(&context);
	uint8_t nameLen, nameIdx;

	//printf("SRPC_getGroups++\n");

	while (group != NULL)
	{
		uint8_t *pSrpcMessage, *pBuf;
		//printf("SRPC_getGroups: group != null\n");

		//printf("SRPC_getGroups: malloc'ing %d bytes\n", (2 + (sizeof(uint16_t)) + ((uint8_t) (group->groupNameStr[0]))));

		//RpcMessage contains function ID param Data Len and param data
		//2 (SRPC header) + sizeof(uint16_t) (GroupId) + sizeof(uint8_t) (GroupName Len) + group->groupNameStr[0] (string)
		pSrpcMessage = malloc(
				2 + (sizeof(uint16_t)) + sizeof(uint8_t) + ((uint8_t)(group->name[0])));

		pBuf = pSrpcMessage;

		//Set func ID in RPCS buffer
		*pBuf++ = SRPC_GET_GROUP_RSP;
		//param size
		//sizeof(uint16_t) (GroupId) + sizeof(uint8_t) (SceneId) + sizeof(uint8_t) (GroupName Len) + group->groupNameStr[0] (string)
		*pBuf++ = (sizeof(uint16_t) + sizeof(uint8_t) + group->name[0]);

		*pBuf++ = group->id & 0xFF;
		*pBuf++ = (group->id & 0xFF00) >> 8;

		nameLen = strlen(group->name);
		if (nameLen > 0)
		{
			*pBuf++ = nameLen;
			for (nameIdx = 0; nameIdx < nameLen; nameIdx++)
			{
				*pBuf++ = group->name[nameIdx];
			}
			printf("SRPC_getGroups: name:%s\n", group->name);
		}
		else
		{
			*pBuf++ = 0;
		}

		//printf("SRPC_CallBack_addGroupRsp: groupName[%d] %s\n", group->groupNameStr[0], &(group->groupNameStr[1]));
		//Send SRPC
		srpcSend(pSrpcMessage, clientFd);
		free(pSrpcMessage);
		//get next group (NULL if all done)
		group = groupListGetNextGroup(&context);
	}
	//printf("SRPC_getGroups--\n");

	return 0;
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_addGroupRsp
 *
 * @brief   Sends the groupId to the client after a groupAdd
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_addGroupRsp(uint16_t groupId, char *nameStr,
		uint32_t clientFd)
{
	uint8_t *pSrpcMessage, *pBuf;

	//printf("SRPC_CallBack_addGroupRsp++\n");

	//printf("SRPC_CallBack_addGroupRsp: malloc'ing %d bytes\n", 2 + 3 + nameStr[0]);
	//RpcMessage contains function ID param Data Len and param data
	pSrpcMessage = malloc(2 + 3 + strlen(nameStr));

	pBuf = pSrpcMessage;

	//Set func ID in SRPC buffer
	*pBuf++ = SRPC_ADD_GROUP_RSP;
	//param size
	*pBuf++ = 3 + strlen(nameStr);

	*pBuf++ = groupId & 0xFF;
	*pBuf++ = (groupId & 0xFF00) >> 8;

	*pBuf++ = strlen(nameStr);
	int i;
	for (i = 0; i < strlen(nameStr); i++)
	{
		*pBuf++ = nameStr[i];
	}

	printf("SRPC_CallBack_addGroupRsp: groupName[%d] %s\n", strlen(nameStr),
			nameStr);
	//Send SRPC
	srpcSend(pSrpcMessage, clientFd);

	//printf("SRPC_CallBack_addGroupRsp--\n");

	return;
}

/*********************************************************************
 * @fn          uint8_t SRPC_getScenes(uint8_t *pBuf, uint32_t clientFd)
 *
 * @brief       This function exposes an interface to get the scenes defined for a group.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getScenes(uint8_t *pBuf, uint32_t clientFd)
{
	uint32_t context = 0;
	sceneRecord_t *scene = sceneListGetNextScene(&context);
	uint8_t nameLen, nameIdx;

	//printf("SRPC_getScenes++\n");

	while (scene != NULL)
	{
		uint8_t *pSrpcMessage, *pBuf;
		//printf("SRPC_getScenes: scene != null\n");

		//printf("SRPC_getScenes: malloc'ing %d bytes\n", (2 + (sizeof(uint16_t)) + (sizeof(uint8_t)) + (sizeof(uint8_t)) + ((uint8_t) (scene->sceneNameStr[0]))));

		//RpcMessage contains function ID param Data Len and param data
		//2 (SRPC header) + sizeof(uint16_t) (GroupId) + sizeof(uint8_t) (SceneId) + sizeof(uint8_t) (SceneName Len) + scene->sceneNameStr[0] (string)
		pSrpcMessage = malloc(
				2 + (sizeof(uint16_t)) + (sizeof(uint8_t)) + (sizeof(uint8_t))
						+ ((uint8_t)(scene->name[0])));

		pBuf = pSrpcMessage;

		//Set func ID in RPCS buffer
		*pBuf++ = SRPC_GET_SCENE_RSP;
		//param size
		//sizeof(uint16_t) (GroupId) + sizeof(uint8_t) (SceneId) + sizeof(uint8_t) (SceneName Len) + scene->sceneNameStr[0] (string)
		*pBuf++ = (sizeof(uint16_t) + (sizeof(uint8_t)) + (sizeof(uint8_t))
				+ scene->name[0]);

		*pBuf++ = scene->groupId & 0xFF;
		*pBuf++ = (scene->groupId & 0xFF00) >> 8;

		*pBuf++ = scene->sceneId;

		nameLen = strlen(scene->name);
		if (nameLen > 0)
		{
			*pBuf++ = nameLen;
			for (nameIdx = 0; nameIdx < nameLen; nameIdx++)
			{
				*pBuf++ = scene->name[nameIdx];
			}
			printf("SRPC_getScenes: name:%s\n", scene->name);
		}
		else
		{
			*pBuf++ = 0;
		}

		//printf("SRPC_getScenes: sceneName[%d] %s, groupId %x\n", scene->sceneNameStr[0], &(scene->sceneNameStr[1]), scene->groupId);
		//Send SRPC
		srpcSend(pSrpcMessage, clientFd);
		free(pSrpcMessage);
		//get next scene (NULL if all done)
		scene = sceneListGetNextScene(&context);
	}
	//printf("SRPC_getScenes--\n");

	return 0;
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_addSceneRsp
 *
 * @brief   Sends the sceneId to the client after a storeScene
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_addSceneRsp(uint16_t groupId, uint8_t sceneId, char *nameStr,
		uint32_t clientFd)
{
	uint8_t *pSrpcMessage, *pBuf;

	//printf("SRPC_CallBack_addSceneRsp++\n");

	//printf("SRPC_CallBack_addSceneRsp: malloc'ing %d bytes\n", 2 + 4 + nameStr[0]);
	//RpcMessage contains function ID param Data Len and param data
	pSrpcMessage = malloc(2 + 4 + nameStr[0]);

	pBuf = pSrpcMessage;

	//Set func ID in SRPC buffer
	*pBuf++ = SRPC_ADD_SCENE_RSP;
	//param size
	*pBuf++ = 4 + nameStr[0];

	*pBuf++ = groupId & 0xFF;
	*pBuf++ = (groupId & 0xFF00) >> 8;

	*pBuf++ = sceneId & 0xFF;

	*pBuf++ = nameStr[0];
	int i;
	for (i = 0; i < nameStr[0]; i++)
	{
		*pBuf++ = nameStr[i + 1];
	}

	//printf("SRPC_CallBack_addSceneRsp: groupName[%d] %s\n", nameStr[0], nameStr);
	//Send SRPC
	srpcSend(pSrpcMessage, clientFd);

	//printf("SRPC_CallBack_addSceneRsp--\n");

	return;
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_getStateRsp
 *
 * @brief   Sends the get State Rsp to the client that sent a get state
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_getStateRsp(uint8_t state, uint16_t srcAddr,
		uint8_t endpoint, uint32_t clientFd)
{
	uint8_t * pBuf;
	uint8_t pSrpcMessage[2 + 4];

	//printf("SRPC_CallBack_getStateRsp++\n");

	//RpcMessage contains function ID param Data Len and param data

	pBuf = pSrpcMessage;

	//Set func ID in RPCS buffer
	*pBuf++ = SRPC_GET_DEV_STATE_RSP;
	//param size
	*pBuf++ = 4;

	*pBuf++ = srcAddr & 0xFF;
	*pBuf++ = (srcAddr & 0xFF00) >> 8;
	*pBuf++ = endpoint;
	*pBuf++ = state & 0xFF;

	//printf("SRPC_CallBack_getStateRsp: state=%x\n", state);

	//Store the device that sent the request, for now send to all clients
	srpcSendAll(pSrpcMessage);

	//printf("SRPC_CallBack_addSceneRsp--\n");

	return;
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_getLevelRsp
 *
 * @brief   Sends the get Level Rsp to the client that sent a get level
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_getLevelRsp(uint8_t level, uint16_t srcAddr,
		uint8_t endpoint, uint32_t clientFd)
{
	uint8_t * pBuf;
	uint8_t pSrpcMessage[2 + 4];

	//printf("SRPC_CallBack_getLevelRsp++\n");

	//RpcMessage contains function ID param Data Len and param data

	pBuf = pSrpcMessage;

	//Set func ID in RPCS buffer
	*pBuf++ = SRPC_GET_DEV_LEVEL_RSP;
	//param size
	*pBuf++ = 4;

	*pBuf++ = srcAddr & 0xFF;
	*pBuf++ = (srcAddr & 0xFF00) >> 8;
	*pBuf++ = endpoint;
	*pBuf++ = level & 0xFF;

	//printf("SRPC_CallBack_getLevelRsp: level=%x\n", level);

	//Store the device that sent the request, for now send to all clients
	srpcSendAll(pSrpcMessage);

	//printf("SRPC_CallBack_getLevelRsp--\n");

	return;
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_getHueRsp
 *
 * @brief   Sends the get Hue Rsp to the client that sent a get hue
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_getHueRsp(uint8_t hue, uint16_t srcAddr, uint8_t endpoint,
		uint32_t clientFd)
{
	uint8_t * pBuf;
	uint8_t pSrpcMessage[2 + 4];

	//printf("SRPC_CallBack_getLevelRsp++\n");

	//RpcMessage contains function ID param Data Len and param data

	pBuf = pSrpcMessage;

	//Set func ID in RPCS buffer
	*pBuf++ = SRPC_GET_DEV_HUE_RSP;
	//param size
	*pBuf++ = 4;

	*pBuf++ = srcAddr & 0xFF;
	*pBuf++ = (srcAddr & 0xFF00) >> 8;
	*pBuf++ = endpoint;
	*pBuf++ = hue & 0xFF;

	//printf("SRPC_CallBack_getHueRsp: hue=%x\n", hue);

	//Store the device that sent the request, for now send to all clients
	srpcSendAll(pSrpcMessage);

	//printf("SRPC_CallBack_getHueRsp--\n");

	return;
}

/***************************************************************************************************
 * @fn      SRPC_CallBack_getSatRsp
 *
 * @brief   Sends the get Sat Rsp to the client that sent a get sat
 *
 * @return  Status
 ***************************************************************************************************/
void SRPC_CallBack_getSatRsp(uint8_t sat, uint16_t srcAddr, uint8_t endpoint,
		uint32_t clientFd)
{
	uint8_t * pBuf;
	uint8_t pSrpcMessage[2 + 4];

	//printf("SRPC_CallBack_getSatRsp++\n");

	//RpcMessage contains function ID param Data Len and param data

	pBuf = pSrpcMessage;

	//Set func ID in RPCS buffer
	*pBuf++ = SRPC_GET_DEV_SAT_RSP;
	//param size
	*pBuf++ = 4;

	*pBuf++ = srcAddr & 0xFF;
	*pBuf++ = (srcAddr & 0xFF00) >> 8;
	*pBuf++ = endpoint;
	*pBuf++ = sat & 0xFF;

	//printf("SRPC_CallBack_getSatRsp: sat=%x\n", sat);

	//Store the device that sent the request, for now send to all clients
	srpcSendAll(pSrpcMessage);

	//printf("SRPC_CallBack_getSatRsp--\n");

	return;
}

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

/*********************************************************************
 * @fn          SRPC_getDevices
 *
 * @brief       This function exposes an interface to allow an upper layer to start device discovery.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_getDevices(uint8_t *pBuf, uint32_t clientFd)
{
	epInfoExtended_t epInfoEx;
	uint32_t context = 0;
	uint8_t *pSrpcMessage;

	//printf("SRPC_getDevices++ \n");

	while ((epInfoEx.epInfo = devListGetNextDev(&context)) != NULL)
	{
		epInfoEx.type = EP_INFO_TYPE_EXISTING;
		epInfoEx.prevNwkAddr = 0xFFFF;
		epInfoEx.epInfo->flags = MT_NEW_DEVICE_FLAGS_NONE;

		//Send epInfo
		pSrpcMessage = srpcParseEpInfo(&epInfoEx);
		printf("SRPC_getDevices: %x:%x:%x:%x:%x\n", epInfoEx.epInfo->nwkAddr, epInfoEx.epInfo->endpoint,
				epInfoEx.epInfo->profileID, epInfoEx.epInfo->deviceID, epInfoEx.type);
		//Send SRPC
		srpcSend(pSrpcMessage, clientFd);
		free(pSrpcMessage);

		usleep(1000);

		//get next device (NULL if all done)
//printf("--?--> 0x%04X, 0x%02X", (uint32_t)epInfoEx.epInfo->nwkAddr, (uint32_t)epInfoEx.epInfo->endpoint);
//printf(" --!--> 0x%08X\n", (uint32_t)epInfoEx.epInfo);
	}

	return 0;
}

/*********************************************************************
 * @fn          SRPC_notSupported
 *
 * @brief       This function is called for unsupported commands.
 *
 * @param       pBuf - incomin messages
 *
 * @return      afStatus_t
 */
static uint8_t SRPC_notSupported(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}

static uint8_t SRPC_get_gateway_information(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}
static uint8_t SRPC_get_zigbee_network_information(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}
static uint8_t SRPC_permit_join_network(uint8_t *pBuf, uint32_t clientFd)
{
	uint8_t endpoint, addrMode;
	uint16_t dstAddr;
	bool state;

       uint8_t duration;
      duration = 0xFF; // 60Ãë=0x3C  25 Ãë =0x19  0xFF;  //ÓÀ¾Ã´ò¿ª
	zbSocOpenNwk(duration);

	return 0;
}
static uint8_t SRPC_reset_gateway(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}     									
static uint8_t SRPC_switch_online_state_of_gateway(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}     	
static uint8_t SRPC_get_short_address_of_all_device(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}  
static uint8_t SRPC_quiry_all_device_detail_of_the_current_connection(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}  
static uint8_t SRPC_report_new_device(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}  
static uint8_t SRPC_heartbeat_packets(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}  		
	
static uint8_t SRPC_reqest_device_information(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}  		
static uint8_t SRPC_require_rssi(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}  	

static uint8_t SRPC_remove_device(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}  
static uint8_t SRPC_update_device_name(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}  		
static uint8_t SRPC_set_device_switch_state(uint8_t *pBuf, uint32_t clientFd)
{

	uint8_t endpoint, addrMode;
	uint16_t dstAddr;
	bool state;

	//printf("SRPC_setDeviceState++\n");

	//increment past SRPC header
	//pBuf += 2;

	//addrMode = (afAddrMode_t) *pBuf++;
	addrMode = 0x02; //
	//   3A 00 1A 5A 54 5A 46 31 30 30 30 FE 8E 02 79 C3 0B 00 00 00 00 00 00 00 00 00 *
	dstAddr = BUILD_UINT16(pBuf[14], pBuf[15]);
	//pBuf += Z_EXTADDR_LEN;
	//endpoint = *pBuf++;
	endpoint = pBuf[16];
	// index past panId
	//pBuf += 2;

	//state = (bool) *pBuf;
	state = (bool) pBuf[17];

	//printf("SRPC_setDeviceState: dstAddr.addr.shortAddr=%x, endpoint=%x dstAddr.mode=%x, state=%x\n", dstAddr, endpoint, addrMode, state);
		printf("-->%s,%d \n",__FUNCTION__,__LINE__);
	// Set light state on/off
	zbSocSetState(state, dstAddr, endpoint, addrMode);
	return 0;
}  	
static uint8_t SRPC_require_device_switch_state(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}  
static uint8_t SRPC_set_light_brightness(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}  
static uint8_t SRPC_require_light_brightness(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}  
static uint8_t SRPC_set_light_hue_and_saturation(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}  
static uint8_t SRPC_require_light_device_hue_and_saturation(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}  
static uint8_t SRPC_require_light_color_temperature(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
}  
static uint8_t SRPC_set_light_color_temperature(uint8_t *pBuf, uint32_t clientFd)
{
	return 0;
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

/*********************************************************************
 * @fn          SRPC_SendEpInfo
 *
 * @brief       This function exposes an interface to allow an upper layer to start send an ep indo to all devices.
 *
 * @param       epInfo - pointer to the epInfo to be sent
 *
 * @return      afStatus_t
 */
uint8_t SRPC_SendEpInfo(epInfoExtended_t *epInfoEx)
{ 
  uint8_t *pSrpcMessage = srpcParseEpInfo(epInfoEx);  
  
  printf("SRPC_SendEpInfo++ %x:%x:%x:%x\n", epInfoEx->epInfo->nwkAddr, epInfoEx->epInfo->endpoint, epInfoEx->epInfo->profileID, epInfoEx->epInfo->deviceID);
    
  //Send SRPC
  srpcSendAll(pSrpcMessage);  
  free(pSrpcMessage); 
  printf("RSPC_SendEpInfo--\n");
  
  return 0;  
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
// int msg_header_checker(char*str, int len){
// 	int i = 0,j = 0, _len = 0;

// 	_len = len;
// 	printf("msg_header_checker\n");
// 	while(*(str + i) != '{'){ 
// 		printf("1. %c, i:%03d\n",*(str + i), i);
// 		i++;
// 		_len--;
// 		if(i >= len)
// 			return 0;
// 	}

// 	printf("\n");
// 	printf("mLen:%03d\n", len - _len);
// 	return (len - _len);
// }

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

/*接收包header高低字节转换*/
int msg_header_check(uint16_t header)
{
	int ret = 0;
	uint16_t TransHeader = 0;

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

#if 1
static char tcpRxBuf[1024*60] = {0};
#endif
void SRPC_RxCB(int clientFd)
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
		byteRead = read(clientFd, tcpRxBuf + len, 1024);
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
	M1_LOG_INFO("clientFd:%d,rx len:%05d, rx header:%x,%x,%x,%x, rx data:%s\n",clientFd, \
		len, tcpRxBuf[0],tcpRxBuf[1],tcpRxBuf[2],tcpRxBuf[3],tcpRxBuf+4);
	rc = client_write(&client_block->stack_block, tcpRxBuf, len);
	if(rc != TCP_SERVER_SUCCESS)
		M1_LOG_ERROR("client_write failed\n");

	Finish:
	len = 0;
	memset(tcpRxBuf, 0, 1024*60);

	M1_LOG_DEBUG("SRPC_RxCB--\n");

	return;
}

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

int client_block_init(void)
{
	M1_LOG_DEBUG( "client_block_init\n");
	int i;

	stack_block_init();

	for(i = 0; i < STACK_BLOCK_NUM; i++){
		client_block[i].clientFd = 0;
		// client_block[i].stack_block = NULL;
	}
}

client_block_t* client_stack_block_req(int clientFd)
{
	pthread_mutex_lock(&mutex_lock_sock);
	M1_LOG_DEBUG( "block_req\n");
	int i;
	int j = -1;

	for(i = 0; i <  STACK_BLOCK_NUM; i++){
		if(-1 == j)
			if(0 == client_block[i].clientFd)
				j = i;

		if(clientFd == client_block[i].clientFd){
			pthread_mutex_unlock(&mutex_lock_sock);
			return &client_block[i];
		}
	}

	client_block[j].clientFd = clientFd;
	if(TCP_SERVER_FAILED == stack_block_req(&client_block[j].stack_block)){
		M1_LOG_ERROR( "block_req failed\n");
		pthread_mutex_unlock(&mutex_lock_sock);
		return NULL;
	}

	pthread_mutex_unlock(&mutex_lock_sock);
	return &client_block[j];
} 

int client_block_destory(int clientFd)
{
	pthread_mutex_lock(&mutex_lock_sock);
	M1_LOG_DEBUG( "block_destory\n");
	int i;

	for(i = 0; i <  STACK_BLOCK_NUM; i++){

		if(clientFd == client_block[i].clientFd){
			client_block[i].clientFd = 0;
			if(TCP_SERVER_FAILED == stack_block_destroy(client_block[i].stack_block)){
				M1_LOG_ERROR("block_destroy failed\n");
				pthread_mutex_unlock(&mutex_lock_sock);
				return TCP_SERVER_FAILED;
			}
		}
	}
	pthread_mutex_unlock(&mutex_lock_sock);
	return TCP_SERVER_SUCCESS;
}

/*client write/read**************************************************************************************/
int client_write(stack_mem_t* d, char* data, int len)
{
	M1_LOG_DEBUG("write begin: num:%d\n, d->wPtr:%05d, d->rPtr:%05d,d->start:%05d,len:%05d, d->end:%05d\n",d->blockNum,d->wPtr, d->rPtr, d->start, len, d->end);
	//M1_LOG_DEBUG("header:%x,%x,%x,%x,str:%s\n",*(uint8_t*)&data[0],*(uint8_t*)&data[1],data[2],data[3],&data[4]);
	if(NULL == d){
		M1_LOG_ERROR( "NULL == d\n");
		return TCP_SERVER_FAILED;
	}

	int rc = 0;
	uint16_t header = 0;
	uint16_t distance = 0;

	header = *(uint16_t*)data;
	header = (uint16_t)(((header << 8) & 0xff00) | ((header >> 8) & 0xff)) & 0xffff;
	if(header == MSG_HEADER){
		distance = (*(uint16_t*)(data + BLOCK_LEN_OFFSET)) & 0xFFFF;
		distance = (((distance << 8) & 0xff00) | ((distance >> 8) & 0xff)) & 0xffff;
	}

	M1_LOG_INFO("len:%05d, distance:%05d\n",len, distance);
	pthread_mutex_lock(&mutex_lock_sock);
	rc = stack_push(d, data, len ,distance);
	pthread_mutex_unlock(&mutex_lock_sock);
	if(rc != TCP_SERVER_SUCCESS)
		M1_LOG_ERROR( "client write failed\n");
	
	M1_LOG_INFO("write end: num:%d\n, d->wPtr:%05d, d->rPtr:%05d,d->start:%05d,len:%05d, d->end:%05d\n",d->blockNum,d->wPtr, d->rPtr, d->start, len, d->end);
	return rc;
}

void client_read(void)
{
	M1_LOG_DEBUG( "client_read\n");
	int i = 0;
	int rc = TCP_SERVER_SUCCESS;
	int count = 0;
	uint16_t len = 0;
	uint16_t header = 0;
	char* headerP = NULL;
	char data[60*1024] = {0};
	volatile stack_mem_t* d = NULL;

	while(1){
		//M1_LOG_DEBUG("-------------------------%d read----------------------------\n",i);
		d = &client_block[i].stack_block;
		//M1_LOG_DEBUG( "read begin:d->rPtr:%05d, d->wPtr:%05d\n",d->rPtr, d->wPtr);
		if(client_block[i].clientFd == 0){
			goto Finish;
		}

		do{
			headerP = d->rPtr;
			pthread_mutex_lock(&mutex_lock_sock);
			rc = stack_pop(d, data, STACK_UNIT);
			pthread_mutex_unlock(&mutex_lock_sock);
			if(rc != TCP_SERVER_SUCCESS){
				goto Finish;
			}
			header = *(uint16_t*)data;
			header = (uint16_t)(((header << 8) & 0xff00) | ((header >> 8) & 0xff)) & 0xffff;
		}while(header != MSG_HEADER);

		len = *(uint16_t*)&data[2];
		len = (uint16_t)(((len << 8) & 0xff00) | ((len >> 8) & 0xff)) & 0xffff;
		#if M1_DBG
		M1_LOG_INFO( "clientFd: %d read,data[2]:%x,data[3]:%x,len:%05d,data:%s\n", client_block[i].clientFd, data[2], data[3],len,&data[4]);
		#endif		
		if((len+4) <= STACK_UNIT){
			client_read_to_data_handle(data + 4, len, client_block[i].clientFd);
			goto Finish;
		}

		rc = stack_pop(d, data + STACK_UNIT, len - STACK_UNIT + 4);
		if(rc != TCP_SERVER_SUCCESS){
			goto Finish;
		}

		client_read_to_data_handle(data + 4, len, client_block[i].clientFd);

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

