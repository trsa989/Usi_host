/**
 * \file  ifaceG3Mac.c
 *
 * \brief USI Host Interface to G3 MAC Layer
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

//*************************************Includes*********************************
#include <AdpApi.h>

/* System includes */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "../addUsi.h"
#include "../G3.h"
#include "usi.h"

#include "mac_wrapper_defs.h"
#include "mac_wrapper.h"

/* Enable traces to show them in console stderr (0 or 1)*/
//#define DEBUG_IFACE

#ifdef DEBUG_IFACE
#define LOG_IFACE_G3_MAC(...) fprintf(stderr, "[IFACE]: ") ; fprintf(stderr, __VA_ARGS__)
#define debug_print(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG_IFACE_G3_MAC(...) do {} while (0)
#define debug_print(...) do {} while (0)
#endif

#define MAX_SIZE_MAC_BUFFER	G3_MACSAP_DATA_SIZE + (G3_MACSAP_DATA_SIZE / 2)

/* Minumum message lengths */
#define MIN_MAC_STATUS_MSG_LEN                   1
#define MIN_MAC_DATA_INDICATION_MSG_LEN          31
#define MIN_MAC_DATA_CONFIRM_MSG_LEN             6
#define MIN_MAC_RESET_CONFIRM_MSG_LEN            1
#define MIN_MAC_SET_CONFIRM_MSG_LEN              7
#define MIN_MAC_GET_CONFIRM_MSG_LEN              8
#define MIN_MAC_SCAN_CONFIRM_MSG_LEN             1
#define MIN_MAC_START_CONFIRM_MSG_LEN            1
#define MIN_MAC_BEACON_NOTIFY_MSG_LEN            7
#define MIN_MAC_COMM_STATUS_INDICATION_MSG_LEN   11

//// Transmission buffer
static uint8_t buffTxMacG3[G3_MACSAP_DATA_SIZE];
static CmdParams macG3Msg;

/* Callbacks */
struct TMacWrpNotifications g_macNotifications;
#ifdef G3_HYBRID_PROFILE
struct TMacWrpNotifications g_macNotificationsRF;
#endif

//*** Structures ************************************************************
typedef struct
{
	bool f_sync_req; // Flag to indicate syncronous request
	bool f_sync_res; // Flag to indicate syncronous response
	struct TMacWrpGetConfirm s_GetConfirm;
	struct TMacWrpSetConfirm s_SetConfirm;
} T_mac_sync_mgmt;

T_mac_sync_mgmt g_mac_sync_mgmt;
#ifdef G3_HYBRID_PROFILE
T_mac_sync_mgmt g_mac_sync_mgmt_rf;
#endif

/**********************************************************************************************************************/
/** Forward declaration
 **********************************************************************************************************************/

/**********************************************************************************************************************/
/** Use this function to initialize the MAC layer. The MAC layer should be initialized before doing any other operation.
 * The APIs cannot be mixed, if the stack is initialized in MAC mode then only the MAC functions can be used and if the
 * stack is initialized in ADP mode then only ADP functions can be used.
 * @param pNotifications Structure with callbacks used to notify MAC specific events (if NULL the layer is deinitialized)
 * @param band Working band (should be inline with the hardware)
 **********************************************************************************************************************/
void MacWrapperInitialize(struct TMacWrpNotifications *pNotifications, uint8_t band)
{
	uint8_t *ptrBuff;
	uint16_t length;
	uint32_t result;

	LOG_IFACE_G3_MAC("MacWrapperInitialize...");
	
	// Copy callbacks
	memcpy(&g_macNotifications, pNotifications, sizeof(struct TMacWrpNotifications));

	ptrBuff = buffTxMacG3;

	*ptrBuff++ = G3_SERIAL_MSG_MAC_INITIALIZE;
	*ptrBuff++ = band;

	length = ptrBuff - buffTxMacG3;
	macG3Msg.pType = PROTOCOL_MAC_G3;
	macG3Msg.buf = buffTxMacG3;
	macG3Msg.len = length;
	// Send packet
	result = usi_SendCmd(&macG3Msg) ? 0 : -1;

	// Set sync flags to false
	g_mac_sync_mgmt.f_sync_req = false;
	g_mac_sync_mgmt.f_sync_res = false;
	LOG_IFACE_G3_MAC("%s\r\n", (result == 0) ? "OK" : "ERROR");
}

/**********************************************************************************************************************/
/** This function is called periodically in embedded apps. Not needed on Host side.
 **********************************************************************************************************************/
void MacWrapperEventHandler(void)
{
	LOG_IFACE_G3_MAC("MacWrapperEventHandler...OK\r\n");
}

/**********************************************************************************************************************/
/** The MacDataRequest primitive requests the transfer of an application PDU to another device or multiple devices.
 ***********************************************************************************************************************
 * @param MacDataRequest Mac Data Request Structure
 **********************************************************************************************************************/
void MacWrapperMcpsDataRequest(struct TMacWrpDataRequest *MacDataRequest)
{
	uint8_t *ptrBuff;
	uint16_t length;
	uint32_t i, result;
	uint16_t us_addr_mode_len;
	
	LOG_IFACE_G3_MAC("MacWrapperMcpsDataRequest...");

	ptrBuff = buffTxMacG3;

	*ptrBuff++ = G3_SERIAL_MSG_MAC_DATA_REQUEST;
	*ptrBuff++ = MacDataRequest->m_u8MsduHandle;
	*ptrBuff++ = MacDataRequest->m_eSecurityLevel;
	*ptrBuff++ = MacDataRequest->m_u8KeyIndex;
	*ptrBuff++ = MacDataRequest->m_eQualityOfService;
	*ptrBuff++ = MacDataRequest->m_u8TxOptions;
	*ptrBuff++ = (MacDataRequest->m_nDstPanId >> 8) & 0xFF;
	*ptrBuff++ = MacDataRequest->m_nDstPanId & 0xFF;
	*ptrBuff++ = (MacDataRequest->m_eSrcAddrMode == MAC_ADDRESS_MODE_SHORT) ? 2 : 8;
	us_addr_mode_len = (MacDataRequest->m_DstAddr.m_eAddrMode == MAC_ADDRESS_MODE_SHORT) ? 2 : 8;
	*ptrBuff++ = us_addr_mode_len;
	if (MacDataRequest->m_DstAddr.m_eAddrMode == MAC_ADDRESS_MODE_SHORT) {
		*ptrBuff++ = ((MacDataRequest->m_DstAddr.m_nShortAddress >> 8) & 0xFF);
		*ptrBuff++ = (MacDataRequest->m_DstAddr.m_nShortAddress & 0xFF);
	}
	else {
		memcpy(ptrBuff,&MacDataRequest->m_DstAddr.m_ExtendedAddress,us_addr_mode_len);
		ptrBuff += us_addr_mode_len;
	}
	*ptrBuff++ = (MacDataRequest->m_u16MsduLength >> 8) & 0xFF;
	*ptrBuff++ = (MacDataRequest->m_u16MsduLength & 0xFF);
	for (i = 0; i < MacDataRequest->m_u16MsduLength; i++) {
		*ptrBuff++ = MacDataRequest->m_pMsdu[i];
	}

	length = ptrBuff - buffTxMacG3;
	macG3Msg.pType = PROTOCOL_MAC_G3;
	macG3Msg.buf = buffTxMacG3;
	macG3Msg.len = length;
	// Send packet
	result = usi_SendCmd(&macG3Msg) ? 0 : -1;

	LOG_IFACE_G3_MAC("%s\r\n", (result == 0) ? "OK" : "ERROR");
}

