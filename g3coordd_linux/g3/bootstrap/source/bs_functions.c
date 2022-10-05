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
#include <AdpApi.h>
#include <AdpApiTypes.h>
#include <mac_wrapper.h>
#include <ProtoLbp.h>
#include <ProtoEapPsk.h>

#include "bs_functions.h"
#include "bs_api.h"
#include "conf_bs.h"

#include <oss_if.h>
#ifndef LINUX
#include <hal/hal.h>
#else
#include <Random.h>
#endif

#include <string.h>
#include <stdio.h>

/* unsigned char m_Data[400]; */
/* unsigned short m_u16Length; */

uint8_t uc_nsdu_handle = 0;

bool m_bRekey;

static bool bGetShortAddressFromExtended;

static uint16_t us_msg_timeout_in_s = 40;

uint16_t us_blacklist_size = 0;
uint8_t puc_blacklist[MAX_LBDS][ADP_ADDRESS_64BITS];

t_context g_current_context;

t_bootstrap_slot bootstrap_slots[BOOTSTRAP_NUM_SLOTS];
#ifdef BS_LEVEL_STRATEGY_ENABLED
uint32_t ul_level_startegy_timeout = 0;
uint8_t uc_level_startegy_last_level_reg = 0;
#endif
static struct TEapPskKey g_EapPskKey = {
	{0xAB, 0x10, 0x34, 0x11, 0x45, 0x11, 0x1B, 0xC3, 0xC1, 0x2D, 0xE8, 0xFF, 0x11, 0x14, 0x22, 0x04}
};

struct TEapPskNetworkAccessIdentifierS g_IdS;

const struct TEapPskNetworkAccessIdentifierS x_ids_arib = { NETWORK_ACCESS_IDENTIFIER_SIZE_S_ARIB,
							    {0x53, 0x4D, 0xAD, 0xB2, 0xC4, 0xD5, 0xE6, 0xFA, 0x53, 0x4D, 0xAD, 0xB2, 0xC4, 0xD5, 0xE6, 0xFA,
							     0x53, 0x4D, 0xAD, 0xB2, 0xC4, 0xD5, 0xE6, 0xFA, 0x53, 0x4D, 0xAD, 0xB2, 0xC4, 0xD5, 0xE6, 0xFA,
							     0x53, 0x4D} };

const struct TEapPskNetworkAccessIdentifierS x_ids_cenelec_fcc = { NETWORK_ACCESS_IDENTIFIER_SIZE_S_CENELEC_FCC,
								   {0x81, 0x72, 0x63, 0x54, 0x45, 0x36, 0x27, 0x18} };

static uint8_t g_au8CurrKeyIndex = 0;
static uint8_t g_au8CurrGMK[16]  = {0xAF, 0x4D, 0x6D, 0xCC, 0xF1, 0x4D, 0xE7, 0xC1, 0xC4, 0x23, 0x5E, 0x6F, 0xEF, 0x6C, 0x15, 0x1F};
static uint8_t g_au8RekeyGMK[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16};

static TBootstrapConfiguration g_s_bs_conf;
/************************************************************************************/

/** LBDs table
 ************************************************************************************/
/* LBDs table */
lbds_list_entry_t g_lbds_list[MAX_LBDS];
/* Active LBDs counter */
static uint16_t g_lbds_counter = 0;
static uint16_t g_lbds_list_size = 0;

/************************************************************************************/

/** Parameters transferred
 * Note: the parameters are already encoded on 1 byte (M field and last bit included
 ************************************************************************************/
#define CONF_PARAM_SHORT_ADDR 0x1D
#define CONF_PARAM_GMK 0x27
#define CONF_PARAM_GMK_ACTIVATION 0x2B
#define CONF_PARAM_GMK_REMOVAL 0x2F
#define CONF_PARAM_RESULT 0x31

#ifdef BS_LEVEL_STRATEGY_ENABLED
void set_level_strategy_timeout(uint32_t ul_timeout_value)
{
	ul_level_startegy_timeout = ul_timeout_value;
}

#endif

/**
 * \brief Returns if the short address is valid
 *
 * \param us_short_address  Short address
 *
 * \return true / false.
 */
static bool _is_valid_address(uint16_t us_short_address)
{
	/* Check if the short address is out of the range */
	if (us_short_address < g_current_context.initialShortAddr) {
		return(false);
	} else if (us_short_address - g_current_context.initialShortAddr > MAX_LBDS) {
		return(false);
	} else {
		return(true);
	}
}

/**
 * \brief Returns if the short address is valid
 *
 * \param puc_extended_address  Extended address address
 *
 * \return true / false.
 */
bool is_null_address(uint8_t *puc_extended_address)
{
	uint8_t i;
	/* Check if the short address is null */
	for (i = 0; i < ADP_ADDRESS_64BITS; i++) {
		/* If any of the bytes of the address isn't null, return false. */
		if (puc_extended_address[i] != 0x00) {
			return(false);
		}
	}
	/* At this point, the address is null */
	return(true);
}

/**
 * \brief Returns the number of active LBDs
 *
 * \return number of active LBDs
 */
uint16_t get_lbds_count(void)
{
	uint16_t lbd_count, us_idx;

	lbd_count = 0;
	for (us_idx = 0; us_idx < MAX_LBDS; us_idx++) {
		if (!(is_null_address(g_lbds_list[us_idx].puc_extended_address))) {
			lbd_count++;
		}
	}

	return lbd_count;
}

/**
 * \brief Returns the LBD short address in position i of the LBDs list, if it is active
 *
 * \param i index in the LBDs list.
 *
 * \return LBD short address
 *
 * NOTE: Returns 0 if the address is not active.
 */
uint16_t get_lbd_address(uint16_t i)
{
	/* Check if the position is free */
	if (is_null_address(g_lbds_list[i].puc_extended_address)) {
		return 0x0000;
	} else {
		/* The short address is calculated using the index and the initial short address */
		return (i + g_current_context.initialShortAddr);
	}
}

/**
 * \brief Returns if the device is active in the LBDs list
 *
 * \param us_short_address short address
 *
 * \return 1 (yes) / 0 (no)
 *
 * NOTE: Returns 0 if the address is not active.
 */
uint8_t device_is_in_list(uint16_t us_short_address)
{
	/* Check if the short address is out of the range */
	if (!_is_valid_address(us_short_address)) {
		return (0);
	}

	/* Check if the address of the device is active */
	if (is_null_address(g_lbds_list[us_short_address - g_current_context.initialShortAddr].puc_extended_address)) {
		return (0);
	} else {
		return (1);
	}
}

/**
 * \brief Returns LBP estimated number of hops to LBA by short address
 *
 * \param us_short_address short address
 *
 * \return 0xFF (if addres is not in use) / #of hops
 *
 * NOTE: Returns LBP estimated number of hops to LBA by short address.
 */
uint8_t get_lbp_hops_from_short_address(uint16_t us_short_address)
{
	/* Check if the address of the device is active */
	if (is_null_address(g_lbds_list[us_short_address - g_current_context.initialShortAddr].puc_extended_address)) {
		return (0xff);
	} else {
		return g_lbds_list[us_short_address - g_current_context.initialShortAddr].uc_lbp_hops;
	}
}

