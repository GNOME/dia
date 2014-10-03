/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diagdkrenderer.c - refactoring of the render to gdk facility
 * Copyright (C) 2002 Hans Breuer
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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdk.h>

#define PANGO_ENABLE_ENGINE
#include <pango/pango-engine.h>
#include <pango/pango.h>

#include "diagdkrenderer.h"
#include "dia_image.h"
#include "color.h"
#include "font.h"
#include "text.h"
#include "textline.h"

#include "time.h"

#ifdef HAVE_FREETYPE
#include <pango/pango.h>
#include <pango/pangoft2.h>
#endif


static int get_width_pixels (DiaRenderer *);
static int get_height_pixels (DiaRenderer *);

static void begin_render (DiaRenderer *, const Rectangle *update);
static void end_render (DiaRenderer *);

static void set_linewidth (DiaRenderer *renderer, real linewidth);
static void set_linecaps (DiaRenderer *renderer, LineCaps mode);
static void set_linejoin (DiaRenderer *renderer, LineJoin mode);
static void set_linestyle (DiaRenderer *renderer, LineStyle mode, real length);
static void set_fillstyle (DiaRenderer *renderer, FillStyle mode);

static void draw_line (DiaRenderer *renderer,
                       Point *start, Point *end,
                       Color *color);
static void draw_arc (DiaRenderer *renderer,
                      Point *center,
                      real width, real height,
                      real angle1, real angle2,
                      Color *color);
static void fill_arc (DiaRenderer *renderer,
                      Point *center,
                      real width, real height,
                      real angle1, real angle2,
                      Color *color);
static void draw_ellipse (DiaRenderer *renderer,
                          Point *center,
                          real width, real height,
                          Color *fill, Color *stroke);
static void draw_string (DiaRenderer *renderer,
                         const gchar *text,
                         Point *pos,
                         Alignment alignment,
                         Color *color);
static void draw_text_line (DiaRenderer *renderer,
			    TextLine *text,
			    Point *pos,
			    Alignment alignment,
			    Color *color);
static void draw_image (DiaRenderer *renderer,
                        Point *point,
                        real width, real height,
                        DiaImage *image);

static void draw_rect (DiaRenderer *renderer,
                       Point *ul_corner, Point *lr_corner,
                       Color *fill, Color *stroke);
static void draw_polyline (DiaRenderer *renderer,
                           Point *points, int num_points,
                           Color *color);
static void draw_polygon (DiaRenderer *renderer,
                          Point *points, int num_points,
                          Color *fill, Color *stroke);

static real get_text_width (DiaRenderer *renderer,
                            const gchar *text, int length);

static void dia_gdk_renderer_class_init (DiaGdkRendererClass *klass);
static void renderer_init (DiaGdkRenderer *renderer, void*);

static gpointer parent_class = NULL;

/** Get the type object for the GdkRenderer.
 * @return A static GType object describing GdkRenderers.
 */
GType
dia_gdk_renderer_get_type(void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (DiaGdkRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) dia_gdk_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (DiaGdkRenderer),
        0,              /* n_preallocs */
        (GInstanceInitFunc)renderer_init            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "DiaGdkRenderer",
                                            &object_info, 0);
    }
  
  return object_type;
}

/** Initialize a renderer object.
 * @param renderer A renderer object to initialize.
 * @param p Ignored, purpose unknown.
 */
static void
renderer_init(DiaGdkRenderer *renderer, void* p)
{
  renderer->line_width = 1;
  renderer->line_style = GDK_LINE_SOLID;
  renderer->cap_style = GDK_CAP_BUTT;
  renderer->join_style = GDK_JOIN_ROUND;

  renderer->saved_line_style = LINESTYLE_SOLID;
  renderer->dash_length = 10;
  renderer->dot_length = 2;

  renderer->highlight_color = NULL;
  
  renderer->current_alpha = 1.0;
}

/** Clean up a renderer object after use.
 * @param object Renderer object to free subobjects from.
 */
static void
renderer_finalize(GObject *object)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);

  if (renderer->pixmap != NULL)
    g_object_unref(renderer->pixmap);

  if (renderer->gc != NULL)
    g_object_unref(renderer->gc);

  if (renderer->clip_region != NULL)
    gdk_region_destroy(renderer->clip_region);

  if (renderer->transform)
    g_object_unref (renderer->transform);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/** Initialize members of the renderer class object.  This sets up a bunch
 * of functions to call for various render functions.
 * @param klass The class object to initialize. 
 */
