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

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TOOLBOX_MENU "/ToolboxMenu"
#define DISPLAY_MENU "/DisplayMenu"
#define INTEGRATED_MENU "/IntegratedUIMenu"
#define INVISIBLE_MENU "/InvisibleMenu"

struct zoom_pair {
  const char *string;
  const int   value;
};

extern const struct zoom_pair zooms[10];

void            integrated_ui_toolbar_set_zoom_text                      (GtkToolbar *toolbar,
                                                                          const char *text);
void            integrated_ui_toolbar_grid_snap_synchronize_to_display   (gpointer    ddisp);
void            integrated_ui_toolbar_object_snap_synchronize_to_display (gpointer    ddisp);
void            integrated_ui_toolbar_guides_snap_synchronize_to_display (gpointer    ddisp);

/* TODO: rename: menus_get_integrated_ui_menubar() */
void            menus_get_integrated_ui_menubar  (GtkWidget      **menubar,
                                                  GtkWidget      **toolbar,
                                                  GtkAccelGroup  **accel);
void            menus_get_toolbox_menubar        (GtkWidget      **menubar,
                                                  GtkAccelGroup  **accel);
GtkWidget     * menus_get_display_popup          (void);
GtkAccelGroup * menus_get_display_accels         (void);
GtkWidget *     menus_create_display_menubar     (GtkUIManager   **ui_manager,
                                                  GtkActionGroup **actions);

GtkActionGroup *menus_get_tool_actions           (void);
GtkActionGroup *menus_get_display_actions        (void);

GtkAction *     menus_get_action                 (const char      *name);
GtkWidget *     menus_get_widget                 (const char      *name);
void            menus_set_recent                 (GtkActionGroup  *actions);
void            menus_clear_recent               (void);

#define VIEW_MAIN_TOOLBAR_ACTION     "ViewMainToolbar"
#define VIEW_MAIN_STATUSBAR_ACTION   "ViewMainStatusbar"
#define VIEW_LAYERS_ACTION           "ViewLayers"

G_END_DECLS
