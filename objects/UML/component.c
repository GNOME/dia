/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <string.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "stereotype.h"

#include "pixmaps/component.xpm"

typedef struct _Component Component;
struct _Component {
  Element element;

  ConnectionPoint connections[8];

  char *stereotype;
  Text *text;

  char *st_stereotype;
  TextAttributes attrs;
};

#define COMPONENT_BORDERWIDTH 0.1
#define COMPONENT_CHEIGHT 0.7
#define COMPONENT_CWIDTH 2.0
#define COMPONENT_MARGIN_X 0.4
#define COMPONENT_MARGIN_Y 0.3

static real component_distance_from(Component *cmp, Point *point);
static void component_select(Component *cmp, Point *clicked_point,
				Renderer *interactive_renderer);
static void component_move_handle(Component *cmp, Handle *handle,
				     Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void component_move(Component *cmp, Point *to);
static void component_draw(Component *cmp, Renderer *renderer);
static Object *component_create(Point *startpoint,
				   void *user_data,
				   Handle **handle1,
				   Handle **handle2);
static void component_destroy(Component *cmp);
static Object *component_load(ObjectNode obj_node, int version,
				 const char *filename);

static PropDescription *component_describe_props(Component *component);
static void component_get_props(Component *component, GPtrArray *props);
static void component_set_props(Component *component, GPtrArray *props);

static void component_update_data(Component *cmp);

static ObjectTypeOps component_type_ops =
{
  (CreateFunc) component_create,
  (LoadFunc)   component_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL, 
  (ApplyDefaultsFunc) NULL
};

ObjectType component_type =
{
  "UML - Component",   /* name */
  0,                      /* version */
  (char **) component_xpm,  /* pixmap */
  
  &component_type_ops       /* ops */
};

static ObjectOps component_ops = {
  (DestroyFunc)         component_destroy,
  (DrawFunc)            component_draw,
  (DistanceFunc)        component_distance_from,
  (SelectFunc)          component_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            component_move,
  (MoveHandleFunc)      component_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   component_describe_props,
  (GetPropsFunc)        component_get_props,
  (SetPropsFunc)        component_set_props
};

static PropDescription component_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
  N_("Stereotype"), NULL, NULL },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL },   
  PROP_DESC_END
};

static PropDescription *
component_describe_props(Component *component)
{
  return component_props;
}

static PropOffset component_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  {"stereotype", PROP_TYPE_STRING, offsetof(Component , stereotype) },
  {"text",PROP_TYPE_TEXT,offsetof(Component,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(Component,attrs.font)},
  {"text_height",PROP_TYPE_REAL,offsetof(Component,attrs.height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Component,attrs.color)},  
  { NULL, 0, 0 },
};


static void
component_get_props(Component * component, GPtrArray *props)
{
  text_get_attributes(component->text,&component->attrs);
  object_get_props_from_offsets(&component->element.object,
                                component_offsets,props);
}

static void
component_set_props(Component *component, GPtrArray *props)
{
  object_set_props_from_offsets(&component->element.object, 
                                component_offsets, props);
  apply_textattr_properties(props,component->text,"text",&component->attrs);
  g_free(component->st_stereotype);
  component->st_stereotype = NULL;
  component_update_data(component);
}

static real
component_distance_from(Component *cmp, Point *point)
{
  Object *obj = &cmp->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
component_select(Component *cmp, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  text_set_cursor(cmp->text, clicked_point, interactive_renderer);
  text_grab_focus(cmp->text, &cmp->element.object);
  element_update_handles(&cmp->element);
}

static void
component_move_handle(Component *cmp, Handle *handle,
			 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(cmp!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
}

static void
component_move(Component *cmp, Point *to)
{
  cmp->element.corner = *to;

  component_update_data(cmp);
}

static void
component_draw(Component *cmp, Renderer *renderer)
{
  Element *elem;
  real x, y, w, h;
  Point p1, p2;
  
  assert(cmp != NULL);
  assert(renderer != NULL);

  elem = &cmp->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;
  
  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, COMPONENT_BORDERWIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);

  p1.x = x + COMPONENT_CWIDTH/2; p1.y = y;
  p2.x = x+w; p2.y = y+h;

  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);

  p1.x= x; p1.y = y +(h - 3*COMPONENT_CHEIGHT)/2.0;
  p2.x = x+COMPONENT_CWIDTH; p2.y = p1.y + COMPONENT_CHEIGHT;

  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);
  
  p1.y = p2.y + COMPONENT_CHEIGHT;
  p2.y = p1.y + COMPONENT_CHEIGHT;

  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);

  if (cmp->st_stereotype != NULL &&
      cmp->st_stereotype[0] != '\0') {
    p1 = cmp->text->position;
    p1.y -= cmp->text->height;
    renderer->ops->draw_string(renderer, cmp->st_stereotype, &p1, 
			       ALIGN_LEFT, &color_black);
  }

  text_draw(cmp->text, renderer);
}

