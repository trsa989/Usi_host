/**
 * \file
 *
 * \brief USI Host G3 MAC Wrapper API
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
/**********************************************************************************************************************/

/** \addtogroup G3
 * @{
 **********************************************************************************************************************/

/**********************************************************************************************************************/

/** Defines the MAC wrapper interface in order to allow user monitor and control calls between MAC and ADP. Any module
 *   should use functions exported by this interface and not direct calls to MAC
 ***********************************************************************************************************************
 *
 * @file
 *
 **********************************************************************************************************************/
#ifndef __MAC_WRAPPER_H__
#define __MAC_WRAPPER_H__

#include "mac_wrapper_defs.h"

/* #define ENABLE_MAC_WRAPPER_LOG */

struct TMacWrpNotifications;

/**********************************************************************************************************************/

/** Description of struct TMacWrpDataRequest
 ***********************************************************************************************************************
 * @param m_eSrcAddrMode Source address mode 0, 16, 64 bits
 * @param m_nDstPanId The 16-bit PAN identifier of the entity to which the MSDU is being transferred
 * @param m_DstAddr The individual device address of the entity to which the MSDU is being transferred.
 * @param m_u16MsduLength The number of octets contained in the MSDU to be transmitted by the MAC sublayer entity.
 * @param m_pMsdu The set of octets forming the MSDU to be transmitted by the MAC sublayer entity
 * @param m_u8MsduHandle The handle associated with the MSDU to be transmitted by the MAC sublayer entity.
 * @param m_u8TxOptions Indicate the transmission options for this MSDU:
 *    0: unacknowledged transmission, 1: acknowledged transmission
 * @param m_eSecurityLevel The security level to be used: 0x00 unecrypted; 0x05 encrypted
 * @param m_u8KeyIndex The index of the key to be used.
 * @param u8QualityOfService The QOS (quality of service) parameter of the MSDU to be transmitted by the MAC sublayer
 *      entity
 *        0x00 = normal priority
 *        0x01 = high priority
 **********************************************************************************************************************/
struct TMacWrpDataRequest {
	enum EMacWrpAddressMode m_eSrcAddrMode;
	TMacWrpPanId m_nDstPanId;
	struct TMacWrpAddress m_DstAddr;
	uint16_t m_u16MsduLength;
	const uint8_t *m_pMsdu;
	uint8_t m_u8MsduHandle;
	uint8_t m_u8TxOptions;
	enum EMacWrpSecurityLevel m_eSecurityLevel;
	uint8_t m_u8KeyIndex;
	enum EMacWrpQualityOfService m_eQualityOfService;
};

/**********************************************************************************************************************/

/** Description of struct TMacWrpDataConfirm
 ***********************************************************************************************************************
 * @param m_u8MsduHandle The handle associated with the MSDU being confirmed.
 * @param m_eStatus The status of the last MSDU transmission.
 * @param m_nTimestamp The time, in symbols, at which the data were transmitted.
 **********************************************************************************************************************/
struct TMacWrpDataConfirm {
	uint8_t m_u8MsduHandle;
	enum EMacWrpStatus m_eStatus;
	TMacWrpTimestamp m_nTimestamp;
};

/**********************************************************************************************************************/

/** Description of struct TMacWrpDataIndication
 ***********************************************************************************************************************
 * @param m_nSrcPanId The 16-bit PAN identifier of the device from which the frame was received.
 * @param m_SrcAddr The address of the device which sent the message.
 * @param m_nDstPanId The 16-bit PAN identifier of the entity to which the MSDU is being transferred.
 * @param m_DstAddr The individual device address of the entity to which the MSDU is being transferred.
 * @param m_u16MsduLength The number of octets contained in the MSDU to be indicated to the upper layer.
 * @param m_pMsdu The set of octets forming the MSDU received by the MAC sublayer entity.
 * @param m_u8MpduLinkQuality The (forward) LQI value measured during reception of the message.
 * @param m_u8Dsn The DSN of the received frame
 * @param m_nTimestamp The absolute time in milliseconds at which the frame was received and constructed, decrypted
 * @param m_eSecurityLevel The security level of the received message: 0x00 unecrypted; 0x05 encrypted
 * @param m_u8KeyIndex The index of the key used.
 * @param u8QualityOfService The QOS (quality of service) parameter of the MSDU received by the MAC sublayer entity
 *        0x00 = normal priority
 *        0x01 = high priority
 * @param m_u8RecvModulation Modulation of the received message.
 * @param m_u8RecvModulationScheme Modulation of the received message.
 * @param m_RecvToneMap The ToneMap of the received message.
 * @param m_u8ComputedModulation Computed modulation of the received message.
 * @param m_u8ComputedModulationScheme Computed modulation of the received message.
 * @param m_ComputedToneMap Compute ToneMap of the received message.
 **********************************************************************************************************************/
