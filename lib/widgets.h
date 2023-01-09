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

#pragma once

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "dia-enums.h"

G_BEGIN_DECLS

/* **** Util functions for Gtk stuff **** */

GtkWidget *dia_toggle_button_new_with_icon_names (const char *on,
                                                  const char *off);

GdkPixbuf *pixbuf_from_resource (const char *path);

G_END_DECLS
