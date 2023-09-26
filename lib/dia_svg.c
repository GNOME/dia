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

/**
 * SECTION:dia_svg
 * @title: Dia SVG
 * @short_description: A set of function helping to read and write SVG with Dia
 *
 * The Dia application supports various variants of SVG. There are
 * at least two importers of SVG dialects, namely \ref Shapes and
 * the standard SVG importer \ref Plugins. Both are using theses
 * services to a large extend, but they are also doing there own
 * thing regarding the SVG dialect interpretation.
 */

/*!
 * A global variable to be accessed by "currentColor"
 */
static int _current_color = 0xFF000000;

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
  gs->linestyle = parent_style ? parent_style->linestyle : DIA_LINE_STYLE_SOLID;
  gs->dashlength = parent_style ? parent_style->dashlength : 1;
  /* http://www.w3.org/TR/SVG/painting.html#FillProperty - default black
   * but we still have to see the difference
   */
  gs->fill = parent_style ? parent_style->fill : DIA_SVG_COLOUR_DEFAULT;
  gs->fill_opacity = parent_style ? parent_style->fill_opacity : 1.0;
  gs->linecap = parent_style ? parent_style->linecap : DIA_LINE_CAPS_DEFAULT;
  gs->linejoin = parent_style ? parent_style->linejoin : DIA_LINE_JOIN_DEFAULT;
  gs->linestyle = parent_style ? parent_style->linestyle : DIA_LINE_STYLE_DEFAULT;
  gs->font = (parent_style && parent_style->font) ? g_object_ref (parent_style->font) : NULL;
  gs->font_height = parent_style ? parent_style->font_height : 0.8;
  gs->alignment = parent_style ? parent_style->alignment : DIA_ALIGN_LEFT;

  gs->stop_color = parent_style ? parent_style->stop_color : 0x000000; /* default black */
  gs->stop_opacity = parent_style ? parent_style->stop_opacity : 1.0;
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
  g_clear_object (&dest->font);
  dest->font = src->font ? g_object_ref (src->font) : NULL;
  dest->font_height = src->font_height;
  dest->alignment = src->alignment;
  dest->stop_color = src->stop_color;
  dest->stop_opacity = src->stop_opacity;
}

