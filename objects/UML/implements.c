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

#include <assert.h>
#include <math.h>
#include <string.h>

#include "intl.h"
#include "object.h"
#include "connection.h"
#include "render.h"
#include "handle.h"
#include "properties.h"

#include "pixmaps/implements.xpm"

typedef struct _Implements Implements;

struct _Implements {
  Connection connection;

  Handle text_handle;
  Handle circle_handle;

  real circle_diameter;

  Point circle_center; /* Calculated from diameter*/

  utfchar *text;
  Point text_pos;
  real text_width;

};
  
#define IMPLEMENTS_WIDTH 0.1
#define IMPLEMENTS_FONTHEIGHT 0.8

#define HANDLE_CIRCLE_SIZE (HANDLE_CUSTOM1)
#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM2)


static DiaFont *implements_font = NULL;

static void implements_move_handle(Implements *implements, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void implements_move(Implements *implements, Point *to);
static void implements_select(Implements *implements, Point *clicked_point,
			      Renderer *interactive_renderer);
static void implements_draw(Implements *implements, Renderer *renderer);
static Object *implements_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real implements_distance_from(Implements *implements, Point *point);
static void implements_update_data(Implements *implements);
static void implements_destroy(Implements *implements);

static PropDescription *implements_describe_props(Implements *implements);
static void implements_get_props(Implements * implements, GPtrArray *props);
static void implements_set_props(Implements * implements, GPtrArray *props);

static Object *implements_load(ObjectNode obj_node, int version,
			       const char *filename);


static ObjectTypeOps implements_type_ops =
{
  (CreateFunc) implements_create,
  (LoadFunc)   implements_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL, 
  (ApplyDefaultsFunc) NULL
};

ObjectType implements_type =
{
  "UML - Implements",   /* name */
  0,                   /* version */
  (char **) implements_xpm,  /* pixmap */
  &implements_type_ops       /* ops */
};

static ObjectOps implements_ops = {
  (DestroyFunc)         implements_destroy,
  (DrawFunc)            implements_draw,
  (DistanceFunc)        implements_distance_from,
  (SelectFunc)          implements_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            implements_move,
  (MoveHandleFunc)      implements_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   implements_describe_props,
  (GetPropsFunc)        implements_get_props,
  (SetPropsFunc)        implements_set_props
};

static PropDescription implements_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  { "text", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Interface:"), NULL, NULL },
  { "text_pos", PROP_TYPE_POINT, 0, NULL, NULL, NULL },
  { "diameter", PROP_TYPE_REAL, 0, NULL, NULL, NULL },
  PROP_DESC_END
};

static PropDescription *
implements_describe_props(Implements *implements)
{
  if (implements_props[0].quark == 0)
    prop_desc_list_calculate_quarks(implements_props);
  return implements_props;
}

static PropOffset implements_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  { "text", PROP_TYPE_STRING, offsetof(Implements, text) },
  { "text_pos", PROP_TYPE_POINT, offsetof(Implements, text_pos) },
  { "diameter", PROP_TYPE_REAL, offsetof(Implements, circle_diameter) },
  { NULL, 0, 0 }
};

static void
implements_get_props(Implements * implements, GPtrArray *props)
{
  object_get_props_from_offsets(&implements->connection.object, 
                                implements_offsets, props);
}

static void
implements_set_props(Implements *implements, GPtrArray *props)
{
  object_set_props_from_offsets(&implements->connection.object, 
                                implements_offsets, props);
  implements_update_data(implements);
}

static real
implements_distance_from(Implements *implements, Point *point)
{
  Point *endpoints;
  real dist1, dist2;
  
  endpoints = &implements->connection.endpoints[0];
  dist1 = distance_line_point( &endpoints[0], &endpoints[1],
			      IMPLEMENTS_WIDTH, point);
  dist2 = distance_point_point( &implements->circle_center, point)
    - implements->circle_diameter/2.0;
  if (dist2<0)
    dist2 = 0;
  
  return MIN(dist1, dist2);
}

static void
implements_select(Implements *implements, Point *clicked_point,
	    Renderer *interactive_renderer)
{
  connection_update_handles(&implements->connection);
}

static void
implements_move_handle(Implements *implements, Handle *handle,
		 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  Point v1, v2;
  
  assert(implements!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    implements->text_pos = *to;
  } else if (handle->id == HANDLE_CIRCLE_SIZE) {
    v1 = implements->connection.endpoints[0];
    point_sub(&v1, &implements->connection.endpoints[1]);
    point_normalize(&v1);
    v2 = *to;
    point_sub(&v2, &implements->connection.endpoints[1]);
    implements->circle_diameter = point_dot(&v1, &v2);
    if (implements->circle_diameter<0.03)
      implements->circle_diameter = 0.03;
  } else {
    v1 = implements->connection.endpoints[1];
    connection_move_handle(&implements->connection, handle->id, to, reason);
    point_sub(&v1, &implements->connection.endpoints[1]);
    point_sub(&implements->text_pos, &v1);
  }

  implements_update_data(implements);
}

