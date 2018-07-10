#include <curl/curl.h>
#include "sqlite3.h"
#include "m1_protocol.h"
#include "m1_common_log.h"

extern sqlite3* db;
static int m1_update_flag = 0;
const char* m1Version = "V1.10.15";
const char* url = "http://www.dalitek.top:18010/io/file/io/dom100_update.tar.gz";
const char* savefile = "dom100_update.tar.gz";

/*http update*/
static size_t curl_write_func(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  return fwrite(ptr, size, nmemb, stream);
}

static int my_progress_func(char *progress_data,  
                     double t, /* dltotal */  
                     double d, /* dlnow */  
                     double ultotal,  
                     double ulnow)  
{  
  printf("%s %g / %g (%g %%)\n", progress_data, d, t, d*100.0/t);  
  return 0;  
} 

static int curl_download_app(const char *url,const char *savefile,int IsResponse)
{
    CURL *curl;
    CURLcode res;
    FILE *outfile;
    curl = curl_easy_init();
    char *progress_data="#";
    if(curl){
        outfile = fopen(savefile, "wb");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_func);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	  if(IsResponse){
	  	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, my_progress_func);  
	  	curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, progress_data);  
	  }
        res = curl_easy_perform(curl);
        fclose(outfile);
        curl_easy_cleanup(curl);
    }else
		return -1;
	return 0;
}

static void m1_update_flag_set(int d)
{
		m1_update_flag = d;
}

static int m1_update_on(void)
{
	/*校验md5并更新*/
	system("/bin/update.sh");
}

static int m1_update(void)
{
	if(curl_download_app(url, savefile, 0) < 0)
		printf("http request file failed\n");

	m1_update_flag_set(1);
	printf("m1_update begin!");

	return M1_PROTOCOL_OK;
}

int m1_update_check(void)
{
	if(m1_update_flag == 1)
		m1_update_on();
}

static int ap_update(char* devId, cJSON* devData)
{
	int rc             = 0;
	int ret            = M1_PROTOCOL_OK;
	int clientFd       = 0;
	char* sql          = NULL;
	sqlite3_stmt* stmt = NULL;

    sql = "select a.CLIENT_FD from conn_info as a, all_dev as b where a.AP_ID = b.AP_ID and b.DEV_ID = ? limit 1;";
    M1_LOG_DEBUG("sql:%s\n",sql);

    rc = sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }

    sqlite3_bind_text(stmt, 1, devId, -1, NULL);
    rc = sqlite3_step(stmt);   
    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();
    }
    if(rc == SQLITE_ROW)
    {
        clientFd = sqlite3_column_int(stmt, 0);
    }
    else
    {
        M1_LOG_ERROR("m1_del_dev_from_ap failed\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    
    /*发送到AP*/
    char * p = cJSON_PrintUnformatted(devData);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }

    M1_LOG_DEBUG("string:%s\n",p);
    socketSeverSend((uint8_t*)p, strlen(p), clientFd);

    Finish:
    if(stmt)
        sqlite3_finalize(stmt);

    return ret;
}

int m1_ap_update(cJSON* devData)
{
	printf("m1_ap_update\n");
    int clientFd       = 0;
    int ret            = M1_PROTOCOL_OK;
    const char* M1Id   = "DOA100";
    cJSON* pduJson     = NULL;
    cJSON* pduDataJson = NULL;
    cJSON* devIdJson   = NULL;

    if(devData == NULL){
        ret = M1_PROTOCOL_FAILED;
        goto Finish;
    }
    /*获取pdu*/
    pduJson = cJSON_GetObjectItem(devData, "pdu");
    if(NULL == pduJson){
        M1_LOG_ERROR("pdu null\n");
        goto Finish;
    }
    /*获取devData*/
    pduDataJson = cJSON_GetObjectItem(pduJson, "devData");
    if(NULL == pduDataJson){
        M1_LOG_ERROR("devData null”\n");
        goto Finish;
    }
    /*获取AP ID*/
    devIdJson = cJSON_GetObjectItem(pduDataJson,"devId");
    if(NULL == devIdJson){
        M1_LOG_ERROR("apIdJson null”\n");
        goto Finish;
    }
    M1_LOG_DEBUG("apId:%s”\n",devIdJson->valuestring);

    /*设备更新判断*/
    if(strcmp(M1Id, devIdJson->valuestring) == 0)
    {
    	ret = m1_update();
    }
    else
    {
 		ret = ap_update(devIdJson->valuestring, devData);   	
    }

    Finish:
    return ret;
}

