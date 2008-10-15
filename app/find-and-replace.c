/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * find-and-replace.c - common functionality applied to diagram
 *
 * Copyright (C) 2008 Hans Breuer
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

#include <config.h>

#include <gtk/gtk.h>

#include "intl.h"

#include "diagram.h"
#include "display.h"
#include "object.h"
#include "object_ops.h"
#include "connectionpoint_ops.h"
#include "undo.h"

#include "find-and-replace.h"
/* messing with property internals */
#include "propinternals.h"

enum {
  RESPONSE_FIND = -20,
  RESPONSE_REPLACE = -21,
  RESPONSE_REPLACE_ALL = -23
};

enum {
  MATCH_CASE = (1<<0),
  MATCH_WORD = (1<<1)
};

typedef struct _SearchData {
  const gchar *key;
  guint      flags;
  Diagram *diagram;
  DiaObject *first; /* the first one found */
  DiaObject *found; /* the one we were looking for */
  DiaObject  *last; /* previously found */
  gboolean seen_last;
} SearchData;

/*! Match and possibly modify the given objects property */
Property *
_match_string_prop (DiaObject *obj, const SearchData *sd, const gchar *replacement)
{
  Property *prop;
  gchar   **name;
  gboolean ret = FALSE;
  gchar    *repl = NULL;

  if ((prop = object_prop_by_name(obj, "name")) != NULL)
    name = &((StringProperty *)prop)->string_data;
  else if ((prop = object_prop_by_name(obj, "text")) != NULL)
    name = &((TextProperty *)prop)->text_data;

  if (!prop)
    return NULL;

  /* search part */
  if (sd->flags & MATCH_CASE) {
    const gchar *p = strstr (*name, sd->key);
    ret = p != NULL;
    if (p && replacement) {
      gchar *a = g_strndup (*name, p - *name);
      gchar *b = g_strdup (p + strlen(sd->key));
      repl = g_strdup_printf ("%s%s%s", a, replacement, b);
      g_free (a);
      g_free (b);
    }
  } else {
    gchar *s1 = g_utf8_casefold (*name, -1);
    gchar *s2 = g_utf8_casefold (sd->key, -1);
    const gchar *p = strstr (s1, s2);
    ret = p != NULL;
    if (p && replacement) {
      gchar *a = g_strndup (*name, p - s1);
      gchar *b = g_strdup (*name + strlen(a) + strlen(sd->key));
      repl = g_strdup_printf ("%s%s%s", a, replacement, b);
      g_free (a);
      g_free (b);
    }
    g_free (s1);
    g_free (s2);
  }

  if (sd->flags & MATCH_WORD)
    ret = (ret && strlen(*name) == strlen(sd->key));

  /* replace part */
  if (ret && replacement) {
    g_free (*name);
    *name = repl;
  } else {
    g_free (repl);
  }
  
  if (ret)
    return prop;
    
  prop->ops->free(prop);
  return NULL;
}

static gboolean
_matches (DiaObject *obj, const SearchData *sd)
{
  Property *prop = NULL;

  if (!obj)
    return FALSE;

  prop = _match_string_prop (obj, sd, NULL);
  if (prop)
    prop->ops->free(prop);

  return (prop != NULL);    
}

static void
find_func (DiaObject *obj, gpointer user_data)
{
  SearchData *sd = (SearchData *)user_data;
  
  if (!sd->found) {
    if (_matches (obj, sd)) {
      if (!sd->first)
        sd->first = obj;
      if (obj == sd->last)
        sd->seen_last = TRUE;
      else if (sd->seen_last) {
        sd->found = obj;
      }
    }
  }
}

static gboolean
_replace (DiaObject *obj, const SearchData *sd, const char *replacement)
{
  ObjectChange *obj_change;
  Property *prop;
  GPtrArray *plist;

  prop = _match_string_prop (obj, sd, replacement);
  if (!prop)
    return FALSE;
    
  plist = prop_list_from_single (prop);
  obj_change = object_apply_props (obj, plist);
  prop_list_free (plist);

  if (obj_change)
    undo_object_change(sd->diagram, obj, obj_change);
    
  object_add_updates(obj, sd->diagram);
  diagram_update_connections_object(sd->diagram, obj, TRUE);
  diagram_modified(sd->diagram);
  diagram_object_modified(sd->diagram, obj);
  diagram_update_extents(sd->diagram);
  diagram_flush(sd->diagram);
  
  return TRUE;
}

