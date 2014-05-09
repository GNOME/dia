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

#include <cairo.h>
/* some backend headers, win32 missing in official Cairo */
#ifdef CAIRO_HAS_PNG_SURFACE_FEATURE
#include <cairo-png.h>
#endif
#ifdef  CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif
#ifdef  CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif
#ifdef CAIRO_HAS_WIN32_SURFACE
#include <cairo-win32.h>
/* avoid namespace collisions */
#define Rectangle RectangleWin32
#endif
#ifdef CAIRO_HAS_SCRIPT_SURFACE
#include <cairo-script.h>
#endif

#ifdef HAVE_PANGOCAIRO_H
#include <pango/pangocairo.h>
#endif

#include "intl.h"
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
  OUTPUT_CLIPBOARD,
  OUTPUT_SVG,
  OUTPUT_CAIRO_SCRIPT
} OutputKind;

#if defined CAIRO_HAS_WIN32_SURFACE && CAIRO_VERSION > 10510
#define DIA_CAIRO_CAN_EMF 1
#pragma message ("DiaCairo can EMF;)")
#endif

/* dia export funtion */
static gboolean
export_data(DiagramData *data, DiaContext *ctx,
	    const gchar *filename, const gchar *diafilename,
            void* user_data)
{
  DiaCairoRenderer *renderer;
  FILE *file;
  real width, height;
  OutputKind kind = (OutputKind)user_data;
  /* the passed in filename is in GLib's filename encoding. On Linux everything 
   * should be fine in passing it to the C-runtime (or cairo). On win32 GLib's
   * filename encdong is always utf-8, so another conversion is needed.
   */
  gchar *filename_crt = (gchar *)filename;
#if DIA_CAIRO_CAN_EMF
  HDC hFileDC = NULL;
#endif

  if (kind != OUTPUT_CLIPBOARD) {
    file = g_fopen(filename, "wb"); /* "wb" for binary! */

    if (file == NULL) {
      dia_context_add_message_with_errno(ctx, errno, _("Can't open output file %s."), 
					 dia_context_get_filename(ctx));
      return FALSE;
    }
    fclose (file);
#ifdef G_OS_WIN32
    filename_crt =  g_locale_from_utf8 (filename, -1, NULL, NULL, NULL);
    if (!filename_crt) {
      dia_context_add_message(ctx, _("Can't convert output filename '%s' to locale encoding.\n"
				     "Please choose a different name to save with Cairo.\n"),
			      dia_context_get_filename(ctx));
      return FALSE;
    }
#endif
  } /* != CLIPBOARD */
  renderer = g_object_new (DIA_TYPE_CAIRO_RENDERER, NULL);
  renderer->dia = data; /* FIXME: not sure if this a good idea */
  renderer->scale = 1.0;

  switch (kind) {
#ifdef CAIRO_HAS_PS_SURFACE
  case OUTPUT_PS :
    width  = data->paper.width * (72.0 / 2.54) + 0.5;
    height = data->paper.height * (72.0 / 2.54) + 0.5;
    renderer->scale = data->paper.scaling * (72.0 / 2.54);
    DIAG_NOTE(g_message ("PS Surface %dx%d\n", (int)width, (int)height)); 
    renderer->surface = cairo_ps_surface_create (filename_crt,
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
    width  = ceil((data->extents.right - data->extents.left) * renderer->scale) + 1;
    height = ceil((data->extents.bottom - data->extents.top) * renderer->scale) + 1;
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
#define DPI 300.0 /* 600.0? */
    /* I just don't get how the scaling is supposed to work, dpi versus page size ? */
    renderer->scale = data->paper.scaling * (72.0 / 2.54);
    /* Dia's paper.width already contains the scale, cairo needs it without 
     * Similar for margins, Dia's without, but cairo wants them. The full
     * extents don't matter here, because we do cairo_pdf_set_size() for every page.
     */
    width = (data->paper.lmargin + data->paper.width * data->paper.scaling + data->paper.rmargin)
          * (72.0 / 2.54) + 0.5;
    height = (data->paper.tmargin + data->paper.height * data->paper.scaling + data->paper.bmargin)
           * (72.0 / 2.54) + 0.5;
    DIAG_NOTE(g_message ("PDF Surface %dx%d\n", (int)width, (int)height));
    renderer->surface = cairo_pdf_surface_create (filename_crt,
                                                  width, height);
    cairo_surface_set_fallback_resolution (renderer->surface, DPI, DPI);
#undef DPI
    break;
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
  case OUTPUT_SVG :
    /* quite arbitrary, but consistent with ../pixbuf ;-) */
    renderer->scale = 20.0 * data->paper.scaling; 
    width  = ceil((data->extents.right - data->extents.left) * renderer->scale) + 1;
    height = ceil((data->extents.bottom - data->extents.top) * renderer->scale) + 1;
    DIAG_NOTE(g_message ("SVG Surface %dx%d\n", (int)width, (int)height));
    /* use case screwed by API shakeup. We need to special case */
    renderer->surface = cairo_svg_surface_create(
						filename_crt,
						(int)width, (int)height);
    break;
#endif
#ifdef CAIRO_HAS_SCRIPT_SURFACE
  case OUTPUT_CAIRO_SCRIPT :
    /* quite arbitrary, but consistent with ../pixbuf ;-) */
    renderer->scale = 20.0 * data->paper.scaling; 
    width  = (data->extents.right - data->extents.left) * renderer->scale + 0.5;
    height = (data->extents.bottom - data->extents.top) * renderer->scale + 0.5;
    DIAG_NOTE(g_message ("CairoScript Surface %dx%d\n", (int)width, (int)height));
    {
      cairo_device_t *csdev = cairo_script_create (filename_crt);
      cairo_script_set_mode (csdev, CAIRO_SCRIPT_MODE_ASCII);
      renderer->surface = cairo_script_surface_create(csdev, CAIRO_CONTENT_COLOR_ALPHA,
						      width, height);
      cairo_device_destroy (csdev);
    }
    break;
#endif
  /* finally cairo can render to MetaFiles */
#if DIA_CAIRO_CAN_EMF
  case OUTPUT_EMF :
  case OUTPUT_WMF : /* different only on close/'play' */
  case OUTPUT_CLIPBOARD :
    /* NOT: renderer->with_alpha = TRUE; */
    {
      /* see wmf/wmf.cpp */
      /* CreateEnhMetaFile() takes 0.01 mm, but the resulting clipboard 
       * image is much too big, e.g. when pasting to PowerPoint. So instead
       * of 1000 use sth smaller to scale? But that would need new scaling 
       * for line thickness as well ... 
       * Also there is something wrong with clipping if running on a dual screen
       * sometimes parts of the diagram are clipped away. Not sure if this is
       * hitting some internal width limits, maintianing the viewport ratio,
       * but not the diagram boundaries.
       */
      RECT bbox = { 0, 0, 
                   (int)((data->extents.right - data->extents.left) * data->paper.scaling * 1000.0),
		   (int)((data->extents.bottom - data->extents.top) * data->paper.scaling * 1000.0) };
      RECT clip;
      /* CreateEnhMetaFile() takes resolution 0.01 mm,  */
      hFileDC = CreateEnhMetaFile (NULL, NULL, &bbox, "DiaCairo\0Diagram\0");

#if 0
      /* On Windows 7/64 with two wide screen monitors, the clipping of the resulting 
       * metafile is too small. Scaling the bbox or via SetWorldTransform() does not help. 
       * Maybe we need to explitily set the clipping for cairo?
       */
      GetClipBox (hFileDC, &clip); /* this is the display resolution */
      if (clip.right / (real)bbox.right > clip.bottom / (real)bbox.bottom)
	clip.right = clip.bottom * bbox.right / bbox.bottom;
      else
	clip.bottom = clip.bottom * bbox.right / bbox.bottom;

      IntersectClipRect(hFileDC, clip.left, clip.top, clip.right, clip.bottom);
#endif
      renderer->surface = cairo_win32_printing_surface_create (hFileDC);

      renderer->scale = 1000.0/25.4 * data->paper.scaling;
      if (LOBYTE (g_win32_get_windows_version()) > 0x05 ||
	  LOWORD (g_win32_get_windows_version()) > 0x0105)
	renderer->scale *= 0.72; /* Works w/o for XP, but not on Vista/Win7 */
    }
    break;
#endif
  default :
    /* quite arbitrary, but consistent with ../pixbuf ;-) */
    renderer->scale = 20.0 * data->paper.scaling; 
    width  = ceil((data->extents.right - data->extents.left) * renderer->scale) + 1;
    height = ceil((data->extents.bottom - data->extents.top) * renderer->scale) + 1;
    DIAG_NOTE(g_message ("Image Surface %dx%d\n", (int)width, (int)height)); 
    renderer->surface = cairo_image_surface_create (CAIRO_FORMAT_A8, (int)width, (int)height);
  }

  /* use extents */
  DIAG_NOTE(g_message("export_data extents %f,%f -> %f,%f", 
            data->extents.left, data->extents.top, data->extents.right, data->extents.bottom));

  if (OUTPUT_PDF == kind)
    data_render_paginated(data, DIA_RENDERER(renderer), NULL);
  else
    data_render(data, DIA_RENDERER(renderer), NULL, NULL, NULL);

#if defined CAIRO_HAS_PNG_FUNCTIONS
  if (OUTPUT_PNGA == kind || OUTPUT_PNG == kind)
    {
      cairo_surface_write_to_png(renderer->surface, filename_crt);
      cairo_surface_destroy(renderer->surface);
    }
#endif
#if DIA_CAIRO_CAN_EMF
  if (OUTPUT_EMF == kind) {
    FILE* f = g_fopen(filename, "wb");
    HENHMETAFILE hEmf = CloseEnhMetaFile(hFileDC);
    UINT nSize = GetEnhMetaFileBits (hEmf, 0, NULL);
    BYTE* pData = g_new(BYTE, nSize);
    nSize = GetEnhMetaFileBits (hEmf, nSize, pData);
    if (f) {
      fwrite(pData,1,nSize,f);
      fclose(f);
    } else {
      dia_context_add_message(ctx, _("Can't write %d bytes to %s"), nSize, filename);
    }
    DeleteEnhMetaFile (hEmf);
    g_free (pData);
  } else if (OUTPUT_WMF == kind) {
    FILE* f = g_fopen(filename, "wb");
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
      dia_context_add_message(ctx, _("Can't write %d bytes to %s"), nSize, filename);
    }
    ReleaseDC(NULL, hdc);
    DeleteEnhMetaFile (hEmf);
    g_free (pData);
  } else if (OUTPUT_CLIPBOARD == kind) {
    HENHMETAFILE hEmf = CloseEnhMetaFile(hFileDC);
    if (   OpenClipboard(NULL) 
        && EmptyClipboard() 
        && SetClipboardData (CF_ENHMETAFILE, hEmf)
        && CloseClipboard ()) {
      hEmf = NULL; /* data now owned by clipboard */
    } else {
      dia_context_add_message(ctx, _("Clipboard copy failed"));
      DeleteEnhMetaFile (hEmf);
    }
  }
#endif
  g_object_unref(renderer);
  if (filename != filename_crt)
    g_free (filename_crt);
  return TRUE;
}

