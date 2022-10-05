/**
 * \file
 *
 * \brief Base Node Management FW Upgrade Header.
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

#ifndef _BASE_NODE_MNG_FW_UPGRADE_H_
#define _BASE_NODE_MNG_FW_UPGRADE_H_

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

// Firmware Upgrade Options
struct TfwUpgradeOptions{
  uint8_t   mult_en;          // 0: Unicast 1: Multicast
  uint16_t  arq_en;           // ARQ Enable
  uint32_t  pagesize;         // Page Size
  uint32_t  delay;            // Restart Delay
  uint32_t  timer;            // Safety Timer
  uint8_t   filter_model;     // Model Filter
  uint8_t   filter_vendor;    // Vendor Filter
  uint8_t   sign_algo;        // Signature Algorithm
  uint16_t  sign_size;        // Signature Size
  uint8_t   vendor[32];       // Vendor
  uint8_t   vendor_len;       // Vendor Length
  uint8_t   model[32];        // Model
  uint8_t   model_len;        // Model Length
  uint8_t   version[32];      // Version
  uint8_t   version_len;      // Version Length
  char      binarypath[64];   // Firmware Upgrade Binary Path
};
typedef struct TfwUpgradeOptions fwUpgradeOptions;

typedef struct {
        uint8_t code : 7;
        uint8_t is_permanent : 1;
        uint8_t manuf_code;
} fu_exception_t;

/* FW Upgrade Status */
typedef enum {
    FW_UPGRADE_IDLE        = 0,
    FW_UPGRADE_RUNNING     = 1,
    FW_UPGRADE_FINISH      = 2
} fw_upgrade_status_t;

/**
 * \brief Set Firmware Upgrade Options
 * \param  options -> Firmware Upgrade Options
 *
 * \return
*/
int fw_upgrade_set_upg_options(struct TfwUpgradeOptions *options);

/**
 * \brief Get Firmware Upgrade Options
 * \param  options -> Firmware Upgrade Options
 *
 * \return
*/
int fw_upgrade_get_upg_options(struct TfwUpgradeOptions *options);

/**
 * \brief Set Match Rule based on Model and Version. Match rules: 0000 0MV0
 *        If M and/or V are set, only the nodes matching model and/or vendor will be upgraded.
 *        The decision will be taken on Base Node
 * \param options        Firmware Upgrade options
 *
 * \return
*/
int fw_upgrade_set_match_rule(struct TfwUpgradeOptions *options);

/**
 * \brief Get Match Rule options
 * \param options        Firmware Upgrade options
 *
 * \return
*/
int fw_upgrade_get_match_rule(struct TfwUpgradeOptions *options);

/**
 * \brief Set FW Data Info based on Vendor,Model and Version
 * \param options        Firmware Upgrade options
 *
 * \return
*/
int fw_upgrade_set_fw_data_info(struct TfwUpgradeOptions *options);

/**
 * \brief Get FW Data Info
 * \param options        Firmware Upgrade options
 *
 * \return
*/
int fw_upgrade_get_fw_data_info(struct TfwUpgradeOptions *options);

/**
 * \brief Clear Base Node FW Upgrade List
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
*/
int bmng_fup_clear_target_list_request_sync(struct TmacSetConfirm *pmacSetConfirm);

/**
 * \brief Add Service Node to FW Upgrade List
 * \param puc_eui48       Service Node EUI48
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
*/
int bmng_fup_add_target_request_sync(uint8_t * puc_eui48, struct TmacSetConfirm *pmacSetConfirm);

/*
 * \brief Abort the FW Upgrade process for a Service Node
 * \param puc_eui48       Service Node EUI48
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
 */
int bmng_fup_abort_fu_request_sync(uint8_t * puc_eui48, struct TmacSetConfirm *pmacSetConfirm);

/*
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
int bmng_fup_set_upg_options_request_sync(uint8_t uc_arq_en, uint8_t uc_page_size, uint8_t uc_mult_en, uint32_t ui_delay, uint32_t ui_timer, struct TmacSetConfirm *pmacSetConfirm);

/*
 * \brief Set Match Rule based on Model and Version
 * \param uc_model        Model version
 * \param uc_vendor       Vendor
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
 */
int bmng_fup_set_match_rule_request_sync(uint8_t uc_rule, struct TmacSetConfirm *pmacSetConfirm);

