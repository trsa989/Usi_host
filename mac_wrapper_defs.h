/**
 * \file
 *
 * \brief USI Host G3 MAC Wrapper Definitions
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
/**********************************************************************************************************************/
 /*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

/** \addtogroup MacSublayer
 * @{
 **********************************************************************************************************************/

/**********************************************************************************************************************/

/** This file contains data types and functions of the MAC WRAPPER API.
 ***********************************************************************************************************************
 *
 * @file
 *
 **********************************************************************************************************************/

#ifndef MAC_WRAPPER_DEFS_H_
#define MAC_WRAPPER_DEFS_H_

#if defined (__CC_ARM)
#pragma anon_unions
#endif

#include <stdbool.h>

#define MAC_WRP_BAND_CENELEC_A 0
#define MAC_WRP_BAND_CENELEC_B 1
#define MAC_WRP_BAND_FCC 2
#define MAC_WRP_BAND_ARIB 3

#define MAC_WRP_MAX_TONES 72
#define MAC_WRP_MAX_TONE_GROUPS 24

struct TMacWrpToneMap {
	uint8_t m_au8Tm[(MAC_WRP_MAX_TONE_GROUPS + 7) / 8];
};

struct TMacWrpToneMask {
	uint8_t m_au8ToneMask[(MAC_WRP_MAX_TONES + 7) / 8];
};

enum EMacWrpModulationType {
	MAC_WRP_MODULATION_ROBUST = 0x00,
	MAC_WRP_MODULATION_DBPSK_BPSK = 0x01,
	MAC_WRP_MODULATION_DQPSK_QPSK = 0x02,
	MAC_WRP_MODULATION_D8PSK_8PSK = 0x03,
	MAC_WRP_MODULATION_16_QAM = 0x04,
};

enum EMacWrpModulationScheme {
	MAC_WRP_MODULATION_SCHEME_DIFFERENTIAL = 0x00,
	MAC_WRP_MODULATION_SCHEME_COHERENT = 0x01,
};

struct TMacWrpToneMapResponseData {
	enum EMacWrpModulationType m_eModulationType;
	enum EMacWrpModulationScheme m_eModulationScheme;
	struct TMacWrpToneMap m_ToneMap;
};

typedef uint16_t TMacWrpPanId;

typedef uint16_t TMacWrpShortAddress;

/* The extended address has to be little endian. */
struct TMacWrpExtendedAddress {
	uint8_t m_au8Address[8];
};

enum EMacWrpAddressMode {
	MAC_WRP_ADDRESS_MODE_NO_ADDRESS = 0x00,
	MAC_WRP_ADDRESS_MODE_SHORT = 0x02,
	MAC_WRP_ADDRESS_MODE_EXTENDED = 0x03,
};

#ifndef LINUX
struct TMacWrpAddress {
	enum EMacWrpAddressMode m_eAddrMode;
	union {
		TMacWrpShortAddress m_nShortAddress;
		struct TMacWrpExtendedAddress m_ExtendedAddress;
	};
};
#else
struct TMacWrpAddress {
	uint8_t m_eAddrMode;
	uint8_t dummy;
	union {
		TMacWrpShortAddress m_nShortAddress;
		struct TMacWrpExtendedAddress m_ExtendedAddress;
	};
};
#endif

struct TMacWrpFc {
	uint16_t m_nFrameType : 3;
	uint16_t m_nSecurityEnabled : 1;
	uint16_t m_nFramePending : 1;
	uint16_t m_nAckRequest : 1;
	uint16_t m_nPanIdCompression : 1;
	uint16_t m_nReserved : 3;
	uint16_t m_nDestAddressingMode : 2;
	uint16_t m_nFrameVersion : 2;
	uint16_t m_nSrcAddressingMode : 2;
};

struct TMacWrpSegmentControl {
	uint8_t m_nRes : 4;
	uint8_t m_nTmr : 1;
	uint8_t m_nCc : 1;
	uint8_t m_nCap : 1;
	uint8_t m_nLsf : 1;
	uint16_t m_nSc : 6;
	uint16_t m_nSl : 10;
};

struct TMacWrpAuxiliarySecurityHeader {
	uint8_t m_nSecurityLevel : 3;
	uint8_t m_nKeyIdentifierMode : 2;
	uint8_t m_nReserved : 3;
	uint32_t m_u32FrameCounter;
	uint8_t m_u8KeyIdentifier;
};

struct TMacWrpMhr {
	struct TMacWrpSegmentControl m_SegmentControl;
	struct TMacWrpFc m_Fc;
	uint8_t m_u8SequenceNumber;
	TMacWrpPanId m_nDestinationPanIdentifier;
	struct TMacWrpAddress m_DestinationAddress;
	TMacWrpPanId m_nSourcePanIdentifier;
	struct TMacWrpAddress m_SourceAddress;
	struct TMacWrpAuxiliarySecurityHeader m_SecurityHeader;
};

struct TMacWrpFrame {
	struct TMacWrpMhr m_Header;
	uint16_t m_u16PayloadLength;
	uint8_t *m_pu8Payload;
	uint8_t m_u8PadLength;
	uint16_t m_u16Fcs;
};

typedef uint32_t TMacWrpTimestamp;

struct TMacWrpPanDescriptor {
	TMacWrpPanId m_nPanId;
	uint8_t m_u8LinkQuality;
	TMacWrpShortAddress m_nLbaAddress;
	uint16_t m_u16RcCoord;
};

