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
#include "undo.h"

static GtkWidget *dialog = NULL;
static GtkWidget *dynamic_check;
static GtkWidget *width_x_entry, *width_y_entry;
static GtkWidget *visible_x_entry, *visible_y_entry;
static GtkWidget *bg_colour, *grid_colour, *pagebreak_colour;
static GtkWidget *hex_check, *hex_size_entry;

static void diagram_properties_respond(GtkWidget *widget,
                                       gint response_id,
                                       gpointer user_data);

static void
diagram_properties_update_sensitivity(GtkToggleButton *widget,
				      gpointer userdata)
{
  Diagram *dia = ddisplay_active_diagram();
  gboolean dyn_grid, square_grid, hex_grid;

  if (!dia)
    return; /* safety first */
  
  dia->grid.dynamic =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dynamic_check));
  dyn_grid = dia->grid.dynamic;
  if (!dyn_grid)
    dia->grid.hex =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hex_check));

  square_grid = !dyn_grid && !dia->grid.hex;
  hex_grid = !dyn_grid && dia->grid.hex;


  gtk_widget_set_sensitive(width_x_entry, square_grid);
  gtk_widget_set_sensitive(width_y_entry, square_grid);
  gtk_widget_set_sensitive(visible_x_entry, square_grid);
  gtk_widget_set_sensitive(visible_y_entry, square_grid);
  gtk_widget_set_sensitive(hex_check, !dyn_grid);
  gtk_widget_set_sensitive(hex_size_entry, hex_grid);
}

