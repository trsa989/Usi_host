/**
 * \file
 *
 * \brief USI Host - G3 ADP API type definition
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
 
#ifndef __ADP_API_TYPES_H__
#define __ADP_API_TYPES_H__

#include "conf_global.h"

#if defined (__CC_ARM)
#pragma anon_unions
#endif

#define ADP_ADDRESS_16BITS 2
#define ADP_ADDRESS_64BITS 8

enum EAdpMac_Modulation {
	MOD_ROBO, MOD_BPSK, MOD_DBPSK, MOD_QPSK, MOD_DQPSK, MOD_8PSK, MOD_D8PSK, MOD_16QAM, MOD_UNKNOWN = 255
};

struct TAdpExtendedAddress {
	uint8_t m_au8Value[8];
};

struct TAdpAddress {
	uint8_t m_u8AddrSize; /* ADP_ADDRESS_16BITS(2) or ADP_ADDRESS_64BITS(8) */
	union {
		uint16_t m_u16ShortAddr;
		struct TAdpExtendedAddress m_ExtendedAddress;
	};
};

enum TAdpBand {
	ADP_BAND_CENELEC_A = 0, ADP_BAND_CENELEC_B = 1, ADP_BAND_FCC = 2, ADP_BAND_ARIB = 3
};

/**********************************************************************************************************************/

/** PAN descriptor structure specification
 *
 ***********************************************************************************************************************
 * @param u16PanId The 16-bit PAN identifier.
 * @param u8LinkQuality The 8-bit link quality of LBA.
 * @param u16LbaAddress The 16 bit short address of a device in this PAN to be used as the LBA by the associating device.
 * @param u16RcCoord The estimated route cost from LBA to the coordinator.
 **********************************************************************************************************************/
struct TAdpPanDescriptor {
	uint16_t m_u16PanId;
	uint8_t m_u8LinkQuality;
	uint16_t m_u16LbaAddress;
	uint16_t m_u16RcCoord;
#ifdef G3_HYBRID_PROFILE
	uint8_t m_u8MediaType;
#endif
};

/**********************************************************************************************************************/

/** Path discovery
 *
 ***********************************************************************************************************************
 * @param m_u16HopAddress The hop / node address
 * @param m_u8Mns MetricNotSupported: 1 the metric type is not supported by the hop, 0 if supported
 * @param  m_u8LinkCost LinkCost of the node
 * @param  m_u8PhaseDiff Phase Differential on link
 * @param  m_u8Mrx Medium from which request is received
 * @param  m_u8Mtx Medium to which request is transmitted
 * @param  m_u8RsvBits Reserved Bits. Read from incoming frame to propagate them correctly
 **********************************************************************************************************************/
struct THopDescriptor {
	uint16_t m_u16HopAddress;
	uint8_t m_u8Mns;
	uint8_t m_u8LinkCost;
	uint8_t m_u8PhaseDiff;
	uint8_t m_u8Mrx;
	uint8_t m_u8Mtx;
	uint8_t m_u8RsvBits;
};

/**********************************************************************************************************************/

/** Path discovery
 *
 ***********************************************************************************************************************
 * @param m_u16DstAddr The short unicast destination address of the path discovery.
 * @param m_u16ExpectedOrigAddr The expected originator of the path reply
 * @param m_u16OrigAddr The real originator of the path reply
 * @param m_u8MetricType Path metric type
 * @param m_u8ForwardHopsCount Number of path hops in the forward table
 * @param m_u8ReverseHopsCount Number of path hops in the reverse table
 * @param m_aForwardPath Table with the information of each hop in forward direction (according to m_u8ForwardHopsCount)
 * @param m_aReversePath Table with the information of each hop in reverse direction (according to m_u8ReverseHopsCount)
 **********************************************************************************************************************/
struct TPathDescriptor {
	uint16_t m_u16DstAddr;
	uint16_t m_u16ExpectedOrigAddr;
	uint16_t m_u16OrigAddr;
	uint16_t m_u16RsvBits;
	uint8_t m_u8MetricType;
	uint8_t m_u8ForwardHopsCount;
	uint8_t m_u8ReverseHopsCount;
	struct THopDescriptor m_aForwardPath[16];
	struct THopDescriptor m_aReversePath[16];
};

