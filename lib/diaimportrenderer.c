/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diaimportrenderer.c - Dia renderer class creating DiaObject(s)
 *
 * Copyright (C) 2014 Hans Breuer
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

#include "diaimportrenderer.h"
#include "object.h"
#include "text.h"
#include "create.h"
#include "properties.h"
#include "dia_image.h"
#include "diatransformrenderer.h"
#include "diapathrenderer.h"

static void begin_render (DiaRenderer *, const DiaRectangle *update);
static void end_render (DiaRenderer *);

static void set_linewidth (DiaRenderer *renderer, real linewidth);
static void set_linecaps  (DiaRenderer *renderer, DiaLineCaps  mode);
static void set_linejoin  (DiaRenderer *renderer, DiaLineJoin  mode);
static void set_linestyle (DiaRenderer *renderer, DiaLineStyle mode, double dash_length);
static void set_fillstyle (DiaRenderer *renderer, DiaFillStyle mode);

static void draw_line (DiaRenderer *renderer,
		       Point *start, Point *end,
		       Color *color);
static void draw_rect (DiaRenderer *renderer,
		       Point *ul_corner, Point *lr_corner,
		       Color *fill, Color *stroke);
static void draw_arc (DiaRenderer *renderer,
		      Point *center,
		      real width, real height,
		      real angle1, real angle2,
		      Color *color);
static void fill_arc (DiaRenderer *renderer,
		      Point *center,
		      real width, real height,
		      real angle1, real angle2,
		      Color *color);
static void draw_ellipse (DiaRenderer *renderer,
			  Point *center,
			  real width, real height,
			  Color *fill, Color *stroke);
static void draw_bezier (DiaRenderer *renderer,
			 BezPoint *points,
			 int numpoints,
			 Color *color);
static void draw_beziergon (DiaRenderer *renderer,
			    BezPoint *points,
			    int numpoints,
			    Color *fill,
			    Color *stroke);
static void draw_string            (DiaRenderer  *renderer,
                                    const char   *text,
                                    Point        *pos,
                                    DiaAlignment  alignment,
                                    Color        *color);
static void draw_image (DiaRenderer *renderer,
			Point *point,
			real width, real height,
			DiaImage *image);

static void draw_polyline (DiaRenderer *renderer,
                           Point *points, int num_points,
                           Color *color);
static void draw_rounded_polyline (DiaRenderer *renderer,
				   Point *points, int num_points,
				   Color *color, real radius);
static void draw_polygon (DiaRenderer *renderer,
			  Point *points, int num_points,
			  Color *fill, Color *stroke);

static void draw_rounded_rect (DiaRenderer *renderer,
			       Point *ul_corner, Point *lr_corner,
			       Color *fill, Color *stroke, real radius);

static void draw_line_with_arrows  (DiaRenderer *renderer,
				    Point *start, Point *end,
				    real line_width,
				    Color *line_color,
				    Arrow *start_arrow,
				    Arrow *end_arrow);
static void draw_arc_with_arrows  (DiaRenderer *renderer,
				   Point *start,
				   Point *end,
				   Point *midpoint,
				   real line_width,
				   Color *color,
				   Arrow *start_arrow,
				   Arrow *end_arrow);
static void draw_polyline_with_arrows (DiaRenderer *renderer,
				       Point *points, int num_points,
				       real line_width,
				       Color *color,
				       Arrow *start_arrow,
				       Arrow *end_arrow);
static void draw_rounded_polyline_with_arrows (DiaRenderer *renderer,
					       Point *points, int num_points,
					       real line_width,
					       Color *color,
					       Arrow *start_arrow,
					       Arrow *end_arrow,
					       real radius);

static void draw_bezier_with_arrows (DiaRenderer *renderer,
				     BezPoint *points,
				     int num_points,
				     real line_width,
				     Color *color,
				     Arrow *start_arrow,
				     Arrow *end_arrow);

static gboolean is_capable_to (DiaRenderer *renderer, RenderCapability cap);

static void set_pattern (DiaRenderer *renderer, DiaPattern *pat);

static gpointer parent_class = NULL;

G_DEFINE_TYPE(DiaImportRenderer, dia_import_renderer, DIA_TYPE_RENDERER);

