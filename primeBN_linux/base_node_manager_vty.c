/**
 * \file
 *
 * \brief Base Node Manager VTY file.
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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "command.h"
#include "vty.h"
#include "globals.h"
#include "mac_pib.h"
#include "mac_defs.h"
#include "prime_api_host.h"
#include "prime_api_defs_host.h"
#include "ifacePrime_api.h"
#include "UsiCfg.h"
#include "base_node_manager.h"
#include "base_node_mng.h"
#include "base_node_dlmsotcp.h"
#include "base_node_mng_fw_upgrade.h"
#include "base_node_network.h"
#include "prime_bmng_network_events.h"
#include "base_node_manager_vty.h"
#include "prime_log.h"
#include "prime_utils.h"
#include "return_codes.h"
#include "prime_sniffer.h"
#include "vtysh_config.h"
#include "conf_global.h"
#include <string.h>

/************************************************************
*       Defines                                             *
*************************************************************/
#define VTY_CHECK_PRIME_MODE(x...)                                                  \
        if (g_st_config.mode != x){                                                 \
           vty_out(vty,"No PRIME %s Node. Not implemented\r\n",mac_state_str[x]);   \
           return CMD_ERR_NOTHING_TODO;                                             \
        }

#define VTY_CHECK_PRIME_MODE_NO_BASE()                          \
        if (g_st_config.mode == PRIME_MODE_BASE){               \
           vty_out(vty,"PRIME Base Node. Not implemented\r\n"); \
           return CMD_ERR_NOTHING_TODO;                         \
        }

#define VTY_CHECK_PRIME_MTP_MODE()          \
        if (!g_st_config.mtp_mode){         \
           vty_out(vty,"No MTP mode\r\n");  \
           return CMD_ERR_NOTHING_TODO;     \
        }

/* Global Configuration */
extern struct st_configuration g_st_config ;
extern struct st_status g_st_status ;
extern struct st_info g_st_info ;

/* Linked List for PRIME Network visibility */
extern mchp_list prime_network;

/* Only to print Modulation Scheme */
const char modultation_scheme_str[2][14] = {"Differential","Coherent"};
/* Only to print Modulation Type */
const char modultation_type_str[4][5] = {"ROBO","BPSK","QPSK","8PSK"};
/* Only to print MAC State */
const char mac_state_str[4][16] = {" DISCONN","TERMINAL"," SWITCH ","  BASE  "};
/* Only to print MAC Connection Types */
const char mac_connection_type_str[7][14] = {"INVALID","IPv4_AR","IPv4_UNICAST","432","MGMT","IPv6_AR","IPv6_UNICAST"};
/* Only to print CL4-32 Connection State */
const char cl432_conn_state_str[4][14] = {"CLOSED","CONNECTING","DISCONNECTING","OPEN"};
/* Only to print Zero Cross Directions */
const char zcstatus_dir_str[4][9] = {"UNKNOWN","FALLING","RAISING","RESERVED"};
/* Only to print Zero Cross Status */
const char zcstatus_status_str[8][12] = {"UNKNOWN","50HZ","60HZ","RESERVED","RESERVED","RESERVED","IRREGULAR","UNAVAILABLE"};
/* Only to print Firmware Update Status */
const char fw_upgrade_status_str[3][8] = { "IDLE", "RUNNING", "FINISH" };

TPRIMEsniffer_log_options PRIMEsniffer_log_options;  /* G3 Sniffer Options Global Var */
extern uint32_t flag_prime_sniffer;

/*
   Definition of a new node PRIME_NODE.

   The command "end" must support this node to return to ENABLE_NODE.
   For that add "case PRIME_NODE:" to line 2464 of file lib/command.c
*/
static struct cmd_node prime_node =
{
   PRIME_NODE,
   "%s(prime)# ",
   1
};

static struct list *prime_config = NULL;

DEFUN (config_prime,
       config_prime_cmd,
       "prime",
       "Configuration Prime parameters\n"
       "Configuration Prime parameters\n")
{
  vty->node = PRIME_NODE;
  return CMD_SUCCESS;
}

DEFUN (config_prime_exit,
       config_prime_exit_cmd,
       "exit",
       "Exit current mode and down to previous mode\n")
{
  vty->node = CONFIG_NODE;

  return CMD_SUCCESS;
}

DEFUN (config_prime_end,
       config_prime_end_cmd,
       "end",
       "End current mode and change to enable mode\n")
{
	vty->node = ENABLE_NODE;
	return CMD_SUCCESS;
}

DEFUN (prime_config_log_enable,
       prime_config_log_enable_cmd,
       "config log (enabled|disabled)",
       PRIME_CONFIG_STR
       "PRIME log\n"
       "PRIME log enabled\n"
       "PRIME log disabled\n")
{
    char info[256];

    // Configure Log
    if (strcmp(argv[0],"enabled") == 0){
       // Enable Log
       prime_enable_log();
    }else{
       // Disable Log
       prime_disable_log();
    }

    // Write File Config
    sprintf(info, "config log enabled");
    config_del_line_byleft(prime_config, info);
    sprintf(info, "config log disabled");
    config_del_line_byleft(prime_config, info);
    sprintf(info, "config log %s", argv[0]);
    config_add_line(prime_config, info);
    ENSURE_CONFIG(vty);

    vty_out(vty,"PRIME Log %s\r\n",argv[0]);

	  return CMD_SUCCESS;
}

DEFUN (prime_config_loglevel,
       prime_config_loglevel_cmd,
       "config loglevel (global|sniffer|dlmsotcp|net_events) <0-4>",
       PRIME_CONFIG_STR
       "PRIME loglevel\n"
       "PRIME Global loglevel\n"
       "PRIME Sniffer loglevel\n"
       "PRIME DLMSoTCP loglevel\n"
       "PRIME Network Events loglevel\n"
       "PRIME loglevel 0:NONE 1:FATAL 2:ERR 3:INFO 4:DEBUG\n")
{
    int loglevel = 0;

    loglevel = atoi(argv[1]);
    if (strncmp("global",argv[0],7)==0){
       prime_set_loglevel(loglevel);
       prime_sniffer_set_loglevel(loglevel);
       prime_dlmsotcp_set_loglevel(loglevel);
       prime_bmng_network_event_set_loglevel(loglevel);
    }else if (strncmp("sniffer",argv[0],8)==0){
       prime_sniffer_set_loglevel(loglevel);
    }else if (strncmp("dlmsotcp",argv[0],9)==0){
       prime_dlmsotcp_set_loglevel(loglevel);
    }else if (strncmp("net_events",argv[0],11)==0){
       prime_bmng_network_event_set_loglevel(loglevel);
    }
    vty_out(vty,"PRIME Loglevel %s set to %d\r\n",argv[0],loglevel);
	  return CMD_SUCCESS;
}

DEFUN (prime_show_info,
       prime_show_info_cmd,
       "show info",
       PRIME_SHOW_STR
       "PRIME Information\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
/*********************************************
*       Code                                 *
**********************************************/
  vty_out(vty,"PRIME Information\r\n");
  vty_out(vty,"\r * Vendor ID   : 0x%04X\r\n",g_st_info.app_vendor_id);
  vty_out(vty,"\r * Product ID  : 0x%04X\r\n",g_st_info.app_product_id);
  vty_out(vty,"\r * FW Version  : %s\r\n",g_st_info.app_version);
  vty_out(vty,"\r * PHY Version : 0x%08X\r\n",g_st_info.phy_sw_version);
  vty_out(vty,"\r * HOST Version: 0x%08X\r\n",g_st_info.phy_host_version);
  vty_out(vty,"\r * MAC Version : 0x%08X\r\n",g_st_info.mac_sw_version);
  vty_out(vty,"\r * 4-32 Version: 0x%08X\r\n",g_st_info.cl432_sw_version);
	return CMD_SUCCESS;
}

