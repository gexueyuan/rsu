/*
 * Virtual terminal [aka TeletYpe] interface routine.
 * Copyright (C) 1997, 98 Kunihiro Ishiguro
 *
 * This file is part of GNU Zebra.
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "zebra.h"
#include <sys/prctl.h>
#include <sched.h>
#include "buffer.h"
#include <version.h>
#include "command.h"
#include "sockunion.h"
#include "memory.h"
#include "vty.h"
#include "cli_lib.h"
#include <arpa/telnet.h>
#include "cli_user_lib.h"
#include "loop_buf.h"

#include "osal_cmn.h"

/* Vty events */
enum event
{
  VTY_SERV,
  VTY_READ,
  VTY_WRITE,
  VTY_TIMEOUT_RESET,
#ifdef VTYSH
  VTYSH_SERV,
  VTYSH_READ,
  VTYSH_WRITE
#endif /* VTYSH */
};

//static void vty_event (enum event, int, struct vty *);

/* Extern host structure from command.c */
extern struct host host;

/* Vector which store each vty structure. */
static vector vtyvec;

/* Current directory. */
char *vty_cwd = NULL;

/* Configure lock. */
//static int vty_config;

/* Login password check. */
static int no_password_check = 0;

/* 最大支持3个telnet连接 */
#define MAX_TELNET_NUM 3
static struct vty *g_pstVtyArr[MAX_TELNET_NUM];
static pthread_mutex_t g_stVtyMutex = PTHREAD_MUTEX_INITIALIZER;
/* telnet信息 */
static StrTelnetInfo g_stTelnetInfo;

/******begin:防止非法用户重复登陆**********************************************/
typedef struct DosNodeTag
{
    int iSecend; //剩余秒数
    char szIp[20];
    struct DosNodeTag *next;
}StrDosNode;
static StrDosNode *g_pstDosHead;
static pthread_mutex_t g_stDosMutex = PTHREAD_MUTEX_INITIALIZER;

/*****************************************************************************
 函 数 名  : FindDos
 功能描述  : 查找当前登陆是否在dos攻击表里面
 输入参数  : char *pIp
 输出参数  : 无
 返 回 值  : int 1表示存在，0表示不存在
 调用函数  :
 被调函数  :
*****************************************************************************/
static int FindDos(char *pIp)
{
    int iRet = 0;
    StrDosNode *pstTmp;

    pthread_mutex_lock(&g_stDosMutex);
    pstTmp = g_pstDosHead;

    while (NULL != pstTmp)
    {
        if (0 == strcmp(pIp, pstTmp->szIp))
        {
            iRet = 1;
            break;
        }
        pstTmp = pstTmp->next;
    }
    pthread_mutex_unlock(&g_stDosMutex);

    return iRet;
}

/*****************************************************************************
 函 数 名  : DosThread
 功能描述  : dos攻击链表的处理函数
 输入参数  : void *arg
 输出参数  : 无
 返 回 值  : void
 调用函数  :
 被调函数  :
*****************************************************************************/
static void DosThread (void *arg)
{
    StrDosNode *pstTmp;
    StrDosNode *pstTmp2;
    int flag = 1;

    THREAD_SETNAME_PRI("vty_dos", 40);

    while (flag)
    {
        pthread_mutex_lock(&g_stDosMutex);

        pstTmp = g_pstDosHead;

        if (NULL != pstTmp)
        {
            while (NULL != pstTmp)
            {
                if (pstTmp->iSecend > 0)
                {
                    pstTmp->iSecend--;
                    pstTmp = pstTmp->next;
                }
                else
                {
                    pstTmp2 = pstTmp;
                    pstTmp = pstTmp->next;
                    if (g_pstDosHead == pstTmp2)
                    {
                        g_pstDosHead = pstTmp;
                    }
                    free(pstTmp2);
                }
            }
        }
        else
        {
            flag = 0;
        }
        pthread_mutex_unlock(&g_stDosMutex);

        sleep(1);
    }

    return;
}

/*****************************************************************************
 函 数 名  : AddDos
 功能描述  : 增加一个节点到dos攻击链表中
 输入参数  : char *pIp
 输出参数  : 无
 返 回 值  : void
 调用函数  :
 被调函数  :
*****************************************************************************/
static void AddDos(char *pIp)
{
    StrDosNode *pstTmp;
    pthread_t tid;

    pstTmp = (StrDosNode *)malloc(sizeof(StrDosNode));
    if (NULL == pstTmp)
    {
        return;
    }
    sprintf(pstTmp->szIp, pIp);
    pstTmp->iSecend = 60;

    pthread_mutex_lock(&g_stDosMutex);
    pstTmp->next = g_pstDosHead;
    g_pstDosHead = pstTmp;
    pthread_mutex_unlock(&g_stDosMutex);

    if (NULL == pstTmp->next)
    {
        pthread_create(&tid, NULL, (void *)DosThread, NULL);
        if (0 != pthread_detach(tid))
        {
            printf("pthread_detach error \n");
        }
    }

    return;
}


/* 设置telnet端口 */
int SetTelnetPort(int iPort)
{
    if (iPort<0 || iPort>0xffff)
    {
        return OSAL_ERROR;
    }

    g_stTelnetInfo.iPort = iPort;

    return OSAL_EOK;
}

/* 获取telnet端口 */
int GetTelnetPort()
{
    return g_stTelnetInfo.iPort;
}

/* telnet是否启动 */
bool_t IsTelnetEnable()
{
    return (g_stTelnetInfo.iEnable == ENABLEVAR_ENABLE);
}

void vty_serv_sock (void *arg);
int SetTelnetEnable()
{
    /* 如果telnet没有启动，启动telnet */
    if (ENABLEVAR_ENABLE != g_stTelnetInfo.iEnable)
    {
        if (0 != rc_pthread_create(&g_stTelnetInfo.stTelnetId, "tk_telnet", TK_PRIO_DEFAULT, vty_serv_sock, NULL))
        {
            perror("Create pthread error!");
            return OSAL_ERROR;
        }
    }

    g_stTelnetInfo.iEnable = ENABLEVAR_ENABLE;

    return OSAL_EOK;
}

/* 删除所有vty用户 */
static void close_all_vty()
{
    int i;

    pthread_mutex_lock(&g_stVtyMutex);
    for (i=0; i<MAX_TELNET_NUM; ++i)
    {
        if (NULL != g_pstVtyArr[i])
        {
            g_pstVtyArr[i]->status = VTY_CLOSE;
        }
    }
    pthread_mutex_unlock(&g_stVtyMutex);

    return;
}

int SetTelnetDisable()
{
    /* 如果telnet已经启动，关闭telnet */
    if (ENABLEVAR_ENABLE == g_stTelnetInfo.iEnable)
    {
        /* 关闭所有telnet连接 */
        close_all_vty();

        /* 关闭telnet主线程 */
        pthread_cancel(g_stTelnetInfo.stTelnetId);
        g_stTelnetInfo.stTelnetId = 0;
        g_stTelnetInfo.iEnable = ENABLEVAR_DISABLE;
    }

    return OSAL_EOK;
}

/* telnet线程的初始化 */
void TelnetInit()
{
    int iEnable = 1;
    int iPort;
    int rc_error;

    g_stTelnetInfo.iEnable = ENABLEVAR_DISABLE;

    /* telnet port can be configed */
    g_stTelnetInfo.iPort = 23;
    g_stTelnetInfo.stTelnetId = 0;

    /* 如果telnet默认是开启的，开启telnet */
    if (ENABLEVAR_ENABLE == iEnable)
    {
        SetTelnetEnable();
    }

    return;
}



/* VTY standard output function. */
int
vty_out (struct vty *vty, const char *format, ...)
{
  va_list args;
  int len = 0;
  int size = 1024;
  char buf[1024];
  char *p = NULL;

  if (vty_shell (vty))
    {
      va_start (args, format);
      vprintf (format, args);
      va_end (args);
    }
  else
    {
      /* Try to write to initial buffer.  */
      va_start (args, format);
      len = vsnprintf (buf, sizeof buf, format, args);
      va_end (args);

      /* Initial buffer is not enough.  */
      if (len < 0 || len >= size)
	{
	  while (1)
	    {
	      if (len > -1)
		size = len + 1;
	      else
		size = size * 2;

	      p = XREALLOC (MTYPE_VTY_OUT_BUF, p, size);
	      if (! p)
		return -1;

	      va_start (args, format);
	      len = vsnprintf (p, size, format, args);
	      va_end (args);

	      if (len > -1 && len < size)
		break;
	    }
	}

      /* When initial buffer is enough to store all output.  */
      if (! p)
	p = buf;

      /* Pointer p must point out buffer. */
      buffer_put (vty->obuf, (u_char *) p, len);

      /* If p is not different with buf, it is allocated buffer.  */
      if (p != buf)
	XFREE (MTYPE_VTY_OUT_BUF, p);
    }

  return len;
}

