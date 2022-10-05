/**
 * \file
 *
 * \brief USI Host PRIME API Definitions
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


#ifndef PRIME_API_DEFS_H
#define PRIME_API_DEFS_H

#include "mac_defs.h"
#include "cl_432_defs.h"

#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* @endcond */

/* PRIME_API callbacks type definition */
/* @{ */
/** Page size for FU */
typedef enum {
	PAGE_SIZE_AUTO = 0,
	PAGE_SIZE_32,
	PAGE_SIZE_64,
	PAGE_SIZE_128,
	PAGE_SIZE_192
} fup_page_size_t;

/** FUP ACK codes  */
typedef enum {
	FUP_ACK_OK = 0,
	FUP_ACK_ERROR,
	FUP_ACK_ERROR_MAC,
	FUP_ACK_ERROR_MODEL,
	FUP_ACK_ERROR_CRC,
	FUP_ACK_ERROR_DATA,
	FUP_ACK_ERROR_CRC_FILE,
	FUP_ACK_CRC_ONGOING,
	FUP_ACK_FU_ONGOING
} x_fup_ack_code_t;

/** PLC upgrade state */
typedef enum {
    FUP_STATE_IDLE        = 0x00,
    FUP_STATE_RECEIVING   = 0x01,
    FUP_STATE_COMPLETE    = 0x02,
    FUP_STATE_COUNTDOWN   = 0x03,
    FUP_STATE_UPGRADED    = 0x04,
    FUP_STATE_EXCEPTION   = 0x05,  /* Only 1.4 */
    FUP_STATE_UNKNOWN     = 0x7F,
    FUP_STATE_CONFIRMED   = 0x80   /* Correspond with FUP_STATE_ENDED_NOTIFICATION */
} x_fup_state_t;

/** PLC upgrade excepion state */
typedef enum {
    FUP_EXCEPTION_GENERAL             = 0x00,
    FUP_EXCEPTION_PROTOCOL            = 0x01,
    FUP_EXCEPTION_CRC_FAIL            = 0x02,
    FUP_EXCEPTION_INVALID_IMG         = 0x03,
    FUP_EXCEPTION_SIGNATURE           = 0x04,
    FUP_EXCEPTION_SAFETY_TIME_EXPIRED = 0x05  /* Only 1.4 */
} x_fup_exception_state_t;

/** FUP error codes  */
typedef enum {
	FUP_ERROR_NODE_NO_ERROR = 0,
	FUP_ERROR_NODE_WRONGSTATE_EXEC,
	FUP_ERROR_NODE_WRONGSTATE_UPG,
	FUP_ERROR_NODE_WRONGSTATE,
	FUP_ERROR_NODE_WRONGSTATE_RCV,
	FUP_ERROR_NODE_WRONGSTATE_CNTDWN,
	FUP_ERROR_NODE_FW_NOTMATCH,
	FUP_ERROR_NODE_REVERT_ERRORINIT,
	FUP_ERROR_NODE_REVERT_ERR7,
	FUP_ERROR_NODE_RETRY_KILL,
	FUP_ERROR_NODE_UNICAST_TIMEOUT,
	FUP_ERROR_NODE_CONFIRM_IND,
	FUP_ERROR_NODE_REVERT_CRCNOK,
	FUP_ERROR_NODE_RESTART,
	FUP_ERROR_NODE_WRONG_MISS_BITMAP,
	FUP_ERROR_NODE_WRONG_MISS_LIST,
	FUP_ERROR_NODE_VENDOR_INVALID,
	FUP_ERROR_NODE_MODEL_NOTMATCH,
	FUP_WARNING_NODE_ALREADY_UPDATED,
	FUP_WARNING_NODE_ALREADY_EXECUTED,
	FUP_WARNING_NODE_LINK_QUALITY
} x_fup_error_code_t;

/** Network events */
typedef enum {
	BMNG_NET_EVENT_REGISTER,
	BMNG_NET_EVENT_UNREGISTER,
	BMNG_NET_EVENT_PROMOTE,
	BMNG_NET_EVENT_DEMOTE,
	BMNG_NET_EVENT_ALIVE,
	BMNG_NET_EVENT_REBOOT,
	BMNG_NET_EVENT_NO_DUK,
	BMNG_NET_EVENT_UNKNOWN_NODE,
	BMNG_NET_EVENT_ERROR
} bmng_event_t;

