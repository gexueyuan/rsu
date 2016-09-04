/*****************************************************************************
 Copyright(C) Beijing Carsmart Technology Co., Ltd.
 All rights reserved.
 
 @file   : user_mng_cmd.c
 @brief  : user manager command
 @author : wanglei
 @history:
           2015-12-1    wanglei    Created file
           ...
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include "readline.h"
#include "history.h"

#include "zebra.h"
#include "cli_version.h"
#include "command.h"
#include "cli_user_lib.h"

#define E_USER_BASE                                  (0)
#define E_USER_NAME_IS_EXIST                         (E_USER_BASE | 1)
#define E_USER_NUM_EXCEED                            (E_USER_BASE | 2)


/* 获取用户密码 */
static int get_user_info(const char *pcName, char *pcPasswd, int *piPriority)
{
	int rc_error;
	uint32_t i;
	char szPasswd[MAX_PASSWD_LEN+1] = {0};
	int iPriority;
    int online;
    int type;
    rc_error = 0;
    get_cli_user_info(pcName, pcPasswd, piPriority, &online, &type);

    return rc_error;
}

/* 用户登陆认证 */
int use_auth(char *pcUserName, char *pcPasswd, int *piPriority)
{
	int iErrCode;
	char szPasswd[MAX_PASSWD_LEN+1] = {0};
	
    if (NULL == pcUserName || NULL == pcPasswd)
    {
        return CMD_WARNING;
    }

    iErrCode = get_user_info(pcUserName, szPasswd, piPriority);

    if ((OSAL_EOK != iErrCode) || (0 != strcmp(pcPasswd, szPasswd)))
    {
        return CMD_ERR_NO_MATCH;
    }

    return CMD_SUCCESS;
}

/* 进入enable模式的密码认证 */
int enable_auth(char *pcPasswd)
{
	unsigned long index = 0;
	int rc_error;
	char szPasswd[MAX_PASSWD_LEN+1] = {0};
#if 0
	
    if (NULL == pcPasswd)
    {
        return CMD_WARNING;
    }

    if ((OSAL_EOK != rc_error) || (0 != strcmp(pcPasswd, szPasswd)))
    {
        return CMD_WARNING;
    }
#endif

    return CMD_SUCCESS;
}

/* 成功:返回添加的用户的下标;失败:返回-1;用户满:返回最大用户数 */
int registe_user(char *pcUserName, int iTerminalType)
{
    unsigned long i;
	int iPriority;
	int rc_error;

	iPriority = 0;
    if (NULL == pcUserName)
    {
        return -1;
    }
    user_sem_lock();

    for (i=0; i<MAX_CLI_USER; ++i)
    {
        if (0 == strcmp(pcUserName, g_astUserArr[i].szName))
        {
            g_astUserArr[i].iTerminalType |= iTerminalType;
            break;
        }
    }
    
    if (MAX_CLI_USER == i)
    {
        for (i=0; i<MAX_CLI_USER; ++i)
        {
            if (0 == g_astUserArr[i].iTerminalType)
            {
                sprintf(g_astUserArr[i].szName, pcUserName);
                g_astUserArr[i].iTerminalType = iTerminalType;
                g_astUserArr[i].iPriority = iPriority;
                break;
            }
        }
    }
    
    user_sem_unlock();

    return i;
}

/* 如果没有用户登陆，对iUserIndex对应的用户信息清零 */
void release_user(int iUserIndex, int iTerminal)
{
    user_sem_lock();

    if (iUserIndex < MAX_CLI_USER)
    {
        g_astUserArr[iUserIndex].iTerminalType ^= iTerminal;
        if (0 == g_astUserArr[iUserIndex].iTerminalType)
        {
            memset(&g_astUserArr[iUserIndex], 0, sizeof(strUserInfo));
        }
    }
    
    user_sem_unlock();

    return;
}

int set_user_passwd(const char *pcName, const char *pcPasswd)
{
    int i;	
    for (i=0; i<MAX_CLI_USER; ++i)
    {
        if (0 == g_astUserArr[i].iTerminalType)
        {
            memcpy(g_astUserArr[i].szPwd, pcPasswd, MAX_PASSWD_LEN);
            break;
        }
    }

    return CMD_SUCCESS;
}

static int set_user_privilege(const char *pcName, int iPrivilege)
{
	int i;
	
    for (i=0; i<MAX_CLI_USER; ++i)
    {
        if (0 == g_astUserArr[i].iTerminalType)
        {
            g_astUserArr[i].iPriority = iPrivilege;
            break;
        }
    }

    return CMD_SUCCESS;
}

