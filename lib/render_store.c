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
#include <string.h>
#include <stdio.h>

#include "render_store.h"
#include "message.h"
#include "dia_image.h"

typedef struct _RenderStorePrivate RenderStorePrivate;


typedef enum _Command {
  CMD_SET_LINEWIDTH,
  CMD_SET_LINECAPS,
  CMD_SET_LINEJOIN,
  CMD_SET_LINESTYLE,
  CMD_SET_DASHLENGTH,
  CMD_SET_FILLSTYLE,
  CMD_SET_FONT,
  CMD_DRAW_LINE,
  CMD_DRAW_POLYLINE,
  CMD_DRAW_POLYGON,
  CMD_FILL_POLYGON,
  CMD_DRAW_RECT,
  CMD_FILL_RECT,
  CMD_DRAW_ARC,
  CMD_FILL_ARC,
  CMD_DRAW_ELLIPSE,
  CMD_FILL_ELLIPSE,
  CMD_DRAW_BEZIER,
  CMD_FILL_BEZIER,
  CMD_DRAW_STRING,
  CMD_DRAW_IMAGE
} Command;

typedef union _Data {
  Command command;
  real real_data;
  int int_data;
  void *ptr_data;
} Data;

struct _RenderStorePrivate {
  RenderStore public;

  Data *data;
  int max_num_data;
  int num_data;

  Point corner;
  real magnify;
};


RenderStore *
new_render_store(void)
{
  RenderStorePrivate *store;

  store = g_new(RenderStorePrivate, 1);
  store->data = NULL;
  store->num_data = 0;

  return (RenderStore *)store;
}

void
destroy_render_store(RenderStore *store)
{
  RenderStorePrivate *private;

  private = (RenderStorePrivate *)store;

  g_free(private->data);
  g_free(store);
}


/* Adding: */

static void
add_data(RenderStorePrivate *store, Data *data)
{
  if (store->data == NULL) {
    store->max_num_data = 30;
    store->data = g_malloc(store->max_num_data * sizeof(Data));
  }
  if (store->num_data >= store->max_num_data) {
    store->max_num_data += 30;
    store->data = g_realloc(store->data, store->max_num_data * sizeof(Data));
  }

  store->data[store->num_data] = *data;
  
  store->num_data++;
}

static void
help_add_point(RenderStorePrivate *store, Point *point)
{
  Data data;
  data.real_data = point->x;
  add_data(store, &data);
  data.real_data = point->y;
  add_data(store, &data);
}

static void
help_add_color(RenderStorePrivate *store, Color *color)
{
  Data data;
  data.real_data = color->red;
  add_data(store, &data);
  data.real_data = color->green;
  add_data(store, &data);
  data.real_data = color->blue;
  add_data(store, &data);
}

static void
add_point_point_color(RenderStorePrivate *store, Command command,
		      Point *start, Point *end, Color *color)
{
  Data data;

  data.command = command;
  add_data(store, &data);
  help_add_point(store, start);
  help_add_point(store, end);
  help_add_color(store, color);
}

static void
add_real(RenderStorePrivate *store, Command command, real real_val)
{
  Data data;

  data.command = command;
  add_data(store, &data);
  data.real_data = real_val;
  add_data(store, &data);
}

static void
add_int(RenderStorePrivate *store, Command command, int int_val)
{
  Data data;

  data.command = command;
  add_data(store, &data);
  data.int_data = int_val;
  add_data(store, &data);
}

static void
add_font_real(RenderStorePrivate *store, Command command,
	      Font *font, real real_val)
{
  Data data;

  data.command = command;
  add_data(store, &data);
  data.ptr_data = (void *)font;
  add_data(store, &data);
  data.real_data = real_val;
  add_data(store, &data);
}

static void
add_points_numpoints_color(RenderStorePrivate *store, Command command,
			   Point *points, int num_points, Color *color)
{
  Data data;
  Point *points_copy;
  
  data.command = command;
  add_data(store, &data);
  data.int_data = num_points;
  add_data(store, &data);

  points_copy = g_malloc(sizeof(Point)*num_points);
  memcpy(points_copy, points, sizeof(Point)*num_points);
  data.ptr_data = (void *) points_copy;
  add_data(store, &data);

  help_add_color(store, color);
}

