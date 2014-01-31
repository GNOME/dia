/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * filedlg.c: some dialogs for saving/loading/exporting files.
 * Copyright (C) 1999 James Henstridge
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

#ifndef FILEDLG_H
#define FILEDLG_H

#include <gtk/gtk.h>

void file_open_callback(GtkAction *action);
void file_save_callback(GtkAction *action);
void file_save_as_callback(GtkAction *action);
void file_export_callback(GtkAction *action);

gboolean file_save_as(Diagram *dia, DDisplay *ddisp);

#endif
