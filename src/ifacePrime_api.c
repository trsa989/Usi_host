/**
 * \file
 *
 * \brief USI Host - Interface PRIME API
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

/* *************************************Includes********************************* */
#include <string.h>

#include "ifacePrime_api.h"

#include "../prime_api_host.h"

#include "../prime_api_defs_host.h"

#include "Usi.h"

#include "../cl_432_defs.h"

#include "UsiCfg.h"

/* *************************************Definitions****************************** */
/* ------------------------------------- */

/* Pointers to callback functions to be establish*/
static prime_cl_null_establish_ind_cb_t prime_cl_null_establish_ind_cb = 0;
static prime_cl_null_establish_cfm_cb_t prime_cl_null_establish_cfm_cb = 0;
static prime_cl_null_release_ind_cb_t prime_cl_null_release_ind_cb = 0;
static prime_cl_null_release_cfm_cb_t prime_cl_null_release_cfm_cb = 0;
static prime_cl_null_join_ind_cb_t prime_cl_null_join_ind_cb = 0;
static prime_cl_null_join_cfm_cb_t prime_cl_null_join_cfm_cb = 0;
static prime_cl_null_leave_cfm_cb_t prime_cl_null_leave_cfm_cb = 0;
static prime_cl_null_leave_ind_cb_t prime_cl_null_leave_ind_cb = 0;
static prime_cl_null_data_cfm_cb_t prime_cl_null_data_cfm_cb = 0;
static prime_cl_null_data_ind_cb_t prime_cl_null_data_ind_cb = 0;
static prime_cl_null_plme_reset_cfm_cb_t prime_cl_null_plme_reset_cfm_cb = 0;
static prime_cl_null_plme_sleep_cfm_cb_t prime_cl_null_plme_sleep_cfm_cb = 0;
static prime_cl_null_plme_resume_cfm_cb_t prime_cl_null_plme_resume_cfm_cb = 0;
static prime_cl_null_plme_testmode_cfm_cb_t prime_cl_null_plme_testmode_cfm_cb = 0;
static prime_cl_null_plme_get_cfm_cb_t prime_cl_null_plme_get_cfm_cb = 0;
static prime_cl_null_plme_set_cfm_cb_t prime_cl_null_plme_set_cfm_cb = 0;
static prime_cl_null_mlme_register_cfm_cb_t prime_cl_null_mlme_register_cfm_cb = 0;
static prime_cl_null_mlme_register_ind_cb_t prime_cl_null_mlme_register_ind_cb = 0;
static prime_cl_null_mlme_unregister_cfm_cb_t prime_cl_null_mlme_unregister_cfm_cb = 0;
static prime_cl_null_mlme_unregister_ind_cb_t prime_cl_null_mlme_unregister_ind_cb = 0;
static prime_cl_null_mlme_promote_cfm_cb_t prime_cl_null_mlme_promote_cfm_cb = 0;
static prime_cl_null_mlme_promote_ind_cb_t prime_cl_null_mlme_promote_ind_cb = 0;
static prime_cl_null_mlme_demote_cfm_cb_t prime_cl_null_mlme_demote_cfm_cb = 0;
static prime_cl_null_mlme_demote_ind_cb_t prime_cl_null_mlme_demote_ind_cb = 0;
static prime_cl_null_mlme_reset_cfm_cb_t prime_cl_null_mlme_reset_cfm_cb = 0;
static prime_cl_null_mlme_get_cfm_cb_t prime_cl_null_mlme_get_cfm_cb = 0;
static prime_cl_null_mlme_list_get_cfm_cb_t prime_cl_null_mlme_list_get_cfm_cb = 0;
static prime_cl_null_mlme_set_cfm_cb_t prime_cl_null_mlme_set_cfm_cb = 0;
static prime_cl_432_establish_cfm_cb_t prime_cl_432_establish_cfm_cb = 0;
static prime_cl_432_release_cfm_cb_t prime_cl_432_release_cfm_cb = 0;
static prime_cl_432_dl_data_cfm_cb_t prime_cl_432_dl_data_cfm_cb = 0;
static prime_cl_432_dl_data_ind_cb_t prime_cl_432_dl_data_ind_cb = 0;
static prime_cl_432_dl_leave_ind_cb_t prime_cl_432_dl_leave_ind_cb = 0;
static prime_cl_432_dl_join_ind_cb_t prime_cl_432_dl_join_ind_cb = 0;
static bmng_fup_ack_ind_cb_t fup_ack_ind_cb = 0;
static bmng_fup_status_ind_cb_t fup_status_ind_cb = 0;
static bmng_fup_error_ind_cb_t fup_error_ind_cb = 0;
static bmng_fup_version_ind_cb_t fup_version_ind_cb = 0;
static bmng_fup_kill_ind_cb_t fup_kill_ind_cb = 0;
static bmng_network_event_ind_cb_t network_event_ind_cb = 0;
static bmng_pprof_get_response_cb_t pprof_get_response_cb = 0;
static bmng_pprof_get_enhanced_response_cb_t pprof_get_enhanced_cb = 0;
static bmng_pprof_ack_ind_cb_t pprof_ack_ind_cb = 0;
static bmng_pprof_zerocross_response_cb_t pprof_zerocross_response_cb = 0;
static bmng_pprof_zc_diff_response_cb_t pprof_zc_diff_response_cb = 0;
static bmng_whitelist_ack_cb_t bmng_whitelist_ack_cb = 0;

static prime_debug_error_indication_cb_t prime_debug_ind_cd = 0;

/* buffer used to serialization */
static uint8_t uc_serial_buf[MAX_LENGTH_432_DATA];

/*USI CmdParams*/
CmdParams prime_api_msg;

cl_null_data_pointer _data_pointer_table[NUM_ELEMENTS_POINTER_TABLE];

/**
 * \brief CL NULL Set callback functions
 * \param px_prime_cbs  Pointer to mac callbacks struct
 */
void prime_cl_null_set_callbacks(prime_cl_null_callbacks_t *px_prime_cbs)
{
	prime_cl_null_establish_ind_cb = px_prime_cbs->prime_cl_null_establish_ind_cb;
	prime_cl_null_establish_cfm_cb = px_prime_cbs->prime_cl_null_establish_cfm_cb;
	prime_cl_null_release_ind_cb = px_prime_cbs->prime_cl_null_release_ind_cb;
	prime_cl_null_release_cfm_cb = px_prime_cbs->prime_cl_null_release_cfm_cb;
	prime_cl_null_join_ind_cb = px_prime_cbs->prime_cl_null_join_ind_cb;
	prime_cl_null_join_cfm_cb = px_prime_cbs->prime_cl_null_join_cfm_cb;
	prime_cl_null_leave_cfm_cb = px_prime_cbs->prime_cl_null_leave_cfm_cb;
	prime_cl_null_leave_ind_cb = px_prime_cbs->prime_cl_null_leave_ind_cb;
	prime_cl_null_data_cfm_cb = px_prime_cbs->prime_cl_null_data_cfm_cb;
	prime_cl_null_data_ind_cb = px_prime_cbs->prime_cl_null_data_ind_cb;
	prime_cl_null_plme_reset_cfm_cb = px_prime_cbs->prime_cl_null_plme_reset_cfm_cb;
	prime_cl_null_plme_sleep_cfm_cb = px_prime_cbs->prime_cl_null_plme_sleep_cfm_cb;
	prime_cl_null_plme_resume_cfm_cb = px_prime_cbs->prime_cl_null_plme_resume_cfm_cb;
	prime_cl_null_plme_testmode_cfm_cb = px_prime_cbs->prime_cl_null_plme_testmode_cfm_cb;
	prime_cl_null_plme_get_cfm_cb = px_prime_cbs->prime_cl_null_plme_get_cfm_cb;
	prime_cl_null_plme_set_cfm_cb = px_prime_cbs->prime_cl_null_plme_set_cfm_cb;
	prime_cl_null_mlme_register_cfm_cb = px_prime_cbs->prime_cl_null_mlme_register_cfm_cb;
	prime_cl_null_mlme_register_ind_cb = px_prime_cbs->prime_cl_null_mlme_register_ind_cb;
	prime_cl_null_mlme_unregister_cfm_cb = px_prime_cbs->prime_cl_null_mlme_unregister_cfm_cb;
	prime_cl_null_mlme_unregister_ind_cb = px_prime_cbs->prime_cl_null_mlme_unregister_ind_cb;
	prime_cl_null_mlme_promote_cfm_cb = px_prime_cbs->prime_cl_null_mlme_promote_cfm_cb;
	prime_cl_null_mlme_promote_ind_cb = px_prime_cbs->prime_cl_null_mlme_promote_ind_cb;
	prime_cl_null_mlme_demote_cfm_cb = px_prime_cbs->prime_cl_null_mlme_demote_cfm_cb;
	prime_cl_null_mlme_demote_ind_cb = px_prime_cbs->prime_cl_null_mlme_demote_ind_cb;
	prime_cl_null_mlme_reset_cfm_cb = px_prime_cbs->prime_cl_null_mlme_reset_cfm_cb;
	prime_cl_null_mlme_get_cfm_cb = px_prime_cbs->prime_cl_null_mlme_get_cfm_cb;
	prime_cl_null_mlme_list_get_cfm_cb = px_prime_cbs->prime_cl_null_mlme_list_get_cfm_cb;
	prime_cl_null_mlme_set_cfm_cb = px_prime_cbs->prime_cl_null_mlme_set_cfm_cb;
}

