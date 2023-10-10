/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * find-and-replace.c - common functionality applied to diagram
 *
 * Copyright (C) 2008 Hans Breuer
 * Copyright (C) 2008 Johann Tienhaara (patched)
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
  MATCH_WORD = (1<<1),
  /* Don't just match the name/text - match UML attributes etc too? */
  MATCH_ALL_PROPERTIES = (1<<2)
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


/*! Match and possibly modify the given object's given property.
 * Returns FALSE if not matched or if the input property is NULL. */
static gboolean
_match_text_prop (DiaObject *obj, const SearchData *sd, const gchar *replacement, gchar **value_to_match)
{
  gboolean is_match = FALSE;
  gchar    *repl = NULL;

  if (!value_to_match || *value_to_match == NULL)
    return FALSE;

  /* search part */
  if (sd->flags & MATCH_CASE) {
    const char *p = strstr (*value_to_match, sd->key);
    is_match = p != NULL;
    if (p && replacement) {
      char *a = g_strndup (*value_to_match, p - *value_to_match);
      char *b = g_strdup (p + strlen (sd->key));

      repl = g_strdup_printf ("%s%s%s", a, replacement, b);

      g_clear_pointer (&a, g_free);
      g_clear_pointer (&b, g_free);
    }
  } else {
    gchar *s1 = g_utf8_casefold (*value_to_match, -1);
    gchar *s2 = g_utf8_casefold (sd->key, -1);
    const gchar *p = strstr (s1, s2);
    is_match = p != NULL;
    if (p && replacement) {
      char *a = g_strndup (*value_to_match, p - s1);
      char *b = g_strdup (*value_to_match + strlen(a) + strlen(sd->key));

      repl = g_strdup_printf ("%s%s%s", a, replacement, b);

      g_clear_pointer (&a, g_free);
      g_clear_pointer (&b, g_free);
    }

    g_clear_pointer (&s1, g_free);
    g_clear_pointer (&s2, g_free);
  }

  if (sd->flags & MATCH_WORD)
    is_match = (is_match && strlen(*value_to_match) == strlen(sd->key));

  /* replace part */
  if (is_match && replacement) {
    g_clear_pointer (&*value_to_match, g_free);
    *value_to_match = repl;
  } else {
    g_clear_pointer (&repl, g_free);
  }

  return is_match;
}


/*! Match and possibly modify the given object's name/text property */
static GPtrArray *
_match_name_prop (DiaObject *obj, const SearchData *sd, const gchar *replacement)
{
  Property *prop;
  gchar   **name;
  gboolean is_match = FALSE;
  GPtrArray *plist = NULL;

  if ((prop = object_prop_by_name(obj, "name")) != NULL)
    name = &((StringProperty *)prop)->string_data;
  else if ((prop = object_prop_by_name(obj, "text")) != NULL)
    name = &((TextProperty *)prop)->text_data;
  else
    return NULL;

  is_match = _match_text_prop (obj, sd, replacement, name);

  if (!is_match) {
    prop->ops->free (prop);
    return NULL;
  }

  plist = prop_list_from_single (prop);

  return plist;
}

/*! GHRFunc : match SearchData against value in the hashtable */
static gboolean
_match_value (gpointer  key,
	      gpointer  value,
	      gpointer  user_data)
{
  const SearchData *sd = (SearchData *)user_data;

  if (sd->flags & MATCH_CASE) {
    return strstr ((char *)value, sd->key) != NULL;
  } else {
    gchar *s1 = g_utf8_casefold ((char *)value, -1);
    gchar *s2 = g_utf8_casefold (sd->key, -1);
    gboolean matched = strstr (s1, s2) != NULL;
    g_clear_pointer (&s1, g_free);
    g_clear_pointer (&s2, g_free);
    return matched;
  }
}

