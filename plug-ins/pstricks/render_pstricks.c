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
6. Verify the Pango stuff isn't spitting out things TeX is unable to read
   
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
#include <errno.h>

#include "intl.h"
#include "render_pstricks.h"
#include "message.h"
#include "diagramdata.h"
#include "dia_image.h"
#include "filter.h"

#define POINTS_in_INCH 28.346

static void begin_render(DiaRenderer *self);
static void end_render(DiaRenderer *self);
static void set_linewidth(DiaRenderer *self, real linewidth);
static void set_linecaps(DiaRenderer *self, LineCaps mode);
static void set_linejoin(DiaRenderer *self, LineJoin mode);
static void set_linestyle(DiaRenderer *self, LineStyle mode);
static void set_dashlength(DiaRenderer *self, real length);
static void set_fillstyle(DiaRenderer *self, FillStyle mode);
static void set_font(DiaRenderer *self, DiaFont *font, real height);
static void draw_line(DiaRenderer *self, 
		      Point *start, Point *end, 
		      Color *line_color);
static void draw_polyline(DiaRenderer *self, 
			  Point *points, int num_points, 
			  Color *line_color);
static void draw_polygon(DiaRenderer *self, 
			 Point *points, int num_points, 
			 Color *line_color);
static void fill_polygon(DiaRenderer *self, 
			 Point *points, int num_points, 
			 Color *line_color);
static void draw_rect(DiaRenderer *self, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void fill_rect(DiaRenderer *self, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
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
			 Color *color);
static void fill_ellipse(DiaRenderer *self, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void draw_bezier(DiaRenderer *self, 
			BezPoint *points,
			int numpoints,
			Color *color);
static void fill_bezier(DiaRenderer *self, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *color);
static void draw_string(DiaRenderer *self,
			const char *text,
			Point *pos, Alignment alignment,
			Color *color);
static void draw_image(DiaRenderer *self,
		       Point *point,
		       real width, real height,
		       DiaImage image);

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
pstricks_renderer_finalize (GObject *object)
{
  PstricksRenderer *pstricks_renderer = PSTRICKS_RENDERER (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
pstricks_renderer_class_init (PstricksRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = pstricks_renderer_finalize;

  renderer_class->begin_render = begin_render;
  renderer_class->end_render = end_render;

  renderer_class->set_linewidth = set_linewidth;
  renderer_class->set_linecaps = set_linecaps;
  renderer_class->set_linejoin = set_linejoin;
  renderer_class->set_linestyle = set_linestyle;
  renderer_class->set_dashlength = set_dashlength;
  renderer_class->set_fillstyle = set_fillstyle;
  renderer_class->set_font = set_font;
  
  renderer_class->draw_line = draw_line;
  renderer_class->draw_polyline = draw_polyline;
  
  renderer_class->draw_polygon = draw_polygon;
  renderer_class->fill_polygon = fill_polygon;

  renderer_class->draw_rect = draw_rect;
  renderer_class->fill_rect = fill_rect;

  renderer_class->draw_arc = draw_arc;
  renderer_class->fill_arc = fill_arc;

  renderer_class->draw_ellipse = draw_ellipse;
  renderer_class->fill_ellipse = fill_ellipse;

  renderer_class->draw_bezier = draw_bezier;
  renderer_class->fill_bezier = fill_bezier;

  renderer_class->draw_string = draw_string;

  renderer_class->draw_image = draw_image;
}


static void 
set_line_color(PstricksRenderer *renderer,Color *color)
{
    fprintf(renderer->file, "\\newrgbcolor{dialinecolor}{%f %f %f}\n",
	    (double) color->red, (double) color->green, (double) color->blue);
    fprintf(renderer->file,"\\psset{linecolor=dialinecolor}\n");
}

static void 
set_fill_color(PstricksRenderer *renderer,Color *color)
{
    fprintf(renderer->file, "\\newrgbcolor{diafillcolor}{%f %f %f}\n",
	    (double) color->red, (double) color->green, (double) color->blue);
    fprintf(renderer->file,"\\psset{fillcolor=diafillcolor}\n");
}

static void
begin_render(DiaRenderer *self)
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

    fprintf(renderer->file, "\\psset{linewidth=%f}\n", (double) linewidth);
}

static void
set_linecaps(DiaRenderer *self, LineCaps mode)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
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
set_linejoin(DiaRenderer *self, LineJoin mode)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
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
set_linestyle(DiaRenderer *self, LineStyle mode)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
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
set_dashlength(DiaRenderer *self, real length)
{  /* dot = 20% of len */
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);

    if (length<0.001)
	length = 0.001;
  
    renderer->dash_length = length;
    renderer->dot_length = length*0.2;
  
    set_linestyle(self, renderer->saved_line_style);
}

static void
set_fillstyle(DiaRenderer *self, FillStyle mode)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);

    switch(mode) {
    case FILLSTYLE_SOLID:
	break;
    default:
	message_error("pstricks_renderer: Unsupported fill mode specified!\n");
    }
}

