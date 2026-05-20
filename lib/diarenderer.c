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

#include "object.h"
#include "textline.h"
#include "diatransformrenderer.h"
#include "standard-path.h" /* text_to_path */
#include "boundingbox.h" /* PolyBBextra */
#include "dia-graphene.h"
#include "dia-layer.h"
#include "dia-text.h"
#include "diamarshal.h"

#include "diarenderer.h"


/**
 * DiaRenderer:
 *
 * Base for all of Dia's rendering facilities.
 *
 * Renderers work in close cooperation with [type@Dia.Object]. They provide
 * the way to make all the object drawing independent of concrete drawing
 * back-ends.
 *
 * ## Implementing Renderers
 *
 * ### Low-level Methods
 *
 * A few virtual methods of [type@Dia.Renderer] _must_ be implemented in every
 * derived renderer.
 *
 * - [vfunc@Dia.Renderer.begin_render]
 * - [vfunc@Dia.Renderer.end_render]
 * - [vfunc@Dia.Renderer.set_linewidth]
 * - [vfunc@Dia.Renderer.set_linecaps]
 * - [vfunc@Dia.Renderer.set_linejoin]
 * - [vfunc@Dia.Renderer.set_linestyle]
 * - [vfunc@Dia.Renderer.set_fillstyle]
 * - [vfunc@Dia.Renderer.draw_line]
 * - [vfunc@Dia.Renderer.draw_arc]
 * - [vfunc@Dia.Renderer.fill_arc]
 * - [vfunc@Dia.Renderer.draw_ellipse]
 * - [vfunc@Dia.Renderer.draw_string]
 * - [vfunc@Dia.Renderer.draw_image]
 *
 * Additionally [vfunc@Dia.Renderer.draw_polygon] should be a priority to
 * implement as it's default implementation is very limited.
 *
 * ### Text
 *
 * TODO: Figure this out
 *
 * ### Medium-level Methods
 *
 * The medium level renderer methods have a working fall-back implementation
 * to give the same visual appearance as native member function
 * implementations. However these functions should be overwritten, if the
 * goal is further processing or optimized output.
 *
 * - [vfunc@Dia.Renderer.draw_bezier]
 * - [vfunc@Dia.Renderer.draw_beziergon]
 * - [vfunc@Dia.Renderer.draw_rect]
 * - [vfunc@Dia.Renderer.draw_polyline]
 * - [vfunc@Dia.Renderer.draw_rounded_polyline]
 * - [vfunc@Dia.Renderer.draw_rounded_rect]
 *
 * ### High-level methods
 *
 * #### Arrows
 *
 * A renderer implementation with a compatible concept of arrows should
 * overwrite this function set to get the most high level output. For all
 * other renderer a line with arrows will be split into multiple objects,
 * which still will resemble the original appearance of the diagram.
 *
 * - [vfunc@Dia.Renderer.draw_line_with_arrows]
 * - [vfunc@Dia.Renderer.draw_polyline_with_arrows]
 * - [vfunc@Dia.Renderer.draw_rounded_polyline_with_arrows]
 * - [vfunc@Dia.Renderer.draw_arc_with_arrows]
 * - [vfunc@Dia.Renderer.draw_bezier_with_arrows]
 *
 * #### Layers and Objects
 *
 * If a target format has a notion of logical groups a renderer should
 * consider implementing [vfunc@Dia.Renderer.draw_layer] and
 * [vfunc@Dia.Renderer.draw_object] as they ‘wrap’ the drawing of layers and
 * objects respectively.
 *
 * Since: 0.98
 */


