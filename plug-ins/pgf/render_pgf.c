/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * render_pgf.c: Exporting module/plug-in to TeX PGF package
 * Copyright (C) 2005 Moritz Kirmse
 * Originally derived from render_pstricks.c (pstricks plug-in)
 * Copyright (C) 2000 Jacek Pliszka <pliszka@fuw.edu.pl>
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
Implementation choices:

The PGF package is written with the idea in mind that the typesetting software will
(usually) do a better and more beautiful job than the user.

This export filter respects thes and therefore some export features are special:

- _if_ text is NOT escaped: (La)TeX will interpret this code.
Therefore text size wil not be consistent and bounding boxes will not work.
When text is inside a box (as for example in the UML modules), the result wil not
be as expected. (Some text would break TeX processing altogther, see below.)

-some arrows from the PGF-library are used (latex, stealth and to)
for these arrows, size selection and bounding box won't work

-the rounded corners for polyline-paths are slightly different:
	the radius measure is not the radius of the arc,
	but the length cut off at the angle from each segment of the polyline
however the PGF native corner rounding was implemented, since I observed a discontinuous
line thickness at the interface between the segments and arcs

TODO:
-Image exporting function
-Implement PGF basic layer throughout
-Implement all the Arrow styles
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

#include <glib.h>
#include <glib/gstdio.h>

#include "render_pgf.h"
#include "diagramdata.h"
#include "dia_image.h"
#include "filter.h"
#include "arrows.h"
#include "dia-version-info.h"

#define POINTS_in_INCH 28.346
#define DTOSTR_BUF_SIZE G_ASCII_DTOSTR_BUF_SIZE
#define pgf_dtostr(buf,d) \
	g_ascii_formatd(buf,sizeof(buf),"%f",d)
#define pgf_itostr(buf,i) \
	g_sprintf(buf,"%d",i)

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
static void draw_rounded_polyline (DiaRenderer *self,
                        Point *points, int num_points,
                        Color *color, real radius);
static void draw_polygon(DiaRenderer *self,
			 Point *points, int num_points,
			 Color *fill, Color *stroke);
static void draw_rounded_rect(DiaRenderer *self,
			      Point *ul_corner, Point *lr_corner,
			      Color *fill, Color *stroke, real radius);
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
static void draw_string                (DiaRenderer  *self,
                                        const char   *text,
                                        Point        *pos,
                                        DiaAlignment  alignment,
                                        Color        *color);
static void draw_image(DiaRenderer *self,
		       Point *point,
		       real width, real height,
		       DiaImage *image);

static void draw_line_with_arrows(DiaRenderer *renderer, Point *start, Point *end,
                                  real line_width, Color *line_color,
                                  Arrow *start_arrow, Arrow *end_arrow);
static void draw_arc_with_arrows(DiaRenderer *renderer, Point *start, Point *end, Point *midpoint,
                                 real line_width, Color *color,
                                 Arrow *start_arrow, Arrow *end_arrow);
static void draw_polyline_with_arrows(DiaRenderer *renderer, Point *points, int num_points,
                                     real line_width, Color *color,
                                     Arrow *start_arrow, Arrow *end_arrow);
static void draw_rounded_polyline_with_arrows(DiaRenderer *renderer,
                                     Point *points, int num_points, real line_width, Color *color,
				     Arrow *start_arrow, Arrow *end_arrow, real radius);
static void draw_bezier_with_arrows(DiaRenderer *renderer, BezPoint *points, int num_points,
                                   real line_width, Color *color,
                                   Arrow *start_arrow, Arrow *end_arrow);

/*store the higher level arrow functions for arrows not (yet) implemented in this PGF macro*/
void (*orig_draw_line_with_arrows)  (DiaRenderer *renderer, Point *start, Point *end,
                                  real line_width, Color *line_color,
                                  Arrow *start_arrow, Arrow *end_arrow);

void (*orig_draw_arc_with_arrows)  (DiaRenderer *renderer, Point *start, Point *end, Point *midpoint,
                                 real line_width, Color *color,
                                 Arrow *start_arrow, Arrow *end_arrow);

void (*orig_draw_polyline_with_arrows) (DiaRenderer *renderer, Point *points, int num_points,
                                     real line_width, Color *color,
                                     Arrow *start_arrow, Arrow *end_arrow);

void (*orig_draw_rounded_polyline_with_arrows) (DiaRenderer *renderer,
                                     Point *points, int num_points, real line_width, Color *color,
				     Arrow *start_arrow, Arrow *end_arrow, real radius);

void (*orig_draw_bezier_with_arrows) (DiaRenderer *renderer, BezPoint *points, int num_points,
                                   real line_width, Color *color,
                                   Arrow *start_arrow, Arrow *end_arrow);

enum {
  PROP_0,
  PROP_FONT,
  PROP_FONT_HEIGHT,
  LAST_PROP
};


/*!
 * \brief Advertize special capabilities
 *
 * Some objects drawing adapts to capabilities advertized by the respective
 * renderer. Usually there is a fallback, but generally the real thing should
 * be better.
 *
 * \memberof _PgfRenderer
 */
static gboolean
is_capable_to (DiaRenderer *renderer, RenderCapability cap)
{
  if (RENDER_HOLES == cap)
    return TRUE;
  else if (RENDER_ALPHA == cap)
    return TRUE;
  else if (RENDER_AFFINE == cap)
    return FALSE; /* not now */
  else if (RENDER_PATTERN == cap)
    return FALSE; /* might be possible, too */
  return FALSE;
}

static void pgf_renderer_class_init (PgfRendererClass *klass);

static gpointer parent_class = NULL;

GType
pgf_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PgfRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pgf_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PgfRenderer),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "PGFRenderer",
                                            &object_info, 0);
    }

  return object_type;
}

