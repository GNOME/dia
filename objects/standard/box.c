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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"
#include "create.h"
#include "message.h"
#include "pattern.h"


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

/*!
 * \brief Standard - Box : rectangular box
 * \extends _Element
 * \ingroup StandardObjects
 */
struct _Box {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  double border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  DiaLineStyle line_style;
  DiaLineJoin line_join;
  double dashlength;
  double corner_radius;
  AspectType aspect;
  DiaPattern *pattern;
  double angle; /*!< between [-45°-45°] to simplify connection point handling */
};

static struct _BoxProperties {
  gboolean show_background;
  double corner_radius;
  AspectType aspect;
} default_properties = { TRUE, 0.0 };


static double           box_distance_from    (Box              *box,
                                              Point            *point);
static void             box_select           (Box              *box,
                                              Point            *clicked_point,
                                              DiaRenderer      *interactive_renderer);
static DiaObjectChange *box_move_handle      (Box              *box,
                                              Handle           *handle,
                                              Point            *to,
                                              ConnectionPoint  *cp,
                                              HandleMoveReason  reason,
                                              ModifierKeys      modifiers);
static DiaObjectChange *box_move             (Box              *box,
                                              Point            *to);
static void             box_draw             (Box              *box,
                                              DiaRenderer      *renderer);
static void             box_update_data      (Box              *box);
static DiaObject       *box_create           (Point            *startpoint,
                                              void             *user_data,
                                              Handle          **handle1,
                                              Handle          **handle2);
static void             box_destroy          (Box              *box);
static DiaObject       *box_copy             (Box              *box);
static void             box_set_props        (Box              *box,
                                              GPtrArray        *props);
static void             box_save             (Box              *box,
                                              ObjectNode        obj_node,
                                              DiaContext       *ctx);
static DiaObject       *box_load             (ObjectNode        obj_node,
                                              int               version,
                                              DiaContext       *ctx);
static DiaMenu         *box_get_object_menu  (Box              *box,
                                              Point            *clickedpoint);
static gboolean         box_transform        (Box              *box,
                                              const DiaMatrix  *m);


static ObjectTypeOps box_type_ops = {
  (CreateFunc) box_create,
  (LoadFunc)   box_load,
  (SaveFunc)   box_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

static PropOffset box_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Box, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Box, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Box, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Box, show_background) },
  { "aspect", PROP_TYPE_ENUM, offsetof(Box, aspect) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Box, line_style), offsetof(Box, dashlength) },
  { "line_join", PROP_TYPE_ENUM, offsetof(Box, line_join) },
  { "corner_radius", PROP_TYPE_REAL, offsetof(Box, corner_radius) },
  { "pattern", PROP_TYPE_PATTERN, offsetof(Box, pattern) },
  { "angle", PROP_TYPE_REAL, offsetof(Box, angle) },
  { NULL, 0, 0 }
};

static PropNumData corner_radius_data = { 0.0, 10.0, 0.1 };
static PropNumData angle_data = { -45, 45, 1 };

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
  PROP_STD_LINE_JOIN_OPTIONAL,
  { "corner_radius", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Corner radius"), NULL, &corner_radius_data },
  { "aspect", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
    N_("Aspect ratio"), NULL, prop_aspect_data },
  PROP_STD_PATTERN,
  { "angle", PROP_TYPE_REAL, PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL,
    N_("Rotation"), N_("Rotation angle"), &angle_data },
  PROP_DESC_END
};

DiaObjectType box_type =
{
  "Standard - Box",  /* name */
  0,                 /* version */
  (const char **) "res:/org/gnome/Dia/objects/standard/box.png",
  &box_type_ops,      /* ops */
  NULL,              /* pixmap_file */
  0,                 /* default_user_data */
  box_props,
  box_offsets
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
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      box_get_object_menu,
  (DescribePropsFunc)   object_describe_props,
  (GetPropsFunc)        object_get_props,
  (SetPropsFunc)        box_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
  (TransformFunc)       box_transform,
};

static void
box_set_props(Box *box, GPtrArray *props)
{
  object_set_props_from_offsets(&box->element.object,
                                box_offsets, props);
  box_update_data(box);
}

