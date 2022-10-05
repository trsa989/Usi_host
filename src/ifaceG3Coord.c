/**
 * \file
 *
 * \brief USI Host Interface to G3 Coordinator Layer
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
#include <stdlib.h>

#include "../addUsi.h"
#include "../G3.h"

#include <bs_api.h>

#include "Usi.h"

/* //// *** Declarations ********************************************************** */

#define MAX_SIZE_MAC_BUFFER     G3_MACSAP_DATA_SIZE + (G3_MACSAP_DATA_SIZE / 2)

/* // *************************************Local Vars******************************* */
/* // Transmission buffer */
static Uint8 buffTxCoordG3[G3_MACSAP_DATA_SIZE];
static CmdParams coordG3Msg;

/* Callbacks */
static pf_app_leave_ind_cb_t g3_app_leave_ind_cb = 0;
static pf_app_join_ind_cb_t g3_app_join_ind_cb = 0;

/**
 * @brief g3_coordInitialize
 * Use this function to initialize the Coordinator layer.
 * The Coordinator layer should be initialized before doing any other operation.
 * @param band: Working band for phy (shoud be inline with the firmware/hardware)
 * @return (int) Status/Result
 */
void bs_init(TBootstrapConfiguration s_bs_conf)
{
	Uint8 *ptrBuff;
	Uint16 length;
	int result;

	ptrBuff = buffTxCoordG3;

	*ptrBuff++ = G3_SERIAL_MSG_COORD_INITIALIZE;
	*ptrBuff++ = s_bs_conf.m_u8BandInfo;

	length = ptrBuff - buffTxCoordG3;
	coordG3Msg.pType = PROTOCOL_COORD_G3;
	coordG3Msg.buf = buffTxCoordG3;
	coordG3Msg.len = length;
	/* Send packet */
	result = usi_SendCmd(&coordG3Msg) ? 0 : -1;

	LOG_G3_DEBUG("bs_init result = %d\r\n", result);
}

/* Bootstrap module process, must be called at least once a second */
void bs_process(void)
{
}

/**
 * @brief g3_bootstrapGetRequest
 * The BootstrapGetRequest primitive allows the upper layer to get the value of an attribute
 * from the Bootstrap (Coordinator) information base.
 * @param attribute_id: The identifier of the Bootstrap IB attribute to read.
 * @param attribute_index: The index within the table of the specified IB attribute to read.
 * @return information asked in the request.
 * @return
 */
void bs_lbp_get_param(uint32_t ul_attribute_id, uint16_t us_attribute_idx, struct t_bs_lbp_get_param_confirm *p_get_confirm)
{
	Uint8 *ptrBuff;
	Uint16 length;
	int result;

	ptrBuff = buffTxCoordG3;

	*ptrBuff++ = G3_SERIAL_MSG_COORD_GET_REQUEST;
	*ptrBuff++ = ((ul_attribute_id >> 24) & 0xFF);
	*ptrBuff++ = ((ul_attribute_id >> 16) & 0xFF);
	*ptrBuff++ = ((ul_attribute_id >> 8) & 0xFF);
	*ptrBuff++ = (ul_attribute_id & 0xFF);
	*ptrBuff++ = (us_attribute_idx >> 8);
	*ptrBuff++ = (us_attribute_idx & 0xFF);

	length = ptrBuff - buffTxCoordG3;
	coordG3Msg.pType = PROTOCOL_COORD_G3;
	coordG3Msg.buf = buffTxCoordG3;
	coordG3Msg.len = length;
	/* Send packet */
	result = usi_SendCmd(&coordG3Msg) ? 0 : -1;

	LOG_G3_DEBUG("bs_lbp_get_param result = %d", result);
}

/**
 * @brief g3_bootstrapSetRequest
 * The BootstrapSetRequest primitive allows the upper layer to set the value of an attribute
 * in the Bootstrap (Coordinator) information base.
 * @param attribute_id: The identifier of the Bootstrap IB attribute to write.
 * @param attribute_index: The index within the table of the specified IB attribute to write.
 * @param attribute_data: The data of the Bootstrap IB attribute.
 * @param attribute_len: The length of the Bootstrap IB attribute.
 * @return result of the request
 */
