/**
 * \file
 *
 * \brief Base Node Management file.
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
#include <arpa/inet.h>
#include <ctype.h>
#include <pthread.h>

#include "mngLayerHost.h"
#include "prime_api_host.h"
#include "prime_api_defs_host.h"
#include "cl_432_defs.h"
#include "mac_pib.h"
#include "mac_defs.h"

#include "globals.h"
#include "base_node_manager.h"
#include "base_node_mng.h"
#include "base_node_dlmsotcp.h"
#include "base_node_mng_fw_upgrade.h"
#include "prime_bmng_network_events.h"
#include "base_node_network.h"
#include "prime_utils.h"
#include "prime_log.h"
#include "return_codes.h"

/************************************************************
*       Defines                                             *
*************************************************************/
#define PRIME_SYNC_TIMEOUT 3
#define MAX_PLME_REQUEST_RETRIES 3

/* Define this depending on Base USI node buffers*/
#define  MNGP_PRIME_ENHANCED_LIST_MAX_RECORDS 30

#define MNG_PLANE_LOCAL 0
#define MNG_PLANE_BASE 1

T_prime_sync_mgmt g_prime_sync_mgmt;
uint8_t flag_GetListConfirm = false;          // Global Flag to Detect End of GetListRequest answer from Base Node Modem

/*
struct __attribute__((__packed__)) TprimeSyncMgmt{
    bool f_sync_res;                          // Flag to indicate syncronous response
    bool f_sync_req;                          // Flag to indicate syncronous request
    uint32_t ;                // In order to handle correctly syncronous requests
    struct TmacGetConfirm s_macGetConfirm;    // Get Confirm callback struct
    struct TmacSetConfirm s_macSetConfirm;    // Set Confirm callback struct
};
typedef struct TprimeSyncNullEstablishRequest T_prime_sync_null_establish_request;
*/

extern mchp_list prime_network;
extern struct st_configuration g_st_config;

/* Mutex for accessing PRIME Network Information */
pthread_mutex_t prime_usi_cmd_mutex = PTHREAD_MUTEX_INITIALIZER;
/* Mutex for CL Null establish request */
pthread_mutex_t prime_cl_null_establish_request_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * \brief Lock Mutex to access USI at CMD Level
  */
int prime_usi_cmd_mutex_lock()
{
   return pthread_mutex_lock(&prime_usi_cmd_mutex);
}

/**
 * \brief UnLock Mutex to access USI at CMD Level
  */
int prime_usi_cmd_mutex_unlock()
{
   return pthread_mutex_unlock(&prime_usi_cmd_mutex);
}

/*
 * \brief PRIME Converge Layer Null Establish Request Mutex Lock
 * \return 0
 */
int prime_cl_null_establish_request_mutex_lock()
{
    return pthread_mutex_lock(&prime_cl_null_establish_request_mutex);
}

/*
 * \brief PRIME Converge Layer Null Establish Request Mutex Unlock
 * \return 0
 */
int prime_cl_null_establish_request_mutex_unlock()
{
    return pthread_mutex_unlock(&prime_cl_null_establish_request_mutex);
}

/**
 * \brief GET PRIME MAC Mode
 *        PRIME MAC Mode: Disconnected, Terminal, Switch, Base
 */
uint8_t prime_get_mode()
{
/*********************************************
*       Local Envars                         *
**********************************************/
   static uint8_t first_time = 1;
   struct TmacGetConfirm x_pib_confirm;
 /*********************************************
 *       Code                                 *
 **********************************************/
   if (first_time){
       g_st_config.mode = -1;
       g_st_config.state = -1;
       /* Check if Base Node through propietary PIB */
       prime_cl_null_mlme_get_request_sync(PIB_MAC_PLC_STATE,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
       if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
           // 1 Byte should be the result
           // 0: SN Disconnected - 1: SN Detection - 2: SN Registering - 3: SN Operative - 4: Base Node
           if (x_pib_confirm.m_au8AttributeValue[0] == 4){
              g_st_config.mode = PRIME_MODE_BASE;
              /* State not important on Base Node */
           }else{
              /* State of Service Node - Check when Switch Mode */
              g_st_config.state = x_pib_confirm.m_au8AttributeValue[0];
              prime_cl_null_mlme_get_request_sync(PIB_MAC_STATE,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
              if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
                 // 0: Disconnected - 1: Terminal - 2: Switch - 3: Base
                 g_st_config.mode = x_pib_confirm.m_au8AttributeValue[0];
              }
          }
       }
       PRIME_LOG(LOG_INFO,"PRIME Mode: %d\r\n",g_st_config.mode);
   }
   return g_st_config.mode;
}

/**
 * \brief GET PRIME MAC Security Profile
 *        PRIME MAC Security Profile
 */
uint8_t prime_get_sec_profile()
{
/*********************************************
*       Local Envars                         *
**********************************************/
static uint8_t first_time = 1;
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
  if (first_time){
     g_st_config.sec_profile = -1;
     // Set Security Profile on Base NODES - PIB: PIB_MAC_SEC_PROFILE_USED
     prime_cl_null_mlme_get_request_sync(PIB_MAC_SEC_PROFILE_USED,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
     if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
        // Update Glabal Configuration
        g_st_config.sec_profile = x_pib_confirm.m_au8AttributeValue[0];
     }
     PRIME_LOG(LOG_INFO,"PRIME Sec profile: %d\r\n",g_st_config.sec_profile);
  }
  return g_st_config.sec_profile;
}

/**
 * \brief GET PRIME MAC Band Plan
 *        PRIME MAC Band Plan
 */
uint8_t prime_get_band_plan()
{
/*********************************************
*       Local Envars                         *
**********************************************/
static uint8_t first_time = 1;
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
  if (first_time){
     g_st_config.band_plan = 255; /* Default allow all the channels */
     // Get Band Plan value
     prime_cl_null_plme_get_request_sync(PIB_PHY_TXRX_CHANNEL_LIST,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
     if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
        // Update Glabal Configuration
        g_st_config.band_plan = x_pib_confirm.m_au8AttributeValue[0];
     }
     PRIME_LOG(LOG_INFO,"PRIME Band Plan: %d\r\n",g_st_config.band_plan);
  }
  return g_st_config.band_plan;
}

/**
 * \brief GET PRIME PHY TX Channel
 *        PRIME PHY TX Channel
 */
uint8_t prime_get_tx_channel()
{
/*********************************************
*       Local Envars                         *
**********************************************/
static uint8_t first_time = 1;
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
  if (first_time){
     g_st_config.tx_channel = -1;
     prime_cl_null_plme_get_request_sync(PIB_PHY_TX_CHANNEL,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
     if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
        // Update Glabal Configuration
        g_st_config.tx_channel = x_pib_confirm.m_au8AttributeValue[0];
     }
     PRIME_LOG(LOG_INFO,"PRIME TX Channel: %d\r\n",g_st_config.tx_channel);
  }
  return g_st_config.tx_channel;
}


/**
 * \brief Initialize MAC configuration
 */
int32_t prime_init_mac()
{
/*********************************************
*       Local Envars                         *
**********************************************/
  struct TmacSetConfirm p_pib_confirm;
  struct TmacGetConfirm m_pib_confirm;
  uint8_t not_zero = 0;
  uint8_t eui48[6];
  uint8_t i;
/*********************************************
*       Code                                 *
**********************************************/
  // Get EUI48 from Stack
  prime_cl_null_mlme_get_request_sync(PIB_MTP_MAC_EUI_48,PRIME_SYNC_TIMEOUT_GET_REQUEST,&m_pib_confirm);
  if (m_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      for (i=0;i<6;i++){
          if (m_pib_confirm.m_au8AttributeValue[i])
             not_zero++;
          eui48[i] = m_pib_confirm.m_au8AttributeValue[i];
      }
      if (not_zero){
         // MAC exists and different than "000000000000"
         memcpy(&g_st_config.eui48,eui48,6);
         PRIME_LOG(LOG_INFO,"PRIME MAC: 0x%02X%02X%02X%02X%02X%02X\r\n",eui48[0],eui48[1],eui48[2],eui48[3],eui48[4],eui48[5]);
         return SUCCESS;
      }else{
         // Establish Default MAC Address - Stack is in MTP Mode sure
         PRIME_LOG(LOG_INFO,"PRIME set default MAC address\r\n");
         g_st_config.mtp_mode = 1;
         prime_cl_null_mlme_set_request_sync(PIB_MTP_MAC_EUI_48,&g_st_config.eui48,6,PRIME_SYNC_TIMEOUT_SET_REQUEST, &p_pib_confirm);
         if (m_pib_confirm.m_u8Status == MLME_RESULT_DONE){
            // Get out of MTP Mode
            not_zero = 0;
            prime_cl_null_plme_set_request_sync(PIB_MTP_PHY_ENABLE,&not_zero,1,PRIME_SYNC_TIMEOUT_SET_REQUEST, &p_pib_confirm);
            if (p_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
                PRIME_LOG(LOG_INFO,"PRIME set Normal Mode\r\n");
                return SUCCESS;
            }
         }
      }
   }
   return ERROR_PRIME_INIT_MAC;
}

/**
 * \brief Initialize PRIME configuration from PRIME Stack
 */
int32_t prime_base_node_init_params()
{
  prime_get_mode();
  prime_get_sec_profile();
  prime_init_mac();
  return SUCCESS;
}

/**
 * \brief Configure Embedded Sniffer
 */
int32_t prime_embedded_sniffer(uint8_t enable)
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
  prime_cl_null_plme_set_request_sync(PIB_PHY_SNIFFER_ENABLED,&enable,1,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
     return ERROR_SET_EMBEDDED_SNIFFER;
  }
  return SUCCESS;
}

/**
 * \brief CL Null Establish Indication Callback
 * \param us_con_handle: Unique identifier of the connection
 * \param puc_eui48:     Address of the node which initiates the connection establish procedure
 * \param uc_type:       Convergence Layer type of the connection
 * \param puc_data:      Data associated with the connection establishment procedure
 * \param us_data_len:   Length of the data in bytes
 * \param uc_cfbytes:    Flag to indicate whether or not the connection should use the contention or contention-free channel access scheme
 * \param uc_ae:         Flag to indicate that authentication and encryption is requested
 * \return
 */
void _prime_cl_null_establish_ind_cb(uint16_t us_con_handle, uint8_t *puc_eui48, uint8_t uc_type, uint8_t *puc_data, uint16_t us_data_len, uint8_t uc_cfbytes, uint8_t uc_ae)
{
   PRIME_LOG(LOG_DBG,"MAC establish indication received: handle: %d - eui48: %s - type: %d - ae: %d\r\n", us_con_handle, eui48_to_str(puc_eui48,NULL), uc_type, uc_ae);
   return ;
}

/**
 * \brief CL Null Establish Confirm Callback
 * \param us_con_handle: Unique identifier of the connection
 * \param uc_result:     Result of the connection establishment process
 * \param puc_eui48:     Address of the node which initiates the connection establish procedure
 * \param uc_type:       Convergence Layer type of the connection
 * \param puc_data:      Data associated with the connection establishment procedure
 * \param us_data_len:   Length of the data in bytes
 * \param uc_ae:         Flag to indicate that authentication and encryption is requested
 * \return
 */
 void _prime_cl_null_establish_cfm_cb(uint16_t us_con_handle, mac_establish_confirm_result_t uc_result, uint8_t *puc_eui48, uint8_t uc_type, uint8_t *puc_data, uint16_t us_data_len, uint8_t uc_ae)
{
    PRIME_LOG(LOG_DBG,"Confirm for MAC establish request received: handle: %d - result: %d - eui48: %s - type: %d - ae: %d\r\n", us_con_handle, uc_result, eui48_to_str(puc_eui48,NULL), uc_type, uc_ae);
    return ;
}

/**
 * \brief CL Null Release Indication Callback
 * \param us_con_handle: Unique identifier of the connection
 * \param uc_reason:     Cause of the connection release
 * \return
 */
void _prime_cl_null_release_ind_cb(uint16_t us_con_handle, mac_release_indication_reason_t uc_reason)
{
    PRIME_LOG(LOG_DBG,"MAC release indication received: handle: %d - uc_reason: %d\r\n", us_con_handle, uc_reason);
    return ;
}

/**
 * \brief CL Null Release Confirmation Callback
 * \param us_con_handle: Unique identifier of the connection
 * \param uc_result:     Result of the connection release process
 * \return
 */
void _prime_cl_null_release_cfm_cb(uint16_t us_con_handle, mac_release_confirm_result_t uc_result)
{
    PRIME_LOG(LOG_DBG,"MAC release confirm received: handle: %d - result: %d\r\n", us_con_handle, uc_result);
    return ;
}

/**
 * \brief CL Null Join Indication Callback
 * \param us_con_handle: Unique identifier of the connection
 * \param puc_eui48:     Address of the node which wishes to join the multicast group (only valid for base node)
 * \param uc_con_type:   Connection type
 * \param puc_data:      Data associated with the join request procedure
 * \param us_data_len:   Length of the data in bytes
 * \param uc_ae:         Flag to indicate that authentication and encryption is requested
 * \return
 */
void _prime_cl_null_join_ind_cb(uint16_t us_con_handle, uint8_t *puc_eui48, uint8_t uc_con_type, uint8_t *puc_data, uint16_t us_data_len, uint8_t uc_ae)
{
    PRIME_LOG(LOG_DBG,"MAC Join indication received: handle: %d - eui48: %s - type: %d - ae: %d\r\n", us_con_handle, eui48_to_str(puc_eui48,NULL), uc_con_type, uc_ae);
    return ;
}

/**
 * \brief CL Null Join Confirmation Callback
 * \param us_con_handle: Unique identifier of the connection
 * \param uc_result:     Result of the connection join process
 * \return
 */
void _prime_cl_null_join_cfm_cb(uint16_t us_con_handle, mac_join_confirm_result_t uc_result)
{
    PRIME_LOG(LOG_DBG,"MAC Join confirm received: handle: %d - result: %d\r\n", us_con_handle, uc_result);
    return ;
}

/**
 * \brief CL Null Leave Indication Callback
 * \param us_con_handle: Unique identifier of the connection
 * \param puc_eui48:     Address of the node which wishes to join the multicast group (only valid for base node)
 * \return
 */
void _prime_cl_null_leave_ind_cb(uint16_t us_con_handle, uint8_t *puc_eui48)
{
   PRIME_LOG(LOG_DBG,"MAC leave indication received: handle: %d - eui48: %s\r\n",us_con_handle,eui48_to_str(puc_eui48,NULL));
   return ;
}

/**
 * \brief CL Null Leave Confirmation Callback
 * \param us_con_handle: Unique identifier of the connection
 * \param uc_result:     Result of the connection join process
 * \return
 */
void _prime_cl_null_leave_cfm_cb(uint16_t us_con_handle, mac_leave_confirm_result_t uc_result)
{
   PRIME_LOG(LOG_DBG,"MAC leave confirm received: handle: %d - result: %d\r\n", us_con_handle, uc_result);
   return ;
}

/**
 * \brief CL Null Leave Confirmation Callback
 * \param us_con_handle: Unique identifier of the connection
 * \param puc_data:      Pointer to data to be transmitted through this connection
 * \param drt_result     Result of the transmission (MAC_DATA_SUCCESS, MAC_DATA_TIMEOUT, MAC_DATA_ERROR_SENDING)
 * \return
 */
void _prime_cl_null_data_cfm_cb(uint16_t us_con_handle, uint8_t *puc_data, mac_data_result_t drt_result)
{
   PRIME_LOG(LOG_DBG,"MAC Data Confirm received: handle: %d - result: %d\r\n", us_con_handle, drt_result);
   return ;
}

/**
 * \brief CL Null Data Indication Callback
 * \param us_con_handle: Unique identifier of the connection
 * \param puc_data:      Pointer to data to be transmitted through this connection
 * \param us_data_len:   Length of the data in bytes
 * \param ul_time_ref:   Time Reference
 * \return
 */
void _prime_cl_null_data_ind_cb(uint16_t us_con_handle, uint8_t *puc_data, uint16_t us_data_len, uint32_t ul_time_ref)
{
   PRIME_LOG(LOG_DBG,"MAC Data indication received: handle: %d - time_ref : %u\r\n", us_con_handle, ul_time_ref);
   return ;
}

/**
 * \brief PLME Request syncronous with retries
 * \param prime_plme_request Function Pointer that can point to functions with no return type and no parameters
 * \param uc_timeout         Timeout for the request
 * \param pmacGetConfirm     Pointer to TmacGetConfirm structure
 */
