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
 */
#include <config.h>

#include <assert.h>
#undef GTK_DISABLE_DEPRECATED /* GtkList, ... */
#include <gtk/gtk.h>

#include "class.h"
#include "diaoptionmenu.h"

#include "class_dialog.h"

/************************************************************
 ******************** ATTRIBUTES ****************************
 ************************************************************/

static void
attributes_set_sensitive(UMLClassDialog *prop_dialog, gint val)
{
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_name), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_type), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_value), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_comment), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_visible), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_class_scope), val);
}

static void
attributes_set_values(UMLClassDialog *prop_dialog, UMLAttribute *attr)
{
  gtk_entry_set_text(prop_dialog->attr_name, attr->name);
  gtk_entry_set_text(prop_dialog->attr_type, attr->type);
  if (attr->value != NULL)
    gtk_entry_set_text(prop_dialog->attr_value, attr->value);
  else
    gtk_entry_set_text(prop_dialog->attr_value, "");

  if (attr->comment != NULL)
    _class_set_comment(prop_dialog->attr_comment, attr->comment);
  else
    _class_set_comment(prop_dialog->attr_comment, "");


  dia_option_menu_set_active(prop_dialog->attr_visible, attr->visibility);
  gtk_toggle_button_set_active(prop_dialog->attr_class_scope, attr->class_scope);
}

static void
attributes_clear_values(UMLClassDialog *prop_dialog)
{
  gtk_entry_set_text(prop_dialog->attr_name, "");
  gtk_entry_set_text(prop_dialog->attr_type, "");
  gtk_entry_set_text(prop_dialog->attr_value, "");
  _class_set_comment(prop_dialog->attr_comment, "");
  gtk_toggle_button_set_active(prop_dialog->attr_class_scope, FALSE);
}

static void
attributes_get_values (UMLClassDialog *prop_dialog, UMLAttribute *attr)
{
  g_free (attr->name);
  g_free (attr->type);
  if (attr->value != NULL)
    g_free (attr->value);

  attr->name = g_strdup (gtk_entry_get_text (prop_dialog->attr_name));
  attr->type = g_strdup (gtk_entry_get_text (prop_dialog->attr_type));
  
  attr->value = g_strdup (gtk_entry_get_text(prop_dialog->attr_value));
  attr->comment = g_strdup (_class_get_comment(prop_dialog->attr_comment));

  attr->visibility = (UMLVisibility)dia_option_menu_get_active (prop_dialog->attr_visible);
    
  attr->class_scope = prop_dialog->attr_class_scope->active;
}

void
_attributes_get_current_values(UMLClassDialog *prop_dialog)
{
  UMLAttribute *current_attr;
  GtkLabel *label;
  char *new_str;

  if (prop_dialog != NULL && prop_dialog->current_attr != NULL) {
    current_attr = (UMLAttribute *)
      g_object_get_data(G_OBJECT(prop_dialog->current_attr), "user_data");
    if (current_attr != NULL) {
      attributes_get_values(prop_dialog, current_attr);
      label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(prop_dialog->current_attr)));
      new_str = uml_get_attribute_string(current_attr);
      gtk_label_set_text (label, new_str);
      g_free (new_str);
    }
  }
}

static void
attribute_list_item_destroy_callback(GtkWidget *list_item,
				     gpointer data)
{
  UMLAttribute *attr;

  attr = (UMLAttribute *) g_object_get_data(G_OBJECT(list_item), "user_data");

  if (attr != NULL) {
    uml_attribute_destroy(attr);
    /*printf("Destroying list_item's user_data!\n");*/
  }
}

