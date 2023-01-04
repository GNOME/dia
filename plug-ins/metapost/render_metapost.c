/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * render_metapost.c: Exporting module/plug-in to TeX Metapost
 * Copyright (C) 2001 Chris Sperandio
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
 * This was basically borrowed from the pstricks plug-in.
 *
 * TODO:
 * 1. Include file support.
 * 2. Linestyles really need tweaking.
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

#include "render_metapost.h"
#include "diagramdata.h"
#include "filter.h"
#include "dia_image.h"
#include "font.h"
#include "text.h"
#include "textline.h"
#include "dia-version-info.h"

#define POINTS_in_INCH 28.346
#define DTOSTR_BUF_SIZE G_ASCII_DTOSTR_BUF_SIZE
#define mp_dtostr(buf,d) \
	g_ascii_formatd(buf, sizeof(buf), "%f", d)

/* An entry in the font-mapping lookup table. */
typedef struct _font_lookup_entry {
    char            *dia_name;
        /* Dia's name for the font. */

    char            *mp_name;
        /* Second argument for the \usefont command in the embedded TeX
         * we'll pass to MetaPost. */

    real            size_mult;
        /* Converts from a Dia font size to a MetaPost font "scaling factor".
         * If x is the size of your Dia font, then the size of your MetaPost
         * font will be (x * size_mult) / (10 pts), and you'll have to use
         * (x * size_mult) as the "scale" argument to the label() command. */

} _font_lookup_entry;

#define DEFAULT_MP_FONT "cmr"
#define DEFAULT_MP_WEIGHT "m"
#define DEFAULT_MP_SLANT "n"

#define DEFAULT_SIZE_MULT (1.9F)

#define MAX_FONT_NAME_LEN 256

/* A lookup table for converting Dia fonts into MetaPost fonts. */
/* TODO: Make the fonts in this table map more closely. */
static _font_lookup_entry FONT_LOOKUP_TABLE[] =
{
    /* Since Dia doesn't usually have a "computer modern" font, we map Century
     * Schoolbook to that font. */
    {"century schoolbook l", DEFAULT_MP_FONT, DEFAULT_SIZE_MULT},

    /* Sans serif fonts go to phv. */
    {"arial",       "phv", 2.05F},
    {"helvetica",   "phv", 2.05F},
    {"sans",        "phv", 2.05F},

    /* Monospace fonts map to Computer Modern Typewriter. */
    {"courier",         "cmtt", 2.3F},
    {"courier new",     "cmtt", 2.3F},
    {"monospace",       "cmtt", 2.3F},
    {NULL, NULL, 0.0F}
        /* Terminator. */
};


/* An entry in the font weight lookup table. */
typedef struct _weight_lookup_entry {
    DiaFontStyle    weight;
        /* Mask your style with DIA_FONT_STYLE_GET_WEIGHT() and compare
         * to this... */

    char            *mp_weight;
        /* Third argument for the \usefont command in the embedded TeX
         * we'll pass to MetaPost. */
} _weight_lookup_entry;

#define STYLE_TERMINATOR ((DiaFontStyle)0xffffffff)
static _weight_lookup_entry WEIGHT_LOOKUP_TABLE[] =
{
    {DIA_FONT_ULTRALIGHT,       "m"},
    {DIA_FONT_LIGHT,            "m"},
    {DIA_FONT_WEIGHT_NORMAL,    "m"},
    {DIA_FONT_MEDIUM,           "m"},
    {DIA_FONT_DEMIBOLD,         "b"},
    {DIA_FONT_BOLD,             "b"},
    {DIA_FONT_ULTRABOLD,        "b"},
    {DIA_FONT_HEAVY,            "b"},
    {STYLE_TERMINATOR, NULL}
        /* Terminator */
};

/* An entry in the font slant lookup table. */
typedef struct _slant_lookup_entry {
    DiaFontStyle    slant;
    char            *mp_slant;
} _slant_lookup_entry;

static _slant_lookup_entry SLANT_LOOKUP_TABLE[] =
{
    {DIA_FONT_NORMAL,   "n"},
    {DIA_FONT_OBLIQUE,  "sl"},
    {DIA_FONT_ITALIC,   "it"},
    {STYLE_TERMINATOR, NULL}
        /* Terminator */
};

enum {
  PROP_0,
  PROP_FONT,
  PROP_FONT_HEIGHT,
  LAST_PROP
};


static void end_draw_op(MetapostRenderer *renderer);
static void draw_with_linestyle(MetapostRenderer *renderer);

