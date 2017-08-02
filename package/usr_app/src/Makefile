helloworld : helloworld.o poll_server.o sqlite.o m1_protocol.o thread.o cjson.o
	$(CC) $(LDFLAGS) helloworld.o poll_server.o  sqlite.o m1_protocol.o thread.o  cjson.o -lm -o helloworld -lsqlite3 -lpthread

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

cjson.o : cjson.c cjson.h
	$(CC) $(CFLAGS) -c cjson.c	

clean:
	rm *.o helloworld
