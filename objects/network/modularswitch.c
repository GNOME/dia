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
 * Adoption for Modularswitch by roland@support-system.com
 *
 */

#include <config.h>
#include "intl.h"
#include "render_object.h"

#include "network.h"

#include "pixmaps/modularswitch.xpm"

static Object *modularswitch_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void modularswitch_save(RenderObject *modularswitch, ObjectNode obj_node,
			 const char *filename);
static Object *modularswitch_load(ObjectNode obj_node, int version,
			    const char *filename);

static ObjectTypeOps modularswitch_type_ops =
{
  (CreateFunc) modularswitch_create,
  (LoadFunc)   modularswitch_load,
  (SaveFunc)   modularswitch_save
};

ObjectType modularswitch_type =
{
  "Network - Modular Switch",   /* name */
  0,                     /* version */
  (char **) modularswitch_xpm, /* pixmap */

  &modularswitch_type_ops      /* ops */
};

#define MODULARSWITCH_LINE NETWORK_GENERAL_LINEWIDTH
#define MODULARSWITCH_WIDTH 9.5
#define MODULARSWITCH_HEIGHT 7.5
#define MODULARSWITCH_BORDER 0.325
#define MODULARSWITCH_UNDER1 0.35
#define MODULARSWITCH_UNDER2 0.35