void prime_cl_null_plme_request_sync(void (*prime_plme_request)(), uint8_t uc_timeout, struct TmacGetConfirm *pmacGetConfirm)
{
  uint8_t try = 0;

	/* Synchronous Request - Locks access to USI */
  prime_usi_cmd_mutex_lock();

  while (try < MAX_PLME_REQUEST_RETRIES){
  	// Set the sync flags to intercept the callback
  	g_prime_sync_mgmt.f_sync_req = true;
  	g_prime_sync_mgmt.f_sync_res = false;

  	// Send the asynchronous call
  	prime_plme_request();

  	// Wait processing until flag activates, or timeout
  	addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  	if (g_prime_sync_mgmt.f_sync_res){
  		// Confirm received
      pmacGetConfirm->m_u8Status = g_prime_sync_mgmt.s_macGetConfirm.m_u8Status;
      PRIME_LOG(LOG_ERR,"Confirm for request received (try=%d)\r\n",try);
      break;
  	}
  	else{
  		// Timeout - Confirm not received
  		pmacGetConfirm->m_u8Status = ERROR_CONFIRM_NOT_RECEIVED;
  		PRIME_LOG(LOG_ERR,"ERROR: Confirm for request not received (try=%d)\r\n",try);
      try++;
  	}

  	// Sleep for 10ms to next try
    usleep(10000);
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  /* Synchronous Request - Unlocks access to USI */
  prime_usi_cmd_mutex_unlock();

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_plme_request_sync result = %d\r\n",pmacGetConfirm->m_u8Status);
}

/**
 * \brief PLME reset request syncronous
 * \param uc_timeout     Timeout for the request
 * \param pmacGetConfirm pointer to TmacGetConfirm structure
 */
void prime_cl_null_plme_reset_request_sync(uint8_t uc_timeout, struct TmacGetConfirm *pmacGetConfirm)
{

	//PRIME_LOG(LOG_DBG,"prime_cl_null_plme_reset_request_sync\r\n");

	// Set the sync flags to intercept the callback
	g_prime_sync_mgmt.f_sync_req = true;
	g_prime_sync_mgmt.f_sync_res = false;

	// Send the asynchronous call
	prime_cl_null_plme_reset_request();

	// Wait processing until flag activates, or timeout
	addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

	if (g_prime_sync_mgmt.f_sync_res){
		// Confirm received
    pmacGetConfirm->m_u8Status = g_prime_sync_mgmt.s_macGetConfirm.m_u8Status;
	}
	else{
		// Confirm not received
		pmacGetConfirm->m_u8Status = ERROR_CONFIRM_NOT_RECEIVED;
		PRIME_LOG(LOG_ERR,"ERROR: Confirm for reset request not received\r\n");
	}

	g_prime_sync_mgmt.f_sync_req = false;
	g_prime_sync_mgmt.f_sync_res = false;

  //PRIME_UNLOCK_USI_CMD()

	//PRIME_LOG(LOG_DBG,"prime_cl_null_plme_reset_request_sync result = %d\r\n",pmacGetConfirm->m_u8Status);
}

/********************************************************
 * \brief Callback after receiving a PLME Reset Confirm
 * \param x_result : Request result
 *
 * \return -
 *
 ********************************************************/
void _prime_cl_null_plme_reset_cfm_cb(plme_result_t x_result)
{
  //PRIME_LOG(LOG_DEBUG,"_prime_cl_null_plme_reset_cfm_cb result = 0x%04X\r\n", x_result);
  memset(&g_prime_sync_mgmt.s_macGetConfirm,0,sizeof(struct TmacGetConfirm));
  if (g_prime_sync_mgmt.f_sync_req){
      g_prime_sync_mgmt.s_macGetConfirm.m_u8Status = x_result;
		  g_prime_sync_mgmt.f_sync_res = true;
  }
  return ;
}

/**
 * \brief PLME sleep request syncronous
 * \param uc_timeout     Timeout for the request
 * \param pmacGetConfirm pointer to TmacGetConfirm structure
 */
void prime_cl_null_plme_sleep_request_sync(uint8_t uc_timeout, struct TmacGetConfirm *pmacGetConfirm)
{
  //uint8_t result = -1;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_plme_sleep_request_sync\r\n");

	// Set the sync flags to intercept the callback
	g_prime_sync_mgmt.f_sync_req = true;
	g_prime_sync_mgmt.f_sync_res = false;

	// Send the asynchronous call
	prime_cl_null_plme_sleep_request();

	// Wait processing until flag activates, or timeout
	addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

	if (g_prime_sync_mgmt.f_sync_res){
		// Confirm received
    pmacGetConfirm->m_u8Status = g_prime_sync_mgmt.s_macGetConfirm.m_u8Status;
		//result = 0;
	}
	else{
		// Confirm not received
		pmacGetConfirm->m_u8Status = ERROR_CONFIRM_NOT_RECEIVED;
		PRIME_LOG(LOG_ERR,"ERROR: Confirm for sleep request not received\r\n");
	}

	g_prime_sync_mgmt.f_sync_req = false;
	g_prime_sync_mgmt.f_sync_res = false;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_plme_sleep_request_sync result = %d\r\n",result);
}

/********************************************************/
/**
 * \brief Callback after receiving a PLME Sleep Confirm
 * \param x_result : Request result
 *
 * \return -
 *
 ********************************************************/
void _prime_cl_null_plme_sleep_cfm_cb(plme_result_t x_result)
{
  //PRIME_LOG(LOG_DEBUG,"_prime_cl_null_plme_sleep_cfm_cb result = 0x%04X\r\n", x_result);
  memset(&g_prime_sync_mgmt.s_macGetConfirm,0,sizeof(struct TmacGetConfirm));
  if (g_prime_sync_mgmt.f_sync_req){
      g_prime_sync_mgmt.s_macGetConfirm.m_u8Status = x_result;
		  g_prime_sync_mgmt.f_sync_res = true;
  }
  return ;
}

/**
 * \brief PLME resume request syncronous
 * \param uc_timeout     Timeout for the request
 * \param pmacGetConfirm pointer to TmacGetConfirm structure
 */
void prime_cl_null_plme_resume_request_sync(uint8_t uc_timeout, struct TmacGetConfirm *pmacGetConfirm)
{
  //uint8_t result = -1;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_plme_resume_request_sync\r\n");

	// Set the sync flags to intercept the callback
	g_prime_sync_mgmt.f_sync_req = true;
	g_prime_sync_mgmt.f_sync_res = false;

	// Send the asynchronous call
	prime_cl_null_plme_resume_request();

	// Wait processing until flag activates, or timeout
	addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

	if (g_prime_sync_mgmt.f_sync_res){
		// Confirm received
    pmacGetConfirm->m_u8Status = g_prime_sync_mgmt.s_macGetConfirm.m_u8Status;
		//result = 0;
	}
	else{
		// Confirm not received
		pmacGetConfirm->m_u8Status = ERROR_CONFIRM_NOT_RECEIVED;
		PRIME_LOG(LOG_ERR,"ERROR: Confirm for resume request not received\r\n");
	}

	g_prime_sync_mgmt.f_sync_req = false;
	g_prime_sync_mgmt.f_sync_res = false;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_plme_resume_request_sync result = %d\r\n",result);
}

/********************************************************
 * \brief Callback after receiving a PLME Resume Confirm
 * \param x_result : Request result
 *
 * \return -
 ********************************************************/
void _prime_cl_null_plme_resume_cfm_cb(plme_result_t x_result)
{
  //PRIME_LOG(LOG_DEBUG,"_prime_cl_null_plme_resume_cfm_cb result = 0x%04X\r\n", x_result);
  memset(&g_prime_sync_mgmt.s_macGetConfirm,0,sizeof(struct TmacGetConfirm));
  if (g_prime_sync_mgmt.f_sync_req){
      g_prime_sync_mgmt.s_macGetConfirm.m_u8Status = x_result;
		  g_prime_sync_mgmt.f_sync_res = true;
  }
  return ;
}

/********************************************************
 * \brief Delete a Device from the List
 * \param p_prime_sn_device : pointer to PRIME SN Device
 *
 * \return -
 ********************************************************/
void _prime_cl_null_plme_testmode_cfm_cb(plme_result_t x_result)
{
  return ;
}

/**
 * \brief Callback after receiving a PLME Get Confirm
 * \param x_status       : Get Request status
 * \param us_pib_attrib  : PIB attribute
 * \param pv_pib_value   : PIB Value pointer
 * \param uc_pib_size    : PIB Value pointer size
 * \return -
 */
void _prime_cl_null_plme_get_cfm_cb(plme_result_t x_status, uint16_t us_pib_attrib, void *pv_pib_value, uint8_t uc_pib_size)
{
  memset(&g_prime_sync_mgmt.s_macGetConfirm,0,sizeof(struct TmacGetConfirm));
  if (g_prime_sync_mgmt.f_sync_req && (g_prime_sync_mgmt.m_u16AttributeId == us_pib_attrib)){
      g_prime_sync_mgmt.s_macGetConfirm.m_u8Status = x_status;
      g_prime_sync_mgmt.s_macGetConfirm.m_u16AttributeId = us_pib_attrib;
      g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength = uc_pib_size;
      memcpy(&g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue,pv_pib_value,uc_pib_size);
		  g_prime_sync_mgmt.f_sync_res = true;
  }
  return ;
}

/**
 * \brief PLME get request syncronous
 * \param us_pib_attrib  PIB attribute
 * \param uc_timeout     Timeout for the request
 * \param pmacGetConfirm pointer to TmacGetConfirm structure
 */
void prime_cl_null_plme_get_request_sync(uint16_t us_pib_attrib, uint8_t uc_timeout, struct TmacGetConfirm *pmacGetConfirm)
{
  //uint8_t result = -1;
  uint16_t us_value;
  uint32_t ui_value;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_plme_get_request_sync attr = 0x%04X\r\n", us_pib_attrib);

	// Set the sync flags to intercept the callback
	g_prime_sync_mgmt.f_sync_req = true;
	g_prime_sync_mgmt.f_sync_res = false;
	g_prime_sync_mgmt.m_u16AttributeId = us_pib_attrib; /* Used in order to know AttributeId requested on GetConfirm callback */

	// Send the asynchronous call
	prime_cl_null_plme_get_request(us_pib_attrib);

	// Wait processing until flag activates, or timeout
	addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

	if (g_prime_sync_mgmt.f_sync_res){
		// Confirm received
    pmacGetConfirm->m_u8Status = g_prime_sync_mgmt.s_macGetConfirm.m_u8Status;
  	pmacGetConfirm->m_u16AttributeId = g_prime_sync_mgmt.s_macGetConfirm.m_u16AttributeId;
  	pmacGetConfirm->m_u8AttributeLength = g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength;
    // Take care with endianess - USI_HOST transfers on Big Endian
    if (pmacGetConfirm->m_u8AttributeLength == 4){
       memcpy(&ui_value,&g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue,4);
       ui_value = ntohl(ui_value);
       memcpy(&pmacGetConfirm->m_au8AttributeValue,&ui_value,4);
    }else if (pmacGetConfirm->m_u8AttributeLength == 2){
       memcpy(&us_value,&g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue,2);
       us_value = ntohs(us_value);
       memcpy(&pmacGetConfirm->m_au8AttributeValue,&us_value,2);
    }else{
       memcpy(&pmacGetConfirm->m_au8AttributeValue, &g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue,g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength);
    }
		//result = 0;
	}
	else{
		// Confirm not received
		pmacGetConfirm->m_u8Status = -1;
		PRIME_LOG(LOG_ERR,"ERROR: Confirm for 0x%04X not received\r\n",us_pib_attrib);
	}

	g_prime_sync_mgmt.f_sync_req = false;
	g_prime_sync_mgmt.f_sync_res = false;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_plme_get_request_sync result = %d\r\n",result);
}

/**
 * \brief PLME set request syncronous
 * \param us_pib_attrib  PIB attribute
 * \param pv_pib_value   PIB Value pointer
 * \param uc_pib_size    PIB Value pointer size
 * \param uc_timeout     Timeout for the request
 * \param pmacSetConfirm Set Confirm pointer
 * \result -
 */
void prime_cl_null_plme_set_request_sync(uint16_t us_pib_attrib, void *pv_pib_value, uint8_t uc_pib_size, uint8_t uc_timeout, struct TmacSetConfirm *pmacSetConfirm)
{

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_plme_set_request_sync attr = 0x%04X\r\n", us_pib_attrib);

	// Set the sync flags to intercept the callback
	g_prime_sync_mgmt.f_sync_req = true;
	g_prime_sync_mgmt.f_sync_res = false;
	g_prime_sync_mgmt.m_u16AttributeId = us_pib_attrib; /* Used in order to know AttributeId requested on GetConfirm callback */

	// Send the asynchronous call
	prime_cl_null_plme_set_request(us_pib_attrib, pv_pib_value, uc_pib_size);

	// Wait processing until flag activates, or timeout
	addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

	if (g_prime_sync_mgmt.f_sync_res){
		// Confirm received
    pmacSetConfirm->m_u8Status = g_prime_sync_mgmt.s_macSetConfirm.m_u8Status;
  	pmacSetConfirm->m_u16AttributeId = g_prime_sync_mgmt.s_macSetConfirm.m_u16AttributeId;
	}
	else{
		// Confirm not received
		pmacSetConfirm->m_u8Status = -1;
		PRIME_LOG(LOG_ERR,"ERROR: Confirm for 0x%04X not received\r\n",us_pib_attrib);
	}

	g_prime_sync_mgmt.f_sync_req = false;
	g_prime_sync_mgmt.f_sync_res = false;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_plme_set_request_sync result = %d\r\n",pmacSetConfirm->m_u8Status);
}

/**
 * \brief Callback after receiving a PLME Set Confirm
 * \param x_result       : Set Confirm result
 * \return -
 */
void _prime_cl_null_plme_set_cfm_cb(plme_result_t x_result)
{
  //PRIME_LOG(LOG_DEBUG,"_prime_cl_null_plme_set_cfm_cb result = 0x%04X\r\n", x_result);
  memset(&g_prime_sync_mgmt.s_macSetConfirm,0,sizeof(struct TmacSetConfirm));
//  if (g_prime_sync_mgmt.f_sync_req && (g_prime_sync_mgmt.m_u16AttributeId == us_pib_attrib)){
      g_prime_sync_mgmt.s_macSetConfirm.m_u8Status = x_result;
//      g_prime_sync_mgmt.s_macSetConfirm.m_u16AttributeId = us_pib_attrib;
		  g_prime_sync_mgmt.f_sync_res = true;
//  }
  return ;
}

/**
 * \brief MLME Promote Request Syncronous
 * \param puc_eui48         EUI48 of Service Node to promote
 * \param uc_bcn_mode       Beacon Mode
 * \param uc_timeout        Timeout for request
 * \param pmacGetConfirm    pointer to TmacGetConfirm structure
 */
void prime_cl_null_mlme_promote_request_sync(uint8_t * puc_eui48, uint8_t uc_bcn_mode, uint8_t uc_timeout, struct TmacGetConfirm *pmacGetConfirm)
{
  //uint8_t result = -1;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_mlme_promote_request_sync\r\n");

	// Set the sync flags to intercept the callback
	g_prime_sync_mgmt.f_sync_req = true;
	g_prime_sync_mgmt.f_sync_res = false;

	// Send the asynchronous call
	prime_cl_null_mlme_promote_request(puc_eui48, uc_bcn_mode);

	// Wait processing until flag activates, or timeout
	addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

	if (g_prime_sync_mgmt.f_sync_res){
		// Confirm received
    pmacGetConfirm->m_u8Status = g_prime_sync_mgmt.s_macGetConfirm.m_u8Status;
		//result = 0;
	}
	else{
		// Confirm not received
		pmacGetConfirm->m_u8Status = -1;
		PRIME_LOG(LOG_ERR,"ERROR: Confirm for promotion request not received\r\n");
	}

	g_prime_sync_mgmt.f_sync_req = false;
	g_prime_sync_mgmt.f_sync_res = false;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_mlme_promote_request_sync result = %d\r\n",result);
}

/**
 * \brief MLME Promotion Confirm Callback
 * \param x_result : MLME Result of Promote operation
 *
 */
void _prime_cl_null_mlme_promote_cfm_cb(mlme_result_t x_result)
{
  //PRIME_LOG(LOG_DEBUG,"_prime_cl_null_mlme_promote_cfm_cb result = 0x%04X\r\n", x_result);
  memset(&g_prime_sync_mgmt.s_macGetConfirm,0,sizeof(struct TmacGetConfirm));
  if (g_prime_sync_mgmt.f_sync_req){
      g_prime_sync_mgmt.s_macGetConfirm.m_u8Status = x_result;
      g_prime_sync_mgmt.f_sync_res = true;
  }
  return ;
}

/**
 * \brief MLME Demote Request Syncronous
 * \param uc_timeout        Timeout for request
 * \param pmacGetConfirm    pointer to TmacGetConfirm structure
 */
void prime_cl_null_mlme_demote_request_sync(uint8_t uc_timeout, struct TmacGetConfirm *pmacGetConfirm)
{
  //uint8_t result = -1;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_mlme_demote_request_sync\r\n");

	// Set the sync flags to intercept the callback
	g_prime_sync_mgmt.f_sync_req = true;
	g_prime_sync_mgmt.f_sync_res = false;

	// Send the asynchronous call
	prime_cl_null_mlme_demote_request();

	// Wait processing until flag activates, or timeout
	addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

	if (g_prime_sync_mgmt.f_sync_res){
		// Confirm received
    pmacGetConfirm->m_u8Status = g_prime_sync_mgmt.s_macGetConfirm.m_u8Status;
		//result = 0;
	}
	else{
		// Confirm not received
		pmacGetConfirm->m_u8Status = -1;
		PRIME_LOG(LOG_ERR,"ERROR: Confirm for demotion request not received\r\n");
	}

	g_prime_sync_mgmt.f_sync_req = false;
	g_prime_sync_mgmt.f_sync_res = false;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_mlme_demote_request_sync result = %d\r\n",result);
}

/**
 * \brief MLME Demotion Confirm Callback
 * \param x_result : MLME Result of Demotion operation
 *
 */
void _prime_cl_null_mlme_demote_cfm_cb(mlme_result_t x_result)
{
  //PRIME_LOG(LOG_DEBUG,"_prime_cl_null_mlme_demote_cfm_cb result = 0x%04X\r\n", x_result);
  memset(&g_prime_sync_mgmt.s_macGetConfirm,0,sizeof(struct TmacGetConfirm));
  if (g_prime_sync_mgmt.f_sync_req){
      g_prime_sync_mgmt.s_macGetConfirm.m_u8Status = x_result;
      g_prime_sync_mgmt.f_sync_res = true;
  }
  return ;
}

/*
 * \brief MLME Unregister Request Syncronous
 * \param pmacGetConfirm : Get Confirm structure pointer
 * \result -
 */
void prime_cl_null_mlme_unregister_request_sync(struct TmacGetConfirm *pmacGetConfirm)
{
  //uint8_t result = -1;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_mlme_unregister_request_sync\r\n");

	// Set the sync flags to intercept the callback
	g_prime_sync_mgmt.f_sync_req = true;
	g_prime_sync_mgmt.f_sync_res = false;

	// Send the asynchronous call
	prime_cl_null_mlme_unregister_request();

	// Wait processing until flag activates, or timeout
	addUsi_WaitProcessing(PRIME_SYNC_TIMEOUT_DEFAULT, (Bool *)&g_prime_sync_mgmt.f_sync_res);

	if (g_prime_sync_mgmt.f_sync_res){
		// Confirm received
    pmacGetConfirm->m_u8Status = g_prime_sync_mgmt.s_macGetConfirm.m_u8Status;
		//result = 0;
	}
	else{
		// Confirm not received
		pmacGetConfirm->m_u8Status = -1;
		PRIME_LOG(LOG_ERR,"ERROR: Confirm for unregister request not received\r\n");
	}

	g_prime_sync_mgmt.f_sync_req = false;
	g_prime_sync_mgmt.f_sync_res = false;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_mlme_unregister_request_sync result = %d\r\n",result);
}

/**
 * \brief MLME Unregister Confirm Callback
 * \param x_result : MLME Result of Unregister operation
 *
 */
void _prime_cl_null_mlme_unregister_cfm_cb(mlme_result_t x_result)
{
  //PRIME_LOG(LOG_DEBUG,"_prime_cl_null_mlme_unregister_cfm_cb result = 0x%04X\r\n", x_result);
  memset(&g_prime_sync_mgmt.s_macGetConfirm,0,sizeof(struct TmacGetConfirm));
  if (g_prime_sync_mgmt.f_sync_req){
      g_prime_sync_mgmt.s_macGetConfirm.m_u8Status = x_result;
      g_prime_sync_mgmt.f_sync_res = true;
  }
  return ;
}

/**
 * \brief MLME Reset Request Syncronous
 * \param uc_timeout        Timeout for the request
 * \param pmacGetConfirm    pointer to TmacGetConfirm structure
 */
void prime_cl_null_mlme_reset_request_sync(uint8_t uc_timeout, struct TmacGetConfirm *pmacGetConfirm)
{
  //uint8_t result = -1;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_mlme_reset_request_sync\r\n");

	// Set the sync flags to intercept the callback
	g_prime_sync_mgmt.f_sync_req = true;
	g_prime_sync_mgmt.f_sync_res = false;

	// Send the asynchronous call
	prime_cl_null_mlme_reset_request();

	// Wait processing until flag activates, or timeout
	addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

	if (g_prime_sync_mgmt.f_sync_res){
		// Confirm received
    pmacGetConfirm->m_u8Status = g_prime_sync_mgmt.s_macGetConfirm.m_u8Status;
		//result = 0;
	}
	else{
		// Confirm not received
		pmacGetConfirm->m_u8Status = -1;
		PRIME_LOG(LOG_ERR,"ERROR: Confirm for reset request not received\r\n");
	}

	g_prime_sync_mgmt.f_sync_req = false;
	g_prime_sync_mgmt.f_sync_res = false;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_mlme_reset_request_sync result = %d\r\n",result);
}

/**
 * \brief MLME Reset Confirm Callback
 * \param x_result   MLME reset confirm callback
 */
void _prime_cl_null_mlme_reset_cfm_cb(mlme_result_t x_result)
{
  //PRIME_LOG(LOG_DEBUG,"_prime_cl_null_mlme_reset_cfm_cb result=%d\r\n", x_result);
  memset(&g_prime_sync_mgmt.s_macGetConfirm,0,sizeof(struct TmacGetConfirm));
  if (g_prime_sync_mgmt.f_sync_req){
      g_prime_sync_mgmt.s_macGetConfirm.m_u8Status = x_result;
		  g_prime_sync_mgmt.f_sync_res = true;
  }
  return ;
}

/**
 * \brief MLME Get Confirm Callback
 * \param x_status       : Get Request status
 * \param us_pib_attrib  : PIB attribute
 * \param pv_pib_value   : PIB Value pointer
 * \param uc_pib_size    : PIB Value pointer size
 * \return -
 */
void _prime_cl_null_mlme_get_cfm_cb(mlme_result_t x_status, uint16_t us_pib_attrib, void *pv_pib_value, uint8_t uc_pib_size)
{
  //PRIME_LOG(LOG_DEBUG,"_prime_cl_null_mlme_get_cfm_cb status = 0x%02X attr = 0x%04X\r\n", x_status, us_pib_attrib);
  memset(&g_prime_sync_mgmt.s_macGetConfirm,0,sizeof(struct TmacGetConfirm));
  if (g_prime_sync_mgmt.f_sync_req && (g_prime_sync_mgmt.m_u16AttributeId == us_pib_attrib)){
      g_prime_sync_mgmt.s_macGetConfirm.m_u8Status = x_status;
      g_prime_sync_mgmt.s_macGetConfirm.m_u16AttributeId = us_pib_attrib;
      g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength = uc_pib_size;
      memcpy(&g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue,pv_pib_value,uc_pib_size);
		  g_prime_sync_mgmt.f_sync_res = true;
  }
  return ;
}

/**
 * \brief MLME get request syncronous
 * \param us_pib_attrib     PIB attribute
 * \param uc_timeout        Timeout for request
 * \param pmacGetConfirm    pointer to TmacGetConfirm structure
 */
void prime_cl_null_mlme_get_request_sync(uint16_t us_pib_attrib, uint8_t uc_timeout, struct TmacGetConfirm *pmacGetConfirm)
{
  //uint8_t  result = -1;
  uint16_t us_value;
  uint32_t ui_value;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_mlme_get_request_sync attr = 0x%04X\r\n", us_pib_attrib);

	// Set the sync flags to intercept the callback
	g_prime_sync_mgmt.f_sync_req = true;
	g_prime_sync_mgmt.f_sync_res = false;
	g_prime_sync_mgmt.m_u16AttributeId = us_pib_attrib; /* Used in order to know AttributeId requested on GetConfirm callback */

	// Send the asynchronous call
	prime_cl_null_mlme_get_request(us_pib_attrib);

	// Wait processing until flag activates, or timeout
	addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

	if (g_prime_sync_mgmt.f_sync_res){
		// Confirm received
    pmacGetConfirm->m_u8Status = g_prime_sync_mgmt.s_macGetConfirm.m_u8Status;
  	pmacGetConfirm->m_u16AttributeId = g_prime_sync_mgmt.s_macGetConfirm.m_u16AttributeId;
  	pmacGetConfirm->m_u8AttributeLength = g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength;
    // Take care with endianess - USI_HOST transfers on Big Endian
    if (pmacGetConfirm->m_u8AttributeLength == 4){
       memcpy(&ui_value,&g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue,4);
       ui_value = ntohl(ui_value);
       memcpy(&pmacGetConfirm->m_au8AttributeValue,&ui_value,4);
    }else if (pmacGetConfirm->m_u8AttributeLength == 2){
       memcpy(&us_value,&g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue,2);
       us_value = ntohs(us_value);
       memcpy(&pmacGetConfirm->m_au8AttributeValue,&us_value,2);
    }else{
  	   memcpy(&pmacGetConfirm->m_au8AttributeValue, &g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue,g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength);
    }
		//result = 0;
	}
	else{
		// Confirm not received
		pmacGetConfirm->m_u8Status = -1;
		PRIME_LOG(LOG_ERR,"ERROR: Confirm for 0x%04X not received\r\n",us_pib_attrib);
	}

	g_prime_sync_mgmt.f_sync_req = false;
	g_prime_sync_mgmt.f_sync_res = false;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_mlme_get_request_sync result = %d\r\n",result);
}

/**
 * \brief MLME get list request syncronous
 * \param us_pib_attrib        PIB attribute
 * \param uc_timeout           Timeout for request
 * \param pMlmeGetListConfirm  pointer to TmacGetConfirm structure
 */
void prime_cl_null_mlme_list_get_request_sync(uint16_t us_pib_attrib, uint8_t uc_timeout, struct TmacGetConfirm *pmacGetConfirm)
{
  uint8_t result = -1;
  uint8_t seconds = 240;        // Maximum 240 seconds
  static time_t start_t,end_t;  // Get List receives more than 1 prime_cl_null_mlme_list_get_cb
  double diff_t = 0;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_mlme_list_get_request_sync attr = 0x%04X\r\n", us_pib_attrib);

	// Set the sync flags to intercept the callback
	g_prime_sync_mgmt.f_sync_req = true;
	g_prime_sync_mgmt.f_sync_res = false;
	g_prime_sync_mgmt.m_u16AttributeId = us_pib_attrib; /* Used in order to know AttributeId requested on GetConfirm callback */
  flag_GetListConfirm = 0;

  // Init Time
  time(&start_t);
	// Send the asynchronous call - Response finish when received _prime_cl_null_mlme_list_get_cfm_cb with length == 0
  // At least 2 _prime_cl_null_mlme_list_get_cfm_cb() are received for each request
	prime_cl_null_mlme_list_get_request(us_pib_attrib);

	// Wait for the defined time, or until the referenced flag activates
	while ((diff_t < seconds) && !flag_GetListConfirm){
        // Wait processing until flag activates, or timeout
  	    addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);
        // Check Response Flag
        if (g_prime_sync_mgmt.f_sync_res == TRUE){
      		// Receiving Data from GetList
          g_prime_sync_mgmt.f_sync_res = FALSE;
          result = MLME_RESULT_DONE;
          PRIME_LOG(LOG_DEBUG,"Callback Confirm for 0x%04X atribute list received\r\n",us_pib_attrib);
      	}
      	else{
      		// Not Receiving Data from GetList... How to cancel???
      		//pMlmeGetListConfirm->m_u8Status = -1;
      		PRIME_LOG(LOG_ERR,"ERROR: Confirm for 0x%04X atribute list not received\r\n",us_pib_attrib);
          break;
      	}

        // Final Time
    		time(&end_t);
    		//seconds_elapsed = ((end-start)/CLOCKS_PER_SEC);
    		diff_t = difftime(end_t, start_t);
	}

  if (flag_GetListConfirm == false){
     // Error getting list
     PRIME_LOG(LOG_ERR,"ERROR getting 0x%04X attribute list\r\n",us_pib_attrib);
     result = -1;
  }
  pmacGetConfirm->m_u8Status = result;

	g_prime_sync_mgmt.f_sync_req = false;
	g_prime_sync_mgmt.f_sync_res = false;

	//PRIME_LOG(LOG_DEBUG,"prime_cl_null_mlme_get_request_sync result = %d\r\n",result);
}

