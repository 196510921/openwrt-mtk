#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "m1_protocol.h"
#include "socket_server.h"


int district_create_handle(payload_t data)
{
	printf("district_create_handle\n");
	cJSON* districtNameJson = NULL;
	cJSON* apIdJson = NULL;
	cJSON* apIdArrayJson = NULL;

	sqlite3* db = NULL;
	sqlite3_stmt* stmt = NULL;
	int id, rc, number1,i;
	int row_number = 0;
	char time[30];
	char sql_1[200];

	if(data.pdu == NULL) return M1_PROTOCOL_FAILED;
	getNowTime(time);

    rc = sqlite3_open("dev_info.db", &db);  
    if( rc ){  
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));  
        return M1_PROTOCOL_FAILED;  
    }else{  
        fprintf(stderr, "Opened database successfully\n");  
    }

	/*获取table id*/
	char* sql = "select ID from district_table order by ID desc limit 1";
	/*linkage_table*/
	id = sql_id(db, sql);

	/*获取收到数据包信息*/
    districtNameJson = cJSON_GetObjectItem(data.pdu, "districtName");
    printf("districtName:%s\n",districtNameJson->valuestring);
    apIdArrayJson = cJSON_GetObjectItem(data.pdu, "apId");
    number1 = cJSON_GetArraySize(apIdArrayJson);
    printf("number1:%d\n",number1);

   	/*删除原有表scenario_table中的旧scenario*/
	sprintf(sql_1,"select ID from district_table where DIS_NAME = \"%s\";",districtNameJson->valuestring);	
	row_number = sql_row_number(db, sql_1);
	printf("row_number:%d\n",row_number);
	if(row_number > 0){
		sprintf(sql_1,"delete from district_table where DIS_NAME = \"%s\";",districtNameJson->valuestring);				
		sqlite3_reset(stmt);
		sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt, NULL);
		while(sqlite3_step(stmt) == SQLITE_ROW);
	}
    
    /*存取到数据表scenario_table中*/
    for(i = 0; i < number1; i++){
		apIdJson = cJSON_GetArrayItem(apIdArrayJson, i);
		printf("apId:%s\n",apIdJson->valuestring);

		sql = "insert into district_table(ID, DIS_NAME, AP_ID, TIME) values(?,?,?,?);";
		printf("sql:%s\n",sql);
		sqlite3_reset(stmt);
		sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
		sqlite3_bind_int(stmt, 1, id);
		id++;
		sqlite3_bind_text(stmt, 2, districtNameJson->valuestring, -1, NULL);
		sqlite3_bind_text(stmt, 3, apIdJson->valuestring, -1, NULL);
		sqlite3_bind_text(stmt, 4, time, -1, NULL);
		rc = sqlite3_step(stmt); 
		printf("step() return %s\n", rc == SQLITE_DONE ? "SQLITE_DONE": rc == SQLITE_ROW ? "SQLITE_ROW" : "SQLITE_ERROR"); 
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return M1_PROTOCOL_OK;
    
}






