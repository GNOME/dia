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

#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "object.h"
#include "objchange.h"
#include "intl.h"
#include "class.h"


typedef struct _Disconnect {
  ConnectionPoint *cp;
  DiaObject *other_object;
  Handle *other_handle;
} Disconnect;

typedef struct _UMLClassState UMLClassState;

struct _UMLClassState {
  char *name;
  char *stereotype;
  int abstract;
  int suppress_attributes;
  int suppress_operations;
  int visible_attributes;
  int visible_operations;

  /* Attributes: */
  GList *attributes;

  /* Operators: */
  GList *operations;

  /* Template: */
  int template;
  GList *formal_params;
};


typedef struct _UMLClassChange UMLClassChange;

struct _UMLClassChange {
  ObjectChange obj_change;
  
  UMLClass *obj;

  GList *added_cp;
  GList *deleted_cp;
  GList *disconnected;

  int applied;
  
  UMLClassState *saved_state;
};

static UMLClassState *umlclass_get_state(UMLClass *umlclass);
static ObjectChange *new_umlclass_change(UMLClass *obj, UMLClassState *saved_state,
					 GList *added, GList *deleted,
					 GList *disconnected);

/**** Utility functions ******/
static void
umlclass_store_disconnects(UMLClassDialog *prop_dialog,
			   ConnectionPoint *cp)
{
  Disconnect *dis;
  DiaObject *connected_obj;
  GList *list;
  int i;
  
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

/********************************************************
 ******************** CLASS *****************************
 ********************************************************/

static void
class_read_from_dialog(UMLClass *umlclass, UMLClassDialog *prop_dialog)
{
  const gchar *s;

  if (umlclass->name != NULL)
    g_free(umlclass->name);

  s = gtk_entry_get_text (prop_dialog->classname);
  if (s && s[0])
    umlclass->name = g_strdup (s);
  else
    umlclass->name = NULL;

  if (umlclass->stereotype != NULL)
    g_free(umlclass->stereotype);
  
  s = gtk_entry_get_text(prop_dialog->stereotype);
  if (s && s[0])
    umlclass->stereotype = g_strdup (s);
  else
    umlclass->stereotype = NULL;
  
  if (umlclass->comment != NULL)
    g_free (umlclass->comment);

  s = gtk_entry_get_text(prop_dialog->comment);
  if (s && s[0])
    umlclass->comment = g_strdup (s);
  else
    umlclass->comment = NULL;
  
  umlclass->abstract = prop_dialog->abstract_class->active;
  umlclass->visible_attributes = prop_dialog->attr_vis->active;
  umlclass->visible_operations = prop_dialog->op_vis->active;
  umlclass->wrap_operations = prop_dialog->op_wrap->active;
  umlclass->wrap_after_char = gtk_spin_button_get_value_as_int(prop_dialog->wrap_after_char);
  umlclass->visible_comments = prop_dialog->comments_vis->active;
  umlclass->suppress_attributes = prop_dialog->attr_supp->active;
  umlclass->suppress_operations = prop_dialog->op_supp->active;
  umlclass->text_color = prop_dialog->text_color->col;
  umlclass->line_color = prop_dialog->line_color->col;
  umlclass->fill_color = prop_dialog->fill_color->col;
  umlclass->normal_font = dia_font_selector_get_font (prop_dialog->normal_font);
  umlclass->polymorphic_font = dia_font_selector_get_font (prop_dialog->polymorphic_font);
  umlclass->abstract_font = dia_font_selector_get_font (prop_dialog->abstract_font);
  umlclass->classname_font = dia_font_selector_get_font (prop_dialog->classname_font);
  umlclass->abstract_classname_font = dia_font_selector_get_font (prop_dialog->abstract_classname_font);
  umlclass->comment_font = dia_font_selector_get_font (prop_dialog->comment_font);

  umlclass->font_height = gtk_spin_button_get_value_as_float (prop_dialog->normal_font_height);
  umlclass->abstract_font_height = gtk_spin_button_get_value_as_float (prop_dialog->abstract_font_height);
  umlclass->polymorphic_font_height = gtk_spin_button_get_value_as_float (prop_dialog->polymorphic_font_height);
  umlclass->classname_font_height = gtk_spin_button_get_value_as_float (prop_dialog->classname_font_height);
  umlclass->abstract_classname_font_height = gtk_spin_button_get_value_as_float (prop_dialog->abstract_classname_font_height);
  umlclass->comment_font_height = gtk_spin_button_get_value_as_float (prop_dialog->comment_font_height);

}

static void
class_fill_in_dialog(UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;

  prop_dialog = umlclass->properties_dialog;

  if (umlclass->name)
    gtk_entry_set_text(prop_dialog->classname, umlclass->name);
  if (umlclass->stereotype != NULL)
    gtk_entry_set_text(prop_dialog->stereotype, umlclass->stereotype);
  else
    gtk_entry_set_text(prop_dialog->stereotype, "");

  if (umlclass->comment != NULL)
    gtk_entry_set_text(prop_dialog->comment, umlclass->comment);
  else
    gtk_entry_set_text(prop_dialog->comment, "");

  gtk_toggle_button_set_active(prop_dialog->abstract_class, umlclass->abstract);
  gtk_toggle_button_set_active(prop_dialog->attr_vis, umlclass->visible_attributes);
  gtk_toggle_button_set_active(prop_dialog->op_vis, umlclass->visible_operations);
  gtk_toggle_button_set_active(prop_dialog->op_wrap, umlclass->wrap_operations);
  gtk_spin_button_set_value (prop_dialog->wrap_after_char, umlclass->wrap_after_char);
  gtk_toggle_button_set_active(prop_dialog->comments_vis, umlclass->visible_comments);
  gtk_toggle_button_set_active(prop_dialog->attr_supp, umlclass->suppress_attributes);
  gtk_toggle_button_set_active(prop_dialog->op_supp, umlclass->suppress_operations);
  dia_color_selector_set_color(prop_dialog->text_color, &umlclass->text_color);
  dia_color_selector_set_color(prop_dialog->line_color, &umlclass->line_color);
  dia_color_selector_set_color(prop_dialog->fill_color, &umlclass->fill_color);
  dia_font_selector_set_font (prop_dialog->normal_font, umlclass->normal_font);
  dia_font_selector_set_font (prop_dialog->polymorphic_font, umlclass->polymorphic_font);
  dia_font_selector_set_font (prop_dialog->abstract_font, umlclass->abstract_font);
  dia_font_selector_set_font (prop_dialog->classname_font, umlclass->classname_font);
  dia_font_selector_set_font (prop_dialog->abstract_classname_font, umlclass->abstract_classname_font);
  dia_font_selector_set_font (prop_dialog->comment_font, umlclass->comment_font);
  gtk_spin_button_set_value (prop_dialog->normal_font_height, umlclass->font_height);
  gtk_spin_button_set_value (prop_dialog->polymorphic_font_height, umlclass->polymorphic_font_height);
  gtk_spin_button_set_value (prop_dialog->abstract_font_height, umlclass->abstract_font_height);
  gtk_spin_button_set_value (prop_dialog->classname_font_height, umlclass->classname_font_height);
  gtk_spin_button_set_value (prop_dialog->abstract_classname_font_height, umlclass->abstract_classname_font_height);
  gtk_spin_button_set_value (prop_dialog->comment_font_height, umlclass->comment_font_height);
}

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
class_create_page(GtkNotebook *notebook,  UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *vbox;
  GtkWidget *entry;
  GtkWidget *checkbox;
  GtkWidget *text_color;
  GtkWidget *fill_color;
  GtkWidget *line_color;
  GtkWidget *table;
  GtkObject *adj;

  prop_dialog = umlclass->properties_dialog;

  /* Class page: */
  page_label = gtk_label_new_with_mnemonic (_("_Class"));
  
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
  
  table = gtk_table_new (3, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  label = gtk_label_new(_("Class name:"));
  entry = gtk_entry_new();
  prop_dialog->classname = GTK_ENTRY(entry);
  gtk_widget_grab_focus(entry);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Stereotype:"));
  entry = gtk_entry_new();
  prop_dialog->stereotype = GTK_ENTRY(entry);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Comment:"));
  entry = gtk_entry_new();
  prop_dialog->comment = GTK_ENTRY(entry);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,2,3, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,2,3, GTK_FILL | GTK_EXPAND,0, 0,2);

  hbox = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Abstract"));
  prop_dialog->abstract_class = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Attributes visible"));
  prop_dialog->attr_vis = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  checkbox = gtk_check_button_new_with_label(_("Suppress Attributes"));
  prop_dialog->attr_supp = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Operations visible"));
  prop_dialog->op_vis = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  checkbox = gtk_check_button_new_with_label(_("Suppress operations"));
  prop_dialog->op_supp = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  hbox  = gtk_hbox_new(TRUE, 5);
  hbox2 = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Wrap Operations"));
  prop_dialog->op_wrap = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  adj = gtk_adjustment_new( umlclass->wrap_after_char, 0.0, 200.0, 1.0, 5.0, 1.0);
  prop_dialog->wrap_after_char = GTK_SPIN_BUTTON(gtk_spin_button_new( GTK_ADJUSTMENT( adj), 0.1, 0));
  gtk_spin_button_set_numeric( GTK_SPIN_BUTTON( prop_dialog->wrap_after_char), TRUE);
  gtk_spin_button_set_snap_to_ticks( GTK_SPIN_BUTTON( prop_dialog->wrap_after_char), TRUE);
  prop_dialog->max_length_label = GTK_LABEL( gtk_label_new( _("Wrap after this length: ")));
  gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET( prop_dialog->max_length_label), FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET( prop_dialog->wrap_after_char), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET( hbox2), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 5);
  checkbox = gtk_check_button_new_with_label(_("Comments visible"));
  prop_dialog->comments_vis = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  /** Fonts and Colors selection **/
  gtk_box_pack_start (GTK_BOX (vbox), gtk_hseparator_new(), FALSE, FALSE, 3);

  table = gtk_table_new (5, 6, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
  gtk_table_set_homogeneous (GTK_TABLE (table), FALSE);

  /* head line */
  label = gtk_label_new (_("Kind"));
                                                    /* L, R, T, B */
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  label = gtk_label_new (_("Font"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 0, 1);
  label = gtk_label_new (_("Size"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 0, 1);
  
  /* property rows */
  create_font_props_row (GTK_TABLE (table), _("Normal"), 1,
                         umlclass->normal_font,
                         umlclass->font_height,
                         &(prop_dialog->normal_font),
                         &(prop_dialog->normal_font_height));
  create_font_props_row (GTK_TABLE (table), _("Polymorphic"), 2,
                         umlclass->polymorphic_font,
                         umlclass->polymorphic_font_height,
                         &(prop_dialog->polymorphic_font),
                         &(prop_dialog->polymorphic_font_height));
  create_font_props_row (GTK_TABLE (table), _("Abstract"), 3,
                         umlclass->abstract_font,
                         umlclass->abstract_font_height,
                         &(prop_dialog->abstract_font),
                         &(prop_dialog->abstract_font_height));
  create_font_props_row (GTK_TABLE (table), _("Class Name"), 4,
                         umlclass->classname_font,
                         umlclass->classname_font_height,
                         &(prop_dialog->classname_font),
                         &(prop_dialog->classname_font_height));
  create_font_props_row (GTK_TABLE (table), _("Abstract Class"), 5,
                         umlclass->abstract_classname_font,
                         umlclass->abstract_classname_font_height,
                         &(prop_dialog->abstract_classname_font),
                         &(prop_dialog->abstract_classname_font_height));
  create_font_props_row (GTK_TABLE (table), _("Comment"), 6,
                         umlclass->comment_font,
                         umlclass->comment_font_height,
                         &(prop_dialog->comment_font),
                         &(prop_dialog->comment_font_height));



  table = gtk_table_new (2, 3, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox),
		      table, FALSE, TRUE, 0);
  /* should probably be refactored too. */
  label = gtk_label_new(_("Text Color"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 2);
  text_color = dia_color_selector_new();
  dia_color_selector_set_color((DiaColorSelector *)text_color, &umlclass->text_color);
  prop_dialog->text_color = (DiaColorSelector *)text_color;
  gtk_table_attach (GTK_TABLE (table), text_color, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, 0, 3, 2);

  label = gtk_label_new(_("Foreground Color"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 2);
  line_color = dia_color_selector_new();
  dia_color_selector_set_color((DiaColorSelector *)line_color, &umlclass->line_color);
  prop_dialog->line_color = (DiaColorSelector *)line_color;
  gtk_table_attach (GTK_TABLE (table), line_color, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 3, 2);
  
  label = gtk_label_new(_("Background Color"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 2);
  fill_color = dia_color_selector_new();
  dia_color_selector_set_color((DiaColorSelector *)fill_color, &umlclass->fill_color);
  prop_dialog->fill_color = (DiaColorSelector *)fill_color;
  gtk_table_attach (GTK_TABLE (table), fill_color, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL, 0, 3, 2);

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
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->attr_comment), val);
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

  if (attr->comment != NULL)
    gtk_entry_set_text(prop_dialog->attr_comment, attr->comment);
  else
    gtk_entry_set_text(prop_dialog->attr_comment, "");


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
  gtk_entry_set_text(prop_dialog->attr_comment, "");
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
  attr->comment = g_strdup (gtk_entry_get_text(prop_dialog->attr_comment));

  attr->visibility = (UMLVisibility)
		GPOINTER_TO_INT (gtk_object_get_user_data (
					 GTK_OBJECT (gtk_menu_get_active (prop_dialog->attr_visible))));
    
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

  /* Due to GtkList oddities, this may get called during destroy.
   * But it'll reference things that are already dead and crash.
   * Thus, we stop it before it gets that bad.  See bug #156706 for
   * one example.
   */
  if (umlclass->destroyed) return;

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
  char *utfstr;
  prop_dialog = umlclass->properties_dialog;

  attributes_get_current_values(prop_dialog);

  attr = uml_attribute_new();

  utfstr = uml_get_attribute_string (attr);
  list_item = gtk_list_item_new_with_label (utfstr);
  gtk_widget_show (list_item);
  g_free (utfstr);

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
  DiaObject *obj;

  obj = &umlclass->element.object;

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
      attr->left_connection = g_new0(ConnectionPoint,1);
      attr->left_connection->object = obj;
      attr->left_connection->connected = NULL;
      
      attr->right_connection = g_new0(ConnectionPoint,1);
      attr->right_connection->object = obj;
      attr->right_connection->connected = NULL;

      prop_dialog->added_connections =
	g_list_prepend(prop_dialog->added_connections,
		       attr->left_connection);
      prop_dialog->added_connections =
	g_list_prepend(prop_dialog->added_connections,
		       attr->right_connection);
    } 

    if ( (prop_dialog->attr_vis->active) &&
	 (!prop_dialog->attr_supp->active) ) { 
      obj->connections[connection_index++] = attr->left_connection;
      obj->connections[connection_index++] = attr->right_connection;
    } else {
      umlclass_store_disconnects(prop_dialog, attr->left_connection);
      object_remove_connections_to(attr->left_connection);
      umlclass_store_disconnects(prop_dialog, attr->right_connection);
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
	gtk_list_item_new_with_label (g_list_nth(umlclass->attributes_strings, i)->data);
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

static int
attributes_update_event(GtkWidget *widget, GdkEventFocus *ev, UMLClass *umlclass)
{
  attributes_get_current_values(umlclass->properties_dialog);
  return 0;
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
  GtkWidget *table;
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

  gtk_signal_connect (GTK_OBJECT (list), "selection_changed",
		      GTK_SIGNAL_FUNC(attributes_list_selection_changed_callback),
		      umlclass);

  vbox2 = gtk_vbox_new(FALSE, 5);

  button = gtk_button_new_with_mnemonic (_("_New"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(attributes_list_new_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_mnemonic (_("_Delete"));
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

  table = gtk_table_new (5, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

  label = gtk_label_new(_("Name:"));
  entry = gtk_entry_new();
  prop_dialog->attr_name = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (attributes_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (attributes_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Type:"));
  entry = gtk_entry_new();
  prop_dialog->attr_type = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (attributes_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (attributes_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Value:"));
  entry = gtk_entry_new();
  prop_dialog->attr_value = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (attributes_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (attributes_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,2,3, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,2,3, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Comment:"));
  entry = gtk_entry_new();
  prop_dialog->attr_comment = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (attributes_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (attributes_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,3,4, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,3,4, GTK_FILL | GTK_EXPAND,0, 0,2);


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
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_comment), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_kind), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_kind_button), val);
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
  if (param->comment != NULL)
    gtk_entry_set_text(prop_dialog->param_comment, param->comment);
  else
    gtk_entry_set_text(prop_dialog->param_comment, "");

  gtk_option_menu_set_history(prop_dialog->param_kind_button,
			      (gint)param->kind);
}

static void
parameters_clear_values(UMLClassDialog *prop_dialog)
{
  gtk_entry_set_text(prop_dialog->param_name, "");
  gtk_entry_set_text(prop_dialog->param_type, "");
  gtk_entry_set_text(prop_dialog->param_value, "");
  gtk_entry_set_text(prop_dialog->param_comment, "");
  gtk_option_menu_set_history(prop_dialog->param_kind_button,
			      (gint) UML_UNDEF_KIND);

}

static void
parameters_get_values (UMLClassDialog *prop_dialog, UMLParameter *param)
{
  g_free(param->name);
  g_free(param->type);
  g_free(param->comment);
  if (param->value != NULL)
    g_free(param->value);
  
  param->name = g_strdup (gtk_entry_get_text (prop_dialog->param_name));
  param->type = g_strdup (gtk_entry_get_text (prop_dialog->param_type));
  
  param->value = g_strdup (gtk_entry_get_text(prop_dialog->param_value));
  param->comment = g_strdup (gtk_entry_get_text(prop_dialog->param_comment));

  param->kind = (UMLParameterKind) GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(gtk_menu_get_active(prop_dialog->param_kind))));
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
  char *utf;

  prop_dialog = umlclass->properties_dialog;

  parameters_get_current_values(prop_dialog);

  current_op = (UMLOperation *)
    gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_op));
  
  param = uml_parameter_new();

  utf = uml_get_parameter_string (param);
  list_item = gtk_list_item_new_with_label (utf);
  gtk_widget_show (list_item);
  g_free (utf);

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
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_stereotype), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_comment), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_visible_button), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_visible), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_class_scope), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_inheritance_type), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_inheritance_type_button), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_query), val);

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
  gchar *str;

  gtk_entry_set_text(prop_dialog->op_name, op->name);
  if (op->type != NULL)
    gtk_entry_set_text(prop_dialog->op_type, op->type);
  else
    gtk_entry_set_text(prop_dialog->op_type, "");

  if (op->stereotype != NULL)
    gtk_entry_set_text(prop_dialog->op_stereotype, op->stereotype);
  else
    gtk_entry_set_text(prop_dialog->op_stereotype, "");

  if (op->comment != NULL)
    gtk_entry_set_text(prop_dialog->op_comment, op->comment);
  else
    gtk_entry_set_text(prop_dialog->op_comment, "");

  gtk_option_menu_set_history(prop_dialog->op_visible_button,
			      (gint)op->visibility);
  gtk_toggle_button_set_active(prop_dialog->op_class_scope, op->class_scope);
  gtk_toggle_button_set_active(prop_dialog->op_query, op->query);
  gtk_option_menu_set_history(prop_dialog->op_inheritance_type_button,
			      (gint)op->inheritance_type);

  gtk_list_clear_items(prop_dialog->parameters_list, 0, -1);
  prop_dialog->current_param = NULL;
  parameters_set_sensitive(prop_dialog, FALSE);

  list = op->parameters;
  while (list != NULL) {
    param = (UMLParameter *)list->data;

    str = uml_get_parameter_string (param);
    list_item = gtk_list_item_new_with_label (str);
    g_free (str);

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
  gtk_entry_set_text(prop_dialog->op_stereotype, "");
  gtk_entry_set_text(prop_dialog->op_comment, "");
  gtk_toggle_button_set_active(prop_dialog->op_class_scope, FALSE);
  gtk_toggle_button_set_active(prop_dialog->op_query, FALSE);

  gtk_list_clear_items(prop_dialog->parameters_list, 0, -1);
  prop_dialog->current_param = NULL;
  parameters_set_sensitive(prop_dialog, FALSE);
}


