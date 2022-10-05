/**
 * \file
 *
 * \brief aes_wrapper : Wrapper layer between G3 stack and Cipher module
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
#include "cipher_wrapper.h"
#include "mbedtls/cipher.h"
#include "mbedtls/cmac.h"
#include "mbedtls/ccm.h"

#if defined(__cplusplus)
extern "C"
{
#endif

const cipher_wrapper_cipher_info_t *cipher_wrapper_cipher_info_from_type(const cipher_wrapper_cipher_type_t cipher_type)
{
	return (const cipher_wrapper_cipher_info_t *)(mbedtls_cipher_info_from_type((mbedtls_cipher_type_t)cipher_type));
}

int cipher_wrapper_cipher_setup(cipher_wrapper_cipher_context_t *ctx, const cipher_wrapper_cipher_info_t *cipher_info)
{
	return mbedtls_cipher_setup((mbedtls_cipher_context_t *)ctx, (const mbedtls_cipher_info_t *)cipher_info);
}

int cipher_wrapper_cipher_cmac_starts(cipher_wrapper_cipher_context_t *ctx, const unsigned char *key, size_t keybits)
{
	return mbedtls_cipher_cmac_starts((mbedtls_cipher_context_t *)ctx, key, keybits);
}

int cipher_wrapper_cipher_cmac_update(cipher_wrapper_cipher_context_t *ctx, const unsigned char *input, size_t ilen)
{
	return mbedtls_cipher_cmac_update((mbedtls_cipher_context_t *)ctx, input, ilen);
}

int cipher_wrapper_cipher_cmac_finish(cipher_wrapper_cipher_context_t *ctx, unsigned char *output)
{
	return mbedtls_cipher_cmac_finish((mbedtls_cipher_context_t *)ctx, output);
}

int cipher_wrapper_cipher_cmac_reset(cipher_wrapper_cipher_context_t *ctx)
{
	return mbedtls_cipher_cmac_reset((mbedtls_cipher_context_t *)ctx );
}

void cipher_wrapper_cipher_free(cipher_wrapper_cipher_context_t *ctx)
{
	mbedtls_cipher_free((mbedtls_cipher_context_t *)ctx);
}

void cipher_wrapper_ccm_init(cipher_wrapper_ccm_context *ctx)
{
	mbedtls_ccm_init((mbedtls_ccm_context *)ctx);
}

int cipher_wrapper_ccm_setkey(cipher_wrapper_ccm_context *ctx, cipher_wrapper_cipher_id_t cipher,
		const unsigned char *key, unsigned int keybits)
{
	return mbedtls_ccm_setkey((mbedtls_ccm_context *)ctx, (mbedtls_cipher_id_t)cipher, key, keybits);
}

int cipher_wrapper_ccm_auth_decrypt(cipher_wrapper_ccm_context *ctx, size_t length, const unsigned char *iv,
		size_t iv_len, const unsigned char *add, size_t add_len, const unsigned char *input,
		unsigned char *output, const unsigned char *tag, size_t tag_len)
{
	return mbedtls_ccm_auth_decrypt((mbedtls_ccm_context *)ctx, length, iv, iv_len, add, add_len, input, output, tag, tag_len);
}

int cipher_wrapper_ccm_encrypt_and_tag(cipher_wrapper_ccm_context *ctx, size_t length, const unsigned char *iv,
		size_t iv_len, const unsigned char *add, size_t add_len, const unsigned char *input,
		unsigned char *output, unsigned char *tag, size_t tag_len)
{
	return mbedtls_ccm_encrypt_and_tag((mbedtls_ccm_context *)ctx, length, iv, iv_len, add, add_len, input, output, tag, tag_len);
}

void cipher_wrapper_ccm_free(cipher_wrapper_ccm_context *ctx)
{
	mbedtls_ccm_free((mbedtls_ccm_context *)ctx);
}

#if defined(__cplusplus)
}
#endif
