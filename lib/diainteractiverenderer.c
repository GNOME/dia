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
  iface->copy_to_window = NULL;
  iface->set_size = NULL;
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

