/* AADL plugin for DIA
*
* Copyright (C) 2005 Laboratoire d'Informatique de Paris 6
* Author: Pierre Duquesne
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


/* based on uml/node.c and network/bus.c */

/* TODO:  */

/* FIXME: - automatic box resizing to fit text is broken because of new text */
/*          positionning functions                                           */

/* Explanations:

   Aadl box is the abstract class of all other container-style AADL elements.
   It provides functions to create, move, resize, delete, and to add/remove
   ports.
*/

#include "config.h"

#include <glib/gi18n-lib.h>

#include "aadl.h"
#include "edit_port_declaration.h"

#define PORT_HANDLE_AADLBOX (HANDLE_CUSTOM9)

static Aadlport *
new_port(Aadl_type t, gchar *d)
{
  Aadlport *p;
  p = g_new0(Aadlport,1);
  p->handle = g_new0(Handle,1);
  p->type = t;
  p->declaration = g_strdup(d);
  return p;
}


static void
free_port(Aadlport *port)
{
  if (port) {
    g_clear_pointer (&port->handle, g_free);
    g_clear_pointer (&port->declaration, g_free);
    g_clear_pointer (&port, g_free);
  }
}

enum change_type {
  TYPE_ADD_POINT,
  TYPE_REMOVE_POINT,
  TYPE_ADD_CONNECTION,
  TYPE_REMOVE_CONNECTION
};


struct _DiaAADLPointObjectChange {
  DiaObjectChange obj_change;

  enum change_type type;
  int applied;

  Point point;
  Aadlport *port; /* owning ref when not applied for ADD_POINT
		     owning ref when applied for REMOVE_POINT */

  ConnectionPoint *connection;
};


DIA_DEFINE_OBJECT_CHANGE (DiaAADLPointObjectChange, dia_aadl_point_object_change)


static void aadlbox_update_data(Aadlbox *aadlbox);
static void aadlbox_add_port(Aadlbox *aadlbox, const Point *p, Aadlport *port);
static void aadlbox_remove_port(Aadlbox *aadlbox, Aadlport *port);
static DiaObjectChange *aadlbox_add_port_callback          (DiaObject *obj,
                                                            Point     *clicked,
                                                            gpointer   data);
static DiaObjectChange *aadlbox_delete_port_callback       (DiaObject *obj,
                                                            Point     *clicked,
                                                            gpointer   data);
int aadlbox_point_near_port(Aadlbox *aadlbox, Point *p);

static void aadlbox_add_connection(Aadlbox *aadlbox, const Point *p,
				   ConnectionPoint *connection);
static void aadlbox_remove_connection(Aadlbox *aadlbox,
				      ConnectionPoint *connection);
static DiaObjectChange *aadlbox_add_connection_callback    (DiaObject *obj,
                                                            Point     *clicked,
                                                            gpointer   data);
static DiaObjectChange *aadlbox_delete_connection_callback (DiaObject *obj,
                                                            Point     *clicked,
                                                            gpointer   data);
static int aadlbox_point_near_connection(Aadlbox *aadlbox, Point *p);

/* == TEMPLATES ==

    ObjectTypeOps aadlbox_type_ops =
    {
      (CreateFunc) aadlbox_create,
      (LoadFunc )   aadlbox_load,
      (SaveFunc)   aadlbox_save,
      (GetDefaultsFunc)   NULL,
      (ApplyDefaultsFunc) NULL
    };

    DiaObjectType aadlbox_type =
    {
      "AADL - Box",     // name
      0,                // version
      aadlbox_xpm,      // pixmap

      &aadlbox_type_ops // ops
    };

    DiaMenu *aadlbox_get_object_menu(Aadlbox *aadlbox, Point *clickedpoint);
    static ObjectOps aadlbox_ops =
    {
      (DestroyFunc)         aadlbox_destroy,
      (DrawFunc)            aadlbox_draw,
      (DistanceFunc)        aadlbox_distance_from,
      (SelectFunc)          aadlbox_select,
      (CopyFunc)            aadlbox_copy,
      (MoveFunc)            aadlbox_move,
      (MoveHandleFunc)      aadlbox_move_handle,
      (GetPropertiesFunc)   object_create_props_dialog,
      (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
      (ObjectMenuFunc)      aadlbox_get_object_menu,
      (DescribePropsFunc)   aadlbox_describe_props,
      (GetPropsFunc)        aadlbox_get_props,
      (SetPropsFunc)        aadlbox_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
    };
*/

