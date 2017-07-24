helloworld : helloworld.o poll_server.o sqlite.o
	$(CC) $(LDFLAGS) helloworld.o poll_server.o  sqlite.o -o helloworld -lsqlite3

helloworld.o : helloworld.c poll_socket.h sqlite.h
	$(CC) $(CFLAGS) -c helloworld.c

poll_server.o : poll_server.c poll_socket.h
	$(CC) $(CFLAGS) -c poll_server.c

sqlite.o : sqlite.c sqlite.h
	$(CC) $(CFLAGS) -c sqlite.c

	
clean:
	rm *.o helloworld
