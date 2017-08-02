#include <stdio.h>
#include "poll_socket.h"
#include "sqlite.h"
#include "m1_protocol.h"
#include "thread.h"

extern void* sqlite_test(void* argc);
extern void* server_init(void* argc);
extern char * makeJson(void);

int main(int argc, char* argv[])
{
	char* db_name = "test_db";
	char* tmp = NULL;

	printf("Hello world\n");
	tmp = makeJson();
	printf("Json:%s\n",tmp);
	//thread_init();

	//sqlite_test(NULL);
	//thread_create(sqlite_test);
	
	//thread_create(test_data);
    //thread_create(test_data_1);

    //thread_create(server_init);
	
	//server_init(NULL);	
    printf("wait thread end!\n");
	return 0;
}