static gint
fnr_respond (GtkWidget *widget, gint response_id, gpointer data)
{
  const gchar *search = gtk_entry_get_text (g_object_get_data (G_OBJECT (widget), "search-entry")); 
  const gchar *replace;
  DDisplay *ddisp = (DDisplay*)data;
  GList *list;
  SearchData sd = { 0, };
  sd.diagram = ddisp->diagram;
  sd.flags =  gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( 
                  g_object_get_data (G_OBJECT (widget), "match-case"))) ? MATCH_CASE : 0;
  sd.flags |= gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( 
                  g_object_get_data (G_OBJECT (widget), "match-word"))) ? MATCH_WORD : 0;
  

  switch (response_id) {
  case RESPONSE_FIND :
    sd.key = search;
    sd.last = g_object_get_data (G_OBJECT (widget), "last-found");
    if (!_matches (sd.last, &sd))
      sd.last = NULL; /* reset if we start a new search */
    diagram_remove_all_selected (ddisp->diagram, TRUE);
    data_foreach_object (ddisp->diagram->data, find_func, &sd);
    /* remember it */
    sd.last = sd.found ? sd.found : sd.first;
    g_object_set_data (G_OBJECT (widget), "last-found", sd.last);
    if (sd.last) {
      if (dia_object_get_parent_layer(sd.last) != ddisp->diagram->data->active_layer) {
        /* can only select objects in the active layer */
        data_set_active_layer(ddisp->diagram->data, dia_object_get_parent_layer(sd.last));
        diagram_add_update_all(ddisp->diagram);
        diagram_flush(ddisp->diagram);
      }
      diagram_select (ddisp->diagram, sd.last);
    }
    break;
  case RESPONSE_REPLACE :
    replace = gtk_entry_get_text (g_object_get_data (G_OBJECT (widget), "replace-entry"));
    sd.key = search;
    sd.last = g_object_get_data (G_OBJECT (widget), "last-found");
    if (!_matches (sd.last, &sd)) {
      sd.last = NULL; /* reset if we start a new search */
      data_foreach_object (ddisp->diagram->data, find_func, &sd);
    }
    sd.last = sd.found ? sd.found : sd.first;
    g_object_set_data (G_OBJECT (widget), "last-found", sd.last);
    if (sd.last) {
      _replace (sd.last, &sd, replace);
      undo_set_transactionpoint(ddisp->diagram->undo);
    }
    g_object_set_data (G_OBJECT (widget), "last-found", sd.last);
    break;
  case RESPONSE_REPLACE_ALL :
    replace = gtk_entry_get_text (g_object_get_data (G_OBJECT (widget), "replace-entry"));
    sd.key = search;
    sd.last = g_object_get_data (G_OBJECT (widget), "last-found");
    do {
      if (!_matches (sd.last, &sd)) {
        sd.last = NULL; /* reset if we start a new search */
	sd.first = NULL;
        data_foreach_object (ddisp->diagram->data, find_func, &sd);
      }
      sd.last = sd.found ? sd.found : sd.first;
      if (sd.last)
        if (!_replace (sd.last, &sd, replace))
	  sd.last = NULL;
    } while (sd.last);
    g_object_set_data (G_OBJECT (widget), "last-found", sd.last);
    undo_set_transactionpoint(ddisp->diagram->undo);
    break;
  default:
    gtk_widget_hide (widget);
  }
  return 0;
}

