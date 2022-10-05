/**
 * \file
 *
 * \brief PRIME Sniffer file.
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include "prime_log.h"
#include "prime_utils.h"
#include "return_codes.h"
#include "base_node_manager.h"
#include "base_node_mng.h"
#include "ifacePrimeSniffer.h"
#include "prime_sniffer.h"
#include "conf_global.h"

#define MCHP_LOG_MAGIC_LEN 8
const unsigned char c_puc_mchp_log_magic_prime_number[MCHP_LOG_MAGIC_LEN ]={0x41,0x54,0x50,0x4c,0x53,0x46,0x00,0x01};

static int32_t prime_sniffer_flags = 0;
static int fd_sniffer;
#define SNIFFER_LISTEN_BACKLOG 1
static int32_t socket_sniffer = -1;
static int32_t session_sniffer = -1; // Only one connection available
static int32_t flag_sniffer_tcp_connected; // Detects connection alive...
static int32_t tcp_port_sniffer = 4444;
pthread_t tcp_sniffer_thread;
pthread_attr_t tcp_sniffer_thread_attr;

static int prime_sniffer_loglevel = PRIME_LOG_ERR;

/*
 * \brief  Get PRIME Sniffer Loglevel
 *
 * \return loglevel
 */
int prime_sniffer_get_loglevel(){
	return prime_sniffer_loglevel;
}

/*
 * \brief  	   Set PRIME Sniffer Loglevel
 *
 * \param [in]  int loglevel
 * \return 0
 */
int prime_sniffer_set_loglevel(int loglevel){
	PRIME_LOG(LOG_INFO,"Setting Sniffer Loglevel to %i\r\n",loglevel);
  prime_sniffer_loglevel = loglevel;
	return SUCCESS;
}

/*
 * \brief  	   Set PRIME Sniffer Flags
 *
 * \param [in]  int PRIME Sniffer flags
 * \return 0
 */
int prime_sniffer_set_flags(int flags)
{
	PRIME_LOG(LOG_INFO,"Setting Sniffer Flags to %i\r\n",flags);
  prime_sniffer_flags = flags;
	return SUCCESS;
}

/*
 * \brief  Get PRIME Sniffer Flags
 *
 * \return Snifer Flags
 */
int prime_sniffer_get_flags()
{
	return prime_sniffer_flags;
}

/*
 * \brief USI Protocol - Add escape sequences
 *
 * \param data: USI sniffer message without flag start/end
 * \param len: USI sniffer message length
 */
int add_escape_sequences(unsigned char *data, int len)
{
    unsigned char tail[2000];
    int i =0;

    for (i = 0; i < len; i ++) {
        if (data[i] == 0x7E) {
				//copy tail...
				memcpy(tail, data+i+1, len -i);

				//add escape sequence
				data[i] = 0x7D;
				data[i+1] = 0x20 ^ 0x7E;

				//copy back tail msg
				memcpy(data+i+2, tail, len -i);
				//update len
				len++;
				i++;
		} else if (data[i] == 0x7D) {
				//copy tail...
				memcpy(tail, data+i+1, len -i);

				//add escape sequence
				data[i] = 0x7D;
				data[i+1] = 0x20 ^0x7D;

				//copy back tail msg
				memcpy(data+i+2, tail, len -i);
				//update len
				len++;
				i++;
        }
    }
    return len;
}

/*
 * \brief USI Protocol - Add flags start/end
 *
 * \param data: USI sniffer message without flag start/end
 * \param len: USI sniffer message length
 */
int add_7es(unsigned char *data, int len)
{
    unsigned char tail[2000];

    memcpy(tail, data, len );
    data[0] = 0x7E;
    memcpy(data+1, tail, len );
    data[len+1] = 0x7E;
    return len +2;
}

/*
 * \brief PRIME Sniffer Callback Function
 *        Sniffer Binary Format suppose save USI frames raw
 *        Received after a frame received. Open file like on Microchip PLC Sniffer with "File\Import ATPL Log..."
 *
 * \param msg: USI sniffer message (Full USI Frame without 0x7Es and Escape Sequences)
 * \param len: USI sniffer message length
 */
