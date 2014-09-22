CXX=g++

CFLAGS=-g -Wall -pipe -m32

LIB=-lpthread -L/usr/local/mysql/lib -lmysqlclient -lz
INCFLGS=-I/usr/local/mysql/include/ 

TARGET=idc_speed_detect

OBJ=FastPing.o idc_speed_detect.o cmysql.o

${TARGET}:$(OBJ)
	${CXX} ${CFLAGS} -o $@ ${INCFLGS} $^ ${LIB} 

clean:
	rm -rvf $(OBJ) *.o ${TARGET} *~ core* *.out *.log
	
%.o: %.cpp
	gcc ${CFLAGS} ${INCFLGS} -c -o $@ $<
