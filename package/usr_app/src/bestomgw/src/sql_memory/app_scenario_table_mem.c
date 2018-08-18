#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "app_scenario_table_mem.h"
#include "app_mem.h"

static int mem_app_scenario_table_init(void);
static int mem_app_scenario_table_search(void* d);
static int mem_app_scenario_table_select(void* d);
static int mem_app_scenario_table_update(void* d);
static int mem_app_scenario_table_insert(void* d);
static int mem_app_scenario_table_delete(void* d);
static int mem_app_scenario_table_destroy(void);

static cJSON* scenario_table_array;
pthread_mutex_t mem_app_scenario_table_lock;

const app_mem_func_t mem_app_scenario_table = {
    mem_app_scenario_table_init,
    mem_app_scenario_table_search,
    mem_app_scenario_table_select,
    mem_app_scenario_table_update,
    mem_app_scenario_table_insert,
    mem_app_scenario_table_delete,
    mem_app_scenario_table_destroy
};

static int mem_app_scenario_table_init(void)
{
	pthread_mutex_init(&mem_app_scenario_table_lock, NULL);
	/*build Json stored in memory*/
	scenario_table_array = cJSON_CreateArray();
	if(NULL == scenario_table_array)
    {
        printf("scenario_table_array NULL\n");
        return -1;
    }

    return 0;
}

static int mem_app_scenario_table_search(void* d)
{
	int ret                     = -1;
	int i                       = 0;
	int num                     = 0;
	cJSON* data_obj             = NULL;
    app_scenario_table_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }

    app_scenario_table_mem_t* data = (app_scenario_table_mem_t*)d;

	pthread_mutex_lock(&mem_app_scenario_table_lock);
    
    num = cJSON_GetArraySize(scenario_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj           = cJSON_GetArrayItem(scenario_table_array, i);
        member_data.scen_name = cJSON_GetObjectItem(data_obj, "SCEN_NAME");
        if(strcmp(data->member_data->scen_name->valuestring, member_data.scen_name->valuestring) == 0)
        {
        	ret = 0;
        	break;        
        }
    }

	pthread_mutex_unlock(&mem_app_scenario_table_lock);

	return ret;
}

static int mem_app_scenario_table_select(void* d)
{
	int ret                     = 0;
	int i                       = 0;
	int num                     = 0;
	cJSON* data_obj             = NULL;
    cJSON* data_dup_obj         = NULL;
    app_scenario_table_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }

    app_scenario_table_mem_t* data = (app_scenario_table_mem_t*)d;

    if(data->member_data == NULL || data->data_in_arry == NULL)
    {
        printf("member_data or data_in_array NULL\n");
        return -1;   
    }

	pthread_mutex_lock(&mem_app_scenario_table_lock);
    
    num = cJSON_GetArraySize(scenario_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj              = cJSON_GetArrayItem(scenario_table_array, i);
        member_data.scen_name = cJSON_GetObjectItem(data_obj, "SCEN_NAME");
        member_data.scen_pic  = cJSON_GetObjectItem(data_obj, "SCEN_PIC");
        member_data.district  = cJSON_GetObjectItem(data_obj, "DISTRICT");
        member_data.ap_id     = cJSON_GetObjectItem(data_obj, "AP_ID");
        member_data.dev_id    = cJSON_GetObjectItem(data_obj, "DEV_ID");
        member_data.type      = cJSON_GetObjectItem(data_obj, "TYPE");
        member_data.value     = cJSON_GetObjectItem(data_obj, "VALUE");
        member_data.delay     = cJSON_GetObjectItem(data_obj, "DELAY");

        /*link_name条件判断*/
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

        /*district条件判断*/
        if(data->member_data->district != NULL)
        {
            if(data_dup_obj != NULL)
            {
                cJSON_Delete(data_dup_obj);
            }

            if(strcmp(data->member_data->district->valuestring, member_data.district->valuestring) == 0)
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
                data_dup_obj = NULL;
                continue;
            }
        }

        /*ap_id条件判断*/
         if(data->member_data->ap_id != NULL)
        {
            if(data_dup_obj != NULL)
            {
                cJSON_Delete(data_dup_obj);
            }

            if(strcmp(data->member_data->ap_id->valuestring, member_data.ap_id->valuestring) == 0)
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
                data_dup_obj = NULL;
                continue;
            }
        }

        /*dev_id条件判断*/
         if(data->member_data->dev_id != NULL)
        {
            if(data_dup_obj != NULL)
            {
                cJSON_Delete(data_dup_obj);
            }

            if(strcmp(data->member_data->dev_id->valuestring, member_data.dev_id->valuestring) == 0)
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
                data_dup_obj = NULL;
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
    	printf("mem_scenario_table_dev_select not found\n");
    }

    Finish:

	pthread_mutex_unlock(&mem_app_scenario_table_lock);
	return ret;
}

