#include "sql_table.h"
#include "m1_common_log.h"
/*prototype*****************************************************************************************************************/
static int tb_bind(sqlite3_stmt* stmt, void* d);
static int tb_select(sqlite3_stmt* stmt, void* d);
/*variable******************************************************************************************************************/
const table_methods tb_linkage_table_methods = {
	"linkage_table",
	tb_bind,
	tb_select
};

/*insert bind table "linkage_table"*/
int tb_bind(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("linkage_table_bind\n");

	if(NULL == d){
		M1_LOG_ERROR("d NULL\n");
		return SQL_TABLE_FAILED;
	}

	tb_linkage_table* data = d;
	
	if(SQLITE_OK != sqlite3_bind_int(stmt, 1, data->id)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 2, data->link_name, strlen(data->link_name), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 3, data->district, strlen(data->district), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 4, data->exec_type, strlen(data->exec_type), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 5, data->exec_id, strlen(data->exec_id), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 6, data->status, strlen(data->status), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 7, data->enable, strlen(data->enable), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 8, data->time, strlen(data->time), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}

	M1_LOG_DEBUG("insert TB_LINKAGE_TABLE:id:%d,link_name:%s,district:%s,exec_type:%s,exec_id:%s,  \
		status:%s,enable:%s,time:%s\n",data->id,data->link_name, data->district, data->exec_type,  \
		data->exec_id, data->status, data->enable, data->time);

	return SQL_TABLE_SUCCESS;
}


/*select table "conn_info" one time*/
static int tb_select(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("tb_link_trigger_table_select\n");

	return SQL_TABLE_SUCCESS;
}