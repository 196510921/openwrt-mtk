#include "dev_zh.h"
#include "dev_common.h"
#include "m1_common_log.h"

static devErr ZHInit(void* pUser, void* pFrame, UINT16* pusLen);
static devErr ZHRead(void* pUser, void* pFrame, UINT16* pusLen);
static devErr ZHWrite(void* pUser, void* pFrame, UINT16* pusLen); 

devFunc func_dev_ZH = {
    ZHInit,
	ZHRead,
	ZHWrite  
};

static devErr ZHInit(void* pUser, void* pFrame, UINT16* pusLen)
{

}

static devErr ZHRead(void* pUser, void* pFrame, UINT16* pusLen)
{
	UINT8 checksum           = 0;
	int i                    = 0;
	int num                  = 0;
	ZHuRxData_t* ZHUserData  = (ZHuRxData_t*)pUser;
	ZHfRxData_t* ZHFrameData = (ZHfRxData_t*)pFrame;
	UINT8* checksumFrame     = (UINT8*)&pFrame[*pusLen - 1];

	{
		/*header部分*/
		ZHUserData->header.gwAddr = ZHFrameData->header.gwAddr;
		ZHUserData->header.Fn     = ZHFrameData->header.Fn;
		ZHUserData->header.value  = ZHFrameData->header.value;
		ZHUserData->header.num    = ZHFrameData->header.num;
		num                       = ZHFrameData->header.num;
		if(num == 0xFF)
		{
			num = 0x01;
		}
		/*info 部分*/
		if(ZHUserData->header.value == 0x02)//多台设备的在线状态
		{
			for(i = 0; i < num; i++)
			{
				ZHUserData->info.infoOnline[i].outAddr     = ZHFrameData->info.infoOnline[i].outAddr;
				ZHUserData->info.infoOnline[i].inAddr      = ZHFrameData->info.infoOnline[i].inAddr;
				ZHUserData->info.infoOnline[i].Online      = ZHFrameData->info.infoOnline[i].Online;	
			}
		}
		else//设备的参数状态
		{
			for(i = 0; i < num; i++)
			{
				ZHUserData->info.infoStatus[i].outAddr     = ZHFrameData->info.infoStatus[i].outAddr;
				ZHUserData->info.infoStatus[i].inAddr      = ZHFrameData->info.infoStatus[i].inAddr;
				ZHUserData->info.infoStatus[i].status      = ZHFrameData->info.infoStatus[i].status;
				ZHUserData->info.infoStatus[i].temperature = ZHFrameData->info.infoStatus[i].temperature;	
				ZHUserData->info.infoStatus[i].mode        = ZHFrameData->info.infoStatus[i].mode;
				ZHUserData->info.infoStatus[i].speed       = ZHFrameData->info.infoStatus[i].speed;
				ZHUserData->info.infoStatus[i].roomTemp    = ZHFrameData->info.infoStatus[i].roomTemp;
				ZHUserData->info.infoStatus[i].errNum      = ZHFrameData->info.infoStatus[i].errNum;
				ZHUserData->info.infoStatus[i].remain1     = ZHFrameData->info.infoStatus[i].remain1;
				ZHUserData->info.infoStatus[i].remain2     = ZHFrameData->info.infoStatus[i].remain2;
			}
		} 
		
	}

	/*检查checksum*/
    checksum = app_checksum((UINT8*)&pFrame[0], *pusLen - 1);
	*pusLen -= ZH_CHECKSUM_LEN;
    printf("checksum:%x checksumFrame:%x\n",checksum, *checksumFrame);
    if(checksum != *checksumFrame)
    {
    	printf("checksum not match!");
    	return DEV_ERROR;
    }

	/*debug info*/
	{	
		printf("Frame Data:");
		for(i = 0; i < *pusLen; i++)
			printf("%x ",*(UINT8*)&pFrame[i]);
		printf("\n ");

		printf("gwAddr:%d Fn:%d value:%d num:%d \n",\
					  ZHUserData->header.gwAddr, ZHUserData->header.Fn,\
					  ZHUserData->header.value, ZHUserData->header.num);
		if(ZHUserData->header.value == 0x02)//多台设备的在线状态
		{
			for(i = 0; i < num; i++)
				printf("dAddr[%d]外机:%x 内机:%x 在线状态:%x \n",\
				 i,ZHUserData->info.infoOnline[i].outAddr, \
				 ZHUserData->info.infoOnline[i].inAddr,\
				 ZHUserData->info.infoOnline[i].Online);
		}
		else
		{
			for(i = 0; i < num; i++)
				printf("dAddr[%d]外机:%x 内机:%x 开关:%x 温度:%x 模式:%x 风速:%x 房间温度:%x 故障代码:%x\n",\
				 i,ZHUserData->info.infoStatus[i].outAddr, \
				 ZHUserData->info.infoStatus[i].inAddr,\
				 ZHUserData->info.infoStatus[i].status,\
				 ZHUserData->info.infoStatus[i].temperature,\
				 ZHUserData->info.infoStatus[i].mode,\
				 ZHUserData->info.infoStatus[i].speed,\
				 ZHUserData->info.infoStatus[i].roomTemp,\
				 ZHUserData->info.infoStatus[i].errNum);
		}
		printf("pusLen:%d\n",*pusLen);		
	}

	return DEV_OK;
}

static devErr ZHWrite(void* pUser, void* pFrame, UINT16* pusLen)
{
	int i                    = 0;
	int num                  = 0;
	ZHuTxData_t* ZHUserData  = (ZHuTxData_t*)pUser;
	ZHfTxData_t* ZHFrameData = (ZHfTxData_t*)pFrame;
	UINT8* checksum          = (UINT8*)&pFrame[*pusLen];

	{
		/*header部分*/
		ZHFrameData->header.gwAddr = ZHUserData->header.gwAddr;
		ZHFrameData->header.Fn     = ZHUserData->header.Fn;
		ZHFrameData->header.value  = ZHUserData->header.value;
		ZHFrameData->header.num    = ZHUserData->header.num;
		num                        = ZHUserData->header.num;
		if(num == 0xFF)
		{
			num = 0x01;
		}
		/*addr 部分*/
		for(i = 0; i < num; i++)
		{
			ZHFrameData->dAddr[i].outAddr = ZHUserData->dAddr[i].outAddr;
			ZHFrameData->dAddr[i].inAddr  = ZHUserData->dAddr[i].inAddr;
		}
	}

	*checksum = app_checksum((UINT8*)&pFrame[0], *pusLen);
	*pusLen += ZH_CHECKSUM_LEN; //帧结构数据长度
	/*debug info*/
	{
		printf("gwAddr:%d Fn:%d value:%d num:%d \n",\
					  ZHFrameData->header.gwAddr, ZHFrameData->header.Fn, \
					  ZHFrameData->header.value, ZHFrameData->header.num);
		for(i = 0; i < num; i++)
			printf("dAddr[%x]外机:%x 内机:%x \n", \
				          i, ZHFrameData->dAddr[i].outAddr, ZHFrameData->dAddr[i].inAddr);

		printf("checksum:%x\n",*checksum);
		printf("pusLen:%x\n",*pusLen);
		printf("Frame Data:");
		for(i = 0; i < *pusLen; i++)
			printf("%x ",*(UINT8*)&pFrame[i]);
		printf("\n ");
	}

	return DEV_OK;
}