static void
dia_gdk_renderer_class_init(DiaGdkRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = renderer_finalize;

  renderer_class->get_width_pixels  = get_width_pixels;
  renderer_class->get_height_pixels = get_height_pixels;

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

  /* use <draw|fill>_bezier from DiaRenderer */

  renderer_class->draw_string  = draw_string;
  renderer_class->draw_text_line = draw_text_line;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_rect    = draw_rect;
  renderer_class->draw_polyline  = draw_polyline;

  /* Interactive functions */
  renderer_class->get_text_width = get_text_width;
}

/** Convert Dia color objects into GDK color objects.
 * If the highlight color is set, that will be used instead.  This allows
 * rendering of an object to do highlight rendering.
 * @param renderer The renderer to check for highlight color.
 * @param col A color object to convert.
 * @param gdk_col Resulting GDK convert.
 */
static void
renderer_color_convert(DiaGdkRenderer *renderer,
		       Color *col, GdkColor *gdk_col)
{
  if (renderer->highlight_color != NULL) {
    color_convert(renderer->highlight_color, gdk_col);
  } else {
    color_convert(col, gdk_col);
  }
  if (col->alpha != renderer->current_alpha) {
    if (col->alpha == 1.0)
      gdk_gc_set_fill(renderer->gc, GDK_SOLID);
    else {
      static gchar bits[9][4] = {
        { 0x00, 0x00, 0x00, 0x00 }, /*   0% */
	{ 0x20, 0x02, 0x20, 0x02 },
        { 0x22, 0x88, 0x22, 0x88 }, /*  25% */
	{ 0x4A, 0xA4, 0x4A, 0xA4 },
        { 0x5A, 0xA5, 0x5A, 0xA5 }, /*  50% */
	{ 0x57, 0xBA, 0x57, 0xBA },
        { 0xBE, 0xEB, 0xBE, 0xEB }, /*  75% */
	{ 0xEF, 0xFE, 0xEF, 0xFE },
        { 0xFF, 0xFF, 0xFF, 0xFF }, /* 100% */
      };
      GdkBitmap *stipple = gdk_bitmap_create_from_data (NULL, bits[(int)(9*col->alpha+.49)], 4, 4);
      gdk_gc_set_stipple (renderer->gc, stipple);
      g_object_unref (stipple);
      gdk_gc_set_fill(renderer->gc, GDK_STIPPLED);
    }
    renderer->current_alpha = col->alpha;
  }
}

static void 
begin_render (DiaRenderer *object, const Rectangle *update)
{
}

static void 
end_render (DiaRenderer *object)
{
}

static void 
set_linewidth (DiaRenderer *object, real linewidth)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);

  if (renderer->highlight_color != NULL) {
    /* 6 pixels wide -> 3 pixels beyond normal obj */
    real border = dia_untransform_length(renderer->transform, 6);
    linewidth += border;
  } 

  /* 0 == hairline **/
  renderer->line_width =
    dia_transform_length(renderer->transform, linewidth);

  if (renderer->line_width<=0)
    renderer->line_width = 1; /* Minimum 1 pixel. */

  gdk_gc_set_line_attributes(renderer->gc,
			     renderer->line_width,
			     renderer->line_style,
			     renderer->cap_style,
			     renderer->join_style);
}

static void 
set_linecaps (DiaRenderer *object, LineCaps mode)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);

  if (renderer->highlight_color != NULL) {
    renderer->cap_style = GDK_CAP_ROUND;
  } else {
    switch(mode) {
    case LINECAPS_DEFAULT:
    case LINECAPS_BUTT:
      renderer->cap_style = GDK_CAP_BUTT;
      break;
    case LINECAPS_ROUND:
      renderer->cap_style = GDK_CAP_ROUND;
      break;
    case LINECAPS_PROJECTING:
      renderer->cap_style = GDK_CAP_PROJECTING;
      break;
    }
  }
 
  gdk_gc_set_line_attributes(renderer->gc,
			     renderer->line_width,
			     renderer->line_style,
			     renderer->cap_style,
			     renderer->join_style);
}

