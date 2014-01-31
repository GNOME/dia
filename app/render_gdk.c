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
#include <stdlib.h>
#include <string.h>
#include <gdk/gdk.h>

#include "object.h"
#include "message.h"
#include "render_gdk.h"
#include "diagdkrenderer.h"


/** Used for highlighting mainpoint connections. */
static Color cp_main_color = { 1.0, 0.8, 0.0, 1.0 };
/** Used for highlighting normal connections. */
static Color cp_color = { 1.0, 0.0, 0.0, 1.0 };

static Color text_edit_color = {1.0, 1.0, 0.0, 1.0 };


static void clip_region_clear(DiaRenderer *renderer);
static void clip_region_add_rect(DiaRenderer *renderer,
                                 Rectangle *rect);

static void draw_pixel_line(DiaRenderer *renderer,
                            int x1, int y1,
                            int x2, int y2,
                            Color *color);
static void draw_pixel_rect(DiaRenderer *renderer,
                            int x, int y,
                            int width, int height,
                            Color *color);
static void fill_pixel_rect(DiaRenderer *renderer,
                            int x, int y,
                            int width, int height,
                            Color *color);
static void set_size (DiaRenderer *renderer, 
                      gpointer window,
                      int width, int height);
static void copy_to_window (DiaRenderer *renderer, 
                gpointer window,
                int x, int y, int width, int height);
static void draw_object_highlighted (DiaRenderer *renderer,
                                     DiaObject *object,
                                     DiaHighlightType type);

typedef struct _DiaGdkInteractiveRenderer DiaGdkInteractiveRenderer;
struct _DiaGdkInteractiveRenderer
{
  DiaGdkRenderer parent_instance; /*!< inheritance in object oriented C */
};
typedef struct _DiaGdkInteractiveRendererClass DiaGdkInteractiveRendererClass;
struct _DiaGdkInteractiveRendererClass
{
  DiaGdkRendererClass parent_class; /*!< the base class */
};
static GType dia_gdk_interactive_renderer_get_type (void);
#define DIA_TYPE_GDK_INTERACTIVE_RENDERER           (dia_gdk_interactive_renderer_get_type ())
#define DIA_GDK_INTERACTIVE_RENDERER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_GDK_INTERACTIVE_RENDERER, DiaGdkInteractiveRenderer))

static void
dia_gdk_renderer_iface_init (DiaInteractiveRendererInterface* iface)
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

G_DEFINE_TYPE_WITH_CODE (DiaGdkInteractiveRenderer, dia_gdk_interactive_renderer, DIA_TYPE_GDK_RENDERER,
			 G_IMPLEMENT_INTERFACE (DIA_TYPE_INTERACTIVE_RENDERER_INTERFACE, dia_gdk_renderer_iface_init));

static void
dia_gdk_interactive_renderer_class_init(DiaGdkInteractiveRendererClass *klass)
{
}
static void
dia_gdk_interactive_renderer_init(DiaGdkInteractiveRenderer *object)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER(object);
  DiaRenderer *dia_renderer = DIA_RENDERER(object);
  
  dia_renderer->is_interactive = 1;

  renderer->gc = NULL;
  renderer->pixmap = NULL;
  renderer->clip_region = NULL;
}

DiaRenderer *
new_gdk_renderer(DDisplay *ddisp)
{
  DiaGdkRenderer *renderer;

  renderer = g_object_new (DIA_TYPE_GDK_INTERACTIVE_RENDERER, NULL);
  renderer->transform = dia_transform_new (&ddisp->visible, &ddisp->zoom_factor);

  return DIA_RENDERER(renderer);
}

static void
set_size(DiaRenderer *object, gpointer window,
		      int width, int height)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);

  if (renderer->pixmap != NULL)
    g_object_unref(renderer->pixmap);

  if (window)
    renderer->pixmap = gdk_pixmap_new(GDK_WINDOW(window),  width, height, -1);
  else /* the integrated UI insist to call us too early */
    renderer->pixmap = gdk_pixmap_new(NULL,  width, height, 24);

  if (renderer->gc == NULL) {
    renderer->gc = gdk_gc_new(renderer->pixmap);

    gdk_gc_set_line_attributes(renderer->gc,
			       renderer->line_width,
			       renderer->line_style,
			       renderer->cap_style,
			       renderer->join_style);
  }
}

