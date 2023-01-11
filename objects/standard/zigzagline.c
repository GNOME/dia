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
#include "orth_conn.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"
#include "autoroute.h"
#include "create.h"

#define DEFAULT_WIDTH 0.15

#define HANDLE_MIDDLE HANDLE_CUSTOM1

/*!
 * \brief Standard - ZigZagLine : orthogonal connection object
 * \extends _OrthConn
 * \ingroup StandardObjects
 */
typedef struct _Zigzagline {
  OrthConn orth;

  Color line_color;
  DiaLineStyle line_style;
  DiaLineJoin line_join;
  DiaLineCaps line_caps;
  double dashlength;
  double line_width;
  double corner_radius;
  Arrow start_arrow, end_arrow;
} Zigzagline;


static DiaObjectChange *zigzagline_move_handle   (Zigzagline       *zigzagline,
                                                  Handle           *handle,
                                                  Point            *to,
                                                  ConnectionPoint  *cp,
                                                  HandleMoveReason  reason,
                                                  ModifierKeys      modifiers);
static DiaObjectChange *zigzagline_move          (Zigzagline       *zigzagline,
                                                  Point            *to);
static void zigzagline_select(Zigzagline *zigzagline, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void zigzagline_draw(Zigzagline *zigzagline, DiaRenderer *renderer);
static DiaObject *zigzagline_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real zigzagline_distance_from(Zigzagline *zigzagline, Point *point);
static void zigzagline_update_data(Zigzagline *zigzagline);
static void zigzagline_destroy(Zigzagline *zigzagline);
static DiaObject *zigzagline_copy(Zigzagline *zigzagline);
static DiaMenu *zigzagline_get_object_menu(Zigzagline *zigzagline,
					   Point *clickedpoint);

static void zigzagline_set_props(Zigzagline *zigzagline, GPtrArray *props);

static void zigzagline_save(Zigzagline *zigzagline, ObjectNode obj_node,
			    DiaContext *ctx);
static DiaObject *zigzagline_load(ObjectNode obj_node, int version, DiaContext *ctx);

static ObjectTypeOps zigzagline_type_ops =
{
  (CreateFunc)zigzagline_create,   /* create */
  (LoadFunc)  zigzagline_load,     /* load */
  (SaveFunc)  zigzagline_save,      /* save */
  (GetDefaultsFunc)   NULL,/*zigzagline_get_defaults*/
  (ApplyDefaultsFunc) NULL /*zigzagline_apply_defaults*/
};

static PropNumData zigzagline_corner_radius_data = { 0.0, 10.0, 0.1 };

static PropDescription zigzagline_props[] = {
  ORTHCONN_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_STD_LINE_JOIN_OPTIONAL,
  PROP_STD_LINE_CAPS_OPTIONAL,
  PROP_STD_START_ARROW,
  PROP_STD_END_ARROW,
  { "corner_radius", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Corner radius"), NULL, &zigzagline_corner_radius_data },
  PROP_DESC_END
};

static PropOffset zigzagline_offsets[] = {
  ORTHCONN_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Zigzagline, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Zigzagline, line_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Zigzagline, line_style), offsetof(Zigzagline, dashlength) },
  { "line_join", PROP_TYPE_ENUM, offsetof(Zigzagline, line_join) },
  { "line_caps", PROP_TYPE_ENUM, offsetof(Zigzagline, line_caps) },
  { "start_arrow", PROP_TYPE_ARROW, offsetof(Zigzagline, start_arrow) },
  { "end_arrow", PROP_TYPE_ARROW, offsetof(Zigzagline, end_arrow) },
  { "corner_radius", PROP_TYPE_REAL, offsetof(Zigzagline, corner_radius) },
  { NULL, 0, 0 }
};

