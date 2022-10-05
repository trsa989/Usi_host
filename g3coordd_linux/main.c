/**
 * \file
 *
 * \brief G3 Coordinator App
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
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
/* #include <linux/in.h> */
#include <linux/if.h>
#include <linux/if_tun.h>
/* #include <netinet/in.h> */
#include <poll.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <arpa/inet.h>

#include <signal.h>
#include <unistd.h> /* usleep */
#include <time.h> /* usleep */
#include <pthread.h>

#include <limits.h>

#include "Config.h"

#include "globals.h"

#include "mac_wrapper.h"
#include "AdpApi.h"
#include "AdpApiTypes.h"
#include "gpio.h"
#include "led.h"
#include "udp_responder.h"
#include "app_adp_mng.h"
#include "Logger.h"
#include "tun.h"

const unsigned char band_str[4][12] = {"CENELEC_A", "CENELEC_B", "FCC", "ARIB"};

#define COMMS_FD 0
#define POLLTIMEOUT 20 /* ms */

/* Default Configuration */
struct st_configuration g_st_config = {
	.b_verbose     = 1,
	.b_conformance = 0, 
	.uc_band       = ADP_BAND_CENELEC_A,
	.us_pan_id     = 0x781D,
	.sz_ipaddress  = "FD00:0:2:781D:1122:3344:5566:0000",
	.uc_prefix_len = 64,
	.sz_tun_name   = "g3plc",
	.sz_tty_name   = "/dev/ttyS1",
	.sz_tty_speed  = 230400,
	.sz_hostname   = "127.0.0.1",
	.sz_tcp_port   = 3000,
	.sz_port_type  = 0,
	.uc16_psk_key  = CONF_PSK_KEY,
	.uc16_gmk_key  = CONF_GMK_KEY,
};

/* TUN file descriptor */
int tunfd;

uint32_t g_ui_read_tun = 1;

int g_usi_fd = 0;

#define MIKROBUS1 0
#define MIKROBUS2 1
#define MIKROBUS1_GPIO_RESET "PB2"  /* Reset for mikroBUS 1 is mapped on PB2  */
#define MIKROBUS2_GPIO_RESET "PA26" /* Reset for mikroBUS 2 is mapped on PA26 */

/*******************************************************/

/**
 * \brief Resets G3 adp_mac_serialized Modem
 * \return 0
 *
 *******************************************************/
void reset_g3_adp_modem(int mikroBUS)
{
	char gpioname[8];

	memset(gpioname, '0', 8);
	if (MIKROBUS1 == mikroBUS) {
		strncpy(gpioname, MIKROBUS1_GPIO_RESET, 8);
	} else {
		strncpy(gpioname, MIKROBUS2_GPIO_RESET, 8);
	}

	SET_GPIO(gpioname, 0);
	sleep(1);
	SET_GPIO(gpioname, 1);
	sleep(1);
}

/*******************************************************/

/**
 * \brief Pthread to handle usi_process
 * usi_process needs to be called periodically to transmit/receive
 * USI Frames to/from serial port -
 * \return 0
 *
 *******************************************************/
void usi_process_thread()
{
	while (1) {
		addUsi_Process();
		usleep(1000);
	}
	pthread_exit(NULL);
}

/*******************************************************/

/**
 * \brief Handling signals
 *
 * \return 0
 *
 *******************************************************/
void app_g3_coordinator_signals_handler(int signum)
{
	if ((SIGINT == signum) || (SIGHUP == signum) || (SIGKILL == signum)) {
		/* Ctrl+C Signal */
		LOG_ERR(Log("I die...\r\n"));
		/* Clean exit should be written */
		exit(0);
	}

	return;
}