static void
add_bezpoints_numpoints_color(RenderStorePrivate *store, Command command,
			      BezPoint *points, int num_points, Color *color)
{
  Data data;
  BezPoint *points_copy;
  
  data.command = command;
  add_data(store, &data);
  data.int_data = num_points;
  add_data(store, &data);

  points_copy = g_malloc(sizeof(BezPoint)*num_points);
  memcpy(points_copy, points, sizeof(BezPoint)*num_points);
  data.ptr_data = (void *) points_copy;
  add_data(store, &data);

  help_add_color(store, color);
}

static void
add_point_r4_color(RenderStorePrivate *store, Command command,
		   Point *point,
		   real r1, real r2, real r3, real r4,
		   Color *color)
{
  Data data;

  data.command = command;
  add_data(store, &data);
  
  help_add_point(store, point);
  
  data.real_data = r1;
  add_data(store, &data);
  data.real_data = r2;
  add_data(store, &data);
  data.real_data = r3;
  add_data(store, &data);
  data.real_data = r4;
  add_data(store, &data);

  help_add_color(store, color);
}

static void
add_point_r2_color(RenderStorePrivate *store, Command command,
		   Point *point,
		   real r1, real r2,
		   Color *color)
{
  Data data;

  data.command = command;
  add_data(store, &data);
  
  help_add_point(store, point);
  
  data.real_data = r1;
  add_data(store, &data);
  data.real_data = r2;
  add_data(store, &data);

  help_add_color(store, color);
}

static void
add_ptr_point_int_color(RenderStorePrivate *store, Command command,
			void *ptr,
			Point *point,
			int int_val,
			Color *color)
{
  Data data;

  data.command = command;
  add_data(store, &data);
  
  data.ptr_data = ptr;
  add_data(store, &data);
  
  help_add_point(store, point);
  
  data.int_data = int_val;
  add_data(store, &data);

  help_add_color(store, color);
}

void
rs_add_set_linewidth(RenderStore *store, real linewidth)
{
  add_real((RenderStorePrivate *)store, CMD_SET_LINEWIDTH, linewidth);
}

void
rs_add_set_linecaps(RenderStore *store, LineCaps mode)
{
  add_int((RenderStorePrivate *)store, CMD_SET_LINECAPS, mode);
}

void
rs_add_set_linejoin(RenderStore *store, LineJoin mode)
{
  add_int((RenderStorePrivate *)store, CMD_SET_LINEJOIN, mode);
}

void
rs_add_set_linestyle(RenderStore *store, LineStyle mode)
{
  add_int((RenderStorePrivate *)store, CMD_SET_LINESTYLE, mode);
}

void
rs_add_set_dashlength(RenderStore *store, real length)
{
  add_real((RenderStorePrivate *)store, CMD_SET_DASHLENGTH, length);
}

void
rs_add_set_fillstyle(RenderStore *store, FillStyle mode)
{
  add_int((RenderStorePrivate *)store, CMD_SET_FILLSTYLE, mode);
}

void
rs_add_set_font(RenderStore *store, Font *font, real height)
{
  add_font_real((RenderStorePrivate *)store, CMD_SET_FONT, font, height);
}

void
rs_add_draw_line(RenderStore *store, Point *start, Point *end, Color *color)
{
  add_point_point_color((RenderStorePrivate *)store, CMD_DRAW_LINE,
			start, end, color);
}
		   
void
rs_add_draw_polyline(RenderStore *store,
		     Point *points, int num_points,
		     Color *color)
{
  add_points_numpoints_color((RenderStorePrivate *)store, CMD_DRAW_POLYLINE,
			     points, num_points, color);
}

void
rs_add_draw_polygon(RenderStore *store,
		    Point *points, int num_points,
		    Color *color)
{
  add_points_numpoints_color((RenderStorePrivate *)store, CMD_DRAW_POLYGON,
			     points, num_points, color);
}

void
rs_add_fill_polygon(RenderStore *store,
		    Point *points, int num_points,
		    Color *color)
{
  add_points_numpoints_color((RenderStorePrivate *)store, CMD_FILL_POLYGON,
			     points, num_points, color);
}