static DiaObjectType zigzagline_type =
{
  "Standard - ZigZagLine",   /* name */
  /* Version 0 had no autorouting and so shouldn't have it set by default. */
  1,                         /* version */
  (const char **) "res:/org/gnome/Dia/objects/standard/zigzagline.png",
  &zigzagline_type_ops,      /* ops */
  NULL,
  0,
  zigzagline_props,
  zigzagline_offsets
};

DiaObjectType *_zigzagline_type = (DiaObjectType *) &zigzagline_type;


static ObjectOps zigzagline_ops = {
  (DestroyFunc)         zigzagline_destroy,
  (DrawFunc)            zigzagline_draw,
  (DistanceFunc)        zigzagline_distance_from,
  (SelectFunc)          zigzagline_select,
  (CopyFunc)            zigzagline_copy,
  (MoveFunc)            zigzagline_move,
  (MoveHandleFunc)      zigzagline_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      zigzagline_get_object_menu,
  (DescribePropsFunc)   object_describe_props,
  (GetPropsFunc)        object_get_props,
  (SetPropsFunc)        zigzagline_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static void
zigzagline_set_props(Zigzagline *zigzagline, GPtrArray *props)
{
  object_set_props_from_offsets(&zigzagline->orth.object, zigzagline_offsets,
				props);
  zigzagline_update_data(zigzagline);
}

static real
zigzagline_distance_from(Zigzagline *zigzagline, Point *point)
{
  OrthConn *orth = &zigzagline->orth;
  return orthconn_distance_from(orth, point, zigzagline->line_width);
}

static void
zigzagline_select(Zigzagline *zigzagline, Point *clicked_point,
		  DiaRenderer *interactive_renderer)
{
  orthconn_update_data(&zigzagline->orth);
}


static DiaObjectChange *
zigzagline_move_handle (Zigzagline       *zigzagline,
                        Handle           *handle,
                        Point            *to,
                        ConnectionPoint  *cp,
                        HandleMoveReason  reason,
                        ModifierKeys      modifiers)
{
  DiaObjectChange *change;

  g_return_val_if_fail (zigzagline != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  change = orthconn_move_handle ((OrthConn*) zigzagline,
                                 handle,
                                 to,
                                 cp,
                                 reason,
                                 modifiers);

  zigzagline_update_data (zigzagline);

  return change;
}


static DiaObjectChange *
zigzagline_move (Zigzagline *zigzagline, Point *to)
{
  orthconn_move (&zigzagline->orth, to);
  zigzagline_update_data (zigzagline);

  return NULL;
}


static void
zigzagline_draw (Zigzagline *zigzagline, DiaRenderer *renderer)
{
  OrthConn *orth = &zigzagline->orth;
  Point *points;
  int n;

  points = &orth->points[0];
  n = orth->numpoints;

  dia_renderer_set_linewidth (renderer, zigzagline->line_width);
  dia_renderer_set_linestyle (renderer, zigzagline->line_style, zigzagline->dashlength);
  dia_renderer_set_linejoin (renderer, zigzagline->line_join);
  dia_renderer_set_linecaps (renderer, zigzagline->line_caps);

  dia_renderer_draw_rounded_polyline_with_arrows (renderer,
                                                  points,
                                                  n,
                                                  zigzagline->line_width,
                                                  &zigzagline->line_color,
                                                  &zigzagline->start_arrow,
                                                  &zigzagline->end_arrow,
                                                  zigzagline->corner_radius);
}

static DiaObject *
zigzagline_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Zigzagline *zigzagline;
  OrthConn *orth;
  DiaObject *obj;
  Point dummy = {0, 0};

  /*zigzagline_init_defaults();*/
  zigzagline = g_new0 (Zigzagline, 1);
  orth = &zigzagline->orth;
  obj = &orth->object;

  obj->type = &zigzagline_type;
  obj->ops = &zigzagline_ops;

  if (startpoint)
    orthconn_init(orth, startpoint);
  else
    orthconn_init(orth, &dummy);

  if (user_data) {
    MultipointCreateData *pcd = (MultipointCreateData *)user_data;
    orthconn_set_points (orth, pcd->num_points, pcd->points);
  }

  zigzagline->line_width =  attributes_get_default_linewidth();
  zigzagline->line_color = attributes_get_foreground();
  attributes_get_default_line_style (&zigzagline->line_style,
                                     &zigzagline->dashlength);
  zigzagline->line_join = DIA_LINE_JOIN_MITER;
  zigzagline->line_caps = DIA_LINE_CAPS_BUTT;
  zigzagline->start_arrow = attributes_get_default_start_arrow();
  zigzagline->end_arrow = attributes_get_default_end_arrow();
  zigzagline->corner_radius = 0.0;

  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];

  zigzagline_update_data(zigzagline);

  return &zigzagline->orth.object;
}