static const struct _SvgNamedColor {
  const gint  value;
  const char *name;
} _svg_named_colors [] = {
  /* complete list copied from sodipodi source */
	{ 0xF0F8FF, "aliceblue" },
	{ 0xFAEBD7, "antiquewhite" },
	{ 0x00FFFF, "aqua" },
	{ 0x7FFFD4, "aquamarine" },
	{ 0xF0FFFF, "azure" },
	{ 0xF5F5DC, "beige" },
	{ 0xFFE4C4, "bisque" },
	{ 0x000000, "black" },
	{ 0xFFEBCD, "blanchedalmond" },
	{ 0x0000FF, "blue" },
	{ 0x8A2BE2, "blueviolet" },
	{ 0xA52A2A, "brown" },
	{ 0xDEB887, "burlywood" },
	{ 0x5F9EA0, "cadetblue" },
	{ 0x7FFF00, "chartreuse" },
	{ 0xD2691E, "chocolate" },
	{ 0xFF7F50, "coral" },
	{ 0x6495ED, "cornflowerblue" },
	{ 0xFFF8DC, "cornsilk" },
	{ 0xDC143C, "crimson" },
	{ 0x00FFFF, "cyan" },
	{ 0x00008B, "darkblue" },
	{ 0x008B8B, "darkcyan" },
	{ 0xB8860B, "darkgoldenrod" },
	{ 0xA9A9A9, "darkgray" },
	{ 0x006400, "darkgreen" },
	{ 0xA9A9A9, "darkgrey" },
	{ 0xBDB76B, "darkkhaki" },
	{ 0x8B008B, "darkmagenta" },
	{ 0x556B2F, "darkolivegreen" },
	{ 0xFF8C00, "darkorange" },
	{ 0x9932CC, "darkorchid" },
	{ 0x8B0000, "darkred" },
	{ 0xE9967A, "darksalmon" },
	{ 0x8FBC8F, "darkseagreen" },
	{ 0x483D8B, "darkslateblue" },
	{ 0x2F4F4F, "darkslategray" },
	{ 0x2F4F4F, "darkslategrey" },
	{ 0x00CED1, "darkturquoise" },
	{ 0x9400D3, "darkviolet" },
	{ 0xFF1493, "deeppink" },
	{ 0x00BFFF, "deepskyblue" },
	{ 0x696969, "dimgray" },
	{ 0x696969, "dimgrey" },
	{ 0x1E90FF, "dodgerblue" },
	{ 0xB22222, "firebrick" },
	{ 0xFFFAF0, "floralwhite" },
	{ 0x228B22, "forestgreen" },
	{ 0xFF00FF, "fuchsia" },
	{ 0xDCDCDC, "gainsboro" },
	{ 0xF8F8FF, "ghostwhite" },
	{ 0xFFD700, "gold" },
	{ 0xDAA520, "goldenrod" },
	{ 0x808080, "gray" },
	{ 0x008000, "green" },
	{ 0xADFF2F, "greenyellow" },
	{ 0x808080, "grey" },
	{ 0xF0FFF0, "honeydew" },
	{ 0xFF69B4, "hotpink" },
	{ 0xCD5C5C, "indianred" },
	{ 0x4B0082, "indigo" },
	{ 0xFFFFF0, "ivory" },
	{ 0xF0E68C, "khaki" },
	{ 0xE6E6FA, "lavender" },
	{ 0xFFF0F5, "lavenderblush" },
	{ 0x7CFC00, "lawngreen" },
	{ 0xFFFACD, "lemonchiffon" },
	{ 0xADD8E6, "lightblue" },
	{ 0xF08080, "lightcoral" },
	{ 0xE0FFFF, "lightcyan" },
	{ 0xFAFAD2, "lightgoldenrodyellow" },
	{ 0xD3D3D3, "lightgray" },
	{ 0x90EE90, "lightgreen" },
	{ 0xD3D3D3, "lightgrey" },
	{ 0xFFB6C1, "lightpink" },
	{ 0xFFA07A, "lightsalmon" },
	{ 0x20B2AA, "lightseagreen" },
	{ 0x87CEFA, "lightskyblue" },
	{ 0x778899, "lightslategray" },
	{ 0x778899, "lightslategrey" },
	{ 0xB0C4DE, "lightsteelblue" },
	{ 0xFFFFE0, "lightyellow" },
	{ 0x00FF00, "lime" },
	{ 0x32CD32, "limegreen" },
	{ 0xFAF0E6, "linen" },
	{ 0xFF00FF, "magenta" },
	{ 0x800000, "maroon" },
	{ 0x66CDAA, "mediumaquamarine" },
	{ 0x0000CD, "mediumblue" },
	{ 0xBA55D3, "mediumorchid" },
	{ 0x9370DB, "mediumpurple" },
	{ 0x3CB371, "mediumseagreen" },
	{ 0x7B68EE, "mediumslateblue" },
	{ 0x00FA9A, "mediumspringgreen" },
	{ 0x48D1CC, "mediumturquoise" },
	{ 0xC71585, "mediumvioletred" },
	{ 0x191970, "midnightblue" },
	{ 0xF5FFFA, "mintcream" },
	{ 0xFFE4E1, "mistyrose" },
	{ 0xFFE4B5, "moccasin" },
	{ 0xFFDEAD, "navajowhite" },
	{ 0x000080, "navy" },
	{ 0xFDF5E6, "oldlace" },
	{ 0x808000, "olive" },
	{ 0x6B8E23, "olivedrab" },
	{ 0xFFA500, "orange" },
	{ 0xFF4500, "orangered" },
	{ 0xDA70D6, "orchid" },
	{ 0xEEE8AA, "palegoldenrod" },
	{ 0x98FB98, "palegreen" },
	{ 0xAFEEEE, "paleturquoise" },
	{ 0xDB7093, "palevioletred" },
	{ 0xFFEFD5, "papayawhip" },
	{ 0xFFDAB9, "peachpuff" },
	{ 0xCD853F, "peru" },
	{ 0xFFC0CB, "pink" },
	{ 0xDDA0DD, "plum" },
	{ 0xB0E0E6, "powderblue" },
	{ 0x800080, "purple" },
	{ 0xFF0000, "red" },
	{ 0xBC8F8F, "rosybrown" },
	{ 0x4169E1, "royalblue" },
	{ 0x8B4513, "saddlebrown" },
	{ 0xFA8072, "salmon" },
	{ 0xF4A460, "sandybrown" },
	{ 0x2E8B57, "seagreen" },
	{ 0xFFF5EE, "seashell" },
	{ 0xA0522D, "sienna" },
	{ 0xC0C0C0, "silver" },
	{ 0x87CEEB, "skyblue" },
	{ 0x6A5ACD, "slateblue" },
	{ 0x708090, "slategray" },
	{ 0x708090, "slategrey" },
	{ 0xFFFAFA, "snow" },
	{ 0x00FF7F, "springgreen" },
	{ 0x4682B4, "steelblue" },
	{ 0xD2B48C, "tan" },
	{ 0x008080, "teal" },
	{ 0xD8BFD8, "thistle" },
	{ 0xFF6347, "tomato" },
	{ 0x40E0D0, "turquoise" },
	{ 0xEE82EE, "violet" },
	{ 0xF5DEB3, "wheat" },
	{ 0xFFFFFF, "white" },
	{ 0xF5F5F5, "whitesmoke" },
	{ 0xFFFF00, "yellow" },
	{ 0x9ACD32, "yellowgreen" }
};

static int
_cmp_color (const void *key, const void *elem)
{
  const char *a = key;
  const struct _SvgNamedColor *color = elem;

  return strcmp (a, color->name);
}

/*!
 * \brief Get an SVG color value by name
 *
 * The list of named SVG tiny colors has only 17 entries according to
 * http://www.w3.org/TR/CSS21/syndata.html#color-units
 * Still pango_color_parse() does not support seven of them including
 * 'white'. This function supports all of them.
 * Against the SVG full specification (and the SVG Test Suite) pango's
 * long list is still missing colors, e.g. crimson. So this function is
 * using a supposed to be complete internal list.
 *
 * \ingroup DiaSvg
 */
