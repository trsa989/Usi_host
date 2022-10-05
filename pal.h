/**
 * \file
 *
 * \brief USI Host G3 PAL API
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

/** Defines the PHY unified interface to G3 stack. Any module should use functions exported by this interface and not
 * direct calls
 ***********************************************************************************************************************
 *
 * @file
 *
 **********************************************************************************************************************/

#ifndef __PHY_SUBLAYER_H__
#define __PHY_SUBLAYER_H__

enum EPhyStatus {
	PHY_STATUS_BUSY = 0x00,
	PHY_STATUS_BUSY_RX = 0x01,
	PHY_STATUS_BUSY_TX = 0x02,
	PHY_STATUS_FORCE_TRX_OFF = 0x03,
	PHY_STATUS_IDLE = 0x04,
	PHY_STATUS_INVALID_PARAMETER = 0x05,
	PHY_STATUS_RX_ON = 0x06,
	PHY_STATUS_SUCCESS = 0x07,
	PHY_STATUS_TRX_OFF = 0x08,
	PHY_STATUS_TX_ON = 0x09,
	PHY_STATUS_UNSUPPORTED_ATTRIBUTE = 0x0A,
	PHY_STATUS_READ_ONLY = 0x0B,
};

enum EPhyModulationType {
	PHY_MODULATION_ROBUST = 0x00,
	PHY_MODULATION_DBPSK_BPSK = 0x01,
	PHY_MODULATION_DQPSK_QPSK = 0x02,
	PHY_MODULATION_D8PSK_8PSK = 0x03,
	PHY_MODULATION_16_QAM = 0x04,
};

enum EPhyModulationScheme {
	PHY_MODULATION_SCHEME_DIFFERENTIAL = 0x00,
	PHY_MODULATION_SCHEME_COHERENT = 0x01,
};

enum EPhyDelimiterType {
	PHY_DELIMITER_NO_RESPONSE_EXPECTED = 0x00,
	PHY_DELIMITER_RESPONSE_EXPECTED = 0x01,
	PHY_DELIMITER_ACK = 0x02,
	PHY_DELIMITER_NACK = 0x03,
};

enum EPhyGetSetResult {
	PHY_GETSET_RESULT_OK = 0x00,
	PHY_GETSET_RESULT_INVALID_PARAM = 0x01,
	PHY_GETSET_RESULT_READ_ONLY = 0x02
};

#define PHY_MAX_TONES 72
#define MAC_WRP_MAX_TONE_GROUPS 24

struct TPhyToneMap {
	uint8_t m_au8Tm[(MAC_WRP_MAX_TONE_GROUPS + 7) / 8];
};

struct TPhyToneMask {
	uint8_t m_au8ToneMask[(PHY_MAX_TONES + 7) / 8];
};

/* Pre emphasis, 4 bits per value. */
struct TPhyPreEmphasis {
	uint8_t m_au8PreEmphasis[MAC_WRP_MAX_TONE_GROUPS];
};

struct TPhyAckFch {
	uint16_t m_u16Fcs;
	uint8_t m_u8Ssca;
	enum EPhyDelimiterType m_eDelimiterType;
};

struct TPhyToneEntry {
	uint8_t m_u8ToneMap : 1;
	uint8_t m_u8PreEmphasis : 4;
	uint8_t m_u8ToneMask : 1;
};

enum EPhyTrxState {
	PHY_TRX_STATE_TXON_RXOFF = 0x01,
	PHY_TRX_STATE_TXOFF_RXON = 0x02,
};

enum EPhyCsStatus {
	PHY_CS_STATUS_IDLE = 0x00,
	PHY_CS_STATUS_BUSY = 0x01,
};

struct TPhyTxParameters {
	uint8_t m_u8TxPower;
	enum EPhyModulationType m_eModulationType;
	enum EPhyModulationScheme m_eModulationScheme;
	struct TPhyToneMap m_ToneMap;
	struct TPhyPreEmphasis m_PreEmphasis;
	struct TPhyToneMask m_ToneMask;
	uint8_t m_u8TwoRSBlocks;
};

struct TPdDataRequest {
	uint16_t m_u16PsduLength;
	uint8_t *m_pPsdu;
	bool m_bDelayed;
	uint32_t m_u32Time;
	bool m_bPerformCs;
};

struct TPdDataConfirm {
	enum EPhyStatus m_eStatus;
	uint32_t m_u32Time;
};

struct TPdDataIndication {
	uint16_t m_u16PsduLength;
	uint8_t *m_pPsdu;
	uint8_t m_u8Dt;
	uint32_t m_u32Time;
};

