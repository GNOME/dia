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

static void
dia_interactive_renderer_iface_init (DiaInteractiveRendererInterface *iface)
{
  /* NULL initialization probably already done by GObject */
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
dia_interactive_renderer_interface_get_type (void)
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

/*
 * Wrapper functions using the above
 */
void 
dia_renderer_set_size (DiaRenderer* renderer, gpointer window, 
                       int width, int height)
{
  DiaInteractiveRendererInterface *irenderer =
    DIA_GET_INTERACTIVE_RENDERER_INTERFACE (renderer);

  g_return_if_fail (irenderer != NULL);
  g_return_if_fail (irenderer->set_size != NULL);

  irenderer->set_size (renderer, window, width, height);
}

void
dia_interactive_renderer_paint (DiaRenderer *renderer,
                                cairo_t     *ctx, 
                                int          width,
                                int          height)
{
  DiaInteractiveRendererInterface *irenderer =
    DIA_GET_INTERACTIVE_RENDERER_INTERFACE (renderer);
  
  g_return_if_fail (irenderer != NULL);
  g_return_if_fail (irenderer->paint != NULL);

  irenderer->paint (renderer, ctx, width, height);
}

void
dia_interactive_renderer_set_selection (DiaRenderer *renderer,
                                        gboolean     has_selection,
                                        double       x,
                                        double       y,
                                        double       width,
                                        double       height)
{
  DiaInteractiveRendererInterface *irenderer =
    DIA_GET_INTERACTIVE_RENDERER_INTERFACE (renderer);
  
  g_return_if_fail (irenderer != NULL);
  g_return_if_fail (irenderer->set_selection != NULL);

  irenderer->set_selection (renderer, has_selection, x, y, width, height);
}
