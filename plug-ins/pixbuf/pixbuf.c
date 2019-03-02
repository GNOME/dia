/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * pixbuf.c -- GdkPixbuf Exporter writing to various pixbuf supported
 *             bitmap formats.
 *
 * Copyright (C) 2002, 2004 Hans Breuer, <Hans@Breuer.Org>
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

#include "geometry.h"
#include "renderer/diacairo.h"
#include "filter.h"
#include "plug-ins.h"
#include "properties.h"
#include "object.h"
#include "intl.h"

static gboolean
export_data(DiagramData *data, DiaContext *ctx,
	    const gchar *filename, const gchar *diafilename,
	    void* user_data)
{
  DiaCairoRenderer *renderer;
  GdkColor color;
  int width, height;
  GdkPixbuf* pixbuf = NULL;
  GError* error = NULL;
  Rectangle rect;
  real zoom = 1.0;
  cairo_t *cctx;
  const char* format = (const char*)user_data;

  rect.left = data->extents.left;
  rect.top = data->extents.top;
  rect.right = data->extents.right;
  rect.bottom = data->extents.bottom;

  /* quite arbitrary */
  zoom = 20.0 * data->paper.scaling; 
  /* Adding a bit of padding to account for rounding errors.  Better to
   * pad than to clip.  See bug #413275 */
  width = ceil((rect.right - rect.left) * zoom) + 1;
  height = ceil((rect.bottom - rect.top) * zoom) + 1;

  renderer = g_object_new (dia_cairo_renderer_get_type(), NULL);
  renderer->scale = zoom;
  renderer->surface = cairo_surface_reference (cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                  width, height));

  cctx = cairo_create (renderer->surface);

  /* draw background */
  color_convert (&data->bg_color, &color);
  gdk_cairo_set_source_color (cctx, &color);
  cairo_rectangle (cctx, 0, 0, width, height);
  cairo_fill (cctx);

  data_render (data, DIA_RENDERER (renderer), NULL, NULL, NULL);

  #if GTK_CHECK_VERSION(3,0,0)
  pixbuf = gdk_pixbuf_get_from_surface (renderer->surface, 0, 0,
                                        width, height);
  #else
  {
    GdkPixmap *pixmap;
    cairo_t *cr;

    pixmap = gdk_pixmap_new (NULL, width, height, 24);
    cr = gdk_cairo_create (pixmap);

    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface (cr, renderer->surface, 0, 0);
    cairo_paint (cr);
    pixbuf = gdk_pixbuf_get_from_drawable (NULL,
                                           pixmap,
                                           gdk_colormap_get_system (),
                                           0, 0, 0, 0, width, height);
  }
  #endif
  if (pixbuf)
    {
      gdk_pixbuf_save (pixbuf, filename, format, &error, NULL);
      g_object_unref (pixbuf);
    }
  else
    {
      dia_context_add_message(ctx, _("Failed to create pixbuf from drawable."));
    }

  if (error)
    {
      dia_context_add_message(ctx, _("Could not save file:\n%s\n%s"),
		              dia_context_get_filename(ctx),
			      error->message);
      g_error_free (error);
    }

  g_object_unref (renderer);

  return TRUE;
}

static gboolean
import_data (const gchar *filename, DiagramData *data, DiaContext *ctx, void* user_data)
{
  DiaObjectType *otype = object_get_type("Standard - Image");
  gint width, height;

  if (!otype) /* this would be really broken */
    return FALSE;

  if (!user_data) {
    dia_context_add_message(ctx, _("Calling error, missing user_data."));
    return FALSE;
  }

  if (gdk_pixbuf_get_file_info (filename, &width, &height))
    {
      DiaObject *obj;
      Handle *h1, *h2;
      Point point;
      point.x = point.y = 0.0;

      obj = otype->ops->create(&point, otype->default_user_data, &h1, &h2);
      if (obj)
        {
          GPtrArray *plist = g_ptr_array_new ();

          prop_list_add_filename (plist, "image_file", filename);
          prop_list_add_real (plist, "elem_width", width / 20.0);
          prop_list_add_real (plist, "elem_height", height / 20.0);

          obj->ops->set_props(obj, plist);
          prop_list_free (plist);

          layer_add_object(data->active_layer, obj);
          return TRUE;
        }
    }
  else
    {
      dia_context_add_message(ctx, _("Pixbuf[%s] can't load:\n%s"), 
			      (gchar*)user_data, dia_context_get_filename(ctx));
    }

  return FALSE;
}

static GList *_import_filters = NULL;
static GList *_export_filters = NULL;

/* --- dia plug-in interface --- */
static gboolean
_plugin_can_unload (PluginInfo *info)
{
  return TRUE;
}

