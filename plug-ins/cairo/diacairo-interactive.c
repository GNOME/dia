/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diacairo-interactive.c -- Cairo based interactive renderer
 * Copyright (C) 2006, Nguyen Thai Ngoc Duy
 * Copyright (C) 2007, Hans Breuer, <Hans@Breuer.Org>
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

#include "diacairo.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h> /* GTK_CHECK_VERSION */

#include "intl.h"
#include "color.h"
#include "diatransform.h"
#include "object.h"
#include "textline.h"

/* There is a variant prepared for GTK+3 
 * but it seems to be sligly slower than the original version.
 */
#if GTK_CHECK_VERSION(3,0,0)
#define DIA_CAIRO_WITH_PIXMAP 0
#else
#define DIA_CAIRO_WITH_PIXMAP 1
#endif

#define DIA_TYPE_CAIRO_INTERACTIVE_RENDERER           (dia_cairo_interactive_renderer_get_type ())
#define DIA_CAIRO_INTERACTIVE_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_CAIRO_INTERACTIVE_RENDERER, DiaCairoInteractiveRenderer))
#define DIA_CAIRO_INTERACTIVE_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_CAIRO_INTERACTIVE_RENDERER, DiaCairoInteractiveRendererClass))
#define DIA_CAIRO_IS_INTERACTIVE_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_CAIRO_INTERACTIVE_RENDERER))
#define DIA_CAIRO_INTERACTIVE_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_CAIRO_INTERACTIVE_RENDERER, DiaCairoInteractiveRendererClass))

GType dia_cairo_interactive_renderer_get_type (void) G_GNUC_CONST;

typedef struct _DiaCairoInteractiveRenderer DiaCairoInteractiveRenderer;
typedef struct _DiaCairoInteractiveRendererClass DiaCairoInteractiveRendererClass;

struct _DiaCairoInteractiveRenderer
{
  DiaCairoRenderer parent_instance;

  /*< private >*/
  Rectangle *visible;
  real *zoom_factor;

#if DIA_CAIRO_WITH_PIXMAP
  GdkPixmap *pixmap;              /* The pixmap shown in this display  */
  GdkGC *gc;
#else
  cairo_surface_t *pixmap;        /* The pixmap shown in this display  */
#endif
  guint32 width;                  /* The width of the pixmap in pixels */
  guint32 height;                 /* The height of the pixmap in pixels */
  GdkRegion *clip_region;

  /** If non-NULL, this rendering is a highlighting with the given color. */
  Color *highlight_color;
};

struct _DiaCairoInteractiveRendererClass
{
  DiaRendererClass parent_class;
};

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

static void cairo_interactive_renderer_get_property (GObject         *object,
			 guint            prop_id,
			 GValue          *value,
			 GParamSpec      *pspec);
static void cairo_interactive_renderer_set_property (GObject         *object,
			 guint            prop_id,
			 const GValue    *value,
			 GParamSpec      *pspec);

enum {
  PROP_0,
  PROP_ZOOM,
  PROP_RECT
};


static int
get_width_pixels (DiaRenderer *object)
{ 
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);
  return renderer->width;
}

static int
get_height_pixels (DiaRenderer *object)
{ 
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);
  return renderer->height;
}

/* gobject boiler plate */
static void cairo_interactive_renderer_init (DiaCairoInteractiveRenderer *r, void *p);
static void cairo_interactive_renderer_class_init (DiaCairoInteractiveRendererClass *klass);

static gpointer parent_class = NULL;

static void
cairo_interactive_renderer_init (DiaCairoInteractiveRenderer *object, void *p)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);
  DiaRenderer *dia_renderer = DIA_RENDERER(object);
  
  dia_renderer->is_interactive = 1;
  
  renderer->pixmap = NULL;

  renderer->highlight_color = NULL;
}

static void
cairo_interactive_renderer_finalize (GObject *object)
{
#if !DIA_CAIRO_WITH_PIXMAP
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);
#endif
  DiaCairoRenderer *base_renderer = DIA_CAIRO_RENDERER (object);

  if (base_renderer->cr)
    cairo_destroy (base_renderer->cr);
  base_renderer->cr = NULL;
