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
 * - move draw_ellipse_by_arc into libdia to make it available for other
 *   interested renderers ?
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "geometry.h"
#include "diarenderer.h"
#include "filter.h"
#include "plug-ins.h"
#include "font.h"

/* #DEFINE DEBUG_HPGL */

/* format specific */
#define HPGL_MAX_PENS 8

#define PEN_HAS_COLOR (1 << 0)
#define PEN_HAS_WIDTH (1 << 1)

/* GObject boiler plate */
#define HPGL_TYPE_RENDERER           (hpgl_renderer_get_type ())
#define HPGL_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), HPGL_TYPE_RENDERER, HpglRenderer))
#define HPGL_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), HPGL_TYPE_RENDERER, HpglRendererClass))
#define HPGL_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HPGL_TYPE_RENDERER))
#define HPGL_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), HPGL_TYPE_RENDERER, HpglRendererClass))

enum {
  PROP_0,
  PROP_FONT,
  PROP_FONT_HEIGHT,
  LAST_PROP
};


GType hpgl_renderer_get_type (void) G_GNUC_CONST;

typedef struct _HpglRenderer HpglRenderer;
typedef struct _HpglRendererClass HpglRendererClass;

struct _HpglRenderer
{
  DiaRenderer parent_instance;

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

  DiaFont *font;
  real font_height;

  Point size;  /* extent size */
  real scale;
  real offset; /* in dia units */
};

struct _HpglRendererClass
{
  DiaRendererClass parent_class;
};

#ifdef DEBUG_HPGL
#  define DIAG_NOTE(action) action
#else
#  define DIAG_NOTE(action)
#endif

/* hpgl helpers */
static void
hpgl_select_pen(HpglRenderer* renderer, Color* color, real width)
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
hpgl_scale(HpglRenderer *renderer, real val)
{
    return (int)((val + renderer->offset) * renderer->scale);
}

/* render functions */
static void
begin_render(DiaRenderer *object, const DiaRectangle *update)
{
    HpglRenderer *renderer = HPGL_RENDERER (object);
    int i;

    DIAG_NOTE(g_message("begin_render"));

    /* initialize pens */
    for (i = 0; i < HPGL_MAX_PENS; i++) {
        renderer->pen[i].color = color_black;
        renderer->pen[i].width = 0.0;
        renderer->pen[i].has_it = 0;
    }
    renderer->last_pen = -1;
}

static void
end_render(DiaRenderer *object)
{
    HpglRenderer *renderer = HPGL_RENDERER (object);

    DIAG_NOTE(g_message("end_render"));
    fclose(renderer->file);
}

static void
set_linewidth(DiaRenderer *object, real linewidth)
{
    HpglRenderer *renderer = HPGL_RENDERER (object);

    DIAG_NOTE(g_message("set_linewidth %f", linewidth));

    hpgl_select_pen(renderer, NULL, linewidth);
}


static void
set_linecaps (DiaRenderer *object, DiaLineCaps mode)
{
  DIAG_NOTE (g_message ("set_linecaps %d", mode));

  switch (mode) {
    case DIA_LINE_CAPS_DEFAULT:
    case DIA_LINE_CAPS_BUTT:
      break;
    case DIA_LINE_CAPS_ROUND:
      break;
    case DIA_LINE_CAPS_PROJECTING:
      break;
    default:
      g_warning ("HpglRenderer: Unsupported fill mode specified!");
  }
}


static void
set_linejoin (DiaRenderer *object, DiaLineJoin mode)
{
  DIAG_NOTE (g_message ( "set_join %d", mode));

  switch (mode) {
    case DIA_LINE_JOIN_DEFAULT:
    case DIA_LINE_JOIN_MITER:
      break;
    case DIA_LINE_JOIN_ROUND:
      break;
    case DIA_LINE_JOIN_BEVEL:
      break;
    default:
      g_warning ("HpglRenderer : Unsupported fill mode specified!");
  }
}


