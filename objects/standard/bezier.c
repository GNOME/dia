/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
 *
 * Bezier object  Copyright (C) 1999 Lars R. Clausen
 * Conversion to use BezierConn by James Henstridge.
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
#include "bezier_conn.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "diainteractiverenderer.h"
#include "attributes.h"
#include "diamenu.h"
#include "properties.h"
#include "create.h"

#define DEFAULT_WIDTH 0.15

typedef struct _Bezierline Bezierline;

/*!
 * \brief Standard - BezierLine : a bezier curve object
 * \ingroup StandardObjects
 * \extends _BezierConn
 */
struct _Bezierline {
  BezierConn bez;

  Color line_color;
  DiaLineStyle line_style;
  DiaLineJoin line_join;
  DiaLineCaps line_caps;
  double dashlength;
  double line_width;
  Arrow start_arrow, end_arrow;
  double absolute_start_gap, absolute_end_gap;
};


static DiaObjectChange* bezierline_move_handle     (Bezierline       *bezierline,
                                                    Handle           *handle,
                                                    Point            *to,
                                                    ConnectionPoint  *cp,
                                                    HandleMoveReason  reason,
                                                    ModifierKeys      modifiers);
static DiaObjectChange* bezierline_move            (Bezierline       *bezierline,
                                                    Point            *to);
static void bezierline_select(Bezierline *bezierline, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void bezierline_draw(Bezierline *bezierline, DiaRenderer *renderer);
static DiaObject *bezierline_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real bezierline_distance_from(Bezierline *bezierline, Point *point);
static void bezierline_update_data(Bezierline *bezierline);
static void bezierline_destroy(Bezierline *bezierline);
static DiaObject *bezierline_copy(Bezierline *bezierline);

static PropDescription *bezierline_describe_props(Bezierline *bezierline);
static void bezierline_get_props(Bezierline *bezierline, GPtrArray *props);
static void bezierline_set_props(Bezierline *bezierline, GPtrArray *props);

static void bezierline_save(Bezierline *bezierline, ObjectNode obj_node,
			    DiaContext *ctx);
static DiaObject *bezierline_load(ObjectNode obj_node, int version, DiaContext *ctx);
static DiaMenu *bezierline_get_object_menu(Bezierline *bezierline, Point *clickedpoint);
static gboolean bezierline_transform(Bezierline *bezierline, const DiaMatrix *m);

static void compute_gap_points(Bezierline *bezierline, Point *gap_points);
static real approx_bez_length(BezierConn *bez);
static void exchange_bez_gap_points(BezierConn * bez, Point* gap_points);

static ObjectTypeOps bezierline_type_ops =
{
  (CreateFunc)bezierline_create,   /* create */
  (LoadFunc)  bezierline_load,     /* load */
  (SaveFunc)  bezierline_save,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

static DiaObjectType bezierline_type =
{
  "Standard - BezierLine",   /* name */
  0,                         /* version */
  (const char **) "res:/org/gnome/Dia/objects/standard/bezierline.png",
  &bezierline_type_ops,      /* ops */
  NULL,                      /* pixmap_file */
  0                          /* default_user_data */
};

DiaObjectType *_bezierline_type = (DiaObjectType *) &bezierline_type;


static ObjectOps bezierline_ops = {
  (DestroyFunc)         bezierline_destroy,
  (DrawFunc)            bezierline_draw,
  (DistanceFunc)        bezierline_distance_from,
  (SelectFunc)          bezierline_select,
  (CopyFunc)            bezierline_copy,
  (MoveFunc)            bezierline_move,
  (MoveHandleFunc)      bezierline_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      bezierline_get_object_menu,
  (DescribePropsFunc)   bezierline_describe_props,
  (GetPropsFunc)        bezierline_get_props,
  (SetPropsFunc)        bezierline_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
  (TransformFunc)       bezierline_transform,
};

static PropNumData gap_range = { -G_MAXFLOAT, G_MAXFLOAT, 0.1};

static PropDescription bezierline_props[] = {
  BEZCONN_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_STD_LINE_JOIN_OPTIONAL,
  PROP_STD_LINE_CAPS_OPTIONAL,
  PROP_STD_START_ARROW,
  PROP_STD_END_ARROW,
  PROP_FRAME_BEGIN("gaps",0,N_("Line gaps")),
  { "absolute_start_gap", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
    N_("Absolute start gap"), NULL, &gap_range },
  { "absolute_end_gap", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
    N_("Absolute end gap"), NULL, &gap_range },
  PROP_FRAME_END("gaps",0),
  PROP_DESC_END
};

static PropDescription *
bezierline_describe_props(Bezierline *bezierline)
{
  if (bezierline_props[0].quark == 0)
    prop_desc_list_calculate_quarks(bezierline_props);
  return bezierline_props;
}

static PropOffset bezierline_offsets[] = {
  BEZCONN_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Bezierline, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Bezierline, line_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Bezierline, line_style), offsetof(Bezierline, dashlength) },
  { "line_join", PROP_TYPE_ENUM, offsetof(Bezierline, line_join) },
  { "line_caps", PROP_TYPE_ENUM, offsetof(Bezierline, line_caps) },
  { "start_arrow", PROP_TYPE_ARROW, offsetof(Bezierline, start_arrow) },
  { "end_arrow", PROP_TYPE_ARROW, offsetof(Bezierline, end_arrow) },
  PROP_OFFSET_FRAME_BEGIN("gaps"),
  { "absolute_start_gap", PROP_TYPE_REAL, offsetof(Bezierline, absolute_start_gap) },
  { "absolute_end_gap", PROP_TYPE_REAL, offsetof(Bezierline, absolute_end_gap) },
  PROP_OFFSET_FRAME_END("gaps"),
  { NULL, 0, 0 }
};

