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
#include "message.h"
#include "color.h"
#include "font.h"
#include "text.h"

#ifdef HAVE_FREETYPE
#include <pango/pango.h>
#include <pango/pangoft2.h>
#endif

static int get_width_pixels (DiaRenderer *);
static int get_height_pixels (DiaRenderer *);

static void begin_render (DiaRenderer *);
static void end_render (DiaRenderer *);

static void set_linewidth (DiaRenderer *renderer, real linewidth);
static void set_linecaps (DiaRenderer *renderer, LineCaps mode);
static void set_linejoin (DiaRenderer *renderer, LineJoin mode);
static void set_linestyle (DiaRenderer *renderer, LineStyle mode);
static void set_dashlength (DiaRenderer *renderer, real length);
static void set_fillstyle (DiaRenderer *renderer, FillStyle mode);

static void draw_line (DiaRenderer *renderer,
                       Point *start, Point *end,
                       Color *color);
static void fill_polygon (DiaRenderer *renderer,
                          Point *points, int num_points,
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
                          Color *color);
static void fill_ellipse (DiaRenderer *renderer,
                          Point *center,
                          real width, real height,
                          Color *color);
static void draw_string (DiaRenderer *renderer,
                         const gchar *text,
                         Point *pos,
                         Alignment alignment,
                         Color *color);
static void draw_text (DiaRenderer *renderer,
                         Text *text);
static void draw_image (DiaRenderer *renderer,
                        Point *point,
                        real width, real height,
                        DiaImage image);

static void draw_rect (DiaRenderer *renderer,
                       Point *ul_corner, Point *lr_corner,
                       Color *color);
static void fill_rect (DiaRenderer *object,
                       Point *ul_corner, Point *lr_corner,
                       Color *color);
static void draw_polyline (DiaRenderer *renderer,
                           Point *points, int num_points,
                           Color *color);
static void draw_polygon (DiaRenderer *renderer,
                          Point *points, int num_points,
                          Color *color);

static real get_text_width (DiaRenderer *renderer,
                            const gchar *text, int length);

static void dia_gdk_renderer_class_init (DiaGdkRendererClass *klass);
static void renderer_init (DiaGdkRenderer *renderer, void*);

static gpointer parent_class = NULL;

GType
dia_gdk_renderer_get_type (void)
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

static void
renderer_init (DiaGdkRenderer *renderer, void* p)
{
  renderer->line_width = 1;
  renderer->line_style = GDK_LINE_SOLID;
  renderer->cap_style = GDK_CAP_BUTT;
  renderer->join_style = GDK_JOIN_MITER;

  renderer->saved_line_style = LINESTYLE_SOLID;
  renderer->dash_length = 10;
  renderer->dot_length = 2;
}

static void
renderer_finalize (GObject *object)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);

  if (renderer->pixmap != NULL)
    gdk_pixmap_unref(renderer->pixmap);

  if (renderer->gc != NULL)
    gdk_gc_unref(renderer->gc);

  if (renderer->clip_region != NULL)
    gdk_region_destroy(renderer->clip_region);

  if (renderer->transform)
    g_object_unref (renderer->transform);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
dia_gdk_renderer_class_init (DiaGdkRendererClass *klass)
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
  renderer_class->set_dashlength = set_dashlength;
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->draw_line    = draw_line;
  renderer_class->fill_polygon = fill_polygon;
  renderer_class->draw_rect    = draw_rect;
  renderer_class->fill_rect    = fill_rect;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;
  renderer_class->fill_ellipse = fill_ellipse;

  /* use <draw|fill>_bezier from DiaRenderer */

  renderer_class->draw_string  = draw_string;
  renderer_class->draw_text    = draw_text;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_rect = draw_rect;
  renderer_class->draw_polyline  = draw_polyline;
  renderer_class->draw_polygon   = draw_polygon;

  /* Interactive functions */
  renderer_class->get_text_width = get_text_width;
}

static void 
begin_render (DiaRenderer *object)
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

  switch(mode) {
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

  switch(mode) {
  case LINEJOIN_MITER:
    renderer->cap_style = GDK_JOIN_MITER;
    break;
  case LINEJOIN_ROUND:
    renderer->cap_style = GDK_JOIN_ROUND;
    break;
  case LINEJOIN_BEVEL:
    renderer->cap_style = GDK_JOIN_BEVEL;
    break;
  }
 
  gdk_gc_set_line_attributes(renderer->gc,
			     renderer->line_width,
			     renderer->line_style,
			     renderer->cap_style,
			     renderer->join_style);
}