/**
 * \brief Print
 * \param entry        Print Register Device Entry
 */
void print_macListRegDevice(macListRegDevices * entry)
{
  PRIME_LOG(LOG_INFO,"0x%02X%02X%02X%02X%02X%02X %05d %03d %03d %03d %03d %03d 0x%04X\r\n",
                                                                    entry->regEntryID[0],
                                                                    entry->regEntryID[1],
                                                                    entry->regEntryID[2],
                                                                    entry->regEntryID[3],
                                                                    entry->regEntryID[4],
                                                                    entry->regEntryID[5],
                                                                    entry->regEntryLNID,
                                                                    entry->regEntryState,
                                                                    entry->regEntryLSID,
                                                                    entry->regEntrySID,
                                                                    entry->regEntryLevel,
                                                                    entry->regEntryTCap,
                                                                    entry->regEntrySwCap);
  return ;
}

/**
 * \brief Print
 * \param entry        Print Active Connection Entry
 */
void print_macListActiveConn(macListActiveConn * entry)
{
  PRIME_LOG(LOG_INFO,"0x%02X%02X%02X%02X%02X%02X %05d %03d %05d %03d\r\n",
                                                                    entry->connEntryID[0],
                                                                    entry->connEntryID[1],
                                                                    entry->connEntryID[2],
                                                                    entry->connEntryID[3],
                                                                    entry->connEntryID[4],
                                                                    entry->connEntryID[5],
                                                                    entry->connEntryLNID,
                                                                    entry->connEntrySID,
                                                                    entry->connEntryLCID,
                                                                    entry->connType);
  return ;
}

/**
 * \brief Print
 * \param entry        Print Multicast Entry
 */
void print_macListMcastEntries(macListMcastEntries * entry)
{
  PRIME_LOG(LOG_INFO,"%05d %05d\r\n", entry->mcastEntryLCID, entry->mcastEntryMembers);
  return ;
}

/**
 * \brief Print
 * \param entry        Print Switch Table Entry
 */
void print_macListSwitchTable(macListSwitchTable * entry)
{
  PRIME_LOG(LOG_INFO,"%05d %03d %03d %03d\r\n",
                                 entry->stblEntryLNID,
                                 entry->stblEntryLSID,
                                 entry->stbleEntrySID,
                                 entry->stblEntryALVTime);
  return ;
}

/**
 * \brief Print
 * \param entry        Print Direct Connection Entry
 */
void print_macListDirectConn(macListDirectConn * entry)
{
  PRIME_LOG(LOG_INFO,"%03d %05d %05d 0x%02X%02X%02X%02X%02X%02X %03d %05d %05d 0x%02X%02X%02X%02X%02X%02X %03d 0x%02X%02X%02X%02X%02X%02X\r\n",
                                                                    entry->dconnEntrySrcSID,
                                                                    entry->dconEntrySrcLNID,
                                                                    entry->dconnEntrySrcLCID,
                                                                    entry->dconnEntrySrcID[0],
                                                                    entry->dconnEntrySrcID[1],
                                                                    entry->dconnEntrySrcID[2],
                                                                    entry->dconnEntrySrcID[3],
                                                                    entry->dconnEntrySrcID[4],
                                                                    entry->dconnEntrySrcID[5],
                                                                    entry->dconnEntryDstSID,
                                                                    entry->dconnEntryDstLNID,
                                                                    entry->dconnEntryDstLCID,
                                                                    entry->dconnEntryDstID[0],
                                                                    entry->dconnEntryDstID[1],
                                                                    entry->dconnEntryDstID[2],
                                                                    entry->dconnEntryDstID[3],
                                                                    entry->dconnEntryDstID[4],
                                                                    entry->dconnEntryDstID[5],
                                                                    entry->dconnEntryDSID,
                                                                    entry->dconnEntryDID[0],
                                                                    entry->dconnEntryDID[1],
                                                                    entry->dconnEntryDID[2],
                                                                    entry->dconnEntryDID[3],
                                                                    entry->dconnEntryDID[4],
                                                                    entry->dconnEntryDID[5]);
  return ;
}

/**
 * \brief Print
 * \param entry        Print Direct Table Entry
 */
void print_macListDirectTable(macListDirectTable * entry)
{
  PRIME_LOG(LOG_INFO,"%03d %05d %05d %03d %05d %05d 0x%02X%02X%02X%02X%02X%02X\r\n",
                                                                    entry->dconnEntrySrcSID,
                                                                    entry->dconEntrySrcLNID,
                                                                    entry->dconnEntrySrcLCID,
                                                                    entry->dconnEntryDstSID,
                                                                    entry->dconnEntryDstLNID,
                                                                    entry->dconnEntryDstLCID,
                                                                    entry->dconnEntryDID[0],
                                                                    entry->dconnEntryDID[1],
                                                                    entry->dconnEntryDID[2],
                                                                    entry->dconnEntryDID[3],
                                                                    entry->dconnEntryDID[4],
                                                                    entry->dconnEntryDID[5]);
  return ;
}

