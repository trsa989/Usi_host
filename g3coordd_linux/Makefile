CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar

COPTS_COORD= -c -O0 -pipe -g3 -Wall -DLINUX -D__G3_COORD__ -DSPEC_COMPLIANCE=17
COPTS_COORD+= -DAPP_CONFORMANCE_TEST -DG3_HYBRID_PROFILE

.PHONY: folders
TARGETS = folders g3coordd

OBJ_DIR = ./OBJ
DIRS = $(OBJ_DIR)

LIBS =
LDFLAGS =  -lpthread
INCLUDE = -I./g3 \
		  -I. \
			-I../. \
		  -I./source \
		  -I./source/port \
		  -I./source/port/oss \
		  -I./source/port/oss/module_config \
		  -I./source/port/common \
			-I./g3/bootstrap/include \
			-I./g3/bootstrap/source \
			-I./mbed-tls \
			-I./mbed-tls/include \
			-I./mbed-tls/include/mbedtls \

all: $(TARGETS)

OBJ_LIST = $(OBJ_DIR)/main.o \
			 $(OBJ_DIR)/Logger.o \
		   $(OBJ_DIR)/oss_if.o \
			 $(OBJ_DIR)/userFnc.o	\
			 $(OBJ_DIR)/tun.o	\
			 $(OBJ_DIR)/addUsi.o	\
			 $(OBJ_DIR)/ifaceG3Adp.o	\
			 $(OBJ_DIR)/Usi.o	\
			 $(OBJ_DIR)/UsiCfg.o	\
			 $(OBJ_DIR)/app_adp_mng.o \
			 $(OBJ_DIR)/udp_responder.o \
			 $(OBJ_DIR)/storage.o	\
			 $(OBJ_DIR)/Random.o	\
			 $(OBJ_DIR)/gpio.o	\
			 $(OBJ_DIR)/led.o	\
			 $(OBJ_DIR)/bs_main.o \
			 $(OBJ_DIR)/bs_functions.o \
			 $(OBJ_DIR)/ProtoEapPsk.o \
			 $(OBJ_DIR)/ProtoLbp.o \
			 $(OBJ_DIR)/aes_sig.o \
		   $(OBJ_DIR)/aesni.o \
		   $(OBJ_DIR)/ccm.o \
		   $(OBJ_DIR)/cipher.o \
		   $(OBJ_DIR)/cipher_wrap.o \
		   $(OBJ_DIR)/cmac.o \
		   $(OBJ_DIR)/debug_tls.o \
		   $(OBJ_DIR)/error.o \
		   $(OBJ_DIR)/gcm.o \
		   $(OBJ_DIR)/memory_buffer_alloc.o \
		   $(OBJ_DIR)/oid_tls.o \
		   $(OBJ_DIR)/platform_tls.o \
		   $(OBJ_DIR)/timing.o \
		   $(OBJ_DIR)/version.o \
		   $(OBJ_DIR)/version_features.o \
		   $(OBJ_DIR)/aes_wrapper.o \
			 $(OBJ_DIR)/cipher_wrapper.o \
 		   $(OBJ_DIR)/eax.o \
		   $(OBJ_DIR)/debug.o


$(OBJ_DIR)/addUsi.o: ../src/addUsi.c
	$(CC) $(COPTS_COORD) $(INCLUDE) -o $(OBJ_DIR)/addUsi.o ../src/addUsi.c

$(OBJ_DIR)/Usi.o: ../src/Usi.c ../src/Usi.h
	$(CC) $(COPTS_COORD) $(INCLUDE) -o $(OBJ_DIR)/Usi.o ../src/Usi.c

$(OBJ_DIR)/UsiCfg.o: ../src/UsiCfg.c ../src/Usi.h
	$(CC) $(COPTS_COORD) $(INCLUDE) -o $(OBJ_DIR)/UsiCfg.o ../src/UsiCfg.c

$(OBJ_DIR)/ifaceG3Adp.o: ../src/ifaceG3Adp.c
	$(CC) $(COPTS_COORD) $(INCLUDE) -o $(OBJ_DIR)/ifaceG3Adp.o ../src/ifaceG3Adp.c

