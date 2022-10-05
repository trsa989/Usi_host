/**
 * \file
 *
 * \brief USI Host Interface to G3 ADP Layer
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
/* System includes */
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "../addUsi.h"
#include "../G3.h"

#include "Usi.h"

#include <AdpApi.h>
#include <AdpApiTypes.h>
#include <mac_wrapper_defs.h>

#define MAX_SIZE_MAC_BUFFER     G3_MACSAP_DATA_SIZE + (G3_MACSAP_DATA_SIZE / 2)

/* #define LOG_IFACE_G3_ADP(x ...) LOG_G3_DEBUG(x) */
#define LOG_IFACE_G3_ADP(X...)

/* Buffer for USI Frame Message Data Field */
static Uint8 buffTxAdpG3[G3_MACSAP_DATA_SIZE];
/* Global Command Params for ADP */
static CmdParams adpG3Msg;

/* Callbacks */
struct TAdpNotifications g_adpNotifications;

/* *** Structures ************************************************************ */
typedef struct {
	bool f_sync_req;                       /* Flag to indicate syncronous request */
	bool f_sync_res;                       /* Flag to indicate syncronous response */
	uint32_t m_u32AttributeId;             /* In order to handle correctly syncronous requests */
	struct TAdpGetConfirm s_GetConfirm;    /* Adp Get Confirm callback struct */
	struct TAdpSetConfirm s_SetConfirm;    /* Adp Set Confirm callback struct */
	struct TAdpMacGetConfirm s_MacGetConfirm; /* Adp Mac Get Confirm callback struct */
	struct TAdpMacSetConfirm s_MacSetConfirm; /* Adp Mac Set Confirm callback struct */
} T_adp_sync_mgmt;

T_adp_sync_mgmt g_adp_sync_mgmt;

/**********************************************************************************************************************/

/** Forward declaration
 **********************************************************************************************************************/

/**********************************************************************************************************************/

/** Use this function to initialize the ADP layer. The ADP layer should be initialized before doing any other operation.
 *  The APIs cannot be mixed, if the stack is initialized in ADP mode then only the ADP functions can be used and if the
 *  stack is initialized in MAC mode then only MAC functions can be used.
 *  @param pNotifications Structure with callbacks used to notify ADP specific events (if NULL the layer is deinitialized)
 *  @param band Working band [CEN-A,CEN-B,FCC] (should be inline with the hardware)
 **********************************************************************************************************************/
void AdpInitialize(struct TAdpNotifications *pNotifications, enum TAdpBand band)
{
	Uint8 *ptrBuff; /* Pointer to Message Data of USI Frame */
	Uint16 length; /* Message Data Length of USI Frame */
	int result;

	LOG_IFACE_G3_ADP("AdpInitialize Band=%d\r\n", band);

	/* Copy callbacks */
	memcpy(&g_adpNotifications, pNotifications, sizeof(struct TAdpNotifications));

	ptrBuff = buffTxAdpG3;

	/* Filling Message Data of USI Frame */
	*ptrBuff++ = G3_SERIAL_MSG_ADP_INITIALIZE;
	*ptrBuff++ = (Uint8)(band);
	/* Message Data Length of USI Frame */
	length = ptrBuff - buffTxAdpG3;
	/* Filling USI Frame Structure */
	adpG3Msg.pType = PROTOCOL_ADP_G3; /* Protocol ID of USI Frame */
	adpG3Msg.buf = buffTxAdpG3;
	adpG3Msg.len = length;
	/* Send USI Frame */
	result = usi_SendCmd(&adpG3Msg) ? 0 : -1;

	/* Set sync flags to false */
	g_adp_sync_mgmt.f_sync_req = false;
	g_adp_sync_mgmt.f_sync_res = false;

	LOG_IFACE_G3_ADP("AdpInitialize result = %d\r\n", result);
}

/**********************************************************************************************************************/

/** This function is called periodically in embedded apps. Not needed on Host side.
 **********************************************************************************************************************/
bool AdpEventHandler(void)
{
	return(true);
}

/**********************************************************************************************************************/

/** The AdpDataRequest primitive requests the transfer of an application PDU to another device or multiple devices.
 ***********************************************************************************************************************
 * @param u16NsduLength The size of the NSDU, in bytes; Up to 1280
 * @param pNsdu The NSDU to send; should be a valid IPv6 packet
 * @param u8NsduHandle The handle of the NSDU to transmit. This parameter is used to identify in the AdpDataConfirm
 *                              primitive which request it is concerned with. It can be randomly chosen by the application layer.
 * @param bDiscoverRoute If TRUE, a route discovery procedure will be performed prior to sending the frame if a route
 *                              to the destination is not available in the routing table. If FALSE, no route discovery is performed.
 * @param u8QualityOfService The requested quality of service (QoS) of the frame to send. Allowed values are:
 *				0x00 = normal priority
 *				0x01 = high priority
 **********************************************************************************************************************/
void AdpDataRequest(uint16_t u16NsduLength, const uint8_t *pNsdu,
		uint8_t u8NsduHandle, bool bDiscoverRoute, uint8_t u8QualityOfService)
{
	Uint8 *ptrBuff; /* Pointer to Message Data of USI Frame */
	Uint16 length; /* Message Data Length of USI Frame */
	int i, result;

	LOG_IFACE_G3_ADP("AdpDataRequest init\r\n");
	ptrBuff = buffTxAdpG3;
	/* Filling Message Data of USI Frame */
	*ptrBuff++ = G3_SERIAL_MSG_ADP_DATA_REQUEST;
	*ptrBuff++ = u8NsduHandle;
	*ptrBuff++ = bDiscoverRoute ? 1 : 0;
	*ptrBuff++ = u8QualityOfService;
	*ptrBuff++ = (u16NsduLength >> 8);
	*ptrBuff++ = (u16NsduLength & 0xFF);
	for (i = 0; i < u16NsduLength; i++) {
		*ptrBuff++ = pNsdu[i];
	}
	/* Message Data Length of USI Frame */
	length = ptrBuff - buffTxAdpG3;
	/* Filling USI Frame Structure */
	adpG3Msg.pType = PROTOCOL_ADP_G3;
	adpG3Msg.buf = buffTxAdpG3;
	adpG3Msg.len = length;
	/* Send USI Frame */
	result = usi_SendCmd(&adpG3Msg) ? 0 : -1;
	LOG_IFACE_G3_ADP("AdpDataRequest result = %d\r\n", result);
}

/**********************************************************************************************************************/

/** The AdpDataConfirm primitive allows the upper layer to be notified of the completion of an AdpDataRequest.
 ***********************************************************************************************************************
 * @param m_u8Status The status code of a previous AdpDataRequest identified by its NsduHandle.
 * @param m_u8NsduHandle The handle of the NSDU confirmed by this primitive.
 **********************************************************************************************************************/
typedef void (*AdpDataConfirm)(struct TAdpDataConfirm *pDataConfirm);

/**********************************************************************************************************************/

/** The AdpDataIndication primitive is used to transfer received data from the adaptation sublayer to the upper layer.
 ***********************************************************************************************************************
 * @param m_u16NsduLength The size of the NSDU, in bytes; Up to 1280 bytes
 * @param m_pNsdu The received NSDU
 * @param m_u8LinkQualityIndicator The value of the link quality during reception of the frame.
 **********************************************************************************************************************/
typedef void (*AdpDataIndication)(struct TAdpDataIndication *pDataIndication);

/**********************************************************************************************************************/

/** The AdpDiscoveryRequest primitive allows the upper layer to scan for networks operating in its POS.
 ***********************************************************************************************************************
 * @param u8Duration The number of seconds the scan shall last.
 **********************************************************************************************************************/
void AdpDiscoveryRequest(uint8_t u8Duration)
{
	Uint8 *ptrBuff; /* Pointer to Message Data of USI Frame */
	Uint16 length; /* Message Data Length of USI Frame */
	int result;

	LOG_IFACE_G3_ADP("AdpDiscoveryRequest Duration=%d\r\n", u8Duration);
	ptrBuff = buffTxAdpG3;
	/* Filling Message Data of USI Frame */
	*ptrBuff++ = G3_SERIAL_MSG_ADP_DISCOVERY_REQUEST;
	*ptrBuff++ = u8Duration;
	/* Message Data Length of USI Frame */
	length = ptrBuff - buffTxAdpG3;
	/* Filling USI Frame Structure */
	adpG3Msg.pType = PROTOCOL_ADP_G3;
	adpG3Msg.buf = buffTxAdpG3;
	adpG3Msg.len = length;
	/* Send USI Frame */
	result = usi_SendCmd(&adpG3Msg) ? 0 : -1;
	LOG_IFACE_G3_ADP("AdpDiscoveryRequest result = %d\r\n", result);
}

/**********************************************************************************************************************/

/** The AdpDiscoveryIndication primitive is generated by the ADP layer to notify the application about the discovery
 * of a new PAN coordinator or LBA
 ***********************************************************************************************************************
 * @param pPanDescriptor PAN descriptor contains information about the PAN
 **********************************************************************************************************************/
typedef void (*AdpDiscoveryIndication)(struct TAdpPanDescriptor *pPanDescriptor);

/**********************************************************************************************************************/

/** The AdpDiscoveryConfirm primitive allows the upper layer to be notified of the completion of an AdpDiscoveryRequest.
 ***********************************************************************************************************************
 * @param m_u8Status The status of the scan request.
 **********************************************************************************************************************/
typedef void (*AdpDiscoveryConfirm)(uint8_t m_u8Status);

/**********************************************************************************************************************/

/** The AdpNetworkStartRequest primitive allows the upper layer to request the starting of a new network. It shall
 * only be invoked by a device designated as the PAN coordinator during the factory process.
 ***********************************************************************************************************************
 * @param u16PanId The PANId of the network to create; determined at the application level
 **********************************************************************************************************************/
void AdpNetworkStartRequest(uint16_t u16PanId)
{
	Uint8 *ptrBuff; /* Pointer to Message Data of USI Frame */
	Uint16 length; /* Message Data Length of USI Frame */
	int result;

	LOG_IFACE_G3_ADP("AdpNetworkStartRequest PANid:0x%X\r\n", u16PanId);
	ptrBuff = buffTxAdpG3;
	/* Filling Message Data of USI Frame */
	*ptrBuff++ = G3_SERIAL_MSG_ADP_NETWORK_START_REQUEST;
	*ptrBuff++ = (u16PanId >> 8);
	*ptrBuff++ = (u16PanId & 0xFF);
	/* Message Data Length of USI Frame */
	length = ptrBuff - buffTxAdpG3;
	/* Filling USI Frame Structure */
	adpG3Msg.pType = PROTOCOL_ADP_G3;
	adpG3Msg.buf = buffTxAdpG3;
	adpG3Msg.len = length;
	/* Send USI Frame */
	result = usi_SendCmd(&adpG3Msg) ? 0 : -1;
	LOG_IFACE_G3_ADP("AdpNetworkStartRequest result = %d\r\n", result);
}

/**********************************************************************************************************************/

/** The AdpNetworkStartConfirm primitive allows the upper layer to be notified of the completion of an
 * AdpNetworkStartRequest.
 ***********************************************************************************************************************
 * @param m_u8Status The status of the request.
 **********************************************************************************************************************/
typedef void (*AdpNetworkStartConfirm)(
	struct TAdpNetworkStartConfirm *pNetworkStartConfirm);


#ifdef G3_HYBRID_PROFILE
/**********************************************************************************************************************/

/** The AdpNetworkJoinRequest primitive allows the upper layer to join an existing network.
 ***********************************************************************************************************************
 * @param u16PanId The 16-bit PAN identifier of the network to join.
 * @param u16LbaAddress The 16-bit short address of the device acting as a LoWPAN bootstrap agent (relay)
 * @param u8MediaType The Media Type to use for frame exchange with LBA
 **********************************************************************************************************************/
void AdpNetworkJoinRequest(uint16_t u16PanId, uint16_t u16LbaAddress, uint8_t u8MediaType)
{
	Uint8 *ptrBuff; /* Pointer to Message Data of USI Frame */
	Uint16 length; /* Message Data Length of USI Frame */
	int result;

	LOG_IFACE_G3_ADP("AdpNetworkJoinRequest PANid:0x%X LbaAdress:0x%X MediaType:%d\r\n", u16PanId, u16LbaAddress, u8MediaType);
	ptrBuff = buffTxAdpG3;
	/* Filling Message Data of USI Frame */
	*ptrBuff++ = G3_SERIAL_MSG_ADP_NETWORK_JOIN_REQUEST;
	*ptrBuff++ = (u16PanId >> 8);
	*ptrBuff++ = (u16PanId & 0xFF);
	*ptrBuff++ = (u16LbaAddress >> 8);
	*ptrBuff++ = (u16LbaAddress & 0xFF);
	*ptrBuff++ = u8MediaType;
	/* Message Data Length of USI Frame */
	length = ptrBuff - buffTxAdpG3;
	/* Filling USI Frame Structure */
	adpG3Msg.pType = PROTOCOL_ADP_G3;
	adpG3Msg.buf = buffTxAdpG3;
	adpG3Msg.len = length;
	/* Send USI Frame */
	result = usi_SendCmd(&adpG3Msg) ? 0 : -1;
	LOG_IFACE_G3_ADP("AdpNetworkJoinRequest result = %d\r\n", result);
}
#else
/**********************************************************************************************************************/

/** The AdpNetworkJoinRequest primitive allows the upper layer to join an existing network.
 ***********************************************************************************************************************
 * @param u16PanId The 16-bit PAN identifier of the network to join.
 * @param u16LbaAddress The 16-bit short address of the device acting as a LoWPAN bootstrap agent (relay)
 **********************************************************************************************************************/
void AdpNetworkJoinRequest(uint16_t u16PanId, uint16_t u16LbaAddress)
{
	Uint8 *ptrBuff; /* Pointer to Message Data of USI Frame */
	Uint16 length; /* Message Data Length of USI Frame */
	int result;

	LOG_IFACE_G3_ADP("AdpNetworkJoinRequest PANid:0x%X LbaAdress:0x%X\r\n", u16PanId, u16LbaAddress);
	ptrBuff = buffTxAdpG3;
	/* Filling Message Data of USI Frame */
	*ptrBuff++ = G3_SERIAL_MSG_ADP_NETWORK_JOIN_REQUEST;
	*ptrBuff++ = (u16PanId >> 8);
	*ptrBuff++ = (u16PanId & 0xFF);
	*ptrBuff++ = (u16LbaAddress >> 8);
	*ptrBuff++ = (u16LbaAddress & 0xFF);
	/* Message Data Length of USI Frame */
	length = ptrBuff - buffTxAdpG3;
	/* Filling USI Frame Structure */
	adpG3Msg.pType = PROTOCOL_ADP_G3;
	adpG3Msg.buf = buffTxAdpG3;
	adpG3Msg.len = length;
	/* Send USI Frame */
	result = usi_SendCmd(&adpG3Msg) ? 0 : -1;
	LOG_IFACE_G3_ADP("AdpNetworkJoinRequest result = %d\r\n", result);
}
#endif

/**********************************************************************************************************************/

/** The AdpNetworkJoinConfirm primitive allows the upper layer to be notified of the completion of an
 * AdpNetworkJoinRequest.
 ***********************************************************************************************************************
 * @param m_u8Status The status of the request.
 * @param m_u16NetworkAddress The 16-bit network address that was allocated to the device.
 * @param m_u16PanId The 16-bit address of the PAN of which the device is now a member.
 **********************************************************************************************************************/
typedef void (*AdpNetworkJoinConfirm)(
	struct TAdpNetworkJoinConfirm *pNetworkJoinConfirm);

/**********************************************************************************************************************/

/** The AdpNetworkLeaveRequest primitive allows a non-coordinator device to remove itself from the network.
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
void AdpNetworkLeaveRequest(void)
{
	Uint8 *ptrBuff;
	Uint16 length;
	int result;

	LOG_IFACE_G3_ADP("AdpNetworkLeaveRequest init\r\n");
	ptrBuff = buffTxAdpG3;
	/* Filling Message Data of USI Frame */
	*ptrBuff++ = G3_SERIAL_MSG_ADP_NETWORK_LEAVE_REQUEST;
	/* Message Data Length of USI Frame */
	length = ptrBuff - buffTxAdpG3;
	/* Filling USI Frame Structure */
	adpG3Msg.pType = PROTOCOL_ADP_G3;
	adpG3Msg.buf = buffTxAdpG3;
	adpG3Msg.len = length;
	/* Send USI Frame */
	result = usi_SendCmd(&adpG3Msg) ? 0 : -1;
	LOG_IFACE_G3_ADP("AdpNetworkLeaveRequest result = %d\r\n", result);
}

/**********************************************************************************************************************/

/** The AdpNetworkLeaveConfirm primitive allows the upper layer to be notified of the completion of an
 * AdpNetworkLeaveRequest.
 ***********************************************************************************************************************
 * @param m_u8Status The status of the request.
 * @param m_ExtendedAddress The 64-bit network address of the device
 **********************************************************************************************************************/
typedef void (*AdpNetworkLeaveConfirm)(
	struct TAdpNetworkLeaveConfirm *pLeaveConfirm);

/**********************************************************************************************************************/

/** The AdpNetworkLeaveIndication primitive is generated by the ADP layer of a non-coordinator device to inform
 * the upper layer that it has been unregistered from the network by the coordinator.
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
typedef void (*AdpNetworkLeaveIndication)(void);

/**********************************************************************************************************************/

/** The AdpResetRequest primitive performs a reset of the adaptation sublayer and allows the resetting of the MIB
 * attributes.
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
void AdpResetRequest(void)
{
	Uint8 *ptrBuff;
	Uint16 length;
	int result;

	LOG_IFACE_G3_ADP("AdpResetRequest start\r\n");
	ptrBuff = buffTxAdpG3;
	/* Filling Message Data of USI Frame */
	*ptrBuff++ = G3_SERIAL_MSG_ADP_RESET_REQUEST;
	/* Message Data Length of USI Frame */
	length = ptrBuff - buffTxAdpG3;
	/* Filling USI Frame Structure */
	adpG3Msg.pType = PROTOCOL_ADP_G3;
	adpG3Msg.buf = buffTxAdpG3;
	adpG3Msg.len = length;
	/* Send USI Frame */
	result = usi_SendCmd(&adpG3Msg) ? 0 : -1;
	LOG_IFACE_G3_ADP("AdpResetRequest result = %d\r\n", result);
}

/**********************************************************************************************************************/

/** The AdpResetConfirm primitive allows the upper layer to be notified of the completion of an AdpResetRequest.
 ***********************************************************************************************************************
 * @param m_u8Status The status of the request.
 **********************************************************************************************************************/
typedef void (*AdpResetConfirm)(struct TAdpResetConfirm *pResetConfirm);

/**********************************************************************************************************************/

/** The AdpGetRequest primitive allows the upper layer to get the value of an attribute from the ADP information base.
 ***********************************************************************************************************************
 * @param u32AttributeId The identifier of the ADP IB attribute to read.
 * @param u16AttributeIndex The index within the table of the specified IB attribute to read.
 **********************************************************************************************************************/
void AdpGetRequest(uint32_t u32AttributeId, uint16_t u16AttributeIndex)
{
	Uint8 *ptrBuff;
	Uint16 length;
	int result;

	LOG_IFACE_G3_ADP("AdpGetRequest attr = 0x%X; index = 0x%X\r\n", u32AttributeId, u16AttributeIndex);
	ptrBuff = buffTxAdpG3;
	/* Filling Message Data of USI Frame */
	*ptrBuff++ = G3_SERIAL_MSG_ADP_GET_REQUEST;
	*ptrBuff++ = ((u32AttributeId >> 24) & 0xFF);
	*ptrBuff++ = ((u32AttributeId >> 16) & 0xFF);
	*ptrBuff++ = ((u32AttributeId >> 8) & 0xFF);
	*ptrBuff++ = (u32AttributeId & 0xFF);
	*ptrBuff++ = (u16AttributeIndex >> 8);
	*ptrBuff++ = (u16AttributeIndex & 0xFF);
	/* Message Data Length of USI Frame */
	length = ptrBuff - buffTxAdpG3;
	/* Filling USI Frame Structure */
	adpG3Msg.pType = PROTOCOL_ADP_G3;
	adpG3Msg.buf = buffTxAdpG3;
	adpG3Msg.len = length;
	/* Send USI Frame */
	result = usi_SendCmd(&adpG3Msg) ? 0 : -1;
	LOG_IFACE_G3_ADP("AdpGetRequest result = %d\r\n", result);
}

/**********************************************************************************************************************/

/** The AdpGetRequestSync primitive allows the upper layer to get the value of an attribute from the ADP information
 * synchronously.
 ***********************************************************************************************************************
 * @param u32AttributeId The identifier of the ADP IB attribute to read.
 * @param u16AttributeIndex The index within the table of the specified IB attribute to read.
 * @param pGetConfirm Get result.
 **********************************************************************************************************************/
void AdpGetRequestSync(uint32_t u32AttributeId, uint16_t u16AttributeIndex,
		struct TAdpGetConfirm *pGetConfirm)
{
	uint8_t result = -1;

	LOG_IFACE_G3_ADP("AdpGetRequestSync attr = 0x%X; index = 0x%X\r\n", u32AttributeId, u16AttributeIndex);
	/* Set the sync flags to intercept the callback */
	g_adp_sync_mgmt.f_sync_req = true;
	g_adp_sync_mgmt.f_sync_res = false;
	g_adp_sync_mgmt.m_u32AttributeId = u32AttributeId; /* Used in order to know AttributeId requested on AdpGetConfirm callback */

	/* Send the asynchronous call */
	AdpGetRequest(u32AttributeId, u16AttributeIndex);

	/* Wait processing until flag activates, or timeout */
	addUsi_WaitProcessing(G3_SYNC_TIMEOUT, (Bool *)(&g_adp_sync_mgmt.f_sync_res));

	if (g_adp_sync_mgmt.f_sync_res) {
		/* pGetConfirm = &(g_adp_sync_mgmt.s_GetConfirm); */
		memcpy(pGetConfirm, &g_adp_sync_mgmt.s_GetConfirm, sizeof(struct TAdpGetConfirm));
		result = 0;
	} else {
		/* Confirm not received */
		pGetConfirm->m_u8Status = G3_TIMEOUT;
		LOG_IFACE_G3_ADP("ERROR: Confirm for 0x%X not received\r\n", u32AttributeId);
	}

	g_adp_sync_mgmt.f_sync_req = false;
	g_adp_sync_mgmt.f_sync_res = false;
	LOG_IFACE_G3_ADP("AdpGetRequestSync result = %d\r\n", result);
}

/**********************************************************************************************************************/

/** The AdpGetConfirm primitive allows the upper layer to be notified of the completion of an AdpGetRequest.
 ***********************************************************************************************************************
 * @param m_u8Status The status of the request.
 * @param m_u32AttributeId The identifier of the IB attribute read
 * @param m_u16AttributeIndex The index within the table of the specified IB attribute read.
 * @param m_u8AttributeLength The length of the value of the attribute read from the IB.
 * @param m_au8AttributeValue The value of the attribute read from the IB.
 **********************************************************************************************************************/
typedef void (*AdpGetConfirm)(struct TAdpGetConfirm *pGetConfirm);

