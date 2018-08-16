#include <stdio.h>
#include "app_mem.h"
#include "app_conn_info_mem.h"

#define APP_MEME_UNIT_TEST 1

#define APP_MEM_INFO_NUM 2

/*设备信息表*/
const app_mem_info_t app_mem_info[APP_MEM_INFO_NUM] = 
{
	{MEM_CONN_INFO, &mem_app_conn_info, "conn_info"},
	{MEM_MAX,       NULL,               "数据信息最大值"}
};


#if APP_MEME_UNIT_TEST

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

int main(int argc, char* argv[])
{
	
	app_conn_info_test();

	return 0;
}
#endif