struct TMacWrpDataIndication {
	TMacWrpPanId m_nSrcPanId;
	struct TMacWrpAddress m_SrcAddr;
	TMacWrpPanId m_nDstPanId;
	struct TMacWrpAddress m_DstAddr;
	uint16_t m_u16MsduLength;
	uint8_t *m_pMsdu;
	uint8_t m_u8MpduLinkQuality;
	uint8_t m_u8Dsn;
	TMacWrpTimestamp m_nTimestamp;
	enum EMacWrpSecurityLevel m_eSecurityLevel;
	uint8_t m_u8KeyIndex;
	enum EMacWrpQualityOfService m_eQualityOfService;
	uint8_t m_u8RecvModulation;
	uint8_t m_u8RecvModulationScheme;
	struct TMacWrpToneMap m_RecvToneMap;
	uint8_t m_u8ComputedModulation;
	uint8_t m_u8ComputedModulationScheme;
	struct TMacWrpToneMap m_ComputedToneMap;
};

/**********************************************************************************************************************/

/** Description of struct TMacWrpSnifferIndication
 ***********************************************************************************************************************
 * @param m_u8FrameType The frame type.
 * @param m_nSrcPanId The 16-bit PAN identifier of the device from which the frame was received.
 * @param m_nSrcPanId The 16-bit PAN identifier of the device from which the frame was received.
 * @param m_SrcAddr The address of the device which sent the message.
 * @param m_nDstPanId The 16-bit PAN identifier of the entity to which the MSDU is being transferred.
 * @param m_DstAddr The individual device address of the entity to which the MSDU is being transferred.
 * @param m_u16MsduLength The number of octets contained in the MSDU to be indicated to the upper layer.
 * @param m_pMsdu The set of octets forming the MSDU received by the MAC sublayer entity.
 * @param m_u8MpduLinkQuality The (forward) LQI value measured during reception of the message.
 * @param m_u8Dsn The DSN of the received frame
 * @param m_nTimestamp The absolute time in milliseconds at which the frame was received and constructed, decrypted
 * @param m_eSecurityLevel The security level of the received message: 0x00 unecrypted; 0x05 encrypted
 * @param m_u8KeyIndex The index of the key used.
 * @param u8QualityOfService The QOS (quality of service) parameter of the MSDU received by the MAC sublayer entity
 *        0x00 = normal priority
 *        0x01 = high priority
 * @param m_u8RecvModulation Modulation of the received message.
 * @param m_u8RecvModulationScheme Modulation of the received message.
 * @param m_RecvToneMap The ToneMap of the received message.
 * @param m_u8ComputedModulation Computed modulation of the received message.
 * @param m_u8ComputedModulationScheme Computed modulation of the received message.
 * @param m_ComputedToneMap Compute ToneMap of the received message.
 **********************************************************************************************************************/
struct TMacWrpSnifferIndication {
	uint8_t m_u8FrameType;
	TMacWrpPanId m_nSrcPanId;
	struct TMacWrpAddress m_SrcAddr;
	TMacWrpPanId m_nDstPanId;
	struct TMacWrpAddress m_DstAddr;
	uint16_t m_u16MsduLength;
	uint8_t *m_pMsdu;
	uint8_t m_u8MpduLinkQuality;
	uint8_t m_u8Dsn;
	TMacWrpTimestamp m_nTimestamp;
	enum EMacWrpSecurityLevel m_eSecurityLevel;
	uint8_t m_u8KeyIndex;
	enum EMacWrpQualityOfService m_eQualityOfService;
	uint8_t m_u8RecvModulation;
	uint8_t m_u8RecvModulationScheme;
	struct TMacWrpToneMap m_RecvToneMap;
	uint8_t m_u8ComputedModulation;
	uint8_t m_u8ComputedModulationScheme;
	struct TMacWrpToneMap m_ComputedToneMap;
};

/**********************************************************************************************************************/

