/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * pixbuf.c -- GdkPixbuf Exporter writing to various pixbuf supported
 *             bitmap formats.
 *
 * Copyright (C) 2002, Hans Breuer, <Hans@Breuer.Org>
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "diagdkrenderer.h"
#include "filter.h"
#include "plug-ins.h"

static Rectangle rect;
static real zoom = 1.0;

static void
export_data(DiagramData *data, const gchar *filename, 
            const gchar *diafilename, void* user_data)
{
  DiaGdkRenderer *renderer;
  GdkColor color;
  int width, height;
  GdkPixbuf* pixbuf = NULL;
  GError* error = NULL;
  const char* format = strrchr(filename, '.');

  if (format)
    format++;

  rect.left = data->extents.left;
  rect.top = data->extents.top;
  rect.right = data->extents.right;
  rect.bottom = data->extents.bottom;

  /* quite arbitrary */
  zoom = 10.0 * data->paper.scaling; 
  width = rect.right * zoom;
  height = rect.bottom * zoom;

  renderer = g_object_new (DIA_TYPE_GDK_RENDERER, NULL);
  renderer->transform = dia_transform_new (&rect, &zoom);
  renderer->pixmap = gdk_pixmap_new(NULL, width, height, 24);
  renderer->gc = gdk_gc_new(renderer->pixmap);

  /* draw background */
  color_convert (&data->bg_color, &color);
  gdk_gc_set_foreground(renderer->gc, &color);
  gdk_draw_rectangle(renderer->pixmap, renderer->gc, 1,
		 0, 0, width, height);

  data_render(data, DIA_RENDERER (renderer), NULL, NULL, NULL);

  pixbuf = gdk_pixbuf_get_from_drawable (NULL, renderer->pixmap, NULL,
                                         0, 0, 0, 0,
                                         width, height);
  if (pixbuf)
    {
      gdk_pixbuf_save (pixbuf, filename, format, &error, NULL);
      g_object_unref (pixbuf);
    }

  if (error)
    {
      message_warning(_("Could not save file:\n%s\n%s"),
                      filename,
                      error->message);
      g_error_free (error);
    }

  g_object_unref (renderer);
}

static const gchar *extensions[] = { "bmp", "png", "jpg", "jpeg", NULL };
static DiaExportFilter my_export_filter = {
    N_("GdkPixbuf"),
    extensions,
    export_data
};

/* --- dia plug-in interface --- */

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Pixbuf",
                            _("gdk-pixbuf based bitmap export"),
                            NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  filter_register_export(&my_export_filter);
#if WPG_WITH_IMPORT
  filter_register_import(&my_import_filter);
#endif

  return DIA_PLUGIN_INIT_OK;
}
