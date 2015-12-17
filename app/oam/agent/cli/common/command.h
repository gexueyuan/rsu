/*
 * Zebra configuration command interface routine
 * Copyright (C) 1997, 98 Kunihiro Ishiguro
 *
 * This file is part of GNU Zebra.
 *
 * GNU Zebra is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2, or (at your
 * option) any later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _ZEBRA_COMMAND_H
#define _ZEBRA_COMMAND_H

#include "vector.h"
#include "vty.h"

//extern pthread_mutex_t g_stUserInfoMutex;


/* Host configuration variable */
struct host
{
  /* Host name of this router. */
  char *name;
  /* Banner configuration. */
  const char *motd;
};

/* There are some command levels which called from command node. */
enum node_type
{
    USER_NODE,
    AUTH_NODE,			/* Authentication mode of vty interface. */
    VIEW_NODE,			/* View node. Default mode of vty interface. */
    AUTH_ENABLE_NODE,		/* Authentication mode for change enable. */
    ENABLE_NODE,			/* Enable node. */
    CONFIG_NODE,			/* Config node. Default mode of config file. */
    CHANGE_PASSWD_NODE,    /* 修改密码的节点 */
    CHANGE_PASSWD_AGAIN_NODE, /* 重新修改密码的节点 */
    DEBUG_NODE,			/* Debug node. */
};

typedef struct cli_running_show_s
{
    int (*func)(int port_id, char *p_outbuf, int outbuf_len);
    struct cli_running_show_t *p_next;
}cli_running_show_t;

/* Node which has some commands and prompt string and configuration
   function pointer . */
struct cmd_node
{
  /* Node index. */
  enum node_type node;

  /* Prompt character at vty interface. */
  const char *prompt;

  /* Is this node's configuration goes to vtysh ? */
  int vtysh;

  /* Node's configuration write function */
  int (*func) (struct vty *);

  /* Vector of this node's command list. */
  vector cmd_vector;
  /*show running config func*/
  cli_running_show_t *p_running_show;
};

enum
{
  CMD_ATTR_DEPRECATED = 1,
  CMD_ATTR_HIDDEN,
};

enum
{
    CMD_HELP_LANG_ENGLISH = 0,
    CMD_HELP_LANG_CHINESE,
    CMD_HELP_LANG_NUM
};

enum
{
    CMD_LEVEL_VISITOR_0 = 0,
    CMD_LEVEL_VISITOR_1 = 1,
    CMD_LEVEL_VISITOR_2 = 2,
    CMD_LEVEL_VISITOR_3 = 3,
    CMD_LEVEL_VISITOR_4 = 4,
    CMD_LEVEL_MONITOR_5 = 5,
    CMD_LEVEL_MONITOR_6 = 6,
    CMD_LEVEL_MONITOR_7 = 7,
    CMD_LEVEL_MONITOR_8 = 8,
    CMD_LEVEL_MONITOR_9 = 9,
    CMD_LEVEL_MONITOR_10 = 10,
    CMD_LEVEL_OPERATOR_11 = 11,
    CMD_LEVEL_OPERATOR_12 = 12,
    CMD_LEVEL_OPERATOR_13 = 13,
    CMD_LEVEL_OPERATOR_14 = 14,
    CMD_LEVEL_ADMINISTRATOR_15 = 15
};

/* Structure of command element. */
struct cmd_element
{
  const char *string;			/* Command specification by string. */
  int (*func) (struct cmd_element *, struct vty *, int, const char *[]);
  const char *doc;	/* Documentation of this command. */
  int daemon;                   /* Daemon to which this command belong. */
  vector strvec;		/* Pointing out each description vector. */
  unsigned int cmdsize;		/* Command index count. */
  char *config;			/* Configuration string */
  vector subconfig;		/* Sub configuration string */
  char attr;			/* Command attributes */
  char level;
};

/* Command description structure. */
struct desc
{
  char *cmd;                    /* Command string. */
  char *str[CMD_HELP_LANG_NUM]; /* Command's description. */
};

/* Return value of the commands. */
#define CMD_SUCCESS              0
#define CMD_WARNING              1
#define CMD_ERR_NO_MATCH         2
#define CMD_ERR_AMBIGUOUS        3
#define CMD_ERR_INCOMPLETE       4
#define CMD_ERR_EXEED_ARGC_MAX   5
#define CMD_ERR_NOTHING_TODO     6
#define CMD_COMPLETE_FULL_MATCH  7
#define CMD_COMPLETE_MATCH       8
#define CMD_COMPLETE_LIST_MATCH  9
#define CMD_ERR_PRIORITY_LOWER  10
#define CMD_ERR_UNKNOWN         11

