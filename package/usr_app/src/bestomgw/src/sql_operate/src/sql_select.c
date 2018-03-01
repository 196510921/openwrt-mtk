#include "sql_operate.h"
#include "sql_table.h"
#include "m1_common_log.h"

/*prtotype*/
static int sqlite3_select(sql_operate* d);
static int sqlite3_select_next(sql_operate* d);
static void sqlite3_select_end(sql_operate* d);

/*variable*************************************************************************************************************/
// extern const table_methods tb_account_info_methods;
// extern const table_methods tb_account_table_methods;
// extern const table_methods tb_all_dev_methods;
// extern const table_methods tb_conn_info_methods;
// extern const table_methods tb_district_table_methods;
// extern const table_methods tb_link_exec_table_methods;
// extern const table_methods tb_link_trigger_table_methods;
// extern const table_methods tb_linkage_table_methods;
// extern const table_methods tb_param_table_methods;

const sql_methods sql_select= {
	sqlite3_select,
	sqlite3_select_next,
	sqlite3_select_end
};

static int sqlite3_select(sql_operate* d)
{
	M1_LOG_INFO("select\n");

	int ret = SQL_OPERATE_SUCCESS;
	extern sqlite3* db;
	const int tableName = d->table;
	const char* sql = d->sql;

	if(NULL == d->data){
		M1_LOG_ERROR("d->data NULL\n");		
	}

	if(SQLITE_OK != sqlite3_prepare_v2(db, sql, strlen(sql), &d->stmt ,NULL)){
		ret = SQL_OPERATE_FAILED;
		M1_LOG_ERROR( "sqlite3_prepare_v2 error: %s\n", sqlite3_errmsg(db));
		goto Finish;
	}

	/*选择要插入的表*/
	switch(tableName){
	case TB_ACCOUNT_INFO:
		if(SQL_OPERATE_SUCCESS != tb_account_info_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
		break;
	case TB_ACCOUNT_TABLE:
		if(SQL_OPERATE_SUCCESS != tb_account_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
		break;
	case TB_ALL_DEV:
		if(SQL_OPERATE_SUCCESS != tb_all_dev_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
		break;
	case TB_CONN_INFO:     
		if(SQL_OPERATE_SUCCESS != tb_conn_info_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
		break;
	case TB_DISTRICT_TABLE:
	    if(SQL_OPERATE_SUCCESS != tb_district_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
	    break;
	case TB_LINK_EXEC_TABLE: 
	    if(SQL_OPERATE_SUCCESS != tb_link_exec_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
	    break;
	case TB_LINK_TRIGGER_TABLE: 
		if(SQL_OPERATE_SUCCESS != tb_link_trigger_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
		break;
	case TB_LINKAGE_TABLE:
		if(SQL_OPERATE_SUCCESS != tb_linkage_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
		break;
	case TB_PARAM_TABLE:
		if(SQL_OPERATE_SUCCESS != tb_param_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
		break;
	case TB_PROJECT_TABLE:
        if(SQL_OPERATE_SUCCESS != tb_project_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
        break;
	case TB_SCEN_ALARM_TABLE:
	    if(SQL_OPERATE_SUCCESS != tb_scen_alarm_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
	    break;
	case TB_SCENARIO_TABLE:
	    if(SQL_OPERATE_SUCCESS != tb_scenario_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
	    break;

	default: M1_LOG_ERROR("not match table:%d\n",tableName);
		break;	
	}


	Finish:
	// if(SQL_OPERATE_FAILED == ret)
	// 	if(*d->stmt)
	// 		sqlite3_finalize(*d->stmt);
	
	return ret;
}

static int sqlite3_select_next(sql_operate* d)
{
	M1_LOG_INFO("select\n");

	int ret = SQL_OPERATE_SUCCESS;
	const int tableName = d->table;
	const char* sql = d->sql;
	extern sqlite3* db;

	if(NULL == d->data){
		M1_LOG_ERROR("d->data NULL\n");		
	}
	printf("sql:%s,table:%d\n",sql, tableName);
	/*选择要插入的表*/
	switch(tableName){
	case TB_ACCOUNT_INFO:
		if(SQL_OPERATE_SUCCESS != tb_account_info_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
		break;
	case TB_ACCOUNT_TABLE:
		if(SQL_OPERATE_SUCCESS != tb_account_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
		break;
	case TB_ALL_DEV:
		if(SQL_OPERATE_SUCCESS != tb_all_dev_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
		break;
	case TB_CONN_INFO:     
		if(SQL_OPERATE_SUCCESS != tb_conn_info_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
		break;
	case TB_DISTRICT_TABLE:
	    if(SQL_OPERATE_SUCCESS != tb_district_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
	    break;
	case TB_LINK_EXEC_TABLE: 
	    if(SQL_OPERATE_SUCCESS != tb_link_exec_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
	    break;
	case TB_LINK_TRIGGER_TABLE: 
		if(SQL_OPERATE_SUCCESS != tb_link_trigger_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
		break;
	case TB_LINKAGE_TABLE:
		if(SQL_OPERATE_SUCCESS != tb_linkage_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
		break;
	case TB_PARAM_TABLE:
		if(SQL_OPERATE_SUCCESS != tb_param_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
		break;
	case TB_PROJECT_TABLE:
        if(SQL_OPERATE_SUCCESS != tb_project_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
        break;
	case TB_SCEN_ALARM_TABLE:
	    if(SQL_OPERATE_SUCCESS != tb_scen_alarm_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
	    break;
	case TB_SCENARIO_TABLE:
	    if(SQL_OPERATE_SUCCESS != tb_scenario_table_methods.tb_select(d->stmt, d->data)){
			ret = SQL_OPERATE_FAILED; goto Finish;
		}
	    break;

	default: M1_LOG_ERROR("not match table:%d\n",tableName);
		break;	
	}

	Finish:

	return ret;
}

static void sqlite3_select_end(sql_operate* d)
{
	// sqlite3_stmt** stmt = d->stmt;
	M1_LOG_INFO("select_end\n");

	if(d->stmt)
		sqlite3_finalize(d->stmt);	
}