static void
bezierline_get_props(Bezierline *bezierline, GPtrArray *props)
{
  object_get_props_from_offsets(&bezierline->bez.object, bezierline_offsets,
				props);
}

static void
bezierline_set_props(Bezierline *bezierline, GPtrArray *props)
{
  object_set_props_from_offsets(&bezierline->bez.object, bezierline_offsets,
				props);
  bezierline_update_data(bezierline);
}

static real
bezierline_distance_from(Bezierline *bezierline, Point *point)
{
  BezierConn *bez = &bezierline->bez;
  if (connpoint_is_autogap(bez->object.handles[0]->connected_to) ||
      connpoint_is_autogap(bez->object.handles[3*(bez->bezier.num_points-1)]->connected_to) ||
      bezierline->absolute_start_gap || bezierline->absolute_end_gap) {
    Point gap_points[4];
    real distance;
    compute_gap_points(bezierline, gap_points);
    exchange_bez_gap_points(bez,gap_points);
    distance = bezierconn_distance_from(bez, point, bezierline->line_width);
    exchange_bez_gap_points(bez,gap_points);
    return distance;
  } else {
    return bezierconn_distance_from(bez, point, bezierline->line_width);
  }
}

static int
bezierline_closest_segment(Bezierline *bezierline, Point *point)
{
  BezierConn *bez = &bezierline->bez;
  return beziercommon_closest_segment(&bez->bezier, point, bezierline->line_width);
}

static void
bezierline_select(Bezierline *bezierline, Point *clicked_point,
		  DiaRenderer *interactive_renderer)
{
  bezierconn_update_data(&bezierline->bez);
}


