/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diapsrenderer.c -- implements the base class for Postscript rendering
 *   It is mostly refactoring of render_eps.c (some stuff not from the
 *   latest version but from 1.24) before PS rendering became multi-pass
 *   and text rendering became (necessary) complicated.
 * Refatoring: Copyright (C) 2002 Hans Breuer
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

#include <string.h>
#include <time.h>

#include "diapsrenderer.h"
#include "dia_image.h"
#include "font.h"
#include "textline.h"

#define DTOSTR_BUF_SIZE G_ASCII_DTOSTR_BUF_SIZE
#define psrenderer_dtostr(buf,d) \
	g_ascii_formatd(buf, sizeof(buf), "%f", d)

enum {
  PROP_0,
  PROP_FONT,
  PROP_FONT_HEIGHT,
  LAST_PROP
};


/* Returns TRUE if this file is an encapsulated postscript file
 * (including e.g. epsi).
 */
static gboolean renderer_is_eps(DiaPsRenderer *renderer) {
  return renderer->pstype == PSTYPE_EPS ||
    renderer->pstype == PSTYPE_EPSI;
}

/** Returns TRUE if this file is an EPSI file */
static gboolean renderer_is_epsi(DiaPsRenderer *renderer) {
  return renderer->pstype == PSTYPE_EPSI;
}

void
lazy_setcolor(DiaPsRenderer *renderer,
              Color *color)
{
  gchar r_buf[DTOSTR_BUF_SIZE];
  gchar g_buf[DTOSTR_BUF_SIZE];
  gchar b_buf[DTOSTR_BUF_SIZE];

  if (!color_equals(color, &(renderer->lcolor))) {
    renderer->lcolor = *color;
    fprintf(renderer->file, "%s %s %s srgb\n",
            psrenderer_dtostr(r_buf, (gdouble) color->red),
            psrenderer_dtostr(g_buf, (gdouble) color->green),
            psrenderer_dtostr(b_buf, (gdouble) color->blue) );
  }
}

static void
begin_render(DiaRenderer *self, const DiaRectangle *update)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
  time_t time_now;

  g_assert (renderer->file != NULL);

  time_now  = time(NULL);

  if (renderer_is_eps(renderer))
    fprintf(renderer->file,
            "%%!PS-Adobe-2.0 EPSF-2.0\n");
  else
    fprintf(renderer->file,
            "%%!PS-Adobe-2.0\n");
  fprintf(renderer->file,
          "%%%%Title: %s\n"
          "%%%%Creator: Dia v%s\n"
          "%%%%CreationDate: %s"
          "%%%%For: %s\n"
          "%%%%Orientation: %s\n",
          renderer->title ? renderer->title : "(NULL)" ,
          VERSION,
          ctime(&time_now),
          g_get_user_name(),
          renderer->is_portrait ? "Portrait" : "Landscape");

  if (renderer_is_epsi(renderer)) {
    g_assert(!"Preview image not implemented");
    /* but it *may* belong here ... */
  }
  if (renderer_is_eps(renderer))
    fprintf(renderer->file,
            "%%%%Magnification: 1.0000\n"
	  "%%%%BoundingBox: 0 0 %d %d\n",
            (int) ceil(  (renderer->extent.right - renderer->extent.left)
                       * renderer->scale),
            (int) ceil(  (renderer->extent.bottom - renderer->extent.top)
                       * renderer->scale) );

  else
    fprintf(renderer->file,
	  "%%%%DocumentPaperSizes: %s\n",
            renderer->paper ? renderer->paper : "(NULL)");

  fprintf(renderer->file,
          "%%%%BeginSetup\n");
  fprintf(renderer->file,
          "%%%%EndSetup\n"
          "%%%%EndComments\n");

  DIA_PS_RENDERER_GET_CLASS(self)->begin_prolog(renderer);
  /* get our font definitions */
  DIA_PS_RENDERER_GET_CLASS(self)->dump_fonts(renderer);
  /* done it */
  DIA_PS_RENDERER_GET_CLASS(self)->end_prolog(renderer);
}

static void
end_render (DiaRenderer *self)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER (self);

  if (renderer_is_eps (renderer)) {
    fprintf (renderer->file, "showpage\n");
  }

  g_clear_object (&renderer->font);
}

