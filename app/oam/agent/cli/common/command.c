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
#include <stdio.h>

#include "readline.h"
#include "history.h"

#include "zebra.h"
#include "memory.h"
#include "cli_version.h"
#include "vector.h"
#include "vty.h"
#include "command.h"
#include "cli_lib.h"

#include "cv_osal.h"


#define HOSTNAME_DEF  "Carsmart"

extern void install_dev_manage_cmd();
extern void install_net_cmd();

/* Command vector which includes some level of command lists. Normally
   each daemon maintains each own cmdvec. */
vector cmdvec = NULL;

struct desc deoam_cr;
char *command_cr = NULL;

/* Host information structure. */
struct host host;

/* Standard command node structures. */
static struct cmd_node user_node =
{
  .node =   USER_NODE,
  .prompt = "Login:"
};

static struct cmd_node auth_node =
{
  .node =   AUTH_NODE,
  .prompt = "Password:"
};

static struct cmd_node view_node =
{
  .node =   VIEW_NODE,
  .prompt = "%s>"
};

static struct cmd_node auth_enable_node =
{
  .node =   AUTH_ENABLE_NODE,
  .prompt = "Password:"
};

static struct cmd_node enable_node =
{
  .node =   ENABLE_NODE,
  .prompt = "%s#"
};

/* 修改设备进入特权模式的密码 */
static struct cmd_node change_pass_node =
{
  .node =   CHANGE_PASSWD_NODE,
  .prompt = "Please input password:"
};
static struct cmd_node change_pass_again_node =
{
  .node =   CHANGE_PASSWD_AGAIN_NODE,
  .prompt = "Please input again:"
};

static struct cmd_node config_node =
{
  .node =   CONFIG_NODE,
  .prompt = "%s(config)#"
};

static struct cmd_node debug_node =
{
  .node =   DEBUG_NODE,
  .prompt = "%s(debug)#"
};


/* Default motd string. */
static const char *default_motd =
"\r\n\
Hello, this is " CLI_AGNET_PROGNAME " (version " CLI_AGNET_VERSION ").\r\n\
\r\n";
/*" CLI_AGNET_COPYRIGHT "\r\n\*/

/* Utility function to concatenate argv argument into a single string
   with inserting ' ' character between each argument.  */
char *
argv_concat (const char **argv, int argc, int shift)
{
  int i;
  size_t len;
  char *str;
  char *p;

  len = 0;
  for (i = shift; i < argc; i++)
    len += strlen(argv[i])+1;
  if (!len)
    return NULL;
  p = str = XMALLOC(MTYPE_TMP, len);
  for (i = shift; i < argc; i++)
    {
      size_t arglen;
      memcpy(p, argv[i], (arglen = strlen(argv[i])));
      p += arglen;
      *p++ = ' ';
    }
  *(p-1) = '\0';
  return str;
}

/* Install top node of command vector. */
void
install_node (struct cmd_node *node,
	      int (*func) (struct vty *))
{
  vector_set_index (cmdvec, node->node, node);
  node->func = func;
  node->cmd_vector = vector_init (VECTOR_MIN_SIZE);
}

/* Compare two command's string.  Used in sort_node (). */
static int
cmp_node (const void *p, const void *q)
{
  const struct cmd_element *a = *(struct cmd_element * const *)p;
  const struct cmd_element *b = *(struct cmd_element * const *)q;

  return strcasecmp (a->string, b->string);
}

static int
cmp_desc (const void *p, const void *q)
{
  const struct desc *a = *(struct desc * const *)p;
  const struct desc *b = *(struct desc * const *)q;

  return strcmp (a->cmd, b->cmd);
}

/* Sort each node's command element according to command string. */
void
sort_node ()
{
  unsigned int i, j;
  struct cmd_node *cnode;
  vector descvec;
  struct cmd_element *cmd_element;

  for (i = 0; i < vector_active (cmdvec); i++)
    if ((cnode = vector_slot (cmdvec, i)) != NULL)
      {
	vector cmd_vector = cnode->cmd_vector;
	qsort (cmd_vector->index, vector_active (cmd_vector),
	       sizeof (void *), cmp_node);

	for (j = 0; j < vector_active (cmd_vector); j++)
	  if ((cmd_element = vector_slot (cmd_vector, j)) != NULL
	      && vector_active (cmd_element->strvec))
	    {
	      descvec = vector_slot (cmd_element->strvec,
				     vector_active (cmd_element->strvec) - 1);
	      qsort (descvec->index, vector_active (descvec),
	             sizeof (void *), cmp_desc);
	    }
      }
}

/* Breaking up string into each command piece. I assume given
   character is separated by a space character. Return value is a
   vector which includes char ** data element. */
vector
cmd_make_strvec (const char *string)
{
  const char *cp, *start;
  char *token;
  int strlen;
  vector strvec;

  if (string == NULL)
    return NULL;

  cp = string;

  /* Skip white spaces. */
  while (isspace ((int) *cp) && *cp != '\0')
    cp++;

  /* Return if there is only white spaces */
  if (*cp == '\0')
    return NULL;

  /* Prepare return vector. */
  strvec = vector_init (VECTOR_MIN_SIZE);

  /* Copy each command piece and set into vector. */
  while (1)
  {
      start = cp;
      while (!(isspace ((int) *cp) || *cp == '\r' || *cp == '\n') &&
	     *cp != '\0')
	cp++;
      strlen = cp - start;
      token = XMALLOC (MTYPE_STRVEC, strlen + 1);
      memcpy (token, start, strlen);
      *(token + strlen) = '\0';
      vector_set (strvec, token);

      while ((isspace ((int) *cp) || *cp == '\n' || *cp == '\r') &&
	     *cp != '\0')
	cp++;

      if (*cp == '\0')
	return strvec;
  }
}

/* Free allocated string vector. */
void
cmd_free_strvec (vector v)
{
  unsigned int i;
  char *cp;

  if (!v)
    return;

  for (i = 0; i < vector_active (v); i++)
    if ((cp = vector_slot (v, i)) != NULL)
      XFREE (MTYPE_STRVEC, cp);

  vector_free (v);
}

/* Fetch next description.  Used in cmd_make_descvec(). */
static char *
cmd_deoam_str (const char **string)
{
  const char *cp, *start;
  char *token;
  int strlen;

  cp = *string;

  if (cp == NULL)
    return NULL;

  /* Skip white spaces. */
  while (isspace ((int) *cp) && *cp != '\0')
    cp++;

  /* Return if there is only white spaces */
  if (*cp == '\0')
    return NULL;

  start = cp;

  while (!(*cp == '\r' || *cp == '\n') && *cp != '\0')
    cp++;

  strlen = cp - start;
  token = XMALLOC (MTYPE_STRVEC, strlen + 1);
  memcpy (token, start, strlen);
  *(token + strlen) = '\0';

  *string = cp;

  return token;
}

/* New string vector. */
static vector
cmd_make_descvec (const char *string, const char *descstr)
{
  int multiple = 0;
  const char *sp;
  char *token;
  int len;
  const char *cp;
  const char *dp;
  int i = 0;
  vector allvec;
  vector strvec = NULL;
  struct desc *desc;

  cp = string;
  dp = descstr;

  if (cp == NULL)
    return NULL;

  allvec = vector_init (VECTOR_MIN_SIZE);

  while (1)
    {
      while (isspace ((int) *cp) && *cp != '\0')
	cp++;

      if (*cp == '(')
	{
	  multiple = 1;
	  cp++;
	}
      if (*cp == ')')
	{
	  multiple = 0;
	  cp++;
	}
      if (*cp == '|')
	{
	  if (! multiple)
	    {
	      fprintf (stderr, "Command parse error!: %s\n", string);
	      exit (1);
	    }
	  cp++;
	}

      while (isspace ((int) *cp) && *cp != '\0')
	cp++;

      if (*cp == '(')
	{
	  multiple = 1;
	  cp++;
	}

      if (*cp == '\0')
	return allvec;

      sp = cp;

      while (! (isspace ((int) *cp) || *cp == '\r' || *cp == '\n' || *cp == ')' || *cp == '|') && *cp != '\0')
	cp++;

      len = cp - sp;

      token = XMALLOC (MTYPE_STRVEC, len + 1);
      memcpy (token, sp, len);
      *(token + len) = '\0';

      desc = XCALLOC (MTYPE_DESC, sizeof (struct desc));
      desc->cmd = token;
      for (i = 0; i < CMD_HELP_LANG_NUM; i++)
      {
        desc->str[i] = cmd_deoam_str (&dp);
      }

      if (multiple)
	{
	  if (multiple == 1)
	    {
	      strvec = vector_init (VECTOR_MIN_SIZE);
	      vector_set (allvec, strvec);
	    }
	  multiple++;
	}
      else
	{
	  strvec = vector_init (VECTOR_MIN_SIZE);
	  vector_set (allvec, strvec);
	}
      vector_set (strvec, desc);
    }
}

