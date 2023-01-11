/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * standard-path.c -- path based object drawing
 * Copyright (C) 2012, Hans Breuer <Hans@Breuer.Org>
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

/*!
 * \file standard-path.c - one to rule them all ;)
 *
 * One of Dia's early deficiencies is the lack of full path support. The basics
 * are there, e.g. _BezierShape and _BezierConn, but both are bound to have only
 * a single BEZ_MOVE_TO because the _DiaGdkRenderer does not support RENDER_HOLES
 * and even worse the original file format of bezierconn_save() only works
 * with a single move-to, too.
 * Instead of breaking forward compatibility this is implementing a new bezier
 * based object, which can render holes, even if the _DiaRenderer can not. It is
 * using a newer file format representation through BezPointarrayProperty.
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include "object.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"
#include "propinternals.h"
#include "boundingbox.h"
#include "standard-path.h"
#include "create.h"
#include "bezier-common.h"
#include "pattern.h"
#include "dia-object-change-list.h"


#define NUM_HANDLES 8

typedef enum {
  PDO_STROKE = (1<<0),
  PDO_FILL   = (1<<1),
  PDO_BOTH   = (PDO_STROKE|PDO_FILL)
} PathDrawingOps;

typedef struct _StdPath StdPath;

/*!
 * \brief 'Standard - Path' - bezier points for stroke or fill
 *
 * The _StdPath object can be used to represent many of the \ref StandardObjects.
 * It is not intended to replace all the basic objects, but rather to extend
 * facilities of them by offering conversion from and to these objects.
 *
 * Rather than duplicating all the specific editing of the standard objects this
 * object supports some more generic approach.
 *
 * \extends _DiaObject
 * \ingroup StandardObjects
 */
struct _StdPath {
  DiaObject object; /**< inheritance */

  int num_points; /**< >= 2 */
  BezPoint *points; /**< point data */

  int stroke_or_fill;

  Color line_color;
  double line_width;
  DiaLineStyle line_style;
  double dashlength;
  DiaLineJoin line_join;
  DiaLineCaps line_caps;
  Color fill_color;

  /*! mirroring (stdpath->stroke_or_fill & PDO_FILL) */
  gboolean show_background;

  /* mostly useful for debugging transformations */
  gboolean show_control_lines;

  /*! optional gradient */
  DiaPattern *pattern;

  Handle handles[NUM_HANDLES];

  HandleMoveReason move_reason;
};

static DiaObject *stdpath_create (Point *startpoint,
				  void *user_data,
				  Handle **handle1,
				  Handle **handle2);
static DiaObject *
stdpath_load(ObjectNode obj_node, int version,DiaContext *ctx);

static ObjectTypeOps stdpath_type_ops =
{
  (CreateFunc)stdpath_create,
  (LoadFunc)  stdpath_load,
  (SaveFunc)  object_save_using_properties, /* stdpath_save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

static PropEnumData _drawing_enum_data [] = {
  { N_("Stroke"), PDO_STROKE },
  { N_("Fill"), PDO_FILL },
  { N_("Fill & Stroke"), PDO_BOTH },
  { NULL, 0 }
};
static PropDescription stdpath_props[] = {
  OBJECT_COMMON_PROPERTIES,
  { "bez_points", PROP_TYPE_BEZPOINTARRAY, 0, N_("Bezier points"), NULL},
  { "stroke_or_fill", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE, N_("Drawing"), NULL, _drawing_enum_data },
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_LINE_STYLE_OPTIONAL,
  PROP_STD_LINE_JOIN_OPTIONAL,
  PROP_STD_LINE_CAPS_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,
  /* just to simplify transferring properties between objects */
  { "show_background", PROP_TYPE_BOOL, PROP_FLAG_DONT_SAVE,  N_("Draw background"), NULL, NULL },
  { "show_control_lines", PROP_TYPE_BOOL, PROP_FLAG_OPTIONAL, N_("Draw Control Lines") },
  { "pattern", PROP_TYPE_PATTERN, PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL, N_("Pattern"), NULL },
  PROP_DESC_END
};

