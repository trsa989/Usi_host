/**
 * \file
 *
 * \brief USI Host Interface to PRIME Sniffer
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

#ifndef IFACEPRIMESNIFFER_H
#define IFACEPRIMESNIFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define SNIFFER_IF_PHY_COMMAND_SET_CHANNEL               2     /* SET PLC channel (1 = CENELEC- A) */

void prime_sniffer_set_cb(void (*sap_handler)(uint8_t *msg, uint16_t len));
void prime_sniffer_set_channel(uint8_t uc_channel);

/*Received command function. It calls the appropiate callback*/
uint8_t prime_sniffer_receivedCmd(uint8_t *ptrMsg, uint16_t len);

/* Sniffer callback type definition */

/**
 * Sniffer message indication
 *
 * - msg:      Pointer to message
 * - len:      Message Length
 */
typedef void (*prime_sniffer_msg_cb_t)(uint8_t *ptrMsg, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* IFACEPRIMESNIFFER_H */
