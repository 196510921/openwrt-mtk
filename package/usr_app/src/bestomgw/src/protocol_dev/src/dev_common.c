#include "dev_common.h"
#include "uart_485.h"
#include "m1_common_log.h"
#include "dev_zh.h"
#include "sqlite3.h"

/*获取操作接口*/
static devErr devWrite(devWrite_t data);
static devErr devRead(devRead_t data);
static devErr getdevFunc(UINT16 devId, devInfo* devInfo);

extern const devFunc func_dev_ZH;
comPNode head_485;

/*设备信息表*/
const devInfo mDevInfo[] = 
{
	{U_ttyS2, P_485_ZH, DEV_ID_ZH, &func_dev_ZH, "ZHONGHONG gateway"},
	{U_MAX,   P_MAX,    DEV_ID_MAX, NULL,        "设备信息最大值"}
};

/*设备初始化*/
static devErr devInit(void)
{
	/*初始化485队列*/
    Init_comPQueue(&head_485);
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

	UINT8 testFrame[25] = {0x01,0x50,0x0F,0x02,0x01,0x03,0x01,0x14,0x02,0x01,0x20,0x00,0x00,\
		0x00,0x02,0x02,0x00,0x14,0x04,0x01,0x23,0x00,0x00,0x00,0xDE};
	memcpy(pFrame, testFrame, 25);

	switch(getDevInfo.protocol)
	{
		case P_485_ZH:	
			pUser   = data.userData;
			pusLen  = 25;
			ret = getDevInfo.dFunc->devRead(pUser, pFrame, &pusLen);
			/*串口read*/
			/*read结果转换*/
		break;

		case P_MAX:
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
			M1_LOG_WARN("485 protocol not match !\n");
		break;
	}
	/*串口写入*/

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

	for(i = 0; i < num; i++)
	{
		txData.d.ZHuTxData.dAddr[i].outAddr = (cmd.devAddr[i] >> 8)&0xFF;
		txData.d.ZHuTxData.dAddr[i].inAddr = cmd.devAddr[i] & 0xFF;
		/*广播类型*/
		if(txData.d.ZHuTxData.dAddr[0].outAddr == 0xFF)
		{
			txData.d.ZHuTxData.header.num = 0xFF;
		}

	}

	txData.len = ZH_DEV_HEADER_LEN + num * ZH_DEV_ADDR_LEN;

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

	for(i = 0; i < num; i++)
	{
		rxData.cmd.d.ZHuTxData.dAddr[i].outAddr = (cmd.devAddr[i] >> 8)&0xFF;
		rxData.cmd.d.ZHuTxData.dAddr[i].inAddr = cmd.devAddr[i] & 0xFF;
		/*广播类型*/
		if(rxData.cmd.d.ZHuTxData.dAddr[0].outAddr == 0xFF)
		{
			rxData.cmd.d.ZHuTxData.header.num = 0xFF;
		}

	}

	rxData.cmd.len = ZH_DEV_HEADER_LEN + num * ZH_DEV_ADDR_LEN;

	rxData.userData = userData;

	ret = devRead(rxData);

	/*将获取的状态写入队列*/
	comPush(&head_485, userData);
	
	return ret;
}

