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

#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "config.h"
#include "object.h"
#include "objchange.h"
#include "intl.h"
#include "class.h"


/********************************************************
 ******************** CLASS *****************************
 ********************************************************/

static void
class_read_from_dialog(UMLClass *umlclass, UMLClassDialog *prop_dialog)
{
  char *str;

  g_free(umlclass->name);
  umlclass->name = strdup(gtk_entry_get_text(prop_dialog->classname));
  if (umlclass->stereotype != NULL)
    g_free(umlclass->stereotype);
  
  str = gtk_entry_get_text(prop_dialog->stereotype);
  if (strlen(str) != 0)
    umlclass->stereotype = strdup(str);
  else
    umlclass->stereotype = NULL;
  
  umlclass->abstract = prop_dialog->abstract_class->active;
  umlclass->visible_attributes = prop_dialog->attr_vis->active;
  umlclass->visible_operations = prop_dialog->op_vis->active;
  umlclass->suppress_attributes = prop_dialog->attr_supp->active;
  umlclass->suppress_operations = prop_dialog->op_supp->active;
}

static void
class_fill_in_dialog(UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  
  prop_dialog = umlclass->properties_dialog;

  gtk_entry_set_text(prop_dialog->classname, umlclass->name);
  if (umlclass->stereotype != NULL)
    gtk_entry_set_text(prop_dialog->stereotype, umlclass->stereotype);
  else 
    gtk_entry_set_text(prop_dialog->stereotype, "");

  gtk_toggle_button_set_active(prop_dialog->abstract_class, umlclass->abstract);
  gtk_toggle_button_set_active(prop_dialog->attr_vis, umlclass->visible_attributes);
  gtk_toggle_button_set_active(prop_dialog->op_vis, umlclass->visible_operations);
  gtk_toggle_button_set_active(prop_dialog->attr_supp, umlclass->suppress_attributes);
  gtk_toggle_button_set_active(prop_dialog->op_supp, umlclass->suppress_operations);
}

static void 
class_create_page(GtkNotebook *notebook,  UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *entry;
  GtkWidget *checkbox;

  prop_dialog = umlclass->properties_dialog;

  /* Class page: */
  page_label = gtk_label_new (_("Class"));
  
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
  
  hbox = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Class name:"));
  gtk_box_pack_start (GTK_BOX (hbox),
		      label, FALSE, TRUE, 0);
  entry = gtk_entry_new();
  prop_dialog->classname = GTK_ENTRY(entry);
  gtk_box_pack_start (GTK_BOX (hbox),
		      entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
		      hbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Stereotype:"));
  gtk_box_pack_start (GTK_BOX (hbox),
		      label, FALSE, TRUE, 0);
  entry = gtk_entry_new();
  prop_dialog->stereotype = GTK_ENTRY(entry);
  gtk_box_pack_start (GTK_BOX (hbox),
		      entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
		      hbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Abstract"));
  prop_dialog->abstract_class = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox),
		      checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
		      hbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Attributes visible"));
  prop_dialog->attr_vis = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox),
		      checkbox, TRUE, TRUE, 0);
  checkbox = gtk_check_button_new_with_label(_("Suppress Attributes"));
  prop_dialog->attr_supp = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox),
		      checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),          
		      hbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Operations visible"));
  prop_dialog->op_vis = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox),
		      checkbox, TRUE, TRUE, 0);
  checkbox = gtk_check_button_new_with_label(_("Suppress operations"));
  prop_dialog->op_supp = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox),
		      checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),
		      hbox, FALSE, TRUE, 0);
  
  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page(notebook, vbox, page_label);
  
}


/************************************************************
 ******************** ATTRIBUTES ****************************
 ************************************************************/

static void
attributes_set_sensitive(UMLClassDialog *prop_dialog, gint val)
{
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_name), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_type), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_value), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_visible_button), val);
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

  gtk_option_menu_set_history(prop_dialog->attr_visible_button,
			      (gint)attr->visibility);
  gtk_toggle_button_set_active(prop_dialog->attr_class_scope, attr->class_scope);
}

static void
attributes_clear_values(UMLClassDialog *prop_dialog)
{
  gtk_entry_set_text(prop_dialog->attr_name, "");
  gtk_entry_set_text(prop_dialog->attr_type, "");
  gtk_entry_set_text(prop_dialog->attr_value, "");
  gtk_toggle_button_set_active(prop_dialog->attr_class_scope, FALSE);
}

static void
attributes_get_values(UMLClassDialog *prop_dialog, UMLAttribute *attr)
{
  char *str;
  
  g_free(attr->name);
  attr->name = g_strdup(gtk_entry_get_text(prop_dialog->attr_name));

  g_free(attr->type);
  attr->type = g_strdup(gtk_entry_get_text(prop_dialog->attr_type));
  
  if (attr->value != NULL)
    g_free(attr->value);
  str = gtk_entry_get_text(prop_dialog->attr_value);
  if (strlen(str)!=0)
    attr->value = g_strdup(str);
  else
    attr->value = NULL;

  attr->visibility = (UMLVisibility)
    GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(gtk_menu_get_active(prop_dialog->attr_visible))));
    
  attr->class_scope = prop_dialog->attr_class_scope->active;
}

static void
attributes_get_current_values(UMLClassDialog *prop_dialog)
{
  UMLAttribute *current_attr;
  GtkLabel *label;
  char *new_str;

  if (prop_dialog->current_attr != NULL) {
    current_attr = (UMLAttribute *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_attr));
    if (current_attr != NULL) {
      attributes_get_values(prop_dialog, current_attr);
      label = GTK_LABEL(GTK_BIN(prop_dialog->current_attr)->child);
      new_str = uml_get_attribute_string(current_attr);
      gtk_label_set_text(label, new_str);
      g_free(new_str);
    }
  }
}

