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
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "files.h"

#include "pixmaps/ellipse.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.15

typedef struct _Ellipse Ellipse;

struct _Ellipse {
  Element element;

  ConnectionPoint connections[8];
  
  real border_width;
  Color border_color;
  Color inner_color;
};

static real ellipse_distance_from(Ellipse *ellipse, Point *point);
static void ellipse_select(Ellipse *ellipse, Point *clicked_point,
			   Renderer *interactive_renderer);
static void ellipse_move_handle(Ellipse *ellipse, Handle *handle,
				Point *to, HandleMoveReason reason);
static void ellipse_move(Ellipse *ellipse, Point *to);
static void ellipse_draw(Ellipse *ellipse, Renderer *renderer);
static void ellipse_update_data(Ellipse *ellipse);
static Object *ellipse_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void ellipse_destroy(Ellipse *ellipse);
static Object *ellipse_copy(Ellipse *ellipse);

static void ellipse_save(Ellipse *ellipse, int fd);
static Object *ellipse_load(int fd, int version);

static ObjectTypeOps ellipse_type_ops =
{
  (CreateFunc) ellipse_create,
  (LoadFunc)   ellipse_load,
  (SaveFunc)   ellipse_save
};

ObjectType ellipse_type =
{
  "Standard - Ellipse",   /* name */
  0,                      /* version */
  (char **) ellipse_xpm,  /* pixmap */
  
  &ellipse_type_ops       /* ops */
};

ObjectType *_ellipse_type = (ObjectType *) &ellipse_type;

static ObjectOps ellipse_ops = {
  (DestroyFunc)         ellipse_destroy,
  (DrawFunc)            ellipse_draw,
  (DistanceFunc)        ellipse_distance_from,
  (SelectFunc)          ellipse_select,
  (CopyFunc)            ellipse_copy,
  (MoveFunc)            ellipse_move,
  (MoveHandleFunc)      ellipse_move_handle,
  (GetPropertiesFunc)   object_return_null,
  (ApplyPropertiesFunc) object_return_void,
  (IsEmptyFunc)         object_return_false
};

static real
ellipse_distance_from(Ellipse *ellipse, Point *point)
{
  Object *obj = &ellipse->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
ellipse_select(Ellipse *ellipse, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  element_update_handles(&ellipse->element);
}

static void
ellipse_move_handle(Ellipse *ellipse, Handle *handle,
		    Point *to, HandleMoveReason reason)
{
  assert(ellipse!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
  element_move_handle(&ellipse->element, handle->id, to, reason);
  ellipse_update_data(ellipse);
}

static void
ellipse_move(Ellipse *ellipse, Point *to)
{
  ellipse->element.corner = *to;
  ellipse_update_data(ellipse);
}

static void
ellipse_draw(Ellipse *ellipse, Renderer *renderer)
{
  Point center;
  Element *elem;
  
  assert(ellipse != NULL);
  assert(renderer != NULL);

  elem = &ellipse->element;

  center.x = elem->corner.x + elem->width/2;
  center.y = elem->corner.y + elem->height/2;
  
  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);

  renderer->ops->fill_ellipse(renderer, 
			     &center,
			     elem->width, elem->height,
			     &ellipse->inner_color);

  renderer->ops->set_linewidth(renderer, ellipse->border_width);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);

  renderer->ops->draw_ellipse(renderer, 
			  &center,
			  elem->width, elem->height,
			  &ellipse->border_color);
}