/*
 * \brief Request a connection establishment
 *
 * \param puc_eui48     Address of the node to which this connection will be addressed
 * \param uc_type       Convergence Layer type of the connection
 * \param puc_data      Data associated with the connection establishment procedure
 * \param us_data_len   Length of the data in bytes
 * \param uc_arq        Flag to indicate whether or not the ARQ mechanism should be used for this connection
 * \param uc_cfbytes    Flag to indicate whether or not the connection should use the contention or contention-free channel access scheme
 *
 */
void prime_cl_null_establish_request(uint8_t *puc_eui48, uint8_t uc_type, uint8_t *puc_data, uint16_t us_data_len, uint8_t uc_arq, uint8_t uc_cfbytes, uint8_t uc_ae)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_establish_request_cmd;
	memcpy(puc_msg, puc_eui48, LEN_ID_EUI48);
	puc_msg += LEN_ID_EUI48;
	*puc_msg++ = uc_type;
	*puc_msg++ = (uint8_t)(us_data_len >> 8);
	*puc_msg++ = (uint8_t)(us_data_len & 0xFF);
	memcpy(puc_msg, puc_data, us_data_len);
	puc_msg += us_data_len;
	*puc_msg++ = uc_arq;
	*puc_msg++ = uc_cfbytes;
	*puc_msg++ = uc_ae;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief Response to a connection establishment indication
 *
 * \param us_con_handle Uniquely identifier of the connection
 * \param uc_answer     Action to be taken for this connection establishment
 * \param puc_data      Data associated with the connection establishment procedure
 * \param us_data_len   Length of the data in bytes
 *
 */
void prime_cl_null_establish_response(uint16_t us_con_handle, mac_establish_response_answer_t uc_answer, uint8_t *puc_data, uint16_t us_data_len, uint8_t uc_ae)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_establish_response_cmd;
	*puc_msg++ = (uint8_t)(us_con_handle >> 8);
	*puc_msg++ = (uint8_t)(us_con_handle & 0xFF);
	*puc_msg++ = (uint8_t)uc_answer;
	*puc_msg++ = (uint8_t)(us_data_len >> 8);
	*puc_msg++ = (uint8_t)(us_data_len & 0xFF);
	memcpy(puc_msg, puc_data, us_data_len);
	puc_msg += us_data_len;
	*puc_msg++ = uc_ae;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief Response to a connection establishment indication
 *
 * \param us_con_handle Uniquely identifier of the connection
 *
 * \param puc_data      Data associated with the connection establishment procedure
 * \param us_data_len   Length of the data in bytes
 *
 */
void prime_cl_null_redirect_response(uint16_t us_con_handle, uint8_t *puc_eui48, uint8_t *puc_data, uint16_t us_data_len)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_redirect_response_cmd;
	*puc_msg++ = (uint8_t)(us_con_handle >> 8);
	*puc_msg++ = (uint8_t)(us_con_handle & 0xFF);
	memcpy(puc_msg, puc_eui48, 6);
	puc_msg += 6;
	*puc_msg++ = (uint8_t)(us_data_len >> 8);
	*puc_msg++ = (uint8_t)(us_data_len & 0xFF);
	memcpy(puc_msg, puc_data, us_data_len);
	puc_msg += us_data_len;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief Initiate the release process of a connection
 *
 * \param us_con_handle Uniquely identifier of the connection
 *
 */
void prime_cl_null_release_request(uint16_t us_con_handle)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_release_request_cmd;
	*puc_msg++ = (uint8_t)(us_con_handle >> 8);
	*puc_msg++ = (uint8_t)(us_con_handle & 0xFF);

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief Response to a connection release process
 *
 * \param us_con_handle Uniquely identifier of the connection
 * \param uc_answer     Action to be taken for this connection release procesudre
 *
 */
void prime_cl_null_release_response(uint16_t us_con_handle, mac_release_response_answer_t uc_answer)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_release_response_cmd;
	*puc_msg++ = (uint8_t)(us_con_handle >> 8);
	*puc_msg++ = (uint8_t)(us_con_handle & 0xFF);
	*puc_msg++ = (uint8_t)uc_answer;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * MAC Join Request
 * - us_broadcast:      Join type (broadcast or multicast connection)
 * - us_con_handle:     Unique identifier of the connection (only used for base node)
 * - puc_eui48:         Address of the node to which this join is being requested (only used for base node)
 * - uc_con_type:       Connection type
 * - puc_data:          Data associated with the join request procedure
 * - us_data_len:       Length of the data in bytesn
 */
void prime_cl_null_join_request(mac_join_mode_t uc_broadcast, uint16_t us_conn_handle, uint8_t *puc_eui48, connection_type_t uc_con_type, uint8_t *puc_data, uint16_t us_data_len, uint8_t uc_ae)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_join_request_cmd;
	*puc_msg++ = (uint8_t)uc_broadcast;
	*puc_msg++ = (uint8_t)(us_conn_handle >> 8);
	*puc_msg++ = (uint8_t)(us_conn_handle & 0xFF);
	memcpy(puc_msg, puc_eui48, 6);
	puc_msg += 6;
	*puc_msg++ = (uint8_t)uc_con_type;
	*puc_msg++ = (uint8_t)(us_data_len >> 8);
	*puc_msg++ = (uint8_t)(us_data_len & 0xFF);
	memcpy(puc_msg, puc_data, us_data_len);
	puc_msg += us_data_len;
	*puc_msg++ = uc_ae;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * MAC Join Response
 * - us_con_handle:        Unique identifier of the connection
 * - puc_eui48:            Address of the node which requested the multicast group join (only used for base node)
 * - uc_answer:            Action to be taken for this join request
 */
void prime_cl_null_join_response(uint16_t us_con_handle, uint8_t *puc_eui48, mac_join_response_answer_t uc_answer, uint8_t uc_ae)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_join_response_cmd;
	*puc_msg++ = (uint8_t)(us_con_handle >> 8);
	*puc_msg++ = (uint8_t)(us_con_handle & 0xFF);
	if (puc_eui48) {
		*puc_msg++ = puc_eui48[0];
		*puc_msg++ = puc_eui48[1];
		*puc_msg++ = puc_eui48[2];
		*puc_msg++ = puc_eui48[3];
		*puc_msg++ = puc_eui48[4];
		*puc_msg++ = puc_eui48[5];
	} else {
		*puc_msg++ = 0;
		*puc_msg++ = 0;
		*puc_msg++ = 0;
		*puc_msg++ = 0;
		*puc_msg++ = 0;
		*puc_msg++ = 0;
	}

	*puc_msg++ = (uint8_t)uc_answer;
	*puc_msg++ = uc_ae; *puc_msg++ = uc_ae;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * MAC Leave Request
 * - us_con_handle:        Unique identifier of the connection
 * - puc_eui48:            Address of the node to be removed from the multicast group (only used for base node)
 */
void prime_cl_null_leave_request(uint16_t us_con_handle, uint8_t *puc_eui48)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_leave_request_cmd;
	*puc_msg++ = (uint8_t)(us_con_handle >> 8);
	*puc_msg++ = (uint8_t)(us_con_handle & 0xFF);
	memcpy(puc_msg, puc_eui48, 6);
	puc_msg += 6;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief Initiate the transmission process of data over a connection
 *
 * \param us_con_handle    Specifies the connection to be used for the data transmission
 * \param puc_data         Pointer to data to be transmitted through this connection
 * \param us_data_len      Length of the data in bytes
 * \param uc_prio          Priority of the data to be sent when using the CSMA access scheme
 */
