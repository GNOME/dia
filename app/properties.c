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

#include <gtk/gtk.h>

#include "intl.h"
#include "properties.h"
#include "object_ops.h"
#include "connectionpoint_ops.h"
#include "undo.h"
#include "message.h"

static GtkWidget *dialog = NULL;
static GtkWidget *dialog_vbox = NULL;
static GtkWidget *object_part = NULL;
static Object *current_obj = NULL;
static Diagram *current_dia = NULL;

static GtkWidget *no_properties_dialog = NULL;

static gint properties_okay(GtkWidget *canvas, gpointer data);
static gint properties_apply(GtkWidget *canvas, gpointer data);

static void create_dialog()
{
  GtkWidget *button;

  dialog = gtk_dialog_new();
  
  gtk_window_set_title(GTK_WINDOW (dialog), _("Object properties"));
  gtk_window_set_wmclass(GTK_WINDOW (dialog),
			  "properties_window", "Dia");
  gtk_window_set_policy(GTK_WINDOW (dialog),
			FALSE, TRUE, TRUE);
  gtk_container_set_border_width(GTK_CONTAINER (dialog), 2);

  dialog_vbox = GTK_DIALOG(dialog)->vbox;

  button = gtk_button_new_with_label( _("OK") );
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), 
		     button, TRUE, TRUE, 0);
  gtk_signal_connect_object(GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC(properties_okay),
			    GTK_OBJECT(dialog));
  gtk_widget_show (button);

  button = gtk_button_new_with_label(_("Apply"));
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		     button, TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT (button), "clicked",
		     GTK_SIGNAL_FUNC(properties_apply),
		     NULL);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  button = gtk_button_new_with_label( _("Close") );
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), 
		     button, TRUE, TRUE, 0);
  gtk_signal_connect_object(GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_hide),
			    GTK_OBJECT(dialog));
  gtk_widget_show (button);

  gtk_signal_connect(GTK_OBJECT (dialog), "delete_event",
		     GTK_SIGNAL_FUNC(gtk_widget_hide), NULL);

  no_properties_dialog = gtk_label_new(_("This object has no properties."));
  gtk_widget_show (no_properties_dialog);
}

static gint
properties_dialog_destroyed(GtkWidget *dialog, gpointer data)
{
  if (dialog == object_part) {
    object_part = NULL;
    current_obj = NULL;
    current_dia = NULL;
  }
  return 0;
}

static gint
properties_okay(GtkWidget *canvas, gpointer data)
{
  gint ret = properties_apply(canvas,data);
  gtk_widget_hide(canvas);
  return ret;
}

static gint
properties_apply(GtkWidget *canvas, gpointer data)
{
  ObjectChange *obj_change = NULL;

  if ( (current_obj == NULL) || (current_dia == NULL) )
    return 0;
  
  object_add_updates(current_obj, current_dia);
  obj_change = current_obj->ops->apply_properties(current_obj);
  object_add_updates(current_obj, current_dia);

  diagram_update_connections_object(current_dia, current_obj, TRUE);
  
  if (obj_change != NULL) {
    undo_object_change(current_dia, current_obj, obj_change);
  }
  
  diagram_modified(current_dia);

  diagram_update_extents(current_dia);
    
  if (obj_change != NULL) {
    undo_set_transactionpoint(current_dia->undo);
  }  else {
    message_warning(_("This object doesn't support Undo/Redo.\n"
		      "Undo information erased."));
    undo_clear(current_dia->undo);
  }

  diagram_flush(current_dia);

  return 0;
}

void
properties_show(Diagram *dia, Object *obj)
{
  GtkWidget *properties;

  if (obj != NULL) 
    properties = obj->ops->get_properties(obj);

  if (dialog == NULL)
    create_dialog();

  if ((obj==NULL) || (properties == NULL)) { /* No properties or no object */
    properties = no_properties_dialog;
    obj = NULL;
    dia = NULL;
  }

  if (object_part != NULL) {
    gtk_container_remove(GTK_CONTAINER(dialog_vbox), object_part);
    object_part = NULL;
    current_obj = NULL;
    current_dia = NULL;
  }
  gtk_signal_connect (GTK_OBJECT (properties), "destroy",
		      GTK_SIGNAL_FUNC(properties_dialog_destroyed), NULL);
  gtk_box_pack_start(GTK_BOX(dialog_vbox), properties, TRUE, TRUE, 0);
  gtk_widget_show (properties);
  gtk_widget_show (dialog);
  object_part = properties;
  current_obj = obj;
  current_dia = dia;
}

void
properties_hide_if_shown(Diagram *dia, Object *obj)
{
  if (current_obj == obj) {
    properties_show(dia, NULL);
  }
}

void
properties_update_if_shown(Diagram *dia, Object *obj)
{
  if (current_obj == obj) {
    properties_show(dia, obj);
  }
}









