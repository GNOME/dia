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

#include "pixmaps/entity.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define TEXT_BORDER_WIDTH_X 0.7
#define TEXT_BORDER_WIDTH_Y 0.5
#define WEAK_BORDER_WIDTH 0.25
#define FONT_HEIGHT 0.8
#define DIAMOND_RATIO 0.6

typedef struct _Entity Entity;

#define NUM_CONNECTIONS 9

struct _Entity {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  real border_width;
  Color border_color;
  Color inner_color;

  gboolean associative;

  DiaFont *font;
  real font_height;
  char *name;
  real name_width;

  int weak;

};

static real entity_distance_from(Entity *entity, Point *point);
static void entity_select(Entity *entity, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static DiaObjectChange* entity_move_handle(Entity *entity, Handle *handle,
					Point *to, ConnectionPoint *cp,
					HandleMoveReason reason,
					ModifierKeys modifiers);
static DiaObjectChange* entity_move(Entity *entity, Point *to);
static void entity_draw(Entity *entity, DiaRenderer *renderer);
static void entity_update_data(Entity *entity);
static DiaObject *entity_create(Point *startpoint,
			     void *user_data,
			     Handle **handle1,
			     Handle **handle2);
static void entity_destroy(Entity *entity);
static DiaObject *entity_copy(Entity *entity);
static PropDescription *
entity_describe_props(Entity *entity);
static void entity_get_props(Entity *entity, GPtrArray *props);
static void entity_set_props(Entity *entity, GPtrArray *props);

static void entity_save(Entity *entity, ObjectNode obj_node,
			DiaContext *ctx);
static DiaObject *entity_load(ObjectNode obj_node, int version,DiaContext *ctx);

static ObjectTypeOps entity_type_ops =
{
  (CreateFunc) entity_create,
  (LoadFunc)   entity_load,
  (SaveFunc)   entity_save
};

DiaObjectType entity_type =
{
  "ER - Entity",  /* name */
  0,           /* version */
  entity_xpm,   /* pixmap */
  &entity_type_ops, /* ops */
  NULL,        /* pixmap_file */
  NULL,  /* default_user_data */
  NULL,     /* prop_descs */
  NULL,   /* prop_offsets */
  DIA_OBJECT_HAS_VARIANTS /* flags */
};

DiaObjectType *_entity_type = (DiaObjectType *) &entity_type;

static ObjectOps entity_ops = {
  (DestroyFunc)         entity_destroy,
  (DrawFunc)            entity_draw,
  (DistanceFunc)        entity_distance_from,
  (SelectFunc)          entity_select,
  (CopyFunc)            entity_copy,
  (MoveFunc)            entity_move,
  (MoveHandleFunc)      entity_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   entity_describe_props,
  (GetPropsFunc)        entity_get_props,
  (SetPropsFunc)        entity_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription entity_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Name:"), NULL, NULL },
  { "weak", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_NO_DEFAULTS,
    N_("Weak:"), NULL, NULL },
  { "associative", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Associative:"), NULL, NULL },
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_DESC_END
};

static PropDescription *
entity_describe_props(Entity *entity)
{
  if (entity_props[0].quark == 0)
    prop_desc_list_calculate_quarks(entity_props);
  return entity_props;
}

static PropOffset entity_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "name", PROP_TYPE_STRING, offsetof(Entity, name) },
  { "weak", PROP_TYPE_BOOL, offsetof(Entity, weak) },
  { "associative", PROP_TYPE_BOOL, offsetof(Entity, associative) },
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Entity, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Entity, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Entity, inner_color) },
  { "text_font", PROP_TYPE_FONT, offsetof (Entity, font) },
  { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof(Entity, font_height) },
  { NULL, 0, 0}
};


static void
entity_get_props(Entity *entity, GPtrArray *props)
{
  object_get_props_from_offsets(&entity->element.object,
                                entity_offsets, props);
}

static void
entity_set_props(Entity *entity, GPtrArray *props)
{
  object_set_props_from_offsets(&entity->element.object,
                                entity_offsets, props);
  entity_update_data(entity);
}

static real
entity_distance_from(Entity *entity, Point *point)
{
  Element *elem = &entity->element;
  DiaRectangle rect;

  rect.left = elem->corner.x - entity->border_width/2;
  rect.right = elem->corner.x + elem->width + entity->border_width/2;
  rect.top = elem->corner.y - entity->border_width/2;
  rect.bottom = elem->corner.y + elem->height + entity->border_width/2;
  return distance_rectangle_point(&rect, point);
}

static void
entity_select(Entity *entity, Point *clicked_point,
	   DiaRenderer *interactive_renderer)
{
  element_update_handles(&entity->element);
}


