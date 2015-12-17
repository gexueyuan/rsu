/*
   $Id$

   Command interpreter routine for virtual terminal [aka TeletYpe]
   Copyright (C) 1997, 98, 99 Kunihiro Ishiguro

This file is part of GNU Zebra.

GNU Zebra is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2, or (at your
option) any later version.

GNU Zebra is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Zebra; see the file COPYING.  If not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/wait.h>

#include "readline.h"
#include "history.h"

#include "vector.h"
#include "vty.h"
#include "command.h"
#include "cli_user_lib.h"
#include "cli_lib.h"

#include "cv_osal.h"

#define CONFIRM_LEN 8
#define VERDOR_ID_LEN 8
#define MAC_ADDR_LEN  6



DEFUN
(
    reboot,
    reboot_cmd,
    "reboot",
    "Reboot\n重新启动\n",
    CMD_LEVEL_ADMINISTRATOR_15
)
{
    system("reboot");
	return CMD_SUCCESS;
}




static int show_version(struct vty *vty)
{
	char hw_version[20+1] = {0};
	char sw_version[20+1] = {0};
	char eeprom_version[8] = {0};
	char szfw_version[16] = {0};
    char SerialNo[25] = {0};
    char ProductVer[17] = {0};
	int deviceType = 0;
    char deviceTypeString[65]={0};

    memset(deviceTypeString, 0, sizeof(deviceTypeString));

    
    VTY_OUT_EC(vty, "%sDevice Type       : %s%s"
                "%sSerial Number     : %s%s"
                "%sProduct Version   : %s%s"
                "%sSoftware Version  : %s%s"
                "%sHardware Version  : %s%s"
                ,
                "%s产品名称   : %s%s"
                "%s产品序列号 : %s%s"
                "%s产品版本   : %s%s"
                "%s软件版本   : %s%s"
                "%s硬件版本   : %s%s"
                ,
                VTY_LINE_HEAD, deviceTypeString, VTY_NEWLINE,
                VTY_LINE_HEAD, SerialNo, VTY_NEWLINE,
                VTY_LINE_HEAD, ProductVer, VTY_NEWLINE,
                VTY_LINE_HEAD, sw_version, VTY_NEWLINE,
                VTY_LINE_HEAD, hw_version, VTY_NEWLINE
                );

    return CMD_SUCCESS;
}


DEFUN
(
    show_all_info,
    show_info_cmd,
    "show (information|parameter|version)",
    SHOW_TIPS_EN SHOW_TIPS_CH
    "Device information\n设备信息\n"
    "Device detail information\n设备详细信息\n"
    "Capability information\n能力信息\n"
    "Device hardware, software, firmware,EEPROM version\n设备硬件、软件、固件和EEPROM版本\n",
    CMD_LEVEL_MONITOR_10
)
{
    switch (argv[0][0])
    {
        /* information */
        case 'i':
        {
            //return show_info(vty, FALSE);
            break;
        }

        /* param */
        case 'p':
        {
            //return show_info(vty, TRUE);
            break;
        }

        /* version */
        case 'v':
        {
            return show_version(vty);
        }

        default:
            return CMD_WARNING;
    }
    return CMD_SUCCESS;
}


DEFUN
(
    write_fun,
    write_cmd,
    "write",
    "Save config data\n存储用户的配置信息\n",
    CMD_LEVEL_ADMINISTRATOR_15
)
{
    /* to be added.. 保存配置到配置文件 */
    
    return CMD_SUCCESS;
}

int set_enable_passwd(char *pcPasswd)
{
    int iError = OSAL_EOK;
    int uiError_Code = OSAL_EOK;

#if 0
    /* to be added ... */
#endif

    return uiError_Code;
}