static void
fnr_dialog_setup_common (GtkWidget *dialog, gboolean is_replace, DDisplay *ddisp)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *search_entry;
  GtkWidget *match_case;
  GtkWidget *match_word;

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), RESPONSE_FIND);

  /* don't destroy dialog when window manager close button pressed */
  g_signal_connect(G_OBJECT (dialog), "response",
		   G_CALLBACK(fnr_respond), ddisp);
  g_signal_connect(G_OBJECT(dialog), "delete_event",
		   G_CALLBACK(gtk_widget_hide), NULL);
  g_signal_connect(GTK_OBJECT(dialog), "delete_event",
		   G_CALLBACK(gtk_true), NULL);

  vbox = GTK_DIALOG(dialog)->vbox;

  hbox = gtk_hbox_new (FALSE, 12);
  label = gtk_label_new_with_mnemonic (_("_Search for:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  search_entry = gtk_entry_new ();
  g_object_set_data (G_OBJECT (dialog), "search-entry", search_entry);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), search_entry);
  gtk_entry_set_width_chars (GTK_ENTRY (search_entry), 30);
  gtk_box_pack_start (GTK_BOX (hbox), search_entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 6);
  
  if (is_replace) {
    GtkWidget *replace_entry;

    hbox = gtk_hbox_new (FALSE, 12);
    label = gtk_label_new_with_mnemonic (_("Replace _with:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    replace_entry = gtk_entry_new ();
    g_object_set_data (G_OBJECT (dialog), "replace-entry", replace_entry);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), replace_entry);
    gtk_entry_set_width_chars (GTK_ENTRY (replace_entry), 30);
    gtk_box_pack_start (GTK_BOX (hbox), replace_entry, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 6);
  }

  match_case = gtk_check_button_new_with_mnemonic (_("_Match case"));
  gtk_box_pack_start (GTK_BOX (vbox), match_case, FALSE, FALSE, 6);
  g_object_set_data (G_OBJECT (dialog), "match-case", match_case);

  match_word = gtk_check_button_new_with_mnemonic (_("Match _entire word only"));
  gtk_box_pack_start (GTK_BOX (vbox), match_word, FALSE, FALSE, 6);
  g_object_set_data (G_OBJECT (dialog), "match-word", match_word);

  gtk_widget_show_all (vbox);
}

/**
 * React to <Display>/Edit/Find
 */
void
edit_find_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  Diagram *dia;
  GtkWidget *dialog;

  ddisp = ddisplay_active();
  if (!ddisp) return;
  dia = ddisp->diagram;

  /* no static var, instead we are attaching the dialog to the diplay shell */
  dialog = g_object_get_data (G_OBJECT (ddisp->shell), "edit-find-dialog");
  if (!dialog) {
    dialog = gtk_dialog_new_with_buttons (
		_("Find"), 
		GTK_WINDOW (ddisp->shell), GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		GTK_STOCK_FIND, RESPONSE_FIND,
		NULL);

    fnr_dialog_setup_common (dialog, FALSE, ddisp);
  }
  g_object_set_data (G_OBJECT (ddisp->shell), "edit-find-dialog", dialog);

  gtk_dialog_run (GTK_DIALOG (dialog));  
}

/**
 * React to <Display>/Edit/Replace
 */
void
edit_replace_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  Diagram *dia;
  GtkWidget *dialog;

  ddisp = ddisplay_active();
  if (!ddisp) return;
  dia = ddisp->diagram;

  /* no static var, instead we are attaching the dialog to the diplay shell */
  dialog = g_object_get_data (G_OBJECT (ddisp->shell), "edit-replace-dialog");
  if (!dialog) {
    GtkWidget *button;
    dialog = gtk_dialog_new_with_buttons (
		_("Replace"),
		GTK_WINDOW (ddisp->shell), GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		_("Replace _All"), RESPONSE_REPLACE_ALL,
		NULL);
    /* not adding the button in the list above to modify it's text; 
     * the default "Find and Replace" is just too long for my taste ;) 
     */
    button = gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Replace"), RESPONSE_REPLACE);
    gtk_button_set_image (GTK_BUTTON (button), 
                          gtk_image_new_from_stock (GTK_STOCK_FIND_AND_REPLACE, GTK_ICON_SIZE_BUTTON));

    gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_FIND, RESPONSE_FIND);

    fnr_dialog_setup_common (dialog, TRUE, ddisp);
  }
  g_object_set_data (G_OBJECT (ddisp->shell), "edit-replace-dialog", dialog);

  gtk_dialog_run (GTK_DIALOG (dialog));  
}

