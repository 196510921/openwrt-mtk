#include <stdio.h>
#include "app_mem.h"
#include "app_all_dev_mem.h"
#include "app_conn_info_mem.h"
#include "app_param_table_mem.h"
#include "app_linkage_table_mem.h"
#include "app_scenario_table_mem.h"
#include "app_scen_alarm_table_mem.h"

#define APP_MEME_UNIT_TEST            1
#define APP_MEM_CONN_INFO_TEST        0
#define APP_MEM_PARAM_TABLE_TEST      0
#define APP_MEM_ALL_DEV_TEST          0
#define APP_MEM_LINKAGE_TABLE_TEST    0
#define APP_MEM_SCENARIO_TABLE_TEST   0
#define APP_MEM_SCEN_ALARM_TABLE_TEST 1


/*设备信息表*/
const app_mem_info_t app_mem_info[] = 
{
	{MEM_CONN_INFO,        &mem_app_conn_info,        "conn_info"},
	{MEM_PARAM_TABLE,      &mem_app_param_table,      "param_table"},
	{MEM_ALL_DEV,          &mem_app_all_dev,          "all_dev"},
	{MEM_LINKAGE_TABLE,    &mem_app_linkage_table,    "linkage_table"},
	{MEM_SCENARIO_TABLE,   &mem_app_scenario_table,   "scenario_table"},
	{MEM_SCEN_ALARM_TABLE, &mem_app_scen_alarm_table, "scenario_table"},
	{MEM_MAX,              NULL,                      "数据信息最大值"}
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

/******************************************linkage_table*********************************************************************/
#if APP_MEM_LINKAGE_TABLE_TEST
extern int app_linkage_table_mem_print();
int app_linkage_table_test(void)
{
	int ret             = 0;
	int i               = 0;
	int num             = 0;
	cJSON *data_in_arry = NULL;
	cJSON *data_obj     = NULL;

	app_linkage_table_tb  member_data[5];

	/*create devData array*/
    data_in_arry = cJSON_CreateArray();
    if(NULL == data_in_arry)
    {
        cJSON_Delete(data_in_arry);
        return -1;
    } 

	{
	    ret = app_mem_info[MEM_LINKAGE_TABLE].dFunc->app_mem_init();
	    if(ret == -1)
	    	printf("app_mem_init failed\n");
	}

	{
		/*0*/
		member_data[0].link_name = cJSON_CreateString("LIGHT_ON");
		member_data[0].district  = cJSON_CreateString("district1");
		member_data[0].exec_type = cJSON_CreateString("11111111");
		member_data[0].exec_id   = cJSON_CreateString("11111111");
		member_data[0].status    = cJSON_CreateString("ON");
		member_data[0].enable    = cJSON_CreateString("ENABLE");
		/*1*/
		member_data[1].link_name = cJSON_CreateString("LIGHT_OFF");
		member_data[1].district  = cJSON_CreateString("district1");
		member_data[1].exec_type = cJSON_CreateString("22222222");
		member_data[1].exec_id   = cJSON_CreateString("22222222");
		member_data[1].status    = cJSON_CreateString("OFF");
		member_data[1].enable    = cJSON_CreateString("ENABLE");
		/*2*/
		member_data[2].link_name = cJSON_CreateString("AC_ON");
		member_data[2].district  = cJSON_CreateString("district2");
		member_data[2].exec_type = cJSON_CreateString("33333333");
		member_data[2].exec_id   = cJSON_CreateString("33333333");
		member_data[2].status    = cJSON_CreateString("ON");
		member_data[2].enable    = cJSON_CreateString("ENABLE");
		/*3*/
		member_data[3].link_name = NULL;
		member_data[3].district  = NULL;
		member_data[3].exec_type = NULL;
		member_data[3].exec_id   = NULL;
		member_data[3].status    = NULL;
		member_data[3].enable    = NULL;

		/*4*/
		member_data[4].link_name = cJSON_CreateString("AC_ON");;
		member_data[4].district  = NULL;
		member_data[4].exec_type = NULL;
		member_data[4].exec_id   = NULL;
		member_data[4].status    = NULL;
		member_data[4].enable    = NULL;
	}

	app_linkage_table_mem_t select_data[4] = {
		{data_in_arry,&member_data[0]},
		{data_in_arry,&member_data[1]},
		{data_in_arry,&member_data[2]},
		{data_in_arry,&member_data[4]}
	};

	/*insert*/
	for(i = 0; i < 3; i++)
	{
		ret = app_mem_info[MEM_LINKAGE_TABLE].dFunc->app_mem_search(&select_data[i]);	
		if(ret == -1)
		{
			ret = app_mem_info[MEM_LINKAGE_TABLE].dFunc->app_mem_insert(&select_data[i]);
			if(ret == -1)
				printf("app_mem_insert failed\n");	
			else
				printf("app_mem_insert success\n");
			
			app_linkage_table_mem_print();
		}
	}

	/*update*/
	member_data[2].link_name = cJSON_CreateString("AC_ON");
	member_data[2].district  = cJSON_CreateString("district2");
	member_data[2].exec_type = cJSON_CreateString("66666666");
	member_data[2].exec_id   = cJSON_CreateString("66666666");
	member_data[2].status    = cJSON_CreateString("OFF");
	member_data[2].enable    = cJSON_CreateString("DISABLE");

	ret = app_mem_info[MEM_LINKAGE_TABLE].dFunc->app_mem_search(&select_data[2]);	
	if(ret == -1)
	{
		ret = app_mem_info[MEM_LINKAGE_TABLE].dFunc->app_mem_insert(&select_data[2]);
		if(ret == -1)
			printf("app_mem_insert failed\n");	
		else
			printf("app_mem_insert success\n");
		
		app_linkage_table_mem_print();
	}
	else
	{
		ret = app_mem_info[MEM_LINKAGE_TABLE].dFunc->app_mem_update(&select_data[2]);
		if(ret == -1)
			printf("app_mem_update failed\n");		
		else
			printf("app_mem_update success\n");
		
		app_linkage_table_mem_print();
	}

	ret = app_mem_info[MEM_LINKAGE_TABLE].dFunc->app_mem_select(&select_data[3]);
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
       		member_data[3].link_name   = cJSON_GetObjectItem(data_obj, "LINK_NAME");
       		member_data[3].district    = cJSON_GetObjectItem(data_obj, "DISTRICT");
       		member_data[3].exec_type = cJSON_GetObjectItem(data_obj, "EXEC_TYPE");
       		member_data[3].exec_id      = cJSON_GetObjectItem(data_obj, "EXEC_ID");
       		member_data[3].status    = cJSON_GetObjectItem(data_obj, "STATUS");
       		member_data[3].enable      = cJSON_GetObjectItem(data_obj, "ENABLE");
			printf("app_mem_select data[%d]link_name:%s,district:%s,exec_type:%s,exec_id:%s,status:%s,enable:%s\n",\
												                           i,\
			                                                               member_data[3].link_name->valuestring,\
			                                                               member_data[3].district->valuestring,\
			                                                               member_data[3].exec_type->valuestring,\
			                                                               member_data[3].exec_id->valuestring,\
			                                                               member_data[3].status->valuestring,\
			                                                               member_data[3].enable->valuestring);
		}
		
	}

	ret = app_mem_info[MEM_LINKAGE_TABLE].dFunc->app_mem_delete(&select_data[2]);
    if(ret == -1)
    {
    	printf("app_mem_delete failed\n");
    }
    else
    {
    	printf("app_mem_delete success\n");
    	app_linkage_table_mem_print();
    }
    return ret;
}
#endif

