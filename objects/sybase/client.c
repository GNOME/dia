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

#include "pixmaps/client.xpm"

static Object *client_create(Point *startpoint,
			     void *user_data,
			     Handle **handle1,
			     Handle **handle2);
static void client_save(RenderObject *client, ObjectNode obj_node,
			const char *filename);
static Object *client_load(ObjectNode obj_node, int version,
			   const char *filename);

static ObjectTypeOps client_type_ops =
{
  (CreateFunc) client_create,
  (LoadFunc)   client_load,
  (SaveFunc)   client_save
};

ObjectType client_type =
{
  "Sybase - Client Application",   /* name */
  0,                     /* version */
  (char **) client_xpm,    /* pixmap */

  &client_type_ops         /* ops */
};

#define CLIENT_LINE SYBASE_GENERAL_LINEWIDTH
#define CLIENT_WIDTH 2.0
#define CLIENT_HEIGHT 2.0

RenderObjectDescriptor client_desc = {
  NULL,                            /* store */
  { CLIENT_WIDTH*0.5,
    CLIENT_HEIGHT},              /* move_point */
  CLIENT_WIDTH,                   /* width */
  CLIENT_HEIGHT,                  /* height */
  CLIENT_LINE / 2.0,              /* extra_border */

  TRUE,                            /* use_text */
  { CLIENT_WIDTH*0.5,
    CLIENT_HEIGHT + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &client_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point p1;
  Point *points;
  Point *poly;

  client_desc.initial_font = font_getfont("Courier");
  
  store = new_render_store();

  rs_add_set_linewidth(store, CLIENT_LINE);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);
    
  p1.x = CLIENT_WIDTH*0.5;
  p1.y = CLIENT_HEIGHT*0.5;
  rs_add_fill_ellipse(store, &p1, CLIENT_WIDTH, CLIENT_HEIGHT,
                      &color_white);
  rs_add_draw_ellipse(store, &p1, CLIENT_WIDTH, CLIENT_HEIGHT,
                      &color_black);

  poly = g_new(Point, 3);
  poly[0].x=CLIENT_WIDTH*0.3;
  poly[0].y=CLIENT_HEIGHT*0.25;
  poly[1].x=CLIENT_WIDTH*0.3;
  poly[1].y=CLIENT_HEIGHT*0.45;
  poly[2].x=CLIENT_WIDTH*0.5;
  poly[2].y=CLIENT_HEIGHT*0.35;
  rs_add_draw_polygon(store, poly, 3, &color_black);
  g_free(poly);

  poly = g_new(Point, 3);
  poly[0].x=CLIENT_WIDTH*0.3;
  poly[0].y=CLIENT_HEIGHT*0.75;
  poly[1].x=CLIENT_WIDTH*0.3;
  poly[1].y=CLIENT_HEIGHT*0.55;
  poly[2].x=CLIENT_WIDTH*0.5;
  poly[2].y=CLIENT_HEIGHT*0.65;
  rs_add_draw_polygon(store, poly, 3, &color_black);
  g_free(poly);

  poly = g_new(Point, 3);
  poly[0].x=CLIENT_WIDTH*0.6;
  poly[0].y=CLIENT_HEIGHT*0.4;
  poly[1].x=CLIENT_WIDTH*0.6;
  poly[1].y=CLIENT_HEIGHT*0.6;
  poly[2].x=CLIENT_WIDTH*0.8;
  poly[2].y=CLIENT_HEIGHT*0.50;
  rs_add_draw_polygon(store, poly, 3, &color_black);
  g_free(poly);

  points = g_new(Point, 4);
  points[0].x = CLIENT_WIDTH * 0.5;
  points[0].y = 0.0;
  points[1].x = CLIENT_WIDTH * 0.5;
  points[1].y = CLIENT_HEIGHT;
  points[2].x = 0.0;
  points[2].y = CLIENT_HEIGHT * 0.5;
  points[3].x = CLIENT_WIDTH; 
  points[3].y = CLIENT_HEIGHT * 0.5;
  
  client_desc.connection_points = points;
  client_desc.num_connection_points = 4;
  client_desc.store = store;
}

static Object *
client_create(Point *startpoint,
	      void *user_data,
	      Handle **handle1,
	      Handle **handle2)
{
  if (client_desc.store == NULL) {
    render_to_store();
  }
  return new_render_object(startpoint, handle1, handle2,
			   &client_desc);
}

static void
client_save(RenderObject *client, ObjectNode obj_node, const char *filename)
{
  render_object_save(client, obj_node);
}

static Object *
client_load(ObjectNode obj_node, int version, const char *filename)
{
  if (client_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &client_desc);
}