void bs_lbp_set_param(uint32_t ul_attribute_id, uint16_t us_attribute_idx, uint8_t uc_attribute_len, const uint8_t *puc_attribute_value,
		struct t_bs_lbp_set_param_confirm *p_set_confirm)
{
	Uint8 *ptrBuff;
	Uint16 length;
	int i, result;

	ptrBuff = buffTxCoordG3;

	*ptrBuff++ = G3_SERIAL_MSG_COORD_SET_REQUEST;
	*ptrBuff++ = ((ul_attribute_id >> 24) & 0xFF);
	*ptrBuff++ = ((ul_attribute_id >> 16) & 0xFF);
	*ptrBuff++ = ((ul_attribute_id >> 8) & 0xFF);
	*ptrBuff++ = (ul_attribute_id & 0xFF);
	*ptrBuff++ = (us_attribute_idx >> 8);
	*ptrBuff++ = (us_attribute_idx & 0xFF);
	*ptrBuff++ = uc_attribute_len;
	for (i = 0; i < uc_attribute_len; i++) {
		*ptrBuff++ = puc_attribute_value[i];
	}

	length = ptrBuff - buffTxCoordG3;
	coordG3Msg.pType = PROTOCOL_COORD_G3;
	coordG3Msg.buf = buffTxCoordG3;
	coordG3Msg.len = length;
	/* Send packet */
	result = usi_SendCmd(&coordG3Msg) ? 0 : -1;

	p_set_confirm->ul_attribute_id = ul_attribute_id;
	p_set_confirm->us_attribute_idx = us_attribute_idx;
	p_set_confirm->uc_status = result == 0 ? LBP_STATUS_OK : LBP_STATUS_NOK;

	LOG_G3_DEBUG("bs_lbp_set_param attribute id = 0x%02x; index = %u; result = %d\r\n", ul_attribute_id, us_attribute_idx, result);
}

/**
 * @brief g3_bootstrapLbpRekeyingStart
 * @return
 */
void bs_lbp_launch_rekeying()
{
	Uint8 *ptrBuff;
	Uint16 length;
	int result;

	ptrBuff = buffTxCoordG3;

	*ptrBuff++ = G3_SERIAL_MSG_COORD_REKEYING_REQUEST;

	length = ptrBuff - buffTxCoordG3;
	coordG3Msg.pType = PROTOCOL_COORD_G3;
	coordG3Msg.buf = buffTxCoordG3;
	coordG3Msg.len = length;
	/* Send packet */
	result = usi_SendCmd(&coordG3Msg) ? 0 : -1;

	LOG_G3_DEBUG("bs_lbp_launch_rekeying result = %d", result);
}

/**
 * @brief g3_bootstrapLbpKickRequest
 * @param short_addr
 * @return
 */
void bs_lbp_kick_device(uint16_t us_short_address)
{
	Uint8 *ptrBuff;
	Uint16 length;
	int result;

	ptrBuff = buffTxCoordG3;

	*ptrBuff++ = G3_SERIAL_MSG_COORD_KICK_REQUEST;
	*ptrBuff++ = (us_short_address >> 8);
	*ptrBuff++ = (us_short_address & 0xFF);

	length = ptrBuff - buffTxCoordG3;
	coordG3Msg.pType = PROTOCOL_COORD_G3;
	coordG3Msg.buf = buffTxCoordG3;
	coordG3Msg.len = length;
	/* Send packet */
	result = usi_SendCmd(&coordG3Msg) ? 0 : -1;

	LOG_G3_DEBUG("bs_lbp_kick_device result = %d", result);
}

