/**
 * \file
 *
 * \brief G3 Coordinator Module
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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <AdpApiTypes.h>
#include <ProtoEapPsk.h>
#include <crypto/eax.h>
#include <crypto/aes_wrapper.h>
#include <crypto/cipher_wrapper.h>
#include <mac_wrapper.h>
/* #include <AdpMacInterface.h> */

#define LOG_LEVEL LOG_LEVEL_ADP
#include <Logger.h>

#define UNUSED(v)          (void)(v)

#define member_size(type, member) sizeof(((type *)0)->member)

/**********************************************************************************************************************/

/** EAP_PSK_Initialize computes KDK and AK from PSK secret
 **********************************************************************************************************************/
void EAP_PSK_Initialize(const struct TEapPskKey *pKey, struct TEapPskContext *pPskContext)
{
	aes_wrapper_context aes_eap_ctx;

	uint8_t au8Block[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	uint8_t au8Res[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	memset(pPskContext, 0, sizeof(struct TEapPskContext));

	/* Initialize the AES */
	aes_wrapper_aes_init(&aes_eap_ctx);
	/* Trigger the AES */
	aes_wrapper_aes_setkey_enc(&aes_eap_ctx, pKey->m_au8Value, 128);
	aes_wrapper_aes_encrypt(&aes_eap_ctx, au8Block, au8Res);

	/* xor with c1 = "1" */
	au8Res[15] ^= 0x01;

	/* generate AK */
	aes_wrapper_aes_encrypt(&aes_eap_ctx, au8Res, pPskContext->m_Ak.m_au8Value);

	/* generate AK */
	/* xor with c1 = "2" */
	au8Res[15] ^= 0x03; /* 3 instead of 2 because it has been already xor'ed with 1 and we want to get back the initial value */

	/* generate KDK */
	aes_wrapper_aes_encrypt(&aes_eap_ctx, au8Res, pPskContext->m_Kdk.m_au8Value);

	/* Free the AES */
	aes_wrapper_aes_free(&aes_eap_ctx);
}

/**********************************************************************************************************************/

/** The EAP_PSK_InitializeTEK primitive is used to initialize the TEK key
 **********************************************************************************************************************/
void EAP_PSK_InitializeTEK(const struct TEapPskRand *pRandP, struct TEapPskContext *pPskContext)
{
	aes_wrapper_context aes_eap_ctx;
	uint8_t au8Res[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	/* Initialize the AES */
	aes_wrapper_aes_init(&aes_eap_ctx);
	/* Trigger the AES */
	aes_wrapper_aes_setkey_enc(&aes_eap_ctx, pPskContext->m_Kdk.m_au8Value, 128);

	aes_wrapper_aes_encrypt(&aes_eap_ctx, pRandP->m_au8Value, au8Res);

	/* xor with c1 = "1" */
	au8Res[15] ^= 0x01;

	/* generate TEK */
	aes_wrapper_aes_encrypt(&aes_eap_ctx, au8Res, pPskContext->m_Tek.m_au8Value);

	/* Free the AES */
	aes_wrapper_aes_free(&aes_eap_ctx);
}

/**********************************************************************************************************************/

/** The EAP_PSK_Decode_Message primitive is used to decode the header of the EAP-PSK messages
 **********************************************************************************************************************/
bool EAP_PSK_Decode_Message(uint16_t u16MessageLength, uint8_t *pMessage, uint8_t *pu8Code, uint8_t *pu8Identifier,
		uint8_t *pu8TSubfield, uint16_t *pu16EAPDataLength, uint8_t **pEAPData)
{
	bool bRet = false;

	if (u16MessageLength >= 4) {
		uint16_t u16EapMessageLength = 0;

		*pu8Code = pMessage[0];
		*pu8Identifier = pMessage[1];
		u16EapMessageLength = ((pMessage[2] << 8) | pMessage[3]);

		/* A message with the Length field set to a value larger than the number of received octets MUST be silently discarded. */
		bRet = (u16EapMessageLength <= u16MessageLength);

		if (bRet && (u16EapMessageLength >= 6)) {
			*pu8TSubfield = pMessage[5];
			*pEAPData = &pMessage[6];
			*pu16EAPDataLength = u16EapMessageLength - 6; /* 4 for the size of the header + 2 eaptype and T field */

			bRet = (pMessage[4] == EAP_PSK_IANA_TYPE);
		}
	}

	return bRet;
}

/**********************************************************************************************************************/

/** The EAP_PSK_Decode_Message1 primitive is used to decode the first EAP-PSK message (T = 0)
 **********************************************************************************************************************/
bool EAP_PSK_Decode_Message1(uint16_t u16MessageLength, uint8_t *pMessage, struct TEapPskRand *pRandS,
		struct TEapPskNetworkAccessIdentifier *pIdS)
{
	bool bRet = false;

	/* check the length of the message */
	if (u16MessageLength >= sizeof(pRandS->m_au8Value)) {
		memcpy(pRandS->m_au8Value, pMessage, sizeof(pRandS->m_au8Value));

		pIdS->m_u8Length = u16MessageLength - sizeof(pRandS->m_au8Value);
		memcpy(pIdS->m_au8Value, &pMessage[sizeof(pRandS->m_au8Value)], pIdS->m_u8Length);
		bRet = true;
	}

	return bRet;
}

/**********************************************************************************************************************/

/** The EAP_PSK_Encode_Message2 primitive is used to encode the second EAP-PSK message (type 1)
 **********************************************************************************************************************/
uint16_t EAP_PSK_Encode_Message2(const struct TEapPskContext *pPskContext, uint8_t u8Identifier,
		const struct TEapPskRand *pRandS, const struct TEapPskRand *pRandP, const struct TEapPskNetworkAccessIdentifier *pIdS,
		const struct TEapPskNetworkAccessIdentifier *pIdP, uint16_t u16MemoryBufferLength, uint8_t *pMemoryBuffer)
{
	uint16_t u16Ret = 0;

	/* check the size of the buffer */
	if (u16MemoryBufferLength >= 62) {
		int16_t ret;
		/* compute first MacP = CMAC-AES-128(AK, IdP||IdS||RandS||RandP) */
		uint8_t au8MacP[16];
		uint8_t au8Seed[2 * member_size(struct TEapPskNetworkAccessIdentifier, m_au8Value)
		+ 2 * member_size(struct TEapPskRand, m_au8Value)];
		uint16_t u16SeedUsedSize = 0;

		cipher_wrapper_cipher_context_t m_ctx;
		const cipher_wrapper_cipher_info_t *cipher_info;
		/* Initialize CMAC */
		cipher_info = cipher_wrapper_cipher_info_from_type(CIPHER_WRAPPER_CIPHER_AES_128_ECB);
		if (cipher_info == NULL) {
			LOG_DBG(Log("\ncipher_wrapper_cipher_info_from_type failed"));
		}

		ret = cipher_wrapper_cipher_setup( &m_ctx, cipher_info );
		LOG_DBG(Log("\n cipher_wrapper_cipher_setup returned %d %d", ret, m_ctx.cipher_info->type));

		memcpy(au8Seed, pIdP->m_au8Value, pIdP->m_u8Length);
		u16SeedUsedSize += pIdP->m_u8Length;

		memcpy(&au8Seed[u16SeedUsedSize], pIdS->m_au8Value, pIdS->m_u8Length);
		u16SeedUsedSize += pIdS->m_u8Length;

		memcpy(&au8Seed[u16SeedUsedSize], pRandS->m_au8Value, sizeof(pRandS->m_au8Value));
		u16SeedUsedSize += sizeof(pRandS->m_au8Value);

		memcpy(&au8Seed[u16SeedUsedSize], pRandP->m_au8Value, sizeof(pRandP->m_au8Value));
		u16SeedUsedSize += sizeof(pRandP->m_au8Value);

		ret = cipher_wrapper_cipher_cmac_starts(&m_ctx, pPskContext->m_Ak.m_au8Value, 8 * sizeof(pPskContext->m_Ak.m_au8Value));
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_starts returned %d", ret));

		ret = cipher_wrapper_cipher_cmac_update(&m_ctx, au8Seed, u16SeedUsedSize);
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_update returned %d", ret));

		ret = cipher_wrapper_cipher_cmac_finish(&m_ctx, au8MacP);
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_finish returned %d", ret));

		ret = cipher_wrapper_cipher_cmac_reset(&m_ctx);
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_reset returned %d", ret));

		cipher_wrapper_cipher_free( &m_ctx);

		LOG_DBG(LogBuffer(pPskContext->m_Ak.m_au8Value, sizeof(pPskContext->m_Ak.m_au8Value), "Seed "));
		LOG_DBG(LogBuffer(au8Seed, u16SeedUsedSize, "Seed "));
		LOG_DBG(LogBuffer(au8MacP, sizeof(au8MacP), "MacP "));

		/* encode the EAP header; length field will be set at the end of the block */
		pMemoryBuffer[0] = EAP_RESPONSE;
		pMemoryBuffer[1] = u8Identifier;
		pMemoryBuffer[4] = EAP_PSK_IANA_TYPE;
		pMemoryBuffer[5] = EAP_PSK_T1;
		u16Ret = 6;

		/* EAP message: add PSK fields */
		memcpy(&pMemoryBuffer[u16Ret], pRandS->m_au8Value, sizeof(pRandS->m_au8Value));
		u16Ret += sizeof(pRandS->m_au8Value);
		memcpy(&pMemoryBuffer[u16Ret], pRandP->m_au8Value, sizeof(pRandP->m_au8Value));
		u16Ret += sizeof(pRandP->m_au8Value);
		memcpy(&pMemoryBuffer[u16Ret], au8MacP, sizeof(au8MacP));
		u16Ret += sizeof(au8MacP);
		memcpy(&pMemoryBuffer[u16Ret], pIdP->m_au8Value, pIdP->m_u8Length);
		u16Ret += pIdP->m_u8Length;

		/* now update the EAP header length field */
		pMemoryBuffer[2] = (uint8_t)((u16Ret >> 8) & 0x00FF);
		pMemoryBuffer[3] = (uint8_t)(u16Ret & 0x00FF);

		UNUSED(ret);
	}

	return u16Ret;
}

/**********************************************************************************************************************/

/** The EAP_PSK_Decode_Message3 primitive is used to decode the third EAP-PSK message (type 2)
 **********************************************************************************************************************/
bool EAP_PSK_Decode_Message3(uint16_t u16MessageLength, uint8_t *pMessage, const struct TEapPskContext *pPskContext,
		uint16_t u16HeaderLength, uint8_t *pHeader, struct TEapPskRand *pRandS, uint32_t *pu32Nonce,
		uint8_t *pu8PChannelResult, uint16_t *pu16PChannelDataLength, uint8_t **pPChannelData)
{
	bool bRet = false;

	if (u16MessageLength >= 59) {
		uint8_t au8MacS[16];
		eax_ctx eaxCtx[1];
		int16_t ret;

		memcpy(pRandS->m_au8Value, pMessage, sizeof(pRandS->m_au8Value));

		/* verify MacS: MAC_S = CMAC-AES-128(AK, IdS||RandP) */
		uint8_t au8Seed[member_size(struct TEapPskNetworkAccessIdentifier, m_au8Value)
		+ member_size(struct TEapPskRand, m_au8Value)];
		uint16_t u16SeedUsedSize = 0;

		cipher_wrapper_cipher_context_t m_ctx;
		const cipher_wrapper_cipher_info_t *cipher_info;
		/* Initialize CMAC */
		cipher_info = cipher_wrapper_cipher_info_from_type(CIPHER_WRAPPER_CIPHER_AES_128_ECB);
		if (cipher_info == NULL) {
			LOG_DBG(Log("\ncipher_wrapper_cipher_info_from_type failed"));
		}

		ret = cipher_wrapper_cipher_setup( &m_ctx, cipher_info );
		LOG_DBG(Log("\n cipher_wrapper_cipher_setup returned %d %d", ret, m_ctx.cipher_info->type));

		memcpy(au8Seed, pPskContext->m_IdS.m_au8Value, pPskContext->m_IdS.m_u8Length);
		u16SeedUsedSize += pPskContext->m_IdS.m_u8Length;

		memcpy(&au8Seed[u16SeedUsedSize], pPskContext->m_RandP.m_au8Value, sizeof(pPskContext->m_RandP.m_au8Value));
		u16SeedUsedSize += sizeof(pPskContext->m_RandP.m_au8Value);

		ret = cipher_wrapper_cipher_cmac_starts(&m_ctx, pPskContext->m_Ak.m_au8Value, 8 * sizeof(pPskContext->m_Ak.m_au8Value));
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_starts returned %d", ret));

		ret = cipher_wrapper_cipher_cmac_update(&m_ctx, au8Seed, u16SeedUsedSize);
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_update returned %d", ret));

		ret = cipher_wrapper_cipher_cmac_finish(&m_ctx, au8MacS);
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_finish returned %d", ret));

		ret = cipher_wrapper_cipher_cmac_reset(&m_ctx);
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_reset returned %d", ret));

		cipher_wrapper_cipher_free( &m_ctx);

		if (memcmp(au8MacS, &pMessage[sizeof(pRandS->m_au8Value)], sizeof(au8MacS)) == 0) {
			/* decrypt P-CHANNEL */
			/* P-CHANNEL uses the TEK key */
			if (RETURN_GOOD
					== eax_init_and_key(pPskContext->m_Tek.m_au8Value, sizeof(pPskContext->m_Tek.m_au8Value), eaxCtx)) {
				uint8_t au8Nonce[16];
				uint8_t *pNonce = &pMessage[32];
				uint8_t *pTag = &pMessage[36];
				uint8_t *pProtectedData = &pMessage[52];
				uint16_t u16ProtectedDataLength = u16MessageLength - 52;

				/* prepare 16 bytes nonce */
				/* nonce should be big endian */
				memset(au8Nonce, 0, sizeof(au8Nonce));
				au8Nonce[12] = pNonce[0];
				au8Nonce[13] = pNonce[1];
				au8Nonce[14] = pNonce[2];
				au8Nonce[15] = pNonce[3];

				/* The protected data is the 22 bytes header of the EAP message. */
				/* The G3 specifies a slightly modified EAP header but in the same time */
				/* the specification requires to compute the authentication tag over the */
				/* on the original EAP header */
				/* So we change the header to make it "EAP compliant", we compute the */
				/* auth tag and then we change back the header */

				/* right shift Code field with 2 bits as indicated in the EAP specification */
				pHeader[0] >>= 2;

				LOG_DBG(LogBuffer(pPskContext->m_Tek.m_au8Value, sizeof(pPskContext->m_Tek.m_au8Value), "TEK: "));
				LOG_DBG(LogBuffer(au8Nonce, 16, "Nonce/IV: "));
				LOG_DBG(LogBuffer(pHeader, u16HeaderLength, "Header: "));
				LOG_DBG(LogBuffer(pProtectedData, u16ProtectedDataLength, "Data-encr: "));
				LOG_DBG(LogBuffer(pTag, 16, "Tag: "));

				/* RETURN_GOOD is returned if the input tag matches that for the decrypted message */
				if (RETURN_GOOD == eax_decrypt_message(au8Nonce, /* the initialization vector    */
						16, /* and its length in bytes      */
						pHeader, /* the header buffer            */
						u16HeaderLength, /* and its length in bytes      */
						pProtectedData, /* the message buffer           */
						u16ProtectedDataLength, /* and its length in bytes      */
						pTag, /* the buffer for the tag       */
						16, /* and its length in bytes      */
						eaxCtx) /* the mode context             */
						) {
					/* retrieve protected parameters */
					/* uint8_t u8ExtField = ((pProtectedData[0] & 0x20) >> 5); */
					*pu8PChannelResult = ((pProtectedData[0] & 0xC0) >> 6);

					*pu16PChannelDataLength = u16ProtectedDataLength - 1;
					*pPChannelData = &pProtectedData[1];

					((uint8_t *)pu32Nonce)[0] = pNonce[3];
					((uint8_t *)pu32Nonce)[1] = pNonce[2];
					((uint8_t *)pu32Nonce)[2] = pNonce[1];
					((uint8_t *)pu32Nonce)[3] = pNonce[0];

					bRet = true;
				} else {
					LOG_DBG(Log("Decode FAILED"));
				}

				LOG_DBG(LogBuffer(pProtectedData, u16ProtectedDataLength, "Data-plain: "));

				/* Fix EAP header: left shift Code field with 2 bits as indicated in the G3 specification */
				pHeader[0] <<= 2;

				eax_end(eaxCtx);
			}

			UNUSED(ret);
		} else {
			/* cannot verify MacS */
			LOG_ERR(Log("Cannot verify MAC_S"));
		}
	}

	return bRet;
}

/**********************************************************************************************************************/

/** The EAP_PSK_Encode_Message4 primitive is used to encode the second EAP-PSK message (type 3)
 **********************************************************************************************************************/
uint16_t EAP_PSK_Encode_Message4(const struct TEapPskContext *pPskContext, uint8_t u8Identifier,
		const struct TEapPskRand *pRandS, uint32_t u32Nonce, uint8_t u8PChannelResult, uint16_t u16PChannelDataLength,
		uint8_t *pPChannelData, uint16_t u16MemoryBufferLength, uint8_t *pMemoryBuffer)
{
	uint16_t u16Ret = 0;

	/* check the size of the buffer */
	if (u16MemoryBufferLength >= 43 + u16PChannelDataLength) {
		uint8_t *pTag = 0L;
		eax_ctx eaxCtx[1];
		uint8_t au8Nonce[16];
		uint8_t *pProtectedData = 0L;
		uint16_t u16ProtectedDataLength = 0;

		/* encode the EAP header; length field will be set at the end of the block */
		pMemoryBuffer[0] = EAP_RESPONSE;
		pMemoryBuffer[1] = u8Identifier;
		pMemoryBuffer[4] = EAP_PSK_IANA_TYPE;
		pMemoryBuffer[5] = EAP_PSK_T3;
		u16Ret = 6;

		/* EAP message: add PSK fields */
		memcpy(&pMemoryBuffer[u16Ret], pRandS->m_au8Value, sizeof(pRandS->m_au8Value));
		u16Ret += sizeof(pRandS->m_au8Value);

		/* nonce should be big endian */
		memset(au8Nonce, 0, sizeof(au8Nonce));

		au8Nonce[12] = pMemoryBuffer[u16Ret] = (uint8_t)((u32Nonce >> 24) & 0xFF);
		au8Nonce[13] = pMemoryBuffer[u16Ret + 1] = (uint8_t)((u32Nonce >> 16) & 0xFF);
		au8Nonce[14] = pMemoryBuffer[u16Ret + 2] = (uint8_t)((u32Nonce >> 8) & 0xFF);
		au8Nonce[15] = pMemoryBuffer[u16Ret + 3] = (uint8_t)(u32Nonce & 0xFF);
		u16Ret += 4;

		/* Tag will be set after data protection */
		pTag = &pMemoryBuffer[u16Ret];
		u16Ret += 16;

		/* protected data */
		pProtectedData = &pMemoryBuffer[u16Ret];

		if (u16PChannelDataLength > 0) {
			/* result / extension = 1 */
			pProtectedData[u16ProtectedDataLength] = (u8PChannelResult << 6) | 0x20;
			u16ProtectedDataLength++;

			memcpy(&pProtectedData[u16ProtectedDataLength], pPChannelData, u16PChannelDataLength);
			u16ProtectedDataLength += u16PChannelDataLength;
		} else {
			/* result / extension = 0 */
			pProtectedData[u16ProtectedDataLength] = (u8PChannelResult << 6);
			u16ProtectedDataLength++;
		}

		u16Ret += u16ProtectedDataLength;

		/* protect data in P-Channel (actually it's just the last byte) */
		/* P-CHANNEL uses the TEK key */
		if (RETURN_GOOD == eax_init_and_key(pPskContext->m_Tek.m_au8Value, sizeof(pPskContext->m_Tek.m_au8Value), eaxCtx)) {
			/* update the EAP header length field */
			pMemoryBuffer[2] = (uint8_t)((u16Ret >> 8) & 0x00FF);
			pMemoryBuffer[3] = (uint8_t)(u16Ret & 0x00FF);

			/* The protected data is the 22 bytes header of the EAP message. */
			/* The G3 specifies a slightly modified EAP header but in the same time */
			/* the specification requires to compute the authentication tag over the */
			/* the original EAP header */
			/* So we change the header to make it "EAP compliant", we compute the */
			/* auth tag and then we change back the header */

			/* right shift Code field with 2 bits as indicated in the EAP specification */
			pMemoryBuffer[0] >>= 2;

			LOG_DBG(LogBuffer(pPskContext->m_Tek.m_au8Value, sizeof(pPskContext->m_Tek.m_au8Value), "TEK: "));
			LOG_DBG(LogBuffer(au8Nonce, 16, "Nonce/IV: "));
			LOG_DBG(LogBuffer(pMemoryBuffer, 22, "Header: "));
			LOG_DBG(LogBuffer(pProtectedData, u16ProtectedDataLength, "Data-plain: "));

			if (RETURN_GOOD != eax_encrypt_message(au8Nonce, /* the initialization vector    */
					16, /* and its length in bytes      */
					pMemoryBuffer, /* the header buffer            */
					22, /* and its length in bytes      */
					pProtectedData, /* the message buffer           */
					u16ProtectedDataLength, /* and its length in bytes      */
					pTag, /* the buffer for the tag       */
					16, /* and its length in bytes      */
					eaxCtx) /* the mode context             */
					) {
				u16Ret = 0;
			}

			LOG_DBG(LogBuffer(pProtectedData, u16ProtectedDataLength, "Data-encr: "));
			LOG_DBG(LogBuffer(pTag, 16, "Tag: "));

			/* Fix EAP header: left shift Code field with 2 bits as indicated in the G3 specification */
			pMemoryBuffer[0] <<= 2;

			eax_end(eaxCtx);
		} else {
			u16Ret = 0;
		}
	}

	return u16Ret;
}

/**********************************************************************************************************************/

/** The EAP_PSK_Encode_Message1 primitive is used to encode the first EAP-PSK message (type 0)
 * This message is sent from the server to device
 ***********************************************************************************************************************
 *
 * @param u8Identifier Message identifier retrieved from the Request
 *
 * @param au8RandS RandS parameter built by the server
 *
 * @param au8IdS IdS parameter (the server identity)
 *
 * @param u16MemoryBufferLength size of the buffer which will be used for data encoding
 *
 * @param pMemoryBuffer OUT parameter; upon successful return contains the encoded message; this buffer should be previously
 *                      allocated
 *
 * @return encoded length or 0 if encoding failed
 *
 **********************************************************************************************************************/
uint16_t EAP_PSK_Encode_Message1(
		uint8_t u8Identifier,
		const struct TEapPskRand *pRandS,
		const struct TEapPskNetworkAccessIdentifierS *pIdS,
		uint16_t u16MemoryBufferLength,
		uint8_t *pMemoryBuffer
		)
{
	uint16_t u16Ret = 0;

	/* check the size of the buffer */
	if (u16MemoryBufferLength >= 30) {
		/* encode the EAP header; length field will be set at the end of the block */
		pMemoryBuffer[0] = EAP_REQUEST;
		pMemoryBuffer[1] = u8Identifier;
		pMemoryBuffer[4] = EAP_PSK_IANA_TYPE;
		pMemoryBuffer[5] = EAP_PSK_T0;
		u16Ret = 6;

		/* EAP message: add PSK fields */
		memcpy(&pMemoryBuffer[u16Ret], pRandS->m_au8Value, sizeof(pRandS->m_au8Value));
		u16Ret += sizeof(pRandS->m_au8Value);
		memcpy(&pMemoryBuffer[u16Ret], pIdS->m_au8Value, pIdS->uc_size);
		u16Ret += pIdS->uc_size;

		/* now update the EAP header length field */
		pMemoryBuffer[2] = (uint8_t)((u16Ret >> 8) & 0x00FF);
		pMemoryBuffer[3] = (uint8_t)(u16Ret & 0x00FF);
	}

	return u16Ret;
}

/**********************************************************************************************************************/

/** The EAP_PSK_Decode_Message2 primitive is used to decode the second EAP-PSK message (type 1) and also to check
 * the MacP parameter
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
bool EAP_PSK_Decode_Message2(
		uint8_t u8BandId,
		uint16_t u16MessageLength,
		uint8_t *pMessage,
		const struct TEapPskContext *pPskContext,
		const struct TEapPskNetworkAccessIdentifierS *pIdS,
		struct TEapPskRand *pRandS,
		struct TEapPskRand *pRandP
		)
{
	bool bRet = false;
	uint8_t uc_min_msg_length;
	enum EMacWrpStatus x_status;
	struct TMacWrpPibValue x_pib_value;

	/* Read the band from MIB */
	/* x_status = (enum EMacWrpStatus)AdpMac_GetMibSync((uint32_t)MAC_WRP_PIB_MANUF_BAND_INFORMATION, 0, &x_pib_value); */

	/* if (x_status != MAC_WRP_STATUS_SUCCESS) { */
	/*  x_pib_value.m_au8Value[0] = MAC_WRP_BAND_CENELEC_A; */
	/* } */

	/* if (x_pib_value.m_au8Value[0] == MAC_WRP_BAND_ARIB) { */
	/*  uc_min_msg_length = 49; */
	/* } */
	/* else { */
	uc_min_msg_length = 56;
	/* } */
	if (u16MessageLength >= uc_min_msg_length) { /* EAP header already removed */
		uint8_t au8MacP[16];
		uint8_t au8ExpectedMacP[16];
		struct TEapPskNetworkAccessIdentifierP idP;
		int16_t ret;

		/* if (x_pib_value.m_au8Value[0] == MAC_WRP_BAND_ARIB) { */
		/*  // In ARIB, ID_P can be range between 8 and 36 bytes, its length depends on the message length */
		/*  idP.uc_size = u16MessageLength - 48; */
		/*  // Maximum size is checked */
		/*  if (idP.uc_size > NETWORK_ACCESS_IDENTIFIER_MAX_SIZE_P) { */
		/*    idP.uc_size = NETWORK_ACCESS_IDENTIFIER_MAX_SIZE_P; */
		/*  } */
		/* } */
		/* else { */
		idP.uc_size = 8;
		/* } */

		uint8_t au8Seed[NETWORK_ACCESS_IDENTIFIER_MAX_SIZE_P + NETWORK_ACCESS_IDENTIFIER_MAX_SIZE_S + 2 * member_size(struct TEapPskRand, m_au8Value)];
		uint8_t au8Seed_size = idP.uc_size + pIdS->uc_size + 2 * member_size(struct TEapPskRand, m_au8Value);

		uint16_t u16DecodeOffset = 0;
		uint16_t u16SeedOffset = 0;

		cipher_wrapper_cipher_context_t m_ctx;
		const cipher_wrapper_cipher_info_t *cipher_info;
		/* Initialize CMAC */
		cipher_info = cipher_wrapper_cipher_info_from_type(CIPHER_WRAPPER_CIPHER_AES_128_ECB);
		if (cipher_info == NULL) {
			LOG_DBG(Log("\ncipher_wrapper_cipher_info_from_type failed"));
		}

		ret = cipher_wrapper_cipher_setup( &m_ctx, cipher_info );
		LOG_DBG(Log("\n cipher_wrapper_cipher_setup returned %d", ret));

		memcpy(pRandS->m_au8Value, &pMessage[u16DecodeOffset], sizeof(pRandS->m_au8Value));
		u16DecodeOffset += sizeof(pRandS->m_au8Value);

		memcpy(pRandP->m_au8Value, &pMessage[u16DecodeOffset], sizeof(pRandP->m_au8Value));
		u16DecodeOffset += sizeof(pRandP->m_au8Value);

		memcpy(au8MacP, &pMessage[u16DecodeOffset], sizeof(au8MacP));
		u16DecodeOffset += sizeof(au8MacP);

		memcpy(idP.m_au8Value, &pMessage[u16DecodeOffset], idP.uc_size);
		u16DecodeOffset += idP.uc_size;

		/* compute MacP = CMAC-AES-128(AK, IdP||IdS||RandS||RandP) */
		memcpy(au8Seed, idP.m_au8Value, idP.uc_size);
		u16SeedOffset += idP.uc_size;

		memcpy(&au8Seed[u16SeedOffset], pIdS->m_au8Value, pIdS->uc_size);
		u16SeedOffset += pIdS->uc_size;

		memcpy(&au8Seed[u16SeedOffset], pRandS->m_au8Value, sizeof(pRandS->m_au8Value));
		u16SeedOffset += sizeof(pRandS->m_au8Value);

		memcpy(&au8Seed[u16SeedOffset], pRandP->m_au8Value, sizeof(pRandP->m_au8Value));
		u16SeedOffset += sizeof(pRandP->m_au8Value);

		ret = cipher_wrapper_cipher_cmac_starts(&m_ctx, pPskContext->m_Ak.m_au8Value, 8 * sizeof(pPskContext->m_Ak.m_au8Value));
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_starts returned %d", ret));

		ret = cipher_wrapper_cipher_cmac_update(&m_ctx, au8Seed, au8Seed_size);
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_update returned %d", ret));

		ret = cipher_wrapper_cipher_cmac_finish(&m_ctx, au8ExpectedMacP);
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_finish returned %d", ret));

		ret = cipher_wrapper_cipher_cmac_reset(&m_ctx);
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_reset returned %d", ret));

		cipher_wrapper_cipher_free( &m_ctx);

		LOG_DBG(Log("\n u16DecodeOffset: %d, u16SeedOffset: %d, au8Seed_size: %d, idP.uc_size: %d, pIdS->uc_size: %d\n", u16DecodeOffset, u16SeedOffset, au8Seed_size,
				idP.uc_size, pIdS->uc_size));

		bRet = (memcmp(au8ExpectedMacP, au8MacP, sizeof(au8MacP)) == 0);

		UNUSED(ret);
	}

	return bRet;
}

/**********************************************************************************************************************/

/** The EAP_PSK_Encode_Message3 primitive is used to encode the third EAP-PSK message (type 2)
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
uint16_t EAP_PSK_Encode_Message3(
		const struct TEapPskContext *pPskContext,
		uint8_t u8Identifier,
		const struct TEapPskRand *pRandS,
		const struct TEapPskRand *pRandP,
		const struct TEapPskNetworkAccessIdentifierS *pIdS,
		uint32_t u32Nonce,
		uint8_t u8PChannelResult,
		uint16_t u16PChannelDataLength,
		uint8_t *pPChannelData,
		uint16_t u16MemoryBufferLength,
		uint8_t *pMemoryBuffer
		)
{
	uint16_t u16Ret = 0;

	/* check the size of the buffer */
	if (u16MemoryBufferLength >= 59 + u16PChannelDataLength) {
		/* compute first MAC_S = CMAC-AES-128(AK, ID_S||RAND_P) */
		eax_ctx eaxCtx[1];
		uint8_t au8MacS[16];
		uint8_t *pProtectedData = 0L;
		uint16_t u16ProtectedDataLength = 0;
		uint8_t *pTag = 0L;
		uint8_t au8Nonce[16];
		int16_t ret;

		uint8_t au8Seed[NETWORK_ACCESS_IDENTIFIER_MAX_SIZE_S + member_size(struct TEapPskRand, m_au8Value)];
		uint8_t au8Seed_size = pIdS->uc_size + member_size(struct TEapPskRand, m_au8Value);

		cipher_wrapper_cipher_context_t m_ctx;
		const cipher_wrapper_cipher_info_t *cipher_info;
		/* Initialize CMAC */
		cipher_info = cipher_wrapper_cipher_info_from_type(CIPHER_WRAPPER_CIPHER_AES_128_ECB);
		if (cipher_info == NULL) {
			LOG_DBG(Log("\ncipher_wrapper_cipher_info_from_type failed"));
		}

		ret = cipher_wrapper_cipher_setup( &m_ctx, cipher_info );
		LOG_DBG(Log("\n cipher_wrapper_cipher_setup returned %d %d", ret, m_ctx.cipher_info->type));

		memcpy(au8Seed, pIdS->m_au8Value, pIdS->uc_size);
		memcpy(&au8Seed[pIdS->uc_size], pRandP->m_au8Value, sizeof(pRandP->m_au8Value));

		ret = cipher_wrapper_cipher_cmac_starts(&m_ctx, pPskContext->m_Ak.m_au8Value, 8 * sizeof(pPskContext->m_Ak.m_au8Value));
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_starts returned %d", ret));

		ret = cipher_wrapper_cipher_cmac_update(&m_ctx, au8Seed, au8Seed_size);
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_update returned %d", ret));

		ret = cipher_wrapper_cipher_cmac_finish(&m_ctx, au8MacS);
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_finish returned %d", ret));

		ret = cipher_wrapper_cipher_cmac_reset(&m_ctx);
		LOG_DBG(Log("\n cipher_wrapper_cipher_cmac_reset returned %d", ret));

		UNUSED(ret);

		cipher_wrapper_cipher_free( &m_ctx);

		/* encode the EAP header; length field will be set at the end of the block */
		pMemoryBuffer[0] = EAP_REQUEST;
		pMemoryBuffer[1] = u8Identifier;
		pMemoryBuffer[4] = EAP_PSK_IANA_TYPE;
		pMemoryBuffer[5] = EAP_PSK_T2;
		u16Ret = 6;

		/* EAP message: add PSK fields */
		memcpy(&pMemoryBuffer[u16Ret], pRandS->m_au8Value, sizeof(pRandS->m_au8Value));
		u16Ret += sizeof(pRandS->m_au8Value);
		memcpy(&pMemoryBuffer[u16Ret], au8MacS, sizeof(au8MacS));
		u16Ret += sizeof(au8MacS);

		/* prepare P-Channel content */
		/* nonce should be big endian */
		memset(au8Nonce, 0, sizeof(au8Nonce));
		au8Nonce[12] = pMemoryBuffer[u16Ret] = (uint8_t)((u32Nonce >> 24) & 0xFF);
		au8Nonce[13] = pMemoryBuffer[u16Ret + 1] = (uint8_t)((u32Nonce >> 16) & 0xFF);
		au8Nonce[14] = pMemoryBuffer[u16Ret + 2] = (uint8_t)((u32Nonce >> 8) & 0xFF);
		au8Nonce[15] = pMemoryBuffer[u16Ret + 3] = (uint8_t)(u32Nonce & 0xFF);
		u16Ret += 4;

		/* tag will be added later */
		pTag = &pMemoryBuffer[u16Ret];
		u16Ret += 16;

		/* protected data */
		pProtectedData = &pMemoryBuffer[u16Ret];

		if (u16PChannelDataLength > 0) {
			/* result / extension = 1 */
			pProtectedData[u16ProtectedDataLength] = (u8PChannelResult << 6) | 0x20;
			u16ProtectedDataLength++;

			memcpy(&pProtectedData[u16ProtectedDataLength], pPChannelData, u16PChannelDataLength);
			u16ProtectedDataLength += u16PChannelDataLength;
		} else {
			/* result / extension = 0 */
			pProtectedData[u16ProtectedDataLength] = (u8PChannelResult << 6);
			u16ProtectedDataLength++;
		}

		u16Ret += u16ProtectedDataLength;

		/* encrypt P-Channel using TEK key */
		if (RETURN_GOOD == eax_init_and_key(pPskContext->m_Tek.m_au8Value, sizeof(pPskContext->m_Tek.m_au8Value), eaxCtx)) {
			/* now update the EAP header length field */
			pMemoryBuffer[2] = (uint8_t)((u16Ret >> 8) & 0x00FF);
			pMemoryBuffer[3] = (uint8_t)(u16Ret & 0x00FF);

			/* The protected data is the 22 bytes header of the EAP message. */
			/* The G3 specifies a slightly modified EAP header but in the same time */
			/* the specification requires to compute the authentication tag over the */
			/* on the original EAP header */
			/* So we change the header to make it "EAP compliant", we compute the */
			/* auth tag and then we change back the header */

			/* right shift Code field with 2 bits as indicated in the EAP specification */
			pMemoryBuffer[0] >>= 2;

			LOG_DBG(LogBuffer(pPskContext->m_Tek.m_au8Value, sizeof(pPskContext->m_Tek.m_au8Value), "TEK: "));

			LOG_DBG(LogBuffer(au8Nonce, 16, "Nonce/IV: "));
			LOG_DBG(LogBuffer(pMemoryBuffer, 22, "Header: "));
			LOG_DBG(LogBuffer(pProtectedData, u16ProtectedDataLength, "Data-plain: "));
			LOG_DBG(LogBuffer(eaxCtx, sizeof(eaxCtx), "EaxCtx: "));

			if (RETURN_GOOD != eax_encrypt_message(
					au8Nonce, /* the initialization vector    */
					16, /* and its length in bytes      */
					pMemoryBuffer, /* the header buffer            */
					22, /* and its length in bytes      */
					pProtectedData, /* the message buffer           */
					u16ProtectedDataLength, /* and its length in bytes      */
					pTag, /* the buffer for the tag       */
					16, /* and its length in bytes      */
					eaxCtx) /* the mode context             */
					) {
				u16Ret = 0;
			}

			LOG_DBG(LogBuffer(pProtectedData, u16ProtectedDataLength, "Data-encr: "));
			LOG_DBG(LogBuffer(pTag, 16, "Tag: "));

			/* Fix EAP header: left shift Code field with 2 bits as indicated in the G3 specification */
			pMemoryBuffer[0] <<= 2;

			eax_end(eaxCtx);
		} else {
			u16Ret = 0;
		}
	}

	return u16Ret;
}

/**********************************************************************************************************************/

/**
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
bool EAP_PSK_Decode_Message4(
		uint16_t u16MessageLength,
		uint8_t *pMessage,
		const struct TEapPskContext *pPskContext,
		uint16_t u16HeaderLength,
		uint8_t *pHeader,
		struct TEapPskRand *pRandS,
		uint32_t *pu32Nonce,
		uint8_t *pu8PChannelResult,
		uint16_t *pu16PChannelDataLength,
		uint8_t **pPChannelData
		)
{
	bool bRet = false;

	/* TODO: review size (ARIB) */
	if (u16MessageLength >= 41) {
		eax_ctx eaxCtx[1];

		memcpy(pRandS->m_au8Value, pMessage, sizeof(pRandS->m_au8Value));

		/* decrypt P-CHANNEL */
		/* P-CHANNEL uses the TEK key */
		if (RETURN_GOOD == eax_init_and_key(pPskContext->m_Tek.m_au8Value, sizeof(pPskContext->m_Tek.m_au8Value), eaxCtx)) {
			uint8_t au8Nonce[16];
			uint8_t *pNonce = &pMessage[16];
			uint8_t *pTag = &pMessage[20];
			uint8_t *pProtectedData = &pMessage[36];
			uint16_t u16ProtectedDataLength = u16MessageLength - 36;

			/* prepare 16 bytes nonce */
			/* nonce should be big endian */
			memset(au8Nonce, 0, sizeof(au8Nonce));
			au8Nonce[12] = pNonce[0];
			au8Nonce[13] = pNonce[1];
			au8Nonce[14] = pNonce[2];
			au8Nonce[15] = pNonce[3];

			/* The protected data is the 22 bytes header of the EAP message. */
			/* The G3 specifies a slightly modified EAP header but in the same time */
			/* the specification requires to compute the authentication tag over the */
			/* on the original EAP header */
			/* So we change the header to make it "EAP compliant", we compute the */
			/* auth tag and then we change back the header */

			/* right shift Code field with 2 bits as indicated in the EAP specification */
			pHeader[0] >>= 2;

			LOG_DBG(LogBuffer(pPskContext->m_Tek.m_au8Value, sizeof(pPskContext->m_Tek.m_au8Value), "TEK: "));
			LOG_DBG(LogBuffer(au8Nonce, 16, "Nonce/IV: "));
			LOG_DBG(LogBuffer(pHeader, u16HeaderLength, "Header: "));
			LOG_DBG(LogBuffer(pProtectedData, u16ProtectedDataLength, "Data-encr: "));
			LOG_DBG(LogBuffer(pTag, 16, "Tag: "));

			/* RETURN_GOOD is returned if the input tag matches that for the decrypted message */
			if (RETURN_GOOD == eax_decrypt_message(
					au8Nonce, /* the initialization vector    */
					16, /* and its length in bytes      */
					pHeader, /* the header buffer            */
					u16HeaderLength, /* and its length in bytes      */
					pProtectedData, /* the message buffer           */
					u16ProtectedDataLength, /* and its length in bytes      */
					pTag, /* the buffer for the tag       */
					16, /* and its length in bytes      */
					eaxCtx) /* the mode context             */
					) {
				LOG_DBG(Log("Decode SUCCESS"));
				/* retrieve protected parameters */
				/* uint8_t u8ExtField = ((pProtectedData[0] & 0x20) >> 5); */
				*pu8PChannelResult = ((pProtectedData[0] & 0xC0) >> 6);
				*pPChannelData = &pProtectedData[1];
				*pu16PChannelDataLength = u16ProtectedDataLength - 1;

				((uint8_t *)pu32Nonce)[0] = pNonce[3];
				((uint8_t *)pu32Nonce)[1] = pNonce[2];
				((uint8_t *)pu32Nonce)[2] = pNonce[1];
				((uint8_t *)pu32Nonce)[3] = pNonce[0];

				bRet = true;
			} else {
				LOG_DBG(Log("Decode FAILED"));
			}

			LOG_DBG(LogBuffer(pProtectedData, u16ProtectedDataLength, "Data-plain: "));

			/* Fix EAP header: left shift Code field with 2 bits as indicated in the G3 specification */
			pHeader[0] <<= 2;

			eax_end(eaxCtx);
		}
	}

	return bRet;
}

