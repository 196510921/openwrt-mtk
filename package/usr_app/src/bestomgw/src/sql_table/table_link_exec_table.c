#include "sql_table.h"
#include "m1_common_log.h"
/*prototype*****************************************************************************************************************/
static int tb_bind(sqlite3_stmt* stmt, void* d);
static int tb_select(sqlite3_stmt* stmt, void* d);
/*variable******************************************************************************************************************/
const table_methods tb_link_exec_table_methods = {
	"link_exec_table",
	tb_bind,
	tb_select
};

/*insert bind table "conn_info"*/
int tb_bind(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("link_exec_table_bind\n");

	if(NULL == d){
		M1_LOG_ERROR("d NULL\n");
		return SQL_TABLE_FAILED;
	}

	tb_link_exec_table* data = d;
    
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
	if(SQLITE_OK != sqlite3_bind_text(stmt, 4, data->ap_id, strlen(data->ap_id), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 5, data->dev_id, strlen(data->dev_id), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_int(stmt, 6, data->type)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_int(stmt, 7, data->value)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_int(stmt, 8, data->delay)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
    if(SQLITE_OK != sqlite3_bind_text(stmt, 9, data->time, strlen(data->time), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}

	M1_LOG_DEBUG("insert TB_LINK_EXEC_TABLE:id:%d,link_name:%s,district:%s,ap_id:%s,dev_id:%s,  \
		type:%d,value:%d,delay:%d,time:%s\n",data->id, data->link_name, data->district, data->ap_id,  \
		data->dev_id, data->type, data->value, data->delay, data->time);

	return SQL_TABLE_SUCCESS;
}

/*select table "conn_info" one time*/
static int tb_select(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("tb_link_exec_table_select\n");

	return SQL_TABLE_SUCCESS;
}





