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
#include "render_metapost.h"
#include "message.h"
#include "diagramdata.h"
#include "filter.h"
#include "dia_image.h"

#define POINTS_in_INCH 28.346

static void end_draw_op(MetapostRenderer *renderer);
static void draw_with_linestyle(MetapostRenderer *renderer);

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
metapost_renderer_finalize (GObject *object)
{ 
  MetapostRenderer *metapost_renderer = METAPOST_RENDERER (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
metapost_renderer_class_init (MetapostRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = metapost_renderer_finalize;

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
end_draw_op(MetapostRenderer *renderer)
{
    fprintf(renderer->file, "\n    withpen pencircle scaled %5.4fx", 
            (double)renderer->line_width);
    
    if (!color_equals(&renderer->color, &color_black))
        fprintf(renderer->file, "\n    withcolor (%5.4f, %5.4f, %5.4f)", 
                (double)renderer->color.red,
                (double)renderer->color.green,
                (double)renderer->color.blue);
    
    draw_with_linestyle(renderer);
    fprintf(renderer->file, ";\n");
}
	    
static void 
set_line_color(MetapostRenderer *renderer,Color *color)
{
    renderer->color = *color;
    fprintf(renderer->file, "%% set_line_color %f, %f, %f\n", 
            (double)color->red, (double)color->green, (double)color->blue);
}

static void 
set_fill_color(MetapostRenderer *renderer,Color *color)
{
    set_line_color(renderer,color);
}


static void
begin_render(DiaRenderer *self)
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

static void
set_linewidth(DiaRenderer *self, real linewidth)
{  /* 0 == hairline **/
    MetapostRenderer *renderer = METAPOST_RENDERER (self);
    fprintf(renderer->file, "%% set_linewidth %f\n", linewidth);
    renderer->line_width = linewidth;
}

static void
set_linecaps(DiaRenderer *self, LineCaps mode)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);

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
set_linejoin(DiaRenderer *self, LineJoin mode)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);

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
	/* noop; required at least for msvc */;
    }

    renderer->saved_line_join = mode;
}

static void 
set_linestyle(DiaRenderer *self, LineStyle mode)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);
    renderer->saved_line_style = mode;
}

static void
draw_with_linestyle(MetapostRenderer *renderer)
{
    real hole_width;

    switch(renderer->saved_line_style) {
    case LINESTYLE_SOLID:
	break;
    case LINESTYLE_DASHED:
	fprintf(renderer->file, "\n    dashed dashpattern (on %fx off %fx)", 
		renderer->dash_length,
		renderer->dash_length);
	break;
    case LINESTYLE_DASH_DOT:
	hole_width = (renderer->dash_length - renderer->dot_length) / 2.0;
	fprintf(renderer->file, "\n    dashed dashpattern (on %fx off %fx on %fx off %fx)",
		renderer->dash_length,
		hole_width,
		renderer->dot_length,
		hole_width );
	break;
    case LINESTYLE_DASH_DOT_DOT:
	hole_width = (renderer->dash_length - 2.0*renderer->dot_length) / 3.0;
	fprintf(renderer->file, "\n    dashed dashpattern (on %fx off %fx on %fx off %fx on %fx off %fx)",
		renderer->dash_length,
		hole_width,
		renderer->dot_length,
		hole_width,
		renderer->dot_length,
		hole_width );
	break;
    case LINESTYLE_DOTTED:
	fprintf(renderer->file, "\n    dashed dashpattern (on %fx off %fx)", 
		renderer->dot_length,
		renderer->dot_length);
	break;
    }
}

static void
set_dashlength(DiaRenderer *self, real length)
{  /* dot = 20% of len */
    MetapostRenderer *renderer = METAPOST_RENDERER (self);

    if (length<0.001)
	length = 0.001;
  
    renderer->dash_length = length;
    renderer->dot_length = length*0.05;
  
    set_linestyle(self, renderer->saved_line_style);
}