/** Description of struct TMacWrpBeaconNotifyIndication
 ***********************************************************************************************************************
 * @param m_PanDescriptor The PAN descriptor
 **********************************************************************************************************************/
struct TMacWrpBeaconNotifyIndication {
	struct TMacWrpPanDescriptor m_PanDescriptor;
};

/**********************************************************************************************************************/

/** Description of struct TMacWrpGetRequest
 ***********************************************************************************************************************
 * @param m_ePibAttribute The attribute id.
 * @param m_u16PibAttributeIndex The index of the element in the table.
 **********************************************************************************************************************/
struct TMacWrpGetRequest {
	enum EMacWrpPibAttribute m_ePibAttribute;
	uint16_t m_u16PibAttributeIndex;
};

/**********************************************************************************************************************/

/** Description of struct TMacWrpGetConfirm
 ***********************************************************************************************************************
 * @param m_eStatus The status of the request.
 * @param m_ePibAttribute The attribute id.
 * @param m_u16PibAttributeIndex The index of the element in the table.
 * @param m_PibAttributeValue The value of the attribute.
 **********************************************************************************************************************/
struct TMacWrpGetConfirm {
	enum EMacWrpStatus m_eStatus;
	enum EMacWrpPibAttribute m_ePibAttribute;
	uint16_t m_u16PibAttributeIndex;
	struct TMacWrpPibValue m_PibAttributeValue;
};

/**********************************************************************************************************************/

/** Description of struct TMacWrpResetRequest
 ***********************************************************************************************************************
 * @param m_bSetDefaultPib True to reset the PIB to the default values, false otherwise
 **********************************************************************************************************************/
struct TMacWrpResetRequest {
	bool m_bSetDefaultPib;
};

/**********************************************************************************************************************/

/** Description of struct TMacWrpResetConfirm
 ***********************************************************************************************************************
 * @param m_eStatus The status of the request.
 **********************************************************************************************************************/
struct TMacWrpResetConfirm {
	enum EMacWrpStatus m_eStatus;
};

/**********************************************************************************************************************/

/** Description of struct TMacWrpScanRequest
 ***********************************************************************************************************************
 * @param m_u16ScanDuration Duration of the scan in seconds
 **********************************************************************************************************************/
struct TMacWrpScanRequest {
	uint16_t m_u16ScanDuration;
};

/**********************************************************************************************************************/

/** Description of struct TMacWrpScanConfirm
 ***********************************************************************************************************************
 * @param m_eStatus The status of the request.
 **********************************************************************************************************************/
struct TMacWrpScanConfirm {
	enum EMacWrpStatus m_eStatus;
};

/**********************************************************************************************************************/

/** Description of struct TMacWrpCommStatusIndication
 ***********************************************************************************************************************
 * @param m_nPanId The PAN identifier of the device from which the frame was received or to which the frame was being
 *    sent.
 * @param m_SrcAddr The individual device address of the entity from which the frame causing the error originated.
 * @param m_DstAddr The individual device address of the device for which the frame was intended.
 * @param m_eStatus The communications status.
 * @param m_eSecurityLevel The security level purportedly used by the received frame.
 * @param m_u8KeyIndex The index of the key purportedly used by the originator of the received frame.
 **********************************************************************************************************************/
struct TMacWrpCommStatusIndication {
	TMacWrpPanId m_nPanId;
	struct TMacWrpAddress m_SrcAddr;
	struct TMacWrpAddress m_DstAddr;
	enum EMacWrpStatus m_eStatus;
	enum EMacWrpSecurityLevel m_eSecurityLevel;
	uint8_t m_u8KeyIndex;
};

/**********************************************************************************************************************/

/** Description of struct TMacWrpGetRequest
 ***********************************************************************************************************************
 * @param m_ePibAttribute The attribute id.
 * @param m_u16PibAttributeIndex The index of the element in the table.
 * @param m_PibAttributeValue The value of the attribute.
 **********************************************************************************************************************/
struct TMacWrpSetRequest {
	enum EMacWrpPibAttribute m_ePibAttribute;
	uint16_t m_u16PibAttributeIndex;
	struct TMacWrpPibValue m_PibAttributeValue;
};

/**********************************************************************************************************************/

/** Description of struct TMacWrpSetConfirm
 ***********************************************************************************************************************
 * @param m_eStatus The status of the request.
 * @param m_ePibAttribute The attribute id.
 * @param m_u16PibAttributeIndex The index of the element in the table.
 **********************************************************************************************************************/
