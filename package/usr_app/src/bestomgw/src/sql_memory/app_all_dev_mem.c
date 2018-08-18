#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "app_all_dev_mem.h"
#include "app_mem.h"

static int mem_app_all_dev_init(void);
static int mem_app_all_dev_search(void* d);
static int mem_app_all_dev_select(void* d);
static int mem_app_all_dev_update(void* d);
static int mem_app_all_dev_insert(void* d);
static int mem_app_all_dev_delete(void* d);
static int mem_app_all_dev_destroy(void);

static cJSON* all_dev_array;
pthread_mutex_t mem_app_all_dev_lock;

const app_mem_func_t mem_app_all_dev = {
    mem_app_all_dev_init,
    mem_app_all_dev_search,
    mem_app_all_dev_select,
    mem_app_all_dev_update,
    mem_app_all_dev_insert,
    mem_app_all_dev_delete,
    mem_app_all_dev_destroy
};

static int mem_app_all_dev_init(void)
{
	pthread_mutex_init(&mem_app_all_dev_lock, NULL);
	/*build Json stored in memory*/
	all_dev_array = cJSON_CreateArray();
	if(NULL == all_dev_array)
    {
        printf("all_dev_array NULL\n");
        return -1;
    }

    return 0;
}

static int mem_app_all_dev_search(void* d)
{
	int ret                     = -1;
	int i                       = 0;
	int num                     = 0;
	cJSON* data_obj             = NULL;
    app_all_dev_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }

    app_all_dev_mem_t* data     = (app_all_dev_mem_t*)d;

	pthread_mutex_lock(&mem_app_all_dev_lock);
    
    num = cJSON_GetArraySize(all_dev_array);
    for(i = 0; i < num; i++)
    {
    	data_obj            = cJSON_GetArrayItem(all_dev_array, i);
        member_data.dev_id = cJSON_GetObjectItem(data_obj, "DEV_ID");
        if(strcmp(data->member_data->dev_id->valuestring, member_data.dev_id->valuestring) == 0)
        {
        	ret = 0;
        	break;        
        }
    }

	pthread_mutex_unlock(&mem_app_all_dev_lock);

	return ret;
}

