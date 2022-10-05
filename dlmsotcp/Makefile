#CXX=/home/sesteban/sam9n12-rootfs/buildroot-2011.05/output/host/usr/bin/arm-unknown-linux-uclibcgnueabi-g++
CXX=gcc

#COPTS = -O0 -Wall -g  -static
COPTS = -O0 -Wall -g

CFLAGS= $(COPTS) -c

TARGETS = dlmsotcp 

LIBS = -ldl
LDFLAGS = $(COPTS)

INCLUDE = -I"./"
INCLUDE+= -I"../"
INCLUDE+= -I"../src"

OBJ_DIR = ./OBJ

OBJ_COMMON =               \
        $(OBJ_DIR)/main.o		\
        $(OBJ_DIR)/debug.o		\
        $(OBJ_DIR)/dlmsotcp.o		\
		$(OBJ_DIR)/tty_usi.o		\
    	$(OBJ_DIR)/addUsi.o		\
    	$(OBJ_DIR)/ifaceMngLayer.o	\
    	$(OBJ_DIR)/ifacePrime_api.o	\
		$(OBJ_DIR)/ifacePrimeSniffer.o	\
		$(OBJ_DIR)/Usi.o		\
		$(OBJ_DIR)/UsiCfg.o
  	
    
all: $(TARGETS)

dlmsotcp:$(OBJ_COMMON)
	$(CXX)  $(LDFLAGS) $(INCLUDE) -o dlmsotcp $(OBJ_COMMON) $(LIBS)

$(OBJ_DIR)/main.o: ./main.c
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/main.o ./main.c

$(OBJ_DIR)/dlmsotcp.o: ./dlmsotcp.c ./dlmsotcp.h
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/dlmsotcp.o ./dlmsotcp.c	

$(OBJ_DIR)/debug.o: ./debug.c ./debug.h
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/debug.o ./debug.c

$(OBJ_DIR)/tty_usi.o: ./tty_usi.c
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/tty_usi.o ./tty_usi.c	

$(OBJ_DIR)/addUsi.o: ../src/addUsi.c
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/addUsi.o ../src/addUsi.c
	
$(OBJ_DIR)/ifaceMngLayer.o: ../src/ifaceMngLayer.c 
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/ifaceMngLayer.o ../src/ifaceMngLayer.c 

$(OBJ_DIR)/ifacePrime_api.o: ../src/ifacePrime_api.c ../src/ifacePrime_api.h
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/ifacePrime_api.o ../src/ifacePrime_api.c
	
$(OBJ_DIR)/ifacePrimeSniffer.o: ../src/ifacePrimeSniffer.c
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/ifacePrimeSniffer.o ../src/ifacePrimeSniffer.c 

$(OBJ_DIR)/Usi.o: ../src/Usi.c ../src/Usi.h
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/Usi.o ../src/Usi.c

$(OBJ_DIR)/UsiCfg.o: ../src/UsiCfg.c ../src/Usi.h
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/UsiCfg.o ../src/UsiCfg.c


clean:
	rm $(OBJ_DIR)/*.o
	rm dlmsotcp
