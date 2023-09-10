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

#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "properties-dialog.h"
#include "object_ops.h"
#include "object.h"
#include "connectionpoint_ops.h"
#include "undo.h"
#include "message.h"
#include "properties.h"

static GtkWidget *dialog = NULL;
static GtkWidget *dialog_vbox = NULL;
static GtkWidget *object_part = NULL;
static GList *current_objects = NULL;
static Diagram *current_dia = NULL;

static GtkWidget *no_properties_dialog = NULL;

static int      properties_respond   (GtkWidget   *widget,
                                      int          response_id,
                                      gpointer     data);
static gboolean properties_key_event (GtkWidget   *widget,
                                      GdkEventKey *event,
                                      gpointer     data);
static void properties_dialog_hide(void);

static void
create_dialog(GtkWidget *parent)
{
  GtkWidget *action_area;
/*   GtkWidget *actionbox; */
/*   GList *buttons; */

  dialog = gtk_dialog_new_with_buttons (_("Object properties"),
                                        parent ? GTK_WINDOW (parent) : NULL,
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        _("_Close"), GTK_RESPONSE_CLOSE,
                                        _("_Apply"), GTK_RESPONSE_APPLY,
                                        _("_OK"), GTK_RESPONSE_OK,
                                        NULL);

  gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_OK);

  action_area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));
  gtk_widget_set_margin_bottom (action_area, 6);
  gtk_widget_set_margin_top (action_area, 6);
  gtk_widget_set_margin_start (action_area, 6);
  gtk_widget_set_margin_end (action_area, 6);

  dialog_vbox = gtk_dialog_get_content_area (GTK_DIALOG(dialog));

  gtk_window_set_role(GTK_WINDOW (dialog), "properties_window");

  g_signal_connect(G_OBJECT (dialog), "response",
                   G_CALLBACK (properties_respond), NULL);
  g_signal_connect(G_OBJECT (dialog), "delete_event",
		   G_CALLBACK(properties_dialog_hide), NULL);
  g_signal_connect(G_OBJECT (dialog), "destroy",
		   G_CALLBACK(gtk_widget_destroyed), &dialog);
  g_signal_connect(G_OBJECT (dialog), "destroy",
		   G_CALLBACK(gtk_widget_destroyed), &dialog_vbox);
  g_signal_connect(G_OBJECT (dialog), "key-release-event",
		   G_CALLBACK(properties_key_event), NULL);

  no_properties_dialog = gtk_label_new(_("This object has no properties."));
  gtk_widget_show (no_properties_dialog);
  g_object_ref_sink (G_OBJECT (no_properties_dialog));
}

