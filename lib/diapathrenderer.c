/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diapathrenderer.c -- render _everything_ to a path
 * Copyright (C) 2012 Hans Breuer <hans@breuer.org>
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

#include "diapathrenderer.h"
#include "text.h" /* just for text->color */
#include "standard-path.h" /* for text_to_path() */
#include "boundingbox.h"

#include "attributes.h" /* attributes_get_foreground() */

typedef enum {
  PATH_STROKE = (1<<0),
  PATH_FILL   = (1<<1)
} PathLastOp;

/*!
 * \brief Renderer which turns everything into a path (or a list thereof)
 *
 * The path renderer does not produce any external output by itself. It
 * strokes it's results only internally in one or more paths. These are
 * further processed by e.g. create_standard_path_from_object().
 *
 * \extends _DiaRenderer
 */
struct _DiaPathRenderer
{
  DiaRenderer parent_instance; /*!< inheritance in object oriented C */

  GPtrArray *pathes;

  Color stroke;
  Color fill;

  PathLastOp last_op;
};

struct _DiaPathRendererClass
{
  DiaRendererClass parent_class; /*!< the base class */
};


G_DEFINE_TYPE (DiaPathRenderer, dia_path_renderer, DIA_TYPE_RENDERER)

/*!
 * \brief Constructor
 * Initialize everything which needs something else than 0.
 * \memberof _DiaPathRenderer
 */
static void
dia_path_renderer_init (DiaPathRenderer *self)
{
  self->stroke = attributes_get_foreground ();
  self->fill = attributes_get_background ();
}

static void
_path_renderer_clear (DiaPathRenderer *self)
{
  if (self->pathes) {
    guint i;

    for (i = 0; i < self->pathes->len; ++i) {
      GArray *path = g_ptr_array_index (self->pathes, i);

      g_array_free (path, TRUE);
    }
    g_ptr_array_free (self->pathes, TRUE);
    self->pathes = NULL;
  }
}
/*!
 * \brief Destructor
 * If there are still paths left, deallocate them
 * \memberof _DiaPathRenderer
 */
static void
dia_path_renderer_finalize (GObject *object)
{
  DiaPathRenderer *self = DIA_PATH_RENDERER (object);

  _path_renderer_clear (self);
  G_OBJECT_CLASS (dia_path_renderer_parent_class)->finalize (object);
}

/*!
 * \brief Deliver the current path
 * To be advanced if we want to support more than one path, e.g. for multiple
 * color support.
 * @param self   explicit this pointer
 * @param stroke line color or NULL
 * @param fill   fill color or NULL
 * \private \memberof _DiaPathRenderer
 */
static GArray *
_get_current_path (DiaPathRenderer *self,
		   const Color     *stroke,
		   const Color     *fill)
{
  GArray *path;
  /* creating a new path for every new color */
  gboolean new_path = FALSE;

  if (stroke && memcmp (stroke, &self->stroke, sizeof(*stroke)) != 0) {
    memcpy (&self->stroke, stroke, sizeof(*stroke));
    new_path = TRUE;
  }
  if (fill  && memcmp (fill, &self->fill, sizeof(*fill)) != 0) {
    memcpy (&self->fill, fill, sizeof(*fill));
    new_path = TRUE;
  }
  /* also create a new path if the last op was a fill and we now stroke */
  if (   (stroke && self->last_op == PATH_FILL)
      || (fill && self->last_op == PATH_STROKE))
    new_path = TRUE;
  self->last_op = stroke ? PATH_STROKE : PATH_FILL;

  if (!self->pathes || new_path) {
    if (!self->pathes)
      self->pathes = g_ptr_array_new ();
    g_ptr_array_add (self->pathes, g_array_new (FALSE, FALSE, sizeof(BezPoint)));
  }
  path = g_ptr_array_index (self->pathes, self->pathes->len - 1);
  return path;
}
/*!
 * \brief Optimize away potential duplicated path
 * Dia's object rendering often consists of identical consecutive fill and stroke
 * operations. This function checks if two identical paths are at the end of our
 * list and just removes the second one.
 * \private \memberof _DiaPathRenderer
 */
