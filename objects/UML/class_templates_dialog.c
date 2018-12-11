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
 ******************** TEMPLATES *****************************
 ************************************************************/

static void
templates_set_sensitive(UMLClassDialog *prop_dialog, gint val)
{
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->templ_name), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->templ_type), val);
}

static void
templates_set_values (UMLClassDialog *prop_dialog,
		      UMLFormalParameter *param)
{
  if (param->name)
    gtk_entry_set_text(prop_dialog->templ_name, param->name);
  if (param->type != NULL)
    gtk_entry_set_text(prop_dialog->templ_type, param->type);
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
  g_free(param->name);
  if (param->type != NULL)
    g_free(param->type);

  param->name = g_strdup (gtk_entry_get_text (prop_dialog->templ_name));
  param->type = g_strdup (gtk_entry_get_text (prop_dialog->templ_type));
}


void
_templates_get_current_values(UMLClassDialog *prop_dialog)
{
  UMLFormalParameter *current_param;
  GtkLabel *label;
  gchar* new_str;

  if (prop_dialog->current_templ != NULL) {
    current_param = (UMLFormalParameter *)
      g_object_get_data(G_OBJECT(prop_dialog->current_templ), "user_data");
    if (current_param != NULL) {
      templates_get_values(prop_dialog, current_param);
      label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(prop_dialog->current_templ)));
      new_str = uml_get_formalparameter_string (current_param);
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
    g_object_get_data(G_OBJECT(list_item), "user_data");

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
  GObject *list_item;
  UMLFormalParameter *param;

  prop_dialog = umlclass->properties_dialog;

  if (!prop_dialog)
    return; /* maybe hiding a bug elsewhere */

  _templates_get_current_values(prop_dialog);
  
  list = GTK_LIST(gtklist)->selection;
  if (!list) { /* No selected */
    templates_set_sensitive(prop_dialog, FALSE);
    templates_clear_values(prop_dialog);
    prop_dialog->current_templ = NULL;
    return;
  }
  
  list_item = G_OBJECT(list->data);
  param = (UMLFormalParameter *)g_object_get_data(G_OBJECT(list_item), "user_data");
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
  char *utfstr;

  prop_dialog = umlclass->properties_dialog;

  _templates_get_current_values(prop_dialog);

  param = uml_formalparameter_new();

  utfstr = uml_get_formalparameter_string (param);
  list_item = gtk_list_item_new_with_label (utfstr);
  gtk_widget_show (list_item);
  g_free (utfstr);

  g_object_set_data(G_OBJECT(list_item), "user_data", param);
  g_signal_connect (G_OBJECT (list_item), "destroy",
		    G_CALLBACK (templates_list_item_destroy_callback), NULL);
  
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

    g_object_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    g_object_unref(list_item);

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

    g_object_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    g_object_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }
}


void
_templates_read_from_dialog(UMLClass *umlclass, UMLClassDialog *prop_dialog)
{
  GList *list;
  UMLFormalParameter *param;
  GtkWidget *list_item;
  GList *clear_list;

  _templates_get_current_values(prop_dialog); /* if changed, update from widgets */

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
      g_object_get_data(G_OBJECT(list_item), "user_data");
    g_object_set_data(G_OBJECT(list_item), "user_data", NULL);
    umlclass->formal_params = g_list_append(umlclass->formal_params, param);
    list = g_list_next(list);
  }
  clear_list = g_list_reverse (clear_list);
  gtk_list_remove_items (GTK_LIST (prop_dialog->templates_list), clear_list);
  g_list_free (clear_list);
}

void
_templates_fill_in_dialog(UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLFormalParameter *param_copy;
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
      UMLFormalParameter *param = (UMLFormalParameter *)list->data;
      gchar *paramstr = uml_get_formalparameter_string(param);

      list_item = gtk_list_item_new_with_label (paramstr);
      param_copy = uml_formalparameter_copy(param);
      g_object_set_data(G_OBJECT(list_item), "user_data", 
			       (gpointer) param_copy);
      g_signal_connect (G_OBJECT (list_item), "destroy",
			G_CALLBACK (templates_list_item_destroy_callback), NULL);
      gtk_container_add (GTK_CONTAINER (prop_dialog->templates_list),
			 list_item);
      gtk_widget_show (list_item);
      
      list = g_list_next(list); i++;
      g_free (paramstr);
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
  _templates_get_current_values(umlclass->properties_dialog);
}

static int
templates_update_event(GtkWidget *widget, GdkEventFocus *ev, UMLClass *umlclass)
{
  _templates_get_current_values(umlclass->properties_dialog);
  return 0;
}

void 
_templates_create_page(GtkNotebook *notebook,  UMLClass *umlclass)
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
  
  prop_dialog = umlclass->properties_dialog;

  /* Templates page: */
  page_label = gtk_label_new_with_mnemonic (_("_Templates"));
  
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

  g_signal_connect (G_OBJECT (list), "selection_changed",
		    G_CALLBACK(templates_list_selection_changed_callback), umlclass);

  vbox2 = gtk_vbox_new(FALSE, 5);

  button = gtk_button_new_from_stock (GTK_STOCK_NEW);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK(templates_list_new_callback), umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK(templates_list_delete_callback), umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_GO_UP);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK(templates_list_move_up_callback), umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_GO_DOWN);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK(templates_list_move_down_callback), umlclass);
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

  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

  label = gtk_label_new(_("Name:"));
  entry = gtk_entry_new();
  prop_dialog->templ_name = GTK_ENTRY(entry);
  g_signal_connect (G_OBJECT (entry), "focus_out_event",
		    G_CALLBACK (templates_update_event), umlclass); 
  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (templates_update), umlclass); 
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Type:"));
  entry = gtk_entry_new();
  prop_dialog->templ_type = GTK_ENTRY(entry);
  g_signal_connect (G_OBJECT (entry), "focus_out_event",
		    G_CALLBACK (templates_update_event), umlclass);
  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (templates_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);

  gtk_widget_show(vbox2);
  
  /* TODO: Add stuff here! */
  
  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page (notebook, vbox, page_label);
}