void
rs_add_draw_rect(RenderStore *store, 
		 Point *ul_corner, Point *lr_corner,
		 Color *color)
{
  add_point_point_color((RenderStorePrivate *)store, CMD_DRAW_RECT,
			ul_corner, lr_corner, color);
}

void
rs_add_fill_rect(RenderStore *store, 
		 Point *ul_corner, Point *lr_corner,
		 Color *color)
{
  add_point_point_color((RenderStorePrivate *)store, CMD_FILL_RECT,
			ul_corner, lr_corner, color);
}


void
rs_add_draw_arc(RenderStore *store,
		Point *center,
		real width, real height,
		real angle1, real angle2,
		Color *color)
{
  add_point_r4_color((RenderStorePrivate *)store, CMD_DRAW_ARC,
		     center,
		     width, height,
		     angle1, angle2,
		     color);
}

void
rs_add_fill_arc(RenderStore *store,
		Point *center,
		real width, real height,
		real angle1, real angle2,
		Color *color)
{
  add_point_r4_color((RenderStorePrivate *)store, CMD_FILL_ARC,
		     center,
		     width, height,
		     angle1, angle2,
		     color);
}

void
rs_add_draw_ellipse(RenderStore *store,
		    Point *center,
		    real width, real height,
		    Color *color)
{
  add_point_r2_color((RenderStorePrivate *)store, CMD_DRAW_ELLIPSE,
		     center,
		     width, height,
		     color);
}

void
rs_add_fill_ellipse(RenderStore *store,
		    Point *center,
		    real width, real height,
		    Color *color)
{
  add_point_r2_color((RenderStorePrivate *)store, CMD_FILL_ELLIPSE,
		     center,
		     width, height,
		     color);
}

void
rs_add_draw_bezier(RenderStore *store,
		   BezPoint *points,
		   int num_points,
		   Color *color)
{
  add_bezpoints_numpoints_color((RenderStorePrivate *)store,
				CMD_DRAW_BEZIER,
				points, num_points, color);
}

void
rs_add_fill_bezier(RenderStore *store,
		   BezPoint *points,
		   int num_points,
		   Color *color)
{
  add_bezpoints_numpoints_color((RenderStorePrivate *)store,
				CMD_FILL_BEZIER,
				points, num_points, color);
}

void
rs_add_draw_string(RenderStore *store,
		   const char *text,
		   Point *pos,
		   Alignment alignment,
		   Color *color)
{
  char *str;

  str = strdup(text);
  
  add_ptr_point_int_color((RenderStorePrivate *)store,
			  CMD_DRAW_STRING,
			  str, pos, alignment, color);
}

void
rs_add_draw_image(Renderer *store,
		  Point *point,
		  real width, real height,
		  DiaImage *image)
{
}

/* Rendering: */

static void
scale_point(RenderStorePrivate *store, Point *point)
{
  point->x *= store->magnify;
  point->x += store->corner.x;
  point->y *= store->magnify;
  point->y += store->corner.y;
}

static int
render_point_point_color(Renderer *renderer,
			 void (*render_function)(Renderer *renderer, 
						 Point *p1, Point *p2,
						 Color *color),
			 RenderStorePrivate *store,
			 int pos)
{
  Point p1, p2;
  Color color;

  p1.x = store->data[pos++].real_data;
  p1.y = store->data[pos++].real_data;
  scale_point(store, &p1);

  p2.x = store->data[pos++].real_data;
  p2.y = store->data[pos++].real_data;
  scale_point(store, &p2);
  
  color.red = store->data[pos++].real_data;
  color.green = store->data[pos++].real_data;
  color.blue = store->data[pos++].real_data;
  
  (*render_function)(renderer, &p1, &p2, &color);
  return pos;
}

static int
render_real(Renderer *renderer,
	    void (*render_function)(Renderer *renderer, real real_val),
	    RenderStorePrivate *store, int pos)
{
  real real_val;
  
  real_val = store->data[pos++].real_data;
  
  (*render_function)(renderer, real_val);
  return pos;
}

static int
render_int(Renderer *renderer,
	    void (*render_function)(Renderer *renderer, int int_val),
	    RenderStorePrivate *store, int pos)
{
  int int_val;
  
  int_val = store->data[pos++].int_data;
  
  (*render_function)(renderer, int_val);
  return pos;
}

