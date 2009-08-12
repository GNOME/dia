/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * vdx-export.c: Visio XML export filter for dia
 * Copyright (C) 2006-2007 Ian Redfern
 * based on the xfig filter code
 * Copyright (C) 2001 Lars Clausen
 * based on the dxf filter code
 * Copyright (C) 2000 James Henstridge, Steffen Macke
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
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include <glib/gstdio.h>

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "diarenderer.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "dia_image.h"
#include "group.h"

#include "vdx.h"
#include "visio-types.h"

/* Following code taken from xfig-export.c */

#define VDX_TYPE_RENDERER           (vdx_renderer_get_type ())
#define VDX_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), VDX_TYPE_RENDERER, VDXRenderer))
#define VDX_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), VDX_TYPE_RENDERER, VDXRendererClass))
#define VDX_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VDX_TYPE_RENDERER))
#define VDX_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), VDX_TYPE_RENDERER, VDXRendererClass))

GType vdx_renderer_get_type (void) G_GNUC_CONST;

typedef struct _VDXRenderer VDXRenderer;
typedef struct _VDXRendererClass VDXRendererClass;

struct _VDXRendererClass
{
    DiaRendererClass parent_class;
};

struct _VDXRenderer
{
    DiaRenderer parent_instance;

    FILE *file;

    int depth;

    real linewidth;
    LineCaps capsmode;
    LineJoin joinmode;
    LineStyle stylemode;
    real dashlength;
    FillStyle fillmode;
    DiaFont *font;
    real fontheight;

    /* Additions for VDX */

    gboolean first_pass;        /* When we make table of colours and fonts */
    GArray *Colors;             /* Table of colours */
    GArray *Fonts;              /* Table of fonts */
    unsigned int shapeid;       /* Shape counter */
    unsigned int version;       /* Visio version */
    unsigned int xml_depth;     /* Pretty-printer */
};


static void begin_render(DiaRenderer *self);
static void end_render(DiaRenderer *renderer);
static void set_linewidth(DiaRenderer *self, real linewidth);
static void set_linecaps(DiaRenderer *self, LineCaps mode);
static void set_linejoin(DiaRenderer *self, LineJoin mode);
static void set_linestyle(DiaRenderer *self, LineStyle mode);
static void set_dashlength(DiaRenderer *self, real length);
static void set_fillstyle(DiaRenderer *self, FillStyle mode);
static void set_font(DiaRenderer *self, DiaFont *font, real height);
static void draw_line(DiaRenderer *self, 
		      Point *start, Point *end, 
		      Color *color);
static void draw_polyline(DiaRenderer *self, 
			  Point *points, int num_points, 
			  Color *color);
static void draw_polygon(DiaRenderer *self, 
			 Point *points, int num_points, 
			 Color *color);
static void fill_polygon(DiaRenderer *self, 
			 Point *points, int num_points, 
			 Color *color);
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
static void draw_string(DiaRenderer *self,
			const char *text,
			Point *pos, Alignment alignment,
			Color *color);
static void draw_image(DiaRenderer *self,
		       Point *point,
		       real width, real height,
		       DiaImage *image);

static void vdx_renderer_class_init (VDXRendererClass *klass);

static void
export_vdx(DiagramData *data, const gchar *filename, 
           const gchar *diafilename, void* user_data);

static int
vdxCheckColor(VDXRenderer *renderer, Color *color); 

static gpointer parent_class = NULL;


/** Renderer type handler
 * @returns renderer type
 */

GType
vdx_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (VDXRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) vdx_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (VDXRenderer),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "VDXRenderer",
                                            &object_info, 0);
    }
  
  return object_type;
}

/** Finalise a renderer
 * @param object a renderer
 */

static void
vdx_renderer_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/** Class constructor for renderer
 * @param klass a renderer
 */

static void
vdx_renderer_class_init (VDXRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = vdx_renderer_finalize;

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

  /* Until we have NURBS, let Dia use lines */
  /* renderer_class->draw_bezier = draw_bezier; */
  /* renderer_class->fill_bezier = fill_bezier; */
  /* renderer_class->draw_bezier_with_arrows = draw_bezier_with_arrows; */

  renderer_class->draw_string = draw_string;

  renderer_class->draw_image = draw_image;

  /* Believe these are never used or are unnecessary */
  /* renderer_class->draw_line_with_arrows = draw_line_with_arrows; */
  /* renderer_class->draw_polyline_with_arrows = draw_polyline_with_arrows; */
  /* renderer_class->draw_arc_with_arrows = draw_arc_with_arrows; */
  /* renderer_class->draw_object = draw_object; */

}

/** Initialises VDXrenderer
 * @param self a renderer
 */

static void 
begin_render(DiaRenderer *self) 
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    Color c;
    
    renderer->depth = 0;
    
    renderer->linewidth = 0;
    renderer->capsmode = 0;
    renderer->joinmode = 0;
    renderer->stylemode = 0;
    renderer->dashlength = 0;
    renderer->fillmode = 0;
    renderer->font = NULL;
    renderer->fontheight = 1;

    /* Specific to VDX */

    renderer->Colors = g_array_new(FALSE, TRUE, sizeof (Color));
    renderer->Fonts = g_array_new(FALSE, TRUE, sizeof (char *));
    renderer->shapeid = 0;
    /* renderer->version = 0; */

    /* Black and white are 0 and 1 respectively */
    c.red = 0.0; c.green = 0.0; c.blue = 0.0; c.alpha = 1.0;
    vdxCheckColor(renderer, &c);
    c.red = 1.0; c.green = 1.0; c.blue = 1.0; c.alpha = 1.0;
    vdxCheckColor(renderer, &c);
}

/** Destructor for renderer
 * @param self a renderer
 */

static void 
end_render(DiaRenderer *self) 
{
    VDXRenderer *renderer = VDX_RENDERER(self);

    /* Specific to VDX */
    g_array_free(renderer->Colors, TRUE);
    g_array_free(renderer->Fonts, TRUE);
}

/** Convert a Dia point to a Visio one
 * @param p a Dia-space point
 * @returns a Visio-space point
 */

static Point
visio_point(Point p)
{
    Point q;
    q.x = p.x/vdx_Point_Scale;
    q.y = (p.y - vdx_Y_Offset)/vdx_Y_Flip/vdx_Point_Scale;
    return q;
}

/** Convert a Dia absolute length to a Visio one
 * @param length a length
 * @returns length in Visio space
 */
  
static double 
visio_length(double length)
{
    return length/vdx_Point_Scale;
}

/** Set line width
 * @param self a renderer
 * @param linewidth new line width
 */

static void 
set_linewidth(DiaRenderer *self, real linewidth) 
{
  VDXRenderer *renderer = VDX_RENDERER(self);

  renderer->linewidth = linewidth;
}

/** Set line caps
 * @param self a renderer
 * @param mode new line caps
 */

static void 
set_linecaps(DiaRenderer *self, LineCaps mode) 
{
  VDXRenderer *renderer = VDX_RENDERER(self);

  renderer->capsmode = mode;
}

/** Set line join
 * @param self a renderer
 * @param mode new line join
 */

static void 
set_linejoin(DiaRenderer *self, LineJoin mode) 
{
  VDXRenderer *renderer = VDX_RENDERER(self);

  renderer->joinmode = mode;
}

/** Set line style
 * @param self a renderer
 * @param mode new line style
 */

static void 
set_linestyle(DiaRenderer *self, LineStyle mode) 
{
  VDXRenderer *renderer = VDX_RENDERER(self);

  renderer->stylemode = mode;
}