static void
set_linewidth(DiaRenderer *self, real linewidth)
{  /* 0 == hairline **/
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
  gchar lw_buf[DTOSTR_BUF_SIZE];

  /* Adobe's advice:  Set to very small but fixed size, to avoid changes
   * due to different resolution output. */
  /* .01 cm becomes <5 dots on 1200 DPI */
  if (linewidth == 0.0) linewidth=.01;
  fprintf(renderer->file, "%s slw\n",
	  psrenderer_dtostr(lw_buf, (gdouble) linewidth) );
}

static void
set_linecaps(DiaRenderer *self, LineCaps mode)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
  int ps_mode;

  switch(mode) {
  case LINECAPS_DEFAULT:
  case LINECAPS_BUTT:
    ps_mode = 0;
    break;
  case LINECAPS_ROUND:
    ps_mode = 1;
    break;
  case LINECAPS_PROJECTING:
    ps_mode = 2;
    break;
  default:
    ps_mode = 0;
  }

  fprintf(renderer->file, "%d slc\n", ps_mode);
}

static void
set_linejoin(DiaRenderer *self, LineJoin mode)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
  int ps_mode;

  switch(mode) {
  case LINEJOIN_DEFAULT:
  case LINEJOIN_MITER:
    ps_mode = 0;
    break;
  case LINEJOIN_ROUND:
    ps_mode = 1;
    break;
  case LINEJOIN_BEVEL:
    ps_mode = 2;
    break;
  default:
    ps_mode = 0;
  }

  fprintf(renderer->file, "%d slj\n", ps_mode);
}


static void
set_linestyle (DiaRenderer *self, LineStyle mode, real dash_length)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
  real hole_width;
  gchar dashl_buf[DTOSTR_BUF_SIZE];
  gchar dotl_buf[DTOSTR_BUF_SIZE];
  gchar holew_buf[DTOSTR_BUF_SIZE];
  real dot_length;

  if (dash_length < 0.001) {
    dash_length = 0.001;
  }

  dot_length = dash_length*0.2; /* dot = 20% of len */

  switch (mode) {
    case LINESTYLE_DEFAULT:
    case LINESTYLE_SOLID:
      fprintf (renderer->file, "[] 0 sd\n");
      break;
    case LINESTYLE_DASHED:
      fprintf (renderer->file, "[%s] 0 sd\n",
               psrenderer_dtostr (dashl_buf, dash_length) );
      break;
    case LINESTYLE_DASH_DOT:
      hole_width = (dash_length - dot_length) / 2.0;
      psrenderer_dtostr (holew_buf, hole_width);
      psrenderer_dtostr (dashl_buf, dash_length);
      psrenderer_dtostr (dotl_buf, dot_length);
      fprintf (renderer->file, "[%s %s %s %s] 0 sd\n",
               dashl_buf,
               holew_buf,
               dotl_buf,
               holew_buf );
      break;
    case LINESTYLE_DASH_DOT_DOT:
      hole_width = (dash_length - 2.0*dot_length) / 3.0;
      psrenderer_dtostr (holew_buf, hole_width);
      psrenderer_dtostr (dashl_buf, dash_length);
      psrenderer_dtostr (dotl_buf, dot_length);
      fprintf (renderer->file, "[%s %s %s %s %s %s] 0 sd\n",
               dashl_buf,
               holew_buf,
               dotl_buf,
               holew_buf,
               dotl_buf,
               holew_buf );
      break;
    case LINESTYLE_DOTTED:
      fprintf (renderer->file, "[%s] 0 sd\n",
               psrenderer_dtostr (dotl_buf, dot_length) );
      break;
    default:
      g_return_if_reached ();
  }
}


static void
set_fillstyle (DiaRenderer *self, FillStyle mode)
{
  switch (mode) {
    case FILLSTYLE_SOLID:
      break;
    default:
      g_warning ("%s: Unsupported fill mode specified!",
                 G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
  }
}


static void
set_font (DiaRenderer *self, DiaFont *font, real height)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER (self);
  gchar h_buf[DTOSTR_BUF_SIZE];

  if (font != renderer->font || height != renderer->font_height) {
    DiaFont *old_font;

    fprintf (renderer->file,
             "/%s-latin1 ff %s scf sf\n",
             dia_font_get_psfontname (font),
             psrenderer_dtostr (h_buf, (gdouble) height*0.7));
    old_font = renderer->font;
    renderer->font = font;
    g_object_ref (renderer->font);
    g_clear_object (&old_font);
    renderer->font_height = height;
  }
}