#if !DIA_CAIRO_WITH_PIXMAP
  if (renderer->pixmap)
    cairo_surface_destroy (renderer->pixmap);
#endif
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* Get the width of the given text in cm */
static real
get_text_width(DiaRenderer *object,
               const gchar *text, int length)
{
  real result;
  TextLine *text_line;

  if (length != g_utf8_strlen(text, -1)) {
    char *shorter;
    int ulen;
    ulen = g_utf8_offset_to_pointer(text, length)-text;
    if (!g_utf8_validate(text, ulen, NULL)) {
      g_warning ("Text at char %d not valid\n", length);
    }
    shorter = g_strndup(text, ulen);
    text_line = text_line_new(shorter, object->font, object->font_height);
    g_free (shorter);
  } else {
    text_line = text_line_new(text, object->font, object->font_height);
  }
  result = text_line_get_width(text_line);
  text_line_destroy(text_line);
  return result;
}
/** Used as background? for text editing */
static Color text_edit_color = {1.0, 1.0, 0.7 };

static real
calculate_relative_luminance (const Color *c)
{
  real R, G, B;

  R = (c->red <= 0.03928) ? c->red / 12.92 : pow((c->red+0.055)/1.055, 2.4);
  G = (c->green <= 0.03928) ? c->green / 12.92 : pow((c->green+0.055)/1.055, 2.4);
  B = (c->blue <= 0.03928) ? c->blue / 12.92 : pow((c->blue+0.055)/1.055, 2.4);

  return 0.2126 * R + 0.7152 * G + 0.0722 * B;
}
static void 
draw_text_line (DiaRenderer *self, TextLine *text_line,
		Point *pos, Alignment alignment, Color *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  DiaCairoInteractiveRenderer *interactive = DIA_CAIRO_INTERACTIVE_RENDERER (self);

  if (interactive->highlight_color) {
    /* the high_light color is just taken as a hint, alternative needs
     * to have some contrast to cursor color (curently hard coded black)
     */
    static Color alternate_color = { 0.5, 0.5, 0.4 };
    real rl, cr1, cr2;

    /* just draw the box */
    real h = text_line_get_height (text_line);
    real w = text_line_get_width (text_line);
    real x = pos->x;
    real y = pos->y;

    y -= text_line_get_ascent(text_line);    
    x -= text_line_get_alignment_adjustment (text_line, alignment);

    rl = calculate_relative_luminance (color) + 0.05;
    cr1 = calculate_relative_luminance (interactive->highlight_color) + 0.05;
    cr1 = (cr1 > rl) ? cr1 / rl : rl / cr1;
    cr2 = calculate_relative_luminance (&alternate_color) + 0.05;
    cr2 = (cr2 > rl) ? cr2 / rl : rl / cr2;

    /* use color giving the better contrast ratio, if necessary 
     * http://www.w3.org/TR/2008/REC-WCAG20-20081211/#visual-audio-contrast-contrast
     */
    if (cr1 > cr2)
      cairo_set_source_rgba (renderer->cr, 
			   interactive->highlight_color->red,
			   interactive->highlight_color->green,
			   interactive->highlight_color->blue,
			   1.0);
    else
      cairo_set_source_rgba (renderer->cr, 
			   alternate_color.red,
			   alternate_color.green,
			   alternate_color.blue,
			   1.0);

    cairo_rectangle (renderer->cr, x, y, w, h);
    cairo_fill (renderer->cr);
  }
  DIA_RENDERER_CLASS (parent_class)->draw_text_line (self, text_line, pos, alignment, color);
}
static void
draw_object_highlighted (DiaRenderer     *self, 
			 DiaObject       *object,
			 DiaHighlightType type)
{
  DiaCairoInteractiveRenderer *interactive = DIA_CAIRO_INTERACTIVE_RENDERER (self);

  switch (type) {
  case DIA_HIGHLIGHT_TEXT_EDIT:
    interactive->highlight_color = &text_edit_color;
    break;
  case DIA_HIGHLIGHT_CONNECTIONPOINT_MAIN:
  case DIA_HIGHLIGHT_CONNECTIONPOINT:
  case DIA_HIGHLIGHT_NONE:
    interactive->highlight_color = NULL;
    break;
  }
  /* usually this method would need to draw the object twice,
   * once with highlight and once without. But due to our
   * draw_text_line implementation we only need one run */
  object->ops->draw(object, self);
  /* always reset when done with this object */
  interactive->highlight_color = NULL;
}
static void 
dia_cairo_interactive_renderer_iface_init (DiaInteractiveRendererInterface* iface)
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