/** Network Event information */
typedef struct {
	bmng_event_t net_event;
	uint8_t mac[6];
	uint8_t sid;
	uint16_t lnid;
	uint8_t lsid;
	uint8_t alvRxcnt;
	uint8_t alvTxcnt;
	uint8_t alvTime;
} bmng_net_event_t;

/** PRIME Profile ACK codes  */
typedef enum {
	PPROF_ACK_OK = 0,
	PPROF_ACK_ERROR
} pprof_ack_code_t;

/** PRIME Whitelist ACK codes  */
typedef enum {
	WLIST_ACK_OK = 0,
	WLIST_ERROR
} whitelist_ack_code_t;

/**
 * MAC Establish Indication
 * - us_con_handle: Unique identifier of the connection
 * - puc_eui48:     Address of the node which initiates the connection establish procedure
 * - uc_type:       Convergence Layer type of the connection
 * - puc_data:      Data associated with the connection establishment procedure
 * - us_data_len:   Length of the data in bytes
 * - uc_cfbytes:    Flag to indicate whether or not the connection should use the contention or contention-free channel access scheme
 * - uc_ae:         Flag to indicate that authentication and encryption is requested
 */

typedef void (*prime_cl_null_establish_ind_cb_t)(uint16_t us_con_handle, uint8_t *puc_eui48, uint8_t uc_type, uint8_t *puc_data,
		uint16_t us_data_len, uint8_t uc_cfbytes, uint8_t uc_ae);

/**
 * MAC Establish Confirm
 * - us_con_handle: Unique identifier of the connection
 * - uc_result:     Result of the connection establishment process
 * - puc_eui48:     Address of the node to which this connection is being established
 * - uc_type:       Convergence Layer type of the connection
 * - puc_data:      Data associated with the connection establishment procedure
 * - us_data_len:   Length of the data in bytes
 * - uc_ae:         Flag to indicate that authentication and encryption is requested
 */
typedef void (*prime_cl_null_establish_cfm_cb_t)(uint16_t us_con_handle, mac_establish_confirm_result_t uc_result,
		uint8_t *puc_eui48, uint8_t uc_type, uint8_t *puc_data, uint16_t us_data_len, uint8_t uc_ae);

/**
 * MAC Release Indication
 * - us_con_handle: Unique identifier of the connection
 * - uc_reason:     Cause of the connection release
 */
typedef void (*prime_cl_null_release_ind_cb_t)(uint16_t us_con_handle, mac_release_indication_reason_t uc_reason);

/**
 * MAC Release Confirm
 * - us_con_handle: Unique identifier of the connection
 * - uc_result:     Result of the connection release process
 */
typedef void (*prime_cl_null_release_cfm_cb_t)(uint16_t us_con_handle, mac_release_confirm_result_t uc_result);

/**
 * MAC Join Indication
 * - us_con_handle:        Unique identifier of the connection
 * - puc_eui48:            Address of the node which wishes to join the multicast group (only valid for base node)
 * - uc_con_type:          Connection type
 * - puc_data:             Data associated with the join request procedure
 * - us_data_len:          Length of the data in bytes
 * - uc_ae:                Flag to indicate that authentication and encryption is requested
 */
typedef void (*prime_cl_null_join_ind_cb_t)(uint16_t us_con_handle, uint8_t *puc_eui48, uint8_t uc_con_type, uint8_t *puc_data, uint16_t us_data_len, uint8_t uc_ae);

/**
 * MAC Join Confirm
 * - us_con_handle:        Unique identifier of the connection
 * - uc_result:            Result of the join request process
 * - uc_ae:                Flag to indicate that authentication and encryption is requested
 */
typedef void (*prime_cl_null_join_cfm_cb_t)(uint16_t us_con_handle, mac_join_confirm_result_t uc_result, uint8_t uc_ae);

/**
 * MAC Leave Indication
 * - us_con_handle:        Unique identifier of the connection
 * - puc_eui48:            Address of the node to remove from the multicast group (only valid for base node)
 */
typedef void (*prime_cl_null_leave_ind_cb_t)(uint16_t us_con_handle, uint8_t *puc_eui48);

/**
 * MAC Leave Confirm
 * - us_con_handle:        Unique identifier of the connection
 * - uc_result:            Result of the leave request process
 */