static DiaObjectChange *
bezierline_move_handle (Bezierline       *bezierline,
                        Handle           *handle,
                        Point            *to,
                        ConnectionPoint  *cp,
                        HandleMoveReason  reason,
                        ModifierKeys      modifiers)
{
  g_return_val_if_fail (bezierline != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  if (reason == HANDLE_MOVE_CREATE || reason == HANDLE_MOVE_CREATE_FINAL) {
    /* During creation, change the control points */
    BezierConn *bez = &bezierline->bez;
    Point dist = bez->bezier.points[0].p1;

    point_sub(&dist, to);
    dist.y = 0;
    point_scale(&dist, 0.332);

    bezierconn_move_handle(bez, handle, to, cp, reason, modifiers);

    bez->bezier.points[1].p1 = bez->bezier.points[0].p1;
    point_sub(&bez->bezier.points[1].p1, &dist);
    bez->bezier.points[1].p2 = *to;
    point_add(&bez->bezier.points[1].p2, &dist);
  } else {
    bezierconn_move_handle(&bezierline->bez, handle, to, cp, reason, modifiers);
  }

  bezierline_update_data(bezierline);

  return NULL;
}


static DiaObjectChange *
bezierline_move (Bezierline *bezierline, Point *to)
{
  bezierconn_move (&bezierline->bez, to);
  bezierline_update_data (bezierline);

  return NULL;
}


static void
exchange_bez_gap_points(BezierConn * bez, Point* gap_points)
{
        Point tmp_points[4];
        tmp_points[0] = bez->bezier.points[0].p1;
        tmp_points[1] = bez->bezier.points[1].p1;
        tmp_points[2] = bez->bezier.points[bez->bezier.num_points-1].p2;
        tmp_points[3] = bez->bezier.points[bez->bezier.num_points-1].p3;
        bez->bezier.points[0].p1 = gap_points[0];
        bez->bezier.points[1].p1 = gap_points[1];
        bez->bezier.points[bez->bezier.num_points-1].p2 = gap_points[2];
        bez->bezier.points[bez->bezier.num_points-1].p3 = gap_points[3];
        gap_points[0] = tmp_points[0];
        gap_points[1] = tmp_points[1];
        gap_points[2] = tmp_points[2];
        gap_points[3] = tmp_points[3];
}
static real approx_bez_length(BezierConn *bez)
{
        /* Approximates the length of the bezier curve
         * by the length of the polyline joining its points */
        Point *current, *last, vec;
        real length = .0;
        int i;
        current = &bez->bezier.points[0].p1;
        for (i=1; i < bez->bezier.num_points ; i++){
            last = current;
            current = &bez->bezier.points[i].p3;
            point_copy(&vec,last);
            point_sub(&vec,current);
            length += point_len(&vec);
        }
        return length;
}

static void
compute_gap_points(Bezierline *bezierline, Point *gap_points)
{
        real bez_length;
        BezierConn *bez = &bezierline->bez;
        Point vec_start, vec_end;


        gap_points[0] = bez->bezier.points[0].p1;
        gap_points[1] = bez->bezier.points[1].p1;
        gap_points[2] = bez->bezier.points[bez->bezier.num_points-1].p2;
        gap_points[3] = bez->bezier.points[bez->bezier.num_points-1].p3;

        point_copy(&vec_start, &gap_points[1]);
        point_sub(&vec_start, &gap_points[0]);
        point_normalize(&vec_start); /* unit vector pointing from first point */
        point_copy(&vec_end, &gap_points[2]);
        point_sub(&vec_end, &gap_points[3]);
        point_normalize(&vec_end); /* unit vector pointing from last point */


        bez_length = approx_bez_length(bez) ;

        if (connpoint_is_autogap(bez->object.handles[0]->connected_to) &&
               (bez->object.handles[0])->connected_to != NULL &&
               (bez->object.handles[0])->connected_to->object != NULL ) {
            Point end;
            point_copy(&end, &gap_points[0]);
            point_add_scaled(&end, &vec_start, bez_length); /* far away on the same slope */
            end = calculate_object_edge(&gap_points[0], &end,
                            (bez->object.handles[0])->connected_to->object);
            point_sub(&end, &gap_points[0]); /* vector from old start to new start */
            /* move points */
            point_add(&gap_points[0], &end);
            point_add(&gap_points[1], &end);
        }

        if (connpoint_is_autogap(bez->object.handles[3*(bez->bezier.num_points-1)]->connected_to) &&
                (bez->object.handles[3*(bez->bezier.num_points-1)])->connected_to != NULL &&
                (bez->object.handles[3*(bez->bezier.num_points-1)])->connected_to->object != NULL) {
            Point end;
            point_copy(&end, &gap_points[3]);
            point_add_scaled(&end, &vec_end, bez_length); /* far away on the same slope */
            end = calculate_object_edge(&gap_points[3], &end,
                            (bez->object.handles[3*(bez->bezier.num_points-1)])->connected_to->object);
            point_sub(&end, &gap_points[3]); /* vector from old end to new end */
            /* move points */
            point_add(&gap_points[3], &end);
            point_add(&gap_points[2], &end);
        }


        /* adds the absolute start gap  according to the slope at the first point */
        point_add_scaled(&gap_points[0], &vec_start, bezierline->absolute_start_gap);
        point_add_scaled(&gap_points[1], &vec_start, bezierline->absolute_start_gap);

        /* adds the absolute end gap  according to the slope at the last point */
        point_add_scaled(&gap_points[2], &vec_end, bezierline->absolute_end_gap);
        point_add_scaled(&gap_points[3], &vec_end, bezierline->absolute_end_gap);
}

static void
bezierline_draw (Bezierline *bezierline, DiaRenderer *renderer)
{
  Point gap_points[4]; /* two first and two last bez points */
  BezierConn *bez = &bezierline->bez;

  dia_renderer_set_linewidth (renderer, bezierline->line_width);
  dia_renderer_set_linestyle (renderer, bezierline->line_style, bezierline->dashlength);
  dia_renderer_set_linejoin (renderer, bezierline->line_join);
  dia_renderer_set_linecaps (renderer, bezierline->line_caps);

  if (connpoint_is_autogap (bez->object.handles[0]->connected_to) ||
      connpoint_is_autogap (bez->object.handles[3*(bez->bezier.num_points-1)]->connected_to) ||
      bezierline->absolute_start_gap || bezierline->absolute_end_gap) {

    compute_gap_points (bezierline,gap_points);
    exchange_bez_gap_points (bez,gap_points);
    dia_renderer_draw_bezier_with_arrows (renderer,
                                          bez->bezier.points,
                                          bez->bezier.num_points,
                                          bezierline->line_width,
                                          &bezierline->line_color,
                                          &bezierline->start_arrow,
                                          &bezierline->end_arrow);
    exchange_bez_gap_points (bez,gap_points);
  } else {
    dia_renderer_draw_bezier_with_arrows (renderer,
                                          bez->bezier.points,
                                          bez->bezier.num_points,
                                          bezierline->line_width,
                                          &bezierline->line_color,
                                          &bezierline->start_arrow,
                                          &bezierline->end_arrow);
  }

#if 0
  dia_renderer_draw_bezier(renderer, bez->bezier.points, bez->bezier.num_points,
			     &bezierline->line_color);

  if (bezierline->start_arrow.type != ARROW_NONE) {
    arrow_draw(renderer, bezierline->start_arrow.type,
	       &bez->bezier.points[0].p1, &bez->bezier.points[1].p1,
	       bezierline->start_arrow.length, bezierline->start_arrow.width,
	       bezierline->line_width,
	       &bezierline->line_color, &color_white);
  }
  if (bezierline->end_arrow.type != ARROW_NONE) {
    arrow_draw(renderer, bezierline->end_arrow.type,
	       &bez->bezier.points[bez->bezier.num_points-1].p3,
	       &bez->bezier.points[bez->bezier.num_points-1].p2,
	       bezierline->end_arrow.length, bezierline->end_arrow.width,
	       bezierline->line_width,
	       &bezierline->line_color, &color_white);
  }
#endif

  /* Only display lines while selected.  Calling dia_object_is_selected()
   * every time may take a little long.  Some time can be saved by storing
   * whether the object is currently selected in bezierline_select, and
   * only checking while selected.  But we'll do that if needed.
   */
  if (DIA_IS_INTERACTIVE_RENDERER (renderer) &&
      dia_object_is_selected (&bezierline->bez.object)) {
    bezier_draw_control_lines (bezierline->bez.bezier.num_points,
                               bezierline->bez.bezier.points,
                               renderer);
  }
}

static DiaObject *
bezierline_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Bezierline *bezierline;
  BezierConn *bez;
  DiaObject *obj;
  Point defaultlen = { .3, .3 };

  bezierline = g_new0(Bezierline, 1);
  bezierline->bez.object.enclosing_box = g_new0 (DiaRectangle, 1);
  bez = &bezierline->bez;
  obj = &bez->object;

  obj->type = &bezierline_type;
  obj->ops = &bezierline_ops;

  if (user_data == NULL) {
    bezierconn_init(bez, 2);

    bez->bezier.points[0].p1 = *startpoint;
    bez->bezier.points[1].p1 = *startpoint;
    point_add(&bez->bezier.points[1].p1, &defaultlen);
    bez->bezier.points[1].p2 = bez->bezier.points[1].p1;
    point_add(&bez->bezier.points[1].p2, &defaultlen);
    bez->bezier.points[1].p3 = bez->bezier.points[1].p2;
    point_add(&bez->bezier.points[1].p3, &defaultlen);
  } else {
    BezierCreateData *bcd = (BezierCreateData*)user_data;

    bezierconn_init(bez, bcd->num_points);
    beziercommon_set_points (&bez->bezier, bcd->num_points, bcd->points);
  }

  bezierline->line_width =  attributes_get_default_linewidth();
  bezierline->line_color = attributes_get_foreground();
  attributes_get_default_line_style (&bezierline->line_style,
                                     &bezierline->dashlength);
  bezierline->line_join = DIA_LINE_JOIN_MITER;
  bezierline->line_caps = DIA_LINE_CAPS_BUTT;
  bezierline->start_arrow = attributes_get_default_start_arrow();
  bezierline->end_arrow = attributes_get_default_end_arrow();

  *handle1 = bez->object.handles[0];
  *handle2 = bez->object.handles[3];

  bezierline_update_data(bezierline);

  return &bezierline->bez.object;
}

