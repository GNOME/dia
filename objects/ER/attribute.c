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

/* DO NOT USE THIS OBJECT AS A BASIS FOR A NEW OBJECT. */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"
#include "dia-state-object-change.h"

#include "pixmaps/attribute.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define FONT_HEIGHT 0.8
#define MULTIVALUE_BORDER_WIDTH_X 0.4
#define MULTIVALUE_BORDER_WIDTH_Y 0.2
#define TEXT_BORDER_WIDTH_X 1.0
#define TEXT_BORDER_WIDTH_Y 0.5

#define NUM_CONNECTIONS 9

typedef struct _Attribute Attribute;

struct _AttributeState {
  ObjectState obj_state;

  gchar *name;
  real name_width;

  gboolean key;
  gboolean weakkey;
  gboolean derived;
  gboolean multivalue;

  real border_width;
  Color border_color;
  Color inner_color;
};

struct _Attribute {
  Element element;

  DiaFont *font;
  real font_height;
  gchar *name;
  real name_width;

  ConnectionPoint connections[NUM_CONNECTIONS];

  gboolean key;
  gboolean weakkey;
  gboolean derived;
  gboolean multivalue;

  real border_width;
  Color border_color;
  Color inner_color;
};

static real attribute_distance_from(Attribute *attribute, Point *point);
static void attribute_select(Attribute *attribute, Point *clicked_point,
			   DiaRenderer *interactive_renderer);
static DiaObjectChange* attribute_move_handle(Attribute *attribute, Handle *handle,
					   Point *to, ConnectionPoint *cp,
					   HandleMoveReason reason,
					   ModifierKeys modifiers);
static DiaObjectChange* attribute_move(Attribute *attribute, Point *to);
static void attribute_draw(Attribute *attribute, DiaRenderer *renderer);
static void attribute_update_data(Attribute *attribute);
static DiaObject *attribute_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void attribute_destroy(Attribute *attribute);
static DiaObject *attribute_copy(Attribute *attribute);
static PropDescription *
attribute_describe_props(Attribute *attribute);
static void
attribute_get_props(Attribute *attribute, GPtrArray *props);
static void
attribute_set_props(Attribute *attribute, GPtrArray *props);

static void attribute_save(Attribute *attribute, ObjectNode obj_node,
			   DiaContext *ctx);
static DiaObject *attribute_load(ObjectNode obj_node, int version,DiaContext *ctx);

static ObjectTypeOps attribute_type_ops =
{
  (CreateFunc) attribute_create,
  (LoadFunc)   attribute_load,
  (SaveFunc)   attribute_save
};

DiaObjectType attribute_type =
{
  "ER - Attribute",  /* name */
  0,              /* version */
  attribute_xpm,   /* pixmap */
  &attribute_type_ops /* ops */
};

DiaObjectType *_attribute_type = (DiaObjectType *) &attribute_type;

static ObjectOps attribute_ops = {
  (DestroyFunc)         attribute_destroy,
  (DrawFunc)            attribute_draw,
  (DistanceFunc)        attribute_distance_from,
  (SelectFunc)          attribute_select,
  (CopyFunc)            attribute_copy,
  (MoveFunc)            attribute_move,
  (MoveHandleFunc)      attribute_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   attribute_describe_props,
  (GetPropsFunc)        attribute_get_props,
  (SetPropsFunc)        attribute_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription attribute_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Name:"), NULL, NULL },
  { "key", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Key:"), NULL, NULL },
  { "weakkey", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Weak key:"), NULL, NULL },
  { "derived", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Derived:"), NULL, NULL },
  { "multivalue", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Multivalue:"), NULL, NULL },
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_DESC_END
};

static PropDescription *
attribute_describe_props(Attribute *attribute)
{
  if (attribute_props[0].quark == 0)
    prop_desc_list_calculate_quarks(attribute_props);
  return attribute_props;
}

static PropOffset attribute_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "name", PROP_TYPE_STRING, offsetof(Attribute, name) },
  { "key", PROP_TYPE_BOOL, offsetof(Attribute, key) },
  { "weakkey", PROP_TYPE_BOOL, offsetof(Attribute, weakkey) },
  { "derived", PROP_TYPE_BOOL, offsetof(Attribute, derived) },
  { "multivalue", PROP_TYPE_BOOL, offsetof(Attribute, multivalue) },
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Attribute, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Attribute, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Attribute, inner_color) },
  { "text_font", PROP_TYPE_FONT, offsetof(Attribute, font) },
  { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof(Attribute, font_height) },
  { NULL, 0, 0}
};


