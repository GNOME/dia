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

#include "pixmaps/arc.xpm"

#define DEFAULT_WIDTH 0.25

#define HANDLE_MIDDLE HANDLE_CUSTOM1

typedef struct _Arc {
  Connection connection;

  Handle middle_handle;

  Color arc_color;
  
  real curve_distance;
  real line_width;

  /* Calculated parameters: */
  real radius;
  Point center;
  real angle1, angle2;
     
} Arc;

static void arc_move_handle(Arc *arc, Handle *handle,
			    Point *to, HandleMoveReason reason);
static void arc_move(Arc *arc, Point *to);
static void arc_select(Arc *arc, Point *clicked_point,
		       Renderer *interactive_renderer);
static void arc_draw(Arc *arc, Renderer *renderer);
static Object *arc_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static real arc_distance_from(Arc *arc, Point *point);
static void arc_update_data(Arc *arc);
static void arc_update_handles(Arc *arc);
static void arc_destroy(Arc *arc);
static Object *arc_copy(Arc *arc);

static void arc_save(Arc *arc, int fd);
static Object *arc_load(int fd, int version);

static ObjectTypeOps arc_type_ops =
{
  (CreateFunc) arc_create,
  (LoadFunc)   arc_load,
  (SaveFunc)   arc_save
};

ObjectType arc_type =
{
  "Standard - Arc",  /* name */
  0,                 /* version */
  (char **) arc_xpm, /* pixmap */
  
  &arc_type_ops      /* ops */
};

ObjectType *_arc_type = (ObjectType *) &arc_type;

static ObjectOps arc_ops = {
  (DestroyFunc)        arc_destroy,
  (DrawFunc)           arc_draw,
  (DistanceFunc)       arc_distance_from,
  (SelectFunc)         arc_select,
  (CopyFunc)           arc_copy,
  (MoveFunc)           arc_move,
  (MoveHandleFunc)     arc_move_handle,
  (ShowPropertiesFunc) object_show_properties_none_yet,
  (IsEmptyFunc)        object_return_false
};


static int
in_angle(real angle, real startangle, real endangle)
{
  if (startangle > endangle) {  /* passes 360 degrees */
    endangle += 360.0;
    if (angle<startangle)
      angle += 360;
  }
  return (angle>=startangle) && (angle<=endangle);
}

static real
arc_distance_from(Arc *arc, Point *point)
{
  Point *endpoints;
  Point from_center;
  real angle;
  real d, d2;
  
  endpoints = &arc->connection.endpoints[0];

  from_center = *point;
  point_sub(&from_center, &arc->center);

  angle = -atan2(from_center.y, from_center.x)*180.0/M_PI;
  if (angle<0)
    angle+=360.0;

  if (in_angle(angle, arc->angle1, arc->angle2)) {
    d = fabs(sqrt(point_dot(&from_center, &from_center)) - arc->radius);
    d -= arc->line_width/2.0;
    if (d<0)
      d = 0.0;
    return d;
  } else {
    d = distance_point_point(&endpoints[0], point);
    d2 = distance_point_point(&endpoints[1], point);

    return MIN(d,d2);
  }
}

static void
arc_select(Arc *arc, Point *clicked_point,
	   Renderer *interactive_renderer)
{
  arc_update_handles(arc);
}

static void
arc_update_handles(Arc *arc)
{
  Point *middle_pos;
  real dist,dx,dy;

  Connection *conn = &arc->connection;

  connection_update_handles(conn);
  
  middle_pos = &arc->middle_handle.pos;

  dx = conn->endpoints[1].x - conn->endpoints[0].x;
  dy = conn->endpoints[1].y - conn->endpoints[0].y;
  
  dist = sqrt(dx*dx + dy*dy);
  middle_pos->x =
    (conn->endpoints[0].x + conn->endpoints[1].x) / 2.0 -
    arc->curve_distance*dy/dist;
  middle_pos->y =
    (conn->endpoints[0].y + conn->endpoints[1].y) / 2.0 +
    arc->curve_distance*dx/dist;
}


static void
arc_move_handle(Arc *arc, Handle *handle,
		Point *to, HandleMoveReason reason)
{
  assert(arc!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_MIDDLE) {
    Point a,b;
    real tmp;

    b = *to;
    point_sub(&b, &arc->connection.endpoints[0]);
   
    a = arc->connection.endpoints[1];
    point_sub(&a, &arc->connection.endpoints[0]);

    tmp = point_dot(&a,&b);
    arc->curve_distance =
      sqrt(fabs(point_dot(&b,&b) - tmp*tmp/point_dot(&a,&a)));
    
    if (a.x*b.y - a.y*b.x < 0) 
      arc->curve_distance = -arc->curve_distance;

  } else {
    connection_move_handle(&arc->connection, handle->id, to, reason);
  }

  arc_update_data(arc);
}

static void
arc_move(Arc *arc, Point *to)
{
  Point start_to_end;
  Point *endpoints = &arc->connection.endpoints[0]; 

  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  arc_update_data(arc);
}

static void
arc_draw(Arc *arc, Renderer *renderer)
{
  Point *endpoints;
  real width;
    
  assert(arc != NULL);
  assert(renderer != NULL);

  endpoints = &arc->connection.endpoints[0];

  renderer->ops->set_linewidth(renderer, arc->line_width);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);
  
  /* Special case when almost line: */
  if (fabs(arc->curve_distance) <= 0.0001) {
    renderer->ops->draw_line(renderer,
			     &endpoints[0], &endpoints[1],
			     &color_black);
    return;
  }
  
  width = 2*arc->radius;
  
  renderer->ops->draw_arc(renderer,
			  &arc->center,
			  width, width,
			  arc->angle1, arc->angle2,
			  &arc->arc_color);
  
}

