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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "dia-props.h"

#include <gtk/gtk.h>

#include "intl.h"
#include "display.h"
#include "widgets.h"
#include "display.h"

static GtkWidget *dialog = NULL;
static GtkWidget *width_x_entry, *width_y_entry;
static GtkWidget *visible_x_entry, *visible_y_entry;
static GtkWidget *bg_colour;

static void diagram_properties_respond(GtkWidget *widget,
                                       gint response_id,
                                       gpointer user_data);

static void
create_diagram_properties_dialog(void)
{
  GtkWidget *dialog_vbox;
  GtkWidget *notebook;
  GtkWidget *table;
  GtkWidget *label;
  GtkAdjustment *adj;

  dialog = gtk_dialog_new_with_buttons(
             _("Diagram Properties"),
             GTK_WINDOW(ddisplay_active()->shell),
             GTK_DIALOG_DESTROY_WITH_PARENT,
             GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
             GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
             GTK_STOCK_OK, GTK_RESPONSE_OK,
             NULL);

  //GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_OK);

  dialog_vbox = GTK_DIALOG(dialog)->vbox;

  gtk_window_set_role(GTK_WINDOW(dialog), "diagram_properties");

  g_signal_connect(G_OBJECT(dialog), "response",
		   G_CALLBACK(diagram_properties_respond),
		   NULL);
  g_signal_connect(G_OBJECT(dialog), "delete_event",
		   G_CALLBACK(gtk_widget_hide), NULL);

  notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
  gtk_box_pack_start(GTK_BOX(dialog_vbox), notebook, TRUE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(notebook), 2);
  gtk_widget_show(notebook);

  /* the grid page */
  table = gtk_table_new(3,3,FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 2);
  gtk_table_set_row_spacings(GTK_TABLE(table), 1);
  gtk_table_set_col_spacings(GTK_TABLE(table), 2);

  label = gtk_label_new(_("x"));
  gtk_table_attach(GTK_TABLE(table), label, 1,2, 0,1,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);
  label = gtk_label_new(_("y"));
  gtk_table_attach(GTK_TABLE(table), label, 2,3, 0,1,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  label = gtk_label_new(_("Spacing"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 1,2,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0.0, 10.0, 0.1, 10.0, 10.0));
  width_x_entry = gtk_spin_button_new(adj, 1.0, 3);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(width_x_entry), TRUE);
  gtk_table_attach(GTK_TABLE(table), width_x_entry, 1,2, 1,2,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(width_x_entry);

  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0.0, 10.0, 0.1, 10.0, 10.0));
  width_y_entry = gtk_spin_button_new(adj, 1.0, 3);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(width_y_entry), TRUE);
  gtk_table_attach(GTK_TABLE(table), width_y_entry, 2,3, 1,2,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(width_y_entry);

  label = gtk_label_new(_("Visible spacing"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 2,3,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0.0, 100.0, 1.0, 10.0, 10.0));
  visible_x_entry = gtk_spin_button_new(adj, 1.0, 0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(visible_x_entry), TRUE);
  gtk_table_attach(GTK_TABLE(table), visible_x_entry, 1,2, 2,3,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(visible_x_entry);

  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0.0, 100.0, 1.0, 10.0, 10.0));
  visible_y_entry = gtk_spin_button_new(adj, 1.0, 0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(visible_y_entry), TRUE);
  gtk_table_attach(GTK_TABLE(table), visible_y_entry, 2,3, 2,3,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(visible_y_entry);

  label = gtk_label_new(_("Grid"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, label);
  gtk_widget_show(table);
  gtk_widget_show(label);

  table = gtk_table_new(1,2, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 2);
  gtk_table_set_row_spacings(GTK_TABLE(table), 1);
  gtk_table_set_col_spacings(GTK_TABLE(table), 2);

  label = gtk_label_new(_("Background Colour"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 0,1,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  bg_colour = dia_color_selector_new();
  gtk_table_attach(GTK_TABLE(table), bg_colour, 1,2, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(bg_colour);

  label = gtk_label_new(_("Background"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, label);
  gtk_widget_show(table);
  gtk_widget_show(label);
}

/* diagram_properties_retrieve
 * Retrieves properties of a diagram *dia and sets the values in the
 * diagram properties dialog.
 */
static void diagram_properties_retrieve(Diagram *dia)
{
  g_return_if_fail(dia != NULL);

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(width_x_entry),
			      dia->data->grid.width_x);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(width_y_entry),
			      dia->data->grid.width_y);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(visible_x_entry),
			      dia->data->grid.visible_x);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(visible_y_entry),
			      dia->data->grid.visible_y);
  dia_color_selector_set_color(DIACOLORSELECTOR(bg_colour), &dia->data->bg_color);

}
void
diagram_properties_show(Diagram *dia)
{
  if (!dialog)
    create_diagram_properties_dialog();
 
  diagram_properties_retrieve(dia);
  
  gtk_widget_show(dialog);
}

static void
diagram_properties_respond(GtkWidget *widget,
                           gint       response_id,
                           gpointer   user_data)
{
  Diagram *active_diagram = ddisplay_active_diagram();

  if (response_id != GTK_RESPONSE_OK ||
      response_id != GTK_RESPONSE_APPLY) {
    if (active_diagram) {
      active_diagram->data->grid.width_x =
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(width_x_entry));
      active_diagram->data->grid.width_y =
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(width_y_entry));
      active_diagram->data->grid.visible_x =
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(visible_x_entry));
      active_diagram->data->grid.visible_y =
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(visible_y_entry));
      dia_color_selector_get_color(DIACOLORSELECTOR(bg_colour),
  				 &active_diagram->data->bg_color);
      diagram_add_update_all(active_diagram);
      diagram_flush(active_diagram);
    }
  }
  if (response_id != GTK_RESPONSE_APPLY)
    gtk_widget_hide(dialog);
}

/* diagram_properties_set_diagram
 * Called when the active diagram is changed. It updates the contents
 * of the diagram properties dialog
 */
void
diagram_properties_set_diagram(Diagram *dia)
{
  if (dialog && dia != NULL)
  {
    diagram_properties_retrieve(dia);
  }
}