/**
 * \brief Print
 * \param entry        Print Available Switches Entry
 */
void print_macListAvailableSwitches(macListAvailableSwitches * entry)
{
  PRIME_LOG(LOG_INFO,"0x%02X%02X%02X%02X%02X%02X %03d %03d %03d %03d\r\n",
                                                                    entry->slistEntrySNA[0],
                                                                    entry->slistEntrySNA[1],
                                                                    entry->slistEntrySNA[2],
                                                                    entry->slistEntrySNA[3],
                                                                    entry->slistEntrySNA[4],
                                                                    entry->slistEntrySNA[5],
                                                                    entry->slistEntryLSID,
                                                                    entry->slistEntryLevel,
                                                                    entry->slistEntryRxLvl,
                                                                    entry->slistEntryRxSNR);
  return ;
}

/**
 * \brief Print
 * \param entry        Print Physical Communication Entry
 */
void print_macListPhyComm(macListPhyComm * entry)
{
  PRIME_LOG(LOG_INFO,"%05d %03d %03d %03d %03d %03d %03d %05d\r\n",
                                                                    entry->phyCommLNID,
                                                                    entry->phyCommSID,
                                                                    entry->phyCommTxPwr,
                                                                    entry->phyCommRxLvl,
                                                                    entry->phyCommSNR,
                                                                    entry->phyCommTxModulation,
                                                                    entry->phyCommPhyTypeCapability,
                                                                    entry->phyCommRxAge);
  return ;
}

/**
 * \brief Print
 * \param entry        Print Firmware Upgrade List Entry
 */
void print_macListFU(macListFU * entry)
{
  PRIME_LOG(LOG_INFO,"0x%s %s 0x%04X %d\r\n", eui48_to_str(entry->fuMAC, NULL),
                                              fup_state_to_str(entry->fuNodeState,NULL),
                                                                    entry->fuLNID,
                                                                    entry->fuPagesCompleted);
  return ;
}

/**
 * \brief Print
 * \param entry        Print CL 4-32 Connection Entry
 */
void print_cl432ListNode(cl432ListNode * entry)
{
  int i;

  PRIME_LOG(LOG_INFO,"CL432_NodeEntry-> EUI48=0x%s, CL432_ADDR=0x%04X, CL432_SERIAL=",eui48_to_str(entry->cl432mac, NULL),entry->cl432address);
  for (i=0;i<entry->cl432serial_len;i++){
       PRIME_LOG_NOSTAMP(LOG_INFO,"%c",entry->cl432serial[i]);
  }
  PRIME_LOG_NOSTAMP(LOG_INFO,"\r\n");
}

/**
 * \brief Callback after receiving a MLME Get Confirm
 * \param x_status       : Get Request status
 * \param us_pib_attrib  : PIB attribute
 * \param puc_pib_buff   : PIB Value pointer
 * \param uc_pib_len     : PIB Value pointer length
 * \return -
 */
 void _prime_cl_null_mlme_list_get_cfm_cb(mlme_result_t x_status, uint16_t us_pib_attrib, uint8_t *puc_pib_buff, uint16_t us_pib_len)
{
  uint16_t us_entries, index;
  macListRegDevices reg_device_entry;
  macListActiveConn active_conn_entry;
  mac_conn *active_conn_entry_tmp;
  macListMcastEntries mcast_entry;
  macListSwitchTable switch_table_entry;
  macListDirectConn direct_conn_entry;
  macListDirectTable direct_table_entry;
  macListAvailableSwitches available_switches_entry;
  macListPhyComm phy_comm_entry;
  cl432ListNode cl432_node_entry;
  prime_sn *sn;

  PRIME_LOG(LOG_DEBUG,"_prime_cl_null_mlme_list_get_cfm_cb result = %d\r\n",x_status);
  if (g_prime_sync_mgmt.f_sync_req && (g_prime_sync_mgmt.m_u16AttributeId == us_pib_attrib)){
      g_prime_sync_mgmt.s_macGetConfirm.m_u8Status = x_status;
      //g_prime_sync_mgmt.s_macGetConfirm.m_u16AttributeId = us_pib_attrib;
      //g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength = uc_pib_size;
      //memcpy(&g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue,pv_pib_value,uc_pib_size);
      g_prime_sync_mgmt.f_sync_res = true;
  }
  if (x_status == MLME_RESULT_DONE){
    if ((!flag_GetListConfirm) && (us_pib_len == 0)){
       // Received Last Message after a mlme_list_get_request
       PRIME_LOG(LOG_DEBUG,"_prime_cl_null_mlme_list_get_cfm_cb last message received\r\n");
       flag_GetListConfirm = true;
    }
    switch (us_pib_attrib)
    {
      case PIB_MAC_LIST_REGISTER_DEVICES:
        PRIME_LOG(LOG_DBG,"Received PIB_MAC_LIST_REGISTER_DEVICES confirm\r\n");
        // Detect Number of entries
        us_entries = us_pib_len / sizeof(macListRegDevices);
        PRIME_LOG(LOG_DEBUG,"#Entries = %u - %lu - %u\r\n",us_pib_len,sizeof(macListRegDevices),us_entries);
        for (index = 0; index < us_entries; index++){
           memcpy(&reg_device_entry.regEntryID[0],puc_pib_buff,6);
           puc_pib_buff+=6;
           reg_device_entry.regEntryLNID  = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
           puc_pib_buff+=2;
           reg_device_entry.regEntryState = *puc_pib_buff++;
           reg_device_entry.regEntryLSID  = *puc_pib_buff++;
           reg_device_entry.regEntrySID   = *puc_pib_buff++;
           reg_device_entry.regEntryLevel = *puc_pib_buff++;
           reg_device_entry.regEntryTCap  = *puc_pib_buff++;
           reg_device_entry.regEntrySwCap = *puc_pib_buff++;
           print_macListRegDevice(&reg_device_entry);
           // Update the information on the PRIME NETWORK LINKED LIST
           sn = prime_network_find_sn(&prime_network,reg_device_entry.regEntryID);
           if (sn == (prime_sn *) NULL){
              sn = prime_network_add_sn(&prime_network,reg_device_entry.regEntryID, ADMIN_DISABLED, SECURITY_PROFILE_UNKNOWN, NULL);
              if (sn == (prime_sn *) NULL){
                 PRIME_LOG(LOG_ERR,"Imposible to add Service Node\r\n");
                 break;
              }
           }
           prime_network_mutex_lock();
           sn->regEntryLNID = reg_device_entry.regEntryLNID;
           sn->regEntryState = reg_device_entry.regEntryState;
           sn->regEntryLSID = reg_device_entry.regEntryLSID;
           sn->regEntrySID = reg_device_entry.regEntrySID;
           sn->regEntryLevel = reg_device_entry.regEntryLevel;
           sn->regEntryTCap = reg_device_entry.regEntryTCap;
           sn->regEntrySwCap = reg_device_entry.regEntrySwCap;
           sn->regEntryTimeStamp = 0 /* timestamp */;
           sn->registered = TRUE;
           prime_network_mutex_unlock();
           /* Update Maximum Level of PRIME Network */
           if (reg_device_entry.regEntryLevel > prime_network_get_max_level()){
              prime_network_set_max_level(reg_device_entry.regEntryLevel);
           }
        }
        break;
      case PIB_MAC_LIST_ACTIVE_CONN:
        PRIME_LOG(LOG_DBG,"Received PIB_MAC_LIST_ACTIVE_CONN confirm\r\n");
        // Detect Number of entries
        us_entries = us_pib_len / (sizeof(macListActiveConn)-2); // SID move from uint8_t to uint16_t on macListActiveConnEx and append uint8_t ConnType
        PRIME_LOG(LOG_DBG,"#Entries = %u - %lu - %u\r\n",us_pib_len,sizeof(macListActiveConn)-2,us_entries);
        for (index = 0; index < us_entries; index++){
          active_conn_entry.connEntrySID  = *puc_pib_buff++;  // uint8_t on this table uint16_t on Extended table...
          active_conn_entry.connEntryLNID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          active_conn_entry.connEntryLCID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          memcpy(&active_conn_entry.connEntryID[0],puc_pib_buff,6);
          puc_pib_buff+=6;
          active_conn_entry.connType = 255;
          print_macListActiveConn(&active_conn_entry);
          sn = prime_network_find_sn(&prime_network,active_conn_entry.connEntryID);
          if (sn == (prime_sn *) NULL){
             sn = prime_network_add_sn(&prime_network,active_conn_entry.connEntryID, ADMIN_DISABLED, SECURITY_PROFILE_UNKNOWN, NULL);
             if (sn == (prime_sn *) NULL){
                PRIME_LOG(LOG_DBG,"Imposible to add Service Node\r\n");
                break;
             }
          }
          prime_network_mutex_lock();
          sn->regEntryLNID = active_conn_entry.connEntryLNID;
          sn->regEntrySID = active_conn_entry.connEntrySID;
          prime_network_mutex_unlock();
          active_conn_entry_tmp = prime_sn_find_mac_connection(sn, active_conn_entry.connEntryLCID);
          if (active_conn_entry_tmp == (mac_conn *) NULL){
             active_conn_entry_tmp = prime_sn_add_mac_connection(sn, active_conn_entry.connEntryLCID, active_conn_entry.connType);
             if (active_conn_entry_tmp == (mac_conn *) NULL){
                PRIME_LOG(LOG_DBG,"Imposible to add MAC Connection\r\n");
                break;
             }
          }
          prime_network_mutex_lock();
          active_conn_entry_tmp->connType = active_conn_entry.connType;
          prime_network_mutex_unlock();
        }
        break;
      case PIB_MAC_LIST_ACTIVE_CONN_EX:
        PRIME_LOG(LOG_DBG,"Received PIB_MAC_LIST_ACTIVE_CONN_EX confirm\r\n");
        // Detect Number of entries
        us_entries = us_pib_len / sizeof(macListActiveConn);
        PRIME_LOG(LOG_DBG,"#Entries = %u - %lu - %u\r\n",us_pib_len,sizeof(macListActiveConn),us_entries);
        for (index = 0; index < us_entries; index++){
          active_conn_entry.connEntrySID  = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          active_conn_entry.connEntryLNID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          active_conn_entry.connEntryLCID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          memcpy(&active_conn_entry.connEntryID[0],puc_pib_buff,6);
          puc_pib_buff+=6;
          active_conn_entry.connType = *puc_pib_buff++;
          print_macListActiveConn(&active_conn_entry);
          sn = prime_network_find_sn(&prime_network,active_conn_entry.connEntryID);
          if (sn == (prime_sn *) NULL){
             sn = prime_network_add_sn(&prime_network,active_conn_entry.connEntryID, ADMIN_DISABLED, SECURITY_PROFILE_UNKNOWN, NULL);
             if (sn == (prime_sn *) NULL){
                PRIME_LOG(LOG_ERR,"Imposible to add Service Node\r\n");
                break;
             }
          }
          prime_network_mutex_lock();
          sn->regEntryLNID = active_conn_entry.connEntryLNID;
          sn->regEntrySID = active_conn_entry.connEntrySID;
          prime_network_mutex_unlock();
          active_conn_entry_tmp = prime_sn_find_mac_connection(sn, active_conn_entry.connEntryLCID);
          if (active_conn_entry_tmp == (mac_conn *) NULL){
             active_conn_entry_tmp = prime_sn_add_mac_connection(sn, active_conn_entry.connEntryLCID, active_conn_entry.connType);
             if (active_conn_entry_tmp == (mac_conn *) NULL){
                PRIME_LOG(LOG_DBG,"Imposible to add MAC Connection\r\n");
                break;
             }
          }
          prime_network_mutex_lock();
          active_conn_entry_tmp->connType = active_conn_entry.connType;
          prime_network_mutex_unlock();
        }
        break;
      case PIB_MAC_LIST_MCAST_ENTRIES:
        PRIME_LOG(LOG_DBG,"Received PIB_MAC_LIST_MCAST_ENTRIES confirm\r\n");
        // Detect Number of entries
        us_entries = us_pib_len / sizeof(macListMcastEntries);
        PRIME_LOG(LOG_DBG,"#Entries = %u - %lu - %u\r\n",us_pib_len,sizeof(macListMcastEntries),us_entries);
        for (index = 0; index < us_entries; index++){
          mcast_entry.mcastEntryLCID  = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          mcast_entry.mcastEntryMembers = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          print_macListMcastEntries(&mcast_entry);
        }
        break;
      case PIB_MAC_LIST_SWITCH_TABLE:
        PRIME_LOG(LOG_DBG,"Received PIB_MAC_LIST_SWITCH_TABLE confirm\r\n");
        // Detect Number of entries
        us_entries = us_pib_len / sizeof(macListSwitchTable);
        PRIME_LOG(LOG_DBG, "#Entries = %u - %lu - %u\r\n",us_pib_len,sizeof(macListSwitchTable),us_entries);
        for (index = 0; index < us_entries; index++){
          switch_table_entry.stblEntryLNID  = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          switch_table_entry.stblEntryLSID = *puc_pib_buff++;
          switch_table_entry.stbleEntrySID = *puc_pib_buff++;
          switch_table_entry.stblEntryALVTime = *puc_pib_buff++;
          print_macListSwitchTable(&switch_table_entry);
        }
        break;
      case PIB_MAC_LIST_DIRECT_CONN:
        PRIME_LOG(LOG_DBG,"Received PIB_MAC_LIST_DIRECT_CONN confirm\r\n");
        // Detect Number of entries
        us_entries = us_pib_len / sizeof(macListDirectConn);
        PRIME_LOG(LOG_DBG,"#Entries = %u - %lu - %u\r\n",us_pib_len,sizeof(macListDirectConn),us_entries);
        for (index = 0; index < us_entries; index++){
          direct_conn_entry.dconnEntrySrcSID  = *puc_pib_buff++;
          direct_conn_entry.dconEntrySrcLNID  = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          direct_conn_entry.dconnEntrySrcLCID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          memcpy(&direct_conn_entry.dconnEntrySrcID[0],puc_pib_buff,6);
          puc_pib_buff+=6;
          direct_conn_entry.dconnEntryDstSID  = *puc_pib_buff++;
          direct_conn_entry.dconnEntryDstLNID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          direct_conn_entry.dconnEntryDstLCID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          memcpy(&direct_conn_entry.dconnEntryDstID[0],puc_pib_buff,6);
          puc_pib_buff+=6;
          direct_conn_entry.dconnEntryDstSID  = *puc_pib_buff++;
          memcpy(&direct_conn_entry.dconnEntryDID[0],puc_pib_buff,6);
          puc_pib_buff+=6;
          print_macListDirectConn(&direct_conn_entry);
        }
        break;
      case PIB_MAC_LIST_DIRECT_TABLE:
        PRIME_LOG(LOG_DBG,"Received PIB_MAC_LIST_DIRECT_TABLE confirm\r\n");
        // Detect Number of entries
        us_entries = us_pib_len / sizeof(macListDirectTable);
        PRIME_LOG(LOG_DBG,"#Entries = %u - %lu - %u\r\n",us_pib_len,sizeof(macListDirectTable),us_entries);
        for (index = 0; index < us_entries; index++){
          direct_table_entry.dconnEntrySrcSID  = *puc_pib_buff++;
          direct_table_entry.dconEntrySrcLNID  = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          direct_table_entry.dconnEntrySrcLCID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          direct_table_entry.dconnEntryDstSID  = *puc_pib_buff++;
          direct_table_entry.dconnEntryDstLNID  = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          direct_table_entry.dconnEntryDstLCID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          memcpy(&direct_table_entry.dconnEntryDID[0],puc_pib_buff,6);
          puc_pib_buff+=6;
          print_macListDirectTable(&direct_table_entry);
        }
        break;
      case PIB_MAC_LIST_AVAILABLE_SWITCHES:
        PRIME_LOG(LOG_DBG,"Received PIB_MAC_LIST_AVAILABLE_SWITCHES confirm\r\n");
        // Detect Number of entries
        us_entries = us_pib_len / sizeof(macListAvailableSwitches);
        PRIME_LOG(LOG_DBG,"#Entries = %u - %lu - %u\r\n",us_pib_len,sizeof(macListAvailableSwitches),us_entries);
        for (index = 0; index < us_entries; index++){
          memcpy(&available_switches_entry.slistEntrySNA[0],puc_pib_buff,6);
          puc_pib_buff+=6;
          available_switches_entry.slistEntryLSID   = *puc_pib_buff++;
          available_switches_entry.slistEntryLevel  = *puc_pib_buff++;
          available_switches_entry.slistEntryRxLvl  = *puc_pib_buff++;
          available_switches_entry.slistEntryRxSNR  = *puc_pib_buff++;
          print_macListAvailableSwitches(&available_switches_entry);
          /* Update Maximum Level of PRIME Network */
          if (available_switches_entry.slistEntryLevel > prime_network_get_max_level()){
             prime_network_set_max_level(available_switches_entry.slistEntryLevel);
          }
          // Update the information on the PRIME NETWORK LINKED LIST
          sn = prime_network_find_sn(&prime_network,available_switches_entry.slistEntrySNA);
          if (sn == NULL){
             PRIME_LOG(LOG_ERR,"Available Switch not found before\r\n");
             break;
          }
          sn->regEntryLSID    = available_switches_entry.slistEntryLSID;
          sn->regEntryLevel = available_switches_entry.slistEntryLevel;
          sn->regEntryRxLvl = available_switches_entry.slistEntryRxLvl;
          sn->regEntryRxSNR = available_switches_entry.slistEntryRxSNR;
          sn->state           = SN_STATE_SWITCH;
          sn->regEntryState   = REGISTER_STATE_SWITCH;
        }
        break;
      case PIB_MAC_LIST_PHY_COMM:
        PRIME_LOG(LOG_DBG,"Received PIB_MAC_LIST_PHY_COMM confirm\r\n");
        // Detect Number of entries
        us_entries = us_pib_len / sizeof(macListPhyComm);
        PRIME_LOG(LOG_DBG,"#Entries = %u - %lu - %u\r\n",us_pib_len,sizeof(macListPhyComm),us_entries);
        for (index = 0; index < us_entries; index++){
          phy_comm_entry.phyCommLNID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          phy_comm_entry.phyCommSID  = *puc_pib_buff++;
          phy_comm_entry.phyCommTxPwr  = *puc_pib_buff++;
          phy_comm_entry.phyCommRxLvl  = *puc_pib_buff++;
          phy_comm_entry.phyCommSNR  = *puc_pib_buff++;
          phy_comm_entry.phyCommTxModulation  = *puc_pib_buff++;
          phy_comm_entry.phyCommPhyTypeCapability  = *puc_pib_buff++;
          phy_comm_entry.phyCommRxAge = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          puc_pib_buff+=2;
          print_macListPhyComm(&phy_comm_entry);
        }
        break;
      case PIB_432_LIST_NODES:
        PRIME_LOG(LOG_DBG,"PIB_432_LIST_NODES\r\n");
        // Detect Number of entries
        us_entries = us_pib_len / sizeof(cl432ListNode);
        PRIME_LOG(LOG_DBG,"#Entries = %u - %lu - %u\r\n",us_pib_len,sizeof(cl432ListNode),us_entries);
        for (index = 0; index < us_entries; index++){
          cl432_node_entry.cl432address = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
          if (cl432_node_entry.cl432address >= (0xFFF -1)) {
        		 /* Broadcast connection, ignore */
        		 puc_pib_buff+=sizeof(cl432ListNode);
        	   continue;
        	}
          puc_pib_buff+=2;
          memcpy(&cl432_node_entry.cl432serial[0],puc_pib_buff,16);
          puc_pib_buff+=16;
          cl432_node_entry.cl432serial_len = *puc_pib_buff++;
          memcpy(&cl432_node_entry.cl432mac[0],puc_pib_buff,6);
          puc_pib_buff+=6;
          print_cl432ListNode(&cl432_node_entry);
          sn = prime_network_find_sn(&prime_network,cl432_node_entry.cl432mac);
          if (sn != (prime_sn *) NULL){
             prime_network_mutex_lock();
             sn->cl432Conn.connState = CL432_CONN_STATE_OPEN;
             sn->cl432Conn.connAddress = cl432_node_entry.cl432address;
             memcpy(sn->cl432Conn.connSerialNumber,cl432_node_entry.cl432serial,16);
             sn->cl432Conn.connLenSerial = cl432_node_entry.cl432serial_len;
             memcpy(sn->cl432Conn.connMAC, cl432_node_entry.cl432mac, 6);
             prime_network_mutex_unlock();
          }
        }
        break;
      default:
        break;
    }
  }

  return ;
}