static void
draw_line(DiaRenderer *self,
	  Point *start, Point *end,
	  Color *line_color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
  gchar sx_buf[DTOSTR_BUF_SIZE];
  gchar sy_buf[DTOSTR_BUF_SIZE];
  gchar ex_buf[DTOSTR_BUF_SIZE];
  gchar ey_buf[DTOSTR_BUF_SIZE];

  lazy_setcolor(renderer,line_color);

  fprintf(renderer->file, "n %s %s m %s %s l s\n",
	  psrenderer_dtostr(sx_buf, start->x),
	  psrenderer_dtostr(sy_buf, start->y),
	  psrenderer_dtostr(ex_buf, end->x),
	  psrenderer_dtostr(ey_buf, end->y) );
}

static void
draw_polyline(DiaRenderer *self,
	      Point *points, int num_points,
	      Color *line_color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
  int i;
  gchar px_buf[DTOSTR_BUF_SIZE];
  gchar py_buf[DTOSTR_BUF_SIZE];

  lazy_setcolor(renderer,line_color);

  fprintf(renderer->file, "n %s %s m ",
	  psrenderer_dtostr(px_buf, points[0].x),
	  psrenderer_dtostr(py_buf, points[0].y) );

  for (i=1;i<num_points;i++) {
    fprintf(renderer->file, "%s %s l ",
	    psrenderer_dtostr(px_buf, points[i].x),
	    psrenderer_dtostr(py_buf, points[i].y) );
  }

  fprintf(renderer->file, "s\n");
}

static void
psrenderer_polygon(DiaPsRenderer *renderer,
		   Point *points,
		   gint num_points,
		   Color *line_color,
		   gboolean filled)
{
  gint i;
  gchar px_buf[DTOSTR_BUF_SIZE];
  gchar py_buf[DTOSTR_BUF_SIZE];

  lazy_setcolor(renderer, line_color);

  fprintf(renderer->file, "n %s %s m ",
	  psrenderer_dtostr(px_buf, points[0].x),
	  psrenderer_dtostr(py_buf, points[0].y) );


  for (i=1;i<num_points;i++) {
    fprintf(renderer->file, "%s %s l ",
	    psrenderer_dtostr(px_buf, points[i].x),
	    psrenderer_dtostr(py_buf, points[i].y) );
  }
  if (filled)
    fprintf(renderer->file, "ef\n");
  else
    fprintf(renderer->file, "cp s\n");
}

static void
draw_polygon (DiaRenderer *self,
	      Point *points, int num_points,
	      Color *fill, Color *stroke)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
  /* XXX: not optimized to fill and stroke at once */
  if (fill)
    psrenderer_polygon(renderer, points, num_points, fill, TRUE);
  if (stroke)
    psrenderer_polygon(renderer, points, num_points, stroke, FALSE);
}

static void
psrenderer_arc(DiaPsRenderer *renderer,
	       Point *center,
	       real width, real height,
	       real angle1, real angle2,
	       Color *color,
	       gboolean filled)
{
  gchar cx_buf[DTOSTR_BUF_SIZE];
  gchar cy_buf[DTOSTR_BUF_SIZE];
  gchar a1_buf[DTOSTR_BUF_SIZE];
  gchar a2_buf[DTOSTR_BUF_SIZE];
  gchar w_buf[DTOSTR_BUF_SIZE];
  gchar h_buf[DTOSTR_BUF_SIZE];

  lazy_setcolor(renderer, color);

  if (angle2 < angle1) {
    /* swap for counter-clockwise */
    real tmp = angle1;
    angle1 = angle2;
    angle2 = tmp;
  }
  psrenderer_dtostr(cx_buf, (gdouble) center->x);
  psrenderer_dtostr(cy_buf, (gdouble) center->y);
  psrenderer_dtostr(a1_buf, (gdouble) 360.0 - angle1);
  psrenderer_dtostr(a2_buf, (gdouble) 360.0 - angle2);
  psrenderer_dtostr(w_buf, (gdouble) width / 2.0);
  psrenderer_dtostr(h_buf, (gdouble) height / 2.0);

  fprintf(renderer->file, "n ");

  if (filled)
    fprintf(renderer->file, "%s %s m ", cx_buf, cy_buf);

  fprintf(renderer->file, "%s %s %s %s %s %s ellipse %s\n",
	  cx_buf, cy_buf, w_buf, h_buf, a2_buf, a1_buf,
	  filled ? "f" : "s" );
}


