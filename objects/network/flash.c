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

#include "config.h"
#include "intl.h"
#include "render_object.h"

#include "network.h"

#include "pixmaps/flash.xpm"

static Object *flash_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void flash_save(RenderObject *flash, ObjectNode obj_node,
			 const char *filename);
static Object *flash_load(ObjectNode obj_node, int version,
			    const char *filename);

static ObjectTypeOps flash_type_ops =
{
  (CreateFunc) flash_create,
  (LoadFunc)   flash_load,
  (SaveFunc)   flash_save
};

ObjectType flash_type =
{
  "Network - WAN Connection",   /* name */
  0,                     /* version */
  (char **) flash_xpm, /* pixmap */

  &flash_type_ops      /* ops */
};

#define FLASH_LINE NETWORK_GENERAL_LINEWIDTH
#define FLASH_WIDTH 7.0
#define FLASH_HEIGHT 11.0
#define FLASH_BORDER 0.325

#define FLASH_BOTTOM (FLASH_HEIGHT)
RenderObjectDescriptor flash_desc = {
  NULL,                            /* store */
  { FLASH_WIDTH*0.5,
    FLASH_BOTTOM },              /* move_point */
  FLASH_WIDTH,                   /* width */
  FLASH_BOTTOM,                  /* height */
  FLASH_LINE / 2.0,              /* extra_border */

  FALSE,                            /* use_text */
  { FLASH_WIDTH*0.5,
    FLASH_BOTTOM + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &flash_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point *points;
  Point poly[7];

  flash_desc.initial_font = font_getfont("Courier");
  
  store = new_render_store();

  rs_add_set_linewidth(store, FLASH_LINE);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);

  /* The case of the flash */
  poly[0].x = FLASH_WIDTH  * 0.29;
  poly[0].y = FLASH_HEIGHT * 0.09;
  poly[1].x = FLASH_WIDTH  * 0.71;
  poly[1].y = FLASH_HEIGHT * 0.09;
  poly[2].x = FLASH_WIDTH  * 0.57;
  poly[2].y = FLASH_HEIGHT * 0.32;
  poly[3].x = FLASH_WIDTH  * 0.94;
  poly[3].y = FLASH_HEIGHT * 0.32;
  poly[4].x = FLASH_WIDTH  * 0.29;
  poly[4].y = FLASH_HEIGHT * 0.95;
  poly[5].x = FLASH_WIDTH  * 0.43;
  poly[5].y = FLASH_HEIGHT * 0.50;
  poly[6].x = FLASH_WIDTH  * 0.14;
  poly[6].y = FLASH_HEIGHT * 0.50;
  rs_add_fill_polygon(store, poly, 7, &color_black);
  rs_add_draw_polygon(store, poly, 7, &color_black);

  points = g_new(Point, 1);
  points[0].x = FLASH_WIDTH * 0.5;
  points[0].y =
    FLASH_HEIGHT;
    
  flash_desc.connection_points = points;
  flash_desc.num_connection_points = 1;
  flash_desc.store = store;
}

static Object *
flash_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  if (flash_desc.store == NULL) {
    render_to_store();
  }

  return new_render_object(startpoint, handle1, handle2,
			   &flash_desc);
}

static void
flash_save(RenderObject *flash, ObjectNode obj_node,
	     const char *filename)
{
  render_object_save(flash, obj_node);
}

static Object *
flash_load(ObjectNode obj_node, int version, const char *filename)
{
  if (flash_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &flash_desc);
}