static PropDescription aadlbox_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "declaration", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
                                               N_("Declaration"), NULL, NULL },
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR_OPTIONAL,
  { "name", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL },
  PROP_DESC_END
};


PropDescription *
aadlbox_describe_props(Aadlbox *aadlbox)
{
  if (aadlbox_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(aadlbox_props);
  }
  return aadlbox_props;
}

static PropOffset aadlbox_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  {"declaration",PROP_TYPE_STRING,offsetof(Aadlbox,declaration)},
  {"line_colour",PROP_TYPE_COLOUR,offsetof(Aadlbox,line_color)},
  {"fill_colour",PROP_TYPE_COLOUR,offsetof(Aadlbox,fill_color)},
  {"name",PROP_TYPE_TEXT,offsetof(Aadlbox,name)},
  {"text_font",PROP_TYPE_FONT,offsetof(Aadlbox,name),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT,offsetof(Aadlbox,name),offsetof(Text,height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Aadlbox,name),offsetof(Text,color)},
  { NULL, 0, 0 },
};

void
aadlbox_get_props(Aadlbox * aadlbox, GPtrArray *props)
{
  object_get_props_from_offsets(&aadlbox->element.object,
                                aadlbox_offsets,props);
}

void
aadlbox_set_props(Aadlbox *aadlbox, GPtrArray *props)
{
  object_set_props_from_offsets(&aadlbox->element.object,
                                aadlbox_offsets,props);
  aadlbox_update_data(aadlbox);
}

DiaObject *aadlbox_copy(DiaObject *obj)
{
  int i;
  Handle *handle1,*handle2;
  Aadlport *port;
  ConnectionPoint *connection;
  Aadlbox *aadlbox = (Aadlbox *) obj;
  void *user_data = ((Aadlbox *) obj)->specific;


  DiaObject *newobj = obj->type->ops->create(&obj->position,
					     user_data,
  					     &handle1,&handle2);
  object_copy_props(newobj,obj,FALSE);

  /* copy ports */
  for (i=0; i<aadlbox->num_ports; i++) {
    Point p;
    point_copy(&p, &aadlbox->ports[i]->handle->pos);
    port = new_port(aadlbox->ports[i]->type, aadlbox->ports[i]->declaration);

    aadlbox_add_port((Aadlbox *)newobj, &p, port);
  }

  /* copy connection points */
  for (i=0; i<aadlbox->num_connections; i++) {
    Point p;
    point_copy(&p, &aadlbox->connections[i]->pos);
    connection= g_new0(ConnectionPoint, 1);

    aadlbox_add_connection((Aadlbox *)newobj, &p, connection);
  }

  return newobj;

}


/***********************************************
 **              UNDO / REDO                  **
 ***********************************************/


static void
dia_aadl_point_object_change_free (DiaObjectChange *self)
{
  DiaAADLPointObjectChange *change = DIA_AADL_POINT_OBJECT_CHANGE (self);

  if ( (change->type==TYPE_ADD_POINT && !change->applied) ||
       (change->type==TYPE_REMOVE_POINT && change->applied) ) {

    free_port (change->port);
    change->port = NULL;
  } else if ((change->type==TYPE_ADD_CONNECTION && !change->applied) ||
             (change->type==TYPE_REMOVE_CONNECTION && change->applied)) {

    g_clear_pointer (&change->connection, g_free);
  }
}