static void 
set_linejoin (DiaRenderer *object, LineJoin mode)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);

  if (renderer->highlight_color != NULL) {
    renderer->join_style = GDK_JOIN_ROUND;
  } else {
    switch(mode) {
    case LINEJOIN_DEFAULT:
    case LINEJOIN_MITER:
      renderer->join_style = GDK_JOIN_MITER;
      break;
    case LINEJOIN_ROUND:
      renderer->join_style = GDK_JOIN_ROUND;
      break;
    case LINEJOIN_BEVEL:
      renderer->join_style = GDK_JOIN_BEVEL;
      break;
    default :
      /* invalid mode, just here to set a breakpoint */
      renderer->join_style = GDK_JOIN_ROUND;
      break;
    }
  }
 
  gdk_gc_set_line_attributes(renderer->gc,
			     renderer->line_width,
			     renderer->line_style,
			     renderer->cap_style,
			     renderer->join_style);
}

/** Set the dashes for this renderer.
 * offset determines where in the pattern the dashes will start.
 * It is used by the grid in particular to make the grid dashes line up.
 */
void
dia_gdk_renderer_set_dashes(DiaGdkRenderer *renderer, int offset)
{
  gint8 dash_list[6];
  int hole_width;
  
  switch(renderer->saved_line_style) {
  case LINESTYLE_DEFAULT:
  case LINESTYLE_SOLID:
    break;
  case LINESTYLE_DASHED:
    dash_list[0] = renderer->dash_length;
    dash_list[1] = renderer->dash_length;
    gdk_gc_set_dashes(renderer->gc, offset, dash_list, 2);
    break;
  case LINESTYLE_DASH_DOT:
    hole_width = (renderer->dash_length - renderer->dot_length) / 2;
    if (hole_width==0)
      hole_width = 1;
    dash_list[0] = renderer->dash_length;
    dash_list[1] = hole_width;
    dash_list[2] = renderer->dot_length;
    dash_list[3] = hole_width;
    gdk_gc_set_dashes(renderer->gc, offset, dash_list, 4);
    break;
  case LINESTYLE_DASH_DOT_DOT:
    hole_width = (renderer->dash_length - 2*renderer->dot_length) / 3;
    if (hole_width==0)
      hole_width = 1;
    dash_list[0] = renderer->dash_length;
    dash_list[1] = hole_width;
    dash_list[2] = renderer->dot_length;
    dash_list[3] = hole_width;
    dash_list[4] = renderer->dot_length;
    dash_list[5] = hole_width;
    gdk_gc_set_dashes(renderer->gc, offset, dash_list, 6);
    break;
  case LINESTYLE_DOTTED:
    dash_list[0] = renderer->dot_length;
    dash_list[1] = renderer->dot_length;
    gdk_gc_set_dashes(renderer->gc, offset, dash_list, 2);
    break;
  }
}

static void 
set_linestyle (DiaRenderer *object, LineStyle mode, real length)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);
  /* dot = 10% of len */
  real ddisp_len;

  ddisp_len =
    dia_transform_length(renderer->transform, length);
  
  renderer->dash_length = (int)floor(ddisp_len+0.5);
  renderer->dot_length = (int)floor(ddisp_len*0.1+0.5);

  if (renderer->dash_length<=0)
    renderer->dash_length = 1;
  if (renderer->dash_length>255)
    renderer->dash_length = 255;
  if (renderer->dot_length<=0)
    renderer->dot_length = 1;
  if (renderer->dot_length>255)
    renderer->dot_length = 255;

  renderer->saved_line_style = mode;
  switch(mode) {
  case LINESTYLE_DEFAULT:
  case LINESTYLE_SOLID:
    renderer->line_style = GDK_LINE_SOLID;
    break;
  case LINESTYLE_DASHED:
    renderer->line_style = GDK_LINE_ON_OFF_DASH;
    dia_gdk_renderer_set_dashes(renderer, 0);
    break;
  case LINESTYLE_DASH_DOT:
    renderer->line_style = GDK_LINE_ON_OFF_DASH;
    dia_gdk_renderer_set_dashes(renderer, 0);
    break;
  case LINESTYLE_DASH_DOT_DOT:
    renderer->line_style = GDK_LINE_ON_OFF_DASH;
    dia_gdk_renderer_set_dashes(renderer, 0);
    break;
  case LINESTYLE_DOTTED:
    renderer->line_style = GDK_LINE_ON_OFF_DASH;
    dia_gdk_renderer_set_dashes(renderer, 0);
    break;
  }
  gdk_gc_set_line_attributes(renderer->gc,
			     renderer->line_width,
			     renderer->line_style,
			     renderer->cap_style,
			     renderer->join_style);
}

