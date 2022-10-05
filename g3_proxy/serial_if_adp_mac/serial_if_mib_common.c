/**
 * \file
 *
 * \brief Serial Interface for MAC layer
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

#include <stdbool.h>
#include <stdint.h>
#include "conf_global.h"
#include "serial_if_mib_common.h"
#include "string.h"
#include "debug.h"

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

static uint8_t auc_aux_endiannes_buf[256]; /* !<  Set Request Endianness tranformation buffer */
static uint16_t debug_set_length = 0;

/* void process_MIB_get_request(uint8_t * puc_serial_data, struct TMacWrpGetRequest * ps_results ){ */
void process_MIB_get_request(uint8_t *puc_serial_data, void *ps_results2 )
{
	uint32_t ul_aux;

	struct TMacWrpGetRequest *ps_results = (struct TMacWrpGetRequest *)ps_results2;

	ul_aux = ((uint32_t)*puc_serial_data++) << 24;
	ul_aux += ((uint32_t)*puc_serial_data++) << 16;
	ul_aux += ((uint32_t)*puc_serial_data++) << 8;
	ul_aux += (uint32_t)*puc_serial_data++;
	ps_results->m_ePibAttribute = (enum EMacWrpPibAttribute)ul_aux;

	ps_results->m_u16PibAttributeIndex = ((uint16_t)*puc_serial_data++) << 8;
	ps_results->m_u16PibAttributeIndex += (uint16_t)*puc_serial_data;
}

