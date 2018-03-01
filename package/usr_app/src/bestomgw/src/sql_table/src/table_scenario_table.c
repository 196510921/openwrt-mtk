#include "sql_table.h"
#include "m1_common_log.h"
/*prototype*****************************************************************************************************************/
static int tb_bind(sqlite3_stmt* stmt, void* d);
static int tb_select(sqlite3_stmt* stmt, void* d);
/*variable******************************************************************************************************************/
const table_methods tb_scenario_table_methods = {
	"scenario_table",
	tb_bind,
	tb_select
};

/*insert bind table "scenario_table_table"*/
int tb_bind(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("scenario_table_bind\n");

	if(NULL == d){
		M1_LOG_ERROR("d NULL\n");
		return SQL_TABLE_FAILED;
	}

	tb_scenario_table* data = d;
   
	if(SQLITE_OK != sqlite3_bind_int(stmt, 1, data->id)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 2, data->scen_name, strlen(data->scen_name), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 3, data->scen_pic, strlen(data->scen_pic), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 4, data->district, strlen(data->district), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 5, data->ap_id, strlen(data->ap_id), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 6, data->dev_id, strlen(data->dev_id), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_int(stmt, 7, data->type)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_int(stmt, 8, data->value)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_int(stmt, 9, data->delay)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 10, data->account, strlen(data->account), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
    if(SQLITE_OK != sqlite3_bind_text(stmt, 11, data->time, strlen(data->time), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}

	M1_LOG_DEBUG("insert TB_SCENARIO_TABLE:id:%d,scen_name:%s,scen_pic:%s,district:%s,ap_id:%s,          \
	dev_id:%s,type:%d,value:%d,delay:%d,account:%s,time:%s\n",data->id,data->scen_name,data->scen_pic,   \
	data->district,data->ap_id,data->dev_id,data->type,data->value,data->delay,data->account,data->time);

	return SQL_TABLE_SUCCESS;
}

/*select table "conn_info" one time*/
static int tb_select(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("tb_scenario_table_select\n");

	return SQL_TABLE_SUCCESS;
}