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

#include "pixmaps/dataserver.xpm"

static Object *dataserver_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static void dataserver_save(RenderObject *dataserver, ObjectNode obj_node,
			    const char *filename);
static Object *dataserver_load(ObjectNode obj_node, int version,
			       const char *filename);

static ObjectTypeOps dataserver_type_ops =
{
  (CreateFunc) dataserver_create,
  (LoadFunc)   dataserver_load,
  (SaveFunc)   dataserver_save
};

ObjectType dataserver_type =
{
  "Sybase - Dataserver",   /* name */
  0,                     /* version */
  (char **) dataserver_xpm,    /* pixmap */

  &dataserver_type_ops         /* ops */
};

#define DATASERVER_LINE SYBASE_GENERAL_LINEWIDTH
#define DATASERVER_WIDTH 2.0
#define DATASERVER_HEIGHT 2.0

RenderObjectDescriptor dataserver_desc = {
  NULL,                            /* store */
  { DATASERVER_WIDTH*0.5,
    DATASERVER_HEIGHT },              /* move_point */
  DATASERVER_WIDTH,                   /* width */
  DATASERVER_HEIGHT,                  /* height */
  DATASERVER_LINE / 2.0,              /* extra_border */

  TRUE,                            /* use_text */
  { DATASERVER_WIDTH*0.5,
    DATASERVER_HEIGHT + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &dataserver_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point p1, p2;
  Point *points;

  dataserver_desc.initial_font = font_getfont("Courier");
  
  store = new_render_store();

  rs_add_set_linewidth(store, DATASERVER_LINE);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);
    
  p1.x = 0.0;
  p1.y = 0.0;
  p2.x = DATASERVER_WIDTH;
  p2.y = DATASERVER_HEIGHT;
  rs_add_fill_rect(store, &p1, &p2, &color_white);

  p1.x = DATASERVER_WIDTH*0.5;
  p1.y = DATASERVER_HEIGHT*0.5;
  rs_add_fill_ellipse(store, &p1, DATASERVER_WIDTH, DATASERVER_HEIGHT,
		      &color_white);
  rs_add_draw_ellipse(store, &p1, DATASERVER_WIDTH, DATASERVER_HEIGHT,
		      &color_black);

  /* Draw a line from 60 degrees to -60 degrees on the circle */
  p1.x = DATASERVER_WIDTH*0.75;
  p1.y = DATASERVER_HEIGHT*0.073;
  p2.x = DATASERVER_WIDTH*0.75;
  p2.y = DATASERVER_HEIGHT-DATASERVER_HEIGHT*0.072;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = DATASERVER_WIDTH*0.12;
  p1.y = DATASERVER_HEIGHT/6;
  p2.x = DATASERVER_WIDTH*0.75;
  p2.y = DATASERVER_HEIGHT/6;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = DATASERVER_WIDTH*0.0353;
  p1.y = DATASERVER_HEIGHT/3;
  p2.x = DATASERVER_WIDTH*0.75;
  p2.y = DATASERVER_HEIGHT/3;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = DATASERVER_WIDTH*0.0;
  p1.y = DATASERVER_HEIGHT/2;
  p2.x = DATASERVER_WIDTH*0.75;
  p2.y = DATASERVER_HEIGHT/2;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = DATASERVER_WIDTH*0.0353;
  p1.y = DATASERVER_HEIGHT*2/3;
  p2.x = DATASERVER_WIDTH*0.75;
  p2.y = DATASERVER_HEIGHT*2/3;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = DATASERVER_WIDTH*0.12;
  p1.y = DATASERVER_HEIGHT*5/6;
  p2.x = DATASERVER_WIDTH*0.75;
  p2.y = DATASERVER_HEIGHT*5/6;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  points = g_new(Point, 4);
  points[0].x = DATASERVER_WIDTH * 0.5;
  points[0].y = 0.0;
  points[1].x = DATASERVER_WIDTH * 0.5;
  points[1].y = DATASERVER_HEIGHT;
  points[2].x = 0.0;
  points[2].y = DATASERVER_HEIGHT * 0.5;
  points[3].x = DATASERVER_WIDTH;
  points[3].y = DATASERVER_HEIGHT * 0.5;

  dataserver_desc.connection_points = points;
  dataserver_desc.num_connection_points = 4;
  dataserver_desc.store = store;
}

static Object *
dataserver_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  if (dataserver_desc.store == NULL) {
    render_to_store();
  }
  return new_render_object(startpoint, handle1, handle2,
			   &dataserver_desc);
}

static void
dataserver_save(RenderObject *dataserver, ObjectNode obj_node, const char *filename)
{
  render_object_save(dataserver, obj_node);
}

static Object *
dataserver_load(ObjectNode obj_node, int version, const char *filename)
{
  if (dataserver_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &dataserver_desc);
}