void prime_sniffer_process(uint8_t* msg, uint16_t len)
{
	  int length, wc;

		/* Some decoding/statistics could be included here getting information from sniffer trace */
    if (prime_sniffer_flags & SNIFFER_FLAG_ENABLE){
       /* Save Frame as MCHP Sniffer */
		   // Add escape characters - Don't worry about pointer, comes from global buffer
		   length = add_escape_sequences(msg,len);
		   // Add USI START/STOP => 0x7E - Don't worry about pointer, comes from global buffer
		   length = add_7es(msg,length);
       if (prime_sniffer_flags & SNIFFER_FLAG_LOGFILE) {
           //PRIME_LOG(LOG_DBG,"Writting Sniffer Frame to Logfile \r\n");
		       //Save to the file
		       wc = write(fd_sniffer,msg,length);
			     if (wc != length) {
						  close(fd_sniffer);
				      PRIME_LOG(LOG_ERR,"Error writting to the sniffer file\r\n");
				      //return ERROR_SNIFFER_LOGFILE_WRITE;
			     }
       }
       if ((prime_sniffer_flags & SNIFFER_FLAG_SOCKET) && flag_sniffer_tcp_connected) {
         //PRIME_LOG(LOG_DBG,"Sniffer TCP: Writting to socket \r\n");
   		   //Write to the socket
   		   wc = write(session_sniffer,msg,length);
   		   if (wc != length) {
            flag_sniffer_tcp_connected = 0;
						close(session_sniffer);
   		      PRIME_LOG(LOG_ERR,"Sniffer TCP: Error writting to socket\r\n");
   		      //return ERROR_SNIFFER_TCP_WRITE;
   		   }
       }
    }
		//return SUCCESS;
}

/*
 * \brief Starts PRIME Sniffer to Logfile
 *        Opens the logfile for writting and includes Magic ID
 *        to be understable by MCHP PLC Sniffer Tool (ATPL Log)
 * \return
 */
int prime_sniffer_logfile_start()
{
/*********************************************************
*       Vars
*********************************************************/
	 char filename[64];
 	 struct tm *timenow;
	 int wc;
	 time_t now;
/*********************************************************
*       Code
*********************************************************/
  PRIME_LOG(LOG_DBG,"prime_sniffer_logfile_start\r\n");
  /* Create Sniffer Logfile */
  now = time(NULL);
  timenow = gmtime(&now);
  strftime(filename, sizeof(filename), "/etc/config/sniffer_%Y%m%d_%H%M%S.bin", timenow);
	fd_sniffer = open(filename, O_CREAT|O_RDWR|O_NONBLOCK, S_IRWXU);
  if (fd_sniffer <= 0){
     PRIME_LOG_PERROR(LOG_ERR,"Cannot create the sniffer logfile:");
		 return ERROR_SNIFFER_LOGFILE_OPEN;
  }
	/* Write MCHP Magic Number defined for PRIME Sniffer */
	wc = write(fd_sniffer,c_puc_mchp_log_magic_prime_number, MCHP_LOG_MAGIC_LEN);
	if (wc < MCHP_LOG_MAGIC_LEN){
		 PRIME_LOG(LOG_ERR,"Cannot write on the sniffer file\r\n");
		 return ERROR_SNIFFER_LOGFILE_WRITE;
	}
	PRIME_LOG(LOG_INFO,"Started logging on logfile %s\r\n",filename);
  return SUCCESS;
}

/********************************************************
* \brief Check if TCP port is available
* \param
* \return
********************************************************/
int prime_sniffer_tcp_check_port(int32_t tcp_port){
/*********************************************************
*       Vars
*********************************************************/
/*********************************************************
*       Code
*********************************************************/
    return prime_tcp_check_port(tcp_port);
}

/********************************************************
* \brief Get TCP Port for Sniffer
*
* \param
* \return
********************************************************/
int32_t prime_sniffer_tcp_get_port()
{
/*********************************************************
*       Vars
*********************************************************/
/*********************************************************
*       Code
*********************************************************/
   return tcp_port_sniffer;
}

