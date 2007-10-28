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

#define TOOLBOX_MENU "/ToolboxMenu"
#define DISPLAY_MENU "/DisplayMenu"
#define POPUP_MENU   "/PopupMenu"

struct zoom_pair { const gchar *string; const gint value; };

extern const struct zoom_pair zooms[10];

/* all the menu items that can be updated */
struct _UpdatableMenuItems 
{
  GtkAction *undo;
  GtkAction *redo;

  GtkAction *copy;
  GtkAction *cut;
  GtkAction *paste;
  GtkAction *edit_delete;
  GtkAction *edit_duplicate;
  GtkAction *copy_text;
  GtkAction *cut_text;
  GtkAction *paste_text;

  GtkAction *send_to_back;
  GtkAction *bring_to_front;
  GtkAction *send_backwards;
  GtkAction *bring_forwards;

  GtkAction *group;
  GtkAction *ungroup;

  GtkAction *parent;
  GtkAction *unparent;
  GtkAction *unparent_children;

  GtkAction *align_h_l;
  GtkAction *align_h_c;
  GtkAction *align_h_r;
  GtkAction *align_h_e;
  GtkAction *align_h_a;

  GtkAction *align_v_t;
  GtkAction *align_v_c;
  GtkAction *align_v_b;
  GtkAction *align_v_e;
  GtkAction *align_v_a;

  GtkAction *properties;
  
  GtkAction *select_all;
  GtkAction *select_none;
  GtkAction *select_invert;
  GtkAction *select_transitive;
  GtkAction *select_connected;
  GtkAction *select_same_type;

  GtkAction *select_replace;
  GtkAction *select_union;
  GtkAction *select_intersection;
  GtkAction *select_remove;
  GtkAction *select_inverse;
};

typedef struct _UpdatableMenuItems UpdatableMenuItems;

void 
integrated_ui_toolbar_set_zoom_text (GtkToolbar *toolbar, const gchar * text);

void 
integrated_ui_toolbar_grid_snap_synchronize_to_display (gpointer ddisp);

void
integrated_ui_toolbar_object_snap_synchronize_to_display (gpointer ddisp);

/* TODO: rename: menus_get_integrated_ui_menubar() */
void            menus_get_integrated_ui_menubar  (GtkWidget **menubar, GtkWidget **toolbar, 
                                                  GtkAccelGroup **accel);
void            menus_get_toolbox_menubar        (GtkWidget **menubar, GtkAccelGroup **accel);
GtkWidget     * menus_get_display_popup          (void);
GtkAccelGroup * menus_get_display_accels         (void);
GtkWidget *     menus_create_display_menubar     (GtkUIManager **ui_manager, GtkActionGroup **actions);
void            menus_initialize_updatable_items (UpdatableMenuItems *items, GtkActionGroup *actions);

GtkAccelGroup  * menus_get_accel_group  (void);
GtkActionGroup * menus_get_action_group (void);

GtkAction *     menus_get_action       (const gchar *name);
void            menus_set_recent       (GtkActionGroup *actions);
void            menus_clear_recent     (void);

#define VIEW_MAIN_TOOLBAR_ACTION     "ViewMainToolbar"
#define VIEW_MAIN_STATUSBAR_ACTION   "ViewMainStatusbar"

#endif /* MENUS_H */

