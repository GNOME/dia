/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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
#include <string.h> /* strlen */
#include <gdk/gdk.h>

#include "render_libart.h"

#ifdef HAVE_LIBART

#include "object.h"
#include "dialibartrenderer.h"
#include <libart_lgpl/art_rgb.h>
#include "font.h"
#include "color.h"

/** Used for highlighting mainpoint connections. */
static Color cp_main_color = { 1.0, 0.8, 0.0, 1.0 };
/** Used for highlighting normal connections. */
static Color cp_color = { 1.0, 0.0, 0.0, 1.0 };

static Color text_edit_color = {1.0, 1.0, 0.0, 1.0 };


static void clip_region_clear(DiaRenderer *self);
static void clip_region_add_rect(DiaRenderer *self,
				 Rectangle *rect);

static void draw_pixel_line(DiaRenderer *self,
			    int x1, int y1,
			    int x2, int y2,
			    Color *color);
static void draw_pixel_rect(DiaRenderer *self,
				 int x, int y,
				 int width, int height,
				 Color *color);
static void fill_pixel_rect(DiaRenderer *self,
				 int x, int y,
				 int width, int height,
				 Color *color);
static void set_size(DiaRenderer *self, gpointer window,
                     int width, int height);
static void copy_to_window (DiaRenderer *self, gpointer window,
                            int x, int y, int width, int height);
static void draw_object_highlighted (DiaRenderer *renderer,
                                     DiaObject *object,
                                     DiaHighlightType type);


void 
dia_libart_renderer_iface_init (DiaInteractiveRendererInterface* iface)
{
  iface->clip_region_clear = clip_region_clear;
  iface->clip_region_add_rect = clip_region_add_rect;
  iface->draw_pixel_line = draw_pixel_line;
  iface->draw_pixel_rect = draw_pixel_rect;
  iface->fill_pixel_rect = fill_pixel_rect;
  iface->copy_to_window = copy_to_window;
  iface->set_size = set_size;
  iface->draw_object_highlighted = draw_object_highlighted;
}


DiaRenderer *
new_libart_renderer(DiaTransform *trans, int interactive)
{
  DiaLibartRenderer *renderer;

  renderer = g_object_new(DIA_TYPE_LIBART_RENDERER, NULL);
  renderer->transform = trans;
  renderer->parent_instance.is_interactive = interactive;

  return DIA_RENDERER (renderer);
}

static void
set_size(DiaRenderer *self, gpointer window,
         int width, int height)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  int i;
  
  if ( (renderer->pixel_width==width) &&
       (renderer->pixel_height==height) )
    return;
  
  if (renderer->rgb_buffer != NULL) {
    g_free(renderer->rgb_buffer);
  }

  renderer->rgb_buffer = g_new (guint8, width * height * 3);
  for (i=0;i<width * height * 3;i++)
    renderer->rgb_buffer[i] = 0xff;
  renderer->pixel_width = width;
  renderer->pixel_height = height;
}

static void
copy_to_window (DiaRenderer *self, gpointer window,
                int x, int y, int width, int height)
{
  GdkGC *copy_gc;
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  int w;

  copy_gc = gdk_gc_new(GDK_WINDOW(window));

  w = renderer->pixel_width;
  
  gdk_draw_rgb_image(window,
		     copy_gc,
		     x,y,
		     width, height,
		     GDK_RGB_DITHER_NONE,
		     renderer->rgb_buffer+x*3+y*3*w,
		     w*3);
  g_object_unref(copy_gc);
}

static void
clip_region_clear(DiaRenderer *self)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);

  renderer->clip_rect_empty = 1;
  renderer->clip_rect.top = 0;
  renderer->clip_rect.bottom = 0;
  renderer->clip_rect.left = 0;
  renderer->clip_rect.right = 0;
}

static void
clip_region_add_rect(DiaRenderer *self,
		     Rectangle *rect)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  int x1,y1;
  int x2,y2;
  IntRectangle r;

  dia_transform_coords(renderer->transform, rect->left, rect->top,  &x1, &y1);
  dia_transform_coords(renderer->transform, rect->right, rect->bottom,  &x2, &y2);

  if (x1 < 0)
    x1 = 0;
  if (y1 < 0)
    y1 = 0;
  if (x2 >= renderer->pixel_width)
    x2 = renderer->pixel_width - 1;
  if (y2 >= renderer->pixel_height)
    y2 = renderer->pixel_height - 1;
  
  r.top = y1;
  r.bottom = y2;
  r.left = x1;
  r.right = x2;
  
  if (renderer->clip_rect_empty) {
    renderer->clip_rect = r;
    renderer->clip_rect_empty = 0;
  } else {
    int_rectangle_union(&renderer->clip_rect, &r);
  }
}