static void 
set_fillstyle (DiaRenderer *object, FillStyle mode)
{
  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    g_warning("gdk_renderer: Unsupported fill mode specified!\n");
  }
}

static void 
draw_line (DiaRenderer *object, Point *start, Point *end, Color *line_color)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);

  GdkGC *gc = renderer->gc;
  GdkColor color;
  int x1,y1,x2,y2;

  if (line_color->alpha == 0.0)
    return;

  dia_transform_coords(renderer->transform, start->x, start->y, &x1, &y1);
  dia_transform_coords(renderer->transform, end->x, end->y, &x2, &y2);
  
  renderer_color_convert(renderer, line_color, &color);
  gdk_gc_set_foreground(gc, &color);
  
  gdk_draw_line(renderer->pixmap, gc,
		x1, y1,	x2, y2);
}

static void 
draw_polygon (DiaRenderer *object, Point *points, int num_points,
	      Color *fill, Color *stroke)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);
  GdkGC *gc = renderer->gc;
  GdkColor color;
  GdkPoint *gdk_points;
  int i,x,y;

  gdk_points = g_new(GdkPoint, num_points);

  for (i=0;i<num_points;i++) {
    dia_transform_coords(renderer->transform, points[i].x, points[i].y, &x, &y);
    gdk_points[i].x = x;
    gdk_points[i].y = y;
  }

  if (fill && fill->alpha > 0.0) {
    renderer_color_convert(renderer, fill, &color);
    gdk_gc_set_foreground(gc, &color);
  
    gdk_draw_polygon(renderer->pixmap, gc, TRUE, gdk_points, num_points);
  }
  if (stroke && stroke->alpha > 0.0) {
    renderer_color_convert(renderer, stroke, &color);
    gdk_gc_set_foreground(gc, &color);
  
    gdk_draw_polygon(renderer->pixmap, gc, FALSE, gdk_points, num_points);
  }
  g_free(gdk_points);
}

static void
draw_fill_arc (DiaRenderer *object, 
          Point *center, 
          real width, real height,
          real angle1, real angle2,
          Color *color,
          gboolean fill)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);
  GdkGC *gc = renderer->gc;
  GdkColor gdkcolor;
  gint top, left, bottom, right;
  real dangle;
  gboolean counter_clockwise = angle2 > angle1;

  if (color->alpha == 0.0)
    return;

  dia_transform_coords(renderer->transform,
                       center->x - width/2, center->y - height/2,
                       &left, &top);
  dia_transform_coords(renderer->transform,
                       center->x + width/2, center->y + height/2,
                       &right, &bottom);
  
  if ((left>right) || (top>bottom))
    return;

  renderer_color_convert(renderer, color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);

  /* GDK wants it always counter-clockwise */
  if (counter_clockwise)
    dangle = angle2-angle1;
  else
    dangle = angle1-angle2;
  if (dangle<0)
    dangle += 360.0;

  gdk_draw_arc(renderer->pixmap,
	       gc, fill,
	       left, top, right-left, bottom-top,
	       (int) (counter_clockwise ? angle1 : angle2) * 64.0,
	       (int) (dangle * 64.0));
}
static void 
draw_arc (DiaRenderer *object, 
          Point *center, 
          real width, real height,
          real angle1, real angle2,
          Color *color)
{
  draw_fill_arc (object, center, width, height, angle1, angle2, color, FALSE);
}

static void 
fill_arc (DiaRenderer *object, Point *center,
          real width, real height, real angle1, real angle2,
          Color *color)
{
  draw_fill_arc (object, center, width, height, angle1, angle2, color, TRUE);
}

