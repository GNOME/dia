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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

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
#include "plugin-manager.h"
#include "select.h"
#include "dia_dirs.h"
#include "diagram_tree_window.h"
#include "object_ops.h"
#include "sheets.h"

static void plugin_callback (GtkWidget *widget, gpointer data);

#ifdef GNOME

static GnomeUIInfo toolbox_filemenu[] = {
  GNOMEUIINFO_MENU_NEW_ITEM(N_("_New diagram"), N_("Create new diagram"),
			    file_new_callback, NULL),
  GNOMEUIINFO_MENU_OPEN_ITEM(file_open_callback, NULL),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_TOGGLEITEM(N_("_Diagram tree"), N_("Show diagram tree"),
			 diagtree_show_callback, NULL),
  GNOMEUIINFO_ITEM_NONE(N_("_Sheets and Objects..."),
                        N_("Modify sheets and their objects"),
                        sheets_dialog_show_callback),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_PREFERENCES_ITEM(file_preferences_callback, NULL),
  GNOMEUIINFO_ITEM_NONE(N_("P_lugins"), NULL, file_plugins_callback),
  GNOMEUIINFO_SEPARATOR,
    /* recent file list is dynamically inserted here */
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
        { GNOME_APP_UI_ITEM, label, tooltip, (gpointer)callback, GINT_TO_POINTER(user_data), \
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
  GNOMEUIINFO_ITEM_NONE(N_("Diagram Properties..."), NULL, view_diagram_properties_callback),
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
  GNOMEUIINFO_ITEM_NONE(N_("Redraw"), NULL, view_redraw_callback),
  GNOMEUIINFO_END
};

static GnomeUIInfo selecttype_radiolist[] = {
  GNOMEUIINFO_RADIOITEM_DATA(N_("Replace"), NULL, select_style_callback, 
			GUINT_TO_POINTER(SELECT_REPLACE), NULL),
  GNOMEUIINFO_RADIOITEM_DATA(N_("Union"), NULL, select_style_callback,
			GUINT_TO_POINTER(SELECT_UNION), NULL),
  GNOMEUIINFO_RADIOITEM_DATA(N_("Intersect"), NULL, select_style_callback,
			GUINT_TO_POINTER(SELECT_INTERSECTION), NULL),
  GNOMEUIINFO_RADIOITEM_DATA(N_("Remove"), NULL, select_style_callback, 
			GUINT_TO_POINTER(SELECT_REMOVE), NULL), 
  GNOMEUIINFO_RADIOITEM_DATA(N_("Invert"), NULL, select_style_callback, 
			GUINT_TO_POINTER(SELECT_INVERT), NULL),
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
  GNOMEUIINFO_RADIOLIST(selecttype_radiolist),
  GNOMEUIINFO_END
};

static GnomeUIInfo objects_align_h[] = {
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Left"), NULL, objects_align_h_callback, DIA_ALIGN_LEFT),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Center"), NULL, objects_align_h_callback, DIA_ALIGN_CENTER),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Right"), NULL, objects_align_h_callback, DIA_ALIGN_RIGHT),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Equal Distance"), NULL, objects_align_h_callback, DIA_ALIGN_EQUAL),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Adjacent"), NULL, objects_align_h_callback,DIA_ALIGN_ADJACENT),
  GNOMEUIINFO_END
};

static GnomeUIInfo objects_align_v[] = {
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Top"), NULL, objects_align_v_callback, DIA_ALIGN_TOP),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Center"), NULL, objects_align_v_callback, DIA_ALIGN_CENTER),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Bottom"), NULL, objects_align_v_callback, DIA_ALIGN_BOTTOM),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Equal Distance"), NULL, objects_align_v_callback, DIA_ALIGN_EQUAL),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Adjacent"), NULL, objects_align_h_callback, DIA_ALIGN_ADJACENT),
  GNOMEUIINFO_END
};

