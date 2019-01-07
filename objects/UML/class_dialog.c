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
 * File:    class_dialog.c
 *
 * Purpose: This file contains the code the draws and handles the class
 *          dialog. This is the dialog box that is displayed when the
 *          class Icon is double clicked.
 */
/*--------------------------------------------------------------------------**
 * Copyright(c) 2005 David Klotzbach
**                                                                          **
** Multi-Line Comments May 10, 2005 - Dave Klotzbach                        **
** dklotzbach@foxvalley.net                                                 **
**                                                                          **
**--------------------------------------------------------------------------*/

#include <config.h>

#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "object.h"
#include "objchange.h"
#include "intl.h"
#include "class.h"
#include "diaoptionmenu.h"
#include "diafontselector.h"
#include "dia-uml-class.h"

#include "class_dialog.h"

/* hide this functionality before rewrite;) */
void 
umlclass_dialog_free (UMLClassDialog *dialog)
{
  gtk_widget_destroy(dialog->dialog);
  /* destroy-signal destroy_properties_dialog already does 'g_free(dialog);' and more */
}

typedef struct _Disconnect {
  ConnectionPoint *cp;
  DiaObject *other_object;
  Handle *other_handle;
} Disconnect;

typedef struct _UMLClassChange UMLClassChange;

struct _UMLClassChange {
  ObjectChange obj_change;
  
  UMLClass *obj;

  GList *added_cp;
  GList *deleted_cp;
  GList *disconnected;

  int applied;
  
  DiaUmlClass *saved_state;
};

static ObjectChange *
new_umlclass_change (UMLClass    *obj,
                     DiaUmlClass *saved_state,
                     GList       *added,
                     GList       *deleted,
                     GList       *disconnected);

/**** Utility functions ******/
void
_umlclass_store_disconnects (ConnectionPoint  *cp,
                             GList           **disconnected)
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
        *disconnected = g_list_prepend(*disconnected, dis);
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
  umlclass->line_width = gtk_spin_button_get_value(prop_dialog->line_width);
  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (prop_dialog->text_color),
                              &umlclass->text_color);
  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (prop_dialog->line_color),
                              &umlclass->line_color);
  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (prop_dialog->fill_color),
                              &umlclass->fill_color);

  umlclass->normal_font = dia_font_selector_get_font (prop_dialog->normal_font);
  umlclass->polymorphic_font = dia_font_selector_get_font (prop_dialog->polymorphic_font);
  umlclass->abstract_font = dia_font_selector_get_font (prop_dialog->abstract_font);
  umlclass->classname_font = dia_font_selector_get_font (prop_dialog->classname_font);
  umlclass->abstract_classname_font = dia_font_selector_get_font (prop_dialog->abstract_classname_font);
  umlclass->comment_font = dia_font_selector_get_font (prop_dialog->comment_font);

  umlclass->font_height = gtk_spin_button_get_value (prop_dialog->normal_font_height);
  umlclass->abstract_font_height = gtk_spin_button_get_value (prop_dialog->abstract_font_height);
  umlclass->polymorphic_font_height = gtk_spin_button_get_value (prop_dialog->polymorphic_font_height);
  umlclass->classname_font_height = gtk_spin_button_get_value (prop_dialog->classname_font_height);
  umlclass->abstract_classname_font_height = gtk_spin_button_get_value (prop_dialog->abstract_classname_font_height);
  umlclass->comment_font_height = gtk_spin_button_get_value (prop_dialog->comment_font_height);
}

static void
class_fill_in_dialog(UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;

  prop_dialog = umlclass->properties_dialog;

  gtk_spin_button_set_value (prop_dialog->line_width, umlclass->line_width);
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (prop_dialog->text_color),
                              &umlclass->text_color);
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (prop_dialog->line_color),
                              &umlclass->line_color);
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (prop_dialog->fill_color),
                              &umlclass->fill_color);
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
create_font_props_row (GtkGrid    *table,
                       const char *kind,
                       gint        row,
                       DiaFont    *font,
                       real        height,
                       DiaFontSelector **fontsel,
                       GtkSpinButton   **heightsel)
{
  GtkWidget *label;
  GtkAdjustment *adj;

  label = gtk_label_new (kind);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_yalign (GTK_LABEL (label), 0.5);
  gtk_grid_attach (table, label, 0, row, 1, 1);
  *fontsel = DIA_FONT_SELECTOR (dia_font_selector_new ());
  dia_font_selector_set_font (DIA_FONT_SELECTOR (*fontsel), font);
  gtk_grid_attach (GTK_GRID (table), GTK_WIDGET(*fontsel), 1, row, 1, 1);

  adj = gtk_adjustment_new (height, 0.1, 10.0, 0.1, 1.0, 0);
  *heightsel = GTK_SPIN_BUTTON (gtk_spin_button_new (adj, 1.0, 2));
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (*heightsel), TRUE);
  gtk_grid_attach (table, GTK_WIDGET (*heightsel), 2, row, 1, 1);
}