static Object *
arc_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Arc *arc;
  Connection *conn;
  Object *obj;
  Point defaultlen = { 1.0, 1.0 };

  arc = g_malloc(sizeof(Arc));

  arc->line_width =  attributes_get_default_linewidth();
  arc->curve_distance = 1.0;
  arc->arc_color = attributes_get_foreground(); 
  
  conn = &arc->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);
 
  obj = (Object *) arc;
  
  obj->type = &arc_type;;
  obj->ops = &arc_ops;
  
  connection_init(conn, 3, 0);

  obj->handles[2] = &arc->middle_handle;
  arc->middle_handle.id = HANDLE_MIDDLE;
  arc->middle_handle.type = HANDLE_MINOR_CONTROL;
  arc->middle_handle.connectable = FALSE;
  arc->middle_handle.connected_to = NULL;

  arc_update_data(arc);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return (Object *)arc;
}

static void
arc_destroy(Arc *arc)
{
  connection_destroy(&arc->connection);
}

static Object *
arc_copy(Arc *arc)
{
  Arc *newarc;
  Connection *conn, *newconn;
  Object *newobj;
  
  conn = &arc->connection;
  
  newarc = g_malloc(sizeof(Arc));
  newconn = &newarc->connection;
  newobj = (Object *) newarc;

  connection_copy(conn, newconn);

  newarc->arc_color = arc->arc_color;
  newarc->curve_distance = arc->curve_distance;
  newarc->line_width = arc->line_width;
  newarc->radius = arc->radius;
  newarc->center = arc->center;
  newarc->angle1 = arc->angle1;
  newarc->angle2 = arc->angle2;

  newobj->handles[2] = &newarc->middle_handle;
  
  newarc->middle_handle = arc->middle_handle;

  return (Object *)newarc;
}

static void
arc_update_data(Arc *arc)
{
  Connection *conn = &arc->connection;
  Object *obj = (Object *) arc;
  Point *endpoints;
  real x1,y1,x2,y2,xc,yc;
  real lensq, alpha, radius;
  real angle1, angle2;
  
  endpoints = &arc->connection.endpoints[0];
  x1 = endpoints[0].x;
  y1 = endpoints[0].y;
  x2 = endpoints[1].x;
  y2 = endpoints[1].y;
  
  lensq = (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1);
  radius = lensq/(8*arc->curve_distance) + arc->curve_distance/2.0;

  alpha = (radius - arc->curve_distance) / sqrt(lensq);
  
  xc = (x1 + x2) / 2.0 + (y2 - y1)*alpha;
  yc = (y1 + y2) / 2.0 + (x1 - x2)*alpha;

  angle1 = -atan2(y1-yc, x1-xc)*180.0/M_PI;
  if (angle1<0)
    angle1+=360.0;
  angle2 = -atan2(y2-yc, x2-xc)*180.0/M_PI;
  if (angle2<0)
    angle2+=360.0;

  if (radius<0.0) {
    real tmp;
    tmp = angle1;
    angle1 = angle2;
    angle2 = tmp;
    radius = -radius;
  }
  
  arc->radius = radius;
  arc->center.x = xc; arc->center.y = yc;
  arc->angle1 = angle1;
  arc->angle2 = angle2;
  
  connection_update_boundingbox(conn);
  /* fix boundingbox for line_width: */
  if (in_angle(0, arc->angle1, arc->angle2)) {
    obj->bounding_box.right = arc->center.x + arc->radius;
  }
  if (in_angle(90, arc->angle1, arc->angle2)) {
    obj->bounding_box.top = arc->center.y - arc->radius;
  }
  if (in_angle(180, arc->angle1, arc->angle2)) {
    obj->bounding_box.left = arc->center.x - arc->radius;
  }
  if (in_angle(270, arc->angle1, arc->angle2)) {
    obj->bounding_box.bottom = arc->center.y + arc->radius;
  }
  
  obj->bounding_box.top -= arc->line_width/2;
  obj->bounding_box.left -= arc->line_width/2;
  obj->bounding_box.bottom += arc->line_width/2;
  obj->bounding_box.right += arc->line_width/2;

  obj->position = conn->endpoints[0];
  
  arc_update_handles(arc);
}

static void
arc_save(Arc *arc, int fd)
{
  connection_save(&arc->connection, fd);

  write_color(fd, &arc->arc_color);
  write_real(fd, arc->curve_distance);
  write_real(fd, arc->line_width);
}

static Object *
arc_load(int fd, int version)
{
  Arc *arc;
  Connection *conn;
  Object *obj;

  arc = g_malloc(sizeof(Arc));

  conn = &arc->connection;
  obj = (Object *) arc;

  obj->type = &arc_type;;
  obj->ops = &arc_ops;

  connection_load(conn, fd);

  read_color(fd, &arc->arc_color);
  arc->curve_distance = read_real(fd);
  arc->line_width = read_real(fd);

  connection_init(conn, 3, 0);

  obj->handles[2] = &arc->middle_handle;
  arc->middle_handle.id = HANDLE_MIDDLE;
  arc->middle_handle.type = HANDLE_MINOR_CONTROL;
  arc->middle_handle.connectable = FALSE;
  arc->middle_handle.connected_to = NULL;

  arc_update_data(arc);

  return (Object *)arc;
}


