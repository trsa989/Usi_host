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

#ifndef BS_FUNCTIONS_H
#define BS_FUNCTIONS_H

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

#include <stdbool.h>
#include <ProtoEapPsk.h>

#define BS_MAX_JOIN_TIME  250000

/* #define LOG_BOOTSTRAP(a)   printf a */
#define LOG_BOOTSTRAP(a)   (void)0

/* It enables the network building strategy by levels,
 * this strategy allows to register nodes in an ordered manner depending
 * on how far the nodes are based in Hop Count*/
/* #define BS_LEVEL_STRATEGY_ENABLED */

/*Defines the minimum time in seconds where not new nodes
 * are registered before starting to register nodes in next level.
 * This ensures the network is built in an ordered manner */
#define BS_LEVEL_STRATEGY_TIMEOUT_INTERVAL  60

/* Set RandS to a fixed value (default is random) */
/* #define FIXED_RAND_S */
/* PAN ID */
extern uint16_t g_u16NetworkId;

/* Status codes related to HostInterface processing */
enum e_bootstrap_slot_state {
	BS_STATE_WAITING_JOINNING = 0,
	BS_STATE_SENT_EAP_MSG_1,
	BS_STATE_WAITING_EAP_MSG_2,
	BS_STATE_SENT_EAP_MSG_3,
	BS_STATE_WAITING_EAP_MSG_4,
	BS_STATE_SENT_EAP_MSG_ACCEPTED,
	BS_STATE_SENT_EAP_MSG_DECLINED,
};

/* BOOTSTRAP_NUM_SLOTS defines the number of parallel bootstrap procedures that must be carried out */
#define BOOTSTRAP_NUM_SLOTS 5
#define BOOTSTRAP_MSG_MAX_RETRIES 1

typedef struct {
	enum e_bootstrap_slot_state e_state;
	struct TAdpExtendedAddress m_LbdAddress;
	uint16_t us_lba_src_addr;
	uint16_t us_assigned_short_address;
	uint8_t uc_tx_handle;
	uint32_t ul_timeout;
	uint8_t uc_tx_attemps;
	struct TEapPskRand m_randS;
	uint32_t ul_nonce;
	uint8_t uc_pending_confirms;
	uint8_t uc_pending_tx_handler;
	uint8_t uc_lbp_hops;
	unsigned char auc_data[100];
	unsigned short us_data_length;
	struct TEapPskContext m_PskContext;
#ifdef G3_HYBRID_PROFILE
  	uint8_t m_u8MediaType;
  	uint8_t m_u8DisableBackupMedium;
#endif
} t_bootstrap_slot;
struct TAddress {
	uint8_t m_u8AddrLength;
	union {
		uint16_t m_u16ShortAddr;
		uint8_t m_u8ExtendedAddr[8];
	};
};

/* Status codes related to HostInterface processing */
enum ELbsStatus {
	LBS_STATUS_SUCCESS = 0,
	LBS_STATUS_NOT_ALLOWED,
	LBS_STATUS_UNKNOWN_COMMAND,
	LBS_STATUS_INVALID_PARAMETER
};

/* Indications to be notified to the upper layers */
enum lbp_indications {
	LBS_NONE = 0,
	LBS_KICK
};

#define MAC_SET_REQUEST_VALUE_LEN    16
struct TCbMsgMacSetRequest {
	uint32_t m_u32AttributeId;
	uint16_t m_u16AttributeIndex;
	uint8_t m_u8AttributeLength;
	uint8_t m_pu8AttributeValue[MAC_SET_REQUEST_VALUE_LEN];
};

/* HiMsgMacSetConfirm parameters definition */
struct TCbMsgMacSetConfirm {
	uint8_t m_u8Status;
	uint32_t m_u32AttributeId;
	uint16_t m_u16AttributeIndex;
};

struct TCbMsgAdpInitializeConfirm {
	uint8_t m_u8Status;
};

struct TCbMsgAdpNetworkStartRequest {
	uint16_t m_u16PanId;
};

#define ADP_SET_REQUEST_VALUE_LEN    28
struct TCbMsgAdpSetRequest {
	uint32_t m_u32AttributeId;
	uint16_t m_u16AttributeIndex;
	uint8_t m_u8AttributeLength;
	uint8_t m_pu8AttributeValue[ADP_SET_REQUEST_VALUE_LEN];
};

#define APD_LBP_REQUEST_BUFF_LEN  104 /* Max.nsdu=96, max.AddrLen=8. */
struct TCbMsgAdpLbpRequest {
	uint8_t m_u8NsduHandle;
	uint8_t m_u8MaxHops;
	bool m_bDiscoverRoute;
	uint8_t m_u8QualityOfService;
	bool m_bSecurityEnable;
	uint8_t m_u8DstAddrLength; /* 2 or 8 bytes */
	uint16_t m_u16NsduLength;
	uint8_t m_buffer[APD_LBP_REQUEST_BUFF_LEN];
};