/* Say hello to vty interface. */
void
vty_hello (struct vty *vty)
{
    vty_out (vty, host.motd);
}

/* Put out prompt and wait input from user. */
static void
vty_prompt (struct vty *vty)
{
  struct utsname names;
  const char*hostname;

    if (vty->type == VTY_TERM)
    {
        hostname = host.name;
        if (!hostname)
        {
            uname (&names);
            hostname = names.nodename;
        }
        switch(vty->node)
        {
            case USER_NODE:
            case AUTH_NODE:
            case VIEW_NODE:
            case AUTH_ENABLE_NODE:
            case ENABLE_NODE:
            case CONFIG_NODE:
            case DEBUG_NODE:
                vty_out (vty, cmd_prompt(vty->node), hostname);
                break;

            case CHANGE_PASSWD_NODE:
            case CHANGE_PASSWD_AGAIN_NODE:
                vty_out(vty, cmd_prompt(vty->node));
                break;
            default:
                vty_out (vty, "Carsmart#");
                break;
        }
    }

    return;
}

/* Send WILL TELOPT_ECHO to remote server. */
static void
vty_will_echo (struct vty *vty)
{
  unsigned char cmd[] = { IAC, WILL, TELOPT_ECHO, '\0' };
  vty_out (vty, "%s", cmd);
}

/* Make suppress Go-Ahead telnet option. */
static void
vty_will_suppress_go_ahead (struct vty *vty)
{
  unsigned char cmd[] = { IAC, WILL, TELOPT_SGA, '\0' };
  vty_out (vty, "%s", cmd);
}

/* Make don't use linemode over telnet. */
static void
vty_dont_linemode (struct vty *vty)
{
  unsigned char cmd[] = { IAC, DONT, TELOPT_LINEMODE, '\0' };
  vty_out (vty, "%s", cmd);
}

/* Use window size. */
static void
vty_do_window_size (struct vty *vty)
{
  unsigned char cmd[] = { IAC, DO, TELOPT_NAWS, '\0' };
  vty_out (vty, "%s", cmd);
}

/* Allocate new vty struct. */
struct vty *
vty_new ()
{
  struct vty *new = XCALLOC (MTYPE_VTY, sizeof (struct vty));

  new->obuf = buffer_new(0);	/* Use default buffer size. */
  new->cmd_input_buf = XCALLOC (MTYPE_VTY, VTY_BUFSIZ);
  new->max = VTY_BUFSIZ;

  return new;
}

/* 增加vty用户 */
static void registe_vty(struct vty *vty)
{
    int i;

    pthread_mutex_lock(&g_stVtyMutex);
    for (i=0; i<MAX_TELNET_NUM; ++i)
    {
        if(NULL == g_pstVtyArr[i])
        {
            g_pstVtyArr[i] = vty;
            vty->iVtyIndex = i;
            break;
        }
    }
    pthread_mutex_unlock(&g_stVtyMutex);

    /* 如果已经达到最大登陆限制 */
    if(MAX_TELNET_NUM == i)
    {
        vty_out(vty, "Has reached the maximum telnet login limit.%s", VTY_NEWLINE);
        vty->status = VTY_CLOSE;
    }

    return;
}

/* 删除vty用户 */
static void release_vty(struct vty *vty)
{
    pthread_mutex_lock(&g_stVtyMutex);
    if (vty->iVtyIndex < MAX_TELNET_NUM)
    {
        g_pstVtyArr[vty->iVtyIndex] = NULL;
    }
    pthread_mutex_unlock(&g_stVtyMutex);

    return;
}

/* 所有vty用户打印trap */
void put_all_vty_trap(char *pc_trap)
{
    int i;
    int len = strlen(pc_trap);

    pthread_mutex_lock(&g_stVtyMutex);
    for (i=0; i<MAX_TELNET_NUM; ++i)
    {
        if(NULL != g_pstVtyArr[i])
        {
            if (g_pstVtyArr[i]->fd > 0)
            {
                //vty_out(g_pstVtyArr[i], pc_trap);
                //vty_flush(g_pstVtyArr[i]);
                write(g_pstVtyArr[i]->pts, pc_trap, len);
            }
        }
    }
    pthread_mutex_unlock(&g_stVtyMutex);

    return;
}


/* Authentication of vty */
static void vty_auth (struct vty *vty, char *buf)
{
    int iNameLen;

    char *crypt (const char *, const char *);

    switch (vty->node)
    {
        case USER_NODE:
        {
            iNameLen = strlen(buf);
            if ((iNameLen>0) && (iNameLen <= MAX_USER_NAME_LEN))
            {
                strcpy(vty->pcBuf, buf);
                vty->node = AUTH_NODE;
            }
            return;
        }
        case AUTH_NODE:
        {
            vty->node = VIEW_NODE;
            if (CMD_SUCCESS == use_auth(vty->pcBuf, buf, &vty->iPriority))
            {
                vty->user_index = registe_user(vty->pcBuf, 1<<(vty->iVtyIndex+1));

                if (MAX_CLI_USER == vty->user_index)
                {
                    vty_out(vty, "Has reached the maximum user limit.%s", VTY_NEWLINE);
                    //vty->node = USER_NODE;
                    vty->status = VTY_CLOSE;
                }
            }
            else
            {
                if (vty->iInvLogin < 2)
                {
                    vty_out(vty, "Incorrect user name or password%s", VTY_NEWLINE);
                    vty->node = USER_NODE;
                    vty->iInvLogin++;
                }
                else
                {
                    vty_out(vty, "%% Bad passwords, too many failures! Retry after 60 seconds.%s", VTY_NEWLINE);
                    AddDos(vty->address);
                    vty->status = VTY_CLOSE;
                }
            }
            return;
        }

        case AUTH_ENABLE_NODE:
        {
            if(CMD_SUCCESS == enable_auth(buf))
            {
                vty->node = ENABLE_NODE;
            }
            else
            {
                vty->node = VIEW_NODE;
                VTY_OUT_EC(vty, "password error!%s", "密码无效!%s", VTY_NEWLINE);
            }
            return;
        }
    }
}

/* Command execution over the vty interface. */
int execute_command(struct vty *vty, char *buf)
{
    int ret;
    vector vline;

    /* Split readline string up into the vector */
    vline = cmd_make_strvec (buf);

    if (vline == NULL)
        return CMD_SUCCESS;

    ret = cmd_execute_command (vline, vty, 0);
    cmd_free_strvec (vline);

    switch (ret)
    {
        case CMD_SUCCESS:
            break;
        case CMD_WARNING:
            //VTY_OUT_EC(vty, "Warning...%s", VTY_NEWLINE);
            break;
        case CMD_ERR_AMBIGUOUS:
            VTY_OUT_EC(vty, "%% Ambiguous command.%s", "%% 有歧义的命令。%s", VTY_NEWLINE);
            break;
        case CMD_ERR_NO_MATCH:
            VTY_OUT_EC(vty, "%% Unknown command.%s", "%% 未知的命令。%s", VTY_NEWLINE);
            break;
        case CMD_ERR_INCOMPLETE:
            VTY_OUT_EC(vty, "%% Command incomplete.%s", "%% 未完成的命令。%s", VTY_NEWLINE);
            break;
        case CMD_ERR_PRIORITY_LOWER:
            VTY_OUT_EC(vty, "%% You Need higher priority!%s", "%% 你需要更高的权限来执行这个命令!%s", VTY_NEWLINE);
            break;

        default :
            VTY_OUT_EC(vty, "%% command: %s failed, error code is %d%s", "%% 执行命令%s失败%d!%s", buf, ret, VTY_NEWLINE);
            VTY_SET_UNSUCCESSFULLY(vty);
            VTY_UNKNOWN_FAIL(vty);
            break;
    }

    return ret;
}

/* When time out occur output message then close connection. */
static int vty_timeout (struct vty *vty)
{
    vty->v_timeout = 0;

    /* Clear buffer*/
    buffer_reset (vty->obuf);
    vty_out (vty, "%sVty connection is timed out.%s", VTY_NEWLINE, VTY_NEWLINE);

    /* Close connection. */
    vty->status = VTY_CLOSE;
    //vty_close (vty);

    return 0;
}

static const char telnet_backward_char = 0x08;
static const char telnet_space_char = ' ';