static GnomeUIInfo objectsmenu[] = {
  GNOMEUIINFO_ITEM_NONE(N_("Send to _Back"), NULL, objects_place_under_callback),
  GNOMEUIINFO_ITEM_NONE(N_("Bring to _Front"), NULL, objects_place_over_callback),
  GNOMEUIINFO_ITEM_NONE(N_("Send Backwards"), NULL, objects_place_down_callback),
  GNOMEUIINFO_ITEM_NONE(N_("Bring Forwards"), NULL, objects_place_up_callback),
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
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Beziergon"), NULL, NULL, 0),
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
  GNOMEUIINFO_HELP("dia"),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_ABOUT_ITEM(help_about_callback, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo toolbox_menu[] = {
  GNOMEUIINFO_MENU_FILE_TREE(toolbox_filemenu),
  GNOMEUIINFO_MENU_HELP_TREE(helpmenu),
  GNOMEUIINFO_END
};

static GnomeUIInfo inputmethod_menu[] = {
  GNOMEUIINFO_END
};


static GnomeUIInfo display_menu[] = {
  GNOMEUIINFO_MENU_FILE_TREE(filemenu),
  GNOMEUIINFO_MENU_EDIT_TREE(editmenu),
  GNOMEUIINFO_MENU_VIEW_TREE(viewmenu),
  GNOMEUIINFO_SUBTREE(N_("_Select"), selectmenu),
  GNOMEUIINFO_SUBTREE(N_("_Objects"), objectsmenu),
  GNOMEUIINFO_SUBTREE(N_("_Tools"), toolsmenu),
  GNOMEUIINFO_SUBTREE(N_("_Dialogs"), dialogsmenu),
  GNOMEUIINFO_SUBTREE(N_("_Input Methods"), inputmethod_menu),
  GNOMEUIINFO_END
};

#else /* !GNOME */

#define MRU_MENU_ENTRY_SIZE (strlen ("/File/MRU00 ") + 1)
#define MRU_MENU_ACCEL_SIZE sizeof ("<control>0")

static GtkItemFactoryEntry toolbox_menu_items[] =
{
  {N_("/_File"),               NULL,         NULL,       0,    "<Branch>" },
  {   "/File/tearoff",         NULL,         NULL,       0,   "<Tearoff>" },
  {N_("/File/_New"),           "<control>N", file_new_callback,         0,
      "<StockItem>", GTK_STOCK_NEW },
  {N_("/File/_Open"),          "<control>O", file_open_callback,        0,
      "<StockItem>", GTK_STOCK_OPEN },
  {N_("/File/---"),            NULL,         NULL,       0, "<Separator>" },
  {N_("/File/_Diagram tree"),  NULL,         diagtree_show_callback,    0,
   "<ToggleItem>" },
  {N_("/File/Sheets and Objects..."),
                               NULL,         sheets_dialog_show_callback, 0 },
  {N_("/File/---"),            NULL,         NULL,       0, "<Separator>" },
 {N_("/File/_Preferences..."),NULL,         file_preferences_callback, 0,
      "<StockItem>", GTK_STOCK_PREFERENCES },
   {N_("/File/P_lugins"),       NULL,         file_plugins_callback,     0 },
  {N_("/File/---"),            NULL,         NULL,       0, "<Separator>" },
    /* recent file list is dynamically inserted here */
  {N_("/File/---"),            NULL,         NULL,       0, "<Separator>" },
  {N_("/File/_Quit"),          "<control>Q", file_quit_callback,        0,
      "<StockItem>", GTK_STOCK_QUIT },
  {N_("/_Help"),               NULL,         NULL,       0,    "<Branch>" },
  {   "/Help/tearoff",         NULL,         NULL,       0,   "<Tearoff>" },
  {N_("/Help/_Manual"),        "F1",         help_manual_callback,      0,
      "<StockItem>", GTK_STOCK_HELP },
  {N_("/Help/---"),            NULL,         NULL,       0, "<Separator>" },
  {N_("/Help/_About"),         NULL,         help_about_callback,       0 },
};

/* calculate the number of menu_item's */
static int toolbox_nmenu_items = sizeof(toolbox_menu_items) / sizeof(toolbox_menu_items[0]);

/* this one is needed while we create the menubar in GTK instead of Gnome */
static GtkItemFactoryEntry display_menu_items[] =
{
  {   "/tearoff",                 NULL,         NULL,         0, "<Tearoff>" },
  {N_("/_File"),                  NULL,         NULL,           0, "<Branch>"},
  {   "/File/tearoff",            NULL,         NULL,         0, "<Tearoff>" },
  {N_("/File/_New"),              "<control>N", file_new_callback,          0,
      "<StockItem>", GTK_STOCK_NEW },
  {N_("/File/_Open"),             "<control>O", file_open_callback,         0,
      "<StockItem>", GTK_STOCK_OPEN },
  {N_("/File/_Save"),             "<control>S", file_save_callback,         0,
      "<StockItem>", GTK_STOCK_SAVE },
  {N_("/File/Save _As..."),       "<control>W", file_save_as_callback,      0,
      "<StockItem>", GTK_STOCK_SAVE_AS },
  {N_("/File/_Export..."),        NULL,         file_export_callback,       0},
  {N_("/File/---"),               NULL,         NULL,        0, "<Separator>"},
  {N_("/File/Page Set_up..."),    NULL,         file_pagesetup_callback,    0},
  {N_("/File/_Print Diagram..."), "<control>P", file_print_callback,        0,
      "<StockItem>", GTK_STOCK_PRINT },
  {N_("/File/---"),               NULL,         NULL,        0, "<Separator>"},
  {N_("/File/_Close"),            NULL,         file_close_callback,        0,
      "<StockItem>", GTK_STOCK_CLOSE },
  {   "/File/---MRU",             NULL,         NULL,        0, "<Separator>"},
  {N_("/File/_Quit"),              "<control>Q", file_quit_callback,        0,
      "<StockItem>", GTK_STOCK_QUIT},
  {N_("/_Edit"),                  NULL,         NULL,           0, "<Branch>"},
  {   "/Edit/tearoff",            NULL,         NULL,         0, "<Tearoff>" },
  {N_("/Edit/_Copy"),             "<control>C", edit_copy_callback,         0,
      "<StockItem>", GTK_STOCK_COPY },
  {N_("/Edit/C_ut"),              "<control>X", edit_cut_callback,          0,
      "<StockItem>", GTK_STOCK_CUT },
  {N_("/Edit/_Paste"),            "<control>V", edit_paste_callback,        0,
      "<StockItem>", GTK_STOCK_PASTE },
  {N_("/Edit/_Delete"),           "<control>D", edit_delete_callback,       0,
      "<StockItem>", GTK_STOCK_DELETE },
  {N_("/Edit/_Undo"),             "<control>Z", edit_undo_callback,         0,
      "<StockItem>", GTK_STOCK_UNDO },
  {N_("/Edit/_Redo"),             "<control>R", edit_redo_callback,         0,
      "<StockItem>", GTK_STOCK_REDO },
  {N_("/Edit/Copy Text"),         NULL,         edit_copy_text_callback,    0},
  {N_("/Edit/Cut Text"),          NULL,         edit_cut_text_callback,     0},
  {N_("/Edit/Paste _Text"),       NULL,         edit_paste_text_callback,   0},
  {N_("/_View"),                  NULL,         NULL,           0, "<Branch>"},
  {   "/View/tearoff",            NULL,         NULL,         0, "<Tearoff>" },
  {N_("/View/Zoom _In"),          NULL,          view_zoom_in_callback,     0,
      "<StockItem>", GTK_STOCK_ZOOM_IN },
  {N_("/View/Zoom _Out"),         NULL,          view_zoom_out_callback,    0,
      "<StockItem>", GTK_STOCK_ZOOM_OUT },
  {N_("/View/_Zoom"),             NULL,         NULL,           0, "<Branch>"},
  {   "/View/Zoom/tearoff",       NULL,         NULL,         0, "<Tearoff>" },
  {N_("/View/Zoom/400%"),         NULL,         view_zoom_set_callback,  4000},
  {N_("/View/Zoom/283%"),         NULL,         view_zoom_set_callback,  2828},
  {N_("/View/Zoom/200%"),         NULL,         view_zoom_set_callback,  2000},
  {N_("/View/Zoom/141%"),         NULL,         view_zoom_set_callback,  1414},
  {N_("/View/Zoom/100%"),         NULL,         view_zoom_set_callback,  1000,
      "<StockItem>", GTK_STOCK_ZOOM_100 },
  {N_("/View/Zoom/85%"),          NULL,         view_zoom_set_callback,   850},
  {N_("/View/Zoom/70.7%"),        NULL,         view_zoom_set_callback,   707},
  {N_("/View/Zoom/50%"),          NULL,         view_zoom_set_callback,   500},
  {N_("/View/Zoom/35.4%"),        NULL,         view_zoom_set_callback,   354},
  {N_("/View/Zoom/25%"),          NULL,         view_zoom_set_callback,   250},
  {N_("/View/Diagram Properties..."),NULL,         view_diagram_properties_callback, 0},
#ifdef HAVE_LIBART  
  {N_("/View/_AntiAliased"),      NULL,         view_aa_callback,           0, "<ToggleItem>"},
#endif
  {N_("/View/_Visible Grid"),     NULL,         view_visible_grid_callback, 0, "<ToggleItem>"},
  {N_("/View/_Snap To Grid"),     NULL,         view_snap_to_grid_callback, 0, "<ToggleItem>"},
  {N_("/View/Show _Rulers"),      NULL,         view_toggle_rulers_callback,0, "<ToggleItem>"},
  {N_("/View/Show _Connection Points"),	  NULL,		view_show_cx_pts_callback,   0,	"<ToggleItem>"},
  {N_("/View/---"),               NULL,         NULL,        0, "<Separator>"},
  {N_("/View/New _View"),         "<control>I", view_new_view_callback,     0},
  {N_("/View/Show _All"),         "<control>A", view_show_all_callback,     0},
  {N_("/View/Re_draw"),           NULL,         view_redraw_callback,       0},
  {N_("/_Select"),                NULL,         NULL,           0, "<Branch>"},
  {   "/Select/tearoff",          NULL,         NULL,         0, "<Tearoff>" },
  {N_("/Select/All"),             NULL,         select_all_callback,        0},
  {N_("/Select/None"),            NULL,         select_none_callback,       0},
  {N_("/Select/Invert"),          NULL,         select_invert_callback,     0},
  {N_("/Select/Connected"),       NULL,         select_connected_callback,  0},
  {N_("/Select/Transitive"),      NULL,         select_transitive_callback, 0},
  {N_("/Select/Same Type"),       NULL,         select_same_type_callback,  0},
  {N_("/Select/---"),             NULL,         NULL,        0, "<Separator>"},
  {N_("/Select/Replace"),       NULL, select_style_callback,
   SELECT_REPLACE, "<RadioItem>"},
  {N_("/Select/Union"),     NULL, select_style_callback,
   SELECT_UNION, "/Select/Replace"},
  {N_("/Select/Intersect"), NULL, select_style_callback,
   SELECT_INTERSECTION, "/Select/Replace"},
  {N_("/Select/Remove"),    NULL, select_style_callback,
   SELECT_REMOVE, "/Select/Replace"},
  {N_("/Select/Invert"),    NULL, select_style_callback,
   SELECT_INVERT, "/Select/Replace"},
  {N_("/_Objects"),               NULL,         NULL,           0, "<Branch>"},
  {   "/Objects/tearoff",         NULL,         NULL,         0, "<Tearoff>" },
  {N_("/Objects/Send to _Back"),  "<control>B",objects_place_under_callback,0},
  {N_("/Objects/Bring to _Front"),"<control>F", objects_place_over_callback,0},
  {N_("/Objects/Send Backwards"),  "<control><shift>B",objects_place_down_callback,0},
  {N_("/Objects/Bring Forwards"),"<control><shift>F", objects_place_up_callback,0},
  {N_("/Objects/---"),            NULL,         NULL,        0, "<Separator>"},
  {N_("/Objects/_Group"),         "<control>G", objects_group_callback,     0},
  {N_("/Objects/_Ungroup"),       "<control>U", objects_ungroup_callback,   0},
  {N_("/Objects/---"),            NULL,         NULL,        0, "<Separator>"},
  {N_("/Objects/Align _Horizontal"),       NULL, NULL,          0, "<Branch>"},
  {   "/Objects/Align Horizontal/tearoff", NULL, NULL,        0, "<Tearoff>" },
  {N_("/Objects/Align Horizontal/Left"),   NULL, objects_align_h_callback,  DIA_ALIGN_LEFT},
  {N_("/Objects/Align Horizontal/Center"), NULL, objects_align_h_callback,  DIA_ALIGN_CENTER},
  {N_("/Objects/Align Horizontal/Right"),  NULL, objects_align_h_callback,  DIA_ALIGN_RIGHT},
  {N_("/Objects/Align Horizontal/Equal Distance"), NULL, objects_align_h_callback,    DIA_ALIGN_EQUAL},
  {N_("/Objects/Align Horizontal/Adjacent"), NULL, objects_align_h_callback,    DIA_ALIGN_ADJACENT},
  {N_("/Objects/Align _Vertical"),         NULL, NULL,          0, "<Branch>"},
  {   "/Objects/Align Vertical/tearoff",   NULL, NULL,        0, "<Tearoff>" },
  {N_("/Objects/Align Vertical/Top"),      NULL, objects_align_v_callback,  DIA_ALIGN_TOP},
  {N_("/Objects/Align Vertical/Center"),   NULL, objects_align_v_callback,  DIA_ALIGN_CENTER},
  {N_("/Objects/Align Vertical/Bottom"),   NULL, objects_align_v_callback,  DIA_ALIGN_BOTTOM},
  {N_("/Objects/Align Vertical/Equal Distance"),   NULL, objects_align_v_callback,    DIA_ALIGN_EQUAL},
  {N_("/Objects/Align Vertical/Adjacent"), NULL, objects_align_v_callback,  DIA_ALIGN_ADJACENT},

  {N_("/_Tools"),                 NULL,     NULL,               0, "<Branch>"},
  {   "/Tools/tearoff",           NULL,         NULL,         0, "<Tearoff>" },
  {N_("/Tools/Modify"),           NULL,     NULL,                           0},
  {N_("/Tools/Magnify"),          NULL,     NULL,                           0},
  {N_("/Tools/Scroll"),           NULL,     NULL,                           0},
  {N_("/Tools/Text"),             NULL,     NULL,                           0},
  {N_("/Tools/Box"),              NULL,     NULL,                           0},
  {N_("/Tools/Ellipse"),          NULL,     NULL,                           0},
  {N_("/Tools/Polygon"),          NULL,     NULL,                           0},
  {N_("/Tools/Beziergon"),        NULL,     NULL,                           0},
  {N_("/Tools/Line"),             NULL,     NULL,                           0},
  {N_("/Tools/Arc"),              NULL,     NULL,                           0},
  {N_("/Tools/Zigzagline"),       NULL,     NULL,                           0},
  {N_("/Tools/Polyline"),         NULL,     NULL,                           0},
  {N_("/Tools/Bezierline"),       NULL,     NULL,                           0},
  {N_("/Tools/Image"),            NULL,     NULL,                           0},

  {N_("/_Dialogs"),               NULL,     NULL,               0, "<Branch>"},
  {   "/Dialogs/tearoff",         NULL,         NULL,         0, "<Tearoff>" },
  {N_("/Dialogs/_Properties"),    NULL,     dialogs_properties_callback,    0},
  {N_("/Dialogs/_Layers"),        NULL,     dialogs_layers_callback,        0},

  {N_("/_Input Methods"),         NULL,     NULL,               0, "<Branch>"},
  {   "/Input Methods/tearoff",   NULL,     NULL,               0, "<Tearoff>" },
};

static int display_nmenu_items = (sizeof(display_menu_items) /
				  sizeof(display_menu_items[0]));

#endif /* !GNOME */

/* need initialisation? */
static gboolean initialise = TRUE;

/* the actual menus */
static GtkWidget *toolbox_menubar = NULL;
static GtkAccelGroup *toolbox_accels = NULL;
static GtkWidget *display_menus = NULL;
static GtkAccelGroup *display_accels = NULL;

#ifdef GNOME
static GHashTable *toolbox_extras = NULL;
static GHashTable *display_extras = NULL;
#else
static GtkItemFactory *toolbox_item_factory = NULL;
static GtkItemFactory *display_item_factory = NULL;
#endif

#ifndef GNOME
#ifdef ENABLE_NLS

static gchar *
menu_translate (const gchar *path,
    		gpointer     data)
{
  static gchar *menupath = NULL;

  GtkItemFactory *item_factory = NULL;
  gchar *retval;
  gchar *factory;
  gchar *translation;
  gchar *domain = NULL;
  gchar *complete = NULL;
  gchar *p, *t;

  factory = (gchar *) data;

  if (menupath)
    g_free (menupath);

  retval = menupath = g_strdup (path);

  if ((strstr (path, "/tearoff1") != NULL) ||
      (strstr (path, "/---") != NULL) ||
      (strstr (path, "/MRU") != NULL))
    return retval;

  if (factory)
    item_factory = gtk_item_factory_from_path (factory);
  if (item_factory)
    {
      domain = gtk_object_get_data (GTK_OBJECT (item_factory), "textdomain");
      complete = gtk_object_get_data (GTK_OBJECT (item_factory), "complete");
    }
  
  if (domain)   /*  use the plugin textdomain  */
    {
      g_free (menupath);
      menupath = g_strconcat (factory, path, NULL);

      if (complete)
	{
	  /*  
           *  This is a branch, use the complete path for translation, 
	   *  then strip off entries from the end until it matches. 
	   */
	  complete = g_strconcat (factory, complete, NULL);
	  translation = g_strdup (dgettext (domain, complete));

	  while (*complete && *translation && strcmp (complete, menupath))
	    {
	      p = strrchr (complete, '/');
	      t = strrchr (translation, '/');
	      if (p && t)
		{
		  *p = '\0';
		  *t = '\0';
		}
	      else
		break;
	    }

	  g_free (complete);
	}
      else
	{
	  translation = dgettext (domain, menupath);
	}

      /* 
       * Work around a bug in GTK+ prior to 1.2.7 (similar workaround below)
       */
      if (strncmp (factory, translation, strlen (factory)) == 0)
	{
	  retval = translation + strlen (factory);
	  if (complete)
	    {
	      g_free (menupath);
	      menupath = translation;
	    }
	}
      else
	{
	  g_warning ("bad translation for menupath: %s", menupath);
	  retval = menupath + strlen (factory);
	  if (complete)
	    g_free (translation);
	}
    }
  else   /*  use the dia textdomain  */
    {
      if (complete)
	{
	  /*  
           *  This is a branch, use the complete path for translation, 
	   *  then strip off entries from the end until it matches. 
	   */
	  complete = g_strdup (complete);
	  translation = g_strdup (gettext (complete));
	  
	  while (*complete && *translation && strcmp (complete, menupath))
	    {
	      p = strrchr (complete, '/');
	      t = strrchr (translation, '/');
	      if (p && t)
		{
		  *p = '\0';
		  *t = '\0';
		}
	      else
		break;
	    }
	  g_free (complete);
	}
      else
	translation = gettext (menupath);

      if (*translation == '/')
	{
	  retval = translation;
	  if (complete)
	    {
	      g_free (menupath);
	      menupath = translation;
	    }
	}
      else
	{
	  g_warning ("bad translation for menupath: %s", menupath);
	  if (complete)
	    g_free (translation);
	}
    }
  
  return retval;
}

#endif  /*  ENABLE_NLS  */
#endif /* !GNOME */

static void
tool_menu_select(GtkWidget *w, gpointer   data) {
  ToolButtonData *tooldata = (ToolButtonData *) data;

  if (tooldata == NULL) {
    g_warning(_("NULL tooldata in tool_menu_select"));
    return;
  }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tooldata->widget),TRUE);
}

