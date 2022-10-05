/**
 * \file
 *
 * \brief Serial Interface for ADP layer
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

/* System includes */
#include <stdint.h>
/* #include <stdio.h> */
#include <stdbool.h>
#include "string.h"

#include <PrjCfg.h>
/* Serial interface */
#include "serial_if_coordinator.h"
#include "serial_if_adp.h"
#include "serial_if_common.h"
#include "serial_if_mib_common.h"
#include "AdpApi.h"
#include "bs_api.h"
#include "AdpApiTypes.h"
#include "mac_wrapper.h"
#include "usi_cli.h"
#include "usi_cfg_cli.h"
#include "debug.h"

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

#ifdef USE_PROTOCOL_COORD_G3_PORT

/* Known message ids */
enum ESerialMessageId {
	/* Generic messages */
	SERIAL_MSG_STATUS = 0,

	/* ADP ACCESS */
	SERIAL_MSG_COORD_REQUEST_MESSAGES_BEGIN = 1,

	SERIAL_MSG_COORD_INITIALIZE = SERIAL_MSG_COORD_REQUEST_MESSAGES_BEGIN,
	SERIAL_MSG_COORD_SET_REQUEST,
	SERIAL_MSG_COORD_GET_REQUEST,
	SERIAL_MSG_COORD_KICK_REQUEST,
	SERIAL_MSG_COORD_REKEYING_REQUEST,

	SERIAL_MSG_COORD_SET_CONFIRM,
	SERIAL_MSG_COORD_GET_CONFIRM,
	SERIAL_MSG_COORD_JOIN_INDICATION,
	SERIAL_MSG_COORD_LEAVE_INDICATION,

	SERIAL_MSG_COORD_REQUEST_MESSAGES_END = SERIAL_MSG_COORD_LEAVE_INDICATION
};

/* Status codes related to HostInterface processing */
enum ESerialStatus {
	SERIAL_STATUS_SUCCESS = 0,
	SERIAL_STATUS_NOT_ALLOWED,
	SERIAL_STATUS_UNKNOWN_COMMAND,
	SERIAL_STATUS_INVALID_PARAMETER
};

/* ! \name Data structure to communicate with USI layer */
/* @{ */
static x_usi_serial_cmd_params_t x_adp_serial_msg;
/* @} */

