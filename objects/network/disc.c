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
#include "render_object.h"
#include "sheet.h"

#include "config.h"
#include "intl.h"
#include "network.h"

#include "pixmaps/disc.xpm"

static Object *disc_create(Point *startpoint,
			   void *user_data,
			   Handle **handle1,
			   Handle **handle2);
static void disc_save(RenderObject *disc, ObjectNode obj_node,
		      const char *filename);
static Object *disc_load(ObjectNode obj_node, int version,
			 const char *filename);

static ObjectTypeOps disc_type_ops =
{
  (CreateFunc) disc_create,
  (LoadFunc)   disc_load,
  (SaveFunc)   disc_save
};

ObjectType disc_type =
{
  "Network - Storage",   /* name */
  0,                     /* version */
  (char **) disc_xpm,    /* pixmap */

  &disc_type_ops         /* ops */
};

SheetObject disc_sheetobj =
{
  "Network - Storage",             /* type */
  N_("The symbol for storage. Disc or database."),  /* description */
  (char **) disc_xpm,     /* pixmap */

  NULL                    /* user_data */
};

#define DISC_LINE NETWORK_GENERAL_LINEWIDTH
#define DISC_WIDTH 2.0
#define DISC_HEIGHT 1.7
#define DISC_ELLIPSE 0.5

#define DISC_BOTTOM DISC_HEIGHT+2*DISC_ELLIPSE

RenderObjectDescriptor disc_desc = {
  NULL,                            /* store */
  { DISC_WIDTH*0.5,
    DISC_BOTTOM },              /* move_point */
  DISC_WIDTH,                   /* width */
  DISC_BOTTOM,                  /* height */
  DISC_LINE / 2.0,              /* extra_border */

  TRUE,                            /* use_text */
  { DISC_WIDTH*0.5,
    DISC_BOTTOM + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &disc_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point p1, p2;
  Point *points;

  disc_desc.initial_font = font_getfont("Courier");
  
  store = new_render_store();

  rs_add_set_linewidth(store, DISC_LINE);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);
    
  p1.x = 0.0;
  p1.y = DISC_ELLIPSE;
  p2.x = DISC_WIDTH;
  p2.y = DISC_HEIGHT+DISC_ELLIPSE;
  rs_add_fill_rect(store, &p1, &p2, &color_white);

  p1.x = 0.0;
  p1.y = DISC_ELLIPSE;
  p2.x = 0.0;
  p2.y = DISC_HEIGHT+DISC_ELLIPSE;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = DISC_WIDTH;
  p1.y = DISC_ELLIPSE;
  p2.x = DISC_WIDTH;
  p2.y = DISC_HEIGHT+DISC_ELLIPSE;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = DISC_WIDTH*0.5;;
  p1.y = DISC_ELLIPSE;
  rs_add_fill_ellipse(store, &p1, DISC_WIDTH, DISC_ELLIPSE*2,
		      &color_white);
  rs_add_draw_ellipse(store, &p1, DISC_WIDTH, DISC_ELLIPSE*2,
		      &color_black);

  p1.x = DISC_WIDTH*0.5;;
  p1.y = DISC_ELLIPSE+DISC_HEIGHT;
  rs_add_fill_arc(store, &p1, DISC_WIDTH, DISC_ELLIPSE*2,
		  180.0, 360.0,
		  &color_white);
  rs_add_draw_arc(store, &p1, DISC_WIDTH, DISC_ELLIPSE*2,
		  180.0, 360.0,
		  &color_black);

    
  points = g_new(Point, 2);
  points[0].x = DISC_WIDTH * 0.5;
  points[0].y = 0.0;

  points[1].x = DISC_WIDTH * 0.5;
  points[1].y = DISC_BOTTOM;

  disc_desc.connection_points = points;
  disc_desc.num_connection_points = 2;
  disc_desc.store = store;
}

static Object *
disc_create(Point *startpoint,
	    void *user_data,
	    Handle **handle1,
	    Handle **handle2)
{
  if (disc_desc.store == NULL) {
    render_to_store();
  }
  return new_render_object(startpoint, handle1, handle2,
			   &disc_desc);
}

static void
disc_save(RenderObject *disc, ObjectNode obj_node, const char *filename)
{
  render_object_save(disc, obj_node);
}

static Object *
disc_load(ObjectNode obj_node, int version, const char *filename)
{
  if (disc_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &disc_desc);
}
