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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>
#include "diacontext.h"
#include "diainteractiverenderer.h"
#include "boundingbox.h"

#include "arrows.h"
#include "diarenderer.h"
#include "attributes.h"

/**
 * SECTION:arrows
 * @title: Arrow
 * @short_description: Arrow drawing
 *
 * A set of standard arrows to be used at line ends
 */

/**** prototypes ****/
static void
draw_empty_ellipse (DiaRenderer *renderer,
                    Point       *to,
                    Point       *from,
                    real         length,
                    real         width,
                    real         linewidth,
                    Color       *fg_color);
static void
calculate_double_arrow (Point       *second_to,
                        Point       *second_from,
                        const Point *to,
                        const Point *from,
                        real         length);

static void
draw_crow_foot (DiaRenderer *renderer,
                Point       *to,
                Point       *from,
                real         length,
                real         width,
                real         linewidth,
                Color       *fg_color,
                Color       *bg_color);

static int
calculate_diamond (Point       *poly/*[4]*/,
                   const Point *to,
                   const Point *from,
                   real         length,
                   real         width);


G_DEFINE_BOXED_TYPE (Arrow, dia_arrow, dia_arrow_copy, dia_arrow_free)


Arrow *
dia_arrow_copy (Arrow *self)
{
  Arrow *new;

  g_return_val_if_fail (self != NULL, NULL);

  new = g_new0 (Arrow, 1);

  new->type = self->type;
  new->length = self->length;
  new->width = self->width;

  return new;
}


void
dia_arrow_free (Arrow *self)
{
  g_clear_pointer (&self, g_free);
}


/**
 * calculate_arrow_point:
 * @arrow: An arrow to calculate adjustments for.  The arrow type
 *         determines what adjustments are done..
 * @to: Where the arrow points to (e.g. connection point)
 * @from: Where the arrow points from (e.g. bezier control line end)
 * @move_arrow: (out): A place to return the new end point of the arrow head
 *                     (i.e. where 'to' should be to render the arrow head well)
 * @move_line: (out): A place to return the new end point of the line (i.e.
 *                    where 'to' should be to render the connecting line without
 *                    leaving bits of its linewidth outside the arrow).
 * @linewidth: The linewidth used for drawing both arrow and line.
 *
 * This function adjusts the placement of the line and
 * the arrow, so that the arrow doesn't overshoot the connectionpoint and the
 * line doesn't stick out the other end of the arrow.
 */
void
calculate_arrow_point (const Arrow *arrow,
                       const Point *to,
                       const Point *from,
                       Point       *move_arrow,
                       Point       *move_line,
                       real         linewidth)
{
  real add_len;
  real angle;
  Point tmp;
  real dist;
  ArrowType arrow_type = arrow->type;
  /* Otherwise line is drawn through arrow
   * head for some hollow arrow heads
   * */
  if (linewidth == 0.0) {
    linewidth = 0.0001;
  }

  dist = distance_point_point (from, to);
  /*
   * Since some of the calculations are sensitive to small values,
   * ignore small arrowheads.  They won't be visible anyway.
   */
  if (arrow->length < MIN_ARROW_DIMENSION ||
      arrow->width < MIN_ARROW_DIMENSION) {
    arrow_type = ARROW_NONE;
  }

  /* default to non-moving arrow */
  move_arrow->x = 0.0;
  move_arrow->y = 0.0;
  /* First, we move the arrow head backwards.
   * This in most cases just accounts for the linewidth of the arrow.
   * In pointy arrows, this means we must look at the angle of the
   * arrowhead.
   * */
  switch (arrow_type) {
    case ARROW_LINES:
    case ARROW_HOLLOW_TRIANGLE:
    case ARROW_UNFILLED_TRIANGLE:
    case ARROW_FILLED_CONCAVE:
    case ARROW_BLANKED_CONCAVE:
    case ARROW_DOUBLE_HOLLOW_TRIANGLE:
      if (arrow->width < 0.0000001) {
        angle = 75*2*G_PI/360.0; /* -> add_len=0 */
      } else {
        angle = atan (arrow->length/(arrow->width/2));
      }
      if (angle < 75*2*G_PI/360.0) {
        add_len = .5*linewidth/cos(angle);
      } else {
        add_len = 0;
      }

      /* don't move arrow if it would change direction */
      if (fabs (add_len) < dist) {
        *move_arrow = *to;
        point_sub (move_arrow, from);
        point_normalize (move_arrow);
        point_scale (move_arrow, add_len);
      }
      break;
    case ARROW_HALF_HEAD:
      if (arrow->width < 0.0000001) return;
      angle = atan (arrow->length/(arrow->width/2));
      if (angle < 60*2*G_PI/360.0) {
        add_len = linewidth/cos (angle);
      } else {
        add_len = 0;
      }

      /* don't move arrow if it would change direction */
      if (fabs (add_len) < dist) {
        *move_arrow = *to;
        point_sub (move_arrow, from);
        point_normalize (move_arrow);
        point_scale (move_arrow, add_len);
      }
      break;
    case ARROW_FILLED_TRIANGLE:
    case ARROW_HOLLOW_ELLIPSE:
    case ARROW_ROUNDED:
    case ARROW_DIMENSION_ORIGIN:
    case ARROW_BLANKED_DOT:
    case ARROW_BLANKED_BOX:
      add_len = .5*linewidth;

      /* don't move arrow if it would change direction */
      if (fabs (add_len) < dist) {
        *move_arrow = *to;
        point_sub (move_arrow, from);
        point_normalize (move_arrow);
        point_scale (move_arrow, add_len);
      }
      break;
    case ARROW_ONE_EXACTLY:
    case ARROW_ONE_OR_NONE:
    case ARROW_ONE_OR_MANY:
    case ARROW_NONE_OR_MANY:
    case ARROW_NONE:
    case ARROW_HOLLOW_DIAMOND:
    case ARROW_FILLED_DIAMOND:
    case ARROW_SLASHED_CROSS:
    case ARROW_FILLED_ELLIPSE:
    case ARROW_DOUBLE_FILLED_TRIANGLE:
    case ARROW_FILLED_DOT:
    case ARROW_FILLED_BOX:
    case ARROW_SLASH_ARROW:
    case ARROW_INTEGRAL_SYMBOL:
    case ARROW_CROW_FOOT:
    case ARROW_CROSS:
    case ARROW_HALF_DIAMOND:
    case ARROW_OPEN_ROUNDED:
    case ARROW_FILLED_DOT_N_TRIANGLE:
    case ARROW_BACKSLASH:
    case ARROW_THREE_DOTS:
    case MAX_ARROW_TYPE:
    default:
      move_arrow->x = 0.0;
      move_arrow->y = 0.0;
      break;
  }

  /* Now move the line to be behind the arrowhead. */
  switch (arrow_type) {
    case ARROW_LINES:
    case ARROW_HALF_HEAD:
      *move_line = *move_arrow;
      point_scale (move_line, 2.0);
      return;
    case ARROW_HOLLOW_TRIANGLE:
    case ARROW_UNFILLED_TRIANGLE:
    case ARROW_FILLED_TRIANGLE:
    case ARROW_FILLED_ELLIPSE:
    case ARROW_HOLLOW_ELLIPSE:
    case ARROW_ROUNDED:
      *move_line = *move_arrow;
      point_normalize (move_line);
      point_scale (move_line, arrow->length);
      point_add (move_line, move_arrow);
      return;
    case ARROW_HALF_DIAMOND:
    case ARROW_OPEN_ROUNDED:
      /* These don't move the arrow, so *move_arrow can't be used. */
      *move_line = *to;
      point_sub (move_line, from);
      point_normalize (move_line);
      point_scale (move_line, arrow->length);
      point_add (move_line, move_arrow);
      return;
    case ARROW_HOLLOW_DIAMOND:
    case ARROW_FILLED_DIAMOND:
      /* Make move_line be a unit vector in direction of line */
      *move_line = *to;
      point_sub (move_line, from);
      point_normalize (move_line);

      /* Set the length to arrow_length - \/2*linewidth */
      tmp = *move_line;
      point_scale (move_line, arrow->length);
      point_scale (&tmp, G_SQRT2*linewidth);
      point_sub (move_line, &tmp);
      return;
    case ARROW_DIMENSION_ORIGIN:
    case ARROW_BLANKED_DOT:
    case ARROW_BLANKED_BOX:
      *move_line = *move_arrow;
      point_normalize (move_line);
      point_scale (move_line, .5*arrow->length);
      return;
    case ARROW_FILLED_DOT:
    case ARROW_FILLED_BOX:
      *move_line = *to;
      point_sub (move_line, from);
      point_normalize (move_line);
      point_scale (move_line, .5*arrow->length);
      return;
    case ARROW_FILLED_CONCAVE:
    case ARROW_BLANKED_CONCAVE:
      *move_line = *move_arrow;
      point_normalize (move_line);
      point_scale (move_line, .75*arrow->length);
      point_add (move_line, move_arrow);
      return;
    case ARROW_DOUBLE_HOLLOW_TRIANGLE:
      *move_line = *move_arrow;
      point_normalize (move_line);
      tmp = *move_line;
      point_scale (move_line, 2.0*arrow->length);
      point_add (move_line, move_arrow);
      point_scale (&tmp, linewidth);
      point_add (move_line, &tmp);
      return;
    case ARROW_DOUBLE_FILLED_TRIANGLE:
      *move_line = *to;
      point_sub (move_line, from);
      point_normalize (move_line);
      point_scale (move_line, 2*arrow->length);
      return;
    case ARROW_FILLED_DOT_N_TRIANGLE:
      *move_line = *to;
      point_sub (move_line, from);
      point_normalize (move_line);
      point_scale (move_line, arrow->length + arrow->width);
      return;
    case ARROW_THREE_DOTS:
      *move_line = *to;
      point_sub (move_line, from);
      add_len = point_len (move_line);
      point_normalize (move_line);
      if (add_len > 4*arrow->length)
        point_scale (move_line, 2*arrow->length);
      else
        point_scale (move_line, arrow->length);
      return;
    case ARROW_SLASH_ARROW:
    case ARROW_INTEGRAL_SYMBOL:
      *move_line = *to;
      point_sub (move_line, from);
      point_normalize (move_line);
      point_scale (move_line, arrow->length / 2);
      return;
    case ARROW_ONE_EXACTLY:
    case ARROW_ONE_OR_NONE:
    case ARROW_ONE_OR_MANY:
    case ARROW_NONE_OR_MANY:
    case ARROW_NONE:
    case ARROW_SLASHED_CROSS:
    case ARROW_CROW_FOOT:
    case ARROW_CROSS:
    case ARROW_BACKSLASH:
    case MAX_ARROW_TYPE:
    default:
      move_arrow->x = 0.0;
      move_arrow->y = 0.0;
      move_line->x = 0.0;
      move_line->y = 0.0;
      return;
  }
}