typedef struct _DiaRendererPrivate DiaRendererPrivate;
struct _DiaRendererPrivate {
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


enum {
  FONT_CHANGED,
  N_SIGNALS
};
static guint signals[N_SIGNALS];


struct _BezierApprox {
  Point *points;
  int numpoints;
  int currpoint;
};


static void
bezier_approx_free (BezierApprox *self)
{
  g_clear_pointer (&self->points, g_free);
  g_free (self);
}


static void
dia_renderer_dispose (GObject *object)
{
  DiaRenderer *renderer = DIA_RENDERER (object);
  DiaRendererPrivate *priv = dia_renderer_get_instance_private (renderer);

  g_clear_object (&priv->font);

  g_clear_pointer (&priv->bezier, bezier_approx_free);

  G_OBJECT_CLASS (dia_renderer_parent_class)->dispose (object);
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
      dia_renderer_set_font (self,
                             g_value_get_object (value),
                             priv->font_height);
      break;
    case PROP_FONT_HEIGHT:
      dia_renderer_set_font (self,
                             priv->font,
                             g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


/**
 * DiaRendererClass::draw_layer:
 * @self: the [type@Dia.Renderer]
 * @layer: layer to draw.
 * @active: %TRUE if it is the currently active layer.
 * @update: the update rectangle, %NULL for unlimited.
 *
 * Render all the visible object in the layer.
 *
 * ## Default Implementation
 *
 * Given an update rectangle calls the [method@Dia.Renderer.draw_object] only
 * for intersected objects. This method does _not_ care for layer visibility,
 * though. If an exporter wants to 'see' also invisible layers this method
 * needs to be overwritten. Also it does not pass any matrix to
 * [method@Dia.Renderer.draw_object].
 *
 * ::: tip "Render Model Layer"
 *     High-level.
 */
static void
dia_renderer_real_draw_layer (DiaRenderer  *self,
                              DiaLayer     *layer,
                              gboolean      active,
                              DiaRectangle *update)
{
  GList *list = dia_layer_get_object_list (layer);

  g_return_if_fail (layer != NULL);

  /* Draw all objects  */
  while (list!=NULL) {
    DiaObject *obj = (DiaObject *) list->data;

    if (update == NULL ||
        rectangle_intersects (update, dia_object_get_enclosing_box (obj))) {
      dia_renderer_draw_object (self, obj, NULL);
    }
    list = g_list_next (list);
  }
}


/**
 * DiaRendererClass::draw_object:
 * @self: the [type@Dia.Renderer]
 * @object:
 * @matrix:
 *
 * Render the given object with optional transformation matrix.
 *
 * A renderer capable of affine transformation should impelment this method.
 *
 * ## Default Implementation
 *
 * If matrix is %NULL calls [method@Dia.Object.draw] with the given renderer,
 * otherwise the object is transformed with the help of
 * [type@Dia.TransformRenderer].
 *
 * ::: tip "Render Model Layer"
 *     High-level.
 */
static void
dia_renderer_real_draw_object (DiaRenderer *self,
                               DiaObject   *object,
                               DiaMatrix   *matrix)
{
  if (matrix) {
    DiaRenderer *tr = dia_transform_renderer_new (self);

    dia_renderer_draw_object (tr, object, matrix);

    g_clear_object (&tr);

    return;
  }

  dia_object_draw (object, self);
}


/**
 * DiaRendererClass::begin_render:
 * @self: the [type@Dia.Renderer]
 * @update:
 *
 * Called before rendering begins.
 *
 * Can be used to do various pre-rendering setup.
 *
 * ::: tip "Render Model Layer"
 *     Low-level.
 */
static void
dia_renderer_real_begin_render (DiaRenderer        *self,
                                const DiaRectangle *update)
{
  g_warning ("%s::begin_render not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
}


/**
 * DiaRendererClass::end_render:
 * @self: the [type@Dia.Renderer]
 *
 * Used to do various clean-ups.
 *
 * ::: tip "Render Model Layer"
 *     Low-level.
 */
static void
dia_renderer_real_end_render (DiaRenderer *self)
{
  g_warning ("%s::end_render not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
}


/**
 * DiaRendererClass::set_linewidth:
 * @self: the [type@Dia.Renderer]
 * @line_width:
 *
 * Change the line width for the strokes to come.
 *
 * ::: tip "Render Model Layer"
 *     Low-level.
 */
static void
dia_renderer_real_set_linewidth (DiaRenderer *self, double line_width)
{
  g_warning ("%s::set_line_width not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
}


/**
 * DiaRendererClass::set_linecaps:
 * @self: the [type@Dia.Renderer]
 * @line_caps: a [type@Dia.LineCaps]
 *
 * Change the line caps for the strokes to come.
 *
 * ::: tip "Render Model Layer"
 *     Low-level.
 */
static void
dia_renderer_real_set_linecaps (DiaRenderer *self, DiaLineCaps line_caps)
{
  g_warning ("%s::set_line_caps not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
}


/**
 * DiaRendererClass::set_linejoin:
 * @self: the [type@Dia.Renderer]
 * @line_join: a [type@Dia.LineJoin]
 *
 * Change the line join mode for the strokes to come.
 *
 * ::: tip "Render Model Layer"
 *     Low-level.
 */
static void
dia_renderer_real_set_linejoin (DiaRenderer *self, DiaLineJoin line_join)
{
  g_warning ("%s::set_line_join not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
}


/**
 * DiaRendererClass::set_linestyle:
 * @self: the [type@Dia.Renderer]
 * @line_syle: a [type@Dia.LineStyle]
 * @dash_length:
 *
 * Change line style and dash length for the strokes to come.
 *
 * ::: tip "Render Model Layer"
 *     Low-level.
 */
static void
dia_renderer_real_set_linestyle (DiaRenderer  *self,
                                 DiaLineStyle  line_syle,
                                 double        dash_length)
{
  g_warning ("%s::set_line_style not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
}


/**
 * DiaRendererClass::set_fillstyle:
 * @self: the [type@Dia.Renderer]
 * @fill_style: a [type@Dia.FillStyle]
 *
 * Set the fill mode for following fills.
 *
 * As of this writing there is only one fill mode defined, so this function
 * might never get called, because it does not make a difference.
 *
 * ::: tip "Render Model Layer"
 *     Low-level.
 */
static void
dia_renderer_real_set_fillstyle (DiaRenderer *self, DiaFillStyle fill_style)
{
  g_warning ("%s::set_fill_style not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
}


/**
 * DiaRendererClass::font_changed:
 * @self: the [type@Dia.Renderer]
 * @font: the newly active [type@Dia.Font]
 * @font_height: the newly active font height
 *
 * Implement thus vfunc to observe changes in the active font.
 *
 * Since: 0.98
 */
static void
dia_renderer_real_font_changed (DiaRenderer *self,
                                DiaFont     *font,
                                double       font_height)
{

}


/**
 * DiaRendererClass::draw_line:
 * @self: the [type@Dia.Renderer]
 * @start_point: the start [type@Dia.Point] of the line
 * @end_point: the end [type@Dia.Point] of the line
 * @line_colour: the [type@Dia.Colour] to stroke
 *
 * Draw a single line segment.
 *
 * ::: tip "Render Model Layer"
 *     Low-level.
 */
static void
dia_renderer_real_draw_line (DiaRenderer *self,
                             Point       *start_point,
                             Point       *end_point,
                             DiaColour   *line_colour)
{
  g_warning ("%s::draw_line not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
}


/**
 * DiaRendererClass::draw_polygon:
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points):
 * @n_points: the number of @points
 * @fill: (nullable): the [type@Dia.Colour] to fill
 * @stroke: (nullable): the [type@Dia.Colour] to stroke
 *
 * Fill and/or stroke a polygon.
 *
 * ## Default Implementation
 *
 * Draws as a series of lines, largely ignoring fill. Relying on this should
 * be a last-resort.
 *
 * ::: tip "Render Model Layer"
 *     Low-level.
 */
static void
dia_renderer_real_draw_polygon (DiaRenderer *self,
                                Point       *points,
                                int          n_points,
                                DiaColour   *fill,
                                DiaColour   *stroke)
{
  DiaColour *colour = fill ? fill : stroke;

  g_return_if_fail (n_points > 1);
  g_return_if_fail (colour != NULL);

  for (int i = 0; i < n_points - 1; i++) {
    dia_renderer_draw_line (self,
                            &points[i + 0],
                            &points[i + 1],
                            colour);
  }

  /* close it in any case */
  if ((points[0].x != points[n_points-1].x) ||
      (points[0].y != points[n_points-1].y)) {
    dia_renderer_draw_line (self,
                            &points[n_points - 1],
                            &points[0],
                            colour);
  }
}


/**
 * DiaRendererClass::draw_arc:
 * @self: the [type@Dia.Renderer]
 * @centre:
 * @width:
 * @height:
 * @angle1:
 * @angle2:
 * @line_colour: the [type@Dia.Colour] to stroke
 *
 * Draw an arc, given its centre, the bounding box (widget, height), the
 * start angle and the end angle. It's counter-clockwise if
 * `angle2 > angle1`.
 *
 * ::: tip "Render Model Layer"
 *     Low-level.
 */
static void
dia_renderer_real_draw_arc (DiaRenderer *self,
                            Point       *centre,
                            double       width,
                            double       height,
                            double       angle1,
                            double       angle2,
                            DiaColour   *line_colour)
{
  g_warning ("%s::draw_arc not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
}


/**
 * DiaRendererClass::fill_arc:
 * @self: the [type@Dia.Renderer]
 * @centre:
 * @width:
 * @height:
 * @angle1:
 * @angle2:
 * @fill_colour: the [type@Dia.Colour] to fill
 *
 * Fill an arc (a pie-chart)
 *
 * ::: tip "Render Model Layer"
 *     Low-level.
 */
static void
dia_renderer_real_fill_arc (DiaRenderer *self,
                            Point       *centre,
                            double       width,
                            double       height,
                            double       angle1,
                            double       angle2,
                            DiaColour   *fill_colour)
{
  g_warning ("%s::fill_arc not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
}


/**
 * DiaRendererClass::draw_ellipse:
 * @self: the [type@Dia.Renderer]
 * @centre:
 * @width:
 * @height:
 * @fill: (nullable): the [type@Dia.Colour] to fill
 * @stroke: (nullable): the [type@Dia.Colour] to stroke
 *
 * Fill and/or stroke an ellipse.
 *
 * ::: tip "Render Model Layer"
 *     Low-level.
 */
static void
dia_renderer_real_draw_ellipse (DiaRenderer *self,
                                Point       *centre,
                                double       width,
                                double       height,
                                DiaColour   *fill,
                                DiaColour   *stroke)
{
  g_warning ("%s::draw_ellipse not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
}


/**
 * DiaRendererClass::draw_string:
 * @self: the [type@Dia.Renderer]
 * @text: the string to draw
 * @pos:
 * @alignment:
 * @text_colour: the [type@Dia.Colour] for the text
 *
 * Draw a string
 *
 * ::: tip "Render Model Layer"
 *     Low-level.
 */
static void
dia_renderer_real_draw_string (DiaRenderer  *self,
                               const char   *text,
                               Point        *pos,
                               DiaAlignment  alignment,
                               DiaColour    *text_colour)
{
  g_warning ("%s::draw_string not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
}


/**
 * DiaRendererClass::draw_image:
 * @self: the [type@Dia.Renderer]
 * @point:
 * @width:
 * @height:
 * @image: the [type@Dia.Image] to draw
 *
 * Draw an image (pixbuf).
 *
 * ::: tip "Render Model Layer"
 *     Low-level.
 */
static void
dia_renderer_real_draw_image (DiaRenderer *self,
                              Point       *point,
                              double       width,
                              double       height,
                              DiaImage    *image)
{
  g_warning ("%s::draw_image not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
}


/**
 * DiaRendererClass::draw_text:
 * @self: the [type@Dia.Renderer]
 * @text: the [type@Dia.Text] to draw
 *
 * ## Default Implementation
 *
 * Splits the given [type@Dia.Text] object into single lines and passes them
 * to [method@Dia.Renderer.draw_text_line]. A Renderer with a concept of
 * multi-line text should overwrite it.
 *
 * ::: tip "Render Model Layer"
 *     Medium-level.
 */
static void
dia_renderer_real_draw_text (DiaRenderer *self, DiaText *text)
{
  size_t n_lines;
  TextLine **lines = dia_text_get_lines (text, &n_lines);
  DiaColour text_colour;
  Point pos;

  dia_text_get_colour (text, &text_colour);
  dia_text_get_position (text, &pos);

  for (size_t i = 0; i < n_lines;i++) {
    dia_renderer_draw_text_line (self,
                                 lines[i],
                                 &pos,
                                 dia_text_get_alignment (text),
                                 &text_colour);
    pos.y += dia_text_get_height (text);
  }
}


/**
 * DiaRendererClass::draw_rotated_text:
 * @self: the [type@Dia.Renderer]
 * @text: the [type@Dia.Text]
 * @centre: where to draw @text
 * @angle: how much to rotate @text about @centre
 *
 * ## Default Implementation
 *
 * Converts the given [type@Dia.Text] object to a path and passes it to
 * [method@Dia.Renderer.draw_beziergon] if the renderer supports rendering
 * with holes. If not a fallback implementation is used.
 *
 * A Renderer with a good own concept of rotated text should
 * overwrite it.
 *
 * ::: tip "Render Model Layer"
 *     Medium-level.
 */
static void
dia_renderer_real_draw_rotated_text (DiaRenderer *self,
                                     DiaText     *text,
                                     Point       *centre,
                                     double       angle)
{
  if (angle == 0.0) {
    /* maybe the fallback should also consider centre? */
    dia_renderer_draw_text (self, text);
  } else {
    Point text_position;
    DiaColour text_colour;
    GArray *path = g_array_new (FALSE, FALSE, sizeof (BezPoint));

    dia_text_get_position (text, &text_position);
    dia_text_get_colour (text, &text_colour);

    if (!dia_text_is_empty (text) && text_to_path (text, path)) {
      /* Scaling and transformation here */
      DiaRectangle bz_bb, tx_bb;
      PolyBBExtras extra = { 0, };
      double sx, sy;
      guint i;
      double dx = centre ? (text_position.x - centre->x) : 0;
      double dy = centre ? (text_position.y - centre->y) : 0;
      DiaMatrix m = { 1, 0, 0, 1, 0, 0 };
      DiaMatrix t = { 1, 0, 0, 1, 0, 0 };

      polybezier_bbox (&g_array_index (path, BezPoint, 0), path->len, &extra, TRUE, &bz_bb);
      dia_text_calc_boundingbox (text, &tx_bb);
      sx = (tx_bb.right - tx_bb.left) / (bz_bb.right - bz_bb.left);
      sy = (tx_bb.bottom - tx_bb.top) / (bz_bb.bottom - bz_bb.top);

      /* move centre to origin */
      if (DIA_ALIGN_LEFT == dia_text_get_alignment (text)) {
        t.x0 = -bz_bb.left;
      } else if (DIA_ALIGN_RIGHT == dia_text_get_alignment (text)) {
        t.x0 = - bz_bb.right;
      } else {
        t.x0 = -(bz_bb.left + bz_bb.right) / 2.0;
      }
      t.x0 -= dx / sx;
      t.y0 = - bz_bb.top - (dia_text_get_ascent (text) - dy) / sy;
      dia_matrix_set_angle_and_scales (&m, G_PI * angle / 180.0, sx, sx);
      dia_matrix_multiply (&m, &t, &m);
      /* move back centre from origin */
      if (DIA_ALIGN_LEFT == dia_text_get_alignment (text)) {
        t.x0 = tx_bb.left;
      } else if (DIA_ALIGN_RIGHT == dia_text_get_alignment (text)) {
        t.x0 = tx_bb.right;
      } else {
        t.x0 = (tx_bb.left + tx_bb.right) / 2.0;
      }
      t.x0 += dx;
      t.y0 = tx_bb.top + (dia_text_get_ascent (text) - dy);
      dia_matrix_multiply (&m, &m, &t);

      for (i = 0; i < path->len; ++i) {
        BezPoint *bp = &g_array_index (path, BezPoint, i);
        transform_bezpoint (bp, &m);
      }

      if (dia_renderer_is_capable_of (self, DIA_RENDER_HOLES)) {
        dia_renderer_draw_beziergon (self,
                                     &g_array_index (path, BezPoint, 0),
                                     path->len,
                                     &text_colour,
                                     NULL);
      } else {
        dia_renderer_bezier_fill (self,
                                  &g_array_index (path, BezPoint, 0),
                                  path->len,
                                  &text_colour);
      }
    } else {
      DiaColour magenta = { 1.0, 0.0, 1.0, 1.0 };
      Point pt = centre ? *centre : text_position;
      DiaMatrix m = { 1, 0, 0, 1, pt.x, pt.y };
      DiaMatrix t = { 1, 0, 0, 1, -pt.x, -pt.y };
      DiaRectangle tb;
      Point poly[4];
      int i;

      dia_text_calc_boundingbox (text, &tb);
      poly[0].x = tb.left;  poly[0].y = tb.top;
      poly[1].x = tb.right; poly[1].y = tb.top;
      poly[2].x = tb.right; poly[2].y = tb.bottom;
      poly[3].x = tb.left;  poly[3].y = tb.bottom;

      dia_matrix_set_angle_and_scales (&m, G_PI * angle / 180.0, 1.0, 1.0);
      dia_matrix_multiply (&m, &t, &m);

      for (i = 0; i < 4; ++i) {
        transform_point (&poly[i], &m);
      }

      dia_renderer_set_linewidth (self, 0.0);
      dia_renderer_draw_polygon (self,
                                 poly,
                                 4,
                                 NULL,
                                 &magenta);
    }

    g_array_free (path, TRUE);
  }
}


/**
 * DiaRendererClass::draw_rotated_image:
 * @self: the [type@Dia.Renderer]
 * @point:
 * @width:
 * @height:
 * @angle:
 * @image: the [type@Dia.Image] to draw
 *
 * ::: tip "Render Model Layer"
 *     Medium-level.
 */
static void
dia_renderer_real_draw_rotated_image (DiaRenderer *self,
                                      Point       *point,
                                      double       width,
                                      double       height,
                                      double       angle,
                                      DiaImage    *image)
{
  if (angle == 0.0) {
    dia_renderer_draw_image (self, point, width, height, image);
  } else {
    /* XXX: implement fallback */
    g_warning ("Ignoring image rotation");
    dia_renderer_draw_image (self, point, width, height, image);
  }
}


/**
 * DiaRendererClass::draw_text_line:
 * @self: the [type@Dia.Renderer]
 * @text_line: the [type@Dia.TextLine] to draw
 * @pos: where to draw @text_line
 * @alignment: text [type@Dia.Alignment]
 * @text_colour: text [type@Dia.Colour]
 *
 * ## Default Implementation
 *
 * Calls [method@Dia.Renderer.set_font] and [method@Dia.Renderer.draw_string].
 *
 * ::: tip "Render Model Layer"
 *     Medium-level.
 */
static void
dia_renderer_real_draw_text_line (DiaRenderer  *self,
                                  TextLine     *text_line,
                                  Point        *pos,
                                  DiaAlignment  alignment,
                                  Color        *text_color)
{
  dia_renderer_set_font (self,
                         text_line_get_font (text_line),
                         text_line_get_height (text_line));

  dia_renderer_draw_string (self,
                            text_line_get_string (text_line),
                            pos,
                            alignment,
                            text_color);
}


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
bezier_add_point (BezierApprox *bezier,
                  Point        *point)
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
bezier_add_lines (BezierApprox *bezier,
                  Point         points[4])
{
  Point u, v, x, y;
  Point r[4];
  Point s[4];
  Point middle;
  double delta;
  double v_len_sq;

  /* Check if almost flat: */
  u = points[1];
  point_sub (&u, &points[0]);
  v = points[3];
  point_sub (&v, &points[0]);
  y = v;
  v_len_sq = point_dot (&v,&v);
  if (isnan (v_len_sq)) {
    g_warning ("v_len_sq is NaN while calculating bezier curve!");
    return;
  }

  if (v_len_sq < 0.000001) {
    v_len_sq = 0.000001;
  }

  point_scale (&y, point_dot (&u, &v) / v_len_sq);
  x = u;
  point_sub (&x,&y);
  delta = point_dot (&x,&x);
  if (delta < BEZIER_SUBDIVIDE_LIMIT_SQ) {
    u = points[2];
    point_sub (&u, &points[3]);
    v = points[0];
    point_sub (&v, &points[3]);
    y = v;
    v_len_sq = point_dot (&v,&v);
    if (v_len_sq < 0.000001) {
      v_len_sq = 0.000001;
    }
    point_scale (&y, point_dot (&u, &v) / v_len_sq);
    x = u;
    point_sub (&x,&y);
    delta = point_dot (&x,&x);
    if (delta < BEZIER_SUBDIVIDE_LIMIT_SQ) { /* Almost flat, draw a line */
      bezier_add_point (bezier, &points[3]);
      return;
    }
  }
  /* Subdivide into two bezier curves: */

  middle = points[1];
  point_add (&middle, &points[2]);
  point_scale (&middle, 0.5);

  r[0] = points[0];

  r[1] = points[0];
  point_add (&r[1], &points[1]);
  point_scale (&r[1], 0.5);

  r[2] = r[1];
  point_add (&r[2], &middle);
  point_scale (&r[2], 0.5);

  s[3] = points[3];

  s[2] = points[2];
  point_add (&s[2], &points[3]);
  point_scale (&s[2], 0.5);

  s[1] = s[2];
  point_add (&s[1], &middle);
  point_scale (&s[1], 0.5);

  r[3] = r[2];
  point_add (&r[3], &s[1]);
  point_scale (&r[3], 0.5);

  s[0] = r[3];
  bezier_add_lines (bezier, r);
  bezier_add_lines (bezier, s);
}


static void
bezier_add_curve (BezierApprox *bezier,
                  Point         points[4])
{
  /* Is the bezier curve malformed? */
  if ((distance_point_point (&points[0], &points[1]) < 0.00001) &&
      (distance_point_point (&points[2], &points[3]) < 0.00001) &&
      (distance_point_point (&points[0], &points[3]) < 0.00001)) {
    bezier_add_point (bezier, &points[3]);
  }

  bezier_add_lines (bezier, points);
}


static void
approximate_bezier (BezierApprox *bezier,
                    BezPoint     *points,
                    int           numpoints)
{
  Point curve[4];

  if (points[0].type != BEZ_MOVE_TO) {
    g_warning ("first BezPoint must be a BEZ_MOVE_TO");
  }

  curve[3] = points[0].p1;
  bezier_add_point (bezier, &points[0].p1);

  for (int i = 1; i < numpoints; i++) {
    switch (points[i].type) {
      case BEZ_MOVE_TO:
        g_warning ("only first BezPoint can be a BEZ_MOVE_TO");
        curve[3] = points[i].p1;
        break;
      case BEZ_LINE_TO:
        bezier_add_point (bezier, &points[i].p1);
        curve[3] = points[i].p1;
        break;
      case BEZ_CURVE_TO:
        curve[0] = curve[3];
        curve[1] = points[i].p1;
        curve[2] = points[i].p2;
        curve[3] = points[i].p3;
        bezier_add_curve (bezier, curve);
        break;
      default:
        g_return_if_reached ();
    }
  }
}


/**
 * DiaRendererClass::draw_bezier:
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points):
 * @n_points: the number of @points
 * @line_colour: the [type@Dia.Colour] to stroke
 *
 * Draw a bezier curve, given it's control points
 *
 * The first [type@Dia.BezPoint] must be of type
 * [enum@Dia.BezPointType.MOVE_TO], and no other ones may be
 * [enum@Dia.BezPointType.MOVE_TO]s. If further holes are supported by a
 * specific renderer should be checked with
 * [method@Dia.Renderer.is_capable_to] ([flags@Dia.RenderCapability.HOLES]).
 *
 * ## Default Implementation
 *
 * Converts the given path into a polyline approximation and calls
 * [method@Dia.Renderer.draw_polyline].
 *
 * ::: tip "Render Model Layer"
 *     Medium-level.
 */
static void
dia_renderer_real_draw_bezier (DiaRenderer *self,
                               BezPoint    *points,
                               int          n_points,
                               DiaColour   *line_colour)
{
  DiaRendererPrivate *priv = dia_renderer_get_instance_private (self);
  BezierApprox *bezier;

  if (priv->bezier) {
    bezier = priv->bezier;
  } else {
    priv->bezier = bezier = g_new0 (BezierApprox, 1);
  }

  if (bezier->points == NULL) {
    bezier->numpoints = 30;
    bezier->points = g_new0 (Point, bezier->numpoints);
  }

  bezier->currpoint = 0;
  approximate_bezier (bezier, points, n_points);

  dia_renderer_draw_polyline (self,
                              bezier->points,
                              bezier->currpoint,
                              line_colour);
}


/**
 * DiaRendererClass::draw_beziergon:
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points):
 * @n_points: number of @points
 * @fill: (nullable): the [type@Dia.Colour] to fill
 * @stroke: (nullable): the [type@Dia.Colour] to stroke
 *
 * Fill and/or stroke a closed bezier curve.
 *
 * ## Default Implementation
 *
 * Can only handle a single [enum@Dia.BezPointType.MOVE_TO] at the first
 * point. It emulates the actual drawing with an approximated polyline.
 *
 * ::: tip "Render Model Layer"
 *     Medium-level.
 */
static void
dia_renderer_real_draw_beziergon (DiaRenderer *self,
                                  BezPoint    *points,
                                  int          numpoints,
                                  DiaColour   *fill,
                                  DiaColour   *stroke)
{
  DiaRendererPrivate *priv = dia_renderer_get_instance_private (self);
  BezierApprox *bezier;

  g_return_if_fail (fill != NULL || stroke != NULL);

  if (priv->bezier) {
    bezier = priv->bezier;
  } else {
    priv->bezier = bezier = g_new0 (BezierApprox, 1);
  }

  if (bezier->points == NULL) {
    bezier->numpoints = 30;
    bezier->points = g_new0 (Point, bezier->numpoints);
  }

  bezier->currpoint = 0;
  approximate_bezier (bezier, points, numpoints);

  if (fill || stroke) {
    dia_renderer_draw_polygon (self,
                               bezier->points,
                               bezier->currpoint,
                               fill,
                               stroke);
  }
}


/**
 * DiaRendererClass::draw_rect:
 * @self: the [type@Dia.Renderer]
 * @ul_corner: upper-left point
 * @lr_corner: lower-right point
 * @fill: (nullable): the [type@Dia.Colour] to fill
 * @stroke: (nullable): the [type@Dia.Colour] to stroke
 *
 * Stroke and/or fill a rectangle
 *
 * This only needs to be implemented in the derived class if it differs from
 * [vfunc@Dia.Renderer.draw_polygon]. Given that
 * [vfunc@Dia.Renderer.draw_polygon] is a required method we can use that
 * instead of forcing every inherited class to implement
 * [vfunc@Dia.Renderer.draw_rect], too.
 *
 * ::: tip "Render Model Layer"
 *     Medium-level.
 */
static void
dia_renderer_real_draw_rect (DiaRenderer *self,
                             Point       *ul_corner,
                             Point       *lr_corner,
                             DiaColour   *fill,
                             DiaColour   *stroke)
{
  if (DIA_RENDERER_GET_CLASS (self)->draw_polygon == &dia_renderer_real_draw_polygon) {
    g_warning ("%s::draw_rect and draw_polygon not implemented!",
               G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
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
    dia_renderer_draw_polygon (self, corner, 4, fill, stroke);
  }
}


/**
 * DiaRendererClass::draw_polyline:
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points): set of [type@Dia.Point]s to draw between
 * @n_points: the number of @points
 * @line_colour: the [type@Dia.Arrow] to stroke
 *
 * Draw a polyline with the given colour
 *
 * ## Default Implementation
 *
 * Delegates drawing to consecutive calls to [method@Dia.Renderer.draw_line].
 *
 * ::: tip "Render Model Layer"
 *     Medium-level.
 */
static void
dia_renderer_real_draw_polyline (DiaRenderer *self,
                                 Point       *points,
                                 int          n_points,
                                 DiaColour   *line_colour)
{
  for (int i = 0; i < n_points - 1; i++) {
    dia_renderer_draw_line (self,
                            &points[i + 0],
                            &points[i + 1],
                            line_colour);
  }
}


/* calculate the maximum possible radius for 3 points
 *   use the following,
 *   given points p1,p2, and p3
 *   let c = min(length(p1,p2)/2,length(p2,p3)/2)
 *   let a = dot2(p1-p2, p3-p2)
 *     (ie, angle between lines p1,p2 and p2,p3)
 *   then maxr = c * sin(a/2)
 */
static double
calculate_min_radius (Point *p1, Point *p2, Point *p3)
{
  double c;
  double a;
  Point v1,v2;

  c = MIN (distance_point_point (p1, p2) / 2,
           distance_point_point (p2, p3) / 2);
  v1.x = p1->x-p2->x; v1.y = p1->y-p2->y;
  v2.x = p3->x-p2->x; v2.y = p3->y-p2->y;
  a = dot2 (&v1, &v2);
  return (c * sin (a / 2));
}


/**
 * DiaRendererClass::draw_rounded_polyline:
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points):
 * @n_points: the number of @points
 * @line_colour: the [type@Dia.Colour] to stroke
 * @radius:
 *
 * Draw a polyline with optionally rounded corners
 *
 * ## Default Implementation
 *
 * Based on [method@Dia.Renderer.draw_line] and
 * [method@Dia.Renderer.draw_arc], but uses
 * [method@Dia.Renderer.draw_polyline] when the rounding is too small.
 *
 * ::: tip "Render Model Layer"
 *     Medium-level.
 */
static void
dia_renderer_real_draw_rounded_polyline (DiaRenderer *renderer,
                                         Point       *points,
                                         int          n_points,
                                         DiaColour   *line_colour,
                                         double       radius)
{
  int i = 0;
  Point p1, p2, p3, p4;
  Point *p;
  p = points;

  if (radius < 0.00001) {
    dia_renderer_draw_polyline (renderer, points, n_points, line_colour);
    return;
  }

  /* skip arc computations if we only have points */
  if (n_points <= 2) {
    i = 0;

    p1.x = p[i].x;
    p1.y = p[i].y;
    p2.x = p[i + 1].x;
    p2.y = p[i + 1].y;

    dia_renderer_draw_line (renderer, &p1, &p2, line_colour);

    return;
  }

  i = 0;
  /* full rendering 3 or more points */
  p1.x = p[i].x;
  p1.y = p[i].y;
  p2.x = p[i + 1].x;
  p2.y = p[i + 1].y;
  for (i = 0; i <= n_points - 3; i++) {
    Point c;
    double start_angle, stop_angle;
    double min_radius;
    gboolean arc_it;

    p3.x = p[i + 1].x;
    p3.y = p[i + 1].y;
    p4.x = p[i + 2].x;
    p4.y = p[i + 2].y;

    /* adjust the radius if it would cause odd rendering */
    min_radius = MIN (radius, calculate_min_radius (&p1, &p2, &p4));
    arc_it = fillet (&p1,
                     &p2,
                     &p3,
                     &p4,
                     min_radius,
                     &c,
                     &start_angle,
                     &stop_angle);
    /* start with the line drawing to allow joining in backend */
    dia_renderer_draw_line (renderer, &p1, &p2, line_colour);
    if (arc_it) {
      dia_renderer_draw_arc (renderer,
                             &c,
                             min_radius * 2,
                             min_radius * 2,
                             start_angle,
                             stop_angle,
                             line_colour);
    }

    p1.x = p3.x;
    p1.y = p3.y;
    p2.x = p4.x;
    p2.y = p4.y;
  }

  dia_renderer_draw_line (renderer, &p3, &p4, line_colour);
}


/**
 * DiaRendererClass::draw_rounded_rect:
 * @self: the [type@Dia.Renderer]
 * @ul_corner: upper-left corner
 * @lr_corner: lower-right corner
 * @fill: (nullable): the [type@Dia.Colour] to fill
 * @stroke: (nullable): the [type@Dia.Colour] to stroke
 * @radius:
 *
 * Fill and/or stroke a rectangle with rounded corners
 *
 * ## Default Implementation
 *
 * Assembles a rectangle with potentially rounded corners from consecutive
 * [method@Dia.Renderer.draw_arc] and [method@Dia.Renderer.draw_line] calls
 * when stroked.
 *
 * Filling is done by two overlapping rectangles and four
 * [method@Dia.Renderer.fill_arc] calls.
 *
 * ::: tip "Render Model Layer"
 *     High-level.
 */
static void
dia_renderer_real_draw_rounded_rect (DiaRenderer *renderer,
                                     Point       *ul_corner,
                                     Point       *lr_corner,
                                     DiaColour   *fill,
                                     DiaColour   *stroke,
                                     double       radius)
{
  /* clip radius per axis to use the full API;) */
  double rw = MIN (radius, (lr_corner->x - ul_corner->x) / 2);
  double rh = MIN (radius, (lr_corner->y - ul_corner->y) / 2);

  if (rw < 0.00001 || rh < 0.00001) {
    dia_renderer_draw_rect (renderer, ul_corner, lr_corner, fill, stroke);
  } else {
    double tlx = ul_corner->x; /* top-left x */
    double tly = ul_corner->y; /* top-left y */
    double brx = lr_corner->x; /* bottom-right x */
    double bry = lr_corner->y; /* bottom-right y */
    /* calculate all start/end points needed in advance, counter-clockwise */
    Point pts[8];
    Point cts[4]; /* ... and centres */

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

    for (int i = 0; i < 4; ++i) {
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


/**
 * dia_renderer_real_draw_line_with_arrows:
 * @self: the [type@Dia.Renderer]
 * @start_point: the start [type@Dia.Point] of the line
 * @end_point: the end [type@Dia.Point] of the line
 * @line_width:
 * @line_colour: the [type@Dia.Colour] to stroke
 * @start_arrow: (nullable): the start [type@Dia.Arrow]
 * @end_arrow: (nullable): the end [type@Dia.Arrow]
 *
 * Draw a line fitting to the given arrows
 *
 * ::: tip "Render Model Layer"
 *     High-level.
 */
static void
dia_renderer_real_draw_line_with_arrows (DiaRenderer *renderer,
                                         Point       *start_point,
                                         Point       *end_point,
                                         double       line_width,
                                         DiaColour   *line_colour,
                                         Arrow       *start_arrow,
                                         Arrow       *end_arrow)
{
  Point oldstart = *start_point;
  Point oldend = *end_point;
  Point start_arrow_head;
  Point end_arrow_head;

  /* Calculate how to more the line to account for arrow heads */
  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    calculate_arrow_point (start_arrow,
                           start_point,
                           end_point,
                           &move_arrow,
                           &move_line,
                           line_width);
    start_arrow_head = *start_point;
    point_sub (&start_arrow_head, &move_arrow);
    point_sub (start_point, &move_line);
  }

  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    calculate_arrow_point (end_arrow,
                           end_point,
                           start_point,
                           &move_arrow,
                           &move_line,
                           line_width);
    end_arrow_head = *end_point;
    point_sub (&end_arrow_head, &move_arrow);
    point_sub (end_point, &move_line);
  }

  dia_renderer_draw_line (renderer, start_point, end_point, line_colour);

  /* Actual arrow drawing down here so line styles aren't disturbed */
  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    dia_arrow_draw (start_arrow,
                    renderer,
                    &start_arrow_head,
                    end_point,
                    line_width,
                    line_colour,
                    &DIA_COLOUR_WHITE);
  }

  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    dia_arrow_draw (end_arrow,
                    renderer,
                    &end_arrow_head,
                    start_point,
                    line_width,
                    line_colour,
                    &DIA_COLOUR_WHITE);
  }

  *start_point = oldstart;
  *end_point = oldend;
}


/**
 * dia_renderer_real_draw_polyline_with_arrows:
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points):
 * @n_points: number of @points
 * @line_width:
 * @line_colour: the [type@Dia.Colour] to stroke
 * @start_arrow: (nullable): the start [type@Dia.Arrow]
 * @end_arrow: (nullable): the end [type@Dia.Arrow]
 *
 * Draw a polyline fitting to the given arrows
 *
 * ::: tip "Render Model Layer"
 *     High-level.
 */
static void
dia_renderer_real_draw_polyline_with_arrows (DiaRenderer *renderer,
                                             Point       *points,
                                             int          n_points,
                                             double       line_width,
                                             DiaColour   *line_colour,
                                             Arrow       *start_arrow,
                                             Arrow       *end_arrow)
{
  /* Index of first and last point with a non-zero length segment */
  int firstline = 0;
  int lastline = n_points;
  Point oldstart = points[firstline];
  Point oldend = points[lastline - 1];
  Point start_arrow_head;
  Point end_arrow_head;

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;

    while (firstline < n_points - 1 &&
           distance_point_point (&points[firstline],
                                 &points[firstline + 1]) < 0.0000001) {
      firstline++;
    }

    if (firstline == n_points - 1) {
      firstline = 0; /* No non-zero lines, it doesn't matter. */
    }

    oldstart = points[firstline];
    calculate_arrow_point (start_arrow,
                           &points[firstline],
                           &points[firstline + 1],
                           &move_arrow,
                           &move_line,
                           line_width);
    start_arrow_head = points[firstline];
    point_sub (&start_arrow_head, &move_arrow);
    point_sub (&points[firstline], &move_line);
  }

  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;

    while (lastline > 0 &&
           distance_point_point (&points[lastline - 1],
                                 &points[lastline - 2]) < 0.0000001) {
      lastline--;
    }

    if (lastline == 0) {
      firstline = n_points; /* No non-zero lines, it doesn't matter. */
    }

    oldend = points[lastline - 1];
    calculate_arrow_point (end_arrow,
                           &points[lastline - 1],
                           &points[lastline - 2],
                           &move_arrow,
                           &move_line,
                           line_width);
    end_arrow_head = points[lastline - 1];
    point_sub (&end_arrow_head, &move_arrow);
    point_sub (&points[lastline - 1], &move_line);
  }

  /* Don't draw degenerate line segments at end of line */
  if (lastline-firstline > 1) {
    /* probably hiding a bug above, but don't try to draw a negative
     * number of points at all, fixes bug #148139 */
    dia_renderer_draw_polyline (renderer,
                                &points[firstline],
                                lastline - firstline,
                                line_colour);
  }

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    dia_arrow_draw (start_arrow,
                    renderer,
                    &start_arrow_head,
                    &points[firstline + 1],
                    line_width,
                    line_colour,
                    &DIA_COLOUR_WHITE);
  }

  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    dia_arrow_draw (end_arrow,
                    renderer,
                    &end_arrow_head,
                    &points[lastline - 2],
                    line_width,
                    line_colour,
                    &DIA_COLOUR_WHITE);
  }

  points[firstline] = oldstart;
  points[lastline - 1] = oldend;
}


/**
 * DiaRendererClass::draw_rounded_polyline_with_arrows:
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points):
 * @n_points: the number of @points
 * @line_width:
 * @line_colour: the [type@Dia.Colour] to stroke
 * @start_arrow: (nullable): the start [type@Dia.Arrow]
 * @end_arrow: (nullable): the end [type@Dia.Arrow]
 * @radius:
 *
 * Draw a rounded polyline fitting to the given arrows.
 *
 * ::: tip "Render Model Layer"
 *     High-level.
 */
static void
dia_renderer_real_draw_rounded_polyline_with_arrows (DiaRenderer *renderer,
                                                     Point       *points,
                                                     int          n_points,
                                                     double       line_width,
                                                     DiaColour   *line_colour,
                                                     Arrow       *start_arrow,
                                                     Arrow       *end_arrow,
                                                     double       radius)
{
  /* Index of first and last point with a non-zero length segment */
  int firstline = 0;
  int lastline = n_points;
  Point oldstart = points[firstline];
  Point oldend = points[lastline - 1];
  Point start_arrow_head;
  Point end_arrow_head;

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;

    while (firstline < n_points - 1 &&
           distance_point_point (&points[firstline],
                                 &points[firstline + 1]) < 0.0000001) {
      firstline++;
    }

    if (firstline == n_points - 1) {
      firstline = 0; /* No non-zero lines, it doesn't matter. */
    }

    oldstart = points[firstline];
    calculate_arrow_point (start_arrow,
                           &points[firstline],
                           &points[firstline + 1],
                           &move_arrow,
                           &move_line,
                           line_width);
    start_arrow_head = points[firstline];
    point_sub (&start_arrow_head, &move_arrow);
    point_sub (&points[firstline], &move_line);
  }

  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;

    while (lastline > 0 &&
           distance_point_point (&points[lastline - 1],
                                 &points[lastline - 2]) < 0.0000001) {
      lastline--;
    }

    if (lastline == 0) {
      firstline = n_points; /* No non-zero lines, it doesn't matter. */
    }

    oldend = points[lastline - 1];
    calculate_arrow_point (end_arrow,
                           &points[lastline - 1],
                           &points[lastline - 2],
                           &move_arrow,
                           &move_line,
                           line_width);
    end_arrow_head = points[lastline - 1];
    point_sub (&end_arrow_head, &move_arrow);
    point_sub (&points[lastline - 1], &move_line);
  }

  /* Don't draw degenerate line segments at end of line */
  if (lastline-firstline > 1) { /* only if there is something */
    dia_renderer_draw_rounded_polyline (renderer,
                                        &points[firstline],
                                        lastline - firstline,
                                        line_colour,
                                        radius);
  }

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    dia_arrow_draw (start_arrow,
                    renderer,
                    &start_arrow_head,
                    &points[firstline + 1],
                    line_width,
                    line_colour,
                    &DIA_COLOUR_WHITE);
  }

  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    dia_arrow_draw (end_arrow,
                    renderer,
                    &end_arrow_head,
                    &points[lastline - 2],
                    line_width,
                    line_colour,
                    &DIA_COLOUR_WHITE);
  }

  points[firstline] = oldstart;
  points[lastline - 1] = oldend;
}


/*
 * points_to_line:
 *
 * Figure the equation for a line given by two points.
 *
 * Returns: %FALSE if the line is vertical (infinite a).
 */
static gboolean
points_to_line (double *a, double *b, Point *p1, Point *p2)
{
  if (fabs (p1->x - p2->x) < 0.000000001) {
    return FALSE;
  }

  *a = (p2->y - p1->y) / (p2->x - p1->x);
  *b = p1->y - (*a) * p1->x;

  return TRUE;
}


/*
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
  double a1, b1, a2, b2;

  /* Find coefficients of lines */
  if (!(points_to_line (&a1, &b1, p1a, p1b))) {
    if (!(points_to_line (&a2, &b2, p2a, p2b))) {
      if (fabs (p1a->x - p2a->x) < 0.00000001) {
        *cross = *p1a;
        return TRUE;
      } else {
        return FALSE;
      }
    }
    cross->x = p1a->x;
    cross->y = a2 * (p1a->x) + b2;
    return TRUE;
  }

  if (!(points_to_line (&a2, &b2, p2a, p2b))) {
    cross->x = p2a->x;
    cross->y = a1*(p2a->x)+b1;
    return TRUE;
  }

  /* Solve */
  if (fabs (a1 - a2) < 0.000000001) {
    if (fabs (b1 - b2) < 0.000000001) {
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
    cross->x = (b2 - b1) / (a1 - a2);
    cross->y = a1 * cross->x + b1;
    return TRUE;
  }
}


/*
 * find_centre_point:
 *
 * Given three points, find the centre of the circle they describe.
 *
 * The renderer should disappear once the debugging is done.
 *
 * Returns: %FALSE if the centre could not be determined (i.e. the points
 * all lie really close together).
 */
static gboolean
find_centre_point (Point       *centre,
                   const Point *p1,
                   const Point *p2,
                   const Point *p3)
{
  Point mid1;
  Point mid2;
  Point orth1;
  Point orth2;
  double tmp;

  /* Find vector from middle between two points towards centre */
  mid1 = *p1;
  point_sub (&mid1, p2);
  point_scale (&mid1, 0.5);
  orth1 = mid1;
  point_add (&mid1, p2); /* Now midpoint between p1 & p2 */
  tmp = orth1.x;
  orth1.x = orth1.y;
  orth1.y = -tmp;
  point_add (&orth1, &mid1);


  /* Again, with two other points */
  mid2 = *p2;
  point_sub (&mid2, p3);
  point_scale (&mid2, 0.5);
  orth2 = mid2;
  point_add (&mid2, p3); /* Now midpoint between p2 & p3 */
  tmp = orth2.x;
  orth2.x = orth2.y;
  orth2.y = -tmp;
  point_add (&orth2, &mid2);

  /* The intersection between these two is the centre */
  if (!intersection_line_line(centre, &mid1, &orth1, &mid2, &orth2)) {
    /* Degenerate circle */
    /* Case 1: Points are all on top of each other.  Nothing to do. */
    if (fabs ((p1->x + p2->x + p3->x) / 3 - p1->x) < 0.0000001 &&
        fabs ((p1->y + p2->y + p3->y) / 3 - p1->y) < 0.0000001) {
      return FALSE;
    }

    /* Case 2: Two points are on top of each other.  Midpoint of
     * non-degenerate line is centre. */
    if (distance_point_point_manhattan(p1, p2) < 0.0000001) {
      *centre = mid2;
      return TRUE;
    } else if (distance_point_point_manhattan (p1, p3) < 0.0000001 ||
               distance_point_point_manhattan (p2, p3) < 0.0000001) {
      *centre = mid1;
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
  point_sub (&dot1, c);
  point_normalize (&dot1);
  dot2 = *b;
  point_sub (&dot2, c);
  point_normalize (&dot2);
  return point_cross (&dot1, &dot2) > 0;
}


/**
 * DiaRendererClass::draw_arc_with_arrows:
 * @self: the [type@Dia.Renderer]
 * @start_point: the start [type@Dia.Point] of the arc
 * @end_point: the end [type@Dia.Point] of the arc
 * @mid_point: the mid [type@Dia.Point] of the arc
 * @line_width:
 * @line_colour: the [type@Dia.Colour] to stroke
 * @start_arrow: (nullable): the start [type@Dia.Arrow]
 * @end_arrow: (nullable): the end [type@Dia.Arrow]
 *
 * Draw an arc fitting to the given arrows.
 *
 * ::: tip "Render Model Layer"
 *     High-level.
 */
static void
dia_renderer_real_draw_arc_with_arrows (DiaRenderer *renderer,
                                        Point       *start_point,
                                        Point       *end_point,
                                        Point       *mid_point,
                                        double       line_width,
                                        DiaColour   *line_colour,
                                        Arrow       *start_arrow,
                                        Arrow       *end_arrow)
{
  Point new_startpoint = *start_point;
  Point new_endpoint = *end_point;
  Point centre;
  double width, angle1, angle2;
  gboolean righthand;
  Point start_arrow_head;
  Point start_arrow_end;
  Point end_arrow_head;
  Point end_arrow_end;

  if (!find_centre_point (&centre, start_point, end_point, mid_point)) {
    /* Degenerate circle -- should have been caught by the drawer? */
    g_warning ("Degenerated arc in draw_arc_with_arrows()");
    centre = *start_point; /* continue to draw something bogus ... */
  }

  righthand = is_right_hand (start_point, mid_point, end_point);
  /* calculate original direction */
  angle1 = -atan2 (new_startpoint.y - centre.y,
                   new_startpoint.x - centre.x) * 180.0 / G_PI;
  while (angle1 < 0.0) {
    angle1 += 360.0;
  }
  angle2 = -atan2 (new_endpoint.y - centre.y,
                   new_endpoint.x - centre.x) * 180.0 / G_PI;
  while (angle2 < 0.0) {
    angle2 += 360.0;
  }

  width = 2 * distance_point_point (&centre, start_point);

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    double tmp;

    start_arrow_end = *start_point;
    point_sub (&start_arrow_end, &centre);
    tmp = start_arrow_end.x;
    if (righthand) {
      start_arrow_end.x = -start_arrow_end.y;
      start_arrow_end.y = tmp;
    } else {
      start_arrow_end.x = start_arrow_end.y;
      start_arrow_end.y = -tmp;
    }
    point_add (&start_arrow_end, start_point);

    calculate_arrow_point (start_arrow,
                           start_point,
                           &start_arrow_end,
                           &move_arrow,
                           &move_line,
                           line_width);
    start_arrow_head = *start_point;
    point_sub (&start_arrow_head, &move_arrow);
    point_sub (&new_startpoint, &move_line);
  }

  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    double tmp;

    end_arrow_end = *end_point;
    point_sub (&end_arrow_end, &centre);
    tmp = end_arrow_end.x;
    if (righthand) {
      end_arrow_end.x = end_arrow_end.y;
      end_arrow_end.y = -tmp;
    } else {
      end_arrow_end.x = -end_arrow_end.y;
      end_arrow_end.y = tmp;
    }
    point_add (&end_arrow_end, end_point);

    calculate_arrow_point (end_arrow,
                           end_point,
                           &end_arrow_end,
                           &move_arrow,
                           &move_line,
                           line_width);
    end_arrow_head = *end_point;
    point_sub (&end_arrow_head, &move_arrow);
    point_sub (&new_endpoint, &move_line);
  }

  /* Now we possibly have new start- and endpoint. We must not
   * recalculate the centre cause the new points lie on the tangential
   * approximation of the original arc arrow lines not on the arc itself.
   * The one thing we need to deal with is calculating the (new) angles
   * and get rid of the arc drawing altogether if got degenerated.
   */
  angle1 = -atan2 (new_startpoint.y - centre.y,
                   new_startpoint.x - centre.x) * 180.0 / G_PI;
  while (angle1 < 0.0) {
    angle1 += 360.0;
  }
  angle2 = -atan2 (new_endpoint.y - centre.y,
                   new_endpoint.x - centre.x) * 180.0 / G_PI;
  while (angle2 < 0.0) {
    angle2 += 360.0;
  }

  /* Only draw it if the original direction is preserved */
  if (is_right_hand (&new_startpoint,
                     mid_point,
                     &new_endpoint) == righthand) {
    /* make it direction aware */
    if (!righthand && angle2 < angle1) {
      angle1 -= 360.0;
    } else if (righthand && angle2 > angle1) {
      angle2 -= 360.0;
    }

    dia_renderer_draw_arc (renderer,
                           &centre,
                           width,
                           width,
                           angle1,
                           angle2,
                           line_colour);
  }

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    dia_arrow_draw (start_arrow,
                    renderer,
                    &start_arrow_head,
                    &start_arrow_end,
                    line_width,
                    line_colour,
                    &DIA_COLOUR_WHITE);
  }

  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    dia_arrow_draw (end_arrow,
                    renderer,
                    &end_arrow_head,
                    &end_arrow_end,
                    line_width,
                    line_colour,
                    &DIA_COLOUR_WHITE);
  }
}


/**
 * DiaRendererClass::draw_bezier_with_arrows:
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points):
 * @n_points: the number of @points
 * @line_width:
 * @line_colour: the [type@Dia.Colour] to stroke
 * @start_arrow: (nullable): the start [type@Dia.Arrow]
 * @end_arrow: (nullable): the end [type@Dia.Arrow]
 *
 * Draw a bezier line fitting to the given arrows.
 *
 * ::: tip "Render Model Layer"
 *     High-level.
 */
static void
dia_renderer_real_draw_bezier_with_arrows (DiaRenderer *renderer,
                                           BezPoint    *points,
                                           int          n_points,
                                           double       line_width,
                                           DiaColour   *line_colour,
                                           Arrow       *start_arrow,
                                           Arrow       *end_arrow)
{
  Point startpoint, endpoint;
  Point start_arrow_head;
  Point end_arrow_head;

  startpoint = points[0].p1;
  endpoint = points[n_points - 1].p3;

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    Point move_arrow;
    Point move_line;
    calculate_arrow_point (start_arrow,
                           &points[0].p1,
                           &points[1].p1,
                           &move_arrow,
                           &move_line,
                           line_width);
    start_arrow_head = points[0].p1;
    point_sub (&start_arrow_head, &move_arrow);
    point_sub (&points[0].p1, &move_line);
  }

  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    Point move_arrow;
    Point move_line;
    calculate_arrow_point (end_arrow,
                           &points[n_points - 1].p3,
                           &points[n_points - 1].p2,
                           &move_arrow,
                           &move_line,
                           line_width);
    end_arrow_head = points[n_points - 1].p3;
    point_sub (&end_arrow_head, &move_arrow);
    point_sub (&points[n_points - 1].p3, &move_line);
  }

  dia_renderer_draw_bezier (renderer, points, n_points, line_colour);

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    dia_arrow_draw (start_arrow,
                    renderer,
                    &start_arrow_head,
                    &points[1].p1,
                    line_width,
                    line_colour,
                    &DIA_COLOUR_WHITE);
  }

  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    dia_arrow_draw (end_arrow,
                    renderer,
                    &end_arrow_head,
                    &points[n_points - 1].p2,
                    line_width,
                    line_colour,
                    &DIA_COLOUR_WHITE);
  }

  points[0].p1 = startpoint;
  points[n_points - 1].p3 = endpoint;
}


/**
 * DiaRendererClass::get_text_width:
 * @self: the [type@Dia.Renderer]
 * @text:
 * @length:
 *
 * Calculate text width of given string with previously set font.
 *
 * Should we really provide this? It formerly was an 'interactive op'.
 *
 * As of this writing it is only used for cursor positioning in with
 * an interactive renderer.
 */
static double
dia_renderer_real_get_text_width (DiaRenderer *renderer,
                                  const char  *text,
                                  int          length)
{
  double ret = 0;
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


/**
 * DiaRendererClass::is_capable_of:
 * @self: the [type@Dia.Renderer]
 * @capabilities: set of [type@Dia.RenderCapability]
 *
 * Advertise special renderer capabilities
 *
 * The base class advertises none of the advanced capabilities, but it has
 * basic transformation support in [method@Dia.Renderer.draw_object] with the
 * help of [type@Dia.TransformRenderer]. Only an advanced renderer
 * implementation will overwrite this method with it's own capabilities.
 *
 * Returning %TRUE from this method usually requires to adapt at least one
 * other member function, too.
 *
 * Current special capabilities are
 *  - [flags@Dia.RenderCapability.HOLES]: [vfunc@Dia.Renderer.draw_beziergon]
 *    has to support multiple [enum@Dia.BezPointType.MOVE_TO].
 *  - [flags@Dia.RenderCapability.ALPHA]: the alpha component of
 *    [type@Dia.Colour] is handled to create transparency.
 *  - [flags@Dia.RenderCapability.AFFINE]: at least
 *    [vfunc@Dia.Renderer.draw_object] to be overwritten to support affine
 *    transformations. At some point in time also
 *    [vfunc@Dia.Renderer.draw_text] and [vfunc@Dia.Renderer.draw_image] need
 *    to handle at least rotation.
 *  - [flags@Dia.RenderCapability.PATTERN]: [vfunc@Dia.Renderer.set_pattern]
 *    overwrite and filling with pattern instead of fill colour.
 *
 * Remember: [type@Dia.RenderCapability] is a flag type, @capabilities may
 * reference multiple capabilities and %TRUE should only be returned if _all_
 * specified are supported.
 */
static gboolean
dia_renderer_real_is_capable_of (DiaRenderer         *renderer,
                                 DiaRenderCapability  capabilities)
{
  return FALSE;
}


/**
 * DiaRendererClass::set_pattern:
 * @self: the [type@Dia.Renderer]
 * @pattern: the [type@Dia.Pattern]
 *
 * Set the (gradient) pattern for later fill.
 *
 * The base class has no pattern (gradient) support, implementations should
 * report [flags@Dia.RenderCapability.PATTERN] if they support this.
 */
static void
dia_renderer_real_set_pattern (DiaRenderer *renderer, DiaPattern *pattern)
{
  g_warning ("%s::set_pattern not implemented!",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (renderer)));
}


static void
dia_renderer_class_init (DiaRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = dia_renderer_dispose;
  object_class->get_property= dia_renderer_get_property;
  object_class->set_property= dia_renderer_set_property;

  klass->draw_layer = dia_renderer_real_draw_layer;
  klass->draw_object = dia_renderer_real_draw_object;
  klass->get_text_width = dia_renderer_real_get_text_width;

  klass->begin_render = dia_renderer_real_begin_render;
  klass->end_render = dia_renderer_real_end_render;

  klass->set_linewidth = dia_renderer_real_set_linewidth;
  klass->set_linecaps = dia_renderer_real_set_linecaps;
  klass->set_linejoin = dia_renderer_real_set_linejoin;
  klass->set_linestyle = dia_renderer_real_set_linestyle;
  klass->set_fillstyle = dia_renderer_real_set_fillstyle;
  klass->font_changed = dia_renderer_real_font_changed;

  klass->draw_line = dia_renderer_real_draw_line;
  klass->draw_rect = dia_renderer_real_draw_rect;
  klass->draw_polygon = dia_renderer_real_draw_polygon;
  klass->draw_arc = dia_renderer_real_draw_arc;
  klass->fill_arc = dia_renderer_real_fill_arc;
  klass->draw_ellipse = dia_renderer_real_draw_ellipse;
  klass->draw_string = dia_renderer_real_draw_string;
  klass->draw_image = dia_renderer_real_draw_image;

  /* medium level functions */
  klass->draw_bezier = dia_renderer_real_draw_bezier;
  klass->draw_beziergon = dia_renderer_real_draw_beziergon;
  klass->draw_rounded_polyline = dia_renderer_real_draw_rounded_polyline;
  klass->draw_polyline = dia_renderer_real_draw_polyline;
  klass->draw_text = dia_renderer_real_draw_text;
  klass->draw_text_line = dia_renderer_real_draw_text_line;
  klass->draw_rotated_text = dia_renderer_real_draw_rotated_text;
  klass->draw_rotated_image = dia_renderer_real_draw_rotated_image;

  /* highest level functions */
  klass->draw_rounded_rect = dia_renderer_real_draw_rounded_rect;
  klass->draw_line_with_arrows = dia_renderer_real_draw_line_with_arrows;
  klass->draw_arc_with_arrows = dia_renderer_real_draw_arc_with_arrows;
  klass->draw_polyline_with_arrows = dia_renderer_real_draw_polyline_with_arrows;
  klass->draw_rounded_polyline_with_arrows = dia_renderer_real_draw_rounded_polyline_with_arrows;
  klass->draw_bezier_with_arrows = dia_renderer_real_draw_bezier_with_arrows;

  /* other */
  klass->is_capable_of = dia_renderer_real_is_capable_of;
  klass->set_pattern = dia_renderer_real_set_pattern;


  /**
   * DiaRenderer:font:
   *
   * Since: 0.98
   */
  pspecs[PROP_FONT] =
    g_param_spec_object ("font", NULL, NULL,
                         DIA_TYPE_FONT,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * DiaRenderer:font-height:
   *
   * Since: 0.98
   */
  pspecs[PROP_FONT_HEIGHT] =
    g_param_spec_double ("font-height", NULL, NULL,
                         0.0,
                         G_MAXDOUBLE,
                         1.0,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);


  /**
   * DiaRenderer::font-changed:
   * @self: the [type@Dia.Renderer]
   * @font: the now-active [type@Dia.Font]
   * @font_height: the now-active font height
   *
   * Emitted when a [method@Dia.Renderer.set_font] call caused the font
   * information to change.
   *
   * Since: 0.98
   */
  signals[FONT_CHANGED] = g_signal_new ("font-changed",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        G_STRUCT_OFFSET (DiaRendererClass,
                                                         font_changed),
                                        NULL, NULL,
                                        dia_marshal_VOID__OBJECT_DOUBLE,
                                        G_TYPE_NONE,
                                        2,
                                        DIA_TYPE_FONT,
                                        G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[FONT_CHANGED],
                              G_TYPE_FROM_CLASS (klass),
                              dia_marshal_VOID__OBJECT_DOUBLEv);
}


static void
dia_renderer_init (DiaRenderer *self)
{
  DiaRendererPrivate *priv = dia_renderer_get_instance_private (self);

  priv->font_height = 1.0;
}


/**
 * dia_renderer_draw_layer: (virtual draw_layer)
 * @self: the [type@Dia.Renderer]
 * @layer: the [type@Dia.Layer]
 * @active: is this the active layer
 * @update: (nullable): area to draw
 *
 * Render all the visible objects in the layer.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_layer (DiaRenderer  *self,
                         DiaLayer     *layer,
                         gboolean      active,
                         DiaRectangle *update)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_layer (self, layer, active, update);
}


/**
 * dia_renderer_draw_object: (virtual draw_object)
 * @self: the [type@Dia.Renderer]
 * @object: the [type@Dia.Object] to draw
 * @matrix: (nullable):
 *
 * Calls the objects draw function, which then calls back into the renderer.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_object (DiaRenderer *self,
                          DiaObject   *object,
                          DiaMatrix   *matrix)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_object (self, object, matrix);
}


/**
 * dia_renderer_get_text_width: (virtual get_text_width)
 * @self: the [type@Dia.Renderer]
 * @text:
 * @length:
 *
 * Returns the EXACT width of text in cm, using the current font. There has
 * been some confusion as to the definition of this.
 *
 * It used to say the width was in pixels, but actual width returned was cm.
 *
 * You shouldn't know about pixels anyway.
 *
 * Since: 0.98
 */
double
dia_renderer_get_text_width (DiaRenderer *self,
                             const char  *text,
                             int          length)
{
  g_return_val_if_fail (DIA_IS_RENDERER (self), 0);

  return DIA_RENDERER_GET_CLASS (self)->get_text_width (self, text, length);
}


/**
 * dia_renderer_begin_render: (virtual begin_render)
 * @self: the [type@Dia.Renderer]
 * @update:
 *
 * Called before rendering begins.
 *
 * Since: 0.98
 */
void
dia_renderer_begin_render (DiaRenderer        *self,
                           const DiaRectangle *update)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->begin_render (self, update);
}


/**
 * dia_renderer_end_render: (virtual end_render)
 * @self: the [type@Dia.Renderer]
 *
 * Called after all rendering is done.
 *
 * Since: 0.98
 */
void
dia_renderer_end_render (DiaRenderer *self)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->end_render (self);
}


/**
 * dia_renderer_set_linewidth: (virtual set_linewidth)
 * @self: the [type@Dia.Renderer]
 * @line_width:
 *
 * Set the current line width.
 *
 * If `line_width == 0`, the line will be a 'hairline'.
 *
 * Since: 0.98
 */
void
dia_renderer_set_linewidth (DiaRenderer *self,
                            double       line_width)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->set_linewidth (self, line_width);
}


