/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * hpgl.c -- HPGL export plugin for dia
 * Copyright (C) 2000, Hans Breuer, <Hans@Breuer.Org>
 *   based on dummy.c 
 *   based on CGM plug-in Copyright (C) 1999 James Henstridge. 
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
 * ToDo:
 * - provide more rendering functions like "render_draw_ellipse_by_arc" 
 *   to be used by simple format renderers; move them to libdia?
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
#include "plug-ins.h"

/* #DEFINE DEBUG_HPGL */

/* format specific */
#define HPGL_MAX_PENS 8

/* --- the renderer --- */
#define MY_RENDERER_NAME "HPGL"

#define PEN_HAS_COLOR (1 << 0) 
#define PEN_HAS_WIDTH (1 << 1) 

typedef struct _MyRenderer MyRenderer;
struct _MyRenderer {
    Renderer renderer;

    FILE *file;

    /*
     * The number of pens is limited. This is used to select one.
     */
    struct {
        Color color;
        float width;
        int   has_it;
    } pen[HPGL_MAX_PENS];
    int last_pen;
    real dash_length;
    real font_height;

    Point size;  /* extent size */
    real scale;
    real offset; /* in dia units */
};

/* include function declares and render object "vtable" */
#include "../renderer.inc"

#ifdef DEBUG_HPGL
#  define DIAG_NOTE(action) action
#else
#  define DIAG_NOTE(action)
#endif

/* hpgl helpers */
static void
hpgl_select_pen(MyRenderer* renderer, Color* color, real width)
{
    int nPen = 0;
    int i;
    /* look if this pen is defined already */
    if (0.0 != width) {
        /* either width ... */
        for (i = 0; i < HPGL_MAX_PENS; i++) {
            if (!(renderer->pen[i].has_it & PEN_HAS_WIDTH)) {
                nPen = i;
                break;
            }
            if (width == renderer->pen[i].width) {
                nPen = i;
                break;
            }
        }
    }
 
    if (NULL != color) {
        for (i = nPen; i < HPGL_MAX_PENS; i++) {
            if (!(renderer->pen[i].has_it & PEN_HAS_COLOR)) {
                nPen = i;
                break;
            }
            if (   (color->red == renderer->pen[i].color.red)
                && (color->green == renderer->pen[i].color.green)
                && (color->blue == renderer->pen[i].color.blue)) {
                nPen = i;
                break;
            }
        }
    }
    /* "create" new pen ... */
    if ((nPen < HPGL_MAX_PENS) && (-1 < nPen)) {
        if (0.0 != width) {
            renderer->pen[nPen].width = width;
            renderer->pen[nPen].has_it |= PEN_HAS_WIDTH;
        }
        if (NULL != color) {
            renderer->pen[nPen].color = *color;
            renderer->pen[nPen].has_it |= PEN_HAS_COLOR;
        }
    }
    /* ... or use best fitting one */
    else if (-1 == nPen) {
        nPen = 0; /* TODO: */
    }

    if (renderer->last_pen != nPen)
        fprintf(renderer->file, "SP%d;\n", nPen+1);
    renderer->last_pen = nPen;
}

static int
hpgl_scale(MyRenderer *renderer, real val)
{
    return (int)((val + renderer->offset) * renderer->scale);
}

/* render functions */ 
static void
begin_render(MyRenderer *renderer, DiagramData *data)
{
    int i;

    DIAG_NOTE(g_message("begin_render"));

    /* initialize pens */
    for (i = 0; i < HPGL_MAX_PENS; i++) {
        renderer->pen[i].color = color_black;
        renderer->pen[i].width = 0.0;
        renderer->pen[i].has_it = 0;
    }
    renderer->last_pen = -1;
    renderer->dash_length = 0.0;
}

static void
end_render(MyRenderer *renderer)
{
    DIAG_NOTE(g_message("end_render"));
    fclose(renderer->file);
}

static void
set_linewidth(MyRenderer *renderer, real linewidth)
{  
    DIAG_NOTE(g_message("set_linewidth %f", linewidth));

    hpgl_select_pen(renderer, NULL, linewidth);
}

