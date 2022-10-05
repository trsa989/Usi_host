/**
 * \file
 *
 * \brief USI Host
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

#include "../addUsi.h"
#include "Usi.h"

#include <time.h>
#include <unistd.h>

/**
 * @brief addUsi_Init
 * Use this function to initialize the USI and start it.
 */
void addUsi_Init(void)
{
	usi_Init();
	usi_Start();
}

/**
 * @brief addUsi_Process
 * Use this function to process USI messages in TX and RX
 * Call this function periodically
 */
void addUsi_Process(void)
{
	usi_RxProcess();
	usi_TxProcess();
}

/**
 * @brief addUsi_WaitProcessing
 * Use this function waiting for syncronous requests
 * @param seconds: Seconds waiting for syncronous flag
 * @param flag: Syncronous Flag pointer
  */
void addUsi_WaitProcessing(uint8_t seconds, Bool *flag)
{
#ifdef __linux__
	static time_t start_t;
	static time_t end_t;
	double diff_t = 0;

	time(&start_t);
	/* Wait for the defined time, or until the referenced flag activates */
	while ((diff_t < seconds) && !*flag){
		time(&end_t);
		/* seconds_elapsed = ((end-start)/CLOCKS_PER_SEC); */
		diff_t = difftime(end_t, start_t);
		/* addUsi_Process();  / * Running on thread or ualarm * / */
		/* 1ms waiting - Same timing than usiProcess() running on thread or ualarm */
		usleep(1000);
	}
#endif
}

/**
 * @brief addUsi_ConfigurePort
 * Use this function for Port Configuring
 * @param logPort: Log Port
 * @param port_type: Port Type
 * @param port_chn: Port Channel
 * @param port_speed: Port Speed
 */
void addUsi_ConfigurePort(uint8_t logPort, uint8_t port_type, uint8_t port_chn, uint32_t port_speed)
{
	usi_ConfigurePort(logPort, port_type, port_chn, port_speed);
}

/* / @cond 0 */
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/* / @endcond */