/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * renderer.inc - The Dia renderer interface as include file
 *                to be used with export filters
 *             and
 *                renderer functions to construct complex 
 *                geometric forms by simple drawing commands
 */

/* --- the renderer --- */
#if !defined(MY_RENDERER_NAME)
#error You need to define your private name and renderer struct before including this file!
#endif 

static void begin_render(MyRenderer *renderer, DiagramData *data);
static void end_render(MyRenderer *renderer);
static void set_linewidth(MyRenderer *renderer, real linewidth);
static void set_linecaps(MyRenderer *renderer, LineCaps mode);
static void set_linejoin(MyRenderer *renderer, LineJoin mode);
static void set_linestyle(MyRenderer *renderer, LineStyle mode);
static void set_dashlength(MyRenderer *renderer, real length);
static void set_fillstyle(MyRenderer *renderer, FillStyle mode);
static void set_font(MyRenderer *renderer, Font *font, real height);
static void draw_line(MyRenderer *renderer, 
		      Point *start, Point *end, 
		      Color *line_colour);
static void draw_polyline(MyRenderer *renderer, 
			  Point *points, int num_points, 
			  Color *line_colour);
static void draw_polygon(MyRenderer *renderer, 
			 Point *points, int num_points, 
			 Color *line_colour);
static void fill_polygon(MyRenderer *renderer, 
			 Point *points, int num_points, 
			 Color *line_colour);
static void draw_rect(MyRenderer *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void fill_rect(MyRenderer *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void draw_arc(MyRenderer *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void fill_arc(MyRenderer *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void draw_ellipse(MyRenderer *renderer, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void fill_ellipse(MyRenderer *renderer, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void draw_bezier(MyRenderer *renderer, 
			BezPoint *points,
			int numpoints,
			Color *colour);
static void fill_bezier(MyRenderer *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *colour);
static void draw_string(MyRenderer *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *colour);
static void draw_image(MyRenderer *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);

static RenderOps MyRenderOps = {
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
};

/* Draw an ellipse approximation consisting out of
 * four arcs. 
 */
void
renderer_draw_ellipse_by_arc(
    MyRenderer *renderer, 
    Point *center,
    real width, real height,
    Color *colour)
{
  real a, b, e, d, alpha, c, x, y;
  real g, gamma, r;
  Point pt;
  real angle;

  a = width / 2;
  b = height / 2;
  e = sqrt(a*a - b*b);

  alpha = 0.25*M_PI - asin((e/a) * sin(0.75*M_PI));
  d = 2*a*sin(alpha);

  c = (sin(0.25*M_PI) * (2*e + d)) / sin(0.75*M_PI - alpha);

  y = c * sin (alpha);
  x = c * cos (alpha) - e;

  /* draw arcs */
  g = sqrt((a-x)*(a-x) + y*y);
  gamma = acos((a-x)/g);
  r = (sin(gamma) * g) / sin(M_PI-2*gamma);

  pt.y = center->y;
  angle = (180 * (M_PI-2*gamma)) / M_PI;
  pt.x = center->x + a - r; /* right */
  draw_arc(renderer, &pt, 2*r, 2*r, 360-angle, angle, colour);
  pt.x = center->x - a + r; /* left */
  draw_arc(renderer, &pt, 2*r, 2*r, 180-angle, 180+angle, colour);


  g = sqrt((b-y)*(b-y) + x*x);
  gamma = acos((b-y)/g);
  r = (sin(gamma) * g) / sin(M_PI-2*gamma);

  pt.x = center->x;
  angle = (180 * (M_PI-2*gamma)) / M_PI;
  pt.y = center->y - b + r; /* top */
  draw_arc(renderer, &pt, 2*r, 2*r, 90-angle, 90+angle, colour);
  pt.y = center->y + b - r; /* bottom */
  draw_arc(renderer, &pt, 2*r, 2*r, 270-angle, 270+angle, colour);
}