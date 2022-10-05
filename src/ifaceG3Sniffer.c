/**
 * \file
 *
 * \brief USI Host Interface to G3 Sniffer
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

#include "../ifaceG3Sniffer.h"
#include "../addUsi.h"
#include "Usi.h"
#include "UsiCfg.h"

/* Pointer to callback function to be establish*/
static g3_sniffer_msg_cb_t g3_sniffer_cb = 0;

/* buffer used to serialization */
static uint8_t uc_serial_buf[8];

/*USI CmdParams*/
CmdParams sniffer_msg;

/**
 * \brief Set sniffer callback
 *
 * \param sap_handler      Handler of the upper layer
 *
 */
void g3_sniffer_set_cb(void (*sap_handler)(uint8_t *msg, uint16_t len))
{
	g3_sniffer_cb = sap_handler;
}

/**
 * \brief Set PLC channel
 *
 * \param uc_channel      Channel to be set
 *
 */
void g3_sniffer_set_channel(uint8_t uc_channel)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	*puc_msg++ = SNIFFER_IF_PHY_COMMAND_SET_CHANNEL;
	*puc_msg++ = uc_channel;

	/*Send to USI*/
	sniffer_msg.pType = PROTOCOL_SNIF_G3;
	sniffer_msg.buf = uc_serial_buf;
	sniffer_msg.len = puc_msg - uc_serial_buf;

	usi_SendCmd(&sniffer_msg);
}

uint8_t g3_sniffer_receivedCmd(uint8_t *ptrMsg, uint16_t len)
{
	if (g3_sniffer_cb) {
		g3_sniffer_cb(ptrMsg, len);
	}

	return(TRUE);
}
