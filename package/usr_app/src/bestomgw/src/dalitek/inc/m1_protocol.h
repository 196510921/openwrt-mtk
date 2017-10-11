#ifndef _M1_PROTOCOL_H_
#define _M1_PROTOCOL_H_

#include "cJSON.h"
#include "sqlite3.h"
/*api result*/
enum m1_protocol_result{
	M1_PROTOCOL_OK = 0,
	M1_PROTOCOL_FAILED,
	M1_PROTOCOL_NO_RSP,
};

/*response result*/
enum rsp_result{
	RSP_FAILED = 0,
	RSP_OK = 1,
};
/*socket package*/
typedef struct _socket_package{
	int len;
 	int clientFd;
 	char* data;
}m1_package_t;
/*pdu*/
typedef struct _payload{
  	int clientFd;
  	cJSON* pdu;
}payload_t;
/*common response data*/
typedef struct _rsp_data{
	int clientFd;
	int sn;
	int pduType;
	int result;
}rsp_data_t;

typedef struct _linkage_status{
	char* logical;
	char* status;
	int value;
	int threshold;
} linkage_status_t;



//void data_handle(m1_package_t* package);
void data_handle(void);
/*联动相关API*/
void trigger_cb(void* udp, int type, char const* db_name, char const* table_name, sqlite3_int64 rowid);
void data_update_cb(int id);
int trigger_cb_handle(void);
int linkage_task(void);
int linkage_msg_handle(payload_t data);
int app_req_linkage(int clientFd, int sn);
/*场景相关API*/
int scenario_exec(char* data, sqlite3* db);
int scenario_create_handle(payload_t data);
int scenario_alarm_create_handle(payload_t data);
int app_req_scenario(int clientFd, int sn);
int app_req_scenario_name(int clientFd, int sn);
/*区域相关API*/
int district_create_handle(payload_t data);
int app_req_district(int clientFd, int sn);
/*通用API*/
void m1_protocol_init(void);
void getNowTime(char* _time);
int sql_exec(sqlite3* db, char*sql);
int sql_id(sqlite3* db, char* sql);
int sql_row_number(sqlite3* db, char*sql);
void create_sql_trigger(void);
/*delay send*/
void delay_send_task(void);
void delay_send(cJSON* d, int delay, int clientFd);
/*数据库*/
int thread_sqlite3_step(sqlite3_stmt** stmt, sqlite3* db);
/*Download*********************************************************************/
/*APP request AP information*/
#define TYPE_REQ_AP_INFO                         0x0003
/*APP enable/disable device access into net*/
#define TYPE_DEV_NET_CONTROL                     0x0004
/*device write*/
#define TYPE_DEV_WRITE                           0x0005
/*device read*/
#define TYPE_DEV_READ                            0x0006
/*子设备/AP/联动/场景/区域-启动(停止/删除)*/
#define TYPE_COMMON_OPERATE                      0x0007
/*联动新建*/
#define TYPE_CREATE_LINKAGE                      0x000A
/*场景新建*/
#define TYPE_CREATE_SCENARIO                     0x000B
/*区域新建*/
#define TYPE_CREATE_DISTRICT                     0x000C
/*场景定时*/
#define TYPE_SCENARIO_ALARM                      0x000D
/*APP request device information */
#define TYPE_REQ_DEV_INFO                        0x000E
/*APP 请求场景信息 */
#define TYPE_REQ_SCEN_INFO                       0x0010
/*APP 请求联动信息 */
#define TYPE_REQ_LINK_INFO                       0x0011
/*APP 请求区域信息 */
#define TYPE_REQ_DISTRICT_INFO                   0x0012
/*APP 请求场景名称信息 */
#define TYPE_REQ_SCEN_NAME_INFO                  0x0013
/*Upload*********************************************************************/

/*AP report device data to M1*//*M1 report device data to APP*/
#define TYPE_REPORT_DATA                         0x1002
/*M1 report device information to APP*/
#define TYPE_M1_REPORT_AP_INFO					 0x1003
/*M1 report added AP & dev information to APP*/
#define TYPE_M1_REPORT_ADDED_INFO				 0x1004
/*M1 report AP information to APP*/
#define TYPE_M1_REPORT_DEV_INFO					 0x1005
/*M1 report AP information to APP*/
#define TYPE_AP_REPORT_AP_INFO                   0x1006
/*AP report device information to M1*/
#define TYPE_AP_REPORT_DEV_INFO                  0x1007
/*M1上报场景信息到APP*/
#define TYPE_M1_REPORT_SCEN_INFO                 0x1008
/*M1上报联动信息到APP*/
#define TYPE_M1_REPORT_LINK_INFO                 0x1009
/*M1上报区域信息到APP*/
#define TYPE_M1_REPORT_DISTRICT_INFO             0x100A
/*M1上报场景名称到APP*/
#define TYPE_M1_REPORT_SCEN_NAME_INFO            0x100B

/*write added device information */
#define TYPE_ECHO_DEV_INFO                       0x4005
/*request added device information*/
#define TYPE_REQ_ADDED_INFO                      0x4006


/*common response*/
#define TYPE_COMMON_RSP							 0x8004



/*self define*/
#define SCENARIO_DELAY_TOP    3600   //1个小时，单位秒

#endif //_M1_PROTOCOL_H_