static void
pgf_renderer_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  PgfRenderer *self = PGF_RENDERER (object);

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
pgf_renderer_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  PgfRenderer *self = PGF_RENDERER (object);

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
pgf_renderer_finalize (GObject *object)
{
  PgfRenderer *self = PGF_RENDERER (object);

  g_clear_object (&self->font);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
pgf_renderer_class_init (PgfRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = pgf_renderer_set_property;
  object_class->get_property = pgf_renderer_get_property;
  object_class->finalize = pgf_renderer_finalize;

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

  /*problem: the round angles behave slightly differently under PGF:
	the radius measure is not the radius, but the length cut off at the angle
	from each segment of the polyline
	however, to keep the inherited round corner generation, this command can be commented out*/
  renderer_class->draw_rounded_polyline = draw_rounded_polyline;

  renderer_class->draw_polygon = draw_polygon;

  renderer_class->draw_rounded_rect = draw_rounded_rect;

  renderer_class->draw_arc = draw_arc;
  renderer_class->fill_arc = fill_arc;

  renderer_class->draw_ellipse = draw_ellipse;

  renderer_class->draw_bezier = draw_bezier;
  renderer_class->draw_beziergon = draw_beziergon;

/* to keep the dia arrows instead of the native PGF ones, comment out this block of commands */
  orig_draw_line_with_arrows = renderer_class->draw_line_with_arrows;
  renderer_class->draw_line_with_arrows = draw_line_with_arrows;
  orig_draw_arc_with_arrows = renderer_class->draw_arc_with_arrows;
  renderer_class->draw_arc_with_arrows = draw_arc_with_arrows;
  orig_draw_polyline_with_arrows = renderer_class->draw_polyline_with_arrows;
  renderer_class->draw_polyline_with_arrows = draw_polyline_with_arrows;
  orig_draw_rounded_polyline_with_arrows = renderer_class->draw_rounded_polyline_with_arrows;
  renderer_class->draw_rounded_polyline_with_arrows = draw_rounded_polyline_with_arrows;
  orig_draw_bezier_with_arrows = renderer_class->draw_bezier_with_arrows;
  renderer_class->draw_bezier_with_arrows = draw_bezier_with_arrows;

  renderer_class->draw_string = draw_string;

  renderer_class->draw_image = draw_image;

  g_object_class_override_property (object_class, PROP_FONT, "font");
  g_object_class_override_property (object_class, PROP_FONT_HEIGHT, "font-height");
}


static void
set_line_color(PgfRenderer *renderer,Color *color)
{
    gchar red_buf[DTOSTR_BUF_SIZE];
    gchar green_buf[DTOSTR_BUF_SIZE];
    gchar blue_buf[DTOSTR_BUF_SIZE];

    fprintf(renderer->file, "\\definecolor{dialinecolor}{rgb}{%s, %s, %s}\n",
	    pgf_dtostr(red_buf, (gdouble) color->red),
	    pgf_dtostr(green_buf, (gdouble) color->green),
	    pgf_dtostr(blue_buf, (gdouble) color->blue) );
    fprintf(renderer->file,"\\pgfsetstrokecolor{dialinecolor}\n");
    fprintf(renderer->file,"\\pgfsetstrokeopacity{%s}\n",
	    pgf_dtostr(red_buf, (gdouble) color->alpha));
}

static void
set_fill_color(PgfRenderer *renderer,Color *color)
{
    gchar red_buf[DTOSTR_BUF_SIZE];
    gchar green_buf[DTOSTR_BUF_SIZE];
    gchar blue_buf[DTOSTR_BUF_SIZE];

    fprintf(renderer->file, "\\definecolor{diafillcolor}{rgb}{%s, %s, %s}\n",
	    pgf_dtostr(red_buf, (gdouble) color->red),
	    pgf_dtostr(green_buf, (gdouble) color->green),
	    pgf_dtostr(blue_buf, (gdouble) color->blue) );
    fprintf(renderer->file,"\\pgfsetfillcolor{diafillcolor}\n");
    fprintf(renderer->file,"\\pgfsetfillopacity{%s}\n",
	    pgf_dtostr(red_buf, (gdouble) color->alpha));
}

static void
begin_render(DiaRenderer *self, const DiaRectangle *update)
{
}


static void
end_render (DiaRenderer *self)
{
  PgfRenderer *renderer = PGF_RENDERER (self);

  fprintf (renderer->file, "\\end{tikzpicture}\n");
  fclose (renderer->file);
}


static void
set_linewidth (DiaRenderer *self, real linewidth)
{
  /* 0 == hairline (thinnest possible line on device) **/
  PgfRenderer *renderer = PGF_RENDERER (self);
  gchar d_buf[DTOSTR_BUF_SIZE];


   fprintf (renderer->file, "\\pgfsetlinewidth{%s\\du}\n",
            pgf_dtostr(d_buf, (gdouble) linewidth) );
}


static void
set_linecaps (DiaRenderer *self, DiaLineCaps mode)
{
  PgfRenderer *renderer = PGF_RENDERER (self);

  switch (mode) {
    case DIA_LINE_CAPS_BUTT:
      fprintf (renderer->file, "\\pgfsetbuttcap\n");
      break;
    case DIA_LINE_CAPS_ROUND:
      fprintf (renderer->file, "\\pgfsetroundcap\n");
      break;
    case DIA_LINE_CAPS_PROJECTING:
      fprintf (renderer->file, "\\pgfsetrectcap\n");
      break;
    case DIA_LINE_CAPS_DEFAULT:
    default:
      fprintf (renderer->file, "\\pgfsetbuttcap\n");
  }
}


static void
set_linejoin (DiaRenderer *self, DiaLineJoin mode)
{
  PgfRenderer *renderer = PGF_RENDERER (self);
  /* int ps_mode; */

  switch (mode) {
    case DIA_LINE_JOIN_DEFAULT:
    case DIA_LINE_JOIN_MITER:
      fprintf (renderer->file, "\\pgfsetmiterjoin\n");
      break;
    case DIA_LINE_JOIN_ROUND:
      fprintf (renderer->file, "\\pgfsetroundjoin\n");
      break;
    case DIA_LINE_JOIN_BEVEL:
      fprintf (renderer->file, "\\pgfsetbeveljoin\n");
      break;
    default:
      fprintf (renderer->file, "\\pgfsetmiterjoin\n");
  }
}


static void
set_linestyle (DiaRenderer *self, DiaLineStyle mode, double dash_length)
{
  PgfRenderer *renderer = PGF_RENDERER(self);
  double hole_width;
  char dash_length_buf[DTOSTR_BUF_SIZE];
  char dot_length_buf[DTOSTR_BUF_SIZE];
  char hole_width_buf[DTOSTR_BUF_SIZE];
  double dot_length;

  if (dash_length < 0.001) {
    dash_length = 0.001;
  }

  /* dot = 20% of len - elsewhere 10% */
  dot_length = dash_length * 0.2;

  switch(mode) {
    case DIA_LINE_STYLE_DEFAULT:
    case DIA_LINE_STYLE_SOLID:
    default:
      fprintf (renderer->file, "\\pgfsetdash{}{0pt}\n");
      break;
    case DIA_LINE_STYLE_DASHED:
      pgf_dtostr (dash_length_buf, dash_length);
      fprintf (renderer->file, "\\pgfsetdash{{%s\\du}{%s\\du}}{0\\du}\n",
               dash_length_buf,
               dash_length_buf);
      break;
    case DIA_LINE_STYLE_DASH_DOT:
      hole_width = ( dash_length -  dot_length) / 2.0;
      pgf_dtostr (hole_width_buf, hole_width);
      pgf_dtostr (dot_length_buf, dot_length);
      pgf_dtostr (dash_length_buf, dash_length);
      fprintf (renderer->file, "\\pgfsetdash{{%s\\du}{%s\\du}{%s\\du}{%s\\du}}{0cm}\n",
               dash_length_buf,
               hole_width_buf,
               dot_length_buf,
               hole_width_buf );
      break;
    case DIA_LINE_STYLE_DASH_DOT_DOT:
      hole_width = ( dash_length - 2.0* dot_length) / 3.0;
      pgf_dtostr (hole_width_buf, hole_width);
      pgf_dtostr (dot_length_buf, dot_length);
      pgf_dtostr (dash_length_buf, dash_length);
      fprintf (renderer->file, "\\pgfsetdash{{%s\\du}{%s\\du}{%s\\du}{%s\\du}{%s\\du}{%s\\du}}{0cm}\n",
               dash_length_buf,
               hole_width_buf,
               dot_length_buf,
               hole_width_buf,
               dot_length_buf,
               hole_width_buf );
      break;
    case DIA_LINE_STYLE_DOTTED:
      pgf_dtostr (dot_length_buf, dot_length);
      fprintf (renderer->file, "\\pgfsetdash{{\\pgflinewidth}{%s\\du}}{0cm}\n", dot_length_buf);
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
      g_warning ("%s: Unsupported fill mode specified!",
                 G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
  }
}


static void
set_font(DiaRenderer *self, DiaFont *font, real height)
{
  PgfRenderer *renderer = PGF_RENDERER(self);

  g_clear_object (&renderer->font);
  renderer->font = g_object_ref (font);

  fprintf(renderer->file, "%% setfont left to latex\n");
}

static void
draw_line(DiaRenderer *self,
	  Point *start, Point *end,
	  Color *line_color)
{
    PgfRenderer *renderer = PGF_RENDERER(self);
    gchar sx_buf[DTOSTR_BUF_SIZE];
    gchar sy_buf[DTOSTR_BUF_SIZE];
    gchar ex_buf[DTOSTR_BUF_SIZE];
    gchar ey_buf[DTOSTR_BUF_SIZE];

    set_line_color(renderer,line_color);

    fprintf(renderer->file, "\\draw (%s\\du,%s\\du)--(%s\\du,%s\\du);\n",
	    pgf_dtostr(sx_buf,start->x),
	    pgf_dtostr(sy_buf,start->y),
	    pgf_dtostr(ex_buf,end->x),
	    pgf_dtostr(ey_buf,end->y) );
}

static void
draw_polyline(DiaRenderer *self,
	      Point *points, int num_points,
	      Color *line_color)
{
    PgfRenderer *renderer = PGF_RENDERER(self);
    int i;
    gchar px_buf[DTOSTR_BUF_SIZE];
    gchar py_buf[DTOSTR_BUF_SIZE];

    set_line_color(renderer,line_color);
    fprintf(renderer->file, "\\draw (%s\\du,%s\\du)",
	    pgf_dtostr(px_buf,points[0].x),
	    pgf_dtostr(py_buf,points[0].y) );

    for (i=1;i<num_points;i++) {
	fprintf(renderer->file, "--(%s\\du,%s\\du)",
		pgf_dtostr(px_buf,points[i].x),
		pgf_dtostr(py_buf,points[i].y) );
    }

    fprintf(renderer->file, ";\n");
}

/*problem: the round angles behave slightly differently under PGF:
	the radius measure is not the radius, but the length cut off at the angle
	from each segment of the polyline*/
static void draw_rounded_polyline (DiaRenderer *self,
                        Point *points, int num_points,
                        Color *color, real radius)
{
	gchar rad_buf[DTOSTR_BUF_SIZE];

	PgfRenderer *renderer = PGF_RENDERER(self);
	pgf_dtostr(rad_buf, (gdouble) radius);
	fprintf(renderer->file, "{\\pgfsetcornersarced{\\pgfpoint{%s\\du}{%s\\du}}",
					rad_buf, rad_buf);
	draw_polyline(self,points,num_points,color);
	fprintf(renderer->file, "}");
}

static void
pgf_polygon(PgfRenderer *renderer,
		 Point *points, gint num_points,
		 Color *line_color, gboolean filled)
{
    gint i;
    gchar px_buf[DTOSTR_BUF_SIZE];
    gchar py_buf[DTOSTR_BUF_SIZE];

    if (!filled) {set_line_color(renderer,line_color);}
    else {set_fill_color(renderer,line_color);}


    fprintf(renderer->file, "\\%s (%s\\du,%s\\du)",
	    (filled?"fill":"draw"),
	    pgf_dtostr(px_buf,points[0].x),
	    pgf_dtostr(py_buf,points[0].y) );

    for (i=1;i<num_points;i++) {
	fprintf(renderer->file, "--(%s\\du,%s\\du)",
		pgf_dtostr(px_buf,points[i].x),
		pgf_dtostr(py_buf,points[i].y) );
    }
    fprintf(renderer->file,"--cycle;\n");
}

static void
draw_polygon(DiaRenderer *self,
	     Point *points, int num_points,
	     Color *fill, Color *stroke)
{
    PgfRenderer *renderer = PGF_RENDERER(self);

    /* XXX: not optimized to fill and stroke at once */
    if (fill)
      pgf_polygon(renderer,points,num_points,fill,TRUE);
    if (stroke)
      pgf_polygon(renderer,points,num_points,stroke,FALSE);
}

static void
pgf_rect(PgfRenderer *renderer,
	      Point *ul_corner, Point *lr_corner,
	      Color *color, gboolean filled)
{
    gchar ulx_buf[DTOSTR_BUF_SIZE];
    gchar uly_buf[DTOSTR_BUF_SIZE];
    gchar lrx_buf[DTOSTR_BUF_SIZE];
    gchar lry_buf[DTOSTR_BUF_SIZE];

    if (!filled) {set_line_color(renderer,color);}
    else {set_fill_color(renderer,color);}

    pgf_dtostr(ulx_buf, (gdouble) ul_corner->x);
    pgf_dtostr(uly_buf, (gdouble) ul_corner->y);
    pgf_dtostr(lrx_buf, (gdouble) lr_corner->x);
    pgf_dtostr(lry_buf, (gdouble) lr_corner->y);

    fprintf(renderer->file, "\\%s (%s\\du,%s\\du)--(%s\\du,%s\\du)--(%s\\du,%s\\du)--(%s\\du,%s\\du)--cycle;\n",
	    (filled?"fill":"draw"),
	    ulx_buf, uly_buf,
	    ulx_buf, lry_buf,
	    lrx_buf, lry_buf,
	    lrx_buf, uly_buf );
}

static void
stroke_rounded_rect(DiaRenderer *self,
			      Point *ul_corner, Point *lr_corner,
			      Color *color, real radius)
{
	gchar rad_buf[DTOSTR_BUF_SIZE];

	PgfRenderer *renderer = PGF_RENDERER(self);
	pgf_dtostr(rad_buf, (gdouble) radius);
	fprintf(renderer->file, "{\\pgfsetcornersarced{\\pgfpoint{%s\\du}{%s\\du}}",
					rad_buf, rad_buf);
	pgf_rect(renderer,ul_corner,lr_corner,color,FALSE);
	fprintf(renderer->file, "}");
}

static void
fill_rounded_rect(DiaRenderer *self,
			      Point *ul_corner, Point *lr_corner,
			      Color *color, real radius)
{
	PgfRenderer *renderer = PGF_RENDERER(self);
	gchar rad_buf[DTOSTR_BUF_SIZE];
	pgf_dtostr(rad_buf, (gdouble) radius);
	fprintf(renderer->file, "{\\pgfsetcornersarced{\\pgfpoint{%s\\du}{%s\\du}}",
					rad_buf, rad_buf);
	pgf_rect(renderer,ul_corner,lr_corner,color,TRUE);
	fprintf(renderer->file, "}");
}

static void
draw_rounded_rect(DiaRenderer *self,
			      Point *ul_corner, Point *lr_corner,
			      Color *fill, Color *stroke, real radius)
{
	if (fill)
		fill_rounded_rect (self, ul_corner, lr_corner, fill, radius);
	if (stroke)
		stroke_rounded_rect (self, ul_corner, lr_corner, stroke, radius);
}

static void
pgf_arc(PgfRenderer *renderer,
	     Point *center,
	     real width, real height,
	     real angle1, real angle2,
	     Color *color,int filled)
{
    double radius1,radius2;
    int ang1,ang2;
    gchar stx_buf[DTOSTR_BUF_SIZE];
    gchar sty_buf[DTOSTR_BUF_SIZE];
    gchar cx_buf[DTOSTR_BUF_SIZE];
    gchar cy_buf[DTOSTR_BUF_SIZE];
    gchar r1_buf[DTOSTR_BUF_SIZE];
    gchar r2_buf[DTOSTR_BUF_SIZE];
    gchar sqrt_buf[DTOSTR_BUF_SIZE];
    gchar angle1_buf[DTOSTR_BUF_SIZE];
    gchar angle2_buf[DTOSTR_BUF_SIZE];

    radius1=(double) width/2.0;
    radius2=(double) height/2.0;

    /* counter-clockwise */
    if (angle2 < angle1) {
       real tmp = angle1;
       angle1 = angle2;
       angle2 = tmp;
    }
    pgf_dtostr(stx_buf,center->x+ radius1*cos(angle1*.017453));
    pgf_dtostr(sty_buf,center->y- radius2*sin(angle1*.017453));
    pgf_dtostr(cx_buf,center->x);
    pgf_dtostr(cy_buf,center->y);
    pgf_dtostr(r1_buf,radius1);
    pgf_dtostr(r2_buf,radius2);
    pgf_dtostr(sqrt_buf,sqrt(radius1*radius1+radius2*radius2));
    ang1=(int)angle1;
    ang2=(int)angle2;
    ang2=ang1+(360+ang2-ang1)%360;
    pgf_itostr(angle1_buf,360-ang1);
    pgf_itostr(angle2_buf,360-ang2);

    if (!filled) {set_line_color(renderer,color);}
    else {set_fill_color(renderer,color);}


    fprintf(renderer->file,"\\pgfpathmoveto{\\pgfpoint{%s\\du}{%s\\du}}\n",
	    stx_buf, sty_buf);
    fprintf(renderer->file,"\\pgfpatharc{%s}{%s}{%s\\du and %s\\du}\n",
	    angle1_buf, angle2_buf,
	    r1_buf, r2_buf);

    if (filled)
	fprintf(renderer->file, "\\pgfusepath{fill}\n");
    else
	fprintf(renderer->file, "\\pgfusepath{stroke}\n");
}

static void
draw_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
    PgfRenderer *renderer = PGF_RENDERER(self);

    pgf_arc(renderer,center,width,height,angle1,angle2,color,0);
}

static void
fill_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
    PgfRenderer *renderer = PGF_RENDERER(self);

    pgf_arc(renderer,center,width,height,angle1,angle2,color,1);
}

