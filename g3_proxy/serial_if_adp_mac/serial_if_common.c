/**
 * \file
 *
 * \brief Serial Interface for MAC layer
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

#include <stdbool.h>
#include <stdint.h>
/* #include <stdio.h> */
#include <PrjCfg.h>
#include "serial_if_common.h"
#include "serial_if_mac.h"
#include "serial_if_adp.h"
#include "serial_if_coordinator.h"
#include "usi_cli.h"
#include "AdpApi.h"
#include "mac_wrapper.h"
#include "bs_api.h"

#include "usi_cfg_cli.h"
#include "debug.h"

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

usi_decode_cmd_cb cbFnCliSnifferG3;
usi_decode_cmd_cb cbFnCliG3Mac;
usi_decode_cmd_cb cbFnCliG3Adp;
usi_decode_cmd_cb cbFnCliG3Coord;

static enum ESerialMode e_serial_status = SERIAL_MODE_NOT_INITIALIZED;

/**
 * \brief Display SW version in console
 */
static void _show_version( void )
{
	/*	struct TAdpGetConfirm getConfirm; */
	/*	struct TAdpMacGetConfirm x_pib_confirm; */

#if defined (CONF_BAND_CENELEC_A)
	PRINTF(PRINT_INFO, "G3 Band: CENELEC-A\r\n");
#elif defined (CONF_BAND_FCC)
	PRINTF(PRINT_INFO, "G3 Band: FCC\r\n");
#elif defined (CONF_BAND_ARIB)
	PRINTF(PRINT_INFO, "G3 Band: ARIB\r\n");
#else
	PRINTF(PRINT_INFO, "G3 Band: CENELEC-A\r\n");
#endif

	/*
	 *      AdpGetRequestSync(ADP_IB_SOFT_VERSION, 0, &getConfirm);
	 *      if (getConfirm.m_u8AttributeLength == 6) {
	 *              printf("G3 stack version: %hu.%hu.%hu Date: 20%hu-%hu-%hu\r\n",
	 *                              getConfirm.m_au8AttributeValue[0],
	 *                              getConfirm.m_au8AttributeValue[1],
	 *                              getConfirm.m_au8AttributeValue[2],
	 *                              getConfirm.m_au8AttributeValue[3],
	 *                              getConfirm.m_au8AttributeValue[4],
	 *                              getConfirm.m_au8AttributeValue[5]);
	 *      }
	 *
	 *      AdpGetRequestSync(ADP_IB_MANUF_ADP_INTERNAL_VERSION, 0, &getConfirm);
	 *      if (getConfirm.m_u8AttributeLength == 6) {
	 *              printf("ADP version: %hu.%hu.%hu Date: 20%hu-%hu-%hu\r\n",
	 *                              getConfirm.m_au8AttributeValue[0],
	 *                              getConfirm.m_au8AttributeValue[1],
	 *                              getConfirm.m_au8AttributeValue[2],
	 *                              getConfirm.m_au8AttributeValue[3],
	 *                              getConfirm.m_au8AttributeValue[4],
	 *                              getConfirm.m_au8AttributeValue[5]);
	 *      }
	 *
	 *      AdpMacGetRequestSync((uint32_t)MAC_WRP_PIB_MANUF_MAC_INTERNAL_VERSION, 0, &x_pib_confirm);
	 *      if (x_pib_confirm.m_u8AttributeLength == 6) {
	 *              printf("MAC version: %hu.%hu.%hu Date: 20%hu-%hu-%hu\r\n",
	 *                              x_pib_confirm.m_au8AttributeValue[0],
	 *                              x_pib_confirm.m_au8AttributeValue[1],
	 *                              x_pib_confirm.m_au8AttributeValue[2],
	 *                              x_pib_confirm.m_au8AttributeValue[3],
	 *                              x_pib_confirm.m_au8AttributeValue[4],
	 *                              x_pib_confirm.m_au8AttributeValue[5]);
	 *      }
	 *
	 *      AdpMacGetRequestSync((uint32_t)MAC_WRP_PIB_MANUF_MAC_RT_INTERNAL_VERSION, 0, &x_pib_confirm);
	 *      if (x_pib_confirm.m_u8AttributeLength == 6) {
	 *              printf("MAC RT version: %hu.%hu.%hu Date: 20%hu-%hu-%hu\r\n",
	 *                              x_pib_confirm.m_au8AttributeValue[0],
	 *                              x_pib_confirm.m_au8AttributeValue[1],
	 *                              x_pib_confirm.m_au8AttributeValue[2],
	 *                              x_pib_confirm.m_au8AttributeValue[3],
	 *                              x_pib_confirm.m_au8AttributeValue[4],
	 *                              x_pib_confirm.m_au8AttributeValue[5]);
	 *      }
	 *
	 *      AdpMacGetRequestSync((uint32_t)MAC_WRP_PIB_MANUF_PHY_PARAM, MAC_WRP_PHY_PARAM_VERSION, &x_pib_confirm);
	 *      if (x_pib_confirm.m_u8AttributeLength == 4) {
	 *              printf("PHY version: %02x.%02x.%02x.%02x\r\n",
	 *                              x_pib_confirm.m_au8AttributeValue[3],
	 *                              x_pib_confirm.m_au8AttributeValue[2],
	 *                              x_pib_confirm.m_au8AttributeValue[1],
	 *                              x_pib_confirm.m_au8AttributeValue[0]);
	 *      }
	 */

	return;
}

void adp_mac_serial_if_init(void)
{
	/* Set usi callbacks */
#ifdef USE_PROTOCOL_MAC_G3_PORT
	cbFnCliG3Mac = serial_if_g3mac_api_parser;
#endif
#ifdef USE_PROTOCOL_ADP_G3_PORT
	cbFnCliG3Adp = serial_if_g3adp_api_parser;
#endif
#ifdef USE_PROTOCOL_COORD_G3_PORT
	cbFnCliG3Coord = serial_if_coordinator_api_parser;
#endif
}

void adp_mac_serial_if_process(void)
{
	if (e_serial_status == SERIAL_MODE_ADP) {
		AdpEventHandler();
	} else if (e_serial_status == SERIAL_MODE_COORD) {
		AdpEventHandler();
#ifdef USE_PROTOCOL_COORD_G3_PORT
		bs_process();
#endif
	} else if (e_serial_status == SERIAL_MODE_MAC) {
#ifdef USE_PROTOCOL_MAC_G3_PORT
		MacEventHandler();
#endif
	}
}

void adp_mac_serial_if_set_state(enum ESerialMode e_state)
{
	e_serial_status = e_state;

	switch (e_state) {
	case SERIAL_MODE_MAC:
		PRINTF(PRINT_INFO, "Serial mode initialized as MAC\r\n");
		break;

	case SERIAL_MODE_ADP:
		PRINTF(PRINT_INFO, "Serial mode initialized as ADP\r\n");
		break;

	case SERIAL_MODE_COORD:
		PRINTF(PRINT_INFO, "Serial mode initialized as COORD\r\n");
		break;

	default:
		break;
	}

	_show_version();
}

enum ESerialMode  adp_mac_serial_if_get_state(void)
{
	return e_serial_status;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
