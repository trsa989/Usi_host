/**
 * \file
 *
 * \brief Base Node Management Header.
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

#ifndef _BASE_NODE_MNG_H_
#define _BASE_NODE_MNG_H_

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

#include <stdbool.h>

struct __attribute__((__packed__)) TprimeSyncMgmt{
    bool f_sync_res;                          // Flag to indicate syncronous response
    bool f_sync_req;                          // Flag to indicate syncronous request
    uint32_t m_u16AttributeId;                // In order to handle correctly syncronous requests
    struct TmacGetConfirm s_macGetConfirm;    // Get Confirm callback struct
    struct TmacSetConfirm s_macSetConfirm;    // Set Confirm callback struct
};
typedef struct TprimeSyncMgmt T_prime_sync_mgmt;

// List of registered devices. This list is maintained by the Base Node only. Each entry in this list shall comprise the following information.
struct __attribute__((__packed__)) TmacListRegDevices{
  uint8_t  regEntryID[6];      // EUI48
  uint16_t regEntryLNID;       // LNID allocated to this Node
  uint8_t  regEntryState;      // Functional state of this Node
  uint8_t  regEntryLSID;       // SID Allocated to this Nodes
  uint8_t  regEntrySID;        // SID of Switch through which this Node is connected
  uint8_t  regEntryLevel;      // Hierarchy level of this Node
  uint8_t  regEntryTCap;       // Bitmap of MAC capabilities
  uint8_t  regEntrySwCap;      // Bitmap of MAC Switching Capabilities
};
typedef struct TmacListRegDevices macListRegDevices;

// List of active non-direct connections. This list is maintained by the Base Node only.
struct __attribute__((__packed__)) TmacListActiveConn{
  uint8_t  connEntryID[6];     // EUI48
  uint16_t connEntryLNID;      // LNID allocated to this Node
  uint16_t connEntryLCID;      // LCID allocated to this connection
  uint16_t connEntrySID;       // SID of Switch through which this Node is connected - uint16_t becasuse of Extended Table
  uint8_t  connType;           // Connection Type
};
typedef struct TmacListActiveConn macListActiveConn;

// List of entries in multicast switching table. This list is not maintained by Service Nodes in a Terminal functional state.
struct __attribute__((__packed__)) TmacListMcastEntries{
  uint16_t mcastEntryLCID;     // Multicast Group LCID
  uint16_t mcastEntryMembers;  // Number of Multicast Members
};
typedef struct TmacListMcastEntries macListMcastEntries;

// List the Switch table. This list is not maintained by Service Nodes in a Terminal functional state.
struct __attribute__((__packed__)) TmacListSwitchTable{
  uint16_t stblEntryLNID;      // LNID of attached Switch Node
  uint8_t  stblEntryLSID;      // LSID assigned to the attached Switch Node.
  uint8_t  stbleEntrySID;      // SID of attached Switch Node
  uint8_t  stblEntryALVTime;   // The TIME value used for the Keep Alive process
};
typedef struct TmacListSwitchTable macListSwitchTable;

// List of direct connections that are active. This list is maintained only in the Base Node.
struct __attribute__((__packed__)) TmacListDirectConn{
  uint8_t  dconnEntrySrcSID;   // SID of Switch through which the source Service Node is connected.
  uint16_t dconEntrySrcLNID;   // NID allocated to the source Service Node.
  uint16_t dconnEntrySrcLCID;  // LCID allocated to this connection at the source.
  uint8_t  dconnEntrySrcID[6]; // EUI-48 of source Service Node.
  uint8_t  dconnEntryDstSID;   // SID of Switch through which the destination Service Node is connected.
  uint16_t dconnEntryDstLNID;  // NID allocated to the destination Service Node.
  uint16_t dconnEntryDstLCID;  // LCID allocated to this connection at the destination.
  uint8_t  dconnEntryDstID[6]; // EUI-48 of destination Service Node.
  uint8_t  dconnEntryDSID;     // SID of Switch that is the direct Switch.
  uint8_t  dconnEntryDID[6];   // EUI-48 of direct switch.
};
typedef struct TmacListDirectConn macListDirectConn;

// List the direct Switch table
struct __attribute__((__packed__)) TmacListDirectTable{
  uint8_t  dconnEntrySrcSID;   // SID of Switch through which the source Service Node is connected.
  uint16_t dconEntrySrcLNID;   // NID allocated to the source Service Node.
  uint16_t dconnEntrySrcLCID;  // LCID allocated to this connection at the source.
  uint8_t  dconnEntryDstSID;   // SID of Switch through which the destination Service Node is connected.
  uint16_t dconnEntryDstLNID;  // NID allocated to the destination Service Node.
  uint16_t dconnEntryDstLCID;  // LCID allocated to this connection at the destination.
  uint8_t  dconnEntryDID[6];   // EUI-48 of direct switch.
};
typedef struct TmacListDirectTable macListDirectTable;

// List of Switch Nodes whose beacons are received.
struct __attribute__((__packed__)) TmacListAvailableSwitches{
  uint8_t  slistEntrySNA[6];   // EUI-48 of the Subnetwork
  uint8_t  slistEntryLSID;     // SID of this Switch
  uint8_t  slistEntryLevel;    // Level of this Switch in Subnetwork hierarchy.
  uint8_t  slistEntryRxLvl;    // Received signal level for this Switch.
  uint8_t  slistEntryRxSNR;    // Signal to Noise Ratio for this Switch.
};
typedef struct TmacListAvailableSwitches macListAvailableSwitches;

// List of PHY communication parameters. This table is maintained in every Node. For Terminal Nodes it contains only one entry for the Switch the Node is connected through. For other Nodes is contains also entries for every directly connected child Node.
struct __attribute__((__packed__)) TmacListPhyComm{
  uint16_t phyCommLNID;              // LNID of the peer device
  uint8_t  phyCommSID;               // SID of the peer device
  uint8_t  phyCommTxPwr;             // Tx power of GPDU packets send to the device.
  uint8_t  phyCommRxLvl;             // Rx power level of GPDU packets received from the device.
  uint8_t  phyCommSNR;               // SNR of GPDU packets received from the device.
  uint8_t  phyCommTxModulation;      // Modulation scheme to be used for communicating with this node.
  uint8_t  phyCommPhyTypeCapability; // Capability of the node to receive only PHY Type A or PHY Type A+B frames 0: Type A only node - 1: Type A+B capable node
  uint16_t phyCommRxAge;             // Time [seconds] since last update of phyCommTxModulation.
};
typedef struct TmacListPhyComm macListPhyComm;

// List of Firmware Upgrade Nodes
struct __attribute__((__packed__)) TmacListFU{
  uint8_t   fuMAC[6];         // EUI-48 MAC
  uint16_t  fuLNID;           // LNID Local Node Identifier
  uint32_t  fuPagesCompleted; // Pages Completed
  uint8_t   fuNodeState;      // Node State - x_fup_state_t
};
typedef struct TmacListFU macListFU;

// List of CL4-32 Nodes
struct __attribute__((__packed__)) Tcl432ListNode{
  uint16_t  cl432address;        // CL4-32 Address
  uint8_t   cl432serial[16];     // CL4-32 Serial
  uint8_t   cl432serial_len;     // CL4-32 Serial Lenght
  uint8_t   cl432mac[6];         // EUI-48 MAC
};
typedef struct Tcl432ListNode cl432ListNode;

/** RESULTS values for convergence layer primitives */
typedef enum {
	FLAG_PRIME_PLME_GET_REQUEST_SYNC      = 0x00000001,
	FLAG_PRIME_PLME_SET_REQUEST_SYNC      = 0x00000002,
  FLAG_PRIME_PLME_RESET_REQUEST_SYNC    = 0x00000004,
  FLAG_PRIME_PLME_RESUME_REQUEST_SYNC   = 0x00000008,
  FLAG_PRIME_PLME_SLEEP_REQUEST_SYNC    = 0x00000010,
  FLAG_PRIME_MLME_GET_REQUEST_SYNC      = 0x00000020,
	FLAG_PRIME_MLME_SET_REQUEST_SYNC      = 0x00000040,
  FLAG_PRIME_MLME_GET_LIST_REQUEST_SYNC = 0x00000080,
  FLAG_PRIME_MLME_PROMOTE_REQUEST_SYNC  = 0x00000100,
  FLAG_PRIME_MLME_DEMOTE_REQUEST_SYNC   = 0x00000200,
  FLAG_PRIME_MLME_RESET_REQUEST_SYNC    = 0x00000400,
  FLAG_PRIME_MNGP_GET_REQUEST_SYNC      = 0x00000800,
  FLAG_PRIME_MNGP_SET_REQUEST_SYNC      = 0x00001000,
  FLAG_PRIME_MNGP_GET_LIST_REQUEST_SYNC = 0x00002000,
	FLAG_PRIME_BMNG_GET_REQUEST_SYNC      = 0x00004000,
  FLAG_PRIME_BMNG_SET_REQUEST_SYNC      = 0x00008000,
} flag_usi_request_sync_type;

