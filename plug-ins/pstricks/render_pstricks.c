/* -*- Mode: C; c-basic-offset: 4 -*- */
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

#include "render_pstricks.h"
#include "message.h"
#include "diagramdata.h"
#include "dia_image.h"
#include "filter.h"
#include "font.h"
#include "dia-version-info.h"

#define POINTS_in_INCH 28.346
#define DTOSTR_BUF_SIZE G_ASCII_DTOSTR_BUF_SIZE
#define pstricks_dtostr(buf,d) \
	g_ascii_formatd(buf,sizeof(buf),"%f",d)

enum {
  PROP_0,
  PROP_FONT,
  PROP_FONT_HEIGHT,
  LAST_PROP
};


static void begin_render(DiaRenderer *self, const DiaRectangle *update);
static void end_render(DiaRenderer *self);
static void set_linewidth(DiaRenderer *self, real linewidth);
static void set_linecaps (DiaRenderer *self, DiaLineCaps  mode);
static void set_linejoin (DiaRenderer *self, DiaLineJoin  mode);
static void set_linestyle(DiaRenderer *self, DiaLineStyle mode, double dash_length);
static void set_fillstyle(DiaRenderer *self, DiaFillStyle mode);
static void set_font(DiaRenderer *self, DiaFont *font, real height);
static void draw_line(DiaRenderer *self,
		      Point *start, Point *end,
		      Color *line_color);
static void draw_polyline(DiaRenderer *self,
			  Point *points, int num_points,
			  Color *line_color);
static void draw_polygon(DiaRenderer *self,
			 Point *points, int num_points,
			 Color *fill, Color *stroke);
static void draw_arc(DiaRenderer *self,
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void fill_arc(DiaRenderer *self,
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void draw_ellipse(DiaRenderer *self,
			 Point *center,
			 real width, real height,
			 Color *fill, Color *stroke);
static void draw_bezier(DiaRenderer *self,
			BezPoint *points,
			int numpoints,
			Color *color);
static void draw_beziergon(DiaRenderer *self,
			   BezPoint *points,
			   int numpoints,
			   Color *fill,
			   Color *stroke);
static void draw_string                  (DiaRenderer  *self,
                                          const char   *text,
                                          Point        *pos,
                                          DiaAlignment  alignment,
                                          Color        *color);
static void draw_image(DiaRenderer *self,
		       Point *point,
		       real width, real height,
		       DiaImage *image);

/*!
 * \brief Advertize special capabilities
 *
 * Some objects drawing adapts to capabilities advertized by the respective
 * renderer. Usually there is a fallback, but generally the real thing should
 * be better.
 *
 * \memberof _PstricksRenderer
 */
static gboolean
is_capable_to (DiaRenderer *renderer, RenderCapability cap)
{
  if (RENDER_HOLES == cap)
    return TRUE; /* ... with under-documented fillstyle=eofill */
  else if (RENDER_ALPHA == cap)
    return FALSE; /* simulate with hatchwidth? */
  else if (RENDER_AFFINE == cap)
    return FALSE; /* maybe by: \translate, \scale, \rotate */
  else if (RENDER_PATTERN == cap)
    return FALSE; /* nope */
  return FALSE;
}

/* GObject stuff */
static void pstricks_renderer_class_init (PstricksRendererClass *klass);

static gpointer parent_class = NULL;

GType
pstricks_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PstricksRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pstricks_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PstricksRenderer),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "PstricksRenderer",
                                            &object_info, 0);
    }

  return object_type;
}