devErr app_conditioner_db_handle(void)
{
	ZHuRxData_t* userData = NULL;
	int num               = 0;
	int rc                = 0;
	char dAddr[6]         = {0};
	char* sql_1 = NULL;
	char* sql_1_1 = NULL;
	char* sql_2_1 = NULL;
	char* sql_2_2 = NULL;
	char* sql_3 = NULL;
	sqlite3_stmt* stmt_1   = NULL;
	sqlite3_stmt* stmt_1_1   = NULL;            
	sqlite3_stmt* stmt_2_1   = NULL;
	sqlite3_stmt* stmt_2_2   = NULL;            
	sqlite3_stmt* stmt_3   = NULL;        
	sqlite3* db = NULL;
	char* errorMsg       = NULL;
	int ret = 0;    
	int i = 0;


	if(!comIsEmpty(&head_485))
	{
		if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &errorMsg)!=SQLITE_OK)
	    {
	    	M1_LOG_WARN("BEGIN IMMEDIATE errorMsg:%s",errorMsg);
        	sqlite3_free(errorMsg);
	    	return;
	    }
	}

	sql_1 = "select ID from all_dev where DEV_ID = ?;";
	if(sqlite3_prepare_v2(db, sql_1, strlen(sql_1), &stmt_1, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = DEV_ERROR;
        goto Finish; 
    }
    sql_1_1 = "insert into all_dev(DEV_NAME, DEV_ID, AP_ID, PID, ADDED, NET, STATUS, ACCOUNT)values(?,?,?,?,?,?,?,?);";
	if(sqlite3_prepare_v2(db, sql_1_1, strlen(sql_1_1), &stmt_1_1, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = DEV_ERROR;
        goto Finish; 
    }
	sql_2_1 = "insert into param_table(DEV_ID, DEV_NAME, TYPE, VALUE)values(?,?,?,?);";
	if(sqlite3_prepare_v2(db, sql_2_1, strlen(sql_2_1), &stmt_2_1, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = DEV_ERROR;
        goto Finish; 
    }
	sql_2_2 = "update param_table set VALUE = ?,TIME = datetime('now') where DEV_ID = ? and TYPE = ?;";
	if(sqlite3_prepare_v2(db, sql_2_2, strlen(sql_2_2), &stmt_2_2, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = DEV_ERROR;
        goto Finish; 
    }
	sql_3 = "update param_table set VALUE = ?,TIME = datetime('now') where TYPE = ?;";
	if(sqlite3_prepare_v2(db, sql_3, strlen(sql_3), &stmt_3, NULL) != SQLITE_OK)
    {
        M1_LOG_ERROR( "sqlite3_prepare_v2:error %s\n", sqlite3_errmsg(db));  
        ret = DEV_ERROR;
        goto Finish; 
    }

	while(comPop(&head_485, (UINT8*)userData))
	{
		printf("gwAddr:%d Fn:%d value:%d num:%d \n",\
			userData->header.gwAddr, userData->header.Fn,\
			userData->header.value, userData->header.num);
		
		for(i = 0; i < num; i++)
		{
			printf("dAddr[%d]外机:%x 内机:%x 开关:%x 温度:%x 模式:%x 风速:%x 房间温度:%x 故障代码:%x\n",\
			 i,userData->info[i].outAddr, userData->info[i].inAddr,userData->info[i].status,\
			 userData->info[i].temperature,userData->info[i].mode,userData->info[i].speed,\
			 userData->info[i].roomTemp,userData->info[i].errNum);

			dAddr[0] = (userData->header.gwAddr / 16) + '0';
			dAddr[1] = (userData->header.gwAddr % 16) + '0';
			dAddr[2] = userData->info[i].outAddr / 16 + '0';
			dAddr[3] = userData->info[i].outAddr % 16 + '0';
			dAddr[4] = userData->info[i].inAddr / 16 + '0';
			dAddr[5] = userData->info[i].inAddr % 16 + '0';

			/*查询设备是否存在*/
			{
			    sqlite3_bind_text(stmt_1, 1, dAddr, -1, NULL);

				rc = sqlite3_step(stmt_1);   
		        if((rc != SQLITE_ROW) && (rc != SQLITE_DONE) && (rc != SQLITE_OK))
		        {
		            M1_LOG_ERROR("step() return %s, number:%03d\n", "SQLITE_ERROR",rc);
		            if(rc == SQLITE_CORRUPT)
		                exit(0);
		        }
	        }

			if(userData->header.value == 0x02)
			{
				if(rc != SQLITE_ROW)
				{
					/*插入新设备到all_dev*/
				}
			}
			else
			{
				if(rc == SQLITE_ROW)
				{
					/*更新设备参数到param_table*/
				}
				else
				{
					/*插入新设备参数到param_table*/
				}
	        }
		}
			
	}

	rc = sql_commit(db);
    if(rc == SQLITE_OK)
    {
        M1_LOG_DEBUG("COMMIT OK\n");
    }

    Finish:
	
	return DEV_OK;
}