/**
 * calculate_arrow:
 * @poly: A three-element array in which to return the three points
 *        involved in making a simple arrow: poly[0] is the right-
 *        hand point, poly[1] is the tip, and poly[2] is the left-hand
 *        point.
 * @to: Where the arrow is pointing to
 * @from: Where the arrow is pointing from (e.g. the end of the stem)
 * @length: How long the arrowhead should be.
 * @width: How wide the arrowhead should be.

 * Calculate the corners of a normal arrow.
 */
static int
calculate_arrow (Point       *poly,
                 const Point *to,
                 const Point *from,
                 real         length,
                 real         width)
{
  Point delta;
  Point orth_delta;
  real len;

  delta = *to;
  point_sub (&delta, from);
  len = point_len (&delta);
  if (len <= 0.0001) {
    delta.x=1.0;
    delta.y=0.0;
  } else {
    delta.x/=len;
    delta.y/=len;
  }

  orth_delta.x = delta.y;
  orth_delta.y = -delta.x;

  point_scale (&delta, length);
  point_scale (&orth_delta, width/2.0);

  poly[0] = *to;
  point_sub (&poly[0], &delta);
  point_sub (&poly[0], &orth_delta);
  poly[1] = *to;
  poly[2] = *to;
  point_sub (&poly[2], &delta);
  point_add (&poly[2], &orth_delta);

  return 3;
}

/**
 * calculate_crow:
 * @poly: A three-element array in which to return the three points
 *        involved in making a simple arrow: poly[0] is the tip, poly[1]
 *        is the right-hand point, and poly[2] is the left-hand point.
 * @to: Where the arrow is pointing to
 * @from: Where the arrow is pointing from (e.g. the end of the stem)
 * @length: How long the arrowhead should be.
 * @width: How wide the arrowhead should be.
 *
 * Calculate the actual point of a crows-foot arrow.
 */
static int
calculate_crow (Point       *poly,
                const Point *to,
                const Point *from,
                real         length,
                real         width)
{
  Point delta;
  Point orth_delta;
  real len;

  delta = *to;
  point_sub (&delta, from);
  len = point_len (&delta);
  if (len <= 0.0001) {
    delta.x=1.0;
    delta.y=0.0;
  } else {
    delta.x/=len;
    delta.y/=len;
  }

  orth_delta.x = delta.y;
  orth_delta.y = -delta.x;

  point_scale (&delta, length);
  point_scale (&orth_delta, width/2.0);

  poly[0] = *to;
  point_sub (&poly[0], &delta);
  poly[1] = *to;
  point_sub (&poly[1], &orth_delta);
  poly[2] = *to;
  point_add (&poly[2], &orth_delta);

  return 3;
}

/**
 * draw_none_or_many:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrow lines.
 * @bg_color: Ignored.
 *
 * todo The ER-drawing methods are ripe for some refactoring.
 *
 * Draw ER arrow for 0..N according to Modern database management,
 *  McFadden/Hoffer/Prescott, Addison-Wessley, 1999
 */
static void
draw_none_or_many (DiaRenderer *renderer,
                   Point       *to,
                   Point       *from,
                   real         length,
                   real         width,
                   real         linewidth,
                   Color       *fg_color,
                   Color       *bg_color)
{
  Point second_from, second_to;

  draw_crow_foot (renderer,
                  to,
                  from,
                  length,
                  width,
                  linewidth,
                  fg_color,
                  bg_color);

  calculate_double_arrow (&second_to, &second_from, to, from, length);
  /* use the middle of the arrow */

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  draw_empty_ellipse (renderer,
                      &second_to,
                      &second_from,
                      length/2,
                      width,
                      linewidth,
                      fg_color);
}

/**
 * draw_one_exactly:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrow lines.
 * @bg_color: Ignored.
 *
 * ER arrow for exactly-one relations according to Modern database management,
 *  McFadden/Hoffer/Prescott, Addison-Wessley, 1999
 */
static void
draw_one_exactly (DiaRenderer *renderer,
                  Point       *to,
                  Point       *from,
                  real         length,
                  real         width,
                  real         linewidth,
                  Color       *fg_color,
                  Color       *bg_color)
{
  Point vl,vt;
  Point bs,be;

  /* the first line */
  point_copy (&vl,from); point_sub (&vl,to);
  if (point_len (&vl) > 0) {
    point_normalize (&vl);
  } else {
    vl.x = 1.0; vl.y = 0.0;
  }
  if (!isfinite (vl.x)) {
    vl.x = 1.0; vl.y = 0.0;
  }
  point_get_perp (&vt,&vl);
  point_copy_add_scaled (&bs,to,&vl,length/2);
  point_copy_add_scaled (&be,&bs,&vt,-width/2.0);
  point_add_scaled (&bs,&vt,width/2.0);

  dia_renderer_draw_line (renderer,&bs,&be,fg_color);

  point_copy_add_scaled (&bs,to,&vl,length);

  point_copy_add_scaled (&be,&bs,&vt,-width/2.0);
  point_add_scaled (&bs,&vt,width/2.0);

  dia_renderer_draw_line (renderer,&bs,&be,fg_color);
}

/**
 * draw_one_or_many:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrow lines.
 * @bg_color: Ignored.
 *
 * ER arrow for 1..N according to Modern database management,
 * McFadden/Hoffer/Prescott, Addison-Wessley, 1999
 */
static void
draw_one_or_many (DiaRenderer *renderer,
                  Point       *to,
                  Point       *from,
                  real         length,
                  real         width,
                  real         linewidth,
                  Color       *fg_color,
                  Color       *bg_color)
{

  Point poly[6];

  draw_crow_foot (renderer,
                  to,
                  from,
                  length,
                  width,
                  linewidth,
                  fg_color,
                  bg_color);

  calculate_arrow (poly, to, from, length, width);

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  dia_renderer_draw_line (renderer, &poly[0], &poly[2], fg_color);
}

/**
 * draw_one_or_none:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrow lines.
 * @bg_color: Ignored.
 *
 * ER arrow for 0,1 according to Modern database management,
 * McFadden/Hoffer/Prescott, Addison-Wessley, 1999
 */
static void
draw_one_or_none (DiaRenderer *renderer,
                  Point       *to,
                  Point       *from,
                  real         length,
                  real         width,
                  real         linewidth,
                  Color       *fg_color,
                  Color       *bg_color)
{
  Point vl,vt;
  Point bs,be;
  Point second_from, second_to;

  /* the  line */
  point_copy (&vl,from); point_sub (&vl,to);
  if (point_len (&vl) > 0) {
    point_normalize (&vl);
  } else {
    vl.x = 1.0; vl.y = 0.0;
  }
  if (!isfinite (vl.x)) {
    vl.x = 1.0; vl.y = 0.0;
  }
  point_get_perp (&vt,&vl);
  point_copy_add_scaled (&bs,to,&vl,length/2);
  point_copy_add_scaled (&be,&bs,&vt,-width/2.0);
  point_add_scaled (&bs,&vt,width/2.0);

  dia_renderer_draw_line (renderer,&bs,&be,fg_color);
  /* the ellipse */
  calculate_double_arrow (&second_to, &second_from, to, from, length);
  draw_empty_ellipse (renderer, &second_to, &second_from, length/2, width, linewidth, fg_color);
}

/**
 * draw_crow_foot:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrow lines.
 * @bg_color: Ignored.
 *
 * Draw a crow's foot arrowhead.
 */
static void
draw_crow_foot (DiaRenderer *renderer,
                Point       *to,
                Point       *from,
                real         length,
                real         width,
                real         linewidth,
                Color       *fg_color,
                Color       *bg_color)
{
  Point poly[3];

  calculate_crow (poly, to, from, length, width);

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);

  dia_renderer_draw_line (renderer, &poly[0], &poly[1], fg_color);
  dia_renderer_draw_line (renderer, &poly[0], &poly[2], fg_color);
}

/**
 * draw_lines:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrow lines.
 *
 * Draw a simple open line arrow.
 */
static void
draw_lines (DiaRenderer *renderer,
            Point       *to,
            Point       *from,
            real         length,
            real         width,
            real         linewidth,
            Color       *fg_color,
            Color       *bg_color)
{
  Point poly[3];

  calculate_arrow (poly, to, from, length, width);

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  dia_renderer_draw_polyline (renderer, poly, 3, fg_color);
}

static int
calculate_ellipse (Point       *poly,
                   const Point *to,
                   const Point *from,
                   real         length,
                   real         width)
{
  return calculate_diamond (poly, to, from, length, width);
}

/**
 * draw_fill_ellipse:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrow lines.
 * @bg_color: Used to fill the ellipse, usually white and non-settable.
 *                 If null, the ellipse is filled with fg_color and slightly
 *                 smaller (by linewidth/2).
 *
 * Draw an arrowhead that is a filled ellipse.
 */
static void
draw_fill_ellipse (DiaRenderer *renderer,
                   Point       *to,
                   Point       *from,
                   real         length,
                   real         width,
                   real         linewidth,
                   Color       *fg_color,
                   Color       *bg_color)
{
  BezPoint bp[5];
  Point vl,vt;

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  if (!bg_color) {
    /* no bg_color means filled ellipse ; we then compensate for the line width
     */
    length += linewidth;
    width += linewidth;
  }
  point_copy (&vl,from); point_sub (&vl,to);
  if (point_len (&vl) > 0) {
    point_normalize (&vl);
  } else {
    vl.x = 1.0; vl.y = 0.0;
  }
  if (!isfinite (vl.x)) {
    vl.x = 1.0; vl.y = 0.0;
  }
  point_get_perp (&vt, &vl);


  /* This pile of crap is quite well handled by gcc. */
  bp[0].type = BEZ_MOVE_TO;
  point_copy (&bp[0].p1,to);
  bp[1].type = bp[2].type = bp[3].type = bp[4].type = BEZ_CURVE_TO;
  point_copy (&bp[4].p3, &bp[0].p1);

  point_copy_add_scaled (&bp[2].p3,&bp[0].p1,&vl,length);
  point_copy_add_scaled (&bp[2].p2,&bp[2].p3,&vt,-width / 4.0);
  point_copy_add_scaled (&bp[3].p1,&bp[2].p3,&vt,width / 4.0);
  point_copy_add_scaled (&bp[1].p1,&bp[0].p1,&vt,-width / 4.0);
  point_copy_add_scaled (&bp[4].p2,&bp[0].p1,&vt,width / 4.0);
  point_copy_add_scaled (&bp[1].p3,&bp[0].p1,&vl,length / 2.0); /* temp */
  point_copy_add_scaled (&bp[3].p3,&bp[1].p3,&vt,width / 2.0);
  point_add_scaled (&bp[1].p3,&vt,-width / 2.0);
  point_copy_add_scaled (&bp[1].p2,&bp[1].p3,&vl,-length / 4.0);
  point_copy_add_scaled (&bp[4].p1,&bp[3].p3,&vl,-length / 4.0);
  point_copy_add_scaled (&bp[2].p1,&bp[1].p3,&vl,length / 4.0);
  point_copy_add_scaled (&bp[3].p2,&bp[3].p3,&vl,length / 4.0);
  if (bg_color) {
    dia_renderer_draw_beziergon (renderer,
                                 bp,
                                 sizeof(bp)/sizeof(bp[0]),
                                 bg_color,
                                 fg_color);
  } else {
    dia_renderer_draw_beziergon (renderer,
                                 bp,
                                 sizeof(bp)/sizeof(bp[0]),
                                 fg_color,
                                 NULL);
  }
}

/**
 * draw_empty_ellipse:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrow lines.
 *
 * Draw an arrowhead that is an ellipse with empty interior.
 */
static void
draw_empty_ellipse (DiaRenderer *renderer,
                    Point       *to,
                    Point       *from,
                    real         length,
                    real         width,
                    real         linewidth,
                    Color       *fg_color)
{
  BezPoint bp[5];
  Point vl,vt;
  Point disp;

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  point_copy (&vl,from);
  point_sub (&vl,to);
  if (point_len (&vl) > 0) {
    point_normalize (&vl);
  } else {
    vl.x = 1.0; vl.y = 0.0;
  }
  if (!isfinite (vl.x)) {
    vl.x = 1.0; vl.y = 0.0;
  }

  point_get_perp (&vt,&vl);

  point_copy (&disp, &vl);
  disp.x *= length/2;
  disp.y *= length/2;

  /* This pile of crap is quite well handled by gcc. */
  bp[0].type = BEZ_MOVE_TO;
  point_copy (&bp[0].p1,to);
  point_add (&bp[0].p1,&disp);
  bp[1].type = bp[2].type = bp[3].type = bp[4].type = BEZ_CURVE_TO;
  point_copy (&bp[4].p3,&bp[0].p1);

  point_copy_add_scaled (&bp[2].p3,&bp[0].p1,&vl,length);
  point_copy_add_scaled (&bp[2].p2,&bp[2].p3,&vt,-width / 4.0);
  point_copy_add_scaled (&bp[3].p1,&bp[2].p3,&vt,width / 4.0);
  point_copy_add_scaled (&bp[1].p1,&bp[0].p1,&vt,-width / 4.0);
  point_copy_add_scaled (&bp[4].p2,&bp[0].p1,&vt,width / 4.0);
  point_copy_add_scaled (&bp[1].p3,&bp[0].p1,&vl,length / 2.0); /* temp */
  point_copy_add_scaled (&bp[3].p3,&bp[1].p3,&vt,width / 2.0);
  point_add_scaled (&bp[1].p3,&vt,-width / 2.0);
  point_copy_add_scaled (&bp[1].p2,&bp[1].p3,&vl,-length / 4.0);
  point_copy_add_scaled (&bp[4].p1,&bp[3].p3,&vl,-length / 4.0);
  point_copy_add_scaled (&bp[2].p1,&bp[1].p3,&vl,length / 4.0);
  point_copy_add_scaled (&bp[3].p2,&bp[3].p3,&vl,length / 4.0);

  dia_renderer_draw_bezier (renderer,bp,sizeof(bp)/sizeof(bp[0]),fg_color);
}

static int
calculate_box (Point       *poly,
               const Point *to,
               const Point *from,
               real         length,
               real         width)
{
  Point vl, vt;
  Point bs, be;

  point_copy (&vl,from); point_sub (&vl,to);
  if (point_len (&vl) > 0) {
    point_normalize (&vl);
  } else {
    vl.x = 1.0; vl.y = 0.0;
  }
  if (!isfinite (vl.x)) {
    vl.x = 1.0; vl.y = 0.0;
  }
  point_get_perp (&vt,&vl);

  point_copy_add_scaled (&bs,to,&vl,length/4);
  point_copy_add_scaled (&be,&bs,&vt,-width/2.0);
  point_add_scaled (&bs,&vt,width/2.0);

  point_copy (&poly[0],to);
  point_copy (&poly[1],&poly[0]);
  point_add_scaled (&poly[0],&vt,width/4.0);
  point_add_scaled (&poly[1],&vt,-width/4.0);
  point_copy_add_scaled (&poly[2],&poly[1],&vl,length/2.0);
  point_copy_add_scaled (&poly[3],&poly[0],&vl,length/2.0);

  poly[4] = bs;
  poly[5] = be;

  return 6;
}

/**
 * draw_fill_box:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrow lines.
 * @bg_color: The color used for the interior of the box.  If
 *                 fg_color == bg_color, the box is rendered slightly smaller.
 *
 * Draw an arrow head that is an (optional) filled box.
 */
static void
draw_fill_box (DiaRenderer *renderer,
               Point       *to,
               Point       *from,
               real         length,
               real         width,
               real         linewidth,
               Color       *fg_color,
               Color       *bg_color)
{
  Point poly[6];
  real lw_factor,clength,cwidth;

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  if (fg_color == bg_color) {
    /* Filled dot */
    lw_factor = linewidth;
  } else {
    /* Hollow dot or dimension origin */
    lw_factor = 0.0;
  }
  clength = length + lw_factor;
  cwidth = width + lw_factor;

  calculate_box (poly, to, from, clength, cwidth);

  if (fg_color == bg_color) {
    dia_renderer_draw_polygon (renderer, poly, 4, fg_color, NULL);
  } else {
    dia_renderer_draw_polygon (renderer, poly, 4, bg_color, fg_color);
  }
  dia_renderer_draw_line (renderer,&poly[4],&poly[5],fg_color);
}

static int
calculate_dot (Point       *poly,
               const Point *to,
               const Point *from,
               real         length,
               real         width)
{
  return calculate_diamond (poly, to, from, length, width);
}

/**
 * draw_fill_dot:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrow lines.
 * @bg_color: The color used for the interior of the dot.
 *
 * bug Need to describe the diff between this and ellipse arrow.
 *
 * Draw a "filled dot" arrow.
 */
