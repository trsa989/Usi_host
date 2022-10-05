/**
 * \file
 *
 * \brief Base Node DLMSoTCP.
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
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "mngLayerHost.h"
#include "prime_api_host.h"
#include "prime_api_defs_host.h"
#include "mac_pib.h"
#include "cl_432_defs.h"

#include "prime_utils.h"
#include "prime_log.h"
#include "return_codes.h"
#include "base_node_network.h"
#include "base_node_manager.h"
#include "base_node_mng.h"
#include "base_node_dlmsotcp.h"

/************************************************************
*       Defines                                             *
*************************************************************/
/* Define this depending on Base USI node buffers*/
#define DLMSoTCPPORT                 4059
#define PIB_432_CONNECTED_NODES_LIST 0x8250
#define SERIAL_NUMBER_432_MAC		     16

#define NUM_MAX_DLMSOTCP_MSG         20

#define MAX_MSG_LIST                 5
#define MAX_432_SEND_RETRY           2

struct x_dlms_msg{
    uint32_t ui_timestamp;
    uint16_t us_retries;
    uint16_t us_dst;
    uint16_t us_lsap;
    uint16_t us_length;
    uint8_t data[MAX_BUFFER_SIZE];
    struct x_dlms_msg * px_link;
} ;

struct x_dlms_msg_list {
	struct x_dlms_msg * px_head;
	struct x_dlms_msg * px_tail;
	uint16_t		        us_count;
};

struct {
    uint16_t dst_address;
    uint8_t serial_number[SERIAL_NUMBER_432_MAC];
    uint8_t len_serial_number;
    uint8_t mac[6];
    uint8_t autoclose_enabled;
} x_list_nodes_connected[MAX_NUM_NODES_CONNECTED];

/* Store the maximum index in the list of connected nodes */
static uint16_t us_max_index_connected_node;

/* Enable notifications flag */
static int32_t i_enable_notifications= FALSE;

static dl_432_buffer_t dlms_432_msg;
static unsigned char uc_rx_buffer[MAX_BUFFER_SIZE];

/* Concentrator socket descriptor */
int g_concentrator_fd = 0;
int i_server_sd = 0;         /* */

static int tcp_port_dlmsotcp = DLMSoTCPPORT;

pthread_t prime_dlmsotcp_thread;
pthread_attr_t prime_dlmsotcp_thread_attr;

/* Linked List for DLMS Messages */
dlms_msg_list prime_dlmsotcp_free_msg_list;
dlms_msg_list prime_dlmsotcp_pending_cfm_msg_list;

/* Mutex for accessing PRIME Network Information */
pthread_mutex_t prime_dlmsotcp_mutex = PTHREAD_MUTEX_INITIALIZER;

static int prime_dlmsotcp_loglevel = PRIME_LOG_DBG;
static FILE * prime_dlmsotcp_logfile_fd;

/* Linked List for PRIME Network visibility */
extern mchp_list prime_network;
/* Number of PRIME Service Nodes Registered - Better with function */
extern uint16_t prime_network_sn;

/*
 * \brief  Get PRIME DLMSoTCP Loglevel
 * \return loglevel
 */
int prime_dlmsotcp_get_loglevel()
{
  return prime_dlmsotcp_loglevel;
}

/*
 * \brief  	   Set PRIME DLMSoTCP Loglevel
 * \param [in]  int loglevel
 * \return 0
 */
int prime_dlmsotcp_set_loglevel(int loglevel)
{
	PRIME_LOG(LOG_INFO,"Setting DLMSoTCP Loglevel to %i\r\n",loglevel);
  prime_dlmsotcp_loglevel = loglevel;
	return SUCCESS;
}

/*
 * \brief  	  Set DLMSoTCP File Log Name
 *
 * \param [in] char * filename
 * \return 0
 */
int prime_dlmsotcp_set_logfile(char * filename)
{
/**********************************************************
*       Local Vars                                        *
***********************************************************/
/**********************************************************
*       Code                                              *
***********************************************************/
    PRIME_LOG(LOG_INFO,"Setting DLMSoTCP Logfile to %s\r\n",filename);
    if (prime_dlmsotcp_logfile_fd != NULL)
       fclose(prime_dlmsotcp_logfile_fd);

    prime_dlmsotcp_logfile_fd = fopen(filename, "a");
	  if (prime_dlmsotcp_logfile_fd == NULL){
		   /// STDERR if fails
		   prime_dlmsotcp_logfile_fd = stderr;
	  }
    return 0;
}

/*
 * \brief PRIME DLMSoTCP Mutex Lock
 * \return
 */
int prime_dlmsotcp_mutex_lock()
{
    return pthread_mutex_lock(&prime_dlmsotcp_mutex);
}

/*
 * \brief PRIME DLMSoTCP Mutex Unlock
 * \return
 */
int prime_dlmsotcp_mutex_unlock()
{
    return pthread_mutex_unlock(&prime_dlmsotcp_mutex);
}

/*
 * \brief Set TCP Port for DLMSoTCP
 * \param tcp_port : TCP Port for DLMSoTCP server
 * \return
 */
int32_t prime_dlmsotcp_tcp_set_port(int32_t tcp_port)
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
      tcp_port_dlmsotcp = tcp_port;
   return ret;
}

/*
 * \brief Get TCP Port for DLMSoTCP
 * \param
 * \return TCP Port
 */
int32_t prime_dlmsotcp_tcp_get_port()
{
/*********************************************************
*       Vars
*********************************************************/
/*********************************************************
*       Code
*********************************************************/
   return tcp_port_dlmsotcp;
}
#if 0
/********************************************************
* \brief Thread to handle connections to TCP DLMSoTCP
*
* \param  thread_parameters
* \return
********************************************************/
void * prime_dlmsotcp_thread(void * thread_parameters)
{
/*********************************************************
*       Vars
*********************************************************/
/*********************************************************
*       Code
*********************************************************/
   while (1){
     session_dlmsotcp=accept(socket_dlmsotcp,0,0);
     if (session_dlmsotcp == -1){
       PRIME_LOG_PERROR(LOG_ERR,"Cannot accept DLMSoTCP connection:");
       pthread_exit(NULL);
     }
     PRIME_LOG(LOG_INFO,"DLMSoTCP connection accepted\r\n");
     flag_dlmsotcp_connected = 1;
     while (flag_dlmsotcp_connected){
			  /* Only one connection each time */
        sleep(1);
     }
     close(session_dlmsotcp);
   }
}

/*
 * \brief Starts PRIME DLMSoTCP
 *        Open a TCP Server to receive connections from
 *        DLMSoTCP
 *
 * \param dlmsotcp_port: DLMSoTCP TCP Port
 */
