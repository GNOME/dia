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
#include "pattern.h"
#include "diapathrenderer.h"
#include "message.h"


#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.15

typedef enum {
  FREE_ASPECT,
  FIXED_ASPECT,
  CIRCLE_ASPECT
} AspectType;

typedef struct _Ellipse Ellipse;

/*!
 * \brief Standard - Ellipse
 * \extends _Element
 * \ingroup StandardObjects
 */
struct _Ellipse {
  Element element;

  ConnectionPoint connections[9];
  Handle center_handle;

  double border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  AspectType aspect;
  DiaLineStyle line_style;
  double dashlength;
  DiaPattern *pattern;
  double angle; /*!< between [-45-45] to simplify connection point handling */
};

static struct _EllipseProperties {
  AspectType aspect;
  gboolean show_background;
} default_properties = { FREE_ASPECT, TRUE };


static double           ellipse_distance_from   (Ellipse          *ellipse,
                                                 Point            *point);
static void             ellipse_select          (Ellipse          *ellipse,
                                                 Point            *clicked_point,
                                                 DiaRenderer      *interactive_renderer);
static DiaObjectChange *ellipse_move_handle     (Ellipse          *ellipse,
                                                 Handle           *handle,
                                                 Point            *to,
                                                 ConnectionPoint  *cp,
                                                 HandleMoveReason  reason,
                                                 ModifierKeys      modifiers);
static DiaObjectChange *ellipse_move            (Ellipse          *ellipse,
                                                 Point            *to);
static void ellipse_draw(Ellipse *ellipse, DiaRenderer *renderer);
static void ellipse_update_data(Ellipse *ellipse);
static DiaObject *ellipse_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void ellipse_destroy(Ellipse *ellipse);
static DiaObject *ellipse_copy(Ellipse *ellipse);

static void ellipse_set_props(Ellipse *ellipse, GPtrArray *props);

static void ellipse_save(Ellipse *ellipse, ObjectNode obj_node, DiaContext *ctx);
static DiaObject *ellipse_load(ObjectNode obj_node, int version, DiaContext *ctx);
static DiaMenu *ellipse_get_object_menu(Ellipse *ellipse, Point *clickedpoint);
static gboolean ellipse_transform(Ellipse *ellipse, const DiaMatrix *m);

static ObjectTypeOps ellipse_type_ops =
{
  (CreateFunc) ellipse_create,
  (LoadFunc)   ellipse_load,
  (SaveFunc)   ellipse_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

static PropOffset ellipse_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Ellipse, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Ellipse, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Ellipse, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Ellipse, show_background) },
  { "aspect", PROP_TYPE_ENUM, offsetof(Ellipse, aspect) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Ellipse, line_style), offsetof(Ellipse, dashlength) },
  { "pattern", PROP_TYPE_PATTERN, offsetof(Ellipse, pattern) },
  { "angle", PROP_TYPE_REAL, offsetof(Ellipse, angle) },
  { NULL, 0, 0 }
};

static PropNumData angle_data = { -45, 45, 1 };

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
  PROP_STD_PATTERN,
  { "angle", PROP_TYPE_REAL, PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL,
    N_("Rotation"), N_("Rotation angle"), &angle_data },
  PROP_DESC_END
};

DiaObjectType ellipse_type =
{
  "Standard - Ellipse",   /* name */
  0,                      /* version */
  (const char **) "res:/org/gnome/Dia/objects/standard/ellipse.png",
  &ellipse_type_ops,      /* ops */
  NULL,
  0,
  ellipse_props,
  ellipse_offsets
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
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      ellipse_get_object_menu,
  (DescribePropsFunc)   object_describe_props,
  (GetPropsFunc)        object_get_props,
  (SetPropsFunc)        ellipse_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
  (TransformFunc)       ellipse_transform,
};

static void
ellipse_set_props(Ellipse *ellipse, GPtrArray *props)
{

  object_set_props_from_offsets(&ellipse->element.object,
                                ellipse_offsets, props);

  ellipse_update_data(ellipse);
}

static GArray *
_ellipse_to_path (Ellipse *ellipse, Point *center)
{
  Element *elem = &ellipse->element;
  DiaMatrix m = { 1.0, 0.0, 0.0, 1.0, center->x, center->y };
  DiaMatrix t = { 1.0, 0.0, 0.0, 1.0, -center->x, -center->y };
  GArray *path;
  int i;

  dia_matrix_set_angle_and_scales (&m, G_PI*ellipse->angle/180, 1.0, 1.0);
  dia_matrix_multiply (&m, &t, &m);
  path = g_array_new (FALSE, FALSE, sizeof(BezPoint));
  path_build_ellipse (path, center, elem->width, elem->height);
  for (i = 0; i < path->len; ++i)
    transform_bezpoint (&g_array_index (path, BezPoint, i), &m);
  return path;
}

