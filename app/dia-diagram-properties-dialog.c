/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dia-props.c - a dialog for the diagram properties
 * Copyright (C) 2000 James Henstridge
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "dia-diagram-properties-dialog.h"

#include <gtk/gtk.h>

#include "display.h"
#include "undo.h"
#include "dia-colour-selector.h"


typedef struct _DiaDiagramPropertiesDialogPrivate DiaDiagramPropertiesDialogPrivate;
struct _DiaDiagramPropertiesDialogPrivate {
  Diagram *diagram;

  /* Grid */
  GtkWidget *dynamic;
  GtkWidget *manual;
  GtkWidget *manual_props;
  GtkWidget *hex;
  GtkWidget *hex_props;

  GtkAdjustment *spacing_x;
  GtkAdjustment *spacing_y;
  GtkAdjustment *vis_spacing_x;
  GtkAdjustment *vis_spacing_y;
  GtkAdjustment *hex_size;

  /* Colours */
  GtkWidget *background;
  GtkWidget *grid_lines;
  GtkWidget *page_lines;
  GtkWidget *guide_lines;
};

G_DEFINE_TYPE_WITH_PRIVATE (DiaDiagramPropertiesDialog, dia_diagram_properties_dialog, GTK_TYPE_DIALOG)

enum {
  PROP_0,
  PROP_DIAGRAM,
  LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
dia_diagram_properties_dialog_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  DiaDiagramPropertiesDialog *self = DIA_DIAGRAM_PROPERTIES_DIALOG (object);

  switch (property_id) {
    case PROP_DIAGRAM:
      dia_diagram_properties_dialog_set_diagram (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_diagram_properties_dialog_get_property (GObject    *object,
                                            guint       property_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
  DiaDiagramPropertiesDialog *self = DIA_DIAGRAM_PROPERTIES_DIALOG (object);

  switch (property_id) {
    case PROP_DIAGRAM:
      g_value_set_object (value, dia_diagram_properties_dialog_get_diagram (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
diagram_died (gpointer data, GObject *dead)
{
  DiaDiagramPropertiesDialog *self = DIA_DIAGRAM_PROPERTIES_DIALOG (data);
  DiaDiagramPropertiesDialogPrivate *priv = dia_diagram_properties_dialog_get_instance_private (self);

  g_return_if_fail (DIA_IS_DIAGRAM_PROPERTIES_DIALOG (data));

  priv->diagram = NULL;

  dia_diagram_properties_dialog_set_diagram (self, NULL);
}


static void
dia_diagram_properties_dialog_finalize (GObject *object)
{
  DiaDiagramPropertiesDialog *self = DIA_DIAGRAM_PROPERTIES_DIALOG (object);
  DiaDiagramPropertiesDialogPrivate *priv = dia_diagram_properties_dialog_get_instance_private (self);

  if (priv->diagram) {
    g_object_weak_unref (G_OBJECT (priv->diagram), diagram_died, object);
  }

  G_OBJECT_CLASS (dia_diagram_properties_dialog_parent_class)->finalize (object);
}


static gboolean
dia_diagram_properties_dialog_delete_event (GtkWidget *widget, GdkEventAny *event)
{
  gtk_widget_hide (widget);

  /* We're caching, so don't destroy */
  return TRUE;
}


static void
dia_diagram_properties_dialog_response (GtkDialog *dialog,
                                        int        response_id)
{
  DiaDiagramPropertiesDialog *self = DIA_DIAGRAM_PROPERTIES_DIALOG (dialog);
  DiaDiagramPropertiesDialogPrivate *priv = dia_diagram_properties_dialog_get_instance_private (self);

  if (response_id == GTK_RESPONSE_OK ||
      response_id == GTK_RESPONSE_APPLY) {
    if (priv->diagram) {
      /* we do not bother for the actual change, just record the
       * whole possible change */
      dia_mem_swap_change_new (priv->diagram,
                               &priv->diagram->grid,
                               sizeof(priv->diagram->grid));
      dia_mem_swap_change_new (priv->diagram,
                               &priv->diagram->data->bg_color,
                               sizeof(priv->diagram->data->bg_color));
      dia_mem_swap_change_new (priv->diagram,
                               &priv->diagram->pagebreak_color,
                               sizeof(priv->diagram->pagebreak_color));
      dia_mem_swap_change_new (priv->diagram,
                               &priv->diagram->guide_color,
                               sizeof(priv->diagram->guide_color));
      undo_set_transactionpoint (priv->diagram->undo);

      priv->diagram->grid.dynamic =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->dynamic));
      priv->diagram->grid.width_x = gtk_adjustment_get_value (priv->spacing_x);
      priv->diagram->grid.width_y = gtk_adjustment_get_value (priv->spacing_y);
      priv->diagram->grid.visible_x = gtk_adjustment_get_value (priv->vis_spacing_x);
      priv->diagram->grid.visible_y = gtk_adjustment_get_value (priv->vis_spacing_y);
      priv->diagram->grid.hex =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->hex));
      priv->diagram->grid.hex_size = gtk_adjustment_get_value (priv->hex_size);
      dia_colour_selector_get_colour (DIA_COLOUR_SELECTOR (priv->background),
                                      &priv->diagram->data->bg_color);
      dia_colour_selector_get_colour (DIA_COLOUR_SELECTOR (priv->grid_lines),
                                      &priv->diagram->grid.colour);
      dia_colour_selector_get_colour (DIA_COLOUR_SELECTOR (priv->page_lines),
                                      &priv->diagram->pagebreak_color);
      dia_colour_selector_get_colour (DIA_COLOUR_SELECTOR (priv->guide_lines),
                                      &priv->diagram->guide_color);
      diagram_add_update_all (priv->diagram);
      diagram_flush (priv->diagram);
      diagram_set_modified (priv->diagram, TRUE);
    }
  }

  if (response_id != GTK_RESPONSE_APPLY) {
    gtk_widget_hide (GTK_WIDGET (dialog));
  }
}


static void
update_sensitivity (GtkToggleButton *widget,
                    gpointer         userdata)
{
  DiaDiagramPropertiesDialog *self = DIA_DIAGRAM_PROPERTIES_DIALOG (userdata);
  DiaDiagramPropertiesDialogPrivate *priv = dia_diagram_properties_dialog_get_instance_private (self);
  gboolean dyn_grid, square_grid, hex_grid;

  if (!priv->diagram)
    return; /* safety first */

  priv->diagram->grid.dynamic =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->dynamic));
  dyn_grid = priv->diagram->grid.dynamic;
  if (!dyn_grid) {
    priv->diagram->grid.hex =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->hex));
  }

  square_grid = !dyn_grid && !priv->diagram->grid.hex;
  hex_grid = !dyn_grid && priv->diagram->grid.hex;


  gtk_widget_set_sensitive (priv->manual_props, square_grid);
  gtk_widget_set_sensitive (priv->hex_props, hex_grid);
}


