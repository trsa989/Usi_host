/**
 * \file
 *
 * \brief G3 Coordinator App
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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>

#include "AdpApi.h"
#include "AdpApiTypes.h"

#include "app_adp_mng.h"
#include "tun.h"
#include "storage.h"
#include "led.h"

#include "Config.h"
#include "globals.h"
#define LOG_LEVEL LOG_LVL_DBG
#include "Logger.h"

#include "conf_global.h"
#include "conf_bs.h"
#include "oss_if.h"
#include "bs_api.h"

static uint8_t s_uc_G3_NET_PREFIX_LEN = 64;
static uint8_t s_puc_G3_NET_IPV6[16] = {0xFD, 0x00, 0x00, 0x00, 0x00, 0x02, 0x78, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static bool s_b_write_available = 1;
static uint16_t s_us_panid;

/* MAC include */
#include "mac_wrapper.h"

/* Avoid unwanted warnings */
#define UNUSED(x) (void)(x)

extern int tunfd;

/* Check SPEC_COMPLIANCE definition */
#ifndef SPEC_COMPLIANCE
  #error "SPEC_COMPLIANCE undefined"
#endif

#ifdef APP_DEBUG_CONSOLE
#       define LOG_APP_DEBUG(format, ...)   fprintf(stderr, format, ## __VA_ARGS__)
#else
#       define LOG_APP_DEBUG(...)   (void)0
#endif

#define ADP_DATA_BLOCKING_FILE     "/tmp/G3_AdpData_blocking"
#define G3_PLC_NODE_LIST           "/tmp/g3plc_node_list"

#define NETWORK_START_TIMEOUT 10

/* constants related to network join */
enum NetworkJoinStatus {
	NETWORK_NOT_JOINED = 0,
	NETWORK_JOIN_PENDING = 1,
	NETWORK_JOINED = 2
};

/* constants related to network start */
enum NetworkStartStatus {
	NETWORK_NOT_STARTED = 0,
	NETWORK_START_PENDING = 1,
	NETWORK_STARTED = 2
};

/* global variables */
uint8_t g_u8NetworkJoinStatus = NETWORK_NOT_JOINED;
uint8_t g_u8NetworkStartStatus = NETWORK_NOT_STARTED;

/* Context information, to be updated when PanId is known */
#define CONTEXT_INFORMATION_0_SIZE   14
static uint8_t au8ConfContextInformationTable0[CONTEXT_INFORMATION_0_SIZE];

/* MIB types and definitions */
enum MibType {
	MIB_MAC = 0,
	MIB_ADP
};

struct MibData {
	uint8_t m_u8Type;
	const char *m_szName;
	uint32_t m_u32Id;
	uint16_t m_u16Index;
	uint8_t m_u8ValueLength;
	const uint8_t *m_pu8Value;
};

uint8_t g_u8MibInitIndex = 0;

#if (SPEC_COMPLIANCE >= 17)
#define APP_MIB_TABLE_SIZE (sizeof(g_MibSettings) / sizeof(struct MibData))
#endif


struct MibData g_MibSettings[] =
{
  { MIB_MAC, "MAC_WRP_PIB_MANUF_EXTENDED_ADDRESS", MAC_WRP_PIB_MANUF_EXTENDED_ADDRESS, 0, 8, CONF_EXTENDED_ADDRESS },
  { MIB_MAC, "MAC_WRP_PIB_SHORT_ADDRESS", MAC_WRP_PIB_SHORT_ADDRESS, 0, 2, CONF_SHORT_ADDRESS },
#ifdef G3_HYBRID_PROFILE
	{ MIB_MAC, "MAC_WRP_PIB_DUTY_CYCLE_LIMIT_RF", MAC_WRP_PIB_DUTY_CYCLE_LIMIT_RF, 0, 2, CONF_DUTY_CYCLE_LIMIT_RF },
#endif
#ifdef APP_CONFORMANCE_TEST
  { MIB_ADP, "ADP_IB_BLACKLIST_TABLE_ENTRY_TTL", ADP_IB_BLACKLIST_TABLE_ENTRY_TTL, 0, 2, CONF_BLACKLIST_TABLE_ENTRY_TTL },
  { MIB_ADP, "ADP_IB_GROUP_TABLE", ADP_IB_GROUP_TABLE, 0, 2, CONF_GROUP_TABLE_0 },
  { MIB_ADP, "ADP_IB_GROUP_TABLE", ADP_IB_GROUP_TABLE, 1, 2, CONF_GROUP_TABLE_1 },
#if (SPEC_COMPLIANCE >= 17)
  { MIB_MAC, "MAC_WRP_PIB_TMR_TTL", MAC_WRP_PIB_TMR_TTL, 0, 1, CONF_TMR_TTL },
#if (ENABLE_ROUTING == 1)
  { MIB_ADP, "ADP_IB_DESTINATION_ADDRESS_SET", ADP_IB_DESTINATION_ADDRESS_SET, 0, 2, CONF_DEST_ADDR_SET_0 },
#endif // ENABLE_ROUTING
#endif // SPEC_COMPLIANCE
  #ifdef G3_HYBRID_PROFILE
	{ MIB_MAC, "MAC_WRP_PIB_MAX_CSMA_BACKOFFS_RF", MAC_WRP_PIB_MAX_CSMA_BACKOFFS_RF, 0, 1, CONF_MAX_CSMA_BACKOFFS_RF },
	{ MIB_MAC, "MAC_WRP_PIB_MAX_FRAME_RETRIES_RF", MAC_WRP_PIB_MAX_FRAME_RETRIES_RF, 0, 1, CONF_MAX_FRAME_RETRIES_RF },
	{ MIB_MAC, "MAC_WRP_PIB_POS_TABLE_ENTRY_TTL", MAC_WRP_PIB_POS_TABLE_ENTRY_TTL, 0, 1, CONF_POS_TABLE_TTL },
    #if (ENABLE_ROUTING == 1)
	{ MIB_ADP, "ADP_IB_KR", ADP_IB_KR, 0, 1, CONF_KR },
	{ MIB_ADP, "ADP_IB_KM", ADP_IB_KM, 0, 1, CONF_KM },
	{ MIB_ADP, "ADP_IB_KC", ADP_IB_KC, 0, 1, CONF_KC },
	{ MIB_ADP, "ADP_IB_KQ", ADP_IB_KQ, 0, 1, CONF_KQ },
	{ MIB_ADP, "ADP_IB_KH", ADP_IB_KH, 0, 1, CONF_KH },
	{ MIB_ADP, "ADP_IB_KRT", ADP_IB_KRT, 0, 1, CONF_KRT },
	{ MIB_ADP, "ADP_IB_KQ_RF", ADP_IB_KQ_RF, 0, 1, CONF_KQ_RF },
	{ MIB_ADP, "ADP_IB_KH_RF", ADP_IB_KH_RF, 0, 1, CONF_KH_RF },
	{ MIB_ADP, "ADP_IB_KRT_RF", ADP_IB_KRT_RF, 0, 1, CONF_KRT_RF },
	{ MIB_ADP, "ADP_IB_KDC_RF", ADP_IB_KDC_RF, 0, 1, CONF_KDC_RF },
    #endif
  #endif
#endif // APP_CONFORMANCE_TEST
  { MIB_ADP, "ADP_IB_CONTEXT_INFORMATION_TABLE", ADP_IB_CONTEXT_INFORMATION_TABLE, 0, 14, CONF_CONTEXT_INFORMATION_TABLE_0 },
  { MIB_ADP, "ADP_IB_CONTEXT_INFORMATION_TABLE", ADP_IB_CONTEXT_INFORMATION_TABLE, 1, 10, CONF_CONTEXT_INFORMATION_TABLE_1 },
#if (ENABLE_ROUTING == 1)
	{ MIB_ADP, "ADP_IB_ROUTING_TABLE_ENTRY_TTL", ADP_IB_ROUTING_TABLE_ENTRY_TTL, 0, 2, CONF_ROUTING_TABLE_ENTRY_TTL },
#endif
	{ MIB_ADP, "ADP_IB_MAX_JOIN_WAIT_TIME", ADP_IB_MAX_JOIN_WAIT_TIME, 0, 2, CONF_MAX_JOIN_WAIT_TIME },
	{ MIB_ADP, "ADP_IB_MAX_HOPS", ADP_IB_MAX_HOPS, 0, 1, CONF_MAX_HOPS },
  { MIB_ADP, "ADP_IB_MANUF_EAP_PRESHARED_KEY", ADP_IB_MANUF_EAP_PRESHARED_KEY, 0, 16, CONF_PSK_KEY },
};

