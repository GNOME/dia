/*
 * C++ implementation of Dia Bindings Helper Functions
 *
 * Copyright (C) 2007, Hans Breuer, <Hans@Breuer.Org>
 *
 * This is free software; you can redistribute it and/or modify
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
#include "dia-extra.h"

#include <glib.h>
#include "message.h"
#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib-object.h>
#include "properties.h"
#include "object.h"
#include "plug-ins.h"

/*
 * copied from ../app/diaconv.c
 */
static void __cdecl
stderr_message_internal(const char *title, const char *fmt,
                        va_list *args,  va_list *args2)
{
  static gchar *buf = NULL;
  static gint   alloc = 0;
  gint len;

  len = format_string_length_upper_bound (fmt, args);

  if (len >= alloc) {
    if (buf)
      g_free (buf);
    
    alloc = nearest_pow (MAX(len + 1, 1024));
    
    buf = g_new (char, alloc);
  }
  
  vsprintf (buf, fmt, *args2);
  
  fprintf(stderr,
          "%s %s: %s\n", 
          "",title,buf);
}

static void
dia_extra_redirect (void)
{
  set_message_func(&stderr_message_internal);
}

/*!
 * 
 *
 * \todo Dia's init function needs to be cleaned. E.g. it would be useful
 * to just initialize the plug-ins needed and not everything. Also this
 * function may be split to be partially called at bindings initialization
 * time.
 */
void dia::register_plugins ()
{
    g_type_init();
    dia_extra_redirect();
    printf ("ATTENTION: crashing may be caused by the other pydia extension picked up here.");
    //FIXME: Dia's init functions need to be cleaned - and made aware of multiple calls ...
    stdprops_init();

    object_registry_init();
    dia_register_plugins ();
}
