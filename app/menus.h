/* xxxxxx -- an diagram creation/manipulation program
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
#ifndef MENUS_H
#define MENUS_H

#include <gtk/gtk.h>

#ifdef GTK_HAVE_FEATURES_1_1_0
#define MY_GTK_ACCEL_TYPE GtkAccelGroup
#else
#define MY_GTK_ACCEL_TYPE GtkAcceleratorTable
#endif /* GTK_HAVE_FEATURES_1_1_0 */

extern void menus_get_toolbox_menubar (GtkWidget         **menubar,
				       MY_GTK_ACCEL_TYPE **accel);
extern void menus_get_image_menu (GtkWidget         **menu,
				  MY_GTK_ACCEL_TYPE **accel);

extern void menus_set_sensitive       (char                 *path,
				       int                   sensitive);

extern void menus_set_state (char *path, int   state);

#endif /* MENUS_H */