$(OBJ_DIR)/ifaceG3Coord.o: ../src/ifaceG3Coord.c
		$(CC) $(COPTS_COORD) $(INCLUDE) -o $(OBJ_DIR)/ifaceG3Coord.o ../src/ifaceG3Coord.c

$(OBJ_DIR)/userFnc.o: ./userFnc.c
	$(CC) $(COPTS_COORD) $(INCLUDE) -o $(OBJ_DIR)/userFnc.o ./userFnc.c

$(OBJ_DIR)/tun.o: ./tun.c
	$(CC) $(COPTS_COORD) $(INCLUDE) -o $(OBJ_DIR)/tun.o ./tun.c

$(OBJ_DIR)/udp_responder.o: ./udp_responder.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./udp_responder.c -o $(OBJ_DIR)/udp_responder.o

$(OBJ_DIR)/storage.o: ./source/port/common/storage.c ./source/port/common/storage.h
	$(CC) $(COPTS_COORD) $(INCLUDE) ./source/port/common/storage.c -o $(OBJ_DIR)/storage.o

$(OBJ_DIR)/Random.o: ./source/port/common/Random.c ./source/port/common/Random.h
	$(CC) $(COPTS_COORD) $(INCLUDE) ./source/port/common/Random.c -o $(OBJ_DIR)/Random.o

$(OBJ_DIR)/gpio.o: ./source/port/common/gpio.c ./source/port/common/gpio.h
	$(CC) $(COPTS_COORD) $(INCLUDE) ./source/port/common/gpio.c -o $(OBJ_DIR)/gpio.o

$(OBJ_DIR)/led.o: ./source/port/common/gpio.c ./source/port/common/led.h
	$(CC) $(COPTS_COORD) $(INCLUDE) ./source/port/common/led.c -o $(OBJ_DIR)/led.o


$(OBJ_DIR)/oss_if.o: ./source/port/oss/oss_if.c ./source/port/oss/oss_if.h
	$(CC) $(COPTS_COORD) $(INCLUDE) ./source/port/oss/oss_if.c  -o $(OBJ_DIR)/oss_if.o

$(OBJ_DIR)/Logger.o: ./source/port/common/Logger.c ./source/port/common/Logger.h
	$(CC) $(COPTS_COORD) $(INCLUDE) ./source/port/common/Logger.c  -o $(OBJ_DIR)/Logger.o

$(OBJ_DIR)/app_adp_mng.o: ./app_adp_mng.c ./app_adp_mng.h
	$(CC) $(COPTS_COORD) $(INCLUDE) ./app_adp_mng.c -o $(OBJ_DIR)/app_adp_mng.o

$(OBJ_DIR)/main.o: ./main.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./main.c -o $(OBJ_DIR)/main.o

$(OBJ_DIR)/bs_main.o: ./g3/bootstrap/source/bs_main.c ./g3/bootstrap/source/bs_functions.h ./g3/bootstrap/include/bs_api.h
	$(CC) $(COPTS_COORD) $(INCLUDE) ./g3/bootstrap/source/bs_main.c  -o $(OBJ_DIR)/bs_main.o

$(OBJ_DIR)/bs_functions.o: ./g3/bootstrap/source/bs_functions.c ./g3/bootstrap/source/bs_functions.h ./g3/bootstrap/include/bs_api.h
	$(CC) $(COPTS_COORD) $(INCLUDE) ./g3/bootstrap/source/bs_functions.c  -o $(OBJ_DIR)/bs_functions.o

$(OBJ_DIR)/ProtoEapPsk.o: ./g3/bootstrap/source/ProtoEapPsk.c ../ProtoEapPsk.h
	$(CC) $(COPTS_COORD) $(INCLUDE) ./g3/bootstrap/source/ProtoEapPsk.c  -o $(OBJ_DIR)/ProtoEapPsk.o

$(OBJ_DIR)/ProtoLbp.o: ./g3/bootstrap/source/ProtoLbp.c ../ProtoLbp.h
	$(CC) $(COPTS_COORD) $(INCLUDE) ./g3/bootstrap/source/ProtoLbp.c  -o $(OBJ_DIR)/ProtoLbp.o

