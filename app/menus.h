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
#ifndef MENUS_H
#define MENUS_H

#include <gtk/gtk.h>

#ifdef GNOME
#include <gnome.h>
#endif

struct zoom_pair { const gchar *string; const gint value; };

extern const struct zoom_pair zooms[10];

/* all the menu items that can be updated */
struct _UpdatableMenuItems 
{
  GtkWidget *copy;
  GtkWidget *cut;
  GtkWidget *paste;
#ifndef GNOME
  GtkWidget *edit_delete;
#endif
  GtkWidget *copy_text;
  GtkWidget *cut_text;
  GtkWidget *paste_text;

  GtkWidget *send_to_back;
  GtkWidget *bring_to_front;

  GtkWidget *group;
  GtkWidget *ungroup;

  GtkWidget *align_h_l;
  GtkWidget *align_h_c;
  GtkWidget *align_h_r;
  GtkWidget *align_h_e;
  GtkWidget *align_h_a;

  GtkWidget *align_v_t;
  GtkWidget *align_v_c;
  GtkWidget *align_v_b;
  GtkWidget *align_v_e;
  GtkWidget *align_v_a;
};

typedef struct _UpdatableMenuItems UpdatableMenuItems;

void menus_get_toolbox_menubar (GtkWidget **menubar, GtkAccelGroup **accel);
void menus_get_image_menu      (GtkWidget **menu,    GtkAccelGroup **accel);
void menus_get_image_menubar   (GtkWidget **menu, GtkItemFactory **display_mbar_item_factory);

GtkWidget *menus_get_item_from_path(char *path, GtkItemFactory *item_factory);
GtkWidget *menus_add_path           (const gchar *path);
void menus_initialize_updatable_items (UpdatableMenuItems *items, 
				       GtkItemFactory *factory, const char *display);


#endif /* MENUS_H */

