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

#include <time.h>
#include <unistd.h>

#include "dlmsotcp.h"
#include "debug.h"

#include "mngLayerHost.h"
#include "prime_api_host.h"
#include "prime_api_defs_host.h"
#include "cl_432_defs.h"

/* Define this depending on Base USI node buffers*/
#define  MNGP_PRIME_ENHANCED_LIST_MAX_RECORDS 30

static struct {
	uint16_t dst_address;
	uint8_t serial_number[SERIAL_NUMBER_432_MAC];
	uint8_t len_serial_number;
	uint8_t mac[6];
	uint8_t autoclose_enabled;
}
x_list_nodes_connected[MAX_NUM_NODES_CONNECTED];

/* Store the maximum index in the list of connected nodes */
static uint16_t us_max_index_connected_node;

/* Enable notifications flag */
static int32_t i_enable_notifications = FALSE;

static dl_432_buffer_t dlms_432_msg;
static unsigned char uc_rx_buffer[MAX_BUFFER_SIZE];

/* Concentrator socket descriptor */
extern int g_concentrator_fd;

static struct x_dlms_msg_list x_free_messages_list;
static struct x_dlms_msg_list x_pending_cfm_messages_list;
/* List of mesages */

static void _dlms_msg_list_init(struct x_dlms_msg_list *px_list)
{
	/*  */
	px_list->px_head = NULL;
	px_list->px_tail = NULL;
	px_list->us_count = 0;
}

static void _dlms_msg_put(struct x_dlms_msg_list *px_list, struct x_dlms_msg *px_buf)
{
	px_buf->px_link = NULL;
	if (px_list->px_tail) {
		px_list->px_tail->px_link = px_buf;
	} else {
		px_list->px_head = px_buf;
	}

	px_list->px_tail = px_buf;
	(px_list->us_count)++;
}

static struct x_dlms_msg *_dlms_msg_get(struct x_dlms_msg_list *px_list)
{
	struct x_dlms_msg *px_buf;

	px_buf = px_list->px_head;
	if (px_buf) {
		px_list->px_head = px_buf->px_link;
		(px_list->us_count)--;
	}

	if (!px_list->px_head) {
		px_list->px_tail = NULL;
	}

	return px_buf;
}

/**
 * \brief Set autoclose flag for a 432-connection
 */
static void _add_device_auto_close(uint16_t us_device)
{
	x_list_nodes_connected[us_device].autoclose_enabled++;
}

/**
 * \brief clear autoclose flag for a 432-connection
 */
static void _remove_device_auto_close(uint16_t us_device)
{
	x_list_nodes_connected[us_device].autoclose_enabled = 0;
}

/**
 * \brief test if a 432-connection has enabled the autoclose flag (DLMSoTCP status)
 */
static int _is_enabled_auto_close(uint16_t us_device)
{
	if ((us_device >= MAX_NUM_NODES_CONNECTED) || (x_list_nodes_connected[us_device].dst_address == CL_432_INVALID_ADDRESS)) {
		return 0;
	} else {
		return (x_list_nodes_connected[us_device].autoclose_enabled > 0) ? 1 : 0;
	}
}

static void _send_queued_432_data()
{
	uint32_t now = time(NULL);
	x_pending_cfm_messages_list.px_head->ui_timestamp =  now;
	x_pending_cfm_messages_list.px_head->us_retries   = MAX_432_SEND_RETRY;

	/* build dl_432_buffer... */
	dl_432_buffer_t msg;
	memcpy(msg.dl.buff, x_pending_cfm_messages_list.px_head->data,
			x_pending_cfm_messages_list.px_head->us_length);
	msg.dl.lsap = x_pending_cfm_messages_list.px_head->us_lsap;

	/* send data */
	uint8_t uc_dst_lsap;
	if (x_pending_cfm_messages_list.px_head->us_dst == 0xFFF) {
		uc_dst_lsap = 0;
	} else {
		uc_dst_lsap = 1;
	}

	prime_cl_432_dl_data_request(uc_dst_lsap, x_pending_cfm_messages_list.px_head->us_lsap,
			x_pending_cfm_messages_list.px_head->us_dst,
			&msg,
			x_pending_cfm_messages_list.px_head->us_length,
			0);
	if (uc_dst_lsap == 1) {
		PRINTF(PRINT_INFO, "SEND QUEUED PACKET: cl_432 send: #%d, lsap %d, dest %d\n",
				x_pending_cfm_messages_list.px_head->us_retries,
				x_pending_cfm_messages_list.px_head->us_lsap,
				x_pending_cfm_messages_list.px_head->us_dst);
	} else {
		PRINTF(PRINT_INFO, "SEND QUEUED BORADCAST PACKET: cl_432 send: #%d, lsap %d, dest %d\n",
				x_pending_cfm_messages_list.px_head->us_retries,
				x_pending_cfm_messages_list.px_head->us_lsap,
				x_pending_cfm_messages_list.px_head->us_dst);
	}
}

