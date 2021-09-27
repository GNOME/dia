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

/*! \file diarenderer.h -- the basic renderer interface definition */
#ifndef DIA_RENDERER_H
#define DIA_RENDERER_H

#include <glib-object.h>

#include "dia-enums.h"
#include "geometry.h"

/* HACK: Work around circular deps */
typedef struct _DiaRenderer DiaRenderer;

#include "diagramdata.h"

G_BEGIN_DECLS

typedef enum {
  RENDER_HOLES   = (1<<0),
  RENDER_ALPHA   = (1<<1),
  RENDER_AFFINE  = (1<<2),
  RENDER_PATTERN = (1<<3)
} RenderCapability;

#define DIA_TYPE_RENDERER dia_renderer_get_type ()
G_DECLARE_DERIVABLE_TYPE (DiaRenderer, dia_renderer, DIA, RENDERER, GObject)


/**
 * DiaRendererClass:
 * @draw_layer: Render all the visible object in the layer1
 * @draw_object: Calls the objects draw function, which calls the renderer
 *    again Affine transformation is mostly done on the renderer side
 *    for matrix != NULL
 * @get_text_width: Returns the EXACT width of text in cm, using the current
 *    font. There has been some confusion as to the definition of this.
 *    It used to say the width was in pixels, but actual width returned
 *    was cm.  You shouldn't know about pixels anyway.
 * @begin_render: Called before rendering begins. Can be used to do various
 *    pre-rendering setup.
 * @end_render: Called after all rendering is done. Used to do various
 *    clean-ups.
 * @set_linewidth: Set the current line width if linewidth==0, the line will
 *    be an 'hairline'
 * @set_linecaps: Set the current linecap (the way lines are ended)
 * @set_linejoin: Set the current linejoin (the way two lines are
 *    joined together)
 * @set_linestyle: Set the current line style and the dash length, when the
 *    style is not SOLID. A dot will be 10% of length
 * @set_fillstyle: Set the fill style
 * @draw_line: Draw a line from start to end, using color and the current
 *    line style
 * @draw_polygon: the polygon is filled using the current fill type and
 *    stroked with the current line style
 * @draw_arc: Draw an arc, given its center, the bounding box (widget, height)
 *    the start angle and the end angle. It's counter-clockwise
 *    if angle2 > angle1
 * @fill_arc: Same a DrawArcFunc except the arc is filled (a pie-chart)
 * @draw_ellipse: Draw an ellipse, given its center and the bounding box
 * @draw_string: Print a string at pos, using the current font
 * @draw_image: Draw an image, given its bounding box
 * @draw_bezier: Draw a bezier curve, given it's control points.
 * @draw_beziergon: Fill and/or stroke a  closed bezier
 * @draw_polyline: Draw a line joining multiple points, using color and the
 *    current line style
 * @draw_text: Print a #Text. It holds its own information.
 * @draw_text_line: Print a #TextLine. It holds its own font/size information.
 * @draw_rect: Draw a rectangle, given its upper-left and lower-right corners
 * @draw_rounded_rect: Draw a rounded rectangle, given its upper-left and
 *    lower-right corners
 * @draw_rounded_polyline: Draw a line joining multiple points, using color
 *    and the current line style with rounded corners between segments
 * @draw_line_with_arrows: Highest level function doing specific
 *    arrow positioning
 * @draw_arc_with_arrows: Highest level function doing specific
 *    arrow positioning
 * @draw_polyline_with_arrows: Highest level function doing specific
 *    arrow positioning
 * @draw_rounded_polyline_with_arrows:
 * @draw_bezier_with_arrows:
 * @is_capable_to: allows to adapt #DiaObject implementations to certain
 *    renderer capabilities
 * @set_pattern: fill with a pattern, currently only gradient
 * @draw_rotated_text: draw text rotated around center with given
 *    angle in degrees
 * @draw_rotated_image: draw image rotated around center with given angle
 *    in degrees
 *
 * Base class for all of Dia's render facilities
 *
 * Renderers work in close cooperation with #DiaObject. They provide the way to
 * make all the object drawing independent of concrete drawing back-ends
 */
struct _DiaRendererClass
{
  GObjectClass parent_class;

