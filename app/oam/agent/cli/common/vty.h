/* Virtual terminal [aka TeletYpe] interface routine
   Copyright (C) 1997 Kunihiro Ishiguro

This file is part of GNU Zebra.

GNU Zebra is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

GNU Zebra is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Zebra; see the file COPYING.  If not, write to the Free
Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

#ifndef _ZEBRA_VTY_H
#define _ZEBRA_VTY_H

#include <pthread.h>
#include "loop_buf.h"
#include "cv_osal.h"

#define VTY_BUFSIZ 512
#define VTY_MAXHIST 20

#define ONU_PORT_NUM_MAX 32
#define CHANGE_USER_PASSWOED  1
#define CHANGE_ENABLE_PASSWOED  2


typedef struct TelnetInfoTag
{
    pthread_t stTelnetId;
    int iEnable; /* 取值 ENABLEVAR_ENABLE ENABLEVAR_DISABLE */
    int iPort; /* 端口 */
}StrTelnetInfo;

/* VTY struct. */
struct vty 
{
  /* File descripter of this vty. */
  int fd;

  /* Is this vty connect to file or not */
  enum {VTY_TERM, VTY_SHELL} type;

  /* Node status of this vty */
  unsigned int node;

    /* 在用户数信息组中的下标 */
  int user_index;
  /* 下面变量是个临时的变量，登陆后设置为null,用时，申请内存，用完释放并置成NULL */
  char *pcBuf;
  /* 记录是第几个telnet */
  int iVtyIndex;
  /* 记录修改密码的类型 */
  int iChangePasswdType;
  int iPriority;

  //begin: 为了命令行支持ctrl+c
  int iBreak;/* 当iBreak=1时，中断命令的循环操作 */
  pid_t stChildPId;/* 当按下ctrl+c时，需要中断的子进程 */
  pthread_cond_t cond; //执行线程的条件变量
  loop_buf_t ring;

  //下面这两个为终端，是为了telnet支持syslog
  int ptm; //伪终端主设备
  int pts; //伪终端从设备


  /* What address is this vty comming from. */
  char *address;

  /* Failure count */
  int fail;

  /* Output buffer. */
  struct buffer *obuf;

  /* Command input buffer */
  char *cmd_input_buf;;

  /* Command cursor point */
  int cp;

  /* Command length */
  int length;

  /* Command max length. */
  int max;

  /* Histry of command */
  char *hist[VTY_MAXHIST];

  /* History lookup current point */
  int hp;

  /* History insert end point */
  int hindex;

  /* For current referencing point of interface, route-map,
     access-list etc... */
  void *index;

  /* For multiple level index treatment such as key chain and key. */
  void *index_sub;

  /* For escape character. */
  unsigned char escape;

  /* Current vty status. */
  enum {VTY_NORMAL, VTY_CLOSE, VTY_MORE, VTY_MORELINE} status;

  /* IAC handling: was the last character received the
     IAC (interpret-as-command) escape character (and therefore the next
     character will be the command code)?  Refer to Telnet RFC 854. */
  unsigned char iac;

  /* IAC SB (option subnegotiation) handling */
  unsigned char iac_sb_in_progress;
  /* At the moment, we care only about the NAWS (window size) negotiation,
     and that requires just a 5-character buffer (RFC 1073):
       <NAWS char> <16-bit width> <16-bit height> */
#define TELNET_NAWS_SB_LEN 5
  unsigned char sb_buf[TELNET_NAWS_SB_LEN];
  /* How many subnegotiation characters have we received?  We just drop
     those that do not fit in the buffer. */
  size_t sb_len;

  /* Window width/height. */
  int width;
  int height;

  /* Configure lines. */
  int lines;

  /* Terminal monitor. */
  int monitor;

  /* In configure mode. */
  int config;

  /* Timeout seconds and thread. */
  unsigned long v_timeout;

  int language;

  int iInvLogin; //无效的登陆次数，最多允许用户尝试3次，超过4次就要等待60秒，重新登陆

  int port_list[ONU_PORT_NUM_MAX];

};

/* Integrated configuration file. */
#define INTEGRATE_DEFAULT_CONFIG "Quagga.conf"

/* Small macro to determine newline is newline only or linefeed needed. */
#define VTY_NEWLINE  ((vty->type == VTY_TERM) ? "\r\n" : "\n")
#define VTY_LEFT_INDENT "    "
#define VTY_LINE_HEAD "    "

/* Default time out value */
#define VTY_TIMEOUT_DEFAULT 600

/* Vty read buffer size. */
#define VTY_READ_BUFSIZ 512

/* Directory separator. */
#ifndef DIRECTORY_SEP
#define DIRECTORY_SEP '/'
#endif /* DIRECTORY_SEP */

#ifndef IS_DIRECTORY_SEP
#define IS_DIRECTORY_SEP(c) ((c) == DIRECTORY_SEP)
#endif

/* GCC have printf type attribute check.  */
#ifdef __GNUC__
#define PRINTF_ATTRIBUTE(a,b) __attribute__ ((__format__ (__printf__, a, b)))
#else
#define PRINTF_ATTRIBUTE(a,b)
#endif /* __GNUC__ */

