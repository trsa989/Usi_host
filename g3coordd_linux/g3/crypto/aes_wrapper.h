/**
 * \file
 *
 * \brief aes_wrapper : Wrapper layer between G3 stack and AES module
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

#ifndef _AES_WRAPPER_H
#define _AES_WRAPPER_H

#include <stdlib.h>

/*  This include is used to find 8 & 32 bit unsigned integer types  */
#include "brg_types.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * \brief The AES context-type definition.
 */
typedef struct {
	int nr;                 /*!< The number of rounds. */
	uint32_t *rk;           /*!< AES round keys. */
	uint32_t buf[68];       /*!< Unaligned data buffer. This buffer can
	                         *   hold 32 extra Bytes, which can be used for
	                         *   one of the following purposes:
	                         *   Alignment if VIA padlock is used.
	                         *   Simplifying key expansion in the 256-bit
	                         *       case by generating an extra round key. */
}
aes_wrapper_context;

#define AES_VAR     /* if variable key size scheduler is needed     */

#define AES_ENCRYPT /* if support for encryption is needed          */
#define AES_DECRYPT /* if support for decryption is needed          */

#define AES_BLOCK_SIZE  16  /* the AES block size in bytes          */
#define N_COLS           4  /* the number of columns in the state   */

/* The key schedule length is 11, 13 or 15 16-byte blocks for 128,  */
/* 192 or 256-bit keys respectively. That is 176, 208 or 240 bytes  */
/* or 44, 52 or 60 32-bit words.                                    */
#define KS_LENGTH       44

#define AES_RETURN INT_RETURN

/* This routine must be called before first use if non-static       */
/* tables are being used                                            */
void crypto_init(void);

AES_RETURN aes_encrypt(const unsigned char *in, unsigned char *out);
AES_RETURN aes_decrypt(const unsigned char *in, unsigned char *out);
AES_RETURN aes_key(const unsigned char *key, int key_len);

/* API functions to map on source file to mbedTLS or library used */
void aes_wrapper_aes_init(aes_wrapper_context *ctx);
int aes_wrapper_aes_setkey_enc(aes_wrapper_context *ctx, const unsigned char *key, unsigned int keybits);
void aes_wrapper_aes_encrypt(aes_wrapper_context * ctx, const unsigned char input[16], unsigned char output[16]);
void aes_wrapper_aes_free(aes_wrapper_context *ctx);

#if defined(__cplusplus)
}
#endif

#endif