static void
attribute_list_item_destroy_callback(GtkWidget *list_item,
				     gpointer data)
{
  UMLAttribute *attr;

  attr = (UMLAttribute *) gtk_object_get_user_data(GTK_OBJECT(list_item));

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

  prop_dialog = umlclass->properties_dialog;

  attributes_get_current_values(prop_dialog);
  
  list = GTK_LIST(gtklist)->selection;
  if (!list) { /* No selected */
    attributes_set_sensitive(prop_dialog, FALSE);
    attributes_clear_values(prop_dialog);
    prop_dialog->current_attr = NULL;
    return;
  }
  
  list_item = GTK_OBJECT(list->data);
  attr = (UMLAttribute *)gtk_object_get_user_data(list_item);
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
  char *str;

  prop_dialog = umlclass->properties_dialog;

  attributes_get_current_values(prop_dialog);

  attr = uml_attribute_new();

  str = uml_get_attribute_string(attr);
  list_item = gtk_list_item_new_with_label(str);
  gtk_widget_show(list_item);
  g_free(str);

  gtk_object_set_user_data(GTK_OBJECT(list_item), attr);
  gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
		      GTK_SIGNAL_FUNC (attribute_list_item_destroy_callback),
		      NULL);
  
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
      gtk_object_get_user_data(GTK_OBJECT(gtklist->selection->data));

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

    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

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

    
    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }
}

static void
attributes_read_from_dialog(UMLClass *umlclass,
			    UMLClassDialog *prop_dialog,
			    int connection_index)
{
  GList *list;
  UMLAttribute *attr;
  GtkWidget *list_item;
  GList *clear_list;
  Object *obj;

  obj = (Object *) umlclass;

  attributes_get_current_values(prop_dialog); /* if changed, update from widgets */
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
      gtk_object_get_user_data(GTK_OBJECT(list_item));
    gtk_object_set_user_data(GTK_OBJECT(list_item), NULL);
    umlclass->attributes = g_list_append(umlclass->attributes, attr);
    
    if (attr->left_connection == NULL) {
      attr->left_connection = g_new(ConnectionPoint,1);
      attr->left_connection->object = obj;
      attr->left_connection->connected = NULL;
      
      attr->right_connection = g_new(ConnectionPoint,1);
      attr->right_connection->object = obj;
      attr->right_connection->connected = NULL;
    } 

    if ( (prop_dialog->attr_vis->active) &&
	 (!prop_dialog->attr_supp->active) ) { 
      obj->connections[connection_index++] = attr->left_connection;
      obj->connections[connection_index++] = attr->right_connection;
    } else {
      object_remove_connections_to(attr->left_connection);
      object_remove_connections_to(attr->right_connection);
    }

    list = g_list_next(list);
  }
  clear_list = g_list_reverse (clear_list);
  gtk_list_remove_items (GTK_LIST (prop_dialog->attributes_list), clear_list);
  g_list_free (clear_list);
}

static void
attributes_fill_in_dialog(UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLAttribute *attr;
  UMLAttribute *attr_copy;
  GtkWidget *list_item;
  GList *list;
  int i;

  prop_dialog = umlclass->properties_dialog;

  /* copy in new attributes: */
  if (prop_dialog->attributes_list->children == NULL) {
    i = 0;
    list = umlclass->attributes;
    while (list != NULL) {
      attr = (UMLAttribute *)list->data;
      
      list_item =
	gtk_list_item_new_with_label (umlclass->attributes_strings[i]);
      attr_copy = uml_attribute_copy(attr);
      gtk_object_set_user_data(GTK_OBJECT(list_item), (gpointer) attr_copy);
      gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
			  GTK_SIGNAL_FUNC (attribute_list_item_destroy_callback),
			  NULL);
      gtk_container_add (GTK_CONTAINER (prop_dialog->attributes_list), list_item);
      gtk_widget_show (list_item);
      
      list = g_list_next(list); i++;
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
  attributes_get_current_values(umlclass->properties_dialog);
}

static void
attributes_update_event(GtkWidget *widget, GdkEventFocus *ev, UMLClass *umlclass)
{
  attributes_get_current_values(umlclass->properties_dialog);
}

static void 
attributes_create_page(GtkNotebook *notebook,  UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox2;
  GtkWidget *entry;
  GtkWidget *checkbox;
  GtkWidget *scrolled_win;
  GtkWidget *button;
  GtkWidget *list;
  GtkWidget *frame;
  GtkWidget *omenu;
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *menuitem;
  GSList *group;

  prop_dialog = umlclass->properties_dialog;

  /* Attributes page: */
  page_label = gtk_label_new (_("Attributes"));
  
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

  gtk_signal_connect (GTK_OBJECT (list), "selection_changed",
		      GTK_SIGNAL_FUNC(attributes_list_selection_changed_callback),
		      umlclass);

  vbox2 = gtk_vbox_new(FALSE, 5);

  button = gtk_button_new_with_label (_("New"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(attributes_list_new_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_label (_("Delete"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(attributes_list_delete_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_label (_("Move up"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(attributes_list_move_up_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_label (_("Move down"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(attributes_list_move_down_callback),
		      umlclass);
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

  hbox2 = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Name:"));
  entry = gtk_entry_new();
  prop_dialog->attr_name = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (attributes_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (attributes_update), umlclass);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);

  hbox2 = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Type:"));
  entry = gtk_entry_new();
  prop_dialog->attr_type = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (attributes_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (attributes_update), umlclass);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);

  hbox2 = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Value:"));
  entry = gtk_entry_new();
  prop_dialog->attr_value = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (attributes_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (attributes_update), umlclass);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);

  hbox2 = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Visibility:"));

  omenu = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  prop_dialog->attr_visible = GTK_MENU(menu);
  prop_dialog->attr_visible_button = GTK_OPTION_MENU(omenu);
  submenu = NULL;
  group = NULL;
    
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Public"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (attributes_update), umlclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem),
			   GINT_TO_POINTER(UML_PUBLIC) );
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Private"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (attributes_update), umlclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem),
			   GINT_TO_POINTER(UML_PRIVATE) );
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Protected"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (attributes_update), umlclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem),
			   GINT_TO_POINTER(UML_PROTECTED) );
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Implementation"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (attributes_update), umlclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem),
			   GINT_TO_POINTER(UML_IMPLEMENTATION) );
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  
  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);

  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), omenu, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);

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

/*************************************************************
 ******************** OPERATIONS *****************************
 *************************************************************/

/* Forward declaration: */
static void operations_get_current_values(UMLClassDialog *prop_dialog);

static void
parameters_set_sensitive(UMLClassDialog *prop_dialog, gint val)
{
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_name), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_type), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_value), val);
}