static uint8_t uc_serial_rsp_buf[2048]; /* !<  Response working buffer */

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void MsgStatus(enum ESerialStatus status, uint8_t uc_serial_if_cmd)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "MsgStatus (bootstrap). Status = %u.\r\n", (uint8_t)status);

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_STATUS;
	uc_serial_rsp_buf[us_serial_response_len++] = status;
	uc_serial_rsp_buf[us_serial_response_len++] = uc_serial_if_cmd;
	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_COORD_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void BootstrapNotification_JoinIndication(uint8_t *pExtAddr, uint16_t u16SrcAddr)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "BootstrapNotification_JoinIndication.\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_COORD_JOIN_INDICATION;
	memcpy(&uc_serial_rsp_buf[us_serial_response_len], pExtAddr, 8);
	us_serial_response_len += 8;
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(u16SrcAddr >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(u16SrcAddr);
	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_COORD_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void BootstrapNotification_LeaveIndication(uint16_t u16SrcAddr,
		bool bSecurityEnabled,
		uint8_t u8LinkQualityIndicator,
		uint8_t *pNsdu,
		uint16_t u16NsduLength)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "BootstrapNotification_LeaveIndication.\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_COORD_LEAVE_INDICATION;
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(u16SrcAddr >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(u16SrcAddr & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(bSecurityEnabled);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(u8LinkQualityIndicator);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(u16NsduLength & 0xFF);
	memcpy(&uc_serial_rsp_buf[us_serial_response_len], pNsdu, u16NsduLength);
	us_serial_response_len += u16NsduLength;
	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_COORD_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerCoordInitialize(const uint8_t *puc_msg_content)
{
	enum ESerialStatus status = SERIAL_STATUS_INVALID_PARAMETER;

	PRINTF(PRINT_INFO, "_triggerCoordInitialize.\r\n");

	uint8_t m_u8Band = *puc_msg_content;
	struct TAdpNotifications *ps_serial_if_adp_notifications;
	struct TAdpNotifications notifications;
	TBootstrapConfiguration s_bs_conf;
	struct TAdpGetConfirm getConfirm;

	ps_serial_if_adp_notifications = serial_if_adp_get_notifications();

	notifications.fnctAdpDataConfirm = ps_serial_if_adp_notifications->fnctAdpDataConfirm;
	notifications.fnctAdpDataIndication = ps_serial_if_adp_notifications->fnctAdpDataIndication;
	notifications.fnctAdpDiscoveryConfirm = ps_serial_if_adp_notifications->fnctAdpDiscoveryConfirm;
	notifications.fnctAdpDiscoveryIndication = ps_serial_if_adp_notifications->fnctAdpDiscoveryIndication;
	notifications.fnctAdpNetworkStartConfirm = ps_serial_if_adp_notifications->fnctAdpNetworkStartConfirm;
	notifications.fnctAdpNetworkJoinConfirm = ps_serial_if_adp_notifications->fnctAdpNetworkJoinConfirm;
	notifications.fnctAdpNetworkLeaveIndication = ps_serial_if_adp_notifications->fnctAdpNetworkLeaveIndication;
	notifications.fnctAdpNetworkLeaveConfirm = ps_serial_if_adp_notifications->fnctAdpNetworkLeaveConfirm;
	notifications.fnctAdpResetConfirm = ps_serial_if_adp_notifications->fnctAdpResetConfirm;
	notifications.fnctAdpSetConfirm = ps_serial_if_adp_notifications->fnctAdpSetConfirm;
	notifications.fnctAdpGetConfirm = ps_serial_if_adp_notifications->fnctAdpGetConfirm;
	notifications.fnctAdpMacSetConfirm = ps_serial_if_adp_notifications->fnctAdpMacSetConfirm;
	notifications.fnctAdpMacGetConfirm = ps_serial_if_adp_notifications->fnctAdpMacGetConfirm;
	notifications.fnctAdpRouteDiscoveryConfirm = ps_serial_if_adp_notifications->fnctAdpRouteDiscoveryConfirm;
	notifications.fnctAdpPathDiscoveryConfirm = ps_serial_if_adp_notifications->fnctAdpPathDiscoveryConfirm;
	notifications.fnctAdpNetworkStatusIndication = ps_serial_if_adp_notifications->fnctAdpNetworkStatusIndication;
	notifications.fnctAdpBufferIndication = ps_serial_if_adp_notifications->fnctAdpBufferIndication;
	notifications.fnctAdpPREQIndication = ps_serial_if_adp_notifications->fnctAdpPREQIndication;
	notifications.fnctAdpUpdNonVolatileDataIndication = ps_serial_if_adp_notifications->fnctAdpUpdNonVolatileDataIndication;
	notifications.fnctAdpRouteNotFoundIndication = ps_serial_if_adp_notifications->fnctAdpRouteNotFoundIndication;
	notifications.fnctAdpLbpConfirm = ps_serial_if_adp_notifications->fnctAdpLbpConfirm;
	notifications.fnctAdpLbpIndication = ps_serial_if_adp_notifications->fnctAdpLbpIndication;

	AdpInitialize(&notifications, (enum TAdpBand)m_u8Band);
	AdpGetRequestSync(ADP_IB_MAX_HOPS, 0, &getConfirm);
	s_bs_conf.m_u8BandInfo = m_u8Band;
	s_bs_conf.m_u8MaxHop = getConfirm.m_au8AttributeValue[0];

	bs_init(s_bs_conf);
	bs_lbp_join_ind_set_cb(BootstrapNotification_JoinIndication);
	bs_lbp_leave_ind_set_cb(BootstrapNotification_LeaveIndication);

	adp_mac_serial_if_set_state(SERIAL_MODE_COORD);

	status = SERIAL_STATUS_SUCCESS;

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void _send_set_confirm(struct t_bs_lbp_set_param_confirm *p_set_confirm)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "_send_set_confirm:  attribute id = 0x%02x; index = %u; result = %u\r\n", p_set_confirm->ul_attribute_id, p_set_confirm->us_attribute_idx, p_set_confirm->uc_status);

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_COORD_SET_CONFIRM;
	/* ------------------------------------------------------------------------------------------ */
	uc_serial_rsp_buf[us_serial_response_len++] = p_set_confirm->uc_status;
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)((p_set_confirm->ul_attribute_id >> 24) & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)((p_set_confirm->ul_attribute_id >> 16) & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)((p_set_confirm->ul_attribute_id >> 8) & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(p_set_confirm->ul_attribute_id & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(p_set_confirm->us_attribute_idx >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(p_set_confirm->us_attribute_idx & 0xFF);
	/* -------------------------------------------------------------------------------------------- */
	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_COORD_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void _send_get_confirm(struct t_bs_lbp_get_param_confirm *p_get_confirm)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "_send_get_confirm.\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_COORD_GET_CONFIRM;
	/* ------------------------------------------------------------------------------------------ */
	uc_serial_rsp_buf[us_serial_response_len++] = p_get_confirm->uc_status;
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)((p_get_confirm->ul_attribute_id >> 24) & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)((p_get_confirm->ul_attribute_id >> 16) & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)((p_get_confirm->ul_attribute_id >> 8) & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(p_get_confirm->ul_attribute_id & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(p_get_confirm->us_attribute_idx >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(p_get_confirm->us_attribute_idx & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(p_get_confirm->uc_attribute_length);
	memcpy(&uc_serial_rsp_buf[us_serial_response_len], &p_get_confirm->uc_attribute_value[0], p_get_confirm->uc_attribute_length);
	us_serial_response_len += p_get_confirm->uc_attribute_length;
	/* -------------------------------------------------------------------------------------------- */
	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_COORD_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerCoordSetRequest(const uint8_t *puc_msg_content)
{
	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;
	uint32_t u32AttributeId;
	uint16_t u16AttributeIndex;
	uint8_t *puc_buffer = (uint8_t *)puc_msg_content;
	uint8_t u8AttributeLength = 0;

	struct t_bs_lbp_set_param_confirm set_confirm;

	PRINTF(PRINT_INFO, "_triggerCoordSetRequest.\r\n");

	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		u32AttributeId = ((uint32_t)*puc_buffer++) << 24;
		u32AttributeId += ((uint32_t)*puc_buffer++) << 16;
		u32AttributeId += ((uint32_t)*puc_buffer++) << 8;
		u32AttributeId += (uint32_t)*puc_buffer++;

		u16AttributeIndex = ((uint16_t)*puc_buffer++) << 8;
		u16AttributeIndex += (uint16_t)*puc_buffer++;

		u8AttributeLength = *puc_buffer++;

		bs_lbp_set_param(u32AttributeId, u16AttributeIndex, u8AttributeLength, puc_buffer, &set_confirm);
		_send_set_confirm(&set_confirm);
		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerCoordGetRequest(const uint8_t *puc_msg_content)
{
	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;
	uint32_t u32AttributeId;
	uint16_t u16AttributeIndex;
	uint8_t *puc_buffer = (uint8_t *)puc_msg_content;

	struct t_bs_lbp_get_param_confirm get_confirm;

	PRINTF(PRINT_INFO, "_triggerCoordGetRequest.\r\n");

	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		u32AttributeId = ((uint32_t)*puc_buffer++) << 24;
		u32AttributeId += ((uint32_t)*puc_buffer++) << 16;
		u32AttributeId += ((uint32_t)*puc_buffer++) << 8;
		u32AttributeId += (uint32_t)*puc_buffer++;

		u16AttributeIndex = ((uint16_t)*puc_buffer++) << 8;
		u16AttributeIndex += (uint16_t)*puc_buffer;

		bs_lbp_get_param(u32AttributeId, u16AttributeIndex, &get_confirm);
		_send_get_confirm(&get_confirm);
		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerCoordKickRequest(const uint8_t *puc_msg_content)
{
	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;

	uint16_t us_short_addr;
	uint8_t *puc_buffer = (uint8_t *)puc_msg_content;

	PRINTF(PRINT_INFO, "_triggerCoordKickRequest.\r\n");

	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		us_short_addr = ((uint16_t)*puc_buffer++) << 8;
		us_short_addr += (uint16_t)*puc_buffer;

		bs_lbp_kick_device(us_short_addr);

		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerCoordRekeyringRequest(const uint8_t *puc_msg_content)
{
	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;

	PRINTF(PRINT_INFO, "_triggerCoordRekeyringRequest.\r\n");
	/*	UNUSED(puc_msg_content); */
	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		bs_lbp_launch_rekeying();
		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

uint8_t serial_if_coordinator_api_parser(uint8_t *puc_rx_msg, uint16_t us_len)
{
	uint8_t uc_serial_if_cmd;
	uint8_t *puc_rx;
	enum ESerialStatus status = SERIAL_STATUS_UNKNOWN_COMMAND;

	/* Protection for invalid us_length */
	if (!us_len) {
		PRINTF(PRINT_ERROR, "serial_if_coordinator_api_parser ERROR: Invalid length.\r\n");
		return(false);
	}

	/* Process received message */
	uc_serial_if_cmd = puc_rx_msg[0] & 0x3F;
	PRINTF(PRINT_INFO, "serial_if_coordinator_api_parser. command = %u\r\n", uc_serial_if_cmd);
	puc_rx = &puc_rx_msg[1];

	switch (uc_serial_if_cmd) {
	case SERIAL_MSG_COORD_INITIALIZE:
		status = _triggerCoordInitialize(puc_rx);
		break;

	case SERIAL_MSG_COORD_SET_REQUEST:
		_triggerCoordSetRequest(puc_rx);
		break;

	case SERIAL_MSG_COORD_GET_REQUEST:
		_triggerCoordGetRequest(puc_rx);
		break;

	case SERIAL_MSG_COORD_KICK_REQUEST:
		_triggerCoordKickRequest(puc_rx);
		break;

	case SERIAL_MSG_COORD_REKEYING_REQUEST:
		_triggerCoordRekeyringRequest(puc_rx);
		break;

	default:
		break;
	}

	/* initialize doesn't have Confirm so send Status */
	/* other messages all have send / confirm send status only if there is a processing error */
	if ((status != SERIAL_STATUS_UNKNOWN_COMMAND) && ((status != SERIAL_STATUS_SUCCESS) || (uc_serial_if_cmd == SERIAL_MSG_COORD_INITIALIZE))) {
		MsgStatus(status, uc_serial_if_cmd);
		status = SERIAL_STATUS_SUCCESS;
	}

	/* if (status == SERIAL_STATUS_SUCCESS) return(true); */
	return(true);
}

#endif

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