void _dlmsotcp_cl_432_dl_data_cfm_cb(uint8_t uc_dst_lsap, uint8_t uc_src_lsap, uint16_t us_dst_address, uint8_t uc_tx_status)
{
	struct x_dlms_msg *px_msg;
	PRINTF(PRINT_INFO, "_dlmsotcp_cl_432_dl_data_cfm_cb");

	if (x_pending_cfm_messages_list.us_count <= 0) {
		/* if this queue is empty, do nothing */
		return;
	}

	/* remove msg from pending_cfm queue */
	px_msg = _dlms_msg_get(&x_pending_cfm_messages_list);

	if (px_msg->us_dst != us_dst_address) {
		PRINTF(PRINT_INFO, "TX CONFIRM DOES NOT MATCH QUEUE Destination:  %d != %d.\n",
				px_msg->us_dst, us_dst_address);
	}

	/* recycle memory: put it back into free msg list */
	px_msg->ui_timestamp = 0;
	px_msg->us_retries = 0;
	px_msg->px_link = NULL;
	_dlms_msg_put(&x_free_messages_list, px_msg);

	switch (uc_tx_status) {
	case 0:
		/* Do nothing, msg  OK*/
		PRINTF(PRINT_INFO, "TX DATA OK: Free %d, Pending %d \r\n", x_free_messages_list.us_count, x_pending_cfm_messages_list.us_count);
		break;

	case CL_432_TX_STATUS_TIMEOUT:
		PRINTF(PRINT_INFO, "TX DATA ERROR CL_432_TX_STATUS_TIMEOUT: dest %d. retry later\n",
				us_dst_address);
		/* Do not retry this message. Concentrator must handle timeouts! */
		break;

	case CL_432_TX_STATUS_ERROR_BAD_ADDRESS:
		PRINTF(PRINT_INFO, "TX DATA ERROR CL_432_TX_STATUS_ERROR_BAD_ADDRESS: dest %d. retry later\n",
				us_dst_address);

		break;

	case CL_432_TX_STATUS_ERROR_BAD_HANLDER:
	{
		PRINTF(PRINT_INFO, "TX DATA ERROR CL_432_TX_STATUS_ERROR_BAD_HANLDER: dest %d. REMOVE MESSAGE\n",
				us_dst_address);
	}
	break;

	case CL_432_TX_STATUS_PREVIOUS_COMM:
		PRINTF(PRINT_INFO, "TX DATA ERROR CL_432_TX_STATUS_PREVIOUS_COMM: dest %d. retry later\n",
				us_dst_address);
		break;

	default:
		PRINTF(PRINT_INFO, "TX DATA ERROR %d: dest %d. retry later\n",
				uc_tx_status, us_dst_address);
		break;
	}

	/* Are there any pending messages? Send! */
	if (x_pending_cfm_messages_list.us_count > 0) {
		_send_queued_432_data();
	}
}

/**
 * \brief Updates connection status and sends the New Device DLMSoTCP message
 *      to the client.
 */
