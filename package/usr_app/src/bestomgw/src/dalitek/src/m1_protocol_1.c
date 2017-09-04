#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "m1_protocol.h"
#include "socket_server.h"

extern fifo_t dev_data_fifo;
void trigger_cb(void* udp, int type, char const* db_name, char const* table_name, sqlite3_int64 rowid);
//void* sqlite3_update_hool(sqlite3* db, update_callback, void* udp);

static char* linkage_status(char* condition, int threshold, int value)
{	
	char* status = NULL;
	int tmp;

	tmp = strcmp(condition, "<");
	if (0 == tmp){ 
		if(value < threshold)
			status = "OFF";
		else 
			status = "ON";
	}
	else {
		if(value >= threshold)
			status = "ON";
		else
			status = "OFF";
	}
	printf("linkage_status:%s, value:%d, threshold:%d\n",status,value, threshold);

	return status;
}

int trigger_cb_handle(void)
{
	int rc ,value, param_type, threshold;
	uint32_t rowid;
	char* devId = NULL, *condition = NULL, *status = NULL, *link_name = NULL;
	char sql[200], sql_1[200], sql_2[200];

	sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL,*stmt_1 = NULL, *stmt_2 = NULL;

    rc = sqlite3_open("dev_info.db", &db);  
    if(rc){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }
    /*添加update/insert/delete监察*/
    rc = sqlite3_update_hook(db, trigger_cb, "trigger_cb_handle");
    if(rc){
        fprintf(stderr, "sqlite3_update_hook falied: %s\n", sqlite3_errmsg(db));  
    }

    while(fifo_read(&dev_data_fifo, &rowid)){
		/*check linkage table*/
		sprintf(sql,"select VALUE, DEV_ID, TYPE from param_table where rowid = %05d;",rowid);
		//printf("sql:%s\n",sql);
		sqlite3_reset(stmt); 
		rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
		if(rc){
			printf("sqlite3_prepare_v2 failed\n");
			return M1_PROTOCOL_FAILED;
		}else{
			printf("sqlite3_prepare_v2 ok\n");  		
		}
		rc = sqlite3_step(stmt);
		printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
	    value = sqlite3_column_int(stmt,0);
	    devId = sqlite3_column_text(stmt,1);
	    param_type = sqlite3_column_int(stmt,2);
	    printf("value:%05d, devId:%s, param_type:\n",value, devId, param_type);
	    /*get linkage table*/
	    sprintf(sql_1,"select THRESHOLD,CONDITION,LINK_NAME from linkage_table where DEV_ID = \"%s\" and TYPE = %05d;",devId,param_type);
		printf("sql_1:%s\n",sql_1);
		sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL);
		while(sqlite3_step(stmt_1) == SQLITE_ROW){
			threshold = sqlite3_column_int(stmt_1,0);
	        condition = sqlite3_column_text(stmt_1,1);
	        link_name = sqlite3_column_text(stmt_1,2);
	        printf("threshold:%05d, condition:%s\n, link_name:%s\n", threshold, condition, link_name);
	 		status = linkage_status(condition, threshold, value);
	 		/*set linkage table*/
	 		sprintf(sql_2,"update linkage_table set STATUS = \"%s\" where DEV_ID = \"%s\" and TYPE = %05d and LINK_NAME = \"%s\" ;",status,devId,param_type,link_name);
	 		//printf("sql_2:%s\n",sql_2);
	 		sqlite3_reset(stmt_2);
			sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL);
			rc = sqlite3_step(stmt_2);
			printf("step2() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 		
	 	}
 	}

 	sqlite3_finalize(stmt);
 	sqlite3_finalize(stmt_1);
 	sqlite3_finalize(stmt_2);
    sqlite3_close(db);
     
}

void trigger_cb(void* udp, int type, char const* db_name, char const* table_name, sqlite3_int64 rowid)
{
	int rc;

	printf("trigger_cb\n");
	rc = strcmp(udp, "AP_report_data_handle");
	if(0 == rc)
	{
		fifo_write(&dev_data_fifo, rowid);
	}
	
}


void fifo_init(fifo_t* fifo, uint32_t* buffer, uint32_t len)
{
    fifo->buffer = buffer;
    fifo->len = len;
    fifo->wptr = fifo->rptr = 0;
}

void fifo_write(fifo_t* fifo, uint32_t d)
{
    fifo->buffer[fifo->wptr] = d;
    fifo->wptr = (fifo->wptr + 1) % fifo->len;
}

uint32_t fifo_read(fifo_t* fifo, uint32_t* d)
{
    if (fifo->wptr == fifo->rptr) return 0;
    
    *d = fifo->buffer[fifo->rptr];
    fifo->rptr = (fifo->rptr + 1) % fifo->len;
    
    return 1;
}