static PropOffset stdpath_offsets[] = {
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { "bez_points", PROP_TYPE_BEZPOINTARRAY, offsetof(StdPath,points), offsetof(StdPath,num_points)},
  { "stroke_or_fill", PROP_TYPE_ENUM, offsetof(StdPath, stroke_or_fill) },
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(StdPath, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(StdPath, line_color) },
  { "line_style", PROP_TYPE_LINESTYLE, offsetof(StdPath, line_style), offsetof(StdPath, dashlength) },
  { "line_join", PROP_TYPE_ENUM, offsetof(StdPath, line_join) },
  { "line_caps", PROP_TYPE_ENUM, offsetof(StdPath, line_caps) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(StdPath, fill_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(StdPath, show_background) },
  { "show_control_lines", PROP_TYPE_BOOL, offsetof(StdPath, show_control_lines) },
  { "pattern", PROP_TYPE_PATTERN, offsetof(StdPath, pattern) },
  { NULL, 0, 0 }
};

/*!
 * To make this static again we would need some registering function
 */
DiaObjectType stdpath_type =
{
  "Standard - Path",      /* name */
  0,                      /* version */
  (const char **) "res:/org/gnome/Dia/icons/standard-path.png",
  &stdpath_type_ops,      /* ops */
  NULL,                   /* pixmap_file */
  0,                      /* default_user_data */
  stdpath_props,
  stdpath_offsets
};


/* Class definition */
static DiaObjectChange *stdpath_move_handle (StdPath          *stdpath,
                                             Handle           *handle,
                                             Point            *to,
                                             ConnectionPoint  *cp,
                                             HandleMoveReason  reason,
                                             ModifierKeys      modifiers);
static DiaObjectChange *stdpath_move        (StdPath          *stdpath,
                                             Point            *to);
static void stdpath_select(StdPath *stdpath, Point *clicked_point,
			   DiaRenderer *interactive_renderer);
static void stdpath_draw(StdPath *stdpath, DiaRenderer *renderer);
static real stdpath_distance_from (StdPath *stdpath, Point *point);
static void stdpath_update_data (StdPath *stdpath);
static void stdpath_destroy (StdPath *stdpath);
static DiaObject *stdpath_copy (StdPath *stdpath);
static DiaMenu *stdpath_get_object_menu(StdPath *stdpath,
					Point *clickedpoint);
static void stdpath_get_props(StdPath *stdpath, GPtrArray *props);
static void stdpath_set_props(StdPath *stdpath, GPtrArray *props);
static gboolean stdpath_transform(StdPath *stdpath, const DiaMatrix *m);

static ObjectOps stdpath_ops = {
  (DestroyFunc)         stdpath_destroy,
  (DrawFunc)            stdpath_draw,
  (DistanceFunc)        stdpath_distance_from,
  (SelectFunc)          stdpath_select,
  (CopyFunc)            stdpath_copy,
  (MoveFunc)            stdpath_move,
  (MoveHandleFunc)      stdpath_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      stdpath_get_object_menu,
  (DescribePropsFunc)   object_describe_props,
  (GetPropsFunc)        stdpath_get_props,
  (SetPropsFunc)        stdpath_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
  (TransformFunc)       stdpath_transform,
};

/*!
 * \brief Setup unchangeable part of the object handles
 *
 * The object must be initalized already.
 * \protected \memberof _StdPath
 *
 * \todo Create a new handle type HANDLE_INVISBLE couple with object lock
 */
static void
stdpath_init_handles (StdPath *stdpath)
{
  DiaObject *obj = &stdpath->object;
  int i;

  g_return_if_fail (obj->handles != NULL && obj->num_handles == NUM_HANDLES);

  for (i = 0; i < NUM_HANDLES; ++i) {
    obj->handles[i] = &stdpath->handles[i];
    obj->handles[i]->connect_type = HANDLE_NONCONNECTABLE;
    obj->handles[i]->type = HANDLE_MAJOR_CONTROL;
    obj->handles[i]->connected_to = NULL;
  }
  /* from left to right, from top to bottom */
  obj->handles[0]->id = HANDLE_RESIZE_NW;
  obj->handles[1]->id = HANDLE_RESIZE_N;
  obj->handles[2]->id = HANDLE_RESIZE_NE;
  obj->handles[3]->id = HANDLE_RESIZE_W;
  obj->handles[4]->id = HANDLE_RESIZE_E;
  obj->handles[5]->id = HANDLE_RESIZE_SW;
  obj->handles[6]->id = HANDLE_RESIZE_S;
  obj->handles[7]->id = HANDLE_RESIZE_SE;
}
/*! Factory function - create default object */
static DiaObject *
stdpath_create (Point *startpoint,
		void *user_data,
		Handle **handle1,
		Handle **handle2)
{
  StdPath *stdpath;
  DiaObject *obj;
  Point sp = {0, 0};

  stdpath = g_new0 (StdPath,1);
  obj = &stdpath->object;
  obj->type = &stdpath_type;
  obj->ops = &stdpath_ops;

  object_init (obj, NUM_HANDLES, 0);
  stdpath_init_handles (stdpath);
  if (startpoint)
    sp = *startpoint;

  if (user_data == NULL) {
    /* just to have something to play with
       <bezpoint type="moveto" p1="0,1"/>
       <bezpoint type="curveto" p1="0,0" p2="2,2" p3="2,1"/>
       <bezpoint type="curveto" p1="2,0" p2="0,2" p3="0,1"/>
     */
    BezPoint *bp;
    stdpath->num_points = 3;
    bp = stdpath->points = g_new (BezPoint, 3);
    bp[0].type = BEZ_MOVE_TO;
    bp[0].p1.x = sp.x + 0; bp[0].p1.y = sp.y + 1;
    bp[0].p3 = bp[0].p2 = bp[0].p1; /* not strictly necessary */
    bp[1].type = BEZ_CURVE_TO;
    bp[1].p1 = sp;
    bp[1].p2.x = sp.x + 2; bp[1].p2.y = sp.y + 2;
    bp[1].p3.x = sp.x + 2; bp[1].p3.y = sp.y + 1;
    bp[2].type = BEZ_CURVE_TO;
    bp[2].p1.x = sp.x + 2; bp[2].p1.y = sp.y + 0;
    bp[2].p2.x = sp.x + 0; bp[2].p2.y = sp.y + 2;
    bp[2].p3.x = sp.x + 0; bp[2].p3.y = sp.y + 1;
  } else {
    BezierCreateData *bcd = (BezierCreateData*)user_data;

    if (bcd->num_points < 2) {
      g_warning ("'Standard - Path' needs at least two points");
      /* this is a stress test - object might not be setup completely */
      object_destroy (obj);
      g_clear_pointer (&stdpath, g_free);
      return NULL;
    }
    stdpath->num_points = bcd->num_points;
    stdpath->points = g_memdup2 (bcd->points,
                                 bcd->num_points * sizeof(BezPoint));
  }

  stdpath->stroke_or_fill = PDO_STROKE; /* default: stroke only */
  stdpath->line_width = attributes_get_default_linewidth();
  stdpath->line_color = attributes_get_foreground();
  stdpath->fill_color = attributes_get_background();

  *handle1 = stdpath->object.handles[0];
  *handle2 = stdpath->object.handles[7];

  stdpath_update_data (stdpath);

  return obj;
}

static DiaObject *
stdpath_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&stdpath_type, obj_node, version, ctx);
}

