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
 *
 * File:    table_dialog.c
 *
 * Purpose: This file contains the code the draws and handles the
 *          (databse) table dialog. This is the dialog box that is
 *          displayed when the table Icon is double clicked.
 */

/*
 * The code listed here is very similar to the one for the UML class
 * dialog. Indeed, I've used it as a template. -- pn
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#undef GTK_DISABLE_DEPRECATED /* GtkList, */
#include <gtk/gtk.h>
#include <string.h>
#include "database.h"

struct _TablePropDialog {
  GtkWidget * dialog;

  /* general page */

  GtkEntry * table_name;
  GtkTextView * table_comment;
  GtkToggleButton * comment_visible;
  GtkToggleButton * comment_tagging;
  GtkToggleButton * underline_primary_key;
  GtkToggleButton * bold_primary_key;

  DiaColorSelector * text_color;
  DiaColorSelector * line_color;
  DiaColorSelector * fill_color;

  DiaFontSelector *normal_font;
  GtkSpinButton *normal_font_height;

  DiaFontSelector *name_font;
  GtkSpinButton *name_font_height;

  DiaFontSelector *comment_font;
  GtkSpinButton *comment_font_height;

  GtkSpinButton * border_width;

  /* attributes page */

  GtkList *     attributes_list;
  GtkEntry *    attribute_name;
  GtkEntry *    attribute_type;
  GtkTextView * attribute_comment;
  GtkToggleButton * attribute_primary_key;
  GtkToggleButton * attribute_nullable;
  GtkToggleButton * attribute_unique;

  GtkListItem * cur_attr_list_item;
  GList * added_connections;
  GList * deleted_connections;
  GList * disconnected_connections;
};

void 
table_dialog_free (TablePropDialog *dialog)
{
  gtk_widget_destroy (dialog->dialog);
  g_free (dialog);
}

/* ----------------------------------------------------------------------- */

static void destroy_prop_dialog (GtkWidget*, gpointer);
static void create_dialog_pages(GtkNotebook *, Table *);
static void create_general_page (GtkNotebook *, Table *);
static void create_attribute_page (GtkNotebook *, Table *);
static void create_style_page (GtkNotebook *, Table *);
static void fill_in_dialog (Table *);
static void general_page_fill_in_dialog (Table *);
static void general_page_props_to_object (Table *, TablePropDialog *);
static void attribute_page_props_to_object (Table *, TablePropDialog *);
static void attributes_page_values_to_attribute (TablePropDialog *,
                                                 TableAttribute *);
static void create_font_props_row (GtkTable   *table,
                                   const char *kind,
                                   gint        row,
                                   DiaFont    *font,
                                   real        height,
                                   DiaFontSelector **fontsel,
                                   GtkSpinButton   **heightsel);
static const gchar * get_comment(GtkTextView *view);
static void set_comment(GtkTextView *view, gchar *text);
static void attributes_list_selection_changed_cb (GtkWidget *, Table *);
static void attributes_page_set_sensitive (TablePropDialog *, gboolean);
static void attributes_page_clear_values(TablePropDialog *);
static void attributes_page_set_values (TablePropDialog *, TableAttribute *);
static void attributes_page_update_cur_attr_item (TablePropDialog *);
static void attribute_list_item_destroy_cb (GtkWidget *, gpointer);
static void attributes_list_new_button_clicked_cb (GtkWidget *, Table *);
static void attributes_list_delete_button_clicked_cb (GtkWidget *, Table *);
static void attributes_list_moveup_button_clicked_cb (GtkWidget *, Table *);
static void attributes_list_movedown_button_clicked_cb (GtkWidget *, Table *);
static void attributes_list_add_attribute (Table *, TableAttribute *, gboolean);
static void current_attribute_update (GtkWidget *, Table *);
static int current_attribute_update_event (GtkWidget *,
                                           GdkEventFocus *, Table *);
static void attributes_page_fill_in_dialog (Table *);
static void attribute_primary_key_toggled_cb (GtkToggleButton *, Table *);
static void attribute_nullable_toggled_cb (GtkToggleButton *, Table *);
static void attribute_unique_toggled_cb (GtkToggleButton *, Table *);

static void table_dialog_store_disconnects (TablePropDialog *,
                                            ConnectionPoint *);
static gchar * table_get_attribute_string (TableAttribute *);

static void table_state_set (TableState *, Table *);
static void table_state_free (TableState *);
static void table_change_free (TableChange *);
static void table_change_revert (TableChange *, DiaObject *);
static void table_change_apply (TableChange *, DiaObject *);


/* ----------------------------------------------------------------------- */

GtkWidget *
table_get_properties_dialog (Table * table, gboolean is_default)
{
  TablePropDialog * prop_dialog;
  GtkWidget * vbox;
  GtkWidget * notebook;

  if (table->prop_dialog == NULL) {
    prop_dialog = g_new0 (TablePropDialog, 1);
    table->prop_dialog = prop_dialog;

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_object_ref (GTK_OBJECT(vbox));
    gtk_object_sink (GTK_OBJECT(vbox));
    prop_dialog->dialog = vbox;

    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (notebook), 10);

    gtk_object_set_user_data (GTK_OBJECT (notebook), (gpointer) table);

    gtk_signal_connect (GTK_OBJECT (notebook),
                        "destroy",
                        GTK_SIGNAL_FUNC (destroy_prop_dialog),
                        (gpointer) table);

    create_dialog_pages (GTK_NOTEBOOK (notebook), table);
    /* by default the table name entry is focused */
    gtk_widget_grab_focus (GTK_WIDGET (table->prop_dialog->table_name));
    gtk_widget_show (notebook);
  }

  fill_in_dialog (table);
  gtk_widget_show (table->prop_dialog->dialog);

  return table->prop_dialog->dialog;
}