static DiaObjectChange *
entity_move_handle (Entity           *entity,
                    Handle           *handle,
                    Point            *to,
                    ConnectionPoint  *cp,
                    HandleMoveReason  reason,
                    ModifierKeys      modifiers)
{
  g_return_val_if_fail (entity != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  element_move_handle (&entity->element,
                       handle->id,
                       to,
                       cp,
                       reason,
                       modifiers);

  entity_update_data (entity);

  return NULL;
}


static DiaObjectChange*
entity_move(Entity *entity, Point *to)
{
  entity->element.corner = *to;

  entity_update_data(entity);

  return NULL;
}

static void
entity_draw (Entity *entity, DiaRenderer *renderer)
{
  Point ul_corner, lr_corner;
  Point p;
  Element *elem;
  double diff;

  g_return_if_fail (entity != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &entity->element;

  ul_corner.x = elem->corner.x;
  ul_corner.y = elem->corner.y;
  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);

  dia_renderer_set_linewidth (renderer, entity->border_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);

  dia_renderer_draw_rect (renderer,
                          &ul_corner,
                          &lr_corner,
                          &entity->inner_color,
                          &entity->border_color);

  if (entity->weak) {
    diff = WEAK_BORDER_WIDTH/*MIN(elem->width / 2.0 * 0.20, elem->height / 2.0 * 0.20)*/;
    ul_corner.x += diff;
    ul_corner.y += diff;
    lr_corner.x -= diff;
    lr_corner.y -= diff;
    dia_renderer_draw_rect (renderer,
                            &ul_corner,
                            &lr_corner,
                            NULL,
                            &entity->border_color);
  }
  if(entity->associative){
    Point corners[4];
    corners[0].x = elem->corner.x;
    corners[0].y = elem->corner.y + elem->height / 2;
    corners[1].x = elem->corner.x + elem->width / 2;
    corners[1].y = elem->corner.y;
    corners[2].x = elem->corner.x + elem->width;
    corners[2].y = elem->corner.y + elem->height / 2;
    corners[3].x = elem->corner.x + elem->width / 2;
    corners[3].y = elem->corner.y + elem->height;
    dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);

    dia_renderer_set_linewidth (renderer, entity->border_width);
    dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
    dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);

    dia_renderer_draw_polygon (renderer, corners, 4,
                               &entity->inner_color,
                               &entity->border_color);
  }

  p.x = elem->corner.x + elem->width / 2.0;
  p.y = elem->corner.y + (elem->height - entity->font_height)/2.0 +
      dia_font_ascent (entity->name,entity->font, entity->font_height);
  dia_renderer_set_font (renderer, entity->font, entity->font_height);
  dia_renderer_draw_string (renderer,
                            entity->name,
                            &p,
                            DIA_ALIGN_CENTRE,
                            &color_black);
}

static void
entity_update_data(Entity *entity)
{
  Element *elem = &entity->element;
  DiaObject *obj = &elem->object;
  ElementBBExtras *extra = &elem->extra_spacing;

  entity->name_width =
    dia_font_string_width(entity->name, entity->font, entity->font_height);

  if(entity->associative){
    elem->width = entity->name_width + 2*TEXT_BORDER_WIDTH_X;
    elem->height = elem->width * DIAMOND_RATIO;
  }
  else {
  elem->width = entity->name_width + 2*TEXT_BORDER_WIDTH_X;
  elem->height = entity->font_height + 2*TEXT_BORDER_WIDTH_Y;
  }

  /* Update connections: */
  connpoint_update(&entity->connections[0],
		    elem->corner.x,
		    elem->corner.y,
		    DIR_NORTHWEST);
  connpoint_update(&entity->connections[1],
		   elem->corner.x + elem->width / 2.0,
		   elem->corner.y,
		   DIR_NORTH);
  connpoint_update(&entity->connections[2],
		   elem->corner.x + elem->width,
		   elem->corner.y,
		   DIR_NORTHEAST);
  connpoint_update(&entity->connections[3],
		   elem->corner.x,
		   elem->corner.y + elem->height / 2.0,
		   DIR_WEST);
  connpoint_update(&entity->connections[4],
		   elem->corner.x + elem->width,
		   elem->corner.y + elem->height / 2.0,
		   DIR_EAST);
  connpoint_update(&entity->connections[5],
		   elem->corner.x,
		   elem->corner.y + elem->height,
		   DIR_SOUTHWEST);
  connpoint_update(&entity->connections[6],
		   elem->corner.x + elem->width / 2.0,
		   elem->corner.y + elem->height,
		   DIR_SOUTH);
  connpoint_update(&entity->connections[7],
		   elem->corner.x + elem->width,
		   elem->corner.y + elem->height,
		   DIR_SOUTHEAST);
  connpoint_update(&entity->connections[8],
		   elem->corner.x + elem->width / 2.0,
		   elem->corner.y + elem->height / 2.0,
		   DIR_ALL);

  extra->border_trans = entity->border_width/2.0;
  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);
}