static void
set_fillstyle(DiaRenderer *self, FillStyle mode)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);

    switch(mode) {
    case FILLSTYLE_SOLID:
	break;
    default:
	message_error("metapost_renderer: Unsupported fill mode specified!\n");
    }
}

static void
set_font(DiaRenderer *self, DiaFont *font, real height)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);
    /*
     * TODO: implement size, figure out how to specify tex document font 
     */
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

    set_line_color(renderer,line_color);

    fprintf(renderer->file, "  draw (%fx,%fy)--(%fx,%fy)",
	    start->x, start->y, end->x, end->y);
    end_draw_op(renderer);
}

static void
draw_polyline(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_color)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);
    int i;
    
    set_line_color(renderer,line_color);
  
    fprintf(renderer->file, 
	    "  draw (%fx,%fy)",
	    points[0].x, points[0].y);

    for (i=1;i<num_points;i++) {
	fprintf(renderer->file, "--(%fx,%fy)",
		points[i].x, points[i].y);
    }
    end_draw_op(renderer);
}

static void
draw_polygon(DiaRenderer *self, 
	     Point *points, int num_points, 
	     Color *line_color)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);
    int i;

    set_line_color(renderer,line_color);
  
    fprintf(renderer->file, 
	    "  draw (%fx,%fy)",
	    points[0].x, points[0].y);

    for (i=1;i<num_points;i++) {
	fprintf(renderer->file, "--(%fx,%fy)",
		points[i].x, points[i].y);
    }
    fprintf(renderer->file,"--cycle");
    end_draw_op(renderer);
}

static void
fill_polygon(DiaRenderer *self, 
	     Point *points, int num_points, 
	     Color *line_color)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);
    int i;

    set_line_color(renderer,line_color);
    
    fprintf(renderer->file, "%% fill_polygon\n");
    fprintf(renderer->file, 
	    "  path p;\n"
	    "  p = (%fx,%fy)",
	    points[0].x, points[0].y);

    for (i=1;i<num_points;i++) {
	fprintf(renderer->file, "--(%fx,%fy)",
		points[i].x, points[i].y);
    }
    fprintf(renderer->file,"--cycle;\n");
    fprintf(renderer->file,"  fill p ");
    end_draw_op(renderer);
}

static void
draw_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);

    set_line_color(renderer,color);

    fprintf(renderer->file, 
	    "  draw (%fx,%fy)--(%fx,%fy)--(%fx,%fy)--(%fx,%fy)--cycle",
	    (double) ul_corner->x, (double) ul_corner->y,
	    (double) ul_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) ul_corner->y );
    end_draw_op(renderer);
}

static void
fill_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);

    fprintf(renderer->file, 
	    "  path p;\n"
	    "  p = (%fx,%fy)--(%fx,%fy)--(%fx,%fy)--(%fx,%fy)--cycle;\n",
	    (double) ul_corner->x, (double) ul_corner->y,
	    (double) ul_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) lr_corner->y,
	    (double) lr_corner->x, (double) ul_corner->y );
    fprintf(renderer->file,
	    "  fill p withcolor (%f,%f,%f);\n",
	    (double) color->red, (double) color->green, (double) color->blue);
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

    radius1=(double) width/2.0;
    radius2=(double) height/2.0;

    fprintf(renderer->file,"%%metapost_arc\n");
    fprintf(renderer->file, "%% %s = %f", "center->x", center->x);
    fprintf(renderer->file, "%% %s = %f", "center->y", center->y);
    fprintf(renderer->file, "%% %s = %f", "width", width);
    fprintf(renderer->file, "%% %s = %f", "height", height);
    fprintf(renderer->file, "%% %s = %f", "angle1", angle1);
    fprintf(renderer->file, "%% %s = %f", "angle2", angle2);
    
    
    angle1 = angle1*M_PI/180;
    angle2 = angle2*M_PI/180;
    angle3 = (double) (angle1+angle2)/2;
    if (angle1 > angle2)
        angle3 += M_PI;

    set_line_color(renderer,color);

    fprintf(renderer->file,
	    "  draw (%fx,%fy)..(%fx,%fy)..(%fx,%fy)",
	    cx + radius1*cos(angle1), cy - radius2*sin(angle1),
	    cx + radius1*cos(angle3), cy - radius2*sin(angle3),
	    cx + radius1*cos(angle2), cy - radius2*sin(angle2));
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
	     Color *color)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);

    set_line_color(renderer,color);
    
    fprintf(renderer->file, 
	    "  draw (%fx,%fy)..(%fx,%fy)..(%fx,%fy)..(%fx,%fy)..cycle",
	    (double) center->x+width/2.0, (double) center->y,
	    (double) center->x,           (double) center->y+height/2.0,
	    (double) center->x-width/2.0, (double) center->y,
	    (double) center->x,           (double) center->y-height/2.0);
    end_draw_op(renderer);
}

