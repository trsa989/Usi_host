/**
 * \file
 *
 * \brief PRIME Log file.
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

/************************************************************
*       Includes                                            *
*************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include "prime_log.h"

/************************************************************
*       Global Vars                                         *
*************************************************************/
static int prime_loglevel = PRIME_LOG_ERR;
static int prime_global_log = false;
FILE * prime_logfile_fd;
/* Mutex for accessing PRIME Log */
pthread_mutex_t prime_log_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * \brief  	 Get PRIME Log Status
 * \return   TRUE if enabled, FALSE if disabled
 */
int prime_get_log(){
   return prime_global_log;
}

/*
 * \brief  	    Enable PRIME Log
 * \return 0
 */
int prime_enable_log(){
  prime_global_log = true;
	return 0;
}

/*
 * \brief  	    Disable PRIME Log
 * \return 0
 */
int prime_disable_log(){
  prime_global_log = false;
	return 0;
}

/*
 * \brief  	    Set PRIME Loglevel
 *
 * \param [in]  int loglevel -> global loglevel
 * \return 0
 */
int prime_set_loglevel(int loglevel){
    prime_loglevel = loglevel;
    //prime_sniffer_loglevel(loglevel);
    //prime_dlmsovertcp_loglevel(loglevel);
	return 0;
}

/*
 * \brief  Get Loglevel
 *
 * \return loglevel
 */
int prime_get_loglevel(){
	return prime_loglevel;
}

/*
 * \brief  	  Set File Log Name
 *
 * \param [in] char * filename
 * \return 0
 */
int prime_set_logfile(char * filename)
{
/**********************************************************
*       Local Vars                                        *
***********************************************************/
/**********************************************************
*       Code                                              *
***********************************************************/
    PRIME_LOG(LOG_INFO,"Setting Global Logfile to %s\r\n",filename);
    if (prime_logfile_fd != NULL)
       fclose(prime_logfile_fd);

    prime_logfile_fd = fopen(filename, "a");
	  if (prime_logfile_fd == NULL){
		   /// STDERR si falla
		   prime_logfile_fd = stderr;
	  }
    return 0;
}

/*
 * \brief PRIME LOG Mutex Lock
 * \return
 */
int prime_log_mutex_lock()
{
    return pthread_mutex_lock(&prime_log_mutex);
}

/*
 * \brief PRIME DLMSoTCP Mutex Unlock
 * \return
 */
int prime_log_mutex_unlock()
{
    return pthread_mutex_unlock(&prime_log_mutex);
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */
