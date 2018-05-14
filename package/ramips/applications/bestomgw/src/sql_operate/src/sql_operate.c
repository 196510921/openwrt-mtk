#include "sql_operate.h"
#include "sql_table.h"
#include "m1_common_log.h"
/*prototypy*******************************************************************************************************/

/*variable*******************************************************************************************************/
void sql_rollback(sqlite3* db)
{
	int rc;
	char* errorMsg = NULL;

	if(sqlite3_exec(db, "ROLLBACK", NULL, NULL, &errorMsg) == SQLITE_OK){
	    M1_LOG_INFO("ROLLBACK ok！\n");
	}else{
	    M1_LOG_ERROR("ROLLBACK failed！\n");
	}

	if(errorMsg)
		sqlite3_free(errorMsg);
}

int sql_step(sqlite3* db, char* sql)
{
	int rc;
	sqlite3_stmt* stmt = NULL;

	if(SQLITE_OK != sqlite3_prepare_v2(db ,sql ,strlen(sql) ,&stmt ,NULL)){
		if(stmt)
			sqlite3_finalize(stmt);
		return SQL_OPERATE_FAILED;
	}
	
	if(rc = sqlite3_step(stmt) != SQLITE_DONE){
		M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
	    if(rc == SQLITE_BUSY || rc == SQLITE_MISUSE || rc == SQLITE_LOCKED){
	        rollback(db);
	    }else if(rc == SQLITE_CORRUPT){
        	M1_LOG_ERROR("SQLITE FATAL ERROR!\n");
        	system("/sql_restore.sh");
        	exit(0);
    	}

		sqlite3_finalize(stmt);
		return SQL_OPERATE_FAILED;
	}

	if(stmt)
		sqlite3_finalize(stmt);

	M1_LOG_DEBUG("SQLITE3: step ok！\n");
	return SQL_OPERATE_SUCCESS;
}

/*事物开始*/
int sql_begin_transaction(sqlite3* db)
{
	const char * sql = "BEGIN" ;

	if(sql_step(db, sql) != SQL_OPERATE_SUCCESS){
		M1_LOG_DEBUG("SQLITE3: BEGIN failed！\n");
		sql_rollback(db);
		return SQL_OPERATE_FAILED;
	}

	M1_LOG_DEBUG("SQLITE3: BEGIN ok！\n");
	return SQL_OPERATE_SUCCESS; 
}

/*提交事物*/
int sql_commit_transaction(sqlite3* db)
{
	const char * sql = "COMMIT" ;

	if(sql_step(db, sql) != SQL_OPERATE_SUCCESS){
		M1_LOG_DEBUG("SQLITE3: COMMIT failed！\n");
		return SQL_OPERATE_FAILED;
	}
	
	M1_LOG_DEBUG("SQLITE3: COMMIT ok！\n");
	return SQL_OPERATE_SUCCESS; 
}