/**
 * \brief MLME set request syncronous
 * \param us_pib_attrib  PIB attribute
 * \param pv_pib_value   PIB Value pointer
 * \param uc_pib_size    PIB Value pointer size
 * \param uc_timeout     Timeout for the request
 * \param pmacSetConfirm Set Confirm pointer
 * \result -
 */
void prime_cl_null_mlme_set_request_sync(uint16_t us_pib_attrib, void *pv_pib_value, uint8_t uc_pib_size, uint8_t uc_timeout, struct TmacSetConfirm *pmacSetConfirm)
{
  uint8_t result = -1;

	PRIME_LOG(LOG_DBG,"prime_cl_null_mlme_set_request_sync attr = 0x%04X\r\n", us_pib_attrib);

	// Set the sync flags to intercept the callback
	g_prime_sync_mgmt.f_sync_req = true;
	g_prime_sync_mgmt.f_sync_res = false;
	g_prime_sync_mgmt.m_u16AttributeId = us_pib_attrib; /* Used in order to know AttributeId requested on GetConfirm callback */

	// Send the asynchronous call
	prime_cl_null_mlme_set_request(us_pib_attrib, pv_pib_value, uc_pib_size);

	// Wait processing until flag activates, or timeout
	addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

	if (g_prime_sync_mgmt.f_sync_res){
		// Confirm received
    pmacSetConfirm->m_u8Status = g_prime_sync_mgmt.s_macSetConfirm.m_u8Status;
  	pmacSetConfirm->m_u16AttributeId = g_prime_sync_mgmt.s_macSetConfirm.m_u16AttributeId;
		result = 0;
	}
	else{
		// Confirm not received
		pmacSetConfirm->m_u8Status = -1;
		PRIME_LOG(LOG_ERR,"ERROR: Confirm for 0x%04X not received\r\n",us_pib_attrib);
	}

	g_prime_sync_mgmt.f_sync_req = false;
	g_prime_sync_mgmt.f_sync_res = false;

	PRIME_LOG(LOG_DBG,"prime_cl_null_mlme_set_request_sync result = %d\r\n",result);
}

void _prime_cl_null_mlme_set_cfm_cb(mlme_result_t x_result)
{
  PRIME_LOG(LOG_DBG,"_prime_cl_null_mlme_set_cfm_cb result = 0x%04X\r\n", x_result);
  memset(&g_prime_sync_mgmt.s_macSetConfirm,0,sizeof(struct TmacSetConfirm));
//  if (g_prime_sync_mgmt.f_sync_req && (g_prime_sync_mgmt.m_u16AttributeId == us_pib_attrib)){
      g_prime_sync_mgmt.s_macSetConfirm.m_u8Status = x_result;
//      g_prime_sync_mgmt.s_macSetConfirm.m_u16AttributeId = us_pib_attrib;
		  g_prime_sync_mgmt.f_sync_res = true;
//  }
  return ;
}

/**
 * \brief  CL 432 Data indication
 * \param  uc_dst_lsap      Destination LSAP
 * \param  uc_src_lsap      Source LSAP
 * \param  us_dst_address   Destination 432 Address
 * \param  src_address      Source Address
 * \param  puc_data         Pointer to received data
 * \param  us_lsdu_len      Length of the data
 * \param  uc_link_class    Link class (non used)
 * \result -
 */
void _prime_cl_432_dl_data_ind_cb(uint8_t uc_dst_lsap, uint8_t uc_src_lsap, uint16_t us_dst_address, uint16_t src_address, uint8_t *puc_data, uint16_t us_lsdu_len, uint8_t uc_link_class)
{
  PRIME_LOG(LOG_DBG,"_prime_cl_432_dl_data_ind_cb information: \r\n"
                 "\r\t- Destination LSAP   : %05d\r\n"
                 "\r\t- Source LSAP        : %05d\r\n"
                 "\r\t- Destination Address: %05d\r\n"
                 "\r\t- Source Address     : %05d\r\n"
                 "\r\t- Data Lenght        : %05d\r\n"
                 "\r\t- Link Class         : %03d\r\n",
                 uc_dst_lsap,                           \
                 uc_src_lsap,                           \
                 us_dst_address,                        \
                 src_address,                           \
                 us_lsdu_len,                           \
                 uc_link_class);
   //PRIME_LOG(LOG_DBG,puc_data, us_lsdu_len, "\r\t- Data          : %X");
}

/**
 * \brief  CL 432 Data confirm
 * \param  uc_dst_lsap      Destination LSAP
 * \param  uc_src_lsap      Source LSAP
 * \param  us_dst_address   Destination 432 Address
 * \param  uc_tx_status     Tx status
 */
void _prime_cl_432_dl_data_cfm_cb(uint8_t uc_dst_lsap, uint8_t uc_src_lsap, uint16_t us_dst_address, uint8_t uc_tx_status)
{
  PRIME_LOG(LOG_DBG,"_prime_cl_432_dl_data_cfm_cb: \r\n"
                 "\r\t- Destination LSAP   : %05d\r\n"
                 "\r\t- Source LSAP        : %05d\r\n"
                 "\r\t- Destination Address: %05d\r\n"
                 "\r\t- TX Status          : %05d\r\n",
                 uc_dst_lsap,                           \
                 uc_src_lsap,                           \
                 us_dst_address,                        \
                 uc_tx_status);

}

/**
 * \brief CL 432 Leave Indication callback
 * \param uc_lnid LNID of the node closing the 432 connection
 */
void _prime_cl_432_dl_leave_ind_cb(uint16_t us_lnid)
{
  PRIME_LOG(LOG_DBG,"_prime_cl_432_dl_leave_ind_cb: \r\n"
                 "\r\t- LNID   : %05d\r\n",
                 us_lnid);
}

/**
 * \brief CL 432 Join Indication callback
 * \param puc_device_id    Device ID
 * \param uc_device_id_len Device ID Lenght
 * \param us_dst_address   CL4-32 Destination Address
 * \param puc_mac          MAC EUI48
 */
void _prime_cl_432_dl_join_ind_cb(uint8_t *puc_device_id, uint8_t uc_device_id_len, uint16_t us_dst_address, uint8_t *puc_mac)
{
  PRIME_LOG(LOG_DBG,"_prime_cl_432_dl_join_ind_cb_t information: \r\n"
                    "\r\t- Destination Address: %05d\r\n"
                    "\r\t- EUI48              : %s\r\n",
                                us_dst_address,                        \
                                eui48_to_str(puc_mac, NULL));
}

/**
 * \brief Decode Management Response on Local and Base Management Planes
 * \param mng_plane Management Plane Local or Remote
 * \param eui48     EUI48 MAC address
 * \param ptrMsg    Pointer to MAC Data response
 * \param len       MAC Data Response Length
 */
