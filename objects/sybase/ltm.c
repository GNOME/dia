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

#include "config.h"
#include "intl.h"
#include "sybase.h"

#include "pixmaps/ltm.xpm"

static Object *ltm_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void ltm_save(RenderObject *ltm, ObjectNode obj_node,
		     const char *filename);
static Object *ltm_load(ObjectNode obj_node, int version,
			const char *filename);

static ObjectTypeOps ltm_type_ops =
{
  (CreateFunc) ltm_create,
  (LoadFunc)   ltm_load,
  (SaveFunc)   ltm_save
};

ObjectType ltm_type =
{
  "Sybase - Log Transfer Manager/Rep Agent",   /* name */
  0,                     /* version */
  (char **) ltm_xpm,    /* pixmap */

  &ltm_type_ops         /* ops */
};

#define LTM_LINE SYBASE_GENERAL_LINEWIDTH
#define LTM_WIDTH 2.0
#define LTM_HEIGHT 2.0

RenderObjectDescriptor ltm_desc = {
  NULL,                            /* store */
  { LTM_WIDTH*0.5,
    LTM_HEIGHT },              /* move_point */
  LTM_WIDTH,                   /* width */
  LTM_HEIGHT,                  /* height */
  LTM_LINE / 2.0,              /* extra_border */

  TRUE,                            /* use_text */
  { LTM_WIDTH*0.5,
    LTM_HEIGHT + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &ltm_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point p1, p2;
  Point *points;
  Point poly[3];
  BezPoint bez[2];

  ltm_desc.initial_font = font_getfont("Courier");
  
  store = new_render_store();

  rs_add_set_linewidth(store, LTM_LINE);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);
    
  p1.x = LTM_WIDTH*0.5;
  p1.y = LTM_HEIGHT*0.5;
  rs_add_fill_ellipse(store, &p1, LTM_WIDTH, LTM_HEIGHT,
                      &color_white);
  rs_add_draw_ellipse(store, &p1, LTM_WIDTH, LTM_HEIGHT,
                      &color_black);

  poly[0].x = LTM_WIDTH * 0.35;
  poly[0].y = LTM_HEIGHT * 0.20;
  poly[1].x = LTM_WIDTH * 0.15;
  poly[1].y = LTM_HEIGHT * 0.50;
  poly[2].x = LTM_WIDTH * 0.55;
  poly[2].y = LTM_HEIGHT * 0.50;
  rs_add_draw_polygon(store, poly, 3, &color_black);

  bez[0].type = BEZ_MOVE_TO;
  bez[0].p1.x = LTM_WIDTH  * 0.25;
  bez[0].p1.y = LTM_HEIGHT * 0.50;
  bez[1].type = BEZ_CURVE_TO;
  bez[1].p1.x = LTM_WIDTH  * 0.25;
  bez[1].p1.y = LTM_HEIGHT * 0.75;
  bez[1].p2.x = LTM_WIDTH  * 0.40;
  bez[1].p2.y = LTM_HEIGHT * 0.90;
  bez[1].p3.x = LTM_WIDTH  * 0.60;
  bez[1].p3.y = LTM_HEIGHT * 0.90;
  rs_add_draw_bezier(store, bez, 2, &color_black); 

  bez[0].type = BEZ_MOVE_TO;
  bez[0].p1.x = LTM_WIDTH  * 0.45;
  bez[0].p1.y = LTM_HEIGHT * 0.50;
  bez[1].type = BEZ_CURVE_TO;
  bez[1].p1.x = LTM_WIDTH  * 0.45;
  bez[1].p1.y = LTM_HEIGHT * 0.75;
  bez[1].p2.x = LTM_WIDTH  * 0.50;
  bez[1].p2.y = LTM_HEIGHT * 0.85;
  bez[1].p3.x = LTM_WIDTH  * 0.60;
  bez[1].p3.y = LTM_HEIGHT * 0.90;
  rs_add_draw_bezier(store, bez, 2, &color_black); 

  bez[0].type = BEZ_MOVE_TO;
  bez[0].p1.x = LTM_WIDTH  * 0.90;
  bez[0].p1.y = LTM_HEIGHT * 0.50;
  bez[1].type = BEZ_CURVE_TO;
  bez[1].p1.x = LTM_WIDTH  * 0.90;
  bez[1].p1.y = LTM_HEIGHT * 0.75;
  bez[1].p2.x = LTM_WIDTH  * 0.75;
  bez[1].p2.y = LTM_HEIGHT * 0.90;
  bez[1].p3.x = LTM_WIDTH  * 0.60;
  bez[1].p3.y = LTM_HEIGHT * 0.90;
  rs_add_draw_bezier(store, bez, 2, &color_black); 

  bez[0].type = BEZ_MOVE_TO;
  bez[0].p1.x = LTM_WIDTH  * 0.70;
  bez[0].p1.y = LTM_HEIGHT * 0.50;
  bez[1].type = BEZ_CURVE_TO;
  bez[1].p1.x = LTM_WIDTH  * 0.70;
  bez[1].p1.y = LTM_HEIGHT * 0.75;
  bez[1].p2.x = LTM_WIDTH  * 0.65;
  bez[1].p2.y = LTM_HEIGHT * 0.85;
  bez[1].p3.x = LTM_WIDTH  * 0.60;
  bez[1].p3.y = LTM_HEIGHT * 0.90;
  rs_add_draw_bezier(store, bez, 2, &color_black); 

  p1.x = LTM_WIDTH*0.70;
  p1.y = LTM_HEIGHT*0.50;
  p2.x = LTM_WIDTH*0.90;
  p2.y = LTM_HEIGHT*0.50;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  points = g_new(Point, 4);
  points[0].x = LTM_WIDTH * 0.5;
  points[0].y = 0.0;
  points[1].x = LTM_WIDTH * 0.5;
  points[1].y = LTM_HEIGHT;
  points[2].x = 0.0;
  points[2].y = LTM_HEIGHT * 0.5;
  points[3].x = LTM_WIDTH;
  points[3].y = LTM_HEIGHT * 0.5;

  ltm_desc.connection_points = points;
  ltm_desc.num_connection_points = 4;
  ltm_desc.store = store;
}

static Object *
ltm_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  if (ltm_desc.store == NULL) {
    render_to_store();
  }
  return new_render_object(startpoint, handle1, handle2,
			   &ltm_desc);
}

static void
ltm_save(RenderObject *ltm, ObjectNode obj_node, const char *filename)
{
  render_object_save(ltm, obj_node);
}

static Object *
ltm_load(ObjectNode obj_node, int version, const char *filename)
{
  if (ltm_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &ltm_desc);
}