static void
renderer_finalize (GObject *object)
{
  DiaRenderer *renderer = DIA_RENDERER (object);
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);

  /* anything more to destroy? */
  if (self->objects) {
    /* if no one called dia_import_renderer_get_objects() */
    destroy_object_list(self->objects);
    self->objects = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
dia_import_renderer_class_init (DiaImportRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = renderer_finalize;

  renderer_class->begin_render = begin_render;
  renderer_class->end_render   = end_render;

  renderer_class->set_linewidth  = set_linewidth;
  renderer_class->set_linecaps   = set_linecaps;
  renderer_class->set_linejoin   = set_linejoin;
  renderer_class->set_linestyle  = set_linestyle;
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->draw_line    = draw_line;
  renderer_class->draw_rect    = draw_rect;
  renderer_class->draw_polygon = draw_polygon;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;
  renderer_class->draw_string  = draw_string;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_bezier  = draw_bezier;
  renderer_class->draw_beziergon  = draw_beziergon;
  renderer_class->draw_rounded_polyline  = draw_rounded_polyline;
  renderer_class->draw_polyline  = draw_polyline;

  /* highest level functions */
  renderer_class->draw_rounded_rect = draw_rounded_rect;
  renderer_class->draw_line_with_arrows = draw_line_with_arrows;
  renderer_class->draw_arc_with_arrows  = draw_arc_with_arrows;
  renderer_class->draw_polyline_with_arrows = draw_polyline_with_arrows;
  renderer_class->draw_rounded_polyline_with_arrows = draw_rounded_polyline_with_arrows;
  renderer_class->draw_bezier_with_arrows = draw_bezier_with_arrows;

  /* other */
  renderer_class->is_capable_to = is_capable_to;
  renderer_class->set_pattern = set_pattern;
}


static void
dia_import_renderer_init (DiaImportRenderer *self)
{
  /* set all (non-null) defaults */
  self->line_style = DIA_LINE_STYLE_SOLID;
  self->dash_length = 1.0;
  self->line_caps = DIA_LINE_CAPS_BUTT;
  self->line_join = DIA_LINE_JOIN_MITER;

  self->objects = NULL;
}


static void
begin_render (DiaRenderer *renderer, const DiaRectangle *update)
{
  g_warning ("%s::begin_render not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
}

static void
end_render (DiaRenderer *renderer)
{
  g_warning ("%s::end_render not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
}

static void
set_linewidth (DiaRenderer *renderer, real linewidth)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  self->line_width = linewidth;
}


static void
set_linecaps (DiaRenderer *renderer, DiaLineCaps mode)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  self->line_caps = mode;
}


static void
set_linejoin (DiaRenderer *renderer, DiaLineJoin mode)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  self->line_join = mode;
}


static void
set_linestyle (DiaRenderer *renderer, DiaLineStyle mode, double dash_length)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  self->line_style = mode;
  self->dash_length = dash_length;
}


static void
set_fillstyle (DiaRenderer *renderer, DiaFillStyle mode)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  self->fill_style = mode;
}

/*!
 * \brief Apply the current renderer's style to the given object
 * \memberof _DiaImportRenderer \private
 */
static void
_apply_style (DiaImportRenderer *self,
	      DiaObject *obj,
	      const Color *fill,
	      const Color *stroke,
	      real radius)
{
  GPtrArray *props = g_ptr_array_new ();

  prop_list_add_line_width (props, self->line_width);
  if (fill) {
    prop_list_add_fill_colour (props, fill);
    prop_list_add_show_background (props, TRUE);
  } else {
    prop_list_add_show_background (props, FALSE);
  }
  if (stroke) {
    prop_list_add_line_style (props, self->line_style, self->dash_length);
    /* XXX: line_join, line_caps */
    prop_list_add_line_colour (props, stroke);
  } else if (fill) {
    /* line width to 0, line color to fill */
    prop_list_add_line_width (props, 0.0);
    prop_list_add_line_colour (props, fill);
  }
  if (radius > 0.0) {
    prop_list_add_real (props, "corner_radius", radius);
  }
  dia_object_set_properties (obj, props);
  prop_list_free (props);
}
/*!
 * \brief Remember the object for later use
 * \memberof _DiaImportRenderer \private
 */
static void
_push_object (DiaImportRenderer *self, DiaObject *obj)
{
  self->objects = g_list_append (self->objects, obj);
}

/*!
 * \brief Draw a simple line
 * \memberof _DiaImportRenderer
 */