static void 
set_linestyle (DiaRenderer *object, LineStyle mode)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);

  char dash_list[6];
  int hole_width;
  
  renderer->saved_line_style = mode;
  switch(mode) {
  case LINESTYLE_SOLID:
    renderer->line_style = GDK_LINE_SOLID;
    break;
  case LINESTYLE_DASHED:
    renderer->line_style = GDK_LINE_ON_OFF_DASH;
    dash_list[0] = renderer->dash_length;
    dash_list[1] = renderer->dash_length;
    gdk_gc_set_dashes(renderer->gc, 0, dash_list, 2);
    break;
  case LINESTYLE_DASH_DOT:
    renderer->line_style = GDK_LINE_ON_OFF_DASH;
    hole_width = (renderer->dash_length - renderer->dot_length) / 2;
    if (hole_width==0)
      hole_width = 1;
    dash_list[0] = renderer->dash_length;
    dash_list[1] = hole_width;
    dash_list[2] = renderer->dot_length;
    dash_list[3] = hole_width;
    gdk_gc_set_dashes(renderer->gc, 0, dash_list, 4);
    break;
  case LINESTYLE_DASH_DOT_DOT:
    renderer->line_style = GDK_LINE_ON_OFF_DASH;
    hole_width = (renderer->dash_length - 2*renderer->dot_length) / 3;
    if (hole_width==0)
      hole_width = 1;
    dash_list[0] = renderer->dash_length;
    dash_list[1] = hole_width;
    dash_list[2] = renderer->dot_length;
    dash_list[3] = hole_width;
    dash_list[4] = renderer->dot_length;
    dash_list[5] = hole_width;
    gdk_gc_set_dashes(renderer->gc, 0, dash_list, 6);
    break;
  case LINESTYLE_DOTTED:
    renderer->line_style = GDK_LINE_ON_OFF_DASH;
    dash_list[0] = renderer->dot_length;
    dash_list[1] = renderer->dot_length;
    gdk_gc_set_dashes(renderer->gc, 0, dash_list, 2);
    break;
  }
  gdk_gc_set_line_attributes(renderer->gc,
			     renderer->line_width,
			     renderer->line_style,
			     renderer->cap_style,
			     renderer->join_style);
}

static void 
set_dashlength (DiaRenderer *object, real length)
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
  set_linestyle(object, renderer->saved_line_style);
}

static void 
set_fillstyle (DiaRenderer *object, FillStyle mode)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);

  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    message_error("gdk_renderer: Unsupported fill mode specified!\n");
  }
}

static void 
draw_line (DiaRenderer *object, Point *start, Point *end, Color *line_color)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);

  GdkGC *gc = renderer->gc;
  GdkColor color;
  int x1,y1,x2,y2;
  
  dia_transform_coords(renderer->transform, start->x, start->y, &x1, &y1);
  dia_transform_coords(renderer->transform, end->x, end->y, &x2, &y2);
  
  color_convert(line_color, &color);
  gdk_gc_set_foreground(gc, &color);
  
  gdk_draw_line(renderer->pixmap, gc,
		x1, y1,	x2, y2);
}

static void 
fill_polygon (DiaRenderer *object, Point *points, int num_points, Color *line_color)
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
  
  color_convert(line_color, &color);
  gdk_gc_set_foreground(gc, &color);
  
  gdk_draw_polygon(renderer->pixmap, gc, TRUE, gdk_points, num_points);
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
  
  dia_transform_coords(renderer->transform,
                       center->x - width/2, center->y - height/2,
                       &left, &top);
  dia_transform_coords(renderer->transform,
                       center->x + width/2, center->y + height/2,
                       &right, &bottom);
  
  if ((left>right) || (top>bottom))
    return;

  color_convert(color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);

  dangle = angle2-angle1;
  if (dangle<0)
    dangle += 360.0;
  
  gdk_draw_arc(renderer->pixmap,
	       gc, fill,
	       left, top, right-left, bottom-top,
	       (int) (angle1*64.0), (int) (dangle*64.0));
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
              Color *color)
{
  draw_arc(object, center, width, height, 0.0, 360.0, color); 
}

static void 
fill_ellipse (DiaRenderer *object, Point *center,
              real width, real height, Color *color)
{
  fill_arc(object, center, width, height, 0.0, 360.0, color); 
}

static gint 
get_layout_first_baseline(PangoLayout* layout) 
{
  PangoLayoutIter* iter = pango_layout_get_iter(layout);
  gint result = pango_layout_iter_get_baseline(iter) / PANGO_SCALE;
  pango_layout_iter_free(iter);
  return result;
}

