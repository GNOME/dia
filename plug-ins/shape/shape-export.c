/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * shape-export.c: shape export filter for dia
 * Copyright (C) 2000 Steffen Macke
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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <locale.h>

#include <entities.h>
#if defined(LIBXML_VERSION) && LIBXML_VERSION >= 20000
#define root children
#define childs children
#endif

/* the dots per centimetre to render this diagram at */
/* this matches the setting `100%' setting in dia. */
#define DPCM 20

#include <tree.h>
#include "geometry.h"
#include "render.h"
#include "filter.h"
#include "intl.h"
#include "message.h"
#include "diagramdata.h"

typedef struct _RendererShape RendererShape;
struct _RendererShape {
  Renderer renderer;

  char *filename;

  xmlDocPtr doc;
  xmlNodePtr root;
  xmlNodePtr connection_root;
  xmlNsPtr svg_name_space;

  LineStyle saved_line_style;
  real dash_length;
  real dot_length;

  real linewidth;
  const char *linecap;
  const char *linejoin;
  char *linestyle; /* not const -- must free */

  real fontsize;
};

static RendererShape *new_shape_renderer(DiagramData *data, const char *filename);
static void destroy_shape_renderer(RendererShape *renderer);

static void begin_render(RendererShape *renderer, DiagramData *data);
static void end_render(RendererShape *renderer);
static void set_linewidth(RendererShape *renderer, real linewidth);
static void set_linecaps(RendererShape *renderer, LineCaps mode);
static void set_linejoin(RendererShape *renderer, LineJoin mode);
static void set_linestyle(RendererShape *renderer, LineStyle mode);
static void set_dashlength(RendererShape *renderer, real length);
static void set_fillstyle(RendererShape *renderer, FillStyle mode);
static void set_font(RendererShape *renderer, Font *font, real height);
static void draw_line(RendererShape *renderer, 
		      Point *start, Point *end, 
		      Color *line_colour);
static void draw_polyline(RendererShape *renderer, 
			  Point *points, int num_points, 
			  Color *line_colour);
static void draw_polygon(RendererShape *renderer, 
			 Point *points, int num_points, 
			 Color *line_colour);
static void fill_polygon(RendererShape *renderer, 
			 Point *points, int num_points, 
			 Color *line_colour);
