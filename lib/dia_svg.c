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

#include "dia_svg.h"

enum
{
  FONT_NAME_LENGTH_MAX = 40
};

void
dia_svg_parse_style(xmlNodePtr node, DiaSvgGraphicStyle *s)
{
  char *str, *ptr;
  gchar temp[FONT_NAME_LENGTH_MAX+1]; /* font-family names will be limited to 40 characters */
  int i = 0;
  gboolean over = FALSE;  
  char *family = NULL, *style = NULL, *weight = NULL;
      
  ptr = str = xmlGetProp(node, "style");
  s->alignment = -1;

 if (str) {
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
    } else if (!strncmp("stroke:", ptr, 7)) {
      ptr += 7;
      while ((ptr[0] != '\0') && g_ascii_isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      if (!strncmp(ptr, "none", 4))
	s->stroke = DIA_SVG_COLOUR_NONE;
      else if (!strncmp(ptr, "foreground", 10) || !strncmp(ptr, "fg", 2) ||
	       !strncmp(ptr, "default", 7))
	s->stroke = DIA_SVG_COLOUR_FOREGROUND;
      else if (!strncmp(ptr, "background", 10) || !strncmp(ptr, "bg", 2) ||
	       !strncmp(ptr, "inverse", 7))
	s->stroke = DIA_SVG_COLOUR_BACKGROUND;
      else if (!strncmp(ptr, "text", 4))
	s->stroke = DIA_SVG_COLOUR_TEXT;
      else if (ptr[0] == '#')
	s->stroke = strtol(ptr+1, NULL, 16) & 0xffffff;
    } else if (!strncmp("fill:", ptr, 5)) {
      ptr += 5;
      while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      if (!strncmp(ptr, "none", 4))
	s->fill = DIA_SVG_COLOUR_NONE;
      else if (!strncmp(ptr, "foreground", 10) || !strncmp(ptr, "fg", 2) ||
	       !strncmp(ptr, "inverse", 7))
	s->fill = DIA_SVG_COLOUR_FOREGROUND;
      else if (!strncmp(ptr, "background", 10) || !strncmp(ptr, "bg", 2) ||
	       !strncmp(ptr, "default", 7))
	s->fill = DIA_SVG_COLOUR_BACKGROUND;
      else if (!strncmp(ptr, "text", 4))
	s->fill = DIA_SVG_COLOUR_TEXT;
      else if (ptr[0] == '#')
	s->fill = strtol(ptr+1, NULL, 16) & 0xffffff;
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
    } else if (!strncmp("stroke-dashlength:", ptr, 18)) {
      ptr += 18;
      while (ptr[0] != '\0' && g_ascii_isspace(ptr[0])) ptr++;
      if (ptr[0] == '\0') break;

      if (!strncmp(ptr, "default", 7))
	s->dashlength = 1.0;
      else {
	s->dashlength = g_ascii_strtod(ptr, &ptr);
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
      }
    }

    /* skip up to the next attribute */
    while (ptr[0] != '\0' && ptr[0] != ';' && ptr[0] != '\n') ptr++;
    if (ptr[0] != '\0') ptr++;
  }
  xmlFree(str);
 }

 if (family || style || weight) {
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
     }
 }
}

