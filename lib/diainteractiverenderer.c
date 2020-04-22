/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diainteractiverenderer.c - interface definition, implementation
 *                            live in app
 * Copyright (C) 2002 Hans Breuer (refactoring)
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

#include "diarenderer.h"
#include "diainteractiverenderer.h"

G_DEFINE_INTERFACE (DiaInteractiveRenderer, dia_interactive_renderer, DIA_TYPE_RENDERER)

/**
 * SECTION:diainteractiverenderer
 * @title: DiaInteractiveRenderer
 * @short_description: #DiaRenderer for displays
 *
 * Interface to be provide by interactive renderers
 *
 * The interactive renderer interface extends a renderer with clipping
 * and drawing with pixel coordinates.
 */

static int
get_width_pixels (DiaInteractiveRenderer *self)
{
  g_return_val_if_fail (DIA_IS_INTERACTIVE_RENDERER (self), 0);

  g_critical ("get_width_pixels isn't implemented for %s",
              g_type_name (G_TYPE_FROM_INSTANCE (self)));

  return 0;
}

static int
get_height_pixels (DiaInteractiveRenderer *self)
{
  g_return_val_if_fail (DIA_IS_INTERACTIVE_RENDERER (self), 0);

  g_critical ("get_height_pixels isn't implemented for %s",
              g_type_name (G_TYPE_FROM_INSTANCE (self)));

  return 0;
}

static void
dia_interactive_renderer_default_init (DiaInteractiveRendererInterface *iface)
{
  /* NULL initialization probably already done by GObject */
  iface->get_width_pixels = get_width_pixels;
  iface->get_height_pixels = get_height_pixels;
  iface->clip_region_clear = NULL;
  iface->clip_region_add_rect = NULL;
  iface->draw_pixel_line = NULL;
  iface->draw_pixel_rect = NULL;
  iface->fill_pixel_rect = NULL;
  iface->paint = NULL;
  iface->set_size = NULL;
  iface->draw_object_highlighted = NULL;
}


/**
 * dia_interactive_renderer_get_width_pixels:
 * @self: the #DiaInteractiveRenderer
 *
 * Get the width in pixels of the drawing area (if any)
 *
 * Returns: the width
 *
 * Since: 0.98
 */
int
dia_interactive_renderer_get_width_pixels (DiaInteractiveRenderer *self)
{
  DiaInteractiveRendererInterface *irenderer =
    DIA_INTERACTIVE_RENDERER_GET_IFACE (self);

  g_return_val_if_fail (irenderer != NULL, 0);
  g_return_val_if_fail (irenderer->get_width_pixels != NULL, 0);

  return irenderer->get_width_pixels (self);
}


/**
 * dia_interactive_renderer_get_height_pixels:
 * @self: the #DiaInteractiveRenderer
 *
 * Get the height in pixels of the drawing area (if any)
 *
 * Returns: the height
 *
 * Since: 0.98
 */
int
dia_interactive_renderer_get_height_pixels (DiaInteractiveRenderer *self)
{
  DiaInteractiveRendererInterface *irenderer =
    DIA_INTERACTIVE_RENDERER_GET_IFACE (self);

  g_return_val_if_fail (irenderer != NULL, 0);
  g_return_val_if_fail (irenderer->get_height_pixels != NULL, 0);

  return irenderer->get_height_pixels (self);
}


/**
 * dia_interactive_renderer_set_size:
 * @self: the #DiaInteractiveRenderer
 * @window: the #GdkWindow
 * @width: width of the canvas
 * @height: height of the canvas
 *
 * Size adjustment to the given window
 *
 * Since: 0.98
 */
void
dia_interactive_renderer_set_size (DiaInteractiveRenderer *self,
                                   gpointer                window,
                                   int                     width,
                                   int                     height)
{
  DiaInteractiveRendererInterface *irenderer =
    DIA_INTERACTIVE_RENDERER_GET_IFACE (self);

  g_return_if_fail (irenderer != NULL);
  g_return_if_fail (irenderer->set_size != NULL);

  irenderer->set_size (self, window, width, height);
}


/**
 * dia_interactive_renderer_clip_region_clear:
 * @self: the #DiaInteractiveRenderer
 *
 * Since: 0.98
 */
void
dia_interactive_renderer_clip_region_clear (DiaInteractiveRenderer *self)
{
  DiaInteractiveRendererInterface *irenderer =
    DIA_INTERACTIVE_RENDERER_GET_IFACE (self);

  g_return_if_fail (irenderer != NULL);
  g_return_if_fail (irenderer->clip_region_clear != NULL);

  irenderer->clip_region_clear (self);
}


/**
 * dia_interactive_renderer_clip_region_add_rect:
 * @self: the #DiaInteractiveRenderer
 * @rect: the #DiaRectangle to add
 *
 * Since: 0.98
 */
void
dia_interactive_renderer_clip_region_add_rect (DiaInteractiveRenderer *self,
                                               DiaRectangle           *rect)
{
  DiaInteractiveRendererInterface *irenderer =
    DIA_INTERACTIVE_RENDERER_GET_IFACE (self);

  g_return_if_fail (irenderer != NULL);
  g_return_if_fail (irenderer->clip_region_add_rect != NULL);

  irenderer->clip_region_add_rect (self, rect);
}


