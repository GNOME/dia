/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * render_svg.c - an SVG renderer for dia, based on render_eps.c
 * Copyright (C) 1999, 2000 James Henstridge
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
#include <config.h>

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

#include <tree.h>
#include "geometry.h"
#include "render.h"
#include "filter.h"
#include "intl.h"
#include "message.h"
#include "diagramdata.h"

typedef struct _RendererSVG RendererSVG;
struct _RendererSVG {
  Renderer renderer;

  char *filename;

  xmlDocPtr doc;
  xmlNodePtr root;

  LineStyle saved_line_style;
  real dash_length;
  real dot_length;

  real linewidth;
  const char *linecap;
  const char *linejoin;
  char *linestyle; /* not const -- must free */

  real fontsize;
};

static RendererSVG *new_svg_renderer(DiagramData *data, const char *filename);
static void destroy_svg_renderer(RendererSVG *renderer);

static void begin_render(RendererSVG *renderer, DiagramData *data);
static void end_render(RendererSVG *renderer);
static void set_linewidth(RendererSVG *renderer, real linewidth);
static void set_linecaps(RendererSVG *renderer, LineCaps mode);
static void set_linejoin(RendererSVG *renderer, LineJoin mode);
static void set_linestyle(RendererSVG *renderer, LineStyle mode);
static void set_dashlength(RendererSVG *renderer, real length);
static void set_fillstyle(RendererSVG *renderer, FillStyle mode);
static void set_font(RendererSVG *renderer, Font *font, real height);
static void draw_line(RendererSVG *renderer, 
		      Point *start, Point *end, 
		      Color *line_colour);
static void draw_polyline(RendererSVG *renderer, 
			  Point *points, int num_points, 
			  Color *line_colour);
static void draw_polygon(RendererSVG *renderer, 
			 Point *points, int num_points, 
			 Color *line_colour);
static void fill_polygon(RendererSVG *renderer, 
			 Point *points, int num_points, 
			 Color *line_colour);
