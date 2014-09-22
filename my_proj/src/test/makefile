INCFLGS=-I. -I/usr/local/mysql/include 
LIB=-L. -L/usr/local/lib -L/usr/local/mysql/lib -lmysqlclient -lz -L../lib ../lib/json_lib/libjson.a 
CFLAGS=-Wall -g
COM = ../common
SRCFILES := $(wildcard [^l]*.cpp)
BIN      := $(patsubst %.cpp,%,$(SRCFILES))
INC=-I../include 
COM_OBJS=ItilIntf.o cmysql.o Log.o Conf.o http.o

all:$(BIN)
	cp $(BIN) ../../bin
$(BIN): %:%.o $(COM_OBJS)
	g++ $(CFLAGS) -o $@ $^ $(LIB)
%.o: $(COM)/%.cpp
	g++ -g -c $(CFLAGS) $(INC) $(INCFLGS) $^	
%.o: $(COM)/%.c
	g++ -g -c $(CFLAGS) $(INC) $(INCFLGS) $^ 
%.o: %.cpp
	g++ -g -c $(CFLAGS) $(INC) $(INCFLGS) -o $@ $^
clean:
	rm -f $(COM_OBJS) $(BIN) *.o
