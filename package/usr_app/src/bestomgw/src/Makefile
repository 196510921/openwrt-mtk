#############################################################
# nfc daemon Makefile 
#############################################################
EXEC = bestomgwd
CFLAGS += -O2 -Wall -I bestomgw/inc -I dalitek/inc -I sql_operate/inc -I sql_table/inc -I tcp_client/inc
##-IthreadPool


##OBJS = bestomgw/src/utils.o bestomgw/src/sqlite3.o bestomgw/src/timertask.o bestomgw/src/gateway.o 

OBJS = bestomgw/src/utils.o bestomgw/src/sqlite3.o bestomgw/src/zbController.o \
	bestomgw/src/zbSocCmd.o bestomgw/src/interface_devicelist.o bestomgw/src/interface_grouplist.o \
	bestomgw/src/interface_scenelist.o bestomgw/src/interface_srpcserver.o \
	bestomgw/src/socket_server.o bestomgw/src/SimpleDB.o bestomgw/src/SimpleDBTxt.o  bestomgw/src/cJSON.o \
	dalitek/src/m1_protocol.o dalitek/src/m1_linkage.o dalitek/src/m1_scenario.o dalitek/src/m1_district.o \
	dalitek/src/buf_manage.o  dalitek/src/account_config.o \
	dalitek/src/m1_project.o dalitek/src/sql_backup.o dalitek/src/m1_common_log.o dalitek/src/m1_device.o \
	tcp_client/src/tcp_client.o	
	#sql_operate/sql_operate.o sql_operate/sql_delete.o sql_operate/sql_insert.o sql_opera:te/sql_select.o \
	#sql_operate/sql_modify.o \
	#sql_table/sql_table.o \
	#sql_table/table_all_dev.o \
	#sql_table/table_account_table.o sql_table/table_account_info.o \
	#sql_table/table_conn_info.o sql_table/table_district_table.o sql_table/table_link_exec_table.o \
	#sql_table/table_link_trigger_table.o \
	#sql_table/table_linkage_table.o sql_table/table_param_table.o sql_table/table_project_table.o \
	#sql_table/table_scen_alarm_table.o \
	#sql_table/table_scenario_table.o
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

