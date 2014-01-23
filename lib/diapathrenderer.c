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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#include "config.h"

#include "diapathrenderer.h"
#include "text.h" /* just for text->color */
#include "standard-path.h" /* for text_to_path() */
#include "boundingbox.h"

#include "attributes.h" /* attributes_get_foreground() */

/*!
 * \brief Renderer which turns everything into a path (or a list thereof)
 *
 * The path renderer does not produce any external output by itself. It
 * stroes it's results only internally in one or more pathes. These are
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
};

struct _DiaPathRendererClass
{
  DiaRendererClass parent_class; /*!< the base class */
};


G_DEFINE_TYPE (DiaPathRenderer, dia_path_renderer, DIA_TYPE_RENDERER)

/*!
 * \brief Constructor
 * Initialize everything which needs something else than 0.
 */
static void
dia_path_renderer_init (DiaPathRenderer *self)
{
  self->stroke = attributes_get_foreground ();
  self->fill = attributes_get_background ();
}
/*!
 * \brief Destructor
 * If there are still pathes left, deallocate them
 */
static void
dia_path_renderer_finalize (GObject *object)
{
  DiaPathRenderer *self = DIA_PATH_RENDERER (object);

  if (self->pathes) {
    guint i;
   
    for (i = 0; i < self->pathes->len; ++i) {
      GArray *path = g_ptr_array_index (self->pathes, i);

      g_array_free (path, TRUE);
    }
    g_ptr_array_free (self->pathes, TRUE);
    self->pathes = NULL;
  }
  G_OBJECT_CLASS (dia_path_renderer_parent_class)->finalize (object);
}

/*!
 * \brief Deliver the current path
 * To be advanced if we want to support more than one path, e.g. for multiple
 * color support.
 * @param self   explicit this pointer
 * @param stroke line color or NULL
 * @param fill   fill color or NULL
 * \private \memeberof _DiaPathRenderer
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

  if (!self->pathes || new_path) {
    if (!self->pathes)
      self->pathes = g_ptr_array_new ();
    g_ptr_array_add (self->pathes, g_array_new (FALSE, FALSE, sizeof(BezPoint)));
  }
  path = g_ptr_array_index (self->pathes, self->pathes->len - 1);
  return path;
}
/*!
 * \brief Starting a new rendering run
 * Could be used to clean the path leftovers from a previous run.
 * Typical export renderers flush here.
 */
static void
begin_render (DiaRenderer *self, const Rectangle *update)
{
}
/*!
 * \brief End of current rendering run
 * Should not be used to clean the accumulated pathes 
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
set_linecaps(DiaRenderer *self, LineCaps mode)
{
}
static void
set_linejoin(DiaRenderer *self, LineJoin mode)
{
}
static void
set_linestyle(DiaRenderer *self, LineStyle mode)
{
}
static void
set_dashlength(DiaRenderer *self, real length)
{  /* dot = 20% of len */
}
static void
set_fillstyle(DiaRenderer *self, FillStyle mode)
{
}

/*!
 * \brief Find the last point matching or add a new move-to
 *
 * Used to create as continuous pathes, but still incomplete. If the the exisiting
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

  if (!last || last->x != pt->x || last->y != pt->y) {
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

  r_sin_A = radius * sin (angle_A);
  r_cos_A = radius * cos (angle_A);
  r_sin_B = radius * sin (angle_B);
  r_cos_B = radius * cos (angle_B);

  h = 4.0/3.0 * tan ((angle_B - angle_A) / 4.0);
  
  bp.type = BEZ_CURVE_TO;
  bp.p1.x = center->x + r_cos_A - h * r_sin_A;
  bp.p1.y = center->y + r_sin_A + h * r_cos_A,
  bp.p2.x = center->x + r_cos_B + h * r_sin_B,
  bp.p2.y = center->y + r_sin_B - h * r_cos_B,
  bp.p3.x = center->x + r_cos_B;
  bp.p3.y = center->y + r_sin_B;

  g_array_append_val (path, bp);
}

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
	  const Color *stroke, const Color *fill)
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
static void
draw_polyline(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
  _polyline (self, points, num_points, line_colour, NULL);
}
static void
draw_polygon(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
  DiaPathRenderer *renderer = DIA_PATH_RENDERER (self);
  GArray *path = _get_current_path (renderer, line_colour, NULL);

  /* FIXME: can't be that simple ;) */
  _polyline (self, points, num_points, line_colour, NULL);
  _path_lineto (path, &points[0]);
}
static void
fill_polygon(DiaRenderer *self, 
	     Point *points, int num_points, 
	     Color *color)
{
  DiaPathRenderer *renderer = DIA_PATH_RENDERER (self);
  GArray *path = _get_current_path (renderer, NULL, color);

  _polyline (self, points, num_points, NULL, color);
  _path_lineto (path, &points[0]);
}
static void
_rect (DiaRenderer *self, 
       Point *ul_corner, Point *lr_corner,
       const Color *stroke, const Color *fill)
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
static void
draw_rect (DiaRenderer *self, 
	   Point *ul_corner, Point *lr_corner,
	   Color *color)
{
  _rect (self, ul_corner, lr_corner, color, NULL);
}
static void
fill_rect (DiaRenderer *self, 
	   Point *ul_corner, Point *lr_corner,
	   Color *color)
{
  _rect (self, ul_corner, lr_corner, NULL, color);
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

  while (angle1 > angle2)
    angle1 -= 360;

  ar1 = (M_PI / 180.0) * angle2;
  ar2 = (M_PI / 180.0) * angle1;
  /* one segment for ever 90 degrees */
  segs = (int)(fabs(ar2 - ar1) / (M_PI/2)) + 1;
  ars = - (ar2 - ar1) / segs;

  /* move to start point */
  start.x = center->x + (width / 2.0)  * cos(ar1);
  start.y = center->y - (height / 2.0) * sin(ar1);

  /* Dia and Cairo don't agree on arc definitions, so it needs
   * to be converted, i.e. mirrored at the x axis
   */
  ar1 = - ar1;
  ar2 = - ar2;
  while (ar1 < 0 || ar2 < 0) {
    ar1 += 2 * M_PI;
    ar2 += 2 * M_PI;
  }

  if (!closed) {
    _path_append (path, &start);
    for (i = 0; i < segs; ++i, ar1 += ars)
      _path_arc_segment (path, center, radius, ar1, ar1 + ars);
  } else {
    _path_moveto (path, &start);
    _path_arc_segment (path, center, radius, ar1, ar2);
    _path_lineto (path, center);
    _path_lineto (path, &start);
  }
}