/**********************************************************************************************************************/

/** The AdpMacGetRequest primitive allows the upper layer to get the value of an attribute from the MAC information base.
 * The upper layer cannot access directly the MAC layer while ADP is running
 ***********************************************************************************************************************
 * @param u32AttributeId The identifier of the MAC IB attribute to read.
 * @param u16AttributeIndex The index within the table of the specified IB attribute to read.
 **********************************************************************************************************************/
void AdpMacGetRequest(uint32_t u32AttributeId, uint16_t u16AttributeIndex)
{
	Uint8 *ptrBuff;
	Uint16 length;
	int result;

	LOG_IFACE_G3_ADP("AdpMacGetRequest attr = 0x%X - index = 0x%X", u32AttributeId, u16AttributeIndex);
	ptrBuff = buffTxAdpG3;
	/* Filling Message Data of USI Frame */
	*ptrBuff++ = G3_SERIAL_MSG_ADP_MAC_GET_REQUEST;
	*ptrBuff++ = ((u32AttributeId >> 24) & 0xFF);
	*ptrBuff++ = ((u32AttributeId >> 16) & 0xFF);
	*ptrBuff++ = ((u32AttributeId >> 8) & 0xFF);
	*ptrBuff++ = (u32AttributeId & 0xFF);
	*ptrBuff++ = (u16AttributeIndex >> 8);
	*ptrBuff++ = (u16AttributeIndex & 0xFF);
	/* Message Data Length of USI Frame */
	length = ptrBuff - buffTxAdpG3;
	/* Filling USI Frame Structure */
	adpG3Msg.pType = PROTOCOL_ADP_G3;
	adpG3Msg.buf = buffTxAdpG3;
	adpG3Msg.len = length;
	/* Send USI Frame */
	result = usi_SendCmd(&adpG3Msg) ? 0 : -1;
	LOG_IFACE_G3_ADP("AdpMacGetRequest result = %d\r\n", result);
}

/**********************************************************************************************************************/

/** The AdpMacGetRequestSync primitive allows the upper layer to get the value of an attribute from the MAC information
* base synchronously. The upper layer cannot access directly the MAC layer while ADP is running.
***********************************************************************************************************************
* @param u32AttributeId The identifier of the ADP IB attribute to read.
* @param u16AttributeIndex The index within the table of the specified IB attribute to read.
* @param pGetConfirm Get result.
**********************************************************************************************************************/
void AdpMacGetRequestSync(uint32_t u32AttributeId, uint16_t u16AttributeIndex,
		struct TAdpMacGetConfirm *pGetConfirm)
{
	uint8_t result = -1;

	LOG_IFACE_G3_ADP("AdpMacGetRequestSync attr = 0x%X - index = 0x%X\r\n", u32AttributeId, u16AttributeIndex);

	/* Set the sync flags to intercept the callback */
	g_adp_sync_mgmt.f_sync_req = true;
	g_adp_sync_mgmt.f_sync_res = false;
	g_adp_sync_mgmt.m_u32AttributeId = u32AttributeId; /* Used in order to know AttributeId requested on AdpMacGetConfirm callback */

	/* Send the asynchronous call */
	AdpMacGetRequest(u32AttributeId, u16AttributeIndex);

	/* Wait processing until flag activates, or timeout */
	addUsi_WaitProcessing(G3_SYNC_TIMEOUT, (Bool *)(&g_adp_sync_mgmt.f_sync_res));

	if (g_adp_sync_mgmt.f_sync_res) {
		/* Confirm received */
		/* pGetConfirm = &g_adp_sync_mgmt.s_MacGetConfirm; */
		memcpy(pGetConfirm, &g_adp_sync_mgmt.s_MacGetConfirm, sizeof(struct TAdpMacGetConfirm));
		result = 0;
	} else {
		/* Confirm not received */
		pGetConfirm->m_u8Status = G3_TIMEOUT;
		LOG_IFACE_G3_ADP("ERROR: Confirm for 0x%X not received\r\n", u32AttributeId);
	}

	g_adp_sync_mgmt.f_sync_req = false;
	g_adp_sync_mgmt.f_sync_res = false;

	LOG_IFACE_G3_ADP("AdpMacGetRequestSync result = %d\r\n", result);
}

/**********************************************************************************************************************/

/** The AdpMacGetConfirm primitive allows the upper layer to be notified of the completion of an AdpMacGetRequest.
 ***********************************************************************************************************************
 * @param m_u8Status The status of the scan request.
 * @param m_u32AttributeId The identifier of the IB attribute read
 * @param m_u16AttributeIndex The index within the table of the specified IB attribute read.
 * @param m_u8AttributeLength The length of the value of the attribute read from the IB.
 * @param m_au8AttributeValue The value of the attribute read from the IB.
 **********************************************************************************************************************/
typedef void (*AdpMacGetConfirm)(struct TAdpMacGetConfirm *pGetConfirm);

/**********************************************************************************************************************/

/** The AdpSetRequest primitive allows the upper layer to set the value of an attribute in the ADP information base.
 ***********************************************************************************************************************
 * @param u32AttributeId The identifier of the ADP IB attribute set
 * @param u16AttributeIndex The index within the table of the specified IB attribute.
 * @param u8AttributeLength The length of the value of the attribute to set
 * @param pu8AttributeValue The value of the attribute to set
 **********************************************************************************************************************/
void AdpSetRequest(uint32_t u32AttributeId, uint16_t u16AttributeIndex,
		uint8_t u8AttributeLength, const uint8_t *pu8AttributeValue)
{
	Uint8 *ptrBuff;
	Uint16 length;
	int result;
	uint8_t u8_aux;

	LOG_IFACE_G3_ADP("AdpSetRequest attribute id = 0x%X - index = %d - length %d\r\n", u32AttributeId, u16AttributeIndex, u8AttributeLength);

	ptrBuff = buffTxAdpG3;
	/* Filling Message Data of USI Frame */
	*ptrBuff++ = G3_SERIAL_MSG_ADP_SET_REQUEST;
	*ptrBuff++ = ((u32AttributeId >> 24) & 0xFF);
	*ptrBuff++ = ((u32AttributeId >> 16) & 0xFF);
	*ptrBuff++ = ((u32AttributeId >> 8) & 0xFF);
	*ptrBuff++ = (u32AttributeId & 0xFF);
	*ptrBuff++ = (u16AttributeIndex >> 8);
	*ptrBuff++ = (u16AttributeIndex & 0xFF);
	*ptrBuff++ = u8AttributeLength;

	switch (u32AttributeId) {
	case ADP_IB_SECURITY_LEVEL:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpSecurityLevel */
		break;

	case ADP_IB_PREFIX_TABLE:
		if (u8AttributeLength) { /* len = 0 => Reset Table */
			u8_aux = 0;
			uint8_t u8PrefixLength_bytes = u8AttributeLength - 11;
			*ptrBuff++ = pu8AttributeValue[u8_aux++]; /* m_u8PrefixLength */
			*ptrBuff++ = pu8AttributeValue[u8_aux++]; /* m_bOnLinkFlag */
			*ptrBuff++ = pu8AttributeValue[u8_aux++]; /* m_bAutonomousAddressConfigurationFlag */
			mem_copy_to_usi_endianness_uint32(ptrBuff, (uint8_t *)&pu8AttributeValue[u8_aux]);  /* u32ValidTime */
			u8_aux += 4;
			ptrBuff += 4;
			mem_copy_to_usi_endianness_uint32(ptrBuff, (uint8_t *)&pu8AttributeValue[u8_aux]);  /* u32PreferredTime */
			u8_aux += 4;
			ptrBuff += 4;
			memcpy((uint8_t *)&pu8AttributeValue[u8_aux], ptrBuff, u8PrefixLength_bytes); /* m_au8Prefix */
			u8_aux += u8PrefixLength_bytes;
			ptrBuff += u8PrefixLength_bytes;
		}

		break;

	case ADP_IB_BROADCAST_LOG_TABLE_ENTRY_TTL:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16AdpBroadcastLogTableEntryTTL */
		ptrBuff += 2;
		break;

	case ADP_IB_METRIC_TYPE:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpMetricType */
		break;

	case ADP_IB_LOW_LQI_VALUE:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpLowLQIValue */
		break;

	case ADP_IB_HIGH_LQI_VALUE:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpHighLQIValue */
		break;

	case ADP_IB_RREP_WAIT:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpRREPWait */
		break;

	case ADP_IB_CONTEXT_INFORMATION_TABLE:
		if (u8AttributeLength) { /* len = 0 => Reset Table */
			uint8_t u8ContextLength  = u8AttributeLength - 4;
			mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)(&pu8AttributeValue[0]));  /* u16ValidTime */
			ptrBuff += 2;
			*ptrBuff++ = pu8AttributeValue[2]; /* m_bValidForCompression */
			*ptrBuff++ = pu8AttributeValue[3]; /* m_u8BitsContextLength */
			memcpy(ptrBuff, &pu8AttributeValue[4], u8ContextLength); /* m_au8Context */
			ptrBuff += u8ContextLength;
		}

		break;

	case ADP_IB_COORD_SHORT_ADDRESS:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16AdpCoordShortAddress */
		ptrBuff += 2;
		break;

	case ADP_IB_RLC_ENABLED:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_bAdpRLCEnabled */
		break;

	case ADP_IB_ADD_REV_LINK_COST:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpAddRevLinkCost */
		break;

	case ADP_IB_BROADCAST_LOG_TABLE:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[0]);  /* m_u16SrcAddr */
		ptrBuff += 2;
		*ptrBuff++ = pu8AttributeValue[2]; /* m_u8SequenceNumber */
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[3]);  /* u16ValidTime */
		ptrBuff += 2;
		break;

	case ADP_IB_ROUTING_TABLE:
		if (u8AttributeLength) { /* len = 0 => Reset Table */
			mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[0]);  /* m_u16DstAddr */
			ptrBuff += 2;
			mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[2]);  /* m_u16NextHopAddr */
			ptrBuff += 2;
			mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[4]);  /* m_u16RouteCost */
			ptrBuff += 2;
			*ptrBuff++ = pu8AttributeValue[6]; /* m_u8HopCount || m_u8WeakLinkCount */
#ifdef G3_HYBRID_PROFILE
			*ptrBuff++ = pu8AttributeValue[7]; /* m_u8MediaType */
			mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[8]);  /* u16ValidTime */
			ptrBuff += 2;
#else
			mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[7]);  /* u16ValidTime */
			ptrBuff += 2;
#endif
		}
		break;

	case ADP_IB_UNICAST_RREQ_GEN_ENABLE:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_bAdpUnicastRREQGenEnable */
		break;

	case ADP_IB_GROUP_TABLE:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16GroupAddress */
		ptrBuff += 2;
		*ptrBuff++ = pu8AttributeValue[2];  /* m_bValid */
		break;

	case ADP_IB_MAX_HOPS:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpMaxHops */
		break;

	case ADP_IB_DEVICE_TYPE:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_eAdpDeviceType */
		break;

	case ADP_IB_NET_TRAVERSAL_TIME:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpNetTraversalTime */
		break;

	case ADP_IB_ROUTING_TABLE_ENTRY_TTL:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16AdpRoutingTableEntryTTL */
		ptrBuff += 2;
		break;

	case ADP_IB_KR:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpKr */
		break;

	case ADP_IB_KM:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpKm */
		break;

	case ADP_IB_KC:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpKc */
		break;

	case ADP_IB_KQ:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpKq */
		break;

	case ADP_IB_KH:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpKh */
		break;

	case ADP_IB_RREQ_RETRIES:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpRREQRetries */
		break;

	case ADP_IB_RREQ_RERR_WAIT: /* ADP_IB_RREQ_WAIT also enters here as it has the same numeric value */
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpRREQRERRWait */
		break;

	case ADP_IB_WEAK_LQI_VALUE:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpWeakLQIValue */
		break;

	case ADP_IB_KRT:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpKrt */
		break;

	case ADP_IB_SOFT_VERSION:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8Major */
		*ptrBuff++ = pu8AttributeValue[1]; /* m_u8Minor */
		*ptrBuff++ = pu8AttributeValue[2]; /* m_u8Revision */
		*ptrBuff++ = pu8AttributeValue[3]; /* m_u8Year */
		*ptrBuff++ = pu8AttributeValue[4]; /* m_u8Month */
		*ptrBuff++ = pu8AttributeValue[5]; /* m_u8Day */
		break;

	/*		case ADP_IB_SNIFFER_MODE: */
	/*			//TODO */
	/*			break; */

	case ADP_IB_BLACKLIST_TABLE:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[0]);  /* m_u16Addr */
		ptrBuff += 2;
#ifdef G3_HYBRID_PROFILE
		*ptrBuff++ = pu8AttributeValue[2]; /* m_u8MediaType */
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[3]);  /* u16ValidTime */
		ptrBuff += 2;
#else
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[2]);  /* u16ValidTime */
		ptrBuff += 2;
#endif
		break;

	case ADP_IB_BLACKLIST_TABLE_ENTRY_TTL:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16AdpBlacklistTableEntryTTL */
		ptrBuff += 2;
		break;

	case ADP_IB_MAX_JOIN_WAIT_TIME:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16AdpMaxJoinWaitTime */
		ptrBuff += 2;
		break;

	case ADP_IB_PATH_DISCOVERY_TIME:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpPathDiscoveryTime */
		break;

	case ADP_IB_ACTIVE_KEY_INDEX:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8AdpActiveKeyIndex */
		break;

	case ADP_IB_DESTINATION_ADDRESS_SET:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16Addr */
		ptrBuff += 2;
		break;

	case ADP_IB_DEFAULT_COORD_ROUTE_ENABLED:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_bAdpDefaultCoordRouteEnabled */
		break;

	case ADP_IB_DISABLE_DEFAULT_ROUTING:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_bAdpDisableDefaultRouting */
		break;

	/* manufacturer */
	case ADP_IB_MANUF_REASSEMBY_TIMER:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16AdpReassembyTimer */
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_IPV6_HEADER_COMPRESSION:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_bAdpIPv6HeaderCompression */
		break;

	case ADP_IB_MANUF_EAP_PRESHARED_KEY:
		memcpy(ptrBuff, pu8AttributeValue, u8AttributeLength); /* m_au8Value */
		ptrBuff += u8AttributeLength;
		break;

	case ADP_IB_MANUF_EAP_NETWORK_ACCESS_IDENTIFIER:
		memcpy(ptrBuff, pu8AttributeValue, u8AttributeLength); /* m_au8Value */
		ptrBuff += u8AttributeLength;
		break;

	case ADP_IB_MANUF_BROADCAST_SEQUENCE_NUMBER:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8BroadcastSequenceNumber */
		break;

	case ADP_IB_MANUF_REGISTER_DEVICE:
		memcpy(ptrBuff, (uint8_t *)&pu8AttributeValue[0], 8); /* m_EUI64Address */
		ptrBuff += 8;
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[8]);  /* m_u16PanId */
		ptrBuff += 2;
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[10]);  /* m_u16ShortAddress */
		ptrBuff += 2;
		*ptrBuff++ = pu8AttributeValue[12]; /* m_u8KeyIndex */
		memcpy(ptrBuff, (uint8_t *)&pu8AttributeValue[13], 16); /* m_au8GMK */
		ptrBuff += 16;
		break;

	case ADP_IB_MANUF_DATAGRAM_TAG:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16DatagramTag */
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_RANDP:
		memcpy(ptrBuff, pu8AttributeValue, u8AttributeLength); /* m_au8Value */
		ptrBuff += u8AttributeLength;
		break;

	case ADP_IB_MANUF_ROUTING_TABLE_COUNT:
		mem_copy_to_usi_endianness_uint32(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_au8RandP */
		ptrBuff += 4;
		break;

	case ADP_IB_MANUF_DISCOVER_SEQUENCE_NUMBER:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16DiscoverRouteGlobalSeqNo */
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_FORCED_NO_ACK_REQUEST:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8ForceNoAck */
		break;

	case ADP_IB_MANUF_LQI_TO_COORD:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8LQIToCoord */
		break;

	case ADP_IB_MANUF_BROADCAST_ROUTE_ALL:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_bBroadcastRouteAll */
		break;

	case ADP_IB_MANUF_KEEP_PARAMS_AFTER_KICK_LEAVE:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_bKeepParamsAfterKickLeave */
		break;

	case ADP_IB_MANUF_ADP_INTERNAL_VERSION:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8Major */
		*ptrBuff++ = pu8AttributeValue[1]; /* m_u8Minor */
		*ptrBuff++ = pu8AttributeValue[2]; /* m_u8Revision */
		*ptrBuff++ = pu8AttributeValue[3]; /* m_u8Year */
		*ptrBuff++ = pu8AttributeValue[4]; /* m_u8Month */
		*ptrBuff++ = pu8AttributeValue[5]; /* m_u8Day */
		break;

	case ADP_IB_MANUF_CIRCULAR_ROUTES_DETECTED:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16CircularRoutesDetected */
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_LAST_CIRCULAR_ROUTE_ADDRESS:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16CircularRoutesDetected */
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_MAX_REPAIR_RESEND_ATTEMPTS:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_u8MaxRepairReSendAttemps */
		break;

	case ADP_IB_MANUF_DISABLE_AUTO_RREQ:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_bDisableAutoRREQ */
		break;

	case ADP_IB_MANUF_ALL_NEIGHBORS_BLACKLISTED_COUNT:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16AllNeighborBlacklistedCount */
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_QUEUED_ENTRIES_REMOVED_TIMEOUT_COUNT:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16QueuedEntriesRemovedTimeoutCount */
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_QUEUED_ENTRIES_REMOVED_ROUTE_ERROR_COUNT:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16QueuedEntriesRemovedRouteErrorCount */
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_PENDING_DATA_IND_SHORT_ADDRESS:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_PendingDataIndication.m_DstDeviceAddress.m_u16ShortAddr */
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_GET_BAND_CONTEXT_TONES:
		*ptrBuff++ = pu8AttributeValue[0]; /* m_BandContext.m_u8Tones */
		break;

	case ADP_IB_MANUF_UPDATE_NON_VOLATILE_DATA:
		*ptrBuff++ = pu8AttributeValue[0]; /*  m_bUpdNonVolatileData */
		break;

	case ADP_IB_MANUF_DISCOVER_ROUTE_GLOBAL_SEQ_NUM:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16DiscoverRouteGlobalSeqNo */
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_FRAGMENT_DELAY:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16AdpFragmentDelay */
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_DYNAMIC_FRAGMENT_DELAY_ENABLED:
		*ptrBuff++ = pu8AttributeValue[0]; /*  m_bAdpDynamicFragmentDelayEnabled */
		break;

	case ADP_IB_MANUF_DYNAMIC_FRAGMENT_DELAY_FACTOR:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);  /* m_u16AdpDynamicFragmentDelayFactor */
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_BLACKLIST_TABLE_COUNT:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_BROADCAST_LOG_TABLE_COUNT:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_CONTEXT_INFORMATION_TABLE_COUNT:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_GROUP_TABLE_COUNT:
		mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 2;
		break;

	case ADP_IB_MANUF_ROUTING_TABLE_ELEMENT:
		if (u8AttributeLength) { /* len = 0 => Reset Table */
			mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[0]);  /* m_u16DstAddr */
			ptrBuff += 2;
			mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[2]);  /* m_u16NextHopAddr */
			ptrBuff += 2;
			mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[4]);  /* m_u16RouteCost */
			ptrBuff += 2;
			*ptrBuff++ = pu8AttributeValue[6]; /* m_u8HopCount || m_u8WeakLinkCount */
#ifdef G3_HYBRID_PROFILE
			*ptrBuff++ = pu8AttributeValue[7]; /* m_u8MediaType */
			mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[8]);  /* u16ValidTime */
			ptrBuff += 2;
#else
			mem_copy_to_usi_endianness_uint16(ptrBuff, (uint8_t *)&pu8AttributeValue[7]);  /* u16ValidTime */
			ptrBuff += 2;
#endif
		}
		break;

	case ADP_IB_MANUF_SET_PHASEDIFF_PREQ_PREP:
		*ptrBuff++ = pu8AttributeValue[0]; /*  m_bSetPhaseDiffPREQPREP */
		break;

	case ADP_IB_MANUF_HYBRID_PROFILE:
		*ptrBuff++ = pu8AttributeValue[0]; /*  m_bAdpHybridProfile */
		break;
		
#ifdef G3_HYBRID_PROFILE
	case ADP_IB_LOW_LQI_VALUE_RF:
		*ptrBuff++ = pu8AttributeValue[0]; /*  m_u8AdpLowLQIValueRF */
		break;

	case ADP_IB_HIGH_LQI_VALUE_RF:
		*ptrBuff++ = pu8AttributeValue[0]; /*  m_u8AdpHighLQIValueRF */
		break;
		
	case ADP_IB_KQ_RF:
		*ptrBuff++ = pu8AttributeValue[0]; /*  m_u8AdpKqRF */
		break;
		
	case ADP_IB_KH_RF:
		*ptrBuff++ = pu8AttributeValue[0]; /*  m_u8AdpKhRF */
		break;
		
	case ADP_IB_KRT_RF:
		*ptrBuff++ = pu8AttributeValue[0]; /*  m_u8AdpKrtRF */
		break;
		
	case ADP_IB_KDC_RF:
		*ptrBuff++ = pu8AttributeValue[0]; /*  m_u8AdpKdcRF */
		break;
		
	case ADP_IB_USE_BACKUP_MEDIA:
		*ptrBuff++ = pu8AttributeValue[0]; /*  m_bAdpUseBackupMedia */
		break;
#endif

	default:
		/* Unknown PIB, copy the value as it is. */
		memcpy(ptrBuff, pu8AttributeValue, u8AttributeLength);
		ptrBuff += u8AttributeLength;
	}

	/* Message Data Length of USI Frame */
	length = ptrBuff - buffTxAdpG3;
	/* Filling USI Frame Structure */
	adpG3Msg.pType = PROTOCOL_ADP_G3;
	adpG3Msg.buf = buffTxAdpG3;
	adpG3Msg.len = length;
	/* Send USI Frame */
	result = usi_SendCmd(&adpG3Msg) ? 0 : -1;
	LOG_IFACE_G3_ADP("AdpSetRequest result = %d\r\n", result);
}

/**********************************************************************************************************************/

/** The AdpSetConfirm primitive allows the upper layer to be notified of the completion of an AdpSetRequest.
 ***********************************************************************************************************************
 * @param m_u8Status The status of the scan request.
 * @param m_u32AttributeId The identifier of the IB attribute set.
 * @param m_u16AttributeIndex The index within the table of the specified IB attribute.
 **********************************************************************************************************************/
typedef void (*AdpSetConfirm)(struct TAdpSetConfirm *pSetConfirm);

/**********************************************************************************************************************/

/** The AdpSetRequestSync primitive allows the upper layer to set the value of an attribute in the ADP information base
* synchronously
***********************************************************************************************************************
* @param u32AttributeId The identifier of the ADP IB attribute set
* @param u16AttributeIndex The index within the table of the specified IB attribute.
* @param u8AttributeLength The length of the value of the attribute to set
* @param pu8AttributeValue The value of the attribute to set
* @param pSetConfirm The set confirm
**********************************************************************************************************************/
void AdpSetRequestSync(uint32_t u32AttributeId, uint16_t u16AttributeIndex,
		uint8_t u8AttributeLength, const uint8_t *pu8AttributeValue,
		struct TAdpSetConfirm *pSetConfirm)
{
	uint8_t result = -1;

