#############################################################
# nfc daemon Makefile 
#############################################################
EXEC    = mina_test_exe
CFLAGS += -O2 -Wall \
          -Isrc/core \
          -Isrc/test \
   

OBJS    =   src/core/iobuffer.o src/core/iosession.o src/core/session_config.o \
            src/core/socket_acceptor.o src/core/connector.o src/core/socket_handler.o \
            src/test/common.o src/test/main.o src/test/test_acceptor.o src/test/test_connector.o 

LIBS = -lpthread -levent_pthreads -levent 
#LDFLAGS = -levent

all: $(EXEC)

$(EXEC): $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) 

clean:
	rm -rf $(OBJS) $(EXEC)

romfs:
	$(ROMFSINST) $(EXEC) /bin/$(EXEC)

