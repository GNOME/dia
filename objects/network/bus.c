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

#include "object.h"
#include "connection.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "diamenu.h"
#include "properties.h"

#include "pixmaps/bus.xpm"

#define LINE_WIDTH 0.1

#define DEFAULT_NUMHANDLES 6
#define HANDLE_BUS (HANDLE_CUSTOM1)


typedef struct _Bus {
  Connection connection;

  int num_handles;
  Handle **handles;
  Point *parallel_points;
  Point real_ends[2];
  Color line_color;
} Bus;


#define DIA_NET_TYPE_BUS_OBJECT_CHANGE dia_net_bus_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaNetBusObjectChange,
                      dia_net_bus_object_change,
                      DIA_NET, BUS_OBJECT_CHANGE,
                      DiaObjectChange)


enum change_type {
  TYPE_ADD_POINT,
  TYPE_REMOVE_POINT
};


struct _DiaNetBusObjectChange {
  DiaObjectChange obj_change;

  enum change_type type;
  int applied;

  Point point;
  Handle *handle; /* owning ref when not applied for ADD_POINT
		     owning ref when applied for REMOVE_POINT */
  ConnectionPoint *connected_to; /* NULL if not connected */
};


DIA_DEFINE_OBJECT_CHANGE (DiaNetBusObjectChange, dia_net_bus_object_change)


static DiaObjectChange* bus_move_handle        (Bus              *bus,
                                                Handle           *handle,
                                                Point            *to,
                                                ConnectionPoint  *cp,
                                                HandleMoveReason  reason,
                                                ModifierKeys      modifiers);
static DiaObjectChange* bus_move               (Bus              *bus,
                                                Point            *to);
static void bus_select(Bus *bus, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static void bus_draw(Bus *bus, DiaRenderer *renderer);
static DiaObject *bus_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static real bus_distance_from(Bus *bus, Point *point);
static void bus_update_data(Bus *bus);
static void bus_destroy(Bus *bus);
static DiaObject *bus_copy(Bus *bus);

static PropDescription *bus_describe_props(Bus *bus);
static void bus_get_props(Bus *bus, GPtrArray *props);
static void bus_set_props(Bus *bus, GPtrArray *props);
static void bus_save(Bus *bus, ObjectNode obj_node, DiaContext *ctx);
static DiaObject *bus_load(ObjectNode obj_node, int version, DiaContext *ctx);
static DiaMenu *bus_get_object_menu(Bus *bus, Point *clickedpoint);

static DiaObjectChange *bus_create_change (Bus              *bus,
                                           enum change_type  type,
                                           Point            *point,
                                           Handle           *handle,
                                           ConnectionPoint  *connected_to);


static ObjectTypeOps bus_type_ops =
{
  (CreateFunc) bus_create,
  (LoadFunc)   bus_load,
  (SaveFunc)   bus_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType bus_type =
{
  "Network - Bus",   /* name */
  0,                  /* version */
  (const char **) bus_xpm,  /* pixmap */
  &bus_type_ops       /* ops */
};

static ObjectOps bus_ops = {
  (DestroyFunc)         bus_destroy,
  (DrawFunc)            bus_draw,
  (DistanceFunc)        bus_distance_from,
  (SelectFunc)          bus_select,
  (CopyFunc)            bus_copy,
  (MoveFunc)            bus_move,
  (MoveHandleFunc)      bus_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      bus_get_object_menu,
  (DescribePropsFunc)   bus_describe_props,
  (GetPropsFunc)        bus_get_props,
  (SetPropsFunc)        bus_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription bus_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  PROP_STD_LINE_COLOUR,
  PROP_DESC_END
};

static PropDescription *
bus_describe_props(Bus *bus)
{
  if (bus_props[0].quark == 0)
    prop_desc_list_calculate_quarks(bus_props);
  return bus_props;
}

static PropOffset bus_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Bus, line_color) },
  { NULL, 0, 0 }
};

static void
bus_get_props(Bus *bus, GPtrArray *props)
{
  object_get_props_from_offsets(&bus->connection.object, bus_offsets,
				props);
}

static void
bus_set_props(Bus *bus, GPtrArray *props)
{
  object_set_props_from_offsets(&bus->connection.object, bus_offsets,
				props);
  bus_update_data(bus);
}

static real
bus_distance_from(Bus *bus, Point *point)
{
  Point *endpoints;
  real min_dist;
  int i;

  endpoints = &bus->real_ends[0];
  min_dist = distance_line_point( &endpoints[0], &endpoints[1],
				  LINE_WIDTH, point);
  for (i=0;i<bus->num_handles;i++) {
    min_dist = MIN(min_dist,
		   distance_line_point( &bus->handles[i]->pos,
					&bus->parallel_points[i],
					LINE_WIDTH, point));
  }
  return min_dist;
}

static void
bus_select(Bus *bus, Point *clicked_point,
	    DiaRenderer *interactive_renderer)
{
  connection_update_handles(&bus->connection);
}


static DiaObjectChange *
bus_move_handle (Bus              *bus,
                 Handle           *handle,
                 Point            *to,
                 ConnectionPoint  *cp,
                 HandleMoveReason  reason,
                 ModifierKeys      modifiers)
{
  Connection *conn = &bus->connection;
  Point *endpoints;
  double *parallel=NULL;
  double *perp=NULL;
  Point vhat, vhatperp;
  Point u;
  double vlen, vlen2;
  double len_scale;
  int i;
  const int num_handles = bus->num_handles; /* const to help scan-build */

  /* code copied to Misc/tree.c */
  parallel = (real *)g_alloca (num_handles * sizeof(real));
  perp = (real *)g_alloca (num_handles * sizeof(real));

  if (handle->id == HANDLE_BUS) {
    handle->pos = *to;
  } else {
    endpoints = &conn->endpoints[0];
    vhat = endpoints[1];
    point_sub(&vhat, &endpoints[0]);
    if ((fabs(vhat.x) == 0.0) && (fabs(vhat.y)==0.0)) {
      vhat.x += 0.01;
    }
    vlen = sqrt(point_dot(&vhat, &vhat));
    point_scale(&vhat, 1.0/vlen);

    vhatperp.x = -vhat.y;
    vhatperp.y =  vhat.x;
    for (i=0;i<num_handles;i++) {
      u = bus->handles[i]->pos;
      point_sub(&u, &endpoints[0]);
      parallel[i] = point_dot(&vhat, &u);
      perp[i] = point_dot(&vhatperp, &u);
    }

    connection_move_handle(&bus->connection, handle->id, to, cp,
			   reason, modifiers);

    vhat = endpoints[1];
    point_sub(&vhat, &endpoints[0]);
    if ((fabs(vhat.x) == 0.0) && (fabs(vhat.y)==0.0)) {
      vhat.x += 0.01;
    }
    vlen2 = sqrt(point_dot(&vhat, &vhat));
    len_scale = vlen2 / vlen;
    point_normalize(&vhat);
    vhatperp.x = -vhat.y;
    vhatperp.y =  vhat.x;
    for (i=0;i<num_handles;i++) {
      if (bus->handles[i]->connected_to == NULL) {
	u = vhat;
	point_scale(&u, parallel[i]*len_scale);
	point_add(&u, &endpoints[0]);
	bus->parallel_points[i] = u;
	u = vhatperp;
	point_scale(&u, perp[i]);
	point_add(&u, &bus->parallel_points[i]);
	bus->handles[i]->pos = u;
	}
    }
  }

  bus_update_data(bus);

  return NULL;
}


static DiaObjectChange *
bus_move (Bus *bus, Point *to)
{
  Point delta;
  Point *endpoints = &bus->connection.endpoints[0];
  DiaObject *obj = &bus->connection.object;
  int i;

  delta = *to;
  point_sub(&delta, &obj->position);

  for (i=0;i<2;i++) {
    point_add(&endpoints[i], &delta);
    point_add(&bus->real_ends[i], &delta);
  }

  for (i=0;i<bus->num_handles;i++) {
    if (bus->handles[i]->connected_to == NULL) {
      point_add(&bus->handles[i]->pos, &delta);
    }
  }

  bus_update_data(bus);

  return NULL;
}