static void
bezierline_destroy(Bezierline *bezierline)
{
  g_clear_pointer (&bezierline->bez.object.enclosing_box, g_free);
  bezierconn_destroy(&bezierline->bez);
}

static DiaObject *
bezierline_copy(Bezierline *bezierline)
{
  Bezierline *newbezierline;
  BezierConn *bez, *newbez;

  bez = &bezierline->bez;

  newbezierline = g_new0(Bezierline, 1);
  newbezierline->bez.object.enclosing_box = g_new0 (DiaRectangle, 1);
  newbez = &newbezierline->bez;

  bezierconn_copy(bez, newbez);

  newbezierline->line_color = bezierline->line_color;
  newbezierline->line_width = bezierline->line_width;
  newbezierline->line_style = bezierline->line_style;
  newbezierline->line_join = bezierline->line_join;
  newbezierline->line_caps = bezierline->line_caps;
  newbezierline->dashlength = bezierline->dashlength;
  newbezierline->start_arrow = bezierline->start_arrow;
  newbezierline->end_arrow = bezierline->end_arrow;
  newbezierline->absolute_start_gap = bezierline->absolute_start_gap;
  newbezierline->absolute_end_gap = bezierline->absolute_end_gap;

  return &newbezierline->bez.object;
}


static void
bezierline_update_data(Bezierline *bezierline)
{
  BezierConn *bez = &bezierline->bez;
  DiaObject *obj = &bez->object;
  PolyBBExtras *extra = &bez->extra_spacing;

  bezierconn_update_data(bez);

  extra->start_trans = extra->start_long =
  extra->middle_trans =
  extra->end_trans = extra->end_long = (bezierline->line_width / 2.0);

  obj->position = bez->bezier.points[0].p1;

  if (connpoint_is_autogap(bez->object.handles[0]->connected_to) ||
      connpoint_is_autogap(bez->object.handles[3*(bez->bezier.num_points-1)]->connected_to) ||
      bezierline->absolute_start_gap || bezierline->absolute_end_gap ||
      bezierline->start_arrow.type != ARROW_NONE || bezierline->end_arrow.type != ARROW_NONE) {
    Point gap_points[4];
    DiaRectangle bbox_union = {bez->bezier.points[0].p1.x, bez->bezier.points[0].p1.y,
			    bez->bezier.points[0].p1.x, bez->bezier.points[0].p1.y};
    compute_gap_points(bezierline, gap_points);
    exchange_bez_gap_points(bez,gap_points);
    /* further modifying the points data, accounts for corrcet arrow and bezier bounding box */
    if (bezierline->start_arrow.type != ARROW_NONE) {
      DiaRectangle bbox;
      Point move_arrow, move_line;
      Point to = bez->bezier.points[0].p1, from = bez->bezier.points[1].p1;

      calculate_arrow_point(&bezierline->start_arrow, &to, &from, &move_arrow, &move_line, bezierline->line_width);
      point_sub(&to, &move_arrow);
      point_sub(&bez->bezier.points[0].p1, &move_line);
      arrow_bbox (&bezierline->start_arrow, bezierline->line_width, &to, &from, &bbox);
      rectangle_union (&bbox_union, &bbox);
    }
    if (bezierline->end_arrow.type != ARROW_NONE) {
      DiaRectangle bbox;
      Point move_arrow, move_line;
      int num_points = bez->bezier.num_points;
      Point to = bez->bezier.points[num_points-1].p3, from = bez->bezier.points[num_points-1].p2;

      calculate_arrow_point(&bezierline->end_arrow, &to, &from, &move_arrow, &move_line, bezierline->line_width);
      point_sub(&to, &move_arrow);
      point_sub(&bez->bezier.points[num_points-1].p3, &move_line);
      arrow_bbox (&bezierline->end_arrow, bezierline->line_width, &to, &from, &bbox);
      rectangle_union (&bbox_union, &bbox);
    }
    bezierconn_update_boundingbox(bez);
    rectangle_union (&obj->bounding_box, &bbox_union);
    exchange_bez_gap_points(bez,gap_points);
  } else {
    bezierconn_update_boundingbox(bez);
  }
    /* add control points to the bounding box, needed to make them visible when showing all
      * and to remove traces from them */
  {
    int i, num_points = bez->bezier.num_points;
    g_assert (obj->enclosing_box != NULL);
    *obj->enclosing_box = obj->bounding_box;
    /* starting with the second point, the first one is MOVE_TO */
    for (i = 1; i < num_points; ++i) {
      if (bez->bezier.points[i].type != BEZ_CURVE_TO)
        continue;
      rectangle_add_point(obj->enclosing_box, &bez->bezier.points[i].p1);
      rectangle_add_point(obj->enclosing_box, &bez->bezier.points[i].p2);
    }
  }
}

