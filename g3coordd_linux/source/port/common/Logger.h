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

/** This file contains declarations of the logger.
 ***********************************************************************************************************************
 *
 * In each file (.c) using the logger, the logger header has to be included. Before including it, the log level for
 * the file has to be defined. Valid log levels are @ref LOG_LVL_NONE, @ref LOG_LVL_FATAL, @ref LOG_LVL_ERR,
 * @ref LOG_LVL_INFO and @ref LOG_LVL_DBG. For example:
 *
 * @code
 * #define LOG_LEVEL LOG_LVL_ERR
 * #include <common/Logger.h>
 * @endcode
 *
 * Log messages can be output with the logging macros @ref LOG_ERR, @ref LOG_PRIO, @ref LOG_INFO and @ref LOG_DBG:
 *
 * @code
 * LOG_DBG(Log("Example log with a value: %u", u16Value));
 * @endcode
 *
 * To output a buffer in hex, use LogBuffer() instead of Log():
 *
 * @code
 * LOG_DBG(LogBuffer(pBuffer, u16Length, "Buffer with length: %u. Data: ", u16Length));
 * @endcode
 *
 * @file
 *
 **********************************************************************************************************************/

#ifndef __LOGGER_H__
#define __LOGGER_H__

#ifdef LINUX
void LogEnable(int enable);
#endif
/**********************************************************************************************************************/

/** This function outputs the provided format string and arguments into the log.
 ***********************************************************************************************************************
 * @param level Verbose output level.
 * @param strFormat The format string in printf() style.
 * @param ... Arguments of the format string.
 *
 **********************************************************************************************************************/

void Log(const char *strFormat, ...);
/**********************************************************************************************************************/

/** This function outputs into the log the buffer (in hex) and the provided format string and arguments.
 ***********************************************************************************************************************
 * @param level Verbose output level.
 * @param pBuffer Buffer to be output into the logs.
 * @param u32Length Length of the buffer, in bytes.
 * @param strFormat The format string in printf() style.
 * @param ... Arguments of the format string.
 *
 **********************************************************************************************************************/
void LogBuffer(const void *pBuffer, uint32_t u32Length, const char *strFormat, ...);
/**********************************************************************************************************************/

/** This function is called if the condition in LOG_ASSERT is not met, it outputs a corresponding LOG_LVL_FATAL message.
 ***********************************************************************************************************************
 * @param file Source file name.
 * @param line Line number.
 * @param condition Checked condition.
 *
 **********************************************************************************************************************/
void LogAssert(const char *file, uint32_t line, const char *condition);

/**********************************************************************************************************************/

/** This function is a logger helper used to extract the file name from full path
 * It is mainly used to process the __FILE__ macro output
 ***********************************************************************************************************************
 *
 * @param fullpath path file (should work also if the path is not 'full'
 *
 **********************************************************************************************************************/
const char *LogGetFileName(const char *fullpath);

/*  #define NDEBUG */

/* / Log level meaning that no log messages are output. */
  #define LOG_LVL_NONE      0
/* / Log level meaning that messages generated by LOG_ASSERT are output. */
  #define LOG_LVL_FATAL     1
/* / Log level meaning that messages generated by LOG_ASSERT, LOG_ERR, LOG_PRIO are output. */
  #define LOG_LVL_ERR       2
/* / Log level meaning that messages generated by LOG_ASSERT, LOG_ERR, LOG_PRIO, LOG_INFO are output. */
  #define LOG_LVL_INFO      3
/* / Log level meaning that all messages are output (LOG_ASSERT, LOG_ERR, LOG_PRIO, LOG_INFO and LOG_DBG). */
  #define LOG_LVL_DBG       4

#define LOG_LEVEL_ADP LOG_LVL_ERR

  #ifndef NDEBUG
extern uint32_t oss_get_up_time_ms(void);
  #endif

#endif

/* No protection against multiple inclusion is required from this point. */

/* Use a default setting if nobody defined a log level */
#ifndef LOG_LEVEL
/* #define LOG_LEVEL LOG_LVL_NONE */
/* #define LOG_LEVEL LOG_LVL_ERR */
/* #define LOG_LEVEL LOG_LVL_INFO */
#define LOG_LEVEL LOG_LVL_DBG
#endif

#undef LOG_ASSERT
#undef LOG_ERR
#undef LOG_PRIO
#undef LOG_INFO
#undef LOG_DBG

#if (LOG_LEVEL >= LOG_LVL_FATAL) && !defined(NDEBUG)
/* / Checks the condition, and if it is not fulfilled, a corresponding fatal message is output. */
  #define LOG_ASSERT(condition)	\
	((condition) ? (void)0 : \
	LogAssert( \
	LogGetFileName(__FILE__), __LINE__, # condition))
#else
  #define LOG_ASSERT(condition)     /* Nothing */
#endif

#if (LOG_LEVEL >= LOG_LVL_ERR) && !defined(NDEBUG)
/* / Outputs an error message. */
  #define LOG_ERR(expression) \
	{Log("\r\n%08X ERR   %s:%d ", oss_get_up_time_ms(), LogGetFileName(__FILE__), __LINE__); expression; }
/* / Outputs a high priority information message (log level LOG_LVL_ERR). */
  #define LOG_PRIO(expression) \
	{Log("\r\n%08X PRIO  %s:%d ", oss_get_up_time_ms(), LogGetFileName(__FILE__), __LINE__); expression; }
#else
  #define LOG_ERR(expression)    /* Nothing */
  #define LOG_PRIO(expression)    /* Nothing */
#endif

#if (LOG_LEVEL >= LOG_LVL_INFO) && !defined(NDEBUG)
/* / Outputs an information message. */
#pragma message "LOG_INFO enabled"
  #define LOG_INFO(expression) \
	{Log("\r\n%08X INFO  %s:%d ", oss_get_up_time_ms(), LogGetFileName(__FILE__), __LINE__); expression; }
#else
  #define LOG_INFO(expression)     /* Nothing */
#endif

#if (LOG_LEVEL >= LOG_LVL_DBG) && !defined(NDEBUG)
/* / Outputs a debug message. */
#pragma message "LOG_DBG enabled"
  #define LOG_DBG(expression) \
	{Log("\r\n%08X DBG   %s:%d ", oss_get_up_time_ms(), LogGetFileName(__FILE__), __LINE__); expression; }
#else
	#pragma message "ARG, no debug"
  #define LOG_DBG(expression)             /* Nothing */
#endif

/**********************************************************************************************************************/

/** @}
 **********************************************************************************************************************/