static void
set_linecaps(MyRenderer *renderer, LineCaps mode)
{
    DIAG_NOTE(g_message("set_linecaps %d", mode));

    switch(mode) {
    case LINECAPS_BUTT:
	break;
    case LINECAPS_ROUND:
	break;
    case LINECAPS_PROJECTING:
	break;
    default:
	message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
    }
}

static void
set_linejoin(MyRenderer *renderer, LineJoin mode)
{
    DIAG_NOTE(g_message("set_join %d", mode));

    switch(mode) {
    case LINEJOIN_MITER:
	break;
    case LINEJOIN_ROUND:
	break;
    case LINEJOIN_BEVEL:
	break;
    default:
	message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
    }
}

static void
set_linestyle(MyRenderer *renderer, LineStyle mode)
{
    DIAG_NOTE(g_message("set_linestyle %d", mode));

    /* line type */
    switch (mode) {
    case LINESTYLE_SOLID:
      fprintf(renderer->file, "LT;\n");
      break;
    case LINESTYLE_DASHED:
      if (renderer->dash_length > 0.5) /* ??? unit of dash_lenght ? */
          fprintf(renderer->file, "LT2;\n"); /* short */
      else
          fprintf(renderer->file, "LT3;\n"); /* long */
      break;
    case LINESTYLE_DASH_DOT:
      fprintf(renderer->file, "LT4;\n");
      break;
    case LINESTYLE_DASH_DOT_DOT:
      fprintf(renderer->file, "LT5;\n"); /* ??? Mittellinie? */
      break;
    case LINESTYLE_DOTTED:
      fprintf(renderer->file, "LT1;\n");
      break;
    default:
	message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
    }
}

static void
set_dashlength(MyRenderer *renderer, real length)
{  
    DIAG_NOTE(diag_note("set_dashlength %f", length));

    /* dot = 20% of len */
    renderer->dash_length = length;
}

static void
set_fillstyle(MyRenderer *renderer, FillStyle mode)
{
    DIAG_NOTE(g_message("set_fillstyle %d", mode));

    switch(mode) {
    case FILLSTYLE_SOLID:
	break;
    default:
	message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
    }
}

static void
set_font(MyRenderer *renderer, Font *font, real height)
{
    DIAG_NOTE(g_message("set_font %f", height));
    renderer->font_height = height;
}

/* Need to translate coord system:
 * 
 *   Dia x,y -> Hpgl x,-y
 *
 * doing it before scaling.
 */
static void
draw_line(MyRenderer *renderer, 
	  Point *start, Point *end, 
	  Color *line_colour)
{
    DIAG_NOTE(g_message("draw_line %f,%f -> %f, %f", 
              start->x, start->y, end->x, end->y));
    hpgl_select_pen(renderer, line_colour, 0.0);
    fprintf (renderer->file, 
             "PU%d,%d;PD%d,%d;\n",
             hpgl_scale(renderer, start->x), hpgl_scale(renderer, -start->y),
             hpgl_scale(renderer, end->x), hpgl_scale(renderer, -end->y));
}

static void
draw_polyline(MyRenderer *renderer, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
    int i;

    DIAG_NOTE(g_message("draw_polyline n:%d %f,%f ...", 
              num_points, points->x, points->y));

    g_return_if_fail(1 < num_points);

    hpgl_select_pen(renderer, line_colour, 0.0);
    fprintf (renderer->file, "PU%d,%d;PD;PA",
             hpgl_scale(renderer, points[0].x),
             hpgl_scale(renderer, -points[0].y));
    /* absolute movement */
    for (i = 1; i < num_points-1; i++)
        fprintf(renderer->file, "%d,%d,",
                hpgl_scale(renderer, points[i].x),
                hpgl_scale(renderer, -points[i].y));
    i = num_points - 1;
    fprintf(renderer->file, "%d,%d;\n",
            hpgl_scale(renderer, points[i].x),
            hpgl_scale(renderer, -points[i].y));
}

static void
draw_polygon(MyRenderer *renderer, 
	     Point *points, int num_points, 
	     Color *line_colour)
{
    DIAG_NOTE(g_message("draw_polygon n:%d %f,%f ...", 
              num_points, points->x, points->y));
    draw_polyline(renderer,points,num_points,line_colour);
    /* last to first */
    draw_line(renderer, &points[num_points-1], &points[0], line_colour);
}

