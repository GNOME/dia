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

struct zoom_pair { const gchar *string; const gint value; };

extern const struct zoom_pair zooms[10];

/* all the menu items that can be updated */
struct _UpdatableMenuItems 
{
  GtkMenuItem *copy;
  GtkMenuItem *cut;
  GtkMenuItem *paste;
#ifndef GNOME_MENUS_ARE_BORING
  GtkMenuItem *edit_delete;
#endif
  GtkMenuItem *copy_text;
  GtkMenuItem *cut_text;
  GtkMenuItem *paste_text;

  GtkMenuItem *send_to_back;
  GtkMenuItem *bring_to_front;
  GtkMenuItem *send_backwards;
  GtkMenuItem *bring_forwards;

  GtkMenuItem *group;
  GtkMenuItem *ungroup;

  GtkMenuItem *parent;
  GtkMenuItem *unparent;
  GtkMenuItem *unparent_children;

  GtkMenuItem *align_h_l;
  GtkMenuItem *align_h_c;
  GtkMenuItem *align_h_r;
  GtkMenuItem *align_h_e;
  GtkMenuItem *align_h_a;

  GtkMenuItem *align_v_t;
  GtkMenuItem *align_v_c;
  GtkMenuItem *align_v_b;
  GtkMenuItem *align_v_e;
  GtkMenuItem *align_v_a;

  GtkMenuItem *properties;
};

typedef struct _UpdatableMenuItems UpdatableMenuItems;

void menus_get_toolbox_menubar (GtkWidget **menubar, GtkAccelGroup **accel);
void menus_get_image_menu      (GtkWidget **menu,    GtkAccelGroup **accel);
void menus_get_image_menubar   (GtkWidget **menu, GtkItemFactory **display_mbar_item_factory);

GtkMenuItem *menus_get_item_from_path(char *path, GtkItemFactory *item_factory);
GtkWidget *menus_add_path           (const gchar *path);
void menus_initialize_updatable_items (UpdatableMenuItems *items, 
				       GtkItemFactory *factory, const char *display);


#endif /* MENUS_H */

