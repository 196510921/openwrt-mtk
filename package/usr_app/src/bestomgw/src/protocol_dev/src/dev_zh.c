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
		for(i = 0; i < num; i++)
		{
			ZHUserData->info[i].outAddr     = ZHFrameData->info[i].outAddr;
			ZHUserData->info[i].inAddr      = ZHFrameData->info[i].inAddr;
			ZHUserData->info[i].status      = ZHFrameData->info[i].status;
			ZHUserData->info[i].temperature = ZHFrameData->info[i].temperature;	
			ZHUserData->info[i].mode        = ZHFrameData->info[i].mode;
			ZHUserData->info[i].speed       = ZHFrameData->info[i].speed;
			ZHUserData->info[i].roomTemp    = ZHFrameData->info[i].roomTemp;
			ZHUserData->info[i].errNum      = ZHFrameData->info[i].errNum;
			ZHUserData->info[i].remain1     = ZHFrameData->info[i].remain1;
			ZHUserData->info[i].remain2     = ZHFrameData->info[i].remain2;
		}
	}

	printf("pusLen:%x\n",*pusLen);
	printf("gwAddr:%d Fn:%d value:%d num:%d \n",\
					  ZHUserData->header.gwAddr, ZHUserData->header.Fn, \
					  ZHUserData->header.value, ZHUserData->header.num);
	/*检查checksum*/
    checksum = app_checksum((UINT8*)&pFrame[0], *pusLen - 1);
	*pusLen -= ZH_CHECKSUM_LEN;
    printf("checksum:%d checksumFrame:%d\n",checksum, *checksumFrame);
    if(checksum != *checksumFrame)
    {
    	M1_LOG_WARN("checksum not match!");
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
		for(i = 0; i < num; i++)
			printf("dAddr[%d]外机:%x 内机:%x 开关:%x 温度:%x 模式:%x 风速:%x 房间温度:%x 故障代码:%x\n",\
			 i,ZHUserData->info[i].outAddr, ZHUserData->info[i].inAddr,ZHUserData->info[i].status,\
			 ZHUserData->info[i].temperature,ZHUserData->info[i].mode,ZHUserData->info[i].speed,\
			 ZHUserData->info[i].roomTemp,ZHUserData->info[i].errNum);

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
	printf("pusLen:%x\n",*pusLen);
	printf("gwAddr:%d Fn:%d value:%d num:%d \n",\
					  ZHUserData->header.gwAddr, ZHUserData->header.Fn, \
					  ZHUserData->header.value, ZHUserData->header.num);
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



