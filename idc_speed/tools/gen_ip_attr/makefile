CXX=g++

CFLAGS=-g -Wall -pipe -DDEBUG_ENABLE -m32

LIB=-lpthread  -L/usr/local/mysql/lib -lmysqlclient -lz
INCFLGS=-I/usr/local/mysql/include/ 

TARGET=gen_ip_attr_to_db

OBJ=gen_ip_attr_to_db.o cmysql.o 

${TARGET}:$(OBJ)
	${CXX} ${CFLAGS} -o $@ ${INCFLGS} $^ ${LIB} 

clean:
	rm -rvf $(OBJ) *.o ${TARGET} *~ core* *.out *.log
	
%.o: %.cpp
	${CXX} ${CFLAGS} ${INCFLGS} -c -o $@ $<

%.o: %.c
	gcc ${CFLAGS} ${INCFLGS} -c -o $@ $<
