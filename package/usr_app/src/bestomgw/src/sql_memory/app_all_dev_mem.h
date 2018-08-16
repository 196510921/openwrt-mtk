#ifndef APP_ALL_DEV_MEM_H
#define APP_ALL_DEV_MEM_H

#include "cJSON.h"

typedef struct 
{
  cJSON *dev_id;
  cJSON *dev_name;
  cJSON *ap_id;
  cJSON *pid;
  cJSON *added;
  cJSON *net;
  cJSON *status;	
}app_all_dev_tb;

typedef struct 
{
  cJSON *data_in_arry;
  app_all_dev_tb* member_data;  
}app_all_dev_mem_t;


#endif //APP_ALL_DEV_MEM_H
