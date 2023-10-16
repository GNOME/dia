/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diarenderer.c - GObject based dia renderer base class
 * Copyright (C) 1998-2002 Various Dia developers
 * Copyright (C) 2002 Hans Breuer (refactoring)
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

#include "diarenderer.h"
#include "object.h"
#include "text.h"
#include "textline.h"
#include "diatransformrenderer.h"
#include "standard-path.h" /* text_to_path */
#include "boundingbox.h" /* PolyBBextra */
#include "dia-layer.h"

typedef struct _DiaRendererPrivate DiaRendererPrivate;
struct _DiaRendererPrivate
{
  DiaFont *font;
  double   font_height; /* IMO It should be possible use the font's size to keep
                         * this info, but currently _not_ : multi-line text is
                         * growing on every line when zoomed: BUG in font.c  --hb
                         */
  BezierApprox *bezier;
};

G_DEFINE_TYPE_WITH_PRIVATE (DiaRenderer, dia_renderer, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_FONT,
  PROP_FONT_HEIGHT,
  LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };


struct _BezierApprox {
  Point *points;
  int numpoints;
  int currpoint;
};

static void begin_render (DiaRenderer *, const DiaRectangle *update);
static void end_render (DiaRenderer *);

static void set_linewidth (DiaRenderer *renderer, real linewidth);
static void set_linecaps      (DiaRenderer  *renderer,
                               DiaLineCaps   mode);
static void set_linejoin      (DiaRenderer  *renderer,
                               DiaLineJoin   mode);
static void set_linestyle     (DiaRenderer  *renderer,
                               DiaLineStyle  mode,
                               double        length);
static void set_fillstyle     (DiaRenderer  *renderer,
                               DiaFillStyle  mode);

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
static void draw_string          (DiaRenderer  *renderer,
                                  const char   *text,
                                  Point        *pos,
                                  DiaAlignment  alignment,
                                  Color        *color);
static void draw_image           (DiaRenderer  *renderer,
                                  Point        *point,
                                  double        width,
                                  double        height,
                                  DiaImage     *image);
static void draw_text            (DiaRenderer  *renderer,
                                  Text         *text);
static void draw_text_line       (DiaRenderer  *renderer,
                                  TextLine     *text_line,
                                  Point        *pos,
                                  DiaAlignment  alignment,
                                  Color        *color);
static void draw_rotated_text (DiaRenderer *renderer, Text *text,
			       Point *center, real angle);
