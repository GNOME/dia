/* Minimal reimplementation of GtkList, prototyped in Vala */


#include <glib.h>
#include <gtk/gtk.h>

#include "dia-canvas.h"
#include "disp_callbacks.h"
#include "toolbox.h"
#include "display.h"
#include "interface.h"
#include "object.h"

typedef struct _DiaCanvasPrivate DiaCanvasPrivate;

struct _DiaCanvasPrivate {
  DDisplayBox *disp_box;
};

G_DEFINE_TYPE_WITH_CODE (DiaCanvas, dia_canvas, GTK_TYPE_DRAWING_AREA,
                         G_ADD_PRIVATE (DiaCanvas))

enum {
  PROP_DDISPLAY = 1,
  N_PROPS
};
static GParamSpec* properties[N_PROPS];


GtkWidget *
dia_canvas_new (DDisplay *ddisp)
{
  return g_object_new (DIA_TYPE_CANVAS,
                       "ddisplay", display_box_new (ddisp),
                       NULL);
}

DDisplay *
dia_canvas_get_ddisplay (DiaCanvas *self)
{
  DiaCanvasPrivate *priv;
  DDisplayBox      *box;

  g_return_val_if_fail (self != NULL, NULL);
  
  priv = dia_canvas_get_instance_private (DIA_CANVAS (self));
  box = priv->disp_box;

  return box->ddisp;
}

static gboolean
dia_canvas_drag_drop (GtkWidget      *self,
                      GdkDragContext *context,
                      gint            x,
                      gint            y,
                      guint           time)
{
  if (gtk_drag_get_source_widget (context) != NULL) {
    /* we only accept drops from the same instance of the application,
     * as the drag data is a pointer in our address space */
    return TRUE;
  }
  gtk_drag_finish (context, FALSE, FALSE, time);
  return FALSE;
}

static void
dia_canvas_drag_data_received (GtkWidget        *self,
                               GdkDragContext   *context,
                               gint              x,
                               gint              y,
                               GtkSelectionData *data,
                               guint             info,
                               guint             time)
{
  DDisplay *ddisp = dia_canvas_get_ddisplay (DIA_CANVAS (self));
  if (gtk_selection_data_get_format (data) == 8 &&
      gtk_selection_data_get_length (data) == sizeof (ToolButtonData *) &&
      gtk_drag_get_source_widget (context) != NULL) {
    ToolButtonData *tooldata = *(ToolButtonData **) gtk_selection_data_get_data (data);
    /* g_message("Tool drop %s at (%d, %d)", (gchar *)tooldata->extra_data, x, y);*/
    ddisplay_drop_object (ddisp, x, y,
                          object_get_type ((gchar *) tooldata->extra_data),
                          tooldata->user_data);

    gtk_drag_finish (context, TRUE, FALSE, time);
  } else {
    dia_dnd_file_drag_data_received (self, context, x, y, data, info, time, ddisp);
  }
  /* ensure the right window has the focus for text editing */
  gtk_window_present (GTK_WINDOW (ddisp->shell));
}

/*!
 * Called when the widget's window "size, position or stacking"
 * changes. Needs GDK_STRUCTURE_MASK set.
 */
static void
dia_canvas_size_allocate (GtkWidget     *self,
                          GtkAllocation *alloc)
{
  int width, height;
  DDisplay *ddisp = dia_canvas_get_ddisplay (DIA_CANVAS (self));

  if (ddisp->renderer) {
    width = dia_renderer_get_width_pixels (ddisp->renderer);
    height = dia_renderer_get_height_pixels (ddisp->renderer);
  } else {
    /* We can continue even without a renderer here because
     * ddisplay_resize_canvas () does the setup for us.
     */
    width = height = 0;
  }

  /* Only do this when size is really changing */
  if (width != alloc->width || height != alloc->height) {
    g_message ("Canvas size change...");
    ddisplay_resize_canvas (ddisp, alloc->width, alloc->height);
    ddisplay_update_scrollbars(ddisp);
  }

  /* If the UI is not integrated, resizing should set the resized
   * window as active.  With integrated UI, there is only one window.
   */
  if (is_integrated_ui () == 0)
    display_set_active(ddisp);

  GTK_WIDGET_CLASS (dia_canvas_parent_class)->size_allocate (self, alloc);
}

