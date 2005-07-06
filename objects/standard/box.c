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
#include "diarenderer.h"
#include "attributes.h"
#include "widgets.h"
#include "message.h"
#include "properties.h"

#include "pixmaps/box.xpm"
#include "tool-icons.h"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.25

#define NUM_CONNECTIONS 9

typedef enum {
  FREE_ASPECT,
  FIXED_ASPECT,
  SQUARE_ASPECT
} AspectType;

typedef struct _Box Box;

struct _Box {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  LineStyle line_style;
  real dashlength;
  real corner_radius;
  AspectType aspect;
};

static struct _BoxProperties {
  gboolean show_background;
  real corner_radius;
  AspectType aspect;
} default_properties = { TRUE, 0.0 };

static real box_distance_from(Box *box, Point *point);
static void box_select(Box *box, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static ObjectChange* box_move_handle(Box *box, Handle *handle,
			    Point *to, ConnectionPoint *cp,
				     HandleMoveReason reason, 
			    ModifierKeys modifiers);
static ObjectChange* box_move(Box *box, Point *to);
static void box_draw(Box *box, DiaRenderer *renderer);
static void box_update_data(Box *box);
static DiaObject *box_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void box_destroy(Box *box);
static DiaObject *box_copy(Box *box);

static PropDescription *box_describe_props(Box *box);
static void box_get_props(Box *box, GPtrArray *props);
static void box_set_props(Box *box, GPtrArray *props);

static void box_save(Box *box, ObjectNode obj_node, const char *filename);
static DiaObject *box_load(ObjectNode obj_node, int version, const char *filename);
static DiaMenu *box_get_object_menu(Box *box, Point *clickedpoint);

static ObjectTypeOps box_type_ops =
{
  (CreateFunc) box_create,
  (LoadFunc)   box_load,
  (SaveFunc)   box_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType box_type =
{
  "Standard - Box",  /* name */
  0,                 /* version */
  (char **) box_icon, /* pixmap */

  &box_type_ops      /* ops */
};

DiaObjectType *_box_type = (DiaObjectType *) &box_type;

static ObjectOps box_ops = {
  (DestroyFunc)         box_destroy,
  (DrawFunc)            box_draw,
  (DistanceFunc)        box_distance_from,
  (SelectFunc)          box_select,
  (CopyFunc)            box_copy,
  (MoveFunc)            box_move,
  (MoveHandleFunc)      box_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      box_get_object_menu,
  (DescribePropsFunc)   box_describe_props,
  (GetPropsFunc)        box_get_props,
  (SetPropsFunc)        box_set_props,
};

static PropNumData corner_radius_data = { 0.0, 10.0, 0.1 };

static PropEnumData prop_aspect_data[] = {
  { N_("Free"), FREE_ASPECT },
  { N_("Fixed"), FIXED_ASPECT },
  { N_("Square"), SQUARE_ASPECT },
  { NULL, 0 }
};
static PropDescription box_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
  PROP_STD_LINE_STYLE,
  { "corner_radius", PROP_TYPE_LENGTH, PROP_FLAG_VISIBLE,
    N_("Corner radius"), NULL, &corner_radius_data },
  { "aspect", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
    N_("Aspect ratio"), NULL, prop_aspect_data },
  PROP_DESC_END
};

static PropDescription *
box_describe_props(Box *box)
{
  if (box_props[0].quark == 0)
    prop_desc_list_calculate_quarks(box_props);
  return box_props;
}

static PropOffset box_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "line_width", PROP_TYPE_REAL, offsetof(Box, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Box, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Box, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Box, show_background) },
  { "aspect", PROP_TYPE_ENUM, offsetof(Box, aspect) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Box, line_style), offsetof(Box, dashlength) },
  { "corner_radius", PROP_TYPE_LENGTH, offsetof(Box, corner_radius) },
  { NULL, 0, 0 }
};

static void
box_get_props(Box *box, GPtrArray *props)
{
  object_get_props_from_offsets(&box->element.object, 
                                box_offsets, props);
}

static void
box_set_props(Box *box, GPtrArray *props)
{
  object_set_props_from_offsets(&box->element.object, 
                                box_offsets, props);
  box_update_data(box);
}

static real
box_distance_from(Box *box, Point *point)
{
  Element *elem = &box->element;
  Rectangle rect;

  rect.left = elem->corner.x - box->border_width/2;
  rect.right = elem->corner.x + elem->width + box->border_width/2;
  rect.top = elem->corner.y - box->border_width/2;
  rect.bottom = elem->corner.y + elem->height + box->border_width/2;
  return distance_rectangle_point(&rect, point);
}

