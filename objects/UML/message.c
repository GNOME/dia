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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "object.h"
#include "connection.h"
#include "render.h"
#include "sheet.h"
#include "handle.h"
#include "arrows.h"

#include "pixmaps/message.xpm"

typedef struct _Message Message;
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

static MessageDialog *properties_dialog;

static void message_move_handle(Message *message, Handle *handle,
				   Point *to, HandleMoveReason reason);
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
static GtkWidget *message_get_properties(Message *message);
static void message_apply_properties(Message *message);
static void message_save(Message *message, ObjectNode obj_node,
			 const char *filename);
static Object *message_load(ObjectNode obj_node, int version,
			    const char *filename);


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

SheetObject message_sheetobj =
{
  "UML - Message",             /* type */
  "Create a message",
                                /* description */
  (char **) message_xpm,     /* pixmap */
  NULL                          /* user_data */
};

static ObjectOps message_ops = {
  (DestroyFunc)         message_destroy,
  (DrawFunc)            message_draw,
  (DistanceFunc)        message_distance_from,
  (SelectFunc)          message_select,
  (CopyFunc)            message_copy,
  (MoveFunc)            message_move,
  (MoveHandleFunc)      message_move_handle,
  (GetPropertiesFunc)   message_get_properties,
  (ApplyPropertiesFunc) message_apply_properties,
  (IsEmptyFunc)         object_return_false
};

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
		 Point *to, HandleMoveReason reason)
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

  if (mname) 
      renderer->ops->draw_string(renderer,
				 mname, //message->text,
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
  Object *obj;

  if (message_font == NULL)
    message_font = font_getfont("Courier");
  
  message = g_malloc(sizeof(Message));

  conn = &message->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  conn->endpoints[1].x += 1.5;
 
  obj = (Object *) message;
  
  obj->type = &message_type;
  obj->ops = &message_ops;
  
  connection_init(conn, 3, 0);

  message->text = NULL;
  message->text_width = 0.0;
  message->text_pos.x = 0.5*(conn->endpoints[0].x + conn->endpoints[1].x);
  message->text_pos.y = 0.5*(conn->endpoints[0].y + conn->endpoints[1].y);

  message->text_handle.id = HANDLE_MOVE_TEXT;
  message->text_handle.type = HANDLE_MINOR_CONTROL;
  message->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  message->text_handle.connected_to = NULL;
  obj->handles[2] = &message->text_handle;
  
  message_update_data(message);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return (Object *)message;
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
  
  newmessage = g_malloc(sizeof(Message));
  newconn = &newmessage->connection;
  newobj = (Object *) newmessage;

  connection_copy(conn, newconn);

  newmessage->text_handle = message->text_handle;
  newobj->handles[2] = &newmessage->text_handle;

  newmessage->text = (message->text) ? strdup(message->text): NULL;
  newmessage->text_pos = message->text_pos;
  newmessage->text_width = message->text_width;

  newmessage->type = message->type;

  return (Object *)newmessage;
}


static void
message_update_data(Message *message)
{
  Connection *conn = &message->connection;
  Object *obj = (Object *) message;
  Rectangle rect;
  
  obj->position = conn->endpoints[0];

  message->text_handle.pos = message->text_pos;

  connection_update_handles(conn);

  /* Boundingbox: */
  connection_update_boundingbox(conn);

  /* Add boundingbox for text: */
  rect.left = message->text_pos.x;
  rect.right = rect.left + message->text_width;
  rect.top = message->text_pos.y - font_ascent(message_font, MESSAGE_FONTHEIGHT);
  rect.bottom = rect.top + MESSAGE_FONTHEIGHT;
  rectangle_union(&obj->bounding_box, &rect);

  /* fix boundingbox for message_width and arrow: */
  obj->bounding_box.top -= MESSAGE_WIDTH/2 + MESSAGE_ARROWLEN;
  obj->bounding_box.left -= MESSAGE_WIDTH/2 + MESSAGE_ARROWLEN;
  obj->bounding_box.bottom += MESSAGE_WIDTH/2 + MESSAGE_ARROWLEN;
  obj->bounding_box.right += MESSAGE_WIDTH/2 + MESSAGE_ARROWLEN;
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
  Object *obj;

  if (message_font == NULL)
    message_font = font_getfont("Courier");

  message = g_malloc(sizeof(Message));

  conn = &message->connection;
  obj = (Object *) message;

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

  message->text_width =
      font_string_width(message->text, message_font, MESSAGE_FONTHEIGHT);

  message->text_handle.id = HANDLE_MOVE_TEXT;
  message->text_handle.type = HANDLE_MINOR_CONTROL;
  message->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  message->text_handle.connected_to = NULL;
  obj->handles[2] = &message->text_handle;
  
  message_update_data(message);
  
  return (Object *)message;
}


