/**
 * \file
 *
 * \brief USI Host
 *
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

#include "g3proxy.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <asm/types.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* To sleep for ms */
#include <unistd.h>

#include "addUsi.h"
#include "userFnc.h"

#include "usi_cli.h"

#include "debug.h"

#include "serial_if_common.h"

#define TCP_PORT 20001

#define TRUE  1
#define FALSE 0

/* g3proxy.c app runs in an infinite loop. To terminate it, it must be killed. In this
 * case no gcov code coverage data is dumped.
 * Defining RUNTIME_CODE_COVERAGE_GCOV uses signal USR1 to dump code coverage info.
 * Sending this signal (before killing the app) dumps coverage data. Example:
 * killall -USR1 g3proxy.exe
 * The file g3_proxy/OBJ/g3proxy.gcna will be created when the signal is received.
 */
/* #define RUNTIME_CODE_COVERAGE_GCOV */

td_x_args g_x_args = { TRUE,
		       4,
		       "/dev/ttyS30",
		       230400,
		       4059};

int g_usi_fd = 0;
int g_concentrator_fd = 0;

int tcp_port = TCP_PORT;

fd_set g_master_set, g_working_set;
int g_max_fd;

#ifdef RUNTIME_CODE_COVERAGE_GCOV
void __gcov_flush();

static void catch_function(int signal)
{
	__gcov_flush();
}

#endif

int open_server(int _i_port)
{
	int i_listen_sd;
	int i_on = 1;
	int i_rc;
	struct sockaddr_in x_addr;

	i_listen_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (i_listen_sd < 0) {
		PRINTF(PRINT_ERROR, "server socket() failed");
		exit(-1);
	}

	/* Allow socket descriptor to be reuseable                   */
	i_rc = setsockopt(i_listen_sd, SOL_SOCKET, SO_REUSEADDR, &i_on, sizeof(i_on));
	if (i_rc < 0) {
		PRINTF(PRINT_ERROR, "setsockopt() failed");
		close(i_listen_sd);
		exit(-1);
	}

	/* i_on =1; */
	/* i_rc = setsockopt(i_listen_sd, IPPROTO_TCP,  TCP_NODELAY, &i_on, sizeof(i_on)); */

	/* Set socket to be non-blocking.  All of the sockets for    */
	/* the incoming connections will also be non-blocking since  */
	/* they will inherit that state from the listening socket.   */

	/*i_rc = ioctl(i_listen_sd, FIONBIO, (char *)&i_on);
	 * if (i_rc < 0)
	 * {
	 *      PRINTF(PRINT_ERROR,"ioctl() failed");
	 *  close(i_listen_sd);
	 *  exit(-1);
	 * }*/

	/* Bind the socket                                           */
	memset(&x_addr, 0, sizeof(struct sockaddr_in));
	x_addr.sin_family      = AF_INET;
	x_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	x_addr.sin_port        = htons(_i_port);
	i_rc = bind(i_listen_sd, (struct sockaddr *)&x_addr, sizeof(x_addr));
	if (i_rc < 0) {
		PRINTF(PRINT_ERROR, "bind() failed");
		close(i_listen_sd);
		exit(-1);
	}

	/* Set the listen back log                                   */
	i_rc = listen(i_listen_sd, 1);
	if (i_rc < 0) {
		PRINTF(PRINT_ERROR, "listen() failed");
		close(i_listen_sd);
		exit(-1);
	}

	return i_listen_sd;
}

