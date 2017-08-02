#ifndef _M1_PROTOCOL_H_
#define _M1_PROTOCOL_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <stdbool.h>
#include <errno.h>

void data_handle(uint8_t* _data, uint16_t len);
int _test_data(void);


typedef struct _pdu1{
 uint16_t p_type;                              //payload type
 uint16_t p_len;                               //payload length
 uint8_t p_data[];                             //payload data
}pdu_t;

/*Upload*********************************************************************/

/*AP report device data to M1*/
#define TYPE_PDU1                              0x1001

typedef struct _unit1_data{
 uint16_t param_id;                            //parameter id(reference to selfdefine device paramenter table)
 uint8_t  v_len;                               //parameter length
 uint8_t val[];							       //parameter value
}unit1_data_t;

typedef struct _pdu1_data{
 uint16_t d_type;                              //device type
 uint8_t d_id[8];
 uint8_t d_len;
 uint8_t u_data[];
}pdu1_data_t;

/*AP report device information to M1*/
#define TYPE_PDU6                              0x1003

typedef struct _pdu6_data{
 uint16_t port;
 uint16_t d_type;                              //device type
 uint8_t ap_id[8];
 uint8_t d_id[8];
 uint8_t n_len;
 char name[];
}pdu6_data_t;
/*Download*********************************************************************/
/*APP request AP/device information */
#define TYPE_PDU2                              0x0003
/*APP enable/disable device access into net*/
#define TYPE_PDU3                             0x0004
/*device write*/
#define TYPE_PDU4                             0x0005
typedef struct _unit4_data{
 uint16_t param_id;                            //parameter id(reference to selfdefine device paramenter table)
 uint8_t  v_len;                               //parameter length
 uint8_t val[];							       //parameter value
}unit4_data_t;

typedef struct _pdu4_data{
 uint8_t d_id[8];                               //device type
 uint8_t d_len;                                 //device data length
 uint8_t u_data[];                              //device data 
}pdu4_data_t;

/*device read*/
#define TYPE_PDU5                               0x0006
typedef struct _pdu5_data{
 uint8_t d_id[8];                               //device type
 uint8_t d_len;                                 //device data length
 uint8_t u_data[];                            //device data 
}pdu5_data_t;


#endif //_M1_PROTOCOL_H_