static void 
class_create_page(GtkWidget *notebook, UMLClass *umlclass)
{
  GtkWidget *page_label;
 
  /* Class page: */
  page_label = gtk_label_new_with_mnemonic (_("_Class"));
  
  umlclass->properties_dialog->editor = dia_uml_class_editor_new (dia_uml_class_new (umlclass));
  
  gtk_widget_show (page_label);
  gtk_widget_show (umlclass->properties_dialog->editor);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), umlclass->properties_dialog->editor, page_label);  
}

static void 
style_create_page(GtkNotebook *notebook,  UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *line_width;
  GtkWidget *text_color;
  GtkWidget *fill_color;
  GtkWidget *line_color;
  GtkWidget *table;
  GtkAdjustment *adj;

  prop_dialog = umlclass->properties_dialog;

  /** Fonts and Colors selection **/
  page_label = gtk_label_new_with_mnemonic (_("_Style"));
  
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
  
  table = gtk_grid_new ();
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);

  /* head line */
  label = gtk_label_new (_("Kind"));
                                                    /* L, R, T, B */
  gtk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);
  label = gtk_label_new (_("Font"));
  gtk_grid_attach (GTK_GRID (table), label, 1, 0, 1, 1);
  label = gtk_label_new (_("Size"));
  gtk_grid_attach (GTK_GRID (table), label, 2, 0, 1, 1);
  
  /* property rows */
  create_font_props_row (GTK_GRID (table), _("Normal"), 1,
                         umlclass->normal_font,
                         umlclass->font_height,
                         &(prop_dialog->normal_font),
                         &(prop_dialog->normal_font_height));
  create_font_props_row (GTK_GRID (table), _("Polymorphic"), 2,
                         umlclass->polymorphic_font,
                         umlclass->polymorphic_font_height,
                         &(prop_dialog->polymorphic_font),
                         &(prop_dialog->polymorphic_font_height));
  create_font_props_row (GTK_GRID (table), _("Abstract"), 3,
                         umlclass->abstract_font,
                         umlclass->abstract_font_height,
                         &(prop_dialog->abstract_font),
                         &(prop_dialog->abstract_font_height));
  create_font_props_row (GTK_GRID (table), _("Class Name"), 4,
                         umlclass->classname_font,
                         umlclass->classname_font_height,
                         &(prop_dialog->classname_font),
                         &(prop_dialog->classname_font_height));
  create_font_props_row (GTK_GRID (table), _("Abstract Class"), 5,
                         umlclass->abstract_classname_font,
                         umlclass->abstract_classname_font_height,
                         &(prop_dialog->abstract_classname_font),
                         &(prop_dialog->abstract_classname_font_height));
  create_font_props_row (GTK_GRID (table), _("Comment"), 6,
                         umlclass->comment_font,
                         umlclass->comment_font_height,
                         &(prop_dialog->comment_font),
                         &(prop_dialog->comment_font_height));



  table = gtk_grid_new ();
  gtk_grid_set_row_homogeneous (GTK_GRID (table), TRUE);
  gtk_grid_set_column_homogeneous (GTK_GRID (table), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox),
		      table, FALSE, TRUE, 0);
  /* should probably be refactored too. */
  label = gtk_label_new(_("Line Width"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_yalign (GTK_LABEL (label), 0.5);
  gtk_widget_set_hexpand (label, TRUE);
  gtk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);
  adj = gtk_adjustment_new (umlclass->line_width, 0.0, G_MAXFLOAT, 0.1, 1.0, 0);
  line_width = gtk_spin_button_new (adj, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (line_width), TRUE);
  prop_dialog->line_width = GTK_SPIN_BUTTON(line_width);
  gtk_widget_set_hexpand (line_width, TRUE);
  gtk_grid_attach (GTK_GRID (table), line_width, 1, 0, 1, 1);

  label = gtk_label_new(_("Text Color"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_yalign (GTK_LABEL (label), 0.5);
  gtk_widget_set_hexpand (label, TRUE);
  gtk_grid_attach (GTK_GRID (table), label, 0, 1, 1, 1);
  text_color = gtk_color_button_new();
  gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (text_color), TRUE);
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (text_color), &umlclass->text_color);
  prop_dialog->text_color = (DiaColorSelector *)text_color;

  gtk_widget_set_hexpand (text_color, TRUE);
  gtk_grid_attach (GTK_GRID (table), text_color, 1, 1, 1, 1);

  label = gtk_label_new(_("Foreground Color"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_yalign (GTK_LABEL (label), 0.5);
  gtk_widget_set_hexpand (label, TRUE);
  gtk_grid_attach (GTK_GRID (table), label, 0, 2, 1, 1);
  line_color = gtk_color_button_new();
  gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (line_color), TRUE);
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (line_color), &umlclass->line_color);
  prop_dialog->line_color = (DiaColorSelector *)line_color;

  gtk_widget_set_hexpand (line_color, TRUE);
  gtk_grid_attach (GTK_GRID (table), line_color, 1, 2, 1, 1);
  
  label = gtk_label_new(_("Background Color"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_yalign (GTK_LABEL (label), 0.5);
  gtk_widget_set_hexpand (label, TRUE);
  gtk_grid_attach (GTK_GRID (table), label, 0, 3, 1, 1);
  fill_color = gtk_color_button_new ();
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (fill_color), &umlclass->fill_color);
  prop_dialog->fill_color = (DiaColorSelector *)fill_color;

  gtk_widget_set_hexpand (fill_color, TRUE);
  gtk_grid_attach (GTK_GRID (table), fill_color, 1, 3, 1, 1);

  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page(notebook, vbox, page_label);
  
}

/******************************************************
 ******************** ALL *****************************
 ******************************************************/

static void
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
#ifdef DEBUG
  umlclass_sanity_check(umlclass, "Filling in dialog before attrs");
#endif
  class_fill_in_dialog(umlclass);
}