static void
pgf_ellipse(PgfRenderer *renderer,
		 Point *center,
		 real width, real height,
		 Color *color, gboolean filled)
{
    gchar cx_buf[DTOSTR_BUF_SIZE];
    gchar cy_buf[DTOSTR_BUF_SIZE];
    gchar width_buf[DTOSTR_BUF_SIZE];
    gchar height_buf[DTOSTR_BUF_SIZE];

    if (!filled) {set_line_color(renderer,color);}
    else {set_fill_color(renderer,color);}

/* "\\%sdraw (%s\\du,%s\\du) ellipse (%s\\du and %s\\du)\n", */
    fprintf(renderer->file,
	    "\\pgfpathellipse{\\pgfpoint{%s\\du}{%s\\du}}"
	                    "{\\pgfpoint{%s\\du}{0\\du}}"
	                    "{\\pgfpoint{0\\du}{%s\\du}}\n"
	    "\\pgfusepath{%s}\n",
	    pgf_dtostr(cx_buf,center->x),
	    pgf_dtostr(cy_buf,center->y),
	    pgf_dtostr(width_buf,width/2.0),
	    pgf_dtostr(height_buf,height/2.0),
	    (filled?"fill":"stroke") );
}

static void
draw_ellipse(DiaRenderer *self,
	     Point *center,
	     real width, real height,
	     Color *fill, Color *stroke)
{
    PgfRenderer *renderer = PGF_RENDERER(self);

    if (fill)
	pgf_ellipse(renderer,center,width,height,fill,TRUE);
    if (stroke)
	pgf_ellipse(renderer,center,width,height,stroke,FALSE);
}


