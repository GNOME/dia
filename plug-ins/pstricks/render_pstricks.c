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
4. join procedures for drawing and filling
5. Maybe draw and fill in a single move?? Will solve the problems
   with visible thin white line between the border and the fill

NOT WORKING (exporting macros):
 1. linecaps
 2. linejoins
 3. dashdot and dashdotdot line styles

*/


#include <config.h>

#include <string.h>
#include <time.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "intl.h"
#include "render_pstricks.h"
#include "message.h"
#include "diagramdata.h"

#define POINTS_in_INCH 28.346

static void begin_render(RendererPSTRICKS *renderer, DiagramData *data);
static void end_render(RendererPSTRICKS *renderer);
static void set_linewidth(RendererPSTRICKS *renderer, real linewidth);
static void set_linecaps(RendererPSTRICKS *renderer, LineCaps mode);
static void set_linejoin(RendererPSTRICKS *renderer, LineJoin mode);
static void set_linestyle(RendererPSTRICKS *renderer, LineStyle mode);
static void set_dashlength(RendererPSTRICKS *renderer, real length);
static void set_fillstyle(RendererPSTRICKS *renderer, FillStyle mode);
static void set_font(RendererPSTRICKS *renderer, DiaFont *font, real height);
static void draw_line(RendererPSTRICKS *renderer, 
		      Point *start, Point *end, 
		      Color *line_color);
static void draw_polyline(RendererPSTRICKS *renderer, 
			  Point *points, int num_points, 
			  Color *line_color);
