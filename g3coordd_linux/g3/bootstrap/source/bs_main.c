/**
 * \file
 *
 * \brief G3 Coordinator Module
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

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <AdpApiTypes.h>
#include <AdpApi.h>
#include <mac_wrapper.h>
#include <oss_if.h>

#include "bs_api.h"
#include "bs_functions.h"
#include "conf_bs.h"

int g_lbs_join_finished = 0;

uint16_t us_rekey_idx = 0;
uint16_t us_rekey_phase = LBP_REKEYING_PHASE_DISTRIBUTE;

/* Buffer and length for non-bootstrap messages (i.e., KICK, etc...) */
uint8_t g_puc_data[100];
uint16_t g_us_length;

static TBootstrapAdpNotifications ss_notifications;

extern lbds_list_entry_t g_lbds_list[MAX_LBDS];

static pf_app_leave_ind_cb_t pf_app_leave_ind_cb;
static pf_app_join_ind_cb_t pf_app_join_ind_cb;

static void _set_keying_table(uint8_t u8KeyIndex, uint8_t *key)
{
	AdpMacSetRequest(MAC_WRP_PIB_KEY_TABLE, u8KeyIndex, 16, key);
	AdpSetRequest(ADP_IB_ACTIVE_KEY_INDEX, 0, sizeof(u8KeyIndex), &u8KeyIndex);
	SetKeyIndex(u8KeyIndex);
}

static void _activate_new_key(void)
{
	uint8_t u8NewKeyIndex;

	/* Get current key index and set the new one to the other */
	if (GetKeyIndex() == 0) {
		u8NewKeyIndex = 1;
	} else {
		u8NewKeyIndex = 0;
	}

	/* Set GMK from Rekey GMK */
	set_gmk((uint8_t *)GetRekeyGMK());
	/* Set key table using the new index and new GMK */
	_set_keying_table(u8NewKeyIndex, (uint8_t *)GetGMK());
}

static void _rekeying_process(void)
{
	struct TAddress dstAddr;
	struct TAdpExtendedAddress x_ext_address;
	struct TAdpGetConfirm getConfirm;
	uint8_t uc_handle;

	memcpy(x_ext_address.m_au8Value, g_lbds_list[us_rekey_idx].puc_extended_address, ADP_ADDRESS_64BITS);

	t_bootstrap_slot *p_bs_slot = get_bootstrap_slot_by_addr( g_lbds_list[us_rekey_idx].puc_extended_address);

	if (p_bs_slot) {
		initialize_bootstrap_message(p_bs_slot);
#ifdef G3_HYBRID_PROFILE
		/* DISABLE_BACKUP_FLAG and MediaType set to 0x0 in Rekeying frames */
		p_bs_slot->m_u8DisableBackupMedium = 0;
		p_bs_slot->m_u8MediaType = 0;
#endif
		/* GMK distribution phase */
		if (us_rekey_phase == LBP_REKEYING_PHASE_DISTRIBUTE) {
			/* If re-keying in GMK distribution phase */
			/* Send ADPM-LBP.Request(EAPReq(mes1)) to each registered device */
			Process_Joining0(x_ext_address, p_bs_slot);
			p_bs_slot->e_state = BS_STATE_SENT_EAP_MSG_1;
			memcpy(p_bs_slot->m_LbdAddress.m_au8Value, g_lbds_list[us_rekey_idx].puc_extended_address, ADP_ADDRESS_64BITS);
		} else { /* GMK activation phase (LBP_REKEYING_PHASE_ACTIVATE) */
			process_accepted_GMK_activation(x_ext_address, p_bs_slot);
			p_bs_slot->e_state = BS_STATE_SENT_EAP_MSG_ACCEPTED;
		}

		/* Send the previously prepared message */
		dstAddr.m_u8AddrLength = 2;
		/* The short address is calculated using the index and the initial short address */
		dstAddr.m_u16ShortAddr = us_rekey_idx + get_initial_short_address();

		AdpGetRequestSync(ADP_IB_MAX_HOPS, 0, &getConfirm);
		/* AdpGetRequest(ADP_IB_MAX_HOPS, 0); */
		if (p_bs_slot->uc_pending_confirms > 0) {
			p_bs_slot->uc_pending_tx_handler = p_bs_slot->uc_tx_handle;
		}

		uc_handle = get_next_nsdu_handler();
		p_bs_slot->ul_timeout = oss_get_up_time_ms() + 1000 * get_msg_timeout_value() * 10;
		p_bs_slot->uc_tx_handle = uc_handle;
		p_bs_slot->uc_tx_attemps = 0;
		p_bs_slot->uc_pending_confirms++;
		LOG_BOOTSTRAP(("[BS] AdpLbpRequest Called, handler: %d \r\n", p_bs_slot->uc_tx_handle));
		AdpLbpRequest((struct TAdpAddress const *)&dstAddr,      /* Destination address */
				p_bs_slot->us_data_length,                                /* NSDU length */
				&p_bs_slot->auc_data[0],                                  /* NSDU */
				uc_handle,                            /* NSDU handle */
				getConfirm.m_au8AttributeValue[0],           /* Max. Hops */
				true,                                        /* Discover route */
				0,                                           /* QoS */
				false);                                      /* Security enable */
	}
}

