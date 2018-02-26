#include "sql_table.h"
#include "m1_common_log.h"
/*prototype*****************************************************************************************************************/
static int tb_bind(sqlite3_stmt* stmt, void* d);
static int tb_select(sqlite3_stmt* stmt, void* d);
/*variable******************************************************************************************************************/
const table_methods tb_all_dev_methods = {
	"all_dev",
	tb_bind,
	tb_select
};
/*insert bind table "all_dev"*/
int tb_bind(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("all_dev_bind\n");

	if(NULL == d){
		M1_LOG_ERROR("d NULL\n");
		return SQL_TABLE_FAILED;
	}

	tb_all_dev* data = (tb_all_dev*)d;
	
	if(SQLITE_OK != sqlite3_bind_int(stmt, 1, data->id)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 2, data->dev_id, strlen(data->dev_id), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 3, data->dev_name, strlen(data->dev_name), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 4, data->ap_id, strlen(data->ap_id), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_int(stmt, 5, data->pid)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_int(stmt, 6, data->added)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_int(stmt, 7, data->net)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 8, data->status, strlen(data->status), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 9, data->account, strlen(data->account), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 10, data->time, strlen(data->time), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}

	M1_LOG_DEBUG("insert TB_ALL_DEV:id:%d,dev_id:%s,dev_name:%s,ap_id:%s,pid:%d,added:%d,net:%d,status:%s, \
		account:%s,time:%s\n",data->id, data->dev_id, data->dev_name, data->ap_id, data->pid, data->added, \
		data->net, data->status, data->account, data->time);

	return SQL_TABLE_SUCCESS;
}

/*select table "all_dev" one time*/
int tb_select(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("tb_all_dev_select\n");

	int count = 0;
	int rc = 0;
	const char* name = NULL;

	if(NULL == d){
		M1_LOG_ERROR("d NULL\n");
		return SQL_TABLE_FAILED;
	}

	tb_all_dev* data = (tb_all_dev*)d;

	rc = sqlite3_step(stmt);
	if(SQLITE_ROW != rc)
	{
		M1_LOG_ERROR("step error: %d\n",rc);
		return SQL_TABLE_FAILED ;
	}

	count = sqlite3_column_count(stmt);
	for(int i = 0; i < count; i++){
		name = sqlite3_column_name(stmt, i);
		if(0 == strcmp("ID",name))
			data->id = sqlite3_column_int(stmt, i);
		else if(0 == strcmp("DEV_ID",name))
			data->dev_id = sqlite3_column_text(stmt, i);
		else if(0 == strcmp("DEV_NAME",name))
			data->dev_name = sqlite3_column_text(stmt, i);
		else if(0 == strcmp("AP_ID",name))
			data->ap_id = sqlite3_column_text(stmt, i);
		else if(0 == strcmp("PID",name))
			data->pid = sqlite3_column_int(stmt, i);
		else if(0 == strcmp("ADDED",name))
			data->added = sqlite3_column_int(stmt, i);
		else if(0 == strcmp("NET",name))
			data->net = sqlite3_column_int(stmt, i);
		else if(0 == strcmp("STATUS",name))
			data->status = sqlite3_column_text(stmt, i);
		else if(0 == strcmp("ACCOUNT",name))
			data->account = sqlite3_column_text(stmt, i);
		else if(0 == strcmp("TIME",name))
			data->time = sqlite3_column_text(stmt, i);
		else
			M1_LOG_WARN("column name:%s not match\n",name);		
	}

	return SQL_TABLE_SUCCESS;


}









