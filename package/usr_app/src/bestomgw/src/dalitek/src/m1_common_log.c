
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m1_common_log.h"

m1_log_level_t m1LogLevel = M1_LOG_LEVEL_INFO;


void m1_common_log_set_level(m1_log_level_t m1LogLevel)
{
    m1LogLevel = m1LogLevel;
}

m1_log_level_t m1_common_log_get_level()
{
    return m1LogLevel;
}

void debug_switch(char* input_info)
{

	printf("User input:%s\n",input_info);

	if(strcmp(input_info,"D") == 0){
		m1LogLevel = M1_LOG_LEVEL_DEBUG;
		M1_LOG_DEBUG( " ---------OUTPUT  SWITCH TO DEBUG MODE-------------\n");
	}else if(strcmp(input_info,"I") == 0){
		m1LogLevel = M1_LOG_LEVEL_INFO;
		M1_LOG_DEBUG( " ---------OUTPUT  SWITCH TO DEBUG MODE-------------\n");
		M1_LOG_INFO( " ---------OUTPUT  SWITCH TO INFO MODE-------------\n");
	}else if(strcmp(input_info,"W") == 0){
		m1LogLevel = M1_LOG_LEVEL_WARN;
		M1_LOG_DEBUG( " ---------OUTPUT  SWITCH TO DEBUG MODE-------------\n");
		M1_LOG_INFO( " ---------OUTPUT  SWITCH TO INFO MODE-------------\n");
		M1_LOG_WARN( " ---------OUTPUT  SWITCH TO WARN MODE-------------\n");
	}else if(strcmp(input_info,"E") == 0){
		m1LogLevel = M1_LOG_LEVEL_ERROR;
		M1_LOG_DEBUG( " ---------OUTPUT  SWITCH TO DEBUG MODE-------------\n");
		M1_LOG_INFO( " ---------OUTPUT  SWITCH TO INFO MODE-------------\n");
		M1_LOG_WARN( " ---------OUTPUT  SWITCH TO WARN MODE-------------\n");
		M1_LOG_ERROR( " ---------OUTPUT  SWITCH TO ERROR MODE-------------\n");
	}

	
}