  void     (*draw_layer)                        (DiaRenderer      *renderer,
                                                 DiaLayer         *layer,
                                                 gboolean          active,
                                                 DiaRectangle     *update);
  void     (*draw_object)                       (DiaRenderer      *renderer,
                                                 DiaObject        *object,
                                                 DiaMatrix        *matrix);
  real     (*get_text_width)                    (DiaRenderer      *renderer,
                                                 const gchar      *text,
                                                 int               length);

  /*
   * Function which MUST be implemented by any DiaRenderer
   */
  void     (*begin_render)                      (DiaRenderer      *renderer,
                                                 const DiaRectangle *update);
  void     (*end_render)                        (DiaRenderer      *renderer);

  void     (*set_linewidth)                     (DiaRenderer      *renderer,
                                                 real              linewidth);
  void     (*set_linecaps)                      (DiaRenderer      *renderer,
                                                 DiaLineCaps       mode);
  void     (*set_linejoin)                      (DiaRenderer      *renderer,
                                                 DiaLineJoin       mode);
  void     (*set_linestyle)                     (DiaRenderer      *renderer,
                                                 DiaLineStyle      mode,
                                                 double            length);
  void     (*set_fillstyle)                     (DiaRenderer      *renderer,
                                                 DiaFillStyle      mode);

  void     (*draw_line)                         (DiaRenderer      *renderer,
                                                 Point            *start,
                                                 Point            *end,
                                                 Color            *color);
  void     (*draw_polygon)                      (DiaRenderer      *renderer,
                                                 Point            *points,
                                                 int               num_points,
                                                 Color            *fill,
                                                 Color            *stroke);
  void     (*draw_arc)                          (DiaRenderer      *renderer,
                                                 Point            *center,
                                                 real              width,
                                                 real              height,
                                                 real              angle1,
                                                 real              angle2,
                                                 Color            *color);
  void     (*fill_arc)                          (DiaRenderer      *renderer,
                                                 Point            *center,
                                                 real              width,
                                                 real              height,
                                                 real              angle1,
                                                 real              angle2,
                                                 Color            *color);
  void     (*draw_ellipse)                      (DiaRenderer      *renderer,
                                                 Point            *center,
                                                 real              width,
                                                 real              height,
                                                 Color            *fill,
                                                 Color            *stroke);
  void     (*draw_string)                       (DiaRenderer      *renderer,
                                                 const char       *text,
                                                 Point            *pos,
                                                 DiaAlignment      alignment,
                                                 Color            *color);
  void     (*draw_image)                        (DiaRenderer      *renderer,
                                                 Point            *point,
                                                 real              width,
                                                 real              height,
                                                 DiaImage         *image);

  /*
   * Functions which SHOULD be implemented by specific renderer, but
   * have a default implementation based on the above functions
   */
  void     (*draw_bezier)                       (DiaRenderer      *renderer,
                                                 BezPoint         *points,
                                                 int               numpoints,
                                                 Color            *color);
  void     (*draw_beziergon)                    (DiaRenderer      *renderer,
                                                 BezPoint         *points,
                                                 int               numpoints,
                                                 Color            *fill,
                                                 Color            *stroke);
  void     (*draw_polyline)                     (DiaRenderer      *renderer,
                                                 Point            *points,
                                                 int               num_points,
                                                 Color            *color);
  void     (*draw_text)                         (DiaRenderer      *renderer,
                                                 Text             *text);
  void     (*draw_text_line)                    (DiaRenderer      *renderer,
                                                 TextLine         *text_line,
                                                 Point            *pos,
                                                 DiaAlignment      alignment,
                                                 Color            *color);
  void     (*draw_rect)                         (DiaRenderer      *renderer,
                                                 Point            *ul_corner,
                                                 Point            *lr_corner,
                                                 Color            *fill,
                                                 Color            *stroke);