static void draw_rect(RendererShape *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void fill_rect(RendererShape *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void draw_arc(RendererShape *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void fill_arc(RendererShape *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void draw_ellipse(RendererShape *renderer, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void fill_ellipse(RendererShape *renderer, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void draw_bezier(RendererShape *renderer, 
			BezPoint *points,
			int numpoints,
			Color *colour);
static void fill_bezier(RendererShape *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *colour);
static void draw_string(RendererShape *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *colour);
static void draw_image(RendererShape *renderer,
		  Point *point,
		  real width, real height,
		  DiaImage image);
static void add_connection_point(RendererShape *renderer, 
      Point *point);
static void add_rectangle_connection_points(RendererShape *renderer,
      Point *ul_corner, Point *lr_corner);
static void add_ellipse_connection_points(RendererShape *renderer,
      Point *center, real width, real height);      

static RenderOps ShapeRenderOps = {
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

static RendererShape *
new_shape_renderer(DiagramData *data, const char *filename)
{
  RendererShape *renderer;
  FILE *file;
  char *point;
  xmlNsPtr name_space;
  xmlNodePtr xml_node_ptr;
  gint i;
  gchar *png_filename;
 
  file = fopen(filename, "w");

  if (file==NULL) {
    message_error(_("Couldn't open: '%s' for writing.\n"), filename);
    return NULL;
  }
  fclose(file);

  renderer = g_new(RendererShape, 1);
  renderer->renderer.ops = &ShapeRenderOps;
  renderer->renderer.is_interactive = 0;
  renderer->renderer.interactive_ops = NULL;

  renderer->filename = g_strdup(filename);

  renderer->dash_length = 1.0;
  renderer->dot_length = 0.2;
  renderer->saved_line_style = LINESTYLE_SOLID;

  /* set up the root node */
  renderer->doc = xmlNewDoc("1.0");
  name_space = xmlNewGlobalNs(renderer->doc, "http://www.daa.com.au/~james/dia-shape-ns", NULL);
  renderer->root = xmlNewDocNode(renderer->doc, name_space, "shape", NULL);
  renderer->svg_name_space = xmlNewNs(renderer->root, "http://www.w3.org/2000/svg", "svg");
  renderer->doc->root = renderer->root;

  xmlNewChild(renderer->root, NULL, "name", g_basename(filename));
  point = strrchr(filename, '.');
  i = (int)(point-filename);
  point = g_strndup(filename, i);
  png_filename = g_strdup_printf("%s.png",point);
  g_free(point);
  xmlNewChild(renderer->root, NULL, "icon", g_basename(png_filename));
  g_free(png_filename);
  renderer->connection_root = xmlNewChild(renderer->root, NULL, "connections", NULL);
  xml_node_ptr = xmlNewChild(renderer->root, NULL, "aspectratio",NULL);
  xmlSetProp(xml_node_ptr, "type","fixed");
  renderer->root = xmlNewChild(renderer->root, renderer->svg_name_space, "svg", NULL);
    
  return renderer;
}

static void
begin_render(RendererShape *renderer, DiagramData *data)
{
  renderer->linewidth = 0;
  renderer->fontsize = 1.0;
  renderer->linecap = "butt";
  renderer->linejoin = "miter";
  renderer->linestyle = NULL;
}

static void
end_render(RendererShape *renderer)
{
  g_free(renderer->linestyle);

  xmlSetDocCompressMode(renderer->doc, 0);
  xmlSaveFile(renderer->filename, renderer->doc);
  g_free(renderer->filename);
  xmlFreeDoc(renderer->doc);
}

static void 
destroy_shape_renderer(RendererShape *renderer)
{
  g_free(renderer);
}

static void
add_connection_point(RendererShape *renderer, Point *point) {
  xmlNodePtr node;
  char buf[512];
  
  node = xmlNewChild(renderer->connection_root, NULL, "point", NULL);
  g_snprintf(buf, sizeof(buf), "%g", point->x);
  xmlSetProp(node, "x", buf);
  g_snprintf(buf, sizeof(buf), "%g", point->y);
  xmlSetProp(node, "y", buf);
}

static void
set_linewidth(RendererShape *renderer, real linewidth)
{  /* 0 == hairline **/

  if (linewidth == 0)
    renderer->linewidth = 0.001;
  else
    renderer->linewidth = linewidth;
}

static void
set_linecaps(RendererShape *renderer, LineCaps mode)
{
  switch(mode) {
  case LINECAPS_BUTT:
    renderer->linecap = "butt";
    break;
  case LINECAPS_ROUND:
    renderer->linecap = "round";
    break;
  case LINECAPS_PROJECTING:
    renderer->linecap = "square";
    break;
  default:
    renderer->linecap = "butt";
  }
}

static void
set_linejoin(RendererShape *renderer, LineJoin mode)
{
  switch(mode) {
  case LINEJOIN_MITER:
    renderer->linejoin = "miter";
    break;
  case LINEJOIN_ROUND:
    renderer->linejoin = "round";
    break;
  case LINEJOIN_BEVEL:
    renderer->linejoin = "bevel";
    break;
  default:
    renderer->linejoin = "miter";
  }
}

static void
set_linestyle(RendererShape *renderer, LineStyle mode)
{
  real hole_width;

  renderer->saved_line_style = mode;

  g_free(renderer->linestyle);
  switch(mode) {
  case LINESTYLE_SOLID:
    renderer->linestyle = NULL;
    break;
  case LINESTYLE_DASHED:
    renderer->linestyle = g_strdup_printf("%g", renderer->dash_length);
    break;
  case LINESTYLE_DASH_DOT:
    hole_width = (renderer->dash_length - renderer->dot_length) / 2.0;
    renderer->linestyle = g_strdup_printf("%g %g %g %g",
					  renderer->dash_length,
					  hole_width,
					  renderer->dot_length,
					  hole_width);
    break;
  case LINESTYLE_DASH_DOT_DOT:
    hole_width = (renderer->dash_length - 2.0*renderer->dot_length) / 3.0;
    renderer->linestyle = g_strdup_printf("%g %g %g %g %g %g",
					  renderer->dash_length,
					  hole_width,
					  renderer->dot_length,
					  hole_width,
					  renderer->dot_length,
					  hole_width );
    break;
  case LINESTYLE_DOTTED:
    renderer->linestyle = g_strdup_printf("%g", renderer->dot_length);
    break;
  default:
    renderer->linestyle = NULL;
  }
}

static void
set_dashlength(RendererShape *renderer, real length)
{  /* dot = 20% of len */
  if (length<0.001)
    length = 0.001;
  
  renderer->dash_length = length;
  renderer->dot_length = length*0.2;
  
  set_linestyle(renderer, renderer->saved_line_style);
}

static void
set_fillstyle(RendererShape *renderer, FillStyle mode)
{
  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    message_error("svg_renderer: Unsupported fill mode specified!\n");
  }
}

static void
set_font(RendererShape *renderer, Font *font, real height)
{
  renderer->fontsize = height;
  /* XXXX todo */
  /*
  fprintf(renderer->file, "/%s-latin1 ff %f scf sf\n",
	  font_get_psfontname(font), (double)height);
  */
}

/* the return value of this function should not be saved anywhere */
static gchar *
get_draw_style(RendererShape *renderer,
	       Color *colour)
{
  static GString *str = NULL;

  if (!str) str = g_string_new(NULL);
  g_string_truncate(str, 0);

  g_string_sprintf(str, "stroke-width: %g", renderer->linewidth);
  if (strcmp(renderer->linecap, "butt"))
    g_string_sprintfa(str, "; stroke-linecap: %s", renderer->linecap);
  if (strcmp(renderer->linejoin, "miter"))
    g_string_sprintfa(str, "; stroke-linejoin: %s", renderer->linejoin);
  if (renderer->linestyle)
    g_string_sprintfa(str, "; stroke-dasharray: %s", renderer->linestyle);

  if (colour)
    g_string_sprintfa(str, "; stroke: #%02x%02x%02x",
		      (int)ceil(255*colour->red), (int)ceil(255*colour->green),
		      (int)ceil(255*colour->blue));

  return str->str;
}
/* the return value of this function should not be saved anywhere */
static gchar *
get_fill_style(RendererShape *renderer,
	       Color *colour)
{
  static GString *str = NULL;

  if (!str) str = g_string_new(NULL);

  g_string_sprintf(str, "fill: #%02x%02x%02x",
		   (int)ceil(255*colour->red), (int)ceil(255*colour->green),
		   (int)ceil(255*colour->blue));

  return str->str;
}

static void
draw_line(RendererShape *renderer, 
	  Point *start, Point *end, 
	  Color *line_colour)
{
  xmlNodePtr node;
  char buf[512];
  Point center;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "line", NULL);

  xmlSetProp(node, "style", get_draw_style(renderer, line_colour));

  g_snprintf(buf, sizeof(buf), "%g", start->x);
  xmlSetProp(node, "x1", buf);
  g_snprintf(buf, sizeof(buf), "%g", start->y);
  xmlSetProp(node, "y1", buf);
  g_snprintf(buf, sizeof(buf), "%g", end->x);
  xmlSetProp(node, "x2", buf);
  g_snprintf(buf, sizeof(buf), "%g", end->y);
  xmlSetProp(node, "y2", buf);
  add_connection_point(renderer, start);
  add_connection_point(renderer, end);
  center.x = (start->x + end->x)/2;
  center.y = (start->y + end->y)/2;
  add_connection_point(renderer, &center);

}

static void
draw_polyline(RendererShape *renderer, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
  int i;
  xmlNodePtr node;
  GString *str;
  Point center;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "polyline", NULL);
  
  xmlSetProp(node, "style", get_draw_style(renderer, line_colour));

  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++) {
    g_string_sprintfa(str, "%g,%g ", points[i].x, points[i].y);
    add_connection_point(renderer, &points[i]);
  }
  xmlSetProp(node, "points", str->str);
  g_string_free(str, TRUE);
  for(i = 1; i < num_points; i++) {
    center.x = (points[i].x + points[i-1].x)/2;
    center.y = (points[i].y + points[i-1].y)/2;
    add_connection_point(renderer, &center);
  }
}

static void
draw_polygon(RendererShape *renderer, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
  int i;
  xmlNodePtr node;
  GString *str;
  Point center;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "polygon", NULL);
  
  xmlSetProp(node, "style", get_draw_style(renderer, line_colour));

  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++) {
    g_string_sprintfa(str, "%g,%g ", points[i].x, points[i].y);
    add_connection_point(renderer, &points[i]);
  }
  for(i = 1; i < num_points; i++) {
    center.x = (points[i].x + points[i-1].x)/2;
    center.y = (points[i].y + points[i-1].y)/2;
    add_connection_point(renderer, &center);
  }
  xmlSetProp(node, "points", str->str);
  g_string_free(str, TRUE);
}

static void
fill_polygon(RendererShape *renderer, 
	      Point *points, int num_points, 
	      Color *colour)
{
  int i;
  xmlNodePtr node;
  GString *str;
  Point center;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "polygon", NULL);
  
  xmlSetProp(node, "style", get_fill_style(renderer, colour));

  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++) {
    g_string_sprintfa(str, "%g,%g ", points[i].x, points[i].y);
    add_connection_point(renderer, &points[i]);
  }
  for(i = 1; i < num_points; i++) {
    center.x = (points[i].x + points[i-1].x)/2;
    center.y = (points[i].y + points[i-1].y)/2;
    add_connection_point(renderer, &center);
  }
  xmlSetProp(node, "points", str->str);
  g_string_free(str, TRUE);
}

static void
add_rectangle_connection_points(RendererShape *renderer,
    Point *ul_corner, Point *lr_corner) {
  Point connection;
  coord center_x, center_y;
 
  center_x = (ul_corner->x + lr_corner->x)/2;
  center_y = (ul_corner->y + lr_corner->y)/2;
    
  add_connection_point(renderer, ul_corner);
  add_connection_point(renderer, lr_corner);
  connection.x = ul_corner->x;
  connection.y = lr_corner->y;
  add_connection_point(renderer, &connection);
  connection.y = center_y;
  add_connection_point(renderer, &connection);
  
  connection.x = lr_corner->x;
  connection.y = ul_corner->y;
  add_connection_point(renderer, &connection);
  connection.y = center_y;
  add_connection_point(renderer, &connection);
  
  connection.x = center_x;
  connection.y = lr_corner->y;
  add_connection_point(renderer, &connection);
  connection.y = ul_corner->y;
  add_connection_point(renderer, &connection);
}   
    

static void
draw_rect(RendererShape *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour) {
  xmlNodePtr node;
  char buf[512];
  
  node = xmlNewChild(renderer->root, renderer->svg_name_space, "rect", NULL);

  xmlSetProp(node, "style", get_draw_style(renderer, colour));

  g_snprintf(buf, sizeof(buf), "%g", ul_corner->x);
  xmlSetProp(node, "x", buf);
  g_snprintf(buf, sizeof(buf), "%g", ul_corner->y);
  xmlSetProp(node, "y", buf);
  g_snprintf(buf, sizeof(buf), "%g", lr_corner->x - ul_corner->x);
  xmlSetProp(node, "width", buf);
  g_snprintf(buf, sizeof(buf), "%g", lr_corner->y - ul_corner->y);
  xmlSetProp(node, "height", buf);
  add_rectangle_connection_points(renderer, ul_corner, lr_corner);
}

static void
fill_rect(RendererShape *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
  xmlNodePtr node;
  char buf[512];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "rect", NULL);

  xmlSetProp(node, "style", get_fill_style(renderer, colour));

  g_snprintf(buf, sizeof(buf), "%g", ul_corner->x);
  xmlSetProp(node, "x", buf);
  g_snprintf(buf, sizeof(buf), "%g", ul_corner->y);
  xmlSetProp(node, "y", buf);
  g_snprintf(buf, sizeof(buf), "%g", lr_corner->x - ul_corner->x);
  xmlSetProp(node, "width", buf);
  g_snprintf(buf, sizeof(buf), "%g", lr_corner->y - ul_corner->y);
  xmlSetProp(node, "height", buf);
  add_rectangle_connection_points(renderer, ul_corner, lr_corner);
}

static void
draw_arc(RendererShape *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
  xmlNodePtr node;
  char buf[512];
  real rx = width / 2, ry = height / 2;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "path", NULL);
  
  xmlSetProp(node, "style", get_draw_style(renderer, colour));

  /* this path might be incorrect ... */
  g_snprintf(buf, sizeof(buf), "M %g,%g A %g,%g 0 %d 1 %g,%g",
	     center->x + rx*cos(angle1), center->y + ry * sin(angle1),
	     rx, ry, (angle2 - angle1 > 0),
	     center->x + rx*cos(angle2), center->y + ry * sin(angle2));

  xmlSetProp(node, "d", buf);
}

static void
fill_arc(RendererShape *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
  xmlNodePtr node;
  char buf[512];
  real rx = width / 2, ry = height / 2;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "path", NULL);
  
  xmlSetProp(node, "style", get_fill_style(renderer, colour));

  /* this path might be incorrect ... */
  g_snprintf(buf, sizeof(buf), "M %g,%g A %g,%g 0 %d 1 %g,%g L %g,%g z",
	     center->x + rx*cos(angle1), center->y + ry * sin(angle1),
	     rx, ry, (angle2 - angle1 > 0),
	     center->x + rx*cos(angle2), center->y + ry * sin(angle2),
	     center->x, center->y);

  xmlSetProp(node, "d", buf);
}

static void 
add_ellipse_connection_points(RendererShape *renderer,
      Point *center, real width, real height) {
  Point connection;
  
  connection.x = center->x;
  connection.y = center->y + height/2;
  add_connection_point(renderer, &connection);
  connection.y = center->y - height/2;
  add_connection_point(renderer, &connection);
  
  connection.y = center->y;
  connection.x = center->x - width/2;
  add_connection_point(renderer, &connection);
  connection.x = center->x + width/2;
  add_connection_point(renderer, &connection);
}

static void
draw_ellipse(RendererShape *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
  xmlNodePtr node;
  char buf[512];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "ellipse", NULL);

  xmlSetProp(node, "style", get_draw_style(renderer, colour));

  g_snprintf(buf, sizeof(buf), "%g", center->x);
  xmlSetProp(node, "cx", buf);
  g_snprintf(buf, sizeof(buf), "%g", center->y);
  xmlSetProp(node, "cy", buf);
  g_snprintf(buf, sizeof(buf), "%g", width / 2);
  xmlSetProp(node, "rx", buf);
  g_snprintf(buf, sizeof(buf), "%g", height / 2);
  xmlSetProp(node, "ry", buf);
  add_ellipse_connection_points(renderer, center, width, height);
}

static void
fill_ellipse(RendererShape *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
  xmlNodePtr node;
  char buf[512];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "ellipse", NULL);

  xmlSetProp(node, "style", get_fill_style(renderer, colour));

  g_snprintf(buf, sizeof(buf), "%g", center->x);
  xmlSetProp(node, "cx", buf);
  g_snprintf(buf, sizeof(buf), "%g", center->y);
  xmlSetProp(node, "cy", buf);
  g_snprintf(buf, sizeof(buf), "%g", width / 2);
  xmlSetProp(node, "rx", buf);
  g_snprintf(buf, sizeof(buf), "%g", height / 2);
  xmlSetProp(node, "ry", buf);
  add_ellipse_connection_points(renderer, center, width, height);
}

static void
draw_bezier(RendererShape *renderer, 
	    BezPoint *points,
	    int numpoints,
	    Color *colour)
{
  int i;
  xmlNodePtr node;
  GString *str;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "path", NULL);
  
  xmlSetProp(node, "style", get_draw_style(renderer, colour));

  str = g_string_new(NULL);

  if (points[0].type != BEZ_MOVE_TO)
    g_warning("first BezPoint must be a BEZ_MOVE_TO");

  g_string_sprintf(str, "M %g %g", (double)points[0].p1.x,
		   (double)points[0].p1.y);

  for (i = 1; i < numpoints; i++)
    switch (points[i].type) {
    case BEZ_MOVE_TO:
      g_warning("only first BezPoint can be a BEZ_MOVE_TO");
      break;
    case BEZ_LINE_TO:
      g_string_sprintfa(str, " L %g,%g",
			(double) points[i].p1.x, (double) points[i].p1.y);
      break;
    case BEZ_CURVE_TO:
      g_string_sprintfa(str, " C %g,%g %g,%g %g,%g",
			(double) points[i].p1.x, (double) points[i].p1.y,
			(double) points[i].p2.x, (double) points[i].p2.y,
			(double) points[i].p3.x, (double) points[i].p3.y );
      break;
    }
  xmlSetProp(node, "d", str->str);
  g_string_free(str, TRUE);
}

static void
fill_bezier(RendererShape *renderer, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *colour)
{
  int i;
  xmlNodePtr node;
  GString *str;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "path", NULL);
  
  xmlSetProp(node, "style", get_fill_style(renderer, colour));

  str = g_string_new(NULL);

  if (points[0].type != BEZ_MOVE_TO)
    g_warning("first BezPoint must be a BEZ_MOVE_TO");

  g_string_sprintf(str, "M %g %g", (double)points[0].p1.x,
		   (double)points[0].p1.y);
 
  for (i = 1; i < numpoints; i++)
    switch (points[i].type) {
    case BEZ_MOVE_TO:
      g_warning("only first BezPoint can be a BEZ_MOVE_TO");
      break;
    case BEZ_LINE_TO:
      g_string_sprintfa(str, " L %g,%g",
			(double) points[i].p1.x, (double) points[i].p1.y);
      break;
    case BEZ_CURVE_TO:
      g_string_sprintfa(str, " C %g,%g %g,%g %g,%g",
			(double) points[i].p1.x, (double) points[i].p1.y,
			(double) points[i].p2.x, (double) points[i].p2.y,
			(double) points[i].p3.x, (double) points[i].p3.y );
      break;
    }
  g_string_append(str, "z");
  xmlSetProp(node, "d", str->str);
  g_string_free(str, TRUE);
}

static void
draw_string(RendererShape *renderer,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *colour)
{
  CHAR *enc;
  xmlNodePtr node;
  char buf[512], *style, *tmp;
  real saved_width;

  enc = xmlEncodeEntitiesReentrant(renderer->doc, text);
  node = xmlNewChild(renderer->root, renderer->svg_name_space, "text", enc);
  free(enc);

  saved_width = renderer->linewidth;
  renderer->linewidth = 0.001;
  style = get_fill_style(renderer, colour);
  renderer->linewidth = saved_width;
  switch (alignment) {
  case ALIGN_LEFT:
    style = g_strconcat(style, "; text-align: left", NULL);
    break;
  case ALIGN_CENTER:
    style = g_strconcat(style, "; text-align: center", NULL);
    break;
  case ALIGN_RIGHT:
    style = g_strconcat(style, "; text-align: right", NULL);
    break;
  }
  tmp = g_strdup_printf("%s; font-size: %g", style, renderer->fontsize);
  g_free(style);
  style = tmp;

  /* have to do something about fonts here ... */

  xmlSetProp(node, "style", style);
  g_free(style);

  g_snprintf(buf, sizeof(buf), "%g", pos->x);
  xmlSetProp(node, "x", buf);
  g_snprintf(buf, sizeof(buf), "%g", pos->y);
  xmlSetProp(node, "y", buf);
}

static void
draw_image(RendererShape *renderer,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
  xmlNodePtr node;
  char buf[512];


  node = xmlNewChild(renderer->root, renderer->svg_name_space, "image", NULL);

  g_snprintf(buf, sizeof(buf), "%g", point->x);
  xmlSetProp(node, "x", buf);
  g_snprintf(buf, sizeof(buf), "%g", point->y);
  xmlSetProp(node, "y", buf);
  g_snprintf(buf, sizeof(buf), "%g", width);
  xmlSetProp(node, "width", buf);
  g_snprintf(buf, sizeof(buf), "%g", height);
  xmlSetProp(node, "height", buf);
  xmlSetProp(node, "xlink:href", dia_image_filename(image));
}

static void
export_shape(DiagramData *data, const gchar *filename, 
             const gchar *diafilename, void* user_data)
{
    RendererShape *renderer;
    int i;
    gchar *point;
    gchar *png_filename = NULL;
    DiaExportFilter *exportfilter;
    char *old_locale;
    gfloat old_scaling;
    Rectangle *ext = &data->extents;
    gfloat scaling_x, scaling_y;

    /* create the png preview shown in the toolbox */
    point = strrchr(filename, '.');
    i = (int)(point-filename);
    point = g_strndup(filename, i);
    png_filename = g_strdup_printf("%s.png",point);
    g_free(point);
    exportfilter = filter_guess_export_filter(png_filename);

    if (!exportfilter) {
      message_warning(_("Can't export png without libart!"));
    }
    else {
      /* get the scaling right */
      old_scaling = data->paper.scaling;
      scaling_x = 22/((ext->right - ext->left) * 20);
      scaling_y = 22/((ext->bottom - ext->top) * 20);
      data->paper.scaling = MIN(scaling_x, scaling_y);
      exportfilter->export(data, png_filename, diafilename, user_data);
      data->paper.scaling = old_scaling;
    }
    /* create the shape */
    old_locale = setlocale(LC_NUMERIC, "C");
    if((renderer = new_shape_renderer(data, filename))) {
      data_render(data, (Renderer *)renderer, NULL, NULL, NULL);
      destroy_shape_renderer(renderer);
    }
    setlocale(LC_NUMERIC, old_locale);
    g_free(png_filename);
}

static const gchar *extensions[] = { "shape", NULL };
DiaExportFilter shape_export_filter = {
    N_("dia shape file"),
    extensions,
    export_shape
};
