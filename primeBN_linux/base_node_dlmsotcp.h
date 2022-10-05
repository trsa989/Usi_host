/**
 * \file
 *
 * \brief Base Node DLMSoTCP Header.
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

 #ifndef _BASE_NODE_DLMSOTCP_H_
 #define _BASE_NODE_DLMSOTCP_H_

 /* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

#include "mchp_list.h"

#define PRIME_DLMSOTCP_LOG(lvl, msj...)		      if ((PRIME_##lvl <= prime_dlmsotcp_get_loglevel()) && prime_get_log()){ PRIME_PRINTF(msj); }
#define PRIME_DLMSOTCP_LOG_NOSTAMP(lvl, msj...) if ((PRIME_##lvl <= prime_dlmsotcp_get_loglevel()) && prime_get_log()){ PRIME_PRINTF_NOSTAMP(msj); }
#define PRIME_DLMSOTCP_PRINTF_PERROR(msj...)    ({ prime_log_mutex_lock(); perror(msj);fprintf(prime_logfile_fd,"errno = %d\n",errno); prime_log_mutex_unlock(); })

/*
 * \brief  Get PRIME DLMSoTCP Loglevel
 *
 * \return loglevel
 */
int prime_dlmsotcp_get_loglevel();

/*
 * \brief  	   Set PRIME DLMSoTCP Loglevel
 *
 * \param [in]  int loglevel
 * \return 0
 */
int prime_dlmsotcp_set_loglevel(int loglevel);

/* Define this depending on Base USI node buffers*/
#define MNGP_PRIME_ENHANCED_LIST_MAX_RECORDS  30
#define MAX_NUM_NODES_CONNECTED               2000
#define SERIAL_NUMBER_432_MAC                 16
#define MAX_BUFFER_SIZE                       1514

struct dlms_msg_t{
    mchp_list list;
    uint32_t ui_timestamp;
    uint16_t us_retries;
    uint16_t us_dst;
    uint16_t us_lsap;
    uint16_t us_length;
    uint8_t data[MAX_BUFFER_SIZE];
} ;
typedef struct dlms_msg_t dlms_msg;

struct dlms_msg_list_t{
    mchp_list list;
    uint8_t count;
} ;
typedef struct dlms_msg_list_t dlms_msg_list;

/* List of mesages */

void _dlmsotcp_cl_432_dl_data_cfm_cb(uint8_t uc_dst_lsap, uint8_t uc_src_lsap, uint16_t us_dst_address, uint8_t uc_tx_status);

/**
 * \brief Updates connection status and sends the New Device DLMSoTCP message
 *      to the client.
 */
void _dlmsotcp_cl_432_join_ind_cb(uint8_t *puc_device_id, uint8_t uc_device_id_len,
                                 uint16_t us_dst_address, uint8_t *puc_mac);

void _dlmsotcp_cl_432_leave_ind_cb(uint16_t us_dst_address);

/**
 * \brief New DLMS data arrived, forward it to the client
 */
void _dlmsotcp_cl_432_dl_data_ind_cb(uint8_t uc_dst_lsap, uint8_t uc_src_lsap, uint16_t us_dst_address,
		uint16_t src_address, uint8_t *puc_data, uint16_t uc_lsdu_len, uint8_t uc_link_class);

void base_node_dlmsotcp_auto_close_session();

/**
 * \brief Initialiazion and starting of the DLMSoTCP application.
 */
int base_node_dlmsotcp_init();

/**
 * \brief Initialiazion of the DLMSoTCP application. This application
 * uses 432 callback interface and management protocol.
 */
int base_node_dlmsotcp_start();

/**
 * \brief Stops DLMSoTCP application. This application
 * uses 432 callback interface and management protocol.
 */
int base_node_dlmsotcp_stop();

/**
 * \brief Process messages received from the concentrator.
 * Unpack dlmsotcp protocol headers and forward data to the Base Node.
 */
int32_t dlmsotcp_RxProcess(uint8_t* buf, uint16_t buflen);

/*
 * \brief Push a new PRIME DLMS Message on the list
 * \param prime_dlms_list   -> PRIME DLMS messages List
 * \param px_buf            -> DLMS message to add
 * \param
 *
 * \return -
 */
int prime_dlmsotcp_push_dlms_msg(dlms_msg_list *dlms_list, dlms_msg *msg);

/*
 * \brief Get the first PRIME DLMS message from the list
 * \param dlms_list   -> PRIME DLMS messages List
 * \param msg         -> Pointer where store the PRIME DLMS message
 *
 * \return -
 */
int prime_dlmsotcp_get_dlms_msg(dlms_msg_list *dlms_list, dlms_msg *msg);

/*
 * \brief POP a PRIME DLMS message from the list
 * \param dlms_list   -> PRIME DLMS messages List
 * \param msg         -> Pointer where store the PRIME DLMS message
 *
 * \return -
 */
int prime_dlmsotcp_pop_dlms_msg(dlms_msg_list *dlms_list, dlms_msg *msg);

/********************************************************
* \brief Set TCP Port for DLMSoTCP
* \param
* \return
********************************************************/
int32_t prime_dlmsotcp_set_port(int32_t tcp_port);

/********************************************************
* \brief Get TCP Port for DLMS over TCP
*
* \param
* \return
********************************************************/
int32_t prime_dlmsotcp_get_port();

/********************************************************
* \brief Get TCP Port for Sniffer
*
* \param
* \return
********************************************************/
int open_dlmsotcp_server(int _i_port);

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */

#endif
