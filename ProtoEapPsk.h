/**
 * \file
 *
 * \brief USI Host EAP-PSK Protocol API
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
/**********************************************************************************************************************/

/** This file contains RFC4764 (The EAP-PSK Protocol: A Pre-Shared Key Extensible Authentication Protocol (EAP) Method)
* implementation related to G3 needs
*
***********************************************************************************************************************
*
* @file
*
**********************************************************************************************************************/

#ifndef __EAP_PSK_H__
#define __EAP_PSK_H__

#include <stdint.h>

/* todo: remove code needed by LBS (ifdef?) */

/**********************************************************************************************************************/

/** EAP-PSK message type
 *
 ***********************************************************************************************************************
 *
 * IANA allocated value
 *
 **********************************************************************************************************************/
#define EAP_PSK_IANA_TYPE 0x2F

/**********************************************************************************************************************/

/** EAP message types
 *
 ***********************************************************************************************************************
 *
 * The value takes in account the 2 reserved bits (values are left shifted by 2 bits)
 *
 **********************************************************************************************************************/
#define EAP_REQUEST 0x04
#define EAP_RESPONSE 0x08
#define EAP_SUCCESS 0x0C
#define EAP_FAILURE 0x10

/**********************************************************************************************************************/

/** T-subfield types
 *
 ***********************************************************************************************************************
 *
 * 0 The first EAP-PSK message
 * 1 The second EAP-PSK message
 * 2 The third EAP-PSK message
 * 3 The forth EAP-PSK message
 *
 **********************************************************************************************************************/
#define EAP_PSK_T0 (0x00 << 6)
#define EAP_PSK_T1 (0x01 << 6)
#define EAP_PSK_T2 (0x02 << 6)
#define EAP_PSK_T3 (0x03 << 6)

/**********************************************************************************************************************/

/** P-Channel result field
 *
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
#define PCHANNEL_RESULT_CONTINUE 0x01
#define PCHANNEL_RESULT_DONE_SUCCESS 0x02
#define PCHANNEL_RESULT_DONE_FAILURE 0x03

/**********************************************************************************************************************/

/** The EAP_PSK NetworkAccessIdentifier P & S types
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
#define NETWORK_ACCESS_IDENTIFIER_MAX_SIZE_S   34
#define NETWORK_ACCESS_IDENTIFIER_MAX_SIZE_P   36

#define NETWORK_ACCESS_IDENTIFIER_SIZE_S_ARIB   NETWORK_ACCESS_IDENTIFIER_MAX_SIZE_S
#define NETWORK_ACCESS_IDENTIFIER_SIZE_P_ARIB   NETWORK_ACCESS_IDENTIFIER_MAX_SIZE_P

#define NETWORK_ACCESS_IDENTIFIER_SIZE_S_CENELEC_FCC   8
#define NETWORK_ACCESS_IDENTIFIER_SIZE_P_CENELEC_FCC   8

struct TEapPskNetworkAccessIdentifierP {
	uint8_t uc_size;
	uint8_t m_au8Value[NETWORK_ACCESS_IDENTIFIER_MAX_SIZE_P];
};

struct TEapPskNetworkAccessIdentifierS {
	uint8_t uc_size;
	uint8_t m_au8Value[NETWORK_ACCESS_IDENTIFIER_MAX_SIZE_S];
};

/**********************************************************************************************************************/

/** The EAP_PSK key type
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
struct TEapPskKey {
	uint8_t m_au8Value[16];
};

/**********************************************************************************************************************/

/** The EAP_PSK RAND type
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
struct TEapPskRand {
	uint8_t m_au8Value[16];
};

/**********************************************************************************************************************/

/** The EAP_PSK NetworkAccessIdentifier
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
struct TEapPskNetworkAccessIdentifier {
	uint8_t m_au8Value[36]; /* the size is calculated to be able to store the IdP for ARIB band */
	uint8_t m_u8Length;
};

/**********************************************************************************************************************/

