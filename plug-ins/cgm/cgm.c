/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * cgm.c -- CGM plugin for dia
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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glib.h>

#include "config.h"
#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "render.h"
#include "filter.h"

static const gchar *dia_version_string = "Dia-" VERSION;
#define IS_ODD(n) (n & 0x01)

/* --- routines to write various quantities to the CGM stream. --- */

static void
write_int16(FILE *fp, gint16 n)
{
    if (n < 0) {
	n = -n;
	putc( ((n & 0xff00) >> 8) | 0x80, fp);
    } else
	putc( (n & 0xff00) >> 8, fp);
    putc( n & 0xff, fp);
}

static void
write_uint16(FILE *fp, guint16 n)
{
    putc( (n & 0xff00) >> 8, fp);
    putc( n & 0xff, fp);
}

static void
write_int32(FILE *fp, gint32 n)
{
    if (n < 0) {
	n = -n;
	putc( ((n & 0xff000000) >> 24) | 0x80, fp);
    } else
	putc( (n & 0xff000000) >> 24, fp);
    putc( (n & 0xff0000) >> 16, fp);
    putc( (n & 0xff00) >> 8, fp);
    putc( n & 0xff, fp);
}

static void
write_uint32(FILE *fp, guint32 n)
{
    putc( (n & 0xff000000) >> 24, fp);
    putc( (n & 0xff0000) >> 16, fp);
    putc( (n & 0xff00) >> 8, fp);
    putc( n & 0xff, fp);
}

static void
write_colour(FILE *fp, Color *c)
{
    putc((int) c->red   * 255, fp);
    putc((int) c->green * 255, fp);
    putc((int) c->blue  * 255, fp);
}

#define REALSIZE 4
/* 32 bit fixed point real number */
static void
write_real(FILE *fp, double x)
{
    write_int32(fp, (gint32) (x * (1 << 16)));
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
  font->name))

static void
init_fonts(void)
{
    static gboolean alreadyrun = FALSE;
    GList *tmp;
    GString *str;
    gint i;

    if (alreadyrun) return;
    alreadyrun = TRUE;

    fonthash = g_hash_table_new(g_str_hash, g_str_equal);
    str = g_string_new(NULL);
    for (tmp = font_names, i = 1; tmp != NULL; tmp = tmp->next, i++) {
	g_string_append_c(str, strlen(tmp->data));
	g_string_append(str, tmp->data);
	g_hash_table_insert(fonthash, tmp->data, GINT_TO_POINTER(i));
    }
    fontlist = str->str;
    fontlistlen = str->len;
    g_string_free(str, FALSE);
}

/* --- the renderer --- */

typedef struct _RendererCGM RendererCGM;
struct _RendererCGM {
    Renderer renderer;

    FILE *file;

    Font *font;
    real font_height;
};

static void begin_render(RendererCGM *renderer, DiagramData *data);
static void end_render(RendererCGM *renderer);
static void set_linewidth(RendererCGM *renderer, real linewidth);
static void set_linecaps(RendererCGM *renderer, LineCaps mode);
static void set_linejoin(RendererCGM *renderer, LineJoin mode);
static void set_linestyle(RendererCGM *renderer, LineStyle mode);
static void set_dashlength(RendererCGM *renderer, real length);
static void set_fillstyle(RendererCGM *renderer, FillStyle mode);
static void set_font(RendererCGM *renderer, Font *font, real height);
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

static RenderOps CgmRenderOps = {
    (BeginRenderFunc) begin_render,
    (EndRenderFunc) end_render,

    (SetLineWidthFunc) set_linewidth,
    (SetLineCapsFunc) set_linecaps,
    (SetLineJoinFunc) set_linejoin,
    (SetLineStyleFunc) set_linestyle,
    (SetDashLengthFunc) set_dashlength,
    (SetFillStyleFunc) set_fillstyle,
    (SetFontFunc) set_font,
  
    (DrawLineFunc) draw_line,
    (DrawPolyLineFunc) draw_polyline,
  
    (DrawPolygonFunc) draw_polygon,
    (FillPolygonFunc) fill_polygon,

    (DrawRectangleFunc) draw_rect,
    (FillRectangleFunc) fill_rect,

    (DrawArcFunc) draw_arc,
    (FillArcFunc) fill_arc,

    (DrawEllipseFunc) draw_ellipse,
    (FillEllipseFunc) fill_ellipse,

    (DrawBezierFunc) draw_bezier,
    (FillBezierFunc) fill_bezier,

    (DrawStringFunc) draw_string,

    (DrawImageFunc) draw_image,
};

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
    /* line width */
    write_elhead(renderer->file, 5, 3, REALSIZE);
    write_real(renderer->file, linewidth);
    /* edge width */
    write_elhead(renderer->file, 5, 28, REALSIZE);
    write_real(renderer->file, linewidth);
}