static void
pstricks_renderer_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  PstricksRenderer *self = PSTRICKS_RENDERER (object);

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
pstricks_renderer_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PstricksRenderer *self = PSTRICKS_RENDERER (object);

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
pstricks_renderer_finalize (GObject *object)
{
  PstricksRenderer *self = PSTRICKS_RENDERER (object);

  g_clear_object (&self->font);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
pstricks_renderer_class_init (PstricksRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = pstricks_renderer_set_property;
  object_class->get_property = pstricks_renderer_get_property;
  object_class->finalize = pstricks_renderer_finalize;

  renderer_class->begin_render = begin_render;
  renderer_class->end_render = end_render;
  renderer_class->is_capable_to = is_capable_to;

  renderer_class->set_linewidth = set_linewidth;
  renderer_class->set_linecaps = set_linecaps;
  renderer_class->set_linejoin = set_linejoin;
  renderer_class->set_linestyle = set_linestyle;
  renderer_class->set_fillstyle = set_fillstyle;

  renderer_class->draw_line = draw_line;
  renderer_class->draw_polyline = draw_polyline;

  renderer_class->draw_polygon = draw_polygon;

  renderer_class->draw_arc = draw_arc;
  renderer_class->fill_arc = fill_arc;

  renderer_class->draw_ellipse = draw_ellipse;

  renderer_class->draw_bezier = draw_bezier;
  renderer_class->draw_beziergon = draw_beziergon;

  renderer_class->draw_string = draw_string;

  renderer_class->draw_image = draw_image;

  g_object_class_override_property (object_class, PROP_FONT, "font");
  g_object_class_override_property (object_class, PROP_FONT_HEIGHT, "font-height");
}


static void
set_line_color(PstricksRenderer *renderer,Color *color)
{
    gchar red_buf[DTOSTR_BUF_SIZE];
    gchar green_buf[DTOSTR_BUF_SIZE];
    gchar blue_buf[DTOSTR_BUF_SIZE];

    fprintf(renderer->file, "\\newrgbcolor{dialinecolor}{%s %s %s}%%\n",
	    pstricks_dtostr(red_buf, (gdouble) color->red),
	    pstricks_dtostr(green_buf, (gdouble) color->green),
	    pstricks_dtostr(blue_buf, (gdouble) color->blue) );
    fprintf(renderer->file,"\\psset{linecolor=dialinecolor}\n");
}

static void
set_fill_color(PstricksRenderer *renderer,Color *color)
{
    gchar red_buf[DTOSTR_BUF_SIZE];
    gchar green_buf[DTOSTR_BUF_SIZE];
    gchar blue_buf[DTOSTR_BUF_SIZE];

    fprintf(renderer->file, "\\newrgbcolor{diafillcolor}{%s %s %s}%%\n",
	    pstricks_dtostr(red_buf, (gdouble) color->red),
	    pstricks_dtostr(green_buf, (gdouble) color->green),
	    pstricks_dtostr(blue_buf, (gdouble) color->blue) );
    fprintf(renderer->file,"\\psset{fillcolor=diafillcolor}\n");
}

static void
begin_render(DiaRenderer *self, const DiaRectangle *update)
{
}

static void
end_render(DiaRenderer *self)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);

    fprintf(renderer->file,"}\\endpspicture");
    fclose(renderer->file);
}

static void
set_linewidth(DiaRenderer *self, real linewidth)
{  /* 0 == hairline **/
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
    gchar d_buf[DTOSTR_BUF_SIZE];


    fprintf(renderer->file, "\\psset{linewidth=%scm}\n",
	    pstricks_dtostr(d_buf, (gdouble) linewidth) );
}