static void
dia_aadl_point_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaAADLPointObjectChange *change = DIA_AADL_POINT_OBJECT_CHANGE (self);

  change->applied = 1;
  switch (change->type) {
    case TYPE_ADD_POINT:
      aadlbox_add_port ((Aadlbox *) obj, &change->point, change->port);
      break;
    case TYPE_REMOVE_POINT:
      aadlbox_remove_port ((Aadlbox *) obj, change->port);
      break;
    case TYPE_ADD_CONNECTION:
      aadlbox_add_connection ((Aadlbox *) obj,
                              &change->point, change->connection);
      break;
    case TYPE_REMOVE_CONNECTION:
      aadlbox_remove_connection ((Aadlbox *) obj, change->connection);
      break;
    default:
      g_return_if_reached ();
  }
  aadlbox_update_data ((Aadlbox *) obj);
}


static void
dia_aadl_point_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaAADLPointObjectChange *change = DIA_AADL_POINT_OBJECT_CHANGE (self);

  switch (change->type) {
    case TYPE_ADD_POINT:
      aadlbox_remove_port((Aadlbox *)obj, change->port);
      break;

    case TYPE_REMOVE_POINT:
      aadlbox_add_port((Aadlbox *)obj, &change->point, change->port);
      break;

    case TYPE_ADD_CONNECTION:
      aadlbox_remove_connection((Aadlbox *)obj, change->connection);
      break;

    case TYPE_REMOVE_CONNECTION: ;
      aadlbox_add_connection((Aadlbox *)obj, &change->point, change->connection);
      break;

    default:
      g_return_if_reached ();
  }

  aadlbox_update_data ((Aadlbox *) obj);
  change->applied = 0;
}


static DiaObjectChange *
aadlbox_create_change (Aadlbox          *aadlbox,
                       enum change_type  type,
                       Point            *point,
                       void             *data)
{
  DiaAADLPointObjectChange *change;

  change = dia_object_change_new (DIA_AADL_TYPE_POINT_OBJECT_CHANGE);

  change->type = type;
  change->applied = 1;
  change->point = *point;

  switch (type) {
    case TYPE_ADD_POINT:
    case TYPE_REMOVE_POINT:
      change->port = (Aadlport *) data;
      break;

    case TYPE_ADD_CONNECTION:
    case TYPE_REMOVE_CONNECTION:
      change->connection = (ConnectionPoint *) data;
      break;

    default:
      g_return_val_if_reached (NULL);
  }

  return DIA_OBJECT_CHANGE (change);
}




/***********************************************
 **                 MENU                      **
 ***********************************************/

static Aadl_type Access_provider = ACCESS_PROVIDER;
static Aadl_type Access_requirer = ACCESS_REQUIRER;
static Aadl_type In_data_port = IN_DATA_PORT;
static Aadl_type In_event_port = IN_EVENT_PORT;
static Aadl_type In_event_data_port = IN_EVENT_DATA_PORT;
static Aadl_type Out_data_port = OUT_DATA_PORT;
static Aadl_type Out_event_port = OUT_EVENT_PORT;
static Aadl_type Out_event_data_port = OUT_EVENT_DATA_PORT;
static Aadl_type In_out_data_port = IN_OUT_DATA_PORT;
static Aadl_type In_out_event_port = IN_OUT_EVENT_PORT;
static Aadl_type In_out_event_data_port = IN_OUT_EVENT_DATA_PORT;
static Aadl_type Port_group = PORT_GROUP;


