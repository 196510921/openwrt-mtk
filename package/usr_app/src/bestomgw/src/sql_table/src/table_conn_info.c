#include "sql_table.h"
#include "m1_common_log.h"
/*prototype*****************************************************************************************************************/
static int tb_bind(sqlite3_stmt* stmt, void* d);
static int tb_select(sqlite3_stmt* stmt, void* d);
/*variable******************************************************************************************************************/
const table_methods tb_conn_info_methods = {
	"conn_info",
	tb_bind,
	tb_select
};


/*insert bind table "conn_info"*/
static int tb_bind(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("conn_info_bind\n");

	if(NULL == d){
		M1_LOG_ERROR("d NULL\n");
		return SQL_TABLE_FAILED;
	}

	tb_conn_info* data = (tb_conn_info*)d;

	if(SQLITE_OK != sqlite3_bind_int(stmt, 1, data->id)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 2, data->ap_id, strlen(data->ap_id), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_int(stmt, 3, data->client_fd)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}

	M1_LOG_DEBUG("insert TB_CONN_INFO:id:%d,ap_id:%s,client_fd:%d\n",data->id, data->ap_id, data->client_fd);

	return SQL_TABLE_SUCCESS;
}

/*select table "conn_info" one time*/
int tb_select(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("tb_conn_info_select\n");

	return SQL_TABLE_SUCCESS;
}