#define MODULARSWITCH_BOTTOM (MODULARSWITCH_HEIGHT+MODULARSWITCH_UNDER1+MODULARSWITCH_UNDER2)
RenderObjectDescriptor modularswitch_desc = {
  NULL,                            /* store */
  { MODULARSWITCH_WIDTH*0.5,
    MODULARSWITCH_BOTTOM },              /* move_point */
  MODULARSWITCH_WIDTH,                   /* width */
  MODULARSWITCH_BOTTOM,                  /* height */
  MODULARSWITCH_LINE / 2.0,              /* extra_border */

  TRUE,                            /* use_text */
  { MODULARSWITCH_WIDTH*0.5,
    MODULARSWITCH_BOTTOM + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &modularswitch_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point p1, p2;
  Point *points;
  Color col;
  
  modularswitch_desc.initial_font = font_getfont("Courier");
  
  store = new_render_store();

  rs_add_set_linewidth(store, MODULARSWITCH_LINE);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);
    
  p1.x = 0.0;
  p1.y = 0.0;
  p2.x = MODULARSWITCH_WIDTH;
  p2.y = MODULARSWITCH_HEIGHT;
  rs_add_fill_rect(store, &p1, &p2, &color_white);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

  p1.x = 0.0;
  p1.y = MODULARSWITCH_HEIGHT * 0.33;
  p2.x = MODULARSWITCH_WIDTH;
  p2.y = MODULARSWITCH_HEIGHT * 0.33;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 0.0;
  p1.y = MODULARSWITCH_HEIGHT * 0.67;
  p2.x = MODULARSWITCH_WIDTH;
  p2.y = MODULARSWITCH_HEIGHT * 0.67;
  rs_add_draw_line(store, &p1, &p2, &color_black);

/* Draw the upper "fiber" connectors */

  col.red = 1.0;
  col.green = 0.0;
  col.blue = 0.0;

  p1.x = MODULARSWITCH_WIDTH * 0.11;
  p1.y = MODULARSWITCH_HEIGHT * 0.13;
  p2.x = MODULARSWITCH_WIDTH * 0.21;
  p2.y = MODULARSWITCH_HEIGHT * 0.3;
  rs_add_fill_rect(store, &p1, &p2, &col);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

  p1.x = MODULARSWITCH_WIDTH * 0.26;
  p1.y = MODULARSWITCH_HEIGHT * 0.13;
  p2.x = MODULARSWITCH_WIDTH * 0.36;
  p2.y = MODULARSWITCH_HEIGHT * 0.3;
  rs_add_fill_rect(store, &p1, &p2, &col);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

  p1.x = MODULARSWITCH_WIDTH * 0.63;
  p1.y = MODULARSWITCH_HEIGHT * 0.13;
  p2.x = MODULARSWITCH_WIDTH * 0.74;
  p2.y = MODULARSWITCH_HEIGHT * 0.3;
  rs_add_fill_rect(store, &p1, &p2, &col);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

  p1.x = MODULARSWITCH_WIDTH * 0.79;
  p1.y = MODULARSWITCH_HEIGHT * 0.13;
  p2.x = MODULARSWITCH_WIDTH * 0.89;
  p2.y = MODULARSWITCH_HEIGHT * 0.3;
  rs_add_fill_rect(store, &p1, &p2, &col);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

/* Draw the middle "fiber" connectors */

  col.red = 1.0;
  col.green = 1.0;
  col.blue = 0.0;

  p1.x = MODULARSWITCH_WIDTH * 0.11;
  p1.y = MODULARSWITCH_HEIGHT * 0.46;
  p2.x = MODULARSWITCH_WIDTH * 0.26;
  p2.y = MODULARSWITCH_HEIGHT * 0.53;
  rs_add_fill_rect(store, &p1, &p2, &col);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

  p1.x = MODULARSWITCH_WIDTH * 0.42;
  p1.y = MODULARSWITCH_HEIGHT * 0.46;
  p2.x = MODULARSWITCH_WIDTH * 0.58;
  p2.y = MODULARSWITCH_HEIGHT * 0.53;
  rs_add_fill_rect(store, &p1, &p2, &col);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

  p1.x = MODULARSWITCH_WIDTH * 0.74;
  p1.y = MODULARSWITCH_HEIGHT * 0.46;
  p2.x = MODULARSWITCH_WIDTH * 0.89;
  p2.y = MODULARSWITCH_HEIGHT * 0.53;
  rs_add_fill_rect(store, &p1, &p2, &col);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

/* Draw the lower "copper" connectors */

  p1.x = MODULARSWITCH_WIDTH * 0.11;
  p1.y = MODULARSWITCH_HEIGHT * 0.8;
  p2.x = MODULARSWITCH_WIDTH * 0.16;
  p2.y = MODULARSWITCH_HEIGHT * 0.87;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  p1.x = MODULARSWITCH_WIDTH * 0.21;
  p1.y = MODULARSWITCH_HEIGHT * 0.8;
  p2.x = MODULARSWITCH_WIDTH * 0.26;
  p2.y = MODULARSWITCH_HEIGHT * 0.87;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  p1.x = MODULARSWITCH_WIDTH * 0.32;
  p1.y = MODULARSWITCH_HEIGHT * 0.8;
  p2.x = MODULARSWITCH_WIDTH * 0.37;
  p2.y = MODULARSWITCH_HEIGHT * 0.87;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  p1.x = MODULARSWITCH_WIDTH * 0.42;
  p1.y = MODULARSWITCH_HEIGHT * 0.8;
  p2.x = MODULARSWITCH_WIDTH * 0.47;
  p2.y = MODULARSWITCH_HEIGHT * 0.87;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  p1.x = MODULARSWITCH_WIDTH * 0.53;
  p1.y = MODULARSWITCH_HEIGHT * 0.8;
  p2.x = MODULARSWITCH_WIDTH * 0.58;
  p2.y = MODULARSWITCH_HEIGHT * 0.87;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  p1.x = MODULARSWITCH_WIDTH * 0.63;
  p1.y = MODULARSWITCH_HEIGHT * 0.8;
  p2.x = MODULARSWITCH_WIDTH * 0.68;
  p2.y = MODULARSWITCH_HEIGHT * 0.87;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  p1.x = MODULARSWITCH_WIDTH * 0.74;
  p1.y = MODULARSWITCH_HEIGHT * 0.8;
  p2.x = MODULARSWITCH_WIDTH * 0.79;
  p2.y = MODULARSWITCH_HEIGHT * 0.87;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  p1.x = MODULARSWITCH_WIDTH * 0.84;
  p1.y = MODULARSWITCH_HEIGHT * 0.8;
  p2.x = MODULARSWITCH_WIDTH * 0.89;
  p2.y = MODULARSWITCH_HEIGHT * 0.87;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

    
  points = g_new(Point, 1);
  points[0].x = MODULARSWITCH_WIDTH * 0.5;
  points[0].y =
    MODULARSWITCH_HEIGHT + MODULARSWITCH_UNDER1 + MODULARSWITCH_UNDER2;
    
  modularswitch_desc.connection_points = points;
  modularswitch_desc.num_connection_points = 1;
  modularswitch_desc.store = store;
}

static Object *
modularswitch_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  if (modularswitch_desc.store == NULL) {
    render_to_store();
  }

  return new_render_object(startpoint, handle1, handle2,
			   &modularswitch_desc);
}

static void
modularswitch_save(RenderObject *modularswitch, ObjectNode obj_node,
	     const char *filename)
{
  render_object_save(modularswitch, obj_node);
}

static Object *
modularswitch_load(ObjectNode obj_node, int version, const char *filename)
{
  if (modularswitch_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &modularswitch_desc);
}
