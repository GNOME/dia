/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dxf-export.c: dxf export filter for dia
 * Copyright (C) 2000,2004 Steffen Macke
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
#include <errno.h>

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "diarenderer.h"
#include "filter.h"

#define DXF_TYPE_RENDERER           (dxf_renderer_get_type ())
#define DXF_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DXF_TYPE_RENDERER, DxfRenderer))
#define DXF_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DXF_TYPE_RENDERER, DxfRendererClass))
#define DXF_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DXF_TYPE_RENDERER))
#define DXF_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DXF_TYPE_RENDERER, DxfRendererClass))

GType dxf_renderer_get_type (void) G_GNUC_CONST;

typedef struct _DxfRenderer DxfRenderer;
typedef struct _DxfRendererClass DxfRendererClass;

struct _DxfRendererClass
{
  DiaRendererClass parent_class;
};

#define IS_ODD(n) (n & 0x01)

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

struct _DxfRenderer
{
    DiaRenderer parent_instance;

    FILE *file;

    DiaFont *font;

    real y0, y1; 

    LineAttrdxf  lcurrent, linfile;

    FillEdgeAttrdxf fcurrent, finfile;

    TextAttrdxf    tcurrent, tinfile;
    
    char *layername;

};


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
		      Color *line_colour);
static void fill_rect (DiaRenderer *renderer,
                       Point *ul_corner, Point *lr_corner,
                       Color *color);
static void fill_polygon (DiaRenderer *renderer,
                          Point *points, int num_points,
                          Color *color);
static void draw_arc(DiaRenderer *self, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void fill_arc(DiaRenderer *self, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *colour);
static void draw_ellipse(DiaRenderer *self, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void fill_ellipse(DiaRenderer *self, 
			 Point *center,
			 real width, real height,
			 Color *colour);
static void draw_string(DiaRenderer *self,
			const char *text,
			Point *pos, Alignment alignment,
			Color *colour);
static void draw_image(DiaRenderer *self,
		       Point *point,
		       real width, real height,
		       DiaImage image);

static void dxf_renderer_class_init (DxfRendererClass *klass);

static gpointer parent_class = NULL;

GType
dxf_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (DxfRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) dxf_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (DxfRenderer),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "DxfRenderer",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
dxf_renderer_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
dxf_renderer_class_init (DxfRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = dxf_renderer_finalize;

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
  renderer_class->fill_rect = fill_rect;
  renderer_class->fill_polygon = fill_polygon;

  renderer_class->draw_arc = draw_arc;
  renderer_class->fill_arc = fill_arc;

  renderer_class->draw_ellipse = draw_ellipse;
  renderer_class->fill_ellipse = fill_ellipse;

  renderer_class->draw_string = draw_string;

  renderer_class->draw_image = draw_image;
}


static void
init_attributes( DxfRenderer *renderer )
{
    renderer->lcurrent.style = renderer->fcurrent.style = "CONTINUOUS";
}

static void
begin_render(DiaRenderer *self)
{
}

static void
end_render(DiaRenderer *self)
{
    DxfRenderer *renderer = DXF_RENDERER(self);

    fprintf(renderer->file, "0\nENDSEC\n0\nEOF\n");
    fclose(renderer->file);
}

static void
set_linewidth(DiaRenderer *self, real linewidth)
{
    DxfRenderer *renderer = DXF_RENDERER(self);

        /* update current line and edge width */
    renderer->lcurrent.width = renderer->fcurrent.width = linewidth;
}

static void
set_linecaps(DiaRenderer *self, LineCaps mode)
{
}

static void
set_linejoin(DiaRenderer *self, LineJoin mode)
{
}

static void
set_linestyle(DiaRenderer *self, LineStyle mode)
{
    DxfRenderer *renderer = DXF_RENDERER(self);
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
set_dashlength(DiaRenderer *self, real length)
{ 
}

static void
set_fillstyle(DiaRenderer *self, FillStyle mode)
{
}

static void
set_font(DiaRenderer *self, DiaFont *font, real height)
{
    DxfRenderer *renderer = DXF_RENDERER(self);

    renderer->tcurrent.font_height = height;
}

static void
draw_line(DiaRenderer *self, 
	  Point *start, Point *end, 
	  Color *line_colour)
{
    DxfRenderer *renderer = DXF_RENDERER(self);

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
fill_rect (DiaRenderer *renderer,
           Point *ul_corner, Point *lr_corner,
           Color *color)
{
  /* draw a transparent rect. i.e. not implemented */
}

static void 
fill_polygon (DiaRenderer *renderer,
              Point *points, int num_points,
              Color *color)
{
  /* not implemented, but no complaints by base class either */
}

static void
draw_arc(DiaRenderer *self, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    DxfRenderer *renderer = DXF_RENDERER(self);

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
fill_arc(DiaRenderer *self, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    draw_arc(self, center, width, height, angle1, angle2, colour);
}

static void
draw_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
    DxfRenderer *renderer = DXF_RENDERER(self);

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
fill_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
    draw_ellipse(self, center, width, height, colour);
}


static void
draw_string(DiaRenderer *self,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *colour)
{
    DxfRenderer *renderer = DXF_RENDERER(self);

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
        break;
    case ALIGN_RIGHT :
   	fprintf(renderer->file, " 72\n%d\n", 2);
        break;
    case ALIGN_CENTER :
    default:
   	fprintf(renderer->file, " 72\n%d\n", 1);
        break;
    }    
    fprintf(renderer->file, "  7\n%s\n", "0"); /* Text style */
    fprintf(renderer->file, "  1\n%s\n", text);
    fprintf(renderer->file, " 39\n%d\n", (int)(10*renderer->lcurrent.width)); /* Thickness */
    fprintf(renderer->file, " 62\n%d\n", 1);
}

static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
}

static void
export_dxf(DiagramData *data, const gchar *filename, 
           const gchar *diafilename, void* user_data)
{
    DxfRenderer *renderer;
    FILE *file;
    int i;
    Layer *layer;

    file = fopen(filename, "w");

    if (file == NULL) {
	message_error(_("Can't open output file %s: %s\n"), 
		      dia_message_filename(filename), strerror(errno));
	return;
    }

    renderer = g_object_new(DXF_TYPE_RENDERER, NULL);

    renderer->file = file;
    
    /* write layer description */
    fprintf(file,"0\nSECTION\n2\nTABLES\n");
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

    DIA_RENDERER_GET_CLASS(renderer)->begin_render(DIA_RENDERER(renderer));
  
    for (i=0; i<data->layers->len; i++) {
        layer = (Layer *) g_ptr_array_index(data->layers, i);
	    renderer->layername = layer->name;
        layer_render(layer, DIA_RENDERER(renderer), NULL, NULL, data, 0);
    }
  
    DIA_RENDERER_GET_CLASS(renderer)->end_render(DIA_RENDERER(renderer));

    g_object_unref(renderer);
}

static const gchar *extensions[] = { "dxf", NULL };
DiaExportFilter dxf_export_filter = {
    N_("Drawing Interchange File"),
    extensions,
    export_dxf
};