/**
 * \brief G3 ADP Set Confirm Callback Function for ADP and MAC
 *
 * @param u8Status           Status of the SetRequest operation
 * @param u32AttributeId     Attribute ID of the Set Request operation
 * @param u16AttributeIndex  Attribute Index of the Set Request operation
 */
static void SetConfirm(uint8_t u8Status, uint32_t u32AttributeId, uint16_t u16AttributeIndex)
{
	if (u8Status == G3_SUCCESS) {
		/* initialization phase? */
		if (g_u8MibInitIndex < APP_MIB_TABLE_SIZE) {
			if ((g_MibSettings[g_u8MibInitIndex].m_u32Id == u32AttributeId) && (g_MibSettings[g_u8MibInitIndex].m_u16Index == u16AttributeIndex)) {
				if (g_u8MibInitIndex == APP_MIB_TABLE_SIZE) {
					LOG_INFO(Log("Modem fully initialized"));
				}
			} else {
				LOG_ERR(Log("ERR[AppAdpSetConfirm] Invalid SetConfirm received during initialization. Expecting 0x%08X/%u but received 0x%08X/%u",
						g_MibSettings[g_u8MibInitIndex].m_u32Id, g_MibSettings[g_u8MibInitIndex].m_u16Index,
						u32AttributeId, u16AttributeIndex));
			}
		}
	} else {
		LOG_ERR(Log("ERR[AppAdpSetConfirm] status: %u\r\n", u8Status));
	}

	/* g_u8MibInitIndex++; */
}

/**
 * \brief G3 ADP Data Confirm Callback Function
 * Received after the completion of a Data request
 * @param TAdpDataConfirm Pointer:
 *  - m_u8Status The status code of a previous AdpDataRequest identified by its NsduHandle.
 *  - m_u8NsduHandle The handle of the NSDU confirmed by this primitive.
 */
static void AppAdpDataConfirm(struct TAdpDataConfirm *pDataConfirm)
{
	UNUSED(pDataConfirm);
}

/**
 * \brief G3 ADP Data Indication Callback Function
 * The AdpDataIndication primitive is used to transfer received data from the adaptation sublayer to the upper layer.
 * @param TAdpDataIndication Pointer:
 *  - m_u16NsduLength The size of the NSDU, in bytes; Up to 1280 bytes
 *  - m_pNsdu The received NSDU
 *  - m_u8LinkQualityIndicator The value of the link quality during reception of the frame.
 */
static void AppAdpDataIndication(struct TAdpDataIndication *pDataIndication)
{
	if (tunfd >= 0) {
		int ret;
		uint8_t buffer[1284];

		/* On this example the Data from G3 Network will be forwarded to TUN device */
		/* TUN Frame Format (flag IFF_NO_PI not set): Flags [2bytes] + Proto[2 bytes - IPv6 0x86DD] + Raw Protocol [IPv6 Frame] */

		buffer[0] = 0;
		buffer[1] = 0;
		buffer[2] = 0x86;
		buffer[3] = 0xDD;
		memcpy(buffer + 4, pDataIndication->m_pNsdu, pDataIndication->m_u16NsduLength );
		ret = write(tunfd, buffer, pDataIndication->m_u16NsduLength + 4 );
		if (ret >= 0) {
			LOG_DBG(Log("AppAdpDataIndication DATA: %u LQI: %u", pDataIndication->m_u16NsduLength, pDataIndication->m_u8LinkQualityIndicator));
		} else {
			LOG_ERR(Log("AppAdpDataIndication ERROR LEN: %u LQI: %u", pDataIndication->m_u16NsduLength, pDataIndication->m_u8LinkQualityIndicator));
		}
	}
}

/**
 * \brief G3 ADP Discovery Confirm Callback Function
 * The AdpDiscoveryConfirm primitive allows the upper layer to be notified of the completion of an AdpDiscoveryRequest (BREQ Procedure started).
 * @param uc_status
 */
static void AppAdpDiscoveryConfirm(uint8_t uc_status)
{
	if (uc_status == G3_SUCCESS) {
		LOG_INFO(Log("AppAdpDiscoveryConfirm SUCCESS"));
		/* The coordinator is the network creator. Network join is not used. */
		g_u8NetworkJoinStatus = NETWORK_JOINED;
	} else {
		LOG_ERR(Log("AppAdpDiscoveryConfirm ERROR - Status %u", uc_status));
		g_u8NetworkJoinStatus = NETWORK_NOT_JOINED;
	}
}

/**
 * \brief G3 ADP Discovery Indication Callback Function
 * The AdpDiscoveryIndication primitive is generated by the ADP layer to notify the application about the discovery
 * of a new PAN coordinator or LBA
 * @param TAdpPanDescriptor Pointer
 *  - u16PanId The 16-bit PAN identifier.
 *  - u8LinkQuality The 8-bit link quality of LBA.
 *  - u16LbaAddress The 16 bit short address of a device in this PAN to be used as the LBA by the associating device.
 *  - u16RcCoord The estimated route cost from LBA to the coordinator.
 */
static void AppAdpDiscoveryIndication(struct TAdpPanDescriptor *pPanDescriptor)
{
	UNUSED(pPanDescriptor);
}

/**
 * \brief G3 ADP Network Start Confirm Callback Function
 * The AdpNetworkStartConfirm primitive allows the upper layer to be notified of the completion of an
 * AdpNetworkStartRequest. Only used on Coordinator.
 * @param TAdpNetworkStartConfirm:
 *  - m_u8Status The status of the request.
 */
static void AppAdpNetworkStartConfirm(struct TAdpNetworkStartConfirm *pNetworkStartConfirm)
{
  /* Only used on Coordinator */
  LOG_DBG(Log("AppAdpNetworkStartConfirm: %d\r\n",pNetworkStartConfirm->m_u8Status));
  if (pNetworkStartConfirm->m_u8Status == G3_SUCCESS)
     g_u8NetworkStartStatus = NETWORK_STARTED;
}

/**
 * \brief G3 ADP Network Join Confirm Callback Function
 * The AdpNetworkJoinConfirm primitive allows the upper layer to be notified of the completion of an
 * AdpNetworkJoinRequest. Only used on Device.
 * @param TAdpNetworkJoinConfirm Pointer:
 *   - m_u8Status The status of the request.
 *   - m_u16NetworkAddress The 16-bit network address that was allocated to the device.
 *   - m_u16PanId The 16-bit address of the PAN of which the device is now a member.
 */
static void AppAdpNetworkJoinConfirm(struct TAdpNetworkJoinConfirm *pNetworkJoinConfirm)
{
	UNUSED(pNetworkJoinConfirm);
}

/**
 * \brief G3 ADP Network Leave Indication Callback Function
 * The AdpNetworkLeaveIndication primitive is generated by the ADP layer of a non-coordinator device to inform
 * the upper layer that it has been unregistered from the network by the coordinator.
 */
static void AppAdpNetworkLeaveIndication(void)
{
}

/**
 * \brief G3 ADP Network Leave Confirm Callback Function
 * The AdpNetworkLeaveConfirm primitive allows the upper layer to be notified of the completion of an
 * AdpNetworkLeaveRequest
 * @param TAdpNetworkLeaveConfirm Pointer:
 *  - m_u8Status  The status of the request.
 */
static void AppAdpNetworkLeaveConfirm(struct TAdpNetworkLeaveConfirm *pLeaveConfirm)
{
	UNUSED(pLeaveConfirm);
}

/**
 * \brief G3 ADP Reset Confirm Callback Function
 * The AdpResetConfirm primitive allows the upper layer to be notified of the completion of an AdpResetRequest.
 * @param TAdpResetConfirm Pointer:
 *  - m_u8Status  The status of the request.
 */
static void AppAdpResetConfirm(struct TAdpResetConfirm *pResetConfirm)
{
	UNUSED(pResetConfirm);
}

/**
 * \brief G3 ADP Set Confirm Callback Function
 * The AdpSetConfirm primitive allows the upper layer to be notified of the completion of an AdpSetRequest.
 * @param TAdpSetConfirm Pointer:
 *  - m_u8Status The status of the request.
 *  - m_u32AttributeId The identifier of the IB attribute set.
 *  - m_u16AttributeIndex The index within the table of the specified IB attribute.
 */
static void AppAdpSetConfirm(struct TAdpSetConfirm *pSetConfirm)
{
	SetConfirm(pSetConfirm->m_u8Status, pSetConfirm->m_u32AttributeId, pSetConfirm->m_u16AttributeIndex);
}

/**
 * \brief G3 ADP MAC Set Confirm Callback Function
 * The AppAdpMacSetConfirm primitive allows the upper layer to be notified of the completion of an AdpMacSetRequest.
 * @param TAdpMacSetConfirm Pointer:
 *  - m_u8Status The status of the request.
 *  - m_u32AttributeId The identifier of the IB attribute set.
 *  - m_u16AttributeIndex The index within the table of the specified IB attribute.
 */
static void AppAdpMacSetConfirm(struct TAdpMacSetConfirm *pSetConfirm)
{
	SetConfirm(pSetConfirm->m_u8Status, pSetConfirm->m_u32AttributeId, pSetConfirm->m_u16AttributeIndex);
}

