/**
 * \file
 *
 * \brief Base Node Management Firmware Upgrade file.
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

/************************************************************
*       Includes                                            *
*************************************************************/
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "mngLayerHost.h"
#include "prime_api_host.h"
#include "prime_api_defs_host.h"
#include "cl_432_defs.h"
#include "mac_pib.h"
#include "mac_defs.h"

#include "prime_utils.h"
#include "prime_log.h"
#include "return_codes.h"

#include "globals.h"
#include "base_node_manager.h"
#include "base_node_mng.h"
#include "base_node_network.h"
#include "base_node_mng_fw_upgrade.h"

/************************************************************
*       Defines                                             *
*************************************************************/
#define PRIME_SYNC_TIMEOUT 3
#define CHUNKSIZE 0x0200
#define MAX_NUM_TRIES_TRANSFER 3
#define BROADCAST_ADDRESS 0xFFFFFFFFFFFF

/* Extern Vars */
extern mchp_list prime_network;
extern struct st_configuration g_st_config;
extern T_prime_sync_mgmt g_prime_sync_mgmt;
uint32_t fup_global_status = FW_UPGRADE_IDLE;

/* Global Vars */
fwUpgradeOptions fu_options;


uint8_t fup_ack_code_str[10][16] = {"OK","ERROR","ERROR MAC","ERROR MODEL","ERROR_CRC","ERROR_DATA","ERROR_CRC_FILE","CRC_ONGOING","FU_ONGOING" };

#if 0
void prime_bmng_fup_get_state_request(uint8_t * puc_eui48)
#endif

/** CRC-32 evaluation table */
static const uint32_t pul_crc32_table[256] = {
	0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9,
	0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
	0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61,
	0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD,
	0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9,
	0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75,
	0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011,
	0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD,
	0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039,
	0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5,
	0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81,
	0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,
	0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49,
	0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
	0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1,
	0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D,
	0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE,
	0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072,
	0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16,
	0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA,
	0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE,
	0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02,
	0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066,
	0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
	0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E,
	0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692,
	0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6,
	0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A,
	0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E,
	0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2,
	0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686,
	0xD5B88683, 0xD1799B34, 0xDC3ABDED, 0xD8FBA05A,
	0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637,
	0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB,
	0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F,
	0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53,
	0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47,
	0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B,
	0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF,
	0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623,
	0xF12F560E, 0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7,
	0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B,
	0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F,
	0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
	0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7,
	0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B,
	0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F,
	0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3,
	0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640,
	0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C,
	0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8,
	0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24,
	0x119B4BE9, 0x155A565E, 0x18197087, 0x1CD86D30,
	0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
	0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088,
	0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654,
	0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0,
	0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C,
	0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18,
	0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4,
	0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0,
	0x9ABC8BD5, 0x9E7D9662, 0x933EB0BB, 0x97FFAD0C,
	0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668,
	0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4,
};

/**
 * \brief  This function calculates the CRC32 of the input data.
 *
 * \param    puc_buf_ptr  Pointer to the input data
 * \param    ul_len       Length of data to evaluate
 * \param    ul_crc_init  Initial value for the CRC calculation algorithm
 *
 * \retval   calculated CRC
 *
 */
static uint32_t fu_eval_crc_32(const uint8_t *puc_buf_ptr, uint32_t ul_len, uint32_t ul_crc_init)
{
	uint8_t uc_idx;
	uint32_t ul_crc;

	ul_crc = ul_crc_init;
	if (ul_len != 0) {
		while (ul_len--) {
			uc_idx = (uint8_t)(ul_crc >> 24) ^ *puc_buf_ptr++;
			ul_crc = (ul_crc << 8) ^ pul_crc32_table[uc_idx];
		}
	}

	return ul_crc;
}

/**
 * \brief  This function calculates the CRC32 of a file stream
 *
 * \param    file  file stream
 *
 * \retval   calculated CRC
 *
 */
static uint32_t fu_eval_crc_32_file(FILE *file)
{
	uint32_t crc_init, nbytes;
  uint8_t  buffer[CHUNKSIZE];

	crc_init = 0;
  fseek(file,0,SEEK_SET);  /* Set Pointer to the beginning of the file */
  nbytes = fread (buffer, sizeof(uint8_t), CHUNKSIZE, file);
  while (nbytes != 0)
  {
     crc_init = fu_eval_crc_32(buffer, nbytes, crc_init);
     nbytes = fread(buffer, sizeof(uint8_t), CHUNKSIZE, file);
	}

	return crc_init;
}

/**
 * \brief Set Firmware Upgrade Options
 * \param  options -> Firmware Upgrade Options
 *
 * \return
*/
int fw_upgrade_set_upg_options(struct TfwUpgradeOptions *options)
{
  fu_options.mult_en  = options->mult_en;
  fu_options.arq_en   = options->arq_en;
  fu_options.pagesize = options->pagesize;
  fu_options.delay    = options->delay;
  fu_options.timer    = options->timer;
  return SUCCESS;
}

/**
 * \brief Get Firmware Upgrade Options
 * \param  options -> Firmware Upgrade Options
 *
 * \return
*/
int fw_upgrade_get_upg_options(struct TfwUpgradeOptions *options)
{
  options->mult_en = fu_options.mult_en;
  options->arq_en = fu_options.arq_en;
  options->pagesize = fu_options.pagesize;
  options->delay = fu_options.delay;
  options->timer = fu_options.timer;
  return SUCCESS;
}