static void
attributes_list_selection_changed_callback(GtkWidget *gtklist,
					   UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkObject *list_item;
  UMLAttribute *attr;

  /* Due to GtkList oddities, this may get called during destroy.
   * But it'll reference things that are already dead and crash.
   * Thus, we stop it before it gets that bad.  See bug #156706 for
   * one example.
   */
  if (umlclass->destroyed)
    return;

  prop_dialog = umlclass->properties_dialog;

  if (!prop_dialog)
    return;

  _attributes_get_current_values(prop_dialog);
  
  list = GTK_LIST(gtklist)->selection;
  if (!list && prop_dialog) { /* No selected */
    attributes_set_sensitive(prop_dialog, FALSE);
    attributes_clear_values(prop_dialog);
    prop_dialog->current_attr = NULL;
    return;
  }
  
  list_item = GTK_OBJECT(list->data);
  attr = (UMLAttribute *)g_object_get_data(G_OBJECT(list_item), "user_data");
  attributes_set_values(prop_dialog, attr);
  attributes_set_sensitive(prop_dialog, TRUE);

  prop_dialog->current_attr = GTK_LIST_ITEM(list_item);
  gtk_widget_grab_focus(GTK_WIDGET(prop_dialog->attr_name));
}

static void
attributes_list_new_callback(GtkWidget *button,
			     UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkWidget *list_item;
  UMLAttribute *attr;
  char *utfstr;
  prop_dialog = umlclass->properties_dialog;

  _attributes_get_current_values(prop_dialog);

  attr = uml_attribute_new();
  /* need to make the new ConnectionPoint valid and remember them */
  uml_attribute_ensure_connection_points (attr, &umlclass->element.object);
  prop_dialog->added_connections = 
    g_list_prepend(prop_dialog->added_connections, attr->left_connection);
  prop_dialog->added_connections = 
    g_list_prepend(prop_dialog->added_connections, attr->right_connection);

  utfstr = uml_get_attribute_string (attr);
  list_item = gtk_list_item_new_with_label (utfstr);
  gtk_widget_show (list_item);
  g_free (utfstr);

  g_object_set_data(G_OBJECT(list_item), "user_data", attr);
  g_signal_connect (G_OBJECT (list_item), "destroy",
		    G_CALLBACK (attribute_list_item_destroy_callback), NULL);
  
  list = g_list_append(NULL, list_item);
  gtk_list_append_items(prop_dialog->attributes_list, list);

  if (prop_dialog->attributes_list->children != NULL)
    gtk_list_unselect_child(prop_dialog->attributes_list,
			    GTK_WIDGET(prop_dialog->attributes_list->children->data));
  gtk_list_select_child(prop_dialog->attributes_list, list_item);
}

static void
attributes_list_delete_callback(GtkWidget *button,
				UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkList *gtklist;
  UMLAttribute *attr;

  prop_dialog = umlclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->attributes_list);

  if (gtklist->selection != NULL) {
    attr = (UMLAttribute *)
      g_object_get_data(G_OBJECT(gtklist->selection->data), "user_data");

    if (attr->left_connection != NULL) {
      prop_dialog->deleted_connections =
	g_list_prepend(prop_dialog->deleted_connections,
		       attr->left_connection);
      prop_dialog->deleted_connections =
	g_list_prepend(prop_dialog->deleted_connections,
		       attr->right_connection);
    }
    
    list = g_list_prepend(NULL, gtklist->selection->data);
    gtk_list_remove_items(gtklist, list);
    g_list_free(list);
    attributes_clear_values(prop_dialog);
    attributes_set_sensitive(prop_dialog, FALSE);
  }
}

static void
attributes_list_move_up_callback(GtkWidget *button,
				 UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  int i;
  
  prop_dialog = umlclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->attributes_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);
    
    i = gtk_list_child_position(gtklist, list_item);
    if (i>0)
      i--;

    g_object_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    g_object_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }
}

static void
attributes_list_move_down_callback(GtkWidget *button,
				   UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  int i;

  prop_dialog = umlclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->attributes_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);
    
    i = gtk_list_child_position(gtklist, list_item);
    if (i<(g_list_length(gtklist->children)-1))
      i++;

    
    g_object_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    g_object_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }
}

