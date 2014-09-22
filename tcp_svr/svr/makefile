CXX=g++
INCFLGS=-I.   
LIB= -lpthread  -lz 
TARGET=svr
OBJ= Helper.o  main.o  cached_conn.o  protocol_long.o  raw_cache.o  runner.o  CmdProcess.o  Conf.o  Log.o CSessionMgr.o 

${TARGET}: $(OBJ)
	${CXX} -Wall -g -pipe -o $@ $(INCFLGS) $^ ${LIB}
	cp {TARGET} ../bin

clean:
	rm -rvf $(OBJ) *.o ${TARGET} *~ core* *.out 
%.o: %.cpp
	g++ -Wall -g -pipe $(INCFLGS) -c -o $@ $<
%.o: %.cpp
	gcc -Wall -g -pipe $(INCFLGS) -c -o $@ $<