/**
 * dia_interactive_renderer_draw_pixel_line:
 * @self: the #DiaInteractiveRenderer
 * @x1: starting horizontal position
 * @y1: starting vertical position
 * @x2: ending horizontal position
 * @y2: ending vertical position
 * @color: #Color to draw with
 *
 * Since: 0.98
 */
void
dia_interactive_renderer_draw_pixel_line (DiaInteractiveRenderer *self,
                                          int                     x1,
                                          int                     y1,
                                          int                     x2,
                                          int                     y2,
                                          Color                  *color)
{
  DiaInteractiveRendererInterface *irenderer =
    DIA_INTERACTIVE_RENDERER_GET_IFACE (self);

  g_return_if_fail (irenderer != NULL);
  g_return_if_fail (irenderer->draw_pixel_line != NULL);

  irenderer->draw_pixel_line (self, x1, y1, x2, y2, color);
}


/**
 * dia_interactive_renderer_draw_pixel_rect:
 * @self: the #DiaInteractiveRenderer
 * @x: horizontal position
 * @y: vertical position
 * @width: width of the rectangle
 * @height: height of the rectangle
 * @color: #Color to outline with
 *
 * Since: 0.98
 */
void
dia_interactive_renderer_draw_pixel_rect (DiaInteractiveRenderer *self,
                                          int                     x,
                                          int                     y,
                                          int                     width,
                                          int                     height,
                                          Color                  *color)
{
  DiaInteractiveRendererInterface *irenderer =
    DIA_INTERACTIVE_RENDERER_GET_IFACE (self);

  g_return_if_fail (irenderer != NULL);
  g_return_if_fail (irenderer->draw_pixel_rect != NULL);

  irenderer->draw_pixel_rect (self, x, y, width, height, color);
}


/**
 * dia_interactive_renderer_fill_pixel_rect:
 * @self: the #DiaInteractiveRenderer
 * @x: horizontal position
 * @y: vertical position
 * @width: width of the rectangle
 * @height: height of the rectangle
 * @color: #Color to fill with
 *
 * Since: 0.98
 */
void
dia_interactive_renderer_fill_pixel_rect (DiaInteractiveRenderer *self,
                                          int                     x,
                                          int                     y,
                                          int                     width,
                                          int                     height,
                                          Color                  *color)
{
  DiaInteractiveRendererInterface *irenderer =
    DIA_INTERACTIVE_RENDERER_GET_IFACE (self);

  g_return_if_fail (irenderer != NULL);
  g_return_if_fail (irenderer->fill_pixel_rect != NULL);

  irenderer->fill_pixel_rect (self, x, y, width, height, color);
}


/**
 * dia_interactive_renderer_paint:
 * @self: the #DiaInteractiveRenderer
 * @ctx: the #cairo_t
 * @width: width of the canvas
 * @height: height of the canvas
 *
 * Draw @self to @ctx
 *
 * Since: 0.98
 */
void
dia_interactive_renderer_paint (DiaInteractiveRenderer *self,
                                cairo_t                *ctx,
                                int                     width,
                                int                     height)
{
  DiaInteractiveRendererInterface *irenderer =
    DIA_INTERACTIVE_RENDERER_GET_IFACE (self);

  g_return_if_fail (irenderer != NULL);
  g_return_if_fail (irenderer->paint != NULL);

  irenderer->paint (self, ctx, width, height);
}


/**
 * dia_interactive_renderer_draw_object_highlighted:
 * @self: the #DiaInteractiveRenderer
 * @object: the #DiaObject to draw
 * @type: the #DiaHighlightType style
 *
 * Since: 0.98
 */
void
dia_interactive_renderer_draw_object_highlighted (DiaInteractiveRenderer *self,
                                                  DiaObject              *object,
                                                  DiaHighlightType        type)
{
  DiaInteractiveRendererInterface *irenderer =
    DIA_INTERACTIVE_RENDERER_GET_IFACE (self);

  g_return_if_fail (irenderer != NULL);
  g_return_if_fail (irenderer->draw_object_highlighted != NULL);

  irenderer->draw_object_highlighted (self, object, type);
}


/**
 * dia_interactive_renderer_set_selection:
 * @self: the #DiaInteractiveRenderer
 * @has_selection: if %TRUE the selection box should be drawn
 * @x: horizontal position of the selection
 * @y: vertical position of the selection
 * @width: width of the selection
 * @height: height of the selection
 *
 * Since: 0.98
 */
void
dia_interactive_renderer_set_selection (DiaInteractiveRenderer *self,
                                        gboolean                has_selection,
                                        double                  x,
                                        double                  y,
                                        double                  width,
                                        double                  height)
{
  DiaInteractiveRendererInterface *irenderer =
    DIA_INTERACTIVE_RENDERER_GET_IFACE (self);

  g_return_if_fail (irenderer != NULL);
  g_return_if_fail (irenderer->set_selection != NULL);

  irenderer->set_selection (self, has_selection, x, y, width, height);
}