static void draw_rotated_image (DiaRenderer *renderer,
				Point *point,
				real width, real height,
				real angle,
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

static real get_text_width (DiaRenderer *renderer,
                            const gchar *text, int length);

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

static void
dia_renderer_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  DiaRenderer *self = DIA_RENDERER (object);
  DiaRendererPrivate *priv = dia_renderer_get_instance_private (self);

  switch (property_id) {
    case PROP_FONT:
      g_clear_object (&priv->font);
      priv->font = g_value_dup_object (value);
      break;
    case PROP_FONT_HEIGHT:
      priv->font_height = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_renderer_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  DiaRenderer *self = DIA_RENDERER (object);
  DiaRendererPrivate *priv = dia_renderer_get_instance_private (self);

  switch (property_id) {
    case PROP_FONT:
      g_value_set_object (value, priv->font);
      break;
    case PROP_FONT_HEIGHT:
      g_value_set_double (value, priv->font_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


/*!
 * \brief Render all the visible object in the layer
 *
 * @param renderer explicit this pointer
 * @param layer    layer to draw
 * @param active   TRUE if it is the currently active layer
 * @param update   the update rectangle, NULL for unlimited
 * Given an update rectangle the default implementation calls the draw_object()
 * member only for intersected objects. This method does _not_ care for layer
 * visibility, though. If an exporter wants to 'see' also invisible
 * layers this method needs to be overwritten. Also it does not pass any
 * matrix to draw_object().
 *
 * \memberof _DiaRenderer
 */
static void
draw_layer (DiaRenderer  *renderer,
            DiaLayer     *layer,
            gboolean      active,
            DiaRectangle *update)
{
  GList *list = dia_layer_get_object_list (layer);
  void (*func) (DiaRenderer*, DiaObject *, DiaMatrix *);

  g_return_if_fail (layer != NULL);

  func = DIA_RENDERER_GET_CLASS(renderer)->draw_object;
  /* Draw all objects  */
  while (list!=NULL) {
    DiaObject *obj = (DiaObject *) list->data;

    if (update==NULL || rectangle_intersects(update, dia_object_get_enclosing_box(obj))) {
      (*func)(renderer, obj, NULL);
    }
    list = g_list_next(list);
  }
}

/*!
 * \brief Render the given object with optional transformation matrix
 *
 * Calls _DiaObject::DrawFunc with the given renderer, if matrix is NULL.
 * Otherwise the object is transformed with the help of _DiaTransformRenderer.
 * A renderer capable to do affine transformation should overwrite this function.
 *
 * \memberof _DiaRenderer
 */
static void
draw_object (DiaRenderer *renderer,
             DiaObject   *object,
             DiaMatrix   *matrix)
{
  if (matrix) {
#if 1
    DiaRenderer *tr = dia_transform_renderer_new (renderer);

    dia_renderer_draw_object (tr, object, matrix);

    g_clear_object (&tr);

    return;
#else
    /* visual complaints - not completely correct */
    Point pt[4];
    DiaRectangle *bb = &object->bounding_box;
    Color red = { 1.0, 0.0, 0.0, 1.0 };

    pt[0].x = matrix->xx * bb->left + matrix->xy * bb->top + matrix->x0;
    pt[0].y = matrix->yx * bb->left + matrix->yy * bb->top + matrix->y0;
    pt[1].x = matrix->xx * bb->right + matrix->xy * bb->top + matrix->x0;
    pt[1].y = matrix->yx * bb->right + matrix->yy * bb->top + matrix->y0;
    pt[2].x = matrix->xx * bb->right + matrix->xy * bb->bottom + matrix->x0;
    pt[2].y = matrix->yx * bb->right + matrix->yy * bb->bottom + matrix->y0;
    pt[3].x = matrix->xx * bb->left + matrix->xy * bb->bottom + matrix->x0;
    pt[3].y = matrix->yx * bb->left + matrix->yy * bb->bottom + matrix->y0;

    dia_renderer_set_linewidth(renderer, 0.0);
    dia_renderer_set_linestyle(renderer, DIA_LINE_STYLE_DOTTED, 1.0);
    dia_renderer_draw_polygon(renderer, pt, 4, NULL, &red);
    dia_renderer_draw_line(renderer, &pt[0], &pt[2], &red);
    dia_renderer_draw_line(renderer, &pt[1], &pt[3], &red);
#endif
  }
  dia_object_draw (object, renderer);
}

static void
dia_renderer_finalize (GObject *object)
{
  DiaRenderer *renderer = DIA_RENDERER (object);
  DiaRendererPrivate *priv = dia_renderer_get_instance_private (renderer);

  g_clear_object (&priv->font);

  if (priv->bezier) {
    g_clear_pointer (&priv->bezier->points, g_free);
    g_clear_pointer (&priv->bezier, g_free);
  }

  G_OBJECT_CLASS (dia_renderer_parent_class)->finalize (object);
}

static void
dia_renderer_class_init (DiaRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  object_class->set_property= dia_renderer_set_property;
  object_class->get_property= dia_renderer_get_property;
  object_class->finalize = dia_renderer_finalize;

  renderer_class->draw_layer = draw_layer;
  renderer_class->draw_object = draw_object;
  renderer_class->get_text_width = get_text_width;

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
  renderer_class->draw_beziergon = draw_beziergon;
  renderer_class->draw_rounded_polyline  = draw_rounded_polyline;
  renderer_class->draw_polyline  = draw_polyline;
  renderer_class->draw_text      = draw_text;
  renderer_class->draw_text_line = draw_text_line;
  renderer_class->draw_rotated_text = draw_rotated_text;
  renderer_class->draw_rotated_image = draw_rotated_image;

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

  /**
   * DiaRenderer:font:
   *
   * Since: 0.98
   */
  pspecs[PROP_FONT] =
    g_param_spec_object ("font", "Font", "The active font",
                         DIA_TYPE_FONT,
                         G_PARAM_READWRITE);


  /**
   * DiaRenderer:font-height:
   *
   * Since: 0.98
   */
  pspecs[PROP_FONT_HEIGHT] =
    g_param_spec_double ("font-height",
                         "Font height",
                         "Height of the font",
                         0.0,
                         G_MAXDOUBLE,
                         0.0,
                         G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}

/*!
 * \name Renderer Required Members
 * A few member functions of _DiaRenderer must be implemented in every derived renderer.
 * These should be considered pure virtual although due to the C implementation actually
 * these member functions exist put throw a warning.
 @{
 */
/*!
 * \brief Called before rendering begins.
 *
 * Can be used to do various pre-rendering setup.
 *
 * \memberof _DiaRenderer \pure
 */
static void
begin_render (DiaRenderer *object, const DiaRectangle *update)
{
  g_warning ("%s::begin_render not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (object)));
}

/*!
 * \brief Called after all rendering is done.
 *
 * Used to do various clean-ups.
 *
 * \memberof _DiaRenderer \pure
 */
static void
end_render (DiaRenderer *object)
{
  g_warning ("%s::end_render not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (object)));
}

/*!
 * \brief Change the line width for the strokes to come
 * \memberof _DiaRenderer \pure
 */
static void
set_linewidth (DiaRenderer *object, real linewidth)
{
  g_warning ("%s::set_line_width not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (object)));
}


/*
 * Change the line caps for the strokes to come
 */
static void
set_linecaps (DiaRenderer *object, DiaLineCaps mode)
{
  g_warning ("%s::set_line_caps not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (object)));
}


/*
 * Change the line join mode for the strokes to come
 */
static void
set_linejoin (DiaRenderer *renderer, DiaLineJoin mode)
{
  g_warning ("%s::set_line_join not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
}


/*
 * Change line style and dash length for the strokes to come
 */
static void
set_linestyle (DiaRenderer *renderer, DiaLineStyle mode, double dash_length)
{
  g_warning ("%s::set_line_style not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
}


/*!
 * \brief Set the fill mode for following fills
 *
 * As of this writing there is only one fill mode defined, so this function
 * might never get called, because it does not make a difference.
 *
 * \memberof _DiaRenderer \pure
 */
static void
set_fillstyle (DiaRenderer *renderer, DiaFillStyle mode)
{
  g_warning ("%s::set_fill_style not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
}

/*!
 * \brief Draw a single line segment
 * \memberof _DiaRenderer \pure
 */
static void
draw_line (DiaRenderer *renderer, Point *start, Point *end, Color *color)
{
  g_warning ("%s::draw_line not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
}

/*!
 * \brief Fill and/or stroke a polygon
 *
 * The fall-back implementation of draw_polygon is mostly ignoring the fill.
 * A derived renderer should definitely overwrite this function.
 *
 * \memberof _DiaRenderer
 */
static void
draw_polygon (DiaRenderer *renderer,
              Point       *points,
              int          num_points,
              Color       *fill,
              Color       *stroke)
{
  int i;
  Color *color = fill ? fill : stroke;

  g_return_if_fail (num_points > 1);
  g_return_if_fail (color != NULL);

  for (i = 0; i < num_points - 1; i++) {
    dia_renderer_draw_line (renderer, &points[i+0], &points[i+1], color);
  }
  /* close it in any case */
  if (   (points[0].x != points[num_points-1].x)
      || (points[0].y != points[num_points-1].y)) {
    dia_renderer_draw_line (renderer, &points[num_points-1], &points[0], color);
  }
}

/*!
 * \brief Draw an arc
 *
 * Draw an arc, given its center, the bounding box (widget, height),
 * the start angle and the end angle. It's counter-clockwise if angle2>angle1
 *
 * \memberof _DiaRenderer \pure
 */
static void
draw_arc (DiaRenderer *renderer, Point *center,
          real width, real height, real angle1, real angle2,
          Color *color)
{
  g_warning ("%s::draw_arc not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
}

/*!
 * \brief Fill an arc (a pie-chart)
 * \memberof _DiaRenderer \pure
 */
static void
fill_arc (DiaRenderer *renderer, Point *center,
          real width, real height, real angle1, real angle2,
          Color *color)
{
  g_warning ("%s::fill_arc not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
}
/*!
 * \brief Fill and/or stroke an ellipse
 * \memberof _DiaRenderer \pure
 */
static void
draw_ellipse (DiaRenderer *renderer, Point *center,
              real width, real height,
              Color *fill, Color *stroke)
{
  g_warning ("%s::draw_ellipse not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
}

/*!
 * \brief Set the font to use for following strings
 *
 * This function might be called before begin_render() because it also
 * sets the font for following get_text_width() calls.
 *
 * \memberof _DiaRenderer \pure
 */


/*
 * Draw a string
 */
static void
draw_string (DiaRenderer  *renderer,
             const char   *text,
             Point        *pos,
             DiaAlignment  alignment,
             Color        *color)
{
  g_warning ("%s::draw_string not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
}


/*!
 * \brief Draw an image (pixbuf)
 * \memberof _DiaRenderer \pure
 */
static void
draw_image (DiaRenderer *renderer,
            Point *point, real width, real height,
            DiaImage *image)
{
  g_warning ("%s::draw_image not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
}

/*! @} */

/*!
 * \brief Default implementation of draw_text
 *
 * The default implementation of draw_text() splits the given _Text object
 * into single lines and passes them to draw_text_line().
 * A Renderer with a concept of multi-line text should overwrite it.
 *
 * \memberof _DiaRenderer
 */
static void
draw_text (DiaRenderer *renderer,
	   Text *text)
{
  Point pos;
  int i;

  pos = text->position;

  for (i=0;i<text->numlines;i++) {
    TextLine *text_line = text->lines[i];

    dia_renderer_draw_text_line (renderer,
                                 text_line,
                                 &pos,
                                 text->alignment,
                                 &text->color);
    pos.y += text->height;
  }
}

/**
 * draw_rotated_text:
 * @renderer: the #DiaRenderer
 * @text: the #Text
 * @center: where to draw @text
 * @angle: how much to rotate @text about @center
 *
 * Default implementation to draw rotated text
 *
 * The default implementation converts the given #Text object to a
 * path and passes it to draw_beziergon() if the renderer support
 * rendering wih holes. If not a fallback implementation is used.
 *
 * A Renderer with a good own concept of rotated text should
 * overwrite it.
 */
static void
draw_rotated_text (DiaRenderer *renderer,
                   Text        *text,
                   Point       *center,
                   real         angle)
{
  if (angle == 0.0) {
    /* maybe the fallback should also consider center? */
    draw_text (renderer, text);
  } else {
    GArray *path = g_array_new (FALSE, FALSE, sizeof (BezPoint));
    if (!text_is_empty (text) && text_to_path (text, path)) {
      /* Scaling and transformation here */
      DiaRectangle bz_bb, tx_bb;
      PolyBBExtras extra = { 0, };
      real sx, sy;
      guint i;
      real dx = center ? (text->position.x - center->x) : 0;
      real dy = center ? (text->position.y - center->y) : 0;
      DiaMatrix m = { 1, 0, 0, 1, 0, 0 };
      DiaMatrix t = { 1, 0, 0, 1, 0, 0 };

      polybezier_bbox (&g_array_index (path, BezPoint, 0), path->len, &extra, TRUE, &bz_bb);
      text_calc_boundingbox (text, &tx_bb);
      sx = (tx_bb.right - tx_bb.left) / (bz_bb.right - bz_bb.left);
      sy = (tx_bb.bottom - tx_bb.top) / (bz_bb.bottom - bz_bb.top);

      /* move center to origin */
      if (DIA_ALIGN_LEFT == text->alignment) {
        t.x0 = -bz_bb.left;
      } else if (DIA_ALIGN_RIGHT == text->alignment) {
        t.x0 = - bz_bb.right;
      } else {
        t.x0 = -(bz_bb.left + bz_bb.right) / 2.0;
      }
      t.x0 -= dx / sx;
      t.y0 = - bz_bb.top - (text_get_ascent (text) - dy) / sy;
      dia_matrix_set_angle_and_scales (&m, G_PI * angle / 180.0, sx, sx);
      dia_matrix_multiply (&m, &t, &m);
      /* move back center from origin */
      if (DIA_ALIGN_LEFT == text->alignment) {
        t.x0 = tx_bb.left;
      } else if (DIA_ALIGN_RIGHT == text->alignment) {
        t.x0 = tx_bb.right;
      } else {
        t.x0 = (tx_bb.left + tx_bb.right) / 2.0;
      }
      t.x0 += dx;
      t.y0 = tx_bb.top + (text_get_ascent (text) - dy);
      dia_matrix_multiply (&m, &m, &t);

      for (i = 0; i < path->len; ++i) {
        BezPoint *bp = &g_array_index (path, BezPoint, i);
        transform_bezpoint (bp, &m);
      }

      if (dia_renderer_is_capable_of (renderer, RENDER_HOLES)) {
        dia_renderer_draw_beziergon (renderer,
                                      &g_array_index (path, BezPoint, 0),
                                      path->len,
                                      &text->color,
                                      NULL);
      } else {
        dia_renderer_bezier_fill (renderer,
                                  &g_array_index (path, BezPoint, 0),
                                  path->len,
                                  &text->color);
      }
    } else {
      Color magenta = { 1.0, 0.0, 1.0, 1.0 };
      Point pt = center ? *center : text->position;
      DiaMatrix m = { 1, 0, 0, 1, pt.x, pt.y };
      DiaMatrix t = { 1, 0, 0, 1, -pt.x, -pt.y };
      DiaRectangle tb;
      Point poly[4];
      int i;

      text_calc_boundingbox (text, &tb);
      poly[0].x = tb.left;  poly[0].y = tb.top;
      poly[1].x = tb.right; poly[1].y = tb.top;
      poly[2].x = tb.right; poly[2].y = tb.bottom;
      poly[3].x = tb.left;  poly[3].y = tb.bottom;

      dia_matrix_set_angle_and_scales (&m, G_PI * angle / 180.0, 1.0, 1.0);
      dia_matrix_multiply (&m, &t, &m);

      for (i = 0; i < 4; ++i) {
        transform_point (&poly[i], &m);
      }

      dia_renderer_set_linewidth (renderer, 0.0);
      dia_renderer_draw_polygon (renderer,
                                 poly,
                                 4,
                                 NULL,
                                 &magenta);
    }
    g_array_free (path, TRUE);
  }
}

static void
draw_rotated_image (DiaRenderer *renderer,
                    Point       *point,
                    real         width,
                    real         height,
                    real         angle,
                    DiaImage    *image)
{
  if (angle == 0.0) {
    dia_renderer_draw_image (renderer, point, width, height, image);
  } else {
    /* XXX: implement fallback */
    dia_renderer_draw_image (renderer, point, width, height, image);
  }
}


/**
 * draw_text_line:
 * @renderer: the #DiaRenderer
 * @text_line: the #TextLine to draw
 * @pos: where to draw @text_line
 * @alignment: text #DiaAlignment
 * @color: text #Color
 *
 * Default implementation of draw_text_line
 *
 * The default implementation of dia_renderer_draw_text_line() just calls
 * dia_renderer_set_font() and dia_renderer_draw_string().
 */
static void
draw_text_line (DiaRenderer  *renderer,
                TextLine     *text_line,
                Point        *pos,
                DiaAlignment  alignment,
                Color        *color)
{
  dia_renderer_set_font (renderer,
                         text_line_get_font (text_line),
                         text_line_get_height (text_line));

  dia_renderer_draw_string (renderer,
                            text_line_get_string (text_line),
                            pos,
                            alignment,
                            color);
}

/*
 * medium level functions, implemented by the above
 */
/* Bezier implementation notes:
 * These beziers have the following basis matrix:
 * [-1  3 -3  1]
 * [ 3 -6  3  0]
 * [-3  3  0  0]
 * [ 1  0  0  0]
 * (At least that's what Hearn and Baker says for beziers.)
 */
#define BEZIER_SUBDIVIDE_LIMIT 0.001
#define BEZIER_SUBDIVIDE_LIMIT_SQ (BEZIER_SUBDIVIDE_LIMIT*BEZIER_SUBDIVIDE_LIMIT)

static void
bezier_add_point(BezierApprox *bezier,
                 Point *point)
{
  /* Grow if needed: */
  if (bezier->currpoint == bezier->numpoints) {
    bezier->numpoints += 40;
    bezier->points = g_renew (Point, bezier->points, bezier->numpoints);
  }

  bezier->points[bezier->currpoint] = *point;

  bezier->currpoint++;
}

static void
bezier_add_lines(BezierApprox *bezier,
                 Point points[4])
{
  Point u, v, x, y;
  Point r[4];
  Point s[4];
  Point middle;
  double delta;
  real v_len_sq;

  /* Check if almost flat: */
  u = points[1];
  point_sub(&u, &points[0]);
  v = points[3];
  point_sub(&v, &points[0]);
  y = v;
  v_len_sq = point_dot(&v,&v);
  if (isnan(v_len_sq)) {
    g_warning("v_len_sq is NaN while calculating bezier curve!");
    return;
  }
  if (v_len_sq < 0.000001)
    v_len_sq = 0.000001;
  point_scale(&y, point_dot(&u,&v)/v_len_sq);
  x = u;
  point_sub(&x,&y);
  delta = point_dot(&x,&x);
  if (delta < BEZIER_SUBDIVIDE_LIMIT_SQ) {
    u = points[2];
    point_sub(&u, &points[3]);
    v = points[0];
    point_sub(&v, &points[3]);
    y = v;
    v_len_sq = point_dot(&v,&v);
    if (v_len_sq < 0.000001)
      v_len_sq = 0.000001;
    point_scale(&y, point_dot(&u,&v)/v_len_sq);
    x = u;
    point_sub(&x,&y);
    delta = point_dot(&x,&x);
    if (delta < BEZIER_SUBDIVIDE_LIMIT_SQ) { /* Almost flat, draw a line */
      bezier_add_point(bezier, &points[3]);
      return;
    }
  }
  /* Subdivide into two bezier curves: */

  middle = points[1];
  point_add(&middle, &points[2]);
  point_scale(&middle, 0.5);

  r[0] = points[0];

  r[1] = points[0];
  point_add(&r[1], &points[1]);
  point_scale(&r[1], 0.5);

  r[2] = r[1];
  point_add(&r[2], &middle);
  point_scale(&r[2], 0.5);

  s[3] = points[3];

  s[2] = points[2];
  point_add(&s[2], &points[3]);
  point_scale(&s[2], 0.5);

  s[1] = s[2];
  point_add(&s[1], &middle);
  point_scale(&s[1], 0.5);

  r[3] = r[2];
  point_add(&r[3], &s[1]);
  point_scale(&r[3], 0.5);

  s[0] = r[3];

  bezier_add_lines(bezier, r);
  bezier_add_lines(bezier, s);
}

static void
bezier_add_curve(BezierApprox *bezier,
                 Point points[4])
{
  /* Is the bezier curve malformed? */
  if ( (distance_point_point(&points[0], &points[1]) < 0.00001) &&
       (distance_point_point(&points[2], &points[3]) < 0.00001) &&
       (distance_point_point(&points[0], &points[3]) < 0.00001)) {
    bezier_add_point(bezier, &points[3]);
  }

  bezier_add_lines(bezier, points);
}

static void
approximate_bezier (BezierApprox *bezier,
                    BezPoint *points, int numpoints)
{
  Point curve[4];
  int i;

  if (points[0].type != BEZ_MOVE_TO)
    g_warning("first BezPoint must be a BEZ_MOVE_TO");
  curve[3] = points[0].p1;
  bezier_add_point(bezier, &points[0].p1);
  for (i = 1; i < numpoints; i++) {
    switch (points[i].type) {
      case BEZ_MOVE_TO:
        g_warning("only first BezPoint can be a BEZ_MOVE_TO");
        curve[3] = points[i].p1;
        break;
      case BEZ_LINE_TO:
        bezier_add_point(bezier, &points[i].p1);
        curve[3] = points[i].p1;
        break;
      case BEZ_CURVE_TO:
        curve[0] = curve[3];
        curve[1] = points[i].p1;
        curve[2] = points[i].p2;
        curve[3] = points[i].p3;
        bezier_add_curve(bezier, curve);
        break;
      default:
        g_return_if_reached ();
    }
  }
}

/*!
 * \name Medium Level Methods Usually Overwritten by Modern Renderer
 *
 * The medium level renderer methods have a working fall-back implementation to
 * give the same visual appearance as native member function implementations.
 * However these functions should be overwritten, if the goal is further processing
 * or optimized output.
 *
 * @{
 */
/*!
 * \brief Draw a bezier curve, given it's control points
 *
 * The first BezPoint must be of type MOVE_TO, and no other ones may be
 * MOVE_TOs. If further holes are supported by a specific renderer should
 * be checked with _DiaRenderer::is_capable_to(RENDER_HOLES).
 *
 * The default implementation of draw_bezier() converts the given path
 * into a polyline approximation and calls draw_polyline().
 *
 * \memberof _DiaRenderer
 */
static void
draw_bezier (DiaRenderer *renderer,
             BezPoint    *points,
             int          numpoints,
             Color       *color)
{
  DiaRendererPrivate *priv = dia_renderer_get_instance_private (renderer);
  BezierApprox *bezier;

  if (priv->bezier)
    bezier = priv->bezier;
  else
    priv->bezier = bezier = g_new0 (BezierApprox, 1);

  if (bezier->points == NULL) {
    bezier->numpoints = 30;
    bezier->points = g_new0 (Point, bezier->numpoints);
  }

  bezier->currpoint = 0;
  approximate_bezier (bezier, points, numpoints);

  dia_renderer_draw_polyline (renderer,
                              bezier->points,
                              bezier->currpoint,
                              color);
}

/*!
 * \brief Fill and/or stroke a closed bezier curve
 *
 * The default implementation can only handle a single MOVE_TO at the
 * first point. It emulates the actual drawing with an approximated
 * polyline.
 *
 * \memberof _DiaRenderer
 */
static void
draw_beziergon (DiaRenderer *renderer,
                BezPoint    *points,
                int          numpoints,
                Color       *fill,
                Color       *stroke)
{
  DiaRendererPrivate *priv = dia_renderer_get_instance_private (renderer);
  BezierApprox *bezier;

  g_return_if_fail (fill != NULL || stroke != NULL);

  if (priv->bezier)
    bezier = priv->bezier;
  else
    priv->bezier = bezier = g_new0 (BezierApprox, 1);

  if (bezier->points == NULL) {
    bezier->numpoints = 30;
    bezier->points = g_new0 (Point, bezier->numpoints);
  }

  bezier->currpoint = 0;
  approximate_bezier (bezier, points, numpoints);

  if (fill || stroke)
    dia_renderer_draw_polygon (renderer,
                               bezier->points,
                               bezier->currpoint,
                               fill,
                               stroke);
}

/*!
 * \brief Stroke and/or fill a rectangle
 *
 * This only needs to be implemented in the derived class if it differs
 * from draw_polygon. Given that draw_polygon is a required method we can
 * use that instead of forcing every inherited class to implement
 * draw_rect(), too.
 *
 * \memberof _DiaRenderer \pure
 */
static void
draw_rect (DiaRenderer *renderer,
           Point *ul_corner, Point *lr_corner,
           Color *fill, Color *stroke)
{
  if (DIA_RENDERER_GET_CLASS (renderer)->draw_polygon == &draw_polygon) {
    g_warning ("%s::draw_rect and draw_polygon not implemented!",
               G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
  } else {
    Point corner[4];
    /* translate to polygon */
    corner[0] = *ul_corner;
    corner[1].x = lr_corner->x;
    corner[1].y = ul_corner->y;
    corner[2] = *lr_corner;
    corner[3].x = ul_corner->x;
    corner[3].y = lr_corner->y;
    /* delegate transformation and drawing */
    dia_renderer_draw_polygon (renderer, corner, 4, fill, stroke);
  }
}

/*!
 * \brief Draw a polyline with the given color
 *
 * The default implementation is delegating the drawing to consecutive
 * calls to draw_line().
 *
 * \memberof _DiaRenderer
 */
static void
draw_polyline (DiaRenderer *renderer,
               Point       *points,
               int          num_points,
               Color       *color)
{
  int i;

  for (i = 0; i < num_points - 1; i++) {
    dia_renderer_draw_line (renderer, &points[i+0], &points[i+1], color);
  }
}


/* calculate the maximum possible radius for 3 points
     use the following,
     given points p1,p2, and p3
     let c = min(length(p1,p2)/2,length(p2,p3)/2)
     let a = dot2(p1-p2, p3-p2)
       (ie, angle between lines p1,p2 and p2,p3)
     then maxr = c * sin(a/2)
 */
static real
calculate_min_radius( Point *p1, Point *p2, Point *p3 )
{
  real c;
  real a;
  Point v1,v2;

  c = MIN(distance_point_point(p1,p2)/2,distance_point_point(p2,p3)/2);
  v1.x = p1->x-p2->x; v1.y = p1->y-p2->y;
  v2.x = p3->x-p2->x; v2.y = p3->y-p2->y;
  a =  dot2(&v1,&v2);
  return (c*sin(a/2));
}

/*!
 * \brief Draw a polyline with optionally rounded corners
 *
 * Default implementation based on draw_line() and draw_arc(), but uses
 * draw_polyline when the rounding is too small.
 *
 * \memberof _DiaRenderer
 */
static void
draw_rounded_polyline (DiaRenderer *renderer,
                       Point       *points,
                       int          num_points,
                       Color       *color,
                       real         radius)
{
  int i = 0;
  Point p1,p2,p3,p4 ;
  Point *p;
  p = points;

  if (radius < 0.00001) {
    dia_renderer_draw_polyline (renderer, points, num_points, color);
    return;
  }

  /* skip arc computations if we only have points */
  if ( num_points <= 2 ) {
    i = 0;
    p1.x = p[i].x; p1.y = p[i].y;
    p2.x = p[i+1].x; p2.y = p[i+1].y;
    dia_renderer_draw_line (renderer, &p1, &p2, color);
    return;
  }

  i = 0;
  /* full rendering 3 or more points */
  p1.x = p[i].x;   p1.y = p[i].y;
  p2.x = p[i+1].x; p2.y = p[i+1].y;
  for (i = 0; i <= num_points - 3; i++) {
    Point c;
    real start_angle, stop_angle;
    real min_radius;
    gboolean arc_it;

    p3.x = p[i+1].x; p3.y = p[i+1].y;
    p4.x = p[i+2].x; p4.y = p[i+2].y;

    /* adjust the radius if it would cause odd rendering */
    min_radius = MIN(radius, calculate_min_radius(&p1,&p2,&p4));
    arc_it = fillet(&p1,&p2,&p3,&p4, min_radius, &c, &start_angle, &stop_angle);
    /* start with the line drawing to allow joining in backend */
    dia_renderer_draw_line (renderer, &p1, &p2, color);
    if (arc_it) {
      dia_renderer_draw_arc (renderer,
                             &c,
                             min_radius*2,
                             min_radius*2,
                             start_angle,
                             stop_angle,
                             color);
    }
    p1.x = p3.x; p1.y = p3.y;
    p2.x = p4.x; p2.y = p4.y;
  }
  dia_renderer_draw_line (renderer, &p3, &p4, color);
}

/*!
 * \brief Fill and/or stroke a rectangle with rounded corners
 *
 * Default implementation is assembling a rectangle with potentially rounded
 * corners from consecutive draw_arc() and draw_line() calls - if stroked.
 * Filling is done by two overlapping rectangles and four fill_arc() calls.
 *
 * \memberof _DiaRenderer
 */
static void
draw_rounded_rect (DiaRenderer *renderer,
                   Point       *ul_corner,
                   Point       *lr_corner,
                   Color       *fill,
                   Color       *stroke,
                   real         radius)
{
  /* clip radius per axis to use the full API;) */
  real rw = MIN(radius, (lr_corner->x-ul_corner->x)/2);
  real rh = MIN(radius, (lr_corner->y-ul_corner->y)/2);

  if (rw < 0.00001 || rh < 0.00001) {
    dia_renderer_draw_rect (renderer, ul_corner, lr_corner, fill, stroke);
  } else {
    real tlx = ul_corner->x; /* top-left x */
    real tly = ul_corner->y; /* top-left y */
    real brx = lr_corner->x; /* bottom-right x */
    real bry = lr_corner->y; /* bottom-right y */
    /* calculate all start/end points needed in advance, counter-clockwise */
    Point pts[8];
    Point cts[4]; /* ... and centers */
    int i;

    cts[0].x = tlx + rw; cts[0].y = tly + rh; /* ul */
    pts[0].x = tlx; pts[0].y = tly + rh;
    pts[1].x = tlx; pts[1].y = bry - rh; /* down */

    cts[1].x = tlx + rw; cts[1].y = bry - rh; /* ll */
    pts[2].x = tlx + rw; pts[2].y = bry;
    pts[3].x = brx - rw; pts[3].y = bry; /* right */

    cts[2].x = brx - rw; cts[2].y = bry - rh; /* lr */
    pts[4].x = brx; pts[4].y = bry - rh;
    pts[5].x = brx; pts[5].y = tly + rh; /* up */

    cts[3].x = brx - rw; cts[3].y = tly + rh; /* ur */
    pts[6].x = brx - rw; pts[6].y = tly; /* left */
    pts[7].x = tlx + rw; pts[7].y = tly;


    /* If line_width would be available we could approximate small radius with:
     * renderer_ops->draw_polygon (renderer, pts, 8, fill, stroke);
     */
    /* a filled cross w/ overlap : might not be desirable with alpha */
    if (fill) {
      if (pts[3].x > pts[7].x) {
        dia_renderer_draw_rect (renderer, &pts[7], &pts[3], fill, NULL);
      }
      if (pts[4].y > pts[0].y) {
        dia_renderer_draw_rect (renderer, &pts[0], &pts[4], fill, NULL);
      }
    }
    for (i = 0; i < 4; ++i) {
      if (fill) {
        dia_renderer_fill_arc (renderer, &cts[i], 2*rw, 2*rh, (i+1)*90.0, (i+2)*90.0, fill);
      }
      if (stroke) {
        dia_renderer_draw_arc (renderer, &cts[i], 2*rw, 2*rh, (i+1)*90.0, (i+2)*90.0, stroke);
      }
      if (stroke) {
        dia_renderer_draw_line (renderer, &pts[i*2], &pts[i*2+1], stroke);
      }
    }
  }
}
/*! @} */

/*!
 * \name Draw With Arrows Methods
 *
 * A renderer implementation with a compatible concept of arrows should overwrite this
 * function set to get the most high level output. For all other renderer a line with
 * arrows will be split into multiple objects, which still will resemble the original
 * appearance of the diagram.
 *
 * @{
 */

/*!
 * \brief Draw a line fitting to the given arrows
 *
 * \memberof _DiaRenderer
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
  Point oldstart = *startpoint;
  Point oldend = *endpoint;
  Point start_arrow_head;
  Point end_arrow_head;

  /* Calculate how to more the line to account for arrow heads */
  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    calculate_arrow_point(start_arrow, startpoint, endpoint,
			  &move_arrow, &move_line,
			  line_width);
    start_arrow_head = *startpoint;
    point_sub(&start_arrow_head, &move_arrow);
    point_sub(startpoint, &move_line);
  }
  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    calculate_arrow_point(end_arrow, endpoint, startpoint,
 			  &move_arrow, &move_line,
			  line_width);
    end_arrow_head = *endpoint;
    point_sub(&end_arrow_head, &move_arrow);
    point_sub(endpoint, &move_line);
  }

  dia_renderer_draw_line (renderer, startpoint, endpoint, color);

  /* Actual arrow drawing down here so line styles aren't disturbed */
  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    dia_arrow_draw (start_arrow,
                    renderer,
                    &start_arrow_head,
                    endpoint,
                    line_width,
                    color,
                    &color_white);
  }

  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    dia_arrow_draw (end_arrow,
                    renderer,
                    &end_arrow_head,
                    startpoint,
                    line_width,
                    color,
                    &color_white);
  }

  *startpoint = oldstart;
  *endpoint = oldend;
}

/*!
 * \brief Draw a polyline fitting to the given arrows
 *
 * \memberof _DiaRenderer
 */
static void
draw_polyline_with_arrows(DiaRenderer *renderer,
                          Point *points, int num_points,
                          real line_width,
                          Color *color,
                          Arrow *start_arrow,
                          Arrow *end_arrow)
{
  /* Index of first and last point with a non-zero length segment */
  int firstline = 0;
  int lastline = num_points;
  Point oldstart = points[firstline];
  Point oldend = points[lastline-1];
  Point start_arrow_head;
  Point end_arrow_head;

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    while (firstline < num_points-1 &&
	   distance_point_point(&points[firstline],
				&points[firstline+1]) < 0.0000001)
      firstline++;
    if (firstline == num_points-1)
      firstline = 0; /* No non-zero lines, it doesn't matter. */
    oldstart = points[firstline];
    calculate_arrow_point(start_arrow,
			  &points[firstline], &points[firstline+1],
			  &move_arrow, &move_line,
			  line_width);
    start_arrow_head = points[firstline];
    point_sub(&start_arrow_head, &move_arrow);
    point_sub(&points[firstline], &move_line);
  }
  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    while (lastline > 0 &&
	   distance_point_point(&points[lastline-1],
				&points[lastline-2]) < 0.0000001)
      lastline--;
    if (lastline == 0)
      firstline = num_points; /* No non-zero lines, it doesn't matter. */
    oldend = points[lastline-1];
    calculate_arrow_point(end_arrow, &points[lastline-1],
			  &points[lastline-2],
 			  &move_arrow, &move_line,
			  line_width);
    end_arrow_head = points[lastline-1];
    point_sub(&end_arrow_head, &move_arrow);
    point_sub(&points[lastline-1], &move_line);
  }
  /* Don't draw degenerate line segments at end of line */
  if (lastline-firstline > 1) {
    /* probably hiding a bug above, but don't try to draw a negative
     * number of points at all, fixes bug #148139 */
    dia_renderer_draw_polyline (renderer,
                                &points[firstline],
                                lastline-firstline,
                                color);
  }

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    dia_arrow_draw (start_arrow,
                    renderer,
                    &start_arrow_head,
                    &points[firstline+1],
                    line_width,
                    color,
                    &color_white);
  }

  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    dia_arrow_draw (end_arrow,
                    renderer,
                    &end_arrow_head,
                    &points[lastline-2],
                    line_width,
                    color,
                    &color_white);
  }

  points[firstline] = oldstart;
  points[lastline-1] = oldend;
}

/*!
 * \brief Draw a rounded polyline fitting to the given arrows
 *
 * \memberof _DiaRenderer
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
  /* Index of first and last point with a non-zero length segment */
  int firstline = 0;
  int lastline = num_points;
  Point oldstart = points[firstline];
  Point oldend = points[lastline-1];
  Point start_arrow_head;
  Point end_arrow_head;

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    while (firstline < num_points-1 &&
	   distance_point_point(&points[firstline],
				&points[firstline+1]) < 0.0000001)
      firstline++;
    if (firstline == num_points-1)
      firstline = 0; /* No non-zero lines, it doesn't matter. */
    oldstart = points[firstline];
    calculate_arrow_point(start_arrow,
			  &points[firstline], &points[firstline+1],
			  &move_arrow, &move_line,
			  line_width);
    start_arrow_head = points[firstline];
    point_sub(&start_arrow_head, &move_arrow);
    point_sub(&points[firstline], &move_line);
  }
  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    while (lastline > 0 &&
	   distance_point_point(&points[lastline-1],
				&points[lastline-2]) < 0.0000001)
      lastline--;
    if (lastline == 0)
      firstline = num_points; /* No non-zero lines, it doesn't matter. */
    oldend = points[lastline-1];
    calculate_arrow_point(end_arrow, &points[lastline-1],
			  &points[lastline-2],
 			  &move_arrow, &move_line,
			  line_width);
    end_arrow_head = points[lastline-1];
    point_sub(&end_arrow_head, &move_arrow);
    point_sub(&points[lastline-1], &move_line);
  }
  /* Don't draw degenerate line segments at end of line */
  if (lastline-firstline > 1) { /* only if there is something */
    dia_renderer_draw_rounded_polyline (renderer,
                                        &points[firstline],
                                        lastline-firstline,
                                        color, radius);
  }

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    dia_arrow_draw (start_arrow,
                    renderer,
                    &start_arrow_head, &points[firstline+1],
                    line_width,
                    color,
                    &color_white);
  }

  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    dia_arrow_draw (end_arrow,
                    renderer,
                    &end_arrow_head, &points[lastline-2],
                    line_width,
                    color,
                    &color_white);
  }

  points[firstline] = oldstart;
  points[lastline-1] = oldend;
}

/**
 * points_to_line:
 *
 * Figure the equation for a line given by two points.
 *
 * Returns: %FALSE if the line is vertical (infinite a).
 */
static gboolean
points_to_line (real *a, real *b, Point *p1, Point *p2)
{
  if (fabs (p1->x - p2->x) < 0.000000001) {
    return FALSE;
  }
  *a = (p2->y-p1->y)/(p2->x-p1->x);
  *b = p1->y-(*a)*p1->x;
  return TRUE;
}

/**
 * intersection_line_line:
 *
 * Find the intersection between two lines.
 *
 * Returns: %TRUE if the lines intersect in a single point.
 */
static gboolean
intersection_line_line (Point *cross,
                        Point *p1a,
                        Point *p1b,
                        Point *p2a,
                        Point *p2b)
{
  real a1, b1, a2, b2;

  /* Find coefficients of lines */
  if (!(points_to_line (&a1, &b1, p1a, p1b))) {
    if (!(points_to_line (&a2, &b2, p2a, p2b))) {
      if (fabs (p1a->x-p2a->x) < 0.00000001) {
        *cross = *p1a;
        return TRUE;
      } else {
        return FALSE;
      }
    }
    cross->x = p1a->x;
    cross->y = a2*(p1a->x)+b2;
    return TRUE;
  }
  if (!(points_to_line (&a2, &b2, p2a, p2b))) {
    cross->x = p2a->x;
    cross->y = a1*(p2a->x)+b1;
    return TRUE;
  }
  /* Solve */
  if (fabs (a1-a2) < 0.000000001) {
    if (fabs (b1-b2) < 0.000000001) {
      *cross = *p1a;
      return TRUE;
    } else {
      return FALSE;
    }
  } else {
    /*
    a1x+b1 = a2x+b2;
    a1x = a2x+b2-b1;
    a1x-a2x = b2-b1;
    (a1-a2)x = b2-b1;
    x = (b2-b1)/(a1-a2)
    */
    cross->x = (b2-b1)/(a1-a2);
    cross->y = a1*cross->x+b1;
    return TRUE;
  }
}

/**
 * find_center_point:
 *
 * Given three points, find the center of the circle they describe.
 *
 * The renderer should disappear once the debugging is done.
 *
 * Returns: %FALSE if the center could not be determined (i.e. the points
 * all lie really close together).
 */
static gboolean
find_center_point (Point       *center,
                   const Point *p1,
                   const Point *p2,
                   const Point *p3)
{
  Point mid1;
  Point mid2;
  Point orth1;
  Point orth2;
  real tmp;

  /* Find vector from middle between two points towards center */
  mid1 = *p1;
  point_sub(&mid1, p2);
  point_scale(&mid1, 0.5);
  orth1 = mid1;
  point_add(&mid1, p2); /* Now midpoint between p1 & p2 */
  tmp = orth1.x;
  orth1.x = orth1.y;
  orth1.y = -tmp;
  point_add(&orth1, &mid1);


  /* Again, with two other points */
  mid2 = *p2;
  point_sub(&mid2, p3);
  point_scale(&mid2, 0.5);
  orth2 = mid2;
  point_add(&mid2, p3); /* Now midpoint between p2 & p3 */
  tmp = orth2.x;
  orth2.x = orth2.y;
  orth2.y = -tmp;
  point_add(&orth2, &mid2);

  /* The intersection between these two is the center */
  if (!intersection_line_line(center, &mid1, &orth1, &mid2, &orth2)) {
    /* Degenerate circle */
    /* Case 1: Points are all on top of each other.  Nothing to do. */
    if (fabs((p1->x + p2->x + p3->x)/3 - p1->x) < 0.0000001 &&
	fabs((p1->y + p2->y + p3->y)/3 - p1->y) < 0.0000001) {
      return FALSE;
    }

    /* Case 2: Two points are on top of each other.  Midpoint of
     * non-degenerate line is center. */
    if (distance_point_point_manhattan(p1, p2) < 0.0000001) {
      *center = mid2;
      return TRUE;
    } else if (distance_point_point_manhattan(p1, p3) < 0.0000001 ||
	       distance_point_point_manhattan(p2, p3) < 0.0000001) {
      *center = mid1;
      return TRUE;
    }
    /* Case 3: All points on a line.  Nothing to do. */
    return FALSE;
  }
  return TRUE;
}

static gboolean
is_right_hand (const Point *a, const Point *b, const Point *c)
{
  Point dot1, dot2;

  dot1 = *a;
  point_sub(&dot1, c);
  point_normalize(&dot1);
  dot2 = *b;
  point_sub(&dot2, c);
  point_normalize(&dot2);
  return point_cross(&dot1, &dot2) > 0;
}

/*!
 * \brief Draw an arc fitting to the given arrows
 *
 * \memberof _DiaRenderer
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
  Point new_startpoint = *startpoint;
  Point new_endpoint = *endpoint;
  Point center;
  real width, angle1, angle2;
  gboolean righthand;
  Point start_arrow_head;
  Point start_arrow_end;
  Point end_arrow_head;
  Point end_arrow_end;

  if (!find_center_point(&center, startpoint, endpoint, midpoint)) {
    /* Degenerate circle -- should have been caught by the drawer? */
    g_warning("Degenerated arc in draw_arc_with_arrows()");
    center = *startpoint; /* continue to draw something bogus ... */
  }

  righthand = is_right_hand (startpoint, midpoint, endpoint);
  /* calculate original direction */
  angle1 = -atan2(new_startpoint.y - center.y, new_startpoint.x - center.x)*180.0/G_PI;
  while (angle1 < 0.0) angle1 += 360.0;
  angle2 = -atan2(new_endpoint.y - center.y, new_endpoint.x - center.x)*180.0/G_PI;
  while (angle2 < 0.0) angle2 += 360.0;

  width = 2*distance_point_point(&center, startpoint);

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    real tmp;

    start_arrow_end = *startpoint;
    point_sub(&start_arrow_end, &center);
    tmp = start_arrow_end.x;
    if (righthand) {
      start_arrow_end.x = -start_arrow_end.y;
      start_arrow_end.y = tmp;
    } else {
      start_arrow_end.x = start_arrow_end.y;
      start_arrow_end.y = -tmp;
    }
    point_add(&start_arrow_end, startpoint);

    calculate_arrow_point(start_arrow, startpoint, &start_arrow_end,
			  &move_arrow, &move_line,
			  line_width);
    start_arrow_head = *startpoint;
    point_sub(&start_arrow_head, &move_arrow);
    point_sub(&new_startpoint, &move_line);
  }
  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    real tmp;

    end_arrow_end = *endpoint;
    point_sub(&end_arrow_end, &center);
    tmp = end_arrow_end.x;
    if (righthand) {
      end_arrow_end.x = end_arrow_end.y;
      end_arrow_end.y = -tmp;
    } else {
      end_arrow_end.x = -end_arrow_end.y;
      end_arrow_end.y = tmp;
    }
    point_add(&end_arrow_end, endpoint);

    calculate_arrow_point(end_arrow, endpoint, &end_arrow_end,
 			  &move_arrow, &move_line,
			  line_width);
    end_arrow_head = *endpoint;
    point_sub(&end_arrow_head, &move_arrow);
    point_sub(&new_endpoint, &move_line);
  }

  /* Now we possibly have new start- and endpoint. We must not
   * recalculate the center cause the new points lie on the tangential
   * approximation of the original arc arrow lines not on the arc itself.
   * The one thing we need to deal with is calculating the (new) angles
   * and get rid of the arc drawing altogether if got degenerated.
   */
  angle1 = -atan2(new_startpoint.y - center.y, new_startpoint.x - center.x)*180.0/G_PI;
  while (angle1 < 0.0) angle1 += 360.0;
  angle2 = -atan2(new_endpoint.y - center.y, new_endpoint.x - center.x)*180.0/G_PI;
  while (angle2 < 0.0) angle2 += 360.0;

  /* Only draw it if the original direction is preserved */
  if (is_right_hand (&new_startpoint, midpoint, &new_endpoint) == righthand) {
    /* make it direction aware */
    if (!righthand && angle2 < angle1)
      angle1 -= 360.0;
    else if (righthand && angle2 > angle1)
      angle2 -= 360.0;
    dia_renderer_draw_arc (renderer,
                           &center,
                           width,
                           width,
                           angle1,
                           angle2,
                           color);
  }

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    dia_arrow_draw (start_arrow,
                    renderer,
                    &start_arrow_head,
                    &start_arrow_end,
                    line_width,
                    color,
                    &color_white);
  }

  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    dia_arrow_draw (end_arrow,
                    renderer,
                    &end_arrow_head,
                    &end_arrow_end,
                    line_width,
                    color,
                    &color_white);
  }
}


