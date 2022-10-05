/**
 * \file
 *
 * \brief Return Codes Header
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

#ifndef _RETURN_CODES_H_
#define _RETURN_CODES_H_

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

/* Return Error Codes */
typedef enum {
 SUCCESS = 0,
 /* Return Codes for PLME/MLME Primitive Requests */
 ERROR_CONFIRM_NOT_RECEIVED = 1,
 /* Return Codes for USI */
 ERROR_USERFNC_PORT_TYPE = 0x10,
 ERROR_USERFNC_FD = 0x11,
 ERROR_USERFNC_GETHOSTBYNAME = 0x12,
 ERROR_USERFNC_CONNECT = 0x13,
 ERROR_USERFNC_TTY_SPEED = 0x14,
 /* General Return Codes */
 ERROR_PRIME_INIT_MAC = 0xFFFFFFFF,
 ERROR_SET_EMBEDDED_SNIFFER = 0xFFFFFFFE,
 ERROR_PRIME_NETWORK_INIT = 0xFFFFFFFD,
 /* Return Codes for Networking */
 ERROR_TCP_SOCKET = 0xFFFFFFF0,
 ERROR_TCP_CONNECT = 0xFFFFFFEE,
 /* Return Codes for Sniffer */
 ERROR_SNIFFER_LOGFILE_OPEN  = 0xFFFFFFC0,
 ERROR_SNIFFER_LOGFILE_WRITE = 0xFFFFFFBF,
 ERROR_SNIFFER_TCP_SOCKET = 0xFFFFFFBE,
 ERROR_SNIFFER_TCP_SOCKET_OPTS = 0xFFFFFFBD,
 ERROR_SNIFFER_TCP_BIND = 0xFFFFFFBC,
 ERROR_SNIFFER_TCP_LISTEN = 0xFFFFFFBB,
 ERROR_SNIFFER_TCP_ACCEPT = 0xFFFFFFBA,
 ERROR_SNIFFER_TCP_THREAD = 0xFFFFFFB9,
 ERROR_SNIFFER_TCP_WRITE = 0xFFFFFFB8,
 /* Return Codes for Firmware Upgrade */
 ERROR_FW_UPGRADE_FILEPATH = 0xFFFFFFB0,
 ERROR_FW_UPGRADE_SET_OPTIONS = 0xFFFFFFAF,
 ERROR_FW_UPGRADE_CLEAR_TARGET_LIST = 0xFFFFFFAE,
 ERROR_FW_UPGRADE_ADD_TARGET = 0xFFFFFFAD,
 ERROR_FW_UPGRADE_SET_RULE = 0xFFFFFFAC,
 ERROR_FW_UPGRADE_SET_DATA = 0xFFFFFFAB,
 ERROR_FW_UPGRADE_INIT_FILE_TX = 0xFFFFFFAA,
 ERROR_FW_UPGRADE_CRC_CHECK = 0xFFFFFFA9,
 ERROR_FW_UPGRADE_REQUEST = 0xFFFFFFA8,
 /* Return Codes for DLMSoTCP */
 ERROR_DLMSoTCP_THREAD = 0xFFFFFFA0,
 ERROR_DLMSoTCP_MAX_MSGS = 0xFFFFFF9F,
 ERROR_DLMSoTCP_MALLOC = 0xFFFFFF9E,
 ERROR_DLMSoTCP_SELECT = 0xFFFFFF9D,
 ERROR_DLMSoTCP_SOCKET = 0xFFFFFF9C,
 ERROR_DLMSoTCP_VERSION = 0xFFFFFF9B,
 ERROR_DLMSoTCP_MIN_LENGTH = 0xFFFFFF9A,
 ERROR_DLMSoTCP_MAX_LENGTH = 0xFFFFFF99,
 ERROR_DLMSoTCP_FRAGMENTED = 0xFFFFFF98,
 ERROR_DLMSoTCP_SOCKETOPT = 0xFFFFFF97,
 ERROR_DLMSoTCP_BIND = 0xFFFFFF96,
 ERROR_DLMSoTCP_LISTEN = 0xFFFFFF95,
 ERROR_DLMSoTCP_BUFLEN = 0xFFFFFF94,
 ERROR_DLMSoTCP_LAST_MESSAGE = 0xFFFFFF93

} return_codes;

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */

#endif