/********************************************************
* \brief Set TCP Port for Sniffer
* \param
* \return
********************************************************/
int32_t prime_sniffer_tcp_set_port(int32_t tcp_port)
{
/*********************************************************
*       Vars
*********************************************************/
   int ret;
/*********************************************************
*       Code
*********************************************************/
   ret = prime_tcp_check_port(tcp_port);
   if (!ret)
      tcp_port_sniffer = tcp_port;
   return ret;
}

/********************************************************
* \brief Thread to handle connections to TCP Sniffer
*
* \param  thread_parameters
* \return
********************************************************/
void * prime_sniffer_tcp_thread(void * thread_parameters)
{
/*********************************************************
*       Vars
*********************************************************/
/*********************************************************
*       Code
*********************************************************/
   while (1){
     session_sniffer=accept(socket_sniffer,0,0);
     if (session_sniffer == -1){
       PRIME_LOG_PERROR(LOG_ERR,"Cannot accept Sniffer TCP connection:");
       pthread_exit(NULL);
     }
     PRIME_LOG(LOG_INFO,"Sniffer TCP connection accepted\r\n");
     flag_sniffer_tcp_connected = 1;
     while (flag_sniffer_tcp_connected){
			  /* Only one connection each time */
        sleep(1);
     }
     close(session_sniffer);
   }
}

/*
 * \brief Starts PRIME Sniffer to TCP
 *        Open a TCP Server to receive connections from
 *        MCHP PLC Sniffer Tool
 *
 * \param sniffer_port: Sniffer TCP Port
 */
int prime_sniffer_tcp_start(int sniffer_port)
{
/*********************************************************
*       Vars
*********************************************************/
  int i_on =1;
  int ret;
  struct sockaddr_in sock_addr;
/*********************************************************
*       Code
*********************************************************/
  PRIME_LOG(LOG_DBG,"prime_sniffer_tcp_start sniffer_port=0x%08X\r\n",sniffer_port);

	ret = prime_tcp_check_port(sniffer_port);
  if (ret){
     PRIME_LOG(LOG_ERR,"Sniffer TCP Port is not available\r\n");
     return ret;
  }

  socket_sniffer = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_sniffer < 0)
  {
     PRIME_LOG_PERROR(LOG_ERR,"Sniffer TCP: Cannot open socket:");
     return  ERROR_SNIFFER_TCP_SOCKET;
  }

  /* Allow socket descriptor to be reuseable */
  ret = setsockopt(socket_sniffer, SOL_SOCKET, SO_REUSEADDR, &i_on, sizeof(i_on));
  if (ret < 0){
     PRIME_LOG_PERROR(LOG_ERR,"Sniffer TCP: setsockopt() failed:");
     close(socket_sniffer);
     return ERROR_SNIFFER_TCP_SOCKET_OPTS;
  }

  /*
     Set socket to be non-blocking.  All of the sockets for
     the incoming connections will also be non-blocking since
     they will inherit that state from the listening socket.
  */
  /*i_rc = ioctl(sd_sniffer, FIONBIO, (char *)&i_on);
  if (i_rc < 0)
  {
     PRIME_LOG_PERROR(LOG_ERR,"ioctl() failed");
     close(i_listen_sd);
     exit(-1);
  }
  */

  /* Bind the socket */
  memset(&sock_addr, 0, sizeof(struct sockaddr_in));
  sock_addr.sin_family      = AF_INET;
  sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  sock_addr.sin_port        = htons(sniffer_port);
  ret = bind(socket_sniffer, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
  if (ret < 0){
     PRIME_LOG_PERROR(LOG_ERR,"Sniffer TCP: bind() failed\r\n");
     close(socket_sniffer);
     return ERROR_SNIFFER_TCP_BIND;
  }

  /* Set the listen backlog */
  ret = listen(socket_sniffer, SNIFFER_LISTEN_BACKLOG);
  if (ret < 0){
     PRIME_LOG_PERROR(LOG_ERR,"Sniffer TCP: listen() failed\r\n");
     close(socket_sniffer);
     return ERROR_SNIFFER_TCP_LISTEN;
  }

  // Create tcp_sniffer thread to handle USI Frames on serial port
  pthread_attr_init( &tcp_sniffer_thread_attr );
  pthread_attr_setdetachstate( &tcp_sniffer_thread_attr, PTHREAD_CREATE_DETACHED );
  /* Create a thread which handle connections to TCP Sniffer */
  if (pthread_create(&tcp_sniffer_thread, NULL, prime_sniffer_tcp_thread, NULL)) {
      PRIME_LOG_PERROR(LOG_ERR,"Sniffer TCP: Error creating Thread\n");
      return ERROR_SNIFFER_TCP_THREAD;
  }
	PRIME_LOG(LOG_INFO,"Started logging on TCP port %d\r\n",sniffer_port);
  pthread_attr_destroy(&tcp_sniffer_thread_attr);
  return SUCCESS;
}