static void
draw_line (DiaRenderer *renderer, Point *start, Point *end, Color *color)
{
  Point points[2];
  points[0] = *start;
  points[1] = *end;
  draw_rounded_polyline (renderer, &points[0], 2, color, 0.0);
}

static DiaObject *
_make_arc (Point *center,
          real width, real height,
	  real angle1, real angle2)
{
  DiaObject *object;
  Point st, en;
  real rx = width / 2, ry = height / 2;
  real r = sqrt (rx*ry);
  real a, b, d;
  /* handle direction information
   *  - the arc being bigger than 180 => d>r!
   *  - angle2 < angle1 => d must be negative
   */
  gboolean big_arc = fabs(angle2-angle1) > 180.0;
  gboolean clockwise = angle2 < angle1;

  st.x = center->x + rx * cos(angle1*G_PI/180);
  st.y = center->y - ry * sin(angle1*G_PI/180);
  en.x = center->x + rx * cos(angle2*G_PI/180);
  en.y = center->y - ry * sin(angle2*G_PI/180);
  a = distance_point_point (&st, &en) / 2.0;
  if (a < r)
    b = sqrt(r*r - a*a); /* Pythagoras */
  else
    b = 0; /* half circle */
  if (big_arc)
    d = r + b;
  else
    d = r - b;
  if (clockwise)
    d = -d;
  object = create_standard_arc (st.x, st.y, en.x, en.y, d, NULL, NULL);
  return object;
}

/*!
 * \brief Draw an arc
 * \memberof _DiaImportRenderer
 */
static void
draw_arc (DiaRenderer *renderer, Point *center,
          real width, real height, real angle1, real angle2,
          Color *color)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  DiaObject *object = _make_arc (center, width, height, angle1, angle2);

  _apply_style (self, object, NULL, color, 0.0);
  _push_object (self, object);
}

/*!
 * \brief Stroke and/or fill a rectangle
 * \memberof _DiaImportRenderer
 */
static void
draw_rect (DiaRenderer *renderer,
           Point *ul_corner, Point *lr_corner,
           Color *fill, Color *stroke)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  DiaObject *object = create_standard_box (ul_corner->x, ul_corner->y,
					   lr_corner->x - ul_corner->x,
					   lr_corner->y - ul_corner->y);
  _apply_style (self, object, fill,stroke, 0.0);
  _push_object (self, object);
}

/*!
 * \brief Fill an arc
 * As of this writing the Dia "Standard - Arc" (_Arc) object does not support
 * filling. The created object is a filled _Beziergon because of this.
 * \memberof _DiaImportRenderer
 */
static void
fill_arc (DiaRenderer *renderer, Point *center,
          real width, real height, real angle1, real angle2,
          Color *color)
{
#if 0 /* does not work till 'Standard - Arc' supports filling */
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  DiaObject *object = _make_arc (center, width, height, angle1, angle2);

  _apply_style (self, object, color, NULL, 0.0);
  _push_object (self, object);
#else
  GArray *path = g_array_new (FALSE, FALSE, sizeof (BezPoint));
  path_build_arc (path, center, width, height, angle1, angle2, TRUE);
  dia_renderer_draw_beziergon (renderer,
                               &g_array_index (path, BezPoint, 0),
                               path->len,
                               color,
                               NULL);
  g_array_free (path, TRUE);
#endif
}


/*!
 * \brief Draw an ellipse
 * Creates a hollow _Ellipse object.
 * \memberof _DiaImportRenderer
 */
static void
draw_ellipse (DiaRenderer *renderer,
              Point       *center,
              double       width,
              double       height,
              Color       *fill,
              Color       *stroke)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  DiaObject *object = create_standard_ellipse (center->x - width / 2,
                                               center->y - height / 2,
                                               width,
                                               height);

  _apply_style (self, object, fill, stroke, 0.0);
  _push_object (self, object);
}


/*
 * Creates a #Text object with the properties given by #DiaImportRenderer
 * dia_renderer_set_font().
 */
