/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * cgm.c -- CGM plugin for dia
 * Copyright (C) 1999-2000 James Henstridge.
 * Copyright (C) 2000 Henk Jan Priester.
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

#include "cgm.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <errno.h>

#include <glib/gstdio.h>

#include "geometry.h"
#include "filter.h"
#include "plug-ins.h"
#include "diarenderer.h"
#include "dia_image.h"
#include "font.h"
#include "dia-version-info.h"

#include <gdk/gdk.h>

#define CGM_TYPE_RENDERER           (cgm_renderer_get_type ())
#define CGM_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), CGM_TYPE_RENDERER, CgmRenderer))
#define CGM_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), CGM_TYPE_RENDERER, CgmRendererClass))
#define CGM_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CGM_TYPE_RENDERER))
#define CGM_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), CGM_TYPE_RENDERER, CgmRendererClass))

enum {
  PROP_0,
  PROP_FONT,
  PROP_FONT_HEIGHT,
  LAST_PROP
};


/* Noise reduction for
 * return value of 'fwrite', declared with attribute warn_unused_result
 * discussion: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=25509
 * root cause: http://sourceware.org/bugzilla/show_bug.cgi?id=11959
 */
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-result"
#endif

GType cgm_renderer_get_type (void) G_GNUC_CONST;

typedef struct _CgmRenderer CgmRenderer;
typedef struct _CgmRendererClass CgmRendererClass;

struct _CgmRendererClass
{
  DiaRendererClass parent_class;
};

#define IS_ODD(n) (n & 0x01)

/* --- routines to write various quantities to the CGM stream. --- */
/* signed integers are stored in two complement this is common on Unix */
static void
write_uint16(FILE *fp, guint16 n)
{
    putc( (n & 0xff00) >> 8, fp);
    putc( n & 0xff, fp);
}

static void
write_int16(FILE *fp, gint16 n)
{
    write_uint16(fp, (guint16)n);
}

static void
write_uint32(FILE *fp, guint32 n)
{
    putc((n >> 24) & 0xff, fp);
    putc((n >> 16) & 0xff, fp);
    putc((n >> 8) & 0xff, fp);
    putc(n & 0xff, fp);
}

static void
write_colour(FILE *fp, Color *c)
{
    putc((int)(c->red   * 255), fp);
    putc((int)(c->green * 255), fp);
    putc((int)(c->blue  * 255), fp);
}

#define REALSIZE 4
/* 32 bit fixed point real number */
/* stored as 16 bit signed (SI) number and a 16 bit unsigned (UI) for */
/* the fraction */
/* value is SI + UI / 2^16 */
static void
write_real(FILE *fp, double x)
{
    guint32  n;

    if ( x < 0 )
    {
        gint32   wholepart;
        guint16  fraction;

        wholepart = (gint32)x;

        fraction = (guint16)((x - wholepart) * -65536);
        if ( fraction > 0 )
        {
            wholepart--;
            fraction = (guint16)(65536 - fraction);
        }
        n = (guint32)(wholepart << 16) | fraction;
    }
    else
        n = (guint32) (x * (1 << 16));
    write_uint32(fp, n);
}

static void
write_elhead(FILE *fp, CgmElementClass el_class, int el_id, int nparams)
{
    guint16 head;

    head = (el_class & 0x0f) << 12;
    head += (el_id & 0x7f) << 5;

    if (nparams >= 31) {
	/* use long header format */
	head += 31;
	write_uint16(fp, head);
	write_int16(fp, (gint16)nparams);
    } else {
	/* use short header format */
	head += nparams & 0x1f;
	write_uint16(fp, head);
    }
}


/* --- font stuff --- */

static gchar *fontlist;
static gint fontlistlen;
static GHashTable *fonthash;
#define FONT_NUM(font) GPOINTER_TO_INT(g_hash_table_lookup(fonthash, \
     dia_font_get_family(font)))

static void
init_fonts(void)
{
        /* PANGO FIXME: this is probably broken to some extent now */
    static gboolean alreadyrun = FALSE;
    GString *str;
    gint i;
    PangoContext *context;
    PangoFontFamily **families;
    int n_families;
    const char *familyname;

    if (alreadyrun) return;
    alreadyrun = TRUE;

    context = gdk_pango_context_get();
    pango_context_list_families(context,&families,&n_families);

    fonthash = g_hash_table_new(g_str_hash, g_str_equal);
    str = g_string_new(NULL);
    for (i = 0; i < n_families; ++i) {
        familyname = pango_font_family_get_name(families[i]);

        g_string_append_c(str, strlen(familyname));
        g_string_append(str, familyname);
        g_hash_table_insert(fonthash, (gpointer)familyname,
                            GINT_TO_POINTER(i+1));
    }
    fontlist = str->str;
    fontlistlen = str->len;
    g_string_free(str, FALSE);
}

/* --- CGM line attributes --- */
typedef struct _LineAttrCGM
{
    int         cap;
    int         join;
    int         style;
    real        width;
    Color       color;

} LineAttrCGM;