static void
_remove_duplicated_path (DiaPathRenderer *self)
{
  if (self->pathes && self->pathes->len >= 2) {
    GArray *p1 = g_ptr_array_index (self->pathes, self->pathes->len - 2);
    GArray *p2 = g_ptr_array_index (self->pathes, self->pathes->len - 1);
    if (p1->len == p2->len) {
      gboolean same = TRUE;
      guint i;
      for (i = 0; i < p1->len; ++i) {
	const BezPoint *bp1 = &g_array_index (p1, BezPoint, i);
	const BezPoint *bp2 = &g_array_index (p2, BezPoint, i);

	same &= (bp1->type == bp2->type);
	same &= (memcmp (&bp1->p1, &bp2->p1, sizeof(Point)) == 0);
	if (bp1->type == BEZ_CURVE_TO) {
	  same &= (memcmp (&bp1->p2, &bp2->p2, sizeof(Point)) == 0);
	  same &= (memcmp (&bp1->p3, &bp2->p3, sizeof(Point)) == 0);
	}
      }
      if (same) {
	g_array_free (p2, TRUE);
	g_ptr_array_set_size (self->pathes, self->pathes->len - 1);
      }
    }
  }
}
/*!
 * \brief Starting a new rendering run
 * Could be used to clean the path leftovers from a previous run.
 * Typical export renderers flush here.
 * \memberof _DiaPathRenderer
 */
static void
begin_render (DiaRenderer *self, const DiaRectangle *update)
{
}
/*!
 * \brief End of current rendering run
 * Should not be used to clean the accumulated paths
 * \memberof _DiaPathRenderer
 */
static void
end_render(DiaRenderer *self)
{
}
static gboolean
is_capable_to (DiaRenderer *renderer, RenderCapability cap)
{
  if (RENDER_HOLES == cap)
    return TRUE;
  else if (RENDER_ALPHA == cap)
    return TRUE;
  return FALSE;
}
static void
set_linewidth(DiaRenderer *self, real linewidth)
{  /* 0 == hairline **/
}


static void
set_linecaps (DiaRenderer *self, DiaLineCaps mode)
{
}


static void
set_linejoin (DiaRenderer *self, DiaLineJoin mode)
{
}


static void
set_linestyle (DiaRenderer *self, DiaLineStyle mode, double dash_length)
{
}


static void
set_fillstyle (DiaRenderer *self, DiaFillStyle mode)
{
}

/*!
 * \brief Find the last point matching or add a new move-to
 *
 * Used to create as continuous paths, but still incomplete. If the the existing
 * and appended path would match in it's ends there would be a superfluous move-to.
 * Instead there probably should be a direction change with the newly appended path
 * segments to eliminate the extra move-to. The benefit would be that the resulting
 * path can be properly filled.
 */
static void
_path_append (GArray *points, const Point *pt)
{
  const BezPoint *prev = (points->len > 0) ? &g_array_index (points, BezPoint, points->len - 1) : NULL;
  const Point *last = prev ? (prev->type == BEZ_CURVE_TO ? &prev->p3 : &prev->p1) : NULL;

  if (!last || distance_point_point(last, pt) > 0.001) {
    BezPoint bp;
    bp.type = BEZ_MOVE_TO;
    bp.p1 = *pt;
    g_array_append_val (points, bp);
  }
}
static void
_path_moveto (GArray *path, const Point *pt)
{
  BezPoint bp;
  bp.type = BEZ_MOVE_TO;
  bp.p1 = *pt;
  g_array_append_val (path, bp);
}
static void
_path_lineto (GArray *path, const Point *pt)
{
  BezPoint bp;
  bp.type = BEZ_LINE_TO;
  bp.p1 = *pt;
  g_array_append_val (path, bp);
}
/*!
 * \brief Create an arc segment approximation
 * Code copied and adapted to our needs from _cairo_arc_segment()
 */