/** Values for the PRIME Mode */
#define PRIME_MODE_DISCONNECTED 0
#define	PRIME_MODE_TERMINAL     1
#define PRIME_MODE_SWITCH       2
#define PRIME_MODE_BASE         3

/** Values for PRIME Service Node State */
typedef enum {
 SN_STATE_DISCONNECTED  = 0,
 SN_STATE_REGISTERING   = 1,
 SN_STATE_TERMINAL      = 2,
 SN_STATE_UNREGISTERING = 3,
 SN_STATE_PROMOTING     = 4,
 SN_STATE_SWITCH        = 5,
 SN_STATE_DEMOTING      = 6,
 SN_STATE_ROAMING       = 7
} service_node_state;

/** Values for PRIME Service Node Register State */
typedef enum {
 REGISTER_STATE_TERMINAL      = 1,
 REGISTER_STATE_SWITCH        = 2
} service_node_register_state;

/** Values for PRIME Service Node Connection Type */
typedef enum {
 TYPE_CL_INVALID      = 0 /* MAC_CONNECTION_INVALID_TYPE */,
 TYPE_CL_IPv4_AR      = 1 /* MAC_CONNECTION_IPV4_AR_TYPE */,
 TYPE_CL_IPv4_UNICAST = 2 /* MAC_CONNECTION_IPV4_UNICAST_TYPE */,
 TYPE_CL_432          = 3 /* MAC_CONNECTION_CL_432_TYPE */,
 TYPE_CL_MGMT         = 4 /* MAC_CONNECTION_MNGT_TYPE */,
 TYPE_CL_IPv6_AR      = 5 /* MAC_CONNECTION_IPV6_AR_TYPE */,
 TYPE_CL_IPv6_UNICAST = 6 /* MAC_CONNECTION_IPV6_UNICAST_TYPE */
} mac_connection_type;