/**********************************************************************************************************************/
/** The MacWrapperMlmeGetRequest primitive allows the upper layer to get the value of an attribute from the MAC information base.
 ***********************************************************************************************************************
 * @param MacGetRequest Parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeGetRequest(struct TMacWrpGetRequest *MacGetRequest)
{
	uint8_t *ptrBuff;
	uint16_t length;
	uint32_t result;

	ptrBuff = buffTxMacG3;

	*ptrBuff++ = G3_SERIAL_MSG_MAC_GET_REQUEST;
	*ptrBuff++ = ((MacGetRequest->m_ePibAttribute >> 24) & 0xFF);
	*ptrBuff++ = ((MacGetRequest->m_ePibAttribute >> 16) & 0xFF);
	*ptrBuff++ = ((MacGetRequest->m_ePibAttribute >> 8) & 0xFF);
	*ptrBuff++ = (MacGetRequest->m_ePibAttribute & 0xFF);
	*ptrBuff++ = (MacGetRequest->m_u16PibAttributeIndex >> 8);
	*ptrBuff++ = (MacGetRequest->m_u16PibAttributeIndex & 0xFF);

	length = ptrBuff - buffTxMacG3;
	macG3Msg.pType = PROTOCOL_MAC_G3;
	macG3Msg.buf = buffTxMacG3;
	macG3Msg.len = length;
	// Send packet
	result = usi_SendCmd(&macG3Msg) ? 0 : -1;

	LOG_IFACE_G3_MAC("MacWrapperMlmeGetRequest ATTR = 0x%04x ...%s\r\n", MacGetRequest->m_ePibAttribute, (result == 0) ? "OK" : "ERROR");
}

/**********************************************************************************************************************/
/** The MacWrapperMlmeSetRequest primitive allows the upper layer to set the value of an attribute in the MAC information base.
 ***********************************************************************************************************************
 * @param MacSetRequest Parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeSetRequest(struct TMacWrpSetRequest *MacSetRequest)
{
	uint8_t *ptrBuff;
	uint16_t length;
	uint32_t i, result;
	
	ptrBuff = buffTxMacG3;

	*ptrBuff++ = G3_SERIAL_MSG_MAC_SET_REQUEST;
	*ptrBuff++ = ((MacSetRequest->m_ePibAttribute >> 24) & 0xFF);
	*ptrBuff++ = ((MacSetRequest->m_ePibAttribute >> 16) & 0xFF);
	*ptrBuff++ = ((MacSetRequest->m_ePibAttribute >> 8) & 0xFF);
	*ptrBuff++ = (MacSetRequest->m_ePibAttribute & 0xFF);
	*ptrBuff++ = (MacSetRequest->m_u16PibAttributeIndex >> 8);
	*ptrBuff++ = (MacSetRequest->m_u16PibAttributeIndex & 0xFF);
	*ptrBuff++ = MacSetRequest->m_PibAttributeValue.m_u8Length;
	for (i = 0; i < MacSetRequest->m_PibAttributeValue.m_u8Length; i++) {
		*ptrBuff++ = MacSetRequest->m_PibAttributeValue.m_au8Value[i];
	}

	length = ptrBuff - buffTxMacG3;
	macG3Msg.pType = PROTOCOL_MAC_G3;
	macG3Msg.buf = buffTxMacG3;
	macG3Msg.len = length;
	// Send packet
	result = usi_SendCmd(&macG3Msg) ? 0 : -1;

	LOG_IFACE_G3_MAC("MacWrapperMlmeSetRequest ATTR = 0x%04x ...%s\r\n", MacSetRequest->m_ePibAttribute, (result == 0) ? "OK" : "ERROR");
}

/**********************************************************************************************************************/
/** The MacWrapperMlmeResetRequest primitive performs a reset of the mac sublayer and allows the resetting of the MIB
 * attributes.
 ***********************************************************************************************************************
 * @param MacResetRequest Parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeResetRequest(struct TMacWrpResetRequest *MacResetRequest)
{
	uint8_t * ptrBuff;
	uint16_t length;
	uint32_t result;

	LOG_IFACE_G3_MAC("MacWrapperMlmeResetRequest...");
	
	ptrBuff = buffTxMacG3;

	*ptrBuff++ = G3_SERIAL_MSG_MAC_RESET_REQUEST;
	*ptrBuff++ = MacResetRequest->m_bSetDefaultPib;
	
	length = ptrBuff - buffTxMacG3;
	macG3Msg.pType = PROTOCOL_MAC_G3;
	macG3Msg.buf = buffTxMacG3;
	macG3Msg.len = length;
	// Send packet
	result = usi_SendCmd(&macG3Msg) ? 0 : -1;

	LOG_IFACE_G3_MAC("%s\r\n", (result == 0) ? "OK" : "ERROR");
}

/**********************************************************************************************************************/
/** The MacWrapperMlmeStartRequest primitive allows the upper layer to request the starting of a new network.
 ***********************************************************************************************************************
 * @param MacStartRequest The parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeStartRequest(struct TMacWrpStartRequest *MacStartRequest)
{
	uint8_t *ptrBuff;
	uint16_t length;
	uint32_t result;

	LOG_IFACE_G3_MAC("MacWrapperMlmeStartRequest...");
	
	ptrBuff = buffTxMacG3;

	*ptrBuff++ = G3_SERIAL_MSG_MAC_START_REQUEST;
	*ptrBuff++ = ((MacStartRequest->m_nPanId >> 8) & 0xFF);
	*ptrBuff++ = (MacStartRequest->m_nPanId & 0xFF);
	
	length = ptrBuff - buffTxMacG3;
	macG3Msg.pType = PROTOCOL_MAC_G3;
	macG3Msg.buf = buffTxMacG3;
	macG3Msg.len = length;
	// Send packet
	result = usi_SendCmd(&macG3Msg) ? 0 : -1;

	LOG_IFACE_G3_MAC("%s\r\n", (result == 0) ? "OK" : "ERROR");
}

/**********************************************************************************************************************/
/** The MacWrapperMlmeScanRequest primitive allows the upper layer to scan for networks operating in its POS.
 ***********************************************************************************************************************
 * @param MacScanRequest Parameters of the request.
 **********************************************************************************************************************/
void MacWrapperMlmeScanRequest(struct TMacWrpScanRequest *MacScanRequest)
{
	uint8_t *ptrBuff;
	uint16_t length;
	uint32_t result;

	LOG_IFACE_G3_MAC("MacWrapperMlmeScanRequest...");
	
	ptrBuff = buffTxMacG3;

	*ptrBuff++ = G3_SERIAL_MSG_MAC_SCAN_REQUEST;
	*ptrBuff++ = ((MacScanRequest->m_u16ScanDuration >> 8) & 0xFF);
	*ptrBuff++ = (MacScanRequest->m_u16ScanDuration & 0xFF);
	
	length = ptrBuff - buffTxMacG3;
	macG3Msg.pType = PROTOCOL_MAC_G3;
	macG3Msg.buf = buffTxMacG3;
	macG3Msg.len = length;
	// Send packet
	result = usi_SendCmd(&macG3Msg) ? 0 : -1;

	LOG_IFACE_G3_MAC("%s\r\n", (result == 0) ? "OK" : "ERROR");
}

/**********************************************************************************************************************/
/** Callbacks from Serial
 **********************************************************************************************************************/

/**
 * @brief _macStatus_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macStatus_cb(uint8_t* ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_MAC("_macStatus_cb\r\n");
	/* Check the message length */
	if (len < MIN_MAC_STATUS_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR: Size %u < 1\r\n", len);
		return false;
	}

	enum ESerialStatus m_u8Status;
	m_u8Status = (*ptrMsg++);
	if (len > 1) {
		uint8_t m_u8CommandId;
		m_u8CommandId = (*ptrMsg++);
		LOG_IFACE_G3_MAC("CommandId: 0x%X; MacStatus: 0x%X\r\n", m_u8CommandId, m_u8Status);
	}
	else {
		LOG_IFACE_G3_MAC("CommandId: UNKNOWN; MacStatus: 0x%X\r\n", m_u8Status);
	}
	return true;
}