/* Count mandantory string vector size.  This is to determine inputed
   command has enough command length. */
static int
cmd_cmdsize (vector strvec)
{
  unsigned int i;
  int size = 0;
  vector descvec;
  struct desc *desc;

  for (i = 0; i < vector_active (strvec); i++)
    if ((descvec = vector_slot (strvec, i)) != NULL)
    {
      if ((vector_active (descvec)) == 1
        && (desc = vector_slot (descvec, 0)) != NULL)
	{
	  if (desc->cmd == NULL || CMD_OPTION (desc->cmd))
	    return size;
	  else
	    size++;
	}
      else
	size++;
    }
  return size;
}

/* Return prompt character of specified node. */
const char *
cmd_prompt (enum node_type node)
{
  struct cmd_node *cnode;

  cnode = vector_slot (cmdvec, node);
  return cnode->prompt;
}

/* Install a command into a node. */
void
install_element (enum node_type ntype, struct cmd_element *cmd)
{
  struct cmd_node *cnode;

  /* cmd_init hasn't been called */
  if (!cmdvec)
    return;

  cnode = vector_slot (cmdvec, ntype);

  if (cnode == NULL)
    {
      fprintf (stderr, "Command node %d doesn't exist, please check it\n",
	       ntype);
      exit (1);
    }

  vector_set (cnode->cmd_vector, cmd);

  if (cmd->strvec == NULL)
    cmd->strvec = cmd_make_descvec (cmd->string, cmd->doc);

  cmd->cmdsize = cmd_cmdsize (cmd->strvec);
}

void install_show_running (enum node_type ntype,
    int (*func)(int port_id, char *p_outbuf, int outbuf_len))
{
    struct cmd_node *cnode;
    cli_running_show_t *p_malloc = NULL;    
    cli_running_show_t *p_last = NULL;    
    /* cmd_init hasn't been called */
    if (!cmdvec)
    {
        return;
    }
    
    cnode = vector_slot (cmdvec, ntype);
    if (cnode == NULL)
    {
        fprintf (stderr, "Command node %d doesn't exist, please check it\n",
           ntype);
        return;
    }
    p_last = cnode->p_running_show;
    p_malloc = (cli_running_show_t *)malloc(sizeof(cli_running_show_t));
    if(p_malloc == NULL)
    {
        fprintf(stderr,"malloc error:%d\n",sizeof(cli_running_show_t));
        return;
    }
    p_malloc->func = func;
    p_malloc->p_next = NULL;
    if(cnode->p_running_show == NULL)
    {
        cnode->p_running_show = p_malloc;
        return;
    }
    while(p_last)
    {
        if(p_last->p_next == NULL)
        {
            p_last->p_next = p_malloc;
            return;
        }
        p_last = p_last->p_next;
    }
    fprintf(stderr,"install_show_running error:%d\n",ntype);
    return;
}
/* Utility function for getting command vector. */
static vector
cmd_node_vector (vector v, enum node_type ntype)
{
  struct cmd_node *cnode = vector_slot (v, ntype);
  return cnode->cmd_vector;
}

/* Completion match types. */
enum match_type
{
  no_match,
  extend_match,
  ipv4_prefix_match,
  ipv4_match,
  ipv6_prefix_match,
  ipv6_match,
  range_match,
  multi_match,
  vararg_match,
  partly_match,
  exact_match
};

static enum match_type
cmd_ipv4_match (const char *str)
{
  const char *sp;
  int dots = 0, nums = 0;
  char buf[4];

  if (str == NULL)
    return partly_match;

  for (;;)
    {
      memset (buf, 0, sizeof (buf));
      sp = str;
      while (*str != '\0')
	{
	  if (*str == '.')
	    {
	      if (dots >= 3)
		return no_match;

	      if (*(str + 1) == '.')
		return no_match;

	      if (*(str + 1) == '\0')
		return partly_match;

	      dots++;
	      break;
	    }
	  if (!isdigit ((int) *str))
	    return no_match;

	  str++;
	}

      if (str - sp > 3)
	return no_match;

      strncpy (buf, sp, str - sp);
      if (atoi (buf) > 255)
	return no_match;

      nums++;

      if (*str == '\0')
	break;

      str++;
    }

  if (nums < 4)
    return partly_match;

  return exact_match;
}

static enum match_type
cmd_ipv4_prefix_match (const char *str)
{
  const char *sp;
  int dots = 0;
  char buf[4];

  if (str == NULL)
    return partly_match;

  for (;;)
    {
      memset (buf, 0, sizeof (buf));
      sp = str;
      while (*str != '\0' && *str != '/')
	{
	  if (*str == '.')
	    {
	      if (dots == 3)
		return no_match;

	      if (*(str + 1) == '.' || *(str + 1) == '/')
		return no_match;

	      if (*(str + 1) == '\0')
		return partly_match;

	      dots++;
	      break;
	    }

	  if (!isdigit ((int) *str))
	    return no_match;

	  str++;
	}

      if (str - sp > 3)
	return no_match;

      strncpy (buf, sp, str - sp);
      if (atoi (buf) > 255)
	return no_match;

      if (dots == 3)
	{
	  if (*str == '/')
	    {
	      if (*(str + 1) == '\0')
		return partly_match;

	      str++;
	      break;
	    }
	  else if (*str == '\0')
	    return partly_match;
	}

      if (*str == '\0')
	return partly_match;

      str++;
    }

  sp = str;
  while (*str != '\0')
    {
      if (!isdigit ((int) *str))
	return no_match;

      str++;
    }

  if (atoi (sp) > 32)
    return no_match;

  return exact_match;
}

#define IPV6_ADDR_STR		"0123456789abcdefABCDEF:.%"
#define IPV6_PREFIX_STR		"0123456789abcdefABCDEF:.%/"
#define STATE_START		1
#define STATE_COLON		2
#define STATE_DOUBLE		3
#define STATE_ADDR		4
#define STATE_DOT               5
#define STATE_SLASH		6
#define STATE_MASK		7

#ifdef HAVE_IPV6

