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

#include <config.h>

#include "render_object.h"
#include "intl.h"

static real rendobj_distance_from(RenderObject *rend_obj, Point *point);
static void rendobj_select(RenderObject *rend_obj, Point *clicked_point,
			   Renderer *interactive_renderer);
static void rendobj_move_handle(RenderObject *rend_obj, Handle *handle,
				Point *to, HandleMoveReason reason);
static void rendobj_move(RenderObject *rend_obj, Point *to);
static void rendobj_draw(RenderObject *rend_obj, Renderer *renderer);
static void rendobj_update_data(RenderObject *rend_obj);
static void rendobj_destroy(RenderObject *rend_obj);
static Object *rendobj_copy(RenderObject *rend_obj);

static ObjectOps rendobj_ops = {
  (DestroyFunc)         rendobj_destroy,
  (DrawFunc)            rendobj_draw,
  (DistanceFunc)        rendobj_distance_from,
  (SelectFunc)          rendobj_select,
  (CopyFunc)            rendobj_copy,
  (MoveFunc)            rendobj_move,
  (MoveHandleFunc)      rendobj_move_handle,
  (GetPropertiesFunc)   object_return_null,
  (ApplyPropertiesFunc) object_return_void,
  (ObjectMenuFunc)      NULL
};

Object *
new_render_object(Point *startpoint,
		  Handle **handle1,
		  Handle **handle2,
		  const RenderObjectDescriptor *desc)
{
  RenderObject *rend_obj;
  Element *elem;
  Object *obj;
  int i;

  rend_obj = g_malloc(sizeof(RenderObject));
  elem = &rend_obj->element;
  obj = &elem->object;

  obj->type = desc->obj_type;
  obj->ops = &rendobj_ops;
  
  elem->corner = *startpoint;
  elem->width = desc->width;
  elem->height = desc->height;

  rend_obj->desc = desc;
  rend_obj->magnify = 1.0;

  if (desc->use_text) {
    rend_obj->text = new_text("",
			      desc->initial_font,
			      desc->initial_font_height,
			      (Point *)&desc->text_pos,
			      &color_black,
			      desc->initial_alignment);
  } else {
    rend_obj->text = NULL;
  }

  element_init(elem, 8, desc->num_connection_points);

  rend_obj->connections = g_new(ConnectionPoint,
				desc->num_connection_points);
  
  for (i=0;i<desc->num_connection_points;i++) {
    obj->connections[i] = &rend_obj->connections[i];
    rend_obj->connections[i].object = obj;
    rend_obj->connections[i].connected = NULL;
  }

  rendobj_update_data(rend_obj);

  *handle1 = obj->handles[3];
  *handle2 = obj->handles[7];  
  return (Object *)rend_obj;
}


void render_object_save(RenderObject *rend_obj, ObjectNode obj_node)
{
  element_save(&rend_obj->element, obj_node);

  data_add_real(new_attribute(obj_node, "magnify"),
		rend_obj->magnify);
  
  if (rend_obj->desc->use_text) {
    data_add_text(new_attribute(obj_node, "text"),
		  rend_obj->text);
  }
}

Object *render_object_load(ObjectNode obj_node, 
			   const RenderObjectDescriptor *desc)
{
  RenderObject *rend_obj;
  Element *elem;
  Object *obj;
  AttributeNode attr;
  int i;
  Point startpoint = {0.0, 0.0};

  rend_obj = g_malloc(sizeof(RenderObject));
  elem = &rend_obj->element;
  obj = &elem->object;
  
  obj->type = desc->obj_type;
  obj->ops = &rendobj_ops;

  element_load(elem, obj_node);

  rend_obj->desc = desc;

  rend_obj->magnify =  1.0;
  attr = object_find_attribute(obj_node, "magnify");
  if (attr != NULL)
    rend_obj->magnify = data_real( attribute_first_data(attr) );

  if (desc->use_text) {
    attr = object_find_attribute(obj_node, "text");
    if (attr != NULL) {
	    rend_obj->text = data_text( attribute_first_data(attr) );
    } else {
	    /* choose default font name for your locale. see also font_data structure
	       in lib/font.c. if "Courier" works for you, it would be better.  */
	    rend_obj->text = new_text("", font_getfont(_("Courier")), 1.0,
				      &startpoint, &color_black, ALIGN_CENTER);
    }
  }

  element_init(elem, 8, desc->num_connection_points);

  rend_obj->connections = g_new(ConnectionPoint,
				desc->num_connection_points);
  
  for (i=0;i<desc->num_connection_points;i++) {
    obj->connections[i] = &rend_obj->connections[i];
    rend_obj->connections[i].object = obj;
    rend_obj->connections[i].connected = NULL;
  }

  rendobj_update_data(rend_obj);

  return (Object *)rend_obj;
}

static void
rendobj_destroy(RenderObject *rend_obj)
{
  element_destroy(&rend_obj->element);
  if (rend_obj->desc->use_text)
    text_destroy(rend_obj->text);
  g_free(rend_obj->connections);
}