/*!
 * \brief Draw a bezier line fitting to the given arrows
 *
 * \memberof _DiaRenderer
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
  Point startpoint, endpoint;
  Point start_arrow_head;
  Point end_arrow_head;

  startpoint = points[0].p1;
  endpoint = points[num_points-1].p3;

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    Point move_arrow;
    Point move_line;
    calculate_arrow_point(start_arrow, &points[0].p1, &points[1].p1,
			  &move_arrow, &move_line,
			  line_width);
    start_arrow_head = points[0].p1;
    point_sub(&start_arrow_head, &move_arrow);
    point_sub(&points[0].p1, &move_line);
  }
  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    Point move_arrow;
    Point move_line;
    calculate_arrow_point(end_arrow,
			  &points[num_points-1].p3, &points[num_points-1].p2,
			  &move_arrow, &move_line,
			  line_width);
    end_arrow_head = points[num_points-1].p3;
    point_sub(&end_arrow_head, &move_arrow);
    point_sub(&points[num_points-1].p3, &move_line);
  }
  dia_renderer_draw_bezier (renderer, points, num_points, color);

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    dia_arrow_draw (start_arrow,
                    renderer,
                    &start_arrow_head,
                    &points[1].p1,
                    line_width,
                    color,
                    &color_white);
  }

  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    dia_arrow_draw (end_arrow,
                    renderer,
                    &end_arrow_head,
                    &points[num_points-1].p2,
                    line_width,
                    color,
                    &color_white);
  }

  points[0].p1 = startpoint;
  points[num_points-1].p3 = endpoint;

}
/*! @} */