ObjectChange *
table_dialog_apply_changes (Table * table, GtkWidget * unused)
{
  TableState * state;
  GList * disconnected;
  GList * deleted;
  GList * added;
  TablePropDialog * prop_dialog;

  prop_dialog = table->prop_dialog;

  state = table_state_new (table);

  general_page_props_to_object (table, prop_dialog);
  attribute_page_props_to_object (table, prop_dialog);

  table_update_primary_key_font (table);
  table_compute_width_height (table);
  table_update_positions (table);

  disconnected = prop_dialog->disconnected_connections;
  prop_dialog->disconnected_connections = NULL;

  deleted = prop_dialog->deleted_connections;
  prop_dialog->deleted_connections = NULL;

  added = prop_dialog->added_connections;
  prop_dialog->added_connections = NULL;

  fill_in_dialog (table);
  return (ObjectChange *)table_change_new (table, state,
                                           added, deleted, disconnected);
}

/* TableState & TableChange functions ------------------------------------- */

TableState * table_state_new (Table * table)
{
  TableState * state = g_new0 (TableState, 1);
  GList * list;

  state->name = g_strdup (table->name);
  state->comment = g_strdup (table->comment);
  state->visible_comment = table->visible_comment;
  state->tagging_comment = table->tagging_comment;
  state->underline_primary_key = table->underline_primary_key;
  state->bold_primary_key = table->bold_primary_key;
  state->border_width = table->border_width;

  list = table->attributes;
  while (list != NULL)
    {
      TableAttribute * attr = (TableAttribute *) list->data;
      TableAttribute * copy = table_attribute_copy (attr);

      copy->left_connection = attr->left_connection;
      copy->right_connection = attr->right_connection;

      state->attributes = g_list_append (state->attributes, copy);
      list = g_list_next (list);
    }

  return state;
}

/**
 * Set the values stored in state to the passed table and reinit the table
 * object. The table-state object will be free.
 */
static void
table_state_set (TableState * state, Table * table)
{
  table->name = state->name;
  table->comment = state->comment;
  table->visible_comment = state->visible_comment;
  table->tagging_comment = state->tagging_comment;
  table->border_width = state->border_width;
  table->underline_primary_key = state->underline_primary_key;
  table->bold_primary_key = state->bold_primary_key;
  table->border_width = state->border_width;
  table->attributes = state->attributes;

  g_free (state);

  table_update_connectionpoints (table);
  table_update_primary_key_font (table);
  table_compute_width_height (table);
  table_update_positions (table);

  gtk_list_clear_items (table->prop_dialog->attributes_list, 0, -1);
}

static void
table_state_free (TableState * state)
{
  GList * list;

  g_free (state->name);
  g_free (state->comment);

  list = state->attributes;
  while (list != NULL)
    {
      TableAttribute * attr = (TableAttribute *) list->data;
      table_attribute_free (attr);
      list = g_list_next (list);
    }
  g_list_free (state->attributes);

  g_free (state);
}

TableChange * table_change_new (Table * table, TableState * saved_state,
                                GList * added, GList * deleted,
                                GList * disconnects)
{
  TableChange * change;

  change = g_new (TableChange, 1);

  change->obj_change.apply = (ObjectChangeApplyFunc) table_change_apply;
  change->obj_change.revert = (ObjectChangeRevertFunc) table_change_revert;
  change->obj_change.free = (ObjectChangeFreeFunc) table_change_free;

  change->obj = table;
  change->added_cp = added;
  change->deleted_cp = deleted;
  change->disconnected = disconnects;
  change->applied = TRUE;
  change->saved_state = saved_state;

  return change;
}

/**
 * Called to REDO a change on the table object.
 */
static void
table_change_apply (TableChange * change, DiaObject * obj)
{
  TableState * old_state;
  GList * lst;

  g_print ("apply (o: 0x%08x) (c: 0x%08x)\n", (guint) obj, (guint) change);

  /* first the get the current state for later use */
  old_state = table_state_new (change->obj);
  /* now apply the change */
  table_state_set (change->saved_state, change->obj);

  lst = change->disconnected;
  while (lst)
    {
      Disconnect * dis = (Disconnect *) lst->data;
      object_unconnect (dis->other_object, dis->other_handle);
      lst = g_list_next (lst);
    }
  change->saved_state = old_state;
  change->applied = TRUE;
}

/**
 * Called to UNDO a change on the table object.
 */
static void
table_change_revert (TableChange *change, DiaObject *obj)
{
  TableState *old_state;
  GList *list;

  old_state = table_state_new(change->obj);

  table_state_set(change->saved_state, change->obj);
  
  list = change->disconnected;
  while (list) {
    Disconnect *dis = (Disconnect *)list->data;

    object_connect(dis->other_object, dis->other_handle, dis->cp);

    list = g_list_next(list);
  }
  
  change->saved_state = old_state;
  change->applied = FALSE;
}

static void
table_change_free (TableChange *change)
{
  GList * free_list, * lst;

  table_state_free (change->saved_state);

  free_list = (change->applied == TRUE)
    ? change->deleted_cp
    : change->added_cp;

  lst = free_list;
  while (lst)
    {
      ConnectionPoint * cp = (ConnectionPoint *) lst->data;
      g_assert (cp->connected == NULL);
      object_remove_connections_to (cp);
      g_free (cp);

      lst = g_list_next (lst);
    }
  g_list_free (free_list);
}

/* ------------------------------------------------------------------------ */