static void
bus_draw (Bus *bus, DiaRenderer *renderer)
{
  Point *endpoints;
  int i;

  g_return_if_fail (bus != NULL);
  g_return_if_fail (renderer != NULL);

  endpoints = &bus->real_ends[0];

  dia_renderer_set_linewidth (renderer, LINE_WIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  dia_renderer_draw_line (renderer,
                          &endpoints[0],
                          &endpoints[1],
                          &bus->line_color);

  for (i = 0; i < bus->num_handles; i++) {
    dia_renderer_draw_line (renderer,
                            &bus->parallel_points[i],
                            &bus->handles[i]->pos,
                            &bus->line_color);
  }
}

static DiaObject *
bus_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Bus *bus;
  Connection *conn;
  LineBBExtras *extra;
  DiaObject *obj;
  Point defaultlen = { 5.0, 0.0 };

  bus = g_new0 (Bus, 1);

  conn = &bus->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);

  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &bus_type;
  obj->ops = &bus_ops;

  bus->num_handles = DEFAULT_NUMHANDLES;

  connection_init(conn, 2+bus->num_handles, 0);
  bus->line_color = attributes_get_foreground();
  bus->handles = g_new0 (Handle *, bus->num_handles);
  bus->parallel_points = g_new0 (Point, bus->num_handles);
  for (int i = 0; i < bus->num_handles; i++) {
    bus->handles[i] = g_new0 (Handle, 1);
    bus->handles[i]->id = HANDLE_BUS;
    bus->handles[i]->type = HANDLE_MINOR_CONTROL;
    bus->handles[i]->connect_type = HANDLE_CONNECTABLE_NOBREAK;
    bus->handles[i]->connected_to = NULL;
    bus->handles[i]->pos = *startpoint;
    bus->handles[i]->pos.x += 5*((real)i+1)/(bus->num_handles+1);
    bus->handles[i]->pos.y += (i%2==0)?1.0:-1.0;
    obj->handles[2+i] = bus->handles[i];
  }

  extra->start_trans =
    extra->end_trans =
    extra->start_long =
    extra->end_long = LINE_WIDTH/2.0;
  bus_update_data(bus);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return &bus->connection.object;
}

static void
bus_destroy(Bus *bus)
{
  int i;
  connection_destroy (&bus->connection);
  for (i=0;i<bus->num_handles;i++) {
    g_clear_pointer (&bus->handles[i], g_free);
  }
  g_clear_pointer (&bus->handles, g_free);
  g_clear_pointer (&bus->parallel_points, g_free);
}

static DiaObject *
bus_copy(Bus *bus)
{
  Bus *newbus;
  Connection *conn, *newconn;
  DiaObject *newobj;

  conn = &bus->connection;

  newbus = g_new0 (Bus, 1);
  newconn = &newbus->connection;
  newobj = &newconn->object;

  connection_copy(conn, newconn);

  newbus->num_handles = bus->num_handles;
  newbus->line_color = bus->line_color;

  newbus->handles = g_new0 (Handle *, newbus->num_handles);
  newbus->parallel_points = g_new0 (Point, newbus->num_handles);

  for (int i = 0; i < newbus->num_handles; i++) {
    newbus->handles[i] = g_new0 (Handle, 1);
    *newbus->handles[i] = *bus->handles[i];
    newbus->handles[i]->connected_to = NULL;
    newobj->handles[2+i] = newbus->handles[i];
    newbus->parallel_points[i] = bus->parallel_points[i];
  }

  newbus->real_ends[0] = bus->real_ends[0];
  newbus->real_ends[1] = bus->real_ends[1];

  return &newbus->connection.object;
}


static void
bus_update_data(Bus *bus)
{
  Connection *conn = &bus->connection;
  DiaObject *obj = &conn->object;
  int i;
  Point u, v, vhat;
  Point *endpoints;
  real ulen;
  real min_par, max_par;

/*
 * This seems to break stuff wildly.
 */
/*
  if (connpoint_is_autogap(conn->endpoint_handles[0].connected_to) ||
      connpoint_is_autogap(conn->endpoint_handles[1].connected_to)) {
    connection_adjust_for_autogap(conn);
  }
*/
  endpoints = &conn->endpoints[0];
  obj->position = endpoints[0];

  v = endpoints[1];
  point_sub(&v, &endpoints[0]);
  if ((fabs(v.x) == 0.0) && (fabs(v.y)==0.0)) {
    v.x += 0.01;
  }
  vhat = v;
  point_normalize(&vhat);
  min_par = 0.0;
  max_par = point_dot(&vhat, &v);
  for (i=0;i<bus->num_handles;i++) {
    u = bus->handles[i]->pos;
    point_sub(&u, &endpoints[0]);
    ulen = point_dot(&u, &vhat);
    min_par = MIN(min_par, ulen);
    max_par = MAX(max_par, ulen);
    bus->parallel_points[i] = vhat;
    point_scale(&bus->parallel_points[i], ulen);
    point_add(&bus->parallel_points[i], &endpoints[0]);
  }

  min_par -= LINE_WIDTH/2.0;
  max_par += LINE_WIDTH/2.0;

  bus->real_ends[0] = vhat;
  point_scale(&bus->real_ends[0], min_par);
  point_add(&bus->real_ends[0], &endpoints[0]);
  bus->real_ends[1] = vhat;
  point_scale(&bus->real_ends[1], max_par);
  point_add(&bus->real_ends[1], &endpoints[0]);

  connection_update_boundingbox(conn);
  rectangle_add_point(&obj->bounding_box, &bus->real_ends[0]);
  rectangle_add_point(&obj->bounding_box, &bus->real_ends[1]);
  for (i=0;i<bus->num_handles;i++) {
    rectangle_add_point(&obj->bounding_box, &bus->handles[i]->pos);
  }

  connection_update_handles(conn);
}


