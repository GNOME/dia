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
#include "network.h"

#include "pixmaps/computer.xpm"

static Object *computer_create(Point *startpoint,
			       void *user_data,
			       Handle **handle1,
			       Handle **handle2);
static void computer_save(RenderObject *computer, ObjectNode obj_node,
			  const char *filename);
static Object *computer_load(ObjectNode obj_node, int version,
			     const char *filename);

static ObjectTypeOps computer_type_ops =
{
  (CreateFunc) computer_create,
  (LoadFunc)   computer_load,
  (SaveFunc)   computer_save
};

ObjectType computer_type =
{
  "Network - General Computer (Tower)",  /* name */
  0,                 /* version */
  (char **) computer_xpm, /* pixmap */

  &computer_type_ops      /* ops */
};

#define COMPUTER_WIDTH 1.5
#define COMPUTER_HEIGHT 3.5
#define COMPUTER_BOTTOM 0.30
#define COMPUTER_BAY_HEIGHT 0.40

RenderObjectDescriptor computer_desc = {
  NULL,                            /* store */
  { COMPUTER_WIDTH*0.5+COMPUTER_BOTTOM,
    COMPUTER_HEIGHT+COMPUTER_BOTTOM },                    /* move_point */
  COMPUTER_WIDTH+2*COMPUTER_BOTTOM,/* width */
  COMPUTER_HEIGHT+COMPUTER_BOTTOM, /* height */
  NETWORK_GENERAL_LINEWIDTH/2.0,   /* extra_border */

  TRUE,                            /* use_text */
  { COMPUTER_WIDTH*0.5+COMPUTER_BOTTOM,
    COMPUTER_HEIGHT+COMPUTER_BOTTOM + 0.1 },  /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */
  
  NULL,                            /* connection_points */
  0,                               /* num_connection_points */
  
  &computer_type
};

