/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * render_svg.c - an SVG renderer for dia, based on render_eps.c
 * Copyright (C) 1999, 2000 James Henstridge
 *
 * diasvgrenderer.c - refactoring of the above to serve as the
 *                    base class for plug-ins/svg/render_svg.c and
 *                    plug-ins/shape/shape-export.c
 *   Copyright (C) 2002-2014 Hans Breuer
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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <glib.h>
#include <glib/gstdio.h> /* g_sprintf */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <libxml/entities.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>

#include "geometry.h"
#include "dia_image.h"
#include "dia_dirs.h"

#include "diasvgrenderer.h"
#include "textline.h"
#include "prop_pixbuf.h" /* pixbuf_encode_base64() */
#include "pattern.h"
#include "dia-io.h"


/**
 * DiaSvgRenderer:
 * Renders to an SVG 1.1 tree.
 *
 * Internally, Dia uses this for shape file creation and one of the SVG
 * exporters.
 */
G_DEFINE_TYPE (DiaSvgRenderer, dia_svg_renderer, DIA_TYPE_RENDERER)

#define DTOSTR_BUF_SIZE G_ASCII_DTOSTR_BUF_SIZE
#define dia_svg_dtostr(buf,d)                                  \
  g_ascii_formatd(buf,sizeof(buf),"%g",(d)*renderer->scale)


static void  draw_text_line      (DiaRenderer  *self,
                                  TextLine     *text_line,
                                  Point        *pos,
                                  DiaAlignment  alignment,
                                  Color        *colour);

/*!
 * \brief Initialize to SVG rendering defaults
 * \memberof _DiaSvgRenderer
 */
static void
begin_render(DiaRenderer *self, const DiaRectangle *update)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);

  renderer->linewidth = 0;
  renderer->linecap = "butt";
  renderer->linejoin = "miter";
  renderer->linestyle = NULL;
}

/*!
 * \brief Create a unique key to identify the pattern
 */
static gchar *
_make_pattern_key (const DiaPattern *pattern)
{
  gchar *key = g_strdup_printf ("%p", pattern);

  return key;
}

static gboolean
_color_stop_do (real         ofs,
		const Color *col,
		gpointer     user_data)
{
  xmlNodePtr parent = (xmlNodePtr)user_data;
  xmlNodePtr stop = xmlNewChild (parent, parent->ns, (const xmlChar *)"stop", NULL);
  gchar vbuf[DTOSTR_BUF_SIZE];

  g_ascii_formatd(vbuf,sizeof(vbuf),"%g", ofs);
  xmlSetProp (stop, (const xmlChar *)"offset", (const xmlChar *)vbuf);

  g_sprintf (vbuf, "#%02x%02x%02x", (int)(255*col->red), (int)(255*col->green), (int)(255*col->blue));
  xmlSetProp (stop, (const xmlChar *)"stop-color", (const xmlChar *)vbuf);

  g_ascii_formatd(vbuf,sizeof(vbuf),"%g", col->alpha);
  xmlSetProp (stop, (const xmlChar *)"stop-opacity", (const xmlChar *)vbuf);

  return TRUE;
}

typedef struct {
  DiaSvgRenderer *renderer;
  xmlNodePtr      node;
} GradientData;

