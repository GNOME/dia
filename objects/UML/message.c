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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "intl.h"
#include "object.h"
#include "connection.h"
#include "render.h"
#include "handle.h"
#include "arrows.h"
#include "properties.h"

#include "pixmaps/message.xpm"

typedef struct _Message Message;
typedef struct _MessageState MessageState;
typedef struct _MessageDialog MessageDialog;

typedef enum {
    MESSAGE_CALL,
    MESSAGE_CREATE,
    MESSAGE_DESTROY,
    MESSAGE_SIMPLE,
    MESSAGE_RETURN,
    MESSAGE_SEND, /* Asynchronous */
    MESSAGE_RECURSIVE
} MessageType;

struct _MessageState {
  ObjectState obj_state;

  char *text;
  MessageType type;
};

struct _Message {
  Connection connection;

  Handle text_handle;

  char *text;
  Point text_pos;
  real text_width;
    
  MessageType type;
};

struct _MessageDialog {
  GtkWidget *dialog;
  
  GtkEntry *text;

  GtkWidget *m_call;
  GtkWidget *m_return;
  GtkWidget *m_create;
  GtkWidget *m_destroy;
  GtkWidget *m_send;
  GtkWidget *m_simple;
  GtkWidget *m_recursive;
};
  
#define MESSAGE_WIDTH 0.1
#define MESSAGE_DASHLEN 0.4
#define MESSAGE_FONTHEIGHT 0.8
#define MESSAGE_ARROWLEN 0.8
#define MESSAGE_ARROWWIDTH 0.5
#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM1)

#define MESSAGE_CREATE_LABEL "«create»"
#define MESSAGE_DESTROY_LABEL "«destroy»"

static Font *message_font = NULL;

