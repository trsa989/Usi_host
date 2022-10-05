#ifndef _debug_H_
#define _debug_H_


#define DEBUG_IN_FILE



void open_and_create_log_file(void);
void close_log_file(void);
char * timestamp_log(void);
FILE * get_file_debug_ptr(void);

#endif // debugH