int prime_dmlsotcp_start(int dlmsotcp_port)
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
  PRIME_LOG(LOG_DBG,"prime_dlmsotcp_start dlmsotcp_port=0x%08X\r\n",dlmsotcp_port);

	ret = prime_tcp_check_port(dlmsotcp_port);
  if (ret){
     PRIME_LOG(LOG_ERR,"DLMSoTCP Port is not available\r\n");
     return ret;
  }

  socket_dlmsotcp = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_dlmsotcp < 0)
  {
     PRIME_LOG_PERROR(LOG_ERR,"DLMSoTCP: Cannot open socket:");
     return  ERROR_DLMS_TCP_SOCKET;
  }

  /* Allow socket descriptor to be reuseable */
  ret = setsockopt(socket_dlmsotcp, SOL_SOCKET, SO_REUSEADDR, &i_on, sizeof(i_on));
  if (ret < 0){
     PRIME_LOG_PERROR(LOG_ERR,"DLMSoTCP: setsockopt() failed:");
     close(socket_dlmsotcp);
     return ERROR_DLMSoTCP_SOCKETOPT;
  }

  /*
     Set socket to be non-blocking.  All of the sockets for
     the incoming connections will also be non-blocking since
     they will inherit that state from the listening socket.
  */
  /*i_rc = ioctl(socket_dlmsotcp, FIONBIO, (char *)&i_on);
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
  sock_addr.sin_port        = htons(dlmsotcp_port);
  ret = bind(socket_dlmsotcp, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
  if (ret < 0){
     PRIME_LOG_PERROR(LOG_ERR,"DLMSoTCP TCP: bind() failed\r\n");
     close(socket_dlmsotcp);
     return ERROR_DLMSoTCP_BIND;
  }

  /* Set the listen backlog */
  ret = listen(socket_dlmsotcp, DLMS_LISTEN_BACKLOG);
  if (ret < 0){
     PRIME_LOG_PERROR(LOG_ERR,"DLMSoTCP: listen() failed\r\n");
     close(socket_dlmsotcp);
     return ERROR_DLMSoTCP_LISTEN;
  }

  // Create DLMSoTCP thread
  pthread_attr_init( &dlmsotcp_thread_attr );
  pthread_attr_setdetachstate( &dlmsotcp_thread_attr, PTHREAD_CREATE_DETACHED );
  /* Create a thread which handle connections to DLMSoTCP */
  if (pthread_create(&dlmsotcp_thread, NULL, prime_dlmsotcp_thread, NULL)) {
      PRIME_LOG_PERROR(LOG_ERR,"DLMSoTCP: Error creating Thread\n");
      return ERROR_DLMSoTCP_THREAD;
  }
	PRIME_LOG(LOG_INFO,"Started logging on DLMSoTCP port %d\r\n",dlmsotcp_port);
  pthread_attr_destroy(&dlmsotcp_thread_attr);
  return SUCCESS;
}
#endif

/*
 * \brief Print DLMS Message
 * \param prime_dlms_list   -> PRIME DLMS messages List
 * \param px_buf            -> DLMS message to add
 * \param
 * \return -
 */
int prime_dlmsotcp_print_dlms_msg(dlms_msg * msg)
{
    PRIME_DLMSOTCP_LOG(LOG_DBG, "DLMS Message:\r\n *Timestamp = %u\r\n *Retries = %u\r\n *Destination = %d\r\n *LSAP = 0x%04X\r\n * Data Length = %d\r\n * Data = 0x", msg->ui_timestamp,  \
                                                                                                                           msg->us_retries,            \
                                                                                                                           msg->us_dst,                \
                                                                                                                           msg->us_lsap,               \
                                                                                                                           msg->us_length);
    for (int i=0;i<msg->us_length;i++)
        PRIME_DLMSOTCP_LOG_NOSTAMP(LOG_DBG,"%02X",msg->data[i]);
    PRIME_DLMSOTCP_LOG_NOSTAMP(LOG_DBG,"\r\n");
    return 0;
}

/*
 * \brief Push a new PRIME DLMS Message on the list
 * \param prime_dlms_list   -> PRIME DLMS messages List
 * \param px_buf            -> DLMS message to add
 * \param
 *
 * \return -
 */
int prime_dlmsotcp_push_dlms_msg(dlms_msg_list *dlms_list, dlms_msg *msg)
{
/*********************************************************
*       Vars                                             *
*********************************************************/
dlms_msg * p_dlms_msg;
/*********************************************************
*       Code                                             *
*********************************************************/
   pthread_mutex_lock(&prime_dlmsotcp_mutex);
   if (dlms_list->count == NUM_MAX_DLMSOTCP_MSG){
	     PRIME_DLMSOTCP_LOG(LOG_ERR,"Maximum Number of DLMSoTCP Messages\r\n");
	     pthread_mutex_unlock(&prime_dlmsotcp_mutex);
	     return ERROR_DLMSoTCP_MAX_MSGS;
   }
   /// Initial Malloc
   p_dlms_msg = (dlms_msg *) malloc(sizeof(dlms_msg));
   if (p_dlms_msg == (dlms_msg *) NULL){
       PRIME_DLMSOTCP_LOG(LOG_ERR,"Impossible to allocate space for a new PRIME DLMS Message\r\n");
       pthread_mutex_unlock(&prime_dlmsotcp_mutex);
       return ERROR_DLMSoTCP_MALLOC;
   }
   // Initialize Entry
   memset(p_dlms_msg, 0, sizeof(dlms_msg));
   memcpy(p_dlms_msg, msg, sizeof(dlms_msg));
   /// Include DLMS message on the queue
   mchp_list_add_tail(p_dlms_msg, &dlms_list->list);
   dlms_list->count++;
   PRIME_DLMSOTCP_LOG(LOG_INFO, "New DLMS message added to the queue (%d)\r\n",dlms_list->count);
   prime_dlmsotcp_print_dlms_msg(p_dlms_msg);
   pthread_mutex_unlock(&prime_dlmsotcp_mutex);
   return dlms_list->count;
}

/*
 * \brief POP a PRIME DLMS message from the list
 * \param p_prime_sn_device : pointer to PRIME SN Device
 *
 * \return -
 */
int prime_dlmsotcp_get_dlms_msg(dlms_msg_list *dlms_list, dlms_msg *msg)
{
/*********************************************************
*       Local Vars                                       *
*********************************************************/
dlms_msg *entry;
/*********************************************************
*       Code                                             *
*********************************************************/
   pthread_mutex_lock(&prime_dlmsotcp_mutex);
   /// Check if Queue is list_empty
   if (mchp_list_empty(&dlms_list->list)){
      return 0;
   }
   entry = mchp_list_first(&dlms_list->list,dlms_msg);
   memcpy(msg,entry,sizeof(dlms_msg));
   prime_dlmsotcp_print_dlms_msg(msg);
   pthread_mutex_unlock(&prime_dlmsotcp_mutex);
   return dlms_list->count;
}

/*
 * \brief POP a PRIME DLMS message from the list
 * \param p_prime_sn_device : pointer to PRIME SN Device
 *
 * \return -
 */