static void
box_select(Box *box, Point *clicked_point,
	   DiaRenderer *interactive_renderer)
{
  real radius;

  element_update_handles(&box->element);

  if (box->corner_radius > 0) {
    Element *elem = (Element *)box;
    radius = box->corner_radius;
    radius = MIN(radius, elem->width/2);
    radius = MIN(radius, elem->height/2);
    radius *= (1-M_SQRT1_2);

    elem->resize_handles[0].pos.x += radius;
    elem->resize_handles[0].pos.y += radius;
    elem->resize_handles[2].pos.x -= radius;
    elem->resize_handles[2].pos.y += radius;
    elem->resize_handles[5].pos.x += radius;
    elem->resize_handles[5].pos.y -= radius;
    elem->resize_handles[7].pos.x -= radius;
    elem->resize_handles[7].pos.y -= radius;
  }
}

static ObjectChange*
box_move_handle(Box *box, Handle *handle,
		Point *to, ConnectionPoint *cp,
		HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(box!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (box->aspect != FREE_ASPECT){
    double width, height;
    double new_width, new_height;
    double to_width, aspect_width;
    Point corner = box->element.corner;
    Point se_to;

    width = box->element.width;
    height = box->element.height;
    switch (handle->id) {
    case HANDLE_RESIZE_N:
    case HANDLE_RESIZE_S:
      new_height = fabs(to->y - corner.y);
      new_width = new_height / height * width;
      break;
    case HANDLE_RESIZE_W:
    case HANDLE_RESIZE_E:
      new_width = fabs(to->x - corner.x);
      new_height = new_width / width * height;
      break;
    case HANDLE_RESIZE_NW:
    case HANDLE_RESIZE_NE:
    case HANDLE_RESIZE_SW:
    case HANDLE_RESIZE_SE:
      to_width = fabs(to->x - corner.x);
      aspect_width = fabs(to->y - corner.y) / height * width;
      new_width = to_width > aspect_width ? to_width : aspect_width;
      new_height = new_width / width * height;
      break;
    default: 
      new_width = width;
      new_height = height;
      break;
    }
	
    se_to.x = corner.x + new_width;
    se_to.y = corner.y + new_height;
        
    element_move_handle(&box->element, HANDLE_RESIZE_SE, &se_to, cp, reason, modifiers);
  } else {
    element_move_handle(&box->element, handle->id, to, cp, reason, modifiers);
  }

  box_update_data(box);

  return NULL;
}

static ObjectChange*
box_move(Box *box, Point *to)
{
  box->element.corner = *to;
  
  box_update_data(box);

  return NULL;
}

static void
box_draw(Box *box, DiaRenderer *renderer)
{
  Point lr_corner;
  Element *elem;
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);

  
  assert(box != NULL);
  assert(renderer != NULL);

  elem = &box->element;

  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;

  if (box->show_background) {
    renderer_ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  
    /* Problem:  How do we make the fill with rounded corners? */
    if (box->corner_radius > 0) {
      renderer_ops->fill_rounded_rect(renderer,
				       &elem->corner,
				       &lr_corner,
				       &box->inner_color,
				       box->corner_radius);
    } else {
      renderer_ops->fill_rect(renderer, 
			       &elem->corner,
			       &lr_corner, 
			       &box->inner_color);
    }
  }

  renderer_ops->set_linewidth(renderer, box->border_width);
  renderer_ops->set_linestyle(renderer, box->line_style);
  renderer_ops->set_dashlength(renderer, box->dashlength);
  renderer_ops->set_linejoin(renderer, LINEJOIN_MITER);

  if (box->corner_radius > 0) {
    renderer_ops->draw_rounded_rect(renderer, 
			     &elem->corner,
			     &lr_corner, 
			     &box->border_color,
			     box->corner_radius);
  } else {
    renderer_ops->draw_rect(renderer, 
			     &elem->corner,
			     &lr_corner, 
			     &box->border_color);
  }
}