DEFUN
(
    enable_passwd_fun,
    enable_passwd_cmd,
    "enable password",
    "Enable command information\nenable命令信息\n"
    "Change current enable password\n修改enable的密码\n",
    CMD_LEVEL_MONITOR_5
)
{
    extern int change_password(struct vty *vty);
    vty->iChangePasswdType = CHANGE_ENABLE_PASSWOED;
    return change_password(vty);
}


static char *format_LF2CRLF(char *pcLine)
{
    char szTmp[256] = {0};
    char *pcTmp = pcLine;
    unsigned long i;
    for (i = 0 ; i < 256 ; i++)
    {
        if (0 != *pcTmp)
        {
            if ('\n' == *pcTmp)
            {
                szTmp[i++] = '\r';
            }
            szTmp[i] = *pcTmp;
            ++pcTmp;
        }
        else
        {
            break;
        }
    }

    strcpy(pcLine, szTmp);
    return pcLine;
}

int ping(struct vty *vty, const char * const argv[])
{
    int fd[2];
    char szLine[256] = {0};

    if (pipe(fd)<0 )
    {
        vty_out(vty, "pipe fail!\n");
        return CMD_WARNING;
    }

    if ((vty->stChildPId = fork())<0 )
    {
        vty_out(vty, "fork fail!\n");
        return CMD_WARNING;
    }
    //父进程
    else if (vty->stChildPId > 0)
    {
        close(fd[1]);
        while (read(fd[0], szLine, 128) > 0)
        {
    		if (vty_shell (vty))
    		{
	            vty_out(vty, "%s", szLine);
    	        //fflush(stdout);
    	    }
    	    else
    	    {
    	        format_LF2CRLF(szLine);
	            vty_out(vty, "%s", szLine);
    	        vty_flush(vty);
    	    }
            //line[0] = 0;
            memset(szLine, 0, 256);
        }
        close(fd[0]);
        waitpid(vty->stChildPId, NULL, 0);
        vty->stChildPId = 0;
    }
    else
    {
        sigset_t bset;

        //解除对SIGINT的阻塞
        sigemptyset(&bset);
        sigaddset(&bset, SIGINT);
        sigprocmask(SIG_UNBLOCK, &bset, NULL);

        setpgid(0,0);

        close(fd[0]);
        if (fd[1] != STDOUT_FILENO)
        {
            if (dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO)
            {
                printf("dup2 STDOUT_FILENO fail !\n");
            }
        }
        if (fd[1] != STDERR_FILENO)
        {
            if (dup2(fd[1], STDERR_FILENO) != STDERR_FILENO)
            {
                printf("dup2 STDERR_FILENO fail !\n");
            }
        }
        if (fd[1] != STDOUT_FILENO && fd[1] != STDERR_FILENO)
        {
            close(fd[1]);
        }

        if (execv("/bin/ping", (char * const *)argv) <0)
        {
            printf("execv ping fail !\n");
            exit(0);
        }
    }

    return CMD_WARNING;
}
#define PING       "Test that a remote host is reachable\n测试远程主机是否可达\n"
#define PING_IP    "Ip address to ping\n测试主机IP地址\n"
#define PING_WAIT  "Packet timeout in seconds\n指定ping程序等待应答的超时时间\n"
#define PING_TIME  "Time in second(s)\n以秒为单位的时间\n"
#define PING_SIZE  "Size of icmp echo packet\n指定ICMP应答报文的大小\n"
#define PING_COUNT "Number of packets to receive\n指定发送多少个包后退出ping程序\n"

DEFUN
(
    ping_fun,
    ping_cmd,
    "ping A.B.C.D",
    PING PING_IP,
    CMD_LEVEL_OPERATOR_11
)
{
    const char * const arg[] = {"ping", argv[0], "-c", "5", NULL};
    return ping(vty, arg);
}

DEFUN
(
    ping_fun_w,
    ping_cmd_w,
    "ping A.B.C.D waittime <1-60>",
    PING PING_IP PING_WAIT PING_TIME,
    CMD_LEVEL_OPERATOR_11
)
{
    const char * const arg[] = {"ping", argv[0], "-W", argv[1], "-c", "5", NULL};
    return ping(vty, arg);
}