  /*
   * Highest level functions, probably only to be implemented by
   * special 'high level' renderers
   */
  void     (*draw_rounded_rect)                 (DiaRenderer      *renderer,
                                                 Point            *ul_corner,
                                                 Point            *lr_corner,
                                                 Color            *fill,
                                                 Color            *stroke,
                                                 real              radius);
  void     (*draw_rounded_polyline)             (DiaRenderer      *renderer,
                                                 Point            *points,
                                                 int               num_points,
                                                 Color            *color,
                                                 real              radius );
  void     (*draw_line_with_arrows)             (DiaRenderer      *renderer,
                                                 Point            *start,
                                                 Point            *end,
                                                 real              line_width,
                                                 Color            *line_color,
                                                 Arrow            *start_arrow,
                                                 Arrow            *end_arrow);
  void     (*draw_arc_with_arrows)              (DiaRenderer      *renderer,
                                                 Point            *start,
                                                 Point            *end,
                                                 Point            *midpoint,
                                                 real              line_width,
                                                 Color            *color,
                                                 Arrow            *start_arrow,
                                                 Arrow            *end_arrow);
  void     (*draw_polyline_with_arrows)         (DiaRenderer      *renderer,
                                                 Point            *points,
                                                 int               num_points,
                                                 real              line_width,
                                                 Color            *color,
                                                 Arrow            *start_arrow,
                                                 Arrow            *end_arrow);
  void     (*draw_rounded_polyline_with_arrows) (DiaRenderer      *renderer,
                                                 Point            *points,
                                                 int               num_points,
                                                 real              line_width,
                                                 Color            *color,
                                                 Arrow            *start_arrow,
                                                 Arrow            *end_arrow,
                                                 real              radius);
  void     (*draw_bezier_with_arrows)           (DiaRenderer      *renderer,
                                                 BezPoint         *points,
                                                 int               num_points,
                                                 real              line_width,
                                                 Color            *color,
                                                 Arrow            *start_arrow,
                                                 Arrow            *end_arrow);
  gboolean (*is_capable_to)                     (DiaRenderer      *renderer,
                                                 RenderCapability  cap);
  void     (*set_pattern)                       (DiaRenderer      *renderer,
                                                 DiaPattern       *pat);
  void     (*draw_rotated_text)                 (DiaRenderer      *renderer,
                                                 Text             *text,
                                                 Point            *center,
                                                 real              angle);
  void     (*draw_rotated_image)                (DiaRenderer      *renderer,
                                                 Point            *point,
                                                 real              width,
                                                 real              height,
                                                 real              angle,
                                                 DiaImage         *image);
};

void     dia_renderer_draw_layer                        (DiaRenderer      *self,
                                                         DiaLayer         *layer,
                                                         gboolean          active,
                                                         DiaRectangle     *update);
void     dia_renderer_draw_object                       (DiaRenderer      *self,
                                                         DiaObject        *object,
                                                         DiaMatrix        *matrix);
real     dia_renderer_get_text_width                    (DiaRenderer      *self,
                                                         const gchar      *text,
                                                         int               length);
void     dia_renderer_begin_render                      (DiaRenderer      *self,
                                                         const DiaRectangle *update);
void     dia_renderer_end_render                        (DiaRenderer      *self);
void     dia_renderer_set_linewidth                     (DiaRenderer      *self,
                                                         real              linewidth);
void     dia_renderer_set_linecaps                      (DiaRenderer      *self,
                                                         DiaLineCaps       mode);
void     dia_renderer_set_linejoin                      (DiaRenderer      *self,
                                                         DiaLineJoin       mode);
void     dia_renderer_set_linestyle                     (DiaRenderer      *self,
                                                         DiaLineStyle      mode,
                                                         double            length);
void     dia_renderer_set_fillstyle                     (DiaRenderer      *self,
                                                         DiaFillStyle      mode);
void     dia_renderer_set_font                          (DiaRenderer      *self,
                                                         DiaFont          *font,
                                                         double            height);
DiaFont *dia_renderer_get_font                          (DiaRenderer      *self,
                                                         double           *height);
void     dia_renderer_draw_line                         (DiaRenderer      *self,
                                                         Point            *start,
                                                         Point            *end,
                                                         Color            *color);
void     dia_renderer_draw_polygon                      (DiaRenderer      *self,
                                                         Point            *points,
                                                         int               num_points,
                                                         Color            *fill,
                                                         Color            *stroke);
void     dia_renderer_draw_arc                          (DiaRenderer      *self,
                                                         Point            *center,
                                                         real              width,
                                                         real              height,
                                                         real              angle1,
                                                         real              angle2,
                                                         Color            *color);
void     dia_renderer_fill_arc                          (DiaRenderer      *self,
                                                         Point            *center,
                                                         real              width,
                                                         real              height,
                                                         real              angle1,
                                                         real              angle2,
                                                         Color            *color);
void     dia_renderer_draw_ellipse                      (DiaRenderer      *self,
                                                         Point            *center,
                                                         real              width,
                                                         real              height,
                                                         Color            *fill,
                                                         Color            *stroke);