/* --- CGM File/Edge attributes --- */
typedef struct _FillEdgeAttrCGM
{

   int          fill_style;          /* Fill style */
   Color        fill_color;          /* Fill color */

   int          edgevis;             /* Edge visibility */
   int          cap;                 /* Edge cap */
   int          join;                /* Edge join */
   int          style;               /* Edge style */
   real         width;               /* Edge width */
   Color        color;               /* Edge color */

} FillEdgeAttrCGM;


/* --- CGM Text attributes --- */
typedef struct _TextAttrCGM
{
   int          font_num;
   real         font_height;
   Color        color;

} TextAttrCGM;


/* --- the renderer --- */

struct _CgmRenderer
{
    DiaRenderer parent_instance;

    FILE *file;

    DiaFont *font;

    real y0, y1;

    LineAttrCGM  lcurrent, linfile;

    FillEdgeAttrCGM fcurrent, finfile;

    TextAttrCGM    tcurrent, tinfile;

    DiaContext *ctx;
};

static void
init_attributes( CgmRenderer *renderer )
{
    /* current values, (defaults) */
    renderer->lcurrent.cap   = 3;            /* round */
    renderer->lcurrent.join  = 2;            /* mitre */
    renderer->lcurrent.style = 1;
    renderer->lcurrent.width = 0.1;
    renderer->lcurrent.color.red   = 0;
    renderer->lcurrent.color.green = 0;
    renderer->lcurrent.color.blue  = 0;
    renderer->lcurrent.color.alpha = 1.0;

    renderer->linfile.cap    = -1;
    renderer->linfile.join   = -1;
    renderer->linfile.style  = -1;
    renderer->linfile.width  = -1.0;
    renderer->linfile.color.red   = -1;
    renderer->linfile.color.green = -1;
    renderer->linfile.color.blue  = -1;
    renderer->linfile.color.alpha = 1.0;

    /* fill/edge defaults */
    renderer->fcurrent.fill_style = 1;          /* solid */
    renderer->fcurrent.fill_color.red = 0;
    renderer->fcurrent.fill_color.green = 0;
    renderer->fcurrent.fill_color.blue = 0;
    renderer->fcurrent.fill_color.alpha = 1.0;

    renderer->fcurrent.edgevis = 0;
    renderer->fcurrent.cap   = 3;            /* round */
    renderer->fcurrent.join  = 2;            /* mitre */
    renderer->fcurrent.style = 1;
    renderer->fcurrent.width = 0.1;
    renderer->fcurrent.color.red   = 0;
    renderer->fcurrent.color.green = 0;
    renderer->fcurrent.color.blue  = 0;
    renderer->fcurrent.color.alpha = 1.0;

    renderer->finfile.fill_style = -1;
    renderer->finfile.fill_color.red = -1;
    renderer->finfile.fill_color.green = -1;
    renderer->finfile.fill_color.blue = -1;
    renderer->finfile.fill_color.alpha = 1.0;

    renderer->finfile.edgevis = -1;
    renderer->finfile.cap   = -1;
    renderer->finfile.join  = -1;
    renderer->finfile.style = -1;
    renderer->finfile.width = -1.;
    renderer->finfile.color.red   = -1.0;
    renderer->finfile.color.green = -1.0;
    renderer->finfile.color.blue  = -1.0;
    renderer->finfile.color.alpha = 1.0;

    renderer->tcurrent.font_num    = 1;
    renderer->tcurrent.font_height = 0.1;
    renderer->tcurrent.color.red   = 0.0;
    renderer->tcurrent.color.green = 0.0;
    renderer->tcurrent.color.blue  = 0.0;
    renderer->tcurrent.color.alpha = 1.0;

    renderer->tinfile.font_num    = -1;
    renderer->tinfile.font_height = -1.0;
    renderer->tinfile.color.red   = -1.0;
    renderer->tinfile.color.green = -1.0;
    renderer->tinfile.color.blue  = -1.0;
    renderer->tinfile.color.alpha = 1.0;

}



static real
swap_y( CgmRenderer *renderer, real y)
{
   return  (renderer->y0 + renderer->y1 - y);
}


static void
write_line_attributes( CgmRenderer *renderer, Color *color )
{
LineAttrCGM    *lnew, *lold;

    lnew = &renderer->lcurrent;
    lold = &renderer->linfile;

    if ( lnew->cap != lold->cap )
    {
        write_elhead(renderer->file, CGM_ATTRIB, CGM_LINE_CAP, 4);
        write_int16(renderer->file, lnew->cap);
        write_int16(renderer->file, 3);      /* cap of dashlines match */
                                             /* normal cap */
        lold->cap = lnew->cap;
    }
    if ( lnew->join != lold->join )
    {
        write_elhead(renderer->file, CGM_ATTRIB, CGM_LINE_JOIN, 2);
        write_int16(renderer->file, lnew->join);
        lold->join = lnew->join;
    }
    if ( lnew->style != lold->style )
    {
        write_elhead(renderer->file, CGM_ATTRIB, CGM_LINE_TYPE, 2);
        write_int16(renderer->file, lnew->style);
        lold->style = lnew->style;
    }
    if ( lnew->width != lold->width )
    {
        write_elhead(renderer->file, CGM_ATTRIB, CGM_LINE_WIDTH, REALSIZE);
        write_real(renderer->file, lnew->width);
        lold->width = lnew->width;
    }
    lnew->color = *color;
    if ( lnew->color.red != lold->color.red ||
         lnew->color.green != lold->color.green ||
         lnew->color.blue != lold->color.blue ||
         lnew->color.alpha != lold->color.alpha)
    {
        write_elhead(renderer->file, CGM_ATTRIB, CGM_LINE_COLOR, 3); /* line color */
        write_colour(renderer->file, &lnew->color);
        putc(0, renderer->file);
        lold->color = lnew->color;
    }
}


/*
** Update the fill/edge attributes.
**
** fill_color		!= NULL, style solid, interrior-color is fill_color
**                      == NULL, style empty
**
** edge_color           != NULL, edge on with the color and the other
**                               attributes
**                      == NULL, edge off
*/

static void
write_filledge_attributes( CgmRenderer *renderer, Color *fill_color,
                           Color *edge_color )
{
FillEdgeAttrCGM    *fnew, *fold;

    fnew = &renderer->fcurrent;
    fold = &renderer->finfile;
    /*
    ** Set the edge attributes
    */

    if ( edge_color == NULL )
    {
        fnew->edgevis = 0;   /* edge off */
        if ( fnew->edgevis != fold->edgevis )
        {
            write_elhead(renderer->file, CGM_ATTRIB, CGM_EDGE_VISIBILITY, 2);
            write_int16(renderer->file, fnew->edgevis);
            fold->edgevis = fnew->edgevis;
        }
    }
    else
    {
        fnew->edgevis = 1;   /* edge on */
        if ( fnew->edgevis != fold->edgevis )
        {
            write_elhead(renderer->file, CGM_ATTRIB, CGM_EDGE_VISIBILITY, 2);
            write_int16(renderer->file, fnew->edgevis);
            fold->edgevis = fnew->edgevis;
        }
        if ( fnew->cap != fold->cap )
        {
            write_elhead(renderer->file, CGM_ATTRIB, CGM_EDGE_CAP, 4);
            write_int16(renderer->file, fnew->cap);
            write_int16(renderer->file, 3);  /* cap of dashlines match */
                                             /* normal cap */
            fold->cap = fnew->cap;
        }
        if ( fnew->join != fold->join )
        {
            write_elhead(renderer->file, CGM_ATTRIB, CGM_EDGE_JOIN, 2);
            write_int16(renderer->file, fnew->join);
            fold->join = fnew->join;
        }
        if ( fnew->style != fold->style )
        {
            write_elhead(renderer->file, CGM_ATTRIB, CGM_EDGE_TYPE, 2);
            write_int16(renderer->file, fnew->style);
            fold->style = fnew->style;
        }
        if ( fnew->width != fold->width )
        {
            write_elhead(renderer->file, CGM_ATTRIB, CGM_EDGE_WIDTH, REALSIZE);
            write_real(renderer->file, fnew->width);
            fold->width = fnew->width;
        }
        fnew->color = *edge_color;
        if ( fnew->color.red != fold->color.red ||
             fnew->color.green != fold->color.green ||
             fnew->color.blue != fold->color.blue ||
             fnew->color.alpha != fold->color.alpha)
        {
            write_elhead(renderer->file, CGM_ATTRIB, CGM_EDGE_COLOR, 3); /* line color */
            write_colour(renderer->file, &fnew->color);
            putc(0, renderer->file);
            fold->color = fnew->color;
        }
    }

    if ( fill_color == NULL )
    {
        fnew->fill_style = 4;       /* empty */
        if ( fnew->fill_style != fold->fill_style )
        {
            write_elhead(renderer->file, CGM_ATTRIB, CGM_INTERIOR_STYLE, 2);
            write_int16(renderer->file, fnew->fill_style);
            fold->fill_style = fnew->fill_style;
        }
    }
    else
    {
        fnew->fill_style = 1;       /* solid fill */
        if ( fnew->fill_style != fold->fill_style )
        {
            write_elhead(renderer->file, CGM_ATTRIB, CGM_INTERIOR_STYLE, 2);
            write_int16(renderer->file, fnew->fill_style);
            fold->fill_style = fnew->fill_style;
        }
        fnew->fill_color = *fill_color;
        if ( fnew->fill_color.red != fold->fill_color.red ||
             fnew->fill_color.green != fold->fill_color.green ||
             fnew->fill_color.blue != fold->fill_color.blue ||
             fnew->fill_color.alpha != fold->fill_color.alpha)
        {
            write_elhead(renderer->file, CGM_ATTRIB, CGM_FILL_COLOR, 3); /* fill color */
            write_colour(renderer->file, &fnew->fill_color);
            putc(0, renderer->file);
            fold->fill_color = fnew->fill_color;
        }
    }
}