static real
ellipse_distance_from(Ellipse *ellipse, Point *point)
{
  Element *elem = &ellipse->element;
  Point center;

  center.x = elem->corner.x+elem->width/2;
  center.y = elem->corner.y+elem->height/2;

  if (ellipse->angle != 0) {
    real dist;
    GArray *path = _ellipse_to_path (ellipse, &center);

    dist = distance_bez_shape_point (&g_array_index (path, BezPoint, 0), path->len,
				     ellipse->border_width, point);
    g_array_free (path, TRUE);
    return dist;
  }
  return distance_ellipse_point(&center, elem->width, elem->height,
				ellipse->border_width, point);
}


static void
ellipse_select (Ellipse     *ellipse,
                Point       *clicked_point,
                DiaRenderer *interactive_renderer)
{
  element_update_handles (&ellipse->element);
}


static DiaObjectChange*
ellipse_move_handle (Ellipse          *ellipse,
                     Handle           *handle,
                     Point            *to,
                     ConnectionPoint  *cp,
                     HandleMoveReason  reason,
                     ModifierKeys      modifiers)
{
  Element *elem = &ellipse->element;
  Point nw_to, se_to;

  g_return_val_if_fail (ellipse != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  g_return_val_if_fail (handle->id < 8 || handle->id == HANDLE_CUSTOM1, NULL);

  if (handle->id == HANDLE_CUSTOM1) {
    Point delta, corner_to;
    delta.x = to->x - (elem->corner.x + elem->width/2);
    delta.y = to->y - (elem->corner.y + elem->height/2);
    corner_to.x = elem->corner.x + delta.x;
    corner_to.y = elem->corner.y + delta.y;
    return ellipse_move (ellipse, &corner_to);
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

      nw_to.x = center.x - new_width/2;
      nw_to.y = center.y - new_height/2;
      se_to.x = center.x + new_width/2;
      se_to.y = center.y + new_height/2;

      element_move_handle (&ellipse->element, HANDLE_RESIZE_NW, &nw_to, cp, reason, modifiers);
      element_move_handle (&ellipse->element, HANDLE_RESIZE_SE, &se_to, cp, reason, modifiers);
    } else {
      Point center;
      Point opposite_to;
      center.x = elem->corner.x + elem->width/2;
      center.y = elem->corner.y + elem->height/2;
      opposite_to.x = center.x - (to->x-center.x);
      opposite_to.y = center.y - (to->y-center.y);

      element_move_handle (&ellipse->element, handle->id, to, cp, reason, modifiers);
      /* this second move screws the intended object size, e.g. from dot2dia.py
       * but without it the 'centered' behaviour during edit is screwed
       */
      element_move_handle (&ellipse->element, 7-handle->id, &opposite_to, cp, reason, modifiers);
    }

    ellipse_update_data (ellipse);

    return NULL;
  }
}


static DiaObjectChange *
ellipse_move (Ellipse *ellipse, Point *to)
{
  ellipse->element.corner = *to;
  ellipse_update_data (ellipse);

  return NULL;
}