static Object *
rendobj_copy(RenderObject *rend_obj)
{
  int i;
  RenderObject *newrend_obj;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &rend_obj->element;
  
  newrend_obj = g_malloc(sizeof(RenderObject));
  newelem = &newrend_obj->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newrend_obj->desc = rend_obj->desc;
  newrend_obj->magnify = rend_obj->magnify;
  if (rend_obj->desc->use_text) {
    newrend_obj->text = text_copy(rend_obj->text);
  } else {
    newrend_obj->text = NULL;
  }
  
  newrend_obj->connections = g_new(ConnectionPoint,
				   rend_obj->desc->num_connection_points);
  
  for (i=0;i<rend_obj->desc->num_connection_points;i++) {
    newobj->connections[i] = &newrend_obj->connections[i];
    newrend_obj->connections[i].object = newobj;
    newrend_obj->connections[i].pos = rend_obj->connections[i].pos;
    newrend_obj->connections[i].last_pos = rend_obj->connections[i].last_pos;
    newrend_obj->connections[i].connected = NULL;
  }

  rendobj_update_data(newrend_obj);

  return (Object *)newrend_obj;  
}

static real
rendobj_distance_from(RenderObject *rend_obj, Point *point)
{
  Element *elem = &rend_obj->element;
  Rectangle rect;
  real dist;
  real text_dist;
  
  /* Todo: Make this better, more changeable by the user. */
  
  rect.left = elem->corner.x;
  rect.right = elem->corner.x + elem->width;
  rect.top = elem->corner.y ;
  rect.bottom = elem->corner.y + elem->height;

  dist = distance_rectangle_point(&rect, point);
  if (rend_obj->desc->use_text) {
    text_dist = text_distance_from(rend_obj->text, point);
    if (text_dist<dist)
      dist = text_dist;
  }
  return dist;
}

static void
rendobj_select(RenderObject *rend_obj, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  if (rend_obj->desc->use_text) {
    text_set_cursor(rend_obj->text, clicked_point, interactive_renderer);
    text_grab_focus(rend_obj->text, (Object *)rend_obj);
  }
  
  element_update_handles(&rend_obj->element);
}

static void
rendobj_move_handle(RenderObject *rend_obj, Handle *handle,
		    Point *to, HandleMoveReason reason)
{
  element_move_handle_aspect(&rend_obj->element, handle->id, to,
			     rend_obj->desc->width/rend_obj->desc->height);

  rendobj_update_data(rend_obj);
}

static void
rendobj_move(RenderObject *rend_obj, Point *to)
{
  Point p;

  p = rend_obj->desc->move_position;
  rend_obj->element.corner = *to;
  point_scale(&p, rend_obj->magnify);
  point_sub(&rend_obj->element.corner,
	    &p);
  
  rendobj_update_data(rend_obj);
}

static void
rendobj_draw(RenderObject *rend_obj, Renderer *renderer)
{
  render_store_render(rend_obj->desc->store,
		      renderer,
		      &rend_obj->element.corner,
		      rend_obj->magnify);
  if ( (rend_obj->desc->use_text) &&
       (!text_is_empty(rend_obj->text)) )
    text_draw(rend_obj->text, renderer);
}

static void
rendobj_update_data(RenderObject *rend_obj)
{
  Point p;
  const RenderObjectDescriptor *desc;
  Element *elem;
  Object *obj;
  int i;

  elem = &rend_obj->element;
  obj = &elem->object;
  desc = rend_obj->desc;

  rend_obj->magnify = elem->width / rend_obj->desc->width;
  
  /* Update connections: */
  for (i=0;i<desc->num_connection_points;i++) {
    rend_obj->connections[i].pos = desc->connection_points[i];
    point_scale(&rend_obj->connections[i].pos, rend_obj->magnify);
    point_add(&rend_obj->connections[i].pos, &elem->corner);
  }

  if (desc->use_text) {
    p = desc->text_pos;
    point_scale(&p, rend_obj->magnify);
    point_add(&p, &elem->corner);
    p.y += rend_obj->text->ascent;
    text_set_position(rend_obj->text, &p);
  }
  
  element_update_boundingbox(elem);
  /* fix boundingbox for extra_border: */
  obj->bounding_box.top -= desc->extra_border;
  obj->bounding_box.left -= desc->extra_border;
  obj->bounding_box.bottom += desc->extra_border;
  obj->bounding_box.right += desc->extra_border;

  if (desc->use_text) {
    Rectangle text_box;
    
    text_calc_boundingbox(rend_obj->text, &text_box);
    rectangle_union(&obj->bounding_box, &text_box);
  }
  
  obj->position = elem->corner;

  p = desc->move_position;
  point_scale(&p, rend_obj->magnify);
  point_add(&obj->position, &p);
  
  element_update_handles(elem);
}