int change_password(struct vty *vty)
{
    int iErrCode = CMD_SUCCESS;
    char szPasswd[MAX_PASSWD_LEN+1];
    char szAgainPasswd[MAX_PASSWD_LEN+1];

    extern int change_shell_passwd(struct vty *pstVty, char *pcPasswd, char *pcPasswdAgain);
    /* shell */
    if (VTY_SHELL == vty->type)
    {
        iErrCode = change_shell_passwd(vty, szPasswd, szAgainPasswd);
        if (CMD_SUCCESS != iErrCode)
        {
            INPUT_PASSWD_TOO_LENGTH(vty);
        }
        else
        {
            if (0 == strcmp(szPasswd, szAgainPasswd))
            {
                /* 修改mib中passwd节点的值 */
                if (CHANGE_USER_PASSWOED == vty->iChangePasswdType)
                {
                    iErrCode = set_user_passwd(g_astUserArr[vty->user_index].szName, szPasswd);
                }
                else
                {
                    iErrCode = set_enable_passwd(szPasswd);
                }
            }
            else
            {
                iErrCode = CMD_WARNING;
            }

            if (CMD_SUCCESS != iErrCode)
            {
                TWO_INPUT_PASSWD_DIFF(vty);
            }
        }
    }
    /* telnet */
    else
    {
        vty->node = CHANGE_PASSWD_NODE;
    }

    return iErrCode;
}

DEFUN
(
    passwd_fun, 
    passwd_cmd,
    "password",
    "Set user password\n修改当前登录用户的密码\n",
    CMD_LEVEL_MONITOR_5
)
{
    vty->iChangePasswdType = CHANGE_USER_PASSWOED;
    return change_password(vty);
}

/* 增加一用户 */
static int user_add(const char *pcName, const char *pcPasswd)
{
    int rc_error = 0;
    uint32_t i;
    int ulPriority = 15;

    for (i=0; i<MAX_CLI_USER; ++i)
    {
        if (0 == strcmp(pcName, g_astUserArr[i].szName))
        {
            //g_astUserArr[i].iTerminalType |= iTerminalType;
            break;
        }
    }
    

    return rc_error;
}

/* 删除一用户 */
static int user_del(const char *pcName)
{
    int rc_error = 0;
    uint32_t i;
    return rc_error;
}

DEFUN
(
    add_user_fun, 
    add_user_cmd,
    "user name USERNAME password PASSWORD",
    "User information\n用户信息\n"
    "Set user name\n设置用户名\n"
    "user name (Not exceed 16 characters!)\n用户名 (不超过16个字符!)\n"
    "Set password\n设置密码\n"
    "Password (Not exceed 16 characters!)\n用户密码 (不超过16个字符!)\n",
    CMD_LEVEL_ADMINISTRATOR_15
)
{
	int iErrCode;
	int iPriority;
	char szPasswd[MAX_PASSWD_LEN+1] = {0};

    /* 检查用户名和密码的长度 */
	if (strlen(argv[0])>MAX_USER_NAME_LEN)
	{
        VTY_SET_UNSUCCESSFULLY(vty);
        VTY_OUT_EC(vty, "user name not exceed %d characters%s", "用户名长度不能超过%d个字符%s", 
                        MAX_USER_NAME_LEN, VTY_NEWLINE);
	    return CMD_WARNING;
	}
	if (strlen(argv[1])>MAX_PASSWD_LEN)
	{
        VTY_SET_UNSUCCESSFULLY(vty);
        VTY_OUT_EC(vty, "Password not exceed %d characters%s", "用户密码长度不能超过%d个字符%s", 
                        MAX_PASSWD_LEN, VTY_NEWLINE);
	    return CMD_WARNING;
	}

	iErrCode = get_user_info(argv[0], szPasswd, &iPriority);
	
    if (OSAL_EOK != iErrCode)
    {
        iErrCode = user_add(argv[0], argv[1]); 
        if(iErrCode != 0)
        {
            if(iErrCode == E_USER_NUM_EXCEED)
            {
                VTY_SET_UNSUCCESSFULLY(vty);
                VTY_OUT_EC(vty, "Exceed the max user number of 5%s", "超过5个最大用户数%s", VTY_NEWLINE);
                return CMD_WARNING;
            }
            else if(iErrCode == E_USER_NAME_IS_EXIST)
            {
                VTY_SET_UNSUCCESSFULLY(vty);
                VTY_OUT_EC(vty, "The user name already exist%s", "该用户名已存在%s", VTY_NEWLINE);
                return CMD_WARNING;
            }
            else
            {
                return iErrCode;
            }
        }        
    }
    else
    {
        if (0 != strcmp(argv[1], szPasswd))
        {
            iErrCode = set_user_passwd(argv[0], argv[1]);
        }
    }

    return iErrCode;
}

