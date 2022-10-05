/**
 * \file
 *
 * \brief Base Manager VTY Header.
 *
 * Copyright (c) 2020 Microchip Technology Inc. and its subsidiaries.
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

#ifndef _PRIME_VTY_H
#define _PRIME_VTY_H

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

#include <stdint.h>

#define vty_out_mchp(vty, args...)   ({ if ( vty && (vty->fd != -1) ) vty_out(vty, args); })

int prime_vty_init(void);

#define PRIME_SHOW_STR     "Show PRIME\r\n"
#define PRIME_CONFIG_STR   "PRIME Configuration\r\n"
#define REQUEST_PIB        "PIB Attribute\r\n"
#define REQUEST_PHY_PIB    "PHY Attribute\r\n"
#define REQUEST_MAC_PIB    "MAC Attribute\r\n"
#define REQUEST_MTP_PIB    "MTP Attribute\r\n"
#define REQUEST_CL432_PIB  "CL4-32 Attribute\r\n"
#define REQUEST_APP_PIB    "APP Attribute\r\n"
#define REQUEST_GET_PIB    "Get PIB Attribute\r\n"
#define REQUEST_SET_PIB    "Set PIB Attribute\r\n"
#define REQUEST_ACTION_PIB "Action PIB Attribute\r\n"
#define REQUEST_MLME       "Request via MLME primitive\r\n"
#define REQUEST_PLME       "Request via PLME primitive\r\n"
#define REQUEST_MNGP       "Request via MNGP\r\n"
#define REQUEST_BMNG       "Request via Base Management PRIME Profile\r\n"

/****************************************************
* @struct TPRIMEsniffer_log_options
* @brief  Structure to save PRIME Sniffer Configuration
****************************************************/
typedef struct {
  uint8_t enabled;    /*! Enable Sniffer */
  uint8_t logfile_en; /*! Enable Sniffer on Logfile */
  uint8_t tcp_en;     /*! Enable Sniffer on TCP socket */
  uint32_t tcp_port;  /*! TCP Port */
} TPRIMEsniffer_log_options;

#define PRIMESNIFFER_LOG_OPTIONS_ENABLED_DEFAULT    0
#define PRIMESNIFFER_LOG_OPTIONS_LOGFILE_EN_DEFAULT 0
#define PRIMESNIFFER_LOG_OPTIONS_TCP_EN             0
#define PRIMESNIFFER_LOG_OPTIONS_TCP_PORT           4444

#define TOPOLOGY_FILE "/tmp/topology"

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */

#endif /* _PRIME_VTY_H */