/*! Match and possibly modify one property in an object. */
static gboolean
_match_prop (DiaObject *obj, const SearchData *sd, const gchar *replacement, Property *prop)
{
  PropertyType prop_type;
  gboolean is_match = FALSE;
  gchar **text_data;

  if (!prop)
    return FALSE;

  /* TODO: We could probably speed this up by using the type_quark,
   *       but I don't know enough yet to use it safely... */
  prop_type = prop->descr->type;
  if (!prop_type)
    return FALSE;

  /* Special case: array of sub-properties.  Do not continue with
   * checking text for this property.  Instead, just
   * recurse into _match_prop() for each sub-property in
   * the array. */
  if (   strcmp (prop_type, PROP_TYPE_SARRAY) == 0
      || strcmp (prop_type, PROP_TYPE_DARRAY) == 0) {
    GPtrArray *records = ((ArrayProperty *) prop)->records;
    guint rnum;

    if (!records)
      return FALSE;

    for (rnum = 0; rnum < records->len && !is_match; ++rnum) {
      GPtrArray *sub_props = g_ptr_array_index (records, rnum);
      guint sub_num;

      for (sub_num = 0; sub_num < sub_props->len && !is_match; ++sub_num) {
	Property *sub_prop = g_ptr_array_index (sub_props, sub_num);

        is_match = _match_prop (obj, sd, replacement, sub_prop);
      }
    }
    /* Done. */
    return is_match;
  }
  else if (strcmp (prop_type, PROP_TYPE_DICT) == 0)
  {
    GHashTable *ht = ((DictProperty*)prop)->dict;
    if (ht && g_hash_table_find (ht, _match_value, (gpointer)sd))
      return TRUE;
  }


  /* Check for string / text property. */
  if (   strcmp (prop_type, PROP_TYPE_MULTISTRING) == 0
      || strcmp (prop_type, PROP_TYPE_STRING) == 0)
  {
    text_data = &((StringProperty *) prop)->string_data;
  } else if (strcmp (prop_type, PROP_TYPE_TEXT) == 0) {
    text_data = &((TextProperty *) prop)->text_data;
  }
  /* TODO future:
  else if ( strcmp (prop_type, PROP_TYPE_STRINGLIST) == 0)
  {
  }
  */
  else {
    /* Not a type we're interested in (int, real, geometry, etc). */
    return FALSE;
  }

  return _match_text_prop (obj, sd, replacement, text_data);
}

/*! Match and possibly modify all the given object's properties. */
static GPtrArray *
_match_all_props (DiaObject *obj, const SearchData *sd, const gchar *replacement)
{
  GPtrArray *all_plist = NULL;
  GPtrArray *matched_plist = NULL;
  const PropDescription *desc;
  guint pnum;

  if (!obj)
    return NULL;

  desc = object_get_prop_descriptions (obj);
  if (!desc)
    return NULL;

  all_plist = prop_list_from_descs (desc, pdtpp_true);
  if (!all_plist)
    return NULL;

  /* Step though all object properties.
   * Along the way, construct a list of matching properties (or
   * replaced properties). */
  for (pnum = 0; pnum < all_plist->len; ++pnum) {
    Property *prop = g_ptr_array_index (all_plist, pnum);
    gboolean is_match = FALSE;
    const gchar *prop_name;

    if (!prop || !prop->descr->name)
      continue;

    /* This extra step seems to be necessary to populate the property data. */
    prop_name = prop->descr->name;
    prop->ops->free (prop);
    prop = object_prop_by_name (obj, prop_name);

    is_match = _match_prop (obj, sd, replacement, prop);

    if (!is_match) {
      prop->ops->free (prop);
      continue;
    }

    /* We have a match. */
    if (!matched_plist) {
      /* First time. */
      matched_plist = prop_list_from_single (prop);
    } else {
      /* FIXME: do we really want a replace all here? */
      /* Subsequent finds. */
      GPtrArray *append_plist;
      append_plist = prop_list_from_single (prop);
      prop_list_add_list (matched_plist, append_plist);
      prop_list_free (append_plist);
    }

  } /* Continue stepping through all object properties. */

  return matched_plist;
}


/*! Match and possibly modify one or more properties in an object.
 *  Returns a list of modified Properties. */
static GPtrArray *
_match_props (DiaObject *obj, const SearchData *sd, const gchar *replacement)
{
  g_return_val_if_fail (obj && sd, NULL);

  if (sd->flags & MATCH_ALL_PROPERTIES)
    return _match_all_props (obj, sd, replacement);
  else
    return _match_name_prop (obj, sd, replacement);
}


/* Only match (find), do not replace any values. */
static gboolean
_matches (DiaObject *obj, const SearchData *sd)
{
  GPtrArray *plist = NULL;

  if (!obj)
    return FALSE;

  plist = _match_props (obj, sd, NULL);
  if (!plist)
    return FALSE;

  prop_list_free (plist);

  return TRUE;
}