static void
pgf_bezier (PgfRenderer *renderer,
            BezPoint    *points,
            gint         numpoints,
            Color       *fill,
            Color       *stroke,
            gboolean     closed)
{
  gint i;
  gchar p1x_buf[DTOSTR_BUF_SIZE];
  gchar p1y_buf[DTOSTR_BUF_SIZE];
  gchar p2x_buf[DTOSTR_BUF_SIZE];
  gchar p2y_buf[DTOSTR_BUF_SIZE];
  gchar p3x_buf[DTOSTR_BUF_SIZE];
  gchar p3y_buf[DTOSTR_BUF_SIZE];

  if (fill) {
    set_fill_color (renderer, fill);
  }
  if (stroke) {
    set_line_color (renderer, stroke);
  }

  if (points[0].type != BEZ_MOVE_TO) {
    g_warning("first BezPoint must be a BEZ_MOVE_TO");
  }

  for (i = 0; i < numpoints; i++) {
    switch (points[i].type) {
      case BEZ_MOVE_TO:
        fprintf (renderer->file,
                 "\\pgfpathmoveto{\\pgfpoint{%s\\du}{%s\\du}}\n",
                 pgf_dtostr (p1x_buf, points[i].p1.x),
                 pgf_dtostr (p1y_buf, points[i].p1.y) );
        break;
      case BEZ_LINE_TO:
        fprintf (renderer->file,
                 "\\pgfpathlineto{\\pgfpoint{%s\\du}{%s\\du}}\n",
                 pgf_dtostr (p1x_buf,points[i].p1.x),
                 pgf_dtostr (p1y_buf,points[i].p1.y) );
        break;
      case BEZ_CURVE_TO:
        fprintf (renderer->file,
                 "\\pgfpathcurveto{\\pgfpoint{%s\\du}{%s\\du}}"
                 "{\\pgfpoint{%s\\du}{%s\\du}}"
                 "{\\pgfpoint{%s\\du}{%s\\du}}\n",
                 pgf_dtostr (p1x_buf, points[i].p1.x),
                 pgf_dtostr (p1y_buf, points[i].p1.y),
                 pgf_dtostr (p2x_buf, points[i].p2.x),
                 pgf_dtostr (p2y_buf, points[i].p2.y),
                 pgf_dtostr (p3x_buf, points[i].p3.x),
                 pgf_dtostr (p3y_buf, points[i].p3.y) );
        break;
      default:
        g_return_if_reached ();
    }
  }

  if (closed) {
    fprintf(renderer->file, "\\pgfpathclose\n");
  }

  if (fill && stroke) {
    fprintf(renderer->file, "\\pgfusepath{fill,stroke}\n");
  } else if (fill) {
    fprintf (renderer->file, "\\pgfusepath{fill}\n");
    /*	fill[fillstyle=solid,fillcolor=diafillcolor,linecolor=diafillcolor]}\n"); */
  } else if (stroke) {
    fprintf (renderer->file, "\\pgfusepath{stroke}\n");
  }
}