static enum match_type
cmd_ipv6_match (const char *str)
{
  int state = STATE_START;
  int colons = 0, nums = 0, double_colon = 0;
  const char *sp = NULL;
  struct sockaddr_in6 sin6_dummy;
  int ret;

  if (str == NULL)
    return partly_match;

  if (strspn (str, IPV6_ADDR_STR) != strlen (str))
    return no_match;

  /* use inet_pton that has a better support,
   * for example inet_pton can support the automatic addresses:
   *  ::1.2.3.4
   */
  ret = inet_pton(AF_INET6, str, &sin6_dummy.sin6_addr);

  if (ret == 1)
    return exact_match;

  while (*str != '\0')
    {
      switch (state)
	{
	case STATE_START:
	  if (*str == ':')
	    {
	      if (*(str + 1) != ':' && *(str + 1) != '\0')
		return no_match;
     	      colons--;
	      state = STATE_COLON;
	    }
	  else
	    {
	      sp = str;
	      state = STATE_ADDR;
	    }

	  continue;
	case STATE_COLON:
	  colons++;
	  if (*(str + 1) == ':')
	    state = STATE_DOUBLE;
	  else
	    {
	      sp = str + 1;
	      state = STATE_ADDR;
	    }
	  break;
	case STATE_DOUBLE:
	  if (double_colon)
	    return no_match;

	  if (*(str + 1) == ':')
	    return no_match;
	  else
	    {
	      if (*(str + 1) != '\0')
		colons++;
	      sp = str + 1;
	      state = STATE_ADDR;
	    }

	  double_colon++;
	  nums++;
	  break;
	case STATE_ADDR:
	  if (*(str + 1) == ':' || *(str + 1) == '\0')
	    {
	      if (str - sp > 3)
		return no_match;

	      nums++;
	      state = STATE_COLON;
	    }
	  if (*(str + 1) == '.')
	    state = STATE_DOT;
	  break;
	case STATE_DOT:
	  state = STATE_ADDR;
	  break;
	default:
	  break;
	}

      if (nums > 8)
	return no_match;

      if (colons > 7)
	return no_match;

      str++;
    }

#if 0
  if (nums < 11)
    return partly_match;
#endif /* 0 */

  return exact_match;
}

static enum match_type
cmd_ipv6_prefix_match (const char *str)
{
  int state = STATE_START;
  int colons = 0, nums = 0, double_colon = 0;
  int mask;
  const char *sp = NULL;
  char *endptr = NULL;

  if (str == NULL)
    return partly_match;

  if (strspn (str, IPV6_PREFIX_STR) != strlen (str))
    return no_match;

  while (*str != '\0' && state != STATE_MASK)
    {
      switch (state)
	{
	case STATE_START:
	  if (*str == ':')
	    {
	      if (*(str + 1) != ':' && *(str + 1) != '\0')
		return no_match;
	      colons--;
	      state = STATE_COLON;
	    }
	  else
	    {
	      sp = str;
	      state = STATE_ADDR;
	    }

	  continue;
	case STATE_COLON:
	  colons++;
	  if (*(str + 1) == '/')
	    return no_match;
	  else if (*(str + 1) == ':')
	    state = STATE_DOUBLE;
	  else
	    {
	      sp = str + 1;
	      state = STATE_ADDR;
	    }
	  break;
	case STATE_DOUBLE:
	  if (double_colon)
	    return no_match;

	  if (*(str + 1) == ':')
	    return no_match;
	  else
	    {
	      if (*(str + 1) != '\0' && *(str + 1) != '/')
		colons++;
	      sp = str + 1;

	      if (*(str + 1) == '/')
		state = STATE_SLASH;
	      else
		state = STATE_ADDR;
	    }

	  double_colon++;
	  nums += 1;
	  break;
	case STATE_ADDR:
	  if (*(str + 1) == ':' || *(str + 1) == '.'
	      || *(str + 1) == '\0' || *(str + 1) == '/')
	    {
	      if (str - sp > 3)
		return no_match;

	      for (; sp <= str; sp++)
		if (*sp == '/')
		  return no_match;

	      nums++;

	      if (*(str + 1) == ':')
		state = STATE_COLON;
	      else if (*(str + 1) == '.')
		state = STATE_DOT;
	      else if (*(str + 1) == '/')
		state = STATE_SLASH;
	    }
	  break;
	case STATE_DOT:
	  state = STATE_ADDR;
	  break;
	case STATE_SLASH:
	  if (*(str + 1) == '\0')
	    return partly_match;

	  state = STATE_MASK;
	  break;
	default:
	  break;
	}

      if (nums > 11)
	return no_match;

      if (colons > 7)
	return no_match;

      str++;
    }

  if (state < STATE_MASK)
    return partly_match;

  mask = strtol (str, &endptr, 10);
  if (*endptr != '\0')
    return no_match;

  if (mask < 0 || mask > 128)
    return no_match;

/* I don't know why mask < 13 makes command match partly.
   Forgive me to make this comments. I Want to set static default route
   because of lack of function to originate default in ospf6d; sorry
       yasu
  if (mask < 13)
    return partly_match;
*/

  return exact_match;
}

#endif /* HAVE_IPV6  */

#define DECIMAL_STRLEN_MAX 10

static int
cmd_multi_match (const char *range, const char *str)
{
    char *p;
    char buf[DECIMAL_STRLEN_MAX + 1];
    char *endptr = NULL;
    unsigned long min, max, val, val_max;
    const char *cp;
    const char *sp;

    if (str == NULL)
        return 1;

    range++;
    p = strchr (range, '-');
    if (p == NULL)
        return 0;
    if (p - range > DECIMAL_STRLEN_MAX)
        return 0;
    strncpy (buf, range, p - range);
    buf[p - range] = '\0';
    min = strtoul (buf, &endptr, 10);
    if (*endptr != '\0')
        return 0;

    range = p + 1;
    p = strchr (range, '}');
    if (p == NULL)
        return 0;
    if (p - range > DECIMAL_STRLEN_MAX)
        return 0;
    strncpy (buf, range, p - range);
    buf[p - range] = '\0';
    max = strtoul (buf, &endptr, 10);
    if (*endptr != '\0')
        return 0;

    cp = str;
    while(1)
    {
        while (isspace ((int) *cp) && *cp != '\0')
			cp++;

        if ('\0' == *cp)
            break;

        sp = cp;

        while (!(',' == *cp || '-' == *cp) && '\0' != *cp)
            cp++;

        if (cp == sp)
            break;

        strncpy (buf, sp, cp - sp);
        buf[cp - sp] = '\0';
        val = strtoul (buf, &endptr, 10);
        if (*endptr != '\0')
            return 0;

        if (val < min || val > max)
            return 0;

        if ('\0' == *cp)
            break;

        cp++;
        if ('-' == *(cp - 1))
        {
            sp = cp;
            while (',' != *cp && '\0' != *cp)
                cp++;

            if (cp == sp)
                break;
            strncpy (buf, sp, cp - sp);
            buf[cp - sp] = '\0';
            val_max = strtoul (buf, &endptr, 10);
            if (*endptr != '\0')
                return 0;

            if (val_max < val || val_max < min || val_max > max)
                return 0;
        }
    }

    return 1;
}

static int
cmd_range_match (const char *range, const char *str)
{
  char *endptr = NULL;
  long min, max, val;

  if (str == NULL)
    return 1;

  val = strtol (str, &endptr, 10);
  if (*endptr != '\0')
    return 0;

  range++;
  min = strtol (range, &endptr, 10);
  if (*endptr != '-')
    return 0;

  range = endptr + 1;
  max = strtol (range, &endptr, 10);
  if (*endptr != '>')
    return 0;

  if (val < min || val > max)
    return 0;

  return 1;
}