int prime_dlmsotcp_pop_dlms_msg(dlms_msg_list *dlms_list, dlms_msg *msg)
{
/*********************************************************
*       Local Vars                                       *
*********************************************************/
dlms_msg *entry;
/*********************************************************
*       Code                                             *
*********************************************************/
   pthread_mutex_lock(&prime_dlmsotcp_mutex);
   /// Check if Queue is list_empty
   if (mchp_list_empty(&dlms_list->list)){
      return 0;
   }
   entry = mchp_list_first(&dlms_list->list,dlms_msg);
   memcpy(msg,entry,sizeof(dlms_msg));
   /// Delete DLMS Message
   mchp_list_del(entry);
   /// Free Memory on PRIME Network List
   free(entry);
   dlms_list->count--;
   prime_dlmsotcp_print_dlms_msg(msg);
   PRIME_DLMSOTCP_LOG(LOG_INFO, "Deleted DLMS Message (%d)\r\n",dlms_list->count);
   pthread_mutex_unlock(&prime_dlmsotcp_mutex);
   return dlms_list->count;
}

/**
 * \brief Set autoclose flag for a 432-connection
 */
static void _add_device_auto_close(uint16_t us_device)
{
/*********************************************************
*       Local Vars                                       *
*********************************************************/
prime_sn * p_prime_sn;
/*********************************************************
*       Code                                             *
*********************************************************/
  p_prime_sn = prime_network_find_sn_cl432_address(&prime_network,us_device);
  if (p_prime_sn == (prime_sn *)NULL){
    // Service Node still not seen before
  }else{
    prime_network_mutex_lock();
    p_prime_sn->autoclose_enabled++;
    prime_network_mutex_unlock();
  }
}

/**
 * \brief clear autoclose flag for a 432-connection
 */
static void _remove_device_auto_close(uint16_t us_device)
{
/*********************************************************
*       Local Vars                                       *
*********************************************************/
prime_sn * p_prime_sn;
/*********************************************************
*       Code                                             *
*********************************************************/
  p_prime_sn = prime_network_find_sn_cl432_address(&prime_network,us_device);
  if (p_prime_sn == (prime_sn *)NULL){
    // Service Node still not seen before
  }else{
    prime_network_mutex_lock();
    p_prime_sn->autoclose_enabled = 0;
    prime_network_mutex_unlock();
  }
}
#if 0
/**
 * \brief test if a 432-connection has enabled the autoclose flag (DLMSoTCP status)
 */
static int _is_enabled_auto_close(uint16_t cl432_address)
{
  /*********************************************************
  *       Local Vars                                       *
  *********************************************************/
  prime_sn * p_prime_sn;
  /*********************************************************
  *       Code                                             *
  *********************************************************/
    p_prime_sn = prime_network_find_sn_cl432_address(&prime_network,cl432_address);
    if (p_prime_sn == (prime_sn *)NULL){
      // Service Node still not seen before
      return 0;
    }else{
      if (p_prime_sn->cl432Conn.connAddress == CL_432_INVALID_ADDRESS) {
          return 0;
      } else {
          return (p_prime_sn->autoclose_enabled > 0) ? 1 : 0;
      }
    }
}
#endif

static void _send_queued_432_data()
{
	uint32_t now = time(NULL);

  // Get Message to be sent
  dlms_msg px_msg_mio;
  dlms_msg *px_msg = &px_msg_mio;
  prime_dlmsotcp_pop_dlms_msg(&prime_dlmsotcp_pending_cfm_msg_list, px_msg);
  px_msg->ui_timestamp = now;
  px_msg->us_retries   = MAX_432_SEND_RETRY;
  /* build dl_432_buffer... */
  dl_432_buffer_t msg;
  memcpy(msg.dl.buff, px_msg->data, px_msg->us_length);
  msg.dl.lsap = px_msg->us_lsap;
  //send data
  uint8_t uc_dst_lsap;
  if (px_msg->us_dst == 0xFFF) {
      uc_dst_lsap = 0;
  } else {
      uc_dst_lsap = 1;
  }
  prime_cl_432_dl_data_request(uc_dst_lsap, px_msg->us_lsap, px_msg->us_dst, &msg, px_msg->us_length,0);
  if (uc_dst_lsap == 1) {
      PRIME_DLMSOTCP_LOG(LOG_INFO,"SEND QUEUED PACKET: cl_432 send: #%d, lsap %d, dest %d\r\n", px_msg->us_retries, px_msg->us_lsap, px_msg->us_dst);
  } else {
      PRIME_DLMSOTCP_LOG(LOG_INFO,"SEND QUEUED BORADCAST PACKET: cl_432 send: #%d, lsap %d, dest %d\r\n", px_msg->us_retries, px_msg->us_lsap, px_msg->us_dst);
  }
}


void _dlmsotcp_cl_432_dl_data_cfm_cb(uint8_t uc_dst_lsap, uint8_t uc_src_lsap, uint16_t us_dst_address, uint8_t uc_tx_status)
{
  dlms_msg px_msg_mio;
  dlms_msg *px_msg = &px_msg_mio;

	PRIME_DLMSOTCP_LOG(LOG_DEBUG,"_dlmsotcp_cl_432_dl_data_cfm_cb\r\n");

  if (prime_dlmsotcp_pending_cfm_msg_list.count == 0){
     /* if this queue is empty, do nothing */
     return;
  }
  prime_dlmsotcp_pop_dlms_msg(&prime_dlmsotcp_pending_cfm_msg_list, px_msg);
  if (px_msg->us_dst != us_dst_address) {
     PRIME_DLMSOTCP_LOG(LOG_ERR,"TX CONFIRM DOES NOT MATCH QUEUE Destination:  %d != %d.\r\n", px_msg->us_dst,us_dst_address);
  }

	switch (uc_tx_status) {
		case 0:
			/* Do nothing, msg  OK*/
      PRIME_DLMSOTCP_LOG(LOG_INFO,"TX DATA OK: Pending %d \r\n", prime_dlmsotcp_pending_cfm_msg_list.count);
			break;
		case CL_432_TX_STATUS_TIMEOUT:
			PRIME_DLMSOTCP_LOG(LOG_INFO,"TX DATA ERROR CL_432_TX_STATUS_TIMEOUT: dest %d. retry later\r\n",
								 us_dst_address);
			/* Do not retry this message. Concentrator must handle timeouts! */
			break;
		case CL_432_TX_STATUS_ERROR_BAD_ADDRESS:
			PRIME_DLMSOTCP_LOG(LOG_INFO,"TX DATA ERROR CL_432_TX_STATUS_ERROR_BAD_ADDRESS: dest %d. retry later\r\n",
					 us_dst_address);

			break;
		case CL_432_TX_STATUS_ERROR_BAD_HANLDER:
			PRIME_DLMSOTCP_LOG(LOG_INFO,"TX DATA ERROR CL_432_TX_STATUS_ERROR_BAD_HANLDER: dest %d. REMOVE MESSAGE\r\n",
					 us_dst_address);
			break;
		case CL_432_TX_STATUS_PREVIOUS_COMM:
			PRIME_DLMSOTCP_LOG(LOG_INFO,"TX DATA ERROR CL_432_TX_STATUS_PREVIOUS_COMM: dest %d. retry later\r\n",
					 us_dst_address);
			break;
		default:
			PRIME_DLMSOTCP_LOG(LOG_INFO,"TX DATA ERROR %d: dest %d. retry later\r\n",
					 uc_tx_status,us_dst_address);
			break;
	}

	//Are there any pending messages? Send!
  if (prime_dlmsotcp_pending_cfm_msg_list.count > 0){
		 _send_queued_432_data();
	}
}