	LOG_IFACE_G3_ADP("AdpSetRequestSync attr = 0x%X - index = 0x%X\r\n", u32AttributeId, u16AttributeIndex);

	/* Set the sync flags to intercept the callback */
	g_adp_sync_mgmt.f_sync_req = true;
	g_adp_sync_mgmt.m_u32AttributeId = u32AttributeId; /* Used in order to know AttributeId requested on AdpSetConfirm callback */
	g_adp_sync_mgmt.f_sync_res = false;

	/* Send the asynchronous call */
	AdpSetRequest(u32AttributeId, u16AttributeIndex, u8AttributeLength, pu8AttributeValue);

	/* Wait processing until flag activates, or timeout */
	addUsi_WaitProcessing(G3_SYNC_TIMEOUT, (Bool *)(&g_adp_sync_mgmt.f_sync_res));

	if (g_adp_sync_mgmt.f_sync_res) {
		/* Confirm received */
		/* pSetConfirm = &g_adp_sync_mgmt.s_SetConfirm; */
		memcpy(pSetConfirm, &g_adp_sync_mgmt.s_SetConfirm, sizeof(struct TAdpSetConfirm));
		result = 0;
	} else {
		/* Confirm not received */
		pSetConfirm->m_u8Status = G3_TIMEOUT;
		LOG_IFACE_G3_ADP("ERROR: Confirm not received\r\n");
	}

	g_adp_sync_mgmt.f_sync_req = false;
	g_adp_sync_mgmt.f_sync_res = false;

	LOG_IFACE_G3_ADP("AdpSetRequestSync result = %u\r\n", result);
}

/**********************************************************************************************************************/

/* The AdpMacSetRequest primitive allows the upper layer to set the value of an attribute in the MAC information base.
 * The upper layer cannot access directly the MAC layer while ADP is running
 ***********************************************************************************************************************
 * @param u32AttributeId The identifier of the MAC IB attribute set
 * @param u16AttributeIndex The index within the table of the specified IB attribute.
 * @param u8AttributeLength The length of the value of the attribute to set
 * @param pu8AttributeValue The value of the attribute to set
 **********************************************************************************************************************/