static void
parameters_set_values(UMLClassDialog *prop_dialog, UMLParameter *param)
{
  gtk_entry_set_text(prop_dialog->param_name, param->name);
  gtk_entry_set_text(prop_dialog->param_type, param->type);
  if (param->value != NULL)
    gtk_entry_set_text(prop_dialog->param_value, param->value);
  else
    gtk_entry_set_text(prop_dialog->param_value, "");
}

static void
parameters_clear_values(UMLClassDialog *prop_dialog)
{
  gtk_entry_set_text(prop_dialog->param_name, "");
  gtk_entry_set_text(prop_dialog->param_type, "");
  gtk_entry_set_text(prop_dialog->param_value, "");
}

static void
parameters_get_values(UMLClassDialog *prop_dialog, UMLParameter *param)
{
  char *str;
  
  g_free(param->name);
  param->name = g_strdup(gtk_entry_get_text(prop_dialog->param_name));

  g_free(param->type);
  param->type = g_strdup(gtk_entry_get_text(prop_dialog->param_type));

  if (param->value != NULL)
    g_free(param->value);
  
  str = gtk_entry_get_text(prop_dialog->param_value);
  if (strlen(str)!=0)
    param->value = g_strdup(str);
  else
    param->value = NULL;
}

static void
parameters_get_current_values(UMLClassDialog *prop_dialog)
{
  UMLParameter *current_param;
  GtkLabel *label;
  char *new_str;

  if (prop_dialog->current_param != NULL) {
    current_param = (UMLParameter *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_param));
    if (current_param != NULL) {
      parameters_get_values(prop_dialog, current_param);
      label = GTK_LABEL(GTK_BIN(prop_dialog->current_param)->child);
      new_str = uml_get_parameter_string(current_param);
      gtk_label_set_text(label, new_str);
      g_free(new_str);
    }
  }
}


static void
parameters_list_selection_changed_callback(GtkWidget *gtklist,
					   UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkObject *list_item;
  UMLParameter *param;

  prop_dialog = umlclass->properties_dialog;

  parameters_get_current_values(prop_dialog);
  
  list = GTK_LIST(gtklist)->selection;
  if (!list) { /* No selected */
    parameters_set_sensitive(prop_dialog, FALSE);
    parameters_clear_values(prop_dialog);
    prop_dialog->current_param = NULL;
    return;
  }
  
  list_item = GTK_OBJECT(list->data);
  param = (UMLParameter *)gtk_object_get_user_data(list_item);
  parameters_set_values(prop_dialog, param);
  parameters_set_sensitive(prop_dialog, TRUE);

  prop_dialog->current_param = GTK_LIST_ITEM(list_item);
  gtk_widget_grab_focus(GTK_WIDGET(prop_dialog->param_name));
}

static void
parameters_list_new_callback(GtkWidget *button,
			     UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkWidget *list_item;
  UMLOperation *current_op;
  UMLParameter *param;
  char *str;

  prop_dialog = umlclass->properties_dialog;

  parameters_get_current_values(prop_dialog);

  current_op = (UMLOperation *)
    gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));
  
  param = uml_parameter_new();

  str = uml_get_parameter_string(param);
  list_item = gtk_list_item_new_with_label(str);
  gtk_widget_show(list_item);
  g_free(str);

  gtk_object_set_user_data(GTK_OBJECT(list_item), param);

  current_op->parameters = g_list_append(current_op->parameters,
					 (gpointer) param);
  
  list = g_list_append(NULL, list_item);
  gtk_list_append_items(prop_dialog->parameters_list, list);

  if (prop_dialog->parameters_list->children != NULL)
    gtk_list_unselect_child(prop_dialog->parameters_list,
			    GTK_WIDGET(prop_dialog->parameters_list->children->data));
  gtk_list_select_child(prop_dialog->parameters_list, list_item);

  prop_dialog->current_param = GTK_LIST_ITEM(list_item);
}

static void
parameters_list_delete_callback(GtkWidget *button,
				UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkList *gtklist;
  UMLOperation *current_op;
  UMLParameter *param;
  
  prop_dialog = umlclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->parameters_list);


  if (gtklist->selection != NULL) {
    /* Remove from current operations parameter list: */
    current_op = (UMLOperation *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));
    param = (UMLParameter *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_param));
    
    current_op->parameters = g_list_remove(current_op->parameters,
					   (gpointer) param);
    uml_parameter_destroy(param);
    
    /* Remove from gtk list: */
    list = g_list_prepend(NULL, prop_dialog->current_param);

    prop_dialog->current_param = NULL;

    gtk_list_remove_items(gtklist, list);
    g_list_free(list);
  }
}

static void
parameters_list_move_up_callback(GtkWidget *button,
				 UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  UMLOperation *current_op;
  UMLParameter *param;
  int i;
  
  prop_dialog = umlclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->parameters_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);
    
    i = gtk_list_child_position(gtklist, list_item);
    if (i>0)
      i--;

    param = (UMLParameter *) gtk_object_get_user_data(GTK_OBJECT(list_item));

    /* Move parameter in current operations list: */
    current_op = (UMLOperation *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));
    
    current_op->parameters = g_list_remove(current_op->parameters,
					   (gpointer) param);
    current_op->parameters = g_list_insert(current_op->parameters,
					   (gpointer) param,
					   i);

    /* Move parameter in gtk list: */
    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

    gtk_list_select_child(gtklist, list_item);

    operations_get_current_values(prop_dialog);
  }
}