/** Set dash length
 * @param self a renderer
 * @param mode new dash length
 */

static void 
set_dashlength(DiaRenderer *self, real length) 
{
  VDXRenderer *renderer = VDX_RENDERER(self);

  renderer->dashlength = length;
}

/** Set fill style
 * @param self a renderer
 * @param mode new file style
 */

static void 
set_fillstyle(DiaRenderer *self, FillStyle mode) 
{
  VDXRenderer *renderer = VDX_RENDERER(self);

  renderer->fillmode = mode;
}

/** Set font
 * @param self a renderer
 * @param font new font
 * @param height new font height
 */

static void 
set_font(DiaRenderer *self, DiaFont *font, real height) 
{
  VDXRenderer *renderer = VDX_RENDERER(self);

  renderer->font = font;
  renderer->fontheight = height;
}

/** Get colour number from colour table
 * @param renderer a renderer
 * @param color the colour
 * @returns a colour index (black=0)
 */

static int
vdxCheckColor(VDXRenderer *renderer, Color *color) 
{
    int i;

    Color cmp_color;
    for (i = 0; i < renderer->Colors->len; i++) 
    {
        cmp_color = g_array_index(renderer->Colors, Color, i);
        if (color_equals(color, &cmp_color)) return i;
    }
    /* Grow table */
    g_array_append_val(renderer->Colors, *color);
    return renderer->Colors->len;
}

/** Get font number from font table
 * @param renderer a renderer
 * @returns a font index (from 0)
 */

static int
vdxCheckFont(VDXRenderer *renderer) 
{
    int i;

    const char *cmp_font;
    const char *font = dia_font_get_legacy_name(renderer->font);
    for (i = 0; i < renderer->Fonts->len; i++) 
    {
        cmp_font = g_array_index(renderer->Fonts, char *, i);
        if (!strcmp(cmp_font, font)) return i;
    }
    /* Grow table */
    g_array_append_val(renderer->Fonts, font);
    return renderer->Fonts->len;
}

/** Create a Visio line style object
 * @param self a VDXRenderer
 * @param color a colour
 * @param Line a Line object
 * @param start_arrow optional start arrow
 * @param end_arrow optional end arrow
 * @todo join, caps, dashlength
 */

static void 
create_Line(VDXRenderer *renderer, Color *color, struct vdx_Line *Line,
            Arrow *start_arrow, Arrow *end_arrow) 
{
    /* A Line (colour etc) */
    memset(Line, 0, sizeof(*Line));
    Line->type = vdx_types_Line;
    switch (renderer->stylemode)
    {
    case LINESTYLE_DASHED:
        Line->LinePattern = 2;
        break;
    case LINESTYLE_DOTTED:
        Line->LinePattern = 3;
        break;
    case LINESTYLE_DASH_DOT:
        Line->LinePattern = 4;
        break;
    case LINESTYLE_DASH_DOT_DOT:
        Line->LinePattern = 5;
        break;
    default:
    case LINESTYLE_SOLID:
        Line->LinePattern = 1;
        break;
    }
    Line->LineColor = *color;
    Line->LineWeight = renderer->linewidth / vdx_Line_Scale;
    if (start_arrow || end_arrow) 
    {
        g_debug("create_Line (ARROWS)");
    }
}


/** Create a Visio fill style object
 * @param self a VDXRenderer
 * @param color a colour
 * @todo fillstyle
 */

static void 
create_Fill(VDXRenderer *renderer, Color *color, struct vdx_Fill *Fill)
{
    /* A Fill (colour etc) */
    memset(Fill, 0, sizeof(*Fill));
    Fill->type = vdx_types_Fill;
    Fill->FillForegnd = *color;
    Fill->FillPattern = 1;      /* Solid fill */
}


/** Render a Dia line
 * @param self a renderer
 * @param start start of line
 * @param end end of line
 * @param color line colour
 */

static void 
draw_line(DiaRenderer *self, Point *start, Point *end, Color *color) 
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    Point a, b;
    struct vdx_Shape Shape;
    struct vdx_XForm XForm;
    struct vdx_XForm1D XForm1D;
    struct vdx_Geom Geom;
    struct vdx_MoveTo MoveTo;
    struct vdx_LineTo LineTo;
    struct vdx_Line Line;
    char NameU[VDX_NAMEU_LEN];

    /* First time through, just construct the colour table */
    if (renderer->first_pass) 
    {
        vdxCheckColor(renderer, color);
        return;
    }
    
    g_debug("draw_line((%f,%f), (%f,%f))", start->x, start->y, end->x, end->y);
    
    /* Setup the standard shape object */
    memset(&Shape, 0, sizeof(Shape));
    Shape.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "Line.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* An XForm */
    memset(&XForm, 0, sizeof(XForm));
    XForm.type = vdx_types_XForm;
    a = visio_point(*start);
    b = visio_point(*end);
    XForm.PinX = a.x;           /* Start */
    XForm.PinY = a.y;
    XForm.Width = fabs(b.x - a.x); /* Must be non-negative */
    XForm.Height = fabs(b.y - a.y); /* Must be non-negative */
    XForm.LocPinX = 0.0;
    XForm.LocPinY = 0.0;
    XForm.Angle = 0.0;

    /* Lines must have an XForm1D as well */
    memset(&XForm1D, 0, sizeof(XForm1D));
    XForm1D.type = vdx_types_XForm1D;
    XForm1D.BeginX = a.x;
    XForm1D.BeginY = a.y;
    XForm1D.EndX = b.x;
    XForm1D.EndY = b.y;

    /* Standard Geom object */
    memset(&Geom, 0, sizeof(Geom));
    Geom.NoFill = 1;
    Geom.type = vdx_types_Geom;

    /* Two children - MoveTo(start) and LineTo(end) */
    memset(&MoveTo, 0, sizeof(MoveTo));
    MoveTo.type = vdx_types_MoveTo;
    MoveTo.IX = 1;
    MoveTo.X = 0;
    MoveTo.Y = 0;

    memset(&LineTo, 0, sizeof(LineTo));
    LineTo.type = vdx_types_LineTo;
    LineTo.IX = 2;
    LineTo.X = b.x-a.x; 
    LineTo.Y = b.y-a.y;

    /* A Line (colour etc) */
    create_Line(renderer, color, &Line, 0, 0);

    /* Setup children */
    Geom.children = g_slist_append(Geom.children, &MoveTo);
    Geom.children = g_slist_append(Geom.children, &LineTo);

    Shape.children = g_slist_append(Shape.children, &XForm);
    Shape.children = g_slist_append(Shape.children, &XForm1D);
    Shape.children = g_slist_append(Shape.children, &Line);
    Shape.children = g_slist_append(Shape.children, &Geom);

    /* Write out XML */
    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    /* Free up list entries */
    g_slist_free(Geom.children);
    g_slist_free(Shape.children);
}


/** Render a Dia polyline
 * @param self a renderer
 * @param points the points
 * @param num_points how many points
 * @param color line colour
 */

