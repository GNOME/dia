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

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <errno.h>
#include <glib/gstdio.h>

#include "autocad_pal.h"

#include "intl.h"
#include "geometry.h"
#include "diarenderer.h"
#include "filter.h"
#include "dia-layer.h"
#include "font.h"

/* used to be 10 and inconsistent with import and even here */
#define MAGIC_THICKNESS_FACTOR (1.0)

#define DXF_TYPE_RENDERER           (dxf_renderer_get_type ())
#define DXF_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DXF_TYPE_RENDERER, DxfRenderer))
#define DXF_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DXF_TYPE_RENDERER, DxfRendererClass))
#define DXF_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DXF_TYPE_RENDERER))
#define DXF_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DXF_TYPE_RENDERER, DxfRendererClass))

enum {
  PROP_0,
  PROP_FONT,
  PROP_FONT_HEIGHT,
  LAST_PROP
};


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

  const char *layername;
};

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
set_font (DiaRenderer *self, DiaFont *font, real height)
{
  DxfRenderer *renderer = DXF_RENDERER (self);

  g_clear_object (&renderer->font);
  renderer->font = g_object_ref (font);
  renderer->tcurrent.font_height = height;
}

static void
dxf_renderer_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  DxfRenderer *self = DXF_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      set_font (DIA_RENDERER (self),
                DIA_FONT (g_value_get_object (value)),
                self->tcurrent.font_height);
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
dxf_renderer_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  DxfRenderer *self = DXF_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      g_value_set_object (value, self->font);
      break;
    case PROP_FONT_HEIGHT:
      g_value_set_double (value, self->tcurrent.font_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dxf_renderer_finalize (GObject *object)
{
  DxfRenderer *self = DXF_RENDERER (object);

  g_clear_object (&self->font);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
init_attributes( DxfRenderer *renderer )
{
    renderer->lcurrent.style = renderer->fcurrent.style = "CONTINUOUS";
}

static void
begin_render(DiaRenderer *self, const DiaRectangle *update)
{
}

static void
end_render(DiaRenderer *self)
{
    DxfRenderer *renderer = DXF_RENDERER(self);

    fprintf(renderer->file, "  0\nENDSEC\n  0\nEOF\n");
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
set_linestyle (DiaRenderer *self, LineStyle mode, real dash_length)
{
  DxfRenderer *renderer = DXF_RENDERER(self);
  char   *style;

  switch (mode) {
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
    case LINESTYLE_DEFAULT:
    default:
      style = "CONTINUOUS";
      break;
  }
  renderer->lcurrent.style = renderer->fcurrent.style = style;
}


static void
set_fillstyle(DiaRenderer *self, FillStyle mode)
{
}


static int
dxf_color (const Color *color)
{
    /* Fixed colors
     * 0 - black ?
     * 1 - red
     * 2 - yellow
     * 3 - green
     * 4 - cyan
     * 5 - blue
     * 6 - purple
     * 7 - white
     * 8 - gray
     * ...
     */
    RGB_t rgb = {color->red*255, color->green*255, color->blue*255};
    return pal_get_index (rgb);
}

static void
draw_line(DiaRenderer *self,
	  Point *start, Point *end,
	  Color *line_colour)
{
    DxfRenderer *renderer = DXF_RENDERER(self);
    gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

    fprintf(renderer->file, "  0\nLINE\n");
    fprintf(renderer->file, "  8\n%s\n", renderer->layername);
    fprintf(renderer->file, "  6\n%s\n", renderer->lcurrent.style);
    fprintf(renderer->file, " 10\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", start->x));
    fprintf(renderer->file, " 20\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", (-1)*start->y));
    fprintf(renderer->file, " 11\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", end->x));
    fprintf(renderer->file, " 21\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", (-1)*end->y));
    fprintf(renderer->file, " 39\n%d\n", (int)(MAGIC_THICKNESS_FACTOR*renderer->lcurrent.width)); /* Thickness */
    fprintf(renderer->file, " 62\n%d\n", dxf_color (line_colour));