/**
 * \brief G3 ADP Get Confirm Callback Function
 * The AppAdpGetConfirm primitive allows the upper layer to be notified of the completion of an AdpGetRequest.
 * @param TAdpGetConfirm Pointer:
 *  - m_u8Status The status of the request.
 *  - m_u32AttributeId The identifier of the IB attribute read
 *  - m_u16AttributeIndex The index within the table of the specified IB attribute read.
 *  - m_u8AttributeLength The length of the value of the attribute read from the IB.
 *  - m_au8AttributeValue The value of the attribute read from the IB.
 */
static void AppAdpGetConfirm(struct TAdpGetConfirm *pGetConfirm)
{
	UNUSED(pGetConfirm);
}

/**
 * \brief G3 ADP MAC Get Confirm Callback Function
 * The AppAdpMacGetConfirm primitive allows the upper layer to be notified of the completion of an AdpMacGetRequest.
 * @param TAdpMacGetConfirm Pointer:
 *  - m_u8Status The status of the request.
 *  - m_u32AttributeId The identifier of the IB attribute read
 *  - m_u16AttributeIndex The index within the table of the specified IB attribute read.
 *  - m_u8AttributeLength The length of the value of the attribute read from the IB.
 *  - m_au8AttributeValue The value of the attribute read from the IB.
 */
static void AppAdpMacGetConfirm(struct TAdpMacGetConfirm *pGetConfirm)
{
	UNUSED(pGetConfirm);
}

/**
 * \brief G3 ADP Route Discovery Confirm Callback Function
 * The AdpRouteDiscoveryConfirm primitive allows the upper layer to be notified of the completion of AdpRouteDiscoveryRequest.
 * @param TAdpRouteDiscoveryConfirm Pointer:
 *  - m_u8Status  The status of the request.
 */
static void AppAdpRouteDiscoveryConfirm(struct TAdpRouteDiscoveryConfirm *pRouteDiscoveryConfirm)
{
	UNUSED(pRouteDiscoveryConfirm);
}

/**
 * \brief G3 ADP Path Discovery Confirm Callback Function
 * The AdpPathDiscoveryConfirm primitive allows the upper layer to be notified of the completion of a
 * AdpPathDiscoveryRequest.
 * @param TAdpPathDiscoveryConfirm Pointer:
 *  - m_u8Status The status of the path discovery. (status can be INCOMPLETE and the other parameters contain the discovered path)
 *  - m_u16DstAddr The short unicast destination address of the path discovery.
 *  - m_u16Originator The originator of the path reply
 *  - m_u8PathMetricType Path metric type
 *  - m_u8ForwardHopsCount Number of path hops in the forward table
 *  - m_u8ReverseHopsCount Number of path hops in the reverse table
 *  - m_aForwardPath Table with the information of each hop in forward direction (according to m_u8ForwardHopsCount)
 *  - m_aReversePath Table with the information of each hop in reverse direction (according to m_u8ReverseHopsCount)
 */
static void AppAdpPathDiscoveryConfirm(struct TAdpPathDiscoveryConfirm *pPathDiscoveryConfirm)
{
	UNUSED(pPathDiscoveryConfirm);
}

/**
 * \brief G3 ADP Network Status Indication Callback Function
 * The AdpNetworkStatusIndication primitive allows the next higher layer of a PAN coordinator or a coordinator to be
 * notified when a particular event occurs on the PAN.
 * @param TAdpNetworkStatusIndication Pointer:
 *  - m_u16PanId The 16-bit PAN identifier of the device from which the frame was received or to which the frame was being sent.
 *  - m_SrcDeviceAddress The individual device address of the entity from which the frame causing the error originated.
 *  - m_DstDeviceAddress The individual device address of the device for which the frame was intended.
 *  - m_u8Status The communications status.
 *  - m_u8SecurityLevel The security level purportedly used by the received frame.
 *  - m_u8KeyIndex The index of the key purportedly used by the originator of the received frame.
 */
static void AppAdpNetworkStatusIndication(struct TAdpNetworkStatusIndication *pNetworkStatusIndication)
{
	UNUSED(pNetworkStatusIndication);
}

/**
 * \brief G3 ADP Buffer Indication Callback Function
 * The AdpBufferIndication primitive allows the next higher layer to be notified when the modem has reached its
 *  capability limit to perform the next frame. Some Flow Control Mechanism could be implemented thanks to this feature.
 * @param TAdpBufferIndication Pointer:
 *  - m_bBufferReady TRUE: modem is ready to receipt more data frame;
 *                   FALSE: modem is not ready, stop sending data frame.
 */
static void AppAdpBufferIndication(struct TAdpBufferIndication *pBufferIndication)
{
	if (s_b_write_available != pBufferIndication->m_bBufferReady) {
		s_b_write_available = pBufferIndication->m_bBufferReady;
		if (pBufferIndication->m_bBufferReady) {
			/* ADP Buffers ready */
			LOG_DBG(Log("ADP Buffers ready now!"));
			/* Remove ADP Data Blocking File */
			unlink(ADP_DATA_BLOCKING_FILE);
			SET_LED_RED_OFF();
		} else {
			/* ADP Buffers NOT ready */
			LOG_DBG(Log("ADP Buffers NOT ready!"));
			/* Create ADP Data Blocking File */
			int fd_block;
			fd_block = open(ADP_DATA_BLOCKING_FILE, O_CREAT | O_RDONLY, S_IRUSR | S_IRGRP | S_IROTH);
			if (fd_block == -1) {
				LOG_ERR(Log("Impossible to create ADP Data Blocking File: %s", strerror(errno)));
				exit(1);
			}

			close(fd_block);
			SET_LED_RED_HEARTBEAT(LED_HEARTBEAT_NOT_INVERTED);
		}
	}
}

/**
 * \brief G3 ADP Upd Non Volatile Data Indication Callback Function
 * The AdpUpdNonVolatileDataIndication primitive allows the next higher layer to be notified when non-volatile stored
 * data must be updated to protect the system in case of critical failure. This feature could be implemented on Modem.
 */
