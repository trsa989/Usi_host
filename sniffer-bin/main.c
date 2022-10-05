#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
#include "addUsi.h"

#include "mngLayerHost.h"
#include "prime_api_host.h"
#include "prime_api_defs_host.h"
#include "cl_432_defs.h"
#include "ifacePrimeSniffer.h"

#define ATMEL_LOG_MAGIC_LEN 8
const unsigned char c_puc_atmel_log_magic_number[ATMEL_LOG_MAGIC_LEN ] = {0x41, 0x54, 0x50, 0x4c, 0x53, 0x46, 0x0, 0x01};

td_x_args g_x_args = { FALSE,
		       0,
		       "127.0.0.1",
		       13000,
		       "/tmp/sniffer.bin"};

/* USI file descriptor */
int g_usi_fd = 0;
/* Log file descriptor */
int g_log_fd = 0;

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
			{"host", required_argument, 0, 'h'},
			{"port", required_argument, 0, 'p'},
			{"file", required_argument, 0, 'f'},
			{"verbose-level", required_argument, 0, 'v'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long(argc, argv, "p:f:h:v:", long_options, &option_index);

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

		case 'f':
		{
			PRINTF(PRINT_INFO, "Option -f  [file] with value `%s'\n", optarg);
			strncpy(g_x_args.sz_file_bin, optarg, 255);

			break;
		}

		case 'p':
		{
			PRINTF(PRINT_INFO, "Option -p  [port] with value `%s'\n", optarg);

			if (getParseInt(optarg, (int *)&g_x_args.ui_port) != 0) {
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

		case 'h':
		{
			PRINTF(PRINT_INFO, "Option -s  [host] with value `%s'\n", optarg);
			strncpy(g_x_args.sz_host_name, optarg, 255);
			break;
		}

		case '?':
			/* getopt_long already printed an error message. */
			return -1;

			break;
		}
	}
	return 0;
}

int add_escape_sequences(unsigned char *data, int len)
{
	unsigned char tail[2000];
	int i = 0;

	for (i = 0; i < len; i++) {
		if (data[i] == 0x7E) {
			/* copy tail... */
			memcpy(tail, data + i + 1, len - i);

			/* add escape sequence */
			data[i] = 0x7D;
			data[i + 1] = 0x20 ^ 0x7E;

			/* copy back tail msg */
			memcpy(data + i + 2, tail, len - i);
			/* update len */
			len++;
			i++;
		} else if (data[i] == 0x7D) {
			/* copy tail... */
			memcpy(tail, data + i + 1, len - i);

			/* add escape sequence */
			data[i] = 0x7D;
			data[i + 1] = 0x20 ^ 0x7D;

			/* copy back tail msg */
			memcpy(data + i + 2, tail, len - i);
			/* update len */
			len++;
			i++;
		}
	}

	return len;
}

int add_7es(unsigned char *data, int len)
{
	unsigned char tail[2000];

	memcpy(tail, data, len );
	data[0] = 0x7E;
	memcpy(data + 1, tail, len );
	data[len + 1] = 0x7E;

	return len + 2;
}

int remove_escape_sequences(uint8_t *ptrMsg, uint16_t len)
{
	unsigned char tail[2000];
	int i = 0;

	for (i = 0; i < len; i++) {
		if (ptrMsg[i] == 0x7D) {
			/* copy tail... */
			memcpy(tail, ptrMsg + i + 2, len - i - 1 );
			if (ptrMsg[i + 1] == 0x5E) {
				/* remove escape sequence */
				ptrMsg[i] = 0x7E;
				/* copy back tail msg */
				memcpy(ptrMsg + i + 1, tail, len - i - 1);
				/* update len */
				len--;
			} else if (ptrMsg[i + 1] == 0x5D) {
				ptrMsg[i] = 0x7D;
				memcpy(ptrMsg + i + 1, tail, len - i - 1);
				len--;
			} else {
				return 0; /* error! */
			}
		}
	}
	return len;
}

int  remove_7es(uint8_t *ptrMsg, uint16_t len)
{
	uint8_t *temp[2000];

	if ((len == 0) || (len > 2000)) {
		return 0;
	}

	memcpy(temp, ptrMsg, len);

	memcpy(ptrMsg, ptrMsg + 1, len - 2);
	ptrMsg[len - 1] = 0;

	return len - 2;
}

long long get_timestamp_miliseconds_since_epoc()
{
	long long time_in_ms;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	time_in_ms = ((1000000 * tv.tv_sec) + tv.tv_usec) / 1000;
	return time_in_ms;
}

uint16_t eval_crc16(uint8_t *bufPtr, unsigned len)
{
	static const uint16_t crc16Table[256] = {
		0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
		0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
		0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
		0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
		0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
		0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
		0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
		0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
		0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
		0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
		0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
		0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
		0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
		0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
		0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
		0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
		0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
		0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
		0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
		0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
		0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
		0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
		0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
		0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
		0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
		0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
		0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
		0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
		0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
		0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
		0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
		0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
	};
	unsigned short crc = 0;

	while (len--) {
		crc = crc16Table [(crc >> 8) & 0xff] ^ (crc << 8) ^ (*bufPtr++ & 0x00ff);
	}

	return crc;
}

