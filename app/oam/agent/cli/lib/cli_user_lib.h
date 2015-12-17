/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : cli_user_lib.h
 @brief  : cli_user_lib.h header file
 @author : wanglei
 @history:
           2015-12-1    wanglei    Created file
           ...
******************************************************************************/

#ifndef __CLI_LIB_H__
#define __CLI_LIB_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
#define MAX_CLI_USER    4
#define MAX_USER_NAME_LEN 16
#define MAX_PASSWD_LEN    16

#define USERSTATUS_ONLINE	1L
#define USERSTATUS_OFFLINE	2L

/*----------------------------------------------*
 * 类型定义                                     *
 *----------------------------------------------*/

typedef struct _user_node
{
    char szName[MAX_USER_NAME_LEN+1];
    char szPwd[MAX_PASSWD_LEN+1];
    int iPriority;
    int iTerminalType;
}strUserInfo;

/*----------------------------------------------*
 * 常量、变量声明                               *
 *----------------------------------------------*/
extern strUserInfo *g_astUserArr;

/*----------------------------------------------*
 * 函数声明                                     *
 *----------------------------------------------*/
int user_sem_lock(void);
int user_sem_unlock(void);
int get_cli_user_info(char *user_name, char *user_pwd, int *pri, int *online, int *terminal_type);
int get_cli_user_pwd(char *user_name, char *user_pwd);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __CLI_LIB_H__ */
