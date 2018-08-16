#include <stdio.h>
#include "app_mem.h"
#include "app_all_dev_mem.h"
#include "app_conn_info_mem.h"
#include "app_param_table_mem.h"

#define APP_MEME_UNIT_TEST       1
#define APP_MEM_CONN_INFO_TEST   0
#define APP_MEM_PARAM_TABLE_TEST 0
#define APP_MEM_ALL_DEV_TEST     1


/*设备信息表*/
const app_mem_info_t app_mem_info[] = 
{
	{MEM_CONN_INFO,   &mem_app_conn_info,   "conn_info"},
	{MEM_PARAM_TABLE, &mem_app_param_table, "param_table"},
	{MEM_ALL_DEV,     &mem_app_all_dev,     "all_dev"},
	{MEM_MAX,         NULL,                 "数据信息最大值"}
};


/***********************************conn_info***************************************************************/
#if APP_MEM_CONN_INFO_TEST
extern int app_conn_info_mem_print();
static void app_conn_info_test(void)
{
	int ret = 0;
	int i   = 0;
	{
	    ret = app_mem_info[MEM_CONN_INFO].dFunc->app_mem_init();
	    if(ret == -1)
	    	printf("app_mem_init failed\n");
	}
	app_conn_info_mem_t select_data = {
			"12345678",
			1
	};
	for(i = 0; i < 2; i++)
	{	
		ret = app_mem_info[MEM_CONN_INFO].dFunc->app_mem_search(&select_data);	
		if(ret == -1)
		{
			ret = app_mem_info[MEM_CONN_INFO].dFunc->app_mem_insert(&select_data);
			if(ret == -1)
				printf("app_mem_insert failed\n");	
			else
				printf("app_mem_insert success\n");
			
			app_conn_info_mem_print();
		}
		else
		{
			select_data.client_fd = 2;
			ret = app_mem_info[MEM_CONN_INFO].dFunc->app_mem_update(&select_data);
			if(ret == -1)
				printf("app_mem_update failed\n");		
			else
				printf("app_mem_update success\n");
			
			app_conn_info_mem_print();
		}
		ret = app_mem_info[MEM_CONN_INFO].dFunc->app_mem_select(&select_data);
		if(ret == -1)
		{
			printf("app_mem_select not found\n");
		}	
		else
		{
			printf("app_mem_select client_fd:%d\n", select_data.client_fd);
		}
	}
	ret = app_mem_info[MEM_CONN_INFO].dFunc->app_mem_delete(&select_data);
    if(ret == -1)
    {
    	printf("app_mem_delete failed\n");
    }
    else
    {
    	printf("app_mem_delete success\n");
    	app_conn_info_mem_print();
    }	
}
#endif
/*************************************param_table**********************************************************************/
#if APP_MEM_PARAM_TABLE_TEST
extern int app_param_table_mem_print();
int app_param_table_test(void)
{
	int ret             = 0;
	int i               = 0;
	int num             = 0;
	cJSON *data_in_arry = NULL;
	cJSON *data_obj     = NULL;
	cJSON *dev_id       = NULL;
	cJSON *type         = NULL;
	cJSON *value        = NULL;
	/*create devData array*/
    data_in_arry = cJSON_CreateArray();
    if(NULL == data_in_arry)
    {
        cJSON_Delete(data_in_arry);
        return -1;
    } 
	{
	    ret = app_mem_info[MEM_PARAM_TABLE].dFunc->app_mem_init();
	    if(ret == -1)
	    	printf("app_mem_init failed\n");
	}
	app_param_table_mem_t select_data[3] = {
		{data_in_arry,"12345678",1001,1},
		{data_in_arry,"12345678",1002,2},
		{data_in_arry,"12345679",1003,3}
	};
	for(i = 0; i < 3; i++)
	{
		ret = app_mem_info[MEM_PARAM_TABLE].dFunc->app_mem_search(&select_data[i]);	
		if(ret == -1)
		{
			ret = app_mem_info[MEM_PARAM_TABLE].dFunc->app_mem_insert(&select_data[i]);
			if(ret == -1)
				printf("app_mem_insert failed\n");	
			else
				printf("app_mem_insert success\n");
			
			app_param_table_mem_print();
		}
		else
		{
			select_data[i].value = 4;
			ret = app_mem_info[MEM_PARAM_TABLE].dFunc->app_mem_update(&select_data[i]);
			if(ret == -1)
				printf("app_mem_update failed\n");		
			else
				printf("app_mem_update success\n");
			
			app_param_table_mem_print();
		}
	}
	ret = app_mem_info[MEM_PARAM_TABLE].dFunc->app_mem_search(&select_data[2]);	
	if(ret == -1)
	{
		ret = app_mem_info[MEM_PARAM_TABLE].dFunc->app_mem_insert(&select_data[2]);
		if(ret == -1)
			printf("app_mem_insert failed\n");	
		else
			printf("app_mem_insert success\n");
		
		app_param_table_mem_print();
	}
	else
	{
		select_data[2].value = 4;
		ret = app_mem_info[MEM_PARAM_TABLE].dFunc->app_mem_update(&select_data[2]);
		if(ret == -1)
			printf("app_mem_update failed\n");		
		else
			printf("app_mem_update success\n");
		
		app_param_table_mem_print();
	}
	ret = app_mem_info[MEM_PARAM_TABLE].dFunc->app_mem_select(&select_data[0]);
	if(ret == -1)
	{
		printf("app_mem_select not found\n");
	}	
	else
	{
		num = cJSON_GetArraySize(data_in_arry);
		for(i = 0; i < num; i++)
		{
			data_obj = cJSON_GetArrayItem(data_in_arry, i);
       		dev_id   = cJSON_GetObjectItem(data_obj, "DEV_ID");
       		type     = cJSON_GetObjectItem(data_obj, "TYPE");
       		value    = cJSON_GetObjectItem(data_obj, "VALUE");
			printf("app_mem_select data[%d]dev_id:%s,type:%d,value:%d\n",\
												 i,\
			                                     dev_id->valuestring,\
			                                     type->valueint,\
			                                     value->valueint);
		}
		
	}
	ret = app_mem_info[MEM_PARAM_TABLE].dFunc->app_mem_delete(&select_data[2]);
    if(ret == -1)
    {
    	printf("app_mem_delete failed\n");
    }
    else
    {
    	printf("app_mem_delete success\n");
    	app_param_table_mem_print();
    }
    return ret;
}
#endif
/******************************************all_dev*********************************************************************/
#if APP_MEM_ALL_DEV_TEST
extern int app_all_dev_mem_print();
int app_all_dev_test(void)
{
	int ret             = 0;
	int i               = 0;
	int num             = 0;
	cJSON *data_in_arry = NULL;
	cJSON *data_obj     = NULL;

	app_all_dev_tb  member_data[5];

	/*create devData array*/
    data_in_arry = cJSON_CreateArray();
    if(NULL == data_in_arry)
    {
        cJSON_Delete(data_in_arry);
        return -1;
    } 

	{
	    ret = app_mem_info[MEM_ALL_DEV].dFunc->app_mem_init();
	    if(ret == -1)
	    	printf("app_mem_init failed\n");
	}

	{
		/*0*/
		member_data[0].ap_id    = cJSON_CreateString("11111111");
		member_data[0].dev_id   = cJSON_CreateString("11111111");
		member_data[0].dev_name = cJSON_CreateString("DOA200-1");
		member_data[0].pid      = cJSON_CreateNumber(1001);
		member_data[0].added    = cJSON_CreateNumber(1);
		member_data[0].net      = cJSON_CreateNumber(1);
		member_data[0].status   = cJSON_CreateString("ON");
		/*1*/
		member_data[1].ap_id    = cJSON_CreateString("11111111");
		member_data[1].dev_id   = cJSON_CreateString("12345678");
		member_data[1].dev_name = cJSON_CreateString("DOS09-8");
		member_data[1].pid      = cJSON_CreateNumber(1008);
		member_data[1].added    = cJSON_CreateNumber(1);
		member_data[1].net      = cJSON_CreateNumber(1);
		member_data[1].status   = cJSON_CreateString("ON");
		/*2*/
		member_data[2].ap_id    = cJSON_CreateString("11111111");
		member_data[2].dev_id   = cJSON_CreateString("12345679");
		member_data[2].dev_name = cJSON_CreateString("DOS09-9");
		member_data[2].pid      = cJSON_CreateNumber(1009);
		member_data[2].added    = cJSON_CreateNumber(1);
		member_data[2].net      = cJSON_CreateNumber(1);
		member_data[2].status   = cJSON_CreateString("ON");
		/*3*/
		member_data[3].ap_id    = NULL;
		member_data[3].dev_id   = NULL;//cJSON_CreateString("12345679");
		member_data[3].dev_name = NULL;//cJSON_CreateString("DOS09-9");
		member_data[3].pid      = NULL;//cJSON_CreateNumber(1009);
		member_data[3].added    = NULL;//cJSON_CreateNumber(1);
		member_data[3].net      = NULL;//cJSON_CreateNumber(1);
		member_data[3].status   = NULL;

		/*4*/
		member_data[4].ap_id    = cJSON_CreateString("11111111");
		member_data[4].dev_id   = NULL;//cJSON_CreateString("12345679");
		member_data[4].dev_name = NULL;//cJSON_CreateString("DOS09-9");
		member_data[4].pid      = NULL;//cJSON_CreateNumber(1009);
		member_data[4].added    = NULL;//cJSON_CreateNumber(1);
		member_data[4].net      = NULL;//cJSON_CreateNumber(1);
		member_data[4].status   = NULL;//cJSON_CreateString("ON");
	}

	app_all_dev_mem_t select_data[4] = {
		{data_in_arry,&member_data[0]},
		{data_in_arry,&member_data[1]},
		{data_in_arry,&member_data[2]},
		{data_in_arry,&member_data[3]}
	};

	/*insert*/
	for(i = 0; i < 3; i++)
	{
		ret = app_mem_info[MEM_ALL_DEV].dFunc->app_mem_search(&select_data[i]);	
		if(ret == -1)
		{
			ret = app_mem_info[MEM_ALL_DEV].dFunc->app_mem_insert(&select_data[i]);
			if(ret == -1)
				printf("app_mem_insert failed\n");	
			else
				printf("app_mem_insert success\n");
			
			app_all_dev_mem_print();
		}
	}

	/*update*/
	member_data[2].ap_id    = cJSON_CreateString("11111112");
	member_data[2].dev_id   = cJSON_CreateString("12345679");
	member_data[2].dev_name = cJSON_CreateString("DOS09-2");
	member_data[2].pid      = cJSON_CreateNumber(1002);
	member_data[2].added    = cJSON_CreateNumber(0);
	member_data[2].net      = cJSON_CreateNumber(0);
	member_data[2].status   = cJSON_CreateString("OFF");

	ret = app_mem_info[MEM_ALL_DEV].dFunc->app_mem_search(&select_data[2]);	
	if(ret == -1)
	{
		ret = app_mem_info[MEM_ALL_DEV].dFunc->app_mem_insert(&select_data[2]);
		if(ret == -1)
			printf("app_mem_insert failed\n");	
		else
			printf("app_mem_insert success\n");
		
		app_all_dev_mem_print();
	}
	else
	{
		ret = app_mem_info[MEM_ALL_DEV].dFunc->app_mem_update(&select_data[2]);
		if(ret == -1)
			printf("app_mem_update failed\n");		
		else
			printf("app_mem_update success\n");
		
		app_all_dev_mem_print();
	}

	ret = app_mem_info[MEM_ALL_DEV].dFunc->app_mem_select(&select_data[3]);
	if(ret == -1)
	{
		printf("app_mem_select not found\n");
	}	
	else
	{
		num = cJSON_GetArraySize(data_in_arry);
		for(i = 0; i < num; i++)
		{
			data_obj = cJSON_GetArrayItem(data_in_arry, i);
       		member_data[3].dev_id   = cJSON_GetObjectItem(data_obj, "DEV_ID");
       		member_data[3].ap_id    = cJSON_GetObjectItem(data_obj, "AP_ID");
       		member_data[3].dev_name = cJSON_GetObjectItem(data_obj, "DEV_NAME");
       		member_data[3].pid      = cJSON_GetObjectItem(data_obj, "PID");
       		member_data[3].added    = cJSON_GetObjectItem(data_obj, "ADDED");
       		member_data[3].net      = cJSON_GetObjectItem(data_obj, "NET");
       		member_data[3].status   = cJSON_GetObjectItem(data_obj, "STATUS");
			printf("app_mem_select data[%d]dev_id:%s,ap_id:%s,dev_name:%s,pid:%d,added:%d,net:%d,status:%s\n",\
												                           i,\
			                                                               member_data[3].dev_id->valuestring,\
			                                                               member_data[3].ap_id->valuestring,\
			                                                               member_data[3].dev_name->valuestring,\
			                                                               member_data[3].pid->valueint,\
			                                                               member_data[3].added->valueint,\
			                                                               member_data[3].net->valueint,\
			                                                               member_data[3].status->valuestring);
		}
		
	}

	ret = app_mem_info[MEM_ALL_DEV].dFunc->app_mem_delete(&select_data[2]);
    if(ret == -1)
    {
    	printf("app_mem_delete failed\n");
    }
    else
    {
    	printf("app_mem_delete success\n");
    	app_all_dev_mem_print();
    }
    return ret;
}
#endif


#if APP_MEME_UNIT_TEST
int main(int argc, char* argv[])
{
	#if APP_MEM_CONN_INFO_TEST
	app_conn_info_test();
	#endif

	#if APP_MEM_PARAM_TABLE_TEST
	app_param_table_test();
	#endif

	#if APP_MEM_ALL_DEV_TEST
	app_all_dev_test();
	#endif

	return 0;
}
#endif