void AdpMacSetRequest(uint32_t u32AttributeId, uint16_t u16AttributeIndex,
		uint8_t u8AttributeLength, const uint8_t *pu8AttributeValue)
{
	Uint8 *ptrBuff;
	Uint16 length;
	int result;
	uint8_t u8_aux;

	LOG_IFACE_G3_ADP("AdpMacSetRequest attribute id = 0x%02x - index = %u - length %u\r\n", u32AttributeId, u16AttributeIndex, u8AttributeLength);

	ptrBuff = buffTxAdpG3;
	/* Filling Message Data of USI Frame */
	*ptrBuff++ = G3_SERIAL_MSG_ADP_MAC_SET_REQUEST;
	*ptrBuff++ = ((u32AttributeId >> 24) & 0xFF);
	*ptrBuff++ = ((u32AttributeId >> 16) & 0xFF);
	*ptrBuff++ = ((u32AttributeId >> 8) & 0xFF);
	*ptrBuff++ = (u32AttributeId & 0xFF);
	*ptrBuff++ = (u16AttributeIndex >> 8);
	*ptrBuff++ = (u16AttributeIndex & 0xFF);
	*ptrBuff++ = u8AttributeLength;

	/* In attributes whose pu8AttributeValue contains a struct with bit fields, */
	/* that bit fields use a whole bit in the attrib value of the serial interface. */
	/* The attrib. value must be adapted: */
	switch (u32AttributeId) {
	case MAC_WRP_PIB_ACK_WAIT_DURATION:
		/* u16AckWaitDuration */
		mem_copy_to_usi_endianness_uint16((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 2;
		break;

	case MAC_WRP_PIB_BSN:
		/* m_u8Bsn */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_DSN:
		/* m_u8Dsn */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MAX_BE:
		/* m_u8MaxBe */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MAX_CSMA_BACKOFFS:
		/* m_u8MaxCsmaBackoffs */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MAX_FRAME_RETRIES:
		/* m_u8MaxFrameRetries */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MIN_BE:
		/* m_u8MinBe */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_PAN_ID:
		/* m_nPanId */
		mem_copy_to_usi_endianness_uint16((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 2;
		break;

	case MAC_WRP_PIB_SECURITY_ENABLED:
		/* Boolean value */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_SHORT_ADDRESS:
		/* m_nShortAddress */
		mem_copy_to_usi_endianness_uint16((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 2;
		break;

	case MAC_WRP_PIB_PROMISCUOUS_MODE:
		/* m_bPromiscuousMode */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_TIMESTAMP_SUPPORTED:
		/* Boolean value */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_KEY_TABLE:
		memcpy((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue, MAC_WRP_SECURITY_KEY_LENGTH);
		ptrBuff += MAC_WRP_SECURITY_KEY_LENGTH;
		break;

	case MAC_WRP_PIB_FRAME_COUNTER:
		/* m_au32FrameCounter */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_HIGH_PRIORITY_WINDOW_SIZE:
		/* Boolean value */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_TX_DATA_PACKET_COUNT:
		/* m_u32TxDataPacketCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_RX_DATA_PACKET_COUNT:
		/* m_u32RxDataPacketCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_TX_CMD_PACKET_COUNT:
		/* m_u32TxCmdPacketCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_RX_CMD_PACKET_COUNT:
		/* m_u32RxCmdPacketCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_CSMA_FAIL_COUNT:
		/* m_u32CsmaFailCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_CSMA_NO_ACK_COUNT:
		/* m_u32CsmaNoAckCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_RX_DATA_BROADCAST_COUNT:
		/* m_u32TxDataBroadcastCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_TX_DATA_BROADCAST_COUNT:
		/* m_u32RxDataBroadcastCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_BAD_CRC_COUNT:
		/* m_u32BadCrcCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_NEIGHBOUR_TABLE:
		if (u8AttributeLength == 16) {
			*(ptrBuff - 1) = 18;         /* Increased length to consider bytes instead of bit fields. */
			struct TMacWrpNeighbourEntry *pNeighbourEntry = (struct TMacWrpNeighbourEntry *)(pu8AttributeValue);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_nShortAddress >> 8);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_nShortAddress & 0xFF);
			memcpy((uint8_t *)ptrBuff, (uint8_t *)&pNeighbourEntry->m_ToneMap.m_au8Tm[0], (MAC_WRP_MAX_TONE_GROUPS + 7) / 8);
			ptrBuff += (MAC_WRP_MAX_TONE_GROUPS + 7) / 8;
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_nModulationType);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_nTxGain);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_nTxRes);
			memcpy((uint8_t *)ptrBuff, (uint8_t *)&pNeighbourEntry->m_TxCoef.m_au8TxCoef[0], 6);
			ptrBuff += 6;
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_nModulationScheme);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_nPhaseDifferential);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_u8Lqi);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_u16TmrValidTime >> 8);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_u16TmrValidTime & 0xFF);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_u16NeighbourValidTime >> 8);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_u16NeighbourValidTime & 0xFF);
		} else {
			LOG_IFACE_G3_ADP("AdpMacSetRequest ERROR u8AttributeLength = %u does not match TMacWrpNeighbourEntry size (%zu)\r\n", u8AttributeLength, sizeof(struct TMacWrpNeighbourEntry));
			return;
		}

		break;

	case MAC_WRP_PIB_FREQ_NOTCHING:
		/* m_bFreqNotching */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_POS_TABLE:
		if (u8AttributeLength == 6) {
			*(ptrBuff - 1) = 5;         /* Decreased length */
			struct TMacWrpPOSEntry *pPOSEntry = (struct TMacWrpPOSEntry *)(pu8AttributeValue);
			*ptrBuff++ = (uint8_t)(pPOSEntry->m_nShortAddress >> 8);
			*ptrBuff++ = (uint8_t)(pPOSEntry->m_nShortAddress & 0xFF);
			*ptrBuff++ = (uint8_t)(pPOSEntry->m_u8Lqi);
			*ptrBuff++ = (uint8_t)(pPOSEntry->m_u16POSValidTime >> 8);
			*ptrBuff++ = (uint8_t)(pPOSEntry->m_u16POSValidTime & 0xFF);
		} else {
			LOG_IFACE_G3_ADP("AdpMacSetRequest ERROR u8AttributeLength = %u does not match TMacWrpPOSEntry size (%zu)\r\n", u8AttributeLength, sizeof(struct TMacWrpPOSEntry));
			return;
		}

		break;

	case MAC_WRP_PIB_CSMA_FAIRNESS_LIMIT:
		/* m_u8CsmaFairnessLimit */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_TMR_TTL:
		/* m_u8TmrTtl */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_POS_TABLE_ENTRY_TTL:         /* MAC_WRP_PIB_NEIGHBOUR_TABLE_ENTRY_TTL also enters here as it has the same numeric value */
		/* m_u8NeighbourTableEntryTtl */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_RC_COORD:
		/* m_u16RcCoord */
		mem_copy_to_usi_endianness_uint16((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 2;
		break;

	case MAC_WRP_PIB_TONE_MASK:
		memcpy((uint8_t *)ptrBuff, (uint8_t *)&pu8AttributeValue[0], (MAC_WRP_MAX_TONES + 7) / 8);
		ptrBuff += (MAC_WRP_MAX_TONES + 7) / 8;
		break;

	case MAC_WRP_PIB_BEACON_RANDOMIZATION_WINDOW_LENGTH:
		/* m_u8BeaconRandomizationWindowLength */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_A:
		/* m_u8A */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_K:
		/* m_u8K */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MIN_CW_ATTEMPTS:
		/* m_u8MinCwAttempts */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_CENELEC_LEGACY_MODE:
		/* PhyBoolGetLegacyMode */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_FCC_LEGACY_MODE:
		/* PhyBoolGetLegacyMode */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_BROADCAST_MAX_CW_ENABLE:
		*ptrBuff++ = pu8AttributeValue[0];         /* m_bBroadcastMaxCwEnable */
		break;

	case MAC_WRP_PIB_MANUF_DEVICE_TABLE:
		u8_aux = 0;
		/* m_nPanId */
		mem_copy_to_usi_endianness_uint16((uint8_t *)&ptrBuff[u8_aux], (uint8_t *)&pu8AttributeValue[0]);
		u8_aux += 2;
		/* m_nShortAddress */
		mem_copy_to_usi_endianness_uint16((uint8_t *)&ptrBuff[u8_aux], (uint8_t *)&pu8AttributeValue[2]);
		u8_aux += 2;
		/* m_au32FrameCounter */
		mem_copy_to_usi_endianness_uint32((uint8_t *)&ptrBuff[u8_aux], (uint8_t *)&pu8AttributeValue[4]);
		u8_aux += 4;
		ptrBuff += u8_aux;
		break;

	case MAC_WRP_PIB_MANUF_EXTENDED_ADDRESS:
		/* m_au8Address */
		memcpy((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue, 8);
		ptrBuff += 8;
		break;

	case MAC_WRP_PIB_MANUF_NEIGHBOUR_TABLE_ELEMENT:
		if (u8AttributeLength == 16) {
			*(ptrBuff - 1) = 18;         /* Increased length to consider bytes instead of bit fields. */
			struct TMacWrpNeighbourEntry *pNeighbourEntry = (struct TMacWrpNeighbourEntry *)(pu8AttributeValue);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_nShortAddress >> 8);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_nShortAddress & 0xFF);
			memcpy((uint8_t *)ptrBuff, (uint8_t *)&pNeighbourEntry->m_ToneMap.m_au8Tm[0], (MAC_WRP_MAX_TONE_GROUPS + 7) / 8);
			ptrBuff += (MAC_WRP_MAX_TONE_GROUPS + 7) / 8;
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_nModulationType);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_nTxGain);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_nTxRes);
			memcpy((uint8_t *)ptrBuff, (uint8_t *)&pNeighbourEntry->m_TxCoef.m_au8TxCoef[0], 6);
			ptrBuff += 6;
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_nModulationScheme);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_nPhaseDifferential);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_u8Lqi);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_u16TmrValidTime >> 8);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_u16TmrValidTime & 0xFF);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_u16NeighbourValidTime >> 8);
			*ptrBuff++ = (uint8_t)(pNeighbourEntry->m_u16NeighbourValidTime & 0xFF);
		} else {
			LOG_IFACE_G3_ADP("AdpMacSetRequest ERROR u8AttributeLength = %u does not match TMacWrpNeighbourEntry size (%zu)\r\n", u8AttributeLength, sizeof(struct TMacWrpNeighbourEntry));
			return;
		}

		break;

	case MAC_WRP_PIB_MANUF_BAND_INFORMATION:
		u8_aux = 0;
		/* m_u16FlMax */
		mem_copy_to_usi_endianness_uint16(&ptrBuff[u8_aux], (uint8_t *)&pu8AttributeValue[0]);
		u8_aux += 2;
		/* m_u8Band */
		ptrBuff[u8_aux++] = pu8AttributeValue[2];
		/* m_u8Tones */
		ptrBuff[u8_aux++] = pu8AttributeValue[3];
		/* m_u8Carriers */
		ptrBuff[u8_aux++] = pu8AttributeValue[4];
		/* m_u8TonesInCarrier */
		ptrBuff[u8_aux++] = pu8AttributeValue[5];
		/* m_u8FlBand */
		ptrBuff[u8_aux++] = pu8AttributeValue[6];
		/* m_u8MaxRsBlocks */
		ptrBuff[u8_aux++] = pu8AttributeValue[7];
		/* m_u8TxCoefBits */
		ptrBuff[u8_aux++] = pu8AttributeValue[8];
		/* m_u8PilotsFreqSpa */
		ptrBuff[u8_aux++] = pu8AttributeValue[9];
		ptrBuff += u8_aux;
		break;

	case MAC_WRP_PIB_MANUF_COORD_SHORT_ADDRESS:
		/* m_nCoordShortAddress */
		mem_copy_to_usi_endianness_uint16((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 2;
		break;

	case MAC_WRP_PIB_MANUF_MAX_MAC_PAYLOAD_SIZE:
		/* u16MaxMacPayloadSize */
		mem_copy_to_usi_endianness_uint16((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 2;
		break;

	case MAC_WRP_PIB_MANUF_SECURITY_RESET:
		/* Response will be mac_status_denied */
		break;

	case MAC_WRP_PIB_MANUF_FORCED_MOD_SCHEME:
		/* m_u8ForcedModScheme */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MANUF_FORCED_MOD_TYPE:
		/* m_u8ForcedModScheme */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MANUF_FORCED_TONEMAP:
		*ptrBuff++ = pu8AttributeValue[0];
		*ptrBuff++ = pu8AttributeValue[1];
		*ptrBuff++ = pu8AttributeValue[2];
		break;

	case MAC_WRP_PIB_MANUF_FORCED_MOD_SCHEME_ON_TMRESPONSE:
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MANUF_FORCED_MOD_TYPE_ON_TMRESPONSE:
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MANUF_FORCED_TONEMAP_ON_TMRESPONSE:
		*ptrBuff++ = pu8AttributeValue[0];
		*ptrBuff++ = pu8AttributeValue[1];
		*ptrBuff++ = pu8AttributeValue[2];
		break;

	case MAC_WRP_PIB_MANUF_LAST_RX_MOD_SCHEME:
		/* m_LastRxModScheme */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MANUF_LAST_RX_MOD_TYPE:
		/* m_LastRxModType */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MANUF_LBP_FRAME_RECEIVED:
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MANUF_LNG_FRAME_RECEIVED:
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MANUF_BCN_FRAME_RECEIVED:
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MANUF_NEIGHBOUR_TABLE_COUNT:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_MANUF_RX_OTHER_DESTINATION_COUNT:
		/* m_u32RxOtherDestinationCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_INVALID_FRAME_LENGTH_COUNT:
		/* m_u32RxInvalidFrameLengthCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_MAC_REPETITION_COUNT:
		/* m_u32RxMACRepetitionCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_WRONG_ADDR_MODE_COUNT:
		/* m_u32RxWrongAddrModeCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_UNSUPPORTED_SECURITY_COUNT:
		/* m_u32RxUnsupportedSecurityCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_WRONG_KEY_ID_COUNT:
		/* m_u32RxWrongKeyIdCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_INVALID_KEY_COUNT:
		/* m_u32RxInvalidKeyCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_WRONG_FC_COUNT:
		/* m_u32RxWrongFCCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_DECRYPTION_ERROR_COUNT:
		/* m_u32RxDecryptionErrorCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_SEGMENT_DECODE_ERROR_COUNT:
		/* m_u32RxSegmentDecodeErrorCount */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;

	case MAC_WRP_PIB_MANUF_ENABLE_MAC_SNIFFER:
		/* m_bMacSniffer */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MANUF_POS_TABLE_COUNT:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_MANUF_RETRIES_LEFT_TO_FORCE_ROBO:
		/* m_u8RetriesToForceRobo */
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MANUF_MAC_INTERNAL_VERSION:
		/* Version */
		*ptrBuff++ = pu8AttributeValue[0];         /* m_u8Major */
		*ptrBuff++ = pu8AttributeValue[1];         /* m_u8Minor */
		*ptrBuff++ = pu8AttributeValue[2];         /* m_u8Revision */
		*ptrBuff++ = pu8AttributeValue[3];         /* m_u8Year */
		*ptrBuff++ = pu8AttributeValue[4];         /* m_u8Month */
		*ptrBuff++ = pu8AttributeValue[5];         /* m_u8Day */
		break;

	case MAC_WRP_PIB_MANUF_MAC_RT_INTERNAL_VERSION:
		/* Version */
		*ptrBuff++ = pu8AttributeValue[0];         /* m_u8Major */
		*ptrBuff++ = pu8AttributeValue[1];         /* m_u8Minor */
		*ptrBuff++ = pu8AttributeValue[2];         /* m_u8Revision */
		*ptrBuff++ = pu8AttributeValue[3];         /* m_u8Year */
		*ptrBuff++ = pu8AttributeValue[4];         /* m_u8Month */
		*ptrBuff++ = pu8AttributeValue[5];         /* m_u8Day */
		break;

	case MAC_WRP_PIB_MANUF_RESET_MAC_STATS:
		/* Response will be mac_status_denied */
		break;

	case MAC_WRP_PIB_MANUF_SLEEP_MODE:
		*ptrBuff++ = pu8AttributeValue[0];
		break;

	case MAC_WRP_PIB_MANUF_DEBUG_SET:
		*ptrBuff++ = pu8AttributeValue[0];
		*ptrBuff++ = pu8AttributeValue[1];
		*ptrBuff++ = pu8AttributeValue[2];
		*ptrBuff++ = pu8AttributeValue[3];
		*ptrBuff++ = pu8AttributeValue[4];
		*ptrBuff++ = pu8AttributeValue[5];
		*ptrBuff++ = pu8AttributeValue[6];
		break;

	case MAC_WRP_PIB_MANUF_DEBUG_READ:
		memcpy(ptrBuff, pu8AttributeValue, u8AttributeLength);
		ptrBuff += u8AttributeLength;
		break;

	case MAC_WRP_PIB_MANUF_PHY_PARAM:
		switch (u16AttributeIndex) {
		case MAC_WRP_PHY_PARAM_VERSION:
		case MAC_WRP_PHY_PARAM_TX_TOTAL:
		case MAC_WRP_PHY_PARAM_TX_TOTAL_BYTES:
		case MAC_WRP_PHY_PARAM_TX_TOTAL_ERRORS:
		case MAC_WRP_PHY_PARAM_BAD_BUSY_TX:
		case MAC_WRP_PHY_PARAM_TX_BAD_BUSY_CHANNEL:
		case MAC_WRP_PHY_PARAM_TX_BAD_LEN:
		case MAC_WRP_PHY_PARAM_TX_BAD_FORMAT:
		case MAC_WRP_PHY_PARAM_TX_TIMEOUT:
		case MAC_WRP_PHY_PARAM_RX_TOTAL:
		case MAC_WRP_PHY_PARAM_RX_TOTAL_BYTES:
		case MAC_WRP_PHY_PARAM_RX_RS_ERRORS:
		case MAC_WRP_PHY_PARAM_RX_EXCEPTIONS:
		case MAC_WRP_PHY_PARAM_RX_BAD_LEN:
		case MAC_WRP_PHY_PARAM_RX_BAD_CRC_FCH:
		case MAC_WRP_PHY_PARAM_RX_FALSE_POSITIVE:
		case MAC_WRP_PHY_PARAM_RX_BAD_FORMAT:
			mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
			ptrBuff += 4;
			break;

		case MAC_WRP_PHY_PARAM_ENABLE_AUTO_NOISE_CAPTURE:
			*ptrBuff++ = pu8AttributeValue[0];
			break;

		case MAC_WRP_PHY_PARAM_TIME_BETWEEN_NOISE_CAPTURES:
			mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
			ptrBuff += 4;
			break;

		case MAC_WRP_PHY_PARAM_DELAY_NOISE_CAPTURE_AFTER_RX:
			*ptrBuff++ = pu8AttributeValue[0];
			break;

		case MAC_WRP_PHY_PARAM_CFG_AUTODETECT_BRANCH:
			*ptrBuff++ = pu8AttributeValue[0];
			break;

		case MAC_WRP_PHY_PARAM_CFG_IMPEDANCE:
			*ptrBuff++ = pu8AttributeValue[0];
			break;

		case MAC_WRP_PHY_PARAM_RRC_NOTCH_ACTIVE:
			*ptrBuff++ = pu8AttributeValue[0];
			break;

		case MAC_WRP_PHY_PARAM_RRC_NOTCH_INDEX:
			*ptrBuff++ = pu8AttributeValue[0];
			break;

		case MAC_WRP_PHY_PARAM_PLC_DISABLE:
			*ptrBuff++ = pu8AttributeValue[0];
			break;

		case MAC_WRP_PHY_PARAM_NOISE_PEAK_POWER:
			*ptrBuff++ = pu8AttributeValue[0];
			break;

		case MAC_WRP_PHY_PARAM_LAST_MSG_LQI:
			*ptrBuff++ = pu8AttributeValue[0];
			break;

		case MAC_WRP_PHY_PARAM_LAST_MSG_RSSI:
			mem_copy_to_usi_endianness_uint16((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
			break;

		case MAC_WRP_PHY_PARAM_ACK_TX_CFM:
			mem_copy_to_usi_endianness_uint16((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
			break;

		case MAC_WRP_PHY_PARAM_TONE_MAP_RSP_ENABLED_MODS:
			*ptrBuff++ = pu8AttributeValue[0];
			break;

		case MAC_WRP_PHY_PARAM_RESET_PHY_STATS:
			break;

		}
		break;

#ifdef G3_HYBRID_PROFILE
	case MAC_WRP_PIB_DSN_RF:
		/* m_u8DsnRF */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	case MAC_WRP_PIB_MAX_BE_RF:
		/* m_u8MaxBeRF */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	case MAC_WRP_PIB_MAX_CSMA_BACKOFFS_RF:
		/* m_u8MaxCsmaBackoffsRF */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	case MAC_WRP_PIB_MAX_FRAME_RETRIES_RF:
		/* m_u8MaxFrameRetriesRF */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	case MAC_WRP_PIB_MIN_BE_RF:
		/* m_u8MinBeRF */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	case MAC_WRP_PIB_TIMESTAMP_SUPPORTED_RF:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;
	case MAC_WRP_PIB_DEVICE_TABLE_RF:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;
	case MAC_WRP_PIB_FRAME_COUNTER_RF:
		/* m_u32FrameCounterRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_DUPLICATE_DETECTION_TTL_RF:
		/* m_u8DuplicateDetectionTtlRF */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	case MAC_WRP_PIB_COUNTER_OCTETS_RF:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;
	case MAC_WRP_PIB_RETRY_COUNT_RF:
		/* m_u32RetryCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MULTIPLE_RETRY_COUNT_RF:
		/* m_u32MultipleRetryCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_TX_FAIL_COUNT_RF:
		/* m_u32TxFailCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_TX_SUCCESS_COUNT_RF:
		/* m_u32TxSuccessCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_FCS_ERROR_COUNT_RF:
		/* m_u32FcsErrorCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_SECURITY_FAILURE_COUNT_RF:
		/* m_u32SecurityFailureCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_DUPLICATE_FRAME_COUNT_RF:
		/* m_u32DuplicateFrameCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_RX_SUCCESS_COUNT_RF:
		/* m_u32RxSuccessCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_NACK_COUNT_RF:
		/* m_u32NackCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_USE_ENHANCED_BEACON_RF:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;
	case MAC_WRP_PIB_EB_HEADER_IE_LIST_RF:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;
	case MAC_WRP_PIB_EB_PAYLOAD_IE_LIST_RF:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;
	case MAC_WRP_PIB_EB_FILTERING_ENABLED_RF:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;
	case MAC_WRP_PIB_EBSN_RF:
		/* m_u8EBsnRF */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	case MAC_WRP_PIB_EB_AUTO_SA_RF:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;
	case MAC_WRP_PIB_SEC_SECURITY_LEVEL_LIST_RF:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;
	case MAC_WRP_PIB_POS_TABLE_RF: /* 9 Byte entries. */
		if (u8AttributeLength == 10) {
			*(ptrBuff - 1) = 9;         /* Decreased length */
			struct TMacWrpPOSEntryRF *pPOSEntry = (struct TMacWrpPOSEntryRF *)(pu8AttributeValue);
			*ptrBuff++ = (uint8_t)(pPOSEntry->m_nShortAddress >> 8);
			*ptrBuff++ = (uint8_t)(pPOSEntry->m_nShortAddress & 0xFF);
			*ptrBuff++ = (uint8_t)(pPOSEntry->m_u8ForwardLqi);
			*ptrBuff++ = (uint8_t)(pPOSEntry->m_u8ReverseLqi);
			*ptrBuff++ = (uint8_t)(pPOSEntry->m_u8DutyCycle);
			*ptrBuff++ = (uint8_t)(pPOSEntry->m_u8ForwardTxPowerOffset);
			*ptrBuff++ = (uint8_t)(pPOSEntry->m_u8ReverseTxPowerOffset);
			*ptrBuff++ = (uint8_t)(pPOSEntry->m_u16POSValidTime >> 8);
			*ptrBuff++ = (uint8_t)(pPOSEntry->m_u16POSValidTime & 0xFF);
		} else {
			LOG_IFACE_G3_ADP("AdpMacSetRequest ERROR u8AttributeLength = %u does not match TMacWrpPOSEntry size (%zu)\r\n", u8AttributeLength, sizeof(struct TMacWrpPOSEntry));
			return;
		}
		break;
	case MAC_WRP_PIB_OPERATING_MODE_RF:
		/* m_u8OperatingModeRF */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	case MAC_WRP_PIB_CHANNEL_NUMBER_RF:
		/* m_u16ChannelNumberRF */
		mem_copy_to_usi_endianness_uint16((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 2;
		break;
	case MAC_WRP_PIB_DUTY_CYCLE_USAGE_RF:
		/* m_u8DutyCycleUsageRF */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	case MAC_WRP_PIB_DUTY_CYCLE_PERIOD_RF:
		/* m_u16DutyCyclePeriodRF */
		mem_copy_to_usi_endianness_uint16((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 2;
		break;
	case MAC_WRP_PIB_DUTY_CYCLE_LIMIT_RF:
		/* m_u16DutyCycleLimitRF */
		mem_copy_to_usi_endianness_uint16((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 2;
		break;
	case MAC_WRP_PIB_DUTY_CYCLE_THRESHOLD_RF:
		/* m_u8DutyCycleThresholdRF */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	case MAC_WRP_PIB_DISABLE_PHY_RF:
		/* m_bDisablePhyRF */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	/* Manufacturer specific */
	case MAC_WRP_PIB_MANUF_SECURITY_RESET_RF:
		/*  8 bits (bool) */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	case MAC_WRP_PIB_MANUF_LBP_FRAME_RECEIVED_RF:
		/* m_bLBPFrameReceivedRF */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	case MAC_WRP_PIB_MANUF_LNG_FRAME_RECEIVED_RF:
		/* m_bLNGFrameReceivedRF */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	case MAC_WRP_PIB_MANUF_BCN_FRAME_RECEIVED_RF:
		/* m_bBCNFrameReceivedRF */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	case MAC_WRP_PIB_MANUF_RX_OTHER_DESTINATION_COUNT_RF:
		/* m_u32RxOtherDestinationCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_RX_INVALID_FRAME_LENGTH_COUNT_RF:
		/* m_u32RxInvalidFrameLengthCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_RX_WRONG_ADDR_MODE_COUNT_RF:
		/* m_u32RxWrongAddrModeCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_RX_UNSUPPORTED_SECURITY_COUNT_RF:
		/* m_u32RxUnsupportedSecurityCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_RX_WRONG_KEY_ID_COUNT_RF:
		/* m_u32RxWrongKeyIdCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_RX_INVALID_KEY_COUNT_RF:
		/* m_u32RxInvalidKeyCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_RX_WRONG_FC_COUNT_RF:
		/* m_u32RxWrongFCCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_RX_DECRYPTION_ERROR_COUNT_RF:
		/* m_u32RxDecryptionErrorCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_TX_DATA_PACKET_COUNT_RF:
		/* m_u32TxDataPacketCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_RX_DATA_PACKET_COUNT_RF:
		/* m_u32RxDataPacketCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_TX_CMD_PACKET_COUNT_RF:
		/* m_u32TxCmdPacketCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_RX_CMD_PACKET_COUNT_RF:
		/* m_u32RxCmdPacketCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_CSMA_FAIL_COUNT_RF:
		/* m_u32CsmaFailCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_RX_DATA_BROADCAST_COUNT_RF:
		/* m_u32RxDataBroadcastCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_TX_DATA_BROADCAST_COUNT_RF:
		/* m_u32TxDataBroadcastCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_BAD_CRC_COUNT_RF:
		/* m_u32BadCrcCountRF */
		mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 4;
		break;
	case MAC_WRP_PIB_MANUF_ENABLE_MAC_SNIFFER_RF:
		/* m_bMacSnifferRF */
		*ptrBuff++ = pu8AttributeValue[0];
		break;
	case MAC_WRP_PIB_MANUF_POS_TABLE_COUNT_RF:
		/* 16 bits */
		mem_copy_to_usi_endianness_uint16((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
		ptrBuff += 2;
		break;
	case MAC_WRP_PIB_MANUF_MAC_INTERNAL_VERSION_RF:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;
	case MAC_WRP_PIB_MANUF_RESET_MAC_STATS_RF:
		/* Response will be mac_status_denied */
		break;
	case MAC_WRP_PIB_MANUF_POS_TABLE_ELEMENT_RF:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;
	case MAC_WRP_PIB_MANUF_PHY_PARAM_RF:
		switch (u16AttributeIndex) {
		case MAC_WRP_RF_PHY_PARAM_PHY_CHANNEL_FREQ_HZ:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_TOTAL:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_TOTAL_BYTES:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_TOTAL:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_BUSY_TX:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_BUSY_RX:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_BUSY_CHN:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_BAD_LEN:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_BAD_FORMAT:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_TIMEOUT:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_ABORTED:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_CFM_NOT_HANDLED:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_TOTAL:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_TOTAL_BYTES:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_TOTAL:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_FALSE_POSITIVE:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_BAD_LEN:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_BAD_FORMAT:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_BAD_FCS_PAY:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_ABORTED:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_OVERRIDE:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_IND_NOT_HANDLED:
			mem_copy_to_usi_endianness_uint32((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
			ptrBuff += 4;
			break;

		case MAC_WRP_RF_PHY_PARAM_DEVICE_ID:
		case MAC_WRP_RF_PHY_PARAM_PHY_BAND_OPERATING_MODE:
		case MAC_WRP_RF_PHY_PARAM_PHY_CHANNEL_NUM:
		case MAC_WRP_RF_PHY_PARAM_PHY_CCA_ED_DURATION:
		case MAC_WRP_RF_PHY_PARAM_PHY_TURNAROUND_TIME:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_PAY_SYMBOLS:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_PAY_SYMBOLS:
		case MAC_WRP_RF_PHY_PARAM_MAC_UNIT_BACKOFF_PERIOD:
			mem_copy_to_usi_endianness_uint16((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue);
			ptrBuff += 2;
			break;

		case MAC_WRP_RF_PHY_PARAM_DEVICE_RESET:
		case MAC_WRP_RF_PHY_PARAM_TRX_RESET:
		case MAC_WRP_RF_PHY_PARAM_TRX_SLEEP:
		case MAC_WRP_RF_PHY_PARAM_PHY_CCA_ED_THRESHOLD:
		case MAC_WRP_RF_PHY_PARAM_PHY_STATS_RESET:
		case MAC_WRP_RF_PHY_PARAM_TX_FSK_FEC:
		case MAC_WRP_RF_PHY_PARAM_TX_OFDM_MCS:
			*ptrBuff++ = pu8AttributeValue[0];
			break;

		case MAC_WRP_RF_PHY_PARAM_FW_VERSION:
			/* MAC_WRP_STATUS_READ_ONLY */
			break;
			
		case MAC_WRP_RF_PHY_PARAM_PHY_CCA_ED_CONFIG:
			mem_copy_to_usi_endianness_uint16((uint8_t *)ptrBuff, (uint8_t *)pu8AttributeValue); /* us_duration_us */
			ptrBuff += 2;
			*ptrBuff++ = pu8AttributeValue[2]; /* sc_threshold_dBm */
			break;

		default:
			break;
		}
		break;
#endif

	default:         /* Unknown PIB, copy the value as it is. */
		memcpy(ptrBuff, pu8AttributeValue, u8AttributeLength);
		ptrBuff += u8AttributeLength;
	}
	/* Message Data Length of USI Frame */
	length = ptrBuff - buffTxAdpG3;
	/* Filling USI Frame Structure */
	adpG3Msg.pType = PROTOCOL_ADP_G3;
	adpG3Msg.buf = buffTxAdpG3;
	adpG3Msg.len = length;
	/* Send USI Frame */
	result = usi_SendCmd(&adpG3Msg) ? 0 : -1;
	LOG_IFACE_G3_ADP("AdpMacSetRequest result = %d\r\n", result);
}

/**********************************************************************************************************************/

/** The AdpMacSetConfirm primitive allows the upper layer to be notified of the completion of an AdpMacSetRequest.
 ***********************************************************************************************************************
 * @param m_u8Status The status of the scan request.
 * @param m_u32AttributeId The identifier of the IB attribute set.
 * @param m_u16AttributeIndex The index within the table of the specified IB attribute.
 **********************************************************************************************************************/
typedef void (*AdpMacSetConfirm)(struct TAdpMacSetConfirm *pSetConfirm);

/**********************************************************************************************************************/

/** The AdpMacSetRequestSync primitive allows the upper layer to set the value of an attribute in the MAC information base
 * synchronously
 ***********************************************************************************************************************
 * @param u32AttributeId The identifier of the MAC IB attribute set
 * @param u16AttributeIndex The index within the table of the specified IB attribute.
 * @param u8AttributeLength The length of the value of the attribute to set
 * @param pu8AttributeValue The value of the attribute to set
 * @param pSetConfirm The set confirm
 **********************************************************************************************************************/
void AdpMacSetRequestSync(uint32_t u32AttributeId, uint16_t u16AttributeIndex,
		uint8_t u8AttributeLength, const uint8_t *pu8AttributeValue,
		struct TAdpMacSetConfirm *pSetConfirm)
{
	uint8_t result = -1;

	LOG_IFACE_G3_ADP("AdpMacSetRequestSync start\r\n");

	/* Set the sync flags to intercept the callback */
	g_adp_sync_mgmt.f_sync_req = true;
	g_adp_sync_mgmt.m_u32AttributeId = u32AttributeId; /* Used in order to know AttributeId requested on AdpMacSetConfirm callback */
	g_adp_sync_mgmt.f_sync_res = false;

	/* Send the asynchronous call */
	AdpMacSetRequest(u32AttributeId, u16AttributeIndex, u8AttributeLength, pu8AttributeValue);

	/* Wait processing until flag activates, or timeout */
	addUsi_WaitProcessing(G3_SYNC_TIMEOUT, (Bool *)(&g_adp_sync_mgmt.f_sync_res));

	if (g_adp_sync_mgmt.f_sync_res) {
		/* Confirm received */
		/* pSetConfirm = &g_adp_sync_mgmt.s_MacSetConfirm; */
		memcpy(pSetConfirm, &g_adp_sync_mgmt.s_MacSetConfirm, sizeof(struct TAdpMacSetConfirm));
		result = 0;
	} else {
		/* Confirm not received */
		LOG_IFACE_G3_ADP("ERROR: Confirm for 0x%X not received\r\n", u32AttributeId);
	}

	g_adp_sync_mgmt.f_sync_req = false;
	g_adp_sync_mgmt.f_sync_res = false;

	LOG_IFACE_G3_ADP("AdpMacSetRequestSync result = %u\r\n", result);
}

/**********************************************************************************************************************/

/* The AdpNetworkStatusIndication primitive allows the next higher layer of a PAN coordinator or a coordinator to be
 * notified when a particular event occurs on the PAN.
 **********************************************************************************************************************
 * @param m_u16PanId The 16-bit PAN identifier of the device from which the frame was received or to which the frame
 *    was being sent.
 * @param m_SrcDeviceAddress The individual device address of the entity from which the frame causing the error
 *    originated.
 * @param m_DstDeviceAddress The individual device address of the device for which the frame was intended.
 * @param m_u8Status The communications status.
 * @param m_u8SecurityLevel The security level purportedly used by the received frame.
 * @param m_u8KeyIndex The index of the key purportedly used by the originator of the received frame.
 **********************************************************************************************************************/
typedef void (*AdpNetworkStatusIndication)(
	struct TAdpNetworkStatusIndication *pNetworkStatusIndication);

/**********************************************************************************************************************/

/** The AdpBufferIndication primitive allows the next higher layer to be notified when the modem has reached its
 * capability limit to perform the next frame..
 ***********************************************************************************************************************
 * @param m_bBufferReady TRUE: modem is ready to receipt more data frame;
 *                       FALSE: modem is not ready, stop sending data frame.
 **********************************************************************************************************************/
typedef void (*AdpBufferIndication)(struct TAdpBufferIndication *pBufferIndication);

/**********************************************************************************************************************/

/** The AdpPREQIndication primitive allows the next higher layer to be notified when a PREQ frame is received
 * in unicast mode with Originator Address equal to Coordinator Address and with Destination Address equal to Device Address
 **********************************************************************************************************************/
typedef void (*AdpPREQIndication)(void);

/**********************************************************************************************************************/

/** The AdpRouteDiscoveryRequest primitive allows the upper layer to initiate a route discovery.
 ***********************************************************************************************************************
 * @param u16DstAddr The short unicast destination address of the route discovery.
 * @param u8MaxHops This parameter indicates the maximum number of hops allowed for the route discovery (Range: 0x01 - 0x0E)
 **********************************************************************************************************************/
void AdpRouteDiscoveryRequest(uint16_t u16DstAddr, uint8_t u8MaxHops)
{
	Uint8 *ptrBuff;
	Uint16 length;
	int result;

	ptrBuff = buffTxAdpG3;
	/* Filling Message Data of USI Frame */
	*ptrBuff++ = G3_SERIAL_MSG_ADP_ROUTE_DISCOVERY_REQUEST;
	*ptrBuff++ = (u16DstAddr >> 8);
	*ptrBuff++ = (u16DstAddr & 0xFF);
	*ptrBuff++ = u8MaxHops;
	/* Message Data Length of USI Frame */
	length = ptrBuff - buffTxAdpG3;
	/* Filling USI Frame Structure */
	adpG3Msg.pType = PROTOCOL_ADP_G3;
	adpG3Msg.buf = buffTxAdpG3;
	adpG3Msg.len = length;
	/* Send USI Frame */
	result = usi_SendCmd(&adpG3Msg) ? 0 : -1;

	LOG_IFACE_G3_ADP("AdpRouteDiscoveryRequest result = %d\r\n", result);
}

/**********************************************************************************************************************/

/** The AdpRouteDiscoveryConfirm primitive allows the upper layer to be notified of the completion of a
 * AdpRouteDiscoveryRequest.
 ***********************************************************************************************************************
 * @param m_u8Status The status of the route discovery.
 **********************************************************************************************************************/
typedef void (*AdpRouteDiscoveryConfirm)(
	struct TAdpRouteDiscoveryConfirm *pRouteDiscoveryConfirm);

/**********************************************************************************************************************/

/** The AdpPathDiscoveryRequest primitive allows the upper layer to initiate a path discovery.
 ***********************************************************************************************************************
 * @param u16DstAddr The short unicast destination address of the path discovery.
 * @param u8MetricType The metric type to be used for the path discovery. (Range: 0x00 - 0x0F)
 **********************************************************************************************************************/
void AdpPathDiscoveryRequest(uint16_t u16DstAddr, uint8_t u8MetricType)
{
	Uint8 *ptrBuff;
	Uint16 length;
	int result;

	ptrBuff = buffTxAdpG3;
	/* Filling Message Data of USI Frame */
	*ptrBuff++ = G3_SERIAL_MSG_ADP_PATH_DISCOVERY_REQUEST;
	*ptrBuff++ = (u16DstAddr >> 8);
	*ptrBuff++ = (u16DstAddr & 0xFF);
	*ptrBuff++ = u8MetricType;
	/* Message Data Length of USI Frame */
	length = ptrBuff - buffTxAdpG3;
	/* Filling USI Frame Structure */
	adpG3Msg.pType = PROTOCOL_ADP_G3;
	adpG3Msg.buf = buffTxAdpG3;
	adpG3Msg.len = length;
	/* Send USI Frame */
	result = usi_SendCmd(&adpG3Msg) ? 0 : -1;

	LOG_IFACE_G3_ADP("AdpPathDiscoveryRequest result = %d\r\n", result);
}

/**********************************************************************************************************************/

/** The AdpPathDiscoveryConfirm primitive allows the upper layer to be notified of the completion of a
 * AdpPathDiscoveryRequest.
 ***********************************************************************************************************************
 * @param m_u8Status The status of the path discovery. (status can be INCOMPLETE and the other parameters contain the
 *				discovered path)
 * @param m_u16DstAddr The short unicast destination address of the path discovery.
 * @param m_u16Originator The originator of the path reply
 * @param m_u8PathMetricType Path metric type
 * @param m_u8ForwardHopsCount Number of path hops in the forward table
 * @param m_u8ReverseHopsCount Number of path hops in the reverse table
 * @param m_aForwardPath Table with the information of each hop in forward direction (according to m_u8ForwardHopsCount)
 * @param m_aReversePath Table with the information of each hop in reverse direction (according to m_u8ReverseHopsCount)
 **********************************************************************************************************************/
typedef void (*AdpPathDiscoveryConfirm)(
	struct TAdpPathDiscoveryConfirm *pPathDiscoveryConfirm);

/**********************************************************************************************************************/

/** The AdpLbpRequest primitive allows the upper layer of the client to send the LBP message to the server modem.
 ***********************************************************************************************************************
 * @param pDstAddr 16-bit address of LBA or LBD or 64 bit address (extended address of LBD)
 * @param u16NsduLength The size of the NSDU, in bytes
 * @param pNsdu The NSDU to send
 * @param u8NsduHandle The handle of the NSDU to transmit. This parameter is used to identify in the AdpLbpConfirm
 *                                      primitive which request is concerned. It can be randomly chosen by the application layer.
 * @param u8MaxHops The number of times the frame will be repeated by network routers.
 * @param bDiscoveryRoute If TRUE, a route discovery procedure will be performed prior to sending the frame if a route
 *                                      to the destination is not available in the routing table. If FALSE, no route discovery is performed.
 * @param u8QualityOfService The requested quality of service (QoS) of the frame to send. Allowed values are:
 *					0x00 = standard priority
 *					0x01 = high priority
 * @param bSecurityEnable If TRUE, this parameter enables the MAC layer security for sending the frame.
 **********************************************************************************************************************/
void AdpLbpRequest(const struct TAdpAddress *pDstAddr, uint16_t u16NsduLength,
		uint8_t *pNsdu, uint8_t u8NsduHandle, uint8_t u8MaxHops,
		bool bDiscoveryRoute, uint8_t u8QualityOfService, bool bSecurityEnable)
{
	Uint8 *ptrBuff;
	Uint16 length;
	int i, result;

	LOG_IFACE_G3_ADP("AdpLbPRequest\r\n");

	ptrBuff = buffTxAdpG3;
	/* Filling Message Data of USI Frame */
	*ptrBuff++ = G3_SERIAL_MSG_ADP_LBP_REQUEST;
	*ptrBuff++ = u8NsduHandle;
	*ptrBuff++ = u8MaxHops;
	*ptrBuff++ = bDiscoveryRoute ? 1 : 0;
	*ptrBuff++ = u8QualityOfService;
	*ptrBuff++ = bSecurityEnable ? 1 : 0;
	*ptrBuff++ = pDstAddr->m_u8AddrSize == ADP_ADDRESS_16BITS ? ADP_ADDRESS_16BITS : ADP_ADDRESS_64BITS;
	*ptrBuff++ = (u16NsduLength >> 8);
	*ptrBuff++ = (u16NsduLength & 0xFF);
	if (pDstAddr->m_u8AddrSize == ADP_ADDRESS_16BITS) {
		*ptrBuff++ = (uint8_t)((pDstAddr->m_u16ShortAddr & 0xFF00) >> 8);
		*ptrBuff++ = (uint8_t)(pDstAddr->m_u16ShortAddr & 0x00FF);
	} else if (pDstAddr->m_u8AddrSize == ADP_ADDRESS_64BITS) {
		memcpy(ptrBuff, (uint8_t *)(&pDstAddr->m_ExtendedAddress.m_au8Value[0]), ADP_ADDRESS_64BITS);
		ptrBuff += ADP_ADDRESS_64BITS;
	} else {
		/* ToDo: Log error */
		LOG_IFACE_G3_ADP("AdpLbpRequest ERROR");
	}

	for (i = 0; i < u16NsduLength; i++) {
		*ptrBuff++ = pNsdu[i];
	}
	/* Message Data Length of USI Frame */
	length = ptrBuff - buffTxAdpG3;
	/* Filling USI Frame Structure */
	adpG3Msg.pType = PROTOCOL_ADP_G3;
	adpG3Msg.buf = buffTxAdpG3;
	adpG3Msg.len = length;
	/* Send USI Frame */
	result = usi_SendCmd(&adpG3Msg) ? 0 : -1;
	LOG_IFACE_G3_ADP("AdpLbpRequest result = %d\r\n", result);
}

/*************** CALLBACK FROM SERIAL TO ADP CALLBACK ****************/

/**
 * @brief _cl_null_adpStatus_cb
 * The AdpStatus primitive is used to notify an error processing a previous request
 * ((status != SERIAL_STATUS_UNKNOWN_COMMAND) && (status != SERIAL_STATUS_SUCCESS))
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpStatus_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpStatus_cb\r\n");
	/* Check the message length */
	if (len < 1) {
		LOG_IFACE_G3_ADP("ERROR: Size %u < 1\r\n", len);
		return(false);
	}

	enum ESerialStatus m_u8Status;
	m_u8Status = (*ptrMsg++);
	if (len > 1){
		uint8_t m_u8CommandId;
		m_u8CommandId = (*ptrMsg++);
		LOG_IFACE_G3_ADP("CommandId: 0x%X; AdpStatus: 0x%X\r\n", m_u8CommandId, m_u8Status);
	}
	else{
		LOG_IFACE_G3_ADP("CommandId: UNKNOWN; AdpStatus: 0x%X\r\n", m_u8Status);
	}
	return(true);
}

/**
 * @brief _cl_null_adpDataIndication_cb
 * The AdpDataIndication primitive is used to transfer received data from the adaptation sublayer to the upper layer.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpDataIndication_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpDataIndication_cb\r\n");
	/* Check the message length */
	if (len < 3) {
		LOG_IFACE_G3_ADP("ERROR: Size %u < 3\r\n", len);
		return(false);
	}

	if (g_adpNotifications.fnctAdpDataIndication) {
		struct TAdpDataIndication adpDataIndication;
		adpDataIndication.m_u8LinkQualityIndicator = (*ptrMsg++);
		adpDataIndication.m_u16NsduLength = (*ptrMsg++);
		adpDataIndication.m_u16NsduLength = (*ptrMsg++) + (adpDataIndication.m_u16NsduLength << 8);
		/* If the length matches, the NSDU is copied */
		if (len == adpDataIndication.m_u16NsduLength + 3) {
			adpDataIndication.m_pNsdu = (uint8_t *)malloc(adpDataIndication.m_u16NsduLength * sizeof(uint8_t));
			memcpy((uint8_t *)adpDataIndication.m_pNsdu, ptrMsg, adpDataIndication.m_u16NsduLength);
			/* Trigger the callback */
			g_adpNotifications.fnctAdpDataIndication(&adpDataIndication); /* lqi, nsdu_len, nsdu); */
			free((uint8_t *)(adpDataIndication.m_pNsdu));
		} else {
			LOG_IFACE_G3_ADP("ERROR: wrong indication length.\r\n");
			return(false);
		}
	}

	return(true);
}

/**
 * @brief _cl_null_adpNetworkStatusIndication_cb
 * The AdpNetworkStatusIndication primitive allows the next higher layer of a PAN coordinator or a coordinator to be
 * notified when a particular event occurs on the PAN.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpNetworkStatusIndication_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpNetworkStatusIndication_cb\r\n");
	/* Check the message length */
	if (len < 11) {
		LOG_IFACE_G3_ADP("ERROR: Size %u < 11\r\n", len);
		return(false);
	}

	if (g_adpNotifications.fnctAdpNetworkStatusIndication) {
		struct TAdpNetworkStatusIndication adpNetworkStatusIndication;
		adpNetworkStatusIndication.m_u16PanId = (*ptrMsg++);
		adpNetworkStatusIndication.m_u16PanId = (*ptrMsg++) + (adpNetworkStatusIndication.m_u16PanId << 8);
		adpNetworkStatusIndication.m_SrcDeviceAddress.m_u8AddrSize = (*ptrMsg++);
		if (adpNetworkStatusIndication.m_SrcDeviceAddress.m_u8AddrSize == ADP_ADDRESS_16BITS) {
			memcpy(&adpNetworkStatusIndication.m_SrcDeviceAddress.m_u16ShortAddr, ptrMsg, ADP_ADDRESS_16BITS);
			ptrMsg += ADP_ADDRESS_16BITS;
		} else if (adpNetworkStatusIndication.m_SrcDeviceAddress.m_u8AddrSize == ADP_ADDRESS_64BITS) {
			memcpy(&adpNetworkStatusIndication.m_SrcDeviceAddress.m_ExtendedAddress, ptrMsg, ADP_ADDRESS_64BITS);
			ptrMsg += ADP_ADDRESS_64BITS;
		} else {
			LOG_IFACE_G3_ADP("ERROR: wrong src address size.\r\n");
			return(false);
		}

		adpNetworkStatusIndication.m_DstDeviceAddress.m_u8AddrSize = (*ptrMsg++);
		if (adpNetworkStatusIndication.m_DstDeviceAddress.m_u8AddrSize == ADP_ADDRESS_16BITS) {
			memcpy(&adpNetworkStatusIndication.m_DstDeviceAddress.m_u16ShortAddr, ptrMsg, ADP_ADDRESS_16BITS);
			ptrMsg += ADP_ADDRESS_16BITS;
		} else if (adpNetworkStatusIndication.m_DstDeviceAddress.m_u8AddrSize == ADP_ADDRESS_64BITS) {
			memcpy(&adpNetworkStatusIndication.m_DstDeviceAddress.m_ExtendedAddress, ptrMsg, ADP_ADDRESS_64BITS);
			ptrMsg += ADP_ADDRESS_64BITS;
		} else {
			LOG_IFACE_G3_ADP("ERROR: wrong dst address size.\r\n");
			return(false);
		}

		adpNetworkStatusIndication.m_u8Status = (*ptrMsg++);
		adpNetworkStatusIndication.m_u8SecurityLevel = (*ptrMsg++);
		adpNetworkStatusIndication.m_u8KeyIndex = (*ptrMsg++);
#ifdef G3_HYBRID_PROFILE
		adpNetworkStatusIndication.m_u8MediaType = (*ptrMsg++);
#endif
		/* Trigger the callback */
		g_adpNotifications.fnctAdpNetworkStatusIndication(&adpNetworkStatusIndication);
	}

	return(true);
}

/**
 * @brief _cl_null_adpNetworkDiscoveryIndication_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpNetworkDiscoveryIndication_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpNetworkDiscoveryIndication_cb\r\n");

	/* Check the message length */
	if (len < 7) {
		LOG_IFACE_G3_ADP("ERROR: Size %u < 7\r\n", len);
		return(false);
	}

	if (g_adpNotifications.fnctAdpDiscoveryIndication) {
		struct TAdpPanDescriptor sPanDescriptor;

		sPanDescriptor.m_u16PanId = (*ptrMsg++);
		sPanDescriptor.m_u16PanId = (*ptrMsg++) + (sPanDescriptor.m_u16PanId << 8);
		sPanDescriptor.m_u8LinkQuality = (*ptrMsg++);
		sPanDescriptor.m_u16LbaAddress = (*ptrMsg++);
		sPanDescriptor.m_u16LbaAddress = (*ptrMsg++) + (sPanDescriptor.m_u16LbaAddress << 8);
		sPanDescriptor.m_u16RcCoord = (*ptrMsg++);
		sPanDescriptor.m_u16RcCoord = (*ptrMsg++) + (sPanDescriptor.m_u16RcCoord << 8);
#ifdef G3_HYBRID_PROFILE
		sPanDescriptor.m_u8MediaType = (*ptrMsg++);
#endif
		/* Trigger the callback */
		g_adpNotifications.fnctAdpDiscoveryIndication(&sPanDescriptor);
	}

	LOG_IFACE_G3_ADP("_cl_null_adpNetworkDiscoveryIndication_cb\r\n");
	return(true);
}

/**
 * @brief _cl_null_adpNetworkLeaveIndication_cb
 * The AdpNetworkLeaveIndication primitive is generated by the ADP layer of a non-coordinator device to inform
 * the upper layer that it has been unregistered from the network by the coordinator.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpNetworkLeaveIndication_cb()
{
	LOG_IFACE_G3_ADP("_cl_null_adpNetworkLeaveIndication_cb\r\n");

	if (g_adpNotifications.fnctAdpNetworkLeaveIndication) {
		/* Trigger the callback */
		g_adpNotifications.fnctAdpNetworkLeaveIndication();
	}

	return(true);
}

/**
 * @brief _cl_null_adpLBPIndication_cb
 * The AdpLbpIndication primitive is used to transfer a received LBP frame from the ADP layer to the upper layer.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpLBPIndication_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpLBPIndication_cb\r\n");
	/* Check the message length */
	if (len < 4) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adpNotifications.fnctAdpLbpIndication) {
		struct TAdpLbpIndication adpLbpIndication;
		adpLbpIndication.m_u16SrcAddr = (*ptrMsg++);
		adpLbpIndication.m_u16SrcAddr = (*ptrMsg++) + (adpLbpIndication.m_u16SrcAddr << 8);
		adpLbpIndication.m_u16NsduLength = (*ptrMsg++);
		adpLbpIndication.m_u16NsduLength = (*ptrMsg++) + (adpLbpIndication.m_u16NsduLength << 8);
		/* If the length matches, the NSDU is copied */
		LOG_IFACE_G3_ADP("m_u16SrcAddr=0x%X,length=%d; m_u16NsduLength=%d\r\n", adpLbpIndication.m_u16SrcAddr, len, adpLbpIndication.m_u16NsduLength);
		if (len == adpLbpIndication.m_u16NsduLength + 6) {
			adpLbpIndication.m_pNsdu = (uint8_t *)malloc(adpLbpIndication.m_u16NsduLength * sizeof(uint8_t));
			memcpy(&adpLbpIndication.m_pNsdu[0], ptrMsg, adpLbpIndication.m_u16NsduLength);
			ptrMsg += adpLbpIndication.m_u16NsduLength;
			adpLbpIndication.m_u8LinkQualityIndicator = (*ptrMsg++);
			adpLbpIndication.m_bSecurityEnabled = (*ptrMsg++);
			/* Trigger the callback */
			g_adpNotifications.fnctAdpLbpIndication(&adpLbpIndication);
			free(adpLbpIndication.m_pNsdu);
		} else {
			LOG_IFACE_G3_ADP("ERROR: wrong indication length.\r\n");
			return(false);
		}
	}

	return(true);
}

/**
 * @brief _cl_null_adpDataConfirm_cb
 * The AdpDataConfirm primitive allows the upper layer to be notified of the completion of an AdpDataRequest.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpDataConfirm_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpDataConfirm_cb\r\n");
	/* Check the message length */
	if (len < 2) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adpNotifications.fnctAdpDataConfirm) {
		struct TAdpDataConfirm adpDataConfirm;
		adpDataConfirm.m_u8Status = (*ptrMsg++);
		adpDataConfirm.m_u8NsduHandle = (*ptrMsg++);
		g_adpNotifications.fnctAdpDataConfirm(&adpDataConfirm);
	}

	return(true);
}

/**
 * @brief _cl_null_adpDiscoveryConfirm_cb
 * The AdpDiscoveryConfirm primitive allows the upper layer to be notified of the completion of an AdpDiscoveryRequest
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpDiscoveryConfirm_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpDiscoveryConfirm_cb\r\n");
	/* Check the message length */
	if (len < 1) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adpNotifications.fnctAdpDiscoveryConfirm) {
		uint8_t status;
		status = (*ptrMsg++);
		g_adpNotifications.fnctAdpDiscoveryConfirm(status);
	}

	return(true);
}

/**
 * @brief _cl_null_adpNetworkStartConfirm_cb
 * The AdpNetworkStartConfirm primitive allows the upper layer to be notified of the completion of an AdpNetworkStartRequest.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpNetworkStartConfirm_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpNetworkStartConfirm_cb\r\n");
	/* Check the message length */
	if (len < 1) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adpNotifications.fnctAdpNetworkStartConfirm) {
		struct TAdpNetworkStartConfirm adpNetworkStartConfirm;
		adpNetworkStartConfirm.m_u8Status = (*ptrMsg++);
		g_adpNotifications.fnctAdpNetworkStartConfirm(&adpNetworkStartConfirm);
	}

	return(true);
}

/**
 * @brief _cl_null_adpNetworkJoinConfirm_cb
 * The AdpNetworkJoinConfirm primitive allows the upper layer to be notified of the completion of an AdpNetworkJoinRequest.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpNetworkJoinConfirm_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpNetworkJoinConfirm_cb\r\n");
	/* Check the message length */
	if (len < 5) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adpNotifications.fnctAdpNetworkJoinConfirm) {
		struct TAdpNetworkJoinConfirm adpNetworkJoinConfirm;
		adpNetworkJoinConfirm.m_u8Status = (*ptrMsg++);
		adpNetworkJoinConfirm.m_u16NetworkAddress = (*ptrMsg++);
		adpNetworkJoinConfirm.m_u16NetworkAddress = (*ptrMsg++) + (adpNetworkJoinConfirm.m_u16NetworkAddress << 8);
		adpNetworkJoinConfirm.m_u16PanId = (*ptrMsg++);
		adpNetworkJoinConfirm.m_u16PanId = (*ptrMsg++) + (adpNetworkJoinConfirm.m_u16PanId << 8);
		g_adpNotifications.fnctAdpNetworkJoinConfirm(&adpNetworkJoinConfirm);
	}

	return(true);
}

/**
 * @brief _cl_null_adpNetworkLeaveConfirm_cb
 * The AdpNetworkLeaveConfirm primitive allows the upper layer to be notified of the completion of an AdpNetworkLeaveRequest.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpNetworkLeaveConfirm_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpNetworkLeaveConfirm_cb\r\n");
	/* Check the message length */
	if (len < 1) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adpNotifications.fnctAdpNetworkLeaveConfirm) {
		struct TAdpNetworkLeaveConfirm adpNetworkLeaveConfirm;
		adpNetworkLeaveConfirm.m_u8Status = (*ptrMsg++);
		g_adpNotifications.fnctAdpNetworkLeaveConfirm(&adpNetworkLeaveConfirm);
	}

	return(true);
}

