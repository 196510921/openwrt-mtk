#ifndef _M1_PROTOCOL_H_
#define _M1_PROTOCOL_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <stdbool.h>
#include <errno.h>

void data_handle(char* data);
int _test_data(void);


/*Upload*********************************************************************/

/*AP report device data to M1*/
#define TYPE_REPORT_DATA                         0x1002
/*AP report AP information to M1*/
#define TYPE_REPORT_AP_INFO                      0x1003
/*AP report device information to M1*/
#define TYPE_REPORT_DEV_INFO                     0x1005
/*Download*********************************************************************/
/*APP request AP information*/
#define TYPE_REQ_AP_INFO                         0x0003
/*APP request device information */
#define TYPE_REQ_DEV_INFO                        0x000E

/*APP enable/disable device access into net*/
#define TYPE_DEV_NET_CONTROL                     0x0004
/*device write*/
#define TYPE_DEV_WRITE                           0x0005
/*device read*/
#define TYPE_DEV_READ                            0x0006
/*write added device information */
#define TYPE_ECHO_DEV_INFO                       0x4005
/*request added device information*/
#define TYPE_REQ_ADDED_INFO                      0x4006

#endif //_M1_PROTOCOL_H_


