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

#include "object.h"
#include "connection.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "files.h"
#include "sheet.h"

#include "pixmaps/bus.xpm"

#define LINE_WIDTH 0.1

#define DEFAULT_NUMHANDLES 10
#define HANDLE_BUS (HANDLE_CUSTOM1)

typedef struct _BusPropertiesDialog BusPropertiesDialog;

typedef struct _Bus {
  Connection connection;

  int num_handles;
  Handle **handles;
  Point *parallel_points;
  Point real_ends[2];

  BusPropertiesDialog *properties_dialog;
} Bus;

struct _BusPropertiesDialog {
  GtkWidget *dialog;

  GtkSpinButton *num_handles_spinner;
};

static void bus_move_handle(Bus *bus, Handle *handle,
			    Point *to, HandleMoveReason reason);
static void bus_move(Bus *bus, Point *to);
static void bus_select(Bus *bus, Point *clicked_point,
		       Renderer *interactive_renderer);
static void bus_draw(Bus *bus, Renderer *renderer);
static Object *bus_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static real bus_distance_from(Bus *bus, Point *point);
static void bus_update_data(Bus *bus);
static void bus_destroy(Bus *bus);
static Object *bus_copy(Bus *bus);

static void bus_save(Bus *bus, int fd);
static Object *bus_load(int fd, int version);
static GtkWidget *bus_get_properties(Bus *bus);
static void bus_apply_properties(Bus *bus);


static ObjectTypeOps bus_type_ops =
{
  (CreateFunc) bus_create,
  (LoadFunc)   bus_load,
  (SaveFunc)   bus_save
};

ObjectType bus_type =
{
  "Standard - Bus",   /* name */
  0,                  /* version */
  (char **) bus_xpm,  /* pixmap */
  &bus_type_ops       /* ops */
};

SheetObject bus_sheetobj =
{
  "Standard - Bus",             /* type */
  "Ethernet bus.",        /* description */
  (char **) bus_xpm,     /* pixmap */
  NULL                   /* user_data */
};

static ObjectOps bus_ops = {
  (DestroyFunc)         bus_destroy,
  (DrawFunc)            bus_draw,
  (DistanceFunc)        bus_distance_from,
  (SelectFunc)          bus_select,
  (CopyFunc)            bus_copy,
  (MoveFunc)            bus_move,
  (MoveHandleFunc)      bus_move_handle,
  (GetPropertiesFunc)   bus_get_properties,
  (ApplyPropertiesFunc) bus_apply_properties,
  (IsEmptyFunc)         object_return_false
};


static void
bus_apply_properties(Bus *bus)
{
  BusPropertiesDialog *prop_dialog;
  int new_num_handles;
  int i;

  prop_dialog = bus->properties_dialog;

  new_num_handles = gtk_spin_button_get_value_as_int(prop_dialog->num_handles_spinner);

  if (new_num_handles != bus->num_handles) {
    if (new_num_handles > bus->num_handles) {
      /* Allocate more handles */
      bus->handles = g_realloc(bus->handles,
			       sizeof(Handle *)*new_num_handles);
      bus->parallel_points = g_realloc(bus->parallel_points,
				       sizeof(Point)*new_num_handles);

      for (i=bus->num_handles;i<new_num_handles;i++) {
	bus->handles[i] = g_new(Handle,1);
	bus->handles[i]->id = HANDLE_BUS;
	bus->handles[i]->type = HANDLE_MINOR_CONTROL;
	bus->handles[i]->connect_type = HANDLE_CONNECTABLE_NOBREAK;
	bus->handles[i]->connected_to = NULL;
	bus->handles[i]->pos.x =
	  0.5 * bus->connection.endpoints[0].x +
	  0.5 * bus->connection.endpoints[1].x;
	bus->handles[i]->pos.y =
	  0.5 * bus->connection.endpoints[0].y +
	  0.5 * bus->connection.endpoints[1].y;
	bus->handles[i]->pos.x += 0.5;
	bus->handles[i]->pos.y += 0.5;
	object_add_handle((Object *) bus, bus->handles[i]);
      }

      bus->num_handles = new_num_handles;
    } else {
      /* Delete old handles */
      for (i=new_num_handles;i<bus->num_handles;i++) {
	object_remove_handle((Object *) bus, bus->handles[i]);
	g_free(bus->handles[i]);
      }
      
      bus->handles = g_realloc(bus->handles,
			       sizeof(Handle *)*new_num_handles);
      bus->parallel_points = g_realloc(bus->parallel_points,
				       sizeof(Point)*new_num_handles);
      bus->num_handles = new_num_handles;
    }
  }
  bus_update_data(bus);
}