typedef void (*prime_cl_null_leave_cfm_cb_t)(uint16_t us_con_handle, mac_leave_confirm_result_t uc_result);

/**
 * MAC Data Indication
 * - us_con_handle:    Unique identifier of the connection
 * - puc_data:         Pointer to data to be received through this connection
 * - us_data_len:      Length of the data in bytes
 */
typedef void (*prime_cl_null_data_ind_cb_t)(uint16_t us_con_handle, uint8_t *puc_data, uint16_t us_data_len, uint32_t ul_time_ref);

/**
 * MAC Data Confirm
 * - us_con_handle:    Unique identifier of the connection
 * - puc_data:         Pointer to data to be transmitted through this connection
 * - drt_result:       Result of the transmission (MAC_DATA_SUCCESS, MAC_DATA_TIMEOUT, MAC_DATA_ERROR_SENDING)
 */
typedef void (*prime_cl_null_data_cfm_cb_t)(uint16_t us_con_handle, uint8_t *puc_data, mac_data_result_t drt_result);

/**
 * PLME Reset Confirm
 * - x_result:           Result
 */
typedef void (*prime_cl_null_plme_reset_cfm_cb_t)(plme_result_t x_result);

/**
 * PLME Sleep Confirm
 * - x_result:           Result
 */
typedef void (*prime_cl_null_plme_sleep_cfm_cb_t)(plme_result_t x_result);

/**
 * PLME Resume Confirm
 * - x_result:           Result
 */
typedef void (*prime_cl_null_plme_resume_cfm_cb_t)(plme_result_t x_result);

/**
 * PLME Testmode Confirm
 * - x_result:           Result
 */
typedef void (*prime_cl_null_plme_testmode_cfm_cb_t)(plme_result_t x_result);

/**
 * PLME Get Confirm
 * - x_status:           Status
 * - us_pib_attrib:      PIB attribute
 * - pv_pib_value:       PIB attribute value
 * - uc_pib_size:        PIB attribute value size
 */
typedef void (*prime_cl_null_plme_get_cfm_cb_t)(plme_result_t x_status, uint16_t us_pib_attrib, void *pv_pib_value, uint8_t uc_pib_size);

/**
 * PLME Set Confirm
 * - x_result:           Result
 */
typedef void (*prime_cl_null_plme_set_cfm_cb_t)(plme_result_t x_result);

/**
 * MLME Register Confirm
 * - x_result:     Result
 * - puc_sna:      Pointer to SNA
 * - uc_sid:       Switch Identifier
 */
typedef void (*prime_cl_null_mlme_register_cfm_cb_t)(mlme_result_t x_result, uint8_t *puc_sna, uint8_t uc_sid);

/**
 * MLME Register Indication
 * - puc_sna:      Pointer to SNA
 * - uc_sid:       Switch Identifier
 */
typedef void (*prime_cl_null_mlme_register_ind_cb_t)(uint8_t *puc_sna, uint8_t uc_sid);

/**
 * MLME Unregister Confirm
 * - x_result:      Result
 */
typedef void (*prime_cl_null_mlme_unregister_cfm_cb_t)(mlme_result_t x_result);

/**
 * MLME Unregister Indication
 */
typedef void (*prime_cl_null_mlme_unregister_ind_cb_t)(void);

/**
 * MLME Promote Confirm
 * - x_result:      Result
 */
typedef void (*prime_cl_null_mlme_promote_cfm_cb_t)(mlme_result_t x_result);

/**
 * MLME Promote Indication
 */
typedef void (*prime_cl_null_mlme_promote_ind_cb_t)(void);

/**
 * MLME Demote Confirm
 * - x_result:      Result
 */
typedef void (*prime_cl_null_mlme_demote_cfm_cb_t)(mlme_result_t x_result);

/**
 * MLME Demote Indication
 */
typedef void (*prime_cl_null_mlme_demote_ind_cb_t)(void);

/**
 * MLME Reset Confirm
 * - x_result:           Result
 */
typedef void (*prime_cl_null_mlme_reset_cfm_cb_t)(mlme_result_t x_result);

/**
 * MLME Get Confirm
 * - x_status:           Status
 * - us_pib_attrib:      PIB attribute
 * - pv_pib_value:       PIB attribute value
 * - uc_pib_size:        PIB attribute value size
 */