static int mem_app_all_dev_select(void* d)
{
	int ret                     = 0;
	int i                       = 0;
	int num                     = 0;
	cJSON* data_obj             = NULL;
    cJSON* data_dup_obj         = NULL;
    app_all_dev_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }

    app_all_dev_mem_t* data = (app_all_dev_mem_t*)d;

    if(data->member_data == NULL || data->data_in_arry == NULL)
    {
        printf("member_data or data_in_array NULL\n");
        return -1;   
    }

	pthread_mutex_lock(&mem_app_all_dev_lock);
    
    num = cJSON_GetArraySize(all_dev_array);
    for(i = 0; i < num; i++)
    {
    	data_obj            = cJSON_GetArrayItem(all_dev_array, i);
        member_data.dev_id = cJSON_GetObjectItem(data_obj, "DEV_ID");
        member_data.ap_id = cJSON_GetObjectItem(data_obj, "AP_ID");
        member_data.dev_name = cJSON_GetObjectItem(data_obj, "DEV_NAME");
        member_data.pid = cJSON_GetObjectItem(data_obj, "PID");
        member_data.added = cJSON_GetObjectItem(data_obj, "ADDED");
        member_data.net = cJSON_GetObjectItem(data_obj, "NET");
        member_data.status = cJSON_GetObjectItem(data_obj, "STATUS");

        /*dev_id条件判断*/
        if(data->member_data->dev_id != NULL)
        {
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

        /*pid条件判断*/
        if(data->member_data->pid != NULL)
        {
            if(data_dup_obj != NULL)
            {
                cJSON_Delete(data_dup_obj);
            }

            if(data->member_data->pid->valueint == member_data.pid->valueint)
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

        /*added条件判断*/
        if(data->member_data->added != NULL)
        {
            if(data_dup_obj != NULL)
            {
                cJSON_Delete(data_dup_obj);
            }

            if(data->member_data->added->valueint == member_data.added->valueint)
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

        /*net条件判断*/
        if(data->member_data->net != NULL)
        {
            if(data_dup_obj != NULL)
            {
                cJSON_Delete(data_dup_obj);
            }

            if(data->member_data->net->valueint == member_data.net->valueint)
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

        /*status条件判断*/
        if(data->member_data->status != NULL)
        {
            if(data_dup_obj != NULL)
            {
                cJSON_Delete(data_dup_obj);
            }

            if(strcmp(data->member_data->status->valuestring, member_data.status->valuestring) == 0)
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
    	printf("mem_app_all_dev_select not found\n");
    }

    Finish:

	pthread_mutex_unlock(&mem_app_all_dev_lock);
	return ret;
}

static int mem_app_all_dev_update(void* d)
{
	int i                       = 0;
	int num                     = 0;
    int ret                     = 0;
	cJSON* data_obj             = NULL;   
    app_all_dev_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_all_dev_mem_t* data     = (app_all_dev_mem_t*)d;

	pthread_mutex_lock(&mem_app_all_dev_lock);

	num = cJSON_GetArraySize(all_dev_array);
    for(i = 0; i < num; i++)
    {
    	data_obj             = cJSON_GetArrayItem(all_dev_array, i);
        member_data.dev_id   = cJSON_GetObjectItem(data_obj, "DEV_ID");
        member_data.dev_name = cJSON_GetObjectItem(data_obj, "DEV_NAME");
        member_data.ap_id    = cJSON_GetObjectItem(data_obj, "AP_ID");
        member_data.pid      = cJSON_GetObjectItem(data_obj, "PID");
        member_data.added    = cJSON_GetObjectItem(data_obj, "ADDED");
        member_data.net      = cJSON_GetObjectItem(data_obj, "NET");
        member_data.status   = cJSON_GetObjectItem(data_obj, "STATUS");

        if(strcmp(data->member_data->dev_id->valuestring, member_data.dev_id->valuestring) == 0)
        {
            /*更新 dev_name*/
            if((member_data.dev_name->valuestring != NULL)\
             && (strcmp(data->member_data->dev_name->valuestring, member_data.dev_name->valuestring) != 0))
            {
                member_data.dev_name = cJSON_CreateString(data->member_data->dev_name->valuestring);
                if(NULL == member_data.dev_name)
                {
                    // create object faild, exit
                    printf("dev_name NULL\n");
                    cJSON_Delete(member_data.dev_name);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"DEV_NAME", member_data.dev_name);                
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

            /*更新 pid*/
            if((member_data.pid != NULL) && (data->member_data->pid->valueint != member_data.ap_id->valueint))
            {
                member_data.pid = cJSON_CreateNumber(data->member_data->pid->valueint);
                if(NULL == member_data.pid)
                {
                    // create object faild, exit
                    printf("pid NULL\n");
                    cJSON_Delete(member_data.pid);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"PID", member_data.pid);                
            }

            /*更新 added*/
            if((member_data.added != NULL) && (data->member_data->added->valueint != member_data.added->valueint))
            {
                member_data.added = cJSON_CreateNumber(data->member_data->added->valueint);
                if(NULL == member_data.added)
                {
                    // create object faild, exit
                    printf("added NULL\n");
                    cJSON_Delete(member_data.added);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"ADDED", member_data.added);                
            }

            /*更新 net*/
            if((member_data.net != NULL) && (data->member_data->net->valueint != member_data.net->valueint))
            {
                member_data.net = cJSON_CreateNumber(data->member_data->net->valueint);
                if(NULL == member_data.net)
                {
                    // create object faild, exit
                    printf("net NULL\n");
                    cJSON_Delete(member_data.net);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"NET", member_data.net);                
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
	pthread_mutex_unlock(&mem_app_all_dev_lock);	
	return ret;
}

static int mem_app_all_dev_insert(void* d)
{
    int ret         = 0;
	cJSON* data_obj = NULL;	

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_all_dev_mem_t* data = (app_all_dev_mem_t*)d;

	pthread_mutex_lock(&mem_app_all_dev_lock);

	data_obj = cJSON_CreateObject();
    if(NULL == data_obj)
    {
        // create object faild, exit
        printf("data_obj NULL\n");
        cJSON_Delete(data_obj);
        ret = -1;
        goto Finish;
    }
    cJSON_AddItemToArray(all_dev_array, data_obj);

    cJSON_AddStringToObject(data_obj, "DEV_ID", data->member_data->dev_id->valuestring);
    cJSON_AddStringToObject(data_obj, "DEV_NAME", data->member_data->dev_name->valuestring);
    cJSON_AddStringToObject(data_obj, "AP_ID", data->member_data->ap_id->valuestring);
    cJSON_AddNumberToObject(data_obj, "PID", data->member_data->pid->valueint);
    cJSON_AddNumberToObject(data_obj, "ADDED", data->member_data->added->valueint);
    cJSON_AddNumberToObject(data_obj, "NET", data->member_data->net->valueint);
    cJSON_AddStringToObject(data_obj, "STATUS", data->member_data->status->valuestring);
	
    Finish:
	pthread_mutex_unlock(&mem_app_all_dev_lock);
	return ret;
}

static int mem_app_all_dev_delete(void* d)
{
	int i                       = 0;
	int num                     = 0;
	cJSON* data_obj             = NULL;
    app_all_dev_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_all_dev_mem_t* data     = (app_all_dev_mem_t*)d;

	pthread_mutex_lock(&mem_app_all_dev_lock);

	num = cJSON_GetArraySize(all_dev_array);
    for(i = 0; i < num; i++)
    {
    	data_obj           = cJSON_GetArrayItem(all_dev_array, i);
        member_data.dev_id = cJSON_GetObjectItem(data_obj, "DEV_ID");

        if(strcmp(data->member_data->dev_id->valuestring, member_data.dev_id->valuestring) == 0)
        {
        	cJSON_DeleteItemFromArray(all_dev_array, i);
        	break;        
        }
    }

	pthread_mutex_unlock(&mem_app_all_dev_lock);

	return 0;
}

static int mem_app_all_dev_destroy(void)
{
	cJSON_Delete(all_dev_array);
	pthread_mutex_destroy(&mem_app_all_dev_lock);

	return 0;
}

int app_all_dev_mem_print(void)
{
	char * p = cJSON_PrintUnformatted(all_dev_array);
    if(NULL == p)
    {    
    	printf("cJSON_PrintUnformatted failed\n");
        return -1;
    }

    printf("string:%s\n",p);

    return 0;
}
