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
 *
 * Adopted for Hub-Primitive roland@support-system.com
 */

#include <config.h>
#include "intl.h"
#include "render_object.h"

#include "network.h"

#include "pixmaps/hub.xpm"

static Object *hub_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void hub_save(RenderObject *hub, ObjectNode obj_node,
			 const char *filename);
static Object *hub_load(ObjectNode obj_node, int version,
			    const char *filename);

static ObjectTypeOps hub_type_ops =
{
  (CreateFunc) hub_create,
  (LoadFunc)   hub_load,
  (SaveFunc)   hub_save
};

ObjectType hub_type =
{
  "Network - Hub",   /* name */
  0,                     /* version */
  (char **) hub_xpm, /* pixmap */

  &hub_type_ops      /* ops */
};

#define HUB_LINE NETWORK_GENERAL_LINEWIDTH
#define HUB_WIDTH 5
#define HUB_HEIGHT 1.5
#define HUB_BORDER 0.325
#define HUB_UNDER1 0.35
#define HUB_UNDER2 0.35

#define HUB_BOTTOM (HUB_HEIGHT+HUB_UNDER1+HUB_UNDER2)
RenderObjectDescriptor hub_desc = {
  NULL,                            /* store */
  { HUB_WIDTH*0.5,
    HUB_BOTTOM },              /* move_point */
  HUB_WIDTH,                   /* width */
  HUB_BOTTOM,                  /* height */
  HUB_LINE / 2.0,              /* extra_border */

  TRUE,                            /* use_text */
  { HUB_WIDTH*0.5,
    HUB_BOTTOM + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &hub_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point p1, p2;
  Point *points;

  hub_desc.initial_font = font_getfont("Courier");
  
  store = new_render_store();

  rs_add_set_linewidth(store, HUB_LINE);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);
    
  p1.x = 0.0;
  p1.y = 0.0;
  p2.x = HUB_WIDTH;
  p2.y = HUB_HEIGHT;
  rs_add_fill_rect(store, &p1, &p2, &computer_color);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

/* draw the 8 points (f*ckin lame) */

  p1.x = HUB_WIDTH * 0.1;
  p1.y = HUB_HEIGHT * 0.5;
  p2.x = HUB_WIDTH * 0.15;
  p2.y = HUB_HEIGHT * 0.66;
  rs_add_fill_rect(store, &p1, &p2, &color_black);
  rs_add_draw_rect(store, &p1, &p2, &color_black);
  p1.x = HUB_WIDTH * 0.2;
  p1.y = HUB_HEIGHT * 0.5;
  p2.x = HUB_WIDTH * 0.25;
  p2.y = HUB_HEIGHT * 0.66;
  rs_add_fill_rect(store, &p1, &p2, &color_black);
  rs_add_draw_rect(store, &p1, &p2, &color_black);
  p1.x = HUB_WIDTH * 0.3;
  p1.y = HUB_HEIGHT * 0.5;
  p2.x = HUB_WIDTH * 0.35;
  p2.y = HUB_HEIGHT * 0.66;
  rs_add_fill_rect(store, &p1, &p2, &color_black);
  rs_add_draw_rect(store, &p1, &p2, &color_black);
  p1.x = HUB_WIDTH * 0.4;
  p1.y = HUB_HEIGHT * 0.5;
  p2.x = HUB_WIDTH * 0.45;
  p2.y = HUB_HEIGHT * 0.66;
  rs_add_fill_rect(store, &p1, &p2, &color_black);
  rs_add_draw_rect(store, &p1, &p2, &color_black);
  p1.x = HUB_WIDTH * 0.5;
  p1.y = HUB_HEIGHT * 0.5;
  p2.x = HUB_WIDTH * 0.55;
  p2.y = HUB_HEIGHT * 0.66;
  rs_add_fill_rect(store, &p1, &p2, &color_black);
  rs_add_draw_rect(store, &p1, &p2, &color_black);
  p1.x = HUB_WIDTH * 0.6;
  p1.y = HUB_HEIGHT * 0.5;
  p2.x = HUB_WIDTH * 0.65;
  p2.y = HUB_HEIGHT * 0.66;
  rs_add_fill_rect(store, &p1, &p2, &color_black);
  rs_add_draw_rect(store, &p1, &p2, &color_black);
  p1.x = HUB_WIDTH * 0.7;
  p1.y = HUB_HEIGHT * 0.5;
  p2.x = HUB_WIDTH * 0.75;
  p2.y = HUB_HEIGHT * 0.66;
  rs_add_fill_rect(store, &p1, &p2, &color_black);
  rs_add_draw_rect(store, &p1, &p2, &color_black);
  p1.x = HUB_WIDTH * 0.8;
  p1.y = HUB_HEIGHT * 0.5;
  p2.x = HUB_WIDTH * 0.85;
  p2.y = HUB_HEIGHT * 0.66;
  rs_add_fill_rect(store, &p1, &p2, &color_black);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

    
  points = g_new(Point, 9);
  points[0].x = HUB_WIDTH * 0.5;
  points[0].y =
    HUB_HEIGHT + HUB_UNDER1 + HUB_UNDER2;
  points[1].x = HUB_WIDTH * 0.125;
  points[1].y = HUB_HEIGHT * 0.58;
  points[2].x = HUB_WIDTH * 0.225;
  points[2].y = HUB_HEIGHT * 0.58;
  points[3].x = HUB_WIDTH * 0.325;
  points[3].y = HUB_HEIGHT * 0.58;
  points[4].x = HUB_WIDTH * 0.425;
  points[4].y = HUB_HEIGHT * 0.58;
  points[5].x = HUB_WIDTH * 0.525;
  points[5].y = HUB_HEIGHT * 0.58;
  points[6].x = HUB_WIDTH * 0.625;
  points[6].y = HUB_HEIGHT * 0.58;
  points[7].x = HUB_WIDTH * 0.725;
  points[7].y = HUB_HEIGHT * 0.58;
  points[8].x = HUB_WIDTH * 0.825;
  points[8].y = HUB_HEIGHT * 0.58;
    
  hub_desc.connection_points = points;
  hub_desc.num_connection_points = 9;
  hub_desc.store = store;
}

static Object *
hub_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  if (hub_desc.store == NULL) {
    render_to_store();
  }

  return new_render_object(startpoint, handle1, handle2,
			   &hub_desc);
}

static void
hub_save(RenderObject *hub, ObjectNode obj_node,
	     const char *filename)
{
  render_object_save(hub, obj_node);
}

static Object *
hub_load(ObjectNode obj_node, int version, const char *filename)
{
  if (hub_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &hub_desc);
}