static void AdpNotification_LbpIndication(struct TAdpLbpIndication *pLbpIndication)
{
	enum lbp_indications indication = LBS_NONE;

	LOG_BOOTSTRAP(("[BS] pLbpIndication->m_u16NsduLength %hu.\r\n", pLbpIndication->m_u16NsduLength));
	LOG_BOOTSTRAP(("[BS] TAdpLbpIndication: SrcAddr: 0x%04X LinkQualityIndicator: %hu SecurityEnabled: %hu NsduLength: %hu.\r\n",
			pLbpIndication->m_u16SrcAddr, pLbpIndication->m_u8LinkQualityIndicator,
			pLbpIndication->m_bSecurityEnabled, pLbpIndication->m_u16NsduLength));

	indication = ProcessLBPMessage(pLbpIndication);

  LOG_BOOTSTRAP(("[BS]ProcessLBPMessage ends\n"));

	/* Upper layer indications */
	if (indication == LBS_KICK) {
		if (pf_app_leave_ind_cb != NULL) {
			  pf_app_leave_ind_cb(pLbpIndication->m_u16SrcAddr, pLbpIndication->m_bSecurityEnabled, pLbpIndication->m_u8LinkQualityIndicator,
					pLbpIndication->m_pNsdu, pLbpIndication->m_u16NsduLength);
		}

		/* Remove the device from the joined devices list */
		remove_lbds_list_entry(pLbpIndication->m_u16SrcAddr);
	}

	LOG_BOOTSTRAP(("[BS] pLbpIndication ends\n"));
}