static void 
draw_ellipse (DiaRenderer *object, Point *center,
              real width, real height, 
              Color *fill, Color *stroke)
{
  if (fill && fill->alpha > 0.0)
    fill_arc(object, center, width, height, 0.0, 360.0, fill);
  if (stroke && stroke->alpha > 0.0)    
    draw_arc(object, center, width, height, 0.0, 360.0, stroke);
}

/* Draw a highlighted version of a string.
 */
static void
draw_highlighted_string(DiaGdkRenderer *renderer,
			PangoLayout *layout,
			int x, int y,
			GdkColor *color)
{
  gint width, height;

  pango_layout_get_pixel_size(layout, &width, &height);

  gdk_gc_set_foreground(renderer->gc, color);

  gdk_draw_rectangle (renderer->pixmap,
		      renderer->gc, TRUE,
		      x-3, y-3,
		      width+6, height+6);
}

static void 
draw_string (DiaRenderer *object,
             const gchar *text, Point *pos, Alignment alignment,
             Color *color)
{
  TextLine *text_line = text_line_new(text, object->font, object->font_height);
  draw_text_line(object, text_line, pos, alignment, color);
  text_line_destroy(text_line);
}

#ifdef HAVE_FREETYPE
static void
initialize_ft_bitmap(FT_Bitmap *ftbitmap, int width, int height)
{
  int rowstride = 32*((width+31)/31);
  guint8 *graybitmap = (guint8*)g_new0(guint8, height*rowstride);
       
  ftbitmap->rows = height;
  ftbitmap->width = width;
  ftbitmap->pitch = rowstride;
  ftbitmap->buffer = graybitmap;
  ftbitmap->num_grays = 256;
  ftbitmap->pixel_mode = ft_pixel_mode_grays;
  ftbitmap->palette_mode = 0;
  ftbitmap->palette = 0;
}
#endif

/** Draw a TextLine object.
 * @param object The renderer object to use for transform and output
 * @param text_line The TextLine to render, including font and height.
 * @param pos The position to render it at.
 * @param color The color to render it with.
 */
static void 
draw_text_line (DiaRenderer *object, TextLine *text_line,
		Point *pos, Alignment alignment, Color *color)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);
  GdkColor gdkcolor;
  int x,y;
  Point start_pos;
  PangoLayout* layout = NULL;
  const gchar *text = text_line_get_string(text_line);
  int height_pixels;
  real font_height = text_line_get_height(text_line);
  real scale = dia_transform_length(renderer->transform, 1.0);

  if (text == NULL || *text == '\0') return; /* Don't render empty strings. */

  point_copy(&start_pos,pos);

  renderer_color_convert(renderer, color, &gdkcolor);
 
  height_pixels = dia_transform_length(renderer->transform, font_height);
  if (height_pixels < 2) { /* "Greeking" instead of making tiny font */
    int width_pixels = dia_transform_length(renderer->transform,
					    text_line_get_width(text_line));
    gdk_gc_set_foreground(renderer->gc, &gdkcolor);
    gdk_gc_set_dashes(renderer->gc, 0, (gint8*)"\1\2", 2);
    dia_transform_coords(renderer->transform, start_pos.x, start_pos.y, &x, &y);
    gdk_draw_line(renderer->pixmap, renderer->gc, x, y, x + width_pixels, y);
    return;
  } else {
    start_pos.y -= text_line_get_ascent(text_line);
    start_pos.x -= text_line_get_alignment_adjustment (text_line, alignment);
  
    dia_transform_coords(renderer->transform, 
			 start_pos.x, start_pos.y, &x, &y);

    layout = dia_font_build_layout(text, text_line->font,
				   dia_transform_length(renderer->transform, text_line->height)/20.0);
#if defined(PANGO_VERSION_ENCODE)
#  if (PANGO_VERSION >= PANGO_VERSION_ENCODE(1,16,0))
    /* I'd say the former Pango API was broken, i.e. leaky */
#    define HAVE_pango_layout_get_line_readonly
#   endif
#endif
    text_line_adjust_layout_line (text_line, 
#if defined(HAVE_pango_layout_get_line_readonly)
                                  pango_layout_get_line_readonly(layout, 0),
#else
                                  pango_layout_get_line(layout, 0),
#endif
				  scale/20.0);

    if (renderer->highlight_color != NULL) {
      draw_highlighted_string(renderer, layout, x, y, &gdkcolor);
    } else {
#if defined HAVE_FREETYPE
      {
	FT_Bitmap ftbitmap;
	int width, height;
	GdkPixbuf *rgba = NULL;
	
	width = dia_transform_length(renderer->transform,
				     text_line_get_width(text_line));
	height = dia_transform_length(renderer->transform, 
				      text_line_get_height(text_line));
	
	if (width > 0) {
	  int stride;
	  guchar* pixels;
	  int i,j;
	  guint8 *graybitmap;

	  initialize_ft_bitmap(&ftbitmap, width, height);
	  pango_ft2_render_layout(&ftbitmap, layout, 0, 0);
	  
	  graybitmap = ftbitmap.buffer;
	  
	  rgba = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
	  stride = gdk_pixbuf_get_rowstride(rgba);
	  pixels = gdk_pixbuf_get_pixels(rgba);
	  for (i = 0; i < height; i++) {
	    for (j = 0; j < width; j++) {
	      pixels[i*stride+j*4] = gdkcolor.red>>8;
	      pixels[i*stride+j*4+1] = gdkcolor.green>>8;
	      pixels[i*stride+j*4+2] = gdkcolor.blue>>8;
	      pixels[i*stride+j*4+3] = graybitmap[i*ftbitmap.pitch+j];
	    }
	  }
	  g_free(graybitmap);

	  gdk_draw_pixbuf(renderer->pixmap, renderer->gc, rgba, 0, 0, x, y, width, height, GDK_RGB_DITHER_NONE, 0, 0);

	  g_object_unref(G_OBJECT(rgba));
	}
      }
#else
      gdk_gc_set_foreground(renderer->gc, &gdkcolor);
	
      gdk_draw_layout(renderer->pixmap, renderer->gc, x, y, layout);
#endif
    } /* !higlight_color */
    g_object_unref(G_OBJECT(layout));
  } /* !greeking */
}