/*!
 * \brief Update handle positions to reflect the current object state
 *
 * The object bounding box needs to be calculated already.
 * \protected \memberof _StdPath
 */
static void
stdpath_update_handles(StdPath *stdpath)
{
  DiaObject *obj = &stdpath->object;
  PolyBBExtras extra = { 0, };
  DiaRectangle rect, *bb;

  g_return_if_fail (obj->handles != NULL);

  /* Using the zero-line-width boundingbox for handles */
  bb = &rect;
  polybezier_bbox (stdpath->points, stdpath->num_points, &extra,
		   FALSE /*(stdpath->stroke_or_fill & PDO_FILL)*/, bb);

  /* from left to right, from top to bottom */
  obj->handles[0]->pos.x = bb->left;
  obj->handles[0]->pos.y = bb->top;
  obj->handles[1]->pos.x = (bb->left + bb->right) / 2.0;
  obj->handles[1]->pos.y = bb->top;
  /* adjust handle pos for runtime shear? */
  obj->handles[2]->pos.x = bb->right;
  obj->handles[2]->pos.y = bb->top;
  obj->handles[3]->pos.x = bb->left;
  obj->handles[3]->pos.y = (bb->top + bb->bottom) / 2.0;
  obj->handles[4]->pos.x = bb->right;
  obj->handles[4]->pos.y = (bb->top + bb->bottom) / 2.0;
  /* adjust handle pos for runtime perspective? */
  obj->handles[5]->pos.x = bb->left;
  obj->handles[5]->pos.y = bb->bottom;
  obj->handles[6]->pos.x = (bb->left + bb->right) / 2.0;
  obj->handles[6]->pos.y = bb->bottom;
  obj->handles[7]->pos.x = bb->right;
  obj->handles[7]->pos.y = bb->bottom;
}
/*!
 * \brief Object update function called after property change
 *
 * Not in the object interface but very important anyway.
 * Used to recalculate the object data after a change
 * \protected \memberof _StdPath
 */
static void
stdpath_update_data (StdPath *stdpath)
{
  DiaObject *obj = &stdpath->object;
  DiaRectangle *bb = &obj->bounding_box;
  PolyBBExtras extra = { 0 };
  real lw = stdpath->stroke_or_fill & PDO_STROKE ? stdpath->line_width : 0.0;

  extra.start_trans =
  extra.end_trans =
  extra.start_long =
  extra.end_long =
  extra.middle_trans = lw/2.0;

  /* recalculate the bounding box */
  polybezier_bbox (stdpath->points, stdpath->num_points, &extra,
		   FALSE /*(stdpath->stroke_or_fill & PDO_FILL)*/, bb);
  /* adjust position from it */
  obj->position.x = stdpath->points[0].p1.x;
  obj->position.y = stdpath->points[0].p1.y;
  /* adjust handles */
  stdpath_update_handles (stdpath);
}

/*!
 * \brief Object drawing to the given renderer
 *
 * \memberof _StdPath
 */
static void
stdpath_draw (StdPath *stdpath, DiaRenderer *renderer)
{
  dia_renderer_set_linewidth (renderer, stdpath->line_width);
  dia_renderer_set_linestyle (renderer, stdpath->line_style, stdpath->dashlength);
  dia_renderer_set_linejoin (renderer, stdpath->line_join);
  dia_renderer_set_linecaps (renderer, stdpath->line_caps);

  if (dia_renderer_is_capable_of (renderer, RENDER_HOLES)) {
    if (stdpath->stroke_or_fill & PDO_FILL) {
      Color fill = stdpath->fill_color;
      if (stdpath->pattern) {
        dia_pattern_get_fallback_color (stdpath->pattern, &fill);
        if (dia_renderer_is_capable_of (renderer, RENDER_PATTERN)) {
          dia_renderer_set_pattern (renderer, stdpath->pattern);
        }
      }
      if (stdpath->stroke_or_fill & PDO_STROKE) { /* also stroke -> combine */
        dia_renderer_draw_beziergon (renderer,
                                     stdpath->points,
                                     stdpath->num_points,
                                     &fill,
                                     &stdpath->line_color);
      } else {
        dia_renderer_draw_beziergon (renderer,
                                     stdpath->points,
                                     stdpath->num_points,
                                     &fill,
                                     NULL);
      }
      if (dia_renderer_is_capable_of (renderer, RENDER_PATTERN)) {
        dia_renderer_set_pattern (renderer, NULL);
      }
    }
    if (stdpath->stroke_or_fill == PDO_STROKE) { /* stroke only */
      dia_renderer_draw_bezier (renderer,
                                stdpath->points,
                                stdpath->num_points,
                                &stdpath->line_color);
    }
  } else {
    /* step-wise approach */
    /* if it wouldn't RENDER_HOLES it presumably also wouldn't RENDER_PATTERN ... */
    if (stdpath->stroke_or_fill & PDO_FILL) {
      Color fill = stdpath->fill_color;
      if (stdpath->pattern) {
        dia_pattern_get_fallback_color (stdpath->pattern, &fill);
      }
      dia_renderer_bezier_fill (renderer, stdpath->points, stdpath->num_points, &fill);
    }

    if (stdpath->stroke_or_fill & PDO_STROKE) {
      dia_renderer_bezier_stroke (renderer, stdpath->points, stdpath->num_points, &stdpath->line_color);
    }
  }

  if (stdpath->show_control_lines) {
    bezier_draw_control_lines (stdpath->num_points, stdpath->points, renderer);
  }
}