static void draw_polyline(DiaRenderer *self, Point *points, int num_points, 
			  Color *color)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    Point a, b;
    struct vdx_Shape Shape;
    struct vdx_XForm XForm;
    struct vdx_Geom Geom;
    struct vdx_MoveTo MoveTo;
    struct vdx_LineTo* LineTo;
    struct vdx_Line Line;
    char NameU[VDX_NAMEU_LEN];
    unsigned int i;
    double minX, minY, maxX, maxY;

    /* First time through, just construct the colour table */
    if (renderer->first_pass) 
    {
        vdxCheckColor(renderer, color);
        return;
    }
    
    g_debug("draw_polyline(%d)", num_points);
    
    /* Setup the standard shape object */
    memset(&Shape, 0, sizeof(Shape));
    Shape.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "PolyLine.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* An XForm */
    memset(&XForm, 0, sizeof(XForm));
    XForm.type = vdx_types_XForm;
    a = visio_point(points[0]);

    /* Find width and height */
    minX = points[0].x; minY = points[0].y; 
    maxX = points[0].x; maxY = points[0].y;
    for (i=1; i<num_points; i++)
    {
        if (points[i].x < minX) minX = points[i].x;
        if (points[i].x > maxX) maxX = points[i].x;
        if (points[i].y < minY) minY = points[i].y;
        if (points[i].y > maxY) maxY = points[i].y;
    }
    XForm.Width = visio_length(maxX - minX);
    XForm.Height = visio_length(maxY - minY);

    XForm.PinX = a.x;           /* Start */
    XForm.PinY = a.y;
    XForm.LocPinX = 0.0;
    XForm.LocPinY = 0.0;
    XForm.Angle = 0.0;

    /* Standard Geom object */
    memset(&Geom, 0, sizeof(Geom));
    Geom.NoFill = 1;
    Geom.type = vdx_types_Geom;

    /* Multiple children - MoveTo(start) and LineTo(others) */
    memset(&MoveTo, 0, sizeof(MoveTo));
    MoveTo.type = vdx_types_MoveTo;
    MoveTo.IX = 1;
    MoveTo.X = 0;
    MoveTo.Y = 0;

    LineTo = g_new0(struct vdx_LineTo, num_points-1);
    for (i=0; i<num_points-1; i++)
    {
        LineTo[i].type = vdx_types_LineTo;
        LineTo[i].IX = i+2;
        b = visio_point(points[i+1]);
        LineTo[i].X = b.x-a.x; 
        LineTo[i].Y = b.y-a.y;
    }

    /* A Line (colour etc) */
    create_Line(renderer, color, &Line, 0, 0);

    /* Setup children */
    Geom.children = g_slist_append(Geom.children, &MoveTo);
    for (i=0; i<num_points-1; i++)
    {
        Geom.children = g_slist_append(Geom.children, &LineTo[i]);
    }

    Shape.children = g_slist_append(Shape.children, &XForm);
    Shape.children = g_slist_append(Shape.children, &Line);
    Shape.children = g_slist_append(Shape.children, &Geom);

    /* Write out XML */
    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    /* Free up list entries */
    g_slist_free(Geom.children);
    g_slist_free(Shape.children);
    g_free(LineTo);
}

/** Render a Dia polygon
 * @param self a renderer
 * @param points corners of polygon
 * @param num_points number of points
 * @param color line colour
 */

static void draw_polygon(DiaRenderer *self, 
			 Point *points, int num_points, 
			 Color *color)
{
    Point *more_points = g_new0(Point, num_points+1);
    memcpy(more_points, points, num_points*sizeof(Point));
    more_points[num_points] = more_points[0];
    g_debug("draw_polygon -> draw_polyline");
    draw_polyline(self, more_points, num_points+1, color);
    g_free(more_points);
}

/** Render a Dia filled polygon
 * @param self a renderer
 * @param points corners of polygon
 * @param num_points number of points
 * @param color line colour
 */

static void fill_polygon(DiaRenderer *self, 
			 Point *points, int num_points, 
			 Color *color)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    Point a, b;
    struct vdx_Shape Shape;
    struct vdx_XForm XForm;
    struct vdx_Geom Geom;
    struct vdx_MoveTo MoveTo;
    struct vdx_LineTo* LineTo;
    struct vdx_Fill Fill;
    char NameU[VDX_NAMEU_LEN];
    unsigned int i;
    double minX, minY, maxX, maxY;

    /* First time through, just construct the colour table */
    if (renderer->first_pass) 
    {
        vdxCheckColor(renderer, color);
        return;
    }
    
    g_debug("fill_polygon(%d)", num_points);
    
    /* Setup the standard shape object */
    memset(&Shape, 0, sizeof(Shape));
    Shape.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "FillPolygon.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* An XForm */
    memset(&XForm, 0, sizeof(XForm));
    XForm.type = vdx_types_XForm;
    a = visio_point(points[0]);

    /* Find width and height */
    minX = points[0].x; minY = points[0].y; 
    maxX = points[0].x; maxY = points[0].y;
    for (i=1; i<num_points; i++)
    {
        if (points[i].x < minX) minX = points[i].x;
        if (points[i].x > maxX) maxX = points[i].x;
        if (points[i].y < minY) minY = points[i].y;
        if (points[i].y > maxY) maxY = points[i].y;
    }
    XForm.Width = visio_length(maxX - minX);
    XForm.Height = visio_length(maxY - minY);

    XForm.PinX = a.x;           /* Start */
    XForm.PinY = a.y;
    XForm.LocPinX = 0.0;
    XForm.LocPinY = 0.0;
    XForm.Angle = 0.0;

    /* Standard Geom object */
    memset(&Geom, 0, sizeof(Geom));
    Geom.type = vdx_types_Geom;

    /* Multiple children - MoveTo(start) and LineTo(others) */
    memset(&MoveTo, 0, sizeof(MoveTo));
    MoveTo.type = vdx_types_MoveTo;
    MoveTo.IX = 1;
    MoveTo.X = 0;
    MoveTo.Y = 0;

    LineTo = g_new0(struct vdx_LineTo, num_points);
    for (i=0; i<num_points; i++)
    {
        LineTo[i].type = vdx_types_LineTo;
        LineTo[i].IX = i+2;
        /* Last point = first */
        if (i == num_points-1) b = a;
        else b = visio_point(points[i+1]);
        LineTo[i].X = b.x-a.x; 
        LineTo[i].Y = b.y-a.y;
    }

    /* A Line (colour etc) */
    create_Fill(renderer, color, &Fill);

    /* Setup children */
    Geom.children = g_slist_append(Geom.children, &MoveTo);
    for (i=0; i<num_points; i++)
    {
        Geom.children = g_slist_append(Geom.children, &LineTo[i]);
    }

    Shape.children = g_slist_append(Shape.children, &XForm);
    Shape.children = g_slist_append(Shape.children, &Fill);
    Shape.children = g_slist_append(Shape.children, &Geom);

    /* Write out XML */
    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    /* Free up list entries */
    g_slist_free(Geom.children);
    g_slist_free(Shape.children);
    g_free(LineTo);
}

/** Render a Dia rectangle
 * @param self a renderer
 * @param ul_corner Upper-left corner
 * @param lr_corner Loower-right corner
 * @param color line colour
 */

static void draw_rect(DiaRenderer *self, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color)
{
    Point points[5];            /* 5 so we close path */

    g_debug("draw_rect((%f,%f), (%f,%f)) -> draw_polyline", 
            ul_corner->x, ul_corner->y, lr_corner->x, lr_corner->y);
    points[0].x = ul_corner->x; points[0].y = lr_corner->y;
    points[1] = *lr_corner;
    points[2].x = lr_corner->x; points[2].y = ul_corner->y;
    points[3] = *ul_corner;
    points[4] = points[0];
    
    draw_polygon(self, points, 5, color);
}

/** Render a Dia filled rectangle
 * @param self a renderer
 * @param ul_corner Upper-left corner
 * @param lr_corner Lower-right corner
 * @param color line colour
 */