static gboolean
svg_named_color (const char *name, gint32 *color)
{
  const struct _SvgNamedColor *elem;

  g_return_val_if_fail (name != NULL && color != NULL, FALSE);

  elem = bsearch (name, _svg_named_colors,
		  G_N_ELEMENTS(_svg_named_colors), sizeof(_svg_named_colors[0]),
		  _cmp_color);
  if (elem) {
    *color = elem->value;
      return TRUE;
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
  else if (0 == strcmp(str, "currentColor"))
    *color = _current_color;
  else if (0 == strncmp(str, "rgb(", 4)) {
    int r = 0, g = 0, b = 0;
    if (3 == sscanf (str+4, "%d,%d,%d", &r, &g, &b)) {
      /* Set alpha to 1.0 */
      *color = ((0xFF<<24) & 0xFF000000) | ((r<<16) & 0xFF0000) | ((g<<8) & 0xFF00) | (b & 0xFF);
    } else if (strchr (str+4, '%')) {
      /* e.g. cairo uses percent values */
      char **vals = g_strsplit (str+4, "%,", -1);
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
  } else {
    char* se = strchr (str, ';');
    if (!se) /* style might have trailign space */
      se = strchr (str, ' ');

    if (!se) {
      return svg_named_color (str, color);
    } else {
      /* need to make a copy of the color only */
      gboolean ret;
      char *sc = g_strndup (str, se - str);
      ret = svg_named_color (sc, color);
      g_clear_pointer (&sc, g_free);
      return ret;
    }
  }
  return TRUE;
}

/*!
 * \brief Convert a string to a color
 *
 * SVG spec also allows 'inherit' as color value, which leads
 * to false here. Should still mostly work because the color
 * is supposed to be initialized before.
 *
 * \ingroup DiaSvg
 */
gboolean
dia_svg_parse_color (const char *str, Color *color)
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
_parse_dasharray (DiaSvgStyle *s, double user_scale, char *str, char **end)
{
  char *ptr;
  /* by also splitting on ';' we can also parse the continued style string */
  char **dashes = g_regex_split_simple ("[\\s,;]+", (char *) str, 0, 0);
  int n = 0;
  double dl;

  s->dashlength = g_ascii_strtod(str, &ptr);
  if (s->dashlength <= 0.0) {
    /* e.g. "none" */
    s->linestyle = DIA_LINE_STYLE_SOLID;
  } else if (user_scale > 0) {
    s->dashlength /= user_scale;
  }

  if (s->dashlength) { /* at least one value */
    while (dashes[n] && g_ascii_strtod (dashes[n], NULL) > 0) {
      ++n; /* Dia can not do arbitrary length, the number of dashes gives the style */
    }
  }

  if (n > 0) {
    s->dashlength = g_ascii_strtod (dashes[0], NULL);
  }

  if (user_scale > 0) {
      s->dashlength /= user_scale;
  }

  switch (n) {
    case 0:
      s->linestyle = DIA_LINE_STYLE_SOLID;
      break;
    case 1:
      s->linestyle = DIA_LINE_STYLE_DASHED;
      break;
    case 2:
      dl = g_ascii_strtod (dashes[1], NULL);

      if (user_scale > 0) {
        dl /= user_scale;
      }

      if (dl < s->line_width || dl > s->dashlength) {
        /* the difference is arbitrary */
        s->linestyle = DIA_LINE_STYLE_DOTTED;
        s->dashlength *= 10.0; /* dot = 10% of len */
      } else {
        s->linestyle = DIA_LINE_STYLE_DASHED;
      }
      break;
    case 4:
      s->linestyle = DIA_LINE_STYLE_DASH_DOT;
      break;
    default :
      /* If an odd number of values is provided, then the list of values is repeated to
       * yield an even number of values. Thus, stroke-dasharray: 5,3,2 is equivalent to
       * stroke-dasharray: 5,3,2,5,3,2.
       */
    case 6:
      s->linestyle = DIA_LINE_STYLE_DASH_DOT_DOT;
      break;
  }
  g_strfreev (dashes);

  if (end)
    *end = ptr;
}


static void
_parse_linejoin (DiaSvgStyle *s, const char *val)
{
  if (!strncmp (val, "miter", 5)) {
    s->linejoin = DIA_LINE_JOIN_MITER;
  } else if (!strncmp (val, "round", 5)) {
    s->linejoin = DIA_LINE_JOIN_ROUND;
  } else if (!strncmp (val, "bevel", 5)) {
    s->linejoin = DIA_LINE_JOIN_BEVEL;
  } else if (!strncmp (val, "default", 7)) {
    s->linejoin = DIA_LINE_JOIN_DEFAULT;
  }
}


static void
_parse_linecap (DiaSvgStyle *s, const char *val)
{
  if (!strncmp(val, "butt", 4))
    s->linecap = DIA_LINE_CAPS_BUTT;
  else if (!strncmp(val, "round", 5))
    s->linecap = DIA_LINE_CAPS_ROUND;
  else if (!strncmp(val, "square", 6) || !strncmp(val, "projecting", 10))
    s->linecap = DIA_LINE_CAPS_PROJECTING;
  else if (!strncmp(val, "default", 7))
    s->linecap = DIA_LINE_CAPS_DEFAULT;
}

/*!
 * \brief Given any of the three parameters adjust the font
 * @param s      Dia SVG style to modify
 * @param family comma-separated list of family names
 * @param style  font slant string
 * @param weight font weight string
 */
static void
_style_adjust_font (DiaSvgStyle *s, const char *family, const char *style, const char *weight)
{
  g_clear_object (&s->font);
  /* given font_height is bogus, especially if not given at all
    * or without unit ... see bug 665648 about invalid CSS
    */
  s->font = dia_font_new_from_style (DIA_FONT_SANS, s->font_height > 0 ? s->font_height : 1.0);
  if (family) {
    /* SVG allows a list of families here, also there is some strange formatting
      * seen, like 'Arial'. If the given family name can not be resolved by
      * Pango it complaints loudly with g_warning().
      */
    char **families = g_strsplit (family, ",", -1);
    int i = 0;
    gboolean found = FALSE;
    while (!found && families[i]) {
      const char *chomped = g_strchomp (g_strdelimit (families[i], "'", ' '));
      PangoFont *loaded;

      dia_font_set_any_family(s->font, chomped);
      loaded = pango_context_load_font (dia_font_get_context (),
                                        dia_font_get_description (s->font));
      if (loaded) {
        g_clear_object (&loaded);
        found = TRUE;
      }
      ++i;
    }

    if (!found) {
      dia_font_set_any_family (s->font, "sans");
    }

    g_strfreev (families);
  }

  if (style) {
    dia_font_set_slant_from_string (s->font, style);
  }

  if (weight) {
    dia_font_set_weight_from_string (s->font, weight);
  }
}


static void
_parse_text_align (DiaSvgStyle *s, const char *ptr)
{
  if (!strncmp (ptr, "start", 5)) {
    s->alignment = DIA_ALIGN_LEFT;
  } else if (!strncmp (ptr, "end", 3)) {
    s->alignment = DIA_ALIGN_RIGHT;
  } else if (!strncmp (ptr, "middle", 6)) {
    s->alignment = DIA_ALIGN_CENTRE;
  }
}


/*!
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
dia_svg_parse_style_string (DiaSvgStyle *s, double user_scale, const char *str)
{
  int i = 0;
  char *ptr = (char *) str;
  char *family = NULL, *style = NULL, *weight = NULL;

  while (ptr[0] != '\0') {
    /* skip white space at start */
    while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
    if (ptr[0] == '\0') break;

    if (!strncmp("font-family:", ptr, 12)) {
      ptr += 12;
      while ((ptr[0] != '\0') && g_ascii_isspace(ptr[0])) ptr++;
      i = 0;
      while (ptr[i] != '\0' && ptr[i] != ';') ++i;
      /* with i==0 we fall back to 'sans' too */
      if (strncmp (ptr, "sanserif", i) == 0 || strncmp (ptr, "sans-serif", i) == 0)
        family = g_strdup ("sans"); /* special name adaption */
      else
        family = i > 0 ? g_strndup(ptr, i) : NULL;
      ptr += i;
    } else if (!strncmp("font-weight:", ptr, 12)) {
      ptr += 12;
      while ((ptr[0] != '\0') && g_ascii_isspace(ptr[0])) ptr++;
      i = 0;
      while (ptr[i] != '\0' && ptr[i] != ';') ++i;
      weight = i > 0 ? g_strndup (ptr, i) : NULL;
      ptr += i;
    } else if (!strncmp("font-style:", ptr, 11)) {
      ptr += 11;
      while ((ptr[0] != '\0') && g_ascii_isspace(ptr[0])) ptr++;
      i = 0;
      while (ptr[i] != '\0' && ptr[i] != ';') ++i;
      style = i > 0 ? g_strndup(ptr, i) : NULL;
      ptr += i;
    } else if (!strncmp("font-size:", ptr, 10)) {
      ptr += 10;
      while ((ptr[0] != '\0') && g_ascii_isspace(ptr[0])) ptr++;
      i = 0;
      while (ptr[i] != '\0' && ptr[i] != ';') ++i;
      s->font_height = g_ascii_strtod(ptr, NULL);
      ptr += i;
      if (user_scale > 0)
	s->font_height /= user_scale;
    } else if (!strncmp("text-anchor:", ptr, 12)) {
      ptr += 12;
      while ((ptr[0] != '\0') && g_ascii_isspace(ptr[0])) ptr++;

      _parse_text_align(s, ptr);
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
    } else if (!strncmp("stop-color:", ptr, 11)) {
      ptr += 11;
      while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      if (!_parse_color (&s->stop_color, ptr))
	 s->stop_color = DIA_SVG_COLOUR_NONE;
    } else if (!strncmp("stop-opacity:", ptr, 13)) {
      ptr += 13;
      s->stop_opacity = g_ascii_strtod(ptr, &ptr);
    } else if (!strncmp("opacity", ptr, 7)) {
      double opacity;
      ptr += 7;
      opacity = g_ascii_strtod(ptr, &ptr);
      /* multiplicative effect of opacity */
      s->stroke_opacity *= opacity;
      s->fill_opacity *= opacity;
    } else if (!strncmp("stroke-linecap:", ptr, 15)) {
      ptr += 15;
      while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      _parse_linecap (s, ptr);
    } else if (!strncmp("stroke-linejoin:", ptr, 16)) {
      ptr += 16;
      while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      _parse_linejoin (s, ptr);
    } else if (!strncmp("stroke-pattern:", ptr, 15)) {
      /* Apparently not an offical SVG style attribute, but
       * referenced in custom-shapes document. So we continue
       * supporting it (read only).
       */
      ptr += 15;
      while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      if (!strncmp (ptr, "solid", 5)) {
        s->linestyle = DIA_LINE_STYLE_SOLID;
      } else if (!strncmp (ptr, "dashed", 6)) {
        s->linestyle = DIA_LINE_STYLE_DASHED;
      } else if (!strncmp (ptr, "dash-dot", 8)) {
        s->linestyle = DIA_LINE_STYLE_DASH_DOT;
      } else if (!strncmp (ptr, "dash-dot-dot", 12)) {
        s->linestyle = DIA_LINE_STYLE_DASH_DOT_DOT;
      } else if (!strncmp (ptr, "dotted", 6)) {
        s->linestyle = DIA_LINE_STYLE_DOTTED;
      } else if (!strncmp (ptr, "default", 7)) {
        s->linestyle = DIA_LINE_STYLE_DEFAULT;
      }
      /* XXX: deal with a real pattern */
    } else if (!strncmp ("stroke-dashlength:", ptr, 18)) {
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
    } else if (!strncmp ("stroke-dasharray:", ptr, 17)) {
      s->linestyle = DIA_LINE_STYLE_DASHED;
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
    _style_adjust_font (s, family, style, weight);

    g_clear_pointer (&family, g_free);
    g_clear_pointer (&style, g_free);
    g_clear_pointer (&weight, g_free);
  }
}


/*!
 * \brief Parse SVG style properties
 *
 * This function not only parses the style attribute of the given node
 * it also extracts some of the style properties directly.
 * @param node An XML node to parse a style from.
 * @param s The SVG style object to fill out.  This should previously be
 *          initialized to some default values.
 * @param user_scale if >0 scalable values (font-size, stroke-width, ...)
 *          are divided by this, otherwise ignored
 *
 * \ingroup DiaSvg
 */
void
dia_svg_parse_style (xmlNodePtr node, DiaSvgStyle *s, double user_scale)
{
  xmlChar *str;

  str = xmlGetProp(node, (const xmlChar *)"style");

  if (str) {
    dia_svg_parse_style_string (s, user_scale, (char *)str);
    xmlFree(str);
  }

  /* ugly svg variations, it is allowed to give style properties without
   * the style attribute, i.e. direct attributes
   */
  str = xmlGetProp(node, (const xmlChar *)"color");
  if (str) {
    int c;
    if (_parse_color (&c, (char *) str))
      _current_color = c;
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"opacity");
  if (str) {
    double opacity = g_ascii_strtod ((char *) str, NULL);
    /* multiplicative effect of opacity */
    s->stroke_opacity *= opacity;
    s->fill_opacity *= opacity;
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"stop-color");
  if (str) {
    if (!_parse_color (&s->stop_color, (char *) str) && strcmp ((const char *) str, "inherit") != 0)
      s->stop_color = DIA_SVG_COLOUR_NONE;
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"stop-opacity");
  if (str) {
    s->stop_opacity = g_ascii_strtod((char *) str, NULL);
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"fill");
  if (str) {
    if (!_parse_color (&s->fill, (char *) str) && strcmp ((const char *) str, "inherit") != 0)
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
    if (!_parse_color (&s->stroke, (char *) str) && strcmp ((const char *) str, "inherit") != 0)
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
    if (strcmp ((const char *)str, "inherit") != 0)
      _parse_dasharray (s, user_scale, (char *)str, NULL);
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"stroke-linejoin");
  if (str) {
    if (strcmp ((const char *)str, "inherit") != 0)
      _parse_linejoin (s, (char *)str);
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"stroke-linecap");
  if (str) {
    if (strcmp ((const char *)str, "inherit") != 0)
      _parse_linecap (s, (char *)str);
    xmlFree(str);
  }

  /* text-props, again ;( */
  str = xmlGetProp(node, (const xmlChar *)"font-size");
  if (str) {
    /* for inherit we just leave the original value,
     * should be initialized by parent style already
     */
    if (strcmp ((const char *)str, "inherit") != 0) {
      s->font_height = g_ascii_strtod ((char *)str, NULL);
      if (user_scale > 0)
	s->font_height /= user_scale;
    }
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"text-anchor");
  if (str) {
    _parse_text_align (s, (const char*) str);
    xmlFree(str);
  }
  {
    xmlChar *family = xmlGetProp(node, (const xmlChar *)"font-family");
    xmlChar *slant  = xmlGetProp(node, (const xmlChar *)"font-style");
    xmlChar *weight = xmlGetProp(node, (const xmlChar *)"font-weight");
    if (family || slant || weight) {
      _style_adjust_font (s, (char *)family, (char *)slant, (char *)weight);

      if (family)
        xmlFree(family);
      if (slant)
        xmlFree(slant);
      if (weight)
        xmlFree(weight);
    }
  }
}


/**
 * _path_arc_segment:
 * @points: destination array of #BezPoint
 * @xc: center x
 * @yc: center y
 * @th0: first angle
 * @th1: second angle
 * @rx: radius x
 * @ry: radius y
 * @x_axis_rotation: rotation of the axis
 * @last_p2: the resulting current point
 *
 * Parse a SVG description of an arc segment.
 *
 * Code stolen from (and adapted)
 * http://www.inkscape.org/doc/doxygen/html/svg-path_8cpp.php#a7
 * which may have got it from rsvg, hope it is correct ;)
 *
 * If you want the description of the algorithm read the SVG specs:
 * http://www.w3.org/TR/SVG/paths.html#PathDataEllipticalArcCommands
 */
static void
_path_arc_segment (GArray *points,
                   double  xc,
                   double  yc,
                   double  th0,
                   double  th1,
                   double  rx,
                   double  ry,
                   double  x_axis_rotation,
                   Point  *last_p2)
{
  BezPoint bez;
  double sin_th, cos_th;
  double a00, a01, a10, a11;
  double x1, y1, x2, y2, x3, y3;
  double t;
  double th_half;

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


/*
 * Parse an SVG description of a full arc.
 */
static void
_path_arc (GArray *points,
           double  cpx,
           double  cpy,
           double  rx,
           double  ry,
           double  x_axis_rotation,
           int     large_arc_flag,
           int     sweep_flag,
           double  x,
           double  y,
           Point  *last_p2)
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

/**
 * dia_svg_parse_path:
 * @path_str: A string describing an SVG path.
 * @unparsed: The position in @path_str where parsing ended, or %NULL if
 *            the string was completely parsed. This should be used for
 *            calling the function until it is fully parsed.
 * @closed: Whether the path was closed.
 * @current_point: to retain it over splitting
 *
 * Takes SVG path content and converts it in an array of BezPoint.
 *
 * SVG pathes can contain multiple MOVE_TO commands while Dia's bezier
 * object can only contain one so you may need to call this function
 * multiple times.
 *
 * @bug This function is way too long (324 lines). So dont touch it. please!
 * Shouldn't we try to turn straight lines, simple arc, polylines and
 * zigzaglines into their appropriate objects?  Could either be done by
 * returning an object or by having functions that try parsing as
 * specific simple paths.
 * NOPE: Dia is capable to handle beziers and the file has given us some so
 * WHY should be break it in to pieces ???
 *
 * Returns: %TRUE if there is any useful data in parsed to points
 */
gboolean
dia_svg_parse_path (GArray      *points,
                    const char  *path_str,
                    char       **unparsed,
                    gboolean    *closed,
                    Point       *current_point)
{
  enum {
    PATH_MOVE, PATH_LINE, PATH_HLINE, PATH_VLINE, PATH_CURVE,
    PATH_SMOOTHCURVE, PATH_QUBICCURVE, PATH_TTQCURVE,
    PATH_ARC, PATH_CLOSE, PATH_END } last_type = PATH_MOVE;
  Point last_open = {0.0, 0.0};
  Point last_point = {0.0, 0.0};
  Point last_control = {0.0, 0.0};
  gboolean last_relative = FALSE;
  BezPoint bez = { 0, };
  char *path = (char *)path_str;
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
    g_printerr ("Path: %s\n", path);
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
    case 'q':
      path++;
      path_chomp(path);
      last_type = PATH_QUBICCURVE;
      last_relative = TRUE;
      break;
    case 'Q':
      path++;
      path_chomp(path);
      last_type = PATH_QUBICCURVE;
      last_relative = FALSE;
      break;
    case 't':
      path++;
      path_chomp(path);
      last_type = PATH_TTQCURVE;
      last_relative = TRUE;
      break;
    case 'T':
      path++;
      path_chomp(path);
      last_type = PATH_TTQCURVE;
      last_relative = FALSE;
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
	while (path != NULL && strchr("0123456789.+-", path[0])) path++;
	path_chomp(path);
	*closed = TRUE;
	need_next_element = TRUE;
	goto MORETOPARSE;
      }
      break;
    default:
      g_warning("unsupported path code '%c'", path[0]);
      last_type = PATH_END;
      path++;
      path_chomp(path);
      break;
    }

    /* actually parse the path component */
    switch (last_type) {
      case PATH_MOVE:
        if (points->len - points_at_start > 1) {
          g_warning ("Only first point should be 'move'");
        }
        bez.type = BEZ_MOVE_TO;
        bez.p1.x = g_ascii_strtod (path, &path);
        path_chomp (path);
        bez.p1.y = g_ascii_strtod (path, &path);
        path_chomp (path);
        if (last_relative) {
          bez.p1.x += last_point.x;
          bez.p1.y += last_point.y;
        }
        last_point = bez.p1;
        last_control = bez.p1;
        last_open = bez.p1;
        if (points->len - points_at_start == 1) {
          /* stupid svg, but we can handle it */
          g_array_index (points, BezPoint, 0) = bez;
        } else {
          g_array_append_val (points, bez);
        }
        /* [SVG11 8.3.2] If a moveto is followed by multiple pairs of coordinates,
        * the subsequent pairs are treated as implicit lineto commands
        */
        last_type = PATH_LINE;
        break;
      case PATH_LINE:
        bez.type = BEZ_LINE_TO;
        bez.p1.x = g_ascii_strtod (path, &path);
        path_chomp (path);
        bez.p1.y = g_ascii_strtod (path, &path);
        path_chomp (path);
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

        g_array_append_val (points, bez);
        break;
      case PATH_HLINE:
        bez.type = BEZ_LINE_TO;
        bez.p1.x = g_ascii_strtod (path, &path);
        path_chomp (path);
        bez.p1.y = last_point.y;
        if (last_relative) {
          bez.p1.x += last_point.x;
        }

        INIT_LINE_TO_AS_CURVE_TO;

        last_point = bez.p1;
        last_control = bez.p1;

        g_array_append_val (points, bez);
        break;
      case PATH_VLINE:
        bez.type = BEZ_LINE_TO;
        bez.p1.x = last_point.x;
        bez.p1.y = g_ascii_strtod (path, &path);
        path_chomp (path);
        if (last_relative) {
          bez.p1.y += last_point.y;
        }

        INIT_LINE_TO_AS_CURVE_TO;

  #undef INIT_LINE_TO_AS_CURVE_TO

        last_point = bez.p1;
        last_control = bez.p1;

        g_array_append_val (points, bez);
        break;
      case PATH_CURVE:
        bez.type = BEZ_CURVE_TO;
        bez.p1.x = g_ascii_strtod (path, &path);
        path_chomp (path);
        bez.p1.y = g_ascii_strtod (path, &path);
        path_chomp (path);
        bez.p2.x = g_ascii_strtod (path, &path);
        path_chomp (path);
        bez.p2.y = g_ascii_strtod (path, &path);
        path_chomp (path);
        bez.p3.x = g_ascii_strtod (path, &path);
        path_chomp (path);
        bez.p3.y = g_ascii_strtod (path, &path);
        path_chomp (path);
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

        g_array_append_val (points, bez);
        break;
      case PATH_SMOOTHCURVE:
        bez.type = BEZ_CURVE_TO;
        bez.p1.x = 2 * last_point.x - last_control.x;
        bez.p1.y = 2 * last_point.y - last_control.y;
        bez.p2.x = g_ascii_strtod (path, &path);
        path_chomp (path);
        bez.p2.y = g_ascii_strtod (path, &path);
        path_chomp (path);
        bez.p3.x = g_ascii_strtod (path, &path);
        path_chomp (path);
        bez.p3.y = g_ascii_strtod (path, &path);
        path_chomp (path);
        if (last_relative) {
          bez.p2.x += last_point.x;
          bez.p2.y += last_point.y;
          bez.p3.x += last_point.x;
          bez.p3.y += last_point.y;
        }
        last_point = bez.p3;
        last_control = bez.p2;

        g_array_append_val (points, bez);
        break;
      case PATH_QUBICCURVE: {
          /* raise quadratic bezier to cubic (copied from librsvg) */
          double x1, y1;
          x1 = g_ascii_strtod (path, &path);
          path_chomp (path);
          y1 = g_ascii_strtod (path, &path);
          path_chomp (path);
          if (last_relative) {
            x1 += last_point.x;
            y1 += last_point.y;
          }
          bez.type = BEZ_CURVE_TO;
          bez.p1.x = (last_point.x + 2 * x1) * (1.0 / 3.0);
          bez.p1.y = (last_point.y + 2 * y1) * (1.0 / 3.0);
          bez.p3.x = g_ascii_strtod (path, &path);
          path_chomp (path);
          bez.p3.y = g_ascii_strtod (path, &path);
          path_chomp (path);
          if (last_relative) {
            bez.p3.x += last_point.x;
            bez.p3.y += last_point.y;
          }
          bez.p2.x = (bez.p3.x + 2 * x1) * (1.0 / 3.0);
          bez.p2.y = (bez.p3.y + 2 * y1) * (1.0 / 3.0);
          last_point = bez.p3;
          last_control.x = x1;
          last_control.y = y1;
          g_array_append_val (points, bez);
        }
        break;
      case PATH_TTQCURVE:
        {
          /* Truetype quadratic bezier curveto */
          double xc, yc; /* quadratic control point */

          xc = 2 * last_point.x - last_control.x;
          yc = 2 * last_point.y - last_control.y;
          /* generate a quadratic bezier with control point = xc, yc */
          bez.type = BEZ_CURVE_TO;
          bez.p1.x = (last_point.x + 2 * xc) * (1.0 / 3.0);
          bez.p1.y = (last_point.y + 2 * yc) * (1.0 / 3.0);
          bez.p3.x = g_ascii_strtod (path, &path);
          path_chomp (path);
          bez.p3.y = g_ascii_strtod (path, &path);
          path_chomp (path);
          if (last_relative) {
            bez.p3.x += last_point.x;
            bez.p3.y += last_point.y;
          }
          bez.p2.x = (bez.p3.x + 2 * xc) * (1.0 / 3.0);
          bez.p2.y = (bez.p3.y + 2 * yc) * (1.0 / 3.0);
          last_point = bez.p3;
          last_control.x = xc;
          last_control.y = yc;
          g_array_append_val (points, bez);
        }
        break;
      case PATH_ARC:
        {
          double rx, ry;
          double xrot;
          int    largearc, sweep;
          Point  dest, dest_c;
          dest_c.x=0;
          dest_c.y=0;

          rx = g_ascii_strtod (path, &path);
          path_chomp (path);
          ry = g_ascii_strtod (path, &path);
          path_chomp (path);
        #if 1 /* ok if it is all properly separated */
          xrot = g_ascii_strtod (path, &path);
          path_chomp (path);

          largearc = (int) g_ascii_strtod (path, &path);
          path_chomp (path);
          sweep = (int) g_ascii_strtod (path, &path);
          path_chomp (path);
        #else
          /* Actually three flags, which might not be properly separated,
          * but even with this paths-data-20-f.svg does not work. IMHO the
          * test case is seriously borked and can only pass if parsing
          * the arc is tweaked against the test. In other words that test
          * looks like it is built against one specific implementation.
          * Inkscape and librsvg fail, Firefox pass.
          */
          xrot = path[0] == '0' ? 0.0 : 1.0; ++path;
          path_chomp(path);

          largearc = path[0] == '0' ? 0 : 1; ++path;
          path_chomp(path);
          sweep =  path[0] == '0' ? 0 : 1; ++path;
          path_chomp(path);
        #endif

          dest.x = g_ascii_strtod (path, &path);
          path_chomp (path);
          dest.y = g_ascii_strtod (path, &path);
          path_chomp (path);

          if (last_relative) {
            dest.x += last_point.x;
            dest.y += last_point.y;
          }

          /* avoid matherr with bogus values - just ignore them
          * does happen e.g. with 'Chem-Widgets - clamp-large'
          */
          if (last_point.x != dest.x || last_point.y != dest.y) {
            _path_arc (points, last_point.x, last_point.y,
                      rx, ry, xrot, largearc, sweep, dest.x, dest.y,
                      &dest_c);
          }
          last_point = dest;
          last_control = dest_c;
        }
        break;
      case PATH_CLOSE:
        /* close the path with a line - second condition to ignore single close */
        if (!*closed && (points->len != points_at_start)) {
          const BezPoint *bpe = &g_array_index (points, BezPoint, points->len-1);
          /* if the last point already meets the first point dont add it again */
          const Point pte = bpe->type == BEZ_CURVE_TO ? bpe->p3 : bpe->p1;
          if (pte.x != last_open.x || pte.y != last_open.y) {
            bez.type = BEZ_LINE_TO;
            bez.p1 = last_open;
            g_array_append_val (points, bez);
          }
          last_point = last_open;
        }
        *closed = TRUE;
        need_next_element = TRUE;
        break;
      case PATH_END:
        while (*path != '\0') {
          path++;
        }
        need_next_element = FALSE;
        break;
      default:
        g_return_val_if_reached (FALSE);
    }
    /* get rid of any ignorable characters */
    path_chomp (path);
MORETOPARSE:
    if (need_next_element) {
      /* check if there really is more to be parsed */
      if (path[0] != 0) {
        *unparsed = path;
      } else {
        *unparsed = NULL;
      }
      break; /* while */
    }
  }

  /* avoid returning an array with only one point (I'd say the exporter
   * producing such is rather broken, but *our* bezier creation code
   * would crash on it.
   */
  if (points->len < 2) {
    g_array_set_size (points, 0);
  }

  if (current_point) {
    *current_point = last_point;
  }

  return (points->len > 1);
}


