/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dxf-export.c: dxf export filter for dia
 * Copyright (C) 2000 Steffen Macke
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

static const gchar *dia_version_string = "Dia-" VERSION;
#define IS_ODD(n) (n & 0x01)

static void
init_fonts(void)
{

}

/* --- dxf line attributes --- */
typedef struct _LineAttrdxf
{
    int         cap;
    int         join;
    char    	*style;
    real        width;
    Color       color;

} LineAttrdxf;

/* --- dxf File/Edge attributes --- */
typedef struct _FillEdgeAttrdxf
{

   int          fill_style;          /* Fill style */
   Color        fill_color;          /* Fill color */

   int          edgevis;             /* Edge visibility */
   int          cap;                 /* Edge cap */
   int          join;                /* Edge join */
   char         *style;               /* Edge style */
   real         width;               /* Edge width */ 
   Color        color;               /* Edge color */

} FillEdgeAttrdxf;


/* --- dxf Text attributes --- */
typedef struct _TextAttrdxf
{
   int          font_num;
   real         font_height;
   Color        color;

} TextAttrdxf;


/* --- the renderer --- */

typedef struct _Rendererdxf Rendererdxf;
struct _Rendererdxf {
    Renderer renderer;

    FILE *file;

    Font *font;

    real y0, y1; 

    LineAttrdxf  lcurrent, linfile;

    FillEdgeAttrdxf fcurrent, finfile;

    TextAttrdxf    tcurrent, tinfile;
    
    char *layername;

};


int get_version(void);
void register_objects(void);
void register_sheets(void);


static void begin_render(Rendererdxf *renderer, DiagramData *data);
static void end_render(Rendererdxf *renderer);
static void set_linewidth(Rendererdxf *renderer, real linewidth);
static void set_linecaps(Rendererdxf *renderer, LineCaps mode);
static void set_linejoin(Rendererdxf *renderer, LineJoin mode);
static void set_linestyle(Rendererdxf *renderer, LineStyle mode);
static void set_dashlength(Rendererdxf *renderer, real length);
static void set_fillstyle(Rendererdxf *renderer, FillStyle mode);
static void set_font(Rendererdxf *renderer, Font *font, real height);
static void draw_line(Rendererdxf *renderer, 
		      Point *start, Point *end, 
		      Color *line_colour);
static void draw_polyline(Rendererdxf *renderer, 
			  Point *points, int num_points, 
			  Color *line_colour);
static void draw_polygon(Rendererdxf *renderer, 
			 Point *points, int num_points, 
			 Color *line_colour);
static void fill_polygon(Rendererdxf *renderer, 
			 Point *points, int num_points, 
			 Color *line_colour);