enum EMacWrpSecurityLevel {
	MAC_WRP_SECURITY_LEVEL_NONE = 0x00,
	MAC_WRP_SECURITY_LEVEL_ENC_MIC_32 = 0x05,
};

enum EMacWrpQualityOfService {
	MAC_WRP_QUALITY_OF_SERVICE_NORMAL_PRIORITY = 0x00,
	MAC_WRP_QUALITY_OF_SERVICE_HIGH_PRIORITY = 0x01,
};

enum EMacWrpTxOptions {
	MAC_WRP_TX_OPTION_ACK = 0x01,
};

enum EMacWrpStatus {
	MAC_WRP_STATUS_SUCCESS = 0x00,
	MAC_WRP_STATUS_BEACON_LOSS = 0xE0,
	MAC_WRP_STATUS_CHANNEL_ACCESS_FAILURE = 0xE1,
	MAC_WRP_STATUS_COUNTER_ERROR = 0xDB,
	MAC_WRP_STATUS_DENIED = 0xE2,
	MAC_WRP_STATUS_DISABLE_TRX_FAILURE = 0xE3,
	MAC_WRP_STATUS_FRAME_TOO_LONG = 0xE5,
	MAC_WRP_STATUS_IMPROPER_KEY_TYPE = 0xDC,
	MAC_WRP_STATUS_IMPROPER_SECURITY_LEVEL = 0xDD,
	MAC_WRP_STATUS_INVALID_ADDRESS = 0xF5,
	MAC_WRP_STATUS_INVALID_GTS = 0xE6,
	MAC_WRP_STATUS_INVALID_HANDLE = 0xE7,
	MAC_WRP_STATUS_INVALID_INDEX = 0xF9,
	MAC_WRP_STATUS_INVALID_PARAMETER = 0xE8,
	MAC_WRP_STATUS_LIMIT_REACHED = 0xFA,
	MAC_WRP_STATUS_NO_ACK = 0xE9,
	MAC_WRP_STATUS_NO_BEACON = 0xEA,
	MAC_WRP_STATUS_NO_DATA = 0xEB,
	MAC_WRP_STATUS_NO_SHORT_ADDRESS = 0xEC,
	MAC_WRP_STATUS_ON_TIME_TOO_LONG = 0xF6,
	MAC_WRP_STATUS_OUT_OF_CAP = 0xED,
	MAC_WRP_STATUS_PAN_ID_CONFLICT = 0xEE,
	MAC_WRP_STATUS_PAST_TIME = 0xF7,
	MAC_WRP_STATUS_READ_ONLY = 0xFB,
	MAC_WRP_STATUS_REALIGNMENT = 0xEF,
	MAC_WRP_STATUS_SCAN_IN_PROGRESS = 0xFC,
	MAC_WRP_STATUS_SECURITY_ERROR = 0xE4,
	MAC_WRP_STATUS_SUPERFRAME_OVERLAP = 0xFD,
	MAC_WRP_STATUS_TRACKING_OFF = 0xF8,
	MAC_WRP_STATUS_TRANSACTION_EXPIRED = 0xF0,
	MAC_WRP_STATUS_TRANSACTION_OVERFLOW = 0xF1,
	MAC_WRP_STATUS_TX_ACTIVE = 0xF2,
	MAC_WRP_STATUS_UNAVAILABLE_KEY = 0xF3,
	MAC_WRP_STATUS_UNSUPPORTED_ATTRIBUTE = 0xF4,
	MAC_WRP_STATUS_UNSUPPORTED_LEGACY = 0xDE,
	MAC_WRP_STATUS_UNSUPPORTED_SECURITY = 0xDF,
	MAC_WRP_STATUS_ALTERNATE_PANID_DETECTION = 0x80,
	MAC_WRP_STATUS_QUEUE_FULL = 0xD0,
};

#define MAC_WRP_PAN_ID_BROADCAST (0xFFFFu)
#define MAC_WRP_SHORT_ADDRESS_BROADCAST (0xFFFFu)
#define MAC_WRP_SHORT_ADDRESS_UNDEFINED (0xFFFFu)

#define MAC_WRP_SECURITY_KEY_LENGTH (16)

struct TMacWrpSecurityKey {
	bool m_bValid;
	uint8_t m_au8Key[MAC_WRP_SECURITY_KEY_LENGTH];
};

