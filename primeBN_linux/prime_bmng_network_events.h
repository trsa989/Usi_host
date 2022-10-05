/**
 * \file
 *
 * \brief PRIME Base Management Network Events.
 *
 * Copyright (c) 2020 Microchip Technology Inc. and its subsidiaries.
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

#ifndef _PRIME_NETWORK_EVENTS_H_
#define _PRIME_NETWORK_EVENTS_H_

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

typedef enum {
	NEW_ENTRY,
  DB_REMOVE
} network_events_db_cmds;

#define PRIME_NETWORK_EVENTS_LOG(lvl, msj...)		     if ((PRIME_##lvl <= prime_bmng_network_event_get_loglevel()) && prime_get_log()) PRIME_PRINTF(msj);
#define PRIME_NETWORK_EVENTS_LOG_NOSTAMP(lvl, msj...) if ((PRIME_##lvl <= prime_bmng_network_event_get_loglevel()) && prime_get_log()) PRIME_PRINTF_NOSTAMP(msj);

/***********************************************************
*       Functions                                          *
************************************************************/
/*
 * \brief  Get PRIME Network Management Loglevel
 *
 * \return loglevel
 */
int prime_bmng_network_event_get_loglevel();

/*
 * \brief  	   Set PRIME Network Management Loglevel
 *
 * \param [in]  int loglevel
 * \return 0
 */
int prime_bmng_network_event_set_loglevel(int loglevel);

/*
 * \brief PRIME BMNG Network Events Mutex Lock
 * \return
 */
int prime_bmng_network_events_mutex_lock();

/*
 * \brief PRIME BMNG Network Events Mutex Unlock
 * \return
 */
int prime_bmng_network_events_mutex_unlock();

/**
* \brief   Base Management Callback Network Event Indication
*          Asyncronous events from PRIME Network - Cqan interfere normal use of USI Interface
* \param   px_net_event: event
*/
void prime_bmng_network_event_msg_cb(bmng_net_event_t *px_net_event);

#endif  // _PRIME_NETWORK_EVENTS_DB_H
