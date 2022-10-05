/**
 * \file
 *
 * \brief G3 Coordinator API
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

#ifndef BS_API_H
#define BS_API_H

#include <AdpApi.h>

/* LBP parameter get confirm */
struct t_bs_lbp_get_param_confirm {
	uint8_t uc_status;
	uint32_t ul_attribute_id;
	uint16_t us_attribute_idx;
	uint8_t uc_attribute_length;
	uint8_t uc_attribute_value[64];
};

/* LBP parameter set confirm */
struct t_bs_lbp_set_param_confirm {
	uint8_t uc_status;
	uint32_t ul_attribute_id;
	uint16_t us_attribute_idx;
};

/* List of LBP attribute IDs */
enum lbp_ib_attribute {
	LBP_IB_IDS                         = 0x00000000,
	LBP_IB_DEVICE_LIST                 = 0x00000001,
	LBP_IB_INITIAL_SHORT_ADDRESS       = 0x00000002,
	LBP_IB_ADD_DEVICE_TO_BLACKLIST     = 0x00000003,
	LBP_IB_PSK                         = 0x00000004,
	LBP_IB_GMK                         = 0x00000005,
	LBP_IB_REKEY_GMK                   = 0x00000006,
	LBP_IB_SHORT_ADDRESS_FROM_EXTENDED = 0x00000007,
	LBP_IB_MSG_TIMEOUT                 = 0x00000008
};

/* LBP status */
enum lbp_status {
	LBP_STATUS_OK = 0,
	LBP_STATUS_NOK,
	LBP_STATUS_UNSUPPORTED_PARAMETER,
	LBP_STATUS_INVALID_INDEX,
	LBP_STATUS_INVALID_LENGTH,
	LBP_STATUS_INVALID_VALUE
};

typedef struct {
	AdpLbpConfirm fnctAdpLbpConfirm;
	AdpLbpIndication fnctAdpLbpIndication;
} TBootstrapAdpNotifications;

typedef struct {
	uint8_t m_u8BandInfo;
	uint8_t m_u8MaxHop;
} TBootstrapConfiguration;

/* User-defined callback for ADPM-NETWORK-LEAVE indication */
typedef void (*pf_app_leave_ind_cb_t)(uint16_t u16SrcAddr, bool bSecurityEnabled, uint8_t u8LinkQualityIndicator, uint8_t *pNsdu, uint16_t u16NsduLength);
/* User-defined callback for ADPM-NETWORK-JOIN indication */
typedef void (*pf_app_join_ind_cb_t)(uint8_t *puc_extended_address, uint16_t us_short_address);

/* Bootstrap module initialization */
void bs_init(TBootstrapConfiguration s_bs_conf);

/* Bootstrap module process, must be called at least once a second */
void bs_process(void);

TBootstrapAdpNotifications *bs_get_not_handlers(void);

/* LBP control functions */
void bs_lbp_launch_rekeying(void);
void bs_lbp_kick_device(uint16_t us_short_address);

/* LBP parameters external access */
uint16_t bs_lbp_get_lbds_counter(void);
uint16_t bs_lbp_get_lbds_address(uint16_t i);
bool bs_lbp_get_lbds_ex_address(uint16_t us_short_address, uint8_t *puc_extended_address);
void bs_lbp_get_param(uint32_t ul_attribute_id, uint16_t us_attribute_idx, struct t_bs_lbp_get_param_confirm *p_get_confirm);
void bs_lbp_set_param(uint32_t ul_attribute_id, uint16_t us_attribute_idx, uint8_t uc_attribute_len, const uint8_t *puc_attribute_value,
		struct t_bs_lbp_set_param_confirm *p_set_confirm);
bool bs_get_ext_addr_by_short(uint16_t us_short_address, uint8_t *puc_extended_address);
bool bs_get_short_addr_by_ext(uint8_t *puc_extended_address, uint16_t *pus_short_address);

void bs_lbp_leave_ind_set_cb(pf_app_leave_ind_cb_t pf_handler);
void bs_lbp_join_ind_set_cb(pf_app_join_ind_cb_t pf_handler);

#endif /* BS_API_H */