static void
_plugin_unload (PluginInfo *info)
{
  GList *p;

  for (p = _import_filters; p != NULL; p = g_list_next(p)) {
    DiaImportFilter *ifilter = (DiaImportFilter *)p->data;
    filter_unregister_import (ifilter);
    g_free ((gchar*)ifilter->description);
    g_strfreev ((gchar**)ifilter->extensions);
    g_free ((gpointer)ifilter->user_data);
    g_free ((gchar*)ifilter->unique_name);
  }
  g_list_free (_import_filters);
  for (p = _export_filters; p != NULL; p = g_list_next(p)) {
    DiaExportFilter *efilter = p->data;
    filter_unregister_export (efilter);
    g_free ((gchar*)efilter->description);
    g_strfreev ((gchar**)efilter->extensions);
    g_free ((gpointer)efilter->user_data);
    g_free ((gchar*)efilter->unique_name);
  }
  g_list_free (_export_filters);
}

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  /*
   * If GTK is not initialized yet don't register this plug-in. This is
   * almost the same as app_is_interactive() but avoids to make plug-ins
   * depend on Dia's app core function. Also what we really need is a 
   * display, not an interactive app ;)
   */
  if (gdk_display_get_default ()) {
    if (!dia_plugin_info_init(info, "Pixbuf",
			      _("gdk-pixbuf-based bitmap export/import"),
			      _plugin_can_unload,
                              _plugin_unload))
      return DIA_PLUGIN_INIT_ERROR;
    else {
      GSList* formats = gdk_pixbuf_get_formats ();
      GSList* sl;

      /* if we get this far we still may be running non-interactive. To avoid complains
       * from color_convert() we are initializing ourselves ;)
       */
     color_init ();

     /*
      * Instead of hard-coding capabilities, ask GdkPixbuf what's installed
      */
     for (sl = formats; sl != NULL; sl = g_slist_next (sl)) {
        GdkPixbufFormat* format = (GdkPixbufFormat*)sl->data;

        if (gdk_pixbuf_format_is_writable (format)) {
          DiaExportFilter* efilter = g_new0 (DiaExportFilter, 1);
          gchar* name;

          name = gdk_pixbuf_format_get_name (format);
          /* the pixbuf desriptions are too generic for Dia's usage, make our own */
          efilter->description = g_strdup_printf ("Pixbuf[%s]", name);
          /* NOT: gdk_pixbuf_format_get_description (format); */
          efilter->extensions = (const gchar**)gdk_pixbuf_format_get_extensions (format);
          efilter->export_func = export_data;
          efilter->user_data = g_strdup (name);
          efilter->unique_name = g_strdup_printf ("pixbuf-%s", (gchar*)efilter->user_data);
          g_free (name);
          _export_filters = g_list_append (_export_filters, efilter);
          filter_register_export(efilter);
        }
        /* there is no write only filter */
        {
          DiaImportFilter* ifilter;
          gchar* name;

          name = gdk_pixbuf_format_get_name (format);
          /* filter out the less useful ones to keep the total list reasonable short 
           * (If anyone complains make it configurable via persistence and this default ;-)
           */
          if (   strcmp (name, "ani") == 0
              || strcmp (name, "ico") == 0
              || strcmp (name, "pcx") == 0
              || strcmp (name, "pnm") == 0
              || strcmp (name, "ras") == 0
              || strcmp (name, "tiff") == 0
              || strcmp (name, "wbmp") == 0
              || strcmp (name, "xbm") == 0)
            {
              g_free (name);
              continue;
            }
	  ifilter = g_new0 (DiaImportFilter, 1);
          /* the pixbuf desriptions are too generic for Dia's usage, make our own */
          ifilter->description = g_strdup_printf ("Pixbuf[%s]", name);
          ifilter->extensions = (const gchar**)gdk_pixbuf_format_get_extensions (format);
          ifilter->import_func = import_data;
          ifilter->user_data = gdk_pixbuf_format_get_name (format);
          /* they are in differnt namespaces aren't they? */
          ifilter->unique_name = g_strdup_printf ("pixbuf-%s", name);
	  /* don't use pixbuf loader for vector formats */
	  if (   strcmp (name, "svg") == 0
	      || strcmp (name, "svgz") == 0
	      || strcmp (name, "wmf") == 0
	      || strcmp (name, "emf") == 0)
	    ifilter->hints = FILTER_DONT_GUESS;
          g_free (name);
          _import_filters = g_list_append (_import_filters, ifilter);
          filter_register_import(ifilter);
        }
      }
      g_slist_free (formats);
    }
  }

  return DIA_PLUGIN_INIT_OK;
}
