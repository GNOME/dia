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

#include "pixmaps/antenna.xpm"

static Object *antenna_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void antenna_save(RenderObject *antenna, ObjectNode obj_node,
			 const char *filename);
static Object *antenna_load(ObjectNode obj_node, int version,
			    const char *filename);

static ObjectTypeOps antenna_type_ops =
{
  (CreateFunc) antenna_create,
  (LoadFunc)   antenna_load,
  (SaveFunc)   antenna_save
};

ObjectType antenna_type =
{
  "Network - Antenna",   /* name */
  0,                     /* version */
  (char **) antenna_xpm, /* pixmap */

  &antenna_type_ops      /* ops */
};

#define ANTENNA_LINE NETWORK_GENERAL_LINEWIDTH
#define ANTENNA_WIDTH 1
#define ANTENNA_HEIGHT 3
#define ANTENNA_BORDER 0.325
#define ANTENNA_UNDER1 0.35
#define ANTENNA_UNDER2 0.35

#define ANTENNA_BOTTOM (ANTENNA_HEIGHT+ANTENNA_UNDER1+ANTENNA_UNDER2)
RenderObjectDescriptor antenna_desc = {
  NULL,                            /* store */
  { ANTENNA_WIDTH*0.5,
    ANTENNA_BOTTOM },              /* move_point */
  ANTENNA_WIDTH,                   /* width */
  ANTENNA_BOTTOM,                  /* height */
  ANTENNA_LINE / 2.0,              /* extra_border */

  TRUE,                            /* use_text */
  { ANTENNA_WIDTH*0.5,
    ANTENNA_BOTTOM + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &antenna_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point p1, p2;
  Point *points;
  Point poly[6];

  /* choose default font name for your locale. see also font_data structure
     in lib/font.c. if "Courier" works for you, it would be better.  */
  antenna_desc.initial_font = font_getfont(_("Courier"));
  
  store = new_render_store();

  rs_add_set_linewidth(store, ANTENNA_LINE);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);
    
/* the small stuff */

  p1.x = 0.00;
  p1.y = ANTENNA_HEIGHT * 0.17;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.25;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 0.00;
  p1.y = ANTENNA_HEIGHT * 0.25;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.33;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 0.00;
  p1.y = ANTENNA_HEIGHT * 0.33;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.42;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 0.00;
  p1.y = ANTENNA_HEIGHT * 0.42;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.50;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 0.00;
  p1.y = ANTENNA_HEIGHT * 0.50;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.58;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 0.00;
  p1.y = ANTENNA_HEIGHT * 0.58;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.67;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 0.00;
  p1.y = ANTENNA_HEIGHT * 0.67;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.75;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 0.00;
  p1.y = ANTENNA_HEIGHT * 0.75;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.83;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 0.00;
  p1.y = ANTENNA_HEIGHT * 0.83;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.92;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 1.00;
  p1.y = ANTENNA_HEIGHT * 0.17;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.25;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 1.00;
  p1.y = ANTENNA_HEIGHT * 0.25;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.33;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 1.00;
  p1.y = ANTENNA_HEIGHT * 0.33;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.42;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 1.00;
  p1.y = ANTENNA_HEIGHT * 0.42;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.50;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 1.00;
  p1.y = ANTENNA_HEIGHT * 0.50;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.58;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 1.00;
  p1.y = ANTENNA_HEIGHT * 0.58;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.67;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 1.00;
  p1.y = ANTENNA_HEIGHT * 0.67;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.75;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 1.00;
  p1.y = ANTENNA_HEIGHT * 0.75;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.83;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 1.00;
  p1.y = ANTENNA_HEIGHT * 0.83;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.92;
  rs_add_draw_line(store, &p1, &p2, &color_black);

/* clean unneeded */

  poly[0].x = 0.00;
  poly[0].y = 0.00;
  poly[1].x = ANTENNA_WIDTH;
  poly[1].y = 0.00;
  poly[2].x = ANTENNA_WIDTH;
  poly[2].y = ANTENNA_HEIGHT * 0.92;
  poly[3].x = ANTENNA_WIDTH  * 0.50;
  poly[3].y = ANTENNA_HEIGHT * 0.17;
  poly[4].x = 0.00;
  poly[4].y = ANTENNA_HEIGHT * 0.92;
  rs_add_fill_polygon(store, poly, 5, &color_white);

/* now the small lightning */

  p1.x = ANTENNA_WIDTH * 0.50;
  p1.y = ANTENNA_HEIGHT * 0.17;
  p2.x = ANTENNA_WIDTH * 0.44;
  p2.y = ANTENNA_HEIGHT * 0.08;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = ANTENNA_WIDTH * 0.44;
  p1.y = ANTENNA_HEIGHT * 0.08;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.125;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = ANTENNA_WIDTH * 0.50;
  p1.y = ANTENNA_HEIGHT * 0.125;
  p2.x = ANTENNA_WIDTH * 0.375;
  p2.y = 0.00;
  rs_add_draw_line(store, &p1, &p2, &color_black);



  rs_add_set_linewidth(store, ANTENNA_LINE * 2);

/* now the large outer stuff */

  poly[0].x = ANTENNA_WIDTH * 0.50;
  poly[0].y = ANTENNA_HEIGHT * 0.17;
  poly[1].x = ANTENNA_WIDTH;
  poly[1].y = ANTENNA_HEIGHT * 0.92;
  poly[2].x = ANTENNA_WIDTH * 0.5;
  poly[2].y = ANTENNA_HEIGHT;
  poly[3].x = 0.00;
  poly[3].y = ANTENNA_HEIGHT * 0.92;
  rs_add_draw_polygon(store, poly, 4, &color_black);

  p1.x = 0.00;
  p1.y = ANTENNA_HEIGHT * 0.92;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT * 0.83;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = ANTENNA_WIDTH * 0.50;
  p1.y = ANTENNA_HEIGHT * 0.83;
  p2.x = ANTENNA_WIDTH;
  p2.y = ANTENNA_HEIGHT * 0.92;
  rs_add_draw_line(store, &p1, &p2, &color_black);
    
  p1.x = ANTENNA_WIDTH * 0.50;
  p1.y = ANTENNA_HEIGHT * 0.17;
  p2.x = ANTENNA_WIDTH * 0.50;
  p2.y = ANTENNA_HEIGHT;
  rs_add_draw_line(store, &p1, &p2, &color_black);
    
  points = g_new(Point, 1);
  points[0].x = ANTENNA_WIDTH * 0.5;
  points[0].y =
    ANTENNA_HEIGHT + ANTENNA_UNDER1 + ANTENNA_UNDER2;
    
  antenna_desc.connection_points = points;
  antenna_desc.num_connection_points = 1;
  antenna_desc.store = store;
}

static Object *
antenna_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  if (antenna_desc.store == NULL) {
    render_to_store();
  }

  return new_render_object(startpoint, handle1, handle2,
			   &antenna_desc);
}

static void
antenna_save(RenderObject *antenna, ObjectNode obj_node,
	     const char *filename)
{
  render_object_save(antenna, obj_node);
}

static Object *
antenna_load(ObjectNode obj_node, int version, const char *filename)
{
  if (antenna_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &antenna_desc);
}