void     dia_renderer_draw_string                       (DiaRenderer      *self,
                                                         const char       *text,
                                                         Point            *pos,
                                                         DiaAlignment      alignment,
                                                         Color            *color);
void     dia_renderer_draw_image                        (DiaRenderer      *self,
                                                         Point            *point,
                                                         real              width,
                                                         real              height,
                                                         DiaImage         *image);
void     dia_renderer_draw_bezier                       (DiaRenderer      *self,
                                                         BezPoint         *points,
                                                         int               numpoints,
                                                         Color            *color);
void     dia_renderer_draw_beziergon                    (DiaRenderer      *self,
                                                         BezPoint         *points,
                                                         int               numpoints,
                                                         Color            *fill,
                                                         Color            *stroke);
void     dia_renderer_draw_polyline                     (DiaRenderer      *self,
                                                         Point            *points,
                                                         int               num_points,
                                                         Color            *color);
void     dia_renderer_draw_text                         (DiaRenderer      *self,
                                                         Text             *text);
void     dia_renderer_draw_text_line                    (DiaRenderer      *self,
                                                         TextLine         *text_line,
                                                         Point            *pos,
                                                         DiaAlignment      alignment,
                                                         Color            *color);
void     dia_renderer_draw_rect                         (DiaRenderer      *self,
                                                         Point            *ul_corner,
                                                         Point            *lr_corner,
                                                         Color            *fill,
                                                         Color            *stroke);
void     dia_renderer_draw_rounded_rect                 (DiaRenderer      *self,
                                                         Point            *ul_corner,
                                                         Point            *lr_corner,
                                                         Color            *fill,
                                                         Color            *stroke,
                                                         real              radius);
void     dia_renderer_draw_rounded_polyline             (DiaRenderer      *self,
                                                         Point            *points,
                                                         int               num_points,
                                                         Color            *color,
                                                         real              radius);
void     dia_renderer_draw_line_with_arrows             (DiaRenderer      *self,
                                                         Point            *start,
                                                         Point            *end,
                                                         real              line_width,
                                                         Color            *line_color,
                                                         Arrow            *start_arrow,
                                                         Arrow            *end_arrow);
void     dia_renderer_draw_arc_with_arrows              (DiaRenderer      *self,
                                                         Point            *start,
                                                         Point            *end,
                                                         Point            *midpoint,
                                                         real              line_width,
                                                         Color            *color,
                                                         Arrow            *start_arrow,
                                                         Arrow            *end_arrow);
void     dia_renderer_draw_polyline_with_arrows         (DiaRenderer      *self,
                                                         Point            *points,
                                                         int               num_points,
                                                         real              line_width,
                                                         Color            *color,
                                                         Arrow            *start_arrow,
                                                         Arrow            *end_arrow);
void     dia_renderer_draw_rounded_polyline_with_arrows (DiaRenderer      *self,
                                                         Point            *points,
                                                         int               num_points,
                                                         real              line_width,
                                                         Color            *color,
                                                         Arrow            *start_arrow,
                                                         Arrow            *end_arrow,
                                                         real              radius);
void     dia_renderer_draw_bezier_with_arrows           (DiaRenderer      *self,
                                                         BezPoint         *points,
                                                         int               num_points,
                                                         real              line_width,
                                                         Color            *color,
                                                         Arrow            *start_arrow,
                                                         Arrow            *end_arrow);
gboolean dia_renderer_is_capable_of                     (DiaRenderer      *self,
                                                         RenderCapability  cap);
void     dia_renderer_set_pattern                       (DiaRenderer      *self,
                                                         DiaPattern       *pat);
void     dia_renderer_draw_rotated_text                 (DiaRenderer      *self,
                                                         Text             *text,
                                                         Point            *center,
                                                         real              angle);
void     dia_renderer_draw_rotated_image                (DiaRenderer      *self,
                                                         Point            *point,
                                                         real              width,
                                                         real              height,
                                                         real              angle,
                                                         DiaImage         *image);

void     dia_renderer_bezier_fill                       (DiaRenderer      *self,
                                                         BezPoint         *pts,
                                                         int               total,
                                                         Color            *color);
void     dia_renderer_bezier_stroke                     (DiaRenderer      *self,
                                                         BezPoint         *pts,
                                                         int               total,
                                                         Color            *color);

/*! \brief query DIA_RENDER_BOUNDING_BOXES */
int render_bounding_boxes (void);

G_END_DECLS

#endif /* DIA_RENDERER_H */
