#ifndef _DEV_COMMON_H_
#define _DEV_COMMON_H_
#include "utils.h"
#include "dev_zh.h"
#include "cJSON.h"

/*返回状态*/
typedef enum{
	DEV_OK         = 0,
	DEV_ERROR      = 1
}devErr;

/*485通信接口号*/
typedef enum{
    U_ttyS2        = 0x0001,
    U_ttyUSB0      = 0x0002,
    U_ttyUSB1      = 0x0003,
    U_ttyUSB2      = 0x0004,
    U_MAX
}uartNum;

/********************************设备注册表*******************************/
/*协议类型*/
typedef enum{
 P_485_ZH        =0x0001,
 P_MAX
}pType;
/*设备ID*/
typedef enum{
 DEV_ID_ZH        =0x0001,
 DEV_ID_MAX
}devId;
/********************************设备控制接口********************************/
typedef struct {
	devErr (*devInit)(void* pUser, void* pFrame, UINT16* pusLen);        //设备初始化
	devErr (*devRead)(void* pUser, void* pFrame, UINT16* pusLen);        //设备读取
	devErr (*devWrite)(void* pUser, void* pFrame, UINT16* pusLen);       //设备写入
}devFunc;
/********************************设备信息***********************************/
typedef struct
{    
    uartNum        uart;
    pType          protocol;
    devId          Id;
    const devFunc* dFunc;
    const char*    devName;    
}devInfo; 

/*app对设备操作*/
typedef union{
	ZHuTxData_t    ZHuTxData;
	ZHfRxData_t    ZHfRxData;
}uDataApp;

/*write*/
typedef struct{
	UINT16    devId;
	UINT16    len;
	uDataApp  d;
}devWrite_t;

/*read*/
typedef struct{
	devWrite_t cmd;
	UINT8*     userData;
}devRead_t;

/*app cmd*/
typedef struct{
	UINT8  param;
	UINT8  value;
	UINT8  dNum;
	UINT8  gwAddr;
	UINT16 devAddr[1];
}appCmd_t;


/*checksum*/
UINT8 app_checksum(UINT8* d, UINT16 len);

/*通用链表*/
typedef struct _comPNode
{
 UINT8* item;
 struct _comPNode *next;
}comPNode, *comPQueue;


/*对上层数据类型与接口*/
/*设备命令类型*/
typedef enum{
 DEV_READ_ONLINE        = 0x01,
 DEV_READ_STATUS        = 0x02,
 DEV_WRITE              = 0x03,
 DEV_TYPE_MAX
}devCmdType_t;

typedef struct{
	char*        devId;
	cJSON*       paramJson;
	devCmdType_t cmdType;
}dev485Opt_t;

typedef struct{
	cJSON*  paramJson;
	UINT8   Fn;
	UINT8   value; 
}dev485WriteParam_t;

/*485线程*/
void dev_485_thread(void);
devErr dev485Init(void);
/*读取485空调设备参数状态*/
int dev_485_operate(dev485Opt_t cmd);


void Init_comPQueue(comPQueue pQueue);
void comPush(comPQueue pQueue, uint8* item);
//bool comPop(comPQueue pQueue, uint8 *pItem);
bool comPop(comPQueue pQueue, uint8 **pItem);
bool comIsEmpty(comPQueue pQueue);

devErr dev_conditioner_on(void);
#endif //_DEV_COMMON_H_








