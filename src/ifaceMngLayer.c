/**
 * \file
 *
 * \brief USI Host Interface to PRIME Management layer
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

/* *************************************Includes********************************* */
#include <string.h>

#include "../addUsi.h"

#include "../mngLayerHost.h"

#include "Usi.h"

/* *************************************Definitions****************************** */
#define HEAD_LENGTH     2

#define MAX_LENGTH_TX_BUFFER    512

#define LENGTH_PIB                                      2
#define LENGTH_INDEX                            1
#define LENGTH_GET_PIB_QUERY            (LENGTH_PIB + LENGTH_INDEX)

#define TYPE_HEADER(type)                       (type & 0x003F)
#define EN_PIBQRY_SHORT_ITERATOR        0
#define EN_PIBQRY_LONG_ITERATOR         1

#define ITERATOR_TYPE_SHIFT                     7
#define ITERATOR_TYPE_MASK                      0x01
#define ITERATOR_LENGTH_MASK            0x7F

#define LITERATOR_EN_LIST_LEN(iterLen) (iterLen + 4)
#define SLITERATOR_EN_LIST_LEN                  6

/* *************************************Local Vars******************************* */
/* Transmission buffer */
CmdParams txMsg;
uint16_t numCharsTxBuff = 0;
uint8_t buffTx[MAX_LENGTH_TX_BUFFER];

/* Pointer to callback function to be establish*/
static mngp_rsp_cb_t mngp_rsp_cb = 0;

/* ************************************************************************** */

/** @brief	Initializes the transmission buffer
 *
 *  @param		-
 *
 *  @return		_
 **************************************************************************/
void mngLay_NewMsg(uint8_t cmd)
{
	numCharsTxBuff = 0;
	txMsg.pType = cmd;
}

/* ************************************************************************** */

/** @brief	Get Enhanced Pib List Query length
 *
 *  @param		iter	List Iterator
 *
 *  @return		Query Length
 **************************************************************************/
uint8_t _gePibListEnLength(uint8_t *iter)
{
	uint8_t len;
	uint8_t iterType;
	uint8_t iterLength;

	iterType = (iter[0] >> ITERATOR_TYPE_SHIFT) & ITERATOR_TYPE_MASK;

	switch (iterType) {
	case EN_PIBQRY_SHORT_ITERATOR:
		len = SLITERATOR_EN_LIST_LEN;
		break;

	case EN_PIBQRY_LONG_ITERATOR:
		iterLength = iter[0] & ITERATOR_LENGTH_MASK;
		len = LITERATOR_EN_LIST_LEN(iterLength);
		break;

	default:
		len = 0;
		break;
	}

	return len;
}

/* ************************************************************************** */

/** @brief	put in the Transmission buffer a GetPibQuery message
 *
 *  @param		pib
 *  @param		index
 *
 *  @return		TRUE  - OK
 *              FALSE - ERROR
 **************************************************************************/
uint8_t mngLay_AddGetPibQuery(uint16_t pib, uint8_t index)
{
	uint8_t *ptrBuff;

	/* Error conditions */
	/* if there is other message without send */
	if (txMsg.pType != MNGP_PRIME_GETQRY) {
		return(FALSE);
	}

	/* There is no room in the buffer */
	if ((numCharsTxBuff + LENGTH_GET_PIB_QUERY) >= MAX_LENGTH_TX_BUFFER) {
		return(FALSE);
	}

	ptrBuff = &buffTx[numCharsTxBuff];

	*ptrBuff++ = (uint8_t)(pib >> 8);
	*ptrBuff++ = (uint8_t)(pib & 0xFF);
	*ptrBuff++ = index;

	/* Update the length of the message */
	numCharsTxBuff += 3;

	return(TRUE);
}

/* ************************************************************************** */

/* ************************************************************************** */

/** @brief	put in the Transmission buffer a SetPib message
 *
 *  @param		pib
 *  @param		length
 *  @param		&msg address for the data
 *
 *  @return		TRUE  - OK
 *              FALSE - ERROR
 **************************************************************************/
uint8_t mngLay_AddSetPib(uint16_t pib, uint16_t length, uint8_t *msg)
{
	uint8_t *ptrBuff;

	/* Error conditions */
	/* if there is other message without send */
	if (txMsg.pType  != MNGP_PRIME_SET) {
		return(FALSE);
	}

	/* There is no room in the buffer */
	if ((numCharsTxBuff + LENGTH_PIB + length) >= MAX_LENGTH_TX_BUFFER) {
		return(FALSE);
	}

	ptrBuff = &buffTx[numCharsTxBuff];

	*ptrBuff++ = (uint8_t)(pib >> 8);
	*ptrBuff++ = (uint8_t)(pib & 0xFF);
	memcpy(ptrBuff, msg, length);

	/* Update the length of the message */
	numCharsTxBuff += LENGTH_PIB + length;

	return(TRUE);
}

/* ************************************************************************** */

/** @brief	put in the Transmission buffer a Reset Stats message
 *
 *  @param		-
 *
 *  @return		TRUE  - OK
 *              FALSE - ERROR
 **************************************************************************/