static void
draw_fill_dot (DiaRenderer *renderer,
               Point       *to,
               Point       *from,
               real         length,
               real         width,
               real         linewidth,
               Color       *fg_color,
               Color       *bg_color)
{
  BezPoint bp[5];
  Point vl,vt;
  Point bs,be;
  real lw_factor,clength,cwidth;

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  if (fg_color == bg_color) {
    /* Filled dot */
    lw_factor = linewidth;
  } else {
    /* Hollow dot or dimension origin */
    lw_factor = 0.0;
  }
  clength = length + lw_factor;
  cwidth = width + lw_factor;

  point_copy (&vl, from); point_sub (&vl, to);
  if (point_len (&vl) > 0) {
    point_normalize (&vl);
  } else {
    vl.x = 1.0; vl.y = 0.0;
  }
  if (!isfinite (vl.x)) {
    vl.x = 1.0; vl.y = 0.0;
  }
  point_get_perp (&vt,&vl);

  point_copy_add_scaled (&bs,to,&vl,length/4);
  point_copy_add_scaled (&be,&bs,&vt,-width/2.0);
  point_add_scaled (&bs,&vt,width/2.0);

  /* This pile of crap is quite well handled by gcc. */
  bp[0].type = BEZ_MOVE_TO;
  point_copy (&bp[0].p1,to);
  bp[1].type = bp[2].type = bp[3].type = bp[4].type = BEZ_CURVE_TO;
  point_copy (&bp[4].p3,&bp[0].p1);

  point_copy_add_scaled (&bp[2].p3,&bp[0].p1,&vl,clength/2);
  point_copy_add_scaled (&bp[2].p2,&bp[2].p3,&vt,-cwidth / 8.0);
  point_copy_add_scaled (&bp[3].p1,&bp[2].p3,&vt,cwidth / 8.0);
  point_copy_add_scaled (&bp[1].p1,&bp[0].p1,&vt,-cwidth / 8.0);
  point_copy_add_scaled (&bp[4].p2,&bp[0].p1,&vt,cwidth / 8.0);
  point_copy_add_scaled (&bp[1].p3,&bp[0].p1,&vl,clength / 4.0); /* temp */
  point_copy_add_scaled (&bp[3].p3,&bp[1].p3,&vt,cwidth / 4.0);
  point_add_scaled (&bp[1].p3,&vt,-cwidth / 4.0);
  point_copy_add_scaled (&bp[1].p2,&bp[1].p3,&vl,-clength / 8.0);
  point_copy_add_scaled (&bp[4].p1,&bp[3].p3,&vl,-clength / 8.0);
  point_copy_add_scaled (&bp[2].p1,&bp[1].p3,&vl,clength / 8.0);
  point_copy_add_scaled (&bp[3].p2,&bp[3].p3,&vl,clength / 8.0);

  if (!bg_color) {
    /* Means dimension origin */
    Point dos,doe;

    point_copy_add_scaled (&doe,to,&vl,length);
    point_copy_add_scaled (&dos,to,&vl,length/2);

    dia_renderer_draw_line (renderer,&dos,&doe,fg_color);
  } else {
    dia_renderer_draw_beziergon (renderer,bp,sizeof(bp)/sizeof(bp[0]),bg_color,NULL);
  }
  if (fg_color != bg_color) {
    dia_renderer_draw_bezier (renderer,bp,sizeof(bp)/sizeof(bp[0]),fg_color);
  }
  dia_renderer_draw_line (renderer,&bs,&be,fg_color);
}

/**
 * draw_integral:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrow lines.
 * @bg_color: The color used to kludge around the longer stem of this
 *                 arrow.
 *
 * bug The bg_color kludge should not be necessary, arrow pos is adjustable.
 *
 * Draw the integral-sign arrow head.
 */
static void
draw_integral (DiaRenderer *renderer,
               Point       *to,
               Point       *from,
               real         length,
               real         width,
               real         linewidth,
               Color       *fg_color)
{
  BezPoint bp[2];
  Point vl,vt;
  Point bs,be, bs2,be2;
  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  point_copy (&vl,from); point_sub (&vl,to);
  if (point_len (&vl) > 0)
    point_normalize (&vl);
  else {
    vl.x = 1.0; vl.y = 0.0;
  }
  if (!finite(vl.x)) {
    vl.x = 1.0; vl.y = 0.0;
  }
  point_get_perp (&vt,&vl);

  point_copy_add_scaled (&bs,to,&vl,length/2);
  point_copy_add_scaled (&be,&bs,&vt,-width/2.0);
  point_add_scaled (&bs,&vt,width/2.0);

  point_copy_add_scaled (&bs2,to,&vl,length/2);
  point_copy_add_scaled (&be2,&bs2,&vl,length/2);

  bp[0].type = BEZ_MOVE_TO;
  bp[1].type = BEZ_CURVE_TO;
  point_copy_add_scaled (&bp[0].p1,to,&vl,.1*length);
  point_add_scaled (&bp[0].p1,&vt,.4*width);
  point_copy_add_scaled (&bp[1].p3,to,&vl,.9*length);
  point_add_scaled (&bp[1].p3,&vt,-.4*width);
  point_copy_add_scaled (&bp[1].p1,&bp[0].p1,&vl,.35*length);
  point_copy_add_scaled (&bp[1].p2,&bp[1].p3,&vl,-.35*length);

  dia_renderer_draw_line (renderer, &bs2, &be2, fg_color);
  dia_renderer_draw_line (renderer, &bs, &be, fg_color);
  dia_renderer_draw_bezier (renderer,bp,sizeof(bp)/sizeof(bp[0]),fg_color);
}

static int
calculate_slashed (Point       *poly,
                   const Point *to,
                   const Point *from,
                   real         length,
                   real         width)
{
  Point vl,vt;

  point_copy (&vl,from);
  point_sub (&vl,to);
  if (point_len (&vl) > 0) {
    point_normalize (&vl);
  } else {
    vl.x = 1.0; vl.y = 0.0;
  }
  if (!isfinite (vl.x)) {
    vl.x = 1.0; vl.y = 0.0;
  }
  point_get_perp (&vt,&vl);

  point_copy_add_scaled (&poly[2],to,&vl,length/2);
  point_copy_add_scaled (&poly[3],&poly[2],&vt,-width/2.0);
  point_add_scaled (&poly[2],&vt,width/2.0);

  point_copy_add_scaled (&poly[0],to,&vl,length/2);
  point_copy_add_scaled (&poly[1],&poly[0],&vl,length/2);

  point_copy_add_scaled (&poly[4],to,&vl,.1*length);
  point_add_scaled (&poly[4],&vt,.4*width);
  point_copy_add_scaled (&poly[5],to,&vl,.9*length);
  point_add_scaled (&poly[5],&vt,-.4*width);

  return 6;
}

/**
 * draw_slashed:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrow lines.
 * @bg_color: Used for a kludge of "erasing" the line tip instead of
 *                 figuring out the correct way to do this.
 *
 * bug Figure out the right way to do this, avoid kludge.
 *
 * Draw the arrowhead that is a line with a slash through it.
 */
static void
draw_slashed (DiaRenderer *renderer,
              Point       *to,
              Point       *from,
              real         length,
              real         width,
              real         linewidth,
              Color       *fg_color,
              Color       *bg_color)
{
  Point poly[6];

  calculate_slashed (poly, to, from, length, width);

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  dia_renderer_draw_line (renderer, &poly[0], &poly[1], fg_color);
  dia_renderer_draw_line (renderer, &poly[2], &poly[3], fg_color);
  dia_renderer_draw_line (renderer, &poly[4], &poly[5], fg_color);
}

/**
 * calculate_halfhead:
 * @poly: The three-element array to store the result in.
 * @to: Where the arrow points to
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrowhead
 * @width: The width of the arrowhead
 * @linewidth: The width of the lines used to draw the arrow
 *
 * bug Describe better what is put into poly.
 *
 * Calculate positions for the half-head arrow (only left-hand(?) line drawn)
 */
static int
calculate_halfhead (Point       *poly,
                    const Point *to,
                    const Point *from,
                    real         length,
                    real         width)
{
  Point delta;
  Point orth_delta;
  real len;

  delta = *to;
  point_sub (&delta, from);
  len = point_len (&delta);
  if (len <= 0.0001) {
    delta.x=1.0;
    delta.y=0.0;
  } else {
    delta.x/=len;
    delta.y/=len;
  }

  orth_delta.x = delta.y;
  orth_delta.y = -delta.x;

  point_scale (&delta, length);
  point_scale (&orth_delta, width/2.0);

  poly[0] = *to;
  point_sub (&poly[0], &delta);
  point_sub (&poly[0], &orth_delta);
  poly[1] = *to;
  poly[2] = *to;
  point_normalize (&delta);
  point_scale (&delta, 0);
  point_sub (&poly[2], &delta);
  /*  point_add(&poly[2], &orth_delta);*/
  return 3;
}

/**
 * draw_halfhead:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @color: The color used for drawing the arrow lines.
 *
 * Draw a halfhead arrow.
 */
static void
draw_halfhead (DiaRenderer *renderer,
               Point       *to,
               Point       *from,
               real         length,
               real         width,
               real         linewidth,
               Color       *fg_color,
               Color       *bg_color)
{
  Point poly[3];

  calculate_halfhead (poly, to, from, length, width);

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  dia_renderer_draw_polyline (renderer, poly, 3, fg_color);
}

/**
 * draw_triangle:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @color: The color used for drawing the arrow lines.
 *
 * Draw a basic triangular arrow.
 */
static void
draw_triangle (DiaRenderer *renderer,
               Point       *to,
               Point       *from,
               real         length,
               real         width,
               real         linewidth,
               Color       *bg_color,
               Color       *fg_color)
{
  Point poly[3];

  calculate_arrow (poly, to, from, length, width);

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);

  dia_renderer_draw_polygon (renderer, poly, 3, bg_color, fg_color);
}

