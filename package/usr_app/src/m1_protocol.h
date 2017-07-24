#ifndef _M1_PROTOCOL_H_
#define _M1_PROTOCOL_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

/*Upload*********************************************************************/

/*AP report device data to M1*/
typedef struct _unit1_data{
 uint16_t param_id;                            //parameter id(reference to selfdefine device paramenter table)
 uint8_t  v_len;                               //parameter length
 uint8_t* val;							       //parameter value
 struct _unit1_data* next;
}unit1_data_t;

typedef struct _pdu1_data{
 uint16_t d_type;                               //device type
 uint8_t d_len;                                 //device data length
 unit1_data_t* u_data;                          //device data
 struct _pdu1_data* next; 
}pdu1_data_t;

typedef struct _pdu1{
 uint16_t port;                               //port number
 uint16_t p_type;                             //payload type
 uint8_t p_len;                               //payload length
 pdu1_data_t* p_data;                           //payload data
}pdu1_t;

/*APP request AP/device information */
typedef struct _pdu2{
 uint16_t p_type;                             //payload type
 uint8_t p_len;                               //payload length
 uint8_t p_data;                              //payload data
}pdu2_t;

/*APP enable/disable device access into net*/
typedef struct _pdu3{
 uint16_t p_type;                             //payload type
 uint8_t p_len;                               //payload length
 uint8_t p_data;                              //payload data
}pdu3_t;

#endif //_M1_PROTOCOL_H_