static void
end_draw_op(MetapostRenderer *renderer)
{
    gchar d1_buf[DTOSTR_BUF_SIZE];
    gchar d2_buf[DTOSTR_BUF_SIZE];
    gchar d3_buf[DTOSTR_BUF_SIZE];

    /* the following pencircle seems not to work well with line caps, but using
     *	renderer->saved_line_cap == DIA_LINE_CAPS_ROUND ? "pencircle" : "pensquare",
     * doesn't make it anz better
     */
    fprintf(renderer->file, "\n    withpen pencircle scaled %sx",
            g_ascii_formatd(d1_buf, sizeof(d1_buf), "%5.4f", (gdouble) renderer->line_width) );

    if (!color_equals(&renderer->color, &color_black))
        fprintf(renderer->file, "\n    withcolor (%s, %s, %s)",
                g_ascii_formatd(d1_buf, sizeof(d1_buf), "%5.4f", (gdouble) renderer->color.red),
                g_ascii_formatd(d2_buf, sizeof(d2_buf), "%5.4f", (gdouble) renderer->color.green),
                g_ascii_formatd(d3_buf, sizeof(d3_buf), "%5.4f", (gdouble) renderer->color.blue) );

    draw_with_linestyle(renderer);
    fprintf(renderer->file, ";\n");
}

static void
set_line_color(MetapostRenderer *renderer,Color *color)
{
    gchar red_buf[DTOSTR_BUF_SIZE];
    gchar green_buf[DTOSTR_BUF_SIZE];
    gchar blue_buf[DTOSTR_BUF_SIZE];

    renderer->color = *color;
    fprintf(renderer->file, "%% set_line_color %s, %s, %s\n",
            mp_dtostr(red_buf, (gdouble)color->red),
	    mp_dtostr(green_buf, (gdouble)color->green),
	    mp_dtostr(blue_buf, (gdouble)color->blue) );
}

static void
begin_render(DiaRenderer *self, const DiaRectangle *update)
{
}

static void
end_render(DiaRenderer *self)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);

    fprintf(renderer->file,"endfig;\n");
    fprintf(renderer->file,"end;\n");
    fclose(renderer->file);
}

/*!
 * \brief Advertise special capabilities
 *
 * Some objects drawing adapts to capabilities advertised by the respective
 * renderer. Usually there is a fallback, but generally the real thing should
 * be better.
 *
 * Holes tested with http://www.tlhiv.org/mppreview/ and render-test as well as
 * convert-to-path: apparently Metapost is not capable to render holes or
 * draw_beziergon is doing something wrong.
 *
 * \memberof _MetapostRenderer
 */
static gboolean
is_capable_to (DiaRenderer *renderer, RenderCapability cap)
{
  if (RENDER_HOLES == cap)
    return FALSE; /* maybe with unfill? */
  else if (RENDER_ALPHA == cap)
    return FALSE; /* not now */
  else if (RENDER_AFFINE == cap)
    return FALSE; /* not now */
  else if (RENDER_PATTERN == cap)
    return FALSE; /* might be possible, too */
  return FALSE;
}


static void
set_linewidth(DiaRenderer *self, real linewidth)
{  /* 0 == hairline **/
    gchar buf[DTOSTR_BUF_SIZE];
    MetapostRenderer *renderer = METAPOST_RENDERER (self);

    fprintf(renderer->file, "%% set_linewidth %s\n", mp_dtostr(buf,linewidth) );
    renderer->line_width = linewidth;
}


static void
set_linecaps (DiaRenderer *self, DiaLineCaps mode)
{
  MetapostRenderer *renderer = METAPOST_RENDERER (self);

  if (mode == renderer->saved_line_cap) {
    return;
  }

  switch (mode) {
    case DIA_LINE_CAPS_DEFAULT:
    case DIA_LINE_CAPS_BUTT:
      fprintf (renderer->file, "linecap:=butt;\n");
      break;
    case DIA_LINE_CAPS_ROUND:
      fprintf (renderer->file, "linecap:=rounded;\n");
      break;
    case DIA_LINE_CAPS_PROJECTING:
      /* is this right? */
      fprintf(renderer->file, "linecap:=squared;\n");
      break;
    default:
      fprintf (renderer->file, "linecap:=squared;\n");
  }

  renderer->saved_line_cap = mode;
}


static void
set_linejoin (DiaRenderer *self, DiaLineJoin mode)
{
  MetapostRenderer *renderer = METAPOST_RENDERER (self);

  if (mode == renderer->saved_line_join) {
    return;
  }

  switch (mode) {
    case DIA_LINE_JOIN_DEFAULT:
    case DIA_LINE_JOIN_MITER:
      fprintf (renderer->file, "linejoin:=mitered;\n");
      break;
    case DIA_LINE_JOIN_ROUND:
      fprintf (renderer->file, "linejoin:=rounded;\n");
      break;
    case DIA_LINE_JOIN_BEVEL:
      fprintf (renderer->file, "linejoin:=beveled;\n");
      break;
    default:
      /* noop; required at least for msvc */;
  }

  renderer->saved_line_join = mode;
}


static void
set_linestyle (DiaRenderer *self, DiaLineStyle mode, double dash_length)
{
  MetapostRenderer *renderer = METAPOST_RENDERER (self);

  renderer->saved_line_style = mode;
  /* dot = 10% of len */
  if (dash_length < 0.001) {
    dash_length = 0.001;
  }

  renderer->dash_length = dash_length;
  renderer->dot_length = dash_length * 0.1;
}