/**
 * calculate_diamond:
 * @poly: A 4-element arrow to hold the return values:
 *        poly[0] holds the tip of the diamond.
 *        poly[1] holds the right-side tip of the diamond.
 *        poly[2] holds the back end of the diamond.
 *        poly[3] holds the left-side tip of the diamond.
 * @to: The point the arrow points to.
 * @from: The point the arrow points away from (e.g. bezier control line)
 * @length: The length of the arrowhead
 * @width: The width of the arrowhead
 *
 * Calculate the points needed to draw a diamond arrowhead.
 */
static int
calculate_diamond (Point       *poly,
                   const Point *to,
                   const Point *from,
                   real         length,
                   real         width)
{
  Point delta;
  Point orth_delta;
  real len;

  delta = *to;
  point_sub (&delta, from);
  len = sqrt (point_dot (&delta, &delta));
  if (len <= 0.0001) {
    delta.x=1.0;
    delta.y=0.0;
  } else {
    delta.x/=len;
    delta.y/=len;
  }

  orth_delta.x = delta.y;
  orth_delta.y = -delta.x;

  point_scale (&delta, length/2.0);
  point_scale (&orth_delta, width/2.0);

  poly[0] = *to;
  poly[1] = *to;
  point_sub (&poly[1], &delta);
  point_sub (&poly[1], &orth_delta);
  poly[2] = *to;
  point_sub (&poly[2], &delta);
  point_sub (&poly[2], &delta);
  poly[3] = *to;
  point_sub (&poly[3], &delta);
  point_add (&poly[3], &orth_delta);

  return 4;
}

/**
 * draw_diamond:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @color: The color used for drawing the arrowhead.
 *
 * Draw a diamond-shaped arrow head.
 */
static void
draw_diamond (DiaRenderer *renderer,
              Point       *to,
              Point       *from,
              real         length,
              real         width,
              real         linewidth,
              Color       *fill,
              Color       *stroke)
{
  Point poly[4];

  calculate_diamond (poly, to, from, length, width);

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  dia_renderer_draw_polygon (renderer, poly, 4, fill, stroke);
}

/**
 * draw_half_diamond:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @color: The color used for drawing the arrowhead.
 *
 * Draw a right-hand part of a diamond arrowhead.
 */
static void
draw_half_diamond (DiaRenderer *renderer,
                   Point       *to,
                   Point       *from,
                   real         length,
                   real         width,
                   real         linewidth,
                   Color       *fg_color,
                   Color       *bg_color)
{
  Point poly[4];

  calculate_diamond (poly, to, from, length, width);

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  dia_renderer_draw_polyline (renderer, poly+1, 3, fg_color);
}

/**
 * calculate_slashed_cross:
 * @poly: A 6-element array to hold the points calculated:
 * @to: Where the arrow points to.
 * @from: Where the arrow points from (e.g. other end of stem).
 * @length: The length of the arrowhead.
 * @width: The width of the arrowhead.
 *
 * bug Describe what is where in the poly array.
 *
 * Calculate the points needed to draw a slashed-cross arrowhead.
 */
static int
calculate_slashed_cross (Point       *poly,
                         const Point *to,
                         const Point *from,
                         real         length,
                         real         width)
{
  Point delta;
  Point orth_delta;
  real len;
  int i;

  delta = *to;
  point_sub (&delta, from);
  len = sqrt (point_dot (&delta, &delta));
  if (len <= 0.0001) {
    delta.x=1.0;
    delta.y=0.0;
  } else {
    delta.x/=len;
    delta.y/=len;
  }

  orth_delta.x = delta.y;
  orth_delta.y = -delta.x;

  point_scale (&delta, length/2.0);
  point_scale (&orth_delta, width/2.0);

  for (i = 0; i < 6; i++) poly[i] = *to;

  point_add (&poly[1], &delta);

  point_add (&poly[2], &delta);
  point_add (&poly[2], &orth_delta);

  point_sub (&poly[3], &delta);
  point_sub (&poly[3], &orth_delta);

  point_add (&poly[4], &orth_delta);
  point_sub (&poly[5], &orth_delta);

  return 6;
}

/**
 * draw_slashed_cross:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @color: The color used for drawing the arrowhead.
 *
 * Draw a slashed cross arrowhead.
 */
static void
draw_slashed_cross (DiaRenderer *renderer,
                    Point       *to,
                    Point       *from,
                    real         length,
                    real         width,
                    real         linewidth,
                    Color       *fg_color,
                    Color       *bg_color)
{
  Point poly[6];

  calculate_slashed_cross (poly, to, from, length, width);

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  dia_renderer_draw_line (renderer, &poly[0],&poly[1], fg_color);
  dia_renderer_draw_line (renderer, &poly[2],&poly[3], fg_color);
  dia_renderer_draw_line (renderer, &poly[4],&poly[5], fg_color);
}

static int
calculate_backslash (Point       *poly,
                     const Point *to,
                     const Point *from,
                     real         length,
                     real         width)
{
  Point delta;
  Point orth_delta;
  real len;

  delta = *to;
  point_sub (&delta, from);
  len = sqrt (point_dot (&delta, &delta));
  if (len <= 0.0001) {
    delta.x=1.0;
    delta.y=0.0;
  } else {
    delta.x/=len;
    delta.y/=len;
  }

  orth_delta.x = delta.y;
  orth_delta.y = -delta.x;

  point_scale (&delta, length/2.0);
  point_scale (&orth_delta, width/2.0);

  poly[0] = *to;
  point_sub (&poly[0], &delta);
  point_sub (&poly[0], &delta);
  point_sub (&poly[0], &delta);
  point_add (&poly[0], &orth_delta);

  poly[1] = *to;
  point_sub (&poly[1], &delta);
  point_sub (&poly[1], &orth_delta);

  return 2;
}

/**
 * draw_backslash:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @color: The color used for drawing the arrowhead.
 *
 * Draw a backslash arrowhead.
 */
static void
draw_backslash (DiaRenderer *renderer,
                Point       *to,
                Point       *from,
                real         length,
                real         width,
                real         linewidth,
                Color       *fg_color,
                Color       *bg_color)
{
  Point poly[2];

  calculate_backslash (poly, to, from, length, width);

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  dia_renderer_draw_line (renderer, &poly[0], &poly[1], fg_color);
}

/**
 * draw_cross:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @color: The color used for drawing the arrowhead.
 *
 * Draw a cross-like arrowhead.
 */
static void
draw_cross (DiaRenderer *renderer,
            Point       *to,
            Point       *from,
            real         length,
            real         width,
            real         linewidth,
            Color       *fg_color,
            Color       *bg_color)
{
  Point poly[6];

  calculate_arrow (poly, to, from, length, width);

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  dia_renderer_draw_line (renderer, &poly[0],&poly[2], fg_color);
  /*dia_renderer_draw_line (renderer, &poly[4],&poly[5], color); */
}

/**
 * calculate_double_arrow:
 * @second_to: Return value for the point where the second arrowhead
 *                  should point to.
 * @second_from: Return value for the point where the second arrowhead
 *                  should point from.
 * @to: Where the first arrowhead should point to.
 * @from: Where the whole arrow points from (e.g. end of stem)
 * @length: The length of each arrowhead.
 *
 * Calculate where to put the second arrowhead of a double arrow
 */
static void
calculate_double_arrow (Point       *second_to,
                        Point       *second_from,
                        const Point *to,
                        const Point *from,
                        real         length)
{
  Point delta;
  real len;

  delta = *to;
  point_sub (&delta, from);
  len = sqrt (point_dot (&delta, &delta));
  if (len <= 0.0001) {
    delta.x=1.0;
    delta.y=0.0;
  } else {
    delta.x/=len;
    delta.y/=len;
  }

  point_scale (&delta, length/2);

  *second_to = *to;
  point_sub (second_to, &delta);
  point_sub (second_to, &delta);
  *second_from = *from;
  point_add (second_from, &delta);
  point_add (second_from, &delta);
}

/**
 * draw_double_triangle:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @color: The color used for drawing the arrowhead.
 *
 * Draw a double-triangle arrowhead.
 */
static void
draw_double_triangle (DiaRenderer *renderer,
                      Point       *to,
                      Point       *from,
                      real         length,
                      real         width,
                      real         linewidth,
                      Color       *bg_color,
                      Color       *fg_color)
{
  Point second_from, second_to;

  draw_triangle (renderer, to, from, length, width, linewidth, bg_color, fg_color);
  calculate_double_arrow (&second_to, &second_from, to, from, length+linewidth);
  draw_triangle (renderer, &second_to, &second_from, length, width, linewidth, bg_color, fg_color);
}

/**
 * calculate_concave:
 * @poly: A 4-element array of return points:
 *             poly[0] is the tip of the arrow.
 *             poly[1] is the right-hand point of the arrow.
 *             poly[2] is the rear indent point of the arrow.
 *             poly[3] is the left-hand point of the arrow.
 * @to: Where the arrow points to.
 * @from: Where the arrow points from (e.g. bezier control point)
 * @length: The length of the arrow.
 * @width: The width of the arrow.
 *
 * Calculate the points needed to draw a concave arrowhead.
 */
