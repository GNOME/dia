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
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

struct _DiaBuilder {
  GtkBuilder parent;
};

#define DIA_TYPE_BUILDER dia_builder_get_type()
G_DECLARE_FINAL_TYPE (DiaBuilder, dia_builder, DIA, BUILDER, GtkBuilder)


DiaBuilder *dia_builder_new                    (const char *path);
void        dia_builder_get                    (DiaBuilder *self,
                                                const char *first_object_name,
                                                ...);
void        dia_builder_connect                (DiaBuilder *self,
                                                gpointer    data,
                                                const char *first_signal_name,
                                                GCallback   first_signal_symbol,
                                                ...);
void        dia_builder_add_callback_symbol    (DiaBuilder *self,
                                                const char *callback_name,
                                                GCallback   callback_symbol);
GCallback   dia_builder_lookup_callback_symbol (DiaBuilder *self,
                                                const char *callback_name);
void        dia_builder_connect_signals        (DiaBuilder *self,
                                                gpointer    user_data);
char       *dia_builder_path                   (const char *name);

G_END_DECLS