GType
dia_cairo_interactive_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (DiaCairoInteractiveRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) cairo_interactive_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (DiaCairoInteractiveRenderer),
        0,              /* n_preallocs */
	(GInstanceInitFunc) cairo_interactive_renderer_init /* init */
      };

      static const GInterfaceInfo irenderer_iface_info = 
      {
	(GInterfaceInitFunc) dia_cairo_interactive_renderer_iface_init,
	NULL,           /* iface_finalize */
	NULL            /* iface_data     */
      };

      object_type = g_type_register_static (DIA_TYPE_CAIRO_RENDERER,
                                            "DiaCairoInteractiveRenderer",
                                            &object_info, 0);

      /* register the interactive renderer interface */
      g_type_add_interface_static (object_type,
                                   DIA_TYPE_INTERACTIVE_RENDERER_INTERFACE,
                                   &irenderer_iface_info);

    }
  
  return object_type;
}

static void
begin_render(DiaRenderer *self, const Rectangle *update)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (self);
  DiaCairoRenderer *base_renderer = DIA_CAIRO_RENDERER (self);

  g_return_if_fail (base_renderer->cr == NULL);
#if DIA_CAIRO_WITH_PIXMAP
  base_renderer->cr = gdk_cairo_create(renderer->pixmap);
#else
  base_renderer->cr = cairo_create(renderer->pixmap);
#endif

  /* Setup clipping for this sequence of render operations */
  /* Must be done before the scaling because the clip is in pixel coords */
  gdk_cairo_region (base_renderer->cr, renderer->clip_region);
  cairo_clip(base_renderer->cr); 

  cairo_scale (base_renderer->cr, *renderer->zoom_factor, *renderer->zoom_factor);
  cairo_translate (base_renderer->cr, -renderer->visible->left, -renderer->visible->top);

  /* second clipping */
  if (update) {
    real width = update->right - update->left;
    real height = update->bottom - update->top;
    cairo_rectangle (base_renderer->cr, update->left, update->top, width, height);
    cairo_clip (base_renderer->cr);
  }
#ifdef HAVE_PANGOCAIRO_H
  base_renderer->layout = pango_cairo_create_layout (base_renderer->cr);
#endif

  cairo_set_fill_rule (base_renderer->cr, CAIRO_FILL_RULE_EVEN_ODD);

#if !DIA_CAIRO_WITH_PIXMAP
  /* should we set the background color? Or do nothing at all? */
  /* if this is drawn you can see 'clipping in action', outside of the clip it gets yellow ;) */
  cairo_set_source_rgba (base_renderer->cr, 1.0, 1.0, .8, 1.0);
  cairo_set_operator (base_renderer->cr, CAIRO_OPERATOR_OVER);
  cairo_rectangle (base_renderer->cr, 0, 0, renderer->width, renderer->height);
  cairo_fill (base_renderer->cr);
#endif
}

static void
end_render(DiaRenderer *self)
{
  DiaCairoRenderer *base_renderer = DIA_CAIRO_RENDERER (self);

  cairo_show_page (base_renderer->cr);
  cairo_destroy (base_renderer->cr);
  base_renderer->cr = NULL;
}

