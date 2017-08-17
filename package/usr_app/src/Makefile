CC=gcc
EXEC = helloworld
OBJS = helloworld.o poll_server.o sqlite.o m1_protocol.o thread.o cJSON.o
CFLAGS += -O2 -Wall 
LIBS = -lpthread -lm -lrt -ldl -lsqlite3

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)  $(LIBS)
.PHONY:clean
clean:
	rm -rf $(OBJS) $(EXEC)

romfs:
	$(ROMFSINST) $(EXEC) /bin/$(EXEC)