struct TMacWrpSetConfirm {
	enum EMacWrpStatus m_eStatus;
	enum EMacWrpPibAttribute m_ePibAttribute;
	uint16_t m_u16PibAttributeIndex;
};

/**********************************************************************************************************************/

/** Description of struct TMacWrpStartRequest
 ***********************************************************************************************************************
 * @param m_nPanId The pan id.
 **********************************************************************************************************************/
struct TMacWrpStartRequest {
	TMacWrpPanId m_nPanId;
};

/**********************************************************************************************************************/

/** Description of struct TMacWrpStartConfirm
 ***********************************************************************************************************************
 * @param m_eStatus The status of the request.
 **********************************************************************************************************************/
struct TMacWrpStartConfirm {
	enum EMacWrpStatus m_eStatus;
};

/**********************************************************************************************************************/

/** ADP use this function to initialize the MAC layer. The MAC layer should be initialized before doing any other operation.
 * @param pNotifications Structure with callbacks used by the MAC layer to notify the upper layer about specific events
 * @param band Working band (should be inline with the hardware)
 **********************************************************************************************************************/
void MacWrapperInitialize(struct TMacWrpNotifications *pNotifications, uint8_t u8Band);

/**********************************************************************************************************************/

/** This function must be called periodically in order to allow the G3 stack to run and execute its internal tasks.
 **********************************************************************************************************************/
void MacWrapperEventHandler(void);

/**********************************************************************************************************************/

/** The MacWrapperMcpsDataRequest primitive requests the transfer of an application PDU to another device or multiple devices.
 * Parameters from struct TMcpsDataRequest
 ***********************************************************************************************************************
 * @param pParameters Request parameters
 **********************************************************************************************************************/
void MacWrapperMcpsDataRequest(struct TMacWrpDataRequest *pParameters);

/**********************************************************************************************************************/

/** The MacWrapperMlmeGetRequest primitive allows the upper layer to get the value of an attribute from the MAC information base.
 ***********************************************************************************************************************
 * @param pParameters Parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeGetRequest(struct TMacWrpGetRequest *pParameters);

/**********************************************************************************************************************/

/** The MacWrapperMlmeGetRequestSync primitive allows the upper layer to get the value of an attribute from the MAC information
 * base in synchronous way.
 ***********************************************************************************************************************
 * @param eAttribute IB identifier
 * @param u16Index IB index
 * @param pValue pointer to results
 **********************************************************************************************************************/
enum EMacWrpStatus MacWrapperMlmeGetRequestSync(enum EMacWrpPibAttribute eAttribute, uint16_t u16Index, struct TMacWrpPibValue *pValue);

/**********************************************************************************************************************/

/** The MacWrapperMlmeSetRequest primitive allows the upper layer to set the value of an attribute in the MAC information base.
 ***********************************************************************************************************************
 * @param pParameters Parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeSetRequest(struct TMacWrpSetRequest *pParameters);

/**********************************************************************************************************************/

/** The MacWrapperMlmeSetRequestSync primitive allows the upper layer to set the value of an attribute in the MAC information
 * base in synchronous way.
 ***********************************************************************************************************************
 * @param eAttribute IB identifier
 * @param u16Index IB index
 * @param pValue pointer to IB new values
 **********************************************************************************************************************/
enum EMacWrpStatus MacWrapperMlmeSetRequestSync(enum EMacWrpPibAttribute eAttribute, uint16_t u16Index, const struct TMacWrpPibValue *pValue);

/**********************************************************************************************************************/

/** The MacWrapperMlmeResetRequest primitive performs a reset of the mac sublayer and allows the resetting of the MIB
 * attributes.
 ***********************************************************************************************************************
 * @param pParameters Parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeResetRequest(struct TMacWrpResetRequest *pParameters);

/**********************************************************************************************************************/

/** The MacWrapperMlmeScanRequest primitive allows the upper layer to scan for networks operating in its POS.
 ***********************************************************************************************************************
 * @param pParameters Parameters of the request.
 **********************************************************************************************************************/
void MacWrapperMlmeScanRequest(struct TMacWrpScanRequest *pParameters);

/**********************************************************************************************************************/

