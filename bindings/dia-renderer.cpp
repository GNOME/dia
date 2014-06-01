/*
 * C++ interface of DiaRenderer - just to have something clean to wrap
 *
 * Copyright (C) 2007, Hans Breuer, <Hans@Breuer.Org>
 *
 * This is free software; you can redistribute it and/or modify
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

#include "diarenderer.h"
#include <assert.h>

#include "object.h"
#include "dia-object.h"

#include "dia-renderer.h"

// return width in pixels, only for interactive renderers
int 
dia::Renderer::get_width_pixels () const
{
    assert (self);
    return DIA_RENDERER_GET_CLASS(self)->get_width_pixels (self);
}
//  return width in pixels, only for interactive renderers
int 
dia::Renderer::get_height_pixels () const
{
    assert (self);
    return DIA_RENDERER_GET_CLASS(self)->get_height_pixels (self);
}
// simply calls the objects draw function, which calls this again
void 
dia::Renderer::draw_object (Object* o)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_object (self, o->Self(), NULL);
}
// Returns the EXACT width of text in cm, using the current font.
double 
dia::Renderer::get_text_width (const gchar *text, int length) const
{
    assert (self);
    return DIA_RENDERER_GET_CLASS(self)->get_text_width (self, text, length);
}
// called before any rendering takes palce
void 
dia::Renderer::begin_render (const Rectangle *update)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->begin_render (self, update);
}
// finished rendering
void 
dia::Renderer::end_render ()
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->end_render (self);
}
// set current linewidth
void 
dia::Renderer::set_linewidth (double w)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->set_linewidth (self, w);
}
// set current linecaps
void 
dia::Renderer::set_linecaps (LineCaps mode)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->set_linecaps (self, mode);
}
// set current linejoin
void 
dia::Renderer::set_linejoin (LineJoin join)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->set_linejoin (self, join);
}
// set current linestyle
void 
dia::Renderer::set_linestyle (LineStyle style, real dash_length)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->set_linestyle (self, style, dash_length);
}
// set current font
void 
dia::Renderer::set_font (Font* font, double height)
{
    assert (self);
    //FIXME: implement
}
// Draw a line from start to end, using color and the current line style
void 
dia::Renderer::draw_line (Point *start, Point *end, Color *color)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_line (self, start, end, color);
}
// Fill and/or stroke a rectangle, given its upper-left and lower-right corners
void 
dia::Renderer::draw_rect (Point *ul_corner, Point *lr_corner, Color *fill, Color *stroke)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_rect (self, ul_corner, lr_corner, fill, stroke);
}
// Draw an arc, given its center, the bounding box (widget, height), the start angle and the end angle
void 
dia::Renderer::draw_arc (Point *center, double width, double height,
		         double angle1, double angle2,
		         Color *color)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_arc (self, center, width, height, angle1, angle2, color);
}
// Same a DrawArcFunc except the arc is filled (a pie-chart)
void 
dia::Renderer::fill_arc (Point *center, double width, double height,
		         double angle1, double angle2,
		         Color *color)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->fill_arc (self, center, width, height, angle1, angle2, color);
}
// Draw an ellipse, given its center and the bounding box
void 
dia::Renderer::draw_ellipse (Point *center, double width, double height, Color *fill, Color *stroke)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_ellipse (self, center, width, height, fill, stroke);
}
// Print a string at pos, using the current font
void 
dia::Renderer::draw_string (const gchar *text, Point *pos, Alignment alignment, Color *color)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_string (self, text, pos, alignment, color);
}
// Draw an image, given its bounding box
void 
dia::Renderer::draw_image (Point *point, double width, double height, Image* image)
{
    assert (self);
    //FIXME: DIA_RENDERER_GET_CLASS(self)->draw_image (self, point, width, height, image);
}

// draw a bezier line - possibly as approximation consisting of straight lines
void 
dia::Renderer::draw_bezier (BezPoint *points, int numpoints, Color *color)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_bezier (self, points, numpoints, color);
}
// fill a bezier line - possibly as approximation consisting of a polygon
void 
dia::Renderer::draw_beziergon (BezPoint *points, int numpoints, Color *fill, Color *stroke)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_beziergon (self, points, numpoints, fill, stroke);
}
// drawing a polyline - or fallback to single line segments
void 
dia::Renderer::draw_polyline (Point *points, int num_points, Color *color)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_polyline (self, points, num_points, color);
}
// Draw a polygon, using the current line and/or fill style
void 
dia::Renderer::draw_polygon (Point *points, int num_points, Color *fill, Color *stroke)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_polygon (self, points, num_points, fill, stroke);
}
// draw a Text.  It holds its own information like position, style, ...
void 
dia::Renderer::draw_text (Text* text)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_text (self, text);
}
// Draw a polyline with round corners
void
dia::Renderer::draw_rounded_polyline (Point *points, int num_points, Color *color, double radius )
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_rounded_polyline (self, points, num_points, color, radius);
}
// specialized draw_rect() with round corners
void 
dia::Renderer::draw_rounded_rect (Point *ul_corner, Point *lr_corner,
				  Color *fill, Color *stroke, real radius)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_rounded_rect (self, ul_corner, lr_corner, fill, stroke, radius);
}
// specialized draw_line() for renderers with an own concept of Arrow
void 
dia::Renderer::draw_line_with_arrows  (Point *start, Point *end, real line_width, Color *line_color, 
                                       Arrow *start_arrow, Arrow *end_arrow)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_line_with_arrows (self, start, end, line_width, line_color, start_arrow, end_arrow);
}
// specialized draw_line() for renderers with an own concept of Arrow
void 
dia::Renderer::draw_arc_with_arrows  (Point *start, Point *end, Point *midpoint, real line_width, Color *color,
                                      Arrow *start_arrow, Arrow *end_arrow)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_arc_with_arrows (self, start, end, midpoint, line_width, color, start_arrow, end_arrow);
}
// specialized draw_polyline() for renderers with an own concept of Arrow
void 
dia::Renderer::draw_polyline_with_arrows (Point *points, int num_points, real line_width, Color *color,
                                          Arrow *start_arrow, Arrow *end_arrow)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_polyline_with_arrows (self, points, num_points, line_width, color, start_arrow, end_arrow);
}
// specialized draw_rounded_polyline() for renderers with an own concept of Arrow
void 
dia::Renderer::draw_rounded_polyline_with_arrows (Point *points, int num_points, real line_width, Color *color,
					          Arrow *start_arrow, Arrow *end_arrow, real radius)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_rounded_polyline_with_arrows (self, points, num_points, line_width, color, start_arrow, end_arrow, radius);
}
// specialized draw_bezier() for renderers with an own concept of Arrow
void 
dia::Renderer::draw_bezier_with_arrows (BezPoint *points, int num_points, real line_width, Color *color,
                                        Arrow *start_arrow, Arrow *end_arrow)
{
    assert (self);
    DIA_RENDERER_GET_CLASS(self)->draw_bezier_with_arrows (self, points, num_points, line_width, color, start_arrow, end_arrow);
}
