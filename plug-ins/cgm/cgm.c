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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glib.h>

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "render.h"
#include "filter.h"
#include "plug-ins.h"

#if G_OS_WIN32
#include <pango/pangowin32.h>
#define pango_platform_font_map_for_display() \
    pango_win32_font_map_for_display()
#else
#include <pango/pangoft2.h>
#define pango_platform_font_map_for_display() \
    pango_ft2_font_map_for_display()
      /* Could have used X's font map ? */
#endif

static const gchar *dia_version_string = "Dia-" VERSION;
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
write_int32(FILE *fp, gint32 n)
{
    write_uint32(fp, (guint32) n);
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
write_elhead(FILE *fp, int el_class, int el_id, int nparams)
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
    PangoFontMap *fmap;
    PangoFontFamily **families;
    int n_families;
    const char *familyname;
    
    if (alreadyrun) return;
    alreadyrun = TRUE;

    fmap = pango_platform_font_map_for_display();
    pango_font_map_list_families(fmap,&families,&n_families);
    
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

typedef struct _RendererCGM RendererCGM;
struct _RendererCGM {
    Renderer renderer;

    FILE *file;

    DiaFont *font;

    real y0, y1; 

    LineAttrCGM  lcurrent, linfile;

    FillEdgeAttrCGM fcurrent, finfile;

    TextAttrCGM    tcurrent, tinfile;

};



static void begin_render(RendererCGM *renderer, DiagramData *data);
static void end_render(RendererCGM *renderer);
static void set_linewidth(RendererCGM *renderer, real linewidth);
static void set_linecaps(RendererCGM *renderer, LineCaps mode);
static void set_linejoin(RendererCGM *renderer, LineJoin mode);
static void set_linestyle(RendererCGM *renderer, LineStyle mode);
static void set_dashlength(RendererCGM *renderer, real length);
static void set_fillstyle(RendererCGM *renderer, FillStyle mode);
static void set_font(RendererCGM *renderer, DiaFont *font, real height);
static void draw_line(RendererCGM *renderer, 
		      Point *start, Point *end, 
		      Color *line_colour);
static void draw_polyline(RendererCGM *renderer, 
			  Point *points, int num_points, 
			  Color *line_colour);
static void draw_polygon(RendererCGM *renderer, 
			 Point *points, int num_points, 
			 Color *line_colour);
static void fill_polygon(RendererCGM *renderer, 
			 Point *points, int num_points, 
			 Color *line_colour);
static void draw_rect(RendererCGM *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void fill_rect(RendererCGM *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void draw_arc(RendererCGM *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void fill_arc(RendererCGM *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void draw_ellipse(RendererCGM *renderer, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void fill_ellipse(RendererCGM *renderer, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void draw_bezier(RendererCGM *renderer, 
			BezPoint *points,
			int numpoints,
			Color *colour);
static void fill_bezier(RendererCGM *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *colour);
static void draw_string(RendererCGM *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *colour);
static void draw_image(RendererCGM *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);

static RenderOps *CgmRenderOps;

static void
init_cgm_renderops()
{
    CgmRenderOps = create_renderops_table();

    CgmRenderOps->begin_render = (BeginRenderFunc) begin_render;
    CgmRenderOps->end_render = (EndRenderFunc) end_render;

    CgmRenderOps->set_linewidth = (SetLineWidthFunc) set_linewidth;
    CgmRenderOps->set_linecaps = (SetLineCapsFunc) set_linecaps;
    CgmRenderOps->set_linejoin = (SetLineJoinFunc) set_linejoin;
    CgmRenderOps->set_linestyle = (SetLineStyleFunc) set_linestyle;
    CgmRenderOps->set_dashlength = (SetDashLengthFunc) set_dashlength;
    CgmRenderOps->set_fillstyle = (SetFillStyleFunc) set_fillstyle;
    CgmRenderOps->set_font = (SetFontFunc) set_font;
  
    CgmRenderOps->draw_line = (DrawLineFunc) draw_line;
    CgmRenderOps->draw_polyline = (DrawPolyLineFunc) draw_polyline;
  
    CgmRenderOps->draw_polygon = (DrawPolygonFunc) draw_polygon;
    CgmRenderOps->fill_polygon = (FillPolygonFunc) fill_polygon;

    CgmRenderOps->draw_rect = (DrawRectangleFunc) draw_rect;
    CgmRenderOps->fill_rect = (FillRectangleFunc) fill_rect;

    CgmRenderOps->draw_arc = (DrawArcFunc) draw_arc;
    CgmRenderOps->fill_arc = (FillArcFunc) fill_arc;

    CgmRenderOps->draw_ellipse = (DrawEllipseFunc) draw_ellipse;
    CgmRenderOps->fill_ellipse = (FillEllipseFunc) fill_ellipse;

    CgmRenderOps->draw_bezier = (DrawBezierFunc) draw_bezier;
    CgmRenderOps->fill_bezier = (FillBezierFunc) fill_bezier;

    CgmRenderOps->draw_string = (DrawStringFunc) draw_string;

    CgmRenderOps->draw_image = (DrawImageFunc) draw_image;
}


static void
init_attributes( RendererCGM *renderer )
{
    /* current values, (defaults) */
    renderer->lcurrent.cap   = 3;            /* round */
    renderer->lcurrent.join  = 2;            /* mitre */
    renderer->lcurrent.style = 1;
    renderer->lcurrent.width = 0.1;
    renderer->lcurrent.color.red   = 0;
    renderer->lcurrent.color.green = 0;
    renderer->lcurrent.color.blue  = 0;

    renderer->linfile.cap    = -1;
    renderer->linfile.join   = -1;
    renderer->linfile.style  = -1;
    renderer->linfile.width  = -1.0;
    renderer->linfile.color.red   = -1;
    renderer->linfile.color.green = -1;
    renderer->linfile.color.blue  = -1;

    /* fill/edge defaults */
    renderer->fcurrent.fill_style = 1;          /* solid */
    renderer->fcurrent.fill_color.red = 0;
    renderer->fcurrent.fill_color.green = 0;
    renderer->fcurrent.fill_color.blue = 0;

    renderer->fcurrent.edgevis = 0;
    renderer->fcurrent.cap   = 3;            /* round */
    renderer->fcurrent.join  = 2;            /* mitre */
    renderer->fcurrent.style = 1;
    renderer->fcurrent.width = 0.1;
    renderer->fcurrent.color.red   = 0;
    renderer->fcurrent.color.green = 0;
    renderer->fcurrent.color.blue  = 0;
   
    renderer->finfile.fill_style = -1;
    renderer->finfile.fill_color.red = -1;
    renderer->finfile.fill_color.green = -1;
    renderer->finfile.fill_color.blue = -1;

    renderer->finfile.edgevis = -1;
    renderer->finfile.cap   = -1;            
    renderer->finfile.join  = -1;           
    renderer->finfile.style = -1;
    renderer->finfile.width = -1.;
    renderer->finfile.color.red   = -1.0;
    renderer->finfile.color.green = -1.0;
    renderer->finfile.color.blue  = -1.0;

    renderer->tcurrent.font_num    = 1;
    renderer->tcurrent.font_height = 0.1;
    renderer->tcurrent.color.red   = 0.0;
    renderer->tcurrent.color.green = 0.0;
    renderer->tcurrent.color.blue  = 0.0;

    renderer->tinfile.font_num    = -1;
    renderer->tinfile.font_height = -1.0;
    renderer->tinfile.color.red   = -1.0;
    renderer->tinfile.color.green = -1.0;
    renderer->tinfile.color.blue  = -1.0;
   
}
    


static real
swap_y( RendererCGM *renderer, real y)
{
   return  (renderer->y0 + renderer->y1 - y);
}


static void
write_line_attributes( RendererCGM *renderer, Color *color )
{
LineAttrCGM    *lnew, *lold;

    lnew = &renderer->lcurrent;
    lold = &renderer->linfile; 

    if ( lnew->cap != lold->cap )
    {
        write_elhead(renderer->file, 5, 37, 4);
        write_int16(renderer->file, lnew->cap);
        write_int16(renderer->file, 3);      /* cap of dashlines match */
                                             /* normal cap */
        lold->cap = lnew->cap;
    }
    if ( lnew->join != lold->join )
    {
        write_elhead(renderer->file, 5, 38, 2);
        write_int16(renderer->file, lnew->join);
        lold->join = lnew->join; 
    }
    if ( lnew->style != lold->style )
    {
        write_elhead(renderer->file, 5, 2, 2);
        write_int16(renderer->file, lnew->style);
        lold->style = lnew->style;
    }
    if ( lnew->width != lold->width )
    {
        write_elhead(renderer->file, 5, 3, REALSIZE);
        write_real(renderer->file, lnew->width);
        lold->width = lnew->width;
    }
    lnew->color = *color;
    if ( lnew->color.red != lold->color.red ||
         lnew->color.green != lold->color.green ||
         lnew->color.blue != lold->color.blue )
    {
        write_elhead(renderer->file, 5, 4, 3); /* line colour */
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
write_filledge_attributes( RendererCGM *renderer, Color *fill_color,
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
            write_elhead(renderer->file, 5, 30, 2);    
            write_int16(renderer->file, fnew->edgevis);
            fold->edgevis = fnew->edgevis;
        }
    }
    else
    {
        fnew->edgevis = 1;   /* edge on */
        if ( fnew->edgevis != fold->edgevis )
        {
            write_elhead(renderer->file, 5, 30, 2);  
            write_int16(renderer->file, fnew->edgevis);
            fold->edgevis = fnew->edgevis;
        }
        if ( fnew->cap != fold->cap )
        {
            write_elhead(renderer->file, 5, 44, 4);
            write_int16(renderer->file, fnew->cap);
            write_int16(renderer->file, 3);  /* cap of dashlines match */
                                             /* normal cap */
            fold->cap = fnew->cap;
        }
        if ( fnew->join != fold->join )
        {
            write_elhead(renderer->file, 5, 45, 2);
            write_int16(renderer->file, fnew->join);
            fold->join = fnew->join; 
        }
        if ( fnew->style != fold->style )
        {
            write_elhead(renderer->file, 5, 27, 2);
            write_int16(renderer->file, fnew->style);
            fold->style = fnew->style;
        }
        if ( fnew->width != fold->width )
        {
            write_elhead(renderer->file, 5, 28, REALSIZE);
            write_real(renderer->file, fnew->width);
            fold->width = fnew->width;
        }
        fnew->color = *edge_color;
        if ( fnew->color.red != fold->color.red ||
             fnew->color.green != fold->color.green ||
             fnew->color.blue != fold->color.blue )
        {
            write_elhead(renderer->file, 5, 29, 3); /* line colour */
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
            write_elhead(renderer->file, 5, 22, 2); 
            write_int16(renderer->file, fnew->fill_style);
            fold->fill_style = fnew->fill_style;
        }
    }
    else
    {
        fnew->fill_style = 1;       /* solid fill */
        if ( fnew->fill_style != fold->fill_style )
        {
            write_elhead(renderer->file, 5, 22, 2); 
            write_int16(renderer->file, fnew->fill_style);
            fold->fill_style = fnew->fill_style;
        }
        fnew->fill_color = *fill_color;
        if ( fnew->fill_color.red != fold->fill_color.red ||
             fnew->fill_color.green != fold->fill_color.green ||
             fnew->fill_color.blue != fold->fill_color.blue )
        {
            write_elhead(renderer->file, 5, 23, 3);   /* fill colour */
            write_colour(renderer->file, &fnew->fill_color);
            putc(0, renderer->file);
            fold->fill_color = fnew->fill_color;
        }
    }
}



static void
write_text_attributes( RendererCGM *renderer, Color *text_color)
{
TextAttrCGM    *tnew, *told;


    tnew = &renderer->tcurrent;
    told = &renderer->tinfile; 
    /*
    ** Set the text attributes
    */
    if ( tnew->font_num != told->font_num )
    {
        write_elhead(renderer->file, 5, 10, 2);
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
        write_elhead(renderer->file, 5, 15, REALSIZE);
        write_real(renderer->file, h_basecap);
        told->font_height = tnew->font_height;
    }

    tnew->color = *text_color;
    if ( tnew->color.red != told->color.red ||
         tnew->color.green != told->color.green ||
         tnew->color.blue != told->color.blue )
    {
        write_elhead(renderer->file, 5, 14, 3);   /* text colour */
        write_colour(renderer->file, &tnew->color);
        putc(0, renderer->file);
        told->color = tnew->color;
    }
}


static void
begin_render(RendererCGM *renderer, DiagramData *data)
{
}

static void
end_render(RendererCGM *renderer)
{
    /* end picture */
    write_elhead(renderer->file, 0, 5, 0);
    /* end metafile */
    write_elhead(renderer->file, 0, 2, 0);

    fclose(renderer->file);
}

static void
set_linewidth(RendererCGM *renderer, real linewidth)
{  /* 0 == hairline **/

    /* update current line and edge width */
    renderer->lcurrent.width = renderer->fcurrent.width = linewidth;
}

static void
set_linecaps(RendererCGM *renderer, LineCaps mode)
{
int  cap;

    switch(mode) 
    {
    case LINECAPS_BUTT:
	cap = 2;
	break;
    case LINECAPS_ROUND:
	cap = 3;
	break;
    case LINECAPS_PROJECTING:
	cap = 4;
	break;
    default:
	cap = 2;
        break;
    }
    renderer->lcurrent.cap = renderer->fcurrent.cap = cap;
}

static void
set_linejoin(RendererCGM *renderer, LineJoin mode)
{
int    join;

    switch(mode) 
    {
    case LINEJOIN_MITER:
	join = 2;
	break;
    case LINEJOIN_ROUND:
	join = 3;
	break;
    case LINEJOIN_BEVEL:
        join = 4;
	break;
    default:
	join = 2;
        break;
    }
    renderer->lcurrent.join = renderer->fcurrent.join = join;
}

static void
set_linestyle(RendererCGM *renderer, LineStyle mode)
{
gint16   style;

    switch(mode)
    {
    case LINESTYLE_DASHED:
       style = 2;
       break;
    case LINESTYLE_DASH_DOT:
       style = 4;
       break;
    case LINESTYLE_DASH_DOT_DOT:
       style = 5;
       break;
    case LINESTYLE_DOTTED:
       style = 3;
       break;
    case LINESTYLE_SOLID:
    default:
       style = 1;
       break;
    }
    renderer->lcurrent.style = renderer->fcurrent.style = style;
}

static void
set_dashlength(RendererCGM *renderer, real length)
{  /* dot = 20% of len */
    /* CGM doesn't support setting a dash length */
}

static void
set_fillstyle(RendererCGM *renderer, FillStyle mode)
{
#if 0
    switch(mode) {
    case FILLSTYLE_SOLID:
	write_elhead(renderer->file, 5, 22, 2);
	write_int16(renderer->file, 1);
	break;
    default:
	message_error("svg_renderer: Unsupported fill mode specified!\n");
    }
#endif
}

static void
set_font(RendererCGM *renderer, DiaFont *font, real height)
{
    dia_font_unref(renderer->font);
    renderer->font = dia_font_ref(font);
    renderer->tcurrent.font_num = FONT_NUM(font);
    renderer->tcurrent.font_height = height;
}

static void
draw_line(RendererCGM *renderer, 
	  Point *start, Point *end, 
	  Color *line_colour)
{

    write_line_attributes(renderer, line_colour);

    write_elhead(renderer->file, 4, 1, 4 * REALSIZE);
    write_real(renderer->file, start->x);
    write_real(renderer->file, swap_y(renderer, start->y));
    write_real(renderer->file, end->x);
    write_real(renderer->file, swap_y(renderer, end->y));
}

static void
draw_polyline(RendererCGM *renderer, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
    int i;

    write_line_attributes(renderer, line_colour);

    write_elhead(renderer->file, 4, 1, num_points * 2 * REALSIZE);
    for (i = 0; i < num_points; i++) {
	write_real(renderer->file, points[i].x);
	write_real(renderer->file, swap_y(renderer, points[i].y));
    }
}

static void
draw_polygon(RendererCGM *renderer, 
	     Point *points, int num_points, 
	     Color *line_colour)
{
    int i;

    write_filledge_attributes(renderer, NULL, line_colour);

    write_elhead(renderer->file, 4, 7, num_points * 2 * REALSIZE);
    for (i = 0; i < num_points; i++) {
	write_real(renderer->file, points[i].x);
	write_real(renderer->file, swap_y(renderer, points[i].y));
    }
}

static void
fill_polygon(RendererCGM *renderer, 
	     Point *points, int num_points, 
	     Color *colour)
{
    int i;

    write_filledge_attributes(renderer, colour, NULL);

    write_elhead(renderer->file, 4, 7, num_points * 2 * REALSIZE);
    for (i = 0; i < num_points; i++) {
	write_real(renderer->file, points[i].x);
	write_real(renderer->file, swap_y(renderer, points[i].y));
    }
}

static void
draw_rect(RendererCGM *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
    write_filledge_attributes(renderer, NULL, colour);

    write_elhead(renderer->file, 4, 11, 4 * REALSIZE);
    write_real(renderer->file, ul_corner->x);
    write_real(renderer->file, swap_y(renderer, ul_corner->y));
    write_real(renderer->file, lr_corner->x);
    write_real(renderer->file, swap_y(renderer, lr_corner->y));
}

static void
fill_rect(RendererCGM *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
    write_filledge_attributes(renderer, colour, NULL);

    write_elhead(renderer->file, 4, 11, 4 * REALSIZE);
    write_real(renderer->file, ul_corner->x);
    write_real(renderer->file, swap_y(renderer, ul_corner->y));
    write_real(renderer->file, lr_corner->x);
    write_real(renderer->file, swap_y(renderer, lr_corner->y));
}



static void
write_ellarc(RendererCGM *renderer,
             int   elemid,
             Point *center,
             real  width, real  height,
             real  angle1, real angle2 )
{
real rx = width / 2, ry = height / 2;
int  len;
real ynew;

    /*
    ** Angle's are in degrees, need to be converted to 2PI.
    */
    angle1 = (angle1 / 360.0) * 2 * M_PI;
    angle2 = (angle2 / 360.0) * 2 * M_PI;

    ynew = swap_y(renderer, center->y);

    /*
    ** Elliptical Arc (18) or Elliptical Arc close (19).
    */
    len = elemid == 18 ? (10 * REALSIZE) : (10 * REALSIZE + 2);
    write_elhead(renderer->file, 4, elemid, len);
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
    if ( elemid == 19 )
        write_int16(renderer->file, 0);
} 


static void
draw_arc(RendererCGM *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    write_line_attributes(renderer, colour);
    write_ellarc(renderer, 18, center, width, height, angle1, angle2);
}

static void
fill_arc(RendererCGM *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{

    write_filledge_attributes(renderer, colour, NULL);
    write_ellarc(renderer, 19, center, width, height, angle1, angle2);
}

static void
draw_ellipse(RendererCGM *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
real  ynew;

    write_filledge_attributes(renderer, NULL, colour);

    ynew = swap_y(renderer, center->y);
    write_elhead(renderer->file, 4, 17, 6 * REALSIZE);
    write_real(renderer->file, center->x); /* center */
    write_real(renderer->file, ynew);
    write_real(renderer->file, center->x);      /* axes 1 */
    write_real(renderer->file, ynew + height/2);
    write_real(renderer->file, center->x + width/2); /* axes 2 */
    write_real(renderer->file, ynew );
}

static void
fill_ellipse(RendererCGM *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
real ynew;

    write_filledge_attributes(renderer, colour, NULL);

    ynew = swap_y(renderer, center->y);
    write_elhead(renderer->file, 4, 17, 6 * REALSIZE);
    write_real(renderer->file, center->x); /* center */
    write_real(renderer->file, ynew);
    write_real(renderer->file, center->x);      /* axes 1 */
    write_real(renderer->file, ynew + height/2);
    write_real(renderer->file, center->x + width/2); /* axes 2 */
    write_real(renderer->file, ynew);
}


static void 
write_bezier(RendererCGM *renderer, 
             BezPoint *points, 
             int numpoints) 
{
    int     i;
    Point   current;

    if (points[0].type != BEZ_MOVE_TO)
	g_warning("first BezPoint must be a BEZ_MOVE_TO");

    current.x = points[0].p1.x;
    current.y = swap_y(renderer, points[0].p1.y);

    for (i = 1; i < numpoints; i++)
    {
	switch (points[i].type) 
        {
	case BEZ_MOVE_TO:
	    g_warning("only first BezPoint can be a BEZ_MOVE_TO");
	    break;
	case BEZ_LINE_TO:
            write_elhead(renderer->file, 4, 1, 4 * REALSIZE);
            write_real(renderer->file, current.x);
            write_real(renderer->file, current.y);
            write_real(renderer->file, points[i].p1.x);
            write_real(renderer->file, swap_y(renderer, points[i].p1.y));
            current.x = points[i].p1.x;
            current.y = swap_y(renderer, points[i].p1.y);
            break;
	case BEZ_CURVE_TO:
            write_elhead(renderer->file, 4, 26, 8 * REALSIZE + 2);
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
        }
    }
}


static void
draw_bezier(RendererCGM *renderer, 
	    BezPoint *points,
	    int numpoints,
	    Color *colour)
{
    if ( numpoints < 2 )
        return;

    write_line_attributes(renderer, colour);
    write_bezier(renderer, points, numpoints);
}


static void
fill_bezier(RendererCGM *renderer, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *colour)
{
    if ( numpoints < 2 )
        return;

    write_filledge_attributes(renderer, colour, NULL);

    /*
    ** A filled bezier is created by using it within a figure.
    */
    write_elhead(renderer->file, 0, 8, 0);     /* begin figure */
    write_bezier(renderer, points, numpoints);
    write_elhead(renderer->file, 0, 9, 0);     /* end figure */
}



static void
draw_string(RendererCGM *renderer,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *colour)
{
    double x = pos->x, y = swap_y(renderer, pos->y);
    gint len, chunk;
    const gint maxfirstchunk = 255 - 2 * REALSIZE - 2 - 1;
    const gint maxappendchunk = 255 - 2 - 1;

    /* check for empty strings */
    len = strlen(text);
    if ( len == 0 )
        return;

    write_text_attributes(renderer, colour);

    switch (alignment) {
    case ALIGN_LEFT:
        break;
    case ALIGN_CENTER:
        x -= dia_font_string_width(text, renderer->font, 
                                   renderer->tcurrent.font_height)/2;
        break;
    case ALIGN_RIGHT:
        x -= dia_font_string_width(text, renderer->font, 
                                   renderer->tcurrent.font_height);
        break;
    }
    /* work out size of first chunk of text */
    chunk = MIN(maxfirstchunk, len);
    write_elhead(renderer->file, 4, 4, 2 * REALSIZE + 2 + 1 + chunk);
    write_real(renderer->file, x);
    write_real(renderer->file, y);
    write_int16(renderer->file, (len == chunk)); /* last chunk? */
    putc(chunk, renderer->file);
    fwrite(text, sizeof(char), chunk, renderer->file);
    if (!IS_ODD(chunk))
	putc(0, renderer->file);

    len -= chunk;
    text += chunk;
    while (len > 0) {
	/* append text */
	chunk = MIN(maxappendchunk, len);
	write_elhead(renderer->file, 4, 6, 2 + 1 + chunk);
	write_int16(renderer->file, (len == chunk));
	putc(chunk, renderer->file);
	fwrite(text, sizeof(char), chunk, renderer->file);
	if (!IS_ODD(chunk))
	    putc(0, renderer->file);

	len -= chunk;
	text += chunk;
    }
}


static void
draw_image(RendererCGM *renderer,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
    const gint maxlen = 32767 - 6 * REALSIZE - 4 * 2;
    double x1 = point->x, y1 = swap_y(renderer, point->y), 
           x2 = x1+width, y2 = y1-height;
    gint rowlen = dia_image_width(image) * 3, lines = dia_image_height(image);
    double linesize = (y1 - y2) / lines;
    gint chunk, clines = lines;
    guint8 *pImg, *ptr;

    if (rowlen > maxlen) {
	message_error(_("Image row length larger than maximum cell array.\n"
			"Image not exported to CGM."));
	return;
    }

    ptr = pImg = dia_image_rgb_data(image);

    while (lines > 0) {
	chunk = MIN(rowlen * lines, maxlen);
	clines = chunk / rowlen;
	chunk = clines * rowlen;

	write_elhead(renderer->file, 4, 9, 6*REALSIZE + 8 + chunk);
	write_real(renderer->file, x1); /* first corner */
	write_real(renderer->file, y1);
	write_real(renderer->file, x2); /* second corner */
	write_real(renderer->file, y1 - linesize*clines/*y2*/);
	write_real(renderer->file, x2); /* third corner */
	write_real(renderer->file, y1);

	/* write image size */
	write_int16(renderer->file, dia_image_width(image));
	write_int16(renderer->file, clines);

	write_int16(renderer->file, 8); /* colour precision */
	write_int16(renderer->file, 1); /* packed encoding */

	fwrite(ptr, sizeof(guint8), chunk, renderer->file);

	lines -= clines;
	ptr += chunk;
	y1 -= clines * linesize;
    }
    g_free (pImg);
}

static void
export_cgm(DiagramData *data, const gchar *filename, 
           const gchar *diafilename, void* user_data)
{
    RendererCGM *renderer;
    FILE *file;
#if THIS_IS_PROBABLY_DEAD_CODE
    gchar buf[512];
#endif
    Rectangle *extent;
    gint len;

    file = fopen(filename, "wb");

    if (file == NULL) {
	message_error(_("Couldn't open: '%s' for writing.\n"), filename);
	return;
    }

    if (CgmRenderOps == NULL)
	init_cgm_renderops();

    renderer = g_new(RendererCGM, 1);
    renderer->renderer.ops = CgmRenderOps;
    renderer->renderer.is_interactive = 0;
    renderer->renderer.interactive_ops = NULL;

    renderer->file = file;

    /* write BEGMF */
    len = strlen(dia_version_string);
    write_elhead(file, 0, 1, len + 1);
    putc(len, file);
    fwrite(dia_version_string, sizeof(char), len, file);
    if (!IS_ODD(len))
	putc(0, file);

    /* write metafile version */
    write_elhead(file, 1, 1, 2);
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
    write_elhead(file, 1, 4, 2);
    write_int16(file, 16);

    /* write element virtual device unit type */
    write_elhead(file, 1, 3, 2);
    write_int16(file, 1); /* real number */

    /* write the colour precision */
    write_elhead(file, 1, 7, 2);
    write_int16(file, 8); /* 8 bits per chanel */

    /* write element list command */
    write_elhead(file, 1, 11, 6);
    write_int16(file,  1);
    write_int16(file, -1);
    write_int16(file,  5);

    /* write font list */
    init_fonts();
    write_elhead(file, 1, 13, fontlistlen);
    fwrite(fontlist, sizeof(char), fontlistlen, file);
    if (IS_ODD(fontlistlen))
	putc(0, file);
 
    /* begin picture */
    len = strlen(diafilename);
    write_elhead(file, 0, 3, len + 1);
    putc(len, file);
    fwrite(diafilename, sizeof(char), len, file);
    if (!IS_ODD(len))
	putc(0, file);

    /* write the colour mode string */
    write_elhead(file, 2, 2, 2);
    write_int16(file, 1); /* direct colour mode (as opposed to indexed) */

    /* write edge width mode */
    write_elhead(file, 2, 5, 2);
    write_int16(file, 0); /* relative to virtual device coordinates */

    /* write line width mode */
    write_elhead(file, 2, 3, 2);
    write_int16(file, 0); /* relative to virtual device coordinates */

    extent = &data->extents;

    /*
    ** Change the swap Y-coordinate in the CGM, to get a more 
    ** 'portable' CGM   
    */
    /* write extents */
    write_elhead(file, 2, 6, 4 * REALSIZE);
    write_real(file, extent->left);
    write_real(file, extent->top);
    write_real(file, extent->right);
    write_real(file, extent->bottom);
    renderer->y1 = extent->top;
    renderer->y0 = extent->bottom;

    /* write back colour */
    write_elhead(file, 2, 7, 3);
    write_colour(file, &data->bg_color);
    putc(0, file);

    /* begin the picture body */
    write_elhead(file, 0, 4, 0);

    /* make text be the right way up */
    write_elhead(file, 5, 16, 4 * REALSIZE);
    write_real(file,  0);
    write_real(file,  1);
    write_real(file,  1);
    write_real(file,  0);

    /* set the text alignment to left/base */
    write_elhead(file, 5, 18, 4 + 2 * REALSIZE);
    write_int16(file, 1);          /* left */
    write_int16(file, 4);          /* base */
    write_real(file, 0.0);
    write_real(file, 0.0);

    init_attributes(renderer);

    data_render(data, (Renderer *)renderer, NULL, NULL, NULL);

    dia_font_unref(renderer->font);
    g_free(renderer);
}

static const gchar *extensions[] = { "cgm", NULL };
static DiaExportFilter cgm_export_filter = {
    N_("Computer Graphics Metafile"),
    extensions,
    export_cgm
};


/* --- dia plug-in interface --- */

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
    if (!dia_plugin_info_init(info, "CGM",
			      _("Computer Graphics Metafile export filter"),
			      NULL, NULL))
	return DIA_PLUGIN_INIT_ERROR;

    filter_register_export(&cgm_export_filter);

    return DIA_PLUGIN_INIT_OK;
}