/**
 * dia_renderer_set_linecaps: (virtual set_linecaps)
 * @self: the [type@Dia.Renderer]
 * @line_caps: the [type@Dia.LineCaps] style
 *
 * Set the current linecap (the way lines are ended).
 *
 * Since: 0.98
 */
void
dia_renderer_set_linecaps (DiaRenderer *self,
                           DiaLineCaps  line_caps)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->set_linecaps (self, line_caps);
}


/**
 * dia_renderer_set_linejoin: (virtual set_linejoin)
 * @self: the [type@Dia.Renderer]
 * @line_join: the [type@Dia.LineJoin] style
 *
 * Set the current linejoin (the way two lines are joined together).
 *
 * Since: 0.98
 */
void
dia_renderer_set_linejoin (DiaRenderer *self,
                           DiaLineJoin  line_join)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->set_linejoin (self, line_join);
}


/**
 * dia_renderer_set_linestyle: (virtual set_linestyle)
 * @self: the [type@Dia.Renderer]
 * @line_syle: the [type@Dia.LineStyle]
 * @dash_length:
 *
 * Set the current line style and the dash length, when the style is not
 * [enum@Dia.LineStyle.SOLID]. A dot will be 10% of length.
 *
 * Since: 0.98
 */
void
dia_renderer_set_linestyle (DiaRenderer  *self,
                            DiaLineStyle  line_syle,
                            double        dash_length)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->set_linestyle (self, line_syle, dash_length);
}


