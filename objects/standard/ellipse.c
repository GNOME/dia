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

#include "pixmaps/ellipse.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.15

typedef enum {
  FREE_ASPECT,
  FIXED_ASPECT,
  CIRCLE_ASPECT
} AspectType;

typedef struct _Ellipse Ellipse;

struct _Ellipse {
  Element element;

  ConnectionPoint connections[9];
  Handle center_handle;
  
  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  AspectType aspect;
  LineStyle line_style;
  real dashlength;
};

static struct _EllipseProperties {
  AspectType aspect;
  gboolean show_background;
} default_properties = { FREE_ASPECT, TRUE };

static real ellipse_distance_from(Ellipse *ellipse, Point *point);
static void ellipse_select(Ellipse *ellipse, Point *clicked_point,
			   DiaRenderer *interactive_renderer);
static ObjectChange* ellipse_move_handle(Ellipse *ellipse, Handle *handle,
					 Point *to, ConnectionPoint *cp,
					 HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* ellipse_move(Ellipse *ellipse, Point *to);
static void ellipse_draw(Ellipse *ellipse, DiaRenderer *renderer);
static void ellipse_update_data(Ellipse *ellipse);
static DiaObject *ellipse_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void ellipse_destroy(Ellipse *ellipse);
static DiaObject *ellipse_copy(Ellipse *ellipse);

static PropDescription *ellipse_describe_props(Ellipse *ellipse);
static void ellipse_get_props(Ellipse *ellipse, GPtrArray *props);
static void ellipse_set_props(Ellipse *ellipse, GPtrArray *props);

static void ellipse_save(Ellipse *ellipse, ObjectNode obj_node, const char *filename);
static DiaObject *ellipse_load(ObjectNode obj_node, int version, const char *filename);
static DiaMenu *ellipse_get_object_menu(Ellipse *ellipse, Point *clickedpoint);

static ObjectTypeOps ellipse_type_ops =
{
  (CreateFunc) ellipse_create,
  (LoadFunc)   ellipse_load,
  (SaveFunc)   ellipse_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType ellipse_type =
{
  "Standard - Ellipse",   /* name */
  0,                      /* version */
  (char **) ellipse_xpm,  /* pixmap */
  
  &ellipse_type_ops       /* ops */
};

DiaObjectType *_ellipse_type = (DiaObjectType *) &ellipse_type;

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
  (ObjectMenuFunc)      ellipse_get_object_menu,
  (DescribePropsFunc)   ellipse_describe_props,
  (GetPropsFunc)        ellipse_get_props,
  (SetPropsFunc)        ellipse_set_props,
};

static PropEnumData prop_aspect_data[] = {
  { N_("Free"), FREE_ASPECT },
  { N_("Fixed"), FIXED_ASPECT },
  { N_("Circle"), CIRCLE_ASPECT },
  { NULL, 0 }
};
static PropDescription ellipse_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
  PROP_STD_LINE_STYLE,
  { "aspect", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
    N_("Aspect ratio"), NULL, prop_aspect_data },
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
  { "aspect", PROP_TYPE_ENUM, offsetof(Ellipse, aspect) },
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
  Element *elem = &ellipse->element;
  Point center;

  center.x = elem->corner.x+elem->width/2;
  center.y = elem->corner.y+elem->height/2;

  return distance_ellipse_point(&center, elem->width, elem->height,
				ellipse->border_width, point);
}

static void
ellipse_select(Ellipse *ellipse, Point *clicked_point,
	       DiaRenderer *interactive_renderer)
{
  element_update_handles(&ellipse->element);
}