/** The EAP_PSK_Context type keeps information needed for EAP-PSK calls
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
struct TEapPskContext {
	struct TEapPskKey m_Kdk; /* Derivation key */
	struct TEapPskKey m_Ak; /* Authentication key */
	struct TEapPskKey m_Tek; /* Transient key */
	struct TEapPskNetworkAccessIdentifier m_IdS;
	struct TEapPskRand m_RandP;
	struct TEapPskRand m_RandS;
};

/**********************************************************************************************************************/

/** The EAP_PSK_Initialize primitive is used to initialize the EAP-PSK module
 ***********************************************************************************************************************
 * @param au8EAPPSK Shared secret needed to join the network
 * @param aesCtx The AES context structure
 * @param pContext OUT parameter; EAP-PSK context needed in other functions
 **********************************************************************************************************************/
void EAP_PSK_Initialize(const struct TEapPskKey *pKey, struct TEapPskContext *pPskContext);

/**********************************************************************************************************************/

/** The EAP_PSK_InitializeTEK primitive is used to initialize the TEK key
 ***********************************************************************************************************************
 * @param au8RandP RandP random number computed by the local device used in 2nd message
 * @param aesCtx The AES context structure
 * @param pContext OUT parameter; EAP-PSK context needed in other functions
 **********************************************************************************************************************/
void EAP_PSK_InitializeTEK(const struct TEapPskRand *pRandP, struct TEapPskContext *pPskContext);

/**********************************************************************************************************************/

/** The EAP_PSK_Decode_Message1 primitive is used to decode the first EAP-PSK message (type 0)
 ***********************************************************************************************************************
 * @param u16MessageLength Length of the message
 * @param pMessage Length of the message
 * @param pu8Code OUT parameter; upon successful return contains the Code field parameter
 * @param pu8Identifier OUT parameter; upon successful return contains the identifier field (aids in matching Responses
 *                                      with Requests)
 * @param pu8TSubfield OUT parameter; upon successful return contains the T subfield parameter
 * @param pu16EAPDataLength OUT parameter; upon successful return contains the length of the EAP data
 * @param pEAPData OUT parameter; upon successful return contains a pointer the the EAP data
 * @return true if the message can be decoded; false otherwise
 **********************************************************************************************************************/
bool EAP_PSK_Decode_Message(uint16_t u16MessageLength, uint8_t *pMessage, uint8_t *pu8Code, uint8_t *pu8Identifier,
		uint8_t *pu8TSubfield, uint16_t *pu16EAPDataLength, uint8_t **pEAPData);

/**********************************************************************************************************************/

/** The EAP_PSK_Decode_Message1 primitive is used to decode the first EAP-PSK message (type 0)
 ***********************************************************************************************************************
 * @param u16MessageLength Length of the message
 * @param pMessage Length of the message
 * @param au8RandS OUT parameter; upon successful return contains the RandS parameter (16 bytes random number
 *                                      generated by LBS)
 * @param au8IdS OUT parameter; upon successful return contains the IdS parameter (8 byte EUI-64 address of server)
 * @return true if the message can be decoded; false otherwise
 **********************************************************************************************************************/
bool EAP_PSK_Decode_Message1(uint16_t u16MessageLength, uint8_t *pMessage, struct TEapPskRand *pRandS,
		struct TEapPskNetworkAccessIdentifier *pIdS);

/**********************************************************************************************************************/

/** The EAP_PSK_Encode_Message2 primitive is used to encode the second EAP-PSK message (type 1)
 ***********************************************************************************************************************
 * @param context EAP-PSK context initialized in EAP_PSK_Initialize
 * @param u8Identifier Message identifier retrieved from the Request
 * @param au8RandS RandS parameter received from the server
 * @param au8RandP RandP random number computed by the local device
 * @param au8IdS IdS parameter received from the server
 * @param au8IdP IdP parameter: identity of the local device
 * @param u16MemoryBufferLength size of the buffer which will be used for data encoding
 * @param pMemoryBuffer OUT parameter; upon successful return contains the encoded message; this buffer should be previously
 *                              allocated; requested size being at least 62 bytes
 * @return encoded length or 0 if encoding failed
 **********************************************************************************************************************/