static void
general_page_props_to_object (Table * table, TablePropDialog * prop_dialog)
{
  gchar * s;

  /* name of the table */
  if (table->name != NULL)
    g_free (table->name);
  s = (gchar *) gtk_entry_get_text (prop_dialog->table_name);
  if (s != NULL && s[0] != '\0')
    table->name = g_strdup (s);
  else
    table->name = NULL;

  /* the table's comment */
  if (table->comment != NULL)
    g_free (table->comment);
  s = (gchar *) get_comment (prop_dialog->table_comment);
  if (s != NULL && s[0] != '\0')
    table->comment = g_strdup (s);
  else
    table->comment = NULL;

  table->visible_comment =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prop_dialog->comment_visible));
  table->tagging_comment =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prop_dialog->comment_tagging));
  table->underline_primary_key =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prop_dialog->underline_primary_key));
  table->bold_primary_key =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prop_dialog->bold_primary_key));

  table->border_width =
    gtk_spin_button_get_value (GTK_SPIN_BUTTON (prop_dialog->border_width));

  table->normal_font = dia_font_selector_get_font (prop_dialog->normal_font);
  table->name_font = dia_font_selector_get_font (prop_dialog->name_font);
  table->comment_font = dia_font_selector_get_font (prop_dialog->comment_font);

  table->normal_font_height =
    gtk_spin_button_get_value_as_float (prop_dialog->normal_font_height);
  table->name_font_height =
    gtk_spin_button_get_value_as_float (prop_dialog->name_font_height);
  table->comment_font_height =
    gtk_spin_button_get_value_as_float (prop_dialog->comment_font_height);

  dia_color_selector_get_color (GTK_WIDGET (prop_dialog->text_color),
                                &table->text_color);
  dia_color_selector_get_color (GTK_WIDGET (prop_dialog->line_color),
                                &table->line_color);
  dia_color_selector_get_color (GTK_WIDGET (prop_dialog->fill_color),
                                &table->fill_color);
}

static void
attribute_page_props_to_object (Table * table, TablePropDialog * prop_dialog)
{
  GList * list;
  GList * clear_list = NULL;
  TableAttribute * attr;
  GtkWidget * list_item;
  DiaObject * obj;
  ConnectionPoint * cp;

  /* if there is any currently edited item ... update its values */
  attributes_page_update_cur_attr_item (prop_dialog);

  obj = &table->element.object;

  /* free current attributes */
  list = table->attributes;
  while (list != NULL)
    {
      attr = (TableAttribute *) list->data;
      table_attribute_free (attr);
      /* do not free the connection points they are shared by
       * the corresponding attribute's stored in the gtklist on
       * the attributes page
       */
      list = g_list_next (list);
    }
  g_list_free (table->attributes);
  table->attributes = NULL;

  /* insert new attributes and remove them from gtklist */
  list = GTK_LIST (prop_dialog->attributes_list)->children;
  while (list != NULL)
    {
      list_item = GTK_WIDGET (list->data);

      clear_list = g_list_prepend (clear_list, list_item);
      attr = (TableAttribute *)
        gtk_object_get_user_data (GTK_OBJECT (list_item));
      /* set to NULL so the attribute get's not freed when list_item is
         destroyed */
      gtk_object_set_user_data (GTK_OBJECT (list_item), NULL);
      table->attributes = g_list_append (table->attributes, attr);

      list = g_list_next (list);
    }

  table_update_connectionpoints (table);

  if (clear_list != NULL)
    {
      clear_list = g_list_reverse (clear_list);
      gtk_list_remove_items (GTK_LIST (prop_dialog->attributes_list), clear_list);
      g_list_free (clear_list);
    }

  list = prop_dialog->deleted_connections;
  while (list != NULL)
    {
      cp = (ConnectionPoint *) list->data;

      table_dialog_store_disconnects (prop_dialog, cp);
      object_remove_connections_to (cp);

      list = g_list_next (list);
    }
}

static void
fill_in_dialog (Table * table)
{
  general_page_fill_in_dialog (table);
  attributes_page_fill_in_dialog (table);
}

static void
general_page_fill_in_dialog (Table * table)
{
  TablePropDialog * prop_dialog = table->prop_dialog;

  if (table->name)
    gtk_entry_set_text (prop_dialog->table_name, table->name);
  if (table->comment)
    set_comment (prop_dialog->table_comment, table->comment);
  else
    set_comment (prop_dialog->table_comment, "");

  gtk_toggle_button_set_active (prop_dialog->comment_visible, table->visible_comment);
  gtk_toggle_button_set_active (prop_dialog->comment_tagging, table->tagging_comment);
  gtk_toggle_button_set_active (prop_dialog->underline_primary_key, table->underline_primary_key);
  gtk_toggle_button_set_active (prop_dialog->bold_primary_key, table->bold_primary_key);
  gtk_spin_button_set_value (prop_dialog->border_width, table->border_width);

  dia_font_selector_set_font (prop_dialog->normal_font, table->normal_font);
  dia_font_selector_set_font (prop_dialog->name_font, table->name_font);
  dia_font_selector_set_font (prop_dialog->comment_font, table->comment_font);

  dia_color_selector_set_color(GTK_WIDGET(prop_dialog->text_color), &table->text_color);
  dia_color_selector_set_color(GTK_WIDGET(prop_dialog->line_color), &table->line_color);
  dia_color_selector_set_color(GTK_WIDGET(prop_dialog->fill_color), &table->fill_color);
}

static void
attributes_page_fill_in_dialog (Table * table)
{
  TablePropDialog * prop_dialog;
  GList * list;

  prop_dialog = table->prop_dialog;

  if (prop_dialog->attributes_list->children == NULL)
    {
      list = table->attributes;
      while (list != NULL)
        {
          TableAttribute * attr = (TableAttribute *) list->data;
          TableAttribute * attr_copy = table_attribute_copy (attr);
          /* see `attribute_page_props_to_object' why we take over
           * the original connection-points instead of creating new ones.
           */
          attr_copy->left_connection = attr->left_connection;
          attr_copy->right_connection = attr->right_connection;

          attributes_list_add_attribute (table, attr_copy, FALSE);

          list = g_list_next (list);
        }

      prop_dialog->cur_attr_list_item = NULL;
      attributes_page_set_sensitive (prop_dialog, FALSE);
      attributes_page_clear_values (prop_dialog);
    }
}