/**
 * dia_renderer_set_fillstyle: (virtual set_fillstyle)
 * @self: the [type@Dia.Renderer]
 * @fill_style: the [type@Dia.FillStyle]
 *
 * Set the fill style.
 *
 * Since: 0.98
 */
void
dia_renderer_set_fillstyle (DiaRenderer  *self,
                            DiaFillStyle  fill_style)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->set_fillstyle (self, fill_style);
}


/**
 * dia_renderer_set_font:
 * @self: the [type@Dia.Renderer]
 * @font: a new [type@Dia.Font]
 * @font_height:
 *
 * Update the active font used for operations like
 * [method@Dia.Renderer.draw_string].
 *
 * Since: 0.98
 */
void
dia_renderer_set_font (DiaRenderer *self,
                       DiaFont     *font,
                       double       font_height)
{
  DiaRendererPrivate *priv;
  gboolean font_changed;
  gboolean height_changed;

  g_return_if_fail (DIA_IS_RENDERER (self));

  priv = dia_renderer_get_instance_private (self);

  font_changed = g_set_object (&priv->font, font);
  height_changed = G_APPROX_VALUE (priv->font_height,
                                   font_height,
                                   FLT_EPSILON);
  priv->font_height = font_height;

  if (font_changed || height_changed) {
    g_signal_emit (self, signals[FONT_CHANGED], 0, font, font_height);
  }

  if (font_changed) {
    g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_FONT]);
  }

  if (height_changed) {
    g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_FONT]);
  }
}