static void
write_text_attributes( CgmRenderer *renderer, Color *text_color)
{
TextAttrCGM    *tnew, *told;


    tnew = &renderer->tcurrent;
    told = &renderer->tinfile;
    /*
    ** Set the text attributes
    */
    if ( tnew->font_num != told->font_num )
    {
        write_elhead(renderer->file, CGM_ATTRIB, CGM_TEXT_FONT_INDEX, 2);
        write_int16(renderer->file, tnew->font_num);
        told->font_num = tnew->font_num;
    }

    if ( tnew->font_height != told->font_height )
    {
        real    h_basecap;

        /* in CGM we need the base-cap height, I used the 0.9 to correct */
        /* it because it was still to high. There might be a better way */
        /* but for now.... */

        h_basecap = 0.9 * (tnew->font_height -
                           dia_font_descent("Aq",renderer->font,
                           tnew->font_height));
        write_elhead(renderer->file, CGM_ATTRIB, CGM_CHARACTER_HEIGHT, REALSIZE);
        write_real(renderer->file, h_basecap);
        told->font_height = tnew->font_height;
    }

    tnew->color = *text_color;
    if ( tnew->color.red != told->color.red ||
         tnew->color.green != told->color.green ||
         tnew->color.blue != told->color.blue ||
         tnew->color.alpha != told->color.alpha)
    {
        write_elhead(renderer->file, CGM_ATTRIB, CGM_TEXT_COLOR, 3);   /* text colour */
        write_colour(renderer->file, &tnew->color);
        putc(0, renderer->file);
        told->color = tnew->color;
    }
}


static void
begin_render(DiaRenderer *self, const DiaRectangle *update)
{
}

static void
end_render(DiaRenderer *self)
{
    CgmRenderer *renderer = CGM_RENDERER(self);

    /* end picture */
    write_elhead(renderer->file, CGM_DELIM, CGM_END_PICTURE, 0);
    /* end metafile */
    write_elhead(renderer->file, CGM_DELIM, CGM_END_METAFILE, 0);

    fclose(renderer->file);
}

static void
set_linewidth(DiaRenderer *self, real linewidth)
{  /* 0 == hairline **/
    CgmRenderer *renderer = CGM_RENDERER(self);

    /* update current line and edge width */
    renderer->lcurrent.width = renderer->fcurrent.width = linewidth;
}


static void
set_linecaps (DiaRenderer *self, DiaLineCaps mode)
{
  CgmRenderer *renderer = CGM_RENDERER (self);
  int cap;

  switch (mode) {
    case DIA_LINE_CAPS_DEFAULT:
    case DIA_LINE_CAPS_BUTT:
      cap = 2;
      break;
    case DIA_LINE_CAPS_ROUND:
      cap = 3;
      break;
    case DIA_LINE_CAPS_PROJECTING:
      cap = 4;
      break;
    default:
      cap = 2;
      break;
  }

  renderer->lcurrent.cap = renderer->fcurrent.cap = cap;
}


static void
set_linejoin (DiaRenderer *self, DiaLineJoin mode)
{
  CgmRenderer *renderer = CGM_RENDERER (self);
  int join;

  switch (mode) {
    case DIA_LINE_JOIN_DEFAULT:
    case DIA_LINE_JOIN_MITER:
      join = 2;
      break;
    case DIA_LINE_JOIN_ROUND:
      join = 3;
      break;
    case DIA_LINE_JOIN_BEVEL:
      join = 4;
      break;
    default:
      join = 2;
      break;
  }

  renderer->lcurrent.join = renderer->fcurrent.join = join;
}


static void
set_linestyle (DiaRenderer *self, DiaLineStyle mode, double dash_length)
{
  CgmRenderer *renderer = CGM_RENDERER (self);
  gint16   style;

  /* XXX: According to specs (mil-std-2301) and tests with OpenOffice only
    *      solid=1 and dashed=2 are supported.
    */
  switch (mode) {
    case DIA_LINE_STYLE_DASHED:
      style = 2;
      break;
    case DIA_LINE_STYLE_DASH_DOT:
      style = 4;
      break;
    case DIA_LINE_STYLE_DASH_DOT_DOT:
      style = 5;
      break;
    case DIA_LINE_STYLE_DOTTED:
      style = 3;
      break;
    case DIA_LINE_STYLE_DEFAULT:
    case DIA_LINE_STYLE_SOLID:
    default:
      style = 1;
      break;
  }
  renderer->lcurrent.style = renderer->fcurrent.style = style;
}


static void
set_fillstyle (DiaRenderer *self, DiaFillStyle mode)
{
#if 0
    switch(mode) {
    case DIA_FILL_STYLE_SOLID:
	write_elhead(renderer->file, CGM_ATTRIB, CGM_INTERIOR_STYLE, 2);
	write_int16(renderer->file, 1);
	break;
    default:
	g_warning("cgm_renderer: Unsupported fill mode specified!");
    }
#endif
}


static void
wpg_renderer_set_font (DiaRenderer *self, DiaFont *font, double height)
{
  CgmRenderer *renderer = CGM_RENDERER(self);

  g_set_object (&renderer->font, font);

  renderer->tcurrent.font_num = FONT_NUM (font);
  renderer->tcurrent.font_height = height;
}