static void
ellipse_draw (Ellipse *ellipse, DiaRenderer *renderer)
{
  Point center;
  Element *elem;
  GArray *path = NULL;

  g_return_if_fail (ellipse != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &ellipse->element;

  center.x = elem->corner.x + elem->width/2;
  center.y = elem->corner.y + elem->height/2;

  if (ellipse->angle != 0) {
    path = _ellipse_to_path (ellipse, &center);
  }

  dia_renderer_set_linewidth (renderer, ellipse->border_width);
  dia_renderer_set_linestyle (renderer, ellipse->line_style, ellipse->dashlength);

  if (ellipse->show_background) {
    Color fill = ellipse->inner_color;

    dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);

    if (ellipse->pattern) {
      dia_pattern_get_fallback_color (ellipse->pattern, &fill);

      if (dia_renderer_is_capable_of (renderer, RENDER_PATTERN)) {
        dia_renderer_set_pattern (renderer, ellipse->pattern);
      }
    }

    if (!path) {
      dia_renderer_draw_ellipse (renderer,
                                 &center,
                                 elem->width,
                                 elem->height,
                                 &fill,
                                 &ellipse->border_color);
    } else {
      dia_renderer_draw_beziergon (renderer,
                                   &g_array_index (path, BezPoint, 0),
                                   path->len,
                                   &fill, &ellipse->border_color);
    }

    if (dia_renderer_is_capable_of (renderer, RENDER_PATTERN)) {
      dia_renderer_set_pattern (renderer, NULL);
    }
  } else {
    if (!path) {
      dia_renderer_draw_ellipse (renderer,
                                 &center,
                                 elem->width,
                                 elem->height,
                                 NULL,
                                 &ellipse->border_color);
    } else {
      dia_renderer_draw_beziergon (renderer,
                                   &g_array_index (path, BezPoint, 0),
                                   path->len,
                                   NULL,
                                   &ellipse->border_color);
    }
  }

  if (path) {
    g_array_free (path, TRUE);
  }
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

  if (ellipse->angle != 0) {
    DiaMatrix m = { 1.0, 0.0, 0.0, 1.0, center.x, center.y };
    DiaMatrix t = { 1.0, 0.0, 0.0, 1.0, -center.x, -center.y };
    int i;

    dia_matrix_set_angle_and_scales (&m, G_PI*ellipse->angle/180, 1.0, 1.0);
    dia_matrix_multiply (&m, &t, &m);
    for (i = 0; i < 8; ++i)
      transform_point (&ellipse->connections[i].pos, &m);
  }
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

  ellipse = g_new0 (Ellipse, 1);
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
  g_clear_object (&ellipse->pattern);
  element_destroy (&ellipse->element);
}


static DiaObject *
ellipse_copy(Ellipse *ellipse)
{
  int i;
  Ellipse *newellipse;
  Element *elem, *newelem;
  DiaObject *newobj;

  elem = &ellipse->element;

  newellipse = g_new0 (Ellipse, 1);
  newelem = &newellipse->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newellipse->border_width = ellipse->border_width;
  newellipse->border_color = ellipse->border_color;
  newellipse->inner_color = ellipse->inner_color;
  newellipse->dashlength = ellipse->dashlength;
  newellipse->show_background = ellipse->show_background;
  newellipse->aspect = ellipse->aspect;
  newellipse->angle = ellipse->angle;
  newellipse->line_style = ellipse->line_style;
  if (ellipse->pattern)
    newellipse->pattern = g_object_ref (ellipse->pattern);

  newellipse->center_handle = ellipse->center_handle;
  newellipse->center_handle.connected_to = NULL;
  newobj->handles[8] = &newellipse->center_handle;

  for (i=0;i<9;i++) {
    newobj->connections[i] = &newellipse->connections[i];
    newellipse->connections[i].object = newobj;
    newellipse->connections[i].connected = NULL;
    newellipse->connections[i].pos = ellipse->connections[i].pos;
    newellipse->connections[i].flags = ellipse->connections[i].flags;
  }

  return &newellipse->element.object;
}


static void
ellipse_save(Ellipse *ellipse, ObjectNode obj_node, DiaContext *ctx)
{
  element_save(&ellipse->element, obj_node, ctx);

  if (ellipse->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  ellipse->border_width, ctx);

  if (!color_equals(&ellipse->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &ellipse->border_color, ctx);

  if (!color_equals(&ellipse->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &ellipse->inner_color, ctx);

  if (!ellipse->show_background)
    data_add_boolean(new_attribute(obj_node, "show_background"),
		     ellipse->show_background, ctx);

  if (ellipse->aspect != FREE_ASPECT)
    data_add_enum(new_attribute(obj_node, "aspect"),
		  ellipse->aspect, ctx);
  if (ellipse->angle != 0.0)
    data_add_real(new_attribute(obj_node, "angle"),
		  ellipse->angle, ctx);

  if (ellipse->line_style != DIA_LINE_STYLE_SOLID) {
    data_add_enum(new_attribute(obj_node, "line_style"),
		  ellipse->line_style, ctx);

    if (ellipse->dashlength != DEFAULT_LINESTYLE_DASHLEN)
	    data_add_real(new_attribute(obj_node, "dashlength"),
			  ellipse->dashlength, ctx);
  }

  if (ellipse->pattern)
    data_add_pattern(new_attribute(obj_node, "pattern"),
		     ellipse->pattern, ctx);
}

static DiaObject *ellipse_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  Ellipse *ellipse;
  Element *elem;
  DiaObject *obj;
  int i;
  AttributeNode attr;

  ellipse = g_new0 (Ellipse, 1);
  elem = &ellipse->element;
  obj = &elem->object;

  obj->type = &ellipse_type;
  obj->ops = &ellipse_ops;

  element_load(elem, obj_node, ctx);

  ellipse->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    ellipse->border_width =  data_real(attribute_first_data(attr), ctx);

  ellipse->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &ellipse->border_color, ctx);

  ellipse->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &ellipse->inner_color, ctx);

  ellipse->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    ellipse->show_background = data_boolean(attribute_first_data(attr), ctx);

  ellipse->aspect = FREE_ASPECT;
  attr = object_find_attribute(obj_node, "aspect");
  if (attr != NULL)
    ellipse->aspect = data_enum(attribute_first_data(attr), ctx);

  ellipse->angle = 0.0;
  attr = object_find_attribute(obj_node, "angle");
  if (attr != NULL)
    ellipse->angle =  data_real(attribute_first_data(attr), ctx);

  ellipse->line_style = DIA_LINE_STYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    ellipse->line_style =  data_enum(attribute_first_data(attr), ctx);

  ellipse->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    ellipse->dashlength = data_real(attribute_first_data(attr), ctx);

  attr = object_find_attribute(obj_node, "pattern");
  if (attr != NULL)
    ellipse->pattern = data_pattern(attribute_first_data(attr), ctx);

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


