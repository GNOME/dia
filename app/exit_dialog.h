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

#pragma once

#include <gtk/gtk.h>

#include "diagram.h"


G_BEGIN_DECLS


/**
 * DiaExitDialogResult:
 * @DIA_EXIT_DIALOG_SAVE: The selected diagrams should be saved, then quit
 * @DIA_EXIT_DIALOG_CANCEL: Don't quit, don't save
 * @DIA_EXIT_DIALOG_QUIT: Close anyway, loosing changes
 *
 * Since: 0.98
 */
typedef enum /*< enum,prefix=DIA >*/
{
  DIA_EXIT_DIALOG_SAVE,   /*< nick=save >*/
  DIA_EXIT_DIALOG_CANCEL, /*< nick=cancel >*/
  DIA_EXIT_DIALOG_QUIT,   /*< nick=quit >*/
} DiaExitDialogResult;


/**
 * DiaExitDialogItem:
 * @name: the name of the item, show in the dialogue
 * @path: the full path of the item, show in the tooltip
 * @data: the #Diagram itself
 */
typedef struct {
  char    *name;
  char    *path;
  Diagram *data;
} DiaExitDialogItem;


struct _DiaExitDialog {
  GObject parent;
};


#define DIA_TYPE_EXIT_DIALOG dia_exit_dialog_get_type ()
G_DECLARE_FINAL_TYPE (DiaExitDialog, dia_exit_dialog, DIA, EXIT_DIALOG, GObject)


DiaExitDialog       *dia_exit_dialog_new      (GtkWindow      *parent);
void                 dia_exit_dialog_add_item (DiaExitDialog  *self,
                                               const char     *name,
                                               const char     *filepath,
                                               Diagram        *diagram);
DiaExitDialogResult  dia_exit_dialog_run      (DiaExitDialog  *self,
                                               GPtrArray     **items);


G_END_DECLS
