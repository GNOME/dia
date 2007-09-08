/* dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dia_svg.c --  Refactoring by Hans Breuer from :
 *
 * Custom Objects -- objects defined in XML rather than C.
 * Copyright (C) 1999 James Henstridge.
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

#include <string.h>
#include <stdlib.h>

#include <pango/pango-attributes.h>

#include "dia_svg.h"

/** Initialize a style object from another style object or defaults.
 * @param gs An SVG style object to initialize.
 * @param parent_style An SVG style object to copy values from, or NULL,
 *                     in which case defaults will be used.
 */
void
dia_svg_style_init(DiaSvgStyle *gs, DiaSvgStyle *parent_style)
{
  g_return_if_fail (gs);
  gs->stroke = parent_style ? parent_style->stroke : (-1);
  gs->line_width = parent_style ? parent_style->line_width : 0.0;
  gs->linestyle = parent_style ? parent_style->linestyle : LINESTYLE_SOLID;
  gs->dashlength = parent_style ? parent_style->dashlength : 1;
  gs->fill = parent_style ? parent_style->fill : (-1);
  gs->linecap = parent_style ? parent_style->linecap : DIA_SVG_LINECAPS_DEFAULT;
  gs->linejoin = parent_style ? parent_style->linejoin : DIA_SVG_LINEJOIN_DEFAULT;
  gs->linestyle = parent_style ? parent_style->linestyle : DIA_SVG_LINESTYLE_DEFAULT;
  gs->font = (parent_style && parent_style->font) ? dia_font_ref(parent_style->font) : NULL;
  gs->font_height = parent_style ? parent_style->font_height : 0.8;
  gs->alignment = parent_style ? parent_style->alignment : ALIGN_LEFT;
}

/** Copy style values from one SVG style object to another.
 * @param dest SVG style object to copy to.
 * @param src SVG style object to copy from.
 */
void
dia_svg_style_copy(DiaSvgStyle *dest, DiaSvgStyle *src)
{
  g_return_if_fail (dest && src);

  dest->stroke = src->stroke;
  dest->line_width = src->line_width;
  dest->linestyle = src->linestyle;
  dest->dashlength = src->dashlength;
  dest->fill = src->fill;
  if (dest->font)
    dia_font_unref (dest->font);
  dest->font = src->font ? dia_font_ref(src->font) : NULL;
  dest->font_height = src->font_height;
  dest->alignment = src->alignment;
}

/** Parse an SVG color description.
 * @param color A place to store the color information (0RGB)
 * @param str An SVG color description string to parse.
 * @return TRUE if parsing was successful.
 * Shouldn't we use an actual Dia Color object as return value?
 * Would require that the DiaSvgStyle object uses that, too.  If we did that,
 * we could even return the color object directly, and we would be able to use
 * >8 bits per channel.
 * But we would not be able to handle named colors anymore ...
 */
static gboolean
_parse_color(gint32 *color, const char *str)
{
  if (str[0] == '#')
    *color = strtol(str+1, NULL, 16) & 0xffffff;
  else if (0 == strncmp(str, "none", 4))
    *color = DIA_SVG_COLOUR_NONE;
  else if (0 == strncmp(str, "foreground", 10) || 0 == strncmp(str, "fg", 2) ||
	   0 == strncmp(str, "inverse", 7))
    *color = DIA_SVG_COLOUR_FOREGROUND;
  else if (0 == strncmp(str, "background", 10) || 0 == strncmp(str, "bg", 2) ||
	   0 == strncmp(str, "default", 7))
    *color = DIA_SVG_COLOUR_BACKGROUND;
  else if (0 == strcmp(str, "text"))
    *color = DIA_SVG_COLOUR_TEXT;
  else if (0 == strncmp(str, "rgb(", 4)) {
    int r = 0, g = 0, b = 0;
    if (3 == sscanf (str+4, "%d,%d,%d", &r, &g, &b))
      *color = ((r<<16) & 0xFF0000) | ((g<<8) & 0xFF00) | (b & 0xFF);
    else
      return FALSE;
  } else {
    /* Pango needs null terminated strings, so we just use it as a fallback */
    PangoColor pc;
    char* se = strchr (str, ';');

    if (!se) {
      if (pango_color_parse (&pc, str))
	*color = ((pc.red >> 8) << 16) | ((pc.green >> 8) << 8) | (pc.blue >> 8);
      else
	return FALSE;
    } else {
      gchar* sz = g_strndup (str, se - str);
      gboolean ret = pango_color_parse (&pc, str);

      if (ret)
	*color = ((pc.red >> 8) << 16) | ((pc.green >> 8) << 8) | (pc.blue >> 8);
      g_free (sz);
      return ret;
    }
  }
  return TRUE;
}