static int
calculate_concave (Point       *poly,
                   const Point *to,
                   const Point *from,
                   real         length,
                   real         width)
{
  Point delta;
  Point orth_delta;
  real len;

  delta = *to;
  point_sub (&delta, from);
  len = sqrt (point_dot (&delta, &delta));
  if (len <= 0.0001) {
    delta.x=1.0;
    delta.y=0.0;
  } else {
    delta.x/=len;
    delta.y/=len;
  }

  orth_delta.x = delta.y;
  orth_delta.y = -delta.x;

  point_scale (&delta, length/4.0);
  point_scale (&orth_delta, width/2.0);

  poly[0] = *to;
  poly[1] = *to;
  point_sub (&poly[1], &delta);
  point_sub (&poly[1], &delta);
  point_sub (&poly[1], &delta);
  point_sub (&poly[1], &delta);
  point_sub (&poly[1], &orth_delta);
  poly[2] = *to;
  point_sub (&poly[2], &delta);
  point_sub (&poly[2], &delta);
  point_sub (&poly[2], &delta);
  poly[3] = *to;
  point_add (&poly[3], &orth_delta);
  point_sub (&poly[3], &delta);
  point_sub (&poly[3], &delta);
  point_sub (&poly[3], &delta);
  point_sub (&poly[3], &delta);

  return 4;
}

/**
 * draw_concave_triangle:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrowhead lines
 * @bg_color: The color used for drawing the arrowhead interior.
 *
 * Draw a concave triangle arrowhead.
 */
static void
draw_concave_triangle (DiaRenderer *renderer,
                       Point       *to,
                       Point       *from,
                       real         length,
                       real         width,
                       real         linewidth,
                       Color       *fg_color,
                       Color       *bg_color)
{
  Point poly[4];

  calculate_concave (poly, to, from, length, width);

  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  if (fg_color == bg_color) {
    dia_renderer_draw_polygon (renderer, poly, 4, bg_color, bg_color);
  } else {
    dia_renderer_draw_polygon (renderer, poly, 4, NULL, fg_color);
  }
}

/**
 * draw_rounded:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrowhead lines
 * @bg_color: Ignored.
 *
 * Draw a rounded (half-circle) arrowhead.
 */
static void
draw_rounded (DiaRenderer *renderer,
              Point       *to,
              Point       *from,
              real         length,
              real         width,
              real         linewidth,
              Color       *fg_color,
              Color       *bg_color)
{
  Point p = *to;
  Point delta;
  real len, rayon;
  real rapport = 1.0;
  real angle_start;

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  delta = *from;

  point_sub (&delta, to);

  len = sqrt (point_dot (&delta, &delta)); /* line length */
  rayon = (length / 2.0);
  if (len > 0.0) {
    /* otherwise no length, no direction - but invalid coords */
    rapport = rayon / len;

    p.x += delta.x * rapport;
    p.y += delta.y * rapport;
  }
  angle_start = 90.0 - dia_asin ((p.y - to->y) / rayon) * (180.0 / G_PI);
  if (p.x - to->x < 0) { angle_start = 360.0 - angle_start; }

  dia_renderer_draw_arc (renderer, &p, width, length, angle_start, angle_start - 180.0, fg_color);

  if (len > 0.0) {
    /* scan-build complains about may be used uninitialized, but nothing is changing len since init */
    p.x += delta.x * rapport;
    p.y += delta.y * rapport;
  }
  dia_renderer_draw_line (renderer, &p, to, fg_color);
}

/**
 * draw_open_rounded:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrowhead lines
 * @bg_color: Ignored.
 *
 * todo Describe the arrowhead better.
 *
 * Draw an open rounded arrowhead.
 */
static void
draw_open_rounded (DiaRenderer *renderer,
                   Point       *to,
                   Point       *from,
                   real         length,
                   real         width,
                   real         linewidth,
                   Color       *fg_color,
                   Color       *bg_color)
{
  Point p = *to;
  Point delta;
  real len, rayon;
  real angle_start;

  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  delta = *from;

  point_sub (&delta, to);

  len = sqrt (point_dot (&delta, &delta)); /* line length */
  rayon = (length / 2.0);
  if (len > 0.0) {
    /* no length, no direction - but invalid coords */
    real rapport = rayon / len;
    p.x += delta.x * rapport;
    p.y += delta.y * rapport;
  }
  angle_start = 90.0 - dia_asin ((p.y - to->y) / rayon) * (180.0 / 3.14);
  if (p.x - to->x < 0) { angle_start = 360.0 - angle_start;  }

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_draw_arc (renderer,
                         &p,
                         width,
                         length,
                         angle_start - 180.0,
                         angle_start,
                         fg_color);
}

/**
 * draw_filled_dot_n_triangle:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrowhead lines
 * @bg_color: Ignored.
 *
 * Draw an arrowhead with a circle in front of a triangle, filled.
 */
static void
draw_filled_dot_n_triangle (DiaRenderer *renderer,
                            Point       *to,
                            Point       *from,
                            real         length,
                            real         width,
                            real         linewidth,
                            Color       *fg_color,
                            Color       *bg_color)
{
  Point p_dot = *to, p_tri = *to, delta;
  real len, rayon;
  real rapport;
  Point poly[3];

  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linewidth (renderer, linewidth);

  delta = *from;

  point_sub (&delta, to);

  len = sqrt (point_dot (&delta, &delta)); /* line length */

  /* dot */
  rayon = (width / 2.0);

  if (len > 0.0) {
    /* no length, no direction - but invalid coords */
    rapport = rayon / len;
    p_dot.x += delta.x * rapport;
    p_dot.y += delta.y * rapport;
  }
  dia_renderer_draw_ellipse (renderer,
                             &p_dot,
                             width,
                             width,
                             fg_color,
                             NULL);
  /* triangle */
  if (len > 0.0) {
    rapport = width / len;
    p_tri.x += delta.x * rapport;
    p_tri.y += delta.y * rapport;
  }
  calculate_arrow (poly, &p_tri, from, length, width);
  dia_renderer_draw_polygon (renderer, poly, 3, fg_color, NULL);
}

/**
 * draw_three_dots:
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @length: The length of the arrow
 * @width: The width of the arrow
 * @linewidth: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrowhead lines
 *
 * Draw an arrowhead that is simply three dots (ellipsis)
 */
static void
draw_three_dots (DiaRenderer *renderer,
                 Point       *to,
                 Point       *from,
                 real         length,
                 real         width,
                 real         linewidth,
                 Color       *fg_color)
{

  gdouble dot_width;
  gdouble hole_width;
  gdouble len;
  gint i;
  Point delta, dot_from, dot_to;

  delta = *to;
  point_sub (&delta, from);
  len = point_len (&delta);
  point_normalize (&delta);

  if (len > 4 * width) {
    width *= 2;
  }
  dot_width = width * 0.2;
  hole_width = width / 3 - dot_width;

  dia_renderer_set_linewidth (renderer, linewidth);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  for (i = 0; i < 3; i++) {
    dot_from.x = to->x - i  * (dot_width + hole_width) * delta.x;
    dot_from.y = to->y - i  * (dot_width + hole_width) * delta.y;
    dot_to.x = to->x - ((i + 1) * dot_width + i * hole_width) * delta.x;
    dot_to.y = to->y - ((i + 1) * dot_width + i * hole_width) * delta.y;
    dia_renderer_draw_line (renderer, &dot_from, &dot_to, fg_color);
  }
}

/* hollow is still filed, but with background color */
static void
draw_hollow_triangle (DiaRenderer *renderer,
                      Point       *to,
                      Point       *from,
                      real         length,
                      real         width,
                      real         linewidth,
                      Color       *fg_color,
                      Color       *bg_color)
{
  draw_triangle (renderer, to, from, length, width, linewidth, bg_color, fg_color);
}

/* unfilled is no background drawn */
static void
draw_filled_triangle (DiaRenderer *renderer,
                      Point       *to,
                      Point       *from,
                      real         length,
                      real         width,
                      real         linewidth,
                      Color       *fg_color,
                      Color       *bg_color)
{
  draw_triangle (renderer, to, from, length, width, linewidth, fg_color, fg_color);
}

static void
draw_unfilled_triangle (DiaRenderer *renderer,
                        Point       *to,
                        Point       *from,
                        real         length,
                        real         width,
                        real         linewidth,
                        Color       *fg_color,
                        Color       *bg_color)
{
  draw_triangle (renderer, to, from, length, width, linewidth, NULL, fg_color);
}

/* hollow is still filed, but with background color */
static void
draw_hollow_diamond (DiaRenderer *renderer,
                     Point       *to,
                     Point       *from,
                     real         length,
                     real         width,
                     real         linewidth,
                     Color       *fg_color,
                     Color       *bg_color)
{
  draw_diamond (renderer, to, from, length, width, linewidth, bg_color, fg_color);
}

static void
draw_filled_diamond (DiaRenderer *renderer,
                     Point       *to,
                     Point       *from,
                     real         length,
                     real         width,
                     real         linewidth,
                     Color       *fg_color,
                     Color       *bg_color)
{
  draw_diamond (renderer, to, from, length, width, linewidth, fg_color, fg_color);
}

static void
draw_filled_ellipse (DiaRenderer *renderer,
                     Point       *to,
                     Point       *from,
                     real         length,
                     real         width,
                     real         linewidth,
                     Color       *fg_color,
                     Color       *bg_color)
{
  draw_fill_ellipse (renderer,to,from,length,width,linewidth,fg_color,fg_color);
}

static void
draw_filled_dot (DiaRenderer *renderer,
                 Point       *to,
                 Point       *from,
                 real         length,
                 real         width,
                 real         linewidth,
                 Color       *fg_color,
                 Color       *bg_color)
{
  draw_fill_dot (renderer,to,from,length,width,linewidth,fg_color,fg_color);
}