/**
 * \brief Updates connection status and sends the New Device DLMSoTCP message
 *      to the client.
 */
void _dlmsotcp_cl_432_join_ind_cb(uint8_t *puc_device_id, uint8_t uc_device_id_len,
                                 uint16_t us_dst_address, uint8_t *puc_mac)
{
  /*********************************************************
  *       Local Vars                                       *
  *********************************************************/
  prime_sn * p_prime_sn;
	uint8_t uc_length;
	uint8_t puc_dlms_msg[512];
  /*********************************************************
  *       Code                                             *
  *********************************************************/
	PRIME_DLMSOTCP_LOG(LOG_DBG,"_dlmsotcp_cl_432_join_ind_cb eui48=%s dst_address=0x%04X device_id=%s\r\n",eui48_to_str(puc_mac,NULL),us_dst_address,puc_device_id);

	/* Add node to list */
	if (us_dst_address >= us_max_index_connected_node) {
		 us_max_index_connected_node = us_dst_address + 1;
	}
  p_prime_sn = prime_network_find_sn(&prime_network,puc_mac);
  if (p_prime_sn){
    // Service Node still not seen before
  }else{
    prime_network_mutex_lock();
    p_prime_sn->cl432Conn.connState = CL432_CONN_STATE_OPEN;
    p_prime_sn->cl432Conn.connAddress = us_dst_address;
    memset(p_prime_sn->cl432Conn.connSerialNumber,'\0',16);
    memcpy(p_prime_sn->cl432Conn.connSerialNumber,puc_device_id,uc_device_id_len);
    p_prime_sn->cl432Conn.connLenSerial = uc_device_id_len;
    memcpy(p_prime_sn->cl432Conn.connMAC,puc_mac,6);
    prime_network_mutex_unlock();
  }

  uc_length = (uc_device_id_len > 16) ? 16 : uc_device_id_len;

	/* Forward notification to concentrator */
	if (p_prime_sn->autoclose_enabled == FALSE) {
		if ((g_concentrator_fd != 0) && (i_enable_notifications)) {
			/* DLMSoTCP Version 0x0001 */
			puc_dlms_msg[0] = 0;
			puc_dlms_msg[1] = 1;

	        /* CUSTOM MESSAGE Device BN --> DC */
			puc_dlms_msg[2] = 0;
			puc_dlms_msg[3] = 0;

			/*  CUSTOM MESSAGE Association BN --> DC    */
			puc_dlms_msg[4] = 0;
			puc_dlms_msg[5] = 1;

			/* Lenght of the DLMS data field */
			puc_dlms_msg[6] = 0;
			puc_dlms_msg[7] = 12 + uc_length;    /* MAXIMUM 28 = 12 + LEN_432_SERIAL_NUMBER */

			/* NEW_DEVICE_NOTIFICATION (0x01) */
			puc_dlms_msg[8] = 1;

			/* Source DeviceID */
			puc_dlms_msg[9] = us_dst_address >> 8;
			puc_dlms_msg[10] = us_dst_address & 0xFF;    /* Source DeviceID */

			/* Flags with the capabilities of the device--> 0x0001 : ARQ  */
			puc_dlms_msg[11] = 0;
			puc_dlms_msg[12] = 1;

			/* Length of the DLMS identifier (as in ZIV000000). IBERDROLA => 13*/
			puc_dlms_msg[13] = uc_length;

			/* DLMS identifier of the reported device */
			memcpy(puc_dlms_msg + 14, puc_device_id, uc_length);

			/* EUI48 of the reported device */
			memcpy(puc_dlms_msg + 14 + uc_length, puc_mac, 6);

			uc_length = 20 + uc_length;

      PRIME_DLMSOTCP_LOG(LOG_DBG,"[DLMSoTCP] << 0x");
      for (int i=0;i<uc_length;i++)
         PRIME_DLMSOTCP_LOG_NOSTAMP(LOG_DBG,"%02X",puc_dlms_msg[i]);
      PRIME_DLMSOTCP_LOG_NOSTAMP(LOG_DBG,"\r\n");

			/* Send notification to Concentrator */
			write(g_concentrator_fd, puc_dlms_msg, uc_length);

	    }
	}
}

void _dlmsotcp_cl_432_leave_ind_cb(uint16_t us_dst_address)
{
  /*********************************************************
  *       Local Vars                                       *
  *********************************************************/
  prime_sn * p_prime_sn;
  uint8_t uc_length;
	uint8_t puc_dlms_msg[512];

  /*********************************************************
  *       Code                                             *
  *********************************************************/

	PRIME_DLMSOTCP_LOG(LOG_DBG,"_dlmsotcp_cl_432_leave_ind_cb us_dst_address=0x%04X\r\n",us_dst_address);
	/* remove node from list */
  p_prime_sn = prime_network_find_sn_cl432_address(&prime_network,us_dst_address);
  if (p_prime_sn == (prime_sn *)NULL){
    // Service Node still not seen before
  }else{
    p_prime_sn->cl432Conn.connAddress = CL_432_INVALID_ADDRESS;
    p_prime_sn->autoclose_enabled = FALSE;
  }

	/* fordward notification */
	if ((g_concentrator_fd > 0) && (i_enable_notifications)) {
		if (!p_prime_sn->autoclose_enabled) {
			/* Version 0x0001 */
			puc_dlms_msg[0] = 0;
			puc_dlms_msg[1] = 1;

			/* CUSTOM MESSAGE Device BN --> DC */
			puc_dlms_msg[2] = 0;
			puc_dlms_msg[3] = 0;

			/* CUSTOM MESSAGE Association BN --> DC    */
			puc_dlms_msg[4] = 0;
			puc_dlms_msg[5] = 1;

			/* Lenght of the DLMS data field */
			puc_dlms_msg[6] = 0;
			puc_dlms_msg[7] = 3;  /* Fixed */

			/* REMOVE_DEVICE_NOTIFICATION (0x02) */
			puc_dlms_msg[8] = 2;

			/* Source DeviceID */
			puc_dlms_msg[9] = (us_dst_address >> 8) & 0xFF;
			puc_dlms_msg[10] = us_dst_address & 0xFF;

			uc_length = 11;

			/* Send notification to Concentrator */
      PRIME_DLMSOTCP_LOG(LOG_DBG,"[DLMSoTCP] << 0x");
      for (int i=0;i<uc_length;i++)
         PRIME_DLMSOTCP_LOG_NOSTAMP(LOG_DBG,"%02X",puc_dlms_msg[i]);
      PRIME_DLMSOTCP_LOG_NOSTAMP(LOG_DBG,"\r\n");
			write(g_concentrator_fd, puc_dlms_msg, uc_length);
		}
	}
}

/**
 * \brief Send the list of 432 nodes to the remote client.
 *      When a new client connection is accepted, it is needed
 *      to notify the current 432 connection status.
 */
