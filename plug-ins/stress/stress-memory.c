/*
 * stress-memory.c -- functions to play around with virtual memory
 *
 * Copyright (C) 2011, Hans Breuer <hans@breuer.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* NOTHING Dia specific HERE */
#include "stress-memory.h"

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

gboolean
vmem_avail (guint64 *size)
{
  guint64 avail = 0;
  gpointer p = NULL;
#ifdef G_OS_WIN32
  MEMORY_BASIC_INFORMATION mbi;

  while (VirtualQuery (p, &mbi, sizeof(mbi))) {
    if (MEM_FREE == mbi.State)
      avail += mbi.RegionSize;
    p = ((char*)mbi.BaseAddress + mbi.RegionSize);
  }
#else
  return FALSE;
#endif
  if (size)
    *size = avail;
  return (avail > 0);
}

static GHashTable *virtual_reserved = NULL;

gboolean
vmem_reserve (guint64 size)
{
  if (!virtual_reserved)
    virtual_reserved = g_hash_table_new (g_direct_hash, g_direct_equal);

#ifdef G_OS_WIN32
  {
    /* scan entire process for contiguous blocks of memory */
    gpointer p = NULL;
    gsize maxbs = 0, bs; /* maximum block size */ 
    guint64 size_to_eat = size;
    guint64 page_size;
    MEMORY_BASIC_INFORMATION mbi;
    SYSTEM_INFO si;
    
    GetSystemInfo (&si);
    page_size = si.dwPageSize;

    while (VirtualQuery (p, &mbi, sizeof(mbi))) {
      if (MEM_FREE == mbi.State)
        if (mbi.RegionSize > maxbs)
          maxbs = mbi.RegionSize;
      p = ((char*)mbi.BaseAddress + mbi.RegionSize);
    }
    if (maxbs < page_size)
      return TRUE;
    bs = maxbs;
    while (size_to_eat >= page_size) {
      /* start with the largest free block */
      if (size_to_eat > bs) { /* reserve but MEM_COMMIT */
        p = VirtualAlloc(NULL, bs, MEM_RESERVE, PAGE_READWRITE);
	if (p) {
	  g_hash_table_insert (virtual_reserved, p, (gpointer)bs);
	  size_to_eat -= bs;
	} else {
	  /* reduce block size to request as a whole */
	  if (bs <= page_size)
	    break;
	  bs -= page_size;
	}
      } else {
        /* find a convenient bs to start reserving */
	while (size_to_eat < bs && bs > page_size)
	  bs = bs >> 1;
	if (bs < page_size)
	  break;
      }
      /* the while loop is ignoring extra memory used for the hash */
    }
    /* done anything */
    return (size_to_eat < size);
  }
#else
  return FALSE;
#endif
}

static void
_release_one (gpointer key,
              gpointer value,
              gpointer user_data)
{
#ifdef G_OS_WIN32
  if (key)
    VirtualFree(key, 0, MEM_RELEASE);
#else
  g_print ("release_one?");
#endif
}

void
vmem_release (void)
{
  if (virtual_reserved) {
    g_hash_table_foreach (virtual_reserved, _release_one, NULL);
    g_hash_table_destroy (virtual_reserved);
    virtual_reserved = NULL;
  }
}

