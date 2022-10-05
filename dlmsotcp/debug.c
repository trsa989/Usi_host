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

#include "debug.h"
#include <stdio.h>
#include <stdarg.h>

#include "dlmsotcp.h"

extern td_x_args g_x_args;

void PRINTF(int level, const char *format, ...)
{
	char szStr[500];

	if ((g_x_args.i_verbose != 0) && (level <= g_x_args.i_verbose_level)) {
		va_list argptr;
		va_start(argptr, format);
		vsprintf(szStr, format, argptr);
		fprintf(stderr, "%s", szStr);
		va_end(argptr);
	}

	if (level == PRINT_ERROR) {
		va_list argptr;
		va_start(argptr, format);
		vsprintf(szStr, format, argptr);
		/* fprintf(stderr,"%s", szStr); */
		va_end(argptr);
		syslog(LOG_ERR, "%s", szStr);
	}
}