void _dlmsotcp_cl_432_join_ind_cb(uint8_t *puc_device_id, uint8_t uc_device_id_len,
		uint16_t us_dst_address, uint8_t *puc_mac)
{
	uint8_t uc_length;
	uint8_t puc_dlms_msg[512];

	PRINTF(PRINT_INFO, "_dlmsotcp_cl_432_join_ind_cb\n");

	/* Add node to list */
	if (us_dst_address >= us_max_index_connected_node) {
		us_max_index_connected_node = us_dst_address + 1;
	}

	x_list_nodes_connected[us_dst_address].dst_address = us_dst_address;
	uc_length = (uc_device_id_len > 16) ? 16 : uc_device_id_len;
	memcpy(x_list_nodes_connected[us_dst_address].serial_number, puc_device_id, uc_length);
	x_list_nodes_connected[us_dst_address].len_serial_number = uc_length;
	memcpy(x_list_nodes_connected[us_dst_address].mac, puc_mac, 6);

	/* Forward notification to concentrator */
	if (x_list_nodes_connected[us_dst_address].autoclose_enabled == FALSE) {
		if ((g_concentrator_fd != 0) && (i_enable_notifications)) {
			/* DLMSoTCP Version 0x0001 */
			puc_dlms_msg[0] = 0;
			puc_dlms_msg[1] = 1;

			/* CUSTOM MESSAGE Device BN --> DC */
			puc_dlms_msg[2] = 0;
			puc_dlms_msg[3] = 0;

			/*  CUSTOM MESSAGE Association BN --> DC    */
			puc_dlms_msg[4] = 0;
			puc_dlms_msg[5] = 1;

			/* Lenght of the DLMS data field */
			puc_dlms_msg[6] = 0;
			puc_dlms_msg[7] = 12 + uc_length;    /* MAXIMUM 28 = 12 + LEN_432_SERIAL_NUMBER */

			/* NEW_DEVICE_NOTIFICATION (0x01) */
			puc_dlms_msg[8] = 1;

			/* Source DeviceID */
			puc_dlms_msg[9] = us_dst_address >> 8;
			puc_dlms_msg[10] = us_dst_address & 0xFF;    /* Source DeviceID */

			/* Flags with the capabilities of the device--> 0x0001 : ARQ  */
			puc_dlms_msg[11] = 0;
			puc_dlms_msg[12] = 1;

			/* Length of the DLMS identifier (as in ZIV000000). IBERDROLA => 13*/
			puc_dlms_msg[13] = uc_length;

			/* DLMS identifier of the reported device */
			memcpy(puc_dlms_msg + 14, puc_device_id, uc_length);

			/* EUI48 of the reported device */
			memcpy(puc_dlms_msg + 14 + uc_length, puc_mac, 6);

			uc_length = 20 + uc_length;

			/* Send notification to Concentrator */
			write(g_concentrator_fd, puc_dlms_msg, uc_length);
		}
	}
}

void _dlmsotcp_cl_432_leave_ind_cb(uint16_t us_dst_address)
{
	uint8_t uc_length;
	uint8_t puc_dlms_msg[512];

	PRINTF(PRINT_INFO, "_dlmsotcp_cl_432_leave_ind_cb\n");
	/* remove node from list */
	x_list_nodes_connected[us_dst_address].dst_address = CL_432_INVALID_ADDRESS;
	x_list_nodes_connected[us_dst_address].autoclose_enabled = FALSE;

	/* fordward notification */
	if ((g_concentrator_fd != 0) && (i_enable_notifications)) {
		if (!_is_enabled_auto_close(us_dst_address)) {
			/* Version 0x0001 */
			puc_dlms_msg[0] = 0;
			puc_dlms_msg[1] = 1;

			/* CUSTOM MESSAGE Device BN --> DC */
			puc_dlms_msg[2] = 0;
			puc_dlms_msg[3] = 0;

			/* CUSTOM MESSAGE Association BN --> DC    */
			puc_dlms_msg[4] = 0;
			puc_dlms_msg[5] = 1;

			/* Lenght of the DLMS data field */
			puc_dlms_msg[6] = 0;
			puc_dlms_msg[7] = 3;  /* Fixed */

			/* REMOVE_DEVICE_NOTIFICATION (0x02) */
			puc_dlms_msg[8] = 2;

			/* Source DeviceID */
			puc_dlms_msg[9] = (us_dst_address >> 8) & 0xFF;
			puc_dlms_msg[10] = us_dst_address & 0xFF;

			uc_length = 11;

			/* Send notification to Concentrator */
			write(g_concentrator_fd, puc_dlms_msg, uc_length);
		}
	}
}

/**
 * \brief Send the list of 432 nodes to the remote client.
 *      When a new client connection is accepted, it is needed
 *      to notify the current 432 connection status.
 */