typedef void (*prime_cl_null_mlme_get_cfm_cb_t)(mlme_result_t x_status, uint16_t us_pib_attrib, void *pv_pib_value, uint8_t uc_pib_size);

/**
 * MLME List Get Confirm
 * - x_status:           Status
 * - us_pib_attrib:      PIB attribute
 * - puc_pib_buff:       Buffer with PIB attribute values
 * - us_pib_len:         Buffer length
 */
typedef void (*prime_cl_null_mlme_list_get_cfm_cb_t)(mlme_result_t x_status, uint16_t us_pib_attrib, uint8_t *puc_pib_buff, uint16_t us_pib_len);

/**
 * MLME Set Confirm
 * - x_result:           Result
 */
typedef void (*prime_cl_null_mlme_set_cfm_cb_t)(mlme_result_t x_result);

/** MAC callbacks configuration */
typedef struct {
	prime_cl_null_establish_ind_cb_t prime_cl_null_establish_ind_cb;
	prime_cl_null_establish_cfm_cb_t prime_cl_null_establish_cfm_cb;
	prime_cl_null_release_ind_cb_t prime_cl_null_release_ind_cb;
	prime_cl_null_release_cfm_cb_t prime_cl_null_release_cfm_cb;
	prime_cl_null_join_ind_cb_t prime_cl_null_join_ind_cb;
	prime_cl_null_join_cfm_cb_t prime_cl_null_join_cfm_cb;
	prime_cl_null_leave_ind_cb_t prime_cl_null_leave_ind_cb;
	prime_cl_null_leave_cfm_cb_t prime_cl_null_leave_cfm_cb;
	prime_cl_null_data_ind_cb_t prime_cl_null_data_ind_cb;
	prime_cl_null_data_cfm_cb_t prime_cl_null_data_cfm_cb;
	prime_cl_null_plme_reset_cfm_cb_t prime_cl_null_plme_reset_cfm_cb;
	prime_cl_null_plme_sleep_cfm_cb_t prime_cl_null_plme_sleep_cfm_cb;
	prime_cl_null_plme_resume_cfm_cb_t prime_cl_null_plme_resume_cfm_cb;
	prime_cl_null_plme_testmode_cfm_cb_t prime_cl_null_plme_testmode_cfm_cb;
	prime_cl_null_plme_get_cfm_cb_t prime_cl_null_plme_get_cfm_cb;
	prime_cl_null_plme_set_cfm_cb_t prime_cl_null_plme_set_cfm_cb;
	prime_cl_null_mlme_register_ind_cb_t prime_cl_null_mlme_register_ind_cb;
	prime_cl_null_mlme_register_cfm_cb_t prime_cl_null_mlme_register_cfm_cb;
	prime_cl_null_mlme_unregister_ind_cb_t prime_cl_null_mlme_unregister_ind_cb;
	prime_cl_null_mlme_unregister_cfm_cb_t prime_cl_null_mlme_unregister_cfm_cb;
	prime_cl_null_mlme_promote_ind_cb_t prime_cl_null_mlme_promote_ind_cb;
	prime_cl_null_mlme_promote_cfm_cb_t prime_cl_null_mlme_promote_cfm_cb;
	prime_cl_null_mlme_demote_ind_cb_t prime_cl_null_mlme_demote_ind_cb;
	prime_cl_null_mlme_demote_cfm_cb_t prime_cl_null_mlme_demote_cfm_cb;
	prime_cl_null_mlme_reset_cfm_cb_t prime_cl_null_mlme_reset_cfm_cb;
	prime_cl_null_mlme_get_cfm_cb_t prime_cl_null_mlme_get_cfm_cb;
	prime_cl_null_mlme_list_get_cfm_cb_t prime_cl_null_mlme_list_get_cfm_cb;
	prime_cl_null_mlme_set_cfm_cb_t prime_cl_null_mlme_set_cfm_cb;
} prime_cl_null_callbacks_t;

/**
 * CL 432 Establish confirm
 *
 * - puc_device_id:          Pointer to the device identifier data
 * - uc_device_id_len:       Length of the device identfier
 * - us_dst_address:         Destination 432 Address
 * - us_base_address:        Base 432 Address
 */