static void
zigzagline_destroy(Zigzagline *zigzagline)
{
  orthconn_destroy(&zigzagline->orth);
}

static DiaObject *
zigzagline_copy(Zigzagline *zigzagline)
{
  Zigzagline *newzigzagline;
  OrthConn *orth, *neworth;

  orth = &zigzagline->orth;

  newzigzagline = g_new0 (Zigzagline, 1);
  neworth = &newzigzagline->orth;

  orthconn_copy(orth, neworth);

  newzigzagline->line_color = zigzagline->line_color;
  newzigzagline->line_width = zigzagline->line_width;
  newzigzagline->line_style = zigzagline->line_style;
  newzigzagline->line_join = zigzagline->line_join;
  newzigzagline->line_caps = zigzagline->line_caps;
  newzigzagline->dashlength = zigzagline->dashlength;
  newzigzagline->start_arrow = zigzagline->start_arrow;
  newzigzagline->end_arrow = zigzagline->end_arrow;
  newzigzagline->corner_radius = zigzagline->corner_radius;

  zigzagline_update_data(newzigzagline);

  return &newzigzagline->orth.object;
}

static void
zigzagline_update_data(Zigzagline *zigzagline)
{
  OrthConn *orth = &zigzagline->orth;
  DiaObject *obj = &orth->object;
  PolyBBExtras *extra = &orth->extra_spacing;

  orthconn_update_data(&zigzagline->orth);

  extra->start_long =
    extra->end_long =
    extra->middle_trans =
    extra->start_trans =
    extra->end_trans = (zigzagline->line_width / 2.0);

  orthconn_update_boundingbox(orth);

  if (zigzagline->start_arrow.type != ARROW_NONE) {
    DiaRectangle bbox;
    Point move_arrow, move_line;
    Point to = orth->points[0];
    Point from = orth->points[1];
    calculate_arrow_point(&zigzagline->start_arrow, &to, &from,
                          &move_arrow, &move_line, zigzagline->line_width);
    /* move them */
    point_sub(&to, &move_arrow);
    point_sub(&from, &move_line);

    arrow_bbox (&zigzagline->start_arrow, zigzagline->line_width, &to, &from, &bbox);
    rectangle_union (&obj->bounding_box, &bbox);
  }
  if (zigzagline->end_arrow.type != ARROW_NONE) {
    DiaRectangle bbox;
    Point move_arrow, move_line;
    int n = orth->numpoints;
    Point to = orth->points[n-1];
    Point from = orth->points[n-2];
    calculate_arrow_point(&zigzagline->end_arrow, &to, &from,
                          &move_arrow, &move_line, zigzagline->line_width);
    /* move them */
    point_sub(&to, &move_arrow);
    point_sub(&from, &move_line);

    arrow_bbox (&zigzagline->end_arrow, zigzagline->line_width, &to, &from, &bbox);
    rectangle_union (&obj->bounding_box, &bbox);
  }
}


static DiaObjectChange *
zigzagline_add_segment_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;

  change = orthconn_add_segment ((OrthConn *) obj, clicked);
  zigzagline_update_data ((Zigzagline *) obj);

  return change;
}


