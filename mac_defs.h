/**
 * \file
 *
 * \brief MAC_DEFS: PRIME MAC definitions
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

#ifndef MAC_DEFS_H_INCLUDE
#define MAC_DEFS_H_INCLUDE

/* System includes */
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* @endcond */

/**
 * \ingroup prime_ng_group
 * \defgroup prime_mac_group PRIME MAC Layer
 *
 * This module provides configuration and utils for
 * the MAC layer of PRIME.
 * @{
 */

/** \brief MAC invalid values */
/* @{ */
#define MAC_INVALID_SID                         0xFF
#define MAC_INVALID_LNID                        0x3FFF
#define MAC_INVALID_LSID                        0xFF
#define MAC_BROADCAST_LNID                      0x3FFF
#define MAC_MULTICAST_LNID                      0x3FFE
#define MAC_INVALID_HANDLER                     0xFFFF
#define MAC_INVALID_LCID                        0xFFFF
/* @} */

/** PRIME version customer values */
typedef struct {
	char fw_version[16];
	char fw_model[16];
	char fw_vendor[16];
	char pib_version[16];
	uint16_t pib_vendor;
	uint16_t pib_model;
} mac_version_info_t;

/** PLME result values */
typedef enum {
	PLME_RESULT_SUCCESS     = 0,
	PLME_RESULT_FAILED      = 1,
	PLME_RESULT_REJECTED    = 2,
	PLME_RESULT_BADATTR     = 2,
} plme_result_t;

/** MLME result values */
typedef enum {
	MLME_RESULT_DONE        = 0,
	MLME_RESULT_FAILED      = 1,
	MLME_RESULT_REJECTED    = 1,
	MLME_RESULT_TIMEOUT     = 2,
  MLME_RESULT_NOSUCHDEVICE= 4,
	MLME_RESULT_NOSNA       = 8,
	MLME_RESULT_NOSWITCH    = 9,
	MLME_RESULT_REDUNDANT   = 10,
	MLME_RESULT_BADATTR     = 11,
	MLME_RESULT_OUTOFRANGE  = 12,
	MLME_RESULT_READONLY    = 13
} mlme_result_t;

/** Connection types */
typedef enum {
	MAC_CONNECTION_INVALID_TYPE         = 0,
	MAC_CONNECTION_IPV4_AR_TYPE         = 1,
	MAC_CONNECTION_IPV4_UNICAST_TYPE    = 2,
	MAC_CONNECTION_CL_432_TYPE          = 3,
	MAC_CONNECTION_MNGT_TYPE            = 4,
	MAC_CONNECTION_IPV6_AR_TYPE         = 5,
	MAC_CONNECTION_IPV6_UNICAST_TYPE    = 6,
} connection_type_t;

/** MAC SAP result values for MAC_ESTABLISH.confirm primitive */
typedef enum {
	MAC_ESTABLISH_CONFIRM_RESULT_SUCCESS            = 0,
	MAC_ESTABLISH_CONFIRM_RESULT_REJECT             = 1,
	MAC_ESTABLISH_CONFIRM_RESULT_TIMEOUT            = 2,
	MAC_ESTABLISH_CONFIRM_RESULT_NO_BANDWIDTH       = 3,
	MAC_ESTABLISH_CONFIRM_RESULT_NO_SUCH_DEVICE     = 4,
	MAC_ESTABLISH_CONFIRM_RESULT_REDIRECT_FAILED    = 5,
	MAC_ESTABLISH_CONFIRM_RESULT_NOT_REGISTERED     = 6,
	MAC_ESTABLISH_CONFIRM_RESULT_NO_MORE_LCIDS      = 7,
	MAC_ESTABLISH_CONFIRM_RESULT_PROCCESS_ACTIVE    = 0x80
} mac_establish_confirm_result_t;

/** Values for the answer parameter in MAC_ESTABLISH.response primitive */
typedef enum {
	MAC_ESTABLISH_RESPONSE_ANSWER_ACCEPT    = 0,
	MAC_ESTABLISH_RESPONSE_ANSWER_REJECT    = 1,
} mac_establish_response_answer_t;

/** Values for the reason parameter in MAC_RELEASE.indication primitive */
typedef enum {
	MAC_RELEASE_INDICATION_REASON_SUCCESS   = 0,
	MAC_RELEASE_INDICATION_REASON_ERROR     = 1,
} mac_release_indication_reason_t;

/** Values for the answer parameter in MAC_RELEASE.response primitive */
typedef enum {
	MAC_RELEASE_RESPONSE_ACCEPT = 0,
} mac_release_response_answer_t;

