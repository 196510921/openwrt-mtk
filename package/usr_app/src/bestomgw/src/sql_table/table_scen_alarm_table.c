#include "sql_table.h"
#include "m1_common_log.h"
/*prototype*****************************************************************************************************************/
static int tb_bind(sqlite3_stmt* stmt, void* d);
static int tb_select(sqlite3_stmt* stmt, void* d);
/*variable******************************************************************************************************************/
const table_methods tb_scen_alarm_table_methods = {
	"scen_alarm_table",
	tb_bind,
	tb_select
};

/*insert bind table "scen_alarm_table"*/
int tb_bind(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("scen_alarm_table_bind\n");

	if(NULL == d){
		M1_LOG_ERROR("d NULL\n");
		return SQL_TABLE_FAILED;
	}

	tb_scen_alarm_table* data = d;
    
	if(SQLITE_OK != sqlite3_bind_int(stmt, 1, data->id)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 2, data->scen_name, strlen(data->scen_name), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_int(stmt, 3, data->hour)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_int(stmt, 4, data->minutes)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 5, data->week, strlen(data->week), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 6, data->status, strlen(data->status), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
    if(SQLITE_OK != sqlite3_bind_text(stmt, 7, data->time, strlen(data->time), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}

	M1_LOG_DEBUG("insert TB_SCEN_ALARM_TABLE:id:%d, scen_name:%s, hour:%d, minutes:%d, week:%s,     \
	status:%s, time:%s\n",data->id,data->scen_name,data->hour,data->minutes,data->week,data->status, \
	data->time);

	return SQL_TABLE_SUCCESS;
}



/*select table "conn_info" one time*/
static int tb_select(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("tb_scen_alarm_table_select\n");

	return SQL_TABLE_SUCCESS;
}