static void
parameters_list_move_down_callback(GtkWidget *button,
				   UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  UMLOperation *current_op;
  UMLParameter *param;
  int i;
  
  prop_dialog = umlclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->parameters_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);
    
    i = gtk_list_child_position(gtklist, list_item);
    if (i<(g_list_length(gtklist->children)-1))
      i++;

    param = (UMLParameter *) gtk_object_get_user_data(GTK_OBJECT(list_item));

    /* Move parameter in current operations list: */
    current_op = (UMLOperation *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));
    
    current_op->parameters = g_list_remove(current_op->parameters,
					   (gpointer) param);
    current_op->parameters = g_list_insert(current_op->parameters,
					   (gpointer) param,
					   i);

    /* Move parameter in gtk list: */
    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

    gtk_list_select_child(gtklist, list_item);

    operations_get_current_values(prop_dialog);
  }
}

static void
operations_set_sensitive(UMLClassDialog *prop_dialog, gint val)
{
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_name), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_type), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_visible_button), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_visible), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_class_scope), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_abstract), val);

  gtk_widget_set_sensitive(prop_dialog->param_new_button, val);
  gtk_widget_set_sensitive(prop_dialog->param_delete_button, val);
  gtk_widget_set_sensitive(prop_dialog->param_down_button, val);
  gtk_widget_set_sensitive(prop_dialog->param_up_button, val);
}

static void
operations_set_values(UMLClassDialog *prop_dialog, UMLOperation *op)
{
  GList *list;
  UMLParameter *param;
  GtkWidget *list_item;
  char *str;
  
  gtk_entry_set_text(prop_dialog->op_name, op->name);
  if (op->type != NULL)
    gtk_entry_set_text(prop_dialog->op_type, op->type);
  else
    gtk_entry_set_text(prop_dialog->op_type, "");

  gtk_option_menu_set_history(prop_dialog->op_visible_button,
			      (gint)op->visibility);
  gtk_toggle_button_set_active(prop_dialog->op_class_scope, op->class_scope);
  gtk_toggle_button_set_active(prop_dialog->op_abstract, op->abstract);

  gtk_list_clear_items(prop_dialog->parameters_list, 0, -1);
  prop_dialog->current_param = NULL;
  parameters_set_sensitive(prop_dialog, FALSE);

  list = op->parameters;
  while (list != NULL) {
    param = (UMLParameter *)list->data;

    str = uml_get_parameter_string(param);
    list_item = gtk_list_item_new_with_label (str);
    g_free(str);
    gtk_object_set_user_data(GTK_OBJECT(list_item), (gpointer) param);
    gtk_container_add (GTK_CONTAINER (prop_dialog->parameters_list), list_item);
    gtk_widget_show (list_item);
    
    list = g_list_next(list);
  }
}

static void
operations_clear_values(UMLClassDialog *prop_dialog)
{
  gtk_entry_set_text(prop_dialog->op_name, "");
  gtk_entry_set_text(prop_dialog->op_type, "");
  gtk_toggle_button_set_active(prop_dialog->op_class_scope, FALSE);
  gtk_toggle_button_set_active(prop_dialog->op_abstract, FALSE);

  gtk_list_clear_items(prop_dialog->parameters_list, 0, -1);
  prop_dialog->current_param = NULL;
  parameters_set_sensitive(prop_dialog, FALSE);
}


static void
operations_get_values(UMLClassDialog *prop_dialog, UMLOperation *op)
{
  char *str;
  
  g_free(op->name);
  op->name = g_strdup(gtk_entry_get_text(prop_dialog->op_name));

  if (op->type != NULL)
    g_free(op->type);
  str = gtk_entry_get_text(prop_dialog->op_type);
  if (strlen(str)!=0)
    op->type = g_strdup(str);
  else
    op->type = NULL;

  op->visibility = (UMLVisibility)
    GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(gtk_menu_get_active(prop_dialog->op_visible))));
    
  op->class_scope = prop_dialog->op_class_scope->active;
  op->abstract = prop_dialog->op_abstract->active;
}

static void
operations_get_current_values(UMLClassDialog *prop_dialog)
{
  UMLOperation *current_op;
  GtkLabel *label;
  char *new_str;

  parameters_get_current_values(prop_dialog);

  if (prop_dialog->current_op != NULL) {
    current_op = (UMLOperation *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));
    if (current_op != NULL) {
      operations_get_values(prop_dialog, current_op);
      label = GTK_LABEL(GTK_BIN(prop_dialog->current_op)->child);
      new_str = uml_get_operation_string(current_op);
      gtk_label_set_text(label, new_str);
      g_free(new_str);
    }
  }
}

static void
operations_list_item_destroy_callback(GtkWidget *list_item,
				      gpointer data)
{
  UMLOperation *op;

  op = (UMLOperation *) gtk_object_get_user_data(GTK_OBJECT(list_item));

  if (op != NULL) {
    uml_operation_destroy(op);
    /*printf("Destroying operation list_item's user_data!\n");*/
  }
}

static void
operations_list_selection_changed_callback(GtkWidget *gtklist,
					   UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkObject *list_item;
  UMLOperation *op;

  prop_dialog = umlclass->properties_dialog;

  operations_get_current_values(prop_dialog);
  
  list = GTK_LIST(gtklist)->selection;
  if (!list) { /* No selected */
    operations_set_sensitive(prop_dialog, FALSE);
    operations_clear_values(prop_dialog);
    prop_dialog->current_op = NULL;
    return;
  }
  
  list_item = GTK_OBJECT(list->data);
  op = (UMLOperation *)gtk_object_get_user_data(list_item);
  operations_set_values(prop_dialog, op);
  operations_set_sensitive(prop_dialog, TRUE);

  prop_dialog->current_op = GTK_LIST_ITEM(list_item);
  gtk_widget_grab_focus(GTK_WIDGET(prop_dialog->op_name));
}

static void
operations_list_new_callback(GtkWidget *button,
			     UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkWidget *list_item;
  UMLOperation *op;
  char *str;

  prop_dialog = umlclass->properties_dialog;

  operations_get_current_values(prop_dialog);

  op = uml_operation_new();

  str = uml_get_operation_string(op);
  list_item = gtk_list_item_new_with_label(str);
  gtk_widget_show(list_item);
  g_free(str);

  gtk_object_set_user_data(GTK_OBJECT(list_item), op);
  gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
		      GTK_SIGNAL_FUNC (operations_list_item_destroy_callback),
		      NULL);
  
  list = g_list_append(NULL, list_item);
  gtk_list_append_items(prop_dialog->operations_list, list);

  if (prop_dialog->operations_list->children != NULL)
    gtk_list_unselect_child(prop_dialog->operations_list,
			    GTK_WIDGET(prop_dialog->operations_list->children->data));
  gtk_list_select_child(prop_dialog->operations_list, list_item);
}

