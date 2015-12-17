/* Virtual terminal interface shell.
 * Copyright (C) 2000 Kunihiro Ishiguro
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

#include <sys/un.h>
#include <setjmp.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>

#include "readline.h"
#include "history.h"
#include <termios.h>
#include <unistd.h>

#include "command.h"
#include "memory.h"
#include "vtysh.h"
#include "cli_user_lib.h"

/* Struct VTY. */
struct vty *vty;

/* A static variable for holding the line. */
static char *line_read;
char history_file[MAXPATHLEN];

/* 1:echo 0:no echo */
void rl_set_echo (int flag)
{   
    static int set = 0;
    static struct termios stTermiosOld;
    struct termios stTermios;

    if (set == 0) {
        tcgetattr(STDIN_FILENO, &stTermiosOld);
        set = 1;
    }
    if (0 == flag) {
        /* 设置不回显 */ 
        tcgetattr(STDIN_FILENO, &stTermios);
        stTermios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
        tcsetattr(STDIN_FILENO, TCSANOW, &stTermios);        
    }
    else{
        tcsetattr(STDIN_FILENO, TCSANOW, &stTermiosOld);        
    }
    
}

int vtysh_execute (const char *line)
{
  return execute_command(vty, (char*)line);
}

/* We don't care about the point of the cursor when '?' is typed. */
int vtysh_rl_describe (int count, int c)
{
  int ret;
  unsigned int i;
  vector vline;
  vector describe;
  int width;
  struct desc *desc;

  vline = cmd_make_strvec (rl_line_buffer);

  /* In case of '> ?'. */
  if (vline == NULL)
    {
      vline = vector_init (1);
      vector_set (vline, '\0');
    }
  else 
    if (rl_end && isspace ((int) rl_line_buffer[rl_end - 1]))
      vector_set (vline, '\0');

  describe = cmd_describe_command (vline, vty, &ret);

  fprintf (stdout,"\n");

  /* Ambiguous and no match error. */
  switch (ret)
    {
    case CMD_ERR_AMBIGUOUS:
      cmd_free_strvec (vline);
      VTY_OUT_EC(vty, "%% Ambiguous command.%s", "%% 有歧义的命令。%s", VTY_NEWLINE);
      rl_on_new_line ();
      return 0;
      break;
    case CMD_ERR_NO_MATCH:
      cmd_free_strvec (vline);
      VTY_OUT_EC(vty, "%% Unknown command.%s", "%% 未知的命令。%s", VTY_NEWLINE);
      rl_on_new_line ();
      return 0;
      break;
    }  

  /* Get width of command string. */
  width = 0;
  for (i = 0; i < vector_active (describe); i++)
    if ((desc = vector_slot (describe, i)) != NULL)
      {
	int len;

	if (desc->cmd[0] == '\0')
	  continue;

	len = strlen (desc->cmd);
	if (desc->cmd[0] == '.')
	  len--;

	if (width < len)
	  width = len;
      }

  for (i = 0; i < vector_active (describe); i++)
    if ((desc = vector_slot (describe, i)) != NULL)
      {
	if (desc->cmd[0] == '\0')
	  continue;

	if (! desc->str[vty->language])
	  fprintf (stdout,"  %-s\n",
		   desc->cmd[0] == '.' ? desc->cmd + 1 : desc->cmd);
	else
	  fprintf (stdout,"  %-*s  %s\n",
		   width,
		   desc->cmd[0] == '.' ? desc->cmd + 1 : desc->cmd,
		   desc->str[vty->language]);
      }

  cmd_free_strvec (vline);
  vector_free (describe);

  rl_on_new_line();

  return 0;
}

/* Result of cmd_complete_command() call will be stored here
 * and used in new_completion() in order to put the space in
 * correct places only. */
int complete_status;

static char *
command_generator (const char *text, int state)
{
  vector vline;
  static char **matched = NULL;
  static int index = 0;

  /* First call. */
  if (! state)
    {
      index = 0;

      if (vty->node == USER_NODE || vty->node == AUTH_NODE || vty->node == AUTH_ENABLE_NODE ||
         (vty->node == CHANGE_PASSWD_NODE) || (vty->node == CHANGE_PASSWD_AGAIN_NODE))
	    return NULL;

      vline = cmd_make_strvec (rl_line_buffer);
      if (vline == NULL)
	return NULL;

      if (rl_end && isspace ((int) rl_line_buffer[rl_end - 1]))
	vector_set (vline, '\0');

      matched = cmd_complete_command (vline, vty, &complete_status);
    }

  if (matched && matched[index])
    return matched[index++];

  return NULL;
}

static char **
new_completion (char *text, int start, int end)
{
  char **matches;

  matches = rl_completion_matches (text, command_generator);

  if (matches)
    {
      rl_point = rl_end;
      if (complete_status == CMD_COMPLETE_FULL_MATCH)
	rl_pending_input = ' ';
    }

  return matches;
}

/* To disable readline's filename completion. */
static char *
vtysh_completion_entry_function (const char *ignore, int invoking_key)
{
  return NULL;
}