static void
create_diagram_properties_dialog(Diagram *dia)
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

  gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_OK);

  dialog_vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  gtk_window_set_role(GTK_WINDOW(dialog), "diagram_properties");

  g_signal_connect(G_OBJECT(dialog), "response",
		   G_CALLBACK(diagram_properties_respond),
		   NULL);
  g_signal_connect(G_OBJECT(dialog), "delete_event",
		   G_CALLBACK(gtk_widget_hide), NULL);
  g_signal_connect(G_OBJECT(dialog), "destroy",
		   G_CALLBACK(gtk_widget_destroyed), &dialog);

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

  dynamic_check = gtk_check_button_new_with_label(_("Dynamic grid"));
  gtk_table_attach(GTK_TABLE(table), dynamic_check, 1,2, 0,1,
		   GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect(G_OBJECT(dynamic_check), "toggled", 
		   G_CALLBACK(diagram_properties_update_sensitivity), NULL);
	    
  gtk_widget_show(dynamic_check);

  label = gtk_label_new(_("x"));
  gtk_table_attach(GTK_TABLE(table), label, 1,2, 1,2,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);
  label = gtk_label_new(_("y"));
  gtk_table_attach(GTK_TABLE(table), label, 2,3, 1,2,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  label = gtk_label_new(_("Spacing"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 2,3,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0.0, 10.0, 0.1, 10.0, 0));
  width_x_entry = gtk_spin_button_new(adj, 1.0, 3);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(width_x_entry), TRUE);
  gtk_table_attach(GTK_TABLE(table), width_x_entry, 1,2, 2,3,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(width_x_entry);

  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0.0, 10.0, 0.1, 10.0, 0));
  width_y_entry = gtk_spin_button_new(adj, 1.0, 3);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(width_y_entry), TRUE);
  gtk_table_attach(GTK_TABLE(table), width_y_entry, 2,3, 2,3,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(width_y_entry);

  label = gtk_label_new(_("Visible spacing"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 3,4,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0.0, 100.0, 1.0, 10.0, 0));
  visible_x_entry = gtk_spin_button_new(adj, 1.0, 0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(visible_x_entry), TRUE);
  gtk_table_attach(GTK_TABLE(table), visible_x_entry, 1,2, 3,4,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(visible_x_entry);

  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0.0, 100.0, 1.0, 10.0, 0));
  visible_y_entry = gtk_spin_button_new(adj, 1.0, 0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(visible_y_entry), TRUE);
  gtk_table_attach(GTK_TABLE(table), visible_y_entry, 2,3, 3,4,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(visible_y_entry);

  /* Hexes! */
  hex_check = gtk_check_button_new_with_label(_("Hex grid"));
  gtk_table_attach(GTK_TABLE(table), hex_check, 1,2, 4,5,
		   GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect(G_OBJECT(hex_check), "toggled", 
		   G_CALLBACK(diagram_properties_update_sensitivity), NULL);
	    
  gtk_widget_show(hex_check);

  label = gtk_label_new(_("Hex grid size"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 5,6,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0.0, 100.0, 1.0, 10.0, 0));
  hex_size_entry = gtk_spin_button_new(adj, 1.0, 0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(hex_size_entry), TRUE);
  gtk_table_attach(GTK_TABLE(table), hex_size_entry, 1,2, 5,6,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(hex_size_entry);

  label = gtk_label_new(_("Grid"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, label);
  gtk_widget_show(table);
  gtk_widget_show(label);

  /* The background page */
  table = gtk_table_new(1,2, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 2);
  gtk_table_set_row_spacings(GTK_TABLE(table), 1);
  gtk_table_set_col_spacings(GTK_TABLE(table), 2);

  label = gtk_label_new(_("Background"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 0,1,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  bg_colour = dia_color_selector_new();
  gtk_table_attach(GTK_TABLE(table), bg_colour, 1,2, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(bg_colour);

  label = gtk_label_new(_("Grid Lines"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 1,2,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  grid_colour = dia_color_selector_new();
  gtk_table_attach(GTK_TABLE(table), grid_colour, 1,2, 1,2,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(grid_colour);

  label = gtk_label_new(_("Page Breaks"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 2,3,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  pagebreak_colour = dia_color_selector_new();
  gtk_table_attach(GTK_TABLE(table), pagebreak_colour, 1,2, 2,3,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(pagebreak_colour);

  label = gtk_label_new(_("Colors"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, label);
  gtk_widget_show(table);
  gtk_widget_show(label);
}

/* diagram_properties_retrieve
 * Retrieves properties of a diagram *dia and sets the values in the
 * diagram properties dialog.
 */
static void
diagram_properties_retrieve(Diagram *dia)
{
  gchar *title;
  gchar *name = dia ? diagram_get_name(dia) : "?";

  g_return_if_fail(dia != NULL);
  if (!dialog)
    return;

  /* Can we be sure that the filename is the 'proper title'? */
  title = g_strdup_printf(_("Diagram Properties: %s"), name ? name : "??");
  gtk_window_set_title(GTK_WINDOW(dialog), title);
  g_free(name);
  g_free(title);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dynamic_check),
			   dia->grid.dynamic);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(width_x_entry),
			    dia->grid.width_x);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(width_y_entry),
			    dia->grid.width_y);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(visible_x_entry),
			    dia->grid.visible_x);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(visible_y_entry),
			    dia->grid.visible_y);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hex_check),
			   dia->grid.hex);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(hex_size_entry),
			    dia->grid.hex_size);

  dia_color_selector_set_color(bg_colour,
			       &dia->data->bg_color);
  dia_color_selector_set_color(grid_colour,
			       &dia->grid.colour);
  dia_color_selector_set_color(pagebreak_colour, 
			       &dia->pagebreak_color);

  diagram_properties_update_sensitivity(GTK_TOGGLE_BUTTON(dynamic_check), dia);

}

void
diagram_properties_show(Diagram *dia)
{
  if (dialog) {
    /* This makes the dialog a child of the newer diagram */
    gtk_widget_destroy(dialog);
    dialog = NULL;
  }
  
  create_diagram_properties_dialog(dia);
 
  diagram_properties_retrieve(dia);
  
  gtk_window_set_transient_for(GTK_WINDOW(dialog),
			       GTK_WINDOW (ddisplay_active()->shell));
  gtk_widget_show(dialog);
}

static void
diagram_properties_respond(GtkWidget *widget,
                           gint       response_id,
                           gpointer   user_data)
{
  Diagram *active_diagram = ddisplay_active_diagram();

  if (response_id == GTK_RESPONSE_OK ||
      response_id == GTK_RESPONSE_APPLY) {
    if (active_diagram) {
      /* we do not bother for the actual change, just record the 
       * whole possible change */
      undo_change_memswap (active_diagram, 
        &active_diagram->grid, sizeof(active_diagram->grid));
      undo_change_memswap (active_diagram, 
        &active_diagram->data->bg_color, sizeof(active_diagram->data->bg_color));
      undo_change_memswap (active_diagram, 
        &active_diagram->pagebreak_color, sizeof(active_diagram->pagebreak_color));
      undo_set_transactionpoint(active_diagram->undo);

      active_diagram->grid.dynamic =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dynamic_check));
      active_diagram->grid.width_x =
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(width_x_entry));
      active_diagram->grid.width_y =
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(width_y_entry));
      active_diagram->grid.visible_x =
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(visible_x_entry));
      active_diagram->grid.visible_y =
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(visible_y_entry));
      active_diagram->grid.hex =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hex_check));
      active_diagram->grid.hex_size =
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(hex_size_entry));
      dia_color_selector_get_color(bg_colour,
  				 &active_diagram->data->bg_color);
      dia_color_selector_get_color(grid_colour,
  				 &active_diagram->grid.colour);
      dia_color_selector_get_color(pagebreak_colour,
  				 &active_diagram->pagebreak_color);
      diagram_add_update_all(active_diagram);
      diagram_flush(active_diagram);
      diagram_set_modified(active_diagram, TRUE);
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