/*
 * \brief Starts PRIME Sniffer capabilities via USI_HOST
 *
 * \param flags: enable/disable global / logfile / logTCP status
 * \return
*/
int prime_sniffer_start(uint32_t flags)
{
/*********************************************************
*       Vars
*********************************************************/
   int ret = 0;
/*********************************************************
*      Code
*********************************************************/
	 PRIME_LOG(LOG_DBG,"prime_sniffer_start flags=0x%08X\r\n",flags);

   if (!flags){
      return 0;
   }
   if (flags){
      if (flags & SNIFFER_FLAG_LOGFILE){
				 if (~(prime_sniffer_flags & SNIFFER_FLAG_LOGFILE)){
					  /* Create Sniffer Logfile */
         		ret |= prime_sniffer_logfile_start();
				 }
			}
      if (flags & SNIFFER_FLAG_SOCKET){
				 if (~(prime_sniffer_flags & SNIFFER_FLAG_SOCKET)){
					  /* Sniffer TCP Start */
         		ret |= prime_sniffer_tcp_start(tcp_port_sniffer);
				 }
			}
			/* Set Sniffer Callback */
			prime_sniffer_set_cb(prime_sniffer_process);

			if (flags & SNIFFER_FLAG_ENABLE){
      	/* Enable Embedded Sniffer */
      	prime_embedded_sniffer(1);
		  }
      prime_sniffer_flags = flags;
   }
	 return ret;
}

/**
  * \brief Stops PRIME Sniffer capabilities via USI_HOST
  *
  * \param flags: enable/disable global / logfile / logTCP status
  * \return
*/
int prime_sniffer_stop(uint32_t flags)
{
#if 0
void *res;
#endif
   if (prime_sniffer_flags){
      if (flags & SNIFFER_FLAG_LOGFILE){
        /* Save Sniffer Log File */
        PRIME_LOG(LOG_INFO,"Stopping Sniffer Logfile...\r\n");
			  close(fd_sniffer);
      }
      if (flags & SNIFFER_FLAG_SOCKET){
        PRIME_LOG(LOG_INFO,"Stopping Sniffer TCP...\r\n");
				#if 0
        /* Sniffer TCP Thread Stop */
        s = pthread_cancel(tcp_sniffer_thread);
				if (s != 0){
						PRIME_LOG(LOG_ERR,"Error Stopping Sniffer TCP thread...\r\n");
				}
				s = pthread_join(tcp_sniffer_thread,&res);
				if (s != 0){
					  PRIME_LOG(LOG_ERR,"Error Waiting Sniffer TCP thread finish\r\n");
				}
				if (res == PTHREAD_CANCELED){
					  PRIME_LOG(LOG_INFO,"Sniffer TCP thread stopped\r\n");
				}else{
					  PRIME_LOG(LOG_ERR,"Error stopping Sniffer TCP thread\r\n");
				}
				#endif
				/* Close File Descriptors */
				//close(session_sniffer);
				flag_sniffer_tcp_connected = 0;
        close(socket_sniffer);
      }
      prime_sniffer_flags = flags;
   }

   /* Disable Embedded Sniffer */
   prime_embedded_sniffer(0);

	 /* Set Null Sniffer Callback */
	 prime_sniffer_set_cb(NULL);

	 return SUCCESS;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