void
vtysh_readline_init (void)
{
  /* readline related settings. */
 // rl_bind_key ('?', (Function *) vtysh_rl_describe);
  rl_completion_entry_function = vtysh_completion_entry_function;
  rl_attempted_completion_function = (CPPFunction *)new_completion;
  /* do not append space after completion. It will be appended
   * in new_completion() function explicitly. */
  rl_completion_append_character = '\0';
}

char *
vtysh_prompt (void)
{
    static struct utsname names;
    static char buf[100];
    const char*hostname;
    extern struct host host;

    hostname = host.name;

    if (!hostname)
    {
        if (!names.nodename[0])
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
            snprintf (buf, sizeof(buf), cmd_prompt(vty->node), hostname);
            break;
        case CHANGE_PASSWD_NODE:
        case CHANGE_PASSWD_AGAIN_NODE:
            snprintf (buf, sizeof(buf), cmd_prompt(vty->node));
            break;


        default:
            snprintf (buf, sizeof(buf), "Carsmart#");
            break;
    }

  return buf;
}

void
vtysh_init_vty (void)
{
  /* Make vty structure. */
  vty = vty_new ();
  vty->type = VTY_SHELL;
  vty->node = USER_NODE;
  vty->language = CMD_HELP_LANG_ENGLISH;
  vty->v_timeout = VTY_TIMEOUT_DEFAULT;

}

/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
char *
vtysh_rl_gets ()
{
  HIST_ENTRY *last;
  /* If the buffer has already been allocated, return the memory
   * to the free pool. */
  if (line_read)
    {
      free (line_read);
      line_read = NULL;
    }

    /* 重新登陆 */
    if (USER_NODE == vty->node)
    {
        return NULL;
    }
     
  /* Get a line from the user.  Change prompt according to node.  XXX. */
  line_read = readline (vtysh_prompt ());
     
  /* If the line has any text in it, save it on the history. But only if
   * last command in history isn't the same one. */
  if (line_read && *line_read)
    {
      using_history();
      last = previous_history();
      if (!last || strcmp (last->line, line_read) != 0) {
    	add_history (line_read);
    	append_history(1,history_file);
      }
    }

    /* 处理特殊情况 */
    if (NULL != line_read)
    {
        /* 超时,或者ctr+c */
        if (CTR_C == line_read[0])
        {
            free(line_read);
            line_read = NULL;
        }
        
        /* ctr+Z */
        else if (CTR_Z == line_read[0])
        {
            line_read[0] = '\n';
            line_read[1] = 0;
            printf("\n\r");
        }
    }
     
  return (line_read);
}

/* 用户登陆认证 */
static void shell_login(struct vty *pstVty)
{
    int iUserIndex;
    char szUserName[MAX_USER_NAME_LEN+1];
    struct termios stTermiosOld;
    struct termios stTermios;

    while (1)
    {
        if (NULL != line_read)
        {
            free(line_read);
        }
        line_read = readline (vtysh_prompt());
        if ((NULL == line_read) || (0 == *line_read) || (strlen(line_read) > MAX_USER_NAME_LEN) )
        {
            pstVty->node = USER_NODE;        
            continue;
        }

        /* 为了开发阶段方便退出 */
        if (0 == strcmp(line_read, "!quit"))
        {
            exit(0);
        }
        
        sprintf(szUserName, line_read);
        pstVty->node = AUTH_NODE;
        /*
        tcgetattr(STDIN_FILENO, &stTermios);
        stTermiosOld = stTermios;
        stTermios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
        tcsetattr(STDIN_FILENO, TCSANOW, &stTermios);

        free(line_read);
        line_read = readline (vtysh_prompt());
        tcsetattr(STDIN_FILENO, TCSANOW, &stTermiosOld);
        */
        rl_set_echo(0);
        free(line_read);
        line_read = readline (vtysh_prompt());
        rl_set_echo(1);
        
        pstVty->node = USER_NODE;
        if ((NULL == line_read) || (0 == *line_read) || (strlen(line_read) > MAX_PASSWD_LEN) )
        {
            vty_out(vty, VTY_NEWLINE);
            continue;
        }   

        if (CMD_SUCCESS == use_auth(szUserName, line_read, &vty->iPriority))
        {
            iUserIndex = registe_user(szUserName, 1);
            if (MAX_CLI_USER == iUserIndex)
            {
                vty_out(vty, "Has reached the maximum user limit.%s", VTY_NEWLINE);
                continue;
            }
            pstVty->user_index = iUserIndex;

            break;
        }
        else
        {
            vty_out(vty, "Incorrect user name or password%s", VTY_NEWLINE);
            continue;
        }
    }

    pstVty->node = VIEW_NODE;
    vty_out(vty, VTY_NEWLINE);

    return;
}

