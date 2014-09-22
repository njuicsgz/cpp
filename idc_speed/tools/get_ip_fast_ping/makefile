CXX=g++

CFLAGS=-g -Wall -pipe -m32

LIB=-lpthread -lz
INCFLGS=-I./

TARGET=fast_ping_tool

OBJ=FastPing.o fast_ping_tool.o 

${TARGET}:$(OBJ)
	${CXX} ${CFLAGS} -o $@ ${INCFLGS} $^ ${LIB} 

clean:
	rm -rvf $(OBJ) *.o ${TARGET} *~ core* *.out *.log
	
%.o: %.cpp
	gcc ${CFLAGS} ${INCFLGS} -c -o $@ $<