uint16_t EAP_PSK_Encode_Message2(const struct TEapPskContext *pPskContext, uint8_t u8Identifier,
		const struct TEapPskRand *pRandS, const struct TEapPskRand *pRandP, const struct TEapPskNetworkAccessIdentifier *pIdS,
		const struct TEapPskNetworkAccessIdentifier *pIdP, uint16_t u16MemoryBufferLength, uint8_t *pMemoryBuffer);

/**********************************************************************************************************************/

/** The EAP_PSK_Decode_Message3 primitive is used to decode the third EAP-PSK message (type 2)
 ***********************************************************************************************************************
 * @param u16MessageLength Length of the message
 * @param pMessage Length of the message
 * @param pskContext Initialized PSK context
 * @param u16HeaderLength Length of the header field
 * @param pHeader Header field: the first 22 bytes of the EAP Request or Response packet used to compute the
 *         auth tag
 * @param au8RandS OUT parameter; upon successful return contains the RandS parameter (16 bytes random number
 *                                      generated by LBS)
 * @param pu8PChannelResult OUT parameter; upon successful return contains the result indication flag
 * @param pau8CurrGMKId OUT parameter; upon successful return contains the Key Identifier of the current GMK
 * @param au8CurrGMK OUT parameter; upon successful return contains the 16 byte value of the current GMK
 * @param au8PrecGMK OUT parameter; upon successful return contains the 16 byte value of the preceding GMK
 * @return true if the message can be decoded; false otherwise
 **********************************************************************************************************************/
bool EAP_PSK_Decode_Message3(uint16_t u16MessageLength, uint8_t *pMessage, const struct TEapPskContext *pPskContext,
		uint16_t u16HeaderLength, uint8_t *pHeader, struct TEapPskRand *pRandS, uint32_t *pu32Nonce,
		uint8_t *pu8PChannelResult, uint16_t *pu16PChannelDataLength, uint8_t **pPChannelData);

/**********************************************************************************************************************/

/** The EAP_PSK_Encode_Message4 primitive is used to encode the second EAP-PSK message (type 3)
 ***********************************************************************************************************************
 * @param pskContext EAP-PSK context initialized in EAP_PSK_Initialize
 * @param u8Identifier Message identifier retrieved from the Request
 * @param au8RandS RandS parameter received from the server
 * @param u32Nonce Nonce needed for P-Channel
 * @param u8PChannelResult
 * @param u16MemoryBufferLength size of the buffer which will be used for data encoding
 * @param pMemoryBuffer OUT parameter; upon successful return contains the encoded message; this buffer should be previously
 *                              allocated; requested size being at least 62 bytes
 * @return encoded length or 0 if encoding failed
 **********************************************************************************************************************/
uint16_t EAP_PSK_Encode_Message4(const struct TEapPskContext *pPskContext, uint8_t u8Identifier,
		const struct TEapPskRand *pRandS, uint32_t u32Nonce, uint8_t u8PChannelResult, uint16_t u16PChannelDataLength,
		uint8_t *pPChannelData, uint16_t u16MemoryBufferLength, uint8_t *pMemoryBuffer);

/**********************************************************************************************************************/

/** The EAP_PSK_Encode_Message1 primitive is used to decode the first EAP-PSK message (type 0)
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
 *                              allocated
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
		);

/**********************************************************************************************************************/

/** The EAP_PSK_Decode_Message2 primitive is used to decode the second EAP-PSK message (type 1) and also to check
 * the MacP parameter
 ***********************************************************************************************************************
 *
 * @param context EAP-PSK context initialized in EAP_PSK_Initialize
 * @return true if the message can be decoded and the MacP field verified; false otherwise
 *
 **********************************************************************************************************************/
bool EAP_PSK_Decode_Message2(
		uint8_t u8BandId,
		uint16_t u16MessageLength,
		uint8_t *pMessage,
		const struct TEapPskContext *pPskContext,
		const struct TEapPskNetworkAccessIdentifierS *pIdS,
		struct TEapPskRand *pRandS, /* out */
		struct TEapPskRand *pRandP /* out */
		);