static DiaObjectChange *_path_object_invert_change_create    (DiaObject *obj);
static DiaObjectChange *_path_object_transform_change_create (DiaObject *obj,
                                                              DiaMatrix *mat);

/*!
 * \brief Change the direction of the path
 */
static void
_stdpath_invert (StdPath *stdpath)
{
  BezPoint *bezier = stdpath->points;
  gsize n = stdpath->num_points;
  BezPoint *inverted = g_alloca (sizeof(BezPoint)*n);
  guint i;

  inverted[0].type = BEZ_MOVE_TO;
  inverted[0].p1 = (bezier[n-1].type == BEZ_CURVE_TO ? bezier[n-1].p3 : bezier[n-1].p1);
  for (i = 1; i < n; ++i) {
    inverted[i].type = bezier[n-i].type;
    if (bezier[n-i].type == BEZ_CURVE_TO) {
      inverted[i].p1 = bezier[n-i].p2;
      inverted[i].p2 = bezier[n-i].p1;
      inverted[i].p3 = (bezier[n-i-1].type == BEZ_CURVE_TO ? bezier[n-i-1].p3 : bezier[n-i-1].p1);
    } else {
      inverted[i].p1 = (bezier[n-i-1].type == BEZ_CURVE_TO ? bezier[n-i-1].p3 : bezier[n-i-1].p1);
    }
  }
  memcpy (stdpath->points, inverted, sizeof(BezPoint)*n);
  stdpath_update_handles (stdpath);
}


static DiaObjectChange *
_invert_path_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  StdPath *stdpath = (StdPath *) obj;
  _stdpath_invert (stdpath);
  return _path_object_invert_change_create (obj);
}


struct _DiaPathObjectChange {
  DiaObjectChange parent;
};


DIA_DEFINE_OBJECT_CHANGE (DiaPathObjectChange, dia_path_object_change)


static void
dia_path_object_change_free (DiaObjectChange *self)
{

}


static void
dia_path_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  _stdpath_invert ((StdPath *) obj);
}


static void
dia_path_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  _stdpath_invert ((StdPath *) obj);
}


static DiaObjectChange *
_path_object_invert_change_create (DiaObject *obj)
{
  return dia_object_change_new (DIA_TYPE_PATH_OBJECT_CHANGE);
}


static void
_path_transform (StdPath *sp, const DiaMatrix *m)
{
  BezPoint *bezier = sp->points;
  gsize n = sp->num_points;
  guint i;

  for (i = 0; i < n; ++i)
    transform_bezpoint (&bezier[i], m);

  stdpath_update_data (sp);
}


struct _DiaPathTransformObjectChange {
  DiaObjectChange change;
  DiaMatrix       matrix;
};


DIA_DEFINE_OBJECT_CHANGE (DiaPathTransformObjectChange,
                          dia_path_transform_object_change)


static void
dia_path_transform_object_change_free (DiaObjectChange *self)
{

}


static void
dia_path_transform_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaPathTransformObjectChange *ptc = DIA_PATH_TRANSFORM_OBJECT_CHANGE (self);
  StdPath *sp = (StdPath *) obj;

  _path_transform (sp, &ptc->matrix);
}


static void
dia_path_transform_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  StdPath *sp = (StdPath *) obj;
  DiaPathTransformObjectChange *ptc = DIA_PATH_TRANSFORM_OBJECT_CHANGE (self);
  DiaMatrix mi = ptc->matrix;

  if (cairo_matrix_invert ((cairo_matrix_t *) &mi) != CAIRO_STATUS_SUCCESS) {
    g_warning ("_ptc_revert matrix invert");
  }
  _path_transform (sp, &mi);
}


static DiaObjectChange *
_path_object_transform_change_create (DiaObject *obj, DiaMatrix *matrix)
{
  DiaPathTransformObjectChange *ptc;

  ptc = dia_object_change_new (DIA_TYPE_PATH_TRANSFORM_OBJECT_CHANGE);

  ptc->matrix = *matrix;

  return DIA_OBJECT_CHANGE (ptc);
}


/*!
 * \brief Flip the path over the vertical or horizontal axis
 *
 * Flip horizontal/vertical like custom objects do, except that this
 * function is modifying the underlying point data. This function is
 * available with the context menu of the 'Standard - Path' object.
 * @return Undo information
 *
 * \relates _StdPath
 */
static DiaObjectChange *
_path_flip_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  gboolean horz = data == NULL;
  StdPath *sp = (StdPath *)obj;
  DiaMatrix m = {horz ? -1 : 1, 0, 0, horz ? 1 : -1, 0, 0 };
  DiaMatrix translate = {1, 0, 0, 1, 0, 0 };
  real cx = (sp->object.bounding_box.left + sp->object.bounding_box.right) / 2;
  real cy = (sp->object.bounding_box.top + sp->object.bounding_box.bottom) / 2;

  /* flip around center */
  translate.x0 = cx;
  translate.y0 = cy;
  dia_matrix_multiply (&m, &m, &translate);
  translate.x0 = -cx;
  translate.y0 = -cy;
  dia_matrix_multiply (&m, &translate, &m);

  _path_transform (sp, &m);
  return _path_object_transform_change_create (obj, &m);
}