static int mem_app_scenario_table_update(void* d)
{
	int i                       = 0;
	int num                     = 0;
    int ret                     = 0;
	cJSON* data_obj             = NULL;   
    app_scenario_table_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_scenario_table_mem_t* data     = (app_scenario_table_mem_t*)d;

	pthread_mutex_lock(&mem_app_scenario_table_lock);

	num = cJSON_GetArraySize(scenario_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj              = cJSON_GetArrayItem(scenario_table_array, i);
        member_data.scen_name = cJSON_GetObjectItem(data_obj, "SCEN_NAME");
        member_data.scen_pic  = cJSON_GetObjectItem(data_obj, "SCEN_PIC");
        member_data.district  = cJSON_GetObjectItem(data_obj, "DISTRICT");
        member_data.ap_id     = cJSON_GetObjectItem(data_obj, "AP_ID");
        member_data.dev_id    = cJSON_GetObjectItem(data_obj, "DEV_ID");
        member_data.type      = cJSON_GetObjectItem(data_obj, "TYPE");
        member_data.value     = cJSON_GetObjectItem(data_obj, "VALUE");
        member_data.delay     = cJSON_GetObjectItem(data_obj, "DELAY");

        if(strcmp(data->member_data->scen_name->valuestring, member_data.scen_name->valuestring) == 0)
        {
            /*更新 scen_pic*/
            if((member_data.scen_pic->valuestring != NULL)\
             && (strcmp(data->member_data->scen_pic->valuestring, member_data.scen_pic->valuestring) != 0))
            {
                member_data.scen_pic = cJSON_CreateString(data->member_data->scen_pic->valuestring);
                if(NULL == member_data.scen_pic)
                {
                    // create object faild, exit
                    printf("scen_pic NULL\n");
                    cJSON_Delete(member_data.scen_pic);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"SCEN_PIC", member_data.scen_pic);                
            }

            /*更新 district*/
            if((member_data.district->valuestring != NULL)\
             && (strcmp(data->member_data->district->valuestring, member_data.district->valuestring) != 0))
            {
                member_data.district = cJSON_CreateString(data->member_data->district->valuestring);
                if(NULL == member_data.district)
                {
                    // create object faild, exit
                    printf("district NULL\n");
                    cJSON_Delete(member_data.district);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"DISTRICT", member_data.district);                
            }

        	/*更新 ap_id*/
            if((member_data.ap_id->valuestring != NULL)\
             && (strcmp(data->member_data->ap_id->valuestring, member_data.ap_id->valuestring) != 0))
            {
                member_data.ap_id = cJSON_CreateString(data->member_data->ap_id->valuestring);
                if(NULL == member_data.ap_id)
                {
                    // create object faild, exit
                    printf("ap_id NULL\n");
                    cJSON_Delete(member_data.ap_id);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"AP_ID", member_data.ap_id);                
            }

            /*更新 dev_id*/
            if((member_data.dev_id->valuestring != NULL)\
             && (strcmp(data->member_data->dev_id->valuestring, member_data.dev_id->valuestring) != 0))
            {
                member_data.dev_id = cJSON_CreateString(data->member_data->dev_id->valuestring);
                if(NULL == member_data.dev_id)
                {
                    // create object faild, exit
                    printf("dev_id NULL\n");
                    cJSON_Delete(member_data.dev_id);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"DEV_ID", member_data.dev_id);                
            }

            /*更新 type*/
            if((member_data.type != NULL) && (data->member_data->type->valueint != member_data.type->valueint))
            {
                member_data.type = cJSON_CreateNumber(data->member_data->type->valueint);
                if(NULL == member_data.type)
                {
                    // create object faild, exit
                    printf("type NULL\n");
                    cJSON_Delete(member_data.type);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"TYPE", member_data.type);                
            }

            /*更新 value*/
            if((member_data.value != NULL) && (data->member_data->value->valueint != member_data.value->valueint))
            {
                member_data.value = cJSON_CreateNumber(data->member_data->value->valueint);
                if(NULL == member_data.value)
                {
                    // create object faild, exit
                    printf("value NULL\n");
                    cJSON_Delete(member_data.value);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"VALUE", member_data.value);                
            }

            /*更新delay*/
            if((member_data.delay != NULL) && (data->member_data->delay->valueint != member_data.delay->valueint))
            {
                member_data.delay = cJSON_CreateNumber(data->member_data->delay->valueint);
                if(NULL == member_data.delay)
                {
                    // create object faild, exit
                    printf("delay NULL\n");
                    cJSON_Delete(member_data.delay);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"DELAY", member_data.delay);                
            }

        	break;        
        }
    }

    Finish:
	pthread_mutex_unlock(&mem_app_scenario_table_lock);	
	return ret;
}