static void
bezierline_save(Bezierline *bezierline, ObjectNode obj_node,
	        DiaContext *ctx)
{
  if (connpoint_is_autogap(bezierline->bez.object.handles[0]->connected_to) ||
      connpoint_is_autogap(bezierline->bez.object.handles[3*(bezierline->bez.bezier.num_points-1)]->connected_to) ||
      bezierline->absolute_start_gap || bezierline->absolute_end_gap) {
    Point gap_points[4];
    compute_gap_points(bezierline, gap_points);
    exchange_bez_gap_points(&bezierline->bez,gap_points);
    bezierconn_update_boundingbox(&bezierline->bez);
    exchange_bez_gap_points(&bezierline->bez,gap_points);
  }
  bezierconn_save(&bezierline->bez, obj_node, ctx);

  if (!color_equals(&bezierline->line_color, &color_black))
    data_add_color(new_attribute(obj_node, "line_color"),
		   &bezierline->line_color, ctx);

  if (bezierline->line_width != 0.1)
    data_add_real(new_attribute(obj_node, PROP_STDNAME_LINE_WIDTH),
		  bezierline->line_width, ctx);

  if (bezierline->line_style != DIA_LINE_STYLE_SOLID) {
    data_add_enum (new_attribute (obj_node, "line_style"),
                   bezierline->line_style,
                   ctx);
  }

  if (bezierline->line_style != DIA_LINE_STYLE_SOLID &&
      bezierline->dashlength != DEFAULT_LINESTYLE_DASHLEN) {
    data_add_real (new_attribute (obj_node, "dashlength"),
                   bezierline->dashlength,
                   ctx);
  }

  if (bezierline->line_join != DIA_LINE_JOIN_MITER) {
    data_add_enum (new_attribute (obj_node, "line_join"),
                   bezierline->line_join,
                   ctx);
  }

  if (bezierline->line_caps != DIA_LINE_CAPS_BUTT) {
    data_add_enum (new_attribute (obj_node, "line_caps"),
                   bezierline->line_caps,
                   ctx);
  }

  if (bezierline->start_arrow.type != ARROW_NONE) {
    dia_arrow_save (&bezierline->start_arrow,
                    obj_node,
                    "start_arrow",
                    "start_arrow_length",
                    "start_arrow_width",
                    ctx);
  }

  if (bezierline->end_arrow.type != ARROW_NONE) {
    dia_arrow_save (&bezierline->end_arrow,
                    obj_node,
                    "end_arrow",
                    "end_arrow_length",
                    "end_arrow_width",
                    ctx);
  }

  if (bezierline->absolute_start_gap)
    data_add_real(new_attribute(obj_node, "absolute_start_gap"),
                 bezierline->absolute_start_gap, ctx);
  if (bezierline->absolute_end_gap)
    data_add_real(new_attribute(obj_node, "absolute_end_gap"),
                 bezierline->absolute_end_gap, ctx);
}