static void
operations_list_delete_callback(GtkWidget *button,
				UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkList *gtklist;
  UMLOperation *op;
  

  prop_dialog = umlclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->operations_list);

  if (gtklist->selection != NULL) {
    op = (UMLOperation *)
      gtk_object_get_user_data(GTK_OBJECT(gtklist->selection->data));

    if (op->left_connection != NULL) {
      prop_dialog->deleted_connections =
	g_list_prepend(prop_dialog->deleted_connections,
		       op->left_connection);
      prop_dialog->deleted_connections =
	g_list_prepend(prop_dialog->deleted_connections,
		       op->right_connection);
    }

    list = g_list_prepend(NULL, gtklist->selection->data);
    gtk_list_remove_items(gtklist, list);
    g_list_free(list);
    operations_clear_values(prop_dialog);
    operations_set_sensitive(prop_dialog, FALSE);
  }
}

static void
operations_list_move_up_callback(GtkWidget *button,
				 UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  int i;
  
  prop_dialog = umlclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->operations_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);
    
    i = gtk_list_child_position(gtklist, list_item);
    if (i>0)
      i--;

    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }

}

static void
operations_list_move_down_callback(GtkWidget *button,
				   UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  int i;
  
  prop_dialog = umlclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->operations_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);
    
    i = gtk_list_child_position(gtklist, list_item);
    if (i<(g_list_length(gtklist->children)-1))
      i++;

    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }
}

static void
operations_read_from_dialog(UMLClass *umlclass,
			    UMLClassDialog *prop_dialog,
			    int connection_index)
{
  GList *list;
  UMLOperation *op;
  GtkWidget *list_item;
  GList *clear_list;
  Object *obj;

  obj = (Object *) umlclass;

  operations_get_current_values(prop_dialog); /* if changed, update from widgets */
  /* Free current operations: */
  list = umlclass->operations;
  while (list != NULL) {
    op = (UMLOperation *)list->data;
    uml_operation_destroy(op);
    list = g_list_next(list);
  }
  g_list_free (umlclass->operations);
  umlclass->operations = NULL;

  /* Insert new operations and remove them from gtklist: */
  list = GTK_LIST (prop_dialog->operations_list)->children;
  clear_list = NULL;
  while (list != NULL) {
    list_item = GTK_WIDGET(list->data);
    
    clear_list = g_list_prepend (clear_list, list_item);
    op = (UMLOperation *)
      gtk_object_get_user_data(GTK_OBJECT(list_item));
    gtk_object_set_user_data(GTK_OBJECT(list_item), NULL);
    umlclass->operations = g_list_append(umlclass->operations, op);
    
    if (op->left_connection == NULL) {
      op->left_connection = g_new(ConnectionPoint,1);
      op->left_connection->object = obj;
      op->left_connection->connected = NULL;
      
      op->right_connection = g_new(ConnectionPoint,1);
      op->right_connection->object = obj;
      op->right_connection->connected = NULL;
    }
    
    if ( (prop_dialog->op_vis->active) &&
	 (!prop_dialog->op_supp->active) ) { 
      obj->connections[connection_index++] = op->left_connection;
      obj->connections[connection_index++] = op->right_connection;
    } else {
      object_remove_connections_to(op->left_connection);
      object_remove_connections_to(op->right_connection);
    }
    
    list = g_list_next(list);
  }
  clear_list = g_list_reverse (clear_list);
  gtk_list_remove_items (GTK_LIST (prop_dialog->operations_list), clear_list);
  g_list_free (clear_list);
}

static void
operations_fill_in_dialog(UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *op;
  UMLOperation *op_copy;
  GtkWidget *list_item;
  GList *list;
  int i;

  prop_dialog = umlclass->properties_dialog;

  if (prop_dialog->operations_list->children == NULL) {
    i = 0;
    list = umlclass->operations;
    while (list != NULL) {
      op = (UMLOperation *)list->data;
      
      list_item =
	gtk_list_item_new_with_label (umlclass->operations_strings[i]);
      op_copy = uml_operation_copy(op);
      gtk_object_set_user_data(GTK_OBJECT(list_item), (gpointer) op_copy);
      gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
			  GTK_SIGNAL_FUNC (operations_list_item_destroy_callback),
			  NULL);
      gtk_container_add (GTK_CONTAINER (prop_dialog->operations_list), list_item);
      gtk_widget_show (list_item);
      
      list = g_list_next(list); i++;
    }

    /* set operations non-sensitive */
    prop_dialog->current_op = NULL;
    operations_set_sensitive(prop_dialog, FALSE);
    operations_clear_values(prop_dialog);
  }
}

static void
operations_update(GtkWidget *widget, UMLClass *umlclass)
{
  operations_get_current_values(umlclass->properties_dialog);
}

static void
operations_update_event(GtkWidget *widget, GdkEventFocus *ev, UMLClass *umlclass)
{
  operations_get_current_values(umlclass->properties_dialog);
}