static DiaObjectChange *
zigzagline_delete_segment_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;

  change = orthconn_delete_segment ((OrthConn *) obj, clicked);
  zigzagline_update_data ((Zigzagline *) obj);

  return change;
}


/*!
 * \brief Upgrade the _Zigzagline to a _Bezierline
 *
 * Accessible through the object's context menu this function substitutes
 * the _Zigzagline with a _Bezierline. The function creates a favorable
 * representation of the original _OrthConn data of the given object.
 *
 * @param obj  explicit this pointer
 * @param clicked  last clicked point on the canvas or NULL
 * @param data a pointer to extra data, unused here
 * @return  undo/redo information as _ObjectChange
 *
 * \memberof _Zigzagline
 */
static DiaObjectChange *
_convert_to_bezierline_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Zigzagline *zigzagline = (Zigzagline *)obj;
  OrthConn *orth = &zigzagline->orth;
  BezPoint *bp;
  int i, j, num_points;
  DiaObject *bezier;

  num_points = (orth->numpoints + 1) / 2;
  bp = g_new(BezPoint, num_points);
  bp[0].type = BEZ_MOVE_TO;
  bp[0].p1 = orth->points[0];

  for (i = 1, j = 1; i < num_points && j < orth->numpoints; ++i) {
    bp[i].type = BEZ_CURVE_TO;
    bp[i].p1 = orth->points[j++];
    if (j == orth->numpoints - 1)
      bp[i].p2 = orth->points[j-1]; /* use the previous point again */
    else
      bp[i].p2 = orth->points[j++];
    if (j + 2 < orth->numpoints) {
      /* if we have more than two points left, use the middle of the segment */
      Point p = { (orth->points[j-1].x + orth->points[j].x) / 2.0,
		  (orth->points[j-1].y + orth->points[j].y) / 2.0 };
      bp[i].p3 = p;
    } else if (j + 2 == orth->numpoints) {
      /* if we one extra point left, use the middle of two segments */
      Point p1 = { (orth->points[j-2].x + orth->points[j-1].x) / 2.0,
		   (orth->points[j-2].y + orth->points[j-1].y) / 2.0 };
      Point p2 = { (orth->points[j-1].x + orth->points[j].x) / 2.0,
		   (orth->points[j-1].y + orth->points[j].y) / 2.0 };
      Point p =  { (p1.x + p2.x) / 2.0, (p1.y + p2.y) / 2.0 };
      bp[i].p3 = p;
      /* also adapt the previous control point */
      bp[i].p2 = p1;
      /* and do the final bezpoint */
      bp[i+1].type = BEZ_CURVE_TO;
      bp[i+1].p1 = p2;
      bp[i+1].p2 = orth->points[j];
      bp[i+1].p3 = orth->points[j+1];
      break;
    } else {
      bp[i].p3 = orth->points[j++];
    }
  }
  bezier = create_standard_bezierline(num_points, bp, &zigzagline->end_arrow, &zigzagline->start_arrow);
  g_clear_pointer (&bp, g_free);

  return object_substitute (obj, bezier);
}

static DiaMenuItem object_menu_items[] = {
  { N_("Add segment"), zigzagline_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), zigzagline_delete_segment_callback, NULL, 1 },
  { N_("Upgrade to Bezierline"), _convert_to_bezierline_callback, NULL, 1 },
  ORTHCONN_COMMON_MENUS,
};