static DiaObject *
bezierline_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  Bezierline *bezierline;
  BezierConn *bez;
  DiaObject *obj;
  AttributeNode attr;

  bezierline = g_new0(Bezierline, 1);
  bezierline->bez.object.enclosing_box = g_new0 (DiaRectangle, 1);

  bez = &bezierline->bez;
  obj = &bez->object;

  obj->type = &bezierline_type;
  obj->ops = &bezierline_ops;

  bezierconn_load(bez, obj_node, ctx);

  bezierline->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &bezierline->line_color, ctx);

  bezierline->line_width = 0.1;
  attr = object_find_attribute(obj_node, PROP_STDNAME_LINE_WIDTH);
  if (attr != NULL)
    bezierline->line_width = data_real(attribute_first_data(attr), ctx);

  bezierline->line_style = DIA_LINE_STYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    bezierline->line_style = data_enum(attribute_first_data(attr), ctx);

  bezierline->line_join = DIA_LINE_JOIN_MITER;
  attr = object_find_attribute (obj_node, "line_join");
  if (attr != NULL) {
    bezierline->line_join = data_enum (attribute_first_data (attr), ctx);
  }

  bezierline->line_caps = DIA_LINE_CAPS_BUTT;
  attr = object_find_attribute (obj_node, "line_caps");
  if (attr != NULL) {
    bezierline->line_caps = data_enum (attribute_first_data (attr), ctx);
  }

  bezierline->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    bezierline->dashlength = data_real(attribute_first_data(attr), ctx);

  dia_arrow_load (&bezierline->start_arrow,
                  obj_node,
                  "start_arrow",
                  "start_arrow_length",
                  "start_arrow_width",
                  ctx);

  dia_arrow_load (&bezierline->end_arrow,
                  obj_node,
                  "end_arrow",
                  "end_arrow_length",
                  "end_arrow_width",
                  ctx);

  bezierline->absolute_start_gap = 0.0;
  attr = object_find_attribute(obj_node, "absolute_start_gap");
  if (attr != NULL)
    bezierline->absolute_start_gap =  data_real(attribute_first_data(attr), ctx);
  bezierline->absolute_end_gap = 0.0;
  attr = object_find_attribute(obj_node, "absolute_end_gap");
  if (attr != NULL)
    bezierline->absolute_end_gap =  data_real(attribute_first_data(attr), ctx);

  /* if "screws up the bounding box if auto_gap" it must be fixed there
   * not by copying some meaningless bounding_box before this function call!
   * But the real fix is in connectionpoint.c(connpoint_is_autogap)
   */
  bezierline_update_data(bezierline);

  return &bezierline->bez.object;
}