#define ADMIN_ENABLE   1
#define ADMIN_DISABLED 0

/* Timeouts for Syncronism on serialization (in seconds) */
#define PRIME_SYNC_TIMEOUT_DEFAULT          3
#define PRIME_SYNC_TIMEOUT_RESET_REQUEST    3
#define PRIME_SYNC_TIMEOUT_SLEEP_REQUEST    3
#define PRIME_SYNC_TIMEOUT_RESUME_REQUEST   3
#define PRIME_SYNC_TIMEOUT_REBOOT_REQUEST   3
#define PRIME_SYNC_TIMEOUT_GET_REQUEST      3
#define PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST 3
#define PRIME_SYNC_TIMEOUT_SET_REQUEST      3
#define PRIME_SYNC_TIMEOUT_ZC_REQUEST       3

//#define prime_cl_null_plme_reset_request_sync(x) prime_cl_null_plme_request_sync(prime_cl_null_plme_sleep_request(),x)

/**
 * \brief GET PRIME MAC State
 *        PRIME MAC State: 0: Disconnected 1: Terminal 2: Switch 3: Base
 */
uint8_t prime_get_mode();

/**
 * \brief GET PRIME MAC Band Plan
 *        PRIME MAC Band Plan
 */
uint8_t prime_get_band_plan();

/**
 * \brief GET PRIME PHY TX Channel
 *        PRIME PHY TX Channel
 */
uint8_t prime_get_tx_channel();

/**
 * \brief Configure Embedded Sniffer
 *
 * \param enable => Enables/Disables embedded sniffer
 */
int32_t prime_embedded_sniffer(uint8_t enable);

/**
 * \brief PLME Request syncronous with retries - For Reset & Sleep & Resume requests
 * \param prime_plme_request Function Pointer that can point to functions with no return type and no parameters
 * \param timeout            Timeout waiting answer from PRIME stack
 * \param pmacGetConfirm     Pointer to TmacGetConfirm structure
 */
void prime_cl_null_plme_request_sync(void (*prime_plme_request)(), uint8_t timeout, struct TmacGetConfirm *pmacGetConfirm);