static gint
save_accels(gpointer data)
{
  gchar *accelfilename;

  accelfilename = dia_config_filename("menurc");
  if (accelfilename) {
    gtk_accel_map_save (accelfilename);
    g_free (accelfilename);
  }
  return TRUE;
}

#ifdef GNOME
/* create the GnomeUIBuilderData structure that connects the callbacks
 * so that they have the same signature as type 1 GtkItemFactory
 * callbacks */

static void
dia_menu_signal_proxy (GtkWidget *widget, GnomeUIInfo *uiinfo)
{
  GtkItemFactoryCallback1 signal_handler =
    (GtkItemFactoryCallback1)uiinfo->moreinfo;
  guint action = GPOINTER_TO_UINT (uiinfo->user_data);

  if (signal_handler) {
    (* signal_handler) (NULL, action, widget);
  } else {
    g_warning("called dia_menu_signal_proxy with NULL signal_handler !");
  }
}

static void
dia_menu_signal_connect_func (GnomeUIInfo *uiinfo, gchar *signal_name,
			     GnomeUIBuilderData *uibdata)
{
    /* only connect the signal if uiinfo->moreinfo not NULL */
    /* otherwise it is non sens and generate extraneous warnings */
    /* see bug 55047 <http://bugzilla.gnome.org/show_bug.cgi?id=55047> */
    if (uiinfo->moreinfo != NULL) {
        gtk_signal_connect (GTK_OBJECT(uiinfo->widget), signal_name,
                            GTK_SIGNAL_FUNC(dia_menu_signal_proxy), uiinfo);
    }
}