static GtkWidget *
bus_get_properties(Bus *bus)
{
  BusPropertiesDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *spinner;
  GtkWidget *hbox;
  GtkAdjustment *adj;

  if (bus->properties_dialog == NULL) {
  
    prop_dialog = g_new(BusPropertiesDialog, 1);
    bus->properties_dialog = prop_dialog;

    dialog = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = dialog;

    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new ("Number of handles:");
    gtk_misc_set_padding (GTK_MISC (label), 10, 10);
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    adj = (GtkAdjustment *)
      gtk_adjustment_new(10.0, 1.0, 99.0, 1.0, 0.0, 0.0);
    spinner = gtk_spin_button_new(adj, 1.0, 0);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spinner), FALSE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spinner), TRUE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinner),
			      (float) bus->num_handles);
    prop_dialog->num_handles_spinner = GTK_SPIN_BUTTON(spinner);
    gtk_box_pack_start(GTK_BOX (hbox), spinner, FALSE, TRUE, 0);
    gtk_widget_show (spinner);

    gtk_box_pack_start(GTK_BOX (dialog), hbox, FALSE, TRUE, 0);
    gtk_widget_show (hbox);

    gtk_widget_show (dialog);
  }

  
  return bus->properties_dialog->dialog;
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
	    Renderer *interactive_renderer)
{
  connection_update_handles(&bus->connection);
}

static void
bus_move_handle(Bus *bus, Handle *handle,
		Point *to, HandleMoveReason reason)
{
  Connection *conn = &bus->connection;
  Point *endpoints;
  static real *parallel=NULL;
  static real *perp=NULL;
  static int max_num=0;
  Point vhat, vhatperp;
  Point u;
  real vlen, vlen2;
  real len_scale;
  int i;

  if (bus->num_handles>max_num) {
    if (parallel!=NULL) {
      g_free(parallel);
      g_free(perp);
    }
    parallel = g_malloc(sizeof(real)*bus->num_handles);
    perp = g_malloc(sizeof(real)*bus->num_handles);
    max_num = bus->num_handles;
  }

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
    for (i=0;i<bus->num_handles;i++) {
      u = bus->handles[i]->pos;
      point_sub(&u, &endpoints[0]);
      parallel[i] = point_dot(&vhat, &u);
      perp[i] = point_dot(&vhatperp, &u);
    }
    
    connection_move_handle(&bus->connection, handle->id, to, reason);

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
    for (i=0;i<bus->num_handles;i++) {
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
}

static void
bus_move(Bus *bus, Point *to)
{
  Point delta;
  Point *endpoints = &bus->connection.endpoints[0]; 
  Object *obj = (Object *) bus;
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
}

static void
bus_draw(Bus *bus, Renderer *renderer)
{
  Point *endpoints;
  int i;
  
  assert(bus != NULL);
  assert(renderer != NULL);

  endpoints = &bus->real_ends[0];
  
  renderer->ops->set_linewidth(renderer, LINE_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_line(renderer,
			   &endpoints[0], &endpoints[1],
			   &color_black);

  for (i=0;i<bus->num_handles;i++) {
    renderer->ops->draw_line(renderer,
			     &bus->parallel_points[i],
			     &bus->handles[i]->pos,
			     &color_black);
  }
}

static Object *
bus_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Bus *bus;
  Connection *conn;
  Object *obj;
  Point defaultlen = { 5.0, 0.0 };
  int i;

  bus = g_malloc(sizeof(Bus));

  conn = &bus->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);
 
  obj = (Object *) bus;
  
  obj->type = &bus_type;
  obj->ops = &bus_ops;

  bus->num_handles = DEFAULT_NUMHANDLES;

  connection_init(conn, 2+bus->num_handles, 0);

  bus->handles = g_malloc(sizeof(Handle *)*bus->num_handles);
  bus->parallel_points = g_malloc(sizeof(Point)*bus->num_handles);
  for (i=0;i<bus->num_handles;i++) {
    bus->handles[i] = g_new(Handle,1);
    bus->handles[i]->id = HANDLE_BUS;
    bus->handles[i]->type = HANDLE_MINOR_CONTROL;
    bus->handles[i]->connect_type = HANDLE_CONNECTABLE_NOBREAK;
    bus->handles[i]->connected_to = NULL;
    bus->handles[i]->pos = *startpoint;
    bus->handles[i]->pos.x += 5*((real)i+1)/(bus->num_handles+1);
    bus->handles[i]->pos.y += (i%2==0)?1.0:-1.0;
    obj->handles[2+i] = bus->handles[i];
  }

  bus_update_data(bus);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return (Object *)bus;
}

