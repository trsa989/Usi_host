/**
 * \file
 *
 * \brief Simple USI Client
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
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Port includes */
#include "addUsi.h"

#include "debug.h"
/* Config Include */
#include "usi_cli.h"
#include "usi_cfg_cli.h"

/* *** Declarations ********************************************************** */

#define MIN_OVERHEAD            5       /* 1 Start Byte, 2 Bytes (Len+Protocol), 1 End Byte, 1 CRC Byte */

/* *** Public Variables ****************************************************** */

extern usi_decode_cmd_cb cbFnCliSnifferG3;
extern usi_decode_cmd_cb cbFnCliG3Mac;
extern usi_decode_cmd_cb cbFnCliG3Adp;
extern usi_decode_cmd_cb cbFnCliG3Coord;

/* Concentrator socket descriptor */
extern int g_concentrator_fd;
static unsigned char uc_rx_buffer[MAX_BUFFER_SIZE];
static unsigned char uc_rx_tmp_buffer[MAX_BUFFER_SIZE];
static unsigned char uc_tx_buffer[MAX_BUFFER_SIZE];
static unsigned char uc_tx_tmp_buffer[MAX_BUFFER_SIZE];
static RxParam usiCliRxParam;

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

/** @brief	Calculate CRC16 using a table for the polinomial
 *
 *      @param		bufPtr	Pointer to data buffer
 *      @param		len		Buffer size
 *
 *      @return		calculated CRC
 *
 * This function calculates the corresponding CRC for the given data.
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

/** @brief	This function process the complete received data.
 *
 *      @param		port	Port where message is received
 *
 * Switching data depending on protocol type [TYPE field]
 **************************************************************************/

static uint8_t _processMsg(uint16_t count)
{
	uint16_t len;
	uint8_t type;
	uint8_t *rxBuf;
	uint8_t result = TRUE;

	PRINTF(PRINT_INFO, "\r\nProcessing message (len = %u)\r\n", count);

	/* Get Reception buffer */
	rxBuf = uc_rx_buffer;
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
	/* G3 */
	case PROTOCOL_SNIF_G3:
		if (cbFnCliSnifferG3 != NULL) {
			result = cbFnCliSnifferG3(&rxBuf[0], len + 4);
		}

		break;

	case PROTOCOL_MAC_G3:
		if (cbFnCliG3Mac != NULL) {
			result = cbFnCliG3Mac(&rxBuf[PAYLOAD_OFFSET], len); /* internal commands */
		}

		break;

	case PROTOCOL_ADP_G3:
		if (cbFnCliG3Adp != NULL) {
			result = cbFnCliG3Adp(&rxBuf[PAYLOAD_OFFSET], len); /* internal commands */
		}

		break;

	case PROTOCOL_COORD_G3:
		if (cbFnCliG3Coord != NULL) {
			result = cbFnCliG3Coord(&rxBuf[PAYLOAD_OFFSET], len); /* internal commands */
		}

		break;

	default:
		break;
	}

	return result;
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
uint8_t usi_cli_send_cmd(x_usi_serial_cmd_params_t *msg)
{
	uint32_t crc;
	uint8_t *ptr2TxBuf;
	uint8_t *ptr2AuxTxBuf;
	uint16_t i;
	uint8_t ch;
	uint16_t putChars = 0;
	uint16_t bytes_sent;
	uint8_t pType = msg->uc_protocol_type;
	uint16_t len = msg->us_len;

	/* Get ptr to TxBuffer */
	ptr2TxBuf = uc_tx_buffer;
	ptr2AuxTxBuf = uc_tx_tmp_buffer;

	/* Copy message to aux buffer including header */
	ptr2AuxTxBuf[0] = LEN_HI_PROTOCOL(len);
	ptr2AuxTxBuf[1] = LEN_LO_PROTOCOL(len) + TYPE_PROTOCOL(pType);
	/* memcpy (&ptr2AuxTxBuf[2], msg, len); */
	memcpy(&ptr2AuxTxBuf[2], msg->ptr_buf, len);

	/* Add 2 header bytes to LEN */
	len += 2;

	/* Calculate CRC */
	crc = (uint32_t)_evalCrc16(ptr2AuxTxBuf, len);
	ptr2AuxTxBuf[len] = (uint8_t)(crc >> 8);
	ptr2AuxTxBuf[len + 1] = (uint8_t)(crc);
	len += 2;

	/* Fill tx buffer adding required escapes -------------------------------------------- */
	/* ----------------------------------------------------------------------------------- */

	/* Start Escape */
	ptr2TxBuf[putChars++] = MSGMARK;

	/* Message */
	for (i = 0; i < len; i++) {
		/* Get next char */
		ch = ptr2AuxTxBuf[i];

		if (ch == MSGMARK || ch == ESCMARK) {
			/* Escape needed (mark) */
			ptr2TxBuf[putChars++] = 0x7D;

			/* Escape needed (modified char) */
			ptr2TxBuf[putChars++] = (ch ^ 0x20);
		} else {
			/* No escape */
			ptr2TxBuf[putChars++] = ch;
		}
	}

	/* End Escape */
	ptr2TxBuf[putChars++] = MSGMARK;

	/* Send notification to Concentrator */
	bytes_sent = write(g_concentrator_fd, ptr2TxBuf, putChars);

	/*
	 * PRINTF(PRINT_INFO, "\r\n*** TCP: %u bytes written (from %u)***\r\n", bytes_sent, putChars);
	 * for(i = 0; i < bytes_sent; i++){
	 *      PRINTF(PRINT_INFO, "%02X:", ptr2TxBuf[i]);
	 * }
	 * PRINTF(PRINT_INFO, "\r\n");
	 */
	return(TRUE);
}