/*!
 * \name Interactive Renderer Methods
 * @{
 */
/*!
 * \brief Calculate text width of given string with previously set font
 *
 * Should we really provide this? It formerly was an 'interactive op'
 * and depends on DiaRenderer::set_font() not or properly overwritten.
 * As of this writing it is only used for cursor positioning in with
 * an interactive renderer.
 *
 * \memberof _DiaRenderer
 */
static real
get_text_width (DiaRenderer *renderer,
                const gchar *text,
                int          length)
{
  real ret = 0;
  DiaFont *font;
  double font_height;

  font = dia_renderer_get_font (renderer, &font_height);

  if (font) {
    char *str = g_strndup (text, length);

    ret = dia_font_string_width (str, font, font_height);

    g_clear_pointer (&str, g_free);
  } else {
    g_warning ("%s::get_text_width not implemented (and font == NULL)!",
               G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
  }

  return ret;
}

/*! @} */

/*!
 * \brief Advertise special renderer capabilities
 *
 * The base class advertises none of the advanced capabilities, but it
 * has basic transformation support in draw_object() with the help of
 * _DiaTransformRenderer. Only an advanced renderer implementation will
 * overwrite this method with it's own capabilities. Returning TRUE
 * from this method usually requires to adapt at least one other member
 * function, too. Current special capabilities are
 *  - RENDER_HOLES : draw_beziergon() has to support multiple MOVE_TO
 *  - RENDER_ALPHA : the alpha component of _Color is handled to create transparency
 *  - RENDER_AFFINE : at least draw_object() to be overwritten to support affine transformations.
 *    At some point in time also draw_text() and draw_image() need to handle at least rotation.
 *  - RENDER_PATTERN : set_pattern() overwrite and filling with pattern instead of fill color
 *
 * \memberof _DiaRenderer
 */
static gboolean
is_capable_to (DiaRenderer *renderer, RenderCapability cap)
{
  return FALSE;
}

/*!
 * \brief Set the (gradient) pattern for later fill
 * The base class has no pattern (gradient) support
 * \memberof _DiaRenderer \pure
 */
static void
set_pattern (DiaRenderer *renderer, DiaPattern *pat)
{
  g_warning ("%s::set_pattern not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
}

static void
dia_renderer_init (DiaRenderer *self)
{

}

/**
 * dia_renderer_bezier_fill
 * @self: the #DiaRenderer
 * @pts: the #BezPoint s of the bezier
 * @total: the length of @pts
 * @color: colour to fill with
 *
 * Helper function to fill bezier with multiple %BEZ_MOVE_TO
 * A slightly improved version to split a bezier with multiple move-to into
 * a form which can be used with #DiaRenderer not supporting %RENDER_HOLES.
 * With reasonable placement of the second movement it works well for single
 * holes at least. There are artifacts for more complex path to render.
 *
 * Since: 0.98
 */
void
dia_renderer_bezier_fill (DiaRenderer *self,
                          BezPoint    *pts,
                          int          total,
                          Color       *color)
{
  int i;
  gboolean needs_split = FALSE;

  for (i = 1; i < total; ++i) {
    if (BEZ_MOVE_TO == pts[i].type) {
      needs_split = TRUE;
      break;
    }
  }
  if (!needs_split) {
    dia_renderer_draw_beziergon (self, pts, total, color, NULL);
  } else {
    GArray *points = g_array_new (FALSE, FALSE, sizeof(BezPoint));
    Point close_to;
    gboolean needs_close = FALSE;
    /* start with move-to */
    g_array_append_val (points, pts[0]);
    for (i = 1; i < total; ++i) {
      if (BEZ_MOVE_TO == pts[i].type) {
        /* check whether the start point of the second outline is within the first outline. */
        real dist = distance_bez_shape_point (&g_array_index (points, BezPoint, 0), points->len, 0, &pts[i].p1);
        if (dist > 0) { /* outside, just create a new one? */
          /* flush what we have */
          if (needs_close) {
            BezPoint bp;
            bp.type = BEZ_LINE_TO;
            bp.p1 = close_to;
            g_array_append_val(points, bp);
          }
          dia_renderer_draw_beziergon (self,
                                       &g_array_index (points, BezPoint, 0),
                                       points->len,
                                       color,
                                       NULL);
          g_array_set_size (points, 0);
          g_array_append_val (points, pts[i]); /* new needs move-to */
          needs_close = FALSE;
        } else {
          BezPoint bp = pts[i];
          bp.type = BEZ_LINE_TO;
          /* just turn the move- to a line-to */
          g_array_append_val (points, bp);
          /* and remember the point we lined from */
          close_to = (pts[i-1].type == BEZ_CURVE_TO ? pts[i-1].p3 : pts[i-1].p1);
          needs_close = TRUE;
        }
      } else {
        g_array_append_val (points, pts[i]);
      }
    }
    if (points->len > 1) {
      /* actually most renderers need at least three points, but having only one
       * point is an artifact coming from the algorithm above: "new needs move-to" */
      dia_renderer_draw_beziergon (self,
                                   &g_array_index (points, BezPoint, 0),
                                   points->len,
                                   color,
                                   NULL);
    }
    g_array_free (points, TRUE);
  }
}

/*!
 * \brief Helper function to fill bezier with multiple BEZ_MOVE_TO
 * \relates _DiaRenderer
 */
G_GNUC_UNUSED static void
bezier_render_fill_old (DiaRenderer *renderer, BezPoint *pts, int total, Color *color)
{
  int i, n = 0;
  /* first draw the fills */
  int s1 = 0, n1 = 0;
  int s2 = 0;
  for (i = 1; i < total; ++i) {
    if (BEZ_MOVE_TO == pts[i].type) {
      /* check whether the start point of the second outline is within the first outline.
       * If so it need to be subtracted - currently blanked. */
      real dist = distance_bez_shape_point (&pts[s1],  n1 > 0 ? n1 : i - s1, 0, &pts[i].p1);
      if (s2 > s1) { /* blanking the previous one */
        n = i - s2 - 1;
        dia_renderer_draw_beziergon (renderer, &pts[s2], n, &color_white, NULL);
      } else { /* fill the outer shape */
        n1 = n = i - s1;
        dia_renderer_draw_beziergon (renderer, &pts[s1], n, color, NULL);
      }
      if (dist > 0) { /* remember as new outer outline */
        s1 = i;
        n1 = 0;
        s2 = 0;
      } else {
        s2 = i;
      }
    }
  }
  /* the last one is not drawn yet, i is pointing to the last element */
  if (s2 > s1) { /* blanking the previous one */
    if (i - s2 - 1 > 1) { /* depending on the above we may be ready */
      dia_renderer_draw_beziergon (renderer, &pts[s2], i - s2, &color_white, NULL);
    }
  } else {
    if (i - s1 - 1 > 1) {
      dia_renderer_draw_beziergon (renderer, &pts[s1], i - s1, color, NULL);
    }
  }
}

/**
 * dia_renderer_bezier_stroke:
 * @self: the #DiaRenderer
 * @pts: the #BezPoint s to draw
 * @total: length of @pts
 * @color: colour of the stroke
 *
 * Helper function to stroke a bezier with multiple %BEZ_MOVE_TO
 *
 * Since: 0.98
 */
void
dia_renderer_bezier_stroke (DiaRenderer *self,
                            BezPoint    *pts,
                            int          total,
                            Color       *color)
{
  int i, n = 0;
  for (i = 1; i < total; ++i) {
    if (BEZ_MOVE_TO == pts[i].type) {
      dia_renderer_draw_bezier (self, &pts[n], i - n, color);
      n = i;
    }
  }
  /* the last one, if there is one */
  if (i - n > 1) {
    dia_renderer_draw_bezier (self, &pts[n], i - n, color);
  }
}


void
dia_renderer_draw_layer (DiaRenderer      *self,
                         DiaLayer         *layer,
                         gboolean          active,
                         DiaRectangle     *update)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_layer (self, layer, active, update);
}


void
dia_renderer_draw_object (DiaRenderer      *self,
                          DiaObject        *object,
                          DiaMatrix        *matrix)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_object (self, object, matrix);
}