static void
cairo_interactive_renderer_class_init (DiaCairoInteractiveRendererClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = cairo_interactive_renderer_finalize;

  gobject_class->set_property = cairo_interactive_renderer_set_property;
  gobject_class->get_property = cairo_interactive_renderer_get_property;

  g_object_class_install_property (gobject_class,
				   PROP_ZOOM,
				   g_param_spec_pointer ("zoom",
 							_("Zoom pointer"),
							_("Zoom pointer"),
							G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
				   PROP_RECT,
				   g_param_spec_pointer ("rect",
 							_("Visible rect pointer"),
							_("Visible rect pointer"),
							G_PARAM_READWRITE));
							
  /* renderer members */
  renderer_class->get_width_pixels  = get_width_pixels;
  renderer_class->get_height_pixels = get_height_pixels;

  renderer_class->begin_render = begin_render;
  renderer_class->end_render   = end_render;

  /* mostly for cursor placement */
  renderer_class->get_text_width = get_text_width;
  /* highlight for text editing is special */
  renderer_class->draw_text_line = draw_text_line;
}

static void
cairo_interactive_renderer_set_property (GObject         *object,
			 guint            prop_id,
			 const GValue    *value,
			 GParamSpec      *pspec)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);

  switch (prop_id) {
    case PROP_ZOOM:
      renderer->zoom_factor = g_value_get_pointer(value);
      break;
    case PROP_RECT:
      renderer->visible = g_value_get_pointer(value);
      break;
    default:
      break;
    }
}

static void
cairo_interactive_renderer_get_property (GObject         *object,
			 guint            prop_id,
			 GValue          *value,
			 GParamSpec      *pspec)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);
  
  switch (prop_id) {
    case PROP_ZOOM:
      g_value_set_pointer (value, renderer->zoom_factor);
      break;
    case PROP_RECT:
      g_value_set_pointer (value, renderer->visible);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
set_size(DiaRenderer *object, gpointer window,
		      int width, int height)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);
  DiaCairoRenderer *base_renderer = DIA_CAIRO_RENDERER (object);

  renderer->width = width;
  renderer->height = height;
#if DIA_CAIRO_WITH_PIXMAP
  if (renderer->pixmap != NULL)
    g_object_unref(renderer->pixmap);

  /* TODO: we can probably get rid of this extra pixmap and just draw directly
   * to what gdk_cairo_create() gives us for the window
   */
  renderer->pixmap = gdk_pixmap_new(GDK_WINDOW(window),  width, height, -1);
#else
# if GTK_CHECK_VERSION(2,22,0)
  renderer->pixmap = gdk_window_create_similar_surface (GDK_WINDOW (window),
							CAIRO_CONTENT_COLOR,
							width, height);
# else
  {
    cairo_rectangle_t extents;

    extents.x = 0;
    extents.y = 0;
    extents.width = width;
    extents.height = height;
    renderer->pixmap = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, &extents);
  }
# endif
#endif

  if (base_renderer->surface != NULL)
    cairo_surface_destroy(base_renderer->surface);
#if DIA_CAIRO_WITH_PIXMAP
  if (renderer->gc == NULL)
    renderer->gc = gdk_gc_new(renderer->pixmap);
#endif
}

static void
copy_to_window (DiaRenderer *object, gpointer window,
                int x, int y, int width, int height)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);
#if DIA_CAIRO_WITH_PIXMAP
  static GdkGC *copy_gc = NULL;
  
  if (!copy_gc)
    copy_gc = gdk_gc_new(window);

  gdk_draw_drawable (GDK_WINDOW(window),
                     copy_gc,
                     renderer->pixmap,
                     x, y,
                     x, y,
                     width > 0 ? width : -width, height > 0 ? height : -height);
#else
  cairo_t *cr;

  cr = gdk_cairo_create (GDK_WINDOW(window));
  cairo_set_source_surface (cr, renderer->pixmap, 0.0, 0.0);
  cairo_rectangle (cr, x, y, width > 0 ? width : -width, height > 0 ? height : -height);
  cairo_clip (cr);
  cairo_paint (cr);
  cairo_destroy (cr);
#endif
}

static void
clip_region_clear(DiaRenderer *object)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);

  if (renderer->clip_region != NULL)
    gdk_region_destroy(renderer->clip_region);

  renderer->clip_region =  gdk_region_new();