void prime_cl_null_data_request(uint16_t us_con_handle, uint8_t *puc_data, uint16_t us_data_len, uint8_t uc_prio, uint32_t ul_time_ref)
{
	uint8_t *puc_msg;
	int index;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_data_request_cmd;
	*puc_msg++ = (uint8_t)(us_con_handle >> 8);
	*puc_msg++ = (uint8_t)(us_con_handle & 0xFF);
	*puc_msg++ = (uint8_t)(us_data_len >> 8);
	*puc_msg++ = (uint8_t)(us_data_len & 0xFF);
	memcpy(puc_msg, puc_data, us_data_len);
	puc_msg += us_data_len;
	*puc_msg++ = uc_prio;
    *puc_msg++ = (uint8_t) (ul_time_ref & 0xFF);
    *puc_msg++ = (uint8_t) ((ul_time_ref >> 8)  & 0xFF);
    *puc_msg++ = (uint8_t) ((ul_time_ref >> 16) & 0xFF);
    *puc_msg++ = (uint8_t) ((ul_time_ref >> 24) & 0xFF);

	/* Find a free element of table */
	for (index = 0; index < NUM_ELEMENTS_POINTER_TABLE; index++) {
		if (!_data_pointer_table[index].valid) {
			/* Store connection hanlder and data pointer in table */
			_data_pointer_table[index].con_handle = us_con_handle;
			_data_pointer_table[index].puc_data = puc_data;
			_data_pointer_table[index].valid = true;
			break;
		}
	}

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief PLME reset request
 *
 */
void prime_cl_null_plme_reset_request(void)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_plme_reset_request_cmd;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief PLME sleep request
 *
 */
void prime_cl_null_plme_sleep_request(void)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_plme_sleep_request_cmd;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief PLME resumr request
 *
 */
void prime_cl_null_plme_resume_request(void)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_plme_resume_request_cmd;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief PLME testmode request
 *
 * \param uc_enable         Start/Stop test mode
 * \param uc_mode           Transmission mode
 * \param uc_modulation     Transmission modulation
 * \param uc_pwr_level      Transmission power level
 *
 */
void prime_cl_null_plme_testmode_request(uint8_t uc_enable, uint8_t uc_mode, uint8_t uc_modulation, uint8_t uc_pwr_level)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_plme_testmode_request_cmd;
	*puc_msg++ = uc_enable;
	*puc_msg++ = uc_mode;
	*puc_msg++ = uc_modulation;
	*puc_msg++ = uc_pwr_level;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief PLME get request
 *
 * \param us_pib_attrib      PIB attribute
 *
 */
void prime_cl_null_plme_get_request(uint16_t us_pib_attrib)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_plme_get_request_cmd;
	*puc_msg++ = (uint8_t)(us_pib_attrib >> 8);
	*puc_msg++ = (uint8_t)(us_pib_attrib & 0xFF);

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief PLME set request
 *
 * \param us_pib_attrib      PIB attribute
 * \param pv_pib_value       PIB attribute value
 * \param uc_pib_size        PIB attribute value size
 */
void prime_cl_null_plme_set_request(uint16_t us_pib_attrib, void *pv_pib_value, uint8_t uc_pib_size)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_plme_set_request_cmd;
	*puc_msg++ = (uint8_t)(us_pib_attrib >> 8);
	*puc_msg++ = (uint8_t)(us_pib_attrib & 0xFF);
	*puc_msg++ = uc_pib_size;
	memcpy(puc_msg, (uint8_t *)pv_pib_value, uc_pib_size);
	puc_msg += uc_pib_size;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief MLME register request
 *
 * \param puc_sna      Pointer to SNA
 * \param uc_sid       Switch Identifier
 *
 */
void prime_cl_null_mlme_register_request(uint8_t *puc_sna, uint8_t uc_sid)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_mlme_register_request_cmd;
	if (puc_sna == NULL) {
		memset(puc_msg, 0xFF, LEN_ID_EUI48);
	} else {
		memcpy(puc_msg, puc_sna, LEN_ID_EUI48);
	}

	puc_msg += LEN_ID_EUI48;
	*puc_msg++ = uc_sid;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief MLME unregister request
 *
 */
void prime_cl_null_mlme_unregister_request(void)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_mlme_unregister_request_cmd;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void prime_cl_null_mlme_unregister_request_base(unsigned char *_pucEui48)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_mlme_unregister_request_cmd;
	memcpy(puc_msg, (uint8_t *)_pucEui48, LEN_ID_EUI48);
	puc_msg += LEN_ID_EUI48;
	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief MLME promote request
 *
 */
void prime_cl_null_mlme_promote_request(uint8_t *puc_eui48, uint8_t uc_bcn_mode)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;
	*puc_msg++ = prime_cl_null_mlme_promote_request_cmd;
	memcpy(puc_msg, (uint8_t *)puc_eui48, LEN_ID_EUI48);
	puc_msg += LEN_ID_EUI48;
	*puc_msg++ = uc_bcn_mode;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief MLME demote request
 *
 */
void prime_cl_null_mlme_demote_request(void)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_mlme_demote_request_cmd;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief MLME reset request
 *
 */
void prime_cl_null_mlme_reset_request(void)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_mlme_reset_request_cmd;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief MLME get request
 *
 * \param us_pib_attrib      PIB attribute
 *
 */
void prime_cl_null_mlme_get_request(uint16_t us_pib_attrib)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_mlme_get_request_cmd;
	*puc_msg++ = (uint8_t)(us_pib_attrib >> 8);
	*puc_msg++ = (uint8_t)(us_pib_attrib & 0xFF);

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief MLME list get request
 *
 * \param us_pib_attrib      PIB attribute
 *
 */
void prime_cl_null_mlme_list_get_request(uint16_t us_pib_attrib)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_mlme_list_get_request_cmd;
	*puc_msg++ = (uint8_t)(us_pib_attrib >> 8);
	*puc_msg++ = (uint8_t)(us_pib_attrib & 0xFF);

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief MLME set request
 *
 * \param us_pib_attrib      PIB attribute
 * \param pv_pib_value       PIB attribute value
 * \param uc_pib_size        PIB attribute value size
 */
void prime_cl_null_mlme_set_request(uint16_t us_pib_attrib, void *pv_pib_value, uint8_t uc_pib_size)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_null_mlme_set_request_cmd;
	*puc_msg++ = (uint8_t)(us_pib_attrib >> 8);
	*puc_msg++ = (uint8_t)(us_pib_attrib & 0xFF);
	*puc_msg++ = uc_pib_size;
	memcpy(puc_msg, (uint8_t *)pv_pib_value, uc_pib_size);
	puc_msg += uc_pib_size;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief MLME ACTION request
 *
 * \param us_pib_attrib      ACTION PIB attribute
 * \param pv_pib_value       PIB attribute value
 * \param uc_pib_size        PIB attribute value size
 */

/*void prime_cl_null_mlme_action_request(uint16_t us_pib_attrib, void *pv_pib_value, uint8_t uc_pib_size)
 * {
 *  uint8_t *puc_msg;
 *
 *  //Insert parameters
 *  puc_msg = uc_serial_buf;
 *
 * puc_msg++ = prime_cl_null_mlme_set_request_cmd; //ACTION
 * puc_msg++ = (uint8_t)(us_pib_attrib >> 8);
 * puc_msg++ = (uint8_t)(us_pib_attrib & 0xFF);
 * puc_msg++ = uc_pib_size;
 *  memcpy(puc_msg,(uint8_t *)pv_pib_value,uc_pib_size);
 *  puc_msg += uc_pib_size;
 *
 *  //Send to USI
 *  prime_api_msg.pType = PROTOCOL_PRIME_API;
 *  prime_api_msg.buf = uc_serial_buf;
 *  prime_api_msg.len = puc_msg - uc_serial_buf;
 *
 *  usi_SendCmd(&prime_api_msg);
 * }*/

/**
 * \brief Initialize a cl_432 connection process
 *
 * \param puc_device_id          Pointer to the device identifier data
 * \param uc_device_id_len       Length of the device identfier
 */
void prime_cl_432_establish_request(uint8_t *puc_device_id, uint8_t uc_device_id_len, uint8_t uc_ae)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_432_establish_request_cmd;
	*puc_msg++ = uc_device_id_len;
	memcpy(puc_msg, puc_device_id, uc_device_id_len);
	puc_msg += uc_device_id_len;
	puc_msg += uc_ae;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief Initializes a cl_432 disconnection process
 *
 * \param us_dst_address   Address to disconnect
 */
void prime_cl_432_release_request(uint16_t us_dst_address)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_432_release_request_cmd;
	*puc_msg++ = (uint8_t)(us_dst_address >> 8);
	*puc_msg++ = (uint8_t)(us_dst_address & 0xFF);

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/**
 * \brief Start a sending data from a 432 connection
 *
 * \param uc_dst_lsap      Destination LSAP
 * \param uc_src_lsap      Source LSAP
 * \param us_dst_address   Destination 432 Address
 * \param px_buff          Pointer to the data buffer
 * \param uc_lsdu_len      Length of the data
 * \param uc_link_class    Link class (non used)
 */
void prime_cl_432_dl_data_request(uint8_t uc_dst_lsap, uint8_t uc_src_lsap, uint16_t us_dst_address, dl_432_buffer_t *px_buff, uint16_t us_lsdu_len,
		uint8_t uc_link_class)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_cl_432_dl_data_request_cmd;
	*puc_msg++ = uc_dst_lsap;
	*puc_msg++ = uc_src_lsap;
	*puc_msg++ = (uint8_t)(us_dst_address >> 8);
	*puc_msg++ = (uint8_t)(us_dst_address & 0xFF);
	*puc_msg++ = (uint8_t)(us_lsdu_len >> 8);
	*puc_msg++ = (uint8_t)(us_lsdu_len & 0xFF);
	memcpy(puc_msg, px_buff->dl.buff, us_lsdu_len);
	puc_msg += us_lsdu_len;
	*puc_msg++ = uc_link_class;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

uint8_t _cl_null_establish_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_establish_ind_cb) {
		uint16_t us_con_handle;
		uint8_t *puc_eui48;
		uint8_t uc_type;
		uint8_t *puc_data;
		uint16_t us_data_len;
		uint8_t uc_cfbytes;
		uint8_t uc_ae;

		us_con_handle = ((uint16_t)(*ptrMsg++)) << 8;
		us_con_handle += *ptrMsg++;
		puc_eui48 = ptrMsg;
		ptrMsg += LEN_ID_EUI48;
		uc_type = *ptrMsg++;
		us_data_len = ((uint16_t)(*ptrMsg++)) << 8;
		us_data_len += *ptrMsg++;

		/* This will break P13 api */

		/*if (len != 13 + us_data_len)
		 *  return(false);*/
		if (us_data_len > 256) {
			return(false);
		}

		puc_data = ptrMsg;
		ptrMsg += us_data_len;
		uc_cfbytes = *ptrMsg++;
		uc_ae = *ptrMsg++;

		prime_cl_null_establish_ind_cb(us_con_handle, puc_eui48, uc_type, puc_data, us_data_len, uc_cfbytes, uc_ae);
	}

	return(true);
}

