#############################################################
# nfc daemon Makefile 
#############################################################
EXEC = bestomgwd
CFLAGS += -O2 -Wall -Ibestomgw/inc -Idalitek/inc 
##-IthreadPool


##OBJS = bestomgw/src/utils.o bestomgw/src/sqlite3.o bestomgw/src/timertask.o bestomgw/src/gateway.o 

OBJS = bestomgw/src/utils.o bestomgw/src/sqlite3.o bestomgw/src/zbController.o \
	bestomgw/src/zbSocCmd.o bestomgw/src/interface_devicelist.o bestomgw/src/interface_grouplist.o \
	bestomgw/src/interface_scenelist.o bestomgw/src/interface_srpcserver.o \
	bestomgw/src/socket_server.o bestomgw/src/SimpleDB.o bestomgw/src/SimpleDBTxt.o  bestomgw/src/cJSON.o \
	dalitek/src/m1_protocol.o dalitek/src/m1_linkage.o dalitek/src/m1_scenario.o dalitek/src/m1_district.o	dalitek/src/buf_manage.o  dalitek/src/account_config.o \
	dalitek/src/m1_project.o dalitek/src/sql_backup.o
	##threadPool/thpool.o

#LIBS = -lpthread -lm -lrt -lglib-2.0 -ldl
LIBS = -lpthread -lm -lrt -ldl
all: $(EXEC)

$(EXEC): $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) 
	#$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) -lpthread -lm -lrt  -lglib-2.0    -ldl

clean:
	rm -rf $(OBJS) $(EXEC)

romfs:
	$(ROMFSINST) $(EXEC) /bin/$(EXEC)

