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

/**
 * SECTION:dia-interactive-renderer
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
dia_interactive_renderer_iface_init (DiaInteractiveRendererInterface *iface)
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

GType
dia_interactive_renderer_get_type (void)
{
  static GType iface_type = 0;

  if (!iface_type)
    {
      static const GTypeInfo iface_info =
      {
        sizeof (DiaInteractiveRendererInterface),
	(GBaseInitFunc)     dia_interactive_renderer_iface_init,
	(GBaseFinalizeFunc) NULL,
      };

      iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                           "DiaInteractiveRendererInterface",
                                           &iface_info,
                                           0);

      g_type_interface_add_prerequisite (iface_type,
                                         DIA_TYPE_RENDERER);
    }

  return iface_type;
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
  return DIA_INTERACTIVE_RENDERER_GET_IFACE (self)->get_width_pixels (self);
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
  return DIA_INTERACTIVE_RENDERER_GET_IFACE (self)->get_height_pixels (self);
}