static GnomeUIBuilderData dia_menu_uibdata = {
  (GtkSignalFunc)dia_menu_signal_connect_func, /* connect_func */
  NULL,                         /* data */
  FALSE,                        /* is_interp */
  (GtkCallbackMarshal) 0,       /* relay_func */
  (GtkDestroyNotify) 0          /* destroy_func */
};
#endif

/*
  Purpose: set the generic callback for all the items in the menu "Tools"
 */
static void 
menus_set_tools_callback (const char * menu_name, GtkItemFactory *item_factory)
{
    gint i, len;
    GString *path;
    GtkWidget *menuitem;
    
    path = g_string_new(menu_name);
    g_string_append(path, "/Tools/");
    len = path->len;
    for (i = 0; i < num_tools; i++) {
	g_string_append(path, tool_data[i].menuitem_name);
	menuitem = menus_get_item_from_path(path->str, item_factory);
	if (menuitem != NULL)
	    gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			       GTK_SIGNAL_FUNC(tool_menu_select),
			       &tool_data[i].callback_data);
        else
            g_warning ("couldn't find tool menu item %s", path->str);
	g_string_truncate(path, len);
    }
    g_string_free(path, TRUE);
}


static void
menus_init(void)
{
#ifndef GNOME
  GtkItemFactoryEntry *translated_entries;
#else
  GtkWidget *menuitem;
#endif
  gchar *accelfilename;
  GList *cblist;

  if (!initialise)
    return;
  
  initialise = FALSE;

#ifdef GNOME
  /* the toolbox menu */
  toolbox_accels = gtk_accel_group_new();
  toolbox_menubar = gtk_menu_bar_new();
  gnome_app_fill_menu_custom(GTK_MENU_SHELL(toolbox_menubar), toolbox_menu,
			     &dia_menu_uibdata, toolbox_accels, TRUE, 0);

  /* the display menu */
  display_menus = gtk_menu_new();
  display_accels = gtk_accel_group_new();
  gtk_menu_set_accel_group(GTK_MENU(display_menus), display_accels);
  menuitem = gtk_tearoff_menu_item_new();
  gtk_menu_prepend(GTK_MENU(display_menus), menuitem);
  gnome_app_fill_menu_custom(GTK_MENU_SHELL(display_menus), display_menu,
			     &dia_menu_uibdata, display_accels, FALSE, 0);
  /* and the tearoff ... */
  menuitem = gtk_tearoff_menu_item_new();
  gtk_menu_prepend(GTK_MENU(display_menus), menuitem);
  gtk_widget_show(menuitem);

  /* initialise the extras arrays ... */
  toolbox_extras = g_hash_table_new(g_str_hash, g_str_equal);
  display_extras = g_hash_table_new(g_str_hash, g_str_equal);
#else
  /* the toolbox menu */
  toolbox_accels = gtk_accel_group_new();
  toolbox_item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<Toolbox>",
					      toolbox_accels);
  gtk_object_set_data (GTK_OBJECT (toolbox_item_factory), "factory_path",
		       (gpointer) "toolbox");
