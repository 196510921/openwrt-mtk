#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "app_scen_alarm_table_mem.h"
#include "app_mem.h"

static int mem_app_scen_alarm_table_init(void);
static int mem_app_scen_alarm_table_search(void* d);
static int mem_app_scen_alarm_table_select(void* d);
static int mem_app_scen_alarm_table_update(void* d);
static int mem_app_scen_alarm_table_insert(void* d);
static int mem_app_scen_alarm_table_delete(void* d);
static int mem_app_scen_alarm_table_destroy(void);

static cJSON* scen_alarm_table_array;
pthread_mutex_t mem_app_scen_alarm_table_lock;

const app_mem_func_t mem_app_scen_alarm_table = {
    mem_app_scen_alarm_table_init,
    mem_app_scen_alarm_table_search,
    mem_app_scen_alarm_table_select,
    mem_app_scen_alarm_table_update,
    mem_app_scen_alarm_table_insert,
    mem_app_scen_alarm_table_delete,
    mem_app_scen_alarm_table_destroy
};

static int mem_app_scen_alarm_table_init(void)
{
	pthread_mutex_init(&mem_app_scen_alarm_table_lock, NULL);
	/*build Json stored in memory*/
	scen_alarm_table_array = cJSON_CreateArray();
	if(NULL == scen_alarm_table_array)
    {
        printf("scen_alarm_table_array NULL\n");
        return -1;
    }

    return 0;
}

static int mem_app_scen_alarm_table_search(void* d)
{
	int ret                     = -1;
	int i                       = 0;
	int num                     = 0;
	cJSON* data_obj             = NULL;
    app_scen_alarm_table_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }

    app_scen_alarm_table_mem_t* data = (app_scen_alarm_table_mem_t*)d;

	pthread_mutex_lock(&mem_app_scen_alarm_table_lock);
    
    num = cJSON_GetArraySize(scen_alarm_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj           = cJSON_GetArrayItem(scen_alarm_table_array, i);
        member_data.scen_name = cJSON_GetObjectItem(data_obj, "SCEN_NAME");
        if(strcmp(data->member_data->scen_name->valuestring, member_data.scen_name->valuestring) == 0)
        {
        	ret = 0;
        	break;        
        }
    }

	pthread_mutex_unlock(&mem_app_scen_alarm_table_lock);

	return ret;
}

static int mem_app_scen_alarm_table_select(void* d)
{
	int ret                     = 0;
	int i                       = 0;
	int num                     = 0;
	cJSON* data_obj             = NULL;
    cJSON* data_dup_obj         = NULL;
    app_scen_alarm_table_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }

    app_scen_alarm_table_mem_t* data = (app_scen_alarm_table_mem_t*)d;

    if(data->member_data == NULL || data->data_in_arry == NULL)
    {
        printf("member_data or data_in_array NULL\n");
        return -1;   
    }

	pthread_mutex_lock(&mem_app_scen_alarm_table_lock);
    
    num = cJSON_GetArraySize(scen_alarm_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj              = cJSON_GetArrayItem(scen_alarm_table_array, i);
        member_data.scen_name = cJSON_GetObjectItem(data_obj, "SCEN_NAME");
        member_data.hour      = cJSON_GetObjectItem(data_obj, "HOUR");
        member_data.minutes   = cJSON_GetObjectItem(data_obj, "MINUTES");
        member_data.week      = cJSON_GetObjectItem(data_obj, "WEEK");
        member_data.status    = cJSON_GetObjectItem(data_obj, "STATUS");

        /*scen_name条件判断*/
        if(data->member_data->scen_name != NULL)
        {
            if(strcmp(data->member_data->scen_name->valuestring, member_data.scen_name->valuestring) == 0)
            {
                data_dup_obj = cJSON_Duplicate(data_obj, 1);
                if(NULL == data_dup_obj)
                {
                    printf("data_dup_obj NULL\n");
                    ret = -1;
                    goto Finish;
                }
            }
            else
            {
                continue;
            }

        }

        if(data_dup_obj != NULL)
        {
            cJSON_AddItemToArray(data->data_in_arry, data_dup_obj);          
            data_dup_obj = NULL;   
        }
    }

    if(num == 0)
    {
    	ret = -1;
    	printf("mem_scen_alarm_table_dev_select not found\n");
    }

    Finish:

	pthread_mutex_unlock(&mem_app_scen_alarm_table_lock);
	return ret;
}

