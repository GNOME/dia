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
#ifndef RENDER_LIBART_H
#define RENDER_LIBART_H


typedef struct _RendererLibart RendererLibart;

#include "geometry.h"
#include "render.h"
#include "display.h"

#ifdef HAVE_LIBART
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_vpath_dash.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_vpath_stroke.h>
#endif

struct _RendererLibart {
  Renderer renderer;

#ifdef HAVE_LIBART
  DDisplay *ddisp;
  guint8 *rgb_buffer;
  int clip_rect_empty;
  IntRectangle clip_rect;
  
  /* line attributes: */
  double line_width;
  ArtPathStrokeCapType cap_style;
  ArtPathStrokeJoinType join_style;

  LineStyle saved_line_style;
  int dash_enabled;
  ArtVpathDash dash;
  double dash_length;
  double dot_length;

  GdkFont *gdk_font;
  int font_height;
#endif  
};

extern RendererLibart *new_libart_renderer(DDisplay *ddisp);
extern void destroy_libart_renderer(RendererLibart *renderer);
extern void libart_renderer_set_size(RendererLibart *renderer,
				     GdkWindow *window,
				     int width, int height);
extern void renderer_libart_copy_to_window(RendererLibart *renderer,
					   GdkWindow *window,
					   int x, int y,
					   int width, int height);

#endif /* RENDER_GDK_H */