static void
bus_add_handle (Bus *bus, Point *p, Handle *handle)
{
  int i;

  bus->num_handles++;

  /* Allocate more handles */
  bus->handles = g_renew (Handle *, bus->handles, bus->num_handles);
  bus->parallel_points = g_renew (Point, bus->parallel_points, bus->num_handles);

  i = bus->num_handles - 1;

  bus->handles[i] = handle;
  bus->handles[i]->id = HANDLE_BUS;
  bus->handles[i]->type = HANDLE_MINOR_CONTROL;
  bus->handles[i]->connect_type = HANDLE_CONNECTABLE_NOBREAK;
  bus->handles[i]->connected_to = NULL;
  bus->handles[i]->pos = *p;
  object_add_handle(&bus->connection.object, bus->handles[i]);
}

static void
bus_remove_handle(Bus *bus, Handle *handle)
{
  int i, j;

  for (i=0;i<bus->num_handles;i++) {
    if (bus->handles[i] == handle) {
      object_remove_handle(&bus->connection.object, handle);

      for (j=i;j<bus->num_handles-1;j++) {
	bus->handles[j] = bus->handles[j+1];
	bus->parallel_points[j] = bus->parallel_points[j+1];
      }

      bus->num_handles--;
      bus->handles = g_renew (Handle *, bus->handles, bus->num_handles);
      bus->parallel_points = g_renew (Point,
                                      bus->parallel_points,
                                      bus->num_handles);

      break;
    }
  }
}


static DiaObjectChange *
bus_add_handle_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Bus *bus = (Bus *) obj;
  Handle *handle;

  handle = g_new0(Handle,1);
  bus_add_handle(bus, clicked, handle);
  bus_update_data(bus);

  return bus_create_change(bus, TYPE_ADD_POINT, clicked, handle, NULL);
}