/* Make completion match and return match type flag. */
static enum match_type
cmd_filter_by_completion (char *command, vector v, unsigned int index)
{
  unsigned int i;
  const char *str;
  struct cmd_element *cmd_element;
  enum match_type match_type;
  vector descvec;
  struct desc *desc;

  match_type = no_match;

  /* If command and cmd_element string does not match set NULL to vector */
  for (i = 0; i < vector_active (v); i++)
    if ((cmd_element = vector_slot (v, i)) != NULL)
      {
	if (index >= vector_active (cmd_element->strvec))
	  vector_slot (v, i) = NULL;
	else
	  {
	    unsigned int j;
	    int matched = 0;

	    descvec = vector_slot (cmd_element->strvec, index);

	    for (j = 0; j < vector_active (descvec); j++)
	      if ((desc = vector_slot (descvec, j)))
		{
		  str = desc->cmd;

		  if (CMD_VARARG (str))
		    {
		      if (match_type < vararg_match)
			match_type = vararg_match;
		      matched++;
		    }
		  else if (CMD_RANGE (str))
		    {
		      if (cmd_range_match (str, command))
			{
			  if (match_type < range_match)
			    match_type = range_match;

			  matched++;
			}
		    }
          else if (CMD_MULTI(str) )
            {
            	if (cmd_multi_match (str, command))
            	{
            		if (match_type < multi_match)
            			match_type = multi_match;

            		matched++;
            	}
            }
#ifdef HAVE_IPV6
		  else if (CMD_IPV6 (str))
		    {
		      if (cmd_ipv6_match (command))
			{
			  if (match_type < ipv6_match)
			    match_type = ipv6_match;

			  matched++;
			}
		    }
		  else if (CMD_IPV6_PREFIX (str))
		    {
		      if (cmd_ipv6_prefix_match (command))
			{
			  if (match_type < ipv6_prefix_match)
			    match_type = ipv6_prefix_match;

			  matched++;
			}
		    }
#endif /* HAVE_IPV6  */
		  else if (CMD_IPV4 (str))
          {
              if (exact_match == cmd_ipv4_match(command))
              {
                  if (match_type < ipv4_match)
                  match_type = ipv4_match;

                  matched++;
              }
          }
		  else if (CMD_IPV4_PREFIX (str))
		    {
		      if (cmd_ipv4_prefix_match (command))
			{
			  if (match_type < ipv4_prefix_match)
			    match_type = ipv4_prefix_match;
			  matched++;
			}
		    }
		  else
		    /* Check is this point's argument optional ? */
		  if (CMD_OPTION (str) || CMD_VARIABLE (str))
		    {
		      if (match_type < extend_match)
			match_type = extend_match;
		      matched++;
		    }
		  else if (strncasecmp (command, str, strlen (command)) == 0)
		    {
		      if (strcasecmp (command, str) == 0)
			match_type = exact_match;
		      else
			{
			  if (match_type < partly_match)
			    match_type = partly_match;
			}
		      matched++;
		    }
		}
	    if (!matched)
	      vector_slot (v, i) = NULL;
	  }
      }
  return match_type;
}

/* Check ambiguous match */
static int
is_cmd_ambiguous (char *command, vector v, int index, enum match_type type)
{
  unsigned int i;
  unsigned int j;
  const char *str = NULL;
  struct cmd_element *cmd_element;
  const char *matched = NULL;
  vector descvec;
  struct desc *desc;

  for (i = 0; i < vector_active (v); i++)
    if ((cmd_element = vector_slot (v, i)) != NULL)
      {
	int match = 0;

	descvec = vector_slot (cmd_element->strvec, index);

	for (j = 0; j < vector_active (descvec); j++)
	  if ((desc = vector_slot (descvec, j)))
	    {
	      enum match_type ret;

	      str = desc->cmd;

	      switch (type)
		{
		case exact_match:
		  if (!(CMD_OPTION (str) || CMD_VARIABLE (str))
		      && strcasecmp (command, str) == 0)
		    match++;
		  break;
		case partly_match:
		  if (!(CMD_OPTION (str) || CMD_VARIABLE (str))
		      && strncasecmp (command, str, strlen (command)) == 0)
		    {
		      if (matched && strcasecmp (matched, str) != 0)
			return 1;	/* There is ambiguous match. */
		      else
			matched = str;
		      match++;
		    }
		  break;
		case range_match:
		  if (cmd_range_match (str, command))
		    {
		      if (matched && strcasecmp (matched, str) != 0)
			return 1;
		      else
			matched = str;
		      match++;
		    }
		  break;
        case multi_match:
            if (cmd_multi_match (str, command))
            {
                if (matched && strcasecmp (matched, str) != 0)
                	return 1;
                else
                	matched = str;
                match++;
            }
            break;
#ifdef HAVE_IPV6
		case ipv6_match:
		  if (CMD_IPV6 (str))
		    match++;
		  break;
		case ipv6_prefix_match:
		  if ((ret = cmd_ipv6_prefix_match (command)) != no_match)
		    {
		      if (ret == partly_match)
			return 2;	/* There is incomplete match. */

		      match++;
		    }
		  break;
#endif /* HAVE_IPV6 */
		case ipv4_match:
		  if (CMD_IPV4 (str))
		    match++;
		  break;
		case ipv4_prefix_match:
		  if ((ret = cmd_ipv4_prefix_match (command)) != no_match)
		    {
		      if (ret == partly_match)
			return 2;	/* There is incomplete match. */

		      match++;
		    }
		  break;
		case extend_match:
		  if (CMD_OPTION (str) || CMD_VARIABLE (str))
		    match++;
		  break;
		case no_match:
		default:
		  break;
		}
	    }
	if (!match)
	  vector_slot (v, i) = NULL;
      }
  return 0;
}

/* If src matches dst return dst string, otherwise return NULL */
static const char *
cmd_entry_function (const char *src, const char *dst)
{
  /* Skip variable arguments. */
  if (CMD_OPTION (dst) || CMD_VARIABLE (dst) || CMD_VARARG (dst) ||
      CMD_IPV4 (dst) || CMD_IPV4_PREFIX (dst) || CMD_RANGE (dst) || CMD_MULTI(dst))
    return NULL;

  /* In case of 'command \t', given src is NULL string. */
  if (src == NULL)
    return dst;

  /* Matched with input string. */
  if (strncasecmp (src, dst, strlen (src)) == 0)
    return dst;

  return NULL;
}

/* If src matches dst return dst string, otherwise return NULL */
/* This version will return the dst string always if it is
   CMD_VARIABLE for '?' key processing */
static const char *
cmd_entry_function_desc (const char *src, const char *dst)
{
  if (CMD_VARARG (dst))
    return dst;

  if (CMD_RANGE (dst))
    {
      if (cmd_range_match (dst, src))
	return dst;
      else
	return NULL;
    }

    if (CMD_MULTI (dst))
    {
      if (cmd_multi_match (dst, src))
	return dst;
      else
	return NULL;
    }
#ifdef HAVE_IPV6
  if (CMD_IPV6 (dst))
    {
      if (cmd_ipv6_match (src))
	return dst;
      else
	return NULL;
    }

  if (CMD_IPV6_PREFIX (dst))
    {
      if (cmd_ipv6_prefix_match (src))
	return dst;
      else
	return NULL;
    }
#endif /* HAVE_IPV6 */

  if (CMD_IPV4 (dst))
    {
      if (cmd_ipv4_match (src))
	return dst;
      else
	return NULL;
    }

  if (CMD_IPV4_PREFIX (dst))
    {
      if (cmd_ipv4_prefix_match (src))
	return dst;
      else
	return NULL;
    }

  /* Optional or variable commands always match on '?' */
  if (CMD_OPTION (dst) || CMD_VARIABLE (dst))
    return dst;

  /* In case of 'command \t', given src is NULL string. */
  if (src == NULL)
    return dst;

  if (strncasecmp (src, dst, strlen (src)) == 0)
    return dst;
  else
    return NULL;
}

/* Check same string element existence.  If it isn't there return
    1. */
static int
cmd_unique_string (vector v, const char *str)
{
  unsigned int i;
  char *match;

  for (i = 0; i < vector_active (v); i++)
    if ((match = vector_slot (v, i)) != NULL)
      if (strcasecmp (match, str) == 0)
	return 0;
  return 1;
}

/* Compare string to description vector.  If there is same string
   return 1 else return 0. */
static int
deoam_unique_string (vector v, const char *str)
{
  unsigned int i;
  struct desc *desc;

  for (i = 0; i < vector_active (v); i++)
    if ((desc = vector_slot (v, i)) != NULL)
      if (strcasecmp (desc->cmd, str) == 0)
	return 1;
  return 0;
}

static int cmd_try_do_shortcut (enum node_type node, char* first_word)
{
  if ( first_word != NULL &&
       node != USER_NODE &&
       node != AUTH_NODE &&
       node != VIEW_NODE &&
       node != AUTH_ENABLE_NODE &&
       node != ENABLE_NODE &&
       0 == strcmp( "do", first_word ) )
    return 1;
  return 0;
}