static void
_gradient_do (gpointer key,
	      gpointer value,
	      gpointer user_data)
{
  DiaPattern *pattern = (DiaPattern *)value;
  GradientData *gd = (GradientData *)user_data;
  DiaSvgRenderer *renderer = gd->renderer;
  xmlNodePtr parent = gd->node;
  xmlNodePtr gradient;
  DiaPatternType pt;
  guint flags;
  Point p1, p2;
  gchar vbuf[DTOSTR_BUF_SIZE];
  real scale = renderer->scale;

  dia_pattern_get_settings (pattern, &pt, &flags);
  if ((flags & DIA_PATTERN_USER_SPACE)==0)
    scale = 1.0;
  dia_pattern_get_points (pattern, &p1, &p2);
  if (pt == DIA_LINEAR_GRADIENT) {
    gradient = xmlNewChild (parent, parent->ns, (const xmlChar *)"linearGradient", NULL);
    xmlSetProp (gradient, (const xmlChar *)"x1", (const xmlChar *)g_ascii_formatd(vbuf,sizeof(vbuf),"%g", p1.x * scale));
    xmlSetProp (gradient, (const xmlChar *)"y1", (const xmlChar *)g_ascii_formatd(vbuf,sizeof(vbuf),"%g", p1.y * scale));
    xmlSetProp (gradient, (const xmlChar *)"x2", (const xmlChar *)g_ascii_formatd(vbuf,sizeof(vbuf),"%g", p2.x * scale));
    xmlSetProp (gradient, (const xmlChar *)"y2", (const xmlChar *)g_ascii_formatd(vbuf,sizeof(vbuf),"%g", p2.y * scale));
  } else if  (pt == DIA_RADIAL_GRADIENT) {
    real r;
    dia_pattern_get_radius (pattern, &r);
    gradient = xmlNewChild (parent, parent->ns, (const xmlChar *)"radialGradient", NULL);
    xmlSetProp (gradient, (const xmlChar *)"cx", (const xmlChar *)g_ascii_formatd(vbuf,sizeof(vbuf),"%g", p1.x * scale));
    xmlSetProp (gradient, (const xmlChar *)"cy", (const xmlChar *)g_ascii_formatd(vbuf,sizeof(vbuf),"%g", p1.y * scale));
    xmlSetProp (gradient, (const xmlChar *)"fx", (const xmlChar *)g_ascii_formatd(vbuf,sizeof(vbuf),"%g", p2.x * scale));
    xmlSetProp (gradient, (const xmlChar *)"fy", (const xmlChar *)g_ascii_formatd(vbuf,sizeof(vbuf),"%g", p2.y * scale));
    xmlSetProp (gradient, (const xmlChar *)"r", (const xmlChar *)g_ascii_formatd(vbuf,sizeof(vbuf),"%g", r * scale));
  } else {
    gradient = xmlNewChild (parent, parent->ns, (const xmlChar *)"pattern", NULL);
  }
  /* don't miss to set the id */
  {
    char *id = _make_pattern_key (pattern);
    xmlSetProp (gradient, (const xmlChar *)"id", (const xmlChar *)id);
    g_clear_pointer (&id, g_free);
  }
  if (flags & DIA_PATTERN_USER_SPACE)
    xmlSetProp (gradient, (const xmlChar *)"gradientUnits", (const xmlChar *)"userSpaceOnUse");
  if (flags & DIA_PATTERN_EXTEND_REPEAT)
    xmlSetProp (gradient, (const xmlChar *)"spreadMethod", (const xmlChar *)"repeat");
  else if (flags & DIA_PATTERN_EXTEND_REFLECT)
    xmlSetProp (gradient, (const xmlChar *)"spreadMethod", (const xmlChar *)"reflect");
  else if (flags & DIA_PATTERN_EXTEND_PAD)
    xmlSetProp (gradient, (const xmlChar *)"spreadMethod", (const xmlChar *)"pad");

  if (pt == DIA_LINEAR_GRADIENT || pt == DIA_RADIAL_GRADIENT) {
    dia_pattern_foreach (pattern, _color_stop_do, gradient);
  } else {
    g_warning ("SVG pattern data not implemented");
  }
}


static void
end_render (DiaRenderer *self)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  DiaContext *ctx = dia_context_new (_("SVG Export"));

  g_clear_pointer (&renderer->linestyle, g_free);

  /* handle potential patterns */
  if (renderer->patterns) {
    xmlNodePtr root = xmlDocGetRootElement (renderer->doc);
    xmlNodePtr defs = xmlNewNode (renderer->svg_name_space, (const xmlChar *) "defs");
    GradientData gd = { renderer, defs };
    g_hash_table_foreach (renderer->patterns, _gradient_do, &gd);
    xmlAddPrevSibling (root->children, defs);
    g_hash_table_destroy (renderer->patterns);
    renderer->patterns = NULL;
  }

  dia_context_set_filename (ctx, renderer->filename);
  dia_io_save_document (renderer->filename, renderer->doc, FALSE, ctx);

  g_clear_pointer (&renderer->filename, g_free);
  g_clear_pointer (&renderer->doc, xmlFreeDoc);
  g_clear_pointer (&ctx, dia_context_release);
}


