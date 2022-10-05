/**
 * \file
 *
 * \brief PRIME Log Header.
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

#ifndef _PRIME_LOG_H_
#define _PRIME_LOG_H_

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/* / @endcond */

/***********************************************************
*       Include                                            *
************************************************************/
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
/***********************************************************
*       External Vars                                      *
************************************************************/
extern FILE * prime_logfile_fd;

/***********************************************************
*       Defines                                            *
************************************************************/
//#define PRIME_PRINTF(msj...)		      ({ struct timeval tv; gettimeofday(&tv, NULL); fprintf(prime_logfile_fd, "[%010ld.%06ld-%s] ", tv.tv_sec, tv.tv_usec,__FILE__); fprintf(prime_logfile_fd, msj); fflush(prime_logfile_fd); })
#define PRIME_PRINTF(msj...)		      ({ struct timeval tv; gettimeofday(&tv, NULL); prime_log_mutex_lock(); fprintf(prime_logfile_fd, "[%010ld.%06ld] ", tv.tv_sec, tv.tv_usec); fprintf(prime_logfile_fd, msj); fflush(prime_logfile_fd); prime_log_mutex_unlock(); })
#define PRIME_PRINTF_NOSTAMP(msj...)	({ prime_log_mutex_lock(); fprintf(prime_logfile_fd,msj); fflush(prime_logfile_fd); prime_log_mutex_unlock(); })
#define PRIME_PRINTF_PERROR(msj...)   ({ prime_log_mutex_lock(); perror(msj);fprintf(prime_logfile_fd,"errno = %d\n",errno); prime_log_mutex_unlock(); })

#define PRIME_LOG_NONE		  0
#define PRIME_LOG_ERR		    1
#define PRIME_LOG_ERROR     1
#define PRIME_LOG_WARN		  2
#define PRIME_LOG_INFO		  3
#define PRIME_LOG_DEBUG		  4
#define PRIME_LOG_DBG		    4

#define PRIME_LOG(lvl, msj...)		     if ((PRIME_##lvl <= prime_get_loglevel()) && prime_get_log()) PRIME_PRINTF(msj);
#define PRIME_LOG_NOSTAMP(lvl, msj...) if ((PRIME_##lvl <= prime_get_loglevel()) && prime_get_log()) PRIME_PRINTF_NOSTAMP(msj);
#define PRIME_LOG_PERROR(lvl, msj...)	 if ((PRIME_##lvl <= prime_get_loglevel()) && prime_get_log()) PRIME_PRINTF_PERROR(msj);

#define PRIME_LOGFILE_DEFAULT "/tmp/prime.log"

/*
 * \brief  	    Enable PRIME Log
 * \return 0
 */
int prime_enable_log();

/*
 * \brief  	    Disable PRIME Log
 * \return 0
 */
int prime_disable_log();

/*
 * \brief  	 Modificacion global del loglevel
 * \param [in]  int loglevel -> Loglevel global del demonio
 * \return 0
 */
int prime_set_loglevel(int loglevel);

/*
 * \brief  Get log Status
 * \return Enabled/Disabled
 */
int prime_get_log();

/*
 * \brief  Get loglevel
 * \return loglevel
 */
int prime_get_loglevel();

/*
 * \brief  	 Set Logfile
 * \param [in]  char * filename
 * \return 0
 */
int prime_set_logfile(char * filename);

/*
 * \brief PRIME DLMSoTCP Mutex Lock
 * \return
 */
int prime_log_mutex_lock();

/*
 * \brief PRIME DLMSoTCP Mutex Unlock
 * \return
 */
int prime_log_mutex_unlock();

#endif

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