/**
 * dia_renderer_get_font:
 * @self: the [type@Dia.Renderer]
 * @height: (optional) (out): the current font height
 *
 * Returns: (transfer none): the active [type@Dia.Font].
 *
 * Since: 0.98
 */
DiaFont *
dia_renderer_get_font (DiaRenderer *self,
                       double      *height)
{
  DiaRendererPrivate *priv;

  g_return_val_if_fail (DIA_IS_RENDERER (self), NULL);

  priv = dia_renderer_get_instance_private (self);

  if (height) {
    *height = priv->font_height;
  }

  return priv->font;
}


/**
 * dia_renderer_draw_line: (virtual draw_line)
 * @self: the [type@Dia.Renderer]
 * @start_point:
 * @end_point:
 * @line_colour: the [type@Dia.Colour] to stroke
 *
 * Draw a line from @start_point to @end_point, using @line_colour and the
 * current line style.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_line (DiaRenderer *self,
                        Point       *start_point,
                        Point       *end_point,
                        DiaColour   *line_colour)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_line (self,
                                                start_point,
                                                end_point,
                                                line_colour);
}


/**
 * dia_renderer_draw_polygon: (virtual draw_polygon)
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points):
 * @n_points: number of @points
 * @fill: the [type@Dia.Colour] to fill
 * @stroke: the [type@Dia.Colour] to stroke
 *
 * Draw a polygon filled using the current fill type and stroked with the
 * current line style.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_polygon (DiaRenderer *self,
                           Point       *points,
                           int          n_points,
                           DiaColour   *fill,
                           DiaColour   *stroke)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_polygon (self,
                                               points,
                                               n_points,
                                               fill,
                                               stroke);
}


/**
 * dia_renderer_draw_arc: (virtual draw_arc)
 * @self: the [type@Dia.Renderer]
 * @centre:
 * @width:
 * @height:
 * @angle1:
 * @angle2:
 * @line_colour: the [type@Dia.Colour] to stroke
 *
 * Draw an arc, given its centre, the bounding box (widget, height) the start
 * angle and the end angle.
 *
 * It's counter-clockwise if `angle2 > angle1`.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_arc (DiaRenderer *self,
                       Point       *centre,
                       double       width,
                       double       height,
                       double       angle1,
                       double       angle2,
                       DiaColour   *line_colour)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_arc (self,
                                           centre,
                                           width,
                                           height,
                                           angle1,
                                           angle2,
                                           line_colour);
}


/**
 * dia_renderer_fill_arc: (virtual fill_arc)
 * @self: the [type@Dia.Renderer]
 * @centre:
 * @width:
 * @height:
 * @angle1:
 * @angle2:
 * @fill_colour: the [type@Dia.Colour] to fill
 *
 * Draw an arc, same as [method@Dia.Renderer.draw_arc], but with the area
 * under the arc filled.
 *
 * Since: 0.98
 */