/* Basic function to write buffer to vty. */
static void
vty_write (struct vty *vty, const char *buf, size_t nbytes)
{
  if ((vty->node == AUTH_NODE) || (vty->node == AUTH_ENABLE_NODE) ||
     (vty->node == CHANGE_PASSWD_NODE) || (vty->node == CHANGE_PASSWD_AGAIN_NODE))
    return;

  /* Should we do buffering here ?  And make vty_flush (vty) ? */
  buffer_put (vty->obuf, buf, nbytes);
}

/* Ensure length of input buffer.  Is buffer is short, double it. */
static void
vty_ensure (struct vty *vty, int length)
{
  if (vty->max <= length)
    {
      vty->max *= 2;
      vty->cmd_input_buf = XREALLOC (MTYPE_VTY, vty->cmd_input_buf, vty->max);
    }
}

/* Basic function to insert character into vty. */
static void
vty_self_insert (struct vty *vty, char c)
{
  int i;
  int length;

  vty_ensure (vty, vty->length + 1);
  length = vty->length - vty->cp;
  memmove (&vty->cmd_input_buf[vty->cp + 1], &vty->cmd_input_buf[vty->cp], length);
  vty->cmd_input_buf[vty->cp] = c;

  vty_write (vty, &vty->cmd_input_buf[vty->cp], length + 1);
  for (i = 0; i < length; i++)
    vty_write (vty, &telnet_backward_char, 1);

  vty->cp++;
  vty->length++;
}

/* Self insert character 'c' in overwrite mode. */
static void
vty_self_insert_overwrite (struct vty *vty, char c)
{
  vty_ensure (vty, vty->length + 1);
  vty->cmd_input_buf[vty->cp++] = c;

  if (vty->cp > vty->length)
    vty->length++;

  if ((vty->node == AUTH_NODE) || (vty->node == AUTH_ENABLE_NODE) ||
     (vty->node == CHANGE_PASSWD_NODE) || (vty->node == CHANGE_PASSWD_AGAIN_NODE))
    return;

  vty_write (vty, &c, 1);
}

/* Insert a word into vty interface with overwrite mode. */
static void
vty_insert_word_overwrite (struct vty *vty, char *str)
{
  int len = strlen (str);
  vty_write (vty, str, len);
  strcpy (&vty->cmd_input_buf[vty->cp], str);
  vty->cp += len;
  vty->length = vty->cp;
}

/* Forward character. */
static void
vty_forward_char (struct vty *vty)
{
  if (vty->cp < vty->length)
    {
      vty_write (vty, &vty->cmd_input_buf[vty->cp], 1);
      vty->cp++;
    }
}

/* Backward character. */
static void
vty_backward_char (struct vty *vty)
{
  if (vty->cp > 0)
    {
      vty->cp--;
      vty_write (vty, &telnet_backward_char, 1);
    }
}

/* Move to the beginning of the line. */
static void
vty_beginning_of_line (struct vty *vty)
{
  while (vty->cp)
    vty_backward_char (vty);
}

/* Move to the end of the line. */
static void
vty_end_of_line (struct vty *vty)
{
  while (vty->cp < vty->length)
    vty_forward_char (vty);
}

static void vty_kill_line_from_beginning (struct vty *);
static void vty_redraw_line (struct vty *);

/* Print command line history.  This function is called from
   vty_next_line and vty_previous_line. */
static void
vty_history_print (struct vty *vty)
{
  int length;

  vty_kill_line_from_beginning (vty);

  /* Get previous line from history buffer */
  length = strlen (vty->hist[vty->hp]);
  memcpy (vty->cmd_input_buf, vty->hist[vty->hp], length);
  vty->cp = vty->length = length;

  /* Redraw current line */
  vty_redraw_line (vty);
}

/* Show next command line history. */
static void
vty_next_line (struct vty *vty)
{
  int try_index;

  if (vty->hp == vty->hindex)
    return;

  /* Try is there history exist or not. */
  try_index = vty->hp;
  if (try_index == (VTY_MAXHIST - 1))
    try_index = 0;
  else
    try_index++;

  /* If there is not history return. */
  if (vty->hist[try_index] == NULL)
    return;
  else
    vty->hp = try_index;

  vty_history_print (vty);
}

/* Show previous command line history. */
static void
vty_previous_line (struct vty *vty)
{
  int try_index;

  try_index = vty->hp;
  if (try_index == 0)
    try_index = VTY_MAXHIST - 1;
  else
    try_index--;

  if (vty->hist[try_index] == NULL)
    return;
  else
    vty->hp = try_index;

  vty_history_print (vty);
}

/* This function redraw all of the command line character. */
static void
vty_redraw_line (struct vty *vty)
{
  vty_write (vty, vty->cmd_input_buf, vty->length);
  vty->cp = vty->length;
}

/* Forward word. */
static void
vty_forward_word (struct vty *vty)
{
  while (vty->cp != vty->length && vty->cmd_input_buf[vty->cp] != ' ')
    vty_forward_char (vty);

  while (vty->cp != vty->length && vty->cmd_input_buf[vty->cp] == ' ')
    vty_forward_char (vty);
}

/* Backward word without skipping training space. */
static void
vty_backward_pure_word (struct vty *vty)
{
  while (vty->cp > 0 && vty->cmd_input_buf[vty->cp - 1] != ' ')
    vty_backward_char (vty);
}

/* Backward word. */
static void
vty_backward_word (struct vty *vty)
{
  while (vty->cp > 0 && vty->cmd_input_buf[vty->cp - 1] == ' ')
    vty_backward_char (vty);

  while (vty->cp > 0 && vty->cmd_input_buf[vty->cp - 1] != ' ')
    vty_backward_char (vty);
}

/* When '^D' is typed at the beginning of the line we move to the down
   level. */
static void
vty_down_level (struct vty *vty)
{
  vty_out (vty, "%s", VTY_NEWLINE);
  (*config_exit_cmd.func)(NULL, vty, 0, NULL);
  vty_prompt (vty);
  vty->cp = 0;
}

/* When '^Z' is received from vty, move down to the enable mode. */
static void
vty_end_config (struct vty *vty)
{
  vty_out (vty, "%s", VTY_NEWLINE);

  switch (vty->node)
    {
    case VIEW_NODE:
    case ENABLE_NODE:
      /* Nothing to do. */
      break;
    case CONFIG_NODE:
      vty->node = ENABLE_NODE;
      break;
    default:
      /* Unknown node, we have to ignore it. */
      break;
    }

  vty_prompt (vty);
  memset (vty->cmd_input_buf, 0, vty->max);
  vty->length = 0;
  vty->cp = 0;
}

/* Delete a charcter at the current point. */
static void
vty_delete_char (struct vty *vty)
{
  int i;
  int size;

  if (vty->length == 0)
    {
      vty_down_level (vty);
      return;
    }

  if (vty->cp == vty->length)
    return;			/* completion need here? */

  size = vty->length - vty->cp;

  vty->length--;
  memmove (&vty->cmd_input_buf[vty->cp], &vty->cmd_input_buf[vty->cp + 1], size - 1);
  vty->cmd_input_buf[vty->length] = '\0';

  if ((vty->node == AUTH_NODE) || (vty->node == AUTH_ENABLE_NODE) ||
     (vty->node == CHANGE_PASSWD_NODE) || (vty->node == CHANGE_PASSWD_AGAIN_NODE))
    return;

  vty_write (vty, &vty->cmd_input_buf[vty->cp], size - 1);
  vty_write (vty, &telnet_space_char, 1);

  for (i = 0; i < size; i++)
    vty_write (vty, &telnet_backward_char, 1);
}

/* Delete a character before the point. */
static void
vty_delete_backward_char (struct vty *vty)
{
  if (vty->cp == 0)
    return;

  vty_backward_char (vty);
  vty_delete_char (vty);
}

/* Kill rest of line from current point. */
static void
vty_kill_line (struct vty *vty)
{
  int i;
  int size;

  size = vty->length - vty->cp;

  if (size == 0)
    return;

  for (i = 0; i < size; i++)
    vty_write (vty, &telnet_space_char, 1);
  for (i = 0; i < size; i++)
    vty_write (vty, &telnet_backward_char, 1);

  memset (&vty->cmd_input_buf[vty->cp], 0, size);
  vty->length = vty->cp;
}

/* Kill line from the beginning. */
static void
vty_kill_line_from_beginning (struct vty *vty)
{
  vty_beginning_of_line (vty);
  vty_kill_line (vty);
}

/* Delete a word before the point. */
static void
vty_forward_kill_word (struct vty *vty)
{
  while (vty->cp != vty->length && vty->cmd_input_buf[vty->cp] == ' ')
    vty_delete_char (vty);
  while (vty->cp != vty->length && vty->cmd_input_buf[vty->cp] != ' ')
    vty_delete_char (vty);
}