static void
attribute_get_props(Attribute *attribute, GPtrArray *props)
{
  object_get_props_from_offsets(&attribute->element.object,
                                attribute_offsets, props);
}

static void
attribute_set_props(Attribute *attribute, GPtrArray *props)
{
  object_set_props_from_offsets(&attribute->element.object,
                                attribute_offsets, props);
  attribute_update_data(attribute);
}

static real
attribute_distance_from(Attribute *attribute, Point *point)
{
  Element *elem = &attribute->element;
  Point center;

  center.x = elem->corner.x+elem->width/2;
  center.y = elem->corner.y+elem->height/2;

  return distance_ellipse_point(&center, elem->width, elem->height,
				attribute->border_width, point);
}

static void
attribute_select(Attribute *attribute, Point *clicked_point,
	       DiaRenderer *interactive_renderer)
{
  element_update_handles(&attribute->element);
}


static DiaObjectChange *
attribute_move_handle (Attribute        *attribute,
                       Handle           *handle,
                       Point            *to,
                       ConnectionPoint  *cp,
                       HandleMoveReason  reason,
                       ModifierKeys      modifiers)
{
  g_return_val_if_fail (attribute != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  g_return_val_if_fail (handle->id < 8, NULL);

  element_move_handle (&attribute->element,
                       handle->id,
                       to,
                       cp,
                       reason,
                       modifiers);
  attribute_update_data (attribute);

  return NULL;
}


static DiaObjectChange*
attribute_move(Attribute *attribute, Point *to)
{
  attribute->element.corner = *to;
  attribute_update_data(attribute);

  return NULL;
}


static void
attribute_draw (Attribute *attribute, DiaRenderer *renderer)
{
  Point center;
  Point start, end;
  Point p;
  Element *elem;
  real width;

  g_return_if_fail (attribute != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &attribute->element;

  center.x = elem->corner.x + elem->width/2;
  center.y = elem->corner.y + elem->height/2;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_draw_ellipse (renderer,
                             &center,
                             elem->width,
                             elem->height,
                             &attribute->inner_color,
                             NULL);

  dia_renderer_set_linewidth (renderer, attribute->border_width);
  if (attribute->derived) {
    dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_DASHED, 0.3);
  } else {
    dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  }

  dia_renderer_draw_ellipse (renderer,
                             &center,
                             elem->width,
                             elem->height,
                             NULL,
                             &attribute->border_color);

  if(attribute->multivalue) {
    dia_renderer_draw_ellipse (renderer,
                               &center,
                               elem->width - 2*MULTIVALUE_BORDER_WIDTH_X,
                               elem->height - 2*MULTIVALUE_BORDER_WIDTH_Y,
                               NULL,
                               &attribute->border_color);
  }

  p.x = elem->corner.x + elem->width / 2.0;
  p.y = elem->corner.y + (elem->height - attribute->font_height)/2.0 +
         dia_font_ascent (attribute->name,
                          attribute->font,
                          attribute->font_height);

  dia_renderer_set_font (renderer, attribute->font, attribute->font_height);
  dia_renderer_draw_string (renderer,
                            attribute->name,
                            &p,
                            DIA_ALIGN_CENTRE,
                            &color_black);

  if (attribute->key || attribute->weakkey) {
    if (attribute->weakkey) {
      dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_DASHED, 0.3);
    } else {
      dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
    }
    width = dia_font_string_width (attribute->name,
                                   attribute->font,
                                   attribute->font_height);
    start.x = center.x - width / 2;
    start.y = center.y + 0.4;
    end.x = center.x + width / 2;
    end.y = center.y + 0.4;
    dia_renderer_draw_line (renderer, &start, &end, &color_black);
  }
}

