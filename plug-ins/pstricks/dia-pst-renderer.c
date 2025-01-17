/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * render_pstricks.c: Exporting module/plug-in to TeX Pstricks
 * Copyright (C) 2000 Jacek Pliszka <pliszka@fuw.edu.pl>
 *  9.5.2000
 * with great help from James Henstridge and Denis Girou
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

/*
TODO:

1. transparent background in images
2. fonts selection/sizes
3. dots ???
4. Maybe draw and fill in a single move?? Will solve the problems
   with visible thin white line between the border and the fill
5. Verify the Pango stuff isn't spitting out things TeX is unable to read

NOT WORKING (exporting macros):
 1. linecaps
 2. linejoins
 3. dashdot and dashdotdot line styles

*/


#include "config.h"

#include <glib/gi18n-lib.h>

#include <string.h>
#include <time.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#include <glib/gstdio.h>

#include "message.h"
#include "diagramdata.h"
#include "dia_image.h"
#include "filter.h"
#include "font.h"
#include "dia-version-info.h"
#include "dia-locale.h"

#include "dia-pst-renderer.h"

#define POINTS_in_INCH 28.346


struct _DiaPstRenderer {
  DiaRenderer parent_instance;

  FILE *file;
  int is_ps;
  int pagenum;

  DiaContext *ctx;

  DiaFont *font;
  double font_height;
};


G_DEFINE_FINAL_TYPE (DiaPstRenderer, dia_pst_renderer, DIA_TYPE_RENDERER)


enum {
  PROP_0,
  PROP_FONT,
  PROP_FONT_HEIGHT,
  LAST_PROP
};


static inline void
set_font (DiaPstRenderer *self, DiaFont *font, double height)
{
  DiaLocaleContext ctx;

  g_set_object (&self->font, font);
  self->font_height = height;

  dia_locale_push_numeric (&ctx);
  fprintf (self->file,
           "\\setfont{%s}{%.17g}\n",
           dia_font_get_psfontname (font),
           height);
  dia_locale_pop (&ctx);
}