#if DIA_CAIRO_WITH_PIXMAP
  gdk_gc_set_clip_region(renderer->gc, renderer->clip_region);
#endif
}

static void
clip_region_add_rect(DiaRenderer *object,
		 Rectangle *rect)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);
  GdkRectangle clip_rect;
  int x1,y1;
  int x2,y2;

  DiaTransform *transform;        /* Our link to the display settings */

  transform = dia_transform_new(renderer->visible,renderer->zoom_factor);
  dia_transform_coords(transform, rect->left, rect->top,  &x1, &y1);
  dia_transform_coords(transform, rect->right, rect->bottom,  &x2, &y2);
  g_object_unref(transform);
  
  clip_rect.x = x1;
  clip_rect.y = y1;
  clip_rect.width = x2 - x1 + 1;
  clip_rect.height = y2 - y1 + 1;

  gdk_region_union_with_rect(renderer->clip_region, &clip_rect);
#if DIA_CAIRO_WITH_PIXMAP
  gdk_gc_set_clip_region(renderer->gc, renderer->clip_region);
#endif
}

static void
draw_pixel_line(DiaRenderer *object,
		int x1, int y1,
		int x2, int y2,
		Color *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (object);
  double x1u = x1 + .5, y1u = y1 + .5, x2u = x2 + .5, y2u = y2 + .5;
  double lw[2];
  lw[0] = 1; lw[1] = 0;
  
  cairo_device_to_user_distance (renderer->cr, &lw[0], &lw[1]);
  cairo_set_line_width (renderer->cr, lw[0]);

  cairo_device_to_user (renderer->cr, &x1u, &y1u);
  cairo_device_to_user (renderer->cr, &x2u, &y2u);

  cairo_set_source_rgba (renderer->cr, color->red, color->green, color->blue, color->alpha);
  cairo_move_to (renderer->cr, x1u, y1u);
  cairo_line_to (renderer->cr, x2u, y2u);
  cairo_stroke (renderer->cr);
}

static void
draw_pixel_rect(DiaRenderer *object,
		int x, int y,
		int width, int height,
		Color *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (object);
  double x1u = x + .5, y1u = y + .5, x2u = x + width + .5, y2u = y + height + .5;
  double lw[2];
  lw[0] = 1; lw[1] = 0;
  
  cairo_device_to_user_distance (renderer->cr, &lw[0], &lw[1]);
  cairo_set_line_width (renderer->cr, lw[0]);

  cairo_device_to_user (renderer->cr, &x1u, &y1u);
  cairo_device_to_user (renderer->cr, &x2u, &y2u);

  cairo_set_source_rgba (renderer->cr, color->red, color->green, color->blue, color->alpha);
  cairo_rectangle (renderer->cr, x1u, y1u, x2u - x1u, y2u - y1u);
  cairo_stroke (renderer->cr);
}

static void
fill_pixel_rect(DiaRenderer *object,
		int x, int y,
		int width, int height,
		Color *color)
{
#if DIA_CAIRO_WITH_PIXMAP
  /* if we do it with cairo there is something wrong with the clipping? */
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);
  GdkGC *gc = renderer->gc;
  GdkColor gdkcolor;
    
  color_convert(color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);

  gdk_draw_rectangle (renderer->pixmap, gc, TRUE,
		      x, y,  width, height);
#else
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (object);
  double x1u = x + .5, y1u = y + .5, x2u = x + width + .5, y2u = y + height + .5;
  double lw[2];
  lw[0] = 1; lw[1] = 0;
  
  cairo_device_to_user_distance (renderer->cr, &lw[0], &lw[1]);
  cairo_set_line_width (renderer->cr, lw[0]);

  cairo_device_to_user (renderer->cr, &x1u, &y1u);
  cairo_device_to_user (renderer->cr, &x2u, &y2u);

  cairo_set_source_rgba (renderer->cr, color->red, color->green, color->blue, color->alpha);
  cairo_rectangle (renderer->cr, x1u, y1u, x2u - x1u, y2u - y1u);
  cairo_fill (renderer->cr);
#endif
}