static void fill_rect(DiaRenderer *self, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color)
{
    Point points[5];            /* 5 so we close path */

    g_debug("fill_rect -> fill_polygon");
    points[0].x = ul_corner->x; points[0].y = lr_corner->y;
    points[1] = *lr_corner;
    points[2].x = lr_corner->x; points[2].y = ul_corner->y;
    points[3] = *ul_corner;
    points[4] = points[0];
    
    fill_polygon(self, points, 5, color);
}

/** Render a Dia arc
 * @param self a renderer
 * @param center centre of arc
 * @param width width of ellipse
 * @param height height of ellipse (= width for Dia circular arcs)
 * @param angle1 start angle to x axis in degrees
 * @param angle2 end angle to x axis in degrees
 * @param color line colour
 * @todo Not done yet
 */

static void draw_arc(DiaRenderer *self, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    Point a;
    struct vdx_Shape Shape;
    struct vdx_XForm XForm;
    struct vdx_Geom Geom;
    struct vdx_EllipticalArcTo EllipticalArcTo;
    struct vdx_MoveTo MoveTo;
    struct vdx_Line Line;
    char NameU[VDX_NAMEU_LEN];
    Point start, control, end;
    float control_angle;

    /* First time through, just construct the colour table */
    if (renderer->first_pass) 
    {
        vdxCheckColor(renderer, color);
        return;
    }
    
    g_debug("draw_arc((%f,%f),%f,%f;%f,%f)", center->x, center->y,
            width, height, angle1, angle2);

    /* Setup the standard shape object */
    memset(&Shape, 0, sizeof(Shape));
    Shape.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "Arc.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* An XForm */
    memset(&XForm, 0, sizeof(XForm));
    XForm.type = vdx_types_XForm;

    /* Find the start of the arc */
    start = *center;
    start.x += (width/2.0)*cos(angle1*DEG_TO_RAD);
    start.y -= (height/2.0)*sin(angle1*DEG_TO_RAD);
    g_debug("start(%f,%f)", start.x, start.y);
    start = visio_point(start);

    /* Find a control point at the midpoint of the arc */
    control = *center;
    control_angle = (angle1 + angle2)/2.0;
    if (angle1 > angle2)
    {
        /* Arc goes antclockwise - allow for this */
        control_angle -= 180;
    }
    control.x += (width/2.0)*cos(control_angle*DEG_TO_RAD);
    control.y -= (height/2.0)*sin(control_angle*DEG_TO_RAD);
    g_debug("control(%f,%f @ %f)", control.x, control.y, control_angle);
    control = visio_point(control);

    /* And the endpoint */
    end = *center;
    end.x += (width/2.0)*cos(angle2*DEG_TO_RAD);
    end.y -= (height/2.0)*sin(angle2*DEG_TO_RAD);
    g_debug("end(%f,%f)", end.x, end.y);
    end = visio_point(end);

    a = start;
    XForm.PinX = a.x;           /* Start */
    XForm.PinY = a.y;
    XForm.Width = visio_length(width);
    XForm.Height = visio_length(height);
    XForm.LocPinX = 0;
    XForm.LocPinY = 0;
    XForm.Angle = 0.0;

    /* Standard Geom object */
    memset(&Geom, 0, sizeof(Geom));
    Geom.NoFill = 1;
    Geom.type = vdx_types_Geom;

    memset(&MoveTo, 0, sizeof(MoveTo));
    MoveTo.type = vdx_types_MoveTo;
    MoveTo.IX = 1;
    MoveTo.X = 0;
    MoveTo.Y = 0;

    /* Second child - EllipticalArcTo */
    memset(&EllipticalArcTo, 0, sizeof(EllipticalArcTo));
    EllipticalArcTo.type = vdx_types_EllipticalArcTo;
    EllipticalArcTo.IX = 2;

    /* X and Y are the end point
       A and B are a control point on the arc
       C is the angle of the major axis
       D is the ratio of the major to minor axes */

    /* Need to fix these */
    EllipticalArcTo.X = end.x - a.x;
    EllipticalArcTo.Y = end.y - a.y;
    EllipticalArcTo.A = control.x - a.x;
    EllipticalArcTo.B = control.y - a.y;
    EllipticalArcTo.C = 0.0;      /* Dia major axis always x axis  */
    if (fabs(height) > EPSILON)
        EllipticalArcTo.D = width/height; /* Always 1 for Dia */
    else
        EllipticalArcTo.D = 1/EPSILON;

    /* A Line (colour etc) */
    create_Line(renderer, color, &Line, 0, 0);

    /* Setup children */
    Geom.children = g_slist_append(Geom.children, &MoveTo);
    Geom.children = g_slist_append(Geom.children, &EllipticalArcTo);

    Shape.children = g_slist_append(Shape.children, &XForm);
    Shape.children = g_slist_append(Shape.children, &Line);
    Shape.children = g_slist_append(Shape.children, &Geom);

    /* Write out XML */
    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    /* Free up list entries */
    g_slist_free(Geom.children);
    g_slist_free(Shape.children);
}

/** Render a Dia filled arc
 * @param self a renderer
 * @param center centre of arc
 * @param width width of bounding box
 * @param height height of bounding box
 * @param angle1 start angle
 * @param angle2 end angle
 * @param color line colour
 * @todo Not done yet - believe unused
 */

static void fill_arc(DiaRenderer *self, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    
    if (renderer->first_pass) 
    {
        vdxCheckColor(renderer, color);
        return;
    }
    g_debug("fill_arc (TODO)");
}

/** Render a Dia ellipse (parallel to axes)
 * @param self a renderer
 * @param center centre of ellipse
 * @param width width of bounding box
 * @param height height of bounding box
 * @param color line colour
 */

static void draw_ellipse(DiaRenderer *self, 
			 Point *center,
			 real width, real height,
			 Color *color)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    Point a;
    struct vdx_Shape Shape;
    struct vdx_XForm XForm;
    struct vdx_Geom Geom;
    struct vdx_Ellipse Ellipse;
    struct vdx_Line Line;
    char NameU[VDX_NAMEU_LEN];

    /* First time through, just construct the colour table */
    if (renderer->first_pass) 
    {
        vdxCheckColor(renderer, color);
        return;
    }
    
    g_debug("draw_ellipse");
    
    /* Setup the standard shape object */
    memset(&Shape, 0, sizeof(Shape));
    Shape.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "Ellipse.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* An XForm */
    memset(&XForm, 0, sizeof(XForm));
    XForm.type = vdx_types_XForm;
    a = visio_point(*center);
    XForm.PinX = a.x;           /* Start */
    XForm.PinY = a.y;
    XForm.Width = visio_length(width);
    XForm.Height = visio_length(height);
    XForm.LocPinX = XForm.Width/2.0;
    XForm.LocPinY = XForm.Height/2.0;
    XForm.Angle = 0.0;

    /* Standard Geom object */
    memset(&Geom, 0, sizeof(Geom));
    Geom.NoFill = 1;
    Geom.type = vdx_types_Geom;

    /* One child - Ellipse */
    memset(&Ellipse, 0, sizeof(Ellipse));
    Ellipse.type = vdx_types_Ellipse;
    Ellipse.IX = 1;
    Ellipse.X = XForm.Width/2.0;
    Ellipse.Y = XForm.Height/2.0;
    Ellipse.A = XForm.Width;
    Ellipse.B = XForm.Height/2.0;
    Ellipse.C = XForm.Width/2.0;
    Ellipse.D = XForm.Height;

    /* A Line (colour etc) */
    create_Line(renderer, color, &Line, 0, 0);

    /* Setup children */
    Geom.children = g_slist_append(Geom.children, &Ellipse);

    Shape.children = g_slist_append(Shape.children, &XForm);
    Shape.children = g_slist_append(Shape.children, &Line);
    Shape.children = g_slist_append(Shape.children, &Geom);

    /* Write out XML */
    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    /* Free up list entries */
    g_slist_free(Geom.children);
    g_slist_free(Shape.children);
}

