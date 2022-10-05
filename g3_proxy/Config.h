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

#ifndef __CONFIG_H__
#define __CONFIG_H__

/* Discovery / NetworkScan timeout in seconds */
#define CONF_DISCOVERY_TIMEOUT (uint8_t)10u

/* PSK / Network authentication Key (16 bytes) */
#define CONF_PSK_KEY (const uint8_t *)"\xAB\x10\x34\x11\x45\x11\x1B\xC3\xC1\x2D\xE8\xFF\x11\x14\x22\x04"

/* Context information table: index 0 (Context 0 with value c_IPv6_PREFIX & x_PAN_ID (length = 80)) */
#define CONF_CONTEXT_INFORMATION_TABLE_0 (const uint8_t *)"\x02\x00" "\x01" "\x50" "\xFE\x80\x00\x00\x00\x00\x00\x00\x78\x1D"

/* Context information table: index 1 (Context 1 with value �112233445566� (length = 48)) */
#define CONF_CONTEXT_INFORMATION_TABLE_1 (const uint8_t *)"\x02\x00" "\x01" "\x30" "\x11\x22\x33\x44\x55\x66"

/* Routing table entry TTL (2 bytes, little endian) (5 minutes) */
#define CONF_ROUTING_TABLE_ENTRY_TTL (const uint8_t *)"\x05\x00"

/* Blacklist table entry TTL (2 bytes, little endian) (2 minutes) */
#define CONF_BLACKLIST_TABLE_ENTRY_TTL (const uint8_t *)"\x02\x00"

/* GroupTable: index 0 (2 bytes, little endian) (0x8567 � note that the IPv6 layer must listen to ff12:30:1122:3344:5566:0:123:4567 in correspondence to this
 * group) */
#define CONF_GROUP_TABLE_0 (const uint8_t *)"\x67\x85"

/* Define spec compiance, select one: 17 or 15 (spec'17 or spec'15) */
#define SPEC_COMPLIANCE  17
/* #define SPEC_COMPLIANCE  15 */

#endif