static void _send_list_432_connections(void)
{
/*********************************************************
*       Local Vars                                       *
*********************************************************/
mchp_list *entry, *tmp;
prime_sn * p_prime_sn;
uint8_t uc_length;
uint8_t puc_dlms_msg[512];
/*********************************************************
*       Code                                             *
*********************************************************/

    PRIME_DLMSOTCP_LOG(LOG_DBG,"[DLMSoTCP] - _send_list_432_connections - %d, %d",g_concentrator_fd,i_enable_notifications);
    if ((g_concentrator_fd != 0) && (i_enable_notifications)) {
      if (prime_network_sn == 0){
         PRIME_DLMSOTCP_LOG(LOG_ERR,"No PRIME SN Present\n");
      }else{
         list_for_each_safe(entry, tmp, &prime_network) {
            p_prime_sn = list_entry(entry, prime_sn, list);
            if ((p_prime_sn->cl432Conn.connState == CL432_CONN_STATE_OPEN) && (p_prime_sn->cl432Conn.connAddress != CL_432_INVALID_ADDRESS));
                PRIME_DLMSOTCP_LOG(LOG_DBG,"[DLMSoTCP] - _send_list_432_connections - dst_address - %d\r\n",p_prime_sn->cl432Conn.connAddress);
        				/* DLMSoTCP Version 0x0001 */
        				puc_dlms_msg[0] = 0;
        				puc_dlms_msg[1] = 1;

        				/* CUSTOM MESSAGE Device BN --> DC */
        				puc_dlms_msg[2] = 0;
        				puc_dlms_msg[3] = 0;

        				/*  CUSTOM MESSAGE Association BN --> DC    */
        				puc_dlms_msg[4] = 0;
        				puc_dlms_msg[5] = 1;

        				/* Lenght of the DLMS data field */
        				puc_dlms_msg[6] = 0;
        				puc_dlms_msg[7] = 12 + p_prime_sn->cl432Conn.connLenSerial;    /* MAXIMUM 28 = 12 + LEN_432_SERIAL_NUMBER */

        				/* NEW_DEVICE_NOTIFICATION (0x01) */
        				puc_dlms_msg[8] = 1;

        				/* Source DeviceID */
        				puc_dlms_msg[9] = p_prime_sn->cl432Conn.connAddress >> 8;
        				puc_dlms_msg[10]= p_prime_sn->cl432Conn.connAddress & 0xFF;

        				/* Flags with the capabilities of the device--> 0x0001 : ARQ  */
        				puc_dlms_msg[11] = 0;
        				puc_dlms_msg[12] = 1;

        				/* Length of the DLMS identifier (as in ZIV000000). IBERDROLA => 13*/
        				puc_dlms_msg[13] =  p_prime_sn->cl432Conn.connLenSerial;

        				/* DLMS identifier of the reported device */
        				memcpy(puc_dlms_msg + 14, p_prime_sn->cl432Conn.connSerialNumber, p_prime_sn->cl432Conn.connLenSerial);

        				/* EUI48 of the reported device */
        				memcpy(puc_dlms_msg + 14 + p_prime_sn->cl432Conn.connLenSerial, p_prime_sn->cl432Conn.connMAC, 6);

        				uc_length = 20 + p_prime_sn->cl432Conn.connLenSerial;

        				/* Send notification to Concentrator */
                PRIME_DLMSOTCP_LOG(LOG_DBG,"[DLMSoTCP] << 0x");
                for (int i=0;i<uc_length;i++)
                   PRIME_DLMSOTCP_LOG_NOSTAMP(LOG_DBG,"%02X",puc_dlms_msg[i]);
                PRIME_DLMSOTCP_LOG_NOSTAMP(LOG_DBG,"\r\n");
        				write(g_concentrator_fd, puc_dlms_msg, uc_length);
    			}
        }
    }
}

/**
 * \brief New DLMS data arrived, forward it to the client
 */
void _dlmsotcp_cl_432_dl_data_ind_cb(uint8_t uc_dst_lsap, uint8_t uc_src_lsap, uint16_t us_dst_address,
		uint16_t src_address, uint8_t *puc_data, uint16_t uc_lsdu_len, uint8_t uc_link_class)
{
	uint16_t us_length;
	uint8_t puc_dlms_msg[MAX_LENGTH_432_DATA];

	PRIME_DLMSOTCP_LOG(LOG_DBG,"\n_dlmsotcp_cl_432_dl_data_ind_cb  SRC=%d, DEST=%d, LEN=%d\r\n", uc_src_lsap, us_dst_address, uc_lsdu_len);

	(void)(uc_dst_lsap);
	(void)(uc_src_lsap);
	(void)(uc_dst_lsap);
	(void)(uc_lsdu_len);
	(void)(us_dst_address);
	(void)(uc_link_class);

	/* Forward dlms data messages */
	if ((g_concentrator_fd != 0) && (i_enable_notifications)) {
		/* DLMSoTCP Version 0x0001 */
		puc_dlms_msg[0] = 0;
		puc_dlms_msg[1] = 1;

		/* Source DeviceID */
		puc_dlms_msg[2] = src_address >> 8;
		puc_dlms_msg[3] = src_address & 0xFF;

		/* From SPEC: Association ID for the data going BN --> DC  */
		/* Association   */
		puc_dlms_msg[4] = 0;
		puc_dlms_msg[5] = uc_dst_lsap;

		/* Length of the DLMS data field */
		puc_dlms_msg[6] = uc_lsdu_len >> 8;
		puc_dlms_msg[7] = uc_lsdu_len & 0xFF;

		/* DLMS MESSAGE */

		/* If it is an empty message, it will not be forwarded */
		if (uc_lsdu_len > 0) {
			memcpy(puc_dlms_msg + 8, puc_data, uc_lsdu_len);

			/* LENGTH DLMS + Ticket67 headers (8) */
			us_length = uc_lsdu_len + 8;

			/* Send notification to Concentrator */
      PRIME_DLMSOTCP_LOG(LOG_DBG,"[DLMSoTCP] << 0x");
      for (int i=0;i<us_length;i++)
         PRIME_DLMSOTCP_LOG_NOSTAMP(LOG_DBG,"%02X",puc_dlms_msg[i]);
      PRIME_DLMSOTCP_LOG_NOSTAMP(LOG_DBG,"\r\n");
			write(g_concentrator_fd, puc_dlms_msg, us_length);
		}
	}
}

void _dlmsotcp_close_432()
{
/*********************************************************
*       Local Vars                                       *
*********************************************************/
mchp_list *entry, *tmp;
prime_sn * p_prime_sn;
/*********************************************************
*       Code                                             *
*********************************************************/
    if (prime_network_sn == 0){
  	   PRIME_DLMSOTCP_LOG(LOG_ERR,"No PRIME SN Present\n");
    }else{
       list_for_each_safe(entry, tmp, &prime_network) {
          prime_network_mutex_lock();
          p_prime_sn = list_entry(entry, prime_sn, list);
          if ((p_prime_sn->cl432Conn.connState == CL432_CONN_STATE_OPEN) && (p_prime_sn->autoclose_enabled)) {
             prime_cl_432_release_request(p_prime_sn->cl432Conn.connAddress);
             p_prime_sn->cl432Conn.connState = CL432_CONN_STATE_CLOSED;
             p_prime_sn->cl432Conn.connAddress = CL_432_INVALID_ADDRESS;
             p_prime_sn->autoclose_enabled = FALSE;
          }
          prime_network_mutex_unlock();
       }
    }
}

