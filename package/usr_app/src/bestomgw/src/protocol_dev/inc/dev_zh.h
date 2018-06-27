#ifndef _DEV_ZH_H_
#define _DEV_ZH_H_
#include "utils.h"

#define ZH_DEV_HEADER_LEN      4
#define ZH_DEV_ADDR_LEN        2
#define ZH_CHECKSUM_LEN        1
#define ZH_DEV_INFO_LEN        10

/**********从弱电集成控制器到网关*********/
#define DEV_PARAM_TYPE_SWITCH   0x0001
#define DEV_PARAM_TYPE_TEMP     0x0002
#define DEV_PARAM_TYPE_MODE     0x0003
#define DEV_PARAM_TYPE_SPEED    0x0004

/*用户侧数据*/
typedef struct{
  UINT8 gwAddr;           //本网关地址
  UINT8 Fn;               //功能码
  UINT8 value;            //控制值
  UINT8 num;              //空调数量
}ZHHeader_t;

typedef struct{
  UINT8 outAddr;          //空调外机地址
  UINT8 inAddr;           //空调内机地址
}ZHTxInfo_t;

typedef struct{
  ZHHeader_t header;	
  ZHTxInfo_t dAddr[1];
}ZHuTxData_t;

/*帧侧数据*/
#pragma pack(1)
typedef struct{
  ZHHeader_t header;
  ZHTxInfo_t dAddr[1];
}ZHfTxData_t;
#pragma pack()

/************从网关到弱电集成控制器*********/
/*用户侧数据*/
typedef struct{
  UINT8 outAddr;          //空调外机地址
  UINT8 inAddr;           //空调内机地址
  UINT8 status;           //空调开关状态
  UINT8	temperature;      //温度设定
  UINT8 mode;             //模式设定
  UINT8 speed;            //风速设定
  UINT8 roomTemp;         //房间温度
  UINT8 errNum;           //故障代码
  UINT8 remain1;          //备用1
  UINT8 remain2;          //备用2
}RxInfo;

typedef struct{
	ZHHeader_t header;
	RxInfo     info[1];
}ZHuRxData_t;
/*帧侧数据*/
#pragma pack(1)
typedef struct{
	ZHHeader_t header;
	RxInfo    info[1];
}ZHfRxData_t;
#pragma pack()


#endif //_DEV_ZH_H_