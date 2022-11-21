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

#define G_LOG_DOMAIN "DiaCairo"

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <cairo.h>
/* some backend headers, win32 missing in official Cairo */
#include <cairo-svg.h>
#ifdef  CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif
#ifdef  CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif
#ifdef CAIRO_HAS_WIN32_SURFACE
#include <cairo-win32.h>
/* avoid namespace collisions */
#endif
#ifdef CAIRO_HAS_SCRIPT_SURFACE
#include <cairo-script.h>
#endif

#include <pango/pangocairo.h>

#include "geometry.h"
#include "dia_image.h"
#include "diarenderer.h"
#include "filter.h"
#include "plug-ins.h"

#include "renderer/diacairo.h"
#include "renderer/diacairo-print.h"

#ifdef CAIRO_HAS_PS_SURFACE
static const gchar *ps_extensions[] = { "ps", NULL };
static DiaExportFilter ps_export_filter = {
    N_("Cairo PostScript"),
    ps_extensions,
    cairo_export_data,
    (void*)OUTPUT_PS,
    "cairo-ps" /* unique name */
};

static const gchar *eps_extensions[] = { "eps", NULL };
static DiaExportFilter eps_export_filter = {
    N_("Encapsulated PostScript"),
    eps_extensions,
    cairo_export_data,
    (void*)OUTPUT_EPS,
    "cairo-eps"
};
#endif

#ifdef CAIRO_HAS_PDF_SURFACE
static const gchar *pdf_extensions[] = { "pdf", NULL };
static DiaExportFilter pdf_export_filter = {
    N_("Cairo Portable Document Format"),
    pdf_extensions,
    /* not using export_print_data() due to bug 599401 */
    cairo_export_data,
    (void*)OUTPUT_PDF,
    "cairo-pdf"
};
#endif

static const gchar *svg_extensions[] = { "svg", NULL };
static DiaExportFilter svg_export_filter = {
    N_("Cairo Scalable Vector Graphics"),
    svg_extensions,
    cairo_export_data,
    (void*)OUTPUT_SVG,
    "cairo-svg",
    FILTER_DONT_GUESS /* don't use this if not asked explicit */
};

#ifdef CAIRO_HAS_SCRIPT_SURFACE
static const gchar *cs_extensions[] = { "cs", NULL };
static DiaExportFilter cs_export_filter = {
    N_("CairoScript"),
    cs_extensions,
    cairo_export_data,
    (void*)OUTPUT_CAIRO_SCRIPT,
    "cairo-script",
    FILTER_DONT_GUESS /* don't use this if not asked explicit */
};
#endif

static const gchar *png_extensions[] = { "png", NULL };
static DiaExportFilter png_export_filter = {
    N_("Cairo PNG"),
    png_extensions,
    cairo_export_data,
    (void*)OUTPUT_PNG,
    "cairo-png"
};

static DiaExportFilter pnga_export_filter = {
    N_("Cairo PNG (with alpha)"),
    png_extensions,
    cairo_export_data,
    (void*)OUTPUT_PNGA,
    "cairo-alpha-png"
};

#if DIA_CAIRO_CAN_EMF
static const gchar *emf_extensions[] = { "emf", NULL };
static DiaExportFilter emf_export_filter = {
    N_("Cairo EMF"),
    emf_extensions,
    cairo_export_data,
    (void*)OUTPUT_EMF,
    "cairo-emf",
    FILTER_DONT_GUESS /* don't use this if not asked explicit */
};

static const gchar *wmf_extensions[] = { "wmf", NULL };
static DiaExportFilter wmf_export_filter = {
    N_("Cairo WMF"),
    wmf_extensions,
    cairo_export_data,
    (void*)OUTPUT_WMF,
    "cairo-wmf",
    FILTER_DONT_GUESS /* don't use this if not asked explicit */
};


static DiaObjectChange *
cairo_clipboard_callback (DiagramData *data,
                          const char  *filename,
                          /* further additions */
                          guint        flags,
                          void        *user_data)
{
  DiaContext *ctx = dia_context_new(_("Cairo Clipboard Copy"));

  g_return_val_if_fail ((OutputKind)user_data == OUTPUT_CLIPBOARD, NULL);
  g_return_val_if_fail (data != NULL, NULL);

  /* filename is not necessary */
  cairo_export_data (data, ctx, filename, filename, user_data);
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
  N_("Print (GTK) …"),
  "/InvisibleMenu/File/FilePrint",
  cairo_print_callback,
  (void *) OUTPUT_PDF
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
  filter_unregister_export(&eps_export_filter);
  filter_unregister_export(&ps_export_filter);
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
  filter_unregister_export(&pdf_export_filter);
#endif
  filter_unregister_export(&svg_export_filter);
#ifdef CAIRO_HAS_SCRIPT_SURFACE
  filter_unregister_export(&cs_export_filter);
#endif
  filter_unregister_export(&png_export_filter);
  filter_unregister_export(&pnga_export_filter);
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
  png_export_filter.renderer_type = g_type_from_name ("DiaCairoInteractiveRenderer");

#ifdef CAIRO_HAS_PS_SURFACE
  filter_register_export(&eps_export_filter);
  filter_register_export(&ps_export_filter);
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
  filter_register_export(&pdf_export_filter);
#endif
  filter_register_export(&svg_export_filter);
#ifdef CAIRO_HAS_SCRIPT_SURFACE
  filter_register_export(&cs_export_filter);
#endif
  filter_register_export(&png_export_filter);
  filter_register_export(&pnga_export_filter);
#if DIA_CAIRO_CAN_EMF
  filter_register_export(&emf_export_filter);
  filter_register_export(&wmf_export_filter);
  filter_register_callback (&cb_clipboard);
#endif

  filter_register_callback (&cb_gtk_print);

  return DIA_PLUGIN_INIT_OK;
}
