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
#ifndef RENDER_GDK_H
#define RENDER_GDK_H


typedef struct _RendererGdk RendererGdk;

#include "geometry.h"
#include "render.h"
#include "display.h"

struct _RendererGdk {
  Renderer renderer;

  DDisplay *ddisp;
  GdkPixmap *pixmap;              /* The pixmap shown in this display  */
  guint32 width;                  /* The width of the pixmap in pixels */
  guint32 height;                 /* The height of the pixmap in pixels */
  GdkGC *render_gc;
  GdkRegion *clip_region;

  /* line attributes: */
  int line_width;
  GdkLineStyle line_style;
  GdkCapStyle cap_style;
  GdkJoinStyle join_style;

  LineStyle saved_line_style;
  int dash_length;
  int dot_length;

#ifdef HAVE_FREETYPE
  FT_Face freetype_font;
#else
  GdkFont *gdk_font;
#endif
  int font_height;
};

RendererGdk *new_gdk_renderer(DDisplay *ddisp);
void destroy_gdk_renderer(RendererGdk *renderer);
void gdk_renderer_set_size(RendererGdk *renderer, GdkWindow *window,
			   int width, int height);
void renderer_gdk_copy_to_window(RendererGdk *renderer,
				 GdkWindow *window,
				 int x, int y,
				 int width, int height);

#endif /* RENDER_GDK_H */