enum
{
  FONT_NAME_LENGTH_MAX = 40
};

/** This function not only parses the style attribute of the given node
 *  it also extracts some of the style properties directly.
 * @param node An XML node to parse a style from.
 * @param s The SVG style object to fill out.  This should previously be
 *          initialized to some default values.
 * @param user_scale, if >0 scalable values (font-size, stroke-width, ...) are divided by this, otherwise ignored
 * @bug This function is way too long (213 lines). So dont touch it :)
 */
void
dia_svg_parse_style(xmlNodePtr node, DiaSvgStyle *s, real user_scale)
{
  xmlChar *str;
  gchar temp[FONT_NAME_LENGTH_MAX+1]; /* font-family names will be limited to 40 characters */
  int i = 0;
  gboolean over = FALSE;
  char *family = NULL, *style = NULL, *weight = NULL;

  str = xmlGetProp(node, (const xmlChar *)"style");

  if (str) {
    gchar *ptr = (gchar *)str;
    while (ptr[0] != '\0') {
      /* skip white space at start */
      while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      if (!strncmp("font-family:", ptr, 12)) {
	ptr += 12;
	while ((ptr[0] != '\0') && g_ascii_isspace(ptr[0])) ptr++;
	i = 0; over = FALSE;
	while (ptr[0] != '\0' && ptr[0] != ';' && !over) {
	  if (i < FONT_NAME_LENGTH_MAX) {
	    temp[i] = ptr[0];
	  } else over = TRUE;
	  i++;
	  ptr++;
	}
	temp[i] = '\0';

	if (!over) family = g_strdup(temp);
      } else if (!strncmp("font-weight:", ptr, 12)) {
	ptr += 12;
	while ((ptr[0] != '\0') && g_ascii_isspace(ptr[0])) ptr++;
	i = 0; over = FALSE;
	while (ptr[0] != '\0' && ptr[0] != ';' && !over) {
	  if (i < FONT_NAME_LENGTH_MAX) {
	    temp[i] = ptr[0];
	  } else over = TRUE;
	  i++;
	  ptr++;
	}
	temp[i] = '\0';

	if (!over) weight = g_strdup(temp);
      } else if (!strncmp("font-style:", ptr, 11)) {
	ptr += 11;
	while ((ptr[0] != '\0') && g_ascii_isspace(ptr[0])) ptr++;
	i = 0; over = FALSE;
	while (ptr[0] != '\0' && ptr[0] != ';' && !over) {
	  if (i < FONT_NAME_LENGTH_MAX) {
	    temp[i] = ptr[0];
	  } else over = TRUE;
	  i++;
	  ptr++;
	}
	temp[i] = '\0';

	if (!over) style = g_strdup(temp);
      } else if (!strncmp("font-size:", ptr, 10)) {
	ptr += 10;
	while ((ptr[0] != '\0') && g_ascii_isspace(ptr[0])) ptr++;
	i = 0; over = FALSE;
	while (ptr[0] != '\0' && ptr[0] != ';' && !over) {
	  if (i < FONT_NAME_LENGTH_MAX) {
	    temp[i] = ptr[0];
	  } else over = TRUE;
	  i++;
	  ptr++;
	}
	temp[i] = '\0';

	if (!over) {
	  s->font_height = g_ascii_strtod(temp, NULL);
	  if (user_scale > 0)
	    s->font_height /= user_scale;
	}
      } else if (!strncmp("text-anchor:", ptr, 12)) {
	ptr += 12;
	while ((ptr[0] != '\0') && g_ascii_isspace(ptr[0])) ptr++;
	if (!strncmp(ptr, "start", 5))
	  s->alignment = ALIGN_LEFT;
	else if (!strncmp(ptr, "end", 3))
	  s->alignment = ALIGN_RIGHT;
	else if (!strncmp(ptr, "middle", 6))
	  s->alignment = ALIGN_CENTER;

      } else if (!strncmp("stroke-width:", ptr, 13)) {
	ptr += 13;
	s->line_width = g_ascii_strtod(ptr, &ptr);
	if (user_scale > 0)
	  s->line_width /= user_scale;
      } else if (!strncmp("stroke:", ptr, 7)) {
	ptr += 7;
	while ((ptr[0] != '\0') && g_ascii_isspace(ptr[0])) ptr++;
	if (ptr[0] == '\0') break;

	_parse_color (&s->stroke, ptr);
      } else if (!strncmp("fill:", ptr, 5)) {
	ptr += 5;
	while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
	if (ptr[0] == '\0') break;

	_parse_color (&s->fill, ptr);
      } else if (!strncmp("stroke-linecap:", ptr, 15)) {
	ptr += 15;
	while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
	if (ptr[0] == '\0') break;

	if (!strncmp(ptr, "butt", 4))
	  s->linecap = LINECAPS_BUTT;
	else if (!strncmp(ptr, "round", 5))
	  s->linecap = LINECAPS_ROUND;
	else if (!strncmp(ptr, "square", 6) || !strncmp(ptr, "projecting", 10))
	  s->linecap = LINECAPS_PROJECTING;
	else if (!strncmp(ptr, "default", 7))
	  s->linecap = DIA_SVG_LINECAPS_DEFAULT;
      } else if (!strncmp("stroke-linejoin:", ptr, 16)) {
	ptr += 16;
	while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
	if (ptr[0] == '\0') break;

	if (!strncmp(ptr, "miter", 5))
	  s->linejoin = LINEJOIN_MITER;
	else if (!strncmp(ptr, "round", 5))
	  s->linejoin = LINEJOIN_ROUND;
	else if (!strncmp(ptr, "bevel", 5))
	  s->linejoin = LINEJOIN_BEVEL;
	else if (!strncmp(ptr, "default", 7))
	  s->linejoin = DIA_SVG_LINEJOIN_DEFAULT;
      } else if (!strncmp("stroke-pattern:", ptr, 15)) {
	ptr += 15;
	while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
	if (ptr[0] == '\0') break;

	if (!strncmp(ptr, "solid", 5))
	  s->linestyle = LINESTYLE_SOLID;
	else if (!strncmp(ptr, "dashed", 6))
	  s->linestyle = LINESTYLE_DASHED;
	else if (!strncmp(ptr, "dash-dot", 8))
	  s->linestyle = LINESTYLE_DASH_DOT;
	else if (!strncmp(ptr, "dash-dot-dot", 12))
	  s->linestyle = LINESTYLE_DASH_DOT_DOT;
	else if (!strncmp(ptr, "dotted", 6))
	  s->linestyle = LINESTYLE_DOTTED;
	else if (!strncmp(ptr, "default", 7))
	  s->linestyle = DIA_SVG_LINESTYLE_DEFAULT;
	/* XXX: deal with a real pattern */
      } else if (!strncmp("stroke-dashlength:", ptr, 18)) {
	ptr += 18;
	while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
	if (ptr[0] == '\0') break;

	if (!strncmp(ptr, "default", 7))
	  s->dashlength = 1.0;
	else {
	  s->dashlength = g_ascii_strtod(ptr, &ptr);
	  if (user_scale > 0)
	    s->dashlength /= user_scale;
	}
      } else if (!strncmp("stroke-dasharray:", ptr, 17)) {
	/* FIXME? do we need to read an array here (not clear from
	 * Dia's usage); do we need to set the linestyle depending
	 * on the array's size ? --hb
	 */
	s->linestyle = LINESTYLE_DASHED;
	ptr += 17;
	while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
	if (ptr[0] == '\0') break;

	if (!strncmp(ptr, "default", 7))
	  s->dashlength = 1.0;
	else {
	  s->dashlength = g_ascii_strtod(ptr, &ptr);
	  if (user_scale > 0)
	    s->dashlength /= user_scale;
	}
      }

      /* skip up to the next attribute */
      while (ptr[0] != '\0' && ptr[0] != ';' && ptr[0] != '\n') ptr++;
      if (ptr[0] != '\0') ptr++;
    }
    xmlFree(str);
  }

  /* ugly svg variations, it is allowed to give style properties without
   * the style attribute, i.e. direct attributes
   */
  str = xmlGetProp(node, (const xmlChar *)"fill");
  if (str) {
    _parse_color (&s->fill, (char *) str);
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"stroke");
  if (str) {
    _parse_color (&s->stroke, (char *) str);
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"stroke-width");
  if (str) {
    s->line_width = g_ascii_strtod((char *) str, NULL);
    xmlFree(str);
    if (user_scale > 0)
      s->line_width /= user_scale;
  }

  if (family || style || weight) {
    if (s->font)
      dia_font_unref (s->font);
    s->font = dia_font_new_from_style(DIA_FONT_SANS,s->font_height/*bogus*/);
    if (family) {
      dia_font_set_any_family(s->font,family);
      g_free(family);
    }
    if (style) {
      dia_font_set_slant_from_string(s->font,style);
      g_free(style);
    }
    if (weight) {
      dia_font_set_weight_from_string(s->font,weight);
      g_free(weight);
    }
  }
}