static void
draw_with_linestyle (MetapostRenderer *renderer)
{
  real hole_width;
  gchar dash_length_buf[DTOSTR_BUF_SIZE];
  gchar dot_lenght_buf[DTOSTR_BUF_SIZE];
  gchar hole_width_buf[DTOSTR_BUF_SIZE];

  switch (renderer->saved_line_style) {
    case DIA_LINE_STYLE_DASHED:
      mp_dtostr (dash_length_buf, renderer->dash_length);
      fprintf (renderer->file, "\n    dashed dashpattern (on %sx off %sx)",
               dash_length_buf, dash_length_buf);
      break;
    case DIA_LINE_STYLE_DASH_DOT:
      hole_width = (renderer->dash_length - renderer->dot_length) / 2.0;

      mp_dtostr (dash_length_buf, renderer->dash_length);
      mp_dtostr (dot_lenght_buf, renderer->dot_length);
      mp_dtostr (hole_width_buf, hole_width);

      fprintf (renderer->file,
               "\n    dashed dashpattern (on %sx off %sx on %sx off %sx)",
               dash_length_buf, hole_width_buf,
               dot_lenght_buf, hole_width_buf);
      break;
    case DIA_LINE_STYLE_DASH_DOT_DOT:
      hole_width = (renderer->dash_length - 2.0*renderer->dot_length) / 3.0;

      mp_dtostr (dash_length_buf, renderer->dash_length);
      mp_dtostr (dot_lenght_buf, renderer->dot_length);
      mp_dtostr (hole_width_buf, hole_width);

      fprintf (renderer->file,
               "\n    dashed dashpattern (on %sx off %sx on %sx off %sx on %sx off %sx)",
               dash_length_buf, hole_width_buf,
               dot_lenght_buf, hole_width_buf,
               dot_lenght_buf, hole_width_buf );
      break;
    case DIA_LINE_STYLE_DOTTED:
      hole_width = renderer->dot_length * 5.0;
      mp_dtostr (dot_lenght_buf, renderer->dot_length);
      mp_dtostr (hole_width_buf, hole_width);

      fprintf (renderer->file, "\n    dashed dashpattern (on %sx off %sx)",
               dot_lenght_buf, hole_width_buf);
      break;
    case DIA_LINE_STYLE_DEFAULT:
    case DIA_LINE_STYLE_SOLID:
    default:
      break;
  }
}


static void
set_fillstyle (DiaRenderer *self, DiaFillStyle mode)
{
  /*MetapostRenderer *renderer = METAPOST_RENDERER (self);*/

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
    MetapostRenderer *renderer = METAPOST_RENDERER (self);
    int i = -1;

    /* Determine what font Dia is using, so we can convert to the closest
     * matching MetaPost font. */
    char *dia_font_name = (char*)dia_font_get_family(font);
    const DiaFontStyle dia_font_style = dia_font_get_style(font);
    const real dia_font_height = height;

    /* Catch default Dia fonts. */
    if (DIA_FONT_STYLE_GET_FAMILY(dia_font_style) == DIA_FONT_SANS) {
        dia_font_name = "sans";
    } else if (DIA_FONT_STYLE_GET_FAMILY(dia_font_style) == DIA_FONT_SERIF) {
        dia_font_name = "serif";
    } else if (DIA_FONT_STYLE_GET_FAMILY(dia_font_style)
            == DIA_FONT_MONOSPACE)
    {
        dia_font_name = "monospace";
    }

    /* Start out with some sensible defaults. */
    renderer->mp_font = DEFAULT_MP_FONT;
    renderer->mp_weight = DEFAULT_MP_WEIGHT;
    renderer->mp_slant = DEFAULT_MP_SLANT;
    renderer->mp_font_height = DEFAULT_SIZE_MULT * dia_font_height;

    /* Try to find a better match for the Dia font by checking our lookup
     * table. */
    for (i = 0; FONT_LOOKUP_TABLE[i].dia_name != NULL; i++) {
        if (0 == strncmp(FONT_LOOKUP_TABLE[i].dia_name, dia_font_name,
                            MAX_FONT_NAME_LEN))
        {
            /* Found a match. */
            renderer->mp_font = FONT_LOOKUP_TABLE[i].mp_name;
            renderer->mp_font_height = FONT_LOOKUP_TABLE[i].size_mult
                                        * dia_font_height;
            break;
        }
    }

    /* Do the same for the weight and size. */
    for (i = 0; WEIGHT_LOOKUP_TABLE[i].weight != STYLE_TERMINATOR; i++) {
        if (DIA_FONT_STYLE_GET_WEIGHT(dia_font_style)
                == WEIGHT_LOOKUP_TABLE[i].weight) {
            renderer->mp_weight = WEIGHT_LOOKUP_TABLE[i].mp_weight;
        }
    }
    for (i = 0; SLANT_LOOKUP_TABLE[i].slant != STYLE_TERMINATOR; i++) {
        if (DIA_FONT_STYLE_GET_SLANT(dia_font_style)
                == SLANT_LOOKUP_TABLE[i].slant) {
            renderer->mp_slant = SLANT_LOOKUP_TABLE[i].mp_slant;
        }
    }

#if 0
    fprintf(renderer->file, "defaultfont:=\"%s\";\n", font_get_psfontname(font));
#endif
}

static void
draw_line(DiaRenderer *self,
	  Point *start, Point *end,
	  Color *line_color)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);
    gchar sx_buf[DTOSTR_BUF_SIZE];
    gchar sy_buf[DTOSTR_BUF_SIZE];
    gchar ex_buf[DTOSTR_BUF_SIZE];
    gchar ey_buf[DTOSTR_BUF_SIZE];

    set_line_color(renderer,line_color);

    fprintf(renderer->file, "  draw (%sx,%sy)--(%sx,%sy)",
	    mp_dtostr(sx_buf, start->x), mp_dtostr(sy_buf, start->y),
	    mp_dtostr(ex_buf, end->x), mp_dtostr(ey_buf, end->y) );
    end_draw_op(renderer);
}

static void
draw_polyline(DiaRenderer *self,
	      Point *points, int num_points,
	      Color *line_color)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);
    int i;
    gchar px_buf[DTOSTR_BUF_SIZE];
    gchar py_buf[DTOSTR_BUF_SIZE];

    set_line_color(renderer,line_color);

    fprintf(renderer->file,
	    "  draw (%sx,%sy)",
	    mp_dtostr(px_buf, points[0].x),
	    mp_dtostr(py_buf, points[0].y) );

    for (i=1;i<num_points;i++) {
	fprintf(renderer->file, "--(%sx,%sy)",
		mp_dtostr(px_buf, points[i].x),
		mp_dtostr(py_buf, points[i].y) );
    }
    end_draw_op(renderer);
}

static void
draw_polygon(DiaRenderer *self,
	     Point *points, int num_points,
	     Color *fill, Color *stroke)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);
    int i;
    gchar px_buf[DTOSTR_BUF_SIZE];
    gchar py_buf[DTOSTR_BUF_SIZE];
    gchar red_buf[DTOSTR_BUF_SIZE];
    gchar green_buf[DTOSTR_BUF_SIZE];
    gchar blue_buf[DTOSTR_BUF_SIZE];

    fprintf(renderer->file, "%% draw_polygon\n");
    if (stroke)
	set_line_color(renderer,stroke);

    fprintf(renderer->file,
	    "  path p;\n"
	    "  p = (%sx,%sy)",
	    mp_dtostr(px_buf, points[0].x),
	    mp_dtostr(py_buf, points[0].y) );

    for (i=1;i<num_points;i++) {
	fprintf(renderer->file, "--(%sx,%sy)",
		mp_dtostr(px_buf, points[i].x),
		mp_dtostr(py_buf, points[i].y) );
    }
    fprintf(renderer->file,"--cycle;\n");

    if (fill)
	fprintf(renderer->file,
		"  fill p withcolor (%s,%s,%s);\n",
		mp_dtostr(red_buf, (gdouble) fill->red),
		mp_dtostr(green_buf, (gdouble) fill->green),
		mp_dtostr(blue_buf, (gdouble) fill->blue) );

    if (stroke) {
	fprintf(renderer->file, "  draw p");
	end_draw_op(renderer);
    }
}

static void
metapost_arc(MetapostRenderer *renderer,
	     Point *center,
	     real width, real height,
	     real angle1, real angle2,
	     Color *color, int filled)
{
    double radius1,radius2;
    double angle3;
    double cx = (double) center->x;
    double cy = (double) center->y;
    gchar d1_buf[DTOSTR_BUF_SIZE];
    gchar d2_buf[DTOSTR_BUF_SIZE];

    radius1=(double) width/2.0;
    radius2=(double) height/2.0;

    fprintf(renderer->file,"%%metapost_arc\n");
    fprintf(renderer->file, "%% %s = %s", "center->x", mp_dtostr(d1_buf, center->x));
    fprintf(renderer->file, "%% %s = %s", "center->y", mp_dtostr(d1_buf, center->y));
    fprintf(renderer->file, "%% %s = %s", "width", mp_dtostr(d1_buf, width));
    fprintf(renderer->file, "%% %s = %s", "height", mp_dtostr(d1_buf, height));
    fprintf(renderer->file, "%% %s = %s", "angle1", mp_dtostr(d1_buf, angle1));
    fprintf(renderer->file, "%% %s = %s", "angle2", mp_dtostr(d1_buf, angle2));

    angle1 = angle1*M_PI/180;
    angle2 = angle2*M_PI/180;
    angle3 = (double) (angle1+angle2)/2;
    if (angle1 > angle2)
        angle3 += M_PI;

    set_line_color(renderer,color);

    fprintf(renderer->file, "  draw (%sx,%sy)..",
	    mp_dtostr(d1_buf, cx + radius1*cos(angle1)),
	    mp_dtostr(d2_buf, cy - radius2*sin(angle1)) );
    fprintf(renderer->file, "(%sx,%sy)..",
	    mp_dtostr(d1_buf, cx + radius1*cos(angle3)),
	    mp_dtostr(d2_buf, cy - radius2*sin(angle3)) );
    fprintf(renderer->file, "(%sx,%sy)",
	    mp_dtostr(d1_buf, cx + radius1*cos(angle2)),
	    mp_dtostr(d2_buf, cy - radius2*sin(angle2)) );
    end_draw_op(renderer);
}

static void
draw_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);

    metapost_arc(renderer,center,width,height,angle1,angle2,color,0);
}

static void
fill_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);

    metapost_arc(renderer,center,width,height,angle1,angle2,color,1);
}

static void
draw_ellipse(DiaRenderer *self,
	     Point *center,
	     real width, real height,
	     Color *fill, Color *stroke)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);
    gchar d1_buf[DTOSTR_BUF_SIZE];
    gchar d2_buf[DTOSTR_BUF_SIZE];
    gchar red_buf[DTOSTR_BUF_SIZE];
    gchar green_buf[DTOSTR_BUF_SIZE];
    gchar blue_buf[DTOSTR_BUF_SIZE];

    if (stroke)
	set_line_color(renderer,stroke);

    fprintf(renderer->file,
	    "  path p;\n"
	    "  p = (%sx,%sy)..",
	    mp_dtostr(d1_buf, (gdouble) center->x+width/2.0),
	    mp_dtostr(d2_buf, (gdouble) center->y) );
    fprintf(renderer->file, "(%sx,%sy)..",
	    mp_dtostr(d1_buf, (gdouble) center->x),
	    mp_dtostr(d2_buf, (gdouble) center->y+height/2.0) );
    fprintf(renderer->file, "(%sx,%sy)..",
	    mp_dtostr(d1_buf, (gdouble) center->x-width/2.0),
	    mp_dtostr(d2_buf, (gdouble) center->y) );
    fprintf(renderer->file, "(%sx,%sy)..cycle;\n",
	    mp_dtostr(d1_buf, (gdouble) center->x),
	    mp_dtostr(d2_buf, (gdouble) center->y-height/2.0) );

    if (fill)
	fprintf(renderer->file,
		"  fill p withcolor (%s,%s,%s);\n",
		mp_dtostr(red_buf, (gdouble) fill->red),
		mp_dtostr(green_buf, (gdouble) fill->green),
		mp_dtostr(blue_buf, (gdouble) fill->blue) );

    if (stroke) {
	fprintf(renderer->file, "  draw p");
	end_draw_op(renderer);
    }
}


static void
draw_bezier (DiaRenderer *self,
             BezPoint    *points,
             int          numpoints, /* numpoints = 4+3*n, n=>0 */
             Color       *color)
{
  MetapostRenderer *renderer = METAPOST_RENDERER (self);
  gint i;
  gchar p1x_buf[DTOSTR_BUF_SIZE];
  gchar p1y_buf[DTOSTR_BUF_SIZE];
  gchar p2x_buf[DTOSTR_BUF_SIZE];
  gchar p2y_buf[DTOSTR_BUF_SIZE];
  gchar p3x_buf[DTOSTR_BUF_SIZE];
  gchar p3y_buf[DTOSTR_BUF_SIZE];

  set_line_color (renderer, color);

  if (points[0].type != BEZ_MOVE_TO) {
    g_warning ("first BezPoint must be a BEZ_MOVE_TO");
  }

  fprintf (renderer->file, "  draw (%sx,%sy)",
           mp_dtostr (p1x_buf, (gdouble) points[0].p1.x),
           mp_dtostr (p1y_buf, (gdouble) points[0].p1.y) );

  for (i = 1; i < numpoints; i++) {
    switch (points[i].type) {
      case BEZ_MOVE_TO:
        g_warning ("only first BezPoint can be a BEZ_MOVE_TO");
        break;
      case BEZ_LINE_TO:
        fprintf (renderer->file, "--(%sx,%sy)",
                 mp_dtostr (p1x_buf, (gdouble) points[i].p1.x),
                 mp_dtostr (p1y_buf, (gdouble) points[i].p1.y) );
        break;
      case BEZ_CURVE_TO:
        fprintf (renderer->file, "..controls (%sx,%sy) and (%sx,%sy)\n    ..(%sx,%sy)",
                 mp_dtostr (p1x_buf, (gdouble) points[i].p1.x),
                 mp_dtostr (p1y_buf, (gdouble) points[i].p1.y),
                 mp_dtostr (p2x_buf, (gdouble) points[i].p2.x),
                 mp_dtostr (p2y_buf, (gdouble) points[i].p2.y),
                 mp_dtostr (p3x_buf, (gdouble) points[i].p3.x),
                 mp_dtostr (p3y_buf, (gdouble) points[i].p3.y) );
        break;
      default:
        g_return_if_reached ();
    }
  }

  end_draw_op (renderer);
}


static void
draw_beziergon (DiaRenderer *self,
                BezPoint    *points,
                int          numpoints,
                Color       *fill,
                Color       *stroke)
{
  MetapostRenderer *renderer = METAPOST_RENDERER (self);
  gint i;
  gchar p1x_buf[DTOSTR_BUF_SIZE];
  gchar p1y_buf[DTOSTR_BUF_SIZE];
  gchar p2x_buf[DTOSTR_BUF_SIZE];
  gchar p2y_buf[DTOSTR_BUF_SIZE];
  gchar p3x_buf[DTOSTR_BUF_SIZE];
  gchar p3y_buf[DTOSTR_BUF_SIZE];
  gchar red_buf[DTOSTR_BUF_SIZE];
  gchar green_buf[DTOSTR_BUF_SIZE];
  gchar blue_buf[DTOSTR_BUF_SIZE];

  if (points[0].type != BEZ_MOVE_TO) {
    g_warning ("first BezPoint must be a BEZ_MOVE_TO");
  }

  if (stroke) {
    set_line_color (renderer,stroke);
  }

  fprintf (renderer->file, "  path p;\n");
  fprintf (renderer->file, "  p = (%sx,%sy)",
           mp_dtostr (p1x_buf, (gdouble) points[0].p1.x),
           mp_dtostr (p1y_buf, (gdouble) points[0].p1.y) );

  for (i = 1; i < numpoints; i++) {
    switch (points[i].type) {
      case BEZ_MOVE_TO:
        /* close previous, new move-to - XXX: not working, not used, gives:
        *   Paths don't touch; `&' will be changed to `..'.
        */
        fprintf (renderer->file, "  ..cycle\n  & (%sx,%sy)",
                 mp_dtostr (p1x_buf, (gdouble) points[i].p1.x),
                 mp_dtostr (p1y_buf, (gdouble) points[i].p1.y) );
        break;
      case BEZ_LINE_TO:
        fprintf (renderer->file, "--(%sx,%sy)",
                 mp_dtostr (p1x_buf, (gdouble) points[i].p1.x),
                 mp_dtostr (p1y_buf, (gdouble) points[i].p1.y) );
        break;
      case BEZ_CURVE_TO:
        fprintf (renderer->file, "..controls (%sx,%sy) and (%sx,%sy)\n    ..(%sx,%sy)",
                 mp_dtostr(p1x_buf, (gdouble) points[i].p1.x),
                 mp_dtostr(p1y_buf, (gdouble) points[i].p1.y),
                 mp_dtostr(p2x_buf, (gdouble) points[i].p2.x),
                 mp_dtostr(p2y_buf, (gdouble) points[i].p2.y),
                 mp_dtostr(p3x_buf, (gdouble) points[i].p3.x),
                 mp_dtostr(p3y_buf, (gdouble) points[i].p3.y) );
        break;
      default:
        g_return_if_reached ();
    }
  }
  fprintf (renderer->file, "\n    ..cycle;\n");

  if (fill) {
    fprintf (renderer->file,
             "  fill p withcolor (%s,%s,%s);\n",
             mp_dtostr (red_buf, (gdouble) fill->red),
             mp_dtostr (green_buf, (gdouble) fill->green),
             mp_dtostr (blue_buf, (gdouble) fill->blue) );
  }

  if (stroke) {
    fprintf (renderer->file, "  draw p");
    end_draw_op (renderer);
  }
}


static void
draw_string (DiaRenderer  *self,
             const char   *text,
             Point        *pos,
             DiaAlignment  alignment,
             Color        *color)
{
  MetapostRenderer *renderer = METAPOST_RENDERER (self);
  char height_buf[DTOSTR_BUF_SIZE];
  char px_buf[DTOSTR_BUF_SIZE];
  char py_buf[DTOSTR_BUF_SIZE];
  char red_buf[DTOSTR_BUF_SIZE];
  char green_buf[DTOSTR_BUF_SIZE];
  char blue_buf[DTOSTR_BUF_SIZE];

  set_line_color (renderer, color);

  /* text position is correct for baseline. Uses macros defined
    * at top of MetaPost file (see export_metapost) to correctly
    * align text. See bug # 332554 */
  switch (alignment) {
    case DIA_ALIGN_LEFT:
      fprintf (renderer->file,"  draw");
      break;
    case DIA_ALIGN_CENTRE:
      fprintf (renderer->file,"  draw hcentered");
      break;
    case DIA_ALIGN_RIGHT:
      fprintf (renderer->file,"  draw rjust");
      break;
    default:
      g_return_if_reached ();
  }

  /* Ideally, we would be able to use the "infont" macro to print this label
    * in the proper font.  Unfortunately, though, metapost is in the habit of
    * converting spaces into visible spaces, which looks rather yucky.  So we
    * embed some TeX with \usefont commands instead. */
  /* Scale text by multiplying text by variable t in metapost */
  fprintf (renderer->file,
           " btex {\\usefont{OT1}{%s}{%s}{%s} %s} etex scaled %st shifted (%sx,%sy)",
           renderer->mp_font, renderer->mp_weight, renderer->mp_slant,
           text,
           g_ascii_formatd (height_buf, sizeof (height_buf), "%g", renderer->mp_font_height),
           mp_dtostr (px_buf, pos->x),
           mp_dtostr (py_buf, pos->y) );

  if (!color_equals(&renderer->color, &color_black)) {
      fprintf (renderer->file, "\n    withcolor (%s, %s, %s)",
               g_ascii_formatd (red_buf, sizeof (red_buf), "%5.4f", (gdouble) renderer->color.red),
               g_ascii_formatd (green_buf, sizeof (green_buf), "%5.4f", (gdouble) renderer->color.green),
               g_ascii_formatd (blue_buf, sizeof (blue_buf), "%5.4f", (gdouble) renderer->color.blue) );
  }

  fprintf (renderer->file,";\n");
}


static void
draw_text (DiaRenderer *self,
           Text        *text)
{
  Point pos;
  int i;
  pos = text->position;

  set_font (self, text->font, text->height);

  for (i = 0; i < text->numlines; i++) {
    TextLine *text_line = text->lines[i];

    draw_string (self,
                 text_line_get_string (text_line),
                 &pos,
                 text->alignment,
                 &text->color);
    pos.y += text->height;
  }
}


static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage *image)
{
    /* images have a banding problem */
    int img_width, img_height, img_rowstride;
    int x, y;
    real xstep, ystep;
    guint8 *rgb_data;
    guint8 *mask_data;
    double ix, iy;
    gchar d1_buf[DTOSTR_BUF_SIZE];
    gchar d2_buf[DTOSTR_BUF_SIZE];
    gchar d3_buf[DTOSTR_BUF_SIZE];
    const char *filename = dia_image_filename(image);
    MetapostRenderer *renderer = METAPOST_RENDERER (self);

    fprintf(renderer->file, "  %% draw_image: %s\n", filename);
#if 0 /* Only working with ConTeXt? */
    if (g_file_test (filename,  G_FILE_TEST_IS_REGULAR)) {
	/* externalfigure "hacker.png" scaled 5cm shifted (-6cm,0); */
	fprintf(renderer->file,
		"  externalfigure \"%s\" scaled %scm shifted (%scm,%scm);\n",
		filename, mp_dtostr(d1_buf, (gdouble) width),
		mp_dtostr(d2_buf, (gdouble) point->x),
		mp_dtostr(d3_buf, (gdouble) point->y));
	return;
    }
#endif
    img_width = dia_image_width(image);
    img_rowstride = dia_image_rowstride(image);
    img_height = dia_image_height(image);

    xstep = width/img_width;
    ystep = height/img_height;

    rgb_data = dia_image_rgb_data(image);
    if (!rgb_data) {
        dia_context_add_message(renderer->ctx, _("Not enough memory for image drawing."));
        return;
    }
    mask_data = dia_image_mask_data(image);

    fprintf(renderer->file, "  pickup pensquare scaled %sx scaled %s;\n",
            mp_dtostr(d1_buf, (gdouble) xstep),
            mp_dtostr(d2_buf, (gdouble) (ystep/xstep)) );


    if (mask_data) {
        fprintf(renderer->file, "  %% have mask\n");
        for (y = 0, iy = point->y; y < img_height; y++, iy += ystep) {
            for (x = 0, ix = point->x; x < img_width; x++, ix += xstep) {
                int i = y*img_rowstride+x*3;
                int m = y*img_width+x;
		fprintf(renderer->file, "  draw (%sx, %sy) ",
			mp_dtostr(d1_buf, ix), mp_dtostr(d2_buf, iy) );
                fprintf(renderer->file, "withcolor (%s, %s, %s);\n",
                        g_ascii_formatd(d1_buf, sizeof(d1_buf), "%5.4f",
					(255-(mask_data[m]*(255-rgb_data[i])/255)/255.0)),
                        g_ascii_formatd(d2_buf, sizeof(d2_buf), "%5.4f",
					(255-(mask_data[m]*(255-rgb_data[i+1])/255))/255.0),
                        g_ascii_formatd(d3_buf, sizeof(d3_buf), "%5.4f",
					(255-(mask_data[m]*(255-rgb_data[i+2])/255))/255.0) );
            }
            fprintf(renderer->file, "\n");
        }
    } else {
        for (y = 0, iy = point->y; y < img_height; y++, iy += ystep) {
            for (x = 0, ix = point->x; x < img_width; x++, ix += xstep) {
                int i = y*img_rowstride+x*3;
		fprintf(renderer->file, "  draw (%sx, %sy) ",
			mp_dtostr(d1_buf, ix), mp_dtostr(d2_buf, iy) );
                fprintf(renderer->file, "withcolor (%s, %s, %s);\n",
                        g_ascii_formatd(d1_buf, sizeof(d1_buf), "%5.4f",
					(gdouble) (rgb_data[i])/255.0),
                        g_ascii_formatd(d2_buf, sizeof(d2_buf), "%5.4f",
					(gdouble) (rgb_data[i+1])/255.0),
                        g_ascii_formatd(d3_buf, sizeof(d3_buf), "%5.4f",
					(gdouble) (rgb_data[i+2])/255.0) );
            }
            fprintf(renderer->file, "\n");
        }
    }

    g_clear_pointer (&mask_data, g_free);
    g_clear_pointer (&rgb_data, g_free);
}

/* GObject stuff */
static void metapost_renderer_class_init (MetapostRendererClass *klass);


static gpointer parent_class = NULL;

GType
metapost_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (MetapostRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) metapost_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (MetapostRenderer),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "MetapostRenderer",
                                            &object_info, 0);
    }

  return object_type;
}

static void
metapost_renderer_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MetapostRenderer *self = METAPOST_RENDERER (object);

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
metapost_renderer_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MetapostRenderer *self = METAPOST_RENDERER (object);

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
metapost_renderer_finalize (GObject *object)
{
  MetapostRenderer *metapost_renderer = METAPOST_RENDERER (object);

  g_clear_object (&metapost_renderer->font);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
metapost_renderer_class_init (MetapostRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->get_property = metapost_renderer_get_property;
  object_class->set_property = metapost_renderer_set_property;
  object_class->finalize = metapost_renderer_finalize;

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
  renderer_class->draw_text = draw_text;

  renderer_class->draw_image = draw_image;

  g_object_class_override_property (object_class, PROP_FONT, "font");
  g_object_class_override_property (object_class, PROP_FONT_HEIGHT, "font-height");
}

/* --- export filter interface --- */
static gboolean
export_metapost(DiagramData *data, DiaContext *ctx,
		const gchar *filename, const gchar *diafilename,
		void* user_data)
{
    MetapostRenderer *renderer;
    FILE *file;
    time_t time_now;
    DiaRectangle *extent;
    const char *name;
    gchar d1_buf[DTOSTR_BUF_SIZE];
    gchar d2_buf[DTOSTR_BUF_SIZE];
    gchar d3_buf[DTOSTR_BUF_SIZE];
    gchar d4_buf[DTOSTR_BUF_SIZE];

    Color initial_color;

    file = g_fopen(filename, "wb");

    if (file == NULL) {
	dia_context_add_message_with_errno (ctx, errno, _("Can't open output file %s"),
					    dia_context_get_filename(ctx));
	return FALSE;
    }

    renderer = g_object_new(METAPOST_TYPE_RENDERER, NULL);

    renderer->file = file;
    renderer->ctx = ctx;

    renderer->dash_length = 1.0;
    renderer->dot_length = 0.2;
    renderer->saved_line_style = DIA_LINE_STYLE_SOLID;

    time_now  = time(NULL);
    extent = &data->extents;

    name = g_get_user_name();

    fprintf(file,
	    "%% Metapost TeX macro\n"
	    "%% Title: %s\n"
	    "%% Creator: Dia v%s\n"
	    "%% CreationDate: %s"
	    "%% For: %s\n"
	    "\n\n"
	    "beginfig(1);\n",
	    diafilename,
	    dia_version_string(),
	    ctime(&time_now),
	    name);

    /* LaTeX header so that our font stuff works properly. */
    fprintf(renderer->file,
             "verbatimtex\n"
             "%%&latex\n"
             "\\documentclass{minimal}\n"
             "\\begin{document}\n"
             "etex\n");

	/* Define Macros for Text Positioning */
    fprintf(renderer->file,
			"%% Define macro for horizontal centering.\n"
			"vardef hcentered primary P =\n"
			"  P shifted -(xpart center P, 0)\n"
			"enddef;\n");

    fprintf(renderer->file,
			"%% Define macro for right justification.\n"
			"vardef rjust primary P =\n"
			"  P shifted -(xpart (lrcorner P - llcorner P), 0)\n"
			"enddef;\n");


    fprintf(renderer->file,"  %% picture(%s,%s)(%s,%s)\n",
	    mp_dtostr(d1_buf, extent->left * data->paper.scaling),
	    mp_dtostr(d2_buf, -extent->bottom * data->paper.scaling),
	    mp_dtostr(d3_buf, extent->right * data->paper.scaling),
	    mp_dtostr(d4_buf, -extent->top * data->paper.scaling) );
    fprintf(renderer->file,"  x = %scm; y = %scm;\n\n",
	    mp_dtostr(d1_buf, data->paper.scaling),
	    mp_dtostr(d2_buf, -data->paper.scaling) );

	/* Create a variable for Text Scaling  't' */
    fprintf(renderer->file,"  t = %s;\n\n",
	    mp_dtostr(d1_buf, data->paper.scaling));

  initial_color.red= 0.0;
  initial_color.green= 0.0;
  initial_color.blue= 0.0;
  initial_color.alpha= 1.0;
  set_line_color (renderer,&initial_color);

  data_render (data, DIA_RENDERER (renderer), NULL, NULL, NULL);

  g_clear_object (&renderer);

  return TRUE;
}


static const gchar *extensions[] = { "mp", NULL };
DiaExportFilter metapost_export_filter = {
  N_("TeX Metapost macros"),
  extensions,
  export_metapost
};
