/**
 * \file
 *
 * \brief Base Node Network.
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

 #ifndef _BASE_NODE_NETWORK_H_
 #define _BASE_NODE_NETWORK_H_

 /* / @cond 0 */
 /**INDENT-OFF**/
 #ifdef __cplusplus
 extern "C" {
 #endif
 /**INDENT-ON**/
 /* / @endcond */

 #include "mchp_list.h"

/* MAC Security Profile */
typedef enum {
    SECURITY_PROFILE_0       = 0,    /* No Security - No encyption */
    SECURITY_PROFILE_1       = 1,
    SECURITY_PROFILE_2       = 2,
    SECURITY_PROFILE_3       = 3,
    SECURITY_PROFILE_UNKNOWN = 8,    /* Unknown Security Profile - Seen from the Network */
} security_profile_t;

/* 4-32 Connection State */
typedef enum {
    CL432_CONN_STATE_CLOSED        = 0,
    CL432_CONN_STATE_CONNECTING    = 1,
    CL432_CONN_STATE_DISCONNECTING = 2,
    CL432_CONN_STATE_OPEN          = 3
} cl432_conn_state_t;

#define CL_432_INVALID_ADDRESS       (0xFFFF)

 /* MAC Connection Information */
struct __attribute__((__packed__)) mac_conn_list_t{     // List of MAC Connections
    mchp_list list;               // LIST
    uint16_t   connEntryLCID;
    uint8_t    connType;
};
typedef struct mac_conn_list_t mac_conn;

/* 4-32 Connection Information */
struct __attribute__((__packed__)) cl432_conn_t{     // List of CL-4-32 Connections
   uint8_t     connState;
   uint16_t    connAddress;
   uint8_t     connSerialNumber[16];
   uint8_t     connLenSerial;
   uint8_t     connMAC[6];
};
typedef struct cl432_conn_t cl432_conn;

/* PRIME Service Node Information Structure  */
struct __attribute__((__packed__)) prime_sn_t{     // List of registered devices
   mchp_list list;                // List
   // Administrative Parameters
   uint8_t     admin_en;          // Administrative Enable flag
   uint8_t     fwup_en;           // Firmware Upgrade Enable flag
   // Security Settings
   uint8_t     security_profile;  // Security Profile
   uint8_t     duk[16];           // DUK
   // From Register List
   uint8_t     registered;        // Registered Flag
   uint8_t     state;             // Service Node State = Terminal/Switch/...
   uint8_t     regEntryID[6];     // EUI48
   uint8_t     regEntryState;     // Functional state of this Node
   uint8_t     regEntryLSID;      // SID Allocated to this Nodes
   uint8_t     regEntrySID;       // SID of Switch through which this Node is connected
   uint8_t     regEntryLevel;     // Hierarchy level of this Node
   uint8_t     regEntrySwCap;     // Bitmap of MAC Switching Capabilities
   uint8_t     regEntryTCap;      // Bitmap of MAC capabilities
   uint16_t    regEntryLNID;      // LNID allocated to this Node
   time_t      regEntryTimeStamp; // Registering Time Stamp
   uint8_t     alvRxcnt;          // Alive Received Counter
   uint8_t     alvTxcnt;          // Alive Transmitted Counter
   uint8_t     alvTime;           // The TIME value used for the Keep Alive process
   uint8_t     regEntryRxLvl;     // Received RX Level from this node (Switch)
   uint8_t     regEntryRxSNR;     // Received RX SNR from this node   (Switch)
   // Firmware Upgrade Information
   uint8_t     fu_state;          // Firmware Upgrade State
   uint32_t    fu_pages;          // Firmware Upgrade Pages
   uint8_t     fu_vendor[32];     // Vendor
   uint8_t     fu_vendor_len;     // Vendor Length
   uint8_t     fu_model[32];      // Model
   uint8_t     fu_model_len;      // Model Length
   uint8_t     fu_version[32];    // Version
   uint8_t     fu_version_len;    // Version Length
   // Zero Cross information
   uint8_t     uc_zc_status;      // Zero Crossing Status
   uint32_t    ui_zct;            // Time Reference (coming from PL360)
   uint32_t    uc_time_freq;      // Network Period in tens of us (Typically 50Hz (2000)/60Hz (1667))
   uint32_t    ul_time_diff;      // Difference between time references in tens of us
   // From Conn;ection List
   mchp_list   macConnList;       // MAC Connections List
   uint16_t    macConns;          // Number of MAC connections
   // From 4-32 Connection List
   cl432_conn  cl432Conn;         // 4-32 Connection Information
   uint8_t     autoclose_enabled; // 4-32 Autoclose Enabled
 };
typedef struct prime_sn_t prime_sn;

/*
 * \brief PRIME Network Mutex Lock
 * \return 0
 *
 */
 int prime_network_mutex_lock();

/*
 * \brief PRIME Network Mutex Lock
 * \return 0
 */
  int prime_network_mutex_unlock();

/*
 * \brief Create a new PRIME SN
 * \param p_prime_network        -> PRIME SN List
 * \param eui48                  -> Pointer to SN MAC address
 * \param sec_profile            -> Security Profile
 * \param duk                    -> Poiner to Security Key -> DUK
 * \param
 *
 * \return Pointer to the new structure
 */
 prime_sn * prime_network_add_sn(mchp_list *p_prime_network, uint8_t *eui48, uint8_t admin_en, uint8_t sec_profile, uint8_t *duk);

 /*
  * \brief Delete a Device from the List
  * \param p_prime_sn_device : pointer to PRIME SN Device
  *
  * \return -
  */
 void prime_network_del_sn(prime_sn *p_prime_sn);

 /*
  * \brief Look for a PRIME SN on the list
  * \param p_prime_network  PRIME SN List
  * \param ha               PRIME SN EUI48 Address
  *
  * \return Pointer to the PRIME SN Structure or NULL if doesn't exist
  */
prime_sn * prime_network_find_sn(mchp_list * p_prime_network, const unsigned char * regEntryID);

/*
* \brief Look for a PRIME SN on the list for LSID
* \param prime_sn_list      PRIME SN List
* \param prime_lsid          PRIME LSID
*
* \return Pointer to the PRIME SN Structure or NULL if doesn't exist
*/
prime_sn * prime_network_find_sn_lsid(mchp_list * p_prime_network, uint8_t lsid);

/*
* \brief Look for a PRIME SN on the list for CL4-32 address
* \param prime_sn_list       PRIME SN List
* \param prime_lsid          CL4-32 Address
*
* \return Pointer to the PRIME SN Structure or NULL if doesn't exist
*/
prime_sn * prime_network_find_sn_cl432_address(mchp_list * p_prime_network, uint16_t cl432_address);

/*
 * \brief Print List of PRIME SN
 * \param p_prime_network => PRIME SN linked list Pointer
 *
 * \return -
 */
void prime_network_print_sn_list(mchp_list *p_prime_network);

/*
 * \brief Shows information about PRIME SN
 * \param p_prime_sn -> PRIME SN Device Pointer
 *
 * \return -
 */
int prime_network_print_sn(prime_sn *p_prime_sn);

/*
 * \brief Add a new MAC Connection
 * \param sn            -> PRIME Service Node Pointer structure
 * \param lcid          -> LCID
 * \param connType      -> Connection Type
 * \param

 * \return Pointer to the new structure
 */
mac_conn* prime_sn_add_mac_connection(prime_sn *sn, uint16_t lcid, uint8_t connType);

/*
 * \brief Look for a PRIME SN on the list
 * \param sn                 PRIME SN Pointer Structure
 * \param lcid               PRIME SN MAC Connecion LCID
 *
 * \return Pointer to the PRIME SN MAC Connection Structure or NULL if doesn't exist
 */
mac_conn * prime_sn_find_mac_connection(prime_sn *sn, uint16_t lcid);

/*
 * \brief Print MAC Connections for a PRIME SN
 * \param mc                 MAC Connection Pointer Structure
 *
 * \return -
 */
int prime_sn_print_mac_connection(mac_conn *mc);

/*
 * \brief Delete MAC Connections
 * \param sn        -> PRIME Service Node Pointer structure
 * \param
 * \return Pointer to the new structure
 */
int prime_sn_del_mac_connections(prime_sn *sn);

/*
 * \brief Get PRIME Network Maximum Level
 *
 * \return 0
 */
uint8_t prime_network_get_max_level();

/*
 * \brief Set PRIME Network Maximum Level
 *
 * \return 0
 */
uint8_t prime_network_set_max_level(uint8_t level);

/*
 * \brief Init PRIME Network
 * \param
 *
 * \return -
 */
int prime_network_init();

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */

#endif
