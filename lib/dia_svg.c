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

/*!
 * \defgroup DiaSvg Services for SVG
 * \ingroup Plugins
 * \brief A set of function helping to read and write SVG with Dia
 *
 * The Dia application supports various variants of SVG. There are
 * at least two importers of SVG dialects, namely \ref Shapes and
 * the standard SVG importer \ref Plugins. Both are using theses
 * serivces to a large extend, but they are also doing there own
 * thing regarding the SVG dialect interpretation.
 */

/*!
 * \brief Initialize a style object from another style object or defaults.
 * @param gs An SVG style object to initialize.
 * @param parent_style An SVG style object to copy values from, or NULL,
 *                     in which case defaults will be used.
 * \ingroup DiaSvg
 */
void
dia_svg_style_init(DiaSvgStyle *gs, DiaSvgStyle *parent_style)
{
  g_return_if_fail (gs);
  gs->stroke = parent_style ? parent_style->stroke : DIA_SVG_COLOUR_DEFAULT;
  gs->stroke_opacity = parent_style ? parent_style->stroke_opacity : 1.0;
  gs->line_width = parent_style ? parent_style->line_width : 0.0;
  gs->linestyle = parent_style ? parent_style->linestyle : LINESTYLE_SOLID;
  gs->dashlength = parent_style ? parent_style->dashlength : 1;
  /* http://www.w3.org/TR/SVG/painting.html#FillProperty - default black
   * but we still have to see the difference
   */ 
  gs->fill = parent_style ? parent_style->fill : DIA_SVG_COLOUR_DEFAULT;
  gs->fill_opacity = parent_style ? parent_style->fill_opacity : 1.0;
  gs->linecap = parent_style ? parent_style->linecap : DIA_SVG_LINECAPS_DEFAULT;
  gs->linejoin = parent_style ? parent_style->linejoin : DIA_SVG_LINEJOIN_DEFAULT;
  gs->linestyle = parent_style ? parent_style->linestyle : DIA_SVG_LINESTYLE_DEFAULT;
  gs->font = (parent_style && parent_style->font) ? dia_font_ref(parent_style->font) : NULL;
  gs->font_height = parent_style ? parent_style->font_height : 0.8;
  gs->alignment = parent_style ? parent_style->alignment : ALIGN_LEFT;
}

/*!
 * \brief Copy style values from one SVG style object to another.
 * @param dest SVG style object to copy to.
 * @param src SVG style object to copy from.
 * \ingroup DiaSvg
 */
void
dia_svg_style_copy(DiaSvgStyle *dest, DiaSvgStyle *src)
{
  g_return_if_fail (dest && src);

  dest->stroke = src->stroke;
  dest->stroke_opacity = src->stroke_opacity;
  dest->line_width = src->line_width;
  dest->linestyle = src->linestyle;
  dest->dashlength = src->dashlength;
  dest->fill = src->fill;
  dest->fill_opacity = src->fill_opacity;
  if (dest->font)
    dia_font_unref (dest->font);
  dest->font = src->font ? dia_font_ref(src->font) : NULL;
  dest->font_height = src->font_height;
  dest->alignment = src->alignment;
}

static const struct _SvgNamedColor {
  const char *name;
  const gint  value;
} _svg_named_colors [] = {
  { "maroon", 0x800000 },
  { "red", 0xff0000 },
  { "orange", 0xffA500 },
  { "yellow", 0xffff00 },
  { "olive", 0x808000 },
  { "purple", 0x800080 },
  { "fuchsia", 0xff00ff },
  { "white", 0xffffff },
  { "lime", 0x00ff00 },
  { "green", 0x008000 },
  { "navy", 0x000080 },
  { "blue", 0x0000ff },
  { "aqua", 0x00ffff },
  { "teal", 0x008080 },
  { "black", 0x000000 },
  { "silver", 0xc0c0c0 },
  { "gray", 0x808080 }

};

/*!
 * \brief Get an SVG color value by name
 *
 * The list of named SVG colors has only 17 entries according to
 * http://www.w3.org/TR/CSS21/syndata.html#color-units
 * Still pango_color_parse() does not support seven of them including
 * 'white'. This function supports all of them.
 *
 * \ingroup DiaSvg
 */
