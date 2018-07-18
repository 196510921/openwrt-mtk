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
const devInfo mDevInfo[UART_DEV_NUM] = 
{
	{U_ttyUSB0, P_485_ZH, DEV_ID_ZH, &func_dev_ZH, "ZHONGHONG gateway"},
	{U_MAX,     P_MAX,    DEV_ID_MAX, NULL,        "设备信息最大值"}
};
/*串口读写数据*/
static uart_data_t uartData[U_MAX];

/*设备初始化*/
devErr dev485Init(void)
{
	int ret    = DEV_OK;
	int i      = 0;
	char* uart = NULL;

	/*初始化485队列*/
    Init_comPQueue(&head_485);

    while(i < UART_DEV_NUM)
    {
    	M1_LOG_INFO("init uart %d\n", i);
 		switch(mDevInfo[i].uart)
	    {
	    	case U_ttyS2:
	    		uart = "/dev/ttyS2";
	    	break;
	    	case U_ttyUSB0:
	    		uart = "/dev/ttyUSB0";
	    	break;
	    	case U_ttyUSB1:
	    		uart = "/dev/ttyUSB1";
	    	break;
	    	case U_ttyUSB2:
	    		uart = "/dev/ttyUSB2";
	    	break;
	    	case U_MAX:
	    		goto Finish;
	    	break;
	    	default:
	    		M1_LOG_WARN("uart type not match\n");
	    		continue;
	    	break;
	    }

	    // ret = getDevInfo.dFunc->devInit(uart,NULL,NULL);
	    uartData[mDevInfo[i].uart].uartFd = uart_485_init(uart);
	    if(ret == DEV_ERROR)
	    {
	    	M1_LOG_WARN(":%s init error\n",uart);
	    	return;
	    }
    	i++;
    }

    Finish:
    //dev_485_db_init();

    return ret;
}
/*设备读取*/
static devErr devRead(devRead_t data)
{
	devErr   ret        = DEV_OK;
	UINT8*   pUser      = NULL;
	UINT8*   pFrame     = (UINT8*)malloc(512);
	UINT16   pusLen     = 0;
	devInfo  getDevInfo;

	ret = devWrite(data.cmd);

	ret = getdevFunc(data.cmd.devId, &getDevInfo);

	uartData[getDevInfo.uart].d = pFrame;
	pusLen = uart_read(&uartData[getDevInfo.uart]);
	if(pusLen <= 0)
	{
		M1_LOG_WARN("read len:%x\n",pusLen);
		ret = DEV_ERROR;
		goto Finish;
	}
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

	Finish:
	free(pFrame);
	return ret;
}
/*设备写入*/
static devErr devWrite(devWrite_t data)
{
	int      i          = 0;
	devErr   ret        = DEV_OK;
	UINT8*   pUser      = NULL;
	UINT8*   pFrame     = (UINT8*)malloc(512);
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
	uartData[getDevInfo.uart].d = pFrame;
	uartData[getDevInfo.uart].len = pusLen;

	uart_write(uartData[getDevInfo.uart]);

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

devErr app_gw_search(void)
{
	int i           = 0;
	int ret         = DEV_OK;
	int num         = 0;
	UINT8* userData = (UINT8*)malloc(512);
	char* sql       = NULL;


	devRead_t rxData;

	/*ZHONGHONG*/
	rxData.cmd.devId = 0x01;                              //网关地址
	rxData.cmd.d.ZHuTxData.header.gwAddr = 0x01;          
	rxData.cmd.d.ZHuTxData.header.Fn     = DEV_ZH_STATUS; //向下查询空调状态
	rxData.cmd.d.ZHuTxData.header.value  = ZH_ALL_ON_OFF; //查询多台在线状态
	rxData.cmd.d.ZHuTxData.header.num    = 0xFF;          //所有设备
	num                                  = 0xFF;

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
		rxData.cmd.d.ZHuTxData.dAddr[i].outAddr = 0xFF;
		rxData.cmd.d.ZHuTxData.dAddr[i].inAddr = 0xFF;
	}

	M1_LOG_INFO("gwAddr:%x Fn:%x value:%x num:%x \n",\
					  rxData.cmd.d.ZHuTxData.header.gwAddr, rxData.cmd.d.ZHuTxData.header.Fn, \
					  rxData.cmd.d.ZHuTxData.header.value, rxData.cmd.d.ZHuTxData.header.num);
	for(i = 0; i < num; i++)
		M1_LOG_INFO("dAddr[%x]外机:%x 内机:%x \n", \
				      i, rxData.cmd.d.ZHuTxData.dAddr[i].outAddr, rxData.cmd.d.ZHuTxData.dAddr[i].inAddr);


	rxData.userData = userData;

	ret = devRead(rxData);
	if(ret != DEV_OK)
	{
		ret = DEV_ERROR;
		goto Finish;
	}

	dev_485_db_init();
	
	Finish:
	free(userData);
	return ret;
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

	M1_LOG_INFO("gwAddr:%x Fn:%x value:%x num:%x \n",\
					  txData.devId, txData.d.ZHuTxData.header.Fn, \
					  txData.d.ZHuTxData.header.value, txData.d.ZHuTxData.header.num);
	for(i = 0; i < num; i++)
		M1_LOG_INFO("dAddr[%x]外机:%x 内机:%x \n", \
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
	UINT8* userData = (UINT8*)malloc(512);
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

	M1_LOG_INFO("gwAddr:%x Fn:%x value:%x num:%x \n",\
					  rxData.cmd.d.ZHuTxData.header.gwAddr, rxData.cmd.d.ZHuTxData.header.Fn, \
					  rxData.cmd.d.ZHuTxData.header.value, rxData.cmd.d.ZHuTxData.header.num);
	for(i = 0; i < num; i++)
		M1_LOG_INFO("dAddr[%x]外机:%x 内机:%x \n", \
				      i, rxData.cmd.d.ZHuTxData.dAddr[i].outAddr, rxData.cmd.d.ZHuTxData.dAddr[i].inAddr);


	rxData.userData = userData;

	ret = devRead(rxData);
	if(ret != DEV_OK)
	{
		ret = DEV_ERROR;
		goto Finish;
	}

	/*将获取的状态写入队列*/
	comPush(&head_485, userData);
	
	Finish:
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
		M1_LOG_DEBUG("app_conditioner_db_handle\n");

		sql_1 = "select ID from all_dev where DEV_ID = ? limit 1;";
		if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
	    {
	        M1_LOG_WARN( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = DEV_ERROR;
	        goto Finish; 
	    }
	    sql_1_1 = "insert into all_dev(DEV_NAME, DEV_ID, AP_ID, PID, ADDED, NET, STATUS, ACCOUNT)values(?,?,?,?,?,?,?,?);";
		if(sqlite3_prepare_v2(db, sql_1_1, strlen(sql_1_1), &stmt_1_1, NULL) != SQLITE_OK)
	    {
	        M1_LOG_WARN( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = DEV_ERROR;
	        goto Finish; 
	    }

	    sql_2 = "select ID from param_table where DEV_ID = ? limit 1;";
		if(sqlite3_prepare_v2(db, sql_2, strlen(sql_2), &stmt_2, NULL) != SQLITE_OK)
	    {
	        M1_LOG_WARN( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = DEV_ERROR;
	        goto Finish; 
	    }
		sql_2_1 = "insert into param_table(DEV_ID, DEV_NAME, TYPE, VALUE)values(?,?,?,?);";
		if(sqlite3_prepare_v2(db, sql_2_1, strlen(sql_2_1), &stmt_2_1, NULL) != SQLITE_OK)
	    {
	        M1_LOG_WARN( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = DEV_ERROR;
	        goto Finish; 
	    }
		sql_2_2 = "update param_table set VALUE = ?,TIME = datetime('now') where DEV_ID = ? and TYPE = ?;";
		if(sqlite3_prepare_v2(db, sql_2_2, strlen(sql_2_2), &stmt_2_2, NULL) != SQLITE_OK)
	    {
	        M1_LOG_WARN( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        ret = DEV_ERROR;
	        goto Finish; 
	    }

	    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)!=SQLITE_OK)
	    {
	    	M1_LOG_WARN("BEGIN IMMEDIATE errorMsg:%s\n",errorMsg);
        	sqlite3_free(errorMsg);
	    	return;
	    }

		while(comPop(&head_485, &_userData))
		{
			userData = (ZHuRxData_t*)_userData;
			if(_userData == NULL)
				M1_LOG_WARN("userData NULL\n");
			M1_LOG_DEBUG("gwAddr:%x Fn:%x value:%x num:%x \n",\
				userData->header.gwAddr, userData->header.Fn,\
				userData->header.value, userData->header.num);
			
			num = userData->header.num;
			for(i = 0; i < num; i++)
			{
				/*485空调网关地址*/
				// gwId[0] = 0x01;
				// gwId[1] = (userData->header.gwAddr / 16) + '0';
				// gwId[2] = (userData->header.gwAddr % 16) + '0';
				// gwId[3] = 0;
				gwId[0] = x2a(userData->header.gwAddr / 16);
				gwId[1] = x2a(userData->header.gwAddr % 16);
				gwId[2] = 0;
				/*485空调地址*/
				dAddr[0] = x2a(userData->header.gwAddr / 16);
				dAddr[1] = x2a(userData->header.gwAddr % 16);

				if(userData->header.value == 0x02)  //所有设备的在线状态
				{

					dAddr[2] = x2a(userData->info.infoOnline[i].outAddr / 16);
					dAddr[3] = x2a(userData->info.infoOnline[i].outAddr % 16);
					dAddr[4] = x2a(userData->info.infoOnline[i].inAddr / 16);
					dAddr[5] = x2a(userData->info.infoOnline[i].inAddr % 16);
					dAddr[6] = 0;

					/*查询设备是否存在*/
					{
						M1_LOG_DEBUG("%s\n",sql_1);
					    sqlite3_bind_text(stmt_1, 1, dAddr, -1, NULL);

						rc = sqlite3_step(stmt_1);   
				        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
				        {
				            M1_LOG_WARN("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
				            if(rc == SQLITE_CORRUPT)
				                exit(0);
				        }

				        sqlite3_reset(stmt_1); 
		                sqlite3_clear_bindings(stmt_1);
			        }

					if(rc != SQLITE_ROW)
					{
						M1_LOG_WARN("%s\n",sql_1_1);
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
			        	    M1_LOG_WARN("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
			        	    if(rc == SQLITE_CORRUPT)
			        	        exit(0);
			        	}

			        	sqlite3_reset(stmt_1_1); 
	                	sqlite3_clear_bindings(stmt_1_1);
			        }
						
				}
				else //单个设备参数值
				{

					dAddr[2] = x2a(userData->info.infoStatus[i].outAddr / 16);
					dAddr[3] = x2a(userData->info.infoStatus[i].outAddr % 16);
					dAddr[4] = x2a(userData->info.infoStatus[i].inAddr / 16);
					dAddr[5] = x2a(userData->info.infoStatus[i].inAddr % 16);
					dAddr[6] = 0;
					/*查询设备是否存在*/
					{
						M1_LOG_DEBUG("%s\n",sql_2);
					    sqlite3_bind_text(stmt_2, 1, dAddr, -1, NULL);

						rc = sqlite3_step(stmt_2);   
				        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
				        {
				            M1_LOG_WARN("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
				            if(rc == SQLITE_CORRUPT)
				                exit(0);
				        }

				        sqlite3_reset(stmt_2); 
		                sqlite3_clear_bindings(stmt_2);
			        }

					if(rc != SQLITE_ROW)
					{
						M1_LOG_WARN("%s\n",sql_2_1);
						for(j = 0; j < 4; j++)
						{
							/*插入新设备参数到param_table*/
							sqlite3_bind_text(stmt_2_1, 1, dAddr, -1, NULL);
							sqlite3_bind_text(stmt_2_1, 2, dAddr, -1, NULL);//用485设备地址作为485设备名
							switch(j)
							{
								case 0:
									sqlite3_bind_int(stmt_2_1, 3, DEV_ON_OFF);//空调开关类型
									if(userData->info.infoStatus[i].status == ZH_ON)	
										sqlite3_bind_int(stmt_2_1, 4, ON_OFF_ON);//开关
									else
										sqlite3_bind_int(stmt_2_1, 4, ON_OFF_OFF);//开关
								break;
								case 1:
									sqlite3_bind_int(stmt_2_1, 3, DEV_CONDITIONER_TEMP);//空调温度类型
									sqlite3_bind_int(stmt_2_1, 4, userData->info.infoStatus[i].temperature);//温度值
								break;
								case 2:
									sqlite3_bind_int(stmt_2_1, 3, DEV_CONDITIONER_MODE);//空调模式类型
									if(userData->info.infoStatus[i].mode == ZH_MODE_COLD)
										sqlite3_bind_int(stmt_2_1, 4, CONDITIONER_MODE_COLD);//模式
									else if(userData->info.infoStatus[i].mode == ZH_MODE_WARM)
										sqlite3_bind_int(stmt_2_1, 4, CONDITIONER_MODE_WARM);//模式
									else if(userData->info.infoStatus[i].mode == ZH_MODE_WIND)
										sqlite3_bind_int(stmt_2_1, 4, CONDITIONER_MODE_WIND);//模式
									else
										sqlite3_bind_int(stmt_2_1, 4, CONDITIONER_MODE_AREFACTION);//模式
								break;
								case 3:
									sqlite3_bind_int(stmt_2_1, 3, DEV_CONDITIONER_SPEED_1);//空调风速类型
									if(userData->info.infoStatus[i].speed == ZH_SPEED_HIGH)
										sqlite3_bind_int(stmt_2_1, 4, CONDITIONER_SPEED_HIGH);//风速值
									else if(userData->info.infoStatus[i].speed == ZH_SPEED_MID)
										sqlite3_bind_int(stmt_2_1, 4, CONDITIONER_SPEED_MID);//风速值
									else
										sqlite3_bind_int(stmt_2_1, 4, CONDITIONER_SPEED_LOW);//风速值
								break;
								default:
								break;
							}

							rc = sqlite3_step(stmt_2_1);   
					        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
					        {
					            M1_LOG_WARN("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
					            if(rc == SQLITE_CORRUPT)
					                exit(0);
					        }

					        sqlite3_reset(stmt_2_1); 
			                sqlite3_clear_bindings(stmt_2_1);
		            	}
					}
					else
					{
						M1_LOG_DEBUG("%s\n",sql_2_2);
						/*更新设备参数到param_table*/	
						for(j = 0; j < 4; j++)
						{
							/*插入新设备参数到param_table*/
							sqlite3_bind_text(stmt_2_2, 2, dAddr, -1, NULL);//485设备地址
							switch(j)
							{
								case 0:
									sqlite3_bind_int(stmt_2_2, 3, DEV_ON_OFF);//空调开关类型
									if(userData->info.infoStatus[i].status == ZH_ON)	
										sqlite3_bind_int(stmt_2_2, 1, ON_OFF_ON);//开关
									else
										sqlite3_bind_int(stmt_2_2, 1, ON_OFF_OFF);//开关
								break;
								case 1:
									sqlite3_bind_int(stmt_2_2, 3, DEV_CONDITIONER_TEMP);//空调温度类型
									sqlite3_bind_int(stmt_2_2, 1, userData->info.infoStatus[i].temperature);//温度值
								break;
								case 2:
									sqlite3_bind_int(stmt_2_2, 3, DEV_CONDITIONER_MODE);//空调模式类型
									if(userData->info.infoStatus[i].mode == ZH_MODE_COLD)
										sqlite3_bind_int(stmt_2_2, 1, CONDITIONER_MODE_COLD);//模式
									else if(userData->info.infoStatus[i].mode == ZH_MODE_WARM)
										sqlite3_bind_int(stmt_2_2, 1, CONDITIONER_MODE_WARM);//模式
									else if(userData->info.infoStatus[i].mode == ZH_MODE_WIND)
										sqlite3_bind_int(stmt_2_2, 1, CONDITIONER_MODE_WIND);//模式
									else
										sqlite3_bind_int(stmt_2_2, 1, CONDITIONER_MODE_AREFACTION);//模式
								break;
								case 3:
									sqlite3_bind_int(stmt_2_2, 3, DEV_CONDITIONER_SPEED_1);//空调风速类型
									if(userData->info.infoStatus[i].speed == ZH_SPEED_HIGH)
										sqlite3_bind_int(stmt_2_2, 1, CONDITIONER_SPEED_HIGH);//风速值
									else if(userData->info.infoStatus[i].speed == ZH_SPEED_MID)
										sqlite3_bind_int(stmt_2_2, 1, CONDITIONER_SPEED_MID);//风速值
									else
										sqlite3_bind_int(stmt_2_2, 1, CONDITIONER_SPEED_LOW);//风速值
								break;
								default:
								break;
							}

							rc = sqlite3_step(stmt_2_2);   
					        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
					        {
					            M1_LOG_WARN("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
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
	        M1_LOG_DEBUG("COMMIT OK\n");
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

	// rc = sqlite3_open("/bin/dev_info.db", &db);
 //    if( rc != SQLITE_OK){  
 //        printf( "Can't open database\n");  
 //    }else{  
 //        printf( "Opened database successfully\n");  
 //    }

    if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg) != SQLITE_OK)
	{
		M1_LOG_WARN("BEGIN IMMEDIATE errorMsg:%s",errorMsg);
    	sqlite3_free(errorMsg);
		return;
	}
	/*查询设备是否存在*/
	{
		sql_1 = "select ID from all_dev where DEV_ID = ? limit 1;";
		if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
		{
		    M1_LOG_WARN( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = DEV_ERROR;
		    goto Finish; 
		}
		M1_LOG_DEBUG("%s\n",sql_1);
		sqlite3_bind_text(stmt_1, 1, gwId, -1, NULL);

		rc = sqlite3_step(stmt_1);   
		if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
		{
		    M1_LOG_WARN("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
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
		    M1_LOG_WARN( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = DEV_ERROR;
		    goto Finish; 
		}
		M1_LOG_DEBUG("%s\n",sql_1_1);				
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
		    M1_LOG_WARN("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
		    if(rc == SQLITE_CORRUPT)
		        exit(0);
		}

		sqlite3_reset(stmt_1_1); 
		sqlite3_clear_bindings(stmt_1_1);

		//插入设备地址
		sql_1_1 = "insert into all_dev(DEV_NAME, DEV_ID, AP_ID, PID, ADDED, NET, STATUS, ACCOUNT)values(?,?,?,?,?,?,?,?);";
		if(sqlite3_prepare_v2(db, sql_1_1, strlen(sql_1_1), &stmt_1_1, NULL) != SQLITE_OK)
		{
		    M1_LOG_WARN( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
		    ret = DEV_ERROR;
		    goto Finish; 
		}
		M1_LOG_DEBUG("%s\n",sql_1_1);				
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
		    M1_LOG_WARN("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
		    if(rc == SQLITE_CORRUPT)
		        exit(0);
		}
	}
	/*插入新参数到param_table*/
	{
		sql_2_1 = "insert into param_table(DEV_ID, DEV_NAME, TYPE, VALUE)values(?,?,?,?);";
		if(sqlite3_prepare_v2(db, sql_2_1, strlen(sql_2_1), &stmt_2_1, NULL) != SQLITE_OK)
		{
		    M1_LOG_WARN( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
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
			    M1_LOG_WARN("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
			    if(rc == SQLITE_CORRUPT)
			        exit(0);
			}

			sqlite3_reset(stmt_2_1); 
			sqlite3_clear_bindings(stmt_2_1);
		}
	}

	Finish:

	rc = sql_commit(db);
	if(rc == SQLITE_OK)
	{
	    M1_LOG_DEBUG("COMMIT OK\n");
	}

	if(stmt_1 != NULL)
        sqlite3_finalize(stmt_1);
    if(stmt_1_1 != NULL)
        sqlite3_finalize(stmt_1_1);
    if(stmt_2_1 != NULL)
        sqlite3_finalize(stmt_2_1);

    // sqlite3_close(db);
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
        M1_LOG_WARN("paramDataJson NULL \n");
        ret = DEV_ERROR;
        goto Finish;
    }
    paramArrayJson = cJSON_GetArrayItem(paramDataJson, 0);
    if(paramArrayJson == NULL)
    {
        M1_LOG_WARN("paramArrayJson NULL \n");
        ret = DEV_ERROR;
        goto Finish;
    }
    valueTypeJson = cJSON_GetObjectItem(paramArrayJson, "type");
    if(valueTypeJson == NULL)
    {
        M1_LOG_WARN("valueTypeJson NULL \n");
        ret = DEV_ERROR;
        goto Finish;
    }
    M1_LOG_DEBUG("type%d:%d\n",valueTypeJson->valueint);
    valueJson = cJSON_GetObjectItem(paramArrayJson, "value");
    if(valueJson == NULL)
    {
        M1_LOG_WARN("valueJson NULL \n");
        ret = DEV_ERROR;
        goto Finish;
    }
    M1_LOG_DEBUG("value%d:%d\n",valueJson->valueint);

    switch(valueTypeJson->valueint)
    {
    	case DEV_ON_OFF://开关
    	{
    		M1_LOG_INFO("命令类型：开关\n");
	    	param->Fn = DEV_ZH_ON_OFF;
	    	if(valueJson->valueint == ON_OFF_ON)
	    		param->value = ZH_ON;
	    	else
	    		param->value = ZH_OFF;
    	}
    	break;
    	case DEV_CONDITIONER_TEMP://温度
    	{
    		M1_LOG_INFO("命令类型：温度\n");
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
			M1_LOG_INFO("命令类型：模式\n");
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
    		M1_LOG_INFO("命令类型：风速\n");
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
    		M1_LOG_WARN("dev_485_write not match\n");
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
	        M1_LOG_WARN( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
	        sqlite3_finalize(stmt);
	        return DEV_ERROR; 
	    }
	    M1_LOG_DEBUG("%s\n",sql);
	    sqlite3_bind_text(stmt, 1, cmd.devId, -1, NULL);
	    rc = sqlite3_step(stmt);   
		if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
		{
		    M1_LOG_WARN("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
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
		M1_LOG_INFO("PID类型为空调\n");

		sqlite3_finalize(stmt);

		M1_LOG_INFO("devId:%s",cmd.devId);
	}

	switch(cmd.cmdType)
	{
		case DEV_READ_ONLINE:
			/*查询设备在线状态*/
			M1_LOG_INFO("查询设备在线状态\n");
			{
				appCmd_t cmd_read;
				cmd_read.param      = DEV_ZH_STATUS;      //向下查询空调状态
				cmd_read.value      = ZH_ALL_ON_OFF;      //查询多台在线状态
				cmd_read.dNum       = 0xFF;               //所有设备
				cmd_read.gwAddr     = cmd.devId[1] - '0'; //网关地址
				cmd_read.devAddr[0] = 0xFFFF;             //所有设备地址
				M1_LOG_INFO("devId:%x,gwAddr:%x\n",cmd.devId[1], cmd_read.gwAddr);
				app_conditioner_read(cmd_read);
			}
		break;
		case DEV_READ_STATUS:
			/*查询设备参数状态*/
			M1_LOG_INFO("查询设备参数状态\n");
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
				M1_LOG_DEBUG("gwId:%x,gwAddr:%x,devId:%x%x%x%x,devAddr:%x\n",\
					cmd.devId[1], cmd_read.gwAddr,cmd.devId[2],cmd.devId[3],\
					cmd.devId[4],cmd.devId[5],cmd_read.devAddr[0]);
				app_conditioner_read(cmd_read);
			}
		break;
		case DEV_WRITE:
			M1_LOG_INFO("设备写入\n");
			{
				appCmd_t cmd_write;
				dev485WriteParam_t param;
				
				param.paramJson = cmd.paramJson;
				dev_485_write(&param);

				cmd_write.param      = param.Fn;           //向下控制开关
				cmd_write.value      = param.value;        //开机
				cmd_write.gwAddr     = cmd.devId[1] - '0'; //网关地址
				cmd_write.devAddr[0] = ((UINT16)a2x(cmd.devId[2]) << 12) & 0xF000 | \
				                       ((UINT16)a2x(cmd.devId[3]) << 8) & 0x0F00 |  \
				                       (a2x(cmd.devId[4]) << 4) & 0x00F0 |         \
				                       a2x(cmd.devId[5]);//设备地址
				if(cmd_write.devAddr[0] == 0xffff)
				{
					cmd_write.dNum       = 0xff;               //设备数量
				}
				else
				{
					cmd_write.dNum       = 0x01;               //设备数量
				}
				M1_LOG_DEBUG("gwId:%x,gwAddr:%x,devId:%x%x%x%x,devAddr:%x\n",\
					cmd.devId[1], cmd_write.gwAddr,cmd.devId[2],cmd.devId[3],\
					cmd.devId[4],cmd.devId[5],cmd_write.devAddr[0]);
				app_conditioner_write(cmd_write);

			}
		break;
		case DEV_TYPE_MAX:
			M1_LOG_INFO("DEV_ID_MAX, cmd out off cmd type range\n");
		break;
		default:
			M1_LOG_INFO("cmd type not match\n");
		break;
	}
	
	return DEV_OK;
}


/************************************************链表**************************************************/
void Init_comPQueue(comPQueue pQueue)
{
  if (NULL == pQueue)
  	return;
  M1_LOG_DEBUG("Init_PQueue\n");
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
    	M1_LOG_DEBUG("pTmp->item NULL\n");
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



