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

#include "pixmaps/repserver.xpm"

static Object *repserver_create(Point *startpoint,
				void *user_data,
				Handle **handle1,
				Handle **handle2);
static void repserver_save(RenderObject *repserver, ObjectNode obj_node,
			   const char *filename);
static Object *repserver_load(ObjectNode obj_node, int version,
			      const char *filename);

static ObjectTypeOps repserver_type_ops =
{
  (CreateFunc) repserver_create,
  (LoadFunc)   repserver_load,
  (SaveFunc)   repserver_save
};

ObjectType repserver_type =
{
  "Sybase - Replication Server",   /* name */
  0,                     /* version */
  (char **) repserver_xpm,    /* pixmap */

  &repserver_type_ops         /* ops */
};

#define REPSERVER_LINE SYBASE_GENERAL_LINEWIDTH
#define REPSERVER_WIDTH 2.0
#define REPSERVER_HEIGHT 2.0

RenderObjectDescriptor repserver_desc = {
  NULL,                            /* store */
  { REPSERVER_WIDTH*0.5,
    REPSERVER_HEIGHT },              /* move_point */
  REPSERVER_WIDTH,                   /* width */
  REPSERVER_HEIGHT,                  /* height */
  REPSERVER_LINE / 2.0,              /* extra_border */

  TRUE,                            /* use_text */
  { REPSERVER_WIDTH*0.5,
    REPSERVER_HEIGHT + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &repserver_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point p1, p2;
  Point *points;
  Point *poly;

  /* choose default font name for your locale. see also font_data structure
     in lib/font.c. if "Courier" works for you, it would be better.  */
  repserver_desc.initial_font = font_getfont (_("Courier"));
  
  store = new_render_store();

  rs_add_set_linewidth(store, REPSERVER_LINE);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);
    
  p1.x = 0.0;
  p1.y = 0.0;
  p2.x = REPSERVER_WIDTH;
  p2.y = REPSERVER_HEIGHT;
  rs_add_fill_rect(store, &p1, &p2, &color_white);

  p1.x = REPSERVER_WIDTH*0.5;
  p1.y = REPSERVER_HEIGHT*0.5;
  rs_add_fill_ellipse(store, &p1, REPSERVER_WIDTH, REPSERVER_HEIGHT,
                      &color_white);
  rs_add_draw_ellipse(store, &p1, REPSERVER_WIDTH, REPSERVER_HEIGHT,
                      &color_black);

  p1.x = REPSERVER_WIDTH*0.5;
  p1.y = 0.0;
  p2.x = REPSERVER_WIDTH*0.5;
  p2.y = REPSERVER_HEIGHT;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 0.0;
  p1.y = REPSERVER_HEIGHT*0.5;
  p2.x = REPSERVER_WIDTH;
  p2.y = REPSERVER_HEIGHT*0.5;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  /* top arrow */
  poly = g_new(Point, 3);
  poly[0].x=REPSERVER_WIDTH*0.5;
  poly[0].y=0.0;
  poly[1].x=REPSERVER_WIDTH*0.5-REPSERVER_HEIGHT*0.075;
  poly[1].y=REPSERVER_HEIGHT*0.15;
  poly[2].x=REPSERVER_WIDTH*0.5+REPSERVER_HEIGHT*0.075;
  poly[2].y=REPSERVER_HEIGHT*0.15;
  rs_add_fill_polygon(store, poly, 3, &color_black);
  g_free(poly);

  /* bottom arrow */
  poly = g_new(Point, 3);
  poly[0].x=REPSERVER_WIDTH*0.5;
  poly[0].y=REPSERVER_HEIGHT;
  poly[1].x=REPSERVER_WIDTH*0.5-REPSERVER_WIDTH*0.075;
  poly[1].y=REPSERVER_HEIGHT-REPSERVER_HEIGHT*0.15;
  poly[2].x=REPSERVER_WIDTH*0.5+REPSERVER_HEIGHT*0.075;
  poly[2].y=REPSERVER_HEIGHT-REPSERVER_HEIGHT*0.15;
  rs_add_fill_polygon(store, poly, 3, &color_black);
  g_free(poly);

  /* right arrow */
  poly = g_new(Point, 3);
  poly[0].x=REPSERVER_WIDTH;
  poly[0].y=REPSERVER_HEIGHT*0.5;
  poly[1].x=REPSERVER_WIDTH-REPSERVER_WIDTH*0.15;
  poly[1].y=REPSERVER_HEIGHT*0.5-REPSERVER_WIDTH*0.075;
  poly[2].x=REPSERVER_WIDTH-REPSERVER_WIDTH*0.15;
  poly[2].y=REPSERVER_HEIGHT*0.5+REPSERVER_WIDTH*0.075;
  rs_add_fill_polygon(store, poly, 3, &color_black);
  g_free(poly);

  /* left-center arrow */
  poly = g_new(Point, 3);
  poly[0].x=REPSERVER_WIDTH*0.5;
  poly[0].y=REPSERVER_HEIGHT*0.5;
  poly[1].x=REPSERVER_WIDTH*0.5-REPSERVER_WIDTH*0.15;
  poly[1].y=REPSERVER_HEIGHT*0.5-REPSERVER_WIDTH*0.075;
  poly[2].x=REPSERVER_WIDTH*0.5-REPSERVER_WIDTH*0.15;
  poly[2].y=REPSERVER_HEIGHT*0.5+REPSERVER_WIDTH*0.075;
  rs_add_fill_polygon(store, poly, 3, &color_black);
  g_free(poly);

  /* center point */
  p1.x = REPSERVER_WIDTH*0.5;
  p1.y = REPSERVER_HEIGHT*0.5;
  rs_add_fill_ellipse(store, &p1, REPSERVER_WIDTH*0.05, REPSERVER_HEIGHT*0.05,
                      &color_black);

  points = g_new(Point, 4);
  points[0].x = REPSERVER_WIDTH * 0.5;
  points[0].y = 0.0;
  points[1].x = REPSERVER_WIDTH * 0.5;
  points[1].y = REPSERVER_HEIGHT;
  points[2].x = 0.0;
  points[2].y = REPSERVER_HEIGHT * 0.5;
  points[3].x = REPSERVER_WIDTH;
  points[3].y = REPSERVER_HEIGHT * 0.5;

  repserver_desc.connection_points = points;
  repserver_desc.num_connection_points = 4;
  repserver_desc.store = store;
}

static Object *
repserver_create(Point *startpoint,
		 void *user_data,
		 Handle **handle1,
		 Handle **handle2)
{
  if (repserver_desc.store == NULL) {
    render_to_store();
  }
  return new_render_object(startpoint, handle1, handle2,
			   &repserver_desc);
}

static void
repserver_save(RenderObject *repserver, ObjectNode obj_node, const char *filename)
{
  render_object_save(repserver, obj_node);
}

static Object *
repserver_load(ObjectNode obj_node, int version, const char *filename)
{
  if (repserver_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &repserver_desc);
}
