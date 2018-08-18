#ifndef APP_DISTRICT_TABLE_MEM_H
#define APP_DISTRICT_TABLE_MEM_H

#include "cJSON.h"

typedef struct 
{
  cJSON *scen_name;
  cJSON *hour;
  cJSON *minutes;
  cJSON *week;
  cJSON *status;
}app_district_table_tb;

typedef struct 
{
  cJSON *data_in_arry;
  app_district_table_tb* member_data;  
}app_district_table_mem_t;


#endif //APP_DISTRICT_TABLE_MEM_H