static void
set_linecaps (DiaRenderer *self, DiaLineCaps mode)
{
  PstricksRenderer *renderer = PSTRICKS_RENDERER (self);
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
set_linejoin (DiaRenderer *self, DiaLineJoin mode)
{
  PstricksRenderer *renderer = PSTRICKS_RENDERER (self);
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
set_linestyle (DiaRenderer *self, DiaLineStyle mode, double dash_length)
{
  PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
  double hole_width;
  char dash_length_buf[DTOSTR_BUF_SIZE];
  char dot_length_buf[DTOSTR_BUF_SIZE];
  char hole_width_buf[DTOSTR_BUF_SIZE];
  double dot_length;

  if (dash_length < 0.001) {
    dash_length = 0.001;
  }

  /* dot = 20% of len - for some reason not the usual 10% */
  dot_length = dash_length * 0.2;

  switch (mode) {
    case DIA_LINE_STYLE_DEFAULT:
    case DIA_LINE_STYLE_SOLID:
      fprintf (renderer->file, "\\psset{linestyle=solid}\n");
      break;
    case DIA_LINE_STYLE_DASHED:
      pstricks_dtostr (dash_length_buf, dash_length);
      fprintf (renderer->file, "\\psset{linestyle=dashed,dash=%s %s}\n",
               dash_length_buf, dash_length_buf);
      break;
    case DIA_LINE_STYLE_DASH_DOT:
      hole_width = (dash_length - dot_length) / 2.0;
      pstricks_dtostr (hole_width_buf, hole_width);
      pstricks_dtostr (dot_length_buf, dot_length);
      pstricks_dtostr (dash_length_buf, dash_length);
      fprintf (renderer->file, "\\psset{linestyle=dashed,dash=%s %s %s %s}\n",
               dash_length_buf, hole_width_buf, dot_length_buf, hole_width_buf );
      break;
    case DIA_LINE_STYLE_DASH_DOT_DOT:
      hole_width = (dash_length - 2.0*dot_length) / 3.0;
      pstricks_dtostr (hole_width_buf, hole_width);
      pstricks_dtostr (dot_length_buf, dot_length);
      pstricks_dtostr (dash_length_buf, dash_length);
      fprintf (renderer->file, "\\psset{linestyle=dashed,dash=%s %s %s %s %s %s}\n",
               dash_length_buf, hole_width_buf, dot_length_buf, hole_width_buf,
               dot_length_buf, hole_width_buf );
      break;
    case DIA_LINE_STYLE_DOTTED:
      pstricks_dtostr (dot_length_buf, dot_length);
      fprintf (renderer->file, "\\psset{linestyle=dotted,dotsep=%s}\n", dot_length_buf);
      break;
    default:
      g_warning ("Unknown mode %i", mode);
      break;
  }
}


static void
set_fillstyle (DiaRenderer *self, DiaFillStyle mode)
{
  switch (mode) {
    case DIA_FILL_STYLE_SOLID:
      break;
    default:
      g_warning("pstricks_renderer: Unsupported fill mode specified!\n");
  }
}


static void
set_font (DiaRenderer *self, DiaFont *font, real height)
{
  PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
  gchar d_buf[DTOSTR_BUF_SIZE];

  g_clear_object (&renderer->font);
  renderer->font = g_object_ref (font);
  renderer->font_height = height;

  fprintf (renderer->file,
           "\\setfont{%s}{%s}\n",
           dia_font_get_psfontname (font),
           pstricks_dtostr (d_buf, (gdouble) height) );
}

static void
draw_line(DiaRenderer *self,
	  Point *start, Point *end,
	  Color *line_color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
    gchar sx_buf[DTOSTR_BUF_SIZE];
    gchar sy_buf[DTOSTR_BUF_SIZE];
    gchar ex_buf[DTOSTR_BUF_SIZE];
    gchar ey_buf[DTOSTR_BUF_SIZE];

    set_line_color(renderer,line_color);

    fprintf(renderer->file, "\\psline(%s,%s)(%s,%s)\n",
	    pstricks_dtostr(sx_buf,start->x),
	    pstricks_dtostr(sy_buf,start->y),
	    pstricks_dtostr(ex_buf,end->x),
	    pstricks_dtostr(ey_buf,end->y) );
}

static void
draw_polyline(DiaRenderer *self,
	      Point *points, int num_points,
	      Color *line_color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
    int i;
    gchar px_buf[DTOSTR_BUF_SIZE];
    gchar py_buf[DTOSTR_BUF_SIZE];

    set_line_color(renderer,line_color);
    fprintf(renderer->file, "\\psline(%s,%s)",
	    pstricks_dtostr(px_buf,points[0].x),
	    pstricks_dtostr(py_buf,points[0].y) );

    for (i=1;i<num_points;i++) {
	fprintf(renderer->file, "(%s,%s)",
		pstricks_dtostr(px_buf,points[i].x),
		pstricks_dtostr(py_buf,points[i].y) );
    }

    fprintf(renderer->file, "\n");
}

static void
draw_polygon (DiaRenderer *self,
	      Point *points, int num_points,
	      Color *fill, Color *stroke)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
    gint i;
    gchar px_buf[DTOSTR_BUF_SIZE];
    gchar py_buf[DTOSTR_BUF_SIZE];
    const char *style;

    if (fill)
	set_fill_color(renderer, fill);
    if (stroke)
	set_line_color(renderer, stroke);

    if (fill && stroke)
      style = "[fillstyle=eofill,fillcolor=diafillcolor,linecolor=dialinecolor]";
    else if (fill)
      style = "[fillstyle=eofill,fillcolor=diafillcolor]";
    else
      style = "";

    /* The graphics objects all have a starred version (e.g., \psframe*) which
     * draws a solid object whose color is linecolor. [pstricks-doc.pdf p7]
     *
     * Not properly documented, but still working ...
     */
    fprintf(renderer->file, "\\pspolygon%s(%s,%s)",
	    style,
	    pstricks_dtostr(px_buf,points[0].x),
	    pstricks_dtostr(py_buf,points[0].y) );

    for (i=1;i<num_points;i++) {
	fprintf(renderer->file, "(%s,%s)",
		pstricks_dtostr(px_buf,points[i].x),
		pstricks_dtostr(py_buf,points[i].y) );
    }
    fprintf(renderer->file,"\n");
}

static void
pstricks_arc(PstricksRenderer *renderer,
	     Point *center,
	     real width, real height,
	     real angle1, real angle2,
	     Color *color,int filled)
{
    double radius1,radius2;
    gchar cx_buf[DTOSTR_BUF_SIZE];
    gchar cy_buf[DTOSTR_BUF_SIZE];
    gchar r1_buf[DTOSTR_BUF_SIZE];
    gchar r2_buf[DTOSTR_BUF_SIZE];
    gchar sqrt_buf[DTOSTR_BUF_SIZE];
    gchar angle1_buf[DTOSTR_BUF_SIZE];
    gchar angle2_buf[DTOSTR_BUF_SIZE];

    radius1=(double) width/2.0;
    radius2=(double) height/2.0;

    pstricks_dtostr(cx_buf,center->x);
    pstricks_dtostr(cy_buf,center->y);
    pstricks_dtostr(r1_buf,radius1);
    pstricks_dtostr(r2_buf,radius2);
    pstricks_dtostr(sqrt_buf,sqrt(radius1*radius1+radius2*radius2));
    /* counter-clockwise */
    if (angle2 < angle1) {
       real tmp = angle1;
       angle1 = angle2;
       angle2 = tmp;
    }
    pstricks_dtostr(angle1_buf,360.0-angle1);
    pstricks_dtostr(angle2_buf,360.0-angle2);

    set_line_color(renderer,color);

    fprintf(renderer->file,"\\psclip{\\pswedge[linestyle=none,fillstyle=none](%s,%s){%s}{%s}{%s}}\n",
	    cx_buf, cy_buf,
	    sqrt_buf,
	    angle2_buf, angle1_buf);

    fprintf(renderer->file,"\\psellipse%s(%s,%s)(%s,%s)\n",
	    (filled?"*":""),
	    cx_buf, cy_buf,
	    r1_buf, r2_buf);

    fprintf(renderer->file,"\\endpsclip\n");
}

static void
draw_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);

    pstricks_arc(renderer,center,width,height,angle1,angle2,color,0);
}

