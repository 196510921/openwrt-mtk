#ifndef _ACCOUNT_CONFIG_H_
#define _ACCOUNT_CONFIG_H_

#include "m1_protocol.h"

int app_req_account_info_handle(payload_t data, int sn);
int app_req_account_config_handle(payload_t data, int sn);
int app_account_config_handle(payload_t data);
int user_login_handle(payload_t data);


#endif //_ACCOUNT_CONFIG_H_