/*!
 * \brief Rotate path towards clicked point
 *
 * Rotate the given path around it's center with the angle given by
 * the top center handle and the clicked point. This function is
 * available with the context menu of the 'Standard - Path' object.
 * @return Undo information
 *
 * \relates _StdPath
 */
static DiaObjectChange *
_path_rotate_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  StdPath *sp = (StdPath *)obj;
  real cx = (sp->object.bounding_box.left + sp->object.bounding_box.right) / 2;
  real cy = (sp->object.bounding_box.top + sp->object.bounding_box.bottom) / 2;
  DiaMatrix m = {1, 0, 0, 1, 0, 0 };
  DiaMatrix translate = {1, 0, 0, 1, 0, 0 };
  Point d;

  d.x = clicked->x - cx;
  d.y = clicked->y - cy;
  point_normalize (&d);
  /* to make the top, center handle clicked no rotation we subtract -PI/2 */
  dia_matrix_set_angle_and_scales (&m, atan2 (d.y, d.x) + M_PI/2.0, 1.0, 1.0);

  /* rotate around center */
  translate.x0 = cx;
  translate.y0 = cy;
  dia_matrix_multiply (&m, &m, &translate);
  translate.x0 = -cx;
  translate.y0 = -cy;
  dia_matrix_multiply (&m, &translate, &m);

  _path_transform (sp, &m);
  return _path_object_transform_change_create (obj, &m);
}

static const Handle *
_path_closest_corner_handle (StdPath *sp, const Point *pt)
{
  int i, j;
  real dist = G_MAXDOUBLE;
  Handle *closest = NULL;
  int corners[] = {0, 2, 5, 7};

  for (j = 0; j < 4; ++j) {
    real new_dist;

    i = corners[j];
    new_dist = distance_point_point (&sp->handles[i].pos, pt);
    if (new_dist < dist) {
      closest = &sp->handles[i];
      dist = new_dist;
    }
  }
  return closest;
}

/*!
 * \brief Shear path towards clicked point
 *
 * Shear the given path towards the clicked point by the amount of
 * the closest corner handle. This is either horizonatl or vertical
 * shear depending on the bigger delta between the handle and
 * the clicked point.
 * This function is available with the context menu of the
 * 'Standard - Path' object.
 * @return Undo information
 *
 * \relates _StdPath
 */
static DiaObjectChange *
_path_shear_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  StdPath *sp = (StdPath *)obj;
  DiaMatrix m = {1, 0, 0, 1, 0, 0 };
  DiaMatrix translate = {1, 0, 0, 1, 0, 0 };
  real cx = (sp->object.bounding_box.left + sp->object.bounding_box.right) / 2;
  real cy = (sp->object.bounding_box.top + sp->object.bounding_box.bottom) / 2;
  const Handle *handle = _path_closest_corner_handle (sp, clicked);
  real dx, dy;

  g_return_val_if_fail (handle != NULL, NULL);

  /* goal is to shear the point at corner handle close to the clicked point */
  dx = (clicked->x - handle->pos.x) * (handle->pos.y > cy ? 1 : -1);
  dy = clicked->y - handle->pos.y;
  /* normalize shear with handle distance from center, only shear one axis */
  if (fabs(dx) > fabs(dy))
    m.xy = dx / fabs(handle->pos.x - cx);
  else
    m.yx = dy / fabs(handle->pos.y - cy);
  /* XXX: shear around center */
  translate.x0 = cx;
  translate.y0 = cy;
  dia_matrix_multiply (&m, &m, &translate);
  translate.x0 = -cx;
  translate.y0 = -cy;
  dia_matrix_multiply (&m, &translate, &m);

  _path_transform (sp, &m);
  return _path_object_transform_change_create (obj, &m);
}

/*!
 * \brief Convert _StdPath to one or more _BezierLine/_Beziergon
 *
 * This conversion splits the given path at it's move-to and creates
 * a group of _BezierLine or _Beziergon as substitute for the single
 * path object. If the path is filled a group of _Beziergon is created.
 *
 * @return Undo information
 *
 * \relates _StdPath
 */
static DiaObjectChange *
_convert_to_beziers_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  StdPath *stdpath = (StdPath *)obj;
  BezPoint *bezier = stdpath->points;
  GList *list = NULL;
  int i, n = 0;
  DiaObjectChange *change;

  for (i = 1; i < stdpath->num_points; ++i) {
    if (bezier[i].type == BEZ_MOVE_TO || i + 1 == stdpath->num_points) {
      DiaObject *rep;
      int num = bezier[i].type == BEZ_MOVE_TO ? i - n : i - n + 1;
      if (stdpath->stroke_or_fill & PDO_FILL) {
        rep = create_standard_beziergon (num, &bezier[n]);
      } else {
        rep = create_standard_bezierline (num, &bezier[n], NULL, NULL);
      }
      if (!rep) {
        /* no Standard objects? */
        break;
      }
      list = g_list_append (list, rep);
      n = i;
    }
  }

  if (!list) {
    change = dia_object_change_list_new ();
  } else if (g_list_length (list) == 1) {
    change = object_substitute (obj, DIA_OBJECT (list->data));
    g_list_free (list);
  } else {
    change = object_substitute (obj, create_standard_group (list));
  }

  return change;
}


static DiaObjectChange *
_show_control_lines (DiaObject *obj, Point *clicked, gpointer data)
{
  StdPath *stdpath = (StdPath *) obj;

  return object_toggle_prop (obj,
                             "show_control_lines",
                             !stdpath->show_control_lines);
}


