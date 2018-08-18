#ifndef APP_SCENARIO_TABLE_MEM_H
#define APP_SCENARIO_TABLE_MEM_H

#include "cJSON.h"

typedef struct 
{
  cJSON *scen_name;
  cJSON *scen_pic;
  cJSON *district;
  cJSON *ap_id;
  cJSON *dev_id;
  cJSON *type;
  cJSON *value;
  cJSON *delay;	
}app_scenario_table_tb;

typedef struct 
{
  cJSON *data_in_arry;
  app_scenario_table_tb* member_data;  
}app_scenario_table_mem_t;


#endif //APP_SCENARIO_TABLE_MEM_H