static void
fill_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);

    pstricks_arc(renderer,center,width,height,angle1,angle2,color,1);
}

static void
pstricks_ellipse(PstricksRenderer *renderer,
		 Point *center,
		 real width, real height,
		 Color *color, gboolean filled)
{
    gchar cx_buf[DTOSTR_BUF_SIZE];
    gchar cy_buf[DTOSTR_BUF_SIZE];
    gchar width_buf[DTOSTR_BUF_SIZE];
    gchar height_buf[DTOSTR_BUF_SIZE];

    set_line_color(renderer,color);

    fprintf(renderer->file, "\\psellipse%s(%s,%s)(%s,%s)\n",
	    (filled?"*":""),
	    pstricks_dtostr(cx_buf,center->x),
	    pstricks_dtostr(cy_buf,center->y),
	    pstricks_dtostr(width_buf,width/2.0),
	    pstricks_dtostr(height_buf,height/2.0) );
}

static void
draw_ellipse(DiaRenderer *self,
	     Point *center,
	     real width, real height,
	     Color *fill, Color *stroke)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);

    if (fill)
	pstricks_ellipse(renderer,center,width,height,fill,TRUE);
    if (stroke)
	pstricks_ellipse(renderer,center,width,height,stroke,FALSE);
}


static void
pstricks_bezier (PstricksRenderer *renderer,
                 BezPoint         *points,
                 gint              numpoints,
                 Color            *fill,
                 Color            *stroke,
                 gboolean          closed)
{
  gint i;
  gchar p1x_buf[DTOSTR_BUF_SIZE];
  gchar p1y_buf[DTOSTR_BUF_SIZE];
  gchar p2x_buf[DTOSTR_BUF_SIZE];
  gchar p2y_buf[DTOSTR_BUF_SIZE];
  gchar p3x_buf[DTOSTR_BUF_SIZE];
  gchar p3y_buf[DTOSTR_BUF_SIZE];

  if (fill) {
    set_fill_color (renderer,fill);
  }

  if (stroke) {
    set_line_color (renderer,stroke);
  }

  fprintf(renderer->file, "\\pscustom{\n");

  if (points[0].type != BEZ_MOVE_TO) {
    g_warning("first BezPoint must be a BEZ_MOVE_TO");
  }

  fprintf (renderer->file, "\\newpath\n\\moveto(%s,%s)\n",
           pstricks_dtostr (p1x_buf, points[0].p1.x),
           pstricks_dtostr (p1y_buf, points[0].p1.y) );

  for (i = 1; i < numpoints; i++) {
    switch (points[i].type) {
      case BEZ_MOVE_TO:
        fprintf (renderer->file, "\\moveto(%s,%s)\n",
                 pstricks_dtostr (p1x_buf,points[i].p1.x),
                 pstricks_dtostr (p1y_buf,points[i].p1.y) );
        break;
      case BEZ_LINE_TO:
        fprintf (renderer->file, "\\lineto(%s,%s)\n",
                 pstricks_dtostr (p1x_buf, points[i].p1.x),
                 pstricks_dtostr (p1y_buf, points[i].p1.y) );
        break;
      case BEZ_CURVE_TO:
        fprintf (renderer->file, "\\curveto(%s,%s)(%s,%s)(%s,%s)\n",
                 pstricks_dtostr (p1x_buf, points[i].p1.x),
                 pstricks_dtostr (p1y_buf, points[i].p1.y),
                 pstricks_dtostr (p2x_buf, points[i].p2.x),
                 pstricks_dtostr (p2y_buf, points[i].p2.y),
                 pstricks_dtostr (p3x_buf, points[i].p3.x),
                 pstricks_dtostr (p3y_buf, points[i].p3.y) );
        break;
      default:
        g_warning ("Unknown type %i", points[i].type);
        break;
    }
  }

  if (closed) {
    fprintf(renderer->file, "\\closepath\n");
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
draw_bezier(DiaRenderer *self,
	    BezPoint *points,
	    int numpoints, /* numpoints = 4+3*n, n=>0 */
	    Color *color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);

    pstricks_bezier(renderer,points,numpoints,NULL,color,FALSE);
}


static void
draw_beziergon (DiaRenderer *self,
		BezPoint *points,
		int numpoints,
		Color *fill,
		Color *stroke)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);

    pstricks_bezier(renderer,points,numpoints,fill,stroke,TRUE);
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
draw_string (DiaRenderer  *self,
             const char   *text,
             Point        *pos,
             DiaAlignment  alignment,
             Color        *color)
{
  PstricksRenderer *renderer = PSTRICKS_RENDERER (self);
  char *escaped = NULL;
  char px_buf[DTOSTR_BUF_SIZE];
  char py_buf[DTOSTR_BUF_SIZE];

  /* only escape the string if it is not starting with \tex */
  if (strncmp (text, "\\tex", 4) != 0)
    escaped = tex_escape_string (text, renderer->ctx);

  set_fill_color (renderer,color);

  fprintf (renderer->file,"\\rput");
  switch (alignment) {
    case DIA_ALIGN_LEFT:
      fprintf(renderer->file,"[l]");
      break;
    case DIA_ALIGN_CENTRE:
      break;
    case DIA_ALIGN_RIGHT:
      fprintf(renderer->file,"[r]");
      break;
    default:
      g_warning ("Unknown alignment %i", alignment);
      break;
  }
  fprintf (renderer->file,
            "(%s,%s){\\psscalebox{1 -1}{%s}}\n",
            pstricks_dtostr (px_buf,pos->x),
            pstricks_dtostr (py_buf,pos->y),
            escaped ? escaped : text);
  g_clear_pointer (&escaped, g_free);
}


