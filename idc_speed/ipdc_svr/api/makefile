LIB= -lz -lpthread 
INCFLGS=-I./
CFLAGS=-g -Wall -pipe
TARGET=ApiTest
OBJ=ApiTest.o ipdc_api.o
${TARGET}:$(OBJ)
	g++ -Wall -g -pipe -o $@ ${INCFLGS} $^ ${LIB}
clean:
	rm -rvf $(OBJ) *.o ${TARGET} *~ core* *.out *.log
%.o: %.cpp
	g++ -Wall -g -pipe -pthread ${INCFLGS} -c -o $@ $<

