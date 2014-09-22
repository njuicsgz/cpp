CXX=g++

CFLAGS=-g -Wall -pipe -DDEBUG_ENABLE -m32

INCFLGS=-I../include

LIB= -lpthread 

TARGET=idcspeed_agt

OBJ=ConnectThread.o helper.o agent.o Log.o Conf.o ipdc_api.o SyncData.o detect_rept.o FastPing.o

${TARGET}:$(OBJ)
	${CXX} ${CFLAGS} -o $@ ${INCFLGS} $^ ${LIB}
	cp  ${TARGET} ../bin/

clean:
	rm -rvf $(OBJ) *.o ${TARGET} *~ core* *.out *.log	
	
%.o: %.cpp
	gcc ${CFLAGS} ${INCFLGS} -c -o $@ $<
%.o: %.c
	gcc ${CFLAGS} $(INCFLGS) -c -o $@ $<