int  add_crc16(uint8_t *data, int len)
{
	unsigned int crc = eval_crc16(data, len);

	data[len] = (crc >> 8) & 0xFF;
	data[len + 1] = crc & 0xFF;

	return len + 2;
}

void print_ASCII(uint8_t *bufPtr, int iLen)
{
	int i = 0;

	if (g_x_args.i_verbose_level > 0) {
		return; /* if not verbose, do not print */
	}

	for (i = 0; i < iLen; i++) {
		printf("%.2X ", bufPtr[i] );
		if ((i % 16) == 0x0F) {
			printf("\n");
		}
	}
	printf("\n");
}

void save_sniffer_msg_cb(uint8_t *ptrMsg, uint16_t len)
{
	int length;
	long long timeStamp;

	print_ASCII(ptrMsg, len);

	/* Now, we a data frame stored in ptrMsg... */
	/* Set "embedded timestamp bit in frame" */
	ptrMsg[4] |= 0x40;

	/* Add 64bit timestamp. */

	timeStamp = get_timestamp_miliseconds_since_epoc();
	ptrMsg[20] =  (timeStamp & 0xFF);
	timeStamp >>= 8;
	ptrMsg[19] =  (timeStamp & 0xFF);
	timeStamp >>= 8;
	ptrMsg[18] = timeStamp & 0xFF;
	timeStamp >>= 8;
	ptrMsg[17] = timeStamp & 0xFF;
	timeStamp >>= 8;
	ptrMsg[16] = timeStamp & 0xFF;
	timeStamp >>= 8;
	ptrMsg[15] = timeStamp & 0xFF;
	timeStamp >>= 8;
	ptrMsg[14] = timeStamp & 0xFF;
	timeStamp >>= 8;
	ptrMsg[13] = timeStamp & 0xFF;

	/* USI Sniffer uses CRC 16-bit */
	/* overwrite new crc */
	length = add_crc16(ptrMsg, len - 2);

	/* add escape characters */
	length = add_escape_sequences(ptrMsg, length);

	/* add 7Es */
	length = add_7es(ptrMsg, length);
	print_ASCII(ptrMsg, length);
	/* save to file */
	write(g_log_fd, ptrMsg, length);

	return;
}

int main(int argc, char **argv)
{
	int i;
	fd_set master_set, working_set;
	int i_max_fd;
	struct timeval timeout;

	/* load command line parameters */
	if (parse_arguments(argc, argv) < 0) {
		printf("Sniffer-Bin log v%d,%d\n", 0, 1 /*VERSION_HI,VERSION_LOW*/);
		printf("Error, check arguments.\n");
		printf("Usage: dlsmotcp [--verborse|silent] [OPTIONS]\n");
		printf("\t-h host       : IP Address to connect, default : 127.0.0.1 \n");
		printf("\t-p port       : TCP Port, default 13000 \n");
		printf("\t-f file       : output file name, default 'sniffer.bin'\n");
		exit(-1);
	}

	/* Init output file */
	g_log_fd = open(g_x_args.sz_file_bin, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
	if (g_log_fd <= 0) {
		perror("Cannot open log file");
		return -1;
	}

	/*Write Atmel Magic ID*/
	int wc = write(g_log_fd, c_puc_atmel_log_magic_number, ATMEL_LOG_MAGIC_LEN);
	if (wc < ATMEL_LOG_MAGIC_LEN) {
		perror("Cannot initialize the file");
		return -1;
	}

	/* Init USI/Base serial connection */
	addUsi_Init();
	PRINTF(PRINT_ERROR, "USI INIT OK.");

	/* Configure Sniffer callback */
	prime_sniffer_set_cb(save_sniffer_msg_cb);

	/* Check USI file desc */
	if (g_usi_fd <= 0) {
		PRINTF(PRINT_ERROR, "Cannot open Usi TTY.");
		exit(-1);
	} else {
		PRINTF(PRINT_INFO, "USI Connected.\n");
	}

	/* Prepare list of file descriptor to monitor with "select" */
	FD_ZERO(&master_set);
	i_max_fd = g_usi_fd;

	FD_SET(g_usi_fd, &master_set);

	while (1) {
		int i_sel_ret;

		memcpy(&working_set, &master_set, sizeof(master_set));

		timeout.tv_sec  = 1;
		timeout.tv_usec = 0;

		/* Wait on file descriptors for data available */
		i_sel_ret = select(i_max_fd + 1, &working_set, NULL, NULL, &timeout);

		if (i_sel_ret < 0) {
			PRINTF(PRINT_ERROR, "Error. Select failed\n");
			close(g_usi_fd);
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
					}
				}
			} /* end for*/
		}
	} /* while server file descriptor ok...*/
	return 0;
}
