/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 2002 Lars Clausen
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

/* This file defines the abstract superclass for renderers.  It defines 
 * a bunch of functions to render higher-level structures with lower-level
 * primitives.
 */

#include "config.h"

#include "render.h"

static void begin_render(Renderer *renderer);
static void end_render(Renderer *renderer);
static void set_linewidth(Renderer *renderer, real linewidth);
static void set_linecaps(Renderer *renderer, LineCaps mode);
static void set_linejoin(Renderer *renderer, LineJoin mode);
static void set_linestyle(Renderer *renderer, LineStyle mode);
static void set_dashlength(Renderer *renderer, real length);
static void set_fillstyle(Renderer *renderer, FillStyle mode);
static void set_font(Renderer *renderer, DiaFont *font, real height);
static void draw_line(Renderer *renderer, 
		      Point *start, Point *end, 
		      Color *line_color);
static void draw_polyline(Renderer *renderer, 
			  Point *points, int num_points, 
			  Color *line_color);
static void draw_polygon(Renderer *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void fill_polygon(Renderer *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void draw_rect(Renderer *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void fill_rect(Renderer *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void draw_arc(Renderer *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void fill_arc(Renderer *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void draw_ellipse(Renderer *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void fill_ellipse(Renderer *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void draw_bezier(Renderer *renderer, 
			BezPoint *points,
			int numpoints,
			Color *color);
static void fill_bezier(Renderer *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *color);
static void draw_string(Renderer *renderer,
			const gchar *text,
			Point *pos, Alignment alignment,
			Color *color);
static void draw_image(Renderer *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);
static void draw_rounded_rect(Renderer *renderer, 
			      Point *ul_corner, Point *lr_corner,
			      Color *color, real rounding);
static void fill_rounded_rect(Renderer *renderer, 
			      Point *ul_corner, Point *lr_corner,
			      Color *color, real rounding);
static void draw_line_with_arrows(Renderer *renderer, 
				  Point *start, Point *end, 
				  real line_width,
				  Color *line_color,
				  Arrow *start_arrow,
				  Arrow *end_arrow);
static void draw_polyline_with_arrows(Renderer *renderer, 
				      Point *points, int n_points,
				      real line_width,
				      Color *line_color,
				      Arrow *start_arrow,
				      Arrow *end_arrow);
static void draw_arc_with_arrows(Renderer *renderer, 
				 Point *start, Point *end,
				 Point *midpoint,
				 real line_width,
				 Color *color,
				 Arrow *start_arrow,
				 Arrow *end_arrow);
static void draw_bezier_with_arrows(Renderer *renderer, 
				    BezPoint *points,
				    int num_points,
				    real line_width,
				    Color *color,
				    Arrow *start_arrow,
				    Arrow *end_arrow);
static void draw_object(Renderer *renderer,
			Object *object);

static RenderOps AbstractRenderOps = {
  begin_render,
  end_render,
  
  /* Line attributes: */
  set_linewidth,
  set_linecaps,
  set_linejoin,
  set_linestyle,
  set_dashlength,
  /* Fill attributes: */
  set_fillstyle,
  /* DiaFont stuff: */
  set_font,
  
  /* Lines: */
  draw_line,
  draw_polyline,

  /* Polygons: */
  draw_polygon,
  fill_polygon,

  /* Rectangles: */
  draw_rect,
  fill_rect,
  
  /* Arcs: */
  draw_arc,
  fill_arc,
  
  /* Ellipses: */
  draw_ellipse,
  fill_ellipse,

  /* Bezier curves: */
  draw_bezier,
  fill_bezier,
  
  /* Text: */
  draw_string,

  /* Images: */
  draw_image,

  /* Later additions, down here for binary combatability */
  draw_rounded_rect,
  fill_rounded_rect,

  /* placeholders */
  draw_line_with_arrows, /* DrawLineWithArrowsFunc */
  draw_polyline_with_arrows, /* DrawPolyLineWithArrowsFunc */
  draw_arc_with_arrows, /* DrawArcWithArrowsFunc */
  draw_bezier_with_arrows,

  draw_object,
};

RenderOps *
create_renderops_table() {
  RenderOps *table = g_new(RenderOps, 1);
  *table = AbstractRenderOps;
  return table;
}

static void begin_render(Renderer *renderer){}
static void end_render(Renderer *renderer){}
static void set_linewidth(Renderer *renderer, real linewidth){}
static void set_linecaps(Renderer *renderer, LineCaps mode){}
static void set_linejoin(Renderer *renderer, LineJoin mode){}
static void set_linestyle(Renderer *renderer, LineStyle mode){}
static void set_dashlength(Renderer *renderer, real length){}
static void set_fillstyle(Renderer *renderer, FillStyle mode){}
static void set_font(Renderer *renderer, DiaFont *font, real height){}
static void draw_line(Renderer *renderer, 
		      Point *start, Point *end, 
		      Color *line_color){}
/* Interim draw_polyline.  Doesn't draw nice corners. */
static void draw_polyline(Renderer *renderer, 
			  Point *points, int num_points, 
			  Color *line_color) {
  int i;
  for (i = 0; i < num_points-1; i++) {
    renderer->ops->draw_line(renderer, &points[i], &points[i+1], line_color);
  }
}
/* Interim draw_polyline.  Doesn't draw nice corners. */
static void draw_polygon(Renderer *renderer, 
			 Point *points, int num_points, 
			 Color *line_color) {
  int i;
  for (i = 0; i < num_points-1; i++) {
    renderer->ops->draw_line(renderer, &points[i], &points[i+1], line_color);
  }
  renderer->ops->draw_line(renderer, &points[i], &points[0], line_color);
}
static void fill_polygon(Renderer *renderer, 
			 Point *points, int num_points, 
			 Color *line_color){}
/* Simple draw_rect.  Draws, but loses info about rectangularity. */
static void draw_rect(Renderer *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color) {
  Point ur_corner, ll_corner;
  ur_corner.x = lr_corner->x;
  ur_corner.y = ul_corner->y;
  ur_corner.x = ul_corner->x;
  ur_corner.y = lr_corner->y;
  renderer->ops->draw_line(renderer, ul_corner, &ur_corner, color);
  renderer->ops->draw_line(renderer, &ur_corner, lr_corner, color);
  renderer->ops->draw_line(renderer, lr_corner, &ll_corner, color);
  renderer->ops->draw_line(renderer, &ll_corner, ul_corner, color);
}
static void fill_rect(Renderer *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color){}
static void draw_arc(Renderer *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color){}
static void fill_arc(Renderer *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color){}
static void draw_ellipse(Renderer *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color){}
static void fill_ellipse(Renderer *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color){}
static void draw_bezier(Renderer *renderer, 
			BezPoint *points,
			int numpoints,
			Color *color){}
static void fill_bezier(Renderer *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *color){}
static void draw_string(Renderer *renderer,
			const gchar *text,
			Point *pos, Alignment alignment,
			Color *color){}
static void draw_image(Renderer *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image){}
static void draw_rounded_rect(Renderer *renderer, 
			      Point *ul_corner, Point *lr_corner,
			      Color *color, real radius) {
  Point start, end, center;

  radius = MIN(radius, (lr_corner->x-ul_corner->x)/2);
  radius = MIN(radius, (lr_corner->y-ul_corner->y)/2);
  start.x = center.x = ul_corner->x+radius;
  end.x = lr_corner->x-radius;
  start.y = end.y = ul_corner->y;
  renderer->ops->draw_line(renderer, &start, &end, color);
  start.y = end.y = lr_corner->y;
  renderer->ops->draw_line(renderer, &start, &end, color);

  center.y = ul_corner->y+radius;
  renderer->ops->draw_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  90.0, 180.0, color);
  center.x = end.x;
  renderer->ops->draw_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  0.0, 90.0, color);

  start.y = ul_corner->y+radius;
  start.x = end.x = ul_corner->x;
  end.y = center.y = lr_corner->y-radius;
  renderer->ops->draw_line(renderer, &start, &end, color);
  start.x = end.x = lr_corner->x;
  renderer->ops->draw_line(renderer, &start, &end, color);

  center.y = lr_corner->y-radius;
  center.x = ul_corner->x+radius;
  renderer->ops->draw_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  180.0, 270.0, color);
  center.x = lr_corner->x-radius;
  renderer->ops->draw_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  270.0, 360.0, color);
}
static void fill_rounded_rect(Renderer *renderer, 
			      Point *ul_corner, Point *lr_corner,
			      Color *color, real radius) {
  Point start, end, center;

  radius = MIN(radius, (lr_corner->x-ul_corner->x)/2);
  radius = MIN(radius, (lr_corner->y-ul_corner->y)/2);
  start.x = center.x = ul_corner->x+radius;
  end.x = lr_corner->x-radius;
  start.y = ul_corner->y;
  end.y = lr_corner->y;
  renderer->ops->fill_rect(renderer, &start, &end, color);

  center.y = ul_corner->y+radius;
  renderer->ops->fill_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  90.0, 180.0, color);
  center.x = end.x;
  renderer->ops->fill_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  0.0, 90.0, color);

  start.x = ul_corner->x;
  start.y = ul_corner->y+radius;
  end.x = lr_corner->x;
  end.y = center.y = lr_corner->y-radius;
  renderer->ops->fill_rect(renderer, &start, &end, color);

  center.y = lr_corner->y-radius;
  center.x = ul_corner->x+radius;
  renderer->ops->fill_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  180.0, 270.0, color);
  center.x = lr_corner->x-radius;
  renderer->ops->fill_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  270.0, 360.0, color);
}