/******************************************scenario_table*********************************************************************/
#if APP_MEM_SCENARIO_TABLE_TEST
extern int app_scenario_table_mem_print();
int app_scenario_table_test(void)
{
	int ret             = 0;
	int i               = 0;
	int num             = 0;
	cJSON *data_in_arry = NULL;
	cJSON *data_obj     = NULL;

	app_scenario_table_tb  member_data[5];

	/*create devData array*/
    data_in_arry = cJSON_CreateArray();
    if(NULL == data_in_arry)
    {
        cJSON_Delete(data_in_arry);
        return -1;
    } 

	{
	    ret = app_mem_info[MEM_SCENARIO_TABLE].dFunc->app_mem_init();
	    if(ret == -1)
	    	printf("app_mem_init failed\n");
	}

	{
		/*0*/
		member_data[0].scen_name = cJSON_CreateString("SCEN_1_ON");
		member_data[0].scen_pic  = cJSON_CreateString("scen_pic_1");
		member_data[0].district  = cJSON_CreateString("district1");
		member_data[0].ap_id     = cJSON_CreateString("11111111");
		member_data[0].dev_id    = cJSON_CreateString("12345678");
		member_data[0].type      = cJSON_CreateNumber(1001);
		member_data[0].value     = cJSON_CreateNumber(2);
		member_data[0].delay     = cJSON_CreateNumber(3);
		/*1*/
		member_data[1].scen_name = cJSON_CreateString("SCEN_2_OFF");
		member_data[1].scen_pic  = cJSON_CreateString("scen_pic_2");
		member_data[1].district  = cJSON_CreateString("district2");
		member_data[1].ap_id     = cJSON_CreateString("11111111");
		member_data[1].dev_id    = cJSON_CreateString("12345679");
		member_data[1].type      = cJSON_CreateNumber(1002);
		member_data[1].value     = cJSON_CreateNumber(4);
		member_data[1].delay     = cJSON_CreateNumber(5);
		/*2*/
		member_data[2].scen_name = cJSON_CreateString("SCEN_3_ON");
		member_data[2].scen_pic  = cJSON_CreateString("scen_pic_3");
		member_data[2].district  = cJSON_CreateString("district3");
		member_data[2].ap_id     = cJSON_CreateString("22222222");
		member_data[2].dev_id    = cJSON_CreateString("1234567a");
		member_data[2].type      = cJSON_CreateNumber(1003);
		member_data[2].value     = cJSON_CreateNumber(6);
		member_data[2].delay     = cJSON_CreateNumber(7);
		/*3*/
		member_data[3].scen_name = NULL;
		member_data[3].scen_pic  = NULL;
		member_data[3].district  = NULL;
		member_data[3].ap_id     = NULL;
		member_data[3].dev_id    = NULL;
		member_data[3].type      = NULL;
		member_data[3].value     = NULL;
		member_data[3].delay     = NULL;

		/*4*/
		member_data[4].scen_name = cJSON_CreateString("SCEN_3_ON");
		member_data[4].scen_pic  = NULL;
		member_data[4].district  = NULL;
		member_data[4].ap_id     = NULL;
		member_data[4].dev_id    = NULL;
		member_data[4].type      = NULL;
		member_data[4].value     = NULL;
		member_data[4].delay     = NULL;
	}

	app_scenario_table_mem_t select_data[4] = {
		{data_in_arry,&member_data[0]},
		{data_in_arry,&member_data[1]},
		{data_in_arry,&member_data[2]},
		{data_in_arry,&member_data[4]}
	};

	/*insert*/
	for(i = 0; i < 3; i++)
	{
		ret = app_mem_info[MEM_SCENARIO_TABLE].dFunc->app_mem_search(&select_data[i]);	
		if(ret == -1)
		{
			ret = app_mem_info[MEM_SCENARIO_TABLE].dFunc->app_mem_insert(&select_data[i]);
			if(ret == -1)
				printf("app_mem_insert failed\n");	
			else
				printf("app_mem_insert success\n");
			
			app_scenario_table_mem_print();
		}
	}

	/*update*/
	member_data[2].scen_name = cJSON_CreateString("SCEN_3_ON");
	member_data[2].scen_pic  = cJSON_CreateString("scen_pic_6");
	member_data[2].district  = cJSON_CreateString("district3");
	member_data[2].ap_id     = cJSON_CreateString("66666666");
	member_data[2].dev_id    = cJSON_CreateString("1234567a");
	member_data[2].type      = cJSON_CreateNumber(1008);
	member_data[2].value     = cJSON_CreateNumber(8);
	member_data[2].delay     = cJSON_CreateNumber(8);

	ret = app_mem_info[MEM_SCENARIO_TABLE].dFunc->app_mem_search(&select_data[2]);	
	if(ret == -1)
	{
		ret = app_mem_info[MEM_SCENARIO_TABLE].dFunc->app_mem_insert(&select_data[2]);
		if(ret == -1)
			printf("app_mem_insert failed\n");	
		else
			printf("app_mem_insert success\n");
		
		app_scenario_table_mem_print();
	}
	else
	{
		ret = app_mem_info[MEM_SCENARIO_TABLE].dFunc->app_mem_delete(&select_data[2]);
        if(ret == -1)
        {
        	printf("app_mem_delete failed\n");
        }
        else
        {
        	printf("app_mem_delete success\n");
        	app_scenario_table_mem_print();
        }

		ret = app_mem_info[MEM_SCENARIO_TABLE].dFunc->app_mem_insert(&select_data[2]);
		if(ret == -1)
			printf("app_mem_update failed\n");	
		else
			printf("app_mem_update success\n");
		
		app_scenario_table_mem_print();
	}

	ret = app_mem_info[MEM_SCENARIO_TABLE].dFunc->app_mem_select(&select_data[3]);
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
       		member_data[3].scen_name = cJSON_GetObjectItem(data_obj, "SCEN_NAME");
       		member_data[3].scen_pic  = cJSON_GetObjectItem(data_obj, "SCEN_PIC");
       		member_data[3].district  = cJSON_GetObjectItem(data_obj, "DISTRICT");
       		member_data[3].ap_id     = cJSON_GetObjectItem(data_obj, "AP_ID");
       		member_data[3].dev_id    = cJSON_GetObjectItem(data_obj, "DEV_ID");
       		member_data[3].type      = cJSON_GetObjectItem(data_obj, "TYPE");
       		member_data[3].value     = cJSON_GetObjectItem(data_obj, "VALUE");
       		member_data[3].delay     = cJSON_GetObjectItem(data_obj, "DELAY");
			printf("app_mem_select data[%d]scen_name:%s,scen_pic:%s,district:%s,ap_id:%s,dev_id:%s,type:%d,value:%d,delay:%d\n",\
												                                          i,\
			                                                                              member_data[3].scen_name->valuestring,\
			                                                                              member_data[3].scen_pic->valuestring,\
			                                                                              member_data[3].district->valuestring,\
			                                                                              member_data[3].ap_id->valuestring,\
			                                                                              member_data[3].dev_id->valuestring,\
			                                                                              member_data[3].type->valueint,\
			                                                                              member_data[3].value->valueint,\
			                                                                              member_data[3].delay->valueint);
		}
		
	}

	return ret;
}
#endif

