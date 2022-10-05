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
#include <stdarg.h>
#include <string.h>
#include <Logger.h>

#include <stdarg.h>
#include <stdio.h>

#ifdef LINUX
static int s_i_verbose = 0;
void LogEnable(int enable)
{
	s_i_verbose = enable;
}

#endif

void Log(const char *strFormat, ...)
{
#ifdef LINUX
	if (s_i_verbose == 0) {
		return;
	}

#endif

	va_list arglist;

	va_start(arglist, strFormat);
	vfprintf(stderr, strFormat, arglist);
	va_end(arglist);
}

void LogBuffer( const void *pBuffer, uint32_t u32Length, const char *strFormat, ...)
{
#ifdef LINUX
	if (s_i_verbose == 0) {
		return;
	}

#endif

	/* this method avoids using printf which is slow */
	static const char *HexBytes
		=
			"000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5F606162636465666768696A6B6C6D6E6F707172737475767778797A7B7C7D7E7F808182838485868788898A8B8C8D8E8F909192939495969798999A9B9C9D9E9FA0A1A2A3A4A5A6A7A8A9AAABACADAEAFB0B1B2B3B4B5B6B7B8B9BABBBCBDBEBFC0C1C2C3C4C5C6C7C8C9CACBCCCDCECFD0D1D2D3D4D5D6D7D8D9DADBDCDDDEDFE0E1E2E3E4E5E6E7E8E9EAEBECEDEEEFF0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF";
	const uint8_t *pu8Data = (const uint8_t *)pBuffer;

	va_list vaArgs;
	uint32_t u32Index;

	va_start(vaArgs, strFormat);
	vprintf(strFormat, vaArgs);
	va_end(vaArgs);

	for (u32Index = 0; u32Index < u32Length; u32Index++) {
		fputc(HexBytes[2 * pu8Data[u32Index]], stderr);
		fputc(HexBytes[2 * pu8Data[u32Index] + 1], stderr);
	}
}

void LogAssert(const char *file, uint32_t line, const char *condition)
{
#ifdef LINUX
	if (s_i_verbose == 0) {
		return;
	}

#endif
	Log("FATAL %s:%d %s\r\n", file, line, condition);
}

/* this function returns only the file name from full path */
const char *LogGetFileName(const char *fullpath)
{
	const char *ret = fullpath;
	int n;

	int nLen = strlen(fullpath);
	for (n = nLen - 1; n >= 0; n--) {
		if ((fullpath[n] == '\\') || (fullpath[n] == '/')) {
			ret = &fullpath[n + 1];
			break;
		}
	}

	return(ret);
}

/**********************************************************************************************************************/

/** @}
 **********************************************************************************************************************/
