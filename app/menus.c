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
#include <gtk/gtk.h>
#include <string.h>

#include "config.h"
#include "intl.h"
#ifdef GNOME
#include <gnome.h>
#endif
#include "menus.h"
#include "commands.h"
#include "message.h"

static GtkItemFactoryEntry toolbox_menu_items[] =
{
  {N_("/_File"),        NULL,         NULL,               0, "<Branch>"},
  {N_("/File/_New"),    "<control>N", file_new_callback,  0 },
  {N_("/File/_Open"),   "<control>O", file_open_callback, 0 },
  {N_("/File/sep1"),    NULL,         NULL,               0, "<Separator>"},
  {N_("/File/_Quit"),   "<control>Q", file_quit_callback, 0 },
  {N_("/_Help"),        NULL,         NULL,               0, "<LastBranch>" },
  {N_("/Help/_About"),  NULL,         help_about_callback,0 },
};

#if GNOME
static GnomeUIInfo filemenu[] = {
  {
    GNOME_APP_UI_ITEM, 
    N_("About"), N_("Info about this program"),
    help_about_callback, NULL, NULL, 
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT,
    0, 0, NULL
  },
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_HELP("help-browser"),
  GNOMEUIINFO_END
};

static GnomeUIInfo helpmenu[] = {
  {
    GNOME_APP_UI_ITEM, 
    N_("About"), N_("Info about this program"),
    help_about_callback, NULL, NULL, 
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT,
    0, 0, NULL
  },
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_HELP("help-browser"),
  GNOMEUIINFO_END
};

static GnomeUIInfo mainmenu[] = {
  GNOMEUIINFO_SUBTREE(N_("File"), filemenu),
  GNOMEUIINFO_SUBTREE(N_("Help"), helpmenu),
  GNOMEUIINFO_END
};
#endif

#if 0
static void
tearoff (gpointer             callback_data,
	 guint                callback_action,
	 GtkWidget           *widget)
{
  g_message ("ItemFactory: activated \"%s\"", gtk_item_factory_path_from_widget (widget));
}
#endif

static GtkItemFactoryEntry display_menu_items[] =
{
  {N_("/_File"),                  NULL,         NULL,                        0, "<Branch>"},
  /*  {"/File/tearoff1 ",         NULL,         tearoff,                     0, "<Tearoff>" }, */
  {N_("/File/_New"),              "<control>N", file_new_callback,           0},
  {N_("/File/_Open"),             "<control>O", file_open_callback,          0},
  {N_("/File/_Save"),             "<control>S", file_save_callback,          0},
  {N_("/File/Save _As..."),       NULL,         file_save_as_callback,       0},
  {N_("/File/_Export To EPS"),    NULL,         file_export_to_eps_callback, 0},
  {N_("/File/sep1"),              NULL,         NULL,                        0, "<Separator>"},
  {N_("/File/_Close"),            NULL,         file_close_callback,         0},
  {N_("/File/_Quit"),              "<control>Q", file_quit_callback,          0},
  {N_("/_Edit"),                  NULL,         NULL,                        0, "<Branch>"},
  /*  {"/Edit/tearoff1 ",         NULL,         tearoff,                     0, "<Tearoff>" }, */
  {N_("/Edit/_Copy"),             "<control>C", edit_copy_callback,          0},
  {N_("/Edit/C_ut"),              "<control>X", edit_cut_callback,           0},
  {N_("/Edit/_Paste"),            "<control>V", edit_paste_callback,         0},
  {N_("/Edit/_Delete"),           "<control>D", edit_delete_callback,        0},
  {N_("/_View"),                  NULL,         NULL,                        0, "<Branch>"},
  /*  {"/View/tearoff1 ",         NULL,         tearoff,                     0, "<Tearoff>" }, */
  {N_("/View/Zoom _In"),          NULL,          view_zoom_in_callback,       0},
  {N_("/View/Zoom _Out"),         NULL,          view_zoom_out_callback,      0},
  {N_("/View/_Zoom"),             NULL,         NULL,                        0, "<Branch>"},
  {N_("/View/Zoom/400%"),         NULL,         view_zoom_set_callback,      4000},
  {N_("/View/Zoom/283%"),         NULL,         view_zoom_set_callback,      2828},
  {N_("/View/Zoom/200%"),         NULL,         view_zoom_set_callback,      2000},
  {N_("/View/Zoom/141%"),         NULL,         view_zoom_set_callback,      1414},
  {N_("/View/Zoom/100%"),         NULL,         view_zoom_set_callback,      1000},
  {N_("/View/Zoom/85%"),          NULL,         view_zoom_set_callback,       850},
  {N_("/View/Zoom/70.7%"),        NULL,         view_zoom_set_callback,       707},
  {N_("/View/Zoom/50%"),          NULL,         view_zoom_set_callback,       500},
  {N_("/View/Zoom/35.4%"),        NULL,         view_zoom_set_callback,       354},
  {N_("/View/Zoom/25%"),          NULL,         view_zoom_set_callback,       250},
  {N_("/View/Edit Grid..."),      NULL,         view_edit_grid_callback,     0},
  {N_("/View/_Visible Grid"),     NULL,         view_visible_grid_callback,  0, "<CheckItem>"},
  {N_("/View/_Snap To Grid"),     NULL,         view_snap_to_grid_callback,  0, "<CheckItem>"},
  {N_("/View/Toggle _Rulers"),    NULL,         view_toggle_rulers_callback, 0, "<CheckItem>"},
  {N_("/View/sep1"),              NULL,         NULL,                        0, "<Separator>"},
  {N_("/View/New _View"),         NULL,         view_new_view_callback,      0},
  {N_("/View/Show _All"),         NULL,         view_show_all_callback,      0},
  {N_("/_Objects"),               NULL,         NULL,                        0, "<Branch>"},
  /*  {"/Objects/tearoff1 ",      NULL,         tearoff,                     0, "<Tearoff>" }, */
  {N_("/Objects/Place _Under"),   NULL,         objects_place_under_callback,0},
  {N_("/Objects/Place _Over"),    NULL,         objects_place_over_callback, 0},
  {N_("/Objects/sep1"),           NULL,         NULL,                        0, "<Separator>"},
  {N_("/Objects/_Group"),         NULL,         objects_group_callback,      0},
  {N_("/Objects/_Ungroup"),       NULL,         objects_ungroup_callback,    0},    
  {N_("/Objects/sep1"),           NULL,         NULL,                        0, "<Separator>"},
  {N_("/Objects/Align _Horizontal"),      NULL, NULL,                        0, "<Branch>"},
  {N_("/Objects/Align Horizontal/Top"),   NULL, objects_align_h_callback,    0},
  {N_("/Objects/Align Horizontal/Center"),NULL, objects_align_h_callback,    1},
  {N_("/Objects/Align Horizontal/Bottom"),NULL, objects_align_h_callback,    2},
  {N_("/Objects/Align _Vertical"),        NULL, NULL,                        0, "<Branch>"},
  {N_("/Objects/Align Vertical/Left"),    NULL, objects_align_v_callback,    0},
  {N_("/Objects/Align Vertical/Center"),  NULL, objects_align_v_callback,    1},
  {N_("/Objects/Align Vertical/Right"),   NULL, objects_align_v_callback,    2},

  /*  {"/Objects/tearoff1 ",      NULL,         tearoff,                     0, "<Tearoff>" }, */
  {N_("/_Dialogs"),               NULL,         NULL,                        0, "<Branch>"},
  {N_("/Dialogs/_Properties"),    NULL,         dialogs_properties_callback,0},
  {N_("/Dialogs/_Layers"),        NULL,         dialogs_layers_callback,0},
};

