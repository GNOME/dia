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

#include "pixmaps/rsm.xpm"

static Object *rsm_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void rsm_save(RenderObject *rsm, ObjectNode obj_node,
		     const char *filename);
static Object *rsm_load(ObjectNode obj_node, int version,
			const char *filename);

static ObjectTypeOps rsm_type_ops =
{
  (CreateFunc) rsm_create,
  (LoadFunc)   rsm_load,
  (SaveFunc)   rsm_save
};

ObjectType rsm_type =
{
  "Sybase - Replication Server Manager",   /* name */
  0,                     /* version */
  (char **) rsm_xpm,    /* pixmap */

  &rsm_type_ops         /* ops */
};

#define RSM_LINE SYBASE_GENERAL_LINEWIDTH
#define RSM_WIDTH 3.0
#define RSM_HEIGHT 2.25
#define RSM_BORDER 0.325
#define RSM_UNDER1 0.35
#define RSM_UNDER2 0.35
#define SCREEN_WIDTH   (RSM_WIDTH - RSM_BORDER * 2.0)
#define SCREEN_HEIGHT  (RSM_HEIGHT - RSM_BORDER * 2.0)
#define REP_WIDTH  (SCREEN_HEIGHT * 0.80)
#define REP_HEIGHT (SCREEN_HEIGHT * 0.80)
#define REP_X  (RSM_WIDTH * 0.5 - REP_WIDTH * 0.5)
#define REP_Y  (RSM_BORDER + SCREEN_HEIGHT * 0.10)

#define RSM_BOTTOM (RSM_HEIGHT+RSM_UNDER1+RSM_UNDER2)