static void
draw_line_with_arrows(Renderer *renderer, 
		      Point *startpoint, 
		      Point *endpoint,
		      real line_width,
		      Color *color,
		      Arrow *start_arrow,
		      Arrow *end_arrow)
{
  Point oldstart = *startpoint;
  Point oldend = *endpoint;
  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    Point arrow_head;
    calculate_arrow_point(start_arrow, startpoint, endpoint, 
			  &move_arrow, &move_line,
			  line_width);
    arrow_head = *startpoint;
    point_sub(&arrow_head, &move_arrow);
    point_sub(startpoint, &move_line);
    arrow_draw(renderer, start_arrow->type,
	       &arrow_head, endpoint,
	       start_arrow->length, start_arrow->width,
	       line_width,
	       color, &color_white);
#ifdef STEM
    Point line_start = startpoint;
    startpoint = arrow_head;
    point_normalize(&move);
    point_scale(&move, start_arrow->length);
    point_sub(startpoint, &move);
    renderer->ops->draw_line(renderer, &line_start, startpoint, color);
#endif
  }
  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    Point arrow_head;
    calculate_arrow_point(end_arrow, endpoint, startpoint,
 			  &move_arrow, &move_line,
			  line_width);
    arrow_head = *endpoint;
    point_sub(&arrow_head, &move_arrow);
    point_sub(endpoint, &move_line);
    arrow_draw(renderer, end_arrow->type,
	       &arrow_head, startpoint,
	       end_arrow->length, end_arrow->width,
	       line_width,
	       color, &color_white);
