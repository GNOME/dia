/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>

#include <libxml/xmlerror.h>

#include "libdia.h"
#include "message.h"
#include "utils.h"
#include "dia_image.h"
#include "color.h"
#include "object.h"

static void
stderr_message_internal(const char *title, enum ShowAgainStyle showAgain,
			const char *fmt, va_list *args,  va_list *args2)
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
          "%s: %s\n", 
          title,buf);
}

#ifdef G_OS_WIN32
static void
myXmlErrorReporting (void *ctx, const char* msg, ...)
{
  va_list args;
  gchar *string;

  va_start(args, msg);
  string = g_strdup_vprintf (msg, args);
  g_print ("%s", string ? string : "xml error (null)?");
  va_end(args);

  g_free(string);
}
#endif

/**
 * Basic (i.e. minimal) initialization of libdia. 
 *
 * It does not load any plug-ins but instead brings libdia to a state that plug-in loading can take place.
 * @param flags a set of DIA_INTERACTIVE, DIA_MESSAGE_STDERR
 */
void
libdia_init (guint flags)
{
  static gboolean initialized = FALSE;
  
  if (initialized)
    return;

  if (flags & DIA_MESSAGE_STDERR)
    set_message_func(stderr_message_internal);    
  LIBXML_TEST_VERSION;

#ifdef G_OS_WIN32
  xmlSetGenericErrorFunc(NULL, myXmlErrorReporting);
#endif

  stdprops_init();

  if (flags & DIA_INTERACTIVE) {

    dia_image_init();

    gdk_rgb_init();

    gtk_rc_parse("diagtkrc");

    color_init();
  }
  initialized = TRUE;

  object_registry_init();
}