/* Argc max counts. */
#define CMD_ARGC_MAX   25

#define HOST_NAME_LEN 32

/* Turn off these macros when uisng cpp with extract.pl */
#ifndef VTYSH_EXTRACT_PL

/* helper defines for end-user DEFUN* macros */
#define DEFUN_CMD_ELEMENT(funcname, cmdname, cmdstr, helpstr, attrs, levels) \
  struct cmd_element cmdname = \
  { \
    .string = cmdstr, \
    .func = funcname, \
    .doc = helpstr, \
    .attr = attrs, \
    .level = levels \
  };

#define DEFUN_CMD_FUNC_DECL(funcname) \
  static int funcname (struct cmd_element *, struct vty *, int, const char *[]);

#define DEFUN_CMD_FUNC_TEXT(funcname) \
  static int funcname \
    (struct cmd_element *self __attribute__ ((unused)), \
     struct vty *vty __attribute__ ((unused)), \
     int argc __attribute__ ((unused)), \
     const char *argv[] __attribute__ ((unused)) )

/* DEFUN for vty command interafce. Little bit hacky ;-). */
#define DEFUN(funcname, cmdname, cmdstr, helpstr, level) \
  DEFUN_CMD_FUNC_DECL(funcname) \
  DEFUN_CMD_ELEMENT(funcname, cmdname, cmdstr, helpstr, 0, level) \
  DEFUN_CMD_FUNC_TEXT(funcname)

#define DEFUN_ATTR(funcname, cmdname, cmdstr, helpstr, attr, level) \
  DEFUN_CMD_FUNC_DECL(funcname) \
  DEFUN_CMD_ELEMENT(funcname, cmdname, cmdstr, helpstr, attr, level) \
  DEFUN_CMD_FUNC_TEXT(funcname)

#define DEFUN_HIDDEN(funcname, cmdname, cmdstr, helpstr, level) \
  DEFUN_ATTR (funcname, cmdname, cmdstr, helpstr, CMD_ATTR_HIDDEN, level)

#define DEFUN_DEPRECATED(funcname, cmdname, cmdstr, helpstr, level) \
  DEFUN_ATTR (funcname, cmdname, cmdstr, helpstr, CMD_ATTR_DEPRECATED, level) \

/* ALIAS macro which define existing command's alias. */
#define ALIAS(funcname, cmdname, cmdstr, helpstr, level) \
  DEFUN_CMD_ELEMENT(funcname, cmdname, cmdstr, helpstr, 0, level)

#define ALIAS_ATTR(funcname, cmdname, cmdstr, helpstr, attr, level) \
  DEFUN_CMD_ELEMENT(funcname, cmdname, cmdstr, helpstr, attr, level)

#define ALIAS_HIDDEN(funcname, cmdname, cmdstr, helpstr, level) \
  DEFUN_CMD_ELEMENT(funcname, cmdname, cmdstr, helpstr, CMD_ATTR_HIDDEN, level)

#define ALIAS_DEPRECATED(funcname, cmdname, cmdstr, helpstr, level) \
  DEFUN_CMD_ELEMENT(funcname, cmdname, cmdstr, helpstr, CMD_ATTR_DEPRECATED, level)

#endif /* VTYSH_EXTRACT_PL */

/* Some macroes */
#define CMD_OPTION(S)   ((S[0]) == '[')
#define CMD_VARIABLE(S) (((S[0]) >= 'A' && (S[0]) <= 'Z') || ((S[0]) == '<'))
#define CMD_VARARG(S)   ((S[0]) == '.')
#define CMD_RANGE(S)	((S[0] == '<'))
#define CMD_MULTI(S)	((S[0] == '{'))

#define CMD_IPV4(S)	   ((strcmp ((S), "A.B.C.D") == 0))
#define CMD_IPV4_PREFIX(S) ((strcmp ((S), "A.B.C.D/M") == 0))
#define CMD_IPV6(S)        ((strcmp ((S), "X:X::X:X") == 0))
#define CMD_IPV6_PREFIX(S) ((strcmp ((S), "X:X::X:X/M") == 0))

/* Common descriptions. */
#define SET_TIPS_EN "Set configuration\n"
#define SET_TIPS_CH "设置系统运行参数\n"

