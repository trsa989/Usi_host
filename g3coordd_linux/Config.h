/**
 * \file
 *
 * \brief G3 Coordinator App
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

/* If APP_CONFORMANCE_TEST is defined, the app is configured according to that ETSI test */
/* #define APP_CONFORMANCE_TEST */

/* Generic G3 IPv6 coordinator address */
#define APP_IPV6_GENERIC_COORDINATOR_ADDR "fe80::7455:ff:fe00:0000"

/* Generic G3 IPv6 local-link address */
#define APP_IPV6_GENERIC_LINK_LOCAL_ADDR "fe80::7455:ff:fe00:0001"

#ifdef APP_CONFORMANCE_TEST
#define APP_IPV6_MULTICAST_ADDR_0 "ff02:0:0:0:0:0:0:1"
#define APP_IPV6_MULTICAST_ADDR_1 "ff12:30:1122:3344:5566:0:123:4567"
#endif

/* Port number for the application */
#define APP_SOCKET_PORT 0xF0B1

#ifdef APP_CONFORMANCE_TEST
/* Port number for conformance */
  #define CONFORMANCE_SOCKET_PORT 0xF0BF
/* Conformance Ping */
  #define CONFORMANCE_PING_TTL                 10
#endif

/* Default timeout for the UDP socket */
#define APP_SOCKET_TIMEOUT 5000

/* Coordinator short address */
#define CONF_SHORT_ADDRESS (const uint8_t *)"\x00\x00"

/* PSK / Network authentication Key (16 bytes) */
/* #define CONF_PSK_KEY (const uint8_t *) "\xAB\x10\x34\x11\x45\x11\x1B\xC3\xC1\x2D\xE8\xFF\x11\x14\x22\x04" */
#define CONF_PSK_KEY "\xAB\x10\x34\x11\x45\x11\x1B\xC3\xC1\x2D\xE8\xFF\x11\x14\x22\x04"

/* GMK (16 bytes) */
/* #define CONF_GMK_KEY (const uint8_t *) "\xAF\x4D\x6D\xCC\xF1\x4D\xE7\xC1\xC4\x23\x5E\x6F\xEF\x6C\x15\x1F" */
#define CONF_GMK_KEY "\xAF\x4D\x6D\xCC\xF1\x4D\xE7\xC1\xC4\x23\x5E\x6F\xEF\x6C\x15\x1F"

/* Context information table: index 0 (Context 0 with value c_IPv6_PREFIX & x_PAN_ID (length = 80)) */
#define CONF_CONTEXT_INFORMATION_TABLE_0 (const uint8_t *)"\x02\x00" "\x01" "\x50" "\xFE\x80\x00\x00\x00\x00\x00\x00\x78\x1D"

/* Context information table: index 1 (Context 1 with value �112233445566� (length = 48)) */
#define CONF_CONTEXT_INFORMATION_TABLE_1 (const uint8_t *)"\x02\x00" "\x01" "\x30" "\x11\x22\x33\x44\x55\x66"

#ifdef APP_CONFORMANCE_TEST
/* Routing table entry TTL (2 bytes, little endian) */
  #define CONF_ROUTING_TABLE_ENTRY_TTL (const uint8_t *)"\x05\x00"  /* 5 minutes */

/* Blacklist table entry TTL (2 bytes, little endian) (2 minutes) */
  #define CONF_BLACKLIST_TABLE_ENTRY_TTL (const uint8_t *)"\x02\x00"

/* IDP (EAP's network access identifier) for ARIB */
  #define CONF_ARIB_IDP (const uint8_t *) \
	"\x48\x45\x4D\x53\xA3\xB2\x48\x45\x4D\x53\xA3\xB2\x48\x45\x4D\x53\xA3\xB2\x48\x45\x4D\x53\xA3\xB2\x48\x45\x4D\x53\xA3\xB2\x48\x45\x4D\x53\xA3\xB2"

/* Default routing (LoadNG) disable */
  #define CONF_DISABLE_LOADNG (const uint8_t *)"\x01"

/* GroupTable: index 0 (2 bytes, little endian) (0x8567 � note that the IPv6 layer must listen to ff02::1 in correspondence to this group) */
  #define CONF_GROUP_TABLE_0 (const uint8_t *)"\x01\x80"

/* GroupTable: index 1 (2 bytes, little endian) (0x8567 � note that the IPv6 layer must listen to ff12:30:1122:3344:5566:0:123:4567 in correspondence to this
 * group) */
  #define CONF_GROUP_TABLE_1 (const uint8_t *)"\x67\x85"
  
#ifdef G3_HYBRID_PROFILE
  /* Duty Cycle Limit RF: 360 (10% out of 3600) */
  #define CONF_DUTY_CYCLE_LIMIT_RF (const uint8_t *)"\x68\x01"
  /* Duty Cycle Limit RF: 3600 (100% out of 3600) */
  /* #define CONF_DUTY_CYCLE_LIMIT_RF (const uint8_t *)"\x10\x0E" */
#endif 

/* Max Join Time: 20 seconds */
  #define CONF_MAX_JOIN_WAIT_TIME (const uint8_t *)"\x14\x00"

/* Max Hops: 8 */
  #define CONF_MAX_HOPS (const uint8_t *)"\x08"

  #if (SPEC_COMPLIANCE >= 17)
/* Tone-Map Response TTL (1 byte) (2 minutes) */
    #define CONF_TMR_TTL (const uint8_t *)"\x02"
/* Destination Address Set: index 0 address added (2 bytes, little endian) */
    #define CONF_DEST_ADDR_SET_0 (const uint8_t *)"\xFF\x7F"
  #endif

  #ifdef G3_HYBRID_PROFILE
    /* Max CSMA Backoffs RF: 50 */
    #define CONF_MAX_CSMA_BACKOFFS_RF (const uint8_t *)"\x32"
    /* Max Frame Retries RF: 5 */
    #define CONF_MAX_FRAME_RETRIES_RF (const uint8_t *)"\x05"
    /* POS Table Entry TTL: 3 minutes */
    #define CONF_POS_TABLE_TTL (const uint8_t *)"\x03"
    /* Routing parameters */
    #define CONF_KR (const uint8_t *)"\x00"
    #define CONF_KM (const uint8_t *)"\x00"
    #define CONF_KC (const uint8_t *)"\x00"
    #define CONF_KQ (const uint8_t *)"\x00"
    #define CONF_KH (const uint8_t *)"\x04"
    #define CONF_KRT (const uint8_t *)"\x00"
    #define CONF_KQ_RF (const uint8_t *)"\x00"
    #define CONF_KH_RF (const uint8_t *)"\x08"
    #define CONF_KRT_RF (const uint8_t *)"\x00"
    #define CONF_KDC_RF (const uint8_t *)"\x0A"
  #endif

#else
/* Routing table entry TTL (2 bytes, little endian) */
  #define CONF_ROUTING_TABLE_ENTRY_TTL (const uint8_t *)"\xB4\x00"  /* 180 minutes */

/* Max Join Time: 90 seconds */
  #define CONF_MAX_JOIN_WAIT_TIME (const uint8_t *)"\x5A\x00"

/* Max Hops: 10 */
  #define CONF_MAX_HOPS (const uint8_t *)"\x0A"
#endif

#endif