static void
draw_filled_box (DiaRenderer *renderer,
                 Point       *to,
                 Point       *from,
                 real         length,
                 real         width,
                 real         linewidth,
                 Color       *fg_color,
                 Color       *bg_color)
{
  draw_fill_box (renderer,to,from,length,width,linewidth,fg_color,fg_color);
}

static void
draw_filled_concave (DiaRenderer *renderer,
                     Point       *to,
                     Point       *from,
                     real         length,
                     real         width,
                     real         linewidth,
                     Color       *fg_color,
                     Color       *bg_color)
{
  draw_concave_triangle (renderer, to, from, length, width, linewidth, fg_color, fg_color);
}

static int
calculate_double_triangle (Point       *poly,
                           const Point *to,
                           const Point *from,
                           real         length,
                           real         width)
{
  Point second_from, second_to;

  calculate_arrow (poly, to, from, length, width);
  calculate_double_arrow (&second_to, &second_from, to, from, length);
  calculate_arrow (poly+3, &second_to, &second_from, length, width);
  return 6;
}

static void
draw_double_hollow_triangle (DiaRenderer *renderer,
                             Point       *to,
                             Point       *from,
                             real         length,
                             real         width,
                             real         linewidth,
                             Color       *fg_color,
                             Color       *bg_color)
{
  draw_double_triangle (renderer, to, from, length, width, linewidth, bg_color, fg_color);
}

static void
draw_double_filled_triangle (DiaRenderer *renderer,
                             Point       *to,
                             Point       *from,
                             real         length,
                             real         width,
                             real         linewidth,
                             Color       *fg_color,
                             Color       *bg_color)
{
  draw_double_triangle (renderer, to, from, length, width, linewidth, fg_color, fg_color);
}

struct ArrowDesc {
  const char  *name;
  ArrowType    enum_value;
  /* calculates the points for the arrow, their number is returned */
  int        (*calculate) (Point       *poly,   /* variable size poly */
                           const Point *to,     /* pointing to */
                           const Point *from,   /* coming from */
                           real         length, /* the arrows length */
                           real         width); /* the arrows width */
  /* draw the arrow, internally calculated with the respective calculate */
  void       (*draw)      (DiaRenderer *renderer,
                           Point       *to,
                           Point       *from,
                           real         length,
                           real         width,
                           real         linewidth, /* the lines width also used in many arrows */
                           Color       *fg_color,  /* the main drawing color */
                           Color       *bg_color); /* not always used */
} arrow_types[] = {
  {NC_("Arrow", "None"),ARROW_NONE},
  {NC_("Arrow", "Lines"),ARROW_LINES, calculate_arrow, draw_lines},
  {NC_("Arrow", "Hollow Triangle"), ARROW_HOLLOW_TRIANGLE, calculate_arrow, draw_hollow_triangle},
  {NC_("Arrow", "Filled Triangle"), ARROW_FILLED_TRIANGLE, calculate_arrow, draw_filled_triangle},
  {NC_("Arrow", "Unfilled Triangle"), ARROW_UNFILLED_TRIANGLE, calculate_arrow, draw_unfilled_triangle},
  {NC_("Arrow", "Hollow Diamond"),ARROW_HOLLOW_DIAMOND, calculate_diamond, draw_hollow_diamond},
  {NC_("Arrow", "Filled Diamond"),ARROW_FILLED_DIAMOND, calculate_diamond, draw_filled_diamond},
  {NC_("Arrow", "Half Diamond"), ARROW_HALF_DIAMOND, calculate_diamond, draw_half_diamond},
  {NC_("Arrow", "Half Head"), ARROW_HALF_HEAD, calculate_halfhead, draw_halfhead},
  {NC_("Arrow", "Slashed Cross"), ARROW_SLASHED_CROSS, calculate_slashed_cross, draw_slashed_cross},
  {NC_("Arrow", "Filled Ellipse"), ARROW_FILLED_ELLIPSE, calculate_ellipse, draw_filled_ellipse},
  {NC_("Arrow", "Hollow Ellipse"), ARROW_HOLLOW_ELLIPSE, calculate_ellipse, draw_fill_ellipse},
  {NC_("Arrow", "Filled Dot"), ARROW_FILLED_DOT, calculate_dot, draw_filled_dot},
  {NC_("Arrow", "Dimension Origin"),ARROW_DIMENSION_ORIGIN},
  {NC_("Arrow", "Blanked Dot"),ARROW_BLANKED_DOT, calculate_dot, draw_fill_dot},
  {NC_("Arrow", "Double Hollow Triangle"),ARROW_DOUBLE_HOLLOW_TRIANGLE, calculate_double_triangle, draw_double_hollow_triangle},
  {NC_("Arrow", "Double Filled Triangle"),ARROW_DOUBLE_FILLED_TRIANGLE, calculate_double_triangle, draw_double_filled_triangle},
  {NC_("Arrow", "Filled Dot and Triangle"), ARROW_FILLED_DOT_N_TRIANGLE},
  {NC_("Arrow", "Filled Box"), ARROW_FILLED_BOX, calculate_box, draw_filled_box},
  {NC_("Arrow", "Blanked Box"),ARROW_BLANKED_BOX, calculate_box, draw_fill_box},
  {NC_("Arrow", "Slashed"), ARROW_SLASH_ARROW, calculate_slashed, draw_slashed},
  {NC_("Arrow", "Integral Symbol"),ARROW_INTEGRAL_SYMBOL},
  {NC_("Arrow", "Crow Foot"), ARROW_CROW_FOOT, calculate_crow, draw_crow_foot},
  {NC_("Arrow", "Cross"),ARROW_CROSS, calculate_arrow, draw_cross},
  {NC_("Arrow", "1-or-many"),ARROW_ONE_OR_MANY},
  {NC_("Arrow", "0-or-many"),ARROW_NONE_OR_MANY},
  {NC_("Arrow", "1-or-0"),ARROW_ONE_OR_NONE},
  {NC_("Arrow", "1 exactly"),ARROW_ONE_EXACTLY},
  {NC_("Arrow", "Filled Concave"),ARROW_FILLED_CONCAVE, calculate_concave, draw_filled_concave},
  {NC_("Arrow", "Blanked Concave"),ARROW_BLANKED_CONCAVE, calculate_concave, draw_concave_triangle},
  {NC_("Arrow", "Round"), ARROW_ROUNDED},
  {NC_("Arrow", "Open Round"), ARROW_OPEN_ROUNDED},
  {NC_("Arrow", "Backslash"), ARROW_BACKSLASH, calculate_backslash, draw_backslash},
  {NC_("Arrow", "Infinite Line"),ARROW_THREE_DOTS},
  {NULL,0}
};

/**
 * arrow_bbox:
 * @self: the arrow
 * @line_width: arrows use the same line width
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @rect: the preintialised bounding box
 *
 * following the signature pattern of lib/boundingbox.h
 * the arrow bounding box is added to the given rect
 */
void
arrow_bbox (const Arrow  *self,
            real          line_width,
            const Point  *to,
            const Point  *from,
            DiaRectangle *rect)
{
  Point poly[6]; /* Attention: must be the maximum used! */
  PolyBBExtras pextra;
  int n_points = 0;
  int idx = arrow_index_from_type (self->type);

  if (ARROW_NONE == self->type) {
    return; /* bbox not growing */
  }

  /* some extra steps necessary for e.g circle shapes? */
  if (arrow_types[idx].calculate) {
    n_points = arrow_types[idx].calculate (poly, to, from, self->length, self->width);
  } else { /* fallback, should vanish */
    n_points = calculate_arrow (poly, to, from, self->length, self->width);
  }
  g_assert (n_points > 0 && n_points <= sizeof(poly)/sizeof(Point));

  pextra.start_trans = pextra.end_trans =
  pextra.start_long = pextra.end_long =
  pextra.middle_trans = line_width/2.0;

  polyline_bbox (poly, n_points, &pextra, TRUE, rect);
}


/**
 * dia_arrow_draw:
 * @self: the #Arrow to draw.
 * @renderer: A renderer instance to draw into
 * @to: The point that the arrow points to.
 * @from: Where the arrow points from (e.g. end of stem)
 * @line_width: The thickness of the lines used to draw the arrow.
 * @fg_color: The color used for drawing the arrowhead lines
 * @bg_color: The color used for drawing the arrowhead interior.
 *
 * Draw any arrowhead.
 *
 * Since: 0.98
 */