static void
draw_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
  psrenderer_arc(renderer, center, width, height, angle1, angle2, color, FALSE);
}

static void
fill_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
  psrenderer_arc(renderer, center, width, height, angle1, angle2, color, TRUE);
}

static void
psrenderer_ellipse(DiaPsRenderer *renderer,
		   Point *center,
		   real width, real height,
		   Color *color,
		   gboolean filled)
{
  gchar cx_buf[DTOSTR_BUF_SIZE];
  gchar cy_buf[DTOSTR_BUF_SIZE];
  gchar w_buf[DTOSTR_BUF_SIZE];
  gchar h_buf[DTOSTR_BUF_SIZE];

  lazy_setcolor(renderer,color);

  fprintf(renderer->file, "n %s %s %s %s 0 360 ellipse %s\n",
	  psrenderer_dtostr(cx_buf, (gdouble) center->x),
	  psrenderer_dtostr(cy_buf, (gdouble) center->y),
	  psrenderer_dtostr(w_buf, (gdouble) width / 2.0),
	  psrenderer_dtostr(h_buf, (gdouble) height / 2.0),
	  filled ? "f" : "cp s" );
}


static void
draw_ellipse (DiaRenderer *self,
              Point       *center,
              real         width,
              real         height,
              Color       *fill,
              Color       *stroke)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER (self);

  if (fill) {
    psrenderer_ellipse (renderer, center, width, height, fill, TRUE);
  }

  if (stroke) {
    psrenderer_ellipse (renderer, center, width, height, stroke, FALSE);
  }
}


static void
psrenderer_bezier (DiaPsRenderer *renderer,
                   BezPoint      *points,
                   gint           numpoints,
                   Color         *color,
                   gboolean       filled)
{
  gint  i;
  gchar p1x_buf[DTOSTR_BUF_SIZE];
  gchar p1y_buf[DTOSTR_BUF_SIZE];
  gchar p2x_buf[DTOSTR_BUF_SIZE];
  gchar p2y_buf[DTOSTR_BUF_SIZE];
  gchar p3x_buf[DTOSTR_BUF_SIZE];
  gchar p3y_buf[DTOSTR_BUF_SIZE];

  lazy_setcolor (renderer,color);

  if (points[0].type != BEZ_MOVE_TO) {
    g_warning ("first BezPoint must be a BEZ_MOVE_TO");
  }

  fprintf (renderer->file, "n %s %s m",
           psrenderer_dtostr (p1x_buf, (gdouble) points[0].p1.x),
           psrenderer_dtostr (p1y_buf, (gdouble) points[0].p1.y) );

  for (i = 1; i < numpoints; i++) {
    switch (points[i].type) {
      case BEZ_MOVE_TO:
        g_warning ("only first BezPoint can be a BEZ_MOVE_TO");
        break;
      case BEZ_LINE_TO:
        fprintf (renderer->file, " %s %s l",
                psrenderer_dtostr (p1x_buf, (gdouble) points[i].p1.x),
                psrenderer_dtostr (p1y_buf, (gdouble) points[i].p1.y) );
        break;
      case BEZ_CURVE_TO:
        fprintf (renderer->file, " %s %s %s %s %s %s c",
                psrenderer_dtostr (p1x_buf, (gdouble) points[i].p1.x),
                psrenderer_dtostr (p1y_buf, (gdouble) points[i].p1.y),
                psrenderer_dtostr (p2x_buf, (gdouble) points[i].p2.x),
                psrenderer_dtostr (p2y_buf, (gdouble) points[i].p2.y),
                psrenderer_dtostr (p3x_buf, (gdouble) points[i].p3.x),
                psrenderer_dtostr (p3y_buf, (gdouble) points[i].p3.y) );
        break;
      default:
        g_return_if_reached ();
    }
  }

  if (filled) {
    fprintf (renderer->file, " ef\n");
  } else {
    fprintf (renderer->file, " s\n");
  }
}


static void
draw_bezier (DiaRenderer *self,
             BezPoint    *points,
             int          numpoints, /* numpoints = 4+3*n, n=>0 */
             Color       *color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER (self);

  psrenderer_bezier (renderer, points, numpoints, color, FALSE);
}