static void 
operations_create_page(GtkNotebook *notebook,  UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox2;
  GtkWidget *vbox3;
  GtkWidget *entry;
  GtkWidget *checkbox;
  GtkWidget *scrolled_win;
  GtkWidget *button;
  GtkWidget *list;
  GtkWidget *frame;
  GtkWidget *omenu;
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *menuitem;
  GSList *group;

  prop_dialog = umlclass->properties_dialog;

  /* Operations page: */
  page_label = gtk_label_new (_("Operations"));
  
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
  prop_dialog->operations_list = GTK_LIST(list);
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  gtk_widget_show (list);

  gtk_signal_connect (GTK_OBJECT (list), "selection_changed",
		      GTK_SIGNAL_FUNC(operations_list_selection_changed_callback),
		      umlclass);

  vbox2 = gtk_vbox_new(FALSE, 5);

  button = gtk_button_new_with_label (_("New"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(operations_list_new_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_label (_("Delete"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(operations_list_delete_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_label (_("Move up"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(operations_list_move_up_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_label (_("Move down"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(operations_list_move_down_callback),
		      umlclass);

  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  frame = gtk_frame_new(_("Operation data"));
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show(frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

  vbox2 = gtk_vbox_new(FALSE, 5);

  hbox2 = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Name:"));
  entry = gtk_entry_new();
  prop_dialog->op_name = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, TRUE, 0);

  hbox2 = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Type:"));
  entry = gtk_entry_new();
  prop_dialog->op_type = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, TRUE, 0);

  hbox2 = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Visibility:"));

  omenu = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  prop_dialog->op_visible = GTK_MENU(menu);
  prop_dialog->op_visible_button = GTK_OPTION_MENU(omenu);
  submenu = NULL;
  group = NULL;
    
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Public"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem),
			   GINT_TO_POINTER(UML_PUBLIC) );
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Private"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem),
			   GINT_TO_POINTER(UML_PRIVATE) );
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Protected"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem),
			   GINT_TO_POINTER(UML_PROTECTED) );
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Implementation"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem),
			   GINT_TO_POINTER(UML_IMPLEMENTATION) );
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  
  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);

  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), omenu, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, TRUE, 0);

  hbox2 = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Class scope"));
  prop_dialog->op_class_scope = GTK_TOGGLE_BUTTON(checkbox);
  gtk_box_pack_start (GTK_BOX (hbox2), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, TRUE, 0);

  hbox2 = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("abstract"));
  prop_dialog->op_abstract = GTK_TOGGLE_BUTTON(checkbox);
  gtk_box_pack_start (GTK_BOX (hbox2), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, TRUE, 0);

  
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);

  vbox2 = gtk_vbox_new(FALSE, 5);

  hbox2 = gtk_hbox_new(FALSE, 5);

  label = gtk_label_new(_("Parameters:"));
  gtk_box_pack_start( GTK_BOX(hbox2), label, FALSE, TRUE, 0);
  
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);
  
  
  hbox2 = gtk_hbox_new(FALSE, 5);
  
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox2), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  list = gtk_list_new ();
  prop_dialog->parameters_list = GTK_LIST(list);
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  gtk_widget_show (list);

  gtk_signal_connect (GTK_OBJECT (list), "selection_changed",
		      GTK_SIGNAL_FUNC(parameters_list_selection_changed_callback),
		      umlclass);

  vbox3 = gtk_vbox_new(FALSE, 5);

  button = gtk_button_new_with_label (_("New"));
  prop_dialog->param_new_button = button;
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(parameters_list_new_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_label (_("Delete"));
  prop_dialog->param_delete_button = button;
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(parameters_list_delete_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_label (_("Move up"));
  prop_dialog->param_up_button = button;
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(parameters_list_move_up_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_label (_("Move down"));
  prop_dialog->param_down_button = button;
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(parameters_list_move_down_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (hbox2), vbox3, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);

  frame = gtk_frame_new(_("Parameter data"));
  vbox3 = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox3), 10);
  gtk_container_add (GTK_CONTAINER (frame), vbox3);
  gtk_widget_show(frame);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, TRUE, 0);
  

  hbox2 = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Name:"));
  entry = gtk_entry_new();
  prop_dialog->param_name = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox2, TRUE, TRUE, 0);

  hbox2 = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Type:"));
  entry = gtk_entry_new();
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  prop_dialog->param_type = GTK_ENTRY(entry);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox2, TRUE, TRUE, 0);

  hbox2 = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Def. value:"));
  entry = gtk_entry_new();
  prop_dialog->param_value = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox2, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
 
  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page (notebook, vbox, page_label);

}

/************************************************************
 ******************** TEMPLATES *****************************
 ************************************************************/

static void
templates_set_sensitive(UMLClassDialog *prop_dialog, gint val)
{
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->templ_name), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->templ_type), val);
}

static void
templates_set_values(UMLClassDialog *prop_dialog,
		     UMLFormalParameter *param)
{
  gtk_entry_set_text(prop_dialog->templ_name, param->name);
  if (param->type != NULL)
    gtk_entry_set_text(prop_dialog->templ_type, param->type);
  else
    gtk_entry_set_text(prop_dialog->templ_type, "");
}

static void
templates_clear_values(UMLClassDialog *prop_dialog)
{
  gtk_entry_set_text(prop_dialog->templ_name, "");
  gtk_entry_set_text(prop_dialog->templ_type, "");
}

static void
templates_get_values(UMLClassDialog *prop_dialog, UMLFormalParameter *param)
{
  char *str;
  
  g_free(param->name);
  param->name = g_strdup(gtk_entry_get_text(prop_dialog->templ_name));

  if (param->type != NULL)
    g_free(param->type);
  str = gtk_entry_get_text(prop_dialog->templ_type);
  if (strlen(str)!=0)
    param->type = g_strdup(str);
  else
    param->type = NULL;
}


static void
templates_get_current_values(UMLClassDialog *prop_dialog)
{
  UMLFormalParameter *current_param;
  GtkLabel *label;
  char *new_str;

  if (prop_dialog->current_templ != NULL) {
    current_param = (UMLFormalParameter *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_templ));
    if (current_param != NULL) {
      templates_get_values(prop_dialog, current_param);
      label = GTK_LABEL(GTK_BIN(prop_dialog->current_templ)->child);
      new_str = uml_get_formalparameter_string(current_param);
      gtk_label_set_text(label, new_str);
      g_free(new_str);
    }
  }
}

static void
templates_list_item_destroy_callback(GtkWidget *list_item,
				     gpointer data)
{
  UMLFormalParameter *param;

  param = (UMLFormalParameter *)
    gtk_object_get_user_data(GTK_OBJECT(list_item));

  if (param != NULL) {
    uml_formalparameter_destroy(param);
    /*printf("Destroying list_item's user_data!\n"); */
  }
}

