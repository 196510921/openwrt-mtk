#include <stdio.h>
#include "poll_socket.h"
#include "sqlite.h"

extern void sqlite_test(void);

int main(int argc, char* argv[])
{
	char* db_name = "test_db";
	
	printf("Hello world\n");
	//server_init();
	sqlite_test();
	return 0;
}