static void _send_list_432_connections(void)
{
	uint8_t uc_length;
	uint8_t puc_dlms_msg[512];
	int i = 0;

	if ((g_concentrator_fd != 0) && (i_enable_notifications)) {
		for (i = 0; i < MAX_NUM_NODES_CONNECTED; i++) {
			if (x_list_nodes_connected[i].dst_address != CL_432_INVALID_ADDRESS) {
				/* DLMSoTCP Version 0x0001 */
				puc_dlms_msg[0] = 0;
				puc_dlms_msg[1] = 1;

				/* CUSTOM MESSAGE Device BN --> DC */
				puc_dlms_msg[2] = 0;
				puc_dlms_msg[3] = 0;

				/*  CUSTOM MESSAGE Association BN --> DC    */
				puc_dlms_msg[4] = 0;
				puc_dlms_msg[5] = 1;

				/* Lenght of the DLMS data field */
				puc_dlms_msg[6] = 0;
				puc_dlms_msg[7] = 12 + x_list_nodes_connected[i].len_serial_number;    /* MAXIMUM 28 = 12 + LEN_432_SERIAL_NUMBER */

				/* NEW_DEVICE_NOTIFICATION (0x01) */
				puc_dlms_msg[8] = 1;

				/* Source DeviceID */
				puc_dlms_msg[9] = x_list_nodes_connected[i].dst_address >> 8;
				puc_dlms_msg[10] = x_list_nodes_connected[i].dst_address & 0xFF;

				/* Flags with the capabilities of the device--> 0x0001 : ARQ  */
				puc_dlms_msg[11] = 0;
				puc_dlms_msg[12] = 1;

				/* Length of the DLMS identifier (as in ZIV000000). IBERDROLA => 13*/
				puc_dlms_msg[13] =  x_list_nodes_connected[i].len_serial_number;

				/* DLMS identifier of the reported device */
				memcpy(puc_dlms_msg + 14, x_list_nodes_connected[i].serial_number, x_list_nodes_connected[i].len_serial_number);

				/* EUI48 of the reported device */
				memcpy(puc_dlms_msg + 14 + x_list_nodes_connected[i].len_serial_number, x_list_nodes_connected[i].mac, 6);

				uc_length = 20 + x_list_nodes_connected[i].len_serial_number;

				/* Send notification to Concentrator */
				write(g_concentrator_fd, puc_dlms_msg, uc_length);
			}
		}
	}
}

/**
 * \brief New DLMS data arrived, forward it to the client
 */
void _dlmsotcp_cl_432_dl_data_ind_cb(uint8_t uc_dst_lsap, uint8_t uc_src_lsap, uint16_t us_dst_address,
		uint16_t src_address, uint8_t *puc_data, uint16_t uc_lsdu_len, uint8_t uc_link_class)
{
	uint16_t us_length;
	uint8_t puc_dlms_msg[MAX_LENGTH_432_DATA];

	PRINTF(PRINT_INFO, "\n_dlmsotcp_cl_432_dl_data_ind_cb  SRC=%d, DEST=%d, LEN=%d\n", uc_src_lsap, us_dst_address, uc_lsdu_len);

	(void)(uc_dst_lsap);
	(void)(uc_src_lsap);
	(void)(uc_dst_lsap);
	(void)(uc_lsdu_len);
	(void)(us_dst_address);
	(void)(uc_link_class);

	/* Forward dlms data messages */
	if ((g_concentrator_fd != 0) && (i_enable_notifications)) {
		/* DLMSoTCP Version 0x0001 */
		puc_dlms_msg[0] = 0;
		puc_dlms_msg[1] = 1;

		/* Source DeviceID */
		puc_dlms_msg[2] = src_address >> 8;
		puc_dlms_msg[3] = src_address & 0xFF;

		/* From SPEC: Association ID for the data going BN --> DC  */
		/* Association   */
		puc_dlms_msg[4] = 0;
		puc_dlms_msg[5] = uc_dst_lsap;

		/* Length of the DLMS data field */
		puc_dlms_msg[6] = uc_lsdu_len >> 8;
		puc_dlms_msg[7] = uc_lsdu_len & 0xFF;

		/* DLMS MESSAGE */

		/* If it is an empty message, it will not be forwarded */
		if (uc_lsdu_len > 0) {
			memcpy(puc_dlms_msg + 8, puc_data, uc_lsdu_len);

			/* LENGTH DLMS + Ticket67 headers (8) */
			us_length = uc_lsdu_len + 8;

			/* Send notification to Concentrator */
			write(g_concentrator_fd, puc_dlms_msg, us_length);
		}
	}
}

/**
 * \brief Management protocol callback. Here, only responses to Enhanced list are handled.
 *        Only Atmel private list 0x8250 is needed to be be processed in order to
 *        retrieve information about 432 nodes connected to the base node.
 */