real
dia_renderer_get_text_width (DiaRenderer      *self,
                             const gchar      *text,
                             int               length)
{
  g_return_val_if_fail (DIA_IS_RENDERER (self), 0);

  return DIA_RENDERER_GET_CLASS (self)->get_text_width (self, text, length);
}


void
dia_renderer_begin_render (DiaRenderer        *self,
                           const DiaRectangle *update)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->begin_render (self, update);
}


void
dia_renderer_end_render (DiaRenderer      *self)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->end_render (self);
}


void
dia_renderer_set_linewidth (DiaRenderer      *self,
                            real              linewidth)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->set_linewidth (self, linewidth);
}


void
dia_renderer_set_linecaps (DiaRenderer      *self,
                           DiaLineCaps       mode)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->set_linecaps (self, mode);
}


void
dia_renderer_set_linejoin (DiaRenderer      *self,
                           DiaLineJoin       mode)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->set_linejoin (self, mode);
}


void
dia_renderer_set_linestyle (DiaRenderer      *self,
                            DiaLineStyle      mode,
                            double            length)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->set_linestyle (self, mode, length);
}


void
dia_renderer_set_fillstyle (DiaRenderer      *self,
                            DiaFillStyle         mode)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->set_fillstyle (self, mode);
}


void
dia_renderer_set_font (DiaRenderer      *self,
                       DiaFont          *font,
                       real              height)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  g_object_set (self,
                "font", font,
                "font-height", height,
                NULL);
}

