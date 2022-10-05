#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <syslog.h>

#define PRINT_ERROR 0
#define PRINT_WARN  1
#define PRINT_INFO  2
#define PRINT_ALL   3

void PRINTF(int level, const char *format, ...);

typedef struct {
	int i_verbose;
	int i_verbose_level;
	char sz_host_name[255];
	unsigned ui_port;
	char sz_file_bin[255];
} td_x_args;

#endif