static DiaMenu object_menu = {
  "Zigzagline",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
zigzagline_get_object_menu(Zigzagline *zigzagline, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &zigzagline->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[1].active = orthconn_can_delete_segment(orth, clickedpoint);
  object_menu_items[2].active = TRUE;
  orthconn_update_object_menu(orth, clickedpoint, &object_menu_items[3]);

  return &object_menu;
}


static void
zigzagline_save(Zigzagline *zigzagline, ObjectNode obj_node,
		DiaContext *ctx)
{
  orthconn_save(&zigzagline->orth, obj_node, ctx);

  if (!color_equals(&zigzagline->line_color, &color_black))
    data_add_color(new_attribute(obj_node, "line_color"),
		   &zigzagline->line_color, ctx);

  if (zigzagline->line_width != 0.1)
    data_add_real(new_attribute(obj_node, PROP_STDNAME_LINE_WIDTH),
		  zigzagline->line_width, ctx);

  if (zigzagline->line_style != DIA_LINE_STYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  zigzagline->line_style, ctx);

  if (zigzagline->line_join != DIA_LINE_JOIN_MITER) {
    data_add_enum (new_attribute (obj_node, "line_join"),
                   zigzagline->line_join,
                   ctx);
  }

  if (zigzagline->line_caps != DIA_LINE_CAPS_BUTT) {
    data_add_enum (new_attribute (obj_node, "line_caps"),
                   zigzagline->line_caps,
                   ctx);
  }

  if (zigzagline->start_arrow.type != ARROW_NONE) {
    dia_arrow_save (&zigzagline->start_arrow,
                    obj_node,
                    "start_arrow",
                    "start_arrow_length",
                    "start_arrow_width",
                    ctx);
  }

  if (zigzagline->end_arrow.type != ARROW_NONE) {
    dia_arrow_save (&zigzagline->end_arrow,
                    obj_node,
                    "end_arrow",
                    "end_arrow_length",
                    "end_arrow_width",
                    ctx);
  }

  if (zigzagline->line_style != DIA_LINE_STYLE_SOLID &&
      zigzagline->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
                  zigzagline->dashlength, ctx);

  if (zigzagline->corner_radius > 0.0)
    data_add_real(new_attribute(obj_node, "corner_radius"),
                  zigzagline->corner_radius, ctx);
}

static DiaObject *
zigzagline_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  Zigzagline *zigzagline;
  OrthConn *orth;
  DiaObject *obj;
  AttributeNode attr;

  zigzagline = g_new0 (Zigzagline, 1);

  orth = &zigzagline->orth;
  obj = &orth->object;

  obj->type = &zigzagline_type;
  obj->ops = &zigzagline_ops;

  orthconn_load(orth, obj_node, ctx);

  zigzagline->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &zigzagline->line_color, ctx);

  zigzagline->line_width = 0.1;
  attr = object_find_attribute(obj_node, PROP_STDNAME_LINE_WIDTH);
  if (attr != NULL)
    zigzagline->line_width = data_real(attribute_first_data(attr), ctx);

  zigzagline->line_style = DIA_LINE_STYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    zigzagline->line_style = data_enum(attribute_first_data(attr), ctx);

  zigzagline->line_join = DIA_LINE_JOIN_MITER;
  attr = object_find_attribute (obj_node, "line_join");
  if (attr != NULL) {
    zigzagline->line_join = data_enum (attribute_first_data (attr), ctx);
  }

  zigzagline->line_caps = DIA_LINE_CAPS_BUTT;
  attr = object_find_attribute (obj_node, "line_caps");
  if (attr != NULL) {
    zigzagline->line_caps = data_enum (attribute_first_data (attr), ctx);
  }

  dia_arrow_load (&zigzagline->start_arrow,
                  obj_node,
                  "start_arrow",
                  "start_arrow_length",
                  "start_arrow_width",
                  ctx);

  dia_arrow_load (&zigzagline->end_arrow,
                  obj_node,
                  "end_arrow",
                  "end_arrow_length",
                  "end_arrow_width",
                  ctx);

  zigzagline->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    zigzagline->dashlength = data_real(attribute_first_data(attr), ctx);

  zigzagline->corner_radius = 0.0;
  attr = object_find_attribute(obj_node, "corner_radius");
  if (attr != NULL)
    zigzagline->corner_radius = data_real(attribute_first_data(attr), ctx);

  zigzagline_update_data(zigzagline);

  return &zigzagline->orth.object;
}