/** The MacWrapperMlmeStartRequest primitive allows the upper layer to request the starting of a new network.
 ***********************************************************************************************************************
 * @param pParameters The parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeStartRequest(struct TMacWrpStartRequest *pParameters);

/**********************************************************************************************************************/

/** The MacWrapperGetNeighbourTableSize primitive gets the size of Mac Neighbour Table.
 **********************************************************************************************************************/
uint16_t MacWrapperGetNeighbourTableSize(void);

/**********************************************************************************************************************/

/** The MacWrpDataConfirm primitive allows the upper layer to be notified of the completion of a MacMcpsDataRequest.
 ***********************************************************************************************************************
 * @param pParameters Parameters of the confirm
 **********************************************************************************************************************/
typedef void (*MacWrpDataConfirm)(struct TMacWrpDataConfirm *pParameters);

/**********************************************************************************************************************/

/** The MacWrpDataIndication primitive is used to transfer received data from the mac sublayer to the upper layer.
 ***********************************************************************************************************************
 * @param pParameters Parameters of the data indication.
 **********************************************************************************************************************/
typedef void (*MacWrpDataIndication)(struct TMacWrpDataIndication *pParameters);

/**********************************************************************************************************************/

/** The MacWrpSnifferIndication primitive is used to transfer received data from the mac sublayer to the upper layer.
 ***********************************************************************************************************************
 * @param pParameters Parameters of the data indication.
 **********************************************************************************************************************/
typedef void (*MacWrpSnifferIndication)(struct TMacWrpSnifferIndication *pParameters);

/**********************************************************************************************************************/

/** The MacWrpGetConfirm primitive allows the upper layer to be notified of the completion of a MacWrpGetRequest.
 ***********************************************************************************************************************
 * @param pParameters Parameters of the confirm.
 **********************************************************************************************************************/
typedef void (*MacWrpGetConfirm)(struct TMacWrpGetConfirm *pParameters);

/**********************************************************************************************************************/

/** The MacWrpSetConfirm primitive allows the upper layer to be notified of the completion of a MacWrpSetRequest.
 ***********************************************************************************************************************
 * @param pParameters Parameters of the confirm.
 **********************************************************************************************************************/
typedef void (*MacWrpSetConfirm)(struct TMacWrpSetConfirm *pParameters);

/**********************************************************************************************************************/

/** The MacWrpResetConfirm primitive allows upper layer to be notified of the completion of a MacWrpResetRequest.
 ***********************************************************************************************************************
 * @param m_u8Status The status of the request.
 **********************************************************************************************************************/
typedef void (*MacWrpResetConfirm)(struct TMacWrpResetConfirm *pParameters);

/**********************************************************************************************************************/

/** The MacWrpBeaconNotify primitive is generated by the MAC layer to notify the application about the discovery
 * of a new PAN coordinator or LBA
 ***********************************************************************************************************************
 * @param pPanDescriptor PAN descriptor contains information about the PAN
 **********************************************************************************************************************/
typedef void (*MacWrpBeaconNotify)(struct TMacWrpBeaconNotifyIndication *pParameters);

/**********************************************************************************************************************/

/** The MacWrpScanConfirm primitive allows the upper layer to be notified of the completion of a MacWrpScanRequest.
 ***********************************************************************************************************************
 * @param pParameters The parameters of the confirm.
 **********************************************************************************************************************/
typedef void (*MacWrpScanConfirm)(struct TMacWrpScanConfirm *pParameters);

/**********************************************************************************************************************/

/** The MacWrpStartConfirm primitive allows the upper layer to be notified of the completion of a TMacWrpStartRequest.
 ***********************************************************************************************************************
 * @param m_u8Status The status of the request.
 **********************************************************************************************************************/
typedef void (*MacWrpStartConfirm)(struct TMacWrpStartConfirm *pParameters);

/**********************************************************************************************************************/

/** The MacWrpCommStatusIndication primitive allows the MAC layer to notify the next higher layer when a particular
 * event occurs on the PAN.
 ***********************************************************************************************************************
 * @param pParameters The parameters of the indication
 **********************************************************************************************************************/
typedef void (*MacWrpCommStatusIndication)(struct TMacWrpCommStatusIndication *pParameters);