static void
fill_polygon(MyRenderer *renderer, 
	     Point *points, int num_points, 
	     Color *colour)
{
    DIAG_NOTE(g_message("fill_polygon n:%d %f,%f ...", 
              num_points, points->x, points->y));
    draw_polyline(renderer,points,num_points,colour);
}

static void
draw_rect(MyRenderer *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
    DIAG_NOTE(g_message("draw_rect %f,%f -> %f,%f", 
              ul_corner->x, ul_corner->y, lr_corner->x, lr_corner->y));
    hpgl_select_pen(renderer, colour, 0.0);
    fprintf (renderer->file, "PU%d,%d;PD;EA%d,%d;\n",
             hpgl_scale(renderer, ul_corner->x),
             hpgl_scale(renderer, -ul_corner->y),
             hpgl_scale(renderer, lr_corner->x),
             hpgl_scale(renderer, -lr_corner->y));
}

static void
fill_rect(MyRenderer *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
    DIAG_NOTE(g_message("fill_rect %f,%f -> %f,%f", 
              ul_corner->x, ul_corner->y, lr_corner->x, lr_corner->y));
#if 0
    hpgl_select_pen(renderer, colour, 0.0);
    fprintf (renderer->file, "PU%d,%d;PD;RA%d,%d;\n",
             hpgl_scale(renderer, ul_corner->x),
             hpgl_scale(renderer, -ul_corner->y),
             hpgl_scale(renderer, lr_corner->x),
             hpgl_scale(renderer, -lr_corner->y));
#else
    /* the fill modes aren't really compatible ... */
   draw_rect(renderer, ul_corner, lr_corner, colour);
#endif
}

static void
draw_arc(MyRenderer *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    Point start;

    DIAG_NOTE(g_message("draw_arc %fx%f <%f,<%f", 
              width, height, angle1, angle2));
    hpgl_select_pen(renderer, colour, 0.0);

    /* move to start point */
    start.x = center->x + (width / 2.0)  * cos((M_PI / 180.0) * angle1);  
    start.y = - center->y + (height / 2.0) * sin((M_PI / 180.0) * angle1);
    fprintf (renderer->file, "PU%d,%d;PD;",
             hpgl_scale(renderer, start.x),
             hpgl_scale(renderer, start.y));
    /* Arc Absolute - around center */
    fprintf (renderer->file, "AA%d,%d,%d;",
             hpgl_scale(renderer, center->x),
             hpgl_scale(renderer, - center->y),
             (int)floor(360.0 - angle1 + angle2));
}

static void
fill_arc(MyRenderer *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    DIAG_NOTE(g_message("fill_arc %fx%f <%f,<%f", 
              width, height, angle1, angle2));
    g_assert (width == height);

    /* move to center */
    fprintf (renderer->file, "PU%d,%d;PD;",
             hpgl_scale(renderer, center->x),
             hpgl_scale(renderer, -center->y));
    /* Edge Wedge */
    fprintf (renderer->file, "EW%d,%d,%d;",
             hpgl_scale(renderer, width),
             (int)angle1, (int)(angle2-angle1));
}

static void
draw_ellipse(MyRenderer *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
  DIAG_NOTE(g_message("draw_ellipse %fx%f center @ %f,%f", 
            width, height, center->x, center->y));

  if (width != height)
  {
    renderer_draw_ellipse_by_arc(renderer, center, width, height, colour);
  }  
  else
  {
    hpgl_select_pen(renderer, colour, 0.0);

    fprintf (renderer->file, "PU%d,%d;CI%d;\n",
             hpgl_scale(renderer, center->x),
             hpgl_scale(renderer, -center->y),
		 hpgl_scale(renderer, width / 2.0));
  }
}

static void
fill_ellipse(MyRenderer *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
    DIAG_NOTE(g_message("fill_ellipse %fx%f center @ %f,%f", 
              width, height, center->x, center->y));
}

static void
draw_bezier(MyRenderer *renderer, 
	    BezPoint *points,
	    int numpoints,
	    Color *colour)
{
    DIAG_NOTE(g_message("draw_bezier n:%d %fx%f ...", 
              numpoints, points->p1.x, points->p1.y));

    /* @todo: provide bezier rendering by simple render function callback like:
     *
     *  renderer_draw_bezier_by_line(renderer, points, numpoints, colour);
     */
}