static int
bus_point_near_handle(Bus *bus, Point *p)
{
  int i, min;
  real dist = 1000.0;
  real d;

  min = -1;
  for (i=0;i<bus->num_handles;i++) {
    d = distance_line_point(&bus->parallel_points[i],
			    &bus->handles[i]->pos, 0.0, p);

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


static DiaObjectChange *
bus_delete_handle_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Bus *bus = (Bus *) obj;
  Handle *handle;
  int handle_num;
  ConnectionPoint *connectionpoint;
  Point p;

  handle_num = bus_point_near_handle(bus, clicked);

  handle = bus->handles[handle_num];
  p = handle->pos;
  connectionpoint = handle->connected_to;

  object_unconnect(obj, handle);
  bus_remove_handle(bus, handle );
  bus_update_data(bus);

  return bus_create_change(bus, TYPE_REMOVE_POINT, &p, handle,
			   connectionpoint);
}

static DiaMenuItem bus_menu_items[] = {
  { N_("Add Handle"), bus_add_handle_callback, NULL, 1 },
  { N_("Delete Handle"), bus_delete_handle_callback, NULL, 1 },
};

static DiaMenu bus_menu = {
  "Bus",
  sizeof(bus_menu_items)/sizeof(DiaMenuItem),
  bus_menu_items,
  NULL
};

static DiaMenu *
bus_get_object_menu(Bus *bus, Point *clickedpoint)
{
  /* Set entries sensitive/selected etc here */
  bus_menu_items[0].active = 1;
  bus_menu_items[1].active = (bus_point_near_handle(bus, clickedpoint) >= 0);
  return &bus_menu;
}

static void
bus_save(Bus *bus, ObjectNode obj_node, DiaContext *ctx)
{
  int i;
  AttributeNode attr;

  connection_save(&bus->connection, obj_node, ctx);

  data_add_color( new_attribute(obj_node, "line_color"), &bus->line_color, ctx);

  attr = new_attribute(obj_node, "bus_handles");

  for (i=0;i<bus->num_handles;i++) {
    data_add_point(attr, &bus->handles[i]->pos, ctx);
  }
}

static DiaObject *
bus_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  Bus *bus;
  Connection *conn;
  LineBBExtras *extra;
  DiaObject *obj;
  AttributeNode attr;
  DataNode data;

  bus = g_new0 (Bus, 1);

  conn = &bus->connection;
  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &bus_type;
  obj->ops = &bus_ops;

  connection_load(conn, obj_node, ctx);

  attr = object_find_attribute(obj_node, "bus_handles");

  bus->num_handles = 0;
  if (attr != NULL)
    bus->num_handles = attribute_num_data(attr);

  connection_init(conn, 2 + bus->num_handles, 0);

  data = attribute_first_data(attr);
  bus->handles = g_new0 (Handle *, bus->num_handles);
  bus->parallel_points = g_new0 (Point, bus->num_handles);
  for (int i = 0; i < bus->num_handles; i++) {
    bus->handles[i] = g_new0 (Handle, 1);
    bus->handles[i]->id = HANDLE_BUS;
    bus->handles[i]->type = HANDLE_MINOR_CONTROL;
    bus->handles[i]->connect_type = HANDLE_CONNECTABLE_NOBREAK;
    bus->handles[i]->connected_to = NULL;
    data_point(data, &bus->handles[i]->pos, ctx);
    obj->handles[2+i] = bus->handles[i];

    data = data_next(data);
  }

  bus->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &bus->line_color, ctx);

  extra->start_trans =
    extra->end_trans =
    extra->start_long =
    extra->end_long = LINE_WIDTH/2.0;
  bus_update_data(bus);

  return &bus->connection.object;
}


static void
dia_net_bus_object_change_free (DiaObjectChange *self)
{
  DiaNetBusObjectChange *change = DIA_NET_BUS_OBJECT_CHANGE (self);

  if ((change->type == TYPE_ADD_POINT && !change->applied) ||
      (change->type == TYPE_REMOVE_POINT && change->applied) ){
    g_clear_pointer (&change->handle, g_free);
  }
}


static void
dia_net_bus_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaNetBusObjectChange *change = DIA_NET_BUS_OBJECT_CHANGE (self);

  change->applied = 1;

  switch (change->type) {
    case TYPE_ADD_POINT:
      bus_add_handle ((Bus *) obj, &change->point, change->handle);
      break;
    case TYPE_REMOVE_POINT:
      object_unconnect (obj, change->handle);
      bus_remove_handle ((Bus *) obj, change->handle);
      break;
    default:
      g_return_if_reached ();
  }

  bus_update_data ((Bus *) obj);
}


static void
dia_net_bus_object_change_revert  (DiaObjectChange *self, DiaObject *obj)
{
  DiaNetBusObjectChange *change = DIA_NET_BUS_OBJECT_CHANGE (self);

  switch (change->type) {
    case TYPE_ADD_POINT:
      bus_remove_handle ((Bus *) obj, change->handle);
      break;
    case TYPE_REMOVE_POINT:
      bus_add_handle ((Bus *) obj, &change->point, change->handle);
      if (change->connected_to) {
        object_connect (obj, change->handle, change->connected_to);
      }
      break;
    default:
      g_return_if_reached ();
  }

  bus_update_data ((Bus *) obj);

  change->applied = 0;
}


static DiaObjectChange *
bus_create_change (Bus              *bus,
                   enum change_type  type,
                   Point            *point,
                   Handle           *handle,
                   ConnectionPoint  *connected_to)
{
  DiaNetBusObjectChange *change;

  change = dia_object_change_new (DIA_NET_TYPE_BUS_OBJECT_CHANGE);

  change->type = type;
  change->applied = 1;
  change->point = *point;
  change->handle = handle;
  change->connected_to = connected_to;

  return DIA_OBJECT_CHANGE (change);
}
