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
 * sCEAD-Plug by roland@support-system.com
 */

#include <config.h>
#include "intl.h"
#include "render_object.h"

#include "network.h"

#include "pixmaps/sceadplug.xpm"

static Object *sceadplug_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void sceadplug_save(RenderObject *sceadplug, ObjectNode obj_node,
			 const char *filename);
static Object *sceadplug_load(ObjectNode obj_node, int version,
			    const char *filename);

static ObjectTypeOps sceadplug_type_ops =
{
  (CreateFunc) sceadplug_create,
  (LoadFunc)   sceadplug_load,
  (SaveFunc)   sceadplug_save
};

ObjectType sceadplug_type =
{
  "Network - scEAD Wall-Plug",   /* name */
  0,                     /* version */
  (char **) sceadplug_xpm, /* pixmap */

  &sceadplug_type_ops      /* ops */
};

#define SCEADPLUG_LINE NETWORK_GENERAL_LINEWIDTH
#define SCEADPLUG_WIDTH 4.0
#define SCEADPLUG_HEIGHT 6.0
#define SCEADPLUG_BORDER 0.325

#define SCEADPLUG_BOTTOM (SCEADPLUG_HEIGHT)
RenderObjectDescriptor sceadplug_desc = {
  NULL,                            /* store */
  { SCEADPLUG_WIDTH*0.5,
    SCEADPLUG_BOTTOM },              /* move_point */
  SCEADPLUG_WIDTH,                   /* width */
  SCEADPLUG_BOTTOM,                  /* height */
  SCEADPLUG_LINE / 2.0,              /* extra_border */

  FALSE,                            /* use_text */
  { SCEADPLUG_WIDTH*0.5,
    SCEADPLUG_BOTTOM + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &sceadplug_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point p1, p2;
  Point *points;

  sceadplug_desc.initial_font = font_getfont("Courier");
  
  store = new_render_store();

  rs_add_set_linewidth(store, SCEADPLUG_LINE);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);

  /* The case of the sceadplug */

  p1.x = 0.00;
  p1.y = 0.00;
  p2.x = SCEADPLUG_WIDTH;
  p2.y = SCEADPLUG_HEIGHT;
  rs_add_fill_rect(store, &p1, &p2, &color_white);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

  p1.x = SCEADPLUG_WIDTH  * 0.125;
  p1.y = SCEADPLUG_HEIGHT * 0.08;
  p2.x = SCEADPLUG_WIDTH  * 0.875;
  p2.y = SCEADPLUG_HEIGHT * 0.92;
  rs_add_fill_rect(store, &p1, &p2, &computer_color);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

/* the small plastic-pieces */

  p1.x = SCEADPLUG_WIDTH  * 0.125;
  p1.y = SCEADPLUG_HEIGHT * 0.08;
  p2.x = SCEADPLUG_WIDTH  * 0.375;
  p2.y = SCEADPLUG_HEIGHT * 0.16;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  p1.x = SCEADPLUG_WIDTH  * 0.125;
  p1.y = SCEADPLUG_HEIGHT * 0.32;
  p2.x = SCEADPLUG_WIDTH  * 0.375;
  p2.y = SCEADPLUG_HEIGHT * 0.40;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  p1.x = SCEADPLUG_WIDTH  * 0.125;
  p1.y = SCEADPLUG_HEIGHT * 0.56;
  p2.x = SCEADPLUG_WIDTH  * 0.375;
  p2.y = SCEADPLUG_HEIGHT * 0.64;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  p1.x = SCEADPLUG_WIDTH  * 0.125;
  p1.y = SCEADPLUG_HEIGHT * 0.80;
  p2.x = SCEADPLUG_WIDTH  * 0.375;
  p2.y = SCEADPLUG_HEIGHT * 0.88;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  p1.x = SCEADPLUG_WIDTH  * 0.625;
  p1.y = SCEADPLUG_HEIGHT * 0.08;
  p2.x = SCEADPLUG_WIDTH  * 0.875;
  p2.y = SCEADPLUG_HEIGHT * 0.16;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  p1.x = SCEADPLUG_WIDTH  * 0.625;
  p1.y = SCEADPLUG_HEIGHT * 0.32;
  p2.x = SCEADPLUG_WIDTH  * 0.875;
  p2.y = SCEADPLUG_HEIGHT * 0.40;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  p1.x = SCEADPLUG_WIDTH  * 0.625;
  p1.y = SCEADPLUG_HEIGHT * 0.56;
  p2.x = SCEADPLUG_WIDTH  * 0.875;
  p2.y = SCEADPLUG_HEIGHT * 0.64;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  p1.x = SCEADPLUG_WIDTH  * 0.625;
  p1.y = SCEADPLUG_HEIGHT * 0.80;
  p2.x = SCEADPLUG_WIDTH  * 0.875;
  p2.y = SCEADPLUG_HEIGHT * 0.88;
  rs_add_fill_rect(store, &p1, &p2, &color_black);

  points = g_new(Point, 1);
  points[0].x = SCEADPLUG_WIDTH * 0.5;
  points[0].y =
    SCEADPLUG_HEIGHT;
    
  sceadplug_desc.connection_points = points;
  sceadplug_desc.num_connection_points = 1;
  sceadplug_desc.store = store;
}

static Object *
sceadplug_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  if (sceadplug_desc.store == NULL) {
    render_to_store();
  }

  return new_render_object(startpoint, handle1, handle2,
			   &sceadplug_desc);
}

static void
sceadplug_save(RenderObject *sceadplug, ObjectNode obj_node,
	     const char *filename)
{
  render_object_save(sceadplug, obj_node);
}

static Object *
sceadplug_load(ObjectNode obj_node, int version, const char *filename)
{
  if (sceadplug_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &sceadplug_desc);
}
