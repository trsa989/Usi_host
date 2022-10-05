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

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "aes_wrapper.h"
#include "mbedtls/aes.h"
#include "mbedtls/memory_buffer_alloc.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/** Key in byte format */
static uint8_t spuc_key[256];
/** Key size in bits */
static uint16_t sus_key_size;

mbedtls_aes_context aes_ctx;

unsigned char mbedtls_buf[512];

void crypto_init(void)
{
	mbedtls_memory_buffer_alloc_init(mbedtls_buf, sizeof(mbedtls_buf));
}

AES_RETURN aes_encrypt(const unsigned char *in, unsigned char *out)
{
	/* Initialize the AES */
	mbedtls_aes_init(&aes_ctx);

	/* Trigger the AES */
	mbedtls_aes_setkey_enc(&aes_ctx, spuc_key, sus_key_size);
	mbedtls_aes_encrypt(&aes_ctx, in, out);

	/* Free the AES */
	mbedtls_aes_free(&aes_ctx);

	return EXIT_SUCCESS;
}

AES_RETURN aes_decrypt(const unsigned char *in, unsigned char *out)
{
	/* Initialize the AES */
	mbedtls_aes_init(&aes_ctx);

	/* Trigger the AES */
	mbedtls_aes_setkey_dec(&aes_ctx, spuc_key, sus_key_size);
	mbedtls_aes_decrypt(&aes_ctx, in, out);

	/* Free the AES */
	mbedtls_aes_free(&aes_ctx);

	return EXIT_SUCCESS;
}

AES_RETURN aes_key(const unsigned char *key, int key_len)
{
	/* Store the key */
	memcpy(spuc_key, key, key_len);

	/* Store the key size */
	sus_key_size = key_len * 8;

	return EXIT_SUCCESS;
}

void aes_wrapper_aes_init(aes_wrapper_context *ctx)
{
	mbedtls_aes_init((mbedtls_aes_context *)ctx);
}

int aes_wrapper_aes_setkey_enc(aes_wrapper_context *ctx, const unsigned char *key, unsigned int keybits)
{
	return mbedtls_aes_setkey_enc((mbedtls_aes_context *)ctx, key, keybits);
}

void aes_wrapper_aes_encrypt(aes_wrapper_context *ctx, const unsigned char input[16], unsigned char output[16])
{
	mbedtls_aes_encrypt((mbedtls_aes_context *)ctx, input, output);
}

void aes_wrapper_aes_free(aes_wrapper_context *ctx)
{
	mbedtls_aes_free((mbedtls_aes_context *)ctx);
}

#if defined(__cplusplus)
}
#endif