static DiaMenuItem _stdpath_menu_items[] = {
  { N_("Convert to Bezier"), _convert_to_beziers_callback, NULL, DIAMENU_ACTIVE },
  { N_("Invert Path"), _invert_path_callback, NULL, DIAMENU_ACTIVE },
  { N_("Flip Horizontal"), _path_flip_callback, NULL, DIAMENU_ACTIVE },
  { N_("Flip Vertical"), _path_flip_callback, GINT_TO_POINTER(1), DIAMENU_ACTIVE },
  { N_("Rotate"), _path_rotate_callback, NULL, DIAMENU_ACTIVE },
  { N_("Shear"), _path_shear_callback, NULL, DIAMENU_ACTIVE },
  { N_("Show Control Lines"), _show_control_lines, NULL, DIAMENU_ACTIVE | DIAMENU_TOGGLE }
};
static DiaMenu _stdpath_menu = {
  "Path",
  sizeof(_stdpath_menu_items)/sizeof(DiaMenuItem),
  _stdpath_menu_items,
  NULL
};

/*!
 * \brief Optionally deliver an object specific menu
 *
 * returning NULL used to crash Dia, the method itself needed to be NULL
 * if there was nothing to do
 *
 * \memberof _StdPath
 */
static DiaMenu *
stdpath_get_object_menu(StdPath *stdpath, Point *clickedpoint)
{
  const int i = G_N_ELEMENTS(_stdpath_menu_items) - 1;
  if (stdpath->show_control_lines)
    _stdpath_menu_items[i].active |= DIAMENU_TOGGLE_ON;
  else
    _stdpath_menu_items[i].active &= ~DIAMENU_TOGGLE_ON;
  return &_stdpath_menu;
}

/*!
 * \brief Initialize the given property vector from the object state.
 *
 * If  offsets and props are part of the object type this does not to be
 * implemented usually. Just object_get_props in the 'vtable' would be enough.
 * We want to ensure that stroke_or_fill and show_background are in sync, though.
 */
static void
stdpath_get_props(StdPath *stdpath, GPtrArray *props)
{
  stdpath->show_background = (stdpath->stroke_or_fill & PDO_FILL) != 0;
  object_get_props(&stdpath->object, props);
}
/*!
 * \brief Set the object state from the given proeprty vector
 * \memberof _StdPath
 */
static void
stdpath_set_props (StdPath *stdpath, GPtrArray *props)
{
  Property *prop;
  stdpath->show_background = (stdpath->stroke_or_fill & PDO_FILL) != 0;
  object_set_props_from_offsets(&stdpath->object, stdpath_offsets, props);
  /* Usually the list wont contain "show_background", but if it
   * it set let it take precedence
   */
  if (   (prop = find_prop_by_name (props, "show_background")) != NULL
      && (prop->experience & PXP_NOTSET) == 0) {
    if (stdpath->show_background)
      stdpath->stroke_or_fill |= PDO_FILL;
    else
      stdpath->stroke_or_fill &= ~PDO_FILL;
  }
  /* now when transferring properties from text we'll loose stroke and fill
   * Instead of drawing nothing make it just fill.
   */
  if (!stdpath->stroke_or_fill)
    stdpath->stroke_or_fill = PDO_FILL;

  stdpath_update_data (stdpath);
}
/*!
 * \brief Calculate the distance of the whole object to the given point
 * \memberof _StdPath
 */
static real
stdpath_distance_from (StdPath *stdpath, Point *point)
{
  real lw = stdpath->stroke_or_fill & PDO_STROKE ? stdpath->line_width : 0.0;

  if (stdpath->stroke_or_fill & PDO_FILL)
    return distance_bez_shape_point (stdpath->points, stdpath->num_points, lw, point);
  else
    return distance_bez_line_point  (stdpath->points, stdpath->num_points, lw, point);
}

static void
_stdpath_scale (StdPath *stdpath, real sx, real sy, const Point *around)
{
  Point p0;
  int i;

  if (around)
    p0 = *around;
  else
    p0 = stdpath->object.position;

  for (i = 0; i < stdpath->num_points; ++i) {
    BezPoint *bp = &stdpath->points[i];

    bp->p1.x = p0.x + (bp->p1.x - p0.x) * sx;
    bp->p1.y = p0.y + (bp->p1.y - p0.y) * sy;
    bp->p2.x = p0.x + (bp->p2.x - p0.x) * sx;
    bp->p2.y = p0.y + (bp->p2.y - p0.y) * sy;
    bp->p3.x = p0.x + (bp->p3.x - p0.x) * sx;
    bp->p3.y = p0.y + (bp->p3.y - p0.y) * sy;
  }
}


/*!
 * \brief Move one of the objects handles
 * \memberof _StdPath
 */