static void message_move_handle(Message *message, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void message_move(Message *message, Point *to);
static void message_select(Message *message, Point *clicked_point,
			      Renderer *interactive_renderer);
static void message_draw(Message *message, Renderer *renderer);
static Object *message_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real message_distance_from(Message *message, Point *point);
static void message_update_data(Message *message);
static void message_destroy(Message *message);
static Object *message_copy(Message *message);
static ObjectChange *message_apply_properties(Message *message, GtkWidget *widget);

static MessageState *message_get_state(Message *message);
static void message_set_state(Message *message,
			      MessageState *state);

static void message_save(Message *message, ObjectNode obj_node,
			 const char *filename);
static Object *message_load(ObjectNode obj_node, int version,
			    const char *filename);

static PropDescription *message_describe_props(Message *mes);
static void message_get_props(Message * message, Property *props, guint nprops);
static void message_set_props(Message *message, Property *props, guint nprops);

static ObjectTypeOps message_type_ops =
{
  (CreateFunc) message_create,
  (LoadFunc)   message_load,
  (SaveFunc)   message_save
};

ObjectType message_type =
{
  "UML - Message",        /* name */
  0,                         /* version */
  (char **) message_xpm,  /* pixmap */
  &message_type_ops       /* ops */
};

static ObjectOps message_ops = {
  (DestroyFunc)         message_destroy,
  (DrawFunc)            message_draw,
  (DistanceFunc)        message_distance_from,
  (SelectFunc)          message_select,
  (CopyFunc)            message_copy,
  (MoveFunc)            message_move,
  (MoveHandleFunc)      message_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,/*message_get_properties,*/
  (ApplyPropertiesFunc) message_apply_properties,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   message_describe_props,
  (GetPropsFunc)        message_get_props,
  (SetPropsFunc)        message_set_props
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
  OBJECT_COMMON_PROPERTIES,
  { "message", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Message:"), NULL, NULL },
  { "message_type", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
    N_("Message type:"), NULL, prop_message_type_data },
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
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { "message", PROP_TYPE_STRING, offsetof(Message, text) },
  { "message_type", PROP_TYPE_ENUM, offsetof(Message, type) },
  { NULL, 0, 0 }
};

static void
message_get_props(Message * message, Property *props, guint nprops)
{
  object_get_props_from_offsets(&message->connection.object, 
                                message_offsets, props, nprops);
}

static void
message_set_props(Message *message, Property *props, guint nprops)
{
  object_set_props_from_offsets(&message->connection.object, 
                                message_offsets, props, nprops);
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
	    Renderer *interactive_renderer)
{
  connection_update_handles(&message->connection);
}

static void
message_move_handle(Message *message, Handle *handle,
		 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  Point p1, p2;
  Point *endpoints;
  
  assert(message!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    message->text_pos = *to;
  } else  {
    endpoints = &message->connection.endpoints[0]; 
    p1.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p1.y = 0.5*(endpoints[0].y + endpoints[1].y);
    connection_move_handle(&message->connection, handle->id, to, reason);
    p2.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p2.y = 0.5*(endpoints[0].y + endpoints[1].y);
    point_sub(&p2, &p1);
    point_add(&message->text_pos, &p2);
  }

  message_update_data(message);
}

static void
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
}

static void
message_draw(Message *message, Renderer *renderer)
{
  Point *endpoints, p1, p2, px;
  ArrowType arrow_type;
  int n1 = 1, n2 = 0;
  char *mname;

  assert(message != NULL);
  assert(renderer != NULL);

  if (message->type==MESSAGE_SEND) 
      arrow_type = ARROW_HALF_HEAD;
  else if (message->type==MESSAGE_SIMPLE) 
      arrow_type = ARROW_LINES;
  else 
      arrow_type = ARROW_FILLED_TRIANGLE;
 
  endpoints = &message->connection.endpoints[0];
  
  renderer->ops->set_linewidth(renderer, MESSAGE_WIDTH);

  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  if (message->type==MESSAGE_RECURSIVE) {
      n1 = 0;
      n2 = 1;
  }

  if (message->type==MESSAGE_RETURN) {
      renderer->ops->set_dashlength(renderer, MESSAGE_DASHLEN);
      renderer->ops->set_linestyle(renderer, LINESTYLE_DASHED);
      n1 = 0;
      n2 = 1;
  } else 
      renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);

  p1 = endpoints[n1];
  p2 = endpoints[n2];

  if (message->type==MESSAGE_RECURSIVE) {
      px.x = p2.x;
      px.y = p1.y;
      renderer->ops->draw_line(renderer,
			       &p1, &px,
			       &color_black);

      renderer->ops->draw_line(renderer,
			   &px, &p2,
			   &color_black);
      p1.y = p2.y;
  } 

  renderer->ops->draw_line(renderer,
			   &p1, &p2,
			   &color_black); 

  arrow_draw(renderer, arrow_type,
	     &p1, &p2,
	     MESSAGE_ARROWLEN, MESSAGE_ARROWWIDTH, MESSAGE_WIDTH,
	     &color_black, &color_white);

  renderer->ops->set_font(renderer, message_font,
			  MESSAGE_FONTHEIGHT);

  if (message->type==MESSAGE_CREATE) 
      mname = MESSAGE_CREATE_LABEL;
  else if (message->type==MESSAGE_DESTROY) 
      mname = MESSAGE_DESTROY_LABEL;
  else
      mname = message->text;

  if (mname && strlen(mname) != 0) 
      renderer->ops->draw_string(renderer,
				 mname, /*message->text,*/
				 &message->text_pos, ALIGN_CENTER,
				 &color_black);
}

static Object *
message_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Message *message;
  Connection *conn;
  ConnectionBBExtras *extra;
  Object *obj;

  if (message_font == NULL)
    message_font = font_getfont("Helvetica");
  
  message = g_malloc0(sizeof(Message));

  conn = &message->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  conn->endpoints[1].x += 1.5;
 
  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &message_type;
  obj->ops = &message_ops;
  
  connection_init(conn, 3, 0);

  message->text = strdup("");
  message->text_width = 0.0;
  message->text_pos.x = 0.5*(conn->endpoints[0].x + conn->endpoints[1].x);
  message->text_pos.y = 0.5*(conn->endpoints[0].y + conn->endpoints[1].y);

  message->text_handle.id = HANDLE_MOVE_TEXT;
  message->text_handle.type = HANDLE_MINOR_CONTROL;
  message->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  message->text_handle.connected_to = NULL;
  obj->handles[2] = &message->text_handle;
  
  extra->start_long = 
    extra->start_trans = 
    extra->end_long = MESSAGE_WIDTH/2.0;
  extra->end_trans = MAX(MESSAGE_WIDTH,MESSAGE_ARROWLEN)/2.0;
  
  message_update_data(message);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return &message->connection.object;
}


static void
message_destroy(Message *message)
{
  connection_destroy(&message->connection);

  g_free(message->text);
}

