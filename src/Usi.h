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

#ifndef SerialProfH
#define SerialProfH

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* *** Declarations ********************************************************** */

#define USI_LOG_LEVEL_ERR   3
#define USI_LOG_LEVEL_INFO  2
#define USI_LOG_LEVEL_DEBUG 1
#define USI_LOG_LEVEL USI_LOG_LEVEL_ERR

#ifdef USI_LOG_LEVEL
#define LOG_USI_DEBUG(...) if (USI_LOG_LEVEL >= USI_LOG_LEVEL_DEBUG) {fprintf(stdout, "[USI]:"); } \
	fprintf(stdout, __VA_ARGS__); fflush(stdout)
#define LOG_USI_INFO(...)  if (USI_LOG_LEVEL >= USI_LOG_LEVEL_INFO) {fprintf(stdout, "[USI]:"); } \
	fprintf(stdout, __VA_ARGS__); fflush(stdout)
#define LOG_USI_ERR(...)   if (USI_LOG_LEVEL >= USI_LOG_LEVEL_ERR) {fprintf(stdout, "[USI]:"); } \
	fprintf(stdout, __VA_ARGS__); fflush(stdout)
#else
#define LOG_USI_DEBUG(...)
#define LOG_USI_INFO(...)
#define LOG_USI_ERR(...)
#endif

#define MSGMARK         0x7e                    /* Start / End marks in message */
#define ESCMARK         0x7d                    /* Escape mark in message */

#define HEADER_LEN      2
#define CRC8_LEN        1
#define CRC16_LEN       2
#define CRC32_LEN       4

#define TRUE  1
#define FALSE 0

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
	uint8_t pType;          /* Protocol Type */
	uint8_t *buf;                   /* Ptr to data buffer */
	uint16_t len;                   /* Length of data */
} CmdParams;

/* *** Functions prototypes ************************************************** */

/* Serial Profile */
void usi_Init(void);
void usi_Start(void);
void usi_RxProcess(void);
void usi_TxProcess(void);
uint8_t usi_SendCmd(CmdParams *msg);
void usi_Flush(void);
void usi_ConfigurePort(uint8_t logPort, uint8_t port_type, uint8_t commPort, uint32_t speed);

#ifdef __cplusplus
}
#endif

#endif