static void
_path_arc_segment (GArray      *path,
		   const Point *center,
		   real         radius,
		   real         angle_A,
		   real         angle_B)
{
  BezPoint bp;
  real r_sin_A, r_cos_A;
  real r_sin_B, r_cos_B;
  real h;

  r_sin_A = -radius * sin (angle_A);
  r_cos_A =  radius * cos (angle_A);
  r_sin_B = -radius * sin (angle_B);
  r_cos_B =  radius * cos (angle_B);

  h = 4.0/3.0 * tan ((angle_B - angle_A) / 4.0);

  bp.type = BEZ_CURVE_TO;
  bp.p1.x = center->x + r_cos_A + h * r_sin_A;
  bp.p1.y = center->y + r_sin_A - h * r_cos_A,
  bp.p2.x = center->x + r_cos_B - h * r_sin_B,
  bp.p2.y = center->y + r_sin_B + h * r_cos_B,
  bp.p3.x = center->x + r_cos_B;
  bp.p3.y = center->y + r_sin_B;

  g_array_append_val (path, bp);
}

/*!
 * \brief Create path from line
 * \memberof _DiaPathRenderer
 */
static void
draw_line(DiaRenderer *self,
	  Point *start, Point *end,
	  Color *line_colour)
{
  DiaPathRenderer *renderer = DIA_PATH_RENDERER (self);
  GArray *points = _get_current_path (renderer, line_colour, NULL);

  _path_append (points, start);
  _path_lineto (points, end);
}

static void
_polyline(DiaRenderer *self,
	  Point *points, int num_points,
	  const Color *fill, const Color *stroke)
{
  DiaPathRenderer *renderer = DIA_PATH_RENDERER (self);
  int i;
  GArray *path = _get_current_path (renderer, stroke, fill);

  g_return_if_fail (num_points > 1);

  if (stroke)
    _path_append (path, &points[0]);
  else
    _path_moveto (path, &points[0]);

  for (i = 1; i < num_points; ++i)
    _path_lineto (path, &points[i]);
}
/*!
 * \brief Create path from polyline
 * \memberof _DiaPathRenderer
 */
static void
draw_polyline(DiaRenderer *self,
	      Point *points, int num_points,
	      Color *line_colour)
{
  _polyline (self, points, num_points, NULL, line_colour);
  _remove_duplicated_path (DIA_PATH_RENDERER (self));
}
/*!
 * \brief Create path from polygon
 * \memberof _DiaPathRenderer
 */
static void
draw_polygon(DiaRenderer *self,
	      Point *points, int num_points,
	      Color *fill, Color *stroke)
{
  DiaPathRenderer *renderer = DIA_PATH_RENDERER (self);

  /* can't be that simple ;) */
  _polyline (self, points, num_points, fill, stroke);
  if (   points[0].x != points[num_points-1].x
      || points[0].y != points[num_points-1].y) {
    GArray *path = _get_current_path (renderer, stroke, NULL);
    _path_lineto (path, &points[0]);
  }
  /* usage of the optimized draw_polygon should imply this being superfluous */
  _remove_duplicated_path (renderer);
}
/*!
 * \brief Create path from rectangle
 * \memberof _DiaPathRenderer
 */
static void
draw_rect (DiaRenderer *self,
	   Point *ul_corner, Point *lr_corner,
	   Color *fill, Color *stroke)
{
  DiaPathRenderer *renderer = DIA_PATH_RENDERER (self);
  GArray *path = _get_current_path (renderer, stroke, fill);
  int i;
  real width  = lr_corner->x - ul_corner->x;
  real height = lr_corner->y - ul_corner->y;
  Point pt = *ul_corner;

  _path_moveto (path, &pt);

  /* 0: top-right, clock-wise */
  for (i = 0; i < 4; ++i) {
    BezPoint bp;

    bp.type = BEZ_LINE_TO;
    bp.p1.x = pt.x + (i < 2 ? width : 0);
    bp.p1.y = pt.y + (i > 0 && i < 3 ? height : 0);
    g_array_append_val (path, bp);
  }
}