/* calculate the number of menu_item's */
static int toolbox_nmenu_items = sizeof(toolbox_menu_items) / sizeof(toolbox_menu_items[0]);
static int display_nmenu_items = sizeof(display_menu_items) / sizeof(display_menu_items[0]);

static GtkItemFactory *toolbox_item_factory = NULL;
static GtkItemFactory *display_item_factory = NULL;
static GtkAccelGroup *display_accel_group = NULL;

#ifdef GNOME
void
gnome_menus_create(GtkWidget* app)
{
  gnome_app_create_menus(GNOME_APP(app), mainmenu);
}
#endif

static GtkItemFactoryEntry *
translate_entries (const GtkItemFactoryEntry *entries, gint n)
{
  gint i;
  GtkItemFactoryEntry *ret;

  ret=g_malloc( sizeof(GtkItemFactoryEntry) * n );
  for (i=0; i<n; i++) {
    /* Translation. Note the explicit use of gettext(). */
    ret[i].path=g_strdup( gettext(entries[i].path) );
    /* accelerator and item_type are not duped, only referenced */
    ret[i].accelerator=entries[i].accelerator;
    ret[i].callback=entries[i].callback;
    ret[i].callback_action=entries[i].callback_action;
    ret[i].item_type=entries[i].item_type;
  }
  return ret;
}

static void 
free_translated_entries(GtkItemFactoryEntry *entries, gint n)
{
  gint i;

  for (i=0; i<n; i++)
    g_free(entries[i].path);
  g_free(entries);
}

void
menus_get_toolbox_menubar (GtkWidget **menubar,
			   GtkAccelGroup **accel_group)
{
  GtkItemFactoryEntry *translated_entries;

  *accel_group = gtk_accel_group_new ();
  toolbox_item_factory =
    gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<Toolbox>", *accel_group);
  translated_entries = translate_entries(toolbox_menu_items, toolbox_nmenu_items); 
  gtk_item_factory_create_items (toolbox_item_factory,
				 toolbox_nmenu_items,
				 translated_entries, NULL);
  free_translated_entries(translated_entries, toolbox_nmenu_items);

  *menubar = gtk_item_factory_get_widget (toolbox_item_factory, "<Toolbox>");
}
void
menus_get_image_menu (GtkWidget **menu,
		      GtkAccelGroup **accel_group)
{
  if (display_item_factory == NULL) {
    display_accel_group = gtk_accel_group_new ();
    display_item_factory =  gtk_item_factory_new (GTK_TYPE_MENU, "<Display>", display_accel_group);
    gtk_item_factory_create_items (display_item_factory,
				   display_nmenu_items,
				   display_menu_items, NULL);
  }
  
  *accel_group = display_accel_group;
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
    message_error(_("Unable to set sensitivity for menu which doesn't exist: %s"), path);
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
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), state);
  }  else {
    message_error(_("Unable to set state for menu which doesn't exist: %s"), path);
  }
}