static void
templates_list_selection_changed_callback(GtkWidget *gtklist,
					  UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkObject *list_item;
  UMLFormalParameter *param;

  prop_dialog = umlclass->properties_dialog;

  templates_get_current_values(prop_dialog);
  
  list = GTK_LIST(gtklist)->selection;
  if (!list) { /* No selected */
    templates_set_sensitive(prop_dialog, FALSE);
    templates_clear_values(prop_dialog);
    prop_dialog->current_templ = NULL;
    return;
  }
  
  list_item = GTK_OBJECT(list->data);
  param = (UMLFormalParameter *)gtk_object_get_user_data(list_item);
  templates_set_values(prop_dialog, param);
  templates_set_sensitive(prop_dialog, TRUE);

  prop_dialog->current_templ = GTK_LIST_ITEM(list_item);
  gtk_widget_grab_focus(GTK_WIDGET(prop_dialog->templ_name));
}

static void
templates_list_new_callback(GtkWidget *button,
			    UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkWidget *list_item;
  UMLFormalParameter *param;
  char *str;

  prop_dialog = umlclass->properties_dialog;

  templates_get_current_values(prop_dialog);

  param = uml_formalparameter_new();

  str = uml_get_formalparameter_string(param);
  list_item = gtk_list_item_new_with_label(str);
  gtk_widget_show(list_item);
  g_free(str);

  gtk_object_set_user_data(GTK_OBJECT(list_item), param);
  gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
		      GTK_SIGNAL_FUNC (templates_list_item_destroy_callback),
		      NULL);
  
  list = g_list_append(NULL, list_item);
  gtk_list_append_items(prop_dialog->templates_list, list);

  if (prop_dialog->templates_list->children != NULL)
    gtk_list_unselect_child(prop_dialog->templates_list,
			    GTK_WIDGET(prop_dialog->templates_list->children->data));
  gtk_list_select_child(prop_dialog->templates_list, list_item);
}

static void
templates_list_delete_callback(GtkWidget *button,
			       UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkList *gtklist;

  prop_dialog = umlclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->templates_list);

  if (gtklist->selection != NULL) {
    list = g_list_prepend(NULL, gtklist->selection->data);
    gtk_list_remove_items(gtklist, list);
    g_list_free(list);
    templates_clear_values(prop_dialog);
    templates_set_sensitive(prop_dialog, FALSE);
  }
}

static void
templates_list_move_up_callback(GtkWidget *button,
				UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  int i;
  
  prop_dialog = umlclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->templates_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);
    
    i = gtk_list_child_position(gtklist, list_item);
    if (i>0)
      i--;

    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }
}

static void
templates_list_move_down_callback(GtkWidget *button,
				   UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  int i;

  prop_dialog = umlclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->templates_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);
    
    i = gtk_list_child_position(gtklist, list_item);
    if (i<(g_list_length(gtklist->children)-1))
      i++;

    gtk_widget_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    gtk_widget_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }
}


static void
templates_read_from_dialog(UMLClass *umlclass, UMLClassDialog *prop_dialog)
{
  GList *list;
  UMLFormalParameter *param;
  GtkWidget *list_item;
  GList *clear_list;

  templates_get_current_values(prop_dialog); /* if changed, update from widgets */

  umlclass->template = prop_dialog->templ_template->active;

  /* Free current formal parameters: */
  list = umlclass->formal_params;
  while (list != NULL) {
    param = (UMLFormalParameter *)list->data;
    uml_formalparameter_destroy(param);
    list = g_list_next(list);
  }
  g_list_free (umlclass->formal_params);
  umlclass->formal_params = NULL;

  /* Insert new formal params and remove them from gtklist: */
  list = GTK_LIST (prop_dialog->templates_list)->children;
  clear_list = NULL;
  while (list != NULL) {
    list_item = GTK_WIDGET(list->data);
    clear_list = g_list_prepend (clear_list, list_item);
    param = (UMLFormalParameter *)
      gtk_object_get_user_data(GTK_OBJECT(list_item));
    gtk_object_set_user_data(GTK_OBJECT(list_item), NULL);
    umlclass->formal_params = g_list_append(umlclass->formal_params, param);
    list = g_list_next(list);
  }
  clear_list = g_list_reverse (clear_list);
  gtk_list_remove_items (GTK_LIST (prop_dialog->templates_list), clear_list);
  g_list_free (clear_list);
}

static void
templates_fill_in_dialog(UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLFormalParameter *param, *param_copy;
  GList *list;
  GtkWidget *list_item;
  int i;

  prop_dialog = umlclass->properties_dialog;

  gtk_toggle_button_set_active(prop_dialog->templ_template, umlclass->template);

  /* copy in new template-parameters: */
  if (prop_dialog->templates_list->children == NULL) {
    i = 0;
    list = umlclass->formal_params;
    while (list != NULL) {
      param = (UMLFormalParameter *)list->data;
      
      list_item =
	gtk_list_item_new_with_label (umlclass->templates_strings[i]);
      param_copy = uml_formalparameter_copy(param);
      gtk_object_set_user_data(GTK_OBJECT(list_item),
			       (gpointer) param_copy);
      gtk_signal_connect (GTK_OBJECT (list_item), "destroy",
			  GTK_SIGNAL_FUNC (templates_list_item_destroy_callback),
			  NULL);
      gtk_container_add (GTK_CONTAINER (prop_dialog->templates_list),
			 list_item);
      gtk_widget_show (list_item);
      
      list = g_list_next(list); i++;
    }
    /* set templates non-sensitive */
    prop_dialog->current_templ = NULL;
    templates_set_sensitive(prop_dialog, FALSE);
    templates_clear_values(prop_dialog);
  }

}


static void
templates_update(GtkWidget *widget, UMLClass *umlclass)
{
  templates_get_current_values(umlclass->properties_dialog);
}

static void
templates_update_event(GtkWidget *widget, GdkEventFocus *ev, UMLClass *umlclass)
{
  templates_get_current_values(umlclass->properties_dialog);
}

