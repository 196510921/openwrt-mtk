#ifndef _SQL_OPERATE_H_
#define _SQL_OPERATE_H_

#include <stdint.h>
#include "utils.h"
#include "sqlite3.h"

typedef enum sql_operate_result sql_operate_result;
enum sql_operate_result {
	SQL_OPERATE_FAILED = 0,
	SQL_OPERATE_SUCCESS,
};

typedef struct sql_operate sql_operate;
struct sql_operate{
	//sqlite3* db;
	sqlite3_stmt* stmt;
	const char* sql;
	const int table;
	void* data;
	int num;
};

/*insert*/
typedef struct sql_methods sql_methods;
struct sql_methods{
	int (*sql_methods)(sql_operate*);
	int (*sql_methods_next)(sql_operate*);
	void* (*sql_methods_end)(sql_operate*);
};

extern const sql_methods sql_insert;
extern const sql_methods sql_select;
extern const sql_methods sql_delete;
extern const sql_methods sql_modify;

int sql_begin_transaction(sqlite3* db);
int sql_commit_transaction(sqlite3* db);
void sql_rollback(sqlite3* db);
int sql_step(sqlite3* db, char* sql);
#endif //_SQL_OPERATE_H_


