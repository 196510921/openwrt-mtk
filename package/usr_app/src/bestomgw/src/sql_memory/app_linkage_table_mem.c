#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "app_linkage_table_mem.h"
#include "app_mem.h"

static int mem_app_linkage_table_init(void);
static int mem_app_linkage_table_search(void* d);
static int mem_app_linkage_table_select(void* d);
static int mem_app_linkage_table_update(void* d);
static int mem_app_linkage_table_insert(void* d);
static int mem_app_linkage_table_delete(void* d);
static int mem_app_linkage_table_destroy(void);

static cJSON* linkage_table_array;
pthread_mutex_t mem_app_linkage_table_lock;

const app_mem_func_t mem_app_linkage_table = {
    mem_app_linkage_table_init,
    mem_app_linkage_table_search,
    mem_app_linkage_table_select,
    mem_app_linkage_table_update,
    mem_app_linkage_table_insert,
    mem_app_linkage_table_delete,
    mem_app_linkage_table_destroy
};

static int mem_app_linkage_table_init(void)
{
	pthread_mutex_init(&mem_app_linkage_table_lock, NULL);
	/*build Json stored in memory*/
	linkage_table_array = cJSON_CreateArray();
	if(NULL == linkage_table_array)
    {
        printf("linkage_table_array NULL\n");
        return -1;
    }

    return 0;
}

static int mem_app_linkage_table_search(void* d)
{
	int ret                     = -1;
	int i                       = 0;
	int num                     = 0;
	cJSON* data_obj             = NULL;
    app_linkage_table_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }

    app_linkage_table_mem_t* data = (app_linkage_table_mem_t*)d;

	pthread_mutex_lock(&mem_app_linkage_table_lock);
    
    num = cJSON_GetArraySize(linkage_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj           = cJSON_GetArrayItem(linkage_table_array, i);
        member_data.link_name = cJSON_GetObjectItem(data_obj, "LINK_NAME");
        if(strcmp(data->member_data->link_name->valuestring, member_data.link_name->valuestring) == 0)
        {
        	ret = 0;
        	break;        
        }
    }

	pthread_mutex_unlock(&mem_app_linkage_table_lock);

	return ret;
}