/**
 * \brief Deactivates the LBD specified by its short address
 *
 * \param us_short_address short address
 */
void remove_lbds_list_entry(uint16_t us_short_address)
{
	/* Check if the short address is out of the range */
	if (!_is_valid_address(us_short_address)) {
		LOG_BOOTSTRAP(("[BS] Error: attempted to deactivate an address out of range [0x%04x]\r\n", us_short_address));
		return;
	} else {
		/* Check if the address is active */
		if (!is_null_address(g_lbds_list[us_short_address - g_current_context.initialShortAddr].puc_extended_address)) {
			/* Deactivate address */
			memset(&g_lbds_list[us_short_address -
					g_current_context.initialShortAddr].puc_extended_address, 0, ADP_ADDRESS_64BITS * sizeof(uint8_t));
			g_lbds_counter--;
		} else {
			/* The address is not active -> The device hasn't joined */
			LOG_BOOTSTRAP(("[BS] Error: attempted to deactivate an inactive address [0x%04x]\r\n", us_short_address));
		}
	}

	return;
}

/**
 * \brief Returns a new address that is not in use.
 *  If the end of the LBDs list is not reached, a new address is given instead
 *  of reusing the addresses that became free. This way, the time to reuse an
 *  address is extended.
 *
 * \return short address
 *
 * NOTE: 0x0000 will be returned if the LBDs list is full
 */
uint16_t get_new_address(struct TAdpExtendedAddress lbdAddress)
{
	uint16_t us_short_address = 0x0000;

	if (bGetShortAddressFromExtended) {
		if (g_lbds_list_size < MAX_LBDS) {
			us_short_address = (lbdAddress.m_au8Value[6] << 8) | lbdAddress.m_au8Value[7];
			g_lbds_list_size++;
		}

		return us_short_address;
	} else {
		uint16_t i = 0;

		/* If the end of the list is not reached, give the next address & increase list size */
		if (g_lbds_list_size < MAX_LBDS) {
			us_short_address = g_lbds_list_size + g_current_context.initialShortAddr;
			g_lbds_list_size++;
			return (us_short_address);
		} else {
			/* End of the list reached: Go through the list to find free positions */
			while ((i < MAX_LBDS)) {
				/* Check if the position is free (null extended address) */
				if (is_null_address(g_lbds_list[i].puc_extended_address)) {
					/* Free position: Return the address (calculated with the index and the initial short address) */
					us_short_address = i + g_current_context.initialShortAddr;
					return (us_short_address);
				}

				i++;
			}
		}

		/* At this point, there is no free positions in the list. Maximum number of devices reached. */
		LOG_BOOTSTRAP(("[BS] Device node: 0x%04x cannot be registered. Maximum number of nodes reached: %d\r\n", g_lbds_counter, MAX_LBDS));
		return us_short_address;
	}
}

/**
 * \brief Adds a LBD to the devices' list. A short address is assigned.
 *
 * \param puc_extended_address extended address
 * \param us_short_address short address
 * \param bitmap with the configuration (specifies if the LBD is active)
 *
 * \return true / false.
 */
bool add_lbds_list_entry(const uint8_t *puc_extended_address, uint16_t us_short_address, uint8_t uc_lbp_hops)
{
	/* Check if the short address is out of the range */
	if (!_is_valid_address(us_short_address)) {
		LOG_BOOTSTRAP(("[BS] Error: attempted to activate an address out of range [0x%04x]\r\n", us_short_address));
		return(false);
	}

	uint16_t us_position = us_short_address - g_current_context.initialShortAddr;
	uint16_t us_dummy_short_address;

	/* Check if the short address is already in use */
	if (!is_null_address(g_lbds_list[us_position].puc_extended_address)) {
		LOG_BOOTSTRAP(("[BS] Short address already in use [0x%04x], not added to list\r\n", us_short_address));
		return(false);
	}

	/* Check if the extended address is already in use */
	if (bs_get_short_addr_by_ext((uint8_t *)puc_extended_address, &us_dummy_short_address)) {
		LOG_BOOTSTRAP(("[BS] Extended address already in use [%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X]\r\n",
				*puc_extended_address, *(puc_extended_address + 1), *(puc_extended_address + 2),
				*(puc_extended_address + 3), *(puc_extended_address + 4), *(puc_extended_address + 5),
				*(puc_extended_address + 6), *(puc_extended_address + 7)));
		return(false);
	}

	memcpy(g_lbds_list[us_position].puc_extended_address, puc_extended_address, 8);
	g_lbds_list[us_position].uc_lbp_hops = uc_lbp_hops;
	LOG_BOOTSTRAP(("[BS] Added address [0x%04x]  LBP HOPS = %d\r\n", us_short_address, uc_lbp_hops));
	g_lbds_counter++;
	LOG_BOOTSTRAP(("[BS] Total num. devices: %d.\r\n", g_lbds_counter));

	return(true);
}

/**
 * \brief Function to handle joined devices list as a hash, indexed
 *        by short address
 *
 * \param us_short_address
 *
 * \return true if index found, false otherwise.
 */
bool bs_get_ext_addr_by_short(uint16_t us_short_address, uint8_t *puc_extended_address)
{
	uint16_t us_index;
	bool found = false;

	us_index = us_short_address - g_current_context.initialShortAddr;

	if (!is_null_address(g_lbds_list[us_index].puc_extended_address)) {
		memcpy(puc_extended_address, g_lbds_list[us_index].puc_extended_address, ADP_ADDRESS_64BITS);
		found = true;
	} else {
		LOG_BOOTSTRAP(("[BS] Error: tried to access joined devices list by not-joined short address.\r\n"));
	}

	return found;
}

/**
 * \brief Function to handle joined devices list as a hash, indexed
 *        by extended address
 *
 * \param
 *
 * \return true if index found, false otherwise.
 */
bool bs_get_short_addr_by_ext(uint8_t *puc_extended_address, uint16_t *pus_short_address)
{
	uint16_t i = 0;
	uint16_t j = 0;
	bool found = false;

	while (j < g_lbds_list_size) {
		if (!is_null_address(g_lbds_list[j].puc_extended_address)) {
			/* Check if the short address matches */
			found = true;
			for (i = 0; i < ADP_ADDRESS_64BITS; i++) {
				/* If any of the bytes of the address isn't null, go on. */
				if (puc_extended_address[i] != g_lbds_list[j].puc_extended_address[i]) {
					found = false;
					break;
				}
			}
		}

		if (found) {
			*pus_short_address = j + g_current_context.initialShortAddr;
			break;
		}

		j++;
	}
	if (found) {
		LOG_BOOTSTRAP(("[BS] Extended address found in joined devices list with short address 0x%04X.\r\n", *pus_short_address));
	} else {
		LOG_BOOTSTRAP(("[BS] Error: extended address not found in joined devices list.\r\n"));
	}

	return found;
}

