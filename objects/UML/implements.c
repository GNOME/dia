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

#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>

#include "object.h"
#include "connection.h"
#include "diarenderer.h"
#include "handle.h"
#include "properties.h"
#include "attributes.h"

#include "pixmaps/implements.xpm"

typedef struct _Implements Implements;

struct _Implements {
  Connection connection;

  Handle text_handle;
  Handle circle_handle;

  real circle_diameter;

  Point circle_center; /* Calculated from diameter*/

  DiaFont *font;
  real     font_height;
  Color    text_color;

  real     line_width;
  Color    line_color;

  gchar *text;
  Point text_pos;
  real text_width;

};

#define IMPLEMENTS_WIDTH 0.1

#define HANDLE_CIRCLE_SIZE (HANDLE_CUSTOM1)
#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM2)

static DiaObjectChange* implements_move_handle(Implements *implements, Handle *handle,
					    Point *to, ConnectionPoint *cp,
					    HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* implements_move(Implements *implements, Point *to);
static void implements_select(Implements *implements, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void implements_draw(Implements *implements, DiaRenderer *renderer);
static DiaObject *implements_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real implements_distance_from(Implements *implements, Point *point);
static void implements_update_data(Implements *implements);
static void implements_destroy(Implements *implements);

static PropDescription *implements_describe_props(Implements *implements);
static void implements_get_props(Implements * implements, GPtrArray *props);
static void implements_set_props(Implements * implements, GPtrArray *props);

static DiaObject *implements_load(ObjectNode obj_node, int version,DiaContext *ctx);


static ObjectTypeOps implements_type_ops =
{
  (CreateFunc) implements_create,
  (LoadFunc)   implements_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType implements_type =
{
  "UML - Implements", /* name */
  0,               /* version */
  implements_xpm,   /* pixmap */
  &implements_type_ops /* ops */
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
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   implements_describe_props,
  (GetPropsFunc)        implements_get_props,
  (SetPropsFunc)        implements_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription implements_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  /* how it used to be before 0.96+SVN */
  { "text", PROP_TYPE_STRING, PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL, N_("Interface:"), NULL, NULL },
  /* new name matching "same name, same type"  rule - reverted, forward compatibility seems more important */
  { "name", PROP_TYPE_STRING, PROP_FLAG_NO_DEFAULTS|PROP_FLAG_LOAD_ONLY|PROP_FLAG_OPTIONAL, N_("Interface:"), NULL, NULL },
  /* can't use PROP_STD_TEXT_COLOUR_OPTIONAL cause it has PROP_FLAG_DONT_SAVE. It is designed to fill the Text object - not some subset */
  PROP_STD_TEXT_FONT_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_TEXT_HEIGHT_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_TEXT_COLOUR_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  { "text_pos", PROP_TYPE_POINT, 0, NULL, NULL, NULL },
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
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
  { "name", PROP_TYPE_STRING, offsetof(Implements, text) },
  { "text_font", PROP_TYPE_FONT, offsetof(Implements, font) },
  { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof(Implements, font_height) },
  { "text_colour",PROP_TYPE_COLOUR,offsetof(Implements, text_color) },
  { "text_pos", PROP_TYPE_POINT, offsetof(Implements, text_pos) },
  { PROP_STDNAME_LINE_WIDTH,PROP_TYPE_LENGTH,offsetof(Implements, line_width) },
  { "line_colour",PROP_TYPE_COLOUR,offsetof(Implements, line_color) },
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
			      implements->line_width, point);
  dist2 = distance_point_point( &implements->circle_center, point)
    - implements->circle_diameter/2.0;
  if (dist2<0)
    dist2 = 0;

  return MIN(dist1, dist2);
}

static void
implements_select(Implements *implements, Point *clicked_point,
	    DiaRenderer *interactive_renderer)
{
  connection_update_handles(&implements->connection);
}


static DiaObjectChange *
implements_move_handle (Implements       *implements,
                        Handle           *handle,
                        Point            *to,
                        ConnectionPoint  *cp,
                        HandleMoveReason  reason,
                        ModifierKeys      modifiers)
{
  Point v1, v2;

  g_return_val_if_fail (implements != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

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
    connection_move_handle(&implements->connection, handle->id, to, cp,
			   reason, modifiers);
    connection_adjust_for_autogap(&implements->connection);
    point_sub(&v1, &implements->connection.endpoints[1]);
    point_sub(&implements->text_pos, &v1);
  }

  implements_update_data(implements);

  return NULL;
}

static DiaObjectChange*
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

  return NULL;
}

static void
implements_draw (Implements *implements, DiaRenderer *renderer)
{
  Point *endpoints;

  g_return_if_fail (implements != NULL);
  g_return_if_fail (renderer != NULL);

  endpoints = &implements->connection.endpoints[0];

  dia_renderer_set_linewidth (renderer, implements->line_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  dia_renderer_draw_line (renderer,
                          &endpoints[0],
                          &endpoints[1],
                          &implements->line_color);
  dia_renderer_draw_ellipse (renderer,
                             &implements->circle_center,
                             implements->circle_diameter,
                             implements->circle_diameter,
                             &color_white,
                             &implements->line_color);


  dia_renderer_set_font (renderer, implements->font, implements->font_height);
  if (implements->text) {
    dia_renderer_draw_string (renderer,
                              implements->text,
                              &implements->text_pos,
                              DIA_ALIGN_LEFT,
                              &implements->text_color);
  }
}

static DiaObject *
implements_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Implements *implements;
  Connection *conn;
  DiaObject *obj;
  Point defaultlen = { 1.0, 1.0 };

  implements = g_new0 (Implements, 1);

  /* old defaults */
  implements->font_height = 0.8;
  implements->font =
      dia_font_new_from_style(DIA_FONT_MONOSPACE, implements->font_height);
  implements->line_width = 0.1;

  conn = &implements->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);

  obj = &conn->object;

  obj->type = &implements_type;
  obj->ops = &implements_ops;

  connection_init(conn, 4, 0);

  implements->line_color = attributes_get_foreground();
  implements->text_color = color_black;
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
implements_destroy (Implements *implements)
{
  connection_destroy (&implements->connection);
  g_clear_object (&implements->font);
  g_clear_pointer (&implements->text, g_free);
}

static void
implements_update_data(Implements *implements)
{
  Connection *conn = &implements->connection;
  DiaObject *obj = &conn->object;
  LineBBExtras *extra = &conn->extra_spacing;
  Point delta;
  Point point;
  real len;
  DiaRectangle rect;

  implements->text_width = 0.0;
  if (implements->text)
    implements->text_width = dia_font_string_width(implements->text,
                                                   implements->font,
                                                   implements->font_height);

  if (connpoint_is_autogap(conn->endpoint_handles[0].connected_to) ||
      connpoint_is_autogap(conn->endpoint_handles[1].connected_to)) {
    connection_adjust_for_autogap(conn);
  }
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
    extra->end_long = implements->line_width/2.0;
  extra->end_trans = (implements->line_width + implements->circle_diameter)/2.0;

  connection_update_boundingbox(conn);

  /* Add boundingbox for text: */
  rect.left = implements->text_pos.x;
  rect.right = rect.left + implements->text_width;
  rect.top = implements->text_pos.y;
  if (implements->text)
    rect.top -= dia_font_ascent(implements->text,
				implements->font,
				implements->font_height);
  rect.bottom = rect.top + implements->font_height;
  rectangle_union(&obj->bounding_box, &rect);
}

static DiaObject *
implements_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&implements_type,
                                      obj_node,version,ctx);
}
