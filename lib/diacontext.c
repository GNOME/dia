/* dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diacontext.c --
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

#include "config.h"

#include "diacontext.h"

/* DiaContext should be used to
 *  - let the caller decide about user notfication and not the callee
 *  - help writing thread-aware library functions
 *  - cleanup message_*() usage in dia/lib, dia/plug-ins and dia/objects
 */

#include <glib-object.h>
#include <string.h>

#include "message.h" /* just for testing */

struct _DiaContext {
  GObject parent_instance;
  /*< private >*/
  char  *desc;
  char  *filename;
  GStrvBuilder *messages;
};


G_DEFINE_FINAL_TYPE (DiaContext, dia_context, G_TYPE_OBJECT);


static void
dia_context_finalize (GObject *object)
{
  DiaContext *context = DIA_CONTEXT (object);

  g_clear_pointer (&context->desc, g_free);
  g_clear_pointer (&context->filename, g_free);
  g_clear_pointer (&context->messages, g_strv_builder_unref);

  G_OBJECT_CLASS (dia_context_parent_class)->finalize (object);
}


static void
dia_context_class_init (DiaContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dia_context_finalize;
}


static void
dia_context_init (DiaContext *self)
{
  self->messages = g_strv_builder_new ();
}


DiaContext *
dia_context_new (const char *desc)
{
  DiaContext *context = g_object_new (DIA_TYPE_CONTEXT, NULL);
  context->desc = g_strdup (desc);
  return context;
}


void
dia_context_release (DiaContext *context)
{
  /* FIXME: this should vanish */
  if (context->messages) {
    GStrv messages = g_strv_builder_end (context->messages);

    if (messages && g_strv_length (messages) > 0) {
      char *combined = g_strjoinv ("\n", messages);

      message_warning ("%s:\n%s",
                        context->desc ? context->desc : "<no context>",
                        combined);

      g_clear_pointer (&combined, g_free);
    }

    g_clear_pointer (&messages, g_strfreev);
  }

  g_object_unref (context);
}


/*!
 * \brief Clean out the context for further use
 */
void
dia_context_reset (DiaContext *context)
{
  g_strfreev (g_strv_builder_end (context->messages));
  g_clear_pointer (&context->desc, g_free);
  g_clear_pointer (&context->filename, g_free);
}


void
dia_context_set_filename (DiaContext *context,
			  const char *filename)
{
  g_return_if_fail (context != NULL);

  g_clear_pointer (&context->filename, g_free);
  context->filename = g_strdup (filename);
}

/*!
 * \brief Get the filename previously set with dia_context_set_filename()
 *
 * Returns the filename previously set with dia_context_set_filename(). For convenience
 * of the API user a valid string gets returned even if no filename is set.
 *
 * @param  context explicit this pointer
 * @return the filename or "?" instead of NULL
 */
const char *
dia_context_get_filename (DiaContext *context)
{
  g_return_val_if_fail (context != NULL, "");

  return context->filename ? context->filename : "?";
}

void
dia_context_add_message (DiaContext *context,
			 const char *format, ...)
{
  gchar *msg;
  va_list args;

  g_return_if_fail (context != NULL);

  g_return_if_fail (context != NULL);

  va_start (args, format);

  msg = g_strdup_vprintf (format, args);
  va_end (args);
  /* ToDo: dont repeat the same message over and over again, except ... */

  g_strv_builder_add (context->messages, msg);
}

void
dia_context_add_message_with_errno (DiaContext *context, int nr,
				    const char *format, ...)
{
  char *errstr;
  char *msg;
  va_list args;

  g_return_if_fail (context != NULL);

  va_start (args, format);

  msg = g_strdup_vprintf (format, args);
  va_end (args);

  errstr = (nr == 0) ? NULL : g_locale_to_utf8 (strerror(nr), -1, NULL, NULL, NULL);
  if (errstr) {
    char *tmp = g_strdup_printf ("%s\n%s", msg, errstr);

    g_clear_pointer (&msg, g_free);
    msg = tmp;
  }
  /* ToDo: dont repeat the same message over and over again, except ... */

  g_strv_builder_add (context->messages, msg);

  g_clear_pointer (&errstr, g_free);
}
