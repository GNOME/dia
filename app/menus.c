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
#ifdef GNOME
#include <gnome.h>
#endif
#include "intl.h"
#include "menus.h"
#include "commands.h"
#include "message.h"
/*#include "interface.h"*/
#include "display.h"

#if GNOME
static GnomeUIInfo toolbox_filemenu[] = {
  GNOMEUIINFO_MENU_NEW_ITEM(N_("_New diagram"), N_("Create new diagram"),
			    file_new_callback, NULL),
  GNOMEUIINFO_MENU_OPEN_ITEM(file_open_callback, NULL),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_EXIT_ITEM(file_quit_callback, NULL),
  GNOMEUIINFO_END  
};

static GnomeUIInfo filemenu[] = {
  GNOMEUIINFO_MENU_NEW_ITEM(N_("_New diagram"), N_("Create new diagram"),
			    file_new_callback, NULL),
  GNOMEUIINFO_MENU_OPEN_ITEM(file_open_callback, NULL),
  GNOMEUIINFO_MENU_SAVE_ITEM(file_save_callback, NULL),
  GNOMEUIINFO_MENU_PREFERENCES_ITEM(file_preferences_callback, NULL),
  GNOMEUIINFO_MENU_SAVE_AS_ITEM(file_save_as_callback, NULL),

  { GNOME_APP_UI_ITEM, "_Export To EPS", NULL,
    file_export_to_eps_callback, NULL, NULL },

  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_CLOSE_ITEM(file_close_callback, NULL),
  GNOMEUIINFO_MENU_EXIT_ITEM(file_quit_callback, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo editmenu[] = {
  GNOMEUIINFO_MENU_COPY_ITEM(edit_copy_callback, NULL),
  GNOMEUIINFO_MENU_CUT_ITEM(edit_cut_callback, NULL),
  GNOMEUIINFO_MENU_PASTE_ITEM(edit_paste_callback, NULL),
  GNOMEUIINFO_END
};

#define GNOMEUIINFO_ITEM_NONE_DATA(label, tooltip, callback, user_data) \
        { GNOME_APP_UI_ITEM, label, tooltip, callback, GINT_TO_POINTER(user_data), \
          NULL, GNOME_APP_PIXMAP_NONE, NULL, 0, (GdkModifierType) 0, NULL }

static GnomeUIInfo zoommenu[] = {
  GNOMEUIINFO_ITEM_NONE_DATA("400%",  NULL, view_zoom_set_callback, 4000),
  GNOMEUIINFO_ITEM_NONE_DATA("283%",  NULL, view_zoom_set_callback, 2828),
  GNOMEUIINFO_ITEM_NONE_DATA("200%",  NULL, view_zoom_set_callback, 2000),
  GNOMEUIINFO_ITEM_NONE_DATA("141%",  NULL, view_zoom_set_callback, 1414),
  GNOMEUIINFO_ITEM_NONE_DATA("100%",  NULL, view_zoom_set_callback, 1000),
  GNOMEUIINFO_ITEM_NONE_DATA("85%",   NULL, view_zoom_set_callback, 850),
  GNOMEUIINFO_ITEM_NONE_DATA("70.7%", NULL, view_zoom_set_callback, 707),
  GNOMEUIINFO_ITEM_NONE_DATA("50%",   NULL, view_zoom_set_callback, 500),
  GNOMEUIINFO_ITEM_NONE_DATA("35.4%", NULL, view_zoom_set_callback, 354),
  GNOMEUIINFO_ITEM_NONE_DATA("25%",   NULL, view_zoom_set_callback, 250),
  GNOMEUIINFO_END
};

static GnomeUIInfo viewmenu[] = {
  GNOMEUIINFO_ITEM_NONE(N_("Zoom _In"), N_("Zoom in 50%"), view_zoom_in_callback),
  GNOMEUIINFO_ITEM_NONE(N_("Zoom _Out"), N_("Zoom out 50%"), view_zoom_out_callback),
  GNOMEUIINFO_SUBTREE(N_("_Zoom"), zoommenu),
  GNOMEUIINFO_ITEM_NONE(N_("Edit Grid..."), NULL, view_edit_grid_callback),
  GNOMEUIINFO_TOGGLEITEM(N_("_Visible Grid"), NULL,
			 view_visible_grid_callback, NULL),
  GNOMEUIINFO_TOGGLEITEM(N_("_Snap To Grid"), NULL,
			 view_snap_to_grid_callback, NULL),
  GNOMEUIINFO_TOGGLEITEM(N_("Toggle _Rulers"), NULL,
			 view_toggle_rulers_callback, NULL),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_NONE(N_("New _View"), NULL, view_new_view_callback),
  GNOMEUIINFO_ITEM_NONE(N_("Show_All"), NULL, view_show_all_callback),
  GNOMEUIINFO_END
};

static GnomeUIInfo objects_align_h[] = {
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Left"), NULL, objects_align_h_callback, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Center"), NULL, objects_align_h_callback, 1),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Right"), NULL, objects_align_h_callback, 2),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Equal Distance"), NULL, objects_align_h_callback, 4),
  GNOMEUIINFO_END
};