/*!
 * \brief Only basic capabilities for the base class
 * \memberof _DiaSvgRenderer
 */
static gboolean
is_capable_to (DiaRenderer *renderer, RenderCapability cap)
{
  if (RENDER_HOLES == cap)
    return FALSE; /* not wanted for shapes */
  else if (RENDER_ALPHA == cap)
    return TRUE; /* also for shapes */
  else if (RENDER_AFFINE == cap)
    return FALSE; /* not for shape renderer */
  else if (RENDER_PATTERN == cap)
    return FALSE; /* some support form derived class needed */
  return FALSE;
}

/*!
 * \brief Set line width
 * \memberof _DiaSvgRenderer
 */
static void
set_linewidth(DiaRenderer *self, real linewidth)
{  /* 0 == hairline **/
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);

  if (linewidth == 0)
    renderer->linewidth = 0.001;
  else
    renderer->linewidth = linewidth;
}


/*
 * Set line caps
 */
static void
set_linecaps (DiaRenderer *self, DiaLineCaps mode)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);

  switch (mode) {
    case DIA_LINE_CAPS_BUTT:
      renderer->linecap = "butt";
      break;
    case DIA_LINE_CAPS_ROUND:
      renderer->linecap = "round";
      break;
    case DIA_LINE_CAPS_PROJECTING:
      renderer->linecap = "square";
      break;
    case DIA_LINE_CAPS_DEFAULT:
    default:
      renderer->linecap = "butt";
  }
}


/*
 * Set line join
 */
static void
set_linejoin (DiaRenderer *self, DiaLineJoin mode)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);

  switch (mode) {
    case DIA_LINE_JOIN_MITER:
      renderer->linejoin = "miter";
      break;
    case DIA_LINE_JOIN_ROUND:
      renderer->linejoin = "round";
      break;
    case DIA_LINE_JOIN_BEVEL:
      renderer->linejoin = "bevel";
      break;
    case DIA_LINE_CAPS_DEFAULT:
    default:
      renderer->linejoin = "miter";
  }
}


/*
 * Set line style
 */
static void
set_linestyle (DiaRenderer *self, DiaLineStyle mode, double dash_length)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  double hole_width;
  char dash_length_buf[DTOSTR_BUF_SIZE];
  char dot_length_buf[DTOSTR_BUF_SIZE];
  char hole_width_buf[DTOSTR_BUF_SIZE];
  double dot_length; /* dot = 20% of len */

  if (dash_length < 0.001) {
    dash_length = 0.001;
  }
  dot_length = dash_length * 0.2;

  g_clear_pointer (&renderer->linestyle, g_free);
  switch (mode) {
    case DIA_LINE_STYLE_SOLID:
      renderer->linestyle = NULL;
      break;
    case DIA_LINE_STYLE_DASHED:
      dia_svg_dtostr (dash_length_buf, dash_length);
      renderer->linestyle = g_strdup_printf ("%s", dash_length_buf);
      break;
    case DIA_LINE_STYLE_DASH_DOT:
      hole_width = (dash_length - dot_length) / 2.0;

      dia_svg_dtostr (dash_length_buf, dash_length);
      dia_svg_dtostr (dot_length_buf, dot_length);
      dia_svg_dtostr (hole_width_buf, hole_width);

      renderer->linestyle = g_strdup_printf ("%s %s %s %s",
                                             dash_length_buf,
                                             hole_width_buf,
                                             dot_length_buf,
                                             hole_width_buf);
      break;
    case DIA_LINE_STYLE_DASH_DOT_DOT:
      hole_width = (dash_length - (2.0 * dot_length)) / 3.0;

      dia_svg_dtostr (dash_length_buf, dash_length);
      dia_svg_dtostr (dot_length_buf, dot_length);
      dia_svg_dtostr (hole_width_buf, hole_width);

      renderer->linestyle = g_strdup_printf ("%s %s %s %s %s %s",
                                             dash_length_buf,
                                             hole_width_buf,
                                             dot_length_buf,
                                             hole_width_buf,
                                             dot_length_buf,
                                             hole_width_buf);
      break;
    case DIA_LINE_STYLE_DOTTED:
      dia_svg_dtostr (dot_length_buf, dot_length);

      renderer->linestyle = g_strdup_printf ("%s", dot_length_buf);
      break;
    case DIA_LINE_STYLE_DEFAULT:
    default:
      renderer->linestyle = NULL;
  }
}


