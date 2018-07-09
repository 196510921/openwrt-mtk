#include "dev_common.h"
#include "uart_485.h"
#include "dev_zh.h"
#include "sqlite3.h"
#include "m1_protocol.h"
#include "m1_common_log.h"

/*获取操作接口*/
static devErr devWrite(devWrite_t data);
static devErr devRead(devRead_t data);
static devErr getdevFunc(UINT16 devId, devInfo* devInfo);
static void dev_485_db_init(void);

extern const devFunc func_dev_ZH;
extern sqlite3* db;
comPNode head_485;

/*设备信息表*/
const devInfo mDevInfo[] = 
{
	{U_ttyS2, P_485_ZH, DEV_ID_ZH, &func_dev_ZH, "ZHONGHONG gateway"},
	{U_MAX,   P_MAX,    DEV_ID_MAX, NULL,        "设备信息最大值"}
};

/*测试接口*/
static devErr dev_uart_test_write(UINT8* data, UINT16 len)
{
	int i = 0;
	printf("串口写入：");
	for(i = 0; i < len; i++)
		printf("%x ",data[i]);
	printf("\n");
	return DEV_OK;
}

static devErr dev_uart_test_read(UINT8* data, UINT16* len, UINT8 type)
{
	printf("串口读取：\n");
	static uint8 i = 0;

	if(type == 0x02)
	{
		UINT8 testFrame[25] = {0x01,0x50,0x02, 0x02, 0x02,0x01,0x01, 0x03,0x01,0x01, 0x5E};
		memcpy(data, testFrame, 11);
		*len = 11;
		i++;
	}
	else
	{
		UINT8 testFrame[25] = {0x01,0x50,0x0F,0x02,0x01,0x03,0x01,0x14,0x02,0x01,0x20,0x00,0x00,\
		0x00,0x02,0x01,0x00,0x14,0x04,0x01,0x23,0x00,0x00,0x00,0xDD};
		memcpy(data, testFrame, 25);

		*len = 25;	
	}
	
	return DEV_OK;
}

/*设备初始化*/
devErr dev485Init(void)
{
	int ret   = DEV_OK;
	int devId = DEV_ID_ZH; //中弘网关
	devInfo  getDevInfo;

	/*初始化485队列*/
    Init_comPQueue(&head_485);
    getdevFunc(devId, &getDevInfo);
    ret = getDevInfo.dFunc->devInit(NULL,NULL,NULL);
    if(ret == DEV_ERROR)
    	return ret;

    dev_485_db_init();

    return ret;
}
/*设备读取*/
static devErr devRead(devRead_t data)
{
	devErr   ret        = DEV_OK;
	UINT8*   pUser      = NULL;
	UINT8*   pFrame     = (UINT8*)malloc(500);
	UINT16   pusLen     = 0;
	devInfo  getDevInfo;

	ret = devWrite(data.cmd);

	ret = getdevFunc(data.cmd.devId, &getDevInfo);

	dev_uart_test_read(pFrame, &pusLen, data.cmd.d.ZHuTxData.header.value);
	switch(getDevInfo.protocol)
	{
		case P_485_ZH:	
			pUser   = data.userData;
			// pusLen  = 25;
			ret = getDevInfo.dFunc->devRead(pUser, pFrame, &pusLen);
			/*串口read*/
			/*read结果转换*/
		break;
		case P_MAX:
		    ret = DEV_ERROR;
			M1_LOG_WARN("485 protocol P_MAX !\n");
		break;
		default:
			ret = DEV_ERROR;
			M1_LOG_WARN("485 protocol not match !\n");
		break;
	}

	free(pFrame);
	return ret;
}
/*设备写入*/
static devErr devWrite(devWrite_t data)
{
	int      i          = 0;
	devErr   ret        = DEV_OK;
	UINT8*   pUser      = NULL;
	UINT8*   pFrame     = (UINT8*)malloc(500);
	UINT16   pusLen     = 0;
	devInfo  getDevInfo;

	ret = getdevFunc(data.devId, &getDevInfo);

	switch(getDevInfo.protocol)
	{
		case P_485_ZH:
			pUser   = (UINT8*)&data.d.ZHuTxData;
			pusLen = data.len;
			ret = getDevInfo.dFunc->devWrite(pUser,pFrame,&pusLen);
		break;
		case P_MAX:
		    ret = DEV_ERROR;
			M1_LOG_WARN("485 protocol P_MAX !\n");
		break;
		default:
			ret = DEV_ERROR;
			M1_LOG_WARN("485 protocol not match !\n");
		break;
	}
	/*串口写入*/
	dev_uart_test_write(pFrame, pusLen);

	free(pFrame);
	return ret;
}