typedef void (*prime_cl_432_establish_cfm_cb_t)(uint8_t *puc_device_id, uint8_t uc_device_id_len, uint16_t us_dst_address, uint16_t us_base_address, uint8_t uc_ae);

/**
 * CL 432 Release confirm
 *
 * - us_dst_address:   Destination 432 Address
 * - uc_result:        Confirmation result
 */
typedef void (*prime_cl_432_release_cfm_cb_t)(uint16_t us_dst_address, dl_432_result_t uc_result);

/**
 * CL 432 Data indication
 *
 * - uc_dst_lsap:      Destination LSAP
 * - uc_src_lsap:      Source LSAP
 * - us_dst_address:   Destination 432 Address
 * - src_address:      Source Address
 * - puc_data:         Pointer to received data
 * - us_lsdu_len:      Length of the data
 * - uc_link_class:    Link class (non used)
 */
typedef void (*prime_cl_432_dl_data_ind_cb_t)(uint8_t uc_dst_lsap, uint8_t uc_src_lsap, uint16_t us_dst_address, uint16_t src_address, uint8_t *puc_data,
		uint16_t us_lsdu_len, uint8_t uc_link_class);

/**
 * CL 432 Data confirm
 *
 * - uc_dst_lsap:      Destination LSAP
 * - uc_src_lsap:      Source LSAP
 * - us_dst_address:   Destination 432 Address
 * - uc_tx_status:     Tx status
 */
typedef void (*prime_cl_432_dl_data_cfm_cb_t)(uint8_t uc_dst_lsap, uint8_t uc_src_lsap, uint16_t us_dst_address, uint8_t uc_tx_status);

/**
 * CL 432 Leave Indication callback
 *
 * - uc_lnid:          LNID of the node closing the 432 connection
 */
typedef void (*prime_cl_432_dl_leave_ind_cb_t)(uint16_t us_lnid);

/**
 * CL 432 Join Indication callback
 *
 * - uc_lnid:          LNID of the node joining the 432 connection
 */
typedef void (*prime_cl_432_dl_join_ind_cb_t)(uint8_t *puc_device_id, uint8_t uc_device_id_len, uint16_t us_dst_address, uint8_t *puc_mac, uint8_t uc_ae);

/** 4-32 callbacks configuration */
typedef struct {
	prime_cl_432_establish_cfm_cb_t prime_cl_432_establish_cfm_cb;
	prime_cl_432_release_cfm_cb_t prime_cl_432_release_cfm_cb;
	prime_cl_432_dl_data_ind_cb_t prime_cl_432_dl_data_ind_cb;
	prime_cl_432_dl_data_cfm_cb_t prime_cl_432_dl_data_cfm_cb;
	prime_cl_432_dl_leave_ind_cb_t prime_cl_432_dl_leave_ind_cb;
	prime_cl_432_dl_join_ind_cb_t prime_cl_432_dl_join_ind_cb;
} prime_cl_432_callbacks_t;

/**
 * FUP ACK CALLBACK
 *
 * - uc_cmd:           Command response
 * - uc_ack:           Ack Value
 * - uc_dta:           Data, optional. If it a response to a data FUP message, contains the frame number.
 */
typedef void (*bmng_fup_ack_ind_cb_t)(uint8_t uc_cmd, uint8_t uc_ack, uint16_t us_data);

/**
 * Fup Status Indication Callback
 *
 * - uc_status:        PRIME FU status
 * - uc_pages:         Number of pages received by the SN
 * - puc_eui48:        EUI48 of the Service node
 */

typedef void (*bmng_fup_status_ind_cb_t)(uint8_t uc_status, uint32_t uc_pages, uint8_t *puc_eui48);

/**
 * Fup ERROR  Indication Callback
 *
 * - uc_error_code_:   Error code
 * - puc_eui48:        EUI48 of the Service node
 */

typedef void (*bmng_fup_error_ind_cb_t)(uint8_t uc_error_code, uint8_t *puc_eui48);

/**
 * Fup Version  Indication Callback
 *
 * - puc_eui48:        EUI48 of the Service node
 * - uc_vendor_len:    Length of the vendor value
 * - puc_vendor:       Buffer containing vendor value
 *
 * - uc_model_len:     Length of the model value
 * - puc_model:        Buffer containing model value
 *
 * - uc_version_len:   Length of the version value
 * - puc_version:      Buffer containing version value
 */