static gboolean
svg_named_color (const char *name, gint32 *color)
{
  int i;

  g_return_val_if_fail (name != NULL && color != NULL, FALSE);

  for (i = 0; i < G_N_ELEMENTS(_svg_named_colors); i++) {
    if (strncmp (name, _svg_named_colors[i].name, strlen(_svg_named_colors[i].name)) == 0) {
      *color = _svg_named_colors[i].value;
      return TRUE;
    }
  }
  return FALSE;
}
/*! 
 * \brief Parse an SVG color description.
 *
 * @param color A place to store the color information (0RGB)
 * @param str An SVG color description string to parse.
 * @return TRUE if parsing was successful.
 *
 * This function is rather tolerant compared to the SVG specification.
 * It supports special names like 'fg', 'bg', 'foregroumd', 'background';
 * three numeric representations: '\#FF0000', 'rgb(1.0,0.0,0.0), 'rgb(100%,0%,0%)'
 * and named colors from two domains: SVG and Pango.
 *
 * \note Shouldn't we use an actual Dia Color object as return value?
 * Would require that the DiaSvgStyle object uses that, too.  If we did that,
 * we could even return the color object directly, and we would be able to use
 * >8 bits per channel.
 * But we would not be able to handle named colors anymore ...
 *
 * \ingroup DiaSvg
 */
static gboolean
_parse_color(gint32 *color, const char *str)
{
  if (str[0] == '#') {
    char *endp = NULL;
    guint32 val = strtol(str+1, &endp, 16);
    if (endp - (str + 1) > 3) /* no 16-bit color here */
      *color =  val & 0xffffff;
    else /* weak estimation: Pango is reusing bits to fill */
      *color = ((0xF & val)<<4) | ((0xF0 & val)<<8) | (((0xF00 & val)>>4)<<16);
  } else if (0 == strncmp(str, "none", 4))
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
    if (3 == sscanf (str+4, "%d,%d,%d", &r, &g, &b)) {
      /* Set alpha to 1.0 */
      *color = ((0xFF<<24) & 0xFF000000) | ((r<<16) & 0xFF0000) | ((g<<8) & 0xFF00) | (b & 0xFF);
    } else if (strchr (str+4, '%')) {
      /* e.g. cairo uses percent values */
      gchar **vals = g_strsplit (str+4, "%,", -1);
      int     i;

      *color = 0xFF000000;
      for (i = 0; vals[i] && i < 3; ++i)
	*color |= ((int)(((255 * g_ascii_strtod(vals[i], NULL)) / 100))<<(16-(8*i)));
      g_strfreev (vals);
    } else {
      return FALSE;
    }
  } else if (0 == strncmp(str, "rgba(", 5)) {
    int r = 0, g = 0, b = 0, a = 0;
    if (4 == sscanf (str+4, "%d,%d,%d,%d", &r, &g, &b, &a))
      *color = ((a<<24) & 0xFF000000) | ((r<<16) & 0xFF0000) | ((g<<8) & 0xFF00) | (b & 0xFF);
    else
      return FALSE;
  } else if (svg_named_color (str, color)) {
    return TRUE;
  } else {
    /* Pango needs null terminated strings, so we just use it as a fallback */
    PangoColor pc;
    char* se = strchr (str, ';');

    if (!se) {
      if (pango_color_parse (&pc, str))
	*color = ((0xFF<<24) & 0xFF000000) | ((pc.red >> 8) << 16) | ((pc.green >> 8) << 8) | (pc.blue >> 8);
      else
	return FALSE;
    } else {
      gchar* sz = g_strndup (str, se - str);
      gboolean ret = pango_color_parse (&pc, sz);

      if (ret)
	*color = ((0xFF<<24) & 0xFF000000) | ((pc.red >> 8) << 16) | ((pc.green >> 8) << 8) | (pc.blue >> 8);
      g_free (sz);
      return ret;
    }
  }
  return TRUE;
}

gboolean
dia_svg_parse_color (const gchar *str, Color *color)
{
  gint32 c;
  gboolean ret = _parse_color (&c, str);

  if (ret) {
    color->red   = ((c & 0xff0000) >> 16) / 255.0;
    color->green = ((c & 0x00ff00) >> 8) / 255.0;
    color->blue  =  (c & 0x0000ff) / 255.0;
    color->alpha = 1.0;
  }
  return ret;
}