uint8_t _cl_null_establish_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_establish_cfm_cb) {
		uint16_t us_con_handle;
		mac_establish_confirm_result_t uc_result;
		uint8_t *puc_eui48;
		uint8_t uc_type;
		uint8_t *puc_data;
		uint16_t us_data_len;
		uint8_t uc_ae;

		us_con_handle = ((uint16_t)(*ptrMsg++)) << 8;
		us_con_handle += *ptrMsg++;
		uc_result = (mac_establish_confirm_result_t)(*ptrMsg++);
		puc_eui48 = ptrMsg;
		ptrMsg += LEN_ID_EUI48;
		uc_type = *ptrMsg++;
		us_data_len = ((uint16_t)(*ptrMsg++)) << 8;
		us_data_len += *ptrMsg++;

		/* This will break P13 api */

		/*if (len != 13 + us_data_len)
		 *  return(false);*/
		if (us_data_len > 256) {
			return(false);
		}

		puc_data = ptrMsg;

		ptrMsg += us_data_len;
		uc_ae = *ptrMsg++;

		prime_cl_null_establish_cfm_cb(us_con_handle, uc_result, puc_eui48, uc_type, puc_data, us_data_len, uc_ae);
	}

	return(true);
}

uint8_t _cl_null_release_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_release_ind_cb) {
		if (len != 3) {
			return(false);
		}

		uint16_t us_con_handle;
		mac_release_indication_reason_t uc_reason;

		us_con_handle = ((uint16_t)(*ptrMsg++)) << 8;
		us_con_handle += *ptrMsg++;
		uc_reason = (mac_release_indication_reason_t)(*ptrMsg++);

		prime_cl_null_release_ind_cb(us_con_handle, uc_reason);
	}

	return(true);
}

uint8_t _cl_null_release_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_release_cfm_cb) {
		if (len != 3) {
			return(false);
		}

		uint16_t us_con_handle;
		mac_release_confirm_result_t uc_result;

		us_con_handle = ((uint16_t)(*ptrMsg++)) << 8;
		us_con_handle += *ptrMsg++;
		uc_result = (mac_release_confirm_result_t)(*ptrMsg++);

		prime_cl_null_release_cfm_cb(us_con_handle, uc_result);
	}

	return(true);
}

uint8_t _cl_null_join_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_join_ind_cb) {
		uint16_t us_con_handle;
		uint8_t *puc_eui48;
		uint8_t uc_con_type;
		uint8_t *puc_data;
		uint16_t us_data_len;
		uint8_t uc_ae;

		us_con_handle = ((uint16_t)(*ptrMsg++)) << 8;
		us_con_handle += *ptrMsg++;
		puc_eui48 = ptrMsg;
		ptrMsg += LEN_ID_EUI48;
		uc_con_type = *ptrMsg++;
		us_data_len = ((uint16_t)(*ptrMsg++)) << 8;
		us_data_len += *ptrMsg++;
		/* This will break P13 api */

		/*if (len != 13 + us_data_len)
		 *  return(false);*/
		if (us_data_len > 256) {
			return(false);
		}

		puc_data = ptrMsg;
		ptrMsg += us_data_len;
		uc_ae = *ptrMsg++;

		prime_cl_null_join_ind_cb(us_con_handle, puc_eui48, uc_con_type, puc_data, us_data_len, uc_ae);
	}

	return(true);
}

uint8_t _cl_null_join_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_join_cfm_cb) {
		uint16_t us_con_handle;
		mac_join_confirm_result_t uc_result;
		uint8_t uc_ae;

		if (len != 3) {
			return(false);
		}

		us_con_handle = ((uint16_t)(*ptrMsg++)) << 8;
		us_con_handle += *ptrMsg++;
		uc_result = (mac_join_confirm_result_t)(*ptrMsg++);
		uc_ae = *ptrMsg++;

		prime_cl_null_join_cfm_cb(us_con_handle, uc_result, uc_ae);
	}

	return(true);
}

uint8_t _cl_null_leave_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_leave_ind_cb) {
		uint16_t us_con_handle;
		uint8_t *puc_eui48;
		uint8_t uc_index;

		if (len != 8) {
			return(false);
		}

		us_con_handle = ((uint16_t)(*ptrMsg++)) << 8;
		us_con_handle += *ptrMsg++;
		puc_eui48 = ptrMsg;
		for (uc_index = 0; uc_index < LEN_ID_EUI48; uc_index++) {
			if (puc_eui48[uc_index] != 0xFF) {
				break;
			} else {
				if (uc_index == LEN_ID_EUI48 - 1) {
					puc_eui48 = NULL;
				}
			}
		}

		prime_cl_null_leave_ind_cb(us_con_handle, puc_eui48);
	}

	return(true);
}

uint8_t _cl_null_leave_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_leave_cfm_cb) {
		uint16_t us_con_handle;
		mac_leave_confirm_result_t uc_result;

		if (len != 3) {
			return(false);
		}

		us_con_handle = ((uint16_t)(*ptrMsg++)) << 8;
		us_con_handle += *ptrMsg++;
		uc_result = (mac_leave_confirm_result_t)*ptrMsg;

		prime_cl_null_leave_cfm_cb(us_con_handle, uc_result);
	}

	return(true);
}

uint8_t _cl_null_data_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_data_cfm_cb) {
		uint16_t us_con_handle;
		uint32_t ul_temp;
		uint8_t *puc_data;
		uint8_t index;
		uint8_t found = false;
		mac_data_result_t drt_result;

		if (len != 7) {
			return(false);
		}

		us_con_handle = ((uint16_t)(*ptrMsg++)) << 8;
		us_con_handle += *ptrMsg++;
		ul_temp = ((uint32_t)(*ptrMsg++)) << 24;
		ul_temp += ((uint32_t)(*ptrMsg++)) << 16;
		ul_temp += ((uint32_t)(*ptrMsg++)) << 8;
		ul_temp += (uint32_t)(*ptrMsg++);
		drt_result = (mac_data_result_t)*ptrMsg;

		/* Search pointer in table using connection handler */
		for (index = 0; index < NUM_ELEMENTS_POINTER_TABLE; index++) {
			if ((_data_pointer_table[index].valid) && (_data_pointer_table[index].con_handle) == us_con_handle) {
				puc_data = _data_pointer_table[index].puc_data;
				_data_pointer_table[index].valid = false;
				found = true;
				break;
			}
		}
		if (found) {
			prime_cl_null_data_cfm_cb(us_con_handle, puc_data, drt_result);
		} else {
			prime_cl_null_data_cfm_cb(us_con_handle, NULL, drt_result);
		}
	}

	return(true);
}

uint8_t _cl_null_data_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_data_ind_cb) {
		uint16_t us_con_handle;
		uint8_t *puc_data;
		uint16_t us_data_len;
        uint32_t ul_time_ref;

		us_con_handle = ((uint16_t)(*ptrMsg++)) << 8;
		us_con_handle += *ptrMsg++;
		us_data_len = ((uint16_t)(*ptrMsg++)) << 8;
		us_data_len += *ptrMsg++;
        ul_time_ref  = ((uint8_t)(*ptrMsg++)) << 24;
	    ul_time_ref += ((uint8_t)(*ptrMsg++)) << 16;
    	ul_time_ref += ((uint8_t)(*ptrMsg++)) << 8;
        ul_time_ref += ((uint8_t)(*ptrMsg++));
		/* if(len != 4 + us_data_len) */
		/*    return(false); */
		puc_data = ptrMsg;

		prime_cl_null_data_ind_cb(us_con_handle, puc_data, us_data_len,ul_time_ref);
	}

	return(true);
}

uint8_t _cl_null_plme_reset_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_plme_reset_cfm_cb) {
		plme_result_t x_result;

		if (len != 1) {
			return(false);
		}

		x_result = (plme_result_t)*ptrMsg;

		prime_cl_null_plme_reset_cfm_cb(x_result);
	}

	return(true);
}

uint8_t _cl_null_plme_sleep_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_plme_sleep_cfm_cb) {
		plme_result_t x_result;

		if (len != 1) {
			return(false);
		}

		x_result = (plme_result_t)*ptrMsg;

		prime_cl_null_plme_sleep_cfm_cb(x_result);
	}

	return(true);
}

uint8_t _cl_null_plme_resume_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_plme_resume_cfm_cb) {
		plme_result_t x_result;

		if (len != 1) {
			return(false);
		}

		x_result = (plme_result_t)*ptrMsg;

		prime_cl_null_plme_resume_cfm_cb(x_result);
	}

	return(true);
}

uint8_t _cl_null_plme_testmode_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_plme_testmode_cfm_cb) {
		plme_result_t x_result;

		if (len != 1) {
			return(false);
		}

		x_result = (plme_result_t)*ptrMsg;

		prime_cl_null_plme_testmode_cfm_cb(x_result);
	}

	return(true);
}