ObjectChange *
umlclass_apply_props_from_dialog(UMLClass *umlclass, GtkWidget *widget)
{
  UMLClassDialog *prop_dialog;
  DiaObject *obj;
  GList *list;
  int num_attrib, num_ops;
  GList *added = NULL, *deleted = NULL, *disconnected = NULL;
  DiaUmlClass *old_state = NULL;
  DiaUmlClass *editor_state;
  gboolean vis, sup;
  
#ifdef DEBUG
  umlclass_sanity_check(umlclass, "Apply from dialog start");
#endif

  prop_dialog = umlclass->properties_dialog;

  old_state = dia_uml_class_new (umlclass);

  editor_state = dia_uml_class_editor_get_class (DIA_UML_CLASS_EDITOR (prop_dialog->editor));
  
  /* Allocate enought connection points for attributes and operations. */
  /* (two per op/attr) */
  g_object_get (editor_state,
                "attributes-visible", &vis,
                "attributes-suppress", &sup,
                NULL);
  if (vis && !sup) {
    num_attrib = g_list_model_get_n_items (dia_uml_class_get_attributes (editor_state));
  } else {
    num_attrib = 0;
  }
  g_object_get (editor_state,
                "operations-visible", &vis,
                "operations-suppress", &sup,
                NULL);
  if (vis && !sup) {
    num_ops = g_list_model_get_n_items (dia_uml_class_get_operations (editor_state));
  } else {
    num_ops = 0;
  }
  obj = &umlclass->element.object;
#ifdef UML_MAINPOINT
  obj->num_connections =
    UMLCLASS_CONNECTIONPOINTS + num_attrib*2 + num_ops*2 + 1;
#else
  obj->num_connections =
    UMLCLASS_CONNECTIONPOINTS + num_attrib*2 + num_ops*2;
#endif
  obj->connections =
    g_realloc(obj->connections,
	      obj->num_connections*sizeof(ConnectionPoint *));

  /* Read from dialog and put in object: */
  class_read_from_dialog(umlclass, prop_dialog);
  /* ^^^ attribs must be called before ops, to get the right order of the
     connectionpoints. */
  dia_uml_class_store (editor_state, umlclass, &added, &deleted, &disconnected);

  /* Reestablish mainpoint */
#ifdef UML_MAINPOINT
  obj->connections[obj->num_connections-1] =
    &umlclass->connections[UMLCLASS_CONNECTIONPOINTS];
#endif

  /* unconnect from all deleted connectionpoints. */
  list = deleted;
  while (list != NULL) {
    ConnectionPoint *connection = (ConnectionPoint *) list->data;

    _umlclass_store_disconnects (connection, &disconnected);
    object_remove_connections_to(connection);
    
    list = g_list_next(list);
  }
  
  /* Update data: */
  umlclass_calculate_data(umlclass);
  umlclass_update_data(umlclass);

  /* Fill in class with the new data: */
  fill_in_dialog(umlclass);
#ifdef DEBUG
  umlclass_sanity_check(umlclass, "Apply from dialog end");
#endif
  return new_umlclass_change(umlclass, old_state, added, deleted, disconnected);
}

