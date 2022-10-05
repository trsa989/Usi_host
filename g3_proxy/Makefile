
CXX=gcc

COPTS = -O0 -Wall -g -fprofile-arcs -ftest-coverage

CFLAGS= $(COPTS) -c -std=c99

# Unix-like environment assumed
MKDIR_P = mkdir -p

.PHONY: directories clean

TARGETS = directories g3proxy

LIBS = -ldl

LDFLAGS = $(COPTS) -lgcov --coverage

INCLUDE = -I"./"
INCLUDE+= -I"./config"
INCLUDE+= -I"./platform/Simulator"
INCLUDE+= -I"../"
INCLUDE+= -I"../src"
INCLUDE+= -I"./serial_if_adp_mac"
INCLUDE+= -I"./usi_cli"

OBJ_DIR = ./OBJ

OBJ_COMMON =                   \
		$(OBJ_DIR)/g3proxy.o	\
		$(OBJ_DIR)/debug.o	\
		$(OBJ_DIR)/tty_usi.o	\
		$(OBJ_DIR)/addUsi.o	\
    	$(OBJ_DIR)/ifaceMngLayer.o	\
    	$(OBJ_DIR)/ifacePrime_api.o	\
		$(OBJ_DIR)/ifacePrimeSniffer.o	\
		$(OBJ_DIR)/ifaceG3Adp.o	\
		$(OBJ_DIR)/ifaceG3Coord.o	\
		$(OBJ_DIR)/ifaceG3Mac.o	\
		$(OBJ_DIR)/Usi.o	\
		$(OBJ_DIR)/UsiCfg.o	\
		$(OBJ_DIR)/serial_if_adp.o	\
		$(OBJ_DIR)/serial_if_common.o	\
		$(OBJ_DIR)/serial_if_coordinator.o	\
		$(OBJ_DIR)/serial_if_mac.o	\
		$(OBJ_DIR)/serial_if_mib_common.o	\
		$(OBJ_DIR)/usi_cli.o	\

		
# Make.
all: $(TARGETS)

directories: ${OBJ_DIR}

${OBJ_DIR}:
	${MKDIR_P} ${OBJ_DIR}

g3proxy:$(OBJ_COMMON)
	$(CXX) $(LDFLAGS) $(INCLUDE) -o g3proxy.exe $(OBJ_COMMON) $(LIBS)
 
$(OBJ_DIR)/g3proxy.o: ./g3proxy.c
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/g3proxy.o ./g3proxy.c
	
$(OBJ_DIR)/debug.o: ./debug.c
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/debug.o ./debug.c 
	
$(OBJ_DIR)/tty_usi.o: ./tty_usi.c
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/tty_usi.o ./tty_usi.c

$(OBJ_DIR)/addUsi.o: ../src/addUsi.c
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/addUsi.o ../src/addUsi.c
			
$(OBJ_DIR)/Usi.o: ../src/Usi.c ../src/Usi.h
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/Usi.o ../src/Usi.c

$(OBJ_DIR)/UsiCfg.o: ../src/UsiCfg.c ../src/Usi.h
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/UsiCfg.o ../src/UsiCfg.c
			
$(OBJ_DIR)/serial_if_adp.o: ./serial_if_adp_mac/serial_if_adp.c ./serial_if_adp_mac/serial_if_adp.h
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/serial_if_adp.o ./serial_if_adp_mac/serial_if_adp.c

$(OBJ_DIR)/serial_if_common.o: ./serial_if_adp_mac/serial_if_common.c ./serial_if_adp_mac/serial_if_common.h
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/serial_if_common.o ./serial_if_adp_mac/serial_if_common.c
	
$(OBJ_DIR)/serial_if_coordinator.o: ./serial_if_adp_mac/serial_if_coordinator.c ./serial_if_adp_mac/serial_if_coordinator.h
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/serial_if_coordinator.o ./serial_if_adp_mac/serial_if_coordinator.c
	
$(OBJ_DIR)/serial_if_mac.o: ./serial_if_adp_mac/serial_if_mac.c ./serial_if_adp_mac/serial_if_mac.h
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/serial_if_mac.o ./serial_if_adp_mac/serial_if_mac.c
	
$(OBJ_DIR)/serial_if_mib_common.o: ./serial_if_adp_mac/serial_if_mib_common.c ./serial_if_adp_mac/serial_if_mib_common.h
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/serial_if_mib_common.o ./serial_if_adp_mac/serial_if_mib_common.c

$(OBJ_DIR)/usi_cli.o: ./usi_cli/usi_cli.c ./usi_cli/usi_cli.h
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/usi_cli.o ./usi_cli/usi_cli.c

$(OBJ_DIR)/ifaceMngLayer.o: ../src/ifaceMngLayer.c 
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/ifaceMngLayer.o ../src/ifaceMngLayer.c 

$(OBJ_DIR)/ifacePrime_api.o: ../src/ifacePrime_api.c ../src/ifacePrime_api.h
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/ifacePrime_api.o ../src/ifacePrime_api.c
	
$(OBJ_DIR)/ifacePrimeSniffer.o: ../src/ifacePrimeSniffer.c
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/ifacePrimeSniffer.o ../src/ifacePrimeSniffer.c 
	
$(OBJ_DIR)/ifaceG3Adp.o: ../src/ifaceG3Adp.c
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/ifaceG3Adp.o ../src/ifaceG3Adp.c 
		
$(OBJ_DIR)/ifaceG3Coord.o: ../src/ifaceG3Coord.c
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/ifaceG3Coord.o ../src/ifaceG3Coord.c 
		
$(OBJ_DIR)/ifaceG3Mac.o: ../src/ifaceG3Mac.c
	$(CXX) $(INCLUDE) $(CFLAGS) -o $(OBJ_DIR)/ifaceG3Mac.o ../src/ifaceG3Mac.c 
  
# Clean.
clean:
	rm -rf $(OBJ_DIR)/*.o
	rm -rf $(OBJ_DIR)/*.gcno
	rm -rf $(OBJ_DIR)/*.gcda
	rm -rf g3proxy.exe
	
