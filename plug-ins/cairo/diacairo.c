/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diacairo.c -- Cairo based export plugin for dia
 * Copyright (C) 2004, Hans Breuer, <Hans@Breuer.Org>
 *   based on wpg.c 
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
#include <string.h>
#include <math.h>

#include <errno.h>
#define G_LOG_DOMAIN "DiaCairo"
#include <glib.h>
#include <glib/gstdio.h>

#ifdef HAVE_PANGOCAIRO_H
#include <pango/pangocairo.h>
#endif

/*
 * To me the following looks rather suspicious. Why do we need to compile
 * the Cairo plug-in at all if we don't have Cairo? As a result we'll
 * show it in the menus/plugin details and the user expects something
 * although there isn't any functionality behind it. Urgh.
 *
 * With Gtk+-2.7.x cairo must be available so this becomes even more ugly
 * when the user has choosen to not build the diacairo plug-in. If noone
 * can come up with a convincing reason to do it this way I'll probably
 * go back to the dont-build-at-all approach when it breaks the next time.
 *                                                                    --hb 
 */
#ifdef HAVE_CAIRO
#  include <cairo.h>
/* some backend headers, win32 missing in official Cairo */
#  ifdef CAIRO_HAS_PNG_SURFACE_FEATURE
#  include <cairo-png.h>
#  endif
#  ifdef  CAIRO_HAS_PS_SURFACE
#  include <cairo-ps.h>
#  endif
#  ifdef  CAIRO_HAS_PDF_SURFACE
#  include <cairo-pdf.h>
#  endif
#  ifdef CAIRO_HAS_SVG_SURFACE
#  include <cairo-svg.h>
#  endif
#  ifdef CAIRO_HAS_WIN32_SURFACE
#  include <cairo-win32.h>
   /* avoid namespace collisions */
#  define Rectangle RectangleWin32
#  endif
#endif

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "dia_image.h"
#include "diarenderer.h"
#include "filter.h"
#include "plug-ins.h"

#include "diacairo.h"
#include "diacairo-print.h"

typedef enum OutputKind
{
  OUTPUT_PS = 1,
  OUTPUT_PNG,
  OUTPUT_PNGA,
  OUTPUT_PDF,
  OUTPUT_WMF,
  OUTPUT_EMF,
  OUTPUT_CB,
  OUTPUT_SVG
} OutputKind;

#if defined CAIRO_HAS_WIN32_SURFACE && CAIRO_VERSION > 10510
#define DIA_CAIRO_CAN_EMF 1
#pragma message ("DiaCairo can EMF;)")
#endif

