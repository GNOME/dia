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

#pragma once

#include <glib-object.h>

#include "dia-enums.h"
#include "geometry.h"

/* HACK: Work around circular deps */
typedef struct _DiaRenderer DiaRenderer;

#include "dia-text.h"
#include "diagramdata.h"

G_BEGIN_DECLS

typedef enum /*< flags,prefix=DIA_RENDER,since=0.98 >*/ {
  DIA_RENDER_HOLES = (1<<0),
  DIA_RENDER_ALPHA = (1<<1),
  DIA_RENDER_AFFINE = (1<<2),
  DIA_RENDER_PATTERN = (1<<3)
} G_GNUC_FLAG_ENUM DiaRenderCapability;


#define DIA_TYPE_RENDERER (dia_renderer_get_type ())
G_DECLARE_DERIVABLE_TYPE (DiaRenderer, dia_renderer, DIA, RENDERER, GObject)


struct _DiaRendererClass {
  GObjectClass parent_class;

  void     (*draw_layer)                         (DiaRenderer          *self,
                                                  DiaLayer             *layer,
                                                  gboolean              active,
                                                  DiaRectangle         *update);
  void     (*draw_object)                        (DiaRenderer          *self,
                                                  DiaObject            *object,
                                                  DiaMatrix            *matrix);
  double   (*get_text_width)                     (DiaRenderer          *self,
                                                  const char           *text,
                                                  int                   length);

  /*
   * Function which MUST be implemented by any DiaRenderer
   */
  void     (*begin_render)                       (DiaRenderer          *self,
                                                  const DiaRectangle   *update);
  void     (*end_render)                         (DiaRenderer          *self);

  void     (*set_linewidth)                      (DiaRenderer          *self,
                                                  double                line_width);
  void     (*set_linecaps)                       (DiaRenderer          *self,
                                                  DiaLineCaps           line_caps);
  void     (*set_linejoin)                       (DiaRenderer          *self,
                                                  DiaLineJoin           line_join);
  void     (*set_linestyle)                      (DiaRenderer          *self,
                                                  DiaLineStyle          line_syle,
                                                  double                dash_length);
  void     (*set_fillstyle)                      (DiaRenderer          *self,
                                                  DiaFillStyle          fill_style);
  void     (*font_changed)                       (DiaRenderer          *self,
                                                  DiaFont              *font,
                                                  double                font_height);

  void     (*draw_line)                          (DiaRenderer          *self,
                                                  Point                *start_point,
                                                  Point                *end_point,
                                                  DiaColour            *line_colour);
  void     (*draw_polygon)                       (DiaRenderer          *self,
                                                  Point                *points,
                                                  int                   n_points,
                                                  DiaColour            *fill,
                                                  DiaColour            *stroke);
  void     (*draw_arc)                           (DiaRenderer          *self,
                                                  Point                *centre,
                                                  double                width,
                                                  double                height,
                                                  double                angle1,
                                                  double                angle2,
                                                  DiaColour            *line_colour);
  void     (*fill_arc)                           (DiaRenderer          *self,
                                                  Point                *centre,
                                                  double                width,
                                                  double                height,
                                                  double                angle1,
                                                  double                angle2,
                                                  DiaColour            *fill_colour);
  void     (*draw_ellipse)                       (DiaRenderer          *self,
                                                  Point                *centre,
                                                  double                width,
                                                  double                height,
                                                  DiaColour            *fill,
                                                  DiaColour            *stroke);
  void     (*draw_string)                        (DiaRenderer          *self,
                                                  const char           *text,
                                                  Point                *pos,
                                                  DiaAlignment          alignment,
                                                  DiaColour            *text_colour);
  void     (*draw_image)                         (DiaRenderer          *self,
                                                  Point                *point,
                                                  double                width,
                                                  double                height,
                                                  DiaImage             *image);

  /*
   * Functions which SHOULD be implemented by specific renderer, but
   * have a default implementation based on the above functions
   */
  void     (*draw_bezier)                        (DiaRenderer          *self,
                                                  BezPoint             *points,
                                                  int                   n_points,
                                                  DiaColour            *line_colour);
  void     (*draw_beziergon)                     (DiaRenderer          *self,
                                                  BezPoint             *points,
                                                  int                   n_points,
                                                  DiaColour            *fill,
                                                  DiaColour            *stroke);
  void     (*draw_polyline)                      (DiaRenderer          *self,
                                                  Point                *points,
                                                  int                   n_points,
                                                  DiaColour            *line_colour);
  void     (*draw_text)                          (DiaRenderer          *self,
                                                  DiaText              *text);
  void     (*draw_text_line)                     (DiaRenderer          *self,
                                                  TextLine             *text_line,
                                                  Point                *pos,
                                                  DiaAlignment          alignment,
                                                  DiaColour            *text_colour);
  void     (*draw_rect)                          (DiaRenderer          *self,
                                                  Point                *ul_corner,
                                                  Point                *lr_corner,
                                                  DiaColour            *fill,
                                                  DiaColour            *stroke);