/** Parse a SVG description of an arc segment.
 * Code stolen from (and adapted)
 * http://www.inkscape.org/doc/doxygen/html/svg-path_8cpp.php#a7
 * which may have got it from rsvg, hope it is correct ;)
 * @param points
 * @param xc
 * @param yc
 * @param th0
 * @param th1
 * @param rx
 * @param ry
 * @param x_axis_rotation
 * @param last_p2
 * If you want the description of the algorithm read the SVG specs.
 */
static void
_path_arc_segment(GArray* points,
		  real xc, real yc,
		  real th0, real th1,
		  real rx, real ry,
		  real x_axis_rotation,
		  Point *last_p2)
{
  BezPoint bez;
  real sin_th, cos_th;
  real a00, a01, a10, a11;
  real x1, y1, x2, y2, x3, y3;
  real t;
  real th_half;

  sin_th = sin (x_axis_rotation * (M_PI / 180.0));
  cos_th = cos (x_axis_rotation * (M_PI / 180.0));
  /* inverse transform compared with rsvg_path_arc */
  a00 = cos_th * rx;
  a01 = -sin_th * ry;
  a10 = sin_th * rx;
  a11 = cos_th * ry;

  th_half = 0.5 * (th1 - th0);
  t = (8.0 / 3.0) * sin(th_half * 0.5) * sin(th_half * 0.5) / sin(th_half);
  x1 = xc + cos (th0) - t * sin (th0);
  y1 = yc + sin (th0) + t * cos (th0);
  x3 = xc + cos (th1);
  y3 = yc + sin (th1);
  x2 = x3 + t * sin (th1);
  y2 = y3 - t * cos (th1);

  bez.type = BEZ_CURVE_TO;
  bez.p1.x = a00 * x1 + a01 * y1;
  bez.p1.y = a10 * x1 + a11 * y1;
  bez.p2.x = a00 * x2 + a01 * y2;
  bez.p2.y = a10 * x2 + a11 * y2;
  bez.p3.x = a00 * x3 + a01 * y3;
  bez.p3.y = a10 * x3 + a11 * y3;

  *last_p2 = bez.p2;

  g_array_append_val(points, bez);
}