#if 0 /* approximately right effect, but only with one out of three DXF viewers */
    /* Lineweight given in 100th of mm */
    fprintf(renderer->file, "370\n%d\n", (int)(renderer->lcurrent.width * 1000.0));
#endif
}

static void
draw_polyline(DiaRenderer *self,
              Point *points, int num_points,
              Color *color)
{
    DxfRenderer *renderer = DXF_RENDERER(self);
    int i;
    gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
    gchar buf2[G_ASCII_DTOSTR_BUF_SIZE];

    fprintf(renderer->file, "  0\nPOLYLINE\n");
    fprintf(renderer->file, "  6\n%s\n", renderer->lcurrent.style);
    fprintf(renderer->file, "  8\n%s\n", renderer->layername);
    /* start and end width are the same */
    fprintf(renderer->file, " 41\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", renderer->lcurrent.width));
    fprintf(renderer->file, " 42\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", renderer->lcurrent.width));
    fprintf(renderer->file, " 62\n%d\n", dxf_color (color));
    /* vertices-follow flag */
    fprintf(renderer->file, " 66\n1\n");

    for (i = 0; i < num_points; ++i)
        fprintf(renderer->file, "  0\nVERTEX\n 10\n%s\n 20\n%s\n",
	        g_ascii_formatd (buf, sizeof(buf), "%g", points[i].x),
		g_ascii_formatd (buf2, sizeof(buf2), "%g", -points[i].y));

    fprintf(renderer->file, "  0\nSEQEND\n");
}

static void
draw_polygon (DiaRenderer *self,
              Point *points, int num_points,
              Color *fill, Color *stroke)
{
  Color *color = fill ? fill : stroke;
  DxfRenderer *renderer = DXF_RENDERER(self);
  /* We could emulate all polygons with multiple SOLID but it might not be
   * worth the effort. Following the easy part of polygons with 3 or 4 points.
   */
  int idx3[4] = {0, 1, 2, 2}; /* repeating last point is by specification
                                    and should not be necessary but it helps
				    limited importers */
  int idx4[4] = {0, 1, 3, 2}; /* SOLID point order differs from Dia's */
  int *idx;
  int i;
  gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar buf2[G_ASCII_DTOSTR_BUF_SIZE];

  g_return_if_fail (color != NULL);

  if (num_points == 3)
    idx = idx3;
  else if (num_points == 4)
    idx = idx4;
  else
    return; /* dont even try */
  fprintf(renderer->file, "  0\nSOLID\n");
  fprintf(renderer->file, "  8\n%s\n", renderer->layername);
  fprintf(renderer->file, " 62\n%d\n", dxf_color (color));
  for (i = 0; i < 4; ++i)
    fprintf(renderer->file, " %d\n%s\n %d\n%s\n",
            10+i, g_ascii_formatd (buf, sizeof(buf), "%g", points[idx[i]].x),
	    20+i, g_ascii_formatd (buf2, sizeof(buf2), "%g", -points[idx[i]].y));
}

static void
draw_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    DxfRenderer *renderer = DXF_RENDERER(self);
    gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
    /* DXF arcs are preferably counter-clockwise, so we might need to swap angles
     * According to my reading of the specs header section group code 70 might allow
     * clockwise arcs with $ANGDIR = 1 but it's not supported on the single arc level
     */
    if (angle2 < angle1) {
	real tmp = angle1;
	angle1 = angle2;
	angle2 = tmp;
    }
    if(width != 0.0){
	fprintf(renderer->file, "  0\nARC\n");
	fprintf(renderer->file, "  8\n%s\n", renderer->layername);
	fprintf(renderer->file, "  6\n%s\n", renderer->lcurrent.style);
	fprintf(renderer->file, " 10\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", center->x));
	fprintf(renderer->file, " 20\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", (-1)*center->y));
	fprintf(renderer->file, " 40\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", width/2)); /* radius */
	fprintf(renderer->file, " 39\n%d\n", (int)(MAGIC_THICKNESS_FACTOR*renderer->lcurrent.width)); /* Thickness */
	/* From specification: "output in degrees to DXF files". */
	fprintf(renderer->file, " 100\nAcDbArc\n");
	fprintf(renderer->file, " 50\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", (angle1 ))); /* start angle */
	fprintf(renderer->file, " 51\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", (angle2 ))); /* end angle */
    }
    fprintf(renderer->file, " 62\n%d\n", dxf_color (colour));
}

