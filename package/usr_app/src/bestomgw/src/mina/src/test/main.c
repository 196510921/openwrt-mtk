/*
 * main.c
 *
 *  Created on: 2014年2月27日
 *      Author: Admin
 */


#include <stdio.h>
#if ZLOG_ENABLE
#include <zlog.h>
#endif
#include "common.h"
#include "iobuffer.h"
#include "minac.h"
#include "test_acceptor.h"
#include "test_connector.h"

void main(int argc,char *argv[]){
	init_common();
	#if ZLOG_ENABLE
	dzlog_info("这是主函数");
	#endif
	printf("这是主函数\n");

	test_acceptor();
	//test_connector();
	#if ZLOG_ENABLE
	dzlog_info("主函数退出");
	#endif
	printf("主函数退出\n");
	pthread_exit(NULL);
}