static DiaObjectChange *
bezierline_add_segment_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Bezierline *bezierline = (Bezierline*) obj;
  int segment;
  DiaObjectChange *change;

  segment = bezierline_closest_segment (bezierline, clicked);
  change = bezierconn_add_segment (&bezierline->bez, segment, clicked);
  bezierline_update_data (bezierline);

  return change;
}


static DiaObjectChange *
bezierline_delete_segment_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  int seg_nr;
  Bezierline *bezierline = (Bezierline*) obj;
  DiaObjectChange *change;

  seg_nr = beziercommon_closest_segment (&bezierline->bez.bezier, clicked, bezierline->line_width);

  change = bezierconn_remove_segment (&bezierline->bez, seg_nr+1);
  bezierline_update_data (bezierline);

  return change;
}


static DiaObjectChange *
bezierline_set_corner_type_callback (DiaObject *obj,
                                     Point     *clicked,
                                     gpointer   data)
{
  Handle *closest;
  Bezierline *bezierline = (Bezierline*) obj;
  DiaObjectChange *change;

  closest = bezierconn_closest_major_handle (&bezierline->bez, clicked);
  change = bezierconn_set_corner_type (&bezierline->bez,
                                       closest,
                                       GPOINTER_TO_INT (data));

  bezierline_update_data (bezierline);

  return change;
}


