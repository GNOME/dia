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
#include "prop_text.h"
#include "prop_geomtypes.h"

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
  if (0 == g_strcasecmp (format, "jpg"))
    format = "jpeg"; /* format name does not match common extension */

  rect.left = data->extents.left;
  rect.top = data->extents.top;
  rect.right = data->extents.right;
  rect.bottom = data->extents.bottom;

  /* quite arbitrary */
  zoom = 20.0 * data->paper.scaling; 
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

static gboolean
import_data (const gchar *filename, DiagramData *data, void* user_data)
{
  ObjectType *otype = object_get_type("Standard - Image");
  GdkPixbuf *pixbuf;
  GError *error = NULL;

  if (!otype) /* this would be really broken */
    return FALSE;

  /* there appears to be no way to ask gdk-pixbuf if it can handle a
   * file other than really loading it. So let's load it twice ...
   */
  pixbuf = gdk_pixbuf_new_from_file (filename, &error);
  if (pixbuf)
    {
      Object *obj;
      Handle *h1, *h2;
      Point point;
      point.x = point.y = 0.0;

      /* we don't need this one ... */
      g_object_unref (pixbuf);

      obj = otype->ops->create(&point, otype->default_user_data, &h1, &h2);
      if (obj)
        {
          PropDescription prop_descs [] = {
            { "image_file", PROP_TYPE_FILE },
            { "elem_width", PROP_TYPE_REAL },
            PROP_DESC_END};
          GPtrArray *plist = prop_list_from_descs (prop_descs, pdtpp_true);
          StringProperty *strprop = g_ptr_array_index(plist, 0);
          RealProperty *realprop  = g_ptr_array_index(plist, 1);

          strprop->string_data = g_strdup (filename);
          realprop->real_data = data->extents.right - data->extents.left;
          obj->ops->set_props(obj, plist);
          prop_list_free (plist);

          layer_add_object(data->active_layer, obj);
          return TRUE;
        }
    }
  else if (error) /* otherwise a pixbuf misbehaviour */
    {
      message_warning ("Failed to load:\n%s", error->message);
      g_error_free (error);
    }

  return FALSE;
}

static const gchar *extensions[] = { "png", "bmp", "jpg", "jpeg", NULL };
static DiaExportFilter export_filter = {
    N_("GdkPixbuf bitmap"),
    extensions,
    export_data
};

static const gchar *extensions_import[] = {
	"bmp", "gif", "jpg", "png", "pnm", "ras", "tif", NULL };
DiaImportFilter import_filter = {
	N_("GdkPixbuf bitmap"),
	extensions_import,
	import_data
};

/* --- dia plug-in interface --- */

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Pixbuf",
                            _("gdk-pixbuf based bitmap export/import"),
                            NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  filter_register_export(&export_filter);
  filter_register_import(&import_filter);

  return DIA_PLUGIN_INIT_OK;
}