static void
draw_line (DiaRenderer *self,
           Point       *start,
           Point       *end,
           Color       *line_colour)
{
  CgmRenderer *renderer = CGM_RENDERER (self);

  write_line_attributes (renderer, line_colour);
  write_elhead (renderer->file, CGM_ELEMENT, CGM_POLYLINE, 2 * 2 * REALSIZE);

  write_real (renderer->file, start->x);
  write_real (renderer->file, swap_y (renderer, start->y));
  write_real (renderer->file, end->x);
  write_real (renderer->file, swap_y (renderer, end->y));
}


static void
draw_polyline(DiaRenderer *self,
	      Point *points, int num_points,
	      Color *line_colour)
{
    CgmRenderer *renderer = CGM_RENDERER(self);
    int i;

    write_line_attributes(renderer, line_colour);

    write_elhead(renderer->file, CGM_ELEMENT, CGM_POLYLINE, num_points * 2 * REALSIZE);
    for (i = 0; i < num_points; i++) {
	write_real(renderer->file, points[i].x);
	write_real(renderer->file, swap_y(renderer, points[i].y));
    }
}

static void
draw_polygon (DiaRenderer *self,
	      Point *points, int num_points,
	      Color *fill, Color *stroke)
{
    CgmRenderer *renderer = CGM_RENDERER(self);
    int i;

    write_filledge_attributes(renderer, fill, stroke);

    write_elhead(renderer->file, CGM_ELEMENT, CGM_POLYGON, num_points * 2 * REALSIZE);
    for (i = 0; i < num_points; i++) {
	write_real(renderer->file, points[i].x);
	write_real(renderer->file, swap_y(renderer, points[i].y));
    }
}

static void
draw_rect(DiaRenderer *self,
	  Point *ul_corner, Point *lr_corner,
	  Color *fill, Color *stroke)
{
    CgmRenderer *renderer = CGM_RENDERER(self);

    write_filledge_attributes(renderer, fill, stroke);

    write_elhead(renderer->file, CGM_ELEMENT, CGM_RECTANGLE, 4 * REALSIZE);
    write_real(renderer->file, ul_corner->x);
    write_real(renderer->file, swap_y(renderer, ul_corner->y));
    write_real(renderer->file, lr_corner->x);
    write_real(renderer->file, swap_y(renderer, lr_corner->y));
}

static void
write_ellarc (CgmRenderer   *renderer,
	      CgmElementId  elemid,
	      Point         *center,
	      real           width, real height,
	      real           angle1, real angle2 )
{
    real rx = width / 2, ry = height / 2;
    int  len;
    real ynew;

    /* Although not explicitly stated CGM seems to want counter-clockwise */
    if (angle2 < angle1) {
	real tmp = angle1;
	angle1 = angle2;
	angle2 = tmp;
    }
    /*
    ** Angle's are in degrees, need to be converted to 2PI.
    */
    angle1 = (angle1 / 360.0) * 2 * M_PI;
    angle2 = (angle2 / 360.0) * 2 * M_PI;

    ynew = swap_y(renderer, center->y);

    /*
    ** Elliptical Arc (18) or Elliptical Arc close (19).
    */
    len = elemid == CGM_ELLIPTICAL_ARC ? (10 * REALSIZE) : (10 * REALSIZE + 2);
    write_elhead(renderer->file, CGM_ELEMENT, elemid, len);
    write_real(renderer->file, center->x);        /* center */
    write_real(renderer->file, ynew);
    write_real(renderer->file, center->x + rx);   /* axes 1 */
    write_real(renderer->file, ynew);
    write_real(renderer->file, center->x);        /* axes 2 */
    write_real(renderer->file, ynew + ry);

    write_real(renderer->file, cos(angle1)); /* vector1 */
    write_real(renderer->file, sin(angle1));
    write_real(renderer->file, cos(angle2)); /* vector2 */
    write_real(renderer->file, sin(angle2));

    /*
    ** Elliptical arc close, use PIE closure.
    */
    if ( elemid == CGM_ELLIPTICAL_ARC_CLOSE )
        write_int16(renderer->file, 0);
}


static void
draw_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    CgmRenderer *renderer = CGM_RENDERER(self);

    write_line_attributes(renderer, colour);
    write_ellarc(renderer, CGM_ELLIPTICAL_ARC, center, width, height, angle1, angle2);
}

static void
fill_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    CgmRenderer *renderer = CGM_RENDERER(self);

    write_filledge_attributes(renderer, colour, NULL);
    write_ellarc(renderer, CGM_ELLIPTICAL_ARC_CLOSE, center, width, height, angle1, angle2);
}

static void
draw_ellipse(DiaRenderer *self,
	     Point *center,
	     real width, real height,
	     Color *fill, Color *stroke)
{
    CgmRenderer *renderer = CGM_RENDERER(self);
    real  ynew;

    write_filledge_attributes(renderer, fill, stroke);

    ynew = swap_y(renderer, center->y);
    write_elhead(renderer->file, CGM_ELEMENT, CGM_ELLIPSE, 6 * REALSIZE);
    write_real(renderer->file, center->x); /* center */
    write_real(renderer->file, ynew);
    write_real(renderer->file, center->x + width/2); /* axes 1 */
    write_real(renderer->file, ynew);
    write_real(renderer->file, center->x); /* axes 2 */
    write_real(renderer->file, ynew + height/2);
}