static void
fill_bezier(MyRenderer *renderer, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *colour)
{
    DIAG_NOTE(g_message("fill_bezier n:%d %fx%f ...", 
              numpoints, points->p1.x, points->p1.y));
}

static void
draw_string(MyRenderer *renderer,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *colour)
{
    DIAG_NOTE(g_message("draw_string %f,%f %s", 
              pos->x, pos->y, text));

    /* set position */
    fprintf(renderer->file, "PU%d,%d;",
            hpgl_scale(renderer, pos->x), hpgl_scale(renderer, -pos->y));

    switch (alignment) {
    case ALIGN_LEFT:
        fprintf (renderer->file, "LO1;\n");
	break;
    case ALIGN_CENTER:
        fprintf (renderer->file, "LO4;\n");
	break;
    case ALIGN_RIGHT:
        fprintf (renderer->file, "LO7;\n");
	break;
    }
    hpgl_select_pen(renderer,colour,0.0);

#if 0
    /*
     * SR - Relative Character Size >0.0 ... 127.999
     *    set the capital letter box width and height as a percentage of
     *    P2X-P1X  and P2Y-P1Y
     */
    height = (127.999 * renderer->font_height * renderer->scale) / renderer->size.y; 
    width  = 0.75 * height; /* FIXME: */
    fprintf(renderer->file, "SR%.3f,%.3f;", width, height);
#else
    /*
     * SI - character size absolute
     *    size needed in centimeters
     */
    fprintf(renderer->file, "SI%.3f,%.3f;", 
            renderer->font_height * renderer->scale * 0.75 * 0.0025,
            renderer->font_height * renderer->scale * 0.0025);

#endif
    fprintf(renderer->file, "DT\003;" /* Terminator */
            "LB%s\003;\n", text);
}

static void
draw_image(MyRenderer *renderer,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
    DIAG_NOTE(g_message("draw_image %fx%f @%f,%f", 
              width, height, point->x, point->y));
    g_warning("HPGL: images unsupported!");
}

static void
export_data(DiagramData *data, const gchar *filename, const gchar *diafilename)
{
    MyRenderer *renderer;
    FILE *file;
    Rectangle *extent;
    real width, height;

    file = fopen(filename, "w"); /* "wb" for binary! */

    if (file == NULL) {
	message_error(_("Couldn't open: '%s' for writing.\n"), filename);
	return;
    }

    renderer = g_new(MyRenderer, 1);
    renderer->renderer.ops = &MyRenderOps;
    renderer->renderer.is_interactive = 0;
    renderer->renderer.interactive_ops = NULL;

    renderer->file = file;

    extent = &data->extents;

    /* use extents */
    DIAG_NOTE(g_message("export_data extents %f,%f -> %f,%f", 
              extent->left, extent->top, extent->right, extent->bottom));

    width  = extent->right - extent->left;
    height = extent->bottom - extent->top;
    renderer->scale = 0.001;
    if (width > height)
        while (renderer->scale * width < 3276.7) renderer->scale *= 10.0;
    else
        while (renderer->scale * height < 3276.7) renderer->scale *= 10.0;
    renderer->offset = 0.0; /* just to have one */

    renderer->size.x = width * renderer->scale;
    renderer->size.y = height * renderer->scale;
#if 0
    /* OR: set page size and scale */
    fprintf(renderer->file, "PS0;SC%d,%d,%d,%d;\n",
            hpgl_scale(renderer, extent->left),
            hpgl_scale(renderer, extent->right),
            hpgl_scale(renderer, extent->bottom),
            hpgl_scale(renderer, extent->top));
#endif
    data_render(data, (Renderer *)renderer, NULL, NULL, NULL);

    g_free(renderer);
}

static const gchar *extensions[] = { "plt", "hpgl", NULL };
static DiaExportFilter my_export_filter = {
    N_(MY_RENDERER_NAME),
    extensions,
    export_data
};


/* --- dia plug-in interface --- */

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
    if (!dia_plugin_info_init(info, "HPGL",
			      _("HP Graphics Language export filter"),
			      NULL, NULL))
	return DIA_PLUGIN_INIT_ERROR;

    filter_register_export(&my_export_filter);

    return DIA_PLUGIN_INIT_OK;
}
