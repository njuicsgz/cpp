CXX=g++
CFLAG=-Wall -g -pipe -m32
INCFLGS=-I. -I/usr/local/mysql-32/include  
LIB= -lpthread  -lz /usr/local/mysql-32/lib/libmysqlclient.a
TARGET=ipdc_svr
OBJ= Helper.o  main.o  cached_conn.o  protocol_long.o  raw_cache.o  runner.o  CmdProcess.o  Conf.o  Log.o CSessionMgr.o DBProcessor.o cmysql.o comm_func.o sync_ip_mem.o 

${TARGET}: $(OBJ)
	${CXX} ${CFLAG} -o $@ $(INCFLGS) $^ ${LIB}
	cp ${TARGET} ../bin

clean:
	rm -rvf $(OBJ) *.o ${TARGET} *~ core* *.out 
%.o: %.cpp
	g++ ${CFLAG} $(INCFLGS) -c -o $@ $<
%.o: %.c
	gcc ${CFLAG} $(INCFLGS) -c -o $@ $<
