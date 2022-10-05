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
#include <stdio.h>
#include <stdbool.h>
/* #include <compiler.h> */
#include "string.h"

#include "conf_global.h"
/* Serial interface */
#include "serial_if_adp.h"
#include "serial_if_common.h"
#include "serial_if_mib_common.h"
#include "AdpApi.h"
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

/* Known message ids */
enum ESerialMessageId {
	/* Generic messages */
	SERIAL_MSG_STATUS = 0,

	/* ADP ACCESS */
	SERIAL_MSG_ADP_REQUEST_MESSAGES_BEGIN = 10,
	SERIAL_MSG_ADP_INITIALIZE = SERIAL_MSG_ADP_REQUEST_MESSAGES_BEGIN,
	SERIAL_MSG_ADP_DATA_REQUEST,
	SERIAL_MSG_ADP_DISCOVERY_REQUEST,
	SERIAL_MSG_ADP_NETWORK_START_REQUEST,
	SERIAL_MSG_ADP_NETWORK_JOIN_REQUEST,
	SERIAL_MSG_ADP_NETWORK_LEAVE_REQUEST,
	SERIAL_MSG_ADP_RESET_REQUEST,
	SERIAL_MSG_ADP_SET_REQUEST,
	SERIAL_MSG_ADP_GET_REQUEST,
	SERIAL_MSG_ADP_LBP_REQUEST,
	SERIAL_MSG_ADP_ROUTE_DISCOVERY_REQUEST,
	SERIAL_MSG_ADP_PATH_DISCOVERY_REQUEST,
	SERIAL_MSG_ADP_MAC_SET_REQUEST,
	SERIAL_MSG_ADP_MAC_GET_REQUEST,
	SERIAL_MSG_ADP_REQUEST_MESSAGES_END = SERIAL_MSG_ADP_MAC_GET_REQUEST,

	SERIAL_MSG_ADP_DATA_CONFIRM = 30,
	SERIAL_MSG_ADP_DATA_INDICATION,
	SERIAL_MSG_ADP_NETWORK_STATUS_INDICATION,
	SERIAL_MSG_ADP_DISCOVERY_CONFIRM,
	SERIAL_MSG_ADP_NETWORK_START_CONFIRM,
	SERIAL_MSG_ADP_NETWORK_JOIN_CONFIRM,
	SERIAL_MSG_ADP_NETWORK_LEAVE_CONFIRM,
	SERIAL_MSG_ADP_NETWORK_LEAVE_INDICATION,
	SERIAL_MSG_ADP_RESET_CONFIRM,
	SERIAL_MSG_ADP_SET_CONFIRM,
	SERIAL_MSG_ADP_GET_CONFIRM,
	SERIAL_MSG_ADP_LBP_CONFIRM,
	SERIAL_MSG_ADP_LBP_INDICATION,
	SERIAL_MSG_ADP_ROUTE_DISCOVERY_CONFIRM,
	SERIAL_MSG_ADP_PATH_DISCOVERY_CONFIRM,
	SERIAL_MSG_ADP_MAC_SET_CONFIRM,
	SERIAL_MSG_ADP_MAC_GET_CONFIRM,
	SERIAL_MSG_ADP_BUFFER_INDICATION,
	SERIAL_MSG_ADP_DISCOVERY_INDICATION,
	SERIAL_MSG_ADP_PREQ_INDICATION,
	SERIAL_MSG_ADP_UPD_NON_VOLATILE_DATA_INDICATION,
	SERIAL_MSG_ADP_ROUTE_NOT_FOUND_INDICATION,
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
static uint8_t auc_aux_endiannes_buf[256]; /* !<  Set Request Endianness tranformation buffer */

static struct TMacWrpSetRequest set_params;
/* static struct TMacWrpGetRequest get_params; */

static struct TAdpNotifications ss_notifications;

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void MsgStatus(enum ESerialStatus status, uint8_t uc_serial_if_cmd)
{
	switch (status) {
	case SERIAL_STATUS_SUCCESS:
		PRINTF(PRINT_INFO, "MsgStatus (status = SERIAL_STATUS_SUCCESS)\r\n");
		break;

	case SERIAL_STATUS_NOT_ALLOWED:
		PRINTF(PRINT_INFO, "MsgStatus (status = SERIAL_STATUS_NOT_ALLOWED)\r\n");
		break;

	case SERIAL_STATUS_UNKNOWN_COMMAND:
		PRINTF(PRINT_INFO, "MsgStatus (status = SERIAL_STATUS_UNKNOWN_COMMAND)\r\n");
		break;

	case SERIAL_STATUS_INVALID_PARAMETER:
		PRINTF(PRINT_INFO, "MsgStatus (status = SERIAL_STATUS_INVALID_PARAMETER)\r\n");
		break;

	default:
		PRINTF(PRINT_INFO, "MsgStatus (status WRONG)\r\n");
		break;
	}

	uint8_t us_serial_response_len;

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_STATUS;
	uc_serial_rsp_buf[us_serial_response_len++] = status;
	uc_serial_rsp_buf[us_serial_response_len++] = uc_serial_if_cmd;
	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_BufferIndication(struct TAdpBufferIndication *pBufferIndication)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_BufferIndication\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_BUFFER_INDICATION;
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pBufferIndication->m_bBufferReady);
	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AppAdpNotification_PREQIndication(void)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_PREQIndication\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_PREQ_INDICATION;
	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AppAdpNotification_UpdNonVolatileDataIndication(void)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_UpdNonVolatileDataIndication\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_UPD_NON_VOLATILE_DATA_INDICATION;
	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AppAdpNotification_RouteNotFoundIndication(struct TAdpRouteNotFoundIndication *pRouteNotFoundIndication)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_RouteNotFoundIndication\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_ROUTE_NOT_FOUND_INDICATION;
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_u16SrcAddr >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_u16SrcAddr & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_u16DestAddr >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_u16DestAddr & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_u16NextHopAddr >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_u16NextHopAddr & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_u16PreviousHopAddr >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_u16PreviousHopAddr & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_u16RouteCost >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_u16RouteCost & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_u8HopCount);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_u8WeakLinkCount);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_bRouteJustBroken);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_bCompressedHeader);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_u16NsduLength >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pRouteNotFoundIndication->m_u16NsduLength & 0xFF);
	memcpy(&uc_serial_rsp_buf[us_serial_response_len], &pRouteNotFoundIndication->m_pNsdu, pRouteNotFoundIndication->m_u16NsduLength);
	us_serial_response_len += pRouteNotFoundIndication->m_u16NsduLength;

	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_DataConfirm(struct TAdpDataConfirm *pDataConfirm)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_DataConfirm (status = %u)\r\n", pDataConfirm->m_u8Status);

	/* Manage Result */
	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_DATA_CONFIRM;
	uc_serial_rsp_buf[us_serial_response_len++] = pDataConfirm->m_u8Status;
	uc_serial_rsp_buf[us_serial_response_len++] = pDataConfirm->m_u8NsduHandle;
	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;

	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_AdpdDataIndication( struct TAdpDataIndication *pDataIndication)
{
	uint16_t us_serial_response_len;
	/* uint8_t i; */

	PRINTF(PRINT_INFO, "AdpNotification_AdpDataIndication\r\n");

	PRINTF(PRINT_INFO, "length = %u ==> ", pDataIndication->m_u16NsduLength);
	/*	for(i = 0; i < pDataIndication->m_u16NsduLength; i++){ */
	/*		PRINTF(PRINT_INFO, "%02X:", pDataIndication->m_pNsdu[i]); */
	/*	} */
	/*	PRINTF(PRINT_INFO, "\r\n"); */

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_DATA_INDICATION;
	uc_serial_rsp_buf[us_serial_response_len++] = pDataIndication->m_u8LinkQualityIndicator;
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pDataIndication->m_u16NsduLength >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pDataIndication->m_u16NsduLength & 0xFF);
	memcpy(&uc_serial_rsp_buf[us_serial_response_len], pDataIndication->m_pNsdu, pDataIndication->m_u16NsduLength);
	us_serial_response_len += pDataIndication->m_u16NsduLength;

	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_NetworkStatusIndication( struct TAdpNetworkStatusIndication *pNetworkStatusIndication)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_NetworkStatusIndication\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_NETWORK_STATUS_INDICATION;

	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pNetworkStatusIndication->m_u16PanId >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pNetworkStatusIndication->m_u16PanId & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = pNetworkStatusIndication->m_SrcDeviceAddress.m_u8AddrSize;
	if (pNetworkStatusIndication->m_SrcDeviceAddress.m_u8AddrSize == ADP_ADDRESS_16BITS) {
		uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pNetworkStatusIndication->m_SrcDeviceAddress.m_u16ShortAddr >> 8);
		uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pNetworkStatusIndication->m_SrcDeviceAddress.m_u16ShortAddr & 0xFF);
	} else { /* ADP_ADDRESS_64BITS */
		memcpy(&uc_serial_rsp_buf[us_serial_response_len], (uint8_t *)&pNetworkStatusIndication->m_SrcDeviceAddress.m_ExtendedAddress,
				pNetworkStatusIndication->m_SrcDeviceAddress.m_u8AddrSize);
		us_serial_response_len += pNetworkStatusIndication->m_SrcDeviceAddress.m_u8AddrSize;
	}

	uc_serial_rsp_buf[us_serial_response_len++] = pNetworkStatusIndication->m_DstDeviceAddress.m_u8AddrSize;
	if (pNetworkStatusIndication->m_DstDeviceAddress.m_u8AddrSize == ADP_ADDRESS_16BITS) {
		uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pNetworkStatusIndication->m_DstDeviceAddress.m_u16ShortAddr >> 8);
		uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pNetworkStatusIndication->m_DstDeviceAddress.m_u16ShortAddr & 0xFF);
	} else { /* ADP_ADDRESS_64BITS */
		memcpy(&uc_serial_rsp_buf[us_serial_response_len], (uint8_t *)&pNetworkStatusIndication->m_DstDeviceAddress.m_ExtendedAddress,
				pNetworkStatusIndication->m_DstDeviceAddress.m_u8AddrSize);
		us_serial_response_len += pNetworkStatusIndication->m_DstDeviceAddress.m_u8AddrSize;
	}

	uc_serial_rsp_buf[us_serial_response_len++] = pNetworkStatusIndication->m_u8Status;
	uc_serial_rsp_buf[us_serial_response_len++] = pNetworkStatusIndication->m_u8SecurityLevel;
	uc_serial_rsp_buf[us_serial_response_len++] = pNetworkStatusIndication->m_u8KeyIndex;