/*  */
/* / ** */
/* * @brief _cl_null_coordSetConfirm_cb */
/* * @param ptrMsg: Received message */
/* * @param len: Received message length */
/* * @return */
/* * / */
/* uint8_t _cl_null_coordGetConfirm_cb(uint8_t* ptrMsg, uint16_t len) */
/* { */
/*    // Check the message length */
/*    if(len < 8){ */
/*        //ToDo: Log error */
/*        return(false); */
/*    } */
/*  */
/*    if (g3_cl_coordGetConfirm_cb) */
/*    { */
/*        { */
/*            uint8_t status, attribute_length; */
/*            uint32_t attribute_id; */
/*            uint16_t attribute_index; */
/*  */
/*            status = (*ptrMsg++); */
/*            attribute_id = (*ptrMsg++); */
/*            attribute_id = (*ptrMsg++) + (attribute_id << 8); */
/*            attribute_id = (*ptrMsg++) + (attribute_id << 8); */
/*            attribute_id = (*ptrMsg++) + (attribute_id << 8); */
/*            attribute_index = (*ptrMsg++); */
/*            attribute_index = (*ptrMsg++) + (attribute_index << 8); */
/*            attribute_length = (*ptrMsg++); */
/*            // If the length matches, the NSDU is copied */
/*            if(len == attribute_length + 8){ */
/*                uint8_t *value = (uint8_t*)malloc(attribute_length*sizeof(uint8_t)); */
/*                memcpy(value, ptrMsg, attribute_length); */
/*                // Trigger the callback */
/*                g3_cl_coordGetConfirm_cb(status, attribute_id, attribute_index, attribute_length, value); */
/*                free(value); */
/*            } else { */
/*                //ToDo: Log error */
/*                return(false); */
/*            } */
/*        } */
/*    } */
/*    return(true); */
/* } */
/*  */
/* / ** */
/* * @brief _cl_null_coordGetConfirm_cb */
/* * @param ptrMsg: Received message */
/* * @param len: Received message length */
/* * @return */
/* * / */
/* uint8_t _cl_null_coordSetConfirm_cb(uint8_t* ptrMsg, uint16_t len) */
/* { */
/*    // Check the message length */
/*    if(len < 7){ */
/*        //ToDo: Log error */
/*        return(false); */
/*    } */
/*  */
/*    if (g3_cl_coordSetConfirm_cb) */
/*    { */
/*        { */
/*            uint8_t status; */
/*            uint32_t attribute_id; */
/*            uint16_t attribute_index; */
/*  */
/*            status = (*ptrMsg++); */
/*            attribute_id = (*ptrMsg++); */
/*            attribute_id = (*ptrMsg++) + (attribute_id << 8); */
/*            attribute_id = (*ptrMsg++) + (attribute_id << 8); */
/*            attribute_id = (*ptrMsg++) + (attribute_id << 8); */
/*            attribute_index = (*ptrMsg++); */
/*            attribute_index = (*ptrMsg++) + (attribute_index << 8); */
/*  */
/*            // Trigger the callback */
/*            g3_cl_coordSetConfirm_cb(status, attribute_id, attribute_index); */
/*        } */
/*    } */
/*    return(true); */
/* } */

/**
 * @brief _cl_null_coordLeaveIndication_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_coordLeaveIndication_cb(uint8_t *ptrMsg, uint16_t len)
{
	/* Check the message length */
	if (len < 2) {
		/* ToDo: Log error */
		return(false);
	}

	if (g3_app_leave_ind_cb) {
		uint16_t u16SrcAddr;
		bool bSecurityEnabled;
		uint8_t u8LinkQualityIndicator;
		uint8_t *pNsdu;
		uint16_t u16NsduLength;

		u16SrcAddr = (*ptrMsg++);
		u16SrcAddr = (*ptrMsg++) + (u16SrcAddr << 8);

		bSecurityEnabled = (*ptrMsg++) ? true : false;

		u8LinkQualityIndicator = (*ptrMsg++);

		u16NsduLength = (*ptrMsg++);

		/* If the length matches, the NSDU is copied */
		if (len == u16NsduLength + 5) {
			pNsdu = (uint8_t *)malloc(u16NsduLength * sizeof(uint8_t));
			memcpy(pNsdu, ptrMsg, u16NsduLength);
			/* Trigger the callback */
			g3_app_leave_ind_cb(u16SrcAddr, bSecurityEnabled, u8LinkQualityIndicator, pNsdu, u16NsduLength);
			free(pNsdu);
		} else {
			/* ToDo: Log error */
			return(false);
		}
	}

	return(true);
}

