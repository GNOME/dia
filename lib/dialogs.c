/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dialogs.c: helper routines for creating simple dialogs
 * Copyright (C) 2002 Lars Clausen
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

#include "intl.h"
#include <gtk/gtk.h>
#include "geometry.h"
#include "dialogs.h"

/* Functions to create dialog widgets.  These should, if used in other
   dialogs, go into a separate file.  They are intended for fairly small
   transient dialogs such as parameters for exports.  The functions are
   meant to be transparent in that they don't make their own structures,
   they're just collections of common actions.
*/

/**
 * dialog_make:
 * @title: dialog title
 * @okay_text: label for @okay_button
 * @cancel_text: label of @cancel_button
 * @okay_button: (out): the accept #GtkButton
 * @cancel_button: (out): the reject #GtkButton
 *
 * Creates a new dialog with a title and Ok and Cancel buttons.
 *
 * Default texts are supplied for Ok and Cancel buttons if %NULL.
 *
 * This function does not call gtk_widget_show(), do
 * gtk_widget_show_all() when all has been added.
 *
 * Returns: the created dialog and sets the two widget pointers.
 */
GtkWidget *
dialog_make (char       *title,
             char       *okay_text,
             char       *cancel_text,
             GtkWidget **okay_button,
             GtkWidget **cancel_button)
{
  GtkWidget *dialog = gtk_dialog_new ();
  GtkWidget *label = gtk_label_new (title);

  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG(dialog))), label);

  *okay_button = gtk_button_new_with_label ((okay_text != NULL ? okay_text : _("OK")));
  *cancel_button = gtk_button_new_with_label ((cancel_text != NULL ? cancel_text : _("Cancel")));

  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_action_area (GTK_DIALOG (dialog))),
                     *okay_button);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_action_area (GTK_DIALOG(dialog))),
                     *cancel_button);

  return dialog;
}

/**
 * dialog_add_spinbutton:
 * @dialog: the #GtkDialog
 * @title: the spin box label
 * @min: minimum value
 * @max: the maximum value
 * @decimals: how many digits
 *
 * Adds a spinbutton with an attached label to a dialog.
 * To get an integer spinbutton, give decimals as 0.
*/
GtkSpinButton *
dialog_add_spinbutton (GtkWidget *dialog,
                       char      *title,
                       real       min,
                       real       max,
                       real       decimals) {
  GtkAdjustment *limits =
    GTK_ADJUSTMENT (gtk_adjustment_new (10.0, min, max, 1.0, 10.0, 0));
  GtkWidget *box = gtk_hbox_new (FALSE, 10);
  GtkWidget *label = gtk_label_new (title);
  GtkWidget *entry = gtk_spin_button_new (limits, 10.0, decimals);

  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), entry, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), box);

  return GTK_SPIN_BUTTON (entry);
}