/* / ** */
/* * AdpRoutingTable. */
/* * */
/* * / */
/* static void AdpRoutingTable(uint16_t data_len, uint8_t *data) */
/* { */
/*	AdpSetRequest(0x0000000C, 0, data_len, data); */
/* } */

/**
 * \brief LBP get rekeying
 *
 */
uint16_t lbp_get_rekeying(void)
{
	return m_bRekey;
}

/**
 * \brief LBP set rekeying
 *
 */
void lbp_set_rekeying(uint8_t on_off)
{
	m_bRekey = on_off;
}

/**
 * \brief Get initial short address
 *
 */
uint16_t get_initial_short_address(void)
{
	return g_current_context.initialShortAddr;
}

/**
 * \brief Set initial short address
 *
 */
bool set_initial_short_address(uint16_t us_short_addr)
{
	if (bGetShortAddressFromExtended) {
		/* Do not allow change initial address */
		return(false);
	} else {
		if (us_short_addr == 0) {
			return(false);
		} else if (us_short_addr > (0xFFFF - MAX_LBDS)) {
			return(false);
		} else {
			g_current_context.initialShortAddr = us_short_addr;
			return(true);
		}
	}
}

/**
 * \brief Get ShortAddressFromExtended IB value
 *
 */
bool get_ib_short_address_from_extended(void)
{
	return bGetShortAddressFromExtended;
}

/**
 * \brief Set ShortAddressFromExtended IB value
 *
 */
void set_ib_short_address_from_extended(bool value)
{
	bGetShortAddressFromExtended = value;
}

/**
 * \brief Get current Key index.
 *
 */
uint8_t GetKeyIndex(void)
{
	return g_au8CurrKeyIndex;
}

/**
 * \brief Set current Key index.
 *
 */
void SetKeyIndex(uint8_t u8KeyIndex)
{
	g_au8CurrKeyIndex = u8KeyIndex;
}

/**
 * \brief Get GMK.
 *
 */
const uint8_t *GetGMK(void)
{
	return g_au8CurrGMK;
}

/**
 * Set GMK.
 *
 */
void set_gmk(uint8_t *puc_new_gmk)
{
	if (puc_new_gmk != NULL) {
		memcpy(g_au8CurrGMK, puc_new_gmk, 16);
	}
}

/**
 * \brief Get rekeying GMK.
 *
 */
const uint8_t *GetRekeyGMK(void)
{
	return g_au8RekeyGMK;
}

/**
 * Set rekeying GMK.
 *
 */
void set_rekey_gmk(uint8_t *puc_new_gmk)
{
	if (puc_new_gmk != NULL) {
		memcpy(g_au8RekeyGMK, puc_new_gmk, 16);
	}
}

/**
 * Set IDS.
 *
 */
void set_ids(const uint8_t *puc_ids, uint8_t uc_size)
{
	memcpy(g_IdS.m_au8Value, puc_ids, uc_size);
	return;
}

/**
 * Set PSK.
 *
 */
void set_psk(uint8_t *puc_new_psk)
{
	if (puc_new_psk != NULL) {
		memcpy(g_EapPskKey.m_au8Value, puc_new_psk, 16);
	}
}

/**
 * \brief LBP init functions
 *
 */
void lbp_init_functions(void)
{
	/* The first short address is initially set to 1 */
	g_current_context.initialShortAddr = 0x0001;

	m_bRekey = LBP_REKEYING_OFF;

	bGetShortAddressFromExtended = false;

	/* Initialize LBP blacklist */
	us_blacklist_size = 0;
	memset(puc_blacklist, 0, MAX_LBDS * ADP_ADDRESS_64BITS);

	/* Initialize LBP list */
	g_lbds_counter = 0;
	g_lbds_list_size = 0;
	memset(g_lbds_list, 0, MAX_LBDS * sizeof(lbds_list_entry_t));

	if (g_s_bs_conf.m_u8BandInfo == ADP_BAND_ARIB) {
		g_IdS.uc_size = NETWORK_ACCESS_IDENTIFIER_MAX_SIZE_S;
		memcpy(g_IdS.m_au8Value, x_ids_arib.m_au8Value, NETWORK_ACCESS_IDENTIFIER_MAX_SIZE_S);
	} else {
		g_IdS.uc_size = 8;
		memcpy(g_IdS.m_au8Value, x_ids_cenelec_fcc.m_au8Value, 8);
	}

	init_bootstrap_slots();
}

/**
 * \brief Initialize bootstrap message
 *
 */
void initialize_bootstrap_message(t_bootstrap_slot *p_bs_slot)
{
	p_bs_slot->us_data_length = 0;
	memset(p_bs_slot->auc_data, 0, sizeof(p_bs_slot->auc_data));
}

/**
 * \brief Process_Joining0.
 *
 */
void Process_Joining0(struct TAdpExtendedAddress pLBPEUI64Address, t_bootstrap_slot *p_bs_slot )
{
	unsigned char *pMemoryBuffer = &p_bs_slot->auc_data[0];
	unsigned short u16MemoryBufferLength = sizeof(p_bs_slot->auc_data);
	uint16_t u16DummyShortAddress;

	LOG_BOOTSTRAP(("[BS] Process Joining 0.\r\n"));

	EAP_PSK_Initialize(&g_EapPskKey, &p_bs_slot->m_PskContext);

	/* initialize RandS */
	uint8_t i;
	for (i = 0; i < sizeof(p_bs_slot->m_randS.m_au8Value); i++) {
		p_bs_slot->m_randS.m_au8Value[i] = platform_random_32() & 0xFF;
	}
#ifdef FIXED_RAND_S
	uint8_t randS[16] = {0x11, 0x84, 0x8D, 0x16, 0xBC, 0x76, 0x76, 0xF6, 0x35, 0x65, 0x90, 0x12, 0x08, 0x2B, 0x3A, 0x97};
	memcpy(p_bs_slot->m_randS.m_au8Value, randS, 16);
#endif

	p_bs_slot->us_data_length = EAP_PSK_Encode_Message1(
			g_current_context.u8EAPIdentifier,
			&p_bs_slot->m_randS,
			&g_IdS,
			u16MemoryBufferLength,
			pMemoryBuffer
			);

	g_current_context.u8EAPIdentifier++;

	p_bs_slot->us_data_length = LBP_Encode_ChallengeRequest(
			&pLBPEUI64Address,
#ifdef G3_HYBRID_PROFILE
			p_bs_slot->m_u8MediaType,
			p_bs_slot->m_u8DisableBackupMedium,
#endif
			p_bs_slot->us_data_length,
			u16MemoryBufferLength,
			pMemoryBuffer
			);

	if (!m_bRekey) {
		/* If extended address is already in list, remove it and give a new short address */
		if (bs_get_short_addr_by_ext(pLBPEUI64Address.m_au8Value, &u16DummyShortAddress)) {
			remove_lbds_list_entry(u16DummyShortAddress);
		}

		/* Get a new address for the device. Its extended address will be added to the list when the joining process finishes. */
		p_bs_slot->us_assigned_short_address = get_new_address(p_bs_slot->m_LbdAddress);
	}
}

