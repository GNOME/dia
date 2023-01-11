/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * Custom Objects -- objects defined in XML rather than C.
 * Copyright (C) 1999 James Henstridge.
 *
 * Custom shape loading on demand.
 * Copyright (C) 2007 Hans Breuer.
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

#include <glib/gi18n-lib.h>

#include "shape_info.h"
#include "custom_util.h"
#include <string.h>
#include <stdarg.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <libxml/parser.h>

/*!
 * \defgroup ObjectCustom Custom Shapes
 * \brief Dia plug-in to support custom shapes
 *
 * \ingroup Shapes
 *
 * The cusom shape format is supported by a Dia plug-in, too. Disabling
 * this plug-in will highly reduce the available shape set, but also the
 * start-up time.
 *
 * Instead of parsing the complete shape file at start-up we read only
 * the minimal info required to register the type. This should speed
 * up startup a lot and also spare some memory for shapes never used
 * at runtime.
 *
 * There are so many shapes in Dia that on my computer there is a difference
 * of 16MB vs. 36MB total memory consumption with none vs. all of them loaded.
 *
 * The startup time is significantly reduced by the lazy loading via XML SAX, too.
 */

typedef enum {
  READ_ON   = 0,
  READ_NAME = 1,
  READ_ICON = 2,
  READ_DONE = 3
} eState;

typedef struct _Context Context;
struct _Context {
  ShapeInfo* si;
  eState  state;
};

static void
startElementNs (void *ctx,
                const xmlChar *localname,
                const xmlChar *prefix,
                const xmlChar *URI,
                int nb_namespaces,
                const xmlChar **namespaces,
                int nb_attributes,
                int nb_defaulted,
                const xmlChar **attributes)
{
  Context* context = (Context*)ctx;

  if (READ_DONE == context->state)
    /* no more to do */;
  else if (strncmp ((const char*)localname, "name", 4) == 0)
    context->state = READ_NAME;
  else if (strncmp ((const char*)localname, "icon", 4) == 0)
    context->state = READ_ICON;
  else if (context->si->name != NULL && context->si->icon != NULL)
    context->state = READ_DONE;
  else
    context->state = READ_ON;
}

static void
_characters (void *ctx,
	    const xmlChar *ch,
	    int len)
{
  Context* context = (Context*)ctx;

  if (READ_DONE == context->state)
    /* no more to do */;
  if (READ_NAME == context->state) {
    char *prev = context->si->name;
    if (!prev) {
      context->si->name = g_strndup ((const char*) ch, len);
    } else {
      char *now = g_strndup ((const char*) ch, len);
      context->si->name = g_strconcat (prev, now, NULL);
      g_clear_pointer (&prev, g_free);
      g_clear_pointer (&now, g_free);
    }
  } else if (READ_ICON == context->state) {
    char *prev = context->si->icon;
    if (!prev) {
      context->si->icon = g_strndup ((const char*)ch, len);
    } else {
      char *now = g_strndup ((const char*)ch, len);
      context->si->icon = g_strconcat (prev, now, NULL);
      g_clear_pointer (&prev, g_free);
      g_clear_pointer (&now, g_free);
    }
  }
}

static void
endElementNs (void *ctx,
	      const xmlChar *localname,
	      const xmlChar *prefix,
	      const xmlChar *URI)
{
  Context* context = (Context*)ctx;

  if (READ_DONE == context->state)
    return;

  if (READ_NAME == context->state)
    if (!context->si->name || context->si->name[0] == '\0')
      g_warning ("Shape (%s) missing type name", context->si->filename);
  if (READ_ICON == context->state)
    if (!context->si->icon || context->si->icon[0] == '\0')
      g_warning ("Shape (%s) missing icon name", context->si->filename);
  if (   (READ_NAME  == context->state || READ_ICON == context->state)
      && context->si->name && context->si->icon)
    context->state = READ_DONE;
  else
    context->state = READ_ON;
}

static void _error (void *ctx, const char * msg, ...) G_GNUC_PRINTF(2, 3);

static void
_error (void *ctx,
        const char * msg,
        ...)
{
  Context* context = (Context*)ctx;
  va_list args;

  if (READ_DONE == context->state)
    return; /* we are ready, not interested in further complains */
  va_start(args, msg);
  g_printerr ("Error: %s\n", context->si->filename);
  g_vfprintf (stderr, msg, args);
  g_printerr ("\n");
  va_end(args);
}

static void _warning (void *ctx, const char * msg, ...) G_GNUC_PRINTF(2, 3);

static void
_warning (void *ctx,
          const char * msg,
          ...)
{
  Context* context = (Context*)ctx;
  va_list args;

  if (READ_DONE == context->state)
    return; /* we are ready, not interested in further complains */
  va_start(args, msg);
  g_printerr ("Warning: %s\n", context->si->filename);
  g_vfprintf (stderr, msg, args);
  g_printerr ("\n");
  va_end(args);
}

/*!
 * \brief Partial load of ShapeInfo jsut for speed
 * This functions loads the first BLOCKSSIZE block of a shape file
 * to extract just the type information from it.
 * \extends ShapeInfo
 * \ingroup ObjectCustom
 */
gboolean
shape_typeinfo_load (ShapeInfo* info)
{
  static xmlSAXHandler saxHandler;
  static gboolean once = FALSE;
#define BLOCKSIZE 512
  char buffer[BLOCKSIZE];
  FILE *f;
  int n;
  Context ctx = { info, READ_ON };

  g_assert (info->filename != NULL);

  if (!once) {
    LIBXML_TEST_VERSION

    memset(&saxHandler, 0, sizeof(saxHandler));
    saxHandler.initialized = XML_SAX2_MAGIC;
    saxHandler.startElementNs = startElementNs;
    saxHandler.characters = _characters;
    saxHandler.endElementNs = endElementNs;
    saxHandler.error = _error;
    saxHandler.warning = _warning;
    once = TRUE;
  }
  f = g_fopen (info->filename, "rb");
  if (!f)
    return FALSE;
  while ((n = fread (buffer, 1, BLOCKSIZE, f)) > 0) {
    int result = xmlSAXUserParseMemory (&saxHandler, &ctx, buffer, n);
    if (result != 0)
      break;
    if (ctx.state == READ_DONE)
      break;
  }
  fclose (f);
  if (ctx.state == READ_DONE) {
    char *tmp = info->icon;
    if (tmp) {
      info->icon = custom_get_relative_filename (info->filename, tmp);
      g_clear_pointer (&tmp, g_free);
    }
    return TRUE;
  } else {
    g_printerr ("Preloading shape file '%s' failed.\n"
                "Please ensure that <name/> and <icon/> are early in the file.\n",
                info->filename);
  }
  return FALSE;
}
