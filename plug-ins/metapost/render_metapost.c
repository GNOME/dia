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
 * 3. Font selection (I like using the latex font though)
 * 4. Font size
 * 5. Images
 * 6. Pen widths
 */


#include <config.h>

#include <string.h>
#include <time.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "intl.h"
#include "render_metapost.h"
#include "message.h"
#include "diagramdata.h"

#define POINTS_in_INCH 28.346

static void end_draw_op(RendererMETAPOST *renderer);
static void draw_with_linestyle(RendererMETAPOST *renderer);

static void begin_render(RendererMETAPOST *renderer, DiagramData *data);
static void end_render(RendererMETAPOST *renderer);
static void set_linewidth(RendererMETAPOST *renderer, real linewidth);
static void set_linecaps(RendererMETAPOST *renderer, LineCaps mode);
static void set_linejoin(RendererMETAPOST *renderer, LineJoin mode);
static void set_linestyle(RendererMETAPOST *renderer, LineStyle mode);
static void set_dashlength(RendererMETAPOST *renderer, real length);
static void set_fillstyle(RendererMETAPOST *renderer, FillStyle mode);
static void set_font(RendererMETAPOST *renderer, DiaFont *font, real height);
static void draw_line(RendererMETAPOST *renderer, 
		      Point *start, Point *end, 
		      Color *line_color);
static void draw_polyline(RendererMETAPOST *renderer, 
			  Point *points, int num_points, 
			  Color *line_color);
