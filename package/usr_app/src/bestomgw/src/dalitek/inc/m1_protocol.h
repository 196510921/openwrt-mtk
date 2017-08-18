#ifndef _M1_PROTOCOL_H_
#define _M1_PROTOCOL_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>

#include "cJSON.h"
#include "sqlite3.h"

/*api result*/
enum m1_protocol_result{
	M1_PROTOCOL_OK = 0,
	M1_PROTOCOL_FAILED,
	M1_PROTOCOL_NO_RSP,
};

/*response result*/
enum rsp_result{
	RSP_FAILED = 0,
	RSP_OK = 1,
};
/*socket package*/
typedef struct _socket_package{
 	int fdClient;
 	char* data;
}m1_package_t;
/*pdu*/
typedef struct _payload{
  	int fdClient;
  	cJSON* pdu;
}payload_t;
/*common response data*/
typedef struct _rsp_data{
	int fdClient;
	int sn;
	int pduType;
	int result;
}rsp_data_t;

//void data_handle(char* data);
void data_handle(m1_package_t package);

/*Download*********************************************************************/
/*APP request AP information*/
#define TYPE_REQ_AP_INFO                         0x0003
/*APP enable/disable device access into net*/
#define TYPE_DEV_NET_CONTROL                     0x0004
/*device write*/
#define TYPE_DEV_WRITE                           0x0005
/*device read*/
#define TYPE_DEV_READ                            0x0006
/*APP request device information */
#define TYPE_REQ_DEV_INFO                        0x000E

/*Upload*********************************************************************/

/*AP report device data to M1*//*M1 report device data to APP*/
#define TYPE_REPORT_DATA                         0x1002
/*M1 report device information to APP*/
#define TYPE_M1_REPORT_AP_INFO					 0x1003
/*M1 report added AP & dev information to APP*/
#define TYPE_M1_REPORT_ADDED_INFO				 0x1004
/*M1 report AP information to APP*/
#define TYPE_M1_REPORT_DEV_INFO					 0x1005
/*M1 report AP information to APP*/
#define TYPE_AP_REPORT_AP_INFO                   0x1006
/*AP report device information to M1*/
#define TYPE_AP_REPORT_DEV_INFO                  0x1007

/*write added device information */
#define TYPE_ECHO_DEV_INFO                       0x4005
/*request added device information*/
#define TYPE_REQ_ADDED_INFO                      0x4006


/*common response*/
#define TYPE_COMMON_RSP							 0x8004

#endif //_M1_PROTOCOL_H_