static void
set_linecaps(RendererCGM *renderer, LineCaps mode)
{
#if 0
    switch(mode) {
    case LINECAPS_BUTT:
	renderer->linecap = "butt";
	break;
    case LINECAPS_ROUND:
	renderer->linecap = "round";
	break;
    case LINECAPS_PROJECTING:
	renderer->linecap = "square";
	break;
    default:
	renderer->linecap = "butt";
    }
#endif
}

static void
set_linejoin(RendererCGM *renderer, LineJoin mode)
{
#if 0
    switch(mode) {
    case LINEJOIN_MITER:
	renderer->linejoin = "miter";
	break;
    case LINEJOIN_ROUND:
	renderer->linejoin = "round";
	break;
    case LINEJOIN_BEVEL:
	renderer->linejoin = "bevel";
	break;
    default:
	renderer->linejoin = "miter";
    }
#endif
}

static void
set_linestyle(RendererCGM *renderer, LineStyle mode)
{
    /* cgm's line style selection is quite small

    /* line type */
    write_elhead(renderer->file, 5, 2, 2);
    if (mode == LINESTYLE_SOLID)
	write_int16(renderer->file, 1);
    else
	write_int16(renderer->file, 2);

    /* edge type */
    write_elhead(renderer->file, 5, 27, 2);
    if (mode == LINESTYLE_SOLID)
	write_int16(renderer->file, 1);
    else
	write_int16(renderer->file, 2);
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
set_font(RendererCGM *renderer, Font *font, real height)
{
    write_elhead(renderer->file, 5, 10, 2);
    write_int16(renderer->file, FONT_NUM(font));

    write_elhead(renderer->file, 5, 15, REALSIZE);
    write_real(renderer->file, height);

    renderer->font = font;
    renderer->font_height = height;
}

static void
draw_line(RendererCGM *renderer, 
	  Point *start, Point *end, 
	  Color *line_colour)
{
    write_elhead(renderer->file, 5, 4, 3); /* line colour */
    write_colour(renderer->file, line_colour);
    putc(0, renderer->file);

    write_elhead(renderer->file, 4, 1, 4 * REALSIZE);
    write_real(renderer->file, start->x);
    write_real(renderer->file, start->y);
    write_real(renderer->file, end->x);
    write_real(renderer->file, end->y);
}

static void
draw_polyline(RendererCGM *renderer, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
    int i;

    write_elhead(renderer->file, 5, 4, 3); /* line colour */
    write_colour(renderer->file, line_colour);
    putc(0, renderer->file);

    write_elhead(renderer->file, 4, 1, num_points * 2 * REALSIZE);
    for (i = 0; i < num_points; i++) {
	write_real(renderer->file, points[i].x);
	write_real(renderer->file, points[i].y);
    }
}

static void
draw_polygon(RendererCGM *renderer, 
	     Point *points, int num_points, 
	     Color *line_colour)
{
    int i;

    write_elhead(renderer->file, 5, 22, 2); /* don't fill */
    write_int16(renderer->file, 4);

    write_elhead(renderer->file, 5, 30, 2); /* visible edge */
    write_int16(renderer->file, 1);

    write_elhead(renderer->file, 5, 29, 3); /* edge colour */
    write_colour(renderer->file, line_colour);
    putc(0, renderer->file);

    write_elhead(renderer->file, 4, 7, num_points * 2 * REALSIZE);
    for (i = 0; i < num_points; i++) {
	write_real(renderer->file, points[i].x);
	write_real(renderer->file, points[i].y);
    }
}

static void
fill_polygon(RendererCGM *renderer, 
	     Point *points, int num_points, 
	     Color *colour)
{
    int i;

    write_elhead(renderer->file, 5, 22, 2); /* fill */
    write_int16(renderer->file, 1);

    write_elhead(renderer->file, 5, 30, 2); /* don't visible edge */
    write_int16(renderer->file, 0);

    write_elhead(renderer->file, 5, 23, 3); /* fill colour */
    write_colour(renderer->file, colour);
    putc(0, renderer->file);

    write_elhead(renderer->file, 4, 7, num_points * 2 * REALSIZE);
    for (i = 0; i < num_points; i++) {
	write_real(renderer->file, points[i].x);
	write_real(renderer->file, points[i].y);
    }
}

static void
draw_rect(RendererCGM *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
    write_elhead(renderer->file, 5, 22, 2); /* don't fill */
    write_int16(renderer->file, 4);

    write_elhead(renderer->file, 5, 30, 2); /* visible edge */
    write_int16(renderer->file, 1);

    write_elhead(renderer->file, 5, 29, 3); /* edge colour */
    write_colour(renderer->file, colour);
    putc(0, renderer->file);

    write_elhead(renderer->file, 4, 11, 4 * REALSIZE);
    write_real(renderer->file, ul_corner->x);
    write_real(renderer->file, ul_corner->y);
    write_real(renderer->file, lr_corner->x);
    write_real(renderer->file, lr_corner->y);
}

static void
fill_rect(RendererCGM *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
    write_elhead(renderer->file, 5, 22, 2); /* fill */
    write_int16(renderer->file, 1);

    write_elhead(renderer->file, 5, 30, 2); /* don't visible edge */
    write_int16(renderer->file, 0);

    write_elhead(renderer->file, 5, 23, 3); /* fill colour */
    write_colour(renderer->file, colour);
    putc(0, renderer->file);

    write_elhead(renderer->file, 4, 11, 4 * REALSIZE);
    write_real(renderer->file, ul_corner->x);
    write_real(renderer->file, ul_corner->y);
    write_real(renderer->file, lr_corner->x);
    write_real(renderer->file, lr_corner->y);
}

static void
draw_arc(RendererCGM *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    real rx = width / 2, ry = height / 2;

    write_elhead(renderer->file, 5, 4, 3); /* line colour */
    write_colour(renderer->file, colour);
    putc(0, renderer->file);

    write_elhead(renderer->file, 4, 18, 10 * REALSIZE);
    write_real(renderer->file, center->x); /* center */
    write_real(renderer->file, center->y);
    write_real(renderer->file, center->x);      /* axes 1 */
    write_real(renderer->file, center->y + ry);
    write_real(renderer->file, center->x + rx); /* axes 2 */
    write_real(renderer->file, center->y);
    write_real(renderer->file, rx * cos(angle1)); /* vector1 */
    write_real(renderer->file, ry * sin(angle1));
    write_real(renderer->file, rx * cos(angle2)); /* vector2 */
    write_real(renderer->file, ry * sin(angle2));
}

static void
fill_arc(RendererCGM *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    real rx = width / 2, ry = height / 2;

    write_elhead(renderer->file, 5, 22, 2); /* fill */
    write_int16(renderer->file, 1);

    write_elhead(renderer->file, 5, 30, 2); /* don't visible edge */
    write_int16(renderer->file, 0);

    write_elhead(renderer->file, 5, 23, 3); /* fill colour */
    write_colour(renderer->file, colour);
    putc(0, renderer->file);

    write_elhead(renderer->file, 4, 18, 10 * REALSIZE);
    write_real(renderer->file, center->x); /* center */
    write_real(renderer->file, center->y);
    write_real(renderer->file, center->x);      /* axes 1 */
    write_real(renderer->file, center->y + ry);
    write_real(renderer->file, center->x + rx); /* axes 2 */
    write_real(renderer->file, center->y);
    write_real(renderer->file, rx * cos(angle1)); /* vector1 */
    write_real(renderer->file, ry * sin(angle1));
    write_real(renderer->file, rx * cos(angle2)); /* vector2 */
    write_real(renderer->file, ry * sin(angle2));
}

static void
draw_ellipse(RendererCGM *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
    write_elhead(renderer->file, 5, 22, 2); /* don't fill */
    write_int16(renderer->file, 4);

    write_elhead(renderer->file, 5, 30, 2); /* visible edge */
    write_int16(renderer->file, 1);

    write_elhead(renderer->file, 5, 29, 3); /* edge colour */
    write_colour(renderer->file, colour);
    putc(0, renderer->file);

    write_elhead(renderer->file, 4, 18, 6 * REALSIZE);
    write_real(renderer->file, center->x); /* center */
    write_real(renderer->file, center->y);
    write_real(renderer->file, center->x);      /* axes 1 */
    write_real(renderer->file, center->y + height/2);
    write_real(renderer->file, center->x + width/2); /* axes 2 */
    write_real(renderer->file, center->y);
}

static void
fill_ellipse(RendererCGM *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
    write_elhead(renderer->file, 5, 22, 2); /* fill */
    write_int16(renderer->file, 1);

    write_elhead(renderer->file, 5, 30, 2); /* don't visible edge */
    write_int16(renderer->file, 0);

    write_elhead(renderer->file, 5, 23, 3); /* fill colour */
    write_colour(renderer->file, colour);
    putc(0, renderer->file);

    write_elhead(renderer->file, 4, 18, 6 * REALSIZE);
    write_real(renderer->file, center->x); /* center */
    write_real(renderer->file, center->y);
    write_real(renderer->file, center->x);      /* axes 1 */
    write_real(renderer->file, center->y + height/2);
    write_real(renderer->file, center->x + width/2); /* axes 2 */
    write_real(renderer->file, center->y);
}

static void
draw_bezier(RendererCGM *renderer, 
	    BezPoint *points,
	    int numpoints,
	    Color *colour)
{
#if 0
    int i;
    xmlNodePtr node;
    GString *str;

    node = xmlNewChild(renderer->root, NULL, "path", NULL);
  
    xmlSetProp(node, "style", get_draw_style(renderer, colour));

    str = g_string_new(NULL);

    if (points[0].type != BEZ_MOVE_TO)
	g_warning("first BezPoint must be a BEZ_MOVE_TO");

    g_string_sprintf(str, "M %g %g", (double)points[0].p1.x,
		     (double)points[0].p1.y);

    for (i = 1; i < numpoints; i++)
	switch (points[i].type) {
	case BEZ_MOVE_TO:
	    g_warning("only first BezPoint can be a BEZ_MOVE_TO");
	    break;
	case BEZ_LINE_TO:
	    g_string_sprintfa(str, " L %g,%g",
			      (double) points[i].p1.x, (double) points[i].p1.y);
	    break;
	case BEZ_CURVE_TO:
	    g_string_sprintfa(str, " C %g,%g %g,%g %g,%g",
			      (double) points[i].p1.x, (double) points[i].p1.y,
			      (double) points[i].p2.x, (double) points[i].p2.y,
			      (double) points[i].p3.x, (double) points[i].p3.y );
	    break;
	}
    xmlSetProp(node, "d", str->str);
    g_string_free(str, TRUE);
#endif
}

static void
fill_bezier(RendererCGM *renderer, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *colour)
{
#if 0
    int i;
    xmlNodePtr node;
    GString *str;

    node = xmlNewChild(renderer->root, NULL, "path", NULL);
  
    xmlSetProp(node, "style", get_fill_style(renderer, colour));

    str = g_string_new(NULL);

    if (points[0].type != BEZ_MOVE_TO)
	g_warning("first BezPoint must be a BEZ_MOVE_TO");

    g_string_sprintf(str, "M %g %g", (double)points[0].p1.x,
		     (double)points[0].p1.y);
 
    for (i = 1; i < numpoints; i++)
	switch (points[i].type) {
	case BEZ_MOVE_TO:
	    g_warning("only first BezPoint can be a BEZ_MOVE_TO");
	    break;
	case BEZ_LINE_TO:
	    g_string_sprintfa(str, " L %g,%g",
			      (double) points[i].p1.x, (double) points[i].p1.y);
	    break;
	case BEZ_CURVE_TO:
	    g_string_sprintfa(str, " C %g,%g %g,%g %g,%g",
			      (double) points[i].p1.x, (double) points[i].p1.y,
			      (double) points[i].p2.x, (double) points[i].p2.y,
			      (double) points[i].p3.x, (double) points[i].p3.y );
	    break;
	}
    g_string_append(str, "z");
    xmlSetProp(node, "d", str->str);
    g_string_free(str, TRUE);
#endif
}

static void
draw_string(RendererCGM *renderer,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *colour)
{
    double x = pos->x, y = pos->y;
    gint len, chunk;
    const gint maxfirstchunk = 255 - 2 * REALSIZE - 2 - 1;
    const gint maxappendchunk = 255 - 2 - 1;

    switch (alignment) {
    case ALIGN_LEFT:
	break;
    case ALIGN_CENTER:
	x -= font_string_width(text, renderer->font, renderer->font_height)/2;
	break;
    case ALIGN_RIGHT:
	x -= font_string_width(text, renderer->font, renderer->font_height);
	break;
    }
    /* work out size of first chunk of text */
    len = strlen(text);
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
    double x1 = point->x, y1 = point->y, x2 = x1+width, y2 = y1+height;
    gint rowlen = dia_image_width(image) * 3, lines = dia_image_height(image);
    double linesize = (y2 - y1) / lines;
    gint chunk, clines = lines;
    guint8 *ptr = dia_image_rgb_data(image);

    while (lines > 0) {
	chunk = MIN(rowlen * lines, maxlen);
	if (chunk == maxlen) {
	    clines = chunk / rowlen;
	    chunk = clines * rowlen;
	}

	write_elhead(renderer->file, 4, 9, 6*REALSIZE + 8 + chunk);
	write_real(renderer->file, x1); /* first corner */
	write_real(renderer->file, y1);
	write_real(renderer->file, x2); /* second corner */
	write_real(renderer->file, y1 + linesize*clines/*y2*/);
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
	y1 += clines * linesize;
    }
}

static void
export_cgm(DiagramData *data, const gchar *filename, const gchar *diafilename)
{
    RendererCGM *renderer;
    FILE *file;
    gchar buf[512];
    Rectangle *extent;
    gint len;

    g_message("Exporting %s as %s", diafilename, filename);

    file = fopen(filename, "wb");

    if (file == NULL) {
	message_error(_("Couldn't open: '%s' for writing.\n"), filename);
	return;
    }

    renderer = g_new(RendererCGM, 1);
    renderer->renderer.ops = &CgmRenderOps;
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

    /* write metafile description -- use original dia filename */
    len = strlen(diafilename);
    write_elhead(file, 1, 2, len + 1);
    putc(len, file);
    fwrite(diafilename, sizeof(char), len, file);
    if (!IS_ODD(len))
	putc(0, file);

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
    write_int16(file,  1);

    /* write font list */
    init_fonts();
    write_elhead(file, 1, 13, fontlistlen);
    fwrite(fontlist, sizeof(char), fontlistlen, file);
    if (IS_ODD(fontlistlen))
	putc(0, file);
 
    /* begin picture */
    write_elhead(file, 0, 3, 1);
    putc(0, file);
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

    /* write extents */
    write_elhead(file, 2, 6, 4 * REALSIZE);
    write_real(file, extent->left);
    write_real(file, extent->bottom);
    write_real(file, extent->right);
    write_real(file, extent->top);

    /* write back colour */
    write_elhead(file, 2, 7, 4);
    write_colour(file, &data->bg_color);
    putc(0, file);

    /* begin the picture body */
    write_elhead(file, 0, 4, 0);

    /* make text be the right way up */
    write_elhead(file, 5, 16, 4 * REALSIZE);
    write_real(file,  0);
    write_real(file, -1);
    write_real(file,  1);
    write_real(file,  0);

    data_render(data, (Renderer *)renderer, NULL, NULL, NULL);

    g_free(renderer);
}

static const gchar *extensions[] = { "cgm", NULL };
static DiaExportFilter cgm_export_filter = {
    N_("Computer Graphics Metafile"),
    extensions,
    export_cgm
};


/* --- dia plug-in interface --- */

int
get_version(void)
{
    return 0;
}

void
register_objects(void)
{
    filter_register_export(&cgm_export_filter);
}

void
register_sheets(void)
{
}