enum
{
  FONT_NAME_LENGTH_MAX = 40
};

static void
_parse_dasharray (DiaSvgStyle *s, real user_scale, gchar *str, gchar **end)
{
  gchar *ptr;
  gchar **dashes = g_strsplit ((gchar *)str, ",", -1);
  int n = 0;
  real dl;

  s->dashlength = g_ascii_strtod(str, &ptr);
  if (s->dashlength <= 0.0) /* e.g. "none" */
    s->linestyle = LINESTYLE_SOLID;
  else if (user_scale > 0)
    s->dashlength /= user_scale;

  if (s->dashlength) { /* at least one value */
    while (dashes[n])
      ++n; /* Dia can not do arbitrary length, the number of dashes gives the style */
  }
  if (n > 0)
    s->dashlength = g_ascii_strtod (dashes[0], NULL);
  if (user_scale > 0)
      s->dashlength /= user_scale;
  switch (n) {
    case 0 :
      s->linestyle = LINESTYLE_SOLID;
      break;
    case 1 :
      s->linestyle = LINESTYLE_DASHED;
      break;
    case 2 :
      dl = g_ascii_strtod (dashes[0], NULL);
      if (user_scale > 0)
        dl /= user_scale;
      if (dl < s->line_width || dl > s->dashlength) { /* the difference is arbitrary */
	s->linestyle = LINESTYLE_DOTTED;
	s->dashlength *= 10.0; /* dot = 10% of len */
      } else {
	s->linestyle = LINESTYLE_DASHED;
      }
      break;
    case 4 :
      s->linestyle = LINESTYLE_DASH_DOT;
      break;
    case 6 : s->linestyle = LINESTYLE_DASH_DOT_DOT;
      break;
    default :
      s->linestyle = LINESTYLE_DOTTED; /* not correct */
      break;
  }
  g_strfreev (dashes);

  if (end)
    *end = ptr;
}

/*
 * \brief Parse SVG/CSS style string
 *
 * Parse as much information from the given style string as Dia can handle.
 * There are still known limitations:
 *  - does not follow references to somewhere inside or outside the string
 *    (e.g. url(...), color and currentColor)
 *  - font parsing should be extended to support lists of fonts somehow
 *
 * \ingroup DiaSvg
 */
