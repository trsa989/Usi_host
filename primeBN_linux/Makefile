CC=$(CROSS_COMPILE)gcc
AR=$(CROSS_COMPILE)ar

COMPILE_DEBUG?=n
DESTDIR?=./

ifeq ($(COMPILE_DEBUG),y)
DEBUG_CFLAGS=-g -O0
else
DEBUG_CFLAGS=-O2
endif

INCLUDE+= -I.                               \
				  -I../.                            \
				-I../src  \
					-I./source/                         \
				  -I./source/port                          \
				  -I./source/port/common                   \
					-I./cli_core

COPTS = -fPIC $(DEBUG_FLAGS) $(INCLUDE) -Wall -D_GNU_SOURCE -DLINUX -DPRIME_API_V_1_4
CFLAGS= $(COPTS)

LIBS =-ldl
LIBS+=-lm -lcrypt -lpthread -lreadline -lncurses -lsqlite3

PROG=bn_prime
PROG_OBJS =               					    \
    	  ../src/addUsi.o									\
    	  ../src/ifaceMngLayer.o					\
    	  ../src/ifacePrime_api.o					\
		    ../src/ifacePrimeSniffer.o			\
		    ../src/Usi.o										\
		    ../src/UsiCfg.o									\
				./source/port/common/gpio.o			\
				./source/port/common/led.o			\
				./prime_log.o    							  \
				./prime_utils.o					        \
				./base_node_manager.o					  \
				./base_node_mng.o							  \
				./base_node_dlmsotcp.o				  \
				./base_node_mng_fw_upgrade.o		\
				./prime_bmng_network_events.o	  \
				./base_node_manager_main.o			\
				./base_node_network.o	          \
				./prime_sniffer.o							  \
				./base_node_manager_vty.o			  \
				./userFnc.o									 	  \
				./cli_core/command.o								\
				./cli_core/linklist.o							\
				./cli_core/memory.o								\
				./cli_core/vector.o								\
				./cli_core/vty.o										\
				./cli_core/vtysh.o									\
				./cli_core/vtysh_config.o					\
				./cli_core/cmd/cmd_common.o				\
				./cli_core/cmd/cmd_ip.o						\
				cli_core/cmd/cmd_tool.o

.PHONY: clean
TARGETS = bn_prime

%.o: %.c
		$(CC) -c $(CFLAGS) $< -o $@

all: $(TARGETS)

project_config:
	# Copy Project Configuration File to Main Folder
	cp -a PrjCfg.h ../.
	echo "Nothing to do"

$(PROG): $(PROG_OBJS)
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

install: install-target

install-target:
		mkdir -p $(DESTDIR)/usr/bin
		cp $(PROG) $(DESTDIR)/usr/bin

install-staging:
	 # Nothing to do

uninstall: uninstall-target

uninstall-target:
	 	-rm -f $(DESTDIR)/usr/bin/$(PROG)

uninstall-staging:
	 	# Nothing to do

clean:
			rm -rf $(PROG) $(PROG_OBJS)
