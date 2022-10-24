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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>

#include <string.h>
#include <math.h>
#include <glib.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include <glib/gstdio.h>

#include "geometry.h"
#include "diarenderer.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "prop_pixbuf.h"
#include "dia_image.h"
#include "group.h"
#include "dia-layer.h"

#include "vdx.h"
#include "visio-types.h"

/* Following code taken from xfig-export.c */

#define VDX_TYPE_RENDERER           (vdx_renderer_get_type ())
#define VDX_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), VDX_TYPE_RENDERER, VDXRenderer))
#define VDX_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), VDX_TYPE_RENDERER, VDXRendererClass))
#define VDX_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VDX_TYPE_RENDERER))
#define VDX_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), VDX_TYPE_RENDERER, VDXRendererClass))


enum {
  PROP_0,
  PROP_FONT,
  PROP_FONT_HEIGHT,
  LAST_PROP
};


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

    double linewidth;
    DiaLineCaps capsmode;
    DiaLineJoin joinmode;
    DiaLineStyle stylemode;
    double dashlength;
    DiaFillStyle fillmode;
    DiaFont *font;
    double fontheight;

    /* Additions for VDX */

    gboolean first_pass;        /* When we make table of colours and fonts */
    GArray *Colors;             /* Table of colours */
    GArray *Fonts;              /* Table of fonts */
    unsigned int shapeid;       /* Shape counter */
    unsigned int version;       /* Visio version */
    unsigned int xml_depth;     /* Pretty-printer */
};


static void vdx_renderer_class_init (VDXRendererClass *klass);

static gboolean export_vdx (DiagramData *data,
                            DiaContext  *ctx,
                            const char  *filename,
                            const char  *diafilename,
                            void        *user_data);

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

/** Set font
 * @param self a renderer
 * @param font new font
 * @param height new font height
 */

static void
set_font (DiaRenderer *self, DiaFont *font, real height)
{
  VDXRenderer *renderer = VDX_RENDERER(self);

  g_clear_object (&renderer->font);
  renderer->font = g_object_ref (font);
  renderer->fontheight = height;
}

