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

#ifndef USICFGCLI_H
#define USICFGCLI_H

#include "addUsi.h"

/* Types to characterize a port as UART, USART or COM */
#define UART_TYPE       0
#define USART_TYPE      1
#define COM_TYPE    2

#ifdef __cplusplus
extern "C" {
#endif

/* *** Declarations ********************************************************** */

/****************************************************************************
**                                PROTOCOL TYPES                                                                **
****************************************************************************/
/* PRIME spec.v.1.3.E */
#define MNGP_PRIME                    0x00
#define MNGP_PRIME_GETQRY             0x00
#define MNGP_PRIME_GETRSP             0x01
#define MNGP_PRIME_SET                0x02
#define MNGP_PRIME_RESET              0x03
#define MNGP_PRIME_REBOOT             0x04
#define MNGP_PRIME_FU                 0x05
#define PROTOCOL_MNGP_PRIME_GETQRY_EN 0x06
#define PROTOCOL_MNGP_PRIME_GETRSP_EN 0x07

/* Atmel serialized protocols */
#define PROTOCOL_SNIF_PRIME         0x13
#define PROTOCOL_MAC_PRIME          0x17
#define PROTOCOL_MLME_PRIME         0x18
#define PROTOCOL_PLME_PRIME         0x19
#define PROTOCOL_432_PRIME          0x1A
#define PROTOCOL_BASEMNG_PRIME      0x1D
#define PROTOCOL_PHY_SERIAL_PRIME   0x1F
#define PROTOCOL_PHY_TESTER         0x22
#define PROTOCOL_SNIF_G3            0x23
#define PROTOCOL_MAC_G3             0x24
#define PROTOCOL_ADP_G3             0x25
#define PROTOCOL_COORD_G3           0x26
#define PROTOCOL_PRIME_API          0x30
#define PROTOCOL_INTERNAL           0x3E
#define PROTOCOL_USER_DEFINED       0xFE
#define PROTOCOL_INVALID            0xFF

/* NOTE: ID 0x3F is reserved for internal messages, do not use it as identifier */

/* *************************************************************************** */
/* *** Types for Function Pointers ******************************************* */
/* / Type for callback function pointers */
typedef uint8_t (*usi_decode_cmd_cb)(uint8_t *puc_msg, uint16_t us_msg_len);
typedef uint8_t (*const Pt2DecodeCmd)(Uint8 *, Uint16);

/* *** Structures ************************************************************ */
typedef struct {
	uint8_t pType;      /* Protocol Type */
	uint8_t port;                   /* Communication Port */
} MapProtocols;

typedef struct {
	uint8_t sType;      /* Serial Communication Type */
	uint8_t chn;                    /* Port Channel (number) */
	uint32_t speed;     /* Port Bauds */
} MapPorts;

typedef struct {
	uint16_t size;      /* Size of buffer */
	uint8_t *const buf;     /* Ptr to buffer */
} MapBuffers;

typedef struct {
	uint8_t rxStat;                                         /* /< Reception status */
	uint8_t rcvPktReady;                                    /* /< Complete received packet flag */
	uint16_t idx;                                                   /* /< Index where next received char is to be stored */
} RxParam;

typedef struct {
	uint16_t idxIn;                                         /* /< Index to insert Chars */
	uint16_t idxOut;                                        /* /< Index to transmit chars */
	uint16_t count;                                         /* /< Number of chars in buffer */
} TxParam;

#ifdef __cplusplus
}
#endif

#endif
