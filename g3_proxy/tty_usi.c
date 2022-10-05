/**
 * \file
 *
 * \brief USI Host - TTY interface (Unix-like serial port management)
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
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <termios.h> /* POSIX terminal control definitions */

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

#include "debug.h"
#include "g3proxy.h"

extern td_x_args g_x_args;

int _open_tty_serial(char *_sz_port, unsigned int _ui_speed);

extern int g_usi_fd;
extern fd_set g_master_set, g_working_set;
extern int g_max_fd;

bool is_serial;

/* @brief	Initialize USI ports
 *
 */
void addUsi_InitPorts(void)
{
	return;
}

/**@brief Open communication port (parameters are defined in PrjCfg.h)
  @param port_type: Port Type (UART_TYPE, USART_TYPE, COM_TYPE)
  @param commPort: Physical Communication Port
  @param speed:Communication speed
  @return 0 on success.
*/

int8_t addUsi_Open(uint8_t port_type, uint8_t port, uint32_t bauds)
{
	struct sockaddr_in serv_addr;
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(g_x_args.ui_server_port);

	/* Check if tty name is realy an IP address. Then Convert IPv4 and IPv6 addresses from text to binary form */
	if (inet_pton(AF_INET, g_x_args.sz_tty_name, &serv_addr.sin_addr) <= 0) {
		/* Not an IP address. Use tty name as serial port */
		is_serial = true;
		/* Ignore Port/Bauds hardcoded parameters, use global parameter configuration */
		g_usi_fd = _open_tty_serial(g_x_args.sz_tty_name, g_x_args.ui_baudrate);
	} else {
		is_serial = false;
		int i_flag = 1;
		if ((g_usi_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printf("\n Socket creation error \n");
			return -1;
		}

		/* Allow socket descriptor to be reuseable                   */
		setsockopt(g_usi_fd, SOL_SOCKET, SO_REUSEADDR, &i_flag, sizeof(int));
		/* i_flag = 1; */
		/* Configure socket for miminum delay */
		/* setsockopt(g_usi_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &i_flag, sizeof(int)); */

		if (connect(g_usi_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
			printf("\nConnection Failed \n");
			return -1;
		}
	}

	return g_usi_fd;
}

/*
 *  @brief	Close the serial port
 *
 *   @param	port		Used for communications
 *   @return		0 Success
 *   -1 Error
 */

int addUsi_Close(int bauds)
{
	close(g_usi_fd);
	return 0;
}

/**@brief Transmit a message through the interface
  @param port: port number configured in PrjCfg.h
  @param msg: buffer holding the message
  @param msglen:buffer length
  @return the number of bytes written.
*/
uint16_t addUsi_TxMsg(uint8_t port_type, uint8_t port, uint8_t *sz_msg, uint16_t i_msglen)
{
	/*
	 * PRINTF(PRINT_INFO, "\r\n*** SENT    = ");
	 * for(int i = 0; i < i_msglen; i++){
	 *      PRINTF(PRINT_INFO, "%02X:", sz_msg[i]);
	 * }
	 * PRINTF(PRINT_INFO, "\r\n");
	 */
	if (is_serial) {
		/* write buffer to tty */
		return write(g_usi_fd, sz_msg, i_msglen);
	} else {
		/* write buffer to socket */
		return send(g_usi_fd, sz_msg, i_msglen, 0 );
	}
}

/**@brief Read char from port
  @param port: port number configured in PrjCfg.h
  @param c: pointer to a character
  @return 0 if a charater has been read, otherwise -1.
*/
int8_t addUsi_RxChar(uint8_t port_type, uint8_t port, uint8_t *c)
{
	int n = 0;

	if (!is_serial) {
		/* check if socket is valid for read */
		ioctl(g_usi_fd, FIONREAD, &n);
		if (n == 0) {
			return -1;
		}
	}

	/* read one byte from tty*/
	int i_recv = read(g_usi_fd, c, 1);
	if (i_recv > 0) {
		return 0;
	}

	return -1;
}

/*
 * @brief	Open TTY serial.
 * @param	_sz_port	tty to connect to.
 * @param	_ui_speed	baudrate configuration.
 * @return	on success, returs the file descriptor representing this tty.
 *                      -1 otherwise
 *
 */
int _open_tty_serial(char *_sz_port, unsigned int _ui_speed)
{
	/* structure to store the port settings in */
	struct termios port_settings;
	unsigned int ui_br = B230400;

	/* open device file */
	int fd = open(_sz_port, O_RDWR | O_NDELAY | O_NONBLOCK);

	if (fd == -1) { /* if open is unsucessful */
		PRINTF(PRINT_ERROR, "Open_port: Unable to open %s \n", _sz_port);
		return -1;
	} else {
		/* fcntl(fd, F_SETFL, O_NONBLOCK); */
		PRINTF(PRINT_INFO, "Tty port is open.\n");
	}

	/*Check valid baudrates */
	if (_ui_speed == 115200) {
		ui_br = B115200;
	} else if (_ui_speed == 230400) {
		ui_br = B230400;
	} else if (_ui_speed == 57600) {
		ui_br = B57600;
	} else if (_ui_speed == 38400) {
		ui_br = B38400;
	} else if (_ui_speed == 19200) {
		ui_br = B19200;
	} else {
		PRINTF(PRINT_ERROR, "Invalid Baudrate.\n");
		return -1;
	}

	memset(&port_settings, 0, sizeof(port_settings));

	/* set baud rates */
	cfsetispeed(&port_settings, ui_br);
	cfsetospeed(&port_settings, ui_br);

	port_settings.c_iflag = 0;
	port_settings.c_oflag = 0;
	port_settings.c_lflag = 0;
	port_settings.c_cflag &= ~PARENB;    /* set no parity, stop bits, data bits */
	port_settings.c_cflag &= ~CSTOPB;
	port_settings.c_cflag &= ~CSIZE;
	port_settings.c_cflag |= CS8;
	port_settings.c_cflag |= CREAD;
	port_settings.c_cflag |= CLOCAL; /* 8n1, see termios.h for more information */

	/* apply the settings to the port */
	tcsetattr(fd, TCSANOW, &port_settings);
	tcflush(fd, TCIOFLUSH);
	return(fd);
}