static void
draw_bezier (DiaRenderer *self,
             BezPoint    *points,
             int          numpoints, /* numpoints = 4+3*n, n=>0 */
             Color       *color)
{
  PgfRenderer *renderer = PGF_RENDERER(self);

  pgf_bezier (renderer, points, numpoints, NULL, color, FALSE);
}


static void
draw_beziergon (DiaRenderer *self,
                BezPoint    *points,
                int          numpoints,
                Color       *fill,
                Color       *stroke)
{
  PgfRenderer *renderer = PGF_RENDERER (self);

  pgf_bezier (renderer, points, numpoints, fill, stroke, TRUE);
}


static int
set_arrows (PgfRenderer *renderer, Arrow *start_arrow, Arrow *end_arrow)
{
  int modified = 2|1; /*=3*/
  fprintf (renderer->file, "%% was here!!!\n");
  switch (start_arrow->type) {
    case ARROW_NONE:
      break;
    case ARROW_LINES:
      fprintf (renderer->file, "\\pgfsetarrowsstart{to}\n");
      break;
    case ARROW_FILLED_TRIANGLE:
      fprintf (renderer->file, "\\pgfsetarrowsstart{latex}\n");
      break;
    case ARROW_FILLED_CONCAVE:
      fprintf (renderer->file, "\\pgfsetarrowsstart{stealth}\n");
      break;
    case ARROW_HOLLOW_TRIANGLE:
    case ARROW_HOLLOW_DIAMOND:
    case ARROW_FILLED_DIAMOND:
    case ARROW_HALF_HEAD:
    case ARROW_SLASHED_CROSS:
    case ARROW_FILLED_ELLIPSE:
    case ARROW_HOLLOW_ELLIPSE:
    case ARROW_DOUBLE_HOLLOW_TRIANGLE:
    case ARROW_DOUBLE_FILLED_TRIANGLE:
    case ARROW_UNFILLED_TRIANGLE:
    case ARROW_FILLED_DOT:
    case ARROW_DIMENSION_ORIGIN:
    case ARROW_BLANKED_DOT:
    case ARROW_FILLED_BOX:
    case ARROW_BLANKED_BOX:
    case ARROW_SLASH_ARROW:
    case ARROW_INTEGRAL_SYMBOL:
    case ARROW_CROW_FOOT:
    case ARROW_CROSS:
    case ARROW_BLANKED_CONCAVE:
    case ARROW_ROUNDED:
    case ARROW_HALF_DIAMOND:
    case ARROW_OPEN_ROUNDED:
    case ARROW_FILLED_DOT_N_TRIANGLE:
    case ARROW_ONE_OR_MANY:
    case ARROW_NONE_OR_MANY:
    case ARROW_ONE_OR_NONE:
    case ARROW_ONE_EXACTLY:
    case ARROW_BACKSLASH:
    case ARROW_THREE_DOTS:
    case MAX_ARROW_TYPE:
    default:
      modified ^= 2;
  }

  if (modified & 2) {
    start_arrow->type = ARROW_NONE;
  }

  switch (end_arrow->type) {
    case ARROW_NONE:
      break;
    case ARROW_LINES:
      fprintf (renderer->file, "\\pgfsetarrowsend{to}\n");
      break;
    case ARROW_FILLED_TRIANGLE:
      fprintf (renderer->file, "\\pgfsetarrowsend{latex}\n");
      break;
    case ARROW_FILLED_CONCAVE:
      fprintf( renderer->file, "\\pgfsetarrowsend{stealth}\n");
      break;
    case ARROW_HOLLOW_TRIANGLE:
    case ARROW_HOLLOW_DIAMOND:
    case ARROW_FILLED_DIAMOND:
    case ARROW_HALF_HEAD:
    case ARROW_SLASHED_CROSS:
    case ARROW_FILLED_ELLIPSE:
    case ARROW_HOLLOW_ELLIPSE:
    case ARROW_DOUBLE_HOLLOW_TRIANGLE:
    case ARROW_DOUBLE_FILLED_TRIANGLE:
    case ARROW_UNFILLED_TRIANGLE:
    case ARROW_FILLED_DOT:
    case ARROW_DIMENSION_ORIGIN:
    case ARROW_BLANKED_DOT:
    case ARROW_FILLED_BOX:
    case ARROW_BLANKED_BOX:
    case ARROW_SLASH_ARROW:
    case ARROW_INTEGRAL_SYMBOL:
    case ARROW_CROW_FOOT:
    case ARROW_CROSS:
    case ARROW_BLANKED_CONCAVE:
    case ARROW_ROUNDED:
    case ARROW_HALF_DIAMOND:
    case ARROW_OPEN_ROUNDED:
    case ARROW_FILLED_DOT_N_TRIANGLE:
    case ARROW_ONE_OR_MANY:
    case ARROW_NONE_OR_MANY:
    case ARROW_ONE_OR_NONE:
    case ARROW_ONE_EXACTLY:
    case ARROW_BACKSLASH:
    case ARROW_THREE_DOTS:
    case MAX_ARROW_TYPE:
    default:
       modified ^= 1;
  }

  if (modified & 1) {
    end_arrow->type = ARROW_NONE;
  }

  return modified; /* =0 if no native arrow is used */
}