static gboolean
dia_canvas_draw (GtkWidget *self,
                 cairo_t   *ctx)
{
  GSList *l;
  Rectangle *r, totrect;
  DiaInteractiveRendererInterface *renderer;
  DDisplay *ddisp = dia_canvas_get_ddisplay (DIA_CANVAS (self));

  g_return_val_if_fail (ddisp->renderer != NULL, FALSE);

  /* Renders updates to pixmap + copies display_areas to canvas(screen) */
  renderer = DIA_GET_INTERACTIVE_RENDERER_INTERFACE (ddisp->renderer);

  /* Only update if update_areas exist */
  l = ddisp->update_areas;
  if (l != NULL)
  {
    totrect = *(Rectangle *) l->data;
  
    g_return_val_if_fail (   renderer->clip_region_clear != NULL
                          && renderer->clip_region_add_rect != NULL, FALSE);

    renderer->clip_region_clear (ddisp->renderer);

    while(l!=NULL) {
      r = (Rectangle *) l->data;

      rectangle_union(&totrect, r);
      renderer->clip_region_add_rect (ddisp->renderer, r);
      
      l = g_slist_next(l);
    }
    /* Free update_areas list: */
    l = ddisp->update_areas;
    while(l!=NULL) {
      g_free(l->data);
      l = g_slist_next(l);
    }
    g_slist_free(ddisp->update_areas);
    ddisp->update_areas = NULL;

    totrect.left -= 0.1;
    totrect.right += 0.1;
    totrect.top -= 0.1;
    totrect.bottom += 0.1;
    
    ddisplay_render_pixmap(ddisp, &totrect);
  }

  dia_interactive_renderer_paint (ddisp->renderer,
                                  ctx,
                                  gtk_widget_get_allocated_width (self),
                                  gtk_widget_get_allocated_height (self));

  return FALSE;
}

static void
dia_canvas_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  DiaCanvas *self = DIA_CANVAS (object);
  DiaCanvasPrivate *priv = dia_canvas_get_instance_private (self);
  DDisplayBox *box;
  switch (property_id) {
    case PROP_DDISPLAY:
      box = g_value_get_boxed (value);
      priv->disp_box = display_box_new (box->ddisp);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_canvas_class_init (DiaCanvasClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = dia_canvas_set_property;

  properties[PROP_DDISPLAY] =
    g_param_spec_boxed ("ddisplay",
                        "DDisplay",
                        "Editor this canvas is part of",
                        DIA_TYPE_DISPLAY,
                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE);

  g_object_class_install_properties (object_class,
                                     N_PROPS,
                                     properties);

  widget_class->drag_drop = dia_canvas_drag_drop;
  widget_class->drag_data_received = dia_canvas_drag_data_received;
  widget_class->size_allocate = dia_canvas_size_allocate;
  widget_class->draw = dia_canvas_draw;

  gtk_widget_class_set_css_name (widget_class, "dia-canvas");
}

static void
dia_canvas_init (DiaCanvas * self)
{
  gtk_widget_add_events (GTK_WIDGET (self),
                         GDK_POINTER_MOTION_MASK |
                         GDK_POINTER_MOTION_HINT_MASK |
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | 
                         GDK_ENTER_NOTIFY_MASK | GDK_KEY_PRESS_MASK |
                         GDK_KEY_RELEASE_MASK);
  gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);
  g_signal_connect (G_OBJECT (self), "event",
                    G_CALLBACK (ddisplay_canvas_events), NULL);

  canvas_setup_drag_dest (GTK_WIDGET (self));
}