/* dia export funtion */
static void
export_data(DiagramData *data, const gchar *filename_utf8, 
            const gchar *diafilename, void* user_data)
{
  DiaCairoRenderer *renderer;
  FILE *file;
  real width, height;
  OutputKind kind = (OutputKind)user_data;
  gchar* filename = g_locale_from_utf8 (filename_utf8, -1, NULL, NULL, NULL);
#if DIA_CAIRO_CAN_EMF
  HDC hFileDC = NULL;
#endif

  if (!filename) {
    message_error(_("Can't convert output filename '%s' to locale encoding.\n"
                    "Please choose a different name to save with cairo.\n"), 
		  dia_message_filename(filename_utf8), strerror(errno));
    return;
  }
  file = g_fopen(filename, "wb"); /* "wb" for binary! */

  if (file == NULL) {
    message_error(_("Can't open output file %s: %s\n"), 
		  dia_message_filename(filename_utf8), strerror(errno));
    return;
  }
  fclose (file);
  renderer = g_object_new (DIA_TYPE_CAIRO_RENDERER, NULL);
  renderer->dia = data; /* FIXME: not sure if this a good idea */
  renderer->scale = 1.0;

  switch (kind) {
#ifdef CAIRO_HAS_PS_SURFACE
  case OUTPUT_PS :
    width  = data->paper.width * (72.0 / 2.54);
    height = data->paper.height * (72.0 / 2.54);
    renderer->scale = data->paper.scaling * (72.0 / 2.54);
    DIAG_NOTE(g_message ("PS Surface %dx%d\n", (int)width, (int)height)); 
    renderer->surface = cairo_ps_surface_create (filename,
                                                 width, height); /*  in points? */
    /* maybe we should increase the resolution here as well */
    break;
#endif  
#if defined CAIRO_HAS_PNG_SURFACE || defined CAIRO_HAS_PNG_FUNCTIONS
  case OUTPUT_PNGA :
    renderer->with_alpha = TRUE;
    /* fall through */
  case OUTPUT_PNG :
    /* quite arbitrary, but consistent with ../pixbuf ;-) */
    renderer->scale = 20.0 * data->paper.scaling; 
    width  = (data->extents.right - data->extents.left) * renderer->scale;
    height = (data->extents.bottom - data->extents.top) * renderer->scale;

    DIAG_NOTE(g_message ("PNG Surface %dx%d\n", (int)width, (int)height));
    /* use case screwed by API shakeup. We need to special case */
    renderer->surface = cairo_image_surface_create(
						CAIRO_FORMAT_ARGB32,
						(int)width, (int)height);
    /* an extra refernce to make it survive end_render():cairo_surface_destroy() */
    cairo_surface_reference(renderer->surface);
    break;
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
  case OUTPUT_PDF :
#define DPI 72.0 /* 600.0? */
    /* I just don't get how the scaling is supposed to work, dpi versus page size ? */
    renderer->scale = data->paper.scaling * (72.0 / 2.54);
    width = data->paper.width * (72.0 / 2.54);
    height = data->paper.height * (72.0 / 2.54);
    DIAG_NOTE(g_message ("PDF Surface %dx%d\n", (int)width, (int)height));
    renderer->surface = cairo_pdf_surface_create (filename,
                                                  width, height);
    cairo_surface_set_fallback_resolution (renderer->surface, DPI, DPI);
#undef DPI
    break;
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
  case OUTPUT_SVG :
    /* quite arbitrary, but consistent with ../pixbuf ;-) */
    renderer->scale = 20.0 * data->paper.scaling; 
    width  = (data->extents.right - data->extents.left) * renderer->scale;
    height = (data->extents.bottom - data->extents.top) * renderer->scale;
    DIAG_NOTE(g_message ("SVG Surface %dx%d\n", (int)width, (int)height));
    /* use case screwed by API shakeup. We need to special case */
    renderer->surface = cairo_svg_surface_create(
						filename,
						(int)width, (int)height);
    break;
#endif
  /* finally cairo can render to MetaFiles */
#if DIA_CAIRO_CAN_EMF
  case OUTPUT_EMF :
  case OUTPUT_WMF : /* different only on close/'play' */
    /* NOT: renderer->with_alpha = TRUE; */
    renderer->scale = 72.0;
    {
      /* see wmf/wmf.cpp */
      HDC  refDC = GetDC(NULL);
      RECT bbox = { 0, 0, 
#if 0
                   (int)((data->extents.right - data->extents.left) * renderer->scale * GetDeviceCaps(refDC, LOGPIXELSX)),
		   (int)((data->extents.bottom - data->extents.top) * renderer->scale * GetDeviceCaps(refDC, LOGPIXELSY)) };
#else
                   (int)((data->extents.right - data->extents.left) * renderer->scale 
		          * 100 * GetDeviceCaps(refDC, HORZSIZE) / GetDeviceCaps(refDC, HORZRES)),
		   (int)((data->extents.bottom - data->extents.top) * renderer->scale
		          * 100 * GetDeviceCaps(refDC, VERTSIZE) / GetDeviceCaps(refDC, VERTRES)) };
#endif
      hFileDC = CreateEnhMetaFile (refDC, NULL, &bbox, "DiaCairo\0Diagram\0");
      renderer->surface = cairo_win32_printing_surface_create (hFileDC);
    }
    break;
#endif
#if 0
  case OUTPUT_CB :
    /* just testing, does not create a file but puts content in the clpboard */
    renderer->scale = 72.0;
    renderer->surface = cairo_win32_surface_create (filename, NULL, CAIRO_WIN32_TARGET_CLIPBOARD, 0, 0);
    break;
#endif
  default :
    /* quite arbitrary, but consistent with ../pixbuf ;-) */
    renderer->scale = 20.0 * data->paper.scaling; 
    width  = (data->extents.right - data->extents.left) * renderer->scale;
    height = (data->extents.bottom - data->extents.top) * renderer->scale;
    DIAG_NOTE(g_message ("Image Surface %dx%d\n", (int)width, (int)height)); 
    renderer->surface = cairo_image_surface_create (CAIRO_FORMAT_A8, (int)width, (int)height);
  }

  /* use extents */
  DIAG_NOTE(g_message("export_data extents %f,%f -> %f,%f", 
            data->extents.left, data->extents.top, data->extents.right, data->extents.bottom));

  data_render(data, DIA_RENDERER(renderer), NULL, NULL, NULL);
