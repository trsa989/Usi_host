#include <stdio.h>
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

#include "debug.h"

extern td_x_args g_x_args;
extern int g_usi_fd;

int _connect_to_server(char *_sz_server, unsigned int _ui_port);

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
	/* Ignore Port/Bauds hardcoded parameters, use global parameter configuration */
	PRINTF(PRINT_ERROR, "USI OPEN...");
	g_usi_fd =  _connect_to_server(g_x_args.sz_host_name, g_x_args.ui_port);

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
	/* write buffer to tty */
	return write(g_usi_fd, sz_msg, i_msglen);
}

/**@brief Read char from port
  @param port: port number configured in PrjCfg.h
  @param c: pointer to a character
  @return 0 if a charater has been read, otherwise -1.
*/
int8_t addUsi_RxChar(uint8_t port_type, uint8_t port, uint8_t *c)
{
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
int _connect_to_server(char *_sz_server, unsigned int _ui_port)
{
	struct hostent *he;  /* estructura que recibirá información sobre el nodo remoto */
	struct sockaddr_in server;      /* información sobre la dirección del servidor */
	int sd;

	PRINTF(PRINT_INFO, "gethostbyname %s\n", g_x_args.sz_host_name);

	/* get the host info */
	if ((he = gethostbyname(g_x_args.sz_host_name)) == NULL) {
		perror("error gethostbyname(): ");
		return -1;
	}

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		/* llamada a socket() */
		perror("socket() error\n");
		return(-1);
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(g_x_args.ui_port);
	/* htons() es necesaria nuevamente ;-o */
	server.sin_addr = *((struct in_addr *)he->h_addr);
	/*he->h_addr pasa la información de ``*he'' a "h_addr" */
	bzero(&(server.sin_zero), 8);

	if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
		PRINTF(PRINT_ERROR, "Connect to Server error!\n");
		perror("connect() error\n");
		return(-1);
	}

	PRINTF(PRINT_INFO, "Connect to Server OK!\n");

	/* Set socket non-blocking */
	fcntl(sd, F_SETFL, O_NONBLOCK);

	/* All OK, return socket descriptor */
	return sd;
}