/**
 * \brief Set Upgrade Options on Base Node FW Upgrade Mechanism
 * \param uc_arq_en       Enables ARQ
 * \param uc_page_size    Page Size size of the data chunks used during the PLC phase of the FU
 * \param uc_mult_en      Enables multicasting to transfer the firmware image through the PLC
 * \param ui_delay        Delay Restart -> time that a Service Node waits before restarting with the new image
 * \param ui_timer        Safety Timer -> time a Service Node must wait before reverting to a former firmware image when the new image is not confirmed by the Base Node
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
*/
int bmng_fup_set_upg_options_request_sync(uint8_t uc_arq_en, uint8_t uc_page_size, uint8_t uc_mult_en, uint32_t ui_delay, uint32_t ui_timer, struct TmacSetConfirm *pmacSetConfirm)
{
  PRIME_LOG(LOG_DEBUG,"bmng_fup_set_upg_options_request_sync\r\n");

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     bmng_fup_clear_target_list_request is asyncronous - Base Node will callback request
     with prime_bmng_fup_ack_ind_msg_cb cmd=bmng_fup_add_target_request_sync
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_fup_set_upg_options_request_cmd;
  /* Default Status */
  pmacSetConfirm->m_u8Status = -1;

  // Send the asynchronous request
  bmng_fup_set_upg_options_request(uc_arq_en, uc_page_size, uc_mult_en, ui_delay, ui_timer);

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(PRIME_SYNC_TIMEOUT, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (!g_prime_sync_mgmt.f_sync_res){
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_ack_ind_msg_cb not received\r\n");
  }else if (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != FUP_ACK_OK){
    // Error sending reboot request
    pmacSetConfirm->m_u8Status = g_prime_sync_mgmt.s_macSetConfirm.m_u8Status;
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_set_upg_options_request_sync failed\r\n");
  }else{
    pmacSetConfirm->m_u8Status = PPROF_ACK_OK;
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"bmng_fup_set_upg_options_request_sync result=%d\r\n",pmacSetConfirm->m_u8Status);

  return pmacSetConfirm->m_u8Status;
}
/**
 * \brief Clear Base Node FW Upgrade List
 * \param pmacSetConfirm  Confirm Structure
 * \return
*/
int bmng_fup_clear_target_list_request_sync(struct TmacSetConfirm *pmacSetConfirm)
{
  PRIME_LOG(LOG_DEBUG,"bmng_fup_clear_target_list_request_sync\r\n");

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     bmng_fup_clear_target_list_request is asyncronous - Base Node will callback request
     with prime_bmng_fup_ack_ind_msg_cb cmd=bmng_fup_add_target_request_sync
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_fup_clear_target_list_request_cmd;
  /* Default Status */
  pmacSetConfirm->m_u8Status = -1;

  // Send the asynchronous request
  bmng_fup_clear_target_list_request();

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(PRIME_SYNC_TIMEOUT, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (!g_prime_sync_mgmt.f_sync_res){
    PRIME_LOG(LOG_DEBUG,"ERROR: bmng_fup_ack_ind_msg_cb not received\r\n");
  }else if (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != FUP_ACK_OK){
    // Error sending reboot request
    pmacSetConfirm->m_u8Status = g_prime_sync_mgmt.s_macSetConfirm.m_u8Status;
    PRIME_LOG(LOG_DEBUG,"ERROR: bmng_fup_clear_target_list_request_sync failed\r\n");
  }else{
    pmacSetConfirm->m_u8Status = PPROF_ACK_OK;
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"bmng_fup_clear_target_list_request_sync result=%d\r\n",pmacSetConfirm->m_u8Status);

  return pmacSetConfirm->m_u8Status;

}

/********************************************************/
/**
 * \brief Add Service Node to FW Upgrade List
 * \param puc_eui48       Service Node EUI48
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
 ********************************************************/
int bmng_fup_add_target_request_sync(uint8_t * puc_eui48, struct TmacSetConfirm *pmacSetConfirm)
{
  PRIME_LOG(LOG_DEBUG,"bmng_fup_add_target_request_sync %s\r\n",eui48_to_str(puc_eui48,NULL));

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     bmng_fup_add_target_request is asyncronous - Base Node will callback request
     with prime_bmng_fup_ack_ind_msg_cb cmd=bmng_fup_add_target_request_sync
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_fup_add_target_request_cmd;
  /* Default Status */
  pmacSetConfirm->m_u8Status = -1;

  // Send the asynchronous request
  bmng_fup_add_target_request(puc_eui48);

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(PRIME_SYNC_TIMEOUT, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (!g_prime_sync_mgmt.f_sync_res){
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_ack_ind_msg_cb not received\r\n");
  }else if (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != FUP_ACK_OK){
    // Error sending reboot request
    pmacSetConfirm->m_u8Status = g_prime_sync_mgmt.s_macSetConfirm.m_u8Status;
    PRIME_LOG(LOG_ERR,"ERROR: prime_bmng_fup_add_target_request_cmd failed\r\n");
  }else{
    pmacSetConfirm->m_u8Status = PPROF_ACK_OK;
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"bmng_fup_add_target_request_sync result=%d\r\n",pmacSetConfirm->m_u8Status);

  return pmacSetConfirm->m_u8Status;

}

/********************************************************/
/**
 * \brief Abort the FW Upgrade process for a Service Node
 * \param puc_eui48       Service Node EUI48
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
 ********************************************************/
int bmng_fup_abort_fu_request_sync(uint8_t * puc_eui48, struct TmacSetConfirm *pmacSetConfirm)
{
  PRIME_LOG(LOG_DEBUG,"bmng_fup_abort_fu_request_sync %s\r\n",eui48_to_str(puc_eui48,NULL));

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     bmng_fup_add_target_request is asyncronous - Base Node will callback request
     with prime_bmng_fup_ack_ind_msg_cb cmd=bmng_fup_add_target_request_sync
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_fup_abort_fu_request_cmd;
  /* Default Status */
  pmacSetConfirm->m_u8Status = -1;

  // Send the asynchronous request
  bmng_fup_abort_fu_request(puc_eui48);

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(PRIME_SYNC_TIMEOUT, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (!g_prime_sync_mgmt.f_sync_res){
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_ack_ind_msg_cb not received\r\n");
  }else if (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != FUP_ACK_OK){
    // Error sending abort request
    pmacSetConfirm->m_u8Status = g_prime_sync_mgmt.s_macSetConfirm.m_u8Status;
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_abort_fu_request_sync failed\r\n");
  }else{
    pmacSetConfirm->m_u8Status = PPROF_ACK_OK;
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"bmng_fup_abort_fu_request_sync result=%d\r\n",pmacSetConfirm->m_u8Status);

  return pmacSetConfirm->m_u8Status;

}

/**
 * \brief Set Match Rule based on Model and Version. Match rules: 0000 0MV0
 *        If M and/or V are set, only the nodes matching model and/or vendor will be upgraded.
 *        The decision will be taken on Base Node
 * \param options        Firmware Upgrade options
 *
 * \return
*/
int fw_upgrade_set_match_rule(struct TfwUpgradeOptions *options)
{
   fu_options.filter_model = options->filter_model ? 1:0;
   fu_options.filter_vendor = options->filter_vendor ? 1:0;
   return SUCCESS;
}

/**
 * \brief Get Match Rule options
 * \param options        Firmware Upgrade options
 *
 * \return
*/
int fw_upgrade_get_match_rule(struct TfwUpgradeOptions *options)
{
   options->filter_model = fu_options.filter_model;
   options->filter_vendor = fu_options.filter_vendor;
   return SUCCESS;
}

/**
 * \brief Set Match Rule based on Model and Version. Match rules: 0000 0MV0
 *        If M and/or V are set, only the nodes matching model and/or vendor will be upgraded.
 *        The decision will be taken on Base Node
 * \param uc_rule         Match Rule
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
*/
int bmng_fup_set_match_rule_request_sync(uint8_t uc_rule, struct TmacSetConfirm *pmacSetConfirm)
{

  PRIME_LOG(LOG_DEBUG,"bmng_fup_set_match_rule_request_sync\r\n");

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     bmng_fup_clear_target_list_request is asyncronous - Base Node will callback request
     with prime_bmng_fup_ack_ind_msg_cb cmd=bmng_fup_add_target_request
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_fup_set_match_rule_request_cmd;
  /* Default Status */
  pmacSetConfirm->m_u8Status = -1;

  // Send the asynchronous request
  bmng_fup_set_match_rule_request(uc_rule);

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(PRIME_SYNC_TIMEOUT, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (!g_prime_sync_mgmt.f_sync_res){
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_ack_ind_msg_cb not received\r\n");
  }else if (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != FUP_ACK_OK){
    // Error sending reboot request
    pmacSetConfirm->m_u8Status = g_prime_sync_mgmt.s_macSetConfirm.m_u8Status;
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_set_match_rule_request_sync failed\r\n");
  }else{
    pmacSetConfirm->m_u8Status = PPROF_ACK_OK;
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"bmng_fup_set_match_rule_request_sync result=%d\r\n",pmacSetConfirm->m_u8Status);

  return pmacSetConfirm->m_u8Status;
}

/**
 * \brief Set Signature Data Request
 * \param uc_sig_algo      Signature Algorithm
 * \param us_sig_len       Signature Length
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
*/
int prime_bmng_fup_set_signature_data_request_sync(uint8_t uc_sig_algo, uint16_t us_sig_len, struct TmacSetConfirm *pmacSetConfirm)
{

  PRIME_LOG(LOG_DEBUG,"prime_bmng_fup_set_signature_data_request_sync\r\n");

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     prime_bmng_fup_set_signature_data_request_cmd is asyncronous - Base Node will callback request
     with prime_bmng_fup_ack_ind_msg_cb cmd=prime_bmng_fup_set_signature_data_request_cmd
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_fup_set_signature_data_request_cmd;
  /* Default Status */
  pmacSetConfirm->m_u8Status = -1;

  // Send the asynchronous request
  bmng_fup_signature_cfg_cmd(uc_sig_algo, us_sig_len);

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(PRIME_SYNC_TIMEOUT, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (!g_prime_sync_mgmt.f_sync_res){
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_ack_ind_msg_cb not received\r\n");
  }else if (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != FUP_ACK_OK){
    // Error sending reboot request
    pmacSetConfirm->m_u8Status = g_prime_sync_mgmt.s_macSetConfirm.m_u8Status;
    PRIME_LOG(LOG_ERR,"ERROR: prime_bmng_fup_set_signature_data_request_sync failed\r\n");
  }else{
    pmacSetConfirm->m_u8Status = PPROF_ACK_OK;
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"prime_bmng_fup_set_signature_data_request_sync result=%d\r\n",pmacSetConfirm->m_u8Status);

  return pmacSetConfirm->m_u8Status;
}

/**
 * \brief Set FW Data Info based on Vendor,Model and Version
 * \param options        Firmware Upgrade options
 *
 * \return
*/
int fw_upgrade_set_fw_data_info(struct TfwUpgradeOptions *options)
{
   memcpy(&fu_options.vendor, options->vendor, options->vendor_len);
   fu_options.vendor_len = options->vendor_len;
   memcpy(&fu_options.model, options->model,options->model_len);
   fu_options.model_len = options->model_len;
   memcpy(&fu_options.version, options->version,options->version_len);
   fu_options.version_len = options->version_len;
   return SUCCESS;
}

/**
 * \brief Get FW Data Info
 * \param options        Firmware Upgrade options
 *
 * \return
*/
int fw_upgrade_get_fw_data_info(struct TfwUpgradeOptions *options)
{
   memcpy(options->vendor,&fu_options.vendor,32);
   options->vendor_len = fu_options.vendor_len;
   memcpy(options->model,&fu_options.model,32);
   options->model_len = fu_options.model_len;
   memcpy(options->version,&fu_options.version,32);
   options->version_len = fu_options.version_len;
   return SUCCESS;
}

/**
 * \brief Set Match Rule based on Model and Version
 * \param uc_vendor_len   Firmware Upgrade Vendor Length
 * \param puc_vendor      Firmware Upgrade Vendor
 * \param uc_model_len    Firmware Upgrade Model Length
 * \param puc_model       Firmware Upgrade Model
 * \param uc_version_len  Firmware Upgrade Version Length
 * \param puc_version     Firmware Upgrade Version
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
*/
int bmng_fup_set_fw_data_request_sync(uint8_t uc_vendor_len, uint8_t * puc_vendor, uint8_t uc_model_len, uint8_t * puc_model, uint8_t uc_version_len, uint8_t * puc_version, struct TmacSetConfirm *pmacSetConfirm)
{
  PRIME_LOG(LOG_DEBUG,"bmng_fup_set_fw_data_request_sync\r\n");

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     bmng_fup_set_fw_data_request is asyncronous - Base Node will callback request
     with prime_bmng_fup_ack_ind_msg_cb cmd=bmng_fup_set_fw_data_request
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_fup_set_fw_data_request_cmd;
  /* Default Status */
  pmacSetConfirm->m_u8Status = -1;

  // Send the asynchronous request
  bmng_fup_set_fw_data_request(uc_vendor_len,puc_vendor,uc_model_len,puc_model,uc_version_len,puc_version);

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(PRIME_SYNC_TIMEOUT, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (!g_prime_sync_mgmt.f_sync_res){
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_ack_ind_msg_cb not received\r\n");
  }else if (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != FUP_ACK_OK){
    // Error sending reboot request
    pmacSetConfirm->m_u8Status = g_prime_sync_mgmt.s_macSetConfirm.m_u8Status;
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_set_fw_data_request_sync failed\r\n");
  }else{
    pmacSetConfirm->m_u8Status = PPROF_ACK_OK;
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"bmng_fup_set_fw_data_request_sync result=%d\r\n",pmacSetConfirm->m_u8Status);

  return pmacSetConfirm->m_u8Status;
}

/**
 * \brief Init File Transmission Request
 * \param us_num_frames   Number of frames to be sent
 * \param ui_file_size    File Size to be sent
 * \param us_frame_size   Frame Size
 * \param ui_crc          File CRC
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
*/
int bmng_fup_init_file_tx_request_sync(uint16_t us_num_frames, uint32_t ui_file_size, uint16_t us_frame_size, uint32_t ui_crc, struct TmacSetConfirm *pmacSetConfirm)
{

  PRIME_LOG(LOG_DEBUG,"prime_bmng_fup_init_file_tx_request_sync\r\n");

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     bmng_fup_clear_target_list_request is asyncronous - Base Node will callback request
     with prime_bmng_fup_ack_ind_msg_cb cmd=bmng_fup_add_target_request
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_fup_init_file_tx_request_cmd;
  /* Default Status */
  pmacSetConfirm->m_u8Status = -1;

  // Send the asynchronous request
  bmng_fup_init_file_tx_request(us_num_frames,ui_file_size,us_frame_size,ui_crc);

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(PRIME_SYNC_TIMEOUT, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (!g_prime_sync_mgmt.f_sync_res){
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_ack_ind_msg_cb not received\r\n");
  }else if (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != FUP_ACK_OK){
    // Error sending reboot request
    pmacSetConfirm->m_u8Status = g_prime_sync_mgmt.s_macSetConfirm.m_u8Status;
    PRIME_LOG(LOG_ERR,"ERROR: prime_bmng_fup_init_file_tx_request_sync failed\r\n");
  }else{
    pmacSetConfirm->m_u8Status = PPROF_ACK_OK;
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"prime_bmng_fup_init_file_tx_request_sync result=%d\r\n",pmacSetConfirm->m_u8Status);

  return pmacSetConfirm->m_u8Status;
}

/**
 * \brief Init File Transmission Request
 * \param us_frame_num    Frame Number control
 * \param us_len          Frame Length
 * \param puc_data        Frame Pointer
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
*/
int bmng_fup_data_frame_request_sync(uint16_t us_frame_num, uint16_t us_len, uint8_t * puc_data, struct TmacSetConfirm *pmacSetConfirm)
{

  PRIME_LOG(LOG_DEBUG,"prime_bmng_fup_data_frame_request_sync\r\n");

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     bmng_fup_clear_target_list_request is asyncronous - Base Node will callback request
     with prime_bmng_fup_ack_ind_msg_cb cmd=bmng_fup_add_target_request
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_fup_data_frame_request_cmd;
  /* Default Status */
  pmacSetConfirm->m_u8Status = -1;

  // Send the asynchronous request
  bmng_fup_data_frame_request(us_frame_num,us_len,puc_data);

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(PRIME_SYNC_TIMEOUT, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (!g_prime_sync_mgmt.f_sync_res){
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_ack_ind_msg_cb not received\r\n");
  }else if (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != FUP_ACK_OK){
    // Error sending reboot request
    pmacSetConfirm->m_u8Status = g_prime_sync_mgmt.s_macSetConfirm.m_u8Status;
    PRIME_LOG(LOG_ERR,"ERROR: prime_bmng_fup_data_frame_request_sync failed\r\n");
  }else{
    pmacSetConfirm->m_u8Status = PPROF_ACK_OK;
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"prime_bmng_fup_data_frame_request_sync result=%d\r\n",pmacSetConfirm->m_u8Status);

  return pmacSetConfirm->m_u8Status;
}

/**
 * \brief Check Firmware Update CRC
 * \param uc_enable       Enable FW Upgrade process
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
*/
int bmng_fup_start_fu_request_sync(uint8_t uc_enable, struct TmacSetConfirm *pmacSetConfirm)
{
	PRIME_LOG(LOG_DEBUG,"bmng_fup_start_fu_request_sync\r\n");

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     bmng_fup_start_fu_request is asyncronous - Base Node will callback request
     with prime_bmng_fup_ack_ind_msg_cb cmd=prime_bmng_fup_start_fu_request_cmd
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_fup_start_fu_request_cmd;
  /* Default Status */
  pmacSetConfirm->m_u8Status = -1;

  // Send the asynchronous request
  bmng_fup_start_fu_request(uc_enable);

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(PRIME_SYNC_TIMEOUT, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (!g_prime_sync_mgmt.f_sync_res){
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_ack_ind_msg_cb not received\r\n");
  }else if (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != FUP_ACK_OK){
    // Error sending reboot request
    pmacSetConfirm->m_u8Status = g_prime_sync_mgmt.s_macSetConfirm.m_u8Status;
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_start_fu_request_sync failed\r\n");
  }else{
    pmacSetConfirm->m_u8Status = PPROF_ACK_OK;
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"bmng_fup_start_fu_request_sync result=%d\r\n",pmacSetConfirm->m_u8Status);

  return pmacSetConfirm->m_u8Status;
}

/**
 * \brief Check Firmware Update CRC
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
*/
int bmng_fup_check_crc_request_sync(struct TmacSetConfirm *pmacSetConfirm)
{

  PRIME_LOG(LOG_DEBUG,"prime_bmng_fup_check_crc_request_sync\r\n");

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     bmng_fup_clear_target_list_request is asyncronous - Base Node will callback request
     with prime_bmng_fup_ack_ind_msg_cb cmd=bmng_fup_add_target_request
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_fup_check_crc_request_cmd;
  /* Default Status */
  pmacSetConfirm->m_u8Status = -1;

  // Send the asynchronous request
  bmng_fup_check_crc_request();

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(PRIME_SYNC_TIMEOUT, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (!g_prime_sync_mgmt.f_sync_res){
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_ack_ind_msg_cb not received\r\n");
  }else if (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != FUP_ACK_OK){
    // Error sending reboot request
    pmacSetConfirm->m_u8Status = g_prime_sync_mgmt.s_macSetConfirm.m_u8Status;
    PRIME_LOG(LOG_ERR,"ERROR: prime_bmng_fup_check_crc_request_sync failed\r\n");
  }else{
    pmacSetConfirm->m_u8Status = PPROF_ACK_OK;
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"prime_bmng_fup_check_crc_request_sync result=%d\r\n",pmacSetConfirm->m_u8Status);

  return pmacSetConfirm->m_u8Status;
}

/**
 * \brief Request FW Version, Model and Vendor to Service Node
 * \param puc_eui48        EUI48 to be requested
 * \param pmacGetConfirm   Get Confirm Structure
 *
 * \return
*/
int bmng_fup_get_version_request_sync(uint8_t * puc_eui48, struct TmacGetConfirm *pmacGetConfirm)
{
  PRIME_LOG(LOG_DEBUG,"bmng_fup_get_version_request_sync eui48=%s\r\n",eui48_to_str(puc_eui48,NULL));

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     bmng_fup_get_version_request is asyncronous - Base Node will callback request
     with prime_bmng_fup_ack_ind_msg_cb cmd=bmng_fup_get_version_request
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_fup_get_version_request_cmd;
  /* Default Status */
  pmacGetConfirm->m_u8Status = -1;

  // Send the asynchronous request
  bmng_fup_get_version_request(puc_eui48);

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(PRIME_SYNC_TIMEOUT, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (!g_prime_sync_mgmt.f_sync_res){
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_ack_ind_msg_cb not received\r\n");
  }else if (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != FUP_ACK_OK){
    // Error sending reboot request
    pmacGetConfirm->m_u8Status = g_prime_sync_mgmt.s_macGetConfirm.m_u8Status;
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_get_version_request_sync failed\r\n");
  }else{
    pmacGetConfirm->m_u8Status = PPROF_ACK_OK;
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"bmng_fup_get_version_request_sync result=%d\r\n",pmacGetConfirm->m_u8Status);

  return pmacGetConfirm->m_u8Status;

}

/**
 * \brief Request FW Upgrade State
 * \param puc_eui48        EUI48 to be requested
 * \param pmacGetConfirm   Get Confirm Structure
 *
 * \return
*/
int bmng_fup_get_state_request_sync(uint8_t * puc_eui48, struct TmacGetConfirm *pmacGetConfirm)
{
  PRIME_LOG(LOG_DEBUG,"bmng_fup_get_state_request_sync eui48=%s\r\n",eui48_to_str(puc_eui48,NULL));

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     bmng_fup_get_state_request is asyncronous - Base Node will callback request
     with prime_bmng_fup_state_ind_msg_cb
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_fup_get_state_request_cmd;
  /* Default Status */
  pmacGetConfirm->m_u8Status = -1;

  // Send the asynchronous request
  bmng_fup_get_state_request(puc_eui48);

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(PRIME_SYNC_TIMEOUT, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (!g_prime_sync_mgmt.f_sync_res){
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_ack_ind_msg_cb not received\r\n");
  }else if (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != FUP_ACK_OK){
    // Error sending reboot request
    pmacGetConfirm->m_u8Status = g_prime_sync_mgmt.s_macGetConfirm.m_u8Status;
    PRIME_LOG(LOG_ERR,"ERROR: bmng_fup_get_state_request_sync failed\r\n");
  }else{
    pmacGetConfirm->m_u8Status = PPROF_ACK_OK;
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"bmng_fup_get_state_request_sync result=%d\r\n",pmacGetConfirm->m_u8Status);

  return pmacGetConfirm->m_u8Status;

}

/**
 * \brief Set FW Upgrade Signature Options
 * \param options  Firmware Options
 *
 * \return
*/
int fw_upgrade_set_signature_options(struct TfwUpgradeOptions *options)
{
   fu_options.sign_algo = options->sign_algo;
	 fu_options.sign_size = options->sign_size;
   return SUCCESS;
}

/**
 * \brief Get Signature options
 * \param options  Firmware Upgrade options
 *
 * \return
*/
int fw_upgrade_get_signature_options(struct TfwUpgradeOptions *options)
{
	 options->sign_algo = fu_options.sign_algo;
	 options->sign_size = fu_options.sign_size;
   return SUCCESS;
}

/**
 * \brief Set FW Upgrade Binary File Path
 * \param filepath  Path To FW Upgrade Binary File
 *
 * \return
*/
int fw_upgrade_set_binary_path(struct TfwUpgradeOptions *options)
{
   memcpy(&fu_options.binarypath,options->binarypath,64);
   return SUCCESS;
}

/**
 * \brief Get Match Rule options
 * \param options        Firmware Upgrade options
 *
 * \return
*/
int fw_upgrade_get_binary_path(struct TfwUpgradeOptions *options)
{
   memcpy(options->binarypath, &fu_options.binarypath, 64);
   return SUCCESS;
}

/**
 * \brief   Base Management Firmware Download
 */
int fw_upgrade_download(void)
{
/*********************************************
*       Local Envars                         *
**********************************************/
  struct TmacSetConfirm x_pib_confirm;
  uint32_t chunksize = CHUNKSIZE;
  uint32_t nbytes, filesize, filecrc;
  uint16_t frame_num,num_tries,num_tries_frame;
  FILE *firmware;
  uint8_t buffer[CHUNKSIZE];
  uint8_t rule = 0;
	uint8_t additional_frame = 0;
  struct stat st;
  prime_sn * sn;
  mchp_list *entry, *tmp;

/*********************************************************
*       Code                                             *
*********************************************************/
    PRIME_LOG(LOG_DEBUG,"prime_bmng_fup_start_process\r\n");

    /* Check Firmware File Exists and can be opened */
    firmware = fopen ((const char *) fu_options.binarypath, "rb");
    if (firmware == NULL){
      PRIME_LOG(LOG_ERR,"Error opening %s firmware upgrade file\r\n", (const char *) fu_options.binarypath);
      return ERROR_FW_UPGRADE_FILEPATH;
    }

    // Set UPGRADE Options
    bmng_fup_set_upg_options_request_sync(fu_options.arq_en, fu_options.pagesize, fu_options.mult_en, fu_options.delay, fu_options.timer, &x_pib_confirm);
    if (x_pib_confirm.m_u8Status != FUP_ACK_OK){
        PRIME_LOG(LOG_ERR,"Error setting Firmware Upgrade Options\r\n");
				fclose(firmware);
				return ERROR_FW_UPGRADE_SET_OPTIONS;
    }

    // Clear Target List
    bmng_fup_clear_target_list_request_sync(&x_pib_confirm);
    if (x_pib_confirm.m_u8Status != FUP_ACK_OK){
        PRIME_LOG(LOG_ERR,"Error clearing Firmware Upgrade Target List\r\n");
				fclose(firmware);
				return ERROR_FW_UPGRADE_CLEAR_TARGET_LIST;
    }

    // Add Target List
    list_for_each_safe(entry, tmp, &prime_network) {
       sn = list_entry(entry, prime_sn, list);
       if (sn->fwup_en){
          bmng_fup_add_target_request_sync(sn->regEntryID, &x_pib_confirm);
          if (x_pib_confirm.m_u8Status != FUP_ACK_OK){
            PRIME_LOG(LOG_ERR,"Error adding %s to Firmware Upgrade list (0x%X)\r\n",eui48_to_str(sn->regEntryID,NULL),x_pib_confirm.m_u8Status);
						fclose(firmware);
						return ERROR_FW_UPGRADE_ADD_TARGET;
          }
       }
    }

    // Set Firmware Image Information
    bmng_fup_set_fw_data_request_sync(fu_options.vendor_len, fu_options.vendor,fu_options.model_len, fu_options.model,fu_options.version_len, fu_options.version, &x_pib_confirm);
    if (x_pib_confirm.m_u8Status != FUP_ACK_OK){
        PRIME_LOG(LOG_ERR,"Error setting Firmware Upgrade Data Information\r\n");
				fclose(firmware);
				return ERROR_FW_UPGRADE_SET_DATA;
    }

		// Set Match Rule
		if (fu_options.filter_model)
			rule |= 0x02;
		if (fu_options.filter_vendor)
			rule |= 0x04;

		bmng_fup_set_match_rule_request_sync(rule, &x_pib_confirm);
		if (x_pib_confirm.m_u8Status != FUP_ACK_OK){
				PRIME_LOG(LOG_ERR,"Error setting Firmware Upgrade Match Rule\r\n");
				fclose(firmware);
				return ERROR_FW_UPGRADE_SET_RULE;
		}

		// Set Signature Data Request
		if (fu_options.sign_algo != 0){
			prime_bmng_fup_set_signature_data_request_sync(fu_options.sign_algo, fu_options.sign_size, &x_pib_confirm);
    	if (x_pib_confirm.m_u8Status != FUP_ACK_OK){
        	PRIME_LOG(LOG_ERR,"Error setting Firmware Upgrade Data Information\r\n");
					fclose(firmware);
					return ERROR_FW_UPGRADE_SET_DATA;
    	}
		}

    // Init File Transfer
    /* Calculate Filesize */
    stat(fu_options.binarypath, &st);
    filesize = st.st_size;
		PRIME_LOG(LOG_DBG,"Firmware Upgrade Filesize: 0x%08X\r\n",filesize);
    /* Calculate CRC */
    filecrc = fu_eval_crc_32_file(firmware);
		PRIME_LOG(LOG_DBG,"Firmware Upgrade CRC: 0x%08X\r\n",filecrc);
    num_tries = MAX_NUM_TRIES_TRANSFER;
		/* Number of frames to be sent */
		chunksize = CHUNKSIZE;
    frame_num = (filesize / chunksize) ;
		/* Check if a last frame with size 0 bytes need to be sent */
		if ((filesize % chunksize) == 0)
		   additional_frame = 1;

    while (num_tries){
      bmng_fup_init_file_tx_request_sync(frame_num + 1, filesize, chunksize /*fu_options.pagesize */, filecrc, &x_pib_confirm);
      if (x_pib_confirm.m_u8Status != FUP_ACK_OK){
          PRIME_LOG(LOG_ERR,"Error setting Firmware Upgrade Init File Tx Request\r\n");
					fclose(firmware);
          return ERROR_FW_UPGRADE_INIT_FILE_TX;
      }
      // Data File Transfer to USI
      frame_num = 1;                              /* Number of frames (chunks) sent */
      num_tries_frame = MAX_NUM_TRIES_TRANSFER;   /* Maximum Tries to send a frame (chunk) */
      fseek(firmware,0,SEEK_SET);                 /* Set Pointer to the beginning of the file */
      nbytes = fread (buffer, sizeof(uint8_t), chunksize, firmware);
      while (((nbytes != 0) || additional_frame) && num_tries_frame)
      {
         /* Send to Base Node Modem  */
         num_tries_frame--; /* Discount one try */
         bmng_fup_data_frame_request_sync(frame_num, nbytes, buffer, &x_pib_confirm);
         if (x_pib_confirm.m_u8Status == FUP_ACK_OK){
            /* Read next chunk */
            nbytes = fread (buffer, sizeof(uint8_t), chunksize, firmware);
            frame_num++;
            num_tries_frame = MAX_NUM_TRIES_TRANSFER;
         }
				 if (additional_frame && (nbytes == 0))
				    additional_frame = 0;
				 usleep(250);
      }
      if (!num_tries_frame){
         /* Impossible to transfer the full file to base node modem */
         /* Decrease Transefer Frame Size */
         chunksize >>= 2;
         num_tries--;
      }else{
				 PRIME_LOG(LOG_INFO,"Firmware Upgrade sent to Base Node Modem\r\n");
				 break;
			}
    }
    if (!num_tries){
      /* File Transfer failed! */
      PRIME_LOG(LOG_ERR,"Error sending firmware upgrade file to base node modem\r\n");
			fclose(firmware);
			return -1;
    }
    fclose(firmware);

    // Check CRC of File Transfered via USI
    bmng_fup_check_crc_request_sync(&x_pib_confirm);
    if (x_pib_confirm.m_u8Status != FUP_ACK_OK){
        PRIME_LOG(LOG_ERR,"Error sending Check CRC Request\r\n");
        return ERROR_FW_UPGRADE_CRC_CHECK;
    }
		fup_global_status = FW_UPGRADE_IDLE;
    return SUCCESS;
}

/**
 * \brief   Base Management Firmware Download
 */
int fw_upgrade_start(uint8_t enable)
{
/*********************************************
*       Local Envars                         *
**********************************************/
  struct TmacSetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
	// Start Firwmare Upgrade
	bmng_fup_start_fu_request_sync(enable, &x_pib_confirm);
	if (x_pib_confirm.m_u8Status != FUP_ACK_OK){
			PRIME_LOG(LOG_ERR,"Impossible to run Firmware Upgrade\r\n");
			return ERROR_FW_UPGRADE_REQUEST;
	}
  fup_global_status = FW_UPGRADE_RUNNING /* FUP_STATUS_RUNNING */ ;
  return SUCCESS;
}

/**
 * \brief   Firmware Upgrade Status
 */
int fw_upgrade_status()
{
/*********************************************
*       Local Envars                         *
**********************************************/
/*********************************************
*       Code                                 *
**********************************************/
  return fup_global_status;
}

/**
 * \brief   Base Management Callback Firmware Update ACK Indication
 *
 * \param   Command response
 * \param   Ack Value
 * \param   Data, optional. If it a response to a data FUP message, contains the frame number.
 */
void prime_bmng_fup_ack_ind_msg_cb(uint8_t uc_cmd, uint8_t uc_ack, uint16_t us_data)
{
    PRIME_LOG(LOG_DEBUG,"bmng_fup_ack_ind_msg_cb uc_cmd=0x%02X  uc_ack=0x%02X us_data=0x%04X\r\n", uc_cmd,uc_ack,us_data);
    memset(&g_prime_sync_mgmt.s_macSetConfirm,0,sizeof(struct TmacSetConfirm));
    if (g_prime_sync_mgmt.f_sync_req && (g_prime_sync_mgmt.m_u16AttributeId == uc_cmd)){
       if ((uc_cmd != prime_bmng_fup_get_version_request_cmd) && (uc_cmd != prime_bmng_fup_get_state_request_cmd)){
          /* Only for Asyncronous Set && Reboot - For Get Request syncronism with response_cb */
          g_prime_sync_mgmt.s_macSetConfirm.m_u8Status = uc_ack;
          g_prime_sync_mgmt.s_macSetConfirm.m_u16AttributeId = uc_cmd;
          memcpy(g_prime_sync_mgmt.s_macSetConfirm.m_au8AttributeValue,&us_data,2);
          g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength = 2;
          g_prime_sync_mgmt.f_sync_res = true;
       }
    }
}

/**
 * \brief   Base Management Callback Firmware Update Error Indication
 *
 * \param   uc_error_code_:   Error code
 * \param   puc_eui48:        EUI48 of the Service node
 */
void prime_bmng_fup_error_ind_msg_cb(uint8_t uc_error_code, uint8_t* puc_eui48)
{
    PRIME_LOG(LOG_DEBUG,"bmng_fup_error_ind_msg_cb uc_error_code=0x%02X eui48=%s\r\n", uc_error_code, eui48_to_str(puc_eui48,NULL));
}

/**
 * \brief   Base Management Callback Firmware Update Kill Indication
 *          Response to a FU_ABORT directive.
 *
 * \param   puc_eui48:        EUI48 of the Service node
 */
void prime_bmng_fup_kill_ind_msg_cb(uint8_t* puc_eui48)
{
  PRIME_LOG(LOG_DEBUG,"prime_bmng_fup_kill_ind_msg_cb eui48=%s\r\n",eui48_to_str(puc_eui48,NULL));
}

/**
 * \brief   Base Management Callback Firmware Update Status Indication
 *
 * \param   uc_status:        PRIME FU status
 * \param   uc_pages:         Number of pages received by the SN
 * \param   puc_eui48:        EUI48 of the Service node
 */
void prime_bmng_fup_status_ind_msg_cb(uint8_t uc_status, uint32_t uc_pages, uint8_t* puc_eui48)
{
/*********************************************
*       Local Envars                         *
**********************************************/
prime_sn *sn;
char broadcast_address[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
/*********************************************
*       Code                                 *
**********************************************/
  PRIME_LOG(LOG_DEBUG,"prime_bmng_fup_status_ind_msg_cb uc_status=0x%02X(%s) uc_pages=0x%04X eui48=%s\r\n", uc_status, fup_state_to_str(uc_status, NULL), uc_pages, eui48_to_str(puc_eui48,NULL));
	if (uc_status == FUP_STATE_EXCEPTION){
		 PRIME_LOG(LOG_ERR,"prime_bmng_fup_status_ind_msg_cb exception=%s\r\n", fup_state_exception_to_str(uc_pages,NULL));
	}
	if ((memcmp(puc_eui48,broadcast_address,6)==0) && (uc_pages == 0) && (uc_status == FUP_STATE_CONFIRMED)){
		 /* End of Firmwarew Upgrade */
	   fup_global_status = FW_UPGRADE_FINISH;
		 PRIME_LOG(LOG_INFO,"Firmware Upgrade Finished\r\n");
	}

	sn = prime_network_find_sn(&prime_network,puc_eui48);
	if (sn == (prime_sn *) NULL){
		 PRIME_LOG(LOG_DBG,"Service Node %s not registered\r\n",eui48_to_str(puc_eui48,NULL));
	}else{
		 prime_network_mutex_lock();
		 sn->fu_state = uc_status;
		 sn->fu_pages = uc_pages;
		 prime_network_mutex_unlock();
	}
	if (g_prime_sync_mgmt.f_sync_req) {
			g_prime_sync_mgmt.s_macGetConfirm.m_u8Status = FUP_ACK_OK;
			/* Information saved on Service Node Entry */
			g_prime_sync_mgmt.f_sync_res = true;
	}
}

/**
 * \brief   Base Management Callback Firmware Update Version Indication
 *
 * \param   uc_vendor_len:    Length of the vendor value
 * \param   puc_vendor:       Buffer containing vendor value
 * \param   uc_model_len:     Length of the model value
 * \param   puc_model:        Buffer containing model value
 * \param   uc_version_len:   Length of the version value
 * \param   puc_version:      Buffer containing version value
 * \param   puc_eui48:        EUI48 of the Service node
 */
void prime_bmng_fup_version_ind_msg_cb(uint8_t* puc_eui48,
                                    uint8_t uc_vendor_len, uint8_t * puc_vendor,
                                    uint8_t uc_model_len,  uint8_t * puc_model,
                                    uint8_t uc_version_len, uint8_t * puc_version)
{
/*********************************************
*       Local Envars                         *
**********************************************/
prime_sn *sn;
/*********************************************
*       Code                                 *
**********************************************/
  PRIME_LOG(LOG_DEBUG,"prime_bmng_fup_version_ind_msg_cb\r\n\t- EUI48\t:%s\r\n\t- Vendor\t:%s\r\n\t- Model\t:%s\r\n\t- Version\t:%s\r\n", eui48_to_str(puc_eui48,NULL), puc_vendor, puc_model, puc_version);
	sn = prime_network_find_sn(&prime_network,puc_eui48);
  if (sn == (prime_sn *) NULL){
     PRIME_LOG(LOG_DBG,"Service Node not registered\r\n");
  }else{
		 prime_network_mutex_lock();
		 memcpy(sn->fu_version, puc_version, uc_version_len);
		 memcpy(sn->fu_model, puc_model, uc_model_len);
		 memcpy(sn->fu_vendor, puc_vendor, uc_vendor_len);
		 prime_network_mutex_unlock();
	}
	if (g_prime_sync_mgmt.f_sync_req) {
			g_prime_sync_mgmt.s_macGetConfirm.m_u8Status = FUP_ACK_OK;
			/* Information saved on Service Node Entry */
			g_prime_sync_mgmt.f_sync_res = true;
	}
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