static void
draw_image (DiaRenderer *self,
            Point       *point,
            double       width,
            double       height,
            DiaImage    *image)
{
  PstricksRenderer *renderer = PSTRICKS_RENDERER (self);
  int img_width, img_height;
  int v;
  int                 x, y;
  unsigned char      *ptr;
  guint8 *rgb_data;
  double points_in_inch = POINTS_in_INCH;
  char points_in_inch_buf[DTOSTR_BUF_SIZE];
  char px_buf[DTOSTR_BUF_SIZE];
  char py_buf[DTOSTR_BUF_SIZE];
  char width_buf[DTOSTR_BUF_SIZE];
  char height_buf[DTOSTR_BUF_SIZE];

  pstricks_dtostr (points_in_inch_buf, points_in_inch);

  img_width = dia_image_width(image);
  img_height = dia_image_height(image);

  rgb_data = dia_image_rgb_data(image);
  if (!rgb_data) {
    dia_context_add_message(renderer->ctx, _("Not enough memory for image drawing."));
    return;
  }

  fprintf (renderer->file, "\\pscustom{\\code{gsave\n");

  if (1) { /* Color output */
	fprintf(renderer->file, "/pix %i string def\n", img_width * 3);
	fprintf(renderer->file, "/grays %i string def\n", img_width);
	fprintf(renderer->file, "/npixls 0 def\n");
	fprintf(renderer->file, "/rgbindx 0 def\n");
	fprintf(renderer->file, "%s %s scale\n",
		points_in_inch_buf, points_in_inch_buf);
	fprintf(renderer->file, "%s %s translate\n",
		pstricks_dtostr(px_buf,point->x),
		pstricks_dtostr(py_buf,point->y) );
	fprintf(renderer->file, "%s %s scale\n",
		pstricks_dtostr(width_buf,width),
		pstricks_dtostr(height_buf,height) );
	fprintf(renderer->file, "%i %i 8\n", img_width, img_height);
	fprintf(renderer->file, "[%i 0 0 %i 0 0]\n", img_width, img_height);
	fprintf(renderer->file, "{currentfile pix readhexstring pop}\n");
	fprintf(renderer->file, "false 3 colorimage\n");
	/*    fprintf(renderer->file, "\n"); */
	for (y = 0; y < img_height; y++) {
	    ptr = rgb_data + y * dia_image_rowstride(image);
	    for (x = 0; x < img_width; x++) {
		fprintf(renderer->file, "%02x", (int)(*ptr++));
		fprintf(renderer->file, "%02x", (int)(*ptr++));
		fprintf(renderer->file, "%02x", (int)(*ptr++));
	    }
	    fprintf(renderer->file, "\n");
	}
    } else { /* Grayscale */
	fprintf(renderer->file, "/pix %i string def\n", img_width);
	fprintf(renderer->file, "/grays %i string def\n", img_width);
	fprintf(renderer->file, "/npixls 0 def\n");
	fprintf(renderer->file, "/rgbindx 0 def\n");
	fprintf(renderer->file, "%s %s scale\n",
		points_in_inch_buf, points_in_inch_buf);
	fprintf(renderer->file, "%s %s translate\n",
		pstricks_dtostr(px_buf,point->x),
		pstricks_dtostr(py_buf,point->y) );
	fprintf(renderer->file, "%s %s scale\n",
		pstricks_dtostr(width_buf,width),
		pstricks_dtostr(height_buf,height) );
	fprintf(renderer->file, "%i %i 8\n", img_width, img_height);
	fprintf(renderer->file, "[%i 0 0 %i 0 0]\n", img_width, img_height);
	fprintf(renderer->file, "{currentfile pix readhexstring pop}\n");
	fprintf(renderer->file, "image\n");
	fprintf(renderer->file, "\n");
	ptr = rgb_data;
	for (y = 0; y < img_height; y++) {
	    for (x = 0; x < img_width; x++) {
		v = (int)(*ptr++);
		v += (int)(*ptr++);
		v += (int)(*ptr++);
		v /= 3;
		fprintf(renderer->file, "%02x", v);
	    }
	    fprintf(renderer->file, "\n");
	}
    }
    /*  fprintf(renderer->file, "%f %f scale\n", 1.0, 1.0/ratio);*/
    fprintf(renderer->file, "grestore\n");
    fprintf(renderer->file, "}}\n");

  g_clear_pointer (&rgb_data, g_free);
}


