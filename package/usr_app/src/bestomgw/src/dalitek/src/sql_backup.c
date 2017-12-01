#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

#include "sqlite3.h"
#include "m1_protocol.h"

/*MACRO**********************************************************************************************************/
#define SQL_CLEAR_TIME     (1 * 60 * 60)

/*数据库冗余删除*/
static int sql_history_data_del(char* time, char* tableName)
{
    int rc;
    int ret = M1_PROTOCOL_OK;
    char* errorMsg = NULL;
    char* sql = malloc(300);
    sqlite3* db = NULL;

    fprintf(stdout, "sql_history_data_del\n");

    rc = sqlite3_open("dev_info.db", &db);
    if( rc != SQLITE_OK){  
        fprintf(stderr, "Can't open database\n");  
        goto Finish;
    }else{  
        fprintf(stdout, "Opened database successfully\n");  
    }

    sprintf(sql,"delete from \"%s\" where TIME < \"%s\";", tableName, time);
    if(sqlite3_exec(db, sql, NULL, NULL, &errorMsg) == SQLITE_OK){
        fprintf(stdout,"sql_history_data_del ok\n");
    }else{
        ret = M1_PROTOCOL_FAILED;
        fprintf(stdout,"sql_history_data_del falied:%s\n",errorMsg);
    }

    Finish:
    free(errorMsg);
    free(sql);

    sqlite3_close(db);

    return ret;
}

/*sqlite3 数据库备份*/
int sql_backup(void)
{
    static int time_syc_flag = 0;
    static int preTime = 0;
    int ret = M1_PROTOCOL_OK;
    char* _time = (char*)malloc(30);
    char* table = "param_table";
    /*获取当前时间*/
    struct tm nowTime;

    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);  //获取相对于1970到现在的秒数
    if(time_syc_flag == 0){
        time_syc_flag = 1;
        preTime = time.tv_sec;       
    }

    fprintf(stdout,"time:%05d, preTime:%05d\n",time.tv_sec, preTime);
    if(time.tv_sec - preTime < SQL_CLEAR_TIME){
        return ret;
    }else{
        preTime = time.tv_sec;
        /*基于当前时间向后半小时*/
        time.tv_sec -= SQL_CLEAR_TIME;
        localtime_r(&time.tv_sec, &nowTime);    

        sprintf(_time, "%04d%02d%02d%02d%02d%02d", nowTime.tm_year + 1900, nowTime.tm_mon+1, nowTime.tm_mday, 
          nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec);

        ret = sql_history_data_del(_time, table);
    }

    free(_time);

    return ret;
}