static DiaMenuItem aadlbox_menu_items[] = {
  { N_("Add Access Provider"), aadlbox_add_port_callback,
                                            &Access_provider, 1 },
  { N_("Add Access Requirer"), aadlbox_add_port_callback,
                                             &Access_requirer, 1 },
  { N_("Add In Data Port"), aadlbox_add_port_callback,
                                             &In_data_port, 1 },
  { N_("Add In Event Port"), aadlbox_add_port_callback,
                                             &In_event_port, 1 },
  { N_("Add In Event Data Port"), aadlbox_add_port_callback,
                                             &In_event_data_port, 1 },
  { N_("Add Out Data Port"), aadlbox_add_port_callback,
                                             &Out_data_port, 1 },
  { N_("Add Out Event Port"), aadlbox_add_port_callback,
                                             &Out_event_port, 1 },
  { N_("Add Out Event Data Port"), aadlbox_add_port_callback,
                                             &Out_event_data_port, 1 },
  { N_("Add In Out Data Port"), aadlbox_add_port_callback,
                                             &In_out_data_port, 1 },
  { N_("Add In Out Event Port"), aadlbox_add_port_callback,
                                             &In_out_event_port, 1 },
  { N_("Add In Out Event Data Port"), aadlbox_add_port_callback,
                                             &In_out_event_data_port, 1 },
  { N_("Add Port Group"), aadlbox_add_port_callback,
                                             &Port_group, 1 },
  { N_("Add Connection Point"), aadlbox_add_connection_callback, NULL, 1 }
};

static DiaMenuItem aadlport_menu_items[] = {
  { N_("Delete Port"), aadlbox_delete_port_callback, NULL, 1 },
  { N_("Edit Port Declaration"), edit_port_declaration_callback, NULL, 1 }
};

static DiaMenuItem aadlconn_menu_items[] = {
  { N_("Delete Connection Point"), aadlbox_delete_connection_callback, NULL, 1}
};


static DiaMenu aadlbox_menu = {
  "AADL",
  sizeof(aadlbox_menu_items)/sizeof(DiaMenuItem),
  aadlbox_menu_items,
  NULL
};

static DiaMenu aadlport_menu = {
  "AADL Port",
  sizeof(aadlport_menu_items)/sizeof(DiaMenuItem),
  aadlport_menu_items,
  NULL
};


static DiaMenu aadlconn_menu = {
  "Connection Point",
  sizeof(aadlconn_menu_items)/sizeof(DiaMenuItem),
  aadlconn_menu_items,
  NULL
};


DiaMenu *
aadlbox_get_object_menu(Aadlbox *aadlbox, Point *clickedpoint)
{
  int n;

  if ((n = aadlbox_point_near_port(aadlbox, clickedpoint)) >= 0) {

    /* no port declaration with event ports */

    if ( aadlbox->ports[n]->type == IN_EVENT_PORT  ||
	 aadlbox->ports[n]->type == OUT_EVENT_PORT ||
	 aadlbox->ports[n]->type == IN_OUT_EVENT_PORT  )

      aadlport_menu_items[1].active = 0;
    else
      aadlport_menu_items[1].active = 1;

    return &aadlport_menu;
  }

  if (aadlbox_point_near_connection(aadlbox, clickedpoint) >= 0)
    return &aadlconn_menu;


  return &aadlbox_menu;
}


/*________________________________
  ________Menu functions__________ */


/* add/remove ports */

int
aadlbox_point_near_port(Aadlbox *aadlbox, Point *p)
{
  int i, min;
  real dist = 1000.0;
  real d;

  min = -1;
  for (i=0;i<aadlbox->num_ports;i++) {
    d = distance_point_point(&aadlbox->ports[i]->handle->pos, p);

    if (d < dist) {
      dist = d;
      min = i;
    }
  }

  if (dist < 0.5)
    return min;
  else
    return -1;
}