static int mem_app_scen_alarm_table_update(void* d)
{
	int i                       = 0;
	int num                     = 0;
    int ret                     = 0;
	cJSON* data_obj             = NULL;   
    app_scen_alarm_table_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_scen_alarm_table_mem_t* data     = (app_scen_alarm_table_mem_t*)d;

	pthread_mutex_lock(&mem_app_scen_alarm_table_lock);

	num = cJSON_GetArraySize(scen_alarm_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj              = cJSON_GetArrayItem(scen_alarm_table_array, i);
        member_data.scen_name = cJSON_GetObjectItem(data_obj, "SCEN_NAME");
        member_data.hour      = cJSON_GetObjectItem(data_obj, "HOUR");
        member_data.minutes   = cJSON_GetObjectItem(data_obj, "MINUTES");
        member_data.week      = cJSON_GetObjectItem(data_obj, "WEEK");
        member_data.status    = cJSON_GetObjectItem(data_obj, "STATUS");

        if(strcmp(data->member_data->scen_name->valuestring, member_data.scen_name->valuestring) == 0)
        {
            /*更新hour*/
            if((member_data.hour != NULL) && (data->member_data->hour->valueint != member_data.hour->valueint))
            {
                member_data.hour = cJSON_CreateNumber(data->member_data->hour->valueint);
                if(NULL == member_data.hour)
                {
                    // create object faild, exit
                    printf("hour NULL\n");
                    cJSON_Delete(member_data.hour);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"HOUR", member_data.hour);                
            }

            /*更新minutes*/
            if((member_data.minutes != NULL) && (data->member_data->minutes->valueint != member_data.minutes->valueint))
            {
                member_data.minutes = cJSON_CreateNumber(data->member_data->minutes->valueint);
                if(NULL == member_data.minutes)
                {
                    // create object faild, exit
                    printf("minutes NULL\n");
                    cJSON_Delete(member_data.minutes);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"MINUTES", member_data.minutes);                
            }

            /*更新 week*/
            if((member_data.week->valuestring != NULL)\
             && (strcmp(data->member_data->week->valuestring, member_data.week->valuestring) != 0))
            {
                member_data.week = cJSON_CreateString(data->member_data->week->valuestring);
                if(NULL == member_data.week)
                {
                    // create object faild, exit
                    printf("week NULL\n");
                    cJSON_Delete(member_data.week);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"WEEK", member_data.week);                
            }

            /*更新 status*/
            if((member_data.status->valuestring != NULL)\
             && (strcmp(data->member_data->status->valuestring, member_data.status->valuestring) != 0))
            {
                member_data.status = cJSON_CreateString(data->member_data->status->valuestring);
                if(NULL == member_data.status)
                {
                    // create object faild, exit
                    printf("status NULL\n");
                    cJSON_Delete(member_data.status);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"STATUS", member_data.status);                
            }

        	break;        
        }
    }

    Finish:
	pthread_mutex_unlock(&mem_app_scen_alarm_table_lock);	
	return ret;
}

static int mem_app_scen_alarm_table_insert(void* d)
{
    int ret         = 0;
	cJSON* data_obj = NULL;	

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_scen_alarm_table_mem_t* data = (app_scen_alarm_table_mem_t*)d;

	pthread_mutex_lock(&mem_app_scen_alarm_table_lock);

	data_obj = cJSON_CreateObject();
    if(NULL == data_obj)
    {
        // create object faild, exit
        printf("data_obj NULL\n");
        cJSON_Delete(data_obj);
        ret = -1;
        goto Finish;
    }
    cJSON_AddItemToArray(scen_alarm_table_array, data_obj);

    cJSON_AddStringToObject(data_obj, "SCEN_NAME", data->member_data->scen_name->valuestring);
    cJSON_AddNumberToObject(data_obj, "HOUR", data->member_data->hour->valueint);
    cJSON_AddNumberToObject(data_obj, "MINUTES", data->member_data->minutes->valueint);
    cJSON_AddStringToObject(data_obj, "WEEK", data->member_data->week->valuestring);
    cJSON_AddStringToObject(data_obj, "STATUS", data->member_data->status->valuestring);
	
    Finish:
	pthread_mutex_unlock(&mem_app_scen_alarm_table_lock);
	return ret;
}

static int mem_app_scen_alarm_table_delete(void* d)
{
	int i                       = 0;
	int num                     = 0;
	cJSON* data_obj             = NULL;
    app_scen_alarm_table_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_scen_alarm_table_mem_t* data     = (app_scen_alarm_table_mem_t*)d;

	pthread_mutex_lock(&mem_app_scen_alarm_table_lock);

	num = cJSON_GetArraySize(scen_alarm_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj              = cJSON_GetArrayItem(scen_alarm_table_array, i);
        member_data.scen_name = cJSON_GetObjectItem(data_obj, "SCEN_NAME");

        if(strcmp(data->member_data->scen_name->valuestring, member_data.scen_name->valuestring) == 0)
        {
        	cJSON_DeleteItemFromArray(scen_alarm_table_array, i);
        	break;        
        }
    }

	pthread_mutex_unlock(&mem_app_scen_alarm_table_lock);

	return 0;
}

static int mem_app_scen_alarm_table_destroy(void)
{
	cJSON_Delete(scen_alarm_table_array);
	pthread_mutex_destroy(&mem_app_scen_alarm_table_lock);

	return 0;
}

int app_scen_alarm_table_mem_print(void)
{
	char * p = cJSON_PrintUnformatted(scen_alarm_table_array);
    if(NULL == p)
    {    
    	printf("cJSON_PrintUnformatted failed\n");
        return -1;
    }

    printf("string:%s\n",p);

    return 0;
}