DEFUN (prime_show_info_mac,
       prime_show_info_mac_cmd,
       "show info MAC",
       PRIME_SHOW_STR
       "PRIME Information\n"
       "PRIME MAC\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  if (!sn->registered){
    vty_out(vty, "Service Node with EUI48 '%s' not registered\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  vty_out(vty,"EUI48          TCap SwCap      Vendor            Model           Version    \r\n");
  vty_out(vty,"-------------- ---- ----- ---------------  ---------------- ----------------\r\n");
  vty_out(vty,"0x%s 0x%02X 0x%02X %16s %16s %16s\r\n",    \
                                              eui48_to_str(sn->regEntryID,NULL), \
                                              sn->regEntryTCap,                  \
                                              sn->regEntrySwCap,                 \
                                              sn->fu_vendor,                     \
                                              sn->fu_model,                      \
                                              sn->fu_version);
	return CMD_SUCCESS;
}

DEFUN (prime_show_status,
       prime_show_status_cmd,
       "show status",
       PRIME_SHOW_STR
       "PRIME Status\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
/*********************************************
*       Code                                 *
**********************************************/

	return CMD_SUCCESS;
}

DEFUN (prime_show_status_mac,
       prime_show_status_mac_cmd,
       "show status MAC",
       PRIME_SHOW_STR
       "PRIME Status\n"
       "PRIME MAC\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  if (!sn->registered){
    vty_out(vty, "Service Node with EUI48 '%s' not registered\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  vty_out(vty,"EUI48           LNID    State   LSID  SID Level TxALV RxALV ALV-T\r\n");
  vty_out(vty,"-------------- ------- -------  ----  --- ----- ----- ----- -----\r\n");
  vty_out(vty,"0x%s  %05d  %08s  %03d  %03d  %03d  0x%02X  0x%02X  0x%02X\r\n",         \
                                              eui48_to_str(sn->regEntryID,NULL),        \
                                              sn->regEntryLNID,                         \
                                              mac_state_str[sn->regEntryState],         \
                                              sn->regEntryLSID,                         \
                                              sn->regEntrySID,                          \
                                              sn->regEntryLevel,                        \
                                              sn->alvTxcnt,                             \
                                              sn->alvRxcnt,                             \
                                              sn->alvTime);
	return CMD_SUCCESS;
}

DEFUN (prime_show_config,
       prime_show_config_cmd,
       "show config",
       PRIME_SHOW_STR
       "PRIME Configuration Information\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
int sniffer_flags;
/*********************************************
*       Code                                 *
**********************************************/
  sniffer_flags = prime_sniffer_get_flags();
  // Configure PRIME Security Profile
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  vty_out(vty,"PRIME Main Configuration\r\n");
  vty_out(vty,"\t * EUI48       : %s\r\n",eui48_to_str(g_st_config.eui48,NULL));
  vty_out(vty,"\t * Mode        : %s\r\n",mac_state_str[g_st_config.mode]);
  vty_out(vty,"\t * Sec Profile : %d\r\n",g_st_config.sec_profile);
  vty_out(vty,"\t * Band Plan   : %d\r\n",g_st_config.band_plan);
  vty_out(vty,"\t * TX Channel  : %d\r\n",g_st_config.tx_channel);
  vty_out(vty,"PRIME Connection Configuration\r\n" \
              "\t * Modemport   : %s\r\n", (g_st_config.sz_port_type != TCP_TYPE) ? "Serial":"TCP/IP");
  if (g_st_config.sz_port_type != TCP_TYPE){
  vty_out(vty,"\t * Port        : %s\r\n"                 \
              "\t * Speed       : %d\r\n",                \
              g_st_config.sz_tty_name, g_st_config.sz_tty_speed);
  }else{
  vty_out(vty,"\t * Remote IP   : %s\r\n"                 \
              "\t * Remote Port : %d\r\n",                \
              g_st_config.sz_hostname, g_st_config.sz_tcp_port);
  }
  vty_out(vty,"PRIME Sniffer Log Configuration\r\n"                     \
              "\t * Enabled     : %s\r\n"                                  \
              "\t * Logfile     : %s\r\n"                                  \
              "\t * TCP Server  : %s\r\n",                                 \
              (sniffer_flags & SNIFFER_FLAG_ENABLE)  ? "Yes": "No", \
              (sniffer_flags & SNIFFER_FLAG_LOGFILE) ? "Yes": "No", \
              (sniffer_flags & SNIFFER_FLAG_SOCKET)  ? "Yes": "No");
  if (sniffer_flags & SNIFFER_FLAG_SOCKET)
     vty_out(vty,"\t * TCP Port    : %d\r\n",prime_sniffer_tcp_get_port());
  vty_out(vty,"PRIME DLMSoTCP\r\n"                                         \
              "\t * Enabled     : %s\r\n" \
              "\t * TCP Port    : %d\r\n", g_st_config.dlmsotcp_mode? "Yes":"No", prime_dlmsotcp_get_port());

	return CMD_SUCCESS;
}

DEFUN (prime_show_config_mac,
       prime_show_config_mac_cmd,
       "show config MAC",
       PRIME_SHOW_STR
       "PRIME Config\n"
       "PRIME MAC\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
	return CMD_SUCCESS;
}

DEFUN (prime_config_sniffer_log_enable,
       prime_config_sniffer_log_enable_cmd,
       "config sniffer_log (enabled|disabled)",
       PRIME_CONFIG_STR
       "PRIME Sniffer Log\n"
       "PRIME Sniffer Log enabled\n"
       "PRIME Sniffer Log disabled\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
int sniffer_flags;
char info[256];
/*********************************************
*       Code                                 *
**********************************************/

    sniffer_flags = prime_sniffer_get_flags();
    // Configure PRIME Band
    if (strcmp(argv[0],"enabled") == 0){
       // Enable PRIME Sniffer
       sniffer_flags |= SNIFFER_FLAG_ENABLE;
       prime_sniffer_start(sniffer_flags);
       g_st_config.sniffer_mode = 1;
    }else{
       // Disables PRIME Sniffer
       sniffer_flags &= ~SNIFFER_FLAG_ENABLE;
       prime_sniffer_stop(sniffer_flags);
       g_st_config.sniffer_mode = 0;
    }

    // Write File Config
    sprintf(info, "config sniffer_log enabled");
    config_del_line_byleft(prime_config, info);
    sprintf(info, "config sniffer_log disabled");
    config_del_line_byleft(prime_config, info);
    sprintf(info, "config sniffer_log %s", argv[0]);
    config_add_line(prime_config, info);
    ENSURE_CONFIG(vty);
    vty_out(vty,"PRIME Sniffer Log %s\r\n",argv[0]);

	  return CMD_SUCCESS;
}

DEFUN (prime_config_sniffer_log_logfile_enable,
       prime_config_sniffer_log_logfile_enable_cmd,
       "config sniffer_log logfile (enabled|disabled)",
       PRIME_CONFIG_STR
       "PRIME Sniffer Log\n"
       "PRIME Sniffer Log logfile \n"
       "PRIME Sniffer Log Logfile enabled\n"
       "PRIME Sniffer Log Logfile disabled\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
int sniffer_flags;
char info[256];
/*********************************************
*       Code                                 *
**********************************************/

    sniffer_flags = prime_sniffer_get_flags();
    // Configure Sniffer Logfile
    if (strcmp(argv[0],"enabled") == 0){
       // Enable PRIME Sniffer to Logfile
       sniffer_flags |= SNIFFER_FLAG_LOGFILE;
    }else{
       // Disables PRIME Sniffer to Logfile
       sniffer_flags &= ~SNIFFER_FLAG_LOGFILE;
    }
    prime_sniffer_set_flags(sniffer_flags);

    // Write File Config
    sprintf(info, "config sniffer_log logfile enabled");
    config_del_line_byleft(prime_config, info);
    sprintf(info, "config sniffer_log logfile disabled");
    config_del_line_byleft(prime_config, info);
    sprintf(info, "config sniffer_log logfile %s", argv[0]);
    config_add_line(prime_config, info);
    ENSURE_CONFIG(vty);
    vty_out(vty,"PRIME Sniffer logfile %s\r\n",argv[0]);

	  return CMD_SUCCESS;
}

DEFUN (prime_config_sniffer_log_tcp_enable,
       prime_config_sniffer_log_tcp_enable_cmd,
       "config sniffer_log tcp (enabled|disabled)",
       PRIME_CONFIG_STR
       "PRIME Sniffer Log\n"
       "PRIME Sniffer Log TCP\n"
       "PRIME Sniffer Log TCP enabled\n"
       "PRIME Sniffer Log TCP disabled\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
int sniffer_flags;
char info[256];
/*********************************************
*       Code                                 *
**********************************************/

    sniffer_flags = prime_sniffer_get_flags();
    // Configure Sniffer Logfile
    if (strcmp(argv[0],"enabled") == 0){
       // Enable PRIME Sniffer to Logfile
       sniffer_flags |= SNIFFER_FLAG_SOCKET;
    }else{
       // Disables PRIME Sniffer to Logfile
       sniffer_flags &= ~SNIFFER_FLAG_SOCKET;
    }
    prime_sniffer_set_flags(sniffer_flags);

    // Write File Config
    sprintf(info, "config sniffer_log tcp enabled");
    config_del_line_byleft(prime_config, info);
    sprintf(info, "config sniffer_log tcp disabled");
    config_del_line_byleft(prime_config, info);
    sprintf(info, "config sniffer_log tcp %s", argv[0]);
    config_add_line(prime_config, info);
    ENSURE_CONFIG(vty);
    vty_out(vty,"PRIME Sniffer Log tcp %s\r\n",argv[0]);

	  return CMD_SUCCESS;
}

DEFUN (prime_config_sniffer_log_tcp_port,
       prime_config_sniffer_log_tcp_port_cmd,
       "config sniffer_log tcp port <0-9999>",
       PRIME_CONFIG_STR
       "PRIME Sniffer Log\n"
       "PRIME Sniffer Log TCP\n"
       "PRIME Sniffer Log TCP Port\n"
       "PRIME Sniffer Log TCP Port value\n")
{
    char info[256];
    int32_t TCPport = 0;
    int32_t ret;

    TCPport = atoi(argv[0]);

    ret = prime_sniffer_tcp_set_port(TCPport);
    if (ret){
        vty_out(vty,"Impossible to set PRIME Sniffer Log tcp port %d (%d)\r\n",TCPport,ret);
        return CMD_ERR_NOTHING_TODO;
    }

    // Write File Config
    sprintf(info, "config sniffer_log tcp port");
    config_del_line_byleft(prime_config, info);
    sprintf(info, "config sniffer_log tcp port %d", TCPport);
    config_add_line(prime_config, info);
    ENSURE_CONFIG(vty);
    vty_out(vty,"PRIME Sniffer Log tcp port %d\r\n",TCPport);

	  return CMD_SUCCESS;
}

DEFUN (prime_show_sniffer_log_config,
       prime_show_sniffer_log_config_cmd,
       "show sniffer_log config",
       PRIME_SHOW_STR
       "PRIME Sniffer Log\n"
       "PRIME Sniffer Log Configuration\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
int sniffer_flags;
/*********************************************
*       Code                                 *
**********************************************/
    sniffer_flags = prime_sniffer_get_flags();
    vty_out(vty,"PRIME Sniffer Log Configuration:\r\n"                       \
                "\r * Enabled     : %s\r\n"                                  \
                "\r * Logfile     : %s\r\n"                                  \
                "\r * TCP Server  : %s\r\n",                                 \
                (sniffer_flags & SNIFFER_FLAG_ENABLE)  ? "Yes": "No",        \
                (sniffer_flags & SNIFFER_FLAG_LOGFILE) ? "Yes": "No",        \
                (sniffer_flags & SNIFFER_FLAG_SOCKET)  ? "Yes": "No");
    if (sniffer_flags & SNIFFER_FLAG_SOCKET)
       vty_out(vty,"\r * TCP Port    : %d\r\n",prime_sniffer_tcp_get_port());
	  return CMD_SUCCESS;
}

/* DLMS over TCP Server */
DEFUN (prime_config_dlmsotcp_enable,
       prime_config_dlmsotcp_enable_cmd,
       "config dlmsotcp (enabled|disabled)",
       PRIME_CONFIG_STR
       "PRIME DLMS over TCP\n"
       "PRIME DLMS over TCP enabled\n"
       "PRIME DLMS over TCP disabled\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
char info[256];
/*********************************************
*       Code                                 *
**********************************************/

    // Configure DLMS over TCP Server
    if (strcmp(argv[0],"enabled") == 0){
       // Enable PRIME DLMSoTCP Server
       base_node_dlmsotcp_start();
       g_st_config.dlmsotcp_mode = 1;
    }else{
       // Disables PRIME DLMSoTCP Server
       base_node_dlmsotcp_stop();
       g_st_config.dlmsotcp_mode = 0;
    }

    // Write File Config
    sprintf(info, "config dlmsotcp");
    config_del_line_byleft(prime_config, info);
    sprintf(info, "config dlmsotcp %s", argv[0]);
    config_add_line(prime_config, info);
    ENSURE_CONFIG(vty);
    vty_out(vty,"PRIME DLMS over TCP %s\r\n",argv[0]);

	  return CMD_SUCCESS;
}

DEFUN (prime_config_dlmsotcp_port,
       prime_config_dlmsotcp_port_cmd,
       "config dlmsotcp port <0-9999>",
       PRIME_CONFIG_STR
       "DLMS over TCP\n"
       "DLMS over TCP Port\n"
       "DLMS over TCP Port value\n")
{
    char info[256];
    int32_t TCPport = 0;
    int32_t ret;

    TCPport = atoi(argv[0]);

    ret = prime_dlmsotcp_set_port(TCPport);
    if (ret){
        vty_out(vty,"Impossible to set PRIME DLMS over TCP port %d (%d)\r\n",TCPport,ret);
        return CMD_ERR_NOTHING_TODO;
    }

    // Write File Config
    sprintf(info, "config dlmsotcp port");
    config_del_line_byleft(prime_config, info);
    sprintf(info, "config dlmsotcp port %d", TCPport);
    config_add_line(prime_config, info);
    ENSURE_CONFIG(vty);
    vty_out(vty,"PRIME DLMS over TCP port %d\r\n",TCPport);

	  return CMD_SUCCESS;
}

DEFUN (prime_show_dlmsotcp_config,
       prime_show_dlmsotcp_config_cmd,
       "show dlmsotcp config",
       PRIME_SHOW_STR
       "PRIME DLMSoTCP\n"
       "PRIME DLMSoTCP Configuration\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
/*********************************************
*       Code                                 *
**********************************************/
    vty_out(vty,"PRIME DLMSoTCP Configuration:\r\n"                          \
                "\r * Enabled     : %s\r\n"                                  \
                "\r * TCP Port    : %d\r\n", g_st_config.dlmsotcp_mode? "Yes":"No", prime_dlmsotcp_get_port());
	  return CMD_SUCCESS;
}

DEFUN (prime_config_mode,
       prime_config_mode_cmd,
       "config mode (base|service)",
       "PRIME Configuration\n"
       "PRIME Configuration mode\n"
       "PRIME Base Configuration\n"
       "PRIME Service Configuration\n")
{
    char info[256];

    if (strcmp(argv[0],"base") == 0){
       /* Run Base Daemon with saved configuration */
       prime_node_main(0, NULL);
    }else{
       /* Run Service Daemon with saved configuration */
       //prime_service_main(0, NULL);
    }

    // Write File Config
    sprintf(info, "config mode ");
    config_del_line_byleft(prime_config, info);
    sprintf(info, "config mode %s", argv[0]);
    config_add_line(prime_config, info);
    ENSURE_CONFIG(vty);
    vty_out(vty,"PRIME Config Mode %s\r\n",argv[0]);
    return CMD_SUCCESS;
}

DEFUN (prime_config_modemport_serial,
       prime_config_modemport_serial_cmd,
       "config modemport serial SERIAL_PORT speed SPEED",
       "Configuration\n"
       "PRIME Modem connection\n"
       "Serial Port connection\n"
       "Serial Port Device (default:ttyS1)\n"
       "Serial Port Speed Configuration\n"
       "Serial Port Speed\n")
{
    char info[256];
    uint32_t speed;

    // Serial Port Device where adp_mac_serialized is connected
    sprintf(info, "/dev/%s",argv[0]);
    if (access(info, F_OK) == -1 ){
      vty_out(vty,"Invalid device:  %s\r\n", info);
      return CMD_ERR_NOTHING_TODO;
    }
    speed = (uint32_t) atoi(argv[1]);
    switch (speed){
      case 230400:
      case 115200:
      case 57600:
      case 38400:
      case 19200:
        break;
      default:
        vty_out(vty,"Invalid speed:  %d\r\n", speed);
        return CMD_ERR_NOTHING_TODO;
    }
    memset(g_st_config.sz_tty_name,0,32);
    memcpy(g_st_config.sz_tty_name,info,31);
    g_st_config.sz_tty_speed = speed;
    g_st_config.sz_port_type = 0;

    // Update Information to be saved
    //config_write_prime();
    // Write File Config
    sprintf(info, "config modemport");
    config_del_line_byleft(prime_config, info);
    sprintf(info, "config modemport serial %s speed %s", argv[0], argv[1]);
    config_add_first_line(prime_config, info);
    //ENSURE_CONFIG(vty);
    vty_out(vty,"PRIME Modem connected to serial port %s at %s bps\r\n",argv[0], argv[1]);
    return CMD_SUCCESS;
}

DEFUN (prime_config_modemport_tcp,
       prime_config_modemport_tcp_cmd,
       "config modemport tcp A.B.C.D port PORT",
       "Configuration\n"
       "PRIME Modem connection\n"
       "TCP connection\n"
       "IP Address value\n"
       "TCP Port\n"
       "TCP Port value\n")
{
    char info[256];

    memset(g_st_config.sz_hostname, '\0',16);
    memcpy(g_st_config.sz_hostname,argv[0],16);
    g_st_config.sz_tcp_port = atoi(argv[1]);
    g_st_config.sz_port_type = 6; /* TCP_TYPE */

    // Write File Config
    sprintf(info, "config modemport");
    config_del_line_byleft(prime_config, info);
    sprintf(info, "config modemport tcp %s port %s", argv[0], argv[1]);
    config_add_first_line(prime_config, info);
    ENSURE_CONFIG(vty);
    vty_out(vty,"PRIME Modem connection to TCP host %s port %s\r\n",argv[0], argv[1]);
    return CMD_SUCCESS;
}

DEFUN (prime_config_tx_channel,
       prime_config_tx_channel_cmd,
       "config tx_channel <1-8>",
       "PRIME configuration\n"
       "PRIME TX channel configuration\n"
       "Prime TX channel\n")
{
    char    info[256];
    struct  TmacSetConfirm x_pib_confirm;
    uint8_t uc_value;
    uint8_t band_plan, tx_channel;

    uc_value = (uint8_t)atoi(argv[0]);

    /* Get Band Plan and TXchannel from PRIME Modem */
    tx_channel = prime_get_tx_channel();
    band_plan  = prime_get_band_plan();
    // Check if available on Band Plan
    if (!(band_plan & (1 << (uc_value-1)))){
      vty_out(vty,"TX Channel %d not allowed on PRIME Band Plan\r\n",uc_value);
      return CMD_ERR_NOTHING_TODO;
    }
    /* Set TX channel only if different than read from PRIME Modem */
    if (tx_channel != uc_value){
      // Configure PRIME Channel
      prime_cl_null_plme_set_request_sync(PIB_PHY_TX_CHANNEL,&uc_value,1,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
      if (x_pib_confirm.m_u8Status != PLME_RESULT_SUCCESS){
         vty_out(vty,"Failed to configure PRIME PIB-PHY TX Channel %d\r\n",uc_value);
         return CMD_ERR_NOTHING_TODO;
      }
      g_st_config.tx_channel = uc_value;
    }

    // Write File Config
    sprintf(info, "config tx_channel");
    config_del_line_byleft(prime_config, info);
    sprintf(info, "config tx_channel %s", argv[0]);
    config_add_line(prime_config, info);
    ENSURE_CONFIG(vty);
    vty_out(vty,"PRIME TX Channel %s configured\r\n",argv[0]);
	  return CMD_SUCCESS;
}

DEFUN (prime_show_security_profile,
       prime_show_security_profile_cmd,
       "show sec_profile",
       "Show PRIME configuration\n"
       "PRIME Security Profile configuration\n"
       "Security Profile value\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
/*********************************************
*       Code                                 *
**********************************************/
    // Configure PRIME Security Profile
    VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
    vty_out(vty,"PRIME Security Profile: %d\r\n",g_st_config.sec_profile);
	  return CMD_SUCCESS;
}

/**
 * \brief Get phyStatsBlkAvgEvm PIB Attribute
 *        Number of bursts received on the PHY layer for which the CRC was incorrect.
 */
DEFUN (prime_pib_phy_get_phyStatsCRCIncorrectCount,
       prime_pib_phy_get_phyStatsCRCIncorrectCount_cmd,
       "pib phy get (plme|mngp) phyStatsCRCIncorrectCount",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "phyStatsCRCIncorrectCount\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct   TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_STATS_CRC_INCORRECT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_PHY_STATS_CRC_INCORRECT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      vty_out(vty,"PRIME PIB-PHY phyStatsCRCIncorrectCount: %u\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsCRCIncorrectCount\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_phyStatsCRCIncorrectCount,
       prime_bmng_pib_phy_get_phyStatsCRCIncorrectCount_cmd,
       "pib phy get bmng MAC phyStatsCRCIncorrectCount",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "phyStatsCRCIncorrectCount\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct   TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_STATS_CRC_INCORRECT,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      vty_out(vty,"PRIME PIB-PHY phyStatsCRCIncorrectCount: %u\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsCRCIncorrectCount\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get phyStatsCRCFailCount PIB Attribute
 *        Number of bursts received on the PHY layer for which the CRC was correct,
 *        but the Protocol field of PHY header had an invalid value.
 *        This count would reflect number of times corrupt data was received and the
 *        CRC calculation failed to detect it.
 */
DEFUN (prime_pib_phy_get_phyStatsCRCFailCount,
       prime_pib_phy_get_phyStatsCRCFailCount_cmd,
       "pib phy get (plme|mngp) phyStatsCRCFailCount",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "phyStatsCRCFailCount\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct   TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_STATS_CRC_FAIL_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_PHY_STATS_CRC_FAIL_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      vty_out(vty,"PRIME PIB-PHY phyStatsCRCFailCount: %u\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsCRCFailCount\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_phyStatsCRCFailCount,
       prime_bmng_pib_phy_get_phyStatsCRCFailCount_cmd,
       "pib phy get bmng MAC phyStatsCRCFailCount",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "phyStatsCRCFailCount\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct   TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_STATS_CRC_FAIL_COUNT,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      vty_out(vty,"PRIME PIB-PHY phyStatsCRCFailCount: %u\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsCRCFailCount\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get phyStatsTxDropCount PIB Attribute
 *        Number of times when PHY layer received new data to transmit (PHY_DATA.request)
 *        and had to either overwrite on existing data in its transmit queue or drop the
 *        data in new request due to full queue.
 */
DEFUN (prime_pib_phy_get_phyStatsTxDropCount,
       prime_pib_phy_get_phyStatsTxDropCount_cmd,
       "pib phy get (plme|mngp) phyStatsTxDropCount",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "phyStatsTxDropCount\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct   TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_STATS_TX_DROP_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_PHY_STATS_TX_DROP_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      vty_out(vty,"PRIME PIB-PHY phyStatsTxDropCount: %u\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsTxDropCount\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_phyStatsTxDropCount,
       prime_bmng_pib_phy_get_phyStatsTxDropCount_cmd,
       "pib phy get bmng MAC phyStatsTxDropCount",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "phyStatsTxDropCount\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct   TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_STATS_TX_DROP_COUNT,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      vty_out(vty,"PRIME PIB-PHY phyStatsTxDropCount: %u\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsTxDropCount\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get phyStatsRxDropCount PIB Attribute
 *        Number of times when PHY layer received new data on the channel and
 *        had to either overwrite on existing data in its receive queue or drop
 *        the newly received data due to full queue.
 */
DEFUN (prime_pib_phy_get_phyStatsRxDropCount,
       prime_pib_phy_get_phyStatsRxDropCount_cmd,
       "pib phy get (plme|mngp) phyStatsRxDropCount",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "phyStatsRxDropCount\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct   TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_STATS_RX_DROP_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_PHY_STATS_RX_DROP_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      vty_out(vty,"PRIME PIB-PHY phyStatsRxDropCount: %u\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsRxDropCount\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_phyStatsRxDropCount,
       prime_bmng_pib_phy_get_phyStatsRxDropCount_cmd,
       "pib phy get bmng MAC phyStatsRxDropCount",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "phyStatsRxDropCount\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct   TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_STATS_RX_DROP_COUNT,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      vty_out(vty,"PRIME PIB-PHY phyStatsRxDropCount: %u\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsRxDropCount\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get phyStatsRxTotalCount PIB Attribute
 *        Number of times when PHY layer received new data on the channel and
 *        had to either overwrite on existing data in its receive queue or drop
 *        the newly received data due to full queue.
 */
DEFUN (prime_pib_phy_get_phyStatsRxTotalCount,
       prime_pib_phy_get_phyStatsRxTotalCount_cmd,
       "pib phy get (plme|mngp) phyStatsRxTotalCount",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "phyStatsRxTotalCount\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct   TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_STATS_RX_TOTAL_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_PHY_STATS_RX_TOTAL_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&ui_value,x_pib_confirm.m_au8AttributeValue,4);
      vty_out(vty,"PRIME PIB-PHY phyStatsRxTotalCount: %u\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsRxTotalCount\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_phyStatsRxTotalCount,
       prime_bmng_pib_phy_get_phyStatsRxTotalCount_cmd,
       "pib phy get bmng MAC phyStatsRxTotalCount",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "phyStatsRxTotalCount\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct   TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_STATS_RX_TOTAL_COUNT,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&ui_value,x_pib_confirm.m_au8AttributeValue,4);
      vty_out(vty,"PRIME PIB-PHY phyStatsRxTotalCount: %u\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsRxTotalCount\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get phyStatsBlkAvgEvm PIB Attribute
 *        Exponential moving average of the EVM over the past 16 PPDUs, as returned by the
 *        PHY_SNR primitive. Note that the PHY_SNR primitive returns a 3-bit number in dB
 *        scale. So first each 3-bit dB number is converted to linear scale (number k goes
 *        to 2^(k/2)), yielding a 7 bit number with 3 fractional bits.
 *        The result is just accumulated over 16 PPDUs and reported.
 */
DEFUN (prime_pib_phy_get_phyStatsBlkAvgEvm,
       prime_pib_phy_get_phyStatsBlkAvgEvm_cmd,
       "pib phy get (plme|mngp) phyStatsBlkAvgEvm",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Stats Block Average EVM\r\n"
       "Service Node EUI48\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct   TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_STATS_BLK_AVG_EVM,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_PHY_STATS_BLK_AVG_EVM,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      vty_out(vty,"PRIME PIB-PHY phyStatsBlkAvgEvm: %u\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsBlkAvgEvm\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_phyStatsBlkAvgEvm,
       prime_bmng_pib_phy_get_phyStatsBlkAvgEvm_cmd,
       "pib phy get bmng MAC phyStatsBlkAvgEvm",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "Stats Block Average EVM\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct   TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_STATS_BLK_AVG_EVM,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      vty_out(vty,"PRIME PIB-PHY phyStatsBlkAvgEvm: %u\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsBlkAvgEvm\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get phyEmaSmoothing PIB Attribute
 *        Smoothing factor divider for values that are updated as exponential moving average
 *        (EMA). Next value is Vnext = S*NewSample+(1–S)*Vprev
 *        Where S=1/(2^phyEMASmoothing).
 */
DEFUN (prime_pib_phy_get_phyEmaSmoothing,
       prime_pib_phy_get_phyEmaSmoothing_cmd,
       "pib phy get (plme|mngp) phyEmaSmoothing",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Smoothing Factor Divider\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_EMA_SMOOTHING,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_PHY_EMA_SMOOTHING,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 1 Bytes should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-PHY phyEmaSmoothing: %u\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyEmaSmoothing\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_phyEmaSmoothing,
       prime_bmng_pib_phy_get_phyEmaSmoothing_cmd,
       "pib phy get bmng MAC phyEmaSmoothing",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "Smoothing Factor Divider\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_EMA_SMOOTHING,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 1 Bytes should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-PHY phyEmaSmoothing: %u\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyEmaSmoothing\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get phyTxQueueLen PIB Attribute
 *        Number of concurrent MPDUs that the PHY transmit buffers can hold.
 */
DEFUN (prime_pib_phy_get_phyTxQueueLen,
       prime_pib_phy_get_phyTxQueueLen_cmd,
       "pib phy get (plme|mngp) phyTxQueueLen",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "phyTxQueueLen\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_TX_QUEUE_LEN,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_PHY_TX_QUEUE_LEN,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      us_value &= 0x3F;
      vty_out(vty,"PRIME PIB-PHY phyTxQueueLen: %u\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyTxQueueLen\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_phyTxQueueLen,
       prime_bmng_pib_phy_get_phyTxQueueLen_cmd,
       "pib phy get bmng MAC phyTxQueueLen",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "phyTxQueueLen\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_TX_QUEUE_LEN,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);

  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      us_value &= 0x3F;
      vty_out(vty,"PRIME PIB-PHY phyTxQueueLen: %u\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyTxQueueLen\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get phyRxQueueLen PIB Attribute
 *        Number of concurrent MPDUs that the PHY receive buffers can hold.
 */
DEFUN (prime_pib_phy_get_phyRxQueueLen,
       prime_pib_phy_get_phyRxQueueLen_cmd,
       "pib phy get (plme|mngp) phyRxQueueLen",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "phyRxQueueLen\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
      prime_cl_null_plme_get_request_sync(PIB_PHY_RX_QUEUE_LEN,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
    prime_mngp_get_request_sync(PIB_PHY_RX_QUEUE_LEN,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      us_value &= 0x03FF;
      vty_out(vty,"PRIME PIB-PHY phyRxQueueLen: %u\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyRxQueueLen\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_phyRxQueueLen,
       prime_bmng_pib_phy_get_phyRxQueueLen_cmd,
       "pib phy get bmng MAC phyRxQueueLen",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "phyRxQueueLen\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_RX_QUEUE_LEN,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);

  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Bytes should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      us_value &= 0x03FF;
      vty_out(vty,"PRIME PIB-PHY phyRxQueueLen: %u\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyRxQueueLen\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get phyTxProcessingDelay PIB Attribute
 *        Time elapsed from the instance when data is received on MAC-PHY communication
 *        interface to the time when it is put on the physical channel. This shall not
 *        include communication delay over the MAC-PHY interface. Value of this attribute
 *        is in unit of microseconds.
 */
DEFUN (prime_pib_phy_get_phyTxProcessingDelay,
       prime_pib_phy_get_phyTxProcessingDelay_cmd,
       "pib phy get (plme|mngp) phyTxProcessingDelay",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "phyTxProcessingDelay\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_TX_PROCESSING_DELAY,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_PHY_TX_PROCESSING_DELAY,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 4 Bytes should be the result
      memcpy(&ui_value,x_pib_confirm.m_au8AttributeValue,4);
      ui_value &= 0x0FFFFF;
      vty_out(vty,"PRIME PIB-PHY phyTxProcessingDelay: %u\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyTxProcessingDelay\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_phyTxProcessingDelay,
       prime_bmng_pib_phy_get_phyTxProcessingDelay_cmd,
       "pib phy get bmng MAC phyTxProcessingDelay",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "phyTxProcessingDelay\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_TX_PROCESSING_DELAY,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 4 Bytes should be the result
      memcpy(&ui_value,x_pib_confirm.m_au8AttributeValue,4);
      ui_value &= 0x0FFFFF;
      vty_out(vty,"PRIME PIB-PHY phyTxProcessingDelay: %u\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyTxProcessingDelay\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get phyRxProcessingDelay PIB Attribute
 *        Time elapsed from the instance when data is received on physical channel
 *        to the time when it is made available to MAC across the MAC-PHY communication
 *        interface. This shall not include communication delay over the MAC-PHY interface.
 *        Value of this attribute is in unit of microseconds.
 */
DEFUN (prime_pib_phy_get_phyRxProcessingDelay,
       prime_pib_phy_get_phyRxProcessingDelay_cmd,
       "pib phy get (plme|mngp) phyRxProcessingDelay",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "phyRxProcessingDelay\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_RX_PROCESSING_DELAY,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
    prime_mngp_get_request_sync(PIB_PHY_RX_PROCESSING_DELAY,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 4 Bytes should be the result
      memcpy(&ui_value,x_pib_confirm.m_au8AttributeValue,4);
      ui_value &= 0x0FFFFF;
      vty_out(vty,"PRIME PIB-PHY phyRxProcessingDelay: %u\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyRxProcessingDelay\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_phyRxProcessingDelay,
       prime_bmng_pib_phy_get_phyRxProcessingDelay_cmd,
       "pib phy get bmng MAC phyRxProcessingDelay",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "phyRxProcessingDelay\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_RX_PROCESSING_DELAY,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);

  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 4 Bytes should be the result
      memcpy(&ui_value,x_pib_confirm.m_au8AttributeValue,4);
      ui_value &= 0x0FFFFF;
      vty_out(vty,"PRIME PIB-PHY phyRxProcessingDelay: %u\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyRxProcessingDelay\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get phyAgcMinGain PIB Attribute
 *        Minimum gain for the AGC <= 0dB
 */
DEFUN (prime_pib_phy_get_phyAgcMinGain,
       prime_pib_phy_get_phyAgcMinGain_cmd,
       "pib phy get (plme|mngp) phyAgcMinGain",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Service Node EUI48\r\n"
       "phyAgcMinGain\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_AGC_MIN_GAIN,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
    prime_mngp_get_request_sync(PIB_PHY_AGC_MIN_GAIN,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-PHY phyAgcMinGain: %u\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyAgcMinGain\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_phyAgcMinGain,
       prime_bmng_pib_phy_get_phyAgcMinGain_cmd,
       "pib phy get bmng MAC phyAgcMinGain",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "phyAgcMinGain\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_AGC_MIN_GAIN,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-PHY phyAgcMinGain: %u\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyAgcMinGain\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get phyAgcStepValue PIB Attribute
 *        Distance between steps in dB <= 6dB
 */
DEFUN (prime_pib_phy_get_phyAgcStepValue,
       prime_pib_phy_get_phyAgcStepValue_cmd,
       "pib phy get (plme|mngp) phyAgcStepValue",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "phyAgcStepValue\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_AGC_STEP_VALUE,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
    prime_mngp_get_request_sync(PIB_PHY_AGC_STEP_VALUE,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      uc_value &= 0x07;
      vty_out(vty,"PRIME PIB-PHY phyAgcStepValue: %u\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyAgcStepValue\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_phyAgcStepValue,
       prime_bmng_pib_phy_get_phyAgcStepValue_cmd,
       "pib phy get bmng MAC phyAgcStepValue",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "phyAgcStepValue\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_AGC_STEP_VALUE,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      uc_value &= 0x07;
      vty_out(vty,"PRIME PIB-PHY phyAgcStepValue: %u\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyAgcStepValue\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get phyAgcStepNumber PIB Attribute
 *        Number of steps so that phyAgcMinGain+((phyAgcStepNumber–1)*phyAgcStepValue) >= 21dB
 */
DEFUN (prime_pib_phy_get_phyAgcStepNumber,
       prime_pib_phy_get_phyAgcStepNumber_cmd,
       "pib phy get (plme|mngp) phyAgcStepNumber",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "phyAgcStepNumber")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_AGC_STEP_NUMBER,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
    prime_mngp_get_request_sync(PIB_PHY_AGC_STEP_NUMBER,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-PHY phyAgcStepNumber: %u\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyAgcStepNumber\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_phyAgcStepNumber,
       prime_bmng_pib_phy_get_phyAgcStepNumber_cmd,
       "pib phy get bmng MAC phyAgcStepNumber",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "phyAgcStepNumber\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_AGC_STEP_NUMBER,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-PHY phyAgcStepNumber: %u\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY phyAgcStepNumber\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get phy_sw_version PIB Attribute
 *        Get PHY Software Version
 */
DEFUN (prime_pib_phy_get_sw_version,
       prime_pib_phy_get_sw_version_cmd,
       "pib phy get (plme|mngp) sw_version",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "PHY Software version\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_SW_VERSION,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_PHY_SW_VERSION,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4) ;
      vty_out(vty,"PRIME PIB-MTP-PHY sw_version: 0x%08X\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MTP-PHY sw_version\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_sw_version,
       prime_bmng_pib_phy_get_sw_version_cmd,
       "pib phy get bmng MAC sw_version",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "PHY Software version\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_SW_VERSION,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4) ;
      vty_out(vty,"PRIME PIB-MTP-PHY sw_version: 0x%08X\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MTP-PHY sw_version\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get ZCT PIB Attribute
 *        Time between the zero cross of the mains and the end of the last transmission or reception.
 */
DEFUN (prime_pib_phy_get_zct,
       prime_pib_phy_get_zct_cmd,
       "pib phy get (plme|mngp) zct",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "PHY Zero Cross Time\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_ZCT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
    prime_mngp_get_request_sync(PIB_PHY_ZCT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4) ;
      vty_out(vty,"PRIME PIB-MTP-PHY ZCT: %u\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MTP-PHY ZCT\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_zct,
       prime_bmng_pib_phy_get_zct_cmd,
       "pib phy get bmng MAC zct",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "PHY Zero Cross Time\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_ZCT,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);

  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4) ;
      vty_out(vty,"PRIME PIB-MTP-PHY ZCT: %u\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MTP-PHY ZCT\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get PHY Host Version PIB Attribute
 *        PL360 Host Controller version.
 */
DEFUN (prime_pib_phy_get_phy_host_version,
       prime_pib_phy_get_host_version_cmd,
       "pib phy get (plme|mngp) host_version",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "HOST version\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_HOST_VERSION,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_PHY_HOST_VERSION,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4) ;
      vty_out(vty,"PRIME PIB-PHY HOST version: 0x%08X\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY HOST version\r\n");
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_phy_get_phy_host_version,
       prime_bmng_pib_phy_get_host_version_cmd,
       "pib phy get bmng MAC host_version",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "HOST version\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
    vty_out(vty,"Service Node EUI48 length is wrong\r\n");
    return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
    vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
    return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_request_sync(mac,PIB_PHY_HOST_VERSION,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4) ;
      vty_out(vty,"PRIME PIB-PHY HOST version: 0x%08X\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY HOST version\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get PHY Channel PIB Attribute
 *        Transmission/Reception channel, only when hardware permits multichannel.
 *        The channel depends on the selected PLC coupling
 */
DEFUN (prime_pib_phy_get_tx_channel,
       prime_pib_phy_get_tx_channel_cmd,
       "pib phy get (plme|mngp) tx_channel",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "PRIME PHY TX Channel (1-8)\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_TX_CHANNEL,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
    prime_mngp_get_request_sync(PIB_PHY_TX_CHANNEL,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-PHY channel %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY channel\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set PHY Channel
 *        Transmission/Reception channel, only when hardware permits multichannel.
 *        The channel depends on the selected PLC coupling
 */
DEFUN (prime_pib_phy_set_tx_channel,
       prime_pib_phy_set_tx_channel_cmd,
       "pib phy set (plme|mngp) tx_channel <1-8>",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_SET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "PRIME PHY TX Channel\n"
       "PRIME PHY TX Channel value\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  uc_value = (uint8_t)atoi(argv[1]);

  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_set_request_sync(PIB_PHY_TX_CHANNEL,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_PHY_TX_CHANNEL,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      vty_out(vty,"PRIME PIB-PHY Channel %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-PHY channel %d\r\n",uc_value);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get TX/RX Channel List PIB Attribute
 *        Get the list of available channels. It has the same structure as the band plan.
 */
DEFUN (prime_pib_phy_get_txrx_channel_list,
       prime_pib_phy_get_txrx_channel_list_cmd,
       "pib phy get (plme|mngp) txrx_channel_list",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "PRIME PHY TX-RX Channel List\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_TXRX_CHANNEL_LIST,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
    prime_mngp_get_request_sync(PIB_PHY_TXRX_CHANNEL_LIST,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-PHY TX-RX channel list %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY TX-RX channel list\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get PHY Sniffer mode PIB Attribute
 *        Enable/disable the sniffer [0: disabled, 1: enabled].
 */
DEFUN (prime_pib_phy_get_sniffer_mode,
       prime_pib_phy_get_sniffer_mode_cmd,
       "pib phy get (plme|mngp) sniffer_mode",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "PRIME Sniffer Mode\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_PHY_SNIFFER_ENABLED,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_PHY_SNIFFER_ENABLED,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-PHY Sniffer mode %s\r\n",uc_value == 1?"enabled":"disabled");
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY Sniffer mode\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set Sniffer Mode
 *        Enable/Disable the Sniffer mode
 */
DEFUN (prime_pib_phy_set_sniffer_mode,
       prime_pib_phy_set_sniffer_mode_cmd,
       "pib phy set (plme|mngp) sniffer_mode (enable|disable)",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_SET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "PRIME Sniffer Mode\n"
       "Sniffer Mode Enabled\n"
       "Sniffer Mode Disabled\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[1],"enable") == 0){
    uc_value = 1;
  }else{
    uc_value = 0;
  }
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_set_request_sync(PIB_PHY_SNIFFER_ENABLED,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_PHY_SNIFFER_ENABLED,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      vty_out(vty,"PRIME PIB-PHY Sniffer Mode %sd\r\n",argv[0]);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-PHY Sniffer Mode %sd\r\n",argv[0]);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get PHY MTP mode PIB Attribute
 *        Get PHY MTP mode.
 */
DEFUN (prime_pib_phy_get_mtp_mode,
       prime_pib_phy_get_mtp_mode_cmd,
       "pib phy get (plme|mngp) mtp_mode",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "PRIME MTP Mode\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_MTP_PHY_ENABLE,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MTP_PHY_ENABLE,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      // Update MTP Mode Status
      g_st_config.mtp_mode = uc_value;
      vty_out(vty,"PRIME PIB-PHY MTP mode %s\r\n", uc_value == 1? "enabled" : "disabled");
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-PHY MTP mode\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set MTP Mode
 *        Enable/Disable the MTP mode
 */
DEFUN (prime_pib_phy_set_mtp_mode,
       prime_pib_phy_set_mtp_mode_cmd,
       "pib phy set (plme|mngp) mtp_mode (enable|disable)",
       REQUEST_PIB
       REQUEST_PHY_PIB
       REQUEST_SET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "PRIME MTP Mode\n"
       "MTP Mode Enabled\n"
       "MTP Mode Disabled\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[1],"enable") == 0){
    uc_value = 1;
  }else{
    uc_value = 0;
  }
  if (uc_value){
     if (strcmp(argv[0],"plme") == 0){
        prime_cl_null_plme_set_request_sync(PIB_MTP_PHY_ENABLE,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
     }else{
        prime_mngp_set_request_sync(PIB_MTP_PHY_ENABLE,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
     }
     if (x_pib_confirm.m_u8Status != PLME_RESULT_SUCCESS){
        vty_out(vty,"Impossible to set PRIME PIB-PHY MTP Mode %sd\r\n",argv[0]);
        return CMD_ERR_NOTHING_TODO;
    }
  }else{
     // When disabling MTP mode, no answer from Modem => Asynchronous call is needed
     if (strcmp(argv[0],"plme") == 0){
        prime_cl_null_plme_set_request(PIB_MTP_PHY_ENABLE,&uc_value,1);
     }else{
        prime_mngp_set_request(PIB_MTP_PHY_ENABLE,&uc_value,1);
     }
     if (g_st_config.sniffer_mode){
        // Enable again Sniffer Embedded
        prime_embedded_sniffer(g_st_config.sniffer_mode);
     }
  }
  // Update MTP Mode Status
  g_st_config.mtp_mode = uc_value;
  vty_out(vty,"PRIME PIB-PHY MTP Mode %sd\r\n",argv[0]);
  return CMD_SUCCESS;

}

/**
 * \brief Get cfg_load_threshold1 PIB Attribute
 *        Load threshold to change the impedance in branch 1.
 */
DEFUN (prime_pib_mtp_get_phy_cfg_load_threshold1,
       prime_pib_mtp_get_phy_cfg_load_threshold1_cmd,
       "pib mtp get (plme|mngp) cfg_load_threshold1",
       REQUEST_PIB
       REQUEST_MTP_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Configuration Load Threshold 1\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_MTP_PHY_CFG_LOAD_THRESHOLD_1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MTP_PHY_CFG_LOAD_THRESHOLD_1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Byte should be the result
      memcpy(&us_value,&x_pib_confirm.m_au8AttributeValue[0],2) ;
      vty_out(vty,"PRIME PIB-MTP-PHY cfg_load_threshold1: %d\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MTP-PHY cfg_load_threshold1\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set cfg_load_threshold1 PIB Attribute
 *        Set threshold to change the impedance in branch 1.
 */
DEFUN (prime_pib_mtp_set_phy_cfg_load_threshold1,
       prime_pib_mtp_set_phy_cfg_load_threshold1_cmd,
       "pib mtp set (plme|mngp) cfg_load_threshold1 <0-65536>",
       REQUEST_PIB
       REQUEST_MTP_PIB
       REQUEST_SET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Configuration Load Threshold 1\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/

  VTY_CHECK_PRIME_MTP_MODE()

  us_value = (uint16_t) atoi(argv[1]);
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_set_request_sync(PIB_MTP_PHY_CFG_LOAD_THRESHOLD_1, (uint8_t *) &us_value,2,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MTP_PHY_CFG_LOAD_THRESHOLD_1, (uint8_t *)&us_value,2,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      vty_out(vty,"PRIME PIB-MTP-PHY cfg_load_threshold1 set to %d\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MTP-PHY cfg_load_threshold1\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get cfg_load_threshold2 PIB Attribute
 *        Load threshold to change the impedance in branch 2.
 */
DEFUN (prime_pib_mtp_get_phy_cfg_load_threshold2,
       prime_pib_mtp_get_phy_cfg_load_threshold2_cmd,
       "pib mtp get (plme|mngp) cfg_load_threshold2",
       REQUEST_PIB
       REQUEST_MTP_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Configuration Load Threshold 2\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_get_request_sync(PIB_MTP_PHY_CFG_LOAD_THRESHOLD_2,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MTP_PHY_CFG_LOAD_THRESHOLD_2,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Byte should be the result
      memcpy(&us_value,&x_pib_confirm.m_au8AttributeValue[0],2) ;
      vty_out(vty,"PRIME PIB-MTP-PHY cfg_load_threshold2: %d\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MTP-PHY cfg_load_threshold2\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set cfg_load_threshold2 PIB Attribute
 *        Set threshold to change the impedance in branch 2.
 */
DEFUN (prime_pib_mtp_set_phy_cfg_load_threshold2,
       prime_pib_mtp_set_phy_cfg_load_threshold2_cmd,
       "pib mtp set (plme|mngp) cfg_load_threshold2 <0-65536>",
       REQUEST_PIB
       REQUEST_MTP_PIB
       REQUEST_SET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Configuration Load Threshold 2\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MTP_MODE()

  us_value = (uint16_t) atoi(argv[1]);
  if (strcmp(argv[0],"plme") == 0){
     prime_cl_null_plme_set_request_sync(PIB_MTP_PHY_CFG_LOAD_THRESHOLD_2,(uint8_t *) &us_value,2,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MTP_PHY_CFG_LOAD_THRESHOLD_2,(uint8_t *) &us_value,2,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      vty_out(vty,"PRIME PIB-MTP-PHY cfg_load_threshold2 set to %d\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MTP-PHY cfg_load_threshold2\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get PHY TX Time PIB Attribute
 *        Transmission time of the last transmitted frame in ten of μs.
 */
DEFUN (prime_pib_mtp_get_phy_tx_time,
       prime_pib_mtp_get_phy_tx_time_cmd,
       "pib mtp get (plme|mngp) tx_time",
       REQUEST_PIB
       REQUEST_MTP_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "TX Time\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
   if (strcmp(argv[0],"plme") == 0){
      prime_cl_null_plme_get_request_sync(PIB_MTP_PHY_TX_TIME,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   }else{
      prime_mngp_get_request_sync(PIB_MTP_PHY_TX_TIME,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   }
   if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4) ;
      vty_out(vty,"PRIME PIB-MTP-PHY tx_time (tens of us): %u\r\n",ui_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MTP-PHY tx_time\r\n");
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get PHY RMS Calc Corrected PIB Attribute
 *        RMS value of the last PLC signal.
 */
DEFUN (prime_pib_mtp_get_phy_rms_calc_corrected,
       prime_pib_mtp_get_phy_rms_calc_corrected_cmd,
       "pib mtp get (plme|mngp) rms_calc_corrected",
       REQUEST_PIB
       REQUEST_MTP_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "RMS Calc Corrected\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  prime_cl_null_plme_get_request_sync(PIB_MTP_PHY_RMS_CALC_CORRECTED,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 2 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4) ;
      vty_out(vty,"PRIME PIB-MTP-PHY RMS calc corrected: %d\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MTP-PHY tx_time\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get PHY continuous TX PIB Attribute
 *        Set the PHY layer to transmit continuously [0: disabled, 1: enabled].
 */
DEFUN (prime_pib_mtp_get_phy_continuous_tx,
       prime_pib_mtp_get_phy_continuous_tx_cmd,
       "pib mtp get phy continuous_tx",
       "PRIME PIB\n"
       "PRIME MTP\n"
       "PRIME get PIB\n"
       "PRIME PHY PIB\n"
       "Configuration of Continuous Transmission\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  prime_cl_null_plme_get_request_sync(PIB_MTP_PHY_CONTINUOUS_TX,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MTP-PHY continuous_tx %s\r\n",uc_value==0?"disabled":"enabled");
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MTP-PHY continuous_tx\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set PHY Continuous TX mode
 *        Set PHY Continuous TX mode
 */
DEFUN (prime_pib_mtp_set_phy_continuous_tx,
       prime_pib_mtp_set_phy_continuous_tx_cmd,
       "pib mtp set phy continuous_tx (enable|disable)",
       "PRIME PIB\n"
       "PRIME MTP\n"
       "PRIME set PIB\n"
       "PRIME PHY PIB\n"
       "Configuration of Continuous TX\n"
       "Enable Continuous Transmission\n"
       "Disable Continuous Transmission\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/

  VTY_CHECK_PRIME_MTP_MODE()

  if (strcmp(argv[0],"enable") == 0){
     uc_value = 1;
  }else{
     uc_value = 0;
  }
  prime_cl_null_plme_set_request_sync(PIB_MTP_PHY_CONTINUOUS_TX,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      vty_out(vty,"PRIME PIB-MTP-PHY continuous_tx set to %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MTP-PHY continuous_tx\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get PHY Auto Impedance Detection
 *        Enable/disable automatic selection of transmission mode [0: disabled, 1: enabled].
 */
DEFUN (prime_pib_mtp_get_phy_drv_auto,
       prime_pib_mtp_get_phy_drv_auto_cmd,
       "pib mtp get phy drv_auto",
       "PRIME PIB\n"
       "PRIME MTP\n"
       "PRIME get PIB\n"
       "PRIME PHY PIB\n"
       "Configuration of DRV auto\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  prime_cl_null_plme_get_request_sync(PIB_MTP_PHY_DRV_AUTO,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MTP-PHY DRV auto %s\r\n",uc_value==0?"disabled":"enabled");
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MTP-PHY DRV auto\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set PHY DRV auto mode
 *        Set PHY DRV auto mode
 */
DEFUN (prime_pib_mtp_set_phy_drv_auto,
       prime_pib_mtp_set_phy_drv_auto_cmd,
       "pib mtp set phy drv_auto (enable|disable)",
       "PRIME PIB\n"
       "PRIME MTP\n"
       "PRIME set PIB\n"
       "PRIME PHY PIB\n"
       "Configuration of DRV auto\n"
       "Enable DRV auto\n"
       "Disable DRV auto\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/

  VTY_CHECK_PRIME_MTP_MODE()

  if (strcmp(argv[0],"enable") == 0){
     uc_value = 1;
  }else{
     uc_value = 0;
  }
  prime_cl_null_plme_set_request_sync(PIB_MTP_PHY_DRV_AUTO,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      vty_out(vty,"PRIME PIB-MTP-PHY drv_auto %sd\r\n",argv[0]);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MTP-PHY drv_auto\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get PHY DRV Impedance
 *        Enable/disable the impedance mode [0: high, 1: low, 2: very low].
 */
DEFUN (prime_pib_mtp_get_phy_drv_impedance,
       prime_pib_mtp_get_phy_drv_impedance_cmd,
       "pib mtp get phy drv_impedance",
       "PRIME PIB\n"
       "PRIME MTP\n"
       "PRIME get PIB\n"
       "PRIME PHY PIB\n"
       "Configuration of DRV impedance\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  prime_cl_null_plme_get_request_sync(PIB_MTP_PHY_DRV_AUTO,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MTP-PHY DRV impedance %s\r\n",uc_value==0?"high":"very low");  // LOW MUST NOT BE AVAILABLE
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MTP-PHY DRV impedance\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set PHY DRV auto mode
 *        Set PHY DRV auto mode
 */
DEFUN (prime_pib_mtp_set_phy_drv_impedance,
       prime_pib_mtp_set_phy_drv_impedance_cmd,
       "pib mtp set phy drv_impedance (high|vlow)",
       "PRIME PIB\n"
       "PRIME MTP\n"
       "PRIME set PIB\n"
       "PRIME PHY PIB\n"
       "Configuration of DRV impedance\n"
       "DRV impedance HIGH\n"
       "DRV impedance VLOW\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/

  VTY_CHECK_PRIME_MTP_MODE()

  if (strcmp(argv[0],"high") == 0){
     uc_value = 0;
  }else{
     uc_value = 2;
  }
  prime_cl_null_plme_set_request_sync(PIB_MTP_PHY_DRV_IMPEDANCE,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      vty_out(vty,"PRIME PIB-MTP-PHY drv_impedance set to %s\r\n",argv[0]);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MTP-PHY drv_impedance\r\n");
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macMinSwitchSearchTime PIB Attribute
 *        Minimum time for which a Service Node in Disconnected status should
 *        scan the channel for Beacons before it can broadcast PNPDU.
 *        This attribute is not maintained in Base Nodes.
 */
DEFUN (prime_pib_mac_get_macMinSwitchSearchTime,
       prime_pib_mac_get_macMinSwitchSearchTime_cmd,
       "pib mac get (mlme|mngp) macMinSwitchSearchTime",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macMinSwitchSearchTime\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
  struct TmacGetConfirm x_pib_confirm;
  uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE()
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_MIN_SWITCH_SEARCH_TIME,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_MIN_SWITCH_SEARCH_TIME,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macMinSwitchSearchTime: %u\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macMinSwitchSearchTime (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macMinSwitchSearchTime,
       prime_bmng_pib_mac_get_macMinSwitchSearchTime_cmd,
       "pib mac get bmng MAC macMinSwitchSearchTime",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macMinSwitchSearchTime\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
  struct TmacGetConfirm x_pib_confirm;
  uint8_t uc_value;
  uint8_t  mac[6];
  prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }

   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_MIN_SWITCH_SEARCH_TIME,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);

   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macMinSwitchSearchTime: %u\r\n",uc_value);
      return CMD_SUCCESS;
  }

  vty_out(vty,"Impossible to get PRIME PIB-MAC macMinSwitchSearchTime (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set macMinSwitchSearchTime PIB Attribute
 *        Minimum time for which a Service Node in Disconnected status should
 *        scan the channel for Beacons before it can broadcast PNPDU.
 *        This attribute is not maintained in Base Nodes.
 */
DEFUN (prime_pib_mac_set_macMinSwitchSearchTime,
       prime_pib_mac_set_macMinSwitchSearchTime_cmd,
       "pib mac set (mlme|mngp) macMinSwitchSearchTime <16-32>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macMinSwitchSearchTime\n"
       "macMinSwitchSearchTime Value (16-32 default:24)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE()
  uc_value = (uint8_t) atoi(argv[1]);

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_MIN_SWITCH_SEARCH_TIME,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else {
     prime_mngp_set_request_sync(PIB_MAC_MIN_SWITCH_SEARCH_TIME,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macMinSwitchSearchTime set to %u\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MAC macMinSwitchSearchTime (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_set_macMinSwitchSearchTime,
       prime_bmng_pib_mac_set_macMinSwitchSearchTime_cmd,
       "pib mac set bmng MAC macMinSwitchSearchTime <16-32>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macMinSwitchSearchTime\n"
       "macMinSwitchSearchTime Value (16-32 default:24)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   uc_value = (uint8_t) atoi(argv[1]);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_set_request_sync(mac,PIB_MAC_MIN_SWITCH_SEARCH_TIME,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macMinSwitchSearchTime set to %u\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to set PRIME PIB-MAC macMinSwitchSearchTime (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macMaxPromotionPdu PIB Attribute
 *        Maximum number of PNPDUs that may be transmitted by a Service Node in
 *        a period of macPromotionPduTxPeriod seconds.
 *        This attribute is not maintained in Base Node.
 */
DEFUN (prime_pib_mac_get_macMaxPromotionPdu,
       prime_pib_mac_get_macMaxPromotionPdu_cmd,
       "pib mac get (mlme|mngp) macMaxPromotionPdu",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macMaxPromotionPdu\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE()
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_MAX_PROMOTION_PDU,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_MAX_PROMOTION_PDU,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macMaxPromotionPdu: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macMaxPromotionPdu (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macMaxPromotionPdu,
       prime_bmng_pib_mac_get_macMaxPromotionPdu_cmd,
       "pib mac get bmng MAC macMaxPromotionPdu",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macMaxPromotionPdu\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_MAX_PROMOTION_PDU,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macMaxPromotionPdu: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macMaxPromotionPdu (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set macMaxPromotionPdu PIB Attribute
 *        Maximum number of PNPDUs that may be transmitted by a Service Node in
 *        a period of macPromotionPduTxPeriod seconds.
 *        This attribute is not maintained in Base Node.
 */
DEFUN (prime_pib_mac_set_macMaxPromotionPdu,
       prime_pib_mac_set_macMaxPromotionPdu_cmd,
       "pib mac set (mlme|mngp) macMaxPromotionPdu <1-4>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macMaxPromotionPdu\n"
       "macMaxPromotionPdu Value (1-4 default:2)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE()
  uc_value = (uint8_t) atoi(argv[1]);
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_MAX_PROMOTION_PDU,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_MAX_PROMOTION_PDU,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macMaxPromotionPdu set to %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MAC macMaxPromotionPdu (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_set_macMaxPromotionPdu,
       prime_bmng_pib_mac_set_macMaxPromotionPdu_cmd,
       "pib mac set bmng MAC macMaxPromotionPdu <1-4>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macMaxPromotionPdu\n"
       "macMaxPromotionPdu Value (1-4 default:2)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   uc_value = (uint8_t) atoi(argv[1]);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_set_request_sync(mac,PIB_MAC_MAX_PROMOTION_PDU,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macMaxPromotionPdu set to %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to set PRIME PIB-MAC macMaxPromotionPdu (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macPromotionPduTxPeriod PIB Attribute
 *        Time quantum for limiting a number of PNPDUs transmitted from a Service Node.
 *        No more than macMaxPromotionPdu may be transmitted in a period of
 *        macPromotionPduTxPeriod seconds.
 */
DEFUN (prime_pib_mac_get_macPromotionPduTxPeriod,
       prime_pib_mac_get_macPromotionPduTxPeriod_cmd,
       "pib mac get (mlme|mngp) macPromotionPduTxPeriod",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macPromotionPduTxPeriod\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE()
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_PROMOTION_PDU_TX_PERIOD,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_PROMOTION_PDU_TX_PERIOD,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macPromotionPduTxPeriod: %d seconds\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macPromotionPduTxPeriod (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macPromotionPduTxPeriod,
       prime_bmng_pib_mac_get_macPromotionPduTxPeriod_cmd,
       "pib mac get bmng MAC macPromotionPduTxPeriod",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macPromotionPduTxPeriod\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_PROMOTION_PDU_TX_PERIOD,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);

   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macPromotionPduTxPeriod: %d seconds\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macPromotionPduTxPeriod (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set macPromotionPduTxPeriod PIB Attribute
 *        Time quantum for limiting a number of PNPDUs transmitted from a Service Node.
 *        No more than macMaxPromotionPdu may be transmitted in a period of
 *        macPromotionPduTxPeriod seconds.
 */
DEFUN (prime_pib_mac_set_macPromotionPduTxPeriod,
       prime_pib_mac_set_macPromotionPduTxPeriod_cmd,
       "pib mac set (mlme|mngp) macPromotionPduTxPeriod <2-8>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macPromotionPduTxPeriod\n"
       "macPromotionPduTxPeriod Value (2-8 default:5)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE()
  uc_value = (uint8_t) atoi(argv[1]);

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_PROMOTION_PDU_TX_PERIOD,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_PROMOTION_PDU_TX_PERIOD,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macPromotionPduTxPeriod set to %d seconds\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MAC macPromotionPduTxPeriod (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_set_macPromotionPduTxPeriod,
       prime_bmng_pib_mac_set_macPromotionPduTxPeriod_cmd,
       "pib mac set bmng MAC macPromotionPduTxPeriod <2-8>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macPromotionPduTxPeriod\n"
       "macPromotionPduTxPeriod Value (2-8 default:5)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   uc_value = (uint8_t) atoi(argv[1]);

   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_set_request_sync(mac,PIB_MAC_PROMOTION_PDU_TX_PERIOD,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macPromotionPduTxPeriod set to %d seconds\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to set PRIME PIB-MAC macPromotionPduTxPeriod (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macSCPMaxTxAttempts PIB Attribute
 *        Number of times the CSMA algorithm would attempt to transmit requested data
 *        when a previous attempt was withheld due to PHY indicating channel busy.
 */
DEFUN (prime_pib_mac_get_macSCPMaxTxAttempts,
       prime_pib_mac_get_macSCPMaxTxAttempts_cmd,
       "pib mac get (mlme|mngp) macSCPMaxTxAttempts",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macSCPMaxTxAttempts\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE()
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_SCP_MAX_TX_ATTEMPTS,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else if (strcmp(argv[0],"mngp") == 0){
          prime_mngp_get_request_sync(PIB_MAC_SCP_MAX_TX_ATTEMPTS,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }

  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macSCPMaxTxAttempts: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macSCPMaxTxAttempts (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macSCPMaxTxAttempts,
       prime_bmng_pib_mac_get_macSCPMaxTxAttempts_cmd,
       "pib mac get bmng MAC macSCPMaxTxAttempts",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macSCPMaxTxAttempts\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_SCP_MAX_TX_ATTEMPTS,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);

   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macSCPMaxTxAttempts: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macSCPMaxTxAttempts (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set macSCPMaxTxAttempts PIB Attribute
 *        Number of times the CSMA algorithm would attempt to transmit requested data
 *        when a previous attempt was withheld due to PHY indicating channel busy.
 */
DEFUN (prime_pib_mac_set_macSCPMaxTxAttempts,
       prime_pib_mac_set_macSCPMaxTxAttempts_cmd,
       "pib mac set (mlme|mngp) macSCPMaxTxAttempts <2-5>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "Service Node EUI48\r\n"
       "macSCPMaxTxAttempts\n"
       "macSCPMaxTxAttempts Value (2-5 default:5)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  uc_value = (uint8_t) atoi(argv[1]);
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_SCP_MAX_TX_ATTEMPTS,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_SCP_MAX_TX_ATTEMPTS,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macSCPMaxTxAttempts set to %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MAC macSCPMaxTxAttempts (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_set_macSCPMaxTxAttempts,
       prime_bmng_pib_mac_set_macSCPMaxTxAttempts_cmd,
       "pib mac set bmng MAC macSCPMaxTxAttempts <2-5>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macSCPMaxTxAttempts\n"
       "macSCPMaxTxAttempts Value (2-5 default:5)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   uc_value = (uint8_t) atoi(argv[1]);

   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_set_request_sync(mac,PIB_MAC_SCP_MAX_TX_ATTEMPTS,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
       vty_out(vty,"PRIME PIB-MAC macSCPMaxTxAttempts set to %d\r\n",uc_value);
       return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to set PRIME PIB-MAC macSCPMaxTxAttempts (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macMinCtlReTxTimer PIB Attribute
 *        Minimum number of seconds for which a MAC entity waits for acknowledgement
 *        of receipt of MAC Control Packet from its peer entity. On expiry of this time,
 *        the MAC entity may retransmit the MAC Control Packet.
 */
DEFUN (prime_pib_mac_get_macMinCtlReTxTimer,
       prime_pib_mac_get_macMinCtlReTxTimer_cmd,
       "pib mac get (mlme|mngp) macMinCtlReTxTimer",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macMinCtlReTxTimer\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_MIN_CTL_RE_TX_TIMER,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_MIN_CTL_RE_TX_TIMER,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macMinCtlReTxTimer: %d seconds\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macMinCtlReTxTimer (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macMinCtlReTxTimer,
       prime_bmng_pib_mac_get_macMinCtlReTxTimer_cmd,
       "pib mac get bmng MAC macMinCtlReTxTimer",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macMinCtlReTxTimer\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_MIN_CTL_RE_TX_TIMER,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macMinCtlReTxTimer: %d seconds\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macMinCtlReTxTimer (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macMinCtlReTxTimer PIB Attribute
 *        Minimum number of seconds for which a MAC entity waits for acknowledgement
 *        of receipt of MAC Control Packet from its peer entity. On expiry of this time,
 *        the MAC entity may retransmit the MAC Control Packet.
 */
DEFUN (prime_pib_mac_set_macMinCtlReTxTimer,
       prime_pib_mac_set_macMinCtlReTxTimer_cmd,
       "pib mac set (mlme|mngp) macMinCtlReTxTimer <2>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macMinCtlReTxTimer\n"
       "macMinCtlReTxTimer Value\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  uc_value = (uint8_t) atoi(argv[1]);
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_MIN_CTL_RE_TX_TIMER,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_MIN_CTL_RE_TX_TIMER,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macMinCtlReTxTimer set to %d seconds\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MAC macMinCtlReTxTimer (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_set_macMinCtlReTxTimer,
       prime_bmng_pib_mac_set_macMinCtlReTxTimer_cmd,
       "pib mac set bmng MAC macMinCtlReTxTimer <2>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macMinCtlReTxTimer\n"
       "macMinCtlReTxTimer Value\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   uc_value = (uint8_t) atoi(argv[1]);

   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_set_request_sync(mac,PIB_MAC_MIN_CTL_RE_TX_TIMER,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);

   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macMinCtlReTxTimer set to %d seconds\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to set PRIME PIB-MAC macMinCtlReTxTimer (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macCtrlMsgFailTime PIB Attribute
 *        Number of seconds for which a MAC entity in Switch Nodes waits before
 *        declaring a children’s transaction procedures expired
 */
DEFUN (prime_pib_mac_get_macCtrlMsgFailTime,
       prime_pib_mac_get_macCtrlMsgFailTime_cmd,
       "pib mac get (mlme|mngp) macCtrlMsgFailTime",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macCtrlMsgFailTime\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_CTL_MSG_FAIL_TIME,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_CTL_MSG_FAIL_TIME,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macCtrlMsgFailTime: %d seconds\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macCtrlMsgFailTime (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macCtrlMsgFailTime,
       prime_bmng_pib_mac_get_macCtrlMsgFailTime_cmd,
       "pib mac get bmng MAC macCtrlMsgFailTime",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macCtrlMsgFailTime\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_CTL_MSG_FAIL_TIME,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macCtrlMsgFailTime: %d seconds\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macCtrlMsgFailTime (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set macCtrlMsgFailTime PIB Attribute
 *        Number of seconds for which a MAC entity in Switch Nodes waits before
 *        declaring a children’s transaction procedures expired
 */
DEFUN (prime_pib_mac_set_macCtrlMsgFailTime,
       prime_pib_mac_set_macCtrlMsgFailTime_cmd,
       "pib mac set (mlme|mngp) macCtrlMsgFailTime <6-100>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macCtrlMsgFailTime\n"
       "macCtrlMsgFailTime Value (6-100 default:45)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  uc_value = (uint8_t) atoi(argv[1]);
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_CTL_MSG_FAIL_TIME,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_CTL_MSG_FAIL_TIME,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macCtrlMsgFailTime set to %d seconds\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MAC macCtrlMsgFailTime (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_set_macCtrlMsgFailTime,
       prime_bmng_pib_mac_set_macCtrlMsgFailTime_cmd,
       "pib mac set bmng MAC macCtrlMsgFailTime <6-100>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macCtrlMsgFailTime\n"
       "macCtrlMsgFailTime Value (6-100 default:45)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   uc_value = (uint8_t) atoi(argv[1]);

   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_set_request_sync(mac,PIB_MAC_CTL_MSG_FAIL_TIME,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);

   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macCtrlMsgFailTime set to %d seconds\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to set PRIME PIB-MAC macCtrlMsgFailTime (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macEMASmoothing PIB Attribute
 *        Smoothing factor divider for values that are updated as exponential moving
 *        average (EMA).
 *        Next value is Vnext = S*NewSample+(1–S)*Vprev
 *        Where S=1/(2^macEMASmoothing)
 */
DEFUN (prime_pib_mac_get_macEMASmoothing,
       prime_pib_mac_get_macEMASmoothing_cmd,
       "pib mac get (mlme|mngp) macEMASmoothing",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macEMASmoothing\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_EMA_SMOOTHING,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_EMA_SMOOTHING,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0] ;
      uc_value &= 0x07;
      vty_out(vty,"PRIME PIB-MAC macEMASmoothing: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macEMASmoothing (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macEMASmoothing,
       prime_bmng_pib_mac_get_macEMASmoothing_cmd,
       "pib mac get bmng MAC macEMASmoothing",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macEMASmoothing\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_EMA_SMOOTHING,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);

   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0] ;
      uc_value &= 0x07;
      vty_out(vty,"PRIME PIB-MAC macEMASmoothing: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macEMASmoothing (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set macEMASmoothing PIB Attribute
 *        Smoothing factor divider for values that are updated as exponential moving
 *        average (EMA).
 *        Next value is Vnext = S*NewSample+(1–S)*Vprev
 *        Where S=1/(2^macEMASmoothing)
 */
DEFUN (prime_pib_mac_set_macEMASmoothing,
       prime_pib_mac_set_macEMASmoothing_cmd,
       "pib mac set (mlme|mngp) macEMASmoothing <0-7>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macEMASmoothing\r\n"
       "macEMASmoothing Value\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_EMA_SMOOTHING,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_EMA_SMOOTHING,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macEMASmoothing set to %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MAC macEMASmoothing (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_set_macEMASmoothing,
       prime_bmng_pib_mac_set_macEMASmoothing_cmd,
       "pib mac set bmng MAC macEMASmoothing <0-7>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macEMASmoothing\r\n"
       "macEMASmoothing Value\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   uc_value = (uint8_t) atoi(argv[1]);

   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_set_request_sync(mac,PIB_MAC_EMA_SMOOTHING,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macEMASmoothing set to %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to set PRIME PIB-MAC macEMASmoothing (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macMinBandSearchTime PIB Attribute
 *        Period of time in seconds for which a disconnected Node listen on a
 *        specific band before moving to one other.
 */
DEFUN (prime_pib_mac_get_macMinBandSearchTime,
       prime_pib_mac_get_macMinBandSearchTime_cmd,
       "pib mac get (mlme|mngp) macMinBandSearchTime",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macMinBandSearchTime\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_MIN_BAND_SEARCH_TIME,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_MIN_BAND_SEARCH_TIME,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0] ;
      vty_out(vty,"PRIME PIB-MAC macMinBandSearchTime: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macMinBandSearchTime (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macMinBandSearchTime,
       prime_bmng_pib_mac_get_macMinBandSearchTime_cmd,
       "pib mac get bmng MAC macMinBandSearchTime",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macMinBandSearchTime\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_MIN_BAND_SEARCH_TIME,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0] ;
      vty_out(vty,"PRIME PIB-MAC macMinBandSearchTime: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macMinBandSearchTime (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set macMinBandSearchTime PIB Attribute
 *        Period of time in seconds for which a disconnected Node listen on a
 *        specific band before moving to one other.
 */
DEFUN (prime_pib_mac_set_macMinBandSearchTime,
       prime_pib_mac_set_macMinBandSearchTime_cmd,
       "pib mac set (mlme|mngp) macMinBandSearchTime <32-120>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macMinBandSearchTime\r\n"
       "macMinBandSearchTime Value (32-120 default:60)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  uc_value = (uint8_t) atoi(argv[1]);
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_MIN_BAND_SEARCH_TIME,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_MIN_BAND_SEARCH_TIME,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macMinBandSearchTime set to %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MAC macMinBandSearchTime (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_set_macMinBandSearchTime,
       prime_bmng_pib_mac_set_macMinBandSearchTime_cmd,
       "pib mac set bmng macMinBandSearchTime <32-120>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macMinBandSearchTime\r\n"
       "macMinBandSearchTime Value (32-120 default:60)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   uc_value = (uint8_t) atoi(argv[1]);

   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_set_request_sync(mac,PIB_MAC_MIN_BAND_SEARCH_TIME,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macMinBandSearchTime set to %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to set PRIME PIB-MAC macMinBandSearchTime (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macPromotionMaxTxPeriod PIB Attribute
 *        Period of time in seconds for which at least one PNPDU shall be sent
 */
DEFUN (prime_pib_mac_get_macPromotionMaxTxPeriod,
       prime_pib_mac_get_macPromotionMaxTxPeriod_cmd,
       "pib mac get (mlme|mngp) macPromotionMaxTxPeriod",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macPromotionMaxTxPeriod\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(0x001B,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(0x001B,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0] ;
      vty_out(vty,"PRIME PIB-MAC macPromotionMaxTxPeriod: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macPromotionMaxTxPeriod (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macPromotionMaxTxPeriod,
       prime_bmng_pib_mac_get_macPromotionMaxTxPeriod_cmd,
       "pib mac get bmng MAC macPromotionMaxTxPeriod",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macPromotionMaxTxPeriod\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,0x001B,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0] ;
      vty_out(vty,"PRIME PIB-MAC macPromotionMaxTxPeriod: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macPromotionMaxTxPeriod (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set macPromotionMaxTxPeriod PIB Attribute
 *        Period of time in seconds for which at least one PNPDU shall be sent
 */
DEFUN (prime_pib_mac_set_macPromotionMaxTxPeriod,
       prime_pib_mac_set_macPromotionMaxTxPeriod_cmd,
       "pib mac set (mlme|mngp) macPromotionMaxTxPeriod <16-120>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macPromotionMaxTxPeriod\r\n"
       "macPromotionMaxTxPeriod Value (16-120 default:32)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();
  uc_value = (uint8_t) atoi(argv[1]);
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(0x001B,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(0x001B,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macPromotionMaxTxPeriod set to %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MAC macPromotionMaxTxPeriod (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_set_macPromotionMaxTxPeriod,
       prime_bmng_pib_mac_set_macPromotionMaxTxPeriod_cmd,
       "pib mac set bmng MAC macPromotionMaxTxPeriod <16-120>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macPromotionMaxTxPeriod\r\n"
       "macPromotionMaxTxPeriod Value (16-120 default:32)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   uc_value = (uint8_t) atoi(argv[1]);

   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_set_request_sync(mac,0x001B,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macPromotionMaxTxPeriod set to %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to set PRIME PIB-MAC macPromotionMaxTxPeriod (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macPromotionMinTxPeriod PIB Attribute
 *        Period of time in seconds for which at no more than one PNPDU shall be sent.
 */
DEFUN (prime_pib_mac_get_macPromotionMinTxPeriod,
       prime_pib_mac_get_macPromotionMinTxPeriod_cmd,
       "pib mac get (mlme|mngp) macPromotionMinTxPeriod",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macPromotionMinTxPeriod\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(0x001C,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(0x001C,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0] ;
      vty_out(vty,"PRIME PIB-MAC macPromotionMinTxPeriod: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macPromotionMinTxPeriod (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macPromotionMinTxPeriod,
       prime_bmng_pib_mac_get_macPromotionMinTxPeriod_cmd,
       "pib mac get bmng MAC macPromotionMinTxPeriod",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macPromotionMinTxPeriod\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,0x001C,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0] ;
      vty_out(vty,"PRIME PIB-MAC macPromotionMinTxPeriod: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macPromotionMinTxPeriod (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set macPromotionMinTxPeriod PIB Attribute
 *        Period of time in seconds for which at no more than one PNPDU shall be sent.
 */
DEFUN (prime_pib_mac_set_macPromotionMinTxPeriod,
       prime_pib_mac_set_macPromotionMinTxPeriod_cmd,
       "pib mac set (mlme|mngp) macPromotionMinTxPeriod <2-16>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macPromotionMinTxPeriod\n"
       "macPromotionMinTxPeriod Value (2-16 default:2)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();
  uc_value = (uint8_t) atoi(argv[1]);
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(0x001C,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(0x001C,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macPromotionMinTxPeriod set to %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MAC macPromotionMinTxPeriod (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_set_macPromotionMinTxPeriod,
       prime_bmng_pib_mac_set_macPromotionMinTxPeriod_cmd,
       "pib mac set bmng MAC macPromotionMinTxPeriod <2-16>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macPromotionMinTxPeriod\n"
       "macPromotionMinTxPeriod Value (2-16 default:2)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   uc_value = (uint8_t) atoi(argv[1]);

   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_set_request_sync(mac,0x001C,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macPromotionMinTxPeriod set to %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to set PRIME PIB-MAC macPromotionMinTxPeriod (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macSARSize PIB Attribute
 *        Maximum Data packet size that can be accepted with the MCPS-DATA.Request
 */
DEFUN (prime_pib_mac_get_macSARSize,
       prime_pib_mac_get_macSARSize_cmd,
       "pib mac get (mlme|mngp) macSARSize",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macSARSize\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_SAR_SIZE,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_SAR_SIZE,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0] ;
      uc_value &= 0x07;
      vty_out(vty,"PRIME PIB-MAC macSARSize: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macSARSize (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set macSARSize PIB Attribute
 *        Maximum Data packet size that can be accepted with the MCPS-DATA.Request
 */
DEFUN (prime_pib_mac_set_macSARSize,
       prime_pib_mac_set_macSARSize_cmd,
       "pib mac set (mlme|mngp) macSARSize <0-7>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macSARSize\n"
       "macSARSize Value (0-7 default:0)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

  uc_value = (uint8_t) atoi(argv[1]);
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_SAR_SIZE,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_SAR_SIZE,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macSARSize set to %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MAC macSARSize (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macRobustnessManagement PIB Attribute
 *        Force the network to operate only with one specific modulation
 */
DEFUN (prime_pib_mac_get_macRobustnessManagement,
       prime_pib_mac_get_macRobustnessManagement_cmd,
       "pib mac get (mlme|mngp) macRobustnessManagement",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macRobustnessManagement\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_ACTION_ROBUSTNESS_MGMT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_ACTION_ROBUSTNESS_MGMT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0] ;
      uc_value &= 0x03;
      vty_out(vty,"PRIME PIB-MAC macRobustnessManagement: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macRobustnessManagement (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macRobustnessManagement,
       prime_bmng_pib_mac_get_macRobustnessManagement_cmd,
       "pib mac get bmng MAC macRobustnessManagement",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macRobustnessManagement\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_ACTION_ROBUSTNESS_MGMT,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0] ;
      uc_value &= 0x03;
      vty_out(vty,"PRIME PIB-MAC macRobustnessManagement: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macRobustnessManagement (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set macRobustnessManagement PIB Attribute
 *        Force the network to operate only with one specific modulation
 */
DEFUN (prime_pib_mac_set_macRobustnessManagement,
       prime_pib_mac_set_macRobustnessManagement_cmd,
       "pib mac set (mlme|mngp) macRobustnessManagement (auto|dbpsk_cc|dqpsk_r|dbpsk_r)",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macRobustnessManagement\n"
       "macRobustnessManagement Auto\n"
       "macRobustnessManagement DBPSK_CC\n"
       "macRobustnessManagement DQPSK_R\n"
       "macRobustnessManagement DBPSK_R\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

  if (strcmp(argv[1],"auto") == 0){
      uc_value = 0;
  }else if (strcmp(argv[1],"dbpsk_cc") == 0){
      uc_value = 1;
  }else if (strcmp(argv[1],"dqpsk_r") == 0){
      uc_value = 2;
  }else{
      uc_value = 3;
  }
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_ROBUSTNESS_MGMT,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_ROBUSTNESS_MGMT,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macRobustnessManagement set to %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MAC macRobustnessManagement (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macUpdatedRMTimeout PIB Attribute
 *        The maximum amount of time that robustness management information
 *        is considered to be valid without any further update
 */
DEFUN (prime_pib_mac_get_macUpdatedRMTimeout,
       prime_pib_mac_get_macUpdatedRMTimeout_cmd,
       "pib mac get (mlme|mngp) macUpdatedRMTimeout",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macUpdatedRMTimeout\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_UPDATED_RM_TIMEOUT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_UPDATED_RM_TIMEOUT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 2 Byte should be the result
      memcpy(&us_value,&x_pib_confirm.m_au8AttributeValue[0],2) ;
      vty_out(vty,"PRIME PIB-MAC macUpdatedRMTimeout: %d\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macUpdatedRMTimeout (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macUpdatedRMTimeout,
       prime_bmng_pib_mac_get_macUpdatedRMTimeout_cmd,
       "pib mac get bmng MAC macUpdatedRMTimeout",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macUpdatedRMTimeout\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_UPDATED_RM_TIMEOUT,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 2 Byte should be the result
      memcpy(&us_value,&x_pib_confirm.m_au8AttributeValue[0],2) ;
      vty_out(vty,"PRIME PIB-MAC macUpdatedRMTimeout: %d\r\n",us_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macUpdatedRMTimeout (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set macUpdatedRMTimeout PIB Attribute
 *        The maximum amount of time that robustness management information
 *        is considered to be valid without any further update
 */
DEFUN (prime_pib_mac_set_macUpdatedRMTimeout,
       prime_pib_mac_set_macUpdatedRMTimeout_cmd,
       "pib mac set (mlme|mngp) macUpdatedRMTimeout <60-3600>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macRobustnessManagement\n"
       "macRobustnessManagement Value (60-3600 default:240)")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  us_value = (uint16_t) atoi(argv[1]);
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_UPDATED_RM_TIMEOUT,(uint8_t *) &us_value,2,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_UPDATED_RM_TIMEOUT,(uint8_t *) &us_value,2,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macUpdatedRMTimeout set to %d\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to set PRIME PIB-MAC macUpdatedRMTimeout (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_set_macUpdatedRMTimeout,
       prime_bmng_pib_mac_set_macUpdatedRMTimeout_cmd,
       "pib mac set bmng MAC macUpdatedRMTimeout <60-3600>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macRobustnessManagement\n"
       "macRobustnessManagement Value (60-3600 default:240)")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   us_value = (uint16_t) atoi(argv[1]);

   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_set_request_sync(mac,PIB_MAC_UPDATED_RM_TIMEOUT,(uint8_t *)&us_value,2,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macUpdatedRMTimeout set to %d\r\n",us_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to set PRIME PIB-MAC macUpdatedRMTimeout (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macALVHopRepetitions PIB Attribute
 *        Number of repletion for the ALV packets
 */
DEFUN (prime_pib_mac_get_macALVHopRepetitions,
       prime_pib_mac_get_macALVHopRepetitions_cmd,
       "pib mac get (mlme|mngp) macALVHopRepetitions",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macALVHopRepetitions\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_ALV_HOP_REPETITIONS,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_ALV_HOP_REPETITIONS,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0] ;
      uc_value &= 0x07;
      vty_out(vty,"PRIME PIB-MAC macALVHopRepetitions: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macALVHopRepetitions (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macALVHopRepetitions,
       prime_bmng_pib_mac_get_macALVHopRepetitions_cmd,
       "pib mac get bmng MAC macALVHopRepetitions",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macALVHopRepetitions\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_ALV_HOP_REPETITIONS,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0] ;
      uc_value &= 0x07;
      vty_out(vty,"PRIME PIB-MAC macALVHopRepetitions: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macALVHopRepetitions (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Set macALVHopRepetitions PIB Attribute
 *        Number of repletion for the ALV packets
 */
DEFUN (prime_pib_mac_set_macALVHopRepetitions,
       prime_pib_mac_set_macALVHopRepetitions_cmd,
       "pib mac set bmng MAC macALVHopRepetitions <0-7>",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_SET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macALVHopRepetitions\n"
       "macALVHopRepetitions Value (0-7 default:5)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   uc_value = (uint8_t) atoi(argv[1]);

   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_set_request_sync(mac,PIB_MAC_ALV_HOP_REPETITIONS,&uc_value,1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macALVHopRepetitions set to %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to set PRIME PIB-MAC macALVHopRepetitions (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

// MAC READ ONLY VARIABLES

/**
 * \brief Get macSCPChSenseCount PIB Attribute
 *        Number of times for which an implementation has to perform channel-sensing.
 */
DEFUN (prime_pib_mac_get_macSCPChSenseCount,
       prime_pib_mac_get_macSCPChSenseCount_cmd,
       "pib mac get (mlme|mngp) macSCPChSenseCount",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macSCPChSenseCount (2-5 default:)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_SCP_CH_SENSE_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_SCP_CH_SENSE_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macSCPChSenseCount: %d seconds\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macSCPChSenseCount (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macSCPChSenseCount,
       prime_bmng_pib_mac_get_macSCPChSenseCount_cmd,
       "pib mac get bmng MAC macSCPChSenseCount",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macSCPChSenseCount (2-5 default:)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_SCP_CH_SENSE_COUNT,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macSCPChSenseCount: %d seconds\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macSCPChSenseCount (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macEUI48 PIB Attribute
 *        EUI-48 of the Node
 */
DEFUN (prime_pib_mac_get_macEUI48,
       prime_pib_mac_get_macEUI48_cmd,
       "pib mac get (mlme|mngp) macEUI48",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "MAC EUI-48\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_EUI_48,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_EUI_48,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 6)){
      // 6 Byte should be the result
      vty_out(vty,"PRIME PIB-MAC EUI-48: 0x%02X%02X%02X%02X%02X%02X\r\n",
                                            x_pib_confirm.m_au8AttributeValue[0],
                                            x_pib_confirm.m_au8AttributeValue[1],
                                            x_pib_confirm.m_au8AttributeValue[2],
                                            x_pib_confirm.m_au8AttributeValue[3],
                                            x_pib_confirm.m_au8AttributeValue[4],
                                            x_pib_confirm.m_au8AttributeValue[5]);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC EUI-48 (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macCSMAR1 PIB Attribute
 *        Control how fast the CSMA contention window shall increase.
 *        Controls exponential increase of initial CSMA contention window size
 */
DEFUN (prime_pib_mac_get_macCSMAR1,
       prime_pib_mac_get_macCSMAR1_cmd,
       "pib mac get (mlme|mngp) macCSMAR1",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macCSMAR1 (0-4 default:3)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_CSMA_R1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_CSMA_R1,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macCSMAR1: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMAR1 (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macCSMAR1,
       prime_bmng_pib_mac_get_macCSMAR1_cmd,
       "pib mac get bmng MAC macCSMAR1",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macCSMAR1 (0-4 default:3)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_CSMA_R1,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macCSMAR1: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMAR1 (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macCSMAR2 PIB Attribute
 *        Control initial CSMA contention window size.
 *        Controls linear increase of initial CSMA contention window size.
 */
DEFUN (prime_pib_mac_get_macCSMAR2,
       prime_pib_mac_get_macCSMAR2_cmd,
       "pib mac get (mlme|mngp) macCSMAR2",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macCSMAR2 (1-4 default:1)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_CSMA_R2,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_CSMA_R2,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macCSMAR2: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMAR2 (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macCSMAR2,
       prime_bmng_pib_mac_get_macCSMAR2_cmd,
       "pib mac get bmng MAC macCSMAR2",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macCSMAR2 (1-4 default:1)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_CSMA_R2,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macCSMAR2: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMAR2 (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macCSMADelay PIB Attribute
 *        The delay between two consecutive CSMA channel senses.
 */
DEFUN (prime_pib_mac_get_macCSMADelay,
       prime_pib_mac_get_macCSMADelay_cmd,
       "pib mac get (mlme|mngp) macCSMADelay",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macCSMADelay (3-9 default:3ms)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_CSMA_DELAY,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_CSMA_DELAY,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macCSMADelay: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMADelay (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macCSMADelay,
       prime_bmng_pib_mac_get_macCSMADelay_cmd,
       "pib mac get bmng MAC macCSMADelay",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macCSMADelay (3-9 default:3ms)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_CSMA_DELAY,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macCSMADelay: %u\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMADelay (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macCSMAR1Robust PIB Attribute
 *        Control how fast the CSMA contention window shall increase when node supports Robust Mode.
 *        Controls exponential increase of initial CSMA contention window size.
 */
DEFUN (prime_pib_mac_get_macCSMAR1Robust,
       prime_pib_mac_get_macCSMAR1Robust_cmd,
       "pib mac get (mlme|mngp) macCSMAR1Robust",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macCSMAR1Robust (0-5 default:4)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_CSMA_R1_ROBUST,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_CSMA_R1_ROBUST,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macCSMAR1Robust: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMAR1Robust (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macCSMAR1Robust,
       prime_bmng_pib_mac_get_macCSMAR1Robust_cmd,
       "pib mac get bmng MAC macCSMAR1Robust",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macCSMAR1Robust (0-5 default:4)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_CSMA_R1_ROBUST,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macCSMAR1Robust: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMAR1Robust (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macCSMAR2Robust PIB Attribute
 *        Control initial CSMA contention window size when node supports Robust Mode.
 *        Controls linear increase of initial CSMA contention window size.
 */
DEFUN (prime_pib_mac_get_macCSMAR2Robust,
       prime_pib_mac_get_macCSMAR2Robust_cmd,
       "pib mac get (mlme|mngp) macCSMAR2Robust",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macCSMAR2Robust (1-8 default:2)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_CSMA_R2_ROBUST,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_CSMA_R2_ROBUST,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macCSMAR2Robust: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMAR2Robust (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macCSMAR2Robust,
       prime_bmng_pib_mac_get_macCSMAR2Robust_cmd,
       "pib mac get bmng MAC macCSMAR2Robust",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macCSMAR2Robust (1-8 default:2)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_CSMA_R2_ROBUST,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macCSMAR2Robust: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMAR2Robust (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macCSMADelayRobust PIB Attribute
 *        The delay between two consecutive CSMA channel senses when node supports Robust Mode.
 */
DEFUN (prime_pib_mac_get_macCSMADelayRobust,
       prime_pib_mac_get_macCSMADelayRobust_cmd,
       "pib mac get (mlme|mngp) macCSMADelayRobust",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macCSMADelayRobust (3-9 default:6)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_CSMA_DELAY_ROBUST,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_CSMA_DELAY_ROBUST,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macCSMADelayRobust: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMADelayRobust (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macCSMADelayRobust,
       prime_bmng_pib_mac_get_macCSMADelayRobust_cmd,
       "pib mac get bmng MAC macCSMADelayRobust",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macCSMADelayRobust (3-9 default:6)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_CSMA_DELAY_ROBUST,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macCSMADelayRobust: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMADelayRobust (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macAliveTimeMode PIB Attribute
 *        Selects the MACAliveTime value mapping for network Alive time.
 */
DEFUN (prime_pib_mac_get_macAliveTimeMode,
       prime_pib_mac_get_macAliveTimeMode_cmd,
       "pib mac get (mlme|mngp) macAliveTimeMode",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macAliveTimeMode (0: 1.4 mode 1:BC mode)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_ALV_TIME_MODE,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_ALV_TIME_MODE,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macAliveTimeMode: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macAliveTimeMode (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

// MAC READ ONLY Functional Attributes

/**
 * \brief Get macLNID PIB Attribute
 *        LNID allocated to this Node at time of its registration.
 *        (0x0000 is reserved for Base Node)
 */
DEFUN (prime_pib_mac_get_macLNID,
       prime_pib_mac_get_macLNID_cmd,
       "pib mac get (mlme|mngp) macLNID",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macLNID (0-16383)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  //VTY_CHECK_PRIME_MODE_NO_BASE();

  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_LNID,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_LNID,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      memcpy(&us_value,&x_pib_confirm.m_au8AttributeValue[0],2);
      vty_out(vty,"PRIME PIB-MAC macLNID: %d\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macLNID (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macLNID,
       prime_bmng_pib_mac_get_macLNID_cmd,
       "pib mac get bmng MAC macLNID",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macLNID (0-16383)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_LNID,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);

   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      memcpy(&us_value,&x_pib_confirm.m_au8AttributeValue[0],2);
      vty_out(vty,"PRIME PIB-MAC macLNID: %d\r\n",us_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macLNID (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macLSID PIB Attribute
 *        LSID allocated to this Node at time of its promotion.
 *        This attribute is not maintained if a Node is in a Terminal functional state.
 *        (0x00 is reserved for Base Node)
 */
DEFUN (prime_pib_mac_get_macLSID,
       prime_pib_mac_get_macLSID_cmd,
       "pib mac get (mlme|mngp) macLSID",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macLSID (0-255)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  //VTY_CHECK_PRIME_MODE_NO_BASE();

  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_LSID,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_LSID,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macLSID: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macLSID (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macLSID,
       prime_bmng_pib_mac_get_macLSID_cmd,
       "pib mac get bmng MAC macLSID",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macLSID (0-255)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_LSID,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);

   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macLSID: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macLSID (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macSID PIB Attribute
 *        SID of the Switch Node through which this Node is connected to the Subnetwork.
 *        This attribute is not maintained in a Base Node.
 */
DEFUN (prime_pib_mac_get_macSID,
       prime_pib_mac_get_macSID_cmd,
       "pib mac get (mlme|mngp) macSID",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macSID (0-255)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();

  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_SID,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_SID,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macSID: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macSID (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macSID,
       prime_bmng_pib_mac_get_macSID_cmd,
       "pib mac get bmng MAC macSID",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macSID (0-255)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_SID,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);

   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macSID: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macSID (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macSNA PIB Attribute
 *        Subnetwork address to which this Node is registered.
 *        The Base Node returns the SNA it is using.
 */
DEFUN (prime_pib_mac_get_macSNA,
       prime_pib_mac_get_macSNA_cmd,
       "pib mac get (mlme|mngp) macSNA",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macSNA\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t sna[6];
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_SNA,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_SNA,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 6)){
      // 6 Byte should be the result
      memcpy(sna,&x_pib_confirm.m_au8AttributeValue[0],6);
      vty_out(vty,"PRIME PIB-MAC macSNA: 0x%02X%02X%02X%02X%02X%02X\r\n",sna[0],sna[1],sna[2],sna[3],sna[4],sna[5]);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macSNA (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macSNA,
       prime_bmng_pib_mac_get_macSNA_cmd,
       "pib mac get bmng MAC macSNA",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macSNA\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t sna[6];
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_SNA,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 6)){
      // 6 Byte should be the result
      memcpy(sna,&x_pib_confirm.m_au8AttributeValue[0],6);
      vty_out(vty,"PRIME PIB-MAC macSNA: 0x%02X%02X%02X%02X%02X%02X\r\n",sna[0],sna[1],sna[2],sna[3],sna[4],sna[5]);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macSNA (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macState PIB Attribute
 *        Present functional state of the Node.
 *        0: DISCONNECTED  1: TERMINAL 2: SWITCH 3: BASE
 */
DEFUN (prime_pib_mac_get_macState,
       prime_pib_mac_get_macState_cmd,
       "pib mac get (mlme|mngp) macState",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macState\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();

  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_STATE,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_STATE,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macState: %s(%d)\r\n",mac_state_str[uc_value],uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macState (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macState,
       prime_bmng_pib_mac_get_macState_cmd,
       "pib mac get bmng MAC macState",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macState\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_STATE,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macState: %s(%d)\r\n",mac_state_str[uc_value],uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macState (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macSCPLength PIB Attribute
 *        The SCP length, in symbols, in present frame.
 */
DEFUN (prime_pib_mac_get_macSCPLength,
       prime_pib_mac_get_macSCPLength_cmd,
       "pib mac get (mlme|mngp) macSCPLength",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macSCPLength\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_SCP_LENGTH,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_SCP_LENGTH,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      memcpy(&us_value, &x_pib_confirm.m_au8AttributeValue[0],2);
      vty_out(vty,"PRIME PIB-MAC macSCPLength: %d\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macSCPLength (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macSCPLength,
       prime_bmng_pib_mac_get_macSCPLength_cmd,
       "pib mac get bmng MAC macSCPLength",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macSCPLength\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_SCP_LENGTH,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      memcpy(&us_value, &x_pib_confirm.m_au8AttributeValue[0],2);
      vty_out(vty,"PRIME PIB-MAC macSCPLength: %d\r\n",us_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macSCPLength (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macNodeHierarchyLevel PIB Attribute
 *        Level of this Node in Subnetwork hierarchy.
 */
DEFUN (prime_pib_mac_get_macNodeHierarchyLevel,
       prime_pib_mac_get_macNodeHierarchyLevel_cmd,
       "pib mac get (mlme|mngp) macNodeHierarchyLevel",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macNodeHierarchyLevel (0-63)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();

  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_NODE_HIERARCHY_LEVEL,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_NODE_HIERARCHY_LEVEL,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macNodeHierarchyLevel: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macNodeHierarchyLevel (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macNodeHierarchyLevel,
       prime_bmng_pib_mac_get_macNodeHierarchyLevel_cmd,
       "pib mac get bmng MAC macNodeHierarchyLevel",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macNodeHierarchyLevel (0-63)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_NODE_HIERARCHY_LEVEL,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macNodeHierarchyLevel: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macNodeHierarchyLevel (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macBeaconRxPos PIB Attribute
 *       Beacon Position on which this device’s Switch Node transmits its beacon.
 *       Position is expressed in terms of symbols from the start of the frame.
 *       This attribute is not maintained in a Base Node.
 */
DEFUN (prime_pib_mac_get_macBeaconRxPos,
       prime_pib_mac_get_macBeaconRxPos_cmd,
       "pib mac get (mlme|mngp) macBeaconRxPos",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macBeaconRxPos (0-1104)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();

  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_BEACON_RX_POS,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_BEACON_RX_POS,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      memcpy(&us_value, &x_pib_confirm.m_au8AttributeValue[0],2);
      vty_out(vty,"PRIME PIB-MAC macBeaconRxPos: %d\r\n",us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macBeaconRxPos (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macBeaconRxPos,
       prime_bmng_pib_mac_get_macBeaconRxPos_cmd,
       "pib mac get bmng MAC macBeaconRxPos",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macBeaconRxPos (0-1104)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_BEACON_RX_POS,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      memcpy(&us_value, &x_pib_confirm.m_au8AttributeValue[0],2);
      vty_out(vty,"PRIME PIB-MAC macBeaconRxPos: %d\r\n",us_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macBeaconRxPos (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macBeaconTxPos PIB Attribute
 *       Beacon Position in which this device transmits its beacon.
 *       Position is expressed in terms of symbols from the start of the frame.
 *       This attribute is not maintained in Service Nodes that are in a Terminal functional state.
 */
DEFUN (prime_pib_mac_get_macBeaconTxPos,
       prime_pib_mac_get_macBeaconTxPos_cmd,
       "pib mac get (mlme|mngp) macBeaconTxPos",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macBeaconTxPos (0-1104)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();

  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_BEACON_TX_POS,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_BEACON_TX_POS,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macBeaconTxPos: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macBeaconTxPos (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macBeaconTxPos,
       prime_bmng_pib_mac_get_macBeaconTxPos_cmd,
       "pib mac get bmng MAC macBeaconTxPos",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macBeaconTxPos (0-1104)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_BEACON_TX_POS,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macBeaconTxPos: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macBeaconTxPos (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macBeaconRxFrequency PIB Attribute
 *       Number of frames between receptions of two successive beacons.
 *       A value of 0x0 indicates beacons are received in every frame.
 *       This attribute is not maintained in Base Node.
 *       Use the same encoding of FRQ field in the packets
 */
DEFUN (prime_pib_mac_get_macBeaconRxFrequency,
       prime_pib_mac_get_macBeaconRxFrequency_cmd,
       "pib mac get (mlme|mngp) macBeaconRxFrequency",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macBeaconRxFrequency (0-5)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();

  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_BEACON_RX_FREQUENCY,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_BEACON_RX_FREQUENCY,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macBeaconRxFrequency: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macBeaconRxFrequency (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macBeaconRxFrequency,
       prime_bmng_pib_mac_get_macBeaconRxFrequency_cmd,
       "pib mac get bmng MAC macBeaconRxFrequency",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macBeaconRxFrequency (0-5)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_BEACON_RX_FREQUENCY,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macBeaconRxFrequency: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macBeaconRxFrequency (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macBeaconTxFrequency PIB Attribute
 *       Number of frames between transmissions of two successive beacons.
 *       A value of 0x0 indicates beacons are transmitted in every frame.
 *       This attribute is not maintained in Service Nodes that are in a Terminal functional state.
 *       Use the same encoding of FRQ field in the packets
 */
DEFUN (prime_pib_mac_get_macBeaconTxFrequency,
       prime_pib_mac_get_macBeaconTxFrequency_cmd,
       "pib mac get (mlme|mngp) macBeaconTxFrequency",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macBeaconTxFrequency (0-5)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();

  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_BEACON_TX_FREQUENCY,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_BEACON_TX_FREQUENCY,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macBeaconTxFrequency: %d\r\n",uc_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macBeaconTxFrequency (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macBeaconTxFrequency,
       prime_bmng_pib_mac_get_macBeaconTxFrequency_cmd,
       "pib mac get bmng MAC macBeaconTxFrequency",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macBeaconTxFrequency (0-5)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_BEACON_TX_FREQUENCY,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 1)){
      // 1 Byte should be the result
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-MAC macBeaconTxFrequency: %d\r\n",uc_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macBeaconTxFrequency (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macCapabilities PIB Attribute
 *       Bitmap of MAC capabilities of a given device.
 *       This attribute shall be maintained on all devices.
 */
DEFUN (prime_pib_mac_get_macCapabilities,
       prime_pib_mac_get_macCapabilities_cmd,
       "pib mac get (mlme|mngp) macCapabilities",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macCapabilities\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_MAC_CAPABILITES,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_MAC_CAPABILITES,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0] << 8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-MAC macCapabilities:\r\n     \
                       * Robust Mode           : %s\r\n   \
                       * Backward Compatibility: %s\r\n   \
                       * Switch Capable        : %s\r\n   \
                       * Packet Aggregation    : %s\r\n   \
                       * Connection Free Period: %s\r\n   \
                       * Direct Connection     : %s\r\n   \
                       * ARQ                   : %s\r\n   \
                       * Multicast Switch      : %s\r\n   \
                       * Direct Connection SW  : %s\r\n   \
                       * Robust Promotion      : %s\r\n   \
                       * ARQ Buffering SW      : %s\r\n", \
                                        us_value & 0x0001 ? "Yes":"No",
                                        us_value & 0x0002 ? "Yes":"No",
                                        us_value & 0x0004 ? "Yes":"No",
                                        us_value & 0x0008 ? "Yes":"No",
                                        us_value & 0x0010 ? "Yes":"No",
                                        us_value & 0x0020 ? "Yes":"No",
                                        us_value & 0x0040 ? "Yes":"No",
                                        us_value & 0x0100 ? "Yes":"No",
                                        us_value & 0x0200 ? "Yes":"No",
                                        us_value & 0x0400 ? "Yes":"No",
                                        us_value & 0x0800 ? "Yes":"No");
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macCapabilities (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macCapabilities,
       prime_bmng_pib_mac_get_macCapabilities_cmd,
       "pib mac get bmng MAC macCapabilities",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macCapabilities\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_MAC_CAPABILITES,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0] << 8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-MAC macCapabilities:\r\n     \
                       * Robust Mode           : %s\r\n   \
                       * Backward Compatibility: %s\r\n   \
                       * Switch Capable        : %s\r\n   \
                       * Packet Aggregation    : %s\r\n   \
                       * Connection Free Period: %s\r\n   \
                       * Direct Connection     : %s\r\n   \
                       * ARQ                   : %s\r\n   \
                       * Multicast Switch      : %s\r\n   \
                       * Direct Connection SW  : %s\r\n   \
                       * Robust Promotion      : %s\r\n   \
                       * ARQ Buffering SW      : %s\r\n", \
                                        us_value & 0x0001 ? "Yes":"No",
                                        us_value & 0x0002 ? "Yes":"No",
                                        us_value & 0x0004 ? "Yes":"No",
                                        us_value & 0x0008 ? "Yes":"No",
                                        us_value & 0x0010 ? "Yes":"No",
                                        us_value & 0x0020 ? "Yes":"No",
                                        us_value & 0x0040 ? "Yes":"No",
                                        us_value & 0x0100 ? "Yes":"No",
                                        us_value & 0x0200 ? "Yes":"No",
                                        us_value & 0x0400 ? "Yes":"No",
                                        us_value & 0x0800 ? "Yes":"No");
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macCapabilities (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macFrameLength PIB Attribute
 *        Frame Length in the present super-frame
 */
DEFUN (prime_pib_mac_get_macFrameLength,
       prime_pib_mac_get_macFrameLength_cmd,
       "pib mac get (mlme|mngp) macFrameLength",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macFrameLength (0-3)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_FRAME_LENGTH,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_FRAME_LENGTH,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0] << 8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-MAC macFrameLength: %d symbols\r\n", us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macFrameLength (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macFrameLength,
       prime_bmng_pib_mac_get_macFrameLength_cmd,
       "pib mac get bmng MAC macFrameLength",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macFrameLength (0-3)\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_FRAME_LENGTH,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0] << 8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-MAC macFrameLength: %d symbols\r\n", us_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macFrameLength (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macCFPLength PIB Attribute
 *        The CFP length in symbols, in present frame
 */
DEFUN (prime_pib_mac_get_macCFPLength,
       prime_pib_mac_get_macCFPLength_cmd,
       "pib mac get (mlme|mngp) macCFPLength",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macCFPLength\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_CFP_LENGTH,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_CFP_LENGTH,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0] << 8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-MAC macCFPLength: %d\r\n", us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macCFPLength (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macCFPLength,
       prime_bmng_pib_mac_get_macCFPLength_cmd,
       "pib mac get bmng MAC macCFPLength",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macCFPLength\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_CFP_LENGTH,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0] << 8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-MAC macCFPLength: %d\r\n", us_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macCFPLength (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macGuardTime PIB Attribute
 *        The guard time between portion of the frame in symbols
 */
DEFUN (prime_pib_mac_get_macGuardTime,
       prime_pib_mac_get_macGuardTime_cmd,
       "pib mac get (mlme|mngp) macGuardTime",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macGuardTime\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_GUARD_TIME,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_GUARD_TIME,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0] << 8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-MAC macGuardTime: %d\r\n", us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macGuardTime (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macGuardTime,
       prime_bmng_pib_mac_get_macGuardTime_cmd,
       "pib mac get bmng MAC macGuardTime",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macGuardTime\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_GUARD_TIME,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0] << 8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-MAC macGuardTime: %d\r\n", us_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macGuardTime (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macBCMode PIB Attribute
 *        MAC is operating in Backward Compatibility Mode
 */
DEFUN (prime_pib_mac_get_macBCMode,
       prime_pib_mac_get_macBCMode_cmd,
       "pib mac get (mlme|mngp) macBCMode",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macBCMode\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_BC_MODE,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_BC_MODE,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0] << 8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-MAC macBCMode: %s\r\n", (us_value == 0)?"No":"Yes" );
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macBCMode (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macBCMode,
       prime_bmng_pib_mac_get_macBCMode_cmd,
       "pib mac get bmng MAC macBCMode",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macBCMode\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_BC_MODE,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0] << 8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-MAC macBCMode: %s\r\n", (us_value == 0)?"No":"Yes" );
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macBCMode (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macBeaconRxQlty PIB Attribute
 *        The QLTY field this device’s Switch Node transmits its beacon.
 */
DEFUN (prime_pib_mac_get_macBeaconRxQlty,
       prime_pib_mac_get_macBeaconRxQlty_cmd,
       "pib mac get (mlme|mngp) macBeaconRxQlty",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macBeaconRxQlty\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_BEACON_RX_QLTY,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_BEACON_RX_QLTY,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0] << 8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-MAC macBeaconRxQlty: %d\r\n", us_value );
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macBeaconRxQlty (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macBeaconRxQlty,
       prime_bmng_pib_mac_get_macBeaconRxQlty_cmd,
       "pib mac get bmng MAC macBeaconRxQlty",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macBeaconRxQlty\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_BEACON_RX_QLTY,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0] << 8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-MAC macBeaconRxQlty: %d\r\n", us_value );
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macBeaconRxQlty (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macBeaconTxQlty PIB Attribute
 *        The QLTY field this device transmits its beacon.
 */
DEFUN (prime_pib_mac_get_macBeaconTxQlty,
       prime_pib_mac_get_macBeaconTxQlty_cmd,
       "pib mac get (mlme|mngp) macBeaconTxQlty",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macBeaconTxQlty\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_BEACON_TX_QLTY,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_BEACON_TX_QLTY,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0] << 8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-MAC macBeaconTxQlty: %d\r\n", us_value );
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macBeaconTxQlty (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macBeaconTxQlty,
       prime_bmng_pib_mac_get_macBeaconTxQlty_cmd,
       "pib mac get bmng MAC macBeaconTxQlty",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macBeaconTxQlty\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_BEACON_TX_QLTY,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0] << 8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-MAC macBeaconTxQlty: %d\r\n", us_value );
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macBeaconTxQlty (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

// MAC Statistical Attributes
/**
 * \brief Get macTxDataPktCount PIB Attribute
 *        Count of successfully transmitted MSDUs
 */
DEFUN (prime_pib_mac_get_macTxDataPktCount,
       prime_pib_mac_get_macTxDataPktCount_cmd,
       "pib mac get (mlme|mngp) macTxDataPktCount",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macTxDataPktCount\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_TX_DATAPKT_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_TX_DATAPKT_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"PRIME PIB-MAC macTxDataPktCount: %u\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macTxDataPktCount (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macTxDataPktCount,
       prime_bmng_pib_mac_get_macTxDataPktCount_cmd,
       "pib mac get bmng MAC macTxDataPktCount",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macTxDataPktCount\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_TX_DATAPKT_COUNT,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"PRIME PIB-MAC macTxDataPktCount: %u\r\n",ui_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macTxDataPktCount (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macRxDataPktCount PIB Attribute
 *        Count of successfully received MSDUs whose destination address was this node
 */
DEFUN (prime_pib_mac_get_macRxDataPktCount,
       prime_pib_mac_get_macRxDataPktCount_cmd,
       "pib mac get (mlme|mngp) macRxDataPktCount",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macRxDataPktCount\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_RX_DATAPKT_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_RX_DATAPKT_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"PRIME PIB-MAC macTxDataPktCount: %u\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macTxDataPktCount (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macRxDataPktCount,
       prime_bmng_pib_mac_get_macRxDataPktCount_cmd,
       "pib mac get bmng MAC macRxDataPktCount",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macRxDataPktCount\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_RX_DATAPKT_COUNT,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"PRIME PIB-MAC macRxDataPktCount: %u\r\n",ui_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macRxDataPktCount (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macTxCtrlPktCount PIB Attribute
 *        Count of successfully transmitted MAC control packets
 */
DEFUN (prime_pib_mac_get_macTxCtrlPktCount,
       prime_pib_mac_get_macTxCtrlPktCount_cmd,
       "pib mac get (mlme|mngp) macTxCtrlPktCount",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macTxCtrlPktCount\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_TX_CTRLPKT_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_TX_CTRLPKT_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"PRIME PIB-MAC macTxCtrlPktCount: %u\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macTxCtrlPktCount (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macTxCtrlPktCount,
       prime_bmng_pib_mac_get_macTxCtrlPktCount_cmd,
       "pib mac get bmng MAC macTxCtrlPktCount",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macTxCtrlPktCount\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_TX_CTRLPKT_COUNT,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"PRIME PIB-MAC macTxCtrlPktCount: %u\r\n",ui_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macTxCtrlPktCount (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macRxCtrlPktCount PIB Attribute
 *        Count of successfully received MAC control packets whose destination address was this node
 */
DEFUN (prime_pib_mac_get_macRxCtrlPktCount,
       prime_pib_mac_get_macRxCtrlPktCount_cmd,
       "pib mac get (mlme|mngp) macRxCtrlPktCount",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macRxCtrlPktCount\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_RX_CTRLPKT_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_RX_CTRLPKT_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"PRIME PIB-MAC macRxCtrlPktCount: %u\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macRxCtrlPktCount (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macRxCtrlPktCount,
       prime_bmng_pib_mac_get_macRxCtrlPktCount_cmd,
       "pib mac get bmng MAC macRxCtrlPktCount",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macRxCtrlPktCount\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_RX_CTRLPKT_COUNT,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"PRIME PIB-MAC macRxCtrlPktCount: %u\r\n",ui_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macRxCtrlPktCount (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macCSMAFailCount PIB Attribute
 *        Count of failed CSMA transmitted attempts.
 */
DEFUN (prime_pib_mac_get_macCSMAFailCount,
       prime_pib_mac_get_macCSMAFailCount_cmd,
       "pib mac get (mlme|mngp) macCSMAFailCount",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macCSMAFailCount\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_CSMA_FAIL_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_CSMA_FAIL_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"PRIME PIB-MAC macCSMAFailCount: %u\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMAFailCount (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macCSMAFailCount,
       prime_bmng_pib_mac_get_macCSMAFailCount_cmd,
       "pib mac get bmng MAC macCSMAFailCount",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macCSMAFailCount\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_CSMA_FAIL_COUNT,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"PRIME PIB-MAC macCSMAFailCount: %u\r\n",ui_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMAFailCount (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macCSMAChBusyCount PIB Attribute
 *        Count of number of times this Node had to back off SCP transmission due to channel busy state.
 */
DEFUN (prime_pib_mac_get_macCSMAChBusyCount,
       prime_pib_mac_get_macCSMAChBusyCount_cmd,
       "pib mac get (mlme|mngp) macCSMAChBusyCount",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macCSMAChBusyCount\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
       prime_cl_null_mlme_get_request_sync(PIB_MAC_CSMA_FAIL_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }else{
       prime_mngp_get_request_sync(PIB_MAC_CSMA_FAIL_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"PRIME PIB-MAC macCSMAChBusyCount: %u\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMAChBusyCount (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macCSMAChBusyCount,
       prime_bmng_pib_mac_get_macCSMAChBusyCount_cmd,
       "pib mac get bmng MAC macCSMAChBusyCount",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "macCSMAChBusyCount\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_CSMA_FAIL_COUNT,0,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"PRIME PIB-MAC macCSMAChBusyCount: %u\r\n",ui_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-MAC macCSMAChBusyCount (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

// MAC Secuity Attributes

// MAC List Attributes

/**
 * \brief Get macListRegDevices PIB Attribute
 *        List of registered devices. This list is maintained by the Base Node only.
 *        Each entry in this list shall comprise the following information.
 */
DEFUN (prime_pib_mac_get_macListRegDevices,
       prime_pib_mac_get_macListRegDevices_cmd,
       "pib mac get (mlme|mngp) macListRegDevices",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Refresh macListRegDevices\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
mchp_list *entry, *tmp;
prime_sn * p_prime_sn;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

  // Remove registerd flag for all the network devices before to know what of them are registered
  prime_network_mutex_lock();
  list_for_each_safe(entry, tmp, &prime_network) {
     p_prime_sn = list_entry(entry, prime_sn, list);
     p_prime_sn->registered = FALSE;
  }
  prime_network_mutex_unlock();

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_list_get_request_sync(PIB_MAC_LIST_REGISTER_DEVICES,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST,&x_pib_confirm);
  }else{
     prime_mngp_list_get_request_sync(PIB_MAC_LIST_REGISTER_DEVICES,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macListRegDevices request success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macListRegDevices request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macListActiveConn PIB Attribute
 *        List of active non-direct connections.
 *        This list is maintained by the Base Node only.
 */
DEFUN (prime_pib_mac_get_macListActiveConn,
       prime_pib_mac_get_macListActiveConn_cmd,
       "pib mac get (mlme|mngp) macListActiveConn",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Refresh macListActiveConn\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
mchp_list *entry, *tmp;
prime_sn * p_prime_sn;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

  // Remove MAC Connections for all the network devices before to know what of them exist
  prime_network_mutex_lock();
  list_for_each_safe(entry, tmp, &prime_network) {
     p_prime_sn = list_entry(entry, prime_sn, list);
     prime_sn_del_mac_connections(p_prime_sn);
  }
  prime_network_mutex_unlock();

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_list_get_request_sync(PIB_MAC_LIST_ACTIVE_CONN,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_list_get_request_sync(PIB_MAC_LIST_ACTIVE_CONN,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macListActiveConn request success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macListActiveConn request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macListActiveConnEx PIB Attribute
 *        List of active non-direct connections.
 *        This list is maintained by the Base Node only. Extended version.
 */
DEFUN (prime_pib_mac_get_macListActiveConnEx,
       prime_pib_mac_get_macListActiveConnEx_cmd,
       "pib mac get (mlme|mngp) macListActiveConnEx",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Refresh macListActiveConnEx\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
mchp_list *entry, *tmp;
prime_sn * p_prime_sn;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

  // Remove MAC Connections for all the network devices before to know what of them exist
  prime_network_mutex_lock();
  list_for_each_safe(entry, tmp, &prime_network) {
     p_prime_sn = list_entry(entry, prime_sn, list);
     prime_sn_del_mac_connections(p_prime_sn);
  }
  prime_network_mutex_unlock();

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_list_get_request_sync(PIB_MAC_LIST_ACTIVE_CONN_EX,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_list_get_request_sync(PIB_MAC_LIST_ACTIVE_CONN_EX,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macListActiveConnEx request success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macListActiveConnEx request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macListMcastEntries PIB Attribute
 *        List of entries in multicast switching table.
 *        This list is not maintained by Service Nodes in a Terminal functional state.
 */
DEFUN (prime_pib_mac_get_macListMcastEntries,
       prime_pib_mac_get_macListMcastEntries_cmd,
       "pib mac get (mlme|mngp) macListMcastEntries",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Refresh macListMcastEntries\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_SWITCH);

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_list_get_request_sync(PIB_MAC_LIST_MCAST_ENTRIES,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_list_get_request_sync(PIB_MAC_LIST_MCAST_ENTRIES,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macListMcastEntries request success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macListMcastEntries request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macListMcastEntries,
       prime_bmng_pib_mac_get_macListMcastEntries_cmd,
       "pib mac get bmng MAC macListMcastEntries",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "Refresh macListMcastEntries\n"
     )
{
  struct TmacGetConfirm x_pib_confirm;
  uint8_t  mac[6];
  prime_sn * sn = NULL;
  /*********************************************
  *       Code                                 *
  **********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_list_request_sync(mac,PIB_MAC_LIST_MCAST_ENTRIES,0,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macListMcastEntries request success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macListMcastEntries request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macListSwitchTable PIB Attribute
 *        List the Switch table.
 *        This list is not maintained by Service Nodes in a Terminal functional state.
 */
DEFUN (prime_pib_mac_get_macListSwitchTable,
       prime_pib_mac_get_macListSwitchTable_cmd,
       "pib mac get (mlme|mngp) macListSwitchTable",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Refresh macListSwitchTable\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_SWITCH);

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_list_get_request_sync(PIB_MAC_LIST_SWITCH_TABLE,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_list_get_request_sync(PIB_MAC_LIST_SWITCH_TABLE,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macListSwitchTable request success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macListSwitchTable request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macListSwitchTable,
       prime_bmng_pib_mac_get_macListSwitchTable_cmd,
       "pib mac get bmng MAC macListSwitchTable",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "Refresh macListSwitchTable\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_list_request_sync(mac,PIB_MAC_LIST_SWITCH_TABLE,0,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macListSwitchTable request success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macListSwitchTable request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macListDirectConn PIB Attribute
 *        List of direct connections that are active.
 *        This list is maintained only in the Base Node.
 */
DEFUN (prime_pib_mac_get_macListDirectConn,
       prime_pib_mac_get_macListDirectConn_cmd,
       "pib mac get (mlme|mngp) macListDirectConn",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Refresh macListDirectConn\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_list_get_request_sync(PIB_MAC_LIST_DIRECT_CONN,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_list_get_request_sync(PIB_MAC_LIST_DIRECT_CONN,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macListDirectConn request success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macListDirectConn request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macListDirectTable PIB Attribute
 *        List the direct Switch table
 */
DEFUN (prime_pib_mac_get_macListDirectTable,
       prime_pib_mac_get_macListDirectTable_cmd,
       "pib mac get (mlme|mngp) macListDirectTable",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Refresh macListDirectTable\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_TERMINAL);

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_list_get_request_sync(PIB_MAC_LIST_DIRECT_TABLE,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_list_get_request_sync(PIB_MAC_LIST_DIRECT_TABLE,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macListDirectTable request success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macListDirectTable request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macListDirectTable,
       prime_bmng_pib_mac_get_macListDirectTable_cmd,
       "pib mac get bmng MAC macListDirectTable",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "Refresh macListDirectTable\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
  }
  sn = prime_network_find_sn(&prime_network,mac);
  if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
  }
  prime_bmng_pprof_get_list_request_sync(mac,PIB_MAC_LIST_DIRECT_TABLE,0,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macListDirectTable request success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macListDirectTable request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macListAvailableSwitches PIB Attribute
 *        List of Switch Nodes whose beacons are received.
 */
DEFUN (prime_pib_mac_get_macListAvailableSwitches,
       prime_pib_mac_get_macListAvailableSwitches_cmd,
       "pib mac get (mlme|mngp) macListAvailableSwitches",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Refresh macListAvailableSwitches\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_TERMINAL);

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_list_get_request_sync(PIB_MAC_LIST_AVAILABLE_SWITCHES,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_list_get_request_sync(PIB_MAC_LIST_AVAILABLE_SWITCHES,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macListAvailableSwitches request success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macListAvailableSwitches request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macListAvailableSwitches,
       prime_bmng_pib_mac_get_macListAvailableSwitches_cmd,
       "pib mac get bmng MAC macListAvailableSwitches",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "Refresh macListAvailableSwitches\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_list_request_sync(mac,PIB_MAC_LIST_AVAILABLE_SWITCHES,0,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macListAvailableSwitches request success\r\n");
      return CMD_SUCCESS;
   }
   vty_out(vty,"PRIME PIB-MAC macListAvailableSwitches request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Get macListPhyComm PIB Attribute
 *        List of PHY communication parameters.
 *        This table is maintained in every Node.
 *        For Terminal Nodes it contains only one entry for the Switch the Node is connected through.
 *        For other Nodes is contains also entries for every directly connected child Node.
 */
DEFUN (prime_pib_mac_get_macListPhyComm,
       prime_pib_mac_get_macListPhyComm_cmd,
       "pib mac get (mlme|mngp) macListPhyComm",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_PLME
       REQUEST_MNGP
       "Refresh macListPhyComm\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_list_get_request_sync(PIB_MAC_LIST_PHY_COMM,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_list_get_request_sync(PIB_MAC_LIST_PHY_COMM,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macListPhyComm request success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macListPhyComm request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_mac_get_macListPhyComm,
       prime_bmng_pib_mac_get_macListPhyComm_cmd,
       "pib mac get bmng MAC macListPhyComm",
       REQUEST_PIB
       REQUEST_MAC_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "Refresh macListPhyComm\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_list_request_sync(mac,PIB_MAC_LIST_PHY_COMM,0,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macListPhyComm request success\r\n");
      return CMD_SUCCESS;
   }
   vty_out(vty,"PRIME PIB-MAC macListPhyComm request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

// MAC ACTION PIB Attributes
// Some of the conformance tests require triggering certain actions
// on Service Nodes and Base Nodes. The 4514 following table lists
// the set of action attributes that need to be supported by all implementations.

/**
 * \brief Action macActionTxData PIB Attribute
 *        Total number of PPDUs correctly decoded. Useful for PHY layer to estimate FER.
 */
DEFUN (prime_pib_mac_action_macActionTxData,
       prime_pib_mac_action_macActionTxData_cmd,
       "mac action (mlme|mngp) macActionTxData <0-255>",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionTxData\n"
       "macActionTxData Value\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();

  uc_value = (uint8_t) atoi(argv[1]);
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_TX_DATA,&uc_value,1,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_TX_DATA,&uc_value,1,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macActionTxData action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macActionTxData action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionConnClose PIB Attribute
 *        Trigger to close one of the open connections.
 */
DEFUN (prime_pib_mac_action_macActionConnClose,
       prime_pib_mac_action_macActionConnClose_cmd,
       "mac action (mlme|mngp) macActionConnClose <0-255>",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionConnClose\n"
       "Connection Number\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();

  uc_value = (uint8_t) atoi(argv[1]);
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_CONN_CLOSE,&uc_value,1,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_CONN_CLOSE,&uc_value,1,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC MACActionConnClose action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC MACActionConnClose action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionRegReject PIB Attribute
 *        Trigger to reject incoming registration request.
 */
DEFUN (prime_pib_mac_action_macActionRegReject,
       prime_pib_mac_action_macActionRegReject_cmd,
       "mac action (mlme|mngp) macActionRegReject <0-255>",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionRegReject\n"
       "Incoming registration request\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();

  uc_value = (uint8_t) atoi(argv[1]);
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_REG_REJECT,&uc_value,1,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_REG_REJECT,&uc_value,1,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC MACActionRegReject action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC MACActionRegReject action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionProReject PIB Attribute
 *        Trigger to reject incoming promotion request.
 */
DEFUN (prime_pib_mac_action_macActionProReject,
       prime_pib_mac_action_macActionProReject_cmd,
       "mac action (mlme|mngp) macActionProReject <0-255>",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionProReject\n"
       "Incoming Promotion request\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();

  uc_value = (uint8_t) atoi(argv[1]);
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_PRO_REJECT,&uc_value,1,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_PRO_REJECT,&uc_value,1,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC MACActionProReject action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC MACActionProReject action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionUnregister PIB Attribute
 *        Trigger to unregister from the Subnetwork.
 */
DEFUN (prime_pib_mac_action_macActionUnregister,
       prime_pib_mac_action_macActionUnregister_cmd,
       "mac action (mlme|mngp) macActionUnregister <0-255>",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionUnregister\n"
       "Incoming Unregister request\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();

  uc_value = (uint8_t) atoi(argv[1]);
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_UNREGISTER,&uc_value,1,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_UNREGISTER,&uc_value,1,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC MACActionUnregister action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC MACActionUnregister action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionPromote PIB Attribute
 *        Trigger to promote a given Service Node from the Subnetwork.
 */
DEFUN (prime_pib_mac_action_macActionPromote,
       prime_pib_mac_action_macActionPromote_cmd,
       "mac action (mlme|mngp) macActionPromote MAC",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionPromote\n"
       "MAC to send Promote Request\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t mac[6];
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[1],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
  }

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_PROMOTE,mac,6,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_PROMOTE,mac,6,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }

  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC MACActionPromote action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC MACActionPromote action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionDemote PIB Attribute
 *        Trigger to demote a given Service Node from the Subnetwork.
 */
DEFUN (prime_pib_mac_action_macActionDemote,
       prime_pib_mac_action_macActionDemote_cmd,
       "mac action (mlme|mngp) macActionDemote MAC",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionDemote\n"
       "MAC to send Demote Request\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t mac[6];
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (str_to_eui48(argv[1],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
  }
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_DEMOTE,mac,6,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_DEMOTE,mac,6,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC MACActionPromote action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC MACActionPromote action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionReject PIB Attribute
 *        Rejects or stops (toggles) rejecting packets of a certain type
 */
DEFUN (prime_pib_mac_action_macActionReject,
       prime_pib_mac_action_macActionReject_cmd,
       "mac action (mlme|mngp) macActionReject MAC (reject|no_reject) type (pro_req_s|prm|con_req_s|reg_req)",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionReject\n"
       "MAC to send Reject Request\n"
       "Start rejecting\n"
       "Stop rejecting\n"
       "Packet Type\n"
       "Packet Type pro_req_s\n"
       "Packet Type prm\n"
       "Packet Type con_req_s\n"
       "Packet Type reg_req\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t mac[6];
uint8_t buffer[8];
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  VTY_CHECK_PRIME_MTP_MODE();

  if (str_to_eui48(argv[1],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
  }
  memcpy(buffer,mac,6);
  if (strcmp(argv[2],"reject")==0){
    buffer[6] = 1;  // Rejection
  }else{
    buffer[6] = 0;  // No Rejection
  }
  if (strcmp(argv[3],"pro_req_s") == 0){
     buffer[7] = 0;
  }else if (strcmp(argv[3],"prm") == 0){
     buffer[7] = 1;
  }else if (strcmp(argv[3],"con_req_s") == 0){
     buffer[7] = 2;
  }else{
     buffer[7] = 3;
  }

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_REJECT,buffer,8,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_REJECT,buffer,8,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC MACActionReject action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC MACActionReject action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionAliveTime PIB Attribute
 *        Forces alive time for the network or sets it as automatic.
 */
DEFUN (prime_pib_mac_action_macActionAliveTime,
       prime_pib_mac_action_macActionAliveTime_cmd,
       "mac action (mlme|mngp) macActionAliveTime <0-255>",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionAliveTime\n"
       "Alive Time request\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/

  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  VTY_CHECK_PRIME_MTP_MODE();

  uc_value = (uint8_t) atoi(argv[1]);
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_ALIVE_TIME,&uc_value,1,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_ALIVE_TIME,&uc_value,1,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macActionAliveTime action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macActionAliveTime action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionBroadcastDataBurst PIB Attribute
 *        Send a burst of data PDU-s with a test sequence using broadcast
 */
DEFUN (prime_pib_mac_action_macActionBroadcastDataBurst,
       prime_pib_mac_action_macActionBroadcastDataBurst_cmd,
       "mac action (mlme|mngp) macActionBroadcastDataBurst number NUM length LEN duty <0-100> lcid LCID priority PRIO",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionBroadcastDataBurst\n"
       "Number of PDUs to be sent\n"
       "Number of PDUs to be sent value\n"
       "Size of data packet to send\n"
       "Size of data packet to send value\n"
       "Average Duty Cycle\n"
       "Average Duty Cycle value\n"
       "LCID\n"
       "LCID value\n"
       "Priority\n"
       "Priority value\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint32_t ui_value;
uint16_t us_value;
//uint8_t  uc_value;
uint8_t  buffer[9];
/*********************************************
*       Code                                 *
**********************************************/

  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  VTY_CHECK_PRIME_MTP_MODE();

  ui_value  = (uint32_t) atoi(argv[1]);
  memcpy(&buffer[0],&ui_value,4);
  buffer[4] = (uint8_t) atoi(argv[2]);
  buffer[5] = (uint8_t) atoi(argv[3]);
  us_value  = (uint16_t) atoi(argv[4]);
  memcpy(&buffer[6],(uint8_t *) &us_value,2);
  buffer[8] = (uint8_t) atoi(argv[5]);

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_BROADCAST_DATA_BURST,buffer,9,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_BROADCAST_DATA_BURST,buffer,9,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macActionBroadcastDataBurst action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macActionBroadcastDataBurst action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionMgmtCon PIB Attribute
 *        Forces establishment/close of the management connection
 */
DEFUN (prime_pib_mac_action_macActionMgmtCon,
       prime_pib_mac_action_macActionMgmtCon_cmd,
       "mac action (mlme|mngp) macActionMgmtCon MAC (close|open)",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionMgmtCon\n"
       "MAC to close/open management connection\n"
       "Close management connection\n"
       "Open management connection\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
//uint8_t uc_value;
uint8_t mac[6];
uint8_t buffer[7];
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  VTY_CHECK_PRIME_MTP_MODE();

  if (str_to_eui48(argv[1],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
  }
  memcpy(buffer,mac,6);
  if (strcmp(argv[2],"close")==0){
    buffer[6] = 0;  // Close the Management Connection
  }else{
    buffer[6] = 1;  // Open the Management Connection
  }

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_MGMT_CON,buffer,7,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_MGMT_CON,buffer,7,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macActionMgmtCon action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macActionMgmtCon action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionMgmtMul PIB Attribute
 *        Forces establishment/close of the management multicast connection
 */
DEFUN (prime_pib_mac_action_macActionMgmtMul,
       prime_pib_mac_action_macActionMgmtMul_cmd,
       "mac action (mlme|mngp) macActionMgmtMul MAC (leave|join)",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionMgmtMul\n"
       "MAC to join/leave multicast management connection\n"
       "Leave multicast management connection\n"
       "Join multicast management connection\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
//uint8_t uc_value;
uint8_t mac[6];
uint8_t buffer[7];
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  VTY_CHECK_PRIME_MTP_MODE();

  if (str_to_eui48(argv[1],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
  }
  memcpy(buffer,mac,6);
  if (strcmp(argv[2],"close")==0){
    buffer[6] = 0;  // Close the Management Connection
  }else{
    buffer[6] = 1;  // Open the Management Connection
  }

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_MGMT_MUL,buffer,7,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_MGMT_MUL,buffer,7,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macActionMgmtMul action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macActionMgmtMul action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionUnregisterBN PIB Attribute
 *        Trigger to unregister a given Service Node from the Subnetwork.
 */
DEFUN (prime_pib_mac_action_macActionUnregisterBN,
       prime_pib_mac_action_macActionUnregisterBN_cmd,
       "mac action (mlme|mngp) macActionUnregisterBN MAC",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionUnregisterBN\n"
       "MAC to unregister\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
//uint8_t uc_value;
uint8_t mac[6];
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

  if (str_to_eui48(argv[1],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
  }
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_UNREGISTER_BN,mac,6,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_UNREGISTER_BN,mac,6,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macActionUnregisterBN action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macActionUnregisterBN action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionConnClose PIB Attribute
 *        Trigger to close an open connection.
 */
DEFUN (prime_pib_mac_action_macActionConnCloseBN,
       prime_pib_mac_action_macActionConnCloseBN_cmd,
       "mac action (mlme|mngp) macActionConnClose MAC lcid LCID",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionConnClose\n"
       "MAC to close the connection\n"
       "LCID of the connection\n"
       "LCID of the connection value\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
//uint8_t uc_value;
uint8_t mac[6];
uint8_t buffer[8];
uint16_t lcid;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

  if (str_to_eui48(argv[1],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
  }
  memcpy(buffer,mac,6);
  lcid = (uint16_t) atoi(argv[2]);
  memcpy(&buffer[6],(uint8_t *) &lcid,2);

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_CONN_CLOSE_BN,buffer,8,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_CONN_CLOSE_BN,buffer,8,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macActionConnCloseBN action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macActionConnCloseBN action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionSegmented432 PIB Attribute
 *        Trigger data transfer whit segmentation mechanism working (Convergence Layer)
 •        Transmit PPDUs over established CL 4-32 Connection (with segmentation)
 •       Trigger at least 1 packet segmented in at least 3 frames
 */
DEFUN (prime_pib_mac_action_macActionSegmented432,
       prime_pib_mac_action_macActionSegmented432_cmd,
       "mac action (mlme|mngp) macActionSegmented432 MAC LEN",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionSegmented432\n"
       "MAC to transmit data\n"
       "Length of the data being transmitted\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
//uint8_t uc_value;
uint8_t mac[6];
uint8_t buffer[8];
uint16_t length;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  VTY_CHECK_PRIME_MTP_MODE();

  if (str_to_eui48(argv[1],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
  }
  memcpy(buffer,mac,6);
  length = (uint16_t) atoi(argv[2]);
  memcpy(&buffer[6],&length,2);

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_SEGMENTED_432,buffer,8,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_SEGMENTED_432,buffer,8,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macActionSegmented432 action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macActionSegmented432 action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionAppemuDataBurst PIB Attribute
 *        Send a burst of data PDU-s with a test sequence using the Appemu connection to the node (if any)
 *        The data shall be transmitted with the flush bit to cero (0) when possible.
 */
DEFUN (prime_pib_mac_action_macActionAppemuDataBurst,
       prime_pib_mac_action_macActionAppemuDataBurst_cmd,
       "mac action (mlme|mngp) macActionAppemuDataBurst MAC number NUM length LEN duty DUTY",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionAppemuDataBurst\n"
       "MAC to transmit data\n"
       "Number of frames\n"
       "Number of frames value\n"
       "Length of the frame\n"
       "Length of rhe frame value\n"
       "Duty Cycle\n"
       "Duty Cycle value\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint32_t number;
uint8_t  mac[6];
uint8_t  buffer[12];
uint8_t  length,duty;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  VTY_CHECK_PRIME_MTP_MODE();

  if (str_to_eui48(argv[1],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
  }
  memcpy(buffer,mac,6);
  number = atoi(argv[2]);
  memcpy(&buffer[6],&number,4);

  length = (uint8_t) atoi(argv[3]);
  buffer[10] = length;

  duty = (uint8_t) atoi(argv[4]);
  buffer[11] = duty;

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_APPEMU_DATA_BURST,buffer,12,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_APPEMU_DATA_BURST,buffer,12,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macActionAppemuDataBurst action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macActionAppemuDataBurst action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Action macActionMgmtDataBurst PIB Attribute
 *        Send a burst of data PDU-s with a test sequence using the Management connection to the node (if any).
 *        The data shall be transmitted with the flush bit set to zero (0) when possible.
 */
DEFUN (prime_pib_mac_action_macActionMgmtDataBurst,
       prime_pib_mac_action_macActionMgmtDataBurst_cmd,
       "mac action (mlme|mngp) macActionMgmtDataBurst MAC number NUM length LEN duty DUTY",
       REQUEST_MAC_PIB
       REQUEST_ACTION_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "macActionMgmtDataBurst\n"
       "MAC to transmit data\n"
       "Number of frames\n"
       "Number of frames value\n"
       "Length of the frame\n"
       "Length of rhe frame value\n"
       "Duty Cycle\n"
       "Duty Cycle value\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacSetConfirm x_pib_confirm;
uint32_t number;
uint8_t  mac[6];
uint8_t  buffer[12];
uint8_t  length,duty;
/*********************************************
*       Code                                 *
**********************************************/

  VTY_CHECK_PRIME_MTP_MODE();
  if (str_to_eui48(argv[1],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
  }
  memcpy(buffer,mac,6);
  number = atoi(argv[2]);
  memcpy(&buffer[6],&number,4);

  length = (uint8_t) atoi(argv[3]);
  buffer[10] = length;

  duty = (uint8_t) atoi(argv[4]);
  buffer[11] = duty;

  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_set_request_sync(PIB_MAC_ACTION_MGMT_DATA_BURST,buffer,12,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_set_request_sync(PIB_MAC_ACTION_MGMT_DATA_BURST,buffer,12,PRIME_SYNC_TIMEOUT_SET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"PRIME PIB-MAC macActionMgmtDataBurst action success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"PRIME PIB-MAC macActionMgmtDataBurst action failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

// 4-32 Convergence Layers
/**
 * \brief Application AppFwVersion PIB Attribute
 *        Textual description of firmware version running on device.
 */
DEFUN (prime_pib_cl432_get_swVersion,
       prime_pib_cl432_get_swVersion_cmd,
       "pib cl432 get (mlme|mngp) swVersion",
       REQUEST_PIB
       REQUEST_CL432_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "swVersion\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct  TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_432_INTERNAL_SW_VERSION,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_432_INTERNAL_SW_VERSION,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 16 Byte should be the result
      memcpy(&ui_value, &x_pib_confirm.m_au8AttributeValue[0], 4);
      vty_out(vty,"PRIME PIB-CL432 SwVersion: 0x%08X\r\n",ui_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-CL432 SwVersion (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_cl432_get_swVersion,
       prime_bmng_pib_cl432_get_swVersion_cmd,
       "pib cl432 get bmng MAC swVersion",
       REQUEST_PIB
       REQUEST_CL432_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "CL4-32 API Version\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct   TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_432_INTERNAL_SW_VERSION,0,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 16 Byte should be the result
      memcpy(&ui_value, &x_pib_confirm.m_au8AttributeValue[0], 4);
      vty_out(vty,"PRIME PIB-CL432 swVersion: 0x%08X\r\n",ui_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-CL432 swVersion (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_pib_cl432_get_connState,
       prime_pib_cl432_get_connState_cmd,
       "pib cl432 get (mlme|mngp) connState",
       REQUEST_PIB
       REQUEST_CL432_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "CL4-32 Connection State\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct  TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
  VTY_CHECK_PRIME_MODE_NO_BASE();
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_432_CON_STATE,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_432_CON_STATE,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte should be the result
      vty_out(vty,"PRIME PIB-CL432 Connection State: 0x%02X(%s)\r\n",x_pib_confirm.m_au8AttributeValue[0],cl432_conn_state_str[x_pib_confirm.m_au8AttributeValue[0]]);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-CL432 Connection State (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

 /**
  * \brief Get cl432ListNodes PIB Attribute
  *        List of active 4-32 Connections
  *        This list is maintained by the Base Node only. Extended version.
  */
 DEFUN (prime_pib_cl432_get_cl432ListNodes,
          prime_pib_cl432_get_cl432ListNodes_cmd,
          "pib cl432 get (mlme|mngp) cl432ListNodes",
          REQUEST_PIB
          REQUEST_CL432_PIB
          REQUEST_GET_PIB
          REQUEST_MLME
          REQUEST_MNGP
          "Refresh CL4-32 Node List\n"
        )
   {
   /*********************************************
   *       Local Envars                         *
   **********************************************/
   struct TmacGetConfirm x_pib_confirm;
   mchp_list *entry, *tmp;
   prime_sn * p_prime_sn;
   /*********************************************
   *       Code                                 *
   **********************************************/
     VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

     // Remove open connection flag for all the network devices before to know what of them are 4-32 connection
     prime_network_mutex_lock();
     list_for_each_safe(entry, tmp, &prime_network) {
        p_prime_sn = list_entry(entry, prime_sn, list);
        p_prime_sn->cl432Conn.connState = CL432_CONN_STATE_CLOSED;
        p_prime_sn->cl432Conn.connAddress = CL_432_INVALID_ADDRESS;
     }
     prime_network_mutex_unlock();

     if (strcmp(argv[0],"mlme") == 0){
        prime_cl_null_mlme_list_get_request_sync(PIB_432_LIST_NODES,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
     }else{
        prime_mngp_list_get_request_sync(PIB_432_LIST_NODES,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST, &x_pib_confirm);
     }
     if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
         vty_out(vty,"PRIME PIB-MAC cl432ListNodes request success\r\n");
         return CMD_SUCCESS;
     }
     vty_out(vty,"PRIME PIB-MAC cl432ListNodes request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
     return CMD_ERR_NOTHING_TODO;
   }

// APPLICATION PIB attributes

/**
 * \brief Application AppFwVersion PIB Attribute
 *        Textual description of firmware version running on device.
 */
DEFUN (prime_pib_app_get_AppFwVersion,
       prime_pib_app_get_AppFwVersion_cmd,
       "pib app get (mlme|mngp) appFwVersion",
       REQUEST_PIB
       REQUEST_APP_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "AppFwVersion\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct  TmacGetConfirm x_pib_confirm;
uint8_t buffer[16];
char    str_value[32];
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_APP_FW_VERSION,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_APP_FW_VERSION,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 16)){
      // 16 Byte should be the result
      memcpy(buffer, &x_pib_confirm.m_au8AttributeValue[0], 16);
      snprintf(str_value,16,"%s",buffer);
      vty_out(vty,"PRIME PIB-APP AppFwVersion: %s\r\n",str_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-APP AppFwVersion (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_app_get_AppFwVersion,
       prime_bmng_pib_app_get_AppFwVersion_cmd,
       "pib app get bmng MAC appFwVersion",
       REQUEST_PIB
       REQUEST_APP_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "AppFwVersion\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct   TmacGetConfirm x_pib_confirm;
uint8_t  buffer[16];
char     str_value[32];
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_APP_FW_VERSION,0,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 16)){
      // 16 Byte should be the result
      memcpy(buffer, &x_pib_confirm.m_au8AttributeValue[0], 16);
      snprintf(str_value,16,"%s",buffer);
      vty_out(vty,"PRIME PIB-APP AppFwVersion: %s\r\n",str_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-APP AppFwVersion (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Application AppVendorId PIB Attribute
 *        PRIME Alliance assigned unique vendor identifier.
 */
DEFUN (prime_pib_app_get_AppVendorId,
       prime_pib_app_get_AppVendorId_cmd,
       "pib app get (mlme|mngp) appVendorId",
       REQUEST_PIB
       REQUEST_APP_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "AppVendorId\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_APP_VENDOR_ID,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_APP_VENDOR_ID,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0]<<8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-APP AppVendorId: 0x%04X\r\n", us_value );
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-APP AppVendorId (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_app_get_AppVendorId,
       prime_bmng_pib_app_get_AppVendorId_cmd,
       "pib app get bmng MAC appVendorId",
       REQUEST_PIB
       REQUEST_APP_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "AppVendorId\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_APP_VENDOR_ID,0,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0]<<8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-APP AppVendorId: 0x%04X\r\n", us_value );
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-APP AppVendorId (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Application AppProductId PIB Attribute
 *        Vendor assigned unique identifier for specific product.
 */
DEFUN (prime_pib_app_get_AppProductId,
       prime_pib_app_get_AppProductId_cmd,
       "pib app get (mlme|mngp) appProductId",
       REQUEST_PIB
       REQUEST_APP_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "AppProductId\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_APP_PRODUCT_ID,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_APP_PRODUCT_ID,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  }
  if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0]<<8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-APP AppProductId: 0x%04X\r\n", us_value);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-APP AppProductId (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_app_get_AppProductId,
       prime_bmng_pib_app_get_AppProductId_cmd,
       "pib app get bmng MAC appProductId",
       REQUEST_PIB
       REQUEST_APP_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "AppProductId\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_APP_PRODUCT_ID,0,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
   if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      us_value = (x_pib_confirm.m_au8AttributeValue[0]<<8) + x_pib_confirm.m_au8AttributeValue[1];
      vty_out(vty,"PRIME PIB-APP AppProductId: 0x%04X\r\n", us_value);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-APP AppProductId (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Application AppListZCStatus PIB Attribute
 *        It returns only one element of the list together with the zero crossing time
 */
DEFUN (prime_pib_app_get_AppListZCStatus,
       prime_pib_app_get_AppListZCStatus_cmd,
       "pib app get (mlme|mngp) appListZCStatus",
       REQUEST_PIB
       REQUEST_APP_PIB
       REQUEST_GET_PIB
       REQUEST_MLME
       REQUEST_MNGP
       "AppListZCStatus\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (strcmp(argv[0],"mlme") == 0){
     prime_cl_null_mlme_get_request_sync(PIB_MAC_APP_LIST_ZC_STATUS,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  }else{
     prime_mngp_get_request_sync(PIB_MAC_APP_LIST_ZC_STATUS,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  }
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte ZCStatus
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-APP ZCStatus: \r\n\t- Terminal Block:%d\r\n\t- Direction:%s\r\n\t- Status:%s\r\n", ((uc_value>>5) & 0x3), zcstatus_dir_str[((uc_value>>3) & 0x3)], zcstatus_status_str[(uc_value & 0x7)]);
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to get PRIME PIB-APP AppListZCStatus (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_bmng_pib_app_get_AppListZCStatus,
       prime_bmng_pib_app_get_AppListZCStatus_cmd,
       "pib app get bmng MAC AppListZCStatus",
       REQUEST_PIB
       REQUEST_APP_PIB
       REQUEST_GET_PIB
       REQUEST_BMNG
       "Service Node EUI48\r\n"
       "AppListZCStatus\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t  uc_value;
uint8_t  mac[6];
prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_pprof_get_request_sync(mac,PIB_MAC_APP_LIST_ZC_STATUS,0,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      // 1 Byte ZCStatus
      uc_value = x_pib_confirm.m_au8AttributeValue[0];
      vty_out(vty,"PRIME PIB-APP ZCStatus: \r\n\t- Terminal Block:%d\r\n\t- Direction:%s\r\n\t- Status:%s\r\n", ((uc_value>>5) & 0x3), zcstatus_dir_str[((uc_value>>3) & 0x3)], zcstatus_status_str[(uc_value & 0x7)]);
      return CMD_SUCCESS;
   }
   vty_out(vty,"Impossible to get PRIME PIB-APP AppListZCStatus (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

// PRIME PLME Primitives

// The PLME RESET Request Primitive is invoked to request the PHY layer
// to reset its present functional state
DEFUN (prime_plme_reset_request,
       prime_plme_reset_request_cmd,
       "phy reset",
       "PRIME PHY PLME Request Primitives\n"
       "PRIME PLME Reset Request\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
  prime_cl_null_plme_reset_request_sync(PRIME_SYNC_TIMEOUT_RESET_REQUEST, &x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      vty_out(vty,"PRIME PHY reset success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to reset PRIME PHY (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

// The PLME SLEEP Request primitive is invoked to request the PHY layer
// to suspend its present activities 865 including all reception functions
DEFUN (prime_plme_sleep_request,
       prime_plme_sleep_request_cmd,
       "phy sleep",
       "PRIME PHY PLME Request Primitives\n"
       "PRIME PLME Sleep Request\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
  prime_cl_null_plme_sleep_request_sync(PRIME_SYNC_TIMEOUT_SLEEP_REQUEST, &x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      vty_out(vty,"PRIME PHY sleep success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to sleep PRIME PHY (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

// The PLME_RESUME.request primitive is invoked to request the PHY layer
// to resume its suspended activities
DEFUN (prime_plme_resume_request,
       prime_plme_resume_request_cmd,
       "phy resume",
       "PRIME PHY PLME Request Primitives\n"
       "PRIME PLME Resume Request\n"
     )
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
  prime_cl_null_plme_resume_request_sync(PRIME_SYNC_TIMEOUT_RESUME_REQUEST, &x_pib_confirm);
  if (x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS){
      vty_out(vty,"PRIME PHY resume success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"Impossible to resume PRIME PHY (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

// PRIME MLME Primitives

// The MLME Unregister Request primitive is used to trigger the unregister process
// in a Service Node that is in a Terminal functional state.
DEFUN (prime_mlme_unregister_request,
       prime_mlme_unregister_request_cmd,
       "mlme remote unregister",
       "PRIME MAC MLME Request Primitives\n"
       "Remote\n"
       "PRIME MLME Unregister Request\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/

  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  prime_cl_null_mlme_unregister_request_sync(&x_pib_confirm);

  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"Unregister Request Success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"Unregister request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

// The MLME PROMOTE Request primitive is used to trigger the promotion process
// in a Service Node that is in 2545 a Terminal functional state.
DEFUN (prime_mlme_promote_request,
       prime_mlme_promote_request_cmd,
       "mac promote MAC bcn_mode (null|dbpsk_f|r_dbpsk|r_dqpsk)",
       "PRIME MAC MLME Request Primitives\n"
       "PRIME MLME Promote Request\n"
       "PRIME EUI48 MAC Address\n"
       "Beacon PDU Modulation Scheme\n"
       "Beacon PDU Modulation Scheme null(0)\n"
       "Beacon PDU Modulation Scheme DBPSK_F(4)\n"
       "Beacon PDU Modulation Scheme R_DBPSK(8)\n"
       "Beacon PDU Modulation Scheme R_DQPSK(9)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint8_t uc_bcn_mode;
uint8_t mac[6];
/*********************************************
*       Code                                 *
**********************************************/
  if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
  }
  if (strcmp(argv[1],"null") == 0){
    uc_bcn_mode=0;
  }else if (strcmp(argv[1],"dbpsk_f") == 0){
    uc_bcn_mode=4;
  }else if (strcmp(argv[1],"r_dbpsk") == 0){
    uc_bcn_mode=8;
  }else{
    uc_bcn_mode=9;
  }
  prime_cl_null_mlme_promote_request_sync(mac, uc_bcn_mode, PRIME_SYNC_TIMEOUT_DEFAULT, &x_pib_confirm);

  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"Promotion Request Success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"Promote request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

// The MLME_DEMOTE.request primitive is used to trigger a demotion process in a
// Service Node that is in a 2589 Switch functional state
DEFUN (prime_mlme_demote_request,
       prime_mlme_demote_request_cmd,
       "mac demote",
       "PRIME MAC MLME Request Primitives\n"
       "PRIME MLME Demote Request\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/

  prime_cl_null_mlme_demote_request_sync(PRIME_SYNC_TIMEOUT_DEFAULT,&x_pib_confirm);
  if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"Demotion Request Success\r\n");
      return CMD_SUCCESS;
  }
  vty_out(vty,"Demote request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
  return CMD_ERR_NOTHING_TODO;
}

// The MLME_RESET.request primitive is used to trigger a Reset to MAC
DEFUN (prime_mlme_reset_request,
       prime_mlme_reset_request_cmd,
       "mac reset",
       "PRIME MAC MLME Request Primitives\n"
       "PRIME MAC Reset Request\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
   prime_cl_null_mlme_reset_request_sync(PRIME_SYNC_TIMEOUT_RESET_REQUEST, &x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
      vty_out(vty,"Reset Request Success\r\n");
      return CMD_SUCCESS;
   }
   vty_out(vty,"Reset request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

// Reboot Base Node Modem or Remote Service Node
DEFUN (prime_modem_reboot_request,
       prime_modem_reboot_request_cmd,
       "reboot",
       "Reboot PRIME Modem\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
/*********************************************
*       Code                                 *
**********************************************/
  prime_mngp_reboot_request();
  return CMD_ERR_NOTHING_TODO;
}

DEFUN (prime_modem_bmng_reboot_request,
       prime_modem_bmng_reboot_request_cmd,
       "reboot MAC",
       "Reboot PRIME Modem\n"
       "Service Node EUI48\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
uint8_t  mac[6];
prime_sn * sn = NULL;
struct TmacSetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn == NULL){
     vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   prime_bmng_reboot_request_sync(mac, PRIME_SYNC_TIMEOUT_REBOOT_REQUEST, &x_pib_confirm);
   if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
       vty_out(vty,"Reboot Request Success\r\n");
       return CMD_SUCCESS;
   }
   vty_out(vty,"Reboot Request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
   return CMD_ERR_NOTHING_TODO;
}

/**
 * \brief Functions related with Statisics
 *
 */

/**
* \brief Show Physical Statistics
*
*/

DEFUN (prime_show_phy_statistics,
       prime_show_phy_statistics_cmd,
       "show phy statistics",
       PRIME_SHOW_STR
       "PRIME PHY\n"
       "PRIME PHY Statistics\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint16_t us_value;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  vty_out(vty,"PRIME PHY Counters\r\n");
  prime_cl_null_plme_get_request_sync(PIB_PHY_STATS_CRC_INCORRECT,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  if ((x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      vty_out(vty,"\t* phyStatsCRCIncorrectCount: %u\r\n",us_value);
  }else{
      vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsCRCIncorrectCount\r\n");
  }

  prime_cl_null_plme_get_request_sync(PIB_PHY_STATS_CRC_FAIL_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  if ((x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      vty_out(vty,"\t* phyStatsCRCFailCount     : %u\r\n",us_value);
  }else{
      vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsCRCFailCount\r\n");
  }

  prime_cl_null_plme_get_request_sync(PIB_PHY_STATS_TX_DROP_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  if ((x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      vty_out(vty,"\t* phyStatsTxDropCount      : %u\r\n",us_value);
  }else{
      vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsTxDropCount\r\n");
  }

  prime_cl_null_plme_get_request_sync(PIB_PHY_STATS_RX_DROP_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  if ((x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS) && (x_pib_confirm.m_u8AttributeLength == 2)){
      // 2 Byte should be the result
      memcpy(&us_value,x_pib_confirm.m_au8AttributeValue,2);
      vty_out(vty,"\t* phyStatsRxDropCount      : %u\r\n",us_value);
  }else{
      vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsRxDropCount\r\n");
  }

  prime_cl_null_plme_get_request_sync(PIB_PHY_STATS_RX_TOTAL_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  if ((x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS) && (x_pib_confirm.m_u8AttributeLength == 4)){
      // 4 Byte should be the result
      memcpy(&ui_value,x_pib_confirm.m_au8AttributeValue,4);
      vty_out(vty,"\t* phyStatsRxTotalCount     : %u\r\n",ui_value);
  }else{
      vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsRxTotalCount\r\n");
  }
  return CMD_SUCCESS;
}

/**
* \brief Show MAC Statistics
*
*/
DEFUN (prime_show_mac_statistics,
       prime_show_mac_statistics_cmd,
       "show mac statistics",
       PRIME_SHOW_STR
       "PRIME MAC\n"
       "PRIME MAC Statistics\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
uint32_t ui_value;
/*********************************************
*       Code                                 *
**********************************************/
  vty_out(vty,"PRIME MAC Counters\r\n");
  prime_cl_null_mlme_get_request_sync(PIB_MAC_TX_DATAPKT_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  if ((x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS) && (x_pib_confirm.m_u8AttributeLength == 4)){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"\t* macTxDataPktCount : %u\r\n",ui_value);
  }else{
      vty_out(vty,"Impossible to get PRIME PIB-PHY phyStatsRxTotalCount\r\n");
  }
  prime_cl_null_mlme_get_request_sync(PIB_MAC_RX_DATAPKT_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  if ((x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS) && (x_pib_confirm.m_u8AttributeLength == 4)){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"\t* MacRxDataPktCount : %u\r\n",ui_value);
  }else{
      vty_out(vty,"Impossible to get PRIME PIB-PHY MacRxDataPktCount\r\n");
  }
  prime_cl_null_mlme_get_request_sync(PIB_MAC_TX_CTRLPKT_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  if ((x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS) && (x_pib_confirm.m_u8AttributeLength == 4)){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"\t* MacTxCtrlPktCount : %u\r\n",ui_value);
  }else{
      vty_out(vty,"Impossible to get PRIME PIB-PHY MacTxCtrlPktCount\r\n");
  }
  prime_cl_null_mlme_get_request_sync(PIB_MAC_RX_CTRLPKT_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  if ((x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS) && (x_pib_confirm.m_u8AttributeLength == 4)){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"\t* MacRxCtrlPktCount : %u\r\n",ui_value);
  }else{
      vty_out(vty,"Impossible to get PRIME PIB-PHY MacRxCtrlPktCount\r\n");
  }
  prime_cl_null_mlme_get_request_sync(PIB_MAC_CSMA_FAIL_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  if ((x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS) && (x_pib_confirm.m_u8AttributeLength == 4)){
      // 4 Byte should be the result
      memcpy(&ui_value,&x_pib_confirm.m_au8AttributeValue[0],4);
      vty_out(vty,"\t* MacCSMAFailCount  : %u\r\n",ui_value);
  }else{
      vty_out(vty,"Impossible to get PRIME PIB-PHY MacCSMAFailCount\r\n");
  }
  prime_cl_null_mlme_get_request_sync(PIB_MAC_CSMA_CH_BUSY_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
  if ((x_pib_confirm.m_u8Status == PLME_RESULT_SUCCESS) && (x_pib_confirm.m_u8AttributeLength == 4)){
      // 4 Byte should be the result
      memcpy(&ui_value,x_pib_confirm.m_au8AttributeValue,4);
      vty_out(vty,"\t* MacCSMAChBusyCount: %u\r\n",ui_value);
  }else{
      vty_out(vty,"Impossible to get PRIME PIB-PHY MacCSMAChBusyCount\r\n");
  }
  return CMD_SUCCESS;
}

/**
* \brief Reset All Statistics
*
*/

DEFUN (prime_reset_all_statistics,
       prime_reset_all_statistics_cmd,
       "reset statistics",
       "Reset\r\n"
       "Reset all Statistics\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
/*********************************************
*       Code                                 *
**********************************************/
  prime_mngp_reset_stats_request();
  return CMD_SUCCESS;
}

/**
* \brief Reset Physical Statistics
*
*/

DEFUN (prime_reset_phy_statistics,
       prime_reset_phy_statistics_cmd,
       "phy reset statistics",
       "PRIME PHY\r\n"
       "Reset\r\n"
       "PRIME PHY Statistics\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
/*********************************************
*       Code                                 *
**********************************************/
  prime_mngp_reset_phy_stats_request();
  return CMD_SUCCESS;
}

/**
* \brief Reset Physical Statistics
*
*/

DEFUN (prime_reset_mac_statistics,
       prime_reset_mac_statistics_cmd,
       "mac reset statistics",
       "PRIME MAC\r\n"
       "Reset\r\n"
       "PRIME MAC Statistics\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
/*********************************************
*       Code                                 *
**********************************************/
  prime_mngp_reset_mac_stats_request();
  return CMD_SUCCESS;
}

/**
 * \brief Zero Crossing Commands
 *
 */
DEFUN (prime_modem_bmng_zc_request,
        prime_modem_bmng_zc_request_cmd,
        "target MAC zero-cross",
        "Target\n"
        "Service Node EUI48\n"
        "Zero Crossing PRIME Modem request\n")
{
 /*********************************************
 *       Local Envars                         *
 **********************************************/
 uint8_t  mac[6];
 prime_sn * sn = NULL;
 struct TmacSetConfirm x_pib_confirm;
 /*********************************************
 *       Code                                 *
 **********************************************/
    VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
    if (str_to_eui48(argv[0],mac)){
      vty_out(vty,"Service Node EUI48 length is wrong\r\n");
      return CMD_ERR_NOTHING_TODO;
    }
    sn = prime_network_find_sn(&prime_network,mac);
    if (sn == NULL){
      vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
      return CMD_ERR_NOTHING_TODO;
    }
    prime_bmng_zero_cross_request_sync(mac, PRIME_SYNC_TIMEOUT_ZC_REQUEST, &x_pib_confirm);
    if (x_pib_confirm.m_u8Status == MLME_RESULT_DONE){
        //vty_out(vty,"ZeroCross Request Success\r\n");
        /* Information is already on structure */
        vty_out(vty, "\t- Status:%s\r\n\t- Terminal Block:%d\r\n\t- Direction:%s\r\n", zcstatus_status_str[(sn->uc_zc_status & 0x7)], ((sn->uc_zc_status>>5) & 0x3), zcstatus_dir_str[((sn->uc_zc_status>>3) & 0x3)]);
        vty_out(vty, "\t- ZCT: %u us\r\n\t- TIME_FREQ: %u Hz\r\n\t- TIME_DIFF: %u us\r\n", (sn->ui_zct * 10), (100000 / sn->uc_time_freq), (sn->ul_time_diff * 10));
        return CMD_SUCCESS;
    }
    vty_out(vty,"ZeroCross Request failed (0x%X)\r\n",x_pib_confirm.m_u8Status);
    return CMD_ERR_NOTHING_TODO;
 }

/**
 * \brief Firmware Upgrade Commands
 *
 */

/**
* \brief Firmware Upgrade Options
*
*/
DEFUN (prime_bmng_fw_upgrade_set_options,
      prime_bmng_fw_upgrade_set_options_cmd,
      "fw-upgrade options (unicast|multicast) arq (enabled|disabled) page_size <0-4> delay <0-65535> timer <0-65535>",
      "Firmware Upgrade\n"
      "Firmware Upgrade Options\n"
      "Firmware Upgrade Unicast\n"
      "Firmware Upgrade Multicast\n"
      "Firmware Upgrade ARQ\n"
      "Firmware Upgrade ARQ enabled\n"
      "Firmware Upgrade ARQ disabled\n"
      "Firmware Upgrade Page Size\n"
      "Firmware Upgrade Page Size value (0:auto 1:32 2:64 3:128 4:192 bytes)\n"
      "Firmware Upgrade Restart Delay\n"
      "Firmware Upgrade Restart Delay value (0...65535 sec)\n"
      "Firmware Upgrade Safety Timer\n"
      "Firmware Upgrade Safety Timer value (0...65535 sec)\n")
{
 /*********************************************
 *       Local Envars                         *
 **********************************************/
 struct TfwUpgradeOptions fu_options;
 /*********************************************
 *       Code                                 *
 **********************************************/
    VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

    if (strcmp(argv[0],"unicast") == 0){
       fu_options.mult_en = 0;
    }else{
       fu_options.mult_en = 1;
    }
    if (strcmp(argv[1],"disabled") == 0){
       fu_options.arq_en = 0;
    }else{
       fu_options.arq_en = 1;
    }
    fu_options.pagesize = (uint8_t) atoi(argv[2]);
    fu_options.delay = atoi(argv[3]);
    fu_options.timer = atoi(argv[4]);
    fw_upgrade_set_upg_options(&fu_options);

    return CMD_SUCCESS;
}

/**
* \brief Firmware Upgrade Set Match Rule
*        It allows the BN to select the devices to be upgraded in function of Model/Version
*
*/
DEFUN (prime_bmng_fw_upgrade_set_match_rule,
      prime_bmng_fw_upgrade_set_match_rule_cmd,
      "fw-upgrade match_rule (model|vendor|both|none)",
      "Firmware Upgrade\n"
      "Firmware Upgrade Match Rule\n"
      "Firmware Upgrade filter by Model\n"
      "Firmware Upgrade filter by Vendor\n"
      "Firmware Upgrade filter by Model and Vendor\n"
      "Firmware Upgrade without filter\n")
{
 /*********************************************
 *       Local Envars                         *
 **********************************************/
  struct TfwUpgradeOptions fu_options;
 /*********************************************
 *       Code                                 *
 **********************************************/
    VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

    if (strcmp(argv[0],"model") == 0){
      fu_options.filter_model = 1;
    }else if (strcmp(argv[0],"vendor") == 0){
      fu_options.filter_vendor = 1;
    }else if (strcmp(argv[0],"both") == 0){
      fu_options.filter_model = 1;
      fu_options.filter_vendor = 1;
    }else{
      fu_options.filter_model = 0;
      fu_options.filter_vendor = 0;
    }

    fw_upgrade_set_match_rule(&fu_options);
    return CMD_SUCCESS;
}

/**
* \brief Firmware Upgrade Show Match Rule
*
*/
DEFUN (prime_bmng_fw_upgrade_show_match_rule,
      prime_bmng_fw_upgrade_show_match_rule_cmd,
      "show fw-upgrade match_rule",
      PRIME_SHOW_STR
      "Firmware Upgrade\n"
      "Firmware Upgrade match rule\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TfwUpgradeOptions fu_options;
/*********************************************
*       Code                                 *
**********************************************/
    VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
    fw_upgrade_get_match_rule(&fu_options);
    vty_out(vty,"Firmware Upgrade Match Rule:\r\n"                                    \
                                             "\t- Model  :%s\r\n"                     \
                                             "\t- Vendor :%s\r\n",                    \
                                                 fu_options.filter_model?"Yes":"No",  \
                                                 fu_options.filter_vendor?"Yes":"No");
    return CMD_SUCCESS;
}


/**
* \brief Firmware Upgrade Set Firmware Image Info
*        Sets information about Vendor, Model and Version for Fw upgrade image
*
*/
DEFUN (prime_bmng_fw_upgrade_set_data_info,
      prime_bmng_fw_upgrade_set_data_info_cmd,
      "fw-upgrade image_info vendor VENDOR model MODEL version VERSION",
      "Firmware Upgrade\n"
      "Firmware Upgrade Image Info \n"
      "Firmware Upgrade Image Info Vendor\n"
      "Firmware Upgrade Image Info Vendor value\n"
      "Firmware Upgrade Image Info Model\n"
      "Firmware Upgrade Image Info Model value\n"
      "Firmware Upgrade Image Info Version\n"
      "Firmware Upgrade Image Info Version value\n")
{
 /*********************************************
 *       Local Envars                         *
 **********************************************/
 struct TfwUpgradeOptions fu_options;
 uint32_t len;
 /*********************************************
 *       Code                                 *
 **********************************************/
    VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

    len = strlen(argv[0]);
    if (len > 32){
       vty_out(vty,"Vendor size error\r\n");
       return CMD_ERR_NOTHING_TODO;
    }
    memcpy(&fu_options.vendor,argv[0],len);
    fu_options.vendor_len = len;

    len = strlen(argv[1]);
    if (len > 32){
       vty_out(vty,"Model size error\r\n");
       return CMD_ERR_NOTHING_TODO;
    }
    memcpy(&fu_options.model,argv[1],len);
    fu_options.model_len = len;

    len = strlen(argv[2]);
    if (len > 32){
       vty_out(vty,"Version size error\r\n");
       return CMD_ERR_NOTHING_TODO;
    }
    memcpy(&fu_options.version,argv[2],len);
    fu_options.version_len = len;

    fw_upgrade_set_fw_data_info(&fu_options);

    return CMD_SUCCESS;
}

/**
* \brief Firmware Upgrade Show Match Rule
*
*/
DEFUN (prime_bmng_fw_upgrade_show_data_info,
      prime_bmng_fw_upgrade_show_data_info_cmd,
      "show fw-upgrade data_info",
      PRIME_SHOW_STR
      "Firmware Upgrade\n"
      "Firmware Upgrade data info\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TfwUpgradeOptions fu_options;
/*********************************************
*       Code                                 *
**********************************************/
    VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
    fw_upgrade_get_fw_data_info(&fu_options);
    vty_out(vty,"Firmware Upgrade Data Info:\r\n"                   \
                                             "\t- Vendor  :%s\r\n"  \
                                             "\t- Model   :%s\r\n"  \
                                             "\t- Version :%s\r\n", \
                                                 fu_options.vendor, \
                                                 fu_options.model,  \
                                                 fu_options.version);
    return CMD_SUCCESS;
}

 /**
  * \brief Firmware Upgrade Clear Target List
  *
  */
DEFUN (prime_bmng_fw_upgrade_clear_target_list,
      prime_bmng_fw_upgrade_clear_target_list_cmd,
      "fw-upgrade clear_target_list",
      "Firmware Upgrade\n"
      "Clear Firmware Upgrade Target List\n")
{
 /*********************************************
 *       Local Envars                         *
 **********************************************/
 prime_sn * sn = NULL;
 mchp_list *entry, *tmp;
 struct TmacSetConfirm x_pib_confirm;
 /*********************************************
 *       Code                                 *
 **********************************************/
    VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
    // FW Upgrade Enable for each node
    prime_network_mutex_lock();
    list_for_each_safe(entry, tmp, &prime_network) {
       sn = list_entry(entry, prime_sn, list);
       sn->fwup_en = 0;
    }
    prime_network_mutex_unlock();
    bmng_fup_clear_target_list_request_sync(&x_pib_confirm);
    if (x_pib_confirm.m_u8Status != FUP_ACK_OK){
       vty_out(vty,"Error clearing target list\r\n");
       return CMD_ERR_NOTHING_TODO;
    }
    return CMD_SUCCESS;
}
/**
* \brief Firmware Upgrade Add Target
*
*/
DEFUN (prime_bmng_fw_upgrade_add_target,
       prime_bmng_fw_upgrade_add_target_cmd,
       "fw-upgrade add MAC",
       "Firmware Upgrade\n"
       "Add Target to Firmware Upgrade mechanism\n"
       "Service Node EUI48\r\n")
{
  /*********************************************
  *       Local Envars                         *
  **********************************************/
  uint8_t  mac[6];
  prime_sn * sn = NULL;
  /*********************************************
  *       Code                                 *
  **********************************************/
     VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
     if (str_to_eui48(argv[0],mac)){
       vty_out(vty,"Service Node EUI48 length is wrong\r\n");
       return CMD_ERR_NOTHING_TODO;
     }
     sn = prime_network_find_sn(&prime_network,mac);
     if (sn == NULL){
       vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
       return CMD_ERR_NOTHING_TODO;
     }
     sn->fwup_en = 1;
     return CMD_SUCCESS;
}

/**
* \brief Firmware Upgrade Add All Registered Targets
*
*/
DEFUN (prime_bmng_fw_upgrade_add_all,
       prime_bmng_fw_upgrade_add_all_cmd,
       "fw-upgrade add all",
       "Firmware Upgrade\n"
       "Add Target to Firmware Upgrade mechanism\n"
       "All Service Nodes\r\n")
{
  /*********************************************
  *       Local Envars                         *
  **********************************************/
  prime_sn * sn = NULL;
  mchp_list *entry, *tmp;
  /*********************************************
  *       Code                                 *
  **********************************************/
     VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

     // FW Upgrade Enable flag for each node
     prime_network_mutex_lock();
     list_for_each_safe(entry, tmp, &prime_network) {
        sn = list_entry(entry, prime_sn, list);
        sn->fwup_en = 1;
     }
     prime_network_mutex_unlock();
     return CMD_SUCCESS;
}

/**
* \brief Firmware Upgrade Abort Target update
*
*/
DEFUN (prime_bmng_fw_upgrade_abort_target,
       prime_bmng_fw_upgrade_abort_target_cmd,
       "fw-upgrade abort MAC",
       "Firmware Upgrade\n"
       "Abort Target Firmware Upgrade\n"
       "Service Node EUI48\r\n")
{
  /*********************************************
  *       Local Envars                         *
  **********************************************/
  uint8_t  mac[6];
  prime_sn * sn = NULL;
  struct TmacSetConfirm x_pib_confirm;
  /*********************************************
  *       Code                                 *
  **********************************************/
     VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
     if (str_to_eui48(argv[0],mac)){
       vty_out(vty,"Service Node EUI48 length is wrong\r\n");
       return CMD_ERR_NOTHING_TODO;
     }
     sn = prime_network_find_sn(&prime_network,mac);
     if (sn == NULL){
       vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
       return CMD_ERR_NOTHING_TODO;
     }
     bmng_fup_abort_fu_request_sync(mac, &x_pib_confirm);
     if (x_pib_confirm.m_u8Status == FUP_ACK_OK){
         vty_out(vty,"%s Firmware Upgrade aborted\r\n");
         return CMD_SUCCESS;
     }
     vty_out(vty,"Error aborting %s Firmware Upgrade (0x%X)\r\n",x_pib_confirm.m_u8Status);
     return CMD_ERR_NOTHING_TODO;
}

/**
* \brief Firmware Upgrade Abort All Target update
*
*/
DEFUN (prime_bmng_fw_upgrade_abort_all,
       prime_bmng_fw_upgrade_abort_all_cmd,
       "fw-upgrade abort all",
       "Firmware Upgrade\n"
       "Abort Target Firmware Upgrade\n"
       "All Service Nodes\r\n")
{
  /*********************************************
  *       Local Envars                         *
  **********************************************/
  prime_sn * sn = NULL;
  mchp_list *entry, *tmp;
  struct TmacGetConfirm x_pib_confirm;
  struct TmacSetConfirm s_pib_confirm;
  /*********************************************
  *       Code                                 *
  **********************************************/
     VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

     // Update FW Upgrade List
     prime_mngp_list_get_request_sync(PIB_FU_LIST,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST,&x_pib_confirm);
     if (x_pib_confirm.m_u8Status != FUP_ACK_OK){
         vty_out(vty,"Failed to get Firmware Upgrade Target List \r\n");
         return CMD_ERR_NOTHING_TODO;
     }

     // FW Upgrade abortion for each node
     prime_network_mutex_lock();
     list_for_each_safe(entry, tmp, &prime_network) {
        sn = list_entry(entry, prime_sn, list);
        if (sn->fwup_en){
           bmng_fup_abort_fu_request_sync(sn->regEntryID, &s_pib_confirm);
           if (s_pib_confirm.m_u8Status == FUP_ACK_OK){
              vty_out(vty,"%s Firmware Upgrade aborted\r\n", eui48_to_str(sn->regEntryID,NULL));
           }else{
              vty_out(vty,"Error aborting %s Firmware Upgrade (0x%X)\r\n", eui48_to_str(sn->regEntryID,NULL), s_pib_confirm.m_u8Status);
           }
        }
     }
     prime_network_mutex_unlock();
     return CMD_SUCCESS;
}

/**
* \brief Firmware Upgrade Show target list
*
*/
DEFUN (prime_bmng_fw_upgrade_show_target_list,
       prime_bmng_fw_upgrade_show_target_list_cmd,
       "show fw-upgrade target_list",
       PRIME_SHOW_STR
       "Firmware Upgrade\n"
       "Target List\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
prime_sn * sn = NULL;
mchp_list *entry, *tmp;
/*********************************************
*       Code                                 *
**********************************************/
     VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
     prime_mngp_list_get_request_sync(PIB_FU_LIST,PRIME_SYNC_TIMEOUT_GET_LIST_REQUEST,&x_pib_confirm);
     if (x_pib_confirm.m_u8Status != FUP_ACK_OK){
       vty_out(vty,"Error getting Firmware Upgrade Target List (0x%X)\r\n",x_pib_confirm.m_u8Status);
       return CMD_ERR_NOTHING_TODO;
     }
     // FW Upgrade Admin Status
     vty_out(vty,"EUI48               Vendor            Model           Version    \r\n");
     vty_out(vty,"-------------- ---------------- ---------------- ----------------\r\n");
     prime_network_mutex_lock();
     list_for_each_safe(entry, tmp, &prime_network) {
        sn = list_entry(entry, prime_sn, list);
        if (sn->fwup_en){
          vty_out(vty,"0x%s %16s %16s %16s\r\n", eui48_to_str(sn->regEntryID,NULL), \
                                                 sn->fu_vendor,                     \
                                                 sn->fu_model,                      \
                                                 sn->fu_version);
        }
     }
     prime_network_mutex_unlock();
     return CMD_SUCCESS;
}

/**
* \brief Firmware Upgrade Show target list
*
*/
DEFUN (prime_bmng_fw_upgrade_show_options,
       prime_bmng_fw_upgrade_show_options_cmd,
       "show fw-upgrade options",
       PRIME_SHOW_STR
       "Firmware Upgrade\n"
       "Target List\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TfwUpgradeOptions fu_options;
/*********************************************
*       Code                                 *
**********************************************/
     VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
     fw_upgrade_get_upg_options(&fu_options);
     vty_out(vty,"Firmware Upgrade Options\r\n\t- Multicast     :%s\r\n"          \
                                              "\t- ARQ           :%s\r\n"         \
                                              "\t- Pagesize      :%d bytes\r\n"   \
                                              "\t- Delay Restart :%d s\r\n"       \
                                              "\t- Safety Timer  :%d s\r\n",      \
                                                  fu_options.mult_en?"Yes":"No",  \
                                                  fu_options.arq_en?"Yes":"No",   \
                                                  fu_options.pagesize,            \
                                                  fu_options.delay,               \
                                                  fu_options.timer);
     return CMD_SUCCESS;
}

/**
* \brief Firmware Upgrade Set Binary Path
*
*/
DEFUN (prime_bmng_fw_upgrade_set_binary_path,
       prime_bmng_fw_upgrade_set_binary_path_cmd,
       "fw-upgrade binarypath FILE",
       "Firmware Upgrade\n"
       "FW Upgrade Binary File\n"
       "FW Upgrade Binary File path (max. 64 characters)\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
uint16_t len;
struct TfwUpgradeOptions fu_options;
FILE *filepath;
/*********************************************
*       Code                                 *
**********************************************/
     VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
     len = strlen(argv[0]);
     if (len > 64){
        vty_out(vty,"Binary Path Length error\r\n");
        return CMD_ERR_NOTHING_TODO;
     }

     filepath = fopen (argv[0], "r");
     if (filepath == NULL){
       vty_out(vty,"Firmware Upgrade Binary File %s doesn't exist\r\n");
       return CMD_ERR_NOTHING_TODO;
     }
     fclose(filepath);
     memset(&fu_options.binarypath,0,64);
     memcpy(&fu_options.binarypath,argv[0],len);
     fw_upgrade_set_binary_path(&fu_options);
     return CMD_SUCCESS;
}

/**
* \brief Firmware Upgrade Show Binary Path
*
*/
DEFUN (prime_bmng_fw_upgrade_show_binary_path,
       prime_bmng_fw_upgrade_show_binary_path_cmd,
       "show fw-upgrade binarypath",
       PRIME_SHOW_STR
       "Firmware Upgrade\n"
       "FW Upgrade Binary File Path\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TfwUpgradeOptions fu_options;
/*********************************************
*       Code                                 *
**********************************************/
     VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
     fw_upgrade_get_binary_path(&fu_options);
     if (fu_options.binarypath != NULL){
        vty_out(vty,"Firmware Upgrade Binary Path :%s\r\n",&fu_options.binarypath);
     }else{
        vty_out(vty,"Firmware Upgrade Binary Path not set\r\n");
     }
     return CMD_SUCCESS;
}

/**
* \brief Firmware Upgrade Get FW Version
*
*/
DEFUN (prime_bmng_fw_upgrade_show_version_target,
       prime_bmng_fw_upgrade_show_version_target_cmd,
       "show fw-upgrade version MAC",
       PRIME_SHOW_STR
       "Firmware Upgrade\n"
       "FW Upgrade Version\n"
       "Service Node EUI48\r\n")
{
  /*********************************************
  *       Local Envars                         *
  **********************************************/
  uint8_t  mac[6];
  prime_sn * sn = NULL;
  struct TmacGetConfirm x_pib_confirm;
  /*********************************************
  *       Code                                 *
  **********************************************/
     VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
     if (str_to_eui48(argv[0],mac)){
       vty_out(vty,"Service Node EUI48 length is wrong\r\n");
       return CMD_ERR_NOTHING_TODO;
     }
     sn = prime_network_find_sn(&prime_network,mac);
     if (sn == NULL){
       vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
       return CMD_ERR_NOTHING_TODO;
     }
     bmng_fup_get_version_request_sync(mac, &x_pib_confirm);
     if (x_pib_confirm.m_u8Status != FUP_ACK_OK){
        vty_out(vty,"Error getting %s Firmware Version (0x%X)\r\n",argv[0], x_pib_confirm.m_u8Status);
        return CMD_ERR_NOTHING_TODO;
     }
     vty_out(vty,"EUI48               Vendor            Model           Version    \r\n");
     vty_out(vty,"-------------- ---------------- ---------------- ----------------\r\n");
     vty_out(vty,"0x%s %16s %16s %16s\r\n", eui48_to_str(sn->regEntryID,NULL), \
                                            sn->fu_vendor,                     \
                                            sn->fu_model,                      \
                                            sn->fu_version);
      return CMD_SUCCESS;
  }

/**
* \brief Firmware Upgrade Get FW Upgrade State Remote MAC
*
*/
DEFUN (prime_bmng_fw_upgrade_show_state_target,
         prime_bmng_fw_upgrade_show_state_target_cmd,
         "show fw-upgrade state MAC",
         PRIME_SHOW_STR
         "Firmware Upgrade\n"
         "FW Upgrade State\n"
         "Service Node EUI48\r\n")
{
  /*********************************************
  *       Local Envars                         *
  **********************************************/
  uint8_t  mac[6];
  prime_sn * sn = NULL;
  struct TmacGetConfirm x_pib_confirm;
  /*********************************************
  *       Code                                 *
  **********************************************/
     VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
     if (str_to_eui48(argv[0],mac)){
       vty_out(vty,"Service Node EUI48 length is wrong\r\n");
       return CMD_ERR_NOTHING_TODO;
     }
     sn = prime_network_find_sn(&prime_network,mac);
     if (sn == NULL){
       vty_out(vty, "Service Node with EUI48 '%s' doesn't exist\r\n", argv[0]);
       return CMD_ERR_NOTHING_TODO;
     }
     bmng_fup_get_state_request_sync(mac, &x_pib_confirm);
     if (x_pib_confirm.m_u8Status != FUP_ACK_OK){
        vty_out(vty,"Error getting %s Firmware State (0x%X)\r\n",argv[0],x_pib_confirm.m_u8Status);
        return CMD_ERR_NOTHING_TODO;
     }
     vty_out(vty,"EUI48               STATE       PAGES\r\n");
     vty_out(vty,"-------------- ---------------- -----\r\n");
     vty_out(vty,"0x%s %16s %05d\r\n", eui48_to_str(sn->regEntryID,NULL),         \
                                            fup_state_to_str(sn->fu_state,NULL),  \
                                            sn->fu_pages);
      return CMD_SUCCESS;
}

  /**
  * \brief Firmware Upgrade Signature Algorithm
  *
  */
  DEFUN (prime_bmng_fw_upgrade_set_signature_options,
         prime_bmng_fw_upgrade_set_signature_options_cmd,
         "fw-upgrade signature_algorithm <0-15> size <0-255>",
         "Firmware Upgrade\r\n"
         "FW Upgrade Signature Algorithm\r\n"
         "FW Upgrade Signature Algorithm (0:none 1:rsa-sha 2:ecdsa-sha)\r\n"
         "FW Upgrade Signature Size\r\n"
         "FW Upgrade Signature Size value\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TfwUpgradeOptions fu_options;
/*********************************************
*       Code                                 *
**********************************************/
     VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

     fu_options.sign_algo = (uint8_t) atoi(argv[0]);
     if (fu_options.sign_algo >= 3){
         vty_out(vty,"Reserved value for future use\r\n");
         return CMD_ERR_NOTHING_TODO;
     }
     fu_options.sign_size = (uint8_t) atoi(argv[1]);;
     fw_upgrade_set_signature_options(&fu_options);
     return CMD_SUCCESS;
}

/**
* \brief Firmware Upgrade Show Signtature Options
*
*/
DEFUN (prime_bmng_fw_upgrade_show_signature_options,
       prime_bmng_fw_upgrade_show_signature_options_cmd,
       "show fw-upgrade signature_options",
       PRIME_SHOW_STR
       "Firmware Upgrade\n"
       "FW Upgrade Signature Options\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TfwUpgradeOptions fu_options;
/*********************************************
*       Code                                 *
**********************************************/
     VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
     fw_upgrade_get_signature_options(&fu_options);
     vty_out(vty,"Firmware Upgrade Signature %d with length %d\r\n",fu_options.sign_algo,fu_options.sign_size);
     return CMD_SUCCESS;
}

/**
* \brief Firmware Upgrade Download
*
*/
DEFUN (prime_bmng_fw_upgrade_download,
       prime_bmng_fw_upgrade_download_cmd,
       "fw-upgrade download",
       "Firmware Upgrade\n"
       "FW Upgrade Download\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
/*********************************************
*       Code                                 *
**********************************************/
     VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
     /* It should be run on background */
     /* Include commands to show download status */
     fw_upgrade_download();
     return CMD_SUCCESS;
}

/**
* \brief Firmware Upgrade Start Process
*
*/
DEFUN (prime_bmng_fw_upgrade_start,
       prime_bmng_fw_upgrade_start_cmd,
       "fw-upgrade start",
       "Firmware Upgrade\n"
       "FW Upgrade Start\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
/*********************************************
*       Code                                 *
**********************************************/
     VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
     /* Include commands to show upgrade status */
     fw_upgrade_start(1);
     return CMD_SUCCESS;
}

/**
* \brief Firmware Upgrade Status Process
*
*/
DEFUN (prime_bmng_fw_upgrade_status,
       prime_bmng_fw_upgrade_status_cmd,
       "fw-upgrade status",
       "Firmware Upgrade\n"
       "FW Upgrade Status\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
struct TmacGetConfirm x_pib_confirm;
prime_sn *sn;
mchp_list *entry, *tmp;
uint16_t us_value;
/*********************************************
*       Code                                 *
**********************************************/
  if (PRIME_MODE_BASE){
    if (fw_upgrade_status() == FW_UPGRADE_RUNNING){
      // FW Upgrade abortion for each node
      vty_out(vty,"EUI48               STATE       PAGES\r\n");
      vty_out(vty,"-------------- ---------------- -----\r\n");
      prime_network_mutex_lock();
      list_for_each_safe(entry, tmp, &prime_network) {
         sn = list_entry(entry, prime_sn, list);
         if (sn->fwup_en){
           if (sn->fu_state != FUP_STATE_EXCEPTION){
             vty_out(vty,"0x%s %16s %05d\r\n", eui48_to_str(sn->regEntryID,NULL),fup_state_to_str(sn->fu_state,NULL),sn->fu_pages);
           }else{
             vty_out(vty,"0x%s %16s %s\r\n", eui48_to_str(sn->regEntryID,NULL), fup_state_to_str(sn->fu_state,NULL), fup_state_exception_to_str(sn->fu_pages,NULL));
           }
         }
      }
      prime_network_mutex_unlock();
    }else{
      vty_out(vty,"FW Upgrade Status: %s\r\n", (fw_upgrade_status() == FW_UPGRADE_IDLE) ? "Idle" : "Finished");
    }
  }else{
    prime_cl_null_mlme_get_request_sync(PIB_FU_APP_FWDL_RUNNING,PRIME_SYNC_TIMEOUT_GET_REQUEST, &x_pib_confirm);
    if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
        // 2 Byte should be the result
        memcpy(&us_value,&x_pib_confirm.m_au8AttributeValue[0],2);
        vty_out(vty,"\t* Status : %s\r\n",(us_value == 1)? "Fimware Download in progress" : "No Firmware Download");
        if (us_value == 1){
          /* Download in Progress */
          prime_cl_null_mlme_get_request_sync(PIB_FU_APP_FWDL_RX_PKT_COUNT,PRIME_SYNC_TIMEOUT_GET_REQUEST,&x_pib_confirm);
          if ((x_pib_confirm.m_u8Status == MLME_RESULT_DONE) && (x_pib_confirm.m_u8AttributeLength == 2)){
              // 2 Byte should be the result
              memcpy(&us_value,&x_pib_confirm.m_au8AttributeValue[0],2);
              vty_out(vty,"\t* #Packets : %hu\r\n",us_value);
          }else{
              vty_out(vty,"Impossible to get PRIME PIB AppFwdlRxPktCount\r\n");
              return CMD_ERR_NOTHING_TODO;
          }
        }
    }else{
        vty_out(vty,"Impossible to get PRIME PIB AppFwdlRxPktCount\r\n");
        return CMD_ERR_NOTHING_TODO;
    }
  }
  return CMD_SUCCESS;
}

/**
* \brief Add MAC Address to the Network
*
*/
DEFUN (prime_network_add_target,
       prime_network_add_target_cmd,
       "network add MAC key (none|default|DUK)",
       "PRIME Network\n"
       "Add Target to PRIME Network\n"
       "Service Node EUI48\r\n"
       "Security Key\r\n"
       "No security\r\n"
       "Security Key default DUK\r\n"
       "Security Key DUK value\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
  uint8_t  mac[6];
  uint8_t  duk[16];
  uint8_t  buffer[22];
  uint8_t  i;
  uint8_t  security;
  prime_sn * sn = NULL;
  struct TmacSetConfirm x_pib_confirm;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   if (strcmp("none",argv[1]) == 0){
       // Check how to calculate Default DUK or each Service Node
       for (i=0;i<16;i++)
          duk[i] = 0x00;
       security = 0;
       vty_out(vty,"No security will be applied\r\n");
   }else if (strcmp("default",argv[1]) == 0){
       // Check how to calculate Default DUK or each Service Node
       memcpy(&duk[0],mac,6);
       memcpy(&duk[6],mac,6);
       for (i=0;i<4;i++)
          duk[12+i] = 0xFF;
       security = 1;
       vty_out(vty,"Default DUK will be applied\r\n");
   }else{
       if (str_to_duk(argv[1],duk)){
          vty_out(vty,"Service Node DUK length is wrong\r\n");
          return CMD_ERR_NOTHING_TODO;
       }
       security = 1;
   }

   if (security & (g_st_config.sec_profile == 0)){
      vty_out(vty,"No Security Profile configured on Base Node\r\n");
      return CMD_ERR_NOTHING_TODO;
   }

   sn = prime_network_find_sn(&prime_network,mac);
   if (sn != NULL){
       vty_out(vty, "Service Node with EUI48 '%s' found\r\n", argv[0]);
       sn->security_profile = g_st_config.sec_profile;
       if (security){
          memcpy(sn->duk,duk,16);
          memcpy(&buffer[0],mac,6);
          memcpy(&buffer[6],duk,16);
          prime_cl_null_mlme_set_request_sync(PIB_MAC_SEC_DUK_BN,&buffer[0],22,PRIME_SYNC_TIMEOUT_SET_REQUEST,&x_pib_confirm);
          if (x_pib_confirm.m_u8Status != MLME_RESULT_DONE){
             vty_out(vty,"Imposible to set Secutity DUK to MAC %s\r\n",argv[0]);
             return CMD_ERR_NOTHING_TODO;
          }
       }
   }else{
     if (security){
        sn = prime_network_add_sn(&prime_network, mac, ADMIN_ENABLE, g_st_config.sec_profile, duk);
     }else{
        sn = prime_network_add_sn(&prime_network, mac, ADMIN_ENABLE, g_st_config.sec_profile, NULL);
     }
     if (sn == NULL){
       vty_out(vty,"Impossible to add Service Node %s\r\n",argv[0]);
       return CMD_ERR_NOTHING_TODO;
     }
   }
   vty_out(vty,"Updated Service Node %s\r\n",argv[0]);
   return CMD_SUCCESS;
}

/**
* \brief Delete MAC Address on the Network
*
*/
DEFUN (prime_network_del_target,
       prime_network_del_target_cmd,
       "network del MAC",
       "PRIME Network\n"
       "Delete Target on PRIME Network\n"
       "Service Node EUI48\r\n")
{
/*********************************************
*       Local Envars                         *
**********************************************/
  uint8_t  mac[6];
  prime_sn * sn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
     vty_out(vty,"Service Node EUI48 length is wrong\r\n");
     return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn != NULL){
     vty_out(vty, "Service Node with EUI48 '%s' found\r\n", argv[0]);
     prime_network_del_sn(sn);
   }else{
     vty_out(vty,"Impossible to del Service Node %s - Not found\r\n",argv[0]);
     return CMD_ERR_NOTHING_TODO;
   }
   vty_out(vty,"Deleted Service Node %s\r\n",argv[0]);
   return CMD_SUCCESS;
}

extern int prime_network_sn;

/**
* \brief Show Registered Devices on PRIME Network
*
*/
DEFUN (prime_network_show_registered_devices,
       prime_network_show_registered_devices_cmd,
       "show network registered_devices",
       PRIME_SHOW_STR
       "PRIME Network\n"
       "Registered Devices\n")
{
/*********************************************************
*       Local Vars                                       *
*********************************************************/
mchp_list *entry, *tmp;
prime_sn * p_prime_sn;
int level;
/*********************************************************
*       Code                                             *
*********************************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (prime_network_sn == 0){
	   vty_out(vty,"No PRIME SN present\n");
  }else{
     vty_out(vty,"EUI48           LNID    State   LSID  SID Level TCap SwCap\n");
     vty_out(vty,"-------------- ------- -------  ----  --- ----- ---- -----\n");
     prime_network_mutex_lock();
     for (level=0; level<=prime_network_get_max_level();level++){
       list_for_each_safe(entry, tmp, &prime_network) {
          p_prime_sn = list_entry(entry, prime_sn, list);
          if ((p_prime_sn->registered) && (p_prime_sn->regEntryLevel == level)){
             vty_out(vty,"0x%s  %05d  %08s  %03d  %03d  %03d  0x%02X 0x%02X\r\n",       \
                                              eui48_to_str(p_prime_sn->regEntryID,NULL), \
                                              p_prime_sn->regEntryLNID,  \
                                              mac_state_str[p_prime_sn->regEntryState], \
                                              p_prime_sn->regEntryLSID,  \
                                              p_prime_sn->regEntrySID,   \
                                              p_prime_sn->regEntryLevel, \
                                              p_prime_sn->regEntryTCap,  \
                                              p_prime_sn->regEntrySwCap);
          }
       }
     }
     prime_network_mutex_unlock();
  }
  return CMD_SUCCESS;
}

/**
* \brief Show Network Tree
*
*/
DEFUN (prime_network_show_topology,
       prime_network_show_topology_cmd,
       "show network topology",
       PRIME_SHOW_STR
       "PRIME Network\n"
       "Network Topology\n")
{
/*********************************************************
*       Local Vars                                       *
*********************************************************/
mchp_list *entry, *tmp;
prime_sn *p_prime_sn, *p_prime_sw;
int level;
FILE *fp_graphviz;
bool en_graphviz = false;
char sn_mac[18], sw_mac[18];
/*********************************************************
*       Code                                             *
*********************************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if (prime_network_sn == 0){
	   vty_out(vty,"No PRIME SN present\n");
  }else{
     vty_out(vty,"State    EUI48         LNID   LSID  SID Level TCap SwCap\n");
     vty_out(vty,"-------- ------------ ------- ----  --- ----- ---- -----\n");
     vty_out(vty,"  BASE   %s  00000  0000  000   0    --   --\r\n", eui48_to_str((const unsigned char *)&g_st_config.eui48,NULL));
     /* Create Graphviz File */
     fp_graphviz = fopen(TOPOLOGY_FILE,"w");
     if (fp_graphviz != NULL){
        en_graphviz=true;
     }
     if (en_graphviz){
        fprintf(fp_graphviz,"digraph Topology {\r\nranksep=3;\r\nratio=auto;\r\n");
        fprintf(fp_graphviz,"%s [shape=diamond]\r\n",eui48_to_str((const unsigned char *)&g_st_config.eui48,NULL));
     }
     prime_network_mutex_lock();
     for (level=0; level<=prime_network_get_max_level();level++){
       /* First - Detect the switch nodes */
       list_for_each_safe(entry, tmp, &prime_network) {
         p_prime_sn = list_entry(entry, prime_sn, list);
         if ((p_prime_sn->registered) && (p_prime_sn->regEntryLevel == level) && p_prime_sn->regEntryState == REGISTER_STATE_SWITCH){
            vty_out(vty," SWITCH  %s  %05d   %03d  %03d  %03d  0x%02X 0x%02X\r\n",       \
                                                                                  eui48_to_str(p_prime_sn->regEntryID,NULL), \
                                                                                  p_prime_sn->regEntryLNID,  \
                                                                                  p_prime_sn->regEntryLSID,  \
                                                                                  p_prime_sn->regEntrySID,   \
                                                                                  p_prime_sn->regEntryLevel, \
                                                                                  p_prime_sn->regEntryTCap,  \
                                                                                  p_prime_sn->regEntrySwCap);
            if (en_graphviz){
              memset(sn_mac,'\0',18);
              memset(sw_mac,'\0',18);
              /* Look for the Parent to do association */
              if (p_prime_sn->regEntrySID == 0){
                // My father is the Base Node
                eui48_to_str((const unsigned char *)&g_st_config.eui48,sw_mac);
              }else{
                p_prime_sw = prime_network_find_sn_lsid(&prime_network,p_prime_sn->regEntrySID);
                eui48_to_str(p_prime_sw->regEntryID,sw_mac);
              }
              eui48_to_str(p_prime_sn->regEntryID,sn_mac);
              fprintf(fp_graphviz,"%s [shape=box]\r\n",sn_mac);
              fprintf(fp_graphviz,"%s -> %s\r\n", sw_mac, sn_mac);
            }
         }
       }
       /* Second - Detect the service nodes */
       list_for_each_safe(entry, tmp, &prime_network) {
          p_prime_sn = list_entry(entry, prime_sn, list);
          if ((p_prime_sn->registered) && (p_prime_sn->regEntryLevel == level) && p_prime_sn->regEntryState == REGISTER_STATE_TERMINAL ){
             vty_out(vty,"TERMINAL %s  %05d   %03d  %03d  %03d  0x%02X 0x%02X\r\n",       \
                                                                                  eui48_to_str(p_prime_sn->regEntryID,NULL), \
                                                                                  p_prime_sn->regEntryLNID,  \
                                                                                  p_prime_sn->regEntryLSID,  \
                                                                                  p_prime_sn->regEntrySID,   \
                                                                                  p_prime_sn->regEntryLevel, \
                                                                                  p_prime_sn->regEntryTCap,  \
                                                                                  p_prime_sn->regEntrySwCap);
             if (en_graphviz){
               memset(sn_mac,'\0',18);
               memset(sw_mac,'\0',18);
               /* Look for the Parent to do association */
               if (p_prime_sn->regEntrySID == 0){
                 // My father is the Base Node
                 eui48_to_str((const unsigned char * )&g_st_config.eui48,sw_mac);
               }else{
                 p_prime_sw = prime_network_find_sn_lsid(&prime_network,p_prime_sn->regEntrySID);
                 eui48_to_str(p_prime_sw->regEntryID,sw_mac);
               }
               eui48_to_str(p_prime_sn->regEntryID,sn_mac);
               fprintf(fp_graphviz,"%s -> %s\r\n", sw_mac, sn_mac);
             }
          }
       }
     }
     prime_network_mutex_unlock();
     if (en_graphviz){
        fprintf(fp_graphviz,"}\r\n");
        fclose(fp_graphviz);
        vty_out(vty,"Generated topology file. Go to http://www.webgraphviz.com/ for visualization.\r\n");
     }
  }
  return CMD_SUCCESS;
}

/**
* \brief Show Registered Devices on PRIME Network
*
*/
DEFUN (prime_network_show_available_switches,
       prime_network_show_available_switches_cmd,
       "show network available_switches",
       PRIME_SHOW_STR
       "PRIME Network\n"
       "Available Switches\n")
{
/*********************************************************
*       Local Vars                                       *
*********************************************************/
mchp_list *entry, *tmp;
prime_sn * p_prime_sn;
/*********************************************************
*       Code                                             *
*********************************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  if ((prime_network_sn == 0) && !(prime_network_get_max_level() > 1)){
	   vty_out(vty, "No PRIME SN Switches present\n");
  }else{
     vty_out(vty,"EUI48          Level  LSID  RxLvl RxSNR\n");
     vty_out(vty,"-------------- ----- ------ ----- -----\n");
     prime_network_mutex_lock();
     list_for_each_safe(entry, tmp, &prime_network) {
        p_prime_sn = list_entry(entry, prime_sn, list);
        if (p_prime_sn->regEntryState == REGISTER_STATE_SWITCH){
           vty_out(vty,"0x%s  %03d  %06d  %03d  %03d\r\n",  eui48_to_str(p_prime_sn->regEntryID,NULL), \
                                                            p_prime_sn->regEntryLevel,  \
                                                            p_prime_sn->regEntryLSID,  \
                                                            p_prime_sn->regEntryRxLvl,   \
                                                            p_prime_sn->regEntryRxSNR);
        }
     }
     prime_network_mutex_unlock();
  }
  return CMD_SUCCESS;
}

/**
* \brief Show Nework Level
*
*/
DEFUN (prime_network_show_level_registered_devices,
       prime_network_show_level_registered_devices_cmd,
       "show network level LEVEL registered_devices",
       PRIME_SHOW_STR
       "PRIME Network\n"
       "PRIME Network Level\n"
       "PRIME Network Level value\n"
       "Target List\n")
{
/*********************************************************
*       Local Vars                                       *
*********************************************************/
mchp_list *entry, *tmp;
prime_sn * p_prime_sn;
uint8_t level;
/*********************************************************
*       Code                                             *
*********************************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
  level = (uint8_t)atoi(argv[0]);

  if ((prime_network_sn == 0) || (level > prime_network_get_max_level())){
     vty_out(vty, "No PRIME SN present at level %d\n",level);
  }else{
     vty_out(vty,"EUI48           LNID    State   LSID  SID Level TCap SwCap\n");
     vty_out(vty,"-------------- ------- -------  ----  --- ----- ---- -----\n");
     prime_network_mutex_lock();
     list_for_each_safe(entry, tmp, &prime_network) {
        p_prime_sn = list_entry(entry, prime_sn, list);
        if ((p_prime_sn->registered) && (p_prime_sn->regEntryLevel == level)){
           vty_out(vty,"0x%s  %05d  %08s  %03d  %03d  %03d  0x%02X 0x%02X\r\n",       \
                                                                                eui48_to_str(p_prime_sn->regEntryID,NULL), \
                                                                                p_prime_sn->regEntryLNID,  \
                                                                                (p_prime_sn->regEntryState == 1) ? "Terminal":" Switch ", \
                                                                                p_prime_sn->regEntryLSID,  \
                                                                                p_prime_sn->regEntrySID,   \
                                                                                p_prime_sn->regEntryLevel, \
                                                                                p_prime_sn->regEntryTCap,  \
                                                                                p_prime_sn->regEntrySwCap);
        }
     }
     prime_network_mutex_unlock();
  }
  return CMD_SUCCESS;
}

DEFUN (prime_network_show_cl432_connections,
       prime_network_show_cl432_connections_cmd,
      "show network cl432_connections",
       PRIME_SHOW_STR
       "PRIME Network\n"
       "4-32 Connections\n")
{
/*********************************************************
*       Local Vars                                       *
*********************************************************/
mchp_list *entry, *tmp;
prime_sn * p_prime_sn;
/*********************************************************
*       Code                                             *
*********************************************************/
  VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);

  if (prime_network_sn == 0){
     vty_out(vty, "No PRIME SN present\n");
  }else{
     vty_out(vty,"  IEC 61334-4-32 ACTIVE CONNECTIONS:\r\n");
     vty_out(vty,"--------------------------------------\r\n");
     vty_out(vty,"EUI48           ADDRESS  SERIAL/NUMBER\n");
     vty_out(vty,"-------------- --------- -------------\n");
     prime_network_mutex_lock();
     list_for_each_safe(entry, tmp, &prime_network) {
        p_prime_sn = list_entry(entry, prime_sn, list);
        if (p_prime_sn->cl432Conn.connState == CL432_CONN_STATE_OPEN){
           vty_out(vty,"0x%s  0x%04X   %13s\r\n",                          \
                                eui48_to_str(p_prime_sn->regEntryID,NULL), \
                                p_prime_sn->cl432Conn.connAddress,         \
                                p_prime_sn->cl432Conn.connSerialNumber);
        }
     }
     prime_network_mutex_unlock();
  }
  return CMD_SUCCESS;
}

/**
* \brief Show MAC Connections for a Service Node
*
*/
DEFUN (prime_show_sn_mac_connections,
       prime_show_sn_mac_connections_cmd,
       "show target MAC mac_connections",
       PRIME_SHOW_STR
       "Target SN\n"
       "Service Node EUI48\n"
       "MAC Connections\n")
{
/*********************************************************
*       Local Vars                                       *
*********************************************************/
uint8_t  mac[6];
mchp_list *entry, *tmp;
prime_sn * sn = NULL;
mac_conn * conn = NULL;
/*********************************************
*       Code                                 *
**********************************************/
   VTY_CHECK_PRIME_MODE(PRIME_MODE_BASE);
   if (str_to_eui48(argv[0],mac)){
      vty_out(vty,"Service Node EUI48 length is wrong\r\n");
      return CMD_ERR_NOTHING_TODO;
   }
   sn = prime_network_find_sn(&prime_network,mac);
   if (sn != NULL){
      if (sn->macConns == 0){
	       vty_out(vty, "No PRIME MAC Connections present\n");
         return CMD_ERR_NOTHING_TODO;
      }else{
         vty_out(vty,"LCID ConnType\n");
         vty_out(vty,"---- --------\n");
         prime_network_mutex_lock();
         list_for_each_safe(entry, tmp, &sn->macConnList) {
            conn = list_entry(entry, mac_conn, list);
            vty_out(vty,"%04d  %d(%s)\r\n", conn->connEntryLCID, conn->connType,mac_connection_type_str[conn->connType]);
         }
         prime_network_mutex_unlock();
      }
   }
   return CMD_SUCCESS;
}

/*
    @brief Function to initialize the configuration structure(s) of this daemon.
*/
void init_prime_conf(void)
{
  /* Default Values for structures */

  /* Sniffer Options */
  PRIMEsniffer_log_options.enabled = PRIMESNIFFER_LOG_OPTIONS_ENABLED_DEFAULT;
  PRIMEsniffer_log_options.logfile_en = PRIMESNIFFER_LOG_OPTIONS_LOGFILE_EN_DEFAULT;
  PRIMEsniffer_log_options.tcp_en = PRIMESNIFFER_LOG_OPTIONS_TCP_EN;
  PRIMEsniffer_log_options.tcp_port = PRIMESNIFFER_LOG_OPTIONS_TCP_PORT;
}

/*
   Configuration write function. Used to write conf file or show running-config.
   It gives the list of commands showing the configuration.
*/
int config_write_prime(struct vty *vty)
{
/*********************************************************
*       Variables Locales Definidas                      *
*********************************************************/
char str[100];
/*********************************************************
*       Code                                           *
*********************************************************/
	vty_out(vty, "!PRIME configuration file.\n");

	if (vty->type == VTY_FILE){
		//BN_PRIME_LOG(LOG_VERBOSE, "write_config: Hidden Configuration\n");
  }
  // Save Configuration
  sprintf(str,"config log %s\n",(prime_get_log() == TRUE)? "enabled":"disabled");
  vty_out(vty,str);
  sprintf(str,"config sniffer_log logfile %s\n",(PRIMEsniffer_log_options.logfile_en == 1) ? "enabled":"disabled");
  vty_out(vty,str);
  sprintf(str,"config sniffer_log tcp %s\n",(PRIMEsniffer_log_options.tcp_en == 1) ? "enabled":"disabled");
  vty_out(vty,str);
  sprintf(str,"config sniffer_log tcp port %d\n",PRIMEsniffer_log_options.tcp_port);
  vty_out(vty,str);
  sprintf(str,"config sniffer_log %s\n",(PRIMEsniffer_log_options.enabled == 1) ? "enabled":"disabled");
  vty_out(vty,str);

	return 0;
}

int prime_vty_init(void)
{

	struct config * prime;

  cmd_install_node (&prime_node, NULL);
  cmd_install_element (CONFIG_NODE, &config_prime_cmd);
  // Common Commands on Specific Nodes
  cmd_install_element (PRIME_NODE, &config_prime_exit_cmd);
  cmd_install_element (PRIME_NODE, &config_prime_end_cmd);
  // Specific Commands
  cmd_install_element (PRIME_NODE, &prime_config_loglevel_cmd);
  cmd_install_element (PRIME_NODE, &prime_config_log_enable_cmd);
  cmd_install_element (PRIME_NODE, &prime_show_info_cmd);
  cmd_install_element (PRIME_NODE, &prime_show_info_mac_cmd);
  cmd_install_element (PRIME_NODE, &prime_show_config_cmd);
  cmd_install_element (PRIME_NODE, &prime_show_config_mac_cmd);
  cmd_install_element (PRIME_NODE, &prime_show_status_cmd);
  cmd_install_element (PRIME_NODE, &prime_show_status_mac_cmd);
  cmd_install_element (PRIME_NODE, &prime_show_security_profile_cmd);
  cmd_install_element (PRIME_NODE, &prime_config_modemport_serial_cmd);
  cmd_install_element (PRIME_NODE, &prime_config_modemport_tcp_cmd);
  cmd_install_element (PRIME_NODE, &prime_config_mode_cmd);
  cmd_install_element (PRIME_NODE, &prime_config_tx_channel_cmd);
  /* Sniffer Configuration */
  cmd_install_element (PRIME_NODE, &prime_show_sniffer_log_config_cmd);
  cmd_install_element (PRIME_NODE, &prime_config_sniffer_log_enable_cmd);
  cmd_install_element (PRIME_NODE, &prime_config_sniffer_log_logfile_enable_cmd);
  cmd_install_element (PRIME_NODE, &prime_config_sniffer_log_tcp_enable_cmd);
  cmd_install_element (PRIME_NODE, &prime_config_sniffer_log_tcp_port_cmd);
  /* DLMS over TCP Server */
  cmd_install_element (PRIME_NODE, &prime_show_dlmsotcp_config_cmd);
  cmd_install_element (PRIME_NODE, &prime_config_dlmsotcp_enable_cmd);
  cmd_install_element (PRIME_NODE, &prime_config_dlmsotcp_port_cmd);

  // PHY PIB Attributes
  // PHY Statistical Attributes
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_phyStatsCRCIncorrectCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_phyStatsCRCIncorrectCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_phyStatsCRCFailCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_phyStatsCRCFailCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_phyStatsTxDropCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_phyStatsTxDropCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_phyStatsRxDropCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_phyStatsRxDropCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_phyStatsRxTotalCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_phyStatsRxTotalCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_phyStatsBlkAvgEvm_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_phyStatsBlkAvgEvm_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_phyEmaSmoothing_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_phyEmaSmoothing_cmd);
  // PHY Read-only parameters
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_phyTxQueueLen_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_phyTxQueueLen_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_phyRxQueueLen_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_phyRxQueueLen_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_phyTxProcessingDelay_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_phyTxProcessingDelay_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_phyRxProcessingDelay_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_phyRxProcessingDelay_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_phyAgcMinGain_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_phyAgcMinGain_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_phyAgcStepValue_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_phyAgcStepValue_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_phyAgcStepNumber_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_phyAgcStepNumber_cmd);
  // PHY Propietary Attribute
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_sw_version_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_sw_version_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_zct_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_zct_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_host_version_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_phy_get_host_version_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_mtp_mode_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_set_mtp_mode_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_tx_channel_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_set_tx_channel_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_txrx_channel_list_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_get_sniffer_mode_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_phy_set_sniffer_mode_cmd);

  // MAC PIB Attributes
  // MAC Statistical Attributes
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macTxDataPktCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macTxDataPktCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macRxDataPktCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macRxDataPktCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macTxCtrlPktCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macTxCtrlPktCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macRxCtrlPktCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macRxCtrlPktCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macCSMAFailCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macCSMAFailCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macCSMAChBusyCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macCSMAChBusyCount_cmd);
  // MAC Read-Write Variables Attributes
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macMinSwitchSearchTime_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macMinSwitchSearchTime_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_set_macMinSwitchSearchTime_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_set_macMinSwitchSearchTime_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macMaxPromotionPdu_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macMaxPromotionPdu_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_set_macMaxPromotionPdu_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_set_macMaxPromotionPdu_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macPromotionPduTxPeriod_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macPromotionPduTxPeriod_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_set_macPromotionPduTxPeriod_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_set_macPromotionPduTxPeriod_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macSCPMaxTxAttempts_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macSCPMaxTxAttempts_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_set_macSCPMaxTxAttempts_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_set_macSCPMaxTxAttempts_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macMinCtlReTxTimer_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macMinCtlReTxTimer_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_set_macMinCtlReTxTimer_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_set_macMinCtlReTxTimer_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macCtrlMsgFailTime_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macCtrlMsgFailTime_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_set_macCtrlMsgFailTime_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_set_macCtrlMsgFailTime_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macEMASmoothing_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macEMASmoothing_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_set_macEMASmoothing_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_set_macEMASmoothing_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macMinBandSearchTime_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macMinBandSearchTime_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_set_macMinBandSearchTime_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_set_macMinBandSearchTime_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macPromotionMaxTxPeriod_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macPromotionMaxTxPeriod_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_set_macPromotionMaxTxPeriod_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_set_macPromotionMaxTxPeriod_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macPromotionMinTxPeriod_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macPromotionMinTxPeriod_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_set_macPromotionMinTxPeriod_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_set_macPromotionMinTxPeriod_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macSARSize_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_set_macSARSize_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macRobustnessManagement_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macRobustnessManagement_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_set_macRobustnessManagement_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macUpdatedRMTimeout_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macUpdatedRMTimeout_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_set_macUpdatedRMTimeout_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_set_macUpdatedRMTimeout_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macALVHopRepetitions_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macALVHopRepetitions_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_set_macALVHopRepetitions_cmd);
  // MAC Read-Only Variables Attributes
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macSCPChSenseCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macSCPChSenseCount_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macEUI48_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macCSMAR1_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macCSMAR1_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macCSMAR2_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macCSMAR2_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macCSMADelay_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macCSMADelay_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macCSMAR1Robust_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macCSMAR1Robust_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macCSMAR2Robust_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macCSMAR2Robust_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macCSMADelayRobust_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macCSMADelayRobust_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macAliveTimeMode_cmd);
  // MAC PIB Functional Attributes Read Only
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macLNID_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macLNID_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macLSID_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macLSID_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macSID_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macSID_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macSNA_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macSNA_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macState_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macState_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macSCPLength_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macSCPLength_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macNodeHierarchyLevel_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macNodeHierarchyLevel_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macBeaconRxPos_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macBeaconRxPos_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macBeaconTxPos_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macBeaconTxPos_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macBeaconRxFrequency_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macBeaconRxFrequency_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macBeaconTxFrequency_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macBeaconTxFrequency_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macCapabilities_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macCapabilities_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macFrameLength_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macFrameLength_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macCFPLength_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macCFPLength_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macGuardTime_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macGuardTime_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macBCMode_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macBCMode_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macBeaconRxQlty_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macBeaconRxQlty_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macBeaconTxQlty_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macBeaconTxQlty_cmd);
  // MAC Lists
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macListRegDevices_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macListActiveConn_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macListActiveConnEx_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macListMcastEntries_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macListMcastEntries_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macListSwitchTable_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macListSwitchTable_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macListDirectConn_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macListDirectTable_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macListDirectTable_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macListAvailableSwitches_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macListAvailableSwitches_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_get_macListPhyComm_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_mac_get_macListPhyComm_cmd);

  // CL432 PIBs
  cmd_install_element (PRIME_NODE, &prime_pib_cl432_get_swVersion_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_cl432_get_swVersion_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_cl432_get_connState_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_cl432_get_cl432ListNodes_cmd);

  // APP PIBs
  cmd_install_element (PRIME_NODE, &prime_pib_app_get_AppFwVersion_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_app_get_AppFwVersion_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_app_get_AppVendorId_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_app_get_AppVendorId_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_app_get_AppProductId_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_app_get_AppProductId_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_app_get_AppListZCStatus_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_pib_app_get_AppListZCStatus_cmd);

  // MTP PIBs
  cmd_install_element (PRIME_NODE, &prime_pib_mtp_get_phy_cfg_load_threshold1_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mtp_set_phy_cfg_load_threshold1_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mtp_get_phy_cfg_load_threshold2_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mtp_set_phy_cfg_load_threshold2_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mtp_get_phy_tx_time_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mtp_get_phy_rms_calc_corrected_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mtp_get_phy_continuous_tx_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mtp_set_phy_continuous_tx_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mtp_get_phy_drv_auto_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mtp_set_phy_drv_auto_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mtp_get_phy_drv_impedance_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mtp_set_phy_drv_impedance_cmd);
  // MAC
  cmd_install_element (PRIME_NODE, &prime_plme_reset_request_cmd);
  cmd_install_element (PRIME_NODE, &prime_plme_sleep_request_cmd);
  cmd_install_element (PRIME_NODE, &prime_plme_resume_request_cmd);
  cmd_install_element (PRIME_NODE, &prime_mlme_unregister_request_cmd);
  cmd_install_element (PRIME_NODE, &prime_mlme_promote_request_cmd);
  cmd_install_element (PRIME_NODE, &prime_mlme_demote_request_cmd);
  cmd_install_element (PRIME_NODE, &prime_mlme_reset_request_cmd);
  // MAC Action PIBs
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionTxData_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionConnClose_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionRegReject_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionProReject_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionUnregister_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionPromote_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionDemote_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionReject_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionAliveTime_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionBroadcastDataBurst_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionMgmtCon_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionMgmtMul_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionUnregisterBN_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionConnCloseBN_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionSegmented432_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionAppemuDataBurst_cmd);
  cmd_install_element (PRIME_NODE, &prime_pib_mac_action_macActionMgmtDataBurst_cmd);

  // Statistics
  cmd_install_element (PRIME_NODE, &prime_show_phy_statistics_cmd);
  cmd_install_element (PRIME_NODE, &prime_show_mac_statistics_cmd);
  cmd_install_element (PRIME_NODE, &prime_reset_all_statistics_cmd);
  cmd_install_element (PRIME_NODE, &prime_reset_phy_statistics_cmd);
  cmd_install_element (PRIME_NODE, &prime_reset_mac_statistics_cmd);

  cmd_install_element (PRIME_NODE, &prime_modem_reboot_request_cmd);
  cmd_install_element (PRIME_NODE, &prime_modem_bmng_reboot_request_cmd);

  // Firmware Upgrade commands
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_clear_target_list_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_add_target_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_add_all_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_set_options_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_set_match_rule_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_set_data_info_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_abort_target_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_abort_all_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_show_target_list_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_show_options_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_show_match_rule_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_show_data_info_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_set_binary_path_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_show_binary_path_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_download_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_start_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_status_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_show_version_target_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_show_state_target_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_set_signature_options_cmd);
  cmd_install_element (PRIME_NODE, &prime_bmng_fw_upgrade_show_signature_options_cmd);

  // PRIME Network commands
  cmd_install_element (PRIME_NODE, &prime_network_add_target_cmd);
  cmd_install_element (PRIME_NODE, &prime_network_del_target_cmd);
  cmd_install_element (PRIME_NODE, &prime_network_show_registered_devices_cmd);
  cmd_install_element (PRIME_NODE, &prime_network_show_cl432_connections_cmd);
  cmd_install_element (PRIME_NODE, &prime_network_show_available_switches_cmd);
  cmd_install_element (PRIME_NODE, &prime_network_show_topology_cmd);
  cmd_install_element (PRIME_NODE, &prime_network_show_level_registered_devices_cmd);

  // PRIME Zero Crossing Commands
  cmd_install_element (PRIME_NODE, &prime_modem_bmng_zc_request_cmd);

  // PRIME Service Node Commands
  cmd_install_element (PRIME_NODE, &prime_show_sn_mac_connections_cmd);

  /* Initialize daemon configuration structure(s) */
  init_prime_conf();

	prime = config_get(PRIME_NODE, "prime");
	prime_config = prime->line;

  return 0;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