/* Get the width of the given text in cm */
static real
get_text_width(DiaRenderer *object,
               const gchar *text, int length)
{
  real result;
  TextLine *text_line;

  if (length != g_utf8_strlen(text, -1)) {
    char *othertx;
    int ulen;
    /* A couple UTF8-chars: æblegrød Š Ť Ž ę ć ń уфхцНОПРЄ є Ґ Њ Ћ Џ */
    ulen = g_utf8_offset_to_pointer(text, length)-text;
    if (!g_utf8_validate(text, ulen, NULL)) {
      g_warning ("Text at char %d not valid\n", length);
    }
    othertx = g_strndup(text, ulen);
    text_line = text_line_new(othertx, object->font, object->font_height);
  } else {
    text_line = text_line_new(text, object->font, object->font_height);
  }
  result = text_line_get_width(text_line);
  text_line_destroy(text_line);
  return result;
}

static void 
draw_image (DiaRenderer *object,
            Point *point, 
            real width, real height,
            DiaImage *image)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);
  if (renderer->highlight_color != NULL) {
    Point lr;
    DiaRendererClass *self_class = DIA_RENDERER_GET_CLASS (object);

    lr = *point;
    lr.x += width;
    lr.y += height;
    self_class->draw_rect(object, point, &lr, renderer->highlight_color, NULL);
  } else {
    int real_width, real_height, real_x, real_y;
    const GdkPixbuf *org = dia_image_pixbuf (image);
    int org_width = gdk_pixbuf_get_width(org);
    int org_height = gdk_pixbuf_get_height(org);
    
    real_width = ROUND(dia_transform_length(renderer->transform, width));
    real_height = ROUND(dia_transform_length(renderer->transform, height));
    dia_transform_coords(renderer->transform, point->x, point->y,
			 &real_x, &real_y);

    if (real_width == org_width && real_height == org_height) {
      gdk_draw_pixbuf(renderer->pixmap, renderer->gc, (GdkPixbuf *)org,
		      0, 0, real_x, real_y, real_width, real_height, 
		      GDK_RGB_DITHER_NORMAL, 0, 0);
    } else if (real_width > org_width || real_height > org_height) {
      /* don't use dia_image_draw for big zooms, it scales the whole pixbuf even if not needed */
      int sub_width = real_width - (real_x >= 0 ? 0 : -real_x);
      int sub_height = real_height - (real_y >= 0 ? 0 : -real_y);

      /* we can also clip to our pixmap size */
      if (get_width_pixels (object) < sub_width)
	sub_width = get_width_pixels (object);
      if (get_height_pixels (object) < sub_height)
	sub_height = get_height_pixels (object);

      if (sub_height > 0 && sub_width > 0) {
        GdkPixbuf *scaled = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (org),
                                            gdk_pixbuf_get_has_alpha (org),
					    gdk_pixbuf_get_bits_per_sample (org),
					    sub_width, sub_height);
        double scale_x = (double)real_width/org_width;
        double scale_y = (double)real_height/org_height;
        gdk_pixbuf_scale (org, scaled,
                          0, 0, sub_width, sub_height,
			  real_x >= 0 ? 0 : real_x, real_y >= 0 ? 0 : real_y,
			  scale_x, scale_y, GDK_INTERP_TILES);
        gdk_draw_pixbuf(renderer->pixmap, renderer->gc, scaled,
		        0, 0, real_x >= 0 ? real_x : 0, real_y >= 0 ? real_y : 0, 
		        sub_width, sub_height, GDK_RGB_DITHER_NORMAL, 0, 0);
        g_object_unref (scaled);
      }
    } else {
      /* otherwise still using the caching variant */
      GdkPixbuf *scaled = dia_image_get_scaled_pixbuf (image, real_width, real_height);
      if (scaled) {
	gdk_draw_pixbuf(renderer->pixmap, renderer->gc, scaled,
		        0, 0, real_x, real_y, real_width, real_height, 
		        GDK_RGB_DITHER_NORMAL, 0, 0);

        g_object_unref (scaled);
      }
    }
  }
}