static void
aadlbox_add_port(Aadlbox *aadlbox, const Point *p, Aadlport *port)
{
  int i;

  aadlbox->num_ports++;

  if (aadlbox->ports == NULL) {
    aadlbox->ports = g_new0 (Aadlport *, aadlbox->num_ports);
  } else {
    /* Allocate more ports */
    aadlbox->ports = g_renew (Aadlport *, aadlbox->ports, aadlbox->num_ports);
  }

  i = aadlbox->num_ports - 1;

  aadlbox->ports[i] = port;
  aadlbox->ports[i]->handle->id = PORT_HANDLE_AADLBOX;
  aadlbox->ports[i]->handle->type = HANDLE_MINOR_CONTROL;
  aadlbox->ports[i]->handle->connect_type = HANDLE_CONNECTABLE_NOBREAK;
  aadlbox->ports[i]->handle->connected_to = NULL;
  aadlbox->ports[i]->handle->pos = *p;
  object_add_handle(&aadlbox->element.object, aadlbox->ports[i]->handle);


  port->in.connected = NULL; port->in.object = &aadlbox->element.object;
  port->out.connected = NULL; port->out.object = &aadlbox->element.object;
  object_add_connectionpoint(&aadlbox->element.object, &port->in);
  object_add_connectionpoint(&aadlbox->element.object, &port->out);

}

static void
aadlbox_remove_port(Aadlbox *aadlbox, Aadlport *port)
{
  int i, j;

  for (i=0;i<aadlbox->num_ports;i++) {
    if (aadlbox->ports[i] == port) {
      object_remove_handle(&aadlbox->element.object, port->handle);

      for (j=i;j<aadlbox->num_ports-1;j++) {
	aadlbox->ports[j] = aadlbox->ports[j+1];
      }

      object_remove_connectionpoint(&aadlbox->element.object, &port->in);
      object_remove_connectionpoint(&aadlbox->element.object, &port->out);

      aadlbox->num_ports--;
      aadlbox->ports = g_renew (Aadlport *, aadlbox->ports, aadlbox->num_ports);
      break;
    }
  }
}


DiaObjectChange *
aadlbox_add_port_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Aadlbox *aadlbox = (Aadlbox *) obj;
  Aadl_type type = *((Aadl_type *) data);
  Aadlport *port;

  port = new_port (type, "");
  aadlbox_add_port (aadlbox, clicked, port);
  aadlbox_update_data (aadlbox);

  return aadlbox_create_change (aadlbox, TYPE_ADD_POINT, clicked, port);
}


DiaObjectChange *
aadlbox_delete_port_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Aadlbox *aadlbox = (Aadlbox *) obj;
  Aadlport *port;
  int port_num;
  Point p;

  port_num = aadlbox_point_near_port (aadlbox, clicked);

  port = aadlbox->ports[port_num];
  p = port->handle->pos;

  aadlbox_remove_port (aadlbox, port);
  aadlbox_update_data (aadlbox);

  return aadlbox_create_change (aadlbox, TYPE_REMOVE_POINT, &p, port);
}




/* add/remove connections */

static int
aadlbox_point_near_connection(Aadlbox *aadlbox, Point *p)
{
  int i, min;
  real dist = 1000.0;
  real d;

  min = -1;
  for (i=0;i<aadlbox->num_connections;i++) {
    d = distance_point_point(&aadlbox->connections[i]->pos, p);

    if (d < dist) {
      dist = d;
      min = i;
    }
  }

  if (dist < 0.5)
    return min;
  else
    return -1;
}

static void
aadlbox_add_connection(Aadlbox *aadlbox, const Point *p, ConnectionPoint *connection)
{
  int i;

  connection->object = (DiaObject *) aadlbox;
  connection->connected = NULL;           /* FIXME: could be avoided ?? */

  aadlbox->num_connections++;

  if (aadlbox->connections == NULL) {
    aadlbox->connections = g_new0 (ConnectionPoint *, aadlbox->num_connections);
  } else {
    /* Allocate more connections */
    aadlbox->connections = g_renew (ConnectionPoint *,
                                    aadlbox->connections,
                                    aadlbox->num_connections);
  }

  i = aadlbox->num_connections - 1;

  aadlbox->connections[i] = connection;
  aadlbox->connections[i]->pos = *p;

  object_add_connectionpoint(&aadlbox->element.object, connection);

}