uint8_t _cl_null_plme_get_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_plme_get_cfm_cb) {
		plme_result_t x_status;
		uint16_t us_pib_attrib;
		void *pv_pib_value;
		uint8_t uc_pib_size;

		x_status = (plme_result_t)*ptrMsg++;
		us_pib_attrib = ((uint16_t)(*ptrMsg++)) << 8;
		us_pib_attrib += *ptrMsg++;
		uc_pib_size = *ptrMsg++;
		if (len != 4 + uc_pib_size) {
			return(false);
		}

		pv_pib_value = (void *)ptrMsg;

		prime_cl_null_plme_get_cfm_cb(x_status, us_pib_attrib, pv_pib_value, uc_pib_size);
	}

	return(true);
}

uint8_t _cl_null_plme_set_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_plme_set_cfm_cb) {
		plme_result_t x_result;

		if (len != 1) {
			return(false);
		}

		x_result = (plme_result_t)*ptrMsg++;

		prime_cl_null_plme_set_cfm_cb(x_result);
	}

	return(true);
}

uint8_t _cl_null_mlme_register_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_mlme_register_cfm_cb) {
		mlme_result_t x_result;
		uint8_t *puc_sna;
		uint8_t uc_sid;

		if (len != 8) {
			return(false);
		}

		x_result = (mlme_result_t)*ptrMsg++;
		puc_sna = ptrMsg;
		ptrMsg += LEN_ID_EUI48;
		uc_sid = *ptrMsg++;

		prime_cl_null_mlme_register_cfm_cb(x_result, puc_sna, uc_sid);
	}

	return(true);
}

uint8_t _cl_null_mlme_register_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_mlme_register_ind_cb) {
		uint8_t *puc_sna;
		uint8_t uc_sid;

		if (len != 7) {
			return(false);
		}

		puc_sna = ptrMsg;
		ptrMsg += LEN_ID_EUI48;
		uc_sid = *ptrMsg++;

		prime_cl_null_mlme_register_ind_cb(puc_sna, uc_sid);
	}

	return(true);
}

uint8_t _cl_null_mlme_unregister_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_mlme_unregister_cfm_cb) {
		mlme_result_t x_result;

		if (len != 1) {
			return(false);
		}

		x_result = (mlme_result_t)*ptrMsg++;

		prime_cl_null_mlme_unregister_cfm_cb(x_result);
	}

	return(true);
}

uint8_t _cl_null_mlme_unregister_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_mlme_unregister_ind_cb) {
		if (len != 0) {
			return(false);
		}

		prime_cl_null_mlme_unregister_ind_cb();
	}

	return(true);
}

uint8_t _cl_null_mlme_promote_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_mlme_promote_cfm_cb) {
		mlme_result_t x_result;

		if (len != 1) {
			return(false);
		}

		x_result = (mlme_result_t)*ptrMsg++;

		prime_cl_null_mlme_promote_cfm_cb(x_result);
	}

	return(true);
}

uint8_t _cl_null_mlme_promote_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_mlme_promote_ind_cb) {
		if (len != 0) {
			return(false);
		}

		prime_cl_null_mlme_promote_ind_cb();
	}

	return(true);
}

uint8_t _cl_null_mlme_demote_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_mlme_demote_cfm_cb) {
		mlme_result_t x_result;

		if (len != 1) {
			return(false);
		}

		x_result = (mlme_result_t)*ptrMsg++;

		prime_cl_null_mlme_demote_cfm_cb(x_result);
	}

	return(true);
}

uint8_t _cl_null_mlme_demote_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_mlme_demote_ind_cb) {
		if (len != 0) {
			return(false);
		}

		prime_cl_null_mlme_demote_ind_cb();
	}

	return(true);
}

uint8_t _cl_null_mlme_reset_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_mlme_reset_cfm_cb) {
		mlme_result_t x_result;

		if (len != 1) {
			return(false);
		}

		x_result = (mlme_result_t)*ptrMsg++;

		prime_cl_null_mlme_reset_cfm_cb(x_result);
	}

	return(true);
}

uint8_t _cl_null_mlme_get_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_mlme_get_cfm_cb) {
		mlme_result_t x_status;
		uint16_t us_pib_attrib;
		void *pv_pib_value;
		uint8_t uc_pib_size;

		x_status = (mlme_result_t)*ptrMsg++;
		us_pib_attrib = ((uint16_t)(*ptrMsg++)) << 8;
		us_pib_attrib += *ptrMsg++;
		uc_pib_size = *ptrMsg++;
		if (len != 4 + uc_pib_size) {
			return(false);
		}

		pv_pib_value = (void *)ptrMsg;

		prime_cl_null_mlme_get_cfm_cb(x_status, us_pib_attrib, pv_pib_value, uc_pib_size);
	}

	return(true);
}

uint8_t _cl_null_mlme_list_get_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_mlme_list_get_cfm_cb) {
		mlme_result_t x_status;
		uint16_t us_pib_attrib;
		uint8_t *puc_pib_buff;
		uint16_t us_pib_len;

		x_status = (mlme_result_t)*ptrMsg++;
		us_pib_attrib = ((uint16_t)(*ptrMsg++)) << 8;
		us_pib_attrib += *ptrMsg++;
		us_pib_len = ((uint16_t)(*ptrMsg++)) << 8;
		us_pib_len += *ptrMsg++;
		if (len != 5 + us_pib_len) {
			return(false);
		}

		puc_pib_buff = ptrMsg;

		prime_cl_null_mlme_list_get_cfm_cb(x_status, us_pib_attrib, puc_pib_buff, us_pib_len);
	}

	return(true);
}

uint8_t _cl_null_mlme_set_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_null_mlme_set_cfm_cb) {
		mlme_result_t x_result;

		if (len != 1) {
			return(false);
		}

		x_result = (mlme_result_t)*ptrMsg++;

		prime_cl_null_mlme_set_cfm_cb(x_result);
	}

	return(true);
}

void prime_cl_432_set_callbacks(prime_cl_432_callbacks_t *px_prime_cbs)
{
	prime_cl_432_establish_cfm_cb = px_prime_cbs->prime_cl_432_establish_cfm_cb;
	prime_cl_432_release_cfm_cb = px_prime_cbs->prime_cl_432_release_cfm_cb;
	prime_cl_432_dl_data_ind_cb = px_prime_cbs->prime_cl_432_dl_data_ind_cb;
	prime_cl_432_dl_data_cfm_cb = px_prime_cbs->prime_cl_432_dl_data_cfm_cb;
	prime_cl_432_dl_join_ind_cb = px_prime_cbs->prime_cl_432_dl_join_ind_cb;
	prime_cl_432_dl_leave_ind_cb = px_prime_cbs->prime_cl_432_dl_leave_ind_cb;
}

uint8_t _cl_432_establish_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_432_establish_cfm_cb) {
		uint8_t *puc_device_id;
		uint8_t uc_device_id_len;
		uint16_t us_dst_address;
		uint16_t us_base_address;
		uint8_t uc_ae = 0;

		uc_device_id_len = *ptrMsg++;
		if (uc_device_id_len > 32) {
			return(false);
		}

		puc_device_id = ptrMsg;
		ptrMsg += uc_device_id_len;
		us_dst_address = ((uint16_t)(*ptrMsg++)) << 8;
		us_dst_address += *ptrMsg++;
		us_base_address = ((uint16_t)(*ptrMsg++)) << 8;
		us_base_address += *ptrMsg++;
		uc_ae += *ptrMsg++;

		prime_cl_432_establish_cfm_cb(puc_device_id, uc_device_id_len, us_dst_address, us_base_address, uc_ae);
	}

	return(true);
}

uint8_t _cl_432_release_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_432_release_cfm_cb) {
		uint16_t us_dst_address;
		dl_432_result_t uc_result;

		if (len != 3) {
			return(false);
		}

		us_dst_address = ((uint16_t)(*ptrMsg++)) << 8;
		us_dst_address += *ptrMsg++;
		uc_result = (dl_432_result_t)*ptrMsg++;

		prime_cl_432_release_cfm_cb(us_dst_address, uc_result);
	}

	return(true);
}

uint8_t _cl_432_dl_data_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_432_dl_data_ind_cb) {
		uint8_t uc_dst_lsap;
		uint8_t uc_src_lsap;
		uint16_t us_dst_address;
		uint16_t src_address;
		uint8_t *puc_data;
		uint16_t us_lsdu_len;
		uint8_t uc_link_class;

		uc_dst_lsap = *ptrMsg++;
		uc_src_lsap = *ptrMsg++;
		us_dst_address = ((uint16_t)(*ptrMsg++)) << 8;
		us_dst_address += *ptrMsg++;
		src_address = ((uint16_t)(*ptrMsg++)) << 8;
		src_address += *ptrMsg++;
		us_lsdu_len = ((uint16_t)(*ptrMsg++)) << 8;
		us_lsdu_len += *ptrMsg++;
		if (len != 9 + us_lsdu_len) {
			return(false);
		}

		puc_data = ptrMsg;
		ptrMsg += us_lsdu_len;
		uc_link_class = *ptrMsg++;

		prime_cl_432_dl_data_ind_cb(uc_dst_lsap, uc_src_lsap, us_dst_address, src_address, puc_data, us_lsdu_len, uc_link_class);
	}

	return(true);
}