/**
 * @brief _cl_null_adpResetConfirm_cb
 * The AdpResetConfirm primitive allows the upper layer to be notified of the completion of an AdpResetRequest.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpResetConfirm_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpResetConfirm_cb\r\n");
	/* Check the message length */
	if (len < 1) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adpNotifications.fnctAdpResetConfirm) {
		struct TAdpResetConfirm adpResetConfirm;
		adpResetConfirm.m_u8Status = (*ptrMsg++);
		g_adpNotifications.fnctAdpResetConfirm(&adpResetConfirm);
	}

	return(true);
}

/**
 * @brief _cl_null_adpSetConfirm_cb
 * The AdpSetConfirm primitive allows the upper layer to be notified of the completion of an AdpSetRequest.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpSetConfirm_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpSetConfirm_cb\r\n");
	/* Check the message length */
	if (len < 1) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adp_sync_mgmt.f_sync_req || g_adpNotifications.fnctAdpSetConfirm) {
		struct TAdpSetConfirm adpSetConfirm;
		adpSetConfirm.m_u8Status = (*ptrMsg++);
		adpSetConfirm.m_u32AttributeId = (*ptrMsg++);
		adpSetConfirm.m_u32AttributeId = (*ptrMsg++) + (adpSetConfirm.m_u32AttributeId << 8);
		adpSetConfirm.m_u32AttributeId = (*ptrMsg++) + (adpSetConfirm.m_u32AttributeId << 8);
		adpSetConfirm.m_u32AttributeId = (*ptrMsg++) + (adpSetConfirm.m_u32AttributeId << 8);
		adpSetConfirm.m_u16AttributeIndex = (*ptrMsg++);
		adpSetConfirm.m_u16AttributeIndex = (*ptrMsg++) + (adpSetConfirm.m_u16AttributeIndex << 8);
		LOG_IFACE_G3_ADP("Status:%d, AttributeId:0x%X,AttributeIndex=0x%X\r\n", adpSetConfirm.m_u8Status, adpSetConfirm.m_u32AttributeId, adpSetConfirm.m_u16AttributeIndex);
		if (g_adp_sync_mgmt.f_sync_req && (g_adp_sync_mgmt.m_u32AttributeId == adpSetConfirm.m_u32AttributeId)) {
			/* Synchronous call -> Store the result */
			LOG_IFACE_G3_ADP("Syncronous Call pending; copying received structure\r\n");
			memcpy(&g_adp_sync_mgmt.s_SetConfirm, &adpSetConfirm, sizeof(struct TAdpSetConfirm));
			g_adp_sync_mgmt.f_sync_res = true;
		}

		if (g_adpNotifications.fnctAdpSetConfirm) {
			/* Asynchronous call -> Callback */
			g_adpNotifications.fnctAdpSetConfirm(&adpSetConfirm);
		}
	}

	return(true);
}

/**
 * @brief _cl_null_adpGetConfirm_cb
 * The AdpGetConfirm primitive allows the upper layer to be notified of the completion of an AdpGetRequest.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpGetConfirm_cb(uint8_t *ptrMsg, uint16_t len)
{
	uint8_t u8_aux;
	LOG_IFACE_G3_ADP("_cl_null_adpGetConfirm_cb\r\n");
	/* Check the message length */
	if (len < 1) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adp_sync_mgmt.f_sync_req || g_adpNotifications.fnctAdpGetConfirm) {
		struct TAdpGetConfirm adpGetConfirm;
		adpGetConfirm.m_u8Status = (*ptrMsg++);
		adpGetConfirm.m_u32AttributeId = (*ptrMsg++);
		adpGetConfirm.m_u32AttributeId = (*ptrMsg++) + (adpGetConfirm.m_u32AttributeId << 8);
		adpGetConfirm.m_u32AttributeId = (*ptrMsg++) + (adpGetConfirm.m_u32AttributeId << 8);
		adpGetConfirm.m_u32AttributeId = (*ptrMsg++) + (adpGetConfirm.m_u32AttributeId << 8);
		adpGetConfirm.m_u16AttributeIndex = (*ptrMsg++);
		adpGetConfirm.m_u16AttributeIndex = (*ptrMsg++) + (adpGetConfirm.m_u16AttributeIndex << 8);
		adpGetConfirm.m_u8AttributeLength = (*ptrMsg++);
		if (adpGetConfirm.m_u8AttributeLength > 64) {
			/* ToDo: Log error */
			return(false);
		}

		LOG_IFACE_G3_ADP("Status:%d, AttributeId:0x%X,AttributeIndex=0x%X,AttributeLength=%d\r\n", adpGetConfirm.m_u8Status, adpGetConfirm.m_u32AttributeId, adpGetConfirm.m_u16AttributeIndex, adpGetConfirm.m_u8AttributeLength);

		switch (adpGetConfirm.m_u32AttributeId) {
		case ADP_IB_SECURITY_LEVEL:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpSecurityLevel */
			break;

		case ADP_IB_PREFIX_TABLE:
			u8_aux = 0;
			uint8_t u8PrefixLength_bytes = adpGetConfirm.m_u8AttributeLength - 11;
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8PrefixLength */
			adpGetConfirm.m_au8AttributeValue[1] = ptrMsg[1]; /* m_bOnLinkFlag */
			adpGetConfirm.m_au8AttributeValue[2] = ptrMsg[2]; /* m_bAutonomousAddressConfigurationFlag */
			mem_copy_from_usi_endianness_uint32((uint8_t *)&ptrMsg[3], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[3]);  /* u32ValidTime */
			mem_copy_from_usi_endianness_uint32((uint8_t *)&ptrMsg[7], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[7]);  /* u32PreferredTime */
			memcpy((uint8_t *)&adpGetConfirm.m_au8AttributeValue[11], (uint8_t *)(&ptrMsg[11]), u8PrefixLength_bytes);  /* m_au8Prefix */
			break;

		case ADP_IB_BROADCAST_LOG_TABLE_ENTRY_TTL:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16AdpBroadcastLogTableEntryTTL */
			break;

		case ADP_IB_METRIC_TYPE:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpMetricType */
			break;

		case ADP_IB_LOW_LQI_VALUE:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpLowLQIValue */
			break;

		case ADP_IB_HIGH_LQI_VALUE:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpHighLQIValue */
			break;

		case ADP_IB_RREP_WAIT:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpRREPWait */
			break;

		case ADP_IB_CONTEXT_INFORMATION_TABLE:
			if(adpGetConfirm.m_u8AttributeLength >= 4){
			u8_aux = adpGetConfirm.m_u8AttributeLength - 4;
				mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* u16ValidTime */
				adpGetConfirm.m_au8AttributeValue[2] = ptrMsg[2]; /* m_bValidForCompression */
				adpGetConfirm.m_au8AttributeValue[3] = ptrMsg[3]; /* m_u8BitsContextLength */
				memcpy((uint8_t *)&adpGetConfirm.m_au8AttributeValue[4], (uint8_t *)&ptrMsg[4], u8_aux);  /* m_au8Context */
			}
			break;

		case ADP_IB_COORD_SHORT_ADDRESS:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16AdpCoordShortAddress */
			break;

		case ADP_IB_RLC_ENABLED:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_bAdpRLCEnabled */
			break;

		case ADP_IB_ADD_REV_LINK_COST:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpAddRevLinkCost */
			break;

		case ADP_IB_BROADCAST_LOG_TABLE:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16SrcAddr */
			adpGetConfirm.m_au8AttributeValue[2] = ptrMsg[2]; /* m_u8SequenceNumber */
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[3], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[3]);  /* u16ValidTime */
			break;

		case ADP_IB_ROUTING_TABLE:
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[0], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[0]);  /* m_u16DstAddr */
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[2], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[2]);  /* m_u16NextHopAddr */
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[4], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[4]);  /* m_u16RouteCost */
			adpGetConfirm.m_au8AttributeValue[6] = ptrMsg[6]; /* m_u8HopCount || m_u8WeakLinkCount */
