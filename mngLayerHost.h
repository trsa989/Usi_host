/**
 * \file
 *
 * \brief USI Host PRIME Management Layer API
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


#ifndef mngIfaceh
#define mngIfaceh

#ifdef __cplusplus
extern "C" {
#endif

#include "addUsi.h"

/* *** Declarations ********************************************************** */

/* / Protocol ID to serialize (USI) */
#define MNGP_PRIME                                      0x00
#define MNGP_PRIME_GETQRY                       0x00
#define MNGP_PRIME_GETRSP                       0x01
#define MNGP_PRIME_SET                          0x02
#define MNGP_PRIME_RESET                        0x03
#define MNGP_PRIME_REBOOT                       0x04
#define MNGP_PRIME_FU                           0x05
#define PROTOCOL_MNGP_PRIME_GETQRY_EN           0x06
#define PROTOCOL_MNGP_PRIME_GETRSP_EN           0x07

/* Item Enhanced header id */
#define MNGP_PRIME_LISTQRY                      0x0E
#define MNGP_PRIME_LISTRSP                      0x0F

/* *** Functions prototypes ************************************************** */

void mngLay_NewMsg(uint8_t cmd);

uint8_t mngLay_AddGetPibQuery(uint16_t pib, uint8_t index);

uint8_t mngLay_AddSetPib(uint16_t pib, uint16_t length, uint8_t *msg);

uint8_t mngLay_AddGetPibListEnQuery(uint16_t pib, uint8_t maxRecords, uint8_t *iterator);

uint8_t mngLay_AddResetStats(uint16_t pib, uint8_t index);

uint8_t mngLay_AddFUMsg(uint16_t length, uint8_t *msg);

uint8_t mngLay_BridgeMsg(uint16_t length, uint8_t *msg);

uint8_t mngLay_SendMsg(void);

void mngp_set_rsp_cb(void (*sap_handler)(uint8_t *ptrMsg, uint16_t len));

/*Received command function. It calls the appropiate callback*/
uint8_t mngLay_receivedCmd(uint8_t *ptrMsg, uint16_t len);

/* MNGP callback type definition */

/**
 * MNGP Response
 *
 * - msg:      Pointer to message
 * - len:      Message Length
 */
typedef void (*mngp_rsp_cb_t)(uint8_t *ptrMsg, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