static void draw_rect(Rendererdxf *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void fill_rect(Rendererdxf *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void draw_arc(Rendererdxf *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void fill_arc(Rendererdxf *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void draw_ellipse(Rendererdxf *renderer, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void fill_ellipse(Rendererdxf *renderer, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void draw_bezier(Rendererdxf *renderer, 
			BezPoint *points,
			int numpoints,
			Color *colour);
static void fill_bezier(Rendererdxf *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *colour);
static void draw_string(Rendererdxf *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *colour);
static void draw_image(Rendererdxf *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);

static RenderOps dxfRenderOps = {
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
init_attributes( Rendererdxf *renderer )
{
    renderer->lcurrent.style = renderer->fcurrent.style = "CONTINUOUS";
}

static void
write_line_attributes( Rendererdxf *renderer, Color *color )
{
}

static void
write_filledge_attributes( Rendererdxf *renderer, Color *fill_color,
                           Color *edge_color )
{
}

static void
write_text_attributes( Rendererdxf *renderer, Color *text_color)
{
}


static void
begin_render(Rendererdxf *renderer, DiagramData *data)
{
}

static void
end_render(Rendererdxf *renderer)
{
	fprintf(renderer->file, "0\nENDSEC\n0\nEOF\n");
    fclose(renderer->file);
}

static void
set_linewidth(Rendererdxf *renderer, real linewidth)
{
        /* update current line and edge width */
    renderer->lcurrent.width = renderer->fcurrent.width = linewidth;
}

static void
set_linecaps(Rendererdxf *renderer, LineCaps mode)
{
}

static void
set_linejoin(Rendererdxf *renderer, LineJoin mode)
{
}

static void
set_linestyle(Rendererdxf *renderer, LineStyle mode)
{
	char   *style;

    switch(mode)
    {
    case LINESTYLE_DASHED:
       style = "DASH";
       break;
    case LINESTYLE_DASH_DOT:
       style = "DASHDOT";
       break;
    case LINESTYLE_DASH_DOT_DOT:
       style = "DASHDOT";
       break;
    case LINESTYLE_DOTTED:
       style = "DOT";
       break;
    case LINESTYLE_SOLID:
    default:
       style = "CONTINUOUS";
       break;
    }
    renderer->lcurrent.style = renderer->fcurrent.style = style;
}

static void
set_dashlength(Rendererdxf *renderer, real length)
{ 
}

static void
set_fillstyle(Rendererdxf *renderer, FillStyle mode)
{
}

static void
set_font(Rendererdxf *renderer, Font *font, real height)
{
	renderer->tcurrent.font_height = height;
}

static void
draw_line(Rendererdxf *renderer, 
	  Point *start, Point *end, 
	  Color *line_colour)
{
	fprintf(renderer->file, "  0\nLINE\n");
    fprintf(renderer->file, "  8\n%s\n", renderer->layername);
    fprintf(renderer->file, "  6\n%s\n", renderer->lcurrent.style);
    fprintf(renderer->file, " 10\n%f\n", start->x);
    fprintf(renderer->file, " 20\n%f\n", (-1)*start->y);
    fprintf(renderer->file, " 11\n%f\n", end->x);
    fprintf(renderer->file, " 21\n%f\n", (-1)*end->y);
    fprintf(renderer->file, " 39\n%d\n", (int)(10*renderer->lcurrent.width)); /* Thickness */
}

static void
draw_polyline(Rendererdxf *renderer, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
	int i;
	for(i=0; i<num_points-1; i++) {
		draw_line(renderer, &points[i], &points[i+1], line_colour);
	}
}

static void
draw_polygon(Rendererdxf *renderer, 
	     Point *points, int num_points, 
	     Color *line_colour)
{
	int i;
	for(i=0; i<num_points-1; i++) {
		draw_line(renderer, &points[i], &points[i+1], line_colour);
	}
	draw_line(renderer, &points[num_points-1],&points[0], line_colour);
}

static void
fill_polygon(Rendererdxf *renderer, 
	     Point *points, int num_points, 
	     Color *colour)
{
	draw_polygon(renderer, points, num_points, colour);
}

static void
draw_rect(Rendererdxf *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
	Point ur_corner, ll_corner;
	
	ur_corner.x = lr_corner->x;
	ur_corner.y = ul_corner->y;
	ll_corner.x = ul_corner->x;
	ll_corner.y = lr_corner->y;
	draw_line(renderer, ul_corner, &ur_corner, colour);
	draw_line(renderer, &ur_corner, lr_corner, colour);
	draw_line(renderer, lr_corner, &ll_corner, colour);
	draw_line(renderer, &ll_corner, ul_corner, colour);
}

static void
fill_rect(Rendererdxf *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
		draw_rect(renderer, ul_corner, lr_corner, colour);
}

static void
draw_arc(Rendererdxf *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
 if(height != 0.0){
            fprintf(renderer->file, "  0\nELLIPSE\n");
            fprintf(renderer->file, "  8\n%s\n", renderer->layername);
            fprintf(renderer->file, "  6\n%s\n", renderer->lcurrent.style);
            fprintf(renderer->file, " 10\n%f\n", center->x);
            fprintf(renderer->file, " 20\n%f\n", (-1)*center->y);
            fprintf(renderer->file, " 11\n%f\n", width/2); /* Endpoint major axis relative to center X*/            
            fprintf(renderer->file, " 40\n%f\n", width/height); /*Ratio major/minor axis*/
            fprintf(renderer->file, " 39\n%d\n", (int)(10*renderer->lcurrent.width)); /* Thickness */
            fprintf(renderer->file, " 41\n%f\n", (angle1/360 ) * 2 * M_PI); /*Start Parameter full ellipse */
            fprintf(renderer->file, " 42\n%f\n", (angle2/360 ) * 2 * M_PI); /* End Parameter full ellipse */		
  }          
}

static void
fill_arc(Rendererdxf *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
	draw_arc(renderer, center, width, height, angle1, angle2, colour);
}

static void
draw_ellipse(Rendererdxf *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
	/* draw a circle instead of an ellipse, if it's one */
	if(width == height){
            fprintf(renderer->file, "  0\nCIRCLE\n");
            fprintf(renderer->file, "  8\n%s\n", renderer->layername);
            fprintf(renderer->file, "  6\n%s\n", renderer->lcurrent.style);
            fprintf(renderer->file, " 10\n%f\n", center->x);
            fprintf(renderer->file, " 20\n%f\n", (-1)*center->y);
            fprintf(renderer->file, " 40\n%f\n", height/2);
            fprintf(renderer->file, " 39\n%d\n", (int)(10*renderer->lcurrent.width)); /* Thickness */
	}
	else if(height != 0.0){
            fprintf(renderer->file, "  0\nELLIPSE\n");
            fprintf(renderer->file, "  8\n%s\n", renderer->layername);
            fprintf(renderer->file, "  6\n%s\n", renderer->lcurrent.style);
            fprintf(renderer->file, " 10\n%f\n", center->x);
            fprintf(renderer->file, " 20\n%f\n", (-1)*center->y);
            fprintf(renderer->file, " 11\n%f\n", width/2); /* Endpoint major axis relative to center X*/            
            fprintf(renderer->file, " 40\n%f\n", height/width); /*Ratio major/minor axis*/
            fprintf(renderer->file, " 39\n%d\n", (int)(10*renderer->lcurrent.width)); /* Thickness */
            fprintf(renderer->file, " 41\n%f\n", 0.0); /*Start Parameter full ellipse */
            fprintf(renderer->file, " 42\n%f\n", 2.0*3.14); /* End Parameter full ellipse */		
	}
}

static void
fill_ellipse(Rendererdxf *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
	draw_ellipse(renderer, center, width, height, colour);
}


static void
draw_bezier(Rendererdxf *renderer, 
	    BezPoint *points,
	    int numpoints,
	    Color *colour)
{
	
}


static void
fill_bezier(Rendererdxf *renderer, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *colour)
{
	draw_bezier(renderer, points, numpoints, colour);
}



static void
draw_string(Rendererdxf *renderer,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *colour)
{
	fprintf(renderer->file, "  0\nTEXT\n");
    fprintf(renderer->file, "  8\n%s\n", renderer->layername);
    fprintf(renderer->file, "  6\n%s\n", renderer->lcurrent.style);
    fprintf(renderer->file, " 10\n%f\n", pos->x);
    fprintf(renderer->file, " 20\n%f\n", (-1)*pos->y);
    fprintf(renderer->file, " 40\n%f\n", renderer->tcurrent.font_height); /* Text height */
    fprintf(renderer->file, " 50\n%f\n", 0.0); /* Text rotation */
    switch(alignment) {
    	case ALIGN_LEFT :
	    	fprintf(renderer->file, " 72\n%d\n", 0);
	    case ALIGN_RIGHT :
   	    	fprintf(renderer->file, " 72\n%d\n", 2);
	    case ALIGN_CENTER :
   	    	fprintf(renderer->file, " 72\n%d\n", 1);	    	    
    }    
    fprintf(renderer->file, "  7\n%s\n", "0"); /* Text style */
    fprintf(renderer->file, "  1\n%s\n", text);
    fprintf(renderer->file, " 39\n%d\n", (int)(10*renderer->lcurrent.width)); /* Thickness */
    fprintf(renderer->file, " 62\n%d\n", 1);
}


static void
draw_image(Rendererdxf *renderer,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
}

static void
export_dxf(DiagramData *data, const gchar *filename, 
           const gchar *diafilename, void* user_data)
{
    Rendererdxf *renderer;
    FILE *file;
    int i;
    Layer *layer;

    file = fopen(filename, "w");

    if (file == NULL) {
	  message_error(_("Couldn't open: '%s' for writing.\n"), filename);
	  return;
    }

    renderer = g_new(Rendererdxf, 1);
    renderer->renderer.ops = &dxfRenderOps;
    renderer->renderer.is_interactive = 0;
    renderer->renderer.interactive_ops = NULL;

    renderer->file = file;
    
    /* write layer description */
    fprintf(file,"O\nSECTION\n2\nTABLES\n");
    for (i=0; i<data->layers->len; i++) {
      layer = (Layer *) g_ptr_array_index(data->layers, i);
      fprintf(file,"0\nLAYER\n2\n%s\n",layer->name);
      if(layer->visible){
		fprintf(file,"62\n%d\n",i+1);
      }
      else {
      	fprintf(file,"62\n%d\n",(-1)*(i+1));
      }
  	}
    fprintf(file, "0\nENDTAB\n0\nENDSEC\n");    
    
    /* write graphics */
    fprintf(file,"0\nSECTION\n2\nENTITIES\n");
    
    init_attributes(renderer);

    (((Renderer *)renderer)->ops->begin_render)((Renderer *)renderer);
  
    for (i=0; i<data->layers->len; i++) {
      layer = (Layer *) g_ptr_array_index(data->layers, i);
	    renderer->layername = layer->name;
      layer_render(layer, (Renderer *)renderer, NULL, NULL, data, 0);
  	}
  
    (((Renderer *)renderer)->ops->end_render)((Renderer *)renderer);

    g_free(renderer);
}

static const gchar *extensions[] = { "dxf", NULL };
DiaExportFilter dxf_export_filter = {
    N_("Drawing Interchange File"),
    extensions,
    export_dxf
};