GtkWidget *
umlclass_get_properties(UMLClass *umlclass, gboolean is_default)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *notebook;

#ifdef DEBUG
  umlclass_sanity_check(umlclass, "Get properties start");
#endif
  if (umlclass->properties_dialog == NULL) {
    prop_dialog = g_new(UMLClassDialog, 1);
    umlclass->properties_dialog = prop_dialog;
    
    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
     prop_dialog->dialog = notebook;

    g_object_set_data(G_OBJECT(notebook), "user_data", (gpointer) umlclass);
    
    g_signal_connect (G_OBJECT (umlclass->properties_dialog->dialog), "destroy",
		      G_CALLBACK(destroy_properties_dialog), umlclass);
    
    class_create_page(notebook, umlclass);
    style_create_page(notebook, umlclass);

    gtk_widget_show (notebook);
  }

  fill_in_dialog(umlclass);
  gtk_widget_show (umlclass->properties_dialog->dialog);

  return umlclass->properties_dialog->dialog;
}


/****************** UNDO stuff: ******************/

static void
umlclass_update_connectionpoints(UMLClass *umlclass)
{
  int num_attrib, num_ops;
  DiaObject *obj;
  GList *list;
  int connection_index;
  
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
#ifdef UML_MAINPOINT
  obj->num_connections = UMLCLASS_CONNECTIONPOINTS + num_attrib*2 + num_ops*2 + 1;
#else
  obj->num_connections = UMLCLASS_CONNECTIONPOINTS + num_attrib*2 + num_ops*2;
#endif
  obj->connections =
    g_realloc(obj->connections,
	      obj->num_connections*sizeof(ConnectionPoint *));

  connection_index = UMLCLASS_CONNECTIONPOINTS;
  
  list = umlclass->attributes;
  while (list != NULL) {
    DiaUmlAttribute *attr = (DiaUmlAttribute *) list->data;
    
    if ( (umlclass->visible_attributes) &&
	 (!umlclass->suppress_attributes)) {
      obj->connections[connection_index] = attr->left_connection;
      connection_index++;
      obj->connections[connection_index] = attr->right_connection;
      connection_index++;
    }
    
    list = g_list_next(list);
  }
  
  list = umlclass->operations;
  while (list != NULL) {
    DiaUmlOperation *op = (DiaUmlOperation *) list->data;
    
    if ( (umlclass->visible_operations) &&
	 (!umlclass->suppress_operations)) {
      obj->connections[connection_index] = op->l_connection;
      connection_index++;
      obj->connections[connection_index] = op->r_connection;
      connection_index++;
    }
    
    list = g_list_next(list);
  }

#ifdef UML_MAINPOINT
  obj->connections[connection_index++] = &umlclass->connections[UMLCLASS_CONNECTIONPOINTS];
#endif
  
}

static void
umlclass_set_state(UMLClass *umlclass, DiaUmlClass *state)
{
  g_object_unref (state);

  umlclass_update_connectionpoints(umlclass);

  umlclass_calculate_data(umlclass);
  umlclass_update_data(umlclass);
  /* TODO: Fix undo
  dia_uml_class_store (state, umlclass); */
}

static void
umlclass_change_apply(UMLClassChange *change, DiaObject *obj)
{
  DiaUmlClass *old_state;
  GList *list;
  
  old_state = dia_uml_class_new (change->obj);

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
  DiaUmlClass *old_state;
  GList *list;
  
  old_state = dia_uml_class_new(change->obj);

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

  g_object_unref (change->saved_state);

  /* Doesn't this mean only one of add, delete can be done in each apply? */
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
new_umlclass_change (UMLClass    *obj,
                     DiaUmlClass *saved_state,
                     GList       *added,
                     GList       *deleted,
                     GList       *disconnected)
{
  UMLClassChange *change;

  change = g_new0(UMLClassChange, 1);
  
  change->obj_change.apply = (ObjectChangeApplyFunc) umlclass_change_apply;
  change->obj_change.revert = (ObjectChangeRevertFunc) umlclass_change_revert;
  change->obj_change.free = (ObjectChangeFreeFunc) umlclass_change_free;

  change->obj = obj;
  change->saved_state = saved_state;
  change->applied = 1;

  change->added_cp = added;
  change->deleted_cp = deleted;
  change->disconnected = disconnected;

  return (ObjectChange *)change;
}