enum EMacWrpPibAttribute {
	MAC_WRP_PIB_ACK_WAIT_DURATION = 0x00000040, /* 16 bits */
	MAC_WRP_PIB_MAX_BE = 0x00000047, /* 8 bits */
	MAC_WRP_PIB_BSN = 0x00000049, /* 8 bits */
	MAC_WRP_PIB_DSN = 0x0000004C, /* 8 bits */
	MAC_WRP_PIB_MAX_CSMA_BACKOFFS = 0x0000004E, /* 8 bits */
	MAC_WRP_PIB_MIN_BE = 0x0000004F, /* 8 bits */
	MAC_WRP_PIB_PAN_ID = 0x00000050, /* 16 bits */
	MAC_WRP_PIB_PROMISCUOUS_MODE = 0x00000051, /* 8 bits (bool) */
	MAC_WRP_PIB_SHORT_ADDRESS = 0x00000053, /* 16 bits */
	MAC_WRP_PIB_MAX_FRAME_RETRIES = 0x00000059, /* 8 bits */
	MAC_WRP_PIB_TIMESTAMP_SUPPORTED = 0x0000005C, /* 8 bits (bool) */
	MAC_WRP_PIB_SECURITY_ENABLED = 0x0000005D, /* 8 bits (bool) */
	MAC_WRP_PIB_KEY_TABLE = 0x00000071, /* 16 Byte entries */
	MAC_WRP_PIB_FRAME_COUNTER = 0x00000077, /* 32 bits */
	MAC_WRP_PIB_HIGH_PRIORITY_WINDOW_SIZE = 0x00000100, /* 8 bits */
	MAC_WRP_PIB_TX_DATA_PACKET_COUNT = 0x00000101, /* 32 bits */
	MAC_WRP_PIB_RX_DATA_PACKET_COUNT = 0x00000102, /* 32 bits */
	MAC_WRP_PIB_TX_CMD_PACKET_COUNT = 0x00000103, /* 32 bits */
	MAC_WRP_PIB_RX_CMD_PACKET_COUNT = 0x00000104, /* 32 bits */
	MAC_WRP_PIB_CSMA_FAIL_COUNT = 0x00000105, /* 32 bits */
	MAC_WRP_PIB_CSMA_NO_ACK_COUNT = 0x00000106, /* 32 bits */
	MAC_WRP_PIB_RX_DATA_BROADCAST_COUNT = 0x00000107, /* 32 bits */
	MAC_WRP_PIB_TX_DATA_BROADCAST_COUNT = 0x00000108, /* 32 bits */
	MAC_WRP_PIB_BAD_CRC_COUNT = 0x00000109, /* 32 bits */
	MAC_WRP_PIB_NEIGHBOUR_TABLE = 0x0000010A, /* 16 Byte entries on Spec17, 18 on Spec15 */
	MAC_WRP_PIB_FREQ_NOTCHING = 0x0000010B, /* 8 bits (bool) */
	MAC_WRP_PIB_CSMA_FAIRNESS_LIMIT = 0x0000010C, /* 8 bits */
	MAC_WRP_PIB_TMR_TTL = 0x0000010D, /* 8 bits */
	MAC_WRP_PIB_NEIGHBOUR_TABLE_ENTRY_TTL = 0x0000010E, /* Used in Spec15. 8 bits */
	MAC_WRP_PIB_POS_TABLE_ENTRY_TTL = 0x0000010E, /* Used in Spec17. 8 bits */
	MAC_WRP_PIB_RC_COORD = 0x0000010F, /* 16 bits */
	MAC_WRP_PIB_TONE_MASK = 0x00000110, /* 9 Byte array */
	MAC_WRP_PIB_BEACON_RANDOMIZATION_WINDOW_LENGTH = 0x00000111, /* 8 bits */
	MAC_WRP_PIB_A = 0x00000112, /* 8 bits */
	MAC_WRP_PIB_K = 0x00000113, /* 8 bits */
	MAC_WRP_PIB_MIN_CW_ATTEMPTS = 0x00000114, /* 8 bits */
	MAC_WRP_PIB_CENELEC_LEGACY_MODE = 0x00000115, /* 8 bits */
	MAC_WRP_PIB_FCC_LEGACY_MODE = 0x00000116, /* 8 bits */
	MAC_WRP_PIB_BROADCAST_MAX_CW_ENABLE = 0x0000011E, /* 8 bits (bool) */
	MAC_WRP_PIB_TRANSMIT_ATTEN = 0x0000011F, /* 8 bits */
	MAC_WRP_PIB_POS_TABLE = 0x00000120, /* 5 Byte entries */
	/* manufacturer specific */
	/* provides access to device table. 8 Byte entries. */
	MAC_WRP_PIB_MANUF_DEVICE_TABLE = 0x08000000,
	/* Extended address of this node. 8 Byte array. */
	MAC_WRP_PIB_MANUF_EXTENDED_ADDRESS = 0x08000001,
	/* provides access to neighbour table by short address (transmitted as index) */
	/* 16 Byte entries on Spec17, 18 on Spec15 */
	MAC_WRP_PIB_MANUF_NEIGHBOUR_TABLE_ELEMENT = 0x08000002,
	/* returns the maximum number of tones used by the band. 11 Byte struct. */
	MAC_WRP_PIB_MANUF_BAND_INFORMATION = 0x08000003,
	/* Short address of the coordinator. 16 bits. */
	MAC_WRP_PIB_MANUF_COORD_SHORT_ADDRESS = 0x08000004,
	/* Maximal payload supported by MAC. 16 bits. */
	MAC_WRP_PIB_MANUF_MAX_MAC_PAYLOAD_SIZE = 0x08000005,
	/* Resets the device table upon a GMK activation. 8 bits (bool). */
	MAC_WRP_PIB_MANUF_SECURITY_RESET = 0x08000006,
	/* Forces Modulation Scheme in every transmitted frame. 8 bits. */
	/* 0 - Not forced, 1 - Force Differential, 2 - Force Coherent */
	MAC_WRP_PIB_MANUF_FORCED_MOD_SCHEME = 0x08000007,
	/* Forces Modulation Type in every transmitted frame. 8 bits. */
	/* 0 - Not forced, 1 - Force BPSK_ROBO, 2 - Force BPSK, 3 - Force QPSK, 4 - Force 8PSK */
	MAC_WRP_PIB_MANUF_FORCED_MOD_TYPE = 0x08000008,
	/* Forces ToneMap in every transmitted frame. 3 Byte array. */
	/* {0} - Not forced, other value will be used as tonemap */
	MAC_WRP_PIB_MANUF_FORCED_TONEMAP = 0x08000009,
	/* Forces Modulation Scheme bit in Tone Map Response. 8 bits. */
	/* 0 - Not forced, 1 - Force Differential, 2 - Force Coherent */
	MAC_WRP_PIB_MANUF_FORCED_MOD_SCHEME_ON_TMRESPONSE = 0x0800000A,
	/* Forces Modulation Type bits in Tone Map Response. 8 bits. */
	/* 0 - Not forced, 1 - Force BPSK_ROBO, 2 - Force BPSK, 3 - Force QPSK, 4 - Force 8PSK */
	MAC_WRP_PIB_MANUF_FORCED_MOD_TYPE_ON_TMRESPONSE = 0x0800000B,
	/* Forces ToneMap field Tone Map Response. 3 Byte array. */
	/* {0} - Not forced, other value will be used as tonemap field */
	MAC_WRP_PIB_MANUF_FORCED_TONEMAP_ON_TMRESPONSE = 0x0800000C,
	/* Gets Modulation Scheme of last received frame. 8 bits. */
	MAC_WRP_PIB_MANUF_LAST_RX_MOD_SCHEME = 0x0800000D,
	/* Gets Modulation Scheme of last received frame. 8 bits. */
	MAC_WRP_PIB_MANUF_LAST_RX_MOD_TYPE = 0x0800000E,
	/* Indicates whether an LBP frame has been received. 8 bits (bool). */
	MAC_WRP_PIB_MANUF_LBP_FRAME_RECEIVED = 0x0800000F,
	/* Indicates whether an LNG frame has been received. 8 bits (bool). */
	MAC_WRP_PIB_MANUF_LNG_FRAME_RECEIVED = 0x08000010,
	/* Indicates whether an Beacon frame has been received. 8 bits (bool). */
	MAC_WRP_PIB_MANUF_BCN_FRAME_RECEIVED = 0x08000011,
	/* Gets number of valid elements in the Neighbour Table. 16 bits. */
	MAC_WRP_PIB_MANUF_NEIGHBOUR_TABLE_COUNT = 0x08000012,
	/* Gets number of discarded packets due to Other Destination. 32 bits. */
	MAC_WRP_PIB_MANUF_RX_OTHER_DESTINATION_COUNT = 0x08000013,
	/* Gets number of discarded packets due to Invalid Frame Length. 32 bits. */
	MAC_WRP_PIB_MANUF_RX_INVALID_FRAME_LENGTH_COUNT = 0x08000014,
	/* Gets number of discarded packets due to MAC Repetition. 32 bits. */
	MAC_WRP_PIB_MANUF_RX_MAC_REPETITION_COUNT = 0x08000015,
	/* Gets number of discarded packets due to Wrong Addressing Mode. 32 bits. */
	MAC_WRP_PIB_MANUF_RX_WRONG_ADDR_MODE_COUNT = 0x08000016,
	/* Gets number of discarded packets due to Unsupported Security. 32 bits. */
	MAC_WRP_PIB_MANUF_RX_UNSUPPORTED_SECURITY_COUNT = 0x08000017,
	/* Gets number of discarded packets due to Wrong Key Id. 32 bits. */
	MAC_WRP_PIB_MANUF_RX_WRONG_KEY_ID_COUNT = 0x08000018,
	/* Gets number of discarded packets due to Invalid Key. 32 bits. */
	MAC_WRP_PIB_MANUF_RX_INVALID_KEY_COUNT = 0x08000019,
	/* Gets number of discarded packets due to Wrong Frame Counter. 32 bits. */
	MAC_WRP_PIB_MANUF_RX_WRONG_FC_COUNT = 0x0800001A,
	/* Gets number of discarded packets due to Decryption Error. 32 bits. */
	MAC_WRP_PIB_MANUF_RX_DECRYPTION_ERROR_COUNT = 0x0800001B,
	/* Gets number of discarded packets due to Segment Decode Error. 32 bits. */
	MAC_WRP_PIB_MANUF_RX_SEGMENT_DECODE_ERROR_COUNT = 0x0800001C,
	/* Enables MAC Sniffer. 8 bits (bool). */
	MAC_WRP_PIB_MANUF_ENABLE_MAC_SNIFFER = 0x0800001D,
	/* Gets number of valid elements in the POS Table. Unused in SPEC-15. 16 bits. */
	MAC_WRP_PIB_MANUF_POS_TABLE_COUNT = 0x0800001E,
	/* Gets or Sets number of retires left before forcing ROBO mode. 8 bits. */
	MAC_WRP_PIB_MANUF_RETRIES_LEFT_TO_FORCE_ROBO = 0x0800001F,
	/* Gets internal MAC version. 6 Byte struct. */
	MAC_WRP_PIB_MANUF_MAC_INTERNAL_VERSION = 0x08000021,
	/* Gets internal MAC RT version. 6 Byte struct. */
	MAC_WRP_PIB_MANUF_MAC_RT_INTERNAL_VERSION = 0x08000022,
	/* Resets MAC statistics */
	MAC_WRP_PIB_MANUF_RESET_MAC_STATS = 0x08000023,
	/* Enable/Disable Sleep Mode */
	MAC_WRP_PIB_MANUF_SLEEP_MODE = 0x08000024,
	/* Set PLC in Debug Mode */
	MAC_WRP_PIB_MANUF_DEBUG_SET = 0x08000025,
	/* Read PLC debug information */
	MAC_WRP_PIB_MANUF_DEBUG_READ = 0x08000026,
	/* Provides access to POS table by short address (referenced as index). 5 Byte entries */
	MAC_WRP_PIB_MANUF_POS_TABLE_ELEMENT = 0x08000027,
#ifdef G3_HYBRID_PROFILE
	/* Gets or sets a parameter in Phy layer. Index will be used to contain PHY parameter ID. */
	/* See definitions below */
	MAC_WRP_PIB_MANUF_PHY_PARAM = 0x08000020,
	/* RF IBs definition */
	MAC_WRP_PIB_DSN_RF = 0x00000200, /* 8 bits */
	MAC_WRP_PIB_MAX_BE_RF = 0x00000201, /* 8 bits */
	MAC_WRP_PIB_MAX_CSMA_BACKOFFS_RF = 0x00000202, /* 8 bits */
	MAC_WRP_PIB_MAX_FRAME_RETRIES_RF = 0x00000203, /* 8 bits */
	MAC_WRP_PIB_MIN_BE_RF = 0x00000204, /* 8 bits */
	MAC_WRP_PIB_TIMESTAMP_SUPPORTED_RF = 0x00000205, /* 8 bits (bool) */
	MAC_WRP_PIB_DEVICE_TABLE_RF = 0x00000206, /* 6 Byte entries. */
	MAC_WRP_PIB_FRAME_COUNTER_RF = 0x00000207, /* 32 bits */
	MAC_WRP_PIB_DUPLICATE_DETECTION_TTL_RF = 0x00000208, /* 8 bits */
	MAC_WRP_PIB_COUNTER_OCTETS_RF = 0x00000209, /* 8 bits */
	MAC_WRP_PIB_RETRY_COUNT_RF = 0x0000020A, /* 32 bits */
	MAC_WRP_PIB_MULTIPLE_RETRY_COUNT_RF = 0x0000020B, /* 32 bits */
	MAC_WRP_PIB_TX_FAIL_COUNT_RF = 0x0000020C, /* 32 bits */
	MAC_WRP_PIB_TX_SUCCESS_COUNT_RF = 0x0000020D, /* 32 bits */
	MAC_WRP_PIB_FCS_ERROR_COUNT_RF = 0x0000020E, /* 32 bits */
	MAC_WRP_PIB_SECURITY_FAILURE_COUNT_RF = 0x0000020F, /* 32 bits */
	MAC_WRP_PIB_DUPLICATE_FRAME_COUNT_RF = 0x00000210, /* 32 bits */
	MAC_WRP_PIB_RX_SUCCESS_COUNT_RF = 0x00000211, /* 32 bits */
	MAC_WRP_PIB_NACK_COUNT_RF = 0x00000212, /* 32 bits */
	MAC_WRP_PIB_USE_ENHANCED_BEACON_RF = 0x00000213, /* 8 bits (bool) */
	MAC_WRP_PIB_EB_HEADER_IE_LIST_RF = 0x00000214, /* Array of 8 bit elements */
	MAC_WRP_PIB_EB_PAYLOAD_IE_LIST_RF = 0x00000215, /* Array of 8 bit elements */
	MAC_WRP_PIB_EB_FILTERING_ENABLED_RF = 0x00000216, /* 8 bits (bool) */
	MAC_WRP_PIB_EBSN_RF = 0x00000217, /* 8 bits */
	MAC_WRP_PIB_EB_AUTO_SA_RF = 0x00000218, /* 8 bits */
	MAC_WRP_PIB_SEC_SECURITY_LEVEL_LIST_RF = 0x0000021A, /* 4 Byte entries. */
	MAC_WRP_PIB_POS_TABLE_RF = 0x0000021C, /* 9 Byte entries. */
	MAC_WRP_PIB_OPERATING_MODE_RF = 0x0000021D, /* 8 bits */
	MAC_WRP_PIB_CHANNEL_NUMBER_RF = 0x0000021E, /* 16 bits */
	MAC_WRP_PIB_DUTY_CYCLE_USAGE_RF = 0x0000021F, /* 8 bits */
	MAC_WRP_PIB_DUTY_CYCLE_PERIOD_RF = 0x00000220, /* 16 bits */
	MAC_WRP_PIB_DUTY_CYCLE_LIMIT_RF = 0x00000221, /* 16 bits */
	MAC_WRP_PIB_DUTY_CYCLE_THRESHOLD_RF = 0x00000222, /* 8 bits */
	MAC_WRP_PIB_DISABLE_PHY_RF = 0x00000223, /* 8 bits (bool) */
	/* Manufacturer specific */
	/* Resets the device table upon a GMK activation. 8 bits (bool) */
	MAC_WRP_PIB_MANUF_SECURITY_RESET_RF = 0x08000203,
	/* Indicates whether an LBP frame for other destination has been received. 8 bits (bool) */
	MAC_WRP_PIB_MANUF_LBP_FRAME_RECEIVED_RF = 0x08000204,
	/* Indicates whether an LBP frame for other destination has been received. 8 bits (bool) */
	MAC_WRP_PIB_MANUF_LNG_FRAME_RECEIVED_RF = 0x08000205,
	/* Indicates whether an Beacon frame from other nodes has been received. 8 bits (bool) */
	MAC_WRP_PIB_MANUF_BCN_FRAME_RECEIVED_RF = 0x08000206,
	/* Gets number of discarded packets due to Other Destination. 32 bits */
	MAC_WRP_PIB_MANUF_RX_OTHER_DESTINATION_COUNT_RF = 0x08000207,
	/* Gets number of discarded packets due to Invalid Frame Lenght. 32 bits */
	MAC_WRP_PIB_MANUF_RX_INVALID_FRAME_LENGTH_COUNT_RF = 0x08000208,
	/* Gets number of discarded packets due to Wrong Addressing Mode. 32 bits */
	MAC_WRP_PIB_MANUF_RX_WRONG_ADDR_MODE_COUNT_RF = 0x08000209,
	/* Gets number of discarded packets due to Unsupported Security. 32 bits */
	MAC_WRP_PIB_MANUF_RX_UNSUPPORTED_SECURITY_COUNT_RF = 0x0800020A,
	/* Gets number of discarded packets due to Wrong Key Id. 32 bits */
	MAC_WRP_PIB_MANUF_RX_WRONG_KEY_ID_COUNT_RF = 0x0800020B,
	/* Gets number of discarded packets due to Invalid Key. 32 bits */
	MAC_WRP_PIB_MANUF_RX_INVALID_KEY_COUNT_RF = 0x0800020C,
	/* Gets number of discarded packets due to Wrong Frame Counter. 32 bits */
	MAC_WRP_PIB_MANUF_RX_WRONG_FC_COUNT_RF = 0x0800020D,
	/* Gets number of discarded packets due to Decryption Error. 32 bits */
	MAC_WRP_PIB_MANUF_RX_DECRYPTION_ERROR_COUNT_RF = 0x0800020E,
	/* Gets number of transmitted Data packets. 32 bits */
	MAC_WRP_PIB_MANUF_TX_DATA_PACKET_COUNT_RF = 0x0800020F,
	/* Gets number of received Data packets. 32 bits */
	MAC_WRP_PIB_MANUF_RX_DATA_PACKET_COUNT_RF = 0x08000210,
	/* Gets number of transmitted Command packets. 32 bits */
	MAC_WRP_PIB_MANUF_TX_CMD_PACKET_COUNT_RF = 0x08000211,
	/* Gets number of received Command packets. 32 bits */
	MAC_WRP_PIB_MANUF_RX_CMD_PACKET_COUNT_RF = 0x08000212,
	/* Gets number of Channel Access failures. 32 bits */
	MAC_WRP_PIB_MANUF_CSMA_FAIL_COUNT_RF = 0x08000213,
	/* Gets number of received broadcast packets. 32 bits */
	MAC_WRP_PIB_MANUF_RX_DATA_BROADCAST_COUNT_RF = 0x08000214,
	/* Gets number of transmitted broadcast packets. 32 bits */
	MAC_WRP_PIB_MANUF_TX_DATA_BROADCAST_COUNT_RF = 0x08000215,
	/* Gets number of received packets with wrong CRC. 32 bits */
	MAC_WRP_PIB_MANUF_BAD_CRC_COUNT_RF = 0x08000216,
	/* Enables MAC Sniffer. 8 bits (bool) */
	MAC_WRP_PIB_MANUF_ENABLE_MAC_SNIFFER_RF = 0x08000217,
	/* Gets number of valid elements in the POS Table. 16 bits */
	MAC_WRP_PIB_MANUF_POS_TABLE_COUNT_RF = 0x08000218,
	/* Gets internal MAC version. 6 Byte struct. */
	MAC_WRP_PIB_MANUF_MAC_INTERNAL_VERSION_RF = 0x08000219,
	/* Resets MAC statistics. 8 bits (bool) */
	MAC_WRP_PIB_MANUF_RESET_MAC_STATS_RF = 0x0800021A,
	/* Provides access to POS table by short address (referenced as index). 9 Byte entries */
	MAC_WRP_PIB_MANUF_POS_TABLE_ELEMENT_RF = 0x0800021B,
	/* Gets or sets a parameter in Phy layer. Index will be used to contain PHY parameter ID */
	MAC_WRP_PIB_MANUF_PHY_PARAM_RF = 0x08000220
#else
	/* Gets or sets a parameter in Phy layer. Index will be used to contain PHY parameter ID. */
	/* See definitions below */
	MAC_WRP_PIB_MANUF_PHY_PARAM = 0x08000020
#endif
};