static void
draw_string (DiaRenderer  *renderer,
             const char   *text,
             Point        *pos,
             DiaAlignment  alignment,
             Color        *color)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  DiaObject *object = create_standard_text (pos->x, pos->y);
  GPtrArray *props = g_ptr_array_new ();
  DiaFont *font;
  double font_height;

  font = dia_renderer_get_font (renderer, &font_height);

  prop_list_add_font (props, "text_font", font);
  prop_list_add_text_colour (props, color);
  prop_list_add_fontsize (props, PROP_STDNAME_TEXT_HEIGHT, font_height);
  prop_list_add_enum (props, "text_alignment", alignment);
  prop_list_add_text (props, "text", text); /* must be last! */

  dia_object_set_properties (object, props);
  prop_list_free (props);

  _push_object (self, object);
}

/*!
 * \brief Draw an image
 * Creates an \_Image object. Ownership of the passed in image is _not_ transferred,
 * but it's pixbuf is referenced. The caller is responsible to destroy the image
 * with dia_image_unref() when it is no longer needed.
 * \memberof _DiaImportRenderer
 */
static void
draw_image (DiaRenderer *renderer,
            Point *point, real width, real height,
            DiaImage *image)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  DiaObject *object = create_standard_image (point->x, point->y, width, height, "");
  GPtrArray *props = g_ptr_array_new ();

  prop_list_add_filename (props, "image_file", dia_image_filename (image));
  /* XXX: reset or merge border? */
  dia_object_set_properties (object, props);
  prop_list_free (props);

  dia_object_set_pixbuf (object, (GdkPixbuf *)dia_image_pixbuf (image));
  _push_object (self, object);
}

/*
 * medium level functions, implemented by the above
 */

/*!
 * \brief Draw a bezier line
 * Creates a _Bezierline object.
 * \memberof _DiaImportRenderer
 */
static void
draw_bezier (DiaRenderer *renderer,
             BezPoint *points, int numpoints,
             Color *color)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  DiaObject *object = create_standard_bezierline (numpoints, points, NULL, NULL);

  _apply_style (self, object, NULL, color, 0.0);
  _push_object (self, object);
}

/*!
 * \brief Draw/fill a bezier shape
 * Creates a _Beziergon object.
 * \memberof _DiaImportRenderer
 */
static void
draw_beziergon (DiaRenderer *renderer,
                BezPoint *points, int numpoints,
                Color *fill,
                Color *stroke)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  DiaObject *object;

  g_return_if_fail (numpoints > 2);
  object = create_standard_beziergon (numpoints, points);
  _apply_style (self, object, fill, stroke, 0.0);
  _push_object (self, object);
}

/*!
 * \brief Draw a polyline
 * Creates a _Polyline object.
 * \memberof _DiaImportRenderer
 */
static void
draw_polyline (DiaRenderer *renderer,
	       Point *points, int num_points,
	       Color *color)
{
  draw_rounded_polyline (renderer, points, num_points, color, 0.0);
}

/*!
 * \brief Create a polyline with optionally rounded corners
 * Creates a _Polyline object.
 * \memberof DiaImportRenderer
 */
static void
draw_rounded_polyline (DiaRenderer *renderer,
                        Point *points, int num_points,
                        Color *color, real radius)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  DiaObject *object = create_standard_polyline (num_points, points, NULL, NULL);
  _apply_style (self, object, NULL, color, radius);
  _push_object (self, object);
}

/*!
 * \brief Draw a polygon filled and/or stroked
 * Creates a _Polygon object.
 * \memberof _DiaImportRenderer
 */
static void
draw_polygon (DiaRenderer *renderer,
              Point *points, int num_points,
              Color *fill, Color *stroke)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  DiaObject *object = create_standard_polygon (num_points, points);
  _apply_style (self, object, fill, stroke, 0.0);
  _push_object (self, object);
}

/*!
 * \brief Draw a rectangle with rounded corners.
 * Creates a _Box object.
 * \memberof _DiaImportRenderer
 */
static void
draw_rounded_rect (DiaRenderer *renderer,
                   Point *ul_corner, Point *lr_corner,
                   Color *fill, Color *stroke, real radius)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  DiaObject *object = create_standard_box (ul_corner->x, ul_corner->y,
					   lr_corner->x - ul_corner->x,
					   lr_corner->y - ul_corner->y);
  _apply_style (self, object, fill, stroke, radius);
  _push_object (self, object);
}

/*!
 * \brief Draw a line with optional arrows.
 * Creates a _Polyine object.
 * \memberof _DiaImportRenderer
 */
