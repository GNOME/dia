/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Jackson diagram -  adapted by Christophe Ponsard
 * This class captures all kind of domains (given, designed, machine)
 * both for generic problems and for problem frames (ie. with domain kinds)
 *
 * based on SADT diagrams copyright (C) 2000, 2001 Cyrille Chepelov
 *
 * Forked from Flowchart toolbox -- objects for drawing flowcharts.
 * Copyright (C) 1999 James Henstridge.
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

#include "pixmaps/shared_phen.xpm"

typedef struct _Message Message;

typedef enum {
    MSG_SHARED,
    MSG_REQ,
} MessageType;

struct _Message {
  Connection connection;

  Handle text_handle;

  char *text;
  Point text_pos;
  real text_width;

  MessageType type;
};

#define MESSAGE_WIDTH 0.09
#define MESSAGE_DASHLEN 0.5
#define MESSAGE_FONTHEIGHT 0.7
#define MESSAGE_ARROWLEN 0.8
#define MESSAGE_ARROWWIDTH 0.5
#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM1)


static DiaFont *message_font = NULL;

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

DiaObjectType jackson_phenomenon_type =
{
  "Jackson - phenomenon",    /* name */
  0,                      /* version */
  jackson_shared_phen_xpm, /* pixmap */
  &message_type_ops,          /* ops */
  NULL, /* pixmap_file */
  NULL, /* default_user_data */
  NULL, /* prop_descs */
  NULL, /* prop_offsets */
  DIA_OBJECT_HAS_VARIANTS /* flags */
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
  { N_("Shared"), MSG_SHARED },
  { N_("Requirement"), MSG_REQ },
  { NULL, 0}
};

static PropDescription message_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  { "text", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Message:"), NULL, NULL },
  { "type", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE|PROP_FLAG_NO_DEFAULTS,
    N_("Type:"), NULL, prop_message_type_data },
  { "text_pos", PROP_TYPE_POINT, 0,
    "text_pos:", NULL,NULL },
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
  { "text", PROP_TYPE_STRING, offsetof(Message, text) },
  { "type", PROP_TYPE_ENUM, offsetof(Message, type) },
  { "text_pos", PROP_TYPE_POINT, offsetof(Message,text_pos) },
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

  dist = distance_line_point(&endpoints[0], &endpoints[1], MESSAGE_WIDTH, point);

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

/* drawing here -- TBD inverse flow ??  */
static void
message_draw (Message *message, DiaRenderer *renderer)
{
  Point *endpoints, p1, p2;
  Arrow arrow;
  int n1 = 1, n2 = 0;
  char *mname = g_strdup (message->text);

  /* some asserts */
  g_return_if_fail (message != NULL);
  g_return_if_fail (renderer != NULL);

  /* arrow type */
  endpoints = &message->connection.endpoints[0];

  dia_renderer_set_linewidth (renderer, MESSAGE_WIDTH);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  if (message->type==MSG_REQ) {
    dia_renderer_set_linestyle (renderer,
                                DIA_LINE_STYLE_DASHED,
                                MESSAGE_DASHLEN);
    arrow.type = ARROW_FILLED_TRIANGLE;
  } else {
    dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
    arrow.type = ARROW_NONE;
  }

  arrow.length = MESSAGE_ARROWLEN;
  arrow.width = MESSAGE_ARROWWIDTH;

  /* could reverse direction here */
  p1 = endpoints[n1];
  p2 = endpoints[n2];

  /* drawing directed line */
  dia_renderer_draw_line_with_arrows (renderer,
                                      &p1,
                                      &p2,
                                      MESSAGE_WIDTH,
                                      &color_black,
                                      &arrow,
                                      NULL);

  /* writing text on arrow (maybe not a good idea) */
  dia_renderer_set_font (renderer, message_font, MESSAGE_FONTHEIGHT);

  if (mname && strlen (mname) != 0) {
    dia_renderer_draw_string (renderer,
                              mname,
                              &message->text_pos,
                              DIA_ALIGN_CENTRE,
                              &color_black);
  }

  g_clear_pointer (&mname, g_free);
}

/* creation here */
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

  if (message_font == NULL) {
    message_font =
      dia_font_new_from_style (DIA_FONT_SANS, MESSAGE_FONTHEIGHT);
  }

  message = g_new0 (Message, 1);

  conn = &message->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  conn->endpoints[1].x += 1.5;

  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &jackson_phenomenon_type;
  obj->ops = &message_ops;

  connection_init(conn, 3, 0);

  message->text = g_strdup("");
  message->text_width = 0.0;
  message->text_pos.x = 0.5*(conn->endpoints[0].x + conn->endpoints[1].x);
  message->text_pos.y = 0.5*(conn->endpoints[0].y + conn->endpoints[1].y);

  message->text_handle.id = HANDLE_MOVE_TEXT;
  message->text_handle.type = HANDLE_MINOR_CONTROL;
  message->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  message->text_handle.connected_to = NULL;
  obj->handles[2] = &message->text_handle;

  extra->start_long = extra->start_trans =  extra->end_long = MESSAGE_WIDTH/2.0;
  extra->end_trans = MAX(MESSAGE_WIDTH,MESSAGE_ARROWLEN)/2.0;

  message_update_data(message);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];

  /* init type */
  switch (GPOINTER_TO_INT(user_data)) {
    case 1: message->type=MSG_SHARED; break;
    case 2: message->type=MSG_REQ; break;
    default: message->type=MSG_SHARED; break;
  }

  return &message->connection.object;
}

static void
message_destroy(Message *message)
{
  connection_destroy(&message->connection);

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
    dia_font_string_width(message->text, message_font, MESSAGE_FONTHEIGHT);

  /* Add boundingbox for text: */
  rect.left = message->text_pos.x-message->text_width/2;
  rect.right = rect.left + message->text_width;
  rect.top = message->text_pos.y -
      dia_font_ascent(message->text, message_font, MESSAGE_FONTHEIGHT);
  rect.bottom = rect.top + MESSAGE_FONTHEIGHT;
  rectangle_union(&obj->bounding_box, &rect);
}


static DiaObject *
message_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&jackson_phenomenon_type,
                                      obj_node,version,ctx);
}