#ifdef ENABLE_NLS
  gtk_item_factory_set_translate_func (toolbox_item_factory, menu_translate,
				       "<Toolbox>", NULL);
#endif
  gtk_item_factory_create_items (toolbox_item_factory,
				 toolbox_nmenu_items,
				 toolbox_menu_items,
				 NULL);
  
  toolbox_menubar = gtk_item_factory_get_widget(toolbox_item_factory,
						"<Toolbox>");

  /* the display menu */
  display_accels = gtk_accel_group_new();
  display_item_factory = gtk_item_factory_new(GTK_TYPE_MENU, "<Display>",
					      display_accels);
#ifdef ENABLE_NLS
  gtk_item_factory_set_translate_func (display_item_factory, menu_translate,
				       "<Display>", NULL);
#endif
  gtk_item_factory_create_items (display_item_factory,
				 display_nmenu_items,
				 display_menu_items,
				 NULL);

  display_menus = gtk_item_factory_get_widget(display_item_factory,
					      "<Display>");
  gtk_menu_set_accel_path(GTK_MENU(display_menus), "<Display>/");
#endif

  menus_set_tools_callback ("<Display>", NULL);

  gtk_menu_set_title(GTK_MENU(display_menus), _("Diagram Menu"));
  
  /* initialize callbacks from plug-ins */
  for (cblist = filter_get_callbacks(); cblist; cblist = cblist->next) {
    DiaCallbackFilter *cbf = cblist->data;
    GtkWidget *newitem;

    newitem = menus_add_path(cbf->menupath);
    if (!newitem) {
      g_warning("Don't know where to add \"%s\" menu entry.", cbf->menupath);
      continue;
    }
    gtk_signal_connect(GTK_OBJECT(newitem), "activate",
		       GTK_SIGNAL_FUNC(plugin_callback), cbf);
  } /* for filter_callbacks */

  /* load accelerators and prepare to later save them */
  accelfilename = dia_config_filename("menurc");
  
  if (accelfilename) {
    gtk_accel_map_load(accelfilename);
    g_free(accelfilename);
  }
  gtk_quit_add(1, save_accels, NULL);

}

