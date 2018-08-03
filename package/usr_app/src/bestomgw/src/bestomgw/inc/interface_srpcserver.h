/**************************************************************************************************
 * Filename:       interface_srpcserver.h
 * Description:    Socket Remote Procedure Call Interface - sample device application.
 *
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _INTERFACE_SRPCSERVER_H
#define _INTERFACE_SRPCSERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "cJSON.h"
#include "buf_manage.h"


#define SRPC_TCP_PORT 0x2be3

#define BLOCK_LEN_OFFSET     2
#define MSG_HEADER           0xFEFD

typedef struct {
	int clientFd;
	stack_mem_t stack_block;
}client_block_t;

enum client_block_status_t{
	TCP_SERVER_FAILED = 0,
	TCP_SERVER_SUCCESS
};



//RPSC ZLL Interface functions
void SRPC_Init(void);

/*client block*/
int client_block_init(void);
client_block_t* client_stack_block_req(int clientFd);
int client_block_destory(int clientFd);
void* client_read(void);
int client_write(stack_mem_t* d, char* data, int len);
/*msg*/
int msg_header_check(uint16_t header);
uint16_t msg_len_get(uint16_t len);

void client_write_test(void);
#ifdef __cplusplus
}
#endif

#endif //INTERFACE_SRPCSERVER_H