static void
box_update_data(Box *box)
{
  Element *elem = &box->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  DiaObject *obj = &elem->object;
  real radius;
  
  if (box->aspect == SQUARE_ASPECT){
    float size = elem->height < elem->width ? elem->height : elem->width;
    elem->height = elem->width = size;
  }

  radius = box->corner_radius;
  radius = MIN(radius, elem->width/2);
  radius = MIN(radius, elem->height/2);
  radius *= (1-M_SQRT1_2);
  
  /* Update connections: */
  box->connections[0].pos.x = elem->corner.x + radius;
  box->connections[0].pos.y = elem->corner.y + radius;
  box->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  box->connections[1].pos.y = elem->corner.y;
  box->connections[2].pos.x = elem->corner.x + elem->width - radius;
  box->connections[2].pos.y = elem->corner.y + radius;
  box->connections[3].pos.x = elem->corner.x;
  box->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  box->connections[4].pos.x = elem->corner.x + elem->width;
  box->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  box->connections[5].pos.x = elem->corner.x + radius;
  box->connections[5].pos.y = elem->corner.y + elem->height - radius;
  box->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  box->connections[6].pos.y = elem->corner.y + elem->height;
  box->connections[7].pos.x = elem->corner.x + elem->width - radius;
  box->connections[7].pos.y = elem->corner.y + elem->height - radius;
  box->connections[8].pos.x = elem->corner.x + elem->width / 2.0;
  box->connections[8].pos.y = elem->corner.y + elem->height / 2.0;

  box->connections[0].directions = DIR_NORTH|DIR_WEST;
  box->connections[1].directions = DIR_NORTH;
  box->connections[2].directions = DIR_NORTH|DIR_EAST;
  box->connections[3].directions = DIR_WEST;
  box->connections[4].directions = DIR_EAST;
  box->connections[5].directions = DIR_SOUTH|DIR_WEST;
  box->connections[6].directions = DIR_SOUTH;
  box->connections[7].directions = DIR_SOUTH|DIR_EAST;
  box->connections[8].directions = DIR_ALL;

  extra->border_trans = box->border_width / 2.0;
  element_update_boundingbox(elem);
  
  obj->position = elem->corner;
  
  element_update_handles(elem);

  if (radius > 0.0) {
    /* Fix the handles, too */
    elem->resize_handles[0].pos.x += radius;
    elem->resize_handles[0].pos.y += radius;
    elem->resize_handles[2].pos.x -= radius;
    elem->resize_handles[2].pos.y += radius;
    elem->resize_handles[5].pos.x += radius;
    elem->resize_handles[5].pos.y -= radius;
    elem->resize_handles[7].pos.x -= radius;
    elem->resize_handles[7].pos.y -= radius;
  }
}

static DiaObject *
box_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Box *box;
  Element *elem;
  DiaObject *obj;
  int i;

  box = g_malloc0(sizeof(Box));
  elem = &box->element;
  obj = &elem->object;
  
  obj->type = &box_type;

  obj->ops = &box_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;

  box->border_width =  attributes_get_default_linewidth();
  box->border_color = attributes_get_foreground();
  box->inner_color = attributes_get_background();
  attributes_get_default_line_style(&box->line_style, &box->dashlength);
  /* For non-default objects, this is overridden by the default */
  box->show_background = default_properties.show_background;
  box->corner_radius = default_properties.corner_radius;
  box->aspect = default_properties.aspect;

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &box->connections[i];
    box->connections[i].object = obj;
    box->connections[i].connected = NULL;
  }
  box->connections[8].flags = CP_FLAGS_MAIN;

  box_update_data(box);

  *handle1 = NULL;
  *handle2 = obj->handles[7];  
  return &box->element.object;
}

static void
box_destroy(Box *box)
{
  element_destroy(&box->element);
}

static DiaObject *
box_copy(Box *box)
{
  int i;
  Box *newbox;
  Element *elem, *newelem;
  DiaObject *newobj;
  
  elem = &box->element;
  
  newbox = g_malloc0(sizeof(Box));
  newelem = &newbox->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newbox->border_width = box->border_width;
  newbox->border_color = box->border_color;
  newbox->inner_color = box->inner_color;
  newbox->show_background = box->show_background;
  newbox->line_style = box->line_style;
  newbox->dashlength = box->dashlength;
  newbox->corner_radius = box->corner_radius;
  newbox->aspect = box->aspect;
  
  for (i=0;i<NUM_CONNECTIONS;i++) {
    newobj->connections[i] = &newbox->connections[i];
    newbox->connections[i].object = newobj;
    newbox->connections[i].connected = NULL;
    newbox->connections[i].pos = box->connections[i].pos;
    newbox->connections[i].last_pos = box->connections[i].last_pos;
    newbox->connections[i].flags = box->connections[i].flags;
  }

  return &newbox->element.object;
}