static void
fill_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    /* emulate by SOLID? */
}

static void
draw_ellipse(DiaRenderer *self,
	     Point *center,
	     real width, real height,
	     Color *fill, Color *stroke)
{
    DxfRenderer *renderer = DXF_RENDERER(self);
    gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
    Color *color = fill ? fill : stroke; /* emulate fill by SOLID? */

    /* draw a circle instead of an ellipse, if it's one */
    if(width == height){
        fprintf(renderer->file, "  0\nCIRCLE\n");
        fprintf(renderer->file, "  8\n%s\n", renderer->layername);
        fprintf(renderer->file, "  6\n%s\n", renderer->lcurrent.style);
        fprintf(renderer->file, " 10\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", center->x));
        fprintf(renderer->file, " 20\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", (-1)*center->y));
        fprintf(renderer->file, " 40\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", height/2));
        fprintf(renderer->file, " 39\n%d\n", (int)(MAGIC_THICKNESS_FACTOR*renderer->lcurrent.width)); /* Thickness */
    }
    else if(height != 0.0){
        fprintf(renderer->file, "  0\nELLIPSE\n");
        fprintf(renderer->file, "  8\n%s\n", renderer->layername);
        fprintf(renderer->file, "  6\n%s\n", renderer->lcurrent.style);
        fprintf(renderer->file, " 10\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", center->x));
        fprintf(renderer->file, " 20\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", (-1)*center->y));
        fprintf(renderer->file, " 11\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", width/2)); /* Endpoint major axis relative to center X*/
        fprintf(renderer->file, " 40\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", height/width)); /*Ratio major/minor axis*/
        fprintf(renderer->file, " 39\n%d\n", (int)(MAGIC_THICKNESS_FACTOR*renderer->lcurrent.width)); /* Thickness */
        fprintf(renderer->file, " 41\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", 0.0)); /*Start Parameter full ellipse */
        fprintf(renderer->file, " 42\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", 2.0*3.14)); /* End Parameter full ellipse */
    }
    fprintf(renderer->file, " 62\n%d\n", dxf_color (color));
}

static void
draw_string(DiaRenderer *self,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *colour)
{
    DxfRenderer *renderer = DXF_RENDERER(self);
    gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

    fprintf(renderer->file, "  0\nTEXT\n");
    fprintf(renderer->file, "  8\n%s\n", renderer->layername);
    fprintf(renderer->file, "  6\n%s\n", renderer->lcurrent.style);
    fprintf(renderer->file, " 10\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", pos->x));
    fprintf(renderer->file, " 20\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", (-1)*pos->y));
    fprintf(renderer->file, " 40\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", renderer->tcurrent.font_height)); /* Text height */
    fprintf(renderer->file, " 50\n%s\n", g_ascii_formatd (buf, sizeof(buf), "%g", 0.0)); /* Text rotation */
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
    fprintf(renderer->file, " 39\n%d\n", (int)(MAGIC_THICKNESS_FACTOR*renderer->lcurrent.width)); /* Thickness */
    fprintf(renderer->file, " 62\n%d\n", dxf_color(colour));
}

static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage *image)
{
}