/**
 * \brief Process accepted GMK activation
 *
 */
uint8_t process_accepted_GMK_activation(struct TAdpExtendedAddress au8LBPEUI64Address, t_bootstrap_slot *p_bs_slot)
{
	unsigned char *pMemoryBuffer = &p_bs_slot->auc_data[0];
	unsigned short u16MemoryBufferLength = sizeof(p_bs_slot->auc_data);
	uint8_t pdata[3];
	uint8_t uc_result = 1;
	uint8_t u8NewKeyIndex;

	/* Get current key index and set the new one to the other */
	if (GetKeyIndex() == 0) {
		u8NewKeyIndex = 1;
	} else {
		u8NewKeyIndex = 0;
	}

	LOG_BOOTSTRAP(("[BS] Accepted(GMK-activation).\r\n"));

	/* prepare the protected data carring the key and short addr */
	pdata[0] = CONF_PARAM_GMK_ACTIVATION;
	pdata[1] = 0x01;
	pdata[2] = u8NewKeyIndex; /* key id */

	p_bs_slot->us_data_length = EAP_PSK_Encode_GMK_Activation(
			pdata,                 /* PCHANNEL data */
			u16MemoryBufferLength,
			pMemoryBuffer);

	/* Encode now the LBP message */
	p_bs_slot->us_data_length = LBP_Encode_AcceptedRequest(
			&au8LBPEUI64Address,
#ifdef G3_HYBRID_PROFILE
			p_bs_slot->m_u8MediaType,
			p_bs_slot->m_u8DisableBackupMedium,
#endif
			p_bs_slot->us_data_length,
			u16MemoryBufferLength,
			pMemoryBuffer);

	return(uc_result);
}

void log_show_slots_status(void)
{
	uint8_t uc_i;
	for (uc_i = 0; uc_i < BOOTSTRAP_NUM_SLOTS; uc_i++) {
		LOG_BOOTSTRAP((
					"[BS] Updating slot %hu with LBD_ADDR: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X, \
					state: %hu, handler: %hu  pending_cfrms: %hu  Timeout: %u, Current_Time: %u \r\n",
					uc_i, bootstrap_slots[uc_i].m_LbdAddress.m_au8Value[0], bootstrap_slots[uc_i].m_LbdAddress.m_au8Value[1],
					bootstrap_slots[uc_i].m_LbdAddress.m_au8Value[2], bootstrap_slots[uc_i].m_LbdAddress.m_au8Value[3],
					bootstrap_slots[uc_i].m_LbdAddress.m_au8Value[4], bootstrap_slots[uc_i].m_LbdAddress.m_au8Value[5],
					bootstrap_slots[uc_i].m_LbdAddress.m_au8Value[6], bootstrap_slots[uc_i].m_LbdAddress.m_au8Value[7],
					bootstrap_slots[uc_i].e_state, bootstrap_slots[uc_i].uc_tx_handle, bootstrap_slots[uc_i].uc_pending_confirms,
					bootstrap_slots[uc_i].ul_timeout, oss_get_up_time_ms()));
	}
}

/**
 * \brief Process_Joining_EAP_T1.
 *
 */
static uint8_t Process_Joining_EAP_T1(struct TAdpExtendedAddress au8LBPEUI64Address, unsigned short u16EAPDataLength,
		unsigned char *pEAPData, t_bootstrap_slot *p_bs_slot)
{
	struct TEapPskRand randS;
	struct TEapPskRand randP;
	unsigned char *pMemoryBuffer = &p_bs_slot->auc_data[0];
	unsigned short u16MemoryBufferLength = sizeof(p_bs_slot->auc_data);
	unsigned char pdata[50];
	unsigned short u16PDataLen = 0;
	unsigned short u16ShortAddr;
	uint8_t u8NewKeyIndex;
	uint8_t uc_result = 1;

	LOG_BOOTSTRAP(("[BS] Process Joining EAP T1.\r\n"));

	if (EAP_PSK_Decode_Message2(g_s_bs_conf.m_u8BandInfo, u16EAPDataLength, pEAPData, &p_bs_slot->m_PskContext, &g_IdS, &randS, &randP)) {
		LOG_BOOTSTRAP(("[BS] Decoded Message2.\r\n"));

		if (memcmp(randS.m_au8Value, p_bs_slot->m_randS.m_au8Value, sizeof(randS.m_au8Value)) != 0) {
			LOG_BOOTSTRAP(("[BS] ERROR: Bad RandS received\r\n"));
			uc_result = 0;
			return(uc_result);
		}

		EAP_PSK_InitializeTEK(&randP, &p_bs_slot->m_PskContext);

		/* encode and send the message T2 */
		u16ShortAddr = p_bs_slot->us_assigned_short_address;

		/* prepare the protected data carring the key and short addr */
		pdata[u16PDataLen++] = 0x02; /* ext field */

		if (!m_bRekey) {
			pdata[u16PDataLen++] = CONF_PARAM_SHORT_ADDR;
			pdata[u16PDataLen++] = 2;
			pdata[u16PDataLen++] = (unsigned char)((u16ShortAddr & 0xFF00) >> 8);
			pdata[u16PDataLen++] = (unsigned char)(u16ShortAddr & 0x00FF);

			pdata[u16PDataLen++] = CONF_PARAM_GMK;
			pdata[u16PDataLen++] = 17;
			pdata[u16PDataLen++] = g_au8CurrKeyIndex; /* key id */
			memcpy(&pdata[u16PDataLen], g_au8CurrGMK, 16); /* key */
			u16PDataLen += 16;

			pdata[u16PDataLen++] = CONF_PARAM_GMK_ACTIVATION;
			pdata[u16PDataLen++] = 1;
			pdata[u16PDataLen++] = g_au8CurrKeyIndex; /* key id */
		} else {
			/* Get current key index and set the new one to the other */
			if (GetKeyIndex() == 0) {
				u8NewKeyIndex = 1;
			} else {
				u8NewKeyIndex = 0;
			}

			pdata[u16PDataLen++] = CONF_PARAM_GMK;
			pdata[u16PDataLen++] = 17;
			pdata[u16PDataLen++] = u8NewKeyIndex; /* key id */
			memcpy(&pdata[u16PDataLen], g_au8RekeyGMK, 16); /* key */
			u16PDataLen += 16;
		}

		LOG_BOOTSTRAP(("[BS] Encoding Message3.\r\n"));
		p_bs_slot->us_data_length = EAP_PSK_Encode_Message3(
				&p_bs_slot->m_PskContext,
				g_current_context.u8EAPIdentifier,
				&randS,
				&randP,
				&g_IdS,
				p_bs_slot->ul_nonce,
				PCHANNEL_RESULT_DONE_SUCCESS,
				u16PDataLen,
				pdata,
				u16MemoryBufferLength,
				pMemoryBuffer
				);

		g_current_context.u8EAPIdentifier++;
		p_bs_slot->ul_nonce++;

		/* Encode now the LBP message */
		p_bs_slot->us_data_length = LBP_Encode_ChallengeRequest(
				&au8LBPEUI64Address,
#ifdef G3_HYBRID_PROFILE
				p_bs_slot->m_u8MediaType,
				p_bs_slot->m_u8DisableBackupMedium,
#endif
				p_bs_slot->us_data_length,
				u16MemoryBufferLength,
				pMemoryBuffer
				);
	} else {
		LOG_BOOTSTRAP(("[BS] ERROR: Process_Joining_EAP_T1.\r\n"));
		uc_result = 0;
	}

	return(uc_result);
}

