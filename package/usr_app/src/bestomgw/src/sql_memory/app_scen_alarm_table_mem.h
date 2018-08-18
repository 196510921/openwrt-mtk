#ifndef APP_SCEN_ALARM_TABLE_MEM_H
#define APP_SCEN_ALARM_TABLE_MEM_H

#include "cJSON.h"

typedef struct 
{
  cJSON *scen_name;
  cJSON *hour;
  cJSON *minutes;
  cJSON *week;
  cJSON *status;
}app_scen_alarm_table_tb;

typedef struct 
{
  cJSON *data_in_arry;
  app_scen_alarm_table_tb* member_data;  
}app_scen_alarm_table_mem_t;


#endif //APP_SCEN_ALARM_TABLE_MEM_H
