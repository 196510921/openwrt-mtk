#ifndef _SQLITE_H_
#define _SQLITE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sqlite3.h>

int sqlite_exec(char* sql);
int sqlite_like(char* sql, void* callback_hd);

#endif //_SQLITE_H_