uint8_t _cl_432_dl_data_cfm(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_432_dl_data_cfm_cb) {
		uint8_t uc_dst_lsap;
		uint8_t uc_src_lsap;
		uint16_t us_dst_address;
		uint8_t uc_tx_status;

		if (len != 5) {
			return(false);
		}

		uc_dst_lsap = *ptrMsg++;
		uc_src_lsap = *ptrMsg++;
		us_dst_address = ((uint16_t)(*ptrMsg++)) << 8;
		us_dst_address += *ptrMsg++;
		uc_tx_status = *ptrMsg++;

		prime_cl_432_dl_data_cfm_cb(uc_dst_lsap, uc_src_lsap, us_dst_address, uc_tx_status);
	}

	return(true);
}

void set_cl_432_dl_leave_ind_cb(prime_cl_432_dl_leave_ind_cb_t _fn_cb)
{
	prime_cl_432_dl_leave_ind_cb = _fn_cb;
}

uint8_t _cl_432_dl_leave_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_432_dl_leave_ind_cb) {
		uint16_t us_lnid;
		if (len != 2) {
			return(false);
		}

		us_lnid = ((uint16_t)(*ptrMsg++)) << 8;
		us_lnid += *ptrMsg++;

		prime_cl_432_dl_leave_ind_cb(us_lnid);
		return(true);
	}

	return(false);
}

void set_cl_432_dl_join_ind_cb(prime_cl_432_dl_join_ind_cb_t _fn_cb)
{
	prime_cl_432_dl_join_ind_cb = _fn_cb;
}

uint8_t _cl_432_dl_join_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_cl_432_dl_join_ind_cb) {
		uint8_t puc_device_id[64]; /* DLMS Device name. */
		uint8_t uc_device_id_len;
		uint16_t us_dst_address;
		uint8_t puc_mac[6];
		uint8_t uc_ae;

		uc_device_id_len = *ptrMsg++;
		memcpy(puc_device_id, ptrMsg, uc_device_id_len);
		ptrMsg        += uc_device_id_len;
		us_dst_address  = ((uint16_t)(*ptrMsg++)) << 8;
		us_dst_address += *ptrMsg++;
		memcpy(puc_mac, ptrMsg, 6);
		ptrMsg += 6;
		uc_ae = *ptrMsg++;

		prime_cl_432_dl_join_ind_cb(puc_device_id, uc_device_id_len, us_dst_address, puc_mac, uc_ae);
		return(true);
	}

	return(false);
}

void bmng_set_callbacks(prime_bmng_callbacks_t *px_bmng_cbs)
{
	fup_ack_ind_cb              = px_bmng_cbs->fup_ack_ind_cb;
	fup_status_ind_cb           = px_bmng_cbs->fup_status_ind_cb;
	fup_error_ind_cb            = px_bmng_cbs->fup_error_ind_cb;
	fup_version_ind_cb          = px_bmng_cbs->fup_version_ind_cb;
	fup_kill_ind_cb             = px_bmng_cbs->fup_kill_ind_cb;
	network_event_ind_cb        = px_bmng_cbs->network_event_ind_cb;
	pprof_ack_ind_cb            = px_bmng_cbs->pprof_ack_ind_cb;
	pprof_get_response_cb       = px_bmng_cbs->pprof_get_response_cb;
	pprof_get_enhanced_cb       = px_bmng_cbs->pprof_get_enhanced_response_cb;
	pprof_zerocross_response_cb = px_bmng_cbs->pprof_zerocross_response_cb;
	pprof_zc_diff_response_cb   = px_bmng_cbs->pprof_zc_diff_response_cb;
	bmng_whitelist_ack_cb       = px_bmng_cbs->whitelist_ack_cb;
}

void bmng_fup_clear_target_list_request()
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_fup_clear_target_list_request_cmd;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void bmng_fup_add_target_request(uint8_t *puc_eui48)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_fup_add_target_request_cmd;
	memcpy(puc_msg, puc_eui48, 6);
	puc_msg += 6;
	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void bmng_fup_set_fw_data_request(uint8_t uc_vendor_len, uint8_t *puc_vendor, uint8_t uc_model_len, uint8_t *puc_model, uint8_t uc_version_len, uint8_t *puc_version  )
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_fup_set_fw_data_request_cmd;
	*puc_msg++ = uc_vendor_len;
	memcpy(puc_msg, puc_vendor, uc_vendor_len);
	puc_msg += uc_vendor_len;
	*puc_msg++ = uc_model_len;
	memcpy(puc_msg, puc_model, uc_model_len);
	puc_msg += uc_model_len;
	*puc_msg++ = uc_version_len;
	memcpy(puc_msg, puc_version, uc_version_len);
	puc_msg += uc_version_len;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);

	return;
}

void bmng_fup_set_upg_options_request(uint8_t uc_arq_en, uint8_t uc_page_size, uint8_t uc_mult_en, uint32_t ui_delay, uint32_t ui_timer)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_fup_set_upg_options_request_cmd;
	*puc_msg++ = uc_arq_en;
	*puc_msg++ = uc_page_size;
	*puc_msg++ = uc_mult_en;
	*puc_msg++ = (uint8_t)(ui_delay >>  24);
	*puc_msg++ = (uint8_t)((ui_delay >> 16) & 0xFF);
	*puc_msg++ = (uint8_t)((ui_delay >>  8) & 0xFF);
	*puc_msg++ = (uint8_t)(ui_delay & 0xFF);
	*puc_msg++ = (uint8_t)(ui_timer >>  24);
	*puc_msg++ = (uint8_t)((ui_timer >> 16) & 0xFF);
	*puc_msg++ = (uint8_t)((ui_timer >>  8) & 0xFF);
	*puc_msg++ = (uint8_t)(ui_timer & 0xFF);

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void bmng_fup_init_file_tx_request(uint16_t us_num_frames, uint32_t ui_file_size, uint16_t us_frame_size, uint32_t ui_crc)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_fup_init_file_tx_request_cmd;
	*puc_msg++ = (uint8_t)((us_num_frames >>  8) & 0xFF);
	*puc_msg++ = (uint8_t)(us_num_frames & 0xFF);
	*puc_msg++ = (uint8_t)(ui_file_size >>  24);
	*puc_msg++ = (uint8_t)((ui_file_size >> 16) & 0xFF);
	*puc_msg++ = (uint8_t)((ui_file_size >>  8) & 0xFF);
	*puc_msg++ = (uint8_t)(ui_file_size & 0xFF);
	*puc_msg++ = (uint8_t)((us_frame_size >>  8) & 0xFF);
	*puc_msg++ = (uint8_t)(us_frame_size & 0xFF);
	*puc_msg++ = (uint8_t)(ui_crc >>  24);
	*puc_msg++ = (uint8_t)((ui_crc >> 16) & 0xFF);
	*puc_msg++ = (uint8_t)((ui_crc >>  8) & 0xFF);
	*puc_msg++ = (uint8_t)(ui_crc & 0xFF);

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void bmng_fup_data_frame_request(uint16_t us_frame_num, uint16_t us_len, uint8_t *puc_data)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_fup_data_frame_request_cmd;
	*puc_msg++ = (uint8_t)((us_frame_num >>  8) & 0xFF);
	*puc_msg++ = (uint8_t)(us_frame_num & 0xFF);
	*puc_msg++ = (uint8_t)((us_len >>  8) & 0xFF);
	*puc_msg++ = (uint8_t)(us_len & 0xFF);
	memcpy(puc_msg, puc_data, us_len);
	puc_msg += us_len;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void bmng_fup_check_crc_request()
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_fup_check_crc_request_cmd;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void bmng_fup_abort_fu_request(uint8_t *puc_eui48)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_fup_abort_fu_request_cmd;
	memcpy(puc_msg, puc_eui48, 6);
	puc_msg += 6;
	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void bmng_fup_start_fu_request(uint8_t uc_enable)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_fup_start_fu_request_cmd;
	*puc_msg++ = uc_enable;
	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void bmng_fup_set_match_rule_request(uint8_t uc_rule)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_fup_set_match_rule_request_cmd;
	*puc_msg++ = uc_rule;
	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void bmng_fup_get_version_request(uint8_t *puc_eui48)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_fup_get_version_request_cmd;
	memcpy(puc_msg, puc_eui48, 6);
	puc_msg += 6;
	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void bmng_fup_get_state_request(uint8_t *puc_eui48)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_fup_get_state_request_cmd;
	memcpy(puc_msg, puc_eui48, 6);
	puc_msg += 6;
	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

uint8_t _prime_fup_ack_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (fup_ack_ind_cb) {
		uint8_t uc_cmd;
		uint8_t uc_ack;
		uint16_t us_data = 0xFFFF;

		if (len < 2) {
			return(false);
		}

		if (len > 4) {
			return(false);
		}

		uc_cmd = *ptrMsg++;
		uc_ack = *ptrMsg++;
		if (len > 2) {
			us_data = ((uint16_t)(*ptrMsg++)) << 8;
			us_data += *ptrMsg++;
		}

		fup_ack_ind_cb(uc_cmd, uc_ack, us_data);
	}

	return(true);
}