static void
attribute_update_data(Attribute *attribute)
{
  Element *elem = &attribute->element;
  DiaObject *obj = &elem->object;
  Point center;
  ElementBBExtras *extra = &elem->extra_spacing;
  real half_x, half_y;

  attribute->name_width =
    dia_font_string_width(attribute->name,
                          attribute->font, attribute->font_height);

  elem->width = attribute->name_width + 2*TEXT_BORDER_WIDTH_X;
  elem->height = attribute->font_height + 2*TEXT_BORDER_WIDTH_Y;

  center.x = elem->corner.x + elem->width / 2.0;
  center.y = elem->corner.y + elem->height / 2.0;

  half_x = elem->width * M_SQRT1_2 / 2.0;
  half_y = elem->height * M_SQRT1_2 / 2.0;

  /* Update connections: */
  connpoint_update(&attribute->connections[0],
		    center.x - half_x,
		    center.y - half_y,
		    DIR_NORTHWEST);
  connpoint_update(&attribute->connections[1],
		    center.x,
		    elem->corner.y,
		    DIR_NORTH);
  connpoint_update(&attribute->connections[2],
		    center.x + half_x,
		    center.y - half_y,
		    DIR_NORTHEAST);
  connpoint_update(&attribute->connections[3],
		    elem->corner.x,
		    center.y,
		    DIR_WEST);
  connpoint_update(&attribute->connections[4],
		    elem->corner.x + elem->width,
		    elem->corner.y + elem->height / 2.0,
		    DIR_EAST);
  connpoint_update(&attribute->connections[5],
		    center.x - half_x,
		    center.y + half_y,
		    DIR_SOUTHWEST);
  connpoint_update(&attribute->connections[6],
		    elem->corner.x + elem->width / 2.0,
		    elem->corner.y + elem->height,
		    DIR_SOUTH);
  connpoint_update(&attribute->connections[7],
		    center.x + half_x,
		    center.y + half_y,
		    DIR_SOUTHEAST);
  connpoint_update(&attribute->connections[8],
		    center.x,
		    center.y,
		    DIR_ALL);

  extra->border_trans = attribute->border_width/2.0;
  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);

}

static DiaObject *
attribute_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Attribute *attribute;
  Element *elem;
  DiaObject *obj;
  int i;

  attribute = g_new0 (Attribute, 1);
  elem = &attribute->element;
  obj = &elem->object;

  obj->type = &attribute_type;
  obj->ops = &attribute_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;

  attribute->border_width =  attributes_get_default_linewidth();
  attribute->border_color = attributes_get_foreground();
  attribute->inner_color = attributes_get_background();

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &attribute->connections[i];
    attribute->connections[i].object = obj;
    attribute->connections[i].connected = NULL;
  }
  attribute->connections[8].flags = CP_FLAGS_MAIN;

  attribute->key = FALSE;
  attribute->weakkey = FALSE;
  attribute->derived = FALSE;
  attribute->multivalue = FALSE;
  attribute->font = dia_font_new_from_style(DIA_FONT_MONOSPACE,FONT_HEIGHT);
  attribute->font_height = FONT_HEIGHT;
  attribute->name = g_strdup(_("Attribute"));

  attribute->name_width =
    dia_font_string_width(attribute->name,
                          attribute->font, attribute->font_height);

  attribute_update_data(attribute);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = obj->handles[0];
  return &attribute->element.object;
}

static void
attribute_destroy(Attribute *attribute)
{
  element_destroy(&attribute->element);
  g_clear_pointer (&attribute->name, g_free);
}

static DiaObject *
attribute_copy(Attribute *attribute)
{
  int i;
  Attribute *newattribute;
  Element *elem, *newelem;
  DiaObject *newobj;

  elem = &attribute->element;

  newattribute = g_new0 (Attribute, 1);
  newelem = &newattribute->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newattribute->border_width = attribute->border_width;
  newattribute->border_color = attribute->border_color;
  newattribute->inner_color = attribute->inner_color;

  for (i=0;i<NUM_CONNECTIONS;i++) {
    newobj->connections[i] = &newattribute->connections[i];
    newattribute->connections[i].object = newobj;
    newattribute->connections[i].connected = NULL;
    newattribute->connections[i].pos = attribute->connections[i].pos;
    newattribute->connections[i].flags = attribute->connections[i].flags;
  }

  newattribute->font = g_object_ref (attribute->font);
  newattribute->font_height = attribute->font_height;
  newattribute->name = g_strdup(attribute->name);
  newattribute->name_width = attribute->name_width;

  newattribute->key = attribute->key;
  newattribute->weakkey = attribute->weakkey;
  newattribute->derived = attribute->derived;
  newattribute->multivalue = attribute->multivalue;

  return &newattribute->element.object;
}


