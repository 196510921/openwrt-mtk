helloworld : helloworld.o poll_server.o sqlite.o m1_protocol.o thread.o cJSON.o
	$(CC) $(LDFLAGS) helloworld.o poll_server.o  sqlite.o m1_protocol.o thread.o  cJSON.o -lm -o helloworld -lsqlite3 -lpthread

helloworld.o : helloworld.c poll_socket.h sqlite.h
	$(CC) $(CFLAGS) -c helloworld.c

poll_server.o : poll_server.c poll_socket.h
	$(CC) $(CFLAGS) -c poll_server.c

sqlite.o : sqlite.c sqlite.h
	$(CC) $(CFLAGS) -c sqlite.c

m1_protocol.o : m1_protocol.c m1_protocol.h
	$(CC) $(CFLAGS) -c m1_protocol.c	


thread.o : thread.c thread.h
	$(CC) $(CFLAGS) -c thread.c	

cjson.o : cJSON.c cJSON.h
	$(CC) $(CFLAGS) -c cJSON.c	

clean:
	rm *.o helloworld