#define DIA_TYPE_ELLIPES_ASPECT_OBJECT_CHANGE dia_ellipse_aspect_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaEllipseAspectObjectChange,
                      dia_ellipse_aspect_object_change,
                      DIA, ELLIPES_ASPECT_OBJECT_CHANGE,
                      DiaObjectChange)


struct _DiaEllipseAspectObjectChange {
  DiaObjectChange obj_change;
  AspectType old_type, new_type;
  /* The points before this got applied.  Afterwards, all points can be
   * calculated.
   */
  Point topleft;
  double width, height;
};


DIA_DEFINE_OBJECT_CHANGE (DiaEllipseAspectObjectChange, dia_ellipse_aspect_object_change)


static void
dia_ellipse_aspect_object_change_free (DiaObjectChange *self)
{
}


static void
dia_ellipse_aspect_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaEllipseAspectObjectChange *change = DIA_ELLIPES_ASPECT_OBJECT_CHANGE (self);
  Ellipse *ellipse = (Ellipse*) obj;

  ellipse->aspect = change->new_type;
  ellipse_update_data (ellipse);
}


static void
dia_ellipse_aspect_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaEllipseAspectObjectChange *change = DIA_ELLIPES_ASPECT_OBJECT_CHANGE (self);
  Ellipse *ellipse = (Ellipse*) obj;

  ellipse->aspect = change->old_type;
  ellipse->element.corner = change->topleft;
  ellipse->element.width = change->width;
  ellipse->element.height = change->height;
  ellipse_update_data (ellipse);
}


static DiaObjectChange *
aspect_create_change (Ellipse *ellipse, AspectType aspect)
{
  DiaEllipseAspectObjectChange *change;

  change = dia_object_change_new (DIA_TYPE_ELLIPES_ASPECT_OBJECT_CHANGE);

  change->old_type = ellipse->aspect;
  change->new_type = aspect;
  change->topleft = ellipse->element.corner;
  change->width = ellipse->element.width;
  change->height = ellipse->element.height;

  return DIA_OBJECT_CHANGE (change);
}


static DiaObjectChange *
ellipse_set_aspect_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;

  change = aspect_create_change ((Ellipse *) obj, (AspectType) data);
  dia_object_change_apply (change, obj);

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

static gboolean
ellipse_transform(Ellipse *ellipse, const DiaMatrix *m)
{
  real a, sx, sy;

  g_return_val_if_fail(m != NULL, FALSE);

  if (!dia_matrix_get_angle_and_scales (m, &a, &sx, &sy)) {
    dia_log_message ("ellipse_transform() can't convert given matrix");
    return FALSE;
  } else {
    real width = ellipse->element.width * sx;
    real height = ellipse->element.height * sy;
    real angle = a*180/G_PI;
    Point c = { ellipse->element.corner.x + width/2.0, ellipse->element.corner.y + height/2.0 };

    /* rotation is invariant to the center */
    transform_point (&c, m);
    /* XXX: we have to bring angle in range [-45..45] which may swap width and height */
    ellipse->angle = angle;
    ellipse->element.width = width;
    ellipse->element.height = height;
    ellipse->element.corner.x = c.x - width / 2.0;
    ellipse->element.corner.y = c.y - height / 2.0;
 }

  ellipse_update_data(ellipse);
  return TRUE;
}