static Object *
message_copy(Message *message)
{
  Message *newmessage;
  Connection *conn, *newconn;
  Object *newobj;
  
  conn = &message->connection;
  
  newmessage = g_malloc0(sizeof(Message));
  newconn = &newmessage->connection;
  newobj = &newconn->object;

  connection_copy(conn, newconn);

  newmessage->text_handle = message->text_handle;
  newobj->handles[2] = &newmessage->text_handle;

  newmessage->text = strdup(message->text);
  newmessage->text_pos = message->text_pos;
  newmessage->text_width = message->text_width;

  newmessage->type = message->type;

  return &newmessage->connection.object;
}

static void
message_state_free(ObjectState *ostate)
{
  MessageState *state = (MessageState *)ostate;
  g_free(state->text);
}

static MessageState *
message_get_state(Message *message)
{
  MessageState *state = g_new0(MessageState, 1);

  state->obj_state.free = message_state_free;

  state->text = g_strdup(message->text);
  state->type = message->type;

  return state;
}

static void
message_set_state(Message *message, MessageState *state)
{
  g_free(message->text);
  message->text = state->text;
  message->type = state->type;
  
  g_free(state);
  
  message_update_data(message);
}


static void
message_update_data(Message *message)
{
  Connection *conn = &message->connection;
  Object *obj = &conn->object;
  Rectangle rect;
  
  obj->position = conn->endpoints[0];

  message->text_handle.pos = message->text_pos;

  connection_update_handles(conn);
  connection_update_boundingbox(conn);

  message->text_width =
    font_string_width(message->text, message_font, MESSAGE_FONTHEIGHT);

  /* Add boundingbox for text: */
  rect.left = message->text_pos.x-message->text_width/2;
  rect.right = rect.left + message->text_width;
  rect.top = message->text_pos.y - font_ascent(message_font, MESSAGE_FONTHEIGHT);
  rect.bottom = rect.top + MESSAGE_FONTHEIGHT;
  rectangle_union(&obj->bounding_box, &rect);
}


static void
message_save(Message *message, ObjectNode obj_node, const char *filename)
{
  connection_save(&message->connection, obj_node);

  data_add_string(new_attribute(obj_node, "text"),
		  message->text);
  data_add_point(new_attribute(obj_node, "text_pos"),
		 &message->text_pos);
  data_add_int(new_attribute(obj_node, "type"),
		   message->type);
}

static Object *
message_load(ObjectNode obj_node, int version, const char *filename)
{
  Message *message;
  AttributeNode attr;
  Connection *conn;
  ConnectionBBExtras *extra;
  Object *obj;

  if (message_font == NULL)
    message_font = font_getfont("Helvetica");

  message = g_malloc0(sizeof(Message));

  conn = &message->connection;
  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &message_type;
  obj->ops = &message_ops;

  connection_load(conn, obj_node);
  
  connection_init(conn, 3, 0);

  message->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    message->text = data_string(attribute_first_data(attr));

  attr = object_find_attribute(obj_node, "text_pos");
  if (attr != NULL)
    data_point(attribute_first_data(attr), &message->text_pos);

  attr = object_find_attribute(obj_node, "type");
  if (attr != NULL)
    message->type = (MessageType)data_int(attribute_first_data(attr));

  message->text_width = 0;
  
  message->text_handle.id = HANDLE_MOVE_TEXT;
  message->text_handle.type = HANDLE_MINOR_CONTROL;
  message->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  message->text_handle.connected_to = NULL;
  obj->handles[2] = &message->text_handle;
  
  extra->start_long = 
    extra->start_trans = 
    extra->end_long = MESSAGE_WIDTH/2.0;
  extra->end_trans = MAX(MESSAGE_WIDTH,MESSAGE_ARROWLEN)/2.0;
  
  message_update_data(message);
  
  return &message->connection.object;
}


static ObjectChange *
message_apply_properties(Message *message, GtkWidget *widget)
{
  ObjectState *old_state;
  
  old_state = (ObjectState *)message_get_state(message);

  object_apply_props_from_dialog((Object *)message, widget);

  /* Read from dialog and put in object: */
    
  message_update_data(message);

  return new_object_state_change(&message->connection.object, old_state, 
				 (GetStateFunc)message_get_state,
				 (SetStateFunc)message_set_state);
}