/*
 * Set fill style
 */
static void
set_fillstyle (DiaRenderer *self, DiaFillStyle mode)
{
  switch (mode) {
    case DIA_FILL_STYLE_SOLID:
      break;
    default:
      g_warning ("svg_renderer: Unsupported fill mode specified!\n");
  }
}


/*!
 * \brief Remember the pattern for later use
 * \memberof _DiaSvgRenderer
 */
static void
set_pattern(DiaRenderer *self, DiaPattern *pattern)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  DiaPattern *prev = renderer->active_pattern;

  if (!renderer->patterns)
    renderer->patterns = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  /* remember */
  if (pattern) {
    renderer->active_pattern = g_object_ref (pattern);
    if (!g_hash_table_lookup (renderer->patterns, pattern))
      g_hash_table_insert (renderer->patterns, _make_pattern_key (pattern), g_object_ref (pattern));
  } else {
    renderer->active_pattern = NULL;
  }

  g_clear_object (&prev);
}

/*!
 * \brief Get the draw style for given fill/stroke
 *
 * The return value of this function should not be saved anywhere
 *
 * \protected \memberof _DiaSvgRenderer
 */
static const gchar *
get_draw_style(DiaSvgRenderer *renderer,
	       Color *fill,
	       Color *stroke)
{
  static GString *str = NULL;
  gchar linewidth_buf[DTOSTR_BUF_SIZE];
  gchar alpha_buf[DTOSTR_BUF_SIZE];

  if (!str)
    str = g_string_new(NULL);
  g_string_truncate(str, 0);

  /* we only append a semicolon with the second attribute */
  if (fill) {
    if (renderer->active_pattern) {
      gchar *key = _make_pattern_key (renderer->active_pattern);
      g_string_printf(str, "fill:url(#%s)", key);
      g_clear_pointer (&key, g_free);
    } else {
      g_string_printf(str, "fill: #%02x%02x%02x; fill-opacity: %s",
		      (int)(255*fill->red), (int)(255*fill->green),
		      (int)(255*fill->blue),
		      g_ascii_formatd(alpha_buf, sizeof(alpha_buf), "%g", fill->alpha));
    }
  } else {
    g_string_printf(str, "fill: none");
  }

  if (stroke) {
    g_string_append_printf(str, "; stroke-opacity: %s; stroke-width: %s",
			   g_ascii_formatd (alpha_buf, sizeof(alpha_buf), "%g", stroke->alpha),
			   dia_svg_dtostr(linewidth_buf, renderer->linewidth) );
    if (strcmp(renderer->linecap, "butt"))
      g_string_append_printf(str, "; stroke-linecap: %s", renderer->linecap);
    if (strcmp(renderer->linejoin, "miter"))
      g_string_append_printf(str, "; stroke-linejoin: %s", renderer->linejoin);
    if (renderer->linestyle)
      g_string_append_printf(str, "; stroke-dasharray: %s", renderer->linestyle);

    g_string_append_printf(str, "; stroke: #%02x%02x%02x",
			   (int)(255*stroke->red),
			   (int)(255*stroke->green),
			   (int)(255*stroke->blue));
  } else {
    g_string_append_printf(str, "; stroke: none");
  }
  return str->str;
}

/*!
 * \brief Draw a single line element
 * \memberof _DiaSvgRenderer
 */