DiaFont *
dia_renderer_get_font (DiaRenderer      *self,
                       double           *height)
{
  DiaFont *font;
  double font_height;

  g_object_get (self,
                "font", &font,
                "font-height", &font_height,
                NULL);

  if (height) {
    *height = font_height;
  }

  return font;
}

void
dia_renderer_draw_line (DiaRenderer      *self,
                        Point            *start,
                        Point            *end,
                        Color            *color)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_line (self, start, end, color);
}


void
dia_renderer_draw_polygon (DiaRenderer      *self,
                           Point            *points,
                           int               num_points,
                           Color            *fill,
                           Color            *stroke)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_polygon (self,
                                               points,
                                               num_points,
                                               fill,
                                               stroke);
}


void
dia_renderer_draw_arc (DiaRenderer      *self,
                       Point            *center,
                       real              width,
                       real              height,
                       real              angle1,
                       real              angle2,
                       Color            *color)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_arc (self,
                                           center,
                                           width,
                                           height,
                                           angle1,
                                           angle2,
                                           color);
}


void
dia_renderer_fill_arc (DiaRenderer      *self,
                       Point            *center,
                       real              width,
                       real              height,
                       real              angle1,
                       real              angle2,
                       Color            *color)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->fill_arc (self,
                                           center,
                                           width,
                                           height,
                                           angle1,
                                           angle2,
                                           color);
}


