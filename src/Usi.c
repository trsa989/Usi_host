/**
 * \file
 *
 * \brief USI Host
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
#include <stdio.h>
#include <string.h>

/* Port includes */
#include "../addUsi.h"
/* Include with the external functions defined by the user */
#include "../userFnc.h"

/* Config Include */
#include "Usi.h"
#include "UsiCfg.h"

/* *** Declarations ********************************************************** */

#define MIN_OVERHEAD            5       /* 1 Start Byte, 2 Bytes (Len+Protocol), 1 End Byte, 1 CRC Byte */

/* *** Public Variables ****************************************************** */

extern usi_decode_cmd_cb cbFnMngLay;
extern usi_decode_cmd_cb cbFnSnifferPrime;
extern usi_decode_cmd_cb cbFn_prime_api;
extern usi_decode_cmd_cb cbFn_primeoudp;

extern usi_decode_cmd_cb cbFnSnifferG3;
extern usi_decode_cmd_cb cbFnG3Mac;
extern usi_decode_cmd_cb cbFnG3Adp;
extern usi_decode_cmd_cb cbFnG3Coord;

extern const uint8_t usiCfgNumProtocols;                /* Number of used protocols */
extern const uint8_t usiCfgNumPorts;                    /* Number of used ports */
extern MapPorts *const usiCfgMapPorts;                          /* Port Mapping */
extern MapProtocols *const usiCfgMapProtocols;          /* Protocol Mapping */
extern MapBuffers *const usiCfgRxBuf;                   /* Reception Buffers Mapping */
extern MapBuffers *const usiCfgTxBuf;                   /* Transmission Buffers Mapping */
extern MapBuffers *const usiCfgAuxTxBuf;                /* Aux buffer for transmission */
extern RxParam *const usiCfgRxParam;                            /* Control parameters in reception */
extern TxParam *const usiCfgTxParam;                            /* Control parameters in transmission */

/* *** Declarations ********************************************************** */
/* Reception states */
enum {
	RX_IDLE,                        /* /< Inactive */
	RX_MSG,                 /* /< Receiving message */
	RX_ESC,                 /* /< Processing escape char */
	RX_EORX                 /* /< Message received correctly */
};
/* ** Transmission   ********************************************************* */
enum {
	TX_IDLE,                        /* /< Inactive */
	TX_MSG                  /* /< Transmitting message */
};

/* *** Local variables ******************************************************* */

static uint32_t rxCrc;
static uint32_t evCrc;

/* -------------------------------- */
/* CRC evaluation table */
static const uint32_t crc32table[256] = {
	0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9,
	0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
	0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61,
	0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD,
	0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9,
	0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75,
	0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011,
	0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD,
	0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039,
	0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5,
	0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81,
	0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,
	0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49,
	0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
	0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1,
	0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D,
	0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE,
	0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072,
	0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16,
	0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA,
	0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE,
	0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02,
	0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066,
	0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
	0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E,
	0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692,
	0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6,
	0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A,
	0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E,
	0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2,
	0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686,
	0xD5B88683, 0xD1799B34, 0xDC3ABDED, 0xD8FBA05A,
	0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637,
	0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB,
	0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F,
	0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53,
	0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47,
	0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B,
	0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF,
	0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623,
	0xF12F560E, 0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7,
	0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B,
	0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F,
	0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
	0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7,
	0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B,
	0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F,
	0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3,
	0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640,
	0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C,
	0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8,
	0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24,
	0x119B4BE9, 0x155A565E, 0x18197087, 0x1CD86D30,
	0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
	0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088,
	0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654,
	0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0,
	0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C,
	0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18,
	0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4,
	0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0,
	0x9ABC8BD5, 0x9E7D9662, 0x933EB0BB, 0x97FFAD0C,
	0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668,
	0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4,
};

