LIB= -lz -lpthread 
INCFLGS=-I../src/
CFLAGS=-g -Wall -pipe
TARGET=ApiTest
OBJ=Api.o ApiTest.o
${TARGET}:$(OBJ)
	g++ -Wall -g -pipe -o $@ ${INCFLGS} $^ ${LIB}
clean:
	rm -rvf $(OBJ) *.o ${TARGET} *~ core* *.out *.log
%.o: ../src/%.cpp
	g++ -Wall -g -pipe -pthread ${INCFLGS} -c -o $@ $<
%.o: %.cpp
	g++ -Wall -g -pipe -pthread ${INCFLGS} -c -o $@ $<