static void AdpNotification_LbpConfirm(struct TAdpLbpConfirm *pLbpConfirm)
{
	uint8_t uc_i;

	t_bootstrap_slot *p_current_slot = NULL;

	bool b_is_accepted_confirm = false;

	for (uc_i = 0; uc_i < BOOTSTRAP_NUM_SLOTS; uc_i++) {
		t_bootstrap_slot *p_slot = get_bootstrap_slot_by_index(uc_i);

		if (p_slot->uc_pending_confirms == 1 && pLbpConfirm->m_u8NsduHandle == p_slot->uc_tx_handle && p_slot->e_state != BS_STATE_WAITING_JOINNING) {
			LOG_BOOTSTRAP(("[BS] AdpNotification_LbpConfirm (%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X).\r\n",
					p_slot->m_LbdAddress.m_au8Value[0], p_slot->m_LbdAddress.m_au8Value[1],
					p_slot->m_LbdAddress.m_au8Value[2], p_slot->m_LbdAddress.m_au8Value[3],
					p_slot->m_LbdAddress.m_au8Value[4], p_slot->m_LbdAddress.m_au8Value[5],
					p_slot->m_LbdAddress.m_au8Value[6], p_slot->m_LbdAddress.m_au8Value[7]));

			p_current_slot = p_slot;
			p_slot->uc_pending_confirms--;
			if (pLbpConfirm->m_u8Status == G3_SUCCESS) {
				switch (p_slot->e_state) {
				case BS_STATE_SENT_EAP_MSG_1:
					p_slot->e_state = BS_STATE_WAITING_EAP_MSG_2;
					LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_WAITING_EAP_MSG_2\r\n"));
					log_show_slots_status();
					break;

				case BS_STATE_SENT_EAP_MSG_3:
					p_slot->e_state = BS_STATE_WAITING_EAP_MSG_4;
					LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_WAITING_EAP_MSG_4\r\n"));
					log_show_slots_status();
					break;

				case BS_STATE_SENT_EAP_MSG_ACCEPTED:
					p_slot->e_state = BS_STATE_WAITING_JOINNING;
					p_slot->ul_nonce =  0;
					LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_WAITING_JOINNING (MSG Accepted)\r\n"));
					log_show_slots_status();
					b_is_accepted_confirm = true;
					break;

				case BS_STATE_SENT_EAP_MSG_DECLINED:
					p_slot->e_state = BS_STATE_WAITING_JOINNING;
					p_slot->ul_nonce =  0;
					LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_WAITING_JOINNING (MSG Declined)\r\n"));
					log_show_slots_status();
					break;

				default:
					p_slot->e_state = BS_STATE_WAITING_JOINNING;
					p_slot->ul_nonce =  0;
					LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_WAITING_JOINNING (Default)\r\n"));
					log_show_slots_status();
					break;
				}
			} else {
				p_slot->e_state = BS_STATE_WAITING_JOINNING;
				p_slot->ul_nonce =  0;
				LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_WAITING_JOINNING (LBP Status Error 0x%02X)\r\n",pLbpConfirm->m_u8Status));
				log_show_slots_status();
			}
		//Check if confirm received is for first request (uc_pending_tx_handler)
		} else if (p_slot->uc_pending_confirms == 2 && pLbpConfirm->m_u8NsduHandle == p_slot->uc_pending_tx_handler) {
			LOG_BOOTSTRAP(("[BS] AdpNotification_LbpConfirm (%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X).\r\n",
					p_slot->m_LbdAddress.m_au8Value[0], p_slot->m_LbdAddress.m_au8Value[1],
					p_slot->m_LbdAddress.m_au8Value[2], p_slot->m_LbdAddress.m_au8Value[3],
					p_slot->m_LbdAddress.m_au8Value[4], p_slot->m_LbdAddress.m_au8Value[5],
					p_slot->m_LbdAddress.m_au8Value[6], p_slot->m_LbdAddress.m_au8Value[7]));
			p_slot->uc_pending_confirms--;
			log_show_slots_status();
			p_current_slot = p_slot;
		//Check if confirm received is for last request (uc_pending_tx_handler)
		} else if (p_slot->uc_pending_confirms == 2 && pLbpConfirm->m_u8NsduHandle == p_slot->uc_tx_handle) {
			LOG_BOOTSTRAP(("[BS] AdpNotification_LbpConfirm (%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X).\r\n",
					p_slot->m_LbdAddress.m_au8Value[0], p_slot->m_LbdAddress.m_au8Value[1],
					p_slot->m_LbdAddress.m_au8Value[2], p_slot->m_LbdAddress.m_au8Value[3],
					p_slot->m_LbdAddress.m_au8Value[4], p_slot->m_LbdAddress.m_au8Value[5],
					p_slot->m_LbdAddress.m_au8Value[6], p_slot->m_LbdAddress.m_au8Value[7]));
			p_slot->uc_pending_confirms--;
			p_slot->uc_tx_handle = p_slot->uc_pending_tx_handler;
			log_show_slots_status();
			p_current_slot = p_slot;
		}
	}

	if (!p_current_slot) {
		LOG_BOOTSTRAP(("[BS] AdpNotification_LbpConfirm from unkown node, status: %d  handler: %d \r\n",
				pLbpConfirm->m_u8Status, pLbpConfirm->m_u8NsduHandle));
		log_show_slots_status();
		return;
	} else {
		p_current_slot->ul_timeout = oss_get_up_time_ms() + 1000 * get_msg_timeout_value();
	}

	if (pLbpConfirm->m_u8Status == G3_SUCCESS && b_is_accepted_confirm) {
		if (lbp_get_rekeying()) {
			/* If a re-keying phase is in progress */
			if (us_rekey_idx < bs_lbp_get_lbds_counter() - 1) {
				us_rekey_idx++;
				/* Continue rekeying process with next node */
				_rekeying_process();
			} else {
				if (us_rekey_phase == LBP_REKEYING_PHASE_DISTRIBUTE) {
					/* All devices have been provided with the new GMK -> next phase */
					us_rekey_phase = LBP_REKEYING_PHASE_ACTIVATE;
					/* Reset the index to send GMK activation to all joined devices. */
					us_rekey_idx = 0;
					/* Call process to start activation phase */
					_rekeying_process();
				} else {
					/* End of re-keying process */
					us_rekey_idx = 0;
					lbp_set_rekeying(LBP_REKEYING_OFF);
					_activate_new_key();
				}
			}
		} else {
			uint8_t *puc_ext_addr;
			uint16_t us_short_addr;

			/* Normal bootstrap */
			g_lbs_join_finished = 1;

			/* After the join process finishes, the entry is added to the list */
			puc_ext_addr = p_current_slot->m_LbdAddress.m_au8Value;
			us_short_addr = p_current_slot->us_assigned_short_address;
			add_lbds_list_entry(puc_ext_addr, us_short_addr, p_current_slot->uc_lbp_hops);
#ifdef BS_LEVEL_STRATEGY_ENABLED
			set_level_strategy_timeout(oss_get_up_time_ms() + 1000 * BS_LEVEL_STRATEGY_TIMEOUT_INTERVAL);
#endif
			/* Upper layer indications */
			if (pf_app_join_ind_cb != NULL) {
				pf_app_join_ind_cb(puc_ext_addr, us_short_addr);
			}
		}
	}
}

