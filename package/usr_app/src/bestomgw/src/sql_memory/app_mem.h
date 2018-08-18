#ifndef APP_MEM_H
#define APP_MEM_H

/*485通信接口号*/
typedef enum{
    MEM_CONN_INFO = 0,
    MEM_PARAM_TABLE,
    MEM_ALL_DEV,
    MEM_LINKAGE_TABLE,
    MEM_SCENARIO_TABLE,
    MEM_SCEN_ALARM_TABLE,
    MEM_MAX
}app_mem_id;

typedef struct {
	int (*app_mem_init)(void);             //内存初始化
	int (*app_mem_search)(void* d);        //搜索
	int (*app_mem_select)(void* d);        //读取
	int (*app_mem_update)(void* d);        //更新
	int (*app_mem_insert)(void* d);        //写入
	int (*app_mem_delete)(void* d);        //删除
	int (*app_mem_destroy)(void);          //退出
}app_mem_func_t;

typedef struct
{    
    int                   id;
    const app_mem_func_t* dFunc;   
    const char*           devName; 
}app_mem_info_t; 

/*conn_info table memory data*/
extern const app_mem_func_t mem_app_conn_info;
/*param_table memory data*/
extern const app_mem_func_t mem_app_param_table;
/*all_dev memory data*/
extern const app_mem_func_t mem_app_all_dev;
/*linkage_table memory data*/
extern const app_mem_func_t mem_app_linkage_table;
/*scenario_table memory data*/
extern const app_mem_func_t mem_app_scenario_table;
/*scen_alarm_table memory data*/
extern const app_mem_func_t mem_app_scen_alarm_table;

#endif//APP_MEM_H



