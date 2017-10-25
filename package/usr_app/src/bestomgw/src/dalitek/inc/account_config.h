#ifndef _ACCOUNT_CONFIG_H_
#define _ACCOUNT_CONFIG_H_

#include "m1_protocol.h"

/*用户请求用户账户信息*/
int app_req_account_info_handle(payload_t data, int sn);
/*APP请求用户配置*/
int app_req_account_config_handle(payload_t data, int sn);
/*APP用户配置处理*/
int app_account_config_handle(payload_t data);
/*用户登录处理*/
int user_login_handle(payload_t data);
/*APP请求区域下场景名称*/
int app_req_dis_scen_name(payload_t data, int sn);
/*APP请求区域名称*/
int app_req_dis_name(int clientFd, int sn);
/*APP请求区域下设备*/
int app_req_dis_dev(payload_t data, int sn);

#endif //_ACCOUNT_CONFIG_H_