/*
 * medium level functions
 */
static void
draw_rect (DiaRenderer *self,
	   Point *ul_corner, Point *lr_corner,
	   Color *fill, Color *stroke)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (self);
  GdkGC *gc = renderer->gc;
  GdkColor gdkcolor;
  gint top, bottom, left, right;
    
  dia_transform_coords(renderer->transform, 
                       ul_corner->x, ul_corner->y, &left, &top);
  dia_transform_coords(renderer->transform, 
                       lr_corner->x, lr_corner->y, &right, &bottom);
  
  if ((left>right) || (top>bottom))
    return;

  if (fill && fill->alpha > 0.0) {
    renderer_color_convert(renderer, fill, &gdkcolor);
    gdk_gc_set_foreground(gc, &gdkcolor);

    gdk_draw_rectangle (renderer->pixmap, gc, TRUE,
			left, top, right-left, bottom-top);
  }
  if (stroke && stroke->alpha > 0.0) {
    renderer_color_convert(renderer, stroke, &gdkcolor);
    gdk_gc_set_foreground(gc, &gdkcolor);

    gdk_draw_rectangle (renderer->pixmap, gc, FALSE,
			left, top, right-left, bottom-top);
  }
}

static void 
draw_polyline (DiaRenderer *self,
               Point *points, int num_points,
               Color *line_color)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (self);
  GdkGC *gc = renderer->gc;
  GdkColor color;
  GdkPoint *gdk_points;
  int i,x,y;

  if (line_color->alpha == 0.0)
    return;

  gdk_points = g_new(GdkPoint, num_points);

  for (i=0;i<num_points;i++) {
    dia_transform_coords(renderer->transform, points[i].x, points[i].y, &x, &y);
    gdk_points[i].x = x;
    gdk_points[i].y = y;
  }
  
  renderer_color_convert(renderer, line_color, &color);
  gdk_gc_set_foreground(gc, &color);
  
  gdk_draw_lines(renderer->pixmap, gc, gdk_points, num_points);
  g_free(gdk_points);
}

static int
get_width_pixels (DiaRenderer *object)
{ 
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);
  int width = 0;

  if (renderer->pixmap)
    gdk_drawable_get_size (GDK_DRAWABLE (renderer->pixmap), &width, NULL);

  return width;
}

static int
get_height_pixels (DiaRenderer *object)
{ 
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);
  int height = 0;

  if (renderer->pixmap)
    gdk_drawable_get_size (GDK_DRAWABLE (renderer->pixmap), NULL, &height);

  return height;
}