static void
dia_diagram_properties_dialog_class_init (DiaDiagramPropertiesDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->set_property = dia_diagram_properties_dialog_set_property;
  object_class->get_property = dia_diagram_properties_dialog_get_property;
  object_class->finalize = dia_diagram_properties_dialog_finalize;

  widget_class->delete_event = dia_diagram_properties_dialog_delete_event;

  dialog_class->response = dia_diagram_properties_dialog_response;

  /**
   * DiaDiagramPropertiesDialog:diagram:
   *
   * Since: 0.98
   */
  pspecs[PROP_DIAGRAM] =
    g_param_spec_object ("diagram",
                         "Diagram",
                         "The current diagram",
                         DIA_TYPE_DIAGRAM,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               DIA_APPLICATION_PATH "dia-diagram-properties-dialog.ui");

  /* Grid Page */
  gtk_widget_class_bind_template_child_private (widget_class, DiaDiagramPropertiesDialog, dynamic);
  gtk_widget_class_bind_template_child_private (widget_class, DiaDiagramPropertiesDialog, manual);
  gtk_widget_class_bind_template_child_private (widget_class, DiaDiagramPropertiesDialog, manual_props);
  gtk_widget_class_bind_template_child_private (widget_class, DiaDiagramPropertiesDialog, hex);
  gtk_widget_class_bind_template_child_private (widget_class, DiaDiagramPropertiesDialog, hex_props);
  gtk_widget_class_bind_template_child_private (widget_class, DiaDiagramPropertiesDialog, spacing_x);
  gtk_widget_class_bind_template_child_private (widget_class, DiaDiagramPropertiesDialog, spacing_y);
  gtk_widget_class_bind_template_child_private (widget_class, DiaDiagramPropertiesDialog, vis_spacing_x);
  gtk_widget_class_bind_template_child_private (widget_class, DiaDiagramPropertiesDialog, vis_spacing_y);
  gtk_widget_class_bind_template_child_private (widget_class, DiaDiagramPropertiesDialog, hex_size);

  /* The background page */
  gtk_widget_class_bind_template_child_private (widget_class, DiaDiagramPropertiesDialog, background);
  gtk_widget_class_bind_template_child_private (widget_class, DiaDiagramPropertiesDialog, grid_lines);
  gtk_widget_class_bind_template_child_private (widget_class, DiaDiagramPropertiesDialog, page_lines);
  gtk_widget_class_bind_template_child_private (widget_class, DiaDiagramPropertiesDialog, guide_lines);

  gtk_widget_class_bind_template_callback (widget_class, update_sensitivity);
}


