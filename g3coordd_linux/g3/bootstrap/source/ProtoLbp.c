/**
 * \file
 *
 * \brief BootStrapping LBP
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
/* #include <AdpNetwork.h> */
#include <AdpApiTypes.h>
#include <ProtoLbp.h>
/* #include <AdpMacInterface.h> */
#include <string.h>

#ifdef G3_HYBRID_PROFILE
/* Disable backup medium flag to add in LBP Header */
#define DISABLE_BACKUP_FLAG   0x01
#endif

/**********************************************************************************************************************/
/** The LBP_Encode_KickFromLBDRequest primitive is used to encode the LBP KICK type message generated by LBD
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
uint16_t LBP_Encode_KickFromLBDRequest(const struct TAdpExtendedAddress *pEUI64Address, uint16_t u16MessageLength,
  uint8_t *pMessageBuffer)
{
  uint16_t u16EncodedLength = 0;

  // check first if the message buffer size if enough
  if (u16MessageLength >= sizeof(struct TAdpExtendedAddress) + 2) {
    // start message encoding
#ifdef G3_HYBRID_PROFILE
		/* DISABLE_BACKUP_FLAG not set in Kick frames, MediaType set to 0x0 */
		pMessageBuffer[0] = (LBP_KICK_FROM_LBD << 4);
#else
		pMessageBuffer[0] = (LBP_KICK_FROM_LBD << 4);
#endif
    pMessageBuffer[1] = 0; // transaction id is reserved

    memcpy(&pMessageBuffer[2], pEUI64Address, sizeof(struct TAdpExtendedAddress));

    u16EncodedLength = sizeof(struct TAdpExtendedAddress) + 2;
  }

  return u16EncodedLength;
}

#ifdef G3_HYBRID_PROFILE

/**********************************************************************************************************************/
/** LBP_Encode_JoiningRequest
 **********************************************************************************************************************/
uint16_t LBP_Encode_JoiningRequest(const struct TAdpExtendedAddress *pEUI64Address, uint8_t u8MediaType,
                                   uint16_t u16BootStrappingDataLength, uint16_t u16MessageLength, uint8_t *pMessageBuffer)
{
  uint16_t u16EncodedLength = 0;

  // check first if the message buffer size if enough
  if (u16MessageLength >= u16BootStrappingDataLength + sizeof(struct TAdpExtendedAddress) + 2) {
    // as the bootstrapping data is already in the message buffer, move it to its place
    // after the A_LDB field
    memmove(&pMessageBuffer[sizeof(struct TAdpExtendedAddress) + 2], pMessageBuffer, u16BootStrappingDataLength);

    // start message encoding
	pMessageBuffer[0] = (LBP_JOINING << 4) | (u8MediaType << 3) | (DISABLE_BACKUP_FLAG << 2);
    pMessageBuffer[1] = 0; // transaction id is reserved

    memcpy(&pMessageBuffer[2], pEUI64Address, sizeof(struct TAdpExtendedAddress));

    u16EncodedLength = u16BootStrappingDataLength + sizeof(struct TAdpExtendedAddress) + 2;
  }

  return u16EncodedLength;
}

/**********************************************************************************************************************/

/** The LBP_Encode_ChallengeRequest primitive is used to encode the LBP CHALLENGE type message.
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
uint16_t LBP_Encode_ChallengeRequest(const struct TAdpExtendedAddress *pEUI64Address, uint8_t u8MediaType,
		uint8_t u8DisableBackupMedium, uint16_t u16BootStrappingDataLength, uint16_t u16MessageLength, uint8_t *pMessageBuffer)
{
	uint16_t u16EncodedLength = 0;

	/* check first if the message buffer size if enough */
	if (u16MessageLength >= u16BootStrappingDataLength + sizeof(struct TAdpExtendedAddress) + 2) {
		/* as the bootstrapping data is already in the message buffer, move it to its place */
		/* after the A_LDB field */
		memmove(&pMessageBuffer[sizeof(struct TAdpExtendedAddress) + 2], pMessageBuffer, u16BootStrappingDataLength);

		/* start message encoding */
		pMessageBuffer[0] = (LBP_CHALLENGE << 4) | (u8MediaType << 3) | (u8DisableBackupMedium << 2);
		pMessageBuffer[1] = 0; /* transaction id is reserved */

		memcpy(&pMessageBuffer[2], pEUI64Address, sizeof(struct TAdpExtendedAddress));

		u16EncodedLength = u16BootStrappingDataLength + sizeof(struct TAdpExtendedAddress) + 2;
	}

	return u16EncodedLength;
}