/* '?' describe command support. */
static vector
cmd_describe_command_real (vector vline, struct vty *vty, int *status)
{
  unsigned int i;
  vector cmd_vector;
#define INIT_MATCHVEC_SIZE 10
  vector matchvec;
  struct cmd_element *cmd_element;
  unsigned int index;
  int ret;
  enum match_type match;
  char *command;

  /* Set index. */
  if (vector_active (vline) == 0)
    {
      *status = CMD_ERR_NO_MATCH;
      return NULL;
    }
  else
    index = vector_active (vline) - 1;

  /* Make copy vector of current node's command vector. */
  cmd_vector = vector_copy (cmd_node_vector (cmdvec, vty->node));

  /* Prepare match vector */
  matchvec = vector_init (INIT_MATCHVEC_SIZE);

  /* Filter commands. */
  /* Only words precedes current word will be checked in this loop. */
  for (i = 0; i < index; i++)
    if ((command = vector_slot (vline, i)))
      {
	match = cmd_filter_by_completion (command, cmd_vector, i);

	if (match == vararg_match)
	  {
	    struct cmd_element *cmd_element;
	    vector descvec;
	    unsigned int j, k;

	    for (j = 0; j < vector_active (cmd_vector); j++)
	      if ((cmd_element = vector_slot (cmd_vector, j)) != NULL
            && !(cmd_element->attr == CMD_ATTR_DEPRECATED || cmd_element->attr == CMD_ATTR_HIDDEN)
		  && (vector_active (cmd_element->strvec)))
		{
		  descvec = vector_slot (cmd_element->strvec,
					 vector_active (cmd_element->strvec) - 1);
		  for (k = 0; k < vector_active (descvec); k++)
		    {
		      struct desc *desc = vector_slot (descvec, k);
		      vector_set (matchvec, desc);
		    }
		}

	    vector_set (matchvec, &deoam_cr);
	    vector_free (cmd_vector);

	    return matchvec;
	  }

	if ((ret = is_cmd_ambiguous (command, cmd_vector, i, match)) == 1)
	  {
	    vector_free (cmd_vector);
	    vector_free (matchvec);
	    *status = CMD_ERR_AMBIGUOUS;
	    return NULL;
	  }
	else if (ret == 2)
	  {
	    vector_free (cmd_vector);
	    vector_free (matchvec);
	    *status = CMD_ERR_NO_MATCH;
	    return NULL;
	  }
      }

  /* Prepare match vector */
  /*  matchvec = vector_init (INIT_MATCHVEC_SIZE); */

  /* Make sure that cmd_vector is filtered based on current word */
  command = vector_slot (vline, index);
  if (command)
    match = cmd_filter_by_completion (command, cmd_vector, index);

  /* Make description vector. */
  for (i = 0; i < vector_active (cmd_vector); i++)
    if ((cmd_element = vector_slot (cmd_vector, i)) != NULL
         && !(cmd_element->attr == CMD_ATTR_DEPRECATED || cmd_element->attr == CMD_ATTR_HIDDEN))
      {
	vector strvec = cmd_element->strvec;

	/* if command is NULL, index may be equal to vector_active */
	if (command && index >= vector_active (strvec))
	  vector_slot (cmd_vector, i) = NULL;
	else
	  {
	    /* Check if command is completed. */
	    if (command == NULL && index == vector_active (strvec))
	      {
		if (!deoam_unique_string (matchvec, command_cr))
		  vector_set (matchvec, &deoam_cr);
	      }
	    else
	      {
		unsigned int j;
		vector descvec = vector_slot (strvec, index);
		struct desc *desc;

		for (j = 0; j < vector_active (descvec); j++)
		  if ((desc = vector_slot (descvec, j)))
		    {
		      const char *string;

		      string = cmd_entry_function_desc (command, desc->cmd);
		      if (string)
			{
			  /* Uniqueness check */
			  if (!deoam_unique_string (matchvec, string))
			    vector_set (matchvec, desc);
			}
		    }
	      }
	  }
      }
  vector_free (cmd_vector);

  if (vector_slot (matchvec, 0) == NULL)
    {
      vector_free (matchvec);
      *status = CMD_ERR_NO_MATCH;
      return NULL;
    }

  *status = CMD_SUCCESS;
  return matchvec;
}

vector
cmd_describe_command (vector vline, struct vty *vty, int *status)
{
  vector ret;

  if ( cmd_try_do_shortcut(vty->node, vector_slot(vline, 0) ) )
    {
      enum node_type onode;
      vector shifted_vline;
      unsigned int index;

      onode = vty->node;
      vty->node = ENABLE_NODE;
      /* We can try it on enable node, cos' the vty is authenticated */

      shifted_vline = vector_init (vector_count(vline));
      /* use memcpy? */
      for (index = 1; index < vector_active (vline); index++)
	{
	  vector_set_index (shifted_vline, index-1, vector_lookup(vline, index));
	}

      ret = cmd_describe_command_real (shifted_vline, vty, status);

      vector_free(shifted_vline);
      vty->node = onode;
      return ret;
  }


  return cmd_describe_command_real (vline, vty, status);
}


/* Check LCD of matched command. */
static int
cmd_lcd (char **matched)
{
  int i;
  int j;
  int lcd = -1;
  char *s1, *s2;
  char c1, c2;

  if (matched[0] == NULL || matched[1] == NULL)
    return 0;

  for (i = 1; matched[i] != NULL; i++)
    {
      s1 = matched[i - 1];
      s2 = matched[i];

      for (j = 0; (c1 = s1[j]) && (c2 = s2[j]); j++)
	if (c1 != c2)
	  break;

      if (lcd < 0)
	lcd = j;
      else
	{
	  if (lcd > j)
	    lcd = j;
	}
    }
  return lcd;
}

/* Command line completion support. */
static char **
cmd_complete_command_real (vector vline, struct vty *vty, int *status)
{
  unsigned int i;
  vector cmd_vector = vector_copy (cmd_node_vector (cmdvec, vty->node));
#define INIT_MATCHVEC_SIZE 10
  vector matchvec;
  struct cmd_element *cmd_element;
  unsigned int index;
  char **match_str;
  struct desc *desc;
  vector descvec;
  char *command;
  int lcd;

  if (vector_active (vline) == 0)
    {
      vector_free (cmd_vector);
      *status = CMD_ERR_NO_MATCH;
      return NULL;
    }
  else
    index = vector_active (vline) - 1;

  /* First, filter by preceeding command string */
  for (i = 0; i < index; i++)
    if ((command = vector_slot (vline, i)))
      {
	enum match_type match;
	int ret;

	/* First try completion match, if there is exactly match return 1 */
	match = cmd_filter_by_completion (command, cmd_vector, i);

	/* If there is exact match then filter ambiguous match else check
	   ambiguousness. */
	if ((ret = is_cmd_ambiguous (command, cmd_vector, i, match)) == 1)
	  {
	    vector_free (cmd_vector);
	    *status = CMD_ERR_AMBIGUOUS;
	    return NULL;
	  }
	/*
	   else if (ret == 2)
	   {
	   vector_free (cmd_vector);
	   *status = CMD_ERR_NO_MATCH;
	   return NULL;
	   }
	 */
      }

  /* Prepare match vector. */
  matchvec = vector_init (INIT_MATCHVEC_SIZE);

  /* Now we got into completion */
  for (i = 0; i < vector_active (cmd_vector); i++)
    if ((cmd_element = vector_slot (cmd_vector, i))
         && !(cmd_element->attr == CMD_ATTR_DEPRECATED || cmd_element->attr == CMD_ATTR_HIDDEN))
      {
	const char *string;
	vector strvec = cmd_element->strvec;

	/* Check field length */
	if (index >= vector_active (strvec))
	  vector_slot (cmd_vector, i) = NULL;
	else
	  {
	    unsigned int j;

	    descvec = vector_slot (strvec, index);
	    for (j = 0; j < vector_active (descvec); j++)
	      if ((desc = vector_slot (descvec, j)))
		{
		  if ((string =
		       cmd_entry_function (vector_slot (vline, index),
					   desc->cmd)))
		    if (cmd_unique_string (matchvec, string))
		      vector_set (matchvec, XSTRDUP (MTYPE_TMP, string));
		}
	  }
      }

  /* We don't need cmd_vector any more. */
  vector_free (cmd_vector);

  /* No matched command */
  if (vector_slot (matchvec, 0) == NULL)
    {
      vector_free (matchvec);

      /* In case of 'command \t' pattern.  Do you need '?' command at
         the end of the line. */
      if (vector_slot (vline, index) == '\0')
	*status = CMD_ERR_NOTHING_TODO;
      else
	*status = CMD_ERR_NO_MATCH;
      return NULL;
    }

  /* Only one matched */
  if (vector_slot (matchvec, 1) == NULL)
    {
      match_str = (char **) matchvec->index;
      vector_only_wrapper_free (matchvec);
      *status = CMD_COMPLETE_FULL_MATCH;
      return match_str;
    }
  /* Make it sure last element is NULL. */
  vector_set (matchvec, NULL);

  /* Check LCD of matched strings. */
  if (vector_slot (vline, index) != NULL)
    {
      lcd = cmd_lcd ((char **) matchvec->index);

      if (lcd)
	{
	  int len = strlen (vector_slot (vline, index));

	  if (len < lcd)
	    {
	      char *lcdstr;

	      lcdstr = XMALLOC (MTYPE_STRVEC, lcd + 1);
	      memcpy (lcdstr, matchvec->index[0], lcd);
	      lcdstr[lcd] = '\0';

	      /* match_str = (char **) &lcdstr; */

	      /* Free matchvec. */
	      for (i = 0; i < vector_active (matchvec); i++)
		{
		  if (vector_slot (matchvec, i))
		    XFREE (MTYPE_STRVEC, vector_slot (matchvec, i));
		}
	      vector_free (matchvec);

	      /* Make new matchvec. */
	      matchvec = vector_init (INIT_MATCHVEC_SIZE);
	      vector_set (matchvec, lcdstr);
	      match_str = (char **) matchvec->index;
	      vector_only_wrapper_free (matchvec);

	      *status = CMD_COMPLETE_MATCH;
	      return match_str;
	    }
	}
    }

  match_str = (char **) matchvec->index;
  vector_only_wrapper_free (matchvec);
  *status = CMD_COMPLETE_LIST_MATCH;
  return match_str;
}

char **
cmd_complete_command (vector vline, struct vty *vty, int *status)
{
  char **ret;

  if ( cmd_try_do_shortcut(vty->node, vector_slot(vline, 0) ) )
    {
      enum node_type onode;
      vector shifted_vline;
      unsigned int index;

      onode = vty->node;
      vty->node = ENABLE_NODE;
      /* We can try it on enable node, cos' the vty is authenticated */

      shifted_vline = vector_init (vector_count(vline));
      /* use memcpy? */
      for (index = 1; index < vector_active (vline); index++)
	{
	  vector_set_index (shifted_vline, index-1, vector_lookup(vline, index));
	}

      ret = cmd_complete_command_real (shifted_vline, vty, status);

      vector_free(shifted_vline);
      vty->node = onode;
      return ret;
  }


  return cmd_complete_command_real (vline, vty, status);
}

/* return parent node */
/* MUST eventually converge on CONFIG_NODE */
enum node_type
node_parent ( enum node_type node )
{
  enum node_type ret;

  assert (node > CONFIG_NODE);

  switch (node)
  {
#if 0
    case INTERFACE_POTS_NODE:
    case INTERFACE_RANGE_POTS_NODE:
        ret = VOICE_NODE;
        break;
    case UPLINK_NODE:
    case VOICE_NODE:
    case INTERFACE_ETH_NODE:
    case INTERFACE_PON_NODE:
 //   case INTERFACE_SSR_NODE:
    case INTERFACE_RANGE_ETH_NODE:
    case INTERFACE_RANGE_PON_NODE:
  //  case INTERFACE_RANGE_SSR_NODE:
#endif
    default:
      ret = CONFIG_NODE;
    }

  return ret;
}

/* Execute command by argument vline vector. */
static int
cmd_execute_command_real (vector vline, struct vty *vty)
{
    unsigned int i;
    unsigned int index;
    vector cmd_vector;
    struct cmd_element *cmd_element;
    struct cmd_element *matched_element;
    unsigned int matched_count, incomplete_count;
    int argc;
    char *argv[CMD_ARGC_MAX];
    enum match_type match = 0;
    int varflag;
    char *command;
    int j;

    /* Make copy of command elements. */
    cmd_vector = vector_copy (cmd_node_vector (cmdvec, vty->node));

    for (index = 0; index < vector_active (vline); index++)
    {
        if ((command = vector_slot (vline, index)))
        {
            int ret;

            match = cmd_filter_by_completion (command, cmd_vector, index);

            if (match == vararg_match)
                break;

            ret = is_cmd_ambiguous (command, cmd_vector, index, match);

            if (ret == 1)
            {
                vector_free (cmd_vector);
                return CMD_ERR_AMBIGUOUS;
            }
            else if (ret == 2)
            {
                vector_free (cmd_vector);
                return CMD_ERR_NO_MATCH;
            }
        }
    }

    /* Check matched count. */
    matched_element = NULL;
    matched_count = 0;
    incomplete_count = 0;

    for (i = 0; i < vector_active (cmd_vector); i++)
    {
        if ((cmd_element = vector_slot (cmd_vector, i)))
        {
            if (match == vararg_match || index >= cmd_element->cmdsize)
            {
                matched_element = cmd_element;
#if 0
                printf ("DEBUG: %s\n", cmd_element->string);
#endif
                matched_count++;
            }
            else
            {
                incomplete_count++;
            }
        }
    }

    /* Finish of using cmd_vector. */
    vector_free (cmd_vector);

    /* To execute command, matched_count must be 1. */
    if (matched_count == 0)
    {
        if (incomplete_count)
            return CMD_ERR_INCOMPLETE;
        else
            return CMD_ERR_NO_MATCH;
    }

    if (matched_count > 1)
        return CMD_ERR_AMBIGUOUS;

    if (vty->iPriority < matched_element->level)
    {
        return CMD_ERR_PRIORITY_LOWER;
    }

    /* Argument treatment */
    varflag = 0;
    argc = 0;

    for (i = 0; i < vector_active (vline); i++)
    {
        if (varflag)
            argv[argc++] = vector_slot (vline, i);
        else
        {
            vector descvec = vector_slot (matched_element->strvec, i);

            if (vector_active (descvec) == 1)
            {
                struct desc *desc = vector_slot (descvec, 0);

                if (CMD_VARARG (desc->cmd))
                    varflag = 1;

                if (varflag || CMD_VARIABLE (desc->cmd))
                {
                    argv[argc++] = vector_slot (vline, i);
                }
                else if (CMD_OPTION (desc->cmd) || CMD_MULTI(desc->cmd))
                {
                    argv[argc++] = vector_slot (vline, i);
                    char *pc_tmp = &desc->cmd[1];

                    if (CMD_OPTION(desc->cmd) && CMD_VARIABLE(pc_tmp))
                    {
                        continue;
                    }
                    j = 0;
                    while ( argv[argc-1][j] )
                    {
                        argv[argc-1][j] = tolower(argv[argc-1][j]);
                        j++;
                    }
                }
            }
            else
            {
                argv[argc++] = vector_slot (vline, i);
                j = 0;
                while ( argv[argc-1][j] )
                {
                    argv[argc-1][j] = tolower(argv[argc-1][j]);
                    j++;
                }
            }
        }

        if (argc >= CMD_ARGC_MAX)
        	return CMD_ERR_EXEED_ARGC_MAX;
    }

    /* Execute matched command. */
    return (*matched_element->func)(matched_element, vty, argc, (const char**) argv);
}

int
cmd_execute_command (vector vline, struct vty *vty, int vtysh)
{
  int ret, saved_ret, tried = 0;
  enum node_type onode, try_node;

  onode = try_node = vty->node;

  if ( cmd_try_do_shortcut(vty->node, vector_slot(vline, 0) ) )
    {
      vector shifted_vline;
      unsigned int index;

      vty->node = ENABLE_NODE;
      /* We can try it on enable node, cos' the vty is authenticated */

      shifted_vline = vector_init (vector_count(vline));
      /* use memcpy? */
      for (index = 1; index < vector_active (vline); index++)
	{
	  vector_set_index (shifted_vline, index-1, vector_lookup(vline, index));
	}

      ret = cmd_execute_command_real (shifted_vline, vty);

      vector_free(shifted_vline);
      vty->node = onode;
      return ret;
  }


  saved_ret = ret = cmd_execute_command_real (vline, vty);

  if (vtysh)
    return saved_ret;

  /* This assumes all nodes above CONFIG_NODE are childs of CONFIG_NODE */
  while ( ret != CMD_SUCCESS && ret != CMD_WARNING
	  && vty->node > CONFIG_NODE )
    {
      try_node = node_parent(try_node);
      vty->node = try_node;
      ret = cmd_execute_command_real (vline, vty);
      tried = 1;
      if (ret == CMD_SUCCESS || ret == CMD_WARNING)
	{
	    if (try_node == vty->node)
	        vty->node = onode;
	  /* succesfull command, leave the node as is */
	  return ret;
	}
    }
  /* no command succeeded, reset the vty to the original node and
     return the error for this node */
  if ( tried )
    vty->node = onode;
  return saved_ret;
}

/* Configration from terminal */
DEFUN (config_terminal,
       config_terminal_cmd,
       "config",
       "Configuration from terminal interface\n""从终端配置设备\n",
       CMD_LEVEL_VISITOR_0)
{
  vty->node = CONFIG_NODE;
  return CMD_SUCCESS;
}

/* Enable command */
DEFUN (enable,
       config_enable_cmd,
       "enable",
       "Turn on privileged mode command\n""进入特权用户模式\n",
       CMD_LEVEL_VISITOR_0)
{
    int iError = CMD_SUCCESS;
    extern int shell_enable(struct vty *pstVty);

    vty->node = AUTH_ENABLE_NODE;

    if ( vty->type == VTY_SHELL)
    {
        /* shell */
        iError = shell_enable(vty);

        if (CMD_SUCCESS == iError)
        {
            vty->node = ENABLE_NODE;
        }
        else
        {
            vty->node = VIEW_NODE;
            VTY_OUT_EC(vty, "%spassword error!", "%s密码无效!", VTY_NEWLINE);
        }
		vty_out(vty, VTY_NEWLINE);
    }

    return iError;
}

/* Disable command */
DEFUN (disable,
       config_disable_cmd,
       "disable",
       "Turn off privileged mode command\n""退出特权用户模式\n",
       CMD_LEVEL_VISITOR_0)
{
  if (vty->node == ENABLE_NODE)
    vty->node = VIEW_NODE;
  return CMD_SUCCESS;
}

/* Down vty node level. */
DEFUN (config_exit,
       config_exit_cmd,
       "exit",
       "Exit current mode and down to previous mode\n""当前模式返回上级模式\n",
       CMD_LEVEL_VISITOR_0)
{
    switch (vty->node)
    {
        case VIEW_NODE:
        {
            if (vty_shell (vty))
            {
                vty->node = USER_NODE;
                //release_user(vty->user_index, 1);
            }
            else
            {
                vty->status = VTY_CLOSE;
            }
            break;
        }

        case ENABLE_NODE:
        {
            vty->node = VIEW_NODE;
            break;
        }

        case CONFIG_NODE:
            vty->node = ENABLE_NODE;
            //vty_config_unlock (vty);
            break;
        case DEBUG_NODE:
            vty->node = ENABLE_NODE;
            break;
        default:
            break;
    }

    return CMD_SUCCESS;
}

/* quit is alias of exit. */
ALIAS (config_exit,
       config_quit_cmd,
       "quit",
       "Exit current mode and down to previous mode\n""当前模式返回上级模式\n",
       CMD_LEVEL_VISITOR_0)

/* End of configuration. */
DEFUN (config_end,
       config_end_cmd,
       "end",
       "End current mode and change to enable mode\n""当前模式返回特权模式\n",
       CMD_LEVEL_VISITOR_0)
{
  switch (vty->node)
    {
    case VIEW_NODE:
    case ENABLE_NODE:
      /* Nothing to do. */
      break;
    case DEBUG_NODE:
        vty->node = ENABLE_NODE;
        break;
    case CONFIG_NODE:
      vty->node = ENABLE_NODE;
      break;
    default:
      break;
    }
  return CMD_SUCCESS;
}

DEFUN (history_fun,
       history_cmd,
       "history",
       "Most recent history command\n""本次登录最近输入过的命令\n",
       CMD_LEVEL_VISITOR_0)
{
    int index;
    HISTORY_STATE *pstHistState;
    HIST_ENTRY *pstHistPos;

    if (vty_shell(vty))
    {
        pstHistState = history_get_history_state();
        for (index=0; index<pstHistState->length-1; ++index)
        {
            pstHistPos = pstHistState->entries[index];
            vty_out (vty, "  %s%s", pstHistPos->line, VTY_NEWLINE);
        }
        free(pstHistState);
    }
    else
    {
        for (index = vty->hindex + 1; index != vty->hindex;)
        {
            if (index == VTY_MAXHIST)
            {
                index = 0;
                continue;
            }

            if (vty->hist[index] != NULL)
            {
                vty_out (vty, "  %s%s", vty->hist[index], VTY_NEWLINE);
            }

            index++;
        }
    }

    return CMD_SUCCESS;
}

DEFUN (time_out_fun,
       time_out_cmd,
       "terminal time-out <1-65535>",
       "Configure terminal\n""配置终端\n"
       "Time-out information\n""超时信息\n"
       "Time-out in seconds\n""以秒为单位的超时时间\n",
       CMD_LEVEL_MONITOR_5)
{
    vty->v_timeout = atoi(argv[0]);

    if (vty_shell (vty))
    {
        rl_set_keyboard_input_timeout(vty->v_timeout);
    }
    return CMD_SUCCESS;
}

DEFUN (clear_fun,
       clear_cmd,
       "clear",
       "Clear screen\n""清屏\n",
       CMD_LEVEL_VISITOR_0)
{
    /* 清屏命令串 */
    char clscode[] = {0x1B, 0x5B, 0x48, 0x1B, 0x5B, 0x4A, 0};

    vty_out(vty, "%s", clscode);

    return CMD_SUCCESS;
}

/* Help display function for all node. */
DEFUN (config_help,
       config_help_cmd,
       "help",
       "Description of the interactive help system\n""帮助信息",
       CMD_LEVEL_VISITOR_0)
{
    if (CMD_HELP_LANG_ENGLISH == vty->language)
  vty_out (vty,
	   "ROAP software provides advanced help. Anytime when you need help,%s\
please press '?' at the command line.%s\
%s\
If nothing matches, the help list will be empty. You must backspace%s\
your command until the available options are shown when entering%s\
a '?'.%s\
Two styles of help are provided by ROAP:%s\
1. Full help is available when you are ready to enter a%s\
command argument (e.g. 'show ?') and describes each possible%s\
argument.%s\
2. Partial help is provided when an abbreviated argument is entered%s\
   and you want to know what arguments match the input%s\
   (e.g. 'show me?'.)%s%s", VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE,
	   VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE,
	   VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE);
    else if (CMD_HELP_LANG_CHINESE== vty->language)
        vty_out (vty,
	   "ROAP 软件提供了先进的帮助特性。任何当您需要帮助的时候，请在命令行%s\
中按下'?'。%s\
%s\
如果没有匹配的帮助信息，帮助列表将显示为空。这时候您需要调整输出%s\
直到再按下一个 '?'时帮助列表中有信息显示。%s\
%s\
ROAP软件提供了两种形式的帮助信息:%s\
1. 当您希望获得某个命令所有可能的参数以及这些参数的描述信息的时候，%s\
请输入该命令，并在空格后按下'?'键。例如通过'show ?'您将获得show命%s\
令的所有后续参数列表。%s\
%s\
2. 当您希望得到某个命令以某几个字符开头的所有参数信息的时候，请在%s\
输入该命令和相应字符后紧接着按下'?'键。 例如'show m?'将列出show命%s\
令中所有以m开头的后续参数。%s%s", VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE,
	   VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE,
	   VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE, VTY_NEWLINE,
	   VTY_NEWLINE, VTY_NEWLINE);
  return CMD_SUCCESS;
}
//第一次进入debug模式时，进行命令注册。后面不再进行注册。
int g_debug;
extern void initLogModules();
extern void finiLogModules();
extern void add_log_enable();