/**
 * \brief Process_Joining_EAP_T3.
 *
 */
static bool Process_Joining_EAP_T3(struct TAdpExtendedAddress au8LBPEUI64Address, unsigned char *pBootStrappingData, unsigned short u16EAPDataLength,
		unsigned char *pEAPData, t_bootstrap_slot *p_bs_slot)
{
	struct TEapPskRand randS;
	unsigned char u8PChannelResult = 0;
	uint32_t u32Nonce = 0;
	unsigned short u16PChannelDataLength = 0;
	unsigned char *pPChannelData = 0L;
	unsigned char *pMemoryBuffer = &p_bs_slot->auc_data[0];
	unsigned short u16MemoryBufferLength = sizeof(p_bs_slot->auc_data);

	LOG_BOOTSTRAP(("[BS] Process Joining EAP T3.\r\n"));

	if (EAP_PSK_Decode_Message4(u16EAPDataLength, pEAPData, &p_bs_slot->m_PskContext,
			22, pBootStrappingData, &randS, &u32Nonce, &u8PChannelResult,
			&u16PChannelDataLength, &pPChannelData)) {
		LOG_BOOTSTRAP(("[BS] Decoded Message4.\r\n"));
		LOG_BOOTSTRAP(("[BS] Encoding Accepted.\r\n"));
		/* encode and send the message T2 */
		p_bs_slot->us_data_length = EAP_PSK_Encode_EAP_Success(
				g_current_context.u8EAPIdentifier,
				u16MemoryBufferLength,
				pMemoryBuffer
				);

		if (memcmp(randS.m_au8Value, p_bs_slot->m_randS.m_au8Value, sizeof(randS.m_au8Value)) != 0) {
			LOG_BOOTSTRAP(("[BS] ERROR: Bad RandS received\r\n"));
			return(false);
		}

		g_current_context.u8EAPIdentifier++;

		/* Encode now the LBP message */
		p_bs_slot->us_data_length = LBP_Encode_AcceptedRequest(
				&au8LBPEUI64Address,
#ifdef G3_HYBRID_PROFILE
				p_bs_slot->m_u8MediaType,
				p_bs_slot->m_u8DisableBackupMedium,
#endif
				p_bs_slot->us_data_length,
				u16MemoryBufferLength,
				pMemoryBuffer
				);

		/* Write device into routing table. Directly connected. */
		/* printf("Add device to Routing Table\n"); */
		/*		uint16_t g_lbs_u16ShortAddr = 1; */
		/*		route_buf[0] = g_lbs_u16ShortAddr & 0x00ff; */
		/*		route_buf[1] = g_lbs_u16ShortAddr>>8 & 0x00ff; */
		/*		route_buf[2] = g_lbs_u16ShortAddr & 0x00ff; */
		/*		route_buf[3] = g_lbs_u16ShortAddr>>8 & 0x00ff; */
		/*		AdpRoutingTable(9, route_buf); */

		return(true);
	} else {
		LOG_BOOTSTRAP(("[BS] ERROR: Process_Joining_EAP_T3.\r\n"));
		return(false);
	}
}

/**
 * \brief Add to blacklist
 *
 */
uint8_t add_to_blacklist(uint8_t *puc_address)
{
	/* TO DO: advanced list management (defragment list, etc...) */

	uint8_t uc_status = 1;

	if (us_blacklist_size < MAX_LBDS) {
		memcpy(puc_blacklist[us_blacklist_size], puc_address, ADP_ADDRESS_64BITS);
		us_blacklist_size++;
	} else {
		/* Blacklist full - error */
		uc_status = 0;
	}

	return uc_status;
}

/**
 * \brief Remove from blacklist
 *
 */
uint8_t remove_from_blacklist(uint16_t us_index)
{
	uint8_t uc_status = 0;

	/* TO DO: advanced list management (defragment list, etc...) */
	if (us_index < MAX_LBDS) {
		memset(puc_blacklist[us_index], 0, ADP_ADDRESS_64BITS);
		uc_status = 1;
	}

	return uc_status;
}

/**
 * \brief Device is in blacklist
 *
 */
static uint8_t _dev_is_in_blacklist(uint8_t *puc_address)
{
	uint16_t i = 0;
	uint8_t uc_found = 0;

	while (i < us_blacklist_size) {
		if (!memcmp(puc_address, puc_blacklist[i], ADP_ADDRESS_64BITS)) {
			uc_found = 1;
			break;
		}

		i++;
	}

	return uc_found;
}

/**
 * \brief ProcessLBPMessage.
 *
 */
enum lbp_indications ProcessLBPMessage(struct TAdpLbpIndication *pLbpIndication)
{
	static struct TAdpExtendedAddress ext_address_in_process;
	unsigned char u8MessageType;
	unsigned char *pBootStrappingData;
	unsigned short u16BootStrappingDataLength;
	struct TAddress dstAddr;

	/* Embedded EAP message */
	unsigned char u8Code = 0;
	unsigned char u8Identifier = 0;
	unsigned char u8TSubfield = 0;
	unsigned short u16EAPDataLength = 0;
	unsigned char *pEAPData = 0L;
	enum lbp_indications lbp_indication = LBS_NONE;
	struct TAdpExtendedAddress m_current_LbdAddress;
	t_bootstrap_slot *p_bs_slot;
#ifdef G3_HYBRID_PROFILE
	uint8_t u8MediaType;
	uint8_t u8DisableBackupMedium;
#endif

	uint16_t u16NsduLength;
	uint8_t *pNsdu;

	LOG_BOOTSTRAP(("[BS] ProcessLBPMessage.\r\n"));

	u16NsduLength = pLbpIndication->m_u16NsduLength;
	pNsdu = &pLbpIndication->m_pNsdu[0];
	u8MessageType = ((pNsdu[0] & 0xF0) >> 4);
#ifdef G3_HYBRID_PROFILE
	u8MediaType = ((pNsdu[0] & 0x08) >> 3);
	u8DisableBackupMedium = ((pNsdu[0] & 0x04) >> 2);
#endif
	pBootStrappingData = &pNsdu[10];
	u16BootStrappingDataLength = u16NsduLength - 10;

	memcpy(m_current_LbdAddress.m_au8Value, &pNsdu[2], 8);
	p_bs_slot = get_bootstrap_slot_by_addr(m_current_LbdAddress.m_au8Value);
	if (p_bs_slot) {
		initialize_bootstrap_message(p_bs_slot);
	}

