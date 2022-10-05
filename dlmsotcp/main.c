/**
 * \file
 *
 * \brief USI Host - DLMS Over TCP
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

#include "debug.h"

#include "dlmsotcp.h"
#include "addUsi.h"

#define TRUE  1
#define FALSE 0

td_x_args g_x_args = { FALSE,
		       0,
		       "/dev/ttyUSB0",
		       115200,
		       4059};

int g_usi_fd = 0;
int g_concentrator_fd = 0;

int getParseInt(char *_szStr, int *_iVal)
{
	char *endptr;
	long tmp;

	errno = 0;    /* To distinguish success/failure after call */
	tmp = strtol(_szStr, &endptr, 10);

	/* Check for various possible errors */
	if ((errno == ERANGE && (tmp == LONG_MAX || tmp == LONG_MIN)) ||
			(errno != 0 && tmp == 0)) {
		return -1;
	}

	if (endptr == _szStr) {
		return -1;
	}

	/* If we got here, strtol() successfully parsed a number */
	*_iVal = (int)tmp;

	return 0;
}

int parse_arguments(int argc, char **argv)
{
	int c;

	while (1) {
		static struct option long_options[] = {
			{"verbose", no_argument, &g_x_args.i_verbose, 1},
			{"silent", no_argument, &g_x_args.i_verbose, 0},
			{"tty", required_argument, 0, 't'},
			{"baudrate", required_argument, 0, 'b'},
			{"server", required_argument, 0, 's'},
			{"verbose-level", required_argument, 0, 'v'},
			{0, 0, 0, 0}
		};

		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long(argc, argv, "t:b:s:h:v:", long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1) {
			break;
		}

		switch (c) {
		case 0:
			/* If this option set a flag, do nothing else now. */
			if (long_options[option_index].flag != 0) {
				break;
			}

			PRINTF(PRINT_WARN, "Option %s", long_options[option_index].name);
			if (optarg) {
				PRINTF(PRINT_WARN, " with arg %s", optarg);
			}

			PRINTF(PRINT_WARN, "\n");
			break;

		case 's':
		{
			PRINTF(PRINT_INFO, "Option -s  [server] with value `%s'\n", optarg);

			if (getParseInt(optarg, (int *)&g_x_args.ui_server_port) != 0) {
				PRINTF(PRINT_ERROR, "Error parsing integer: %s\n", optarg);
				return -1;
			}

			if ((g_x_args.ui_server_port > 0xFFFF) || (g_x_args.ui_server_port <= 1024)) {
				PRINTF(PRINT_ERROR, "Server port not valid [1024-65535].");
				return -1;
			}

			break;
		}

		case 'b':
		{
			PRINTF(PRINT_INFO, "Option -b  [baudrate] with value `%s'\n", optarg);

			if (getParseInt(optarg, (int *)&g_x_args.ui_baudrate) != 0) {
				PRINTF(PRINT_ERROR, "Error parsing integer: %s\n", optarg);
				return -1;
			}

			break;
		}

		case 'v':
		{
			if (getParseInt(optarg, &g_x_args.i_verbose_level) != 0) {
				PRINTF(PRINT_INFO, "Option -v  [verbose level] with value `%s'\n", optarg);
				PRINTF(PRINT_ERROR, "Error parsing integer: %s", optarg);
				return -1;
			}

			if (g_x_args.i_verbose_level < 0) {
				PRINTF(PRINT_INFO, "Option -v  [verbose level] with value `%s'\n", optarg);
				PRINTF(PRINT_INFO, "Verbose level not valid [ 0 - 3 ]");
				return -1;
			}

			break;
		}

		case 't':
		{
			PRINTF(PRINT_INFO, "Option -t  [tty] with value `%s'\n", optarg);
			strncpy(g_x_args.sz_tty_name, optarg, 255);
			break;
		}

		case 'h':
		case '?':
			/* getopt_long already printed an error message. */
			return -1;

			break;
		}
	}
	return 0;
}

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
	i_rc = setsockopt(i_listen_sd, SOL_SOCKET, SO_REUSEADDR,
			&i_on, sizeof(i_on));
	if (i_rc < 0) {
		PRINTF(PRINT_ERROR, "setsockopt() failed");
		close(i_listen_sd);
		exit(-1);
	}

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
	fd_set master_set, working_set;
	int i_max_fd;
	struct timeval timeout;

	/* load command line parameters */
	if (parse_arguments(argc, argv) < 0) {
		printf("DLMSoTCP Gateway v%d,%d\n", VERSION_HI, VERSION_LOW);
		printf("Error, check arguments.\n");
		printf("Usage: dlsmotcp [--verborse|silent] [OPTIONS]\n");
		printf("\t-t tty       : tty device connecting to a base node, default: /dev/ttyUSB0 \n");
		printf("\t-b baudrate  : tty baudrate configuration, default: 115200\n");
		printf("\t-s server    : TCP server port, default ,4059\n");
		exit(-1);
	}

	/* Open TCP server socket */
	i_server_sd =  open_server(g_x_args.ui_server_port);
	if (i_server_sd == 0) {
		PRINTF(PRINT_ERROR, "Cannot open Server socket.");
		exit(-1);
	} else {
		PRINTF(PRINT_INFO, "DLMSoTCP server up. \n");
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

	/* Init dlmsotcp app */
	dlmsotcp_init();

	/* Prepare list of file descriptor to monitor with "select" */
	FD_ZERO(&master_set);
	i_max_fd = (g_usi_fd > i_server_sd) ? g_usi_fd : i_server_sd;

	FD_SET(g_usi_fd, &master_set);
	FD_SET(i_server_sd, &master_set);

	while (1) {
		int i_sel_ret;

		memcpy(&working_set, &master_set, sizeof(master_set));

		timeout.tv_sec  = 1;
		timeout.tv_usec = 0;

		/* Wait on file descriptors for data available */
		i_sel_ret = select(i_max_fd + 1, &working_set, NULL, NULL, &timeout);

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
			PRINTF(PRINT_INFO, ".");
			/* Process USI */
			addUsi_Process();
		} else {
			/* process file descriptors */
			for (i = 0; i <= i_max_fd  &&  i_sel_ret > 0; ++i) {
				if (FD_ISSET(i, &working_set)) {
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
							FD_CLR(g_concentrator_fd, &master_set);
							if (g_concentrator_fd == i_max_fd) {
								i_max_fd--;
							}

							g_concentrator_fd = 0;

							/* Force 432 connection close for current communications */
							dlmsotcp_close_432();
						}

						/* Process concentrator inputs*/
						dlmsotcp_process();
					} else if (i == i_server_sd) {
						/* Handle incoming connections from concentrators*/
						if (g_concentrator_fd == 0) {
							int i_flag;
							/* Ready for new concentrator connection, accept */
							g_concentrator_fd = accept(i_server_sd, NULL, NULL);
							PRINTF(PRINT_INFO, "New Connection established.\n");

							/* Configure socket for miminum delay */
							i_flag = 1;
							setsockopt(g_concentrator_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&i_flag, sizeof(int));

							/*add new socket to select list */
							i_max_fd = (g_concentrator_fd > i_max_fd) ? g_concentrator_fd : i_max_fd;
							FD_SET(g_concentrator_fd, &master_set);
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