void prime_common_mng_rsp_cb(uint8_t mng_plane, uint8_t *eui48, uint8_t* ptrMsg, uint16_t len)
{
    uint16_t us_pib_attrib;
    uint8_t  uc_index;
    uint8_t  *puc_pib_buff;
    uint16_t us_pib_size;
    uint8_t  uc_next=0;
    uint8_t  *ptr;
    uint8_t  uc_enhanced_pib;
    uint8_t  uc_records=0;
    uint8_t  uc_record_len=0;
    uint16_t us_last_iterator=0;
    //uint8_t  uc_value=0;
    uint16_t us_value=0;
    uint32_t ui_value=0;
    macListRegDevices reg_device_entry;
    macListActiveConn active_conn_entry;
    mac_conn *active_conn_entry_tmp;
    macListMcastEntries mcast_entry;
    macListSwitchTable switch_table_entry;
    macListDirectConn direct_conn_entry;
    macListDirectTable direct_table_entry;
    macListAvailableSwitches available_switches_entry;
    macListPhyComm phy_comm_entry;
    macListFU fu_entry;
    cl432ListNode cl432_node_entry;
    prime_sn *sn;

    /*Pib Attribute (2 bytes) | Index (1 byte) | Pib Value (var) | Next (1 byte) */
    PRIME_LOG(LOG_DBG,"prime_mng_rsp_cb information\r\n");
    //for (uc_index=0;uc_index<len;uc_index++)
    //   printf("%02X",ptrMsg[uc_index]);fflush(stdout);
    //printf("\r\n");

    ptr = ptrMsg;

    if (len < 4) // Error!!! Message too short
        return;

    if (ptrMsg[0] != MNGP_PRIME_LISTRSP)
    {
    	  /* Standard response for a PIB */
        us_pib_attrib   = ((*ptr++) << 8);
        us_pib_attrib  += (*ptr++);
        uc_index        = (*ptr++);
        switch (us_pib_attrib){
           /* 4 Bytes Lenght Response */
           /* PHY */
           case PIB_PHY_STATS_RX_TOTAL_COUNT:
           case PIB_PHY_TX_PROCESSING_DELAY:
           case PIB_PHY_RX_PROCESSING_DELAY:
           /* MAC */
           /* MAC Read Only Statistical Variables */
           case PIB_MAC_TX_DATAPKT_COUNT:
           case PIB_MAC_RX_DATAPKT_COUNT:
           case PIB_MAC_TX_CTRLPKT_COUNT:
           case PIB_MAC_RX_CTRLPKT_COUNT:
           case PIB_MAC_CSMA_FAIL_COUNT:
           case PIB_MAC_CSMA_CH_BUSY_COUNT:
           /* Propietary PHY PIBs */
           case PIB_PHY_SW_VERSION:
           case PIB_PHY_ZCT:
           case PIB_PHY_HOST_VERSION:
           /* Propietary MTP PIBs */
           case PIB_MTP_PHY_TX_TIME:
           case PIB_MTP_PHY_RMS_CALC_CORRECTED:
           /* Propietary MAC PIBs */
           case PIB_MAC_INTERNAL_SW_VERSION:
           /* Propietary CL4-32 PIBs */
           case PIB_432_INTERNAL_SW_VERSION:
              us_pib_size = 4;
              break;
           /* 2 Bytes Lenght Response */
           /* PHY */
           case PIB_PHY_STATS_CRC_INCORRECT:
           case PIB_PHY_STATS_CRC_FAIL_COUNT:
           case PIB_PHY_STATS_TX_DROP_COUNT:
           case PIB_PHY_STATS_RX_DROP_COUNT:
           case PIB_PHY_STATS_BLK_AVG_EVM:
           case PIB_PHY_TX_QUEUE_LEN:
           case PIB_PHY_RX_QUEUE_LEN:
           /* MAC */
           /* MAC Read-Write Attribute Variables */
           case PIB_MAC_UPDATED_RM_TIMEOUT:
           /* MAC Read Only Functional Variables */
           case PIB_MAC_LNID:
           case PIB_MAC_SCP_LENGTH:
           case PIB_MAC_BEACON_RX_POS:
           case PIB_MAC_MAC_CAPABILITES:
           case PIB_MAC_FRAME_LENGTH:
           case PIB_MAC_CFP_LENGTH:
           case PIB_MAC_GUARD_TIME:
           case PIB_MAC_BC_MODE:
           case PIB_MAC_BEACON_RX_QLTY:
           case PIB_MAC_BEACON_TX_QLTY:
           /* Propietary MTP PIBs */
           case PIB_MTP_PHY_CFG_LOAD_THRESHOLD_1:
           case PIB_MTP_PHY_CFG_LOAD_THRESHOLD_2:
           case PIB_MTP_PHY_EXECUTE_CALIBRATION:
           /* Propietary MAC PIBs */
           case PIB_MAC_ACTION_CFP_LENGTH:
           /* Propietary APP PIBs */
           case PIB_MAC_APP_VENDOR_ID:
           case PIB_MAC_APP_PRODUCT_ID:
              us_pib_size = 2;
              break;
          /* 1 Byte Lenght Response */
          /* PHY */
           case PIB_PHY_EMA_SMOOTHING:
           case PIB_PHY_AGC_MIN_GAIN:
           case PIB_PHY_AGC_STEP_VALUE:
           case PIB_PHY_AGC_STEP_NUMBER:
           /* MAC */
           /* MAC Read-Write Attribute Variables */
           case PIB_MAC_VERSION:
           case PIB_MAC_MIN_SWITCH_SEARCH_TIME:
           case PIB_MAC_MAX_PROMOTION_PDU:
           case PIB_MAC_PROMOTION_PDU_TX_PERIOD:
           case PIB_MAC_SCP_MAX_TX_ATTEMPTS:
           case PIB_MAC_MIN_CTL_RE_TX_TIMER:
           case PIB_MAC_CTL_MSG_FAIL_TIME:
           case PIB_MAC_EMA_SMOOTHING:
           case PIB_MAC_MIN_BAND_SEARCH_TIME:
           case PIB_MAC_SAR_SIZE:
           case PIB_MAC_ACTION_ROBUSTNESS_MGMT:
           case PIB_MAC_ALV_HOP_REPETITIONS:
           /* MAC Read Only Attribute Variables */
           case PIB_MAC_SCP_CH_SENSE_COUNT:
           case PIB_MAC_CSMA_R1:
           case PIB_MAC_CSMA_R2:
           case PIB_MAC_CSMA_DELAY:
           case PIB_MAC_CSMA_R1_ROBUST:
           case PIB_MAC_CSMA_R2_ROBUST:
           case PIB_MAC_CSMA_DELAY_ROBUST:
           case PIB_MAC_ALV_TIME_MODE:
           /* MAC Read Only Functional Variables */
           case PIB_MAC_LSID:
           case PIB_MAC_SID:
           case PIB_MAC_STATE:
           case PIB_MAC_NODE_HIERARCHY_LEVEL:
           case PIB_MAC_BEACON_TX_POS:
           case PIB_MAC_BEACON_RX_FREQUENCY:
           case PIB_MAC_BEACON_TX_FREQUENCY:
           /* Propietary PHY Variables */
           case PIB_PHY_TX_CHANNEL:
           case PIB_PHY_TXRX_CHANNEL_LIST:
           case PIB_PHY_SNIFFER_ENABLED:
           case PIB_MTP_PHY_ENABLE:
           /* Propietary MTP Variables */
           case PIB_MTP_PHY_CONTINUOUS_TX:
           case PIB_MTP_PHY_DRV_AUTO:
           case PIB_MTP_PHY_DRV_IMPEDANCE:
           /* Propietary MAC Variables */
           case PIB_MAC_PLC_STATE:
           case PIB_MAC_ALV_MIN_LEVEL:
           case PIB_MAC_ACTION_FRAME_LENGTH:
           case PIB_CERTIFICATION_MODE:
           case PIB_MAC_ACTION_ARQ_WIN_SIZE:
           case PIB_MAC_ACTION_BCN_TX_SCHEME:
           case PIB_MAC_ACTION_ALV_TYPE:
           /* Propietary CL4-32 Variables */
           case PIB_432_CON_STATE:
              us_pib_size = 1;
              break;
           case PIB_MAC_EUI_48:
           case PIB_MAC_SNA:
           case PIB_MTP_MAC_EUI_48:
              us_pib_size = 6;
              break;
           case PIB_MAC_APP_FW_VERSION:
              us_pib_size = 16;
              break;
           default:
              /* We don't consider List with the normal get_response method */
              us_pib_size = 0;
              break;
        }
        uc_next = ptr[us_pib_size];
        PRIME_LOG(LOG_DBG,"MNGP Get PIB Attribute Query Attribute=%04X, Index=%d, Next=%d\r\n",us_pib_attrib, uc_index, uc_next);
        if (g_prime_sync_mgmt.f_sync_req && (g_prime_sync_mgmt.m_u16AttributeId == us_pib_attrib)) {
            g_prime_sync_mgmt.s_macGetConfirm.m_u8Status = 0;
            g_prime_sync_mgmt.s_macGetConfirm.m_u16AttributeId = us_pib_attrib;
            g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength = us_pib_size;
            /* Information is sent in big endian */
            if (us_pib_size == 1){
               g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue[0] = *ptr;
            }else if (us_pib_size == 2){
               us_value = ((*ptr++) << 8);
               us_value += *ptr;
               memcpy(&g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue[0],(uint8_t *)&us_value,us_pib_size);
            }else if (us_pib_size == 4){
               ui_value  = ((*ptr++) << 24);
               ui_value += ((*ptr++) << 16);
               ui_value += ((*ptr++) << 8);
               ui_value += (*ptr++);
               memcpy(&g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue[0],(uint8_t *)&ui_value,us_pib_size);
            }else{
               memcpy(&g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue[0],ptr,us_pib_size);
            }
            g_prime_sync_mgmt.f_sync_res = true;
        }
    } else {
        /* Enhanced response for a PIB */
    	  int16_t i = 0;
        //Enhanced PIB
        uc_enhanced_pib = *ptr++;
        us_pib_attrib   = ((uint16_t)(*ptr++)) << 8;
        us_pib_attrib  += (uint16_t)(*ptr++);
        uc_records      = *ptr++;
        uc_record_len   = *ptr++;
        us_pib_size     = *ptr++;

        /* Load data */
        us_last_iterator = 0xFFFF;
        puc_pib_buff = ptr;

        PRIME_LOG(LOG_DBG,"EnhancedPIB=0x%02X Attribute=0x%04X, Records=%d, RecordLength=%d, Size=%d\r\n",uc_enhanced_pib,us_pib_attrib, uc_records,uc_record_len,us_pib_size);

        for ( i= 0; i < uc_records; i++)
        {
        	us_last_iterator =((puc_pib_buff[0] << 8) + puc_pib_buff[1]);
          puc_pib_buff+=2;
          //PRIME_LOG(LOG_DBG,"Iterator=0x%04X\r\n",us_last_iterator);
        	/* Get one record... */
          switch (us_pib_attrib)
          {
            case PIB_MAC_LIST_REGISTER_DEVICES:
              //PRIME_LOG(LOG_DBG,"PIB_MAC_LIST_REGISTER_DEVICES\r\n");
              memcpy(&reg_device_entry.regEntryID[0],puc_pib_buff,6);
              puc_pib_buff+=6;
              reg_device_entry.regEntryLNID  = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              reg_device_entry.regEntryState = *puc_pib_buff++;
              reg_device_entry.regEntryLSID  = *puc_pib_buff++;
              reg_device_entry.regEntrySID   = *puc_pib_buff++;
              reg_device_entry.regEntryLevel = *puc_pib_buff++;
              reg_device_entry.regEntryTCap  = *puc_pib_buff++;
              reg_device_entry.regEntrySwCap = *puc_pib_buff++;
              print_macListRegDevice(&reg_device_entry);
              // Do we need to save the information on the PRIME NETWORK LINKED LIST ???
              sn = prime_network_find_sn(&prime_network,reg_device_entry.regEntryID);
              if (sn == NULL){
                 sn = prime_network_add_sn(&prime_network,reg_device_entry.regEntryID, ADMIN_DISABLED,SECURITY_PROFILE_UNKNOWN,NULL);
                 if (sn == NULL){
                    PRIME_LOG(LOG_ERR,"Imposible to add Service Node\r\n");
                 }
              }
              prime_network_mutex_lock();
              sn->regEntryLNID  = reg_device_entry.regEntryLNID;
              sn->regEntryState = reg_device_entry.regEntryState;
              sn->regEntryLSID  = reg_device_entry.regEntryLSID;
              sn->regEntrySID   = reg_device_entry.regEntrySID;
              sn->regEntryLevel = reg_device_entry.regEntryLevel;
              sn->regEntryTCap  = reg_device_entry.regEntryTCap;
              sn->regEntrySwCap = reg_device_entry.regEntrySwCap;
              sn->registered    = TRUE;
              prime_network_mutex_unlock();
              break;
            case PIB_MAC_LIST_ACTIVE_CONN:
              //PRIME_LOG(LOG_DBG,"PIB_MAC_LIST_ACTIVE_CONN\r\n");
              active_conn_entry.connEntrySID  = *puc_pib_buff++;  // uint8_t on this table uint16_t on Extended table...
              active_conn_entry.connEntryLNID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              active_conn_entry.connEntryLCID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              memcpy(&active_conn_entry.connEntryID[0],puc_pib_buff,6);
              puc_pib_buff+=6;
              active_conn_entry.connType = 255;
              print_macListActiveConn(&active_conn_entry);
              sn = prime_network_find_sn(&prime_network,active_conn_entry.connEntryID);
              if (sn == NULL){
                 sn = prime_network_add_sn(&prime_network,active_conn_entry.connEntryID, ADMIN_DISABLED, SECURITY_PROFILE_UNKNOWN, NULL);
                 if (sn == NULL){
                    PRIME_LOG(LOG_ERR,"Imposible to add Service Node\r\n");
                 }
              }
              prime_network_mutex_lock();
              sn->regEntryLNID = active_conn_entry.connEntryLNID;
              sn->regEntrySID = active_conn_entry.connEntrySID;
              prime_network_mutex_unlock();
              active_conn_entry_tmp = prime_sn_find_mac_connection(sn, active_conn_entry.connEntryLCID);
              if (active_conn_entry_tmp == (mac_conn *) NULL){
                 active_conn_entry_tmp = prime_sn_add_mac_connection(sn, active_conn_entry.connEntryLCID, active_conn_entry.connType);
                 if (active_conn_entry_tmp == (mac_conn *) NULL){
                    PRIME_LOG(LOG_DBG,"Imposible to add MAC Connection\r\n");
                    break;
                 }
              }
              break;
            case PIB_MAC_LIST_ACTIVE_CONN_EX:
              //PRIME_LOG(LOG_DBG,"PIB_MAC_LIST_ACTIVE_CONN_EX\r\n");
              active_conn_entry.connEntrySID  = *puc_pib_buff++;
              active_conn_entry.connEntryLNID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              active_conn_entry.connEntryLCID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              memcpy(&active_conn_entry.connEntryID[0],puc_pib_buff,6);
              puc_pib_buff+=6;
              active_conn_entry.connType = *puc_pib_buff++;
              print_macListActiveConn(&active_conn_entry);
              sn = prime_network_find_sn(&prime_network,active_conn_entry.connEntryID);
              if (sn == NULL){
                 sn = prime_network_add_sn(&prime_network,active_conn_entry.connEntryID, ADMIN_DISABLED, SECURITY_PROFILE_UNKNOWN, NULL);
                 if (sn == NULL){
                    PRIME_LOG(LOG_ERR,"Imposible to add Service Node\r\n");
                 }
              }
              prime_network_mutex_lock();
              sn->regEntryLNID = active_conn_entry.connEntryLNID;
              sn->regEntrySID = active_conn_entry.connEntrySID;
              prime_network_mutex_unlock();
              active_conn_entry_tmp = prime_sn_find_mac_connection(sn, active_conn_entry.connEntryLCID);
              if (active_conn_entry_tmp == (mac_conn *) NULL){
                 active_conn_entry_tmp = prime_sn_add_mac_connection(sn, active_conn_entry.connEntryLCID, active_conn_entry.connType);
                 if (active_conn_entry_tmp == (mac_conn *) NULL){
                    PRIME_LOG(LOG_DBG,"Imposible to add MAC Connection\r\n");
                    break;
                 }
              }
              break;
            case PIB_MAC_LIST_MCAST_ENTRIES:
              //PRIME_LOG(LOG_DBG,"PIB_MAC_LIST_MCAST_ENTRIES\r\n");
              mcast_entry.mcastEntryLCID  = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              mcast_entry.mcastEntryMembers = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              print_macListMcastEntries(&mcast_entry);
              break;
            case PIB_MAC_LIST_SWITCH_TABLE:
              //PRIME_LOG(LOG_DBG,"PIB_MAC_LIST_SWITCH_TABLE\r\n");
              switch_table_entry.stblEntryLNID  = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              switch_table_entry.stblEntryLSID = *puc_pib_buff++;
              switch_table_entry.stbleEntrySID = *puc_pib_buff++;
              switch_table_entry.stblEntryALVTime = *puc_pib_buff++;
              print_macListSwitchTable(&switch_table_entry);
              break;
            case PIB_MAC_LIST_DIRECT_CONN:
              //PRIME_LOG(LOG_DBG,"PIB_MAC_LIST_DIRECT_CONN\r\n");
              direct_conn_entry.dconnEntrySrcSID  = *puc_pib_buff++;
              direct_conn_entry.dconEntrySrcLNID  = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              direct_conn_entry.dconnEntrySrcLCID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              memcpy(&direct_conn_entry.dconnEntrySrcID[0],puc_pib_buff,6);
              puc_pib_buff+=6;
              direct_conn_entry.dconnEntryDstSID  = *puc_pib_buff++;
              direct_conn_entry.dconnEntryDstLNID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              direct_conn_entry.dconnEntryDstLCID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              memcpy(&direct_conn_entry.dconnEntryDstID[0],puc_pib_buff,6);
              puc_pib_buff+=6;
              direct_conn_entry.dconnEntryDstSID  = *puc_pib_buff++;
              memcpy(&direct_conn_entry.dconnEntryDID[0],puc_pib_buff,6);
              puc_pib_buff+=6;
              print_macListDirectConn(&direct_conn_entry);
              break;
            case PIB_MAC_LIST_DIRECT_TABLE:
              //PRIME_LOG(LOG_DBG,"PIB_MAC_LIST_DIRECT_TABLE\r\n");
              direct_table_entry.dconnEntrySrcSID  = *puc_pib_buff++;
              direct_table_entry.dconEntrySrcLNID  = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              direct_table_entry.dconnEntrySrcLCID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              direct_table_entry.dconnEntryDstSID  = *puc_pib_buff++;
              direct_table_entry.dconnEntryDstLNID  = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              direct_table_entry.dconnEntryDstLCID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              memcpy(&direct_table_entry.dconnEntryDID[0],puc_pib_buff,6);
              puc_pib_buff+=6;
              print_macListDirectTable(&direct_table_entry);
              break;
            case PIB_MAC_LIST_AVAILABLE_SWITCHES:
              //PRIME_LOG(LOG_DBG,"PIB_MAC_LIST_AVAILABLE_SWITCHES\r\n");
              memcpy(&available_switches_entry.slistEntrySNA[0],puc_pib_buff,6);
              puc_pib_buff+=6;
              available_switches_entry.slistEntryLSID  = *puc_pib_buff++;
              available_switches_entry.slistEntryLevel  = *puc_pib_buff++;
              available_switches_entry.slistEntryRxLvl  = *puc_pib_buff++;
              available_switches_entry.slistEntryRxSNR  = *puc_pib_buff++;
              print_macListAvailableSwitches(&available_switches_entry);
              break;
            case PIB_MAC_LIST_PHY_COMM:
              //PRIME_LOG(LOG_DBG,"PIB_MAC_LIST_PHY_COMM\r\n");
              phy_comm_entry.phyCommLNID = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              phy_comm_entry.phyCommSID  = *puc_pib_buff++;
              phy_comm_entry.phyCommTxPwr  = *puc_pib_buff++;
              phy_comm_entry.phyCommRxLvl  = *puc_pib_buff++;
              phy_comm_entry.phyCommSNR  = *puc_pib_buff++;
              phy_comm_entry.phyCommTxModulation  = *puc_pib_buff++;
              phy_comm_entry.phyCommPhyTypeCapability  = *puc_pib_buff++;
              phy_comm_entry.phyCommRxAge = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              print_macListPhyComm(&phy_comm_entry);
              break;
            case PIB_FU_LIST:
              //PRIME_LOG(LOG_DBG,"PIB_FU_LIST\r\n");
              fu_entry.fuNodeState = *puc_pib_buff++;
              fu_entry.fuPagesCompleted = puc_pib_buff[3] + (puc_pib_buff[2]<<8) + (puc_pib_buff[1]<<16) + (puc_pib_buff[0]<<24);
              puc_pib_buff+=4;
              //fu_entry.fuLNID  = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              //puc_pib_buff+=2;
              fu_entry.fuLNID = 0;
              memcpy(&fu_entry.fuMAC[0],puc_pib_buff,6);
              puc_pib_buff+=6;
              print_macListFU(&fu_entry);
              sn = prime_network_find_sn(&prime_network, (const unsigned char *)&fu_entry.fuMAC);
              if (sn == NULL){
                 PRIME_LOG(LOG_ERR,"MAC Address %s not found on Network List\r\n", eui48_to_str((const unsigned char *)&fu_entry.fuMAC, NULL));
                 break;
              }
              prime_network_mutex_lock();
              if (!sn->fwup_en)
                sn->fwup_en = 1;
              sn->fu_state = fu_entry.fuNodeState;
              sn->fu_pages = fu_entry.fuPagesCompleted;
              prime_network_mutex_unlock();
              break;
            case PIB_432_LIST_NODES:
              //PRIME_LOG(LOG_DBG,"PIB_432_LIST_NODES\r\n");
              cl432_node_entry.cl432address = puc_pib_buff[1] + (puc_pib_buff[0]<<8);
              puc_pib_buff+=2;
              memcpy(&cl432_node_entry.cl432serial[0],puc_pib_buff,16);
              puc_pib_buff+=16;
              cl432_node_entry.cl432serial_len = *puc_pib_buff++;
              memcpy(&cl432_node_entry.cl432mac[0],puc_pib_buff,6);
              puc_pib_buff+=6;
              print_cl432ListNode(&cl432_node_entry);
              sn = prime_network_find_sn(&prime_network,cl432_node_entry.cl432mac);
              if (sn != (prime_sn *) NULL){
                 prime_network_mutex_lock();
                 sn->cl432Conn.connState = CL432_CONN_STATE_OPEN;
                 sn->cl432Conn.connAddress = cl432_node_entry.cl432address;
                 memcpy(sn->cl432Conn.connSerialNumber,cl432_node_entry.cl432serial,16);
                 sn->cl432Conn.connLenSerial = cl432_node_entry.cl432serial_len;
                 memcpy(sn->cl432Conn.connMAC, cl432_node_entry.cl432mac, 6);
                 prime_network_mutex_unlock();
              }
              break;
            default:
              break;

           	/* advance buffer pointer to record length plus iterator length */
           	//puc_pib_buff+= uc_record_len +2;
          }
        }

        if (uc_records == 0) //|| (uc_records < MNGP_PRIME_ENHANCED_LIST_MAX_RECORDS)) // Last element -> push
        {
           if (g_prime_sync_mgmt.f_sync_req && (g_prime_sync_mgmt.m_u16AttributeId == us_pib_attrib)){
              g_prime_sync_mgmt.s_macGetConfirm.m_u8Status = 0;
              g_prime_sync_mgmt.s_macGetConfirm.m_u16AttributeId = us_pib_attrib;
              g_prime_sync_mgmt.f_sync_res = true;
           }
        }else {

        	uint16_t us_bigendian_iterator=0;
        	if (us_last_iterator == 0xFFFF)
        		return ;
        	if ((us_last_iterator & 0x8000)!= 0)
        	    return ;

        	//Ask for the next iterator..
        	us_last_iterator++;

        	//reverse interator to allow a direct "memcopy"
        	us_bigendian_iterator  = (us_last_iterator & 0xFF ) << 8;
        	us_bigendian_iterator += (us_last_iterator >> 8 ) & 0xFF;

        	/* Request next elements on the list */
          if (mng_plane == MNG_PLANE_LOCAL){
            /* Local Management Plane */
          	mngLay_NewMsg(MNGP_PRIME_EN_PIBQRY);
          	mngLay_AddGetPibListEnQuery(us_pib_attrib, MNGP_PRIME_ENHANCED_LIST_MAX_RECORDS, (uint8_t*) &us_bigendian_iterator);
          	mngLay_SendMsg();
          }else{
            /* Base Management Plane */
            bmng_pprof_get_request(eui48, us_pib_attrib, us_bigendian_iterator);
          }
        }
    }
    return ;
}

