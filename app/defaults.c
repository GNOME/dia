/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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

#include <gtk/gtk.h>

#include "properties.h"
#include "object_ops.h"
#include "connectionpoint_ops.h"

static GtkWidget *dialog = NULL;
static GtkWidget *dialog_vbox = NULL;
static GtkWidget *object_part = NULL;
static ObjectType *current_objtype = NULL;

static GtkWidget *no_defaults_dialog = NULL;

static gint defaults_apply(GtkWidget *canvas, gpointer data);
static gint defaults_close(GtkWidget *canvas, gpointer data);

static void create_dialog()
{
  GtkWidget *button;

  dialog = gtk_dialog_new();
  
  gtk_window_set_title (GTK_WINDOW (dialog), "Object defaults");
  gtk_window_set_wmclass (GTK_WINDOW (dialog),
			  "defaults_window", "Dia");
  gtk_window_set_policy (GTK_WINDOW (dialog),
			 TRUE, TRUE, TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 2);

  dialog_vbox = GTK_DIALOG (dialog)->vbox;
    
  button = gtk_button_new_with_label ("Close");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(defaults_close),
		      NULL);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  button = gtk_button_new_with_label ("Apply");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(defaults_apply),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
		      GTK_SIGNAL_FUNC(gtk_widget_hide), NULL);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  no_defaults_dialog = gtk_label_new("This object has no defaults.");
  gtk_widget_show (no_defaults_dialog);
}

static gint
defaults_dialog_destroyed(GtkWidget *dialog, gpointer data)
{
  if (dialog == object_part) {
    object_part = NULL;
    current_objtype = NULL;
  }
  return 0;
}

static gint
defaults_apply(GtkWidget *canvas, gpointer data)
{
  if (current_objtype != NULL) {
    current_objtype->ops->apply_defaults();
  }
  return 0;
}

static gint
defaults_close(GtkWidget *canvas, gpointer data)
{
  gtk_widget_hide(dialog);
  return 0;
}

void
defaults_show(ObjectType *objtype)
{
  GtkWidget *defaults;

  if ((objtype != NULL) && (objtype->ops->get_defaults != NULL)) {
    defaults = objtype->ops->get_defaults();
  } else {
    defaults = NULL;
  }
  
  if (dialog == NULL)
    create_dialog();

  if ((objtype==NULL) || (defaults == NULL)) { 
    /* No defaults or no object */
    defaults = no_defaults_dialog;
    objtype = NULL;
  }

  if (object_part != NULL) {
    gtk_widget_ref(object_part);
    gtk_container_remove(GTK_CONTAINER(dialog_vbox), object_part);
    gtk_widget_unparent(object_part);
    object_part = NULL;
  }
  gtk_signal_connect (GTK_OBJECT (defaults), "destroy",
		      GTK_SIGNAL_FUNC(defaults_dialog_destroyed), NULL);
  gtk_box_pack_start(GTK_BOX(dialog_vbox), defaults, TRUE, TRUE, 0);
  gtk_widget_show (defaults);
  gtk_widget_show (dialog);
  object_part = defaults;
  current_objtype = objtype;
}