/* List of ADP supported MIB attributes. */
enum EAdpPibAttribute {
	ADP_IB_SECURITY_LEVEL = 0x00000000,
	ADP_IB_PREFIX_TABLE = 0x00000001,
	ADP_IB_BROADCAST_LOG_TABLE_ENTRY_TTL = 0x00000002,
	ADP_IB_METRIC_TYPE = 0x00000003,
	ADP_IB_LOW_LQI_VALUE = 0x00000004,
	ADP_IB_HIGH_LQI_VALUE = 0x00000005,
	ADP_IB_RREP_WAIT = 0x00000006,
	ADP_IB_CONTEXT_INFORMATION_TABLE = 0x00000007,
	ADP_IB_COORD_SHORT_ADDRESS = 0x00000008,
	ADP_IB_RLC_ENABLED = 0x00000009,
	ADP_IB_ADD_REV_LINK_COST = 0x0000000A,
	ADP_IB_BROADCAST_LOG_TABLE = 0x0000000B,
	ADP_IB_ROUTING_TABLE = 0x0000000C,
	ADP_IB_UNICAST_RREQ_GEN_ENABLE = 0x0000000D,
	ADP_IB_GROUP_TABLE = 0x0000000E,
	ADP_IB_MAX_HOPS = 0x0000000F,
	ADP_IB_DEVICE_TYPE = 0x00000010,
	ADP_IB_NET_TRAVERSAL_TIME = 0x00000011,
	ADP_IB_ROUTING_TABLE_ENTRY_TTL = 0x00000012,
	ADP_IB_KR = 0x00000013,
	ADP_IB_KM = 0x00000014,
	ADP_IB_KC = 0x00000015,
	ADP_IB_KQ = 0x00000016,
	ADP_IB_KH = 0x00000017,
	ADP_IB_RREQ_RETRIES = 0x00000018,
	ADP_IB_RREQ_RERR_WAIT = 0x00000019,
	ADP_IB_RREQ_WAIT = 0x00000019,
	ADP_IB_WEAK_LQI_VALUE = 0x0000001A,
	ADP_IB_KRT = 0x0000001B,
	ADP_IB_SOFT_VERSION = 0x0000001C,
	ADP_IB_SNIFFER_MODE = 0x0000001D,
	ADP_IB_BLACKLIST_TABLE = 0x0000001E,
	ADP_IB_BLACKLIST_TABLE_ENTRY_TTL = 0x0000001F,
	ADP_IB_MAX_JOIN_WAIT_TIME = 0x00000020,
	ADP_IB_PATH_DISCOVERY_TIME = 0x00000021,
	ADP_IB_ACTIVE_KEY_INDEX = 0x00000022,
	ADP_IB_DESTINATION_ADDRESS_SET = 0x00000023,
	ADP_IB_DEFAULT_COORD_ROUTE_ENABLED = 0x00000024,
#ifdef G3_HYBRID_PROFILE
	ADP_IB_LOW_LQI_VALUE_RF = 0x000000D0, /* 8 bits */
	ADP_IB_HIGH_LQI_VALUE_RF = 0x000000D1, /* 8 bits */
	ADP_IB_KQ_RF = 0x000000D2, /* 8 bits */
	ADP_IB_KH_RF = 0x000000D3, /* 8 bits */
	ADP_IB_KRT_RF = 0x000000D4, /* 8 bits */
	ADP_IB_KDC_RF = 0x000000D5, /* 8 bits */
	ADP_IB_USE_BACKUP_MEDIA = 0x000000D6, /* 8 bits (bool) */
#endif
	ADP_IB_DISABLE_DEFAULT_ROUTING = 0x000000F0,
	/* manufacturer */
	ADP_IB_MANUF_REASSEMBY_TIMER = 0x080000C0,
	ADP_IB_MANUF_IPV6_HEADER_COMPRESSION = 0x080000C1,
	ADP_IB_MANUF_EAP_PRESHARED_KEY = 0x080000C2,
	ADP_IB_MANUF_EAP_NETWORK_ACCESS_IDENTIFIER = 0x080000C3,
	ADP_IB_MANUF_BROADCAST_SEQUENCE_NUMBER = 0x080000C4,
	ADP_IB_MANUF_REGISTER_DEVICE = 0x080000C5,
	ADP_IB_MANUF_DATAGRAM_TAG = 0x080000C6,
	ADP_IB_MANUF_RANDP = 0x080000C7,
	ADP_IB_MANUF_ROUTING_TABLE_COUNT = 0x080000C8,
	ADP_IB_MANUF_DISCOVER_SEQUENCE_NUMBER = 0x080000C9,
	ADP_IB_MANUF_FORCED_NO_ACK_REQUEST = 0x080000CA,
	ADP_IB_MANUF_LQI_TO_COORD = 0x080000CB,
	ADP_IB_MANUF_BROADCAST_ROUTE_ALL = 0x080000CC,
	ADP_IB_MANUF_KEEP_PARAMS_AFTER_KICK_LEAVE = 0x080000CD,
	ADP_IB_MANUF_ADP_INTERNAL_VERSION = 0x080000CE,
	ADP_IB_MANUF_CIRCULAR_ROUTES_DETECTED = 0x080000CF,
	ADP_IB_MANUF_LAST_CIRCULAR_ROUTE_ADDRESS = 0x080000D0,
	ADP_IB_MANUF_IPV6_ULA_DEST_SHORT_ADDRESS = 0x080000D1,
	ADP_IB_MANUF_MAX_REPAIR_RESEND_ATTEMPTS = 0x080000D2,
	ADP_IB_MANUF_DISABLE_AUTO_RREQ = 0x080000D3,
	ADP_IB_MANUF_ALL_NEIGHBORS_BLACKLISTED_COUNT = 0x080000D5,
	ADP_IB_MANUF_QUEUED_ENTRIES_REMOVED_TIMEOUT_COUNT = 0x080000D6,
	ADP_IB_MANUF_QUEUED_ENTRIES_REMOVED_ROUTE_ERROR_COUNT = 0x080000D7,
	ADP_IB_MANUF_PENDING_DATA_IND_SHORT_ADDRESS = 0x080000D8,
	ADP_IB_MANUF_GET_BAND_CONTEXT_TONES = 0x080000D9,
	ADP_IB_MANUF_UPDATE_NON_VOLATILE_DATA = 0x080000DA,
	ADP_IB_MANUF_DISCOVER_ROUTE_GLOBAL_SEQ_NUM = 0x080000DB,
	ADP_IB_MANUF_FRAGMENT_DELAY = 0x080000DC,
	ADP_IB_MANUF_DYNAMIC_FRAGMENT_DELAY_ENABLED = 0x080000DD,
	ADP_IB_MANUF_DYNAMIC_FRAGMENT_DELAY_FACTOR = 0x080000DE,
	ADP_IB_MANUF_BLACKLIST_TABLE_COUNT = 0x080000DF,
	ADP_IB_MANUF_BROADCAST_LOG_TABLE_COUNT = 0x080000E0,
	ADP_IB_MANUF_CONTEXT_INFORMATION_TABLE_COUNT = 0x080000E1,
	ADP_IB_MANUF_GROUP_TABLE_COUNT = 0x080000E2,
	ADP_IB_MANUF_ROUTING_TABLE_ELEMENT = 0x080000E3,
	ADP_IB_MANUF_SET_PHASEDIFF_PREQ_PREP = 0x080000E4,
	ADP_IB_MANUF_HYBRID_PROFILE = 0x080000E5
};

