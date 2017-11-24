#ifndef _ACCOUNT_CONFIG_H_
#define _ACCOUNT_CONFIG_H_

#include "m1_protocol.h"

/*用户请求用户账户信息*/
int app_req_account_info_handle(payload_t data);
/*APP请求用户配置*/
int app_req_account_config_handle(payload_t data);
/*APP用户配置处理*/
int app_account_config_handle(payload_t data);
/*用户登录处理*/
int user_login_handle(payload_t data);
/*APP请求区域下场景名称*/
int app_req_dis_scen_name(payload_t data);
/*APP请求区域名称*/
int app_req_dis_name(payload_t data);
/*APP请求区域下设备*/
int app_req_dis_dev(payload_t data);
/*用户更改登录密码*/
int app_change_user_key(payload_t data);

#endif //_ACCOUNT_CONFIG_H_