void
dia_renderer_draw_ellipse (DiaRenderer      *self,
                           Point            *center,
                           real              width,
                           real              height,
                           Color            *fill,
                           Color            *stroke)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_ellipse (self,
                                               center,
                                               width,
                                               height,
                                               fill,
                                               stroke);
}


void
dia_renderer_draw_string (DiaRenderer      *self,
                          const char       *text,
                          Point            *pos,
                          DiaAlignment      alignment,
                          Color            *color)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_string (self, text, pos, alignment, color);
}


void
dia_renderer_draw_image (DiaRenderer      *self,
                         Point            *point,
                         real              width,
                         real              height,
                         DiaImage         *image)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_image (self, point, width, height, image);
}


void
dia_renderer_draw_bezier (DiaRenderer      *self,
                          BezPoint         *points,
                          int               numpoints,
                          Color            *color)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_bezier (self, points, numpoints, color);
}


void
dia_renderer_draw_beziergon (DiaRenderer      *self,
                             BezPoint         *points,
                             int               numpoints,
                             Color            *fill,
                             Color            *stroke)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_beziergon (self,
                                                 points,
                                                 numpoints,
                                                 fill,
                                                 stroke);
}


void
dia_renderer_draw_polyline (DiaRenderer      *self,
                            Point            *points,
                            int               num_points,
                            Color            *color)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_polyline (self,
                                                points,
                                                num_points,
                                                color);
}