void base_node_dlmsotcp_auto_close_session()
{
  /*********************************************************
  *       Local Vars                                       *
  *********************************************************/
  mchp_list *entry, *tmp;
  prime_sn * p_prime_sn;
  /*********************************************************
  *       Code                                             *
  *********************************************************/

    //Auto Close the Current Session
    close(g_concentrator_fd);
    /* Init auto_close status removing 4-32 information on all the Network Structure */
    if (prime_network_sn == 0){
       PRIME_DLMSOTCP_LOG(LOG_ERR,"No PRIME SN Present\n");
    }else{
       list_for_each_safe(entry, tmp, &prime_network) {
          prime_network_mutex_lock();
          p_prime_sn = list_entry(entry, prime_sn, list);
          p_prime_sn->cl432Conn.connAddress = CL_432_INVALID_ADDRESS;
          p_prime_sn->autoclose_enabled = FALSE;
          prime_network_mutex_unlock();
       }
    }
}

int _dlmsotcp_process()
{
	if (g_concentrator_fd == 0){
    PRIME_DLMSOTCP_LOG(LOG_ERR,"socket() closed\r\n");
		return -1; /* Nothing to do, socket closed */
  }

	/* Read data from concentrator*/
	ssize_t i_bytes;
	i_bytes = read(g_concentrator_fd,uc_rx_buffer, MAX_BUFFER_SIZE);
	if (i_bytes > 0) {
		dlmsotcp_RxProcess(uc_rx_buffer,i_bytes);
	}
	return i_bytes;
}

/********************************************************
* \brief Thread to handle connections to DLMS over TCP Server
*
* \param  thread_parameters
* \return
********************************************************/
void * dlmsotcp_thread(void * thread_parameters)
{
/*********************************************************
*       Vars
*********************************************************/
//int i_server_sd;
int i;
fd_set master_set, working_set;
int i_max_fd;
struct timeval timeout;
/*********************************************************
*       Code
*********************************************************/
/* Open TCP server socket */
i_server_sd =  open_dlmsotcp_server(tcp_port_dlmsotcp);
if ( i_server_sd == 0) {
    PRIME_DLMSOTCP_LOG(LOG_ERR,"Cannot open Server socket\r\n");
    pthread_exit(NULL);
} else {
    PRIME_DLMSOTCP_LOG(LOG_INFO,"DLMSoTCP server up (%d)\r\n",i_server_sd);
}

/* Prepare list of file descriptor to monitor with "select" */
FD_ZERO(&master_set);
i_max_fd = i_server_sd;

FD_SET(i_server_sd, &master_set);

while(1) {
  int i_sel_ret;

  memcpy(&working_set, &master_set, sizeof(master_set));

  timeout.tv_sec  = 1;
  timeout.tv_usec = 0;

  //Wait on file descriptors for data available
  i_sel_ret = select(i_max_fd + 1, &working_set, NULL, NULL, &timeout);

  if (i_sel_ret < 0) {
    PRIME_DLMSOTCP_LOG(LOG_ERR,"Error. Select failed\r\n");
    /* Force close TCP connection */
    if (g_concentrator_fd > 0){
       close(g_concentrator_fd);
    }
    //close(i_server_sd);
    pthread_exit(NULL);
  } else if (i_sel_ret == 0 ) {
    /* Tick-Tack... Timeout waiting */
    PRIME_DLMSOTCP_LOG(LOG_DBG,".");
    /* Process USI */
    /* addUsi_Process(); */
    usleep(1000);
  } else {
    /* process file descriptors */
    for (i=0; i <= i_max_fd  &&  i_sel_ret > 0; ++i) {
      if (FD_ISSET(i, &working_set)) {
          if ( i == g_concentrator_fd) {
              int n = 0;
              /* check if socket is valid for read */
              ioctl(g_concentrator_fd, FIONREAD, &n);
              if (n== 0){
                  /* Socket has been closed... */
                  PRIME_DLMSOTCP_LOG(LOG_ERR,"Connection Error, closing (%d)....\r\n",g_concentrator_fd);
                  /* Error, close concentrator session */
                  close(g_concentrator_fd);

                  /* Clear concentrator session from select list */
                  FD_CLR(g_concentrator_fd, &master_set);
                  if (g_concentrator_fd == i_max_fd)
                    i_max_fd--;
                  g_concentrator_fd = 0;

                  /* Force 432 connection close for current communications */
                  _dlmsotcp_close_432();
              }
              /* Process concentrator inputs*/
              _dlmsotcp_process();
          }else if ( i == i_server_sd) {
            /* Handle incoming connections from concentrators*/
            if (g_concentrator_fd == 0) {
              int i_flag;
              /* Ready for new concentrator connection, accept */
              g_concentrator_fd = accept(i_server_sd, NULL, NULL);
              PRIME_DLMSOTCP_LOG(LOG_INFO,"New DLMSoTCP Connection established (%d)\r\n",g_concentrator_fd);

              /* Configure socket for miminum delay */
              i_flag = 1;
              setsockopt(g_concentrator_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &i_flag, sizeof(int));

              /*add new socket to select list */
              i_max_fd = (g_concentrator_fd > i_max_fd)? g_concentrator_fd: i_max_fd;
              FD_SET(g_concentrator_fd, &master_set);

            }else {
              /* We do not accept more than one concentrator... */
              /* close inmediately next connections */
              PRIME_DLMSOTCP_LOG(LOG_ERR,"Busy, concentrator connected already\r\n");
              int fd = accept(i_server_sd, NULL, NULL);
              if (fd) {
                close(fd);
              }else {
                  /*Error in server socket, exit! */
                  PRIME_DLMSOTCP_LOG(LOG_ERR, "ERROR in SERVER socket, exit!\r\n");
                  if (g_concentrator_fd > 0)
                    close(g_concentrator_fd);
                  //close(i_server_sd);
                  //pthread_exit(NULL); //return ; /* return ERROR_DLMSoTCP_SOCKET; */
              }
            }
          }
      }
    }/* end for*/
  }
}/* while server file descriptor ok...*/

}