static void
create_dialog_pages(GtkNotebook * notebook, Table * table)
{
  create_general_page (notebook, table);
  create_attribute_page (notebook, table);
  create_style_page (notebook, table);
}

static void
create_attribute_page (GtkNotebook * notebook, Table * table)
{
  TablePropDialog * prop_dialog;
  GtkWidget * page_label;
  GtkWidget * vbox;
  GtkWidget * hbox;
  GtkWidget * scrolledwindow;
  GtkWidget * list;
  GtkWidget * vbox2;
  GtkWidget * button;
  GtkWidget * frame;
  GtkWidget * gtk_table;
  GtkWidget * label;
  GtkWidget * entry;
  GtkWidget * checkbox;

  prop_dialog = table->prop_dialog;

  page_label = gtk_label_new_with_mnemonic (_("_Attributes"));

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

  hbox = gtk_hbox_new (FALSE, 5);
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_widget_show (scrolledwindow);

  list = gtk_list_new ();
  prop_dialog->attributes_list = GTK_LIST (list);
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwindow), list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
                                       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolledwindow)));
  gtk_signal_connect (GTK_OBJECT (list), "selection_changed",
                      GTK_SIGNAL_FUNC (attributes_list_selection_changed_cb),
                      table);
  gtk_widget_show (list);

  /* vbox for the button on the right side of the attributes list */
  vbox2 = gtk_vbox_new (FALSE, 5);

  /* the "new" button */
  button = gtk_button_new_with_mnemonic (_("_New"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (attributes_list_new_button_clicked_cb),
                      table);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  /* the "delete" button */
  button = gtk_button_new_with_mnemonic (_("_Delete"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (attributes_list_delete_button_clicked_cb),
                      table);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  /* the "Move up" button */
  button = gtk_button_new_with_mnemonic (_("Move up"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (attributes_list_moveup_button_clicked_cb),
                      table);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  /* the "Move down" button */
  button = gtk_button_new_with_mnemonic (_("Move down"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (attributes_list_movedown_button_clicked_cb),
                      table);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  frame = gtk_frame_new (_("Attribute data"));
  vbox2 = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 10);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

  gtk_table = gtk_table_new (4, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox2), gtk_table, FALSE, FALSE, 0);

  label = gtk_label_new (_("Name:"));
  entry = gtk_entry_new ();
  prop_dialog->attribute_name = GTK_ENTRY (entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
                      GTK_SIGNAL_FUNC (current_attribute_update_event), table);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (current_attribute_update), table);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (gtk_table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
  gtk_table_attach (GTK_TABLE (gtk_table), entry, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, 0, 0, 2);


  label = gtk_label_new (_("Type:"));
  entry = gtk_entry_new ();
  prop_dialog->attribute_type = GTK_ENTRY (entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
                      GTK_SIGNAL_FUNC (current_attribute_update_event), table);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
                      GTK_SIGNAL_FUNC (current_attribute_update), table);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (gtk_table), label, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
  gtk_table_attach (GTK_TABLE (gtk_table), entry, 1, 2, 1, 2, GTK_FILL | GTK_EXPAND, 0, 0, 2);


  label = gtk_label_new (_("Comment:"));
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
                                       GTK_SHADOW_IN);
  entry = gtk_text_view_new ();
  prop_dialog->attribute_comment = GTK_TEXT_VIEW (entry);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), entry);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (entry), TRUE);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (current_attribute_update_event), table);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (gtk_table), label, 0, 1, 2, 3, GTK_FILL, 0, 0, 0);
  gtk_table_attach (GTK_TABLE (gtk_table), scrolledwindow, 1, 2, 2, 3, GTK_FILL | GTK_EXPAND, 0, 0, 2);

  /* start a new gtk-table */
  gtk_table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox2), gtk_table, FALSE, FALSE, 0);

  checkbox = gtk_check_button_new_with_mnemonic (_("_Primary key"));
  gtk_signal_connect (GTK_OBJECT (checkbox), "toggled",
              GTK_SIGNAL_FUNC (attribute_primary_key_toggled_cb), table);
  prop_dialog->attribute_primary_key = GTK_TOGGLE_BUTTON (checkbox);
  gtk_table_attach (GTK_TABLE (gtk_table), checkbox, 0, 1, 0, 1,
                    GTK_FILL | GTK_EXPAND, 0, 0, 0);

  checkbox = gtk_check_button_new_with_mnemonic (_("N_ullable"));
  gtk_signal_connect (GTK_OBJECT (checkbox), "toggled",
              GTK_SIGNAL_FUNC (attribute_nullable_toggled_cb), table);
  prop_dialog->attribute_nullable = GTK_TOGGLE_BUTTON (checkbox);
  gtk_table_attach (GTK_TABLE (gtk_table), checkbox, 1, 2, 0, 1,
                    GTK_FILL | GTK_EXPAND, 0, 0, 0);

  checkbox = gtk_check_button_new_with_mnemonic (_("Uni_que"));
  gtk_signal_connect (GTK_OBJECT (checkbox), "toggled",
                      GTK_SIGNAL_FUNC (attribute_unique_toggled_cb), table);
  prop_dialog->attribute_unique = GTK_TOGGLE_BUTTON (checkbox);
  gtk_table_attach (GTK_TABLE (gtk_table), checkbox, 1, 2, 1, 2,
                    GTK_FILL | GTK_EXPAND, 0, 0, 0);

  /* show the notebook page */
  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page (notebook, vbox, page_label);
}

