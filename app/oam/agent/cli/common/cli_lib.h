/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cli_lib.h
 @brief  : cli_lib.c header file
 @author : wanglei
 @history:
           2015-10-29    wanglei    Created file
           ...
******************************************************************************/

#ifndef __CLI_LIB_H
#define __CLI_LIB_H

#include <alloca.h>

#ifdef  __cplusplus
extern "C" {
#endif

enum
{
    ENABLEVAR_ENABLE = 1,
    ENABLEVAR_DISABLE,
    ENABLEVAR_MAX
};

enum
{
    TRUEVALUE_ENABLE = 1,
    TRUEVALUE_DISABLE,
    TRUEVALUE_MAX
};

extern int e_strtol(const char * str, long * val, int base);
extern int e_strtoul(const char * str, unsigned long * val, int base);
extern int e_atoi(const char * str, int * val);
extern const char * enablevar_ntop(int language, int n);
#define enablevar_pton(p) (((NULL != (p)) && ('\0' != *(p)) && ('e' == *(p))) ? ENABLEVAR_ENABLE : ENABLEVAR_DISABLE)
extern const char * vlan_mode_ntop(long mode);
extern int mac_pton(const char * p, void * n);
#define mac_ntop(n, p) ((void) sprintf((p), "%02hhX%02hhX.%02hhX%02hhX.%02hhX%02hhX", (n)[0], (n)[1], (n)[2], (n)[3], (n)[4], (n)[5]), p)
extern double power_dbm2mw(double dbm);
extern double power_mw2dbm(double mw);
extern inline float __uc_temp(int v);
extern inline float __uc_volt(int v);
extern inline float __uc_bias(int v);
extern inline float __uc_power(int v);

#define CHECK_IP_F_ALLOW_MCAST 1
extern bool_t cli_checkip(unsigned long IpAddr, int flag);
extern bool_t checkNetMaskValid(unsigned long ulMask);
extern bool_t checkIpAddressValid(unsigned long IpAddr);

#ifdef  __cplusplus
}
#endif

#endif