int main(int argc, char **argv)
{
	int i_server_sd;
	int i;

	struct timeval timeout;

	PRINTF(PRINT_INFO, "G3 Proxy\r\n\r\n");

#ifdef RUNTIME_CODE_COVERAGE_GCOV
	if (signal(SIGINT, catch_function) == SIG_ERR) {
		PRINTF(PRINT_ERROR, "An error occurred while setting a signal handler.\n", stderr);
		return EXIT_FAILURE;
	}
#endif

	if (argc >= 3) {
		if (strlen(argv[1]) <= 3) {
			/* Only port number, add linux TTY */
			strcpy(g_x_args.sz_tty_name, "/dev/ttyS");
			strncat(g_x_args.sz_tty_name, argv[1], 255 - sizeof("/dev/ttyS"));
		} else {
			strcpy(g_x_args.sz_tty_name, argv[1]);
		}

		if (argc == 3) {
			tcp_port = atoi(argv[2]);
			PRINTF(PRINT_INFO, "CONNECTING TO: %s\r\n", g_x_args.sz_tty_name);
			PRINTF(PRINT_INFO, "SERVING AT TCP PORT:    %u\r\n\r\n", tcp_port);
		} else {
			g_x_args.ui_server_port = atoi(argv[2]);
			tcp_port = atoi(argv[3]);
			PRINTF(PRINT_INFO, "CONNECTING TO: %s:%u\r\n", g_x_args.sz_tty_name, g_x_args.ui_server_port);
			PRINTF(PRINT_INFO, "SERVING AT TCP PORT:    %u\r\n\r\n", tcp_port);
		}
	} else {
		PRINTF(PRINT_ERROR, "\r\ng3_proxy.c : Wrong arguments. Usage:\r\n");
		PRINTF(PRINT_ERROR, "  g3_proxy.c <tty_file> <tcp_port_to_serve> : Example: g3_proxy.exe /dev/ttyS31 20001\r\n");
		PRINTF(PRINT_ERROR, "  g3_proxy.c <serial_port_number> <tcp_port_to_serve> : Example: g3_proxy.exe 31 20001\r\n");
		PRINTF(PRINT_ERROR, "  g3_proxy.c <server_ip_address> <server_tcp_port> <tcp_port_to_serve> : Example: g3_proxy.exe 127.0.0.1 20000 20001\r\n\r\n");
		return -1;
	}

	adp_mac_serial_if_init();

	/* Open TCP server socket */
	i_server_sd =  open_server(tcp_port);
	if (i_server_sd == 0) {
		PRINTF(PRINT_ERROR, "Cannot open Server socket.");
		exit(-1);
	} else {
		PRINTF(PRINT_INFO, "Proxy launched.\n");
	}

	/* Init USI/Base serial connection */
	addUsi_Init();

	/* Check USI file desc */
	if (g_usi_fd <= 0) {
		PRINTF(PRINT_ERROR, "Cannot open Usi TTY.");
		exit(-1);
	} else {
		PRINTF(PRINT_INFO, "USI TTY ready. \n");
	}

	/* Prepare list of file descriptor to monitor with "select" */
	FD_ZERO(&g_master_set);
	g_max_fd = (g_usi_fd > i_server_sd) ? g_usi_fd : i_server_sd;

	FD_SET(g_usi_fd, &g_master_set);
	FD_SET(i_server_sd, &g_master_set);

	while (1) {
		int i_sel_ret;

		memcpy(&g_working_set, &g_master_set, sizeof(g_master_set));

		timeout.tv_sec  = 0;
		timeout.tv_usec = 20000;

		/* Wait on file descriptors for data available */
		i_sel_ret = select(g_max_fd + 1, &g_working_set, NULL, NULL, &timeout);

		if (i_sel_ret < 0) {
			PRINTF(PRINT_ERROR, "Error. Select failed\n");
			/* Force close TCP connection */
			if (g_concentrator_fd != 0) {
				close(g_concentrator_fd);
			}

			close(g_usi_fd);
			close(i_server_sd);
			return -1;
		} else if (i_sel_ret == 0) {
			/* Tick-Tack... */
			/* PRINTF(PRINT_INFO,"."); */
			/* Process USI */
			addUsi_Process();
		} else {
			/* process file descriptors */
			for (i = 0; i <= g_max_fd  &&  i_sel_ret > 0; ++i) {
				if (FD_ISSET(i, &g_working_set)) {
					if (i == g_usi_fd) {
						/* Process USI */
						addUsi_Process();
					} else if (i == g_concentrator_fd) {
						int n = 0;

						/* check if socket is valid for read */
						ioctl(g_concentrator_fd, FIONREAD, &n);
						if (n == 0) {
							/* Socket has been closed... */
							PRINTF(PRINT_ERROR, "Connection Error, closing....\n");
							/* Error, close concentrator socket */
							close(g_concentrator_fd);

							/* Clear concentrator socket from select list */
							FD_CLR(g_concentrator_fd, &g_master_set);
							if (g_concentrator_fd == g_max_fd) {
								g_max_fd--;
							}

							g_concentrator_fd = 0;
						}

						/* Process concentrator inputs*/
						usi_cli_RxProcess();
					} else if (i == i_server_sd) {
						/* Handle incoming connections from concentrators*/
						if (g_concentrator_fd == 0) {
							/* int i_flag; */
							/* Ready for new concentrator connection, accept */
							g_concentrator_fd = accept(i_server_sd, NULL, NULL);
							PRINTF(PRINT_INFO, "New Connection established.\n");

							/* Configure socket for miminum delay */
							/* i_flag = 1; */
							/* setsockopt(g_concentrator_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &i_flag, sizeof(int)); */

							/*add new socket to select list */
							g_max_fd = (g_concentrator_fd > g_max_fd) ? g_concentrator_fd : g_max_fd;
							FD_SET(g_concentrator_fd, &g_master_set);
						} else {
							/* We do not accept more than one concentrator... */
							/* close inmediately next connections */
							PRINTF(PRINT_INFO, "Busy, concentrator connected already.\n");
							int fd = accept(i_server_sd, NULL, NULL);
							if (fd) {
								close(fd);
							} else {
								/*Error in server socket, exit! */
								PRINTF(PRINT_ERROR, "ERROR in SERVER socket, exit!");
								if (g_concentrator_fd != 0) {
									close(g_concentrator_fd);
								}

								close(g_usi_fd);
								close(i_server_sd);
								return -2;
							}
						}
					}
				}
			} /* end for*/
		}
	} /* while server file descriptor ok...*/
	return 0;
}