DEFUN_HIDDEN (debug,
	      debug_cmd,
	      "debug-hide 0906",
	      "\n""\n",
	      CMD_LEVEL_ADMINISTRATOR_15)
{
    if (vty->node == ENABLE_NODE)
    {
        vty->node = DEBUG_NODE;
        if (0 == g_debug)
        {
            g_debug = 1;
            //install_dbg_cmd();
        }
    }
    return CMD_SUCCESS;
}

/* Help display function for all node. */
DEFUN (config_list,
       config_list_cmd,
       "list",
       "Print command list\n""显示命令列表\n",
       CMD_LEVEL_MONITOR_5)
{
  unsigned int i;
  struct cmd_node *cnode = vector_slot (cmdvec, vty->node);
  struct cmd_element *cmd;

  for (i = 0; i < vector_active (cnode->cmd_vector); i++)
    if ((cmd = vector_slot (cnode->cmd_vector, i)) != NULL
        && !(cmd->attr == CMD_ATTR_DEPRECATED
             || cmd->attr == CMD_ATTR_HIDDEN))
      vty_out (vty, "  %s%s", cmd->string,
	       VTY_NEWLINE);
  return CMD_SUCCESS;
}

DEFUN (language_change,
       language_change_cmd,
       "language (english|chinese)",
       "Language of help message\n""帮助信息显示语言\n"
       "Help message in english\n""英文显示帮助信息\n"
       "Help message in chinese\n""中文显示帮助信息\n",
       CMD_LEVEL_VISITOR_0)
{
  if ('e' == argv[0][0])
    vty->language = CMD_HELP_LANG_ENGLISH;
  else
    vty->language = CMD_HELP_LANG_CHINESE;

  return CMD_SUCCESS;
}



DEFUN (deb_exit_fun,
       deb_exit_cmd,
       "_exit",
       "Exit the command line process\n退出命令行进程\n",
       CMD_LEVEL_ADMINISTRATOR_15)
{
    if (vty->type == VTY_SHELL)
	{
	    exit(0);
	}
	else
	{
		VTY_OUT_EC(vty,"Telnet does not support this command.%s",
						"Telnet 不支持此命令.%s", VTY_NEWLINE);
		return 0;
	}
}


void install_default (enum node_type node)
{
    install_element (node, &config_exit_cmd);
    install_element (node, &config_quit_cmd);
    install_element (node, &history_cmd);
    install_element (node, &clear_cmd);
    if (ENABLE_NODE != node)
    {
        install_element (node, &config_end_cmd);
    }
    install_element (node, &config_list_cmd);
    install_element (node, &config_help_cmd);
    install_element (node, &language_change_cmd);

    return;
}

void install_view_cmd()
{
	install_node(&view_node, NULL);
    install_element(VIEW_NODE, &config_list_cmd);
    install_element(VIEW_NODE, &config_exit_cmd);
    install_element(VIEW_NODE, &config_quit_cmd);
    install_element(VIEW_NODE, &config_help_cmd);
    install_element(VIEW_NODE, &history_cmd);
    install_element(VIEW_NODE, &clear_cmd);
    install_element(VIEW_NODE, &time_out_cmd);
    install_element(VIEW_NODE, &config_enable_cmd);
    install_element(VIEW_NODE, &language_change_cmd);
    return;
}

void install_enable_cmd()
{
    install_node(&enable_node, NULL);
    install_default(ENABLE_NODE);
    install_element(ENABLE_NODE, &config_disable_cmd);
    install_element(ENABLE_NODE, &config_terminal_cmd);
    install_element(ENABLE_NODE, &debug_cmd);

    return;
}

void install_config_cmd()
{
    unsigned long pon_num = 2;
    unsigned long eth_port = 4;
    install_node(&config_node, NULL);
    install_default(CONFIG_NODE); 
    return;
}

void install_debug_cmd()
{
    install_node(&debug_node, NULL);
    install_default(DEBUG_NODE);
    install_element (DEBUG_NODE, &deb_exit_cmd);

    return;
}



extern void install_user_mng_cmd();

/* Initialize command interface. Install basic nodes and commands. */
void cmd_init ()
{
	uint32_t error_code;
    uint32_t  uiNameLen = HOST_NAME_LEN+1;
    int try_cnt = 0;
    command_cr = XSTRDUP(MTYPE_STRVEC, "<cr>");
    deoam_cr.cmd = command_cr;
    deoam_cr.str[0] = XSTRDUP(MTYPE_STRVEC, "");

    /* Allocate initial top vector of commands. */
    cmdvec = vector_init (VECTOR_MIN_SIZE);

    /* Default host value settings. */
    host.motd = default_motd;
    host.name = HOSTNAME_DEF;

    /* Install top nodes. */
    /* 登陆时输入用户名和密码，进入enable视图输入密码的视图 */
    install_node(&user_node, NULL);
    install_node(&auth_node, NULL);
    install_node(&auth_enable_node, NULL);

    install_node(&change_pass_node, NULL);
    install_node(&change_pass_again_node, NULL);

    install_view_cmd();
    install_enable_cmd();
    install_config_cmd();

    install_dev_manage_cmd();
    install_user_mng_cmd();
    install_vam_manage_cmd();
    
    return;
}

void cmd_terminate ()
{
  unsigned int i, j, k, l;
  struct cmd_node *cmd_node;
  struct cmd_element *cmd_element;
  struct desc *desc;
  vector cmd_node_v, cmd_element_v, deoam_v;

  if (cmdvec)
    {
      for (i = 0; i < vector_active (cmdvec); i++)
        if ((cmd_node = vector_slot (cmdvec, i)) != NULL)
          {
            cmd_node_v = cmd_node->cmd_vector;

            for (j = 0; j < vector_active (cmd_node_v); j++)
              if ((cmd_element = vector_slot (cmd_node_v, j)) != NULL &&
                  cmd_element->strvec != NULL)
                {
                  cmd_element_v = cmd_element->strvec;

                  for (k = 0; k < vector_active (cmd_element_v); k++)
                    if ((deoam_v = vector_slot (cmd_element_v, k)) != NULL)
                      {
                        for (l = 0; l < vector_active (deoam_v); l++)
                          if ((desc = vector_slot (deoam_v, l)) != NULL)
                            {
                              if (desc->cmd)
                                XFREE (MTYPE_STRVEC, desc->cmd);
                              for (i = 0; i < CMD_HELP_LANG_NUM; i++)
                              {
                                if (desc->str[i])
                                    XFREE (MTYPE_STRVEC, desc->str[i]);
                              }

                              XFREE (MTYPE_DESC, desc);
                            }
                        vector_free (deoam_v);
                      }

                  cmd_element->strvec = NULL;
                  vector_free (cmd_element_v);
                }

            vector_free (cmd_node_v);
          }

      vector_free (cmdvec);
      cmdvec = NULL;
    }

  if (command_cr)
    XFREE(MTYPE_STRVEC, command_cr);
  if (deoam_cr.str[0])
    XFREE(MTYPE_STRVEC, deoam_cr.str[0]);
  if (host.name)
    XFREE (MTYPE_HOST, host.name);
}