static gboolean
_parse_transform (const char *trans, graphene_matrix_t *m, double scale)
{
  char **list;
  char *p = strchr (trans, '(');
  int i = 0;

  while (   (*trans != '\0')
         && (*trans == ' ' || *trans == ',' || *trans == '\t' ||
             *trans == '\n' || *trans == '\r')) {
    ++trans; /* skip whitespace */
  }

  if (!p || !*trans) {
    return FALSE; /* silently fail */
  }

  list = g_regex_split_simple ("[\\s,]+", p + 1, 0, 0);
  if (strncmp (trans, "matrix", 6) == 0) {
    float xx = 0, yx = 0, xy = 0, yy = 0, x0 = 0, y0 = 0;

    if (list[i]) {
      xx = g_ascii_strtod (list[i], NULL);
      ++i;
    }

    if (list[i]) {
      yx = g_ascii_strtod (list[i], NULL);
      ++i;
    }

    if (list[i]) {
      xy = g_ascii_strtod (list[i], NULL);
      ++i;
    }

    if (list[i]) {
      yy = g_ascii_strtod (list[i], NULL);
      ++i;
    }

    if (list[i]) {
      x0 = g_ascii_strtod (list[i], NULL);
      ++i;
    }

    if (list[i]) {
      y0 = g_ascii_strtod (list[i], NULL);
      ++i;
    }

    graphene_matrix_init_from_2d (m, xx, yx, xy, yy, x0 / scale, y0 / scale);
  } else if (strncmp (trans, "translate", 9) == 0) {
    double x0 = 0, y0 = 0;

    if (list[i]) {
      x0 = g_ascii_strtod (list[i], NULL);
      ++i;
    }

    if (list[i]) {
      y0 = g_ascii_strtod (list[i], NULL);
      ++i;
    }

    graphene_matrix_init_translate (m, &GRAPHENE_POINT3D_INIT (x0 / scale, y0 / scale, 0));
  } else if (strncmp (trans, "scale", 5) == 0) {
    double xx = 0, yy = 0;

    if (list[i]) {
      xx = g_ascii_strtod (list[i], NULL);
      ++i;
    }

    if (list[i]) {
      yy = g_ascii_strtod (list[i], NULL);
      ++i;
    } else {
      yy = xx;
    }

    graphene_matrix_init_scale (m, xx, yy, 1.0);
  } else if (strncmp (trans, "rotate", 6) == 0) {
    double angle = 0;
    double cx = 0, cy = 0;

    if (list[i]) {
      angle = g_ascii_strtod (list[i], NULL);
      ++i;
    } else {
      g_warning ("transform=rotate no angle?");
    }

    /* FIXME: check with real world data, I'm uncertain */
    /* rotate around the given offset */
    if (list[i]) {
      graphene_point3d_t point;

      cx = g_ascii_strtod (list[i], NULL);
      ++i;

      if (list[i]) {
        cy = g_ascii_strtod (list[i], NULL);
        ++i;
      } else {
        cy = 0.0; /* if offsets don't come in pairs */
      }

      /* translate by -cx,-cy */
      graphene_point3d_init (&point, -(cx / scale), -(cy / scale), 0);
      graphene_matrix_init_translate (m, &point);

      /* rotate by angle */
      graphene_matrix_rotate_z (m, angle);

      /* translate by cx,cy */
      graphene_point3d_init (&point, cx / scale, cy / scale, 0);
      graphene_matrix_translate (m, &point);
    } else {
      graphene_matrix_init_rotate (m, angle, graphene_vec3_z_axis ());
    }
  } else if (strncmp (trans, "skewX", 5) == 0) {
    float skew = 0;

    if (list[i]) {
      skew = g_ascii_strtod (list[i], NULL);
    }

    graphene_matrix_init_skew (m, DIA_RADIANS (skew), 0);
  } else if (strncmp (trans, "skewY", 5) == 0) {
    float skew = 0;

    if (list[i]) {
      skew = g_ascii_strtod (list[i], NULL);
    }

    graphene_matrix_init_skew (m, 0, DIA_RADIANS (skew));
  } else {
    g_warning ("SVG: %s?", trans);

    return FALSE;
  }

  g_clear_pointer (&list, g_strfreev);

  return TRUE;
}