/* Delete a word before the point. */
static void
vty_backward_kill_word (struct vty *vty)
{
  while (vty->cp > 0 && vty->cmd_input_buf[vty->cp - 1] == ' ')
    vty_delete_backward_char (vty);
  while (vty->cp > 0 && vty->cmd_input_buf[vty->cp - 1] != ' ')
    vty_delete_backward_char (vty);
}

/* Transpose chars before or at the point. */
static void
vty_transpose_chars (struct vty *vty)
{
  char c1, c2;

  /* If length is short or point is near by the beginning of line then
     return. */
  if (vty->length < 2 || vty->cp < 1)
    return;

  /* In case of point is located at the end of the line. */
  if (vty->cp == vty->length)
    {
      c1 = vty->cmd_input_buf[vty->cp - 1];
      c2 = vty->cmd_input_buf[vty->cp - 2];

      vty_backward_char (vty);
      vty_backward_char (vty);
      vty_self_insert_overwrite (vty, c1);
      vty_self_insert_overwrite (vty, c2);
    }
  else
    {
      c1 = vty->cmd_input_buf[vty->cp];
      c2 = vty->cmd_input_buf[vty->cp - 1];

      vty_backward_char (vty);
      vty_self_insert_overwrite (vty, c1);
      vty_self_insert_overwrite (vty, c2);
    }
}

/* Stupid comparison routine for qsort () ing strings. */
static int string_compare (const void *p1, const void *p2)
{
	return strcmp(* (char * const *) p1, * (char * const *) p2);
}

/* Do completion at vty interface. */
static void
vty_complete_command (struct vty *vty)
{
  int i;
  int ret;
  char **matched = NULL;
  vector vline;
  int tmp;
  int match_num;
  int line;

  if (vty->node == USER_NODE || vty->node == AUTH_NODE || vty->node == AUTH_ENABLE_NODE ||
     (vty->node == CHANGE_PASSWD_NODE) || (vty->node == CHANGE_PASSWD_AGAIN_NODE))
    return;

  vline = cmd_make_strvec (vty->cmd_input_buf);
  if (vline == NULL)
    return;

  /* In case of 'help \t'. */
  if (isspace ((int) vty->cmd_input_buf[vty->length - 1]))
    vector_set (vline, '\0');

  matched = cmd_complete_command (vline, vty, &ret);

  cmd_free_strvec (vline);

  vty_out (vty, "%s", VTY_NEWLINE);
  switch (ret)
    {
    case CMD_ERR_AMBIGUOUS:
      VTY_OUT_EC(vty, "%% Ambiguous command.%s", "%% 有歧义的命令。%s", VTY_NEWLINE);
      vty_prompt (vty);
      vty_redraw_line (vty);
      break;
    case CMD_ERR_NO_MATCH:
      /* vty_out (vty, "%% There is no matched command.%s", VTY_NEWLINE); */
      vty_prompt (vty);
      vty_redraw_line (vty);
      break;
    case CMD_COMPLETE_FULL_MATCH:
      vty_prompt (vty);
      vty_redraw_line (vty);
      vty_backward_pure_word (vty);
      vty_insert_word_overwrite (vty, matched[0]);
      vty_self_insert (vty, ' ');
      XFREE (MTYPE_TMP, matched[0]);
      break;
    case CMD_COMPLETE_MATCH:
      vty_prompt (vty);
      vty_redraw_line (vty);
      vty_backward_pure_word (vty);
      vty_insert_word_overwrite (vty, matched[0]);
      XFREE (MTYPE_TMP, matched[0]);
      vector_only_index_free (matched);
      return;
      break;
    case CMD_COMPLETE_LIST_MATCH:
		for (match_num = 0; matched[match_num]; match_num++)
			;
		
		qsort (matched, match_num, sizeof (char *), string_compare);

		/* 每行3列，计算总行数 */
		line = match_num/3;
		if ((match_num%3) != 0)
		{
			++line;
		}

		/* 一行一行的打印 */
		for (i = 1; i<=line; i++)
		{
			tmp = i-1;
			if (match_num > tmp)
			{
				vty_out (vty, "%-20s ", matched[tmp]);
				XFREE (MTYPE_TMP, matched[tmp]);
				if (match_num > line + tmp)
				{
					vty_out (vty, "%-20s ", matched[line + tmp]);
					XFREE (MTYPE_TMP, matched[line + tmp]);
					if (match_num > 2*line + tmp)
					{
						vty_out (vty, "%-20s ", matched[2*line + tmp]);
						XFREE (MTYPE_TMP, matched[2*line + tmp]);
					}
				}
			}
			vty_out (vty, "%s", VTY_NEWLINE);
		}

		vty_prompt (vty);
		vty_redraw_line (vty);
		break;
    case CMD_ERR_NOTHING_TODO:
      vty_prompt (vty);
      vty_redraw_line (vty);
      break;
    default:
      break;
    }
  if (matched)
    vector_only_index_free (matched);
}

static void
vty_describe_fold (struct vty *vty, int cmd_width,
		   unsigned int deoam_width, struct desc *desc)
{
  char *buf;
  const char *cmd, *p;
  int pos;

  cmd = desc->cmd[0] == '.' ? desc->cmd + 1 : desc->cmd;

  if (deoam_width <= 0)
    {
      vty_out (vty, "  %-*s  %s%s", cmd_width, cmd, desc->str[vty->language], VTY_NEWLINE);
      return;
    }

  buf = XCALLOC (MTYPE_TMP, strlen (desc->str[vty->language]) + 1);

  for (p = desc->str[vty->language]; strlen (p) > deoam_width; p += pos + 1)
    {
      for (pos = deoam_width; pos > 0; pos--)
      if (*(p + pos) == ' ')
        break;

      if (pos == 0)
      break;

      strncpy (buf, p, pos);
      buf[pos] = '\0';
      vty_out (vty, "  %-*s  %s%s", cmd_width, cmd, buf, VTY_NEWLINE);

      cmd = "";
    }

  vty_out (vty, "  %-*s  %s%s", cmd_width, cmd, p, VTY_NEWLINE);

  XFREE (MTYPE_TMP, buf);
}

/* Describe matched command function. */
static void
vty_describe_command (struct vty *vty)
{
  int ret;
  vector vline;
  vector describe;
  unsigned int i, width, deoam_width;
  struct desc *desc, *deoam_cr = NULL;

  vline = cmd_make_strvec (vty->cmd_input_buf);

  /* In case of '> ?'. */
  if (vline == NULL)
    {
      vline = vector_init (1);
      vector_set (vline, '\0');
    }
  else
    if (isspace ((int) vty->cmd_input_buf[vty->length - 1]))
      vector_set (vline, '\0');

  describe = cmd_describe_command (vline, vty, &ret);

  vty_out (vty, "%s", VTY_NEWLINE);

  /* Ambiguous error. */
  switch (ret)
    {
    case CMD_ERR_AMBIGUOUS:
      VTY_OUT_EC(vty, "%% Ambiguous command.%s", "%% 有歧义的命令。%s", VTY_NEWLINE);
      goto out;
      break;
    case CMD_ERR_NO_MATCH:
      VTY_OUT_EC(vty, "%% Unknown command.%s", "%% 未知的命令。%s", VTY_NEWLINE);
      goto out;
      break;
    }

  /* Get width of command string. */
  width = 0;
  for (i = 0; i < vector_active (describe); i++)
    if ((desc = vector_slot (describe, i)) != NULL)
      {
	unsigned int len;

	if (desc->cmd[0] == '\0')
	  continue;

	len = strlen (desc->cmd);
	if (desc->cmd[0] == '.')
	  len--;

	if (width < len)
	  width = len;
      }

  /* Get width of description string. */
  deoam_width = vty->width - (width + 6);

  /* Print out description. */
  for (i = 0; i < vector_active (describe); i++)
    if ((desc = vector_slot (describe, i)) != NULL)
      {
	if (desc->cmd[0] == '\0')
	  continue;

	if (strcmp (desc->cmd, command_cr) == 0)
	  {
	    deoam_cr = desc;
	    continue;
	  }

	if (!desc->str[vty->language])
	  vty_out (vty, "  %-s%s",
		   desc->cmd[0] == '.' ? desc->cmd + 1 : desc->cmd,
		   VTY_NEWLINE);
	else if (deoam_width >= strlen (desc->str[vty->language]))
	  vty_out (vty, "  %-*s  %s%s", width,
		   desc->cmd[0] == '.' ? desc->cmd + 1 : desc->cmd,
		   desc->str[vty->language], VTY_NEWLINE);
	else
	  vty_describe_fold (vty, width, deoam_width, desc);

#if 0
	vty_out (vty, "  %-*s %s%s", width
		 desc->cmd[0] == '.' ? desc->cmd + 1 : desc->cmd,
		 desc->str ? desc->str : "", VTY_NEWLINE);
#endif /* 0 */
      }

  if ((desc = deoam_cr))
    {
      if (!desc->str[vty->language])
	vty_out (vty, "  %-s%s",
		 desc->cmd[0] == '.' ? desc->cmd + 1 : desc->cmd,
		 VTY_NEWLINE);
      else if (deoam_width >= strlen (desc->str[vty->language]))
	vty_out (vty, "  %-*s  %s%s", width,
		 desc->cmd[0] == '.' ? desc->cmd + 1 : desc->cmd,
		 desc->str[vty->language], VTY_NEWLINE);
      else
	vty_describe_fold (vty, width, deoam_width, desc);
    }

out:
  cmd_free_strvec (vline);
  if (describe)
    vector_free (describe);

  vty_prompt (vty);
  vty_redraw_line (vty);
}