/* BIG FAT WARNING:
 * This code is used to draw pixel based stuff in the RGB buffer.
 * This code is *NOT* as efficient as it could be!
 */

/* All lines and rectangles specifies the coordinates inclusive.
 * This means that a line from x1 to x2 renders both points.
 * If a length is specified the line x to (inclusive) x+width is rendered.
 *
 * The boundaries of the clipping rectangle *are* rendered.
 * so min=5 and max=10 means point 5 and 10 might be rendered to.
 */

/* If the start-end interval is totaly outside the min-max,
   then the returned clipped values can have len<0! */
#define CLIP_1D_LEN(min, max, start, len) \
   if ((start) < (min)) {                 \
      (len) -= (min) - (start);           \
      (start) = (min);                    \
   }                                      \
   if ((start)+(len) > (max)) {           \
      (len) = (max) - (start);            \
   }

/* Does no clipping! */
static void
draw_hline(DiaRenderer *self,
	   int x, int y, int length,
	   guint8 r, guint8 g, guint8 b)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  int stride;
  guint8 *ptr;

  stride = renderer->pixel_width*3;
  ptr = renderer->rgb_buffer + x*3 + y*stride;
  if (length>=0)
    art_rgb_fill_run(ptr, r, g, b, length+1);
}

/* Does no clipping! */
static void
draw_vline(DiaRenderer *self,
	   int x, int y, int height,
	   guint8 r, guint8 g, guint8 b)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  int stride;
  guint8 *ptr;

  stride = renderer->pixel_width*3;
  ptr = renderer->rgb_buffer + x*3 + y*stride;
  height+=y;
  while (y<=height) {
    *ptr++ = r;
    *ptr++ = g;
    *ptr++ = b;
    ptr += stride - 3;
    y++;
  }
}

static void
draw_pixel_line(DiaRenderer *self,
		int x1, int y1,
		int x2, int y2,
		Color *color)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  guint8 r,g,b;
  guint8 *ptr;
  int start, len;
  int stride;
  int i;
  int x, y;
  int dx, dy, adx, ady;
  int incx, incy;
  int incx_ptr, incy_ptr;
  int frac_pos;
  IntRectangle *clip_rect;


  r = color->red*0xff;
  g = color->green*0xff;
  b = color->blue*0xff;

  if (y1==y2) { /* Horizontal line */
    start = x1;
    len = x2-x1;
    CLIP_1D_LEN(renderer->clip_rect.left, renderer->clip_rect.right, start, len);
    
    /* top line */
    if ( (y1>=renderer->clip_rect.top) &&
	 (y1<=renderer->clip_rect.bottom) ) {
      draw_hline(self, start, y1, len, r, g, b);
    }
    return;
  }
  
  if (x1==x2) { /* Vertical line */
    start = y1;
    len = y2-y1;
    CLIP_1D_LEN(renderer->clip_rect.top, renderer->clip_rect.bottom, start, len);

    /* left line */
    if ( (x1>=renderer->clip_rect.left) &&
	 (x1<=renderer->clip_rect.right) ) {
      draw_vline(self, x1, start, len, r, g, b);
    }
    return;
  }

  /* Ugh, kill me slowly for writing this line-drawer.
   * It is actually a standard bresenham, but not very optimized.
   * It is also not very well tested.
   */
  
  stride = renderer->pixel_width*3;
  clip_rect = &renderer->clip_rect;
  
  dx = x2-x1;
  dy = y2-y1;
  adx = (dx>=0)?dx:-dx;
  ady = (dy>=0)?dy:-dy;

  x = x1; y = y1;
  ptr = renderer->rgb_buffer + x*3 + y*stride;
  
  if (adx>=ady) { /* x-major */
    if (dx>0) {
      incx = 1;
      incx_ptr = 3;
    } else {
      incx = -1;
      incx_ptr = -3;
    }
    if (dy>0) {
      incy = 1;
      incy_ptr = stride;
    } else {
      incy = -1;
      incy_ptr = -stride;
    }
    frac_pos = adx;
    
    for (i=0;i<=adx;i++) {
      /* Amazing... He does the clipping in the inner loop!
	 It must be horribly inefficient! */
      if ( (x>=clip_rect->left) &&
	   (x<=clip_rect->right) &&
	   (y>=clip_rect->top) &&
	   (y<=clip_rect->bottom) ) {
	ptr[0] = r;
	ptr[1] = g;
	ptr[2] = b;
      }
      x += incx;
      ptr += incx_ptr;
      frac_pos += ady*2;
      if ((frac_pos > 2*adx) || ((dy>0)&&(frac_pos == 2*adx))) {
	y += incy;
	ptr += incy_ptr;
	frac_pos -= 2*adx;
      }
    }
  } else { /* y-major */
    if (dx>0) {
      incx = 1;
      incx_ptr = 3;
    } else {
      incx = -1;
      incx_ptr = -3;
    }
    if (dy>0) {
      incy = 1;
      incy_ptr = stride;
    } else {
      incy = -1;
      incy_ptr = -stride;
    }
    frac_pos = ady;
    
    for (i=0;i<=ady;i++) {
      /* Amazing... He does the clipping in the inner loop!
	 It must be horribly inefficient! */
      if ( (x>=clip_rect->left) &&
	   (x<=clip_rect->right) &&
	   (y>=clip_rect->top) &&
	   (y<=clip_rect->bottom) ) {
	ptr[0] = r;
	ptr[1] = g;
	ptr[2] = b;
      }
      y += incy;
      ptr += incy_ptr;
      frac_pos += adx*2;
      if ((frac_pos > 2*ady) || ((dx>0)&&(frac_pos == 2*ady))) {
	x += incx;
	ptr += incx_ptr;
	frac_pos -= 2*ady;
      }
    }
  }
}