#if defined CAIRO_HAS_PNG_FUNCTIONS
  if (OUTPUT_PNGA == kind || OUTPUT_PNG == kind)
    {
      cairo_surface_write_to_png(renderer->surface, filename);
      cairo_surface_destroy(renderer->surface);
    }
#endif
#if DIA_CAIRO_CAN_EMF
  if (OUTPUT_EMF == kind) {
    FILE* f = g_fopen(filename_utf8, "wb");
    HENHMETAFILE hEmf = CloseEnhMetaFile(hFileDC);
    UINT nSize = GetEnhMetaFileBits (hEmf, 0, NULL);
    BYTE* pData = g_new(BYTE, nSize);
    nSize = GetEnhMetaFileBits (hEmf, nSize, pData);
    if (f) {
      fwrite(pData,1,nSize,f);
      fclose(f);
    } else {
      message_error (_("Can't write %d bytes to %s"), nSize, filename_utf8);
    }
    DeleteEnhMetaFile (hEmf);
    g_free (pData);
  } else if (OUTPUT_WMF == kind) {
    FILE* f = g_fopen(filename_utf8, "wb");
    HENHMETAFILE hEmf = CloseEnhMetaFile(hFileDC);
    HDC hdc = GetDC(NULL);
    UINT nSize = GetWinMetaFileBits (hEmf, 0, NULL, MM_ANISOTROPIC, hdc);
    BYTE* pData = g_new(BYTE, nSize);
    nSize = GetWinMetaFileBits (hEmf, nSize, pData, MM_ANISOTROPIC, hdc);
    if (f) {
      /* FIXME: write the placeable header */
      fwrite(pData,1,nSize,f);
      fclose(f);
    } else {
      message_error (_("Can't write %d bytes to %s"), nSize, filename_utf8);
    }
    ReleaseDC(NULL, hdc);
    DeleteEnhMetaFile (hEmf);
    g_free (pData);
  }
#endif
  g_object_unref(renderer);
}

static void
export_print_data (DiagramData *data, const gchar *filename_utf8, 
                   const gchar *diafilename, void* user_data)
{
  OutputKind kind = (OutputKind)user_data;
#if GTK_CHECK_VERSION (2,10,0)
  GtkPrintOperation *op = create_print_operation (data, filename_utf8);
  GtkPrintOperationResult res;
  GError *error = NULL;

# ifdef CAIRO_HAS_PDF_SURFACE
  /* as of this writing the only format Gtk+ supports here is PDF */
  g_assert (OUTPUT_PDF == kind);
# endif

  if (!data) {
    message_error (_("Nothing to print"));
    return;
  }

  gtk_print_operation_set_export_filename (op, filename_utf8 ? filename_utf8 : "output.pdf");
  res = gtk_print_operation_run (op, GTK_PRINT_OPERATION_ACTION_EXPORT, NULL, &error);
  if (GTK_PRINT_OPERATION_RESULT_ERROR == res) {
    message_error (error->message);
    g_error_free (error);
  }
#else
  message_error (_("Printing with Gtk+(cairo) requires at least version 2.10."));
#endif
}

#ifdef CAIRO_HAS_PS_SURFACE
static const gchar *ps_extensions[] = { "ps", NULL };
static DiaExportFilter ps_export_filter = {
    N_("Cairo PostScript"),
    ps_extensions,
    export_data,
    (void*)OUTPUT_PS,
    "cairo-ps" /* unique name */
};
#endif

#ifdef CAIRO_HAS_PDF_SURFACE
static const gchar *pdf_extensions[] = { "pdf", NULL };
static DiaExportFilter pdf_export_filter = {
    N_("Cairo Portable Document Format"),
    pdf_extensions,
# if GTK_CHECK_VERSION (2,10,0)
    export_print_data,
# else
    export_data,
# endif
    (void*)OUTPUT_PDF,
    "cairo-pdf"
};
#endif