#ifdef STEM
    Point line_start = endpoint;
    endpoint = arrow_head;
    point_normalize(&move);
    point_scale(&move, end_arrow->length);
    point_sub(endpoint, &move);
    renderer->ops->draw_line(renderer, &line_start, endpoint, color);
#endif
  }
  renderer->ops->draw_line(renderer, startpoint, endpoint, color);

  *startpoint = oldstart;
  *endpoint = oldend;
}

static void
draw_polyline_with_arrows(Renderer *renderer, 
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

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    Point arrow_head;
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
    arrow_head = points[firstline];
    point_sub(&arrow_head, &move_arrow);
    point_sub(&points[firstline], &move_line);
    arrow_draw(renderer, start_arrow->type,
	       &arrow_head, &points[firstline+1],
	       start_arrow->length, start_arrow->width,
	       line_width,
	       color, &color_white);
#ifdef STEM
    Point line_start = &points[firstline];
    &points[firstline] = arrow_head;
    point_normalize(&move);
    point_scale(&move, start_arrow->length);
    point_sub(&points[firstline], &move);
    renderer->ops->draw_line(renderer, &line_start, &points[firstline], color);
#endif
  }
  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    Point arrow_head;
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
    arrow_head = points[lastline-1];
    point_sub(&arrow_head, &move_arrow);
    point_sub(&points[lastline-1], &move_line);
    arrow_draw(renderer, end_arrow->type,
	       &arrow_head, &points[lastline-2],
	       end_arrow->length, end_arrow->width,
	       line_width,
	       color, &color_white);
#ifdef STEM
    Point line_start = &points[lastline-1];
    &points[lastline-1] = arrow_head;
    point_normalize(&move);
    point_scale(&move, end_arrow->length);
    point_sub(&points[lastline-1], &move);
    renderer->ops->draw_line(renderer, &line_start, &points[lastline-1], color);
#endif
  }
  /* Don't draw degenerate line segments at end of line */
  renderer->ops->draw_polyline(renderer, &points[firstline], 
			       lastline-firstline, color);

  points[firstline] = oldstart;
  points[lastline-1] = oldend;
}

/** Figure the equation for a line given by two points.
 * Returns FALSE if the line is vertical (infinite a).
 */