/** Render a Dia filled ellipse (parallel to axes)
 * @param self a renderer
 * @param center centre of ellipse
 * @param width width of bounding box
 * @param height height of bounding box
 * @param color line colour
 */

static void fill_ellipse(DiaRenderer *self, 
			 Point *center,
			 real width, real height,
			 Color *color)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    Point a;
    struct vdx_Shape Shape;
    struct vdx_XForm XForm;
    struct vdx_Geom Geom;
    struct vdx_Ellipse Ellipse;
    struct vdx_Fill Fill;
    char NameU[VDX_NAMEU_LEN];

    /* First time through, just construct the colour table */
    if (renderer->first_pass) 
    {
        vdxCheckColor(renderer, color);
        return;
    }
    
    g_debug("fill_ellipse");
    
    /* Setup the standard shape object */
    memset(&Shape, 0, sizeof(Shape));
    Shape.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "FillEllipse.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* An XForm */
    memset(&XForm, 0, sizeof(XForm));
    XForm.type = vdx_types_XForm;
    a = visio_point(*center);
    XForm.PinX = a.x;           /* Start */
    XForm.PinY = a.y;
    XForm.Width = visio_length(width);
    XForm.Height = visio_length(height);
    XForm.LocPinX = XForm.Width/2.0;
    XForm.LocPinY = XForm.Height/2.0;
    XForm.Angle = 0.0;

    /* Standard Geom object */
    memset(&Geom, 0, sizeof(Geom));
    Geom.type = vdx_types_Geom;

    /* One child - Ellipse */
    memset(&Ellipse, 0, sizeof(Ellipse));
    Ellipse.type = vdx_types_Ellipse;
    Ellipse.IX = 1;
    Ellipse.X = XForm.Width/2.0;
    Ellipse.Y = XForm.Height/2.0;
    Ellipse.A = XForm.Width;
    Ellipse.B = XForm.Height/2.0;
    Ellipse.C = XForm.Width/2.0;
    Ellipse.D = XForm.Height;

    /* A Fill (colour etc) */
    create_Fill(renderer, color, &Fill);

    /* Setup children */
    Geom.children = g_slist_append(Geom.children, &Ellipse);

    Shape.children = g_slist_append(Shape.children, &XForm);
    Shape.children = g_slist_append(Shape.children, &Fill);
    Shape.children = g_slist_append(Shape.children, &Geom);

    /* Write out XML */
    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    /* Free up list entries */
    g_slist_free(Geom.children);
    g_slist_free(Shape.children);
}

/** Render a Dia string
 * @param self a renderer
 * @param text the string
 * @param pos start (or centre etc.)
 * @param alignment alignment
 * @param color line colour
 * @todo Alignment, colour
 * @bug Bounding box incorrect
 */

static void draw_string(DiaRenderer *self,
			const char *text,
			Point *pos, Alignment alignment,
			Color *color)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    Point a;
    struct vdx_Shape Shape;
    struct vdx_XForm XForm;
    struct vdx_Char Char;
    struct vdx_Text Text;
    struct vdx_text my_text;
    char NameU[VDX_NAMEU_LEN];

    if (renderer->first_pass) 
    {
        /* Add to colour and font tables */
        vdxCheckColor(renderer, color);
        vdxCheckFont(renderer);
        return;
    }

    g_debug("draw_string");
    /* Standard shape */
    memset(&Shape, 0, sizeof(Shape));
    Shape.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "Text.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* XForm describes bounding box */
    memset(&XForm, 0, sizeof(XForm));
    XForm.type = vdx_types_XForm;
    a = visio_point(*pos);
    XForm.PinX = a.x;
    XForm.PinY = a.y;
    XForm.Angle = 0;
    /* Hack to give it an approximate bounding box */
    XForm.Height = renderer->fontheight/vdx_Font_Size_Conversion;
    XForm.Width = strlen(text)*renderer->fontheight/vdx_Font_Size_Conversion;

    /* Character properties */
    memset(&Char, 0, sizeof(Char));
    Char.type = vdx_types_Char;
    Char.Font = vdxCheckFont(renderer);
    Char.Color = *color;
    Char.FontScale = 1;
    Char.Size = renderer->fontheight/vdx_Font_Size_Conversion;

    /* Text object - no attributes */
    memset(&Text, 0, sizeof(Text));
    Text.type = vdx_types_Text;

    /* text object (XML pseudo-tag) - no attributes */
    memset(&my_text, 0, sizeof(my_text));
    my_text.type = vdx_types_text;
    my_text.text = (char *)text;

    /* Construct the children */
    Text.children = g_slist_append(Text.children, &my_text);

    Shape.children = g_slist_append(Shape.children, &XForm);
    Shape.children = g_slist_append(Shape.children, &Char);
    Shape.children = g_slist_append(Shape.children, &Text);

    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    g_slist_free(Text.children);
    g_slist_free(Shape.children);
}

/** Reads binary file and converts to Base64 data
 * @param filename file to read
 * @returns Base64 encoded data (or NULL if problem)
 * @note glibc 2.12 offers g_base64_encode()
 */
static char *
read_base64_file(const char *filename)
{
    FILE *f;
    char *b64 = 0;
    char *s = 0;
    int c = 0;
    char map[64];
    unsigned int buf_len = 0;
    unsigned char buf[3];
    struct stat stat_buf;

    if (g_stat(filename, &stat_buf))
    {
        message_error(_("Couldn't read file %s"), filename); 
        return 0;
    }
    b64 = g_new0(char, stat_buf.st_size*4/3+5);
    s = b64;

    f = g_fopen(filename, "r+b");
    if (!f)
    {
        message_error(_("Couldn't read file %s"), filename); 
        return 0;
    }

    /* Construct Base64 mapping table */
    for(c=0; c<26; c++) map[c] = 'A' + c;
    for(c=0; c<26; c++) map[c+26] = 'a' + c;
    for(c=0; c<10; c++) map[c+52] = '0' + c;
    map[62] = '+';
    map[63] = '/';

    while((c = fgetc(f)) != EOF)
    {
        buf[buf_len++] = (unsigned char)c;
        if (buf_len == 3)
        {
            *s++ = map[buf[0] >> 2];
            *s++ = map[((buf[0] & 3) << 4) + (buf[1] >> 4)];
            *s++ = map[((buf[1] & 15) << 2) + (buf[2] >> 6)];
            *s++ = map[buf[2] & 63];
            buf_len = 0;
        }
    }

    if (buf_len == 1)
    {
        *s++ = map[buf[0] >> 2];
        *s++ = map[((buf[0] & 3) << 4)];
        *s++ = '=';
        *s++ = '=';
    }
    if (buf_len == 2)
    {
        *s++ = map[buf[0] >> 2];
        *s++ = map[((buf[0] & 3) << 4) + (buf[1] >> 4)];
        *s++ = map[((buf[1] & 15) << 2)];
        *s++ = '=';
    }

    fclose(f);
    *s = 0;
    return b64;

    /* Deal with any chunks left over */
    if (buf_len)
    {
        fputc(buf[0] << 2 | buf[1] >> 4, f);
        if (buf_len > 1)
        {
            fputc(buf[1] << 4 | buf[2] >> 2, f);
            if (buf_len > 2)
            {
                /* This one can't happen */
                fputc(buf[2] << 6 | buf[3], f);
            }
        }
    }

    fclose(f);
}