#ifdef CAIRO_HAS_SVG_SURFACE
static const gchar *svg_extensions[] = { "svg", NULL };
static DiaExportFilter svg_export_filter = {
    N_("Cairo Scalable Vector Graphics"),
    svg_extensions,
    export_data,
    (void*)OUTPUT_SVG,
    "cairo-svg",
    FILTER_DONT_GUESS /* don't use this if not asked explicit */
};
#endif

static const gchar *png_extensions[] = { "png", NULL };
static DiaExportFilter png_export_filter = {
    N_("Cairo PNG"),
    png_extensions,
    export_data,
    (void*)OUTPUT_PNG,
    "cairo-png"
};

static DiaExportFilter pnga_export_filter = {
    N_("Cairo PNG (with alpha)"),
    png_extensions,
    export_data,
    (void*)OUTPUT_PNGA,
    "cairo-alpha-png"
};

#if DIA_CAIRO_CAN_EMF
static const gchar *emf_extensions[] = { "emf", NULL };
static DiaExportFilter emf_export_filter = {
    N_("Cairo EMF"),
    emf_extensions,
    export_data,
    (void*)OUTPUT_EMF,
    "cairo-emf",
    FILTER_DONT_GUESS /* don't use this if not asked explicit */
};

static const gchar *wmf_extensions[] = { "wmf", NULL };
static DiaExportFilter wmf_export_filter = {
    N_("Cairo WMF"),
    wmf_extensions,
    export_data,
    (void*)OUTPUT_WMF,
    "cairo-wmf",
    FILTER_DONT_GUESS /* don't use this if not asked explicit */
};
#endif

#if 0
static const gchar *cb_extensions[] = { "cb", NULL };
static DiaExportFilter cb_export_filter = {
    N_("Cairo Clipboard"),
    cb_extensions,
    export_data,
    (void*)OUTPUT_CB,
    "cairo-clipboard"
};
#endif /* CAIRO_HAS_WIN32X_SURFACE */

#if GTK_CHECK_VERSION (2,10,0)
static DiaCallbackFilter cb_gtk_print = {
    "FilePrintGTK",
    N_("Print (GTK) ..."),
    "/DisplayMenu/File/FilePrint",
    cairo_print_callback,
    (void*)OUTPUT_PDF
};
#endif

static gboolean
_plugin_can_unload (PluginInfo *info)
{
  /* Can't unlaod as long as we are giving away our types, e.g. dia_cairo_interactive_renderer_get_type () */
  return FALSE;
}

static void
_plugin_unload (PluginInfo *info)
{
#ifdef CAIRO_HAS_PS_SURFACE
  filter_unregister_export(&ps_export_filter);
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
  filter_unregister_export(&pdf_export_filter);
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
  filter_unregister_export(&svg_export_filter);
#endif
#if defined CAIRO_HAS_PNG_SURFACE || defined CAIRO_HAS_PNG_FUNCTIONS
  filter_unregister_export(&png_export_filter);
  filter_unregister_export(&pnga_export_filter);
#endif
#ifdef CAIRO_HAS_WIN32X_SURFACE
  filter_unregister_export(&emf_export_filter);
  filter_unregister_export(&wmf_export_filter);
  filter_unregister_export(&cb_export_filter);
#endif
}

/* --- dia plug-in interface --- */

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Cairo",
                            _("Cairo based Rendering"),
                            _plugin_can_unload,
                            _plugin_unload))
    return DIA_PLUGIN_INIT_ERROR;

  /* FIXME: need to think about of proper way of registartion, see also app/display.c */
  png_export_filter.renderer_type = dia_cairo_interactive_renderer_get_type ();

#ifdef CAIRO_HAS_PS_SURFACE
  filter_register_export(&ps_export_filter);
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
  filter_register_export(&pdf_export_filter);
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
  filter_register_export(&svg_export_filter);
#endif
#if defined CAIRO_HAS_PNG_SURFACE || defined CAIRO_HAS_PNG_FUNCTIONS
  filter_register_export(&png_export_filter);
  filter_register_export(&pnga_export_filter);
#endif
#if DIA_CAIRO_CAN_EMF
  filter_register_export(&emf_export_filter);
  filter_register_export(&wmf_export_filter);
#endif
#ifdef CAIRO_HAS_WIN32X_SURFACE
  filter_register_export(&cb_export_filter);
#endif

#if GTK_CHECK_VERSION (2,10,0)
  filter_register_callback (&cb_gtk_print);
#endif
  
  return DIA_PLUGIN_INIT_OK;
}