static void 
templates_create_page(GtkNotebook *notebook,  UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox2;
  GtkWidget *entry;
  GtkWidget *checkbox;
  GtkWidget *scrolled_win;
  GtkWidget *button;
  GtkWidget *list;
  GtkWidget *frame;
  
  prop_dialog = umlclass->properties_dialog;

  /* Templates page: */
  page_label = gtk_label_new (_("Templates"));
  
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

  hbox2 = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Template class"));
  prop_dialog->templ_template = GTK_TOGGLE_BUTTON(checkbox);
  gtk_box_pack_start (GTK_BOX (hbox2), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, TRUE, 0);
  
  hbox = gtk_hbox_new(FALSE, 5);
  
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  list = gtk_list_new ();
  prop_dialog->templates_list = GTK_LIST(list);
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  gtk_widget_show (list);

  gtk_signal_connect (GTK_OBJECT (list), "selection_changed",
		      GTK_SIGNAL_FUNC(templates_list_selection_changed_callback),
		      umlclass);

  vbox2 = gtk_vbox_new(FALSE, 5);

  button = gtk_button_new_with_label (_("New"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(templates_list_new_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_label (_("Delete"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(templates_list_delete_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_label (_("Move up"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(templates_list_move_up_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_label (_("Move down"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(templates_list_move_down_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  frame = gtk_frame_new(_("Formal parameter data"));
  vbox2 = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 10);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show(frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

  hbox2 = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Name:"));
  entry = gtk_entry_new();
  prop_dialog->templ_name = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (templates_update_event), umlclass); 
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (templates_update), umlclass); 
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);

  hbox2 = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Type:"));
  entry = gtk_entry_new();
  prop_dialog->templ_type = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (templates_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (templates_update), umlclass);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);

  gtk_widget_show(vbox2);
  
  /* TODO: Add stuff here! */
  
  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page (notebook, vbox, page_label);
}



/******************************************************
 ******************** ALL *****************************
 ******************************************************/

static void
switch_page_callback(GtkNotebook *notebook,
		     GtkNotebookPage *page)
{
  UMLClass *umlclass;
  UMLClassDialog *prop_dialog;

  umlclass = (UMLClass *)
    gtk_object_get_user_data(GTK_OBJECT(notebook));

  prop_dialog = umlclass->properties_dialog;

  if (prop_dialog != NULL) {
    attributes_get_current_values(prop_dialog);
    operations_get_current_values(prop_dialog);
    templates_get_current_values(prop_dialog);
  }
}

static void
fill_in_dialog(UMLClass *umlclass)
{
  class_fill_in_dialog(umlclass);
  attributes_fill_in_dialog(umlclass);
  operations_fill_in_dialog(umlclass);
  templates_fill_in_dialog(umlclass);
}

ObjectChange *
umlclass_apply_properties(UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  Object *obj;
  int num_attrib, num_ops;
  GList *list;
  
  prop_dialog = umlclass->properties_dialog;

  /* Allocate enought connection points for attributes and operations. */
  /* (two per op/attr) */
  if ( (prop_dialog->attr_vis->active) && (!prop_dialog->attr_supp->active))
    num_attrib = g_list_length(prop_dialog->attributes_list->children);
  else
    num_attrib = 0;
  if ( (prop_dialog->op_vis->active) && (!prop_dialog->op_supp->active))
    num_ops = g_list_length(prop_dialog->operations_list->children);
  else
    num_ops = 0;
  obj = (Object *) umlclass;
  obj->num_connections = 8 + num_attrib*2 + num_ops*2;
  obj->connections =
    g_realloc(obj->connections,
	      obj->num_connections*sizeof(ConnectionPoint *));
  
  /* Read from dialog and put in object: */
  class_read_from_dialog(umlclass, prop_dialog);
  attributes_read_from_dialog(umlclass, prop_dialog, 8);
  /* ^^^ attribs must be called before ops, to get the right order of the
     connectionpoints. */
  operations_read_from_dialog(umlclass, prop_dialog, 8+num_attrib*2);
  templates_read_from_dialog(umlclass, prop_dialog);

  /* Delete and unconnect from all deleted connectionpoints. */
  list = prop_dialog->deleted_connections;
  while (list != NULL) {
    ConnectionPoint *connection = (ConnectionPoint *) list->data;

    object_remove_connections_to(connection);
    g_free(connection);
    
    list = g_list_next(list);
  }
  g_list_free(prop_dialog->deleted_connections);
  prop_dialog->deleted_connections = NULL;
    
  /* Update data: */
  umlclass_calculate_data(umlclass);
  umlclass_update_data(umlclass);

  /* Fill in class with the new data: */
  fill_in_dialog(umlclass);
  return (ObjectChange *)NULL; /* Temporary. */
}

static void
create_dialog_pages(GtkNotebook *notebook, UMLClass *umlclass)
{
  class_create_page(notebook, umlclass);
  attributes_create_page(notebook, umlclass);
  operations_create_page(notebook, umlclass);
  templates_create_page(notebook, umlclass);
}

GtkWidget *
umlclass_get_properties(UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *vbox;
  GtkWidget *notebook;

  if (umlclass->properties_dialog == NULL) {
    prop_dialog = g_new(UMLClassDialog, 1);
    umlclass->properties_dialog = prop_dialog;

    vbox = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = vbox;

    prop_dialog->current_attr = NULL;
    prop_dialog->current_op = NULL;
    prop_dialog->current_param = NULL;
    prop_dialog->current_templ = NULL;
    prop_dialog->deleted_connections = NULL;
    
    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_box_pack_start (GTK_BOX (vbox),	notebook, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (notebook), 10);

    gtk_object_set_user_data(GTK_OBJECT(notebook), (gpointer) umlclass);
    
    gtk_signal_connect (GTK_OBJECT (notebook),
			"switch_page",
			GTK_SIGNAL_FUNC(switch_page_callback),
			(gpointer) umlclass);
    
    create_dialog_pages(GTK_NOTEBOOK( notebook ), umlclass);

    gtk_widget_show (notebook);
  }

  fill_in_dialog(umlclass);
  gtk_widget_show (umlclass->properties_dialog->dialog);

  return umlclass->properties_dialog->dialog;
}


