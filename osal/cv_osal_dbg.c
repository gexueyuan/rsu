/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cv_osal_debug.c
 @brief  : This is the global debug realization.
 @author : wangyf
 @history:
           2014-11-13    wangyf    Created file
           ...
******************************************************************************/
#include "cv_osal.h"
#include "cv_osal_dbg.h"

static int cv_debug_syslog = 0;
static int cv_debug_timestamp = 0;

void cv_debug_print_timestamp(void)
{
	struct os_reltime tv;

	if (!cv_debug_timestamp)
		return;

	os_get_reltime(&tv);
	printf("(%ld.%06u): ", (long) tv.sec, (unsigned int) tv.usec);
}

void cv_debug_open_syslog(void)
{
	openlog("cv_rsu", LOG_PID | LOG_NDELAY, LOG_USER);
	cv_debug_syslog++;
}


void cv_debug_close_syslog(void)
{
	if (cv_debug_syslog)
		closelog();
}


static int syslog_priority(int level)
{
	switch (level) {
	case OSAL_DEBUG_LOUD:
	case OSAL_DEBUG_TRACE:
		return LOG_DEBUG;
	case OSAL_DEBUG_INFO:
		return LOG_NOTICE;
	case OSAL_DEBUG_WARN:
		return LOG_WARNING;
	case OSAL_DEBUG_ERROR:
		return LOG_ERR;
	}
	return LOG_INFO;
}

void osal_printf(const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

    cv_debug_print_timestamp();
	vprintf(fmt, ap);

	va_end(ap);  
		
	fflush(stdout); 
}

void osal_syslog(int level, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
    
	vprintf(fmt, ap);    
	if (cv_debug_syslog) {
		vsyslog(syslog_priority(level), fmt, ap);
	} 
	va_end(ap);   
}

void osal_printf_unbuf(const char * fmt, ...)
{
    uint8_t buffer[1000];
    va_list args;
    
    va_start(args, fmt);
    memset(buffer, 0, sizeof(buffer));
    vsprintf((char *)buffer, fmt, args);
    va_end(args);
	cv_debug_print_timestamp();   
	(void)fprintf(stderr, "%s", buffer);
}


#if (OSAL_GLOBAL_DEBUG)
/*****************************************************************************
 * declaration of variables and functions                                    *
*****************************************************************************/
/**
 *  [block begin] Debug level configuration
 *  You can add your module here and DONOT modify any other code in the block.
 */
const debug_entry_t debug_entry_table_start = {NULL, NULL};
OSAL_DEBUG_ENTRY_DECLARE(sysc)
OSAL_DEBUG_ENTRY_DECLARE(vam)
OSAL_DEBUG_ENTRY_DECLARE(rcp)
OSAL_DEBUG_ENTRY_DECLARE(wnet)
// add your module here...

const debug_entry_t debug_entry_table_end = {NULL, NULL};

int g_dbg_print_type = 1;


/*****************************************************************************
 * implementation of functions                                               *
*****************************************************************************/
int osal_dbg_set_level(char *module, int level)
{
	debug_entry_t *cmdtp;

	/* Search command table - Use linear search - it's a small table */
	for (cmdtp = (debug_entry_t *)(&debug_entry_table_start + 1); cmdtp->module; cmdtp++) {
		if (strcmp (module, cmdtp->module) == 0) {
		    if (cmdtp->handler){
                (cmdtp->handler)(level);
                return TRUE;
		    }
		}
	}
    return FALSE;
}

void osal_dbg_dump_data(uint8_t *p, uint32_t len)
{
    int i;
    #define LINE_WIDTH 16

    if (len > 0){
        osal_printf("====================== dump data ======================\n");
        osal_printf(" Addr| ");

        for (i=0; i<LINE_WIDTH; i++) {
            osal_printf("%02x ", i);
        }
        osal_printf("\n -----------------------------------------------------");
        
        for (i=0;i<len;i++) {
            if ((i%LINE_WIDTH) == 0) {
                osal_printf("\n %04x| ", i);
            }
            osal_printf("%02x ", *(p+i));
        }
        osal_printf("\n======================== end ==========================\n");
    }
}

void dbg_buf_print(uint8_t *pbuf , uint32_t len)
{
    uint32_t i , j=0;

    printf("=================================================");
    for(i = 0 ; i < len ; i++) {
        if(i%16 == 0) {
            j = 0;
            printf("\r\n");
        }
        printf("%02x ",*((uint8_t *)(pbuf+i)));

        if(++j == 8)
            printf(" ");
    }

    printf("\r\n\n");
    return;
}



#ifdef RT_USING_FINSH
#include <finsh.h>
/**
 * Export to finsh of RT-thread
 */
void debug(char *module, int level)
{
	if (osal_dbg_set_level(module, level)) {
        osal_printf("success.\n");
	}
	else {
        osal_printf("cannot find module \"%s\" !\n", module);
	}
}
FINSH_FUNCTION_EXPORT(debug, 0-off 1-err 2-warn 3-info 4-trace 5-loud)

void dump(uint32_t addr, uint32_t len)
{
    osal_dbg_dump_data((uint8_t *)addr, len);
}
FINSH_FUNCTION_EXPORT(dump, printf the raw data)
#endif /* RT_USING_FINSH */

#endif /* OSAL_GLOBAL_DEBUG */