graphene_matrix_t *
dia_svg_parse_transform (const char *trans, double scale)
{
  graphene_matrix_t *m = NULL;
  char **transforms = g_regex_split_simple ("\\)", trans, 0, 0);
  int i = 0;

  /* go through the list of transformations - not that one would be enough ;) */
  while (transforms[i]) {
    graphene_matrix_t mat;

    if (_parse_transform (transforms[i], &mat, scale)) {
      if (!m) {
        m = graphene_matrix_alloc ();
        graphene_matrix_init_from_matrix (m, &mat);
      } else {
        graphene_matrix_multiply (m, &mat, m);
      }
    }
    ++i;
  }

  g_clear_pointer (&transforms, g_strfreev);

  return m;
}


char *
dia_svg_from_matrix (const graphene_matrix_t *matrix, double scale)
{
  /*  transform="matrix(1,0,0,1,0,0)" */
  char buf[G_ASCII_DTOSTR_BUF_SIZE];
  GString *sm = g_string_new ("matrix(");

  g_ascii_formatd (buf,
                   sizeof (buf),
                   "%g",
                   graphene_matrix_get_value (matrix, 0, 0));
  g_string_append (sm, buf);
  g_string_append (sm, ",");
  g_ascii_formatd (buf,
                   sizeof (buf),
                   "%g",
                   graphene_matrix_get_value (matrix, 0, 1));
  g_string_append (sm, buf);
  g_string_append (sm, ",");
  g_ascii_formatd (buf,
                   sizeof (buf),
                   "%g",
                   graphene_matrix_get_value (matrix, 1, 0));
  g_string_append (sm, buf);
  g_string_append (sm, ",");
  g_ascii_formatd (buf,
                   sizeof (buf),
                   "%g",
                   graphene_matrix_get_value (matrix, 1, 1));
  g_string_append (sm, buf);
  g_string_append (sm, ",");
  g_ascii_formatd (buf,
                   sizeof (buf),
                   "%g",
                   graphene_matrix_get_x_translation (matrix) * scale);
  g_string_append (sm, buf);
  g_string_append (sm, ",");
  g_ascii_formatd (buf,
                   sizeof (buf),
                   "%g",
                   graphene_matrix_get_y_translation (matrix) * scale);
  g_string_append (sm, buf);
  g_string_append (sm, ")");

  return g_string_free (sm, FALSE);
}
