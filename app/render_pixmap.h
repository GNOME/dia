/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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
#ifndef RENDER_PIXMAP_H
#define RENDER_PIXMAP_H


typedef struct _RendererPixmap RendererPixmap;

#include "geometry.h"
#include "render.h"
#include "display.h"

struct _RendererPixmap {
  Renderer renderer;

  GdkDrawable *pixmap;         /* The pixmap shown in this display  */
  GdkGC *render_gc;
  GdkGCValues gcvalues;        /* Original values stored to reset at end*/

  gint xoffset, yoffset;       /* Offset into the drawable */

  /* line attributes: */
  int line_width;
  GdkLineStyle line_style;
  GdkCapStyle cap_style;
  GdkJoinStyle join_style;

  LineStyle saved_line_style;
  int dash_length;
  int dot_length;

  DiaFont *font;
  real font_height;
};

RendererPixmap *new_pixmap_renderer(GdkWindow *window, int width, int height);
void destroy_pixmap_renderer(RendererPixmap *renderer);
void renderer_pixmap_set_pixmap(RendererPixmap *renderer,
				GdkDrawable *drawable, 
				int xoffset, int yoffset,
				int width, int height);
GdkPixmap *renderer_pixmap_get_pixmap(RendererPixmap *renderer);

#endif /* RENDER_PIXMAP_H */
