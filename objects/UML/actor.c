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

#include "object.h"
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "sheet.h"
#include "text.h"

#include "pixmaps/actor.xpm"

typedef struct _Actor Actor;
struct _Actor {
  Element element;

  ConnectionPoint connections[8];

  Text *text;
};

#define ACTOR_WIDTH 2.2
#define ACTOR_HEIGHT 4.6
#define ACTOR_HEAD 0.6
#define ACTOR_BODY 4.0
#define ACTOR_LINEWIDTH 0.1
#define ACTOR_MARGIN_X 0.3
#define ACTOR_MARGIN_Y 0.3

static real actor_distance_from(Actor *actor, Point *point);
static void actor_select(Actor *actor, Point *clicked_point,
			Renderer *interactive_renderer);
static void actor_move_handle(Actor *actor, Handle *handle,
			     Point *to, HandleMoveReason reason);
static void actor_move(Actor *actor, Point *to);
static void actor_draw(Actor *actor, Renderer *renderer);
static Object *actor_create(Point *startpoint,
			   void *user_data,
			   Handle **handle1,
			   Handle **handle2);
static void actor_destroy(Actor *actor);
static Object *actor_copy(Actor *actor);

static void actor_save(Actor *actor, ObjectNode obj_node);
static Object *actor_load(ObjectNode obj_node, int version);

static void actor_update_data(Actor *actor);

static ObjectTypeOps actor_type_ops =
{
  (CreateFunc) actor_create,
  (LoadFunc)   actor_load,
  (SaveFunc)   actor_save
};

ObjectType actor_type =
{
  "UML - Actor",   /* name */
  0,                      /* version */
  (char **) actor_xpm,  /* pixmap */
  
  &actor_type_ops       /* ops */
};

SheetObject actor_sheetobj =
{
  "UML - Actor",             /* type */
  "Create an actor",           /* description */
  (char **) actor_xpm,     /* pixmap */

  NULL                       /* user_data */
};

static ObjectOps actor_ops = {
  (DestroyFunc)         actor_destroy,
  (DrawFunc)            actor_draw,
  (DistanceFunc)        actor_distance_from,
  (SelectFunc)          actor_select,
  (CopyFunc)            actor_copy,
  (MoveFunc)            actor_move,
  (MoveHandleFunc)      actor_move_handle,
  (GetPropertiesFunc)   object_return_null,
  (ApplyPropertiesFunc) object_return_void,
  (IsEmptyFunc)         object_return_false
};

