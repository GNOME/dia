/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 * 
 * A string pre-renderer. Piggybacks on a normal renderer, and turns 
 * DrawString calls into PreDrawString calls ; all other calls are ignored.
 * Copyright (C) 2001 Cyrille Chepelov <chepelov@calixo.net>
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

#include "string_prerenderer.h"
#include "diagramdata.h"

static void begin_render(StringPrerenderer *renderer, DiagramData *data);
static void end_render(StringPrerenderer *renderer);
static void set_linewidth(StringPrerenderer *renderer, real linewidth);
static void set_linecaps(StringPrerenderer *renderer, LineCaps mode);
static void set_linejoin(StringPrerenderer *renderer, LineJoin mode);
static void set_linestyle(StringPrerenderer *renderer, LineStyle mode);
static void set_dashlength(StringPrerenderer *renderer, real length);
static void set_fillstyle(StringPrerenderer *renderer, FillStyle mode);
static void set_font(StringPrerenderer *renderer, Font *font, real height);
static void draw_line(StringPrerenderer *renderer, 
                      Point *start, Point *end, 
                      Color *line_color);
static void draw_polyline(StringPrerenderer *renderer, 
                          Point *points, int num_points, 
                          Color *line_color);
static void draw_polygon(StringPrerenderer *renderer, 
                         Point *points, int num_points, 
                         Color *line_color);
static void fill_polygon(StringPrerenderer *renderer, 
                         Point *points, int num_points, 
                         Color *line_color);
static void draw_rect(StringPrerenderer *renderer, 
                      Point *ul_corner, Point *lr_corner,
                      Color *color);
static void fill_rect(StringPrerenderer *renderer, 
                      Point *ul_corner, Point *lr_corner,
                      Color *color);
static void draw_arc(StringPrerenderer *renderer, 
                     Point *center,
                     real width, real height,
                     real angle1, real angle2,
                     Color *color);
static void fill_arc(StringPrerenderer *renderer, 
                     Point *center,
                     real width, real height,
                     real angle1, real angle2,
                     Color *color);
static void draw_ellipse(StringPrerenderer *renderer, 
                         Point *center,
                         real width, real height,
                         Color *color);
static void fill_ellipse(StringPrerenderer *renderer, 
                         Point *center,
                         real width, real height,
                         Color *color);
static void draw_bezier(StringPrerenderer *renderer, 
                        BezPoint *points,
                        int numpoints,
                        Color *color);
static void fill_bezier(StringPrerenderer *renderer, 
                        BezPoint *points, 
                        int numpoints,
                        Color *color);
static void draw_string(StringPrerenderer *renderer,
                        const utfchar *text,
                        Point *pos, Alignment alignment,
                        Color *color);
static void draw_image(StringPrerenderer *renderer,
                       Point *point,
                       real width, real height,
                       DiaImage image);

static RenderOps PrerendererOps = {
  (BeginRenderFunc) begin_render,
  (EndRenderFunc) end_render,

  (SetLineWidthFunc) set_linewidth,
  (SetLineCapsFunc) set_linecaps,
  (SetLineJoinFunc) set_linejoin,
  (SetLineStyleFunc) set_linestyle,
  (SetDashLengthFunc) set_dashlength,
  (SetFillStyleFunc) set_fillstyle,
  (SetFontFunc) set_font,
  
  (DrawLineFunc) draw_line,
  (DrawPolyLineFunc) draw_polyline,
  
  (DrawPolygonFunc) draw_polygon,
  (FillPolygonFunc) fill_polygon,

  (DrawRectangleFunc) draw_rect,
  (FillRectangleFunc) fill_rect,

  (DrawArcFunc) draw_arc,
  (FillArcFunc) fill_arc,

  (DrawEllipseFunc) draw_ellipse,
  (FillEllipseFunc) fill_ellipse,

  (DrawBezierFunc) draw_bezier,
  (FillBezierFunc) fill_bezier,
 
  (DrawStringFunc) draw_string,

  (DrawImageFunc) draw_image,
  
  (PreDrawStringFunc) NULL, /* do not change this -- CC */ 
};


StringPrerenderer *
create_string_prerenderer(Renderer *renderer)
{
  static StringPrerenderer *prerenderer;

  if (!prerenderer) {
    prerenderer = g_new0(StringPrerenderer,1);
    prerenderer->renderer.ops = &PrerendererOps;
    prerenderer->renderer.is_interactive = 0;
    prerenderer->renderer.interactive_ops = NULL;
  }
  prerenderer->org_renderer = renderer;
  
  return prerenderer;
}

void 
destroy_string_prerenderer(const StringPrerenderer *prerenderer)
{

}

static void 
draw_string(StringPrerenderer *prerenderer,
            const utfchar *text,
            Point *pos, Alignment alignment,
            Color *color)
{
  Renderer *org_renderer = prerenderer->org_renderer;
  org_renderer->ops->predraw_string(org_renderer,text);
}

/* The rest of this file is just empty stubs. */

static void begin_render(StringPrerenderer *renderer, DiagramData *data) {}
static void end_render(StringPrerenderer *renderer) {}
static void set_linewidth(StringPrerenderer *renderer, real linewidth) {}
static void set_linecaps(StringPrerenderer *renderer, LineCaps mode) {}
static void set_linejoin(StringPrerenderer *renderer, LineJoin mode) {}
static void set_linestyle(StringPrerenderer *renderer, LineStyle mode) {}
static void set_dashlength(StringPrerenderer *renderer, real length) {}
static void set_fillstyle(StringPrerenderer *renderer, FillStyle mode) {}
static void set_font(StringPrerenderer *renderer, Font *font, real height) {}
static void draw_line(StringPrerenderer *renderer, 
                      Point *start, Point *end, 
                      Color *line_color) {}
static void draw_polyline(StringPrerenderer *renderer, 
                          Point *points, int num_points, 
                          Color *line_color) {}
static void draw_polygon(StringPrerenderer *renderer, 
                         Point *points, int num_points, 
                         Color *line_color) {}
static void fill_polygon(StringPrerenderer *renderer, 
                         Point *points, int num_points, 
                         Color *line_color) {}
static void draw_rect(StringPrerenderer *renderer, 
                      Point *ul_corner, Point *lr_corner,
                      Color *color) {}
static void fill_rect(StringPrerenderer *renderer, 
                      Point *ul_corner, Point *lr_corner,
                      Color *color) {}
static void draw_arc(StringPrerenderer *renderer, 
                     Point *center,
                     real width, real height,
                     real angle1, real angle2,
                     Color *color) {}
static void fill_arc(StringPrerenderer *renderer, 
                     Point *center,
                     real width, real height,
                     real angle1, real angle2,
                     Color *color) {}
static void draw_ellipse(StringPrerenderer *renderer, 
                         Point *center,
                         real width, real height,
                         Color *color) {}
static void fill_ellipse(StringPrerenderer *renderer, 
                         Point *center,
                         real width, real height,
                         Color *color) {}
static void draw_bezier(StringPrerenderer *renderer, 
                        BezPoint *points,
                        int numpoints,
                        Color *color) {}
static void fill_bezier(StringPrerenderer *renderer, 
                        BezPoint *points, 
                        int numpoints,
                        Color *color) {}
static void draw_image(StringPrerenderer *renderer,
                       Point *point,
                       real width, real height,
                       DiaImage image) {}