	if (u8MessageType == LBP_JOINING) {
		LOG_BOOTSTRAP(("[BS] Processing incoming LBP_JOINING... LBD Address: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X \r\n",
				m_current_LbdAddress.m_au8Value[0], m_current_LbdAddress.m_au8Value[1],
				m_current_LbdAddress.m_au8Value[2], m_current_LbdAddress.m_au8Value[3],
				m_current_LbdAddress.m_au8Value[4], m_current_LbdAddress.m_au8Value[5],
				m_current_LbdAddress.m_au8Value[6], m_current_LbdAddress.m_au8Value[7]));

		/* Check the bootstrapping data in order to see the progress of the joining process */
		if (u16BootStrappingDataLength == 0) {
			LOG_BOOTSTRAP(("[BS] First joining request.\r\n"));
			/* This is the first joining request. Responded only if no other BS was in progress. */
			if (p_bs_slot) {
#ifdef G3_HYBRID_PROFILE
				/* Set Media Type and Disable backup flag from incoming LBP frame on Slot */
				p_bs_slot->m_u8MediaType = u8MediaType;
				p_bs_slot->m_u8DisableBackupMedium = u8DisableBackupMedium;
#endif
				/* Check if the joining device is blacklisted */
				if (_dev_is_in_blacklist(m_current_LbdAddress.m_au8Value)) {
					p_bs_slot->us_data_length = Encode_decline(m_current_LbdAddress.m_au8Value, p_bs_slot);
					p_bs_slot->e_state = BS_STATE_SENT_EAP_MSG_DECLINED;
					memcpy(p_bs_slot->m_LbdAddress.m_au8Value, m_current_LbdAddress.m_au8Value,
							sizeof(m_current_LbdAddress.m_au8Value));
					LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_SENT_EAP_MSG_DECLINED\r\n"));
				} else {
					if (p_bs_slot->e_state == BS_STATE_WAITING_JOINNING) {
						uint8_t uc_num_hops;
						if (pLbpIndication->m_u16SrcAddr == 0xFFFF) {
							uc_num_hops = 1;
						} else {
							uc_num_hops = get_lbp_hops_from_short_address(pLbpIndication->m_u16SrcAddr);
							if (uc_num_hops == 0xFF) {
								/* Unknown src addr (CONF_PC_6LOWPAN_BS_VB_003_RELAY_CONNECTION) */
								uc_num_hops = 1;
							}

							uc_num_hops += 1;
						}

#ifdef BS_LEVEL_STRATEGY_ENABLED
						if (uc_num_hops <= uc_level_startegy_last_level_reg  || timeout_is_past(ul_level_startegy_timeout)) {
							if (uc_num_hops > uc_level_startegy_last_level_reg) {
								uc_level_startegy_last_level_reg = uc_num_hops;
							}

							p_bs_slot->uc_lbp_hops = uc_num_hops;
							p_bs_slot->us_lba_src_addr = pLbpIndication->m_u16SrcAddr;
							memcpy(ext_address_in_process.m_au8Value, m_current_LbdAddress.m_au8Value,
									sizeof(m_current_LbdAddress.m_au8Value));
							memcpy(p_bs_slot->m_LbdAddress.m_au8Value, m_current_LbdAddress.m_au8Value,
									sizeof(m_current_LbdAddress.m_au8Value));
							Process_Joining0(m_current_LbdAddress, p_bs_slot);
							p_bs_slot->e_state = BS_STATE_SENT_EAP_MSG_1;
							LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_SENT_EAP_MSG_1\r\n"));
						} else {
							LOG_BOOTSTRAP(("[BS] LBP JOINNING IGNORED due to level restriction\r\n"));
							return LBS_NONE;
						}

#else
						p_bs_slot->uc_lbp_hops = uc_num_hops;
						p_bs_slot->us_lba_src_addr = pLbpIndication->m_u16SrcAddr;
						memcpy(ext_address_in_process.m_au8Value, m_current_LbdAddress.m_au8Value,
								sizeof(m_current_LbdAddress.m_au8Value));
						memcpy(p_bs_slot->m_LbdAddress.m_au8Value, m_current_LbdAddress.m_au8Value,
								sizeof(m_current_LbdAddress.m_au8Value));
						Process_Joining0(m_current_LbdAddress, p_bs_slot);
						p_bs_slot->e_state = BS_STATE_SENT_EAP_MSG_1;
						LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_SENT_EAP_MSG_1\r\n"));
#endif
					} else {
						LOG_BOOTSTRAP(("[BS] Repeated message processing Joining, silently ignored \r\n"));
					}
				}
			} else {
				LOG_BOOTSTRAP(("[BS] No slots available to process the request. Ignored.\r\n"));
			}
		} else {
			/* Check if the message comes from a device currently under BS */
			if (p_bs_slot) {
#ifdef G3_HYBRID_PROFILE
				/* Set Media Type and Disable backup flag from incoming LBP frame on Slot */
				p_bs_slot->m_u8MediaType = u8MediaType;
				p_bs_slot->m_u8DisableBackupMedium = u8DisableBackupMedium;
#endif
				/* check the type of the bootstrap data */
				if ((pBootStrappingData[0] & 0x01) != 0x01) {
					LOG_BOOTSTRAP(("[BS] Successive joining request.\r\n"));
					if (EAP_PSK_Decode_Message(
							u16BootStrappingDataLength,
							pBootStrappingData,
							&u8Code,
							&u8Identifier,
							&u8TSubfield,
							&u16EAPDataLength,
							&pEAPData)) {
						if (u8Code == EAP_RESPONSE) {
							if (u8TSubfield == EAP_PSK_T1 && (p_bs_slot->e_state == BS_STATE_WAITING_EAP_MSG_2 ||
									p_bs_slot->e_state == BS_STATE_SENT_EAP_MSG_1)) {
								if (Process_Joining_EAP_T1(m_current_LbdAddress, u16EAPDataLength, pEAPData, p_bs_slot) != 1) {
									/* Abort current BS process */
									LOG_BOOTSTRAP(("[BS] LBP error processing EAP T1.\r\n"));
									p_bs_slot->e_state = BS_STATE_WAITING_JOINNING;
									p_bs_slot->uc_pending_confirms = 0;
									p_bs_slot->ul_nonce =  0;

									LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_WAITING_JOINNING\r\n"));
								} else {
									p_bs_slot->e_state = BS_STATE_SENT_EAP_MSG_3;
									LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_SENT_EAP_MSG_3\r\n"));
								}
							} else if (u8TSubfield == EAP_PSK_T3 &&
									(p_bs_slot->e_state == BS_STATE_WAITING_EAP_MSG_4 || p_bs_slot->e_state ==
									BS_STATE_SENT_EAP_MSG_3)) {
								if (Process_Joining_EAP_T3(m_current_LbdAddress, pBootStrappingData, u16EAPDataLength, pEAPData,
										p_bs_slot)) {
									p_bs_slot->e_state = BS_STATE_SENT_EAP_MSG_ACCEPTED;
									LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_SENT_EAP_MSG_ACCEPTED\r\n"));
								} else {
									LOG_BOOTSTRAP(("[BS] LBP error processing EAP T3.\r\n"));
									p_bs_slot->e_state = BS_STATE_WAITING_JOINNING;
									p_bs_slot->uc_pending_confirms = 0;
									p_bs_slot->ul_nonce =  0;
									LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_WAITING_JOINNING\r\n"));
								}
							} else {
								/* Abort current BS process */
								LOG_BOOTSTRAP(("[BS] LBP protocol error.\r\n"));
								p_bs_slot->e_state = BS_STATE_WAITING_JOINNING;
								p_bs_slot->uc_pending_confirms = 0;
								p_bs_slot->ul_nonce =  0;
								LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_WAITING_JOINNING\r\n"));
							}
						}
					} else {
						/* Abort current BS process */
						LOG_BOOTSTRAP(("[BS] ERROR decoding message.\r\n"));
						p_bs_slot->e_state = BS_STATE_WAITING_JOINNING;
						p_bs_slot->uc_pending_confirms = 0;
						p_bs_slot->ul_nonce =  0;
						LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_WAITING_JOINNING\r\n"));
						log_show_slots_status();
					}
				}
			} else {
				LOG_BOOTSTRAP(("[BS] Concurrent successive joining received. Ignored.\r\n"));
			}
		}
	} else if (u8MessageType == LBP_KICK_FROM_LBD) {
		/* Call upper layer LEAVE callback */
		lbp_indication = LBS_KICK;
	} else {
		LOG_BOOTSTRAP(("[BS] ERROR: unknown incoming message.\r\n"));
	}

