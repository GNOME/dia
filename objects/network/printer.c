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

#include "pixmaps/printer.xpm"

static Object *printer_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void printer_save(RenderObject *printer, ObjectNode obj_node,
			 const char *filename);
static Object *printer_load(ObjectNode obj_node, int version,
			    const char *filename);

static ObjectTypeOps printer_type_ops =
{
  (CreateFunc) printer_create,
  (LoadFunc)   printer_load,
  (SaveFunc)   printer_save
};

ObjectType printer_type =
{
  "Network - General Printer",   /* name */
  0,                     /* version */
  (char **) printer_xpm, /* pixmap */

  &printer_type_ops      /* ops */
};

#define PRINTER_LINE NETWORK_GENERAL_LINEWIDTH
#define PRINTER_WIDTH 3.0
#define PRINTER_HEIGHT 2.5
#define PRINTER_BORDER 0.325

#define PRINTER_BOTTOM (PRINTER_HEIGHT)
RenderObjectDescriptor printer_desc = {
  NULL,                            /* store */
  { PRINTER_WIDTH*0.5,
    PRINTER_BOTTOM },              /* move_point */
  PRINTER_WIDTH,                   /* width */
  PRINTER_BOTTOM,                  /* height */
  PRINTER_LINE / 2.0,              /* extra_border */

  FALSE,                            /* use_text */
  { PRINTER_WIDTH*0.5,
    PRINTER_BOTTOM + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &printer_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point p1, p2;
  Point *points;
  Point poly[10];
  gint a, b;
  Color col;

  printer_desc.initial_font = font_getfont("Courier");
  
  store = new_render_store();

  rs_add_set_linewidth(store, PRINTER_LINE);
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);

  /* The case of the printer */
  poly[0].x = PRINTER_WIDTH  * 0.16;
  poly[0].y = PRINTER_HEIGHT * 0.38;
  poly[1].x = PRINTER_WIDTH  * 0.12;
  poly[1].y = PRINTER_HEIGHT * 0.40;
  poly[2].x = PRINTER_WIDTH  * 0.00;
  poly[2].y = PRINTER_HEIGHT * 0.76;
  poly[3].x = PRINTER_WIDTH  * 0.00;
  poly[3].y = PRINTER_HEIGHT * 0.94;
  poly[4].x = PRINTER_WIDTH  * 0.06;
  poly[4].y = PRINTER_HEIGHT * 1.00;
  poly[5].x = PRINTER_WIDTH  * 0.94;
  poly[5].y = PRINTER_HEIGHT * 1.00;
  poly[6].x = PRINTER_WIDTH  * 1.00;
  poly[6].y = PRINTER_HEIGHT * 0.94;
  poly[7].x = PRINTER_WIDTH  * 1.00;
  poly[7].y = PRINTER_HEIGHT * 0.76;
  poly[8].x = PRINTER_WIDTH  * 0.88;
  poly[8].y = PRINTER_HEIGHT * 0.40;
  poly[9].x = PRINTER_WIDTH  * 0.84;
  poly[9].y = PRINTER_HEIGHT * 0.38;
  rs_add_fill_polygon(store, poly, 10, &computer_color);
  rs_add_draw_polygon(store, poly, 10, &color_black);

  p1.x = PRINTER_WIDTH  * 0.00;
  p1.y = PRINTER_HEIGHT * 0.76;
  p2.x = PRINTER_WIDTH  * 0.06;
  p2.y = PRINTER_HEIGHT * 0.72;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = PRINTER_WIDTH  * 0.02;
  p1.y = PRINTER_HEIGHT * 0.70;
  p2.x = PRINTER_WIDTH  * 0.06;
  p2.y = PRINTER_HEIGHT * 0.72;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = PRINTER_WIDTH  * 0.94;
  p1.y = PRINTER_HEIGHT * 0.72;
  p2.x = PRINTER_WIDTH  * 0.06;
  p2.y = PRINTER_HEIGHT * 0.72;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = PRINTER_WIDTH  * 0.94;
  p1.y = PRINTER_HEIGHT * 0.72;
  p2.x = PRINTER_WIDTH  * 0.98;
  p2.y = PRINTER_HEIGHT * 0.70;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = PRINTER_WIDTH  * 0.94;
  p1.y = PRINTER_HEIGHT * 0.72;
  p2.x = PRINTER_WIDTH  * 1.00;
  p2.y = PRINTER_HEIGHT * 0.76;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  /* The paper */
  poly[0].x = PRINTER_WIDTH  * 0.28;
  poly[0].y = PRINTER_HEIGHT * 0.00;
  poly[1].x = PRINTER_WIDTH  * 0.26;
  poly[1].y = PRINTER_HEIGHT * 0.48;
  poly[2].x = PRINTER_WIDTH  * 0.74;
  poly[2].y = PRINTER_HEIGHT * 0.48;
  poly[3].x = PRINTER_WIDTH  * 0.72;
  poly[3].y = PRINTER_HEIGHT * 0.00;
  rs_add_fill_polygon(store, poly, 4, &color_white);
  rs_add_draw_polygon(store, poly, 4, &color_black);

  /* The cover */
  poly[0].x = PRINTER_WIDTH  * 0.20;
  poly[0].y = PRINTER_HEIGHT * 0.48;
  poly[1].x = PRINTER_WIDTH  * 0.16;
  poly[1].y = PRINTER_HEIGHT * 0.62;
  poly[2].x = PRINTER_WIDTH  * 0.84;
  poly[2].y = PRINTER_HEIGHT * 0.62;
  poly[3].x = PRINTER_WIDTH  * 0.80;
  poly[3].y = PRINTER_HEIGHT * 0.48;
  rs_add_fill_polygon(store, poly, 4, &color_black);
  rs_add_draw_polygon(store, poly, 4, &color_black);

  /* A few LEDs on the front */
  rs_add_set_linewidth(store, NETWORK_GENERAL_LINEWIDTH * 0.25);
  col.red = 0.0;
  col.blue = 0.0;
  col.green = 1.00;
  for( a = 0; a < 2; a++)
  {
    for( b = 0; b < 4; b++)
    {
      p1.x = PRINTER_WIDTH  * 0.20 + a * PRINTER_WIDTH * 0.1;
      p1.y = PRINTER_HEIGHT * 0.79 + b * PRINTER_HEIGHT * 0.05;
      rs_add_fill_ellipse( store, &p1,
                           PRINTER_WIDTH * 0.02,
                           PRINTER_WIDTH * 0.02,
                           &col);
      rs_add_draw_ellipse( store, &p1,
                           PRINTER_WIDTH * 0.02,
                           PRINTER_WIDTH * 0.02,
                           &color_black);

    }
  }

  /* two buttons on the front side */
  col.red =   computer_color.red   * 0.8;
  col.blue =  computer_color.blue  * 0.8;
  col.green = computer_color.green * 0.8;
  for( a = 0; a < 2; a++)
  {
    p1.x = PRINTER_WIDTH  * 0.50 + a * PRINTER_WIDTH * 0.25;
    p1.y = PRINTER_HEIGHT * 0.80;
      rs_add_fill_ellipse( store, &p1,
                           PRINTER_WIDTH * 0.125,
                           PRINTER_WIDTH * 0.04,
                           &col);
      rs_add_draw_ellipse( store, &p1,
                           PRINTER_WIDTH * 0.125,
                           PRINTER_WIDTH * 0.04,
                           &color_black);

  }

  points = g_new(Point, 1);
  points[0].x = PRINTER_WIDTH * 0.5;
  points[0].y =
    PRINTER_HEIGHT;
    
  printer_desc.connection_points = points;
  printer_desc.num_connection_points = 1;
  printer_desc.store = store;
}

static Object *
printer_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  if (printer_desc.store == NULL) {
    render_to_store();
  }

  return new_render_object(startpoint, handle1, handle2,
			   &printer_desc);
}

static void
printer_save(RenderObject *printer, ObjectNode obj_node,
	     const char *filename)
{
  render_object_save(printer, obj_node);
}

static Object *
printer_load(ObjectNode obj_node, int version, const char *filename)
{
  if (printer_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &printer_desc);
}