static void
vdx_renderer_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  VDXRenderer *self = VDX_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      set_font (DIA_RENDERER (self),
                DIA_FONT (g_value_get_object (value)),
                self->fontheight);
      break;
    case PROP_FONT_HEIGHT:
      set_font (DIA_RENDERER (self),
                self->font,
                g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
vdx_renderer_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  VDXRenderer *self = VDX_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      g_value_set_object (value, self->font);
      break;
    case PROP_FONT_HEIGHT:
      g_value_set_double (value, self->fontheight);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

/** Finalise a renderer
 * @param object a renderer
 */

static void
vdx_renderer_finalize (GObject *object)
{
  VDXRenderer *self = VDX_RENDERER (object);

  g_clear_object (&self->font);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/** Initialises VDXrenderer
 * @param self a renderer
 */

static void
begin_render(DiaRenderer *self, const DiaRectangle *update)
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
    /* Visio does not like <shape ID='0'> */
    renderer->shapeid = 1;
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


/*
 * Set line caps
 */
static void
set_linecaps (DiaRenderer *self, DiaLineCaps mode)
{
  VDXRenderer *renderer = VDX_RENDERER(self);

  renderer->capsmode = mode;
}


/**
 * Set line join
 */
static void
set_linejoin(DiaRenderer *self, DiaLineJoin mode)
{
  VDXRenderer *renderer = VDX_RENDERER (self);

  renderer->joinmode = mode;
}


/*
 * Set line style
 */
static void
set_linestyle (DiaRenderer *self, DiaLineStyle mode, double dash_length)
{
  VDXRenderer *renderer = VDX_RENDERER (self);

  renderer->stylemode = mode;
  renderer->dashlength = dash_length;
}


/*
 * Set fill style
 */
static void
set_fillstyle (DiaRenderer *self, DiaFillStyle mode)
{
  VDXRenderer *renderer = VDX_RENDERER (self);

  renderer->fillmode = mode;
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
    const char *font = dia_font_get_family(renderer->font);
    for (i = 0; i < renderer->Fonts->len; i++)
    {
        cmp_font = g_array_index(renderer->Fonts, char *, i);
        if (!strcmp(cmp_font, font))
	  return i;
    }
    /* Grow table */
    g_array_append_val(renderer->Fonts, font);
    return renderer->Fonts->len - 1;
}


/**
 * create_Line:
 * @self: a VDXRenderer
 * @color: a colour
 * @Line: a Line object
 * @start_arrow: optional start arrow
 * @end_arrow: optional end arrow
 *
 * Create a Visio line style object
 *
 * @todo join, caps, dashlength
 */
static void
create_Line (VDXRenderer     *renderer,
             Color           *color,
             struct vdx_Line *Line,
             Arrow           *start_arrow,
             Arrow           *end_arrow)
{
  /* A Line (colour etc) */
  memset (Line, 0, sizeof (*Line));

  Line->any.type = vdx_types_Line;

  switch (renderer->stylemode) {
    case DIA_LINE_STYLE_DASHED:
      Line->LinePattern = 2;
      break;
    case DIA_LINE_STYLE_DOTTED:
      Line->LinePattern = 3;
      break;
    case DIA_LINE_STYLE_DASH_DOT:
      Line->LinePattern = 4;
      break;
    case DIA_LINE_STYLE_DASH_DOT_DOT:
      Line->LinePattern = 5;
      break;
    case DIA_LINE_STYLE_DEFAULT:
    case DIA_LINE_STYLE_SOLID:
    default:
      Line->LinePattern = 1;
      break;
  }
  Line->LineColor = *color;
  Line->LineColorTrans = 1.0 - color->alpha;
  Line->LineWeight = renderer->linewidth / vdx_Line_Scale;
  /* VDX only has Rounded (0) or Square (1) ends */
  if (renderer->capsmode != DIA_LINE_CAPS_ROUND) {
    Line->LineCap = 1; /* Square */
  }

  if (start_arrow || end_arrow) {
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
    Fill->any.type = vdx_types_Fill;
    Fill->FillForegnd = *color;
    Fill->FillForegndTrans = 1.0 - color->alpha;
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
    Shape.any.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "Line.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* An XForm */
    memset(&XForm, 0, sizeof(XForm));
    XForm.any.type = vdx_types_XForm;
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
    XForm1D.any.type = vdx_types_XForm1D;
    XForm1D.BeginX = a.x;
    XForm1D.BeginY = a.y;
    XForm1D.EndX = b.x;
    XForm1D.EndY = b.y;

    /* Standard Geom object */
    memset(&Geom, 0, sizeof(Geom));
    Geom.NoFill = 1;
    Geom.any.type = vdx_types_Geom;

    /* Two children - MoveTo(start) and LineTo(end) */
    memset(&MoveTo, 0, sizeof(MoveTo));
    MoveTo.any.type = vdx_types_MoveTo;
    MoveTo.IX = 1;
    MoveTo.X = 0;
    MoveTo.Y = 0;

    memset(&LineTo, 0, sizeof(LineTo));
    LineTo.any.type = vdx_types_LineTo;
    LineTo.IX = 2;
    LineTo.X = b.x-a.x;
    LineTo.Y = b.y-a.y;

    /* A Line (colour etc) */
    create_Line(renderer, color, &Line, 0, 0);

    /* Setup children */
    Geom.any.children = g_slist_append(Geom.any.children, &MoveTo);
    Geom.any.children = g_slist_append(Geom.any.children, &LineTo);

    Shape.any.children = g_slist_append(Shape.any.children, &XForm);
    Shape.any.children = g_slist_append(Shape.any.children, &XForm1D);
    Shape.any.children = g_slist_append(Shape.any.children, &Line);
    Shape.any.children = g_slist_append(Shape.any.children, &Geom);

    /* Write out XML */
    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    /* Free up list entries */
    g_slist_free(Geom.any.children);
    g_slist_free(Shape.any.children);
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
    Shape.any.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "PolyLine.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* An XForm */
    memset(&XForm, 0, sizeof(XForm));
    XForm.any.type = vdx_types_XForm;
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
    Geom.any.type = vdx_types_Geom;

    /* Multiple children - MoveTo(start) and LineTo(others) */
    memset(&MoveTo, 0, sizeof(MoveTo));
    MoveTo.any.type = vdx_types_MoveTo;
    MoveTo.IX = 1;
    MoveTo.X = 0;
    MoveTo.Y = 0;

    LineTo = g_new0(struct vdx_LineTo, num_points-1);
    for (i=0; i<num_points-1; i++)
    {
        LineTo[i].any.type = vdx_types_LineTo;
        LineTo[i].IX = i+2;
        b = visio_point(points[i+1]);
        LineTo[i].X = b.x-a.x;
        LineTo[i].Y = b.y-a.y;
    }

    /* A Line (colour etc) */
    create_Line(renderer, color, &Line, 0, 0);

    /* Setup children */
    Geom.any.children = g_slist_append(Geom.any.children, &MoveTo);
    for (i=0; i<num_points-1; i++)
    {
        Geom.any.children = g_slist_append(Geom.any.children, &LineTo[i]);
    }

    Shape.any.children = g_slist_append(Shape.any.children, &XForm);
    Shape.any.children = g_slist_append(Shape.any.children, &Line);
    Shape.any.children = g_slist_append(Shape.any.children, &Geom);

    /* Write out XML */
    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    /* Free up list entries */
    g_slist_free(Geom.any.children);
    g_slist_free(Shape.any.children);
    g_clear_pointer (&LineTo, g_free);
}

static void
_polygon (DiaRenderer *self,
	  Point *points, int num_points,
	  Color *fill, Color *stroke,
	  real radius)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    Point a, b;
    struct vdx_Shape Shape;
    struct vdx_XForm XForm;
    struct vdx_Geom Geom;
    struct vdx_MoveTo MoveTo;
    struct vdx_LineTo* LineTo;
    struct vdx_Fill Fill;
    struct vdx_Line Line;
    char NameU[VDX_NAMEU_LEN];
    unsigned int i;
    double minX, minY, maxX, maxY;

    /* First time through, just construct the colour table */
    if (renderer->first_pass)
    {
	if (fill)
	    vdxCheckColor(renderer, fill);
	if (stroke)
	    vdxCheckColor(renderer, stroke);
        return;
    }

    g_debug("draw_polygon(%d)", num_points);

    /* Setup the standard shape object */
    memset(&Shape, 0, sizeof(Shape));
    Shape.any.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "Polygon.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* An XForm */
    memset(&XForm, 0, sizeof(XForm));
    XForm.any.type = vdx_types_XForm;
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
    Geom.any.type = vdx_types_Geom;

    /* Multiple children - MoveTo(start) and LineTo(others) */
    memset(&MoveTo, 0, sizeof(MoveTo));
    MoveTo.any.type = vdx_types_MoveTo;
    MoveTo.IX = 1;
    MoveTo.X = 0;
    MoveTo.Y = 0;

    LineTo = g_new0(struct vdx_LineTo, num_points);
    for (i=0; i<num_points; i++)
    {
        LineTo[i].any.type = vdx_types_LineTo;
        LineTo[i].IX = i+2;
        /* Last point = first */
        if (i == num_points-1) b = a;
        else b = visio_point(points[i+1]);
        LineTo[i].X = b.x-a.x;
        LineTo[i].Y = b.y-a.y;
    }

    /* A Line (colour etc) */
    if (fill)
	create_Fill(renderer, fill, &Fill);
    if (stroke)
	create_Line(renderer, stroke, &Line, NULL, NULL);
    Line.Rounding = visio_length (radius);

    Geom.NoFill = fill ? 0 : 1;
    Geom.NoLine = stroke ? 0 : 1;
    /* Setup children */
    Geom.any.children = g_slist_append(Geom.any.children, &MoveTo);
    for (i=0; i<num_points; i++)
    {
        Geom.any.children = g_slist_append(Geom.any.children, &LineTo[i]);
    }

    Shape.any.children = g_slist_append(Shape.any.children, &XForm);
    if (fill)
	Shape.any.children = g_slist_append(Shape.any.children, &Fill);
    if (stroke || radius > 0.0)
	Shape.any.children = g_slist_append(Shape.any.children, &Line);
    Shape.any.children = g_slist_append(Shape.any.children, &Geom);

  /* Write out XML */
  vdx_write_object (renderer->file, renderer->xml_depth, &Shape);

  /* Free up list entries */
  g_slist_free (Geom.any.children);
  g_slist_free (Shape.any.children);
  g_clear_pointer (&LineTo, g_free);
}


/** Render a Dia filled polygon
 * @param self a renderer
 * @param points corners of polygon
 * @param num_points number of points
 * @param color line colour
 */
static void
draw_polygon (DiaRenderer *self,
	      Point *points, int num_points,
	      Color *fill, Color *stroke)
{
  _polygon (self, points, num_points, fill, stroke, 0.0);
}

static void
draw_rounded_rect (DiaRenderer *self,
		   Point *ul_corner, Point *lr_corner,
		   Color *fill, Color *stroke,
		   real radius)
{
    Point points[4];            /* close path done by _polygon() */

    g_debug("draw_rounded_rect((%f,%f), (%f,%f)) -> draw_polyline",
            ul_corner->x, ul_corner->y, lr_corner->x, lr_corner->y);
    points[0].x = ul_corner->x; points[0].y = lr_corner->y;
    points[1] = *lr_corner;
    points[2].x = lr_corner->x; points[2].y = ul_corner->y;
    points[3] = *ul_corner;

    _polygon (self, points, 4, fill, stroke, radius);
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
    Shape.any.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "Arc.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* An XForm */
    memset(&XForm, 0, sizeof(XForm));
    XForm.any.type = vdx_types_XForm;

    /* Find the start of the arc */
    start = *center;
    start.x += (width/2.0)*cos(angle1*DEG_TO_RAD);
    start.y -= (height/2.0)*sin(angle1*DEG_TO_RAD);
    g_debug("start(%f,%f)", start.x, start.y);
    start = visio_point(start);

    /* Find a control point at the midpoint of the arc */
    control = *center;
    control_angle = (angle1 + angle2)/2.0;
    /* no matter which direction the arc is control_angle is always between start and end */
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
    Geom.any.type = vdx_types_Geom;

    memset(&MoveTo, 0, sizeof(MoveTo));
    MoveTo.any.type = vdx_types_MoveTo;
    MoveTo.IX = 1;
    MoveTo.X = 0;
    MoveTo.Y = 0;

    /* Second child - EllipticalArcTo */
    memset(&EllipticalArcTo, 0, sizeof(EllipticalArcTo));
    EllipticalArcTo.any.type = vdx_types_EllipticalArcTo;
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
    Geom.any.children = g_slist_append(Geom.any.children, &MoveTo);
    Geom.any.children = g_slist_append(Geom.any.children, &EllipticalArcTo);

    Shape.any.children = g_slist_append(Shape.any.children, &XForm);
    Shape.any.children = g_slist_append(Shape.any.children, &Line);
    Shape.any.children = g_slist_append(Shape.any.children, &Geom);

    /* Write out XML */
    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    /* Free up list entries */
    g_slist_free(Geom.any.children);
    g_slist_free(Shape.any.children);
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
static void
fill_arc (DiaRenderer *self,
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
    struct vdx_LineTo LineToCenter, LineToStart;
    struct vdx_Fill Fill;
    char NameU[VDX_NAMEU_LEN];
    Point start, control, end;
    float control_angle;

    /* First time through, just construct the colour table */
    if (renderer->first_pass)
    {
        vdxCheckColor(renderer, color);
        return;
    }

    g_debug("fill_arc((%f,%f),%f,%f;%f,%f)", center->x, center->y,
            width, height, angle1, angle2);

    /* Setup the standard shape object */
    memset(&Shape, 0, sizeof(Shape));
    Shape.any.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "Arc.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* An XForm */
    memset(&XForm, 0, sizeof(XForm));
    XForm.any.type = vdx_types_XForm;

    /* Find the start of the arc */
    start = *center;
    start.x += (width/2.0)*cos(angle1*DEG_TO_RAD);
    start.y -= (height/2.0)*sin(angle1*DEG_TO_RAD);
    g_debug("start(%f,%f)", start.x, start.y);
    start = visio_point(start);

    /* Find a control point at the midpoint of the arc */
    control = *center;
    control_angle = (angle1 + angle2)/2.0;
    /* no matter which direction the arc is control_angle is always between start and end */
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
    Geom.NoLine = 1;
    Geom.any.type = vdx_types_Geom;

    memset(&MoveTo, 0, sizeof(MoveTo));
    MoveTo.any.type = vdx_types_MoveTo;
    MoveTo.IX = 1;
    MoveTo.X = 0;
    MoveTo.Y = 0;

    /* Second child - EllipticalArcTo */
    memset(&EllipticalArcTo, 0, sizeof(EllipticalArcTo));
    EllipticalArcTo.any.type = vdx_types_EllipticalArcTo;
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

    /* Line to center - reusing contol for transformed */
    control = visio_point(*center);
    memset(&LineToCenter, 0, sizeof(LineToCenter));
    LineToCenter.any.type = vdx_types_LineTo;
    LineToCenter.IX = 3;
    LineToCenter.X = control.x - a.x;
    LineToCenter.Y = control.y - a.y;
    /* Line to start */
    memset(&LineToStart, 0, sizeof(LineToStart));
    LineToStart.any.type = vdx_types_LineTo;
    LineToStart.IX = 4;
    LineToStart.X = start.x - a.x;
    LineToStart.Y = start.y - a.y;

    /* A Line (colour etc) */
    create_Fill(renderer, color, &Fill);

    /* Setup children */
    Geom.any.children = g_slist_append(Geom.any.children, &MoveTo);
    Geom.any.children = g_slist_append(Geom.any.children, &EllipticalArcTo);
    Geom.any.children = g_slist_append(Geom.any.children, &LineToCenter);
    Geom.any.children = g_slist_append(Geom.any.children, &LineToStart);

    Shape.any.children = g_slist_append(Shape.any.children, &XForm);
    Shape.any.children = g_slist_append(Shape.any.children, &Fill);
    Shape.any.children = g_slist_append(Shape.any.children, &Geom);

    /* Write out XML */
    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    /* Free up list entries */
    g_slist_free(Geom.any.children);
    g_slist_free(Shape.any.children);
}

/** Render a Dia ellipse (parallel to axes)
 * @param self a renderer
 * @param center centre of ellipse
 * @param width width of bounding box
 * @param height height of bounding box
 * @param fill fill colour
 * @param stroke line colour
 */
static void
draw_ellipse (DiaRenderer *self,
	      Point *center,
	      real width, real height,
	      Color *fill, Color *stroke)
{
    VDXRenderer *renderer = VDX_RENDERER(self);
    Point a;
    struct vdx_Shape Shape;
    struct vdx_XForm XForm;
    struct vdx_Geom Geom;
    struct vdx_Ellipse Ellipse;
    struct vdx_Fill Fill;
    struct vdx_Line Line;
    char NameU[VDX_NAMEU_LEN];

    /* First time through, just construct the colour table */
    if (renderer->first_pass)
    {
	if (fill)
	    vdxCheckColor(renderer, fill);
	if (stroke)
	    vdxCheckColor(renderer, stroke);
        return;
    }

    g_debug("fill_ellipse");

    /* Setup the standard shape object */
    memset(&Shape, 0, sizeof(Shape));
    Shape.any.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "Ellipse.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* An XForm */
    memset(&XForm, 0, sizeof(XForm));
    XForm.any.type = vdx_types_XForm;
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
    Geom.any.type = vdx_types_Geom;
    Geom.NoFill = fill ? 0 : 1;
    Geom.NoLine = stroke ? 0 : 1;

    /* One child - Ellipse */
    memset(&Ellipse, 0, sizeof(Ellipse));
    Ellipse.any.type = vdx_types_Ellipse;
    Ellipse.IX = 1;
    Ellipse.X = XForm.Width/2.0;
    Ellipse.Y = XForm.Height/2.0;
    Ellipse.A = XForm.Width;
    Ellipse.B = XForm.Height/2.0;
    Ellipse.C = XForm.Width/2.0;
    Ellipse.D = XForm.Height;

    /* A Fill (colour etc) */
    if (fill)
	create_Fill(renderer, fill, &Fill);
    if (stroke)
	create_Line(renderer, stroke, &Line, NULL, NULL);

    /* Setup children */
    Geom.any.children = g_slist_append(Geom.any.children, &Ellipse);

    Shape.any.children = g_slist_append(Shape.any.children, &XForm);
    if (fill)
	Shape.any.children = g_slist_append(Shape.any.children, &Fill);
    if (stroke)
	Shape.any.children = g_slist_append(Shape.any.children, &Line);
    Shape.any.children = g_slist_append(Shape.any.children, &Geom);

    /* Write out XML */
    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    /* Free up list entries */
    g_slist_free(Geom.any.children);
    g_slist_free(Shape.any.children);
}


/** Render a Dia string
 * @param self a renderer
 * @param text the string
 * @param pos start (or centre etc.)
 * @param alignment alignment
 * @param color line colour
 * @todo DiaAlignment, colour
 * @bug Bounding box incorrect
 */
static void
draw_string (DiaRenderer  *self,
             const char   *text,
             Point        *pos,
             DiaAlignment  alignment,
             Color        *color)
{
    VDXRenderer *renderer = VDX_RENDERER (self);
    Point a;
    struct vdx_Shape Shape;
    struct vdx_XForm XForm;
    struct vdx_Para Para;
    struct vdx_Char Char;
    struct vdx_Text vdxtext;
    struct vdx_pp pp;
    struct vdx_cp cp;
    struct vdx_text my_text;
    char NameU[VDX_NAMEU_LEN];
    DiaFontStyle font_style;
    real text_width;

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
    Shape.any.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "Text.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* XForm describes bounding box */
    memset(&XForm, 0, sizeof(XForm));
    XForm.any.type = vdx_types_XForm;
    /* Align by math until we find the right tags */
    a = *pos;
    text_width = dia_font_string_width(text, renderer->font, renderer->fontheight);
    /* apparently text_width is not useable to scale the text, tried with
     * TextXForm.TxtWidth. But is needed to place the text box. If the text
     * happens to overflow the box width a new line gets created. Ensure the
     * text fits and the box is properly aligned, too.
     */
    text_width *= 1.2;
    a.y += dia_font_descent(text, renderer->font, renderer->fontheight);
    switch (alignment) {
      case DIA_ALIGN_LEFT:
        /* nothing to do this appears to be default */
        break;
      case DIA_ALIGN_CENTRE:
        a.x -= text_width / 2.0;
        break;
      case DIA_ALIGN_RIGHT:
        a.x -= text_width;
        break;
      default:
        g_return_if_reached ();
    }
    a = visio_point(a);
    XForm.PinX = a.x;
    XForm.PinY = a.y;
    XForm.Angle = 0;
    /* Hack to give it an approximate bounding box */
    XForm.Height = renderer->fontheight/vdx_Font_Size_Conversion;
    /* some arbitrary resizing of the paragraph box to make the text always fit */
    XForm.Width = visio_length(text_width);

    /* Character properties */
    memset(&Char, 0, sizeof(Char));
    Char.any.type = vdx_types_Char;
    Char.Font = vdxCheckFont(renderer);
    Char.Color = *color;
    Char.FontScale = 1;
    Char.Size = renderer->fontheight/vdx_Font_Size_Conversion;
    /* Fontstyle: bold=1, italic=2, ... */
    font_style = dia_font_get_style(renderer->font);
    Char.Style = DIA_FONT_STYLE_GET_WEIGHT(font_style) >= DIA_FONT_MEDIUM ? 1 : 0 +
                 DIA_FONT_STYLE_GET_SLANT(font_style) ? 2 : 0;
    /* ... reference the above */
    memset(&cp, 0, sizeof(cp));
    cp.any.type = vdx_types_cp;

    /* Text object - no attributes */
    memset(&vdxtext, 0, sizeof(vdxtext));
    vdxtext.any.type = vdx_types_Text;

    memset(&Para, 0, sizeof(Para));
    Para.any.type = vdx_types_Para;
    Para.HorzAlign = alignment; /* compatible enum : Left, Center, Right */
    /* pp - Paragraph properties */
    memset(&pp, 0, sizeof(pp));
    pp.any.type = vdx_types_pp;

    /* text object (XML pseudo-tag) - no attributes */
    memset(&my_text, 0, sizeof(my_text));
    my_text.any.type = vdx_types_text;
    my_text.text = (char *)text;

    /* Construct the children */
    vdxtext.any.children = g_slist_append(vdxtext.any.children, &pp);
    vdxtext.any.children = g_slist_append(vdxtext.any.children, &cp);
    vdxtext.any.children = g_slist_append(vdxtext.any.children, &my_text);

    Shape.any.children = g_slist_append(Shape.any.children, &XForm);
    Shape.any.children = g_slist_append(Shape.any.children, &Char);
    Shape.any.children = g_slist_append(Shape.any.children, &Para);
    Shape.any.children = g_slist_append(Shape.any.children, &vdxtext);

    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    g_slist_free(vdxtext.any.children);
    g_slist_free(Shape.any.children);
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

    if (renderer->first_pass)
    {
        return;
    }

    g_debug("draw_image((%f,%f), %f, %f, %s", point->x, point->y,
            width, height, dia_image_filename(image));
    /* Setup the standard shape object */
    memset(&Shape, 0, sizeof(Shape));
    Shape.any.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Foreign";
    sprintf(NameU, "Foreign.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    /* An XForm */
    memset(&XForm, 0, sizeof(XForm));
    XForm.any.type = vdx_types_XForm;
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
    Geom.any.type = vdx_types_Geom;
    /* We don't use it, but our decoder needs it */

    /* And a Foreign */
    memset(&Foreign, 0, sizeof(Foreign));
    Foreign.any.type = vdx_types_Foreign;
    Foreign.ImgOffsetX = 0;
    Foreign.ImgOffsetY = 0;
    Foreign.ImgHeight = visio_length(height);
    Foreign.ImgWidth = visio_length(width);

    /* And a ForeignData */
    memset(&ForeignData, 0, sizeof(ForeignData));
    ForeignData.any.type = vdx_types_ForeignData;
    ForeignData.ForeignType = "Bitmap";
    ForeignData.CompressionType = "JPEG";
    ForeignData.CompressionLevel = 1.0;
    ForeignData.ObjectHeight = visio_length(height);
    ForeignData.ObjectWidth = visio_length(width);

    /* no more choice - see pixbuf_encode_base64 */
    ForeignData.CompressionType = "PNG";

    /* And the data itself */
    memset(&text, 0, sizeof(text));
    text.any.type = vdx_types_text;
    text.text = pixbuf_encode_base64 (dia_image_pixbuf (image), NULL);
    if (!text.text) return;     /* Problem reading file */

    /* Setup children */
    Shape.any.children = g_slist_append(Shape.any.children, &XForm);
    Shape.any.children = g_slist_append(Shape.any.children, &Geom);
    Shape.any.children = g_slist_append(Shape.any.children, &Foreign);
    Shape.any.children = g_slist_append(Shape.any.children, &ForeignData);
    ForeignData.any.children = g_slist_append(ForeignData.any.children, &text);

  /* Write out XML */
  vdx_write_object (renderer->file, renderer->xml_depth, &Shape);

  /* Free up list entries */
  g_slist_free (ForeignData.any.children);
  g_slist_free (Shape.any.children);

  /* And the Base64 data */
  g_clear_pointer (&text.text, g_free);
}


/** Convert Dia colour to hex string
 * @param c a colour
 * @returns string in form #000000
 * @note static buffer overwritten with next call; not thread-safe
 */
const char *
vdx_string_color (const Color c)
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
    out = g_renew (char, out, 6 * strlen (s) + 1);
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
            Font.any.type = vdx_types_FontEntry;
            f = g_array_index(renderer->Fonts, char *, i);

            Font.ID = i;
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
    StyleSheet.any.type = vdx_types_StyleSheet;
    StyleSheet.NameU = "No Style";

    /* All these values observed */
    memset(&StyleProp, 0, sizeof(StyleProp));
    StyleProp.any.type = vdx_types_StyleProp;
    StyleProp.EnableLineProps = 1;
    StyleProp.EnableFillProps = 1;
    StyleProp.EnableTextProps = 1;

    memset(&Line, 0, sizeof(Line));
    Line.any.type = vdx_types_Line;
    Line.LineWeight = 0.01;
    Line.LinePattern = 1;

    memset(&Fill, 0, sizeof(Fill));
    Fill.any.type = vdx_types_Fill;
    Fill.FillForegnd = color_black;
    Fill.FillPattern = 1;

    memset(&TextBlock, 0, sizeof(TextBlock));
    TextBlock.any.type = vdx_types_TextBlock;
    TextBlock.VerticalAlign = 1;
    TextBlock.DefaultTabStop = 0.59055118110236;

    memset(&Char, 0, sizeof(Char));
    Char.any.type = vdx_types_Char;
    Char.FontScale = 1;
    Char.Size = 0.16666666666667;

    memset(&Para, 0, sizeof(Para));
    Para.any.type = vdx_types_Para;
    Para.SpLine = -1.2;
    Para.HorzAlign = 0; /* left align in the box */
    Para.BulletStr = "&#xe000;";
    Para.BulletFontSize = "-1";

    memset(&Tabs, 0, sizeof(Tabs));
    Tabs.any.type = vdx_types_Tabs;

    /* Setup children */
    StyleSheet.any.children = g_slist_append(StyleSheet.any.children, &StyleProp);
    StyleSheet.any.children = g_slist_append(StyleSheet.any.children, &Line);
    StyleSheet.any.children = g_slist_append(StyleSheet.any.children, &Fill);
    StyleSheet.any.children = g_slist_append(StyleSheet.any.children, &TextBlock);
    StyleSheet.any.children = g_slist_append(StyleSheet.any.children, &Char);
    StyleSheet.any.children = g_slist_append(StyleSheet.any.children, &Para);
    StyleSheet.any.children = g_slist_append(StyleSheet.any.children, &Tabs);

    fprintf(file, "  <StyleSheets>\n");
    vdx_write_object(renderer->file, 2, &StyleSheet);
    fprintf(file, "  </StyleSheets>\n");

    g_slist_free(StyleSheet.any.children);

    /*  Following attributes observed ... */
    fprintf(file, "  <Pages>\n");
    fprintf(file, "    <Page ID='0'>\n");
    /* Write a single page size defintion - Visio Viewer does not care, but LibreOffice does. */
    fprintf(file, "      <PageSheet ID='0'>\n"
		  "        <PageProps>\n"
		  "          <PageWidth>%f</PageWidth>\n"
		  "          <PageHeight>%f</PageHeight>\n"
		  "        </PageProps>\n"
		  "      </PageSheet>\n",
		  visio_length(data->extents.right - data->extents.left),
		  visio_length(data->extents.bottom - data->extents.top));
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
static gboolean
export_vdx (DiagramData *data,
            DiaContext  *ctx,
            const char  *filename,
            const char  *diafilename,
            void        *user_data)
{
    FILE *file;
    VDXRenderer *renderer;
    int i;
    DiaLayer *layer;
    char* old_locale;

    file = g_fopen(filename, "w");

    if (file == NULL) {
	dia_context_add_message_with_errno (ctx, errno, _("Can't open output file %s"),
					    dia_context_get_filename(ctx));
	return FALSE;
    }

    /* ugly, but still better than creatin corrupt files */
    old_locale = setlocale(LC_NUMERIC, "C");

    /* Create and initialise our renderer */
    renderer = g_object_new(VDX_TYPE_RENDERER, NULL);

    renderer->file = file;

    renderer->first_pass = TRUE;

    renderer->version = 2002;   /* For now */

    dia_renderer_begin_render (DIA_RENDERER (renderer), NULL);

    /* First run through without drawing to setup tables */
    for (i = 0; i < data_layer_count (data); i++) {
      layer = data_layer_get_nth (data, i);
      if (dia_layer_is_visible (layer)) {
        dia_layer_render (layer, DIA_RENDERER (renderer), NULL, NULL, data, 0);
      }
      renderer->depth++;
    }

    write_header (data, renderer);

    dia_renderer_end_render (DIA_RENDERER (renderer));

    renderer->first_pass = FALSE;

    dia_renderer_begin_render (DIA_RENDERER (renderer), NULL);

    /* Now render */

    for (i = 0; i < data_layer_count (data); i++) {
      layer = data_layer_get_nth (data, i);
      if (dia_layer_is_visible (layer)) {
        dia_layer_render (layer, DIA_RENDERER (renderer), NULL, NULL, data, 0);
      }
      renderer->depth++;
    }

    dia_renderer_end_render (DIA_RENDERER (renderer));

  /* Done */

  write_trailer (data, renderer);

  g_clear_object (&renderer);

  /* dont screw Dia's global state */
  setlocale (LC_NUMERIC, old_locale);

  if (fclose (file) != 0) {
    dia_context_add_message_with_errno (ctx, errno,
                                        _("Saving file '%s' failed."),
                                        dia_context_get_filename (ctx));
    return FALSE;
  }

  return TRUE;
}

/* interface from filter.h */

static const char *extensions[] = { "vdx", NULL };
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
    Shape.any.type = vdx_types_Shape;
    Shape.ID = renderer->shapeid++;
    Shape.Type = "Shape";
    sprintf(NameU, "ArrowLine.%d", Shape.ID);
    Shape.NameU = NameU;
    Shape.LineStyle_exists = 1;
    Shape.FillStyle_exists = 1;
    Shape.TextStyle_exists = 1;

    memset(&XForm, 0, sizeof(XForm));
    XForm.any.type = vdx_types_XForm;
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
    XForm1D.any.type = vdx_types_XForm1D;
    XForm1D.BeginX = a.x;
    XForm1D.BeginY = a.y;
    XForm1D.EndX = b.x;
    XForm1D.EndY = b.y;

    memset(&Geom, 0, sizeof(Geom));
    Geom.NoFill = 1;
    Geom.any.type = vdx_types_Geom;

    memset(&MoveTo, 0, sizeof(MoveTo));
    MoveTo.any.type = vdx_types_MoveTo;
    MoveTo.IX = 1;
    MoveTo.X = 0;
    MoveTo.Y = 0;

    memset(&LineTo, 0, sizeof(LineTo));
    LineTo.any.type = vdx_types_LineTo;
    LineTo.IX = 2;
    LineTo.X = b.x-a.x;
    LineTo.Y = b.y-a.y;

    create_Line(renderer, color, &Line, start_arrow, end_arrow);

    Geom.any.children = g_slist_append(Geom.any.children, &MoveTo);
    Geom.any.children = g_slist_append(Geom.any.children, &LineTo);

    Shape.any.children = g_slist_append(Shape.any.children, &XForm);
    Shape.any.children = g_slist_append(Shape.any.children, &XForm1D);
    Shape.any.children = g_slist_append(Shape.any.children, &Line);
    Shape.any.children = g_slist_append(Shape.any.children, &Geom);

    vdx_write_object(renderer->file, renderer->xml_depth, &Shape);

    g_slist_free(Geom.any.children);
    g_slist_free(Shape.any.children);
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

/** Render a Dia closed Bezier
 * @param self a renderer
 * @param points list of Bezier points (last = first)
 * @param numpoints number of points
 * @param color line colour
 * @todo Not done yet - either convert to arcs or NURBS (Visio 2003)
 */
static void
draw_beziergon (DiaRenderer *self,
		BezPoint *points,
		int numpoints,
		Color *fill,
		Color *stroke)
{
    VDXRenderer *renderer = VDX_RENDERER(self);

    if (renderer->first_pass)
    {
        vdxCheckColor(renderer, color);
        return;
    }

    g_debug("draw_beziergon (TODO)");
}

/** Render a Dia object
 * @param self a renderer
 * @param object an object
 * @param matrix NULL for identity
 * @note No work done here - perhaps should push/pop renderer state
 */

static void
draw_object (DiaRenderer *self,
	     DiaObject   *object,
	     DiaMatrix   *matrix)
{
    VDXRenderer *renderer = VDX_RENDERER(self);

    g_debug("draw_object -> renderer");
    /* Get the object to draw itself */
    object->ops->draw(object, DIA_RENDERER(renderer));
}


#endif

/** Class constructor for renderer
 * @param klass a renderer
 */
static void
vdx_renderer_class_init (VDXRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = vdx_renderer_set_property;
  object_class->get_property = vdx_renderer_get_property;
  object_class->finalize = vdx_renderer_finalize;

  renderer_class->begin_render = begin_render;
  renderer_class->end_render = end_render;

  renderer_class->set_linewidth = set_linewidth;
  renderer_class->set_linecaps = set_linecaps;
  renderer_class->set_linejoin = set_linejoin;
  renderer_class->set_linestyle = set_linestyle;
  renderer_class->set_fillstyle = set_fillstyle;

  renderer_class->draw_line = draw_line;
  renderer_class->draw_polyline = draw_polyline;

  renderer_class->draw_polygon = draw_polygon;

  renderer_class->draw_arc = draw_arc;
  renderer_class->fill_arc = fill_arc;

  renderer_class->draw_ellipse = draw_ellipse;

  /* Until we have NURBS, let Dia use lines */
  /* renderer_class->draw_bezier = draw_bezier; */
  /* renderer_class->draw_beziergon = draw_beziergon; */
  /* renderer_class->draw_bezier_with_arrows = draw_bezier_with_arrows; */

  renderer_class->draw_string = draw_string;

  renderer_class->draw_image = draw_image;

  renderer_class->draw_rounded_rect = draw_rounded_rect;
  /* Further high level methods not required (or desired?) */

  g_object_class_override_property (object_class, PROP_FONT, "font");
  g_object_class_override_property (object_class, PROP_FONT_HEIGHT, "font-height");
}