static void AppAdpNotification_UpdNonVolatileDataIndication(void)
{
#ifdef ENABLE_PIB_RESTORE
  	store_persistent_info();
#endif
}

/**
 * \brief G3 ADP Route Not Found Indication Callback Function
 * The AppAdpNotification_RouteNotFoundIndication primitive allows the next higher layer to be notified when
 * a route is not found. This feature could be implemented on Modem.
 */
static void AppAdpNotification_RouteNotFoundIndication(struct TAdpRouteNotFoundIndication *pRouteNotFoundIndication)
{
	UNUSED(pRouteNotFoundIndication);
}

/**
 * \brief G3 ADP PREQ Indication Callback Function
 * The AppAdpNotification_PREQIndication primitive allows the next higher layer to be notified when
 * a PREQ message is received. This feature could be implemented on Modem.
 */
static void AppAdpNotification_PREQIndication(void)
{
}

/**
 * \brief G3 Boot Strapping Join Indication Callback Function
 * The AppBsJoinIndication primitive allows the application to be notified when the boot strapping module has registered a
 * new device on the G3 Network.
 * @param puc_extended_address:  EUI64 Device Address
 * @param us_short_address:      Short Address assigned to device
 */
static void AppBsJoinIndication(uint8_t *puc_extended_address, uint16_t us_short_address)
{
	x_node_list_t node_list[16];
	int n_nodes, index = 0;
	FILE *fd;
	uint16_t short_address;

	/* This application propagate this information to file in order to be used by IP applications */
	SET_LED_BLUE_HEARTBEAT(LED_HEARTBEAT_NOT_INVERTED)
	fd = fopen(G3_PLC_NODE_LIST, "w");
	if (fd == NULL) {
		LOG_ERR(Log("file is created"));
	}

	/* Update list of registered nodes */
	n_nodes = app_update_registered_nodes(&node_list[0]);
	if (n_nodes) {
		LOG_INFO(Log("Updating Node List with %d devices", n_nodes));
		for (index = 0; index < n_nodes; index++) {
			short_address = node_list[index].us_short_address;
			LOG_INFO(Log("Registered Short Address -> 0x%04X", short_address));
			fprintf(fd, "fe80::%04x:ff:fe00:%X\r\n", g_st_config.us_pan_id, short_address);
		}
	}

	fclose(fd);
}

/**
 * \brief G3 Boot Strapping Leave Indication Callback Function
 * The AppBsLeaveIndication primitive allows the application to be notified when the boot strapping module receive a
 * BS KICK message from the Network
 * @param u16SrcAddr:       Short Address assigned to device
 * @param bSecurityEnabled: Security Enabled Flag
 * @param u8LinkQualityIndicator: Link Quality Indicator
 * @param pNsdu The received NSDU
 * @param u16NsduLength The size of the NSDU, in bytes;
 */
static void AppBsLeaveIndication(uint16_t u16SrcAddr, bool bSecurityEnabled, uint8_t u8LinkQualityIndicator, uint8_t *pNsdu, uint16_t u16NsduLength)
{
	UNUSED(bSecurityEnabled);
	UNUSED(u8LinkQualityIndicator);
	UNUSED(pNsdu);
	UNUSED(u16NsduLength);
}

static struct TAdpNotifications notifications;

/**
 * \brief Initialize G3 Stack
 * @param uc_plc_band: G3 Band in use
 */
static void InitializeStack(uint8_t uc_plc_band)
{
	TBootstrapAdpNotifications *ps_bs_notifications;
	struct TAdpSetConfirm pSetConfirm;

	/* Set Bootstrap managment */
	ps_bs_notifications = bs_get_not_handlers();

	/* Set ADP managment */
	notifications.fnctAdpDataConfirm = AppAdpDataConfirm;
	notifications.fnctAdpDataIndication = AppAdpDataIndication;
	notifications.fnctAdpDiscoveryConfirm = AppAdpDiscoveryConfirm;
	notifications.fnctAdpDiscoveryIndication = AppAdpDiscoveryIndication;
	notifications.fnctAdpNetworkStartConfirm = AppAdpNetworkStartConfirm;
	notifications.fnctAdpNetworkJoinConfirm = AppAdpNetworkJoinConfirm;
	notifications.fnctAdpNetworkLeaveIndication = AppAdpNetworkLeaveIndication;
	notifications.fnctAdpNetworkLeaveConfirm = AppAdpNetworkLeaveConfirm;
	notifications.fnctAdpResetConfirm = AppAdpResetConfirm;
	notifications.fnctAdpSetConfirm = AppAdpSetConfirm;
	notifications.fnctAdpMacSetConfirm = AppAdpMacSetConfirm;
	notifications.fnctAdpGetConfirm = AppAdpGetConfirm;
	notifications.fnctAdpMacGetConfirm = AppAdpMacGetConfirm;
	notifications.fnctAdpLbpConfirm = ps_bs_notifications->fnctAdpLbpConfirm;
	notifications.fnctAdpLbpIndication = ps_bs_notifications->fnctAdpLbpIndication;
	notifications.fnctAdpRouteDiscoveryConfirm = AppAdpRouteDiscoveryConfirm;
	notifications.fnctAdpPathDiscoveryConfirm = AppAdpPathDiscoveryConfirm;
	notifications.fnctAdpNetworkStatusIndication = AppAdpNetworkStatusIndication;
	notifications.fnctAdpBufferIndication = AppAdpBufferIndication;
	notifications.fnctAdpUpdNonVolatileDataIndication = AppAdpNotification_UpdNonVolatileDataIndication;
	notifications.fnctAdpRouteNotFoundIndication = AppAdpNotification_RouteNotFoundIndication;
	notifications.fnctAdpPREQIndication = AppAdpNotification_PREQIndication;

	AdpInitialize(&notifications, uc_plc_band);
}

/**
 * \brief Initialize G3 Modem Parameters
 */