/* --- export filter interface --- */
static gboolean
export_pstricks (DiagramData *data,
                 DiaContext  *ctx,
                 const char  *filename,
                 const char  *diafilename,
                 void        *user_data)
{
  PstricksRenderer *renderer;
  FILE *file;
  time_t time_now;
  DiaRectangle *extent;
  const char *name;
  char el_buf[DTOSTR_BUF_SIZE];
  char er_buf[DTOSTR_BUF_SIZE];
  char eb_buf[DTOSTR_BUF_SIZE];
  char et_buf[DTOSTR_BUF_SIZE];
  char scale1_buf[DTOSTR_BUF_SIZE];
  char scale2_buf[DTOSTR_BUF_SIZE];

  Color initial_color;

  file = g_fopen(filename, "wb");

  if (file == NULL) {
    dia_context_add_message_with_errno (ctx,
                                        errno,
                                        _("Can't open output file %s"),
                                        dia_context_get_filename (ctx));
    return FALSE;
  }

  renderer = g_object_new (PSTRICKS_TYPE_RENDERER, NULL);

    renderer->pagenum = 1;
    renderer->file = file;
    renderer->ctx = ctx;

    time_now  = time(NULL);
    extent = &data->extents;

    name = g_get_user_name();

    fprintf(file,
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
	dia_version_string(),
	ctime(&time_now),
	name);

  fprintf (renderer->file,"\\pspicture(%s,%s)(%s,%s)\n",
           pstricks_dtostr (el_buf, extent->left * data->paper.scaling),
           pstricks_dtostr (eb_buf, -extent->bottom * data->paper.scaling),
           pstricks_dtostr (er_buf, extent->right * data->paper.scaling),
           pstricks_dtostr (et_buf, -extent->top * data->paper.scaling) );
  fprintf (renderer->file,"\\psscalebox{%s %s}{\n",
           pstricks_dtostr (scale1_buf, data->paper.scaling),
           pstricks_dtostr (scale2_buf, -data->paper.scaling) );

  initial_color.red = 0.0;
  initial_color.green = 0.0;
  initial_color.blue = 0.0;
  set_line_color (renderer, &initial_color);

  initial_color.red= 1.0;
  initial_color.green= 1.0;
  initial_color.blue= 1.0;
  set_fill_color (renderer, &initial_color);

  data_render (data, DIA_RENDERER (renderer), NULL, NULL, NULL);

  g_clear_object (&renderer);

  return TRUE;
}


static const gchar *extensions[] = { "tex", NULL };
DiaExportFilter pstricks_export_filter = {
  N_("TeX PSTricks macros"),
  extensions,
  export_pstricks,
  NULL,
  "pstricks-tex"
};