/* -------------------------------- */
/* / Table to calculate CRC */
static const uint16_t crc16Table[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

/* ************************************************************************** */

/** @brief	Calculate CRC32 using a table for the polinomial
 *
 *       @param		bufPtr	Ptr to msg with data to use
 *       @param		len		Length of data to evaluate
 *
 *      @return		calculated CRC
 *
 * This function calculates the corresponding CRC for the given data.
 * CRC32 is used on USI Protocol IDs:
 * MNGP_PRIME_GETQRY
 * MNGP_PRIME_GETRSP
 * MNGP_PRIME_SET
 * MNGP_PRIME_RESET
 * MNGP_PRIME_REBOOT
 * MNGP_PRIME_FU
 * MNGP_PRIME_EN_PIBQRY
 * MNGP_PRIME_EN_PIBRSP
 **************************************************************************/

static uint32_t _evalCrc32(const uint8_t *bufPtr, uint16_t len)
{
	uint8_t idx;
	uint32_t crc;

	crc = 0;
	if (len != 0) {
		while (len--) {
			idx = (uint8_t)(crc >> 24) ^ *bufPtr++;
			crc = (crc << 8) ^ crc32table[idx];
		}
	}

	return crc;
}

/* ************************************************************************** */

/** @brief	Calculate CRC16 using a table for the polinomial
 *
 *      @param		bufPtr	Pointer to data buffer
 *      @param		len		Buffer size
 *
 *      @return		calculated CRC
 *
 * This function calculates the corresponding CRC for the given data.
 * CRC16 is used on USI Protocol IDs:
 * PROTOCOL_SNIF_PRIME
 * PROTOCOL_SNIF_G3
 * PROTOCOL_MAC_G3
 * PROTOCOL_ADP_G3
 * PROTOCOL_COORD_G3
 * PROTOCOL_PRIMEoUDP
 **************************************************************************/

static uint16_t _evalCrc16(const uint8_t *bufPtr, uint16_t len)                 /* len = 64 -> 2.93 ms. */
{
	uint16_t crc;

	crc = 0;
	while (len--) {
		crc = (uint16_t)(crc16Table [(crc >> 8) & 0xff] ^ (crc << 8) ^ (*bufPtr++ & 0x00ff));
	}
	return crc;
}

/* ************************************************************************** */

/** @brief	Calculate CRC16 using a table for the polinomial
 *
 *      @param		bufPtr	Pointer to data buffer
 *      @param		len		Buffer size
 *
 *      @return		calculated CRC
 *
 * This function calculates the corresponding CRC for the given data.
 **************************************************************************/

static uint8_t _evalCrc8(const uint8_t *bufPtr, uint16_t len)
{
	uint8_t crc;

	crc = 0;
	while (len--) {
		crc = crc ^ *bufPtr++;
	}
	return crc;
}

/* ************************************************************************** */

/** @brief	Reset reception
 *
 * Initalize reception machine
 **************************************************************************/

static void _resetRx(uint8_t port)
{
	usiCfgRxParam[port].rxStat = RX_IDLE;
	usiCfgRxParam[port].rcvPktReady = FALSE;
	usiCfgRxParam[port].idx = 0;
}

/* ************************************************************************** */

/** @brief	Get number of port from protocol type
 *
 *      @param		pType	Protocol Type
 *      @return		(-1) if protocol is not found
 *                                      otherwise, number of port used for pType
 *
 * Get number of port from protocol type
 **************************************************************************/

static int8_t _getPortFromProtocol(uint8_t pType)
{
	uint8_t i;

	for (i = 0; i < usiCfgNumProtocols; i++) {
		/*		if (usiCfgMapProtocols[i].pType == pType) */
		/*			return usiCfgMapProtocols[i].port; */
		if ((pType < 0x10) && (usiCfgMapProtocols[i].pType == MNGP_PRIME)) {
			return usiCfgMapProtocols[i].port;
		}

		if (usiCfgMapProtocols[i].pType == pType) {
			return usiCfgMapProtocols[i].port;
		}
	}

	return (-1);
}

/* ************************************************************************** */

/** @brief	This function process the complete received data.
 *
 *      @param		port	Port where message is received
 *
 * Switching data depending on protocol type [TYPE field]
 **************************************************************************/

static uint8_t _processMsg(uint8_t port)
{
	uint16_t len;
	uint8_t type;
	uint8_t *rxBuf;
	uint8_t result = TRUE;

	/* Get Reception buffer */
	rxBuf = &usiCfgRxBuf[port].buf[0];
	/* Extract protocol */
	type = TYPE_PROTOCOL(rxBuf[TYPE_PROTOCOL_OFFSET]);

	/* Extract length */
	if (type == PROTOCOL_PRIME_API || type == PROTOCOL_SNIF_PRIME || type == PROTOCOL_PHY_SERIAL_PRIME) {
		/* Extract LEN using XLEN */
		len = XLEN_PROTOCOL(rxBuf[LEN_PROTOCOL_HI_OFFSET], rxBuf[LEN_PROTOCOL_LO_OFFSET], rxBuf[XLEN_PROTOCOL_OFFSET]);
	} else {
		/* Extract LEN using LEN */
		len = LEN_PROTOCOL(rxBuf[LEN_PROTOCOL_HI_OFFSET], rxBuf[LEN_PROTOCOL_LO_OFFSET]);
	}

	/* Call decoding function depending on type */
	switch (type) {
	/* -------------------------------------------------------------- */
	/* PRIME spec.v.1.3.E */
	/* case MNGP_PRIME: */
	/*		case MNGP_PRIME_GETQRY: */
	case MNGP_PRIME_GETRSP:
	/*		case MNGP_PRIME_SET: */
	/*		case MNGP_PRIME_RESET: */
	/*		case MNGP_PRIME_REBOOT: */
	/*		case MNGP_PRIME_FU: */
	case PROTOCOL_MNGP_PRIME_GETRSP_EN:
		if (cbFnMngLay != NULL) {
			result = cbFnMngLay(&rxBuf[PAYLOAD_OFFSET], len);
		}

		break;

	/* -------------------------------------------------------------- */
	/* Atmel serialized protocols */
	case PROTOCOL_SNIF_PRIME:
		if (cbFnSnifferPrime != NULL) {
			result = cbFnSnifferPrime(&rxBuf[0], len + 4);
		}

		break;

	case PROTOCOL_PRIME_API:
		if (cbFn_prime_api != NULL) {
			result = cbFn_prime_api(&rxBuf[PAYLOAD_OFFSET], len);
		}

		break;

	/* G3 */
	case PROTOCOL_SNIF_G3:
		if (cbFnSnifferG3 != NULL) {
			result = cbFnSnifferG3(&rxBuf[0], len + 4);
		}

		break;

	case PROTOCOL_MAC_G3:
		if (cbFnG3Mac != NULL) {
			result = cbFnG3Mac(&rxBuf[PAYLOAD_OFFSET], len); /* internal commands */
		}

		break;

	case PROTOCOL_ADP_G3:
		if (cbFnG3Adp != NULL) {
			result = cbFnG3Adp(&rxBuf[PAYLOAD_OFFSET], len); /* internal commands */
		}

		break;

	case PROTOCOL_COORD_G3:
		if (cbFnG3Coord != NULL) {
			result = cbFnG3Coord(&rxBuf[PAYLOAD_OFFSET], len); /* internal commands */
		}

		break;

	case PROTOCOL_PHY_SERIAL_PRIME:
		if (cbFn_primeoudp != NULL) {
			result = cbFn_primeoudp(&rxBuf[PAYLOAD_OFFSET], len);
		}

		break;

	default:
		break;
	}

	return result;
}

/* ************************************************************************** */

/** @brief	Process received message
 *
 *  @param  port   Communication Port
 *  @return TRUE if message is OK
 *          FALSE if message is not OK
 *
 * Message received is in 'rxBuf', with a size of 'idx' bytes
 *      - Validate len > 4
 *      - Validate CRC32 over the message
 **************************************************************************/

static uint8_t _doEoMsg(uint8_t port)                   /* 3 ms. */
{
	uint8_t *tb;
	uint8_t type;
	uint16_t count;
	uint16_t len;
	uint8_t *rxBuf;

	/* Get buffer and number of bytes */
	count = usiCfgRxParam[port].idx;
	rxBuf = &usiCfgRxBuf[port].buf[0];
	if (count < 4) {                                        /* insuffucient data */
		return(FALSE);
	}

	/* Extract length and protocol */

	type = TYPE_PROTOCOL(rxBuf[TYPE_PROTOCOL_OFFSET]);

	if (type == PROTOCOL_PRIME_API) {
		len = XLEN_PROTOCOL(rxBuf[LEN_PROTOCOL_HI_OFFSET], rxBuf[LEN_PROTOCOL_LO_OFFSET], rxBuf[XLEN_PROTOCOL_OFFSET]);
	} else {
		len = LEN_PROTOCOL(rxBuf[LEN_PROTOCOL_HI_OFFSET], rxBuf[LEN_PROTOCOL_LO_OFFSET]);
	}

	/* Evaluate CRC depending on protocol */
	switch (type) {
	case MNGP_PRIME_GETQRY:
	case MNGP_PRIME_GETRSP:
	case MNGP_PRIME_SET:
	case MNGP_PRIME_RESET:
	case MNGP_PRIME_REBOOT:
	case MNGP_PRIME_FU:
	case PROTOCOL_MNGP_PRIME_GETQRY_EN:
	case PROTOCOL_MNGP_PRIME_GETRSP_EN:
		/* Get received CRC 32 */
		tb = &rxBuf[count - 4];
		rxCrc = (((uint32_t)tb[0]) << 24) | (((uint32_t)tb[1]) << 16) | (((uint32_t)tb[2]) << 8) | ((uint32_t)tb[3]);
		/* Correct length (exclude CRC) */
		usiCfgRxParam[port].idx -= 4;
		/* Calculate CRC */
		evCrc = _evalCrc32(rxBuf, len + 2);         /* +2 header bytes: included in CRC */
		break;

	case PROTOCOL_SNIF_PRIME:
	case PROTOCOL_SNIF_G3:
	case PROTOCOL_MAC_G3:
	case PROTOCOL_ADP_G3:
	case PROTOCOL_COORD_G3:
	case PROTOCOL_PHY_SERIAL_PRIME:
		/* Get received CRC 16 */
		tb = &rxBuf[count - 2];
		rxCrc = (((uint32_t)tb[0]) << 8) | ((uint32_t)tb[1]);
		/* Correct length (exclude CRC) */
		usiCfgRxParam[port].idx -= 2;
		/* Calculate CRC */
		evCrc = (uint32_t)_evalCrc16(rxBuf, len + 2); /* +2 header bytes: included in CRC */
		break;

	case PROTOCOL_PRIME_API:
		/* Get received CRC 8 */
		tb = &rxBuf[count - 1];
		rxCrc = (uint32_t)tb[0];
		/* Correct length (exclude CRC) */
		usiCfgRxParam[port].idx -= 1;
		/* Calculate CRC */
		evCrc = (uint32_t)_evalCrc8(rxBuf, len + 2); /* +2 header bytes: included in CRC */
		break;

	default:
		return(FALSE);
	}

	/* Return CRC ok or not */
	if (rxCrc == evCrc) {
		return(TRUE);
	} else {
		return(FALSE);
	}
}

/**************************************************************************
***************************************************************************
**
**  Interface
**
***************************************************************************
**************************************************************************/

/* ************************************************************************** */

/** @brief	Initialize Serial Profile
 *
 * Init all to inactive
 **************************************************************************/

void usi_Init(void)
{
	uint8_t i;

	for (i = 0; i < usiCfgNumPorts; i++) {
		/* Init Rx Parameters */
		usiCfgRxParam[i].rxStat = RX_IDLE;
		usiCfgRxParam[i].rcvPktReady = FALSE;
		usiCfgRxParam[i].idx = 0;
		/* Init Tx Parameters */
		usiCfgTxParam[i].count = 0;
		usiCfgTxParam[i].idxIn = 0;
		usiCfgTxParam[i].idxOut = 0;
	}
}

/* ************************************************************************** */

/** @brief	Configure communication port
 *
 *   @param	logPort				Logical Port
 *   @param port_type			Port Type (UART_TYPE, USART_TYPE, COM_TYPE)
 *   @param commPort			Physical Communication Port
 *   @param speed				Communication speed
 *
 **************************************************************************/

void usi_ConfigurePort(uint8_t logPort, uint8_t port_type, uint8_t commPort, uint32_t speed)
{
	if (logPort < usiCfgNumPorts) {
		usiCfgMapPorts[logPort].sType = port_type;
		usiCfgMapPorts[logPort].chn =   commPort;
		usiCfgMapPorts[logPort].speed = speed;
	}
}

/* ************************************************************************** */

/** @brief	Start Serial profile
 *
 *       @param		-
 *
 *
 *      - Open every serial channel
 *      - Start reception
 *      - Free transmission
 **************************************************************************/

void usi_Start(void)
{
	uint8_t i;

	for (i = 0; i < usiCfgNumPorts; i++) {
		addUsi_Open(usiCfgMapPorts[i].sType, usiCfgMapPorts[i].chn, usiCfgMapPorts[i].speed);
	}
}

/* ************************************************************************** */

/** @brief	Transmit message
 *
 *       @param		pType	Protocol Type
 *       @param		msg		Ptr to mesg to transmit
 *       @param		len		Size of message
 *
 *      @return		Result of operation:
 *                                              - TRUE: Sent
 *                                              - FALSE: No room to send
 *
 **************************************************************************/
/* uint8_t usi_SendCmd (uint8_t pType, uint8_t *msg, uint16_tlen) */
uint8_t usi_SendCmd(CmdParams *msg)
{
	uint32_t crc;
	int8_t portIdx;
	uint8_t *ptr2TxBuf;
	uint8_t *ptr2AuxTxBuf;
	uint8_t cmd;
	uint16_t availableLen;
	uint16_t prevIdxIn;
	uint16_t i;
	uint8_t ch;
	uint16_t putChars = 0;
	uint8_t pType = msg->pType;
	uint16_t len = msg->len;

	/* Get port index from protocol */
	portIdx = _getPortFromProtocol(pType);
	if (portIdx == -1) {
		return(FALSE);
	}

	/* Get available length in buffer */
	availableLen = usiCfgTxBuf[portIdx].size - usiCfgTxParam[portIdx].count;

	/* First checking, available length at least equal to minimum required space */
	if (availableLen < (len + MIN_OVERHEAD)) {
		return(FALSE);
	}

	/* Get ptr to TxBuffer */
	ptr2TxBuf = usiCfgTxBuf[portIdx].buf;
	ptr2AuxTxBuf = usiCfgAuxTxBuf->buf;

	/* Store index, just in case after checkings we have to restore previous state */
	prevIdxIn = usiCfgTxParam[portIdx].idxIn;

	/* Copy message to aux buffer including header */
	ptr2AuxTxBuf[0] = LEN_HI_PROTOCOL(len);
	ptr2AuxTxBuf[1] = LEN_LO_PROTOCOL(len) + TYPE_PROTOCOL(pType);
	/* memcpy (&ptr2AuxTxBuf[2], msg, len); */
	memcpy(&ptr2AuxTxBuf[2], msg->buf, len);

	/* Adjust XLEN if pType is prime_api protocol */
	cmd = ptr2AuxTxBuf[CMD_PROTOCOL_OFFSET];
	if (pType == PROTOCOL_PRIME_API) {
		ptr2AuxTxBuf[CMD_PROTOCOL_OFFSET] = LEN_EX_PROTOCOL(len) + CMD_PROTOCOL(cmd);
	}

	/* Add 2 header bytes to LEN */
	len += 2;

	/* Calculate CRC */
	switch (pType) {
	case MNGP_PRIME_GETQRY:
	case MNGP_PRIME_GETRSP:
	case MNGP_PRIME_SET:
	case MNGP_PRIME_RESET:
	case MNGP_PRIME_REBOOT:
	case MNGP_PRIME_FU:
	case PROTOCOL_MNGP_PRIME_GETQRY_EN:
		crc = _evalCrc32(ptr2AuxTxBuf, len);
		ptr2AuxTxBuf[len] = (uint8_t)(crc >> 24);
		ptr2AuxTxBuf[len + 1] = (uint8_t)(crc >> 16);
		ptr2AuxTxBuf[len + 2] = (uint8_t)(crc >> 8);
		ptr2AuxTxBuf[len + 3] = (uint8_t)crc;
		len += 4;
		break;

	case PROTOCOL_SNIF_PRIME:
	case PROTOCOL_SNIF_G3:
	case PROTOCOL_MAC_G3:
	case PROTOCOL_ADP_G3:
	case PROTOCOL_COORD_G3:
		crc = (uint32_t)_evalCrc16(ptr2AuxTxBuf, len);
		ptr2AuxTxBuf[len] = (uint8_t)(crc >> 8);
		ptr2AuxTxBuf[len + 1] = (uint8_t)(crc);
		len += 2;
		break;

	case PROTOCOL_PRIME_API:
		crc = (uint32_t)_evalCrc8(ptr2AuxTxBuf, len);
		ptr2AuxTxBuf[len] = (uint8_t)(crc);
		len += 1;
		break;

	case PROTOCOL_PHY_SERIAL_PRIME:
		crc = (uint32_t)_evalCrc16(ptr2AuxTxBuf, len);
		ptr2AuxTxBuf[len] = (uint8_t)(crc >> 8);
		ptr2AuxTxBuf[len + 1] = (uint8_t)(crc);
		len += 2;
		break;

	default:
		crc = (uint32_t)_evalCrc8(ptr2AuxTxBuf, len);
		ptr2AuxTxBuf[len] = (uint8_t)(crc);
		len += 1;
		break;
	}

	/* Fill tx buffer adding required escapes -------------------------------------------- */
	/* ----------------------------------------------------------------------------------- */

	/* Start Escape */
	ptr2TxBuf[usiCfgTxParam[portIdx].idxIn] = MSGMARK;
	usiCfgTxParam[portIdx].idxIn++;
	putChars++;

	/* Check index overflow */
	if (usiCfgTxParam[portIdx].idxIn == usiCfgTxBuf[portIdx].size) {
		usiCfgTxParam[portIdx].idxIn = 0;
	}

	/* Check there is still room */
	if (usiCfgTxParam[portIdx].idxIn == usiCfgTxParam[portIdx].idxOut) {
		/* No Room. Restore index and return error */
		usiCfgTxParam[portIdx].idxIn = prevIdxIn;
		return(FALSE);
	}

	/* Message */
	for (i = 0; i < len; i++) {
		/* Get next char */
		ch = ptr2AuxTxBuf[i];

		if (ch == MSGMARK || ch == ESCMARK) {
			/* Escape needed (mark) */
			ptr2TxBuf[usiCfgTxParam[portIdx].idxIn] = 0x7D;
			usiCfgTxParam[portIdx].idxIn++;
			putChars++;

			/* Check index overflow */
			if (usiCfgTxParam[portIdx].idxIn == usiCfgTxBuf[portIdx].size) {
				usiCfgTxParam[portIdx].idxIn = 0;
			}

			/* Check there is still room */
			if (usiCfgTxParam[portIdx].idxIn == usiCfgTxParam[portIdx].idxOut) {
				/* No Room. Restore index and return error */
				usiCfgTxParam[portIdx].idxIn = prevIdxIn;
				return(FALSE);
			}

			/* Escape needed (modified char) */
			ptr2TxBuf[usiCfgTxParam[portIdx].idxIn] = (ch ^ 0x20);
			usiCfgTxParam[portIdx].idxIn++;
			putChars++;

			/* Check index overflow */
			if (usiCfgTxParam[portIdx].idxIn == usiCfgTxBuf[portIdx].size) {
				usiCfgTxParam[portIdx].idxIn = 0;
			}

			/* Check there is still room */
			if (usiCfgTxParam[portIdx].idxIn == usiCfgTxParam[portIdx].idxOut) {
				/* No Room. Restore index and return error */
				usiCfgTxParam[portIdx].idxIn = prevIdxIn;
				return(FALSE);
			}
		} else {
			/* No escape */
			ptr2TxBuf[usiCfgTxParam[portIdx].idxIn] = ch;
			usiCfgTxParam[portIdx].idxIn++;
			putChars++;

			/* Check index overflow */
			if (usiCfgTxParam[portIdx].idxIn == usiCfgTxBuf[portIdx].size) {
				usiCfgTxParam[portIdx].idxIn = 0;
			}

			/* Check there is still room */
			if (usiCfgTxParam[portIdx].idxIn == usiCfgTxParam[portIdx].idxOut) {
				/* No Room. Restore index and return error */
				usiCfgTxParam[portIdx].idxIn = prevIdxIn;
				return(FALSE);
			}
		}
	}

	/* End Escape */
	ptr2TxBuf[usiCfgTxParam[portIdx].idxIn] = MSGMARK;
	usiCfgTxParam[portIdx].idxIn++;
	putChars++;

	/* Check index overflow */
	if (usiCfgTxParam[portIdx].idxIn == usiCfgTxBuf[portIdx].size) {
		usiCfgTxParam[portIdx].idxIn = 0;
	}

	/* Check there is still room */
	if (usiCfgTxParam[portIdx].idxIn == usiCfgTxParam[portIdx].idxOut) {
		/* No Room. Restore index and return error */
		usiCfgTxParam[portIdx].idxIn = prevIdxIn;
		return(FALSE);
	}

	/* ----------------------------------------------------------------------------------- */
	/* ----------------------------------------------------------------------------------- */

	/* If this point is reached, message is correct in buffer */
	usiCfgTxParam[portIdx].count += putChars;
	/* Message ready to be sent */
	return(TRUE);
}

/* ************************************************************************** */

/** @brief	Process reception machine
 *
 **************************************************************************/

void usi_RxProcess(void)
{
	uint8_t i;
	uint8_t ch;
	uint8_t nextfor;
	uint8_t chn;
	RxParam *rxCfg;
	uint8_t *rxBuf;
	uint8_t sType;

	uint16_t rxBufSize;
	uint16_t rxCfgIdx;

	/* Check reception on every port */
	for (i = 0; i < usiCfgNumPorts; i++) {
		if (usiCfgRxParam[i].rcvPktReady) {
			continue; /* Last message no processed yet */
		}

		nextfor = FALSE;
		sType = usiCfgMapPorts[i].sType;
		chn = usiCfgMapPorts[i].chn;
		rxCfg = &usiCfgRxParam[i];
		rxCfgIdx = rxCfg->idx;
		rxBuf = &usiCfgRxBuf[i].buf[rxCfgIdx];
		rxBufSize = usiCfgRxBuf[i].size;
		while (TRUE) {
			/*if (usiCfgMapPorts[i].sType == UART_TYPE)
			 *      ch = uartRxChar (chn);
			 * else
			 *      ch = spiRxChar (chn);*/

			/* Get char */

			/* Check if there is something to receive */
			if (addUsi_RxChar(sType, chn, &ch) == -1) {
				/* No char */
				break;
			}

			/* Process received char */
			switch (rxCfg->rxStat) {
			case RX_IDLE:
				if (ch == 0x7e) {
					/* Start reception process */
					rxCfg->idx = rxCfgIdx =  0;
					rxCfg->rxStat = RX_MSG;
				}

				continue;    /* Do not introduce any character in buffer */

			case RX_MSG:
				if (ch == 0x7d) {
					/* Ecape information in message */
					rxCfg->rxStat = RX_ESC;
					continue;
				}

				if (ch == 0x7e) {
					if (rxCfgIdx == 0) {
						/* Two consecutive 0x7E */
						/* The first was ending of a non processed message */
						/* The second is the begining of next message to process */
						continue;
					} else {
						rxCfg->idx = rxCfgIdx;         /* Must be updated before call _doEoMsg */
						/* End reception process */
						if (_doEoMsg(i)) {
							/* CRC is OK */
							rxCfg->rxStat = RX_EORX;
							rxCfg->rcvPktReady = TRUE;
						} else {
							rxCfgIdx = 0;
							/* CRC is NOK */
							_resetRx(i);
						}

						nextfor = TRUE;           /* end while */
					}
				}

				break;

			case RX_ESC:
				/* Ecape secuence */
				if (ch == 0x7d) {
					rxCfgIdx = 0;
					/* It is not possible to receive 0x7D again */
					_resetRx(i);
					continue;
				}

				ch ^= 0x20;
				rxCfg->rxStat = RX_MSG;
				break;

			case RX_EORX:
				break;

			default:
				break;
			} /* switch */

			if (nextfor) {
				break;    /* end while */
			}

			/* Insert in buffer if possible */
			if (rxCfgIdx >= rxBufSize) {                                                    /* Too large */
				rxCfgIdx = 0;
				_resetRx(i);
				break;
			}

			*rxBuf++ = ch;
			rxCfgIdx++;
		} /* End while */
		rxCfg->idx = rxCfgIdx;
	} /* End for */
}

/* ************************************************************************** */

/** @brief	Process transmission machine
 *
 **************************************************************************/

void usi_TxProcess(void)
{
	uint8_t i;
	uint16_t len;
	uint16_t txLen;
	uint16_t sentChars;
	TxParam *txCfg;

	for (i = 0; i < usiCfgNumPorts; i++) {                                              /* Check every buffer */
		/* If there is a complete message in RXBuffer, it should be processed */
		if (usiCfgRxParam[i].rcvPktReady) {
			_processMsg(i);
			/* Message processed. Make room for next */
			usiCfgRxParam[i].rcvPktReady = FALSE;
			usiCfgRxParam[i].rxStat = RX_IDLE;
			usiCfgRxParam[i].idx = 0;
		}

		/* Get Tx Len message */
		txCfg = &usiCfgTxParam[i];
		len = txCfg->count;

		/* Check if there is something to transmit */
		if (len) {
			txLen = len;

			/* In case end of buffer is near, transmit only last part, next process will transmit part pf message placed at the beginning of buffer */
			if ((txCfg->idxOut + txLen) > usiCfgTxBuf[i].size) {
				txLen = usiCfgTxBuf[i].size - txCfg->idxOut;
			}

			/* Send chars to device, checking how many have been really processed by device */
			sentChars = addUsi_TxMsg(usiCfgMapPorts[i].sType, usiCfgMapPorts[i].chn, &usiCfgTxBuf[i].buf[txCfg->idxOut], txLen);

			/* Adjust buffer values depending on sent chars */
			txCfg->count -= sentChars;
			txCfg->idxOut += sentChars;
			/* Check end of buffer */
			if (txCfg->idxOut == usiCfgTxBuf[i].size) {
				txCfg->idxOut = 0;
			}
		}
	}
}

/* ************************************************************************** */

/** @brief	Transmits all pending messages on every port
 *
 *
 **************************************************************************/

void usi_Flush(void)
{
	uint8_t i;
	uint8_t pendingTx = TRUE;

	/* Call Tx Process until all buffers are empty */
	while (pendingTx) {
		/* Transmit pending chars */
		usi_TxProcess();

		/* Check every buffer */
		pendingTx = FALSE;
		for (i = 0; i < usiCfgNumPorts; i++) {
			if (usiCfgTxParam[i].count) {
				pendingTx = TRUE;
				break;
			}
		}
	}
}