static ObjectChange*
ellipse_move_handle(Ellipse *ellipse, Handle *handle,
		    Point *to, ConnectionPoint *cp,
		    HandleMoveReason reason, ModifierKeys modifiers)
{
  Element *elem = &ellipse->element;
  Point nw_to, se_to;

  assert(ellipse!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8 || handle->id == HANDLE_CUSTOM1);
  if (handle->id == HANDLE_CUSTOM1) {
    Point delta, corner_to;
    delta.x = to->x - (elem->corner.x + elem->width/2);
    delta.y = to->y - (elem->corner.y + elem->height/2);
    corner_to.x = elem->corner.x + delta.x;
    corner_to.y = elem->corner.y + delta.y;
    return ellipse_move(ellipse, &corner_to);
  } else {
    if (ellipse->aspect != FREE_ASPECT){
        float width, height;
        float new_width, new_height;
        float to_width, aspect_width;
        Point center;

        width = ellipse->element.width;
        height = ellipse->element.height;
        center.x = elem->corner.x + width/2;
        center.y = elem->corner.y + height/2;
        switch (handle->id) {
        case HANDLE_RESIZE_E:
        case HANDLE_RESIZE_W:
            new_width = 2 * fabs(to->x - center.x);
            new_height = new_width / width * height;
            break;
        case HANDLE_RESIZE_N:
        case HANDLE_RESIZE_S:
            new_height = 2 * fabs(to->y - center.y);
            new_width = new_height / height * width;
            break;
        case HANDLE_RESIZE_NW:
        case HANDLE_RESIZE_NE:
        case HANDLE_RESIZE_SW:
        case HANDLE_RESIZE_SE:
            to_width = 2 * fabs(to->x - center.x);
            aspect_width = 2 * fabs(to->y - center.y) / height * width;
            new_width = to_width < aspect_width ? to_width : aspect_width;
            new_height = new_width / width * height;
            break;
	default: 
	    new_width = width;
	    new_height = height;
	    break;
        }
	
        nw_to.x = center.x - new_width/2;
        nw_to.y = center.y - new_height/2;
        se_to.x = center.x + new_width/2;
        se_to.y = center.y + new_height/2;
        
        element_move_handle(&ellipse->element, HANDLE_RESIZE_NW, &nw_to, cp, reason, modifiers);
        element_move_handle(&ellipse->element, HANDLE_RESIZE_SE, &se_to, cp, reason, modifiers);
    } else {
        Point center;
        Point opposite_to;
        center.x = elem->corner.x + elem->width/2;
        center.y = elem->corner.y + elem->height/2;
        opposite_to.x = center.x - (to->x-center.x);
        opposite_to.y = center.y - (to->y-center.y);

        element_move_handle(&ellipse->element, handle->id, to, cp, reason, modifiers);
        element_move_handle(&ellipse->element, 7-handle->id, &opposite_to, cp, reason, modifiers);
    }

    ellipse_update_data(ellipse);

    return NULL;
  }
}

static ObjectChange*
ellipse_move(Ellipse *ellipse, Point *to)
{
  ellipse->element.corner = *to;
  ellipse_update_data(ellipse);

  return NULL;
}

static void
ellipse_draw(Ellipse *ellipse, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  Point center;
  Element *elem;
  
  assert(ellipse != NULL);
  assert(renderer != NULL);

  elem = &ellipse->element;

  center.x = elem->corner.x + elem->width/2;
  center.y = elem->corner.y + elem->height/2;

  if (ellipse->show_background) {
    renderer_ops->set_fillstyle(renderer, FILLSTYLE_SOLID);

    renderer_ops->fill_ellipse(renderer, 
				&center,
				elem->width, elem->height,
				&ellipse->inner_color);
  }

  renderer_ops->set_linewidth(renderer, ellipse->border_width);
  renderer_ops->set_linestyle(renderer, ellipse->line_style);
  renderer_ops->set_dashlength(renderer, ellipse->dashlength);

  renderer_ops->draw_ellipse(renderer, 
			  &center,
			  elem->width, elem->height,
			  &ellipse->border_color);
}