static void InitializeModemParameters(void)
{
	struct TAdpSetConfirm pSetConfirm;
	struct TAdpMacSetConfirm pMacSetConfirm;

	while (g_u8MibInitIndex < APP_MIB_TABLE_SIZE) {
		if (g_u8MibInitIndex == 0) {
			LOG_INFO(Log("Start modem initialization"));
		}

		LOG_DBG(Log("Setting command %02u: %s / %u", g_u8MibInitIndex, g_MibSettings[g_u8MibInitIndex].m_szName, g_MibSettings[g_u8MibInitIndex].m_u16Index));
		if ((g_MibSettings[g_u8MibInitIndex].m_u8Type == 0) && (g_MibSettings[g_u8MibInitIndex].m_szName == 0) && (g_MibSettings[g_u8MibInitIndex].m_u16Index == 0)) {
			return;
		}

		if (g_MibSettings[g_u8MibInitIndex].m_u8Type == MIB_ADP) {
			/* AdpSetRequest(g_MibSettings[g_u8MibInitIndex].m_u32Id, g_MibSettings[g_u8MibInitIndex].m_u16Index, g_MibSettings[g_u8MibInitIndex].m_u8ValueLength, g_MibSettings[g_u8MibInitIndex].m_pu8Value); */
			AdpSetRequestSync(g_MibSettings[g_u8MibInitIndex].m_u32Id, g_MibSettings[g_u8MibInitIndex].m_u16Index, g_MibSettings[g_u8MibInitIndex].m_u8ValueLength, g_MibSettings[g_u8MibInitIndex].m_pu8Value, &pSetConfirm);
		} else {
			/* AdpMacSetRequest(g_MibSettings[g_u8MibInitIndex].m_u32Id, g_MibSettings[g_u8MibInitIndex].m_u16Index, g_MibSettings[g_u8MibInitIndex].m_u8ValueLength, g_MibSettings[g_u8MibInitIndex].m_pu8Value); */
			AdpMacSetRequestSync(g_MibSettings[g_u8MibInitIndex].m_u32Id, g_MibSettings[g_u8MibInitIndex].m_u16Index, g_MibSettings[g_u8MibInitIndex].m_u8ValueLength, g_MibSettings[g_u8MibInitIndex].m_pu8Value, &pMacSetConfirm);
		}

		g_u8MibInitIndex++;
	}
}

#ifdef APP_CONFORMANCE_TEST
/* In conformance, send beacon request before starting network*/
static void StartDiscovery(void)
{
	g_u8NetworkJoinStatus = NETWORK_JOIN_PENDING;

	AdpDiscoveryRequest(10);
}

#endif

/**
 * \brief Initialize G3 Network Parameters
 */
static void InitializeNetworkParameters(uint16_t us_panid, uint8_t *ipv6addr, uint8_t uc_net_len)
{
	struct TAdpSetConfirm pSetConfirm;
	struct ipv6_prefix net_prefix;

	s_b_write_available = 1;
	s_us_panid = us_panid;
	s_uc_G3_NET_PREFIX_LEN = uc_net_len;
	memcpy(s_puc_G3_NET_IPV6, ipv6addr, 16);

	/* Set Network Parameters */
	net_prefix.uc_prefix_len = s_uc_G3_NET_PREFIX_LEN; /* bits */
	net_prefix.uc_on_link_flag = 1;
	net_prefix.uc_auto_config_flag = 1;
	net_prefix.ui_valid_life_time = 0x20C000; /* infinite */
	net_prefix.ui_preferred_life_time = 0x20C000; /* infinite */
	memcpy(net_prefix.puc_prefix, s_puc_G3_NET_IPV6, 16);
	/* AdpSetRequest(ADP_IB_PREFIX_TABLE,0,sizeof(struct ipv6_prefix),(uint8_t*)&net_prefix); */
	AdpSetRequestSync(ADP_IB_PREFIX_TABLE, 0, sizeof(struct ipv6_prefix), (uint8_t *)&net_prefix, &pSetConfirm);
}

static void StartCoordinator(uint8_t uc_plc_band, uint16_t us_panid)
{
	struct TAdpSetConfirm pSetConfirm;
	struct TAdpGetConfirm pGetConfirm;
	TBootstrapConfiguration s_bs_conf;
	struct t_bs_lbp_set_param_confirm lbp_set_confirm;

	LOG_INFO(Log("Starting coordinator..."));

#ifdef ENABLE_PIB_RESTORE
	load_persistent_info();
#endif

	s_bs_conf.m_u8BandInfo = uc_plc_band;
	AdpGetRequestSync(ADP_IB_MAX_HOPS, 0, &pGetConfirm);
	s_bs_conf.m_u8MaxHop = pGetConfirm.m_au8AttributeValue[0];

	/* Set PSK -> Only copies PSK key on BS global var */
	bs_lbp_set_param(LBP_IB_PSK, 0, 16, g_st_config.uc16_psk_key, &lbp_set_confirm);
	/* Set GMK -> Only copies GMK key on BS global var */
	bs_lbp_set_param(LBP_IB_GMK, 0, 16, g_st_config.uc16_gmk_key, &lbp_set_confirm);

	/* Set ADP PSK if configurable */
	/* AdpSetRequestSync(ADP_IB_MANUF_EAP_PRESHARED_KEY, 0, 16, g_st_config.uc16_psk_key, &pSetConfirm); */

	/* Init Bootstrap module */
	bs_init(s_bs_conf); /*! Initialize GMK on ADP_MAC_SERIALIZED Layer */
	bs_lbp_join_ind_set_cb(AppBsJoinIndication);
	bs_lbp_leave_ind_set_cb(AppBsLeaveIndication);

	/* Start G3 Network */
	AdpNetworkStartRequest(s_us_panid);
}

/**
 * \brief G3 ADP Initialization
 *
 * @param uc_plc_band Frequency band to work with.
 * @param us_panid    PANID of the PLC network
 * @param puc_ipaddr  Coordinator's IP6 address
 * @param uc_net_len  IP6 Network prefix length
 */