static void
draw_line_with_arrows(DiaRenderer *self, Point *start, Point *end,
                                  real line_width, Color *line_color,
                                  Arrow *start_arrow, Arrow *end_arrow)
{
    int nat_arr;
    Arrow st_arrow;
    Arrow e_arrow;
    PgfRenderer *renderer = PGF_RENDERER(self);

    if (start_arrow != NULL) {
	st_arrow = *start_arrow;
    } else {
	st_arrow.type = ARROW_NONE;
    }

    if (end_arrow != NULL) {
	e_arrow = *end_arrow;
    } else {
	e_arrow.type = ARROW_NONE;
    }
    fprintf(renderer->file, "{\n");
    set_fill_color(renderer, line_color);
    nat_arr = set_arrows(renderer, &st_arrow, &e_arrow);
    if (nat_arr != 0)
    {
/*      static void set_linewidth(self, line_width);
      draw_line(self, start, end, line_width, color);*/
      orig_draw_line_with_arrows(self, start, end, line_width, line_color, NULL, NULL);
    }
    fprintf(renderer->file, "}\n");
    if (nat_arr != 3)
    {
      orig_draw_line_with_arrows(self, start, end, line_width, line_color, &st_arrow, &e_arrow);
    }

}

static void
draw_arc_with_arrows(DiaRenderer *self, Point *start, Point *end, Point *midpoint,
                                 real line_width, Color *color,
                                 Arrow *start_arrow, Arrow *end_arrow)
{
    int nat_arr;
    Arrow st_arrow;
    Arrow e_arrow;
    PgfRenderer *renderer = PGF_RENDERER(self);

    if (start_arrow != NULL) {
	st_arrow = *start_arrow;
    } else {
	st_arrow.type = ARROW_NONE;
    }

    if (end_arrow != NULL) {
	e_arrow = *end_arrow;
    } else {
	e_arrow.type = ARROW_NONE;
    }

    fprintf(renderer->file, "{\n");
    set_fill_color(renderer, color);
    nat_arr = set_arrows(renderer, &st_arrow, &e_arrow);
    if (nat_arr != 0)
    {
      orig_draw_arc_with_arrows(self, start, end, midpoint, line_width, color, NULL, NULL);
    }
    fprintf(renderer->file, "}\n");
    if (nat_arr != 3)
    {
      orig_draw_arc_with_arrows(self, start, end, midpoint, line_width, color, &st_arrow, &e_arrow);
    }
}