/** Render a Dia bitmap
 * @param self a renderer
 * @param point top left
 * @param width width
 * @param height height
 */

static void draw_image(DiaRenderer *self,
		       Point *point,
		       real width, real height,
		       DiaImage *image)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    Point a, bottom_left;
    struct vdx_Shape Shape;
    struct vdx_XForm XForm;
    struct vdx_Geom Geom;
    struct vdx_Foreign Foreign;
    struct vdx_ForeignData ForeignData;
    struct vdx_text text;
    char NameU[VDX_NAMEU_LEN];
    const char *filename = NULL;
    const char *suffix = NULL;

    if (renderer->first_pass) 
    {
        return;
    }

    g_debug("draw_image((%f,%f), %f, %f, %s", point->x, point->y, 
            width, height, dia_image_filename(image));
    /* Setup the standard shape object */
    memset(&Shape, 0, sizeof(Shape));
    Shape.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Foreign";
    sprintf(NameU, "Foreign.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* An XForm */
    memset(&XForm, 0, sizeof(XForm));
    XForm.type = vdx_types_XForm;
    bottom_left.x = point->x;
    bottom_left.y = point->y + height;
    a = visio_point(bottom_left);
    XForm.PinX = a.x;           /* Start */
    XForm.PinY = a.y;
    XForm.Width = visio_length(width);
    XForm.Height = visio_length(height);
    XForm.LocPinX = 0;
    XForm.LocPinY = 0;
    XForm.Angle = 0.0;

    /* Standard Geom object */
    memset(&Geom, 0, sizeof(Geom));
    Geom.type = vdx_types_Geom;
    /* We don't use it, but our decoder needs it */

    /* And a Foreign */
    memset(&Foreign, 0, sizeof(Foreign));
    Foreign.type = vdx_types_Foreign;
    Foreign.ImgOffsetX = 0;
    Foreign.ImgOffsetY = 0;
    Foreign.ImgHeight = visio_length(height);
    Foreign.ImgWidth = visio_length(width);

    /* And a ForeignData */
    memset(&ForeignData, 0, sizeof(ForeignData));
    ForeignData.type = vdx_types_ForeignData;
    ForeignData.ForeignType = "Bitmap";
    ForeignData.CompressionType = "JPEG";
    ForeignData.CompressionLevel = 1.0;
    ForeignData.ObjectHeight = visio_length(height);
    ForeignData.ObjectWidth = visio_length(width);
    
    filename = dia_image_filename(image);
    if ((suffix = strrchr(filename, '.')))
    {
        suffix++;
        if (!g_ascii_strncasecmp(suffix, "png", 3)) { ForeignData.CompressionType = "PNG"; }
        if (!g_ascii_strncasecmp(suffix, "gif", 3)) { ForeignData.CompressionType = "GIF"; }
        if (!g_ascii_strncasecmp(suffix, "jpg", 3) || !g_ascii_strncasecmp(suffix, "jpeg", 4)) 
        { ForeignData.CompressionType = "JPEG"; }
        if (!g_ascii_strncasecmp(suffix, "tif", 3) || !g_ascii_strncasecmp(suffix, "tiff", 4)) 
        { ForeignData.CompressionType = "TIFF"; }
    }

    /* And the data itself */
    memset(&text, 0, sizeof(text));
    text.type = vdx_types_text;
    text.text = read_base64_file(filename);
    if (!text.text) return;     /* Problem reading file */

    /* Setup children */
    Shape.children = g_slist_append(Shape.children, &XForm);
    Shape.children = g_slist_append(Shape.children, &Geom);
    Shape.children = g_slist_append(Shape.children, &Foreign);
    Shape.children = g_slist_append(Shape.children, &ForeignData);
    ForeignData.children = g_slist_append(ForeignData.children, &text);

    /* Write out XML */
    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    /* Free up list entries */
    g_slist_free(ForeignData.children);
    g_slist_free(Shape.children);

    /* And the Base64 data */
    g_free(text.text);
}

/** Convert Dia colour to hex string
 * @param c a colour
 * @returns string in form #000000
 * @note static buffer overwritten with next call; not thread-safe
 */

const char *
vdx_string_color(Color c)
{
    static char buf[8];
    sprintf(buf, "#%.2X%.2X%.2X", 
            (int)(c.red*255), (int)(c.green*255), (int)(c.blue*255));
    return buf;
}

/** Return string XML-encoded
 * @param s a string
 * @returns encoded string
 * @note uses static buffer so can be used as inline filter in printf
 */

const char *
vdx_convert_xml_string(const char *s)
{
    static char *out = 0;
    char *c;

    /* If (as almost always) no change required, return intact */
    if (strcspn(s, "&<>\"'") == strlen(s)) return s;

    /* Ensure we have enough space, even if all the string is quotes */
    out = realloc(out, 6*strlen(s)+1);
    c = out;

    while(*s)
    {
        switch(*s)
        {
        case '&':
            strcpy(c, "&amp;"); c += 5;
            break;
        case '<':
            strcpy(c, "&lt;"); c += 4;
            break;
        case '>':
            strcpy(c, "&gt;"); c += 4;
            break;
        case '\"':
        case '\'':
            strcpy(c, "&quot;"); c += 6;
            break;
        default:
            *c++ = *s;
        }
        s++;
    }
    *c = 0;
    return out;
}

/** Write VDX file header
 * @param data diagram data
 * @param renderer a renderer
 * @note Must know if 2002 or 2003 before start
 * @todo paper size, orientation, metadata, 2003 font attributes
 */