/*获取操作接口*/
static devErr getdevFunc(UINT16 devId, devInfo* devInfo)
{
	int num = 0;
	int i   = 0;

	num = sizeof(mDevInfo) / sizeof(devInfo);

    for(i = 0; i < num; i++)
    {
        if(mDevInfo[i].Id == devId)
        {
            devInfo->Id       = devId;
            devInfo->uart     = mDevInfo[i].uart;
            devInfo->protocol = mDevInfo[i].protocol;
            devInfo->dFunc    = mDevInfo[i].dFunc;
            devInfo->devName  = mDevInfo[i].devName;
            return DEV_OK;
        }
    }

    return DEV_ERROR;
}

devErr app_conditioner_write(appCmd_t cmd)
{
	int i   = 0;
	int num = 0;
	int ret = DEV_OK;
	devWrite_t txData;

	txData.devId = cmd.gwAddr;
	txData.d.ZHuTxData.header.gwAddr = cmd.gwAddr;
	txData.d.ZHuTxData.header.Fn     = cmd.param;
	txData.d.ZHuTxData.header.value  = cmd.value;
	txData.d.ZHuTxData.header.num    = cmd.dNum;
	num                              = cmd.dNum;

	/*广播类型*/
	if(num == 0xFF)
	{
		num = 1;
		txData.len = ZH_PKG_ONLINE_LEN;
	}
	else
	{
		txData.len = ZH_DEV_HEADER_LEN + num * ZH_DEV_ADDR_LEN;
	}

	for(i = 0; i < num; i++)
	{
		txData.d.ZHuTxData.dAddr[i].outAddr = (cmd.devAddr[i] >> 8)&0xFF;
		txData.d.ZHuTxData.dAddr[i].inAddr = cmd.devAddr[i] & 0xFF;
	}

	printf("gwAddr:%x Fn:%x value:%x num:%x \n",\
					  txData.devId, txData.d.ZHuTxData.header.Fn, \
					  txData.d.ZHuTxData.header.value, txData.d.ZHuTxData.header.num);
	for(i = 0; i < num; i++)
		printf("dAddr[%x]外机:%x 内机:%x \n", \
				      i, txData.d.ZHuTxData.dAddr[i].outAddr, txData.d.ZHuTxData.dAddr[i].inAddr);

	ret = devWrite(txData);
	/*数据库操作*/

	return ret;
}

devErr app_conditioner_read(appCmd_t cmd)
{
	int i           = 0;
	int ret         = DEV_OK;
	int num         = 0;
	UINT8* userData = (UINT8*)malloc(500);
	char* sql       = NULL;

	devRead_t rxData;

	/*ZHONGHONG*/
	rxData.cmd.devId = cmd.gwAddr;
	rxData.cmd.d.ZHuTxData.header.gwAddr = cmd.gwAddr;
	rxData.cmd.d.ZHuTxData.header.Fn     = cmd.param;
	rxData.cmd.d.ZHuTxData.header.value  = cmd.value;
	rxData.cmd.d.ZHuTxData.header.num    = cmd.dNum;
	num                                  = cmd.dNum;

	/*广播类型*/
	if(num == 0xFF)
	{
		num = 1;
		rxData.cmd.len = ZH_PKG_ONLINE_LEN;
	}
	else
	{
		rxData.cmd.len = ZH_DEV_HEADER_LEN + num * ZH_DEV_ADDR_LEN;
	}

	for(i = 0; i < num; i++)
	{
		rxData.cmd.d.ZHuTxData.dAddr[i].outAddr = (cmd.devAddr[i] >> 8)&0xFF;
		rxData.cmd.d.ZHuTxData.dAddr[i].inAddr = cmd.devAddr[i] & 0xFF;
	}

	printf("gwAddr:%x Fn:%x value:%x num:%x \n",\
					  rxData.cmd.d.ZHuTxData.header.gwAddr, rxData.cmd.d.ZHuTxData.header.Fn, \
					  rxData.cmd.d.ZHuTxData.header.value, rxData.cmd.d.ZHuTxData.header.num);
	for(i = 0; i < num; i++)
		printf("dAddr[%x]外机:%x 内机:%x \n", \
				      i, rxData.cmd.d.ZHuTxData.dAddr[i].outAddr, rxData.cmd.d.ZHuTxData.dAddr[i].inAddr);


	rxData.userData = userData;

	ret = devRead(rxData);

	/*将获取的状态写入队列*/
	comPush(&head_485, userData);
	
	return ret;
}