static void
bus_destroy(Bus *bus)
{
  int i;
  connection_destroy(&bus->connection);
  for (i=0;i<bus->num_handles;i++)
    g_free(bus->handles[i]);
  g_free(bus->handles);
  g_free(bus->parallel_points);
}

static Object *
bus_copy(Bus *bus)
{
  Bus *newbus;
  Connection *conn, *newconn;
  Object *newobj;
  int i;
  
  conn = &bus->connection;
  
  newbus = g_malloc(sizeof(Bus));
  newconn = &newbus->connection;
  newobj = (Object *) newbus;
  
  connection_copy(conn, newconn);

  newbus->num_handles = bus->num_handles;

  newbus->handles = g_malloc(sizeof(Handle *)*newbus->num_handles);
  newbus->parallel_points = g_malloc(sizeof(Point)*newbus->num_handles);
  
  for (i=0;i<newbus->num_handles;i++) {
    newbus->handles[i] = g_new(Handle,1);
    *newbus->handles[i] = *bus->handles[i];
    newbus->handles[i]->connected_to = NULL;
    newobj->handles[2+i] = newbus->handles[i];
    newbus->parallel_points[i] = bus->parallel_points[i];
  }

  newbus->real_ends[0] = bus->real_ends[0];
  newbus->real_ends[1] = bus->real_ends[1];

  return (Object *)newbus;
}


static void
bus_update_data(Bus *bus)
{
  Connection *conn = &bus->connection;
  Object *obj = (Object *) bus;
  int i;
  Point u, v, vhat;
  Point *endpoints;
  real ulen;
  real min_par, max_par;
  
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
  /* fix boundingbox for bus_width: */
  obj->bounding_box.top -= LINE_WIDTH/2;
  obj->bounding_box.left -= LINE_WIDTH/2;
  obj->bounding_box.bottom += LINE_WIDTH/2;
  obj->bounding_box.right += LINE_WIDTH/2;

  connection_update_handles(conn);
}


static void
bus_save(Bus *bus, int fd)
{
  Connection *conn;
  int i;

  conn = &bus->connection;
 
  connection_save(conn, fd);

  write_int32(fd, bus->num_handles);
  for (i=0;i<bus->num_handles;i++) {
    write_point(fd, &bus->handles[i]->pos);
  }

  
}

static Object *
bus_load(int fd, int version)
{
  Bus *bus;
  Connection *conn;
  Object *obj;
  int i;

  bus = g_malloc(sizeof(Bus));

  conn = &bus->connection;
  obj = (Object *) bus;

  obj->type = &bus_type;
  obj->ops = &bus_ops;

  connection_load(conn, fd);

  bus->num_handles = read_int32(fd);

  connection_init(conn, 2 + bus->num_handles, 0);

  bus->handles = g_malloc(sizeof(Handle *)*bus->num_handles);
  bus->parallel_points = g_malloc(sizeof(Point)*bus->num_handles);
  for (i=0;i<bus->num_handles;i++) {
    bus->handles[i] = g_new(Handle,1);
    bus->handles[i]->id = HANDLE_BUS;
    bus->handles[i]->type = HANDLE_MINOR_CONTROL;
    bus->handles[i]->connect_type = HANDLE_CONNECTABLE_NOBREAK;
    bus->handles[i]->connected_to = NULL;
    read_point(fd, &bus->handles[i]->pos);
    obj->handles[2+i] = bus->handles[i];
  }

  bus_update_data(bus);

  return (Object *)bus;
}