/******************************************scen_alarm_table*********************************************************************/
#if APP_MEM_SCEN_ALARM_TABLE_TEST
extern int app_scen_alarm_table_mem_print();
int app_scen_alarm_table_test(void)
{
	int ret             = 0;
	int i               = 0;
	int num             = 0;
	cJSON *data_in_arry = NULL;
	cJSON *data_obj     = NULL;

	app_scen_alarm_table_tb  member_data[5];

	/*create devData array*/
    data_in_arry = cJSON_CreateArray();
    if(NULL == data_in_arry)
    {
        cJSON_Delete(data_in_arry);
        return -1;
    } 

	{
	    ret = app_mem_info[MEM_SCEN_ALARM_TABLE].dFunc->app_mem_init();
	    if(ret == -1)
	    	printf("app_mem_init failed\n");
	}

	{
		/*0*/
		member_data[0].scen_name = cJSON_CreateString("SCEN_1_ON");
		member_data[0].hour      = cJSON_CreateNumber(10);
		member_data[0].minutes   = cJSON_CreateNumber(20);
		member_data[0].week      = cJSON_CreateString("Sunday");
		member_data[0].status    = cJSON_CreateString("ON");
		/*1*/
		member_data[1].scen_name = cJSON_CreateString("SCEN_2_OFF");
		member_data[1].hour      = cJSON_CreateNumber(12);
		member_data[1].minutes   = cJSON_CreateNumber(22);
		member_data[1].week      = cJSON_CreateString("Monday");
		member_data[1].status    = cJSON_CreateString("OFF");
		/*2*/
		member_data[2].scen_name = cJSON_CreateString("SCEN_3_ON");
		member_data[2].hour      = cJSON_CreateNumber(13);
		member_data[2].minutes   = cJSON_CreateNumber(23);
		member_data[2].week      = cJSON_CreateString("Tuesday");
		member_data[2].status    = cJSON_CreateString("ON");
		/*3*/
		member_data[3].scen_name = NULL;
		member_data[3].hour      = NULL;
		member_data[3].minutes   = NULL;
		member_data[3].week      = NULL;
		member_data[3].status    = NULL;

		/*4*/
		member_data[4].scen_name = cJSON_CreateString("SCEN_3_ON");
		member_data[4].hour      = NULL;
		member_data[4].minutes   = NULL;
		member_data[4].week      = NULL;
		member_data[4].status    = NULL;
	}

	app_scen_alarm_table_mem_t select_data[4] = {
		{data_in_arry,&member_data[0]},
		{data_in_arry,&member_data[1]},
		{data_in_arry,&member_data[2]},
		{data_in_arry,&member_data[4]}
	};

	/*insert*/
	for(i = 0; i < 3; i++)
	{
		ret = app_mem_info[MEM_SCEN_ALARM_TABLE].dFunc->app_mem_search(&select_data[i]);	
		if(ret == -1)
		{
			ret = app_mem_info[MEM_SCEN_ALARM_TABLE].dFunc->app_mem_insert(&select_data[i]);
			if(ret == -1)
				printf("app_mem_insert failed\n");	
			else
				printf("app_mem_insert success\n");
			
			app_scen_alarm_table_mem_print();
		}
	}

	/*update*/
	member_data[2].scen_name = cJSON_CreateString("SCEN_3_ON");
	member_data[2].hour      = cJSON_CreateNumber(66);
	member_data[2].minutes   = cJSON_CreateNumber(88);
	member_data[2].week      = cJSON_CreateString("Allday");
	member_data[2].status    = cJSON_CreateString("OFF");

	ret = app_mem_info[MEM_SCEN_ALARM_TABLE].dFunc->app_mem_search(&select_data[2]);	
	if(ret == -1)
	{
		ret = app_mem_info[MEM_SCEN_ALARM_TABLE].dFunc->app_mem_insert(&select_data[2]);
		if(ret == -1)
			printf("app_mem_insert failed\n");	
		else
			printf("app_mem_insert success\n");
		
		app_scen_alarm_table_mem_print();
	}
	else
	{
		ret = app_mem_info[MEM_SCEN_ALARM_TABLE].dFunc->app_mem_delete(&select_data[2]);
        if(ret == -1)
        {
        	printf("app_mem_delete failed\n");
        }
        else
        {
        	printf("app_mem_delete success\n");
        	app_scen_alarm_table_mem_print();
        }

		ret = app_mem_info[MEM_SCEN_ALARM_TABLE].dFunc->app_mem_insert(&select_data[2]);
		if(ret == -1)
			printf("app_mem_update failed\n");	
		else
			printf("app_mem_update success\n");
		
		app_scen_alarm_table_mem_print();
	}

	ret = app_mem_info[MEM_SCEN_ALARM_TABLE].dFunc->app_mem_select(&select_data[3]);
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
       		member_data[3].scen_name = cJSON_GetObjectItem(data_obj, "SCEN_NAME");
       		member_data[3].hour      = cJSON_GetObjectItem(data_obj, "HOUR");
       		member_data[3].minutes   = cJSON_GetObjectItem(data_obj, "MINUTES");
       		member_data[3].week      = cJSON_GetObjectItem(data_obj, "WEEK");
       		member_data[3].status    = cJSON_GetObjectItem(data_obj, "STATUS");
			printf("app_mem_select data[%d]scen_name:%s,hour%d,minutes:%d,week:%s,status:%s\n",\
												           i,\
			                                               member_data[3].scen_name->valuestring,\
			                                               member_data[3].hour->valueint,\
			                                               member_data[3].minutes->valueint,\
			                                               member_data[3].week->valuestring,\
			                                               member_data[3].status->valuestring);
		}
		
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

	#if APP_MEM_LINKAGE_TABLE_TEST
	app_linkage_table_test();
	#endif

	#if APP_MEM_SCENARIO_TABLE_TEST
	app_scenario_table_test();
	#endif

	#if APP_MEM_SCEN_ALARM_TABLE_TEST
	app_scen_alarm_table_test();
	#endif

	return 0;
}
#endif