static DiaObject *
entity_create(Point *startpoint,
	      void *user_data,
	      Handle **handle1,
	      Handle **handle2)
{
  Entity *entity;
  Element *elem;
  DiaObject *obj;
  int i;

  entity = g_new0 (Entity, 1);
  elem = &entity->element;
  obj = &elem->object;

  obj->type = &entity_type;

  obj->ops = &entity_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;

  entity->border_width =  attributes_get_default_linewidth();
  entity->border_color = attributes_get_foreground();
  entity->inner_color = attributes_get_background();

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &entity->connections[i];
    entity->connections[i].object = obj;
    entity->connections[i].connected = NULL;
  }
  entity->connections[8].flags = CP_FLAGS_MAIN;

  entity->weak = GPOINTER_TO_INT(user_data);
  entity->font = dia_font_new_from_style(DIA_FONT_MONOSPACE,FONT_HEIGHT);
  entity->font_height = FONT_HEIGHT;
  entity->name = g_strdup(_("Entity"));

  entity->name_width =
    dia_font_string_width(entity->name, entity->font, entity->font_height);

  entity_update_data(entity);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = obj->handles[0];
  return &entity->element.object;
}


static void
entity_destroy (Entity *entity)
{
  g_clear_object (&entity->font);
  element_destroy (&entity->element);
  g_clear_pointer (&entity->name, g_free);
}


static DiaObject *
entity_copy(Entity *entity)
{
  int i;
  Entity *newentity;
  Element *elem, *newelem;
  DiaObject *newobj;

  elem = &entity->element;

  newentity = g_new0 (Entity, 1);
  newelem = &newentity->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newentity->border_width = entity->border_width;
  newentity->border_color = entity->border_color;
  newentity->inner_color = entity->inner_color;

  for (i=0;i<NUM_CONNECTIONS;i++) {
    newobj->connections[i] = &newentity->connections[i];
    newentity->connections[i].object = newobj;
    newentity->connections[i].connected = NULL;
    newentity->connections[i].pos = entity->connections[i].pos;
    newentity->connections[i].flags = entity->connections[i].flags;
  }

  newentity->font = g_object_ref (entity->font);
  newentity->font_height = entity->font_height;
  newentity->name = g_strdup(entity->name);
  newentity->name_width = entity->name_width;

  newentity->weak = entity->weak;

  return &newentity->element.object;
}

static void
entity_save(Entity *entity, ObjectNode obj_node, DiaContext *ctx)
{
  element_save(&entity->element, obj_node, ctx);

  data_add_real(new_attribute(obj_node, "border_width"),
		entity->border_width, ctx);
  data_add_color(new_attribute(obj_node, "border_color"),
		 &entity->border_color, ctx);
  data_add_color(new_attribute(obj_node, "inner_color"),
		 &entity->inner_color, ctx);
  data_add_string(new_attribute(obj_node, "name"),
		  entity->name, ctx);
  data_add_boolean(new_attribute(obj_node, "weak"),
		   entity->weak, ctx);
  data_add_boolean(new_attribute(obj_node, "associative"),
		   entity->associative, ctx);
  data_add_font (new_attribute (obj_node, "font"),
		 entity->font, ctx);
  data_add_real(new_attribute(obj_node, "font_height"),
  		entity->font_height, ctx);
}

static DiaObject *
entity_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  Entity *entity;
  Element *elem;
  DiaObject *obj;
  int i;
  AttributeNode attr;

  entity = g_new0 (Entity, 1);
  elem = &entity->element;
  obj = &elem->object;

  obj->type = &entity_type;
  obj->ops = &entity_ops;

  element_load(elem, obj_node, ctx);

  entity->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    entity->border_width =  data_real(attribute_first_data(attr), ctx);

  entity->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &entity->border_color, ctx);

  entity->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &entity->inner_color, ctx);

  entity->name = NULL;
  attr = object_find_attribute(obj_node, "name");
  if (attr != NULL)
    entity->name = data_string(attribute_first_data(attr), ctx);

  attr = object_find_attribute(obj_node, "weak");
  if (attr != NULL)
    entity->weak = data_boolean(attribute_first_data(attr), ctx);

  attr = object_find_attribute(obj_node, "associative");
  if (attr != NULL)
    entity->associative = data_boolean(attribute_first_data(attr), ctx);

  if (entity->font != NULL) {
    /* This shouldn't happen, but doesn't hurt */
    g_clear_object (&entity->font);
    entity->font = NULL;
  }
  attr = object_find_attribute (obj_node, "font");
  if (attr != NULL)
    entity->font = data_font (attribute_first_data (attr), ctx);

  entity->font_height = FONT_HEIGHT;
  attr = object_find_attribute(obj_node, "font_height");
  if (attr != NULL)
    entity->font_height = data_real(attribute_first_data(attr), ctx);

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &entity->connections[i];
    entity->connections[i].object = obj;
    entity->connections[i].connected = NULL;
  }
  entity->connections[8].flags = CP_FLAGS_MAIN;

  if (entity->font == NULL)
    entity->font = dia_font_new_from_style(DIA_FONT_MONOSPACE,1.0);

  entity->name_width =
    dia_font_string_width(entity->name, entity->font, entity->font_height);

  entity_update_data(entity);

  for (i=0;i<8;i++)
    obj->handles[i]->type = HANDLE_NON_MOVABLE;

  return &entity->element.object;
}
