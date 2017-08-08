#include <stdio.h>
#include "poll_socket.h"
#include "sqlite.h"
#include "m1_protocol.h"
#include "thread.h"

extern void* sqlite_test(void* argc);
extern void* server_init(void* argc);
extern char * makeJson(void);

char* readParam = "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":1,\"pdu\":{\"pduType\":6,\"devData\":[{\"devId\":\"12345678\",\"paramType\":[16391,16390,7656,7644]},{\"devId\":\"12345679\",\"paramType\":[8222,8223,8205]}]}}";
char* writeParam = "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":1,\"pdu\":{\"pduType\":5,\"devData\":[{\"devId\":\"1234567A\",\"param\":[{\"type\":8223,\"value\":10},{\"type\":8222,\"value\":20}]},{\"devId\":\"1234567B\",\"param\":[{\"type\":8223,\"value\":20},{\"type\":8222,\"value\":30}]}]}}";
char* echoParam = "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":1,\"pdu\":{\"pduType\":16389,\"devData\":[\"1234567C\",\"1234567D\"]}}";
char* netControl = "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":1,\"pdu\":{\"pduType\":4,\"devData\": 1}}";
char* reportApInfo = "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":1,\"pdu\":{\"pduType\":3,\"devData\": 1}}";
char* reqaddedInfo = "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":2,\"pdu\":{\"pduType\":16390}}";

int main(int argc, char* argv[])
{
	char* db_name = "test_db";
	char* tmp = NULL;

	printf("Hello world\n");
	//tmp = makeJson();
	/*printf("Json:%s\n",readParam);
	data_handle(readParam);*/
	/*printf("Json:%s\n",writeParam);
	data_handle(writeParam);*/
	/*printf("Json:%s\n",echoParam);
	data_handle(echoParam);*/
	/*printf("Json:%s\n",netControl);
	data_handle(netControl);*/
	printf("Json:%s\n",reportApInfo);
	data_handle(reportApInfo);
	/*printf("Json:%s\n",reqaddedInfo);
	data_handle(reqaddedInfo);
*/
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