static void
_box_get_poly (const Box *box, Point corners[4])
{
  const Element *elem = &box->element;

  corners[0] = elem->corner;
  corners[1] = corners[0];
  corners[1].x += elem->width;
  corners[2] = corners[1];
  corners[2].y += elem->height;
  corners[3] = corners[2];
  corners[3].x -= elem->width;

  if (box->angle != 0) {
    real cx = elem->corner.x + elem->width / 2.0;
    real cy = elem->corner.y + elem->height / 2.0;
    DiaMatrix m = { 1.0, 0.0, 0.0, 1.0, cx, cy };
    DiaMatrix t = { 1.0, 0.0, 0.0, 1.0, -cx, -cy };
    int i;

    dia_matrix_set_angle_and_scales (&m, G_PI*box->angle/180, 1.0, 1.0);
    dia_matrix_multiply (&m, &t, &m);
    for (i = 0; i < 4; ++i)
      transform_point (&corners[i], &m);
  }
}

static real
box_distance_from(Box *box, Point *point)
{
  Element *elem = &box->element;

  if (box->angle == 0) {
    DiaRectangle rect;
    rect.left = elem->corner.x - box->border_width/2;
    rect.right = elem->corner.x + elem->width + box->border_width/2;
    rect.top = elem->corner.y - box->border_width/2;
    rect.bottom = elem->corner.y + elem->height + box->border_width/2;
    return distance_rectangle_point(&rect, point);
  } else {
    Point corners[4];
    _box_get_poly (box, corners);
    return distance_polygon_point (corners, 4, box->border_width, point);
  }
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


static DiaObjectChange *
box_move_handle (Box              *box,
                 Handle           *handle,
                 Point            *to,
                 ConnectionPoint  *cp,
                 HandleMoveReason  reason,
                 ModifierKeys      modifiers)
{
  g_return_val_if_fail (box != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  if (box->aspect != FREE_ASPECT) {
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
      case HANDLE_MOVE_STARTPOINT:
      case HANDLE_MOVE_ENDPOINT:
      case HANDLE_CUSTOM1:
      case HANDLE_CUSTOM2:
      case HANDLE_CUSTOM3:
      case HANDLE_CUSTOM4:
      case HANDLE_CUSTOM5:
      case HANDLE_CUSTOM6:
      case HANDLE_CUSTOM7:
      case HANDLE_CUSTOM8:
      case HANDLE_CUSTOM9:
      default:
        new_width = width;
        new_height = height;
        break;
    }

    se_to.x = corner.x + new_width;
    se_to.y = corner.y + new_height;

    element_move_handle (&box->element, HANDLE_RESIZE_SE, &se_to, cp, reason, modifiers);
  } else {
    element_move_handle (&box->element, handle->id, to, cp, reason, modifiers);
  }

  box_update_data (box);

  return NULL;
}


static DiaObjectChange *
box_move (Box *box, Point *to)
{
  box->element.corner = *to;

  box_update_data (box);

  return NULL;
}


static void
box_draw (Box *box, DiaRenderer *renderer)
{
  Point lr_corner;
  Element *elem;

  g_return_if_fail (box != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &box->element;

  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;

  dia_renderer_set_linewidth (renderer, box->border_width);
  dia_renderer_set_linestyle (renderer, box->line_style, box->dashlength);
  if (box->corner_radius > 0) {
    dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_ROUND);
  } else {
    dia_renderer_set_linejoin (renderer, box->line_join);
  }
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  if (box->show_background) {
    Color fill = box->inner_color;
    dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
    if (box->pattern) {
      dia_pattern_get_fallback_color (box->pattern, &fill);
      if (dia_renderer_is_capable_of (renderer, RENDER_PATTERN)) {
        dia_renderer_set_pattern (renderer, box->pattern);
      }
    }
    if (box->angle == 0) {
      dia_renderer_draw_rounded_rect (renderer,
                                      &elem->corner,
                                      &lr_corner,
                                      &fill,
                                      &box->border_color,
                                      box->corner_radius);
    } else {
      Point poly[4];
      _box_get_poly (box, poly);
      dia_renderer_draw_polygon (renderer, poly, 4, &fill, &box->border_color);
    }
    if (dia_renderer_is_capable_of (renderer, RENDER_PATTERN)) {
      dia_renderer_set_pattern (renderer, NULL);
    }
  } else {
    if (box->angle == 0) {
      dia_renderer_draw_rounded_rect (renderer,
                                      &elem->corner,
                                      &lr_corner,
                                      NULL,
                                      &box->border_color,
                                      box->corner_radius);
    } else {
      Point poly[4];
      _box_get_poly (box, poly);
      dia_renderer_draw_polygon (renderer,
                                 poly,
                                 4,
                                 &box->inner_color,
                                 &box->border_color);
    }
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
  element_update_connections_rectangle (elem, box->connections);

  if (box->angle != 0) {
    real cx = elem->corner.x + elem->width / 2.0;
    real cy = elem->corner.y + elem->height / 2.0;
    DiaMatrix m = { 1.0, 0.0, 0.0, 1.0, cx, cy };
    DiaMatrix t = { 1.0, 0.0, 0.0, 1.0, -cx, -cy };
    int i;

    dia_matrix_set_angle_and_scales (&m, G_PI*box->angle/180, 1.0, 1.0);
    dia_matrix_multiply (&m, &t, &m);
    for (i = 0; i < 8; ++i)
      transform_point (&box->connections[i].pos, &m);
  }

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

  box = g_new0 (Box, 1);
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
  attributes_get_default_line_style (&box->line_style, &box->dashlength);
  box->line_join = DIA_LINE_JOIN_MITER;
  /* For non-default objects, this is overridden by the default */
  box->show_background = default_properties.show_background;
  box->corner_radius = default_properties.corner_radius;
  box->aspect = default_properties.aspect;
  box->angle = 0.0;

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
  g_clear_object (&box->pattern);
  element_destroy (&box->element);
}


static DiaObject *
box_copy(Box *box)
{
  int i;
  Box *newbox;
  Element *elem, *newelem;
  DiaObject *newobj;

  elem = &box->element;

  newbox = g_new0 (Box, 1);
  newelem = &newbox->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newbox->border_width = box->border_width;
  newbox->border_color = box->border_color;
  newbox->inner_color = box->inner_color;
  newbox->show_background = box->show_background;
  newbox->line_style = box->line_style;
  newbox->line_join = box->line_join;
  newbox->dashlength = box->dashlength;
  newbox->corner_radius = box->corner_radius;
  newbox->aspect = box->aspect;
  newbox->angle = box->angle;
  if (box->pattern)
    newbox->pattern = g_object_ref (box->pattern);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    newobj->connections[i] = &newbox->connections[i];
    newbox->connections[i].object = newobj;
    newbox->connections[i].connected = NULL;
    newbox->connections[i].pos = box->connections[i].pos;
    newbox->connections[i].flags = box->connections[i].flags;
  }

  return &newbox->element.object;
}

static void
box_save(Box *box, ObjectNode obj_node, DiaContext *ctx)
{
  element_save(&box->element, obj_node, ctx);

  if (box->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  box->border_width, ctx);

  if (!color_equals(&box->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &box->border_color, ctx);

  if (!color_equals(&box->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &box->inner_color, ctx);

  data_add_boolean(new_attribute(obj_node, "show_background"),
		   box->show_background, ctx);

  if (box->line_style != DIA_LINE_STYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  box->line_style, ctx);

  if (box->line_style != DIA_LINE_STYLE_SOLID &&
      box->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
                  box->dashlength, ctx);

  if (box->line_join != DIA_LINE_JOIN_MITER) {
    data_add_enum (new_attribute (obj_node, "line_join"),
                   box->line_join,
                   ctx);
  }

  if (box->corner_radius > 0.0)
    data_add_real(new_attribute(obj_node, "corner_radius"),
		  box->corner_radius, ctx);

  if (box->aspect != FREE_ASPECT)
    data_add_enum(new_attribute(obj_node, "aspect"),
		  box->aspect, ctx);

  if (box->pattern)
    data_add_pattern(new_attribute(obj_node, "pattern"),
		     box->pattern, ctx);

  if (box->angle != 0.0)
    data_add_real(new_attribute(obj_node, "angle"),
		  box->angle, ctx);

}

static DiaObject *
box_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  Box *box;
  Element *elem;
  DiaObject *obj;
  int i;
  AttributeNode attr;

  box = g_new0 (Box, 1);
  elem = &box->element;
  obj = &elem->object;

  obj->type = &box_type;
  obj->ops = &box_ops;

  element_load(elem, obj_node, ctx);

  box->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    box->border_width =  data_real(attribute_first_data(attr), ctx);

  box->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &box->border_color, ctx);

  box->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &box->inner_color, ctx);

  box->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    box->show_background = data_boolean(attribute_first_data(attr), ctx);

  box->line_style = DIA_LINE_STYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    box->line_style =  data_enum(attribute_first_data(attr), ctx);

  box->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    box->dashlength = data_real(attribute_first_data(attr), ctx);

  box->line_join = DIA_LINE_JOIN_MITER;
  attr = object_find_attribute (obj_node, "line_join");
  if (attr != NULL) {
    box->line_join = data_enum (attribute_first_data (attr), ctx);
  }

  box->corner_radius = 0.0;
  attr = object_find_attribute(obj_node, "corner_radius");
  if (attr != NULL)
    box->corner_radius =  data_real(attribute_first_data(attr), ctx);

  box->aspect = FREE_ASPECT;
  attr = object_find_attribute(obj_node, "aspect");
  if (attr != NULL)
    box->aspect = data_enum(attribute_first_data(attr), ctx);

  attr = object_find_attribute(obj_node, "pattern");
  if (attr != NULL)
    box->pattern = data_pattern(attribute_first_data(attr), ctx);

  box->angle = 0.0;
  attr = object_find_attribute(obj_node, "angle");
  if (attr != NULL)
    box->angle =  data_real(attribute_first_data(attr), ctx);

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


#define DIA_TYPE_BOX_ASPECT_OBJECT_CHANGE dia_box_aspect_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaBoxAspectObjectChange,
                      dia_box_aspect_object_change,
                      DIA, BOX_ASPECT_OBJECT_CHANGE,
                      DiaObjectChange)