static int parse_interger(char *_szStr, int *_iVal, int base)
{
	char *endptr;
	long tmp;

	errno = 0;    /* To distinguish success/failure after call */
	tmp = strtol(_szStr, &endptr, base);

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

void app_g3_coordinator_print_help()
{
	printf("g3coordd [options]\r\n");
	printf("\t--verbose,silent: enable/dissable debug messages on STDOUT. Default: silent\r\n");
	printf("\t-b, --band: PLC Band [CENELEC_A|CENELEC_B|FCC|ARIB]. Default value: CENELEC_A\r\n");
	printf("\t-i, --ip: Coordinator's IPv6 address, default value: FD00:0:2:781D::1122:3344:5566:0000\r\n");
	printf("\t-l, --length: Network prefix length(bits). Default value: 64\r\n");
	printf("\t-p, --PANDID: Coordinator PAN Identifier. Default value: 0x781D\r\n");
	printf("\t-t, --tun: Tun device name. Default value: \"g3plc\"\r\n");
	printf("\t-d, --dev: Serial Port device connected to Adp Mac Serialized device\r\n");
	printf("\t-s, --speed: Serial Port Speed for device connected to Adp Mac Serialized device\r\n");
	printf("\t-n, --hostname: Hostname connected to Adp Mac Serialized device\r\n");
	printf("\t-o, --port: TCP Port for connection to Adp Mac Serialized device\r\n");
	printf("\t-h, --help: print this help.\r\n");
}

static int app_g3_coordinator_parse_arguments(int argc, char **argv)
{
	int c;
	int value;
	struct in6_addr result;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"verbose", no_argument, &g_st_config.b_verbose, 1},
			{"silent", no_argument, &g_st_config.b_verbose, 0},
			/*				{"conformance", no_argument,&g_st_config.b_conformance, 1}, */
			{"band", required_argument, 0, 'b'},
			{"ip", required_argument, 0, 'i'},
			{"length", required_argument, 0, 'l'},
			{"PANID", required_argument, 0, 'p'},
			{"tun", required_argument, 0, 't'},
			{"dev", required_argument, 0, 'd'},
			{"speed", required_argument, 0, 's'},
			{"hostname", required_argument, 0, 'n'},
			{"port", required_argument, 0, 'o'},
			{"help", required_argument, 0, 'h'},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "n:o:b:i:l:p:t:d:s:h", long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1) {
			return 0;
		}

		switch (c) {
		case 0:
			/* If this option set a flag, do nothing else now. */
			if (long_options[option_index].flag != 0) {
				break;
			}

			LOG_DBG(Log("Option %s", long_options[option_index].name));
			if (optarg) {
				LOG_DBG(Log(" with arg %s\n", optarg));
			}

			break;

		case 'h':
			app_g3_coordinator_print_help();
			exit(0);
			break;

		case 'b':
			LOG_INFO(Log("Option -b  [BAND] with value `%s'", optarg));
			if ((strncmp("CENELEC_A", optarg, 9 ) == 0) || (strncmp("CEN_A", optarg, 5 ) == 0) || (strncmp("CENA", optarg, 4 ) == 0)) {
				g_st_config.uc_band = ADP_BAND_CENELEC_A;
			} else if ((strncmp("CENELEC_B", optarg, 9 ) == 0) || (strncmp("CEN_B", optarg, 5 ) == 0) || (strncmp("CENB", optarg, 4 ) == 0)) {
				g_st_config.uc_band = ADP_BAND_CENELEC_B;
			} else if (strncmp("FCC", optarg, 3 ) == 0) {
				g_st_config.uc_band = ADP_BAND_FCC;
			} else if (strncmp("ARIB", optarg, 4 ) == 0) {
				g_st_config.uc_band = ADP_BAND_ARIB;
			} else {
				LOG_ERR(Log("PLC BAND not recognized"));
				return -1;
			}

			break;

		case 'i':
			LOG_INFO(Log("Option -i [IP6] with value `%s'", optarg));
			if (inet_pton(AF_INET6, optarg, &result) != 1) {
				LOG_ERR(Log("INVALID IP6 Address %s", optarg));
				return -1;
			}

			strcpy(g_st_config.sz_ipaddress, optarg);
			break;

		case 'l':
			LOG_INFO(Log("Option -l [prefix] with value `%s'", optarg));
			if (parse_interger(optarg, (int *)&value, 10) != 0) {
				LOG_ERR(Log("Error parsing length value: %s", optarg));
				return -1;
			}

			if ((value < 8) || (value > 127)) {
				LOG_ERR(Log("Net prefix length  not valid. It must be in range [8 128]."));
				return -1;
			}

			g_st_config.uc_prefix_len = value;
			break;

		case 'p':
			LOG_INFO(Log("Option -p [PANID] with value `%s'", optarg));
			if (parse_interger(optarg, (int *)&value, 10) != 0) {
				LOG_ERR(Log("Error parsing PANID value: %s", optarg));
				return -1;
			}

			if (value == 0) {
				/* Try parse in hex format */
				if (parse_interger(optarg, (int *)&value, 0) != 0) {
					LOG_ERR(Log("Error parsing HEX PANID value: %s", optarg));
					return -1;
				}
			}

			if ((value > 0xFFFF) || ((value & 0xFCFF) != value) || (value == 0)) {
				/* In order to avoid the possibility of duplicated IPv6 addresses, the values
				 *       of the PAN ID MUST be chosen so that the 7th (Universal/Local bit) and 8th
				 *             (Individual/Group bit) bits of the first byte of the PAN ID are both zero */
				LOG_ERR(Log("Invalid PANID:  %d(%04X)", value, value));
				return -1;
			}

			g_st_config.us_pan_id = value;
			break;

		case 't':
			LOG_INFO(Log("Option -t [TUN_NAME] with value `%s'", optarg));
			if (strlen(optarg) > 32) {
				LOG_ERR(Log("tun name too large: %s", optarg));
				return -1;
			}

			strcpy(g_st_config.sz_tun_name, optarg);
			break;

		case 'd':
			LOG_INFO(Log("Option -d [DEVICE] with value `%s'", optarg));
			if (strlen(optarg) > 32) {
				LOG_ERR(Log("Serial Port name too large: %s", optarg));
				return -1;
			}

			strcpy(g_st_config.sz_tty_name, optarg);
			break;

		case 's':
			LOG_INFO(Log("Option -s [SPEED] with value `%s'", optarg));
			if (parse_interger(optarg, (int *)&value, 10) != 0) {
				LOG_ERR(Log("Error parsing Serial Port Speed value: %s", optarg));
				return -1;
			}

			g_st_config.sz_tty_speed = value;
			break;

		case 'n':
			/*! Usefull when using socat to redirect serial port to tcp port. For example: */
			/*! socat -d -x TCP-LISTEN:3000,reuseaddr /dev/ttyS1,B230400,raw,echo=0 */
			LOG_INFO(Log("Option -n [HOSTNAME] with value `%s'", optarg));
			if (strlen(optarg) > 32) {
				LOG_ERR(Log("Hostname too large: %s", optarg));
				return -1;
			}

			strcpy(g_st_config.sz_hostname, optarg);
			break;

		case 'o':
			LOG_INFO(Log("Option -o [PORT] with value `%s'", optarg));
			if (parse_interger(optarg, (int *)&value, 10) != 0) {
				LOG_ERR(Log("Error parsing Serial Port Speed value: %s", optarg));
				return -1;
			}

			g_st_config.sz_tcp_port = value;
			break;

		case '?':
			return -1;

		default:
			LOG_ERR(Log("OPTION not recognised: %s", optarg));
		} /* switch */
	} /* while */
	return 0;
}