/**
 * @brief _macDataIndication_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macDataIndication_cb(uint8_t* ptrMsg, uint16_t len)
{
	uint16_t us_addr_mode_len;
	
	LOG_IFACE_G3_MAC("_macDataIndication_cb...");

	// Check the message length
	if (len < MIN_MAC_DATA_INDICATION_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_macNotifications.m_MacWrpDataIndication) {
		struct TMacWrpDataIndication macDataIndication;

		macDataIndication.m_nSrcPanId = (uint16_t)(*ptrMsg++);
		macDataIndication.m_nSrcPanId = (uint16_t)(*ptrMsg++) + (macDataIndication.m_nSrcPanId << 8);
		
		us_addr_mode_len = (*ptrMsg++);
		if (us_addr_mode_len == 2) {
			// MAC_ADDRESS_MODE_SHORT
			macDataIndication.m_SrcAddr.m_eAddrMode = MAC_ADDRESS_MODE_SHORT;
			macDataIndication.m_SrcAddr.m_nShortAddress = (uint16_t)(*ptrMsg++);
			macDataIndication.m_SrcAddr.m_nShortAddress = (uint16_t)(*ptrMsg++) + (macDataIndication.m_SrcAddr.m_nShortAddress << 8);
		}
		else {
			// MAC_ADDRESS_MODE_EXTENDED
			macDataIndication.m_SrcAddr.m_eAddrMode = MAC_ADDRESS_MODE_EXTENDED;
			memcpy(&macDataIndication.m_SrcAddr.m_ExtendedAddress, ptrMsg, us_addr_mode_len);
			ptrMsg += us_addr_mode_len;
		}
		
		macDataIndication.m_nDstPanId = (uint16_t)(*ptrMsg++);
		macDataIndication.m_nDstPanId = (uint16_t)(*ptrMsg++) + (macDataIndication.m_nDstPanId << 8);
		
		us_addr_mode_len = (*ptrMsg++);
		if (us_addr_mode_len == 2) {
			// MAC_ADDRESS_MODE_SHORT
			macDataIndication.m_DstAddr.m_eAddrMode = MAC_ADDRESS_MODE_SHORT;
			macDataIndication.m_DstAddr.m_nShortAddress = (uint16_t)(*ptrMsg++);
			macDataIndication.m_DstAddr.m_nShortAddress = (uint16_t)(*ptrMsg++) + (macDataIndication.m_DstAddr.m_nShortAddress << 8);
		}
		else {
			// MAC_ADDRESS_MODE_EXTENDED
			macDataIndication.m_DstAddr.m_eAddrMode = MAC_ADDRESS_MODE_EXTENDED;
			memcpy(&macDataIndication.m_DstAddr.m_ExtendedAddress,ptrMsg,us_addr_mode_len);
			ptrMsg += us_addr_mode_len;
		}
		
		macDataIndication.m_u8MpduLinkQuality = (*ptrMsg++);
		macDataIndication.m_u8Dsn = (*ptrMsg++);
		macDataIndication.m_nTimestamp = (uint32_t)(*ptrMsg++);
		macDataIndication.m_nTimestamp = (uint32_t)(*ptrMsg++) + (macDataIndication.m_nTimestamp << 8);
		macDataIndication.m_nTimestamp = (uint32_t)(*ptrMsg++) + (macDataIndication.m_nTimestamp << 8);
		macDataIndication.m_nTimestamp = (uint32_t)(*ptrMsg++) + (macDataIndication.m_nTimestamp << 8);
		
		macDataIndication.m_eSecurityLevel = (*ptrMsg++);
		macDataIndication.m_u8KeyIndex = (*ptrMsg++);
		macDataIndication.m_eQualityOfService = (*ptrMsg++);
		
		macDataIndication.m_u8RecvModulation = (*ptrMsg++);
		macDataIndication.m_u8RecvModulationScheme = (*ptrMsg++);
		memcpy(&macDataIndication.m_RecvToneMap.m_au8Tm[0], ptrMsg, 3);
		ptrMsg += 3;
		
		macDataIndication.m_u8ComputedModulation = (*ptrMsg++);
		macDataIndication.m_u8ComputedModulationScheme = (*ptrMsg++);
		memcpy(&macDataIndication.m_ComputedToneMap.m_au8Tm[0], ptrMsg, 3);
		ptrMsg += 3;	
		
		macDataIndication.m_u16MsduLength = (*ptrMsg++);
		macDataIndication.m_u16MsduLength = (*ptrMsg++) + (macDataIndication.m_u16MsduLength << 8);
		
		// Copy MSDU on Buffer
		macDataIndication.m_pMsdu = (uint8_t*)malloc(macDataIndication.m_u16MsduLength*sizeof(uint8_t));
		memcpy((uint8_t *)macDataIndication.m_pMsdu, ptrMsg, macDataIndication.m_u16MsduLength);

		// Trigger the callback
		g_macNotifications.m_MacWrpDataIndication(&macDataIndication);

		free((uint8_t *)(macDataIndication.m_pMsdu));
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}
	return true;
}

/**
 * @brief _macDataConfirm_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macDataConfirm_cb(uint8_t* ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_MAC("_macDataConfirm_cb...");

	// Check the message length
	if (len < MIN_MAC_DATA_CONFIRM_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_macNotifications.m_MacWrpDataConfirm) {
		struct TMacWrpDataConfirm macDataConfirm;

		macDataConfirm.m_u8MsduHandle = (*ptrMsg++);
		macDataConfirm.m_eStatus = (*ptrMsg++);
		macDataConfirm.m_nTimestamp = (uint32_t)(*ptrMsg++);
		macDataConfirm.m_nTimestamp = (uint32_t)(*ptrMsg++) + (macDataConfirm.m_nTimestamp << 8);
		macDataConfirm.m_nTimestamp = (uint32_t)(*ptrMsg++) + (macDataConfirm.m_nTimestamp << 8);
		macDataConfirm.m_nTimestamp = (uint32_t)(*ptrMsg++) + (macDataConfirm.m_nTimestamp << 8);
		
		g_macNotifications.m_MacWrpDataConfirm(&macDataConfirm);
		
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}
	return true;
}

/**
 * @brief _macResetConfirm_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macResetConfirm_cb(uint8_t* ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_MAC("_macResetConfirm_cb...");

	// Check the message length
	if (len < MIN_MAC_RESET_CONFIRM_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_macNotifications.m_MacWrpResetConfirm) {
		struct TMacWrpResetConfirm macResetConfirm;

		macResetConfirm.m_eStatus = (*ptrMsg++);

		g_macNotifications.m_MacWrpResetConfirm(&macResetConfirm);

		LOG_IFACE_G3_MAC("OK\r\n");
	}else{
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}
	return true;
}

/**
 * @brief _macSetConfirm_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macSetConfirm_cb(uint8_t* ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_MAC("_macSetConfirm_cb...");

	// Check the message length
	if (len < MIN_MAC_SET_CONFIRM_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_mac_sync_mgmt.f_sync_req || g_macNotifications.m_MacWrpSetConfirm) {
		struct TMacWrpSetConfirm macSetConfirm;

		macSetConfirm.m_eStatus = (*ptrMsg++);

		macSetConfirm.m_ePibAttribute = (*ptrMsg++);
		macSetConfirm.m_ePibAttribute = (*ptrMsg++) + (macSetConfirm.m_ePibAttribute << 8);
		macSetConfirm.m_ePibAttribute = (*ptrMsg++) + (macSetConfirm.m_ePibAttribute << 8);
		macSetConfirm.m_ePibAttribute = (*ptrMsg++) + (macSetConfirm.m_ePibAttribute << 8);

		macSetConfirm.m_u16PibAttributeIndex = (*ptrMsg++);
		macSetConfirm.m_u16PibAttributeIndex = (*ptrMsg++) + (macSetConfirm.m_u16PibAttributeIndex << 8);

		if (g_mac_sync_mgmt.f_sync_req) {
			// Synchronous call -> Store the result
			memcpy(&g_mac_sync_mgmt.s_SetConfirm, &macSetConfirm, sizeof(struct TMacWrpSetConfirm));
			g_mac_sync_mgmt.f_sync_res = true;
		}
		else if (g_macNotifications.m_MacWrpSetConfirm) {
			// Asynchronous call -> Callback
			g_macNotifications.m_MacWrpSetConfirm(&macSetConfirm);
		}
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}

	return true;
}

/**
 * @brief _macGetConfirm_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macGetConfirm_cb(uint8_t* ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_MAC("_macGetConfirm_cb...");

	// Check the message length
	if (len < MIN_MAC_GET_CONFIRM_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_mac_sync_mgmt.f_sync_req || g_macNotifications.m_MacWrpGetConfirm) {
		struct TMacWrpGetConfirm macGetConfirm;

		macGetConfirm.m_eStatus = (*ptrMsg++);

		macGetConfirm.m_ePibAttribute = (*ptrMsg++);
		macGetConfirm.m_ePibAttribute = (*ptrMsg++) + (macGetConfirm.m_ePibAttribute << 8);
		macGetConfirm.m_ePibAttribute = (*ptrMsg++) + (macGetConfirm.m_ePibAttribute << 8);
		macGetConfirm.m_ePibAttribute = (*ptrMsg++) + (macGetConfirm.m_ePibAttribute << 8);

		macGetConfirm.m_u16PibAttributeIndex = (*ptrMsg++);
		macGetConfirm.m_u16PibAttributeIndex = (*ptrMsg++) + (macGetConfirm.m_u16PibAttributeIndex << 8);

		macGetConfirm.m_PibAttributeValue.m_u8Length = (*ptrMsg++);
		if (macGetConfirm.m_PibAttributeValue.m_u8Length > MAC_WRP_PIB_MAX_VALUE_LENGTH) {
			//ToDo: Log error
			return false;
		}

		memcpy(&macGetConfirm.m_PibAttributeValue.m_au8Value, ptrMsg, macGetConfirm.m_PibAttributeValue.m_u8Length);

		if (g_mac_sync_mgmt.f_sync_req) {
			// Synchronous call -> Store the result
			memcpy(&g_mac_sync_mgmt.s_GetConfirm, &macGetConfirm, sizeof(struct TMacWrpGetConfirm));
			g_mac_sync_mgmt.f_sync_res = true;
		}
		else if (g_macNotifications.m_MacWrpGetConfirm)
		{
			// Asynchronous call -> Callback
			g_macNotifications.m_MacWrpGetConfirm(&macGetConfirm);
		}
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}
	
	return true;
}

/**
 * @brief _macScanConfirm_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macScanConfirm_cb(uint8_t* ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_MAC("_macScanConfirm_cb...");

	// Check the message length
	if (len < MIN_MAC_SCAN_CONFIRM_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_macNotifications.m_MacWrpScanConfirm) {
		struct TMacWrpScanConfirm macScanConfirm;

		macScanConfirm.m_eStatus = (*ptrMsg++);
		
		g_macNotifications.m_MacWrpScanConfirm(&macScanConfirm);
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}
	
	return true;
}

/**
 * @brief _macStartConfirm_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macStartConfirm_cb(uint8_t* ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_MAC("_macStartConfirm_cb...");

	// Check the message length
	if (len < MIN_MAC_START_CONFIRM_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_macNotifications.m_MacWrpStartConfirm) {
		struct TMacWrpStartConfirm macStartConfirm;

		macStartConfirm.m_eStatus = (*ptrMsg++);
		
		g_macNotifications.m_MacWrpStartConfirm(&macStartConfirm);
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}
	
	return true;
}

/**
 * @brief _macBeaconNotify_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macBeaconNotify_cb(uint8_t* ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_MAC("_macBeaconNotify_cb...");

	// Check the message length
	if (len < MIN_MAC_BEACON_NOTIFY_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_macNotifications.m_MacWrpBeaconNotifyIndication) {
		struct TMacWrpBeaconNotifyIndication macBeaconNotifyIndication;

		macBeaconNotifyIndication.m_PanDescriptor.m_nPanId = *ptrMsg++ ;
		macBeaconNotifyIndication.m_PanDescriptor.m_nPanId = (*ptrMsg++) + (macBeaconNotifyIndication.m_PanDescriptor.m_nPanId << 8);
		
		macBeaconNotifyIndication.m_PanDescriptor.m_u8LinkQuality = *ptrMsg++ ;
		
		macBeaconNotifyIndication.m_PanDescriptor.m_nLbaAddress = *ptrMsg++ ;
		macBeaconNotifyIndication.m_PanDescriptor.m_nLbaAddress = (*ptrMsg++) + (macBeaconNotifyIndication.m_PanDescriptor.m_nLbaAddress << 8);
		
		macBeaconNotifyIndication.m_PanDescriptor.m_u16RcCoord = *ptrMsg++ ;
		macBeaconNotifyIndication.m_PanDescriptor.m_u16RcCoord = (*ptrMsg++) + (macBeaconNotifyIndication.m_PanDescriptor.m_u16RcCoord << 8);

		g_macNotifications.m_MacWrpBeaconNotifyIndication(&macBeaconNotifyIndication);
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}

	return true;
}

/**
 * @brief _macCommStatusIndication_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macCommStatusIndication_cb(uint8_t* ptrMsg, uint16_t len)
{
	uint16_t us_addr_mode_len;
	
	LOG_IFACE_G3_MAC("_macCommStatusIndication_cb...");

	// Check the message length
	if (len < MIN_MAC_COMM_STATUS_INDICATION_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_macNotifications.m_MacWrpCommStatusIndication) {
		struct TMacWrpCommStatusIndication macCommStatusIndication;

		macCommStatusIndication.m_nPanId = *ptrMsg++;
		macCommStatusIndication.m_nPanId = (*ptrMsg++) + (macCommStatusIndication.m_nPanId << 8);
		
		// Source Address
		us_addr_mode_len = (*ptrMsg++);
		if (us_addr_mode_len == 2){
			// MAC_ADDRESS_MODE_SHORT
			macCommStatusIndication.m_SrcAddr.m_eAddrMode = MAC_ADDRESS_MODE_SHORT;
			macCommStatusIndication.m_SrcAddr.m_nShortAddress = (*ptrMsg++);
			macCommStatusIndication.m_SrcAddr.m_nShortAddress = (*ptrMsg++) + (macCommStatusIndication.m_SrcAddr.m_nShortAddress << 8);
		}else{
			// MAC_ADDRESS_MODE_EXTENDED
			macCommStatusIndication.m_SrcAddr.m_eAddrMode = MAC_ADDRESS_MODE_EXTENDED;
			memcpy(&macCommStatusIndication.m_SrcAddr.m_ExtendedAddress,ptrMsg,us_addr_mode_len);
			ptrMsg += us_addr_mode_len;
		}
		
		// Destination Address
		us_addr_mode_len = (*ptrMsg++);
		if (us_addr_mode_len == 2){
			// MAC_ADDRESS_MODE_SHORT
			macCommStatusIndication.m_DstAddr.m_eAddrMode = MAC_ADDRESS_MODE_SHORT;
			macCommStatusIndication.m_DstAddr.m_nShortAddress = (*ptrMsg++);
			macCommStatusIndication.m_DstAddr.m_nShortAddress = (*ptrMsg++) + (macCommStatusIndication.m_DstAddr.m_nShortAddress << 8);
		}else{
			// MAC_ADDRESS_MODE_EXTENDED
			macCommStatusIndication.m_DstAddr.m_eAddrMode = MAC_ADDRESS_MODE_EXTENDED;
			memcpy(&macCommStatusIndication.m_DstAddr.m_ExtendedAddress,ptrMsg,us_addr_mode_len);
			ptrMsg += us_addr_mode_len;
		}
		
		macCommStatusIndication.m_eStatus = (*ptrMsg++);
		macCommStatusIndication.m_eSecurityLevel = (*ptrMsg++);
		macCommStatusIndication.m_u8KeyIndex = (*ptrMsg++);

		g_macNotifications.m_MacWrpCommStatusIndication(&macCommStatusIndication);
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}

	return true;
}


#ifdef G3_HYBRID_PROFILE

/**********************************************************************************************************************/
/** Forward declaration
 **********************************************************************************************************************/

