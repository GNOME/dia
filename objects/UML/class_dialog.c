/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 * © 2023 Hubert Figuière <hub@figuiere.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "object.h"
#include "class.h"
#include "diaoptionmenu.h"

#include "class_dialog.h"

/* hide this functionality before rewrite;) */
void
umlclass_dialog_free (UMLClassDialog *dialog)
{
  g_list_free(dialog->deleted_connections);
  gtk_widget_destroy(dialog->dialog);
  /* destroy-signal destroy_properties_dialog already does 'g_clear_pointer (&dialog, g_free);' and more */
}

typedef struct _Disconnect {
  ConnectionPoint *cp;
  DiaObject *other_object;
  Handle *other_handle;
} Disconnect;

typedef struct _UMLClassState UMLClassState;

struct _UMLClassState {
  real font_height;
  real abstract_font_height;
  real polymorphic_font_height;
  real classname_font_height;
  real abstract_classname_font_height;
  real comment_font_height;

  DiaFont *normal_font;
  DiaFont *abstract_font;
  DiaFont *polymorphic_font;
  DiaFont *classname_font;
  DiaFont *abstract_classname_font;
  DiaFont *comment_font;

  char *name;
  char *stereotype;
  char *comment;

  int abstract;
  int suppress_attributes;
  int suppress_operations;
  int visible_attributes;
  int visible_operations;
  int visible_comments;

  int wrap_operations;
  int wrap_after_char;
  int comment_line_length;
  int comment_tagging;

  real line_width;
  Color line_color;
  Color fill_color;
  Color text_color;

  /* Attributes: */
  GList *attributes;

  /* Operators: */
  GList *operations;

  /* Template: */
  int template;
  GList *formal_params;
};


struct _DiaUMLClassObjectChange {
  DiaObjectChange obj_change;

  UMLClass *obj;

  GList *added_cp;
  GList *deleted_cp;
  GList *disconnected;

  int applied;

  UMLClassState *saved_state;
};


DIA_DEFINE_OBJECT_CHANGE (DiaUMLClassObjectChange,
                          dia_uml_class_object_change)


static UMLClassState   *umlclass_get_state  (UMLClass      *umlclass);
static DiaObjectChange *new_umlclass_change (UMLClass      *obj,
                                             UMLClassState *saved_state,
                                             GList         *added,
                                             GList         *deleted,
                                             GList         *disconnected);

