/**
 * \file
 *
 * \brief CL_432_DEFS: CONV 432 layer
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
 
#ifndef CL_432_DEFS_H_INCLUDE
#define CL_432_DEFS_H_INCLUDE

#include <stdint.h>

/* @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* @endcond */

/**
 * \ingroup prime_ng_group
 * \defgroup prime_sscs_432_group PRIME SSCS 4-32
 *
 * This module provides configuration and utils for the
 * SSCS 4-32 in PRIME.
 *
 * @{
 */

/** Length of the LPDU */
#define LPDU_HEADER                    3

/** \brief LSDU Data length */
/** \note It must be smaller than PRIME_MACSAP_DATA_SIZE */
/* @{ */
#define MAX_LENGTH_432_DATA            (1024 - LPDU_HEADER)
/* @} */

/** RESULTS values for convergence layer primitives */
typedef enum {
	DL_432_RESULT_SUCCESS = 0,
	DL_432_RESULT_REJECT = 1,
	DL_432_RESULT_TIMEOUT = 2,
	DL_432_RESULT_NOT_REGISTERED = 6
} dl_432_result_t;

/** LSDU part of DL message */
#define lsdu                           dl.buff

/** Buffer defined for reception/transmission */
typedef union {
	uint8_t lpdu[MAX_LENGTH_432_DATA + LPDU_HEADER];

	struct {
		uint8_t control;
		uint8_t dsap;
		uint8_t lsap;
		uint8_t buff[MAX_LENGTH_432_DATA];
	} dl;
} dl_432_buffer_t;

/** \brief Addresses defined in 432 layer */
/* @{ */
#define CL_432_INVALID_ADDRESS                 (0xFFFF)
#define CL_432_BROADCAST_ADDRESS               (0x0FFF)
/* @} */

/** \brief Transmission errors defined in 432 layer */
/* @{ */
#define CL_432_TX_STATUS_SUCCESS  0
#define CL_432_TX_STATUS_TIMEOUT  2
#define CL_432_TX_STATUS_ERROR_BAD_ADDRESS     (0xC0)
#define CL_432_TX_STATUS_ERROR_BAD_HANLDER     (0xC1)
#define CL_432_TX_STATUS_PREVIOUS_COMM         (0xC2)
/* @} */

/* @} */

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
#endif