static void
draw_line(DiaRenderer *self,
	  Point *start, Point *end,
	  Color *line_colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  gchar d_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"line", NULL);

  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_draw_style(renderer, NULL, line_colour));

  dia_svg_dtostr(d_buf, start->x);
  xmlSetProp(node, (const xmlChar *)"x1", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, start->y);
  xmlSetProp(node, (const xmlChar *)"y1", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, end->x);
  xmlSetProp(node, (const xmlChar *)"x2", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, end->y);
  xmlSetProp(node, (const xmlChar *)"y2", (xmlChar *) d_buf);
}

/*!
 * \brief Draw a polyline element
 * \memberof _DiaSvgRenderer
 */
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

  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_draw_style(renderer, NULL, line_colour));

  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++)
    g_string_append_printf(str, "%s,%s ",
		      dia_svg_dtostr(px_buf, points[i].x),
		      dia_svg_dtostr(py_buf, points[i].y) );
  xmlSetProp(node, (const xmlChar *)"points", (xmlChar *) str->str);
  g_string_free(str, TRUE);
}

/*!
 * \brief Draw a polygon element (filled and/or stroked)
 * \memberof _DiaSvgRenderer
 */
static void
draw_polygon (DiaRenderer *self,
	      Point *points, int num_points,
	      Color *fill, Color *stroke)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  int i;
  xmlNodePtr node;
  GString *str;
  gchar px_buf[DTOSTR_BUF_SIZE];
  gchar py_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"polygon", NULL);

  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_draw_style (renderer, fill, stroke));

  if (fill)
    xmlSetProp(node, (const xmlChar *)"fill-rule", (const xmlChar *) "evenodd");

  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++)
    g_string_append_printf(str, "%s,%s ",
		      dia_svg_dtostr(px_buf, points[i].x),
		      dia_svg_dtostr(py_buf, points[i].y) );
  xmlSetProp(node, (const xmlChar *)"points", (xmlChar *) str->str);
  g_string_free(str, TRUE);
}

/*!
 * \brief Draw a rectangle element (filled and/or stroked)
 * \memberof _DiaSvgRenderer
 */
static void
draw_rect(DiaRenderer *self,
	  Point *ul_corner, Point *lr_corner,
	  Color *fill, Color *stroke)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  gchar d_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"rect", NULL);

  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_draw_style (renderer, fill, stroke));

  dia_svg_dtostr(d_buf, ul_corner->x);
  xmlSetProp(node, (const xmlChar *)"x", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, ul_corner->y);
  xmlSetProp(node, (const xmlChar *)"y", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, lr_corner->x - ul_corner->x);
  xmlSetProp(node, (const xmlChar *)"width", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, lr_corner->y - ul_corner->y);
  xmlSetProp(node, (const xmlChar *)"height", (xmlChar *) d_buf);
}

/*!
 * \brief Draw an arc with a path element
 * \memberof _DiaSvgRenderer
 */
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
  int swp = (angle2 > angle1) ? 0 : 1; /* not always drawing negative direction anymore */
  int large_arc;
  gchar sx_buf[DTOSTR_BUF_SIZE];
  gchar sy_buf[DTOSTR_BUF_SIZE];
  gchar rx_buf[DTOSTR_BUF_SIZE];
  gchar ry_buf[DTOSTR_BUF_SIZE];
  gchar ex_buf[DTOSTR_BUF_SIZE];
  gchar ey_buf[DTOSTR_BUF_SIZE];

  large_arc = (fabs(angle2 - angle1) >= 180);

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"path", NULL);

  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_draw_style(renderer, NULL, colour));

  g_snprintf(buf, sizeof(buf), "M %s,%s A %s,%s 0 %d %d %s,%s",
	     dia_svg_dtostr(sx_buf, sx), dia_svg_dtostr(sy_buf, sy),
	     dia_svg_dtostr(rx_buf, rx), dia_svg_dtostr(ry_buf, ry),
	     large_arc, swp,
	     dia_svg_dtostr(ex_buf, ex), dia_svg_dtostr(ey_buf, ey) );

  xmlSetProp(node, (const xmlChar *)"d", (xmlChar *) buf);
}