static GnomeUIInfo objects_align_v[] = {
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Top"), NULL, objects_align_v_callback, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Center"), NULL, objects_align_v_callback, 1),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Bottom"), NULL, objects_align_v_callback, 2),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Equal Distance"), NULL, objects_align_v_callback, 4),
  GNOMEUIINFO_END
};

static GnomeUIInfo objectsmenu[] = {
  GNOMEUIINFO_ITEM_NONE(N_("Send to _Back"), NULL, objects_place_under_callback),
  GNOMEUIINFO_ITEM_NONE(N_("Bring to _Front"), NULL, objects_place_over_callback),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_NONE(N_("_Group"), NULL, objects_group_callback),
  GNOMEUIINFO_ITEM_NONE(N_("_Ungroup"), NULL, objects_ungroup_callback),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_SUBTREE(N_("Align _Horizontal"), objects_align_h),
  GNOMEUIINFO_SUBTREE(N_("Align _Vertical"), objects_align_v),
  GNOMEUIINFO_END
};

static GnomeUIInfo dialogsmenu[] = {
  GNOMEUIINFO_ITEM_NONE(N_("_Properties"), NULL, dialogs_properties_callback),
  GNOMEUIINFO_ITEM_NONE(N_("_Layers"), NULL, dialogs_layers_callback),
  GNOMEUIINFO_END
};

static GnomeUIInfo helpmenu[] = {
  GNOMEUIINFO_MENU_ABOUT_ITEM(help_about_callback, NULL),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_HELP("help-browser"),
  GNOMEUIINFO_END
};

static GnomeUIInfo toolbox_menu[] = {
  GNOMEUIINFO_SUBTREE(N_("File"), toolbox_filemenu),
  GNOMEUIINFO_SUBTREE(N_("Help"), helpmenu),
  GNOMEUIINFO_END
};

static GnomeUIInfo display_menu[] = {
  GNOMEUIINFO_MENU_FILE_TREE(filemenu),
  GNOMEUIINFO_MENU_EDIT_TREE(editmenu),
  GNOMEUIINFO_MENU_VIEW_TREE(viewmenu),
  GNOMEUIINFO_SUBTREE(N_("Objects"), objectsmenu),
  GNOMEUIINFO_SUBTREE(N_("Dialogs"), dialogsmenu),
  GNOMEUIINFO_END
};
#endif

static GtkItemFactoryEntry toolbox_menu_items[] =
{
  {N_("/_File"),               NULL,         NULL,                      0, "<Branch>"},
  {N_("/File/_New"),           "<control>N", file_new_callback,         0 },
  {N_("/File/_Open"),          "<control>O", file_open_callback,        0 },
  {N_("/File/_Preferences..."),NULL,         file_preferences_callback, 0 },
  {N_("/File/sep1"),           NULL,         NULL,                      0, "<Separator>"},
  {N_("/File/_Quit"),          "<control>Q", file_quit_callback,        0 },
  {N_("/_Help"),               NULL,         NULL,                      0, "<LastBranch>" },
  {N_("/Help/_About"),         NULL,         help_about_callback,       0 },
};

