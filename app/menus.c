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
#include <gtk/gtk.h>
#include <string.h>

#include "menus.h"
#include "commands.h"
#include "message.h"

static GtkItemFactoryEntry toolbox_menu_items[] =
{
  {"/_File",        NULL,         NULL,               0, "<Branch>"},
  {"/File/_New",    "<control>N", file_new_callback,  0 },
  {"/File/_Open",   "<control>O", file_open_callback, 0 },
  {"/File/sep1",    NULL,         NULL,               0, "<Separator>"},
  {"/File/_Quit",   "<control>Q", file_quit_callback, 0 },
  {"/_Help",        NULL,         NULL,               0, "<LastBranch>" },
  {"/Help/_About",  NULL,         NULL,               0 },
};

static void
tearoff (gpointer             callback_data,
	 guint                callback_action,
	 GtkWidget           *widget)
{
  g_message ("ItemFactory: activated \"%s\"", gtk_item_factory_path_from_widget (widget));
}

static GtkItemFactoryEntry display_menu_items[] =
{
  {"/_File",                  NULL,         NULL,                        0, "<Branch>"},
  /*  {"/File/tearoff1 ",         NULL,         tearoff,                     0, "<Tearoff>" }, */
  {"/File/_New",              "<control>N", file_new_callback,           0},
  {"/File/_Open",             "<control>O", file_open_callback,          0},
  {"/File/_Save",             "<control>S", file_save_callback,          0},
  {"/File/Save _As...",       NULL,         file_save_as_callback,       0},
  {"/File/_Export To EPS",    NULL,         file_export_to_eps_callback, 0},
  {"/File/sep1",              NULL,         NULL,                        0, "<Separator>"},
  {"/File/_Close",            NULL,         file_close_callback,         0},
  {"/File/_Quit",              "<control>Q", file_quit_callback,          0},
  {"/_Edit",                  NULL,         NULL,                        0, "<Branch>"},
  /*  {"/Edit/tearoff1 ",         NULL,         tearoff,                     0, "<Tearoff>" }, */
  {"/Edit/_Copy",             "<control>C", edit_copy_callback,          0},
  {"/Edit/C_ut",              "<control>X", edit_cut_callback,           0},
  {"/Edit/_Paste",            "<control>V", edit_paste_callback,         0},
  {"/Edit/_Delete",           "<control>D", edit_delete_callback,        0},
  {"/_View",                  NULL,         NULL,                        0, "<Branch>"},
  /*  {"/View/tearoff1 ",         NULL,         tearoff,                     0, "<Tearoff>" }, */
  {"/View/Zoom _In",          NULL,          view_zoom_in_callback,       0},
  {"/View/Zoom _Out",         NULL,          view_zoom_out_callback,      0},
  {"/View/_Zoom",             NULL,         NULL,                        0, "<Branch>"},
  {"/View/Zoom/400%",         NULL,         view_zoom_set_callback,      400},
  {"/View/Zoom/200%",         NULL,         view_zoom_set_callback,      200},
  {"/View/Zoom/100%",         NULL,         view_zoom_set_callback,      100},
  {"/View/Zoom/50%",          NULL,         view_zoom_set_callback,      50},
  {"/View/Zoom/25%",          NULL,         view_zoom_set_callback,      25},
  {"/View/Edit Grid...",      NULL,         view_edit_grid_callback,     0},
  {"/View/_Visible Grid",     NULL,         view_visible_grid_callback,  0, "<CheckItem>"},
  {"/View/_Snap To Grid",     NULL,         view_snap_to_grid_callback,  0, "<CheckItem>"},
  {"/View/Toggle _Rulers",    NULL,         view_toggle_rulers_callback, 0, "<CheckItem>"},
  {"/View/sep1",              NULL,         NULL,                        0, "<Separator>"},
  {"/View/New _View",         NULL,         view_new_view_callback,      0},
  {"/View/Show _All",         NULL,         view_show_all_callback,      0},
  {"/_Objects",               NULL,         NULL,                        0, "<Branch>"},
  /*  {"/Objects/tearoff1 ",      NULL,         tearoff,                     0, "<Tearoff>" }, */
  {"/Objects/Place _Under",   NULL,         objects_place_under_callback,0},
  {"/Objects/Place _Over",    NULL,         objects_place_over_callback, 0},
  {"/Objects/sep1",           NULL,         NULL,                        0, "<Separator>"},
  {"/Objects/_Group",         NULL,         objects_group_callback,      0},
  {"/Objects/_Ungroup",       NULL,         objects_ungroup_callback,    0},    
  {"/Objects/sep1",           NULL,         NULL,                        0, "<Separator>"},
  {"/Objects/Align _Horizontal",      NULL, NULL,                        0, "<Branch>"},
  {"/Objects/Align Horizontal/Top",   NULL, objects_align_h_callback,    0},
  {"/Objects/Align Horizontal/Center",NULL, objects_align_h_callback,    1},
  {"/Objects/Align Horizontal/Bottom",NULL, objects_align_h_callback,    2},
  {"/Objects/Align _Vertical",        NULL, NULL,                        0, "<Branch>"},
  {"/Objects/Align Vertical/Left",    NULL, objects_align_v_callback,    0},
  {"/Objects/Align Vertical/Center",  NULL, objects_align_v_callback,    1},
  {"/Objects/Align Vertical/Right",   NULL, objects_align_v_callback,    2},

  /*  {"/Objects/tearoff1 ",      NULL,         tearoff,                     0, "<Tearoff>" }, */
  {"/_Dialogs",               NULL,         NULL,                        0, "<Branch>"},
  {"/Dialogs/_Properties",    NULL,         dialogs_properties_callback,0},
  {"/Dialogs/_Layers",        NULL,         dialogs_layers_callback,0},
};

