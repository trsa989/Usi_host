/**
 * \file
 *
 * \brief PRIME utils file.
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

/************************************************************
*       Includes                                            *
*************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "prime_log.h"
#include "return_codes.h"
#include "prime_utils.h"

/************************************************************
*       Global Vars                                         *
*************************************************************/

/*
 * \brief  Convert numeric time to date string
 * \param  *time pointer to time value
 * \return time in string format
 */
char *time2str (time_t *time)
{
/*********************************************
*       Local Envars                         *
**********************************************/
    static char time_str[255];
    struct tm *ptm;
/*********************************************
*       Code                                 *
**********************************************/
    ptm=localtime (time);
    strftime(time_str, 255, "%Y-%m-%d", ptm);
    return time_str;
}

/*
 * \brief  Convert date string to numeric time
 * \param  *date pointer to time value string
 * \return time in numeric format
 */
time_t str2time (char *date) {
/*********************************************
*       Local Envars                         *
**********************************************/
    struct tm time_num;
/*********************************************
*       Code                                 *
**********************************************/
    memset (&time_num, 0, sizeof(struct tm));
    strptime(date, "%m-%d-%Y", &time_num);
    return mktime(&time_num);
}

/*
 * \brief Convert Hexadecimal to Integer
 * \param *s    pointer to hexadecimal vector
 * \return      integer
 */
int Hex2Int(uint8_t *s)
{
/*********************************************
*       Local Envars                         *
**********************************************/
	int n = 0;
	uint8_t ch;
/*********************************************
*       Code                                 *
**********************************************/

	while (*s) {
		ch = *s - '0';
    if (*s =='x' || *s =='X'){
       s++;
       continue;
    }
		if (ch >= 49 && ch <= 55) ch -= 39;	// 'a' - '0'
		else if (ch >= 17 && ch <= 22) ch -= 7;	// 'A' - '0'
		else if (ch < 0 || ch > 16) break;
		n = n<<4;
		n += ch;
		s++;
	}
	return n;
}

/*
 *  \brief Convert string to EUI48 address
 *
 *  \param eui48   -> pointer to EUI48 destination
 *  \param str     -> pointer to String source
 *  \return           pointer to EUI48 destination
 */
int32_t str_to_eui48(char* str, uint8_t * eui48)
{
/*********************************************
*       Local Envars                         *
**********************************************/
  uint8_t *ptr;
  uint8_t chunk[5];
  uint8_t len,i;
/*********************************************
*       Code                                 *
**********************************************/
	ptr = eui48;
  len = strlen(str);
	if ((str[0] == '0') && ((str[1] == 'x') || (str[1] == 'X'))){
		 memmove(&str[0],&str[2],len);
		 len -= 2;
	}
  if (str[len-1] == '\n')
     str[--len] = '\0';
  if (len != (EUI48_LEN<<1)) {
    PRIME_LOG(LOG_ERR,"EUI48 length is wrong (%d)\r\n",len);
    return -1;
  }
  chunk[0]='0';
  chunk[1]='x';
  chunk[4]='\0';
  /* 6 Bytes to be read - MAC Size */
  for (i=0; i<EUI48_LEN; i++)   {
    chunk[2]=str[2*i];
    chunk[3]=str[2*i+1];
    ptr[i]=(uint8_t)Hex2Int(chunk);
  }
	return 0;
}

/*
 *  \brief Convert string to DUK
 *
 *  \param duk     -> pointer to DUK destination
 *  \param str     -> pointer to String source
 *  \return           pointer to DUK destination
 */
int32_t str_to_duk(char* str, uint8_t * duk)
{
/*********************************************
*       Local Envars                         *
**********************************************/
  uint8_t *ptr;
  uint8_t chunk[5];
  uint8_t len,i;
/*********************************************
*       Code                                 *
**********************************************/
	ptr = duk;
  len = strlen(str);
  if (str[len-1] == '\n')
     str[--len] = '\0';
  /* 16 Bytes to be read => 32 HEX characters */
  if (len != (DUK_LEN<<1)){
    PRIME_LOG(LOG_ERR,"DUK length is wrong (%d)\r\n",len);
    return -1;
  }
  chunk[0]='0';
  chunk[1]='x';
  chunk[4]='\0';
  /* 16 Bytes to be read - DUK Size */
  for (i=0; i<DUK_LEN; i++)   {
    chunk[2]=str[2*i];
    chunk[3]=str[2*i+1];
    ptr[i]=(uint8_t)Hex2Int(chunk);
  }
	return 0;
}

 /*
  *  \brief Convert EUI48 address to string - No memory check
  *  \param EUI48   -> pointer to EUI48 source
  *  \param str     -> pointer to String destination
  *  \return           pointer to String destination
  */
char * eui48_to_str(const unsigned char* EUI48, char * str)
{
   char *ptr;
   static char eui48str[EUI48_LEN+1];

   if (str != NULL){
      ptr = str;
   }else{
      memset(eui48str,'\0',EUI48_LEN+1);
      ptr = eui48str;
   }
   snprintf (ptr, ((EUI48_LEN<<2) + 1), "%02x%02x%02x%02x%02x%02x",
	                  EUI48[0], EUI48[1], EUI48[2], EUI48[3], EUI48[4], EUI48[5]);
 	return ptr;
}

/*
 *  \brief Convert FU State to string - No memory check
 *  \param fu_state -> FU State
 *  \param str      -> pointer to String destination
 *  \return            pointer to String destination
 */