static void
draw_beziergon (DiaRenderer *self,
                /* Last point must be same as first point */
                BezPoint    *points,
                int          numpoints,
                Color       *fill,
                Color       *stroke)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER (self);
  if (fill) {
    psrenderer_bezier (renderer, points, numpoints, fill, TRUE);
  }
  /* XXX: still not closing the path */
  if (stroke) {
    psrenderer_bezier (renderer, points, numpoints, stroke, FALSE);
  }
}


static char*
ps_convert_string (const char *text, DiaContext *ctx)
{
  char *buffer;
  gchar *localestr;
  const gchar *str;
  gint len;
  GError * error = NULL;

  localestr = g_convert (text, -1, "LATIN1", "UTF-8", NULL, NULL, &error);

  if (localestr == NULL) {
    dia_context_add_message (ctx, "Can't convert string %s: %s\n",
                             text, error->message);
    localestr = g_strdup (text);
  }

  /* Escape all '(' and ')':  */
  buffer = g_malloc (2 * strlen (localestr) + 1);
  *buffer = 0;
  str = localestr;
  while (*str != 0) {
    len = strcspn (str,"()\\");
    strncat (buffer, str, len);
    str += len;
    if (*str != 0) {
      strcat (buffer,"\\");
      strncat (buffer, str, 1);
      str++;
    }
  }
  g_clear_pointer (&localestr, g_free);
  return buffer;
}


static void
put_text_alignment (DiaPsRenderer *renderer,
                    Alignment      alignment,
                    Point         *pos)
{
  gchar px_buf[DTOSTR_BUF_SIZE];
  gchar py_buf[DTOSTR_BUF_SIZE];

  switch (alignment) {
    case ALIGN_LEFT:
      fprintf (renderer->file, "%s %s m\n",
               psrenderer_dtostr (px_buf, pos->x),
               psrenderer_dtostr (py_buf, pos->y) );
      break;
    case ALIGN_CENTER:
      fprintf (renderer->file, "dup sw 2 div %s ex sub %s m\n",
               psrenderer_dtostr (px_buf, pos->x),
               psrenderer_dtostr (py_buf, pos->y) );
      break;
    case ALIGN_RIGHT:
      fprintf (renderer->file, "dup sw %s ex sub %s m\n",
               psrenderer_dtostr (px_buf, pos->x),
               psrenderer_dtostr (py_buf, pos->y) );
      break;
    default:
      g_return_if_reached ();
  }
}


static void
draw_string(DiaRenderer *self,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
  Point pos_adj;
  gchar *buffer;

  if (1 > strlen(text))
    return;

  lazy_setcolor(renderer,color);

  buffer = ps_convert_string(text, renderer->ctx);

  fprintf(renderer->file, "(%s) ", buffer);
  g_clear_pointer (&buffer, g_free);

  pos_adj.x = pos->x;
  pos_adj.y = pos->y - dia_font_descent ("", renderer->font, renderer->font_height);
  put_text_alignment (renderer, alignment, &pos_adj);

  fprintf(renderer->file, " gs 1 -1 sc sh gr\n");
}

static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage *image)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER(self);
  int img_width, img_height, img_rowstride;
  int x, y;
  guint8 *rgb_data;
  guint8 *mask_data;
  gchar d1_buf[DTOSTR_BUF_SIZE];
  gchar d2_buf[DTOSTR_BUF_SIZE];

  img_width = dia_image_width(image);
  img_rowstride = dia_image_rowstride(image);
  img_height = dia_image_height(image);

  rgb_data = dia_image_rgb_data(image);
  if (!rgb_data) {
    dia_context_add_message(renderer->ctx, _("Not enough memory for image drawing."));
    return;
  }
  mask_data = dia_image_mask_data(image);

  fprintf(renderer->file, "gs\n");

  /* color output only */
  fprintf(renderer->file, "/pix %i string def\n", img_width * 3);
  fprintf(renderer->file, "%i %i 8\n", img_width, img_height);
  fprintf(renderer->file, "%s %s tr\n",
			   psrenderer_dtostr(d1_buf, point->x),
			   psrenderer_dtostr(d2_buf, point->y) );
  fprintf(renderer->file, "%s %s sc\n",
			   psrenderer_dtostr(d1_buf, width),
			   psrenderer_dtostr(d2_buf, height) );
  fprintf(renderer->file, "[%i 0 0 %i 0 0]\n", img_width, img_height);

  fprintf(renderer->file, "{currentfile pix readhexstring pop}\n");
  fprintf(renderer->file, "false 3 colorimage\n");
  fprintf(renderer->file, "\n");

  if (mask_data) {
    for (y = 0; y < img_height; y++) {
      for (x = 0; x < img_width; x++) {
	int i = y*img_rowstride+x*3;
	int m = y*img_width+x;
        fprintf(renderer->file, "%02x", 255-(mask_data[m]*(255-rgb_data[i])/255));
        fprintf(renderer->file, "%02x", 255-(mask_data[m]*(255-rgb_data[i+1])/255));
        fprintf(renderer->file, "%02x", 255-(mask_data[m]*(255-rgb_data[i+2])/255));
      }
      fprintf(renderer->file, "\n");
    }
  } else {
    for (y = 0; y < img_height; y++) {
      for (x = 0; x < img_width; x++) {
	int i = y*img_rowstride+x*3;
        fprintf(renderer->file, "%02x", (int)(rgb_data[i]));
        fprintf(renderer->file, "%02x", (int)(rgb_data[i+1]));
        fprintf(renderer->file, "%02x", (int)(rgb_data[i+2]));
      }
      fprintf(renderer->file, "\n");
    }
  }
  fprintf(renderer->file, "gr\n");
  fprintf(renderer->file, "\n");

  g_clear_pointer (&rgb_data, g_free);
  g_clear_pointer (&mask_data, g_free);
}