/* calculate the number of menu_item's */
static int toolbox_nmenu_items = sizeof(toolbox_menu_items) / sizeof(toolbox_menu_items[0]);
static int display_nmenu_items = sizeof(display_menu_items) / sizeof(display_menu_items[0]);

static GtkItemFactory *toolbox_item_factory = NULL;
static GtkItemFactory *display_item_factory = NULL;

void
menus_get_toolbox_menubar (GtkWidget **menubar,
			   GtkAccelGroup **accel_group)
{
  *accel_group = gtk_accel_group_new ();
  toolbox_item_factory =
    gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<Toolbox>", *accel_group);
  gtk_item_factory_create_items (toolbox_item_factory,
				 toolbox_nmenu_items,
				 toolbox_menu_items, NULL);
  
  *menubar = gtk_item_factory_get_widget (toolbox_item_factory, "<Toolbox>");
}
void
menus_get_image_menu (GtkWidget **menu,
		      GtkAccelGroup **accel_group)
{
  *accel_group = gtk_accel_group_new ();
  if (display_item_factory == NULL) {
    display_item_factory =  gtk_item_factory_new (GTK_TYPE_MENU, "<Display>", *accel_group);
    gtk_item_factory_create_items (display_item_factory,
				   display_nmenu_items,
				   display_menu_items, NULL);
  }
  
  *menu = gtk_item_factory_get_widget (display_item_factory, "<Display>");
}

void
menus_set_sensitive (char *path,
                     int   sensitive)
{
  GtkWidget *widget;
    
  widget = gtk_item_factory_get_widget(display_item_factory, path);
  if (widget == NULL)
    widget = gtk_item_factory_get_widget(toolbox_item_factory, path);

  if (widget != NULL) 
    gtk_widget_set_sensitive (widget, sensitive);
  else
    message_error("Unable to set sensitivity for menu which doesn't exist: %s", path);
}

void
menus_set_state (char *path,
                 int   state)
{
  GtkWidget *widget;
    
  widget = gtk_item_factory_get_widget(display_item_factory, path);
  if (widget == NULL)
    widget = gtk_item_factory_get_widget(toolbox_item_factory, path);

  if (widget != NULL) {
    if (GTK_IS_CHECK_MENU_ITEM (widget))
      gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (widget), state);
  }  else {
    message_error("Unable to set state for menu which doesn't exist: %s", path);
  }
}