static void
fill_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);

    fprintf(renderer->file, 
	    "  path p;\n"
	    "  p = (%fx,%fy)..(%fx,%fy)..(%fx,%fy)..(%fx,%fy)..cycle;\n",
	    (double) center->x+width/2.0, (double) center->y,
	    (double) center->x,           (double) center->y+height/2.0,
	    (double) center->x-width/2.0, (double) center->y,
	    (double) center->x,           (double) center->y-height/2.0);

    fprintf(renderer->file,
	    "  fill p withcolor (%f,%f,%f);\n",
	    (double) color->red, (double) color->green, (double) color->blue);
}



static void
draw_bezier(DiaRenderer *self, 
	    BezPoint *points,
	    int numpoints, /* numpoints = 4+3*n, n=>0 */
	    Color *color)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);
    int i;

    set_line_color(renderer,color);

    if (points[0].type != BEZ_MOVE_TO)
	g_warning("first BezPoint must be a BEZ_MOVE_TO");

    fprintf(renderer->file, "  draw (%fx,%fy)",
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
	    fprintf(renderer->file, "..controls (%fx,%fy) and (%fx,%fy)\n    ..(%fx,%fy)",
		    (double) points[i].p1.x, (double) points[i].p1.y,
		    (double) points[i].p2.x, (double) points[i].p2.y,
		    (double) points[i].p3.x, (double) points[i].p3.y);
	    break;
	}
    end_draw_op(renderer);
}



static void
fill_bezier(DiaRenderer *self, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *color)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);
    int i;

    if (points[0].type != BEZ_MOVE_TO)
	g_warning("first BezPoint must be a BEZ_MOVE_TO");

    fprintf(renderer->file, "  path p;\n");
    fprintf(renderer->file, "  p = (%fx,%fy)",
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
	    fprintf(renderer->file, "..controls (%fx,%fy) and (%fx,%fy)\n    ..(%fx,%fy)",
		    (double) points[i].p1.x, (double) points[i].p1.y,
		    (double) points[i].p2.x, (double) points[i].p2.y,
		    (double) points[i].p3.x, (double) points[i].p3.y);
	    break;
	}
    fprintf(renderer->file, "\n    ..cycle;\n");

    fprintf(renderer->file,
	    "  fill p withcolor (%f,%f,%f);\n",
	    (double) color->red, (double) color->green, (double) color->blue);
}

static void
draw_string(DiaRenderer *self,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
    MetapostRenderer *renderer = METAPOST_RENDERER (self);

    set_line_color(renderer,color);

    switch (alignment) {
    case ALIGN_LEFT:
	fprintf(renderer->file,"  label.rt");
	break;
    case ALIGN_CENTER:
	fprintf(renderer->file,"  label");
	break;
    case ALIGN_RIGHT:
	fprintf(renderer->file,"  label.lft");
	break;
    }
    fprintf(renderer->file,
	    "(btex %s etex,(%fx,%fy))",
	    text, pos->x, pos->y);
    if (!color_equals(&renderer->color, &color_black))
        fprintf(renderer->file, "\n    withcolor (%5.4f, %5.4f, %5.4f)", 
                (double)renderer->color.red, 
                (double)renderer->color.green,
                (double)renderer->color.blue);

    fprintf(renderer->file,";\n");
}

