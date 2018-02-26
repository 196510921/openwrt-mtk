#include "sql_table.h"
#include "m1_common_log.h"
/*prototype*****************************************************************************************************************/
static int tb_bind(sqlite3_stmt* stmt, void* d);
static int tb_select(sqlite3_stmt* stmt, void* d);
/*variable******************************************************************************************************************/
const table_methods tb_district_table_methods = {
	"district_table",
	tb_bind,
	tb_select
};

/*insert bind table "conn_info"*/
int tb_bind(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("district_table_bind\n");

	if(NULL == d){
		M1_LOG_ERROR("d NULL\n");
		return SQL_TABLE_FAILED;
	}

	tb_district_table* data = d;
	   
	if(SQLITE_OK != sqlite3_bind_int(stmt, 1, data->id)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 2, data->dis_name, strlen(data->dis_name), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 3, data->dis_pic, strlen(data->dis_pic), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 4, data->ap_id, strlen(data->ap_id), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 5, data->account, strlen(data->account), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
    if(SQLITE_OK != sqlite3_bind_text(stmt, 6, data->time, strlen(data->time), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}

	M1_LOG_DEBUG("insert TB_DISTRICT_TABLE:id:%d,dis_name:%s,dis_pic:%s,ap_id:%s,account:%s,time:%s\n", \
		data->id, data->dis_name, data->dis_pic,data->ap_id, data->account, data->time);

	return SQL_TABLE_SUCCESS;
}

/*select table "conn_info" one time*/
int tb_select(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("tb_istrict_table_select\n");

	return SQL_TABLE_SUCCESS;
}