/*!
 * \brief Draw an filled arc (pie chart) with a path element
 * \memberof _DiaSvgRenderer
 */
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
  int swp = (angle2 > angle1) ? 0 : 1; /* preserve direction */
  int large_arc = (fabs(angle2 - angle1) >= 180);
  gchar sx_buf[DTOSTR_BUF_SIZE];
  gchar sy_buf[DTOSTR_BUF_SIZE];
  gchar rx_buf[DTOSTR_BUF_SIZE];
  gchar ry_buf[DTOSTR_BUF_SIZE];
  gchar ex_buf[DTOSTR_BUF_SIZE];
  gchar ey_buf[DTOSTR_BUF_SIZE];
  gchar cx_buf[DTOSTR_BUF_SIZE];
  gchar cy_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"path", NULL);

  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *)get_draw_style(renderer, colour, NULL));

  g_snprintf(buf, sizeof(buf), "M %s,%s A %s,%s 0 %d %d %s,%s L %s,%s z",
	     dia_svg_dtostr(sx_buf, sx), dia_svg_dtostr(sy_buf, sy),
	     dia_svg_dtostr(rx_buf, rx), dia_svg_dtostr(ry_buf, ry),
	     large_arc, swp,
	     dia_svg_dtostr(ex_buf, ex), dia_svg_dtostr(ey_buf, ey),
	     dia_svg_dtostr(cx_buf, center->x),
	     dia_svg_dtostr(cy_buf, center->y) );

  xmlSetProp(node, (const xmlChar *)"d", (xmlChar *) buf);
}

/*!
 * \brief Draw an ellipse element (filled and/or stroked)
 * \memberof _DiaSvgRenderer
 */
static void
draw_ellipse(DiaRenderer *self,
	     Point *center,
	     real width, real height,
	     Color *fill, Color *stroke)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  gchar d_buf[DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"ellipse", NULL);

  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) get_draw_style (renderer, fill, stroke));

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
_bezier(DiaRenderer *self,
	BezPoint *points,
	int numpoints,
	Color *fill,
	Color *stroke,
	gboolean closed)
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

  if (fill || stroke)
    xmlSetProp(node, (const xmlChar *)"style", (xmlChar *)get_draw_style(renderer, fill, stroke));

  str = g_string_new(NULL);

  if (points[0].type != BEZ_MOVE_TO)
    g_warning("first BezPoint must be a BEZ_MOVE_TO");

  g_string_printf(str, "M %s %s",
		   dia_svg_dtostr(p1x_buf, (gdouble) points[0].p1.x),
		   dia_svg_dtostr(p1y_buf, (gdouble) points[0].p1.y) );

  for (i = 1; i < numpoints; i++) {
    switch (points[i].type) {
      case BEZ_MOVE_TO:
        if (!dia_renderer_is_capable_of (self, RENDER_HOLES)) {
          g_warning("only first BezPoint should be a BEZ_MOVE_TO");
          g_string_printf (str, "M %s %s",
                          dia_svg_dtostr (p1x_buf, (gdouble) points[i].p1.x),
                          dia_svg_dtostr (p1y_buf, (gdouble) points[i].p1.y) );
        } else {
          g_string_append_printf(str, "M %s %s",
              dia_svg_dtostr(p1x_buf, (gdouble) points[i].p1.x),
              dia_svg_dtostr(p1y_buf, (gdouble) points[i].p1.y) );
        }
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
      default:
        g_return_if_reached ();
    }
  }
  if (fill) {
    xmlSetProp(node, (const xmlChar *)"fill-rule", (const xmlChar *) "evenodd");
    g_string_append(str, "z");
  }
  xmlSetProp(node, (const xmlChar *)"d", (xmlChar *) str->str);
  g_string_free(str, TRUE);
}

/*!
 * \brief Draw a path element (not closed)
 * \memberof _DiaSvgRenderer
 */
static void
draw_bezier(DiaRenderer *self,
	    BezPoint *points,
	    int numpoints,
	    Color *stroke)
{
  _bezier(self, points, numpoints, NULL, stroke, FALSE);
}

/*!
 * \brief Draw a path element (filled and/or stroked)
 * \memberof _DiaSvgRenderer
 */
static void
draw_beziergon (DiaRenderer *self,
		BezPoint *points,
		int numpoints,
		Color *fill,
		Color *stroke)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  /* optimize for stroke-width==0 && fill==stroke */
  if (   fill && stroke && renderer->linewidth == 0.0
      && memcmp(fill, stroke, sizeof(Color))==0)
    stroke = NULL;
  _bezier(self, points, numpoints, fill, stroke, TRUE);
}


/*
 * Draw a text element by delegating to draw_text_line()
 */
static void
draw_string (DiaRenderer  *self,
             const char   *text,
             Point        *pos,
             DiaAlignment  alignment,
             Color        *colour)
{
  TextLine *text_line;
  DiaFont *font;
  double font_height;

  font = dia_renderer_get_font (self, &font_height);
  text_line = text_line_new (text, font, font_height);

  draw_text_line (self, text_line, pos, alignment, colour);
  text_line_destroy (text_line);
}


/*
 * Draw a text element (single line)
 */
static void
draw_text_line (DiaRenderer  *self,
                TextLine     *text_line,
                Point        *pos,
                DiaAlignment  alignment,
                Color        *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  double saved_width;
  char d_buf[DTOSTR_BUF_SIZE];
  DiaFont *font;
  GString *style;

  node = xmlNewTextChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"text",
		         (xmlChar *) text_line_get_string(text_line));

  saved_width = renderer->linewidth;
  renderer->linewidth = 0.001;
  /* return value must not be freed */
  renderer->linewidth = saved_width;
#if 0 /* would need a unit: https://bugzilla.mozilla.org/show_bug.cgi?id=707071#c4 */
  style = g_strdup_printf("%s; font-size: %s", get_draw_style(renderer, colour, NULL),
			dia_svg_dtostr(d_buf, text_line_get_height(text_line)));
#else
  /* get_draw_style: the return value of this function must not be saved
   * anywhere. And of course it must not be free'd */
  style = g_string_new (get_draw_style(renderer, colour, NULL));
#endif
  /* This is going to break for non-LTR texts, as SVG thinks 'start' is
   * 'right' for those. */
  switch (alignment) {
    case DIA_ALIGN_LEFT:
      g_string_append (style, "; text-anchor:start");
      break;
    case DIA_ALIGN_CENTRE:
      g_string_append (style, "; text-anchor:middle");
      break;
    case DIA_ALIGN_RIGHT:
      g_string_append (style, "; text-anchor:end");
      break;
    default:
      break;
  }

  font = text_line_get_font(text_line);
  g_string_append_printf (style, "font-family: %s; font-style: %s; font-weight: %s",
			  dia_font_get_family(font),
			  dia_font_get_slant_string(font),
			  dia_font_get_weight_string(font));

  xmlSetProp(node, (const xmlChar *)"style", (xmlChar *) style->str);
  g_string_free (style, TRUE);

  dia_svg_dtostr(d_buf, pos->x);
  xmlSetProp(node, (const xmlChar *)"x", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, pos->y);
  xmlSetProp(node, (const xmlChar *)"y", (xmlChar *) d_buf);

  /* font-size as single attribute can work like the other length w/o unit */
  dia_svg_dtostr(d_buf, text_line_get_height(text_line));
  xmlSetProp(node, (const xmlChar *)"font-size", (xmlChar *) d_buf);

  dia_svg_dtostr(d_buf, text_line_get_width(text_line));
  xmlSetProp(node, (const xmlChar*)"textLength", (xmlChar *) d_buf);
}

/*!
 * \brief Draw an image element
 * \memberof _DiaSvgRenderer
 */
static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage *image)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  gchar d_buf[DTOSTR_BUF_SIZE];
  gchar *uri = NULL;

  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"image", NULL);

  dia_svg_dtostr(d_buf, point->x);
  xmlSetProp(node, (const xmlChar *)"x", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, point->y);
  xmlSetProp(node, (const xmlChar *)"y", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, width);
  xmlSetProp(node, (const xmlChar *)"width", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, height);
  xmlSetProp(node, (const xmlChar *)"height", (xmlChar *) d_buf);

  /* if the image file location is relative to the SVG file's store
   * a relative path - if it does not have a path: inline it */
  if (strcmp (dia_image_filename(image), "(null)") == 0) {
    gchar *prefix = g_strdup_printf ("data:%s;base64,", dia_image_get_mime_type (image));

    uri = pixbuf_encode_base64 (dia_image_pixbuf (image), prefix);
    if (!uri)
      uri = g_strdup ("(null)");
    xmlSetProp(node, (const xmlChar *)"xlink:href", (xmlChar *) uri);
    g_clear_pointer (&prefix, g_free);
  } else if ((uri = dia_relativize_filename (renderer->filename, dia_image_filename(image))) != NULL)
    xmlSetProp(node, (const xmlChar *)"xlink:href", (xmlChar *) uri);
  else if ((uri = g_filename_to_uri(dia_image_filename(image), NULL, NULL)) != NULL)
    xmlSetProp(node, (const xmlChar *)"xlink:href", (xmlChar *) uri);
  else /* not sure if this fallback is better than nothing */
    xmlSetProp(node, (const xmlChar *)"xlink:href", (xmlChar *) dia_image_filename(image));
  g_clear_pointer (&uri, g_free);
}

/*!
 * \brief Draw a rectangle element (with corner rounding)
 * \memberof _DiaSvgRenderer
 */
static void
draw_rounded_rect (DiaRenderer *self,
		   Point *ul_corner, Point *lr_corner,
		   Color *fill, Color *stroke, real rounding)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"rect", NULL);

  xmlSetProp(node, (const xmlChar *)"style",
	     (const xmlChar *)DIA_SVG_RENDERER_GET_CLASS(self)->get_draw_style (renderer, fill, stroke));

  g_ascii_formatd(buf, sizeof(buf), "%g", ul_corner->x * renderer->scale);
  xmlSetProp(node, (const xmlChar *)"x", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf), "%g", ul_corner->y * renderer->scale);
  xmlSetProp(node, (const xmlChar *)"y", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf), "%g", (lr_corner->x - ul_corner->x) * renderer->scale);
  xmlSetProp(node, (const xmlChar *)"width", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf), "%g", (lr_corner->y - ul_corner->y) * renderer->scale);
  xmlSetProp(node, (const xmlChar *)"height", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf),"%g", (rounding * renderer->scale));
  xmlSetProp(node, (const xmlChar *)"rx", (xmlChar *) buf);
  xmlSetProp(node, (const xmlChar *)"ry", (xmlChar *) buf);
}

/* constructor */
static void
dia_svg_renderer_init (DiaSvgRenderer *self)
{
  self->scale = 1.0;
}

static gpointer parent_class = NULL;

/* destructor */
static void
dia_svg_renderer_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
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

  renderer_class->is_capable_to = is_capable_to;

  renderer_class->set_linewidth  = set_linewidth;
  renderer_class->set_linecaps   = set_linecaps;
  renderer_class->set_linejoin   = set_linejoin;
  renderer_class->set_linestyle  = set_linestyle;
  renderer_class->set_fillstyle  = set_fillstyle;
  renderer_class->set_pattern    = set_pattern;

  renderer_class->draw_line    = draw_line;
  renderer_class->draw_polygon = draw_polygon;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;

  renderer_class->draw_string  = draw_string;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_rect = draw_rect;
  renderer_class->draw_polyline  = draw_polyline;

  renderer_class->draw_bezier   = draw_bezier;
  renderer_class->draw_beziergon = draw_beziergon;
  renderer_class->draw_text_line  = draw_text_line;

  /* high level */
  renderer_class->draw_rounded_rect = draw_rounded_rect;

  /* SVG specific */
  svg_renderer_class->get_draw_style = get_draw_style;
}