struct TMacWrpNotifications {
	MacWrpDataConfirm m_MacWrpDataConfirm;
	MacWrpDataIndication m_MacWrpDataIndication;
	MacWrpGetConfirm m_MacWrpGetConfirm;
	MacWrpSetConfirm m_MacWrpSetConfirm;
	MacWrpResetConfirm m_MacWrpResetConfirm;
	MacWrpBeaconNotify m_MacWrpBeaconNotifyIndication;
	MacWrpScanConfirm m_MacWrpScanConfirm;
	MacWrpStartConfirm m_MacWrpStartConfirm;
	MacWrpCommStatusIndication m_MacWrpCommStatusIndication;
	MacWrpSnifferIndication m_MacWrpSnifferIndication;
};

#ifdef G3_HYBRID_PROFILE

/**********************************************************************************************************************/
/** HyAL uses this function to initialize the RF MAC layer. The MAC layer should be initialized before doing any other operation.
 * @param pNotifications Structure with callbacks used by the MAC layer to notify the upper layer about specific events
 **********************************************************************************************************************/
void MacWrapperInitializeRF(struct TMacWrpNotifications *pNotifications);

/**********************************************************************************************************************/
/** This function must be called priodically in order to allow the G3 stack to run and execute its internal tasks.
 **********************************************************************************************************************/
void MacWrapperEventHandlerRF(void);

/**********************************************************************************************************************/
/** The MacWrapperMcpsDataRequestRF primitive requests the transfer of an application PDU to another device or multiple devices.
 * Parameters from struct TMcpsDataRequest
 ***********************************************************************************************************************
 * @param pParameters Request parameters
 **********************************************************************************************************************/
void MacWrapperMcpsDataRequestRF(struct TMacWrpDataRequest *pParameters);

/**********************************************************************************************************************/
/** The MacWrapperMlmeGetRequestRF primitive allows the upper layer to get the value of an attribute from the MAC information base.
 ***********************************************************************************************************************
 * @param pParameters Parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeGetRequestRF(struct TMacWrpGetRequest *pParameters);

/**********************************************************************************************************************/
/** The MacWrapperMlmeGetRequestSyncRF primitive allows the upper layer to get the value of an attribute from the MAC information
 * base in synchronous way.
 ***********************************************************************************************************************
 * @param eAttribute IB identifier
 * @param u16Index IB index
 * @param pValue pointer to results
 **********************************************************************************************************************/
enum EMacWrpStatus MacWrapperMlmeGetRequestSyncRF(enum EMacWrpPibAttribute eAttribute, uint16_t u16Index, struct TMacWrpPibValue *pValue);

/**********************************************************************************************************************/
/** The MacWrapperMlmeSetRequestRF primitive allows the upper layer to set the value of an attribute in the MAC information base.
 ***********************************************************************************************************************
 * @param pParameters Parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeSetRequestRF(struct TMacWrpSetRequest *pParameters);

/**********************************************************************************************************************/
/** The MacWrapperMlmeSetRequestSyncRF primitive allows the upper layer to set the value of an attribute in the MAC information
 * base in synchronous way.
 ***********************************************************************************************************************
 * @param eAttribute IB identifier
 * @param u16Index IB index
 * @param pValue pointer to IB new values
 **********************************************************************************************************************/
enum EMacWrpStatus MacWrapperMlmeSetRequestSyncRF(enum EMacWrpPibAttribute eAttribute, uint16_t u16Index, const struct TMacWrpPibValue *pValue);

/**********************************************************************************************************************/
/** The MacWrapperMlmeResetRequestRF primitive performs a reset of the mac sublayer and allows the resetting of the MIB
 * attributes.
 ***********************************************************************************************************************
 * @param pParameters Parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeResetRequestRF(struct TMacWrpResetRequest *pParameters);

/**********************************************************************************************************************/
/** The MacWrapperMlmeScanRequestRF primitive allows the upper layer to scan for networks operating in its POS.
 ***********************************************************************************************************************
 * @param pParameters Parameters of the request.
 **********************************************************************************************************************/
void MacWrapperMlmeScanRequestRF(struct TMacWrpScanRequest *pParameters);

/**********************************************************************************************************************/
/** The MacWrapperMlmeStartRequestRF primitive allows the upper layer to request the starting of a new network.
 ***********************************************************************************************************************
 * @param pParameters The parameters of the request
 **********************************************************************************************************************/
void MacWrapperMlmeStartRequestRF(struct TMacWrpStartRequest *pParameters);

#endif /* G3_HYBRID_PROFILE */

#endif

/**********************************************************************************************************************/

/** @}
 **********************************************************************************************************************/
