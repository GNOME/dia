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
#include "tool.h"
#include "commands.h"
#include "message.h"
#include "interface.h"
#include "display.h"
#include "filedlg.h"
#include "select.h"

#if GNOME
static GnomeUIInfo toolbox_filemenu[] = {
  GNOMEUIINFO_MENU_NEW_ITEM(N_("_New diagram"), N_("Create new diagram"),
			    file_new_callback, NULL),
  GNOMEUIINFO_MENU_OPEN_ITEM(file_open_callback, NULL),
  GNOMEUIINFO_MENU_PREFERENCES_ITEM(file_preferences_callback, NULL),
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

  { GNOME_APP_UI_ITEM, N_("_Export..."), NULL,
    file_export_callback, NULL, NULL },

  GNOMEUIINFO_SEPARATOR,
  { GNOME_APP_UI_ITEM, N_("Page Set_up..."), NULL,
    file_pagesetup_callback, NULL, NULL },
  { GNOME_APP_UI_ITEM, N_("_Print Diagram..."), NULL,
    file_print_callback, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PRINT,
    GDK_P, GDK_CONTROL_MASK},

  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_CLOSE_ITEM(file_close_callback, NULL),
  GNOMEUIINFO_MENU_EXIT_ITEM(file_quit_callback, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo editmenu[] = {
  GNOMEUIINFO_MENU_COPY_ITEM(edit_copy_callback, NULL),
  GNOMEUIINFO_MENU_CUT_ITEM(edit_cut_callback, NULL),
  GNOMEUIINFO_MENU_PASTE_ITEM(edit_paste_callback, NULL),
  { GNOME_APP_UI_ITEM, N_("_Delete"), NULL,
    edit_delete_callback, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, "Menu_Clear",
    GDK_D, GDK_CONTROL_MASK },
  GNOMEUIINFO_MENU_UNDO_ITEM(edit_undo_callback, NULL),
  GNOMEUIINFO_MENU_REDO_ITEM(edit_redo_callback, NULL),
  GNOMEUIINFO_ITEM_NONE(N_("Copy Text"), NULL, edit_copy_text_callback),
  GNOMEUIINFO_ITEM_NONE(N_("Cut Text"), NULL, edit_cut_text_callback),
  GNOMEUIINFO_ITEM_NONE(N_("Paste _Text"), NULL, edit_paste_text_callback),
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
#ifdef HAVE_LIBART  
  GNOMEUIINFO_TOGGLEITEM(N_("_AntiAliased"), NULL,
			 view_aa_callback, NULL),
#endif  
  GNOMEUIINFO_TOGGLEITEM(N_("_Visible Grid"), NULL,
			 view_visible_grid_callback, NULL),
  GNOMEUIINFO_TOGGLEITEM(N_("_Snap To Grid"), NULL,
			 view_snap_to_grid_callback, NULL),
  GNOMEUIINFO_TOGGLEITEM(N_("Show _Rulers"), NULL,
			 view_toggle_rulers_callback, NULL),
  GNOMEUIINFO_TOGGLEITEM(N_("Show _Connection Points"), NULL,
                         view_show_cx_pts_callback, NULL),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_ITEM_NONE(N_("New _View"), NULL, view_new_view_callback),
  GNOMEUIINFO_ITEM_NONE(N_("Show _All"), NULL, view_show_all_callback),
  GNOMEUIINFO_END
};

static GnomeUIInfo selectmenu[] = {
  GNOMEUIINFO_ITEM_NONE_DATA(N_("All"), NULL, select_all_callback, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("None"), NULL, select_none_callback, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Invert"), NULL, select_invert_callback, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Connected"), NULL, select_connected_callback, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Transitive"), NULL, select_transitive_callback, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Same Type"), NULL, select_same_type_callback, 0),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_TOGGLEITEM(N_("Replace"), NULL, select_style_callback, 0),
  GNOMEUIINFO_TOGGLEITEM(N_("Union"), NULL, select_style_callback, 1),
  GNOMEUIINFO_TOGGLEITEM(N_("Intersect"), NULL, select_style_callback, 2),
  GNOMEUIINFO_TOGGLEITEM(N_("Remove"), NULL, select_style_callback, 3),
  GNOMEUIINFO_TOGGLEITEM(N_("Invert"), NULL, select_style_callback, 4),
  GNOMEUIINFO_END
};