static void
attributes_page_set_sensitive (TablePropDialog * prop_dialog, gboolean val)
{
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->attribute_name), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->attribute_type), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->attribute_comment), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->attribute_primary_key), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->attribute_nullable), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->attribute_unique), val);
}

static void
attributes_page_clear_values(TablePropDialog * prop_dialog)
{
  gtk_entry_set_text(prop_dialog->attribute_name, "");
  gtk_entry_set_text(prop_dialog->attribute_type, "");
  set_comment(prop_dialog->attribute_comment, "");
  gtk_toggle_button_set_active (prop_dialog->attribute_primary_key, FALSE);
  gtk_toggle_button_set_active (prop_dialog->attribute_nullable, TRUE);
  gtk_toggle_button_set_active (prop_dialog->attribute_unique, FALSE);
}

/**
 * Copy the properties from an attribute to the attribute page.
 */
static void
attributes_page_set_values (TablePropDialog * prop_dialog, TableAttribute * attr)
{
  gtk_entry_set_text (prop_dialog->attribute_name, attr->name);
  gtk_entry_set_text (prop_dialog->attribute_type, attr->type);
  set_comment (prop_dialog->attribute_comment, attr->comment);
  gtk_toggle_button_set_active (prop_dialog->attribute_primary_key,
                                attr->primary_key);
  gtk_toggle_button_set_active (prop_dialog->attribute_nullable,
                                attr->nullable);
  gtk_toggle_button_set_active (prop_dialog->attribute_unique,
                                attr->unique);
}

/**
 * Copy the properties of an attribute from the attribute page to a
 * concrete attribute.
 */
static void
attributes_page_values_to_attribute (TablePropDialog * prop_dialog,
                                     TableAttribute * attr)
{
  if (attr->name) g_free (attr->name);
  if (attr->type) g_free (attr->type);
  if (attr->comment) g_free (attr->comment);

  attr->name = g_strdup (gtk_entry_get_text (prop_dialog->attribute_name));
  attr->type = g_strdup (gtk_entry_get_text (prop_dialog->attribute_type));
  attr->comment = g_strdup (get_comment (prop_dialog->attribute_comment));
  attr->primary_key = gtk_toggle_button_get_active (prop_dialog->attribute_primary_key);
  attr->nullable = gtk_toggle_button_get_active (prop_dialog->attribute_nullable);
  attr->unique = gtk_toggle_button_get_active (prop_dialog->attribute_unique);
}

/**
 * Update the current attribute from the attributes page. This involves
 * copying the new values into the attribute as well as updating the
 * displayed string in the attributes listbox.
 */
static void
attributes_page_update_cur_attr_item (TablePropDialog * prop_dialog)
{
  TableAttribute * attr;
  GtkLabel * label;
  gchar * str;

  if (prop_dialog->cur_attr_list_item != NULL)
    {
      attr = (TableAttribute *)
        gtk_object_get_user_data (GTK_OBJECT (prop_dialog->cur_attr_list_item));
      if (attr != NULL)
        {
          attributes_page_values_to_attribute (prop_dialog, attr);
          label = GTK_LABEL (GTK_BIN (prop_dialog->cur_attr_list_item)->child);
          str = table_get_attribute_string (attr);
          gtk_label_set_text (label, str);
          g_free (str);
        }
    }
}

static void
attribute_primary_key_toggled_cb (GtkToggleButton * button, Table * table)
{
  gboolean active;
  TablePropDialog * prop_dialog = table->prop_dialog;

  active = gtk_toggle_button_get_active (prop_dialog->attribute_primary_key);

  if (active)
    {
      gtk_toggle_button_set_active (prop_dialog->attribute_nullable, FALSE);
      gtk_toggle_button_set_active (prop_dialog->attribute_unique, TRUE);
    }
  attributes_page_update_cur_attr_item (prop_dialog);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->attribute_nullable),
                            !active);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->attribute_unique),
                            !active);
}

static void
attribute_nullable_toggled_cb (GtkToggleButton * button, Table * table)
{
  TablePropDialog * prop_dialog = table->prop_dialog;

  attributes_page_update_cur_attr_item (prop_dialog);
}

static void
attribute_unique_toggled_cb (GtkToggleButton * button, Table * table)
{
  TablePropDialog * prop_dialog = table->prop_dialog;

  attributes_page_update_cur_attr_item (prop_dialog);
}

static void
attributes_list_selection_changed_cb (GtkWidget * gtklist, Table * table)
{
  TablePropDialog * prop_dialog;
  GList * list;
  TableAttribute * attr;
  GtkObject * list_item;

  /* Due to GtkList oddities, this may get called during destroy.
   * But it'll reference things that are already dead and crash.
   * Thus, we stop it before it gets that bad.  See bug #156706 for
   * one example.
   */
  if (table->destroyed == TRUE)
    return;

  prop_dialog = table->prop_dialog;
  attributes_page_update_cur_attr_item (prop_dialog);

  list = GTK_LIST (gtklist)->selection;
  if (list == NULL)
    {
      prop_dialog->cur_attr_list_item = NULL;

      attributes_page_set_sensitive (prop_dialog, FALSE);
      attributes_page_clear_values (prop_dialog);
    }
  else
    {
      list_item = GTK_OBJECT (list->data);
      attr = (TableAttribute *) gtk_object_get_user_data (list_item);
      attributes_page_set_sensitive (prop_dialog, TRUE);
      attributes_page_set_values (prop_dialog, attr);

      prop_dialog->cur_attr_list_item = GTK_LIST_ITEM (list_item);
      gtk_widget_grab_focus (GTK_WIDGET (prop_dialog->attribute_name));
    }
}