static void
aadlbox_remove_connection(Aadlbox *aadlbox, ConnectionPoint *connection)
{
  int i, j;

  for (i=0;i<aadlbox->num_connections;i++) {
    if (aadlbox->connections[i] == connection) {

      for (j=i;j<aadlbox->num_connections-1;j++) {
	aadlbox->connections[j] = aadlbox->connections[j+1];
      }

      object_remove_connectionpoint(&aadlbox->element.object, connection);

      aadlbox->num_connections--;
      aadlbox->connections = g_renew (ConnectionPoint *,
                                      aadlbox->connections,
                                      aadlbox->num_connections);
      break;
    }
  }
}


DiaObjectChange *
aadlbox_add_connection_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Aadlbox *aadlbox = (Aadlbox *) obj;
  ConnectionPoint *connection;

  connection = g_new0(ConnectionPoint,1);

  aadlbox_add_connection (aadlbox, clicked, connection);
  aadlbox_update_data (aadlbox);

  return aadlbox_create_change (aadlbox, TYPE_ADD_CONNECTION, clicked, connection);
}


DiaObjectChange *
aadlbox_delete_connection_callback (DiaObject *obj,
                                    Point     *clicked,
                                    gpointer   data)
{
  Aadlbox *aadlbox = (Aadlbox *) obj;
  ConnectionPoint *connection;
  int connection_num;
  Point p;

  connection_num = aadlbox_point_near_connection (aadlbox, clicked);

  connection = aadlbox->connections[connection_num];
  p = connection->pos;

  aadlbox_remove_connection (aadlbox, connection);
  aadlbox_update_data (aadlbox);

  return aadlbox_create_change (aadlbox,
                                TYPE_REMOVE_CONNECTION,
                                &p,
                                connection);
}

/***********************************************
 **          "CLASSIC FUNCTIONS"              **
 ***********************************************/

