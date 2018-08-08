#############################################################
# nfc daemon Makefile 
#############################################################
EXEC    = udp_broadcast
CFLAGS += -O2 -Wall \

OBJS   = broadcast_msg.o udp_server.o cJSON.o

LIBS   = -lpthread -lm -lrt -ldl 
all: $(EXEC)

$(EXEC): $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) 

clean:
	rm -rf $(OBJS) $(EXEC)

romfs:
	$(ROMFSINST) $(EXEC) /bin/$(EXEC)

