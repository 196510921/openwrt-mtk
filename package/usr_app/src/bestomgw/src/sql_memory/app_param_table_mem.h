#ifndef APP_PARAM_TABLE_MEM_H
#define APP_PARAM_TABLE_MEM_H

#include "cJSON.h"

typedef struct 
{
  cJSON *data_in_arry;
  char  *dev_id;
  int   type;
  int   value;
}app_param_table_mem_t;


#endif //APP_CONN_INFO_MEM_H