static void
create_general_page (GtkNotebook * notebook, Table * table)
{
  TablePropDialog * prop_dialog;
  GtkWidget * page_label;
  GtkWidget * vbox;
  GtkWidget * gtk_table;
  GtkWidget * label;
  GtkWidget * entry;
  GtkWidget * scrolled_window;
  GtkWidget * checkbox;

  prop_dialog = table->prop_dialog;

  page_label = gtk_label_new_with_mnemonic (_("_Table"));

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

  gtk_table = gtk_table_new (3, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), gtk_table, FALSE, FALSE, 0);

  label = gtk_label_new (_("Table name:"));
  entry = gtk_entry_new ();
  prop_dialog->table_name = GTK_ENTRY (entry);
  gtk_widget_grab_focus (entry);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (gtk_table), label, 0, 1, 0, 1,
                    GTK_FILL, 0, 0, 0);
  gtk_table_attach (GTK_TABLE (gtk_table), entry, 1, 2, 0, 1,
                    GTK_FILL | GTK_EXPAND, 0, 0, 2);

  label = gtk_label_new(_("Comment:"));
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_table_attach (GTK_TABLE (gtk_table), scrolled_window, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  entry = gtk_text_view_new ();
  prop_dialog->table_comment = GTK_TEXT_VIEW(entry);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (gtk_table), label, 0,1,2,3, GTK_FILL,0, 0,0);
  gtk_container_add (GTK_CONTAINER (scrolled_window), entry);

  /* --- table for various checkboxes */
  gtk_table = gtk_table_new (2, 2, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), gtk_table, FALSE, FALSE, 0);

  /* XXX create a handler and disable the 'show documentation tag' checkbox
     if 'comment visible' is not active. */
  checkbox = gtk_check_button_new_with_label(_("Comment visible"));
  prop_dialog->comment_visible = GTK_TOGGLE_BUTTON(checkbox);
  gtk_table_attach (GTK_TABLE (gtk_table), checkbox, 0, 1, 0, 1,
                    GTK_FILL, 10, 0, 0);

  checkbox = gtk_check_button_new_with_label(_("Show documentation tag"));
  prop_dialog->comment_tagging = GTK_TOGGLE_BUTTON( checkbox );
  gtk_table_attach (GTK_TABLE (gtk_table), checkbox, 1, 2, 0, 1,
                    GTK_FILL, 10, 0, 0);

  checkbox = gtk_check_button_new_with_label (_("Underline primary keys"));
  prop_dialog->underline_primary_key = GTK_TOGGLE_BUTTON (checkbox);
  gtk_table_attach (GTK_TABLE (gtk_table), checkbox, 0, 1, 1, 2,
                    GTK_FILL, 10, 0, 0);

  checkbox = gtk_check_button_new_with_label (_("Use bold font for primary keys"));
  prop_dialog->bold_primary_key = GTK_TOGGLE_BUTTON (checkbox);
  gtk_table_attach (GTK_TABLE (gtk_table), checkbox, 1, 2, 1, 2,
                    GTK_FILL, 10, 0, 0);
  /* +++ end of the table for various checkboxes */

  /* finally append the created vbox to the notebook */
  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page (notebook, vbox, page_label);

}