/**
 * bs_init.
 *
 */
void bs_init(TBootstrapConfiguration s_bs_conf)
{
	lbp_init_functions();

	set_bs_configuration(s_bs_conf);

	_set_keying_table(INITIAL_KEY_INDEX, (uint8_t *)GetGMK());

	/* Init function pointers */
	pf_app_leave_ind_cb = NULL;
	pf_app_join_ind_cb = NULL;
}

/**
 * bs_process
 *
 */
void bs_process(void)
{
	update_bootstrap_slots();
}

/**
 * bs_get_not_handlers.
 *
 */
TBootstrapAdpNotifications *bs_get_not_handlers(void)
{
	/* Set Notification pointers */
	ss_notifications.fnctAdpLbpConfirm = AdpNotification_LbpConfirm;
	ss_notifications.fnctAdpLbpIndication = AdpNotification_LbpIndication;

	return &ss_notifications;
}

/**
 * bs_lbp_launch_rekeying.
 *
 */
void bs_lbp_launch_rekeying(void)
{
	/* If there are devices that joined the network */
	if (bs_lbp_get_lbds_counter() > 0) {
		/* Start the re-keying process */
		lbp_set_rekeying(LBP_REKEYING_ON);
		us_rekey_phase = LBP_REKEYING_PHASE_DISTRIBUTE;
		/* Send the first re-keying process */
		us_rekey_idx = 0;
		_rekeying_process();
		LOG_BOOTSTRAP(("[BS] Re-keying launched.\r\n"));
	} else {
		/* Error: no device in the network */
		LOG_BOOTSTRAP(("[BS] Re-keying NOT launched: no device in the network.\r\n"));
	}
}

