/* xxxxxx -- an diagram creation/manipulation program
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

#include "pixmaps/zigzag.xpm"

#define DEFAULT_WIDTH 0.15

#define HANDLE_MIDDLE HANDLE_CUSTOM1

typedef struct _Zigzagline {
  Connection connection;

  Handle middle_handle;

  Color line_color;

  int horizontal;
  real middle_pos;

  real line_width;

} Zigzagline;

static void zigzagline_move_handle(Zigzagline *zigzagline, Handle *handle,
				   Point *to, HandleMoveReason reason);
static void zigzagline_move(Zigzagline *zigzagline, Point *to);
static void zigzagline_select(Zigzagline *zigzagline, Point *clicked_point,
			      Renderer *interactive_renderer);
static void zigzagline_draw(Zigzagline *zigzagline, Renderer *renderer);
static Object *zigzagline_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real zigzagline_distance_from(Zigzagline *zigzagline, Point *point);
static void zigzagline_update_data(Zigzagline *zigzagline);
static void zigzagline_update_handles(Zigzagline *zigzagline);
static void zigzagline_destroy(Zigzagline *zigzagline);
static Object *zigzagline_copy(Zigzagline *zigzagline);

static void zigzagline_save(Zigzagline *zigzagline, int fd);
static Object *zigzagline_load(int fd, int version);

static ObjectTypeOps zigzagline_type_ops =
{
  (CreateFunc)zigzagline_create,   /* create */
  (LoadFunc)  zigzagline_load,     /* load */
  (SaveFunc)  zigzagline_save      /* save */
};

static ObjectType zigzagline_type =
{
  "Standard - ZigZagLine",   /* name */
  0,                         /* version */
  (char **) zigzag_xpm,      /* pixmap */
  
  &zigzagline_type_ops       /* ops */
};

ObjectType *_zigzagline_type = (ObjectType *) &zigzagline_type;


static ObjectOps zigzagline_ops = {
  (DestroyFunc)        zigzagline_destroy,
  (DrawFunc)           zigzagline_draw,
  (DistanceFunc)       zigzagline_distance_from,
  (SelectFunc)         zigzagline_select,
  (CopyFunc)           zigzagline_copy,
  (MoveFunc)           zigzagline_move,
  (MoveHandleFunc)     zigzagline_move_handle,
  (ShowPropertiesFunc) object_show_properties_none_yet,
  (IsEmptyFunc)        object_return_false
};


static real
zigzagline_distance_from(Zigzagline *zigzagline, Point *point)
{
  Point *endpoints;
  Point a,b;
  real dist;
  endpoints = &zigzagline->connection.endpoints[0]; 

  if (zigzagline->horizontal) {
    a.x = endpoints[0].x;
    a.y = zigzagline->middle_pos;
    b.x = endpoints[1].x;
    b.y = zigzagline->middle_pos;
  } else {
    a.x = zigzagline->middle_pos;
    a.y = endpoints[0].y;
    b.x = zigzagline->middle_pos;
    b.y = endpoints[1].y;
  }

  dist = distance_line_point( &endpoints[0], &a,
			      zigzagline->line_width, point);
  dist = MIN(dist, distance_line_point( &a, &b,
					zigzagline->line_width, point));
  dist = MIN(dist, distance_line_point( &b, &endpoints[1],
					zigzagline->line_width, point));
  
  return dist;
}

static void
zigzagline_select(Zigzagline *zigzagline, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  zigzagline_update_handles(zigzagline);
}

static void
zigzagline_update_handles(Zigzagline *zigzagline)
{
  Connection *conn = &zigzagline->connection;

  connection_update_handles(&zigzagline->connection);

  if (zigzagline->horizontal) {
    zigzagline->middle_handle.pos.x =
      conn->endpoints[0].x*0.5 + conn->endpoints[1].x*0.5;
    zigzagline->middle_handle.pos.y = zigzagline->middle_pos;
  } else {
    zigzagline->middle_handle.pos.x = zigzagline->middle_pos;
    zigzagline->middle_handle.pos.y =
      conn->endpoints[0].y*0.5 + conn->endpoints[1].y*0.5;
  }
}


static void
zigzagline_move_handle(Zigzagline *zigzagline, Handle *handle,
		       Point *to, HandleMoveReason reason)
{
  assert(zigzagline!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_MIDDLE) {
    if (zigzagline->horizontal) {
      zigzagline->middle_pos = to->y;
    } else {
      zigzagline->middle_pos = to->x;
    }
  } else {
    connection_move_handle(&zigzagline->connection, handle->id, to, reason);
  }

  zigzagline_update_data(zigzagline);
}


static void
zigzagline_move(Zigzagline *zigzagline, Point *to)
{
  Point start_to_end;
  Point *endpoints = &zigzagline->connection.endpoints[0]; 
  real middle_delta;
  
  if (zigzagline->horizontal) {
    middle_delta = zigzagline->middle_pos - endpoints[0].y;
  } else {
    middle_delta = zigzagline->middle_pos - endpoints[0].x;
  }
  
  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  if (zigzagline->horizontal) {
    zigzagline->middle_pos =  endpoints[0].y + middle_delta;
  } else {
    zigzagline->middle_pos =  endpoints[0].x + middle_delta;
  }
  
  zigzagline_update_data(zigzagline);
}

