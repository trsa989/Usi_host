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

#ifndef __DLMS_O_TCP__
#define __DLMS_O_TCP__

#include <unistd.h>
#include <stdint.h>

#define VERSION_HI      0x01
#define VERSION_LOW 0x01

#define DLMSoTCPPORT 4059
#define MAX_NUM_NODES_CONNECTED 2000
#define CL_432_INVALID_ADDRESS                 (0xFFFF)
#define PIB_432_CONNECTED_NODES_LIST 0x8250
#define SERIAL_NUMBER_432_MAC           16

#define MAX_BUFFER_SIZE 1514

#define MAX_MSG_LIST 5
#define MAX_432_SEND_RETRY 2

typedef struct {
	int i_verbose;
	int i_verbose_level;
	char sz_tty_name[255];
	unsigned ui_baudrate;
	unsigned ui_server_port;
} td_x_args;

struct x_dlms_msg {
	uint32_t ui_timestamp;
	uint16_t us_retries;
	uint16_t us_dst;
	uint16_t us_lsap;
	uint16_t us_length;
	uint8_t data[MAX_BUFFER_SIZE];
	struct x_dlms_msg *px_link;
};

struct x_dlms_msg_list {
	struct x_dlms_msg *px_head;
	struct x_dlms_msg *px_tail;
	uint16_t us_count;
};

int dlmsotcp_init();
int dlmsotcp_process();
void dlmsotcp_close_432();

/* Coordinator address */
#define G3_COORDINATOR_PAN_ID  0x781D

/* Port number for the application */
#define APP_SOCKET_PORT 0xF0B1

/* Generic G3 IPv6 coordinator address */
#define APP_IPV6_GENERIC_COORDINATOR_ADDR "fe80::7455:ff:fe00:0000"

/* Generic G3 IPv6 local-link address */
#define APP_IPV6_GENERIC_LINK_LOCAL_ADDR "fe80::7455:ff:fe00:0001"

/* Default timeout for the application */
#define APP_DEFAULT_TIMEOUT 5000

/* Discovery / NetworkScan timeout in seconds */
#define CONF_SHORT_ADDRESS (const uint8_t *)"\x00\x00"

/* PSK / Network authentication Key (16 bytes) */
#define CONF_PSK_KEY (const uint8_t *)"\xAB\x10\x34\x11\x45\x11\x1B\xC3\xC1\x2D\xE8\xFF\x11\x14\x22\x04"

/* GMK (16 bytes) */
#define CONF_GMK_KEY (const uint8_t *)"\xAF\x4D\x6D\xCC\xF1\x4D\xE7\xC1\xC4\x23\x5E\x6F\xEF\x6C\x15\x1F"

/* Context information table: index 0 (Context 0 with value c_IPv6_PREFIX & x_PAN_ID (length = 80)) */
/* Don't change first byte */
/* 10 bytes */
#define CONF_CONTEXT_INFORMATION_TABLE_0 (const uint8_t *)"\x50" "\xFE\x80\x00\x00\x00\x00\x00\x00\x78\x1D"

/* Context information table: index 1 (Context 1 with value ?112233445566? (length = 48)) */
/* Don't change first byte */
/* 6 bytes */
#define CONF_CONTEXT_INFORMATION_TABLE_1 (const uint8_t *)"\x30" "\x11\x22\x33\x44\x55\x66"

/* Routing table entry TTL (2 bytes, little endian) (5 minutes) */
#define CONF_ROUTING_TABLE_ENTRY_TTL (const uint8_t *)"\x05\x00"

/* Blacklist table entry TTL (2 bytes, little endian) (2 minutes) */
#define CONF_BLACKLIST_TABLE_ENTRY_TTL (const uint8_t *)"\x02\x00"

/* GroupTable: index 0 (2 bytes, little endian) (0x8567 ? note that the IPv6 layer must listen to ff12:30:1122:3344:5566:0:123:4567 in correspondence to this group) */
#define CONF_GROUP_TABLE_0 (const uint8_t *)"\x67\x85"

/* RREQ retries: 5 */
#define CONF_RREQ_RETRIES (const uint8_t *)"\x05"

/* Max Join Time: 90 seconds */
#define CONF_MAX_JOIN_WAIT_TIME (const uint8_t *)"\x5A"

/* Max Hops: 10 */
#define CONF_MAX_HOPS (const uint8_t *)"\x0A"

/* static void g3_Init(void); */
/* static void g3_Process(void); */

#endif /* __DLMS_O_TCP__ */