static void
draw_polyline_with_arrows(DiaRenderer *self, Point *points, int num_points,
                                     real line_width, Color *color,
                                     Arrow *start_arrow, Arrow *end_arrow)
{
    int nat_arr;
    Arrow st_arrow;
    Arrow e_arrow;
    PgfRenderer *renderer = PGF_RENDERER(self);

    if (start_arrow != NULL) {
	st_arrow = *start_arrow;
    } else {
	st_arrow.type = ARROW_NONE;
    }

    if (end_arrow != NULL) {
	e_arrow = *end_arrow;
    } else {
	e_arrow.type = ARROW_NONE;
    }

    fprintf(renderer->file, "{\n");
    set_fill_color(renderer, color);
    nat_arr = set_arrows(renderer, &st_arrow, &e_arrow);
    if (nat_arr != 0)
    {
      orig_draw_polyline_with_arrows(self, points, num_points, line_width, color, NULL, NULL);
    }
    fprintf(renderer->file, "}\n");
    if (nat_arr != 3)
    {
      orig_draw_polyline_with_arrows(self, points, num_points, line_width, color, &st_arrow, &e_arrow);
    }
}

static void
draw_rounded_polyline_with_arrows(DiaRenderer *self,
                                     Point *points, int num_points, real line_width, Color *color,
				     Arrow *start_arrow, Arrow *end_arrow, real radius)
{
    int nat_arr;
    Arrow st_arrow;
    Arrow e_arrow;
    PgfRenderer *renderer = PGF_RENDERER(self);

    if (start_arrow != NULL) {
	st_arrow = *start_arrow;
    } else {
	st_arrow.type = ARROW_NONE;
    }

    if (end_arrow != NULL) {
	e_arrow = *end_arrow;
    } else {
	e_arrow.type = ARROW_NONE;
    }

    fprintf(renderer->file, "{\n");
    set_fill_color(renderer, color);
    nat_arr = set_arrows(renderer, &st_arrow, &e_arrow);
    if (nat_arr != 0)
    {
      orig_draw_rounded_polyline_with_arrows(self, points, num_points,
                     line_width, color, NULL, NULL, radius);
    }
    fprintf(renderer->file, "}\n");
    if (nat_arr != 3)
    {
      orig_draw_rounded_polyline_with_arrows(self, points, num_points,
                     line_width, color, &st_arrow, &e_arrow, radius);
    }
}

static void
draw_bezier_with_arrows(DiaRenderer *self, BezPoint *points, int num_points,
                                   real line_width, Color *color,
                                   Arrow *start_arrow, Arrow *end_arrow)
{
    int nat_arr;
    Arrow st_arrow;
    Arrow e_arrow;
    PgfRenderer *renderer = PGF_RENDERER(self);

    if (start_arrow != NULL) {
	st_arrow = *start_arrow;
    } else {
	st_arrow.type = ARROW_NONE;
    }

    if (end_arrow != NULL) {
	e_arrow = *end_arrow;
    } else {
	e_arrow.type = ARROW_NONE;
    }

    fprintf(renderer->file, "{\n");
    set_fill_color(renderer, color);
    nat_arr = set_arrows(renderer, &st_arrow, &e_arrow);
    if (nat_arr != 0)
    {
      orig_draw_bezier_with_arrows(self, points, num_points, line_width, color, &st_arrow, &e_arrow);
    }
    fprintf(renderer->file, "}\n");
    if (nat_arr != 3)
    {
      orig_draw_bezier_with_arrows(self, points, num_points, line_width, color, NULL, NULL);
    }
}





