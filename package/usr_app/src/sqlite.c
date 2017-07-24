#include "sqlite.h"
#include "thread.h"

extern pthread_mutex_t mut;

sqlite3* sqlite_open(sqlite3* db, char* db_name)
{
	char* err_msg = NULL;
	int rc;
	
	rc = sqlite3_open(db_name, &db);
	
	if(rc){
		fprintf(stderr, "can't open database: %s\n",sqlite3_errmsg(db));
		exit(0);
	}else{
		fprintf(stderr, "Opended database successfully\n");
	}
	
	return db;
	
}

static int callback(void* not_used, int argc, char** argv, char** az_col_name)
{
	int i;
	for(i = 0; i < argc; i++){
		printf("%s = %s\n", az_col_name[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
	
}

void sqlite_operate(sqlite3* db, char* sql)
{
	char* err_msg = NULL;
	int rc;
	
	rc = sqlite3_exec(db, sql, callback, 0, &err_msg);
	if(rc != SQLITE_OK){
		fprintf(stderr, "SQL error:%s\n",err_msg);
		sqlite3_free(err_msg);
	}else{
		fprintf(stdout, "Table operation successfully\n");
	}
	sqlite3_close(db);
}


void* sqlite_test(void* argc)
{
	printf("into thread sqlite test!\n");
    pthread_mutex_lock(&mut);

	sqlite3* db = NULL;
	
	char* sql_table = "CREATE TABLE COMPANY("               \
				"ID INT PRIMARY KEY  NOT NULL,"       \
				"NAME           TEXT NOT NULL,"       \
				"AGE            INT  NOT NULL,"       \
				"ADDRESS        CHAR(50),"            \
				"SALARY         REAL);"               ;
				
	char* sql_insert = "INSERT INTO COMPANY (ID, NAME, AGE, ADDRESS, SALARY)"       \
					   "VALUES (1, 'Paul', 32, 'California', 2000.00);"             \
					   "INSERT INTO COMPANY (ID, NAME, AGE, address, SALARY)"       \
					   "VALUES (2, 'Allen', 25, 'Texas', 15000.00);";               \
	
	char* sql_select = "SELECT * FROM COMPANY";
	
	char* sql_update = "UPDATE COMPANY set SALARY = 25000.00 where ID=1;"   \
					   "SELECT * FROM COMPANY";
					   
	char* sql_seg_del = "DELETE from COMPANY where ID=2;"    \
						  "SELECT * FROM COMPANY";
						  
	char* sql_del = "DROP TABLE COMPANY";
	
	/*delete table*/
	db = sqlite_open(db, "test.db");
	sqlite_operate(db, sql_del);
	/*create table*/
	db = sqlite_open(db, "test.db");
	sqlite_operate(db, sql_table);
	/*insert*/
	db = sqlite_open(db, "test.db");
	sqlite_operate(db, sql_insert);
	/*select*/
	db = sqlite_open(db, "test.db");
	sqlite_operate(db, sql_select);
	/*update*/
	db = sqlite_open(db, "test.db");
	sqlite_operate(db, sql_update);
	/*delete segement*/
	db = sqlite_open(db, "test.db");
	sqlite_operate(db, sql_seg_del);

	pthread_mutex_unlock(&mut);
    pthread_exit(NULL);
}


