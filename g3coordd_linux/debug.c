#include <stdio.h>
#include <time.h>
#include "debug.h"
#include <string.h>


char filename[30];
FILE *file_debug_ptr;

void open_and_create_log_file()
{
    static unsigned int flag = 0;
    
    
    if(flag == 0)
    {
        flag = 1;
        struct tm *timenow;  
        time_t now = time(NULL);
        timenow = gmtime(&now);

        strftime(filename,30,"%Y-%m-%d %H:%M:%S.log", timenow);
        
    }   
    file_debug_ptr = fopen(filename, "ab");
    if(file_debug_ptr == NULL)
        printf("ERROR > Create log file.");
   
}
void close_log_file(void)
{
    fclose(file_debug_ptr);
}

char * timestamp_log()
{   
	time_t now = time (NULL);
	char * time = asctime(gmtime(&now));
	time[strlen(time)-1] = '\0';
 	return time;
}

FILE * get_file_debug_ptr(void)
{
    return file_debug_ptr;
}