/**********************************************************************************************************************/

/** The EAP_PSK_Encode_EAP_Success primitive is used to encode the EAP success message
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
uint16_t EAP_PSK_Encode_EAP_Success(
		uint8_t u8Identifier,
		uint16_t u16MemoryBufferLength,
		uint8_t *pMemoryBuffer
		)
{
	uint16_t u16Ret = 0;

	/* check the size of the buffer */
	if (u16MemoryBufferLength >= 4) {
		/* encode the EAP header; length field will be set at the end of the block */
		pMemoryBuffer[0] = EAP_SUCCESS;
		pMemoryBuffer[1] = u8Identifier;
		u16Ret = 4;

		/* now update the EAP header length field */
		pMemoryBuffer[2] = (uint8_t)((u16Ret >> 8) & 0x00FF);
		pMemoryBuffer[3] = (uint8_t)(u16Ret & 0x00FF);
	}

	return u16Ret;
}

/**********************************************************************************************************************/

/** The EAP_PSK_Encode_EAP_Failure primitive is used to encode the EAP failure message
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
uint16_t EAP_PSK_Encode_EAP_Failure(
		uint8_t u8Identifier,
		uint16_t u16MemoryBufferLength,
		uint8_t *pMemoryBuffer
		)
{
	uint16_t u16Ret = 0;

	/* check the size of the buffer */
	if (u16MemoryBufferLength >= 4) {
		/* encode the EAP header; length field will be set at the end of the block */
		pMemoryBuffer[0] = EAP_FAILURE;
		pMemoryBuffer[1] = u8Identifier;
		u16Ret = 4;

		/* now update the EAP header length field */
		pMemoryBuffer[2] = (uint8_t)((u16Ret >> 8) & 0x00FF);
		pMemoryBuffer[3] = (uint8_t)(u16Ret & 0x00FF);
	}

	return u16Ret;
}

/**********************************************************************************************************************/

/** The EAP_PSK_Encode_GMK_Activation primitive is used to encode the GMK activation message
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
uint16_t EAP_PSK_Encode_GMK_Activation(uint8_t *pPChannelData, uint16_t u16MemoryBufferLength, uint8_t *pMemoryBuffer)
{
	uint16_t u16Ret = 0;

	/* check the size of the buffer */
	if (u16MemoryBufferLength >= 3) {
		/* Add GMK-activation EAP message */
		memcpy(pMemoryBuffer, pPChannelData, 3);
		u16Ret = 3;
	}

	return u16Ret;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */