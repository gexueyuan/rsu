/* Zebra common header.
   Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002 Kunihiro Ishiguro

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

#ifndef _ZEBRA_H
#define _ZEBRA_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <assert.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <stdarg.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <arpa/inet.h>
#ifdef BSDI_NRL
#ifdef HAVE_NETINET6_IN6_H
#include <netinet6/in6.h>
#endif /* HAVE_NETINET6_IN6_H */
#ifdef NRL
#include <netinet6/in6.h>
#endif /* NRL */
#endif /* BSDI_NRL */

#include "cv_osal.h"

#if !(defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
/* Not C99; do we need to define va_copy? */
#ifndef va_copy
#ifdef __va_copy
#define va_copy(DST,SRC) __va_copy(DST,SRC)
#else
/* Now we are desperate; this should work on many typical platforms. 
   But this is slightly dangerous, because the standard does not require
   va_copy to be a macro. */
#define va_copy(DST,SRC) memcpy(&(DST), &(SRC), sizeof(va_list))
#warning "Not C99 and no va_copy macro available, falling back to memcpy"
#endif /* __va_copy */
#endif /* !va_copy */
#endif /* !C99 */

/* Local includes: */
#if !(defined(__GNUC__) || defined(VTYSH_EXTRACT_PL)) 
#define __attribute__(x)
#endif  /* !__GNUC__ || VTYSH_EXTRACT_PL */

/* MAX / MIN are not commonly defined, but useful */

struct vty_socket_t
{
    char *vty_addr;
    int vty_port;
};

/* Does the I/O error indicate that the operation should be retried later? */
#define ERRNO_IO_RETRY(EN) \
	(((EN) == EAGAIN) || ((EN) == EWOULDBLOCK) || ((EN) == EINTR))

/* Wrapper around strerror to handle case where it returns NULL. */
#define safe_strerror(errnum) \
    ((strerror(errnum) != NULL) ? strerror(errnum) : "Unknown error")

#endif /* _ZEBRA_H */
