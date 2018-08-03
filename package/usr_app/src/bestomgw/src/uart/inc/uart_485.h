#ifndef _UART_485_H_
#define _UART_485_H_
#include "utils.h"


typedef struct
{
	int uartFd;
	UINT8* d;
	int len;
}uart_data_t;

int uart_485_init(char* uart);
int uart_write(uart_data_t data);
int uart_read(uart_data_t* data);
#endif //_UART_485_H_