void
_attributes_read_from_dialog(UMLClass *umlclass,
			    UMLClassDialog *prop_dialog,
			    int connection_index)
{
  GList *list;
  UMLAttribute *attr;
  GtkWidget *list_item;
  GList *clear_list;
  DiaObject *obj;

  obj = &umlclass->element.object;

  /* if the currently select attribute is changed, update the state in the
   * dialog info from widgets */
  _attributes_get_current_values(prop_dialog);
  /* Free current attributes: */
  list = umlclass->attributes;
  while (list != NULL) {
    attr = (UMLAttribute *)list->data;
    uml_attribute_destroy(attr);
    list = g_list_next(list);
  }
  g_list_free (umlclass->attributes);
  umlclass->attributes = NULL;

  /* Insert new attributes and remove them from gtklist: */
  list = GTK_LIST (prop_dialog->attributes_list)->children;
  clear_list = NULL;
  while (list != NULL) {
    list_item = GTK_WIDGET(list->data);
    
    clear_list = g_list_prepend (clear_list, list_item);
    attr = (UMLAttribute *)
      g_object_get_data(G_OBJECT(list_item), "user_data");
    g_object_set_data(G_OBJECT(list_item), "user_data", NULL);
    umlclass->attributes = g_list_append(umlclass->attributes, attr);
    
    if (attr->left_connection == NULL) {
      uml_attribute_ensure_connection_points (attr, obj);

      prop_dialog->added_connections =
	g_list_prepend(prop_dialog->added_connections,
		       attr->left_connection);
      prop_dialog->added_connections =
	g_list_prepend(prop_dialog->added_connections,
		       attr->right_connection);
    }

    if ( (prop_dialog->attr_vis->active) &&
	 (!prop_dialog->attr_supp->active) ) { 
      obj->connections[connection_index] = attr->left_connection;
      connection_index++;
      obj->connections[connection_index] = attr->right_connection;
      connection_index++;
    } else {
      _umlclass_store_disconnects(prop_dialog, attr->left_connection);
      object_remove_connections_to(attr->left_connection);
      _umlclass_store_disconnects(prop_dialog, attr->right_connection);
      object_remove_connections_to(attr->right_connection);
    }

    list = g_list_next(list);
  }
  clear_list = g_list_reverse (clear_list);
  gtk_list_remove_items (GTK_LIST (prop_dialog->attributes_list), clear_list);
  g_list_free (clear_list);

#if 0 /* UMLClass is *known* to be in an incositent state here, check later or crash ... */
  umlclass_sanity_check(umlclass, "Read from dialog");
#endif
}

void
_attributes_fill_in_dialog(UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLAttribute *attr_copy;
  GtkWidget *list_item;
  GList *list;
  int i;

#ifdef DEBUG
  umlclass_sanity_check(umlclass, "Filling in dialog");  
#endif

  prop_dialog = umlclass->properties_dialog;

  /* copy in new attributes: */
  if (prop_dialog->attributes_list->children == NULL) {
    i = 0;
    list = umlclass->attributes;
    while (list != NULL) {
      UMLAttribute *attr = (UMLAttribute *)list->data;
      gchar *attrstr = uml_get_attribute_string(attr);

      list_item = gtk_list_item_new_with_label (attrstr);
      attr_copy = uml_attribute_copy(attr);
      /* looks wrong but required for complicated ConnectionPoint memory management */
      attr_copy->left_connection = attr->left_connection;
      attr_copy->right_connection = attr->right_connection;
      g_object_set_data(G_OBJECT(list_item), "user_data", (gpointer) attr_copy);
      g_signal_connect (G_OBJECT (list_item), "destroy",
			G_CALLBACK (attribute_list_item_destroy_callback), NULL);
      gtk_container_add (GTK_CONTAINER (prop_dialog->attributes_list), list_item);
      gtk_widget_show (list_item);
      
      list = g_list_next(list); i++;
      g_free (attrstr);
    }
    /* set attributes non-sensitive */
    prop_dialog->current_attr = NULL;
    attributes_set_sensitive(prop_dialog, FALSE);
    attributes_clear_values(prop_dialog);
  }
}

static void
attributes_update(GtkWidget *widget, UMLClass *umlclass)
{
  _attributes_get_current_values(umlclass->properties_dialog);
}

static int
attributes_update_event(GtkWidget *widget, GdkEventFocus *ev, UMLClass *umlclass)
{
  _attributes_get_current_values(umlclass->properties_dialog);
  return 0;
}