static void
vty_clear_buf (struct vty *vty)
{
  memset (vty->cmd_input_buf, 0, vty->max);
}

/* ^C stop current input and do not add command line to the history. */
static void vty_stop_input (struct vty *vty)
{
    vty->iBreak = 1;

    if (vty->stChildPId > 0)
    {
        kill(vty->stChildPId, SIGINT);
        vty->stChildPId = 0;
    }
    return;
}

static int vty_stop_nead (struct vty *vty)
{
    if (0 == vty->iBreak || vty->stChildPId > 0)
    {
        return 1;
    }
    return 0;
}


/* Add current command line to the history buffer. */
static void vty_hist_add (struct vty *vty)
{
  int index;

  if (vty->length == 0)
    return;

  index = vty->hindex ? vty->hindex - 1 : VTY_MAXHIST - 1;

  /* Ignore the same string as previous one. */
  if (vty->hist[index])
    if (strcmp (vty->cmd_input_buf, vty->hist[index]) == 0)
      {
      vty->hp = vty->hindex;
      return;
      }

  /* Insert history entry. */
  if (vty->hist[vty->hindex])
    XFREE (MTYPE_VTY_HIST, vty->hist[vty->hindex]);
  vty->hist[vty->hindex] = XSTRDUP (MTYPE_VTY_HIST, vty->cmd_input_buf);

  /* History index rotation. */
  vty->hindex++;
  if (vty->hindex == VTY_MAXHIST)
    vty->hindex = 0;

  vty->hp = vty->hindex;
}

/* #define TELNET_OPTION_DEBUG */

/* Get telnet window size. */
static int
vty_telnet_option (struct vty *vty, unsigned char *buf, int nbytes)
{
#ifdef TELNET_OPTION_DEBUG
  int i;

  for (i = 0; i < nbytes; i++)
    {
      switch (buf[i])
	{
	case IAC:
	  vty_out (vty, "IAC ");
	  break;
	case WILL:
	  vty_out (vty, "WILL ");
	  break;
	case WONT:
	  vty_out (vty, "WONT ");
	  break;
	case DO:
	  vty_out (vty, "DO ");
	  break;
	case DONT:
	  vty_out (vty, "DONT ");
	  break;
	case SB:
	  vty_out (vty, "SB ");
	  break;
	case SE:
	  vty_out (vty, "SE ");
	  break;
	case TELOPT_ECHO:
	  vty_out (vty, "TELOPT_ECHO %s", VTY_NEWLINE);
	  break;
	case TELOPT_SGA:
	  vty_out (vty, "TELOPT_SGA %s", VTY_NEWLINE);
	  break;
	case TELOPT_NAWS:
	  vty_out (vty, "TELOPT_NAWS %s", VTY_NEWLINE);
	  break;
	default:
	  vty_out (vty, "%x ", buf[i]);
	  break;
	}
    }
  vty_out (vty, "%s", VTY_NEWLINE);

#endif /* TELNET_OPTION_DEBUG */

  switch (buf[0])
    {
    case SB:
      vty->sb_len = 0;
      vty->iac_sb_in_progress = 1;
      return 0;
      break;
    case SE:
      {
	if (!vty->iac_sb_in_progress)
	  return 0;

	if ((vty->sb_len == 0) || (vty->sb_buf[0] == '\0'))
	  {
	    vty->iac_sb_in_progress = 0;
	    return 0;
	  }
	switch (vty->sb_buf[0])
	  {
	  case TELOPT_NAWS:
	    if (vty->sb_len != TELNET_NAWS_SB_LEN)
	      printf("RFC 1073 violation detected: telnet NAWS option "
			"should send %d characters, but we received %lu",
			TELNET_NAWS_SB_LEN, (u_long)vty->sb_len);
	    else if (sizeof(vty->sb_buf) < TELNET_NAWS_SB_LEN)
	      printf("Bug detected: sizeof(vty->sb_buf) %lu < %d, "
		       "too small to handle the telnet NAWS option",
		       (u_long)sizeof(vty->sb_buf), TELNET_NAWS_SB_LEN);
	    else
	      {
		vty->width = ((vty->sb_buf[1] << 8)|vty->sb_buf[2]);
		vty->height = ((vty->sb_buf[3] << 8)|vty->sb_buf[4]);
#ifdef TELNET_OPTION_DEBUG
		vty_out(vty, "TELNET NAWS window size negotiation completed: "
			      "width %d, height %d%s",
			vty->width, vty->height, VTY_NEWLINE);
#endif
	      }
	    break;
	  }
	vty->iac_sb_in_progress = 0;
	return 0;
	break;
      }
    default:
      break;
    }
  return 1;
}

static int change_vty_passwd(struct vty *vty)
{
    int iPasswdLen;
    int iErrCode = CMD_SUCCESS;

    extern int set_enable_passwd(char *pcPasswd);
    extern int set_user_passwd(char *pcName, char *pcPasswd);

    if (CHANGE_PASSWD_NODE == vty->node)
    {
        iPasswdLen = snprintf(vty->pcBuf, (MAX_PASSWD_LEN+1), "%s", vty->cmd_input_buf);
        if((iPasswdLen > MAX_PASSWD_LEN) || (iPasswdLen <= 0))
        {
            INPUT_PASSWD_TOO_LENGTH(vty);
            iErrCode = CMD_WARNING;
            vty->node = ENABLE_NODE;
        }
        else
        {
            vty->node = CHANGE_PASSWD_AGAIN_NODE;
        }
    }
    /* CHANGE_PASSWD_AGAIN_NODE == vty->node */
    else
    {
        if (0 == strcmp(vty->pcBuf,  vty->cmd_input_buf))
        {
            if (CHANGE_USER_PASSWOED == vty->iChangePasswdType)
            {
                iErrCode = set_user_passwd(g_astUserArr[vty->user_index].szName, vty->pcBuf);
            }
            else
            {
                iErrCode = set_enable_passwd(vty->pcBuf);
            }
        }
        else
        {
            TWO_INPUT_PASSWD_DIFF(vty);
            iErrCode = CMD_WARNING;
        }

        vty->node = ENABLE_NODE;
    }

    return iErrCode;
}

/* Execute current command line. */
static int vty_execute (struct vty *vty)
{
  int ret;

  ret = CMD_SUCCESS;

  switch (vty->node)
    {
    case USER_NODE:
    case AUTH_NODE:
    case AUTH_ENABLE_NODE:
      vty_auth (vty, vty->cmd_input_buf);
      break;
    case CHANGE_PASSWD_NODE:
    case CHANGE_PASSWD_AGAIN_NODE:
        change_vty_passwd(vty);
        break;
    default:
      ret = execute_command(vty, vty->cmd_input_buf);

      vty_hist_add (vty);
      break;
    }

  /* Clear command line buffer. */
  vty->cp = vty->length = 0;
  vty_clear_buf (vty);

  if (vty->status != VTY_CLOSE )
    vty_prompt (vty);

  return ret;
}

#define CONTROL(X)  ((X) - '@')
#define VTY_NORMAL     0
#define VTY_PRE_ESCAPE 1
#define VTY_ESCAPE     2

/* Escape character command map. */
static void
vty_escape_map (unsigned char c, struct vty *vty)
{
  switch (c)
    {
    case ('A'):
      vty_previous_line (vty);
      break;
    case ('B'):
      vty_next_line (vty);
      break;
    case ('C'):
      vty_forward_char (vty);
      break;
    case ('D'):
      vty_backward_char (vty);
      break;
    default:
      break;
    }

  /* Go back to normal mode. */
  vty->escape = VTY_NORMAL;
}

/* Quit print out to the buffer. */
static void
vty_buffer_reset (struct vty *vty)
{
  buffer_reset (vty->obuf);
  vty_prompt (vty);
  vty_redraw_line (vty);
}