static int
render_font_real(Renderer *renderer,
		 void (*render_function)(Renderer *renderer,
					 Font *font, real real_val),
		 RenderStorePrivate *store, int pos)
{
  Font *font;
  real real_val;
  
  font = (Font *) store->data[pos++].ptr_data;
  real_val = store->data[pos++].real_data;
  
  (*render_function)(renderer, font, real_val);
  return pos;
}

static int
render_points_numpoints_color(Renderer *renderer,
			      void (*render_function)(Renderer *renderer,
						      Point *point,
						      int num_points,
						      Color *color),
			      RenderStorePrivate *store, int pos)
{
  static Point *points = NULL;
  static int maxsize = 0;
  Color color;
  int num_points;
  int i;

  num_points = store->data[pos++].int_data;

  if ((points==NULL) || (maxsize<num_points)) {
    if (points!=NULL)
      g_free(points);
    
    points = g_malloc(sizeof(Point)*num_points);
    maxsize = num_points;
  }
  
  memcpy(points, store->data[pos++].ptr_data,
	 sizeof(Point)*num_points);
  
  color.red = store->data[pos++].real_data;
  color.green = store->data[pos++].real_data;
  color.blue = store->data[pos++].real_data;

  for (i=0;i<num_points;i++) {
    scale_point(store, &points[i]);
  }

  (*render_function)(renderer, points, num_points, &color);

  return pos;
}

static int
render_bezpoints_numpoints_color(Renderer *renderer,
				 void (*render_function)(Renderer *renderer,
							 BezPoint *point,
							 int num_points,
							 Color *color),
				 RenderStorePrivate *store, int pos)
{
  static BezPoint *points = NULL;
  static int maxsize = 0;
  Color color;
  int num_points;
  int i;

  num_points = store->data[pos++].int_data;

  if ((points==NULL) || (maxsize<num_points)) {
    if (points!=NULL)
      g_free(points);
    
    points = g_malloc(sizeof(BezPoint)*num_points);
    maxsize = num_points;
  }
  
  memcpy(points, store->data[pos++].ptr_data,
	 sizeof(BezPoint)*num_points);
  
  color.red = store->data[pos++].real_data;
  color.green = store->data[pos++].real_data;
  color.blue = store->data[pos++].real_data;

  for (i=0;i<num_points;i++) {
    scale_point(store, &points[i].p1);
    scale_point(store, &points[i].p2);
    scale_point(store, &points[i].p3);
  }

  (*render_function)(renderer, points, num_points, &color);

  return pos;
}

static int 
render_point_sreal2_real2_color(Renderer *renderer,
	     void (*render_function)(Renderer *renderer,
				     Point *point,
				     real r1_scaled, real r2_scaled,
				     real r3, real r4,
				     Color *color),
				RenderStorePrivate *store, int pos)
{
  Point p;
  Color color;
  real r1, r2, r3, r4;
  
  p.x = store->data[pos++].real_data;
  p.y = store->data[pos++].real_data;
  scale_point(store, &p);

  r1 = store->data[pos++].real_data;
  r1 *= store->magnify;
  r2 = store->data[pos++].real_data;
  r2 *= store->magnify;

  r3 = store->data[pos++].real_data;
  r4 = store->data[pos++].real_data;
  
  color.red = store->data[pos++].real_data;
  color.green = store->data[pos++].real_data;
  color.blue = store->data[pos++].real_data;

  (*render_function)(renderer, &p, r1, r2, r3, r4, &color);

  return pos;
}

static int 
render_point_sreal2_color(Renderer *renderer,
	     void (*render_function)(Renderer *renderer,
				     Point *point,
				     real r1_scaled, real r2_scaled,
				     Color *color),
				RenderStorePrivate *store, int pos)
{
  Point p;
  Color color;
  real r1, r2;
  
  p.x = store->data[pos++].real_data;
  p.y = store->data[pos++].real_data;
  scale_point(store, &p);

  r1 = store->data[pos++].real_data;
  r1 *= store->magnify;
  r2 = store->data[pos++].real_data;
  r2 *= store->magnify;

  color.red = store->data[pos++].real_data;
  color.green = store->data[pos++].real_data;
  color.blue = store->data[pos++].real_data;

  (*render_function)(renderer, &p, r1, r2, &color);

  return pos;
}