static void
write_bezier(CgmRenderer *renderer,
             BezPoint *points,
             int numpoints)
{
    int     i;
    Point   current;

    if (points[0].type != BEZ_MOVE_TO) {
      g_warning("first BezPoint must be a BEZ_MOVE_TO");
    }

    current.x = points[0].p1.x;
    current.y = swap_y(renderer, points[0].p1.y);

    for (i = 1; i < numpoints; i++) {
      switch (points[i].type) {
        case BEZ_LINE_TO:
          write_elhead(renderer->file, CGM_ELEMENT, CGM_POLYLINE, 4 * REALSIZE);
          write_real(renderer->file, current.x);
          write_real(renderer->file, current.y);
          write_real(renderer->file, points[i].p1.x);
          write_real(renderer->file, swap_y(renderer, points[i].p1.y));
          current.x = points[i].p1.x;
          current.y = swap_y(renderer, points[i].p1.y);
          break;
        case BEZ_CURVE_TO:
          write_elhead(renderer->file, CGM_ELEMENT, CGM_POLYBEZIER, 8 * REALSIZE + 2);
          write_int16(renderer->file, 1);
          write_real(renderer->file, current.x);
          write_real(renderer->file, current.y);
          write_real(renderer->file, points[i].p1.x);
          write_real(renderer->file, swap_y(renderer, points[i].p1.y));
          write_real(renderer->file, points[i].p2.x);
          write_real(renderer->file, swap_y(renderer, points[i].p2.y));
          write_real(renderer->file, points[i].p3.x);
          write_real(renderer->file, swap_y(renderer, points[i].p3.y));
          current.x = points[i].p3.x;
          current.y = swap_y(renderer, points[i].p3.y);
          break;
        case BEZ_MOVE_TO:
        default:
          g_warning ("only first BezPoint can be a BEZ_MOVE_TO");
          break;
      }
    }
}


static void
draw_bezier(DiaRenderer *self,
	    BezPoint *points,
	    int numpoints,
	    Color *colour)
{
    CgmRenderer *renderer = CGM_RENDERER(self);

    if ( numpoints < 2 )
        return;

    write_line_attributes(renderer, colour);
    write_bezier(renderer, points, numpoints);
}


static void
draw_beziergon (DiaRenderer *self,
		BezPoint *points, /* Last point must be same as first point */
		int numpoints,
		Color *fill,
		Color *stroke)
{
    CgmRenderer *renderer = CGM_RENDERER(self);

    if ( numpoints < 2 )
        return;

    write_filledge_attributes(renderer, fill, stroke);

    /*
    ** A filled bezier is created by using it within a figure.
    */
    write_elhead(renderer->file, CGM_DELIM, CGM_BEGIN_FIGURE, 0);     /* begin figure */
    write_bezier(renderer, points, numpoints);
    write_elhead(renderer->file, CGM_DELIM, CGM_END_FIGURE, 0);     /* end figure */
}


static void
draw_string (DiaRenderer  *self,
             const char   *text,
             Point        *pos,
             DiaAlignment  alignment,
             Color        *colour)
{
  CgmRenderer *renderer = CGM_RENDERER (self);
  double x = pos->x, y = swap_y (renderer, pos->y);
  int len, chunk;
  const int maxfirstchunk = 255 - 2 * REALSIZE - 2 - 1;
  const int maxappendchunk = 255 - 2 - 1;

  /* check for empty strings */
  len = strlen (text);
  if (len == 0) {
    return;
  }

  write_text_attributes (renderer, colour);

  switch (alignment) {
    case DIA_ALIGN_LEFT:
      break;
    case DIA_ALIGN_CENTRE:
      x -= dia_font_string_width (text, renderer->font,
                                  renderer->tcurrent.font_height) / 2;
      break;
    case DIA_ALIGN_RIGHT:
      x -= dia_font_string_width (text, renderer->font,
                                  renderer->tcurrent.font_height);
      break;
    default:
      g_return_if_reached ();
  }

  /* work out size of first chunk of text */
  chunk = MIN (maxfirstchunk, len);
  write_elhead (renderer->file, CGM_ELEMENT, CGM_TEXT, 2 * REALSIZE + 2 + 1 + chunk);
  write_real (renderer->file, x);
  write_real (renderer->file, y);
  write_int16 (renderer->file, (len == chunk)); /* last chunk? */
  putc (chunk, renderer->file);
  fwrite (text, sizeof(char), chunk, renderer->file);
  if (!IS_ODD (chunk)) {
    putc (0, renderer->file);
  }

  len -= chunk;
  text += chunk;
  while (len > 0) {
    /* append text */
    chunk = MIN (maxappendchunk, len);
    write_elhead (renderer->file, CGM_ELEMENT, CGM_APPEND_TEXT, 2 + 1 + chunk);
    write_int16 (renderer->file, (len == chunk));
    putc (chunk, renderer->file);
    fwrite (text, sizeof(char), chunk, renderer->file);
    if (!IS_ODD (chunk))
        putc (0, renderer->file);

    len -= chunk;
    text += chunk;
  }
}