static void render_to_store(void)
{
  RenderStore *store;
  Point p1, p2;
  Point *points;
  Point poly[6];
  Color col;
  int i;

  computer_desc.initial_font = font_getfont("Courier");
  
  store = new_render_store();
  
  rs_add_set_linewidth(store, NETWORK_GENERAL_LINEWIDTH);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);
  
  p1.x = COMPUTER_BOTTOM;
  p1.y = 0.0;
  p2.x = COMPUTER_WIDTH+COMPUTER_BOTTOM;
  p2.y = COMPUTER_HEIGHT;
  
  rs_add_fill_rect(store, &p1, &p2, &computer_color);
  rs_add_draw_rect(store, &p1, &p2, &color_black);
  
  
  /* drive bays: */
  rs_add_set_linewidth(store, 0.01);
  
  for (i=0;i<4;i++) {
    p1.x = COMPUTER_WIDTH*0.1 + COMPUTER_BOTTOM;
    p1.y = COMPUTER_HEIGHT*0.06 + COMPUTER_BAY_HEIGHT*i;
    p2.x = COMPUTER_WIDTH*0.9 + COMPUTER_BOTTOM;
    p2.y = COMPUTER_HEIGHT*0.06 + COMPUTER_BAY_HEIGHT*(i+1);
    rs_add_draw_rect(store, &p1, &p2, &color_black);
  }
  
  /* small drive bay: */
  p1.x = COMPUTER_WIDTH*0.1 + COMPUTER_BOTTOM;
  p1.y = COMPUTER_HEIGHT*0.06 + COMPUTER_BAY_HEIGHT*4.2;
  p2.x = COMPUTER_WIDTH*0.6 + COMPUTER_BOTTOM;
  p2.y = COMPUTER_HEIGHT*0.06 + COMPUTER_BAY_HEIGHT*4.8;
  rs_add_draw_rect(store, &p1, &p2, &color_black);
  
  /* green led: */
  p1.x = COMPUTER_WIDTH*0.85 + COMPUTER_BOTTOM;
  p1.y = COMPUTER_HEIGHT*0.06 + COMPUTER_BAY_HEIGHT*4.3;
  col.red = 0.0;
  col.green = 1.0;
  col.blue = 0.0;
  
  rs_add_fill_ellipse(store, &p1,
		      COMPUTER_WIDTH*0.07,
		      COMPUTER_WIDTH*0.07,
		      &col);
  rs_add_draw_ellipse(store, &p1,
		      COMPUTER_WIDTH*0.07,
		      COMPUTER_WIDTH*0.07,
		      &color_black);
  
  /* yellow led: */
  p1.x = COMPUTER_WIDTH*0.85 + COMPUTER_BOTTOM;
  p1.y = COMPUTER_HEIGHT*0.06 + COMPUTER_BAY_HEIGHT*4.7;
  col.red = 1.0;
  col.green = 1.0;
  col.blue = 0.0;
  
  rs_add_fill_ellipse(store, &p1,
		      COMPUTER_WIDTH*0.07,
		      COMPUTER_WIDTH*0.07,
		      &col);
  rs_add_draw_ellipse(store, &p1,
		      COMPUTER_WIDTH*0.07,
		      COMPUTER_WIDTH*0.07,
		      &color_black);
  
  /* Power button: */
  p1.x = COMPUTER_WIDTH*0.65 + COMPUTER_BOTTOM;
  p1.y = COMPUTER_HEIGHT*0.06 + COMPUTER_BAY_HEIGHT*4.4;
  
  p2.x = COMPUTER_WIDTH*0.77 + COMPUTER_BOTTOM;
  p2.y = COMPUTER_HEIGHT*0.06 + COMPUTER_BAY_HEIGHT*4.8;
  
  rs_add_fill_rect(store, &p1, &p2, &color_white);
  rs_add_draw_rect(store, &p1, &p2, &color_black);
  
  rs_add_set_linewidth(store, NETWORK_GENERAL_LINEWIDTH);
  
  /* Nice lines at the bottom: */
  rs_add_set_linecaps(store, LINECAPS_BUTT);
  
  for (i=0;i<5;i++) {
    p1.x = COMPUTER_WIDTH/6.0*(i+1) + COMPUTER_BOTTOM;
    p1.y = COMPUTER_HEIGHT*0.70;
    p2.x = COMPUTER_WIDTH/6.0*(i+1) + COMPUTER_BOTTOM;
    p2.y = COMPUTER_HEIGHT*0.95;
    rs_add_draw_line(store, &p1, &p2, &color_black);
  }
  
  /* Stand: */
  poly[0].x = 0.0;
  poly[0].y = COMPUTER_HEIGHT + COMPUTER_BOTTOM;
  poly[1].x = COMPUTER_BOTTOM;
  poly[1].y = COMPUTER_HEIGHT - COMPUTER_BOTTOM;
  poly[2].x = COMPUTER_BOTTOM;
  poly[2].y = COMPUTER_HEIGHT;
  poly[3].x = COMPUTER_BOTTOM + COMPUTER_WIDTH;
  poly[3].y = COMPUTER_HEIGHT;
  poly[4].x = COMPUTER_BOTTOM + COMPUTER_WIDTH;
  poly[4].y = COMPUTER_HEIGHT - COMPUTER_BOTTOM;
  poly[5].x = 2*COMPUTER_BOTTOM + COMPUTER_WIDTH;
  poly[5].y = COMPUTER_HEIGHT + COMPUTER_BOTTOM;
  
  col.red = 0.6; col.green = 0.6; col.blue = 0.6;
  
  rs_add_fill_polygon(store, poly, 6, &col);
  rs_add_draw_polygon(store, poly, 6, &color_black);
  
  
  /* Connection points: */
  points = g_new(Point, 2);
  points[0].x = 0.5*(COMPUTER_WIDTH+2*COMPUTER_BOTTOM);
  points[0].y = 0.0;
  
  points[1].x = 0.5*(COMPUTER_WIDTH+2*COMPUTER_BOTTOM);
  points[1].y = COMPUTER_HEIGHT+COMPUTER_BOTTOM;
  
  computer_desc.connection_points = points;
  computer_desc.num_connection_points = 2;
  computer_desc.store = store;
}

static Object *
computer_create(Point *startpoint,
		void *user_data,
		Handle **handle1,
		Handle **handle2)
{
  if (computer_desc.store == NULL) {
    render_to_store();
  }
  
  return new_render_object(startpoint, handle1, handle2,
			   &computer_desc);
  
}

static void
computer_save(RenderObject *computer, ObjectNode obj_node,
	      const char *filename)
{
  render_object_save(computer, obj_node);
}

static Object *
computer_load(ObjectNode obj_node, int version, const char *filename)
{
  if (computer_desc.store == NULL) {
    render_to_store();
  }

  return render_object_load(obj_node, &computer_desc);
}








