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

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <termios.h> /* POSIX terminal control definitions */
#include "../src/Usi.h"
#include "../src/UsiCfg.h"

#include "globals.h"

#define CONFIG_GLOBAL_FD_SERIAL_PORT
#ifdef CONFIG_GLOBAL_FD_SERIAL_PORT
int32_t fd_serial_port = -1;
#else
/* Definitions needed for USI Ports Management */
#define MAX_USI_PORTS 4
struct usi_port_t {
	uint8_t port_type;
	uint8_t port_number;
	int32_t fd;
};
struct usi_port_t usi_ports[MAX_USI_PORTS];
static uint32_t num_usi_ports = 0;
#endif

int _open_tty_serial(char *_sz_port, unsigned int _ui_speed);

/* Global Configuration save information about the connection */
extern struct st_configuration g_st_config;

/**@brief Open communication port (parameters are defined in PrjCfg.h)
  @param port_type: Port Type (UART_TYPE, USART_TYPE, COM_TYPE)
  @param commPort: Physical Communication Port
  @param speed:Communication speed
  @return 0 on success.
*/

int8_t addUsi_Open(uint8_t port_type, uint8_t port_number, uint32_t bauds)
{
	char serial_port[16];
	int32_t fd;
	LOG_USI_ERR("addUsi_Open");
	memset(serial_port, '\0', 16);
	if (g_st_config.sz_port_type != TCP_TYPE) {
		/* Serial Port Connection - From PrjCfg.h */
		//snprintf(serial_port, 16, "/dev/ttyS%d", port_number);
		//fd = _open_tty_serial(serial_port, bauds);
		LOG_USI_ERR("g_st_config.sz_tty_name = %s sz_tty_speed = %d\n\r",g_st_config.sz_tty_name ,g_st_config.sz_tty_speed);
		fd=_open_tty_serial(g_st_config.sz_tty_name, g_st_config.sz_tty_speed);
		LOG_USI_ERR("fd %d \n\r", fd);

	}else if (TCP_TYPE == port_type) {
		/* TCP Port connection - From Global Config */
		fd = _connect_to_server(g_st_config.sz_hostname, g_st_config.sz_tcp_port);
	} else {
		/*! Port Type not defined */
		LOG_USI_ERR("USI Port Type not defined");
		return -1;
	}

	if (-1 == fd) {
		LOG_USI_ERR("Impossible to open port %s", serial_port);
		return -1;
	}else{
		LOG_USI_DEBUG("open port descriptor %d", fd);
	}

#ifdef CONFIG_GLOBAL_FD_SERIAL_PORT
	fd_serial_port = fd;
#else
	usi_ports[num_usi_ports].port_type = port_type;
	usi_ports[num_usi_ports].port_number = port_number;
	usi_ports[num_usi_ports++].fd = fd;
#endif
	return 0;
}

/*
 *  @brief	Close the serial port
 *
 *   @param	port_type	  Port Type from "UsiCfg.h"
 *   @param	port_number	Port Number
 *   @return		  0 Success
 *               -1 Error
 */
int addUsi_Close(uint8_t port_type, uint8_t port_number)
{
	uint32_t i;
	int32_t fd, ret;
	/* Look for File descriptor associated to serial port */
#ifdef CONFIG_GLOBAL_FD_SERIAL_PORT
	fd = fd_serial_port;
#else
	fd = 0;
	for (i = 0; i < MAX_USI_PORTS; i++) {
		if ((usi_ports[i].port_type == port_type) && (usi_ports[i].port_number == port_number)) {
			fd = usi_ports[i].fd;
		}
	}
	if (!fd) {
		LOG_USI_ERR("Close    Serial port not found!!!");
		return -1;
	}
#endif
	close(fd);
	return 0;
}

/*
 * @brief	This function transmits a message through the interface.
 * @param	port_type	  Serial Port Type
 * @param port_number Serial Port Number
 * @param	sz_msg			Pointer to the message to be sent
 * @param	i_msglen		Length of the message to be sent
 * @return	number of bytes written in the output buffer
 * This function transmits a message through the interface.
 *
 */