static void
operations_get_values(UMLClassDialog *prop_dialog, UMLOperation *op)
{
  const gchar *s;

  g_free(op->name);
  if (op->type != NULL)
	  g_free(op->type);

  op->name = g_strdup(gtk_entry_get_text(prop_dialog->op_name));
  op->type = g_strdup (gtk_entry_get_text(prop_dialog->op_type));
  op->comment = g_strdup(gtk_entry_get_text(prop_dialog->op_comment));

  s = gtk_entry_get_text(prop_dialog->op_stereotype);
  if (s && s[0])
    op->stereotype = g_strdup (s);
  else
    op->stereotype = NULL;

  op->visibility = (UMLVisibility)
    GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(gtk_menu_get_active(prop_dialog->op_visible))));
    
  op->class_scope = prop_dialog->op_class_scope->active;
  op->inheritance_type = (UMLInheritanceType)
    GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(gtk_menu_get_active(prop_dialog->op_inheritance_type))));

  op->query = prop_dialog->op_query->active;

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
      gtk_label_set_text (label, new_str);
      g_free (new_str);
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

  if (!prop_dialog)
    return; /* maybe hiding a bug elsewhere */

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
  char *utfstr;

  prop_dialog = umlclass->properties_dialog;

  operations_get_current_values(prop_dialog);

  op = uml_operation_new();

  utfstr = uml_get_operation_string (op);
  list_item = gtk_list_item_new_with_label (utfstr);
  gtk_widget_show (list_item);
  g_free (utfstr);

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
  DiaObject *obj;

  obj = &umlclass->element.object;

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
      op->left_connection = g_new0(ConnectionPoint,1);
      op->left_connection->object = obj;
      op->left_connection->connected = NULL;
      
      op->right_connection = g_new0(ConnectionPoint,1);
      op->right_connection->object = obj;
      op->right_connection->connected = NULL;
      
      prop_dialog->added_connections =
	g_list_prepend(prop_dialog->added_connections,
		       op->left_connection);
      prop_dialog->added_connections =
	g_list_prepend(prop_dialog->added_connections,
		       op->right_connection);
    }
    
    if ( (prop_dialog->op_vis->active) &&
	 (!prop_dialog->op_supp->active) ) { 
      obj->connections[connection_index++] = op->left_connection;
      obj->connections[connection_index++] = op->right_connection;
    } else {
      umlclass_store_disconnects(prop_dialog, op->left_connection);
      object_remove_connections_to(op->left_connection);
      umlclass_store_disconnects(prop_dialog, op->right_connection);
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
	gtk_list_item_new_with_label (g_list_nth(umlclass->operations_strings, i)->data);
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

static int
operations_update_event(GtkWidget *widget, GdkEventFocus *ev, UMLClass *umlclass)
{
  operations_get_current_values(umlclass->properties_dialog);
  return 0;
}

static void 
operations_create_page(GtkNotebook *notebook,  UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *table;
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
  page_label = gtk_label_new_with_mnemonic (_("_Operations"));
  
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

  button = gtk_button_new_with_mnemonic (_("_New"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(operations_list_new_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_mnemonic (_("_Delete"));
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

  table = gtk_table_new (3, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

  label = gtk_label_new(_("Name:"));
  entry = gtk_entry_new();
  prop_dialog->op_name = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Type:"));
  entry = gtk_entry_new();
  prop_dialog->op_type = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Stereotype:"));
  entry = gtk_entry_new();
  prop_dialog->op_stereotype = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,2,3, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,2,3, GTK_FILL | GTK_EXPAND,0, 0,2);


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
  label = gtk_label_new(_("Inheritance type:"));

  omenu = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  prop_dialog->op_inheritance_type = GTK_MENU(menu);
  prop_dialog->op_inheritance_type_button = GTK_OPTION_MENU(omenu);
  submenu = NULL;
  group = NULL;
    
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Abstract"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem),
			   GINT_TO_POINTER(UML_ABSTRACT) );
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Polymorphic (virtual)"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem),
			   GINT_TO_POINTER(UML_POLYMORPHIC) );
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Leaf (final)"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem),
			   GINT_TO_POINTER(UML_LEAF) );
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);

  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), omenu, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, TRUE, 0);

  hbox2 = gtk_hbox_new(FALSE, 5);

  checkbox = gtk_check_button_new_with_label(_("Query"));
  prop_dialog->op_query = GTK_TOGGLE_BUTTON(checkbox);

  gtk_box_pack_start (GTK_BOX (hbox2), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, TRUE, 0);
 
  hbox2 = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Comment:"));
  entry = gtk_entry_new();
  prop_dialog->op_comment = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
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

  button = gtk_button_new_with_mnemonic (_("_New"));
  prop_dialog->param_new_button = button;
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(parameters_list_new_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_mnemonic (_("_Delete"));
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
  
  table = gtk_table_new (5, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox3), table, FALSE, FALSE, 0);

  label = gtk_label_new(_("Name:"));
  entry = gtk_entry_new();
  prop_dialog->param_name = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Type:"));
  entry = gtk_entry_new();
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  prop_dialog->param_type = GTK_ENTRY(entry);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Def. value:"));
  entry = gtk_entry_new();
  prop_dialog->param_value = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,2,3, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,2,3, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Comment:"));
  entry = gtk_entry_new();
  prop_dialog->param_comment = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (operations_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,3,4, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,3,4, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Direction:"));

  omenu = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  prop_dialog->param_kind = GTK_MENU(menu);
  prop_dialog->param_kind_button = GTK_OPTION_MENU(omenu);
  submenu = NULL;
  group = NULL;
    
  menuitem = gtk_radio_menu_item_new_with_label (group, _("Undefined"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem),
			   GINT_TO_POINTER(UML_UNDEF_KIND) );
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  menuitem = gtk_radio_menu_item_new_with_label (group, _("In"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem),
			   GINT_TO_POINTER(UML_IN) );
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("Out"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem),
			   GINT_TO_POINTER(UML_OUT) );
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_radio_menu_item_new_with_label (group, _("In & Out"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (operations_update), umlclass);
  gtk_object_set_user_data(GTK_OBJECT(menuitem),
			   GINT_TO_POINTER(UML_INOUT) );
  group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);

  { 
    GtkWidget * align;
    align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
    gtk_container_add(GTK_CONTAINER(align), omenu);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0,1,4,5, GTK_FILL,0, 0,3);
    gtk_table_attach (GTK_TABLE (table), align, 1,2,4,5, GTK_FILL,0, 0,3);
  }

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