/**
 * \brief PLME reset request syncronous
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacGetConfirm pointer to TmacGetConfirm structure
 */
void prime_cl_null_plme_reset_request_sync(uint8_t timeout, struct TmacGetConfirm *pmacGetConfirm);

/**
 * \brief PLME sleep request syncronous
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacGetConfirm pointer to TmacGetConfirm structure
 */
void prime_cl_null_plme_sleep_request_sync(uint8_t timeout, struct TmacGetConfirm *pmacGetConfirm);
/**
 * \brief PLME resume request syncronous
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacGetConfirm pointer to TmacGetConfirm structure
 */
void prime_cl_null_plme_resume_request_sync(uint8_t timeout, struct TmacGetConfirm *pmacGetConfirm);
/**
 * \brief PLME get request syncronous
 * \param us_pib_attrib  PIB attribute
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacGetConfirm pointer to TmacGetConfirm structure
 */
void prime_cl_null_plme_get_request_sync(uint16_t us_pib_attrib, uint8_t timeout, struct TmacGetConfirm *pmacGetConfirm);
/**
 * \brief PLME set request syncronous
 * \param us_pib_attrib  PIB attribute
 * \param pv_pib_value   PIB value
 * \param uc_pib_size    PIB value length
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacSetConfirm pointer to TmacSetConfirm structure
 */
void prime_cl_null_plme_set_request_sync(uint16_t us_pib_attrib, void *pv_pib_value, uint8_t uc_pib_size, uint8_t timeout, struct TmacSetConfirm *pmacSetConfirm);
/**
 * \brief MLME Promote Request Syncronous
 * \param puc_eui48      EUI48 of Service Node to promote
 * \param uc_bcn_mode    Beacon Mode
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacGetConfirm pointer to TmacGetConfirm structure
 */
void prime_cl_null_mlme_promote_request_sync(uint8_t * puc_eui48, uint8_t uc_bcn_mode, uint8_t timeout, struct TmacGetConfirm *pmacGetConfirm);
/**
 * \brief MLME Demote Request Syncronous
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacGetConfirm pointer to TmacGetConfirm structure
 */
void prime_cl_null_mlme_demote_request_sync(uint8_t timeout, struct TmacGetConfirm *pmacGetConfirm);
/**
 * \brief MLME Reset Request Syncronous
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacGetConfirm pointer to TmacGetConfirm structure
 */
void prime_cl_null_mlme_reset_request_sync(uint8_t timeout, struct TmacGetConfirm *pmacGetConfirm);
/**
 * \brief MLME get request syncronous
 * \param us_pib_attrib  PIB attribute
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacGetConfirm pointer to TmacGetConfirm structure
 */
void prime_cl_null_mlme_get_request_sync(uint16_t us_pib_attrib, uint8_t timeout, struct TmacGetConfirm *pmacGetConfirm);
/**
 * \brief MLME get list request syncronous
 * \param us_pib_attrib        PIB attribute
 * \param timeout              Timeout waiting answer from PRIME stack
 * \param pMlmeGetListConfirm  pointer to TmacGetConfirm structure
 */
void prime_cl_null_mlme_list_get_request_sync(uint16_t us_pib_attrib, uint8_t timeout, struct TmacGetConfirm *pmacGetConfirm);

/*
 * \brief MLME Unregister Request Syncronous
 * \param pmacGetConfirm pointer to TmacGetConfirm structure
 */
void prime_cl_null_mlme_unregister_request_sync(struct TmacGetConfirm *pmacGetConfirm);

/**
 * \brief MLME set request syncronous
 * \param us_pib_attrib  PIB attribute
 * \param pv_pib_value   PIB value
 * \param uc_pib_size    PIB value length
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacSetConfirm pointer to TmacSetConfirm structure
 */
void prime_cl_null_mlme_set_request_sync(uint16_t us_pib_attrib, void *pv_pib_value, uint8_t uc_pib_size, uint8_t timeout, struct TmacSetConfirm *pmacSetConfirm);
/**
 * \brief Management Plane Get Request Synchronous
 * \param us_pib_attrib  PRIME Attribute to get
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacGetConfirm PRIME MAC Get Confirm Pointer
 */
