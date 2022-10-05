/**
 * \file
 *
 * \brief Common Utils
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

/**********************************************************************************************************************/

/** This file contains functions declaration used for pseudo-random numbers generation
 ***********************************************************************************************************************
 *
 * @file
 *
 **********************************************************************************************************************/

#ifndef __RANDOM_H__
#define __RANDOM_H__

/**********************************************************************************************************************/

/**
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
int Random_Initialize(void);

/**********************************************************************************************************************/

/** The Random16 function returns a 16bits random value
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
uint16_t Random16(void);

/**********************************************************************************************************************/

/** The Random16 function returns a random value between limits
 ***********************************************************************************************************************
 *
 * @param u16Min min limit
 * @param u16Max max limit
 *
 **********************************************************************************************************************/
uint16_t Random16Ex(
		uint16_t u16Min,
		uint16_t u16Max
		);

/**********************************************************************************************************************/

/** The Random32 function returns a 32bits random value
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
uint32_t Random32(void);
#define platform_random_32() Random32()
/**********************************************************************************************************************/

/** The Random32 function returns a random value between limits
 ***********************************************************************************************************************
 *
 * @param u32Min min limit
 * @param u32Max max limit
 *
 **********************************************************************************************************************/
uint32_t Random32Ex(
		uint32_t u32Min,
		uint32_t u32Max
		);

/**********************************************************************************************************************/

/** The Random128 function returns a 128bits random value
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
void Random128(
uint8_t au8Value[16]
);

#endif

/**********************************************************************************************************************/

/** @}
 **********************************************************************************************************************/
