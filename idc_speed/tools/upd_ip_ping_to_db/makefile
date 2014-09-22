CXX=g++

CFLAGS=-g -Wall -pipe -m32

LIB=-lpthread -L/usr/local/mysql/lib -lmysqlclient -lz
INCFLGS=-I/usr/local/mysql/include/ 

TARGET=upd_ip_ping_to_db

OBJ=FastPing.o upd_ip_ping_to_db.o cmysql.o

${TARGET}:$(OBJ)
	${CXX} ${CFLAGS} -o $@ ${INCFLGS} $^ ${LIB} 

clean:
	rm -rvf $(OBJ) *.o ${TARGET} *~ core* *.out *.log
	
%.o: %.cpp
	gcc ${CFLAGS} ${INCFLGS} -c -o $@ $<