/**
 * @brief _cl_null_coordLeaveIndication_cb
 * @param ptrMsg: Received message
 * @param len: Received message length
 * @return
 */
uint8_t _cl_null_coordJoinIndication_cb(uint8_t *ptrMsg, uint16_t len)
{
	/* Check the message length */
	if (len < 2) {
		/* ToDo: Log error */
		return(false);
	}

	if (g3_app_join_ind_cb) {
		uint8_t uc_extended_address[8];
		uint16_t us_short_address;

		memcpy(&uc_extended_address[0], ptrMsg, 8);
		ptrMsg += 8;

		us_short_address = (*ptrMsg++);
		us_short_address = (*ptrMsg++) + (us_short_address << 8);

		/* Trigger the callback */
		g3_app_join_ind_cb(uc_extended_address, us_short_address);
	}

	return(true);
}

void bs_lbp_leave_ind_set_cb(pf_app_leave_ind_cb_t pf_handler)
{
	g3_app_leave_ind_cb = pf_handler;
}

void bs_lbp_join_ind_set_cb(pf_app_join_ind_cb_t pf_handler)
{
	g3_app_join_ind_cb = pf_handler;
}

/* TBootstrapAdpNotifications *bs_get_not_handlers(void){ */
/*  */
/* } */

/**
 * @brief g3_COORD_receivedCmd
 * Copies in the Reception buffer the data received in the USI
 * @param ptrMsg: Message
 * @param len: Message length
 * @return TRUE if is possible to extract from the usi buffer
 */
Uint8 g3_COORD_receivedCmd(Uint8 *ptrMsg, Uint16 len)
{
	/*	return putMsgQueue(&qMac, ptrMsg, len); */
	uint8_t uc_cmd;

	uc_cmd = (*ptrMsg++);
	len--;

	switch (uc_cmd) {
	/*    case G3_SERIAL_MSG_COORD_SET_CONFIRM: */
	/*        return _cl_null_coordSetConfirm_cb(ptrMsg,len); */
	/*    case G3_SERIAL_MSG_COORD_GET_CONFIRM: */
	/*        return _cl_null_coordGetConfirm_cb(ptrMsg,len); */
	case G3_SERIAL_MSG_COORD_LEAVE_INDICATION:
		return _cl_null_coordLeaveIndication_cb(ptrMsg, len);

	case G3_SERIAL_MSG_COORD_JOIN_INDICATION:
		return _cl_null_coordJoinIndication_cb(ptrMsg, len);

	default:
		return g3_ADP_receivedCmd(ptrMsg, len);
	}

	/*    uint16_t bs_lbp_get_lbds_counter(void); */
	/*    uint16_t bs_lbp_get_lbds_address(uint16_t i); */
	/*    bool bs_lbp_get_lbds_ex_address(uint16_t us_short_address, uint8_t *puc_extended_address); */
	/*    void bs_lbp_get_param(uint32_t ul_attribute_id, uint16_t us_attribute_idx, struct t_bs_lbp_get_param_confirm *p_get_confirm); */
	/*    void bs_lbp_set_param(uint32_t ul_attribute_id, uint16_t us_attribute_idx, uint8_t uc_attribute_len, const uint8_t *puc_attribute_value, */
	/*              struct t_bs_lbp_set_param_confirm *p_set_confirm); */
	/*    bool bs_get_ext_addr_by_short(uint16_t us_short_address, uint8_t *puc_extended_address); */
	/*    bool bs_get_short_addr_by_ext(uint8_t *puc_extended_address, uint16_t *pus_short_address); */
	return(TRUE);
}