/**********************************************************************************************************************/

/** The LBP_Encode_AcceptedRequest primitive is used to encode the LBP ACCEPTED type message.
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
uint16_t LBP_Encode_AcceptedRequest(const struct TAdpExtendedAddress *pEUI64Address, uint8_t u8MediaType,
		uint8_t u8DisableBackupMedium, uint16_t u16BootStrappingDataLength, uint16_t u16MessageLength, uint8_t *pMessageBuffer)
{
	uint16_t u16EncodedLength = 0;

	/* check first if the message buffer size if enough */
	if (u16MessageLength >= u16BootStrappingDataLength + sizeof(struct TAdpExtendedAddress) + 2) {
		/* as the bootstrapping data is already in the message buffer, move it to its place */
		/* after the A_LDB field */
		memmove(&pMessageBuffer[8 + 2], pMessageBuffer, u16BootStrappingDataLength);

		/* start message encoding */
		pMessageBuffer[0] = (LBP_ACCEPTED << 4) | (u8MediaType << 3) | (u8DisableBackupMedium << 2);
		pMessageBuffer[1] = 0; /* transaction id is reserved */

		memcpy(&pMessageBuffer[2], pEUI64Address, sizeof(struct TAdpExtendedAddress));

		u16EncodedLength = u16BootStrappingDataLength + sizeof(struct TAdpExtendedAddress) + 2;
	}

	return u16EncodedLength;
}

#else

/**********************************************************************************************************************/
/** LBP_Encode_JoiningRequest
 **********************************************************************************************************************/
uint16_t LBP_Encode_JoiningRequest(const struct TAdpExtendedAddress *pEUI64Address, uint16_t u16BootStrappingDataLength,
  uint16_t u16MessageLength, uint8_t *pMessageBuffer)
{
  uint16_t u16EncodedLength = 0;

  // check first if the message buffer size if enough
  if (u16MessageLength >= u16BootStrappingDataLength + sizeof(struct TAdpExtendedAddress) + 2) {
    // as the bootstrapping data is already in the message buffer, move it to its place
    // after the A_LDB field
    memmove(&pMessageBuffer[sizeof(struct TAdpExtendedAddress) + 2], pMessageBuffer, u16BootStrappingDataLength);

    // start message encoding
	pMessageBuffer[0] = (LBP_JOINING << 4);
    pMessageBuffer[1] = 0; // transaction id is reserved

    memcpy(&pMessageBuffer[2], pEUI64Address, sizeof(struct TAdpExtendedAddress));

    u16EncodedLength = u16BootStrappingDataLength + sizeof(struct TAdpExtendedAddress) + 2;
  }

  return u16EncodedLength;
}