static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage *image)
{
    CgmRenderer *renderer = CGM_RENDERER(self);
    const gint maxlen = 32767 - 6 * REALSIZE - 4 * 2;
    double x1 = point->x, y1 = swap_y(renderer, point->y),
           x2 = x1+width, y2 = y1-height;
    gint rowlen = dia_image_width(image) * 3;
    gint x, y, columns = dia_image_width(image);
    gint lines = dia_image_height(image);
    double linesize = (y1 - y2) / lines;
    gint chunk, clines;
    const guint8 *pixels;
    const GdkPixbuf* pixbuf;
    int rowstride = dia_image_rowstride(image);
    gboolean has_alpha;

    if (rowlen > maxlen) {
	dia_context_add_message(renderer->ctx,
				_("Image row length larger than maximum cell array.\n"
				  "Image not exported to CGM."));
	return;
    }

    /* don't copy the data, work on pixbuf  */
    pixbuf = dia_image_pixbuf (image);
    g_return_if_fail (pixbuf != NULL); /* very sick DiaImage */
    pixels = gdk_pixbuf_get_pixels (pixbuf);
    has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);

    while (lines > 0) {
	chunk = (MIN(rowlen * lines, maxlen) / 2) * 2; /* CELL ARRAY is WORD aligned */
	clines = chunk / rowlen;
	chunk = clines * rowlen;

	write_elhead(renderer->file, CGM_ELEMENT, CGM_CELL_ARRAY, 6*REALSIZE + 8 + chunk);
	write_real(renderer->file, x1); /* first corner */
	write_real(renderer->file, y1);
	write_real(renderer->file, x2); /* second corner */
	write_real(renderer->file, y1 - linesize*clines/*y2*/);
	write_real(renderer->file, x2); /* third corner */
	write_real(renderer->file, y1);

	/* write image size */
	write_int16(renderer->file, columns);
	write_int16(renderer->file, clines);

	write_int16(renderer->file, 8); /* color precision */
	write_int16(renderer->file, 1); /* packed encoding */

	/* fwrite(ptr, sizeof(guint8), chunk, renderer->file);
	 * ... would only work for RGB with columns%3==0 and columns%4==0
	 */
	for (y = 0; y < clines; ++y) {
	    const guint8 *ptr = pixels + (dia_image_height(image) - lines + y) * rowstride;
	    for (x = 0; x < columns; ++x) {
		fwrite(ptr, sizeof(guint8), 3, renderer->file);
		if (has_alpha)
		    ptr += 4;
		else
		    ptr += 3;
	    }
	    if (rowlen%2) /* padding */
		fwrite(ptr, sizeof(guint8), 1, renderer->file);
	}

	lines -= clines;
	y1 -= clines * linesize;
    }
}