static void
begin_prolog (DiaPsRenderer *renderer)
{
  g_assert(renderer->file != NULL);

  fprintf(renderer->file, "%%%%BeginProlog\n");
  fprintf(renderer->file,
	  "[ /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
	  "/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
	  "/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
	  "/.notdef /.notdef /space /exclam /quotedbl /numbersign /dollar /percent /ampersand /quoteright\n"
	  "/parenleft /parenright /asterisk /plus /comma /hyphen /period /slash /zero /one\n"
	  "/two /three /four /five /six /seven /eight /nine /colon /semicolon\n"
	  "/less /equal /greater /question /at /A /B /C /D /E\n"
	  "/F /G /H /I /J /K /L /M /N /O\n"
	  "/P /Q /R /S /T /U /V /W /X /Y\n"
	  "/Z /bracketleft /backslash /bracketright /asciicircum /underscore /quoteleft /a /b /c\n"
	  "/d /e /f /g /h /i /j /k /l /m\n"
	  "/n /o /p /q /r /s /t /u /v /w\n"
	  "/x /y /z /braceleft /bar /braceright /asciitilde /.notdef /.notdef /.notdef\n"
	  "/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
	  "/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
	  "/.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef /.notdef\n"
	  "/space /exclamdown /cent /sterling /currency /yen /brokenbar /section /dieresis /copyright\n"
	  "/ordfeminine /guillemotleft /logicalnot /hyphen /registered /macron /degree /plusminus /twosuperior /threesuperior\n"
	  "/acute /mu /paragraph /periodcentered /cedilla /onesuperior /ordmasculine /guillemotright /onequarter /onehalf\n"
	  "/threequarters /questiondown /Agrave /Aacute /Acircumflex /Atilde /Adieresis /Aring /AE /Ccedilla\n"
	  "/Egrave /Eacute /Ecircumflex /Edieresis /Igrave /Iacute /Icircumflex /Idieresis /Eth /Ntilde\n"
	  "/Ograve /Oacute /Ocircumflex /Otilde /Odieresis /multiply /Oslash /Ugrave /Uacute /Ucircumflex\n"
	  "/Udieresis /Yacute /Thorn /germandbls /agrave /aacute /acircumflex /atilde /adieresis /aring\n"
	  "/ae /ccedilla /egrave /eacute /ecircumflex /edieresis /igrave /iacute /icircumflex /idieresis\n"
	  "/eth /ntilde /ograve /oacute /ocircumflex /otilde /odieresis /divide /oslash /ugrave\n"
	  "/uacute /ucircumflex /udieresis /yacute /thorn /ydieresis] /isolatin1encoding exch def\n");

  fprintf(renderer->file,
	  "/cp {closepath} bind def\n"
	  "/c {curveto} bind def\n"
	  "/f {fill} bind def\n"
	  "/a {arc} bind def\n"
	  "/ef {eofill} bind def\n"
	  "/ex {exch} bind def\n"
	  "/gr {grestore} bind def\n"
	  "/gs {gsave} bind def\n"
	  "/sa {save} bind def\n"
	  "/rs {restore} bind def\n"
	  "/l {lineto} bind def\n"
	  "/m {moveto} bind def\n"
	  "/rm {rmoveto} bind def\n"
	  "/n {newpath} bind def\n"
	  "/s {stroke} bind def\n"
	  "/sh {show} bind def\n"
	  "/slc {setlinecap} bind def\n"
	  "/slj {setlinejoin} bind def\n"
	  "/slw {setlinewidth} bind def\n"
	  "/srgb {setrgbcolor} bind def\n"
	  "/rot {rotate} bind def\n"
	  "/sc {scale} bind def\n"
	  "/sd {setdash} bind def\n"
	  "/ff {findfont} bind def\n"
	  "/sf {setfont} bind def\n"
	  "/scf {scalefont} bind def\n"
	  "/sw {stringwidth pop} bind def\n"
	  "/tr {translate} bind def\n"

	  "\n/ellipsedict 8 dict def\n"
	  "ellipsedict /mtrx matrix put\n"
	  "/ellipse\n"
	  "{ ellipsedict begin\n"
          "   /endangle exch def\n"
          "   /startangle exch def\n"
          "   /yrad exch def\n"
          "   /xrad exch def\n"
          "   /y exch def\n"
          "   /x exch def"
	  "   /savematrix mtrx currentmatrix def\n"
          "   x y tr xrad yrad sc\n"
          "   0 0 1 startangle endangle arc\n"
          "   savematrix setmatrix\n"
          "   end\n"
	  "} def\n\n"
	  "/mergeprocs {\n"
	  "dup length\n"
	  "3 -1 roll\n"
	  "dup\n"
	  "length\n"
	  "dup\n"
	  "5 1 roll\n"
	  "3 -1 roll\n"
	  "add\n"
	  "array cvx\n"
	  "dup\n"
	  "3 -1 roll\n"
	  "0 exch\n"
	  "putinterval\n"
	  "dup\n"
	  "4 2 roll\n"
	  "putinterval\n"
	  "} bind def\n");

}

