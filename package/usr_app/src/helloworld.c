#include <stdio.h>
#include "poll_socket.h"
#include "sqlite.h"
#include "m1_protocol.h"
#include "thread.h"

extern void* sqlite_test(void* argc);
extern void* server_init(void* argc);
extern char * makeJson(void);
#define TEXT_NUMBER 10
char* test_buf[TEXT_NUMBER] = {"read","write","echo", "net","reportAP","reportDev","added","apReportData","apReportDev","apReportAp"};

char* readParam = "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":1,\"pdu\":{\"pduType\":6,\"devData\":[{\"devId\":\"12345678\",\"paramType\":[16391,16390,16404]},{\"devId\":\"12345679\",\"paramType\":[8222,8223,16404]}]}}";
char* writeParam = "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":1,\"pdu\":{\"pduType\":5,\"devData\":[{\"devId\":\"12345679\",\"param\":[{\"type\":8223,\"value\":10},{\"type\":8222,\"value\":20}]},{\"devId\":\"12345678\",\"param\":[{\"type\":16390,\"value\":20},{\"type\":16391,\"value\":30}]}]}}";
char* echoParam = "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":1,\"pdu\":{\"pduType\":16389,\"devData\":{\"APId\":\"11111111\",\"devId\":[\"11111111\",\"12345678\",\"12345679\"]}}}";
char* netControl = "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":1,\"pdu\":{\"pduType\":4,\"devData\": 1}}";
char* reportApInfo = "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":1,\"pdu\":{\"pduType\":3,\"devData\": 1}}";
char* reportDevInfo = "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":1,\"pdu\":{\"pduType\":14,\"devData\": \"11111111\"}}";
char* reqaddedInfo = "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":2,\"pdu\":{\"pduType\":16390}}";
char* apReportData = "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":1,\"pdu\":{\"pduType\":4098,\"devData\":[{\"devName\":\"s11\",\"devId\":\"12345678\",\"param\":[{\"type\":8223,\"value\":10},{\"type\":8222,\"value\":20}]},{\"devName\":\"s4\",\"devId\":\"12345679\",\"param\":[{\"type\":16390,\"value\":20},{\"type\":16391,\"value\":80}]}]}}";
char* apReportDev =  "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":1,\"pdu\":{\"pduType\":4103,\"devData\":{\"port\":26248,\"APId\":\"11111111\",\"APName\":\"AP1\",\"dev\":[{\"devId\":\"12345678\",\"devName\":\"s11\"},{\"devId\":\"12345679\",\"devName\":\"s4\"}]}}}";
char* apReportAp =  "{\"sn\":1,\"version\":\"1.0\",\"netFlag\":1,\"cmdType\":1,\"pdu\":{\"pduType\":4102,\"devData\":{\"port\":26248,\"APId\":\"11111111\",\"APName\":\"AP1\"}}}";

int main(int argc, char* argv[])
{
	char* db_name = "test_db";
	char* tmp = NULL;
	int i;

	printf("Hello world\n");
	printf("argv:%s, \n",argv[1]);
	if(argv[1] != NULL){
		for(i = 0; i < TEXT_NUMBER; i++){
			if(strcmp(argv[1], test_buf[i]) == 0)
				break;

		}
		switch(i){
			case 0: printf("Json:%s\n",readParam); data_handle(readParam); break;
			case 1: printf("Json:%s\n",writeParam); data_handle(writeParam); break;
			case 2: printf("Json:%s\n",echoParam); data_handle(echoParam); break;
			case 3: printf("Json:%s\n",netControl); data_handle(netControl); break;
			case 4: printf("Json:%s\n",reportApInfo); data_handle(reportApInfo); break;
			case 5: printf("Json:%s\n",reportDevInfo); data_handle(reportDevInfo); break;
			case 6: printf("Json:%s\n",reqaddedInfo); data_handle(reqaddedInfo); break;
			case 7: printf("Json:%s\n",apReportData); data_handle(apReportData); break;
			case 8: printf("Json:%s\n",apReportDev); data_handle(apReportDev); break;
			case 9: printf("Json:%s\n",apReportAp); data_handle(apReportAp); break;

			default: printf("no match\n");
		}
	}
	
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

