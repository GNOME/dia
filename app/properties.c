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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "intl.h"
#include "properties.h"
#include "object_ops.h"
#include "connectionpoint_ops.h"
#include "undo.h"
#include "message.h"
#include <string.h>

static GtkWidget *dialog = NULL;
static GtkWidget *dialog_vbox = NULL;
static GtkWidget *object_part = NULL;
static Object *current_obj = NULL;
static Diagram *current_dia = NULL;

static GtkWidget *no_properties_dialog = NULL;

static gint properties_respond(GtkWidget *widget, 
                               gint       response_id,
                               gpointer   data);
static gboolean properties_key_event(GtkWidget *widget,
				     GdkEventKey *event,
				     gpointer data);

static void create_dialog()
{
  GtkWidget *actionbox;
  GList *buttons;

  dialog = gtk_dialog_new_with_buttons(
             _("Object properties"),
             GTK_WINDOW (ddisplay_active()->shell), 
             GTK_DIALOG_DESTROY_WITH_PARENT,
             GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
             GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
             GTK_STOCK_OK, GTK_RESPONSE_OK,
             NULL);

  //GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_OK);

  dialog_vbox = GTK_DIALOG(dialog)->vbox;

  gtk_window_set_role(GTK_WINDOW (dialog), "properties_window");

  g_signal_connect(G_OBJECT (dialog), "response",
                   G_CALLBACK (properties_respond), NULL);
  g_signal_connect(G_OBJECT (dialog), "delete_event",
		   G_CALLBACK(gtk_widget_hide), NULL);
  g_signal_connect(G_OBJECT (dialog), "destroy",
		   G_CALLBACK(gtk_widget_destroyed), &dialog);
  g_signal_connect(G_OBJECT (dialog), "destroy",
		   G_CALLBACK(gtk_widget_destroyed), &dialog_vbox);
  g_signal_connect(G_OBJECT (dialog), "key-release-event",
		   G_CALLBACK(properties_key_event), NULL);

  no_properties_dialog = gtk_label_new(_("This object has no properties."));
  gtk_widget_show (no_properties_dialog);
  g_object_ref(G_OBJECT(no_properties_dialog)); 
  gtk_object_sink(GTK_OBJECT(no_properties_dialog));
}

static gint
properties_part_destroyed(GtkWidget *widget, gpointer data)
{
  if (widget == object_part) {
    object_part = NULL;
    current_obj = NULL;
    current_dia = NULL;
  }
  return 0;
}

static gint
properties_dialog_destroyed(GtkWidget *widget, gpointer data)
{
  dialog = NULL;
  return 0;
}

static gboolean
properties_key_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  /* These ought to be done automagically by GtkDialog, but aren't.
   * Adding them this way makes use of Esc/Enter for other purposes in
   * the dialog close the dialog -- not good.
   */
  /*
  if (event->keyval == GDK_Escape) {
    gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_CANCEL);
    return TRUE;
  }
  if (event->keyval == GDK_Return) {
    gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_OK);
  }
  */
  return FALSE;
}

static gint
properties_respond(GtkWidget *widget, 
                   gint       response_id,
                   gpointer   data)
{
  ObjectChange *obj_change = NULL;

  if (   response_id == GTK_RESPONSE_APPLY 
      || response_id == GTK_RESPONSE_OK) {
    if ((current_obj != NULL) && (current_dia != NULL)) {
      object_add_updates(current_obj, current_dia);
      obj_change = current_obj->ops->apply_properties(current_obj, object_part);
      object_add_updates(current_obj, current_dia);

      diagram_update_connections_object(current_dia, current_obj, TRUE);
    
      if (obj_change != NULL) {
	undo_object_change(current_dia, current_obj, obj_change);
      }
    
      diagram_modified(current_dia);

      diagram_object_modified(current_dia, current_obj);

      diagram_update_extents(current_dia);
      
      if (obj_change != NULL) {
	undo_set_transactionpoint(current_dia->undo);
      }  else {
	message_warning(_("This object doesn't support Undo/Redo.\n"
  			"Undo information erased."));
	undo_clear(current_dia->undo);
      }

      diagram_flush(current_dia);
    }
  }

  if (response_id != GTK_RESPONSE_APPLY)
    gtk_widget_hide(widget);

  return 0;
}

void
properties_show(Diagram *dia, Object *obj)
{
  GtkWidget *properties = NULL;

  if (obj != NULL) 
    properties = obj->ops->get_properties(obj, FALSE);

  if (dialog == NULL)
    create_dialog();

  if (obj==NULL) {
    /* Hide dialog when no object is selected */
    gtk_widget_hide(dialog);
    return;
  }

  if (properties == NULL) { /* No properties or no object */
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

  if (obj != NULL) {
    ObjectType *otype;
    gchar *buf;

    otype = obj->type;
    buf = g_strconcat(_("Properties: "), otype->name, NULL);
    gtk_window_set_title(GTK_WINDOW(dialog), buf);
    g_free(buf);
  } else {
    gtk_window_set_title(GTK_WINDOW(dialog), _("Object properties:"));
  }

  g_signal_connect (G_OBJECT (properties), "destroy",
		  G_CALLBACK(properties_part_destroyed), NULL);
  g_signal_connect (G_OBJECT (dialog), "destroy",
		  G_CALLBACK(properties_dialog_destroyed), NULL);

  gtk_box_pack_start(GTK_BOX(dialog_vbox), properties, TRUE, TRUE, 0);

  gtk_widget_show (properties);

  if (obj != current_obj)
    gtk_window_resize (GTK_WINDOW(dialog), 1, 1); /* resize to minimum */
  gtk_window_present (GTK_WINDOW (dialog));
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