static void
templates_get_current_values(UMLClassDialog *prop_dialog)
{
  UMLFormalParameter *current_param;
  GtkLabel *label;
  gchar* new_str;

  if (prop_dialog->current_templ != NULL) {
    current_param = (UMLFormalParameter *)
      gtk_object_get_user_data(GTK_OBJECT(prop_dialog->current_templ));
    if (current_param != NULL) {
      templates_get_values(prop_dialog, current_param);
      label = GTK_LABEL(GTK_BIN(prop_dialog->current_templ)->child);
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
  char *utfstr;

  prop_dialog = umlclass->properties_dialog;

  templates_get_current_values(prop_dialog);

  param = uml_formalparameter_new();

  utfstr = uml_get_formalparameter_string (param);
  list_item = gtk_list_item_new_with_label (utfstr);
  gtk_widget_show (list_item);
  g_free (utfstr);

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

static int
templates_update_event(GtkWidget *widget, GdkEventFocus *ev, UMLClass *umlclass)
{
  templates_get_current_values(umlclass->properties_dialog);
  return 0;
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

  gtk_signal_connect (GTK_OBJECT (list), "selection_changed",
		      GTK_SIGNAL_FUNC(templates_list_selection_changed_callback),
		      umlclass);

  vbox2 = gtk_vbox_new(FALSE, 5);

  button = gtk_button_new_with_mnemonic (_("_New"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(templates_list_new_callback),
		      umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_with_mnemonic (_("_Delete"));
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

  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

  label = gtk_label_new(_("Name:"));
  entry = gtk_entry_new();
  prop_dialog->templ_name = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (templates_update_event), umlclass); 
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (templates_update), umlclass); 
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Type:"));
  entry = gtk_entry_new();
  prop_dialog->templ_type = GTK_ENTRY(entry);
  gtk_signal_connect (GTK_OBJECT (entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (templates_update_event), umlclass);
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      GTK_SIGNAL_FUNC (templates_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);

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

void
destroy_properties_dialog (GtkWidget* widget,
			   gpointer user_data)
{
  /* dialog gone, mark as such */
  UMLClass *umlclass = (UMLClass *)user_data;

  g_free (umlclass->properties_dialog);
  umlclass->properties_dialog = NULL;
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
  DiaObject *obj;
  GList *list;
  int num_attrib, num_ops;
  GList *added, *deleted, *disconnected;
  UMLClassState *old_state = NULL;
  
  prop_dialog = umlclass->properties_dialog;

  old_state = umlclass_get_state(umlclass);
  
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
  obj = &umlclass->element.object;
  obj->num_connections = UMLCLASS_CONNECTIONPOINTS + num_attrib*2 + num_ops*2;
  obj->connections =
    g_realloc(obj->connections,
	      obj->num_connections*sizeof(ConnectionPoint *));
  
  /* Read from dialog and put in object: */
  class_read_from_dialog(umlclass, prop_dialog);
  attributes_read_from_dialog(umlclass, prop_dialog, UMLCLASS_CONNECTIONPOINTS);
  /* ^^^ attribs must be called before ops, to get the right order of the
     connectionpoints. */
  operations_read_from_dialog(umlclass, prop_dialog, UMLCLASS_CONNECTIONPOINTS+num_attrib*2);
  templates_read_from_dialog(umlclass, prop_dialog);

  /* unconnect from all deleted connectionpoints. */
  list = prop_dialog->deleted_connections;
  while (list != NULL) {
    ConnectionPoint *connection = (ConnectionPoint *) list->data;

    umlclass_store_disconnects(prop_dialog, connection);
    object_remove_connections_to(connection);
    
    list = g_list_next(list);
  }
  
  deleted = prop_dialog->deleted_connections;
  prop_dialog->deleted_connections = NULL;
  
  added = prop_dialog->added_connections;
  prop_dialog->added_connections = NULL;
    
  disconnected = prop_dialog->disconnected_connections;
  prop_dialog->disconnected_connections = NULL;
    
  /* Update data: */
  umlclass_calculate_data(umlclass);
  umlclass_update_data(umlclass);

  /* Fill in class with the new data: */
  fill_in_dialog(umlclass);
  return  new_umlclass_change(umlclass, old_state, added, deleted, disconnected);
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
umlclass_get_properties(UMLClass *umlclass, gboolean is_default)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *vbox;
  GtkWidget *notebook;

  if (umlclass->properties_dialog == NULL) {
    prop_dialog = g_new(UMLClassDialog, 1);
    umlclass->properties_dialog = prop_dialog;

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_object_ref(GTK_OBJECT(vbox));
    gtk_object_sink(GTK_OBJECT(vbox));
    prop_dialog->dialog = vbox;

    prop_dialog->current_attr = NULL;
    prop_dialog->current_op = NULL;
    prop_dialog->current_param = NULL;
    prop_dialog->current_templ = NULL;
    prop_dialog->deleted_connections = NULL;
    prop_dialog->added_connections = NULL;
    prop_dialog->disconnected_connections = NULL;
    
    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_box_pack_start (GTK_BOX (vbox),	notebook, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (notebook), 10);

    gtk_object_set_user_data(GTK_OBJECT(notebook), (gpointer) umlclass);
    
    gtk_signal_connect (GTK_OBJECT (notebook),
			"switch_page",
			GTK_SIGNAL_FUNC(switch_page_callback),
			(gpointer) umlclass);
    gtk_signal_connect (GTK_OBJECT (umlclass->properties_dialog->dialog),
		        "destroy",
			GTK_SIGNAL_FUNC(destroy_properties_dialog),
			(gpointer) umlclass);
    
    create_dialog_pages(GTK_NOTEBOOK( notebook ), umlclass);

    gtk_widget_show (notebook);
  }

  fill_in_dialog(umlclass);
  gtk_widget_show (umlclass->properties_dialog->dialog);

  return umlclass->properties_dialog->dialog;
}


/****************** UNDO stuff: ******************/

static void
umlclass_free_state(UMLClassState *state)
{
  GList *list;

  g_free(state->name);
  g_free(state->stereotype);

  list = state->attributes;
  while (list) {
    uml_attribute_destroy((UMLAttribute *) list->data);
    list = g_list_next(list);
  }
  g_list_free(state->attributes);

  list = state->operations;
  while (list) {
    uml_operation_destroy((UMLOperation *) list->data);
    list = g_list_next(list);
  }
  g_list_free(state->operations);

  list = state->formal_params;
  while (list) {
    uml_formalparameter_destroy((UMLFormalParameter *) list->data);
    list = g_list_next(list);
  }
  g_list_free(state->formal_params);
}

static UMLClassState *
umlclass_get_state(UMLClass *umlclass)
{
  UMLClassState *state = g_new0(UMLClassState, 1);
  GList *list;

  state->name = g_strdup(umlclass->name);
  state->stereotype = g_strdup(umlclass->stereotype);

  state->abstract = umlclass->abstract;
  state->suppress_attributes = umlclass->suppress_attributes;
  state->suppress_operations = umlclass->suppress_operations;
  state->visible_attributes = umlclass->visible_attributes;
  state->visible_operations = umlclass->visible_operations;

  
  state->attributes = NULL;
  list = umlclass->attributes;
  while (list != NULL) {
    UMLAttribute *attr = (UMLAttribute *)list->data;
    UMLAttribute *attr_copy;
      
    attr_copy = uml_attribute_copy(attr);

    state->attributes = g_list_append(state->attributes, attr_copy);
    list = g_list_next(list);
  }

  
  state->operations = NULL;
  list = umlclass->operations;
  while (list != NULL) {
    UMLOperation *op = (UMLOperation *)list->data;
    UMLOperation *op_copy;
      
    op_copy = uml_operation_copy(op);

    state->operations = g_list_append(state->operations, op_copy);
    list = g_list_next(list);
  }


  state->template = umlclass->template;
  
  state->formal_params = NULL;
  list = umlclass->formal_params;
  while (list != NULL) {
    UMLFormalParameter *param = (UMLFormalParameter *)list->data;
    UMLFormalParameter *param_copy;
    
    param_copy = uml_formalparameter_copy(param);
    state->formal_params = g_list_append(state->formal_params, param_copy);
    
    list = g_list_next(list);
  }

  return state;
}

static void
umlclass_update_connectionpoints(UMLClass *umlclass)
{
  int num_attrib, num_ops;
  DiaObject *obj;
  GList *list;
  int connection_index;
  UMLClassDialog *prop_dialog;

  prop_dialog = umlclass->properties_dialog;
  
  /* Allocate enought connection points for attributes and operations. */
  /* (two per op/attr) */
  if ( (umlclass->visible_attributes) && (!umlclass->suppress_attributes))
    num_attrib = g_list_length(umlclass->attributes);
  else
    num_attrib = 0;
  if ( (umlclass->visible_operations) && (!umlclass->suppress_operations))
    num_ops = g_list_length(umlclass->operations);
  else
    num_ops = 0;
  
  obj = &umlclass->element.object;
  obj->num_connections = UMLCLASS_CONNECTIONPOINTS + num_attrib*2 + num_ops*2;
  obj->connections =
    g_realloc(obj->connections,
	      obj->num_connections*sizeof(ConnectionPoint *));

  connection_index = UMLCLASS_CONNECTIONPOINTS;
  
  list = umlclass->attributes;
  while (list != NULL) {
    UMLAttribute *attr = (UMLAttribute *) list->data;
    
    if ( (umlclass->visible_attributes) &&
	 (!umlclass->suppress_attributes)) {
      obj->connections[connection_index++] = attr->left_connection;
      obj->connections[connection_index++] = attr->right_connection;
    }
    
    list = g_list_next(list);
  }
  
  gtk_list_clear_items (GTK_LIST (prop_dialog->attributes_list), 0, -1);

  list = umlclass->operations;
  while (list != NULL) {
    UMLOperation *op = (UMLOperation *) list->data;
    
    if ( (umlclass->visible_operations) &&
	 (!umlclass->suppress_operations)) {
      obj->connections[connection_index++] = op->left_connection;
      obj->connections[connection_index++] = op->right_connection;
    }
    
    list = g_list_next(list);
  }
  gtk_list_clear_items (GTK_LIST (prop_dialog->operations_list), 0, -1);
  
}

static void
umlclass_set_state(UMLClass *umlclass, UMLClassState *state)
{
  umlclass->name = state->name;
  umlclass->stereotype = state->stereotype;

  umlclass->abstract = state->abstract;
  umlclass->suppress_attributes = state->suppress_attributes;
  umlclass->suppress_operations = state->suppress_operations;
  umlclass->visible_attributes = state->visible_attributes;
  umlclass->visible_operations = state->visible_operations;

  umlclass->attributes = state->attributes;
  umlclass->operations = state->operations;
  umlclass->template = state->template;
  umlclass->formal_params = state->formal_params;

  g_free(state);

  umlclass_update_connectionpoints(umlclass);

  umlclass_calculate_data(umlclass);
  umlclass_update_data(umlclass);
}

static void
umlclass_change_apply(UMLClassChange *change, DiaObject *obj)
{
  UMLClassState *old_state;
  GList *list;
  
  old_state = umlclass_get_state(change->obj);

  umlclass_set_state(change->obj, change->saved_state);

  list = change->disconnected;
  while (list) {
    Disconnect *dis = (Disconnect *)list->data;

    object_unconnect(dis->other_object, dis->other_handle);

    list = g_list_next(list);
  }

  change->saved_state = old_state;
  change->applied = 1;
}

static void
umlclass_change_revert(UMLClassChange *change, DiaObject *obj)
{
  UMLClassState *old_state;
  GList *list;
  
  old_state = umlclass_get_state(change->obj);

  umlclass_set_state(change->obj, change->saved_state);
  
  list = change->disconnected;
  while (list) {
    Disconnect *dis = (Disconnect *)list->data;

    object_connect(dis->other_object, dis->other_handle, dis->cp);

    list = g_list_next(list);
  }
  
  change->saved_state = old_state;
  change->applied = 0;
}

static void
umlclass_change_free(UMLClassChange *change)
{
  GList *list, *free_list;

  umlclass_free_state(change->saved_state);
  g_free(change->saved_state);

  if (change->applied) 
    free_list = change->deleted_cp;
  else
    free_list = change->added_cp;

  list = free_list;
  while (list != NULL) {
    ConnectionPoint *connection = (ConnectionPoint *) list->data;
    
    g_assert(connection->connected == NULL); /* Paranoid */
    object_remove_connections_to(connection); /* Shouldn't be needed */
    g_free(connection);
      
    list = g_list_next(list);
  }

  g_list_free(free_list);
  
}

static ObjectChange *
new_umlclass_change(UMLClass *obj, UMLClassState *saved_state,
		    GList *added, GList *deleted, GList *disconnected)
{
  UMLClassChange *change;

  change = g_new0(UMLClassChange, 1);
  
  change->obj_change.apply =
    (ObjectChangeApplyFunc) umlclass_change_apply;
  change->obj_change.revert =
    (ObjectChangeRevertFunc) umlclass_change_revert;
  change->obj_change.free =
    (ObjectChangeFreeFunc) umlclass_change_free;

  change->obj = obj;
  change->saved_state = saved_state;
  change->applied = 1;

  change->added_cp = added;
  change->deleted_cp = deleted;
  change->disconnected = disconnected;

  return (ObjectChange *)change;
}
