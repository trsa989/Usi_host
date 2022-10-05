/**
 * \file
 *
 * \brief Configuration File
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

#ifndef CONF_GLOBAL_H_INCLUDED
#define CONF_GLOBAL_H_INCLUDED

#define ENABLE_ROUTING 1
/* Define spec compiance, select one: 17 or 15 (spec'17 or spec'15) */
#define SPEC_COMPLIANCE  17
/* #define SPEC_COMPLIANCE  15 */

#ifdef SPEC_COMPLIANCE
	#if (SPEC_COMPLIANCE == 17) || (SPEC_COMPLIANCE == 15)
/* Correct definition */
	#else
		#error "SPEC_COMPLIANCE not correctly defined, 17 or 15 are the allowed values"
	#endif
#else
	#error "Undefined SPEC_COMPLIANCE"
#endif

#endif /* CONF_GLOBAL_H_INCLUDED */