/* Read data via vty socket. */
static int vty_read (struct vty *vty /* struct thread *thread */)
{
    int i;
    int nbytes = 0;
    unsigned char buf[VTY_READ_BUFSIZ] = {0};

    pthread_mutex_lock(&vty->ring.mutex);
    pthread_cond_wait(&vty->cond, &vty->ring.mutex);
    pthread_mutex_unlock(&vty->ring.mutex);

    /* Read raw data from socket */
    do
    {
        nbytes = loop_buf_pop_n(&vty->ring, buf, VTY_READ_BUFSIZ);

        for (i = 0; i < nbytes; i++)
        {
            if (buf[i] == IAC)
            {
                if (!vty->iac)
                {
                    vty->iac = 1;
                    continue;
                }
                else
                {
                    vty->iac = 0;
                }
            }

            if (vty->iac_sb_in_progress && !vty->iac)
            {
                if (vty->sb_len < sizeof(vty->sb_buf))
                    vty->sb_buf[vty->sb_len] = buf[i];
                vty->sb_len++;
                continue;
            }

            if (vty->iac)
            {
                /* In case of telnet command */
                int ret = 0;
                ret = vty_telnet_option (vty, buf + i, nbytes - i);
                vty->iac = 0;
                i += ret;
                continue;
            }


            if (vty->status == VTY_MORE)
            {
                switch (buf[i])
                {
                    case CONTROL('C'):
                    case 'q':
                    case 'Q':
                        vty_buffer_reset (vty);
                        break;
#if 0 /* More line does not work for "show ip bgp".  */
                    case '\n':
                    case '\r':
                    vty->status = VTY_MORELINE;
                    break;
#endif
                    default:
                        break;
                }
                continue;
            }

            /* Escape character. */
            if (vty->escape == VTY_ESCAPE)
            {
                vty_escape_map (buf[i], vty);
                continue;
            }

            /* Pre-escape status. */
            if (vty->escape == VTY_PRE_ESCAPE)
            {
                switch (buf[i])
                {
                    case '[':
                        vty->escape = VTY_ESCAPE;
                        break;
                    case 'b':
                        vty_backward_word (vty);
                        vty->escape = VTY_NORMAL;
                        break;
                    case 'f':
                        vty_forward_word (vty);
                        vty->escape = VTY_NORMAL;
                        break;
                    case 'd':
                        vty_forward_kill_word (vty);
                        vty->escape = VTY_NORMAL;
                        break;
                    case CONTROL('H'):
                    case 0x7f:
                        vty_backward_kill_word (vty);
                        vty->escape = VTY_NORMAL;
                        break;
                    default:
                        vty->escape = VTY_NORMAL;
                        break;
                }
                continue;
            }

            switch (buf[i])
            {
                case CONTROL('A'):
                    vty_beginning_of_line (vty);
                    break;
                case CONTROL('B'):
                    vty_backward_char (vty);
                    break;
                case CONTROL('C'):
                    //vty_stop_input (vty);
                    break;
                case CONTROL('D'):
                    vty_delete_char (vty);
                    break;
                case CONTROL('E'):
                    vty_end_of_line (vty);
                    break;
                case CONTROL('F'):
                    vty_forward_char (vty);
                    break;
                case CONTROL('H'):
                case 0x7f:
                    vty_delete_backward_char (vty);
                    break;
                case CONTROL('K'):
                    vty_kill_line (vty);
                    break;
                case CONTROL('N'):
                    vty_next_line (vty);
                    break;
                case CONTROL('P'):
                    vty_previous_line (vty);
                    break;
                case CONTROL('T'):
                    vty_transpose_chars (vty);
                    break;
                case CONTROL('U'):
                    vty_kill_line_from_beginning (vty);
                    break;
                case CONTROL('W'):
                    vty_backward_kill_word (vty);
                    break;
                case CONTROL('Z'):
                    vty_end_config (vty);
                    break;
                case '\n':
                    //if (i>0 && '\r' == buf[i-1])
                    {
                        break;
                    }
                case '\r':
                    vty_out (vty, "%s", VTY_NEWLINE);
                    vty_flush(vty);
                    vty_execute (vty);
                    break;
                case '\t':
                    vty_complete_command (vty);
                    break;
                case '?':
                    if (vty->node == USER_NODE || vty->node == AUTH_NODE || vty->node == AUTH_ENABLE_NODE ||
                        (vty->node == CHANGE_PASSWD_NODE) || (vty->node == CHANGE_PASSWD_AGAIN_NODE))
                        vty_self_insert (vty, buf[i]);
                    else
                        vty_describe_command (vty);
                    break;
                case '\033':
                    if (i + 1 < nbytes && buf[i + 1] == '[')
                    {
                        vty->escape = VTY_ESCAPE;
                        i++;
                    }
                    else
                        vty->escape = VTY_PRE_ESCAPE;
                    break;
                default:
                    if (buf[i] > 31 && buf[i] < 127)
                        vty_self_insert (vty, buf[i]);
                    break;
            }
        }

        /* Check status. */
        if (vty->status == VTY_CLOSE)
        {
            //vty_close (vty);
            break;
        }
    }while (!loop_buf_isempty(&vty->ring));

    return 0;
}

/* Read data via vty socket. */
static int vty_read_ctrl(struct vty *vty /* struct thread *thread */)
{
    int i;
    int nbytes;
    unsigned char buf[VTY_READ_BUFSIZ] = {0};
    struct timeval telnet_timeout;/*select timeout*/
    struct timeval *p_st_telnet_timeout;/*select timeout*/
    fd_set rdfdset;
    int ret;
    int max_fd;
    int ctrl_c = 0;
	int cnt = 0;

    FD_ZERO(&rdfdset);
    FD_SET(vty->fd, &rdfdset);
    FD_SET(vty->ptm, &rdfdset);

    max_fd = MAX(vty->fd, vty->ptm);

    if (0 == vty->v_timeout)
    {
        p_st_telnet_timeout = NULL;
    }
    else
    {
        p_st_telnet_timeout = &telnet_timeout;
        telnet_timeout.tv_sec = vty->v_timeout;
        telnet_timeout.tv_usec = 0;
    }
    ret = select(max_fd + 1, &rdfdset, NULL, NULL, p_st_telnet_timeout);

    if (-1 == ret)
    {
        printf("select\n");
        return 0;
    }
    else if (0 == ret)
    {
        vty_timeout(vty);
        return 0;
    }
    else
    {
        if (FD_ISSET(vty->ptm, &rdfdset))
        {
            memset(buf, 0, VTY_READ_BUFSIZ);
            while ((cnt = read(vty->ptm, buf, VTY_READ_BUFSIZ - 1)) > 0)
            {
            	buf[cnt] = '\0';
                vty_out(vty, "%s", buf);
                if (BUFFER_ERROR == vty_flush(vty))               
                {
                    break;
                }
            }
        }
        if (FD_ISSET(vty->fd, &rdfdset))
        {
            /* Read raw data from socket */
            if ((nbytes = read (vty->fd, buf, VTY_READ_BUFSIZ)) <= 0)
            {
                if (nbytes < 0)
                {
                    if (ERRNO_IO_RETRY(errno))
                    {
                        return 0;
                    }

                    vty->monitor = 0; /* disable monitoring to avoid infinite recursion */
                }
                else
                {
                    vty->status = VTY_CLOSE;
                    return 0;
                }
            }

            /* 检查是否收到的CTRL+C,如果收到，停止正在执行的程序，清空之前的命令 */
            for (i = 0; i < nbytes; i++)
            {
                if (CONTROL('C') == buf[i])
                {
                    ctrl_c = 1;
                    break;
                }
            }

            if (1 == ctrl_c && vty_stop_nead(vty))
            {
                loop_buf_reset(&vty->ring);
                /* Clear command line buffer. */
                vty->cp = vty->length = 0;
                vty_clear_buf (vty);
                
                vty_stop_input (vty);
            }
            else
            {   
                /* 把收到的命令压入缓冲区，通知执行线程执行,如果缓冲区已经满，sleep 1s后继续压 */
                while (nbytes != loop_buf_push_n(&vty->ring, buf, nbytes))
                {
                    sleep(1);
                }                
                pthread_cond_signal(&vty->cond);
            }
        }
    }

    return 0;
}