static void
set_font(DiaRenderer *self, DiaFont *font, real height)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);

    fprintf(renderer->file, "\\setfont{%s}{%f}\n",
            dia_font_get_psfontname(font), (double)height);
}

static void
draw_line(DiaRenderer *self, 
	  Point *start, Point *end, 
	  Color *line_color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);

    set_line_color(renderer,line_color);

    fprintf(renderer->file, "\\psline(%f,%f)(%f,%f)\n",
	    start->x, start->y, end->x, end->y);
}

static void
draw_polyline(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
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
draw_polygon(DiaRenderer *self, 
	     Point *points, int num_points, 
	     Color *line_color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
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
fill_polygon(DiaRenderer *self, 
	     Point *points, int num_points, 
	     Color *line_color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
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
draw_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);

    set_line_color(renderer,color);
  
    fprintf(renderer->file, "\\pspolygon(%f,%f)(%f,%f)(%f,%f)(%f,%f)\n",
	    (double) ul_corner->x, (double) ul_corner->y,
	    (double) ul_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) ul_corner->y );

}

static void
fill_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);

    set_line_color(renderer,color);

    fprintf(renderer->file, 
	    "\\pspolygon*(%f,%f)(%f,%f)(%f,%f)(%f,%f)\n",
	    (double) ul_corner->x, (double) ul_corner->y,
	    (double) ul_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) ul_corner->y );
}

static void
pstricks_arc(PstricksRenderer *renderer, 
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
draw_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);

    set_line_color(renderer,color);

    fprintf(renderer->file, "\\psellipse(%f,%f)(%f,%f)\n",
	    (double) center->x, (double) center->y,
	    (double) width/2.0, (double) height/2.0 );
}

static void
fill_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);

    set_line_color(renderer,color);

    fprintf(renderer->file, "\\psellipse*(%f,%f)(%f,%f)\n",
	    (double) center->x, (double) center->y,
	    (double) width/2.0, (double) height/2.0 );
}



static void
draw_bezier(DiaRenderer *self, 
	    BezPoint *points,
	    int numpoints, /* numpoints = 4+3*n, n=>0 */
	    Color *color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
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
fill_bezier(DiaRenderer *self, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
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

/* Do I really want to do this?  What if the text is intended as 
 * TeX text?
 */
static gchar *
tex_escape_string(gchar *src)
{
    GString *dest = g_string_sized_new(g_utf8_strlen(src, -1));
    gchar *next;

    if (!g_utf8_validate(src, -1, NULL)) {
	message_error(_("Not valid UTF8"));
	return g_strdup(src);
    }

    next = src;
    while ((next = g_utf8_next_char(next)) != NULL) {
	switch (*next) {
	case '%': g_string_append(dest, "\\%"); break;
	case '#': g_string_append(dest, "\\#"); break;
	case '$': g_string_append(dest, "\\$"); break;
	case '&': g_string_append(dest, "\\&"); break;
	case '~': g_string_append(dest, "\\~{}"); break;
	case '_': g_string_append(dest, "\\_"); break;
	case '^': g_string_append(dest, "\\^{}"); break;
	case '\\': g_string_append(dest, "\\\\"); break;
	case '{': g_string_append(dest, "\\}"); break;
	case '}': g_string_append(dest, "\\}"); break;
	case '[': g_string_append(dest, "\\ensuremath{\\left[}"); break;
	case ']': g_string_append(dest, "\\ensuremath{\\right]}"); break;
	default: g_string_append(dest, next);
	}
    }

    next = dest->str;
    g_string_free(dest, FALSE);
    return next;
}

static void
draw_string(DiaRenderer *self,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
    gchar *escaped = tex_escape_string(text);

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
    fprintf(renderer->file,"(%f,%f){\\scalebox{1 -1}{%s}}\n",pos->x, pos->y,escaped);
    g_free(escaped);
}

static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
    PstricksRenderer *renderer = PSTRICKS_RENDERER(self);
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
    PstricksRenderer *renderer;
    FILE *file;
    time_t time_now;
    double scale;
    Rectangle *extent;
    const char *name;

    Color initial_color;
 
    file = fopen(filename, "wb");

    if (file==NULL) {
	message_error(_("Can't open output file %s: %s\n"), filename, strerror(errno));
    }

    renderer = g_object_new(PSTRICKS_TYPE_RENDERER, NULL);

    renderer->pagenum = 1;
    renderer->file = file;

    renderer->dash_length = 1.0;
    renderer->dot_length = 0.2;
    renderer->saved_line_style = LINESTYLE_SOLID;
  
    time_now  = time(NULL);
    extent = &data->extents;
  
    scale = POINTS_in_INCH * data->paper.scaling;
  
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

    data_render(data, DIA_RENDERER(renderer), NULL, NULL, NULL);

    g_object_unref(renderer);
}

static const gchar *extensions[] = { "tex", NULL };
DiaExportFilter pstricks_export_filter = {
    N_("TeX PSTricks macros"),
    extensions,
    export_pstricks
};