static void
find_func (gpointer data, gpointer user_data)
{
  DiaObject *obj = data;
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


/* Match and replace property values. */
static gboolean
_replace (DiaObject *obj, const SearchData *sd, const char *replacement)
{
  DiaObjectChange *obj_change;
  GPtrArray *plist = NULL;

  plist = _match_props (obj, sd, replacement);
  if (!plist)
    return FALSE;

  /* Refresh screen and free the list of modified properties. */
  obj_change = object_apply_props (obj, plist);
  prop_list_free (plist);

  if (obj_change)
    dia_object_change_change_new (sd->diagram, obj, obj_change);

  object_add_updates(obj, sd->diagram);
  diagram_update_connections_object(sd->diagram, obj, TRUE);
  diagram_modified(sd->diagram);
  diagram_object_modified(sd->diagram, obj);
  diagram_update_extents(sd->diagram);
  diagram_flush(sd->diagram);

  return TRUE;
}


static int
fnr_respond (GtkWidget *widget, int response_id, gpointer data)
{
  const gchar *search = gtk_entry_get_text (g_object_get_data (G_OBJECT (widget), "search-entry"));
  const gchar *replace;
  DDisplay *ddisp = (DDisplay*)data;
  SearchData sd = { 0, };
  sd.diagram = ddisp->diagram;
  sd.flags =  gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON (
                  g_object_get_data (G_OBJECT (widget), "match-case"))) ? MATCH_CASE : 0;
  sd.flags |= gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON (
                  g_object_get_data (G_OBJECT (widget), "match-word"))) ? MATCH_WORD : 0;
  sd.flags |= gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON (
		  g_object_get_data (G_OBJECT (widget), "match-all-properties"))) ? MATCH_ALL_PROPERTIES : 0;


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
      if (dia_object_get_parent_layer(sd.last) != dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (ddisp->diagram))) {
        /* can only select objects in the active layer */
        data_set_active_layer(ddisp->diagram->data, dia_object_get_parent_layer(sd.last));
        diagram_add_update_all(ddisp->diagram);
        diagram_flush(ddisp->diagram);
      }
      diagram_select (ddisp->diagram, sd.last);
      ddisplay_present_object (ddisp, sd.last);
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
  GtkWidget *match_all_properties;

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), RESPONSE_FIND);

  /* don't destroy dialog when window manager close button pressed */
  g_signal_connect(G_OBJECT (dialog), "response",
		   G_CALLBACK(fnr_respond), ddisp);
  g_signal_connect(G_OBJECT(dialog), "delete_event",
		   G_CALLBACK(gtk_widget_hide), NULL);
  g_signal_connect(G_OBJECT(dialog), "delete_event",
		   G_CALLBACK(gtk_true), NULL);

  vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
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

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
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

  match_all_properties = gtk_check_button_new_with_mnemonic (_("Match _all properties (not just object name)"));
  gtk_box_pack_start (GTK_BOX (vbox), match_all_properties, FALSE, FALSE, 6);
  g_object_set_data (G_OBJECT (dialog), "match-all-properties", match_all_properties);
  if (is_replace)
    gtk_widget_set_sensitive (GTK_WIDGET (match_all_properties), FALSE);


  gtk_widget_show_all (vbox);
}


/**
 * edit_find_callback:
 * @action: the #GtkAction
 *
 * React to `<Display>/Edit/Find`
 *
 * Since: dawn-of-time
 */
void
edit_find_callback (GtkAction *action)
{
  DDisplay *ddisp;
  GtkWidget *dialog;

  ddisp = ddisplay_active();
  if (!ddisp) return;

  /* no static var, instead we are attaching the dialog to the display shell */
  dialog = g_object_get_data (G_OBJECT (ddisp->shell), "edit-find-dialog");
  if (!dialog) {
    dialog = gtk_dialog_new_with_buttons (_("Find"),
                                          GTK_WINDOW (ddisp->shell),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          _("_Close"), GTK_RESPONSE_CLOSE,
                                          _("_Find"), RESPONSE_FIND,
                                          NULL);

    fnr_dialog_setup_common (dialog, FALSE, ddisp);
  }
  g_object_set_data (G_OBJECT (ddisp->shell), "edit-find-dialog", dialog);

  gtk_dialog_run (GTK_DIALOG (dialog));
}


/**
 * edit_replace_callback:
 * @action: the #GtkAction
 *
 * React to `<Display>/Edit/Replace`
 *
 * Since: dawn-of-time
 */
void
edit_replace_callback(GtkAction *action)
{
  DDisplay *ddisp;
  GtkWidget *dialog;

  ddisp = ddisplay_active();
  if (!ddisp) return;

  /* no static var, instead we are attaching the dialog to the display shell */
  dialog = g_object_get_data (G_OBJECT (ddisp->shell), "edit-replace-dialog");
  if (!dialog) {
    dialog = gtk_dialog_new_with_buttons (_("Replace"),
                                          GTK_WINDOW (ddisp->shell),
                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                          _("_Close"), GTK_RESPONSE_CLOSE,
                                          _("Replace _All"), RESPONSE_REPLACE_ALL,
                                          _("_Replace"), RESPONSE_REPLACE,
                                          NULL);

    gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Find"), RESPONSE_FIND);

    fnr_dialog_setup_common (dialog, TRUE, ddisp);
  }
  g_object_set_data (G_OBJECT (ddisp->shell), "edit-replace-dialog", dialog);

  gtk_dialog_run (GTK_DIALOG (dialog));
}