/**
 * \brief Management protocol decoding for MNGP
 * param  ptrMsg: Pointer to MAC Data Response
 * param  len   : MAC Data Response length
 */
void prime_mngp_rsp_cb(uint8_t* ptrMsg, uint16_t len){
    prime_common_mng_rsp_cb(MNG_PLANE_LOCAL,NULL,ptrMsg,len);
}

/**
 * \brief Management Plane Get Request Synchronous
 *
 * \param us_pib_attrib  PRIME Attribute to get
 * \param uc_timeout     Timeout for request
 * \param pmacGetConfirm PRIME MAC Get Confirm Pointer
 *
 */
void prime_mngp_get_request_sync(uint16_t us_pib_attrib, uint8_t uc_timeout, struct TmacGetConfirm *pmacGetConfirm)
{

  PRIME_LOG(LOG_DBG,"prime_mngp_get_request_sync attr = 0x%04X\r\n", us_pib_attrib);

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  g_prime_sync_mgmt.m_u16AttributeId = us_pib_attrib; /* Used in order to know AttributeId requested on GetConfirm callback */

  // Send the asynchronous call
  mngLay_NewMsg(MNGP_PRIME_GETQRY);
  mngLay_AddGetPibQuery(us_pib_attrib ,0);
  mngLay_SendMsg();

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (g_prime_sync_mgmt.f_sync_res){
    // Confirm received
    pmacGetConfirm->m_u8Status = 0;  // No Status on Response Callback
    pmacGetConfirm->m_u16AttributeId = g_prime_sync_mgmt.s_macGetConfirm.m_u16AttributeId;
    pmacGetConfirm->m_u8AttributeLength = g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength;
    /* The Serial Profile Management Plane doesn't need to update the endianess of data */
    memcpy(&pmacGetConfirm->m_au8AttributeValue, &g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue,g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength);
  }
  else{
    // Confirm not received
    pmacGetConfirm->m_u8Status = -1;
    PRIME_LOG(LOG_ERR,"ERROR: Confirm for 0x%04X not received\r\n",us_pib_attrib);
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DBG,"prime_mngp_get_request_sync result = %d\r\n",pmacGetConfirm->m_u8Status);
}


/**
 * \brief Management Plane Set Request ASynchronous
 *
 * \param us_pib_attrib  PRIME Attribute to get
 * \param value          PRIME Attribute Value
 * \param length         PRIME Attribute Value Length
 *
 */
void prime_mngp_set_request(uint16_t us_pib_attrib, uint8_t* value, uint8_t length)
{
  // Send the asynchronous call
  mngLay_NewMsg(MNGP_PRIME_SET);
  mngLay_AddSetPib(us_pib_attrib, length, value);
  mngLay_SendMsg();
}

/**
 * \brief Management Plane Set Request Synchronous => Write and Read to verify
 *
 * \param us_pib_attrib  PRIME Attribute to set
 * \param value          PRIME Attribute Value pointer
 * \param length         PRIME Attribute Value Length
 * \param uc_timeout     Timeout for the request
 * \param pmacSetConfirm PRIME MAC Set Confirm Pointer
 *
 */
void prime_mngp_set_request_sync(uint16_t us_pib_attrib, uint8_t* value, uint8_t length, uint8_t uc_timeout, struct TmacSetConfirm *pmacSetConfirm)
{
  struct TmacGetConfirm pmacGetConfirm;
  uint8_t *ptr = &pmacGetConfirm.m_au8AttributeValue[0];

  // Send the asynchronous request
  prime_mngp_set_request(us_pib_attrib, value, length);
  // TO CHECK because it's not sent!
  usleep(10000);
  prime_mngp_get_request_sync(us_pib_attrib, uc_timeout, &pmacGetConfirm);
  pmacSetConfirm->m_u8Status = pmacGetConfirm.m_u8Status;  // No Status on Response Callback
  if ((pmacSetConfirm->m_u8Status == 0) && (pmacGetConfirm.m_u16AttributeId == us_pib_attrib)){
     pmacSetConfirm->m_u16AttributeId = pmacGetConfirm.m_u16AttributeId;
     if (memcmp(value,ptr,pmacGetConfirm.m_u8AttributeLength) != 0){
        pmacSetConfirm->m_u8Status = -1;
     }
  }
}

/**
 * \brief Management Plane Get List Enhanced Request Synchronous
 *
 * \param us_pib_attrib  PRIME Attribute to get list enhanced
 * \param uc_timeout     Timeout for the request
 * \param pmacGetConfirm PRIME MAC Get Confirm Pointer
 *
 */
void prime_mngp_list_get_request_sync(uint16_t us_pib_attrib, uint8_t uc_timeout, struct TmacGetConfirm *pmacGetConfirm)
{
  PRIME_LOG(LOG_DBG,"prime_mngp_list_get_request_sync attr = 0x%04X\r\n", us_pib_attrib);

  uint16_t us_index = 0;

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  g_prime_sync_mgmt.m_u16AttributeId = us_pib_attrib; /* Used in order to know AttributeId requested on GetConfirm callback */

  // Send the asynchronous call
  mngLay_NewMsg(MNGP_PRIME_EN_PIBQRY);
  mngLay_AddGetPibListEnQuery(us_pib_attrib,MNGP_PRIME_ENHANCED_LIST_MAX_RECORDS,(uint8_t*)&us_index);
  mngLay_SendMsg();

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (g_prime_sync_mgmt.f_sync_res){
    // Confirm received
    pmacGetConfirm->m_u8Status = 0;  // No Status on Response Callback
    pmacGetConfirm->m_u16AttributeId = g_prime_sync_mgmt.s_macGetConfirm.m_u16AttributeId;
    pmacGetConfirm->m_u8AttributeLength = g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength;
    memcpy(pmacGetConfirm->m_au8AttributeValue, &g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue,g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength);
  }
  else{
    // Confirm not received
    pmacGetConfirm->m_u8Status = -1;
    PRIME_LOG(LOG_ERR,"ERROR: Confirm for 0x%04X not received\r\n",us_pib_attrib);
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DBG,"prime_mngp_get_request_sync result = %d\r\n",pmacGetConfirm->m_u8Status);
}

/**
 * \brief Serial Profile Management Reset All Stats Request
 */
void prime_mngp_reset_stats_request()
{
  // Send the asynchronous request
  mngLay_NewMsg(MNGP_PRIME_RESET);
  mngLay_SendMsg();
}

/**
 * \brief Serial Profile Management Reset MAC Stats Request Asynchronous
 *
 */
void prime_mngp_reset_mac_stats_request()
{
  // Send the asynchronous request
  mngLay_NewMsg(MNGP_PRIME_RESET);
  mngLay_AddResetStats(PIB_MAC_TX_DATAPKT_COUNT, 0);
  mngLay_AddResetStats(PIB_MAC_RX_DATAPKT_COUNT, 0);
  mngLay_AddResetStats(PIB_MAC_TX_CTRLPKT_COUNT, 0);
  mngLay_AddResetStats(PIB_MAC_RX_CTRLPKT_COUNT, 0);
  mngLay_AddResetStats(PIB_MAC_CSMA_FAIL_COUNT, 0);
  mngLay_AddResetStats(PIB_MAC_CSMA_CH_BUSY_COUNT, 0);
  mngLay_SendMsg();
}

/**
 * \brief Serial Profile Management Reset PHY Stats Request Asynchronous
 */
void prime_mngp_reset_phy_stats_request()
{
  // Send the asynchronous request
  mngLay_NewMsg(MNGP_PRIME_RESET);
  mngLay_AddResetStats(PIB_PHY_STATS_CRC_INCORRECT, 0);
  mngLay_AddResetStats(PIB_PHY_STATS_CRC_FAIL_COUNT, 0);
  mngLay_AddResetStats(PIB_PHY_STATS_TX_DROP_COUNT, 0);
  mngLay_AddResetStats(PIB_PHY_STATS_RX_DROP_COUNT, 0);
  mngLay_AddResetStats(PIB_PHY_STATS_RX_TOTAL_COUNT, 0);
  mngLay_AddResetStats(PIB_PHY_STATS_BLK_AVG_EVM, 0);
  mngLay_SendMsg();
}

/**
 * \brief Serial Profile Management Reboot Request
 */
void prime_mngp_reboot_request()
{
  // Send the asynchronous request
  mngLay_NewMsg(MNGP_PRIME_REBOOT);
  mngLay_SendMsg();
}

/**
 * \brief   Base Management PRIME Profile ACK indication response callback.
 *          Response to a PPROF GET directive.
 *
 * \param   uc_cmd: Command
 * \param   x_ack_code: ACK code
 */
void prime_bmng_ack_ind_cb(uint8_t uc_cmd, pprof_ack_code_t x_ack_code)
{
  PRIME_LOG(LOG_DEBUG,"bmng_ack_ind_cb uc_cmd=0x%02X x_ack_code=%d\r\n", uc_cmd, x_ack_code);
  memset(&g_prime_sync_mgmt.s_macSetConfirm,0,sizeof(struct TmacSetConfirm));
  if (g_prime_sync_mgmt.f_sync_req && (g_prime_sync_mgmt.m_u16AttributeId == uc_cmd)){
     if ((uc_cmd == prime_bmng_pprof_set_request_cmd) || (uc_cmd == prime_bmng_pprof_reboot_request_cmd)){
        /* Only for Asyncronous Set && Reboot - For Get // Zero Cross Request syncronism with response_cb */
        g_prime_sync_mgmt.s_macSetConfirm.m_u8Status = x_ack_code;
        g_prime_sync_mgmt.s_macSetConfirm.m_u16AttributeId = uc_cmd;
        g_prime_sync_mgmt.f_sync_res = true;
     }
  }
}

/**
 * \brief   Base Management PRIME Profile get pib response callback.
 *          Response to a PPROF_GET_REQUEST directive.
 *
 * \param   puc_eui48: Command
 * \param   us_data_len: Data Response Length
 * \param   puc_data: Data Response
 */
void prime_bmng_pprof_get_response_cb(uint8_t *puc_eui48, uint16_t us_data_len, uint8_t *puc_data)
{
  PRIME_LOG(LOG_DEBUG,"prime_bmng_pprof_get_response_cb eui48=%s\r\n", eui48_to_str(puc_eui48,NULL));
  prime_common_mng_rsp_cb(MNG_PLANE_BASE, puc_eui48, puc_data,us_data_len);
}

/**
 * \brief   Base Management PRIME Profile Get Enhanced Response
 *
 * \param   puc_eui48: Command
 * \param   us_data_length:  Data Response Length
 * \param   puc_data: Data Response
 */
void prime_bmng_pprof_get_enhanced_response_cb(uint8_t *puc_eui48, uint16_t us_data_len, uint8_t *puc_data)
{
  PRIME_LOG(LOG_DEBUG,"prime_bmng_pprof_get_enhanced_response_cb eui48=%s\r\n",eui48_to_str(puc_eui48,NULL));
  //LOG_DBG(LogBuffer(puc_data, us_data_len, "%X"));
  prime_common_mng_rsp_cb(MNG_PLANE_BASE, puc_eui48, puc_data,us_data_len);
}

/**
 * \brief   Base Management Whitelist Acknowledgement
 *
 * \param   uc_cmd: Command being acknowledged
 * \param   x_ack_code:  ACK returned code
 */
void prime_bmng_whitelist_ack_cb(uint8_t uc_cmd, whitelist_ack_code_t x_ack_code)
{
  PRIME_LOG(LOG_DEBUG,"bmng_whitelist_ack_cb uc_cmd=0x%02X x_ack_code=%d\r\n", uc_cmd, x_ack_code);
}

/**
 * \brief   Base Management Zero Cross Request
 *          First received ACK_IND_CB, next ZC_DIFF_RESPONSE_CB and finally ZEROCROSS_RESPONSE_CB
 * \param   puc_eui48      EUI48 to be rebooted
 * \param   uc_timeout     Request Timeout
 * \param   pmacSetConfirm SetConfirm response structure
 */
void prime_bmng_zero_cross_request_sync(uint8_t * puc_eui48, uint8_t uc_timeout, struct TmacSetConfirm *pmacSetConfirm)
{

  PRIME_LOG(LOG_DEBUG,"prime_bmng_zero_cross_request_sync eui48=%s\r\n", eui48_to_str(puc_eui48,NULL));

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     bmng_pprof_reboot is asyncronous - Base Node will callback set request
     with prime_bmng_ack_ind_cb cmd=bmng_pprof_reboot_request_cmd
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_pprof_zc_diff_request_cmd;
  /* Default Status */
  pmacSetConfirm->m_u8Status = -1;

  // Send the asynchronous request
  bmng_pprof_zero_cross_request(puc_eui48);

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (!g_prime_sync_mgmt.f_sync_res){
    PRIME_LOG(LOG_ERR,"ERROR: prime_bmng_ack_ind_cb for bmng_pprof_zc_request_cmd not received\r\n");
  }else if (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != PPROF_ACK_OK){
    // Error sending reboot request
    pmacSetConfirm->m_u8Status = g_prime_sync_mgmt.s_macSetConfirm.m_u8Status;
    PRIME_LOG(LOG_ERR,"ERROR: bmng_pprof_zc_request_cmd failed\r\n");
  }else{
    pmacSetConfirm->m_u8Status = PPROF_ACK_OK;
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"prime_bmng_zero_cross_request_sync result=%d\r\n",pmacSetConfirm->m_u8Status);

  return ;
}

/**
 * \brief   Base Management PRIME Profile Zero Cross Request Response
 *          Last callback received when ZC_DIFF_REQUEST
 *
 * \param   puc_eui48 EUI48 MAC Address
 * \param   uc_id     Identifier
 * \param   ui_zct    Zero Cross Time Reference from PL360 (tens of us)
 */
void prime_bmng_pprof_zerocross_response_cb(uint8_t *puc_eui48, uint8_t uc_zc_status, uint32_t ui_zct)
{
  prime_sn *sn;
  PRIME_LOG(LOG_DEBUG,"bmng_pprof_zerocross_response_cb eui48=%s, uc_id=0x%02X ui_zct=%d\r\n", eui48_to_str(puc_eui48,NULL), uc_zc_status, ui_zct);
  if (g_prime_sync_mgmt.f_sync_req && (g_prime_sync_mgmt.m_u16AttributeId == prime_bmng_pprof_zc_diff_request_cmd)){
      g_prime_sync_mgmt.s_macSetConfirm.m_u8Status = PPROF_ACK_OK;
      g_prime_sync_mgmt.s_macSetConfirm.m_u16AttributeId = prime_bmng_pprof_zc_diff_request_cmd;
      g_prime_sync_mgmt.f_sync_res = true;

      sn = prime_network_find_sn(&prime_network,puc_eui48);
      if (sn != NULL){
         /* Only for Registered Nodes */
         prime_network_mutex_lock();
         sn->uc_zc_status  = uc_zc_status;
         sn->ui_zct = ui_zct;   /* Time reference read from PL360 - tens of microseconds */
         prime_network_mutex_unlock();
      }
  }
}

/**
 * \brief   Base Management PRIME Profile Get Zero Crossing Response
 * \param   puc_eui48    MAC address of the node
 * \param   uc_time_freq Time Frequency
 * \param   ul_time_diff Zero crossing difference
 */