static void
set_linestyle (DiaRenderer *object, DiaLineStyle mode, double dash_length)
{
  HpglRenderer *renderer = HPGL_RENDERER (object);

  DIAG_NOTE (g_message ("set_linestyle %d", mode));

  /* line type */
  switch (mode) {
    case DIA_LINE_STYLE_DEFAULT:
    case DIA_LINE_STYLE_SOLID:
      fprintf (renderer->file, "LT;\n");
      break;
    case DIA_LINE_STYLE_DASHED:
      if (dash_length > 0.5) {
        /* ??? unit of dash_lenght ? */
        fprintf (renderer->file, "LT2;\n"); /* short */
      } else {
        fprintf (renderer->file, "LT3;\n"); /* long */
      }
      break;
    case DIA_LINE_STYLE_DASH_DOT:
      fprintf (renderer->file, "LT4;\n");
      break;
    case DIA_LINE_STYLE_DASH_DOT_DOT:
      fprintf (renderer->file, "LT5;\n"); /* ??? Mittellinie? */
      break;
    case DIA_LINE_STYLE_DOTTED:
      fprintf (renderer->file, "LT1;\n");
      break;
    default:
      g_warning ("HpglRenderer : Unsupported fill mode specified!");
  }
}


static void
set_fillstyle (DiaRenderer *object, DiaFillStyle mode)
{
  DIAG_NOTE (g_message ("set_fillstyle %d", mode));

  switch (mode) {
    case DIA_FILL_STYLE_SOLID:
      break;
    default:
      g_warning ("HpglRenderer : Unsupported fill mode specified!");
    }
}


static void
set_font (DiaRenderer *object, DiaFont *font, real height)
{
  HpglRenderer *renderer = HPGL_RENDERER (object);

  DIAG_NOTE (g_message ("set_font %f", height));

  g_clear_object (&renderer->font);
  renderer->font = g_object_ref (font);
  renderer->font_height = height;
}

/* Need to translate coord system:
 *
 *   Dia x,y -> Hpgl x,-y
 *
 * doing it before scaling.
 */
static void
draw_line(DiaRenderer *object,
          Point *start, Point *end,
          Color *line_colour)
{
    HpglRenderer *renderer = HPGL_RENDERER (object);

    DIAG_NOTE(g_message("draw_line %f,%f -> %f, %f",
              start->x, start->y, end->x, end->y));
    hpgl_select_pen(renderer, line_colour, 0.0);
    fprintf (renderer->file,
             "PU%d,%d;PD%d,%d;\n",
             hpgl_scale(renderer, start->x), hpgl_scale(renderer, -start->y),
             hpgl_scale(renderer, end->x), hpgl_scale(renderer, -end->y));
}