/*M1、AP版本读取*/
static int m1_version_read(char* v)
{
	strcpy(v,m1Version);
	return M1_PROTOCOL_OK;
}

static int ap_version_read(char* v, char* devId)
{
	int rc             = 0;
	int ret            = M1_PROTOCOL_OK;
	char* version      = NULL;
	sqlite3_stmt* stmt = NULL;
	char* sql          = NULL;

	sql = "select VERSION from version_table where DEV_ID = ? limit 1;";
	rc = sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    printf("%s\n",sql);
    sqlite3_bind_text(stmt, 1, devId, -1, NULL);
    rc = sqlite3_step(stmt);   
    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();
    }
    if(rc == SQLITE_ROW )
    {
    	version = sqlite3_column_text(stmt, 0);	
    	printf("version:%s\n",version);
    	strcpy(v,version);
    }
    else
    {
    	ret = M1_PROTOCOL_FAILED;
    	printf("ap version does not exsit!\n");
    }

    Finish:
    if(stmt)
    	sqlite3_finalize(stmt);
    return ret;

}

int m1_ap_version_read(payload_t data)
{
	printf("m1_ap_version_read\n");
	int ret               = M1_PROTOCOL_OK;
    int pduType           = TYPE_REPORT_VERSION_INFO;
	char* version         = (char*)malloc(30);
	cJSON* devIdJson      = NULL;
    cJSON * pJsonRoot     = NULL;
    cJSON * pduJsonObject = NULL;
    cJSON * devDataObject = NULL;

	devIdJson = cJSON_GetObjectItem(data.pdu,"devId");
    printf("devId:%s\n",devIdJson->valuestring);

    if(strcmp(devIdJson->valuestring, "DOA100") == 0 )
    {
    	m1_version_read(version);
    }
    else
    {
    	if(ap_version_read(version, devIdJson->valuestring) != M1_PROTOCOL_OK)
    	{
    		printf("version get failed\n");
    		ret = M1_PROTOCOL_FAILED;
        	goto Finish;
    	}
    }

    /*回复版本信息*/
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        M1_LOG_DEBUG("pJsonRoot NULL\n");
        ret = M1_PROTOCOL_FAILED;
        goto Finish;    
    }

    cJSON_AddNumberToObject(pJsonRoot, "sn", data.sn);
    cJSON_AddStringToObject(pJsonRoot, "version", "1.0");
    cJSON_AddNumberToObject(pJsonRoot, "netFlag", 1);
    cJSON_AddNumberToObject(pJsonRoot, "cmdType", 1);
    /*create pdu object*/
    pduJsonObject = cJSON_CreateObject();
    if(NULL == pduJsonObject)
    {
        cJSON_Delete(pduJsonObject);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;    
    }
    /*add pdu to root*/
    cJSON_AddItemToObject(pJsonRoot, "pdu", pduJsonObject);
    /*add pdu type to pdu object*/
    cJSON_AddNumberToObject(pduJsonObject, "pduType", pduType);
    /*devData*/
    devDataObject = cJSON_CreateObject();
    if(NULL == devDataObject)
    {
        cJSON_Delete(devDataObject);
        ret = M1_PROTOCOL_FAILED;
        goto Finish;    
    }
    /*add devData to pdu*/
    cJSON_AddItemToObject(pduJsonObject, "devData", devDataObject);
    cJSON_AddStringToObject(devDataObject, "devId", devIdJson->valuestring);
    cJSON_AddStringToObject(devDataObject, "version", version);

    char * p = cJSON_PrintUnformatted(pJsonRoot);
    
    if(NULL == p)
    {    
        ret = M1_PROTOCOL_FAILED;
        goto Finish;    
    }

    M1_LOG_DEBUG("string:%s\n",p);
    /*response to client*/
    socketSeverSend((uint8*)p, strlen(p), data.clientFd);

    Finish:
    if(pJsonRoot)
    	cJSON_Delete(pJsonRoot);
    free(version);
    return ret;

}

