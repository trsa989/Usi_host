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

#ifndef APP_ADP_MNG_H_INCLUDED
#define APP_ADP_MNG_H_INCLUDED

#define G3_ADP_MAX_DATA_LENTHG 1280

#define NETWORK_START_TIMEOUT 5


/* Activate APP debug */
#define APP_DEBUG_CONSOLE

#define EXT_ADDR_LEN 8
typedef struct x_node_list {
	uint8_t puc_extended_address[EXT_ADDR_LEN];
	uint16_t us_short_address;
} x_node_list_t;

struct ipv6_prefix {
	uint8_t uc_prefix_len;
	uint8_t uc_on_link_flag;
	uint8_t uc_auto_config_flag;
	uint32_t ui_valid_life_time;
	uint32_t ui_preferred_life_time;
	uint8_t puc_prefix[16];  /* ipv6 addr size */
}
__attribute__((packed));

struct persistent_data {
	uint32_t ui_frame_counter;
	uint16_t us_discover_seq_number;
	uint8_t uc_broadcast_seq_number;
};

void adp_init(uint8_t uc_plc_band, uint16_t us_panid, uint8_t *puc_ipaddr, uint8_t uc_net_len);
void adp_process(uint8_t uc_plc_band, uint16_t us_panid);

bool adp_write_buffers_available();
int  adp_send_ipv6_message(uint8_t *buffer, uint16_t length);

uint16_t app_update_registered_nodes(void *pxNodeList);

void app_show_version( void );
#endif /* APP_ADP_MNG_H_INCLUDED */