/*!
 * \brief Convert an arc to some bezier curve-to
 * \bug For arcs going through angle 0 the result is wrong, 
 * kind of the opposite of the desired.
 * \protected \memberof _DiaPathRenderer
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
static void
draw_arc (DiaRenderer *self, 
	  Point *center,
	  real width, real height,
	  real angle1, real angle2,
	  Color *color)
{
  _arc (self, center, width, height, angle1, angle2, color, NULL);
}
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
static void
_ellipse (DiaRenderer *self,
	  Point *center,
	  real width, real height,
	  const Color *stroke, const Color *fill)
{
  DiaPathRenderer *renderer = DIA_PATH_RENDERER (self);
  GArray *path = _get_current_path (renderer, stroke, fill);

  path_build_ellipse (path, center, width, height);
}
static void
draw_ellipse (DiaRenderer *self, 
	      Point *center,
	      real width, real height,
	      Color *color)
{
  _ellipse (self, center, width, height, color, NULL);
}
static void
fill_ellipse (DiaRenderer *self, 
	      Point *center,
	      real width, real height,
	      Color *color)
{
  _ellipse (self, center, width, height, NULL, color);
}
static void
_bezier (DiaRenderer *self, 
	 BezPoint *points, int numpoints,
	 const Color *stroke, const Color *fill)
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
  if (fill)
    _path_lineto (path, &points[0].p1);
}
static void
draw_bezier (DiaRenderer *self, 
	     BezPoint *points,
	     int numpoints,
	     Color *color)
{
  _bezier(self, points, numpoints, color, NULL);
}
static void
fill_bezier(DiaRenderer *self, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *color)
{
  _bezier(self, points, numpoints, NULL, color);
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
    Rectangle bz_bb, tx_bb;
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

/*!
 * \brief Convert the string back to a _Text object and render that
 * \memberof _DiaPathRenderer
 */
static void
draw_string(DiaRenderer *self,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
  if (text && strlen(text)) {
    Text *text_obj;
    /* it could have been so easy without the context switch */
    text_obj = new_text (text,
			 self->font, self->font_height,
			 pos, color, alignment);
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
  renderer_class->set_dashlength = set_dashlength;
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->draw_line    = draw_line;
  renderer_class->fill_polygon = fill_polygon;
  renderer_class->draw_rect    = draw_rect;
  renderer_class->fill_rect    = fill_rect;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;
  renderer_class->fill_ellipse = fill_ellipse;

  renderer_class->draw_string  = draw_string;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_rect = draw_rect;
  renderer_class->draw_polyline  = draw_polyline;
  renderer_class->draw_polygon   = draw_polygon;

  renderer_class->draw_bezier   = draw_bezier;
  renderer_class->fill_bezier   = fill_bezier;
  renderer_class->draw_text     = draw_text;
  /* other */
  renderer_class->is_capable_to = is_capable_to;
}

#include "object.h"
#include "create.h"
#include "group.h"

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

  obj->ops->draw (obj, renderer);

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
      DiaObject *obj;

      if (points->len < 2)
	obj = NULL;
      else
        obj = create_standard_path (points->len, &g_array_index (points, BezPoint, 0));
      if (obj)
        list = g_list_append (list, obj);
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
  g_object_unref (renderer);

  return path;
}
