#############################################################
# nfc daemon Makefile 
#############################################################
EXEC    = sqlMemExe
CFLAGS += -O2 -Wall -I../bestomgw/inc

OBJS    =   app_mem.o app_conn_info_mem.o ../bestomgw/src/cJSON.o

LIBS = -lpthread -lm -lrt -ldl 
all: $(EXEC)

$(EXEC): $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) 

clean:
	rm -rf $(OBJS) $(EXEC)

romfs:
	$(ROMFSINST) $(EXEC) /bin/$(EXEC)