static GnomeUIInfo objects_align_h[] = {
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Left"), NULL, objects_align_h_callback, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Center"), NULL, objects_align_h_callback, 1),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Right"), NULL, objects_align_h_callback, 2),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Equal Distance"), NULL, objects_align_h_callback, 4),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Adjacent"), NULL, objects_align_h_callback, 5),
  GNOMEUIINFO_END
};

static GnomeUIInfo objects_align_v[] = {
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Top"), NULL, objects_align_v_callback, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Center"), NULL, objects_align_v_callback, 1),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Bottom"), NULL, objects_align_v_callback, 2),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Equal Distance"), NULL, objects_align_v_callback, 4),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Adjacent"), NULL, objects_align_h_callback, 5),
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

static GnomeUIInfo toolsmenu[] = {
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Modify"), NULL, NULL, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Magnify"), NULL, NULL, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Scroll"), NULL, NULL, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Text"), NULL, NULL, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Box"), NULL, NULL, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Ellipse"), NULL, NULL, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Polygon"), NULL, NULL, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Line"), NULL, NULL, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Arc"), NULL, NULL, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Zigzagline"), NULL, NULL, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Polyline"), NULL, NULL, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Bezierline"), NULL, NULL, 0),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Image"), NULL, NULL, 0),
  GNOMEUIINFO_END
};

static GnomeUIInfo dialogsmenu[] = {
  GNOMEUIINFO_ITEM_NONE(N_("_Properties"), NULL, dialogs_properties_callback),
  GNOMEUIINFO_ITEM_NONE(N_("_Layers"), NULL, dialogs_layers_callback),
  GNOMEUIINFO_END
};

