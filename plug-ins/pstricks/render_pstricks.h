/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * render_pstricks.h: Exporting module/plug-in to TeX Pstricks
 * Copyright (C) 2000 Jacek Pliszka <pliszka@fuw.edu.pl>
 *  6.5.2000
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
#ifndef RENDER_PSTRICKS_H
#define RENDER_PSTRICKS_H

#include <stdio.h>

#include "geometry.h"

#include "diarenderer.h"

G_BEGIN_DECLS

#define PSTRICKS_TYPE_RENDERER           (pstricks_renderer_get_type ())
#define PSTRICKS_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), PSTRICKS_TYPE_RENDERER, PstricksRenderer))
#define PSTRICKS_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), PSTRICKS_TYPE_RENDERER, PstricksRendererClass))
#define PSTRICKS_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PSTRICKS_TYPE_RENDERER))
#define PSTRICKS_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PSTRICKS_TYPE_RENDERER, PstricksRendererClass))

GType pstricks_renderer_get_type (void) G_GNUC_CONST;

typedef struct _PstricksRenderer PstricksRenderer;
typedef struct _PstricksRendererClass PstricksRendererClass;

struct _PstricksRendererClass
{
    DiaRendererClass parent_class;
};

struct _PstricksRenderer
{
  DiaRenderer parent_instance;

  FILE *file;
  int is_ps;
  int pagenum;

  DiaContext *ctx;

  DiaFont *font;
  double font_height;
};

extern DiaExportFilter pstricks_export_filter;

G_END_DECLS

#endif /* RENDER_PSTRICKS_H */