enum EMacWrpPhyParam {
	/* Phy layer version number. 32 bits. */
	MAC_WRP_PHY_PARAM_VERSION = 0x010c,
	/* Correctly transmitted frame count. 32 bits. */
	MAC_WRP_PHY_PARAM_TX_TOTAL = 0x0110,
	/* Transmitted bytes count. 32 bits. */
	MAC_WRP_PHY_PARAM_TX_TOTAL_BYTES = 0x0114,
	/* Transmission errors count. 32 bits. */
	MAC_WRP_PHY_PARAM_TX_TOTAL_ERRORS = 0x0118,
	/* Transmission failure due to already in transmission. 32 bits. */
	MAC_WRP_PHY_PARAM_BAD_BUSY_TX = 0x011C,
	/* Transmission failure due to busy channel. 32 bits. */
	MAC_WRP_PHY_PARAM_TX_BAD_BUSY_CHANNEL = 0x0120,
	/* Bad len in message (too short - too long). 32 bits. */
	MAC_WRP_PHY_PARAM_TX_BAD_LEN = 0x0124,
	/* Message to transmit in bad format. 32 bits. */
	MAC_WRP_PHY_PARAM_TX_BAD_FORMAT = 0x0128,
	/* Timeout error in transmission. 32 bits. */
	MAC_WRP_PHY_PARAM_TX_TIMEOUT = 0x012C,
	/* Received correctly messages count. 32 bits. */
	MAC_WRP_PHY_PARAM_RX_TOTAL = 0x0130,
	/* Received bytes count. 32 bits. */
	MAC_WRP_PHY_PARAM_RX_TOTAL_BYTES = 0x0134,
	/* Reception RS errors count. 32 bits. */
	MAC_WRP_PHY_PARAM_RX_RS_ERRORS = 0x0138,
	/* Reception Exceptions count. 32 bits. */
	MAC_WRP_PHY_PARAM_RX_EXCEPTIONS = 0x013C,
	/* Bad len in message (too short - too long). 32 bits. */
	MAC_WRP_PHY_PARAM_RX_BAD_LEN = 0x0140,
	/* Bad CRC in received FCH. 32 bits. */
	MAC_WRP_PHY_PARAM_RX_BAD_CRC_FCH = 0x0144,
	/* CRC correct but invalid protocol. 32 bits. */
	MAC_WRP_PHY_PARAM_RX_FALSE_POSITIVE = 0x0148,
	/* Received message in bad format. 32 bits. */
	MAC_WRP_PHY_PARAM_RX_BAD_FORMAT = 0x014C,
	/* Time between noise captures (in ms). 32 bits. */
	MAC_WRP_PHY_PARAM_TIME_BETWEEN_NOISE_CAPTURES = 0x0158,
	/* Auto detect impedance. 8 bits (bool). */
	MAC_WRP_PHY_PARAM_CFG_AUTODETECT_BRANCH = 0x0161,
	/* Manual impedance configuration. 8 bits. */
	MAC_WRP_PHY_PARAM_CFG_IMPEDANCE = 0x0162,
	/* Indicate if notch filter is active or not. 8 bits (bool). */
	MAC_WRP_PHY_PARAM_RRC_NOTCH_ACTIVE = 0x0163,
	/* Index of the notch filter. 8 bits. */
	MAC_WRP_PHY_PARAM_RRC_NOTCH_INDEX = 0x0164,
	/* Enable periodic noise autodetect and adaptation. 8 bits (bool). */
	MAC_WRP_PHY_PARAM_ENABLE_AUTO_NOISE_CAPTURE = 0x0166,
	/* Noise detection timer reload after a correct reception. 8 bits (bool). */
	MAC_WRP_PHY_PARAM_DELAY_NOISE_CAPTURE_AFTER_RX = 0x0167,
	/* Disable PLC Tx/Rx. 8 bits (bool). */
	MAC_WRP_PHY_PARAM_PLC_DISABLE = 0x016A,
	/* Indicate noise power in dBuV for the noisier carrier. 8 bits. */
	MAC_WRP_PHY_PARAM_NOISE_PEAK_POWER = 0x016B,
	/* LQI value of the last received message. 8 bits. */
	MAC_WRP_PHY_PARAM_LAST_MSG_LQI = 0x016C,
	/* RSSI value of the last received message. 16 bits. */
	MAC_WRP_PHY_PARAM_LAST_MSG_RSSI = 0x016D,
	/* Success transmission of ACK packets. 16 bits. */
	MAC_WRP_PHY_PARAM_ACK_TX_CFM = 0x016E,
	/* Inform PHY layer about enabled modulations on TMR. 8 bits. */
	MAC_WRP_PHY_PARAM_TONE_MAP_RSP_ENABLED_MODS = 0x0174,
	/* Reset Phy Statistics */
	MAC_WRP_PHY_PARAM_RESET_PHY_STATS = 0x0176
};