uint8_t _prime_fup_status_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (fup_status_ind_cb) {
		uint8_t uc_status;
		uint32_t ui_pages;
		uint8_t puc_eui48[6];

		if (len < 9) {
			return(false);
		}

		uc_status = *ptrMsg++;
		ui_pages  = *ptrMsg++; ui_pages <<= 8;
		ui_pages += *ptrMsg++;
		memcpy(puc_eui48, ptrMsg, 6);

		fup_status_ind_cb(uc_status, ui_pages, puc_eui48);
	}

	return(true);
}

uint8_t _prime_fup_error_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (fup_error_ind_cb) {
		uint8_t uc_err_code;
		uint8_t puc_eui48[6];

		if (len < 7) {
			return(false);
		}

		uc_err_code = *ptrMsg++;
		memcpy(puc_eui48, ptrMsg, 6);

		fup_error_ind_cb(uc_err_code, puc_eui48);
	}

	return(true);
}

uint8_t _prime_fup_version_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (fup_version_ind_cb) {
		uint8_t puc_eui48[LEN_ID_EUI48];
		uint8_t uc_vendor_len;
		uint8_t puc_vendor[32];
		uint8_t uc_model_len;
		uint8_t puc_model[32];
		uint8_t uc_version_len;
		uint8_t puc_version[32];

		/* if (len < 6 + uc_vendor_len + uc_model_len + uc_version_len) */
		/*    return(false); */

		memcpy(puc_eui48, ptrMsg, 6);
		ptrMsg += 6;
		uc_vendor_len =  *ptrMsg++;
		if (uc_vendor_len > 32) {
			uc_vendor_len = 32;
		}

		memcpy(puc_vendor, ptrMsg, uc_vendor_len);
		ptrMsg += uc_vendor_len;

		uc_model_len =  *ptrMsg++;
		if (uc_model_len > 32) {
			uc_model_len = 32;
		}

		memcpy(puc_model, ptrMsg, uc_model_len);
		ptrMsg += uc_model_len;

		uc_version_len =  *ptrMsg++;
		if (uc_version_len > 32) {
			uc_version_len = 32;
		}

		memcpy(puc_version, ptrMsg, uc_version_len);
		/* ptrMsg+=uc_version_len; */

		if (len < 6 + uc_vendor_len + uc_model_len + uc_version_len) {
			return(false);
		}

		fup_version_ind_cb(puc_eui48, uc_vendor_len, puc_vendor, uc_model_len, puc_model, uc_version_len, puc_version);
	}

	return(true);
}

uint8_t _prime_fup_kill_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (fup_kill_ind_cb) {
		uint8_t puc_eui48[6];

		if (len < 6) {
			return(false);
		}

		memcpy(puc_eui48, ptrMsg, 6);
		fup_kill_ind_cb(puc_eui48);
	}

	return(true);
}

void bmng_fup_signature_cfg_cmd(uint8_t uc_sig_algo, uint16_t us_sig_len )
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_fup_set_signature_data_request_cmd;
	*puc_msg++ = uc_sig_algo;
	*puc_msg++ = (uint8_t)(us_sig_len >> 8);
	*puc_msg++ = (uint8_t)(us_sig_len & 0xFF);

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

uint8_t _bmng_network_ind(uint8_t *ptrMsg, uint16_t len)
{
	bmng_net_event_t px_net_event;

	if (network_event_ind_cb) {
		px_net_event.net_event = *ptrMsg++;
		memcpy(px_net_event.mac, ptrMsg, 6);
		ptrMsg += 6;
		px_net_event.sid      = *ptrMsg++;
		px_net_event.lnid     = ((uint16_t)(*ptrMsg++)) << 8;
		px_net_event.lnid    += *ptrMsg++;
		px_net_event.lsid     = *ptrMsg++;
		px_net_event.alvRxcnt = *ptrMsg++;
		px_net_event.alvTxcnt = *ptrMsg++;
		px_net_event.alvTime  = *ptrMsg++;

		network_event_ind_cb(&px_net_event);
		return(true);
	}

	return(false);
}

void bmng_pprof_get_request(uint8_t *puc_eui48, uint16_t us_pib, uint8_t uc_index)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_pprof_get_request_cmd;
	memcpy(puc_msg, puc_eui48, 6);
	puc_msg += 6;
	*puc_msg++ = 0; /* length */
	*puc_msg++ = 3; /* length */
	*puc_msg++ = (uint8_t)(us_pib >> 8);
	*puc_msg++ = (uint8_t)(us_pib & 0xFF);
	*puc_msg++ = uc_index; /* index */
	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void bmng_pprof_set_request(uint8_t *puc_eui48, uint16_t us_pib, uint8_t *puc_value, uint8_t uc_pib_len )
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_pprof_set_request_cmd;
	memcpy(puc_msg, puc_eui48, 6);
	puc_msg += 6;
	*puc_msg++ = 0; /* length */
	*puc_msg++ = 2 + uc_pib_len; /* length */
	/* pib attribute */
	*puc_msg++ = (uint8_t)(us_pib >> 8);
	*puc_msg++ = (uint8_t)(us_pib & 0xFF);
	/* pib value */
	memcpy(puc_msg, puc_value, uc_pib_len);
	puc_msg += uc_pib_len;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void bmng_pprof_reboot(uint8_t *puc_eui48)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_pprof_reboot_request_cmd;
	memcpy(puc_msg, puc_eui48, 6);
	puc_msg += 6;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void bmng_pprof_reset(uint8_t *puc_eui48)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_pprof_reset_request_cmd;
	memcpy(puc_msg, puc_eui48, 6);
	puc_msg += 6;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

/*Item Enhanced header id */
#ifndef MNGP_PRIME_LISTQRY
#define MNGP_PRIME_LISTQRY                      0x0E
#define MNGP_PRIME_LISTRSP                      0x0F
#endif

void bmng_pprof_get_enhanced(uint8_t *puc_eui48, uint16_t us_pib, uint8_t uc_max_records, uint16_t us_iterator)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_pprof_get_enhanced_request_cmd;
	memcpy(puc_msg, puc_eui48, 6);
	puc_msg += 6;

	*puc_msg++ = 0; /* length */
	*puc_msg++ = 6; /* length */

	*puc_msg++ = MNGP_PRIME_LISTQRY;
	*puc_msg++ = (uint8_t)(us_pib >> 8);
	*puc_msg++ = (uint8_t)(us_pib & 0xFF);
	*puc_msg++ = uc_max_records;

	memcpy(puc_msg, &us_iterator, 2);
	puc_msg++;
	puc_msg++;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void bmng_pprof_zero_cross_request(uint8_t *puc_eui48)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_pprof_zc_diff_request_cmd;
	memcpy(puc_msg, puc_eui48, 6);
	puc_msg += 6;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

uint8_t _bmng_pprof_get_response_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (pprof_get_response_cb) {
		uint8_t puc_eui48[LEN_ID_EUI48];

		void *pv_pib_value;
		uint16_t us_data_len;

		memcpy(puc_eui48, ptrMsg, 6);
		ptrMsg += 6;
		us_data_len = ((uint16_t)(*ptrMsg++)) << 8;
		us_data_len += *ptrMsg++;

		pv_pib_value = (void *)ptrMsg;

		pprof_get_response_cb(puc_eui48, us_data_len, pv_pib_value);
	}

	return(true);
}

uint8_t _bmng_pprof_get_enhanced_response_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (pprof_get_enhanced_cb) {
		uint8_t puc_eui48[LEN_ID_EUI48];

		void *pv_pib_value;
		uint16_t us_data_len;

		memcpy(puc_eui48, ptrMsg, 6);
		ptrMsg += 6;
		us_data_len = ((uint16_t)(*ptrMsg++)) << 8;
		us_data_len += *ptrMsg++;

		pv_pib_value = (void *)ptrMsg;

		pprof_get_enhanced_cb(puc_eui48, us_data_len, pv_pib_value);
	}

	return(true);
}

uint8_t _bmng_pprof_get_zc_response_cmd(uint8_t *ptrMsg, uint16_t len)
{
	if (pprof_zerocross_response_cb) {
		uint8_t puc_eui48[LEN_ID_EUI48];
		uint8_t uc_id;
		uint32_t ui_zct;

		memcpy(puc_eui48, ptrMsg, 6);
		ptrMsg += 6;
		uc_id = ((uint8_t)(*ptrMsg++));

		ui_zct  = ((uint32_t)(*ptrMsg++)) << 24;
		ui_zct += ((uint32_t)(*ptrMsg++)) << 16;
		ui_zct += ((uint32_t)(*ptrMsg++)) << 8;
		ui_zct += (*ptrMsg++);

		pprof_zerocross_response_cb(puc_eui48, uc_id, ui_zct);
	}

	return(true);
}

uint8_t _bmng_pprof_get_zc_diff_response_cmd(uint8_t *ptrMsg, uint16_t len)
{
	if (pprof_zc_diff_response_cb) {
		uint8_t puc_eui48[LEN_ID_EUI48];
		uint32_t ui_time;
		uint32_t ui_diff;

		memcpy(puc_eui48, ptrMsg, 6);
		ptrMsg += 6;

		ui_time  = ((uint32_t)(*ptrMsg++)) << 24;
		ui_time += ((uint32_t)(*ptrMsg++)) << 16;
		ui_time += ((uint32_t)(*ptrMsg++)) << 8;
		ui_time += (*ptrMsg++);

		ui_diff  = ((uint32_t)(*ptrMsg++)) << 24;
		ui_diff += ((uint32_t)(*ptrMsg++)) << 16;
		ui_diff += ((uint32_t)(*ptrMsg++)) << 8;
		ui_diff += (*ptrMsg++);

		pprof_zc_diff_response_cb(puc_eui48, ui_time, ui_diff);
	}

	return(true);
}