static void
message_apply_properties(Message *message)
{
  MessageDialog *prop_dialog;
  
  prop_dialog = properties_dialog;

  /* Read from dialog and put in object: */
  g_free(message->text);
  message->text = strdup(gtk_entry_get_text(prop_dialog->text));
    
  message->text_width =
      font_string_width(message->text, message_font,
			MESSAGE_FONTHEIGHT);
  
  
  if (GTK_TOGGLE_BUTTON(prop_dialog->m_call)->active) 
      message->type = MESSAGE_CALL;
  else if (GTK_TOGGLE_BUTTON( prop_dialog->m_return )->active) 
      message->type = MESSAGE_RETURN;
  else if (GTK_TOGGLE_BUTTON( prop_dialog->m_create )->active) 
      message->type = MESSAGE_CREATE;
  else if (GTK_TOGGLE_BUTTON( prop_dialog->m_destroy )->active) 
      message->type = MESSAGE_DESTROY;
  else if (GTK_TOGGLE_BUTTON( prop_dialog->m_send )->active) 
      message->type = MESSAGE_SEND;
  else if (GTK_TOGGLE_BUTTON( prop_dialog->m_simple )->active) 
      message->type = MESSAGE_SIMPLE;
  else if (GTK_TOGGLE_BUTTON( prop_dialog->m_recursive )->active) 
      message->type = MESSAGE_RECURSIVE;
  
  message_update_data(message);
}

static void
fill_in_dialog(Message *message)
{
  MessageDialog *prop_dialog;
  char *str;
  GtkToggleButton *button=NULL;

  prop_dialog = properties_dialog;

  if (message->text) {
      str = strdup(message->text);
      gtk_entry_set_text(prop_dialog->text, str);
      g_free(str);
  }

  switch (message->type) {
  case MESSAGE_CALL:
      button = GTK_TOGGLE_BUTTON(prop_dialog->m_call);
      break;
  case MESSAGE_CREATE:
      button = GTK_TOGGLE_BUTTON(prop_dialog->m_create);
      break;
  case MESSAGE_DESTROY:
      button = GTK_TOGGLE_BUTTON(prop_dialog->m_destroy);
      break;
  case MESSAGE_SIMPLE:
      button = GTK_TOGGLE_BUTTON(prop_dialog->m_simple);
      break;
  case MESSAGE_RETURN:
      button = GTK_TOGGLE_BUTTON(prop_dialog->m_return);
      break;
  case MESSAGE_SEND:
      button = GTK_TOGGLE_BUTTON(prop_dialog->m_send);
      break;
  case MESSAGE_RECURSIVE:
      button = GTK_TOGGLE_BUTTON(prop_dialog->m_recursive);
      break;
  }

  gtk_toggle_button_set_active(button, TRUE);
}

static GtkWidget *
message_get_properties(Message *message)
{
  MessageDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *label;
  GSList *group;

  if (properties_dialog == NULL) {

    prop_dialog = g_new(MessageDialog, 1);
    properties_dialog = prop_dialog;
    
    dialog = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = dialog;
    
    hbox = gtk_hbox_new(FALSE, 5);

    label = gtk_label_new("Message:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    entry = gtk_entry_new();
    prop_dialog->text = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    label = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (dialog), label, FALSE, TRUE, 0);
    gtk_widget_show (label);

    label = gtk_label_new("Message type:");
    gtk_box_pack_start (GTK_BOX (dialog), label, FALSE, TRUE, 0);
    gtk_widget_show (label);

    /* */
    prop_dialog->m_call = gtk_radio_button_new_with_label (NULL, "Call");
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->m_call, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->m_call);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prop_dialog->m_call), TRUE);
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (prop_dialog->m_call));
    prop_dialog->m_return = gtk_radio_button_new_with_label(group, "Return");
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->m_return, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->m_return);

    group = gtk_radio_button_group (GTK_RADIO_BUTTON (prop_dialog->m_return));
    prop_dialog->m_send = gtk_radio_button_new_with_label(group, "Asynchronous");
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->m_send, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->m_send);

    group = gtk_radio_button_group (GTK_RADIO_BUTTON (prop_dialog->m_send));
    prop_dialog->m_create = gtk_radio_button_new_with_label(group, "Create");
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->m_create, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->m_create);

    group = gtk_radio_button_group (GTK_RADIO_BUTTON (prop_dialog->m_create));
    prop_dialog->m_destroy = gtk_radio_button_new_with_label(group, "Destroy");
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->m_destroy, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->m_destroy);

    group = gtk_radio_button_group (GTK_RADIO_BUTTON (prop_dialog->m_destroy));
    prop_dialog->m_simple = gtk_radio_button_new_with_label(group, "Simple");
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->m_simple, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->m_simple);

    group = gtk_radio_button_group (GTK_RADIO_BUTTON (prop_dialog->m_simple));
    prop_dialog->m_recursive = gtk_radio_button_new_with_label(group, "Recursive");
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->m_recursive, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->m_recursive);
  }
  
  fill_in_dialog(message);
  gtk_widget_show (properties_dialog->dialog);

  return properties_dialog->dialog;
}