static void
attribute_save(Attribute *attribute, ObjectNode obj_node,
	       DiaContext *ctx)
{
  element_save(&attribute->element, obj_node, ctx);

  data_add_real(new_attribute(obj_node, "border_width"),
		attribute->border_width, ctx);
  data_add_color(new_attribute(obj_node, "border_color"),
		 &attribute->border_color, ctx);
  data_add_color(new_attribute(obj_node, "inner_color"),
		 &attribute->inner_color, ctx);
  data_add_string(new_attribute(obj_node, "name"),
		  attribute->name, ctx);
  data_add_boolean(new_attribute(obj_node, "key"),
		   attribute->key, ctx);
  data_add_boolean(new_attribute(obj_node, "weak_key"),
		   attribute->weakkey, ctx);
  data_add_boolean(new_attribute(obj_node, "derived"),
		   attribute->derived, ctx);
  data_add_boolean(new_attribute(obj_node, "multivalued"),
		   attribute->multivalue, ctx);
  data_add_font (new_attribute (obj_node, "font"),
		 attribute->font, ctx);
  data_add_real(new_attribute(obj_node, "font_height"),
		attribute->font_height, ctx);
}

static DiaObject *
attribute_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  Attribute *attribute;
  Element *elem;
  DiaObject *obj;
  int i;
  AttributeNode attr;

  attribute = g_new0 (Attribute, 1);
  elem = &attribute->element;
  obj = &elem->object;

  obj->type = &attribute_type;
  obj->ops = &attribute_ops;

  element_load(elem, obj_node, ctx);

  attribute->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    attribute->border_width =  data_real(attribute_first_data(attr), ctx);

  attribute->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &attribute->border_color, ctx);

  attribute->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &attribute->inner_color, ctx);

  attribute->name = NULL;
  attr = object_find_attribute(obj_node, "name");
  if (attr != NULL)
    attribute->name = data_string(attribute_first_data(attr), ctx);

  attr = object_find_attribute(obj_node, "key");
  if (attr != NULL)
    attribute->key = data_boolean(attribute_first_data(attr), ctx);

  attr = object_find_attribute(obj_node, "weak_key");
  if (attr != NULL)
    attribute->weakkey = data_boolean(attribute_first_data(attr), ctx);

  attr = object_find_attribute(obj_node, "derived");
  if (attr != NULL)
    attribute->derived = data_boolean(attribute_first_data(attr), ctx);

  attr = object_find_attribute(obj_node, "multivalued");
  if (attr != NULL)
    attribute->multivalue = data_boolean(attribute_first_data(attr), ctx);

  if (attribute->font != NULL) {
    /* This shouldn't happen, but doesn't hurt */
    g_clear_object (&attribute->font);
    attribute->font = NULL;
  }
  attr = object_find_attribute (obj_node, "font");
  if (attr != NULL)
    attribute->font = data_font (attribute_first_data (attr), ctx);

  attribute->font_height = FONT_HEIGHT;
  attr = object_find_attribute (obj_node, "font_height");
  if (attr != NULL)
    attribute->font_height = data_real(attribute_first_data(attr), ctx);

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &attribute->connections[i];
    attribute->connections[i].object = obj;
    attribute->connections[i].connected = NULL;
  }
  attribute->connections[8].flags = CP_FLAGS_MAIN;

  if (attribute->font == NULL)
    attribute->font = dia_font_new_from_style(DIA_FONT_MONOSPACE,
                                              attribute->font_height);

  attribute->name_width = dia_font_string_width(attribute->name,
                                                attribute->font,
                                                attribute->font_height);
  attribute_update_data(attribute);

  for (i=0;i<8;i++)
    obj->handles[i]->type = HANDLE_NON_MOVABLE;

  return &attribute->element.object;
}
