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

#include <config.h>
#include "intl.h"
#include "render_object.h"

#include "network.h"

#include "pixmaps/modem.xpm"

static Object *modem_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void modem_save(RenderObject *modem, ObjectNode obj_node,
			 const char *filename);
static Object *modem_load(ObjectNode obj_node, int version,
			    const char *filename);

static ObjectTypeOps modem_type_ops =
{
  (CreateFunc) modem_create,
  (LoadFunc)   modem_load,
  (SaveFunc)   modem_save
};

ObjectType modem_type =
{
  "Network - Modem",   /* name */
  0,                     /* version */
  (char **) modem_xpm, /* pixmap */

  &modem_type_ops      /* ops */
};

#define MODEM_LINE NETWORK_GENERAL_LINEWIDTH
#define MODEM_WIDTH 9
#define MODEM_HEIGHT 2.5
#define MODEM_BORDER 0.325
#define MODEM_UNDER1 0.35
#define MODEM_UNDER2 0.35

#define MODEM_BOTTOM (MODEM_HEIGHT+MODEM_UNDER1+MODEM_UNDER2)
RenderObjectDescriptor modem_desc = {
  NULL,                            /* store */
  { MODEM_WIDTH*0.5,
    MODEM_BOTTOM },              /* move_point */
  MODEM_WIDTH,                   /* width */
  MODEM_BOTTOM,                  /* height */
  MODEM_LINE / 2.0,              /* extra_border */

  TRUE,                            /* use_text */
  { MODEM_WIDTH*0.5,
    MODEM_BOTTOM + 0.1 },              /* text_pos */
  ALIGN_CENTER,                    /* initial alignment */
  NULL,                            /* initial font, set in render_to_store() */
  1.0,                             /* initial font height */

  NULL,         /* connection_points */
  0,            /* num_connection_points */
  
  &modem_type
};


static void render_to_store(void)
{
  RenderStore *store;
  Point p1, p2;
  Point *points;

  /* choose default font name for your locale. see also font_data structure
     in lib/font.c. if "Courier" works for you, it would be better.  */
  modem_desc.initial_font = font_getfont (_("Courier"));
  
  store = new_render_store();

  rs_add_set_linewidth(store, MODEM_LINE * 2 );
  rs_add_set_linejoin(store, LINEJOIN_MITER);
  rs_add_set_linestyle(store, LINESTYLE_SOLID);
    
/* the outer parts */

  p1.x = 0.00;
  p1.y = MODEM_HEIGHT * 0.2;
  p2.x = MODEM_WIDTH * 0.22;
  p2.y = MODEM_HEIGHT * 0.2;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = MODEM_WIDTH * 0.22;
  p1.y = MODEM_HEIGHT * 0.2;
  p2.x = MODEM_WIDTH * 0.33;
  p2.y = 0.00;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = MODEM_WIDTH * 0.33;
  p1.y = 0.00;
  p2.x = MODEM_WIDTH * 0.66;
  p2.y = 0.00;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = MODEM_WIDTH * 0.66;
  p1.y = 0.00;
  p2.x = MODEM_WIDTH * 0.77;
  p2.y = MODEM_HEIGHT * 0.2;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = MODEM_WIDTH * 0.77;
  p1.y = MODEM_HEIGHT * 0.2;
  p2.x = MODEM_WIDTH;
  p2.y = MODEM_HEIGHT * 0.2;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = MODEM_WIDTH;
  p1.y = MODEM_HEIGHT * 0.2;
  p2.x = MODEM_WIDTH;
  p2.y = MODEM_HEIGHT;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = MODEM_WIDTH;
  p1.y = MODEM_HEIGHT;
  p2.x = 0.0;
  p2.y = MODEM_HEIGHT;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  p1.x = 0.00;
  p1.y = MODEM_HEIGHT;
  p2.x = 0.00;
  p2.y = MODEM_HEIGHT * 0.2;
  rs_add_draw_line(store, &p1, &p2, &color_black);

  rs_add_set_linewidth(store, MODEM_LINE);

/* now the inner square */

  p1.x = MODEM_WIDTH * 0.05;
  p1.y = MODEM_HEIGHT * 0.3;
  p2.x = MODEM_WIDTH * 0.95;
  p2.y = MODEM_HEIGHT * 0.90;
  rs_add_fill_rect(store, &p1, &p2, &computer_color);
  rs_add_draw_rect(store, &p1, &p2, &color_black);

/* last not least the LEDs */

  p1.x = MODEM_WIDTH * 0.16;
  p1.y = MODEM_HEIGHT * 0.6;
  rs_add_fill_ellipse(store, &p1, 1, 1, &color_white);
  rs_add_draw_ellipse(store, &p1, 1, 1,  &color_black);

  p1.x = MODEM_WIDTH * 0.38;
  p1.y = MODEM_HEIGHT * 0.6;
  rs_add_fill_ellipse(store, &p1, 1, 1, &color_white);
  rs_add_draw_ellipse(store, &p1, 1, 1,  &color_black);

  p1.x = MODEM_WIDTH * 0.61;
  p1.y = MODEM_HEIGHT * 0.6;
  rs_add_fill_ellipse(store, &p1, 1, 1, &color_white);
  rs_add_draw_ellipse(store, &p1, 1, 1,  &color_black);

  p1.x = MODEM_WIDTH * 0.83;
  p1.y = MODEM_HEIGHT * 0.6;
  rs_add_fill_ellipse(store, &p1, 1, 1, &color_white);
  rs_add_draw_ellipse(store, &p1, 1, 1,  &color_black);


    
  points = g_new(Point, 1);
  points[0].x = MODEM_WIDTH * 0.5;
  points[0].y =
    MODEM_HEIGHT + MODEM_UNDER1 + MODEM_UNDER2;
    
  modem_desc.connection_points = points;
  modem_desc.num_connection_points = 1;
  modem_desc.store = store;
}

static Object *
modem_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  if (modem_desc.store == NULL) {
    render_to_store();
  }

  return new_render_object(startpoint, handle1, handle2,
			   &modem_desc);
}

static void
modem_save(RenderObject *modem, ObjectNode obj_node,
	     const char *filename)
{
  render_object_save(modem, obj_node);
}

static Object *
modem_load(ObjectNode obj_node, int version, const char *filename)
{
  if (modem_desc.store == NULL) {
    render_to_store();
  }
  return render_object_load(obj_node, &modem_desc);
}
