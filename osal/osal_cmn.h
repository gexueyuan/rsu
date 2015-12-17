/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : osal_cmn.h
 @brief  : osal_cmn.c header file
 @author : wanglei
 @history:
           2015-8-25    wanglei    Created file
           ...
******************************************************************************/


#ifndef _OSAL_CMN_H_
#define _OSAL_CMN_H_

#include "os_core.h"

typedef int osal_status_t;

#define OSAL_EOK                          0               /**< There is no error */
#define OSAL_ERROR                        1               /**< A generic error happens */
#define OSAL_ETIMEOUT                     2               /**< Timed out */
#define OSAL_EFULL                        3               /**< The resource is full */
#define OSAL_EEMPTY                       4               /**< The resource is empty */
#define OSAL_ENOMEM                       5               /**< No memory */
#define OSAL_ENOSYS                       6               /**< No system */
#define OSAL_EBUSY                        7               /**< Busy */
#define OSAL_EIO                          8               /**< IO error */


#define OSAL_STATUS_SUCCESS         (OSAL_EOK)
#define OSAL_STATUS_TIMEOUT         (-OSAL_ETIMEOUT)
#define OSAL_STATUS_FULL            (-OSAL_EFULL)
#define OSAL_STATUS_EMPTY           (-OSAL_ENOMEM)
#define OSAL_STATUS_NOMEM           (-OSAL_ENOMEM)
#define OSAL_STATUS_ERROR_UNDEFINED (-OSAL_ERROR)

void        osal_printf(const char * fmt, ...);
void        osal_syslog(int level, const char * fmt, ...);
uint64_t    osal_current_time(void);
void        osal_udelay(int32_t microseconds);
void        osal_mdelay(int32_t milliseconds);
void        osal_sleep(int32_t milliseconds);

#endif