uint8_t _bmng_pprof_ack_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (pprof_ack_ind_cb) {
		uint8_t uc_cmd;
		uint8_t uc_resp;

		uc_cmd =  *ptrMsg++;
		uc_resp = *ptrMsg++;

		pprof_ack_ind_cb(uc_cmd, uc_resp);
	}

	return(true);
}

uint8_t _bmng_whitelist_ack_cmd(uint8_t *ptrMsg, uint16_t len)
{
	if (bmng_whitelist_ack_cb) {
		uint8_t uc_cmd;
		uint8_t uc_resp;

		uc_cmd =  *ptrMsg++;
		uc_resp = *ptrMsg++;

		bmng_whitelist_ack_cb(uc_cmd, uc_resp);
	}

	return(true);
}

void bmng_whitelist_add_request(uint8_t *puc_eui48)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_whitelist_add_request_cmd;
	memcpy(puc_msg, puc_eui48, 6);
	puc_msg += 6;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

void bmng_whitelist_remove_request(uint8_t *puc_eui48)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = prime_bmng_whitelist_remove_request_cmd;
	memcpy(puc_msg, puc_eui48, 6);
	puc_msg += 6;

	/*Send to USI*/
	prime_api_msg.pType = PROTOCOL_PRIME_API;
	prime_api_msg.buf = uc_serial_buf;
	prime_api_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&prime_api_msg);
}

uint8_t _prime_debug_report_ind(uint8_t *ptrMsg, uint16_t len)
{
	if (prime_debug_ind_cd) {
		uint32_t ui_error_code;

		if (len != 4) {
			return(false);
		}

		ui_error_code  = ((uint32_t)(*ptrMsg++)) << 24;
		ui_error_code += ((uint32_t)(*ptrMsg++)) << 16;
		ui_error_code += ((uint32_t)(*ptrMsg++)) << 8;
		ui_error_code += (*ptrMsg++);

		prime_debug_ind_cd(ui_error_code);
	}

	return(true);
}

void prime_debug_set_cb(prime_debug_error_indication_cb_t debug_cb)
{
	prime_debug_ind_cd = debug_cb;
}

uint8_t ifacePrime_api_ReceivedCmd(uint8_t *ptrMsg, uint16_t len)
{
	uint8_t uc_cmd;

	uc_cmd = (prime_api_cmd_t)((*ptrMsg++) & CMD_PROTOCOL_MSK);
	len--;

	switch (uc_cmd) {
	case prime_cl_null_establish_ind_cmd:
		return _cl_null_establish_ind(ptrMsg, len);

		break;

	case prime_cl_null_establish_cfm_cmd:
		return _cl_null_establish_cfm(ptrMsg, len);

		break;

	case prime_cl_null_release_ind_cmd:
		return _cl_null_release_ind(ptrMsg, len);

		break;

	case prime_cl_null_release_cfm_cmd:
		return _cl_null_release_cfm(ptrMsg, len);

		break;

	case prime_cl_null_join_ind_cmd:
		return _cl_null_join_ind(ptrMsg, len);

		break;

	case prime_cl_null_join_cfm_cmd:
		return _cl_null_join_cfm(ptrMsg, len);

		break;

	case prime_cl_null_leave_cfm_cmd:
		return _cl_null_leave_cfm(ptrMsg, len);

		break;

	case prime_cl_null_leave_ind_cmd:
		return _cl_null_leave_ind(ptrMsg, len);

		break;

	case prime_cl_null_data_cfm_cmd:
		return _cl_null_data_cfm(ptrMsg, len);

		break;

	case prime_cl_null_data_ind_cmd:
		return _cl_null_data_ind(ptrMsg, len);

		break;

	case prime_cl_null_plme_reset_cfm_cmd:
		return _cl_null_plme_reset_cfm(ptrMsg, len);

		break;

	case prime_cl_null_plme_sleep_cfm_cmd:
		return _cl_null_plme_sleep_cfm(ptrMsg, len);

		break;

	case prime_cl_null_plme_resume_cfm_cmd:
		return _cl_null_plme_resume_cfm(ptrMsg, len);

		break;

	case prime_cl_null_plme_testmode_cfm_cmd:
		return _cl_null_plme_testmode_cfm(ptrMsg, len);

		break;

	case prime_cl_null_plme_get_cfm_cmd:
		return _cl_null_plme_get_cfm(ptrMsg, len);

		break;

	case prime_cl_null_plme_set_cfm_cmd:
		return _cl_null_plme_set_cfm(ptrMsg, len);

		break;

	case prime_cl_null_mlme_register_cfm_cmd:
		return _cl_null_mlme_register_cfm(ptrMsg, len);

		break;

	case prime_cl_null_mlme_register_ind_cmd:
		return _cl_null_mlme_register_ind(ptrMsg, len);

		break;

	case prime_cl_null_mlme_unregister_cfm_cmd:
		return _cl_null_mlme_unregister_cfm(ptrMsg, len);

		break;

	case prime_cl_null_mlme_unregister_ind_cmd:
		return _cl_null_mlme_unregister_ind(ptrMsg, len);

		break;

	case prime_cl_null_mlme_promote_cfm_cmd:
		return _cl_null_mlme_promote_cfm(ptrMsg, len);

		break;

	case prime_cl_null_mlme_promote_ind_cmd:
		return _cl_null_mlme_promote_ind(ptrMsg, len);

		break;

	case prime_cl_null_mlme_demote_cfm_cmd:
		return _cl_null_mlme_demote_cfm(ptrMsg, len);

		break;

	case prime_cl_null_mlme_demote_ind_cmd:
		return _cl_null_mlme_demote_ind(ptrMsg, len);

		break;

	case prime_cl_null_mlme_reset_cfm_cmd:
		return _cl_null_mlme_reset_cfm(ptrMsg, len);

		break;

	case prime_cl_null_mlme_get_cfm_cmd:
		return _cl_null_mlme_get_cfm(ptrMsg, len);

		break;

	case prime_cl_null_mlme_list_get_cfm_cmd:
		return _cl_null_mlme_list_get_cfm(ptrMsg, len);

		break;

	case prime_cl_null_mlme_set_cfm_cmd:
		return _cl_null_mlme_set_cfm(ptrMsg, len);

		break;

	case prime_cl_432_establish_cfm_cmd:
		return _cl_432_establish_cfm(ptrMsg, len);

		break;

	case prime_cl_432_release_cfm_cmd:
		return _cl_432_release_cfm(ptrMsg, len);

		break;

	case prime_cl_432_dl_data_ind_cmd:
		return _cl_432_dl_data_ind(ptrMsg, len);

		break;

	case prime_cl_432_dl_data_cfm_cmd:
		return _cl_432_dl_data_cfm(ptrMsg, len);

		break;

	case prime_cl_432_dl_join_ind_cmd: /* = 0x3A, //NOT IMPLEMENTED */
		return _cl_432_dl_join_ind(ptrMsg, len);

		break;

	case prime_cl_432_dl_leave_ind_cmd: /* 0x3B, //NOT IMPLEMENTED */
		return _cl_432_dl_leave_ind(ptrMsg, len);

		break;

	case prime_bmng_fup_ack_cmd:
		return _prime_fup_ack_ind(ptrMsg, len);

		break;

	case prime_bmng_fup_status_ind_cmd:
		return _prime_fup_status_ind(ptrMsg, len);

		break;

	case prime_bmng_fup_error_ind_cmd:
		return _prime_fup_error_ind(ptrMsg, len);

		break;

	case prime_bmng_fup_version_ind_cmd:
		return _prime_fup_version_ind(ptrMsg, len);

		break;

	case prime_bmng_fup_kill_ind_cmd:
		return _prime_fup_kill_ind(ptrMsg, len);

		break;

	case prime_bmng_network_event_cmd:
		return _bmng_network_ind(ptrMsg, len);

		break;

	case prime_bmng_pprof_ack_cmd:
		return _bmng_pprof_ack_ind(ptrMsg, len);

		break;

	case prime_bmng_pprof_get_response_cmd:
		return _bmng_pprof_get_response_ind(ptrMsg, len);

		break;

	case prime_bmng_pprof_get_enhanced_response_cmd:
		return _bmng_pprof_get_enhanced_response_ind(ptrMsg, len);

		break;

	case prime_bmng_pprof_get_zc_response_cmd:
		return _bmng_pprof_get_zc_response_cmd(ptrMsg, len);

		break;

	case prime_bmng_pprof_zc_diff_response_cmd:
		return _bmng_pprof_get_zc_diff_response_cmd(ptrMsg, len);

		break;

	case prime_bmng_whitelist_ack_cmd:
		return _bmng_whitelist_ack_cmd(ptrMsg, len);

		break;

	case prime_bmng_debug_report_indication:
		return _prime_debug_report_ind(ptrMsg, len);

	default:
		return(false);

		break;
	}
}

void ifacePrime_api_init()
{
	memset(&_data_pointer_table[0], 0, sizeof(cl_null_data_pointer) * NUM_ELEMENTS_POINTER_TABLE);
}