struct _DiaBoxAspectObjectChange {
  DiaObjectChange obj_change;
  AspectType old_type, new_type;
  /* The points before this got applied.  Afterwards, all points can be
   * calculated.
   */
  Point topleft;
  double width, height;
};


DIA_DEFINE_OBJECT_CHANGE (DiaBoxAspectObjectChange, dia_box_aspect_object_change)


static void
dia_box_aspect_object_change_free (DiaObjectChange *self)
{
}


static void
dia_box_aspect_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaBoxAspectObjectChange *change = DIA_BOX_ASPECT_OBJECT_CHANGE (self);
  Box *box = (Box *) obj;

  box->aspect = change->new_type;
  box_update_data (box);
}


static void
dia_box_aspect_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaBoxAspectObjectChange *change = DIA_BOX_ASPECT_OBJECT_CHANGE (self);
  Box *box = (Box *) obj;

  box->aspect = change->old_type;
  box->element.corner = change->topleft;
  box->element.width = change->width;
  box->element.height = change->height;
  box_update_data (box);
}


static DiaObjectChange *
aspect_create_change (Box *box, AspectType aspect)
{
  DiaBoxAspectObjectChange *change;

  change = dia_object_change_new (DIA_TYPE_BOX_ASPECT_OBJECT_CHANGE);

  change->old_type = box->aspect;
  change->new_type = aspect;
  change->topleft = box->element.corner;
  change->width = box->element.width;
  change->height = box->element.height;

  return DIA_OBJECT_CHANGE (change);
}