int base_node_dlmsotcp_start()
{

	/* Init dlmsotcp app */
	base_node_dlmsotcp_init();

  // Create tcp_sniffer thread to handle USI Frames on serial port
  pthread_attr_init( &prime_dlmsotcp_thread_attr );
  pthread_attr_setdetachstate( &prime_dlmsotcp_thread_attr, PTHREAD_CREATE_DETACHED );
  /* Create a thread which handle connections to TCP Sniffer */
  if (pthread_create(&prime_dlmsotcp_thread, NULL, dlmsotcp_thread, NULL)) {
      PRIME_DLMSOTCP_PRINTF_PERROR("DLMSoTCP: Error creating Thread\r\n");
      return ERROR_DLMSoTCP_THREAD;
  }
  PRIME_DLMSOTCP_LOG(LOG_INFO,"Started DLMSoTCP on TCP port %d\r\n",tcp_port_dlmsotcp);
  pthread_attr_destroy(&prime_dlmsotcp_thread_attr);
  return SUCCESS;


	return 0;
}

/**
 * \brief Stops DLMSoTCP application. This application
 * uses 432 callback interface and management protocol.
 */
int base_node_dlmsotcp_stop()
{
  PRIME_DLMSOTCP_LOG(LOG_DEBUG,"Closing file descriptors %d,%d\r\n",g_concentrator_fd,i_server_sd);
  close(g_concentrator_fd);
  close(i_server_sd);
  return SUCCESS;
}

/**
 * \brief Initialiazion of the DLMSoTCP application. This application
 * uses 432 callback interface and management protocol.
 */

int base_node_dlmsotcp_init()
{
  /*********************************************************
  *       Local Vars                                       *
  *********************************************************/
  mchp_list *entry, *tmp;
  prime_sn * p_prime_sn;
  /*********************************************************
  *       Code                                             *
  *********************************************************/
  prime_dlmsotcp_set_loglevel(PRIME_LOG_INFO);
	/* Init auto_close status removing 4-32 information on all the Network Structure */
  if (prime_network_sn == 0){
	   PRIME_DLMSOTCP_LOG(LOG_ERR,"No PRIME SN Present\n");
  }else{
     list_for_each_safe(entry, tmp, &prime_network) {
        prime_network_mutex_lock();
        p_prime_sn = list_entry(entry, prime_sn, list);
        prime_cl_432_release_request(p_prime_sn->cl432Conn.connAddress);
        p_prime_sn->cl432Conn.connState = CL432_CONN_STATE_CLOSED;
        p_prime_sn->cl432Conn.connAddress = CL_432_INVALID_ADDRESS;
        p_prime_sn->autoclose_enabled = FALSE;
        prime_network_mutex_unlock();
     }
  }

  // Initialize PRIME Network List
  INIT_LIST_HEAD(&prime_dlmsotcp_free_msg_list.list);
  prime_dlmsotcp_free_msg_list.count = 0;
  INIT_LIST_HEAD(&prime_dlmsotcp_pending_cfm_msg_list.list);
  prime_dlmsotcp_pending_cfm_msg_list.count = 0;

	/* Once that we have the callbacks in place, we query the list of connected nodes.*/
	/* USI must be configured first */
	/* Use Enhanced to query 8250 list (432 connected nodes) */
	//uint16_t us_it = 0; /* query first element */
	//mngLay_NewMsg(MNGP_PRIME_EN_PIBQRY);
	//mngLay_AddGetPibListEnQuery(PIB_432_CONNECTED_NODES_LIST ,MNGP_PRIME_ENHANCED_LIST_MAX_RECORDS,(uint8_t*)&us_it);
	//mngLay_SendMsg();
  struct TmacGetConfirm x_pib_confirm;
  prime_cl_null_mlme_list_get_request_sync(PIB_432_LIST_NODES,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  if (x_pib_confirm.m_u8Status != MLME_RESULT_DONE){
      PRIME_DLMSOTCP_LOG(LOG_ERR,"PRIME PIB-MAC cl432ListNodes request failed\r\n");
      return -1;
  }
	return 0;
}
/**
 * \brief Process messages received from the concentrator.
 * Unpack dlmsotcp protocol headers and forward data to the Base Node.
 */
int32_t dlmsotcp_RxProcess(uint8_t* buf, uint16_t buflen)
{
    uint16_t usVersion;
    uint16_t usSource;
    uint16_t usDest;
    uint16_t usDlmsLen;
    uint8_t *pcDlmsBuf;

__dlmsMsg:
	if (buflen < 8) {
        return ERROR_DLMSoTCP_BUFLEN; /* discard, nothing to do? */
    }

    PRIME_DLMSOTCP_LOG(LOG_DBG,"[DLMSoTCP] >> 0x");
    for (int i=0;i<buflen;i++)
       PRIME_DLMSOTCP_LOG_NOSTAMP(LOG_DBG,"%02X", buf[i]);
    PRIME_DLMSOTCP_LOG_NOSTAMP(LOG_DBG,"\r\n");

    usVersion = (buf[0] << 8) + buf[1];
    usSource  = (buf[2] << 8) + buf[3];
    usDest    = (buf[4] << 8) + buf[5];
    usDlmsLen = (buf[6] << 8) + buf[7];
    pcDlmsBuf = buf + 8;

    if (usVersion != 0x1) {
        PRIME_DLMSOTCP_LOG(LOG_ERR,"DLMSoTCP Bad version\r\n");
        return ERROR_DLMSoTCP_VERSION;
    }

    if (buflen < usDlmsLen + 8 /*header*/) {
        /*mising data*/
        PRIME_DLMSOTCP_LOG(LOG_ERR,"DLMSoTCP Bad length\r\n");
        return ERROR_DLMSoTCP_MIN_LENGTH;
    }

    /* Insert in buffer if possible */
    if (usDlmsLen >= MAX_LENGTH_432_DATA) {
        PRIME_DLMSOTCP_LOG(LOG_ERR,"DLMSoTCP Bad length\r\n");
        return ERROR_DLMSoTCP_MAX_LENGTH;
    }

    if (usDlmsLen > buflen) {
        PRIME_DLMSOTCP_LOG(LOG_ERR,"DLMSoTCP Fragmented Frame\r\n");
        return ERROR_DLMSoTCP_FRAGMENTED; /* not enough data in buffer (fragmented data?) */
    }

    if ((usSource == 1) && (usDest == 0) && (usDlmsLen >= 1)) {
        /* Custom DLMSoTCP message: */
        uint8_t cmd = pcDlmsBuf[0];
        switch (cmd) {
        case 3:
            PRIME_DLMSOTCP_LOG(LOG_DBG,"[DLMSoTCP] CMD: Send List of 432 Connected Nodes\r\n");
        	  i_enable_notifications = TRUE;
            /* SEND LIST OF 432-CONNECTED NODES */
            _send_list_432_connections();
            break;

        case 4: /*DELETE*/
            PRIME_DLMSOTCP_LOG(LOG_DBG,"[DLMSoTCP] CMD: Delete\r\n");
            if (usDlmsLen == 3) {
                uint16_t deviceId = (pcDlmsBuf[1] << 8) + pcDlmsBuf[2];
                _remove_device_auto_close(deviceId);
                prime_cl_432_release_request(deviceId);
            }
            break;

        case 5: /*ENABLE AUTO CLOSE */
            PRIME_DLMSOTCP_LOG(LOG_DBG,"[DLMSoTCP] CMD: Enable Auto Close\r\n");
            if (usDlmsLen == 3) {
                int deviceId = (pcDlmsBuf[1] << 8) + pcDlmsBuf[2];
                _add_device_auto_close(deviceId);
            }
            break;
        case 6: /* DISABLE AUTO CLOSE*/
            PRIME_DLMSOTCP_LOG(LOG_DBG,"[DLMSoTCP] CMD: Disable Auto Close\r\n");
            if (usDlmsLen == 3) {
                uint16_t deviceId = (pcDlmsBuf[1] << 8) + pcDlmsBuf[2];
                _remove_device_auto_close(deviceId);
            }
            break;
        default:
            break;
        }
    }else {
        /*Forward DMLS messages */
        memcpy(dlms_432_msg.dl.buff, pcDlmsBuf, usDlmsLen);
        dlms_432_msg.dl.lsap = usSource & 0xFF;

        //Keep track of messages and handle cfm/retries
        /* get a free msg buffer */
        dlms_msg px_msg_mio;
        dlms_msg *px_msg = &px_msg_mio;
//        prime_dlmsotcp_pop_dlms_msg(&prime_dlmsotcp_free_msg_list,&px_msg_mio);
//        if (px_msg_mio) {
            uint8_t uc_dst_lsap;
            if ( usDest == 0x007F) {
                uc_dst_lsap = 0;
                px_msg->us_dst    = 0xFFF;//IEC-432 broadcast address
                px_msg->us_lsap   = 0;
            } else {
                /* fill the message data */
                px_msg->us_dst    = usDest;
                px_msg->us_lsap   = dlms_432_msg.dl.lsap;
                uc_dst_lsap = 1;
            }
            px_msg->us_length = usDlmsLen;
            memcpy(px_msg->data, dlms_432_msg.dl.buff, usDlmsLen);
            px_msg->ui_timestamp = time(NULL);
            px_msg->us_retries   = 0;
            if (prime_dlmsotcp_pending_cfm_msg_list.count == 0) {
              /* there is no messages with pending confirm... send*/
                prime_cl_432_dl_data_request(uc_dst_lsap, dlms_432_msg.dl.lsap, usDest, &dlms_432_msg, usDlmsLen, 0);
            }
            /*add message to the queue */
            prime_dlmsotcp_push_dlms_msg(&prime_dlmsotcp_pending_cfm_msg_list, px_msg);
//        } else {
//            prime_dlmsotcp_push_dlms_msg(&prime_dlmsotcp_free_msg_list, px_msg_mio);
//            PRIME_DLMSOTCP_LOG(LOG_ERR," ERROR, No more free message buffers %d-%d!!!!!\r\n", prime_dlmsotcp_free_msg_list.us_count, prime_dlmsotcp_pending_cfm_msg_list.us_count);
//            PRIME_DLMSOTCP_LOG(LOG_ERR," Discard message. This should never happen!!!!!\r\n");
//        }

        /* Check pending confirm queue for old messages...*/
        if (prime_dlmsotcp_pending_cfm_msg_list.count > 1) {
        	uint32_t ui_now = time(NULL);
          prime_dlmsotcp_get_dlms_msg(&prime_dlmsotcp_pending_cfm_msg_list, px_msg);
        	uint32_t ui_timestamp = px_msg->ui_timestamp ;
        	if ((ui_now - ui_timestamp ) > 40) {
        		/* Waiting more than 40 seconds for a confirm, delete! */
        		/* put it back in free  messages */
        		prime_dlmsotcp_pop_dlms_msg(&prime_dlmsotcp_pending_cfm_msg_list, px_msg);
        		PRIME_DLMSOTCP_LOG(LOG_ERR," ERROR, QUEUE CONFRIM TIMEOUT  Time= %d, Queued= %d!!!!!\r\n", ui_now - ui_timestamp, prime_dlmsotcp_pending_cfm_msg_list.count);
        		/* send next message */
        		_send_queued_432_data();
        	}
        }
    }
    /* more than one DLMS/432 message in this packet? */
    buflen = buflen - 8 - usDlmsLen;
    if (buflen <= 0) {
        return ERROR_DLMSoTCP_LAST_MESSAGE; /* Last message; */
    }

    /* more data, move forward the buffer pointer */
    buf = buf + 8 + usDlmsLen;
    goto __dlmsMsg;
}

/********************************************************
* \brief Set TCP Port for DLMSoTCP
* \param
* \return
********************************************************/
int32_t prime_dlmsotcp_set_port(int32_t tcp_port)
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
      tcp_port_dlmsotcp = tcp_port;
   return ret;
}

