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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "render_object.h"

#include "intl.h"
#include "sybase.h"

#include "pixmaps/stableq.xpm"

static Object *stableq_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void stableq_save(RenderObject *stableq, ObjectNode obj_node,
			 const char *filename);
static Object *stableq_load(ObjectNode obj_node, int version,
			    const char *filename);

static ObjectTypeOps stableq_type_ops =
{
  (CreateFunc) stableq_create,
  (LoadFunc)   stableq_load,
  (SaveFunc)   stableq_save
};

ObjectType stableq_type =
{
  "Sybase - Stable Queue",   /* name */
  0,                     /* version */
  (char **) stableq_xpm,    /* pixmap */

  &stableq_type_ops         /* ops */
};

#define STABLEQ_LINE SYBASE_GENERAL_LINEWIDTH
#define STABLEQ_WIDTH 2.8
#define STABLEQ_HEIGHT 1.4
#define STABLEQ_ELLIPSE 0.4

#define STABLEQ_BOTTOM STABLEQ_HEIGHT+2*STABLEQ_ELLIPSE

RenderObjectDescriptor stableq_desc = {
  NULL,                            /* store */
  { STABLEQ_WIDTH*0.5,
    STABLEQ_BOTTOM },              /* move_point */
  STABLEQ_WIDTH,                   /* width */
  STABLEQ_BOTTOM,                  /* height */
  STABLEQ_LINE / 2.0,              /* extra_border */

  TRUE,                            /* use_text */
  { STABLEQ_WIDTH*0.5,
    STABLEQ_BOTTOM + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &stableq_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point p1, p2;
  Point *points;

  stableq_desc.initial_font = font_getfont("Courier");
  
  store = new_render_store();

  rs_add_set_linewidth(store, STABLEQ_LINE);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);
    
  p1.x = 0.0;
  p1.y = STABLEQ_ELLIPSE;
  p2.x = STABLEQ_WIDTH;
  p2.y = STABLEQ_HEIGHT+STABLEQ_ELLIPSE;
  rs_add_fill_rect(store, &p1, &p2, &color_white);

  p1.x = 0.0;
  p1.y = STABLEQ_ELLIPSE;
  p2.x = 0.0;
  p2.y = STABLEQ_HEIGHT+STABLEQ_ELLIPSE;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = STABLEQ_WIDTH;
  p1.y = STABLEQ_ELLIPSE;
  p2.x = STABLEQ_WIDTH;
  p2.y = STABLEQ_HEIGHT+STABLEQ_ELLIPSE;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = STABLEQ_WIDTH*0.5;
  p1.y = STABLEQ_ELLIPSE+STABLEQ_HEIGHT;
  rs_add_fill_arc(store, &p1, STABLEQ_WIDTH, STABLEQ_ELLIPSE*2,
		  180.0, 360.0,
		  &color_white);
  rs_add_draw_arc(store, &p1, STABLEQ_WIDTH, STABLEQ_ELLIPSE*2,
		  180.0, 360.0,
		  &color_black);

  p1.x = STABLEQ_WIDTH*0.5;
  p1.y = STABLEQ_ELLIPSE+(STABLEQ_HEIGHT*0.75);
  rs_add_fill_arc(store, &p1, STABLEQ_WIDTH, STABLEQ_ELLIPSE*2,
		  180.0, 360.0,
		  &color_white);
  rs_add_draw_arc(store, &p1, STABLEQ_WIDTH, STABLEQ_ELLIPSE*2,
		  180.0, 360.0,
		  &color_black);

  p1.x = STABLEQ_WIDTH*0.5;
  p1.y = STABLEQ_ELLIPSE+(STABLEQ_HEIGHT*0.5);
  rs_add_fill_arc(store, &p1, STABLEQ_WIDTH, STABLEQ_ELLIPSE*2,
		  180.0, 360.0,
		  &color_white);
  rs_add_draw_arc(store, &p1, STABLEQ_WIDTH, STABLEQ_ELLIPSE*2,
		  180.0, 360.0,
		  &color_black);

  p1.x = STABLEQ_WIDTH*0.5;
  p1.y = STABLEQ_ELLIPSE+(STABLEQ_HEIGHT*0.25);
  rs_add_fill_arc(store, &p1, STABLEQ_WIDTH, STABLEQ_ELLIPSE*2,
		  180.0, 360.0,
		  &color_white);
  rs_add_draw_arc(store, &p1, STABLEQ_WIDTH, STABLEQ_ELLIPSE*2,
		  180.0, 360.0,
		  &color_black);
    
  p1.x = STABLEQ_WIDTH*0.5;
  p1.y = STABLEQ_ELLIPSE;
  rs_add_fill_ellipse(store, &p1, STABLEQ_WIDTH, STABLEQ_ELLIPSE*2,
		      &color_white);
  rs_add_draw_ellipse(store, &p1, STABLEQ_WIDTH, STABLEQ_ELLIPSE*2,
		      &color_black);

  points = g_new(Point, 2);
  points[0].x = STABLEQ_WIDTH * 0.5;
  points[0].y = 0.0;

  points[1].x = STABLEQ_WIDTH * 0.5;
  points[1].y = STABLEQ_BOTTOM;

  stableq_desc.connection_points = points;
  stableq_desc.num_connection_points = 2;
  stableq_desc.store = store;
}

static Object *
stableq_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  if (stableq_desc.store == NULL) {
    render_to_store();
  }
  return new_render_object(startpoint, handle1, handle2,
			   &stableq_desc);
}

static void
stableq_save(RenderObject *stableq, ObjectNode obj_node, const char *filename)
{
  render_object_save(stableq, obj_node);
}

static Object *
stableq_load(ObjectNode obj_node, int version, const char *filename)
{
  if (stableq_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &stableq_desc);
}