void
dia_renderer_fill_arc (DiaRenderer *self,
                       Point       *centre,
                       double       width,
                       double       height,
                       double       angle1,
                       double       angle2,
                       DiaColour   *fill_colour)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->fill_arc (self,
                                           centre,
                                           width,
                                           height,
                                           angle1,
                                           angle2,
                                           fill_colour);
}


/**
 * dia_renderer_draw_ellipse: (virtual draw_ellipse)
 * @self: the [type@Dia.Renderer]
 * @centre:
 * @width:
 * @height:
 * @fill: the [type@Dia.Colour] to fill
 * @stroke: the [type@Dia.Colour] to stroke
 *
 * Draw an ellipse, given its centre and the bounding box.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_ellipse (DiaRenderer *self,
                           Point       *centre,
                           double       width,
                           double       height,
                           DiaColour   *fill,
                           DiaColour   *stroke)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_ellipse (self,
                                               centre,
                                               width,
                                               height,
                                               fill,
                                               stroke);
}


/**
 * dia_renderer_draw_string: (virtual draw_string)
 * @self: the [type@Dia.Renderer]
 * @text: a string to draw
 * @pos:
 * @alignment:
 * @text_colour: the [type@Dia.Colour] to stroke
 *
 * Print a string at @pos, using the current font.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_string (DiaRenderer  *self,
                          const char   *text,
                          Point        *pos,
                          DiaAlignment  alignment,
                          DiaColour    *text_colour)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_string (self,
                                              text,
                                              pos,
                                              alignment,
                                              text_colour);
}


/**
 * dia_renderer_draw_image: (virtual draw_image)
 * @self: the [type@Dia.Renderer]
 * @point:
 * @width:
 * @height:
 * @image: the [type@Dia.Image] to draw
 *
 * Draw an image, given its bounding box.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_image (DiaRenderer *self,
                         Point       *point,
                         double       width,
                         double       height,
                         DiaImage    *image)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_image (self,
                                             point,
                                             width,
                                             height,
                                             image);
}


/**
 * dia_renderer_draw_bezier: (virtual draw_bezier)
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points):
 * @n_points: number of @points
 * @line_colour: the [type@Dia.Colour] to stroke
 *
 * Draw a bezier curve, given it's control points.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_bezier (DiaRenderer *self,
                          BezPoint    *points,
                          int          n_points,
                          DiaColour   *line_colour)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_bezier (self,
                                              points,
                                              n_points,
                                              line_colour);
}


/**
 * dia_renderer_draw_beziergon: (virtual draw_beziergon)
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points):
 * @n_points: number of @points
 * @fill: the [type@Dia.Colour] to fill
 * @stroke: the [type@Dia.Colour] to stroke
 *
 * Fill and/or stroke a closed bezier.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_beziergon (DiaRenderer *self,
                             BezPoint    *points,
                             int          n_points,
                             DiaColour   *fill,
                             DiaColour   *stroke)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_beziergon (self,
                                                 points,
                                                 n_points,
                                                 fill,
                                                 stroke);
}


/**
 * dia_renderer_draw_polyline: (virtual draw_polyline)
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points):
 * @n_points: the number of @points
 * @line_colour: the [type@Dia.Colour] to stroke
 *
 * Draw a line joining multiple points, using @line_colour and the current
 * line style.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_polyline (DiaRenderer *self,
                            Point       *points,
                            int          n_points,
                            DiaColour   *line_colour)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_polyline (self,
                                                points,
                                                n_points,
                                                line_colour);
}


/**
 * dia_renderer_draw_text: (virtual draw_text)
 * @self: the [type@Dia.Renderer]
 * @text: the [type@Dia.Text] to draw
 *
 * Draw @text according to it's own positioning, font, and height.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_text (DiaRenderer *self,
                        DiaText     *text)
{
  g_return_if_fail (DIA_IS_RENDERER (self));
  g_return_if_fail (DIA_IS_TEXT (text));

  DIA_RENDERER_GET_CLASS (self)->draw_text (self, text);
}


/**
 * dia_renderer_draw_text_line: (virtual draw_text_line)
 * @self: the [type@Dia.Renderer]
 * @text_line: the [type@Dia.TextLine] to draw
 * @pos: where to place @text_line
 * @alignment: the [type@Dia.Alignment] of the @text_line
 * @line_colour: the [type@Dia.Colour] for the @text_line
 *
 * Draw @text_line at @pos, using it's own font and height.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_text_line (DiaRenderer  *self,
                             TextLine     *text_line,
                             Point        *pos,
                             DiaAlignment  alignment,
                             DiaColour    *line_colour)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_text_line (self,
                                                 text_line,
                                                 pos,
                                                 alignment,
                                                 line_colour);
}


/**
 * dia_renderer_draw_rect: (virtual draw_rect)
 * @self: the [type@Dia.Renderer]
 * @ul_corner: the upper-left corner
 * @lr_corner: the lower-right corner
 * @fill: (nullable): the [type@Dia.Colour] to fill
 * @stroke: (nullable): the [type@Dia.Colour] to stroke
 *
 * Draw a rectangle, given its upper-left and lower-right corners.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_rect (DiaRenderer *self,
                        Point       *ul_corner,
                        Point       *lr_corner,
                        DiaColour   *fill,
                        DiaColour   *stroke)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_rect (self,
                                            ul_corner,
                                            lr_corner,
                                            fill,
                                            stroke);
}


/**
 * dia_renderer_draw_rounded_rect: (virtual draw_rounded_rect)
 * @self: the [type@Dia.Renderer]
 * @ul_corner: the upper-left corner
 * @lr_corner: the lower-right corner
 * @fill: (nullable): the [type@Dia.Colour] to fill
 * @stroke: (nullable): the [type@Dia.Colour] to stroke
 * @radius:
 *
 * Draw a rounded rectangle, given its upper-left and lower-right corners.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_rounded_rect (DiaRenderer *self,
                                Point       *ul_corner,
                                Point       *lr_corner,
                                DiaColour   *fill,
                                DiaColour   *stroke,
                                double       radius)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_rounded_rect (self,
                                                    ul_corner,
                                                    lr_corner,
                                                    fill,
                                                    stroke,
                                                    radius);
}


/**
 * dia_renderer_draw_rounded_polyline: (virtual draw_rounded_polyline)
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points):
 * @n_points: the number of @points
 * @line_colour: the [type@Dia.Colour] to stroke
 * @radius:
 *
 * Draw a line joining multiple points, using @line_colour and the current
 * line style with rounded corners between segments.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_rounded_polyline (DiaRenderer *self,
                                    Point       *points,
                                    int          n_points,
                                    DiaColour   *line_colour,
                                    double       radius)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_rounded_polyline (self,
                                                        points,
                                                        n_points,
                                                        line_colour,
                                                        radius);
}


/**
 * dia_renderer_draw_line_with_arrows: (virtual draw_line_with_arrows)
 * @self: the [type@Dia.Renderer]
 * @start_point:
 * @end_point:
 * @line_width:
 * @line_colour: the [type@Dia.Colour] to stroke
 * @start_arrow: (nullable): the start [type@Dia.Arrow]
 * @end_arrow: (nullable): the end [type@Dia.Arrow]
 *
 * Draw a line from @start_point to @end_end, in @line_colour and
 * @line_width wide, with optional arrows at either end.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_line_with_arrows (DiaRenderer *self,
                                    Point       *start_point,
                                    Point       *end_point,
                                    double       line_width,
                                    DiaColour   *line_colour,
                                    Arrow       *start_arrow,
                                    Arrow       *end_arrow)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_line_with_arrows (self,
                                                        start_point,
                                                        end_point,
                                                        line_width,
                                                        line_colour,
                                                        start_arrow,
                                                        end_arrow);
}


/**
 * dia_renderer_draw_arc_with_arrows: (virtual draw_arc_with_arrows)
 * @self: the [type@Dia.Renderer]
 * @start_point:
 * @end_point:
 * @mid_point:
 * @line_width:
 * @line_colour: the [type@Dia.Colour] to stroke
 * @start_arrow: (nullable): the start [type@Dia.Arrow]
 * @end_arrow: (nullable): the end [type@Dia.Arrow]
 *
 * Draw an arc from @start_point to @end_point, via @mid_point, in
 * @line_colour and @line_width wide, with optional arrows at either end.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_arc_with_arrows (DiaRenderer *self,
                                   Point       *start_point,
                                   Point       *end_point,
                                   Point       *mid_point,
                                   double       line_width,
                                   DiaColour   *line_colour,
                                   Arrow       *start_arrow,
                                   Arrow       *end_arrow)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_arc_with_arrows (self,
                                                       start_point,
                                                       end_point,
                                                       mid_point,
                                                       line_width,
                                                       line_colour,
                                                       start_arrow,
                                                       end_arrow);
}


/**
 * dia_renderer_draw_polyline_with_arrows: (virtual draw_polyline_with_arrows)
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points):
 * @n_points: the number of @points
 * @line_width:
 * @line_colour: the [type@Dia.Colour] to stroke
 * @start_arrow: (nullable): the start [type@Dia.Arrow]
 * @end_arrow: (nullable): the end [type@Dia.Arrow]
 *
 * Draw a multi-part line between @points, in @line_colour and @line_width
 * wide, with optional arrows at either end.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_polyline_with_arrows (DiaRenderer *self,
                                        Point       *points,
                                        int          n_points,
                                        double       line_width,
                                        DiaColour   *line_colour,
                                        Arrow       *start_arrow,
                                        Arrow       *end_arrow)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_polyline_with_arrows (self,
                                                            points,
                                                            n_points,
                                                            line_width,
                                                            line_colour,
                                                            start_arrow,
                                                            end_arrow);
}


/**
 * dia_renderer_draw_rounded_polyline_with_arrows: (virtual draw_rounded_polyline_with_arrows)
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points):
 * @n_points: the number of @points
 * @line_width:
 * @line_colour: the [type@Dia.Colour] to stroke
 * @start_arrow: (nullable): the start [type@Dia.Arrow]
 * @end_arrow: (nullable): the end [type@Dia.Arrow]
 * @radius:
 *
 * Since: 0.98
 */