#ifdef G3_HYBRID_PROFILE
enum EMacWrpPhyParamRF {
	/* RF device identifier. 16 bits */
	MAC_WRP_RF_PHY_PARAM_DEVICE_ID = 0x0000,
	/* RF PHY layer firmware version number. 6 bytes (see "at86rf_fw_version_t") */
	MAC_WRP_RF_PHY_PARAM_FW_VERSION = 0x0001,
	/* RF device reset (write-only) */
	MAC_WRP_RF_PHY_PARAM_DEVICE_RESET = 0x0002,
	/* RF transceiver (RF09 or RF24) reset (write-only) */
	MAC_WRP_RF_PHY_PARAM_TRX_RESET = 0x0080,
	/* RF transceiver (RF09 or RF24) sleep (write-only) */
	MAC_WRP_RF_PHY_PARAM_TRX_SLEEP = 0x0081,
	/* RF PHY configuration (see "at86rf_phy_cfg_t") */
	MAC_WRP_RF_PHY_PARAM_PHY_CONFIG = 0x0100,
	/* RF PHY band and operating mode. 16 bits (see "at86rf_phy_band_opm_t") */
	MAC_WRP_RF_PHY_PARAM_PHY_BAND_OPERATING_MODE = 0x0101,
	/* RF channel number used for transmission and reception. 16 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_CHANNEL_NUM = 0x0120,
	/* RF frequency in Hz used for transmission and reception. 32 bits (read-only) */
	MAC_WRP_RF_PHY_PARAM_PHY_CHANNEL_FREQ_HZ = 0x0121,
	/* Configuration of Energy Detection for CCA. 3 bytes (see "at86rf_cca_ed_cfg_t") */
	MAC_WRP_RF_PHY_PARAM_PHY_CCA_ED_CONFIG = 0x0140,
	/* Duration in us of Energy Detection for CCA. 16 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_CCA_ED_DURATION = 0x0141,
	/* Threshold in dBm of for CCA with Energy Detection. 16 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_CCA_ED_THRESHOLD = 0x0142,
	/* Turnaround time in us (aTurnaroundTime in IEEE 802.15.4). 16 bits (read-only) */
	MAC_WRP_RF_PHY_PARAM_PHY_TURNAROUND_TIME = 0x0160,
	/* Number of payload symbols in last transmitted message. 16 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_TX_PAY_SYMBOLS = 0x0180,
	/* Number of payload symbols in last received message. 16 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_RX_PAY_SYMBOLS = 0x0181,
	/* Successfully transmitted messages count. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_TX_TOTAL = 0x01A0,
	/* Successfully transmitted bytes count. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_TX_TOTAL_BYTES = 0x01A1,
	/* Transmission errors count. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_TOTAL = 0x01A2,
	/* Transmission errors count due to already in transmission. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_BUSY_TX = 0x01A3,
	/* Transmission errors count due to already in reception. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_BUSY_RX = 0x01A4,
	/* Transmission errors count due to busy channel. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_BUSY_CHN = 0x01A5,
	/* Transmission errors count due to bad message length. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_BAD_LEN = 0x01A6,
	/* Transmission errors count due to bad format. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_BAD_FORMAT = 0x01A7,
	/* Transmission errors count due to timeout. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_TIMEOUT = 0x01A8,
	/* Transmission aborted count. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_TX_ERR_ABORTED = 0x01A9,
	/* Transmission confirms not handled by upper layer count. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_TX_CFM_NOT_HANDLED = 0x01AA,
	/* Successfully received messages count. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_RX_TOTAL = 0x01B0,
	/* Successfully received bytes count. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_RX_TOTAL_BYTES = 0x01B1,
	/* Reception errors count. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_TOTAL = 0x01B2,
	/* Reception false positive count. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_FALSE_POSITIVE = 0x01B3,
	/* Reception errors count due to bad message length. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_BAD_LEN = 0x01B4,
	/* Reception errors count due to bad format or bad FCS in header. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_BAD_FORMAT = 0x01B5,
	/* Reception errors count due to bad FCS in payload. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_BAD_FCS_PAY = 0x01B6,
	/* Reception aborted count. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_RX_ERR_ABORTED = 0x01B7,
	/* Reception overrided (another message with higher signal level) count. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_RX_OVERRIDE = 0x01B8,
	/* Reception indications not handled by upper layer count. 32 bits */
	MAC_WRP_RF_PHY_PARAM_PHY_RX_IND_NOT_HANDLED = 0x01B9,
	/* Reset Phy Statistics (write-only) */
	MAC_WRP_RF_PHY_PARAM_PHY_STATS_RESET = 0x01C0,
	/* Backoff period unit in us (aUnitBackoffPeriod in IEEE 802.15.4) used for CSMA-CA . 16 bits (read-only) */
	MAC_WRP_RF_PHY_PARAM_MAC_UNIT_BACKOFF_PERIOD = 0x0200,
	/* SUN FSK FEC enabled or disabled for transmission (phyFskFecEnabled in IEEE 802.15.4). 8 bits */
	MAC_WRP_RF_PHY_PARAM_TX_FSK_FEC = 0x8000,
	/* SUN OFDM MCS (Modulation and coding scheme) used for transmission. 8 bits */
	MAC_WRP_RF_PHY_PARAM_TX_OFDM_MCS = 0x8001,
};
#endif