/** Values for the result parameter in MAC_RELEASE.confirm primitive */
typedef enum {
	MAC_RELEASE_CONFIRM_RESULT_SUCCESS          = 0,
	MAC_RELEASE_CONFIRM_RESULT_TIMEOUT          = 2,
	MAC_RELEASE_CONFIRM_RESULT_NOT_REGISTERED   = 6,
	MAC_RELEASE_CONFIRM_RESULT_PROCCESS_ACTIVE  = 0x80,
	MAC_RELEASE_CONFIRM_RESULT_BAD_HANDLER      = 0x81,
	MAC_RELEASE_CONFIRM_RESULT_NOT_OPEN_CONN    = 0x82,
	MAC_RELEASE_CONFIRM_RESULT_ERROR_SENDING    = 0x83,
	MAC_RELEASE_CONFIRM_RESULT_BAD_FLOW_MODE    = 0x84,
} mac_release_confirm_result_t;

/** Values of the Result parameter in MAC_DATA.confirm primitive */
typedef enum {
	MAC_DATA_SUCCESS = 0,
	MAC_DATA_TIMEOUT = 2,
	MAC_DATA_ERROR_SENDING = 0x80,
	MAC_DATA_ERROR_PROCESSING_PREVIOUS_REQUEST  = 0x81,
	MAC_DATA_ERROR_NO_FREE_BUFFERS              = 0x82,
	MAC_DATA_ERROR_CON_CLOSED                   = 0x83,
	MAC_DATA_ERROR_RECEIVING_DATA               = 0x84
} mac_data_result_t;

/** Type of join request */
typedef enum {
	REQUEST_MULTICAST       = 0,
	REQUEST_BROADCAST       = 1,
} mac_join_mode_t;

/** Values for the result parameter in MAC_JOIN.confirm primitive */
typedef enum {
	JOIN_CONFIRM_SUCCESS    = 0,
	JOIN_CONFIRM_FAILURE    = 1,
	JOIN_CONFIRM_TIMEOUT    = 2,
	NOT_REGISTERED          = 6,
} mac_join_confirm_result_t;

/** Values for the result parameter in MAC_JOIN.response primitive */
typedef enum {
	JOIN_RESPONSE_ACCEPT    = 0,
	JOIN_RESPONSE_REJECT    = 1,
} mac_join_response_answer_t;

/** Values for the result parameter in MAC_LEAVE.confirm primitive */
typedef enum {
	LEAVE_CONFIRM_ACCEPT    = 0,
	LEAVE_CONFIRM_TIMEOUT   = 1,
	LEAVE_CONFIRM_RESULT_PROCCESS_ACTIVE  = 0x80,
	LEAVE_CONFIRM_RESULT_BAD_HANDLER      = 0x81,
	LEAVE_CONFIRM_RESULT_NOT_OPEN_CONN    = 0x82,
	LEAVE_CONFIRM_RESULT_ERROR_SENDING    = 0x83,
	LEAVE_CONFIRM_RESULT_BAD_FLOW_MODE    = 0x84,
	LEAVE_CONFIRM_RESULT_NOT_REGISTERED   = 0x85
} mac_leave_confirm_result_t;

/** Certification test modes prime 1.3 and prime 1.4 */
typedef enum {
	PRIME1_3_FRAME = 0,
	PRIME1_4_TYPE_A_FRAME = 0,
	PRIME1_4_TYPE_B_FRAME = 2,
	PRIME1_4_PHY_BC_FRAME = 3
} cert_prime_test_frame_t;

/** Certification test parameters */
typedef struct {
	uint16_t us_cert_messg_count;
	uint8_t uc_cert_modulation;
	uint8_t us_cert_signal_att;
	uint8_t us_cert_duty_cycle;
	cert_prime_test_frame_t us_cert_prime_version;
} mlme_certification_parameter_t;

/** Certification modes */
typedef enum {
	NO_CERTIFICATION_MODE      = 0,
	PHY_CERTIFICATION_MODE     = 1,
	MAC_CERTIFICATION_MODE     = 2,
	PHY_CERTIFICATION_MODE_1_4 = 3,
} cert_mode_t;

/** Rejection actions */
typedef enum {
	REJECT_PRO_REQ_S      = 0,
	REJECT_PRM            = 1,
	REJECT_CON_REQ_S      = 2,
	REJECT_REG_REQ        = 3,
} cert_reject_action_t;

/* @} */

/* @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* @endcond */
#endif /* MAC_DEFS_H_INCLUDE */