static DiaObjectChange *
stdpath_move_handle (StdPath          *stdpath,
                     Handle           *handle,
                     Point            *to,
                     ConnectionPoint  *cp,
                     HandleMoveReason  reason,
                     ModifierKeys      modifiers)
{
  const double EPSILON = 0.01;

  /* move_handle is supposed to be just moving that and related handles (e.g.
   * when N is moved NE and NW will move too). But 'opposite' handles are not
   * to be moved at all. So our transformation is invariant to the opposite
   * point of the path, not the object position.
   *
   * Another issue is limit checking. The path object must not get too small,
   * otherwise it can not be restored by reverse move_handle. So implement a
   * minimum distance between the moving and the opposite handle.
   */
  if (handle->id == HANDLE_RESIZE_NW) {
    Point p0 = stdpath->handles[7].pos; /* SE */
    Point p1 = stdpath->handles[0].pos;
    real sx = (to->x - p0.x) / (p1.x - p0.x);
    real sy = (to->y - p0.y) / (p1.y - p0.y);

    g_assert(stdpath->handles[0].id == handle->id);
    g_assert(stdpath->handles[7].id == HANDLE_RESIZE_SE);

    if (to->x + EPSILON > p0.x)
      sx = 1.0;
    if (to->y + EPSILON > p0.y)
      sy = 1.0;
    _stdpath_scale (stdpath, sx, sy, &p0);
  } else if (handle->id == HANDLE_RESIZE_N) {
    Point p0 = stdpath->handles[6].pos; /* S */
    Point p1 = stdpath->handles[1].pos;
    real sy = (to->y - p0.y) / (p1.y - p0.y);

    g_assert(stdpath->handles[1].id == handle->id);
    g_assert(stdpath->handles[6].id == HANDLE_RESIZE_S);

    if (to->y + EPSILON > p0.y)
      sy = 1.0;
    _stdpath_scale (stdpath, 1.0, sy, &p0);
  } else if (handle->id == HANDLE_RESIZE_NE) {
    Point p0 = stdpath->handles[5].pos; /* SW */
    Point p1 = stdpath->handles[2].pos;
    real sx = (to->x - p0.x) / (p1.x - p0.x);
    real sy = (to->y - p0.y) / (p1.y - p0.y);

    g_assert(stdpath->handles[2].id == handle->id);
    g_assert(stdpath->handles[5].id == HANDLE_RESIZE_SW);

    if (to->x - EPSILON < p0.x)
      sx = 1.0;
    if (to->y + EPSILON > p0.y)
      sy = 1.0;
    _stdpath_scale (stdpath, sx, sy, &p0);
  } else if (handle->id == HANDLE_RESIZE_W) {
    /* scale width */
    Point p0 = stdpath->handles[4].pos; /* E */
    Point p1 = stdpath->handles[3].pos;
    real sx = (to->x - p0.x) / (p1.x - p0.x);

    g_assert(stdpath->handles[3].id == handle->id);
    g_assert(stdpath->handles[4].id == HANDLE_RESIZE_E);

    if (to->x + EPSILON > p0.x)
      sx = 1.0;
    _stdpath_scale (stdpath, sx, 1.0, &p0);
  } else if (handle->id == HANDLE_RESIZE_E) {
    /* scale width */
    Point p0 = stdpath->handles[3].pos; /* W */
    Point p1 = stdpath->handles[4].pos;
    real sx = (to->x - p0.x) / (p1.x - p0.x);

    g_assert(stdpath->handles[4].id == handle->id);
    g_assert(stdpath->handles[3].id == HANDLE_RESIZE_W);

    if (to->x - EPSILON < p0.x)
      sx = 1.0;
    _stdpath_scale (stdpath, sx, 1.0, &p0);
  } else if (handle->id == HANDLE_RESIZE_SW) {
    Point p0 = stdpath->handles[2].pos; /* NE */
    Point p1 = stdpath->handles[5].pos;
    real sx = (to->x - p0.x) / (p1.x - p0.x);
    real sy = (to->y - p0.y) / (p1.y - p0.y);

    g_assert(stdpath->handles[5].id == handle->id);
    g_assert(stdpath->handles[2].id == HANDLE_RESIZE_NE);

    if (to->x + EPSILON > p0.x)
      sx = 1.0;
    if (to->y - EPSILON < p0.y)
      sy = 1.0;
    _stdpath_scale (stdpath, sx, sy, &p0);
  } else if (handle->id == HANDLE_RESIZE_S) {
    /* scale height */
    Point p0 = stdpath->handles[1].pos; /* N */
    Point p1 = stdpath->handles[6].pos;
    real sy = (to->y - p0.y) / (p1.y - p0.y);

    g_assert(stdpath->handles[6].id == handle->id);
    g_assert(stdpath->handles[1].id == HANDLE_RESIZE_N);

    if (to->y - EPSILON < p0.y)
      sy = 1.0;
    _stdpath_scale (stdpath, 1.0, sy, &p0);
  } else if (handle->id == HANDLE_RESIZE_SE) {
    /* scale both directions - keep aspect ratio? */
    Point p0 = stdpath->handles[0].pos; /* NW */
    Point p1 = stdpath->handles[7].pos;
    real sx = (to->x - p0.x) / (p1.x - p0.x);
    real sy = (to->y - p0.y) / (p1.y - p0.y);

    g_assert(stdpath->handles[7].id == handle->id);
    g_assert(stdpath->handles[0].id == HANDLE_RESIZE_NW);

    if (to->x - EPSILON < p0.x)
      sx = 1.0;
    if (to->y - EPSILON < p0.y)
      sy = 1.0;
    _stdpath_scale (stdpath, sx, sy, &p0);
  } else if (handle->type != HANDLE_NON_MOVABLE) {
    g_warning ("stdpath_move_handle() %d not moving", handle->id);
  }
  stdpath_update_data (stdpath);
  return NULL;
}


/*!
 * \brief Move the whole object to the given position
 *
 * If the object position does not change the whole object should not either.
 * This is used as a kludge to call the protected update_data() function
 *
 * \memberof _StdPath
 */