/**** Utility functions ******/
void
_umlclass_store_disconnects(UMLClassDialog *prop_dialog,
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

/*
        get the contents of a comment text view.
*/
static gchar *
_class_get_comment (GtkTextView *view)
{
  GtkTextBuffer * buffer = gtk_text_view_get_buffer(view);
  GtkTextIter start;
  GtkTextIter end;

  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_end_iter (buffer, &end);

  return gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
}


static void
_class_set_comment (GtkTextView *view, gchar *text)
{
  GtkTextBuffer * buffer = gtk_text_view_get_buffer (view);

  gtk_text_buffer_set_text (buffer, text, -1);
}


static void
class_read_from_dialog(UMLClass *umlclass, UMLClassDialog *prop_dialog)
{
  const char *s;

  g_clear_pointer (&umlclass->name, g_free);

  s = gtk_entry_get_text (prop_dialog->classname);
  if (s && s[0])
    umlclass->name = g_strdup (s);
  else
    umlclass->name = NULL;

  g_clear_pointer (&umlclass->stereotype, g_free);

  s = gtk_entry_get_text(prop_dialog->stereotype);
  if (s && s[0])
    umlclass->stereotype = g_strdup (s);
  else
    umlclass->stereotype = NULL;

  g_clear_pointer (&umlclass->comment, g_free);

  s = _class_get_comment(prop_dialog->comment);
  if (s && s[0])
    umlclass->comment = g_strdup (s);
  else
    umlclass->comment = NULL;

  umlclass->abstract = gtk_toggle_button_get_active (prop_dialog->abstract_class);
  umlclass->visible_attributes = gtk_toggle_button_get_active (prop_dialog->attr_vis);
  umlclass->visible_operations = gtk_toggle_button_get_active (prop_dialog->op_vis);
  umlclass->wrap_operations = gtk_toggle_button_get_active (prop_dialog->op_wrap);
  umlclass->wrap_after_char = gtk_spin_button_get_value_as_int(prop_dialog->wrap_after_char);
  umlclass->comment_line_length = gtk_spin_button_get_value_as_int(prop_dialog->comment_line_length);
  umlclass->comment_tagging = gtk_toggle_button_get_active (prop_dialog->comment_tagging);
  umlclass->visible_comments = gtk_toggle_button_get_active (prop_dialog->comments_vis);
  umlclass->suppress_attributes = gtk_toggle_button_get_active (prop_dialog->attr_supp);
  umlclass->suppress_operations = gtk_toggle_button_get_active (prop_dialog->op_supp);
  umlclass->line_width = gtk_spin_button_get_value (prop_dialog->line_width);
  dia_colour_selector_get_colour (DIA_COLOUR_SELECTOR (prop_dialog->text_color),
                                  &umlclass->text_color);
  dia_colour_selector_get_colour (DIA_COLOUR_SELECTOR (prop_dialog->line_color),
                                  &umlclass->line_color);
  dia_colour_selector_get_colour (DIA_COLOUR_SELECTOR (prop_dialog->fill_color),
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

  if (umlclass->name)
    gtk_entry_set_text(prop_dialog->classname, umlclass->name);
  if (umlclass->stereotype != NULL)
    gtk_entry_set_text(prop_dialog->stereotype, umlclass->stereotype);
  else
    gtk_entry_set_text(prop_dialog->stereotype, "");

  if (umlclass->comment != NULL)
    _class_set_comment(prop_dialog->comment, umlclass->comment);
  else
    _class_set_comment(prop_dialog->comment, "");

  gtk_toggle_button_set_active(prop_dialog->abstract_class, umlclass->abstract);
  gtk_toggle_button_set_active(prop_dialog->attr_vis, umlclass->visible_attributes);
  gtk_toggle_button_set_active(prop_dialog->op_vis, umlclass->visible_operations);
  gtk_toggle_button_set_active(prop_dialog->op_wrap, umlclass->wrap_operations);
  gtk_spin_button_set_value (prop_dialog->wrap_after_char, umlclass->wrap_after_char);
  gtk_spin_button_set_value (prop_dialog->comment_line_length, umlclass->comment_line_length);
  gtk_toggle_button_set_active(prop_dialog->comment_tagging, umlclass->comment_tagging);
  gtk_toggle_button_set_active(prop_dialog->comments_vis, umlclass->visible_comments);
  gtk_toggle_button_set_active(prop_dialog->attr_supp, umlclass->suppress_attributes);
  gtk_toggle_button_set_active(prop_dialog->op_supp, umlclass->suppress_operations);
  gtk_spin_button_set_value (prop_dialog->line_width, umlclass->line_width);
  dia_colour_selector_set_colour (prop_dialog->text_color, &umlclass->text_color);
  dia_colour_selector_set_colour (prop_dialog->line_color, &umlclass->line_color);
  dia_colour_selector_set_colour (prop_dialog->fill_color, &umlclass->fill_color);
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
create_font_props_row (GtkGrid    *grid,
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
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (grid, label, 0, row, 1, 1);
  *fontsel = DIA_FONT_SELECTOR (dia_font_selector_new ());
  gtk_widget_set_hexpand (GTK_WIDGET (*fontsel), TRUE);
  dia_font_selector_set_font (DIA_FONT_SELECTOR (*fontsel), font);
  gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET(*fontsel), 1, row, 1, 1);

  adj = GTK_ADJUSTMENT (gtk_adjustment_new (height, 0.1, 10.0, 0.1, 1.0, 0));
  *heightsel = GTK_SPIN_BUTTON (gtk_spin_button_new (adj, 1.0, 2));
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (*heightsel), TRUE);
  gtk_grid_attach (grid, GTK_WIDGET (*heightsel), 2, row, 1, 1);
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
  GtkWidget *scrolledwindow;
  GtkWidget *checkbox;
  GtkWidget *grid;
  GtkAdjustment *adj;

  prop_dialog = umlclass->properties_dialog;

  /* Class page: */
  page_label = gtk_label_new_with_mnemonic (_("_Class"));

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);

  label = gtk_label_new(_("Class name:"));
  entry = gtk_entry_new();
  prop_dialog->classname = GTK_ENTRY(entry);
  gtk_widget_grab_focus(entry);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 0, 1, 1);
  gtk_widget_set_hexpand(entry, TRUE);

  label = gtk_label_new(_("Stereotype:"));
  entry = gtk_entry_new();
  prop_dialog->stereotype = GTK_ENTRY(entry);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 1, 1, 1);
  gtk_widget_set_hexpand(entry, TRUE);

  label = gtk_label_new(_("Comment:"));
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_grid_attach (GTK_GRID (grid), scrolledwindow, 1, 2, 1, 1);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
				       GTK_SHADOW_IN);
  entry = gtk_text_view_new ();
  prop_dialog->comment = GTK_TEXT_VIEW(entry);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), entry);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  checkbox = gtk_check_button_new_with_label(_("Abstract"));
  prop_dialog->abstract_class = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  checkbox = gtk_check_button_new_with_label(_("Attributes visible"));
  prop_dialog->attr_vis = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  checkbox = gtk_check_button_new_with_label(_("Suppress Attributes"));
  prop_dialog->attr_supp = GTK_TOGGLE_BUTTON( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  checkbox = gtk_check_button_new_with_label (_("Operations visible"));
  prop_dialog->op_vis = GTK_TOGGLE_BUTTON (checkbox);
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  checkbox = gtk_check_button_new_with_label (_("Suppress operations"));
  prop_dialog->op_supp = GTK_TOGGLE_BUTTON (checkbox);
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_set_homogeneous (GTK_BOX(hbox), TRUE);
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  checkbox = gtk_check_button_new_with_label (_("Wrap Operations"));
  prop_dialog->op_wrap = GTK_TOGGLE_BUTTON (checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  adj = GTK_ADJUSTMENT (gtk_adjustment_new (umlclass->wrap_after_char, 0.0, 200.0, 1.0, 5.0, 0));
  prop_dialog->wrap_after_char = GTK_SPIN_BUTTON (gtk_spin_button_new (adj, 0.1, 0));
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (prop_dialog->wrap_after_char), TRUE);
  gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (prop_dialog->wrap_after_char), TRUE);
  prop_dialog->max_length_label = GTK_LABEL (gtk_label_new( _("Wrap after this length: ")));
  gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET (prop_dialog->max_length_label), FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET (prop_dialog->wrap_after_char), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (hbox2), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_set_homogeneous (GTK_BOX(hbox), TRUE);
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  checkbox = gtk_check_button_new_with_label (_("Comments visible"));
  prop_dialog->comments_vis = GTK_TOGGLE_BUTTON ( checkbox );
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  adj = GTK_ADJUSTMENT (gtk_adjustment_new (umlclass->comment_line_length, 17.0, 200.0, 1.0, 5.0, 0));
  prop_dialog->comment_line_length = GTK_SPIN_BUTTON (gtk_spin_button_new(adj, 0.1, 0));
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (prop_dialog->comment_line_length), TRUE);
  gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (prop_dialog->comment_line_length), TRUE);
  prop_dialog->Comment_length_label = GTK_LABEL (gtk_label_new ( _("Wrap comment after this length: ")));
  gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET (prop_dialog->Comment_length_label), FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), GTK_WIDGET (prop_dialog->comment_line_length), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox),  GTK_WIDGET (hbox2), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox),  hbox, FALSE, TRUE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  checkbox = gtk_check_button_new_with_label (_("Show documentation tag"));
  prop_dialog->comment_tagging = GTK_TOGGLE_BUTTON (checkbox);
  gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page (notebook, vbox, page_label);

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
  GtkWidget *grid;
  GtkAdjustment *adj;

  prop_dialog = umlclass->properties_dialog;

  /** Fonts and Colors selection **/
  page_label = gtk_label_new_with_mnemonic (_("_Style"));

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, TRUE, 0);
  gtk_grid_set_row_homogeneous (GTK_GRID (grid), FALSE);
  gtk_grid_set_column_homogeneous (GTK_GRID (grid), FALSE);

  /* head line */
  label = gtk_label_new (_("Kind"));

  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  label = gtk_label_new (_("Font"));
  gtk_grid_attach (GTK_GRID (grid), label, 1, 0, 1, 1);
  label = gtk_label_new (_("Size"));
  gtk_grid_attach (GTK_GRID (grid), label, 2, 0, 1, 1);

  /* property rows */
  create_font_props_row (GTK_GRID (grid), _("Normal"), 1,
                         umlclass->normal_font,
                         umlclass->font_height,
                         &(prop_dialog->normal_font),
                         &(prop_dialog->normal_font_height));
  create_font_props_row (GTK_GRID (grid), _("Polymorphic"), 2,
                         umlclass->polymorphic_font,
                         umlclass->polymorphic_font_height,
                         &(prop_dialog->polymorphic_font),
                         &(prop_dialog->polymorphic_font_height));
  create_font_props_row (GTK_GRID (grid), _("Abstract"), 3,
                         umlclass->abstract_font,
                         umlclass->abstract_font_height,
                         &(prop_dialog->abstract_font),
                         &(prop_dialog->abstract_font_height));
  create_font_props_row (GTK_GRID (grid), _("Class Name"), 4,
                         umlclass->classname_font,
                         umlclass->classname_font_height,
                         &(prop_dialog->classname_font),
                         &(prop_dialog->classname_font_height));
  create_font_props_row (GTK_GRID (grid), _("Abstract Class"), 5,
                         umlclass->abstract_classname_font,
                         umlclass->abstract_classname_font_height,
                         &(prop_dialog->abstract_classname_font),
                         &(prop_dialog->abstract_classname_font_height));
  create_font_props_row (GTK_GRID (grid), _("Comment"), 6,
                         umlclass->comment_font,
                         umlclass->comment_font_height,
                         &(prop_dialog->comment_font),
                         &(prop_dialog->comment_font_height));



  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, TRUE, 0);

  /* should probably be refactored too. */
  label = gtk_label_new (_("Line Width"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_widget_set_hexpand(label, TRUE);
  adj = GTK_ADJUSTMENT (gtk_adjustment_new (umlclass->line_width, 0.0, G_MAXFLOAT, 0.1, 1.0, 0));
  line_width = gtk_spin_button_new (adj, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (line_width), TRUE);
  prop_dialog->line_width = GTK_SPIN_BUTTON (line_width);
  gtk_grid_attach (GTK_GRID (grid), line_width, 1, 0, 1, 1);
  gtk_widget_set_hexpand(line_width, FALSE);

  label = gtk_label_new (_("Text Color"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_widget_set_hexpand (label, TRUE);
  text_color = dia_colour_selector_new ();
  dia_colour_selector_set_use_alpha (DIA_COLOUR_SELECTOR (text_color), TRUE);
  dia_colour_selector_set_colour (DIA_COLOUR_SELECTOR (text_color), &umlclass->text_color);
  prop_dialog->text_color = DIA_COLOUR_SELECTOR (text_color);
  gtk_grid_attach (GTK_GRID (grid), text_color, 1, 1, 1, 1);
  gtk_widget_set_hexpand (text_color, FALSE);

  label = gtk_label_new(_("Foreground Color"));
  gtk_misc_set_alignment (GTK_MISC(label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
  gtk_widget_set_hexpand (label, TRUE);
  line_color = dia_colour_selector_new();
  dia_colour_selector_set_use_alpha (DIA_COLOUR_SELECTOR (line_color), TRUE);
  dia_colour_selector_set_colour (DIA_COLOUR_SELECTOR (line_color), &umlclass->line_color);
  prop_dialog->line_color = DIA_COLOUR_SELECTOR (line_color);
  gtk_grid_attach (GTK_GRID (grid), line_color, 1, 2, 1, 1);
  gtk_widget_set_hexpand (line_color, FALSE);

  label = gtk_label_new(_("Background Color"));
  gtk_misc_set_alignment (GTK_MISC(label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 3, 1, 1);
  gtk_widget_set_hexpand (label, TRUE);
  fill_color = dia_colour_selector_new();
  dia_colour_selector_set_colour (DIA_COLOUR_SELECTOR (fill_color), &umlclass->fill_color);
  prop_dialog->fill_color = DIA_COLOUR_SELECTOR (fill_color);
  gtk_grid_attach (GTK_GRID (grid), fill_color, 1, 3, 1, 1);
  gtk_widget_set_hexpand (fill_color, FALSE);

  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page(notebook, vbox, page_label);

}

/******************************************************
 ******************** ALL *****************************
 ******************************************************/


static void
destroy_properties_dialog (GtkWidget* widget,
                           gpointer   user_data)
{
  /* dialog gone, mark as such */
  UMLClass *umlclass = (UMLClass *) user_data;

  g_clear_pointer (&umlclass->properties_dialog, g_free);
}


static void
fill_in_dialog (UMLClass *umlclass)
{
#ifdef DEBUG
  umlclass_sanity_check(umlclass, "Filling in dialog before attrs");
#endif
  class_fill_in_dialog (umlclass);
  _attributes_fill_in_dialog (umlclass);
  _operations_fill_in_dialog (umlclass);
  _templates_fill_in_dialog (umlclass);
}


DiaObjectChange *
umlclass_apply_props_from_dialog (UMLClass *umlclass, GtkWidget *widget)
{
  UMLClassDialog *prop_dialog;
  DiaObject *obj;
  GList *list;
  int num_attrib, num_ops;
  GList *added, *deleted, *disconnected;
  UMLClassState *old_state = NULL;

#ifdef DEBUG
  umlclass_sanity_check(umlclass, "Apply from dialog start");
#endif

  prop_dialog = umlclass->properties_dialog;

  old_state = umlclass_get_state(umlclass);

  /* Allocate enought connection points for attributes and operations. */
  /* (two per op/attr) */
  if ( (gtk_toggle_button_get_active (prop_dialog->attr_vis)) && (!gtk_toggle_button_get_active (prop_dialog->attr_supp)))
    num_attrib =gtk_tree_model_iter_n_children (GTK_TREE_MODEL (prop_dialog->attributes_store), NULL);
  else
    num_attrib = 0;
  if (gtk_toggle_button_get_active (prop_dialog->op_vis) &&
      (!gtk_toggle_button_get_active (prop_dialog->op_supp))) {
    num_ops = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (prop_dialog->operations_store), NULL);
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
  obj->connections = g_renew (ConnectionPoint *,
                              obj->connections,
                              obj->num_connections);

  /* Read from dialog and put in object: */
  class_read_from_dialog(umlclass, prop_dialog);
  _attributes_read_from_dialog(umlclass, prop_dialog, UMLCLASS_CONNECTIONPOINTS);
  /* ^^^ attribs must be called before ops, to get the right order of the
     connectionpoints. */
  _operations_read_from_dialog(umlclass, prop_dialog, UMLCLASS_CONNECTIONPOINTS+num_attrib*2);
  _templates_read_from_dialog(umlclass, prop_dialog);

  /* Reestablish mainpoint */
#ifdef UML_MAINPOINT
  obj->connections[obj->num_connections-1] =
    &umlclass->connections[UMLCLASS_CONNECTIONPOINTS];
#endif

  /* unconnect from all deleted connectionpoints. */
  list = prop_dialog->deleted_connections;
  while (list != NULL) {
    ConnectionPoint *connection = (ConnectionPoint *) list->data;

    _umlclass_store_disconnects(prop_dialog, connection);
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
#ifdef DEBUG
  umlclass_sanity_check(umlclass, "Apply from dialog end");
#endif
  return  new_umlclass_change(umlclass, old_state, added, deleted, disconnected);
}

static void
create_dialog_pages(GtkNotebook *notebook, UMLClass *umlclass)
{
  class_create_page(notebook, umlclass);
  _attributes_create_page(notebook, umlclass);
  _operations_create_page(notebook, umlclass);
  _templates_create_page(notebook, umlclass);
  style_create_page(notebook, umlclass);
}

GtkWidget *
umlclass_get_properties(UMLClass *umlclass, gboolean is_default)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *vbox;
  GtkWidget *notebook;

#ifdef DEBUG
  umlclass_sanity_check(umlclass, "Get properties start");
#endif
  if (umlclass->properties_dialog == NULL) {
    prop_dialog = g_new0 (UMLClassDialog, 1);
    umlclass->properties_dialog = prop_dialog;

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    g_object_ref_sink(vbox);
    prop_dialog->dialog = vbox;

    prop_dialog->deleted_connections = NULL;
    prop_dialog->added_connections = NULL;
    prop_dialog->disconnected_connections = NULL;

    notebook = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_box_pack_start (GTK_BOX (vbox),	notebook, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (notebook), 10);

    g_object_set_data(G_OBJECT(notebook), "user_data", (gpointer) umlclass);

    create_dialog_pages(GTK_NOTEBOOK( notebook ), umlclass);

    g_signal_connect (G_OBJECT (umlclass->properties_dialog->dialog),
                      "destroy",
                      G_CALLBACK (destroy_properties_dialog),
                      umlclass);

    gtk_widget_show (notebook);
  }

  fill_in_dialog(umlclass);
  gtk_widget_show (umlclass->properties_dialog->dialog);

  return umlclass->properties_dialog->dialog;
}


/****************** UNDO stuff: ******************/

static void
umlclass_free_state (UMLClassState *state)
{
  g_clear_object (&state->normal_font);
  g_clear_object (&state->abstract_font);
  g_clear_object (&state->polymorphic_font);
  g_clear_object (&state->classname_font);
  g_clear_object (&state->abstract_classname_font);
  g_clear_object (&state->comment_font);

  g_clear_pointer (&state->name, g_free);
  g_clear_pointer (&state->stereotype, g_free);
  g_clear_pointer (&state->comment, g_free);

  g_list_free_full (state->attributes, (GDestroyNotify) uml_attribute_unref);
  g_list_free_full (state->operations, (GDestroyNotify) uml_operation_unref);
  g_list_free_full (state->formal_params, (GDestroyNotify) uml_formal_parameter_unref);
}

static UMLClassState *
umlclass_get_state(UMLClass *umlclass)
{
  UMLClassState *state = g_new0(UMLClassState, 1);
  GList *list;

  state->font_height = umlclass->font_height;
  state->abstract_font_height = umlclass->abstract_font_height;
  state->polymorphic_font_height = umlclass->polymorphic_font_height;
  state->classname_font_height = umlclass->classname_font_height;
  state->abstract_classname_font_height = umlclass->abstract_classname_font_height;
  state->comment_font_height = umlclass->comment_font_height;

  state->normal_font = g_object_ref (umlclass->normal_font);
  state->abstract_font = g_object_ref (umlclass->abstract_font);
  state->polymorphic_font = g_object_ref (umlclass->polymorphic_font);
  state->classname_font = g_object_ref (umlclass->classname_font);
  state->abstract_classname_font = g_object_ref (umlclass->abstract_classname_font);
  state->comment_font = g_object_ref (umlclass->comment_font);

  state->name = g_strdup(umlclass->name);
  state->stereotype = g_strdup(umlclass->stereotype);
  state->comment = g_strdup(umlclass->comment);

  state->abstract = umlclass->abstract;
  state->suppress_attributes = umlclass->suppress_attributes;
  state->suppress_operations = umlclass->suppress_operations;
  state->visible_attributes = umlclass->visible_attributes;
  state->visible_operations = umlclass->visible_operations;
  state->visible_comments = umlclass->visible_comments;

  state->wrap_operations = umlclass->wrap_operations;
  state->wrap_after_char = umlclass->wrap_after_char;
  state->comment_line_length = umlclass->comment_line_length;
  state->comment_tagging = umlclass->comment_tagging;

  state->line_color = umlclass->line_color;
  state->fill_color = umlclass->fill_color;
  state->text_color = umlclass->text_color;

  state->attributes = NULL;
  list = umlclass->attributes;
  while (list != NULL) {
    UMLAttribute *attr = (UMLAttribute *)list->data;
    UMLAttribute *attr_copy;

    attr_copy = uml_attribute_copy(attr);
    /* Looks wrong, but needed fro proper restore */
    attr_copy->left_connection = attr->left_connection;
    attr_copy->right_connection = attr->right_connection;

    state->attributes = g_list_append(state->attributes, attr_copy);
    list = g_list_next(list);
  }


  state->operations = NULL;
  list = umlclass->operations;
  while (list != NULL) {
    UMLOperation *op = (UMLOperation *)list->data;
    UMLOperation *op_copy;

    op_copy = uml_operation_copy(op);
    op_copy->left_connection = op->left_connection;
    op_copy->right_connection = op->right_connection;
    state->operations = g_list_append(state->operations, op_copy);
    list = g_list_next(list);
  }


  state->template = umlclass->template;

  state->formal_params = NULL;
  list = umlclass->formal_params;
  while (list != NULL) {
    UMLFormalParameter *param = (UMLFormalParameter *)list->data;
    UMLFormalParameter *param_copy;

    param_copy = uml_formal_parameter_copy(param);
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
#ifdef UML_MAINPOINT
  obj->num_connections = UMLCLASS_CONNECTIONPOINTS + num_attrib*2 + num_ops*2 + 1;
#else
  obj->num_connections = UMLCLASS_CONNECTIONPOINTS + num_attrib*2 + num_ops*2;
#endif
  obj->connections = g_renew (ConnectionPoint *,
                              obj->connections,
                              obj->num_connections);

  connection_index = UMLCLASS_CONNECTIONPOINTS;

  list = umlclass->attributes;
  while (list != NULL) {
    UMLAttribute *attr = (UMLAttribute *) list->data;

    if ( (umlclass->visible_attributes) &&
	 (!umlclass->suppress_attributes)) {
      obj->connections[connection_index] = attr->left_connection;
      connection_index++;
      obj->connections[connection_index] = attr->right_connection;
      connection_index++;
    }

    list = g_list_next(list);
  }

  if (prop_dialog) {
    gtk_list_store_clear (prop_dialog->attributes_store);
  }

  list = umlclass->operations;
  while (list != NULL) {
    UMLOperation *op = (UMLOperation *) list->data;

    if ( (umlclass->visible_operations) &&
	 (!umlclass->suppress_operations)) {
      obj->connections[connection_index] = op->left_connection;
      connection_index++;
      obj->connections[connection_index] = op->right_connection;
      connection_index++;
    }

    list = g_list_next(list);
  }

  if (prop_dialog) {
    gtk_list_store_clear (prop_dialog->operations_store);
  }

#ifdef UML_MAINPOINT
  obj->connections[connection_index++] = &umlclass->connections[UMLCLASS_CONNECTIONPOINTS];
#endif

}

static void
umlclass_set_state(UMLClass *umlclass, UMLClassState *state)
{
  umlclass->font_height = state->font_height;
  umlclass->abstract_font_height = state->abstract_font_height;
  umlclass->polymorphic_font_height = state->polymorphic_font_height;
  umlclass->classname_font_height = state->classname_font_height;
  umlclass->abstract_classname_font_height = state->abstract_classname_font_height;
  umlclass->comment_font_height = state->comment_font_height;

  /* transfer ownership, but don't leak the previous font */
  g_set_object (&umlclass->normal_font, state->normal_font);
  g_set_object (&umlclass->abstract_font, state->abstract_font);
  g_set_object (&umlclass->polymorphic_font, state->polymorphic_font);
  g_set_object (&umlclass->classname_font, state->classname_font);
  g_set_object (&umlclass->abstract_classname_font, state->abstract_classname_font);
  g_set_object (&umlclass->comment_font, state->comment_font);

  umlclass->name = state->name;
  umlclass->stereotype = state->stereotype;
  umlclass->comment = state->comment;

  umlclass->abstract = state->abstract;
  umlclass->suppress_attributes = state->suppress_attributes;
  umlclass->suppress_operations = state->suppress_operations;
  umlclass->visible_attributes = state->visible_attributes;
  umlclass->visible_operations = state->visible_operations;
  umlclass->visible_comments = state->visible_comments;

  umlclass->wrap_operations = state->wrap_operations;
  umlclass->wrap_after_char = state->wrap_after_char;
  umlclass->comment_line_length = state->comment_line_length;
  umlclass->comment_tagging = state->comment_tagging;

  umlclass->line_color = state->line_color;
  umlclass->fill_color = state->fill_color;
  umlclass->text_color = state->text_color;

  umlclass->attributes = state->attributes;
  umlclass->operations = state->operations;
  umlclass->template = state->template;
  umlclass->formal_params = state->formal_params;

  g_clear_pointer (&state, g_free);

  umlclass_update_connectionpoints(umlclass);

  umlclass_calculate_data(umlclass);
  umlclass_update_data(umlclass);
}


static void
dia_uml_class_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaUMLClassObjectChange *change = DIA_UML_CLASS_OBJECT_CHANGE (self);
  UMLClassState *old_state;
  GList *list;

  old_state = umlclass_get_state (change->obj);

  umlclass_set_state (change->obj, change->saved_state);

  list = change->disconnected;
  while (list) {
    Disconnect *dis = (Disconnect *) list->data;

    object_unconnect (dis->other_object, dis->other_handle);

    list = g_list_next (list);
  }

  change->saved_state = old_state;
  change->applied = 1;
}


static void
dia_uml_class_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaUMLClassObjectChange *change = DIA_UML_CLASS_OBJECT_CHANGE (self);
  UMLClassState *old_state;
  GList *list;

  old_state = umlclass_get_state (change->obj);

  umlclass_set_state (change->obj, change->saved_state);

  list = change->disconnected;
  while (list) {
    Disconnect *dis = (Disconnect *) list->data;

    object_connect (dis->other_object, dis->other_handle, dis->cp);

    list = g_list_next (list);
  }

  change->saved_state = old_state;
  change->applied = 0;
}


static void
dia_uml_class_object_change_free (DiaObjectChange *self)
{
  DiaUMLClassObjectChange *change = DIA_UML_CLASS_OBJECT_CHANGE (self);
  GList *list, *free_list;

  umlclass_free_state (change->saved_state);
  g_clear_pointer (&change->saved_state, g_free);

  /* Doesn't this mean only one of add, delete can be done in each apply? */
  if (change->applied) {
    free_list = change->deleted_cp;
  } else {
    free_list = change->added_cp;
  }

  list = free_list;
  while (list != NULL) {
    ConnectionPoint *connection = (ConnectionPoint *) list->data;

    g_return_if_fail (connection->connected == NULL); /* Paranoid */
    object_remove_connections_to (connection); /* Shouldn't be needed */
    g_clear_pointer (&connection, g_free);

    list = g_list_next (list);
  }

  g_list_free (free_list);
}


static DiaObjectChange *
new_umlclass_change (UMLClass      *obj,
                     UMLClassState *saved_state,
                     GList         *added,
                     GList         *deleted,
                     GList         *disconnected)
{
  DiaUMLClassObjectChange *change;

  change = dia_object_change_new (DIA_UML_TYPE_CLASS_OBJECT_CHANGE);

  change->obj = obj;
  change->saved_state = saved_state;
  change->applied = 1;

  change->added_cp = added;
  change->deleted_cp = deleted;
  change->disconnected = disconnected;

  return DIA_OBJECT_CHANGE (change);
}


char *
buffer_get_text (GtkTextBuffer *buffer)
{
  GtkTextIter start;
  GtkTextIter end;

  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_end_iter (buffer, &end);

  return gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
}
