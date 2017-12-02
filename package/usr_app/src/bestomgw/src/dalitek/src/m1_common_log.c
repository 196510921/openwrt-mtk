
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m1_common_log.h"

m1_log_level_t m1LogLevel = M1_LOG_LEVEL_ERROR;


void m1_common_log_set_level(m1_log_level_t m1LogLevel)
{
    m1LogLevel = m1LogLevel;
}

m1_log_level_t m1_common_log_get_level()
{
    return m1LogLevel;
}
