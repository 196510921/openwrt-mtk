#ifndef APP_PARSE_H
#define APP_PARSE_H

#pragma pack(1) 
typedef struct 
{
	unsigned short fixed;
	unsigned short len;
	char*          buf;
}app_parse_t;
#pragma pack() 

#endif //APP_PARSE_H


