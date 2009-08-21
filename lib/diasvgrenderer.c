/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * render_svg.c - an SVG renderer for dia, based on render_eps.c
 * Copyright (C) 1999, 2000 James Henstridge
 *
 * diasvgrenderer.c - refactoring of the above to serve as the
 *                    base class for plug-ins/svg/render_svg.c and
 *                    plug-ins/shape/shape-export.c
 *   Copyright (C) 2002, Hans Breuer
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
#include <glib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <libxml/entities.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>

#include "geometry.h"
#include "intl.h"
#include "message.h"
#include "dia_xml_libxml.h"
#include "dia_image.h"

#include "diasvgrenderer.h"
#include "textline.h"

#define DTOSTR_BUF_SIZE G_ASCII_DTOSTR_BUF_SIZE
#define dia_svg_dtostr(buf,d)                                  \
  g_ascii_formatd(buf,sizeof(buf),"%g",(d)*renderer->scale)
static void
draw_text_line(DiaRenderer *self, TextLine *text_line,
	       Point *pos, Alignment alignment, Color *colour);

/* DiaSvgRenderer methods */
static void
begin_render(DiaRenderer *self)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);

  renderer->linewidth = 0;
  renderer->linecap = "butt";
  renderer->linejoin = "miter";
  renderer->linestyle = NULL;
}

static void
end_render(DiaRenderer *self)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  g_free(renderer->linestyle);

  xmlSetDocCompressMode(renderer->doc, 0);
  xmlDiaSaveFile(renderer->filename, renderer->doc);
  g_free(renderer->filename);
  xmlFreeDoc(renderer->doc);
}

static void
set_linewidth(DiaRenderer *self, real linewidth)
{  /* 0 == hairline **/
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);

  if (linewidth == 0)
    renderer->linewidth = 0.001;
  else
    renderer->linewidth = linewidth;
}

static void
set_linecaps(DiaRenderer *self, LineCaps mode)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);

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
set_linejoin(DiaRenderer *self, LineJoin mode)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);

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
set_linestyle(DiaRenderer *self, LineStyle mode)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  real hole_width;
  gchar dash_length_buf[DTOSTR_BUF_SIZE];
  gchar dot_length_buf[DTOSTR_BUF_SIZE];
  gchar hole_width_buf[DTOSTR_BUF_SIZE];

  renderer->saved_line_style = mode;

  g_free(renderer->linestyle);
  switch(mode) {
  case LINESTYLE_SOLID:
    renderer->linestyle = NULL;
    break;
  case LINESTYLE_DASHED:
    dia_svg_dtostr(dash_length_buf, renderer->dash_length);
    renderer->linestyle = g_strdup_printf("%s", dash_length_buf);
    break;
  case LINESTYLE_DASH_DOT:
    hole_width = (renderer->dash_length - renderer->dot_length) / 2.0;

    dia_svg_dtostr(dash_length_buf, renderer->dash_length);
    dia_svg_dtostr(dot_length_buf, renderer->dot_length);
    dia_svg_dtostr(hole_width_buf, hole_width);

    renderer->linestyle = g_strdup_printf("%s %s %s %s",
					  dash_length_buf,
					  hole_width_buf,
					  dot_length_buf,
					  hole_width_buf );
    break;
  case LINESTYLE_DASH_DOT_DOT:
    hole_width = (renderer->dash_length - 2.0*renderer->dot_length) / 3.0;

    dia_svg_dtostr(dash_length_buf, renderer->dash_length);
    dia_svg_dtostr(dot_length_buf, renderer->dot_length);
    dia_svg_dtostr(hole_width_buf, hole_width);

    renderer->linestyle = g_strdup_printf("%s %s %s %s %s %s",
					  dash_length_buf,
					  hole_width_buf,
					  dot_length_buf,
					  hole_width_buf,
					  dot_length_buf,
					  hole_width_buf );
    break;
  case LINESTYLE_DOTTED:

    dia_svg_dtostr(dot_length_buf, renderer->dot_length);

    renderer->linestyle = g_strdup_printf("%s", dot_length_buf);
    break;
  default:
    renderer->linestyle = NULL;
  }
}

static void
set_dashlength(DiaRenderer *self, real length)
{  /* dot = 20% of len */
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);

  if (length<0.001)
    length = 0.001;
  
  renderer->dash_length = length;
  renderer->dot_length = length*0.2;
  
  set_linestyle(self, renderer->saved_line_style);
}

static void
set_fillstyle(DiaRenderer *self, FillStyle mode)
{
  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    message_error("svg_renderer: Unsupported fill mode specified!\n");
  }
}

/* the return value of this function should not be saved anywhere */
static const gchar *
get_draw_style(DiaSvgRenderer *renderer,
	       Color *colour)
{
  static GString *str = NULL;
  gchar linewidth_buf[DTOSTR_BUF_SIZE];
  gchar alpha_buf[DTOSTR_BUF_SIZE];

  if (!str) str = g_string_new(NULL);
  g_string_truncate(str, 0);

  /* TODO(CHECK): the shape-export didn't have 'fill: none' here */
  g_string_printf(str, "fill: none; stroke-opacity: %s; stroke-width: %s", 
		  g_ascii_formatd (alpha_buf, sizeof(alpha_buf), "%g", colour->alpha), 
		  dia_svg_dtostr(linewidth_buf, renderer->linewidth) );
  if (strcmp(renderer->linecap, "butt"))
    g_string_append_printf(str, "; stroke-linecap: %s", renderer->linecap);
  if (strcmp(renderer->linejoin, "miter"))
    g_string_append_printf(str, "; stroke-linejoin: %s", renderer->linejoin);
  if (renderer->linestyle)
    g_string_append_printf(str, "; stroke-dasharray: %s", renderer->linestyle);

  if (colour)
    g_string_append_printf(str, "; stroke: #%02x%02x%02x",
		      (int)ceil(255*colour->red), (int)ceil(255*colour->green),
		      (int)ceil(255*colour->blue));

  return str->str;
}

/* the return value of this function should not be saved anywhere */
static const gchar *
get_fill_style(DiaSvgRenderer *renderer,
	       Color *colour)
{
  static GString *str = NULL;
  gchar alpha_buf[DTOSTR_BUF_SIZE];

  if (!str) str = g_string_new(NULL);

  g_string_printf(str, "fill: #%02x%02x%02x; fill-opacity: %s",
		   (int)ceil(255*colour->red), (int)ceil(255*colour->green),
		   (int)ceil(255*colour->blue), 
		   g_ascii_formatd(alpha_buf, sizeof(alpha_buf), "%g", colour->alpha));

  return str->str;
}

static void
draw_line(DiaRenderer *self, 
	  Point *start, Point *end, 
	  Color *line_colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  gchar d_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"line", NULL);

  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_draw_style(renderer, line_colour));

  dia_svg_dtostr(d_buf, start->x);
  xmlSetProp(node, (const xmlChar *)"x1", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, start->y);
  xmlSetProp(node, (const xmlChar *)"y1", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, end->x);
  xmlSetProp(node, (const xmlChar *)"x2", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, end->y);
  xmlSetProp(node, (const xmlChar *)"y2", (xmlChar *) d_buf);
}

static void
draw_polyline(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  int i;
  xmlNodePtr node;
  GString *str;
  gchar px_buf[DTOSTR_BUF_SIZE];
  gchar py_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"polyline", NULL);
  
  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_draw_style(renderer, line_colour));

  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++)
    g_string_append_printf(str, "%s,%s ",
		      dia_svg_dtostr(px_buf, points[i].x),
		      dia_svg_dtostr(py_buf, points[i].y) );
  xmlSetProp(node, (const xmlChar *)"points", (xmlChar *) str->str);
  g_string_free(str, TRUE);
}

static void
draw_polygon(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  int i;
  xmlNodePtr node;
  GString *str;
  gchar px_buf[DTOSTR_BUF_SIZE];
  gchar py_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"polygon", NULL);
  
  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_draw_style(renderer, line_colour));

  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++)
    g_string_append_printf(str, "%s,%s ",
		      dia_svg_dtostr(px_buf, points[i].x),
		      dia_svg_dtostr(py_buf, points[i].y) );
  xmlSetProp(node, (const xmlChar *)"points", (xmlChar *) str->str);
  g_string_free(str, TRUE);
}

static void
fill_polygon(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  int i;
  xmlNodePtr node;
  GString *str;
  gchar px_buf[DTOSTR_BUF_SIZE];
  gchar py_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"polygon", NULL);
  
  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_fill_style(renderer, colour));

  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++)
    g_string_append_printf(str, "%s,%s ",
		      dia_svg_dtostr(px_buf, points[i].x),
		      dia_svg_dtostr(py_buf, points[i].y) );
  xmlSetProp(node, (const xmlChar *)"points", (xmlChar *)str->str);
  g_string_free(str, TRUE);
}

static void
draw_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  gchar d_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"rect", NULL);

  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *)get_draw_style(renderer, colour));

  dia_svg_dtostr(d_buf, ul_corner->x);
  xmlSetProp(node, (const xmlChar *)"x", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, ul_corner->y);
  xmlSetProp(node, (const xmlChar *)"y", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, lr_corner->x - ul_corner->x);
  xmlSetProp(node, (const xmlChar *)"width", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, lr_corner->y - ul_corner->y);
  xmlSetProp(node, (const xmlChar *)"height", (xmlChar *) d_buf);
}

static void
fill_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  gchar d_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"rect", NULL);

  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_fill_style(renderer, colour));

  dia_svg_dtostr(d_buf, ul_corner->x);
  xmlSetProp(node, (const xmlChar *)"x", (xmlChar *)d_buf);
  dia_svg_dtostr(d_buf, ul_corner->y);
  xmlSetProp(node, (const xmlChar *)"y", (xmlChar *)d_buf);
  dia_svg_dtostr(d_buf, lr_corner->x - ul_corner->x);
  xmlSetProp(node, (const xmlChar *)"width", (xmlChar *)d_buf);
  dia_svg_dtostr(d_buf, lr_corner->y - ul_corner->y);
  xmlSetProp(node, (const xmlChar *)"height", (xmlChar *)d_buf);
}

static void
draw_arc(DiaRenderer *self, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  char buf[512];
  real rx = width / 2, ry = height / 2;
  real sx=center->x + rx*cos(angle1*G_PI/180);
  real sy=center->y - ry*sin(angle1*G_PI/180);
  real ex=center->x + rx*cos(angle2*G_PI/180);
  real ey=center->y - ry*sin(angle2*G_PI/180);
  int swp = 0; /* always drawin negative direction */
  int large_arc = (angle2 - angle1 >= 180);
  gchar sx_buf[DTOSTR_BUF_SIZE];
  gchar sy_buf[DTOSTR_BUF_SIZE];
  gchar rx_buf[DTOSTR_BUF_SIZE];
  gchar ry_buf[DTOSTR_BUF_SIZE];
  gchar ex_buf[DTOSTR_BUF_SIZE];
  gchar ey_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"path", NULL);
  
  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_draw_style(renderer, colour));

  g_snprintf(buf, sizeof(buf), "M %s,%s A %s,%s 0 %d %d %s,%s",
	     dia_svg_dtostr(sx_buf, sx), dia_svg_dtostr(sy_buf, sy),
	     dia_svg_dtostr(rx_buf, rx), dia_svg_dtostr(ry_buf, ry),
	     large_arc, swp,
	     dia_svg_dtostr(ex_buf, ex), dia_svg_dtostr(ey_buf, ey) );

  xmlSetProp(node, (const xmlChar *)"d", (xmlChar *) buf);
}

static void
fill_arc(DiaRenderer *self, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  char buf[512];
  real rx = width / 2, ry = height / 2;
  real sx=center->x + rx*cos(angle1*G_PI/180);
  real sy=center->y - ry*sin(angle1*G_PI/180);
  real ex=center->x + rx*cos(angle2*G_PI/180);
  real ey=center->y - ry*sin(angle2*G_PI/180);
  int swp = 0; /* always drawin negative direction */
  int large_arc = (angle2 - angle1 >= 180);
  gchar sx_buf[DTOSTR_BUF_SIZE];
  gchar sy_buf[DTOSTR_BUF_SIZE];
  gchar rx_buf[DTOSTR_BUF_SIZE];
  gchar ry_buf[DTOSTR_BUF_SIZE];
  gchar ex_buf[DTOSTR_BUF_SIZE];
  gchar ey_buf[DTOSTR_BUF_SIZE];
  gchar cx_buf[DTOSTR_BUF_SIZE];
  gchar cy_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"path", NULL);
  
  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *)get_fill_style(renderer, colour));

  g_snprintf(buf, sizeof(buf), "M %s,%s A %s,%s 0 %d %d %s,%s L %s,%s z",
	     dia_svg_dtostr(sx_buf, sx), dia_svg_dtostr(sy_buf, sy),
	     dia_svg_dtostr(rx_buf, rx), dia_svg_dtostr(ry_buf, ry),
	     large_arc, swp,
	     dia_svg_dtostr(ex_buf, ex), dia_svg_dtostr(ey_buf, ey),
	     dia_svg_dtostr(cx_buf, center->x),
	     dia_svg_dtostr(cy_buf, center->y) );

  xmlSetProp(node, (const xmlChar *)"d", (xmlChar *) buf);
}

static void
draw_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  gchar d_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"ellipse", NULL);

  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_draw_style(renderer, colour));

  dia_svg_dtostr(d_buf, center->x);
  xmlSetProp(node, (const xmlChar *)"cx", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, center->y);
  xmlSetProp(node, (const xmlChar *)"cy", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, width / 2);
  xmlSetProp(node, (const xmlChar *)"rx", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, height / 2);
  xmlSetProp(node, (const xmlChar *)"ry", (xmlChar *) d_buf);
}

static void
fill_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  gchar d_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"ellipse", NULL);

  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_fill_style(renderer, colour));

  dia_svg_dtostr(d_buf, center->x);
  xmlSetProp(node, (const xmlChar *)"cx", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, center->y);
  xmlSetProp(node, (const xmlChar *)"cy", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, width / 2);
  xmlSetProp(node, (const xmlChar *)"rx", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, height / 2);
  xmlSetProp(node, (const xmlChar *)"ry", (xmlChar *) d_buf);
}

static void
draw_bezier(DiaRenderer *self, 
	    BezPoint *points,
	    int numpoints,
	    Color *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  int i;
  xmlNodePtr node;
  GString *str;
  gchar p1x_buf[DTOSTR_BUF_SIZE];
  gchar p1y_buf[DTOSTR_BUF_SIZE];
  gchar p2x_buf[DTOSTR_BUF_SIZE];
  gchar p2y_buf[DTOSTR_BUF_SIZE];
  gchar p3x_buf[DTOSTR_BUF_SIZE];
  gchar p3y_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"path", NULL);
  
  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_draw_style(renderer, colour));

  str = g_string_new(NULL);

  if (points[0].type != BEZ_MOVE_TO)
    g_warning("first BezPoint must be a BEZ_MOVE_TO");

  g_string_printf(str, "M %s %s",
		   dia_svg_dtostr(p1x_buf, (gdouble) points[0].p1.x),
		   dia_svg_dtostr(p1y_buf, (gdouble) points[0].p1.y) );

  for (i = 1; i < numpoints; i++)
    switch (points[i].type) {
    case BEZ_MOVE_TO:
      g_warning("only first BezPoint shoul be a BEZ_MOVE_TO");
      g_string_printf(str, "M %s %s",
		      dia_svg_dtostr(p1x_buf, (gdouble) points[i].p1.x),
		      dia_svg_dtostr(p1y_buf, (gdouble) points[i].p1.y) );
      break;
    case BEZ_LINE_TO:
      g_string_append_printf(str, " L %s,%s",
			dia_svg_dtostr(p1x_buf, (gdouble) points[i].p1.x),
			dia_svg_dtostr(p1y_buf, (gdouble) points[i].p1.y) );
      break;
    case BEZ_CURVE_TO:
      g_string_append_printf(str, " C %s,%s %s,%s %s,%s",
			dia_svg_dtostr(p1x_buf, (gdouble) points[i].p1.x),
			dia_svg_dtostr(p1y_buf, (gdouble) points[i].p1.y),
			dia_svg_dtostr(p2x_buf, (gdouble) points[i].p2.x),
			dia_svg_dtostr(p2y_buf, (gdouble) points[i].p2.y),
			dia_svg_dtostr(p3x_buf, (gdouble) points[i].p3.x),
			dia_svg_dtostr(p3y_buf, (gdouble) points[i].p3.y) );
      break;
    }
  xmlSetProp(node, (const xmlChar *)"d", (xmlChar *) str->str);
  g_string_free(str, TRUE);
}

static void
fill_bezier(DiaRenderer *self, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  int i;
  xmlNodePtr node;
  GString *str;
  gchar p1x_buf[DTOSTR_BUF_SIZE];
  gchar p1y_buf[DTOSTR_BUF_SIZE];
  gchar p2x_buf[DTOSTR_BUF_SIZE];
  gchar p2y_buf[DTOSTR_BUF_SIZE];
  gchar p3x_buf[DTOSTR_BUF_SIZE];
  gchar p3y_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"path", NULL);
  
  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_fill_style(renderer, colour));

  str = g_string_new(NULL);

  if (points[0].type != BEZ_MOVE_TO)
    g_warning("first BezPoint must be a BEZ_MOVE_TO");

  g_string_printf(str, "M %s %s",
		   dia_svg_dtostr(p1x_buf, (gdouble) points[0].p1.x),
		   dia_svg_dtostr(p1y_buf, (gdouble) points[0].p1.y) );
 
  for (i = 1; i < numpoints; i++)
    switch (points[i].type) {
    case BEZ_MOVE_TO:
      g_warning("only first BezPoint should be a BEZ_MOVE_TO");
      g_string_printf(str, "M %s %s",
		      dia_svg_dtostr(p1x_buf, (gdouble) points[i].p1.x),
		      dia_svg_dtostr(p1y_buf, (gdouble) points[i].p1.y) );
      break;
    case BEZ_LINE_TO:
      g_string_append_printf(str, " L %s,%s",
			dia_svg_dtostr(p1x_buf, (gdouble) points[i].p1.x),
			dia_svg_dtostr(p1y_buf, (gdouble) points[i].p1.y) );
      break;
    case BEZ_CURVE_TO:
      g_string_append_printf(str, " C %s,%s %s,%s %s,%s",
			dia_svg_dtostr(p1x_buf, (gdouble) points[i].p1.x),
			dia_svg_dtostr(p1y_buf, (gdouble) points[i].p1.y),
			dia_svg_dtostr(p2x_buf, (gdouble) points[i].p2.x),
			dia_svg_dtostr(p2y_buf, (gdouble) points[i].p2.y),
			dia_svg_dtostr(p3x_buf, (gdouble) points[i].p3.x),
			dia_svg_dtostr(p3y_buf, (gdouble) points[i].p3.y) );
      break;
    }
  g_string_append(str, "z");
  xmlSetProp(node, (const xmlChar *)"d", (xmlChar *) str->str);
  g_string_free(str, TRUE);
}

static void
draw_string(DiaRenderer *self,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *colour)
{    
  TextLine *text_line = text_line_new(text, self->font, self->font_height);
  draw_text_line(self, text_line, pos, alignment, colour);
  text_line_destroy(text_line);
}


static void
draw_text_line(DiaRenderer *self, TextLine *text_line,
	       Point *pos, Alignment alignment, Color *colour)
{    
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  char *style, *tmp;
  real saved_width;
  gchar d_buf[DTOSTR_BUF_SIZE];
  DiaFont *font;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"text", 
		     (xmlChar *) text_line_get_string(text_line));
 
  saved_width = renderer->linewidth;
  renderer->linewidth = 0.001;
  style = (char*)get_fill_style(renderer, colour);
  /* return value must not be freed */
  renderer->linewidth = saved_width;
  tmp = g_strdup_printf("%s; font-size: %s", style,
			dia_svg_dtostr(d_buf, text_line_get_height(text_line)));
  style = tmp;
  /* This is going to break for non-LTR texts, as SVG thinks 'start' is
   * 'right' for those. */
  switch (alignment) {
  case ALIGN_LEFT:
    tmp = g_strconcat(style, "; text-anchor:start", NULL);
    break;
  case ALIGN_CENTER:
    tmp = g_strconcat(style, "; text-anchor:middle", NULL);
    break;
  case ALIGN_RIGHT:
    tmp = g_strconcat(style, "; text-anchor:end", NULL);
    break;
  }
  g_free (style);
  style = tmp;

  font = text_line_get_font(text_line);
  tmp = g_strdup_printf("%s; font-family: %s; font-style: %s; "
			"font-weight: %s",style,
			dia_font_get_family(font),
			dia_font_get_slant_string(font),
			dia_font_get_weight_string(font));
  g_free(style);
  style = tmp;

  /* have to do something about fonts here ... */

  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) style);
  g_free(style);

  dia_svg_dtostr(d_buf, pos->x);
  xmlSetProp(node, (const xmlChar *)"x", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, pos->y);
  xmlSetProp(node, (const xmlChar *)"y", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, text_line_get_width(text_line));
  xmlSetProp(node, (const xmlChar*)"textLength", (xmlChar *) d_buf);
}

static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage *image)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  gchar d_buf[DTOSTR_BUF_SIZE];
  gchar *uri;

  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"image", NULL);

  dia_svg_dtostr(d_buf, point->x);
  xmlSetProp(node, (const xmlChar *)"x", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, point->y);
  xmlSetProp(node, (const xmlChar *)"y", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, width);
  xmlSetProp(node, (const xmlChar *)"width", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, height);
  xmlSetProp(node, (const xmlChar *)"height", (xmlChar *) d_buf);
  
  uri = g_filename_to_uri(dia_image_filename(image), NULL, NULL);
  if (uri)
    xmlSetProp(node, (const xmlChar *)"xlink:href", (xmlChar *) uri);
  else /* not sure if this fallbach is better than nothing */
    xmlSetProp(node, (const xmlChar *)"xlink:href", (xmlChar *) dia_image_filename(image));
  g_free (uri);
}

/* constructor */
static void
dia_svg_renderer_init (GTypeInstance *self, gpointer g_class)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  
  renderer->scale = 1.0;
  
}

static gpointer parent_class = NULL;

/* destructor */
static void
dia_svg_renderer_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* gobject boiler plate, vtable initialization  */
static void dia_svg_renderer_class_init (DiaSvgRendererClass *klass);

GType
dia_svg_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (DiaSvgRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) dia_svg_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (DiaSvgRenderer),
        0,              /* n_preallocs */
	dia_svg_renderer_init /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "DiaSvgRenderer",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
dia_svg_renderer_class_init (DiaSvgRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);
  DiaSvgRendererClass *svg_renderer_class = DIA_SVG_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = dia_svg_renderer_finalize;

  /* renderer members */
  renderer_class->begin_render = begin_render;
  renderer_class->end_render   = end_render;

  renderer_class->set_linewidth  = set_linewidth;
  renderer_class->set_linecaps   = set_linecaps;
  renderer_class->set_linejoin   = set_linejoin;
  renderer_class->set_linestyle  = set_linestyle;
  renderer_class->set_dashlength = set_dashlength;
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->draw_line    = draw_line;
  renderer_class->fill_polygon = fill_polygon;
  renderer_class->draw_rect    = draw_rect;
  renderer_class->fill_rect    = fill_rect;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;
  renderer_class->fill_ellipse = fill_ellipse;

  renderer_class->draw_string  = draw_string;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_rect = draw_rect;
  renderer_class->draw_polyline  = draw_polyline;
  renderer_class->draw_polygon   = draw_polygon;

  renderer_class->draw_bezier   = draw_bezier;
  renderer_class->fill_bezier   = fill_bezier;
  renderer_class->draw_text_line  = draw_text_line;

  /* svg specific */
  svg_renderer_class->get_draw_style = get_draw_style;
  svg_renderer_class->get_fill_style = get_fill_style;
}


