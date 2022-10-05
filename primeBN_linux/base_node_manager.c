/**
 * \file
 *
 * \brief Base Node Manager file.
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
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
//#include <linux/in.h>
#include <linux/if.h>
#include <linux/if_tun.h>
//#include <netinet/in.h>
#include <poll.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <arpa/inet.h>

#include <signal.h>
#include <unistd.h> // usleep
#include <time.h> // usleep
#include <pthread.h>

#include <limits.h>
#include "globals.h"

#include "gpio.h"
#include "led.h"

#include "prime_log.h"
#include "return_codes.h"

#include "addUsi.h"
#include "prime_api_host.h"
#include "prime_api_defs_host.h"
#include "mac_pib.h"
#include "mac_defs.h"
#include "base_node_manager.h"
#include "base_node_mng.h"
#include "base_node_network.h"

/************************************************************
*       Defines                                             *
*************************************************************/
#define COMMS_FD 0
#define POLLTIMEOUT 20 /* ms */

#define MIKROBUS1 0
#define MIKROBUS2 1
#define MIKROBUS1_GPIO_RESET "PB2"  /* Reset for mikroBUS 1 is mapped on PB2  */
#define MIKROBUS2_GPIO_RESET "PA26" /* Reset for mikroBUS 2 is mapped on PA26 */

/* Global Configuration */
struct st_configuration g_st_config = {
		.b_verbose      = 0,
		.loglevel       = 0,
		.sz_tty_name    = "/dev/ttyS1",
		.sz_tty_speed   = 230400,                       /*! Serial Port Interface speed to ADP_MAC_SERIALIZED board */
		.sz_hostname    = "127.0.0.1",
		.sz_tcp_port    = 3000,
		.sz_port_type   = 0,
    .mikroBUS       = MIKROBUS1,                    /*! Mikrobus Interface to ADP_MAC_SERIALIZED board - for Reset Pin selection */
		.mode           = 3,                            /* Base Node */
		.state          = 0,														/* */
		.tx_channel     = 1,
		.band_plan      = 1,
		.mtp_mode       = 0,                            /* NO MTP mode */
		.cert_mode      = 0,														/* NO Certification mode */
		.sniffer_mode   = 0,                            /* NO Embedded Sniffer enabled */
		.dlmsotcp_mode  = 0,														/* DLMS over TCP Server */
		.eui48          = { 0x0,0x1,0x2,0x3,0x4,0x5 },
		.sec_profile    = SECURITY_PROFILE_UNKNOWN,
		.duk            = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F }
};

struct st_status g_st_status = {
		.daemon         = 0,
		.sniffer_tcp    = 0,
		.usi_link       = 0,
		.fup_status     = 0
};

struct st_info g_st_info = {
		.app_vendor_id     = 0,
		.app_product_id    = 0,
		.app_version       = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.phy_sw_version    = 0,
		.phy_host_version  = 0,
		.mac_sw_version    = 0,
		.cl432_sw_version   = 0
};

int g_usi_fd=0;
/* Global pthread descriptor */
pthread_t usi_thread = 0;

/*
 * \brief Resets PLC Modem if connected to mikroBUS (PL360G55-CF Board)
 * \param  mikroBUS
 * \return 0
 */
void reset_modem(int mikroBUS)
{
	char gpioname[8];

	memset(gpioname,'0',8);
	if (MIKROBUS1 == mikroBUS)
		 strncpy(gpioname,MIKROBUS1_GPIO_RESET,8);
	else
		 strncpy(gpioname,MIKROBUS2_GPIO_RESET,8);

  SET_GPIO(gpioname,0);
  sleep(1);
  SET_GPIO(gpioname,1);
  sleep(1);
}

/*
 * \brief Pthread to handle usi_process
 *        usi_process needs to be called periodically to transmit/receive
 *        USI Frames to/from serial port*
 */
void * usi_process_thread(void * thread_parameters)
{
  while (1){
		 addUsi_Process();
		 usleep(1000);
	}
	pthread_exit(NULL);
}