static int mem_app_linkage_table_select(void* d)
{
	int ret                     = 0;
	int i                       = 0;
	int num                     = 0;
	cJSON* data_obj             = NULL;
    cJSON* data_dup_obj         = NULL;
    app_linkage_table_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }

    app_linkage_table_mem_t* data = (app_linkage_table_mem_t*)d;

    if(data->member_data == NULL || data->data_in_arry == NULL)
    {
        printf("member_data or data_in_array NULL\n");
        return -1;   
    }

	pthread_mutex_lock(&mem_app_linkage_table_lock);
    
    num = cJSON_GetArraySize(linkage_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj            = cJSON_GetArrayItem(linkage_table_array, i);
        member_data.link_name = cJSON_GetObjectItem(data_obj, "LINK_NAME");
        member_data.district = cJSON_GetObjectItem(data_obj, "DISTRICT");
        member_data.exec_type = cJSON_GetObjectItem(data_obj, "EXEC_TYPE");
        member_data.exec_id = cJSON_GetObjectItem(data_obj, "EXEC_ID");
        member_data.status = cJSON_GetObjectItem(data_obj, "STATUS");
        member_data.enable = cJSON_GetObjectItem(data_obj, "ENABLE");

        /*link_name条件判断*/
        if(data->member_data->link_name != NULL)
        {
            if(strcmp(data->member_data->link_name->valuestring, member_data.link_name->valuestring) == 0)
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

        /*exec_type条件判断*/
         if(data->member_data->exec_type != NULL)
        {
            if(data_dup_obj != NULL)
            {
                cJSON_Delete(data_dup_obj);
            }

            if(strcmp(data->member_data->exec_type->valuestring, member_data.exec_type->valuestring) == 0)
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

        /*enable条件判断*/
         if(data->member_data->enable != NULL)
        {
            if(data_dup_obj != NULL)
            {
                cJSON_Delete(data_dup_obj);
            }

            if(strcmp(data->member_data->enable->valuestring, member_data.enable->valuestring) == 0)
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
    	printf("mem_linkage_table_dev_select not found\n");
    }

    Finish:

	pthread_mutex_unlock(&mem_app_linkage_table_lock);
	return ret;
}

static int mem_app_linkage_table_update(void* d)
{
	int i                       = 0;
	int num                     = 0;
    int ret                     = 0;
	cJSON* data_obj             = NULL;   
    app_linkage_table_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_linkage_table_mem_t* data     = (app_linkage_table_mem_t*)d;

	pthread_mutex_lock(&mem_app_linkage_table_lock);

	num = cJSON_GetArraySize(linkage_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj              = cJSON_GetArrayItem(linkage_table_array, i);
        member_data.link_name = cJSON_GetObjectItem(data_obj, "LINK_NAME");
        member_data.district  = cJSON_GetObjectItem(data_obj, "DISTRICT");
        member_data.exec_type = cJSON_GetObjectItem(data_obj, "EXEC_TYPE");
        member_data.exec_id   = cJSON_GetObjectItem(data_obj, "EXEC_ID");
        member_data.status    = cJSON_GetObjectItem(data_obj, "STATUS");
        member_data.enable    = cJSON_GetObjectItem(data_obj, "ENABLE");

        if(strcmp(data->member_data->link_name->valuestring, member_data.link_name->valuestring) == 0)
        {
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

        	/*更新 exec_type*/
            if((member_data.exec_type->valuestring != NULL)\
             && (strcmp(data->member_data->exec_type->valuestring, member_data.exec_type->valuestring) != 0))
            {
                member_data.exec_type = cJSON_CreateString(data->member_data->exec_type->valuestring);
                if(NULL == member_data.exec_type)
                {
                    // create object faild, exit
                    printf("exec_type NULL\n");
                    cJSON_Delete(member_data.exec_type);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"EXEC_TYPE", member_data.exec_type);                
            }

            /*更新 exec_id*/
            if((member_data.exec_id->valuestring != NULL)\
             && (strcmp(data->member_data->exec_id->valuestring, member_data.exec_id->valuestring) != 0))
            {
                member_data.exec_id = cJSON_CreateString(data->member_data->exec_id->valuestring);
                if(NULL == member_data.exec_id)
                {
                    // create object faild, exit
                    printf("exec_id NULL\n");
                    cJSON_Delete(member_data.exec_id);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"EXEC_ID", member_data.exec_id);                
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

            /*更新 enable*/
            if((member_data.enable->valuestring != NULL)\
             && (strcmp(data->member_data->enable->valuestring, member_data.enable->valuestring) != 0))
            {
                member_data.enable = cJSON_CreateString(data->member_data->enable->valuestring);
                if(NULL == member_data.enable)
                {
                    // create object faild, exit
                    printf("enable NULL\n");
                    cJSON_Delete(member_data.enable);
                    ret = -1;
                    goto Finish;
                }

                cJSON_ReplaceItemInObject(data_obj,"ENABLE", member_data.enable);                
            }

        	break;        
        }
    }

    Finish:
	pthread_mutex_unlock(&mem_app_linkage_table_lock);	
	return ret;
}

static int mem_app_linkage_table_insert(void* d)
{
    int ret         = 0;
	cJSON* data_obj = NULL;	

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_linkage_table_mem_t* data = (app_linkage_table_mem_t*)d;

	pthread_mutex_lock(&mem_app_linkage_table_lock);

	data_obj = cJSON_CreateObject();
    if(NULL == data_obj)
    {
        // create object faild, exit
        printf("data_obj NULL\n");
        cJSON_Delete(data_obj);
        ret = -1;
        goto Finish;
    }
    cJSON_AddItemToArray(linkage_table_array, data_obj);

    cJSON_AddStringToObject(data_obj, "LINK_NAME", data->member_data->link_name->valuestring);
    cJSON_AddStringToObject(data_obj, "DISTRICT", data->member_data->district->valuestring);
    cJSON_AddStringToObject(data_obj, "EXEC_TYPE", data->member_data->exec_type->valuestring);
    cJSON_AddStringToObject(data_obj, "EXEC_ID", data->member_data->exec_id->valuestring);
    cJSON_AddStringToObject(data_obj, "STATUS", data->member_data->status->valuestring);
    cJSON_AddStringToObject(data_obj, "ENABLE", data->member_data->enable->valuestring);
	
    Finish:
	pthread_mutex_unlock(&mem_app_linkage_table_lock);
	return ret;
}

static int mem_app_linkage_table_delete(void* d)
{
	int i                       = 0;
	int num                     = 0;
	cJSON* data_obj             = NULL;
    app_linkage_table_tb member_data  = {0};

    if(d == NULL)
    {
        printf("d NULL\n");
        return -1;
    }
    app_linkage_table_mem_t* data     = (app_linkage_table_mem_t*)d;

	pthread_mutex_lock(&mem_app_linkage_table_lock);

	num = cJSON_GetArraySize(linkage_table_array);
    for(i = 0; i < num; i++)
    {
    	data_obj              = cJSON_GetArrayItem(linkage_table_array, i);
        member_data.link_name = cJSON_GetObjectItem(data_obj, "LINK_NAME");

        if(strcmp(data->member_data->link_name->valuestring, member_data.link_name->valuestring) == 0)
        {
        	cJSON_DeleteItemFromArray(linkage_table_array, i);
        	break;        
        }
    }

	pthread_mutex_unlock(&mem_app_linkage_table_lock);

	return 0;
}

static int mem_app_linkage_table_destroy(void)
{
	cJSON_Delete(linkage_table_array);
	pthread_mutex_destroy(&mem_app_linkage_table_lock);

	return 0;
}

int app_linkage_table_mem_print(void)
{
	char * p = cJSON_PrintUnformatted(linkage_table_array);
    if(NULL == p)
    {    
    	printf("cJSON_PrintUnformatted failed\n");
        return -1;
    }

    printf("string:%s\n",p);

    return 0;
}