struct termios g_stTermiosOld;
void vtysh_reset()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &g_stTermiosOld);
    return;
}
int vtysh_ctl_z(int count, int c)
{
    switch (vty->node)
    {
        case USER_NODE:
        case AUTH_NODE:			
        case VIEW_NODE:			
        case AUTH_ENABLE_NODE:
        case ENABLE_NODE:
        case CHANGE_PASSWD_NODE:
        case CHANGE_PASSWD_AGAIN_NODE:

            break;
        default:
            vty->node = ENABLE_NODE;
            //vty_config_unlock(vty);
            break;
    }
    
    return 0;
}
int vtysh_ctl_c(int count, int c)
{
    rl_line_buffer[0] = CTR_C;
    rl_line_buffer[1] = 0;
    return 0;
}

void init_termios()
{
    struct termios stTermios;

    tcgetattr(STDIN_FILENO, &stTermios);
    g_stTermiosOld = stTermios;
    stTermios.c_cc[VSUSP] = _POSIX_VDISABLE;
    tcsetattr(STDIN_FILENO, TCSANOW, &stTermios);
    rl_bind_key(CTR_Z, vtysh_ctl_z);
    rl_bind_key(CTR_C, vtysh_ctl_c);
}
void vty_shell_sock (void *arg)
{
    vtysh_init_vty();
    vtysh_readline_init();
    init_termios();
    atexit(vtysh_reset);
relogin:    
    vty_hello (vty);
    rl_bind_key('?', rl_insert);
    rl_set_keyboard_input_timeout(0);
    rl_set_echo(1);

    /* 用户登陆认证 */
    shell_login(vty);

    /* 注册帮助信息的处理函数 */
    rl_bind_key ('?', (Function *) vtysh_rl_describe);
    rl_set_keyboard_input_timeout(vty->v_timeout);

    snprintf(history_file, sizeof(history_file), "%s/.history_quagga", getenv("HOME"));
    read_history(history_file);
    /* Main command loop. */
    while (vtysh_rl_gets())
    {
        vtysh_execute (line_read);
    }

    /* 超时之后的清理工作 */
    vty->node = USER_NODE;
    release_user(vty->user_index, 1);
    //vty_config_unlock(vty);
    
    goto relogin;

    history_truncate_file(history_file,1000);
    printf ("\n");

    return;
}

/* 处理进入enable模式的password */
int shell_enable(struct vty *pstVty)
{
    struct termios stTermiosOld;
    struct termios stTermios;

    /* 设置不回显 */
    /*
    tcgetattr(STDIN_FILENO, &stTermios);
    stTermiosOld = stTermios;
    stTermios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
    tcsetattr(STDIN_FILENO, TCSANOW, &stTermios);
    rl_bind_key('?', rl_insert);

    free(line_read);
    line_read = readline (vtysh_prompt());
    tcsetattr(STDIN_FILENO, TCSANOW, &stTermiosOld);
    rl_bind_key ('?', (Function *) vtysh_rl_describe);
    */

    rl_bind_key('?', rl_insert);
    rl_set_echo(0);
    free(line_read);
    line_read = readline (vtysh_prompt());
    rl_set_echo(1);
    rl_bind_key ('?', (Function *) vtysh_rl_describe);
    
    return enable_auth(line_read);
}

/* 入参不做检查，外部保证正确性,输入密码超过最大长度，返回CMD_WARNING */
int change_shell_passwd(struct vty *pstVty, char *pcPasswd, char *pcPasswdAgain)
{
    struct termios stTermiosOld;
    struct termios stTermios;
    char *pcPasswdTmp;
    char *pcPasswdAgainTmp = NULL;
    int iErrCode = CMD_SUCCESS;
    int iPasswdLen;
    
    /* 设置不回显 
    tcgetattr(STDIN_FILENO, &stTermios);
    stTermiosOld = stTermios;
    stTermios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
    tcsetattr(STDIN_FILENO, TCSANOW, &stTermios);
    rl_bind_key('?', rl_insert);
    */
    rl_set_echo(0);
    rl_bind_key('?', rl_insert);

    pstVty->node = CHANGE_PASSWD_NODE;
    pcPasswdTmp = readline(vtysh_prompt());
    vty_out(pstVty,VTY_NEWLINE);
    
    iPasswdLen = snprintf(pcPasswd, (MAX_PASSWD_LEN+1), pcPasswdTmp);
    if((iPasswdLen > MAX_PASSWD_LEN) || (iPasswdLen <= 0))
    {
        iErrCode = CMD_WARNING;
    }
    else
    {
        pstVty->node = CHANGE_PASSWD_AGAIN_NODE;
        pcPasswdAgainTmp = readline(vtysh_prompt());
        vty_out(pstVty,VTY_NEWLINE);
        
        iPasswdLen = snprintf(pcPasswdAgain, (MAX_PASSWD_LEN+1), pcPasswdAgainTmp);

        if((iPasswdLen > MAX_PASSWD_LEN) || (iPasswdLen <= 0))
        {
            iErrCode = CMD_WARNING;
        }
    }
    
    pstVty->node = ENABLE_NODE;
    
    /* 回复回显 
    tcsetattr(STDIN_FILENO, TCSANOW, &stTermiosOld);*/
    rl_set_echo(1);
    rl_bind_key ('?', (Function *) vtysh_rl_describe);

    free(pcPasswdTmp);
    free(pcPasswdAgainTmp);

    return iErrCode;
}