/**
 * bs_lbp_kick_device.
 *
 */
void bs_lbp_kick_device(uint16_t us_short_address)
{
	/* Check if the device had joined the network */
	if (device_is_in_list(us_short_address)) {
		struct TAdpAddress dstAddr;
		struct TAdpGetConfirm getConfirm;
		uint8_t puc_extended_address[ADP_ADDRESS_64BITS];

		/* Send KICK to the device */
		dstAddr.m_u8AddrSize = 2;
		dstAddr.m_u16ShortAddr = us_short_address;

		bs_get_ext_addr_by_short(us_short_address, puc_extended_address);

		g_us_length = Encode_kick_to_LBD(puc_extended_address, sizeof(g_puc_data), g_puc_data);

		/* If message was properly encoded, send it */
		if (g_us_length) {
			AdpGetRequestSync(ADP_IB_MAX_HOPS, 0, &getConfirm);
			/* AdpGetRequest(ADP_IB_MAX_HOPS, 0); */
			AdpLbpRequest((struct TAdpAddress const *)&dstAddr,     /* Destination address */
					g_us_length,                            /* NSDU length */
					g_puc_data,                             /* NSDU */
					get_next_nsdu_handler(),                           /* NSDU handle */
					getConfirm.m_au8AttributeValue[0],      /* Max. Hops */
					true,                                   /* Discover route */
					0,                                      /* QoS */
					false);                                 /* Security enable */

			/* Remove the device from the joined devices list */
			remove_lbds_list_entry(us_short_address);
		} else {
			LOG_BOOTSTRAP(("[BS] Error encoding KICK_TO_LBD.\r\n"));
		}
	} else {
		LOG_BOOTSTRAP(("[BS] Error: attempted KICK of not joined device [0x%04x]\r\n", us_short_address));
	}
}

/**
 * bs_lbp_get_lbds_counter.
 *
 */
uint16_t bs_lbp_get_lbds_counter(void)
{
	return get_lbds_count();
}

/**
 * bs_lbp_get_lbds_address.
 *
 */
uint16_t bs_lbp_get_lbds_address(uint16_t i)
{
	return get_lbd_address(i);
}

/**
 * bs_lbp_get_lbds_ex_address.
 *
 */
bool bs_lbp_get_lbds_ex_address(uint16_t us_short_address, uint8_t *puc_extended_address)
{
	return bs_get_ext_addr_by_short(us_short_address, puc_extended_address);
}

/**
 * bs_lbp_get_param.
 *
 */
