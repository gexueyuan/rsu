/*
 * Memory management routine
 * Copyright (C) 1998 Kunihiro Ishiguro
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
/* malloc.h is generally obsolete, however GNU Libc mallinfo wants it. */
#if !defined(HAVE_STDLIB_H) || (defined(GNU_LINUX) && defined(HAVE_MALLINFO))
#include <malloc.h>
#endif /* !HAVE_STDLIB_H || HAVE_MALLINFO */

#include "memory.h"

static void alloc_inc (int);
static void alloc_dec (int);

/* Fatal memory allocation error occured. */
static void __attribute__ ((noreturn))
zerror (const char *fname, int type, size_t size)
{
  printf("%s : can't allocate memory for size %d: %s\n", 
	    fname, (int)size, safe_strerror(errno));

  abort();
}

/* Memory allocation. */
void *
zmalloc (int type, size_t size)
{
  void *memory;

  memory = malloc (size);

  if (memory == NULL)
    zerror ("malloc", type, size);

  alloc_inc (type);

  return memory;
}

/* Memory allocation with num * size with cleared. */
void *
zcalloc (int type, size_t size)
{
  void *memory;

  memory = calloc (1, size);

  if (memory == NULL)
    zerror ("calloc", type, size);

  alloc_inc (type);

  return memory;
}

/* Memory reallocation. */
void *
zrealloc (int type, void *ptr, size_t size)
{
  void *memory;

  memory = realloc (ptr, size);
  if (memory == NULL)
    zerror ("realloc", type, size);
  return memory;
}

/* Memory free. */
void
zfree (int type, void *ptr)
{
  alloc_dec (type);
  free (ptr);
}

/* String duplication. */
char *
zstrdup (int type, const char *str)
{
  void *dup;

  dup = strdup (str);
  if (dup == NULL)
    zerror ("strdup", type, strlen (str));
  alloc_inc (type);
  return dup;
}

static struct 
{
  char *name;
  long alloc;
} mstat [MTYPE_MAX];

/* Increment allocation counter. */
static void
alloc_inc (int type)
{
  mstat[type].alloc++;
}

/* Decrement allocation counter. */
static void
alloc_dec (int type)
{
  mstat[type].alloc--;
}