void
menus_get_toolbox_menubar (GtkWidget **menubar,
			   GtkAccelGroup **accel_group)
{
  if (initialise)
    menus_init();

  if (menubar)
    *menubar = toolbox_menubar;
  if (accel_group)
    *accel_group = toolbox_accels;
}

void
menus_get_image_menu (GtkWidget **menu,
		      GtkAccelGroup **accel_group)
{
  if (initialise)
    menus_init();

  if (menu)
    *menu = display_menus;
  if (accel_group)
    *accel_group = display_accels;
}

void
menus_get_image_menubar (GtkWidget **menu, GtkItemFactory **display_mbar_item_factory)
{
    if (menu) 
    {
	GtkAccelGroup *display_mbar_accel_group;
#ifndef GNOME
	/* the display menubar. Pure gtk. TODO: make it GNOME */
	display_mbar_accel_group = gtk_accel_group_new ();
	*display_mbar_item_factory =  gtk_item_factory_new (GTK_TYPE_MENU_BAR,
							    "<DisplayMBar>",
							    display_mbar_accel_group);
#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func (*display_mbar_item_factory, 
					     menu_translate,
					     "<DisplayMBar>", NULL);
#endif
	gtk_item_factory_create_items (*display_mbar_item_factory,
				       display_nmenu_items - 1,
				       &(display_menu_items[1]), NULL);

	*menu = gtk_item_factory_get_widget (*display_mbar_item_factory, "<DisplayMBar>");
#else
	GtkWidget *menuitem;
	
	/* the display menu */
	*menu = gtk_menu_bar_new();
	display_mbar_accel_group = gtk_accel_group_new();
	menuitem = gtk_tearoff_menu_item_new();
	gnome_app_fill_menu_custom(GTK_MENU_SHELL(*menu), display_menu,
				   &dia_menu_uibdata, display_mbar_accel_group,
				   TRUE, 0);
	/*	gtk_menu_bar_prepend(GTK_MENU_BAR(*menu), menuitem);
		gtk_widget_show (menuitem);*/
#endif
	menus_set_tools_callback ("<DisplayMBar>", *display_mbar_item_factory);
    }
}

#ifdef GNOME
/* a function that performs a similar task to gnome_app_find_menu_pos,
 * but works from the GnomeUIInfo structures, and takes an untranslated path.  It ignores 

 * but doesn't perform any translation of the menu items, and removes
 * underscores when performing the string comparisons */