static void
dia_pst_renderer_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  DiaPstRenderer *self = DIA_PST_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      set_font (self,
                DIA_FONT (g_value_get_object (value)),
                self->font_height);
      break;
    case PROP_FONT_HEIGHT:
      set_font (self,
                self->font,
                g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_pst_renderer_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  DiaPstRenderer *self = DIA_PST_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      g_value_set_object (value, self->font);
      break;
    case PROP_FONT_HEIGHT:
      g_value_set_double (value, self->font_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_pst_renderer_dispose (GObject *object)
{
  DiaPstRenderer *self = DIA_PST_RENDERER (object);

  g_clear_object (&self->font);

  G_OBJECT_CLASS (dia_pst_renderer_parent_class)->dispose (object);
}


/*!
 * \brief Advertize special capabilities
 *
 * Some objects drawing adapts to capabilities advertized by the respective
 * renderer. Usually there is a fallback, but generally the real thing should
 * be better.
 *
 * \memberof _DiaPstRenderer
 */
static gboolean
dia_pst_renderer_is_capable_to (DiaRenderer *renderer, RenderCapability cap)
{
  if (RENDER_HOLES == cap) {
    return TRUE; /* ... with under-documented fillstyle=eofill */
  } else if (RENDER_ALPHA == cap) {
    return FALSE; /* simulate with hatchwidth? */
  } else if (RENDER_AFFINE == cap) {
    return FALSE; /* maybe by: \translate, \scale, \rotate */
  } else if (RENDER_PATTERN == cap) {
    return FALSE; /* nope */
  }

  return FALSE;
}


static void
set_line_colour (DiaPstRenderer *renderer, Color *colour)
{
  DiaLocaleContext ctx;

  dia_locale_push_numeric (&ctx);
  fprintf (renderer->file,
           "\\newrgbcolor{dialinecolor}{%.17g %.17g %.17g}%%\n",
           colour->red, colour->green, colour->blue);
  fprintf (renderer->file, "\\psset{linecolor=dialinecolor}\n");
  dia_locale_pop (&ctx);
}


static void
set_fill_colour (DiaPstRenderer *renderer, Color *colour)
{
  DiaLocaleContext ctx;

  dia_locale_push_numeric (&ctx);
  fprintf (renderer->file,
           "\\newrgbcolor{diafillcolor}{%.17g %.17g %.17g}%%\n",
           colour->red, colour->green, colour->blue);
  fprintf (renderer->file, "\\psset{fillcolor=diafillcolor}\n");
  dia_locale_pop (&ctx);
}


static void
dia_pst_renderer_begin_render (DiaRenderer *self, const DiaRectangle *update)
{
}


static void
dia_pst_renderer_end_render (DiaRenderer *self)
{
  DiaPstRenderer *renderer = DIA_PST_RENDERER (self);

  fprintf (renderer->file, "}\\endpspicture");
  fclose (renderer->file);
}


static void
dia_pst_renderer_set_linewidth (DiaRenderer *self, double linewidth)
{
  /* 0 == hairline **/
  DiaPstRenderer *renderer = DIA_PST_RENDERER (self);
  DiaLocaleContext ctx;

  dia_locale_push_numeric (&ctx);
  fprintf (renderer->file, "\\psset{linewidth=%.17gcm}\n", linewidth);
  dia_locale_pop (&ctx);
}


static void
dia_pst_renderer_set_linecaps (DiaRenderer *self, DiaLineCaps mode)
{
  DiaPstRenderer *renderer = DIA_PST_RENDERER (self);
  int ps_mode;

  switch (mode) {
    case DIA_LINE_CAPS_DEFAULT:
    case DIA_LINE_CAPS_BUTT:
      ps_mode = 0;
      break;
    case DIA_LINE_CAPS_ROUND:
      ps_mode = 1;
      break;
    case DIA_LINE_CAPS_PROJECTING:
      ps_mode = 2;
      break;
    default:
      ps_mode = 0;
  }

  fprintf (renderer->file, "\\setlinecaps{%d}\n", ps_mode);
}


static void
dia_pst_renderer_set_linejoin (DiaRenderer *self, DiaLineJoin mode)
{
  DiaPstRenderer *renderer = DIA_PST_RENDERER (self);
  int ps_mode;

  switch (mode) {
    case DIA_LINE_JOIN_DEFAULT:
    case DIA_LINE_JOIN_MITER:
      ps_mode = 0;
      break;
    case DIA_LINE_JOIN_ROUND:
      ps_mode = 1;
      break;
    case DIA_LINE_JOIN_BEVEL:
      ps_mode = 2;
      break;
    default:
      ps_mode = 0;
  }

  fprintf (renderer->file, "\\setlinejoinmode{%d}\n", ps_mode);
}


static void
dia_pst_renderer_set_linestyle (DiaRenderer  *self,
                                DiaLineStyle  mode,
                                double        dash_length)
{
  DiaPstRenderer *renderer = DIA_PST_RENDERER (self);
  DiaLocaleContext ctx;
  double hole_width;
  double dot_length;

  if (dash_length < 0.001) {
    dash_length = 0.001;
  }

  /* dot = 20% of len - for some reason not the usual 10% */
  dot_length = dash_length * 0.2;

  dia_locale_push_numeric (&ctx);
  switch (mode) {
    case DIA_LINE_STYLE_DEFAULT:
    case DIA_LINE_STYLE_SOLID:
      fprintf (renderer->file, "\\psset{linestyle=solid}\n");
      break;
    case DIA_LINE_STYLE_DASHED:
      fprintf (renderer->file, "\\psset{linestyle=dashed,dash=%.17g %.17g}\n",
               dash_length, dash_length);
      break;
    case DIA_LINE_STYLE_DASH_DOT:
      hole_width = (dash_length - dot_length) / 2.0;
      fprintf (renderer->file, "\\psset{linestyle=dashed,dash=%.17g %.17g %.17g %.17g}\n",
               dash_length, hole_width, dot_length, hole_width);
      break;
    case DIA_LINE_STYLE_DASH_DOT_DOT:
      hole_width = (dash_length - 2.0*dot_length) / 3.0;
      fprintf (renderer->file, "\\psset{linestyle=dashed,dash=%.17g %.17g %.17g %.17g %.17g %.17g}\n",
               dash_length, hole_width, dot_length, hole_width, dot_length, hole_width);
      break;
    case DIA_LINE_STYLE_DOTTED:
      fprintf (renderer->file, "\\psset{linestyle=dotted,dotsep=%.17g}\n", dot_length);
      break;
    default:
      g_warning ("Unknown mode %i", mode);
      break;
  }
  dia_locale_pop (&ctx);
}


static void
dia_pst_renderer_set_fillstyle (DiaRenderer *self, DiaFillStyle mode)
{
  switch (mode) {
    case DIA_FILL_STYLE_SOLID:
      break;
    default:
      g_warning ("dia_pst_renderer: Unsupported fill mode specified!\n");
  }
}


static void
dia_pst_renderer_draw_line (DiaRenderer *self,
                            Point       *start,
                            Point       *end,
                            Color       *line_color)
{
  DiaPstRenderer *renderer = DIA_PST_RENDERER (self);
  DiaLocaleContext ctx;

  set_line_colour (renderer, line_color);

  dia_locale_push_numeric (&ctx);
  fprintf (renderer->file, "\\psline(%.17g,%.17g)(%.17g,%.17g)\n",
           start->x, start->y, end->x, end->y);
  dia_locale_pop (&ctx);
}


static void
dia_pst_renderer_draw_polyline (DiaRenderer *self,
                                Point       *points,
                                int          num_points,
                                Color       *line_color)
{
  DiaPstRenderer *renderer = DIA_PST_RENDERER (self);
  DiaLocaleContext ctx;

  set_line_colour (renderer, line_color);

  dia_locale_push_numeric (&ctx);
  fprintf (renderer->file, "\\psline(%.17g,%.17g)", points[0].x, points[0].y);
  for (size_t i = 1; i < num_points; i++) {
    fprintf (renderer->file, "(%.17g,%.17g)", points[i].x, points[i].y);
  }
  dia_locale_pop (&ctx);

  fprintf (renderer->file, "\n");
}


static void
dia_pst_renderer_draw_polygon (DiaRenderer *self,
                               Point       *points,
                               int          num_points,
                               Color       *fill,
                               Color       *stroke)
{
  DiaPstRenderer *renderer = DIA_PST_RENDERER(self);
  DiaLocaleContext ctx;
  const char *style;

  if (fill) {
    set_fill_colour (renderer, fill);
  }

  if (stroke) {
    set_line_colour (renderer, stroke);
  }

  if (fill && stroke) {
    style = "[fillstyle=eofill,fillcolor=diafillcolor,linecolor=dialinecolor]";
  } else if (fill) {
    style = "[fillstyle=eofill,fillcolor=diafillcolor]";
  } else {
    style = "";
  }

  /* The graphics objects all have a starred version (e.g., \psframe*) which
   * draws a solid object whose color is linecolor. [pstricks-doc.pdf p7]
   *
   * Not properly documented, but still working ...
   */
  dia_locale_push_numeric (&ctx);
  fprintf (renderer->file, "\\pspolygon%s(%.17g,%.17g)",
           style, points[0].x, points[0].y);
  for (size_t i = 1; i < num_points; i++) {
    fprintf (renderer->file, "(%.17g,%.17g)", points[i].x, points[i].y);
  }
  dia_locale_pop (&ctx);

  fprintf (renderer->file, "\n");
}


static void
pstricks_arc (DiaPstRenderer *renderer,
              Point          *center,
              double          width,
              double          height,
              double          angle1,
              double          angle2,
              Color          *color,
              int             filled)
{
  DiaLocaleContext ctx;
  double radius1 = width / 2.0;
  double radius2 = height / 2.0;

  /* counter-clockwise */
  if (angle2 < angle1) {
    double tmp = angle1;
    angle1 = angle2;
    angle2 = tmp;
  }

  set_line_colour (renderer, color);

  dia_locale_push_numeric (&ctx);
  fprintf (renderer->file,
           "\\psclip{\\pswedge[linestyle=none,fillstyle=none](%.17g,%.17g){%.17g}{%.17g}{%.17g}}\n",
           center->x,
           center->y,
           sqrt (radius1 * radius1 + radius2 * radius2),
           360.0 - angle2,
           360.0 - angle1);
  fprintf (renderer->file, "\\psellipse%s(%.17g,%.17g)(%.17g,%.17g)\n",
           (filled ? "*" : ""), center->x, center->y, radius1, radius2);
  dia_locale_pop (&ctx);

  fprintf (renderer->file, "\\endpsclip\n");
}


static void
dia_pst_renderer_draw_arc (DiaRenderer *self,
                           Point       *center,
                           double       width,
                           double       height,
                           double       angle1,
                           double       angle2,
                           Color       *color)
{
  DiaPstRenderer *renderer = DIA_PST_RENDERER (self);

  pstricks_arc (renderer, center, width, height, angle1, angle2, color, 0);
}


static void
dia_pst_renderer_fill_arc (DiaRenderer *self,
                           Point       *center,
                           double       width,
                           double       height,
                           double       angle1,
                           double       angle2,
                           Color       *color)
{
  DiaPstRenderer *renderer = DIA_PST_RENDERER (self);

  pstricks_arc (renderer, center, width, height, angle1, angle2, color, 1);
}


static void
pstricks_ellipse (DiaPstRenderer *renderer,
                  Point          *center,
                  double          width,
                  double          height,
                  Color          *color,
                  gboolean        filled)
{
  DiaLocaleContext ctx;

  set_line_colour (renderer, color);

  dia_locale_push_numeric (&ctx);
  fprintf (renderer->file,
           "\\psellipse%s(%.17g,%.17g)(%.17g,%.17g)\n",
           (filled ? "*" : ""), center->x, center->y, width / 2.0, height / 2.0);
  dia_locale_pop (&ctx);
}


static void
dia_pst_renderer_draw_ellipse (DiaRenderer *self,
                               Point       *center,
                               double       width,
                               double       height,
                               Color       *fill,
                               Color       *stroke)
{
  DiaPstRenderer *renderer = DIA_PST_RENDERER (self);

  if (fill) {
    pstricks_ellipse (renderer, center, width, height, fill, TRUE);
  }

  if (stroke) {
    pstricks_ellipse (renderer, center, width, height, stroke, FALSE);
  }
}


static void
pstricks_bezier (DiaPstRenderer   *renderer,
                 BezPoint         *points,
                 gint              numpoints,
                 Color            *fill,
                 Color            *stroke,
                 gboolean          closed)
{
  DiaLocaleContext ctx;

  if (fill) {
    set_fill_colour (renderer, fill);
  }

  if (stroke) {
    set_line_colour (renderer, stroke);
  }

  fprintf (renderer->file, "\\pscustom{\n");

  if (points[0].type != BEZ_MOVE_TO) {
    g_warning ("first BezPoint must be a BEZ_MOVE_TO");
  }

  dia_locale_push_numeric (&ctx);
  fprintf (renderer->file, "\\newpath\n\\moveto(%.17g,%.17g)\n",
           points[0].p1.x, points[0].p1.y);

  for (size_t i = 1; i < numpoints; i++) {
    switch (points[i].type) {
      case BEZ_MOVE_TO:
        fprintf (renderer->file, "\\moveto(%.17g,%.17g)\n",
                 points[i].p1.x, points[i].p1.y);
        break;
      case BEZ_LINE_TO:
        fprintf (renderer->file, "\\lineto(%.17g,%.17g)\n",
                 points[i].p1.x, points[i].p1.y);
        break;
      case BEZ_CURVE_TO:
        fprintf (renderer->file, "\\curveto(%.17g,%.17g)(%.17g,%.17g)(%.17g,%.17g)\n",
                 points[i].p1.x, points[i].p1.y,
                 points[i].p2.x, points[i].p2.y,
                 points[i].p3.x, points[i].p3.y);
        break;
      default:
        g_warning ("Unknown type %i", points[i].type);
        break;
    }
  }
  dia_locale_pop (&ctx);

  if (closed) {
    fprintf (renderer->file, "\\closepath\n");
  }

  if (fill && stroke) {
    fprintf (renderer->file, "\\fill[fillstyle=eofill,fillcolor=diafillcolor,linecolor=dialinecolor]}\n");
  } else if (fill) {
    fprintf (renderer->file, "\\fill[fillstyle=eofill,fillcolor=diafillcolor,linecolor=diafillcolor]}\n");
  } else {
    fprintf (renderer->file, "\\stroke}\n");
  }
}


static void
dia_pst_renderer_draw_bezier (DiaRenderer *self,
                              BezPoint    *points,
                              int          numpoints, /* numpoints = 4+3*n, n=>0 */
                              Color       *color)
{
  DiaPstRenderer *renderer = DIA_PST_RENDERER (self);

  pstricks_bezier (renderer, points, numpoints, NULL, color, FALSE);
}


static void
dia_pst_renderer_draw_beziergon (DiaRenderer *self,
                                 BezPoint    *points,
                                 int          numpoints,
                                 Color       *fill,
                                 Color       *stroke)
{
  DiaPstRenderer *renderer = DIA_PST_RENDERER (self);

  pstricks_bezier (renderer, points, numpoints, fill, stroke, TRUE);
}


/* Do we really want to do this?  What if the text is intended as
 * TeX text?  Jacek says leave it as a TeX string.  TeX uses should know
 * how to escape stuff anyway.  Later versions will get an export option.
 *
 * Later (hb): given the UML issue bug #112377 an manually tweaking
 * is not possible as the # is added before anything a user can add. So IMO
 * we need to want this. If there later (much later?) is an export it probably
 * shouldn't produce broken output either ...
 */
static char *
tex_escape_string (const char *src, DiaContext *ctx)
{
  GString *dest = g_string_sized_new (g_utf8_strlen (src, -1));
  char *p;

  if (!g_utf8_validate (src, -1, NULL)) {
    dia_context_add_message (ctx, _("Not valid UTF-8"));
    return g_strdup (src);
  }

  p = (char *) src;
  while (*p != '\0') {
    switch (*p) {
      case '%':
        g_string_append (dest, "\\%");
        break;
      case '#':
        g_string_append (dest, "\\#");
        break;
      case '$':
        g_string_append (dest, "\\$");
        break;
      case '&':
        g_string_append (dest, "\\&");
        break;
      case '~':
        g_string_append (dest, "\\~{}");
        break;
      case '_':
        g_string_append (dest, "\\_");
        break;
      case '^':
        g_string_append (dest, "\\^{}");
        break;
      case '\\':
        g_string_append (dest, "\\textbackslash{}");
        break;
      case '{':
        g_string_append (dest, "\\}");
        break;
      case '}':
        g_string_append (dest, "\\}");
        break;
      case '[':
        g_string_append (dest, "\\ensuremath{\\left[\\right.}");
        break;
      case ']':
        g_string_append (dest, "\\ensuremath{\\left.\\right]}");
        break;
      default:
        /* if we really have utf8 append the whole 'glyph' */
        g_string_append_len (dest, p, g_utf8_skip[(unsigned char) *p]);
    }
    p = g_utf8_next_char (p);
  }

  return g_string_free (dest, FALSE);
}


static void
dia_pst_renderer_draw_string (DiaRenderer  *self,
                              const char   *text,
                              Point        *pos,
                              DiaAlignment  alignment,
                              Color        *color)
{
  DiaPstRenderer *renderer = DIA_PST_RENDERER (self);
  char *escaped = NULL;
  DiaLocaleContext ctx;

  /* only escape the string if it is not starting with \tex */
  if (strncmp (text, "\\tex", 4) != 0) {
    escaped = tex_escape_string (text, renderer->ctx);
  }

  set_fill_colour (renderer, color);

  fprintf (renderer->file, "\\rput");
  switch (alignment) {
    case DIA_ALIGN_LEFT:
      fprintf (renderer->file, "[l]");
      break;
    case DIA_ALIGN_CENTRE:
      break;
    case DIA_ALIGN_RIGHT:
      fprintf (renderer->file, "[r]");
      break;
    default:
      g_warning ("Unknown alignment %i", alignment);
      break;
  }

  dia_locale_push_numeric (&ctx);
  fprintf (renderer->file,
           "(%.17g,%.17g){\\psscalebox{1 -1}{%s}}\n",
           pos->x, pos->y, escaped ? escaped : text);
  dia_locale_pop (&ctx);

  g_clear_pointer (&escaped, g_free);
}


static void
dia_pst_renderer_draw_image (DiaRenderer *self,
                             Point       *point,
                             double       width,
                             double       height,
                             DiaImage    *image)
{
  DiaPstRenderer *renderer = DIA_PST_RENDERER (self);
  int img_width = dia_image_width (image);
  int img_height = dia_image_height (image);
  guint8 *rgb_data = dia_image_rgb_data (image);
  double points_in_inch = POINTS_in_INCH;
  unsigned char *ptr;
  DiaLocaleContext ctx;

  if (!rgb_data) {
    dia_context_add_message (renderer->ctx,
                             _("Not enough memory for image drawing."));
    return;
  }

  fprintf (renderer->file, "\\pscustom{\\code{gsave\n");

  dia_locale_push_numeric (&ctx);
  if (1) { /* Color output */
    fprintf (renderer->file, "/pix %i string def\n", img_width * 3);
    fprintf (renderer->file, "/grays %i string def\n", img_width);
    fprintf (renderer->file, "/npixls 0 def\n");
    fprintf (renderer->file, "/rgbindx 0 def\n");
    fprintf (renderer->file, "%.17g %.17g scale\n", points_in_inch, points_in_inch);
    fprintf (renderer->file, "%.17g %.17g translate\n", point->x, point->y);
    fprintf (renderer->file, "%.17g %.17g scale\n", width, height);
    fprintf (renderer->file, "%i %i 8\n", img_width, img_height);
    fprintf (renderer->file, "[%i 0 0 %i 0 0]\n", img_width, img_height);
    fprintf (renderer->file, "{currentfile pix readhexstring pop}\n");
    fprintf (renderer->file, "false 3 colorimage\n");
    /*    fprintf(renderer->file, "\n"); */
    for (size_t y = 0; y < img_height; y++) {
      ptr = rgb_data + y * dia_image_rowstride (image);
      for (size_t x = 0; x < img_width; x++) {
        fprintf (renderer->file, "%02x", (int) (*ptr++));
        fprintf (renderer->file, "%02x", (int) (*ptr++));
        fprintf (renderer->file, "%02x", (int) (*ptr++));
      }
      fprintf (renderer->file, "\n");
    }
  } else { /* Grayscale */
    fprintf (renderer->file, "/pix %i string def\n", img_width);
    fprintf (renderer->file, "/grays %i string def\n", img_width);
    fprintf (renderer->file, "/npixls 0 def\n");
    fprintf (renderer->file, "/rgbindx 0 def\n");
    fprintf (renderer->file, "%.17g %.17g scale\n", points_in_inch, points_in_inch);
    fprintf (renderer->file, "%.17g %.17g translate\n", point->x, point->y);
    fprintf (renderer->file, "%.17g %.17g scale\n", width, height);
    fprintf (renderer->file, "%i %i 8\n", img_width, img_height);
    fprintf (renderer->file, "[%i 0 0 %i 0 0]\n", img_width, img_height);
    fprintf (renderer->file, "{currentfile pix readhexstring pop}\n");
    fprintf (renderer->file, "image\n");
    fprintf (renderer->file, "\n");
    ptr = rgb_data;
    for (size_t y = 0; y < img_height; y++) {
      for (size_t x = 0; x < img_width; x++) {
        int v = (int) (*ptr++);
        v += (int) (*ptr++);
        v += (int) (*ptr++);
        v /= 3;
        fprintf (renderer->file, "%02x", v);
      }
      fprintf (renderer->file, "\n");
    }
  }
  /*  fprintf(renderer->file, "%f %f scale\n", 1.0, 1.0/ratio);*/
  fprintf (renderer->file, "grestore\n");
  fprintf (renderer->file, "}}\n");
  dia_locale_pop (&ctx);

  g_clear_pointer (&rgb_data, g_free);
}


static void
dia_pst_renderer_class_init (DiaPstRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  object_class->set_property = dia_pst_renderer_set_property;
  object_class->get_property = dia_pst_renderer_get_property;
  object_class->dispose = dia_pst_renderer_dispose;

  renderer_class->begin_render = dia_pst_renderer_begin_render;
  renderer_class->end_render = dia_pst_renderer_end_render;
  renderer_class->is_capable_to = dia_pst_renderer_is_capable_to;

  renderer_class->set_linewidth = dia_pst_renderer_set_linewidth;
  renderer_class->set_linecaps = dia_pst_renderer_set_linecaps;
  renderer_class->set_linejoin = dia_pst_renderer_set_linejoin;
  renderer_class->set_linestyle = dia_pst_renderer_set_linestyle;
  renderer_class->set_fillstyle = dia_pst_renderer_set_fillstyle;

  renderer_class->draw_line = dia_pst_renderer_draw_line;
  renderer_class->draw_polyline = dia_pst_renderer_draw_polyline;

  renderer_class->draw_polygon = dia_pst_renderer_draw_polygon;

  renderer_class->draw_arc = dia_pst_renderer_draw_arc;
  renderer_class->fill_arc = dia_pst_renderer_fill_arc;

  renderer_class->draw_ellipse = dia_pst_renderer_draw_ellipse;

  renderer_class->draw_bezier = dia_pst_renderer_draw_bezier;
  renderer_class->draw_beziergon = dia_pst_renderer_draw_beziergon;

  renderer_class->draw_string = dia_pst_renderer_draw_string;

  renderer_class->draw_image = dia_pst_renderer_draw_image;

  g_object_class_override_property (object_class, PROP_FONT, "font");
  g_object_class_override_property (object_class, PROP_FONT_HEIGHT, "font-height");
}


static void
dia_pst_renderer_init (DiaPstRenderer *self)
{
}


gboolean
dia_pst_renderer_export (DiaPstRenderer *self,
                         DiaContext     *ctx,
                         DiagramData    *data,
                         const char     *filename,
                         const char     *diafilename)
{
  DiaLocaleContext l_ctx;
  time_t time_now;
  DiaRectangle *extent;
  const char *name;
  Color initial_color;

  g_return_val_if_fail (DIA_PST_IS_RENDERER (self), FALSE);

  self->file = g_fopen(filename, "wb");
  if (self->file == NULL) {
    dia_context_add_message_with_errno (ctx,
                                        errno,
                                        _("Can't open output file %s"),
                                        dia_context_get_filename (ctx));
    return FALSE;
  }

  self->pagenum = 1;
  self->ctx = ctx;

  time_now  = time (NULL);
  extent = &data->extents;

  name = g_get_user_name ();

  fprintf (self->file,
           "%% PSTricks TeX macro\n"
           "%% Title: %s\n"
           "%% Creator: Dia v%s\n"
           "%% CreationDate: %s"
           "%% For: %s\n"
           "%% \\usepackage{pstricks}\n"
           "%% The following commands are not supported in PSTricks at present\n"
           "%% We define them conditionally, so when they are implemented,\n"
           "%% this pstricks file will use them.\n"
           "\\ifx\\setlinejoinmode\\undefined\n"
           "  \\newcommand{\\setlinejoinmode}[1]{}\n"
           "\\fi\n"
           "\\ifx\\setlinecaps\\undefined\n"
           "  \\newcommand{\\setlinecaps}[1]{}\n"
           "\\fi\n"
           "%% This way define your own fonts mapping (for example with ifthen)\n"
           "\\ifx\\setfont\\undefined\n"
           "  \\newcommand{\\setfont}[2]{}\n"
                 "\\fi\n",
           diafilename,
           dia_version_string (),
           ctime (&time_now),
           name);

  dia_locale_push_numeric (&l_ctx);
  fprintf (self->file, "\\pspicture(%.17g,%.17g)(%.17g,%.17g)\n",
           extent->left * data->paper.scaling,
           -extent->bottom * data->paper.scaling,
           extent->right * data->paper.scaling,
           -extent->top * data->paper.scaling);
  fprintf (self->file,"\\psscalebox{%.17g %.17g}{\n",
           data->paper.scaling, -data->paper.scaling);
  dia_locale_pop (&l_ctx);

  initial_color.red = 0.0;
  initial_color.green = 0.0;
  initial_color.blue = 0.0;
  set_line_colour (self, &initial_color);

  initial_color.red = 1.0;
  initial_color.green = 1.0;
  initial_color.blue = 1.0;
  set_fill_colour (self, &initial_color);

  data_render (data, DIA_RENDERER (self), NULL, NULL, NULL);

  return TRUE;
}