static void
copy_to_window (DiaRenderer *object, gpointer window,
                int x, int y, int width, int height)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);
  static GdkGC *copy_gc = NULL;
  
  if (!copy_gc)
    copy_gc = gdk_gc_new(window);

  gdk_draw_drawable (GDK_WINDOW(window),
                     copy_gc,
                     renderer->pixmap,
                     x, y,
                     x, y,
                     width, height);
}

static void
clip_region_clear(DiaRenderer *object)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);

  if (renderer->clip_region != NULL)
    gdk_region_destroy(renderer->clip_region);

  renderer->clip_region =  gdk_region_new();

  gdk_gc_set_clip_region(renderer->gc, renderer->clip_region);
}

static void
clip_region_add_rect(DiaRenderer *object,
		 Rectangle *rect)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);
  GdkRectangle clip_rect;
  int x1,y1;
  int x2,y2;

  dia_transform_coords(renderer->transform, rect->left, rect->top,  &x1, &y1);
  dia_transform_coords(renderer->transform, rect->right, rect->bottom,  &x2, &y2);
  
  clip_rect.x = x1;
  clip_rect.y = y1;
  clip_rect.width = x2 - x1 + 1;
  clip_rect.height = y2 - y1 + 1;

  gdk_region_union_with_rect( renderer->clip_region, &clip_rect );
  
  gdk_gc_set_clip_region(renderer->gc, renderer->clip_region);
}

static void
draw_pixel_line(DiaRenderer *object,
		int x1, int y1,
		int x2, int y2,
		Color *color)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);
  GdkGC *gc = renderer->gc;
  GdkColor gdkcolor;
  int target_width, target_height;
    
  dia_gdk_renderer_set_dashes(renderer, x1+y1);

  gdk_drawable_get_size (GDK_DRAWABLE (renderer->pixmap), &target_width, &target_height);

  if (   (x1 < 0 && x2 < 0)
      || (y1 < 0 && y2 < 0)
      || (x1 > target_width && x2 > target_width) 
      || (y1 > target_height && y2 > target_height))
    return; /* clip early rather than failing in Gdk */
  
  color_convert(color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);
  /* reset stippling (alpha emulation) */
  gdk_gc_set_fill(gc, GDK_SOLID);
  renderer->current_alpha = 1.0;
  
  gdk_draw_line(renderer->pixmap, gc, x1, y1, x2, y2);
}

static void
draw_pixel_rect(DiaRenderer *object,
		int x, int y,
		int width, int height,
		Color *color)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);
  GdkGC *gc = renderer->gc;
  GdkColor gdkcolor;
  int target_width, target_height;
    
  dia_gdk_renderer_set_dashes(renderer, x+y);

  gdk_drawable_get_size (GDK_DRAWABLE (renderer->pixmap), &target_width, &target_height);

  if (x + width < 0 || y + height < 0 || x > target_width || y > target_height)
    return; /* clip early rather than failing in Gdk */

  color_convert(color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);
  /* reset stippling (alpha emulation) */
  gdk_gc_set_fill(gc, GDK_SOLID);
  renderer->current_alpha = 1.0;

  gdk_draw_rectangle (renderer->pixmap, gc, FALSE,
		      x, y,  width, height);
}

static void
fill_pixel_rect(DiaRenderer *object,
		int x, int y,
		int width, int height,
		Color *color)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (object);
  GdkGC *gc = renderer->gc;
  GdkColor gdkcolor;
  int target_width, target_height;
    
  gdk_drawable_get_size (GDK_DRAWABLE (renderer->pixmap), &target_width, &target_height);
    
  if (x + width < 0 || y + height < 0 || x > target_width || y > target_height)
    return; /* clip early rather than failing in Gdk */

  color_convert(color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);
  /* reset stippling (alpha emulation) */
  gdk_gc_set_fill(gc, GDK_SOLID);
  renderer->current_alpha = 1.0;

  gdk_draw_rectangle (renderer->pixmap, gc, TRUE,
		      x, y,  width, height);
}

static void
draw_object_highlighted (DiaRenderer *renderer, DiaObject *object, DiaHighlightType type)
{
  DiaGdkRenderer *gdk_rend = DIA_GDK_RENDERER(renderer);
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
    gdk_rend->highlight_color = color;
    object->ops->draw(object, renderer);
    gdk_rend->highlight_color = NULL;
  }

  object->ops->draw(object, renderer);
}