static int 
render_ptr_point_int_color(Renderer *renderer,
	     void (*render_function)(Renderer *renderer,
				     void *ptr,
				     Point *point,
				     int int_val,
				     Color *color),
			   RenderStorePrivate *store, int pos)
{
  void *ptr;
  Point p;
  Color color;
  int int_val;


  ptr = store->data[pos++].ptr_data;
  
  p.x = store->data[pos++].real_data;
  p.y = store->data[pos++].real_data;
  scale_point(store, &p);

  int_val = store->data[pos++].int_data;

  color.red = store->data[pos++].real_data;
  color.green = store->data[pos++].real_data;
  color.blue = store->data[pos++].real_data;

  (*render_function)(renderer, ptr, &p, int_val, &color);

  return pos;
}

void render_store_render(RenderStore *store,
			 Renderer *renderer,
			 Point *pos, real magnify)
{
  RenderStorePrivate *private;
  int i;
  Command command;

  private = (RenderStorePrivate *)store;

  private->corner = *pos;
  private->magnify = magnify;  
  i=0;
  while (i<private->num_data){
    command = private->data[i].command;
    i++;
    
    switch (command) {
    case CMD_SET_LINEWIDTH:
      i = render_real(renderer, renderer->ops->set_linewidth, private, i);
      break;
    case CMD_SET_LINECAPS:
      i = render_int(renderer, renderer->ops->set_linecaps, private, i);
      break;
    case CMD_SET_LINEJOIN:
      i = render_int(renderer, renderer->ops->set_linejoin, private, i);
      break;
    case CMD_SET_LINESTYLE:
      i = render_int(renderer, renderer->ops->set_linestyle, private, i);
      break;
    case CMD_SET_DASHLENGTH:
      i = render_real(renderer, renderer->ops->set_dashlength, private, i);
      break;
    case CMD_SET_FILLSTYLE:
      i = render_int(renderer, renderer->ops->set_fillstyle, private, i);
      break;
    case CMD_SET_FONT:
      i = render_font_real(renderer, renderer->ops->set_font, private, i);
      break;
    case CMD_DRAW_LINE:
      i = render_point_point_color(renderer, renderer->ops->draw_line,
				   private, i);
      break;
    case CMD_DRAW_POLYLINE:
      i = render_points_numpoints_color(renderer, renderer->ops->draw_polyline,
					private, i);
      break;
    case CMD_DRAW_POLYGON:
      i = render_points_numpoints_color(renderer, renderer->ops->draw_polygon,
					private, i);
      break;
    case CMD_FILL_POLYGON:
      i = render_points_numpoints_color(renderer, renderer->ops->fill_polygon,
					private, i);
      break;
    case CMD_FILL_RECT:
      i = render_point_point_color(renderer, renderer->ops->fill_rect,
				   private, i);
      break;
    case CMD_DRAW_RECT:
      i = render_point_point_color(renderer, renderer->ops->draw_rect,
				   private, i);
      break;
    case CMD_DRAW_ARC:
      i = render_point_sreal2_real2_color(renderer,
					  renderer->ops->draw_arc,
					  private, i);
      break;
    case CMD_FILL_ARC:
      i = render_point_sreal2_real2_color(renderer,
					  renderer->ops->fill_arc,
					  private, i);
      break;
    case CMD_DRAW_ELLIPSE:
      i = render_point_sreal2_color(renderer,
				    renderer->ops->draw_ellipse,
				    private, i);
      break;
    case CMD_FILL_ELLIPSE:
      i = render_point_sreal2_color(renderer,
				    renderer->ops->fill_ellipse,
				    private, i);
      break;
    case CMD_DRAW_BEZIER:
      i = render_bezpoints_numpoints_color(renderer,
					   renderer->ops->draw_bezier,
					   private, i);
      break;
    case CMD_FILL_BEZIER:
      i = render_bezpoints_numpoints_color(renderer,
					   renderer->ops->fill_bezier,
					   private, i);
      break;
    case CMD_DRAW_STRING:
      i = render_ptr_point_int_color(renderer,
				     renderer->ops->draw_string,
				     private, i);
      break;
    case CMD_DRAW_IMAGE:
      message_error("Images not supported yet!\n");
      break;
    default:
      message_error("Unknown command in render_store!\n");
    }
  }
}