static void
ellipse_update_data(Ellipse *ellipse)
{
  Element *elem = &ellipse->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  DiaObject *obj = &elem->object;
  Point center;
  real half_x, half_y;
  
  /* handle circle implies height=width */
  if (ellipse->aspect == CIRCLE_ASPECT){
	float size = elem->height < elem->width ? elem->height : elem->width;
	elem->height = elem->width = size;
  }

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
  ellipse->connections[8].pos.x = center.x;
  ellipse->connections[8].pos.y = center.y;

  /* Update directions -- if the ellipse is very thin, these may not be good */
  ellipse->connections[0].directions = DIR_NORTH|DIR_WEST;
  ellipse->connections[1].directions = DIR_NORTH;
  ellipse->connections[2].directions = DIR_NORTH|DIR_EAST;
  ellipse->connections[3].directions = DIR_WEST;
  ellipse->connections[4].directions = DIR_EAST;
  ellipse->connections[5].directions = DIR_SOUTH|DIR_WEST;
  ellipse->connections[6].directions = DIR_SOUTH;
  ellipse->connections[7].directions = DIR_SOUTH|DIR_EAST;
  ellipse->connections[8].directions = DIR_ALL;

  extra->border_trans = ellipse->border_width / 2.0;
  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);

  obj->handles[8]->pos.x = center.x;
  obj->handles[8]->pos.y = center.y;
}

static DiaObject *
ellipse_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Ellipse *ellipse;
  Element *elem;
  DiaObject *obj;
  int i;

  ellipse = g_malloc0(sizeof(Ellipse));
  elem = &ellipse->element;
  obj = &elem->object;
  
  obj->type = &ellipse_type;

  obj->ops = &ellipse_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;

  ellipse->border_width =  attributes_get_default_linewidth();
  ellipse->border_color = attributes_get_foreground();
  ellipse->inner_color = attributes_get_background();
  attributes_get_default_line_style(&ellipse->line_style,
				    &ellipse->dashlength);
  ellipse->show_background = default_properties.show_background;
  ellipse->aspect = default_properties.aspect;

  element_init(elem, 9, 9);

  obj->handles[8] = &ellipse->center_handle;
  obj->handles[8]->id = HANDLE_CUSTOM1;
  obj->handles[8]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[8]->connected_to = NULL;
  obj->handles[8]->connect_type = HANDLE_NONCONNECTABLE;

  for (i=0;i<9;i++) {
    obj->connections[i] = &ellipse->connections[i];
    ellipse->connections[i].object = obj;
    ellipse->connections[i].connected = NULL;
  }
  ellipse->connections[8].flags = CP_FLAGS_MAIN;
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