static void
write_header(DiagramData *data, VDXRenderer *renderer)
{
    FILE *file = renderer->file;
    Color c;
    const char *f;
    unsigned int i;
    struct vdx_StyleSheet StyleSheet;
    struct vdx_StyleProp StyleProp;
    struct vdx_Line Line;
    struct vdx_Fill Fill;
    struct vdx_TextBlock TextBlock;
    struct vdx_Char Char;
    struct vdx_Para Para;
    struct vdx_Tabs Tabs;
    static Color color_black = { 0.0, 0.0, 0.0, 1.0 };

    g_debug("write_header");

    /* Basic XML identifying info */

    fprintf(file, "<?xml version='1.0' encoding='utf-8'?>\n");
    fprintf(file, "<!-- Created by Dia -->\n");
    if (renderer->version == 2002)
    {
        fprintf(file, 
                "<VisioDocument "
                "xmlns='urn:schemas-microsoft-com:office:visio'>\n");
    }
    if (renderer->version == 2003)
    {
        fprintf(file, "<VisioDocument "
                "xmlns='http://schemas.microsoft.com/visio/2003/core' "
                "start='190' metric='0' "
                "DocLangID='1033' version='11.0' xml:space='preserve'>\n");
    }

    /* Skipping DocumentProperties, DocumentSettings - observed unnecessary */

    /*  Other fields that may be useful:
        fprintf(file, (data->paper.is_portrait?"Portrait\n":"Landscape\n"));
        fprintf(file, "%s\n", data->paper.name);
        fprintf(file, "%f\n", data->paper.scaling*100.0);
    */
  
    /* Colour table */

    fprintf(file, "  <Colors>\n");
    for (i = 0; i < renderer->Colors->len; i++) 
    {
        c = g_array_index(renderer->Colors, Color, i);
        fprintf(file, "    <ColorEntry IX='%d' RGB='%s'/>\n", 
                i, vdx_string_color(c));
    }
    fprintf(file, "  </Colors>\n");

    /* Font table - different between the two versions (alas) */

    if (renderer->version == 2002)
    {
        fprintf(file, "  <Fonts>\n");
        for (i = 0; i < renderer->Fonts->len; i++) 
        {
            struct vdx_FontEntry Font;
            memset(&Font, 0, sizeof(Font));
            Font.type = vdx_types_FontEntry;
            f = g_array_index(renderer->Fonts, char *, i);
            
            /* Assume want stndard fonts names converted */
            if (!strcmp(f, "Helvetica")) f = "Arial";
            if (!strcmp(f, "Times")) f = "Times New Roman";

            Font.Name = (char *)f;
            /* Sensible defaults */
            Font.CharSet = 0;
            Font.CharSet_exists = 1;
            Font.PitchAndFamily = 18;
            Font.PitchAndFamily_exists = 1;
            Font.Attributes = 23040;
            Font.Attributes_exists = 1;
            Font.Weight = 0;
            Font.Unicode = 0;

            /* Some observed values */
            if (!strcmp(f, "Arial")) Font.PitchAndFamily = 32;
            if (!strcmp(f, "Wingdings") || !strcmp(f, "Monotype Sorts") ||
                !strcmp(f, "Symbol")) Font.CharSet = 2;
            if (!strcmp(f, "Monotype Sorts")) Font.Attributes = 4096;
            if (!strcmp(f, "Wingdings") || !strcmp(f, "Monotype Sorts"))
                Font.PitchAndFamily = 2;
            vdx_write_object(renderer->file, 2, &Font);
        }
        fprintf(file, "  </Fonts>\n");
    }
    if (renderer->version == 2003)
    {
        fprintf(file, "  <FaceNames>\n");
        for (i = 0; i < renderer->Fonts->len; i++) 
        {
            f = g_array_index(renderer->Fonts, char *, i);

            /* Assume want stndard fonts names converted */
            if (!strcmp(f, "Helvetica")) f = "Arial";
            if (!strcmp(f, "Times")) f = "Times New Roman";

            /* Missing attrs UnicodeRanges="31367 -2147483648 8 0" 
               CharSets="1073742335 -65536" 
               Panos="2 11 6 4 2 2 2 2 2 4" Flags="325" */
            fprintf(file, "    <FaceName ID='%d' Name='%s'/>\n",
                    i, f);
        }
        fprintf(file, "  </FaceNames>\n");
    }

    /* An initial stylesheet (mandatory) */
    memset(&StyleSheet, 0, sizeof(StyleSheet));
    StyleSheet.type = vdx_types_StyleSheet;
    StyleSheet.NameU = "No Style";

    /* All these values observed */
    memset(&StyleProp, 0, sizeof(StyleProp));
    StyleProp.type = vdx_types_StyleProp;
    StyleProp.EnableLineProps = 1;
    StyleProp.EnableFillProps = 1;
    StyleProp.EnableTextProps = 1;

    memset(&Line, 0, sizeof(Line));
    Line.type = vdx_types_Line;
    Line.LineWeight = 0.01;
    Line.LinePattern = 1;
    
    memset(&Fill, 0, sizeof(Fill));
    Fill.type = vdx_types_Fill;
    Fill.FillForegnd = color_black;
    Fill.FillPattern = 1;

    memset(&TextBlock, 0, sizeof(TextBlock));
    TextBlock.type = vdx_types_TextBlock;
    TextBlock.VerticalAlign = 1;
    TextBlock.DefaultTabStop = 0.59055118110236;
    
    memset(&Char, 0, sizeof(Char));
    Char.type = vdx_types_Char;
    Char.FontScale = 1;
    Char.Size = 0.16666666666667;
    
    memset(&Para, 0, sizeof(Para));
    Para.type = vdx_types_Para;
    Para.SpLine = -1.2;
    Para.HorzAlign = 1;
    Para.BulletStr = "&#xe000;";
    Para.BulletFontSize = "-1";

    memset(&Tabs, 0, sizeof(Tabs));
    Tabs.type = vdx_types_Tabs;

    /* Setup children */
    StyleSheet.children = g_slist_append(StyleSheet.children, &StyleProp);
    StyleSheet.children = g_slist_append(StyleSheet.children, &Line);
    StyleSheet.children = g_slist_append(StyleSheet.children, &Fill);
    StyleSheet.children = g_slist_append(StyleSheet.children, &TextBlock);
    StyleSheet.children = g_slist_append(StyleSheet.children, &Char);
    StyleSheet.children = g_slist_append(StyleSheet.children, &Para);
    StyleSheet.children = g_slist_append(StyleSheet.children, &Tabs);

    fprintf(file, "  <StyleSheets>\n");
    vdx_write_object(renderer->file, 2, &StyleSheet);
    fprintf(file, "  </StyleSheets>\n");

    g_slist_free(StyleSheet.children);

    /*  Following attributes observed */
    fprintf(file, "  <Pages>\n");
    fprintf(file, "    <Page ID='0' NameU='Page-1' ViewScale='-1' "
            "ViewCenterX='5.8425196850394' ViewCenterY='3.7244094488189'>\n");
    fprintf(file, "      <Shapes>\n");
    renderer->xml_depth = 4;
    renderer->shapeid = 1;

    /* Exit nested <VisioDocument><Pages><Page><Shapes> */
}

/** Write VDX file trailer
 * @param data diagram data
 * @param renderer a renderer
 */

static void
write_trailer(DiagramData *data, VDXRenderer *renderer)
{
    FILE *file = renderer->file;

    g_debug("write_trailer");

    /* Enter nested <VisioDocument><Pages><Page><Shapes> */

    fprintf(file, "      </Shapes>\n");
    fprintf(file, "    </Page>\n");
    fprintf(file, "  </Pages>\n");
    fprintf(file, "</VisioDocument>\n");
}

/** Write VDX file
 * @param data diagram data
 * @param filename output file (should check suffix)
 * @param diafilename input filename (unused)
 * @param user_data user data (unused)
 * @note Must know if 2002 or 2003 before start
 */

static void
export_vdx(DiagramData *data, const gchar *filename, 
           const gchar *diafilename, void* user_data)
{
    FILE *file;
    VDXRenderer *renderer;
    int i;
    Layer *layer;

    file = g_fopen(filename, "w");

    if (file == NULL) {
        message_error(_("Can't open output file %s: %s\n"), 
                      dia_message_filename(filename), strerror(errno));
        return;
    }

    /* Create and initialise our renderer */
    renderer = g_object_new(VDX_TYPE_RENDERER, NULL);

    renderer->file = file;

    renderer->first_pass = TRUE;
    
    renderer->version = 2002;   /* For now */

    DIA_RENDERER_GET_CLASS(renderer)->begin_render(DIA_RENDERER(renderer));
  
    /* First run through without drawing to setup tables */
    for (i=0; i<data->layers->len; i++) 
    {
        layer = (Layer *) g_ptr_array_index(data->layers, i);
        layer_render(layer, DIA_RENDERER(renderer), NULL, NULL, data, 0);
        renderer->depth++;
    }
  
    write_header(data, renderer);

    DIA_RENDERER_GET_CLASS(renderer)->end_render(DIA_RENDERER(renderer));

    renderer->first_pass = FALSE;

    DIA_RENDERER_GET_CLASS(renderer)->begin_render(DIA_RENDERER(renderer));

    /* Now render */

    for (i=0; i<data->layers->len; i++) 
    {
        layer = (Layer *) g_ptr_array_index(data->layers, i);
        layer_render(layer, DIA_RENDERER(renderer), NULL, NULL, data, 0);
        renderer->depth++;
    }
  
    DIA_RENDERER_GET_CLASS(renderer)->end_render(DIA_RENDERER(renderer));

    /* Done */

    write_trailer(data, renderer);

    g_object_unref(renderer);

    fclose(file);
}

