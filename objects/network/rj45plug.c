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

#include "pixmaps/rj45plug.xpm"

static Object *rj45plug_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void rj45plug_save(RenderObject *rj45plug, ObjectNode obj_node,
			 const char *filename);
static Object *rj45plug_load(ObjectNode obj_node, int version,
			    const char *filename);

static ObjectTypeOps rj45plug_type_ops =
{
  (CreateFunc) rj45plug_create,
  (LoadFunc)   rj45plug_load,
  (SaveFunc)   rj45plug_save
};

ObjectType rj45plug_type =
{
  "Network - RJ45 Wall-Plug",   /* name */
  0,                     /* version */
  (char **) rj45plug_xpm, /* pixmap */

  &rj45plug_type_ops      /* ops */
};

#define RJ45PLUG_LINE NETWORK_GENERAL_LINEWIDTH
#define RJ45PLUG_WIDTH 2.5
#define RJ45PLUG_HEIGHT 2.0
#define RJ45PLUG_BORDER 0.325
#define RJ45PLUG_UNDER1 0.35
#define RJ45PLUG_UNDER2 0.35

#define RJ45PLUG_BOTTOM (RJ45PLUG_HEIGHT+RJ45PLUG_UNDER1+RJ45PLUG_UNDER2)
RenderObjectDescriptor rj45plug_desc = {
  NULL,                            /* store */
  { RJ45PLUG_WIDTH*0.5,
    RJ45PLUG_BOTTOM },              /* move_point */
  RJ45PLUG_WIDTH,                   /* width */
  RJ45PLUG_BOTTOM,                  /* height */
  RJ45PLUG_LINE / 2.0,              /* extra_border */

  TRUE,                            /* use_text */
  { RJ45PLUG_WIDTH*0.5,
    RJ45PLUG_BOTTOM + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &rj45plug_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point p1, p2;
  Point *points;

  rj45plug_desc.initial_font = font_getfont("Courier");
  
  store = new_render_store();

  rs_add_set_linewidth(store, RJ45PLUG_LINE);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);
    
  p1.x = 0.0;
  p1.y = 0.0;
  p2.x = RJ45PLUG_WIDTH;
  p2.y = RJ45PLUG_HEIGHT;
  rs_add_fill_rect(store, &p1, &p2, &computer_color);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

  p1.x = RJ45PLUG_WIDTH * 0.13;
  p1.y = RJ45PLUG_HEIGHT * 0.17;
  p2.x = RJ45PLUG_WIDTH * 0.87;
  p2.y = RJ45PLUG_HEIGHT * 0.83;
  rs_add_fill_rect(store, &p1, &p2, &color_white);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

  p1.x = RJ45PLUG_WIDTH * 0.33;
  p1.y = RJ45PLUG_HEIGHT * 0.67;
  p2.x = RJ45PLUG_WIDTH * 0.67;
  p2.y = RJ45PLUG_HEIGHT * 0.83;
  rs_add_fill_rect(store, &p1, &p2, &computer_color);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

/* the contacts... */

  p1.x = RJ45PLUG_WIDTH * 0.27;
  p1.y = RJ45PLUG_HEIGHT * 0.17;
  p2.x = RJ45PLUG_WIDTH * 0.27;
  p2.y = RJ45PLUG_HEIGHT * 0.25;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = RJ45PLUG_WIDTH * 0.33;
  p1.y = RJ45PLUG_HEIGHT * 0.17;
  p2.x = RJ45PLUG_WIDTH * 0.33;
  p2.y = RJ45PLUG_HEIGHT * 0.25;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = RJ45PLUG_WIDTH * 0.4;
  p1.y = RJ45PLUG_HEIGHT * 0.17;
  p2.x = RJ45PLUG_WIDTH * 0.4;
  p2.y = RJ45PLUG_HEIGHT * 0.25;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = RJ45PLUG_WIDTH * 0.47;
  p1.y = RJ45PLUG_HEIGHT * 0.17;
  p2.x = RJ45PLUG_WIDTH * 0.47;
  p2.y = RJ45PLUG_HEIGHT * 0.25;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = RJ45PLUG_WIDTH * 0.53;
  p1.y = RJ45PLUG_HEIGHT * 0.17;
  p2.x = RJ45PLUG_WIDTH * 0.53;
  p2.y = RJ45PLUG_HEIGHT * 0.25;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = RJ45PLUG_WIDTH * 0.60;
  p1.y = RJ45PLUG_HEIGHT * 0.17;
  p2.x = RJ45PLUG_WIDTH * 0.60;
  p2.y = RJ45PLUG_HEIGHT * 0.25;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = RJ45PLUG_WIDTH * 0.67;
  p1.y = RJ45PLUG_HEIGHT * 0.17;
  p2.x = RJ45PLUG_WIDTH * 0.67;
  p2.y = RJ45PLUG_HEIGHT * 0.25;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = RJ45PLUG_WIDTH * 0.73;
  p1.y = RJ45PLUG_HEIGHT * 0.17;
  p2.x = RJ45PLUG_WIDTH * 0.73;
  p2.y = RJ45PLUG_HEIGHT * 0.25;
  rs_add_draw_line(store, &p1, &p2, &color_black);


    
  points = g_new(Point, 1);
  points[0].x = RJ45PLUG_WIDTH * 0.5;
  points[0].y =
    RJ45PLUG_HEIGHT + RJ45PLUG_UNDER1 + RJ45PLUG_UNDER2;
    
  rj45plug_desc.connection_points = points;
  rj45plug_desc.num_connection_points = 1;
  rj45plug_desc.store = store;
}

static Object *
rj45plug_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  if (rj45plug_desc.store == NULL) {
    render_to_store();
  }

  return new_render_object(startpoint, handle1, handle2,
			   &rj45plug_desc);
}

static void
rj45plug_save(RenderObject *rj45plug, ObjectNode obj_node,
	     const char *filename)
{
  render_object_save(rj45plug, obj_node);
}

static Object *
rj45plug_load(ObjectNode obj_node, int version, const char *filename)
{
  if (rj45plug_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &rj45plug_desc);
}