DEFUN
(
    ping_fun_s,
    ping_cmd_s,
    "ping A.B.C.D size <0-4096>",
    PING PING_IP PING_SIZE PING_SIZE,
    CMD_LEVEL_OPERATOR_11
)
{
    const char * const arg[] = {"ping", argv[0], "-s", argv[1], "-c", "5", NULL};
    return ping(vty, arg);
}

DEFUN
(
    ping_fun_c,
    ping_cmd_c,
    "ping A.B.C.D count <1-65535>",
    PING PING_IP PING_COUNT PING_COUNT,
    CMD_LEVEL_OPERATOR_11
)
{
    const char * const arg[] = {"ping", argv[0], "-c", argv[1], NULL};
    return ping(vty, arg);
}

DEFUN
(
    ping_fun_sw,
    ping_cmd_sw,
    "ping A.B.C.D size <0-4096> waittime <1-60>",
    PING PING_IP PING_SIZE PING_SIZE PING_WAIT PING_TIME,
    CMD_LEVEL_OPERATOR_11
)
{
    const char * const arg[] = {"ping", argv[0], "-s", argv[1], "-W", argv[2], "-c", "5", NULL};
    return ping(vty, arg);
}

DEFUN
(
    ping_fun_cw,
    ping_cmd_cw,
    "ping A.B.C.D count <1-65535> waittime <1-60>",
    PING PING_IP PING_COUNT PING_COUNT PING_WAIT PING_TIME,
    CMD_LEVEL_OPERATOR_11
)
{
    const char * const arg[] = {"ping", argv[0], "-c", argv[1], "-W", argv[2], NULL};
    return ping(vty, arg);
}

DEFUN
(
    ping_fun_cs,
    ping_cmd_cs,
    "ping A.B.C.D count <1-65535> size <0-4096>",
    PING PING_IP PING_COUNT PING_COUNT PING_SIZE PING_SIZE,
    CMD_LEVEL_OPERATOR_11
)
{
    const char * const arg[] = {"ping", argv[0], "-c", argv[1], "-s", argv[2], NULL};
    return ping(vty, arg);
}

DEFUN
(
    ping_fun_csw,
    ping_cmd_csw,
    "ping A.B.C.D count <1-65535> size <0-4096> waittime <1-60>",
    PING PING_IP PING_COUNT PING_COUNT PING_SIZE PING_SIZE PING_WAIT PING_TIME,
    CMD_LEVEL_OPERATOR_11
)
{
    const char * const arg[] = {"ping", argv[0], "-c", argv[1], "-s", argv[2], "-W", argv[3], NULL};
    return ping(vty, arg);
}



void inet_ntov(unsigned long addr, unsigned long * vec)
{
    size_t i;
    unsigned char  * p = (unsigned char *) &addr;

    for (i = 0; i < sizeof(unsigned long); i++)
    {
        vec[i] = *(p + i);
    }
}



void install_dev_manage_cmd()
{
    install_element(ENABLE_NODE, &reboot_cmd);
    install_element(ENABLE_NODE, &show_info_cmd);

    install_element(ENABLE_NODE, &write_cmd);

    install_element(ENABLE_NODE, &enable_passwd_cmd);
    install_element(CONFIG_NODE, &show_info_cmd);
    install_element(ENABLE_NODE, &ping_cmd);
    install_element(ENABLE_NODE, &ping_cmd_w);
    install_element(ENABLE_NODE, &ping_cmd_s);
    install_element(ENABLE_NODE, &ping_cmd_c);
    install_element(ENABLE_NODE, &ping_cmd_sw);
    install_element(ENABLE_NODE, &ping_cmd_cs);
    install_element(ENABLE_NODE, &ping_cmd_cw);
    install_element(ENABLE_NODE, &ping_cmd_csw);

    return;
}