/* ************************************************************************** */

/** @brief	Process reception machine
 *
 **************************************************************************/

void usi_cli_RxProcess(void)
{
	uint16_t i;
	uint8_t ch;
	uint8_t nextfor = 0;
	uint8_t *rxBuf;

	uint16_t rxBufSize;
	uint16_t rxCfgIdx;

	if (g_concentrator_fd == 0) {
		return; /* Nothing to do, socket closed */
	}

	/* Read data from concentrator*/
	ssize_t i_bytes;
	i_bytes = read(g_concentrator_fd, uc_rx_tmp_buffer, MAX_BUFFER_SIZE);
	if (i_bytes > 0) {
		/*
		 * PRINTF(PRINT_INFO, "\r\n*** TCP: %u bytes read ***\r\n", (uint8_t)i_bytes);
		 * for(i = 0; i < i_bytes; i++){
		 *      PRINTF(PRINT_INFO, "%02X:", uc_rx_buffer[i]);
		 * }
		 * PRINTF(PRINT_INFO, "\r\n");
		 */

		rxBuf = uc_rx_buffer;
		rxBufSize = i_bytes;
		/* Check reception on every port */
		for (i = 0; i < i_bytes; i++) {
			ch = uc_rx_tmp_buffer[i];

			/* Process received char */
			switch (usiCliRxParam.rxStat) {
			case RX_IDLE:
				if (ch == 0x7e) {
					/* Start reception process */
					usiCliRxParam.idx = rxCfgIdx =  0;
					usiCliRxParam.rxStat = RX_MSG;
				}

				continue;            /* Do not introduce any character in buffer */

			case RX_MSG:
				if (ch == 0x7d) {
					/* Escape information in message */
					usiCliRxParam.rxStat = RX_ESC;
					continue;
				}

				if (ch == 0x7e) {
					if (rxCfgIdx == 0) {
						/* Two consecutive 0x7E */
						/* The first was ending of a non processed message */
						/* The second is the begining of next message to process */
						continue;
					} else {
						usiCliRxParam.idx = rxCfgIdx;                 /* Must be updated before call _doEoMsg */
						/* End reception process */
						_processMsg(rxCfgIdx);
						/* Restart reception process */
						usiCliRxParam.idx = rxCfgIdx =  0;
						usiCliRxParam.rxStat = RX_IDLE;
						nextfor = TRUE;                   /* end while */
					}
				}

				break;

			case RX_ESC:
				/* Ecape secuence */
				if (ch == 0x7d) {
					rxCfgIdx = 0;
					continue;
				}

				ch ^= 0x20;
				usiCliRxParam.rxStat = RX_MSG;
				break;

			case RX_EORX:
				break;

			default:
				break;
			}         /* switch */

			if (nextfor) {
				break;            /* end while */
			}

			/* Insert in buffer if possible */
			if (rxCfgIdx >= rxBufSize) {                                                            /* Too large */
				rxCfgIdx = 0;
				break;
			}

			*rxBuf++ = ch;
			rxCfgIdx++;

			usiCliRxParam.idx = rxCfgIdx;
		}
	}
}