static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
    /* images have a banding problem */
    int img_width, img_height, img_rowstride;
    int x, y;
    real xstep, ystep;
    guint8 *rgb_data;
    guint8 *mask_data;
    double ix, iy;

    MetapostRenderer *renderer = METAPOST_RENDERER (self);

    fprintf(renderer->file, "  %% draw_image: %s\n", dia_image_filename(image));
    

    img_width = dia_image_width(image);
    img_rowstride = dia_image_rowstride(image);
    img_height = dia_image_height(image);

    xstep = width/img_width;
    ystep = height/img_height;

    rgb_data = dia_image_rgb_data(image);
    mask_data = dia_image_mask_data(image);

    fprintf(renderer->file, "  pickup pensquare scaled %fx scaled %f;\n",
            (double) xstep,
            (double) (ystep/xstep));


    if (mask_data) {
        fprintf(renderer->file, "  %% have mask\n");
        for (y = 0, iy = point->y; y < img_height; y++, iy += ystep) {
            for (x = 0, ix = point->x; x < img_width; x++, ix += xstep) {
                int i = y*img_rowstride+x*3;
                int m = y*img_width+x;
                fprintf(renderer->file, "  draw (%fx, %fy) withcolor (%5.4f, %5.4f, %5.4f);\n",
                        ix, iy,
                        (255-(mask_data[m]*(255-rgb_data[i])/255)/255.0),
                        (255-(mask_data[m]*(255-rgb_data[i+1])/255))/255.0,
                        (255-(mask_data[m]*(255-rgb_data[i+2])/255))/255.0);
            }
            fprintf(renderer->file, "\n");
        }
    } else {
        for (y = 0, iy = point->y; y < img_height; y++, iy += ystep) {
            for (x = 0, ix = point->x; x < img_width; x++, ix += xstep) {
                int i = y*img_rowstride+x*3;
                fprintf(renderer->file, "  draw (%fx, %fy) withcolor (%5.4f, %5.4f, %5.4f);\n",
                        ix, iy,
                        (double)(rgb_data[i])/255.0,
                        (double)(rgb_data[i+1])/255.0,
                        (double)(rgb_data[i+2])/255.0);
            }
            fprintf(renderer->file, "\n");
        }
    }
    
    g_free(mask_data);
    g_free(rgb_data);
}

/* --- export filter interface --- */
static void
export_metapost(DiagramData *data, const gchar *filename, 
                const gchar *diafilename, void* user_data)
{
    MetapostRenderer *renderer;
    FILE *file;
    time_t time_now;
    double scale;
    Rectangle *extent;
    const char *name;

    Color initial_color;
 
    file = fopen(filename, "wb");

    if (file==NULL) {
	message_error(_("Can't open output file %s: %s\n"), file, strerror(errno));
        return;
    }

    renderer = g_object_new(METAPOST_TYPE_RENDERER, NULL);

    renderer->file = file;

    renderer->dash_length = 1.0;
    renderer->dot_length = 0.2;
    renderer->saved_line_style = LINESTYLE_SOLID;
  
    time_now  = time(NULL);
    extent = &data->extents;
  
    scale = POINTS_in_INCH * data->paper.scaling;
  
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
	    VERSION,
	    ctime(&time_now),
	    name);
 
    fprintf(renderer->file,"  %% picture(%f,%f)(%f,%f)\n",
	    extent->left * data->paper.scaling,
	    -extent->bottom * data->paper.scaling,
	    extent->right * data->paper.scaling,
	    -extent->top * data->paper.scaling
	    );
    fprintf(renderer->file,"  x = %fcm; y = %fcm;\n\n",
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

static const gchar *extensions[] = { "mp", NULL };
DiaExportFilter metapost_export_filter = {
    N_("TeX Metapost macros"),
    extensions,
    export_metapost
};