void adp_init(uint8_t uc_plc_band, uint16_t us_panid, uint8_t *puc_ipaddr, uint8_t uc_net_len)
{
	struct in6_addr ip6addr;

	/* Init modules */
	InitializeStack(uc_plc_band);
	InitializeModemParameters();
	inet_pton(AF_INET6, puc_ipaddr, &ip6addr);
	InitializeNetworkParameters(us_panid, (uint8_t *)&ip6addr, uc_net_len);
	/*! Remove List of G3 PLC devices registered */
	unlink(G3_PLC_NODE_LIST);
	/*! Remove ADP Buffers Blocking file */
	unlink(ADP_DATA_BLOCKING_FILE);
}

/**
 * \brief G3 ADP Process need to be periodically called
 *
 * @param uc_plc_band Frequency band to work with.
 * @param us_panid    PANID of the PLC network
 */
void adp_process(uint8_t uc_plc_band, uint16_t us_panid)
{
  static uint32_t start_t = 0;
  uint32_t end_t = 0;
  uint32_t diff_t = 0;

    if(g_u8NetworkJoinStatus == NETWORK_NOT_JOINED) {
#ifdef APP_CONFORMANCE_TEST
       if (g_st_config.b_conformance){
          /* In conformance, send beacon request before starting network*/
          StartDiscovery();
       }else{
          g_u8NetworkJoinStatus = NETWORK_JOINED;
 	      LOG_INFO(Log("Network JoinStatus Joined"));
       }
#else
          g_u8NetworkJoinStatus = NETWORK_JOINED;
#endif
	} else if(g_u8NetworkJoinStatus == NETWORK_JOINED) {
		// network start will start only after the modem is initialized
		if (g_u8NetworkStartStatus == NETWORK_NOT_STARTED) {
	          /* Start node as coordinator */
        	  StartCoordinator(uc_plc_band,us_panid);
	          g_u8NetworkStartStatus = NETWORK_START_PENDING;
	          // Timer Waiting Network Initialization
        	  start_t = oss_get_up_time_ms();
		}else if (g_u8NetworkStartStatus == NETWORK_START_PENDING){
	          end_t = oss_get_up_time_ms();
        	  diff_t = (end_t-start_t) / 1000;
	          if (diff_t > NETWORK_START_TIMEOUT){
        	     LOG_INFO(Log("Coordinator Start Timeout %d\n", diff_t));
	             // Something was wrong - Reset the Network
        	     g_u8NetworkJoinStatus == NETWORK_NOT_JOINED;
	             g_u8NetworkStartStatus == NETWORK_NOT_STARTED;
        	     AdpResetRequest();
	          }
		}
	}

	/* Call bootstrap process*/
	bs_process();
}

/**
 * \brief Update the list of registered nodes from Bootstrap module.
 *
 * @param pxNodeList G3 Device List
 */
uint16_t app_update_registered_nodes(void *pxNodeList)
{
	x_node_list_t *px_list_ptr;
	struct t_bs_lbp_get_param_confirm p_get_confirm;
	uint16_t us_idx, us_num_devices, us_device_cnt;

	px_list_ptr = pxNodeList;

	/* Get the number of devices from Bootstrap module */
	us_num_devices = bs_lbp_get_lbds_counter();
	us_device_cnt = 0;

	/* If no devices found, return */
	if (us_num_devices == 0) {
		return 0;
	}

	/* Update Device Addresses from Bootstrap module */
	for (us_idx = 0; us_idx < MAX_LBDS; us_idx++) {
		bs_lbp_get_param(LBP_IB_DEVICE_LIST, us_idx, &p_get_confirm);
		if (p_get_confirm.uc_status == LBP_STATUS_OK) {
			px_list_ptr->us_short_address = ((uint16_t)p_get_confirm.uc_attribute_value[1]) << 8;
			px_list_ptr->us_short_address += ((uint16_t)p_get_confirm.uc_attribute_value[0]);
			memcpy(px_list_ptr->puc_extended_address, &p_get_confirm.uc_attribute_value[2], sizeof(px_list_ptr->puc_extended_address));
			px_list_ptr++;
			us_device_cnt++;
			if (us_device_cnt == us_num_devices) {
				break;
			}
		}
	}

	return us_num_devices;
}

/**
 * \brief G3 ADP Buffers availability
 *
 */
bool adp_write_buffers_available()
{
	return s_b_write_available;
}

/**
 * \brief Sent IPv6 Message through G3 Stack
 *
 */