/** Return the x adjustment needed for this text and alignment */
static real
get_alignment_adjustment(DiaRenderer *object, 
			 const gchar *text,
			 Alignment alignment)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);

   switch (alignment) {
   case ALIGN_LEFT:
     return 0.0;
   case ALIGN_CENTER:
     return
       dia_font_scaled_string_width(text, object->font,
				    object->font_height,
				    dia_transform_length(renderer->transform, 10.0)/10.0)/2;
     break;
   case ALIGN_RIGHT:
     return
       dia_font_scaled_string_width(text, object->font,
				    object->font_height,
				    dia_transform_length(renderer->transform, 10.0)/10.0);
     break;
   }
   return 0.0;
}

static void 
draw_string (DiaRenderer *object,
             const gchar *text, Point *pos, Alignment alignment,
             Color *color)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);
  GdkGC *gc = renderer->gc;
  GdkColor gdkcolor;

  int x,y;
  Point start_pos;
  PangoLayout* layout = NULL;
  
  if (text == NULL || *text == '\0') return; /* Don't render empty strings. */

  point_copy(&start_pos,pos);

  color_convert(color, &gdkcolor);

  start_pos.x -= get_alignment_adjustment(object, text, alignment);

  {
    int height_pixels = dia_transform_length(renderer->transform, object->font_height);
    if (height_pixels < 2) {
      int width_pixels = dia_transform_length(
                            renderer->transform, 
                            dia_font_string_width(text, object->font, object->font_height));
      gdk_gc_set_foreground(renderer->gc, &gdkcolor);
      gdk_gc_set_dashes(renderer->gc, 0, "\1\2", 2);
      dia_transform_coords(renderer->transform, start_pos.x, start_pos.y, &x, &y);
      gdk_draw_line(renderer->pixmap, renderer->gc, x, y, x + width_pixels, y);
      return;
    }
  }

  /* My apologies for adding more #hell, but the alternative is an abhorrent
   * kludge.
   */
#ifdef HAVE_FREETYPE
  {
   FT_Bitmap ftbitmap;
   guint8 *graybitmap;
   int width, height;
   int rowstride;
   double font_height;
   GdkPixbuf *rgba = NULL;


   start_pos.y -= 
     dia_font_scaled_ascent(text, object->font,
			    object->font_height,
			    dia_transform_length(renderer->transform, 1.0));

   dia_transform_coords(renderer->transform, 
			start_pos.x, start_pos.y, &x, &y);
   
     layout = dia_font_scaled_build_layout(text, object->font,
					   object->font_height,
					   dia_transform_length(renderer->transform, 1.0));
     /*   y -= get_layout_first_baseline(layout);  */
     pango_layout_get_pixel_size(layout, &width, &height);
     
     if (width > 0) {
       rowstride = 32*((width+31)/31);
       
       graybitmap = (guint8*)g_new0(guint8, height*rowstride);
       
       ftbitmap.rows = height;
       ftbitmap.width = width;
       ftbitmap.pitch = rowstride;
       ftbitmap.buffer = graybitmap;
       ftbitmap.num_grays = 256;
       ftbitmap.pixel_mode = ft_pixel_mode_grays;
       ftbitmap.palette_mode = 0;
       ftbitmap.palette = 0;
       pango_ft2_render_layout(&ftbitmap, layout, 0, 0);
       
       {
	 int stride;
	 guchar* pixels;
	 int i,j;
	 rgba = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
	 stride = gdk_pixbuf_get_rowstride(rgba);
	 pixels = gdk_pixbuf_get_pixels(rgba);
	 for (i = 0; i < height; i++) {
	   for (j = 0; j < width; j++) {
	     pixels[i*stride+j*4] = gdkcolor.red>>8;
	     pixels[i*stride+j*4+1] = gdkcolor.green>>8;
	     pixels[i*stride+j*4+2] = gdkcolor.blue>>8;
	     pixels[i*stride+j*4+3] = graybitmap[i*rowstride+j];
	   }
	 }
	 g_free(graybitmap);
       }
     }
     /*
       gdk_draw_pixbuf(renderer->pixmap, gc, rgba, 0, 0, x, y, width, height,
       GDK_RGB_DITHER_NONE, 0, 0);
     */
     if (rgba != NULL) { /* Non-null width */
       gdk_pixbuf_render_to_drawable_alpha(rgba, 
					   renderer->pixmap,
					   0, 0,
					   x, y,
					   width, height,
					   GDK_PIXBUF_ALPHA_FULL,
					   128,
					   GDK_RGB_DITHER_NONE,
					   0, 0);
     }
     /*
       gdk_gc_set_function(gc, GDK_COPY_INVERT);
       gdk_draw_gray_image(renderer->pixmap, gc, x, y, width, height, 
       GDK_RGB_DITHER_NONE, graybitmap, rowstride);
       gdk_gc_set_function(gc, GDK_COPY);
     */
    g_object_unref(G_OBJECT(rgba));
  }