static void draw_polygon(RendererMETAPOST *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void fill_polygon(RendererMETAPOST *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void draw_rect(RendererMETAPOST *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void fill_rect(RendererMETAPOST *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void draw_arc(RendererMETAPOST *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void fill_arc(RendererMETAPOST *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void draw_ellipse(RendererMETAPOST *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void fill_ellipse(RendererMETAPOST *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void draw_bezier(RendererMETAPOST *renderer, 
			BezPoint *points,
			int numpoints,
			Color *color);
static void fill_bezier(RendererMETAPOST *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *color);
static void draw_string(RendererMETAPOST *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *color);
static void draw_image(RendererMETAPOST *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);

static RenderOps MetapostRenderOps = {
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
end_draw_op(RendererMETAPOST *renderer)
{
    /*
     * this was originally put here to handle each drawoption per
     * primitive  
     */ 
    draw_with_linestyle(renderer);
    fprintf(renderer->file, ";\n");
}
	    
static void 
set_line_color(RendererMETAPOST *renderer,Color *color)
{
    fprintf(renderer->file, "%% set_line_color\n");
    fprintf(renderer->file,
	    "drawoptions(withcolor (%f,%f,%f));\n",
	    (double) color->red, (double) color->green, (double) color->blue);
}

static void 
set_fill_color(RendererMETAPOST *renderer,Color *color)
{
    set_line_color(renderer,color);
}

RendererMETAPOST *
new_metapost_renderer(DiagramData *data, const char *filename,
		      const char *diafilename)
{
    RendererMETAPOST *renderer;
    FILE *file;
    time_t time_now;
    double scale;
    Rectangle *extent;
    char *name;

    Color initial_color;
 
    file = fopen(filename, "wb");

    if (file==NULL) {
	message_error(_("Couldn't open: '%s' for writing.\n"), filename);
	return NULL;
    }

    renderer = g_new(RendererMETAPOST, 1);
    renderer->renderer.ops = &MetapostRenderOps;
    renderer->renderer.is_interactive = 0;
    renderer->renderer.interactive_ops = NULL;

    renderer->file = file;

    renderer->dash_length = 1.0;
    renderer->dot_length = 0.2;
    renderer->saved_line_style = LINESTYLE_SOLID;
  
    time_now  = time(NULL);
    extent = &data->extents;
  
    scale = POINTS_in_INCH * data->paper.scaling;
  
    name = getlogin();
    if (name==NULL)
	name = "a user";
  
    fprintf(file,
	    "%% Metapost TeX macro\n"
	    "%% Title: %s\n"
	    "%% Creator: Dia v%s\n"
	    "%% CreationDate: %s"
	    "%% For: %s\n"
	    "\n\n"
	    "beginfig(1);\n",
	    diafilename,
	    VERSION,
	    ctime(&time_now),
	    name);
 
    fprintf(renderer->file,"%% picture(%f,%f)(%f,%f)\n",
	    extent->left * data->paper.scaling,
	    -extent->bottom * data->paper.scaling,
	    extent->right * data->paper.scaling,
	    -extent->top * data->paper.scaling
	    );
    fprintf(renderer->file,"x = %fcm; y = %fcm;\n\n",
	    data->paper.scaling,
	    -data->paper.scaling);

   
    initial_color.red=0.;
    initial_color.green=0.;
    initial_color.blue=0.;
    set_line_color(renderer,&initial_color);
    
    initial_color.red=1.;
    initial_color.green=1.;
    initial_color.blue=1.;
    set_fill_color(renderer,&initial_color);

    return renderer;
}

void
destroy_metapost_renderer(RendererMETAPOST *renderer)
{
    g_free(renderer);
}


static void
begin_render(RendererMETAPOST *renderer, DiagramData *data)
{
}

static void
end_render(RendererMETAPOST *renderer)
{
  
    fprintf(renderer->file,"endfig;\n");
    fprintf(renderer->file,"end;\n");
    fclose(renderer->file);
}

static void
set_linewidth(RendererMETAPOST *renderer, real linewidth)
{  /* 0 == hairline **/
    fprintf(renderer->file, "drawoptions (withpen pencircle scaled %fx);\n", (double) linewidth); 
}

static void
set_linecaps(RendererMETAPOST *renderer, LineCaps mode)
{
    if(mode == renderer->saved_line_cap)
	return;
  
    switch(mode) {
    case LINECAPS_BUTT:
	fprintf(renderer->file, "linecap:=butt;\n");
	break;
    case LINECAPS_ROUND:
	fprintf(renderer->file, "linecap:=rounded;\n");
	break;
    case LINECAPS_PROJECTING:
	/* is this right? */
	fprintf(renderer->file, "linecap:=squared;\n");
	break;
    default:
	fprintf(renderer->file, "linecap:=squared;\n");
    }

    renderer->saved_line_cap = mode;
}

static void
set_linejoin(RendererMETAPOST *renderer, LineJoin mode)
{
    if(mode == renderer->saved_line_join)
	return;

    switch(mode) {
    case LINEJOIN_MITER:
	fprintf(renderer->file, "linejoin:=mitered;\n");
	break;
    case LINEJOIN_ROUND:
	fprintf(renderer->file, "linejoin:=rounded;\n");
	break;
    case LINEJOIN_BEVEL:
	fprintf(renderer->file, "linejoin:=beveled;\n");
	break;
    default:
    }

    renderer->saved_line_join = mode;
}

static void 
set_linestyle(RendererMETAPOST *renderer, LineStyle mode)
{
    renderer->saved_line_style = mode;
}

static void
draw_with_linestyle(RendererMETAPOST *renderer)
{
    real hole_width;

    switch(renderer->saved_line_style) {
    case LINESTYLE_SOLID:
	break;
    case LINESTYLE_DASHED:
	fprintf(renderer->file, "\n dashed dashpattern (on %fx off %fx)", 
		renderer->dash_length,
		renderer->dash_length);
	break;
    case LINESTYLE_DASH_DOT:
	hole_width = (renderer->dash_length - renderer->dot_length) / 2.0;
	fprintf(renderer->file, "\n dashed dashpattern (on %fx off %fx on %fx off %fx)",
		renderer->dash_length,
		hole_width,
		renderer->dot_length,
		hole_width );
	break;
    case LINESTYLE_DASH_DOT_DOT:
	hole_width = (renderer->dash_length - 2.0*renderer->dot_length) / 3.0;
	fprintf(renderer->file, "\n dashed dashpattern (on %fx off %fx on %fx off %fx on %fx off %fx)",
		renderer->dash_length,
		hole_width,
		renderer->dot_length,
		hole_width,
		renderer->dot_length,
		hole_width );
	break;
    case LINESTYLE_DOTTED:
	fprintf(renderer->file, "\n dashed dashpattern (on %fx off %fx)", 
		renderer->dash_length,
		renderer->dash_length);
	break;
    }
}

static void
set_dashlength(RendererMETAPOST *renderer, real length)
{  /* dot = 20% of len */
    if (length<0.001)
	length = 0.001;
  
    renderer->dash_length = length;
    renderer->dot_length = length*0.05;
  
    set_linestyle(renderer, renderer->saved_line_style);
}

static void
set_fillstyle(RendererMETAPOST *renderer, FillStyle mode)
{
    switch(mode) {
    case FILLSTYLE_SOLID:
	break;
    default:
	message_error("metapost_renderer: Unsupported fill mode specified!\n");
    }
}

static void
set_font(RendererMETAPOST *renderer, DiaFont *font, real height)
{
    /*
     * TODO: implement size, figure out how to specify tex document font 
     */
#if 0
    fprintf(renderer->file, "defaultfont:=\"%s\";\n", font_get_psfontname(font));
#endif
}

static void
draw_line(RendererMETAPOST *renderer, 
	  Point *start, Point *end, 
	  Color *line_color)
{
    set_line_color(renderer,line_color);

    fprintf(renderer->file, "draw (%fx,%fy)--(%fx,%fy)",
	    start->x, start->y, end->x, end->y);
    end_draw_op(renderer);
}

static void
draw_polyline(RendererMETAPOST *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
    int i;
    
    set_line_color(renderer,line_color);
  
    fprintf(renderer->file, 
	    "draw (%fx,%fy)",
	    points[0].x, points[0].y);

    for (i=1;i<num_points;i++) {
	fprintf(renderer->file, "--(%fx,%fy)",
		points[i].x, points[i].y);
    }
    end_draw_op(renderer);
}

static void
draw_polygon(RendererMETAPOST *renderer, 
	     Point *points, int num_points, 
	     Color *line_color)
{
    int i;

    set_line_color(renderer,line_color);
  
    fprintf(renderer->file, 
	    "draw (%fx,%fy)",
	    points[0].x, points[0].y);

    for (i=1;i<num_points;i++) {
	fprintf(renderer->file, "--(%fx,%fy)",
		points[i].x, points[i].y);
    }
    fprintf(renderer->file,"--cycle");
    end_draw_op(renderer);
}

static void
fill_polygon(RendererMETAPOST *renderer, 
	     Point *points, int num_points, 
	     Color *line_color)
{
    int i;

    set_line_color(renderer,line_color);
    
    fprintf(renderer->file, 
	    "path p;\n"
	    "p = (%f,%f)",
	    points[0].x, points[0].y);

    for (i=1;i<num_points;i++) {
	fprintf(renderer->file, "--(%fx,%fy)",
		points[i].x, points[i].y);
    }
    fprintf(renderer->file,"--cycle");
    end_draw_op(renderer);
}

static void
draw_rect(RendererMETAPOST *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
    set_line_color(renderer,color);

    fprintf(renderer->file, 
	    "draw (%fx,%fy)--(%fx,%fy)--(%fx,%fy)--(%fx,%fy)--cycle",
	    (double) ul_corner->x, (double) ul_corner->y,
	    (double) ul_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) ul_corner->y );
    end_draw_op(renderer);
}

static void
fill_rect(RendererMETAPOST *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
    fprintf(renderer->file, 
	    "path p;\n"
	    "p = (%fx,%fy)--(%fx,%fy)--(%fx,%fy)--(%fx,%fy)--cycle;\n",
	    (double) ul_corner->x, (double) ul_corner->y,
	    (double) ul_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) ul_corner->y );
    fprintf(renderer->file,
	    "fill p withcolor (%f,%f,%f);\n",
	    (double) color->red, (double) color->green, (double) color->blue);
}

static void
metapost_arc(RendererMETAPOST *renderer, 
	     Point *center,
	     real width, real height,
	     real angle1, real angle2,
	     Color *color,int filled)
{
    double radius1,radius2;
    double angle3 = (double) angle2 - angle1;
    double cx = (double) center->x;
    double cy = (double) center->y;

    radius1=(double) width/2.0;
    radius2=(double) height/2.0;

    set_line_color(renderer,color);

    fprintf(renderer->file,
	    "draw (%fx,%fy)..(%fx,%fy)..(%fx,%fy)",
	    cx + radius1*cos(angle1), cy - radius2*sin(angle1),
	    cx + radius1*cos(angle3), cy - radius2*sin(angle3),
	    cx + radius1*cos(angle2), cy - radius2*sin(angle2));
    end_draw_op(renderer);
}

static void
draw_arc(RendererMETAPOST *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
    metapost_arc(renderer,center,width,height,angle1,angle2,color,0);
}

static void
fill_arc(RendererMETAPOST *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{ 
   metapost_arc(renderer,center,width,height,angle1,angle2,color,1);
}

static void
draw_ellipse(RendererMETAPOST *renderer, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
    set_line_color(renderer,color);
    
    fprintf(renderer->file, 
	    "draw (%fx,%fy)..(%fx,%fy)..(%fx,%fy)..(%fx,%fy)..cycle",
	    (double) center->x+width/2.0, (double) center->y,
	    (double) center->x,           (double) center->y+height/2.0,
	    (double) center->x-width/2.0, (double) center->y,
	    (double) center->x,           (double) center->y-height/2.0);
    end_draw_op(renderer);
}

static void
fill_ellipse(RendererMETAPOST *renderer, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
    fprintf(renderer->file, 
	    "path p;\n"
	    "p = (%fx,%fy)..(%fx,%fy)..(%fx,%fy)..(%fx,%fy)..cycle;\n",
	    (double) center->x+width/2.0, (double) center->y,
	    (double) center->x,           (double) center->y+height/2.0,
	    (double) center->x-width/2.0, (double) center->y,
	    (double) center->x,           (double) center->y-height/2.0);

    fprintf(renderer->file,
	    "fill p withcolor (%f,%f,%f);\n",
	    (double) color->red, (double) color->green, (double) color->blue);
}



static void
draw_bezier(RendererMETAPOST *renderer, 
	    BezPoint *points,
	    int numpoints, /* numpoints = 4+3*n, n=>0 */
	    Color *color)
{
    int i;

    set_line_color(renderer,color);

    if (points[0].type != BEZ_MOVE_TO)
	g_warning("first BezPoint must be a BEZ_MOVE_TO");

    fprintf(renderer->file, "draw (%fx,%fy)",
	    (double) points[0].p1.x, (double) points[0].p1.y);
  
    for (i = 1; i < numpoints; i++)
	switch (points[i].type) {
	case BEZ_MOVE_TO:
	    g_warning("only first BezPoint can be a BEZ_MOVE_TO");
	    break;
	case BEZ_LINE_TO:
	    fprintf(renderer->file, "--(%fx,%fy)",
		    (double) points[i].p1.x, (double) points[i].p1.y);
	    break;
	case BEZ_CURVE_TO:
	    fprintf(renderer->file, "..controls (%fx,%fy) and (%fx,%fy)\n ..(%fx,%fy)\n",
		    (double) points[i].p1.x, (double) points[i].p1.y,
		    (double) points[i].p2.x, (double) points[i].p2.y,
		    (double) points[i].p3.x, (double) points[i].p3.y);
	    break;
	}
    end_draw_op(renderer);
}



static void
fill_bezier(RendererMETAPOST *renderer, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *color)
{
    int i;

    if (points[0].type != BEZ_MOVE_TO)
	g_warning("first BezPoint must be a BEZ_MOVE_TO");

    fprintf(renderer->file, "path p;\n");
    fprintf(renderer->file, "p = (%fx,%fy)",
	    (double) points[0].p1.x, (double) points[0].p1.y);
  
    for (i = 1; i < numpoints; i++)
	switch (points[i].type) {
	case BEZ_MOVE_TO:
	    g_warning("only first BezPoint can be a BEZ_MOVE_TO");
	    break;
	case BEZ_LINE_TO:
	    fprintf(renderer->file, "--(%fx,%fy)",
		    (double) points[i].p1.x, (double) points[i].p1.y);
	    break;
	case BEZ_CURVE_TO:
	    fprintf(renderer->file, "..controls (%fx,%fy) and (%fx,%fy)\n ..(%fx,%fy)\n",
		    (double) points[i].p1.x, (double) points[i].p1.y,
		    (double) points[i].p2.x, (double) points[i].p2.y,
		    (double) points[i].p3.x, (double) points[i].p3.y);
	    break;
	}
    fprintf(renderer->file, "\n ..cycle;\n");

    fprintf(renderer->file,
	    "fill p withcolor (%f,%f,%f);\n",
	    (double) color->red, (double) color->green, (double) color->blue);
}

static void
draw_string(RendererMETAPOST *renderer,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
    set_line_color(renderer,color);

    switch (alignment) {
    case ALIGN_LEFT:
	fprintf(renderer->file,"label.rt");
	break;
    case ALIGN_CENTER:
	fprintf(renderer->file,"label");
	break;
    case ALIGN_RIGHT:
	fprintf(renderer->file,"label.lft");
	break;
    }
    fprintf(renderer->file,
	    "(btex %s etex,(%fx,%fy));\n",
	    text, pos->x, pos->y);
}

static void
draw_image(RendererMETAPOST *renderer,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
    /* TODO: implement image */
}

/* --- export filter interface --- */
static void
export_metapost(DiagramData *data, const gchar *filename, 
                const gchar *diafilename, void* user_data)
{
    RendererMETAPOST *renderer;

    renderer = new_metapost_renderer(data, filename, diafilename);
    data_render(data, (Renderer *)renderer, NULL, NULL, NULL);
    destroy_metapost_renderer(renderer);
}

static const gchar *extensions[] = { "mp", NULL };
DiaExportFilter metapost_export_filter = {
    N_("TeX Metapost macros"),
    extensions,
    export_metapost
};
