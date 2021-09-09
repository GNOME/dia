/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Copyright (C) 2010 Hans Breuer
 * Replacement for deprecated GtkOptionMenu
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

G_BEGIN_DECLS


#define DIA_TYPE_OPTION_MENU dia_option_menu_get_type ()
G_DECLARE_DERIVABLE_TYPE (DiaOptionMenu, dia_option_menu, DIA, OPTION_MENU, GtkComboBox)

struct _DiaOptionMenuClass {
  GtkComboBoxClass parent;
};

GtkWidget *dia_option_menu_new        (void);
void       dia_option_menu_add_item   (DiaOptionMenu *self,
                                       const char    *name,
                                       int            value);
void       dia_option_menu_set_active (DiaOptionMenu *self,
                                       int            active);
int        dia_option_menu_get_active (DiaOptionMenu *self);

G_END_DECLS