#ifdef G3_HYBRID_PROFILE
			adpGetConfirm.m_au8AttributeValue[7] = ptrMsg[7]; /* m_u8MediaType */
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[8], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[8]);  /* u16ValidTime */
#else
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[7], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[7]);  /* u16ValidTime */
#endif
			break;

		case ADP_IB_UNICAST_RREQ_GEN_ENABLE:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_bAdpUnicastRREQGenEnable */
			break;

		case ADP_IB_GROUP_TABLE:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16GroupAddress */
			adpGetConfirm.m_au8AttributeValue[2] = ptrMsg[2];  /* m_bValid */
			break;

		case ADP_IB_MAX_HOPS:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpMaxHops */
			break;

		case ADP_IB_DEVICE_TYPE:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_eAdpDeviceType */
			break;

		case ADP_IB_NET_TRAVERSAL_TIME:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpNetTraversalTime */
			break;

		case ADP_IB_ROUTING_TABLE_ENTRY_TTL:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16AdpRoutingTableEntryTTL */
			break;

		case ADP_IB_KR:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpKr */
			break;

		case ADP_IB_KM:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpKm */
			break;

		case ADP_IB_KC:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpKc */
			break;

		case ADP_IB_KQ:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpKq */
			break;

		case ADP_IB_KH:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpKh */
			break;

		case ADP_IB_RREQ_RETRIES:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpRREQRetries */
			break;

		case ADP_IB_RREQ_RERR_WAIT: /* ADP_IB_RREQ_WAIT also enters here as it has the same numeric value */
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpRREQRERRWait */
			break;

		case ADP_IB_WEAK_LQI_VALUE:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpWeakLQIValue */
			break;

		case ADP_IB_KRT:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpKrt */
			break;

		case ADP_IB_SOFT_VERSION:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8Major */
			adpGetConfirm.m_au8AttributeValue[1] = ptrMsg[1]; /* m_u8Minor */
			adpGetConfirm.m_au8AttributeValue[2] = ptrMsg[2]; /* m_u8Revision */
			adpGetConfirm.m_au8AttributeValue[3] = ptrMsg[3]; /* m_u8Year */
			adpGetConfirm.m_au8AttributeValue[4] = ptrMsg[4]; /* m_u8Month */
			adpGetConfirm.m_au8AttributeValue[5] = ptrMsg[5]; /* m_u8Day */
			break;

		case ADP_IB_SNIFFER_MODE:
			/* TODO */
			break;

		case ADP_IB_BLACKLIST_TABLE:
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[0], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[0]);  /* m_u16Addr */
#ifdef G3_HYBRID_PROFILE
			adpGetConfirm.m_au8AttributeValue[2] = ptrMsg[2]; /* m_u8MediaType */
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[3], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[3]);  /* u16ValidTime */
#else
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[2], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[2]);  /* u16ValidTime */
#endif
			break;

		case ADP_IB_BLACKLIST_TABLE_ENTRY_TTL:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16AdpBlacklistTableEntryTTL */
			break;

		case ADP_IB_MAX_JOIN_WAIT_TIME:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16AdpMaxJoinWaitTime */
			break;

		case ADP_IB_PATH_DISCOVERY_TIME:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpPathDiscoveryTime */
			break;

		case ADP_IB_ACTIVE_KEY_INDEX:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpActiveKeyIndex */
			break;

		case ADP_IB_DESTINATION_ADDRESS_SET:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);                                                                           /* m_u16Addr */
			break;

		case ADP_IB_DEFAULT_COORD_ROUTE_ENABLED:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_bAdpDefaultCoordRouteEnabled */
			break;

		case ADP_IB_DISABLE_DEFAULT_ROUTING:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_bAdpDisableDefaultRouting */
			break;

		/* manufacturer */
		case ADP_IB_MANUF_REASSEMBY_TIMER:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16AdpReassembyTimer */
			break;

		case ADP_IB_MANUF_IPV6_HEADER_COMPRESSION:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_bAdpIPv6HeaderCompression */
			break;

		/*	case ADP_IB_MANUF_EAP_PRESHARED_KEY: */
		/*		//Write Only */
		/*		break; */
		case ADP_IB_MANUF_EAP_NETWORK_ACCESS_IDENTIFIER:
			memcpy(adpGetConfirm.m_au8AttributeValue, ptrMsg, adpGetConfirm.m_u8AttributeLength);  /* m_au8Value */
			break;

		case ADP_IB_MANUF_BROADCAST_SEQUENCE_NUMBER:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8BroadcastSequenceNumber */
			break;

		/*	case ADP_IB_MANUF_REGISTER_DEVICE : */
		/*		break; */
		case ADP_IB_MANUF_DATAGRAM_TAG:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16DatagramTag */
			break;

		case ADP_IB_MANUF_RANDP:
			memcpy(adpGetConfirm.m_au8AttributeValue, (uint8_t *)ptrMsg, adpGetConfirm.m_u8AttributeLength);  /* m_au8Value */
			break;

		case ADP_IB_MANUF_ROUTING_TABLE_COUNT:
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* u32Count */
			break;

		/*	case ADP_IB_MANUF_DISCOVER_SEQUENCE_NUMBER : */
		/*		break; */
		case ADP_IB_MANUF_FORCED_NO_ACK_REQUEST:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8ForceNoAck */
			break;

		case ADP_IB_MANUF_LQI_TO_COORD:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8LQIToCoord */
			break;

		case ADP_IB_MANUF_BROADCAST_ROUTE_ALL:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_bBroadcastRouteAll */
			break;

		case ADP_IB_MANUF_KEEP_PARAMS_AFTER_KICK_LEAVE:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_bKeepParamsAfterKickLeave */
			break;

		case ADP_IB_MANUF_ADP_INTERNAL_VERSION:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8Major */
			adpGetConfirm.m_au8AttributeValue[1] = ptrMsg[1]; /* m_u8Minor */
			adpGetConfirm.m_au8AttributeValue[2] = ptrMsg[2]; /* m_u8Revision */
			adpGetConfirm.m_au8AttributeValue[3] = ptrMsg[3]; /* m_u8Year */
			adpGetConfirm.m_au8AttributeValue[4] = ptrMsg[4]; /* m_u8Month */
			adpGetConfirm.m_au8AttributeValue[5] = ptrMsg[5]; /* m_u8Day */
			break;

		case ADP_IB_MANUF_CIRCULAR_ROUTES_DETECTED:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16CircularRoutesDetected */
			break;

		case ADP_IB_MANUF_LAST_CIRCULAR_ROUTE_ADDRESS:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16LastCircularRouteAddress */
			break;

		case ADP_IB_MANUF_IPV6_ULA_DEST_SHORT_ADDRESS:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* g_AdpExecutionContext.m_AdpMib.m_u16ULADestShortAddress */
			break;

		case ADP_IB_MANUF_MAX_REPAIR_RESEND_ATTEMPTS:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8MaxRepairReSendAttemps */
			break;

		case ADP_IB_MANUF_DISABLE_AUTO_RREQ:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_bDisableAutoRREQ */
			break;

		case ADP_IB_MANUF_ALL_NEIGHBORS_BLACKLISTED_COUNT:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16AllNeighborBlacklistedCount */
			break;

		case ADP_IB_MANUF_QUEUED_ENTRIES_REMOVED_TIMEOUT_COUNT:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16QueuedEntriesRemovedTimeoutCount */
			break;

		case ADP_IB_MANUF_QUEUED_ENTRIES_REMOVED_ROUTE_ERROR_COUNT:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16QueuedEntriesRemovedRouteErrorCount */
			break;

		case ADP_IB_MANUF_PENDING_DATA_IND_SHORT_ADDRESS:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_PendingDataIndication.m_DstDeviceAddress.m_u16ShortAddr */
			break;

		case ADP_IB_MANUF_GET_BAND_CONTEXT_TONES:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_BandContext.m_u8Tones */
			break;

		case ADP_IB_MANUF_UPDATE_NON_VOLATILE_DATA:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_bUpdNonVolatileData */
			break;

		case ADP_IB_MANUF_DISCOVER_ROUTE_GLOBAL_SEQ_NUM:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16DiscoverRouteGlobalSeqNo */
			break;

		case ADP_IB_MANUF_FRAGMENT_DELAY:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16AdpFragmentDelay */
			break;

		case ADP_IB_MANUF_DYNAMIC_FRAGMENT_DELAY_ENABLED:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_bAdpDynamicFragmentDelayEnabled */
			break;

		case ADP_IB_MANUF_DYNAMIC_FRAGMENT_DELAY_FACTOR:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);  /* m_u16AdpDynamicFragmentDelayFactor */
			break;

		case ADP_IB_MANUF_BLACKLIST_TABLE_COUNT:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);
			break;

		case ADP_IB_MANUF_BROADCAST_LOG_TABLE_COUNT:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);
			break;

		case ADP_IB_MANUF_CONTEXT_INFORMATION_TABLE_COUNT:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);
			break;

		case ADP_IB_MANUF_GROUP_TABLE_COUNT:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpGetConfirm.m_au8AttributeValue);
			break;

		case ADP_IB_MANUF_ROUTING_TABLE_ELEMENT:
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[0], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[0]);  /* m_u16DstAddr */
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[2], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[2]);  /* m_u16NextHopAddr */
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[4], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[4]);  /* m_u16RouteCost */
			adpGetConfirm.m_au8AttributeValue[6] = ptrMsg[6]; /* m_u8HopCount || m_u8WeakLinkCount */
#ifdef G3_HYBRID_PROFILE
			adpGetConfirm.m_au8AttributeValue[7] = ptrMsg[7]; /* m_u8MediaType */
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[8], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[8]);  /* u16ValidTime */
#else
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[7], (uint8_t *)&adpGetConfirm.m_au8AttributeValue[7]);  /* u16ValidTime */
#endif
			break;

		case ADP_IB_MANUF_SET_PHASEDIFF_PREQ_PREP:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_bSetPhaseDiffPREQPREP */
			break;

		case ADP_IB_MANUF_HYBRID_PROFILE:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_bAdpHybridProfile */
			break;
			
#ifdef G3_HYBRID_PROFILE
		case ADP_IB_LOW_LQI_VALUE_RF:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpLowLQIValueRF */
			break;
			
		case ADP_IB_HIGH_LQI_VALUE_RF:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpHighLQIValueRF */
			break;
			
		case ADP_IB_KQ_RF:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpKqRF */
			break;
			
		case ADP_IB_KH_RF:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpKhRF */
			break;
			
		case ADP_IB_KRT_RF:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpKrtRF */
			break;
			
		case ADP_IB_KDC_RF:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8AdpKdcRF */
			break;
			
		case ADP_IB_USE_BACKUP_MEDIA:
			adpGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_bAdpUseBackupMedia */
			break;
#endif

		default:
			/* Unknown parameter. Copy it as it is...*/
			memcpy(&adpGetConfirm.m_au8AttributeValue, ptrMsg, adpGetConfirm.m_u8AttributeLength);
			break;
		}

		if (g_adp_sync_mgmt.f_sync_req && (g_adp_sync_mgmt.m_u32AttributeId == adpGetConfirm.m_u32AttributeId)) {
			/* Synchronous call -> Store the result */
			LOG_IFACE_G3_ADP("Synchronous Call pending; copying received structure\r\n");
			memcpy(&g_adp_sync_mgmt.s_GetConfirm, &adpGetConfirm, sizeof(struct TAdpGetConfirm));
			g_adp_sync_mgmt.f_sync_res = true;
		}

		if (g_adpNotifications.fnctAdpGetConfirm) {
			/* Asynchronous call -> Callback */
			g_adpNotifications.fnctAdpGetConfirm(&adpGetConfirm);
		}
	}

	return(true);
}

/**
 * @brief _cl_null_adpLBPConfirm_cb
 * The AdpLbpConfirm primitive allows the upper layer to be notified of the completion of a AdpLbpRequest
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpLBPConfirm_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpLBPConfirm_cb\r\n");
	/* Check the message length */
	if (len < 2) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adpNotifications.fnctAdpLbpConfirm) {
		struct TAdpLbpConfirm adpLbpConfirm;
		adpLbpConfirm.m_u8Status = (*ptrMsg++);
		adpLbpConfirm.m_u8NsduHandle = (*ptrMsg++);
		g_adpNotifications.fnctAdpLbpConfirm(&adpLbpConfirm);
	}

	return(true);
}

/**
 * @brief _cl_null_adpRouteDiscoveryConfirm_cb
 * The AdpRouteDiscoveryConfirm primitive allows the upper layer to be notified of the completion of a AdpRouteDiscoveryRequest.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpRouteDiscoveryConfirm_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpRouteDiscoveryConfirm_cb\r\n");
	/* Check the message length */
	if (len < 1) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adpNotifications.fnctAdpRouteDiscoveryConfirm) {
		struct TAdpRouteDiscoveryConfirm adpRouteDiscoveryConfirm;
		adpRouteDiscoveryConfirm.m_u8Status = (*ptrMsg++);
		g_adpNotifications.fnctAdpRouteDiscoveryConfirm(&adpRouteDiscoveryConfirm);
	}

	return(true);
}

/**
 * @brief _cl_null_adpPathDiscoveryConfirm_cb
 * The AdpPathDiscoveryConfirm primitive allows the upper layer to be notified of the completion of a AdpPathDiscoveryRequest.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpPathDiscoveryConfirm_cb(uint8_t *ptrMsg, uint16_t len)
{
	uint8_t i;

	LOG_IFACE_G3_ADP("_cl_null_adpPathDiscoveryConfirm_cb\r\n");
	/* Check the minimum message length */
	if (len < 8) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adpNotifications.fnctAdpPathDiscoveryConfirm) {
		struct TAdpPathDiscoveryConfirm adpPathDiscoveryConfirm;
		adpPathDiscoveryConfirm.m_u8Status = (*ptrMsg++);
		adpPathDiscoveryConfirm.m_u16DstAddr = (*ptrMsg++);
		adpPathDiscoveryConfirm.m_u16DstAddr = (*ptrMsg++) + (adpPathDiscoveryConfirm.m_u16DstAddr << 8);
		adpPathDiscoveryConfirm.m_u16OrigAddr = (*ptrMsg++);
		adpPathDiscoveryConfirm.m_u16OrigAddr = (*ptrMsg++) + (adpPathDiscoveryConfirm.m_u16OrigAddr << 8);
		adpPathDiscoveryConfirm.m_u8MetricType = (*ptrMsg++);
		adpPathDiscoveryConfirm.m_u8ForwardHopsCount = (*ptrMsg++);
		adpPathDiscoveryConfirm.m_u8ReverseHopsCount = (*ptrMsg++);

		/* Check the message length taking into account the number of hops */
		if (len < 8 + 7 * adpPathDiscoveryConfirm.m_u8ForwardHopsCount + 7 * adpPathDiscoveryConfirm.m_u8ReverseHopsCount) {
			/* ToDo: Log error */
			return(false);
		}

		for (i = 0; i < adpPathDiscoveryConfirm.m_u8ForwardHopsCount; i++) {
			adpPathDiscoveryConfirm.m_aForwardPath[i].m_u16HopAddress = (*ptrMsg++);
			adpPathDiscoveryConfirm.m_aForwardPath[i].m_u16HopAddress = (*ptrMsg++) + (adpPathDiscoveryConfirm.m_aForwardPath[i].m_u16HopAddress << 8);
			adpPathDiscoveryConfirm.m_aForwardPath[i].m_u8Mns = (*ptrMsg++);
			adpPathDiscoveryConfirm.m_aForwardPath[i].m_u8LinkCost = (*ptrMsg++);
			adpPathDiscoveryConfirm.m_aForwardPath[i].m_u8PhaseDiff = (*ptrMsg++);
			adpPathDiscoveryConfirm.m_aForwardPath[i].m_u8Mrx = (*ptrMsg++);
			adpPathDiscoveryConfirm.m_aForwardPath[i].m_u8Mtx = (*ptrMsg++);
		}
		for (i = 0; i < adpPathDiscoveryConfirm.m_u8ReverseHopsCount; i++) {
			adpPathDiscoveryConfirm.m_aReversePath[i].m_u16HopAddress = (*ptrMsg++);
			adpPathDiscoveryConfirm.m_aReversePath[i].m_u16HopAddress = (*ptrMsg++) + (adpPathDiscoveryConfirm.m_aReversePath[i].m_u16HopAddress << 8);
			adpPathDiscoveryConfirm.m_aReversePath[i].m_u8Mns = (*ptrMsg++);
			adpPathDiscoveryConfirm.m_aReversePath[i].m_u8LinkCost = (*ptrMsg++);
			adpPathDiscoveryConfirm.m_aReversePath[i].m_u8PhaseDiff = (*ptrMsg++);
			adpPathDiscoveryConfirm.m_aReversePath[i].m_u8Mrx = (*ptrMsg++);
			adpPathDiscoveryConfirm.m_aReversePath[i].m_u8Mtx = (*ptrMsg++);
		}
		/* Trigger the callback */
		g_adpNotifications.fnctAdpPathDiscoveryConfirm(&adpPathDiscoveryConfirm);
	}

	return(true);
}

/**
 * @brief _cl_null_adpMacSetConfirm_cb
 * The AdpMacSetConfirm primitive allows the upper layer to be notified of the completion of an AdpMacSetRequest
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpMacSetConfirm_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpMacSetConfirm_cb\r\n");
	/* Check the message length */
	if (len < 1) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adp_sync_mgmt.f_sync_req || g_adpNotifications.fnctAdpMacSetConfirm) {
		struct TAdpMacSetConfirm adpMacSetConfirm;
		adpMacSetConfirm.m_u8Status = (*ptrMsg++);
		adpMacSetConfirm.m_u32AttributeId = (*ptrMsg++);
		adpMacSetConfirm.m_u32AttributeId = (*ptrMsg++) + (adpMacSetConfirm.m_u32AttributeId << 8);
		adpMacSetConfirm.m_u32AttributeId = (*ptrMsg++) + (adpMacSetConfirm.m_u32AttributeId << 8);
		adpMacSetConfirm.m_u32AttributeId = (*ptrMsg++) + (adpMacSetConfirm.m_u32AttributeId << 8);
		adpMacSetConfirm.m_u16AttributeIndex = (*ptrMsg++);
		adpMacSetConfirm.m_u16AttributeIndex = (*ptrMsg++) + (adpMacSetConfirm.m_u16AttributeIndex << 8);

		LOG_IFACE_G3_ADP("Status:%d, AttributeId:0x%X,AttributeIndex=0x%X\r\n", adpMacSetConfirm.m_u8Status, adpMacSetConfirm.m_u32AttributeId, adpMacSetConfirm.m_u16AttributeIndex);
		if (g_adp_sync_mgmt.f_sync_req && (g_adp_sync_mgmt.m_u32AttributeId == adpMacSetConfirm.m_u32AttributeId)) {
			/* Synchronous call -> Store the result */
			LOG_IFACE_G3_ADP("Syncronous Call pending; copying received structure\r\n");
			memcpy(&g_adp_sync_mgmt.s_MacSetConfirm, &adpMacSetConfirm, sizeof(struct TAdpMacSetConfirm));
			g_adp_sync_mgmt.f_sync_res = true;
		}

		if (g_adpNotifications.fnctAdpMacSetConfirm) {
			/* Asynchronous call -> Callback */
			g_adpNotifications.fnctAdpMacSetConfirm(&adpMacSetConfirm);
		}
	}

	return(true);
}

