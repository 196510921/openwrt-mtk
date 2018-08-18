#ifndef APP_LINKAGE_TABLE_MEM_H
#define APP_LINKAGE_TABLE_MEM_H

#include "cJSON.h"

typedef struct 
{
  cJSON *link_name;
  cJSON *district;
  cJSON *exec_type;
  cJSON *exec_id;
  cJSON *status;
  cJSON *enable;	
}app_linkage_table_tb;

typedef struct 
{
  cJSON *data_in_arry;
  app_linkage_table_tb* member_data;  
}app_linkage_table_mem_t;


#endif //APP_ALL_DEV_MEM_H