/**********************************************************************************************************************/

/** The EAP_PSK_Encode_Message3 primitive is used to encode the third EAP-PSK message (type 2)
 ***********************************************************************************************************************
 *
 * @param context EAP-PSK context initialized in EAP_PSK_Initialize
 *
 * @param u8Identifier Message identifier retrieved from the Request
 *
 * @param au8RandS RandS parameter received from the server
 *
 * @param au8RandP RandP random number computed by the local device
 *
 * @param au8IdS IdS parameter received from the server
 *
 * @param au8IdP IdP parameter: identity of the local device
 *
 * @param bAuthSuccess true if authentication was successfull; false otherwise
 *
 * @param u32Nonce nonce needed by P-Tunnel
 *
 * @param u8CurrGMKId Represents the Key Identifier of the current GMK
 *
 * @param au8CurrGMK 16 byte value of the current GMK
 *
 * @param au8PrecGMK 16 byte value of the preceding GMK
 *
 * @param u16MemoryBufferLength size of the buffer which will be used for data encoding
 *
 * @param pMemoryBuffer OUT parameter; upon successful return contains the encoded message; this buffer should be previously
 *                              allocated; requested size being at least 62 bytes
 *
 * @return encoded length or 0 if encoding failed
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
		);

/**********************************************************************************************************************/

/** The EAP_PSK_Decode_Message4
 ***********************************************************************************************************************
 *
 *
 * @return encoded length or 0 if encoding failed
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
		);

/**********************************************************************************************************************/

/** The EAP_PSK_Encode_EAP_Success primitive is used to encode the EAP success message
 ***********************************************************************************************************************
 *
 * @param u8Identifier Message identifier
 *
 * @param u16MemoryBufferLength size of the buffer which will be used for data encoding
 *
 * @param pMemoryBuffer OUT parameter; upon successful return contains the encoded message; this buffer should be previously
 *                              allocated; requested size being at least 62 bytes
 *
 * @return encoded length or 0 if encoding failed
 *
 **********************************************************************************************************************/
uint16_t EAP_PSK_Encode_EAP_Success(
		uint8_t u8Identifier,
		uint16_t u16MemoryBufferLength,
		uint8_t *pMemoryBuffer
		);

/**********************************************************************************************************************/

/** The EAP_PSK_Encode_EAP_Failure primitive is used to encode the EAP failure message
 ***********************************************************************************************************************
 *
 * @param u8Identifier Message identifier
 *
 * @param u16MemoryBufferLength size of the buffer which will be used for data encoding
 *
 * @param pMemoryBuffer OUT parameter; upon successful return contains the encoded message; this buffer should be previously
 *                      allocated; requested size being at least 62 bytes
 *
 * @return encoded length or 0 if encoding failed
 *
 **********************************************************************************************************************/
uint16_t EAP_PSK_Encode_EAP_Failure(
		uint8_t u8Identifier,
		uint16_t u16MemoryBufferLength,
		uint8_t *pMemoryBuffer
		);

/**********************************************************************************************************************/

/** The EAP_PSK_Encode_GMK_Activation primitive is used to encode the GMK activation message (end of re-keying process).
 ***********************************************************************************************************************
 *
 * @param pPChannelData
 *
 * @param u16MemoryBufferLength size of the buffer which will be used for data encoding
 *
 * @param pMemoryBuffer OUT parameter; upon successful return contains the encoded message; this buffer should be previously
 *                              allocated; requested size being at least 62 bytes
 *
 * @return encoded length or 0 if encoding failed
 *
 **********************************************************************************************************************/
uint16_t EAP_PSK_Encode_GMK_Activation(
		uint8_t *pPChannelData,
		uint16_t u16MemoryBufferLength,
		uint8_t *pMemoryBuffer
		);

#endif

/**********************************************************************************************************************/

/** @}
 **********************************************************************************************************************/