static gboolean
export_cgm(DiagramData *data, DiaContext *ctx,
	   const gchar *filename, const gchar *diafilename,
	   void* user_data)
{
    CgmRenderer *renderer;
    FILE *file;
    DiaRectangle *extent;
    gint len;

    file = g_fopen(filename, "wb");

    if (file == NULL) {
	dia_context_add_message_with_errno (ctx, errno, _("Can't open output file %s"),
					    dia_context_get_filename(ctx));
	return FALSE;
    }

    renderer = g_object_new(CGM_TYPE_RENDERER, NULL);

    renderer->file = file;
    renderer->ctx = ctx;

    /* write BEGMF */
    len = strlen(dia_version_string());
    write_elhead(file, CGM_DELIM, CGM_BEGIN_METAFILE, len + 1);
    putc(len, file);
    fwrite(dia_version_string(), sizeof(char), len, file);
    if (!IS_ODD(len))
	putc(0, file);

    /* write metafile version */
    write_elhead(file, CGM_DESC, CGM_METAFILE_VERSION, 2);
    write_uint16(file, 3); /* use version 3 because we may use polybeziers */

#if 0
    /* write metafile description -- use original dia filename */
    len = strlen(diafilename);
    write_elhead(file, 1, 2, len + 1);
    putc(len, file);
    fwrite(diafilename, sizeof(char), len, file);
    if (!IS_ODD(len))
	putc(0, file);
#endif

    /* set integer precision */
    write_elhead(file, CGM_DESC, CGM_INTEGER_PRECISION, 2);
    write_int16(file, 16);

    /* write element virtual device unit type */
    write_elhead(file, CGM_DESC, CGM_VDC_TYPE, 2);
    write_int16(file, 1); /* real number */

    /* write the colour precision */
    write_elhead(file, CGM_DESC, CGM_COLOUR_PRECISION, 2);
    write_int16(file, 8); /* 8 bits per channel */

    /* write element list command */
    write_elhead(file, CGM_DESC, CGM_METAFILE_ELEMENT_LIST, 6);
    write_int16(file,  1);
    write_int16(file, -1);
    write_int16(file,  5);

    /* write font list */
    init_fonts();
    write_elhead(file, CGM_DESC, CGM_FONT_LIST, fontlistlen);
    fwrite(fontlist, sizeof(char), fontlistlen, file);
    if (IS_ODD(fontlistlen))
	putc(0, file);

    /* begin picture */
    len = strlen(diafilename);
    write_elhead(file, CGM_DELIM, CGM_BEGIN_PICTURE, len + 1);
    putc(len, file);
    fwrite(diafilename, sizeof(char), len, file);
    if (!IS_ODD(len))
	putc(0, file);

    /* write the color mode string */
    write_elhead(file, CGM_PICDESC, CGM_COLOR_SELECTION_MODE, 2);
    write_int16(file, 1); /* direct color mode (as opposed to indexed) */

    /* write edge width mode */
    write_elhead(file, CGM_PICDESC, CGM_EDGE_WIDTH_SPECIFICATION_MODE, 2);
    write_int16(file, 0); /* relative to virtual device coordinates */

    /* write line width mode */
    write_elhead(file, CGM_PICDESC, CGM_LINE_WIDTH_SPECIFICATION_MODE, 2);
    write_int16(file, 0); /* relative to virtual device coordinates */

    extent = &data->extents;

    /*
    ** Change the swap Y-coordinate in the CGM, to get a more
    ** 'portable' CGM
    */
    /* write extents */
    write_elhead(file, CGM_PICDESC, CGM_VDC_EXTENT, 4 * REALSIZE);
    write_real(file, extent->left);
    write_real(file, extent->top);
    write_real(file, extent->right);
    write_real(file, extent->bottom);
    renderer->y1 = extent->top;
    renderer->y0 = extent->bottom;

    /* write back color */
    write_elhead(file, CGM_PICDESC, CGM_BACKGROUND_COLOR, 3);
    write_colour(file, &data->bg_color);
    putc(0, file);

    /* begin the picture body */
    write_elhead(file, CGM_DELIM, CGM_BEGIN_PICTURE_BODY, 0);

    /* make text be the right way up */
    write_elhead(file, CGM_ATTRIB, CGM_CHARACTER_ORIENTATION, 4 * REALSIZE);
    write_real(file,  0);
    write_real(file,  1);
    write_real(file,  1);
    write_real(file,  0);

    /* set the text alignment to left/base */
    write_elhead(file, CGM_ATTRIB, CGM_TEXT_ALIGNMENT, 4 + 2 * REALSIZE);
    write_int16(file, 1);          /* left */
    write_int16(file, 4);          /* base */
    write_real(file, 0.0);
    write_real(file, 0.0);

  init_attributes (renderer);

  data_render (data, DIA_RENDERER (renderer), NULL, NULL, NULL);

  g_clear_object (&renderer);

  return TRUE;
}

/* GObject stuff */
static void cgm_renderer_class_init (CgmRendererClass *klass);

static gpointer parent_class = NULL;

GType
cgm_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (CgmRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) cgm_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (CgmRenderer),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "CgmRenderer",
                                            &object_info, 0);
    }

  return object_type;
}

static void
cgm_renderer_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  CgmRenderer *self = CGM_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      wpg_renderer_set_font (DIA_RENDERER (self),
                             g_value_get_object (value),
                             self->tcurrent.font_height);
      break;
    case PROP_FONT_HEIGHT:
      wpg_renderer_set_font (DIA_RENDERER (self),
                             self->font,
                             g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
cgm_renderer_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  CgmRenderer *self = CGM_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      g_value_set_object (value, self->font);
      break;
    case PROP_FONT_HEIGHT:
      g_value_set_double (value, self->tcurrent.font_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
cgm_renderer_finalize (GObject *object)
{
  CgmRenderer *self = CGM_RENDERER (object);

  g_clear_object (&self->font);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
cgm_renderer_class_init (CgmRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = cgm_renderer_set_property;
  object_class->get_property = cgm_renderer_get_property;
  object_class->finalize = cgm_renderer_finalize;

  renderer_class->begin_render = begin_render;
  renderer_class->end_render = end_render;

  renderer_class->set_linewidth = set_linewidth;
  renderer_class->set_linecaps = set_linecaps;
  renderer_class->set_linejoin = set_linejoin;
  renderer_class->set_linestyle = set_linestyle;
  renderer_class->set_fillstyle = set_fillstyle;

  renderer_class->draw_line = draw_line;
  renderer_class->draw_polyline = draw_polyline;

  renderer_class->draw_polygon = draw_polygon;

  renderer_class->draw_rect = draw_rect;

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


static const gchar *extensions[] = { "cgm", NULL };
static DiaExportFilter cgm_export_filter = {
    N_("Computer Graphics Metafile"),
    extensions,
    export_cgm
};


/* --- dia plug-in interface --- */
static gboolean
_plugin_can_unload (PluginInfo *info)
{
  return TRUE;
}

static void
_plugin_unload (PluginInfo *info)
{
  filter_unregister_export(&cgm_export_filter);
}

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
    if (!dia_plugin_info_init(info, "CGM",
			      _("Computer Graphics Metafile export filter"),
			      _plugin_can_unload,
                              _plugin_unload))
	return DIA_PLUGIN_INIT_ERROR;

    filter_register_export(&cgm_export_filter);

    return DIA_PLUGIN_INIT_OK;
}