static void draw_rect(RendererSVG *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void fill_rect(RendererSVG *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void draw_arc(RendererSVG *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void fill_arc(RendererSVG *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void draw_ellipse(RendererSVG *renderer, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void fill_ellipse(RendererSVG *renderer, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void draw_bezier(RendererSVG *renderer, 
			BezPoint *points,
			int numpoints,
			Color *colour);
static void fill_bezier(RendererSVG *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *colour);
static void draw_string(RendererSVG *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *colour);
static void draw_image(RendererSVG *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);

static RenderOps SvgRenderOps = {
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

static RendererSVG *
new_svg_renderer(DiagramData *data, const char *filename)
{
  RendererSVG *renderer;
  FILE *file;
  gchar buf[512];
  time_t time_now;
  Rectangle *extent;
  char *name;
 
  file = fopen(filename, "w");

  if (file==NULL) {
    message_error(_("Couldn't open: '%s' for writing.\n"), filename);
    return NULL;
  }
  fclose(file);

  renderer = g_new(RendererSVG, 1);
  renderer->renderer.ops = &SvgRenderOps;
  renderer->renderer.is_interactive = 0;
  renderer->renderer.interactive_ops = NULL;

  renderer->filename = g_strdup(filename);

  renderer->dash_length = 1.0;
  renderer->dot_length = 0.2;
  renderer->saved_line_style = LINESTYLE_SOLID;

  /* set up the root node */
  renderer->doc = xmlNewDoc("1.0");
  renderer->doc->standalone = FALSE;
  xmlCreateIntSubset(renderer->doc, "svg",
		     "-//W3C//DTD SVG 20000802//EN",
		     "http://www.w3.org/TR/2000/CR-SVG-20000802/DTD/svg-20000802.dtd");
  renderer->root = xmlNewDocNode(renderer->doc, NULL, "svg", NULL);
  renderer->doc->root = renderer->root;

  /* set the extents of the SVG document */
  extent = &data->extents;
  g_snprintf(buf, sizeof(buf), "%dcm",
	     (int)ceil((extent->right - extent->left)));
  xmlSetProp(renderer->root, "width", buf);
  g_snprintf(buf, sizeof(buf), "%dcm",
	     (int)ceil((extent->bottom - extent->top)));
  xmlSetProp(renderer->root, "height", buf);
  g_snprintf(buf, sizeof(buf), "%d %d %d %d",
	     (int)floor(extent->left), (int)floor(extent->top),
	     (int)ceil(extent->right - extent->left),
	     (int)ceil(extent->bottom - extent->top));
  xmlSetProp(renderer->root, "viewBox", buf);
  
  time_now = time(NULL);
  name = getlogin();
  if (name==NULL)
    name = "a user";

#if 0
  /* some comments at the top of the file ... */
  xmlAddChild(renderer->root, xmlNewText("\n"));
  xmlAddChild(renderer->root, xmlNewComment("Dia-Version: "VERSION));
  xmlAddChild(renderer->root, xmlNewText("\n"));
  g_snprintf(buf, sizeof(buf), "File: %s", dia->filename);
  xmlAddChild(renderer->root, xmlNewComment(buf));
  xmlAddChild(renderer->root, xmlNewText("\n"));
  g_snprintf(buf, sizeof(buf), "Date: %s", ctime(&time_now));
  buf[strlen(buf)-1] = '\0'; /* remove the trailing new line */
  xmlAddChild(renderer->root, xmlNewComment(buf));
  xmlAddChild(renderer->root, xmlNewText("\n"));
  g_snprintf(buf, sizeof(buf), "For: %s", name);
  xmlAddChild(renderer->root, xmlNewComment(buf));
  xmlAddChild(renderer->root, xmlNewText("\n\n"));

  xmlNewChild(renderer->root, NULL, "title", dia->filename);
#endif
  
  return renderer;
}

static void
begin_render(RendererSVG *renderer, DiagramData *data)
{
  renderer->linewidth = 0;
  renderer->fontsize = 1.0;
  renderer->linecap = "butt";
  renderer->linejoin = "miter";
  renderer->linestyle = NULL;
}

static void
end_render(RendererSVG *renderer)
{
  g_free(renderer->linestyle);

  xmlSetDocCompressMode(renderer->doc, 0);
  xmlSaveFile(renderer->filename, renderer->doc);
  g_free(renderer->filename);
  xmlFreeDoc(renderer->doc);
}

static void destroy_svg_renderer(RendererSVG *renderer)
{
  g_free(renderer);
}

static void
set_linewidth(RendererSVG *renderer, real linewidth)
{  /* 0 == hairline **/

  if (linewidth == 0)
    renderer->linewidth = 0.001;
  else
    renderer->linewidth = linewidth;
}

static void
set_linecaps(RendererSVG *renderer, LineCaps mode)
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
set_linejoin(RendererSVG *renderer, LineJoin mode)
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
set_linestyle(RendererSVG *renderer, LineStyle mode)
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
set_dashlength(RendererSVG *renderer, real length)
{  /* dot = 20% of len */
  if (length<0.001)
    length = 0.001;
  
  renderer->dash_length = length;
  renderer->dot_length = length*0.2;
  
  set_linestyle(renderer, renderer->saved_line_style);
}

static void
set_fillstyle(RendererSVG *renderer, FillStyle mode)
{
  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    message_error("svg_renderer: Unsupported fill mode specified!\n");
  }
}

static void
set_font(RendererSVG *renderer, Font *font, real height)
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
get_draw_style(RendererSVG *renderer,
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
get_fill_style(RendererSVG *renderer,
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
draw_line(RendererSVG *renderer, 
	  Point *start, Point *end, 
	  Color *line_colour)
{
  xmlNodePtr node;
  char buf[512];

  node = xmlNewChild(renderer->root, NULL, "line", NULL);

  xmlSetProp(node, "style", get_draw_style(renderer, line_colour));

  g_snprintf(buf, sizeof(buf), "%g", start->x);
  xmlSetProp(node, "x1", buf);
  g_snprintf(buf, sizeof(buf), "%g", start->y);
  xmlSetProp(node, "y1", buf);
  g_snprintf(buf, sizeof(buf), "%g", end->x);
  xmlSetProp(node, "x2", buf);
  g_snprintf(buf, sizeof(buf), "%g", end->y);
  xmlSetProp(node, "y2", buf);
}

static void
draw_polyline(RendererSVG *renderer, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
  int i;
  xmlNodePtr node;
  GString *str;

  node = xmlNewChild(renderer->root, NULL, "polyline", NULL);
  
  xmlSetProp(node, "style", get_draw_style(renderer, line_colour));

  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++)
    g_string_sprintfa(str, "%g,%g ", points[i].x, points[i].y);
  xmlSetProp(node, "points", str->str);
  g_string_free(str, TRUE);
}

static void
draw_polygon(RendererSVG *renderer, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
  int i;
  xmlNodePtr node;
  GString *str;

  node = xmlNewChild(renderer->root, NULL, "polygon", NULL);
  
  xmlSetProp(node, "style", get_draw_style(renderer, line_colour));

  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++)
    g_string_sprintfa(str, "%g,%g ", points[i].x, points[i].y);
  xmlSetProp(node, "points", str->str);
  g_string_free(str, TRUE);
}

static void
fill_polygon(RendererSVG *renderer, 
	      Point *points, int num_points, 
	      Color *colour)
{
  int i;
  xmlNodePtr node;
  GString *str;

  node = xmlNewChild(renderer->root, NULL, "polygon", NULL);
  
  xmlSetProp(node, "style", get_fill_style(renderer, colour));

  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++)
    g_string_sprintfa(str, "%g,%g ", points[i].x, points[i].y);
  xmlSetProp(node, "points", str->str);
  g_string_free(str, TRUE);
}

static void
draw_rect(RendererSVG *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
  xmlNodePtr node;
  char buf[512];

  node = xmlNewChild(renderer->root, NULL, "rect", NULL);

  xmlSetProp(node, "style", get_draw_style(renderer, colour));

  g_snprintf(buf, sizeof(buf), "%g", ul_corner->x);
  xmlSetProp(node, "x", buf);
  g_snprintf(buf, sizeof(buf), "%g", ul_corner->y);
  xmlSetProp(node, "y", buf);
  g_snprintf(buf, sizeof(buf), "%g", lr_corner->x - ul_corner->x);
  xmlSetProp(node, "width", buf);
  g_snprintf(buf, sizeof(buf), "%g", lr_corner->y - ul_corner->y);
  xmlSetProp(node, "height", buf);
}

static void
fill_rect(RendererSVG *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
  xmlNodePtr node;
  char buf[512];

  node = xmlNewChild(renderer->root, NULL, "rect", NULL);

  xmlSetProp(node, "style", get_fill_style(renderer, colour));

  g_snprintf(buf, sizeof(buf), "%g", ul_corner->x);
  xmlSetProp(node, "x", buf);
  g_snprintf(buf, sizeof(buf), "%g", ul_corner->y);
  xmlSetProp(node, "y", buf);
  g_snprintf(buf, sizeof(buf), "%g", lr_corner->x - ul_corner->x);
  xmlSetProp(node, "width", buf);
  g_snprintf(buf, sizeof(buf), "%g", lr_corner->y - ul_corner->y);
  xmlSetProp(node, "height", buf);
}

static void
draw_arc(RendererSVG *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
  xmlNodePtr node;
  char buf[512];
  real rx = width / 2, ry = height / 2;

  node = xmlNewChild(renderer->root, NULL, "path", NULL);
  
  xmlSetProp(node, "style", get_draw_style(renderer, colour));

  /* this path might be incorrect ... */
  g_snprintf(buf, sizeof(buf), "M %g,%g A %g,%g 0 %d 1 %g,%g",
	     center->x + rx*cos(angle1), center->y + ry * sin(angle1),
	     rx, ry, (angle2 - angle1 > 0),
	     center->x + rx*cos(angle2), center->y + ry * sin(angle2));

  xmlSetProp(node, "d", buf);
}

static void
fill_arc(RendererSVG *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
  xmlNodePtr node;
  char buf[512];
  real rx = width / 2, ry = height / 2;

  node = xmlNewChild(renderer->root, NULL, "path", NULL);
  
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
draw_ellipse(RendererSVG *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
  xmlNodePtr node;
  char buf[512];

  node = xmlNewChild(renderer->root, NULL, "ellipse", NULL);

  xmlSetProp(node, "style", get_draw_style(renderer, colour));

  g_snprintf(buf, sizeof(buf), "%g", center->x);
  xmlSetProp(node, "cx", buf);
  g_snprintf(buf, sizeof(buf), "%g", center->y);
  xmlSetProp(node, "cy", buf);
  g_snprintf(buf, sizeof(buf), "%g", width / 2);
  xmlSetProp(node, "rx", buf);
  g_snprintf(buf, sizeof(buf), "%g", height / 2);
  xmlSetProp(node, "ry", buf);
}

static void
fill_ellipse(RendererSVG *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
  xmlNodePtr node;
  char buf[512];

  node = xmlNewChild(renderer->root, NULL, "ellipse", NULL);

  xmlSetProp(node, "style", get_fill_style(renderer, colour));

  g_snprintf(buf, sizeof(buf), "%g", center->x);
  xmlSetProp(node, "cx", buf);
  g_snprintf(buf, sizeof(buf), "%g", center->y);
  xmlSetProp(node, "cy", buf);
  g_snprintf(buf, sizeof(buf), "%g", width / 2);
  xmlSetProp(node, "rx", buf);
  g_snprintf(buf, sizeof(buf), "%g", height / 2);
  xmlSetProp(node, "ry", buf);
}

static void
draw_bezier(RendererSVG *renderer, 
	    BezPoint *points,
	    int numpoints,
	    Color *colour)
{
  int i;
  xmlNodePtr node;
  GString *str;

  node = xmlNewChild(renderer->root, NULL, "path", NULL);
  
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
fill_bezier(RendererSVG *renderer, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *colour)
{
  int i;
  xmlNodePtr node;
  GString *str;

  node = xmlNewChild(renderer->root, NULL, "path", NULL);
  
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
draw_string(RendererSVG *renderer,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *colour)
{
  CHAR *enc;
  xmlNodePtr node;
  char buf[512], *style, *tmp;
  real saved_width;

  enc = xmlEncodeEntitiesReentrant(renderer->doc, text);
  node = xmlNewChild(renderer->root, NULL, "text", enc);
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
draw_image(RendererSVG *renderer,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
  xmlNodePtr node;
  char buf[512];


  node = xmlNewChild(renderer->root, NULL, "image", NULL);

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
export_svg(DiagramData *data, const gchar *filename, 
           const gchar *diafilename, void* user_data)
{
  RendererSVG *renderer;
  char *old_locale;

  old_locale = setlocale(LC_NUMERIC, "C");
  if ((renderer = new_svg_renderer(data, filename))) {
    data_render(data, (Renderer *)renderer, NULL, NULL, NULL);
    destroy_svg_renderer(renderer);
  }
  setlocale(LC_NUMERIC, old_locale);
}

static const gchar *extensions[] = { "svg", NULL };
DiaExportFilter svg_export_filter = {
  N_("Scalable Vector Graphics"),
  extensions,
  export_svg
};