/* HiMsgAdpDataConfirm parameters definition */
struct TCbMsgAdpDataConfirm {
	uint8_t m_u8Status;
	uint8_t m_u8NsduHandle;
};

struct TCbMsgMacResetConfirm {
	uint8_t m_u8Status;
};

/* HiMsgAdpNetworkStartConfirm parameters definition */
struct TCbMsgAdpNetworkStartConfirm {
	uint8_t m_u8Status;
};

/* HiMsgAdpSetConfirm parameters definition */
struct TCbMsgAdpSetConfirm {
	uint8_t m_u8Status;
	uint32_t m_u32AttributeId;
	uint16_t m_u16AttributeIndex;
};

/* HiMsgAdpLbpConfirm parameters definition */
struct TCbMsgAdpLbpConfirm {
	uint8_t m_u8Status;
	uint8_t m_u8NsduHandle;
};

typedef struct {
	unsigned char u8EAPIdentifier;
	uint16_t initialShortAddr;
	uint8_t extAddr[8];
	uint8_t pending;
} t_context;

/* Bitmap definition for us_bitmap */
#define LBDS_LIST_LBD_ACTIVE 0x01
/* In the device list (g_lbds_list), the short address is specified by the */
/* index in the list plus an offste given by initialShortAddr. */
/* are allowed. */
typedef struct {
	uint8_t puc_extended_address[ADP_ADDRESS_64BITS];
	uint8_t uc_lbp_hops; //hops inferred by lbp algorithm
} lbds_list_entry_t;

#define LBP_REKEYING_ON    1
#define LBP_REKEYING_OFF   0
#define LBP_REKEYING_PHASE_DISTRIBUTE    0
#define LBP_REKEYING_PHASE_ACTIVATE      1

uint16_t get_lbds_count(void);
bool is_null_address(uint8_t *puc_extended_address);
uint16_t get_lbd_address(uint16_t i);
uint8_t device_is_in_list(uint16_t us_short_address);
void remove_lbds_list_entry(uint16_t us_short_address);
void activate_lbds_list_entry(uint16_t us_short_address);
uint16_t get_new_address(struct TAdpExtendedAddress lbdAddress);
bool add_lbds_list_entry(const uint8_t *puc_extended_address, uint16_t us_short_address, uint8_t uc_lbp_hops);

t_context *_add_context(void);
t_context *_get_context(void);

void registered_cb(void);

void lbp_init_functions(void);
void initialize_bootstrap_message(t_bootstrap_slot *p_bs_slot);
uint8_t add_to_blacklist(uint8_t *puc_address);
uint8_t remove_from_blacklist(uint16_t us_index);
enum lbp_indications ProcessLBPMessage(struct TAdpLbpIndication *pLbpIndication);
void Process_Joining0(struct TAdpExtendedAddress pLBPEUI64Address, t_bootstrap_slot *p_bs_slot );
uint8_t process_accepted_GMK_activation(struct TAdpExtendedAddress au8LBPEUI64Address, t_bootstrap_slot *p_bs_slot);
uint16_t Encode_kick_to_LBD(uint8_t *p_ext_addr, uint16_t u16MessageLength, uint8_t *pMessageBuffer);
uint16_t Encode_decline(uint8_t *p_ext_addr, t_bootstrap_slot *p_bs_slot);
uint16_t get_initial_short_address(void);
bool set_initial_short_address(uint16_t us_short_addr);
bool get_ib_short_address_from_extended(void);
void set_ib_short_address_from_extended(bool value);
uint16_t get_msg_timeout_value(void);
void set_msg_timeout_value(uint16_t us_timeout_value);
uint8_t GetKeyIndex(void);
void SetKeyIndex(uint8_t u8KeyIndex);
const uint8_t *GetGMK(void);
void set_gmk(uint8_t *puc_new_gmk);
const uint8_t *GetRekeyGMK(void);
void set_rekey_gmk(uint8_t *puc_new_gmk);
void set_ids(const uint8_t *puc_ids, uint8_t uc_size);
void set_psk(uint8_t *puc_new_psk);
void lbp_set_rekeying(uint8_t on_off);
uint16_t lbp_get_rekeying(void);
uint8_t get_next_nsdu_handler(void);

void  init_bootstrap_slots(void);
t_bootstrap_slot *get_bootstrap_slot_by_addr(uint8_t *p_eui64);
t_bootstrap_slot *get_bootstrap_slot_by_index(uint8_t uc_index);

void  update_bootstrap_slots(void);
bool timeout_is_past(uint32_t ul_timeout_value);
void log_show_slots_status(void);

uint8_t get_lbp_hops_from_short_address(uint16_t us_short_address);
uint8_t  get_max_hops_from_nodes_under_registering(void);
#ifdef BS_LEVEL_STRATEGY_ENABLED
void set_level_strategy_timeout(uint32_t ul_timeout_value);
#endif

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */

#endif /* BS_FUNCTIONS_H */