/*!
 * \brief Create a path from an arc
 */
void
path_build_arc (GArray *path,
		Point *center,
		real width, real height,
		real angle1, real angle2,
		gboolean closed)
{
  Point start;
  real radius = sqrt(width * height) / 2.0;
  real ar1;
  real ar2;
  int i, segs;
  real ars;
  gboolean ccw = angle2 > angle1;

  ar1 = (M_PI / 180.0) * angle1;
  ar2 = (M_PI / 180.0) * angle2;
  /* one segment for ever 90 degrees */
  if (ccw) {
    segs = (int)((ar2 - ar1) / (M_PI/2)) + 1;
    ars = (ar2 - ar1) / segs;
  } else {
    segs = (int)((ar1 - ar2) / (M_PI/2)) + 1;
    ars = -(ar1 - ar2) / segs;
  }
  /* move to start point */
  start.x = center->x + (width / 2.0)  * cos(ar1);
  start.y = center->y - (height / 2.0) * sin(ar1);

  if (!closed) {
    _path_append (path, &start);
    for (i = 0; i < segs; ++i, ar1 += ars)
      _path_arc_segment (path, center, radius, ar1, ar1 + ars);
  } else {
    _path_moveto (path, &start);
    for (i = 0; i < segs; ++i, ar1 += ars)
      _path_arc_segment (path, center, radius, ar1, ar2);
    _path_lineto (path, center);
    _path_lineto (path, &start);
  }
}

/*!
 * \brief Convert an arc to some bezier curve-to
 * \private \memberof _DiaPathRenderer
 */
static void
_arc (DiaRenderer *self,
      Point *center,
      real width, real height,
      real angle1, real angle2,
      const Color *stroke, const Color *fill)
{
  DiaPathRenderer *renderer = DIA_PATH_RENDERER (self);
  GArray *path = _get_current_path (renderer, stroke, fill);

  path_build_arc (path, center, width, height, angle1, angle2, stroke == NULL);
}
/*!
 * \brief Create path from arc
 * \memberof _DiaPathRenderer
 */
static void
draw_arc (DiaRenderer *self,
	  Point *center,
	  real width, real height,
	  real angle1, real angle2,
	  Color *color)
{
  _arc (self, center, width, height, angle1, angle2, color, NULL);
  _remove_duplicated_path (DIA_PATH_RENDERER (self));
}
/*!
 * \brief Create path from closed arc
 * \memberof _DiaPathRenderer
 */
static void
fill_arc (DiaRenderer *self,
	  Point *center,
	  real width, real height,
	  real angle1, real angle2,
	  Color *color)
{
  _arc (self, center, width, height, angle1, angle2, NULL, color);
}
void
path_build_ellipse (GArray *path,
		    Point *center,
		    real width, real height)
{
  real w2 = width/2;
  real h2 = height/2;
  /* FIXME: just a rough estimation to get started */
  real dx = w2 * 0.55;
  real dy = h2 * 0.55;
  Point pt;
  int i;

  pt = *center;
  pt.y -= h2;
  _path_moveto (path, &pt);
  for (i = 0; i < 4; ++i) {
    BezPoint bp;

    /* i=0 is the right-most point, than going clock-wise */
    pt.x = (i % 2 == 1 ? center->x : (i == 0 ? center->x + w2 : center->x -w2));
    pt.y = (i % 2 == 0 ? center->y : (i == 1 ? center->y + h2 : center->y - h2));
    bp.type = BEZ_CURVE_TO;
    bp.p3 = pt;

    switch (i) {
    case 0 : /* going right for p1 */
      bp.p1.x = center->x + dx;
      bp.p1.y = center->y - h2;
      bp.p2.x = pt.x;
      bp.p2.y = pt.y - dy;
      break;
    case 1 : /* going down for p1 */
      bp.p1.x = center->x + w2;
      bp.p1.y = center->y + dy;
      bp.p2.x = pt.x + dx;
      bp.p2.y = pt.y;
      break;
    case 2 : /* going left for p1 */
      bp.p1.x = center->x - dx;
      bp.p1.y = center->y + h2;
      bp.p2.x = pt.x;
      bp.p2.y = pt.y + dy;
      break;
    case 3 : /* going up for p1 */
      bp.p1.x = center->x - w2;
      bp.p1.y = center->y - dy;
      bp.p2.x = pt.x - dx;
      bp.p2.y = pt.y;
      break;
    default :
      g_assert_not_reached ();
    }

    g_array_append_val (path, bp);
  }
}
/*!
 * \brief Create path from ellipse
 * \memberof _DiaPathRenderer
 */