/* Do we really want to do this?  What if the text is intended as
 * TeX text?  Jacek says leave it as a TeX string.  TeX uses should know
 * how to escape stuff anyway.  Later versions will get an export option.
 *
 * Later (hb): given the UML issue bug #112377 an manually tweaking
 * is not possible as the # is added before anything a user can add. So IMO
 * we need to want this. If there later (much later?) is an export option it
 * probably shouldn't produce broken output either ...
 */
static char *
tex_escape_string (const char *src, DiaContext *ctx)
{
  int len = g_utf8_strlen (src, -1);
  GString *dest = g_string_sized_new (len);
  char *p;

  if (!g_utf8_validate (src, -1, NULL)) {
    dia_context_add_message (ctx, _("Not valid UTF-8"));
    return g_strdup (src);
  }

  /* Do not escape TeX macros if string is marked properly -- "$...$" or "{...}" */
  if (len > 2) {
    char b = src[0];
    char e = src[len - 1];
    if ((b == '$' && e == '$') || (b == '{' && e == '}')) {
      return g_strdup (src);
    }
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
        g_string_append (dest, "\\ensuremath{\\backslash}");
        break;
      case '{':
        g_string_append (dest, "\\{");
        break;
      case '}':
        g_string_append (dest, "\\}");
        break;
      case '[':
        g_string_append (dest, "\\ensuremath{[}");
        break;
      case ']':
        g_string_append (dest, "\\ensuremath{]}");
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
             DiaAlignment alignment,
             Color        *color)
{
  PgfRenderer *renderer = PGF_RENDERER (self);
  char *escaped = tex_escape_string (text, renderer->ctx);
  char px_buf[DTOSTR_BUF_SIZE];
  char py_buf[DTOSTR_BUF_SIZE];

  set_line_color (renderer,color);
  set_fill_color (renderer,color);

  fprintf (renderer->file,"\\node");
  switch (alignment) {
    case DIA_ALIGN_LEFT:
      fprintf (renderer->file,
        "[anchor=base west,inner sep=0pt,outer sep=0pt,color=dialinecolor]");
      break;
    case DIA_ALIGN_CENTRE:
      fprintf (renderer->file,
        "[anchor=base,inner sep=0pt, outer sep=0pt,color=dialinecolor]");
      break;
    case DIA_ALIGN_RIGHT:
      fprintf (renderer->file,
        "[anchor=base east,inner sep=0pt, outer sep=0pt,color=dialinecolor]");
      break;
    default:
      g_return_if_reached ();
  }
  fprintf (renderer->file, " at (%s\\du,%s\\du){%s};\n",
            pgf_dtostr (px_buf,pos->x),
            pgf_dtostr (py_buf,pos->y),
            escaped);
  g_clear_pointer (&escaped, g_free);
}

static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage *image)
{
    PgfRenderer *renderer = PGF_RENDERER(self);

    fprintf(renderer->file, "%% image rendering not supported");
}

/* --- export filter interface --- */
static gboolean
export_pgf(DiagramData *data, DiaContext *ctx,
	   const gchar *filename, const gchar *diafilename,
	   void* user_data)
{
    PgfRenderer *renderer;
    FILE *file;
    time_t time_now;
    const char *name;
    gchar scale1_buf[DTOSTR_BUF_SIZE];
    gchar scale2_buf[DTOSTR_BUF_SIZE];

    Color initial_color;

    file = g_fopen(filename, "wb");

    if (file == NULL) {
	dia_context_add_message_with_errno (ctx, errno, _("Can't open output file %s"),
					    dia_context_get_filename(ctx));
	return FALSE;
    }

    renderer = g_object_new(PGF_TYPE_RENDERER, NULL);

    renderer->pagenum = 1;
    renderer->file = file;
    renderer->ctx = ctx;

    time_now  = time(NULL);
    name = g_get_user_name();

    fprintf(file,
	"%% Graphic for TeX using PGF\n"
	"%% Title: %s\n"
	"%% Creator: Dia v%s\n"
	"%% CreationDate: %s"
	"%% For: %s\n"
	"%% \\usepackage{tikz}\n"
	"%% The following commands are not supported in PSTricks at present\n"
	"%% We define them conditionally, so when they are implemented,\n"
	"%% this pgf file will use them.\n"

	"\\ifx\\du\\undefined\n"
  	"  \\newlength{\\du}\n"
	"\\fi\n"
	"\\setlength{\\du}{15\\unitlength}\n"
	"\\begin{tikzpicture}[even odd rule]\n",
	diafilename,
	dia_version_string(),
	ctime(&time_now),
	name);

    fprintf(renderer->file,"\\pgftransformxscale{%s}\n"
	                   "\\pgftransformyscale{%s}\n",
	    pgf_dtostr(scale1_buf,data->paper.scaling),
	    pgf_dtostr(scale2_buf,-data->paper.scaling) );

    initial_color.red=0.;
    initial_color.green=0.;
    initial_color.blue=0.;
    initial_color.alpha=1.;
    set_line_color(renderer,&initial_color);

    initial_color.red=1.;
    initial_color.green=1.;
    initial_color.blue=1.;
    set_fill_color(renderer,&initial_color);

  data_render (data, DIA_RENDERER (renderer), NULL, NULL, NULL);

  g_clear_object (&renderer);

  return TRUE;
}

static const gchar *extensions[] = { "tex", NULL };
DiaExportFilter pgf_export_filter = {
    N_("LaTeX PGF macros"),
    extensions,
    export_pgf,
    NULL,
    "pgf-tex"
};