static GtkItemFactoryEntry display_menu_items[] =
{
  {N_("/_File"),                  NULL,         NULL,                        0, "<Branch>"},
  /*  {"/File/tearoff1 ",         NULL,         tearoff,                     0, "<Tearoff>" }, */
  {N_("/File/_New"),              "<control>N", file_new_callback,           0},
  {N_("/File/_Open"),             "<control>O", file_open_callback,          0},
  {N_("/File/_Save"),             "<control>S", file_save_callback,          0},
  {N_("/File/Save _As..."),       "<control>W", file_save_as_callback,       0},
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
  {N_("/View/New _View"),         "<control>I", view_new_view_callback,      0},
  {N_("/View/Show _All"),         "<control>A", view_show_all_callback,      0},
  {N_("/_Objects"),               NULL,         NULL,                        0, "<Branch>"},
  /*  {"/Objects/tearoff1 ",      NULL,         tearoff,                     0, "<Tearoff>" }, */
  {N_("/Objects/Send to _Back"),  "<control>B", objects_place_under_callback,0},
  {N_("/Objects/Bring to _Front"),"<control>F", objects_place_over_callback, 0},
  {N_("/Objects/sep1"),           NULL,         NULL,                        0, "<Separator>"},
  {N_("/Objects/_Group"),         "<control>G", objects_group_callback,      0},
  {N_("/Objects/_Ungroup"),       "<control>U", objects_ungroup_callback,    0},    
  {N_("/Objects/sep1"),           NULL,         NULL,                        0, "<Separator>"},
  {N_("/Objects/Align _Horizontal"),       NULL, NULL,                        0, "<Branch>"},
  {N_("/Objects/Align Horizontal/Left"),   NULL, objects_align_h_callback,    0},
  {N_("/Objects/Align Horizontal/Center"), NULL, objects_align_h_callback,    1},
  {N_("/Objects/Align Horizontal/Right"),  NULL, objects_align_h_callback,    2},
  {N_("/Objects/Align Horizontal/Equal Distance"), NULL, objects_align_h_callback,    4},
  {N_("/Objects/Align _Vertical"),         NULL, NULL,                        0, "<Branch>"},
  {N_("/Objects/Align Vertical/Top"),      NULL, objects_align_v_callback,    0},
  {N_("/Objects/Align Vertical/Center"),   NULL, objects_align_v_callback,    1},
  {N_("/Objects/Align Vertical/Bottom"),   NULL, objects_align_v_callback,    2},
  {N_("/Objects/Align Vertical/Equal Distance"),   NULL, objects_align_v_callback,    4},

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
gnome_toolbox_menus_create(GtkWidget* app)
{
  gnome_app_create_menus(GNOME_APP(app), toolbox_menu);
}

GtkWidget *
gnome_display_menus_create()
{
  return gnome_popup_menu_new(display_menu);
}
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

static GtkItemFactoryEntry *
translate_entries (const GtkItemFactoryEntry *entries, gint n)
{
  gint i;
  GtkItemFactoryEntry *ret;

  ret=g_malloc(sizeof(GtkItemFactoryEntry) * n);
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


GtkWidget *menus_get_item_from_path (char *path)
{
  GtkWidget *widget;

# ifdef GNOME
  gint pos;
  GtkWidget *parentw;
  GtkMenuShell *parent;

  /* drop the <Display>/ at the start */
  if (! (path = strchr (path, '/'))) return NULL;
  path ++;
  parentw = gnome_app_find_menu_pos (ddisplay_active ()->popup, path, &pos);
  if (! parentw)
    return NULL;

  parent = GTK_MENU_SHELL (parentw);
  widget = (GtkWidget *) g_list_nth (parent->children, pos-1)->data;

# else

  widget = gtk_item_factory_get_widget(display_item_factory, path);
  if (widget == NULL)
    widget = gtk_item_factory_get_widget(toolbox_item_factory, path);

# endif

  return widget;
}