int ap_report_version_handle(payload_t data)
{
	printf("ap_report_version_handle\n");
	int rc               = 0;
	int ret              = M1_PROTOCOL_OK;
	char*  version       = NULL;
	cJSON* devIdJson     = NULL;
	cJSON* verJson       = NULL;
	char* sql            = NULL;
	char* sql_1          = NULL;
	char* sql_2          = NULL;
	sqlite3_stmt* stmt   = NULL;
	sqlite3_stmt* stmt_1 = NULL;
	sqlite3_stmt* stmt_2 = NULL;

	devIdJson = cJSON_GetObjectItem(data.pdu,"devId");
    printf("devId:%s\n",devIdJson->valuestring);
    if(devIdJson == NULL)
    {
    	printf("devIdJson NULL\n");
    	return M1_PROTOCOL_FAILED;
    }
    verJson = cJSON_GetObjectItem(data.pdu,"version");
    printf("version:%s\n",verJson->valuestring);
    if(verJson == NULL)
    {
    	printf("verJson NULL\n");
    	return M1_PROTOCOL_FAILED;
    }
    /*查询该版本是否存在*/
    sql = "select VERSION from version_table where DEV_ID = ? limit 1; ";
    rc = sqlite3_prepare_v2(db, sql, strlen(sql),&stmt, NULL);
    if(rc != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();
        ret = M1_PROTOCOL_FAILED;
        goto Finish; 
    }
    printf("%s\n",sql);
    sqlite3_bind_text(stmt, 1, devIdJson->valuestring, -1, NULL);
    rc = sqlite3_step(stmt);   
    if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    {
        M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
        if(rc == SQLITE_CORRUPT)
            m1_error_handle();
    }
    if(rc == SQLITE_ROW )
    {
    	version = sqlite3_column_text(stmt, 0);	
    	printf("version:%s\n",version);
    	if(strcmp(version, verJson->valuestring) != 0)
    	{
    		/*更新版本*/
    		sql_1 = "update version_table set VERSION = ?,TIME = datetime('now') where DEV_ID = ?;";
    		rc = sqlite3_prepare_v2(db, sql_1, strlen(sql_1),&stmt_1, NULL);
    		if(rc != SQLITE_OK)
    		{
    		    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    		    if(rc == SQLITE_CORRUPT)
    		        m1_error_handle();
    		    ret = M1_PROTOCOL_FAILED;
    		    goto Finish; 
    		}
    		printf("%s\n",sql_1);
    		sqlite3_bind_text(stmt_1, 1, verJson->valuestring, -1, NULL);
    		sqlite3_bind_text(stmt_1, 2, devIdJson->valuestring, -1, NULL);
    		rc = sqlite3_step(stmt_1);   
    		if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    		{
    		    M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
    		    if(rc == SQLITE_CORRUPT)
    		        m1_error_handle();
    		}
    	}
    	
    }
    else
    {
    	/*插入固件版本*/
    	sql_2 = "insert into version_table(VERSION, DEV_ID)values(?,?);";
    	rc = sqlite3_prepare_v2(db, sql_2, strlen(sql_2),&stmt_2, NULL);
    	if(rc != SQLITE_OK)
    	{
    	    M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
    	    if(rc == SQLITE_CORRUPT)
    	        m1_error_handle();
    	    ret = M1_PROTOCOL_FAILED;
    	    goto Finish; 
    	}
    	printf("%s\n",sql_2);
    	sqlite3_bind_text(stmt_2, 1, verJson->valuestring, -1, NULL);
    	sqlite3_bind_text(stmt_2, 2, devIdJson->valuestring, -1, NULL);
    	rc = sqlite3_step(stmt_2);   
    	if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
    	{
    	    M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
    	    if(rc == SQLITE_CORRUPT)
    	        m1_error_handle();
    	}
    }

    Finish:
    if(stmt)
    	sqlite3_finalize(stmt);
    if(stmt_1)
    	sqlite3_finalize(stmt_1);
    if(stmt_2)
    	sqlite3_finalize(stmt_2);

    return ret;
}