#ifdef G3_HYBRID_PROFILE
	uc_serial_rsp_buf[us_serial_response_len++] = pNetworkStatusIndication->m_u8MediaType;
#endif

	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_DiscoveryConfirm(uint8_t u8Status)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_DiscoveryConfirm\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_DISCOVERY_CONFIRM;
	uc_serial_rsp_buf[us_serial_response_len++] = u8Status;
	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_DiscoveryIndication(struct TAdpPanDescriptor *pPanDescriptor)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_DiscoveryIndication\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_DISCOVERY_INDICATION;

	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPanDescriptor->m_u16PanId >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPanDescriptor->m_u16PanId & 0xFF);

	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPanDescriptor->m_u8LinkQuality);

	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPanDescriptor->m_u16LbaAddress >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPanDescriptor->m_u16LbaAddress & 0xFF);

	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPanDescriptor->m_u16RcCoord >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPanDescriptor->m_u16RcCoord & 0xFF);
	
#ifdef G3_HYBRID_PROFILE
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPanDescriptor->m_u8MediaType);
#endif

	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_NetworkStartConfirm( struct TAdpNetworkStartConfirm *pNetworkStartConfirm)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_NetworkStartConfirm\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_NETWORK_START_CONFIRM;
	uc_serial_rsp_buf[us_serial_response_len++] = pNetworkStartConfirm->m_u8Status;

	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_NetworkJoinConfirm( struct TAdpNetworkJoinConfirm *pNetworkJoinConfirm)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_NetworkJoinConfirm - Status = %u; PanID = %u; Addr = %u\r\n", pNetworkJoinConfirm->m_u8Status, pNetworkJoinConfirm->m_u16PanId, pNetworkJoinConfirm->m_u16NetworkAddress);

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_NETWORK_JOIN_CONFIRM;
	uc_serial_rsp_buf[us_serial_response_len++] = pNetworkJoinConfirm->m_u8Status;
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pNetworkJoinConfirm->m_u16NetworkAddress >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pNetworkJoinConfirm->m_u16NetworkAddress & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pNetworkJoinConfirm->m_u16PanId >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pNetworkJoinConfirm->m_u16PanId & 0xFF);

	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_NetworkLeaveIndication(void)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_NetworkLeaveIndication\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_NETWORK_LEAVE_INDICATION;

	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_NetworkLeaveConfirm( struct TAdpNetworkLeaveConfirm *pLeaveConfirm)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_NetworkLeaveConfirm\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_NETWORK_LEAVE_CONFIRM;
	uc_serial_rsp_buf[us_serial_response_len++] = pLeaveConfirm->m_u8Status;

	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_ResetConfirm(struct TAdpResetConfirm *pResetConfirm)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_ResetConfirm\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_RESET_CONFIRM;
	uc_serial_rsp_buf[us_serial_response_len++] = pResetConfirm->m_u8Status;

	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_SetConfirm(struct TAdpSetConfirm *pSetConfirm)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_SetConfirm status=%u\r\n", pSetConfirm->m_u8Status);

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_SET_CONFIRM;
	uc_serial_rsp_buf[us_serial_response_len++] = pSetConfirm->m_u8Status;
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)((pSetConfirm->m_u32AttributeId >> 24) & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)((pSetConfirm->m_u32AttributeId >> 16) & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)((pSetConfirm->m_u32AttributeId >> 8) & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pSetConfirm->m_u32AttributeId & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pSetConfirm->m_u16AttributeIndex >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pSetConfirm->m_u16AttributeIndex & 0xFF);
	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_GetConfirm(struct TAdpGetConfirm *pGetConfirm)
{
	uint8_t us_serial_response_len;

	uint8_t u8PrefixLength_bytes;
	uint8_t u8ContextLength;

	PRINTF(PRINT_INFO, "AdpNotification_GetConfirm\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_GET_CONFIRM;
	uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_u8Status;
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)((pGetConfirm->m_u32AttributeId >> 24) & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)((pGetConfirm->m_u32AttributeId >> 16) & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)((pGetConfirm->m_u32AttributeId >> 8) & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pGetConfirm->m_u32AttributeId & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pGetConfirm->m_u16AttributeIndex >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pGetConfirm->m_u16AttributeIndex & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_u8AttributeLength;

	if (pGetConfirm->m_u8Status == G3_SUCCESS) {
		switch (pGetConfirm->m_u32AttributeId) {
		case ADP_IB_SECURITY_LEVEL:
			PRINTF(PRINT_INFO, "-- ADP_IB_SECURITY_LEVEL\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpSecurityLevel */
			break;

		case ADP_IB_PREFIX_TABLE:
			PRINTF(PRINT_INFO, "-- ADP_IB_PREFIX_TABLE\r\n");
			u8PrefixLength_bytes = pGetConfirm->m_u8AttributeLength - 11;
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8PrefixLength */
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[1]; /* m_bOnLinkFlag */
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[2]; /* m_bAutonomousAddressConfigurationFlag */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[3]);  /* u32ValidTime */
			us_serial_response_len += 4;
			mem_copy_to_usi_endianness_uint32((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[7]);  /* u32PreferredTime */
			us_serial_response_len += 4;
			memcpy((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len], (uint8_t *)&pGetConfirm->m_au8AttributeValue[7], u8PrefixLength_bytes);  /* m_au8Prefix */
			us_serial_response_len += u8PrefixLength_bytes;
			break;

		case ADP_IB_BROADCAST_LOG_TABLE_ENTRY_TTL:
			PRINTF(PRINT_INFO, "-- ADP_IB_BROADCAST_LOG_TABLE_ENTRY_TTL\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16AdpBroadcastLogTableEntryTTL */
			us_serial_response_len += 2;
			break;

		case ADP_IB_METRIC_TYPE:
			PRINTF(PRINT_INFO, "-- ADP_IB_METRIC_TYPE\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpMetricType */
			break;

		case ADP_IB_LOW_LQI_VALUE:
			PRINTF(PRINT_INFO, "-- ADP_IB_LOW_LQI_VALUE\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpLowLQIValue */
			break;

		case ADP_IB_HIGH_LQI_VALUE:
			PRINTF(PRINT_INFO, "-- ADP_IB_HIGH_LQI_VALUE\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpHighLQIValue */
			break;

		case ADP_IB_RREP_WAIT:
			PRINTF(PRINT_INFO, "-- ADP_IB_RREP_WAIT\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpRREPWait */
			break;

		case ADP_IB_CONTEXT_INFORMATION_TABLE:
			PRINTF(PRINT_INFO, "-- ADP_IB_CONTEXT_INFORMATION_TABLE\r\n");
			u8ContextLength = pGetConfirm->m_u8AttributeLength - 4;
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* u16ValidTime */
			us_serial_response_len += 2;
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[2]; /* m_bValidForCompression */
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[3]; /* m_u8BitsContextLength */
			memcpy((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len], (uint8_t *)&pGetConfirm->m_au8AttributeValue[4], u8ContextLength);  /* m_au8Context */
			us_serial_response_len += u8ContextLength;
			break;

		case ADP_IB_COORD_SHORT_ADDRESS:
			PRINTF(PRINT_INFO, "-- ADP_IB_COORD_SHORT_ADDRESS\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16AdpCoordShortAddress */
			us_serial_response_len += 2;
			break;

		case ADP_IB_RLC_ENABLED:
			PRINTF(PRINT_INFO, "-- ADP_IB_RLC_ENABLED\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_bAdpRLCEnabled */
			break;

		case ADP_IB_ADD_REV_LINK_COST:
			PRINTF(PRINT_INFO, "-- ADP_IB_ADD_REV_LINK_COST\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpAddRevLinkCost */
			break;

		case ADP_IB_BROADCAST_LOG_TABLE:
			PRINTF(PRINT_INFO, "-- ADP_IB_BROADCAST_LOG_TABLE\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16SrcAddr */
			us_serial_response_len += 2;
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[2]; /* m_u8SequenceNumber */
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[3]);  /* u16ValidTime */
			us_serial_response_len += 2;
			break;

		case ADP_IB_ROUTING_TABLE:
			PRINTF(PRINT_INFO, "-- ADP_IB_ROUTING_TABLE\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16DstAddr */
			us_serial_response_len += 2;
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[2]);  /* m_u16NextHopAddr */
			us_serial_response_len += 2;
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[4]);  /* m_u16RouteCost */
			us_serial_response_len += 2;
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[6]; /* m_u8HopCount || m_u8WeakLinkCount */
#ifdef G3_HYBRID_PROFILE
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[7]; /* m_u8MediaType */
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[8]);  /* u16ValidTime */
			us_serial_response_len += 2;
#else
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[7]);  /* u16ValidTime */
			us_serial_response_len += 2;
#endif
			break;

		case ADP_IB_UNICAST_RREQ_GEN_ENABLE:
			PRINTF(PRINT_INFO, "-- ADP_IB_UNICAST_RREQ_GEN_ENABLE\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_bAdpUnicastRREQGenEnable */
			break;

		case ADP_IB_GROUP_TABLE:
			PRINTF(PRINT_INFO, "-- ADP_IB_GROUP_TABLE\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16GroupAddress */
			us_serial_response_len += 2;
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[2]; /* m_bValid */
			break;

		case ADP_IB_MAX_HOPS:
			PRINTF(PRINT_INFO, "-- ADP_IB_MAX_HOPS\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpMaxHops */
			break;

		case ADP_IB_DEVICE_TYPE:
			PRINTF(PRINT_INFO, "-- ADP_IB_DEVICE_TYPE\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_eAdpDeviceType */
			break;

		case ADP_IB_NET_TRAVERSAL_TIME:
			PRINTF(PRINT_INFO, "-- ADP_IB_NET_TRAVERSAL_TIME\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpNetTraversalTime */
			break;

		case ADP_IB_ROUTING_TABLE_ENTRY_TTL:
			PRINTF(PRINT_INFO, "-- ADP_IB_ROUTING_TABLE_ENTRY_TTL\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16AdpRoutingTableEntryTTL */
			us_serial_response_len += 2;
			break;

		case ADP_IB_KR:
			PRINTF(PRINT_INFO, "-- ADP_IB_KR\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpKr */
			break;

		case ADP_IB_KM:
			PRINTF(PRINT_INFO, "-- ADP_IB_KM\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpKm */
			break;

		case ADP_IB_KC:
			PRINTF(PRINT_INFO, "-- ADP_IB_KC\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpKc */
			break;

		case ADP_IB_KQ:
			PRINTF(PRINT_INFO, "-- ADP_IB_KQ\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpKq */
			break;

		case ADP_IB_KH:
			PRINTF(PRINT_INFO, "-- ADP_IB_KH\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpKh */
			break;

		case ADP_IB_RREQ_RETRIES:
			PRINTF(PRINT_INFO, "-- ADP_IB_RREQ_RETRIES\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpRREQRetries */
			break;

		case ADP_IB_RREQ_RERR_WAIT: /* ADP_IB_RREQ_WAIT also enters here as it has the same numeric value */
			PRINTF(PRINT_INFO, "-- ADP_IB_RREQ_RERR_WAIT\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpRREQRERRWait */
			break;

		case ADP_IB_WEAK_LQI_VALUE:
			PRINTF(PRINT_INFO, "-- ADP_IB_WEAK_LQI_VALUE\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpWeakLQIValue */
			break;

		case ADP_IB_KRT:
			PRINTF(PRINT_INFO, "-- ADP_IB_KRT\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpKrt */
			break;

		case ADP_IB_SOFT_VERSION:
			PRINTF(PRINT_INFO, "-- ADP_IB_SOFT_VERSION\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8Major */
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[1]; /* m_u8Minor */
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[2]; /* m_u8Revision */
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[3]; /* m_u8Year */
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[4]; /* m_u8Month */
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[5]; /* m_u8Day */
			break;

		case ADP_IB_SNIFFER_MODE:
			PRINTF(PRINT_INFO, "-- ADP_IB_SNIFFER_MODE\r\n");
			/* TODO */
			break;

		case ADP_IB_BLACKLIST_TABLE:
			PRINTF(PRINT_INFO, "-- ADP_IB_BLACKLIST_TABLE\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16Addr */
			us_serial_response_len += 2;
#ifdef G3_HYBRID_PROFILE
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[2]; /* m_u8MediaType */
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[3]);  /* u16ValidTime */
			us_serial_response_len += 2;
#else
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[2]);  /* u16ValidTime */
			us_serial_response_len += 2;
#endif
			break;

		case ADP_IB_BLACKLIST_TABLE_ENTRY_TTL:
			PRINTF(PRINT_INFO, "-- ADP_IB_BLACKLIST_TABLE_ENTRY_TTL\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16AdpBlacklistTableEntryTTL */
			us_serial_response_len += 2;
			break;

		case ADP_IB_MAX_JOIN_WAIT_TIME:
			PRINTF(PRINT_INFO, "-- ADP_IB_MAX_JOIN_WAIT_TIME\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16AdpMaxJoinWaitTime */
			us_serial_response_len += 2;
			break;

		case ADP_IB_PATH_DISCOVERY_TIME:
			PRINTF(PRINT_INFO, "-- ADP_IB_PATH_DISCOVERY_TIME\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpPathDiscoveryTime */
			break;

		case ADP_IB_ACTIVE_KEY_INDEX:
			PRINTF(PRINT_INFO, "-- ADP_IB_ACTIVE_KEY_INDEX\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpActiveKeyIndex */
			break;

		case ADP_IB_DESTINATION_ADDRESS_SET:
			PRINTF(PRINT_INFO, "-- ADP_IB_DESTINATION_ADDRESS_SET\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);                                                                           /* m_u16Addr */
			us_serial_response_len += 2;
			break;

		case ADP_IB_DEFAULT_COORD_ROUTE_ENABLED:
			PRINTF(PRINT_INFO, "-- ADP_IB_DEFAULT_COORD_ROUTE_ENABLED\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_bAdpDefaultCoordRouteEnabled */
			break;

		case ADP_IB_DISABLE_DEFAULT_ROUTING:
			PRINTF(PRINT_INFO, "-- ADP_IB_DISABLE_DEFAULT_ROUTING\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_bAdpDisableDefaultRouting */
			break;

		/* manufacturer */
		case ADP_IB_MANUF_REASSEMBY_TIMER:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_REASSEMBY_TIMER\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16AdpReassembyTimer */
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_IPV6_HEADER_COMPRESSION:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_IPV6_HEADER_COMPRESSION\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_bAdpIPv6HeaderCompression */
			break;

		/*	case ADP_IB_MANUF_EAP_PRESHARED_KEY: */
		/*		//Write Only */
		/*		break; */
		case ADP_IB_MANUF_EAP_NETWORK_ACCESS_IDENTIFIER:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_EAP_NETWORK_ACCESS_IDENTIFIER\r\n");
			memcpy((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)pGetConfirm->m_au8AttributeValue, pGetConfirm->m_u8AttributeLength);  /* m_au8Value */
			us_serial_response_len += pGetConfirm->m_u8AttributeLength;
			break;

		case ADP_IB_MANUF_BROADCAST_SEQUENCE_NUMBER:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_BROADCAST_SEQUENCE_NUMBER\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8BroadcastSequenceNumber */
			break;

		/*	case ADP_IB_MANUF_REGISTER_DEVICE : */
		/*		break; */
		case ADP_IB_MANUF_DATAGRAM_TAG:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_DATAGRAM_TAG\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16DatagramTag */
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_RANDP:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_RANDP\r\n");
			memcpy((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0], pGetConfirm->m_u8AttributeLength);  /* m_au8Value */
			us_serial_response_len += pGetConfirm->m_u8AttributeLength;
			break;

		case ADP_IB_MANUF_ROUTING_TABLE_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_ROUTING_TABLE_COUNT\r\n");
			mem_copy_to_usi_endianness_uint32((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_au8RandP */
			us_serial_response_len += 4;
			break;

		/*	case ADP_IB_MANUF_DISCOVER_SEQUENCE_NUMBER : */
		/*		break; */
		case ADP_IB_MANUF_FORCED_NO_ACK_REQUEST:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_FORCED_NO_ACK_REQUEST\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8ForceNoAck */
			break;

		case ADP_IB_MANUF_LQI_TO_COORD:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_LQI_TO_COORD\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8LQIToCoord */
			break;

		case ADP_IB_MANUF_BROADCAST_ROUTE_ALL:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_BROADCAST_ROUTE_ALL\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_bBroadcastRouteAll */
			break;

		case ADP_IB_MANUF_KEEP_PARAMS_AFTER_KICK_LEAVE:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_KEEP_PARAMS_AFTER_KICK_LEAVE\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_bKeepParamsAfterKickLeave */
			break;

		case ADP_IB_MANUF_ADP_INTERNAL_VERSION:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_ADP_MAC_INTERNAL_VERSION\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8Major */
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[1]; /* m_u8Minor */
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[2]; /* m_u8Revision */
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[3]; /* m_u8Year */
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[4]; /* m_u8Month */
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[5]; /* m_u8Day */
			break;

		case ADP_IB_MANUF_CIRCULAR_ROUTES_DETECTED:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_CIRCULAR_ROUTES_DETECTED\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16CircularRoutesDetected */
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_LAST_CIRCULAR_ROUTE_ADDRESS:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_LAST_CIRCULAR_ROUTE_ADDRESS\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16LastCircularRouteAddress */
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_IPV6_ULA_DEST_SHORT_ADDRESS:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_IPV6_ULA_DEST_SHORT_ADDRESS\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* g_AdpExecutionContext.m_AdpMib.m_u16ULADestShortAddress */
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_MAX_REPAIR_RESEND_ATTEMPTS:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_MAX_REPAIR_RESEND_ATTEMPTS\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8MaxRepairReSendAttemps */
			break;

		case ADP_IB_MANUF_DISABLE_AUTO_RREQ:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_DISABLE_AUTO_RREQ\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_bDisableAutoRREQ */
			break;

		case ADP_IB_MANUF_ALL_NEIGHBORS_BLACKLISTED_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_ALL_NEIGHBORS_BLACKLISTED_COUNT\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16AllNeighborBlacklistedCount */
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_QUEUED_ENTRIES_REMOVED_TIMEOUT_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_QUEUED_ENTRIES_REMOVED_TIMEOUT_COUNT\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16QueuedEntriesRemovedTimeoutCount */
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_QUEUED_ENTRIES_REMOVED_ROUTE_ERROR_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_QUEUED_ENTRIES_REMOVED_ROUTE_ERROR_COUNT\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16QueuedEntriesRemovedRouteErrorCount */
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_PENDING_DATA_IND_SHORT_ADDRESS:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_PENDING_DATA_IND_SHORT_ADDRESS\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_PendingDataIndication.m_DstDeviceAddress.m_u16ShortAddr */
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_GET_BAND_CONTEXT_TONES:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_GET_BAND_CONTEXT_TONES\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_BandContext.m_u8Tones */
			break;

		case ADP_IB_MANUF_UPDATE_NON_VOLATILE_DATA:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_UPDATE_NON_VOLATILE_DATA\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_bUpdNonVolatileData */
			break;

		case ADP_IB_MANUF_DISCOVER_ROUTE_GLOBAL_SEQ_NUM:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_DISCOVER_ROUTE_GLOBAL_SEQ_NUM\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16DiscoverRouteGlobalSeqNo */
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_FRAGMENT_DELAY:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_FRAGMENT_DELAY\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16AdpFragmentDelay */
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_DYNAMIC_FRAGMENT_DELAY_ENABLED:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_DYNAMIC_FRAGMENT_DELAY_ENABLED\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_bAdpDynamicFragmentDelayEnabled */
			break;

		case ADP_IB_MANUF_DYNAMIC_FRAGMENT_DELAY_FACTOR:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_DYNAMIC_FRAGMENT_DELAY_FACTOR\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16AdpFragmentDelay */
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_BLACKLIST_TABLE_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_BLACKLIST_TABLE_COUNT\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_BROADCAST_LOG_TABLE_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_BROADCAST_LOG_TABLE_COUNT\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_CONTEXT_INFORMATION_TABLE_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_CONTEXT_INFORMATION_TABLE_COUNT\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_GROUP_TABLE_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_GROUP_TABLE_COUNT\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);
			us_serial_response_len += 2;
			break;

		case ADP_IB_MANUF_ROUTING_TABLE_ELEMENT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_ROUTING_TABLE_ELEMENT\r\n");
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[0]);  /* m_u16DstAddr */
			us_serial_response_len += 2;
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[2]);  /* m_u16NextHopAddr */
			us_serial_response_len += 2;
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[4]);  /* m_u16RouteCost */
			us_serial_response_len += 2;
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[6]; /* m_u8HopCount || m_u8WeakLinkCount */
#ifdef G3_HYBRID_PROFILE
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[7]; /* m_u8MediaType */
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[8]);  /* u16ValidTime */
			us_serial_response_len += 2;
#else
			mem_copy_to_usi_endianness_uint16((uint8_t *)&uc_serial_rsp_buf[us_serial_response_len],
					(uint8_t *)&pGetConfirm->m_au8AttributeValue[7]);  /* u16ValidTime */
			us_serial_response_len += 2;
#endif
			break;

		case ADP_IB_MANUF_SET_PHASEDIFF_PREQ_PREP:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_SET_PHASEDIFF_PREQ_PREP\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_bSetPhaseDiffPREQPREP */
			break;

		case ADP_IB_MANUF_HYBRID_PROFILE:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_HYBRID_PROFILE\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_bAdpHybridProfile */
			break;
			
#ifdef G3_HYBRID_PROFILE
		case ADP_IB_LOW_LQI_VALUE_RF:
			PRINTF(PRINT_INFO, "-- ADP_IB_LOW_LQI_VALUE_RF\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpLowLQIValueRF */
			break;

		case ADP_IB_HIGH_LQI_VALUE_RF:
			PRINTF(PRINT_INFO, "-- ADP_IB_HIGH_LQI_VALUE_RF\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpHighLQIValueRF */
			break;

		case ADP_IB_KQ_RF:
			PRINTF(PRINT_INFO, "-- ADP_IB_KQ_RF\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpKqRF */
			break;

		case ADP_IB_KH_RF:
			PRINTF(PRINT_INFO, "-- ADP_IB_KH_RF\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpKhRF */
			break;

		case ADP_IB_KRT_RF:
			PRINTF(PRINT_INFO, "-- ADP_IB_KRT_RF\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpKrtRF */
			break;

		case ADP_IB_KDC_RF:
			PRINTF(PRINT_INFO, "-- ADP_IB_KDC_RF\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_u8AdpKdcRF */
			break;

		case ADP_IB_USE_BACKUP_MEDIA:
			PRINTF(PRINT_INFO, "-- ADP_IB_USE_BACKUP_MEDIA\r\n");
			uc_serial_rsp_buf[us_serial_response_len++] = pGetConfirm->m_au8AttributeValue[0]; /* m_bAdpUseBackupMedia */
			break;
#endif

		default:
			break;
		}
	}

	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_SetMacConfirm(struct TAdpMacSetConfirm *pSetMacConfirm)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_SetMacConfirm status: %u\r\n", pSetMacConfirm->m_u8Status);

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_MAC_SET_CONFIRM;
	/* ------------------------------------------------------------------------------------------ */
	uc_serial_rsp_buf[us_serial_response_len++] = pSetMacConfirm->m_u8Status;
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)((pSetMacConfirm->m_u32AttributeId >> 24) & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)((pSetMacConfirm->m_u32AttributeId >> 16) & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)((pSetMacConfirm->m_u32AttributeId >> 8) & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pSetMacConfirm->m_u32AttributeId & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pSetMacConfirm->m_u16AttributeIndex >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pSetMacConfirm->m_u16AttributeIndex & 0xFF);
	/* -------------------------------------------------------------------------------------------- */
	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_GetMacConfirm(struct TAdpMacGetConfirm *pGetMacConfirm)
{
	uint8_t uc_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_GetMacConfirm\r\n");

	uc_serial_response_len = 0;
	uc_serial_rsp_buf[uc_serial_response_len++] = SERIAL_MSG_ADP_MAC_GET_CONFIRM;
	struct TMacWrpGetConfirm sMlmeGetConfirm;
	sMlmeGetConfirm.m_eStatus = (enum EMacWrpStatus)(pGetMacConfirm->m_u8Status);
	sMlmeGetConfirm.m_ePibAttribute = (enum EMacWrpPibAttribute)(pGetMacConfirm->m_u32AttributeId);
	sMlmeGetConfirm.m_u16PibAttributeIndex = pGetMacConfirm->m_u16AttributeIndex;
	sMlmeGetConfirm.m_PibAttributeValue.m_u8Length = pGetMacConfirm->m_u8AttributeLength;
	memcpy(&sMlmeGetConfirm.m_PibAttributeValue.m_au8Value, &pGetMacConfirm->m_au8AttributeValue, sizeof(sMlmeGetConfirm.m_PibAttributeValue.m_au8Value));

	uc_serial_response_len += process_MIB_get_confirm(&uc_serial_rsp_buf[uc_serial_response_len], &sMlmeGetConfirm);

	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = uc_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_LbpConfirm(struct TAdpLbpConfirm *pLbpConfirm)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_LbpConfirm\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_LBP_CONFIRM;
	uc_serial_rsp_buf[us_serial_response_len++] = pLbpConfirm->m_u8Status;
	uc_serial_rsp_buf[us_serial_response_len++] = pLbpConfirm->m_u8NsduHandle;

	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_LbpIndication( struct TAdpLbpIndication *pLbpIndication)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_LbpIndication\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_LBP_INDICATION;
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pLbpIndication->m_u16SrcAddr >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pLbpIndication->m_u16SrcAddr & 0xFF);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pLbpIndication->m_u16NsduLength >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pLbpIndication->m_u16NsduLength & 0xFF);
	memcpy(&uc_serial_rsp_buf[us_serial_response_len], pLbpIndication->m_pNsdu, pLbpIndication->m_u16NsduLength);
	us_serial_response_len += pLbpIndication->m_u16NsduLength;
	uc_serial_rsp_buf[us_serial_response_len++] = pLbpIndication->m_u8LinkQualityIndicator;
	uc_serial_rsp_buf[us_serial_response_len++] = pLbpIndication->m_bSecurityEnabled;

	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_RouteDiscoveryConfirm( struct TAdpRouteDiscoveryConfirm *pRouteDiscoveryConfirm)
{
	uint8_t us_serial_response_len;

	PRINTF(PRINT_INFO, "AdpNotification_RouteDiscoveryConfirm\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_ROUTE_DISCOVERY_CONFIRM;
	uc_serial_rsp_buf[us_serial_response_len++] = pRouteDiscoveryConfirm->m_u8Status;

	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static void AdpNotification_PathDiscoveryConfirm(struct TAdpPathDiscoveryConfirm *pPathDiscoveryConfirm)
{
	uint8_t us_serial_response_len;
	uint8_t u8Index;

	PRINTF(PRINT_INFO, "AdpNotification_PathDiscoveryConfirm\r\n");

	us_serial_response_len = 0;
	uc_serial_rsp_buf[us_serial_response_len++] = SERIAL_MSG_ADP_PATH_DISCOVERY_CONFIRM;
	uc_serial_rsp_buf[us_serial_response_len++] = pPathDiscoveryConfirm->m_u8Status;
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPathDiscoveryConfirm->m_u16DstAddr >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPathDiscoveryConfirm->m_u16DstAddr);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPathDiscoveryConfirm->m_u16OrigAddr >> 8);
	uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPathDiscoveryConfirm->m_u16OrigAddr);
	uc_serial_rsp_buf[us_serial_response_len++] = pPathDiscoveryConfirm->m_u8MetricType;
	uc_serial_rsp_buf[us_serial_response_len++] = pPathDiscoveryConfirm->m_u8ForwardHopsCount;
	uc_serial_rsp_buf[us_serial_response_len++] = pPathDiscoveryConfirm->m_u8ReverseHopsCount;

	for (u8Index = 0; u8Index < pPathDiscoveryConfirm->m_u8ForwardHopsCount; u8Index++) {
		uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPathDiscoveryConfirm->m_aForwardPath[u8Index].m_u16HopAddress >> 8);
		uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPathDiscoveryConfirm->m_aForwardPath[u8Index].m_u16HopAddress);
		uc_serial_rsp_buf[us_serial_response_len++] = pPathDiscoveryConfirm->m_aForwardPath[u8Index].m_u8Mns;
		uc_serial_rsp_buf[us_serial_response_len++] = pPathDiscoveryConfirm->m_aForwardPath[u8Index].m_u8LinkCost;
		uc_serial_rsp_buf[us_serial_response_len++] = pPathDiscoveryConfirm->m_aForwardPath[u8Index].m_u8PhaseDiff;
		uc_serial_rsp_buf[us_serial_response_len++] = pPathDiscoveryConfirm->m_aForwardPath[u8Index].m_u8Mrx;
		uc_serial_rsp_buf[us_serial_response_len++] = pPathDiscoveryConfirm->m_aForwardPath[u8Index].m_u8Mtx;
	}

	for (u8Index = 0; u8Index < pPathDiscoveryConfirm->m_u8ReverseHopsCount; u8Index++) {
		uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPathDiscoveryConfirm->m_aReversePath[u8Index].m_u16HopAddress >> 8);
		uc_serial_rsp_buf[us_serial_response_len++] = (uint8_t)(pPathDiscoveryConfirm->m_aReversePath[u8Index].m_u16HopAddress);
		uc_serial_rsp_buf[us_serial_response_len++] = pPathDiscoveryConfirm->m_aReversePath[u8Index].m_u8Mns;
		uc_serial_rsp_buf[us_serial_response_len++] = pPathDiscoveryConfirm->m_aReversePath[u8Index].m_u8LinkCost;
		uc_serial_rsp_buf[us_serial_response_len++] = pPathDiscoveryConfirm->m_aReversePath[u8Index].m_u8PhaseDiff;
		uc_serial_rsp_buf[us_serial_response_len++] = pPathDiscoveryConfirm->m_aReversePath[u8Index].m_u8Mrx;
		uc_serial_rsp_buf[us_serial_response_len++] = pPathDiscoveryConfirm->m_aReversePath[u8Index].m_u8Mtx;
	}

	/* set usi parameters */
	x_adp_serial_msg.uc_protocol_type = PROTOCOL_ADP_G3;
	x_adp_serial_msg.ptr_buf = &uc_serial_rsp_buf[0];
	x_adp_serial_msg.us_len = us_serial_response_len;
	usi_cli_send_cmd(&x_adp_serial_msg);
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerAdpInitialize(const uint8_t *puc_msg_content)
{
	enum ESerialStatus status = SERIAL_STATUS_SUCCESS;

	uint8_t m_u8Band = *puc_msg_content;

	PRINTF(PRINT_INFO, "_triggerAdpInitialize\r\n");

	AdpInitialize(serial_if_adp_get_notifications(), (enum TAdpBand)m_u8Band);

	adp_mac_serial_if_set_state(SERIAL_MODE_ADP);

	status = SERIAL_STATUS_SUCCESS;

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerAdpDataRequest(const uint8_t *puc_msg_content)
{
	uint8_t *puc_buffer = (uint8_t *)puc_msg_content;
	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;
	uint8_t u8NsduHandle;
	bool bDiscoverRoute;
	uint8_t u8QualityOfService;
	uint16_t u16NsduLength;
	const uint8_t *pNsdu;

	PRINTF(PRINT_INFO, "_triggerAdpDataRequest\r\n");

	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		u8NsduHandle = (uint8_t)*puc_buffer++;
		bDiscoverRoute = (bool) * puc_buffer++;
		u8QualityOfService = (uint8_t)*puc_buffer++;
		u16NsduLength = ((uint16_t)*puc_buffer++) << 8;
		u16NsduLength += (uint16_t)*puc_buffer++;
		pNsdu = puc_buffer;

		AdpDataRequest(u16NsduLength, pNsdu, u8NsduHandle, bDiscoverRoute,
				u8QualityOfService);

		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerAdpDiscoveryRequest(const uint8_t *puc_msg_content)
{
	PRINTF(PRINT_INFO, "_triggerAdpDiscoveryRequest\r\n");

	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;
	uint8_t u8Duration = puc_msg_content[0];
	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		AdpDiscoveryRequest(u8Duration);
		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerAdpNetworkStartRequest(const uint8_t *puc_msg_content)
{
	PRINTF(PRINT_INFO, "_triggerAdpNetworkStartRequest\r\n");

	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;
	uint16_t u16PanId;
	uint8_t *puc_buffer = (uint8_t *)puc_msg_content;

	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		u16PanId = ((uint16_t)*puc_buffer++) << 8;
		u16PanId += (uint16_t)*puc_buffer;

		AdpNetworkStartRequest(u16PanId);
		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerAdpNetworkJoinRequest(const uint8_t *puc_msg_content)
{
	PRINTF(PRINT_INFO, "_triggerAdpNetworkJoinRequest\r\n");

	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;
	uint16_t u16PanId;
	uint16_t u16LbaAddress;
#ifdef G3_HYBRID_PROFILE
	uint8_t u8MediaType;
#endif
	uint8_t *puc_buffer = (uint8_t *)puc_msg_content;
	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		u16PanId = ((uint16_t)*puc_buffer++) << 8;
		u16PanId += (uint16_t)*puc_buffer++;
		u16LbaAddress = ((uint16_t)*puc_buffer++) << 8;
		u16LbaAddress += (uint16_t)*puc_buffer;
#ifdef G3_HYBRID_PROFILE
		u8MediaType = *puc_buffer++;
		AdpNetworkJoinRequest(u16PanId, u16LbaAddress, u8MediaType);
#else
		AdpNetworkJoinRequest(u16PanId, u16LbaAddress);
#endif
		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerAdpNetworkLeaveRequest(const uint8_t *puc_msg_content)
{
	PRINTF(PRINT_INFO, "_triggerAdpNetworkLeaveRequest\r\n");

	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;
	/*	UNUSED(puc_msg_content); */
	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		/* NetworkLeave_Request takes no parameters */
		AdpNetworkLeaveRequest();
		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerAdpResetRequest(const uint8_t *puc_msg_content)
{
	PRINTF(PRINT_INFO, "_triggerAdpResetRequest\r\n");

	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;
	/*	UNUSED(puc_msg_content); */
	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		/* Reset_Request takes no parameters */
		AdpResetRequest();
		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerAdpSetRequest(const uint8_t *puc_msg_content)
{
	PRINTF(PRINT_INFO, "_triggerAdpSetRequest\r\n");

	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;
	uint32_t u32AttributeId;
	uint16_t u16AttributeIndex;
	uint8_t *puc_buffer = (uint8_t *)puc_msg_content;
	uint8_t u8AttributeLengthCnt = 0;
	uint8_t u8AttributeLength = 0;
	uint8_t u8PrefixLength_bytes;

	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		u32AttributeId = ((uint32_t)*puc_buffer++) << 24;
		u32AttributeId += ((uint32_t)*puc_buffer++) << 16;
		u32AttributeId += ((uint32_t)*puc_buffer++) << 8;
		u32AttributeId += (uint32_t)*puc_buffer++;

		u16AttributeIndex = ((uint16_t)*puc_buffer++) << 8;
		u16AttributeIndex += (uint16_t)*puc_buffer++;

		u8AttributeLength = *puc_buffer++;

		PRINTF(PRINT_INFO, "u32AttributeId = %02x\r\n", u32AttributeId);
		switch (u32AttributeId) {
		case ADP_IB_SECURITY_LEVEL:
			PRINTF(PRINT_INFO, "-- ADP_IB_SECURITY_LEVEL\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpSecurityLevel */
			break;

		case ADP_IB_PREFIX_TABLE:
			PRINTF(PRINT_INFO, "-- ADP_IB_PREFIX_TABLE\r\n");
			if (u8AttributeLength) { /* len = 0 => Reset Table */
				u8PrefixLength_bytes = u8AttributeLength - 11;
				auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8PrefixLength */
				auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_bOnLinkFlag */
				auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_bAutonomousAddressConfigurationFlag */
				mem_copy_from_usi_endianness_uint32(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* u32ValidTime */
				u8AttributeLengthCnt += 4;
				puc_buffer += 4;
				mem_copy_from_usi_endianness_uint32(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* u32PreferredTime */
				u8AttributeLengthCnt += 4;
				puc_buffer += 4;
				memcpy((uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt], puc_buffer, u8PrefixLength_bytes); /* m_au8Prefix */
				u8AttributeLengthCnt += u8PrefixLength_bytes;
				puc_buffer += u8PrefixLength_bytes;
			}

			break;

		case ADP_IB_BROADCAST_LOG_TABLE_ENTRY_TTL:
			PRINTF(PRINT_INFO, "-- ADP_IB_BROADCAST_LOG_TABLE_ENTRY_TTL\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16AdpBroadcastLogTableEntryTTL */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_METRIC_TYPE:
			PRINTF(PRINT_INFO, "-- ADP_IB_METRIC_TYPE\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpMetricType */
			break;

		case ADP_IB_LOW_LQI_VALUE:
			PRINTF(PRINT_INFO, "-- ADP_IB_LOW_LQI_VALUE\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpLowLQIValue */
			break;

		case ADP_IB_HIGH_LQI_VALUE:
			PRINTF(PRINT_INFO, "-- ADP_IB_HIGH_LQI_VALUE\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpHighLQIValue */
			break;

		case ADP_IB_RREP_WAIT:
			PRINTF(PRINT_INFO, "-- ADP_IB_RREP_WAIT\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpRREPWait */
			break;

		case ADP_IB_CONTEXT_INFORMATION_TABLE:
			PRINTF(PRINT_INFO, "-- ADP_IB_CONTEXT_INFORMATION_TABLE\r\n");
			if (u8AttributeLength) { /* len = 0 => Reset Table */
				uint8_t u8ContextLength  = u8AttributeLength - 4;
				mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* u16ValidTime */
				u8AttributeLengthCnt += 2;
				puc_buffer += 2;
				auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_bValidForCompression */
				auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8BitsContextLength */
				memcpy((uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt], puc_buffer, u8ContextLength); /* m_au8Context */
				u8AttributeLengthCnt += u8ContextLength;
				puc_buffer += u8ContextLength;
			}

			break;

		case ADP_IB_COORD_SHORT_ADDRESS:
			PRINTF(PRINT_INFO, "-- ADP_IB_COORD_SHORT_ADDRESS\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16AdpCoordShortAddress */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_RLC_ENABLED:
			PRINTF(PRINT_INFO, "-- ADP_IB_RLC_ENABLED\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_bAdpRLCEnabled */
			break;

		case ADP_IB_ADD_REV_LINK_COST:
			PRINTF(PRINT_INFO, "-- ADP_IB_ADD_REV_LINK_COST\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpAddRevLinkCost */
			break;

		case ADP_IB_BROADCAST_LOG_TABLE:
			PRINTF(PRINT_INFO, "-- ADP_IB_BROADCAST_LOG_TABLE\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16SrcAddr */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8SequenceNumber */
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* u16ValidTime */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_ROUTING_TABLE:
			PRINTF(PRINT_INFO, "-- ADP_IB_ROUTING_TABLE\r\n");
			if (u8AttributeLength) { /* len = 0 => Reset Table */
				mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16DstAddr */
				u8AttributeLengthCnt += 2;
				puc_buffer += 2;
				mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16NextHopAddr */
				u8AttributeLengthCnt += 2;
				puc_buffer += 2;
				mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16RouteCost */
				u8AttributeLengthCnt += 2;
				puc_buffer += 2;
				auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8HopCount || m_u8WeakLinkCount */
#ifdef G3_HYBRID_PROFILE
				auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8MediaType */
#endif
				mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_i32ValidTime */
				u8AttributeLengthCnt += 2;
				puc_buffer += 2;
			}

			break;

		case ADP_IB_UNICAST_RREQ_GEN_ENABLE:
			PRINTF(PRINT_INFO, "-- ADP_IB_UNICAST_RREQ_GEN_ENABLE\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_bAdpUnicastRREQGenEnable */
			break;

		case ADP_IB_GROUP_TABLE:
			PRINTF(PRINT_INFO, "-- ADP_IB_GROUP_TABLE\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16GroupAddress */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			/* auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++ ;//m_bValid */
			break;

		case ADP_IB_MAX_HOPS:
			PRINTF(PRINT_INFO, "-- ADP_IB_MAX_HOPS\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpMaxHops */
			break;

		case ADP_IB_DEVICE_TYPE:
			PRINTF(PRINT_INFO, "-- ADP_IB_DEVICE_TYPE\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_eAdpDeviceType */
			break;

		case ADP_IB_NET_TRAVERSAL_TIME:
			PRINTF(PRINT_INFO, "-- ADP_IB_NET_TRAVERSAL_TIME\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpNetTraversalTime */
			break;

		case ADP_IB_ROUTING_TABLE_ENTRY_TTL:
			PRINTF(PRINT_INFO, "-- ADP_IB_ROUTING_TABLE_ENTRY_TTL\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16AdpRoutingTableEntryTTL */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_KR:
			PRINTF(PRINT_INFO, "-- ADP_IB_KR\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpKr */
			break;

		case ADP_IB_KM:
			PRINTF(PRINT_INFO, "-- ADP_IB_KM\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpKm */
			break;

		case ADP_IB_KC:
			PRINTF(PRINT_INFO, "-- ADP_IB_KC\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpKc */
			break;

		case ADP_IB_KQ:
			PRINTF(PRINT_INFO, "-- ADP_IB_KQ\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpKq */
			break;

		case ADP_IB_KH:
			PRINTF(PRINT_INFO, "-- ADP_IB_KH\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpKh */
			break;

		case ADP_IB_RREQ_RETRIES:
			PRINTF(PRINT_INFO, "-- ADP_IB_RREQ_RETRIES\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpRREQRetries */
			break;

		case ADP_IB_RREQ_RERR_WAIT: /* ADP_IB_RREQ_WAIT also enters here as it has the same numeric value */
			PRINTF(PRINT_INFO, "-- ADP_IB_RREQ_RERR_WAIT\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpRREQRERRWait */
			break;

		case ADP_IB_WEAK_LQI_VALUE:
			PRINTF(PRINT_INFO, "-- ADP_IB_WEAK_LQI_VALUE\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpWeakLQIValue */
			break;

		case ADP_IB_KRT:
			PRINTF(PRINT_INFO, "-- ADP_IB_KRT\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpKrt */
			break;

		case ADP_IB_SOFT_VERSION:
			PRINTF(PRINT_INFO, "-- ADP_IB_SOFT_VERSION\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8Major */
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8Minor */
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8Revision */
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8Year */
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8Month */
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8Day */
			break;

		/*		case ADP_IB_SNIFFER_MODE: */
		/*			//TODO */
		/*			break; */
		case ADP_IB_BLACKLIST_TABLE:
			PRINTF(PRINT_INFO, "-- ADP_IB_BLACKLIST_TABLE\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16Addr */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
#ifdef G3_HYBRID_PROFILE
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8MediaType */
#endif
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* u16ValidTime */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_BLACKLIST_TABLE_ENTRY_TTL:
			PRINTF(PRINT_INFO, "-- ADP_IB_BLACKLIST_TABLE_ENTRY_TTL\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16AdpBlacklistTableEntryTTL */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MAX_JOIN_WAIT_TIME:
			PRINTF(PRINT_INFO, "-- ADP_IB_MAX_JOIN_WAIT_TIME\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16AdpMaxJoinWaitTime */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_PATH_DISCOVERY_TIME:
			PRINTF(PRINT_INFO, "-- ADP_IB_PATH_DISCOVERY_TIME\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpPathDiscoveryTime */
			break;

		case ADP_IB_ACTIVE_KEY_INDEX:
			PRINTF(PRINT_INFO, "-- ADP_IB_ACTIVE_KEY_INDEX\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpActiveKeyIndex */
			break;

		case ADP_IB_DESTINATION_ADDRESS_SET:
			PRINTF(PRINT_INFO, "-- ADP_IB_DESTINATION_ADDRESS_SET\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16Addr */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_DEFAULT_COORD_ROUTE_ENABLED:
			PRINTF(PRINT_INFO, "-- ADP_IB_DEFAULT_COORD_ROUTE_ENABLED\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_bAdpDefaultCoordRouteEnabled */
			break;

		case ADP_IB_DISABLE_DEFAULT_ROUTING:
			PRINTF(PRINT_INFO, "-- ADP_IB_DISABLE_DEFAULT_ROUTING\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_bAdpDisableDefaultRouting */
			break;

		/* manufacturer */
		case ADP_IB_MANUF_REASSEMBY_TIMER:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_REASSEMBY_TIMER\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16AdpReassembyTimer */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_IPV6_HEADER_COMPRESSION:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_IPV6_HEADER_COMPRESSION\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_bAdpIPv6HeaderCompression */
			break;

		case ADP_IB_MANUF_EAP_PRESHARED_KEY:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_EAP_PRESHARED_KEY\r\n");
			memcpy((uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt], puc_buffer, u8AttributeLength); /* m_au8Value */
			u8AttributeLengthCnt += u8AttributeLength;
			puc_buffer += u8AttributeLength;
			break;

		case ADP_IB_MANUF_EAP_NETWORK_ACCESS_IDENTIFIER:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_EAP_NETWORK_ACCESS_IDENTIFIER\r\n");
			memcpy((uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt], puc_buffer, u8AttributeLength); /* m_au8Value */
			u8AttributeLengthCnt += u8AttributeLength;
			puc_buffer += u8AttributeLength;
			break;

		case ADP_IB_MANUF_BROADCAST_SEQUENCE_NUMBER:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_BROADCAST_SEQUENCE_NUMBER\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8BroadcastSequenceNumber */
			break;

		case ADP_IB_MANUF_REGISTER_DEVICE:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_REGISTER_DEVICE\r\n");
			memcpy((uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt], puc_buffer, 8); /* m_EUI64Address */
			u8AttributeLengthCnt += 8;
			puc_buffer += 8;
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16PanId */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16ShortAddress */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8KeyIndex */
			memcpy((uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt], puc_buffer, 16); /* m_au8GMK */
			u8AttributeLengthCnt += 16;
			puc_buffer += 16;
			break;

		case ADP_IB_MANUF_DATAGRAM_TAG:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_DATAGRAM_TAG\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16DatagramTag */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_RANDP:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_RANDP\r\n");
			memcpy((uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt], puc_buffer, u8AttributeLength); /* m_au8Value */
			u8AttributeLengthCnt += u8AttributeLength;
			puc_buffer += u8AttributeLength;
			break;

		case ADP_IB_MANUF_ROUTING_TABLE_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_ROUTING_TABLE_COUNT\r\n");
			mem_copy_from_usi_endianness_uint32(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_au8RandP */
			u8AttributeLengthCnt += 4;
			puc_buffer += 4;
			break;

		case ADP_IB_MANUF_DISCOVER_SEQUENCE_NUMBER:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_DISCOVER_SEQUENCE_NUMBER\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16DiscoverRouteGlobalSeqNo */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_FORCED_NO_ACK_REQUEST:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_FORCED_NO_ACK_REQUEST\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8ForceNoAck */
			break;

		case ADP_IB_MANUF_LQI_TO_COORD:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_LQI_TO_COORD\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8LQIToCoord */
			break;

		case ADP_IB_MANUF_BROADCAST_ROUTE_ALL:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_BROADCAST_ROUTE_ALL\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_bBroadcastRouteAll */
			break;

		case ADP_IB_MANUF_KEEP_PARAMS_AFTER_KICK_LEAVE:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_KEEP_PARAMS_AFTER_KICK_LEAVE\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_bKeepParamsAfterKickLeave */
			break;

		case ADP_IB_MANUF_ADP_INTERNAL_VERSION:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_ADP_MAC_INTERNAL_VERSION\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8Major */
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8Minor */
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8Revision */
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8Year */
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8Month */
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8Day */
			break;

		case ADP_IB_MANUF_CIRCULAR_ROUTES_DETECTED:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_CIRCULAR_ROUTES_DETECTED\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16CircularRoutesDetected */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_LAST_CIRCULAR_ROUTE_ADDRESS:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_LAST_CIRCULAR_ROUTE_ADDRESS\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16CircularRoutesDetected */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_IPV6_ULA_DEST_SHORT_ADDRESS:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_IPV6_ULA_DEST_SHORT_ADDRESS\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16CircularRoutesDetected */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_MAX_REPAIR_RESEND_ATTEMPTS:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_MAX_REPAIR_RESEND_ATTEMPTS\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8MaxRepairReSendAttemps */
			break;

		case ADP_IB_MANUF_DISABLE_AUTO_RREQ:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_DISABLE_AUTO_RREQ\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_bDisableAutoRREQ */
			break;

		case ADP_IB_MANUF_ALL_NEIGHBORS_BLACKLISTED_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_ALL_NEIGHBORS_BLACKLISTED_COUNT\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16AllNeighborBlacklistedCount */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_QUEUED_ENTRIES_REMOVED_TIMEOUT_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_QUEUED_ENTRIES_REMOVED_TIMEOUT_COUNT\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16QueuedEntriesRemovedTimeoutCount */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_QUEUED_ENTRIES_REMOVED_ROUTE_ERROR_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_QUEUED_ENTRIES_REMOVED_ROUTE_ERROR_COUNT\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16QueuedEntriesRemovedRouteErrorCount */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_PENDING_DATA_IND_SHORT_ADDRESS:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_PENDING_DATA_IND_SHORT_ADDRESS\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_PendingDataIndication.m_DstDeviceAddress.m_u16ShortAddr */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_GET_BAND_CONTEXT_TONES:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_GET_BAND_CONTEXT_TONES\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_BandContext.m_u8Tones */
			break;

		case ADP_IB_MANUF_UPDATE_NON_VOLATILE_DATA:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_UPDATE_NON_VOLATILE_DATA\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /*  m_bUpdNonVolatileData */
			break;

		case ADP_IB_MANUF_DISCOVER_ROUTE_GLOBAL_SEQ_NUM:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_DISCOVER_ROUTE_GLOBAL_SEQ_NUM\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16DiscoverRouteGlobalSeqNo */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_FRAGMENT_DELAY:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_FRAGMENT_DELAY\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16AdpFragmentDelay */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_DYNAMIC_FRAGMENT_DELAY_ENABLED:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_DYNAMIC_FRAGMENT_DELAY_ENABLED\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /*  m_bAdpDynamicFragmentDelayEnabled */
			break;

		case ADP_IB_MANUF_DYNAMIC_FRAGMENT_DELAY_FACTOR:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_DYNAMIC_FRAGMENT_DELAY_FACTOR\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16AdpDynamicFragmentDelayFactor */
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_BLACKLIST_TABLE_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_BLACKLIST_TABLE_COUNT\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_BROADCAST_LOG_TABLE_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_BROADCAST_LOG_TABLE_COUNT\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_CONTEXT_INFORMATION_TABLE_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_CONTEXT_INFORMATION_TABLE_COUNT\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_GROUP_TABLE_COUNT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_GROUP_TABLE_COUNT\r\n");
			mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
			u8AttributeLengthCnt += 2;
			puc_buffer += 2;
			break;

		case ADP_IB_MANUF_ROUTING_TABLE_ELEMENT:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_ROUTING_TABLE_ELEMENT\r\n");
			if (u8AttributeLength) { /* len = 0 => Reset Table */
				mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16DstAddr */
				u8AttributeLengthCnt += 2;
				puc_buffer += 2;
				mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16NextHopAddr */
				u8AttributeLengthCnt += 2;
				puc_buffer += 2;
				mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_u16RouteCost */
				u8AttributeLengthCnt += 2;
				puc_buffer += 2;
				auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8HopCount || m_u8WeakLinkCount */
#ifdef G3_HYBRID_PROFILE
				auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8MediaType */
#endif
				mem_copy_from_usi_endianness_uint16(puc_buffer, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);  /* m_i32ValidTime */
				u8AttributeLengthCnt += 2;
				puc_buffer += 2;
			}
			break;

		case ADP_IB_MANUF_SET_PHASEDIFF_PREQ_PREP:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_SET_PHASEDIFF_PREQ_PREP\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_bSetPhaseDiffPREQPREP */
			break;

		case ADP_IB_MANUF_HYBRID_PROFILE:
			PRINTF(PRINT_INFO, "-- ADP_IB_MANUF_HYBRID_PROFILE\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_bAdpHybridProfile */
			break;
			
#ifdef G3_HYBRID_PROFILE
		case ADP_IB_LOW_LQI_VALUE_RF:
			PRINTF(PRINT_INFO, "-- ADP_IB_LOW_LQI_VALUE_RF\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpLowLQIValueRF */
			break;

		case ADP_IB_HIGH_LQI_VALUE_RF:
			PRINTF(PRINT_INFO, "-- ADP_IB_HIGH_LQI_VALUE_RF\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpHighLQIValueRF */
			break;

		case ADP_IB_KQ_RF:
			PRINTF(PRINT_INFO, "-- ADP_IB_KQ_RF\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpKqRF */
			break;

		case ADP_IB_KH_RF:
			PRINTF(PRINT_INFO, "-- ADP_IB_KH_RF\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpKhRF */
			break;

		case ADP_IB_KRT_RF:
			PRINTF(PRINT_INFO, "-- ADP_IB_KRT_RF\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpKrtRF */
			break;

		case ADP_IB_KDC_RF:
			PRINTF(PRINT_INFO, "-- ADP_IB_KDC_RF\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_u8AdpKdcRF */
			break;

		case ADP_IB_USE_BACKUP_MEDIA:
			PRINTF(PRINT_INFO, "-- ADP_IB_USE_BACKUP_MEDIA\r\n");
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_buffer++; /* m_bAdpUseBackupMedia */
			break;
#endif

		default:
			break;
		}
		if (u8AttributeLength == u8AttributeLengthCnt) {
			AdpSetRequest(u32AttributeId, u16AttributeIndex, u8AttributeLengthCnt, &auc_aux_endiannes_buf[0]);
			status = SERIAL_STATUS_SUCCESS;
			PRINTF(PRINT_INFO, "---- SERIAL_STATUS_SUCCESS\r\n");
		} else {
			status = SERIAL_STATUS_INVALID_PARAMETER;
			PRINTF(PRINT_INFO, "---- SERIAL_STATUS_INVALID_PARAMETER\r\n");
		}
	}

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerAdpGetRequest(        const uint8_t *puc_msg_content)
{
	PRINTF(PRINT_INFO, "_triggerAdpGetRequest\r\n");

	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;
	uint32_t u32AttributeId;
	uint16_t u16AttributeIndex;
	uint8_t *puc_buffer = (uint8_t *)puc_msg_content;

	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		u32AttributeId = ((uint32_t)*puc_buffer++) << 24;
		u32AttributeId += ((uint32_t)*puc_buffer++) << 16;
		u32AttributeId += ((uint32_t)*puc_buffer++) << 8;
		u32AttributeId += (uint32_t)*puc_buffer++;

		u16AttributeIndex = ((uint16_t)*puc_buffer++) << 8;
		u16AttributeIndex += (uint16_t)*puc_buffer;
		AdpGetRequest(u32AttributeId, u16AttributeIndex);
		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerAdpLbpRequest( const uint8_t *puc_msg_content)
{
	PRINTF(PRINT_INFO, "_triggerAdpLbpRequest\r\n");
	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;
	uint8_t u8NsduHandle;
	uint8_t u8MaxHops;
	bool bDiscoverRoute;
	uint8_t u8QualityOfService;
	bool bSecurityEnable;
	uint8_t u8DstAddrLength;
	uint16_t u16NsduLength;
	uint8_t *puc_buffer = (uint8_t *)puc_msg_content;

	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		u8NsduHandle = (uint8_t)*puc_buffer++;
		u8MaxHops = (uint8_t)*puc_buffer++;
		bDiscoverRoute = (bool) * puc_buffer++;
		u8QualityOfService = (uint8_t)*puc_buffer++;
		bSecurityEnable = (bool) * puc_buffer++;
		u8DstAddrLength = (uint8_t)*puc_buffer++;
		u16NsduLength = ((uint16_t)*puc_buffer++) << 8;
		u16NsduLength += (uint16_t)*puc_buffer++;

		struct TAdpAddress dstAddr;
		if (u8DstAddrLength == 2) {
			dstAddr.m_u8AddrSize = ADP_ADDRESS_16BITS;
			dstAddr.m_u16ShortAddr = ((uint16_t)*puc_buffer++) << 8;
			dstAddr.m_u16ShortAddr += (uint16_t)*puc_buffer++;
		} else {
			dstAddr.m_u8AddrSize = ADP_ADDRESS_64BITS;
			memcpy(&dstAddr.m_ExtendedAddress, puc_buffer, dstAddr.m_u8AddrSize);
			puc_buffer +=  ADP_ADDRESS_64BITS;
		}

		uint8_t *pNsdu =  puc_buffer;
		AdpLbpRequest((const struct TAdpAddress *)&dstAddr, u16NsduLength, pNsdu, u8NsduHandle,
				u8MaxHops, bDiscoverRoute, u8QualityOfService, bSecurityEnable);
		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerAdpRouteDiscoveryRequest( const uint8_t *puc_msg_content)
{
	PRINTF(PRINT_INFO, "_triggerAdpRouteDiscoveryRequest\r\n");
	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;
	uint16_t u16DstAddr;
	uint8_t u8MaxHops;
	uint8_t *puc_buffer = (uint8_t *)puc_msg_content;

	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		u16DstAddr = ((uint16_t)*puc_buffer++) << 8;
		u16DstAddr += (uint16_t)*puc_buffer++;
		u8MaxHops = (uint8_t)*puc_buffer;

		AdpRouteDiscoveryRequest(u16DstAddr, u8MaxHops);
		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerAdpMacSetRequest( const uint8_t *puc_msg_content)
{
	PRINTF(PRINT_INFO, "_triggerAdpMacSetRequest\r\n");

	enum ESerialStatus status = SERIAL_STATUS_INVALID_PARAMETER;

	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		uint8_t *puc_buffer = (uint8_t *)puc_msg_content;
		process_MIB_set_request(puc_buffer, &set_params);
		AdpMacSetRequest(set_params.m_ePibAttribute, set_params.m_u16PibAttributeIndex,
				set_params.m_PibAttributeValue.m_u8Length, &set_params.m_PibAttributeValue.m_au8Value[0]);
		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerAdpMacGetRequest( const uint8_t *puc_msg_content)
{
	PRINTF(PRINT_INFO, "_triggerAdpMacGetRequest\r\n");

	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;
	uint32_t u32AttributeId;
	uint16_t u16AttributeIndex;
	uint8_t *puc_buffer = (uint8_t *)puc_msg_content;

	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		u32AttributeId = ((uint32_t)*puc_buffer++) << 24;
		u32AttributeId += ((uint32_t)*puc_buffer++) << 16;
		u32AttributeId += ((uint32_t)*puc_buffer++) << 8;
		u32AttributeId += (uint32_t)*puc_buffer++;

		u16AttributeIndex = ((uint16_t)*puc_buffer++) << 8;
		u16AttributeIndex += (uint16_t)*puc_buffer;
		AdpMacGetRequest(u32AttributeId, u16AttributeIndex);
		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
static enum ESerialStatus _triggerAdpPathDiscoveryRequest(  const uint8_t *puc_msg_content)
{
	PRINTF(PRINT_INFO, "_triggerAdpPathDiscoveryRequest\r\n");

	enum ESerialStatus status = SERIAL_STATUS_NOT_ALLOWED;
	uint16_t u16DstAddr;
	uint8_t u8MetricType;
	uint8_t *puc_buffer = (uint8_t *)puc_msg_content;

	if (adp_mac_serial_if_get_state() == SERIAL_MODE_ADP || adp_mac_serial_if_get_state() == SERIAL_MODE_COORD) {
		u16DstAddr = ((uint16_t)*puc_buffer++) << 8;
		u16DstAddr += (uint16_t)*puc_buffer++;
		u8MetricType = (uint8_t)*puc_buffer;

		AdpPathDiscoveryRequest(u16DstAddr, u8MetricType);
		status = SERIAL_STATUS_SUCCESS;
	}

	return status;
}

uint8_t serial_if_g3adp_api_parser(uint8_t *puc_rx_msg, uint16_t us_len)
{
	PRINTF(PRINT_INFO, "serial_if_g3adp_api_parser\r\n");

	uint8_t uc_serial_if_cmd;
	uint8_t *puc_rx;
	enum ESerialStatus status = SERIAL_STATUS_UNKNOWN_COMMAND;

	/* Protection for invalid us_length */
	if (!us_len) {
		return(false);
	}

	/* Process received message */
	uc_serial_if_cmd = puc_rx_msg[0] & 0x3F;
	puc_rx = &puc_rx_msg[1];

	switch (uc_serial_if_cmd) {
	case SERIAL_MSG_ADP_INITIALIZE:
		status = _triggerAdpInitialize(puc_rx);
		break;

	case SERIAL_MSG_ADP_DATA_REQUEST:
		status = _triggerAdpDataRequest(puc_rx);
		break;

	case SERIAL_MSG_ADP_DISCOVERY_REQUEST:
		status = _triggerAdpDiscoveryRequest(puc_rx);
		break;

	case SERIAL_MSG_ADP_NETWORK_START_REQUEST:
		status = _triggerAdpNetworkStartRequest(puc_rx);
		break;

	case SERIAL_MSG_ADP_NETWORK_JOIN_REQUEST:
		status = _triggerAdpNetworkJoinRequest(puc_rx);
		break;

	case SERIAL_MSG_ADP_NETWORK_LEAVE_REQUEST:
		status = _triggerAdpNetworkLeaveRequest(puc_rx);
		break;

	case SERIAL_MSG_ADP_RESET_REQUEST:
		status = _triggerAdpResetRequest(puc_rx);
		break;

	case SERIAL_MSG_ADP_SET_REQUEST:
		_triggerAdpSetRequest(puc_rx);
		break;

	case SERIAL_MSG_ADP_GET_REQUEST:
		_triggerAdpGetRequest(puc_rx);
		break;

	case SERIAL_MSG_ADP_LBP_REQUEST:
		status = _triggerAdpLbpRequest(puc_rx);
		break;

	case SERIAL_MSG_ADP_ROUTE_DISCOVERY_REQUEST:
		status = _triggerAdpRouteDiscoveryRequest(puc_rx);
		break;

	case SERIAL_MSG_ADP_PATH_DISCOVERY_REQUEST:
		status = _triggerAdpPathDiscoveryRequest(puc_rx);
		break;

	case SERIAL_MSG_ADP_MAC_SET_REQUEST:
		status = _triggerAdpMacSetRequest(puc_rx);
		break;

	case SERIAL_MSG_ADP_MAC_GET_REQUEST:
		status = _triggerAdpMacGetRequest(puc_rx);
		break;

	default:
		break;
	}

	/* initialize doesn't have Confirm so send Status */
	/* other messages all have send / confirm send status only if there is a processing error */
	if ((status != SERIAL_STATUS_UNKNOWN_COMMAND) && ((status != SERIAL_STATUS_SUCCESS) || (uc_serial_if_cmd == SERIAL_MSG_ADP_INITIALIZE))) {
		MsgStatus(status, uc_serial_if_cmd);
		status = SERIAL_STATUS_SUCCESS;
	}

	/* if (status == SERIAL_STATUS_SUCCESS) return(true); */
	return(true);
}

struct TAdpNotifications *serial_if_adp_get_notifications(void)
{
	PRINTF(PRINT_INFO, "serial_if_adp_get_notifications\r\n");

	ss_notifications.fnctAdpDataConfirm = AdpNotification_DataConfirm;
	ss_notifications.fnctAdpDataIndication = AdpNotification_AdpdDataIndication;
	ss_notifications.fnctAdpDiscoveryConfirm = AdpNotification_DiscoveryConfirm;
	ss_notifications.fnctAdpDiscoveryIndication = AdpNotification_DiscoveryIndication;
	ss_notifications.fnctAdpNetworkStartConfirm = AdpNotification_NetworkStartConfirm;
	ss_notifications.fnctAdpNetworkJoinConfirm = AdpNotification_NetworkJoinConfirm;
	ss_notifications.fnctAdpNetworkLeaveIndication = AdpNotification_NetworkLeaveIndication;
	ss_notifications.fnctAdpNetworkLeaveConfirm = AdpNotification_NetworkLeaveConfirm;
	ss_notifications.fnctAdpResetConfirm = AdpNotification_ResetConfirm;
	ss_notifications.fnctAdpSetConfirm = AdpNotification_SetConfirm;
	ss_notifications.fnctAdpMacSetConfirm = AdpNotification_SetMacConfirm;
	ss_notifications.fnctAdpGetConfirm = AdpNotification_GetConfirm;
	ss_notifications.fnctAdpMacGetConfirm = AdpNotification_GetMacConfirm;
	ss_notifications.fnctAdpLbpConfirm = AdpNotification_LbpConfirm;
	ss_notifications.fnctAdpLbpIndication = AdpNotification_LbpIndication;
	ss_notifications.fnctAdpRouteDiscoveryConfirm = AdpNotification_RouteDiscoveryConfirm;
	ss_notifications.fnctAdpPathDiscoveryConfirm = AdpNotification_PathDiscoveryConfirm;
	ss_notifications.fnctAdpNetworkStatusIndication = AdpNotification_NetworkStatusIndication;
	ss_notifications.fnctAdpBufferIndication = AdpNotification_BufferIndication;
	ss_notifications.fnctAdpPREQIndication = AppAdpNotification_PREQIndication;
	ss_notifications.fnctAdpUpdNonVolatileDataIndication = AppAdpNotification_UpdNonVolatileDataIndication;
	ss_notifications.fnctAdpRouteNotFoundIndication = AppAdpNotification_RouteNotFoundIndication;
	return &ss_notifications;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