/**********************************************************************************************************************/
/** Use this function to initialize the RF MAC layer. The MAC layer should be initialized before doing any other operation.
 * The APIs cannot be mixed, if the stack is initialized in MAC mode then only the MAC functions can be used and if the
 * stack is initialized in ADP mode then only ADP functions can be used.
 * @param pNotifications Structure with callbacks used by the RF MAC layer to notify the upper layer about specific events
 **********************************************************************************************************************/
void MacWrapperInitializeRF(struct TMacWrpNotifications *pNotifications)
{
	uint8_t *ptrBuff;
	uint16_t length;
	uint32_t result;

	LOG_IFACE_G3_MAC("MacWrapperInitializeRF...");
	
	// Copy callbacks
	memcpy(&g_macNotificationsRF, pNotifications, sizeof(struct TMacWrpNotifications));

	ptrBuff = buffTxMacG3;

	*ptrBuff++ = G3_SERIAL_MSG_MAC_INITIALIZE_RF;

	length = ptrBuff - buffTxMacG3;
	macG3Msg.pType = PROTOCOL_MAC_G3;
	macG3Msg.buf = buffTxMacG3;
	macG3Msg.len = length;
	// Send packet
	result = usi_SendCmd(&macG3Msg) ? 0 : -1;

	// Set sync flags to false
	g_mac_sync_mgmt_rf.f_sync_req = false;
	g_mac_sync_mgmt_rf.f_sync_res = false;
	LOG_IFACE_G3_MAC("%s\r\n", (result == 0) ? "OK" : "ERROR");
}