static DiaMenuItem bezierline_menu_items[] = {
  { N_("Add Segment"), bezierline_add_segment_callback, NULL, 1 },
  { N_("Delete Segment"), bezierline_delete_segment_callback, NULL, 1 },
  { NULL, NULL, NULL, 1 },
  { N_("Symmetric control"), bezierline_set_corner_type_callback,
    GINT_TO_POINTER(BEZ_CORNER_SYMMETRIC), 1 },
  { N_("Smooth control"), bezierline_set_corner_type_callback,
    GINT_TO_POINTER(BEZ_CORNER_SMOOTH), 1 },
  { N_("Cusp control"), bezierline_set_corner_type_callback,
    GINT_TO_POINTER(BEZ_CORNER_CUSP), 1 }
};

static DiaMenu bezierline_menu = {
  "Bezierline",
  sizeof(bezierline_menu_items)/sizeof(DiaMenuItem),
  bezierline_menu_items,
  NULL
};

static DiaMenu *
bezierline_get_object_menu(Bezierline *bezierline, Point *clickedpoint)
{
  Handle *closest;
  gboolean closest_is_endpoint;
  BezCornerType ctype = 42; /* nothing */
  gint i;

  closest = bezierconn_closest_major_handle(&bezierline->bez, clickedpoint);
  if (closest->id == HANDLE_MOVE_STARTPOINT ||
      closest->id == HANDLE_MOVE_ENDPOINT)
    closest_is_endpoint = TRUE;
  else
    closest_is_endpoint = FALSE;

  for (i = 0; i < bezierline->bez.bezier.num_points; i++)
    if (bezierline->bez.object.handles[3*i] == closest) {
      ctype = bezierline->bez.bezier.corner_types[i];
      break;
    }

  /* Set entries sensitive/selected etc here */
  bezierline_menu_items[0].active = 1;
  bezierline_menu_items[1].active = bezierline->bez.bezier.num_points > 2;
  bezierline_menu_items[3].active = !closest_is_endpoint &&
    (ctype != BEZ_CORNER_SYMMETRIC);
  bezierline_menu_items[4].active = !closest_is_endpoint &&
    (ctype != BEZ_CORNER_SMOOTH);
  bezierline_menu_items[5].active = !closest_is_endpoint &&
    (ctype != BEZ_CORNER_CUSP);
  return &bezierline_menu;
}

static gboolean
bezierline_transform(Bezierline *bezierline, const DiaMatrix *m)
{
  int i;

  g_return_val_if_fail (m != NULL, FALSE);

  for (i = 0; i < bezierline->bez.bezier.num_points; i++)
    transform_bezpoint (&bezierline->bez.bezier.points[i], m);

  bezierline_update_data(bezierline);
  return TRUE;
}
