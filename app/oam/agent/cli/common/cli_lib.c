/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cli_lib.c
 @brief  : cli common fuctions
 @author : wanglei
 @history:
           2015-10-29    wanglei    Created file
           ...
******************************************************************************/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "command.h"
#include "cli_lib.h"
#include "osal_cmn.h"
#include "cv_osal.h"

const char * enablevar_ntop(int language, int n)
{
    const char * str[CMD_HELP_LANG_NUM][ENABLEVAR_MAX] =
    {
        {"", "enable", "disable"},
        {"", "使能",   "禁止"}
    };

    if ((language < CMD_HELP_LANG_ENGLISH) ||
        (language > CMD_HELP_LANG_CHINESE) ||
        (n < ENABLEVAR_ENABLE) ||
        (n > ENABLEVAR_DISABLE))
    {
        return "--";
    }

    return str[language][n];
}

int e_strtol(const char * str, long * val, int base)
{
    long v;

    if ((NULL == str) || (0 == *str) || (NULL == val))
    {
        return -1;
    }

    errno = 0;
    v = strtol(str, NULL, base);
    if (((0 != errno) && (0 == v)) || ((ERANGE == errno) && ((LONG_MIN == v) || (LONG_MAX == v))))
    {
        return -1;
    }
    errno = 0;
    *val = v;

    return 0;
}

int e_strtoul(const char * str, unsigned long * val, int base)
{
    unsigned long v;

    if ((NULL == str) || (0 == *str) || (NULL == val))
    {
        return -1;
    }

    errno = 0;
    v = strtoul(str, NULL, base);
    if (((0 != errno) && (0 == v)) || ((ERANGE == errno) && (ULONG_MAX == v)))
    {
        return -1;
    }
    errno = 0;
    *val = v;

    return 0;
}

int e_atoi(const char * str, int * val)
{
    long v;

    if (0 != e_strtol(str, &v, 10))
    {
        return -1;
    }
    *val = (int) v;

    return 0;
}

int mac_pton(const char * p, void * n)
{
    int     nr = 0;
    char  * endptr;
    short * sp = n;

    for (;;)
    {
        if (nr++ > 2)
        {
            return -1;
        }
        *sp++ = ntohs((short) strtol(p, &endptr, 16));
        if ((NULL == endptr) || (4 != (endptr - p)))
        {
            return -1;
        }
        switch (*endptr)
        {
            case '.':
            {
                p = endptr + 1;
                break;
            }
            case '\0':
            {
                return (3 == nr) ? 0 : -1;
            }
            default:
            {
                return -1;
            }
        }
    }

    return 0;
}

double power_dbm2mw(double dbm)
{
    return pow(10.0, dbm * 0.1);
}

double power_mw2dbm(double mw)
{
    return 10.0 * log10(mw);
}

inline float __uc_temp(int v)
{
    return (((float) v) / 256.0);
}

inline float __uc_volt(int v)
{
    return (((float) v) / 10000.0);
}

inline float __uc_bias(int v)
{
    return (((float) v) / 500.0);
}

inline float __uc_power(int v)
{
    if (0 == v)
    {
        v = 1;
    }
    return power_mw2dbm(v * 0.0001);
}



/*****************************************************************
    Function Name : checkNetMaskValid
    Processing  : 检查mask的合法性
    Input Param     : mask主机字节序
    Output Param    :
    Return Values : FALSE---invalid, TRUE--valid.
******************************************************************
    History
    Data            Author      Modification
******************************************************************/


bool_t checkNetMaskValid(unsigned long ulMask)
{
    if (ulMask == 0)
    {
        return FALSE;
    }
    while (ulMask & 0x80000000)
    {
        ulMask = ulMask << 1;
    }
    return((ulMask) ? FALSE : TRUE);
}



/****************************************************************/
/** Function Name : checkNetMaskValid
    Processing  : IP地址合法性全面检查，仅针对IP V4地址，
                  不仅作A/B/C/D类检查，且做各类地址检查
 Class Address or Range Status
 *A 0.0.0.0                                 Reserved
 *   1.0.0.0 to 126.0.0.0                   Available
 *   127.0.0.0                              Reserved
 *
 *B 128.0.0.0 to 191.254.0.0                Available
 *   191.255.0.0                            Reserved
 *
 *C 192.0.0.0                               Reserved
 *    192.0.1.0 to 223.255.254              Available
 *    223.255.255.0                         Reserved
 *
 *D 224.0.0.0 to 239.255.255.255            Multicast group addresses
 *E 240.0.0.0 to 255.255.255.254            Reserved
 *   255.255.255.255                        Broadcast
 *
 * Input Param : IpAddr IP地址(主机序)
 * Return Values : FALSE---invalid, TRUE--valid.
 *****************************************************************/
bool_t cli_checkip(unsigned long IpAddr, int flag)
{
    /*地址类型检查*/
    if (IpAddr == INADDR_ANY)
    {
        return FALSE;
    }

    if (IpAddr == INADDR_BROADCAST)/*"255.255.255.255"*/
    {
        return FALSE;
    }

    if (IpAddr == INADDR_LOOPBACK)/*"127.0.0.1"*/
    {
        return FALSE;
    }

    if ( IpAddr == IN_LOOPBACKNET)/*"127.0.0.0"*/
    {
        return FALSE;
    }

    if (!(flag & CHECK_IP_F_ALLOW_MCAST)) {
        if (IN_MULTICAST(IpAddr))
        {
            return FALSE;
        }
        if (IN_EXPERIMENTAL(IpAddr))
        {
            return FALSE;
        }
    }

    /*具体地址检查*/
    if((IpAddr == 0 || (IpAddr&0xFF000000) == 0)/*0.0.0.0*/
    || ((IpAddr&0xFF000000) == 0x7F000000)/*127.0.0.0*/
    || ((IpAddr&0xFFFF0000) == 0xBFFF0000)/*191.255.0.0*/
    || ((IpAddr&0xFFFFFF00) == 0xC0000000)/*192.0.0.0-192.0.0.255*/
    || ((IpAddr&0xFFFFFF00) == 0xDFFFFF00))/*223.255.255.0*/
    {
        return FALSE;
    }

    return TRUE;
}

bool_t checkIpAddressValid(unsigned long IpAddr)
{
    return cli_checkip(IpAddr, 0);
}