  /*
   * Highest level functions, probably only to be implemented by
   * special 'high level' renderers
   */
  void     (*draw_rounded_rect)                  (DiaRenderer          *self,
                                                  Point                *ul_corner,
                                                  Point                *lr_corner,
                                                  DiaColour            *fill,
                                                  DiaColour            *stroke,
                                                  double                radius);
  void     (*draw_rounded_polyline)              (DiaRenderer          *self,
                                                  Point                *points,
                                                  int                   n_points,
                                                  DiaColour            *line_colour,
                                                  double                radius);
  void     (*draw_line_with_arrows)              (DiaRenderer          *self,
                                                  Point                *start_point,
                                                  Point                *end_point,
                                                  double                line_width,
                                                  DiaColour            *line_colour,
                                                  Arrow                *start_arrow,
                                                  Arrow                *end_arrow);
  void     (*draw_arc_with_arrows)               (DiaRenderer          *self,
                                                  Point                *start_point,
                                                  Point                *end_point,
                                                  Point                *mid_point,
                                                  double                line_width,
                                                  DiaColour            *line_colour,
                                                  Arrow                *start_arrow,
                                                  Arrow                *end_arrow);
  void     (*draw_polyline_with_arrows)          (DiaRenderer          *self,
                                                  Point                *points,
                                                  int                   n_points,
                                                  double                line_width,
                                                  DiaColour            *line_colour,
                                                  Arrow                *start_arrow,
                                                  Arrow                *end_arrow);
  void     (*draw_rounded_polyline_with_arrows)  (DiaRenderer          *self,
                                                  Point                *points,
                                                  int                   n_points,
                                                  double                line_width,
                                                  DiaColour            *line_colour,
                                                  Arrow                *start_arrow,
                                                  Arrow                *end_arrow,
                                                  double                radius);
  void     (*draw_bezier_with_arrows)            (DiaRenderer          *self,
                                                  BezPoint             *points,
                                                  int                   n_points,
                                                  double                line_width,
                                                  DiaColour            *line_colour,
                                                  Arrow                *start_arrow,
                                                  Arrow                *end_arrow);
  gboolean (*is_capable_of)                      (DiaRenderer          *self,
                                                  DiaRenderCapability   capabilities);
  void     (*set_pattern)                        (DiaRenderer          *self,
                                                  DiaPattern           *pattern);
  void     (*draw_rotated_text)                  (DiaRenderer          *self,
                                                  DiaText              *text,
                                                  Point                *centre,
                                                  double                angle);
  void     (*draw_rotated_image)                 (DiaRenderer          *self,
                                                  Point                *point,
                                                  double                width,
                                                  double                height,
                                                  double                angle,
                                                  DiaImage             *image);
};


void     dia_renderer_draw_layer                         (DiaRenderer          *self,
                                                          DiaLayer             *layer,
                                                          gboolean              active,
                                                          DiaRectangle         *update);
void     dia_renderer_draw_object                        (DiaRenderer          *self,
                                                          DiaObject            *object,
                                                          DiaMatrix            *matrix);
double   dia_renderer_get_text_width                     (DiaRenderer          *self,
                                                          const char           *text,
                                                          int                   length);
void     dia_renderer_begin_render                       (DiaRenderer          *self,
                                                          const DiaRectangle   *update);
void     dia_renderer_end_render                         (DiaRenderer          *self);
void     dia_renderer_set_linewidth                      (DiaRenderer          *self,
                                                          double                line_width);
void     dia_renderer_set_linecaps                       (DiaRenderer          *self,
                                                          DiaLineCaps           line_caps);
void     dia_renderer_set_linejoin                       (DiaRenderer          *self,
                                                          DiaLineJoin           line_join);
void     dia_renderer_set_linestyle                      (DiaRenderer          *self,
                                                          DiaLineStyle          line_syle,
                                                          double                dash_length);
void     dia_renderer_set_fillstyle                      (DiaRenderer          *self,
                                                          DiaFillStyle          fill_style);
void     dia_renderer_set_font                           (DiaRenderer          *self,
                                                          DiaFont              *font,
                                                          double                height);
DiaFont *dia_renderer_get_font                           (DiaRenderer          *self,
                                                          double               *height);
void     dia_renderer_draw_line                          (DiaRenderer          *self,
                                                          Point                *start_point,
                                                          Point                *end_point,
                                                          DiaColour            *line_colour);
void     dia_renderer_draw_polygon                       (DiaRenderer          *self,
                                                          Point                *points,
                                                          int                   n_points,
                                                          DiaColour            *fill,
                                                          DiaColour            *stroke);
void     dia_renderer_draw_arc                           (DiaRenderer          *self,
                                                          Point                *centre,
                                                          double                width,
                                                          double                height,
                                                          double                angle1,
                                                          double                angle2,
                                                          DiaColour            *line_colour);
void     dia_renderer_fill_arc                           (DiaRenderer          *self,
                                                          Point                *centre,
                                                          double                width,
                                                          double                height,
                                                          double                angle1,
                                                          double                angle2,
                                                          DiaColour            *fill_colour);