static gboolean
points_to_line(real *a, real *b, Point *p1, Point *p2)
{
    if (fabs(p1->x - p2->x) < 0.000000001)
      return FALSE;
    *a = (p2->y-p1->y)/(p2->x-p1->x);
    *b = p1->y-(*a)*p1->x;
    return TRUE;
}

/** Find the intersection between two lines.
 * Returns TRUE if the lines intersect in a single point.
 */
static gboolean
intersection_line_line(Point *cross,
		       Point *p1a, Point *p1b,
		       Point *p2a, Point *p2b)
{
  real a1, b1, a2, b2;

  /* Find coefficients of lines */
  if (!(points_to_line(&a1, &b1, p1a, p1b))) {
    if (!(points_to_line(&a2, &b2, p2a, p2b))) {
      if (fabs(p1a->x-p2a->x) < 0.00000001) {
	*cross = *p1a;
	return TRUE;
      } else return FALSE;
    }
    cross->x = p1a->x;
    cross->y = a2*(p1a->x)+b2;
    return TRUE;
  }
  if (!(points_to_line(&a2, &b2, p2a, p2b))) {
    cross->x = p2a->x;
    cross->y = a1*(p2a->x)+b1;
    return TRUE;
  }
  /* Solve */
  if (fabs(a1-a2) < 0.000000001) {
    if (fabs(b1-b2) < 0.000000001) {
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

/** Given three points, find the center of the circle they describe.
 * Returns FALSE if the center could not be determined (i.e. the points
 * all lie really close together).
 * The renderer should disappear once the debugging is done.
 */
static gboolean
find_center_point(Point *center, Point *p1, Point *p2, Point *p3) 
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
    /* Either the points are really close together, or directly apart */
    printf("Degenerate circle\n");
    if (fabs((p1->x + p2->x + p3->x)/3 - p1->x) < 0.0000001 &&
	fabs((p1->y + p2->y + p3->y)/3 - p1->y) < 0.0000001)
      return FALSE;
    
    return TRUE;
  }
  return TRUE;
}

static real point_cross(Point *p1, Point *p2)
{
  return p1->x*p2->y-p2->x*p1->y;
}

static void
draw_arc_with_arrows(Renderer *renderer, 
		     Point *startpoint, 
		     Point *endpoint,
		     Point *midpoint,
		     real line_width,
		     Color *color,
		     Arrow *start_arrow,
		     Arrow *end_arrow)
{
  Point oldstart = *startpoint;
  Point oldend = *endpoint;
  Point center;
  real width, angle1, angle2;
  Point dot1, dot2;
  gboolean righthand;
  
  if (!find_center_point(&center, startpoint, endpoint, midpoint)) {
    /* Degenerate circle -- should have been caught by the drawer? */
  }

  dot1 = *startpoint;
  point_sub(&dot1, endpoint);
  point_normalize(&dot1);
  dot2 = *midpoint;
  point_sub(&dot2, endpoint);
  point_normalize(&dot2);
  righthand = point_cross(&dot1, &dot2) > 0;
  
  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    Point arrow_head;
    Point arrow_end;
    real tmp;

    arrow_end = *startpoint;
    point_sub(&arrow_end, &center);
    tmp = arrow_end.x;
    if (righthand) {
      arrow_end.x = -arrow_end.y;
      arrow_end.y = tmp;
    } else {
      arrow_end.x = arrow_end.y;
      arrow_end.y = -tmp;
    }
    point_add(&arrow_end, startpoint);

    calculate_arrow_point(start_arrow, startpoint, &arrow_end, 
			  &move_arrow, &move_line,
			  line_width);
    arrow_head = *startpoint;
    point_sub(&arrow_head, &move_arrow);
    point_sub(startpoint, &move_line);
    arrow_draw(renderer, start_arrow->type,
	       &arrow_head, &arrow_end,
	       start_arrow->length, start_arrow->width,
	       line_width,
	       color, &color_white);
#ifdef STEM
    Point line_start = startpoint;
    startpoint = arrow_head;
    point_normalize(&move);
    point_scale(&move, start_arrow->length);
    point_sub(startpoint, &move);
    renderer->ops->draw_line(renderer, &line_start, startpoint, color);
#endif
  }
  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    Point move_arrow, move_line;
    Point arrow_head;
    Point arrow_end;
    real tmp;

    arrow_end = *endpoint;
    point_sub(&arrow_end, &center);
    tmp = arrow_end.x;
    if (righthand) {
      arrow_end.x = arrow_end.y;
      arrow_end.y = -tmp;
    } else {
      arrow_end.x = -arrow_end.y;
      arrow_end.y = tmp;
    }
    point_add(&arrow_end, endpoint);

    calculate_arrow_point(end_arrow, endpoint, &arrow_end,
 			  &move_arrow, &move_line,
			  line_width);
    arrow_head = *endpoint;
    point_sub(&arrow_head, &move_arrow);
    point_sub(endpoint, &move_line);
    arrow_draw(renderer, end_arrow->type,
	       &arrow_head, &arrow_end,
	       end_arrow->length, end_arrow->width,
	       line_width,
	       color, &color_white);
#ifdef STEM
    Point line_start = endpoint;
    endpoint = arrow_head;
    point_normalize(&move);
    point_scale(&move, end_arrow->length);
    point_sub(endpoint, &move);
    renderer->ops->draw_line(renderer, &line_start, endpoint, color);
#endif
  }

  if (!find_center_point(&center, startpoint, endpoint, midpoint)) {
    /* Not sure what to do here */
    *startpoint = oldstart;
    *endpoint = oldend;
    printf("Second degenerate circle\n");
    return;
  }
  width = 2*distance_point_point(&center, startpoint);
  angle1 = -atan2(startpoint->y-center.y, startpoint->x-center.x)*180.0/M_PI;
  while (angle1 < 0.0) angle1 += 360.0;
  angle2 = -atan2(endpoint->y-center.y, endpoint->x-center.x)*180.0/M_PI;
  while (angle2 < 0.0) angle2 += 360.0;
  if (righthand) {
    real tmp = angle1;
    angle1 = angle2;
    angle2 = tmp;
  }
  renderer->ops->draw_arc(renderer, &center, width, width,
			  angle1, angle2, color);

  *startpoint = oldstart;
  *endpoint = oldend;
}