DEFUN
(
    del_user_fun, 
    del_user_cmd,
    "no username USERNAME",
    NO_TIPS_EN NO_TIPS_CH
    "Delete user information\n删除用户信息\n"
    "user name (Not exceed 16 characters!)\n用户名 (不超过16个字符!)\n",
    CMD_LEVEL_ADMINISTRATOR_15
)
{
	int iErrCode;
	int iPriority;
	int i;
	char szPasswd[MAX_PASSWD_LEN+1] = {0};

    /* 检查用户名长度 */
	if (strlen(argv[0])>MAX_USER_NAME_LEN)
	{
        VTY_SET_UNSUCCESSFULLY(vty);
        VTY_OUT_EC(vty, "user name not exceed %d characters%s", "用户名长度不能超过%d个字符%s", 
                        MAX_USER_NAME_LEN, VTY_NEWLINE);
	    return CMD_WARNING;
	}

	/* 不能删除自己 */
	if (0 == strcmp(argv[0], g_astUserArr[vty->user_index].szName))
	{
        VTY_SET_UNSUCCESSFULLY(vty);
        VTY_OUT_EC(vty, "You can not delete yourself!%s", "你不能够删除自己%s", VTY_NEWLINE);
	    return CMD_WARNING;
	}
	/* 不能删除在线的用户 */
	for(i=0; i<MAX_CLI_USER; ++i)
	{
	    if (0 == strcmp(argv[0], g_astUserArr[i].szName))
	    {
            VTY_SET_UNSUCCESSFULLY(vty);
            VTY_OUT_EC(vty, "Can not delete online user!%s", "不能够删除在线用户!%s", VTY_NEWLINE);
    	    return CMD_WARNING;
	    }
	}

	iErrCode = get_user_info(argv[0], szPasswd, &iPriority);
	
    if (OSAL_EOK != iErrCode)
    {
        VTY_SET_UNSUCCESSFULLY(vty);
        VTY_OUT_EC(vty, "User not exist%s", "用户不存在%s", VTY_NEWLINE);
        iErrCode = CMD_WARNING;
    }
    else
    {
        iErrCode = user_del(argv[0]);
    }

    return iErrCode;
}

DEFUN
(
    add_privilege_fun, 
    add_privilege_cmd,
    "user name USERNAME privilege <1-15>",
    "User information\n用户信息\n"
    "Set user name\n设置用户名\n"
    "user name (Not exceed 16 characters!)\n用户名 (不超过16个字符!)\n"
    "Set privilege information\n设置权限\n"
    "Privilege information\n权限信息\n",
    CMD_LEVEL_ADMINISTRATOR_15
)
{
	int iErrCode;
	int iPriority;
	char szPasswd[MAX_PASSWD_LEN+1] = {0};
	
    /* 检查用户名长度 */
	if (strlen(argv[0])>MAX_USER_NAME_LEN)
	{
        VTY_SET_UNSUCCESSFULLY(vty);
        VTY_OUT_EC(vty, "user name not exceed %d characters%s", "用户名长度不能超过%d个字符%s", 
                        MAX_USER_NAME_LEN, VTY_NEWLINE);
	    return CMD_WARNING;
	}

    if (0 == strcmp(argv[0], g_astUserArr[vty->user_index].szName))
    {
        VTY_SET_UNSUCCESSFULLY(vty);
        VTY_OUT_EC(vty, "can't modify self privilege%s", "不能修改自己的权限%s", VTY_NEWLINE);
	    return CMD_WARNING;
    }
    
	iErrCode = get_user_info(argv[0], szPasswd, &iPriority);
	
    if (OSAL_EOK != iErrCode)
    {
        VTY_SET_UNSUCCESSFULLY(vty);
        VTY_OUT_EC(vty, "User not exist%s", "用户不存在%s", VTY_NEWLINE);
        iErrCode = CMD_WARNING;
    }
    else
    {
        if (iPriority != atoi(argv[1]))
        {
            iErrCode = set_user_privilege(argv[0], atoi(argv[1]));
        }
    }

    return iErrCode;
}

DEFUN
(
    show_user_fun, 
    show_user_cmd,
    "show user",
    "Show running system information\n显示系统运行信息\n"
    "User information\n用户信息\n",
    CMD_LEVEL_MONITOR_5
)
{
    uint32_t uiErrorCode;
	int rc_error;
	int iPri;
	uint uiPriLen = sizeof(iPri);
	unsigned long i;
	int iState;
	char szUserName[MAX_USER_NAME_LEN+1];
	char szState[2][10] = {"offline", "online"};
	
    VTY_OUT_EC(vty, "Username        Priority  Server  Userstatus%s",
                    "用户名          优先级  服务器  用户状态%s",
                    VTY_NEWLINE);
    VTY_OUT_EC(vty, "--------------------------------------------%s",
                    "----------------------------------------%s",
                    VTY_NEWLINE);

    while(1)
    {
        for (i=0; i<MAX_CLI_USER; ++i)
        {
            if (0 == strcmp(g_astUserArr[i].szName, szUserName))
            {
                iState = 1;
                break;
            }
        }
        
        VTY_OUT_EC(vty, "%-16s%-10d%-8s%s%s",
                        "%-16s%-8d%-8s%s%s",
                    szUserName, 
                    iPri,
                    "Local",
                    szState[iState],
                    VTY_NEWLINE);
    }
    
    return rc_error;
}

void install_user_mng_cmd()
{
    install_element(ENABLE_NODE, &passwd_cmd);
    install_element(ENABLE_NODE, &add_user_cmd);
    install_element(ENABLE_NODE, &add_privilege_cmd);
    install_element(ENABLE_NODE, &del_user_cmd);
    install_element(ENABLE_NODE, &show_user_cmd);
    return;
}

