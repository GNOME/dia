/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dialinechooser.h -- Copyright (C) 1999 James Henstridge.
 *                     Copyright (C) 2004 Hubert Figuiere
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
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

#include "dia-line-style-selector.h"

G_BEGIN_DECLS

#define DIA_TYPE_LINE_CHOOSER dia_line_chooser_get_type ()
G_DECLARE_FINAL_TYPE (DiaLineChooser, dia_line_chooser, DIA, LINE_CHOOSER, GtkButton)

typedef void (*DiaChangeLineCallback) (DiaLineStyle lstyle,
                                       double       dash_length,
                                       gpointer     user_data);

GtkWidget *dia_line_chooser_new                  (DiaChangeLineCallback  callback,
                                                  gpointer               user_data);
void       dia_line_chooser_set_line_style       (DiaLineChooser        *lchooser,
                                                  DiaLineStyle           style,
                                                  double                 dashlength);

G_END_DECLS
