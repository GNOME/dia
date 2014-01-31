/* 
 * exit_dialog.h: Dialog to allow the user to choose which data to
 * save on exit or to cancel exit.
 *
 * Copyright (C) 2007 Patrick Hallinan
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

#ifndef EXIT_DIALOG_H
#define EXIT_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

enum {
  EXIT_DIALOG_EXIT_NO_SAVE,
  EXIT_DIALOG_EXIT_SAVE_SELECTED,
  EXIT_DIALOG_EXIT_CANCEL
};

GtkWidget *
exit_dialog_make (GtkWindow * parent_window,
                  gchar *     title);

void
exit_dialog_add_item (GtkWidget *    dialog,
                      const gchar *  name, 
                      const gchar *  filepath, 
                      const gpointer optional_data);

typedef struct 
{
  const gchar * name;
  const gchar * path;
  gpointer      data;

} exit_dialog_item_t;

typedef struct
{
  size_t               array_size;
  exit_dialog_item_t * array;

} exit_dialog_item_array_t;

gint
exit_dialog_run (GtkWidget * dialog,
                 exit_dialog_item_array_t ** items_to_save);

void exit_dialog_free_items (exit_dialog_item_array_t *);

G_END_DECLS

#endif