void bs_lbp_get_param(uint32_t ul_attribute_id, uint16_t us_attribute_idx, struct t_bs_lbp_get_param_confirm *p_get_confirm)
{
	p_get_confirm->ul_attribute_id = ul_attribute_id;
	p_get_confirm->us_attribute_idx = us_attribute_idx;
	p_get_confirm->uc_attribute_length = 0;
	p_get_confirm->uc_status = LBP_STATUS_UNSUPPORTED_PARAMETER;

	if (ul_attribute_id == LBP_IB_DEVICE_LIST) {
		if (us_attribute_idx < MAX_LBDS) {
			uint16_t us_short_address = us_attribute_idx + get_initial_short_address();
			/* Check valid entry */
			if (device_is_in_list(us_short_address)) {
				p_get_confirm->uc_status = LBP_STATUS_OK;
				p_get_confirm->uc_attribute_length = 10;
				p_get_confirm->uc_attribute_value[0] = (uint8_t)(us_short_address & 0x00FF);
				p_get_confirm->uc_attribute_value[1] = (uint8_t)(us_short_address >> 8);
				memcpy(&p_get_confirm->uc_attribute_value[2], g_lbds_list[us_attribute_idx].puc_extended_address, ADP_ADDRESS_64BITS);
			} else {
				/* Empty list entry */
				LOG_BOOTSTRAP(("[BS] LBDs entry not valid for index %hu\r\n", us_attribute_idx));
				p_get_confirm->uc_status = LBP_STATUS_INVALID_VALUE;
				memset(&p_get_confirm->uc_attribute_value[0], 0, 10);
			}
		} else {
			/* Invalid list index */
			LOG_BOOTSTRAP(("[BS] Index %hu out of range of LBDs list\r\n", us_attribute_idx));
			p_get_confirm->uc_status = LBP_STATUS_INVALID_INDEX;
			memset(&p_get_confirm->uc_attribute_value[0], 0, 10);
		}
	} else if (ul_attribute_id == LBP_IB_INITIAL_SHORT_ADDRESS) {
		uint16_t us_initial_short_address = get_initial_short_address();
		p_get_confirm->uc_status = LBP_STATUS_OK;
		p_get_confirm->uc_attribute_length = 2;
		p_get_confirm->uc_attribute_value[0] = (uint8_t)(us_initial_short_address >> 8);
		p_get_confirm->uc_attribute_value[1] = (uint8_t)(us_initial_short_address & 0x00FF);
	} else if (ul_attribute_id == LBP_IB_SHORT_ADDRESS_FROM_EXTENDED) {
		bool short_address_from_extended_val = get_ib_short_address_from_extended();
		p_get_confirm->uc_status = LBP_STATUS_OK;
		p_get_confirm->uc_attribute_length = 1;
		p_get_confirm->uc_attribute_value[0] = (uint8_t)(short_address_from_extended_val);
	} else if (ul_attribute_id == LBP_IB_MSG_TIMEOUT) {
		uint16_t us_timeout = get_msg_timeout_value();
		p_get_confirm->uc_status = LBP_STATUS_OK;
		p_get_confirm->uc_attribute_length = 2;
		p_get_confirm->uc_attribute_value[0] = (uint8_t)(us_timeout >> 8);
		p_get_confirm->uc_attribute_value[1] = (uint8_t)(us_timeout & 0x00FF);
	} else {
		/* Unknown LBS parameter */
	}
}

/**
 * bs_lbp_set_param.
 *
 */
void bs_lbp_set_param(uint32_t ul_attribute_id, uint16_t us_attribute_idx, uint8_t uc_attribute_len, const uint8_t *puc_attribute_value,
		struct t_bs_lbp_set_param_confirm *p_set_confirm)
{
	uint16_t us_tmp_short_addr;
	uint16_t us_tmp_timeout;

	p_set_confirm->ul_attribute_id = ul_attribute_id;
	p_set_confirm->us_attribute_idx = us_attribute_idx;
	p_set_confirm->uc_status = LBP_STATUS_UNSUPPORTED_PARAMETER;