static void
dia_diagram_properties_dialog_init (DiaDiagramPropertiesDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          _("_Close"), GTK_RESPONSE_CANCEL,
                          _("_Apply"), GTK_RESPONSE_APPLY,
                          _("_OK"), GTK_RESPONSE_OK,
                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);

  g_signal_connect (G_OBJECT (self), "destroy",
                    G_CALLBACK (gtk_widget_destroyed), &self);
}


/* diagram_properties_retrieve
 * Retrieves properties of a diagram *dia and sets the values in the
 * diagram properties dialog.
 */
void
dia_diagram_properties_dialog_set_diagram (DiaDiagramPropertiesDialog *self,
                                           Diagram                    *diagram)
{
  DiaDiagramPropertiesDialogPrivate *priv;
  gchar *title;
  gchar *name;

  g_return_if_fail (DIA_IS_DIAGRAM_PROPERTIES_DIALOG (self));

  priv = dia_diagram_properties_dialog_get_instance_private (self);

  if (priv->diagram) {
    g_object_weak_unref (G_OBJECT (priv->diagram), diagram_died, self);
    priv->diagram = NULL;
  }

  if (diagram == NULL) {
    gtk_window_set_title (GTK_WINDOW (self), _("Diagram Properties"));

    gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);

    return;
  }

  gtk_widget_set_sensitive (GTK_WIDGET (self), TRUE);

  g_object_weak_ref (G_OBJECT (diagram), diagram_died, self);
  priv->diagram = diagram;

  name = diagram ? diagram_get_name (diagram) : NULL;

  /* Can we be sure that the filename is the 'proper title'? */
  title = g_strdup_printf ("%s", name ? name : _("Diagram Properties"));
  gtk_window_set_title (GTK_WINDOW (self), title);

  g_clear_pointer (&name, g_free);
  g_clear_pointer (&title, g_free);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->dynamic),
                                diagram->grid.dynamic);

  gtk_adjustment_set_value (priv->spacing_x, diagram->grid.width_x);
  gtk_adjustment_set_value (priv->spacing_y, diagram->grid.width_y);
  gtk_adjustment_set_value (priv->vis_spacing_x, diagram->grid.visible_x);
  gtk_adjustment_set_value (priv->vis_spacing_y, diagram->grid.visible_y);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->hex),
                                diagram->grid.hex);

  gtk_adjustment_set_value (priv->hex_size, diagram->grid.hex_size);

  dia_colour_selector_set_colour (DIA_COLOUR_SELECTOR (priv->background),
                                  &diagram->data->bg_color);
  dia_colour_selector_set_colour (DIA_COLOUR_SELECTOR (priv->grid_lines),
                                  &diagram->grid.colour);
  dia_colour_selector_set_colour (DIA_COLOUR_SELECTOR (priv->page_lines),
                                  &diagram->pagebreak_color);
  dia_colour_selector_set_colour (DIA_COLOUR_SELECTOR (priv->guide_lines),
                                  &diagram->guide_color);

  update_sensitivity (GTK_TOGGLE_BUTTON (priv->dynamic), self);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_DIAGRAM]);
}


Diagram *
dia_diagram_properties_dialog_get_diagram (DiaDiagramPropertiesDialog *self)
{
  DiaDiagramPropertiesDialogPrivate *priv;

  g_return_val_if_fail (DIA_IS_DIAGRAM_PROPERTIES_DIALOG (self), NULL);

  priv = dia_diagram_properties_dialog_get_instance_private (self);

  return priv->diagram;
}


DiaDiagramPropertiesDialog *
dia_diagram_properties_dialog_get_default (void)
{
  static DiaDiagramPropertiesDialog *instance;

  if (instance == NULL) {
    instance = g_object_new (DIA_TYPE_DIAGRAM_PROPERTIES_DIALOG,
                             "title", _("Diagram Properties"),
                             NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *) &instance);
  }

  return instance;
}


void
diagram_properties_show(Diagram *dia)
{
  DiaDiagramPropertiesDialog *dialog = dia_diagram_properties_dialog_get_default ();

  dia_diagram_properties_dialog_set_diagram (dialog, dia);

  gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                GTK_WINDOW (ddisplay_active()->shell));
  gtk_widget_show (GTK_WIDGET (dialog));
}
