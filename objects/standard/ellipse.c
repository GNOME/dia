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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "message.h"
#include "properties.h"

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
  gboolean show_background;
  LineStyle line_style;
  real dashlength;
};

static struct _EllipseProperties {
  gboolean show_background;
} default_properties = { TRUE };

static real ellipse_distance_from(Ellipse *ellipse, Point *point);
static void ellipse_select(Ellipse *ellipse, Point *clicked_point,
			   Renderer *interactive_renderer);
static void ellipse_move_handle(Ellipse *ellipse, Handle *handle,
				Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void ellipse_move(Ellipse *ellipse, Point *to);
static void ellipse_draw(Ellipse *ellipse, Renderer *renderer);
static void ellipse_update_data(Ellipse *ellipse);
static Object *ellipse_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void ellipse_destroy(Ellipse *ellipse);
static Object *ellipse_copy(Ellipse *ellipse);

static PropDescription *ellipse_describe_props(Ellipse *ellipse);
static void ellipse_get_props(Ellipse *ellipse, GPtrArray *props);
static void ellipse_set_props(Ellipse *ellipse, GPtrArray *props);

static void ellipse_save(Ellipse *ellipse, ObjectNode obj_node, const char *filename);
static Object *ellipse_load(ObjectNode obj_node, int version, const char *filename);

static ObjectTypeOps ellipse_type_ops =
{
  (CreateFunc) ellipse_create,
  (LoadFunc)   ellipse_load,
  (SaveFunc)   ellipse_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
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
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   ellipse_describe_props,
  (GetPropsFunc)        ellipse_get_props,
  (SetPropsFunc)        ellipse_set_props,
};

static PropDescription ellipse_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
  PROP_STD_LINE_STYLE,
  PROP_DESC_END
};

static PropDescription *
ellipse_describe_props(Ellipse *ellipse)
{
  if (ellipse_props[0].quark == 0)
    prop_desc_list_calculate_quarks(ellipse_props);
  return ellipse_props;
}

static PropOffset ellipse_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "line_width", PROP_TYPE_REAL, offsetof(Ellipse, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Ellipse, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Ellipse, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Ellipse, show_background) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Ellipse, line_style), offsetof(Ellipse, dashlength) },
  { NULL, 0, 0 }
};

static void
ellipse_get_props(Ellipse *ellipse, GPtrArray *props)
{
  object_get_props_from_offsets(&ellipse->element.object, 
                                ellipse_offsets, props);
}

static void
ellipse_set_props(Ellipse *ellipse, GPtrArray *props)
{
  object_set_props_from_offsets(&ellipse->element.object, 
                                ellipse_offsets, props);
  ellipse_update_data(ellipse);
}

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
		    Point *to, HandleMoveReason reason, ModifierKeys modifiers)
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

  if (ellipse->show_background) {
    renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);

    renderer->ops->fill_ellipse(renderer, 
				&center,
				elem->width, elem->height,
				&ellipse->inner_color);
  }

  renderer->ops->set_linewidth(renderer, ellipse->border_width);
  renderer->ops->set_linestyle(renderer, ellipse->line_style);
  renderer->ops->set_dashlength(renderer, ellipse->dashlength);

  renderer->ops->draw_ellipse(renderer, 
			  &center,
			  elem->width, elem->height,
			  &ellipse->border_color);
}

static void
ellipse_update_data(Ellipse *ellipse)
{
  Element *elem = &ellipse->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  Object *obj = &elem->object;
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

  /* Update directions -- if the ellipse is very thin, these may not be good */
  ellipse->connections[0].directions = DIR_NORTH|DIR_WEST;
  ellipse->connections[1].directions = DIR_NORTH;
  ellipse->connections[2].directions = DIR_NORTH|DIR_EAST;
  ellipse->connections[3].directions = DIR_WEST;
  ellipse->connections[4].directions = DIR_EAST;
  ellipse->connections[5].directions = DIR_SOUTH|DIR_WEST;
  ellipse->connections[6].directions = DIR_SOUTH;
  ellipse->connections[7].directions = DIR_SOUTH|DIR_EAST;

  extra->border_trans = ellipse->border_width / 2.0;
  element_update_boundingbox(elem);

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

  ellipse = g_malloc0(sizeof(Ellipse));
  elem = &ellipse->element;
  obj = &elem->object;
  
  obj->type = &ellipse_type;

  obj->ops = &ellipse_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_WIDTH;

  ellipse->border_width =  attributes_get_default_linewidth();
  ellipse->border_color = attributes_get_foreground();
  ellipse->inner_color = attributes_get_background();
  attributes_get_default_line_style(&ellipse->line_style,
				    &ellipse->dashlength);
  ellipse->show_background = default_properties.show_background;

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &ellipse->connections[i];
    ellipse->connections[i].object = obj;
    ellipse->connections[i].connected = NULL;
  }
  ellipse_update_data(ellipse);

  *handle1 = NULL;
  *handle2 = obj->handles[7];
  return &ellipse->element.object;
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
  
  newellipse = g_malloc0(sizeof(Ellipse));
  newelem = &newellipse->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newellipse->border_width = ellipse->border_width;
  newellipse->border_color = ellipse->border_color;
  newellipse->inner_color = ellipse->inner_color;
  newellipse->dashlength = ellipse->dashlength;
  newellipse->show_background = ellipse->show_background;
  newellipse->line_style = ellipse->line_style;

  for (i=0;i<8;i++) {
    newobj->connections[i] = &newellipse->connections[i];
    newellipse->connections[i].object = newobj;
    newellipse->connections[i].connected = NULL;
    newellipse->connections[i].pos = ellipse->connections[i].pos;
    newellipse->connections[i].last_pos = ellipse->connections[i].last_pos;
  }

  return &newellipse->element.object;
}


static void
ellipse_save(Ellipse *ellipse, ObjectNode obj_node, const char *filename)
{
  element_save(&ellipse->element, obj_node);

  if (ellipse->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  ellipse->border_width);
  
  if (!color_equals(&ellipse->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &ellipse->border_color);
  
  if (!color_equals(&ellipse->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &ellipse->inner_color);

  if (!ellipse->show_background)
    data_add_boolean(new_attribute(obj_node, "show_background"),
		     ellipse->show_background);
  
  if (ellipse->line_style != LINESTYLE_SOLID) {
    data_add_enum(new_attribute(obj_node, "line_style"),
		  ellipse->line_style);

    if (ellipse->dashlength != DEFAULT_LINESTYLE_DASHLEN)
	    data_add_real(new_attribute(obj_node, "dashlength"),
			  ellipse->dashlength);
  }
}

static Object *ellipse_load(ObjectNode obj_node, int version, const char *filename)
{
  Ellipse *ellipse;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;

  ellipse = g_malloc0(sizeof(Ellipse));
  elem = &ellipse->element;
  obj = &elem->object;
  
  obj->type = &ellipse_type;
  obj->ops = &ellipse_ops;

  element_load(elem, obj_node);

  ellipse->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    ellipse->border_width =  data_real( attribute_first_data(attr) );

  ellipse->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &ellipse->border_color);
  
  ellipse->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &ellipse->inner_color);
  
  ellipse->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    ellipse->show_background = data_boolean(attribute_first_data(attr));

  ellipse->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    ellipse->line_style =  data_enum( attribute_first_data(attr) );

  ellipse->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
	  ellipse->dashlength = data_real(attribute_first_data(attr));

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &ellipse->connections[i];
    ellipse->connections[i].object = obj;
    ellipse->connections[i].connected = NULL;
  }
  ellipse_update_data(ellipse);

  return &ellipse->element.object;
}
