#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "app_conn_info_mem.h"
#include "app_mem.h"

static int mem_app_conn_info_init(void);
static int mem_app_conn_info_search(void* d);
static int mem_app_conn_info_select(void* d);
static int mem_app_conn_info_update(void* d);
static int mem_app_conn_info_insert(void* d);
static int mem_app_conn_info_delete(void* d);
static int mem_app_conn_info_destroy(void);

static cJSON* conn_info_array;
pthread_mutex_t mem_app_conn_info_lock;

const app_mem_func_t mem_app_conn_info = {
    mem_app_conn_info_init,
    mem_app_conn_info_search,
    mem_app_conn_info_select,
    mem_app_conn_info_update,
    mem_app_conn_info_insert,
    mem_app_conn_info_delete,
    mem_app_conn_info_destroy
};

static int mem_app_conn_info_init(void)
{
	pthread_mutex_init(&mem_app_conn_info_lock, NULL);
	/*build Json stored in memory*/
	conn_info_array = cJSON_CreateArray();
	if(NULL == conn_info_array)
    {
        printf("conn_info_array NULL\n");
        return -1;
    }

    return 0;
}

static int mem_app_conn_info_search(void* d)
{
	int ret         = -1;
	int i           = 0;
	int num         = 0;
	cJSON* data_obj = NULL;
	cJSON* ap_id    = NULL;

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_conn_info_mem_t* data = (app_conn_info_mem_t*)d;

	pthread_mutex_lock(&mem_app_conn_info_lock);
    
    num = cJSON_GetArraySize(conn_info_array);
    for(i = 0; i < num; i++)
    {
    	data_obj  = cJSON_GetArrayItem(conn_info_array, i);
        ap_id     = cJSON_GetObjectItem(data_obj, "AP_ID");
        if(strcmp(data->ap_id,ap_id->valuestring) == 0)
        {
        	ret = 0;
        	break;        
        }
    }

	pthread_mutex_unlock(&mem_app_conn_info_lock);

	return ret;
}

static int mem_app_conn_info_select(void* d)
{
	int ret          = 0;
	int i            = 0;
	int num          = 0;
	cJSON* data_obj  = NULL;
	cJSON* ap_id     = NULL;
	cJSON* client_fd = NULL;

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_conn_info_mem_t* data = (app_conn_info_mem_t*)d;

	pthread_mutex_lock(&mem_app_conn_info_lock);
    
    num = cJSON_GetArraySize(conn_info_array);
    for(i = 0; i < num; i++)
    {
    	data_obj  = cJSON_GetArrayItem(conn_info_array, i);
        ap_id     = cJSON_GetObjectItem(data_obj, "AP_ID");
        client_fd = cJSON_GetObjectItem(data_obj, "CLIENT_FD");
        
        if(strcmp(data->ap_id,ap_id->valuestring) == 0)
        {
        	data->client_fd = client_fd->valueint;
        	break;        
        }
    }

    if(data->client_fd == 0)
    {
    	ret = -1;
    	printf("data->client_fd == 0");
    }

	pthread_mutex_unlock(&mem_app_conn_info_lock);

	return ret;
}

static int mem_app_conn_info_update(void* d)
{
	int i                = 0;
	int num              = 0;
    int ret              = 0;
	cJSON* data_obj      = NULL;
	cJSON* client_fd_obj = NULL;
	cJSON* ap_id         = NULL;
	cJSON* client_fd     = NULL;
	
    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_conn_info_mem_t* data = (app_conn_info_mem_t*)d;

	pthread_mutex_lock(&mem_app_conn_info_lock);

	num = cJSON_GetArraySize(conn_info_array);
    for(i = 0; i < num; i++)
    {
    	data_obj  = cJSON_GetArrayItem(conn_info_array, i);
        ap_id     = cJSON_GetObjectItem(data_obj, "AP_ID");
        client_fd = cJSON_GetObjectItem(data_obj, "CLIENT_FD");

        if(strcmp(data->ap_id,ap_id->valuestring) == 0)
        {
        	if(client_fd->valueint == data->client_fd)
        		break;

        	client_fd_obj = cJSON_CreateNumber(data->client_fd);
    		if(NULL == client_fd_obj)
    		{
    		    // create object faild, exit
    		    printf("data_obj NULL\n");
    		    cJSON_Delete(client_fd_obj);
    		    ret = -1;
                goto Finish;
    		}

        	cJSON_ReplaceItemInObject(data_obj,"CLIENT_FD", client_fd_obj);
        	break;        
        }
    }

    Finish:
	pthread_mutex_unlock(&mem_app_conn_info_lock);	
	return ret;
}

static int mem_app_conn_info_insert(void* d)
{
    int ret         = 0;
	cJSON* data_obj = NULL;
	
    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_conn_info_mem_t* data = (app_conn_info_mem_t*)d;		

	pthread_mutex_lock(&mem_app_conn_info_lock);

	data_obj = cJSON_CreateObject();
    if(NULL == data_obj)
    {
        // create object faild, exit
        printf("data_obj NULL\n");
        cJSON_Delete(data_obj);
        ret = -1;
        goto Finish;
    }
    cJSON_AddItemToArray(conn_info_array, data_obj);

    cJSON_AddStringToObject(data_obj, "AP_ID", data->ap_id);
    cJSON_AddNumberToObject(data_obj, "CLIENT_FD", data->client_fd);
	
    Finish:
	pthread_mutex_unlock(&mem_app_conn_info_lock);
	return ret;
}

static int mem_app_conn_info_delete(void* d)
{
	int i           = 0;
	int num         = 0;
	cJSON* data_obj = NULL;
	cJSON* ap_id    = NULL;
	
    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_conn_info_mem_t* data = (app_conn_info_mem_t*)d;			

	pthread_mutex_lock(&mem_app_conn_info_lock);

	num = cJSON_GetArraySize(conn_info_array);
    for(i = 0; i < num; i++)
    {
    	data_obj = cJSON_GetArrayItem(conn_info_array, i);
        ap_id    = cJSON_GetObjectItem(data_obj, "AP_ID");

        if(strcmp(data->ap_id,ap_id->valuestring) == 0)
        {
        	cJSON_DeleteItemFromArray(conn_info_array, i);
        	break;        
        }
    }

	pthread_mutex_unlock(&mem_app_conn_info_lock);

	return 0;
}

static int mem_app_conn_info_destroy(void)
{
	cJSON_Delete(conn_info_array);
	pthread_mutex_destroy(&mem_app_conn_info_lock);

	return 0;
}

int app_conn_info_mem_print(void)
{
	char * p = cJSON_PrintUnformatted(conn_info_array);
    if(NULL == p)
    {    
    	printf("cJSON_PrintUnformatted failed\n");
        return -1;
    }

    printf("string:%s\n",p);

    return 0;
}