static void
draw_ellipse (DiaRenderer *self,
	      Point *center,
	      real width, real height,
	      Color *fill, Color *stroke)
{
  DiaPathRenderer *renderer = DIA_PATH_RENDERER (self);
  GArray *path = _get_current_path (renderer, stroke, fill);

  path_build_ellipse (path, center, width, height);
}
static void
_bezier (DiaRenderer *self,
	 BezPoint *points, int numpoints,
	 const Color *fill, const Color *stroke)
{
  DiaPathRenderer *renderer = DIA_PATH_RENDERER (self);
  GArray *path = _get_current_path (renderer, stroke, fill);
  int i = 0;

  /* get rid of the first move-to if we can attach to the previous point */
  if (path->len > 0) {
    BezPoint *bp = &g_array_index (path, BezPoint, path->len-1);
    Point *pt = (BEZ_CURVE_TO == bp->type) ? &bp->p3 : &bp->p1;
    if (distance_point_point(pt, &points[0].p1) < 0.001)
      i = 1;
  }
  for (; i < numpoints; ++i)
    g_array_append_val (path, points[i]);
}
/*!
 * \brief Create path from bezier line
 * \memberof _DiaPathRenderer
 */
static void
draw_bezier (DiaRenderer *self,
	     BezPoint *points,
	     int numpoints,
	     Color *color)
{
  _bezier(self, points, numpoints, NULL, color);
  _remove_duplicated_path (DIA_PATH_RENDERER (self));
}
/*!
 * \brief Create path from bezier polygon
 * \memberof _DiaPathRenderer
 */
static void
draw_beziergon (DiaRenderer *self,
		BezPoint *points,
		int numpoints,
		Color *fill,
		Color *stroke)
{
  _bezier(self, points, numpoints, fill, stroke);
}
/*!
 * \brief Convert the text object to a scaled path
 * \memberof _DiaPathRenderer
 */
static void
draw_text (DiaRenderer *self,
	   Text        *text)
{
  DiaPathRenderer *renderer = DIA_PATH_RENDERER (self);
  GArray *path = _get_current_path (renderer, NULL, &text->color);
  int n0 = path->len;

  if (!text_is_empty (text) && text_to_path (text, path)) {
    DiaRectangle bz_bb, tx_bb;
    PolyBBExtras extra = { 0, };
    real dx, dy, sx, sy;
    guint i;

    polybezier_bbox (&g_array_index (path, BezPoint, n0), path->len - n0, &extra, TRUE, &bz_bb);
    text_calc_boundingbox (text, &tx_bb);
    sx = (tx_bb.right - tx_bb.left) / (bz_bb.right - bz_bb.left);
    sy = (tx_bb.bottom - tx_bb.top) / (bz_bb.bottom - bz_bb.top);
    dx = tx_bb.left - bz_bb.left * sx;
    dy = tx_bb.top - bz_bb.top * sy;

    for (i = n0; i < path->len; ++i) {
      BezPoint *bp = &g_array_index (path, BezPoint, i);

      bp->p1.x = bp->p1.x * sx + dx;
      bp->p1.y = bp->p1.y * sy + dy;
      if (bp->type != BEZ_CURVE_TO)
        continue;
      bp->p2.x = bp->p2.x * sx + dx;
      bp->p2.y = bp->p2.y * sy + dy;
      bp->p3.x = bp->p3.x * sx + dx;
      bp->p3.y = bp->p3.y * sy + dy;
    }
  }
}


