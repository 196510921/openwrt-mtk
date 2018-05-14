#ifndef _M1_PROJECT_H_
#define _M1_PROJECT_H_

#include "m1_protocol.h"

int app_get_project_info(payload_t data);
int app_get_project_config(payload_t data);
int app_change_project_config(payload_t data);
int app_create_project(payload_t data);
int app_change_project_key(payload_t data);
#endif //_M1_PROJECT_H_