/**
 * @brief _cl_null_adpMacGetConfirm_cb
 * The AdpMacGetConfirm primitive allows the upper layer to be notified of the completion of an AdpMacGetRequest.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpMacGetConfirm_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpMacGetConfirm_cb\r\n");
	/* Check the message length */
	if (len < 1) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adp_sync_mgmt.f_sync_req || g_adpNotifications.fnctAdpMacGetConfirm) {
		struct TAdpMacGetConfirm adpMacGetConfirm;
		adpMacGetConfirm.m_u8Status = (*ptrMsg++);
		adpMacGetConfirm.m_u32AttributeId = (*ptrMsg++);
		adpMacGetConfirm.m_u32AttributeId = (*ptrMsg++) + (adpMacGetConfirm.m_u32AttributeId << 8);
		adpMacGetConfirm.m_u32AttributeId = (*ptrMsg++) + (adpMacGetConfirm.m_u32AttributeId << 8);
		adpMacGetConfirm.m_u32AttributeId = (*ptrMsg++) + (adpMacGetConfirm.m_u32AttributeId << 8);
		adpMacGetConfirm.m_u16AttributeIndex = (*ptrMsg++);
		adpMacGetConfirm.m_u16AttributeIndex = (*ptrMsg++) + (adpMacGetConfirm.m_u16AttributeIndex << 8);
		adpMacGetConfirm.m_u8AttributeLength = (*ptrMsg++);
		if (adpMacGetConfirm.m_u8AttributeLength > 64) {
			LOG_IFACE_G3_ADP("_cl_null_adpMacGetConfirm_cb: Error, size attribute length exceeds maximum.\r\n\r\n");
			return(false);
		}

		LOG_IFACE_G3_ADP("Status:%d, AttributeId:0x%X,AttributeIndex=0x%X,AttributeLength=%d\r\n", adpMacGetConfirm.m_u8Status, adpMacGetConfirm.m_u32AttributeId, adpMacGetConfirm.m_u16AttributeIndex, adpMacGetConfirm.m_u8AttributeLength);

		switch (adpMacGetConfirm.m_u32AttributeId) {
		case MAC_WRP_PIB_ACK_WAIT_DURATION:
			/* u16AckWaitDuration */
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_BSN:
			/* m_u8Bsn */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_DSN:
			/* m_u8Dsn */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MAX_BE:
			/* m_u8MaxBe */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MAX_CSMA_BACKOFFS:
			/* m_u8MaxCsmaBackoffs */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MAX_FRAME_RETRIES:
			/* m_u8MaxFrameRetries */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MIN_BE:
			/* m_u8MinBe */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_PAN_ID:
			/* m_nPanId */
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_SECURITY_ENABLED:
			/* Boolean value */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_SHORT_ADDRESS:
			/* m_nShortAddress */
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_PROMISCUOUS_MODE:
			/* m_bPromiscuousMode */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_TIMESTAMP_SUPPORTED:
			/* Boolean value */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_KEY_TABLE:
			/* response will be MAC_WRP_STATUS_UNAVAILABLE_KEY */
			break;

		case MAC_WRP_PIB_FRAME_COUNTER:
			/* m_au32FrameCounter */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_HIGH_PRIORITY_WINDOW_SIZE:
			/* Boolean value */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_TX_DATA_PACKET_COUNT:
			/* m_u32TxDataPacketCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_RX_DATA_PACKET_COUNT:
			/* m_u32RxDataPacketCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_TX_CMD_PACKET_COUNT:
			/* m_u32TxCmdPacketCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_RX_CMD_PACKET_COUNT:
			/* m_u32RxCmdPacketCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_CSMA_FAIL_COUNT:
			/* m_u32CsmaFailCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_CSMA_NO_ACK_COUNT:
			/* m_u32CsmaNoAckCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_RX_DATA_BROADCAST_COUNT:
			/* m_u32TxDataBroadcastCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_TX_DATA_BROADCAST_COUNT:
			/* m_u32RxDataBroadcastCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_BAD_CRC_COUNT:
			/* m_u32BadCrcCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		/* In attributes whose pu8AttributeValue contains a struct with bit fields, */
		/* that bit fields use a whole bit in the attrib value of the serial interface. */
		/* The attrib. length must be adapted */
		case MAC_WRP_PIB_NEIGHBOUR_TABLE:
			if (adpMacGetConfirm.m_u8AttributeLength == sizeof(struct TMacWrpNeighbourEntry)) {
				struct TMacWrpNeighbourEntry s_aux_NE;
				s_aux_NE.m_nShortAddress = (uint16_t)((*ptrMsg++) << 8);
				s_aux_NE.m_nShortAddress += (uint16_t)(*ptrMsg++);
				memcpy((uint8_t *)&s_aux_NE.m_ToneMap.m_au8Tm[0], ptrMsg, (MAC_WRP_MAX_TONE_GROUPS + 7) / 8);
				ptrMsg += (MAC_WRP_MAX_TONE_GROUPS + 7) / 8;
				s_aux_NE.m_nModulationType = (uint8_t)(*ptrMsg++);
				s_aux_NE.m_nTxGain = (uint8_t)(*ptrMsg++);
				s_aux_NE.m_nTxRes = (uint8_t)(*ptrMsg++);
				memcpy((uint8_t *)&s_aux_NE.m_TxCoef.m_au8TxCoef[0], ptrMsg, 6);
				ptrMsg += 6;
				s_aux_NE.m_nModulationScheme = (uint8_t)(*ptrMsg++);
				s_aux_NE.m_nPhaseDifferential = (uint8_t)(*ptrMsg++);
				s_aux_NE.m_u8Lqi  = (uint8_t)(*ptrMsg++);
				s_aux_NE.m_u16TmrValidTime = (uint16_t)((*ptrMsg++) << 8);
				s_aux_NE.m_u16TmrValidTime += (uint16_t)(*ptrMsg++);
				s_aux_NE.m_u16NeighbourValidTime = (uint16_t)((*ptrMsg++) << 8);
				s_aux_NE.m_u16NeighbourValidTime += (uint16_t)(*ptrMsg++);

				memcpy(&adpMacGetConfirm.m_au8AttributeValue, &s_aux_NE, sizeof(struct TMacWrpNeighbourEntry));
				/* Struct save 2 bytes with bit-fields */
				adpMacGetConfirm.m_u8AttributeLength  -= 2;
				LOG_IFACE_G3_ADP(
						"\r\n\r\nNEIGHBOUR TABLE = m_nShortAddress = 0x%04x, m_nModulationType = 0x%02x, m_nTxGain = 0x%02x, m_nTxRes = 0x%02x, m_nModulationScheme = 0x%02x, m_nPhaseDifferential = 0x%02x, m_u8Lqi = 0x%04x, m_u16TmrValidTime = 0x%04x\r\n\r\n",
						s_aux_NE.m_nShortAddress, s_aux_NE.m_nModulationType, s_aux_NE.m_nTxGain, s_aux_NE.m_nTxRes, s_aux_NE.m_nModulationScheme, s_aux_NE.m_nPhaseDifferential, s_aux_NE.m_u8Lqi, s_aux_NE.m_u16TmrValidTime);
			} else if (adpMacGetConfirm.m_u8AttributeLength == 0) {
				/* The entry is empty, but the message must be forwarded. */
				LOG_IFACE_G3_ADP("\r\n\r\nNEIGHBOUR TABLE IS EMPTY.\r\n\r\n");
			} else {
				LOG_IFACE_G3_ADP("_cl_null_adpMacGetConfirm_cb ERROR in MAC_WRP_PIB_NEIGHBOUR_TABLE u8AttributeLength = %u is not 5.\r\n", adpMacGetConfirm.m_u8AttributeLength);
				return(false);
			}

			break;

		case MAC_WRP_PIB_FREQ_NOTCHING:
			/* m_bFreqNotching */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_POS_TABLE:
			if (adpMacGetConfirm.m_u8AttributeLength == 5) {
				struct TMacWrpPOSEntry s_aux_POS;
				adpMacGetConfirm.m_u8AttributeLength = sizeof(struct TMacWrpPOSEntry);
				s_aux_POS.m_nShortAddress = (uint16_t)((*ptrMsg++) << 8);
				s_aux_POS.m_nShortAddress += (uint16_t)(*ptrMsg++);
				s_aux_POS.m_u8Lqi = (uint8_t)(*ptrMsg++);
				s_aux_POS.m_u16POSValidTime = (uint16_t)((*ptrMsg++) << 8);
				s_aux_POS.m_u16POSValidTime += (uint16_t)(*ptrMsg++);
				memcpy(&adpMacGetConfirm.m_au8AttributeValue, &s_aux_POS, sizeof(struct TMacWrpPOSEntry));
			} else if (adpMacGetConfirm.m_u8AttributeLength == 0) {
				/* The entry is empty, but the message must be forwarded. */
				LOG_IFACE_G3_ADP("\r\n\r\nPOS TABLE IS EMPTY.\r\n\r\n");
			} else {
				LOG_IFACE_G3_ADP("_cl_null_adpMacGetConfirm_cb ERROR in MAC_WRP_PIB_POS_TABLE u8AttributeLength = %u is not 5.\r\n", adpMacGetConfirm.m_u8AttributeLength);
				return(false);
			}

			break;

		case MAC_WRP_PIB_CSMA_FAIRNESS_LIMIT:
			/* m_u8CsmaFairnessLimit */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_TMR_TTL:
			/* m_u8TmrTtl */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_POS_TABLE_ENTRY_TTL:                         /* MAC_WRP_PIB_NEIGHBOUR_TABLE_ENTRY_TTL also enters here as it has the same numeric value */
			/* m_u8NeighbourTableEntryTtl */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_RC_COORD:
			/* m_u16RcCoord */
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_TONE_MASK:
			memcpy((uint8_t *)&adpMacGetConfirm.m_au8AttributeValue[0],
					(uint8_t *)&ptrMsg[0], (MAC_WRP_MAX_TONES + 7) / 8);
			break;

		case MAC_WRP_PIB_BEACON_RANDOMIZATION_WINDOW_LENGTH:
			/* m_u8BeaconRandomizationWindowLength */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_A:
			/* m_u8A */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_K:
			/* m_u8K */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MIN_CW_ATTEMPTS:
			/* m_u8MinCwAttempts */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_CENELEC_LEGACY_MODE:
			/* PhyBoolGetLegacyMode */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_FCC_LEGACY_MODE:
			/* PhyBoolGetLegacyMode */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_BROADCAST_MAX_CW_ENABLE:
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];                         /* m_bBroadcastMaxCwEnable */
			break;

		case MAC_WRP_PIB_MANUF_DEVICE_TABLE:
			/* m_nPanId */
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[0], (uint8_t *)&adpMacGetConfirm.m_au8AttributeValue[0]);
			/* m_nShortAddress */
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[2], (uint8_t *)&adpMacGetConfirm.m_au8AttributeValue[2]);
			/* m_au32FrameCounter */
			mem_copy_from_usi_endianness_uint32((uint8_t *)&ptrMsg[4], (uint8_t *)&adpMacGetConfirm.m_au8AttributeValue[4]);
			break;

		case MAC_WRP_PIB_MANUF_EXTENDED_ADDRESS:
			/* m_au8Address */
			memcpy(adpMacGetConfirm.m_au8AttributeValue, ptrMsg, 8);
			break;

		/* In attributes whose pu8AttributeValue contains a struct with bit fields, */
		/* that bit fields use a whole bit in the attrib value of the serial interface. */
		/* The attrib. length must be adapted */
		case MAC_WRP_PIB_MANUF_NEIGHBOUR_TABLE_ELEMENT:
			if (adpMacGetConfirm.m_u8AttributeLength == sizeof(struct TMacWrpNeighbourEntry)) {
				struct TMacWrpNeighbourEntry s_aux_NE;
				s_aux_NE.m_nShortAddress = (uint16_t)((*ptrMsg++) << 8);
				s_aux_NE.m_nShortAddress += (uint16_t)(*ptrMsg++);
				memcpy((uint8_t *)&s_aux_NE.m_ToneMap.m_au8Tm[0], ptrMsg, (MAC_WRP_MAX_TONE_GROUPS + 7) / 8);
				ptrMsg += (MAC_WRP_MAX_TONE_GROUPS + 7) / 8;
				s_aux_NE.m_nModulationType = (uint8_t)(*ptrMsg++);
				s_aux_NE.m_nTxGain = (uint8_t)(*ptrMsg++);
				s_aux_NE.m_nTxRes = (uint8_t)(*ptrMsg++);
				memcpy((uint8_t *)&s_aux_NE.m_TxCoef.m_au8TxCoef[0], ptrMsg, 6);
				ptrMsg += 6;
				s_aux_NE.m_nModulationScheme = (uint8_t)(*ptrMsg++);
				s_aux_NE.m_nPhaseDifferential = (uint8_t)(*ptrMsg++);
				s_aux_NE.m_u8Lqi  = (uint8_t)(*ptrMsg++);
				s_aux_NE.m_u16TmrValidTime = (uint16_t)((*ptrMsg++) << 8);
				s_aux_NE.m_u16TmrValidTime += (uint16_t)(*ptrMsg++);
				s_aux_NE.m_u16NeighbourValidTime = (uint16_t)((*ptrMsg++) << 8);
				s_aux_NE.m_u16NeighbourValidTime += (uint16_t)(*ptrMsg++);

				memcpy(&adpMacGetConfirm.m_au8AttributeValue, &s_aux_NE, sizeof(struct TMacWrpNeighbourEntry));
				/* Struct save 2 bytes with bit-fields */
				adpMacGetConfirm.m_u8AttributeLength  -= 2;
				LOG_IFACE_G3_ADP(
						"\r\n\r\nNEIGHBOUR TABLE ELEMENT = m_nShortAddress = 0x%04x, m_nModulationType = 0x%02x, m_nTxGain = 0x%02x, m_nTxRes = 0x%02x, m_nModulationScheme = 0x%02x, m_nPhaseDifferential = 0x%02x, m_u8Lqi = 0x%04x, m_u16TmrValidTime = 0x%04x\r\n\r\n",
						s_aux_NE.m_nShortAddress, s_aux_NE.m_nModulationType, s_aux_NE.m_nTxGain, s_aux_NE.m_nTxRes, s_aux_NE.m_nModulationScheme, s_aux_NE.m_nPhaseDifferential, s_aux_NE.m_u8Lqi, s_aux_NE.m_u16TmrValidTime);
			} else if (adpMacGetConfirm.m_u8AttributeLength == 0) {
				/* The entry is empty, but the message must be forwarded. */
				LOG_IFACE_G3_ADP("\r\n\r\nNEIGHBOUR ELEMENT TABLE IS EMPTY.\r\n\r\n");
			} else {
				LOG_IFACE_G3_ADP("_cl_null_adpMacGetConfirm_cb ERROR in MAC_WRP_PIB_MANUF_NEIGHBOUR_TABLE_ELEMENT u8AttributeLength = %u is not %u.\r\n", adpMacGetConfirm.m_u8AttributeLength, sizeof(struct TMacWrpNeighbourEntry));
				return(false);
			}

			break;

		case MAC_WRP_PIB_MANUF_BAND_INFORMATION:
			/* m_u16FlMax */
			mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[0], (uint8_t *)&adpMacGetConfirm.m_au8AttributeValue[0]);
			/* m_u8Band */
			adpMacGetConfirm.m_au8AttributeValue[2] = ptrMsg[2];
			/* m_u8Tones */
			adpMacGetConfirm.m_au8AttributeValue[3] = ptrMsg[3];
			/* m_u8Carriers */
			adpMacGetConfirm.m_au8AttributeValue[4] = ptrMsg[4];
			/* m_u8TonesInCarrier */
			adpMacGetConfirm.m_au8AttributeValue[5] = ptrMsg[5];
			/* m_u8FlBand */
			adpMacGetConfirm.m_au8AttributeValue[6] = ptrMsg[6];
			/* m_u8MaxRsBlocks */
			adpMacGetConfirm.m_au8AttributeValue[7] = ptrMsg[7];
			/* m_u8TxCoefBits */
			adpMacGetConfirm.m_au8AttributeValue[8] = ptrMsg[8];
			/* m_u8PilotsFreqSpa */
			adpMacGetConfirm.m_au8AttributeValue[9] = ptrMsg[9];
			break;

		case MAC_WRP_PIB_MANUF_COORD_SHORT_ADDRESS:
			/* m_nCoordShortAddress */
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_MANUF_MAX_MAC_PAYLOAD_SIZE:
			/* u16MaxMacPayloadSize */
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_MANUF_SECURITY_RESET:
			/* Response will be mac_status_denied */
			break;

		case MAC_WRP_PIB_MANUF_FORCED_MOD_SCHEME:
			/* m_u8ForcedModScheme */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MANUF_FORCED_MOD_TYPE:
			/* m_u8ForcedModScheme */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MANUF_FORCED_TONEMAP:
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			adpMacGetConfirm.m_au8AttributeValue[1] = ptrMsg[1];
			adpMacGetConfirm.m_au8AttributeValue[2] = ptrMsg[2];
			break;

		case MAC_WRP_PIB_MANUF_FORCED_MOD_SCHEME_ON_TMRESPONSE:
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MANUF_FORCED_MOD_TYPE_ON_TMRESPONSE:
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MANUF_FORCED_TONEMAP_ON_TMRESPONSE:
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			adpMacGetConfirm.m_au8AttributeValue[1] = ptrMsg[1];
			adpMacGetConfirm.m_au8AttributeValue[2] = ptrMsg[2];
			break;

		case MAC_WRP_PIB_MANUF_LAST_RX_MOD_SCHEME:
			/* m_LastRxModScheme */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MANUF_LAST_RX_MOD_TYPE:
			/* m_LastRxModType */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MANUF_LBP_FRAME_RECEIVED:
			/* m_bLBPFrameReceived */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MANUF_LNG_FRAME_RECEIVED:
			/* m_bLNGFrameReceived */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MANUF_BCN_FRAME_RECEIVED:
			/* m_bBCNFrameReceived */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MANUF_NEIGHBOUR_TABLE_COUNT:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_MANUF_RX_OTHER_DESTINATION_COUNT:
			/* m_u32RxOtherDestinationCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_MANUF_RX_INVALID_FRAME_LENGTH_COUNT:
			/* m_u32RxInvalidFrameLengthCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_MANUF_RX_MAC_REPETITION_COUNT:
			/* m_u32RxMACRepetitionCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_MANUF_RX_WRONG_ADDR_MODE_COUNT:
			/* m_u32RxWrongAddrModeCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_MANUF_RX_UNSUPPORTED_SECURITY_COUNT:
			/* m_u32RxUnsupportedSecurityCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_MANUF_RX_WRONG_KEY_ID_COUNT:
			/* m_u32RxWrongKeyIdCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_MANUF_RX_INVALID_KEY_COUNT:
			/* m_u32RxInvalidKeyCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_MANUF_RX_WRONG_FC_COUNT:
			/* m_u32RxWrongFCCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_MANUF_RX_DECRYPTION_ERROR_COUNT:
			/* m_u32RxDecryptionErrorCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_MANUF_RX_SEGMENT_DECODE_ERROR_COUNT:
			/* m_u32RxSegmentDecodeErrorCount */
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_MANUF_ENABLE_MAC_SNIFFER:
			/* m_bMacSniffer */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MANUF_POS_TABLE_COUNT:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_PIB_MANUF_RETRIES_LEFT_TO_FORCE_ROBO:
			/* m_u8RetriesToForceRobo */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MANUF_MAC_INTERNAL_VERSION:
			/* Version */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];                         /* m_u8Major */
			adpMacGetConfirm.m_au8AttributeValue[1] = ptrMsg[1];                         /* m_u8Minor */
			adpMacGetConfirm.m_au8AttributeValue[2] = ptrMsg[2];                         /* m_u8Revision */
			adpMacGetConfirm.m_au8AttributeValue[3] = ptrMsg[3];                         /* m_u8Year */
			adpMacGetConfirm.m_au8AttributeValue[4] = ptrMsg[4];                         /* m_u8Month */
			adpMacGetConfirm.m_au8AttributeValue[5] = ptrMsg[5];                         /* m_u8Day */
			break;

		case MAC_WRP_PIB_MANUF_MAC_RT_INTERNAL_VERSION:
			/* Version */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];                         /* m_u8Major */
			adpMacGetConfirm.m_au8AttributeValue[1] = ptrMsg[1];                         /* m_u8Minor */
			adpMacGetConfirm.m_au8AttributeValue[2] = ptrMsg[2];                         /* m_u8Revision */
			adpMacGetConfirm.m_au8AttributeValue[3] = ptrMsg[3];                         /* m_u8Year */
			adpMacGetConfirm.m_au8AttributeValue[4] = ptrMsg[4];                         /* m_u8Month */
			adpMacGetConfirm.m_au8AttributeValue[5] = ptrMsg[5];                         /* m_u8Day */
			break;

		case MAC_WRP_PIB_MANUF_RESET_MAC_STATS:
			break;  /* If length is 0 then DeviceTable is going to be reset else response will be MAC_WRP_STATUS_INVALID_PARAMETER */

		case MAC_WRP_PIB_MANUF_SLEEP_MODE:
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_PIB_MANUF_DEBUG_SET:
			memcpy((uint8_t *)&adpMacGetConfirm.m_au8AttributeValue[0], ptrMsg[0], adpMacGetConfirm.m_u8AttributeLength);
			break;

		case MAC_WRP_PIB_MANUF_DEBUG_READ:
			/* MAC_WRP_STATUS_READ_ONLY */
			break;

		case MAC_WRP_PIB_MANUF_PHY_PARAM:
			switch (adpMacGetConfirm.m_u16AttributeIndex) {
			case MAC_WRP_PHY_PARAM_VERSION:
			case MAC_WRP_PHY_PARAM_TX_TOTAL:
			case MAC_WRP_PHY_PARAM_TX_TOTAL_BYTES:
			case MAC_WRP_PHY_PARAM_TX_TOTAL_ERRORS:
			case MAC_WRP_PHY_PARAM_BAD_BUSY_TX:
			case MAC_WRP_PHY_PARAM_TX_BAD_BUSY_CHANNEL:
			case MAC_WRP_PHY_PARAM_TX_BAD_LEN:
			case MAC_WRP_PHY_PARAM_TX_BAD_FORMAT:
			case MAC_WRP_PHY_PARAM_TX_TIMEOUT:
			case MAC_WRP_PHY_PARAM_RX_TOTAL:
			case MAC_WRP_PHY_PARAM_RX_TOTAL_BYTES:
			case MAC_WRP_PHY_PARAM_RX_RS_ERRORS:
			case MAC_WRP_PHY_PARAM_RX_EXCEPTIONS:
			case MAC_WRP_PHY_PARAM_RX_BAD_LEN:
			case MAC_WRP_PHY_PARAM_RX_BAD_CRC_FCH:
			case MAC_WRP_PHY_PARAM_RX_FALSE_POSITIVE:
			case MAC_WRP_PHY_PARAM_RX_BAD_FORMAT:
				mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
				break;

			case MAC_WRP_PHY_PARAM_ENABLE_AUTO_NOISE_CAPTURE:
				adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
				break;

			case MAC_WRP_PHY_PARAM_TIME_BETWEEN_NOISE_CAPTURES:
				mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
				break;

			case MAC_WRP_PHY_PARAM_DELAY_NOISE_CAPTURE_AFTER_RX:
				adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
				break;

			case MAC_WRP_PHY_PARAM_CFG_AUTODETECT_BRANCH:
				adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
				break;

			case MAC_WRP_PHY_PARAM_CFG_IMPEDANCE:
				adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
				break;

			case MAC_WRP_PHY_PARAM_RRC_NOTCH_ACTIVE:
				adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
				break;

			case MAC_WRP_PHY_PARAM_RRC_NOTCH_INDEX:
				adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
				break;

			case MAC_WRP_PHY_PARAM_PLC_DISABLE:
				adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
				break;

			case MAC_WRP_PHY_PARAM_NOISE_PEAK_POWER:
				adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
				break;

			case MAC_WRP_PHY_PARAM_LAST_MSG_LQI:
				adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
				break;

			case MAC_WRP_PHY_PARAM_LAST_MSG_RSSI:
				mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
				break;

			case MAC_WRP_PHY_PARAM_ACK_TX_CFM:
				mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
				break;

			case MAC_WRP_PHY_PARAM_TONE_MAP_RSP_ENABLED_MODS:
				adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
				break;
/* Write only
			case MAC_WRP_PHY_PARAM_RESET_PHY_STATS:
				break;
*/
			default:
				break;
			}
			break;

