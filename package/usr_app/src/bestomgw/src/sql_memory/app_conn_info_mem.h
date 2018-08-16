#ifndef APP_CONN_INFO_MEM_H
#define APP_CONN_INFO_MEM_H

#include "cJSON.h"

typedef struct 
{
  char*  ap_id;
  int    client_fd;
}app_conn_info_mem_t;


#endif //APP_CONN_INFO_MEM_H
