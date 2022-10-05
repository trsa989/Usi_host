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

#ifndef _CIPHER_WRAPPER_H
#define _CIPHER_WRAPPER_H

#if defined(__cplusplus)
extern "C"
{
#endif

#define RETURN_GOOD 0

#define CIPHER_WRAPPER_CMAC_C
/* #define CIPHER_WRAPPER_CIPHER_MODE_WITH_PADDING */

#define CIPHER_WRAPPER_CIPHER_BLKSIZE_MAX      16  /**< The longest block used by CMAC is that of AES. */

/** Maximum length of any IV, in Bytes. */
#define CIPHER_WRAPPER_MAX_IV_LENGTH      16
/** Maximum block size of any cipher, in Bytes. */
#define CIPHER_WRAPPER_MAX_BLOCK_LENGTH   16

/** Type of operation. */
typedef enum {
	CIPHER_WRAPPER_OPERATION_NONE = -1,
	CIPHER_WRAPPER_DECRYPT = 0,
	CIPHER_WRAPPER_ENCRYPT,
} cipher_wrapper_operation_t;

/**
 * \brief     Supported cipher types.
 *
 * \warning   RC4 and DES are considered weak ciphers and their use
 *            constitutes a security risk. Arm recommends considering stronger
 *            ciphers instead.
 */
typedef enum {
	CIPHER_WRAPPER_CIPHER_ID_NONE = 0, /**< Placeholder to mark the end of cipher ID lists. */
	CIPHER_WRAPPER_CIPHER_ID_NULL,  /**< The identity cipher, treated as a stream cipher. */
	CIPHER_WRAPPER_CIPHER_ID_AES,   /**< The AES cipher. */
	CIPHER_WRAPPER_CIPHER_ID_DES,   /**< The DES cipher. */
	CIPHER_WRAPPER_CIPHER_ID_3DES,  /**< The Triple DES cipher. */
	CIPHER_WRAPPER_CIPHER_ID_CAMELLIA, /**< The Camellia cipher. */
	CIPHER_WRAPPER_CIPHER_ID_BLOWFISH, /**< The Blowfish cipher. */
	CIPHER_WRAPPER_CIPHER_ID_ARC4,  /**< The RC4 cipher. */
} cipher_wrapper_cipher_id_t;

/**
 * \brief     Supported {cipher type, cipher mode} pairs.
 *
 * \warning   RC4 and DES are considered weak ciphers and their use
 *            constitutes a security risk. Arm recommends considering stronger
 *            ciphers instead.
 */
typedef enum {
	CIPHER_WRAPPER_CIPHER_NONE = 0,         /**< Placeholder to mark the end of cipher-pair lists. */
	CIPHER_WRAPPER_CIPHER_NULL,             /**< The identity stream cipher. */
	CIPHER_WRAPPER_CIPHER_AES_128_ECB,      /**< AES cipher with 128-bit ECB mode. */
	CIPHER_WRAPPER_CIPHER_AES_192_ECB,      /**< AES cipher with 192-bit ECB mode. */
	CIPHER_WRAPPER_CIPHER_AES_256_ECB,      /**< AES cipher with 256-bit ECB mode. */
	CIPHER_WRAPPER_CIPHER_AES_128_CBC,      /**< AES cipher with 128-bit CBC mode. */
	CIPHER_WRAPPER_CIPHER_AES_192_CBC,      /**< AES cipher with 192-bit CBC mode. */
	CIPHER_WRAPPER_CIPHER_AES_256_CBC,      /**< AES cipher with 256-bit CBC mode. */
	CIPHER_WRAPPER_CIPHER_AES_128_CFB128,   /**< AES cipher with 128-bit CFB128 mode. */
	CIPHER_WRAPPER_CIPHER_AES_192_CFB128,   /**< AES cipher with 192-bit CFB128 mode. */
	CIPHER_WRAPPER_CIPHER_AES_256_CFB128,   /**< AES cipher with 256-bit CFB128 mode. */
	CIPHER_WRAPPER_CIPHER_AES_128_CTR,      /**< AES cipher with 128-bit CTR mode. */
	CIPHER_WRAPPER_CIPHER_AES_192_CTR,      /**< AES cipher with 192-bit CTR mode. */
	CIPHER_WRAPPER_CIPHER_AES_256_CTR,      /**< AES cipher with 256-bit CTR mode. */
	CIPHER_WRAPPER_CIPHER_AES_128_GCM,      /**< AES cipher with 128-bit GCM mode. */
	CIPHER_WRAPPER_CIPHER_AES_192_GCM,      /**< AES cipher with 192-bit GCM mode. */
	CIPHER_WRAPPER_CIPHER_AES_256_GCM,      /**< AES cipher with 256-bit GCM mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_128_ECB, /**< Camellia cipher with 128-bit ECB mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_192_ECB, /**< Camellia cipher with 192-bit ECB mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_256_ECB, /**< Camellia cipher with 256-bit ECB mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_128_CBC, /**< Camellia cipher with 128-bit CBC mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_192_CBC, /**< Camellia cipher with 192-bit CBC mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_256_CBC, /**< Camellia cipher with 256-bit CBC mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_128_CFB128, /**< Camellia cipher with 128-bit CFB128 mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_192_CFB128, /**< Camellia cipher with 192-bit CFB128 mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_256_CFB128, /**< Camellia cipher with 256-bit CFB128 mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_128_CTR, /**< Camellia cipher with 128-bit CTR mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_192_CTR, /**< Camellia cipher with 192-bit CTR mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_256_CTR, /**< Camellia cipher with 256-bit CTR mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_128_GCM, /**< Camellia cipher with 128-bit GCM mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_192_GCM, /**< Camellia cipher with 192-bit GCM mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_256_GCM, /**< Camellia cipher with 256-bit GCM mode. */
	CIPHER_WRAPPER_CIPHER_DES_ECB,          /**< DES cipher with ECB mode. */
	CIPHER_WRAPPER_CIPHER_DES_CBC,          /**< DES cipher with CBC mode. */
	CIPHER_WRAPPER_CIPHER_DES_EDE_ECB,      /**< DES cipher with EDE ECB mode. */
	CIPHER_WRAPPER_CIPHER_DES_EDE_CBC,      /**< DES cipher with EDE CBC mode. */
	CIPHER_WRAPPER_CIPHER_DES_EDE3_ECB,     /**< DES cipher with EDE3 ECB mode. */
	CIPHER_WRAPPER_CIPHER_DES_EDE3_CBC,     /**< DES cipher with EDE3 CBC mode. */
	CIPHER_WRAPPER_CIPHER_BLOWFISH_ECB,     /**< Blowfish cipher with ECB mode. */
	CIPHER_WRAPPER_CIPHER_BLOWFISH_CBC,     /**< Blowfish cipher with CBC mode. */
	CIPHER_WRAPPER_CIPHER_BLOWFISH_CFB64,   /**< Blowfish cipher with CFB64 mode. */
	CIPHER_WRAPPER_CIPHER_BLOWFISH_CTR,     /**< Blowfish cipher with CTR mode. */
	CIPHER_WRAPPER_CIPHER_ARC4_128,         /**< RC4 cipher with 128-bit mode. */
	CIPHER_WRAPPER_CIPHER_AES_128_CCM,      /**< AES cipher with 128-bit CCM mode. */
	CIPHER_WRAPPER_CIPHER_AES_192_CCM,      /**< AES cipher with 192-bit CCM mode. */
	CIPHER_WRAPPER_CIPHER_AES_256_CCM,      /**< AES cipher with 256-bit CCM mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_128_CCM, /**< Camellia cipher with 128-bit CCM mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_192_CCM, /**< Camellia cipher with 192-bit CCM mode. */
	CIPHER_WRAPPER_CIPHER_CAMELLIA_256_CCM, /**< Camellia cipher with 256-bit CCM mode. */
} cipher_wrapper_cipher_type_t;

/** Supported cipher modes. */
typedef enum {
	CIPHER_WRAPPER_MODE_NONE = 0,           /**< None. */
	CIPHER_WRAPPER_MODE_ECB,                /**< The ECB cipher mode. */
	CIPHER_WRAPPER_MODE_CBC,                /**< The CBC cipher mode. */
	CIPHER_WRAPPER_MODE_CFB,                /**< The CFB cipher mode. */
	CIPHER_WRAPPER_MODE_OFB,                /**< The OFB cipher mode - unsupported. */
	CIPHER_WRAPPER_MODE_CTR,                /**< The CTR cipher mode. */
	CIPHER_WRAPPER_MODE_GCM,                /**< The GCM cipher mode. */
	CIPHER_WRAPPER_MODE_STREAM,             /**< The stream cipher mode. */
	CIPHER_WRAPPER_MODE_CCM,                /**< The CCM cipher mode. */
} cipher_wrapper_cipher_mode_t;

/**
 * Base cipher information. The non-mode specific functions and values.
 */
typedef struct {
	/** Base Cipher type (e.g. MBEDTLS_CIPHER_ID_AES) */
	cipher_wrapper_cipher_id_t cipher;

	/** Encrypt using ECB */
	int (*ecb_func)( void *ctx, cipher_wrapper_operation_t mode,
			const unsigned char *input, unsigned char *output );

#if defined(CIPHER_WRAPPER_CIPHER_MODE_CBC)
	/** Encrypt using CBC */
	int (*cbc_func)( void *ctx, cipher_wrapper_operation_t mode, size_t length,
			unsigned char *iv, const unsigned char *input,
			unsigned char *output );
#endif

#if defined(CIPHER_WRAPPER_CIPHER_MODE_CFB)
	/** Encrypt using CFB (Full length) */
	int (*cfb_func)( void *ctx, cipher_wrapper_operation_t mode, size_t length, size_t *iv_off,
			unsigned char *iv, const unsigned char *input,
			unsigned char *output );
#endif

#if defined(CIPHER_WRAPPER_CIPHER_MODE_CTR)
	/** Encrypt using CTR */
	int (*ctr_func)( void *ctx, size_t length, size_t *nc_off,
			unsigned char *nonce_counter, unsigned char *stream_block,
			const unsigned char *input, unsigned char *output );
#endif

#if defined(CIPHER_WRAPPER_CIPHER_MODE_STREAM)
	/** Encrypt using STREAM */
	int (*stream_func)( void *ctx, size_t length,
			const unsigned char *input, unsigned char *output );
#endif

	/** Set key for encryption purposes */
	int (*setkey_enc_func)( void *ctx, const unsigned char *key,
			unsigned int key_bitlen );

	/** Set key for decryption purposes */
	int (*setkey_dec_func)( void *ctx, const unsigned char *key,
			unsigned int key_bitlen);

	/** Allocate a new context */
	void * (*ctx_alloc_func)(void);

	/** Free the given context */
	void (*ctx_free_func)( void *ctx );
} cipher_wrapper_cipher_base_t;

/**
 * The CMAC context structure.
 */
typedef struct {
	/** The internal state of the CMAC algorithm.  */
	unsigned char state[CIPHER_WRAPPER_CIPHER_BLKSIZE_MAX];

	/** Unprocessed data - either data that was not block aligned and is still
	 *  pending processing, or the final block. */
	unsigned char unprocessed_block[CIPHER_WRAPPER_CIPHER_BLKSIZE_MAX];

	/** The length of data pending processing. */
	size_t unprocessed_len;
} cipher_wrapper_cmac_context_t;

/**
 * Cipher information. Allows calling cipher functions
 * in a generic way.
 */
typedef struct {
	/** Full cipher identifier. For example,
	 * MBEDTLS_CIPHER_AES_256_CBC.
	 */
	cipher_wrapper_cipher_type_t type;

	/** The cipher mode. For example, MBEDTLS_MODE_CBC. */
	cipher_wrapper_cipher_mode_t mode;

	/** The cipher key length, in bits. This is the
	 * default length for variable sized ciphers.
	 * Includes parity bits for ciphers like DES.
	 */
	unsigned int key_bitlen;

	/** Name of the cipher. */
	const char *name;

	/** IV or nonce size, in Bytes.
	 * For ciphers that accept variable IV sizes,
	 * this is the recommended size.
	 */
	unsigned int iv_size;

	/** Bitflag comprised of MBEDTLS_CIPHER_VARIABLE_IV_LEN and
	 *  MBEDTLS_CIPHER_VARIABLE_KEY_LEN indicating whether the
	 *  cipher supports variable IV or variable key sizes, respectively.
	 */
	int flags;

	/** The block size, in Bytes. */
	unsigned int block_size;

	/** Struct for base cipher information and functions. */
	const cipher_wrapper_cipher_base_t *base;
} cipher_wrapper_cipher_info_t;

/**
 * Generic cipher context.
 */
typedef struct {
	/** Information about the associated cipher. */
	const cipher_wrapper_cipher_info_t *cipher_info;

	/** Key length to use. */
	int key_bitlen;

	/** Operation that the key of the context has been
	 * initialized for.
	 */
	cipher_wrapper_operation_t operation;

#if defined(CIPHER_WRAPPER_CIPHER_MODE_WITH_PADDING)

	/** Padding functions to use, if relevant for
	 * the specific cipher mode.
	 */
	void (*add_padding)( unsigned char *output, size_t olen, size_t data_len );
	int (*get_padding)( unsigned char *input, size_t ilen, size_t *data_len );
#endif

	/** Buffer for input that has not been processed yet. */
	unsigned char unprocessed_data[CIPHER_WRAPPER_MAX_BLOCK_LENGTH];

	/** Number of Bytes that have not been processed yet. */
	size_t unprocessed_len;

	/** Current IV or NONCE_COUNTER for CTR-mode. */
	unsigned char iv[CIPHER_WRAPPER_MAX_IV_LENGTH];

	/** IV size in Bytes, for ciphers with variable-length IVs. */
	size_t iv_size;

	/** The cipher-specific context. */
	void *cipher_ctx;

#if defined(CIPHER_WRAPPER_CMAC_C)
	/** CMAC-specific context. */
	cipher_wrapper_cmac_context_t *cmac_ctx;
#endif
} cipher_wrapper_cipher_context_t;

/**
 * \brief    The CCM context-type definition. The CCM context is passed
 *           to the APIs called.
 */
typedef struct {
	cipher_wrapper_cipher_context_t cipher_ctx;    /*!< The cipher context used. */
}
cipher_wrapper_ccm_context;

/* API functions to map on source file to mbedTLS or library used */
const cipher_wrapper_cipher_info_t *cipher_wrapper_cipher_info_from_type(const cipher_wrapper_cipher_type_t cipher_type);
int cipher_wrapper_cipher_setup(cipher_wrapper_cipher_context_t *ctx, const cipher_wrapper_cipher_info_t *cipher_info);
int cipher_wrapper_cipher_cmac_starts(cipher_wrapper_cipher_context_t *ctx, const unsigned char *key, size_t keybits);
int cipher_wrapper_cipher_cmac_update(cipher_wrapper_cipher_context_t *ctx, const unsigned char *input, size_t ilen);
int cipher_wrapper_cipher_cmac_finish(cipher_wrapper_cipher_context_t *ctx, unsigned char *output);
int cipher_wrapper_cipher_cmac_reset(cipher_wrapper_cipher_context_t *ctx);
void cipher_wrapper_cipher_free(cipher_wrapper_cipher_context_t *ctx);
void cipher_wrapper_ccm_init(cipher_wrapper_ccm_context *ctx);
int cipher_wrapper_ccm_setkey(cipher_wrapper_ccm_context *ctx, cipher_wrapper_cipher_id_t cipher,
		const unsigned char *key, unsigned int keybits);
int cipher_wrapper_ccm_auth_decrypt(cipher_wrapper_ccm_context *ctx, size_t length, const unsigned char *iv,
		size_t iv_len, const unsigned char *add, size_t add_len, const unsigned char *input,
		unsigned char *output, const unsigned char *tag, size_t tag_len);
int cipher_wrapper_ccm_encrypt_and_tag(cipher_wrapper_ccm_context *ctx, size_t length, const unsigned char *iv,
		size_t iv_len, const unsigned char *add, size_t add_len, const unsigned char *input,
		unsigned char *output, unsigned char *tag, size_t tag_len);
void cipher_wrapper_ccm_free(cipher_wrapper_ccm_context *ctx);

#if defined(__cplusplus)
}
#endif

#endif
