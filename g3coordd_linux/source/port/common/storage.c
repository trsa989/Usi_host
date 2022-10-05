/**
 * \file
 *
 * \brief Common Utils
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
#include <string.h>
#include <stdio.h>
#include <AdpApi.h>
#include "Logger.h"
#include "storage.h"

static struct TPersistentInfo persistentInfo;

static uint16_t Crc16(const uint8_t *pu8Data, uint32_t u32Length, uint16_t u16Poly, uint16_t u16Crc);
static void platform_init_storage(void);
static bool platform_write_storage(uint32_t u32Length, const void *pData);
static bool platform_read_storage(uint32_t u32Length, void *pData);

static void _get_persistent_data(struct TPersistentData *data);
static void _set_persistent_data(struct TPersistentData *data);

/**
 * \brief Initialize Storage Capabilities
 * It needs to be implemented if TPersistentInfo is not saved on G3 ADP/MAC Modem
 * Please see ENABLE_PIB_RESTORE on Embedded Project
 * \remarks
 */
void platform_init_storage(void)
{
	return;
}

/**
 * \brief Write Values to be storaged from G3 Stack
 * It needs to be implemented if TPersistentInfo is not saved on G3 ADP/MAC Modem
 * Please see ENABLE_PIB_RESTORE on Embedded Project
 */
bool platform_write_storage(uint32_t u32Length, const void *pData)
{
	bool bRet = false;
	return bRet;
}

/**
 * \brief Read Values to be loaded on G3 Stack
 * It needs to be implemented if TPersistentInfo is not saved on G3 ADP/MAC Modem
 * Please see ENABLE_PIB_RESTORE on Embedded Project
 */
bool platform_read_storage(uint32_t u32Length, void *pData)
{
	bool bRet = false;
	return bRet;
}

/**
 * \brief Stores persistent data.
 *
 * \remarks It needs to be implemented if TPersistentInfo is not saved on G3 ADP/MAC Modem
 */
void store_persistent_info(void)
{
	LOG_INFO(Log("PDD_CB: Persistent data stored.\r\n"));

	_get_persistent_data(&persistentInfo.m_data);

	/* Persistent info Header */
	persistentInfo.m_u16Version = STORAGE_VERSION;
	persistentInfo.m_u16Crc16 = Crc16((const uint8_t *)(&persistentInfo.m_data), sizeof(struct TPersistentData), 0x1021, 0xFFFF);

	/* Write internal data to the persistent storage */
	platform_write_storage(sizeof(struct TPersistentInfo), &persistentInfo);
}

/**
 * \brief Loads persistent data
 *
 * \remarks It needs to be implemented if TPersistentInfo is not saved on G3 ADP/MAC Modem
 */
void load_persistent_info(void)
{
	bool b_upd_info;
	LOG_INFO(Log("Loading persistent data...\r\n"));

	b_upd_info = true;
	platform_init_storage();

	/* Read the persistent storage */
	if (platform_read_storage(sizeof(struct TPersistentInfo), &persistentInfo)) {
		/* Check the CRC */
		uint16_t u16Crc16 = Crc16((const uint8_t *)(&persistentInfo.m_data), sizeof(struct TPersistentData), 0x1021, 0xFFFF);

		if (persistentInfo.m_u16Crc16 != u16Crc16) {
			LOG_ERR(Log("load_persistent_info() CRC error. Read: %u, Calc: %u\r\n", persistentInfo.m_u16Crc16, u16Crc16));
			b_upd_info = false;
		} else if (persistentInfo.m_u16Version > STORAGE_VERSION) {
			LOG_ERR(Log("load_persistent_info() storage version error.\r\n"));
			b_upd_info = false;
		}
	} else {
		LOG_ERR(Log("load_persistent_info() unable to read storage.\r\n"));
		b_upd_info = false;
	}

	/* Increment startup counter */
	persistentInfo.m_u32StartupCounter++;

	/* Set Values to G3 Stack */
	_set_persistent_data(&persistentInfo.m_data);
	LOG_INFO(Log("Persistent data loaded. Startup Counter: %d\r\n", persistentInfo.m_u32StartupCounter));
}

/**
 * \brief Calculates CRC-16
 *
 */
static uint16_t Crc16(const uint8_t *pu8Data, uint32_t u32Length, uint16_t u16Poly, uint16_t u16Crc)
{
	uint8_t u8Index;
	while (u32Length--) {
		u16Crc ^= (*pu8Data++) << 8;
		for (u8Index = 0; u8Index < 8; u8Index++) {
			u16Crc = (u16Crc << 1) ^ ((u16Crc & 0x8000U) ? u16Poly : 0);
		}
	}
	return u16Crc;
}

/**
 * \brief Gets G3 stack info to be stored
 *
 * \param info     Pointer to the persistent info
 */
static void _get_persistent_data(struct TPersistentData *data)
{
	struct TAdpMacGetConfirm macGetConfirm;
	struct TAdpGetConfirm adpGetConfirm;

	/* Read internal data from the stack */
	AdpMacGetRequestSync(MAC_WRP_PIB_FRAME_COUNTER, 0, &macGetConfirm);
	memcpy(&data->m_u32FrameCounter, macGetConfirm.m_au8AttributeValue, macGetConfirm.m_u8AttributeLength);
	AdpGetRequestSync(ADP_IB_MANUF_DISCOVER_SEQUENCE_NUMBER, 0, &adpGetConfirm);
	memcpy(&data->m_u16DiscoverSeqNumber, adpGetConfirm.m_au8AttributeValue, adpGetConfirm.m_u8AttributeLength);
	AdpGetRequestSync(ADP_IB_MANUF_BROADCAST_SEQUENCE_NUMBER, 0, &adpGetConfirm);
	memcpy(&data->m_u8BroadcastSeqNumber, adpGetConfirm.m_au8AttributeValue, adpGetConfirm.m_u8AttributeLength);
}

/**
 * \brief Sets stored info in the G3 stack
 *
 * \param info     Pointer to the persistent info
 */
static void _set_persistent_data(struct TPersistentData *data)
{
	struct TAdpMacSetConfirm macSetConfirm;
	struct TAdpSetConfirm adpSetConfirm;

	/* Write internal data to the stack */
	AdpMacSetRequestSync(MAC_WRP_PIB_FRAME_COUNTER, 0, sizeof(data->m_u32FrameCounter), (const uint8_t *)(&data->m_u32FrameCounter), &macSetConfirm);
	AdpSetRequestSync(ADP_IB_MANUF_DISCOVER_SEQUENCE_NUMBER, 0, sizeof(data->m_u16DiscoverSeqNumber),
			(const uint8_t *)(&data->m_u16DiscoverSeqNumber), &adpSetConfirm);
	AdpSetRequestSync(ADP_IB_MANUF_BROADCAST_SEQUENCE_NUMBER, 0, sizeof(data->m_u8BroadcastSeqNumber),
			(const uint8_t *)(&data->m_u8BroadcastSeqNumber), &adpSetConfirm);
}