void
dia_svg_parse_style_string (DiaSvgStyle *s, real user_scale, const gchar *str)
{
  gchar temp[FONT_NAME_LENGTH_MAX+1]; /* font-family names will be limited to 40 characters */
  int i = 0;
  gboolean over = FALSE;
  gchar *ptr = (gchar *)str;
  char *family = NULL, *style = NULL, *weight = NULL;

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

      if (!over) {
	if (strcmp (temp, "sanserif") == 0 || strcmp (temp, "sans-serif") == 0)
	  family = g_strdup ("sans"); /* special name adaption */
	else
	  family = g_strdup(temp);
      }
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

      if (!_parse_color (&s->stroke, ptr))
	s->stroke = DIA_SVG_COLOUR_NONE;
    } else if (!strncmp("stroke-opacity:", ptr, 15)) {
      ptr += 15;
      s->stroke_opacity = g_ascii_strtod(ptr, &ptr);
    } else if (!strncmp("fill:", ptr, 5)) {
      ptr += 5;
      while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      if (!_parse_color (&s->fill, ptr))
	 s->fill = DIA_SVG_COLOUR_NONE;
    } else if (!strncmp("fill-opacity:", ptr, 13)) {
      ptr += 13;
      s->fill_opacity = g_ascii_strtod(ptr, &ptr);
    } else if (!strncmp("opacity", ptr, 7)) {
      real opacity;
      ptr += 7;
      opacity = g_ascii_strtod(ptr, &ptr);
      /* multiplicative effect of opacity */
      s->stroke_opacity *= opacity;
      s->fill_opacity *= opacity;
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
      s->linestyle = LINESTYLE_DASHED;
      ptr += 17;
      while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      if (!strncmp(ptr, "default", 7))
	s->dashlength = 1.0;
      else
	_parse_dasharray (s, user_scale, ptr, &ptr);
    }

    /* skip up to the next attribute */
    while (ptr[0] != '\0' && ptr[0] != ';' && ptr[0] != '\n') ptr++;
    if (ptr[0] != '\0') ptr++;
  }

  if (family || style || weight) {
    if (s->font)
      dia_font_unref (s->font);
    /* given font_height is bogus, especially if not given at all
     * or without unit ... see bug 665648 about invalid CSS
     */
    s->font = dia_font_new_from_style(DIA_FONT_SANS, s->font_height > 0 ? s->font_height : 1.0);
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


/*
 * \brief Parse SVG style properties
 * This function not only parses the style attribute of the given node
 * it also extracts some of the style properties directly.
 * @param node An XML node to parse a style from.
 * @param s The SVG style object to fill out.  This should previously be
 *          initialized to some default values.
 * @param user_scale, if >0 scalable values (font-size, stroke-width, ...)
 *          are divided by this, otherwise ignored
 * \ingroup DiaSvg
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
    dia_svg_parse_style_string (s, user_scale, (gchar *)str);
    xmlFree(str);
  }

  /* ugly svg variations, it is allowed to give style properties without
   * the style attribute, i.e. direct attributes
   */
  str = xmlGetProp(node, (const xmlChar *)"opacity");
  if (str) {
    real opacity = g_ascii_strtod((char *) str, NULL);
    /* multiplicative effect of opacity */
    s->stroke_opacity *= opacity;
    s->fill_opacity *= opacity;
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"fill");
  if (str) {
    if (!_parse_color (&s->fill, (char *) str))
       s->fill = DIA_SVG_COLOUR_NONE;
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"fill-opacity");
  if (str) {
    s->fill_opacity = g_ascii_strtod((char *) str, NULL);
    xmlFree(str);
  }  
  str = xmlGetProp(node, (const xmlChar *)"stroke");
  if (str) {
    if (!_parse_color (&s->stroke, (char *) str))
       s->stroke = DIA_SVG_COLOUR_NONE;
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"stroke-opacity");
  if (str) {
    s->stroke_opacity = g_ascii_strtod((char *) str, NULL);
    xmlFree(str);
  }  
  str = xmlGetProp(node, (const xmlChar *)"stroke-width");
  if (str) {
    s->line_width = g_ascii_strtod((char *) str, NULL);
    xmlFree(str);
    if (user_scale > 0)
      s->line_width /= user_scale;
  }
  str = xmlGetProp(node, (const xmlChar *)"stroke-dasharray");
  if (str) {
    _parse_dasharray (s, user_scale, (gchar *)str, NULL);
    xmlFree(str);
  }
  /* text-props, again ;( */
  str = xmlGetProp(node, (const xmlChar *)"font-size");
  if (str) {
    s->font_height = g_ascii_strtod((gchar *)str, NULL);
    if (user_scale > 0)
      s->font_height /= user_scale;
    xmlFree(str);
  }
  
}

/*!
 * \brief Parse a SVG description of an arc segment.
 * Code stolen from (and adapted)
 * http://www.inkscape.org/doc/doxygen/html/svg-path_8cpp.php#a7
 * which may have got it from rsvg, hope it is correct ;)
 * @param points destination array of _BezPoint
 * @param xc center x
 * @param yc center y
 * @param th0 first angle
 * @param th1 second angle
 * @param rx radius x
 * @param ry radius y
 * @param x_axis_rotation rotation of the axis
 * @param last_p2 the resulting current point
 * If you want the description of the algorithm read the SVG specs:
 * http://www.w3.org/TR/SVG/paths.html#PathDataEllipticalArcCommands
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

/*!
 * \brief Takes SVG path content and converts it in an array of BezPoint.
 *
 * SVG pathes can contain multiple MOVE_TO commands while Dia's bezier
 * object can only contain one so you may need to call this function
 * multiple times.
 *
 * @param path_str A string describing an SVG path.
 * @param unparsed The position in `path_str' where parsing ended, or NULL if
 *                 the string was completely parsed.  This should be used for
 *                 calling the function until it is fully parsed.
 * @param closed Whether the path was closed.
 * @param current_point to retain it over splitting
 * @return TRUE if there is any useful data in parsed to points
 * @bug This function is way too long (324 lines). So dont touch it. please!
 * Shouldn't we try to turn straight lines, simple arc, polylines and
 * zigzaglines into their appropriate objects?  Could either be done by
 * returning an object or by having functions that try parsing as
 * specific simple paths.
 * NOPE: Dia is capable to handle beziers and the file has given us some so 
 * WHY should be break it in to pieces ???
 */
