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

#include <config.h>
#include "intl.h"
#include "render_object.h"

#include "network.h"

#include "pixmaps/monitor.xpm"

static Object *monitor_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void monitor_save(RenderObject *monitor, ObjectNode obj_node,
			 const char *filename);
static Object *monitor_load(ObjectNode obj_node, int version,
			    const char *filename);

static ObjectTypeOps monitor_type_ops =
{
  (CreateFunc) monitor_create,
  (LoadFunc)   monitor_load,
  (SaveFunc)   monitor_save
};

ObjectType monitor_type =
{
  "Network - General Monitor (With Stand)",   /* name */
  0,                     /* version */
  (char **) monitor_xpm, /* pixmap */

  &monitor_type_ops      /* ops */
};

#define MONITOR_LINE NETWORK_GENERAL_LINEWIDTH
#define MONITOR_WIDTH 3.0
#define MONITOR_HEIGHT 2.25
#define MONITOR_BORDER 0.325
#define MONITOR_UNDER1 0.35
#define MONITOR_UNDER2 0.35

#define MONITOR_BOTTOM (MONITOR_HEIGHT+MONITOR_UNDER1+MONITOR_UNDER2)
RenderObjectDescriptor monitor_desc = {
  NULL,                            /* store */
  { MONITOR_WIDTH*0.5,
    MONITOR_BOTTOM },              /* move_point */
  MONITOR_WIDTH,                   /* width */
  MONITOR_BOTTOM,                  /* height */
  MONITOR_LINE / 2.0,              /* extra_border */

  TRUE,                            /* use_text */
  { MONITOR_WIDTH*0.5,
    MONITOR_BOTTOM + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &monitor_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point p1, p2;
  Point *points;
  Point poly[8];

  /* choose default font name for your locale. see also font_data structure
     in lib/font.c. if "Courier" works for you, it would be better.  */
  monitor_desc.initial_font = font_getfont (_("Courier"));
  
  store = new_render_store();

  rs_add_set_linewidth(store, MONITOR_LINE);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);
    
  p1.x = 0.0;
  p1.y = 0.0;
  p2.x = MONITOR_WIDTH;
  p2.y = MONITOR_HEIGHT;
  rs_add_fill_rect(store, &p1, &p2, &computer_color);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

  p1.x = MONITOR_BORDER;
  p1.y = MONITOR_BORDER;
  p2.x = MONITOR_WIDTH - MONITOR_BORDER;
  p2.y = MONITOR_HEIGHT - MONITOR_BORDER;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  poly[0].x = MONITOR_BORDER * 1.25;
  poly[0].y = MONITOR_HEIGHT;
  poly[1].x = MONITOR_WIDTH * 0.65;
  poly[1].y = MONITOR_HEIGHT;
  poly[2].x = MONITOR_WIDTH * 0.65;
  poly[2].y = MONITOR_HEIGHT + MONITOR_UNDER1;
  poly[3].x = MONITOR_BORDER * 1.50;
  poly[3].y = MONITOR_HEIGHT + MONITOR_UNDER1;
  rs_add_fill_polygon(store, poly, 4, &computer_color);
  rs_add_draw_polygon(store, poly, 4, &color_black);

  poly[0].x = MONITOR_WIDTH * 0.65;
  poly[0].y = MONITOR_HEIGHT;
  poly[1].x = MONITOR_WIDTH - MONITOR_BORDER * 1.25;
  poly[1].y = MONITOR_HEIGHT;
  poly[2].x = MONITOR_WIDTH - MONITOR_BORDER * 1.50;
  poly[2].y = MONITOR_HEIGHT + MONITOR_UNDER1;
  poly[3].x = MONITOR_WIDTH * 0.65;
  poly[3].y = MONITOR_HEIGHT + MONITOR_UNDER1;
  rs_add_fill_polygon(store, poly, 4, &computer_color);
  rs_add_draw_polygon(store, poly, 4, &color_black);

  rs_add_set_linewidth(store, 0.025);

  p1.x = MONITOR_WIDTH * 0.65 + MONITOR_UNDER1 * 0.3;
  p1.y = MONITOR_HEIGHT + MONITOR_UNDER1 * 0.3;
  p2.x = MONITOR_WIDTH * 0.65 + MONITOR_UNDER1 * 0.7;
  p2.y = MONITOR_HEIGHT + MONITOR_UNDER1 * 0.7;
  rs_add_fill_rect(store, &p1, &p2, &color_white);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

  rs_add_set_linewidth(store, MONITOR_LINE);

  poly[0].x = MONITOR_WIDTH * 0.4;
  poly[0].y = MONITOR_HEIGHT + MONITOR_UNDER1;
  poly[1].x = MONITOR_WIDTH * 0.6;
  poly[1].y = MONITOR_HEIGHT + MONITOR_UNDER1;
  poly[2].x = MONITOR_WIDTH * 0.6;
  poly[2].y = MONITOR_HEIGHT + MONITOR_UNDER1 +
    MONITOR_UNDER2*0.5;
  poly[3].x = MONITOR_WIDTH * 0.7;
  poly[3].y = MONITOR_HEIGHT + MONITOR_UNDER1 +
    MONITOR_UNDER2*0.5;
  poly[4].x = MONITOR_WIDTH * 0.7;
  poly[4].y = MONITOR_HEIGHT + MONITOR_UNDER1 +
    MONITOR_UNDER2;
  poly[5].x = MONITOR_WIDTH * 0.3;
  poly[5].y = MONITOR_HEIGHT + MONITOR_UNDER1 +
    MONITOR_UNDER2;
  poly[6].x = MONITOR_WIDTH * 0.3;
  poly[6].y = MONITOR_HEIGHT + MONITOR_UNDER1 +
    MONITOR_UNDER2 * 0.5;
  poly[7].x = MONITOR_WIDTH * 0.4;
  poly[7].y = MONITOR_HEIGHT + MONITOR_UNDER1 +
    MONITOR_UNDER2 * 0.5;
  rs_add_fill_polygon(store, poly, 8, &computer_color);
  rs_add_draw_polygon(store, poly, 8, &color_black);
    
  points = g_new(Point, 1);
  points[0].x = MONITOR_WIDTH * 0.5;
  points[0].y =
    MONITOR_HEIGHT + MONITOR_UNDER1 + MONITOR_UNDER2;
    
  monitor_desc.connection_points = points;
  monitor_desc.num_connection_points = 1;
  monitor_desc.store = store;
}

static Object *
monitor_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  if (monitor_desc.store == NULL) {
    render_to_store();
  }

  return new_render_object(startpoint, handle1, handle2,
			   &monitor_desc);
}

static void
monitor_save(RenderObject *monitor, ObjectNode obj_node,
	     const char *filename)
{
  render_object_save(monitor, obj_node);
}

static Object *
monitor_load(ObjectNode obj_node, int version, const char *filename)
{
  if (monitor_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &monitor_desc);
}