static void app_g3_coordinator_print_arguments()
{
	LOG_INFO(Log(
			"\r\nG3 Coordinator Configuration\r\n \
													Verbose: %s\r\n						 \
													Band : %s\r\n							 \
													PAN ID : 0x%x\r\n					 \
													Prefix Len : %d\r\n				 \
													IP Address : %s\r\n				 \
													TUN device : %s\r\n				 \
													TTY device : %s\r\n				 \
													TTY speed : %d\r\n				 \
													PSK key : %s\r\n					 \
													GMK key    : %s\r\n"                                                                                                                                                                                                                                                                                                                                     ,
			g_st_config.b_verbose ? "ON" : "OFF", band_str[g_st_config.uc_band], g_st_config.us_pan_id, g_st_config.uc_prefix_len, g_st_config.sz_ipaddress,
			g_st_config.sz_tun_name, g_st_config.sz_tty_name, g_st_config.sz_tty_speed, g_st_config.uc16_psk_key, g_st_config.uc16_gmk_key));
}

int main(int argc, char **argv)
{
	static int num_tx = 0;
	struct pollfd poll_fds[2];
	int ret;

	SET_RGB_OFF()
	SET_LED_GREEN_TIMER(500, 500)

	/* Signals to be handled */
	signal(SIGINT, app_g3_coordinator_signals_handler);
	signal(SIGHUP, app_g3_coordinator_signals_handler);
	signal(SIGKILL, app_g3_coordinator_signals_handler);

	/* Global Configuration can be overridden with console parameters */
	if (app_g3_coordinator_parse_arguments(argc, argv) < 0) {
		LOG_ERR(Log("Error parsing arguments"));
		app_g3_coordinator_print_help();
		return -1;
	}

	/* Print Global Configuration */
	app_g3_coordinator_print_arguments();
	LogEnable(g_st_config.b_verbose);

	/* Reset ADP Modem by default */
	reset_g3_adp_modem(MIKROBUS1);

	/* Init USI/Base serial connection */
	addUsi_Init();

	/* Create usi_process thread to handle USI Frames on serial port */
	pthread_t usi_thread;
	/* Create a thread which executes addUsi_Process() periodically */
	if (pthread_create(&usi_thread, NULL, usi_process_thread, NULL)) {
		LOG_ERR(Log("Error creating USI Thread\n"));
		return -1;
	}

	/* Allocate file descriptors */
	tunfd = tun_alloc(g_st_config.sz_tun_name, IFF_TUN);

	/* Configure TUN interface */
	if (tun_configure(g_st_config.sz_tun_name, g_st_config.us_pan_id, g_st_config.sz_ipaddress, g_st_config.uc_prefix_len) != 0) {
		return -1;
	}

#ifdef APP_CONFORMANCE_TEST
	/* Create udp_responder_thread to handle Conformance UDP Responder */
	pthread_t conformance_thread;
	/* Create a thread which executes addUsi_Process() periodically */
	if (pthread_create(&conformance_thread, NULL, udp_responder_thread, (void *)&g_st_config.sz_tun_name)) {
		LOG_ERR(Log("Error creating Conformance Thread\n"));
		return -1;
	}

#endif

	/* Adp Initialization */
	adp_init(g_st_config.uc_band, g_st_config.us_pan_id, g_st_config.sz_ipaddress, g_st_config.uc_prefix_len);

	/* Print Banner */
	LOG_INFO(Log("Microchip G3 Device version %s", G3_COORD_VERSION));
	/*! Show G3 API versions */
	app_show_version();

	/* Configure POLL descriptor arguments */
	poll_fds[COMMS_FD].fd = tunfd;
	poll_fds[COMMS_FD].events = POLLIN;

	SET_LED_GREEN()

	while (1) {
		poll_fds[COMMS_FD].revents = 0;
		poll_fds[COMMS_FD].revents = 0;

		if (adp_write_buffers_available()) {
			/* PRINTF("ADP Buffers availables\r\n"); */
			poll_fds[COMMS_FD].events = POLLIN;
		} else {
			/*If there is no more buffers available in the ADP,
			 * we stop reading new packets from the tun descriptor
			 * until new buffers are available.*/
			/* LOG_DBG(Log("ADP Buffers NOT available")); */
			poll_fds[COMMS_FD].events = 0; /*  */
			continue;
		}

		ret = poll(poll_fds, 2, POLLTIMEOUT);

		/* Process USI - Do via thread */
		/* addUsi_Process(); */
		/* Process timed events */
		adp_process(g_st_config.uc_band, g_st_config.us_pan_id);

		if (ret == 0) {
			continue;
		} else if (ret < 0) {
			/* perror("POLL ERROR!"); */
			/* return -1; */
		}

		if (poll_fds[COMMS_FD].revents & POLLIN) { /* new command from tun iface */
			int len = 0;
			uint8_t buffer[2048];
			num_tx++;

			len = read(poll_fds[COMMS_FD].fd, buffer, G3_ADP_MAX_DATA_LENTHG + 4);
			/* LOG_DBG(Log("Read from TUN...%d, len %d", num_tx, len)); */
			if (len <= 0) {
				LOG_ERR(Log("Error in TUN iface"));
				return -1;
			}

			if (len <= G3_ADP_MAX_DATA_LENTHG + 4) {
				if (adp_send_ipv6_message(buffer + 4, len - 4) < 0) {
					LOG_ERR(Log("ADP Send Message error"));
				} else {
					/* LOG_DBG(Log("ADP Send Message OK, wait confirm.")); */

					/* Do not read till  TX CONFIRM */
					poll_fds[COMMS_FD].events = 0;
				}
			} else {
				LOG_ERR(Log("Error, PDU too large %d", len));
			}
		}

		/* Process timed events */
		adp_process(g_st_config.uc_band, g_st_config.us_pan_id);
		/* Process USI - Do via thread */
		/* addUsi_Process(); */
	}
}