#define MAC_WRP_PIB_MAX_VALUE_LENGTH (144)

struct TMacWrpPibValue {
	uint8_t m_u8Length;
	uint8_t m_au8Value[MAC_WRP_PIB_MAX_VALUE_LENGTH];
};

struct TMacWrpTxCoef {
	uint8_t m_au8TxCoef[6];
};

struct TMacWrpNeighbourEntry {
	TMacWrpShortAddress m_nShortAddress;
	struct TMacWrpToneMap m_ToneMap;
	uint8_t m_nModulationType : 3;
	uint8_t m_nTxGain : 4;
	uint8_t m_nTxRes : 1;
	struct TMacWrpTxCoef m_TxCoef;
	uint8_t m_nModulationScheme : 1;
	uint8_t m_nPhaseDifferential : 3;
	uint8_t m_u8Lqi;
	uint16_t m_u16TmrValidTime;
	uint16_t m_u16NeighbourValidTime;
};

struct TMacWrpPOSEntry {
	TMacWrpShortAddress m_nShortAddress;
	uint8_t m_u8Lqi;
	uint16_t m_u16POSValidTime;
};

#ifdef G3_HYBRID_PROFILE

struct TMacWrpPOSEntryRF {
	TMacWrpShortAddress m_nShortAddress;
	uint8_t m_u8ForwardLqi;
	uint8_t m_u8ReverseLqi;
	uint8_t m_u8DutyCycle;
	uint8_t m_u8ForwardTxPowerOffset;
	uint8_t m_u8ReverseTxPowerOffset;
	uint16_t m_u16POSValidTime;
};

struct TMacWrpMhrRF {
	struct TMacWrpFc m_Fc;
	uint8_t m_u8SequenceNumber;
	TMacWrpPanId m_nDestinationPanIdentifier;
	struct TMacWrpAddress m_DestinationAddress;
	TMacWrpPanId m_nSourcePanIdentifier;
	struct TMacWrpAddress m_SourceAddress;
	struct TMacWrpAuxiliarySecurityHeader m_SecurityHeader;
};

struct TMacWrpFrameRF {
	struct TMacWrpMhrRF m_Header;
	uint16_t m_u16PayloadLength;
	uint8_t *m_pu8Payload;
	uint8_t m_u8PadLength;
	uint16_t m_u16Fcs;
};

#endif /* G3_HYBRID_PROFILE */

#endif

/**********************************************************************************************************************/

/** @}
 **********************************************************************************************************************/