void
dia_renderer_draw_rounded_polyline_with_arrows (DiaRenderer *self,
                                                Point       *points,
                                                int          n_points,
                                                double       line_width,
                                                DiaColour   *line_colour,
                                                Arrow       *start_arrow,
                                                Arrow       *end_arrow,
                                                double       radius)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_rounded_polyline_with_arrows (self,
                                                                    points,
                                                                    n_points,
                                                                    line_width,
                                                                    line_colour,
                                                                    start_arrow,
                                                                    end_arrow,
                                                                    radius);
}


/**
 * dia_renderer_draw_bezier_with_arrows: (virtual draw_bezier_with_arrows)
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points):
 * @n_points: the number of @points
 * @line_width:
 * @line_colour: the [type@Dia.Colour] to stroke
 * @start_arrow: (nullable): the start [type@Dia.Arrow]
 * @end_arrow: (nullable): the end [type@Dia.Arrow]
 *
 * Since: 0.98
 */
void
dia_renderer_draw_bezier_with_arrows (DiaRenderer *self,
                                      BezPoint    *points,
                                      int          n_points,
                                      double       line_width,
                                      DiaColour   *line_colour,
                                      Arrow       *start_arrow,
                                      Arrow       *end_arrow)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_bezier_with_arrows (self,
                                                          points,
                                                          n_points,
                                                          line_width,
                                                          line_colour,
                                                          start_arrow,
                                                          end_arrow);
}


