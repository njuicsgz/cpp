CXX=g++

CFLAGS=-g -Wall -pipe -DDEBUG_ENABLE  

INCFLGS=-I../include

LIB= -lpthread 

TARGET=agent

OBJ=ConnectThread.o helper.o agent.o Log.o Conf.o Api.o

${TARGET}:$(OBJ)
	${CXX} -Wall -g -pipe -o $@ ${INCFLGS} $^ ${LIB} 

clean:
	rm -rvf $(OBJ) *.o ${TARGET} *~ core* *.out *.log
	
%.o: %.cpp
	gcc -Wall -g -pipe ${INCFLGS} -c -o $@ $<
%.o: %.cpp
	gcc -Wall -g -pipe $(INCFLGS) -c -o $@ $<