static void
zigzagline_draw(Zigzagline *zigzagline, Renderer *renderer)
{
  Point *endpoints;
  Point points[4];
  
  assert(zigzagline != NULL);
  assert(renderer != NULL);

  endpoints = &zigzagline->connection.endpoints[0];

  points[0] = endpoints[0];
  points[3] = endpoints[1];
  
  if (zigzagline->horizontal) {
    points[1].x = endpoints[0].x;
    points[1].y = zigzagline->middle_pos;
    points[2].x = endpoints[1].x;
    points[2].y = zigzagline->middle_pos;
  } else {
    points[1].x = zigzagline->middle_pos;
    points[1].y = endpoints[0].y;
    points[2].x = zigzagline->middle_pos;
    points[2].y = endpoints[1].y;
  }

  renderer->ops->set_linewidth(renderer, zigzagline->line_width);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  
  renderer->ops->draw_polyline(renderer,
			       points, 4, 
			       &zigzagline->line_color);
}

static Object *
zigzagline_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Zigzagline *zigzagline;
  Connection *conn;
  Object *obj;
  Point defaultlen = { 1.0, 1.0 };

  zigzagline = g_malloc(sizeof(Zigzagline));

  zigzagline->line_width =  attributes_get_default_linewidth();
  zigzagline->line_color = attributes_get_foreground();
  
  conn = &zigzagline->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);

  zigzagline->horizontal = TRUE;

  if (zigzagline->horizontal) {
    zigzagline->middle_pos =
      conn->endpoints[0].y*0.5 + conn->endpoints[1].y*0.5;
  } else {
    zigzagline->middle_pos =
      conn->endpoints[0].x*0.5 + conn->endpoints[1].x*0.5;
  }
  
  obj = (Object *) zigzagline;
  
  obj->type = &zigzagline_type;
  obj->ops = &zigzagline_ops;
  
  connection_init(conn, 3, 0);

  obj->handles[2] = &zigzagline->middle_handle;
  zigzagline->middle_handle.id = HANDLE_MIDDLE;
  zigzagline->middle_handle.type = HANDLE_MINOR_CONTROL;
  zigzagline->middle_handle.connectable = FALSE;
  zigzagline->middle_handle.connected_to = NULL;
  
  zigzagline_update_data(zigzagline);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return (Object *)zigzagline;
}

static void
zigzagline_destroy(Zigzagline *zigzagline)
{
  connection_destroy(&zigzagline->connection);
}

static Object *
zigzagline_copy(Zigzagline *zigzagline)
{
  Zigzagline *newzigzagline;
  Connection *conn, *newconn;
  Object *newobj;
  
  conn = &zigzagline->connection;
 
  newzigzagline = g_malloc(sizeof(Zigzagline));
  newconn = &newzigzagline->connection;
  newobj = (Object *) newzigzagline;

  connection_copy(conn, newconn);

  newzigzagline->line_color = zigzagline->line_color;
  newzigzagline->horizontal = zigzagline->horizontal;
  newzigzagline->middle_pos = zigzagline->middle_pos;
  newzigzagline->line_width = zigzagline->line_width;

  newobj->handles[2] = &newzigzagline->middle_handle;
  
  newzigzagline->middle_handle = zigzagline->middle_handle;

  return (Object *)newzigzagline;
}

static void
zigzagline_update_data(Zigzagline *zigzagline)
{
  Connection *conn = &zigzagline->connection;
  Object *obj = (Object *) zigzagline;

  
  connection_update_boundingbox(conn);
  /* fix boundingbox for line_width: */
  obj->bounding_box.top -= zigzagline->line_width/2;
  obj->bounding_box.left -= zigzagline->line_width/2;
  obj->bounding_box.bottom += zigzagline->line_width/2;
  obj->bounding_box.right += zigzagline->line_width/2;

  obj->position = conn->endpoints[0];
  
  zigzagline_update_handles(zigzagline);
}

static void
zigzagline_save(Zigzagline *zigzagline, int fd)
{
  connection_save(&zigzagline->connection, fd);

  write_color(fd, &zigzagline->line_color);
  write_int32(fd, zigzagline->horizontal);
  write_real(fd, zigzagline->middle_pos);
  write_real(fd, zigzagline->line_width);
}

static Object *
zigzagline_load(int fd, int version)
{
  Zigzagline *zigzagline;
  Connection *conn;
  Object *obj;

  zigzagline = g_malloc(sizeof(Zigzagline));

  conn = &zigzagline->connection;
  obj = (Object *) zigzagline;
  
  obj->type = &zigzagline_type;
  obj->ops = &zigzagline_ops;

  connection_load(conn, fd);

  read_color(fd, &zigzagline->line_color);
  zigzagline->horizontal = read_int32(fd);
  zigzagline->middle_pos = read_real(fd);
  zigzagline->line_width = read_real(fd);

  connection_init(conn, 3, 0);

  obj->handles[2] = &zigzagline->middle_handle;
  zigzagline->middle_handle.id = HANDLE_MIDDLE;
  zigzagline->middle_handle.type = HANDLE_MINOR_CONTROL;
  zigzagline->middle_handle.connectable = FALSE;
  zigzagline->middle_handle.connected_to = NULL;
  
  zigzagline_update_data(zigzagline);

  return (Object *)zigzagline;
}