/**
 * dia_renderer_is_capable_of: (virtual is_capable_of)
 * @self: the [type@Dia.Renderer]
 * @capabilities:
 *
 * Check if @self supports the features @capabilities.
 *
 * Note [type@Dia.RenderCapability] is a flag type, so multiple capabilities
 * can be queried at once with %TRUE only being returned if all of them are
 * supported.
 *
 * Since: 0.98
 */
gboolean
dia_renderer_is_capable_of (DiaRenderer         *self,
                            DiaRenderCapability  capabilities)
{
  g_return_val_if_fail (DIA_IS_RENDERER (self), FALSE);

  return DIA_RENDERER_GET_CLASS (self)->is_capable_of (self, capabilities);
}


/**
 * dia_renderer_set_pattern: (virtual set_pattern)
 * @self: the [type@Dia.Renderer]
 * @pattern: the [type@Dia.Pattern]
 *
 * Set the current pattern.
 *
 * Since: 0.98
 */
void
dia_renderer_set_pattern (DiaRenderer *self,
                          DiaPattern  *pattern)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->set_pattern (self, pattern);
}


/**
 * dia_renderer_draw_rotated_text: (virtual draw_rotated_text)
 * @self: the [type@Dia.Renderer]
 * @text: the [type@Dia.Text] to draw
 * @centre:
 * @angle:
 *
 * Draw @text rotated around @centre at @angle degrees.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_rotated_text (DiaRenderer *self,
                                DiaText     *text,
                                Point       *centre,
                                double       angle)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_rotated_text (self, text, centre, angle);
}


/**
 * dia_renderer_draw_rotated_image: (virtual draw_rotated_image)
 * @self: the [type@Dia.Renderer]
 * @point:
 * @width:
 * @height:
 * @angle:
 * @image: the [type@Dia.Image] to draw
 *
 * Draw @image rotated around it's centre at @angle degrees.
 *
 * Since: 0.98
 */
void
dia_renderer_draw_rotated_image (DiaRenderer *self,
                                 Point       *point,
                                 double       width,
                                 double       height,
                                 double       angle,
                                 DiaImage    *image)
{
  g_return_if_fail (DIA_IS_RENDERER (self));

  DIA_RENDERER_GET_CLASS (self)->draw_rotated_image (self,
                                                     point,
                                                     width,
                                                     height,
                                                     angle,
                                                     image);
}


/**
 * dia_renderer_bezier_fill
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points): the [type@Dia.BezPoint]s of the bezier
 * @n_points: the number of @points
 * @fill_colour: the [type@Dia.Colour] to fill with
 *
 * Helper function to fill bezier with multiple [enum@Dia.BezPointType.MOVE_TO]
 * A slightly improved version to split a bezier with multiple move-to into
 * a form which can be used with [type@Dia.Renderer] not supporting
 * [flags@Dia.RenderCapability.HOLES].
 *
 * With reasonable placement of the second movement it works well for single
 * holes at least. There are artifacts for more complex path to render.
 *
 * Since: 0.98
 */
void
dia_renderer_bezier_fill (DiaRenderer *self,
                          BezPoint    *points,
                          int          n_points,
                          DiaColour   *fill_colour)
{
  gboolean needs_split = FALSE;

  for (int i = 1; i < n_points; ++i) {
    if (points[i].type == BEZ_MOVE_TO) {
      needs_split = TRUE;
      break;
    }
  }

  if (!needs_split) {
    dia_renderer_draw_beziergon (self, points, n_points, fill_colour, NULL);
  } else {
    GArray *points_array = g_array_new (FALSE, FALSE, sizeof (BezPoint));
    Point close_to;
    gboolean needs_close = FALSE;

    /* start with move-to */
    g_array_append_val (points_array, points[0]);

    for (int i = 1; i < n_points; ++i) {
      if (points[i].type == BEZ_MOVE_TO) {
        /* check whether the start point of the second outline is within the first outline. */
        double dist =
          distance_bez_shape_point (&g_array_index (points_array, BezPoint, 0),
                                    points_array->len,
                                    0,
                                    &points[i].p1);

        if (dist > 0) { /* outside, just create a new one? */
          /* flush what we have */
          if (needs_close) {
            BezPoint bp;
            bp.type = BEZ_LINE_TO;
            bp.p1 = close_to;
            g_array_append_val (points_array, bp);
          }

          dia_renderer_draw_beziergon (self,
                                       &g_array_index (points_array, BezPoint, 0),
                                       points_array->len,
                                       fill_colour,
                                       NULL);

          g_array_set_size (points_array, 0);
          g_array_append_val (points_array, points[i]); /* new needs move-to */

          needs_close = FALSE;
        } else {
          BezPoint bp = points[i];
          bp.type = BEZ_LINE_TO;

          /* just turn the move- to a line-to */
          g_array_append_val (points_array, bp);
          /* and remember the point we lined from */
          close_to = points[i - 1].type == BEZ_CURVE_TO ?
            points[i - 1].p3 : points[i - 1].p1;

          needs_close = TRUE;
        }
      } else {
        g_array_append_val (points_array, points[i]);
      }
    }

    if (points_array->len > 1) {
      /* actually most renderers need at least three points, but having only one
       * point is an artifact coming from the algorithm above: "new needs move-to" */
      dia_renderer_draw_beziergon (self,
                                   &g_array_index (points_array, BezPoint, 0),
                                   points_array->len,
                                   fill_colour,
                                   NULL);
    }

    g_array_free (points_array, TRUE);
  }
}


/**
 * dia_renderer_bezier_stroke:
 * @self: the [type@Dia.Renderer]
 * @points: (array length=n_points): the [type@Dia.BezPoint]s to draw
 * @n_points: number of @points
 * @line_colour: the [type@Dia.Colour] of the stroke
 *
 * Helper function to stroke a bezier with multiple
 * [enum@Dia.BezPointType.MOVE_TO].
 *
 * Since: 0.98
 */
void
dia_renderer_bezier_stroke (DiaRenderer *self,
                            BezPoint    *points,
                            int          n_points,
                            DiaColour   *line_colour)
{
  int i, n = 0;

  for (i = 1; i < n_points; ++i) {
    if (points[i].type == BEZ_MOVE_TO) {
      dia_renderer_draw_bezier (self, &points[n], i - n, line_colour);
      n = i;
    }
  }

  /* the last one, if there is one */
  if (i - n > 1) {
    dia_renderer_draw_bezier (self, &points[n], i - n, line_colour);
  }
}