static void
ellipse_update_data(Ellipse *ellipse)
{
  Element *elem = &ellipse->element;
  Object *obj = (Object *) ellipse;
  Point center;
  real half_x, half_y;

  center.x = elem->corner.x + elem->width / 2.0;
  center.y = elem->corner.y + elem->height / 2.0;
  
  half_x = elem->width * M_SQRT1_2 / 2.0;
  half_y = elem->height * M_SQRT1_2 / 2.0;
    
  /* Update connections: */
  ellipse->connections[0].pos.x = center.x - half_x;
  ellipse->connections[0].pos.y = center.y - half_y;
  ellipse->connections[1].pos.x = center.x;
  ellipse->connections[1].pos.y = elem->corner.y;
  ellipse->connections[2].pos.x = center.x + half_x;
  ellipse->connections[2].pos.y = center.y - half_y;
  ellipse->connections[3].pos.x = elem->corner.x;
  ellipse->connections[3].pos.y = center.y;
  ellipse->connections[4].pos.x = elem->corner.x + elem->width;
  ellipse->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  ellipse->connections[5].pos.x = center.x - half_x;
  ellipse->connections[5].pos.y = center.y + half_y;
  ellipse->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  ellipse->connections[6].pos.y = elem->corner.y + elem->height;
  ellipse->connections[7].pos.x = center.x + half_x;
  ellipse->connections[7].pos.y = center.y + half_y;

  element_update_boundingbox(elem);
  /* fix boundingellipse for line_width: */
  obj->bounding_box.top -= ellipse->border_width/2;
  obj->bounding_box.left -= ellipse->border_width/2;
  obj->bounding_box.bottom += ellipse->border_width/2;
  obj->bounding_box.right += ellipse->border_width/2;

  obj->position = elem->corner;

  element_update_handles(elem);
  
}

static Object *
ellipse_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Ellipse *ellipse;
  Element *elem;
  Object *obj;
  int i;

  ellipse = g_malloc(sizeof(Ellipse));
  elem = &ellipse->element;
  obj = (Object *) ellipse;
  
  obj->type = &ellipse_type;

  obj->ops = &ellipse_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_WIDTH;

  ellipse->border_width =  attributes_get_default_linewidth();
  ellipse->border_color = attributes_get_foreground();
  ellipse->inner_color = attributes_get_background();

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &ellipse->connections[i];
    ellipse->connections[i].object = obj;
    ellipse->connections[i].connected = NULL;
  }
  ellipse_update_data(ellipse);

  *handle1 = NULL;
  *handle2 = obj->handles[0];
  return (Object *)ellipse;
}

static void
ellipse_destroy(Ellipse *ellipse)
{
  element_destroy(&ellipse->element);
}

static Object *
ellipse_copy(Ellipse *ellipse)
{
  int i;
  Ellipse *newellipse;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &ellipse->element;
  
  newellipse = g_malloc(sizeof(Ellipse));
  newelem = &newellipse->element;
  newobj = (Object *) newellipse;

  element_copy(elem, newelem);

  newellipse->border_width = ellipse->border_width;
  newellipse->border_color = ellipse->border_color;
  newellipse->inner_color = ellipse->inner_color;

  for (i=0;i<8;i++) {
    newobj->connections[i] = &newellipse->connections[i];
    newellipse->connections[i].object = newobj;
    newellipse->connections[i].connected = NULL;
    newellipse->connections[i].pos = ellipse->connections[i].pos;
    newellipse->connections[i].last_pos = ellipse->connections[i].last_pos;
  }
  return (Object *)newellipse;
}


static void
ellipse_save(Ellipse *ellipse, int fd)
{
  element_save(&ellipse->element, fd);

  write_real(fd, ellipse->border_width);
  write_color(fd, &ellipse->border_color);
  write_color(fd, &ellipse->inner_color);
 
}

static Object *ellipse_load(int fd, int version)
{
  Ellipse *ellipse;
  Element *elem;
  Object *obj;
  int i;

  ellipse = g_malloc(sizeof(Ellipse));
  elem = &ellipse->element;
  obj = (Object *) ellipse;
  
  obj->type = &ellipse_type;
  obj->ops = &ellipse_ops;

  element_load(elem, fd);

  ellipse->border_width = read_real(fd);
  read_color(fd, &ellipse->border_color);
  read_color(fd, &ellipse->inner_color);

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &ellipse->connections[i];
    ellipse->connections[i].object = obj;
    ellipse->connections[i].connected = NULL;
  }
  ellipse_update_data(ellipse);

  return (Object *)ellipse;
}




