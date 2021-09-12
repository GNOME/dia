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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "dia-guide-dialog.h"
#include "diaoptionmenu.h"


typedef struct _DiaGuideDialogPrivate DiaGuideDialogPrivate;
struct _DiaGuideDialogPrivate {
  Diagram   *diagram;
  GtkWidget *position_entry;
  GtkWidget *orientation_menu;
};

G_DEFINE_TYPE_WITH_PRIVATE (DiaGuideDialog, dia_guide_dialog, GTK_TYPE_DIALOG)

enum {
  PROP_0,
  PROP_DIAGRAM,
  LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
dia_guide_dialog_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  DiaGuideDialog *self = DIA_GUIDE_DIALOG (object);
  DiaGuideDialogPrivate *priv = dia_guide_dialog_get_instance_private (self);

  switch (property_id) {
    case PROP_DIAGRAM:
    g_clear_object (&priv->diagram);
      priv->diagram = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_guide_dialog_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  DiaGuideDialog *self = DIA_GUIDE_DIALOG (object);
  DiaGuideDialogPrivate *priv = dia_guide_dialog_get_instance_private (self);

  switch (property_id) {
    case PROP_DIAGRAM:
      g_value_set_object (value, priv->diagram);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_guide_dialog_finalize (GObject *object)
{
  DiaGuideDialog *self = DIA_GUIDE_DIALOG (object);
  DiaGuideDialogPrivate *priv = dia_guide_dialog_get_instance_private (self);

  g_clear_object (&priv->diagram);

  G_OBJECT_CLASS (dia_guide_dialog_parent_class)->finalize (object);
}


static void
dia_guide_dialog_response (GtkDialog *dialog,
                           int        response_id)
{
  DiaGuideDialog *self = DIA_GUIDE_DIALOG (dialog);
  DiaGuideDialogPrivate *priv = dia_guide_dialog_get_instance_private (self);

  if (response_id == GTK_RESPONSE_OK) {
    real position = gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->position_entry));
    int orientation = dia_option_menu_get_active (DIA_OPTION_MENU (priv->orientation_menu));
    dia_diagram_add_guide (priv->diagram, position, orientation, TRUE);
  }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
dia_guide_dialog_class_init (DiaGuideDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->set_property = dia_guide_dialog_set_property;
  object_class->get_property = dia_guide_dialog_get_property;
  object_class->finalize = dia_guide_dialog_finalize;

  dialog_class->response = dia_guide_dialog_response;

  /**
   * DiaGuideDialog:diagram:
   *
   * Since: 0.98
   */
  pspecs[PROP_DIAGRAM] =
    g_param_spec_object ("diagram",
                         "Diagram",
                         "The current diagram",
                         DIA_TYPE_DIAGRAM,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}

static void
dia_guide_dialog_init (DiaGuideDialog *self)
{
  DiaGuideDialogPrivate *priv = dia_guide_dialog_get_instance_private (self);
  GtkWidget *dialog_vbox;
  GtkWidget *label;
  GtkAdjustment *adj;
  GtkWidget *grid;
  const gdouble UPPER_LIMIT = G_MAXDOUBLE;

  gtk_window_set_title (GTK_WINDOW (self), _("Add New Guide"));
  gtk_window_set_role (GTK_WINDOW (self), "new_guide");

  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          _("_Cancel"), GTK_RESPONSE_CANCEL,
                          _("_OK"), GTK_RESPONSE_OK,
                          NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);

  dialog_vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));

  grid = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);

  label = gtk_label_new (_("Orientation"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_widget_show (label);

  priv->orientation_menu = dia_option_menu_new ();
  gtk_grid_attach (GTK_GRID (grid), priv->orientation_menu, 1, 0, 1, 1);
  dia_option_menu_add_item (DIA_OPTION_MENU (priv->orientation_menu), _("Horizontal"), GTK_ORIENTATION_HORIZONTAL);
  dia_option_menu_add_item (DIA_OPTION_MENU (priv->orientation_menu), _("Vertical"), GTK_ORIENTATION_VERTICAL);
  dia_option_menu_set_active (DIA_OPTION_MENU (priv->orientation_menu), GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_show (priv->orientation_menu);

  label = gtk_label_new (_("Position"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_widget_show (label);

  adj = GTK_ADJUSTMENT (gtk_adjustment_new (1.0, 0.0, UPPER_LIMIT, 0.1, 10.0, 0));
  priv->position_entry = gtk_spin_button_new (adj, 1.0, 3);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (priv->position_entry), TRUE);
  gtk_grid_attach (GTK_GRID (grid), priv->position_entry, 1, 1, 1, 1);
  gtk_widget_set_hexpand (priv->position_entry, TRUE);
  gtk_widget_show (priv->position_entry);

  gtk_widget_show (grid);

  gtk_box_pack_start (GTK_BOX (dialog_vbox), grid, TRUE, TRUE, 0);
  gtk_widget_show (dialog_vbox);
}

GtkWidget *
dia_guide_dialog_new (GtkWindow *parent, Diagram *dia)
{
  return g_object_new (DIA_TYPE_GUIDE_DIALOG,
                       "transient-for", parent,
                      "diagram", dia,
                      NULL);
}