	if (p_bs_slot != NULL) {
		if (p_bs_slot->us_data_length > 0) {
			if (pLbpIndication->m_u16SrcAddr == 0xFFFF) {
				dstAddr.m_u8AddrLength = 8;
				memcpy(dstAddr.m_u8ExtendedAddr, &p_bs_slot->m_LbdAddress.m_au8Value, 8);
			} else {
				dstAddr.m_u8AddrLength = 2;
				dstAddr.m_u16ShortAddr = pLbpIndication->m_u16SrcAddr;
			}

			if (p_bs_slot->uc_pending_confirms > 0) {
				p_bs_slot->uc_pending_tx_handler = p_bs_slot->uc_tx_handle;
			}

			p_bs_slot->uc_tx_handle = get_next_nsdu_handler();
			p_bs_slot->ul_timeout = oss_get_up_time_ms() + 1000 * us_msg_timeout_in_s * 10;
			p_bs_slot->uc_tx_attemps = 0;
			p_bs_slot->uc_pending_confirms++;

			log_show_slots_status();
			LOG_BOOTSTRAP(("[BS] AdpLbpRequest Called, handler: %d \r\n", p_bs_slot->uc_tx_handle));
			AdpLbpRequest((struct TAdpAddress const *)&dstAddr,     /* Destination address */
					p_bs_slot->us_data_length,                                /* NSDU length */
					&p_bs_slot->auc_data[0],                                  /* NSDU */
					p_bs_slot->uc_tx_handle,                            /* NSDU handle */
					g_s_bs_conf.m_u8MaxHop,          						/* Max. Hops */
					true,                                       /* Discover route */
					0,                                          /* QoS */
					false);                                     /* Security enable */
		}
	}

	return(lbp_indication);
}

/**
 * \brief Encode kick to LBD.
 *
 */
uint16_t Encode_kick_to_LBD(uint8_t *p_ext_addr, uint16_t u16MessageLength, uint8_t *pMessageBuffer)
{
	uint16_t u16EncodedLength = 0;

	/* Check first if the message buffer size if enough */
	if (u16MessageLength >= ADP_ADDRESS_64BITS + 2) {
		/* start message encoding */
		pMessageBuffer[0] = (LBP_KICK_TO_LBD << 4); /* Media Type set to 0x0 on Kick */
		pMessageBuffer[1] = 0; /* transaction id is reserved */

		memcpy(&pMessageBuffer[2], p_ext_addr, ADP_ADDRESS_64BITS);

		u16EncodedLength = ADP_ADDRESS_64BITS + 2;
	}

	return u16EncodedLength;
}

/**
 * \brief Encode decline
 *
 */
uint16_t Encode_decline(uint8_t *p_ext_addr, t_bootstrap_slot *p_bs_slot)
{
	uint8_t *pMessageBuffer = &p_bs_slot->auc_data[0];
	uint16_t u16MessageLength = sizeof(p_bs_slot->auc_data);

	uint16_t u16EncodedLength = 0;

	if (u16MessageLength >= ADP_ADDRESS_64BITS + 2) {
#ifdef G3_HYBRID_PROFILE
		pMessageBuffer[0] = (LBP_DECLINE  << 4) | (p_bs_slot->m_u8MediaType << 3) | (p_bs_slot->m_u8DisableBackupMedium << 2);
#else
		pMessageBuffer[0] = (LBP_DECLINE  << 4);
#endif
		pMessageBuffer[1] = 0; /* transaction id is reserved */

		memcpy(&pMessageBuffer[2], p_ext_addr, ADP_ADDRESS_64BITS);

		u16EncodedLength = ADP_ADDRESS_64BITS + 2;
		
		/* encode EAP Failure */
		u16EncodedLength += EAP_PSK_Encode_EAP_Failure(
				g_current_context.u8EAPIdentifier,
				u16MessageLength - u16EncodedLength,
				(pMessageBuffer + u16EncodedLength)
				);
		
		g_current_context.u8EAPIdentifier++;
	}

	return u16EncodedLength;
}

uint8_t get_next_nsdu_handler(void)
{
	return uc_nsdu_handle++;
}

void  init_bootstrap_slots(void)
{
	uint8_t uc_i;

	for (uc_i = 0; uc_i < BOOTSTRAP_NUM_SLOTS; uc_i++) {
		bootstrap_slots[uc_i].e_state =  BS_STATE_WAITING_JOINNING;
		bootstrap_slots[uc_i].uc_pending_confirms = 0;
		bootstrap_slots[uc_i].uc_tx_handle =  0xff;
		bootstrap_slots[uc_i].ul_nonce =  0;
		bootstrap_slots[uc_i].uc_lbp_hops =  0;

		memset(bootstrap_slots[uc_i].m_LbdAddress.m_au8Value, 0xff, 8);
	}
}

