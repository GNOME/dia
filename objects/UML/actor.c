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

#include "config.h"
#include "intl.h"
#include "object.h"
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

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
			     Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void actor_move(Actor *actor, Point *to);
static void actor_draw(Actor *actor, Renderer *renderer);
static Object *actor_create(Point *startpoint,
			   void *user_data,
			   Handle **handle1,
			   Handle **handle2);
static void actor_destroy(Actor *actor);
static Object *actor_copy(Actor *actor);

static void actor_save(Actor *actor, ObjectNode obj_node,
		       const char *filename);
static Object *actor_load(ObjectNode obj_node, int version,
			  const char *filename);

static PropDescription *actor_describe_props(Actor *actor);
static void actor_get_props(Actor *actor, Property *props, guint nprops);
static void actor_set_props(Actor *actor, Property *props, guint nprops);

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
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   actor_describe_props,
  (GetPropsFunc)        actor_get_props,
  (SetPropsFunc)        actor_set_props,
};

static PropDescription actor_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  PROP_STD_TEXT,
  
  PROP_DESC_END
};

static PropDescription *
actor_describe_props(Actor *actor)
{
  if (actor_props[0].quark == 0)
    prop_desc_list_calculate_quarks(actor_props);
  return actor_props;
}

static PropOffset actor_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,

  { NULL, 0, 0 },
};

static struct { const gchar *name; GQuark q; } quarks[] = {
  { "text_font" },
  { "text_height" },
  { "text_colour" },
  { "text" }
};

static void
actor_get_props(Actor * actor, Property *props, guint nprops)
{
  guint i;

  if (object_get_props_from_offsets(&actor->element.object, 
                                    actor_offsets, props, nprops))
    return;
  /* these props can't be handled as easily */
  if (quarks[0].q == 0)
    for (i = 0; i < 4; i++)
      quarks[i].q = g_quark_from_static_string(quarks[i].name);
  for (i = 0; i < nprops; i++) {
    GQuark pquark = g_quark_from_string(props[i].name);

    if (pquark == quarks[0].q) {
      props[i].type = PROP_TYPE_FONT;
      PROP_VALUE_FONT(props[i]) = actor->text->font;
    } else if (pquark == quarks[1].q) {
      props[i].type = PROP_TYPE_REAL;
      PROP_VALUE_REAL(props[i]) = actor->text->height;
    } else if (pquark == quarks[2].q) {
      props[i].type = PROP_TYPE_COLOUR;
      PROP_VALUE_COLOUR(props[i]) = actor->text->color;
    } else if (pquark == quarks[3].q) {
      props[i].type = PROP_TYPE_STRING;
      g_free(PROP_VALUE_STRING(props[i]));
      PROP_VALUE_STRING(props[i]) = text_get_string_copy(actor->text);
    }
  }
}

static void
actor_set_props(Actor *actor, Property *props, guint nprops)
{
  if (!object_set_props_from_offsets(&actor->element.object, 
                                     actor_offsets, props, nprops)) {
    guint i;

    if (quarks[0].q == 0)
      for (i = 0; i < 4; i++)
	quarks[i].q = g_quark_from_static_string(quarks[i].name);

    for (i = 0; i < nprops; i++) {
      GQuark pquark = g_quark_from_string(props[i].name);

      if (pquark == quarks[0].q && props[i].type == PROP_TYPE_FONT) {
	text_set_font(actor->text, PROP_VALUE_FONT(props[i]));
      } else if (pquark == quarks[1].q && props[i].type == PROP_TYPE_REAL) {
	text_set_height(actor->text, PROP_VALUE_REAL(props[i]));
      } else if (pquark == quarks[2].q && props[i].type == PROP_TYPE_COLOUR) {
	text_set_color(actor->text, &PROP_VALUE_COLOUR(props[i]));
      } else if (pquark == quarks[3].q && props[i].type == PROP_TYPE_STRING) {
	text_set_string(actor->text, PROP_VALUE_STRING(props[i]));
      }
    }
  }
  actor_update_data(actor);
}

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
  text_grab_focus(actor->text, &actor->element.object);
  element_update_handles(&actor->element);
}

static void
actor_move_handle(Actor *actor, Handle *handle,
		 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
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
  elem->corner.y -= elem->height / 2.0;

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
  Object *obj = &elem->object;
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
  obj->position.y += elem->height / 2.0;

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
  obj = &elem->object;
  
  obj->type = &actor_type;
  obj->ops = &actor_ops;
  elem->corner = *startpoint;
  elem->width = ACTOR_WIDTH;
  elem->height = ACTOR_HEIGHT;

  font = font_getfont("Helvetica");
  p = *startpoint;
  p.x += ACTOR_MARGIN_X;
  p.y += ACTOR_HEIGHT - font_descent(font, 0.8);
  
  actor->text = new_text(_("Actor"), font, 0.8, &p, &color_black, ALIGN_CENTER);
  
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &actor->connections[i];
    actor->connections[i].object = obj;
    actor->connections[i].connected = NULL;
  }
  elem->extra_spacing.border_trans = ACTOR_LINEWIDTH/2.0;
  actor_update_data(actor);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return &actor->element.object;
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
  newobj = &newelem->object;

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
  
  return &newactor->element.object;
}


static void
actor_save(Actor *actor, ObjectNode obj_node, const char *filename)
{
  element_save(&actor->element, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		actor->text);
}

static Object *
actor_load(ObjectNode obj_node, int version, const char *filename)
{
  Actor *actor;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;
  
  actor = g_malloc(sizeof(Actor));
  elem = &actor->element;
  obj = &elem->object;
  
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
  elem->extra_spacing.border_trans = ACTOR_LINEWIDTH/2.0;
  actor_update_data(actor);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return &actor->element.object;
}