$(OBJ_DIR)/aes_wrapper.o: ./g3/crypto/aes_wrapper.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./g3/crypto/aes_wrapper.c  -o $(OBJ_DIR)/aes_wrapper.o

$(OBJ_DIR)/cipher_wrapper.o: ./g3/crypto/cipher_wrapper.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./g3/crypto/cipher_wrapper.c  -o $(OBJ_DIR)/cipher_wrapper.o

$(OBJ_DIR)/aes_sig.o: ./mbed-tls/library/aes_sig.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./mbed-tls/library/aes_sig.c  -o $(OBJ_DIR)/aes_sig.o

$(OBJ_DIR)/aesni.o: ./mbed-tls/library/aesni.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./mbed-tls/library/aesni.c  -o $(OBJ_DIR)/aesni.o

$(OBJ_DIR)/ccm.o: ./mbed-tls/library/ccm.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./mbed-tls/library/ccm.c  -o $(OBJ_DIR)/ccm.o

$(OBJ_DIR)/cipher.o: ./mbed-tls/library/cipher.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./mbed-tls/library/cipher.c  -o $(OBJ_DIR)/cipher.o

$(OBJ_DIR)/cipher_wrap.o: ./mbed-tls/library/cipher_wrap.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./mbed-tls/library/cipher_wrap.c  -o $(OBJ_DIR)/cipher_wrap.o

$(OBJ_DIR)/cmac.o: ./mbed-tls/library/cmac.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./mbed-tls/library/cmac.c  -o $(OBJ_DIR)/cmac.o

$(OBJ_DIR)/debug_tls.o: ./mbed-tls/library/debug_tls.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./mbed-tls/library/debug_tls.c  -o $(OBJ_DIR)/debug_tls.o

$(OBJ_DIR)/debug.o: ./debug.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./debug.c  -o $(OBJ_DIR)/debug.o

$(OBJ_DIR)/error.o: ./mbed-tls/library/error.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./mbed-tls/library/error.c  -o $(OBJ_DIR)/error.o

$(OBJ_DIR)/gcm.o: ./mbed-tls/library/gcm.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./mbed-tls/library/gcm.c  -o $(OBJ_DIR)/gcm.o

$(OBJ_DIR)/memory_buffer_alloc.o: ./mbed-tls/library/memory_buffer_alloc.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./mbed-tls/library/memory_buffer_alloc.c  -o $(OBJ_DIR)/memory_buffer_alloc.o

$(OBJ_DIR)/oid_tls.o: ./mbed-tls/library/oid_tls.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./mbed-tls/library/oid_tls.c  -o $(OBJ_DIR)/oid_tls.o

$(OBJ_DIR)/platform_tls.o: ./mbed-tls/library/platform_tls.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./mbed-tls/library/platform_tls.c  -o $(OBJ_DIR)/platform_tls.o

$(OBJ_DIR)/timing.o: ./mbed-tls/library/timing.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./mbed-tls/library/timing.c  -o $(OBJ_DIR)/timing.o

$(OBJ_DIR)/version.o: ./mbed-tls/library/version.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./mbed-tls/library/version.c  -o $(OBJ_DIR)/version.o

$(OBJ_DIR)/version_features.o: ./mbed-tls/library/version_features.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./mbed-tls/library/version_features.c  -o $(OBJ_DIR)/version_features.o

$(OBJ_DIR)/eax.o: ./g3/crypto/eax.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./g3/crypto/eax.c  -o $(OBJ_DIR)/eax.o

$(OBJ_DIR)/aes_wrapper.o: ./g3/crypto/aes_wrapper.c
	$(CC) $(COPTS_COORD) $(INCLUDE) ./g3/crypto/aes_wrapper.c  -o $(OBJ_DIR)/aes_wrapper.o

project_config:
	# Copy Project Configuration File to Main Folder
	cp -a PrjCfg.h ../.

g3coordd: project_config $(OBJ_LIST)
	# Compile
	$(CC) $(LDFLAGS) $(OBJ_LIST) -o ./g3coordd $(LDFLAGS)

clean:
	rm -rf $(OBJ_DIR)/*.o
	rm -rf ./g3coordd*

$(shell mkdir -p $(DIRS))
