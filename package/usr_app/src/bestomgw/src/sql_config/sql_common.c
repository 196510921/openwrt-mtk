#include "sql_common.h"
#include "sqlite3.h"
#include "m1_common_log.h"

sqlComT sql_begin_immediate(sqlite3* db)
{
	sqlComT ret    = SQL_OK;
	char* errorMsg = NULL;

	if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)=!SQLITE_OK)
	{
		M1_LOG_WARN("BEGIN IMMEDIATE error:%s",errorMsg);
		ret = SQL_ERROR;
	}
    
	sqlite3_free(errorMsg);
	return ret;
}

sqlComT sql_begin(sqlite3* db)
{
	sqlComT ret    = SQL_OK;
	char* errorMsg = NULL;

	if(sqlite3_exec(db, "BEGIN", NULL, NULL, &errorMsg)!=SQLITE_OK)
	{
		M1_LOG_WARN("BEGIN IMMEDIATE error:%s",errorMsg);
		ret = SQL_ERROR;
	}
    
	sqlite3_free(errorMsg);
	return ret;
}