#ifdef G3_HYBRID_PROFILE
	case MAC_WRP_PIB_DSN_RF:
		/* m_u8DsnRF */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_MAX_BE_RF:
		/* m_u8MaxBeRF */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_MAX_CSMA_BACKOFFS_RF:
		/* m_u8MaxCsmaBackoffsRF */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_MAX_FRAME_RETRIES_RF:
		/* m_u8MaxFrameRetriesRF */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_MIN_BE_RF:
		/* m_u8MinBeRF */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_TIMESTAMP_SUPPORTED_RF:
		/* 8 bits (bool) */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_DEVICE_TABLE_RF:
		/* m_nPanId */
		mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[0], (uint8_t *)&adpMacGetConfirm.m_au8AttributeValue[0]);
		/* m_nShortAddress */
		mem_copy_from_usi_endianness_uint16((uint8_t *)&ptrMsg[2], (uint8_t *)&adpMacGetConfirm.m_au8AttributeValue[2]);
		/* m_au32FrameCounter */
		mem_copy_from_usi_endianness_uint32((uint8_t *)&ptrMsg[4], (uint8_t *)&adpMacGetConfirm.m_au8AttributeValue[4]);
		break;
	case MAC_WRP_PIB_FRAME_COUNTER_RF:
		/* m_u32FrameCounterRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_DUPLICATE_DETECTION_TTL_RF:
		/* m_u8DuplicateDetectionTtlRF */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_COUNTER_OCTETS_RF:
		/* 8 bits */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_RETRY_COUNT_RF:
		/* m_u32RetryCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MULTIPLE_RETRY_COUNT_RF:
		/* m_u32MultipleRetryCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_TX_FAIL_COUNT_RF:
		/* m_u32TxFailCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_TX_SUCCESS_COUNT_RF:
		/* m_u32TxSuccessCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_FCS_ERROR_COUNT_RF:
		/* m_u32FcsErrorCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_SECURITY_FAILURE_COUNT_RF:
		/* m_u32SecurityFailureCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_DUPLICATE_FRAME_COUNT_RF:
		/* m_u32DuplicateFrameCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_RX_SUCCESS_COUNT_RF:
		/* m_u32RxSuccessCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_NACK_COUNT_RF:
		/* m_u32NackCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_USE_ENHANCED_BEACON_RF:
		/* 8 bits (bool) */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_EB_HEADER_IE_LIST_RF:
		/* Array of 1 byte */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_EB_PAYLOAD_IE_LIST_RF:
		/* This IB is an empty array */
		break;
	case MAC_WRP_PIB_EB_FILTERING_ENABLED_RF:
		/* 8 bits (bool) */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_EBSN_RF:
		/* m_u8EBsnRF */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_EB_AUTO_SA_RF:
		/* 8 bits */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_SEC_SECURITY_LEVEL_LIST_RF:
		/* 4 Byte entries. */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8FrameType */
		adpMacGetConfirm.m_au8AttributeValue[1] = ptrMsg[1]; /* m_u8CommandId */
		adpMacGetConfirm.m_au8AttributeValue[2] = ptrMsg[2]; /* m_u8SecurityMinimum */
		adpMacGetConfirm.m_au8AttributeValue[3] = ptrMsg[3]; /* m_bOverrideSecurityMinimum */
		break;
	case MAC_WRP_PIB_POS_TABLE_RF: /* 9 Byte entries. */
		if (adpMacGetConfirm.m_u8AttributeLength == 9) {
			struct TMacWrpPOSEntryRF s_aux_POS;
			adpMacGetConfirm.m_u8AttributeLength = sizeof(struct TMacWrpPOSEntryRF);
			s_aux_POS.m_nShortAddress = (uint16_t)((*ptrMsg++) << 8);
			s_aux_POS.m_nShortAddress += (uint16_t)(*ptrMsg++);
			s_aux_POS.m_u8ForwardLqi = (uint8_t)(*ptrMsg++);
			s_aux_POS.m_u8ReverseLqi = (uint8_t)(*ptrMsg++);
			s_aux_POS.m_u8DutyCycle = (uint8_t)(*ptrMsg++);
			s_aux_POS.m_u8ForwardTxPowerOffset = (uint8_t)(*ptrMsg++);
			s_aux_POS.m_u8ReverseTxPowerOffset = (uint8_t)(*ptrMsg++);
			s_aux_POS.m_u16POSValidTime = (uint16_t)((*ptrMsg++) << 8);
			s_aux_POS.m_u16POSValidTime += (uint16_t)(*ptrMsg++);
			memcpy(&adpMacGetConfirm.m_au8AttributeValue, &s_aux_POS, sizeof(struct TMacWrpPOSEntry));
		} else if (adpMacGetConfirm.m_u8AttributeLength == 0) {
			/* The entry is empty, but the message must be forwarded. */
			LOG_IFACE_G3_ADP("\r\n\r\nPOS TABLE IS EMPTY.\r\n\r\n");
		} else {
			LOG_IFACE_G3_ADP("_cl_null_adpMacGetConfirm_cb ERROR in MAC_WRP_PIB_POS_TABLE_RF u8AttributeLength = %u is not 9.\r\n", adpMacGetConfirm.m_u8AttributeLength);
			return false;
		}
		break;
	case MAC_WRP_PIB_OPERATING_MODE_RF:
		/* m_u8OperatingModeRF */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_CHANNEL_NUMBER_RF:
		/* m_u16ChannelNumberRF */
		mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_DUTY_CYCLE_USAGE_RF:
		/* m_u8DutyCycleUsageRF */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_DUTY_CYCLE_PERIOD_RF:
		/* m_u16DutyCyclePeriodRF */
		mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_DUTY_CYCLE_LIMIT_RF:
		/* m_u16DutyCycleLimitRF */
		mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_DUTY_CYCLE_THRESHOLD_RF:
		/* m_u8DutyCycleThresholdRF */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_DISABLE_PHY_RF:
		/* m_bDisablePhyRF */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	/* Manufacturer specific */
	case MAC_WRP_PIB_MANUF_SECURITY_RESET_RF:
		/*  8 bits (bool) */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_MANUF_LBP_FRAME_RECEIVED_RF:
		/* m_bLBPFrameReceivedRF */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_MANUF_LNG_FRAME_RECEIVED_RF:
		/* m_bLNGFrameReceivedRF */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_MANUF_BCN_FRAME_RECEIVED_RF:
		/* m_bBCNFrameReceivedRF */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_MANUF_RX_OTHER_DESTINATION_COUNT_RF:
		/* m_u32RxOtherDestinationCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_RX_INVALID_FRAME_LENGTH_COUNT_RF:
		/* m_u32RxInvalidFrameLengthCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_RX_WRONG_ADDR_MODE_COUNT_RF:
		/* m_u32RxWrongAddrModeCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_RX_UNSUPPORTED_SECURITY_COUNT_RF:
		/* m_u32RxUnsupportedSecurityCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_RX_WRONG_KEY_ID_COUNT_RF:
		/* m_u32RxWrongKeyIdCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_RX_INVALID_KEY_COUNT_RF:
		/* m_u32RxInvalidKeyCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_RX_WRONG_FC_COUNT_RF:
		/* m_u32RxWrongFCCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_RX_DECRYPTION_ERROR_COUNT_RF:
		/* m_u32RxDecryptionErrorCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_TX_DATA_PACKET_COUNT_RF:
		/* m_u32TxDataPacketCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_RX_DATA_PACKET_COUNT_RF:
		/* m_u32RxDataPacketCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_TX_CMD_PACKET_COUNT_RF:
		/* m_u32TxCmdPacketCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_RX_CMD_PACKET_COUNT_RF:
		/* m_u32RxCmdPacketCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_CSMA_FAIL_COUNT_RF:
		/* m_u32CsmaFailCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_RX_DATA_BROADCAST_COUNT_RF:
		/* m_u32RxDataBroadcastCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_TX_DATA_BROADCAST_COUNT_RF:
		/* m_u32TxDataBroadcastCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_BAD_CRC_COUNT_RF:
		/* m_u32BadCrcCountRF */
		mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_ENABLE_MAC_SNIFFER_RF:
		/* m_bMacSnifferRF */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_MANUF_POS_TABLE_COUNT_RF:
		/* 16 bits */
		mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
		break;
	case MAC_WRP_PIB_MANUF_MAC_INTERNAL_VERSION_RF:
		/* Version */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8Major */
		adpMacGetConfirm.m_au8AttributeValue[1] = ptrMsg[1]; /* m_u8Minor */
		adpMacGetConfirm.m_au8AttributeValue[2] = ptrMsg[2]; /* m_u8Revision */
		adpMacGetConfirm.m_au8AttributeValue[3] = ptrMsg[3]; /* m_u8Year */
		adpMacGetConfirm.m_au8AttributeValue[4] = ptrMsg[4]; /* m_u8Month */
		adpMacGetConfirm.m_au8AttributeValue[5] = ptrMsg[5]; /* m_u8Day */
		break;
	case MAC_WRP_PIB_MANUF_RESET_MAC_STATS_RF:
		/* 8 bits */
		adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
		break;
	case MAC_WRP_PIB_MANUF_POS_TABLE_ELEMENT_RF: /* 9 Byte entries. */
		if (adpMacGetConfirm.m_u8AttributeLength == 9) {
			struct TMacWrpPOSEntryRF s_aux_POS;
			adpMacGetConfirm.m_u8AttributeLength = sizeof(struct TMacWrpPOSEntryRF);
			s_aux_POS.m_nShortAddress = (uint16_t)((*ptrMsg++) << 8);
			s_aux_POS.m_nShortAddress += (uint16_t)(*ptrMsg++);
			s_aux_POS.m_u8ForwardLqi = (uint8_t)(*ptrMsg++);
			s_aux_POS.m_u8ReverseLqi = (uint8_t)(*ptrMsg++);
			s_aux_POS.m_u8DutyCycle = (uint8_t)(*ptrMsg++);
			s_aux_POS.m_u8ForwardTxPowerOffset = (uint8_t)(*ptrMsg++);
			s_aux_POS.m_u8ReverseTxPowerOffset = (uint8_t)(*ptrMsg++);
			s_aux_POS.m_u16POSValidTime = (uint16_t)((*ptrMsg++) << 8);
			s_aux_POS.m_u16POSValidTime += (uint16_t)(*ptrMsg++);
			memcpy(&adpMacGetConfirm.m_au8AttributeValue, &s_aux_POS, sizeof(struct TMacWrpPOSEntry));
		} else if (adpMacGetConfirm.m_u8AttributeLength == 0) {
			/* The entry is empty, but the message must be forwarded. */
			LOG_IFACE_G3_ADP("\r\n\r\nPOS TABLE IS EMPTY.\r\n\r\n");
		} else {
			LOG_IFACE_G3_ADP("_cl_null_adpMacGetConfirm_cb ERROR in MAC_WRP_PIB_POS_TABLE_RF u8AttributeLength = %u is not 9.\r\n", adpMacGetConfirm.m_u8AttributeLength);
			return false;
		}
		break;
	case MAC_WRP_PIB_MANUF_PHY_PARAM_RF:
		switch (adpMacGetConfirm.m_u16AttributeIndex) {
		case MAC_WRP_RF_PHY_PARAM_PHY_CHANNEL_FREQ_HZ:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_TOTAL:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_TOTAL_BYTES:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_TOTAL:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_BUSY_TX:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_BUSY_RX:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_BUSY_CHN:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_BAD_LEN:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_BAD_FORMAT:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_TIMEOUT:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_ABORTED:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_CFM_NOT_HANDLED:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_TOTAL:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_TOTAL_BYTES:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_TOTAL:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_FALSE_POSITIVE:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_BAD_LEN:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_BAD_FORMAT:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_BAD_FCS_PAY:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_ABORTED:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_OVERRIDE:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_IND_NOT_HANDLED:
			mem_copy_from_usi_endianness_uint32((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_RF_PHY_PARAM_DEVICE_ID:
		case MAC_WRP_RF_PHY_PARAM_PHY_BAND_OPERATING_MODE:
		case MAC_WRP_RF_PHY_PARAM_PHY_CHANNEL_NUM:
		case MAC_WRP_RF_PHY_PARAM_PHY_CCA_ED_DURATION:
		case MAC_WRP_RF_PHY_PARAM_PHY_TURNAROUND_TIME:
		case MAC_WRP_RF_PHY_PARAM_PHY_TX_PAY_SYMBOLS:
		case MAC_WRP_RF_PHY_PARAM_PHY_RX_PAY_SYMBOLS:
		case MAC_WRP_RF_PHY_PARAM_MAC_UNIT_BACKOFF_PERIOD:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue);
			break;

		case MAC_WRP_RF_PHY_PARAM_DEVICE_RESET:
		case MAC_WRP_RF_PHY_PARAM_TRX_RESET:
		case MAC_WRP_RF_PHY_PARAM_TRX_SLEEP:
		case MAC_WRP_RF_PHY_PARAM_PHY_CCA_ED_THRESHOLD:
		case MAC_WRP_RF_PHY_PARAM_PHY_STATS_RESET:
		case MAC_WRP_RF_PHY_PARAM_TX_FSK_FEC:
		case MAC_WRP_RF_PHY_PARAM_TX_OFDM_MCS:
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0];
			break;

		case MAC_WRP_RF_PHY_PARAM_FW_VERSION:
			/* Version */
			adpMacGetConfirm.m_au8AttributeValue[0] = ptrMsg[0]; /* m_u8Major */
			adpMacGetConfirm.m_au8AttributeValue[1] = ptrMsg[1]; /* m_u8Minor */
			adpMacGetConfirm.m_au8AttributeValue[2] = ptrMsg[2]; /* m_u8Revision */
			adpMacGetConfirm.m_au8AttributeValue[3] = ptrMsg[3]; /* m_u8Year */
			adpMacGetConfirm.m_au8AttributeValue[4] = ptrMsg[4]; /* m_u8Month */
			adpMacGetConfirm.m_au8AttributeValue[5] = ptrMsg[5]; /* m_u8Day */
			break;
			
		case MAC_WRP_RF_PHY_PARAM_PHY_CCA_ED_CONFIG:
			mem_copy_from_usi_endianness_uint16((uint8_t *)ptrMsg, adpMacGetConfirm.m_au8AttributeValue); /* us_duration_us */
			adpMacGetConfirm.m_au8AttributeValue[2] = ptrMsg[2]; /* sc_threshold_dBm */
			break;

		default:
			break;
		}
		break;
#endif

		default:
			/* Unknown parameter. Copy it as it is...*/
			memcpy(&adpMacGetConfirm.m_au8AttributeValue, ptrMsg, adpMacGetConfirm.m_u8AttributeLength);
			break;
		} /* switch */

		if (g_adp_sync_mgmt.f_sync_req && (g_adp_sync_mgmt.m_u32AttributeId == adpMacGetConfirm.m_u32AttributeId)) {
			/* Synchronous call -> Store the result */
			LOG_IFACE_G3_ADP("Syncronous Call pending; copying received structure\r\n");
			memcpy(&g_adp_sync_mgmt.s_MacGetConfirm, &adpMacGetConfirm, sizeof(struct TAdpMacGetConfirm));
			g_adp_sync_mgmt.f_sync_res = true;
		}

		if (g_adpNotifications.fnctAdpMacGetConfirm) {
			g_adpNotifications.fnctAdpMacGetConfirm(&adpMacGetConfirm);
		}
	}

	return(true);
}

/**
 * @brief _cl_null_adpDiscoveryIndication_cb
 * The AdpDiscoveryIndication primitive is generated by the ADP layer to notify the application about the discovery
 * of a new PAN coordinator or LBA
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpDiscoveryIndication_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpDiscoveryIndication_cb\r\n");
	/* Check the message length */
	if (len < 1) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adpNotifications.fnctAdpDiscoveryIndication) {
		struct TAdpPanDescriptor adpPanDescriptor;
		adpPanDescriptor.m_u16PanId = (*ptrMsg++);
		adpPanDescriptor.m_u16PanId = (*ptrMsg++) + (adpPanDescriptor.m_u16PanId << 8);
		adpPanDescriptor.m_u8LinkQuality = (*ptrMsg++);
		adpPanDescriptor.m_u16LbaAddress = (*ptrMsg++);
		adpPanDescriptor.m_u16LbaAddress = (*ptrMsg++) + (adpPanDescriptor.m_u16LbaAddress << 8);
		adpPanDescriptor.m_u16RcCoord = (*ptrMsg++);
		adpPanDescriptor.m_u16RcCoord = (*ptrMsg++) + (adpPanDescriptor.m_u16RcCoord << 8);
#ifdef G3_HYBRID_PROFILE
		adpPanDescriptor.m_u8MediaType = (*ptrMsg++);
#endif
		g_adpNotifications.fnctAdpDiscoveryIndication(&adpPanDescriptor);
	}

	return(true);
}

/**
 * @brief _cl_null_adpBufferIndication_cb
 * The AdpBufferIndication primitive allows the next higher layer to be notified when the modem has reached its
 * capability limit to perform the next frame.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpBufferIndication_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpBufferIndication_cb\r\n");
	/* Check the message length */
	if (len < 1) {
		LOG_IFACE_G3_ADP("ERROR: wrong message length.\r\n");
		return(false);
	}

	if (g_adpNotifications.fnctAdpBufferIndication) {
		struct TAdpBufferIndication adpBufferIndication;
		adpBufferIndication.m_bBufferReady = (*ptrMsg++) ? true : false;
		g_adpNotifications.fnctAdpBufferIndication(&adpBufferIndication);
	}

	return(true);
}

/**
 * @brief _cl_null_adpUpdNonVolatileDataIndication_cb
 * The AdpUpdNonVolatileDataIndication primitive allows the next higher layer to be notified when non-volatile stored
 * data must be updated to protect the system in case of critical failure.
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpUpdNonVolatileDataIndication_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpUpdNonVolatileDataIndication_cb\r\n");
	if (g_adpNotifications.fnctAdpUpdNonVolatileDataIndication) {
		g_adpNotifications.fnctAdpUpdNonVolatileDataIndication();
	}

	return(true);
}

/**
 * @brief _cl_null_adpPREQIndication_cb
 * The AdpPREQIndication primitive allows the next higher layer to be notified when a PREQ frame is received
 * in unicast mode with Originator Address equal to Coordinator Address and with Destination Address equal to Device Address
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpPREQIndication_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpPREQIndication_cb\r\n");
	if (g_adpNotifications.fnctAdpPREQIndication) {
		g_adpNotifications.fnctAdpPREQIndication();
	}

	return(true);
}

/**
 * @brief _cl_null_adpRouteNotFoundIndication_cb
 * The AdpRouteNotFoundIndication primitive is used to indicate the upper layer that a route is not available
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_adpRouteNotFoundIndication_cb(uint8_t *ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_ADP("_cl_null_adpRouteNotFoundIndication_cb\r\n");
#if 0
	if (g_adpNotifications.fnctAdpRouteNotFoundIndication) {
		struct TAdpRouteNotFoundIndication adpRouteNotFoundIndication;
		adpRouteNotFoundIndication.m_u16SrcAddr = (uint16_t)((*ptrMsg++) << 8);
		adpRouteNotFoundIndication.m_u16SrcAddr += (uint16_t)(*ptrMsg++);
		adpRouteNotFoundIndication.m_u16DestAddr = (uint16_t)((*ptrMsg++) << 8);
		adpRouteNotFoundIndication.m_u16DestAddr += (uint16_t)(*ptrMsg++);
		adpRouteNotFoundIndication.m_u16NextHopAddr = (uint16_t)((*ptrMsg++) << 8);
		adpRouteNotFoundIndication.m_u16NextHopAddr += (uint16_t)(*ptrMsg++);
		adpRouteNotFoundIndication.m_u16PreviousHopAddr = (uint16_t)((*ptrMsg++) << 8);
		adpRouteNotFoundIndication.m_u16PreviousHopAddr += (uint16_t)(*ptrMsg++);
		adpRouteNotFoundIndication.m_u16RouteCost = (uint16_t)((*ptrMsg++) << 8);
		adpRouteNotFoundIndication.m_u16RouteCost += (uint16_t)(*ptrMsg++);
		adpRouteNotFoundIndication.m_u8HopCount = *ptrMsg++;
		adpRouteNotFoundIndication.m_u8WeakLinkCount = *ptrMsg++;
		adpRouteNotFoundIndication.m_bRouteJustBroken = (bool)(*ptrMsg++);
		adpRouteNotFoundIndication.m_bCompressedHeader = (bool)(*ptrMsg++);
		adpRouteNotFoundIndication.m_u16NsduLength = (uint16_t)((*ptrMsg++) << 8);
		adpRouteNotFoundIndication.m_u16NsduLength += (uint16_t)(*ptrMsg++);
		adpRouteNotFoundIndication.m_pNsdu = (uint8_t *)malloc(adpRouteNotFoundIndication.m_u16NsduLength * sizeof(uint8_t));
		memcpy(adpRouteNotFoundIndication.m_pNsdu, ptrMsg, adpRouteNotFoundIndication.m_u16NsduLength);

		g_adpNotifications.fnctAdpRouteNotFoundIndication(&adpRouteNotFoundIndication);
		free(adpRouteNotFoundIndication.m_pNsdu);
	}
#endif
	return(true);
}

/**
 * @brief g3_ADP_receivedCmd
 * Copies in the Reception buffer the data received in the USI
 * @param ptrMsg: Message Data of USI Frame
 * @param len: Message Data length of USI Frame
 * @return TRUE if is possible to extract from the usi buffer
 */
Uint8 g3_ADP_receivedCmd(Uint8 *ptrMsg, Uint16 len)
{
	uint8_t uc_cmd;

	/* Command in first byte of Message Data of USI Frame */
	uc_cmd = (*ptrMsg++);
	len--;

	LOG_IFACE_G3_ADP("g3_ADP_receivedCmd: 0x%X\r\n", uc_cmd);
	switch (uc_cmd) {
	case G3_SERIAL_MSG_STATUS:
		return _cl_null_adpStatus_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_DATA_INDICATION:
		return _cl_null_adpDataIndication_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_DATA_CONFIRM:
		return _cl_null_adpDataConfirm_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_DISCOVERY_CONFIRM:
		return _cl_null_adpDiscoveryConfirm_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_NETWORK_START_CONFIRM:
		return _cl_null_adpNetworkStartConfirm_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_NETWORK_JOIN_CONFIRM:
		return _cl_null_adpNetworkJoinConfirm_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_NETWORK_LEAVE_CONFIRM:
		return _cl_null_adpNetworkLeaveConfirm_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_NETWORK_LEAVE_INDICATION:
		return _cl_null_adpNetworkLeaveIndication_cb();

	case G3_SERIAL_MSG_ADP_RESET_CONFIRM:
		return _cl_null_adpResetConfirm_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_SET_CONFIRM:
		return _cl_null_adpSetConfirm_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_GET_CONFIRM:
		return _cl_null_adpGetConfirm_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_LBP_CONFIRM:
		return _cl_null_adpLBPConfirm_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_LBP_INDICATION:
		return _cl_null_adpLBPIndication_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_ROUTE_DISCOVERY_CONFIRM:
		return _cl_null_adpRouteDiscoveryConfirm_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_PATH_DISCOVERY_CONFIRM:
		return _cl_null_adpPathDiscoveryConfirm_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_MAC_SET_CONFIRM:
		return _cl_null_adpMacSetConfirm_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_MAC_GET_CONFIRM:
		return _cl_null_adpMacGetConfirm_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_DISCOVERY_INDICATION:
		return _cl_null_adpDiscoveryIndication_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_BUFFER_INDICATION:
		return _cl_null_adpBufferIndication_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_PREQ_INDICATION:
		return _cl_null_adpPREQIndication_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_NETWORK_STATUS_INDICATION:
		return _cl_null_adpNetworkStatusIndication_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_UPD_NON_VOLATILE_DATA_INDICATION:
		return _cl_null_adpUpdNonVolatileDataIndication_cb(ptrMsg, len);

	case G3_SERIAL_MSG_ADP_ROUTE_NOT_FOUND_INDICATION:
		return _cl_null_adpRouteNotFoundIndication_cb(ptrMsg, len);

	/* Commands not implemented in callback */
	case G3_SERIAL_MSG_ADP_INITIALIZE:
	case G3_SERIAL_MSG_ADP_DATA_REQUEST:
	case G3_SERIAL_MSG_ADP_DISCOVERY_REQUEST:
	case G3_SERIAL_MSG_ADP_NETWORK_START_REQUEST:
	case G3_SERIAL_MSG_ADP_NETWORK_JOIN_REQUEST:
	case G3_SERIAL_MSG_ADP_NETWORK_LEAVE_REQUEST:
	case G3_SERIAL_MSG_ADP_RESET_REQUEST:
	case G3_SERIAL_MSG_ADP_SET_REQUEST:
	case G3_SERIAL_MSG_ADP_GET_REQUEST:
	case G3_SERIAL_MSG_ADP_LBP_REQUEST:
	case G3_SERIAL_MSG_ADP_ROUTE_DISCOVERY_REQUEST:
	case G3_SERIAL_MSG_ADP_PATH_DISCOVERY_REQUEST:
	case G3_SERIAL_MSG_ADP_MAC_SET_REQUEST:
	case G3_SERIAL_MSG_ADP_MAC_GET_REQUEST:
	default:
		return(false);

		break;
	}

	return(true);
}

void mem_copy_to_usi_endianness_uint32( uint8_t *puc_dst, uint8_t *puc_src)
{
	uint32_t ul_aux;

	memcpy((uint8_t *)&ul_aux, puc_src, 4);

	*puc_dst++ = (uint8_t)(ul_aux >> 24);
	*puc_dst++ = (uint8_t)(ul_aux >> 16);
	*puc_dst++ = (uint8_t)(ul_aux >> 8);
	*puc_dst++ = (uint8_t)(ul_aux);
}

void mem_copy_to_usi_endianness_uint16( uint8_t *puc_dst, uint8_t *puc_src)
{
	uint16_t us_aux;

	memcpy((uint8_t *)&us_aux, puc_src, 2);

	*puc_dst++ = (uint8_t)(us_aux >> 8);
	*puc_dst++ = (uint8_t)(us_aux);
}

void mem_copy_from_usi_endianness_uint32(uint8_t *puc_src, uint8_t *puc_dst)
{
	uint32_t ul_aux = 0;

	ul_aux = (*puc_src++) << 24;
	ul_aux += (*puc_src++) << 16;
	ul_aux += (*puc_src++) << 8;
	ul_aux += *puc_src++;

	memcpy(puc_dst, (uint8_t *)&ul_aux, 4);
}

void mem_copy_from_usi_endianness_uint16(uint8_t *puc_src, uint8_t *puc_dst)
{
	uint16_t us_aux = 0;

	us_aux += (*puc_src++) << 8;
	us_aux += *puc_src++;

	memcpy(puc_dst, (uint8_t *)&us_aux, 2);
}