/* interface from filter.h */

static const gchar *extensions[] = { "vdx", NULL };
DiaExportFilter vdx_export_filter = {
  N_("Visio XML format"),
  extensions,
  export_vdx
};

#if 0

/* The following are not provided by vdx-export.c so we use the defaults.
   If they become necessary after testing, we can reinstate them */

/** Render a Dia line with arrows
 * @param self a renderer
 * @param start start of line
 * @param end end of line
 * @param line_width width of line
 * @param color line colour
 * @param start_arrow start arrow
 * @param end_arrow end arrow
 */

static void draw_line_with_arrows(DiaRenderer *self, 
				  Point *start, Point *end, 
				  real line_width,
				  Color *color,
				  Arrow *start_arrow,
				  Arrow *end_arrow)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    Point a, b;
    struct vdx_Shape Shape;
    struct vdx_XForm XForm;
    struct vdx_XForm1D XForm1D;
    struct vdx_Geom Geom;
    struct vdx_MoveTo MoveTo;
    struct vdx_LineTo LineTo;
    struct vdx_Line Line;
    char NameU[VDX_NAMEU_LEN];

    /* Exactly as draw_line for now */
    
    if (renderer->first_pass) 
    {
        vdxCheckColor(renderer, color);
        return;
    }

    g_debug("draw_line_with_arrows");
    memset(&Shape, 0, sizeof(Shape));
    Shape.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "ArrowLine.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    memset(&XForm, 0, sizeof(XForm));
    XForm.type = vdx_types_XForm;
    a = visio_point(*start);
    b = visio_point(*end);
    XForm.PinX = a.x;
    XForm.PinY = a.y;
    XForm.Width = fabs(b.x - a.x);
    XForm.Height = fabs(b.y - a.y);
    XForm.LocPinX = 0.0;
    XForm.LocPinY = 0.0;
    XForm.Angle = 0.0;

    memset(&XForm1D, 0, sizeof(XForm1D));
    XForm1D.type = vdx_types_XForm1D;
    XForm1D.BeginX = a.x;
    XForm1D.BeginY = a.y;
    XForm1D.EndX = b.x;
    XForm1D.EndY = b.y;

    memset(&Geom, 0, sizeof(Geom));
    Geom.NoFill = 1;
    Geom.type = vdx_types_Geom;

    memset(&MoveTo, 0, sizeof(MoveTo));
    MoveTo.type = vdx_types_MoveTo;
    MoveTo.IX = 1;
    MoveTo.X = 0;
    MoveTo.Y = 0;

    memset(&LineTo, 0, sizeof(LineTo));
    LineTo.type = vdx_types_LineTo;
    LineTo.IX = 2;
    LineTo.X = b.x-a.x; 
    LineTo.Y = b.y-a.y;

    create_Line(renderer, color, &Line, start_arrow, end_arrow);

    Geom.children = g_slist_append(Geom.children, &MoveTo);
    Geom.children = g_slist_append(Geom.children, &LineTo);

    Shape.children = g_slist_append(Shape.children, &XForm);
    Shape.children = g_slist_append(Shape.children, &XForm1D);
    Shape.children = g_slist_append(Shape.children, &Line);
    Shape.children = g_slist_append(Shape.children, &Geom);

    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    g_slist_free(Geom.children);
    g_slist_free(Shape.children);
}

/** Render a Dia polyline with arrows
 * @param self a renderer
 * @param points points on line
 * @param num_points number of points
 * @param line_width width of line
 * @param color line colour
 * @param start_arrow start arrow
 * @param end_arrow end arrow
 * @todo Not done yet
 */

static void draw_polyline_with_arrows(DiaRenderer *self, 
				      Point *points, int num_points, 
				      real line_width,
				      Color *color,
				      Arrow *start_arrow,
				      Arrow *end_arrow)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    
    if (renderer->first_pass) 
    {
        vdxCheckColor(renderer, color);
        return;
    }
    g_debug("draw_polyline_with_arrows (UNUSED)");
}

/** Render a Dia arc with arrows
 * @param self a renderer
 * @param startpoint start of arc
 * @param endpoint end of arc
 * @param midpoint middle of arc
 * @param line_width line width
 * @param color line colour
 * @param start_arrow start arrow
 * @param end_arrow end arrow
 * @todo Not done yet - believe unused
 */

static void draw_arc_with_arrows(DiaRenderer *self, 
				 Point *startpoint,
				 Point *endpoint,
				 Point *midpoint,
				 real line_width,
				 Color *color,
				 Arrow *start_arrow,
				 Arrow *end_arrow)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    
    if (renderer->first_pass) 
    {
        vdxCheckColor(renderer, color);
        return;
    }
    g_debug("draw_arc_with_arrows (TODO)");
}

/** Render a Dia Bezier
 * @param self a renderer
 * @param points list of Bezier points
 * @param numpoints number of points
 * @param color line colour
 * @todo Not done yet - either convert to arcs or NURBS (Visio 2003)
 */

static void draw_bezier(DiaRenderer *self, 
			BezPoint *points,
			int numpoints,
			Color *color)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    
    if (renderer->first_pass) 
    {
        vdxCheckColor(renderer, color);
        return;
    }
    g_debug("draw_bezier (TODO)");
}

/** Render a Dia Bezier with arrows
 * @param self a renderer
 * @param points list of Bezier points
 * @param numpoints number of points
 * @param line_width line width
 * @param color line colour
 * @param start_arrow start arrow
 * @param end_arrow end arrow
 * @todo Not done yet - either convert to arcs or NURBS (Visio 2003)
 */

static void draw_bezier_with_arrows(DiaRenderer *self, 
				    BezPoint *points,
				    int numpoints,
				    real line_width,
				    Color *color,
				    Arrow *start_arrow,
				    Arrow *end_arrow)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    
    if (renderer->first_pass) 
    {
        vdxCheckColor(renderer, color);
        return;
    }
    g_debug("draw_bezier_with_arrows (TODO)");
}

/** Render a Dia filled Bezier
 * @param self a renderer
 * @param points list of Bezier points (last = first)
 * @param numpoints number of points
 * @param color line colour
 * @todo Not done yet - either convert to arcs or NURBS (Visio 2003)
 */

static void fill_bezier(DiaRenderer *self, 
			BezPoint *points,
			int numpoints,
			Color *color)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    
    if (renderer->first_pass) 
    {
        vdxCheckColor(renderer, color);
        return;
    }

    g_debug("fill_bezier (TODO)");
}

/** Render a Dia object
 * @param self a renderer
 * @param object an object
 * @note No work done here - perhaps should push/pop renderer state
 */

static void draw_object(DiaRenderer *self,
			DiaObject *object)
{
    VDXRenderer *renderer = VDX_RENDERER(self);

    g_debug("draw_object -> renderer");
    /* Get the object to draw itself */
    object->ops->draw(object, DIA_RENDERER(renderer));
}


#endif