static DiaObject *
ellipse_copy(Ellipse *ellipse)
{
  int i;
  Ellipse *newellipse;
  Element *elem, *newelem;
  DiaObject *newobj;
  
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
  newellipse->aspect = ellipse->aspect;
  newellipse->line_style = ellipse->line_style;

  newobj->handles[8] = &newellipse->center_handle;
  newellipse->center_handle = ellipse->center_handle;
  newellipse->center_handle.connected_to = NULL;

  for (i=0;i<9;i++) {
    newobj->connections[i] = &newellipse->connections[i];
    newellipse->connections[i].object = newobj;
    newellipse->connections[i].connected = NULL;
    newellipse->connections[i].pos = ellipse->connections[i].pos;
    newellipse->connections[i].last_pos = ellipse->connections[i].last_pos;
    newellipse->connections[i].flags = ellipse->connections[i].flags;
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
  
  if (ellipse->aspect != FREE_ASPECT)
    data_add_enum(new_attribute(obj_node, "aspect"),
		  ellipse->aspect);

  if (ellipse->line_style != LINESTYLE_SOLID) {
    data_add_enum(new_attribute(obj_node, "line_style"),
		  ellipse->line_style);

    if (ellipse->dashlength != DEFAULT_LINESTYLE_DASHLEN)
	    data_add_real(new_attribute(obj_node, "dashlength"),
			  ellipse->dashlength);
  }
}

static DiaObject *ellipse_load(ObjectNode obj_node, int version, const char *filename)
{
  Ellipse *ellipse;
  Element *elem;
  DiaObject *obj;
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

  ellipse->aspect = FREE_ASPECT;
  attr = object_find_attribute(obj_node, "aspect");
  if (attr != NULL)
    ellipse->aspect = data_enum(attribute_first_data(attr));

  ellipse->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    ellipse->line_style =  data_enum( attribute_first_data(attr) );

  ellipse->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
	  ellipse->dashlength = data_real(attribute_first_data(attr));

  element_init(elem, 9, 9);

  obj->handles[8] = &ellipse->center_handle;
  obj->handles[8]->id = HANDLE_CUSTOM1;
  obj->handles[8]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[8]->connected_to = NULL;
  obj->handles[8]->connect_type = HANDLE_NONCONNECTABLE;

  for (i=0;i<9;i++) {
    obj->connections[i] = &ellipse->connections[i];
    ellipse->connections[i].object = obj;
    ellipse->connections[i].connected = NULL;
  }
  ellipse->connections[8].flags = CP_FLAGS_MAIN;

  ellipse_update_data(ellipse);

  return &ellipse->element.object;
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
  Ellipse *ellipse = (Ellipse*)obj;

  ellipse->aspect = change->new_type;
  ellipse_update_data(ellipse);
}

static void
aspect_change_revert(struct AspectChange *change, DiaObject *obj)
{
  Ellipse *ellipse = (Ellipse*)obj;

  ellipse->aspect = change->old_type;
  ellipse->element.corner = change->topleft;
  ellipse->element.width = change->width;
  ellipse->element.height = change->height;
  ellipse_update_data(ellipse);
}

static ObjectChange *
aspect_create_change(Ellipse *ellipse, AspectType aspect)
{
  struct AspectChange *change;

  change = g_new0(struct AspectChange, 1);

  change->obj_change.apply = (ObjectChangeApplyFunc) aspect_change_apply;
  change->obj_change.revert = (ObjectChangeRevertFunc) aspect_change_revert;
  change->obj_change.free = (ObjectChangeFreeFunc) aspect_change_free;

  change->old_type = ellipse->aspect;
  change->new_type = aspect;
  change->topleft = ellipse->element.corner;
  change->width = ellipse->element.width;
  change->height = ellipse->element.height;

  return (ObjectChange *)change;
}


static ObjectChange *
ellipse_set_aspect_callback (DiaObject* obj, Point* clicked, gpointer data)
{
  ObjectChange *change;

  change = aspect_create_change((Ellipse*)obj, (AspectType)data);
  change->apply(change, obj);

  return change;
}

static DiaMenuItem ellipse_menu_items[] = {
  { N_("Free aspect"), ellipse_set_aspect_callback, (void*)FREE_ASPECT, 
    DIAMENU_ACTIVE|DIAMENU_TOGGLE },
  { N_("Fixed aspect"), ellipse_set_aspect_callback, (void*)FIXED_ASPECT, 
    DIAMENU_ACTIVE|DIAMENU_TOGGLE },
  { N_("Circle"), ellipse_set_aspect_callback, (void*)CIRCLE_ASPECT, 
    DIAMENU_ACTIVE|DIAMENU_TOGGLE},
};

static DiaMenu ellipse_menu = {
  "Ellipse",
  sizeof(ellipse_menu_items)/sizeof(DiaMenuItem),
  ellipse_menu_items,
  NULL
};

static DiaMenu *
ellipse_get_object_menu(Ellipse *ellipse, Point *clickedpoint)
{
  /* Set entries sensitive/selected etc here */
  ellipse_menu_items[0].active = DIAMENU_ACTIVE|DIAMENU_TOGGLE;
  ellipse_menu_items[1].active = DIAMENU_ACTIVE|DIAMENU_TOGGLE;
  ellipse_menu_items[2].active = DIAMENU_ACTIVE|DIAMENU_TOGGLE;

  ellipse_menu_items[ellipse->aspect].active = 
    DIAMENU_ACTIVE|DIAMENU_TOGGLE|DIAMENU_TOGGLE_ON;
  
  return &ellipse_menu;
}