/* helper function */
static void
print_reencode_font(FILE *file, char *fontname)
{
  /* Don't reencode the Symbol font, as it doesn't work in latin1 encoding.
   * Instead, just define Symbol-latin1 to be the same as Symbol. */
  if (!strcmp(fontname, "Symbol"))
    fprintf(file,
	    "/%s-latin1\n"
	    "    /%s findfont\n"
	    "definefont pop\n", fontname, fontname);
  else
    fprintf(file,
	    "/%s-latin1\n"
	    "    /%s findfont\n"
	    "    dup length dict begin\n"
	    "	{1 index /FID ne {def} {pop pop} ifelse} forall\n"
	    "	/Encoding isolatin1encoding def\n"
	    "    currentdict end\n"
	    "definefont pop\n", fontname, fontname);
}

static void
dump_fonts (DiaPsRenderer *renderer)
{
  print_reencode_font(renderer->file, "Times-Roman");
  print_reencode_font(renderer->file, "Times-Italic");
  print_reencode_font(renderer->file, "Times-Bold");
  print_reencode_font(renderer->file, "Times-BoldItalic");
  print_reencode_font(renderer->file, "AvantGarde-Gothic");
  print_reencode_font(renderer->file, "AvantGarde-BookOblique");
  print_reencode_font(renderer->file, "AvantGarde-Demi");
  print_reencode_font(renderer->file, "AvantGarde-DemiOblique");
  print_reencode_font(renderer->file, "Bookman-Light");
  print_reencode_font(renderer->file, "Bookman-LightItalic");
  print_reencode_font(renderer->file, "Bookman-Demi");
  print_reencode_font(renderer->file, "Bookman-DemiItalic");
  print_reencode_font(renderer->file, "Courier");
  print_reencode_font(renderer->file, "Courier-Oblique");
  print_reencode_font(renderer->file, "Courier-Bold");
  print_reencode_font(renderer->file, "Courier-BoldOblique");
  print_reencode_font(renderer->file, "Helvetica");
  print_reencode_font(renderer->file, "Helvetica-Oblique");
  print_reencode_font(renderer->file, "Helvetica-Bold");
  print_reencode_font(renderer->file, "Helvetica-BoldOblique");
  print_reencode_font(renderer->file, "Helvetica-Narrow");
  print_reencode_font(renderer->file, "Helvetica-Narrow-Oblique");
  print_reencode_font(renderer->file, "Helvetica-Narrow-Bold");
  print_reencode_font(renderer->file, "Helvetica-Narrow-BoldOblique");
  print_reencode_font(renderer->file, "NewCenturySchlbk-Roman");
  print_reencode_font(renderer->file, "NewCenturySchlbk-Italic");
  print_reencode_font(renderer->file, "NewCenturySchlbk-Bold");
  print_reencode_font(renderer->file, "NewCenturySchlbk-BoldItalic");
  print_reencode_font(renderer->file, "Palatino-Roman");
  print_reencode_font(renderer->file, "Palatino-Italic");
  print_reencode_font(renderer->file, "Palatino-Bold");
  print_reencode_font(renderer->file, "Palatino-BoldItalic");
  print_reencode_font(renderer->file, "Symbol");
  print_reencode_font(renderer->file, "ZapfChancery-MediumItalic");
  print_reencode_font(renderer->file, "ZapfDingbats");
}

