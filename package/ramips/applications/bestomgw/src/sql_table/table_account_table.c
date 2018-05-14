#include "sql_table.h"
#include "m1_common_log.h"
/*prototype*****************************************************************************************************************/
static int tb_bind(sqlite3_stmt* stmt, void* d);
static int tb_select(sqlite3_stmt* stmt, void* d);
/*variable******************************************************************************************************************/
const table_methods tb_account_table_methods = {
	"account_table",
	tb_bind,
	tb_select
};

/*insert bind table "account_table"*/
int tb_bind(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("account_table_bind\n");

	if(NULL == d){
		M1_LOG_ERROR("d NULL\n");
		return SQL_TABLE_FAILED;
	}

	tb_account_table* data = (tb_account_table*)d;

	if(SQLITE_OK != sqlite3_bind_int(stmt, 1, data->id)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 2, data->account, strlen(data->account), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 3, data->key, strlen(data->key), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 4, data->key_auth, strlen(data->key_auth), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 5, data->remote_auth, strlen(data->remote_auth), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 6, data->time, strlen(data->time), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}	

	M1_LOG_DEBUG("insert TB_ACCOUNT_TABLE:id:%d,account:%s,key:%s,key_auth:%s,remote_auth:%s,time:%s\n", \
		data->id, data->account, data->key,data->key_auth, data->remote_auth, data->time);

	return SQL_TABLE_SUCCESS;
} 

/*select table "account_table" one time*/
static int tb_select(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("tb_account_table_select\n");
	int count = 0;
	const char* name = NULL;

	if(NULL == d){
		M1_LOG_ERROR("d NULL\n");
		return SQL_TABLE_FAILED;
	}

	tb_account_table* data = (tb_account_table*)d;

	if(SQLITE_DONE != sqlite3_step(stmt))
	{
		return SQL_TABLE_FAILED ;
		M1_LOG_ERROR("sqlite3_step failed\n");
	}

	count = sqlite3_column_count(stmt);
	for(int i = 0; i < count; i++){
		name = sqlite3_column_name(stmt, i);
		if(0 == strcmp("ID",name))
			data->id = sqlite3_column_int(stmt, i);
		else if(0 == strcmp("ACCOUNT",name))
			data->account = sqlite3_column_text(stmt, i);
		else if(0 == strcmp("KEY",name))
			data->key = sqlite3_column_text(stmt, i);
		else if(0 == strcmp("KEY_AUTH",name))
			data->key_auth = sqlite3_column_text(stmt, i);
		else if(0 == strcmp("REMOTE_AUTH",name))
			data->remote_auth = sqlite3_column_text(stmt, i);
		else if(0 == strcmp("TIME",name))
			data->time = sqlite3_column_text(stmt, i);
		else
			M1_LOG_WARN("column name:%s not match\n",name);		
	}

	return SQL_TABLE_SUCCESS;
}


