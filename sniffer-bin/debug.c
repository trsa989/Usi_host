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