uint8_t mngLay_AddResetStats(uint16_t pib, uint8_t index)
{
	uint8_t *ptrBuff;

	/* Error conditions */
	/* if there is other message without send */
	if (txMsg.pType != MNGP_PRIME_RESET) {
		return(FALSE);
	}

	/* There is no room in the buffer */
	if ((numCharsTxBuff + LENGTH_GET_PIB_QUERY) >= MAX_LENGTH_TX_BUFFER) {
		return(FALSE);
	}

	ptrBuff = &buffTx[numCharsTxBuff];

	*ptrBuff++ = (uint8_t)(pib >> 8);
	*ptrBuff++ = (uint8_t)(pib & 0xFF);
	*ptrBuff++ = index;

	/* Update the length of the message */
	numCharsTxBuff += 3;

	return(TRUE);
}

/* ************************************************************************** */

/** @brief	put in the Transmission buffer a FU message
 *
 *  @param		length
 *  @param		&msg address for the data
 *
 *  @return		TRUE  - OK
 *              FALSE - ERROR
 **************************************************************************/
uint8_t mngLay_AddFUMsg(uint16_t length, uint8_t *msg)
{
	uint8_t *ptrBuff;

	/* Error conditions */
	/* if there is other message without send */
	if (txMsg.pType  != MNGP_PRIME_FU) {
		return(FALSE);
	}

	/* There is no room in the buffer */
	if ((numCharsTxBuff + length) >= MAX_LENGTH_TX_BUFFER) {
		return(FALSE);
	}

	ptrBuff = &buffTx[numCharsTxBuff];

	memcpy(ptrBuff, msg, length);

	/* Update the length of the message */
	numCharsTxBuff += length;

	return(TRUE);
}

/**
 * put in the Transmission buffer a GetPibQuery message
 *
 *  @param		pib
 *  @param		maxRecords : maximum number of items that fits in the message
 *  @param      iterator   : Iterator as defined in prime spec. Iterator size will be handled automatically.
 *
 *  @return     TRUE  - OK
 *              FALSE - ERROR
 **************************************************************************/
uint8_t mngLay_AddGetPibListEnQuery(uint16_t pib, uint8_t maxRecords, uint8_t *iterator)
{
	uint8_t *ptrBuff;
	uint8_t len;

	/* Error conditions */
	/* if there is other message without send */
	if (txMsg.pType != PROTOCOL_MNGP_PRIME_GETQRY_EN) {
		return(FALSE);
	}

	/* get item length */
	len = _gePibListEnLength(iterator);

	/* There is no room in the buffer */
	if ((numCharsTxBuff + len) >= MAX_LENGTH_TX_BUFFER) {
		return(FALSE);
	}

	ptrBuff = &buffTx[numCharsTxBuff];

	*ptrBuff++ = MNGP_PRIME_LISTQRY;
	*ptrBuff++ = (uint8_t)(pib >> 8);
	*ptrBuff++ = (uint8_t)(pib & 0xFF);
	*ptrBuff++ = maxRecords;

	memcpy(ptrBuff, iterator, len - 4);

	/* Update the length of the message */
	numCharsTxBuff += len;

	return(TRUE);
}

/* ************************************************************************** */

/** @brief	put in the Transmission buffer a raw Management Plane message
 *
 *  @param		length
 *  @param		&msg address for the data
 *
 *  @return		TRUE  - OK
 *              FALSE - ERROR
 **************************************************************************/
uint8_t mngLay_BridgeMsg(uint16_t length, uint8_t *msg)
{
	/* If there is other message pending to send */
	if (numCharsTxBuff) {
		return(FALSE);
	}

	/* Look for the type of the message */
	msg++;
	txMsg.pType = (*msg) & 0x3f;
	msg++;

	/* There is no room in the buffer */
	if ((length - 2) >= MAX_LENGTH_TX_BUFFER) {
		return(FALSE);
	}

	/* Update the length of the message */
	numCharsTxBuff = length - 2;

	memcpy(buffTx, msg, numCharsTxBuff);

	return mngLay_SendMsg();
}

/* ************************************************************************** */

/** @brief	Send to the usi the message
 *
 *  @param		-
 *
 *  @return		TRUE  - SENT
 *              FALSE - NO SENT
 **************************************************************************/
uint8_t mngLay_SendMsg(void)
{
	txMsg.len = numCharsTxBuff;
	txMsg.buf = buffTx;
	if (!usi_SendCmd(&txMsg)) {
		return(FALSE);
	}

	/* if the transmission is ok, the message is deleted from the buffer */
	numCharsTxBuff = 0;
	return(TRUE);
}

/* ************************************************************************** */

/**
 * \brief Set the callback function for the MNGP response
 *
 * \param sap_handler      Handler of the upper layer
 *
 *
 **************************************************************************/
void mngp_set_rsp_cb(void (*sap_handler)(uint8_t *ptrMsg, uint16_t len))
{
	mngp_rsp_cb = sap_handler;
}

uint8_t mngLay_receivedCmd(uint8_t *ptrMsg, uint16_t len)
{
	if (mngp_rsp_cb) {
		mngp_rsp_cb(ptrMsg, len);
	}

	return(TRUE);
}