static int mem_app_scenario_table_insert(void* d)
{
    int ret         = 0;
	cJSON* data_obj = NULL;	

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_scenario_table_mem_t* data = (app_scenario_table_mem_t*)d;

	pthread_mutex_lock(&mem_app_scenario_table_lock);

	data_obj = cJSON_CreateObject();
    if(NULL == data_obj)
    {
        // create object faild, exit
        printf("data_obj NULL\n");
        cJSON_Delete(data_obj);
        ret = -1;
        goto Finish;
    }
    cJSON_AddItemToArray(scenario_table_array, data_obj);

    cJSON_AddStringToObject(data_obj, "SCEN_NAME", data->member_data->scen_name->valuestring);
    cJSON_AddStringToObject(data_obj, "SCEN_PIC", data->member_data->scen_pic->valuestring);
    cJSON_AddStringToObject(data_obj, "DISTRICT", data->member_data->district->valuestring);
    cJSON_AddStringToObject(data_obj, "AP_ID", data->member_data->ap_id->valuestring);
    cJSON_AddStringToObject(data_obj, "DEV_ID", data->member_data->dev_id->valuestring);
    cJSON_AddNumberToObject(data_obj, "TYPE", data->member_data->type->valueint);
    cJSON_AddNumberToObject(data_obj, "VALUE", data->member_data->value->valueint);
    cJSON_AddNumberToObject(data_obj, "DELAY", data->member_data->delay->valueint);
	
    Finish:
	pthread_mutex_unlock(&mem_app_scenario_table_lock);
	return ret;
}

static int mem_app_scenario_table_delete(void* d)
{
	int i                       = 0;
	int num                     = 0;
	cJSON* data_obj             = NULL;
    app_scenario_table_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_scenario_table_mem_t* data     = (app_scenario_table_mem_t*)d;

	pthread_mutex_lock(&mem_app_scenario_table_lock);

	num = cJSON_GetArraySize(scenario_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj              = cJSON_GetArrayItem(scenario_table_array, i);
        member_data.scen_name = cJSON_GetObjectItem(data_obj, "SCEN_NAME");

        if(strcmp(data->member_data->scen_name->valuestring, member_data.scen_name->valuestring) == 0)
        {
        	cJSON_DeleteItemFromArray(scenario_table_array, i);
        	break;        
        }
    }

	pthread_mutex_unlock(&mem_app_scenario_table_lock);

	return 0;
}

static int mem_app_scenario_table_destroy(void)
{
	cJSON_Delete(scenario_table_array);
	pthread_mutex_destroy(&mem_app_scenario_table_lock);

	return 0;
}

int app_scenario_table_mem_print(void)
{
	char * p = cJSON_PrintUnformatted(scenario_table_array);
    if(NULL == p)
    {    
    	printf("cJSON_PrintUnformatted failed\n");
        return -1;
    }

    printf("string:%s\n",p);

    return 0;
}
