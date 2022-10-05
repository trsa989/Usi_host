/**
 * \file
 *
 * \brief MAC_PIB: PRIME MAC information base
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

#ifndef MAC_PIB_H_INCLUDE
#define MAC_PIB_H_INCLUDE

/* System includes */
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* @endcond */

/**
 * \weakgroup prime_mac_group
 * @{
 */

/** \brief PHY statistical PIB attributes */
/* @{ */
#define PIB_PHY_STATS_CRC_INCORRECT             0x00A0
#define PIB_PHY_STATS_CRC_FAIL_COUNT            0x00A1
#define PIB_PHY_STATS_TX_DROP_COUNT             0x00A2
#define PIB_PHY_STATS_RX_DROP_COUNT             0x00A3
#define PIB_PHY_STATS_RX_TOTAL_COUNT            0x00A4
#define PIB_PHY_STATS_BLK_AVG_EVM               0x00A5
#define PIB_PHY_EMA_SMOOTHING                   0x00A8
/* @} */

/** \brief PHY implementation PIB attributes */
/* @{ */
#define PIB_PHY_TX_QUEUE_LEN                    0x00B0
#define PIB_PHY_RX_QUEUE_LEN                    0x00B1
#define PIB_PHY_TX_PROCESSING_DELAY             0x00B2
#define PIB_PHY_RX_PROCESSING_DELAY             0x00B3
#define PIB_PHY_AGC_MIN_GAIN                    0x00B4
#define PIB_PHY_AGC_STEP_VALUE                  0x00B5
#define PIB_PHY_AGC_STEP_NUMBER                 0x00B6
/* @} */

/** \brief MAC variable PIB attributes */
/* @{ */
#ifdef PRIME_API_V_1_4
#define PIB_MAC_VERSION                         0x0001
#endif
#define PIB_MAC_MIN_SWITCH_SEARCH_TIME          0x0010
#define PIB_MAC_MAX_PROMOTION_PDU               0x0011
#define PIB_MAC_PROMOTION_PDU_TX_PERIOD         0x0012
#ifndef PRIME_API_V_1_4
#define PIB_MAC_BEACONS_PER_FRAME               0x0013
#endif
#define PIB_MAC_SCP_MAX_TX_ATTEMPTS             0x0014
#ifndef PRIME_API_V_1_4
#define PIB_MAC_CTL_RE_TX_TIMER                 0x0015
#else
#define PIB_MAC_MIN_CTL_RE_TX_TIMER             0x0015
#endif
#ifndef PRIME_API_V_1_4
#define PIB_MAC_SCP_RBO                         0x0016
#endif
#define PIB_MAC_SCP_CH_SENSE_COUNT              0x0017
#ifndef PRIME_API_V_1_4
#define PIB_MAC_MAX_CTL_RE_TX                   0x0018
#else
#define PIB_MAC_CTL_MSG_FAIL_TIME               0x0018
#endif
#define PIB_MAC_EMA_SMOOTHING                   0x0019
#ifdef PRIME_API_V_1_4
#define PIB_MAC_MIN_BAND_SEARCH_TIME            0x001A
#define PIB_MAC_SAR_SIZE                        0x001D
#define PIB_MAC_EUI_48                          0x001F
#define PIB_MAC_CSMA_R1                         0x0034
#define PIB_MAC_CSMA_R2                         0x0035
#define PIB_MAC_CSMA_DELAY                      0x0038
#define PIB_MAC_CSMA_R1_ROBUST                  0x003B
#define PIB_MAC_CSMA_R2_ROBUST                  0x003C
#define PIB_MAC_CSMA_DELAY_ROBUST               0x003D
#define PIB_MAC_ALV_TIME_MODE                   0x003E
#define PIB_MAC_ACTION_ROBUSTNESS_MGMT          0x004A
#define PIB_MAC_UPDATED_RM_TIMEOUT              0x004B
#define PIB_MAC_ALV_HOP_REPETITIONS             0x004C
#endif
/* @} */

