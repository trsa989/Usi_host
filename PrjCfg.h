/**
 * \file
 *
 * \brief USI Host PRIME Project Config File
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

#ifndef PrimePrjCfgH
#define PrimePrjCfgH

/* ---------------------------------------------------------------------------- */
/* All following lines are commented, */
/* They are an example for USI configuration */
/* must uncomment and put the right values */
/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_PORTS                                                       1
/* #define PORT_0 CONF_PORT(COM_TYPE, 3,230400,0x7FFF, 0x7FFF) */
#define PORT_0 CONF_PORT(UART_TYPE, 5, 230400, 4096, 4096)

#ifdef PRIME_MANAGER
#define NUM_PROTOCOLS                       4
#define USE_MNGP_PRIME_PORT                 0
#define USE_PROTOCOL_SNIF_PRIME_PORT        0
#define USE_PROTOCOL_PRIME_API              0
#define USE_PROTOCOL_PHY_SERIAL_PRIME       0
#else
#define NUM_PROTOCOLS                       6
#define USE_MNGP_PRIME_PORT                 0
#define USE_PROTOCOL_SNIF_PRIME_PORT        0
#define USE_PROTOCOL_PRIME_API              0
#define USE_PROTOCOL_ADP_G3_PORT            0
#define USE_PROTOCOL_COORD_G3_PORT          0
#define USE_PROTOCOL_MAC_G3_PORT            0
#endif

#ifdef __cplusplus
}
#endif

#endif /* PrimePrjCfgH */
