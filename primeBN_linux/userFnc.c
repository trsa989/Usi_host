/**
 * \file
 *
 * \brief userFnc file.
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

/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

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

#include <termios.h> // POSIX terminal control definitions
#include "../src/Usi.h"
#include "../src/UsiCfg.h"

#include "prime_log.h"
#include "return_codes.h"
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
static uint32_t num_usi_ports=0;
#endif

int _open_tty_serial(char *_sz_port, unsigned int _ui_speed);

int _connect_to_server(char *_sz_server, unsigned int _ui_port);

/* Global Configuration save information about the connection */
extern struct st_configuration g_st_config;

/*
 * \brief	Open the serial port by calling  comSerial library,
 *        also initiates the module vars
 *        Please adapt the implementation for your platform
 *
 * \param	port		Used for communications
 * \param	bauds		speed
 *
 * \return		    0 Success, -1 Error
 */
int8_t addUsi_Open(uint8_t port_type, uint8_t port_number, uint32_t bauds)
{
  char serial_port[16];
	int32_t fd;

	memset(serial_port,'\0',16);
	if ((g_st_config.sz_tty_name != NULL) && (g_st_config.sz_port_type != TCP_TYPE)){
		// Serial Port Connection - From Global Config - Ignore PrjCfg.h
		LOG_USI_ERR("Serial Port Connection - FFrom Global Config - Ignore PrjCfg.h\n\r");
		fd=_open_tty_serial(g_st_config.sz_tty_name, g_st_config.sz_tty_speed);
	}else if (g_st_config.sz_port_type == TCP_TYPE){
		// TCP Port connection - From Global Config
   	fd=_connect_to_server(g_st_config.sz_hostname,g_st_config.sz_tcp_port);
	}else{
		/* Port from PrjCfg.h */
		LOG_USI_ERR("TTYS_TYPE = %d port_type = %d\n\r",TTYS_TYPE ,port_type);
		if (TTYS_TYPE == port_type){
			  // Serial Port Connection - From PrjCfg.h
			LOG_USI_ERR("Serial Port Connection - From PrjCfg.h\n\r");
			  snprintf(serial_port, 16, "/dev/ttyS%d",port_number);
				fd=_open_tty_serial(serial_port, bauds);
		} else if (TTYACM_TYPE == port_type){
				snprintf(serial_port, 16, "/dev/ttyACM%d",port_number);
				fd=_open_tty_serial(serial_port, bauds);
		} else if (TTYUSB_TYPE == port_type){
				snprintf(serial_port, 16, "/dev/ttyUSB%d",port_number);
				fd=_open_tty_serial(serial_port, bauds);
		}else{
		  /*! Port Type not defined */
			return ERROR_USERFNC_PORT_TYPE;
		}
	}
	if (fd <= 0){
			LOG_USI_ERR("USI Error 0x%X\r\n",fd);
			return ERROR_USERFNC_FD;
	}
#ifdef CONFIG_GLOBAL_FD_SERIAL_PORT
  fd_serial_port = fd;
#else
	usi_ports[num_usi_ports].port_type = port_type;
	usi_ports[num_usi_ports].port_number = port_number;
	usi_ports[num_usi_ports++].fd = fd;
#endif
	return SUCCESS;
}


/*
 *  \brief	Close the serial port
 *
 *  \param	port_type	  Port Type from "UsiCfg.h"
 *  \param	port_number	Port Number
 *  \return		    0 Success
 *               -1 Error
 */
int addUsi_Close(uint8_t port_type, uint8_t port_number)
{
	int32_t fd;
	/* Look for File descriptor associated to serial port */
#ifdef CONFIG_GLOBAL_FD_SERIAL_PORT
	fd = fd_serial_port;
#else
  uint32_t i;
	fd = 0;
	for (i=0;i<MAX_USI_PORTS;i++){
	    if ((usi_ports[i].port_type == port_type) && (usi_ports[i].port_number == port_number)){
		     fd=usi_ports[i].fd;
	    }
	}
	if (fd <= 0){
	    return ERROR_USERFNC_FD;
	}
#endif
	close (fd);
	return SUCCESS;
}

/*
 * \brief	This function transmits a message through the interface.
 *
 * \param	port_type	  Serial Port Type
 * \param port_number Serial Port Number
 * \param	sz_msg			Pointer to the message to be sent
 * \param	i_msglen		Length of the message to be sent
 *
 * \return	number of bytes written in the output buffer
 */
