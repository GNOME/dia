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
#ifndef RENDER_STORE_H
#define RENDER_STORE_H

#include "render.h"
#include "dia_image.h"

typedef struct _RenderStore RenderStore;

RenderStore *new_render_store(void);
void destroy_render_store(RenderStore *store);
void render_store_render(RenderStore *store,
			 Renderer *renderer,
			 Point *pos, real magnify);

void rs_add_set_linewidth(RenderStore *store, real linewidth);
void rs_add_set_linecaps(RenderStore *store, LineCaps mode);
void rs_add_set_linejoin(RenderStore *store, LineJoin mode);
void rs_add_set_linestyle(RenderStore *store, LineStyle mode);
void rs_add_set_dashlength(RenderStore *store, real length);
void rs_add_set_fillstyle(RenderStore *store, FillStyle mode);
void rs_add_set_font(RenderStore *store, Font *font, real height);

void rs_add_draw_line(RenderStore *store,
		      Point *start, Point *end, Color *color);
void rs_add_draw_polyline(RenderStore *store,
			  Point *points, int num_points,
			  Color *color);
void rs_add_draw_polygon(RenderStore *store,
			 Point *points, int num_points,
			 Color *color); 
void rs_add_fill_polygon(RenderStore *store,
			 Point *points, int num_points,
			 Color *color); 
void rs_add_draw_rect(RenderStore *store, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
void rs_add_fill_rect(RenderStore *store, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
void rs_add_draw_arc(RenderStore *store,
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
void rs_add_fill_arc(RenderStore *store,
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
void rs_add_draw_ellipse(RenderStore *store,
			 Point *center,
			 real width, real height,
			 Color *color);
void rs_add_fill_ellipse(RenderStore *store,
			 Point *center,
			 real width, real height,
			 Color *color);
void rs_add_draw_bezier(RenderStore *store,
			BezPoint *points,
			int numpoints,
			Color *color);
void rs_add_fill_bezier(RenderStore *store,
			BezPoint *points,
			int numpoints,
			Color *color);
void rs_add_draw_string(RenderStore *store,
			const char *text,
			Point *pos,
			Alignment alignment,
			Color *color);
void rs_add_draw_image(Renderer *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage *image);

#endif /* RENDER_STORE_H */