static void
implements_move(Implements *implements, Point *to)
{
  Point start_to_end;
  Point delta;
  Point *endpoints = &implements->connection.endpoints[0]; 

  delta = *to;
  point_sub(&delta, &endpoints[0]);
  
  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  point_add(&implements->text_pos, &delta);
  
  implements_update_data(implements);
}

static void
implements_draw(Implements *implements, Renderer *renderer)
{
  Point *endpoints;
  
  assert(implements != NULL);
  assert(renderer != NULL);

  endpoints = &implements->connection.endpoints[0];
  
  renderer->ops->set_linewidth(renderer, IMPLEMENTS_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_line(renderer,
			   &endpoints[0], &endpoints[1],
			   &color_black);

  renderer->ops->fill_ellipse(renderer, &implements->circle_center,
			      implements->circle_diameter,
			      implements->circle_diameter,
			      &color_white);
  renderer->ops->draw_ellipse(renderer, &implements->circle_center,
			      implements->circle_diameter,
			      implements->circle_diameter,
			      &color_black);


  renderer->ops->set_font(renderer, implements_font, IMPLEMENTS_FONTHEIGHT);
  if (implements->text)
    renderer->ops->draw_string(renderer,
			       implements->text,
			       &implements->text_pos, ALIGN_LEFT,
			       &color_black);
}

static Object *
implements_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Implements *implements;
  Connection *conn;
  Object *obj;
  Point defaultlen = { 1.0, 1.0 };

  if (implements_font == NULL) {
	  /* choose default font name for your locale. see also font_data structure
	     in lib/font.c. if "Courier" works for you, it would be better.  */
	  implements_font = font_getfont (_("Courier"));
  }
  
  implements = g_malloc0(sizeof(Implements));

  conn = &implements->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);
 
  obj = &conn->object;

  obj->type = &implements_type;
  obj->ops = &implements_ops;
  
  connection_init(conn, 4, 0);

  implements->text = NULL;
  implements->text_width = 0.0;
  implements->text_pos = conn->endpoints[1];
  implements->text_pos.x -= 0.3;
  implements->circle_diameter = 0.7;

  implements->text_handle.id = HANDLE_MOVE_TEXT;
  implements->text_handle.type = HANDLE_MINOR_CONTROL;
  implements->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  implements->text_handle.connected_to = NULL;
  obj->handles[2] = &implements->text_handle;
  
  implements->circle_handle.id = HANDLE_CIRCLE_SIZE;
  implements->circle_handle.type = HANDLE_MINOR_CONTROL;
  implements->circle_handle.connect_type = HANDLE_NONCONNECTABLE;
  implements->circle_handle.connected_to = NULL;
  obj->handles[3] = &implements->circle_handle;

  implements_update_data(implements);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return &implements->connection.object;
}


static void
implements_destroy(Implements *implements)
{
  connection_destroy(&implements->connection);
  g_free(implements->text);
}

static void
implements_update_data(Implements *implements)
{
  Connection *conn = &implements->connection;
  Object *obj = &conn->object;
  LineBBExtras *extra = &conn->extra_spacing;
  Point delta;
  Point point;
  real len;
  Rectangle rect;

  implements->text_width = 0.0;
  if (implements->text)
    implements->text_width = font_string_width(implements->text,
					       implements_font,
					       IMPLEMENTS_FONTHEIGHT);

  obj->position = conn->endpoints[0];

  implements->text_handle.pos = implements->text_pos;

  /* circle handle/center pos: */
  delta = conn->endpoints[0];
  point_sub(&delta, &conn->endpoints[1]);
  len = sqrt(point_dot(&delta, &delta));
  delta.x /= len; delta.y /= len;

  point = delta;
  point_scale(&point, implements->circle_diameter);
  point_add(&point, &conn->endpoints[1]);
  implements->circle_handle.pos = point;

  point = delta;
  point_scale(&point, implements->circle_diameter/2.0);
  point_add(&point, &conn->endpoints[1]);
  implements->circle_center = point;

  connection_update_handles(conn);

  /* Boundingbox: */
  extra->start_long = 
    extra->start_trans = 
    extra->end_long = IMPLEMENTS_WIDTH/2.0;
  extra->end_trans = (IMPLEMENTS_WIDTH + implements->circle_diameter)/2.0;
  
  connection_update_boundingbox(conn);

  /* Add boundingbox for text: */
  rect.left = implements->text_pos.x;
  rect.right = rect.left + implements->text_width;
  rect.top = implements->text_pos.y - font_ascent(implements_font, IMPLEMENTS_FONTHEIGHT);
  rect.bottom = rect.top + IMPLEMENTS_FONTHEIGHT;
  rectangle_union(&obj->bounding_box, &rect);
}

static Object *
implements_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&implements_type,
                                      obj_node,version,filename);
}