/**********************************************************************************************************************/
/** The LBP_Encode_ChallengeRequest primitive is used to encode the LBP CHALLENGE type message.
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
uint16_t LBP_Encode_ChallengeRequest(const struct TAdpExtendedAddress *pEUI64Address,
  uint16_t u16BootStrappingDataLength, uint16_t u16MessageLength, uint8_t *pMessageBuffer)
{
  uint16_t u16EncodedLength = 0;

  // check first if the message buffer size if enough
  if (u16MessageLength >= u16BootStrappingDataLength + sizeof(struct TAdpExtendedAddress) + 2) {
    // as the bootstrapping data is already in the message buffer, move it to its place
    // after the A_LDB field
    memmove(&pMessageBuffer[sizeof(struct TAdpExtendedAddress) + 2], pMessageBuffer, u16BootStrappingDataLength);

    // start message encoding
    pMessageBuffer[0] = (LBP_CHALLENGE << 4);
    pMessageBuffer[1] = 0; // transaction id is reserved

    memcpy(&pMessageBuffer[2], pEUI64Address, sizeof(struct TAdpExtendedAddress));

    u16EncodedLength = u16BootStrappingDataLength + sizeof(struct TAdpExtendedAddress) + 2;
  }

  return u16EncodedLength;
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/

/** The LBP_Encode_AcceptedRequest primitive is used to encode the LBP ACCEPTED type message.
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
uint16_t LBP_Encode_AcceptedRequest(const struct TAdpExtendedAddress *pEUI64Address,
		uint16_t u16BootStrappingDataLength, uint16_t u16MessageLength, uint8_t *pMessageBuffer)
{
	uint16_t u16EncodedLength = 0;

	/* check first if the message buffer size if enough */
	if (u16MessageLength >= u16BootStrappingDataLength + sizeof(struct TAdpExtendedAddress) + 2) {
		/* as the bootstrapping data is already in the message buffer, move it to its place */
		/* after the A_LDB field */
		memmove(&pMessageBuffer[8 + 2], pMessageBuffer, u16BootStrappingDataLength);

		/* start message encoding */
		pMessageBuffer[0] = (LBP_ACCEPTED << 4);
		pMessageBuffer[1] = 0; /* transaction id is reserved */

		memcpy(&pMessageBuffer[2], pEUI64Address, sizeof(struct TAdpExtendedAddress));

		u16EncodedLength = u16BootStrappingDataLength + sizeof(struct TAdpExtendedAddress) + 2;
	}

	return u16EncodedLength;
}

#endif
/** LBP_IsChallengeResponse
 *
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
bool LBP_IsChallengeResponse(uint16_t u16MessageLength, uint8_t *pMessageBuffer)
{
  return ((u16MessageLength > 1) && ((pMessageBuffer[0] & (LBP_CHALLENGE << 4)) == LBP_CHALLENGE));
}

/**********************************************************************************************************************/

/** LBP_IsAcceptedResponse
 *
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
bool LBP_IsAcceptedResponse(uint16_t u16MessageLength, uint8_t *pMessageBuffer)
{
  return ((u16MessageLength > 1) && ((pMessageBuffer[0] & (LBP_ACCEPTED << 4)) == LBP_ACCEPTED));
}

/**********************************************************************************************************************/

/** LBP_IsDeclineResponse
 *
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
bool LBP_IsDeclineResponse(uint16_t u16MessageLength, uint8_t *pMessageBuffer)
{
  return ((u16MessageLength > 1) && ((pMessageBuffer[0] & (LBP_DECLINE << 4)) == LBP_DECLINE));
}

/**********************************************************************************************************************/

/** LBP_Decode_Message
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
bool LBP_Decode_Message(uint16_t u16MessageLength, uint8_t *pMessageBuffer, uint8_t *pu8MessageType,
  struct TAdpExtendedAddress *pEUI64Address, uint16_t *pu16BootStrappingDataLength, uint8_t **pBootStrappingData)
{
  bool bRet = false;

  if (u16MessageLength >= 2 + sizeof(struct TAdpExtendedAddress)) {
    *pu8MessageType = ((pMessageBuffer[0] & 0xF0) >> 4);

    memcpy(&pEUI64Address->m_au8Value, &pMessageBuffer[2], sizeof(struct TAdpExtendedAddress));

    *pBootStrappingData = &pMessageBuffer[2 + sizeof(struct TAdpExtendedAddress)];
    *pu16BootStrappingDataLength = u16MessageLength - (2 + sizeof(struct TAdpExtendedAddress));

    bRet = true;
  }

  return bRet;
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */