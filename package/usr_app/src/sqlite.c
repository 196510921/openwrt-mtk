#include "sqlite.h"

void (* test_func)(int argc, char** argv, char** az_col_name);


static sqlite3* sqlite_open(sqlite3* db, char* db_name)
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

static void handle_test_1 (int argc, char** argv, char** az_col_name)
{
	int i;
	for(i = 0; i < argc; i++){
		printf("%s = %s\n", az_col_name[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	printf("handle_test_1\n");	
}


static int callback(void* not_used, int argc, char** argv, char** az_col_name)
{
	test_func = not_used;
	(*test_func)(argc,argv,az_col_name);

	return 0;	
}

int sqlite_exec(char* sql)
{
	sqlite3* db = NULL;
	char* err_msg = NULL;
	int rc;

	db = sqlite_open(db, "dev_info.db");

	rc = sqlite3_exec(db, sql, callback, 0, &err_msg);
	if(rc != SQLITE_OK){
		fprintf(stderr, "SQL error:%s\n",err_msg);
		sqlite3_free(err_msg);
	}else{
		fprintf(stdout, "Table operation successfully\n");
	}
	sqlite3_close(db);

	return rc;
}

int sqlite_like(char* sql, void* callback_hd)
{
	sqlite3* db = NULL;
	char* err_msg = NULL;
	int rc, tmp;

	db = sqlite_open(db, "dev_info.db");

	rc = sqlite3_exec(db, sql, callback, callback_hd, &err_msg);
	if(rc != SQLITE_OK){
		fprintf(stderr, "SQL error:%s\n",err_msg);
		sqlite3_free(err_msg);
	}else{
		fprintf(stdout, "Table operation successfully\n");
	}
	sqlite3_close(db);

	return rc;
}

void* sqlite_test(void* argc)
{
	printf("into sqlite test!\n");

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

	sqlite3_stmt* stmt = NULL;

	int rc;
	const char* data = NULL;
	/*1*/
	db = sqlite_open(db, "dev_info.db");

	rc = sqlite3_prepare_v2(db, "CREATE TABLE tb1 (str TEXT)", -1, &stmt, NULL);
	if(rc != SQLITE_OK)
		exit(-1);
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE)
		exit(-1);
	sqlite3_finalize(stmt);
	/*2*/
	rc = sqlite3_prepare_v2(db, "SELECT str FROM tb1 ORDER BY 1", -1, &stmt, NULL);
	if(rc != SQLITE_OK)
		exit(-1);
	while(sqlite3_step(stmt) == SQLITE_ROW){
		data = (const char*)sqlite3_column_text(stmt, 0);
		printf("%s\n", data ? data:"[NULL]");
	}
	
	sqlite3_finalize(stmt);

	/*static char sql_test[200];
	uint16_t d_type = 0x0001;
	uint16_t port = 0x6688;
	uint8_t id_buf[8] = {0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37};
	
	//sprintf(sql_test,"INSERT INTO dev_sum(ID, TYPE, NAME, PORT, AP, TIME, LINK, SCEN, DISTRIC) VALUES(%s, %x, %s, %x, %s, %s, %s, %s, %s);","'1234567B'",d_type,"'FF'",port,"'FF'","'FF'","'FF'","'FF'","'FF'");
	sprintf(sql_test,"INSERT INTO dev_sum(ID, TYPE, NAME, PORT, AP, TIME, LINK, SCEN, DISTRIC) VALUES('%s', %x, '%s', %x, '%s', '%s', '%s', '%s', '%s');",id_buf ,d_type,"FF",port,"FF","FF","FF","FF","FF");
	//printf("%s\n",sql_test);
	sqlite_exec(sql_test);*/
	/*int rc;
	sprintf(sql_test,"SELECT TYPE FROM dev_sum WHERE ID LIKE %s;","'FF'");
	rc = sqlite_like(sql_test, handle_test_1);
	if(rc == SQLITE_OK)
		printf("match ok\n");
	else
		printf("no match\n");
*/


}