char * fup_state_to_str(const uint8_t fu_state, char * str)
{
	char *ptr;
	static char fu_state_str[16];

	if (str != NULL){
		 ptr = str;
	}else{
		 memset(fu_state_str,'\0',16);
		 ptr = fu_state_str;
	}
  switch (fu_state){
		case FUP_STATE_IDLE:
			strncpy(ptr,"IDLE",16);
			break;
		case FUP_STATE_RECEIVING:
			strncpy(ptr,"RECEIVING",16);
			break;
		case FUP_STATE_COMPLETE:
			strncpy(ptr,"COMPLETE",16);
			break;
		case FUP_STATE_COUNTDOWN:
			strncpy(ptr,"COUNTDOWN",16);
			break;
		case FUP_STATE_UPGRADED:
			strncpy(ptr,"UPGRADED",16);
			break;
		case FUP_STATE_EXCEPTION:
			strncpy(ptr,"EXCEPTION",16);
			break;
		case FUP_STATE_CONFIRMED:
			strncpy(ptr,"CONFIRMED",16);
			break;
		default:
			strncpy(ptr,"UNKNOWN",16);
			break;
	}
  return ptr;
}

  /*
   *  \brief Convert FU Exception State to string - No memory check
   *  \param fu_exception_state -> FU Exception State
   *  \param str                -> pointer to String destination
   *  \return                      pointer to String destination
   */
  char * fup_state_exception_to_str(const uint16_t fu_exception_state, char * str)
  {
  	char *ptr;
  	static char fu_exception_state_str[32];

  	if (str != NULL){
  		 ptr = str;
  	}else{
  		 memset(fu_exception_state_str,'\0',32);
  		 ptr = fu_exception_state_str;
  	}
    if (fu_exception_state & 0x0080){
       strncat(ptr,"PERMANENT - ",16);
    }else {
       strncat(ptr,"TEMPORARY - ",16);
    }
    switch (fu_exception_state & 0x007F){
  		case FUP_EXCEPTION_GENERAL:
  			strncat(ptr,"GENERAL",16);
  			break;
  		case FUP_EXCEPTION_PROTOCOL:
  			strncat(ptr,"PROTOCOL",16);
  			break;
  		case FUP_EXCEPTION_CRC_FAIL:
  			strncat(ptr,"CRC",16);
  			break;
  		case FUP_EXCEPTION_INVALID_IMG:
  			strncat(ptr,"IMAGE",16);
  			break;
  		case FUP_EXCEPTION_SIGNATURE:
  			strncat(ptr,"SIGNATURE",16);
  			break;
  		case FUP_EXCEPTION_SAFETY_TIME_EXPIRED:
  			strncat(ptr,"TIMER_EXPIRED",16);
  			break;
  		default:
  			strncat(ptr,"UNKNOWN",16);
  			break;
  	}
    return ptr;
  }

/*
 *  \brief Convert Network Event to string - No memory check
 *  \param netwqork_event -> Network Event
 *  \param str            -> pointer to String destination
 *  \return                  pointer to String destination
 */
char * network_event_to_str(const uint8_t network_event, char * str)
{
	char *ptr;
	static char network_event_str[16];

	if (str != NULL){
		 ptr = str;
	}else{
		 memset(network_event_str,'\0',16);
		 ptr = network_event_str;
	}
	switch (network_event){
		case BMNG_NET_EVENT_REGISTER:
			strncpy(ptr,"REGISTER",16);
			break;
		case BMNG_NET_EVENT_UNREGISTER:
			strncpy(ptr,"UNREGISTER",16);
			break;
		case BMNG_NET_EVENT_PROMOTE:
			strncpy(ptr,"PROMOTE",16);
			break;
		case BMNG_NET_EVENT_DEMOTE:
			strncpy(ptr,"DEMOTE",16);
			break;
		case BMNG_NET_EVENT_ALIVE:
			strncpy(ptr,"ALIVE",16);
			break;
		case BMNG_NET_EVENT_REBOOT:
			strncpy(ptr,"REBOOT",16);
			break;
		case BMNG_NET_EVENT_NO_DUK:
			strncpy(ptr,"NO_DUK",16);
			break;
		case BMNG_NET_EVENT_UNKNOWN_NODE:
			strncpy(ptr,"UNKNOWN",16);
			break;
		default:
			strncpy(ptr,"ERROR",16);
			break;
	}
	return ptr;
}

/********************************************************
* \brief Check if TCP port is available
* \param
* \return
********************************************************/
int prime_tcp_check_port(int32_t tcp_port){
/*********************************************************
*       Vars
*********************************************************/
    char *hostname = "127.0.0.1";
    int32_t sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int32_t ret;
/*********************************************************
*       Code
*********************************************************/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        PRIME_LOG_PERROR(LOG_DBG,"ERROR opening socket");
        return ERROR_TCP_SOCKET;
    }
    server = gethostbyname(hostname);
    if (server == NULL) {
        PRIME_LOG_PERROR(LOG_ERR,"ERROR getting hostbyname:");
        close(sockfd);
        return ERROR_TCP_SOCKET;
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(tcp_port);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) == 0) {
        PRIME_LOG(LOG_DBG,"TCP port %d is not available\r\n",tcp_port);
        ret = ERROR_TCP_CONNECT;
    } else {
        PRIME_LOG(LOG_INFO,"TCP Port %d available\r\n",tcp_port);
        ret = SUCCESS;
    }
    close(sockfd);
    return ret;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
