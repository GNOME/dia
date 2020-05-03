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

#include "dialib.h"
#include "message.h"
#include "utils.h"
#include "color.h"
#include "object.h"
#include "dia_dirs.h"
#include "properties.h" /* stdprops_init() */
#include "standard-path.h"


G_GNUC_PRINTF(3, 0)
static void
stderr_message_internal (const char          *title,
                         enum ShowAgainStyle  showAgain,
                         const char          *fmt,
                         va_list              args,
                         va_list              args2)
{
  static char *buf = NULL;
  static int   alloc = 0;
  int len;

  len = g_printf_string_upper_bound (fmt, args);

  if (len >= alloc) {
    g_clear_pointer (&buf, g_free);

    alloc = nearest_pow (MAX (len + 1, 1024));

    buf = g_new0 (char, alloc);
  }

  vsprintf (buf, fmt, args2);

  g_printerr ("%s: %s\n", title, buf);
}


#ifdef G_OS_WIN32
static void
myXmlErrorReporting (void *ctx, const char* msg, ...)
{
  va_list args;
  gchar *string;

  va_start(args, msg);
  string = g_strdup_vprintf (msg, args);
  g_printerr ("%s", string ? string : "xml error (null)?");
  va_end(args);

  g_clear_pointer (&string, g_free);
}
#endif


/**
 * libdia_init:
 * @flags: a set of #DiaInitFlags
 *
 * Basic (i.e. minimal) initialization of libdia.
 *
 * It does not load any plug-ins but instead brings libdia to a state that
 * plug-in loading can take place.
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

  if (flags & DIA_VERBOSE) {
    dia_log_message_enable (TRUE);
    dia_log_message ("initializing libdia");
  }
  stdprops_init();

  if (flags & DIA_INTERACTIVE) {
    char *diagtkrc;

    gtk_widget_set_default_colormap(gdk_rgb_get_cmap());

    diagtkrc = dia_config_filename("diagtkrc");
    dia_log_message ("Config from %s", diagtkrc);
    gtk_rc_parse(diagtkrc);
    g_clear_pointer (&diagtkrc, g_free);

    color_init();
  }
  initialized = TRUE;

  object_registry_init();

  /* The group_type is registered in app, but it needs to be exported anyway */
  object_register_type(&stdpath_type);
}