/*
 * Convert the string back to a #Text object and render that
 */
static void
draw_string (DiaRenderer  *self,
             const char   *text,
             Point        *pos,
             DiaAlignment  alignment,
             Color        *color)
{
  if (text && strlen (text)) {
    Text *text_obj;
    DiaFont *font;
    double font_height;

    font = dia_renderer_get_font (self, &font_height);
    /* it could have been so easy without the context switch */
    text_obj = new_text (text,
                         font,
                         font_height,
                         pos,
                         color,
                         alignment);
    draw_text (self, text_obj);
    text_destroy (text_obj);
  }
}

/*!
 * \brief Render just a cheap emulation ;)
 *
 * It might be desirable to convert the given pixels to some vector representation.
 * If simple and fast enough that code could even live in Dia's repository. For now
 * just rendering the images bounding box has to be enough.
 *
 * \memberof _DiaPathRenderer
 */
static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage *image)
{
  DiaPathRenderer *renderer = DIA_PATH_RENDERER (self);
  /* warning colors ;) */
  Color stroke = { 1.0, 0.0, 0.0, 0.75 };
  Color fill = { 1.0, 1.0, 0.0, 0.5 };
  GArray *path = _get_current_path (renderer, &stroke, &fill);
  Point to = *point;

  _path_moveto (path, &to);
  to.x += width;
  _path_lineto (path, &to);
  to.y += height;
  _path_lineto (path, &to);
  to.x -= width;
  _path_lineto (path, &to);
  to.y -= height;
  _path_lineto (path, &to);
  to.x += width; to.y += height;
  _path_lineto (path, &to);
}

/*!
 * \brief Create contour of the rounded rect
 *
 * This methods needs to be implemented to avoid the default
 * implementation mixing calls of draw_arc, drar_line and fill_arc.
 * We still use the default but only method but only for the outline.
 *
 * \memberof _DiaPathRenderer
 */
static void
draw_rounded_rect (DiaRenderer *self,
		   Point *ul_corner, Point *lr_corner,
		   Color *fill, Color *stroke, real radius)
{
  DiaPathRenderer *renderer = DIA_PATH_RENDERER (self);
  real rx = (lr_corner->x - ul_corner->x) / 2;
  real ry = (lr_corner->y - ul_corner->y) / 2;
  /* limit radius to fit */
  if (radius > rx)
    radius = rx;
  if (radius > ry)
    radius = ry;
  if (stroke) /* deliberately ignoring fill for path building */
    DIA_RENDERER_CLASS(dia_path_renderer_parent_class)->draw_rounded_rect(self,
									  ul_corner, lr_corner,
									  NULL, stroke, radius);
  else
    DIA_RENDERER_CLASS(dia_path_renderer_parent_class)->draw_rounded_rect(self,
									  ul_corner, lr_corner,
									  fill, NULL, radius);
  /* stroke is set by the piecewise rendering above already */
  if (fill)
    renderer->fill = *fill;
}

/*!
 * \brief _DiaPathRenderer class initialization
 * Overwrite methods of the base classes here.
 */
static void
dia_path_renderer_class_init (DiaPathRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  object_class->finalize = dia_path_renderer_finalize;

  /* renderer members */
  renderer_class->begin_render = begin_render;
  renderer_class->end_render   = end_render;

  renderer_class->set_linewidth  = set_linewidth;
  renderer_class->set_linecaps   = set_linecaps;
  renderer_class->set_linejoin   = set_linejoin;
  renderer_class->set_linestyle  = set_linestyle;
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->draw_line    = draw_line;
  renderer_class->draw_polygon = draw_polygon;
  renderer_class->draw_rect    = draw_rect;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;

  renderer_class->draw_string  = draw_string;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_polyline  = draw_polyline;

  renderer_class->draw_bezier    = draw_bezier;
  renderer_class->draw_beziergon = draw_beziergon;
  renderer_class->draw_text      = draw_text;
  /* highest level function */
  renderer_class->draw_rounded_rect = draw_rounded_rect;
  /* other */
  renderer_class->is_capable_to = is_capable_to;
}