static void
end_prolog (DiaPsRenderer *renderer)
{
  gchar d1_buf[DTOSTR_BUF_SIZE];
  gchar d2_buf[DTOSTR_BUF_SIZE];

  if (renderer_is_eps(renderer)) {
    fprintf(renderer->file,
            "%s %s scale\n",
	    psrenderer_dtostr(d1_buf, renderer->scale),
	    psrenderer_dtostr(d2_buf, -renderer->scale) );
    fprintf(renderer->file,
            "%s %s translate\n",
	    psrenderer_dtostr(d1_buf, -renderer->extent.left),
	    psrenderer_dtostr(d2_buf, -renderer->extent.bottom) );
  } else {
    /* done by BoundingBox above */
  }

  fprintf(renderer->file,
	  "%%%%EndProlog\n\n\n");
}

/* constructor */
static void
ps_renderer_init (GTypeInstance *instance, gpointer g_class)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER (instance);

  renderer->file = NULL;

  renderer->lcolor.red = -1.0;

  renderer->is_portrait = TRUE;

  renderer->scale = 28.346;
}

/* GObject stuff */
static void dia_ps_renderer_class_init (DiaPsRendererClass *klass);

static gpointer parent_class = NULL;

GType
dia_ps_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (DiaPsRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) dia_ps_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (DiaPsRenderer),
        0,              /* n_preallocs */
        ps_renderer_init       /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "DiaPsRenderer",
                                            &object_info, 0);
    }

  return object_type;
}

static void
dia_ps_renderer_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  DiaPsRenderer *self = DIA_PS_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      set_font (DIA_RENDERER (self),
                DIA_FONT (g_value_get_object (value)),
                self->font_height);
      break;
    case PROP_FONT_HEIGHT:
      set_font (DIA_RENDERER (self),
                self->font,
                g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_ps_renderer_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  DiaPsRenderer *self = DIA_PS_RENDERER (object);

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
dia_ps_renderer_finalize (GObject *object)
{
  DiaPsRenderer *renderer = DIA_PS_RENDERER (object);

  g_clear_pointer (&renderer->title, g_free);
  g_clear_object (&renderer->font);
  /*  fclose(renderer->file);*/

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
dia_ps_renderer_class_init (DiaPsRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);
  DiaPsRendererClass *ps_renderer_class = DIA_PS_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = dia_ps_renderer_set_property;
  object_class->get_property = dia_ps_renderer_get_property;
  object_class->finalize = dia_ps_renderer_finalize;

  renderer_class->begin_render = begin_render;
  renderer_class->end_render   = end_render;

  renderer_class->set_linewidth  = set_linewidth;
  renderer_class->set_linecaps   = set_linecaps;
  renderer_class->set_linejoin   = set_linejoin;
  renderer_class->set_linestyle  = set_linestyle;
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->draw_line    = draw_line;
  renderer_class->draw_polygon = draw_polygon;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;
  renderer_class->draw_string  = draw_string;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_bezier  = draw_bezier;
  renderer_class->draw_beziergon  = draw_beziergon;
  renderer_class->draw_polyline  = draw_polyline;

  /* ps specific */
  ps_renderer_class->begin_prolog = begin_prolog;
  ps_renderer_class->dump_fonts = dump_fonts;
  ps_renderer_class->end_prolog = end_prolog;

  g_object_class_override_property (object_class, PROP_FONT, "font");
  g_object_class_override_property (object_class, PROP_FONT_HEIGHT, "font-height");
}