static GtkWidget *
dia_gnome_menu_get_widget (GnomeUIInfo *uiinfo, const gchar *path)
{
  GtkWidget *ret;
  gchar *name_end;
  gint path_len, label_len, i, j;
  gboolean label_matches;

  g_return_val_if_fail (uiinfo != NULL, NULL);
  g_return_val_if_fail (path != NULL, NULL);

  name_end = strchr(path, '/');
  if (name_end == NULL)
    path_len = strlen(path);
  else
    path_len = name_end - path;

  if (path_len == 0)
    return NULL; /* zero path component */

  for (; uiinfo->type != GNOME_APP_UI_ENDOFINFO; uiinfo++)
    switch (uiinfo->type) {
    case GNOME_APP_UI_BUILDER_DATA:
    case GNOME_APP_UI_HELP:
    case GNOME_APP_UI_ITEM_CONFIGURABLE:
      /* we won't bother handling these two options */
      break;

    case GNOME_APP_UI_RADIOITEMS:
      /* descend the radioitem list, without trimming the path */
      ret = dia_gnome_menu_get_widget((GnomeUIInfo *)uiinfo->moreinfo,
				      path);
      if (ret)
	return ret;
      break;

    case GNOME_APP_UI_SEPARATOR:
    case GNOME_APP_UI_ITEM:
    case GNOME_APP_UI_TOGGLEITEM:
      /* check for matching label */
      if (!uiinfo->label)
	break;
      /* check the path component against the label, ignoring underscores */
      label_matches = TRUE;
      label_len = strlen(uiinfo->label);
      for (i = 0, j = 0; i < label_len; i++) {
	if (uiinfo->label[i] == '_') {
	  /* nothing */;
	} else if (uiinfo->label[i] != path[j]) {
	  label_matches = FALSE;
	  break;
	} else {
	  j++;
	}
      }
      if (label_matches && j == path_len && i == label_len)
	return uiinfo->widget;
      break;

    case GNOME_APP_UI_SUBTREE:
    case GNOME_APP_UI_SUBTREE_STOCK:
      /* descend if label matches, trimming off the start of the path */
      if (!uiinfo->label)
	break;
      /* check the path component against the label, ignoring underscores */
      label_matches = TRUE;
      label_len = strlen(uiinfo->label);
      for (i = 0, j = 0; i < label_len; i++) {
	if (uiinfo->label[i] == '_') {
	  /* nothing */;
	} else if (uiinfo->label[i] != path[j]) {
	  label_matches = FALSE;
	  break;
	} else {
	  j++;
	}
      }
      /* does the stripped label match? then recurse. */
      if (label_matches && j == path_len && i == label_len) {
	/* if we are at the end of the path, then return this menuitem */
	if (path[j] == '\0' || (path[j] == '/' && path[j+1] == '\0'))
	  return uiinfo->widget;
	ret = dia_gnome_menu_get_widget((GnomeUIInfo *)uiinfo->moreinfo,
					path + path_len + 1);
	if (ret)
	  return ret;
      }
      break;
    default:
      g_warning ("unexpected GnomeUIInfo element type %d", (int) uiinfo->type);
      break;
    }
  /* if we fall through, return NULL */
  return NULL;
}


/* returns parent GtkMenu */
static GtkMenuShell *
find_parent(GtkMenuShell *top, GnomeUIInfo *uiinfo, GHashTable *extras,
	    const gchar *mpath)
{
  const gchar *slash;
  gchar *parent_path;
  GtkWidget *parent_item, *parent_submenu, *tearoff, *grandparent_submenu;

  slash = strrchr(mpath, '/');
  if (!slash)
    return top;
  parent_path = g_strndup(mpath, slash - mpath);

  /* check the extras hash table and GnomeUIInfo struct */
  parent_item = g_hash_table_lookup(extras, parent_path);
  if (!parent_item)
    parent_item = dia_gnome_menu_get_widget(uiinfo, parent_path);
  if (parent_item) {
    g_free(parent_path);
    if (GTK_MENU_ITEM(parent_item)->submenu) {
      return GTK_MENU_SHELL(GTK_MENU_ITEM(parent_item)->submenu);
    } else {
      GtkWidget *menu = gtk_menu_new();

      /* setup submenu, and add a tearoff at the top */
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent_item), menu);
      tearoff = gtk_tearoff_menu_item_new();
      gtk_container_add(GTK_CONTAINER(menu), tearoff);
      gtk_widget_show(tearoff);
      return GTK_MENU_SHELL(menu);
    }
  }

  /* not there ... need to create it. */
  grandparent_submenu = GTK_WIDGET(find_parent(top, uiinfo, extras,
					       parent_path));
  slash = strrchr(parent_path, '/');
  if (slash)
    slash++;
  else
    slash = parent_path;
  parent_item = gtk_menu_item_new_with_label(slash);
  gtk_container_add(GTK_CONTAINER(grandparent_submenu), parent_item);
  gtk_widget_show(parent_item);
  g_hash_table_insert(extras, parent_path, parent_item); /* add to extras */

  parent_submenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent_item), parent_submenu);
  tearoff = gtk_tearoff_menu_item_new();
  gtk_container_add(GTK_CONTAINER(parent_submenu), tearoff);
  gtk_widget_show(tearoff);

  return GTK_MENU_SHELL(parent_submenu);
}

GtkWidget *
menus_add_path (const gchar *path)
{
  GnomeUIInfo *uiinfo;
  GHashTable *extras;
  GtkMenuShell *top;
  GtkAccelGroup *accel_group;
  const gchar *mpath = path, *label;
  GtkMenuShell *parent;
  GtkWidget *item;

  if (strncmp(path, "<Display>/", strlen("<Display>/")) == 0) {
    mpath = path + strlen("<Display>/");
    uiinfo = display_menu;
    extras = display_extras;
    top = GTK_MENU_SHELL(display_menus);
    accel_group = display_accels;
  } else if (strncmp(path, "<Toolbox>/", strlen("<Toolbox>/")) == 0) {
    mpath = path + strlen("<Toolbox>/");
    uiinfo = toolbox_menu;
    extras = toolbox_extras;
    top = GTK_MENU_SHELL(toolbox_menubar);
    accel_group = toolbox_accels;
  } else {
    g_warning("bad menu path `%s'", path);
    return NULL;
  }
    
  parent = find_parent(top, uiinfo, extras, mpath);
  label = strrchr(mpath, '/');
  if (label)
    label++;
  else
    label = mpath;
  item = gtk_menu_item_new_with_label(label);
  gtk_container_add(GTK_CONTAINER(parent), item);
  gtk_widget_show(item);
  /* add to item factory, so that accel is saved/loaded */
  gtk_item_factory_add_foreign(item, path, accel_group, 0, 0);
  
  return item;
}