RenderObjectDescriptor rsm_desc = {
  NULL,                            /* store */
  { RSM_WIDTH*0.5,
    RSM_BOTTOM },              /* move_point */
  RSM_WIDTH,                   /* width */
  RSM_BOTTOM,                  /* height */
  RSM_LINE / 2.0,              /* extra_border */

  TRUE,                            /* use_text */
  { RSM_WIDTH*0.5,
    RSM_BOTTOM + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &rsm_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point p1, p2;
  Point *points;
  Point poly[8];

  /* choose default font name for your locale. see also font_data structure
     in lib/font.c. if "Courier" works for you, it would be better.  */
  rsm_desc.initial_font = font_getfont (_("Courier"));
  
  store = new_render_store();

  rs_add_set_linewidth(store, RSM_LINE);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);
    
  p1.x = 0.0;
  p1.y = 0.0;
  p2.x = RSM_WIDTH;
  p2.y = RSM_HEIGHT;
  rs_add_fill_rect(store, &p1, &p2, &computer_color);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

  p1.x = RSM_BORDER;
  p1.y = RSM_BORDER;
  p2.x = RSM_WIDTH - RSM_BORDER;
  p2.y = RSM_HEIGHT - RSM_BORDER;
  rs_add_fill_rect(store, &p1, &p2, &color_white);

  p1.x = RSM_WIDTH*0.5;
  p1.y = RSM_HEIGHT*0.5;
  rs_add_draw_ellipse(store, &p1, REP_WIDTH, REP_HEIGHT,
                      &color_black);

  /* top arrow */
  poly[0].x=RSM_WIDTH*0.5;
  poly[0].y=REP_Y;
  poly[1].x=RSM_WIDTH*0.5 - REP_HEIGHT*0.075;
  poly[1].y=REP_Y + REP_HEIGHT*0.15;
  poly[2].x=RSM_WIDTH*0.5 + REP_HEIGHT*0.075;
  poly[2].y=REP_Y + REP_HEIGHT*0.15;
  rs_add_fill_polygon(store, poly, 3, &color_black);

  /* bottom arrow */
  poly[0].x=RSM_WIDTH*0.5;
  poly[0].y=REP_Y + REP_HEIGHT;
  poly[1].x=RSM_WIDTH*0.5 - REP_HEIGHT*0.075;
  poly[1].y=REP_Y + REP_HEIGHT - REP_HEIGHT*0.15;
  poly[2].x=RSM_WIDTH*0.5 + REP_HEIGHT*0.075;
  poly[2].y=REP_Y + REP_HEIGHT - REP_HEIGHT*0.15;
  rs_add_fill_polygon(store, poly, 3, &color_black);

  /* right arrow */
  poly[0].x=REP_X + REP_WIDTH;
  poly[0].y=RSM_HEIGHT * 0.5;
  poly[1].x=poly[0].x - REP_WIDTH*0.15;
  poly[1].y=poly[0].y - REP_WIDTH*0.075;
  poly[2].x=poly[0].x - REP_WIDTH*0.15;
  poly[2].y=poly[0].y + REP_WIDTH*0.075;
  rs_add_fill_polygon(store, poly, 3, &color_black);

  /* left-center arrow */
  poly[0].x=RSM_WIDTH * 0.5;
  poly[0].y=RSM_HEIGHT * 0.5;
  poly[1].x=poly[0].x - REP_WIDTH*0.15;
  poly[1].y=poly[0].y - REP_WIDTH*0.075;
  poly[2].x=poly[0].x - REP_WIDTH*0.15;
  poly[2].y=poly[0].y + REP_WIDTH*0.075;
  rs_add_fill_polygon(store, poly, 3, &color_black);

  p1.x = REP_X;
  p1.y = RSM_HEIGHT * 0.5;
  p2.x = REP_X + REP_WIDTH;
  p2.y = RSM_HEIGHT * 0.5;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = RSM_WIDTH * 0.5;
  p1.y = REP_Y;
  p2.x = RSM_WIDTH * 0.5;
  p2.y = REP_Y + REP_HEIGHT;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  poly[0].x = RSM_BORDER * 1.25;
  poly[0].y = RSM_HEIGHT;
  poly[1].x = RSM_WIDTH * 0.65;
  poly[1].y = RSM_HEIGHT;
  poly[2].x = RSM_WIDTH * 0.65;
  poly[2].y = RSM_HEIGHT + RSM_UNDER1;
  poly[3].x = RSM_BORDER * 1.50;
  poly[3].y = RSM_HEIGHT + RSM_UNDER1;
  rs_add_fill_polygon(store, poly, 4, &computer_color);
  rs_add_draw_polygon(store, poly, 4, &color_black);

  poly[0].x = RSM_WIDTH * 0.65;
  poly[0].y = RSM_HEIGHT;
  poly[1].x = RSM_WIDTH - RSM_BORDER * 1.25;
  poly[1].y = RSM_HEIGHT;
  poly[2].x = RSM_WIDTH - RSM_BORDER * 1.50;
  poly[2].y = RSM_HEIGHT + RSM_UNDER1;
  poly[3].x = RSM_WIDTH * 0.65;
  poly[3].y = RSM_HEIGHT + RSM_UNDER1;
  rs_add_fill_polygon(store, poly, 4, &computer_color);
  rs_add_draw_polygon(store, poly, 4, &color_black);

  rs_add_set_linewidth(store, 0.025);

  p1.x = RSM_WIDTH * 0.65 + RSM_UNDER1 * 0.3;
  p1.y = RSM_HEIGHT + RSM_UNDER1 * 0.3;
  p2.x = RSM_WIDTH * 0.65 + RSM_UNDER1 * 0.7;
  p2.y = RSM_HEIGHT + RSM_UNDER1 * 0.7;
  rs_add_fill_rect(store, &p1, &p2, &color_white);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

  rs_add_set_linewidth(store, RSM_LINE);

  poly[0].x = RSM_WIDTH * 0.4;
  poly[0].y = RSM_HEIGHT + RSM_UNDER1;
  poly[1].x = RSM_WIDTH * 0.6;
  poly[1].y = RSM_HEIGHT + RSM_UNDER1;
  poly[2].x = RSM_WIDTH * 0.6;
  poly[2].y = RSM_HEIGHT + RSM_UNDER1 +
    RSM_UNDER2*0.5;
  poly[3].x = RSM_WIDTH * 0.7;
  poly[3].y = RSM_HEIGHT + RSM_UNDER1 +
    RSM_UNDER2*0.5;
  poly[4].x = RSM_WIDTH * 0.7;
  poly[4].y = RSM_HEIGHT + RSM_UNDER1 +
    RSM_UNDER2;
  poly[5].x = RSM_WIDTH * 0.3;
  poly[5].y = RSM_HEIGHT + RSM_UNDER1 +
    RSM_UNDER2;
  poly[6].x = RSM_WIDTH * 0.3;
  poly[6].y = RSM_HEIGHT + RSM_UNDER1 +
    RSM_UNDER2 * 0.5;
  poly[7].x = RSM_WIDTH * 0.4;
  poly[7].y = RSM_HEIGHT + RSM_UNDER1 +
    RSM_UNDER2 * 0.5;
  rs_add_fill_polygon(store, poly, 8, &computer_color);
  rs_add_draw_polygon(store, poly, 8, &color_black);
    
  points = g_new(Point, 1);
  points[0].x = RSM_WIDTH * 0.5;
  points[0].y =
    RSM_HEIGHT + RSM_UNDER1 + RSM_UNDER2;
    
  rsm_desc.connection_points = points;
  rsm_desc.num_connection_points = 1;
  rsm_desc.store = store;
}

static Object *
rsm_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  if (rsm_desc.store == NULL) {
    render_to_store();
  }
  return new_render_object(startpoint, handle1, handle2,
			   &rsm_desc);
}

static void
rsm_save(RenderObject *rsm, ObjectNode obj_node, const char *filename)
{
  render_object_save(rsm, obj_node);
}

static Object *
rsm_load(ObjectNode obj_node, int version, const char *filename)
{
  if (rsm_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &rsm_desc);
}