#else
  gdk_gc_set_foreground(gc, &gdkcolor);
  dia_transform_coords(renderer->transform, start_pos.x, start_pos.y, &x, &y);

  layout = dia_font_scaled_build_layout(
              text, object->font,
              object->font_height,
              dia_transform_length (renderer->transform, 10.0) / 10.0);
  y -= get_layout_first_baseline(layout);  
  gdk_draw_layout(renderer->pixmap,gc,x,y,layout);
#endif

      /* abuse_layout_object(layout,text); */
  
  g_object_unref(G_OBJECT(layout));
}

/** Caching Pango renderer */
static void
draw_text(DiaRenderer *renderer, Text *text) {
  Point pos;
  int i;

  DIA_RENDERER_GET_CLASS(renderer)->set_font(renderer, text->font, text->height);
  pos = text->position;
  
  for (i=0;i<text->numlines;i++) {
    DIA_RENDERER_GET_CLASS(renderer)->draw_string(renderer,
						  text->line[i],
						  &pos, text->alignment,
						  &text->color);
    pos.y += text->height;
  }
}


/* Get the width of the given text in cm */
static real
get_text_width(DiaRenderer *object,
               const gchar *text, int length)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);
  real result;

  if (length != strlen(text)) {
    char *othertx;
    int ulen;
    /* A couple UTF8-chars: æblegrød Š Ť Ž ę ć ń уфхцНОПРЄ є Ґ Њ Ћ Џ */
    ulen = g_utf8_offset_to_pointer(text, length)-text;
    if (!g_utf8_validate(text, ulen, NULL)) {
      g_warning ("Text at char %d not valid\n", length);
    }
    othertx = g_strndup(text, ulen);
    result = dia_font_scaled_string_width(
                othertx,object->font,
                object->font_height,
                dia_transform_length (renderer->transform, 10.0) / 10.0);
    g_free(othertx);
  } else {
    result = 
      dia_font_scaled_string_width(
            text,object->font,
            object->font_height,
            dia_transform_length (renderer->transform, 10.0) / 10.0);
  }
  return result;
}

static void 
draw_image (DiaRenderer *object,
            Point *point, 
            real width, real height,
            DiaImage image)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);
  int real_width, real_height, real_x, real_y;
  
  real_width = dia_transform_length(renderer->transform, width);
  real_height = dia_transform_length(renderer->transform, height);
  dia_transform_coords(renderer->transform, point->x, point->y,
                       &real_x, &real_y);

  dia_image_draw(image,  renderer->pixmap, real_x, real_y,
		 real_width, real_height);
}

/*
 * medium level functions
 */
void
draw_fill_rect (DiaGdkRenderer *renderer,
                Point *ul_corner, Point *lr_corner,
                Color *color, gboolean fill)
{
  GdkGC *gc = renderer->gc;
  GdkColor gdkcolor;
  gint top, bottom, left, right;
    
  dia_transform_coords(renderer->transform, 
                       ul_corner->x, ul_corner->y, &left, &top);
  dia_transform_coords(renderer->transform, 
                       lr_corner->x, lr_corner->y, &right, &bottom);
  
  if ((left>right) || (top>bottom))
    return;

  color_convert(color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);

  gdk_draw_rectangle (renderer->pixmap,
		      gc, fill,
		      left, top,
		      right-left,
		      bottom-top);
}

static void 
draw_rect (DiaRenderer *object,
           Point *ul_corner, Point *lr_corner,
           Color *color)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);

  draw_fill_rect (renderer, ul_corner, lr_corner, color, FALSE);
}

static void 
fill_rect (DiaRenderer *object,
           Point *ul_corner, Point *lr_corner,
           Color *color)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);

  draw_fill_rect (renderer, ul_corner, lr_corner, color, TRUE);
}

static void 
draw_polyline (DiaRenderer *renderer,
               Point *points, int num_points,
               Color *color)
{
  DiaRendererClass *klass = DIA_RENDERER_GET_CLASS (renderer);
  int i;

  for (i = 0; i < num_points - 1; i++)
    klass->draw_line (renderer, &points[i+0], &points[i+1], color);
}

static void 
draw_polygon (DiaRenderer *renderer,
              Point *points, int num_points,
              Color *color)
{
  DiaRendererClass *klass = DIA_RENDERER_GET_CLASS (renderer);
  int i;

  g_return_if_fail (1 < num_points);

  for (i = 0; i < num_points - 1; i++)
    klass->draw_line (renderer, &points[i+0], &points[i+1], color);
  /* close it in any case */
  if (   (points[0].x != points[num_points-1].x) 
      || (points[0].y != points[num_points-1].y))
    klass->draw_line (renderer, &points[num_points-1], &points[0], color);
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