static GnomeUIInfo helpmenu[] = {
  GNOMEUIINFO_MENU_ABOUT_ITEM(help_about_callback, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo toolbox_menu[] = {
  GNOMEUIINFO_MENU_FILE_TREE(toolbox_filemenu),
  GNOMEUIINFO_MENU_HELP_TREE(helpmenu),
  GNOMEUIINFO_END
};

static GnomeUIInfo display_menu[] = {
  GNOMEUIINFO_MENU_FILE_TREE(filemenu),
  GNOMEUIINFO_MENU_EDIT_TREE(editmenu),
  GNOMEUIINFO_MENU_VIEW_TREE(viewmenu),
  GNOMEUIINFO_SUBTREE(N_("Select"), selectmenu),
  GNOMEUIINFO_SUBTREE(N_("Objects"), objectsmenu),
  GNOMEUIINFO_SUBTREE(N_("Tools"), toolsmenu),
  GNOMEUIINFO_SUBTREE(N_("Dialogs"), dialogsmenu),
  GNOMEUIINFO_END
};

GtkWidget *toolbox_menubar = NULL;
GtkWidget *display_menubar = NULL;

#else /* !GNOME */

static GtkItemFactoryEntry toolbox_menu_items[] =
{
  {N_("/_File"),               NULL,         NULL,                      0, "<Branch>"},
  {N_("/File/_New"),           "<control>N", file_new_callback,         0 },
  {N_("/File/_Open"),          "<control>O", file_open_callback,        0 },
  {N_("/File/_Preferences..."),NULL,         file_preferences_callback, 0 },
  {N_("/File/sep1"),           NULL,         NULL,                      0, "<Separator>"},
  {N_("/File/_Quit"),          "<control>Q", file_quit_callback,        0 },
  {N_("/_Help"),               NULL,         NULL,                      0, "<Branch>" },
  {N_("/Help/_About"),         NULL,         help_about_callback,       0 },
};

static GtkItemFactoryEntry display_menu_items[] =
{
  {N_("/_File"),                  NULL,         NULL,                       0, "<Branch>"},
  /*  {"/File/tearoff1 ",         NULL,         tearoff,                    0, "<Tearoff>" }, */
  {N_("/File/_New"),              "<control>N", file_new_callback,          0},
  {N_("/File/_Open"),             "<control>O", file_open_callback,         0},
  {N_("/File/_Save"),             "<control>S", file_save_callback,         0},
  {N_("/File/Save _As..."),       "<control>W", file_save_as_callback,      0},
  {N_("/File/_Export..."),        NULL,         file_export_callback,       0},
  {N_("/File/sep1"),              NULL,         NULL,                        0, "<Separator>"},
  {N_("/File/Page Set_up..."),    NULL,         file_pagesetup_callback,    0},
  {N_("/File/_Print Diagram..."), "<control>P", file_print_callback,        0},
  {N_("/File/sep2"),              NULL,         NULL,                        0, "<Separator>"},
  {N_("/File/_Close"),            NULL,         file_close_callback,        0},
  {N_("/File/_Quit"),              "<control>Q", file_quit_callback,        0},
  {N_("/_Edit"),                  NULL,         NULL,                       0, "<Branch>"},
  /*  {"/Edit/tearoff1 ",         NULL,         tearoff,                    0, "<Tearoff>" }, */
  {N_("/Edit/_Copy"),             "<control>C", edit_copy_callback,         0},
  {N_("/Edit/C_ut"),              "<control>X", edit_cut_callback,          0},
  {N_("/Edit/_Paste"),            "<control>V", edit_paste_callback,        0},
  {N_("/Edit/_Delete"),           "<control>D", edit_delete_callback,       0},
  {N_("/Edit/_Undo"),             "<control>Z", edit_undo_callback,         0},
  {N_("/Edit/_Redo"),             "<control>R", edit_redo_callback,         0},
  {N_("/Edit/Copy Text"),         NULL,         edit_copy_text_callback,    0},
  {N_("/Edit/Cut Text"),          NULL,         edit_cut_text_callback,     0},
  {N_("/Edit/Paste _Text"),       NULL,         edit_paste_text_callback,   0},
  {N_("/_View"),                  NULL,         NULL,                       0, "<Branch>"},
  /*  {"/View/tearoff1 ",         NULL,         tearoff,                     0, "<Tearoff>" }, */
  {N_("/View/Zoom _In"),          NULL,          view_zoom_in_callback,     0},
  {N_("/View/Zoom _Out"),         NULL,          view_zoom_out_callback,    0},
  {N_("/View/_Zoom"),             NULL,         NULL,                       0, "<Branch>"},
  {N_("/View/Zoom/400%"),         NULL,         view_zoom_set_callback,  4000},
  {N_("/View/Zoom/283%"),         NULL,         view_zoom_set_callback,  2828},
  {N_("/View/Zoom/200%"),         NULL,         view_zoom_set_callback,  2000},
  {N_("/View/Zoom/141%"),         NULL,         view_zoom_set_callback,  1414},
  {N_("/View/Zoom/100%"),         "1",          view_zoom_set_callback,  1000},
  {N_("/View/Zoom/85%"),          NULL,         view_zoom_set_callback,   850},
  {N_("/View/Zoom/70.7%"),        NULL,         view_zoom_set_callback,   707},
  {N_("/View/Zoom/50%"),          NULL,         view_zoom_set_callback,   500},
  {N_("/View/Zoom/35.4%"),        NULL,         view_zoom_set_callback,   354},
  {N_("/View/Zoom/25%"),          NULL,         view_zoom_set_callback,   250},
  {N_("/View/Edit Grid..."),      NULL,         view_edit_grid_callback,    0},
#ifdef HAVE_LIBART  
  {N_("/View/_AntiAliased"),      NULL,         view_aa_callback,           0, "<CheckItem>"},
#endif
  {N_("/View/_Visible Grid"),     NULL,         view_visible_grid_callback, 0, "<CheckItem>"},
  {N_("/View/_Snap To Grid"),     "G",          view_snap_to_grid_callback, 0, "<CheckItem>"},
  {N_("/View/Show _Rulers"),      NULL,         view_toggle_rulers_callback,0, "<CheckItem>"},
  {N_("/View/Show _Connection Points"),	  NULL,		view_show_cx_pts_callback,   0,	"<CheckItem>"},
  {N_("/View/sep1"),              NULL,         NULL,                       0, "<Separator>"},
  {N_("/View/New _View"),         "<control>I", view_new_view_callback,     0},
  {N_("/View/Show _All"),         "<control>A", view_show_all_callback,     0},
  {N_("/Select/All"), NULL, select_all_callback, 0},
  {N_("/Select/None"), NULL, select_none_callback, 0},
  {N_("/Select/Invert"), NULL, select_invert_callback, 0},
  {N_("/Select/Connected"), NULL, select_connected_callback, 0},
  {N_("/Select/Transitive"), NULL, select_transitive_callback, 0},
  {N_("/Select/Same Type"), NULL, select_same_type_callback, 0},
  {N_("/Select/sep1"),           NULL,         NULL,                       0, "<Separator>"},
  {N_("/Select/Replace"), NULL, select_style_callback, 0, "<CheckItem>"},
  {N_("/Select/Union"), NULL, select_style_callback, 1, "<CheckItem>"},
  {N_("/Select/Intersect"), NULL, select_style_callback, 2, "<CheckItem>"},
  {N_("/Select/Remove"), NULL, select_style_callback, 3, "<CheckItem>"},
  {N_("/Select/Invert"), NULL, select_style_callback, 4, "<CheckItem>"},
  {N_("/_Objects"),               NULL,         NULL,                       0, "<Branch>"},
  /*  {"/Objects/tearoff1 ",      NULL,         tearoff,                    0, "<Tearoff>" }, */
  {N_("/Objects/Send to _Back"),  "<control>B", objects_place_under_callback,0},
  {N_("/Objects/Bring to _Front"),"<control>F", objects_place_over_callback,0},
  {N_("/Objects/sep1"),           NULL,         NULL,                       0, "<Separator>"},
  {N_("/Objects/_Group"),         "<control>G", objects_group_callback,     0},
  {N_("/Objects/_Ungroup"),       "<control>U", objects_ungroup_callback,   0},
  {N_("/Objects/sep1"),           NULL,         NULL,                       0, "<Separator>"},
  {N_("/Objects/Align _Horizontal"),       NULL, NULL,                      0, "<Branch>"},
  {N_("/Objects/Align Horizontal/Left"),   NULL, objects_align_h_callback,  0},
  {N_("/Objects/Align Horizontal/Center"), NULL, objects_align_h_callback,  1},
  {N_("/Objects/Align Horizontal/Right"),  NULL, objects_align_h_callback,  2},
  {N_("/Objects/Align Horizontal/Equal Distance"), NULL, objects_align_h_callback,    4},
  {N_("/Objects/Align Horizontal/Adjacent"), NULL, objects_align_h_callback,    5},
  {N_("/Objects/Align _Vertical"),         NULL, NULL,                      0, "<Branch>"},
  {N_("/Objects/Align Vertical/Top"),      NULL, objects_align_v_callback,  0},
  {N_("/Objects/Align Vertical/Center"),   NULL, objects_align_v_callback,  1},
  {N_("/Objects/Align Vertical/Bottom"),   NULL, objects_align_v_callback,  2},
  {N_("/Objects/Align Vertical/Equal Distance"),   NULL, objects_align_v_callback,    4},
  {N_("/Objects/Align Vertical/Adjacent"),   NULL, objects_align_v_callback,    5},

  {N_("/Tools/Modify"), NULL, NULL, 0},
  {N_("/Tools/Magnify"), NULL, NULL, 0},
  {N_("/Tools/Scroll"), NULL, NULL, 0},
  {N_("/Tools/Text"), NULL, NULL, 0},
  {N_("/Tools/Box"), NULL, NULL, 0},
  {N_("/Tools/Ellipse"), NULL, NULL, 0},
  {N_("/Tools/Polygon"), NULL, NULL, 0},
  {N_("/Tools/Line"), NULL, NULL, 0},
  {N_("/Tools/Arc"), NULL, NULL, 0},
  {N_("/Tools/Zigzagline"), NULL, NULL, 0},
  {N_("/Tools/Polyline"), NULL, NULL, 0},
  {N_("/Tools/Bezierline"), NULL, NULL, 0},
  {N_("/Tools/Image"), NULL, NULL, 0},

  /*  {"/Objects/tearoff1 ",      NULL,         tearoff,                     0, "<Tearoff>" }, */
  {N_("/_Dialogs"),               NULL,         NULL,                       0, "<Branch>"},
  {N_("/Dialogs/_Properties"),    NULL,         dialogs_properties_callback,0},
  {N_("/Dialogs/_Layers"),        NULL,         dialogs_layers_callback,0},
};

/* calculate the number of menu_item's */
static int toolbox_nmenu_items = sizeof(toolbox_menu_items) / sizeof(toolbox_menu_items[0]);
static int display_nmenu_items = sizeof(display_menu_items) / sizeof(display_menu_items[0]);

#endif /* !GNOME */

static GtkItemFactory *toolbox_item_factory = NULL;
static GtkItemFactory *display_item_factory = NULL;
static GtkAccelGroup *display_accel_group = NULL;

static void
tool_menu_select(GtkWidget *w, gpointer   data) {
  ToolButtonData *tooldata = (ToolButtonData *) data;

  if (tooldata == NULL) {
    g_warning(_("NULL tooldata in tool_menu_select"));
    return;
  }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tooldata->widget),TRUE);
}