static void
dxf_renderer_class_init (DxfRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = dxf_renderer_set_property;
  object_class->get_property = dxf_renderer_get_property;
  object_class->finalize = dxf_renderer_finalize;

  renderer_class->begin_render = begin_render;
  renderer_class->end_render = end_render;

  renderer_class->set_linewidth = set_linewidth;
  renderer_class->set_linecaps = set_linecaps;
  renderer_class->set_linejoin = set_linejoin;
  renderer_class->set_linestyle = set_linestyle;
  renderer_class->set_fillstyle = set_fillstyle;

  renderer_class->draw_line = draw_line;
  renderer_class->draw_polygon = draw_polygon;
  renderer_class->draw_polyline = draw_polyline;

  renderer_class->draw_arc = draw_arc;
  renderer_class->fill_arc = fill_arc;

  renderer_class->draw_ellipse = draw_ellipse;

  renderer_class->draw_string = draw_string;

  renderer_class->draw_image = draw_image;

  g_object_class_override_property (object_class, PROP_FONT, "font");
  g_object_class_override_property (object_class, PROP_FONT_HEIGHT, "font-height");
}


static gboolean
export_dxf (DiagramData *data,
            DiaContext  *ctx,
            const char  *filename,
            const char  *diafilename,
            void        *user_data)
{
  DxfRenderer *renderer;
  FILE *file;
  gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar buf2[G_ASCII_DTOSTR_BUF_SIZE];

  file = g_fopen(filename, "w");

    if (file == NULL) {
	dia_context_add_message_with_errno (ctx, errno, _("Can't open output file %s"),
					    dia_context_get_filename(ctx));
	return FALSE;
    }

    renderer = g_object_new(DXF_TYPE_RENDERER, NULL);

    renderer->file = file;

    /* drawing limits */
    fprintf(file, "  0\nSECTION\n  2\nHEADER\n");
    fprintf(file, "  9\n$EXTMIN\n 10\n%s\n 20\n%s\n",
      g_ascii_formatd (buf, sizeof(buf), "%g", data->extents.left),
      g_ascii_formatd (buf2, sizeof(buf2), "%g", -data->extents.bottom));
    fprintf(file, "  9\n$EXTMAX\n 10\n%s\n 20\n%s\n",
      g_ascii_formatd (buf, sizeof(buf), "%g", data->extents.right),
      g_ascii_formatd (buf2, sizeof(buf2), "%g", -data->extents.top));
    fprintf(file, "  0\nENDSEC\n");

    /* write layer description */
    fprintf (file,"  0\nSECTION\n  2\nTABLES\n  0\nTABLE\n");
    /* some dummy entry to make it work for more DXF viewers */
    fprintf (file,"  2\nLAYER\n 70\n255\n");

  DIA_FOR_LAYER_IN_DIAGRAM (data, layer, i, {
    fprintf (file,"  0\nLAYER\n  2\n%s\n", dia_layer_get_name (layer));
    if (dia_layer_is_visible (layer)) {
      fprintf (file, " 62\n%d\n", i + 1);
    } else {
      fprintf (file, " 62\n%d\n", (-1) * (i + 1));
    }
  });
  fprintf (file, "  0\nENDTAB\n  0\nENDSEC\n");

    /* write graphics */
    fprintf(file,"  0\nSECTION\n  2\nENTITIES\n");

    init_attributes(renderer);

    dia_renderer_begin_render (DIA_RENDERER (renderer), NULL);

  DIA_FOR_LAYER_IN_DIAGRAM (data, layer, i, {
    renderer->layername = dia_layer_get_name (layer);
    dia_layer_render (layer, DIA_RENDERER (renderer), NULL, NULL, data, 0);
  });

  dia_renderer_end_render (DIA_RENDERER (renderer));

  g_clear_object (&renderer);

  return TRUE;
}


static const gchar *extensions[] = { "dxf", NULL };
DiaExportFilter dxf_export_filter = {
    N_("Drawing Interchange File"),
    extensions,
    export_dxf
};