/**********************************************************************************************************************/
/** This function is called periodically in embedded apps. Not needed on Host side.
 **********************************************************************************************************************/
void MacWrapperEventHandlerRF(void)
{
	LOG_IFACE_G3_MAC("MacWrapperEventHandlerRF...OK\r\n");
}

/**********************************************************************************************************************/
/** The MacDataRequest primitive requests the transfer of an application PDU to another device or multiple devices.
 ***********************************************************************************************************************
 * @param MacDataRequest Mac Data Request Structure
 **********************************************************************************************************************/
void MacWrapperMcpsDataRequestRF(struct TMacWrpDataRequest *MacDataRequest)
{
	uint8_t *ptrBuff;
	uint16_t length;
	uint32_t i, result;
	uint16_t us_addr_mode_len;
	
	LOG_IFACE_G3_MAC("MacWrapperMcpsDataRequestRF...");

	ptrBuff = buffTxMacG3;

	*ptrBuff++ = G3_SERIAL_MSG_MAC_DATA_REQUEST_RF;
	*ptrBuff++ = MacDataRequest->m_u8MsduHandle;
	*ptrBuff++ = MacDataRequest->m_eSecurityLevel;
	*ptrBuff++ = MacDataRequest->m_u8KeyIndex;
	*ptrBuff++ = MacDataRequest->m_eQualityOfService;
	*ptrBuff++ = MacDataRequest->m_u8TxOptions;
	*ptrBuff++ = (MacDataRequest->m_nDstPanId >> 8) & 0xFF;
	*ptrBuff++ = MacDataRequest->m_nDstPanId & 0xFF;
	*ptrBuff++ = (MacDataRequest->m_eSrcAddrMode == MAC_ADDRESS_MODE_SHORT) ? 2 : 8;
	us_addr_mode_len = (MacDataRequest->m_DstAddr.m_eAddrMode == MAC_ADDRESS_MODE_SHORT) ? 2 : 8;
	*ptrBuff++ = us_addr_mode_len;
	if (MacDataRequest->m_DstAddr.m_eAddrMode == MAC_ADDRESS_MODE_SHORT) {
		*ptrBuff++ = ((MacDataRequest->m_DstAddr.m_nShortAddress >> 8) & 0xFF);
		*ptrBuff++ = (MacDataRequest->m_DstAddr.m_nShortAddress & 0xFF);
	}
	else {
		memcpy(ptrBuff,&MacDataRequest->m_DstAddr.m_ExtendedAddress,us_addr_mode_len);
		ptrBuff += us_addr_mode_len;
	}
	*ptrBuff++ = (MacDataRequest->m_u16MsduLength >> 8) & 0xFF;
	*ptrBuff++ = (MacDataRequest->m_u16MsduLength & 0xFF);
	for (i = 0; i < MacDataRequest->m_u16MsduLength; i++) {
		*ptrBuff++ = MacDataRequest->m_pMsdu[i];
	}

	length = ptrBuff - buffTxMacG3;
	macG3Msg.pType = PROTOCOL_MAC_G3;
	macG3Msg.buf = buffTxMacG3;
	macG3Msg.len = length;
	// Send packet
	result = usi_SendCmd(&macG3Msg) ? 0 : -1;

	LOG_IFACE_G3_MAC("%s\r\n", (result == 0) ? "OK" : "ERROR");
}

/**********************************************************************************************************************/
/** The MacWrapperMlmeGetRequestRF primitive allows the upper layer to get the value of an attribute from the MAC information base.
 ***********************************************************************************************************************
 * @param MacGetRequest Parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeGetRequestRF(struct TMacWrpGetRequest *MacGetRequest)
{
	uint8_t *ptrBuff;
	uint16_t length;
	uint32_t result;

	ptrBuff = buffTxMacG3;

	*ptrBuff++ = G3_SERIAL_MSG_MAC_GET_REQUEST_RF;
	*ptrBuff++ = ((MacGetRequest->m_ePibAttribute >> 24) & 0xFF);
	*ptrBuff++ = ((MacGetRequest->m_ePibAttribute >> 16) & 0xFF);
	*ptrBuff++ = ((MacGetRequest->m_ePibAttribute >> 8) & 0xFF);
	*ptrBuff++ = (MacGetRequest->m_ePibAttribute & 0xFF);
	*ptrBuff++ = (MacGetRequest->m_u16PibAttributeIndex >> 8);
	*ptrBuff++ = (MacGetRequest->m_u16PibAttributeIndex & 0xFF);

	length = ptrBuff - buffTxMacG3;
	macG3Msg.pType = PROTOCOL_MAC_G3;
	macG3Msg.buf = buffTxMacG3;
	macG3Msg.len = length;
	// Send packet
	result = usi_SendCmd(&macG3Msg) ? 0 : -1;

	LOG_IFACE_G3_MAC("MacWrapperMlmeGetRequestRF ATTR = 0x%04x ...%s\r\n", MacGetRequest->m_ePibAttribute, (result == 0) ? "OK" : "ERROR");
}

/**********************************************************************************************************************/
/** The MacWrapperMlmeSetRequestRF primitive allows the upper layer to set the value of an attribute in the MAC information base.
 ***********************************************************************************************************************
 * @param MacSetRequest Parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeSetRequestRF(struct TMacWrpSetRequest *MacSetRequest)
{
	uint8_t *ptrBuff;
	uint16_t length;
	uint32_t i, result;
	
	ptrBuff = buffTxMacG3;

	*ptrBuff++ = G3_SERIAL_MSG_MAC_SET_REQUEST_RF;
	*ptrBuff++ = ((MacSetRequest->m_ePibAttribute >> 24) & 0xFF);
	*ptrBuff++ = ((MacSetRequest->m_ePibAttribute >> 16) & 0xFF);
	*ptrBuff++ = ((MacSetRequest->m_ePibAttribute >> 8) & 0xFF);
	*ptrBuff++ = (MacSetRequest->m_ePibAttribute & 0xFF);
	*ptrBuff++ = (MacSetRequest->m_u16PibAttributeIndex >> 8);
	*ptrBuff++ = (MacSetRequest->m_u16PibAttributeIndex & 0xFF);
	*ptrBuff++ = MacSetRequest->m_PibAttributeValue.m_u8Length;
	for (i = 0; i < MacSetRequest->m_PibAttributeValue.m_u8Length; i++) {
		*ptrBuff++ = MacSetRequest->m_PibAttributeValue.m_au8Value[i];
	}

	length = ptrBuff - buffTxMacG3;
	macG3Msg.pType = PROTOCOL_MAC_G3;
	macG3Msg.buf = buffTxMacG3;
	macG3Msg.len = length;
	// Send packet
	result = usi_SendCmd(&macG3Msg) ? 0 : -1;

	LOG_IFACE_G3_MAC("MacWrapperMlmeSetRequestRF ATTR = 0x%04x ...%s\r\n", MacSetRequest->m_ePibAttribute, (result == 0) ? "OK" : "ERROR");
}

/**********************************************************************************************************************/
/** The MacWrapperMlmeResetRequestRF primitive performs a reset of the mac sublayer and allows the resetting of the MIB
 * attributes.
 ***********************************************************************************************************************
 * @param MacResetRequest Parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeResetRequestRF(struct TMacWrpResetRequest *MacResetRequest)
{
	uint8_t * ptrBuff;
	uint16_t length;
	uint32_t result;

	LOG_IFACE_G3_MAC("MacWrapperMlmeResetRequestRF...");
	
	ptrBuff = buffTxMacG3;

	*ptrBuff++ = G3_SERIAL_MSG_MAC_RESET_REQUEST_RF;
	*ptrBuff++ = MacResetRequest->m_bSetDefaultPib;
	
	length = ptrBuff - buffTxMacG3;
	macG3Msg.pType = PROTOCOL_MAC_G3;
	macG3Msg.buf = buffTxMacG3;
	macG3Msg.len = length;
	// Send packet
	result = usi_SendCmd(&macG3Msg) ? 0 : -1;

	LOG_IFACE_G3_MAC("%s\r\n", (result == 0) ? "OK" : "ERROR");
}

/**********************************************************************************************************************/
/** The MacWrapperMlmeStartRequestRF primitive allows the upper layer to request the starting of a new network.
 ***********************************************************************************************************************
 * @param MacStartRequest The parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeStartRequestRF(struct TMacWrpStartRequest *MacStartRequest)
{
	uint8_t *ptrBuff;
	uint16_t length;
	uint32_t result;

	LOG_IFACE_G3_MAC("MacWrapperMlmeStartRequestRF...");
	
	ptrBuff = buffTxMacG3;

	*ptrBuff++ = G3_SERIAL_MSG_MAC_START_REQUEST_RF;
	*ptrBuff++ = ((MacStartRequest->m_nPanId >> 8) & 0xFF);
	*ptrBuff++ = (MacStartRequest->m_nPanId & 0xFF);
	
	length = ptrBuff - buffTxMacG3;
	macG3Msg.pType = PROTOCOL_MAC_G3;
	macG3Msg.buf = buffTxMacG3;
	macG3Msg.len = length;
	// Send packet
	result = usi_SendCmd(&macG3Msg) ? 0 : -1;

	LOG_IFACE_G3_MAC("%s\r\n", (result == 0) ? "OK" : "ERROR");
}

/**********************************************************************************************************************/
/** The MacWrapperMlmeScanRequestRF primitive allows the upper layer to scan for networks operating in its POS.
 ***********************************************************************************************************************
 * @param MacScanRequest Parameters of the request.
 **********************************************************************************************************************/