static void
draw_polyline(DiaRenderer *object,
	      Point *points, int num_points,
	      Color *line_colour)
{
    HpglRenderer *renderer = HPGL_RENDERER (object);
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
draw_polygon(DiaRenderer *object,
	     Point *points, int num_points,
	     Color *fill, Color *stroke)
{
    Color *color = fill ? fill : stroke;
    DIAG_NOTE(g_message("draw_polygon n:%d %f,%f ...",
              num_points, points->x, points->y));
    g_return_if_fail (color != NULL);
    draw_polyline(object,points,num_points, color);
    /* last to first */
    draw_line(object, &points[num_points-1], &points[0], color);
}

static void
draw_rect(DiaRenderer *object,
	  Point *ul_corner, Point *lr_corner,
	  Color *fill, Color *stroke)
{
    Color *colour = fill ? fill : stroke;
    HpglRenderer *renderer = HPGL_RENDERER (object);

    g_return_if_fail (colour != NULL);

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
draw_arc(DiaRenderer *object,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    HpglRenderer *renderer = HPGL_RENDERER (object);
    Point start;

    DIAG_NOTE(g_message("draw_arc %fx%f <%f,<%f",
              width, height, angle1, angle2));
    hpgl_select_pen(renderer, colour, 0.0);

    /* make counter-clockwise swapping start/end */
    if (angle2 < angle1) {
	real tmp = angle1;
	angle1 = angle2;
	angle2 = tmp;
    }
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
fill_arc(DiaRenderer *object,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    HpglRenderer *renderer = HPGL_RENDERER (object);

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

/* may go into lib/diarenderer.c if another renderer would be interested */
/* Draw an ellipse approximation consisting out of
 * four arcs.
 */
static void
draw_ellipse_by_arc (DiaRenderer *renderer,
                     Point *center,
                     real width, real height,
                     Color *colour)
{
  real a, b, e, d, alpha, c, x, y;
  real g, gamma, r;
  Point pt;
  real angle;

  a = width / 2;
  b = height / 2;
  e = sqrt(a*a - b*b);

  alpha = 0.25*M_PI - dia_asin((e/a) * sin(0.75*M_PI));
  d = 2*a*sin(alpha);

  c = (sin(0.25*M_PI) * (2*e + d)) / sin(0.75*M_PI - alpha);

  y = c * sin (alpha);
  x = c * cos (alpha) - e;

  /* draw arcs */
  g = sqrt((a-x)*(a-x) + y*y);
  gamma = dia_acos((a-x)/g);
  r = (sin(gamma) * g) / sin(M_PI-2*gamma);

  pt.y = center->y;
  angle = (180 * (M_PI-2*gamma)) / M_PI;
  pt.x = center->x + a - r; /* right */
  draw_arc(renderer, &pt, 2*r, 2*r, 360-angle, angle, colour);
  pt.x = center->x - a + r; /* left */
  draw_arc(renderer, &pt, 2*r, 2*r, 180-angle, 180+angle, colour);


  g = sqrt((b-y)*(b-y) + x*x);
  gamma = dia_acos((b-y)/g);
  r = (sin(gamma) * g) / sin(M_PI-2*gamma);

  pt.x = center->x;
  angle = (180 * (M_PI-2*gamma)) / M_PI;
  pt.y = center->y - b + r; /* top */
  draw_arc(renderer, &pt, 2*r, 2*r, 90-angle, 90+angle, colour);
  pt.y = center->y + b - r; /* bottom */
  draw_arc(renderer, &pt, 2*r, 2*r, 270-angle, 270+angle, colour);
}

static void
draw_ellipse(DiaRenderer *object,
	     Point *center,
	     real width, real height,
	     Color *fill, Color *stroke)
{
  HpglRenderer *renderer = HPGL_RENDERER (object);
  Color *colour = fill ? fill : stroke;

  DIAG_NOTE(g_message("draw_ellipse %fx%f center @ %f,%f",
            width, height, center->x, center->y));

  if (width != height)
    {
      draw_ellipse_by_arc(object, center, width, height, colour);
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
draw_string (DiaRenderer  *object,
             const char   *text,
             Point        *pos,
             DiaAlignment  alignment,
             Color        *colour)
{
  HpglRenderer *renderer = HPGL_RENDERER (object);
  real width, height;

  DIAG_NOTE (g_message ("draw_string %f,%f %s",
                        pos->x, pos->y, text));

  /* set position */
  fprintf (renderer->file, "PU%d,%d;",
           hpgl_scale (renderer, pos->x), hpgl_scale (renderer, -pos->y));

  switch (alignment) {
    case DIA_ALIGN_LEFT:
      fprintf (renderer->file, "LO1;\n");
      break;
    case DIA_ALIGN_CENTRE:
      fprintf (renderer->file, "LO4;\n");
      break;
    case DIA_ALIGN_RIGHT:
      fprintf (renderer->file, "LO7;\n");
      break;
    default:
      g_return_if_reached ();
  }

  hpgl_select_pen (renderer, colour, 0.0);

#if 0
    /*
     * SR - Relative Character Size >0.0 ... 127.999
     *    set the capital letter box width and height as a percentage of
     *    P2X-P1X  and P2Y-P1Y
     */
    height = (127.999 * renderer->font_height * renderer->scale) / renderer->size.y;
    width  = 0.75 * height; /* FIXME: */
    fprintf(renderer->file, "SR%d.%03d,%d.%03d;",
            (int)width, (int)((width * 1000) % 1000),
            (int)height, (int)((height * 1000) % 1000));
#else
    /*
     * SI - character size absolute
     *    size needed in centimeters
     */
    width = renderer->font_height * renderer->scale * 0.75 * 0.0025;
    height = renderer->font_height * renderer->scale * 0.0025;
    fprintf(renderer->file, "SI%d.%03d,%d.%03d;",
            (int)width, ((int)(width * 1000) % 1000),
            (int)height, ((int)(height * 1000) % 1000));
#endif
    fprintf(renderer->file, "DT\003;" /* Terminator */
            "LB%s\003;\n", text);
}

static void
draw_image(DiaRenderer *object,
	   Point *point,
	   real width, real height,
	   DiaImage *image)
{
    DIAG_NOTE(g_message("draw_image %fx%f @%f,%f",
              width, height, point->x, point->y));
    g_warning("HPGL: images unsupported!");
}

/* overwrite vtable */
static void hpgl_renderer_class_init (HpglRendererClass *klass);

static gpointer parent_class = NULL;

GType
hpgl_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (HpglRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) hpgl_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (HpglRenderer),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "HpglRenderer",
                                            &object_info, 0);
    }

  return object_type;
}

static void
hpgl_renderer_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  HpglRenderer *self = HPGL_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      set_font (DIA_RENDERER (self),
                DIA_FONT (g_value_get_object (value)),
                self->font_height);
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
hpgl_renderer_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  HpglRenderer *self = HPGL_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      g_value_set_object (value, self->font);
      break;
    case PROP_FONT_HEIGHT:
      g_value_set_double (value, self->font_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
hpgl_renderer_finalize (GObject *object)
{
  HpglRenderer *self = HPGL_RENDERER (object);

  g_clear_object (&self->font);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
hpgl_renderer_class_init (HpglRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->get_property = hpgl_renderer_get_property;
  object_class->set_property = hpgl_renderer_set_property;
  object_class->finalize = hpgl_renderer_finalize;

  /* renderer members */
  renderer_class->begin_render = begin_render;
  renderer_class->end_render   = end_render;

  renderer_class->set_linewidth  = set_linewidth;
  renderer_class->set_linecaps   = set_linecaps;
  renderer_class->set_linejoin   = set_linejoin;
  renderer_class->set_linestyle  = set_linestyle;
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->draw_line    = draw_line;
  renderer_class->draw_polygon = draw_polygon;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;

  renderer_class->draw_string  = draw_string;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_rect = draw_rect;
  renderer_class->draw_polyline  = draw_polyline;

  g_object_class_override_property (object_class, PROP_FONT, "font");
  g_object_class_override_property (object_class, PROP_FONT_HEIGHT, "font-height");
}

/* plug-in interface : export function */
static gboolean
export_data(DiagramData *data, DiaContext *ctx,
	    const gchar *filename, const gchar *diafilename,
	    void* user_data)
{
    HpglRenderer *renderer;
    FILE *file;
    DiaRectangle *extent;
    real width, height;

    file = g_fopen(filename, "w"); /* "wb" for binary! */

    if (!file) {
	dia_context_add_message_with_errno(ctx, errno, _("Can't open output file %s."),
					   dia_context_get_filename(ctx));
	return FALSE;
    }

    renderer = g_object_new(HPGL_TYPE_RENDERER, NULL);

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
  data_render(data, DIA_RENDERER(renderer), NULL, NULL, NULL);

  g_clear_object (&renderer);

  return TRUE;
}

static const gchar *extensions[] = { "plt", "hpgl", NULL };
static DiaExportFilter my_export_filter = {
    N_("HP Graphics Language"),
    extensions,
    export_data
};


/* --- dia plug-in interface --- */
static gboolean
_plugin_can_unload (PluginInfo *info)
{
    return TRUE;
}

static void
_plugin_unload (PluginInfo *info)
{
    filter_unregister_export(&my_export_filter);
}

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
    if (!dia_plugin_info_init(info, "HPGL",
                              _("HP Graphics Language export filter"),
			      _plugin_can_unload,
                              _plugin_unload))
	return DIA_PLUGIN_INIT_ERROR;

    filter_register_export(&my_export_filter);

    return DIA_PLUGIN_INIT_OK;
}
