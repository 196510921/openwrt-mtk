#include "sql_table.h"
#include "m1_common_log.h"
/*prototype*****************************************************************************************************************/
static int tb_bind(sqlite3_stmt* stmt, void* d);
static int tb_select(sqlite3_stmt* stmt, void* d);
/*variable******************************************************************************************************************/
const table_methods tb_account_info_methods = {
	"account_info",
	tb_bind,
	tb_select
};

/*insert bind table "account_info"*/
static int tb_bind(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("conn_info_bind\n");

	if(NULL == d){
		M1_LOG_ERROR("d NULL\n");
		return SQL_TABLE_FAILED;
	}

	tb_account_info* data = (tb_account_info*)d;

	if(SQLITE_OK != sqlite3_bind_int(stmt, 1, data->id)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 2, data->account, strlen(data->account), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;	
	}
	if(SQLITE_OK != sqlite3_bind_int(stmt, 3, data->client_fd)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;	
	}

	M1_LOG_DEBUG("insert TB_ACCOUNT_INFO:id:%d,account:%s,client_fd:%d\n",data->id, data->account, data->client_fd);

	return SQL_TABLE_SUCCESS;
}

/*select table "account_info" one time*/
static int tb_select(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("tb_account_info_select\n");
	int count = 0;
	const char* name = NULL;

	if(NULL == d){
		M1_LOG_ERROR("d NULL\n");
		return SQL_TABLE_FAILED;
	}

	tb_account_info* data = (tb_account_info*)d;

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
		else if(0 == strcmp("CLIENT_FD",name))
			data->client_fd = sqlite3_column_int(stmt, i);
		else
			M1_LOG_WARN("column name:%s not match\n",name);		
	}

	return SQL_TABLE_SUCCESS;
}