#define SHOW_TIPS_EN "Show running system information\n"
#define SHOW_TIPS_CH "显示系统运行信息\n"
#define NO_TIPS_EN "Negate a command or set its defaults\n"
#define NO_TIPS_CH "逆向命令或者设置缺省值\n"
#define INTERFACE_TIPS_EN "Interface configuration\n"
#define INTERFACE_TIPS_CH "接口配置\n"
#define ETHERNET_TIPS_EN "ONU ethernet interface\n"
#define ETHERNET_TIPS_CH "以太网接口\n"
#define DISABLE_TIPS_EN "Disable\n"
#define DISABLE_TIPS_CH "禁止\n"
#define ENABLE_TIPS_EN "Enable\n"
#define ENABLE_TIPS_CH "使能\n"
#define ETH_LIST_TIPS_EN "Ethernet list\n"
#define ETH_LIST_TIPS_CH "端口列表\n"
#define ETH_ID_TIPS_EN "Ethernet ID\n"
#define ETH_ID_TIPS_CH "端口号\n"
#define IP_ADDR_TIPS_EN "IP address\n"
#define IP_ADDR_TIPS_CH "IP地址\n"
#define MAC_ADDR_TIPS_EN "MAC address(All 0 indicates no check MAC)\n"
#define MAC_ADDR_TIPS_CH "MAC地址(全0表示不检查MAC)\n"

#define STATISTICV_TIPS_EN "Statistic\n"
#define STATISTICV_TIPS_CH "统计\n"
#define CLEAR_TIPS_EN "Clear\n"
#define CLEAR_TIPS_CH "清除\n"
#define VOICE_TIPS_EN "Voice\n"
#define VOICE_TIPS_CH "语音\n"
#define H248_TIPS_EN "H.248 protocol\n"
#define H248_TIPS_CH "H.248协议\n"
#define RTP_TIPS_EN "Real-Time transport protocol\n"
#define RTP_TIPS_CH "实时传输协议\n"
#define CALL_TIPS_EN "Call\n"
#define CALL_TIPS_CH "呼叫\n"
#define POTS_TIPS_EN "POTS port\n"
#define POTS_TIPS_CH "POTS口\n"
#define POTS_LIST_TIPS_EN "POTS port ID\n"
#define POTS_LIST_TIPS_CH "POTS口ID\n"

#define CNU_SET_GET_CONFIG_EN "set or get configuration of cnu\n"
#define CNU_SET_GET_CONFIG_CH "设置或获取终端配置\n"
#define INPUT_PASSWD_TOO_LENGTH(pstVty) VTY_OUT_EC(pstVty, "The password length can not exceed %d or empty!%s", "密码长度不能超过%d或者为空！%s",MAX_PASSWD_LEN, VTY_NEWLINE)
#define TWO_INPUT_PASSWD_DIFF(pstVty) VTY_OUT_EC(pstVty, "The passwords you input don't match!%s","两次输入密码不匹配！%s", VTY_NEWLINE)

/* Prototypes. */
extern void install_node (struct cmd_node *, int (*) (struct vty *));
extern void install_default (enum node_type);
extern void install_element (enum node_type, struct cmd_element *);
extern void sort_node (void);

/* Concatenates argv[shift] through argv[argc-1] into a single NUL-terminated
   string with a space between each element (allocated using
   XMALLOC(MTYPE_TMP)).  Returns NULL if shift >= argc. */
extern char *argv_concat (const char **argv, int argc, int shift);

extern vector cmd_make_strvec (const char *);
extern void cmd_free_strvec (vector);
extern vector cmd_describe_command (vector, struct vty *, int *status);
extern char **cmd_complete_command (vector, struct vty *, int *status);
extern const char *cmd_prompt (enum node_type);
extern enum node_type node_parent (enum node_type);
extern int cmd_execute_command (vector, struct vty *, int);
extern void cmd_init ();
extern void cmd_terminate (void);
extern int use_auth(char *pcUserName, char *pcPasswd, int *piPriority);
extern int enable_auth(char *pcPasswd);
extern int registe_user(char *pcUserName, int iTerminalType);
extern void release_user(int iUserIndex, int iTerminal);


/* Export typical functions. */
extern struct cmd_element config_end_cmd;
extern struct cmd_element config_exit_cmd;
extern struct cmd_element config_quit_cmd;
extern struct cmd_element config_help_cmd;
extern struct cmd_element config_list_cmd;
extern char *host_config_file (void);
extern void host_config_set (char *);

void install_show_running (enum node_type ntype,int (*func)(int port_id, char *p_outbuf, int outbuf_len));
/* "<cr>" global */
extern char *command_cr;
#endif /* _ZEBRA_COMMAND_H */