/* Utility macros to convert VTY argument to unsigned long or integer. */
#define VTY_GET_LONG(NAME,V,STR) \
do { \
  char *endptr = NULL; \
  (V) = strtoul ((STR), &endptr, 10); \
  if (*endptr != '\0' || (V) == ULONG_MAX) \
    { \
      vty_out (vty, "%% Invalid %s value%s", NAME, VTY_NEWLINE); \
      return CMD_WARNING; \
    } \
} while (0)

#define VTY_GET_INTEGER_RANGE(NAME,V,STR,MIN,MAX) \
do { \
  unsigned long tmpl; \
  VTY_GET_LONG(NAME, tmpl, STR); \
  if ( (tmpl < (MIN)) || (tmpl > (MAX))) \
    { \
      vty_out (vty, "%% Invalid %s value%s", NAME, VTY_NEWLINE); \
      return CMD_WARNING; \
    } \
  (V) = tmpl; \
} while (0)

#define VTY_GET_INTEGER(NAME,V,STR) \
  VTY_GET_INTEGER_RANGE(NAME,V,STR,0U,UINT32_MAX)

#define VTY_GET_IPV4_ADDRESS(NAME,V,STR)                                      \
do {                                                                             \
  int retv;                                                                   \
  retv = inet_aton ((STR), &(V));                                             \
  if (!retv)                                                                  \
    {                                                                         \
      vty_out (vty, "%% Invalid %s value%s", NAME, VTY_NEWLINE);              \
      return CMD_WARNING;                                                     \
    }                                                                         \
} while (0)

#define VTY_GET_IPV4_PREFIX(NAME,V,STR)                                       \
do {                                                                             \
  int retv;                                                                   \
  retv = str2prefix_ipv4 ((STR), &(V));                                       \
  if (retv <= 0)                                                              \
    {                                                                         \
      vty_out (vty, "%% Invalid %s value%s", NAME, VTY_NEWLINE);              \
      return CMD_WARNING;                                                     \
    }                                                                         \
} while (0)

#define VTY_OUT_EC(vty, format_en, format_ch, arg...) \
    do{/*lint -save -e10 -e534*/\
        if (CMD_HELP_LANG_ENGLISH == vty->language) \
            vty_out(vty, format_en, ##arg); \
        else if (CMD_HELP_LANG_CHINESE == vty->language) \
            vty_out(vty, format_ch, ##arg); \
    } while (0)

#define THREAD_SETNAME_PRI(name, pri); \
do{\
    struct sched_param	sched;\
    sched.sched_priority = pri;\
    sched_setscheduler(0,SCHED_RR,&sched);\
    prctl(PR_SET_NAME, name);\
}while(0)

#define VTY_GET_UNSUCCESSFULLY(vty) \
    VTY_OUT_EC(vty, "%sGet unsuccessfully%s", "%s获取失败%s", VTY_NEWLINE,VTY_NEWLINE)

#define VTY_SET_UNSUCCESSFULLY(vty) \
    VTY_OUT_EC(vty, "%sSet unsuccessfully%s", "%s设置失败%s", VTY_NEWLINE,VTY_NEWLINE)

#define VTY_UNKNOWN_FAIL(vty) \
    VTY_OUT_EC(vty, "Unknown reason.%s", "未知错误%s", VTY_NEWLINE)

#define VTY_DOWNLOAD_FAIL(vty) \
    VTY_OUT_EC(vty, "Download failed%s", "下载失败%s", VTY_NEWLINE)
#define VTY_DOWNLOAD_SUCCESS(vty) \
    VTY_OUT_EC(vty, "Download succeed%s", "下载成功%s", VTY_NEWLINE)
#define VTY_UPLOAD_FAIL(vty) \
    VTY_OUT_EC(vty, "Upload failed%s", "上传失败%s", VTY_NEWLINE)
#define VTY_UPLOAD_SUCCESS(vty) \
    VTY_OUT_EC(vty, "Upload succeed%s", "上传成功%s", VTY_NEWLINE)
/* Prototypes. */
extern void vty_init ();
extern void vty_terminate (void);
extern void vty_reset (void);
extern struct vty *vty_new (void);
extern int vty_out (struct vty *, const char *, ...) PRINTF_ATTRIBUTE(2, 3);
extern void vty_serv_sock (void *arg);
extern void vty_close (struct vty *);
//extern int vty_config_lock (struct vty *);
//extern int vty_config_unlock (struct vty *);
extern int vty_shell (struct vty *);
extern void vty_hello (struct vty *);
extern int vty_flush (struct vty *vty);
extern int vty_in_line(struct vty *vty, char* pcLine, int iMaxSize);
extern int execute_command(struct vty *vty, char *buf);
extern int SetTelnetPort(int iPort);
extern int SetTelnetEnable();
extern int SetTelnetDisable();
extern void TelnetInit();

/* 获取telnet端口 */
extern int GetTelnetPort();
extern bool_t IsTelnetEnable();

#endif /* _ZEBRA_VTY_H */
