#include "sql_table.h"
#include "m1_common_log.h"
/*prototype*****************************************************************************************************************/
static int tb_bind(sqlite3_stmt* stmt, void* d);
static int tb_select(sqlite3_stmt* stmt, void* d);
/*variable******************************************************************************************************************/
const table_methods tb_project_table_methods = {
	"project_table",
	tb_bind,
	tb_select
};

/*insert bind table "project_table"*/
int tb_bind(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("project_table_bind\n");

	if(NULL == d){
		M1_LOG_ERROR("d NULL\n");
		return SQL_TABLE_FAILED;
	}

	tb_project_table* data = d;
      
	if(SQLITE_OK != sqlite3_bind_int(stmt, 1, data->id)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 2, data->p_name, strlen(data->p_name), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 3, data->p_number, strlen(data->p_number), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 4, data->p_creator, strlen(data->p_creator), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 5, data->p_manager, strlen(data->p_manager), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 6, data->p_editor, strlen(data->p_editor), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 7, data->p_tel, strlen(data->p_tel), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 8, data->p_add, strlen(data->p_add), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 9, data->p_brief, strlen(data->p_brief), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 10, data->p_key, strlen(data->p_key), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
	if(SQLITE_OK != sqlite3_bind_text(stmt, 11, data->account, strlen(data->account), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}
    if(SQLITE_OK != sqlite3_bind_text(stmt, 12, data->time, strlen(data->time), NULL)){
		M1_LOG_ERROR("bind error\n");
		return SQL_TABLE_FAILED;			
	}

	M1_LOG_DEBUG("insert TB_PROJECT_TABLE:id:%d,p_name:%s,p_number:%s,p_creator:%s,p_manager:%s,   \
	p_editor:%s,p_tel:%s,p_add:%s,p_brief:%s,p_key:%s,account:%s,time:%s\n",data->id, data->p_name,  \
	data->p_number, data->p_creator, data->p_manager, data->p_editor, data->p_tel,data->p_add, data->p_brief,  \
	data->p_key, data->account, data->time);

	return SQL_TABLE_SUCCESS;
}



/*select table "conn_info" one time*/
static int tb_select(sqlite3_stmt* stmt, void* d)
{
	M1_LOG_INFO("tb_project_table_select\n");

	return SQL_TABLE_SUCCESS;
}