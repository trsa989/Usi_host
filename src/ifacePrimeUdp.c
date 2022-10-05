/**
 * \file
 *
 * \brief USI Host - Interface Prime UDP
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

#include "../ifacePrimeUdp.h"
#include "../addUsi.h"
#include "Usi.h"
#include "UsiCfg.h"

#include <stdlib.h>
#define MAX_LENGTH 1024

/* Pointer to callback function to be establish*/
static primeoudp_msg_cb_t primeoudp_cb = 0;

/* buffer used to serialization */
static uint8_t uc_serial_buf[1024];

/*USI CmdParams*/
CmdParams primeoudp_msg;

/**
 * \brief Set Prime over Udp callback
 *
 * \param sap_handler      Handler of the upper layer
 *
 */

void primeoudp_set_cb(void (*sap_handler)(uint8_t *msg, uint16_t len))
{
	primeoudp_cb = sap_handler;
}

/**
 * \brief callback handler
 *        Data must be a RAW PRIME PDU with CRC.
 *
 * \param ptrMsg    Buffer containing message data
 * \param len       Buffer length
 *
 */

uint8_t primeoudp_rcv(uint8_t *ptrMsg, uint16_t len)
{
	if (primeoudp_cb) {
		primeoudp_cb(ptrMsg, len);
	}

	return(TRUE);
}

/**
 * \brief Send data to device.
 *
 * \param ptrMsg    Buffer containing message data
 * \param len       Buffer length
 *
 */
void primeoudp_send(uint8_t *ptrMsg, uint16_t len)
{
	uint8_t *puc_msg;

	/*Insert parameters*/
	puc_msg = uc_serial_buf;

	memcpy(puc_msg, ptrMsg, len);

	/*Send to USI*/
	primeoudp_msg.pType = PROTOCOL_PHY_SERIAL_PRIME;
	primeoudp_msg.buf = uc_serial_buf;
	primeoudp_msg.len = len;

	usi_SendCmd(&primeoudp_msg);
}
