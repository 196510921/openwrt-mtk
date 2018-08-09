#ifndef AP_CONFIG_H
#define AP_CONFIG_H
#include "m1_protocol.h"

/*app config ap router parameter*/
#define TYPE_APP_CFG_AP_ROUTER                    0x0032
/*app config ap zigbee channel*/
#define TYPE_APP_CFG_AP_ZIGBEE                    0x0033
/*app read ap router parameter*/
#define TYPE_APP_READ_AP_ROUTER                   0x0034
/*app read ap zigbee parameter*/
#define TYPE_APP_READ_AP_ZIGBEE                   0x0035
/*ap report ap router parameter*/
#define TYPE_AP_REPORT_AP_ROUTER                  0x1016
/*ap report zigbee param*/
#define TYPE_AP_REPORT_ZIGBEE                     0x1017

int ap_cfg_router(cJSON* data, sqlite3* db);
int app_read_ap_cfg(cJSON* data, sqlite3* db);
#endif //AP_CONFIG_H
