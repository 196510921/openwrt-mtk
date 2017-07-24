#ifndef _SQLITE_H_
#define _SQLITE_H_

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

sqlite3* sqlite_open(sqlite3* db, char* db_name);
void sqlite_operate(sqlite3* db, char* sql);


#endif //_SQLITE_H_

