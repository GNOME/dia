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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "intl.h"
#include "new_guide_dialog.h"
#include "object_ops.h"
#include "object.h"
#include "connectionpoint_ops.h"
#include "undo.h"
#include "message.h"
#include "properties.h"
#include "diaoptionmenu.h"

static GtkWidget *dialog = NULL;
static GtkWidget *position_entry, *orientation_menu;
static Diagram *current_diagram = NULL;

static void
diagram_new_guide_respond(GtkWidget *widget,
                         gint       response_id,
                         gpointer   user_data)
{
  if (response_id == GTK_RESPONSE_OK) {
    real position = gtk_spin_button_get_value( GTK_SPIN_BUTTON(position_entry) );
    int orientation = dia_option_menu_get_active(orientation_menu);
    diagram_add_guide(current_diagram, position, orientation, TRUE);
  }

  /* Hide the dialog if "OK" or "Cancel" were clicked. */
  if (response_id != GTK_RESPONSE_APPLY)
    gtk_widget_hide(dialog);
}

static void
create_new_guide_dialog(Diagram *dia)
{
  GtkWidget *dialog_vbox;
  GtkWidget *label;
  GtkAdjustment *adj;
  GtkWidget *table;
  const gdouble UPPER_LIMIT = G_MAXDOUBLE;

  current_diagram = dia;

  dialog = gtk_dialog_new_with_buttons(
             _("Add New Guide"),
             GTK_WINDOW(ddisplay_active()->shell),
             GTK_DIALOG_DESTROY_WITH_PARENT,
             GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
             GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
             GTK_STOCK_OK, GTK_RESPONSE_OK,
             NULL);

  gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_OK);

  dialog_vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  gtk_window_set_role(GTK_WINDOW(dialog), "new_guide");

  g_signal_connect(G_OBJECT(dialog), "response",
		   G_CALLBACK(diagram_new_guide_respond), NULL);

  g_signal_connect(G_OBJECT(dialog), "delete_event",
		   G_CALLBACK(gtk_widget_hide), NULL);
  g_signal_connect(G_OBJECT(dialog), "destroy",
		   G_CALLBACK(gtk_widget_destroyed), &dialog);

  table = gtk_table_new(3,3,FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 2);
  gtk_table_set_row_spacings(GTK_TABLE(table), 1);
  gtk_table_set_col_spacings(GTK_TABLE(table), 2);

  label = gtk_label_new(_("Orientation: "));
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 0,1,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  orientation_menu = dia_option_menu_new();
  gtk_table_attach(GTK_TABLE(table), orientation_menu, 1,2, 0,1,
		   GTK_FILL, GTK_FILL, 0, 0);
  dia_option_menu_add_item (orientation_menu, "Horizontal", GTK_ORIENTATION_HORIZONTAL);
  dia_option_menu_add_item (orientation_menu, "Vertical", GTK_ORIENTATION_VERTICAL);
  dia_option_menu_set_active (orientation_menu, GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_show(orientation_menu);

  label = gtk_label_new(_("Position: "));
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 1,2,
		   GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0.0, UPPER_LIMIT, 0.1, 10.0, 0));
  position_entry = gtk_spin_button_new(adj, 1.0, 3);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(position_entry), TRUE);
  gtk_table_attach(GTK_TABLE(table), position_entry, 1,2, 1,2,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show(position_entry);

  gtk_widget_show(table);

  gtk_box_pack_start(GTK_BOX(dialog_vbox), table, TRUE, TRUE, 0);
  gtk_widget_show(dialog_vbox);
}

void
dialog_new_guide_show(void)
{
  Diagram *dia;
  dia = ddisplay_active_diagram();

  if(!dia)
    return;

  if (dialog) {
    /* This makes the dialog a child of the newer diagram */
    gtk_widget_destroy(dialog);
    dialog = NULL;
  }

  create_new_guide_dialog(dia);

  gtk_window_set_transient_for(GTK_WINDOW(dialog),
			       GTK_WINDOW (ddisplay_active()->shell));
  gtk_widget_show(dialog);
}