gboolean
dia_svg_parse_path(GArray *points, const gchar *path_str, gchar **unparsed,
		   gboolean *closed, Point *current_point)
{
  enum {
    PATH_MOVE, PATH_LINE, PATH_HLINE, PATH_VLINE, PATH_CURVE,
    PATH_SMOOTHCURVE, PATH_ARC, PATH_CLOSE } last_type = PATH_MOVE;
  Point last_open = {0.0, 0.0};
  Point last_point = {0.0, 0.0};
  Point last_control = {0.0, 0.0};
  gboolean last_relative = FALSE;
  BezPoint bez = { 0, };
  gchar *path = (gchar *)path_str;
  gboolean need_next_element = FALSE;
  /* we can grow the same array in multiple steps */
  gsize points_at_start = points->len;

  *closed = FALSE;
  *unparsed = NULL;

  /* when splitting into pieces, we have to maintain current_point accross them */
  if (current_point)
    last_point = *current_point;

  path_chomp(path);
  while (path[0] != '\0') {
#ifdef DEBUG_CUSTOM
    g_print("Path: %s\n", path);
#endif
    /* check for a new command */
    switch (path[0]) {
    case 'M':
      if (points->len - points_at_start > 0) {
	need_next_element = TRUE;
	goto MORETOPARSE;
      }
      path++;
      path_chomp(path);
      last_type = PATH_MOVE;
      last_relative = FALSE;
      break;
    case 'm':
      if (points->len - points_at_start > 0) {
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
      if (points->len - points_at_start > 1)
	g_warning ("Only first point should be 'move'");
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
      if (points->len - points_at_start == 1) /* stupid svg, but we can handle it */
	g_array_index(points,BezPoint,0) = bez;
      else
        g_array_append_val(points, bez);
      /* [SVG11 8.3.2] If a moveto is followed by multiple pairs of coordinates, 
       * the subsequent pairs are treated as implicit lineto commands 
       */
      last_type = PATH_LINE;
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
      /* Strictly speeaking it should not be necessary to assign the other 
       * two points. But it helps hiding a serious limitation with the 
       * standard bezier serialization, namely only saving one move-to
       * and the rest as curve-to */
#define INIT_LINE_TO_AS_CURVE_TO bez.p3 = bez.p1; bez.p2 = last_point

      INIT_LINE_TO_AS_CURVE_TO;

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

      INIT_LINE_TO_AS_CURVE_TO;

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

      INIT_LINE_TO_AS_CURVE_TO;

#undef INIT_LINE_TO_AS_CURVE_TO

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

	/* avoid matherr with bogus values - just ignore them
	 * does happen e.g. with 'Chem-Widgets - clamp-large' 
	 */
	if (last_point.x != dest.x || last_point.y != dest.y)
	  _path_arc (points, last_point.x, last_point.y,
		     rx, ry, xrot, largearc, sweep, dest.x, dest.y,
		     &dest_c);
	last_point = dest;
	last_control = dest_c;
      }
      break;
    case PATH_CLOSE:
      /* close the path with a line - second condition to ignore single close */
      if (   (last_open.x != last_point.x || last_open.y != last_point.y)
	  && (points->len != points_at_start)) {
	bez.type = BEZ_LINE_TO;
	bez.p1 = last_open;
	g_array_append_val(points, bez);
	last_point = last_open;
      }
      *closed = TRUE;
      need_next_element = TRUE;
    }
    /* get rid of any ignorable characters */
    path_chomp(path);
MORETOPARSE:
    if (need_next_element) {
      /* check if there really is more to be parsed */
      if (path[0] != 0)
	*unparsed = path;
      else
	*unparsed = NULL;
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
  if (current_point)
    *current_point = last_point;
  return (points->len > 1);
}

DiaMatrix *
dia_svg_parse_transform(const gchar *trans, real scale)
{
  DiaMatrix *m;
  gchar *p = strchr (trans, '(');
  gchar **list;
  int i = 0;

  if (!p)
    return NULL; /* silently ignore broken format */

  m = g_new0 (DiaMatrix, 1);
  list = g_regex_split_simple ("[\\s,]+", p+1, 0, 0);
  if (strncmp (trans, "matrix", 6) == 0) {
    if (list[i])
      m->xx = g_ascii_strtod (list[i], NULL), ++i;
    if (list[i])
      m->yx = g_ascii_strtod (list[i], NULL), ++i;
    if (list[i])
      m->xy = g_ascii_strtod (list[i], NULL), ++i;
    if (list[i])
      m->yy = g_ascii_strtod (list[i], NULL), ++i;
    if (list[i])
      m->x0 = g_ascii_strtod (list[i], NULL), ++i;
    if (list[i])
      m->y0 = g_ascii_strtod (list[i], NULL), ++i;
  } else if (strncmp (trans, "translate", 9) == 0) {
    m->xx = m->yy = 1.0;
    if (list[i])
      m->x0 = g_ascii_strtod (list[i], NULL), ++i;
    if (list[i])
      m->y0 = g_ascii_strtod (list[i], NULL), ++i;
  } else if (strncmp (trans, "scale", 5) == 0) {
    if (list[i])
      m->xx = g_ascii_strtod (list[i], NULL), ++i;
    if (list[i])
      m->yy = g_ascii_strtod (list[i], NULL), ++i;
    else
      m->yy = m->xx;
  } else if (strncmp (trans, "rotate", 6) == 0) {
    real angle;
    
    if (list[i])
      angle = g_ascii_strtod (list[i], NULL), ++i;
    else {
      angle = 0;
      g_warning ("transform=rotate no angle?");
    }
    m->xx =  cos(G_PI*angle/180);
    /* FIXME: swapped xy and yx - correct? */
    m->xy = -sin(G_PI*angle/180);
    m->yx =  sin(G_PI*angle/180);
    m->yy =  cos(G_PI*angle/180);
    /* FIXME: check with real world data, I'm uncertain */
    if (list[i])
      m->x0 = g_ascii_strtod (list[i], NULL), ++i;
    if (list[i])
      m->y0 = g_ascii_strtod (list[i], NULL), ++i;
  } else if (strncmp (trans, "skewX", 5) == 0) {
    m->xx = m->yy = 1.0;
    if (list[i])
      m->yx = tan (G_PI*g_ascii_strtod (list[i], NULL)/180);
  } else if (strncmp (trans, "skewY", 5) == 0) {
    m->xx = m->yy = 1.0;
    if (list[i])
      m->xy = tan (G_PI*g_ascii_strtod (list[i], NULL)/180);
  } else {
    g_warning ("SVG: %s?", trans);
    g_free (m);
    m = NULL;
  }
  if (scale > 0 && m) {
    m->x0 /= scale;
    m->y0 /= scale;
  }
  g_strfreev(list);
  return m;
}

gchar *
dia_svg_from_matrix(const DiaMatrix *matrix, real scale)
{
  /*  transform="matrix(1,0,0,1,0,0)" */
  gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
  GString *sm = g_string_new ("matrix(");
  gchar *s;

  g_ascii_formatd (buf, sizeof(buf), "%g", matrix->xx);
  g_string_append (sm, buf); g_string_append (sm, ",");
  g_ascii_formatd (buf, sizeof(buf), "%g", matrix->yx);
  g_string_append (sm, buf); g_string_append (sm, ",");
  g_ascii_formatd (buf, sizeof(buf), "%g", matrix->xy);
  g_string_append (sm, buf); g_string_append (sm, ",");
  g_ascii_formatd (buf, sizeof(buf), "%g", matrix->yy);
  g_string_append (sm, buf); g_string_append (sm, ",");
  g_ascii_formatd (buf, sizeof(buf), "%g", matrix->x0 * scale);
  g_string_append (sm, buf); g_string_append (sm, ",");
  g_ascii_formatd (buf, sizeof(buf), "%g", matrix->y0 * scale);
  g_string_append (sm, buf); g_string_append (sm, ")");
  
  s = sm->str;
  g_string_free (sm, FALSE);
  return s;
}