/** Parse an SVG description of a full arc.
 * @param points
 * @param cpx
 * @param cpy
 * @param rx
 * @param ry
 * @param x_axis_rotation
 * @param large_arc_flag
 * @param sweep_flag
 * @param x
 * @param y
 * @param last_p2
 * @bug Also here don't know what the parameters mean.
 */
static void
_path_arc(GArray *points, double cpx, double cpy,
	   double rx, double ry, double x_axis_rotation,
	   int large_arc_flag, int sweep_flag,
	   double x, double y,
	   Point *last_p2)
{
    double sin_th, cos_th;
    double a00, a01, a10, a11;
    double x0, y0, x1, y1, xc, yc;
    double d, sfactor, sfactor_sq;
    double th0, th1, th_arc;
    double px, py, pl;
    int i, n_segs;

    sin_th = sin (x_axis_rotation * (M_PI / 180.0));
    cos_th = cos (x_axis_rotation * (M_PI / 180.0));

    /*
     * Correction of out-of-range radii as described in Appendix F.6.6:
     *
     *  1. Ensure radii are non-zero (Done?).
     *  2. Ensure that radii are positive.
     *  3. Ensure that radii are large enough.
     */
    if(rx < 0.0) rx = -rx;
    if(ry < 0.0) ry = -ry;

    px = cos_th * (cpx - x) * 0.5 + sin_th * (cpy - y) * 0.5;
    py = cos_th * (cpy - y) * 0.5 - sin_th * (cpx - x) * 0.5;
    pl = (px * px) / (rx * rx) + (py * py) / (ry * ry);

    if(pl > 1.0)
    {
	pl  = sqrt(pl);
	rx *= pl;
	ry *= pl;
    }

    /* Proceed with computations as described in Appendix F.6.5 */

    a00 = cos_th / rx;
    a01 = sin_th / rx;
    a10 = -sin_th / ry;
    a11 = cos_th / ry;
    x0 = a00 * cpx + a01 * cpy;
    y0 = a10 * cpx + a11 * cpy;
    x1 = a00 * x + a01 * y;
    y1 = a10 * x + a11 * y;
    /* (x0, y0) is current point in transformed coordinate space.
       (x1, y1) is new point in transformed coordinate space.

       The arc fits a unit-radius circle in this space.
    */
    d = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    sfactor_sq = 1.0 / d - 0.25;
    if (sfactor_sq < 0) sfactor_sq = 0;
    sfactor = sqrt (sfactor_sq);
    if (sweep_flag == large_arc_flag) sfactor = -sfactor;
    xc = 0.5 * (x0 + x1) - sfactor * (y1 - y0);
    yc = 0.5 * (y0 + y1) + sfactor * (x1 - x0);
    /* (xc, yc) is center of the circle. */

    th0 = atan2 (y0 - yc, x0 - xc);
    th1 = atan2 (y1 - yc, x1 - xc);

    th_arc = th1 - th0;
    if (th_arc < 0 && sweep_flag)
      th_arc += 2 * M_PI;
    else if (th_arc > 0 && !sweep_flag)
      th_arc -= 2 * M_PI;

    n_segs = (int) ceil (fabs (th_arc / (M_PI * 0.5 + 0.001)));

    for (i = 0; i < n_segs; i++) {
      _path_arc_segment(points, xc, yc,
			th0 + i * th_arc / n_segs,
			th0 + (i + 1) * th_arc / n_segs,
			rx, ry, x_axis_rotation,
			last_p2);
    }
}

/* routine to chomp off the start of the string */
#define path_chomp(path) while (path[0]!='\0'&&strchr(" \t\n\r,", path[0])) path++

/** Takes SVG path content and converts it in an array of BezPoint.
 *
 *  SVG pathes can contain multiple MOVE_TO commands while Dia's bezier
 *  object can only contain one so you may need to call this function
 *  multiple times.
 *
 * @param path_str A string describing an SVG path.
 * @param unparsed The position in `path_str' where parsing ended, or NULL if
 *                 the string was completely parsed.  This should be used for
 *                 calling the function until it is fully parsed.
 * @param closed Whether the path was closed.
 * @returns Array of BezPoint objects, or NULL if an error occurred.
 *          The caller is responsible for freeing the array.
 * @bug This function is way too long (324 lines). So dont touch it. please!
 * Shouldn't we try to turn straight lines, simple arc, polylines and
 * zigzaglines into their appropriate objects?  Could either be done by
 * returning an object or by having functions that try parsing as
 * specific simple paths.
 * NOPE: Dia is capable to handle beziers and the file has given us some so WHY should be break it in to pieces ???
 */
GArray*
dia_svg_parse_path(const gchar *path_str, gchar **unparsed, gboolean *closed)
{
  enum {
    PATH_MOVE, PATH_LINE, PATH_HLINE, PATH_VLINE, PATH_CURVE,
    PATH_SMOOTHCURVE, PATH_ARC, PATH_CLOSE } last_type = PATH_MOVE;
  Point last_open = {0.0, 0.0};
  Point last_point = {0.0, 0.0};
  Point last_control = {0.0, 0.0};
  gboolean last_relative = FALSE;
  GArray *points;
  BezPoint bez;
  gchar *path = (gchar *)path_str;
  gboolean need_next_element = FALSE;

  *closed = FALSE;
  *unparsed = NULL;

  points = g_array_new(FALSE, FALSE, sizeof(BezPoint));
  g_array_set_size(points, 0);

  path_chomp(path);
  while (path[0] != '\0') {
#ifdef DEBUG_CUSTOM
    g_print("Path: %s\n", path);
#endif
    /* check for a new command */
    switch (path[0]) {
    case 'M':
      if (points->len > 0) {
	need_next_element = TRUE;
	goto MORETOPARSE;
      }
      path++;
      path_chomp(path);
      last_type = PATH_MOVE;
      last_relative = FALSE;
      break;
    case 'm':
      if (points->len > 0) {
	need_next_element = TRUE;
	goto MORETOPARSE;
      }
      path++;
      path_chomp(path);
      last_type = PATH_MOVE;
      last_relative = TRUE;
      break;
    case 'L':
      path++;
      path_chomp(path);
      last_type = PATH_LINE;
      last_relative = FALSE;
      break;
    case 'l':
      path++;
      path_chomp(path);
      last_type = PATH_LINE;
      last_relative = TRUE;
      break;
    case 'H':
      path++;
      path_chomp(path);
      last_type = PATH_HLINE;
      last_relative = FALSE;
      break;
    case 'h':
      path++;
      path_chomp(path);
      last_type = PATH_HLINE;
      last_relative = TRUE;
      break;
    case 'V':
      path++;
      path_chomp(path);
      last_type = PATH_VLINE;
      last_relative = FALSE;
      break;
    case 'v':
      path++;
      path_chomp(path);
      last_type = PATH_VLINE;
      last_relative = TRUE;
      break;
    case 'C':
      path++;
      path_chomp(path);
      last_type = PATH_CURVE;
      last_relative = FALSE;
      break;
    case 'c':
      path++;
      path_chomp(path);
      last_type = PATH_CURVE;
      last_relative = TRUE;
      break;
    case 'S':
      path++;
      path_chomp(path);
      last_type = PATH_SMOOTHCURVE;
      last_relative = FALSE;
      break;
    case 's':
      path++;
      path_chomp(path);
      last_type = PATH_SMOOTHCURVE;
      last_relative = TRUE;
      break;
    case 'Z':
    case 'z':
      path++;
      path_chomp(path);
      last_type = PATH_CLOSE;
      last_relative = FALSE;
      break;
    case 'A':
      path++;
      path_chomp(path);
      last_type = PATH_ARC;
      last_relative = FALSE;
      break;
    case 'a':
      path++;
      path_chomp(path);
      last_type = PATH_ARC;
      last_relative = TRUE;
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '.':
    case '+':
    case '-':
      if (last_type == PATH_CLOSE) {
	g_warning("parse_path: argument given for implicite close path");
	/* consume one number so we don't fall into an infinite loop */
	while (path != '\0' && strchr("0123456789.+-", path[0])) path++;
	path_chomp(path);
	*closed = TRUE;
	need_next_element = TRUE;
	goto MORETOPARSE;
      }
      break;
    default:
      g_warning("unsupported path code '%c'", path[0]);
      path++;
      path_chomp(path);
      break;
    }

    /* actually parse the path component */
    switch (last_type) {
    case PATH_MOVE:
      bez.type = BEZ_MOVE_TO;
      bez.p1.x = g_ascii_strtod(path, &path);
      path_chomp(path);
      bez.p1.y = g_ascii_strtod(path, &path);
      path_chomp(path);
      if (last_relative) {
	bez.p1.x += last_point.x;
	bez.p1.y += last_point.y;
      }
      last_point = bez.p1;
      last_control = bez.p1;
      last_open = bez.p1;
      g_array_append_val(points, bez);
      break;
    case PATH_LINE:
      bez.type = BEZ_LINE_TO;
      bez.p1.x = g_ascii_strtod(path, &path);
      path_chomp(path);
      bez.p1.y = g_ascii_strtod(path, &path);
      path_chomp(path);
      if (last_relative) {
	bez.p1.x += last_point.x;
	bez.p1.y += last_point.y;
      }
      last_point = bez.p1;
      last_control = bez.p1;

      g_array_append_val(points, bez);
      break;
    case PATH_HLINE:
      bez.type = BEZ_LINE_TO;
      bez.p1.x = g_ascii_strtod(path, &path);
      path_chomp(path);
      bez.p1.y = last_point.y;
      if (last_relative)
	bez.p1.x += last_point.x;
      last_point = bez.p1;
      last_control = bez.p1;

      g_array_append_val(points, bez);
      break;
    case PATH_VLINE:
      bez.type = BEZ_LINE_TO;
      bez.p1.x = last_point.x;
      bez.p1.y = g_ascii_strtod(path, &path);
      path_chomp(path);
      if (last_relative)
	bez.p1.y += last_point.y;
      last_point = bez.p1;
      last_control = bez.p1;

      g_array_append_val(points, bez);
      break;
    case PATH_CURVE:
      bez.type = BEZ_CURVE_TO;
      bez.p1.x = g_ascii_strtod(path, &path);
      path_chomp(path);
      bez.p1.y = g_ascii_strtod(path, &path);
      path_chomp(path);
      bez.p2.x = g_ascii_strtod(path, &path);
      path_chomp(path);
      bez.p2.y = g_ascii_strtod(path, &path);
      path_chomp(path);
      bez.p3.x = g_ascii_strtod(path, &path);
      path_chomp(path);
      bez.p3.y = g_ascii_strtod(path, &path);
      path_chomp(path);
      if (last_relative) {
	bez.p1.x += last_point.x;
	bez.p1.y += last_point.y;
	bez.p2.x += last_point.x;
	bez.p2.y += last_point.y;
	bez.p3.x += last_point.x;
	bez.p3.y += last_point.y;
      }
      last_point = bez.p3;
      last_control = bez.p2;

      g_array_append_val(points, bez);
      break;
    case PATH_SMOOTHCURVE:
      bez.type = BEZ_CURVE_TO;
      bez.p1.x = 2 * last_point.x - last_control.x;
      bez.p1.y = 2 * last_point.y - last_control.y;
      bez.p2.x = g_ascii_strtod(path, &path);
      path_chomp(path);
      bez.p2.y = g_ascii_strtod(path, &path);
      path_chomp(path);
      bez.p3.x = g_ascii_strtod(path, &path);
      path_chomp(path);
      bez.p3.y = g_ascii_strtod(path, &path);
      path_chomp(path);
      if (last_relative) {
	bez.p2.x += last_point.x;
	bez.p2.y += last_point.y;
	bez.p3.x += last_point.x;
	bez.p3.y += last_point.y;
      }
      last_point = bez.p3;
      last_control = bez.p2;

      g_array_append_val(points, bez);
      break;
    case PATH_ARC :
      {
	real  rx, ry;
	real  xrot;
	int   largearc, sweep;
	Point dest, dest_c;
	dest_c.x=0;
	dest_c.y=0;

	rx = g_ascii_strtod(path, &path);
	path_chomp(path);
	ry = g_ascii_strtod(path, &path);
	path_chomp(path);
	xrot = g_ascii_strtod(path, &path);
	path_chomp(path);

	largearc = (int)g_ascii_strtod(path, &path);
	path_chomp(path);
	sweep = (int)g_ascii_strtod(path, &path);
	path_chomp(path);

	dest.x = g_ascii_strtod(path, &path);
	path_chomp(path);
	dest.y = g_ascii_strtod(path, &path);
	path_chomp(path);

	if (last_relative) {
	  dest.x += last_point.x;
	  dest.y += last_point.y;
	}

	_path_arc (points, last_point.x, last_point.y,
		   rx, ry, xrot, largearc, sweep, dest.x, dest.y,
		   &dest_c);
	last_point = dest;
	last_control = dest_c;
      }
      break;
    case PATH_CLOSE:
      /* close the path with a line */
      if (last_open.x != last_point.x || last_open.y != last_point.y) {
	bez.type = BEZ_LINE_TO;
	bez.p1 = last_open;
	g_array_append_val(points, bez);
      }
      *closed = TRUE;
      need_next_element = TRUE;
    }
    /* get rid of any ignorable characters */
    path_chomp(path);
MORETOPARSE:
    if (need_next_element) {
      /* check if there really is mor to be parsed */
      if (path[0] != 0)
	*unparsed = path;
      break; /* while */
    }
  }

  /* avoid returning an array with only one point (I'd say the exporter
   * producing such is rather broken, but *our* bezier creation code
   * would crash on it.
   */
  if (points->len < 2) {
    g_array_set_size(points, 0);
  }
  return points;
}
