#ifndef _DEV_ZH_H_
#define _DEV_ZH_H_
#include "utils.h"

#define ZH_DEV_HEADER_LEN      4
#define ZH_DEV_ADDR_LEN        2
#define ZH_CHECKSUM_LEN        1
#define ZH_DEV_INFO_LEN        10
#define ZH_PKG_ONLINE_LEN      6

/**********从弱电集成控制器到网关*********/
/*空调控制命令*/
#define DEV_ZH_ON_OFF      0x31   //向下控制开关
#define DEV_ZH_TEMP        0x32   //向下控制温度
#define DEV_ZH_MODE        0x33   //向下控制模式
#define DEV_ZH_SPEED       0x34   //向下空值风速
#define DEV_ZH_STATUS      0x50   //向下查询空调状态
/*空调控制值*/
#define ZH_ON              0x01   //开机
#define ZH_OFF             0x02   //关机
#define ZH_TEMP_TH_BOTTOM  0x10   //温度下限16℃
#define ZH_TEMP_TH_TOP     0x1E   //温度上限30℃
#define ZH_MODE_COLD       0x01   //制冷
#define ZH_MODE_WARM       0x08   //制热
#define ZH_MODE_WIND       0x04   //送风
#define ZH_MODE_AREFACTION 0x02   //除湿
#define ZH_SPEED_HIGH      0x01   //高速
#define ZH_SPEED_MID       0x02   //中速
#define ZH_SPEED_LOW       0x04   //低速
#define ZH_DEV_1_STATUS    0x01   //1台空调的状态值
#define ZH_ALL_ON_OFF      0x02   //查询多台设备在线状态

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
/*参数状态*/
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
}RxInfoStatus_t;
/*在线状态*/
typedef struct{
  UINT8 outAddr;          //空调外机地址
  UINT8 inAddr;           //空调内机地址
  UINT8 Online;           //空调在线状态
}RxInfoOnline_t;

/*app对设备操作*/
typedef union{
  RxInfoStatus_t infoStatus[1];
  RxInfoOnline_t infoOnline[1];
}RxInfo;

typedef struct{
	ZHHeader_t header;
	RxInfo     info;
}ZHuRxData_t;
/*帧侧数据*/
#pragma pack(1)
typedef struct{
	ZHHeader_t header;
	RxInfo    info;
}ZHfRxData_t;
#pragma pack()


#endif //_DEV_ZH_H_