t_bootstrap_slot *get_bootstrap_slot_by_addr(uint8_t *p_eui64)
{
	uint8_t uc_i;
	t_bootstrap_slot *p_out_slot = NULL;

	LOG_BOOTSTRAP(("[BS] get_bootstrap_slot_by_addr\r\n"));
	/* Check if the lbd is already started */
	for (uc_i = 0; uc_i < BOOTSTRAP_NUM_SLOTS; uc_i++) {
		if (!memcmp(bootstrap_slots[uc_i].m_LbdAddress.m_au8Value, p_eui64, 8)) {
			p_out_slot = &bootstrap_slots[uc_i];
			LOG_BOOTSTRAP(("[BS] get_bootstrap_slot_by_addr --> Slot in use found: %d \r\n", uc_i));
			break;
		}
	}
	/* If lbd not in progress find free slot */
	if (!p_out_slot) {
		for (uc_i = 0; uc_i < BOOTSTRAP_NUM_SLOTS; uc_i++) {
			if (bootstrap_slots[uc_i].e_state ==  BS_STATE_WAITING_JOINNING) {
				p_out_slot = &bootstrap_slots[uc_i];
				LOG_BOOTSTRAP(("[BS] get_bootstrap_slot_by_addr --> Slot free found: %d \r\n", uc_i));
				break;
			}
		}
	}

	if (!p_out_slot) {
		LOG_BOOTSTRAP(("[BS] get_bootstrap_slot_by_addr --> Slot not found \r\n"));
	}

	return p_out_slot;
}

t_bootstrap_slot *get_bootstrap_slot_by_index(uint8_t uc_index)
{
	return &bootstrap_slots[uc_index];
}

void  update_bootstrap_slots(void)
{
	uint8_t uc_i;

	for (uc_i = 0; uc_i < BOOTSTRAP_NUM_SLOTS; uc_i++) {
		/* log_show_slots_status(); */
		if (bootstrap_slots[uc_i].e_state != BS_STATE_WAITING_JOINNING) {
			if (timeout_is_past(bootstrap_slots[uc_i].ul_timeout)) {
				LOG_BOOTSTRAP(("[BS] timeout_is_past for %d\r\n",uc_i));
				if (bootstrap_slots[uc_i].uc_pending_confirms == 0) {
					if (bootstrap_slots[uc_i].uc_tx_attemps < BOOTSTRAP_MSG_MAX_RETRIES) {
						bootstrap_slots[uc_i].uc_tx_attemps++;
						if (bootstrap_slots[uc_i].e_state == BS_STATE_WAITING_EAP_MSG_2) {
							bootstrap_slots[uc_i].e_state = BS_STATE_SENT_EAP_MSG_1;
							LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_SENT_EAP_MSG_1\r\n"));
							log_show_slots_status();
						} else if (bootstrap_slots[uc_i].e_state == BS_STATE_WAITING_EAP_MSG_4) {
							bootstrap_slots[uc_i].e_state = BS_STATE_SENT_EAP_MSG_3;
							LOG_BOOTSTRAP(("[BS] Slot updated to BS_STATE_SENT_EAP_MSG_3\r\n"));
							log_show_slots_status();
						}

						struct TAddress dstAddr;
						struct TAdpGetConfirm getConfirm;

						if (bootstrap_slots[uc_i].us_data_length > 0) {
							if (bootstrap_slots[uc_i].us_lba_src_addr == 0xFFFF) {
								dstAddr.m_u8AddrLength = 8;
								memcpy(dstAddr.m_u8ExtendedAddr, &bootstrap_slots[uc_i].m_LbdAddress.m_au8Value, 8);
							} else {
								dstAddr.m_u8AddrLength = 2;
								dstAddr.m_u16ShortAddr = bootstrap_slots[uc_i].us_lba_src_addr;
							}

							if (bootstrap_slots[uc_i].uc_pending_confirms > 0) {
								bootstrap_slots[uc_i].uc_pending_tx_handler = bootstrap_slots[uc_i].uc_tx_handle;
							}

							bootstrap_slots[uc_i].uc_tx_handle = get_next_nsdu_handler();
							bootstrap_slots[uc_i].ul_timeout = oss_get_up_time_ms() + 1000 * us_msg_timeout_in_s * 10;
							bootstrap_slots[uc_i].uc_pending_confirms++;

							LOG_BOOTSTRAP(("[BS] Timeout detected. Re-sending MSG for slot: %d Attempt: %d \r\n", uc_i,
									bootstrap_slots[uc_i].uc_tx_attemps));
							log_show_slots_status();
							LOG_BOOTSTRAP(("[BS] AdpLbpRequest Called, handler: %d \r\n", bootstrap_slots[uc_i].uc_tx_handle));
							AdpLbpRequest((struct TAdpAddress const *)&dstAddr,     /* Destination address */
									bootstrap_slots[uc_i].us_data_length,                              /* NSDU length */
									&bootstrap_slots[uc_i].auc_data[0],                                  /* NSDU */
									bootstrap_slots[uc_i].uc_tx_handle,                            /* NSDU handle */
									g_s_bs_conf.m_u8MaxHop,          						/* Max. Hops */
									true,                                       /* Discover route */
									0,                                          /* QoS */
									false);                                     /* Security enable */
						}
					} else {
						LOG_BOOTSTRAP(("[BS] Reset slot %d:  \r\n", uc_i));
						bootstrap_slots[uc_i].e_state = BS_STATE_WAITING_JOINNING;
						bootstrap_slots[uc_i].uc_pending_confirms = 0;
						bootstrap_slots[uc_i].ul_nonce =  0;
						bootstrap_slots[uc_i].ul_timeout = 0xFFFFFFFF;
					}
				} else { /* Pending confirm then increase timeout time */
					LOG_BOOTSTRAP(("[BS] NEVER SHOUL BE HERE --> Reset slot %d:  \r\n", uc_i));
					bootstrap_slots[uc_i].e_state = BS_STATE_WAITING_JOINNING;
					bootstrap_slots[uc_i].uc_pending_confirms = 0;
					bootstrap_slots[uc_i].ul_nonce =  0;
					bootstrap_slots[uc_i].ul_timeout = 0xFFFFFFFF;
				}
			}
		}
	}
}

uint8_t  get_max_hops_from_nodes_under_registering(void)
{
	uint8_t uc_i;
	uint8_t uc_result = 0;
	for (uc_i = 0; uc_i < BOOTSTRAP_NUM_SLOTS; uc_i++) {
		/* log_show_slots_status(); */
		if (bootstrap_slots[uc_i].e_state != BS_STATE_WAITING_JOINNING) {
			if (!timeout_is_past(bootstrap_slots[uc_i].ul_timeout)) {
				if (bootstrap_slots[uc_i].uc_lbp_hops > uc_result) {
					uc_result = bootstrap_slots[uc_i].uc_lbp_hops;
				}
			}
		}
	}
	return uc_result;
}

uint16_t get_msg_timeout_value(void)
{
	return us_msg_timeout_in_s;
}

void set_msg_timeout_value(uint16_t us_timeout_value)
{
	us_msg_timeout_in_s = us_timeout_value;
}

/**********************************************************************************************************************/

/**
 **********************************************************************************************************************/
bool timeout_is_past(uint32_t ul_timeout_value)
{
	return (((int32_t)oss_get_up_time_ms()) - (int32_t)ul_timeout_value > 0);
}

void set_bs_configuration(TBootstrapConfiguration s_bs_conf){
  memcpy(&g_s_bs_conf, &s_bs_conf, sizeof(TBootstrapConfiguration));
}

TBootstrapConfiguration *get_bs_configuration(void){
  return &g_s_bs_conf;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */