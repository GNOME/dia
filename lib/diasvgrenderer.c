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
#include <locale.h>

#include <libxml/entities.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>

#include "geometry.h"
#include "intl.h"
#include "message.h"
#include "dia_xml_libxml.h"
#include "dia_image.h"

#include "diasvgrenderer.h"

/* DiaSvgRenderer methods */
static void
begin_render(DiaRenderer *self)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);

  renderer->linewidth = 0;
  renderer->fontsize = 1.0;
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
  char *old_locale;

  renderer->saved_line_style = mode;

  old_locale = setlocale(LC_NUMERIC, "C");
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
  setlocale(LC_NUMERIC, old_locale);
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
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);

  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    message_error("svg_renderer: Unsupported fill mode specified!\n");
  }
}

static void
set_font(DiaRenderer *self, DiaFont *font, real height)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);

  renderer->fontsize = height;
  if (renderer->font) 
    dia_font_unref(renderer->font);  
  renderer->font = dia_font_ref(font);
}

/* the return value of this function should not be saved anywhere */
static const gchar *
get_draw_style(DiaSvgRenderer *renderer,
	       Color *colour)
{
  static GString *str = NULL;
  char *old_locale;

  if (!str) str = g_string_new(NULL);
  g_string_truncate(str, 0);

  /* TODO(CHECK): the shape-export didn't have 'fill: none' here */
  old_locale = setlocale(LC_NUMERIC, "C");
  g_string_sprintf(str, "fill: none; fill-opacity:0; stroke-width: %g", renderer->linewidth);
  setlocale(LC_NUMERIC, old_locale);
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
static const gchar *
get_fill_style(DiaSvgRenderer *renderer,
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
draw_line(DiaRenderer *self, 
	  Point *start, Point *end, 
	  Color *line_colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  char buf[512];
  char *old_locale;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "line", NULL);

  xmlSetProp(node, "style", get_draw_style(renderer, line_colour));

  old_locale = setlocale(LC_NUMERIC, "C");
  g_snprintf(buf, sizeof(buf), "%g", start->x);
  xmlSetProp(node, "x1", buf);
  g_snprintf(buf, sizeof(buf), "%g", start->y);
  xmlSetProp(node, "y1", buf);
  g_snprintf(buf, sizeof(buf), "%g", end->x);
  xmlSetProp(node, "x2", buf);
  g_snprintf(buf, sizeof(buf), "%g", end->y);
  xmlSetProp(node, "y2", buf);
  setlocale(LC_NUMERIC, old_locale);
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
  char *old_locale;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "polyline", NULL);
  
  xmlSetProp(node, "style", get_draw_style(renderer, line_colour));

  old_locale = setlocale(LC_NUMERIC, "C");
  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++)
    g_string_sprintfa(str, "%g,%g ", points[i].x, points[i].y);
  xmlSetProp(node, "points", str->str);
  g_string_free(str, TRUE);
  setlocale(LC_NUMERIC, old_locale);
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
  char *old_locale;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "polygon", NULL);
  
  xmlSetProp(node, "style", get_draw_style(renderer, line_colour));

  old_locale = setlocale(LC_NUMERIC, "C");
  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++)
    g_string_sprintfa(str, "%g,%g ", points[i].x, points[i].y);
  xmlSetProp(node, "points", str->str);
  g_string_free(str, TRUE);
  setlocale(LC_NUMERIC, old_locale);
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
  char *old_locale;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "polygon", NULL);
  
  xmlSetProp(node, "style", get_fill_style(renderer, colour));

  old_locale = setlocale(LC_NUMERIC, "C");
  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++)
    g_string_sprintfa(str, "%g,%g ", points[i].x, points[i].y);
  xmlSetProp(node, "points", str->str);
  g_string_free(str, TRUE);
  setlocale(LC_NUMERIC, old_locale);
}

static void
draw_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  char buf[512];
  char *old_locale;

  node = xmlNewChild(renderer->root, NULL, "rect", NULL);

  xmlSetProp(node, "style", get_draw_style(renderer, colour));

  old_locale = setlocale(LC_NUMERIC, "C");
  g_snprintf(buf, sizeof(buf), "%g", ul_corner->x);
  xmlSetProp(node, "x", buf);
  g_snprintf(buf, sizeof(buf), "%g", ul_corner->y);
  xmlSetProp(node, "y", buf);
  g_snprintf(buf, sizeof(buf), "%g", lr_corner->x - ul_corner->x);
  xmlSetProp(node, "width", buf);
  g_snprintf(buf, sizeof(buf), "%g", lr_corner->y - ul_corner->y);
  xmlSetProp(node, "height", buf);
  setlocale(LC_NUMERIC, old_locale);
}

static void
fill_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  char buf[512];
  char *old_locale;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "rect", NULL);

  xmlSetProp(node, "style", get_fill_style(renderer, colour));

  old_locale = setlocale(LC_NUMERIC, "C");
  g_snprintf(buf, sizeof(buf), "%g", ul_corner->x);
  xmlSetProp(node, "x", buf);
  g_snprintf(buf, sizeof(buf), "%g", ul_corner->y);
  xmlSetProp(node, "y", buf);
  g_snprintf(buf, sizeof(buf), "%g", lr_corner->x - ul_corner->x);
  xmlSetProp(node, "width", buf);
  g_snprintf(buf, sizeof(buf), "%g", lr_corner->y - ul_corner->y);
  xmlSetProp(node, "height", buf);
  setlocale(LC_NUMERIC, old_locale);
}

static int sweep (real x1,real y1,real x2,real y2,real x3,real y3)
{
  real X2=x2-x1;
  real Y2=y2-y1;
  real X3=x3-x1;
  real Y3=y3-y1;
  real L=sqrt(X2*X2+Y2*Y2);
  real cost=X2/L;
  real sint=Y2/L;
  real res=-X3*sint+Y3*cost;
  return res>0;
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
  real x1=center->x + rx*cos(angle1*M_PI/180);
  real y1=center->y - ry*sin(angle1*M_PI/180);
  real x2=center->x + rx*cos(angle2*M_PI/180);
  real y2=center->y - ry*sin(angle2*M_PI/180);
  int swp = sweep(x1,y1,x2,y2,center->x,center->y);
  int l_arc;
  char *old_locale;

  if (angle2 > angle1) {
    l_arc = (angle2 - angle1) > 180;
  } else {
    l_arc = (360 - angle2 + angle1) > 180;
  }
  
  if (l_arc)
      swp = !swp;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "path", NULL);
  
  xmlSetProp(node, "style", get_draw_style(renderer, colour));

  /* this path might be incorrect ... */
  old_locale = setlocale(LC_NUMERIC, "C");
  g_snprintf(buf, sizeof(buf), "M %g,%g A %g,%g 0 %d %d %g,%g",
	     x1, y1,
	     rx, ry,l_arc ,swp ,
	     x2, y2);

  xmlSetProp(node, "d", buf);
  setlocale(LC_NUMERIC, old_locale);
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
  real x1=center->x + rx*cos(angle1*M_PI/180);
  real y1=center->y - ry*sin(angle1*M_PI/180);
  real x2=center->x + rx*cos(angle2*M_PI/180);
  real y2=center->y - ry*sin(angle2*M_PI/180);
  int swp = sweep(x1,y1,x2,y2,center->x,center->y);
  int l_arc;
  char *old_locale;

  if (angle2 > angle1) {
    l_arc = (angle2 - angle1) > 180;
  } else {
    l_arc = (360 - angle2 + angle1) > 180;
  }

  if (l_arc)
      swp = !swp;

  node = xmlNewChild(renderer->root, NULL, "path", NULL);
  
  xmlSetProp(node, "style", get_fill_style(renderer, colour));

  /* this path might be incorrect ... */
  old_locale = setlocale(LC_NUMERIC, "C");
  g_snprintf(buf, sizeof(buf), "M %g,%g A %g,%g 0 %d %d %g,%g L %g,%g z",
	     x1, y1,
	     rx, ry,l_arc ,swp ,
	     x2, y2,
	     center->x, center->y);

  xmlSetProp(node, "d", buf);
  setlocale(LC_NUMERIC, old_locale);
}

static void
draw_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  char buf[512];
  char *old_locale;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "ellipse", NULL);

  xmlSetProp(node, "style", get_draw_style(renderer, colour));

  old_locale = setlocale(LC_NUMERIC, "C");
  g_snprintf(buf, sizeof(buf), "%g", center->x);
  xmlSetProp(node, "cx", buf);
  g_snprintf(buf, sizeof(buf), "%g", center->y);
  xmlSetProp(node, "cy", buf);
  g_snprintf(buf, sizeof(buf), "%g", width / 2);
  xmlSetProp(node, "rx", buf);
  g_snprintf(buf, sizeof(buf), "%g", height / 2);
  xmlSetProp(node, "ry", buf);
  setlocale(LC_NUMERIC, old_locale);
}

static void
fill_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  char buf[512];
  char *old_locale;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "ellipse", NULL);

  xmlSetProp(node, "style", get_fill_style(renderer, colour));

  old_locale = setlocale(LC_NUMERIC, "C");
  g_snprintf(buf, sizeof(buf), "%g", center->x);
  xmlSetProp(node, "cx", buf);
  g_snprintf(buf, sizeof(buf), "%g", center->y);
  xmlSetProp(node, "cy", buf);
  g_snprintf(buf, sizeof(buf), "%g", width / 2);
  xmlSetProp(node, "rx", buf);
  g_snprintf(buf, sizeof(buf), "%g", height / 2);
  xmlSetProp(node, "ry", buf);
  setlocale(LC_NUMERIC, old_locale);
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
  char *old_locale;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "path", NULL);
  
  xmlSetProp(node, "style", get_draw_style(renderer, colour));

  str = g_string_new(NULL);

  old_locale = setlocale(LC_NUMERIC, "C");
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
  setlocale(LC_NUMERIC, old_locale);
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
  char *old_locale;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "path", NULL);
  
  xmlSetProp(node, "style", get_fill_style(renderer, colour));

  str = g_string_new(NULL);

  old_locale = setlocale(LC_NUMERIC, "C");
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
  setlocale(LC_NUMERIC, old_locale);
}

static void
draw_string(DiaRenderer *self,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *colour)
{    
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  char buf[512];
  char *style, *tmp;
  real saved_width;
  char *old_locale;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "text", text);
 
  saved_width = renderer->linewidth;
  renderer->linewidth = 0.001;
  style = (char*)get_fill_style(renderer, colour);
  /* return value must not be freed */
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
  old_locale = setlocale(LC_NUMERIC, "C");
  tmp = g_strdup_printf("%s; font-size: %g", style, renderer->fontsize);
  setlocale(LC_NUMERIC, old_locale);
  g_free (style);
  style = tmp;

  if (renderer->font) {
     tmp = g_strdup_printf("%s; font-family: %s; font-style: %s; "
                           "font-weight: %s",style,
                           dia_font_get_family(renderer->font),
                           dia_font_get_slant_string(renderer->font),
                           dia_font_get_weight_string(renderer->font));
     g_free(style);
     style = tmp;
  }

  /* have to do something about fonts here ... */

  xmlSetProp(node, "style", style);
  g_free(style);

  old_locale = setlocale(LC_NUMERIC, "C");
  g_snprintf(buf, sizeof(buf), "%g", pos->x);
  xmlSetProp(node, "x", buf);
  g_snprintf(buf, sizeof(buf), "%g", pos->y);
  xmlSetProp(node, "y", buf);
  setlocale(LC_NUMERIC, old_locale);
}

static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  char buf[512];
  char *old_locale;

  node = xmlNewChild(renderer->root, NULL, "image", NULL);

  old_locale = setlocale(LC_NUMERIC, "C");
  g_snprintf(buf, sizeof(buf), "%g", point->x);
  xmlSetProp(node, "x", buf);
  g_snprintf(buf, sizeof(buf), "%g", point->y);
  xmlSetProp(node, "y", buf);
  g_snprintf(buf, sizeof(buf), "%g", width);
  xmlSetProp(node, "width", buf);
  g_snprintf(buf, sizeof(buf), "%g", height);
  xmlSetProp(node, "height", buf);
  xmlSetProp(node, "xlink:href", dia_image_filename(image));
  setlocale(LC_NUMERIC, old_locale);
}

/* constructor */
static void
dia_svg_renderer_init (GTypeInstance   *instance, gpointer g_class)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (instance);

  
}

static gpointer parent_class = NULL;

/* destructor */
static void
dia_svg_renderer_finalize (GObject *object)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (object);

  if (renderer->font) 
    dia_font_unref(renderer->font);  

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

  /* svg specific */
  svg_renderer_class->get_draw_style = get_draw_style;
  svg_renderer_class->get_fill_style = get_fill_style;
}