devErr app_conditioner_db_handle(void)
{
	ZHuRxData_t* userData  = NULL;
	UINT8* _userData        = NULL;
	int num                = 0;
	int rc                 = 0;
	char dAddr[7]          = {0};
	char gwId[4]           = {0};
	int pId                = DEV_POWER_EXEC_S10;//S10空调  
	char* sql_1            = NULL;
	char* sql_1_1          = NULL;
	char* sql_2            = NULL;
	char* sql_2_1          = NULL;
	char* sql_2_2          = NULL;
	char* sql_3            = NULL;
	sqlite3_stmt* stmt_1   = NULL;
	sqlite3_stmt* stmt_1_1 = NULL;            
	sqlite3_stmt* stmt_2_1 = NULL;
	sqlite3_stmt* stmt_2   = NULL;
	sqlite3_stmt* stmt_2_2 = NULL;            
	sqlite3_stmt* stmt_3   = NULL;     
	char* errorMsg         = NULL;
	int ret                = DEV_OK;    
	int i                  = 0;
	int j                  = 0;

	if(!comIsEmpty(&head_485))
	{
		printf("app_conditioner_db_handle\n");

		sql_1 = "select ID from all_dev where DEV_ID = ? limit 1;";
		if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
	    {
	        printf( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = DEV_ERROR;
	        goto Finish; 
	    }
	    sql_1_1 = "insert into all_dev(DEV_NAME, DEV_ID, AP_ID, PID, ADDED, NET, STATUS, ACCOUNT)values(?,?,?,?,?,?,?,?);";
		if(sqlite3_prepare_v2(db, sql_1_1, strlen(sql_1_1), &stmt_1_1, NULL) != SQLITE_OK)
	    {
	        printf( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = DEV_ERROR;
	        goto Finish; 
	    }

	    sql_2 = "select ID from param_table where DEV_ID = ? limit 1;";
		if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK)
	    {
	        printf( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = DEV_ERROR;
	        goto Finish; 
	    }
		sql_2_1 = "insert into param_table(DEV_ID, DEV_NAME, TYPE, VALUE)values(?,?,?,?);";
		if(sqlite3_prepare_v2(db, sql_2_1, strlen(sql_2_1), &stmt_2_1, NULL) != SQLITE_OK)
	    {
	        printf( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = DEV_ERROR;
	        goto Finish; 
	    }
		sql_2_2 = "update param_table set VALUE = ?,TIME = datetime('now') where DEV_ID = ? and TYPE = ?;";
		if(sqlite3_prepare_v2(db, sql_2_2, strlen(sql_2_2), &stmt_2_2, NULL) != SQLITE_OK)
	    {
	        printf( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = DEV_ERROR;
	        goto Finish; 
	    }

	    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)!=SQLITE_OK)
	    {
	    	printf("BEGIN IMMEDIATE errorMsg:%s\n",errorMsg);
        	sqlite3_free(errorMsg);
	    	return;
	    }

		while(comPop(&head_485, &_userData))
		{
			userData = (ZHuRxData_t*)_userData;
			if(_userData == NULL)
				printf("userData NULL\n");
			printf("1:%x,%x\n",_userData[0],_userData[1]);
			printf("gwAddr:%x Fn:%x value:%x num:%x \n",\
				userData->header.gwAddr, userData->header.Fn,\
				userData->header.value, userData->header.num);
			
			num = userData->header.num;
			for(i = 0; i < num; i++)
			{
				/*485空调网关地址*/
				gwId[0] = 0x01;
				gwId[1] = (userData->header.gwAddr / 16) + '0';
				gwId[2] = (userData->header.gwAddr % 16) + '0';
				gwId[3] = 0;
				/*485空调地址*/
				dAddr[0] = (userData->header.gwAddr / 16) + '0';
				dAddr[1] = (userData->header.gwAddr % 16) + '0';
				dAddr[2] = userData->info.infoStatus[i].outAddr / 16 + '0';
				dAddr[3] = userData->info.infoStatus[i].outAddr % 16 + '0';
				dAddr[4] = userData->info.infoStatus[i].inAddr / 16 + '0';
				dAddr[5] = userData->info.infoStatus[i].inAddr % 16 + '0';
				dAddr[6] = 0;

				if(userData->header.value == 0x02)  //所有设备的在线状态
				{
					/*查询设备是否存在*/
					{
						printf("%s\n",sql_1);
					    sqlite3_bind_text(stmt_1, 1, dAddr, -1, NULL);

						rc = sqlite3_step(stmt_1);   
				        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
				        {
				            printf("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
				            if(rc == SQLITE_CORRUPT)
				                exit(0);
				        }

				        sqlite3_reset(stmt_1); 
		                sqlite3_clear_bindings(stmt_1);
			        }

					if(rc != SQLITE_ROW)
					{
						printf("%s\n",sql_1_1);
						/*插入新设备到all_dev*/
						sqlite3_bind_text(stmt_1_1, 1, dAddr, -1, NULL);//用设备地址做设备名
						sqlite3_bind_text(stmt_1_1, 2, dAddr, -1, NULL);
						sqlite3_bind_text(stmt_1_1, 3, gwId, -1, NULL);
						sqlite3_bind_int(stmt_1_1, 4, pId);
						sqlite3_bind_int(stmt_1_1, 5, 0);
		                sqlite3_bind_int(stmt_1_1, 6, 1);
		                sqlite3_bind_text(stmt_1_1, 7,"ON", -1, NULL);
		                sqlite3_bind_text(stmt_1_1, 8, "Dalitek", -1, NULL);

						rc = sqlite3_step(stmt_1_1);   
			        	if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
			        	{
			        	    printf("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
			        	    if(rc == SQLITE_CORRUPT)
			        	        exit(0);
			        	}

			        	sqlite3_reset(stmt_1_1); 
	                	sqlite3_clear_bindings(stmt_1_1);
			        }
						
				}
				else //单个设备参数值
				{
					/*查询设备是否存在*/
					{
						printf("%s\n",sql_2);
					    sqlite3_bind_text(stmt_2, 1, dAddr, -1, NULL);

						rc = sqlite3_step(stmt_2);   
				        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
				        {
				            printf("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
				            if(rc == SQLITE_CORRUPT)
				                exit(0);
				        }

				        sqlite3_reset(stmt_2); 
		                sqlite3_clear_bindings(stmt_2);
			        }

					if(rc != SQLITE_ROW)
					{
						printf("%s\n",sql_2_1);
						for(j = 0; j < 4; j++)
						{
							/*插入新设备参数到param_table*/
							sqlite3_bind_text(stmt_2_1, 1, dAddr, -1, NULL);
							sqlite3_bind_text(stmt_2_1, 2, dAddr, -1, NULL);//用485设备地址作为485设备名
							switch(j)
							{
								case 0:
									sqlite3_bind_int(stmt_2_1, 3, 0x200D);//空调开关类型
									sqlite3_bind_int(stmt_2_1, 4, userData->info.infoStatus[i].status);//开关
								break;
								case 1:
									sqlite3_bind_int(stmt_2_1, 3, 0x8023);//空调温度类型
									sqlite3_bind_int(stmt_2_1, 4, userData->info.infoStatus[i].temperature);//温度值
								break;
								case 2:
									sqlite3_bind_int(stmt_2_1, 3, 0x801B);//空调模式类型
									sqlite3_bind_int(stmt_2_1, 4, userData->info.infoStatus[i].mode);//模式
								break;
								case 3:
									sqlite3_bind_int(stmt_2_1, 3, 0x801C);//空调风速类型
									sqlite3_bind_int(stmt_2_1, 4, userData->info.infoStatus[i].speed);//风速值
								break;
								default:
								break;
							}

							rc = sqlite3_step(stmt_2_1);   
					        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
					        {
					            printf("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
					            if(rc == SQLITE_CORRUPT)
					                exit(0);
					        }

					        sqlite3_reset(stmt_2_1); 
			                sqlite3_clear_bindings(stmt_2_1);
		            	}
					}
					else
					{
						printf("%s\n",sql_2_2);
						/*更新设备参数到param_table*/	
						for(j = 0; j < 4; j++)
						{
							/*插入新设备参数到param_table*/
							sqlite3_bind_text(stmt_2_2, 2, dAddr, -1, NULL);//485设备地址
							switch(j)
							{
								case 0:
									sqlite3_bind_int(stmt_2_2, 3, 0x200D);//空调开关类型
									sqlite3_bind_int(stmt_2_2, 1, userData->info.infoStatus[i].status);//开关
								break;
								case 1:
									sqlite3_bind_int(stmt_2_2, 3, 0x8023);//空调温度类型
									sqlite3_bind_int(stmt_2_2, 1, userData->info.infoStatus[i].temperature);//温度值
								break;
								case 2:
									sqlite3_bind_int(stmt_2_2, 3, 0x801B);//空调模式类型
									sqlite3_bind_int(stmt_2_2, 1, userData->info.infoStatus[i].mode);//模式
								break;
								case 3:
									sqlite3_bind_int(stmt_2_2, 3, 0x801C);//空调风速类型
									sqlite3_bind_int(stmt_2_2, 1, userData->info.infoStatus[i].speed);//风速值
								break;
								default:
								break;
							}

							rc = sqlite3_step(stmt_2_2);   
					        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
					        {
					            printf("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
					            if(rc == SQLITE_CORRUPT)
					                exit(0);
					        }

					        sqlite3_reset(stmt_2_2); 
			                sqlite3_clear_bindings(stmt_2_2);
		            	}
					}
		        }
			}
				
		}

		rc = sql_commit(db);
	    if(rc == SQLITE_OK)
	    {
	        printf("COMMIT OK\n");
	    }
	}

    Finish:
    if(stmt_1 != NULL)
        sqlite3_finalize(stmt_1);
    if(stmt_1_1 != NULL)
        sqlite3_finalize(stmt_1_1);
    if(stmt_2 != NULL)
        sqlite3_finalize(stmt_2);
    if(stmt_2_1 != NULL)
        sqlite3_finalize(stmt_2_1);
    if(stmt_2_2 != NULL)
        sqlite3_finalize(stmt_2_2);
	
    if(_userData)
    	free(_userData);

	return ret;
}

void dev_485_thread(void)
{

	while(1)
	{
		app_conditioner_db_handle();
		usleep(10000);
	}

}

static void dev_485_db_init(void)
{
	int j                  = 0;
	int rc                 = 0;
	int ret                = DEV_OK;
	char* gwId             = "01";
	char* dAddr            = "01FFFF";
	int pId                = DEV_POWER_EXEC_S10;
	char* errorMsg         = NULL;
	char* sql_1            = NULL;
	char* sql_1_1          = NULL;
	char* sql_2_1          = NULL;
	sqlite3_stmt* stmt_1   = NULL;
	sqlite3_stmt* stmt_1_1 = NULL;            
	sqlite3_stmt* stmt_2_1 = NULL; 

	rc = sqlite3_open("/bin/dev_info.db", &db);
    if( rc != SQLITE_OK){  
        printf( "Can't open database\n");  
    }else{  
        printf( "Opened database successfully\n");  
    }

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg) != SQLITE_OK)
	{
		printf("BEGIN IMMEDIATE errorMsg:%s",errorMsg);
    	sqlite3_free(errorMsg);
		return;
	}
	/*查询设备是否存在*/
	{
		sql_1 = "select ID from all_dev where DEV_ID = ? limit 1;";
		if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
		{
		    printf( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = DEV_ERROR;
		    goto Finish; 
		}
		printf("%s\n",sql_1);
		sqlite3_bind_text(stmt_1, 1, gwId, -1, NULL);

		rc = sqlite3_step(stmt_1);   
		if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
		{
		    M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
		    if(rc == SQLITE_CORRUPT)
		        exit(0);
		}

		if(rc == SQLITE_ROW)
			goto Finish; 
	}
	/*插入新设备到all_dev*/
	{
		//插入网关地址
		sql_1_1 = "insert into all_dev(DEV_NAME, DEV_ID, AP_ID, PID, ADDED, NET, STATUS, ACCOUNT)values(?,?,?,?,?,?,?,?);";
		if(sqlite3_prepare_v2(db, sql_1_1, strlen(sql_1_1), &stmt_1_1, NULL) != SQLITE_OK)
		{
		    printf( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = DEV_ERROR;
		    goto Finish; 
		}
		printf("%s\n",sql_1_1);				
		sqlite3_bind_text(stmt_1_1, 1, gwId, -1, NULL);//用设备地址做设备名
		sqlite3_bind_text(stmt_1_1, 2, gwId, -1, NULL);
		sqlite3_bind_text(stmt_1_1, 3, gwId, -1, NULL);
		sqlite3_bind_int(stmt_1_1, 4, pId);
		sqlite3_bind_int(stmt_1_1, 5, 0);
		sqlite3_bind_int(stmt_1_1, 6, 1);
		sqlite3_bind_text(stmt_1_1, 7,"ON", -1, NULL);
		sqlite3_bind_text(stmt_1_1, 8, "Dalitek", -1, NULL);

		rc = sqlite3_step(stmt_1_1);   
		if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
		{
		    printf("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
		    if(rc == SQLITE_CORRUPT)
		        exit(0);
		}

		sqlite3_reset(stmt_1_1); 
		sqlite3_clear_bindings(stmt_1_1);

		//插入设备地址
		sql_1_1 = "insert into all_dev(DEV_NAME, DEV_ID, AP_ID, PID, ADDED, NET, STATUS, ACCOUNT)values(?,?,?,?,?,?,?,?);";
		if(sqlite3_prepare_v2(db, sql_1_1, strlen(sql_1_1), &stmt_1_1, NULL) != SQLITE_OK)
		{
		    printf( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = DEV_ERROR;
		    goto Finish; 
		}
		printf("%s\n",sql_1_1);				
		sqlite3_bind_text(stmt_1_1, 1, dAddr, -1, NULL);//用设备地址做设备名
		sqlite3_bind_text(stmt_1_1, 2, dAddr, -1, NULL);
		sqlite3_bind_text(stmt_1_1, 3, gwId, -1, NULL);
		sqlite3_bind_int(stmt_1_1, 4, pId);
		sqlite3_bind_int(stmt_1_1, 5, 0);
		sqlite3_bind_int(stmt_1_1, 6, 1);
		sqlite3_bind_text(stmt_1_1, 7,"ON", -1, NULL);
		sqlite3_bind_text(stmt_1_1, 8, "Dalitek", -1, NULL);

		rc = sqlite3_step(stmt_1_1);   
		if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
		{
		    printf("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
		    if(rc == SQLITE_CORRUPT)
		        exit(0);
		}
	}
	/*插入新参数到param_table*/
	{
		sql_2_1 = "insert into param_table(DEV_ID, DEV_NAME, TYPE, VALUE)values(?,?,?,?);";
		if(sqlite3_prepare_v2(db, sql_2_1, strlen(sql_2_1), &stmt_2_1, NULL) != SQLITE_OK)
		{
		    printf( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = DEV_ERROR;
		    goto Finish; 
		}

		for(j = 0; j < 5; j++)
		{
			/*插入新设备参数到param_table*/
			sqlite3_bind_text(stmt_2_1, 1, dAddr, -1, NULL);
			sqlite3_bind_text(stmt_2_1, 2, dAddr, -1, NULL);//用485设备地址作为485设备名
			switch(j)
			{
				case 0:
					sqlite3_bind_int(stmt_2_1, 3, DEV_ON_OFF);//空调开关类型
					sqlite3_bind_int(stmt_2_1, 4, ON_OFF_OFF);//开关
				break;
				case 1:
					sqlite3_bind_int(stmt_2_1, 3, DEV_CONDITIONER_TEMP);//空调温度类型
					sqlite3_bind_int(stmt_2_1, 4, ZH_TEMP_TH_BOTTOM);//温度值
				break;
				case 2:
					sqlite3_bind_int(stmt_2_1, 3, DEV_CONDITIONER_MODE);//空调模式类型
					sqlite3_bind_int(stmt_2_1, 4, CONDITIONER_MODE_WIND);//模式
				break;
				case 3:
					sqlite3_bind_int(stmt_2_1, 3, DEV_CONDITIONER_SPEED_1);//空调风速类型
					sqlite3_bind_int(stmt_2_1, 4, CONDITIONER_SPEED_LOW);//风速值
				break;
				case 4:
					sqlite3_bind_int(stmt_2_1, 3, DEV_ONLINE);//空调风速类型
					sqlite3_bind_int(stmt_2_1, 4, ONLINE_ON);//风速值
				break;
				default:
				break;
			}

			rc = sqlite3_step(stmt_2_1);   
			if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
			{
			    printf("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
			    if(rc == SQLITE_CORRUPT)
			        exit(0);
			}

			sqlite3_reset(stmt_2_1); 
			sqlite3_clear_bindings(stmt_2_1);
		}
	}

	rc = sql_commit(db);
	if(rc == SQLITE_OK)
	{
	    printf("COMMIT OK\n");
	}

	Finish:
	 if(stmt_1 != NULL)
        sqlite3_finalize(stmt_1);
    if(stmt_1_1 != NULL)
        sqlite3_finalize(stmt_1_1);
    if(stmt_2_1 != NULL)
        sqlite3_finalize(stmt_2_1);

    sqlite3_close(db);
}

static int dev_485_write(dev485WriteParam_t* param)
{
	int ret               = DEV_OK;
	cJSON* paramDataJson  = NULL;
	cJSON* paramArrayJson = NULL;
    cJSON* valueTypeJson  = NULL;
    cJSON* valueJson      = NULL;

	paramDataJson = cJSON_GetObjectItem(param->paramJson, "param");
    if(paramDataJson == NULL)
    {
        printf("paramDataJson NULL \n");
        ret = DEV_ERROR;
        goto Finish;
    }
    paramArrayJson = cJSON_GetArrayItem(paramDataJson, 0);
    if(paramArrayJson == NULL)
    {
        printf("paramArrayJson NULL \n");
        ret = DEV_ERROR;
        goto Finish;
    }
    valueTypeJson = cJSON_GetObjectItem(paramArrayJson, "type");
    if(valueTypeJson == NULL)
    {
        printf("valueTypeJson NULL \n");
        ret = DEV_ERROR;
        goto Finish;
    }
    printf("type%d:%d\n",valueTypeJson->valueint);
    valueJson = cJSON_GetObjectItem(paramArrayJson, "value");
    if(valueJson == NULL)
    {
        printf("valueJson NULL \n");
        ret = DEV_ERROR;
        goto Finish;
    }
    printf("value%d:%d\n",valueJson->valueint);

    switch(valueTypeJson->valueint)
    {
    	case DEV_ON_OFF://开关
    	{
    		printf("命令类型：开关\n");
	    	param->Fn = DEV_ZH_ON_OFF;
	    	if(valueJson->valueint == ON_OFF_ON)
	    		param->value = ZH_ON;
	    	else
	    		param->value = ZH_OFF;
    	}
    	break;
    	case DEV_CONDITIONER_TEMP://温度
    	{
    		printf("命令类型：温度\n");
	    	param->Fn = DEV_ZH_TEMP;
	    	if(valueJson->valueint < ZH_TEMP_TH_BOTTOM)
	    		param->value = ZH_TEMP_TH_BOTTOM;
	    	else if(valueJson->valueint > ZH_TEMP_TH_TOP)
	    		param->value = ZH_TEMP_TH_TOP;
	    	else
	    		param->value = valueJson->valueint;
    	}
    	break;
    	case DEV_CONDITIONER_MODE://模式
		{
			printf("命令类型：模式\n");
			param->Fn = DEV_ZH_MODE;
	    	if(valueJson->valueint == CONDITIONER_MODE_COLD)
	    		param->value = ZH_MODE_COLD;
	    	else if(valueJson->valueint == CONDITIONER_MODE_WARM)
	    		param->value = ZH_MODE_WARM;
	    	else if(valueJson->valueint == CONDITIONER_MODE_AREFACTION)
	    		param->value = ZH_MODE_AREFACTION;
	    	else
	    		param->value = ZH_MODE_WIND;
		}
    	break;
    	case DEV_CONDITIONER_SPEED_1://风速
    	{
    		printf("命令类型：风速\n");
    		param->Fn = DEV_ZH_SPEED;
    		if(valueJson->valueint == CONDITIONER_SPEED_HIGH)
	    		param->value = ZH_SPEED_HIGH;
	    	else if(valueJson->valueint == CONDITIONER_SPEED_MID)
	    		param->value = ZH_SPEED_MID;
	    	else
	    		param->value = ZH_SPEED_LOW;
    	}
    	break;
    	default:
    		printf("dev_485_write not match\n");
    	break;
    }

    Finish:

    return ret;
}

/*读取485空调设备参数状态*/
int dev_485_operate(dev485Opt_t cmd)
{
	M1_LOG_DEBUG("dev_485_operate\n");
	
	int rc             = 0;
	char* sql           = NULL;
	sqlite3_stmt* stmt = NULL;
	int pId            = 0;

	/*数据库查询对应设备类型是否是空调设备*/
	{
		sql = "select PID from all_dev where DEV_ID = ? limit 1;";
		if(sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	    {
	        printf( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        sqlite3_finalize(stmt);
	        return DEV_ERROR; 
	    }
	    printf("%s\n",sql);
	    sqlite3_bind_text(stmt, 1, cmd.devId, -1, NULL);
	    rc = sqlite3_step(stmt);   
		if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
		{
		    printf("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
		    if(rc == SQLITE_CORRUPT)
		        exit(0);
		}
		pId = sqlite3_column_int(stmt, 0);
		/*判断是否是空调类型*/
		if(pId != DEV_POWER_EXEC_S10)
		{
			sqlite3_finalize(stmt);
			return DEV_ERROR;
		}
		printf("PID类型为空调\n");

		sqlite3_finalize(stmt);

		printf("devId:%s",cmd.devId);
	}

	switch(cmd.cmdType)
	{
		case DEV_READ_ONLINE:
			/*查询设备在线状态*/
			printf("查询设备在线状态\n");
			{
				appCmd_t cmd_read;
				cmd_read.param      = DEV_ZH_STATUS;      //向下查询空调状态
				cmd_read.value      = ZH_ALL_ON_OFF;      //查询多台在线状态
				cmd_read.dNum       = 0xFF;               //所有设备
				cmd_read.gwAddr     = cmd.devId[1] - '0'; //网关地址
				cmd_read.devAddr[0] = 0xFFFF;             //所有设备地址
				printf("devId:%x,gwAddr:%x\n",cmd.devId[1], cmd_read.gwAddr);
				app_conditioner_read(cmd_read);
			}
		break;
		case DEV_READ_STATUS:
			/*查询设备参数状态*/
			printf("查询设备参数状态\n");
			{
				appCmd_t cmd_read;
				cmd_read.param      = DEV_ZH_STATUS;      //向下查询空调状态
				cmd_read.value      = ZH_DEV_1_STATUS;    //查询1台空调
				cmd_read.dNum       = 0x01;               //设备数量
				cmd_read.gwAddr     = cmd.devId[1] - '0'; //网关地址
				cmd_read.devAddr[0] = (((UINT16)a2x(cmd.devId[2]) << 12) & 0xF000) | \
				                       (((UINT16)a2x(cmd.devId[3]) << 8) & 0x0F00) |  \
				                       ((a2x(cmd.devId[4]) << 4) & 0x00F0) |          \
				                       a2x(cmd.devId[5]);//设备地址
				printf("gwId:%x,gwAddr:%x,devId:%x%x%x%x,devAddr:%x\n",\
					cmd.devId[1], cmd_read.gwAddr,cmd.devId[2],cmd.devId[3],\
					cmd.devId[4],cmd.devId[5],cmd_read.devAddr[0]);
				app_conditioner_read(cmd_read);
			}
		break;
		case DEV_WRITE:
			printf("设备写入\n");
			{
				appCmd_t cmd_write;
				dev485WriteParam_t param;
				
				param.paramJson = cmd.paramJson;
				dev_485_write(&param);

				cmd_write.param      = param.Fn;           //向下控制开关
				cmd_write.value      = param.value;        //开机
				cmd_write.dNum       = 0x01;               //设备数量
				cmd_write.gwAddr     = cmd.devId[1] - '0'; //网关地址
				cmd_write.devAddr[0] = ((UINT16)a2x(cmd.devId[2]) << 12) & 0xF000 | \
				                       ((UINT16)a2x(cmd.devId[3]) << 8) & 0x0F00 |  \
				                       (a2x(cmd.devId[4]) << 4) & 0x00F0 |         \
				                       a2x(cmd.devId[5]);//设备地址
				printf("gwId:%x,gwAddr:%x,devId:%x%x%x%x,devAddr:%x\n",\
					cmd.devId[1], cmd_write.gwAddr,cmd.devId[2],cmd.devId[3],\
					cmd.devId[4],cmd.devId[5],cmd_write.devAddr[0]);
				app_conditioner_write(cmd_write);

			}
		break;
		case DEV_TYPE_MAX:
			printf("DEV_ID_MAX, cmd out off cmd type range\n");
		break;
		default:
			printf("cmd type not match\n");
		break;
	}
	
	return DEV_OK;
}


/************************************************链表**************************************************/
void Init_comPQueue(comPQueue pQueue)
{
  if (NULL == pQueue)
  	return;
  printf("Init_PQueue\n");
  pQueue->next = NULL;
}

//从堆中申请一个节点的内存空间
static comPNode* comBuy_Node(uint8* item)
{
  comPNode *pTmp = (comPNode*)malloc(sizeof(comPNode));
  pTmp->item = item;
  pTmp->next = NULL;
  return pTmp;
}

//入队
void comPush(comPQueue pQueue, uint8* item)
{
  comPNode *pTmp = comBuy_Node(item);
  comPNode *pPre = pQueue;

  comPNode *pCur = pQueue->next;
  while (NULL != pCur)
  {
    pPre = pCur;
    pCur = pCur->next;
  }
  pPre->next = pTmp;
}

//出队，从队首(front)出
bool comPop(comPQueue pQueue, uint8 **pItem)
{
  if (!comIsEmpty(pQueue))
  {
    comPNode *pTmp = pQueue->next;
    if(pTmp->item == NULL)
    	printf("pTmp->item NULL\n");
    *pItem = pTmp->item;
    pQueue->next = pTmp->next;
    //free(pTmp);
    return true;
  }
  return false;
}

//队列为空则返回true
bool comIsEmpty(comPQueue pQueue)
{
  return pQueue->next == NULL;
}