static real
actor_distance_from(Actor *actor, Point *point)
{
  Object *obj = &actor->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
actor_select(Actor *actor, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  text_set_cursor(actor->text, clicked_point, interactive_renderer);
  text_grab_focus(actor->text, (Object *)actor);
  element_update_handles(&actor->element);
}

static void
actor_move_handle(Actor *actor, Handle *handle,
		 Point *to, HandleMoveReason reason)
{
  assert(actor!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
}

static void
actor_move(Actor *actor, Point *to)
{
  Element *elem = &actor->element;
  
  elem->corner = *to;
  elem->corner.x -= elem->width/2.0;
  elem->corner.y -= ACTOR_BODY;

  actor_update_data(actor);
}

static void
actor_draw(Actor *actor, Renderer *renderer)
{
  Element *elem;
  real x, y, w, h;
  real r, r1;  
  Point ch, cb, p1, p2;

  assert(actor != NULL);
  assert(renderer != NULL);

  elem = &actor->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;
  
  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, ACTOR_LINEWIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);

  r = ACTOR_HEAD;
  r1 = 2*r;
  ch.x = x + w*0.5;
  ch.y = y + r + ACTOR_MARGIN_Y;
  cb.x = ch.x;
  cb.y = ch.y + r1 + r;
  
  /* head */
  renderer->ops->fill_ellipse(renderer, 
			     &ch,
			     r, r,
			     &color_white);
  renderer->ops->draw_ellipse(renderer, 
			     &ch,
			     r, r,
			     &color_black);  
  
  /* Arms */
  p1.x = ch.x - r1;
  p2.x = ch.x + r1;
  p1.y = p2.y = ch.y + r;
  renderer->ops->draw_line(renderer, 
			   &p1, &p2,
			   &color_black);

  p1.x = ch.x;
  p1.y = ch.y + r*0.5;
  /* body & legs  */
  renderer->ops->draw_line(renderer, 
			   &p1, &cb,
			   &color_black);

  p2.x = ch.x - r1;
  p2.y = y + ACTOR_BODY;
  renderer->ops->draw_line(renderer, 
			   &cb, &p2,
			   &color_black);
  
  p2.x =  ch.x + r1;
  renderer->ops->draw_line(renderer, 
			   &cb, &p2,
			   &color_black);
  
  text_draw(actor->text, renderer);
}

static void
actor_update_data(Actor *actor)
{
  Element *elem = &actor->element;
  Object *obj = (Object *) actor;
  Rectangle text_box;
  Point p;
  
  elem->width = ACTOR_WIDTH + ACTOR_MARGIN_X;
  elem->height = ACTOR_HEIGHT;

  /* Update connections: */
  actor->connections[0].pos.x = elem->corner.x;
  actor->connections[0].pos.y = elem->corner.y;
  actor->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  actor->connections[1].pos.y = elem->corner.y;
  actor->connections[2].pos.x = elem->corner.x + elem->width;
  actor->connections[2].pos.y = elem->corner.y;
  actor->connections[3].pos.x = elem->corner.x;
  actor->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  actor->connections[4].pos.x = elem->corner.x + elem->width;
  actor->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  actor->connections[5].pos.x = elem->corner.x;
  actor->connections[5].pos.y = elem->corner.y + elem->height;
  actor->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  actor->connections[6].pos.y = elem->corner.y + elem->height;
  actor->connections[7].pos.x = elem->corner.x + elem->width;
  actor->connections[7].pos.y = elem->corner.y + elem->height;
  
  element_update_boundingbox(elem);

  p = elem->corner;
  p.x += elem->width/2;
  p.y +=  ACTOR_HEIGHT + actor->text->height;
  text_set_position(actor->text, &p);

  /* Add bounding box for text: */
  text_calc_boundingbox(actor->text, &text_box);
  rectangle_union(&obj->bounding_box, &text_box);

  obj->position = elem->corner;
  obj->position.x += elem->width/2.0;
  obj->position.y += ACTOR_BODY;

  element_update_handles(elem);
}

static Object *
actor_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Actor *actor;
  Element *elem;
  Object *obj;
  Point p;
  Font *font;
  int i;
  
  actor = g_malloc(sizeof(Actor));
  elem = &actor->element;
  obj = (Object *) actor;
  
  obj->type = &actor_type;
  obj->ops = &actor_ops;
  elem->corner = *startpoint;
  elem->width = ACTOR_WIDTH;
  elem->height = ACTOR_HEIGHT;

  font = font_getfont("Helvetica");
  p = *startpoint;
  p.x += ACTOR_MARGIN_X;
  p.y += ACTOR_HEIGHT - font_descent(font, 0.8);
  
  actor->text = new_text("Actor", font, 0.8, &p, &color_black, ALIGN_CENTER);
  
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &actor->connections[i];
    actor->connections[i].object = obj;
    actor->connections[i].connected = NULL;
  }
  actor_update_data(actor);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return (Object *)actor;
}

static void
actor_destroy(Actor *actor)
{
  text_destroy(actor->text);

  element_destroy(&actor->element);
}

static Object *
actor_copy(Actor *actor)
{
  int i;
  Actor *newactor;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &actor->element;
  
  newactor = g_malloc(sizeof(Actor));
  newelem = &newactor->element;
  newobj = (Object *) newactor;

  element_copy(elem, newelem);

  newactor->text = text_copy(actor->text);
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newactor->connections[i];
    newactor->connections[i].object = newobj;
    newactor->connections[i].connected = NULL;
    newactor->connections[i].pos = actor->connections[i].pos;
    newactor->connections[i].last_pos = actor->connections[i].last_pos;
  }

  actor_update_data(newactor);
  
  return (Object *)newactor;
}


static void
actor_save(Actor *actor, ObjectNode obj_node)
{
  element_save(&actor->element, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		actor->text);
}

static Object *
actor_load(ObjectNode obj_node, int version)
{
  Actor *actor;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;
  
  actor = g_malloc(sizeof(Actor));
  elem = &actor->element;
  obj = (Object *) actor;
  
  obj->type = &actor_type;
  obj->ops = &actor_ops;

  element_load(elem, obj_node);
  
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
      actor->text = data_text(attribute_first_data(attr));

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &actor->connections[i];
    actor->connections[i].object = obj;
    actor->connections[i].connected = NULL;
  }
  actor_update_data(actor);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return (Object *)actor;
}