void MacWrapperMlmeScanRequestRF(struct TMacWrpScanRequest *MacScanRequest)
{
	uint8_t *ptrBuff;
	uint16_t length;
	uint32_t result;

	LOG_IFACE_G3_MAC("MacWrapperMlmeScanRequestRF...");
	
	ptrBuff = buffTxMacG3;

	*ptrBuff++ = G3_SERIAL_MSG_MAC_SCAN_REQUEST_RF;
	*ptrBuff++ = ((MacScanRequest->m_u16ScanDuration >> 8) & 0xFF);
	*ptrBuff++ = (MacScanRequest->m_u16ScanDuration & 0xFF);
	
	length = ptrBuff - buffTxMacG3;
	macG3Msg.pType = PROTOCOL_MAC_G3;
	macG3Msg.buf = buffTxMacG3;
	macG3Msg.len = length;
	// Send packet
	result = usi_SendCmd(&macG3Msg) ? 0 : -1;

	LOG_IFACE_G3_MAC("%s\r\n", (result == 0) ? "OK" : "ERROR");
}

/**********************************************************************************************************************/
/** Callbacks from Serial
 **********************************************************************************************************************/

/**
 * @brief _macDataIndicationRF_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macDataIndicationRF_cb(uint8_t* ptrMsg, uint16_t len)
{
	uint16_t us_addr_mode_len;
	
	LOG_IFACE_G3_MAC("_macDataIndicationRF_cb...");

	// Check the message length
	if (len < MIN_MAC_DATA_INDICATION_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_macNotificationsRF.m_MacWrpDataIndication) {
		struct TMacWrpDataIndication macDataIndication;

		macDataIndication.m_nSrcPanId = (uint16_t)(*ptrMsg++);
		macDataIndication.m_nSrcPanId = (uint16_t)(*ptrMsg++) + (macDataIndication.m_nSrcPanId << 8);
		
		us_addr_mode_len = (*ptrMsg++);
		if (us_addr_mode_len == 2) {
			// MAC_ADDRESS_MODE_SHORT
			macDataIndication.m_SrcAddr.m_eAddrMode = MAC_ADDRESS_MODE_SHORT;
			macDataIndication.m_SrcAddr.m_nShortAddress = (uint16_t)(*ptrMsg++);
			macDataIndication.m_SrcAddr.m_nShortAddress = (uint16_t)(*ptrMsg++) + (macDataIndication.m_SrcAddr.m_nShortAddress << 8);
		}
		else {
			// MAC_ADDRESS_MODE_EXTENDED
			macDataIndication.m_SrcAddr.m_eAddrMode = MAC_ADDRESS_MODE_EXTENDED;
			memcpy(&macDataIndication.m_SrcAddr.m_ExtendedAddress, ptrMsg, us_addr_mode_len);
			ptrMsg += us_addr_mode_len;
		}
		
		macDataIndication.m_nDstPanId = (uint16_t)(*ptrMsg++);
		macDataIndication.m_nDstPanId = (uint16_t)(*ptrMsg++) + (macDataIndication.m_nDstPanId << 8);
		
		us_addr_mode_len = (*ptrMsg++);
		if (us_addr_mode_len == 2) {
			// MAC_ADDRESS_MODE_SHORT
			macDataIndication.m_DstAddr.m_eAddrMode = MAC_ADDRESS_MODE_SHORT;
			macDataIndication.m_DstAddr.m_nShortAddress = (uint16_t)(*ptrMsg++);
			macDataIndication.m_DstAddr.m_nShortAddress = (uint16_t)(*ptrMsg++) + (macDataIndication.m_DstAddr.m_nShortAddress << 8);
		}
		else {
			// MAC_ADDRESS_MODE_EXTENDED
			macDataIndication.m_DstAddr.m_eAddrMode = MAC_ADDRESS_MODE_EXTENDED;
			memcpy(&macDataIndication.m_DstAddr.m_ExtendedAddress,ptrMsg,us_addr_mode_len);
			ptrMsg += us_addr_mode_len;
		}
		
		macDataIndication.m_u8MpduLinkQuality = (*ptrMsg++);
		macDataIndication.m_u8Dsn = (*ptrMsg++);
		macDataIndication.m_nTimestamp = (uint32_t)(*ptrMsg++);
		macDataIndication.m_nTimestamp = (uint32_t)(*ptrMsg++) + (macDataIndication.m_nTimestamp << 8);
		macDataIndication.m_nTimestamp = (uint32_t)(*ptrMsg++) + (macDataIndication.m_nTimestamp << 8);
		macDataIndication.m_nTimestamp = (uint32_t)(*ptrMsg++) + (macDataIndication.m_nTimestamp << 8);
		
		macDataIndication.m_eSecurityLevel = (*ptrMsg++);
		macDataIndication.m_u8KeyIndex = (*ptrMsg++);
		macDataIndication.m_eQualityOfService = (*ptrMsg++);
		
		/* These values have no sense in RF but are returned with fixed values from RF MAC */
		macDataIndication.m_u8RecvModulation = (*ptrMsg++);
		macDataIndication.m_u8RecvModulationScheme = (*ptrMsg++);
		memcpy(&macDataIndication.m_RecvToneMap.m_au8Tm[0], ptrMsg, 3);
		ptrMsg += 3;
		macDataIndication.m_u8ComputedModulation = (*ptrMsg++);
		macDataIndication.m_u8ComputedModulationScheme = (*ptrMsg++);
		memcpy(&macDataIndication.m_ComputedToneMap.m_au8Tm[0], ptrMsg, 3);
		ptrMsg += 3;	
		
		macDataIndication.m_u16MsduLength = (*ptrMsg++);
		macDataIndication.m_u16MsduLength = (*ptrMsg++) + (macDataIndication.m_u16MsduLength << 8);
		
		// Copy MSDU on Buffer
		macDataIndication.m_pMsdu = (uint8_t*)malloc(macDataIndication.m_u16MsduLength*sizeof(uint8_t));
		memcpy((uint8_t *)macDataIndication.m_pMsdu, ptrMsg, macDataIndication.m_u16MsduLength);

		// Trigger the callback
		g_macNotificationsRF.m_MacWrpDataIndication(&macDataIndication);

		free((uint8_t *)(macDataIndication.m_pMsdu));
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}
	return true;
}