void _dlmsotcp_mngp_rsp_cb(uint8_t *ptrMsg, uint16_t len)
{
	uint16_t us_pib_attrib;
	uint8_t uc_index;
	uint8_t *puc_pib_value;
	uint16_t us_pib_size;
	uint8_t uc_next;
	uint8_t *ptr;
	uint8_t uc_enhanced_pib;
	uint8_t uc_records = 0;
	uint8_t uc_record_len = 0;

	uint16_t us_last_iterator = 0;

	(void)(uc_enhanced_pib);
	(void)(us_pib_size);
	(void)(uc_next);
	(void)(uc_index);

	ptr = ptrMsg;
	/*Pib Attribute (2 bytes) | Index (1 byte) | Pib Value (var) | Next (1 byte) */
	if (len < 4) { /* Error!!! Message too short */
		return;
	}

	if (ptrMsg[0] != MNGP_PRIME_LISTRSP) {
		/* Standard response for a PIB*/
		/* Don't care. We are only interested on 432-connected nodes list (0x8250)*/
	} else {
		int16_t i = 0;
		/* Enhanced PIB */
		uc_enhanced_pib = *ptr++;
		us_pib_attrib   = ((uint16_t)(*ptr++)) << 8;
		us_pib_attrib  += (uint16_t)(*ptr++);
		uc_records      = *ptr++;
		uc_record_len   = *ptr++;
		us_pib_size = len - 5;

		if (us_pib_attrib != PIB_432_CONNECTED_NODES_LIST) {
			return;
		}

		/* Load data */
		us_last_iterator = 0xFFFF;
		puc_pib_value = ptr;

		for (i = 0; i < uc_records; i++) {
			uint16_t us_address = 0;
			us_last_iterator = ((puc_pib_value[0] << 8) + puc_pib_value[1]);
			/* Get one record... */
			us_address = ((puc_pib_value[2] << 8) + puc_pib_value[3]);

			if (us_address >= (0xFFF - 1)) {
				/* Broadcast connection, ignore */
				puc_pib_value += uc_record_len + 2;
				continue;
			}

			x_list_nodes_connected[us_address].dst_address = us_address;
			memcpy(x_list_nodes_connected[us_address].serial_number, puc_pib_value + 4, SERIAL_NUMBER_432_MAC);
			x_list_nodes_connected[us_address].len_serial_number = *(puc_pib_value + 4 + SERIAL_NUMBER_432_MAC);
			memcpy(x_list_nodes_connected[us_address].mac, puc_pib_value + 4 + SERIAL_NUMBER_432_MAC + 1, 6);

			/* Print node*/
			PRINTF(PRINT_INFO, "Address %d, Serial: %s, EUI48: %02X:%02X:%02X:%02X:%02X:%02X\n",
					us_address, x_list_nodes_connected[us_address].serial_number,
					x_list_nodes_connected[us_address].mac[0], x_list_nodes_connected[us_address].mac[1],
					x_list_nodes_connected[us_address].mac[2], x_list_nodes_connected[us_address].mac[3],
					x_list_nodes_connected[us_address].mac[4], x_list_nodes_connected[us_address].mac[5]);

			/* advance buffer pointer to record length plus iterator length */
			puc_pib_value += uc_record_len + 2;
		}

		if ((uc_records == 0) || (uc_records < MNGP_PRIME_ENHANCED_LIST_MAX_RECORDS)) { /* Last element -> push */
			/* End query */
			/* Do nothing */
		} else {
			uint16_t us_bigendian_iterator = 0;
			if (us_last_iterator == 0xFFFF) {
				return;
			}

			if ((us_last_iterator & 0x8000) != 0) {
				return;
			}

			/* Ask for the next iterator.. */
			us_last_iterator++;

			/* reverse interator to allow a direct "memcopy" */
			us_bigendian_iterator  = (us_last_iterator & 0xFF) << 8;
			us_bigendian_iterator += (us_last_iterator >> 8) & 0xFF;

			/* Request next elements on the list */
			mngLay_NewMsg(PROTOCOL_MNGP_PRIME_GETQRY_EN);

			mngLay_AddGetPibListEnQuery(PIB_432_CONNECTED_NODES_LIST,
					MNGP_PRIME_ENHANCED_LIST_MAX_RECORDS,
					(uint8_t *)&us_bigendian_iterator);
			mngLay_SendMsg();
		}
	}
}

void _bmng_network_event_msg_cb(bmng_net_event_t *px_net_event)
{
	if (px_net_event->net_event == BMNG_NET_EVENT_REBOOT) {
		/* Base node has rebooted */
		/* We need to close the current session */
		close(g_concentrator_fd);

		/* Init auto_close status */
		for (int us_index = 0; us_index < MAX_NUM_NODES_CONNECTED; us_index++) {
			x_list_nodes_connected[us_index].dst_address = CL_432_INVALID_ADDRESS;
			x_list_nodes_connected[us_index].autoclose_enabled = 0;
		}
	}
}

/**
 * \brief Initialiazion of the DLMSoTCP application. This application
 * uses 432 callback interface and management protocol.
 */