/*
 * \brief Initialize USI management - Init USI Port - Starts USI Process Thread
 * \return 0
 */
int initUSImanagement(void)
{
#if 0
	static int initUSI = 0;
	int s;
	void * res;

	if (initUSI){
		 /* Someone Initialize this process before so, kill usi_thread in process */
		 s = pthread_cancel(usi_thread);
     if (s != 0){
        LOG_ERR(Log("Error Canceling USI Thread"));
		 }
     /* Join with thread to see what its exit status was */
     s = pthread_join(usi_thread, &res);
     if (s != 0){
        LOG_ERR(Log("Error Joining USI Thread"));
		 }

     if (res == PTHREAD_CANCELED){
        LOG_DBG(Log("USI Thread cancelled"));
		 }else{
		    LOG_ERR(Log("Error cancelling USI Thread"));
		 }
	}
#endif
	/* Init USI/Base serial connection */
	addUsi_Init();

	// Create usi_process thread to handle USI Frames on serial port
  pthread_attr_t usi_thread_attr;

  pthread_attr_init( &usi_thread_attr );
  pthread_attr_setdetachstate( &usi_thread_attr, PTHREAD_CREATE_DETACHED );
  /* Create a thread which executes addUsi_Process() periodically */
  if (pthread_create(&usi_thread, NULL, (void *)usi_process_thread, NULL)) {
      PRIME_LOG(LOG_ERR,"Error creating USI Thread\r\n");
      return -1;
  }
  pthread_attr_destroy( &usi_thread_attr );
#if 0
	initUSI++;
#endif
  return 0;
}

/*
 * \brief Get Information related with PRIME Layers Version
 */
