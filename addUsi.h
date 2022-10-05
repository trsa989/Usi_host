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

#ifndef addUsih
#define addUsih

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
/* #include <stdbool.h> */
/* *** Declarations ********************************************************** */

#ifndef true
#define true            1
#endif

#ifndef false
#define false           0
#endif

#ifndef NULL
#define NULL            0
#endif

#ifndef TRUE
#define TRUE            true
#endif

#ifndef FALSE
#define FALSE           false
#endif

#ifndef NULL
#define NULL            ((void *)0L)
#endif

#define EXTERN  extern
#define KC51_CODE const
#define KC51_REENT

/* --- Types */
typedef int8_t Int8;
typedef uint8_t Uint8;
typedef int16_t Int16;
typedef uint16_t Uint16;
typedef int32_t Int32;
typedef uint32_t Uint32;
typedef uint8_t Bool;

/* *** Functions prototypes ************************************************** */

/* Control functions */

void addUsi_Init(void);

void addUsi_Process(void);

void addUsi_WaitProcessing(uint8_t seconds, Bool *flag);

void addUsi_ConfigurePort(uint8_t logPort, uint8_t port_type, uint8_t chn, uint32_t speed);

#ifdef __cplusplus
}
#endif

#endif
