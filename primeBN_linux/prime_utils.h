/**
 * \file
 *
 * \brief PRIME Utils
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

#ifndef _PRIME_UTILS_H_
#define _PRIME_UTILS_H_

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

/***********************************************************
*       Includes                                           *
************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <prime_api_defs_host.h>

/***********************************************************
*       Defines                                            *
************************************************************/
#define EUI48_LEN 6
#define DUK_LEN  16

/***********************************************************
*       Functions                                          *
************************************************************/

/*
 * \brief Convert Hexadecimal to Integer
 * \param *s    pointer to hexadecimal vector
 * \return      integer
 */
int Hex2Int(uint8_t *s);

/*
 *  \brief Convert string to EUI48 address
 *
 *  \param eui48   -> pointer to EUI48 destination
 *  \param str     -> pointer to String source
 *  \return           pointer to EUI48 destination
 */
int32_t str_to_eui48(char* str, uint8_t * eui48);

/*
 *  \brief Convert string to DUK
 *
 *  \param duk     -> pointer to DUK destination
 *  \param str     -> pointer to String source
 *  \return           pointer to DUK destination
 */
int32_t str_to_duk(char* str, uint8_t * duk);

/*
 *  \brief Convert EUI48 address to string - No memory check
 *  \param EUI48   -> pointer to EUI48 source
 *  \param str     -> pointer to String destination
 *  \return           pointer to String destination
 */
char * eui48_to_str(const unsigned char* EUI48, char * str);

/*
 *  \brief Convert FU State to string - No memory check
 *  \param fu_state -> FU State
 *  \param str      -> pointer to String destination
 *  \return            pointer to String destination
 */
char * fup_state_to_str(const uint8_t fu_state, char * str);

/*
 *  \brief Convert FU Exception State to string - No memory check
 *  \param fu_exception_state -> FU Exception State
 *  \param str                -> pointer to String destination
 *  \return                      pointer to String destination
 */
char * fup_state_exception_to_str(const uint16_t fu_exception_state, char * str);

/*
 *  \brief Convert Network Event to string - No memory check
 *  \param netwqork_event -> Network Event
 *  \param str            -> pointer to String destination
 *  \return                  pointer to String destination
 */
char * network_event_to_str(const uint8_t network_event, char * str);

/*
 * \brief Check if TCP port is available
 *
 * \param tcp_port  TCP Port to be checked
 * \return
 */
int prime_tcp_check_port(int32_t tcp_port);

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */

#endif  // _PRIME_UTILS_H
