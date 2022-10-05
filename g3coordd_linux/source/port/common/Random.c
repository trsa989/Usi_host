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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* #include <common/Random.h> */
/* #include <common/include/Byte.h> */
#include <Random.h>

static int rand_dev = -1;
/**********************************************************************************************************************/

/**
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
int Random_Initialize()
{
	/* do nothing */
	rand_dev = open("/dev/urandom", O_RDONLY);
	return rand_dev;
}

/**********************************************************************************************************************/

/**
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
uint16_t Random16(void)
{
	if (rand_dev >= 0) {
		uint16_t tmp;
		read(rand_dev, &tmp, sizeof(uint16_t));
		return tmp;
	} else {
		return rand() && 0xFFFF;
	}
}

/**********************************************************************************************************************/

/**
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
uint16_t Random16Ex(
		uint16_t u16Min,
		uint16_t u16Max
		)
{
	uint16_t u16LocalMin = u16Min;

	if (u16Max < u16Min) {
		u16Max = u16Min;
		u16LocalMin = u16Max;
	}

	return Random16() % (u16Max - u16LocalMin + 1) + u16LocalMin;
}

/**********************************************************************************************************************/

/**
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
uint32_t Random32(void)
{
	if (rand_dev >= 0) {
		uint32_t tmp;
		read(rand_dev, &tmp, sizeof(uint32_t));
		return tmp;
	} else {
		return rand();
	}
}

/**********************************************************************************************************************/

/**
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
uint32_t Random32Ex(
		uint32_t u32Min,
		uint32_t u32Max
		)
{
	uint32_t u32LocalMin = u32Min;

	if (u32Max < u32Min) {
		u32Max = u32Min;
		u32LocalMin = u32Max;
	}

	return Random32() % (u32Max - u32LocalMin + 1) + u32LocalMin;
}

/**********************************************************************************************************************/

/**
 ***********************************************************************************************************************
 *
 **********************************************************************************************************************/
void Random128(
		uint8_t au8Value[16]
		)
{
	if (rand_dev >= 0) {
		read(rand_dev, au8Value, 16);
	} else {
		uint32_t tmp;
		uint8_t n = 0;

		for (n = 0; n < 16; n += sizeof(uint32_t)) {
			tmp = rand();
			memcpy(au8Value + n, &tmp, sizeof(uint32_t));
		}
	}
}