typedef void (*bmng_fup_version_ind_cb_t)(uint8_t *puc_eui48,
		uint8_t uc_vendor_len, uint8_t *puc_vendor,
		uint8_t uc_model_len, uint8_t *puc_model,
		uint8_t uc_version_len, uint8_t *puc_version );

/**
 * Fup Kill Indication Callback
 * Response to a FU_ABORT directive.
 *
 * - puc_eui48:        EUI48 of the node
 */

typedef void (*bmng_fup_kill_ind_cb_t)(uint8_t *puc_eui48);

/**
 * Base Management Network event indication Callback
 * Response to a FU_ABORT directive.
 *
 * - px_net_event: event
 */

typedef void (*bmng_network_event_ind_cb_t)(bmng_net_event_t *px_net_event);

/**
 * Base Management PRIME Profile get pib response callback.
 * Response to a PPROF_GET_REQUEST directive.
 *
 * - px_net_event: event
 */

typedef void (*bmng_pprof_get_response_cb_t)(uint8_t *puc_eui48, uint16_t us_data_len, uint8_t *puc_data);

/**
 * Base Management PRIME Profile ACK indication response callback.
 * Response to a PPROF GET directive.
 *
 * - px_net_event: event
 */

typedef void (*bmng_pprof_ack_ind_cb_t)(uint8_t uc_cmd, pprof_ack_code_t x_ack_code);

/**
 * Base Management PRIME Profile Get Enhanced Response
 * - puc_eui48:            MAC address of the node
 * - us_data_length:       Data length
 * - puc_data:             Data
 */
typedef void (*bmng_pprof_get_enhanced_response_cb_t)(uint8_t *puc_eui48, uint16_t us_data_len, uint8_t *puc_data);

/**
 * Base Management PRIME Profile Zero Crossing Response
 * - puc_eui48:            MAC address of the node
 * - uc_zc_status:         Zero cross status
 * - ul_zc_time:           Zero crossing time (in 10s of microsec for ATPL230, in microsec for PL360)
 */
typedef void (*bmng_pprof_zerocross_response_cb_t)(uint8_t *puc_eui48, uint8_t uc_zc_status, uint32_t ul_zc_time);

/**
 * Base Management PRIME Profile Zero Crossing Difference between BN and SN Response
 * - puc_eui48:            MAC address of the node
 * - uc_time_freq:         Frequency
 * - ul_time_diff:         Zero crossing difference (in 10s of microsec for ATPL230, in microsec for PL360)
 */
typedef void (*bmng_pprof_zc_diff_response_cb_t)(uint8_t *puc_eui48, uint32_t uc_time_freq, uint32_t ul_time_diff);

/**
 * Base Management Whitelist Acknowledgement
 * - uc_cmd:             Command being acknowledged
 * - x_ack_code:         ACK returned code
 */
typedef void (*bmng_whitelist_ack_cb_t)(uint8_t uc_cmd, whitelist_ack_code_t x_ack_code);

typedef struct {
	bmng_fup_ack_ind_cb_t fup_ack_ind_cb;
	bmng_fup_error_ind_cb_t fup_error_ind_cb;
	bmng_fup_version_ind_cb_t fup_version_ind_cb;
	bmng_fup_status_ind_cb_t fup_status_ind_cb;
	bmng_fup_kill_ind_cb_t fup_kill_ind_cb;
	bmng_network_event_ind_cb_t network_event_ind_cb;
	bmng_pprof_ack_ind_cb_t pprof_ack_ind_cb;
	bmng_pprof_get_response_cb_t pprof_get_response_cb;
	bmng_pprof_get_enhanced_response_cb_t pprof_get_enhanced_response_cb;
	bmng_pprof_zerocross_response_cb_t pprof_zerocross_response_cb;
	bmng_pprof_zc_diff_response_cb_t pprof_zc_diff_response_cb;
	bmng_whitelist_ack_cb_t whitelist_ack_cb;
} prime_bmng_callbacks_t;

/**
 * Prime Debug Indication Callback
 * Debug event from PRIME Stack
 *
 * ui_error_code:        error code from stack
 */

typedef void (*prime_debug_error_indication_cb_t)(uint32_t ui_error_code);

#ifdef __cplusplus
}
#endif

#endif /* PRIME_API_DEFS_H */