int vty_in_line(struct vty *vty, char* pcLine, int iMaxSize)
{
    int i;
    int nbytes;
    int iCount = 0;
    unsigned char buf[VTY_READ_BUFSIZ];

    int cmd_input_buf_len = strlen(vty->cmd_input_buf);

	if ((NULL == pcLine) || (0 == iMaxSize))
	{
		return OSAL_ERROR;
	}

	memset(pcLine, 0, iMaxSize);

	do{
        if (loop_buf_isempty(&vty->ring))
        {
            pthread_mutex_lock(&vty->ring.mutex);
            pthread_cond_wait(&vty->cond, &vty->ring.mutex);
            pthread_mutex_unlock(&vty->ring.mutex);
        }
		/*读取字符*/
        if ((nbytes = loop_buf_pop_n(&vty->ring, buf, VTY_READ_BUFSIZ)) <= 0)
        {
            if (nbytes < 0)
            {
                if (ERRNO_IO_RETRY(errno))
                {
                    return 0;
                }

                vty->monitor = 0; /* disable monitoring to avoid infinite recursion */
                //printf("read error on vty client fd %d, closing: %s", vty->fd, safe_strerror(errno));
            }
        }

        for (i = 0; i < nbytes; i++)
        {
            if (buf[i] == IAC)
            {
                if (!vty->iac)
                {
                    vty->iac = 1;
                    continue;
                }
                else
                {
                    vty->iac = 0;
                }
            }

            if (vty->iac_sb_in_progress && !vty->iac)
            {
                if (vty->sb_len < sizeof(vty->sb_buf))
                    vty->sb_buf[vty->sb_len] = buf[i];
                vty->sb_len++;
                continue;
            }

            if (vty->iac)
            {
                /* In case of telnet command */
                int ret = 0;
                ret = vty_telnet_option (vty, buf + i, nbytes - i);
                vty->iac = 0;
                i += ret;
                continue;
            }
            switch (buf[i])
            {
                case CONTROL('H'):
                case 0x7f:
                    if (iCount > 0)
                    {
                    	vty_delete_backward_char (vty);
                    	--iCount;
                    	pcLine[iCount] = 0;
                    	vty_flush(vty);
                    }
                    break;
                case '\n':
                    break;
                case '\r':
                    vty_out (vty, "%s", VTY_NEWLINE);
                	vty->cmd_input_buf[cmd_input_buf_len] = 0;
                    return iCount;
              	default:
                    if (buf[i] > 31 && buf[i] < 127)
                    {
                    	if (iCount < (iMaxSize-1))
                    	{
                    		pcLine[iCount] = buf[i];
                    		++iCount;
                    		vty_self_insert (vty, buf[i]);
   	                        vty_flush(vty);
                    	}
                    }
                    break;
            }
        }
	}while(1);

    return 0;
}

/* Flush buffer to the vty. */
int vty_flush (struct vty *vty/* struct thread *thread */)
{
    int erase;
    buffer_status_t flushrc;

    /* Function execution continue. */
    erase = ((vty->status == VTY_MORE || vty->status == VTY_MORELINE));

    /* N.B. if width is 0, that means we don't know the window size. */
    if ((vty->lines == 0) || (vty->width == 0))
        flushrc = buffer_flush_available(vty->obuf, vty->fd);
    else if (vty->status == VTY_MORELINE)
        flushrc = buffer_flush_window(vty->obuf, vty->fd, vty->width, 1, erase, 0);
    else
        flushrc = buffer_flush_window(vty->obuf, vty->fd, vty->width,
                                      vty->lines >= 0 ? vty->lines : vty->height,
                                      erase, 0);

    switch (flushrc)
    {
        case BUFFER_ERROR:
            vty->monitor = 0; /* disable monitoring to avoid infinite recursion */
            //printf("buffer_flush failed on vty client fd %d, closing\n", vty->fd);
            buffer_reset(vty->obuf);
            //vty_close(vty);
            vty->status = VTY_CLOSE;
            return BUFFER_ERROR;
            break;
        case BUFFER_EMPTY:
            if (vty->status != VTY_CLOSE)
            {
                vty->status = VTY_NORMAL;
            }
            else
            {
                write(vty->pts, "\r\n", 3);
            }
            break;
        case BUFFER_PENDING:
            /* There is more data waiting to be written. */
            vty->status = VTY_MORE;
            break;
    }

    return 0;
}

int vty_out_flush (struct vty *vty)
{
    if (vty_shell (vty))
    {
        return fflush(stdout);
    }
    else
    {
        return vty_flush(vty);
    }
}

/* Create new vty structure. */
static struct vty *vty_create (int vty_sock, union sockunion *su)
{
    struct vty *vty;

    /* Allocate new vty structure and set up default values. */
    vty = vty_new ();
    vty->fd = vty_sock;
    vty->type = VTY_TERM;
    vty->language = CMD_HELP_LANG_ENGLISH;
    vty->address = sockunion_su2str (su);
    if (no_password_check)
    {
        vty->node = VIEW_NODE;
    }
    else
    {
        vty->node = USER_NODE;
    }

    vty->user_index = MAX_CLI_USER;
    vty->pcBuf = (char*)malloc(128);
    vty->iVtyIndex = MAX_TELNET_NUM;
    vty->fail = 0;
    vty->cp = 0;
    vty_clear_buf (vty);
    vty->length = 0;
    memset (vty->hist, 0, sizeof (vty->hist));
    vty->hp = 0;
    vty->hindex = 0;
    vector_set_index (vtyvec, vty_sock, vty);
    vty->status = VTY_NORMAL;
    vty->v_timeout = VTY_TIMEOUT_DEFAULT;
    vty->lines = -1;
    vty->iac = 0;
    vty->iac_sb_in_progress = 0;
    vty->sb_len = 0;
    vty->iInvLogin = 0;
    /* Say hello to the world. */
    vty_hello (vty);
    if (! no_password_check)
    vty_out (vty, "%sUser Access Verification%s%s", VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE);

    //初始化ctrl+c功能的数据
    vty->iBreak = 1;
    vty->stChildPId = 0;
    pthread_cond_init(&vty->cond, NULL);
    init_loop_buf(&vty->ring);

    // 开启为终端设备
    vty->ptm = open("/dev/ptmx", O_RDWR | O_NONBLOCK);
    if (vty->ptm > 0)
    {
        const char *name;
		grantpt(vty->ptm);
		unlockpt(vty->ptm);
		name = ptsname(vty->ptm);
		if (NULL == name)
		{
			printf("ptsname error (is /dev/pts mounted?)");
		}
		else
		{
	        vty->pts = open(name, O_RDWR, 0666);
        }
    }

    /* Setting up terminal. */
    vty_will_echo (vty);
    vty_will_suppress_go_ahead (vty);

    vty_dont_linemode (vty);
    vty_do_window_size (vty);
    /* vty_dont_lflow_ahead (vty); */

    vty_prompt (vty);

    registe_vty(vty);

    return vty;
}

static void vty_exe (void *arg)
{
    struct vty *vty = (struct vty *)arg;

    THREAD_SETNAME_PRI("vty_exe", 40);

    while(1)
    {
        vty_flush(vty);
        vty_read(vty);
        if (vty->status == VTY_CLOSE)
        {
            break;
        }        
    }

    return;
}

/* Accept connection from the network. */
static void vty_accept (void *arg)
{
    int vty_sock = ((struct vty_accept_t *)arg)->vty_sock;
    struct vty *vty;
    union sockunion su;
    int ret;
    unsigned int on = 1;
    pthread_t tid;
    int option = 1;
    
    THREAD_SETNAME_PRI("vty_recv", 40);

    memcpy(&su, &(((struct vty_accept_t *)arg)->su), sizeof (union sockunion));

    free(arg);

    //set_nonblocking(vty_sock);

    ret = setsockopt(vty_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));
    ret |= setsockopt(vty_sock, SOL_SOCKET, SO_KEEPALIVE, &option, sizeof(option));
    if (ret < 0)
        printf("can't set sockopt to vty_sock : %s", safe_strerror (errno));

    vty = vty_create (vty_sock, &su);

    /* 达到最大telnet限制是的处理 */
    if (vty->status == VTY_CLOSE)
    {
        vty_close(vty);
        return;
    }

    pthread_create(&tid, NULL, (void *)vty_exe, (void *)vty);

    while(1)
    {
        vty_read_ctrl(vty);
        if (vty->status == VTY_CLOSE)
        {
            pthread_cond_signal(&vty->cond);
            break;
        }
    }
    
    pthread_join(tid, NULL);
    vty_close(vty);

    return;
}

#if defined(HAVE_IPV6) && !defined(NRL)

