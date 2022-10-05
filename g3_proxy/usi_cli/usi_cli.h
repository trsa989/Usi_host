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

#ifndef USICLI_H
#define USICLI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* *** Declarations ********************************************************** */
#define MAX_BUFFER_SIZE 1514

#define MSGMARK         0x7e                    /* Start / End marks in message */
#define ESCMARK         0x7d                    /* Escape mark in message */

#define HEADER_LEN      2
#define CRC8_LEN        1
#define CRC16_LEN       2
#define CRC32_LEN       4

/****************************************************************************
**               PROTOCOL INFORMATION FORMAT                                       **
****************************************************************************/

#define TYPE_PROTOCOL_OFFSET            1
#define TYPE_PROTOCOL_MSK                       0x3F

#define LEN_PROTOCOL_HI_OFFSET          0
#define LEN_PROTOCOL_HI_MSK                     0xFF
#define LEN_PROTOCOL_HI_SHIFT           2

#define LEN_PROTOCOL_LO_OFFSET          1
#define LEN_PROTOCOL_LO_MSK                     0xC0
#define LEN_PROTOCOL_LO_SHIFT           6

#define XLEN_PROTOCOL_OFFSET            2
#define XLEN_PROTOCOL_MSK                       0x80
#define XLEN_PROTOCOL_SHIFT_L           3
#define XLEN_PROTOCOL_SHIFT_R           10

#define PAYLOAD_OFFSET                          2

#define CMD_PROTOCOL_OFFSET                     2
#define CMD_PROTOCOL_MSK                        0x7F

/****************************************************************************
**               MACRO Operators for PROTOCOL pkt                          **
****************************************************************************/

#define TYPE_PROTOCOL(A)                        ((A)&TYPE_PROTOCOL_MSK)

/* A: HI, B: LO, C:Xlen */
#define LEN_PROTOCOL(A, B)                       ((((uint16_t)(A)) << LEN_PROTOCOL_HI_SHIFT) + ((B) >> LEN_PROTOCOL_LO_SHIFT))
#define XLEN_PROTOCOL(A, B, C)            ((((uint16_t)(A)) << LEN_PROTOCOL_HI_SHIFT) + ((B) >> LEN_PROTOCOL_LO_SHIFT) + (((uint16_t)(C)&XLEN_PROTOCOL_MSK) << XLEN_PROTOCOL_SHIFT_L))

#define LEN_HI_PROTOCOL(A)                      (((uint16_t)(A) >> LEN_PROTOCOL_HI_SHIFT) & LEN_PROTOCOL_HI_MSK)
#define LEN_LO_PROTOCOL(A)                      (((uint16_t)(A) << LEN_PROTOCOL_LO_SHIFT) & LEN_PROTOCOL_LO_MSK)
/* #define LEN_EX_PROTOCOL(A)			(((uint16_t)(A)>>XLEN_PROTOCOL_SHIFT_R)&XLEN_PROTOCOL_MSK) */
#define LEN_EX_PROTOCOL(A)                      (((uint16_t)(A & 0x0400)) >> 3)

#define CMD_PROTOCOL(A)                         ((A)&CMD_PROTOCOL_MSK)

/* *** Structures ************************************************************ */
typedef struct {
	uint8_t uc_protocol_type;       /* Protocol Type */
	uint8_t *ptr_buf;               /* Ptr to data buffer */
	uint16_t us_len;                /* Length of data */
} x_usi_serial_cmd_params_t;

/* *** Functions prototypes ************************************************** */

/* Serial Profile */
void usi_cli_Init(void);
void usi_cli_Start(void);

int usi_cli_tcp_process(void);

void usi_cli_RxProcess(void);
void usi_cli_TxProcess(void);
uint8_t usi_cli_send_cmd(x_usi_serial_cmd_params_t *msg);
void usi_cli_Flush(void);
void usi_cli_ConfigurePort(uint8_t logPort, uint8_t port_type, uint8_t commPort, uint32_t speed);

#ifdef __cplusplus
}
#endif

#endif
