/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
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
#include <stdio.h>

#include <glib.h>

#include "object.h"
#include "connection.h"
#include "diarenderer.h"
#include "handle.h"
#include "arrows.h"
#include "properties.h"
#include "attributes.h"

#include "pixmaps/message.xpm"

#include "uml.h"

typedef struct _Message Message;

typedef enum {
    MESSAGE_CALL,
    MESSAGE_CREATE,
    MESSAGE_DESTROY,
    MESSAGE_SIMPLE,
    MESSAGE_RETURN,
    MESSAGE_SEND, /* Asynchronous */
    MESSAGE_RECURSIVE
} MessageType;

struct _Message {
  Connection connection;

  Handle text_handle;

  gchar *text;
  Point text_pos;
  real text_width;

  Color text_color;
  Color line_color;

  DiaFont *font;
  real     font_height;
  real     line_width;

  MessageType type;
};

#define MESSAGE_DASHLEN 0.4
#define MESSAGE_ARROWLEN (message->font_height)
#define MESSAGE_ARROWWIDTH (message->font_height*5./8.)
#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM1)


static DiaObjectChange* message_move_handle(Message *message, Handle *handle,
					 Point *to, ConnectionPoint *cp,
					 HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* message_move(Message *message, Point *to);
static void message_select(Message *message, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void message_draw(Message *message, DiaRenderer *renderer);
static DiaObject *message_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real message_distance_from(Message *message, Point *point);
static void message_update_data(Message *message);
static void message_destroy(Message *message);
static DiaObject *message_load(ObjectNode obj_node, int version,DiaContext *ctx);

static PropDescription *message_describe_props(Message *mes);
static void message_get_props(Message * message, GPtrArray *props);
static void message_set_props(Message *message, GPtrArray *props);

static ObjectTypeOps message_type_ops =
{
  (CreateFunc) message_create,
  (LoadFunc)   message_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType message_type =
{
  "UML - Message", /* name */
  0,            /* version */
  message_xpm,   /* pixmap */
  &message_type_ops /* ops */
};

static ObjectOps message_ops = {
  (DestroyFunc)         message_destroy,
  (DrawFunc)            message_draw,
  (DistanceFunc)        message_distance_from,
  (SelectFunc)          message_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            message_move,
  (MoveHandleFunc)      message_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   message_describe_props,
  (GetPropsFunc)        message_get_props,
  (SetPropsFunc)        message_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropEnumData prop_message_type_data[] = {
  { N_("Call"), MESSAGE_CALL },
  { N_("Create"), MESSAGE_CREATE },
  { N_("Destroy"), MESSAGE_DESTROY },
  { N_("Simple"), MESSAGE_SIMPLE },
  { N_("Return"), MESSAGE_RETURN },
  { N_("Send"), MESSAGE_SEND },
  { N_("Recursive"), MESSAGE_RECURSIVE },
  { NULL, 0}
};

static PropDescription message_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  /* how it used to be before 0.96+SVN */
  { "text", PROP_TYPE_STRING, PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL, N_("Message:"), NULL, NULL },
  /* new name matching "same name, same type"  rule */
  { "message", PROP_TYPE_STRING, PROP_FLAG_NO_DEFAULTS|PROP_FLAG_LOAD_ONLY|PROP_FLAG_OPTIONAL, N_("Message:"), NULL, NULL },
  { "type", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
    N_("Message type:"), NULL, prop_message_type_data },
  /* can't use PROP_STD_TEXT_COLOUR_OPTIONAL cause it has PROP_FLAG_DONT_SAVE. It is designed to fill the Text object - not some subset */
  PROP_STD_TEXT_FONT_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_TEXT_HEIGHT_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_TEXT_COLOUR_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  { "text_pos", PROP_TYPE_POINT, 0,
    "text_pos:", NULL,NULL },
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_DESC_END
};

static PropDescription *
message_describe_props(Message *mes)
{
  if (message_props[0].quark == 0)
    prop_desc_list_calculate_quarks(message_props);
  return message_props;

}

static PropOffset message_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  /* backward compatibility */
  { "text", PROP_TYPE_STRING, offsetof(Message, text) },
  /* new name matching "same name, same type"  rule */
  { "message", PROP_TYPE_STRING, offsetof(Message, text) },
  { "type", PROP_TYPE_ENUM, offsetof(Message, type) },
  { "text_font", PROP_TYPE_FONT, offsetof(Message, font) },
  { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof(Message, font_height) },
  { "text_colour",PROP_TYPE_COLOUR,offsetof(Message,text_color) },
  { "text_pos", PROP_TYPE_POINT, offsetof(Message,text_pos) },
  { PROP_STDNAME_LINE_WIDTH,PROP_TYPE_LENGTH,offsetof(Message, line_width) },
  { "line_colour",PROP_TYPE_COLOUR,offsetof(Message,line_color) },
  { NULL, 0, 0 }
};

static void
message_get_props(Message * message, GPtrArray *props)
{
  object_get_props_from_offsets(&message->connection.object,
                                message_offsets, props);
}

static void
message_set_props(Message *message, GPtrArray *props)
{
  object_set_props_from_offsets(&message->connection.object,
                                message_offsets, props);
  message_update_data(message);
}


static real
message_distance_from(Message *message, Point *point)
{
  Point *endpoints;
  real dist;

  endpoints = &message->connection.endpoints[0];

  dist = distance_line_point(&endpoints[0], &endpoints[1], message->line_width, point);

  return dist;
}

static void
message_select(Message *message, Point *clicked_point,
	    DiaRenderer *interactive_renderer)
{
  connection_update_handles(&message->connection);
}


static DiaObjectChange *
message_move_handle (Message          *message,
                     Handle           *handle,
                     Point            *to,
                     ConnectionPoint  *cp,
                     HandleMoveReason  reason,
                     ModifierKeys      modifiers)
{
  Point p1, p2;
  Point *endpoints;

  g_return_val_if_fail (message != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    message->text_pos = *to;
  } else  {
    endpoints = &message->connection.endpoints[0];
    p1.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p1.y = 0.5*(endpoints[0].y + endpoints[1].y);
    connection_move_handle(&message->connection, handle->id, to, cp, reason, modifiers);
    connection_adjust_for_autogap(&message->connection);
    p2.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p2.y = 0.5*(endpoints[0].y + endpoints[1].y);
    point_sub(&p2, &p1);
    point_add(&message->text_pos, &p2);
  }

  message_update_data(message);

  return NULL;
}

static DiaObjectChange*
message_move(Message *message, Point *to)
{
  Point start_to_end;
  Point *endpoints = &message->connection.endpoints[0];
  Point delta;

  delta = *to;
  point_sub(&delta, &endpoints[0]);

  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  point_add(&message->text_pos, &delta);

  message_update_data(message);

  return NULL;
}


static void
message_draw (Message *message, DiaRenderer *renderer)
{
  Point *endpoints, p1, p2, px;
  Arrow arrow;
  int n1 = 1, n2 = 0;
  char *mname = NULL;

  g_return_if_fail (message != NULL);
  g_return_if_fail (renderer != NULL);

  if (message->type==MESSAGE_SEND)
    arrow.type = ARROW_HALF_HEAD;
  else if (message->type==MESSAGE_SIMPLE)
    arrow.type = ARROW_LINES;
  else
    arrow.type = ARROW_FILLED_TRIANGLE;
  arrow.length = MESSAGE_ARROWLEN;
  arrow.width = MESSAGE_ARROWWIDTH;

  endpoints = &message->connection.endpoints[0];

  dia_renderer_set_linewidth (renderer, message->line_width);

  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  if (message->type==MESSAGE_RECURSIVE) {
    n1 = 0;
    n2 = 1;
  }

  if (message->type==MESSAGE_RETURN) {
    dia_renderer_set_linestyle (renderer,
                                DIA_LINE_STYLE_DASHED,
                                MESSAGE_DASHLEN);
    n1 = 0;
    n2 = 1;
  } else {
    dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  }
  p1 = endpoints[n1];
  p2 = endpoints[n2];

  if (message->type==MESSAGE_RECURSIVE) {
    px.x = p2.x;
    px.y = p1.y;
    dia_renderer_draw_line (renderer,
                            &p1,
                            &px,
                            &message->line_color);

    dia_renderer_draw_line (renderer,
                            &px,
                            &p2,
                            &message->line_color);
    p1.y = p2.y;
  }

  dia_renderer_draw_line_with_arrows (renderer,
                                      &p1,
                                      &p2,
                                      message->line_width,
                                      &message->line_color,
                                      &arrow,
                                      NULL);

  dia_renderer_set_font (renderer,
                         message->font,
                         message->font_height);

  if (message->type==MESSAGE_CREATE)
    mname = g_strdup_printf ("%s%s%s", UML_STEREOTYPE_START, "create", UML_STEREOTYPE_END);
  else if (message->type==MESSAGE_DESTROY)
    mname = g_strdup_printf ("%s%s%s", UML_STEREOTYPE_START, "destroy", UML_STEREOTYPE_END);
  else
    mname = message->text;

  if (mname && strlen (mname) != 0) {
    dia_renderer_draw_string (renderer,
                              mname, /*message->text,*/
                              &message->text_pos,
                              DIA_ALIGN_CENTRE,
                              &message->text_color);
  }

  if (message->type == MESSAGE_CREATE ||
      message->type == MESSAGE_DESTROY) {
    g_clear_pointer (&mname, g_free);
  }
}

static DiaObject *
message_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Message *message;
  Connection *conn;
  LineBBExtras *extra;
  DiaObject *obj;

  message = g_new0 (Message, 1);

  /* old defaults */
  message->font_height = 0.8;
  message->font =
      dia_font_new_from_style (DIA_FONT_SANS, message->font_height);
  message->line_width = 0.1;

  conn = &message->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  conn->endpoints[1].x += 1.5;

  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &message_type;
  obj->ops = &message_ops;

  connection_init(conn, 3, 0);

  message->text_color = color_black;
  message->line_color = attributes_get_foreground();
  message->text = g_strdup("");
  message->text_width = 0.0;
  message->text_pos.x = 0.5*(conn->endpoints[0].x + conn->endpoints[1].x);
  message->text_pos.y = 0.5*(conn->endpoints[0].y + conn->endpoints[1].y) + 0.5;

  message->text_handle.id = HANDLE_MOVE_TEXT;
  message->text_handle.type = HANDLE_MINOR_CONTROL;
  message->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  message->text_handle.connected_to = NULL;
  obj->handles[2] = &message->text_handle;

  extra->start_long =
    extra->start_trans =
    extra->end_long = message->line_width/2.0;
  extra->end_trans = MAX(message->line_width,MESSAGE_ARROWLEN)/2.0;

  message_update_data(message);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return &message->connection.object;
}


static void
message_destroy (Message *message)
{
  connection_destroy (&message->connection);

  g_clear_pointer (&message->text, g_free);
}


static void
message_update_data(Message *message)
{
  Connection *conn = &message->connection;
  DiaObject *obj = &conn->object;
  DiaRectangle rect;

  if (connpoint_is_autogap(conn->endpoint_handles[0].connected_to) ||
      connpoint_is_autogap(conn->endpoint_handles[1].connected_to)) {
    connection_adjust_for_autogap(conn);
  }
  obj->position = conn->endpoints[0];

  message->text_handle.pos = message->text_pos;

  connection_update_handles(conn);
  connection_update_boundingbox(conn);

  message->text_width =
    dia_font_string_width(message->text, message->font, message->font_height);

  /* Add boundingbox for text: */
  rect.left = message->text_pos.x-message->text_width/2;
  rect.right = rect.left + message->text_width;
  rect.top = message->text_pos.y -
      dia_font_ascent(message->text, message->font, message->font_height);
  rect.bottom = rect.top + message->font_height;
  rectangle_union(&obj->bounding_box, &rect);
}


static DiaObject *
message_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&message_type,
                                      obj_node,version,ctx);
}



