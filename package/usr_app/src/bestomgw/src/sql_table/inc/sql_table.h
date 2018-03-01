#ifndef _SQL_TABLE_H_
#define _SQL_TABLE_H_

#include <stdint.h>
#include "utils.h"
#include "sqlite3.h"


typedef enum sql_table_result sql_table_result;
enum sql_table_result {
	SQL_TABLE_FAILED = 0,
	SQL_TABLE_SUCCESS,
};

typedef struct table_methods table_methods;
struct table_methods{
	char* table_type;
	int (*tb_bind)(sqlite3_stmt*, void*);
	int (*tb_select)(sqlite3_stmt*, void*);
};

typedef enum sql_table_type sql_table_type;
enum sql_table_type {
	TB_ACCOUNT_INFO = 1,
	TB_ACCOUNT_TABLE,
	TB_ALL_DEV,
	TB_CONN_INFO,
	TB_DISTRICT_TABLE,
	TB_LINK_EXEC_TABLE,
	TB_LINK_TRIGGER_TABLE,
	TB_LINKAGE_TABLE,
	TB_PARAM_TABLE,
	TB_PROJECT_TABLE,
	TB_SCEN_ALARM_TABLE,
	TB_SCENARIO_TABLE,
};

/*sqlite3 table: account_info*/
typedef struct tb_account_info tb_account_info;
struct tb_account_info{
	int id;
	char* account;
	int client_fd;
};

/*sqlite3 table: account_table*/
typedef struct tb_account_table tb_account_table;
struct tb_account_table{
	int id;
	char* account;
	char* key;
	char* key_auth;
	char* remote_auth;
	char* time;
};

/*sqlite3 table: all_dev*/
typedef struct tb_all_dev tb_all_dev;
struct tb_all_dev{
	int id;
	char* dev_id;
	char* dev_name;
	char* ap_id;
	int pid;
	int added;
	int net;
	char* status;
	char* account;
	char* time;
};

/*sqlite3 table: conn_info*/
typedef struct tb_conn_info tb_conn_info;
struct tb_conn_info{
	int id;
	char* ap_id;
	int client_fd;
};

/*sqlite3 table: district_table*/
typedef struct tb_district_table tb_district_table;
struct tb_district_table{
	int id;
	char* dis_name;
	char* dis_pic;
	char* ap_id;
	char* account;
	char* time;
};

/*sqlite3 table: link_exec_table*/
typedef struct tb_link_exec_table tb_link_exec_table;
struct tb_link_exec_table{
	int id;
	char* link_name;
	char* district;
	char* ap_id;
	char* dev_id;
	int type;
	int value;
	int delay;
	char* time;
};

/*sqlite3 table: link_trigger_table*/
typedef struct tb_link_trigger_table tb_link_trigger_table;
struct tb_link_trigger_table{
	int id;
	char* link_name;
	char* district;
	char* ap_id;
    char* dev_id;
    int type;
    int threshold;
    char* condition;
    char* logical;
    char* status;
    char* time;
};

/*sqlite3 table: linkage_table*/
typedef struct tb_linkage_table tb_linkage_table;
struct tb_linkage_table{
	int id;
	char* link_name;
	char* district;
	char* exec_type;
	char* exec_id;
	char* status;
	char* enable;
	char* time;
};

/*sqlite3 table: param_table*/
typedef struct tb_param_table tb_param_table;
struct tb_param_table{
	int id;
	char* dev_id;
	char* dev_name;
	int type;
	int value;
	char* time;
};

/*sqlite3 table: project_table*/
typedef struct tb_project_table tb_project_table;
struct tb_project_table{
	int id;
	char* p_name;
	char* p_number;
	char* p_creator;
	char* p_manager;
	char* p_editor;
	char* p_tel;
	char* p_add;
	char* p_brief;
	char* p_key;
	char* account;
	char* time;
};

/*sqlite3 table: scen_alarm_table*/
typedef struct tb_scen_alarm_table tb_scen_alarm_table;
struct tb_scen_alarm_table{
	int id;
	char* scen_name;
	int hour;
	int minutes;
	char* week;
	char* status;
	char* time;
};

/*sqlite3 table: scenario_table*/
typedef struct tb_scenario_table tb_scenario_table;
struct tb_scenario_table{
	int id;
	char* scen_name;
	char* scen_pic;
	char* district;
	char* ap_id;
	char* dev_id;
	int type;
	int value;
	int delay;
	char* account;
	char* time;
};


extern const table_methods tb_account_info_methods;
extern const table_methods tb_account_table_methods;
extern const table_methods tb_all_dev_methods;
extern const table_methods tb_conn_info_methods;
extern const table_methods tb_district_table_methods;
extern const table_methods tb_link_exec_table_methods;
extern const table_methods tb_link_trigger_table_methods;
extern const table_methods tb_linkage_table_methods;
extern const table_methods tb_param_table_methods;
extern const table_methods tb_project_table_methods;
extern const table_methods tb_scen_alarm_table_methods;
extern const table_methods tb_scenario_table_methods;

#endif //_SQL_TABLE_H_