#ifdef GNOME
void
gnome_toolbox_menus_create(GtkWidget* app)
{
  gnome_app_create_menus(GNOME_APP(app), toolbox_menu);
  toolbox_menubar = GNOME_APP(app)->menubar;
}

GtkWidget *
gnome_display_menus_create()
{
  GtkWidget *new_menu;
  GtkMenuItem *menuitem;
  GString *path;
  char *display = "<Display>";
  int i;

  new_menu = gnome_popup_menu_new(display_menu);

  path = g_string_new ("<Display>");

  /* There may be a slightly faster way to do this */

  for (i = 0; i < num_tools; i++) {
    g_string_append (g_string_assign(path, display),_("/Tools/"));
    g_string_append (path, tool_data[i].menuitem_name);
    menuitem = (GtkMenuItem *)menus_get_item_from_path(path->str);

    if (menuitem != NULL) {
      gtk_signal_connect (GTK_OBJECT(menuitem), "activate",
                          GTK_SIGNAL_FUNC (tool_menu_select),
                          &tool_data[i].callback_data);
    }    
  }

  return new_menu;
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

#ifndef GNOME
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
  translated_entries = translate_entries(toolbox_menu_items, 
					 toolbox_nmenu_items); 
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
  GtkWidget *new_menu;
  GtkMenuItem *menuitem;
  GString *path;
  char *display = "<Display>";
  GtkItemFactoryEntry *translated_entries;
  int i;

  if (display_item_factory == NULL) {
    display_accel_group = gtk_accel_group_new ();
    display_item_factory =  gtk_item_factory_new (GTK_TYPE_MENU, "<Display>", 
						  display_accel_group);
    translated_entries = translate_entries(display_menu_items, 
					   display_nmenu_items); 
    gtk_item_factory_create_items (display_item_factory,
				   display_nmenu_items,
				   translated_entries, NULL);
    free_translated_entries(translated_entries, display_nmenu_items);
  }
  
  *accel_group = display_accel_group;
  *menu = gtk_item_factory_get_widget (display_item_factory, "<Display>");

  path = g_string_new ("<Display>");

  /* There may be a slightly faster way to do this */

  for (i = 0; i < num_tools; i++) {
    g_string_append (g_string_assign(path, display),_("/Tools/"));
    g_string_append (path, tool_data[i].menuitem_name);
    menuitem = (GtkMenuItem *)menus_get_item_from_path(path->str);

    if (menuitem != NULL) {
      gtk_signal_connect (GTK_OBJECT(menuitem), "activate",
                          GTK_SIGNAL_FUNC (tool_menu_select),
                          &tool_data[i].callback_data);
    }    
  }
}
#endif /* !GNOME */