struct TPdAckRequest {
	struct TPhyAckFch m_AckFch;
	bool m_bDelayed;
	uint32_t m_u32Time;
};

struct TPdAckConfirm {
	enum EPhyStatus m_eStatus;
	uint32_t m_u32Time;
};

struct TPdAckIndication {
	struct TPhyAckFch m_AckFch;
	uint32_t m_u32Time;
};

struct TPlmeSetRequest {
	struct TPhyTxParameters m_TxParameters;
	uint8_t m_u8Dt;
};

struct TPlmeSetConfirm {
	struct TPhyTxParameters m_TxParameters;
	uint8_t m_u8Dt;
};

struct TPlmeGetRequest {
	uint8_t m_u8Dummy;
};

struct TPlmeGetConfirm {
	uint8_t m_u8PpduLinkQuality;
	uint8_t m_u8CarrierSnr[PHY_MAX_TONES];
	uint8_t m_u8PhaseDifferential;
	enum EPhyModulationType m_eModulationType;
	enum EPhyModulationScheme m_eModulationScheme;
	struct TPhyToneMap m_ToneMap;
	uint8_t m_u8Dt;
};

struct TPlmeSetTrxStateRequest {
	enum EPhyTrxState m_eState;
};

struct TPlmeSetTrxStateConfirm {
	enum EPhyStatus m_eStatus;
};

struct TPlmeCsRequest {
	uint8_t m_u8Dummy;
};

struct TPlmeCsConfirm {
	enum EPhyCsStatus m_eStatus;
};

struct TPlmeGetToneMapResponseData {
	enum EPhyModulationType m_eModulationType;
	enum EPhyModulationScheme m_eModulationScheme;
	struct TPhyToneMap m_ToneMap;
};

struct TPhyNotifications;

void PhyInitialize(struct TPhyNotifications *pNotifications, uint8_t u8Band);
void PhyEventHandler(void);

void PhyPdDataRequest(struct TPdDataRequest *pParameters);

typedef void (*PhyPdDataConfirm)(struct TPdDataConfirm *pParameters);
typedef void (*PhyPdDataIndication)(struct TPdDataIndication *pParameters);

void PhyPdAckRequest(struct TPdAckRequest *pParameters);

typedef void (*PhyPdAckConfirm)(struct TPdAckConfirm *pParameters);
typedef void (*PhyPdAckIndication)(struct TPdAckIndication *pParameters);

void PhyPlmeSetRequest(struct TPlmeSetRequest *pParameters);

typedef void (*PhyPlmeSetConfirm)(struct TPlmeSetConfirm *pParameters);

void PhyPlmeGetRequest(struct TPlmeGetRequest *pParameters);

typedef void (*PhyPlmeGetConfirm)(struct TPlmeGetConfirm *pParameters);

void PhyPlmeSetTrxStateRequest(struct TPlmeSetTrxStateRequest *pParameters);

typedef void (*PhyPlmeSetTrxStateConfirm)(struct TPlmeSetTrxStateConfirm *pParameters);

void PhyPlmeCsRequest(struct TPlmeCsRequest *pParameters);

typedef void (*PhyPlmeCsConfirm)(struct TPlmeCsConfirm *pParameters);

void PhyPlmeResetRequest(void);

uint32_t PhyGetTime(void);

void PhyGetToneMapResponseData(struct TPlmeGetToneMapResponseData *pParameters);

enum EPhyGetSetResult PhyGetParam(uint16_t us_id, void *p_val, uint16_t us_len);
enum EPhyGetSetResult PhySetParam(uint16_t us_id, void *p_val, uint16_t us_len);

void PhySetToneMask(uint8_t *puc_tone_mask);

uint8_t PhyGetLegacyMode(void);

uint8_t PhyGetPIBLen(uint16_t us_id);

struct TPhyNotifications {
	PhyPdDataConfirm m_pPdDataConfirm;
	PhyPdDataIndication m_pPdDataIndication;
	PhyPdAckConfirm m_pPdAckConfirm;
	PhyPdAckIndication m_pPdAckIndication;
	PhyPlmeSetConfirm m_pPlmeSetConfirm;
	PhyPlmeGetConfirm m_pPlmeGetConfirm;
	PhyPlmeSetTrxStateConfirm m_pPlmeSetTrxStateConfirm;
	PhyPlmeCsConfirm m_pPlmeCsConfirm;
};

#endif

/**********************************************************************************************************************/

/** @}
 **********************************************************************************************************************/
