/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diacairo.c -- Cairo based export plugin for dia
 * Copyright (C) 2004, 2007 Hans Breuer, <Hans@Breuer.Org>
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

#include <cairo.h>
#include "diarenderer.h"

/*
#define DEBUG_CAIRO
 */
#ifdef DEBUG_CAIRO
#  define DIAG_NOTE(action) action
#else
#  define DIAG_NOTE(action)
#endif
/* Unconditional complain about cairo being in wrong state,
 * it usually shows some wrong assumptions in Dia's code.
 */
#define DIAG_STATE(cr) { \
  if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) \
    g_warning ("%s:%d, %s\n", __FILE__, __LINE__, cairo_status_to_string (cairo_status(cr))); \
}

/* --- the renderer base class --- */
G_BEGIN_DECLS

#define DIA_TYPE_CAIRO_RENDERER           (dia_cairo_renderer_get_type ())
#define DIA_CAIRO_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_CAIRO_RENDERER, DiaCairoRenderer))
#define DIA_CAIRO_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_CAIRO_RENDERER, DiaCairoRendererClass))
#define DIA_IS_CAIRO_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_CAIRO_RENDERER))
#define DIA_CAIRO_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_CAIRO_TYPE_RENDERER, DiaCairoRendererClass))

GType dia_cairo_renderer_get_type (void) G_GNUC_CONST;

typedef struct _DiaCairoRenderer DiaCairoRenderer;
typedef struct _DiaCairoRendererClass DiaCairoRendererClass;

/*!
 * \brief Multi format renderer based on cairo API (http://cairographics.org)
 *
 * The DiaCairoRenderer supports various output formats depending on the build
 * configuration of libcairo. Typically these include SVG, PNG, PDF, PostScript
 * and the display of the windowing system in use.
 * Also - with a recent enough GTK+ version the cairo renderer is interfacing 
 * the native printing subsystem.
 * Finally - only on Windows - there is usually support for Windows Metafiles
* (WMF and EMF), the latter used for Clipboard transport of the whole diagram.
 * \extends _DiaRenderer
 */
struct _DiaCairoRenderer
{
  DiaRenderer parent_instance; /*!< GObject inheritance */

  cairo_t *cr; /**< if NULL it gets created from the surface */
  cairo_surface_t *surface; /**< can be NULL to use the provived cr */

  DiagramData *dia; /*!< pointer to the diagram to render, might be NULL for the display case */

  real scale;
  gboolean with_alpha; /*!< define to TRUE for transparent background */
  gboolean skip_show_page; /*!< when using for print avoid the internal show_page */
  gboolean stroke_pending; /*!< to delay call to cairo_stroke */

  /** caching the font description from set_font */
  PangoLayout *layout;

  /*! If set use for fill */
  DiaPattern *pattern;
};

struct _DiaCairoRendererClass
{
  DiaRendererClass parent_class;
};

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

gboolean cairo_export_data (DiagramData *data,
                            DiaContext  *ctx,
                            const gchar *filename,
                            const gchar *diafilename,
                            void        *user_data);

/* FIXME: need to think about proper registration */
GType dia_cairo_interactive_renderer_get_type (void) G_GNUC_CONST;

DiaRenderer *dia_cairo_interactive_renderer_new ();

G_END_DECLS