/********************************************************
* \brief Get TCP Port for DLMS over TCP
*
* \param
* \return
********************************************************/
int32_t prime_dlmsotcp_get_port()
{
/*********************************************************
*       Vars
*********************************************************/
/*********************************************************
*       Code
*********************************************************/
   return tcp_port_dlmsotcp;
}

/********************************************************
* \brief Get TCP Port for Sniffer
*
* \param
* \return
********************************************************/
int open_dlmsotcp_server(int _i_port)
{
	int i_listen_sd;
	int i_on =1;
	int i_rc;
	struct sockaddr_in   x_addr;

	i_listen_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (i_listen_sd < 0)
	{
		  PRIME_DLMSOTCP_LOG(LOG_ERR,"server socket() failed\r\n");
	    return ERROR_DLMSoTCP_SOCKET;
	}

	/* Allow socket descriptor to be reuseable                   */
	i_rc = setsockopt(i_listen_sd, SOL_SOCKET,  SO_REUSEADDR,
	                   &i_on, sizeof(i_on));
	if (i_rc < 0)
	{
		  PRIME_DLMSOTCP_LOG(LOG_ERR,"setsockopt() failed\r\n");
	    close(i_listen_sd);
	    return ERROR_DLMSoTCP_SOCKETOPT;
	}

	/* Set socket to be non-blocking.  All of the sockets for    */
	/* the incoming connections will also be non-blocking since  */
	/* they will inherit that state from the listening socket.   */
	/*i_rc = ioctl(i_listen_sd, FIONBIO, (char *)&i_on);
	if (i_rc < 0)
	{
		PRINTF(PRINT_ERROR,"ioctl() failed");
	    close(i_listen_sd);
	    exit(-1);
	}*/

	/* Bind the socket                                           */
	memset(&x_addr, 0, sizeof(struct sockaddr_in));
	x_addr.sin_family      = AF_INET;
	x_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	x_addr.sin_port        = htons(_i_port);
	i_rc = bind(i_listen_sd, (struct sockaddr *)&x_addr, sizeof(x_addr));
	if (i_rc < 0)
	{
		    PRIME_DLMSOTCP_LOG(LOG_ERR,"bind() failed\r\n");
	      close(i_listen_sd);
	      return ERROR_DLMSoTCP_BIND;
	}

	/* Set the listen back log - Only one connection */
	i_rc = listen(i_listen_sd, 1);
	if (i_rc < 0)
	{
		  PRIME_DLMSOTCP_LOG(LOG_ERR,"listen() failed\r\n");
	    close(i_listen_sd);
	    return ERROR_DLMSoTCP_LISTEN;
	}

	return i_listen_sd;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