static void
draw_line_with_arrows(DiaRenderer *renderer,
                      Point *startpoint,
                      Point *endpoint,
                      real line_width,
                      Color *color,
                      Arrow *start_arrow,
                      Arrow *end_arrow)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  DiaObject *object;
  Point points[2];
  points[0] = *startpoint;
  points[1] = *endpoint;
  object = create_standard_polyline (2, points, start_arrow, end_arrow);
  _apply_style (self, object, NULL, color, 0.0);
  _push_object (self, object);
}

/*!
 * \brief Draw a polyline with optional arrows.
 * Creates a _Polyline object.
 * \memberof _DiaImportRenderer
 */
static void
draw_polyline_with_arrows(DiaRenderer *renderer,
                          Point *points, int num_points,
                          real line_width,
                          Color *color,
                          Arrow *start_arrow,
                          Arrow *end_arrow)
{
  draw_rounded_polyline_with_arrows(renderer, points, num_points,
				    line_width, color,
				    start_arrow, end_arrow,
				    0.0);
}

/*!
 * \brief Draw a polyline with rounded corners and optional arrows.
 * Creates a _Polyline object.
 * \memberof _DiaImportRenderer
 */
static void
draw_rounded_polyline_with_arrows(DiaRenderer *renderer,
				  Point *points, int num_points,
				  real line_width,
				  Color *color,
				  Arrow *start_arrow,
				  Arrow *end_arrow,
				  real radius)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  DiaObject *object = create_standard_polyline (num_points, points,
						start_arrow, end_arrow);
  _apply_style (self, object, NULL, color, radius);
  _push_object (self, object);
}

/*!
 * \brief Draw an arc with optional arrows.
 * Creates a _Arc object.
 * \memberof _DiaImportRenderer
 */
static void
draw_arc_with_arrows (DiaRenderer *renderer,
                      Point *startpoint,
                      Point *endpoint,
                      Point *midpoint,
                      real line_width,
                      Color *color,
                      Arrow *start_arrow,
                      Arrow *end_arrow)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  DiaObject *object;
  Point chord_point = { (startpoint->x + endpoint->x) / 2.0,
			(startpoint->y + endpoint->y) / 2.0 };
  real d = distance_point_point (&chord_point, midpoint);
  object = create_standard_arc (startpoint->x, startpoint->y,
				endpoint->x, endpoint->y,
				d, start_arrow, end_arrow);

  _apply_style (self, object, NULL, color, 0.0);
  _push_object (self, object);
}

/*!
 * \brief Draw a bezier line with optional arrows.
 * Creates a _Bezierline object.
 * \memberof _DiaImportRenderer
 */
static void
draw_bezier_with_arrows(DiaRenderer *renderer,
                        BezPoint *points,
                        int num_points,
                        real line_width,
                        Color *color,
                        Arrow *start_arrow,
                        Arrow *end_arrow)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  DiaObject *object = create_standard_bezierline (num_points, points,
						 start_arrow, end_arrow);
  _apply_style (self, object, NULL, color, 0.0);
  _push_object (self, object);
}

/*!
 * \brief Advertize renderer capabilities.
 * Everything has to be possible with DiaObject, but there might be some
 * limitation with this renderer's implementation.
 * \memberof _DiaImportRenderer
 */
static gboolean
is_capable_to (DiaRenderer *renderer, RenderCapability cap)
{
  return TRUE;
}


/*!
 * \brief Remember the pattern for later reference
 * \memberof _DiaImportRenderer
 */
static void
set_pattern (DiaRenderer *renderer, DiaPattern *pat)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);

  if (self->pattern && self->pattern != pat) {
    g_set_object (&self->pattern, pat);
  }
}


/*!
 * \brief Take ownership of the objects created so far
 * This function return a _Group object containing all DiaObject created
 * through the renderer interface so far. Ownership is transferred to the
 * caller so the internal list is emptied by this call. If only a single
 * object was created it is returned without the containing group.
 */
DiaObject *
dia_import_renderer_get_objects (DiaRenderer *renderer)
{
  DiaImportRenderer *self = DIA_IMPORT_RENDERER (renderer);
  if (!self || !self->objects)
    return NULL;
  if (g_list_length (self->objects) > 1) {
    DiaObject *group = create_standard_group (self->objects);
    self->objects = NULL;
    return group;
  } else {
    DiaObject *object = self->objects->data;

    g_clear_list (&self->objects, NULL);

    return object;
  }
}