G_GNUC_UNUSED /* keep implmentation for reference, see bug 599401 */
static gboolean
export_print_data (DiagramData *data, DiaContext *ctx,
		   const gchar *filename_utf8, const gchar *diafilename,
		   void* user_data)
{
  OutputKind kind = (OutputKind)user_data;
  GtkPrintOperation *op = create_print_operation (data, filename_utf8);
  GtkPrintOperationResult res;
  GError *error = NULL;

# ifdef CAIRO_HAS_PDF_SURFACE
  /* as of this writing the only format Gtk+ supports here is PDF */
  g_assert (OUTPUT_PDF == kind);
# endif

  if (!data) {
    dia_context_add_message(ctx, _("Nothing to print"));
    return FALSE;
  }

  gtk_print_operation_set_export_filename (op, filename_utf8 ? filename_utf8 : "output.pdf");
  res = gtk_print_operation_run (op, GTK_PRINT_OPERATION_ACTION_EXPORT, NULL, &error);
  if (GTK_PRINT_OPERATION_RESULT_ERROR == res) {
    dia_context_add_message(ctx, "%s", error->message);
    g_error_free (error);
    return FALSE;
  }
  return TRUE;
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
    /* not using export_print_data() due to bug 599401 */
    export_data,
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

#ifdef CAIRO_HAS_SCRIPT_SURFACE
static const gchar *cs_extensions[] = { "cs", NULL };
static DiaExportFilter cs_export_filter = {
    N_("CairoScript"),
    cs_extensions,
    export_data,
    (void*)OUTPUT_CAIRO_SCRIPT,
    "cairo-script",
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

static ObjectChange *
cairo_clipboard_callback (DiagramData *data,
                          const gchar *filename,
                          guint flags, /* further additions */
                          void *user_data)
{
  DiaContext *ctx = dia_context_new(_("Cairo Clipboard Copy"));

  g_return_val_if_fail ((OutputKind)user_data == OUTPUT_CLIPBOARD, NULL);
  g_return_val_if_fail (data != NULL, NULL);

  /* filename is not necessary */
  export_data (data, ctx, filename, filename, user_data);
  dia_context_release (ctx);

  return NULL;
}

static DiaCallbackFilter cb_clipboard = {
   "EditCopyDiagram",
    N_("Copy _Diagram"),
    "/DisplayMenu/Edit/CopyDiagram",
    cairo_clipboard_callback,
    (void*)OUTPUT_CLIPBOARD
};
#endif

static DiaCallbackFilter cb_gtk_print = {
    "FilePrintGTK",
    N_("Print (GTK) \342\200\246"),
    "/InvisibleMenu/File/FilePrint",
    cairo_print_callback,
    (void*)OUTPUT_PDF
};

static gboolean
_plugin_can_unload (PluginInfo *info)
{
  /* Can't unload as long as we are giving away our types,
   * e.g. dia_cairo_interactive_renderer_get_type () */
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
#ifdef CAIRO_HAS_SCRIPT_SURFACE
  filter_unregister_export(&cs_export_filter);
#endif
#if defined CAIRO_HAS_PNG_SURFACE || defined CAIRO_HAS_PNG_FUNCTIONS
  filter_unregister_export(&png_export_filter);
  filter_unregister_export(&pnga_export_filter);
#endif
#if DIA_CAIRO_CAN_EMF
  filter_unregister_export(&emf_export_filter);
  filter_unregister_export(&wmf_export_filter);
  filter_unregister_callback (&cb_clipboard);
#endif
  filter_unregister_callback (&cb_gtk_print);
}

/* --- dia plug-in interface --- */

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Cairo",
                            _("Cairo-based Rendering"),
                            _plugin_can_unload,
                            _plugin_unload))
    return DIA_PLUGIN_INIT_ERROR;

  /* FIXME: need to think about of proper way of registration, see also app/display.c */
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
#ifdef CAIRO_HAS_SCRIPT_SURFACE
  filter_register_export(&cs_export_filter);
#endif
#if defined CAIRO_HAS_PNG_SURFACE || defined CAIRO_HAS_PNG_FUNCTIONS
  filter_register_export(&png_export_filter);
  filter_register_export(&pnga_export_filter);
#endif
#if DIA_CAIRO_CAN_EMF
  filter_register_export(&emf_export_filter);
  filter_register_export(&wmf_export_filter);
  filter_register_callback (&cb_clipboard);
#endif

  filter_register_callback (&cb_gtk_print);
  
  return DIA_PLUGIN_INIT_OK;
}