void prime_mngp_get_request_sync(uint16_t us_pib_attrib, uint8_t timeout, struct TmacGetConfirm *pmacGetConfirm);
/**
 * \brief Management Plane Set Request ASynchronous
 *
 * \param us_pib_attrib  PRIME Attribute to get
 * \param value          Value Pointer to set
 * \param length         Value Pointer length
 *
 */
void prime_mngp_set_request(uint16_t us_pib_attrib, uint8_t* value, uint8_t length);
/**
 * \brief Management Plane Set Request ASynchronous
 * \param us_pib_attrib  PRIME Attribute to set
 * \param pv_pib_value   PIB value
 * \param uc_pib_size    PIB value length
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacSetConfirm pointer to TmacSetConfirm structure
 */
void prime_mngp_set_request_sync(uint16_t us_pib_attrib, uint8_t* pv_pib_value, uint8_t uc_pib_size, uint8_t timeout, struct TmacSetConfirm *pmacSetConfirm);
/**
 * \brief Management Plane Get List Enhanced Request Synchronous
 * \param us_pib_attrib  PRIME Attribute to get list enhanced
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacSetConfirm PRIME MAC Get Confirm Pointer
 */
void prime_mngp_list_get_request_sync(uint16_t us_pib_attrib, uint8_t timeout, struct TmacGetConfirm *pmacGetConfirm);
/**
 * \brief Serial Profile Management Reset All Stats Request
 */
void prime_mngp_reset_stats_request();
/**
 * \brief Serial Profile Management Reset PHY Stats Request Asynchronous
 */
void prime_mngp_reset_phy_stats_request();
/**
 * \brief Serial Profile Management Reset MAC Stats Request Asynchronous
 */
void prime_mngp_reset_mac_stats_request();
/**
 * \brief Serial Profile Management Reboot Request
 */
void prime_mngp_reboot_request();
/**
 * \brief Base Management Plane Get Request Synchronous
 *
 * \param puc_eui48      EUI48
 * \param us_pib_attrib  PRIME Attribute to get
 * \param index          Attribute Index
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacGetConfirm PRIME MAC Get Confirm Pointer
 */
void prime_bmng_pprof_get_request_sync(uint8_t * puc_eui48, uint16_t us_pib_attrib, uint16_t index, uint8_t timeout, struct TmacGetConfirm *pmacGetConfirm);

/**
 * \brief Base Management Plane Get List Request Synchronous
 * \param puc_eui48      EUI48
 * \param us_pib_attrib  PRIME Attribute to get
 * \param index          Attribute Index
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacGetConfirm PRIME MAC Get Confirm Pointer
 *
 */
void prime_bmng_pprof_get_list_request_sync(uint8_t * puc_eui48, uint16_t us_pib_attrib, uint16_t index, uint8_t timeout, struct TmacGetConfirm *pmacGetConfirm);

/**
 * \brief Base Management Plane Set Request Synchronous
 *
 * \param puc_eui48      EUI48
 * \param us_pib_attrib  PRIME Attribute to Set
 * \param value          PRIME Attribute Value
 * \param length         PRIME Attribute Length
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacSetConfirm pointer to TmacSetConfirm structure
 */
void prime_bmng_pprof_set_request_sync(uint8_t * puc_eui48, uint16_t us_pib_attrib, uint8_t* value, uint8_t length, uint8_t timeout, struct TmacSetConfirm *pmacSetConfirm);

/**
 * \brief Serial Profile Management Reboot Request
 * \param puc_eui48      EUI48 to be rebooted
 * \param timeout        Timeout waiting answer from PRIME stack
 * \param pmacSetConfirm pointer to TmacSetConfirm structure
 */
void prime_bmng_reboot_request_sync(uint8_t * puc_eui48, uint8_t timeout, struct TmacSetConfirm *pmacSetConfirm);

/**
 * \brief   Base Management Zero Cross Request
 *          First received ACK_IND_CB, next ZC_DIFF_RESPONSE_CB and finally ZEROCROSS_RESPONSE_CB
 * \param   puc_eui48      EUI48 to be rebooted
 * \param   uc_timeout     Request Timeout
 * \param   pmacSetConfirm SetConfirm response structure
 */
void prime_bmng_zero_cross_request_sync(uint8_t * puc_eui48, uint8_t uc_timeout, struct TmacSetConfirm *pmacSetConfirm);

int base_node_manager_mng_init();

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */

#endif  // _BASE_NODE_MANAGER_MNG_H_
