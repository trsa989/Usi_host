/**
 * \file
 *
 * \brief PRIME Sniffer Header
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

#ifndef _PRIME_SNIFFER_H
#define _PRIME_SNIFFER_H

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

#define SNIFFER_FLAG_ENABLE  0x00000001
#define SNIFFER_FLAG_LOGFILE 0x00000002
#define SNIFFER_FLAG_SOCKET  0x00000004

#define PRIME_SNIFFER_LOG(lvl, msj...)		     if ((PRIME_##lvl <= prime_sniffer_get_loglevel()) && prime_get_log()) PRIME_PRINTF(msj);
#define PRIME_SNIFFER_LOG_NOSTAMP(lvl, msj...) if ((PRIME_##lvl <= prime_sniffer_get_loglevel()) && prime_get_log()) PRIME_PRINTF_NOSTAMP(msj);

/*
 * \brief  Set Sniffer Loglevel
 *
 * \param [in]  int loglevel
 * \return 0
 */
int prime_sniffer_set_loglevel(int loglevel);

/*
 * \brief  Get Sniffer Loglevel
 * \return loglevel
 */
int prime_sniffer_get_loglevel();

/*
 * \brief  	   Set PRIME Sniffer Flags
 *
 * \param [in]  int PRIME Sniffer flags
 * \return 0
 */
int prime_sniffer_set_flags(int flags);

/*
 * \brief  Get PRIME Sniffer Flags
 *
 * \return Snifer Flags
 */
int prime_sniffer_get_flags();

/*
 * \brief Starts Sniffer capabilities
 *
 * \param flags: Sniffer flags
 * \return
 */
int prime_sniffer_start(uint32_t flags);

/*
 * \brief Stops Sniffer capabilities
 *
 * \param flags  PRIME sniffer flags
 * \return
 */
int prime_sniffer_stop(uint32_t flags);

/*
 * \brief PRIME Sniffer Process Callback
 *
 * \param msg:  Sniffer Message
 * \param len:  Sniffer Message Lenght
 * \return
 */
void prime_sniffer_process(uint8_t* msg, uint16_t len);

/*
 * \brief Set TCP Port for Sniffer
 *
 * \param tcp_port TCP Port to be set
 * \return
 */
int32_t prime_sniffer_tcp_set_port(int32_t tcp_port);

/*
 * \brief Get TCP Port for Sniffer
 *
 * \param tcp_port to be got
 * \return
 */
int32_t prime_sniffer_tcp_get_port();

/*
 * \brief Thread to handle connections to TCP Sniffer
 *
 * \param  thread_parameters
 * \return
 */
void * prime_sniffer_tcp_thread(void * thread_parameters);

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */

#endif /* PRIME_SNIFFER_H */
