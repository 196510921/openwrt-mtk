#include <stdio.h>
#include "poll_socket.h"
#include "sqlite.h"
#include "m1_protocol.h"
#include "thread.h"

extern void* sqlite_test(void* argc);
extern void* server_init(void* argc);
extern void* test_data(void* argc);
extern void* test_data_1(void* argc);

int main(int argc, char* argv[])
{
	char* db_name = "test_db";
	
	printf("Hello world\n");
	thread_init();

	thread_create(sqlite_test);
	
	thread_create(test_data);
    thread_create(test_data_1);
    
    thread_create(server_init);
	
	thread_wait();
    printf("wait thread end!\n");
	return 0;
}