#include "object.h"
#include "create.h"
#include "group.h"
#include "path-math.h" /* path_combine() */

/*!
 * \brief Convert an object to a _StdPath by rendering it with _DiaPathRenderer
 *
 * The result is either a single _SdtPath or a _Group of _Stdpath depending on
 * the criteria implemented in _get_current_path() and of course the content of
 * the given object.
 */
DiaObject *
create_standard_path_from_object (DiaObject *obj)
{
  DiaObject *path;
  DiaRenderer *renderer;
  DiaPathRenderer *pr;

  g_return_val_if_fail (obj != NULL, NULL);

  renderer = g_object_new (DIA_TYPE_PATH_RENDERER, 0);
  pr = DIA_PATH_RENDERER (renderer);

  dia_object_draw (obj, renderer);

  /* messing with internals */
  if (!pr->pathes) { /* oops? */
    path = NULL;
  } else if (pr->pathes->len == 1) {
    GArray *points = g_ptr_array_index (pr->pathes, 0);
    if (points->len < 2)
      path = NULL;
    else
      path = create_standard_path (points->len, &g_array_index (points, BezPoint, 0));
  } else {
    /* create a group of pathes */
    GList *list = NULL;
    gsize i;

    for (i = 0; i < pr->pathes->len; ++i) {
      GArray *points = g_ptr_array_index (pr->pathes, i);
      DiaObject *path_obj;
      if (points->len < 2)
        path_obj = NULL;
      else
        path_obj = create_standard_path (points->len, &g_array_index (points, BezPoint, 0));
      if (path_obj)
        list = g_list_append (list, path_obj);
    }
    if (!list) {
      path = NULL;
    } else if (g_list_length (list) == 1) {
      path = list->data;
      g_list_free (list);
    } else {
      /* group_create eating list */
      path = group_create (list);
    }
  }
  g_clear_object (&renderer);

  return path;
}

/*!
 * \brief Combine the list of object into a new path object
 *
 * Every single object get into a single path which gets combined
 * with the following object. The result of that operation than gets
 * combined again with the following object.
 */
DiaObject *
create_standard_path_from_list (GList           *objects,
				PathCombineMode  mode)
{
  DiaObject *path;
  DiaRenderer *renderer;
  DiaPathRenderer *pr;
  GList *list;
  GArray *p1 = NULL, *p2 = NULL;

  renderer = g_object_new (DIA_TYPE_PATH_RENDERER, 0);
  pr = DIA_PATH_RENDERER (renderer);

  for (list = objects; list != NULL; list = list->next) {
    DiaObject *obj = list->data;
    int i;

    _path_renderer_clear (pr);
    dia_object_draw (obj, renderer);
    if (!pr->pathes) /* nothing created? */
      continue;
    /* get a single path from this rendererer run */
    p2 = g_array_new (FALSE, FALSE, sizeof(BezPoint));
    for (i = 0; i < pr->pathes->len; ++i) {
      GArray *points = g_ptr_array_index (pr->pathes, i);
      g_array_append_vals (p2, &g_array_index (points, BezPoint, 0), points->len);
    }
    if (p1 && p2) {
      GArray *combined = path_combine (p1, p2, mode);
      /* combined == NULL is just passed on */
      g_array_free (p1, TRUE);
      p1 = combined;
      g_array_free (p2, TRUE);
      p2 = NULL;
    } else {
      p1 = p2;
      p2 = NULL;
    }
  }
  if (!p1)
    return NULL;
  path = create_standard_path (p1->len, &g_array_index (p1, BezPoint, 0));
  /* copy style from first object processed */
  object_copy_style (path, (DiaObject *)objects->data);
  g_array_free (p1, TRUE);
  return path;
}