void prime_bmng_pprof_zc_diff_response_cb(uint8_t *puc_eui48, uint32_t uc_time_freq, uint32_t ul_time_diff)
{
  prime_sn *sn;
  PRIME_LOG(LOG_DEBUG,"bmng_pprof_zc_diff_response_cb eui48=%s uc_time_freq=%d ul_time_diff=%d\r\n", eui48_to_str(puc_eui48,NULL), uc_time_freq, ul_time_diff);
  sn = prime_network_find_sn(&prime_network,puc_eui48);
  if (sn != NULL){
     /* Only for Registered Nodes */
     prime_network_mutex_lock();
     /* Network Frequency - Tens of microsecond period ? Convert to Hz => (100000 / uc_time_freq) (Typically 50/60 Hz)*/
     sn->uc_time_freq  = uc_time_freq;
     sn->ul_time_diff  = ul_time_diff;
     prime_network_mutex_unlock();
  }
}

/**
 * \brief Base Management Plane Get Request Synchronous
 * \param puc_eui48      EUI48
 * \param us_pib_attrib  PRIME Attribute to get
 * \param index          Attribute Index
 * \param uc_timeout     Timeout for the request
 * \param pmacGetConfirm PRIME MAC Get Confirm Pointer
 *
 */
void prime_bmng_pprof_get_request_sync(uint8_t * puc_eui48, uint16_t us_pib_attrib, uint16_t index, uint8_t uc_timeout, struct TmacGetConfirm *pmacGetConfirm)
{
  PRIME_LOG(LOG_DEBUG,"bmng_pprof_get_request_sync eui48=%s attr = 0x%04X index=%d\r\n", eui48_to_str(puc_eui48,NULL), us_pib_attrib, index);

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  g_prime_sync_mgmt.m_u16AttributeId = us_pib_attrib; /* Used in order to know AttributeId requested on GetConfirm callback */

  // Send the asynchronous request
  bmng_pprof_get_request(puc_eui48, us_pib_attrib, index);
  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (g_prime_sync_mgmt.f_sync_res){
    // Confirm received
    pmacGetConfirm->m_u8Status = 0;  // No Status on Response Callback
    pmacGetConfirm->m_u16AttributeId = g_prime_sync_mgmt.s_macGetConfirm.m_u16AttributeId;
    pmacGetConfirm->m_u8AttributeLength = g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength;
    memcpy(pmacGetConfirm->m_au8AttributeValue, &g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue,g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength);
  }
  else{
    // Confirm not received
    pmacGetConfirm->m_u8Status = -1;
    PRIME_LOG(LOG_ERR,"ERROR: Confirm for 0x%04X not received\r\n",us_pib_attrib);
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"bmng_pprof_get_request_sync result = %d\r\n",pmacGetConfirm->m_u8Status);
}

/**
 * \brief Base Management Plane Get List Request Synchronous
 * \param puc_eui48      EUI48
 * \param us_pib_attrib  PRIME Attribute to get
 * \param index          Attribute Index
 * \param uc_timeout     Timeout for the request
 * \param pmacGetConfirm PRIME MAC Get Confirm Pointer
 *
 */
void prime_bmng_pprof_get_list_request_sync(uint8_t * puc_eui48, uint16_t us_pib_attrib, uint16_t index, uint8_t uc_timeout, struct TmacGetConfirm *pmacGetConfirm)
{
  PRIME_LOG(LOG_DEBUG,"prime_bmng_pprof_get_list_request_sync eui48=%s attr = 0x%04X index=%d\r\n", eui48_to_str(puc_eui48,NULL), us_pib_attrib, index);

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  g_prime_sync_mgmt.m_u16AttributeId = us_pib_attrib; /* Used in order to know AttributeId requested on GetConfirm callback */

  // Send the asynchronous request -
  // PRIME 1.4 uses Enhanced request for Lists - Fails if simple request is used
  bmng_pprof_get_enhanced(puc_eui48, us_pib_attrib, MNGP_PRIME_ENHANCED_LIST_MAX_RECORDS, index);
  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (g_prime_sync_mgmt.f_sync_res){
    // Confirm received
    pmacGetConfirm->m_u8Status = 0;  // No Status on Response Callback
    pmacGetConfirm->m_u16AttributeId = g_prime_sync_mgmt.s_macGetConfirm.m_u16AttributeId;
    pmacGetConfirm->m_u8AttributeLength = g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength;
    memcpy(pmacGetConfirm->m_au8AttributeValue, &g_prime_sync_mgmt.s_macGetConfirm.m_au8AttributeValue,g_prime_sync_mgmt.s_macGetConfirm.m_u8AttributeLength);
  }
  else{
    // Confirm not received
    pmacGetConfirm->m_u8Status = -1;
    PRIME_LOG(LOG_ERR,"ERROR: Confirm for 0x%04X not received\r\n",us_pib_attrib);
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"prime_bmng_pprof_get_list_request_sync result = %d\r\n",pmacGetConfirm->m_u8Status);
}

/**
 * \brief Base Management Plane Set Request Synchronous
 *
 * \param puc_eui48      EUI48
 * \param us_pib_attrib  PRIME Attribute to Set
 * \param value          PRIME Attribute Value
 * \param length         PRIME Attribute Length
 * \param uc_timeout     Timeout for the request
 * \param pmacSetConfirm pointer to TmacSetConfirm structure
 */
void prime_bmng_pprof_set_request_sync(uint8_t * puc_eui48, uint16_t us_pib_attrib, uint8_t* value, uint8_t length, uint8_t uc_timeout, struct TmacSetConfirm *pmacSetConfirm)
{
  struct TmacGetConfirm pmacGetConfirm;
  uint8_t *ptr = &pmacGetConfirm.m_au8AttributeValue[0];

  PRIME_LOG(LOG_DEBUG,"bmng_pprof_set_request_sync eui48=%s attr=0x%04X\r\n", eui48_to_str(puc_eui48,NULL), us_pib_attrib);

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     bmng_pprof_set_request is asyncronous - Base Node will callback set request
     with prime_bmng_ack_ind_cb cmd=prime_bmng_pprof_set_request_cmd
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_pprof_set_request_cmd;
  bmng_pprof_set_request(puc_eui48, us_pib_attrib, value, length);
  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if ((!g_prime_sync_mgmt.f_sync_res) || (g_prime_sync_mgmt.f_sync_res & (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != PPROF_ACK_OK))){
    // ACK Indication Confirm not received or Error Sending
    pmacSetConfirm->m_u8Status = -1;
    g_prime_sync_mgmt.f_sync_req = false;
    g_prime_sync_mgmt.f_sync_res = false;
    PRIME_LOG(LOG_ERR,"ERROR: prime_bmng_ack_ind_cb for 0x%04X not received\r\n",us_pib_attrib);
    return ;
  }

  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  g_prime_sync_mgmt.m_u16AttributeId = us_pib_attrib;
  prime_bmng_pprof_get_request_sync(puc_eui48, us_pib_attrib, 0, PRIME_SYNC_TIMEOUT_GET_REQUEST, &pmacGetConfirm);
  pmacSetConfirm->m_u8Status = pmacGetConfirm.m_u8Status;  // No Status on Response Callback
  if ((pmacSetConfirm->m_u8Status == 0) && (pmacGetConfirm.m_u16AttributeId == us_pib_attrib)){
     pmacSetConfirm->m_u16AttributeId = pmacGetConfirm.m_u16AttributeId;
     if (memcmp(value,ptr,pmacGetConfirm.m_u8AttributeLength) != 0){
        pmacSetConfirm->m_u8Status = -1;
     }
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"bmng_pprof_set_request_sync result = %d\r\n",pmacGetConfirm.m_u8Status);
}

/**
 * \brief   Serial Profile Management Reboot Request
 * \param   puc_eui48      EUI48 to be rebooted
 * \param   uc_timeout     Timeout for the request
 * \param   pmacSetConfirm Set Confirm pointer
 */
void prime_bmng_reboot_request_sync(uint8_t * puc_eui48, uint8_t uc_timeout, struct TmacSetConfirm *pmacSetConfirm)
{
  PRIME_LOG(LOG_DEBUG,"bmng_pprof_set_request_sync eui48=%s\r\n", eui48_to_str(puc_eui48,NULL));

  // Set the sync flags to intercept the callback
  g_prime_sync_mgmt.f_sync_req = true;
  g_prime_sync_mgmt.f_sync_res = false;
  /*
     bmng_pprof_reboot is asyncronous - Base Node will callback set request
     with prime_bmng_ack_ind_cb cmd=bmng_pprof_reboot_request_cmd
  */
  g_prime_sync_mgmt.m_u16AttributeId = prime_bmng_pprof_reboot_request_cmd;
  /* Default Status */
  pmacSetConfirm->m_u8Status = -1;

  // Send the asynchronous request
  bmng_pprof_reboot(puc_eui48);

  // Wait processing until flag activates, or timeout
  addUsi_WaitProcessing(uc_timeout, (Bool *)&g_prime_sync_mgmt.f_sync_res);

  if (!g_prime_sync_mgmt.f_sync_res){
    PRIME_LOG(LOG_ERR,"ERROR: prime_bmng_ack_ind_cb for bmng_pprof_reboot_request_cmd not received\r\n");
  }else if (g_prime_sync_mgmt.s_macSetConfirm.m_u8Status != PPROF_ACK_OK){
    // Error sending reboot request
    pmacSetConfirm->m_u8Status = g_prime_sync_mgmt.s_macSetConfirm.m_u8Status;
    PRIME_LOG(LOG_ERR,"ERROR: bmng_pprof_reboot_request_cmd failed\r\n");
  }else{
    pmacSetConfirm->m_u8Status = PPROF_ACK_OK;
  }

  g_prime_sync_mgmt.f_sync_req = false;
  g_prime_sync_mgmt.f_sync_res = false;

  PRIME_LOG(LOG_DEBUG,"bmng_pprof_set_request_sync result=%d\r\n",pmacSetConfirm->m_u8Status);

  return ;
}

/**
 * \brief Base Node Manager management initialization
 */
int base_node_manager_mng_init()
{
    /* MAC Primitives */
    prime_cl_null_callbacks_t mac_cbs;
    // Initialization
    memset(&mac_cbs, 0, sizeof(prime_cl_null_callbacks_t));
    mac_cbs.prime_cl_null_establish_ind_cb       = (void *) _prime_cl_null_establish_ind_cb;
    mac_cbs.prime_cl_null_establish_cfm_cb       = (void *) _prime_cl_null_establish_cfm_cb;
    mac_cbs.prime_cl_null_release_ind_cb         = (void *) _prime_cl_null_release_ind_cb;
    mac_cbs.prime_cl_null_release_cfm_cb         = (void *) _prime_cl_null_release_cfm_cb;
    mac_cbs.prime_cl_null_join_ind_cb            = (void *) _prime_cl_null_join_ind_cb;
    mac_cbs.prime_cl_null_join_cfm_cb            = (void *) _prime_cl_null_join_cfm_cb;
    mac_cbs.prime_cl_null_leave_ind_cb           = (void *) _prime_cl_null_leave_ind_cb;
    mac_cbs.prime_cl_null_leave_cfm_cb           = (void *) _prime_cl_null_leave_cfm_cb;
    mac_cbs.prime_cl_null_data_cfm_cb            = (void *) _prime_cl_null_data_cfm_cb;
    mac_cbs.prime_cl_null_data_ind_cb            = (void *) _prime_cl_null_data_ind_cb;
    mac_cbs.prime_cl_null_plme_reset_cfm_cb      = (void *) _prime_cl_null_plme_reset_cfm_cb;
    mac_cbs.prime_cl_null_plme_sleep_cfm_cb      = (void *) _prime_cl_null_plme_sleep_cfm_cb;
    mac_cbs.prime_cl_null_plme_resume_cfm_cb     = (void *) _prime_cl_null_plme_resume_cfm_cb;
    mac_cbs.prime_cl_null_plme_testmode_cfm_cb   = (void *) _prime_cl_null_plme_testmode_cfm_cb;
    mac_cbs.prime_cl_null_plme_get_cfm_cb        = (void *) _prime_cl_null_plme_get_cfm_cb;
    mac_cbs.prime_cl_null_plme_set_cfm_cb        = (void *) _prime_cl_null_plme_set_cfm_cb;
    //mac_cbs.prime_cl_null_mlme_register_cfm_cb   = (void *) _prime_cl_null_mlme_register_cfm_cb;     /* This function is not applicable in a Base Node */
    //mac_cbs.prime_cl_null_mlme_register_ind_cb   = (void *)_prime_cl_null_mlme_register_ind_cb;     /* This function is not applicable in a Base Node */
    //mac_cbs.prime_cl_null_mlme_unregister_cfm_cb = (void *) _prime_cl_null_mlme_unregister_cfm_cb;   /* This function is not applicable in a Base Node */
    //mac_cbs.prime_cl_null_mlme_unregister_ind_cb = (void *) _prime_cl_null_mlme_unregister_ind_cb;   /* This function is not applicable in a Base Node */
    mac_cbs.prime_cl_null_mlme_promote_cfm_cb    = (void *) _prime_cl_null_mlme_promote_cfm_cb;
    //mac_cbs.prime_cl_null_mlme_promote_ind_cb    = (void *) _prime_cl_null_mlme_promote_ind_cb;      /* This function is not applicable in a Base Node */
    //mac_cbs.prime_cl_null_mlme_demote_cfm_cb     = (void *) _prime_cl_null_mlme_demote_cfm_cb;       /* This function is not applicable in a Base Node */
    //mac_cbs.prime_cl_null_mlme_demote_ind_cb     = (void *) _prime_cl_null_mlme_demote_ind_cb;       /* This function is not applicable in a Base Node */
    mac_cbs.prime_cl_null_mlme_reset_cfm_cb      = (void *) _prime_cl_null_mlme_reset_cfm_cb;
    mac_cbs.prime_cl_null_mlme_get_cfm_cb        = (void *) _prime_cl_null_mlme_get_cfm_cb;
    mac_cbs.prime_cl_null_mlme_list_get_cfm_cb   = (void *) _prime_cl_null_mlme_list_get_cfm_cb;
    mac_cbs.prime_cl_null_mlme_set_cfm_cb        = (void *)_prime_cl_null_mlme_set_cfm_cb;
    // Set Callbacks
    prime_cl_null_set_callbacks(&mac_cbs);

	  /* IEC 61334-4-32 Primitives Callbacks */
	  prime_cl_432_callbacks_t cl_432_cbs;
    memset(&cl_432_cbs, 0, sizeof(prime_cl_432_callbacks_t));
	  //cl_432_cbs.prime_cl_432_establish_cfm_cb     = NULL ;
	  //cl_432_cbs.prime_cl_432_release_cfm_cb       = NULL ;
    cl_432_cbs.prime_cl_432_dl_data_cfm_cb       = (void *) _dlmsotcp_cl_432_dl_data_cfm_cb;
    cl_432_cbs.prime_cl_432_dl_data_ind_cb       = (void *) _dlmsotcp_cl_432_dl_data_ind_cb;
    cl_432_cbs.prime_cl_432_dl_leave_ind_cb      = (void *) _dlmsotcp_cl_432_leave_ind_cb;
    cl_432_cbs.prime_cl_432_dl_join_ind_cb       = (void *) _dlmsotcp_cl_432_join_ind_cb;
    prime_cl_432_set_callbacks(&cl_432_cbs);

    /* PRIME Management Callback */
	  mngp_set_rsp_cb(prime_mngp_rsp_cb);

    /* Base Management/FUP Primitives callbacks */
    prime_bmng_callbacks_t st_bmng_cbs;
    // Initialization
    memset(&st_bmng_cbs, 0, sizeof(prime_bmng_callbacks_t));
    st_bmng_cbs.fup_ack_ind_cb                  = (void *) prime_bmng_fup_ack_ind_msg_cb;
    st_bmng_cbs.fup_error_ind_cb                = (void *) prime_bmng_fup_error_ind_msg_cb;
    st_bmng_cbs.fup_kill_ind_cb                 = (void *) prime_bmng_fup_kill_ind_msg_cb;
    st_bmng_cbs.fup_status_ind_cb               = (void *) prime_bmng_fup_status_ind_msg_cb;
    st_bmng_cbs.fup_version_ind_cb              = (void *) prime_bmng_fup_version_ind_msg_cb;
    st_bmng_cbs.network_event_ind_cb            = (void *) prime_bmng_network_event_msg_cb;
    st_bmng_cbs.pprof_ack_ind_cb                = (void *) prime_bmng_ack_ind_cb ;
    st_bmng_cbs.pprof_get_response_cb           = (void *) prime_bmng_pprof_get_response_cb;
    st_bmng_cbs.pprof_zerocross_response_cb     = (void *) prime_bmng_pprof_zerocross_response_cb;
    st_bmng_cbs.pprof_get_enhanced_response_cb  = (void *) prime_bmng_pprof_get_enhanced_response_cb;
    st_bmng_cbs.whitelist_ack_cb                = (void *) prime_bmng_whitelist_ack_cb;
    st_bmng_cbs.pprof_zc_diff_response_cb       = (void *) prime_bmng_pprof_zc_diff_response_cb;
    bmng_set_callbacks(&st_bmng_cbs);

    // Initialize PRIME Configuration Parameters
    prime_base_node_init_params();
    // Initialize PRIME Network Structure - LIST - Get Information about the PRIME Network
    prime_network_init();

	  return SUCCESS;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