void     dia_renderer_draw_ellipse                       (DiaRenderer          *self,
                                                          Point                *centre,
                                                          double                width,
                                                          double                height,
                                                          DiaColour            *fill,
                                                          DiaColour            *stroke);
void     dia_renderer_draw_string                        (DiaRenderer          *self,
                                                          const char           *text,
                                                          Point                *pos,
                                                          DiaAlignment          alignment,
                                                          DiaColour            *text_colour);
void     dia_renderer_draw_image                         (DiaRenderer          *self,
                                                          Point                *point,
                                                          double                width,
                                                          double                height,
                                                          DiaImage             *image);
void     dia_renderer_draw_bezier                        (DiaRenderer          *self,
                                                          BezPoint             *points,
                                                          int                   n_points,
                                                          DiaColour            *line_colour);
void     dia_renderer_draw_beziergon                     (DiaRenderer          *self,
                                                          BezPoint             *points,
                                                          int                   n_points,
                                                          DiaColour            *fill,
                                                          DiaColour            *stroke);
void     dia_renderer_draw_polyline                      (DiaRenderer          *self,
                                                          Point                *points,
                                                          int                   n_points,
                                                          DiaColour            *line_colour);
void     dia_renderer_draw_text                          (DiaRenderer          *self,
                                                          DiaText              *text);
void     dia_renderer_draw_text_line                     (DiaRenderer          *self,
                                                          TextLine             *text_line,
                                                          Point                *pos,
                                                          DiaAlignment          alignment,
                                                          DiaColour            *text_colour);
void     dia_renderer_draw_rect                          (DiaRenderer          *self,
                                                          Point                *ul_corner,
                                                          Point                *lr_corner,
                                                          DiaColour            *fill,
                                                          DiaColour            *stroke);
void     dia_renderer_draw_rounded_rect                  (DiaRenderer          *self,
                                                          Point                *ul_corner,
                                                          Point                *lr_corner,
                                                          DiaColour            *fill,
                                                          DiaColour            *stroke,
                                                          double                radius);
void     dia_renderer_draw_rounded_polyline              (DiaRenderer          *self,
                                                          Point                *points,
                                                          int                   n_points,
                                                          DiaColour            *line_colour,
                                                          double                radius);
void     dia_renderer_draw_line_with_arrows              (DiaRenderer          *self,
                                                          Point                *start_point,
                                                          Point                *end_point,
                                                          double                line_width,
                                                          DiaColour            *line_colour,
                                                          Arrow                *start_arrow,
                                                          Arrow                *end_arrow);
void     dia_renderer_draw_arc_with_arrows               (DiaRenderer          *self,
                                                          Point                *start_point,
                                                          Point                *end_point,
                                                          Point                *mid_point,
                                                          double                line_width,
                                                          DiaColour            *line_colour,
                                                          Arrow                *start_arrow,
                                                          Arrow                *end_arrow);
void     dia_renderer_draw_polyline_with_arrows          (DiaRenderer          *self,
                                                          Point                *points,
                                                          int                   n_points,
                                                          double                line_width,
                                                          DiaColour            *line_colour,
                                                          Arrow                *start_arrow,
                                                          Arrow                *end_arrow);
void     dia_renderer_draw_rounded_polyline_with_arrows  (DiaRenderer          *self,
                                                          Point                *points,
                                                          int                   n_points,
                                                          double                line_width,
                                                          DiaColour            *line_colour,
                                                          Arrow                *start_arrow,
                                                          Arrow                *end_arrow,
                                                          double                radius);
void     dia_renderer_draw_bezier_with_arrows            (DiaRenderer          *self,
                                                          BezPoint             *points,
                                                          int                   n_points,
                                                          double                line_width,
                                                          DiaColour            *line_colour,
                                                          Arrow                *start_arrow,
                                                          Arrow                *end_arrow);
gboolean dia_renderer_is_capable_of                      (DiaRenderer          *self,
                                                          DiaRenderCapability   capabilities);
void     dia_renderer_set_pattern                        (DiaRenderer          *self,
                                                          DiaPattern           *pattern);
void     dia_renderer_draw_rotated_text                  (DiaRenderer          *self,
                                                          DiaText              *text,
                                                          Point                *centre,
                                                          double                angle);
void     dia_renderer_draw_rotated_image                 (DiaRenderer          *self,
                                                          Point                *point,
                                                          double                width,
                                                          double                height,
                                                          double                angle,
                                                          DiaImage             *image);

void     dia_renderer_bezier_fill                        (DiaRenderer          *self,
                                                          BezPoint             *points,
                                                          int                   n_points,
                                                          DiaColour            *fill_colour);
void     dia_renderer_bezier_stroke                      (DiaRenderer          *self,
                                                          BezPoint             *points,
                                                          int                   n_points,
                                                          DiaColour            *line_colour);

/*! \brief query DIA_RENDER_BOUNDING_BOXES */
int render_bounding_boxes (void);

G_END_DECLS