real
aadlbox_distance_from(Aadlbox *aadlbox, Point *point)
{
  DiaObject *obj = &aadlbox->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

void
aadlbox_select(Aadlbox *aadlbox, Point *clicked_point,
		    DiaRenderer *interactive_renderer)
{
  text_set_cursor(aadlbox->name, clicked_point, interactive_renderer);
  text_grab_focus(aadlbox->name, &aadlbox->element.object);
  element_update_handles(&aadlbox->element);
}


DiaObjectChange *
aadlbox_move_handle (Aadlbox          *aadlbox,
                     Handle           *handle,
                     Point            *to,
                     ConnectionPoint  *cp,
                     HandleMoveReason  reason,
                     ModifierKeys      modifiers)
{
  g_return_val_if_fail (aadlbox != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  if (handle->id < 8) {
    /* box resizing */
    Element *element = &aadlbox->element;
    Point oldcorner, newcorner;
    real oldw, neww, oldh, newh;
    real w_factor, h_factor;
    Aadlport *p;
    ConnectionPoint *c;
    int i;

    point_copy(&oldcorner, &element->corner);
    oldw = element->width;
    oldh = element->height;

    element_move_handle( &aadlbox->element, handle->id, to, cp,
			 reason, modifiers);

    point_copy(&newcorner, &element->corner);
    neww = element->width;
    newh = element->height;

    /* update ports positions proportionally */
    for (i=0; i < aadlbox->num_ports; i++)
    {
      p = aadlbox->ports[i];

      w_factor = (p->handle->pos.x - oldcorner.x) / oldw;
      h_factor = (p->handle->pos.y - oldcorner.y) / oldh;

      p->handle->pos.x = newcorner.x + w_factor * neww;
      p->handle->pos.y = newcorner.y + h_factor * newh;
    }

    /* update connection points proportionally */
    for (i=0; i < aadlbox->num_connections; i++)
    {
      c = aadlbox->connections[i];

      w_factor = (c->pos.x - oldcorner.x) / oldw;
      h_factor = (c->pos.y - oldcorner.y) / oldh;

      c->pos.x = newcorner.x + w_factor * neww;
      c->pos.y = newcorner.y + h_factor * newh;
    }
  } else {
    /* port handles */
    handle->pos.x = to->x;
    handle->pos.y = to->y;
  }

  aadlbox_update_data (aadlbox);

  /* FIXME !!  Should I free the given structures (to, ...) ? */
  return NULL;
}


DiaObjectChange *
aadlbox_move (Aadlbox *aadlbox, Point *to)
{
  Point p, delta;
  DiaObject *obj = &aadlbox->element.object;
  int i;

  delta = *to;
  point_sub (&delta, &obj->position);

  /* update ports position */
  for (i = 0; i < aadlbox->num_ports; i++) {
    point_add (&aadlbox->ports[i]->handle->pos, &delta);
  }

  /* update connection points position */
  for (i = 0; i < aadlbox->num_connections; i++) {
    point_add (&aadlbox->connections[i]->pos, &delta);
  }


  aadlbox->element.corner = *to;

  p = *to;
  p.x += AADLBOX_TEXT_MARGIN;
  p.y += aadlbox->name->ascent + AADLBOX_TEXT_MARGIN;

  aadlbox_update_data (aadlbox);

  return NULL;
}


void aadlbox_draw(Aadlbox *aadlbox, DiaRenderer *renderer)
{
  int i;

  text_draw(aadlbox->name, renderer);

  /* draw ports */
  for (i=0;i<aadlbox->num_ports;i++)
    aadlbox_draw_port(aadlbox->ports[i], renderer);
}


static void
aadlbox_update_text_position(Aadlbox *aadlbox)
{
  Point p;

  aadlbox->specific->text_position(aadlbox, &p);
  text_set_position(aadlbox->name, &p);
}

static void
aadlbox_update_data(Aadlbox *aadlbox)
{
  Element *elem = &aadlbox->element;
  DiaObject *obj = &aadlbox->element.object;
  Point min_size;
  int i;
  real tmp;

  aadlbox->specific->min_size(aadlbox, &min_size);

  elem->width = MAX(elem->width, min_size.x);
  elem->height = MAX(elem->height, min_size.y);

  element_update_boundingbox(elem);

  /* extend bounding box because of ports */
  /* FIXME: This cause the box to be selectionned when clicking out of it !! */
  obj->bounding_box.top -= AADL_PORT_MAX_OUT + 0.1;
  obj->bounding_box.right += AADL_PORT_MAX_OUT + 0.1;
  obj->bounding_box.bottom += AADL_PORT_MAX_OUT + 0.1;
  obj->bounding_box.left -= AADL_PORT_MAX_OUT + 0.1;

  obj->position = elem->corner;

  aadlbox_update_text_position(aadlbox);

  element_update_handles(elem);

  aadlbox_update_ports(aadlbox);

  for (i=0;i<aadlbox->num_connections;i++)
      aadlbox->specific->project_point_on_nearest_border(aadlbox,
					       &aadlbox->connections[i]->pos,
					       &tmp);
}


/** *NOT A CALLBACK*

    Caller must set:    - obj->type
    ---------------     - obj->ops

*/
DiaObject *aadlbox_create(Point *startpoint, void *user_data,
			  Handle **handle1, Handle **handle2)
{

  Aadlbox *aadlbox;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont *font;

  aadlbox = g_new0 (Aadlbox, 1);
  elem = &aadlbox->element;
  obj = &elem->object;

  elem->corner = *startpoint;

  aadlbox->specific = (Aadlbox_specific *) user_data;

  aadlbox->num_ports = 0;
  aadlbox->ports = NULL;

  aadlbox->line_color = attributes_get_foreground();
  aadlbox->fill_color = attributes_get_background();

  font = dia_font_new_from_style (DIA_FONT_SANS, 0.8);
  /* The text position is recalculated later */
  p.x = 0.0;
  p.y = 0.0;
  aadlbox->name = new_text ("", font, 0.8, &p, &color_black, DIA_ALIGN_LEFT);
  g_clear_object (&font);

  element_init(elem, 8, 0);  /* 8 handles and 0 connection */

  elem->extra_spacing.border_trans = AADLBOX_BORDERWIDTH/2.0;
  aadlbox_update_data(aadlbox);

  *handle1 = NULL;
  *handle2 = obj->handles[7];
  return &aadlbox->element.object;
}

void
aadlbox_destroy(Aadlbox *aadlbox)
{
  int i;
  text_destroy(aadlbox->name);

  /* object_unconnect needs valid handles (from ports) */
  element_destroy(&aadlbox->element);

  for (i=0; i<aadlbox->num_ports; i++)
    free_port(aadlbox->ports[i]);
}

void
aadlbox_save(Aadlbox *aadlbox, ObjectNode obj_node, DiaContext *ctx)
{
  int i;
  AttributeNode attr;
  DataNode composite;

  element_save(&aadlbox->element, obj_node, ctx);
  object_save_props(&aadlbox->element.object, obj_node, ctx);

  attr = new_attribute(obj_node, "aadlbox_ports");

  for (i=0;i<aadlbox->num_ports;i++) {
    composite = data_add_composite(attr, "aadlport", ctx);
    data_add_point(composite_add_attribute(composite, "point"),
		   &aadlbox->ports[i]->handle->pos, ctx);
    data_add_enum(composite_add_attribute(composite, "port_type"),
		   aadlbox->ports[i]->type, ctx);
    data_add_string(composite_add_attribute(composite, "port_declaration"),
		    aadlbox->ports[i]->declaration, ctx);
  }

  attr = new_attribute(obj_node, "aadlbox_connections");

  for (i=0;i<aadlbox->num_connections;i++) {
    data_add_point(attr, &aadlbox->connections[i]->pos, ctx);
  }
}

/* *NOT A CALLBACK* --> Must be called by inherited class (see aadldata.c) */
void aadlbox_load(ObjectNode obj_node, int version, DiaContext *ctx,
		   Aadlbox *aadlbox)
{
  AttributeNode attr;
  DataNode composite, data;
  Aadl_type type;
  gchar *declaration;
  Aadlport *port;
  ConnectionPoint *connection;
  int i, num;

  attr = object_find_attribute(obj_node, "aadlbox_ports");

  composite = attribute_first_data(attr);

  num = attribute_num_data(attr);

  for (i=0; i<num; i++) {

    Point p;
    attr = composite_find_attribute(composite, "point");
    data_point(attribute_first_data(attr), &p, ctx);

    attr = composite_find_attribute(composite, "port_type");
    type = data_enum(attribute_first_data(attr), ctx);

    attr = composite_find_attribute(composite, "port_declaration");
    declaration = data_string(attribute_first_data(attr), ctx);

    port = g_new0(Aadlport,1);
    port->handle = g_new0(Handle,1);
    port->type = type;
    port->declaration = declaration;


    aadlbox_add_port(aadlbox, &p, port);

    composite = data_next(composite);
  }

  attr = object_find_attribute(obj_node, "aadlbox_connections");
  num = attribute_num_data(attr);
  data = attribute_first_data(attr);

  for (i=0; i<num; i++) {
    Point p;
    data_point(data, &p, ctx);

    connection = g_new0(ConnectionPoint,1);
    aadlbox_add_connection(aadlbox, &p, connection);

    data = data_next(data);
  }

  object_load_props(&aadlbox->element.object,obj_node, ctx);
}