static void
box_save(Box *box, ObjectNode obj_node, const char *filename)
{
  element_save(&box->element, obj_node);

  if (box->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  box->border_width);
  
  if (!color_equals(&box->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &box->border_color);
  
  if (!color_equals(&box->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &box->inner_color);
  
  data_add_boolean(new_attribute(obj_node, "show_background"), box->show_background);

  if (box->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  box->line_style);
  
  if (box->line_style != LINESTYLE_SOLID &&
      box->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
                  box->dashlength);

  if (box->corner_radius > 0.0)
    data_add_real(new_attribute(obj_node, "corner_radius"),
		  box->corner_radius);

  if (box->aspect != FREE_ASPECT)
    data_add_enum(new_attribute(obj_node, "aspect"),
		  box->aspect);
}

static DiaObject *
box_load(ObjectNode obj_node, int version, const char *filename)
{
  Box *box;
  Element *elem;
  DiaObject *obj;
  int i;
  AttributeNode attr;

  box = g_malloc0(sizeof(Box));
  elem = &box->element;
  obj = &elem->object;
  
  obj->type = &box_type;
  obj->ops = &box_ops;

  element_load(elem, obj_node);
  
  box->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    box->border_width =  data_real( attribute_first_data(attr) );

  box->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &box->border_color);
  
  box->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &box->inner_color);
  
  box->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    box->show_background = data_boolean( attribute_first_data(attr) );

  box->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    box->line_style =  data_enum( attribute_first_data(attr) );

  box->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    box->dashlength = data_real(attribute_first_data(attr));

  box->corner_radius = 0.0;
  attr = object_find_attribute(obj_node, "corner_radius");
  if (attr != NULL)
    box->corner_radius =  data_real( attribute_first_data(attr) );

  box->aspect = FREE_ASPECT;
  attr = object_find_attribute(obj_node, "aspect");
  if (attr != NULL)
    box->aspect = data_enum(attribute_first_data(attr));

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &box->connections[i];
    box->connections[i].object = obj;
    box->connections[i].connected = NULL;
  }
  box->connections[8].flags = CP_FLAGS_MAIN;

  box_update_data(box);

  return &box->element.object;
}


struct AspectChange {
  ObjectChange obj_change;
  AspectType old_type, new_type;
  /* The points before this got applied.  Afterwards, all points can be
   * calculated.
   */
  Point topleft;
  real width, height;
};

static void
aspect_change_free(struct AspectChange *change)
{
}

static void
aspect_change_apply(struct AspectChange *change, DiaObject *obj)
{
  Box *box = (Box*)obj;

  box->aspect = change->new_type;
  box_update_data(box);
}

static void
aspect_change_revert(struct AspectChange *change, DiaObject *obj)
{
  Box *box = (Box*)obj;

  box->aspect = change->old_type;
  box->element.corner = change->topleft;
  box->element.width = change->width;
  box->element.height = change->height;
  box_update_data(box);
}

static ObjectChange *
aspect_create_change(Box *box, AspectType aspect)
{
  struct AspectChange *change;

  change = g_new0(struct AspectChange, 1);

  change->obj_change.apply = (ObjectChangeApplyFunc) aspect_change_apply;
  change->obj_change.revert = (ObjectChangeRevertFunc) aspect_change_revert;
  change->obj_change.free = (ObjectChangeFreeFunc) aspect_change_free;

  change->old_type = box->aspect;
  change->new_type = aspect;
  change->topleft = box->element.corner;
  change->width = box->element.width;
  change->height = box->element.height;

  return (ObjectChange *)change;
}


static ObjectChange *
box_set_aspect_callback (DiaObject* obj, Point* clicked, gpointer data)
{
  ObjectChange *change;

  change = aspect_create_change((Box*)obj, (AspectType)data);
  change->apply(change, obj);

  return change;
}

static DiaMenuItem box_menu_items[] = {
  { N_("Free aspect"), box_set_aspect_callback, (void*)FREE_ASPECT, 
    DIAMENU_ACTIVE|DIAMENU_TOGGLE },
  { N_("Fixed aspect"), box_set_aspect_callback, (void*)FIXED_ASPECT, 
    DIAMENU_ACTIVE|DIAMENU_TOGGLE },
  { N_("Square"), box_set_aspect_callback, (void*)SQUARE_ASPECT, 
    DIAMENU_ACTIVE|DIAMENU_TOGGLE},
};

static DiaMenu box_menu = {
  "Box",
  sizeof(box_menu_items)/sizeof(DiaMenuItem),
  box_menu_items,
  NULL
};

static DiaMenu *
box_get_object_menu(Box *box, Point *clickedpoint)
{
  /* Set entries sensitive/selected etc here */
  box_menu_items[0].active = DIAMENU_ACTIVE|DIAMENU_TOGGLE;
  box_menu_items[1].active = DIAMENU_ACTIVE|DIAMENU_TOGGLE;
  box_menu_items[2].active = DIAMENU_ACTIVE|DIAMENU_TOGGLE;

  box_menu_items[box->aspect].active = 
    DIAMENU_ACTIVE|DIAMENU_TOGGLE|DIAMENU_TOGGLE_ON;
  
  return &box_menu;
}