/* when the dialog gets destroyed we need to drop our references */
static int
properties_part_destroyed(GtkWidget *widget, gpointer data)
{
  if (widget == object_part) {
    object_part = NULL;
    current_objects = NULL;
    current_dia = NULL;
  }
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


static int
properties_respond (GtkWidget *widget,
                    int        response_id,
                    gpointer   data)
{
  DiaObjectChange *obj_change = NULL;
  gboolean set_tp = TRUE;
  GList *tmp;

  if (   response_id == GTK_RESPONSE_APPLY
      || response_id == GTK_RESPONSE_OK) {
    if ((current_objects != NULL) && (current_dia != NULL)) {
      object_add_updates_list(current_objects, current_dia);

      for (tmp = current_objects; tmp != NULL; tmp = tmp->next) {
        DiaObject *current_obj = (DiaObject*)tmp->data;
        obj_change = dia_object_apply_editor (current_obj, object_part);
        object_add_updates(current_obj, current_dia);
        diagram_update_connections_object(current_dia, current_obj, TRUE);

        if (obj_change != NULL) {
          dia_object_change_change_new (current_dia, current_obj, obj_change);
          set_tp = set_tp && TRUE;
        } else {
          set_tp = FALSE;
        }

        diagram_object_modified(current_dia, current_obj);
      }

      diagram_modified(current_dia);
      diagram_update_extents(current_dia);

      if (set_tp) {
	undo_set_transactionpoint(current_dia->undo);
      }  else {
	message_warning(_("This object doesn't support Undo/Redo.\n"
  			"Undo information erased."));
	undo_clear(current_dia->undo);
      }

      diagram_flush(current_dia);
    }
  }

  if (response_id != GTK_RESPONSE_APPLY) {
#ifdef G_OS_WIN32
    /* on windows we are not hiding the dialog, because shrinking when hidden does
     * not work (the dialog shows up with the same size as before, bug #333751) */
    gtk_widget_destroy (dialog);
#else
    properties_dialog_hide();
#endif
  }

  return 0;
}


/**
 * properties_give_focus:
 * @widget: Some (possibly composite) widget.
 * @data: user data
 *
 * Give focus to the first focusable widget found in `widget'.
 *
 * Since: dawn-of-time
 */
static void
properties_give_focus (GtkWidget *widget, gpointer data)
{
  if (gtk_widget_get_can_focus(widget)) {
    gtk_widget_grab_focus(widget);
  } else {
    if (GTK_IS_CONTAINER(widget)) {
      gtk_container_foreach(GTK_CONTAINER(widget), properties_give_focus, data);
    }
  }
}


static void
clear_dialog_globals (void)
{
  if (object_part != NULL) {
    gtk_container_remove (GTK_CONTAINER (dialog_vbox), object_part);
    object_part = NULL;
  }
  g_list_free (current_objects);
  current_objects = NULL;
  current_dia = NULL;
}


void
object_properties_show(Diagram *dia, DiaObject *obj)
{
  GList *tmp = NULL;
  tmp = g_list_append(tmp, obj);
  object_list_properties_show(dia, tmp);
  g_list_free(tmp);
}

void
object_list_properties_show(Diagram *dia, GList *objects)
{
  GtkWidget *properties;
  DiaObject *one_obj;
  GtkWidget *parent = ddisplay_active() ? ddisplay_active()->shell : NULL;
  if (!dialog)
      create_dialog(parent);
  clear_dialog_globals();

  if (!objects) {
    /* Hide dialog when no object is selected */
    properties_dialog_hide();
    return;
  }

  /* Prefer object-specific UI when only one object is selected. */
  one_obj = (g_list_length(objects) == 1) ? objects->data : NULL;
  if (one_obj)
    properties = one_obj->ops->get_properties(one_obj, FALSE);
  else
    properties = object_list_create_props_dialog(objects, FALSE);
  if (properties == NULL) {
    properties = no_properties_dialog;
  }

  if (one_obj) {
    DiaObjectType *otype = one_obj->type;
    char *buf;

    buf = g_strconcat (_("Properties: "), _(otype->name), NULL);
    gtk_window_set_title (GTK_WINDOW (dialog), buf);
    g_clear_pointer (&buf, g_free);
  } else {
    gtk_window_set_title (GTK_WINDOW (dialog), _("Object properties:"));
  }

  g_signal_connect (G_OBJECT (properties), "destroy",
		  G_CALLBACK(properties_part_destroyed), NULL);
  gtk_box_pack_start(GTK_BOX(dialog_vbox), properties, TRUE, TRUE, 0);

  gtk_widget_show (properties);

  properties_give_focus(properties, NULL);

  /* resize to minimum */
  /* if (obj != current_obj) */
  gtk_window_resize (GTK_WINDOW(dialog), 1, 1);
  if (parent)
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW (parent));
  gtk_window_present (GTK_WINDOW (dialog));
  object_part = properties;
  current_objects = g_list_copy(objects);
  current_dia = dia;
}

void
properties_dialog_hide(void)
{
  if (!dialog)
    return;
  clear_dialog_globals();
  gtk_widget_hide(dialog);
}

void
properties_hide_if_shown(Diagram *dia, DiaObject *obj)
{
  if (g_list_find(current_objects, obj)) {
    if (g_list_length(current_objects) == 1)
      /* If obj is the only object shown, then we hide the dialog. */
      properties_dialog_hide();
    else {
      /* else we remove this object and recreate the dialog. */
      GList *tmp = g_list_copy(current_objects);
      tmp = g_list_remove(tmp, obj);
      object_list_properties_show(dia, tmp);
      g_list_free(tmp);
    }
  }
}

void
properties_update_if_shown(Diagram *dia, DiaObject *obj)
{
  /* if there are multiple objects currently shown, there is no reason
     why the properties of this object should be updated in the
     dialog. */
  if (g_list_length(current_objects) != 1)
    return;
  else {
    /* FIXME: instead of recreating the dialog, there should be a way
       to refresh the existing PropDialog widget. */
    object_properties_show(dia, (DiaObject*)current_objects->data);
  }
}
