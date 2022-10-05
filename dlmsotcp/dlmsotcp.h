/**
 * \file
 *
 * \brief USI Host - DLMS Over TCP
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

#define TRUE  1
#define FALSE 0

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

#endif /* __DLMS_O_TCP__ */
