/**
 * \file
 *
 * \brief Globals.
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

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

#define PRIME_VERSION "0.0.1"

struct st_configuration {
	int      b_verbose;				/* Enable Verbose Mode */
	int      loglevel;        /* Level of verbosity */
	char     sz_tty_name[32]; /* TTY Name for connection to the PRIME Modem */
	uint32_t sz_tty_speed;		/* TTY Speed for connection to the PRIME Modem */
	char     sz_hostname[32];	/* HOSTNAME To connect Daemon - Virtualization */
	uint32_t sz_tcp_port;     /* TCP Port To connect Daemon - Virtualization */
	uint8_t  sz_port_type;    /* Port Type Connection */
  uint8_t  mikroBUS;        /* mikroBUS Conector of SAMA5D27 Board */
	uint8_t  mode;            /* PRIME Mode : Base/Terminal/Switch */
	uint8_t  state;           /* PRIME Mode state : SN Disconnected / Detection / Registering / Operative */
	uint8_t  tx_channel;      /* PHY TX Channel */
	uint8_t  band_plan;       /* Band Plan */
  uint8_t  mtp_mode;        /* MTP Mode : Enabled/Disabled */
	uint8_t  cert_mode;       /* Certification Mode */
	uint8_t  sniffer_mode;    /* Embedded Sniffer Mode : Enabled/Disabled */
	uint8_t  dlmsotcp_mode;   /* DLMS over TCP Server */
	uint8_t  eui48[6];        /* EUI48 */
	uint8_t  sec_profile;     /* Security Profile */
	uint8_t  duk[16];         /* DUK Key */
};

struct st_status {
	uint8_t  daemon     :1;   /* Daemon Status */
	uint8_t  sniffer_tcp:1;   /* TCP Server Sniffer Status */
	uint8_t  usi_link   :1;   /* USI Link Status */
	uint8_t  fup_status :3;   /* FW Upgrade Status */
};

struct st_info {
	uint16_t  app_vendor_id;       /* APP Vendor ID */
	uint16_t  app_product_id;      /* APP Product ID */
	int8_t    app_version[16];     /* APP Version */
	uint32_t  phy_sw_version;      /* PHY SW Version */
	uint32_t  phy_host_version;    /* PHY Host SW Version */
	uint32_t  mac_sw_version;      /* MAC SW Version */
	uint32_t  cl432_sw_version;    /* 4-32 SW Version */
};

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */

#endif //GLOBALS_H__
