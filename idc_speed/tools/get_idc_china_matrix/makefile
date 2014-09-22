CXX=g++

CFLAGS=-g -Wall -pipe -DDEBUG_ENABLE -m32

LIB=-lpthread  -L/usr/local/mysql/lib -lmysqlclient -lz
INCFLGS=-I/usr/local/mysql/include/ 

TARGET=get_idc_china_matrix

OBJ=get_idc_china_matrix.o cmysql.o

${TARGET}:$(OBJ)
	${CXX} ${CFLAGS} -o $@ ${INCFLGS} $^ ${LIB} 

clean:
	rm -rvf $(OBJ) *.o ${TARGET} *~ core* *.out *.log
	
%.o: %.cpp
	${CXX} ${CFLAGS} ${INCFLGS} -c -o $@ $<

%.o: %.c
	gcc ${CFLAGS} ${INCFLGS} -c -o $@ $<