	switch (ul_attribute_id) {
	case LBP_IB_IDS:
		if ((uc_attribute_len == NETWORK_ACCESS_IDENTIFIER_SIZE_S_ARIB) || (uc_attribute_len == NETWORK_ACCESS_IDENTIFIER_SIZE_S_CENELEC_FCC)) {
			set_ids(puc_attribute_value, uc_attribute_len);
			p_set_confirm->uc_status = LBP_STATUS_OK;
		} else {
			p_set_confirm->uc_status = LBP_STATUS_INVALID_LENGTH;
		}

		break;

	case LBP_IB_DEVICE_LIST:
		if (uc_attribute_len == 10) {
			us_tmp_short_addr = ((puc_attribute_value[1] << 8) | puc_attribute_value[0]);
			/* Both short and extended addresses are specified */
			//Nodes manually registered are registered with 0 hops from Coordinator to indicate it.
			if (add_lbds_list_entry(&puc_attribute_value[2], us_tmp_short_addr, 0)) {
				p_set_confirm->uc_status = LBP_STATUS_OK;
			} else {
				LOG_BOOTSTRAP(("[BS] Address: 0x%04x already in use.\r\n", us_tmp_short_addr));
				p_set_confirm->uc_status = LBP_STATUS_INVALID_VALUE;
			}
		} else {
			/* Wrong parameter size */
			p_set_confirm->uc_status = LBP_STATUS_INVALID_LENGTH;
		}

		break;

	case LBP_IB_INITIAL_SHORT_ADDRESS:
		if (uc_attribute_len == 2) {
			if (get_lbds_count() > 0) {
				/* If there are active LBDs, the initial short address cannot be changed */
				p_set_confirm->uc_status = LBP_STATUS_NOK;
			} else {
				us_tmp_short_addr = ((puc_attribute_value[1] << 8) | puc_attribute_value[0]);

				if (set_initial_short_address(us_tmp_short_addr)) {
					p_set_confirm->uc_status = LBP_STATUS_OK;
				} else {
					p_set_confirm->uc_status = LBP_STATUS_INVALID_VALUE;
				}
			}
		} else {
			/* Wrong parameter size */
			p_set_confirm->uc_status = LBP_STATUS_INVALID_LENGTH;
		}

		break;

	case LBP_IB_ADD_DEVICE_TO_BLACKLIST:
		if (uc_attribute_len == 8) {
			add_to_blacklist((uint8_t *)puc_attribute_value);
			p_set_confirm->uc_status = LBP_STATUS_OK;
		} else {
			/* Wrong parameter size */
			p_set_confirm->uc_status = LBP_STATUS_INVALID_LENGTH;
		}

		break;

	case LBP_IB_PSK:
		if (uc_attribute_len == 16) {
			set_psk((uint8_t *)puc_attribute_value);
			p_set_confirm->uc_status = LBP_STATUS_OK;
		} else {
			/* Wrong parameter size */
			p_set_confirm->uc_status = LBP_STATUS_INVALID_LENGTH;
		}

		break;

	case LBP_IB_GMK:
		if (uc_attribute_len == 16) {
			set_gmk((uint8_t *)puc_attribute_value);
			p_set_confirm->uc_status = LBP_STATUS_OK;
		} else {
			/* Wrong parameter size */
			p_set_confirm->uc_status = LBP_STATUS_INVALID_LENGTH;
		}

		break;

	case LBP_IB_REKEY_GMK:
		if (uc_attribute_len == 16) {
			set_rekey_gmk((uint8_t *)puc_attribute_value);
			p_set_confirm->uc_status = LBP_STATUS_OK;
		} else {
			/* Wrong parameter size */
			p_set_confirm->uc_status = LBP_STATUS_INVALID_LENGTH;
		}

		break;

	case LBP_IB_SHORT_ADDRESS_FROM_EXTENDED:
		if (uc_attribute_len == 1) {
			set_ib_short_address_from_extended((bool) * puc_attribute_value);
			p_set_confirm->uc_status = LBP_STATUS_OK;
		} else {
			/* Wrong parameter size */
			p_set_confirm->uc_status = LBP_STATUS_INVALID_LENGTH;
		}

		break;

	case LBP_IB_MSG_TIMEOUT:
		if (uc_attribute_len == 2) {
			us_tmp_timeout = ((puc_attribute_value[1] << 8) | puc_attribute_value[0]);
			set_msg_timeout_value(us_tmp_timeout);
			p_set_confirm->uc_status = LBP_STATUS_OK;
		} else {
			/* Wrong parameter size */
			p_set_confirm->uc_status = LBP_STATUS_INVALID_LENGTH;
		}

		break;

	default:
		/* Unknown LBS parameter */
		break;
	}
}

/**
 * bs_lbp_leave_ind_set_cb.
 *
 */
void bs_lbp_leave_ind_set_cb(pf_app_leave_ind_cb_t pf_handler)
{
	pf_app_leave_ind_cb = pf_handler;
}

/**
 * bs_lbp_join_ind_set_cb.
 *
 */
void bs_lbp_join_ind_set_cb(pf_app_join_ind_cb_t pf_handler)
{
	pf_app_join_ind_cb = pf_handler;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