static void
draw_bezier_with_arrows(Renderer *renderer, 
			BezPoint *points,
			int num_points,
			real line_width,
			Color *color,
			Arrow *start_arrow,
			Arrow *end_arrow)
{
  Point startpoint, endpoint;

  startpoint = points[0].p1;
  endpoint = points[num_points-1].p3;

  if (start_arrow != NULL && start_arrow->type != ARROW_NONE) {
    Point move_arrow;
    Point move_line;
    Point arrow_head;
    calculate_arrow_point(start_arrow, &points[0].p1, &points[1].p1,
			  &move_arrow, &move_line,
			  line_width);
    arrow_head = points[0].p1;
    point_sub(&arrow_head, &move_arrow);
    point_sub(&points[0].p1, &move_line);
    arrow_draw(renderer, start_arrow->type,
	       &arrow_head, &points[1].p1,
	       start_arrow->length, start_arrow->width,
	       line_width,
	       color, &color_white);
    if (0) {
      Point line_start = points[0].p1;
      points[0].p1 = arrow_head;
      point_normalize(&move_arrow);
      point_scale(&move_arrow, start_arrow->length);
      point_sub(&points[0].p1, &move_arrow);
      renderer->ops->draw_line(renderer, &line_start, &points[0].p1, color);
    }
  }
  if (end_arrow != NULL && end_arrow->type != ARROW_NONE) {
    Point move_arrow;
    Point move_line;
    Point arrow_head;
    calculate_arrow_point(end_arrow,
			  &points[num_points-1].p3, &points[num_points-1].p2,
			  &move_arrow, &move_line,
			  line_width);
    arrow_head = points[num_points-1].p3;
    point_sub(&arrow_head, &move_arrow);
    point_sub(&points[num_points-1].p3, &move_line);
    arrow_draw(renderer, end_arrow->type,
	       &arrow_head, &points[num_points-1].p2,
	       end_arrow->length, end_arrow->width,
	       line_width,
	       color, &color_white);
    if (0) {
      Point line_start = points[num_points-1].p3;
      points[num_points-1].p3 = arrow_head;
      point_normalize(&move_arrow);
      point_scale(&move_arrow, end_arrow->length);
      point_sub(&points[num_points-1].p3, &move_arrow);
      renderer->ops->draw_line(renderer, &line_start, &points[num_points-1].p3, color);
    }
  }
  renderer->ops->draw_bezier(renderer, points, num_points, color);

  points[0].p1 = startpoint;
  points[num_points-1].p3 = endpoint;
  
}

static void
draw_object(Renderer *renderer,
	    Object *object) {
  object->ops->draw(object, renderer);
}