static DiaObjectChange *
box_set_aspect_callback (DiaObject* obj, Point* clicked, gpointer data)
{
  DiaObjectChange *change;

  change = aspect_create_change ((Box*) obj, (AspectType) data);
  dia_object_change_apply (change, obj);

  return change;
}


static DiaMenuItem box_menu_items[] = {
  { N_("Free aspect"), box_set_aspect_callback, (void*)FREE_ASPECT,
    DIAMENU_ACTIVE|DIAMENU_TOGGLE },
  { N_("Fixed aspect"), box_set_aspect_callback, (void*)FIXED_ASPECT,
    DIAMENU_ACTIVE|DIAMENU_TOGGLE },
  { N_("Square"), box_set_aspect_callback, (void*)SQUARE_ASPECT,
    DIAMENU_ACTIVE|DIAMENU_TOGGLE}
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

static gboolean
box_transform(Box *box, const DiaMatrix *m)
{
  real a, sx, sy;

  g_return_val_if_fail(m != NULL, FALSE);

  if (!dia_matrix_get_angle_and_scales (m, &a, &sx, &sy)) {
    dia_log_message ("box_transform() can't convert given matrix");
    return FALSE;
  } else {
    real width = box->element.width * sx;
    real height = box->element.height * sy;
    real angle = a*180/G_PI;
    Point c = { box->element.corner.x + width/2.0, box->element.corner.y + height/2.0 };

    /* rotation is invariant to the center */
    transform_point (&c, m);
    /* XXX: we have to bring angle in range [-45..45] which may swap width and height */
    box->angle = angle;
    box->element.width = width;
    box->element.height = height;
    box->element.corner.x = c.x - width / 2.0;
    box->element.corner.y = c.y - height / 2.0;
 }

  box_update_data(box);
  return TRUE;
}