static DiaObjectChange *
stdpath_move (StdPath *stdpath, Point *to)
{
  DiaObject *obj = &stdpath->object;
  Point delta = *to;
  int i;

  point_sub (&delta, &obj->position);

  for (i = 0; i < stdpath->num_points; ++i) {
     BezPoint *bp = &stdpath->points[i];

     point_add (&bp->p1, &delta);
     point_add (&bp->p2, &delta);
     point_add (&bp->p3, &delta);
  }
  stdpath_update_data (stdpath);
  return NULL;
}
/*!
 * \brief Create a deep copy of the object
 * \memberof _StdPath
 */
static DiaObject *
stdpath_copy (StdPath *from)
{
  DiaObject *to;
  to = object_copy_using_properties (&from->object);
  return to;
}


/*!
 * \brief Destruction of the object
 * \memberof _StdPath
 */
static void
stdpath_destroy (StdPath *stdpath)
{
  object_destroy (&stdpath->object);
  g_clear_object (&stdpath->pattern);
  g_clear_pointer (&stdpath->points, g_free);
  /* but not the object itself */
}


/*!
 * \brief Change the object state regarding selection
 * \memberof _StdPath
 */
static void
stdpath_select (StdPath *stdpath, Point *clicked_point,
		DiaRenderer *interactive_renderer)
{
  stdpath_update_handles (stdpath);
}
/*!
 * \brief Change the object state regarding selection
 * \memberof _StdPath
 */
static gboolean
stdpath_transform(StdPath *stdpath, const DiaMatrix *m)
{
  int i;

  g_return_val_if_fail (m != NULL, FALSE);

  for (i = 0; i < stdpath->num_points; i++)
    transform_bezpoint (&stdpath->points[i], m);

  stdpath_update_data(stdpath);
  return TRUE;
}

gboolean
text_to_path (const Text *text, GArray *points)
{
  cairo_t *cr;
  cairo_surface_t *surface;
  PangoLayout *layout;
  PangoRectangle ink_rect;
  char *str;
  gboolean ret = FALSE;

  if (!PANGO_IS_CAIRO_FONT_MAP (pango_context_get_font_map (dia_font_get_context())))
    return FALSE;

  layout = pango_layout_new(dia_font_get_context());
  pango_layout_set_font_description (layout, dia_font_get_description (text->font));
  pango_layout_set_indent (layout, 0);
  pango_layout_set_justify (layout, FALSE);
  pango_layout_set_alignment (layout,
                              text->alignment == DIA_ALIGN_LEFT ?
                                PANGO_ALIGN_LEFT : text->alignment == DIA_ALIGN_RIGHT ?
                                  PANGO_ALIGN_RIGHT : PANGO_ALIGN_CENTER);

  str = text_get_string_copy (text);
  pango_layout_set_text (layout, str, -1);
  g_clear_pointer (&str, g_free);

  pango_layout_get_extents (layout, &ink_rect, NULL);
  /* any surface should do - this one is always available */
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					ink_rect.width / PANGO_SCALE, ink_rect.height / PANGO_SCALE);
  cr = cairo_create (surface);
  cairo_surface_destroy (surface);

  pango_cairo_layout_path (cr, layout);

  /* convert the path */
  if (cairo_status (cr) == CAIRO_STATUS_SUCCESS)
  {
    cairo_path_t *path;
    int i;

    path = cairo_copy_path (cr);

    for (i=0; i < path->num_data; i += path->data[i].header.length) {
      cairo_path_data_t *data = &path->data[i];
      BezPoint bp;

      switch (data->header.type) {
      case CAIRO_PATH_MOVE_TO :
	bp.type = BEZ_MOVE_TO;
	bp.p1.x = data[1].point.x; bp.p1.y = data[1].point.y;
	break;
      case CAIRO_PATH_LINE_TO :
        bp.type = BEZ_LINE_TO;
	bp.p1.x = data[1].point.x; bp.p1.y = data[1].point.y;
	break;
      case CAIRO_PATH_CURVE_TO :
        bp.type = BEZ_CURVE_TO;
	bp.p1.x = data[1].point.x; bp.p1.y = data[1].point.y;
	bp.p2.x = data[2].point.x; bp.p2.y = data[2].point.y;
	bp.p3.x = data[3].point.x; bp.p3.y = data[3].point.y;
	break;
      case CAIRO_PATH_CLOSE_PATH :
        /* can't do anything */
      default :
        continue;
      }
      g_array_append_val (points, bp);
    }
    ret = (path->status == CAIRO_STATUS_SUCCESS);
    cairo_path_destroy (path);
  }
  /* finally scale it ? */

  /* clean up */
  g_clear_object (&layout);
  cairo_destroy (cr);

  return ret;
}

DiaObject *
create_standard_path_from_text (const Text *text)
{
  DiaObject *obj = NULL;
  GArray *points = g_array_new (FALSE, FALSE, sizeof(BezPoint));

  if (text_to_path (text, points))
    obj = create_standard_path (points->len, &g_array_index (points, BezPoint, 0));

  g_array_free (points, TRUE);

  if (obj) {
    StdPath *path = (StdPath *)obj;
    DiaRectangle text_box;
    const DiaRectangle *pbb = &path->object.bounding_box;
    real sx, sy;
    Point pos;

    path->stroke_or_fill = PDO_FILL;
    path->fill_color = text->color;

    /* scale to fit the original size */
    text_calc_boundingbox ((Text *)text, &text_box);
    pos.x = text_box.left; pos.y = text_box.top;
    sx = (text_box.right - text_box.left) / (pbb->right - pbb->left);
    sy = (text_box.bottom - text_box.top) / (pbb->bottom - pbb->top);
    _stdpath_scale (path, sx, sy, NULL);

    /* also adjust top left corner - calling update, too */
    stdpath_move (path, &pos);
  }

  return obj;
}
