/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * render_metapost.h: Exporting module/plug-in to TeX Metapost
 * Copyright (C) 2001 Chris Sperandio
 * Originally derived from render_pstricks.h (pstricks plug-in)
 * Copyright (C) 2000 Jacek Pliszka <pliszka@fuw.edu.pl>
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
#ifndef RENDER_METAPOST_H
#define RENDER_METAPOST_H

#include <stdio.h>

#include "geometry.h"
#include "diarenderer.h"
#include "diagramdata.h"

G_BEGIN_DECLS

#define METAPOST_TYPE_RENDERER           (metapost_renderer_get_type ())
#define METAPOST_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), METAPOST_TYPE_RENDERER, MetapostRenderer))
#define METAPOST_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), METAPOST_TYPE_RENDERER, MetapostRendererClass))
#define METAPOST_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), METAPOST_TYPE_RENDERER))
#define METAPOST_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), METAPOST_TYPE_RENDERER, MetapostRendererClass))

GType metapost_renderer_get_type (void) G_GNUC_CONST;

typedef struct _MetapostRenderer MetapostRenderer;
typedef struct _MetapostRendererClass MetapostRendererClass;

struct _MetapostRenderer {
  DiaRenderer parent_instance;

  FILE *file;

  DiaLineStyle saved_line_style;
  DiaLineCaps  saved_line_cap;
  DiaLineJoin  saved_line_join;

  Color color;

  real line_width;
  real dash_length;
  real dot_length;

  char *mp_font;
  char *mp_weight;
  char *mp_slant;
  real mp_font_height;

  DiaFont *font;
  double font_height;

  DiaContext *ctx;
};

struct _MetapostRendererClass
{
  DiaRendererClass parent_class;
};

extern DiaExportFilter metapost_export_filter;

G_END_DECLS

#endif /* RENDER_METAPOST_H */