void 
_attributes_create_page(GtkNotebook *notebook,  UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox2;
  GtkWidget *table;
  GtkWidget *entry;
  GtkWidget *checkbox;
  GtkWidget *scrolled_win;
  GtkWidget *button;
  GtkWidget *list;
  GtkWidget *frame;
  GtkWidget *omenu;
  GtkWidget *scrolledwindow;

  prop_dialog = umlclass->properties_dialog;

  /* Attributes page: */
  page_label = gtk_label_new_with_mnemonic (_("_Attributes"));
  
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

  hbox = gtk_hbox_new(FALSE, 5);
  
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  list = gtk_list_new ();
  prop_dialog->attributes_list = GTK_LIST(list);
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  gtk_widget_show (list);

  g_signal_connect (G_OBJECT (list), "selection_changed",
		    G_CALLBACK(attributes_list_selection_changed_callback), umlclass);

  vbox2 = gtk_vbox_new(FALSE, 5);

  button = gtk_button_new_from_stock (GTK_STOCK_NEW);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK(attributes_list_new_callback), umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK(attributes_list_delete_callback), umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_GO_UP);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK(attributes_list_move_up_callback), umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_GO_DOWN);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK(attributes_list_move_down_callback), umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  frame = gtk_frame_new(_("Attribute data"));
  vbox2 = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 10);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show(frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

  table = gtk_table_new (5, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

  label = gtk_label_new(_("Name:"));
  entry = gtk_entry_new();
  prop_dialog->attr_name = GTK_ENTRY(entry);
  g_signal_connect (G_OBJECT (entry), "focus_out_event",
		    G_CALLBACK (attributes_update_event), umlclass);
  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (attributes_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Type:"));
  entry = gtk_entry_new();
  prop_dialog->attr_type = GTK_ENTRY(entry);
  g_signal_connect (G_OBJECT (entry), "focus_out_event",
		    G_CALLBACK (attributes_update_event), umlclass);
  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (attributes_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Value:"));
  entry = gtk_entry_new();
  prop_dialog->attr_value = GTK_ENTRY(entry);
  g_signal_connect (G_OBJECT (entry), "focus_out_event",
		    G_CALLBACK (attributes_update_event), umlclass);
  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (attributes_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,2,3, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,2,3, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Comment:"));
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
				       GTK_SHADOW_IN);
  entry = gtk_text_view_new ();
  prop_dialog->attr_comment = GTK_TEXT_VIEW(entry);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), entry);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW (entry),TRUE);
  g_signal_connect (G_OBJECT (entry), "focus_out_event",
		    G_CALLBACK (attributes_update_event), umlclass);
#if 0 /* while the GtkEntry has a "activate" signal, GtkTextView does not.
       * Maybe we should connect to "set-focus-child" instead? 
       */
  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (attributes_update), umlclass);
#endif
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,3,4, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), scrolledwindow, 1,2,3,4, GTK_FILL | GTK_EXPAND,0, 0,2);


  label = gtk_label_new(_("Visibility:"));

  prop_dialog->attr_visible = omenu = dia_option_menu_new ();
  g_signal_connect (G_OBJECT (omenu), "changed",
		    G_CALLBACK (attributes_update), umlclass);
  dia_option_menu_add_item(omenu, _("Public"), UML_PUBLIC);
  dia_option_menu_add_item(omenu, _("Private"), UML_PRIVATE);
  dia_option_menu_add_item(omenu, _("Protected"), UML_PROTECTED);
  dia_option_menu_add_item(omenu, _("Implementation"), UML_IMPLEMENTATION);

  { 
    GtkWidget * align;
    align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
    gtk_container_add(GTK_CONTAINER(align), omenu);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0,1,4,5, GTK_FILL,0, 0,3);
    gtk_table_attach (GTK_TABLE (table), align, 1,2,4,5, GTK_FILL,0, 0,3);
  }

  hbox2 = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Class scope"));
  prop_dialog->attr_class_scope = GTK_TOGGLE_BUTTON(checkbox);
  gtk_box_pack_start (GTK_BOX (hbox2), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, TRUE, 0);
  
  gtk_widget_show(vbox2);
  
  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page(notebook, vbox, page_label);
  
}