int  adp_send_ipv6_message(uint8_t *puc_buffer, uint16_t us_length)
{
	uint8_t puc_dest_extended_addr[EXT_ADDR_LEN];
	uint16_t us_short_addr;
	static uint8_t uc_nsdu_handle = 0;
	struct TAdpSetConfirm pSetConfirm;

	if ((!puc_buffer) || (us_length > G3_ADP_MAX_DATA_LENTHG)) {
		LOG_ERR(Log("ERROR: MSG too large %d", us_length));
		return -1;
	}

#ifdef APP_CONFORMANCE_TEST
	if ((puc_buffer[24] == 0xFF) && (puc_buffer[25] == 0x02)) {
		LOG_ERR(Log("DO NOT FORWARD FF02 broadcast messages in conformance"));
		return -1;
	}
#endif
	/* Get destination EUI64 address in IPv6 header */
	memcpy(puc_dest_extended_addr, puc_buffer + 24 + 8, EXT_ADDR_LEN);

	if (bs_get_short_addr_by_ext(puc_dest_extended_addr, &us_short_addr) == true) {
		/* AdpSetRequestSync(ADP_IB_MANUF_IPV6_ULA_DEST_SHORT_ADDRESS, 0, sizeof(us_short_addr),(uint8_t*)&us_short_addr, &pSetConfirm); */
		AdpSetRequest(ADP_IB_MANUF_IPV6_ULA_DEST_SHORT_ADDRESS, 0, sizeof(us_short_addr), (uint8_t *)&us_short_addr);
		AdpDataRequest(us_length, puc_buffer, uc_nsdu_handle++, true, 0x00);
		return 0;
	} else if ((puc_dest_extended_addr[0] == ((uint8_t)(g_st_config.us_pan_id >> 8))) &&
			(puc_dest_extended_addr[1] == ((uint8_t)(g_st_config.us_pan_id)))) {
		/* ULA 2nd address,LL Address. */
		us_short_addr = puc_dest_extended_addr[6] << 8;
		us_short_addr += puc_dest_extended_addr[7];
		/* Verify if this short address was registered before */
		if (bs_get_ext_addr_by_short(us_short_addr, puc_dest_extended_addr) == true) {
			/* AdpSetRequestSync(ADP_IB_MANUF_IPV6_ULA_DEST_SHORT_ADDRESS, 0, sizeof(us_short_addr),(uint8_t*)&us_short_addr, &pSetConfirm); */
			/* AdpSetRequest(ADP_IB_MANUF_IPV6_ULA_DEST_SHORT_ADDRESS, 0, sizeof(us_short_addr),(uint8_t*)&us_short_addr); */
			AdpDataRequest(us_length, puc_buffer, uc_nsdu_handle++, true, 0x00);
		} else {
			LOG_ERR(Log("Short Address 0x%X not registered\r\n", us_short_addr));
		}

		return 0;
	} else if ((puc_buffer[24] == 0xFF)  && (puc_buffer[25] == 0x02)) {
		/* Multicast, Link local scope */
		us_short_addr = 0xFFFF;
		/* AdpSetRequestSync(ADP_IB_MANUF_IPV6_ULA_DEST_SHORT_ADDRESS, 0, sizeof(us_short_addr),(uint8_t*)&us_short_addr, &pSetConfirm); */
		AdpSetRequest(ADP_IB_MANUF_IPV6_ULA_DEST_SHORT_ADDRESS, 0, sizeof(us_short_addr), (uint8_t *)&us_short_addr);
		AdpDataRequest(us_length, puc_buffer, uc_nsdu_handle++, true, 0x00);
	} else {
		LOG_ERR(Log(stderr, "DEST IP ADDRESS not in BS database!"));
		return -2;
	}

	return 0;
}

void app_show_version( void )
{
	struct TAdpGetConfirm getConfirm;
	struct TAdpMacGetConfirm x_pib_confirm;

	LOG_INFO(Log("Show Versions:"));

	AdpGetRequestSync(ADP_IB_SOFT_VERSION, 0, &getConfirm);
	/* AdpGetRequest(ADP_IB_SOFT_VERSION, 0); */
	if (getConfirm.m_u8AttributeLength == 6) {
		LOG_INFO(Log("G3 STACK: %hhu.%hhu.%hhu Date: 20%hhu-%hhu-%hhu",
				getConfirm.m_au8AttributeValue[0],
				getConfirm.m_au8AttributeValue[1],
				getConfirm.m_au8AttributeValue[2],
				getConfirm.m_au8AttributeValue[3],
				getConfirm.m_au8AttributeValue[4],
				getConfirm.m_au8AttributeValue[5]));
	}

	AdpGetRequestSync(ADP_IB_MANUF_ADP_INTERNAL_VERSION, 0, &getConfirm);
	/* AdpGetRequest(ADP_IB_MANUF_ADP_INTERNAL_VERSION, 0); */
	if (getConfirm.m_u8AttributeLength == 6) {
		LOG_INFO(Log("ADP     : %hhu.%hhu.%hhu Date: 20%hhu-%hhu-%hhu",
				getConfirm.m_au8AttributeValue[0],
				getConfirm.m_au8AttributeValue[1],
				getConfirm.m_au8AttributeValue[2],
				getConfirm.m_au8AttributeValue[3],
				getConfirm.m_au8AttributeValue[4],
				getConfirm.m_au8AttributeValue[5]));
	}

	AdpGetRequestSync(MAC_WRP_PIB_MANUF_MAC_INTERNAL_VERSION, 0, &getConfirm);
	/* AdpGetRequest(MAC_WRP_PIB_MANUF_MAC_INTERNAL_VERSION, 0); */
	if (getConfirm.m_u8AttributeLength == 6) {
		LOG_INFO(Log("MAC     : %hhu.%hhu.%hhu Date: 20%hhu-%hhu-%hhu",
				getConfirm.m_au8AttributeValue[0],
				getConfirm.m_au8AttributeValue[1],
				getConfirm.m_au8AttributeValue[2],
				getConfirm.m_au8AttributeValue[3],
				getConfirm.m_au8AttributeValue[4],
				getConfirm.m_au8AttributeValue[5]));
	}

	AdpGetRequestSync(MAC_WRP_PIB_MANUF_MAC_RT_INTERNAL_VERSION, 0, &getConfirm);
	/* AdpGetRequest(MAC_WRP_PIB_MANUF_MAC_RT_INTERNAL_VERSION, 0); */
	if (getConfirm.m_u8AttributeLength == 6) {
		LOG_INFO(Log("MAC RT  : %hhu.%hhu.%hhu Date: 20%hhu-%hhu-%hhu",
				getConfirm.m_au8AttributeValue[0],
				getConfirm.m_au8AttributeValue[1],
				getConfirm.m_au8AttributeValue[2],
				getConfirm.m_au8AttributeValue[3],
				getConfirm.m_au8AttributeValue[4],
				getConfirm.m_au8AttributeValue[5]));
	}

	AdpMacGetRequestSync((uint32_t)MAC_WRP_PIB_MANUF_PHY_PARAM, MAC_WRP_PHY_PARAM_VERSION, &x_pib_confirm);
	/* AdpMacGetRequest((uint32_t)MAC_WRP_PIB_MANUF_PHY_PARAM, MAC_WRP_PHY_PARAM_VERSION); */
	if (x_pib_confirm.m_u8AttributeLength == 4) {
		LOG_INFO(Log("PHY     : %02x.%02x.%02x.%02x",
				x_pib_confirm.m_au8AttributeValue[3],
				x_pib_confirm.m_au8AttributeValue[2],
				x_pib_confirm.m_au8AttributeValue[1],
				x_pib_confirm.m_au8AttributeValue[0]));
	}

	return;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