/**
 * @brief _macDataConfirmRF_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macDataConfirmRF_cb(uint8_t* ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_MAC("_macDataConfirmRF_cb...");

	// Check the message length
	if (len < MIN_MAC_DATA_CONFIRM_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_macNotificationsRF.m_MacWrpDataConfirm) {
		struct TMacWrpDataConfirm macDataConfirm;

		macDataConfirm.m_u8MsduHandle = (*ptrMsg++);
		macDataConfirm.m_eStatus = (*ptrMsg++);
		macDataConfirm.m_nTimestamp = (uint32_t)(*ptrMsg++);
		macDataConfirm.m_nTimestamp = (uint32_t)(*ptrMsg++) + (macDataConfirm.m_nTimestamp << 8);
		macDataConfirm.m_nTimestamp = (uint32_t)(*ptrMsg++) + (macDataConfirm.m_nTimestamp << 8);
		macDataConfirm.m_nTimestamp = (uint32_t)(*ptrMsg++) + (macDataConfirm.m_nTimestamp << 8);
		
		g_macNotificationsRF.m_MacWrpDataConfirm(&macDataConfirm);
		
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}
	return true;
}

/**
 * @brief _macResetConfirmRF_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macResetConfirmRF_cb(uint8_t* ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_MAC("_macResetConfirmRF_cb...");

	// Check the message length
	if (len < MIN_MAC_RESET_CONFIRM_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_macNotificationsRF.m_MacWrpResetConfirm) {
		struct TMacWrpResetConfirm macResetConfirm;

		macResetConfirm.m_eStatus = (*ptrMsg++);

		g_macNotificationsRF.m_MacWrpResetConfirm(&macResetConfirm);

		LOG_IFACE_G3_MAC("OK\r\n");
	}else{
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}
	return true;
}

/**
 * @brief _macSetConfirmRF_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macSetConfirmRF_cb(uint8_t* ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_MAC("_macSetConfirmRF_cb...");

	// Check the message length
	if (len < MIN_MAC_SET_CONFIRM_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_mac_sync_mgmt_rf.f_sync_req || g_macNotificationsRF.m_MacWrpSetConfirm) {
		struct TMacWrpSetConfirm macSetConfirm;

		macSetConfirm.m_eStatus = (*ptrMsg++);

		macSetConfirm.m_ePibAttribute = (*ptrMsg++);
		macSetConfirm.m_ePibAttribute = (*ptrMsg++) + (macSetConfirm.m_ePibAttribute << 8);
		macSetConfirm.m_ePibAttribute = (*ptrMsg++) + (macSetConfirm.m_ePibAttribute << 8);
		macSetConfirm.m_ePibAttribute = (*ptrMsg++) + (macSetConfirm.m_ePibAttribute << 8);

		macSetConfirm.m_u16PibAttributeIndex = (*ptrMsg++);
		macSetConfirm.m_u16PibAttributeIndex = (*ptrMsg++) + (macSetConfirm.m_u16PibAttributeIndex << 8);

		if (g_mac_sync_mgmt_rf.f_sync_req) {
			// Synchronous call -> Store the result
			memcpy(&g_mac_sync_mgmt_rf.s_SetConfirm, &macSetConfirm, sizeof(struct TMacWrpSetConfirm));
			g_mac_sync_mgmt_rf.f_sync_res = true;
		}
		else if (g_macNotificationsRF.m_MacWrpSetConfirm) {
			// Asynchronous call -> Callback
			g_macNotificationsRF.m_MacWrpSetConfirm(&macSetConfirm);
		}
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}

	return true;
}

/**
 * @brief _macGetConfirmRF_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macGetConfirmRF_cb(uint8_t* ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_MAC("_macGetConfirmRF_cb...");

	// Check the message length
	if (len < MIN_MAC_GET_CONFIRM_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_mac_sync_mgmt_rf.f_sync_req || g_macNotificationsRF.m_MacWrpGetConfirm) {
		struct TMacWrpGetConfirm macGetConfirm;

		macGetConfirm.m_eStatus = (*ptrMsg++);

		macGetConfirm.m_ePibAttribute = (*ptrMsg++);
		macGetConfirm.m_ePibAttribute = (*ptrMsg++) + (macGetConfirm.m_ePibAttribute << 8);
		macGetConfirm.m_ePibAttribute = (*ptrMsg++) + (macGetConfirm.m_ePibAttribute << 8);
		macGetConfirm.m_ePibAttribute = (*ptrMsg++) + (macGetConfirm.m_ePibAttribute << 8);

		macGetConfirm.m_u16PibAttributeIndex = (*ptrMsg++);
		macGetConfirm.m_u16PibAttributeIndex = (*ptrMsg++) + (macGetConfirm.m_u16PibAttributeIndex << 8);

		macGetConfirm.m_PibAttributeValue.m_u8Length = (*ptrMsg++);
		if (macGetConfirm.m_PibAttributeValue.m_u8Length > MAC_WRP_PIB_MAX_VALUE_LENGTH) {
			//ToDo: Log error
			return false;
		}

		memcpy(&macGetConfirm.m_PibAttributeValue.m_au8Value, ptrMsg, macGetConfirm.m_PibAttributeValue.m_u8Length);

		if (g_mac_sync_mgmt_rf.f_sync_req) {
			// Synchronous call -> Store the result
			memcpy(&g_mac_sync_mgmt_rf.s_GetConfirm, &macGetConfirm, sizeof(struct TMacWrpGetConfirm));
			g_mac_sync_mgmt_rf.f_sync_res = true;
		}
		else if (g_macNotificationsRF.m_MacWrpGetConfirm)
		{
			// Asynchronous call -> Callback
			g_macNotificationsRF.m_MacWrpGetConfirm(&macGetConfirm);
		}
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}
	
	return true;
}

/**
 * @brief _macScanConfirmRF_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macScanConfirmRF_cb(uint8_t* ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_MAC("_macScanConfirmRF_cb...");

	// Check the message length
	if (len < MIN_MAC_SCAN_CONFIRM_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_macNotificationsRF.m_MacWrpScanConfirm) {
		struct TMacWrpScanConfirm macScanConfirm;

		macScanConfirm.m_eStatus = (*ptrMsg++);
		
		g_macNotificationsRF.m_MacWrpScanConfirm(&macScanConfirm);
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}
	
	return true;
}

/**
 * @brief _macStartConfirmRF_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macStartConfirmRF_cb(uint8_t* ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_MAC("_macStartConfirmRF_cb...");

	// Check the message length
	if (len < MIN_MAC_START_CONFIRM_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_macNotificationsRF.m_MacWrpStartConfirm) {
		struct TMacWrpStartConfirm macStartConfirm;

		macStartConfirm.m_eStatus = (*ptrMsg++);
		
		g_macNotificationsRF.m_MacWrpStartConfirm(&macStartConfirm);
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}
	
	return true;
}

/**
 * @brief _macBeaconNotifyRF_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macBeaconNotifyRF_cb(uint8_t* ptrMsg, uint16_t len)
{
	LOG_IFACE_G3_MAC("_macBeaconNotifyRF_cb...");

	// Check the message length
	if (len < MIN_MAC_BEACON_NOTIFY_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_macNotificationsRF.m_MacWrpBeaconNotifyIndication) {
		struct TMacWrpBeaconNotifyIndication macBeaconNotifyIndication;

		macBeaconNotifyIndication.m_PanDescriptor.m_nPanId = *ptrMsg++ ;
		macBeaconNotifyIndication.m_PanDescriptor.m_nPanId = (*ptrMsg++) + (macBeaconNotifyIndication.m_PanDescriptor.m_nPanId << 8);
		
		macBeaconNotifyIndication.m_PanDescriptor.m_u8LinkQuality = *ptrMsg++ ;
		
		macBeaconNotifyIndication.m_PanDescriptor.m_nLbaAddress = *ptrMsg++ ;
		macBeaconNotifyIndication.m_PanDescriptor.m_nLbaAddress = (*ptrMsg++) + (macBeaconNotifyIndication.m_PanDescriptor.m_nLbaAddress << 8);
		
		macBeaconNotifyIndication.m_PanDescriptor.m_u16RcCoord = *ptrMsg++ ;
		macBeaconNotifyIndication.m_PanDescriptor.m_u16RcCoord = (*ptrMsg++) + (macBeaconNotifyIndication.m_PanDescriptor.m_u16RcCoord << 8);

		g_macNotificationsRF.m_MacWrpBeaconNotifyIndication(&macBeaconNotifyIndication);
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}

	return true;
}

/**
 * @brief _macCommStatusIndicationRF_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return True if message is correctly decoded, otherwise False
 */