uint16_t addUsi_TxMsg(uint8_t port_type, uint8_t port_number, uint8_t *sz_msg, uint16_t i_msglen)
{
	int32_t fd, ret;

#ifdef CONFIG_GLOBAL_FD_SERIAL_PORT
	fd = fd_serial_port;
#else
  uint32_t i;
  /* Look for File descriptor associated to serial port */
	fd = 0;
	for (i=0;i<MAX_USI_PORTS;i++){
	    if ((usi_ports[i].port_type == port_type) && (usi_ports[i].port_number == port_number)){
         fd=usi_ports[i].fd;
		  }
	}
	if (fd <= 0){
			ret = ERROR_USERFNC_FD;
	}
#endif
	/* write buffer to tty */
	int i;
	fprintf(stderr,"[USI] >> 0x");
	for (i=0;i<i_msglen;i++){
		fprintf(stderr,"%02X",sz_msg[i]);
		fflush(stderr);
	}
	fprintf(stderr,"\r\n"); fflush(stderr);
	ret = write(fd, sz_msg,i_msglen);
	return (ret > 0)? ret: SUCCESS;
}

/*
 * \brief	This function reads a character from the UART.
 *
 * \param	port_type	  Port Type to write in
 * \param	port_number	Port Number to write in
 * \param	c			      Pointer to store character read.
 *
 * \return  	        0 if a character has been read.
 * 			             -1 otherwise
 */
int8_t addUsi_RxChar(uint8_t port_type, uint8_t port_number, uint8_t *c)
{
	int32_t fd, ret;

#ifdef CONFIG_GLOBAL_FD_SERIAL_PORT
	fd = fd_serial_port;
#else
  uint32_t i;
  /* Look for File descriptor associated to serial port */
	fd = 0;
	for (i=0;i<MAX_USI_PORTS;i++){
	    if ((usi_ports[i].port_type == port_type) && (usi_ports[i].port_number == port_number)){
         fd=usi_ports[i].fd;
		  }
	}
	if (fd <= 0){
			ret = ERROR_USERFNC_FD;
	}
#endif
	/* Read one byte from tty */
	ret = read(fd, c, 1);
	return (ret > 0) ? SUCCESS : -1;
}

/*
 * \brief	Open TTY serial.
 *
 * \param	_sz_port	tty to connect to.
 * \param	_ui_speed	baudrate configuration.
 *
 * \return	on success, returs the file descriptor representing this tty.
 * 			   -1 otherwise
 */
int _open_tty_serial(char *_sz_port, unsigned int _ui_speed)
{
	/* structure to store the port settings in */
	struct termios port_settings;
	uint32_t ui_br = B230400;

	/* open device file */
	int fd = open(_sz_port, O_RDWR | O_NDELAY | O_NONBLOCK);

	if (fd <= 0){
		 return ERROR_USERFNC_FD;
	}
  //fcntl(fd, F_SETFL, O_NONBLOCK);

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
		  return ERROR_USERFNC_TTY_SPEED;
	}

	memset(&port_settings,0,sizeof(port_settings));

	/* set baud rates */
	cfsetispeed(&port_settings, ui_br);
	cfsetospeed(&port_settings, ui_br);

	port_settings.c_iflag = 0;
	port_settings.c_oflag = 0;
	port_settings.c_lflag = 0;
 	port_settings.c_cflag &= ~PARENB;    // set no parity, stop bits, data bits
 	port_settings.c_cflag &= ~CSTOPB;
 	port_settings.c_cflag &= ~CSIZE;
 	port_settings.c_cflag |= CS8;
 	port_settings.c_cflag |= CREAD;
 	port_settings.c_cflag |= CLOCAL;     // 8n1, see termios.h for more information

 	/* apply the settings to the port */
	tcsetattr(fd, TCSANOW, &port_settings);
	tcflush(fd, TCIOFLUSH);
	return fd;
}

/*
 * \brief	Open TCP Connection Server.
 *
 * \param	_sz_server	tty to connect to.
 * \param	_ui_speed	baudrate configuration.
 *
 * \return	on success, returs the file descriptor representing this tty.
 * 			   -1 otherwise
 */
int _connect_to_server(char *_sz_server, unsigned int _ui_port)
{
	struct hostent *he;         // Remote Information
  struct sockaddr_in server;	// Server Information
	int fd;

	/* get the host info */
	if ((he = gethostbyname(_sz_server)) == NULL){
    		PRIME_LOG(LOG_ERR,"gethostbyname error\r\n");
	    	return ERROR_USERFNC_GETHOSTBYNAME;
	}

  fd=socket(AF_INET, SOCK_STREAM, 0);
	if (fd <= 0){
	      /* Error calling socket() */
		    PRIME_LOG(LOG_ERR,"Socket error\r\n");
	      return ERROR_USERFNC_FD;
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(_ui_port);
	server.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(server.sin_zero),0,8);

	if (connect(fd, (struct sockaddr *)&server, sizeof(struct sockaddr))==-1){
		 PRIME_LOG(LOG_ERR,"Connection to '%s:%d' error\r\n", _sz_server, _ui_port);
     return ERROR_USERFNC_CONNECT;
	}
	//Set socket non-blocking
	fcntl(fd, F_SETFL, O_NONBLOCK);
	//All OK, return socket descriptor
	return fd;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