void
dia_renderer_draw_text (DiaRenderer      *self,
                        Text             *text)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_text (self, text);
}


void
dia_renderer_draw_text_line (DiaRenderer      *self,
                             TextLine         *text_line,
                             Point            *pos,
                             DiaAlignment      alignment,
                             Color            *color)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_text_line (self,
                                                 text_line,
                                                 pos,
                                                 alignment,
                                                 color);
}


void
dia_renderer_draw_rect (DiaRenderer      *self,
                        Point            *ul_corner,
                        Point            *lr_corner,
                        Color            *fill,
                        Color            *stroke)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_rect (self,
                                            ul_corner,
                                            lr_corner,
                                            fill,
                                            stroke);
}


void
dia_renderer_draw_rounded_rect (DiaRenderer      *self,
                                Point            *ul_corner,
                                Point            *lr_corner,
                                Color            *fill,
                                Color            *stroke,
                                real              radius)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_rounded_rect (self,
                                                    ul_corner,
                                                    lr_corner,
                                                    fill,
                                                    stroke,
                                                    radius);
}


void
dia_renderer_draw_rounded_polyline (DiaRenderer      *self,
                                    Point            *points,
                                    int               num_points,
                                    Color            *color,
                                    real              radius)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_rounded_polyline (self,
                                                        points,
                                                        num_points,
                                                        color,
                                                        radius);
}


void
dia_renderer_draw_line_with_arrows (DiaRenderer      *self,
                                    Point            *start,
                                    Point            *end,
                                    real              line_width,
                                    Color            *line_color,
                                    Arrow            *start_arrow,
                                    Arrow            *end_arrow)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_line_with_arrows (self,
                                                        start,
                                                        end,
                                                        line_width,
                                                        line_color,
                                                        start_arrow,
                                                        end_arrow);
}


void
dia_renderer_draw_arc_with_arrows (DiaRenderer      *self,
                                   Point            *start,
                                   Point            *end,
                                   Point            *midpoint,
                                   real              line_width,
                                   Color            *color,
                                   Arrow            *start_arrow,
                                   Arrow            *end_arrow)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_arc_with_arrows (self,
                                                       start,
                                                       end,
                                                       midpoint,
                                                       line_width,
                                                       color,
                                                       start_arrow,
                                                       end_arrow);
}


void
dia_renderer_draw_polyline_with_arrows (DiaRenderer      *self,
                                        Point            *points,
                                        int               num_points,
                                        real              line_width,
                                        Color            *color,
                                        Arrow            *start_arrow,
                                        Arrow            *end_arrow)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_polyline_with_arrows (self,
                                                            points,
                                                            num_points,
                                                            line_width,
                                                            color,
                                                            start_arrow,
                                                            end_arrow);
}


void
dia_renderer_draw_rounded_polyline_with_arrows (DiaRenderer      *self,
                                                Point            *points,
                                                int               num_points,
                                                real              line_width,
                                                Color            *color,
                                                Arrow            *start_arrow,
                                                Arrow            *end_arrow,
                                                real              radius)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_rounded_polyline_with_arrows (self,
                                                                    points,
                                                                    num_points,
                                                                    line_width,
                                                                    color,
                                                                    start_arrow,
                                                                    end_arrow,
                                                                    radius);
}


void
dia_renderer_draw_bezier_with_arrows (DiaRenderer      *self,
                                      BezPoint         *points,
                                      int               num_points,
                                      real              line_width,
                                      Color            *color,
                                      Arrow            *start_arrow,
                                      Arrow            *end_arrow)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_bezier_with_arrows (self,
                                                          points,
                                                          num_points,
                                                          line_width,
                                                          color,
                                                          start_arrow,
                                                          end_arrow);
}


gboolean
dia_renderer_is_capable_of (DiaRenderer      *self,
                            RenderCapability  cap)
{
  g_return_val_if_fail (DIA_IS_RENDERER (self), FALSE);

  return DIA_RENDERER_GET_CLASS (self)->is_capable_to (self, cap);
}


void
dia_renderer_set_pattern (DiaRenderer      *self,
                          DiaPattern       *pat)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->set_pattern (self, pat);
}


void
dia_renderer_draw_rotated_text (DiaRenderer      *self,
                                Text             *text,
                                Point            *center,
                                real              angle)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_rotated_text (self, text, center, angle);
}


void
dia_renderer_draw_rotated_image (DiaRenderer      *self,
                                 Point            *point,
                                 real              width,
                                 real              height,
                                 real              angle,
                                 DiaImage         *image)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_rotated_image (self,
                                                     point,
                                                     width,
                                                     height,
                                                     angle,
                                                     image);
}