static void
draw_pixel_rect(DiaRenderer *self,
		int x, int y,
		int width, int height,
		Color *color)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  guint8 r,g,b;
  int start, len;

  r = color->red*0xff;
  g = color->green*0xff;
  b = color->blue*0xff;

  /* clip in x */
  start = x;
  len = width;
  CLIP_1D_LEN(renderer->clip_rect.left, renderer->clip_rect.right, start, len);

  /* top line */
  if ( (y>=renderer->clip_rect.top) &&
       (y<=renderer->clip_rect.bottom) ) {
    draw_hline(self, start, y, len, r, g, b);
  }

  /* bottom line */
  if ( (y+height>=renderer->clip_rect.top) &&
       (y+height<=renderer->clip_rect.bottom) ) {
    draw_hline(self, start, y+height, len, r, g, b);
  }

  /* clip in y */
  start = y;
  len = height;
  CLIP_1D_LEN(renderer->clip_rect.top, renderer->clip_rect.bottom, start, len);

  /* left line */
  if ( (x>=renderer->clip_rect.left) &&
       (x<renderer->clip_rect.right) ) {
    draw_vline(self, x, start, len, r, g, b);
  }

  /* right line */
  if ( (x+width>=renderer->clip_rect.left) &&
       (x+width<renderer->clip_rect.right) ) {
    draw_vline(self, x+width, start, len, r, g, b);
  }
}

static void
fill_pixel_rect(DiaRenderer *self,
		int x, int y,
		int width, int height,
		Color *color)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  guint8 r,g,b;
  guint8 *ptr;
  int i;
  int stride;


  CLIP_1D_LEN(renderer->clip_rect.left, renderer->clip_rect.right, x, width);
  if (width < 0)
    return;

  CLIP_1D_LEN(renderer->clip_rect.top, renderer->clip_rect.bottom, y, height);
  if (height < 0)
    return;
  
  r = color->red*0xff;
  g = color->green*0xff;
  b = color->blue*0xff;
  
  stride = renderer->pixel_width*3;
  ptr = renderer->rgb_buffer + x*3 + y*stride;
  for (i=0;i<=height;i++) {
    art_rgb_fill_run(ptr, r, g, b, width+1);
    ptr += stride;
  }
}


static void
draw_object_highlighted (DiaRenderer *renderer, DiaObject *object, DiaHighlightType type)
{
  DiaLibartRenderer *libart_rend = DIA_LIBART_RENDERER(renderer);
  Color *color = NULL;
  switch (type) {
  case DIA_HIGHLIGHT_CONNECTIONPOINT:
    color = &cp_color;
    break;
  case DIA_HIGHLIGHT_CONNECTIONPOINT_MAIN:
    color = &cp_main_color;
    break;
  case DIA_HIGHLIGHT_TEXT_EDIT:
    color = &text_edit_color;
    break;
  case DIA_HIGHLIGHT_NONE:
    color = NULL;
    break;
  }
  if( color ) {
    libart_rend->highlight_color = color;
    object->ops->draw(object, renderer);
    libart_rend->highlight_color = NULL;
  }

  object->ops->draw(object, renderer);
}

#else

DiaRenderer *
new_libart_renderer(DiaTransform *transform, int interactive)
{
  return NULL;
}

#endif