GtkWidget *menus_get_item_from_path (char *path)
{
  GtkWidget *widget = NULL;

# ifdef GNOME
  gint pos;
  char *menu_name;
  GtkWidget *parentw = NULL;
  GtkMenuShell *parent;
  DDisplay *ddisp;

  /* drop the "<Display>/" or "<Toolbox>/" at the start */
  menu_name = path;
  if (! (path = strchr (path, '/'))) return NULL;
  path ++; /* move past the / */

  if (strncmp(menu_name, "<Display>", strlen("<Display>")) == 0) {
    ddisp = ddisplay_active ();
    if (! ddisp)
      return NULL;
    
    parentw = gnome_app_find_menu_pos(ddisp->popup, path, &pos);
  } else if (strncmp(menu_name, "<Toolbox>", strlen("<Toolbox>")) == 0) {
    parentw = gnome_app_find_menu_pos(toolbox_menubar, path, &pos);
  } 
  if (! parentw) {
    g_warning("Can't find menu entry '%s'!\nThis is probably a i18n problem "
	      "(try LANG=C).", path);
    return NULL;
  }

  parent = GTK_MENU_SHELL (parentw);
  widget = (GtkWidget *) g_list_nth (parent->children, pos-1)->data;

# else

  if (display_item_factory) {
    widget = gtk_item_factory_get_widget(display_item_factory, path);
  }
  
  if (widget == NULL) {
    widget = gtk_item_factory_get_widget(toolbox_item_factory, path);
  }

  if (widget == NULL) {
     g_warning("Can't find menu entry '%s'!\nThis is probably a i18n problem "
	       "(try LANG=C).",
	       path);
  }

# endif

  return widget;
}