int dlmsotcp_init()
{
	uint16_t us_index;
	int i;

	/* Init auto_close status */
	for (us_index = 0; us_index < MAX_NUM_NODES_CONNECTED; us_index++) {
		x_list_nodes_connected[us_index].dst_address = CL_432_INVALID_ADDRESS;
		x_list_nodes_connected[us_index].autoclose_enabled = 0;
	}

	/* Init message lists */
	_dlms_msg_list_init(&x_free_messages_list);
	_dlms_msg_list_init(&x_pending_cfm_messages_list);

	/* Reserve memory */
	for (i = 0; i < MAX_MSG_LIST; i++) {
		struct x_dlms_msg *px_buff = (struct x_dlms_msg *)malloc(sizeof(struct x_dlms_msg));
		_dlms_msg_put(&x_free_messages_list, px_buff);
	}

	/* CL NULL Callbacks: NO NEED */

	/* prime_cl_null_callbacks_t st_null_cbs;
	 * st_null_cbs.prime_cl_null_establish_ind_cb = _prime_cl_null_establish_ind_cb;
	 * st_null_cbs.prime_cl_null_establish_cfm_cb = _prime_cl_null_establish_cfm_cb;
	 * st_null_cbs.prime_cl_null_release_ind_cb   = _prime_cl_null_release_ind_cb;
	 * st_null_cbs.prime_cl_null_release_cfm_cb   = _prime_cl_null_release_cfm_cb;
	 * st_null_cbs.prime_cl_null_join_ind_cb  = _prime_cl_null_join_ind_cb;
	 * st_null_cbs.prime_cl_null_join_cfm_cb  = _prime_cl_null_join_cfm_cb;
	 * st_null_cbs.prime_cl_null_leave_ind_cb = _prime_cl_null_leave_ind_cb;
	 * st_null_cbs.prime_cl_null_leave_cfm_cb = _prime_cl_null_leave_cfm_cb;
	 * st_null_cbs.prime_cl_null_data_cfm_cb = _prime_cl_null_data_cfm_cb;
	 * st_null_cbs.prime_cl_null_data_ind_cb = _prime_cl_null_data_ind_cb;
	 * st_null_cbs.prime_cl_null_plme_reset_cfm_cb    = _prime_cl_null_plme_reset_cfm_cb;
	 * st_null_cbs.prime_cl_null_plme_sleep_cfm_cb    = _prime_cl_null_plme_sleep_cfm_cb;
	 * st_null_cbs.prime_cl_null_plme_resume_cfm_cb   = _prime_cl_null_plme_resume_cfm_cb;
	 * st_null_cbs.prime_cl_null_plme_testmode_cfm_cb = _prime_cl_null_plme_testmode_cfm_cb;
	 * st_null_cbs.prime_cl_null_plme_get_cfm_cb      = _prime_cl_null_plme_get_cfm_cb;
	 * st_null_cbs.prime_cl_null_plme_set_cfm_cb      = _prime_cl_null_plme_set_cfm_cb;
	 * st_null_cbs.prime_cl_null_mlme_register_cfm_cb   = _prime_cl_null_mlme_register_cfm_cb;
	 * st_null_cbs.prime_cl_null_mlme_register_ind_cb   = _prime_cl_null_mlme_register_ind_cb;
	 * st_null_cbs.prime_cl_null_mlme_unregister_cfm_cb = _prime_cl_null_mlme_unregister_cfm_cb;
	 * st_null_cbs.prime_cl_null_mlme_unregister_ind_cb = _prime_cl_null_mlme_unregister_ind_cb;
	 * st_null_cbs.prime_cl_null_mlme_promote_cfm_cb    = _prime_cl_null_mlme_promote_cfm_cb;
	 * st_null_cbs.prime_cl_null_mlme_promote_ind_cb    = _prime_cl_null_mlme_promote_ind_cb;
	 * st_null_cbs.prime_cl_null_mlme_demote_cfm_cb     = _prime_cl_null_mlme_demote_cfm_cb;
	 * st_null_cbs.prime_cl_null_mlme_demote_ind_cb     = _prime_cl_null_mlme_demote_ind_cb;
	 * st_null_cbs.prime_cl_null_mlme_reset_cfm_cb      = _prime_cl_null_mlme_reset_cfm_cb;
	 * st_null_cbs.prime_cl_null_mlme_get_cfm_cb        = _prime_cl_null_mlme_get_cfm_cb;
	 * st_null_cbs.prime_cl_null_mlme_list_get_cfm_cb   = _prime_cl_null_mlme_list_get_cfm_cb;
	 * st_null_cbs.prime_cl_null_mlme_set_cfm_cb        = _prime_cl_null_mlme_set_cfm_cb;
	 *
	 * prime_cl_null_set_callbacks(&st_null_cbs);*/

	/* Base Management/FUP callbacks: NO NEED */
	prime_bmng_callbacks_t st_bmng_cbs;

	/*st_bmng_cbs.fup_ack_ind_cb       = _prime_fup_ack_ind_msg_cb;
	 * st_bmng_cbs.fup_error_ind_cb     = _prime_fup_error_ind_msg_cb;
	 * st_bmng_cbs.fup_kill_ind_cb      = _prime_fup_kill_ind_msg_cb;
	 * st_bmng_cbs.fup_status_ind_cb    = _prime_fup_status_ind_msg_cb;
	 * st_bmng_cbs.fup_version_ind_cb   = _prime_fup_version_ind_msg_cb;*/
	st_bmng_cbs.network_event_ind_cb = _bmng_network_event_msg_cb;
	bmng_set_callbacks(&st_bmng_cbs);

	/* Set 432 Callbacks */
	prime_cl_432_callbacks_t st_cl_432_cbs;
	st_cl_432_cbs.prime_cl_432_establish_cfm_cb   = NULL;
	st_cl_432_cbs.prime_cl_432_release_cfm_cb     = NULL;
	st_cl_432_cbs.prime_cl_432_dl_data_cfm_cb     = _dlmsotcp_cl_432_dl_data_cfm_cb;
	st_cl_432_cbs.prime_cl_432_dl_data_ind_cb     = _dlmsotcp_cl_432_dl_data_ind_cb;
	st_cl_432_cbs.prime_cl_432_dl_leave_ind_cb    = _dlmsotcp_cl_432_leave_ind_cb;
	st_cl_432_cbs.prime_cl_432_dl_join_ind_cb     = _dlmsotcp_cl_432_join_ind_cb;
	prime_cl_432_set_callbacks(&st_cl_432_cbs);

	/* PRIME Mng */
	mngp_set_rsp_cb(_dlmsotcp_mngp_rsp_cb);

	/* Embedder sniffer callback. NO NEED*/
	/*prime_sniffer_set_cb(_prime_sniffer_msg_cb);*/

	/* Once that we have the callbacks in place, we query the list of connected nodes.*/
	/* USI must be configured first */
	/* Use Enhanced to query 8250 list (432 connected nodes) */
	uint16_t us_it = 0; /* query first element */
	mngLay_NewMsg(PROTOCOL_MNGP_PRIME_GETQRY_EN);
	mngLay_AddGetPibListEnQuery(PIB_432_CONNECTED_NODES_LIST, MNGP_PRIME_ENHANCED_LIST_MAX_RECORDS, (uint8_t *)&us_it);
	mngLay_SendMsg();

	return 0;
}