static void draw_polygon(RendererPSTRICKS *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void fill_polygon(RendererPSTRICKS *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void draw_rect(RendererPSTRICKS *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void fill_rect(RendererPSTRICKS *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void draw_arc(RendererPSTRICKS *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void fill_arc(RendererPSTRICKS *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void draw_ellipse(RendererPSTRICKS *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void fill_ellipse(RendererPSTRICKS *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void draw_bezier(RendererPSTRICKS *renderer, 
			BezPoint *points,
			int numpoints,
			Color *color);
static void fill_bezier(RendererPSTRICKS *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *color);
static void draw_string(RendererPSTRICKS *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *color);
static void draw_image(RendererPSTRICKS *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);

static RenderOps *PstricksRenderOps;

static void
init_pstricks_renderops() 
{
    PstricksRenderOps = create_renderops_table();

    PstricksRenderOps->begin_render = (BeginRenderFunc) begin_render;
    PstricksRenderOps->end_render = (EndRenderFunc) end_render;

    PstricksRenderOps->set_linewidth = (SetLineWidthFunc) set_linewidth;
    PstricksRenderOps->set_linecaps = (SetLineCapsFunc) set_linecaps;
    PstricksRenderOps->set_linejoin = (SetLineJoinFunc) set_linejoin;
    PstricksRenderOps->set_linestyle = (SetLineStyleFunc) set_linestyle;
    PstricksRenderOps->set_dashlength = (SetDashLengthFunc) set_dashlength;
    PstricksRenderOps->set_fillstyle = (SetFillStyleFunc) set_fillstyle;
    PstricksRenderOps->set_font = (SetFontFunc) set_font;
  
    PstricksRenderOps->draw_line = (DrawLineFunc) draw_line;
    PstricksRenderOps->draw_polyline = (DrawPolyLineFunc) draw_polyline;
  
    PstricksRenderOps->draw_polygon = (DrawPolygonFunc) draw_polygon;
    PstricksRenderOps->fill_polygon = (FillPolygonFunc) fill_polygon;

    PstricksRenderOps->draw_rect = (DrawRectangleFunc) draw_rect;
    PstricksRenderOps->fill_rect = (FillRectangleFunc) fill_rect;

    PstricksRenderOps->draw_arc = (DrawArcFunc) draw_arc;
    PstricksRenderOps->fill_arc = (FillArcFunc) fill_arc;

    PstricksRenderOps->draw_ellipse = (DrawEllipseFunc) draw_ellipse;
    PstricksRenderOps->fill_ellipse = (FillEllipseFunc) fill_ellipse;

    PstricksRenderOps->draw_bezier = (DrawBezierFunc) draw_bezier;
    PstricksRenderOps->fill_bezier = (FillBezierFunc) fill_bezier;

    PstricksRenderOps->draw_string = (DrawStringFunc) draw_string;

    PstricksRenderOps->draw_image = (DrawImageFunc) draw_image;
}


static void 
set_line_color(RendererPSTRICKS *renderer,Color *color)
{
    fprintf(renderer->file, "\\newrgbcolor{dialinecolor}{%f %f %f}\n",
	    (double) color->red, (double) color->green, (double) color->blue);
    fprintf(renderer->file,"\\psset{linecolor=dialinecolor}\n");
}

static void 
set_fill_color(RendererPSTRICKS *renderer,Color *color)
{
    fprintf(renderer->file, "\\newrgbcolor{diafillcolor}{%f %f %f}\n",
	    (double) color->red, (double) color->green, (double) color->blue);
    fprintf(renderer->file,"\\psset{fillcolor=diafillcolor}\n");
}

RendererPSTRICKS *
new_pstricks_renderer(DiagramData *data, const char *filename,
		      const char *diafilename)
{
    RendererPSTRICKS *renderer;
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

    if (PstricksRenderOps == NULL)
	init_pstricks_renderops();

    renderer = g_new(RendererPSTRICKS, 1);
    renderer->renderer.ops = PstricksRenderOps;
    renderer->renderer.is_interactive = 0;
    renderer->renderer.interactive_ops = NULL;

    renderer->pagenum = 1;
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
	VERSION,
	ctime(&time_now),
	name);

    fprintf(renderer->file,"\\pspicture(%f,%f)(%f,%f)\n",
	    extent->left * data->paper.scaling,
	    -extent->bottom * data->paper.scaling,
	    extent->right * data->paper.scaling,
	    -extent->top * data->paper.scaling
	    );
    fprintf(renderer->file,"\\scalebox{%f %f}{\n",
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
destroy_pstricks_renderer(RendererPSTRICKS *renderer)
{
    g_free(renderer);
}


static void
begin_render(RendererPSTRICKS *renderer, DiagramData *data)
{
}

static void
end_render(RendererPSTRICKS *renderer)
{
  
    fprintf(renderer->file,"}\\endpspicture");
    fclose(renderer->file);
}

static void
set_linewidth(RendererPSTRICKS *renderer, real linewidth)
{  /* 0 == hairline **/

    fprintf(renderer->file, "\\psset{linewidth=%f}\n", (double) linewidth);
}

static void
set_linecaps(RendererPSTRICKS *renderer, LineCaps mode)
{
    int ps_mode;
  
    switch(mode) {
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

    fprintf(renderer->file, "\\setlinecaps{%d}\n", ps_mode);
}

static void
set_linejoin(RendererPSTRICKS *renderer, LineJoin mode)
{
    int ps_mode;
  
    switch(mode) {
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

    fprintf(renderer->file, "\\setlinejoinmode{%d}\n", ps_mode);
}

static void
set_linestyle(RendererPSTRICKS *renderer, LineStyle mode)
{
    real hole_width;

    renderer->saved_line_style = mode;
  
    switch(mode) {
    case LINESTYLE_SOLID:
	fprintf(renderer->file, "\\psset{linestyle=solid}\n");
	break;
    case LINESTYLE_DASHED:
	fprintf(renderer->file, "\\psset{linestyle=dashed,dash=%f %f}\n", 
		renderer->dash_length,
		renderer->dash_length);
	break;
    case LINESTYLE_DASH_DOT:
	hole_width = (renderer->dash_length - renderer->dot_length) / 2.0;
	fprintf(renderer->file, "\\linestyleddashdotted{%f %f %f %f}\n",
		renderer->dash_length,
		hole_width,
		renderer->dot_length,
		hole_width );
	break;
    case LINESTYLE_DASH_DOT_DOT:
	hole_width = (renderer->dash_length - 2.0*renderer->dot_length) / 3.0;
	fprintf(renderer->file, "\\linestyleddashdotdotted{%f %f %f %f %f %f}\n",
		renderer->dash_length,
		hole_width,
		renderer->dot_length,
		hole_width,
		renderer->dot_length,
		hole_width );
	break;
    case LINESTYLE_DOTTED:
	fprintf(renderer->file, "\\psset{linestyle=dotted,dotsep=%f}\n", renderer->dot_length);
	break;
    }
}

static void
set_dashlength(RendererPSTRICKS *renderer, real length)
{  /* dot = 20% of len */
    if (length<0.001)
	length = 0.001;
  
    renderer->dash_length = length;
    renderer->dot_length = length*0.2;
  
    set_linestyle(renderer, renderer->saved_line_style);
}

static void
set_fillstyle(RendererPSTRICKS *renderer, FillStyle mode)
{
    switch(mode) {
    case FILLSTYLE_SOLID:
	break;
    default:
	message_error("pstricks_renderer: Unsupported fill mode specified!\n");
    }
}

static void
set_font(RendererPSTRICKS *renderer, DiaFont *font, real height)
{

    fprintf(renderer->file, "\\setfont{%s}{%f}\n", font_get_psfontname(font), (double)height);
}

static void
draw_line(RendererPSTRICKS *renderer, 
	  Point *start, Point *end, 
	  Color *line_color)
{
    set_line_color(renderer,line_color);

    fprintf(renderer->file, "\\psline(%f,%f)(%f,%f)\n",
	    start->x, start->y, end->x, end->y);
}

static void
draw_polyline(RendererPSTRICKS *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
    int i;
  
    set_line_color(renderer,line_color);  
    fprintf(renderer->file, "\\psline(%f,%f)",
	    points[0].x, points[0].y);

    for (i=1;i<num_points;i++) {
	fprintf(renderer->file, "(%f,%f)",
		points[i].x, points[i].y);
    }

    fprintf(renderer->file, "\n");
}

static void
draw_polygon(RendererPSTRICKS *renderer, 
	     Point *points, int num_points, 
	     Color *line_color)
{
    int i;

    set_line_color(renderer,line_color);
  
    fprintf(renderer->file, "\\pspolygon(%f,%f)",
	    points[0].x, points[0].y);

    for (i=1;i<num_points;i++) {
	fprintf(renderer->file, "(%f,%f)",
		points[i].x, points[i].y);
    }
    fprintf(renderer->file,"\n");
}

static void
fill_polygon(RendererPSTRICKS *renderer, 
	     Point *points, int num_points, 
	     Color *line_color)
{
    int i;
  
    set_line_color(renderer,line_color);
  
    fprintf(renderer->file, "\\pspolygon*(%f,%f)",
	    points[0].x, points[0].y);

    for (i=1;i<num_points;i++) {
	fprintf(renderer->file, "(%f,%f)",
		points[i].x, points[i].y);
    }
    fprintf(renderer->file,"\n");
}

static void
draw_rect(RendererPSTRICKS *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
    set_line_color(renderer,color);
  
    fprintf(renderer->file, "\\pspolygon(%f,%f)(%f,%f)(%f,%f)(%f,%f)\n",
	    (double) ul_corner->x, (double) ul_corner->y,
	    (double) ul_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) ul_corner->y );

}

static void
fill_rect(RendererPSTRICKS *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
    set_line_color(renderer,color);

    fprintf(renderer->file, 
	    "\\pspolygon*(%f,%f)(%f,%f)(%f,%f)(%f,%f)\n",
	    (double) ul_corner->x, (double) ul_corner->y,
	    (double) ul_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) ul_corner->y );
}

static void
pstricks_arc(RendererPSTRICKS *renderer, 
	     Point *center,
	     real width, real height,
	     real angle1, real angle2,
	     Color *color,int filled)
{
    double radius1,radius2;
    radius1=(double) width/2.0;
    radius2=(double) height/2.0;

    set_line_color(renderer,color);

    fprintf(renderer->file,"\\psclip{\\pswedge[linestyle=none,fillstyle=none](%f,%f){%f}{%f}{%f}}\n",
	    (double) center->x, (double) center->y,
	    sqrt(radius1*radius1+radius2*radius2),
	    (double) 360.0 - angle2, (double) 360.0 - angle1);
  
    fprintf(renderer->file,"\\psellipse%s(%f,%f)(%f,%f)\n",
	    (filled?"*":""),
	    (double) center->x, (double) center->y,
	    radius1,radius2);

    fprintf(renderer->file,"\\endpsclip\n");
}

static void
draw_arc(RendererPSTRICKS *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
    pstricks_arc(renderer,center,width,height,angle1,angle2,color,0);
}

static void
fill_arc(RendererPSTRICKS *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
    pstricks_arc(renderer,center,width,height,angle1,angle2,color,1);
}

static void
draw_ellipse(RendererPSTRICKS *renderer, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
    set_line_color(renderer,color);

    fprintf(renderer->file, "\\psellipse(%f,%f)(%f,%f)\n",
	    (double) center->x, (double) center->y,
	    (double) width/2.0, (double) height/2.0 );
}

static void
fill_ellipse(RendererPSTRICKS *renderer, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
    set_line_color(renderer,color);

    fprintf(renderer->file, "\\psellipse*(%f,%f)(%f,%f)\n",
	    (double) center->x, (double) center->y,
	    (double) width/2.0, (double) height/2.0 );
}



static void
draw_bezier(RendererPSTRICKS *renderer, 
	    BezPoint *points,
	    int numpoints, /* numpoints = 4+3*n, n=>0 */
	    Color *color)
{
    int i;

    set_line_color(renderer,color);

    fprintf(renderer->file, "\\pscustom{\n");

    if (points[0].type != BEZ_MOVE_TO)
	g_warning("first BezPoint must be a BEZ_MOVE_TO");

    fprintf(renderer->file, "\\newpath\n\\moveto(%f,%f)\n",
	    (double) points[0].p1.x, (double) points[0].p1.y);
  
    for (i = 1; i < numpoints; i++)
	switch (points[i].type) {
	case BEZ_MOVE_TO:
	    g_warning("only first BezPoint can be a BEZ_MOVE_TO");
	    break;
	case BEZ_LINE_TO:
	    fprintf(renderer->file, "\\lineto(%f,%f)\n",
		    (double) points[i].p1.x, (double) points[i].p1.y);
	    break;
	case BEZ_CURVE_TO:
	    fprintf(renderer->file, "\\curveto(%f,%f)(%f,%f)(%f,%f)\n",
		    (double) points[i].p1.x, (double) points[i].p1.y,
		    (double) points[i].p2.x, (double) points[i].p2.y,
		    (double) points[i].p3.x, (double) points[i].p3.y );
	    break;
	}

    fprintf(renderer->file, "\\stroke}\n");
}



static void
fill_bezier(RendererPSTRICKS *renderer, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *color)
{
    int i;

    set_fill_color(renderer,color);

    fprintf(renderer->file, "\\pscustom{\n");

    if (points[0].type != BEZ_MOVE_TO)
	g_warning("first BezPoint must be a BEZ_MOVE_TO");

    fprintf(renderer->file, "\\newpath\n\\moveto(%f,%f)\n",
	    (double) points[0].p1.x, (double) points[0].p1.y);
  
    for (i = 1; i < numpoints; i++)
	switch (points[i].type) {
	case BEZ_MOVE_TO:
	    g_warning("only first BezPoint can be a BEZ_MOVE_TO");
	    break;
	case BEZ_LINE_TO:
	    fprintf(renderer->file, "\\lineto(%f,%f)\n",
		    (double) points[i].p1.x, (double) points[i].p1.y);
	    break;
	case BEZ_CURVE_TO:
	    fprintf(renderer->file, "\\curveto(%f,%f)(%f,%f)(%f,%f)\n",
		    (double) points[i].p1.x, (double) points[i].p1.y,
		    (double) points[i].p2.x, (double) points[i].p2.y,
		    (double) points[i].p3.x, (double) points[i].p3.y );
	    break;
	}

    fprintf(renderer->file, "\\fill[fillstyle=solid,fillcolor=diafillcolor,linecolor=diafillcolor]}\n");
}

static void
draw_string(RendererPSTRICKS *renderer,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
    set_line_color(renderer,color);

    fprintf(renderer->file,"\\rput");
    switch (alignment) {
    case ALIGN_LEFT:
	fprintf(renderer->file,"[l]");
	break;
    case ALIGN_CENTER:
	break;
    case ALIGN_RIGHT:
	fprintf(renderer->file,"[r]");
	break;
    }
    fprintf(renderer->file,"(%f,%f){\\scalebox{1 -1}{%s}}\n",pos->x, pos->y,text);
}

static void
draw_image(RendererPSTRICKS *renderer,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
    int img_width, img_height;
    int v;
    int                 x, y;
    unsigned char      *ptr;
    real ratio;
    guint8 *rgb_data;

    img_width = dia_image_width(image);
    img_height = dia_image_height(image);

    rgb_data = dia_image_rgb_data(image);
  
    ratio = height/width;

    fprintf(renderer->file, "\\pscustom{\\code{gsave\n");
    if (1) { /* Color output */
	fprintf(renderer->file, "/pix %i string def\n", img_width * 3);
	fprintf(renderer->file, "/grays %i string def\n", img_width);
	fprintf(renderer->file, "/npixls 0 def\n");
	fprintf(renderer->file, "/rgbindx 0 def\n");
	fprintf(renderer->file, "%f %f scale\n", POINTS_in_INCH, POINTS_in_INCH);
	fprintf(renderer->file, "%f %f translate\n", point->x, point->y);
	fprintf(renderer->file, "%f %f scale\n", width, height);
	fprintf(renderer->file, "%i %i 8\n", img_width, img_height);
	fprintf(renderer->file, "[%i 0 0 %i 0 0]\n", img_width, img_height);
	fprintf(renderer->file, "{currentfile pix readhexstring pop}\n");
	fprintf(renderer->file, "false 3 colorimage\n");
	/*    fprintf(renderer->file, "\n"); */
	ptr = rgb_data;
	for (y = 0; y < img_width; y++) {
	    for (x = 0; x < img_height; x++) {
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
	fprintf(renderer->file, "%f %f scale\n", POINTS_in_INCH, POINTS_in_INCH);
	fprintf(renderer->file, "%f %f translate\n", point->x, point->y);
	fprintf(renderer->file, "%f %f scale\n", width, height);
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

    g_free (rgb_data);
}

/* --- export filter interface --- */
static void
export_pstricks(DiagramData *data, const gchar *filename, 
                const gchar *diafilename, void* user_data)
{
    RendererPSTRICKS *renderer;

    renderer = new_pstricks_renderer(data, filename, diafilename);
    data_render(data, (Renderer *)renderer, NULL, NULL, NULL);
    destroy_pstricks_renderer(renderer);
}

static const gchar *extensions[] = { "tex", NULL };
DiaExportFilter pstricks_export_filter = {
    N_("TeX PSTricks macros"),
    extensions,
    export_pstricks
};