void *vty_accept_addrinfo(void *arg)
{
    int sock = *(int *)arg;
    int vty_sock;

    free(arg);

    while (1)
    {
        g_pstVtyAcceptArg = malloc(sizeof(struct vty_accept_t));
        memset (&(g_pstVtyAcceptArg->su), 0, sizeof (union sockunion));
        vty_sock = sockunion_accept (sock, &(g_pstVtyAcceptArg->su));
        if (vty_sock < 0)
        {
            printf("can't accept vty socket : %s\n", safe_strerror (errno));
            return -1;
        }
        g_pstVtyAcceptArg->vty_sock = vty_sock;

        pthread_create(&tid, NULL, (void *)vty_accept, (void *)g_pstVtyAcceptArg);
    }

}

static void
vty_serv_sock_addrinfo (const char *hostname, unsigned short port)
{
    int ret;
    struct addrinfo req;
    struct addrinfo *ainfo;
    struct addrinfo *ainfo_save;
    int sock;
    char port_str[BUFSIZ];
    pthread_t tid;

    memset (&req, 0, sizeof (struct addrinfo));
    req.ai_flags = AI_PASSIVE;
    req.ai_family = AF_UNSPEC;
    req.ai_socktype = SOCK_STREAM;
    sprintf (port_str, "%d", port);
    port_str[sizeof (port_str) - 1] = '\0';

    ret = getaddrinfo (hostname, port_str, &req, &ainfo);

    if (ret != 0)
    {
        fprintf (stderr, "getaddrinfo failed: %s\n", gai_strerror (ret));
        exit (1);
    }

    ainfo_save = ainfo;

    do
    {
        if (ainfo->ai_family != AF_INET
#ifdef HAVE_IPV6
            && ainfo->ai_family != AF_INET6
#endif /* HAVE_IPV6 */
            )
            continue;

        sock = socket (ainfo->ai_family, ainfo->ai_socktype, ainfo->ai_protocol);
        if (sock < 0)
            continue;

        sockopt_reuseaddr (sock);
        sockopt_reuseport (sock);

        ret = bind (sock, ainfo->ai_addr, ainfo->ai_addrlen);
        if (ret < 0)
        {
            close (sock);	/* Avoid sd leak. */
            continue;
        }

        ret = listen (sock, 3);
        if (ret < 0)
        {
            close (sock);	/* Avoid sd leak. */
            continue;
        }

        int *fd = malloc(sizeof(int *));
        memcpy(fd, &sock, sizeof(int *));
        pthread_create(&tid, NULL, vty_accept_addrinfo, (void *)fd);

    }while ((ainfo = ainfo->ai_next) != NULL);

    freeaddrinfo (ainfo_save);
}

#endif /* HAVE_IPV6 && ! NRL */

struct vty_accept_t *g_pstVtyAcceptArg;
int g_iAcceptSock;

void clean_up_accept_sock(void *arg)
{
    close(g_iAcceptSock);
    if (NULL != g_pstVtyAcceptArg)
    {
        free(g_pstVtyAcceptArg);
        g_pstVtyAcceptArg = NULL;
    }
}

/* Make vty server socket. */
static void vty_serv_sock_family (const char* addr, unsigned short port, int family)
{
    int ret;
    union sockunion su;
    int vty_sock;
    void* naddr = NULL;
    pthread_t tid;
    pthread_attr_t pthread_attr;
    int err;
    char *pcIp;

    memset (&su, 0, sizeof (union sockunion));
    su.sa.sa_family = family;
    if(addr)
    {
        switch(family)
        {
            case AF_INET:
                naddr=&su.sin.sin_addr;
                break;
#ifdef HAVE_IPV6
            case AF_INET6:
                naddr=&su.sin6.sin6_addr;
                break;
#endif
        }
    }

    if(naddr)
    {
        switch(inet_pton(family,addr,naddr))
        {
            case -1:
                printf("bad address %s",addr);
                naddr=NULL;
                break;
            case 0:
                printf("error translating address %s: %s",addr,safe_strerror(errno));
                naddr=NULL;
        }
    }
    /* Make new socket. */
    g_iAcceptSock = sockunion_stream_socket (&su);
    if (g_iAcceptSock < 0)
    {
        return;
    }

    /* This is server, so reuse address. */
    sockopt_reuseaddr (g_iAcceptSock);
    sockopt_reuseport (g_iAcceptSock);

    /* Bind socket to universal address and given port. */
    ret = sockunion_bind (g_iAcceptSock, &su, port, naddr);
    if (ret < 0)
    {
        printf("can't bind socket\n");
        close (g_iAcceptSock);	/* Avoid sd leak. */
        return;
    }

    /* Listen socket under queue 3. */
    ret = listen (g_iAcceptSock, 3);
    if (ret < 0)
    {
        printf("can't listen socket\n");
        close (g_iAcceptSock);	/* Avoid sd leak. */
        return;
    }

    pthread_cleanup_push(clean_up_accept_sock, NULL);
    while (1)
    {
        g_pstVtyAcceptArg = malloc(sizeof(struct vty_accept_t));
        memset (&(g_pstVtyAcceptArg->su), 0, sizeof (union sockunion));
        vty_sock = sockunion_accept (g_iAcceptSock, &(g_pstVtyAcceptArg->su));
        if (vty_sock < 0)
        {
            free(g_pstVtyAcceptArg);
            g_pstVtyAcceptArg = NULL;
            printf("can't accept vty socket : %s\n", safe_strerror (errno));
            continue;
        }
        g_pstVtyAcceptArg->vty_sock = vty_sock;

        pcIp = sockunion_su2str (&g_pstVtyAcceptArg->su);

        if (0 != FindDos(pcIp) )
        {
            close(vty_sock);
            free(g_pstVtyAcceptArg);
            g_pstVtyAcceptArg = NULL;
            free(pcIp);
            continue;
        }

        err = pthread_attr_init(&pthread_attr);
        err += pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED);
        if (0 != err)
        {
            close(vty_sock);
            free(g_pstVtyAcceptArg);
            g_pstVtyAcceptArg = NULL;
            pthread_attr_destroy(&pthread_attr);
            printf("pthread_atrr_init is error : %s\n", safe_strerror (errno));
            continue;
        }

        pthread_create(&tid, &pthread_attr, (void *)vty_accept, (void *)g_pstVtyAcceptArg);
        g_pstVtyAcceptArg = NULL;

        pthread_attr_destroy(&pthread_attr);
    }
    pthread_cleanup_pop(0);

    return;
}

/* Determine address family to bind. */
void vty_serv_sock (void *arg)
{
    unsigned short port = g_stTelnetInfo.iPort;

    /* If port is set to 0, do not listen on TCP/IP at all! */
    if (port)
    {

#ifdef HAVE_IPV6
#ifdef NRL
        vty_serv_sock_family (NULL, port, AF_INET);
        vty_serv_sock_family (NULL, port, AF_INET6);
#else /* ! NRL */
        vty_serv_sock_addrinfo (NULL, port);
#endif /* NRL*/
#else /* ! HAVE_IPV6 */
        vty_serv_sock_family (NULL, port, AF_INET);
#endif /* HAVE_IPV6 */
    }

    return;
}

/* Close vty interface.  Warning: call this only from functions that
   will be careful not to access the vty afterwards (since it has
   now been freed).  This is safest from top-level functions (called
   directly by the thread dispatcher). */
void
vty_close (struct vty *vty)
{
  int i;

  release_vty(vty);

  /* Flush buffer. */
  buffer_flush_all (vty->obuf, vty->fd);

  /* Free input buffer. */
  buffer_free (vty->obuf);

  /* Free command history. */
  for (i = 0; i < VTY_MAXHIST; i++)
    if (vty->hist[i])
      XFREE (MTYPE_VTY_HIST, vty->hist[i]);

  /* Unset vector. */
  vector_unset (vtyvec, vty->fd);

  /* Close socket. */
  if (vty->fd > 0)
  {
    close (vty->fd);
    vty->fd = -1;
  }

  if (vty->address)
    XFREE (MTYPE_TMP, vty->address);
  if (vty->cmd_input_buf)
    XFREE (MTYPE_VTY, vty->cmd_input_buf);
  if (vty->pcBuf)
    free(vty->pcBuf);
  if (vty->iVtyIndex < MAX_TELNET_NUM)
  {
        /* 左移i位，删除第i个telnet连接的用户标志位 */
        release_user(vty->user_index, 1<<(vty->iVtyIndex+1));
  }
  /* Check configure. */
  //vty_config_unlock (vty);

  //关闭为终端
  close(vty->pts);
  close(vty->ptm);
  
  fini_loop_buf(&vty->ring);

  /* OK free vty. */
  XFREE (MTYPE_VTY, vty);

  return;
}

int
vty_shell (struct vty *vty)
{
  return vty->type == VTY_SHELL ? 1 : 0;
}

/* Install vty's own commands like `who' command. */
void vty_init ()
{
  vtyvec = vector_init (VECTOR_MIN_SIZE);
}