/** \brief MAC functional PIB attributes */
/* @{ */
#define PIB_MAC_LNID                            0x0020
#define PIB_MAC_LSID                            0x0021
#define PIB_MAC_SID                             0x0022
#define PIB_MAC_SNA                             0x0023
#define PIB_MAC_STATE                           0x0024
#define PIB_MAC_SCP_LENGTH                      0x0025
#define PIB_MAC_NODE_HIERARCHY_LEVEL            0x0026
#ifndef PRIME_API_V_1_4
#define PIB_MAC_BEACON_SLOT_COUNT               0x0027
#define PIB_MAC_BEACON_RX_SLOT                  0x0028
#define PIB_MAC_BEACON_TX_SLOT                  0x0029
#endif
#define PIB_MAC_BEACON_RX_FREQUENCY             0x002A
#define PIB_MAC_BEACON_TX_FREQUENCY             0x002B
#define PIB_MAC_MAC_CAPABILITES                 0x002C
#ifdef PRIME_API_V_1_4
#define PIB_MAC_FRAME_LENGTH                    0x002D
#define PIB_MAC_CFP_LENGTH                      0x002E
#define PIB_MAC_GUARD_TIME                      0x002F
#define PIB_MAC_BC_MODE                         0x0030
#define PIB_MAC_BEACON_RX_QLTY                  0x0032
#define PIB_MAC_BEACON_TX_QLTY                  0x0033
#define PIB_MAC_BEACON_RX_POS                   0x0039
#define PIB_MAC_BEACON_TX_POS                   0x003A
#endif
/* @} */

/** \brief MAC statistical PIB attributes */
/* @{ */
#define PIB_MAC_TX_DATAPKT_COUNT                0x0040
#define PIB_MAC_RX_DATAPKT_COUNT                0x0041
#define PIB_MAC_TX_CTRLPKT_COUNT                0x0042
#define PIB_MAC_RX_CTRLPKT_COUNT                0x0043
#define PIB_MAC_CSMA_FAIL_COUNT                 0x0044
#define PIB_MAC_CSMA_CH_BUSY_COUNT              0x0045
/* @} */

/** \brief MAC list PIB attributes */
/* @{ */
#define PIB_MAC_LIST_REGISTER_DEVICES           0x0050
#define PIB_MAC_LIST_ACTIVE_CONN                0x0051
#define PIB_MAC_LIST_MCAST_ENTRIES              0x0052
#ifndef PRIME_API_V_1_4
#define PIB_MAC_LIST_SWITCH_TABLE               0x0053
#else
#define PIB_MAC_LIST_SWITCH_TABLE               0x005A
#endif
#define PIB_MAC_LIST_DIRECT_CONN                0x0054
#define PIB_MAC_LIST_DIRECT_TABLE               0x0055
#define PIB_MAC_LIST_AVAILABLE_SWITCHES         0x0056
#ifndef PRIME_API_V_1_4
#define PIB_MAC_LIST_PHY_COMM                   0x0057
#else
#define PIB_MAC_LIST_PHY_COMM                   0x0059
#endif
#define PIB_MAC_LIST_ACTIVE_CONN_EX             0x0058
/* @} */

/** \brief MAC security PIB attributes */
/* @{ */
#ifdef PRIME_API_V_1_4
#define PIB_MAC_SEC_DUK                         0x005B
#define PIB_MAC_UPDATE_KEYS_TIME                0x005C
#endif
/* @} */

/** \brief MAC action PIB attributes */
/* @{ */
#define PIB_MAC_ACTION_TX_DATA                  0x0060
#define PIB_MAC_ACTION_CONN_CLOSE               0x0061
#define PIB_MAC_ACTION_REG_REJECT               0x0062
#define PIB_MAC_ACTION_PRO_REJECT               0x0063
#define PIB_MAC_ACTION_UNREGISTER               0x0064
#define PIB_MAC_ACTION_PROMOTE                  0x0065
#define PIB_MAC_ACTION_DEMOTE                   0x0066
#define PIB_MAC_ACTION_REJECT                   0x0067
#define PIB_MAC_ACTION_ALIVE_TIME               0x0068
#ifndef PRIME_API_V_1_4
#define PIB_MAC_ACTION_PRM                      0x0069
#endif
#define PIB_MAC_ACTION_BROADCAST_DATA_BURST     0x006A
#define PIB_MAC_ACTION_MGMT_CON                 0x006B
#define PIB_MAC_ACTION_MGMT_MUL                 0x006C
#define PIB_MAC_ACTION_UNREGISTER_BN            0x006D
#define PIB_MAC_ACTION_CONN_CLOSE_BN            0x006E
#define PIB_MAC_ACTION_SEGMENTED_432            0x006F
#define PIB_MAC_ACTION_APPEMU_DATA_BURST        0x0080
#ifdef PRIME_API_V_1_4
#define PIB_MAC_ACTION_MGMT_DATA_BURST          0x0081
#define PIB_MAC_ACTION_PRO_BCN                  0x0082
#define PIB_MAC_ACTION_PROMOTE_DS               0x0083
#endif
/* @} */

/** \brief Management Plane firmware upgrade PIB attributes */
/* @{ */
#define PIB_FU_APP_FWDL_RUNNING                 0x0070
#define PIB_FU_APP_FWDL_RX_PKT_COUNT            0x0071
/* @} */