/* List of errors returned by the ADP */
enum EAdpStatus {
	/* / Success */
	G3_SUCCESS = 0x00,
	/* / Invalid request */
	G3_INVALID_REQUEST = 0xA1,
	/* / Request failed */
	G3_FAILED = 0xA2,
	/* / Invalid IPv6 frame */
	G3_INVALID_IPV6_FRAME = 0xA3,
	/* / Not permited */
	G3_NOT_PERMITED = 0xA4,
	/* / No route to destination */
	G3_ROUTE_ERROR = 0xA5,
	/* / Operation timed out */
	G3_TIMEOUT = 0xA6,
	/* / An attempt to write to a MAC PIB attribute that is in a table failed because the specified table index was out of range. */
	G3_INVALID_INDEX = 0xA7,
	/* / A parameter in the primitive is either not supported or is out of the valid range. */
	G3_INVALID_PARAMETER = 0xA8,
	/* / A scan operation failed to find any network beacons. */
	G3_NO_BEACON = 0xA9,
	/* / A SET/GET request was issued with the identifier of an attribute that is read only. */
	G3_READ_ONLY = 0xB0,
	/* / A SET/GET request was issued with the identifier of a PIB attribute that is not supported. */
	G3_UNSUPPORTED_ATTRIBUTE = 0xB1,
	/* / The path discovery has only a part of the path to its desired final destination. */
	G3_INCOMPLETE_PATH = 0xB2,
	/* / Busy: operation already in progress. */
	G3_BUSY = 0xB3,
	/* / Not enough resources */
	G3_NO_BUFFERS = 0xB4,
	/* / Error internal */
	G3_ERROR_INTERNAL = 0xFF
};

#endif