static uint8_t _macCommStatusIndicationRF_cb(uint8_t* ptrMsg, uint16_t len)
{
	uint16_t us_addr_mode_len;
	
	LOG_IFACE_G3_MAC("_macCommStatusIndicationRF_cb...");

	// Check the message length
	if (len < MIN_MAC_COMM_STATUS_INDICATION_MSG_LEN) {
		LOG_IFACE_G3_MAC("ERROR\r\n");
		return false;
	}

	if (g_macNotificationsRF.m_MacWrpCommStatusIndication) {
		struct TMacWrpCommStatusIndication macCommStatusIndication;

		macCommStatusIndication.m_nPanId = *ptrMsg++;
		macCommStatusIndication.m_nPanId = (*ptrMsg++) + (macCommStatusIndication.m_nPanId << 8);
		
		// Source Address
		us_addr_mode_len = (*ptrMsg++);
		if (us_addr_mode_len == 2){
			// MAC_ADDRESS_MODE_SHORT
			macCommStatusIndication.m_SrcAddr.m_eAddrMode = MAC_ADDRESS_MODE_SHORT;
			macCommStatusIndication.m_SrcAddr.m_nShortAddress = (*ptrMsg++);
			macCommStatusIndication.m_SrcAddr.m_nShortAddress = (*ptrMsg++) + (macCommStatusIndication.m_SrcAddr.m_nShortAddress << 8);
		}else{
			// MAC_ADDRESS_MODE_EXTENDED
			macCommStatusIndication.m_SrcAddr.m_eAddrMode = MAC_ADDRESS_MODE_EXTENDED;
			memcpy(&macCommStatusIndication.m_SrcAddr.m_ExtendedAddress,ptrMsg,us_addr_mode_len);
			ptrMsg += us_addr_mode_len;
		}
		
		// Destination Address
		us_addr_mode_len = (*ptrMsg++);
		if (us_addr_mode_len == 2){
			// MAC_ADDRESS_MODE_SHORT
			macCommStatusIndication.m_DstAddr.m_eAddrMode = MAC_ADDRESS_MODE_SHORT;
			macCommStatusIndication.m_DstAddr.m_nShortAddress = (*ptrMsg++);
			macCommStatusIndication.m_DstAddr.m_nShortAddress = (*ptrMsg++) + (macCommStatusIndication.m_DstAddr.m_nShortAddress << 8);
		}else{
			// MAC_ADDRESS_MODE_EXTENDED
			macCommStatusIndication.m_DstAddr.m_eAddrMode = MAC_ADDRESS_MODE_EXTENDED;
			memcpy(&macCommStatusIndication.m_DstAddr.m_ExtendedAddress,ptrMsg,us_addr_mode_len);
			ptrMsg += us_addr_mode_len;
		}
		
		macCommStatusIndication.m_eStatus = (*ptrMsg++);
		macCommStatusIndication.m_eSecurityLevel = (*ptrMsg++);
		macCommStatusIndication.m_u8KeyIndex = (*ptrMsg++);

		g_macNotificationsRF.m_MacWrpCommStatusIndication(&macCommStatusIndication);
		LOG_IFACE_G3_MAC("OK\r\n");
	}
	else {
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
	}

	return true;
}

#endif

/**
 * @brief g3_MAC_receivedCmd
 * Copies in the Reception buffer the data received in the USI
 * @param buf: Pointer to Message buffer
 * @param len: Message length
 * @return True if is possible to extract from the usi buffer, otherwise False
 */
uint8_t g3_MAC_receivedCmd(uint8_t* buf, uint16_t len)
{
	LOG_IFACE_G3_MAC("g3_MAC_receivedCmd...\r\n");
	uint8_t uc_cmd;

	uc_cmd = (*buf++);
	len--;

	switch (uc_cmd) {
	case G3_SERIAL_MSG_STATUS:
		return _macStatus_cb(buf,len);

	case G3_SERIAL_MSG_MAC_DATA_INDICATION:
		return _macDataIndication_cb(buf,len);

	case G3_SERIAL_MSG_MAC_DATA_CONFIRM:
		return _macDataConfirm_cb(buf,len);

	case G3_SERIAL_MSG_MAC_RESET_CONFIRM:
		return _macResetConfirm_cb(buf,len);

	case G3_SERIAL_MSG_MAC_SET_CONFIRM:
		return _macSetConfirm_cb(buf,len);

	case G3_SERIAL_MSG_MAC_GET_CONFIRM:
		return _macGetConfirm_cb(buf,len);
		
	case G3_SERIAL_MSG_MAC_SCAN_CONFIRM:
		return _macScanConfirm_cb(buf,len);
	
	case G3_SERIAL_MSG_MAC_BEACON_NOTIFY:
		return _macBeaconNotify_cb(buf,len);
	
	case G3_SERIAL_MSG_MAC_START_CONFIRM:
		return _macStartConfirm_cb(buf,len);
	
	case G3_SERIAL_MSG_MAC_COMM_STATUS_INDICATION:
		return _macCommStatusIndication_cb(buf,len);

#ifdef G3_HYBRID_PROFILE
	case G3_SERIAL_MSG_MAC_DATA_INDICATION_RF:
		return _macDataIndicationRF_cb(buf,len);

	case G3_SERIAL_MSG_MAC_DATA_CONFIRM_RF:
		return _macDataConfirmRF_cb(buf,len);

	case G3_SERIAL_MSG_MAC_RESET_CONFIRM_RF:
		return _macResetConfirmRF_cb(buf,len);

	case G3_SERIAL_MSG_MAC_SET_CONFIRM_RF:
		return _macSetConfirmRF_cb(buf,len);

	case G3_SERIAL_MSG_MAC_GET_CONFIRM_RF:
		return _macGetConfirmRF_cb(buf,len);
		
	case G3_SERIAL_MSG_MAC_SCAN_CONFIRM_RF:
		return _macScanConfirmRF_cb(buf,len);
	
	case G3_SERIAL_MSG_MAC_BEACON_NOTIFY_RF:
		return _macBeaconNotifyRF_cb(buf,len);
	
	case G3_SERIAL_MSG_MAC_START_CONFIRM_RF:
		return _macStartConfirmRF_cb(buf,len);
	
	case G3_SERIAL_MSG_MAC_COMM_STATUS_INDICATION_RF:
		return _macCommStatusIndicationRF_cb(buf,len);
#endif

	// Commands not implemented in callback
	case G3_SERIAL_MSG_MAC_INITIALIZE:
	case G3_SERIAL_MSG_MAC_DATA_REQUEST:
	case G3_SERIAL_MSG_MAC_RESET_REQUEST:
	case G3_SERIAL_MSG_MAC_SET_REQUEST:
	case G3_SERIAL_MSG_MAC_GET_REQUEST:
	case G3_SERIAL_MSG_MAC_SCAN_REQUEST:
	case G3_SERIAL_MSG_MAC_START_REQUEST:
#ifdef G3_HYBRID_PROFILE
	case G3_SERIAL_MSG_MAC_INITIALIZE_RF:
	case G3_SERIAL_MSG_MAC_DATA_REQUEST_RF:
	case G3_SERIAL_MSG_MAC_RESET_REQUEST_RF:
	case G3_SERIAL_MSG_MAC_SET_REQUEST_RF:
	case G3_SERIAL_MSG_MAC_GET_REQUEST_RF:
	case G3_SERIAL_MSG_MAC_SCAN_REQUEST_RF:
	case G3_SERIAL_MSG_MAC_START_REQUEST_RF:
#endif
	default:
		LOG_IFACE_G3_MAC("UNKNOWN\r\n");
		return false;
	}

	return true;
}