static void
create_style_page (GtkNotebook * notebook, Table * table)
{
  TablePropDialog * prop_dialog;
  GtkWidget * page_label;
  GtkWidget * vbox;
  GtkWidget * gtk_table;
  GtkWidget * label;
  GtkWidget * hbox;
  GtkWidget * text_color;
  GtkWidget * fill_color;
  GtkWidget * line_color;
  GtkObject * adj;

  prop_dialog = table->prop_dialog;

  page_label = gtk_label_new_with_mnemonic (_("_Style"));

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

  hbox = gtk_hbox_new (FALSE, 5);
  adj = gtk_adjustment_new (table->border_width, 0.00, 10.0, 0.01, 0.1, 1.0);
  prop_dialog->border_width =
    GTK_SPIN_BUTTON (gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0.1, 2));
  gtk_spin_button_set_snap_to_ticks (prop_dialog->border_width, TRUE);
  gtk_spin_button_set_numeric (prop_dialog->border_width, TRUE);
  label = gtk_label_new (_("Border width:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (prop_dialog->border_width),
                      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, FALSE, 3);

  /* fonts */
  gtk_table = gtk_table_new (5, 6, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), gtk_table, FALSE, TRUE, 0);
  gtk_table_set_homogeneous (GTK_TABLE (gtk_table), FALSE);

  label = gtk_label_new (_("Kind"));
  gtk_table_attach_defaults (GTK_TABLE (gtk_table), label, 0, 1, 0, 1);
  label = gtk_label_new (_("Font"));
  gtk_table_attach_defaults (GTK_TABLE (gtk_table), label, 1, 2, 0, 1);
  label = gtk_label_new (_("Size"));
  gtk_table_attach_defaults (GTK_TABLE (gtk_table), label, 2, 3, 0, 1);

  create_font_props_row (GTK_TABLE (gtk_table), _("Normal:"), 3,
                         table->normal_font,
                         table->normal_font_height,
                         &(prop_dialog->normal_font),
                         &(prop_dialog->normal_font_height));
  create_font_props_row (GTK_TABLE (gtk_table), _("Name:"), 4,
                         table->name_font,
                         table->name_font_height,
                         &(prop_dialog->name_font),
                         &(prop_dialog->name_font_height));
  create_font_props_row (GTK_TABLE (gtk_table), _("Comment:"), 5,
                         table->comment_font,
                         table->comment_font_height,
                         &(prop_dialog->comment_font),
                         &(prop_dialog->comment_font_height));

  gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new (), FALSE, FALSE, 3);

  /* colors */
  gtk_table = gtk_table_new (2, 3, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), gtk_table, FALSE, TRUE, 0);
  label = gtk_label_new(_("Text Color:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (gtk_table), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 2);
  text_color = dia_color_selector_new();
  dia_color_selector_set_color(text_color, &table->text_color);
  prop_dialog->text_color = (DiaColorSelector *)text_color;
  gtk_table_attach (GTK_TABLE (gtk_table), text_color, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 3, 2);

  label = gtk_label_new(_("Foreground Color:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (gtk_table), label, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 2);
  line_color = dia_color_selector_new();
  dia_color_selector_set_color(line_color, &table->line_color);
  prop_dialog->line_color = (DiaColorSelector *)line_color;
  gtk_table_attach (GTK_TABLE (gtk_table), line_color, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 3, 2);
  
  label = gtk_label_new(_("Background Color:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (gtk_table), label, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 2);
  fill_color = dia_color_selector_new();
  dia_color_selector_set_color(fill_color, &table->fill_color);
  prop_dialog->fill_color = (DiaColorSelector *)fill_color;
  gtk_table_attach (GTK_TABLE (gtk_table), fill_color, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, 0, 3, 2);

  /* finally append the created vbox to the notebook */
  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page (notebook, vbox, page_label);
}

/**
 * pn: "Borrowed" from the source code of the UML class object.
 */
static void
create_font_props_row (GtkTable   *table,
                       const char *kind,
                       gint        row,
                       DiaFont    *font,
                       real        height,
                       DiaFontSelector **fontsel,
                       GtkSpinButton   **heightsel)
{
  GtkWidget *label;
  GtkObject *adj;

  label = gtk_label_new (kind);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (table, label, 0, 1, row, row+1);
  *fontsel = DIAFONTSELECTOR (dia_font_selector_new ());
  dia_font_selector_set_font (DIAFONTSELECTOR (*fontsel), font);
  gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET(*fontsel), 1, 2, row, row+1);

  adj = gtk_adjustment_new (height, 0.1, 10.0, 0.1, 1.0, 1.0);
  *heightsel = GTK_SPIN_BUTTON (gtk_spin_button_new (GTK_ADJUSTMENT(adj), 1.0, 2));
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (*heightsel), TRUE);
  gtk_table_attach_defaults (table, GTK_WIDGET (*heightsel), 2, 3, row, row+1);
}

static void
attributes_list_new_button_clicked_cb (GtkWidget * button, Table * table)
{
  TablePropDialog * prop_dialog;
  TableAttribute * attr;

  prop_dialog = table->prop_dialog;
  attributes_page_update_cur_attr_item (prop_dialog);

  attr = table_attribute_new ();
  table_attribute_ensure_connection_points (attr, &table->element.object);

  prop_dialog->added_connections =
    g_list_append (prop_dialog->added_connections, attr->left_connection);
  prop_dialog->added_connections =
    g_list_append (prop_dialog->added_connections, attr->right_connection);

  attributes_list_add_attribute (table, attr, TRUE);
}

/**
 * Add the passed attribute into the listbox of attributes. The passed
 * attribute should be a copy of the original attribute as it will be
 * freed when destroying the attribute listbox. If SELECT is TRUE the
 * new list item is selected.
 */
static void
attributes_list_add_attribute (Table * table,
                               TableAttribute * attribute,
                               gboolean select)
{
  TablePropDialog * prop_dialog;
  gchar * attrstr;
  GtkWidget * list_item;
  GList * list;

  prop_dialog = table->prop_dialog;

  attrstr = table_get_attribute_string (attribute);
  list_item = gtk_list_item_new_with_label (attrstr);
  gtk_widget_show (list_item);
  g_free (attrstr);

  gtk_object_set_user_data (GTK_OBJECT (list_item), attribute);
  gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
                      GTK_SIGNAL_FUNC (attribute_list_item_destroy_cb),
                      NULL);
  list = g_list_append (NULL, list_item);
  gtk_list_append_items (prop_dialog->attributes_list, list);
  if (select)
    {
      if (prop_dialog->attributes_list->children != NULL)
        gtk_list_unselect_child (prop_dialog->attributes_list,
                GTK_WIDGET (prop_dialog->attributes_list->children->data));
      gtk_list_select_child (prop_dialog->attributes_list, list_item);
    }
}

static void
current_attribute_update (GtkWidget * unused, Table * table)
{
  attributes_page_update_cur_attr_item (table->prop_dialog);
}

static int
current_attribute_update_event (GtkWidget * unused,
                                GdkEventFocus *unused2,
                                Table * table)
{
  attributes_page_update_cur_attr_item (table->prop_dialog);
  return 0;
}


static void
attributes_list_delete_button_clicked_cb (GtkWidget * button, Table * table)
{
  GList * list;
  GtkList * gtklist;
  TablePropDialog * prop_dialog;
  TableAttribute * attr;

  prop_dialog = table->prop_dialog;
  gtklist = GTK_LIST (prop_dialog->attributes_list);

  if (gtklist->selection != NULL)
    {
      attr = (TableAttribute *)
        gtk_object_get_user_data (GTK_OBJECT (gtklist->selection->data));
      prop_dialog->deleted_connections =
        g_list_prepend (prop_dialog->deleted_connections,
                        attr->left_connection);
      prop_dialog->deleted_connections =
        g_list_prepend (prop_dialog->deleted_connections,
                        attr->right_connection);

      list = g_list_append (NULL, gtklist->selection->data);
      gtk_list_remove_items (gtklist, list);
      g_list_free (list);
      attributes_page_clear_values (prop_dialog);
      attributes_page_set_sensitive (prop_dialog, FALSE);
    }
}

static void
attributes_list_moveup_button_clicked_cb (GtkWidget * button, Table * table)
{
  TablePropDialog * prop_dialog;
  GtkList * gtklist;
  GList * list;
  GtkWidget * list_item;
  gint i;

  prop_dialog = table->prop_dialog;
  gtklist = GTK_LIST (prop_dialog->attributes_list);

  if (gtklist->selection != NULL)
    {
      list_item = GTK_WIDGET (gtklist->selection->data);

      i = gtk_list_child_position (gtklist, list_item);
      if (i > 0)
        {
          i--;

          gtk_widget_ref (list_item);
          list = g_list_prepend (NULL, list_item);
          gtk_list_remove_items (gtklist, list);
          gtk_list_insert_items (gtklist, list, i);
          gtk_widget_unref (list_item);

          gtk_list_select_child (gtklist, list_item);
        }
    }
}

static void
attributes_list_movedown_button_clicked_cb (GtkWidget * button, Table * table)
{
  TablePropDialog * prop_dialog;
  GtkList * gtklist;
  GList * list;
  GtkWidget * list_item;
  gint i;

  prop_dialog = table->prop_dialog;
  gtklist = GTK_LIST (prop_dialog->attributes_list);

  if (gtklist->selection != NULL)
    {
      list_item = GTK_WIDGET (gtklist->selection->data);

      i = gtk_list_child_position (gtklist, list_item);
      if (i < (g_list_length (gtklist->children) - 1))
        {
          i++;

          gtk_widget_ref (list_item);
          list = g_list_prepend (NULL, list_item);
          gtk_list_remove_items (gtklist, list);
          gtk_list_insert_items (gtklist, list, i);
          gtk_widget_unref (list_item);

          gtk_list_select_child (gtklist, list_item);
        }
    }
}

static void
attribute_list_item_destroy_cb (GtkWidget * list_item, gpointer data)
{
  TableAttribute * attr;

  attr = (TableAttribute *) gtk_object_get_user_data (GTK_OBJECT (list_item));
  if (attr != NULL)
    table_attribute_free (attr);
}

static void
destroy_prop_dialog (GtkWidget *widget, gpointer user_data)
{
  Table * table = (Table *) user_data;

  g_free (table->prop_dialog);
  table->prop_dialog = NULL;
}

/*
 * pn: borrowed from the source code of the UML class.
 *
 * get the contents of a comment text view.
 */
const gchar * get_comment(GtkTextView *view)
{
  GtkTextBuffer * buffer = gtk_text_view_get_buffer(view);
  GtkTextIter start;
  GtkTextIter end;

  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

/*
 * pn: "borrowed" from the source code for the UML class.
 */
static void
set_comment(GtkTextView *view, gchar *text)
{
  GtkTextBuffer * buffer = gtk_text_view_get_buffer(view);
  GtkTextIter start;
  GtkTextIter end;

  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);
  gtk_text_buffer_delete(buffer,&start,&end);
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_insert( buffer, &start, text, strlen(text));
}

/**
 * pn: "borrowed" from the source code for the UML class object.
 */
static void
table_dialog_store_disconnects (TablePropDialog * prop_dialog,
                                ConnectionPoint * cp)
{
  Disconnect *dis;
  DiaObject *connected_obj;
  GList *list;
  gint i;

  list = cp->connected;
  while (list != NULL) {
    connected_obj = (DiaObject *)list->data;

    for (i=0;i<connected_obj->num_handles;i++) {
      if (connected_obj->handles[i]->connected_to == cp) {
        dis = g_new0(Disconnect, 1);
        dis->cp = cp;
        dis->other_object = connected_obj;
        dis->other_handle = connected_obj->handles[i];

        prop_dialog->disconnected_connections =
          g_list_prepend(prop_dialog->disconnected_connections, dis);
      }
    }
    list = g_list_next(list);
  }
}


/**
 * Return the string representation of the passed attribute to be
 * rendered. The returned string is allocated in memory and its the
 * caller's responsibility to free this memory if it's not needed
 * anymore.
 */
static gchar *
table_get_attribute_string (TableAttribute * attrib)
{
  int len = 2; /* two chars at the beginning */
  gchar * not_null_str = _("not null");
  gchar * null_str = _("null");
  gchar * is_unique_str = _("unique");
  gchar * str = NULL;
  gchar * strp = NULL;
  gboolean is_nullable = attrib->nullable;
  gboolean is_unique = attrib->unique;

  /* first compute how much we need to allocate */

  if (IS_NOT_EMPTY (attrib->name))
    len += strlen (attrib->name);
  if (IS_NOT_EMPTY (attrib->type))
    {
      len += strlen (attrib->type);
      len += 2; /* comma and a blank between the type and "nullable" */
    }
  len += strlen (is_nullable ? null_str : not_null_str);
  if (IS_NOT_EMPTY (attrib->name))
      len += 2; /* colon and a blank after the  name */
  if (is_unique)
    {
      len += 2; /* command and a blank */
      len += strlen (is_unique_str);
    }

  /* now create the string */

  /* add one gchar for the null termaintor */
  strp = str = g_malloc (sizeof(gchar) * (len + 1));
  strp = g_stpcpy (strp, (attrib->primary_key == TRUE) ? "# " : "  ");
  if (IS_NOT_EMPTY (attrib->name))
    {
      strp = g_stpcpy (strp, attrib->name);
      strp = g_stpcpy (strp, ": ");
    }
  if (IS_NOT_EMPTY (attrib->type))
    {
      strp = g_stpcpy (strp, attrib->type);
      strp = g_stpcpy (strp, ", ");
    }
  strp = g_stpcpy (strp, is_nullable ? null_str : not_null_str);
  if (is_unique)
    {
      strp = g_stpcpy (strp, ", ");
      strp = g_stpcpy (strp, is_unique_str);
    }

  g_assert (strlen (str) == len);

  return str;
}