uint16_t addUsi_TxMsg(uint8_t port_type, uint8_t port_number, uint8_t *sz_msg, uint16_t i_msglen)
{
	uint32_t i;
	int32_t fd, ret;

#ifdef CONFIG_GLOBAL_FD_SERIAL_PORT
	fd = fd_serial_port;
#else
	/* Look for File descriptor associated to serial port */
	fd = 0;
	for (i = 0; i < MAX_USI_PORTS; i++) {
		if ((usi_ports[i].port_type == port_type) && (usi_ports[i].port_number == port_number)) {
			fd = usi_ports[i].fd;
		}
	}
	if (!fd) {
		LOG_USI_ERR("Tx Serial port not found!!!");
		ret = -1;
	}
#endif
	/* write buffer to tty */
	ret = write(fd, sz_msg, i_msglen);
	return (ret > 0) ? ret : 0;
}

/*
 * @brief	This function reads a character from the UART.
 * @param	port_type	  Port Type to write in
 * @param	port_number	Port Number to write in
 * @param	c			      Pointer to store character read.
 * @return  	        0 if a character has been read.
 * 			             -1 otherwise
 *
 */
int8_t addUsi_RxChar(uint8_t port_type, uint8_t port_number, uint8_t *c)
{
	uint32_t i;
	int32_t fd, ret;

#ifdef CONFIG_GLOBAL_FD_SERIAL_PORT
	fd = fd_serial_port;
#else
	/* Look for File descriptor associated to serial port */
	fd = 0;
	for (i = 0; i < MAX_USI_PORTS; i++) {
		if ((usi_ports[i].port_type == port_type) && (usi_ports[i].port_number == port_number)) {
			fd = usi_ports[i].fd;
		}
	}
	if (!fd) {
		LOG_USI_ERR("Rx Serial port not found!!!");
		ret = -1;
	}
#endif
	/* Read one byte from tty */
	ret = read(fd, c, 1);
	return (ret > 0) ? 0 : -1;
}

/*
 * @brief	Open TTY serial.
 * @param	_sz_port	tty to connect to.
 * @param	_ui_speed	baudrate configuration.
 * @return	on success, returs the file descriptor representing this tty.
 *                         -1 otherwise
 *
 */
int _open_tty_serial(char *_sz_port, unsigned int _ui_speed)
{
	/* structure to store the port settings in */
	struct termios port_settings;
	uint32_t ui_br = B230400;

	/* open device file */
	int fd = open(_sz_port, O_RDWR | O_NDELAY | O_NONBLOCK);

	if (fd == -1) {
		LOG_USI_ERR("Open_port: Unable to open %s", _sz_port);
		return -1;
	}
	/* fcntl(fd, F_SETFL, O_NONBLOCK); */
	LOG_USI_INFO("TTY port %s is open with speed %d and descriptor %d", _sz_port, _ui_speed, fd);

	/* Check valid baudrates - B256000 is not defined on Linux */
	if (_ui_speed == 115200) {
		ui_br = B115200;
	} else if (_ui_speed == 921600) { /* Doesn't work for this platform */
		ui_br = B921600;
	} else if (_ui_speed == 230400) {
		ui_br = B230400;
	} else if (_ui_speed == 57600) {
		ui_br = B57600;
	} else if (_ui_speed == 38400) {
		ui_br = B38400;
	} else if (_ui_speed == 19200) {
		ui_br = B19200;
	} else {
		LOG_USI_ERR("Invalid Baudrate");
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
	return fd;
}

/*
 * @brief	Open TCP Connection Server.
 * @param	_sz_server	tty to connect to.
 * @param	_ui_speed	baudrate configuration.
 * @return	on success, returs the file descriptor representing this tty.
 *                      -1 otherwise
 *
 */
int _connect_to_server(char *_sz_server, unsigned int _ui_port)
{
	struct hostent *he;  /* estructura que recibirá información sobre el nodo remoto */
	struct sockaddr_in server; /* información sobre la dirección del servidor */
	int fd;

	LOG_USI_DEBUG("gethostbyname %s\n", _sz_server);

	/* get the host info */
	if ((he = gethostbyname(_sz_server)) == NULL) {
		perror("error gethostbyname(): ");
		return -1;
	}

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		/* llamada a socket() */
		perror("socket() error\n");
		return -1;
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(_ui_port);
	server.sin_addr = *((struct in_addr *)he->h_addr);

	memset(&(server.sin_zero), 0, 8);

	if (connect(fd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
		LOG_USI_ERR("Connection to Server error!\n");
		perror("connect() error\n");
		return -1;
	}

	LOG_USI_INFO("Connection to Server %s:%u OK!\n", _sz_server, _ui_port);

	/* Set socket non-blocking */
	fcntl(fd, F_SETFL, O_NONBLOCK);

	/* All OK, return socket descriptor */
	return fd;
}