/** \brief MAC application PIB attributes */
/* @{ */
#define PIB_MAC_APP_FW_VERSION                  0x0075
#define PIB_MAC_APP_VENDOR_ID                   0x0076
#define PIB_MAC_APP_PRODUCT_ID                  0x0077
#ifdef PRIME_API_V_1_4
#define PIB_MAC_APP_LIST_ZC_STATUS              0x0078
#endif
/* @} */

/** \brief Propietary MAC certification PIB attributes */
/* @{ */
#define PIB_MAC_ACTION_CFP_LENGTH               0x810D
#define PIB_MAC_ACTION_ARQ_WIN_SIZE             0x8124
#ifndef PRIME_API_V_1_4
#define PIB_MAC_ACTION_BCN_SLOT_COUNT           0x810E
#endif
#ifdef PRIME_API_V_1_4
#define PIB_MAC_ALV_MIN_LEVEL                   0x810F
#define PIB_MAC_ACTION_FRAME_LENGTH             0x8110
#endif
#define PIB_CERTIFICATION_MODE                  0x8120
#define PIB_CERTIFICATION_SEND_MSG              0x8121
#ifdef PRIME_API_V_1_4
#define PIB_CERT_MIN_LEVEL_TO_REG               0x8130
#endif
#ifndef PRIME_API_V_1_4
#define PIB_BCN_SLOTS_BUSY                      0x8131
#endif
/* @} */

/** \brief Propietary MAC manufacturing test process (MTP) PIB attributes */
/* @{ */
#define PIB_MTP_PHY_CFG_LOAD_THRESHOLD_1        0x8003
#define PIB_MTP_PHY_CFG_LOAD_THRESHOLD_2        0x8004
#define PIB_MTP_PHY_TX_TIME                     0x8085
#define PIB_MTP_PHY_RMS_CALC_CORRECTED          0x8086
#define PIB_MTP_PHY_EXECUTE_CALIBRATION         0x8087
#define PIB_MTP_PHY_RX_PARAMS                   0x8088
#define PIB_MTP_PHY_TX_PARAMS                   0x8089
#define PIB_MTP_PHY_CONTINUOUS_TX               0x808A
#define PIB_PHY_TX_CHANNEL                      0x8090
#define PIB_PHY_TXRX_CHANNEL_LIST               0x8092
#define PIB_MTP_PHY_ENABLE                      0x808E
#define PIB_MTP_PHY_DRV_AUTO                    0x8301
#define PIB_MTP_PHY_DRV_IMPEDANCE               0x8302
#define PIB_MTP_MAC_WRITE_SNA                   0x8123
/* @} */

/** \brief Propietary Management Plane firmware upgrade PIB attributes */
/* @{ */
#define PIB_FU_LIST                             0x8350
/* @} */

/** \brief Other vendor specific PIB attributes */
/* @{ */
#define PIB_PHY_SW_VERSION                      0x8080
#define PIB_PHY_ZCT                             0x8081
#define PIB_PHY_HOST_VERSION                    0x8082
#define PIB_PHY_SERIAL_ENABLED                  0x8091
#define PIB_MTP_MAC_EUI_48                      0x8100
#define PIB_MAC_PLC_STATE                       0x8101
#define PIB_MAC_SERVICE_STATE                   0x8102
#define PIB_MAC_REG_RSS                         0x8103
#define PIB_MAC_BCN_AVG_EN                      0x8104
#define PIB_PHY_SNIFFER_ENABLED                 0x8106
#define PIB_MAC_INTERNAL_SW_VERSION             0x8126
#define PIB_432_CON_STATE                       0x8200
#define PIB_432_INTERNAL_SW_VERSION             0x8201
#define PIB_432_LIST_NODES                      0x8250
#ifdef PRIME_API_V_1_4
#define PIB_MAC_SEC_DUK_BN                      0x8140
#define PIB_MAC_SEC_PROFILE_USED                0x8141
#define PIB_MAC_SEC_OLD_SWK_TIME                0x8142
#define PIB_MAC_ACTION_MGMT_MUL_SEND_DATA       0x8132
#define PIB_MAC_ACTION_BCN_TX_SCHEME            0x8133
#define PIB_MAC_ACTION_ALV_TYPE                 0x8134
#define PIB_MAC_CHN_SCANNING_MODE               0x8135
#define PIB_MAC_ACTION_CFG_BCN_SWITCH_RATE      0x8136
#define PIB_MAC_ACTION_CFG_SEC_PROF             0x8137
#endif
#define PIB_MAC_WHITELIST                       0x8150
#define PIB_MAC_WHITELIST_ENABLED               0x8151
#define PIB_MAC_ACTION_CLEAR_NWK_STRUCTURE      0x8152
/* @} */

/* @} */

/* @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* @endcond */
#endif /* MAC_PIB_H_INCLUDE */