static void
component_update_data(Component *cmp)
{
  Element *elem = &cmp->element;
  Object *obj = &elem->object;
  Point p;

  cmp->stereotype = remove_stereotype_from_string(cmp->stereotype);
  if (!cmp->st_stereotype) {
    cmp->st_stereotype =  string_to_stereotype(cmp->stereotype);
  }

  elem->width = cmp->text->max_width + 2*COMPONENT_MARGIN_X + COMPONENT_CWIDTH;
  elem->width = MAX(elem->width, 2*COMPONENT_CWIDTH);
  elem->height =  cmp->text->height*cmp->text->numlines +
    cmp->text->descent + 0.1 + 2*COMPONENT_MARGIN_Y ;
  elem->height = MAX(elem->height, 5*COMPONENT_CHEIGHT);

  p = elem->corner;
  p.x += COMPONENT_CWIDTH + COMPONENT_MARGIN_X;
  p.y += COMPONENT_CHEIGHT;
  p.y += cmp->text->ascent;
  if (cmp->stereotype &&
      cmp->stereotype[0] != '\0') {
    p.y += cmp->text->height;
  }
  text_set_position(cmp->text, &p);

  if (cmp->st_stereotype &&
      cmp->st_stereotype[0] != '\0') {
    DiaFont *font;
    font = cmp->text->font;
    elem->height += cmp->text->height;
    elem->width = MAX(elem->width, font_string_width(cmp->st_stereotype,
						     font, cmp->text->height) +
		      2*COMPONENT_MARGIN_X + COMPONENT_CWIDTH);
  }

  /* Update connections: */
  cmp->connections[0].pos = elem->corner;
  cmp->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  cmp->connections[1].pos.y = elem->corner.y;
  cmp->connections[2].pos.x = elem->corner.x + elem->width;
  cmp->connections[2].pos.y = elem->corner.y;
  cmp->connections[3].pos.x = elem->corner.x;
  cmp->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  cmp->connections[4].pos.x = elem->corner.x + elem->width;
  cmp->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  cmp->connections[5].pos.x = elem->corner.x;
  cmp->connections[5].pos.y = elem->corner.y + elem->height;
  cmp->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  cmp->connections[6].pos.y = elem->corner.y + elem->height;
  cmp->connections[7].pos.x = elem->corner.x + elem->width;
  cmp->connections[7].pos.y = elem->corner.y + elem->height;
  
  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);
}

static Object *
component_create(Point *startpoint,
		    void *user_data,
		    Handle **handle1,
		    Handle **handle2)
{
  Component *cmp;
  Element *elem;
  Object *obj;
  Point p;
  DiaFont *font;
  int i;
  
  cmp = g_malloc0(sizeof(Component));
  elem = &cmp->element;
  obj = &elem->object;
  
  obj->type = &component_type;

  obj->ops = &component_ops;

  elem->corner = *startpoint;

  font = font_getfont("Helvetica");
  p = *startpoint;
  p.x += COMPONENT_CWIDTH + COMPONENT_MARGIN_X;
  p.y += 2*COMPONENT_CHEIGHT;
  
  cmp->text = new_text("", font, 0.8, &p, &color_black, ALIGN_LEFT);
  text_get_attributes(cmp->text,&cmp->attrs);

  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &cmp->connections[i];
    cmp->connections[i].object = obj;
    cmp->connections[i].connected = NULL;
  }
  elem->extra_spacing.border_trans = COMPONENT_BORDERWIDTH/2.0;
  cmp->stereotype = NULL;
  cmp->st_stereotype = NULL;
  component_update_data(cmp);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return &cmp->element.object;
}

static void
component_destroy(Component *cmp)
{
  text_destroy(cmp->text);
  g_free(cmp->stereotype);
  g_free(cmp->st_stereotype);
  element_destroy(&cmp->element);
}

static Object *
component_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&component_type,
                                      obj_node,version,filename);
}