#endif

#ifndef GNOME
GtkWidget *
menus_add_path (const gchar *path)
{
  GtkItemFactory *item_factory;
  GtkItemFactoryEntry new_entry = { 0 };

  new_entry.item_type = "<Item>";
  new_entry.accelerator = NULL;
  new_entry.callback = NULL;
  new_entry.callback_action = 0;

  if (strncmp(path, "<Display>/", strlen("<Display>/")) == 0) {
    item_factory = display_item_factory;
    new_entry.path = (gchar *) (path + strlen("<Display>"));
  } else if (strncmp(path, "<Toolbox>/", strlen("<Toolbox>/")) == 0) {
    item_factory = toolbox_item_factory;
    new_entry.path = (gchar *) (path + strlen("<Toolbox>"));
  } else {
    g_warning("bad menu path `%s'", path);
    return NULL;
  }

  gtk_item_factory_create_item(item_factory, &new_entry, NULL, 1);
  return gtk_item_factory_get_widget(item_factory, path);
}
#endif


/*
  Get a menu item widget from a path. 
  In case of an item in the <DisplayMBar> menu bar, provide the corresponding 
  item factory: ddisp->mbar_item_factory.
 */
GtkWidget *
menus_get_item_from_path (char *path, GtkItemFactory *item_factory)
{
  GtkWidget *widget = NULL;

# ifdef GNOME
      /* DDisplay *ddisp; */
  const gchar *menu_name;

  /* drop the "<Display>/", "<DisplayMBar>/" or "<Toolbox>/" at the start */
  menu_name = path;
  if (! (path = strchr (path, '/'))) return NULL;
  path ++; /* move past the / */

  if (strncmp(menu_name, "<Display>", strlen("<Display>")) == 0) {
/*    ddisp = ddisplay_active ();
      if (! ddisp)
      return NULL; */
    widget = dia_gnome_menu_get_widget(display_menu, path);
  } else if  (strncmp(menu_name, "<DisplayMBar>", strlen("<DisplayMBar>"))  == 0) {
      /* finding this requires an item factory*/
      widget = dia_gnome_menu_get_widget(display_menu, path);
  } else if (strncmp(menu_name, "<Toolbox>", strlen("<Toolbox>")) == 0) {
    widget = dia_gnome_menu_get_widget(toolbox_menu, path);
  } 
# else

  if (display_item_factory) {
    widget = gtk_item_factory_get_widget(display_item_factory, path);
  }
  
  if ((widget == NULL) && (item_factory)) {
      widget = gtk_item_factory_get_widget(item_factory, path);
  }
  
  if (widget == NULL) {
    widget = gtk_item_factory_get_widget(toolbox_item_factory, path);
  }
# endif

  if (! widget) {
    g_warning(_("Can't find menu entry '%s'!\nThis is probably a i18n problem "
                "(try LANG=C)."), path);
  }

  return widget;
}


void
menus_initialize_updatable_items (UpdatableMenuItems *items, 
				  GtkItemFactory *factory, const char *display)
{
    static GString *path;
    
#   ifdef GNOME
    /* This should only happen with popmenu. I don't remember why... */
    if ((strcmp(display, "<Display>") == 0) && (ddisplay_active () == NULL))
    {
	return;
    }
#   endif
    path = g_string_new (display);
    g_string_append (path,"/Edit/Copy");
    items->copy = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Edit/Cut");
    items->cut = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Edit/Paste");
    items->paste = menus_get_item_from_path(path->str, factory);
#   ifndef GNOME
    g_string_append (g_string_assign(path, display),"/Edit/Delete");
    items->edit_delete = menus_get_item_from_path(path->str, factory);
#   endif

    g_string_append (g_string_assign(path, display),"/Edit/Copy Text");
    items->copy_text = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Edit/Cut Text");
    items->cut_text = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Edit/Paste Text");
    items->paste_text = menus_get_item_from_path(path->str, factory);

    g_string_append (g_string_assign(path, display),"/Objects/Send to Back");
    items->send_to_back = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Bring to Front");
    items->bring_to_front = menus_get_item_from_path(path->str, factory);
  
    g_string_append (g_string_assign(path, display),"/Objects/Group");
    items->group = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Ungroup");
    items->ungroup = menus_get_item_from_path(path->str, factory);

    g_string_append (g_string_assign(path, display),"/Objects/Align Horizontal/Left");
    items->align_h_l = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align Horizontal/Center");
    items->align_h_c = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align Horizontal/Right");
    items->align_h_r = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align Horizontal/Equal Distance");
    items->align_h_e = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align Horizontal/Adjacent");
    items->align_h_a = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align Vertical/Top");
    items->align_v_t = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align Vertical/Center");
    items->align_v_c = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align Vertical/Bottom");
    items->align_v_b = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align Vertical/Equal Distance");
    items->align_v_e = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align Vertical/Adjacent");
    items->align_v_a = menus_get_item_from_path(path->str, factory);
    
    g_string_free (path,FALSE);
}


static void
plugin_callback (GtkWidget *widget, gpointer data)
{
  DiaCallbackFilter *cbf = data;

  /* and finally invoke it */
  if (cbf->callback) {
    DDisplay *ddisp = ddisplay_active();
    DiagramData* diadata = ddisp ? ddisp->diagram->data : NULL;
    cbf->callback (diadata, 0, cbf->user_data);
  }
}

