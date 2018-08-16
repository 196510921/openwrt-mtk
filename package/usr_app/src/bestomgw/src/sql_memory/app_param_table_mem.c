#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "app_param_table_mem.h"
#include "app_mem.h"

static int mem_app_param_table_init(void);
static int mem_app_param_table_search(void* d);
static int mem_app_param_table_select(void* d);
static int mem_app_param_table_update(void* d);
static int mem_app_param_table_insert(void* d);
static int mem_app_param_table_delete(void* d);
static int mem_app_param_table_destroy(void);

static cJSON* param_table_array;
pthread_mutex_t mem_app_param_table_lock;

const app_mem_func_t mem_app_param_table = {
    mem_app_param_table_init,
    mem_app_param_table_search,
    mem_app_param_table_select,
    mem_app_param_table_update,
    mem_app_param_table_insert,
    mem_app_param_table_delete,
    mem_app_param_table_destroy
};

static int mem_app_param_table_init(void)
{
	pthread_mutex_init(&mem_app_param_table_lock, NULL);
	/*build Json stored in memory*/
	param_table_array = cJSON_CreateArray();
	if(NULL == param_table_array)
    {
        printf("param_table_array NULL\n");
        return -1;
    }

    return 0;
}

static int mem_app_param_table_search(void* d)
{
	int ret         = -1;
	int i           = 0;
	int num         = 0;
	cJSON* data_obj = NULL;
	cJSON* dev_id   = NULL;
    cJSON* type     = NULL;

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_param_table_mem_t* data = (app_param_table_mem_t*)d;

	pthread_mutex_lock(&mem_app_param_table_lock);
    
    num = cJSON_GetArraySize(param_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj = cJSON_GetArrayItem(param_table_array, i);
        dev_id   = cJSON_GetObjectItem(data_obj, "DEV_ID");
        type     = cJSON_GetObjectItem(data_obj, "TYPE");
        if((strcmp(data->dev_id,dev_id->valuestring) == 0) && (type->valueint == data->type))
        {
        	ret = 0;
        	break;        
        }
    }

	pthread_mutex_unlock(&mem_app_param_table_lock);

	return ret;
}

static int mem_app_param_table_select(void* d)
{
	int ret             = 0;
	int i               = 0;
	int num             = 0;
	cJSON* data_obj     = NULL;
    cJSON* data_dup_obj = NULL;
	cJSON* dev_id       = NULL;

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_param_table_mem_t* data = (app_param_table_mem_t*)d;

	pthread_mutex_lock(&mem_app_param_table_lock);
    
    num = cJSON_GetArraySize(param_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj = cJSON_GetArrayItem(param_table_array, i);
        dev_id   = cJSON_GetObjectItem(data_obj, "DEV_ID");
        printf("select dev_id:%s\n",dev_id->valuestring);
        if(strcmp(data->dev_id,dev_id->valuestring) == 0)
        {
            data_dup_obj = cJSON_Duplicate(data_obj, 1);
            if(NULL == data_dup_obj)
            {
                printf("data_dup_obj NULL\n");
                ret = -1;
                goto Finish;
            }
            printf("select num:%d\n",i);
        	cJSON_AddItemToArray(data->data_in_arry, data_dup_obj);      
        }
    }

    if(num == 0)
    {
    	ret = -1;
    	printf("mem_app_param_table_select not found\n");
    }

    Finish:

	pthread_mutex_unlock(&mem_app_param_table_lock);
	return ret;
}

static int mem_app_param_table_update(void* d)
{
	int i            = 0;
	int num          = 0;
    int ret          = 0;
	cJSON* data_obj  = NULL;
	cJSON* value_obj = NULL;
	cJSON* dev_id    = NULL;
    cJSON* type      = NULL;
    cJSON* value     = NULL;

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_param_table_mem_t* data = (app_param_table_mem_t*)d;

	pthread_mutex_lock(&mem_app_param_table_lock);

	num = cJSON_GetArraySize(param_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj = cJSON_GetArrayItem(param_table_array, i);
        dev_id   = cJSON_GetObjectItem(data_obj, "DEV_ID");
        type     = cJSON_GetObjectItem(data_obj, "TYPE");
        value    = cJSON_GetObjectItem(data_obj, "VALUE");

        if((strcmp(data->dev_id,dev_id->valuestring) == 0) && (data->type == type->valueint))
        {
            {
                /*联动触发查询*/
            }

        	if(value->valueint == data->value)
            {
        		break;
            }

        	value_obj = cJSON_CreateNumber(data->value);
    		if(NULL == value_obj)
    		{
    		    // create object faild, exit
    		    printf("data_obj NULL\n");
    		    cJSON_Delete(value_obj);
    		    ret = -1;
                goto Finish;
    		}

        	cJSON_ReplaceItemInObject(data_obj,"VALUE", value_obj);
        	break;        
        }
    }

    Finish:
	pthread_mutex_unlock(&mem_app_param_table_lock);	
	return ret;
}

static int mem_app_param_table_insert(void* d)
{
    int ret         = 0;
	cJSON* data_obj = NULL;
	
    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_param_table_mem_t* data = (app_param_table_mem_t*)d;		

	pthread_mutex_lock(&mem_app_param_table_lock);

	data_obj = cJSON_CreateObject();
    if(NULL == data_obj)
    {
        // create object faild, exit
        printf("data_obj NULL\n");
        cJSON_Delete(data_obj);
        ret = -1;
        goto Finish;
    }
    cJSON_AddItemToArray(param_table_array, data_obj);

    cJSON_AddStringToObject(data_obj, "DEV_ID", data->dev_id);
    cJSON_AddNumberToObject(data_obj, "TYPE", data->type);
    cJSON_AddNumberToObject(data_obj, "VALUE", data->value);
	

    Finish:
	pthread_mutex_unlock(&mem_app_param_table_lock);
	return ret;
}

static int mem_app_param_table_delete(void* d)
{
	int i            = 0;
	int num          = 0;
	cJSON* data_obj  = NULL;
	cJSON* dev_id    = NULL;
    cJSON* type      = NULL;
	
    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_param_table_mem_t* data = (app_param_table_mem_t*)d;			

	pthread_mutex_lock(&mem_app_param_table_lock);

	num = cJSON_GetArraySize(param_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj = cJSON_GetArrayItem(param_table_array, i);
        dev_id   = cJSON_GetObjectItem(data_obj, "DEV_ID");
        type     = cJSON_GetObjectItem(data_obj, "TYPE");

        if((strcmp(data->dev_id,dev_id->valuestring) == 0) && (type->valueint == data->type))
        {
        	cJSON_DeleteItemFromArray(param_table_array, i);
        	break;        
        }
    }

	pthread_mutex_unlock(&mem_app_param_table_lock);

	return 0;
}

static int mem_app_param_table_destroy(void)
{
	cJSON_Delete(param_table_array);
	pthread_mutex_destroy(&mem_app_param_table_lock);

	return 0;
}

int app_param_table_mem_print(void)
{
	char * p = cJSON_PrintUnformatted(param_table_array);
    if(NULL == p)
    {    
    	printf("cJSON_PrintUnformatted failed\n");
        return -1;
    }

    printf("string:%s\n",p);

    return 0;
}