void prime_get_fw_information(void)
{
	struct TmacGetConfirm macGetConfirm;

  /* PIB_PHY_SW_VERSION */
	prime_cl_null_plme_get_request_sync(PIB_PHY_SW_VERSION,PRIME_SYNC_TIMEOUT_GET_REQUEST,&macGetConfirm);
	if ((macGetConfirm.m_u8Status == PLME_RESULT_SUCCESS) && (macGetConfirm.m_u8AttributeLength == 4)){
      // 4 Bytes should be the result
			memcpy(&g_st_info.phy_sw_version,macGetConfirm.m_au8AttributeValue,4);
      PRIME_LOG(LOG_INFO,"PRIME PHY SW Version 0x%08X\r\n",	g_st_info.phy_sw_version);
  }

	/* PIB_PHY_HOST_VERSION */
	prime_cl_null_plme_get_request_sync(PIB_PHY_HOST_VERSION,PRIME_SYNC_TIMEOUT_GET_REQUEST,&macGetConfirm);
	if ((macGetConfirm.m_u8Status == PLME_RESULT_SUCCESS) && (macGetConfirm.m_u8AttributeLength == 4)){
      // 4 Bytes should be the result
			memcpy(&g_st_info.phy_host_version,macGetConfirm.m_au8AttributeValue,4);
      PRIME_LOG(LOG_INFO,"PRIME PHY HOST Version 0x%08X\r\n", g_st_info.phy_host_version);
  }

  /* PIB_MAC_INTERNAL_SW_VERSION */
	prime_cl_null_mlme_get_request_sync(PIB_MAC_INTERNAL_SW_VERSION,PRIME_SYNC_TIMEOUT_GET_REQUEST,&macGetConfirm);
	if ((macGetConfirm.m_u8Status == MLME_RESULT_DONE) && (macGetConfirm.m_u8AttributeLength == 4)){
			memcpy(&g_st_info.mac_sw_version,macGetConfirm.m_au8AttributeValue,4);
			// 4 Bytes should be the result
			PRIME_LOG(LOG_INFO,"PRIME MAC SW Version 0x%08X\r\n", g_st_info.mac_sw_version);
	}
	/* PIB_432_INTERNAL_SW_VERSION - 4 Bytes */
	prime_cl_null_mlme_get_request_sync(PIB_432_INTERNAL_SW_VERSION,PRIME_SYNC_TIMEOUT_GET_REQUEST,&macGetConfirm);
	if ((macGetConfirm.m_u8Status == MLME_RESULT_DONE) && (macGetConfirm.m_u8AttributeLength == 4)){
			memcpy(&g_st_info.cl432_sw_version,macGetConfirm.m_au8AttributeValue,4);
			// 4 Bytes should be the result
			PRIME_LOG(LOG_INFO,"PRIME 4-32 SW Version 0x%08X\r\n",g_st_info.cl432_sw_version);
	}
  /* PRIME_FW_VERSION - 16 Bytes */
	prime_cl_null_mlme_get_request_sync(PIB_MAC_APP_FW_VERSION,PRIME_SYNC_TIMEOUT_GET_REQUEST,&macGetConfirm);
	if ((macGetConfirm.m_u8Status == MLME_RESULT_DONE) && (macGetConfirm.m_u8AttributeLength == 16)){
			memcpy(&g_st_info.app_version,macGetConfirm.m_au8AttributeValue,16);
			// 4 Bytes should be the result
			PRIME_LOG(LOG_INFO,"PRIME FW Version %s\r\n",g_st_info.app_version);
	}
	/* PRIME_FW_VENDOR - 2 Bytes */
	prime_cl_null_mlme_get_request_sync(PIB_MAC_APP_VENDOR_ID,PRIME_SYNC_TIMEOUT_GET_REQUEST,&macGetConfirm);
	if ((macGetConfirm.m_u8Status == MLME_RESULT_DONE) && (macGetConfirm.m_u8AttributeLength == 2)){
			memcpy(&g_st_info.app_vendor_id,macGetConfirm.m_au8AttributeValue,2);
			// 4 Bytes should be the result
			PRIME_LOG(LOG_INFO,"PRIME FW Vendor 0x%04X\r\n", g_st_info.app_vendor_id);
	}
	/* PRIME_FW_MODEL - 2 Bytes */
	prime_cl_null_mlme_get_request_sync(PIB_MAC_APP_PRODUCT_ID,PRIME_SYNC_TIMEOUT_GET_REQUEST,&macGetConfirm);
	if ((macGetConfirm.m_u8Status == MLME_RESULT_DONE) && (macGetConfirm.m_u8AttributeLength == 2)){
			memcpy(&g_st_info.app_product_id,macGetConfirm.m_au8AttributeValue,2);
			// 4 Bytes should be the result
			PRIME_LOG(LOG_INFO,"PRIME Product ID 0x%04X\r\n",	g_st_info.app_product_id);
	}

	return;
}

/*
 * \brief PRIME Node Init
 */
 int prime_node_init(void)
 {
  /* Initialize Base Node Management */
  base_node_manager_mng_init();

	/* Get List of Service Nodes on the Network ¿? */
  return SUCCESS;
}

/*
 * \brief PRIME Node Loop
 */
 void * prime_node_loop(void * thread_parameters)
 {
  while(1){
		/* Nothing to do */
		sleep(1);
	}
  return SUCCESS;
}

/*
 * \brief PRIME Node Main
 */
int prime_node_main (int argc, char **argv)
{

  SET_RGB_OFF()
	SET_LED_GREEN_TIMER(500,500)

	  /* Init USI Management */
  initUSImanagement();

	/* PRIME Node Initialization */
	prime_node_init();

	/* Print Banner */
	PRIME_LOG(LOG_INFO,"Microchip Prime Node\r\n");

	/*! Show PRIME Base Node API versions */
	prime_get_fw_information( );

	SET_LED_GREEN()

  /// Loop
  pthread_t main_thread;
  pthread_attr_t main_thread_attr;

  pthread_attr_init( &main_thread_attr );
  pthread_attr_setdetachstate( &main_thread_attr, PTHREAD_CREATE_DETACHED );
  pthread_create( &main_thread, &main_thread_attr, (void *)prime_node_loop, NULL );
  pthread_attr_destroy( &main_thread_attr );

  return 0;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