/**
 * \brief Process messages received from the concentrator.
 * Unpack dlmsotcp protocol headers and forward data to the Base Node.
 */
void dlmsotcp_RxProcess(uint8_t *buf, uint16_t buflen)
{
	uint16_t usVersion;
	uint16_t usSource;
	uint16_t usDest;
	uint16_t usDlmsLen;
	uint8_t *pcDlmsBuf;

__dlmsMsg:
	if (buflen < 8) {
		return; /* discard, nothing to do? */
	}

	usVersion = (buf[0] << 8) + buf[1];
	usSource  = (buf[2] << 8) + buf[3];
	usDest    = (buf[4] << 8) + buf[5];
	usDlmsLen = (buf[6] << 8) + buf[7];
	pcDlmsBuf = buf + 8;

	if (usVersion != 0x1) {
		PRINTF(PRINT_ERROR, "DLMSoTCP Bad version\n");
		return;
	}

	if (buflen < usDlmsLen + 8 /*header*/) {
		/*mising data*/
		return;
	}

	/* Insert in buffer if possible */
	if (usDlmsLen >= MAX_LENGTH_432_DATA) {
		return;
	}

	if (usDlmsLen > buflen) {
		return; /* not enough data in buffer (fragmented data?) */
	}

	if ((usSource == 1) && (usDest == 0) && (usDlmsLen >= 1)) {
		/* Custom DLMSoTCP message: */
		uint8_t cmd = pcDlmsBuf[0];
		switch (cmd) {
		case 3:
			i_enable_notifications = TRUE;

			/* SEND LIST OF 432-CONNECTED NODES */
			_send_list_432_connections();

			break;

		case 4: /*DELETE*/
			if (usDlmsLen == 3) {
				uint16_t deviceId = (pcDlmsBuf[1] << 8) + pcDlmsBuf[2];
				_remove_device_auto_close(deviceId);
				prime_cl_432_release_request(deviceId);
			}

			break;

		case 5: /*ENABLE AUTO CLOSE */
			if (usDlmsLen == 3) {
				int deviceId = (pcDlmsBuf[1] << 8) + pcDlmsBuf[2];
				_add_device_auto_close(deviceId);
			}

			break;

		case 6: /* DISABLE AUTO CLOSE*/
			if (usDlmsLen == 3) {
				uint16_t deviceId = (pcDlmsBuf[1] << 8) + pcDlmsBuf[2];
				_remove_device_auto_close(deviceId);
			}

			break;

		default:
			break;
		}
	} else {
		/*Forward DMLS messages */

		memcpy(dlms_432_msg.dl.buff, pcDlmsBuf, usDlmsLen);
		dlms_432_msg.dl.lsap = usSource & 0xFF;

		/* Keep track of messages and handle cfm/retries */

		/* get a free msg buffer */
		struct x_dlms_msg *px_msg =  _dlms_msg_get(&x_free_messages_list);
		if (px_msg) {
			uint8_t uc_dst_lsap;

			if (usDest == 0x007F) {
				uc_dst_lsap = 0;
				px_msg->us_dst    = 0xFFF; /* IEC-432 broadcast address */
				px_msg->us_lsap   = 0;
			} else {
				/* fill the message data */
				px_msg->us_dst    = usDest;
				px_msg->us_lsap   = dlms_432_msg.dl.lsap;
				uc_dst_lsap = 1;
			}

			px_msg->us_length = usDlmsLen;
			memcpy(px_msg->data, dlms_432_msg.dl.buff, usDlmsLen);
			px_msg->ui_timestamp = time(NULL);
			px_msg->us_retries   = 0;

			if (x_pending_cfm_messages_list.us_count == 0) {
				/* there is no messages with pending confirm... send*/
				prime_cl_432_dl_data_request(uc_dst_lsap, dlms_432_msg.dl.lsap, usDest, &dlms_432_msg, usDlmsLen, 0);
			}

			/*add message to the queue */
			_dlms_msg_put(&x_pending_cfm_messages_list, px_msg);
		} else {
			PRINTF(PRINT_ERROR, " ERROR, No more free message buffers %d-%d!!!!!\n",
					x_free_messages_list.us_count,
					x_pending_cfm_messages_list.us_count);
			PRINTF(PRINT_ERROR, " Discard message. This should never happen!!!!!\n");
		}

		/* Check pending confirm queue for old messages...*/
		if (x_pending_cfm_messages_list.us_count > 1) {
			uint32_t ui_now = time(NULL);
			uint32_t ui_timestamp = x_pending_cfm_messages_list.px_head->ui_timestamp;
			if ((ui_now - ui_timestamp) > 40) {
				/* Waiting more than 40 seconds for a confirm, delete! */
				/* put it back in free  messages */
				struct x_dlms_msg *px_msg;
				px_msg = _dlms_msg_get(&x_pending_cfm_messages_list);
				/* put it back in free  messages */
				_dlms_msg_put(&x_free_messages_list, px_msg);
				PRINTF(PRINT_ERROR, " ERROR, QUEUE CONFRIM TIMEOUT  Time= %d, Queued= %d!!!!!\n", ui_now - ui_timestamp, x_pending_cfm_messages_list.us_count);

				/* send next message */
				_send_queued_432_data();
			}
		}
	}

	/* more than one DLMS/432 message in this packet? */
	buflen = buflen - 8 - usDlmsLen;
	if (buflen <= 0) {
		return; /* Last message; */
	}

	/* more data, move forward the buffer pointer */
	buf = buf + 8 + usDlmsLen;
	goto __dlmsMsg;
}

int dlmsotcp_process()
{
	if (g_concentrator_fd == 0) {
		return -1; /* Nothing to do, socket closed */
	}

	/* Read data from concentrator*/
	ssize_t i_bytes;
	i_bytes = read(g_concentrator_fd, uc_rx_buffer, MAX_BUFFER_SIZE);
	if (i_bytes > 0) {
		dlmsotcp_RxProcess(uc_rx_buffer, i_bytes);
	}

	return i_bytes;
}

void dlmsotcp_close_432()
{
	uint16_t us_index;

	/* Close 432 connections where auto_close status has been set */
	for (us_index = 0; us_index < MAX_NUM_NODES_CONNECTED; us_index++) {
		if (x_list_nodes_connected[us_index].autoclose_enabled == 1) {
			prime_cl_432_release_request(x_list_nodes_connected[us_index].dst_address);
			/* Update status */
			x_list_nodes_connected[us_index].dst_address = CL_432_INVALID_ADDRESS;
			x_list_nodes_connected[us_index].autoclose_enabled = FALSE;
		}
	}
}