void process_MIB_set_request(uint8_t *puc_serial_data, struct TMacWrpSetRequest *ps_results )
{
	uint8_t u8AttributeLengthCnt = 0;
	struct TMacWrpNeighbourEntry s_aux_NE;
	struct TMacWrpPOSEntry s_aux_PE;
	uint32_t ul_aux;

	PRINTF(PRINT_INFO, "\r\nprocess_MIB_set_request\r\n");
	PRINTF(PRINT_INFO, "Value =\r\n");
	for (int i = 0; i < ps_results->m_PibAttributeValue.m_u8Length; i++) {
		PRINTF(PRINT_INFO, "%02X:", ps_results->m_PibAttributeValue.m_au8Value[i]);
	}
	PRINTF(PRINT_INFO, "\r\n");

	ul_aux = ((uint32_t)*puc_serial_data++) << 24;
	ul_aux += ((uint32_t)*puc_serial_data++) << 16;
	ul_aux += ((uint32_t)*puc_serial_data++) << 8;
	ul_aux += (uint32_t)*puc_serial_data++;
	ps_results->m_ePibAttribute = (enum EMacWrpPibAttribute)ul_aux;

	ps_results->m_u16PibAttributeIndex = ((uint16_t)*puc_serial_data++) << 8;
	ps_results->m_u16PibAttributeIndex += (uint16_t)*puc_serial_data++;

	ps_results->m_PibAttributeValue.m_u8Length = *puc_serial_data++;

	switch (ps_results->m_ePibAttribute) {
	case MAC_WRP_PIB_ACK_WAIT_DURATION:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_BSN:
		/* m_u8Bsn */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_DSN:
		/* m_u8Dsn */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MAX_BE:
		/* m_u8MaxBe */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MAX_CSMA_BACKOFFS:
		/* m_u8MaxCsmaBackoffs */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MAX_FRAME_RETRIES:
		/* m_u8MaxFrameRetries */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MIN_BE:
		/* m_u8MinBe */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_PAN_ID:
		/* m_nPanId */
		mem_copy_from_usi_endianness_uint16(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 2;
		puc_serial_data += 2;
		break;

	case MAC_WRP_PIB_SECURITY_ENABLED:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_SHORT_ADDRESS:
		/* m_nShortAddress */
		mem_copy_from_usi_endianness_uint16(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 2;
		puc_serial_data += 2;
		break;

	case MAC_WRP_PIB_PROMISCUOUS_MODE:
		/* m_bPromiscuousMode */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_TIMESTAMP_SUPPORTED:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_KEY_TABLE:
		memcpy((uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt], puc_serial_data, MAC_WRP_SECURITY_KEY_LENGTH);
		u8AttributeLengthCnt += MAC_WRP_SECURITY_KEY_LENGTH;
		puc_serial_data += MAC_WRP_SECURITY_KEY_LENGTH;
		break;

	case MAC_WRP_PIB_FRAME_COUNTER:
		/* m_au32FrameCounter */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_HIGH_PRIORITY_WINDOW_SIZE:
		/* Boolean value */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_TX_DATA_PACKET_COUNT:
		/* m_u32TxDataPacketCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_RX_DATA_PACKET_COUNT:
		/* m_u32RxDataPacketCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_TX_CMD_PACKET_COUNT:
		/* m_u32TxCmdPacketCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_RX_CMD_PACKET_COUNT:
		/* m_u32RxCmdPacketCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_CSMA_FAIL_COUNT:
		/* m_u32CsmaFailCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_CSMA_NO_ACK_COUNT:
		/* m_u32CsmaNoAckCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_RX_DATA_BROADCAST_COUNT:
		/* m_u32TxDataBroadcastCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_TX_DATA_BROADCAST_COUNT:
		/* m_u32RxDataBroadcastCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_BAD_CRC_COUNT:
		/* m_u32BadCrcCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_NEIGHBOUR_TABLE:
		s_aux_NE.m_nShortAddress = (uint16_t)((*puc_serial_data++) << 8);
		s_aux_NE.m_nShortAddress += (uint16_t)(*puc_serial_data++);
		memcpy((uint8_t *)&s_aux_NE.m_ToneMap.m_au8Tm[0], puc_serial_data, (MAC_WRP_MAX_TONE_GROUPS + 7) / 8);
		puc_serial_data += (MAC_WRP_MAX_TONE_GROUPS + 7) / 8;
		s_aux_NE.m_nModulationType = (uint8_t)(*puc_serial_data++);
		s_aux_NE.m_nTxGain = (uint8_t)(*puc_serial_data++);
		s_aux_NE.m_nTxRes = (uint8_t)(*puc_serial_data++);
		memcpy((uint8_t *)&s_aux_NE.m_TxCoef.m_au8TxCoef[0], puc_serial_data, 6);
		puc_serial_data += 6;
		s_aux_NE.m_nModulationScheme = (uint8_t)(*puc_serial_data++);
		s_aux_NE.m_nPhaseDifferential = (uint8_t)(*puc_serial_data++);
		s_aux_NE.m_u8Lqi  = (uint8_t)(*puc_serial_data++);
		s_aux_NE.m_u16TmrValidTime = (uint16_t)((*puc_serial_data++) << 8);
		s_aux_NE.m_u16TmrValidTime += (uint16_t)(*puc_serial_data++);
		s_aux_NE.m_u16NeighbourValidTime = (uint16_t)((*puc_serial_data++) << 8);
		s_aux_NE.m_u16NeighbourValidTime += (uint16_t)(*puc_serial_data++);
		memcpy((uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt], &s_aux_NE, sizeof(struct TMacWrpNeighbourEntry));
		u8AttributeLengthCnt += sizeof(struct TMacWrpNeighbourEntry);
		/* Struct save 2 bytes with bit-fields */
		ps_results->m_PibAttributeValue.m_u8Length  -= 2;
		break;

	case MAC_WRP_PIB_FREQ_NOTCHING:
		/* m_bFreqNotching */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_POS_TABLE:
		s_aux_PE.m_nShortAddress = (uint16_t)((*puc_serial_data++) << 8);
		s_aux_PE.m_nShortAddress += (uint16_t)(*puc_serial_data++);
		s_aux_PE.m_u8Lqi  = (uint8_t)(*puc_serial_data++);
		s_aux_PE.m_u16POSValidTime = (uint16_t)((*puc_serial_data++) << 8);
		s_aux_PE.m_u16POSValidTime += (uint16_t)(*puc_serial_data++);
		memcpy((uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt], &s_aux_PE, sizeof(struct TMacWrpPOSEntry));
		u8AttributeLengthCnt += sizeof(struct TMacWrpPOSEntry);
		ps_results->m_PibAttributeValue.m_u8Length  = sizeof(struct TMacWrpPOSEntry);
		break;

	case MAC_WRP_PIB_CSMA_FAIRNESS_LIMIT:
		/* m_u8CsmaFairnessLimit */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_TMR_TTL:
		/* m_u8TmrTtl */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_POS_TABLE_ENTRY_TTL: /* MAC_WRP_PIB_NEIGHBOUR_TABLE_ENTRY_TTL also enters here as it has the same numeric value */
		/* m_u8NeighbourTableEntryTtl */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_RC_COORD:
		/* m_u16RcCoord */
		mem_copy_from_usi_endianness_uint16(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 2;
		puc_serial_data += 2;
		break;

	case MAC_WRP_PIB_TONE_MASK:
		memcpy((uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt], puc_serial_data, (MAC_WRP_MAX_TONES + 7) / 8);
		u8AttributeLengthCnt += (MAC_WRP_MAX_TONES + 7) / 8;
		puc_serial_data += (MAC_WRP_MAX_TONES + 7) / 8;
		break;

	case MAC_WRP_PIB_BEACON_RANDOMIZATION_WINDOW_LENGTH:
		/* m_u8BeaconRandomizationWindowLength */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_A:
		/* m_u8A */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_K:
		/* m_u8K */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MIN_CW_ATTEMPTS:
		/* m_u8MinCwAttempts */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_CENELEC_LEGACY_MODE:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_FCC_LEGACY_MODE:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_BROADCAST_MAX_CW_ENABLE:
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++; /* m_bBroadcastMaxCwEnable */
		break;

	case MAC_WRP_PIB_MANUF_DEVICE_TABLE:
	/* MAC_WRP_STATUS_READ_ONLY */
	case MAC_WRP_PIB_MANUF_EXTENDED_ADDRESS:
		/* m_au8Address */
		memcpy((uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt], puc_serial_data, 8);
		u8AttributeLengthCnt += 8;
		puc_serial_data += 8;
		break;

	case MAC_WRP_PIB_MANUF_NEIGHBOUR_TABLE_ELEMENT:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_MANUF_BAND_INFORMATION:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_MANUF_COORD_SHORT_ADDRESS:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_MANUF_MAX_MAC_PAYLOAD_SIZE:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_MANUF_SECURITY_RESET:
		break;         /* If length is 0 then DeviceTable is going to be reset else response will be MAC_WRP_STATUS_INVALID_PARAMETER */

	case MAC_WRP_PIB_MANUF_FORCED_MOD_SCHEME:
		/* m_u8ForcedModScheme */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MANUF_FORCED_MOD_TYPE:
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MANUF_FORCED_TONEMAP:
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MANUF_FORCED_MOD_SCHEME_ON_TMRESPONSE:
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MANUF_FORCED_MOD_TYPE_ON_TMRESPONSE:
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MANUF_FORCED_TONEMAP_ON_TMRESPONSE:
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MANUF_LAST_RX_MOD_SCHEME:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_MANUF_LAST_RX_MOD_TYPE:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_MANUF_LBP_FRAME_RECEIVED:
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MANUF_LNG_FRAME_RECEIVED:
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MANUF_BCN_FRAME_RECEIVED:
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MANUF_NEIGHBOUR_TABLE_COUNT:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_MANUF_RX_OTHER_DESTINATION_COUNT:
		/* m_u32RxOtherDestinationCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_INVALID_FRAME_LENGTH_COUNT:
		/* *m_u32RxInvalidFrameLengthCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_MAC_REPETITION_COUNT:
		/* m_u32RxMACRepetitionCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_WRONG_ADDR_MODE_COUNT:
		/* m_u32RxWrongAddrModeCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_UNSUPPORTED_SECURITY_COUNT:
		/* m_u32RxUnsupportedSecurityCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_WRONG_KEY_ID_COUNT:
		/* m_u32RxWrongKeyIdCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_INVALID_KEY_COUNT:
		/* m_u32RxInvalidKeyCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_WRONG_FC_COUNT:
		/* m_u32RxWrongFCCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_DECRYPTION_ERROR_COUNT:
		/* m_u32RxDecryptionErrorCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_MANUF_RX_SEGMENT_DECODE_ERROR_COUNT:
		/* m_u32RxSegmentDecodeErrorCount */
		mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
		u8AttributeLengthCnt += 4;
		puc_serial_data += 4;
		break;

	case MAC_WRP_PIB_MANUF_ENABLE_MAC_SNIFFER:
		/* m_bMacSniffer */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MANUF_POS_TABLE_COUNT:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_MANUF_RETRIES_LEFT_TO_FORCE_ROBO:
		/* m_u8RetriesToForceRobo */
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MANUF_MAC_INTERNAL_VERSION:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_MANUF_MAC_RT_INTERNAL_VERSION:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_MANUF_RESET_MAC_STATS:
		break;  /* If length is 0 then DeviceTable is going to be reset else response will be MAC_WRP_STATUS_INVALID_PARAMETER */

	case MAC_WRP_PIB_MANUF_SLEEP_MODE:
		auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
		break;

	case MAC_WRP_PIB_MANUF_DEBUG_SET:
		/* m_au8Address */
		memcpy((uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt], puc_serial_data, 7);
		mem_copy_from_usi_endianness_uint16(&puc_serial_data[5], (uint8_t *)&debug_set_length);
		if (debug_set_length > 255) {
			debug_set_length = 0;
		}

		u8AttributeLengthCnt += 7;
		puc_serial_data += 7;
		break;

	case MAC_WRP_PIB_MANUF_DEBUG_READ:
		/* MAC_WRP_STATUS_READ_ONLY */
		break;

	case MAC_WRP_PIB_MANUF_PHY_PARAM:
		switch (ps_results->m_u16PibAttributeIndex) {
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
			/* m_u32BadCrcCount */
			mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
			u8AttributeLengthCnt += 4;
			puc_serial_data += 4;
			break;

		case MAC_WRP_PHY_PARAM_ENABLE_AUTO_NOISE_CAPTURE:
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
			break;

		case MAC_WRP_PHY_PARAM_TIME_BETWEEN_NOISE_CAPTURES:
			mem_copy_from_usi_endianness_uint32(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
			u8AttributeLengthCnt += 4;
			puc_serial_data += 4;
			break;

		case MAC_WRP_PHY_PARAM_DELAY_NOISE_CAPTURE_AFTER_RX:
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
			break;

		case MAC_WRP_PHY_PARAM_CFG_AUTODETECT_BRANCH:
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
			break;

		case MAC_WRP_PHY_PARAM_CFG_IMPEDANCE:
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
			break;

		case MAC_WRP_PHY_PARAM_RRC_NOTCH_ACTIVE:
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
			break;

		case MAC_WRP_PHY_PARAM_RRC_NOTCH_INDEX:
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
			break;

		case MAC_WRP_PHY_PARAM_PLC_DISABLE:
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
			break;

		case MAC_WRP_PHY_PARAM_NOISE_PEAK_POWER:
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
			break;

		case MAC_WRP_PHY_PARAM_LAST_MSG_LQI:
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
			break;

		case MAC_WRP_PHY_PARAM_LAST_MSG_RSSI:
			mem_copy_from_usi_endianness_uint16(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
			u8AttributeLengthCnt += 2;
			puc_serial_data += 2;
			break;

		case MAC_WRP_PHY_PARAM_ACK_TX_CFM:
			mem_copy_from_usi_endianness_uint16(puc_serial_data, (uint8_t *)&auc_aux_endiannes_buf[u8AttributeLengthCnt]);
			u8AttributeLengthCnt += 2;
			puc_serial_data += 2;
			break;

		case MAC_WRP_PHY_PARAM_TONE_MAP_RSP_ENABLED_MODS:
			auc_aux_endiannes_buf[u8AttributeLengthCnt++] = *puc_serial_data++;
			break;

		case MAC_WRP_PHY_PARAM_RESET_PHY_STATS:
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}

	memcpy( &ps_results->m_PibAttributeValue.m_au8Value[0], &auc_aux_endiannes_buf[0], ps_results->m_PibAttributeValue.m_u8Length);
}

uint8_t process_MIB_get_confirm(uint8_t *puc_serial_data, struct TMacWrpGetConfirm *ps_results )
{
	uint8_t us_serial_response_len;
	struct TMacWrpNeighbourEntry *p_aux;
	struct TMacWrpPOSEntry *p_aux2;

	us_serial_response_len = 0;

	puc_serial_data[us_serial_response_len++] = (uint8_t)ps_results->m_eStatus;
	puc_serial_data[us_serial_response_len++] = (uint8_t)(((uint32_t)(ps_results->m_ePibAttribute >> 24)) & 0xFF);
	puc_serial_data[us_serial_response_len++] = (uint8_t)(((uint32_t)(ps_results->m_ePibAttribute >> 16)) & 0xFF);
	puc_serial_data[us_serial_response_len++] = (uint8_t)(((uint32_t)(ps_results->m_ePibAttribute >> 8)) & 0xFF);
	puc_serial_data[us_serial_response_len++] = (uint8_t)(((uint32_t)(ps_results->m_ePibAttribute)) & 0xFF);
	puc_serial_data[us_serial_response_len++] = (uint8_t)(ps_results->m_u16PibAttributeIndex >> 8);
	puc_serial_data[us_serial_response_len++] = (uint8_t)(ps_results->m_u16PibAttributeIndex);

	puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_u8Length;

	PRINTF(PRINT_INFO, "\r\nVALUE = ");
	for (int i = 0; i < ps_results->m_PibAttributeValue.m_u8Length; i++) {
		PRINTF(PRINT_INFO, "%02X:", ps_results->m_PibAttributeValue.m_au8Value[i]);
	}
	PRINTF(PRINT_INFO, "\r\n");

	if (ps_results->m_eStatus == MAC_WRP_STATUS_SUCCESS) {
		switch (ps_results->m_ePibAttribute) {
		case MAC_WRP_PIB_ACK_WAIT_DURATION:
			/* u16AckWaitDuration */
			mem_copy_to_usi_endianness_uint16((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 2;
			break;

		case MAC_WRP_PIB_BSN:
			/* m_u8Bsn */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_DSN:
			/* m_u8Dsn */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MAX_BE:
			/* m_u8MaxBe */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MAX_CSMA_BACKOFFS:
			/* m_u8MaxCsmaBackoffs */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MAX_FRAME_RETRIES:
			/* m_u8MaxFrameRetries */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MIN_BE:
			/* m_u8MinBe */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_PAN_ID:
			/* m_nPanId */
			mem_copy_to_usi_endianness_uint16((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 2;
			break;

		case MAC_WRP_PIB_SECURITY_ENABLED:
			/* Boolean value */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_SHORT_ADDRESS:
			/* m_nShortAddress */
			mem_copy_to_usi_endianness_uint16((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 2;
			break;

		case MAC_WRP_PIB_PROMISCUOUS_MODE:
			/* m_bPromiscuousMode */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_TIMESTAMP_SUPPORTED:
			/* Boolean value */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_KEY_TABLE:
			/* response will be MAC_WRP_STATUS_UNAVAILABLE_KEY */
			break;

		case MAC_WRP_PIB_FRAME_COUNTER:
			/* m_au32FrameCounter */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_HIGH_PRIORITY_WINDOW_SIZE:
			/* Boolean value */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_TX_DATA_PACKET_COUNT:
			/* m_u32TxDataPacketCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_RX_DATA_PACKET_COUNT:
			/* m_u32RxDataPacketCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_TX_CMD_PACKET_COUNT:
			/* m_u32TxCmdPacketCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_RX_CMD_PACKET_COUNT:
			/* m_u32RxCmdPacketCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_CSMA_FAIL_COUNT:
			/* m_u32CsmaFailCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_CSMA_NO_ACK_COUNT:
			/* m_u32CsmaNoAckCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_RX_DATA_BROADCAST_COUNT:
			/* m_u32TxDataBroadcastCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_TX_DATA_BROADCAST_COUNT:
			/* m_u32RxDataBroadcastCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_BAD_CRC_COUNT:
			/* m_u32BadCrcCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_NEIGHBOUR_TABLE:
			p_aux = (struct TMacWrpNeighbourEntry *)&ps_results->m_PibAttributeValue.m_au8Value[0];
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_nShortAddress >> 8);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_nShortAddress & 0xFF);
			memcpy((uint8_t *)&puc_serial_data[us_serial_response_len], (uint8_t *)&p_aux->m_ToneMap.m_au8Tm[0], (MAC_WRP_MAX_TONE_GROUPS + 7) / 8);
			us_serial_response_len += (MAC_WRP_MAX_TONE_GROUPS + 7) / 8;
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_nModulationType);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_nTxGain);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_nTxRes);
			memcpy((uint8_t *)&puc_serial_data[us_serial_response_len], (uint8_t *)&p_aux->m_TxCoef.m_au8TxCoef[0], 6);
			us_serial_response_len += 6;
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_nModulationScheme);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_nPhaseDifferential);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_u8Lqi);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_u16TmrValidTime >> 8);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_u16TmrValidTime & 0xFF);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_u16NeighbourValidTime >> 8);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_u16NeighbourValidTime & 0xFF);
			/* Length has to be incremented by 2 due to bitfields in the entry are serialized in separate fields */
			puc_serial_data[7] = ps_results->m_PibAttributeValue.m_u8Length + 2;
			break;

		case MAC_WRP_PIB_FREQ_NOTCHING:
			/* m_bFreqNotching */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_POS_TABLE:
			p_aux2 = (struct TMacWrpPOSEntry *)&ps_results->m_PibAttributeValue.m_au8Value[0];
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux2->m_nShortAddress >> 8);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux2->m_nShortAddress & 0xFF);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux2->m_u8Lqi);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux2->m_u16POSValidTime >> 8);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux2->m_u16POSValidTime & 0xFF);
			/* Fix length */
			puc_serial_data[7] = 5;
			break;

		case MAC_WRP_PIB_CSMA_FAIRNESS_LIMIT:
			/* m_u8CsmaFairnessLimit */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_TMR_TTL:
			/* m_u8TmrTtl */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_POS_TABLE_ENTRY_TTL: /* MAC_WRP_PIB_NEIGHBOUR_TABLE_ENTRY_TTL also enters here as it has the same numeric value */
			/* m_u8NeighbourTableEntryTtl */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_RC_COORD:
			/* m_u16RcCoord */
			mem_copy_to_usi_endianness_uint16((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 2;
			break;

		case MAC_WRP_PIB_TONE_MASK:
			memcpy((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0], (MAC_WRP_MAX_TONES + 7) / 8);
			us_serial_response_len += (MAC_WRP_MAX_TONES + 7) / 8;
			break;

		case MAC_WRP_PIB_BEACON_RANDOMIZATION_WINDOW_LENGTH:
			/* m_u8BeaconRandomizationWindowLength */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_A:
			/* m_u8A */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_K:
			/* m_u8K */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MIN_CW_ATTEMPTS:
			/* m_u8MinCwAttempts */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_CENELEC_LEGACY_MODE:
			/* PhyBoolGetLegacyMode */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_FCC_LEGACY_MODE:
			/* PhyBoolGetLegacyMode */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_BROADCAST_MAX_CW_ENABLE:
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0]; /* m_bBroadcastMaxCwEnable */
			break;

		case MAC_WRP_PIB_MANUF_DEVICE_TABLE:
			/* m_nPanId */
			mem_copy_to_usi_endianness_uint16((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 2;
			/* m_nShortAddress */
			mem_copy_to_usi_endianness_uint16((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[2]);
			us_serial_response_len += 2;
			/* m_au32FrameCounter */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[4]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_MANUF_EXTENDED_ADDRESS:
			/* m_au8Address */
			memcpy((uint8_t *)&puc_serial_data[us_serial_response_len], (uint8_t *)ps_results->m_PibAttributeValue.m_au8Value, 8);
			us_serial_response_len += 8;
			break;

		case MAC_WRP_PIB_MANUF_NEIGHBOUR_TABLE_ELEMENT:
			p_aux = (struct TMacWrpNeighbourEntry *)&ps_results->m_PibAttributeValue.m_au8Value[0];
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_nShortAddress >> 8);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_nShortAddress & 0xFF);
			memcpy((uint8_t *)&puc_serial_data[us_serial_response_len], (uint8_t *)&p_aux->m_ToneMap.m_au8Tm[0], (MAC_WRP_MAX_TONE_GROUPS + 7) / 8);
			us_serial_response_len += (MAC_WRP_MAX_TONE_GROUPS + 7) / 8;
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_nModulationType);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_nTxGain);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_nTxRes);
			memcpy((uint8_t *)&puc_serial_data[us_serial_response_len], (uint8_t *)&p_aux->m_TxCoef.m_au8TxCoef[0], 6);
			us_serial_response_len += 6;
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_nModulationScheme);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_nPhaseDifferential);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_u8Lqi);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_u16TmrValidTime >> 8);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_u16TmrValidTime & 0xFF);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_u16NeighbourValidTime >> 8);
			puc_serial_data[us_serial_response_len++] = (uint8_t)(p_aux->m_u16NeighbourValidTime & 0xFF);
			/* Length has to be incremented by 2 due to bitfields in the entry are serialized in separate fields */
			puc_serial_data[7] = ps_results->m_PibAttributeValue.m_u8Length + 2;
			break;

		case MAC_WRP_PIB_MANUF_BAND_INFORMATION:
			/* m_u16FlMax */
			mem_copy_to_usi_endianness_uint16((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 2;
			/* m_u8Band */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[2];
			/* m_u8Tones */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[3];
			/* m_u8Carriers */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[4];
			/* m_u8TonesInCarrier */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[5];
			/* m_u8FlBand */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[6];
			/* m_u8MaxRsBlocks */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[7];
			/* m_u8TxCoefBits */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[8];
			/* m_u8PilotsFreqSpa */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[9];
			break;

		case MAC_WRP_PIB_MANUF_COORD_SHORT_ADDRESS:
			/* m_nCoordShortAddress */
			mem_copy_to_usi_endianness_uint16((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 2;
			break;

		case MAC_WRP_PIB_MANUF_MAX_MAC_PAYLOAD_SIZE:
			/* u16MaxMacPayloadSize */
			mem_copy_to_usi_endianness_uint16((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 2;
			break;

		case MAC_WRP_PIB_MANUF_SECURITY_RESET:
			/* Response will be mac_status_denied */
			break;

		case MAC_WRP_PIB_MANUF_FORCED_MOD_SCHEME:
			/* m_u8ForcedModScheme */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MANUF_FORCED_MOD_TYPE:
			/* m_u8ForcedModScheme */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MANUF_FORCED_TONEMAP:
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[1];
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[2];
			break;

		case MAC_WRP_PIB_MANUF_FORCED_MOD_SCHEME_ON_TMRESPONSE:
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MANUF_FORCED_MOD_TYPE_ON_TMRESPONSE:
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MANUF_FORCED_TONEMAP_ON_TMRESPONSE:
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[1];
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[2];
			break;

		case MAC_WRP_PIB_MANUF_LAST_RX_MOD_SCHEME:
			/* m_LastRxModScheme */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MANUF_LAST_RX_MOD_TYPE:
			/* m_LastRxModType */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MANUF_LBP_FRAME_RECEIVED:
			/* m_bLBPFrameReceived */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MANUF_LNG_FRAME_RECEIVED:
			/* m_bLNGFrameReceived */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MANUF_BCN_FRAME_RECEIVED:
			/* m_bBCNFrameReceived */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MANUF_NEIGHBOUR_TABLE_COUNT:
			mem_copy_to_usi_endianness_uint16((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 2;
			break;

		case MAC_WRP_PIB_MANUF_RX_OTHER_DESTINATION_COUNT:
			/* m_u32RxOtherDestinationCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_MANUF_RX_INVALID_FRAME_LENGTH_COUNT:
			/* m_u32RxInvalidFrameLengthCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_MANUF_RX_MAC_REPETITION_COUNT:
			/* m_u32RxMACRepetitionCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_MANUF_RX_WRONG_ADDR_MODE_COUNT:
			/* m_u32RxWrongAddrModeCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_MANUF_RX_UNSUPPORTED_SECURITY_COUNT:
			/* m_u32RxUnsupportedSecurityCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_MANUF_RX_WRONG_KEY_ID_COUNT:
			/* m_u32RxWrongKeyIdCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_MANUF_RX_INVALID_KEY_COUNT:
			/* m_u32RxInvalidKeyCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_MANUF_RX_WRONG_FC_COUNT:
			/* m_u32RxWrongFCCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_MANUF_RX_DECRYPTION_ERROR_COUNT:
			/* m_u32RxDecryptionErrorCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_MANUF_RX_SEGMENT_DECODE_ERROR_COUNT:
			/* m_u32RxSegmentDecodeErrorCount */
			mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 4;
			break;

		case MAC_WRP_PIB_MANUF_ENABLE_MAC_SNIFFER:
			/* m_bMacSniffer */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MANUF_POS_TABLE_COUNT:
			mem_copy_to_usi_endianness_uint16((uint8_t *)&puc_serial_data[us_serial_response_len],
					(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
			us_serial_response_len += 2;
			break;

		case MAC_WRP_PIB_MANUF_RETRIES_LEFT_TO_FORCE_ROBO:
			/* m_u8RetriesToForceRobo */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MANUF_MAC_INTERNAL_VERSION:
			/* Version */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0]; /* m_u8Major */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[1]; /* m_u8Minor */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[2]; /* m_u8Revision */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[3]; /* m_u8Year */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[4]; /* m_u8Month */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[5]; /* m_u8Day */
			break;

		case MAC_WRP_PIB_MANUF_MAC_RT_INTERNAL_VERSION:
			/* Version */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0]; /* m_u8Major */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[1]; /* m_u8Minor */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[2]; /* m_u8Revision */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[3]; /* m_u8Year */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[4]; /* m_u8Month */
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[5]; /* m_u8Day */
			break;

		case MAC_WRP_PIB_MANUF_RESET_MAC_STATS:
			/* Response will be mac_status_denied */
			break;

		case MAC_WRP_PIB_MANUF_SLEEP_MODE:
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			break;

		case MAC_WRP_PIB_MANUF_DEBUG_SET:
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[1];
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[2];
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[3];
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[4];
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[5];
			puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[6];
			break;

		case MAC_WRP_PIB_MANUF_DEBUG_READ:
			memcpy((uint8_t *)&puc_serial_data[us_serial_response_len], (uint8_t *)ps_results->m_PibAttributeValue.m_au8Value, debug_set_length);
			us_serial_response_len += debug_set_length;
			break;

		case MAC_WRP_PIB_MANUF_PHY_PARAM:
			switch (ps_results->m_u16PibAttributeIndex) {
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
				mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
						(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
				us_serial_response_len += 4;
				break;

			case MAC_WRP_PHY_PARAM_ENABLE_AUTO_NOISE_CAPTURE:
				puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
				break;

			case MAC_WRP_PHY_PARAM_TIME_BETWEEN_NOISE_CAPTURES:
				mem_copy_to_usi_endianness_uint32((uint8_t *)&puc_serial_data[us_serial_response_len],
						(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
				us_serial_response_len += 4;
				break;

			case MAC_WRP_PHY_PARAM_DELAY_NOISE_CAPTURE_AFTER_RX:
				puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
				break;

			case MAC_WRP_PHY_PARAM_CFG_AUTODETECT_BRANCH:
				puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
				break;

			case MAC_WRP_PHY_PARAM_CFG_IMPEDANCE:
				puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
				break;

			case MAC_WRP_PHY_PARAM_RRC_NOTCH_ACTIVE:
				puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
				break;

			case MAC_WRP_PHY_PARAM_RRC_NOTCH_INDEX:
				puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
				break;

			case MAC_WRP_PHY_PARAM_PLC_DISABLE:
				puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
				break;

			case MAC_WRP_PHY_PARAM_NOISE_PEAK_POWER:
				puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
				break;

			case MAC_WRP_PHY_PARAM_LAST_MSG_LQI:
				puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
				break;

			case MAC_WRP_PHY_PARAM_LAST_MSG_RSSI:
				mem_copy_to_usi_endianness_uint16((uint8_t *)&puc_serial_data[us_serial_response_len],
						(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
				us_serial_response_len += 2;
				break;

			case MAC_WRP_PHY_PARAM_ACK_TX_CFM:
				mem_copy_to_usi_endianness_uint16((uint8_t *)&puc_serial_data[us_serial_response_len],
						(uint8_t *)&ps_results->m_PibAttributeValue.m_au8Value[0]);
				us_serial_response_len += 2;
				break;

			case MAC_WRP_PHY_PARAM_TONE_MAP_RSP_ENABLED_MODS:
				puc_serial_data[us_serial_response_len++] = ps_results->m_PibAttributeValue.m_au8Value[0];
				break;

			case MAC_WRP_PHY_PARAM_RESET_PHY_STATS:
				break;

			default:
				break;
			}
			break;

		default:
			break;
		}
	}

	return us_serial_response_len;
}

uint8_t process_MIB_set_confirm(uint8_t *puc_serial_data, struct TMacWrpSetConfirm *ps_results )
{
	uint8_t us_serial_response_len;

	us_serial_response_len = 0;

	puc_serial_data[us_serial_response_len++] = (uint8_t)ps_results->m_eStatus;
	puc_serial_data[us_serial_response_len++] = (uint8_t)(((uint32_t)(ps_results->m_ePibAttribute >> 24)) & 0xFF);
	puc_serial_data[us_serial_response_len++] = (uint8_t)(((uint32_t)(ps_results->m_ePibAttribute >> 16)) & 0xFF);
	puc_serial_data[us_serial_response_len++] = (uint8_t)(((uint32_t)(ps_results->m_ePibAttribute >> 8)) & 0xFF);
	puc_serial_data[us_serial_response_len++] = (uint8_t)(((uint32_t)(ps_results->m_ePibAttribute)) & 0xFF);
	puc_serial_data[us_serial_response_len++] = (uint8_t)(ps_results->m_u16PibAttributeIndex >> 8);
	puc_serial_data[us_serial_response_len++] = (uint8_t)(ps_results->m_u16PibAttributeIndex & 0xFF);

	return us_serial_response_len;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
