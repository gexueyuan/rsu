/* Socket union related function.
 * Copyright (c) 1997, 98 Kunihiro Ishiguro
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

#include "vty.h"
#include "sockunion.h"
#include "memory.h"

const char *
inet_sutop (union sockunion *su, char *str)
{
  switch (su->sa.sa_family)
    {
    case AF_INET:
      inet_ntop (AF_INET, &su->sin.sin_addr, str, INET_ADDRSTRLEN);
      break;
#ifdef HAVE_IPV6
    case AF_INET6:
      inet_ntop (AF_INET6, &su->sin6.sin6_addr, str, INET6_ADDRSTRLEN);
      break;
#endif /* HAVE_IPV6 */
    }
  return str;
}

int
str2sockunion (const char *str, union sockunion *su)
{
  int ret;

  memset (su, 0, sizeof (union sockunion));

  ret = inet_pton (AF_INET, str, &su->sin.sin_addr);
  if (ret > 0)			/* Valid IPv4 address format. */
    {
      su->sin.sin_family = AF_INET;
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
      su->sin.sin_len = sizeof(struct sockaddr_in);
#endif /* HAVE_STRUCT_SOCKADDR_IN_SIN_LEN */
      return 0;
    }
#ifdef HAVE_IPV6
  ret = inet_pton (AF_INET6, str, &su->sin6.sin6_addr);
  if (ret > 0)			/* Valid IPv6 address format. */
    {
      su->sin6.sin6_family = AF_INET6;
#ifdef SIN6_LEN
      su->sin6.sin6_len = sizeof(struct sockaddr_in6);
#endif /* SIN6_LEN */
      return 0;
    }
#endif /* HAVE_IPV6 */
  return -1;
}

const char *
sockunion2str (union sockunion *su, char *buf, size_t len)
{
  if  (su->sa.sa_family == AF_INET)
    return inet_ntop (AF_INET, &su->sin.sin_addr, buf, len);
#ifdef HAVE_IPV6
  else if (su->sa.sa_family == AF_INET6)
    return inet_ntop (AF_INET6, &su->sin6.sin6_addr, buf, len);
#endif /* HAVE_IPV6 */
  return NULL;
}

char *
sockunion_su2str (union sockunion *su)
{
  char str[SU_ADDRSTRLEN];

  switch (su->sa.sa_family)
    {
    case AF_INET:
      inet_ntop (AF_INET, &su->sin.sin_addr, str, sizeof (str));
      break;
#ifdef HAVE_IPV6
    case AF_INET6:
      inet_ntop (AF_INET6, &su->sin6.sin6_addr, str, sizeof (str));
      break;
#endif /* HAVE_IPV6 */
    }
  return XSTRDUP (MTYPE_TMP, str);
}

/* Convert IPv4 compatible IPv6 address to IPv4 address. */
static void
sockunion_normalise_mapped (union sockunion *su)
{
#ifdef HAVE_IPV6
    struct sockaddr_in sin;

    if (su->sa.sa_family == AF_INET6 
        && IN6_IS_ADDR_V4MAPPED (&su->sin6.sin6_addr))
    {
        memset (&sin, 0, sizeof (struct sockaddr_in));
        sin.sin_family = AF_INET;
        sin.sin_port = su->sin6.sin6_port;
        memcpy (&sin.sin_addr, ((char *)&su->sin6.sin6_addr) + 12, 4);
        memcpy (su, &sin, sizeof (struct sockaddr_in));
    }
#endif /* HAVE_IPV6 */
}

/* Return socket of sockunion. */
int
sockunion_socket (union sockunion *su)
{
  int sock;

  sock = socket (su->sa.sa_family, SOCK_STREAM, 0);
  if (sock < 0)
    {
      printf("Can't make socket : %s\n", safe_strerror (errno));
      return -1;
    }

  return sock;
}

/* Return accepted new socket file descriptor. */
int
sockunion_accept (int sock, union sockunion *su)
{
    socklen_t len;
    int client_sock;

    len = sizeof (union sockunion);
    client_sock = accept (sock, (struct sockaddr *) su, &len);

    sockunion_normalise_mapped (su);
    return client_sock;
}

/* Make socket from sockunion union. */
int
sockunion_stream_socket (union sockunion *su)
{
    int sock;

    if (su->sa.sa_family == 0)
        su->sa.sa_family = AF_INET_UNION;

    sock = socket (su->sa.sa_family, SOCK_STREAM, 0);

    if (sock < 0)
        printf("can't make socket sockunion_stream_socket\n");

    return sock;
}

/* Bind socket to specified address. */
int
sockunion_bind (int sock, union sockunion *su, unsigned short port, 
		union sockunion *su_addr)
{
    int size = 0;
    int ret;

    if (su->sa.sa_family == AF_INET)
    {
        size = sizeof (struct sockaddr_in);
        su->sin.sin_port = htons (port);
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
        su->sin.sin_len = size;
#endif /* HAVE_STRUCT_SOCKADDR_IN_SIN_LEN */
        if (su_addr == NULL)
            su->sin.sin_addr.s_addr = htonl (INADDR_ANY);
    }
#ifdef HAVE_IPV6
    else if (su->sa.sa_family == AF_INET6)
    {
        size = sizeof (struct sockaddr_in6);
        su->sin6.sin6_port = htons (port);
#ifdef SIN6_LEN
        su->sin6.sin6_len = size;
#endif /* SIN6_LEN */
        if (su_addr == NULL)
        {
#if defined(LINUX_IPV6) || defined(NRL)
            memset (&su->sin6.sin6_addr, 0, sizeof (struct in6_addr));
#else
            su->sin6.sin6_addr = in6addr_any;
#endif /* LINUX_IPV6 */
        }
    }
#endif /* HAVE_IPV6 */


    ret = bind (sock, (struct sockaddr *)su, size);
    if (ret < 0)
        printf("can't bind socket : %s\n", safe_strerror (errno));

    return ret;
}

int
sockopt_reuseaddr (int sock)
{
    int ret;
    int on = 1;

    ret = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(on));
    if (ret < 0)
    {
        printf("can't set sockopt SO_REUSEADDR to socket %d\n", sock);
        return -1;
    }
    return 0;
}

#ifdef SO_REUSEPORT
int
sockopt_reuseport (int sock)
{
    int ret;
    int on = 1;

    ret = setsockopt (sock, SOL_SOCKET, SO_REUSEPORT, (void *)&on, sizeof(on));
    if (ret < 0)
    {
        printf("can't set sockopt SO_REUSEPORT to socket %d\n", sock);
        return -1;
    }
    return 0;
}
#else
int
sockopt_reuseport (int sock)
{
  return 0;
}
#endif /* 0 */