/**
 * \brief Set Signature Data Request
 * \param uc_sig_algo      Signature Algorithm
 * \param us_sig_len       Signature Length
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
*/
int prime_bmng_fup_set_signature_data_request_sync(uint8_t uc_sig_algo, uint16_t us_sig_len, struct TmacSetConfirm *pmacSetConfirm);

/*
 * \brief Set FW Information
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
int bmng_fup_set_fw_data_request_sync(uint8_t uc_vendor_len, uint8_t * puc_vendor, uint8_t uc_model_len, uint8_t * puc_model, uint8_t uc_version_len, uint8_t * puc_version, struct TmacSetConfirm *pmacSetConfirm);

/**
 * \brief Check Firmware Update CRC
 * \param uc_enable       Enable FW Upgrade process
 * \param pmacSetConfirm  Confirm Structure
 *
 * \return
*/
int bmng_fup_start_fu_request_sync(uint8_t uc_enable, struct TmacSetConfirm *pmacSetConfirm);

/*
 * \brief   Base Management Callback Firmware Update ACK Indication
 *
 * \param   Command response
 * \param   Ack Value
 * \param   Data, optional. If it a response to a data FUP message, contains the frame number.
 */
void prime_bmng_fup_ack_ind_msg_cb(uint8_t uc_cmd, uint8_t uc_ack, uint16_t us_data);

/*
 * \brief   Base Management Callback Firmware Update Error Indication
 *
 * \param   uc_error_code_:   Error code
 * \param   puc_eui48:        EUI48 of the Service node
 */
void prime_bmng_fup_error_ind_msg_cb(uint8_t uc_error_code, uint8_t* puc_eui48);

/*
 * \brief   Base Management Callback Firmware Update Kill Indication
 *          Response to a FU_ABORT directive.
 *
 * \param   puc_eui48:        EUI48 of the Service node
 */
void prime_bmng_fup_kill_ind_msg_cb(uint8_t* puc_eui48);

/*
 * \brief   Base Management Callback Firmware Update Status Indication
 *
 * \param   uc_status:        PRIME FU status
 * \param   uc_pages:         Number of pages received by the SN
 * \param   puc_eui48:        EUI48 of the Service node
 */
void prime_bmng_fup_status_ind_msg_cb(uint8_t uc_status, uint32_t uc_pages, uint8_t* puc_eui48);

/*
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
                                    uint8_t uc_version_len, uint8_t * puc_version);

/*
 * \brief Request FW Version, Model and Vendor to Service Node
 * \param puc_eui48        EUI48 to be requested
 * \param pmacGetConfirm   Get Confirm Structure
 *
 * \return
 */
int bmng_fup_get_version_request_sync(uint8_t * puc_eui48, struct TmacGetConfirm *pmacGetConfirm);

/**
 * \brief Request FW Upgrade State
 * \param puc_eui48        EUI48 to be requested
 * \param pmacGetConfirm   Get Confirm Structure
 *
 * \return
*/
int bmng_fup_get_state_request_sync(uint8_t * puc_eui48, struct TmacGetConfirm *pmacGetConfirm);

/*
 * \brief Set FW Upgrade Binary File Path
 * \param filepath  Path To FW Upgrade Binary File
 *
 * \return
 */
int fw_upgrade_set_binary_path(struct TfwUpgradeOptions *options);

/*
 * \brief Get Match Rule options
 * \param options        Firmware Upgrade options
 *
 * \return
 */
int fw_upgrade_get_binary_path(struct TfwUpgradeOptions *options);

/**
 * \brief Set FW Upgrade Signature Options
 * \param options  Firmware Options
 *
 * \return
*/
int fw_upgrade_set_signature_options(struct TfwUpgradeOptions *options);

/**
 * \brief Get Signature options
 * \param options  Firmware Upgrade options
 *
 * \return
*/
int fw_upgrade_get_signature_options(struct TfwUpgradeOptions *options);

/**
 * \brief   Base Management Firmware Download
 */
int fw_upgrade_download(void);

/**
 * \brief   Base Management Firmware Download
 */
int fw_upgrade_start(uint8_t enable);

/**
 * \brief   Base Management Firmware Download
 */
int fw_upgrade_status();

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */

#endif  // _BASE_NODE_MNG_FW_UPGRADE_H_