void
dia_arrow_draw (Arrow       *self,
                DiaRenderer *renderer,
                Point       *to,
                Point       *from,
                double       line_width,
                Color       *fg_color,
                Color       *bg_color)
{
  switch (self->type) {
    case ARROW_NONE:
      break;
    case ARROW_DIMENSION_ORIGIN:
      draw_fill_dot (renderer,
                     to,
                     from,
                     self->length,
                     self->width,
                     line_width,
                     fg_color,
                     NULL);
      break;
    case ARROW_INTEGRAL_SYMBOL:
      draw_integral (renderer,
                     to,
                     from,
                     self->length,
                     self->width,
                     line_width,
                     fg_color);
      break;
    case ARROW_ONE_OR_MANY:
      draw_one_or_many (renderer,
                        to,
                        from,
                        self->length,
                        self->width,
                        line_width,
                        fg_color,
                        bg_color);
      break;
    case ARROW_NONE_OR_MANY:
      draw_none_or_many (renderer,
                         to,
                         from,
                         self->length,
                         self->width,
                         line_width,
                         fg_color,
                         bg_color);
      break;
    case ARROW_ONE_EXACTLY:
      draw_one_exactly (renderer,
                        to,
                        from,
                        self->length,
                        self->width,
                        line_width,
                        fg_color,
                        bg_color);
      break;
    case ARROW_ONE_OR_NONE:
      draw_one_or_none (renderer,
                        to,
                        from,
                        self->length,
                        self->width,
                        line_width,
                        fg_color,
                        bg_color);
      break;
    case ARROW_ROUNDED:
      draw_rounded (renderer,
                    to,
                    from,
                    self->length,
                    self->width,
                    line_width,
                    fg_color,
                    bg_color);
      break;
    case ARROW_OPEN_ROUNDED:
      draw_open_rounded (renderer,
                         to,
                         from,
                         self->length,
                         self->width,
                         line_width,
                         fg_color,
                         bg_color);
      break;
    case ARROW_FILLED_DOT_N_TRIANGLE:
      draw_filled_dot_n_triangle (renderer,
                                  to,
                                  from,
                                  self->length,
                                  self->width,
                                  line_width,
                                  fg_color,
                                  bg_color);
      break;
    case ARROW_THREE_DOTS:
      draw_three_dots (renderer,
                       to,
                       from,
                       self->length,
                       self->width,
                       line_width,
                       fg_color);
      break;
    case MAX_ARROW_TYPE:
      break;
    case ARROW_LINES:
    case ARROW_HOLLOW_TRIANGLE:
    case ARROW_FILLED_TRIANGLE:
    case ARROW_HOLLOW_DIAMOND:
    case ARROW_FILLED_DIAMOND:
    case ARROW_HALF_HEAD:
    case ARROW_SLASHED_CROSS:
    case ARROW_FILLED_ELLIPSE:
    case ARROW_HOLLOW_ELLIPSE:
    case ARROW_DOUBLE_HOLLOW_TRIANGLE:
    case ARROW_DOUBLE_FILLED_TRIANGLE:
    case ARROW_UNFILLED_TRIANGLE:
    case ARROW_FILLED_DOT:
    case ARROW_BLANKED_DOT:
    case ARROW_FILLED_BOX:
    case ARROW_BLANKED_BOX:
    case ARROW_SLASH_ARROW:
    case ARROW_CROW_FOOT:
    case ARROW_CROSS:
    case ARROW_FILLED_CONCAVE:
    case ARROW_BLANKED_CONCAVE:
    case ARROW_HALF_DIAMOND:
    case ARROW_BACKSLASH:
    default:
      {
        int idx = arrow_index_from_type (self->type);
        g_return_if_fail (arrow_types[idx].draw != NULL);
        arrow_types[idx].draw (renderer,
                               to,
                               from,
                               self->length,
                               self->width,
                               line_width,
                               fg_color,
                               bg_color);
        break;
      }
  }

  if ((self->type != ARROW_NONE) && (render_bounding_boxes ()) && DIA_IS_INTERACTIVE_RENDERER (renderer)) {
    DiaRectangle bbox = {0, };
    Point p1, p2;
    Color col = { 1.0, 0.0, 1.0, 1.0 };

    arrow_bbox (self, line_width, to, from, &bbox);

    p1.x = bbox.left;
    p1.y = bbox.top;
    p2.x = bbox.right;
    p2.y = bbox.bottom;

    dia_renderer_set_linewidth (renderer, 0.01);
    dia_renderer_draw_rect (renderer, &p1, &p2, NULL, &col);
  }
}

/* *** Loading and saving arrows. *** */

/**
 * sanitize_arrow:
 * @self: An #Arrow object. This object may be modified to comply with
 * restrictions of %MIN_ARROW_DIMENSION on its length and width and to have a
 * legal head type.
 *
 * Makes sure an arrow object is within reasonable limits
 */
static void
sanitize_arrow (Arrow *self, DiaContext *ctx)
{
  if (self->type > MAX_ARROW_TYPE) {
    dia_context_add_message (ctx, _("Arrow head of unknown type"));
    self->type = ARROW_NONE;
    self->width = DEFAULT_ARROW_WIDTH;
    self->length = DEFAULT_ARROW_LENGTH;
  }

  if (self->length < MIN_ARROW_DIMENSION ||
      self->width < MIN_ARROW_DIMENSION) {
    dia_context_add_message (ctx,
                             _("Arrow head of type %s has too small dimensions; removing.\n"),
                             arrow_get_name_from_type (self->type));
    self->type = ARROW_NONE;
    self->width = DEFAULT_ARROW_WIDTH;
    self->length = DEFAULT_ARROW_LENGTH;
  }
}


/**
 * dia_arrow_save:
 * @arrow: the arrow to save.
 * @obj_node: The XML node to save to.
 * @type_attribute: the name of the attribute of the arrow type.
 * @length_attribute: the name of the attribute of the arrow length.
 * @width_attribute: the name of the attribte of the arrow width.
 * @ctx: a #DiaContext
 *
 * Save the arrow information into three attributes.
 *
 * Since: 0.98
 */
void
dia_arrow_save (Arrow      *arrow,
                ObjectNode  obj_node,
                char       *type_attribute,
                char       *length_attribute,
                char       *width_attribute,
                DiaContext *ctx)
{
  data_add_enum (new_attribute (obj_node, type_attribute),
                 arrow->type, ctx);
  data_add_real (new_attribute (obj_node, length_attribute),
                 arrow->length, ctx);
  data_add_real (new_attribute (obj_node, width_attribute),
                 arrow->width, ctx);
}


/**
 * dia_arrow_load:
 * @arrow: the arrow to store the data info.
 * @obj_node: The XML node to load from.
 * @type_attribute: the name of the attribute of the arrow type.
 * @length_attribute: the name of the attribute of the arrow length.
 * @width_attribute: the name of the attribte of the arrow width.
 * @ctx: the current #DiaContext
 *
 * Load arrow information from three attributes.
 *
 * Since: 0.98
 */
void
dia_arrow_load (Arrow      *arrow,
                ObjectNode  obj_node,
                char       *type_attribute,
                char       *length_attribute,
                char       *width_attribute,
                DiaContext *ctx)
{
  AttributeNode attr;

  arrow->type = ARROW_NONE;
  arrow->length = DEFAULT_ARROW_LENGTH;
  arrow->width = DEFAULT_ARROW_WIDTH;

  attr = object_find_attribute (obj_node, type_attribute);
  if (attr != NULL) {
    arrow->type = data_enum (attribute_first_data (attr), ctx);
  }

  attr = object_find_attribute (obj_node, length_attribute);
  if (attr != NULL) {
    arrow->length = data_real (attribute_first_data (attr),ctx);
  }

  attr = object_find_attribute (obj_node, width_attribute);
  if (attr != NULL) {
    arrow->width = data_real (attribute_first_data (attr),ctx);
  }

  sanitize_arrow (arrow, ctx);
}


/**
 * arrow_type_from_name:
 * @name: The name of an arrow type (case sensitive)
 *
 * Returns the arrow type that corresponds to a given name.
 *
 * Returns The arrow type (see #ArrowType enum in arrows.h).  Returns
 *         %ARROW_NONE if the name doesn't match any known arrow head type.
 */
ArrowType
arrow_type_from_name(const gchar *name)
{
  int i;
  for (i = 0; arrow_types[i].name != NULL; i++) {
    if (!strcmp (arrow_types[i].name, name)) {
      return arrow_types[i].enum_value;
    }
  }
  g_printerr ("Unknown arrow type %s\n", name);
  return 0;
}

/**
 * arrow_index_from_type:
 * @atype: An arrow type as defined in arrows.h

 * Return the index into the arrow_types array (which is sorted by an
 * arbitrary order that we like) for a given arrow type.
 *
 * Returns: An index into the arrow_types array.
 */
gint
arrow_index_from_type (ArrowType atype)
{
  int i = 0;

  for (i = 0; arrow_types[i].name != NULL; i++) {
    if (arrow_types[i].enum_value == atype) {
      return i;
    }
  }
  g_printerr ("Can't find arrow index for type %d\n", atype);
  return 0;
}

/**
 * arrow_type_from_index:
 * @index: an index
 *
 * Return the arrow type for a given arrow index (preferred sorting)
 */
ArrowType
arrow_type_from_index (gint index)
{
  if (index >= 0 && index < MAX_ARROW_TYPE) {
    return arrow_types[index].enum_value;
  }
  return MAX_ARROW_TYPE;
}

/**
 * get_arrow_names:
 *
 * Get a list of all known arrow head names, in arrow_types order.
 *
 * Returns: A newly allocated list of the names.  The list should be
 *          freed after use, but the strings are owned by arrow_types.
 */
GList *
get_arrow_names (void)
{
  int i = 0;
  GList *arrows = NULL;

  for (i = 0; arrow_types[i].name != NULL; i++) {
    arrows = g_list_append (arrows, (char*) arrow_types[i].name);
  }
  return arrows;
}

/**
 * arrow_get_name_from_type:
 * @type: A type of arrow.
 *
 * Get the name of an arrow from its type.
 *
 * Returns: The name of the type, if any such arrow type is defined,
 * or else "unknown arrow". This is a static string and should not be
 * freed or modified.
 */
const gchar *
arrow_get_name_from_type (ArrowType type)
{
  if (type < MAX_ARROW_TYPE) {
    return arrow_types[arrow_index_from_type (type)].name;
  }
  return _("unknown arrow");
}
