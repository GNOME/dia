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
#include "plugin-manager.h"
#include "select.h"
#include "dia_dirs.h"

#if GNOME
static GnomeUIInfo toolbox_filemenu[] = {
  GNOMEUIINFO_MENU_NEW_ITEM(N_("_New diagram"), N_("Create new diagram"),
			    file_new_callback, NULL),
  GNOMEUIINFO_MENU_OPEN_ITEM(file_open_callback, NULL),
  GNOMEUIINFO_MENU_PREFERENCES_ITEM(file_preferences_callback, NULL),
  GNOMEUIINFO_ITEM_NONE(N_("Plug-ins"), NULL, file_plugins_callback),
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
  GNOMEUIINFO_ITEM_NONE(N_("Diagram Propeties..."), NULL, view_diagram_properties_callback),
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
  GNOMEUIINFO_RADIOITEM(N_("Replace"), NULL, select_style_callback, 
			 NULL),
  GNOMEUIINFO_RADIOITEM(N_("Union"), NULL, select_style_callback,
			 NULL),
  GNOMEUIINFO_RADIOITEM(N_("Intersect"), NULL, select_style_callback,
			 NULL),
  GNOMEUIINFO_RADIOITEM(N_("Remove"), NULL, select_style_callback, 
			 NULL), 
  GNOMEUIINFO_RADIOITEM(N_("Invert"), NULL, select_style_callback, 
			 NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo objects_align_h[] = {
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Left"), NULL, objects_align_h_callback,
			     GINT_TO_POINTER(0)),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Center"), NULL, objects_align_h_callback,
			     GINT_TO_POINTER(1)),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Right"), NULL, objects_align_h_callback,
			     GINT_TO_POINTER(2)),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Equal Distance"), NULL, objects_align_h_callback,
			     GINT_TO_POINTER(4)),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Adjacent"), NULL, objects_align_h_callback,
			     GINT_TO_POINTER(5)),
  GNOMEUIINFO_END
};

static GnomeUIInfo objects_align_v[] = {
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Top"), NULL, objects_align_v_callback,
			     GINT_TO_POINTER(0)),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Center"), NULL, objects_align_v_callback,
			     GINT_TO_POINTER(1)),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Bottom"), NULL, objects_align_v_callback,
			     GINT_TO_POINTER(2)),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Equal Distance"), NULL, objects_align_v_callback,
			     GINT_TO_POINTER(4)),
  GNOMEUIINFO_ITEM_NONE_DATA(N_("Adjacent"), NULL, objects_align_h_callback,
			     GINT_TO_POINTER(5)),
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
  GNOMEUIINFO_SUBTREE(N_("_Select"), selectmenu),
  GNOMEUIINFO_SUBTREE(N_("_Objects"), objectsmenu),
  GNOMEUIINFO_SUBTREE(N_("_Tools"), toolsmenu),
  GNOMEUIINFO_SUBTREE(N_("_Dialogs"), dialogsmenu),
  GNOMEUIINFO_END
};

#else /* !GNOME */

static void menus_create_branches (GtkItemFactory       *item_factory,
				   GtkItemFactoryEntry  *entry);

#define MRU_MENU_ENTRY_SIZE (strlen ("/File/MRU00 ") + 1)
#define MRU_MENU_ACCEL_SIZE sizeof ("<control>0")

static GtkItemFactoryEntry toolbox_menu_items[] =
{
  {N_("/_File"),               NULL,         NULL,                      0, "<Branch>"},
  {N_("/File/_New"),           "<control>N", file_new_callback,         0 },
  {N_("/File/_Open"),          "<control>O", file_open_callback,        0 },
  {N_("/File/_Preferences..."),NULL,         file_preferences_callback, 0 },
  {N_("/File/P_lugins"),       NULL,         file_plugins_callback,     0 },
  {N_("/File/---"),            NULL,         NULL,                      0, "<Separator>"},
  {N_("/File/_Quit"),          "<control>Q", file_quit_callback,        0 },
  {N_("/_Help"),               NULL,         NULL,                      0, "<Branch>" },
  {N_("/Help/_About"),         NULL,         help_about_callback,       0 },
};

static GtkItemFactoryEntry display_menu_items[] =
{
  {N_("/_File"),                  NULL,         NULL,           0, "<Branch>"},
  {N_("/File/_New"),              "<control>N", file_new_callback,          0},
  {N_("/File/_Open"),             "<control>O", file_open_callback,         0},
  {N_("/File/_Save"),             "<control>S", file_save_callback,         0},
  {N_("/File/Save _As..."),       "<control>W", file_save_as_callback,      0},
  {N_("/File/_Export..."),        NULL,         file_export_callback,       0},
  {N_("/File/---"),               NULL,         NULL,        0, "<Separator>"},
  {N_("/File/Page Set_up..."),    NULL,         file_pagesetup_callback,    0},
  {N_("/File/_Print Diagram..."), "<control>P", file_print_callback,        0},
  {N_("/File/---"),               NULL,         NULL,        0, "<Separator>"},
  {N_("/File/_Close"),            NULL,         file_close_callback,        0},
  {   "/File/---MRU",             NULL,         NULL,        0, "<Separator>"},
  {N_("/File/_Quit"),              "<control>Q", file_quit_callback,        0},
  {N_("/_Edit"),                  NULL,         NULL,           0, "<Branch>"},
  {N_("/Edit/_Copy"),             "<control>C", edit_copy_callback,         0},
  {N_("/Edit/C_ut"),              "<control>X", edit_cut_callback,          0},
  {N_("/Edit/_Paste"),            "<control>V", edit_paste_callback,        0},
  {N_("/Edit/_Delete"),           "<control>D", edit_delete_callback,       0},
  {N_("/Edit/_Undo"),             "<control>Z", edit_undo_callback,         0},
  {N_("/Edit/_Redo"),             "<control>R", edit_redo_callback,         0},
  {N_("/Edit/Copy Text"),         NULL,         edit_copy_text_callback,    0},
  {N_("/Edit/Cut Text"),          NULL,         edit_cut_text_callback,     0},
  {N_("/Edit/Paste _Text"),       NULL,         edit_paste_text_callback,   0},
  {N_("/_View"),                  NULL,         NULL,           0, "<Branch>"},
  {N_("/View/Zoom _In"),          NULL,          view_zoom_in_callback,     0},
  {N_("/View/Zoom _Out"),         NULL,          view_zoom_out_callback,    0},
  {N_("/View/_Zoom"),             NULL,         NULL,           0, "<Branch>"},
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
  {N_("/View/Diagram Properties"),NULL,         view_diagram_properties_callback, 0},
#ifdef HAVE_LIBART  
  {N_("/View/_AntiAliased"),      NULL,         view_aa_callback,           0, "<ToggleItem>"},
#endif
  {N_("/View/_Visible Grid"),     NULL,         view_visible_grid_callback, 0, "<ToggleItem>"},
  {N_("/View/_Snap To Grid"),     "G",          view_snap_to_grid_callback, 0, "<ToggleItem>"},
  {N_("/View/Show _Rulers"),      NULL,         view_toggle_rulers_callback,0, "<ToggleItem>"},
  {N_("/View/Show _Connection Points"),	  NULL,		view_show_cx_pts_callback,   0,	"<ToggleItem>"},
  {N_("/View/---"),               NULL,         NULL,        0, "<Separator>"},
  {N_("/View/New _View"),         "<control>I", view_new_view_callback,     0},
  {N_("/View/Show _All"),         "<control>A", view_show_all_callback,     0},
  {N_("/_Select"),                NULL,         NULL,           0, "<Branch>"},
  {N_("/Select/All"),             NULL,         select_all_callback,        0},
  {N_("/Select/None"),            NULL,         select_none_callback,       0},
  {N_("/Select/Invert"),          NULL,         select_invert_callback,     0},
  {N_("/Select/Connected"),       NULL,         select_connected_callback,  0},
  {N_("/Select/Transitive"),      NULL,         select_transitive_callback, 0},
  {N_("/Select/Same Type"),       NULL,         select_same_type_callback,  0},
  {N_("/Select/---"),             NULL,         NULL,        0, "<Separator>"},
  {N_("/Select/Replace"),       NULL, select_style_callback, 0, "<RadioItem>"},
  {N_("/Select/Union"),     NULL, select_style_callback, 1, "/Select/Replace"},
  {N_("/Select/Intersect"), NULL, select_style_callback, 2, "/Select/Replace"},
  {N_("/Select/Remove"),    NULL, select_style_callback, 3, "/Select/Replace"},
  {N_("/Select/Invert"),    NULL, select_style_callback, 4, "/Select/Replace"},
  {N_("/_Objects"),               NULL,         NULL,           0, "<Branch>"},
  {N_("/Objects/Send to _Back"),  "<control>B",objects_place_under_callback,0},
  {N_("/Objects/Bring to _Front"),"<control>F", objects_place_over_callback,0},
  {N_("/Objects/---"),            NULL,         NULL,        0, "<Separator>"},
  {N_("/Objects/_Group"),         "<control>G", objects_group_callback,     0},
  {N_("/Objects/_Ungroup"),       "<control>U", objects_ungroup_callback,   0},
  {N_("/Objects/---"),            NULL,         NULL,        0, "<Separator>"},
  {N_("/Objects/Align _Horizontal"),       NULL, NULL,          0, "<Branch>"},
  {N_("/Objects/Align Horizontal/Left"),   NULL, objects_align_h_callback,  0},
  {N_("/Objects/Align Horizontal/Center"), NULL, objects_align_h_callback,  1},
  {N_("/Objects/Align Horizontal/Right"),  NULL, objects_align_h_callback,  2},
  {N_("/Objects/Align Horizontal/Equal Distance"), NULL, objects_align_h_callback,    4},
  {N_("/Objects/Align Horizontal/Adjacent"), NULL, objects_align_h_callback,    5},
  {N_("/Objects/Align _Vertical"),         NULL, NULL,          0, "<Branch>"},
  {N_("/Objects/Align Vertical/Top"),      NULL, objects_align_v_callback,  0},
  {N_("/Objects/Align Vertical/Center"),   NULL, objects_align_v_callback,  1},
  {N_("/Objects/Align Vertical/Bottom"),   NULL, objects_align_v_callback,  2},
  {N_("/Objects/Align Vertical/Equal Distance"),   NULL, objects_align_v_callback,    4},
  {N_("/Objects/Align Vertical/Adjacent"), NULL, objects_align_v_callback,  5},

  {N_("/_Tools"),                 NULL,     NULL,               0, "<Branch>"},
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
  {N_("/Dialogs/_Properties"),    NULL,     dialogs_properties_callback,    0},
  {N_("/Dialogs/_Layers"),        NULL,     dialogs_layers_callback,        0},
};

/* calculate the number of menu_item's */
static int toolbox_nmenu_items = (sizeof(toolbox_menu_items) / 
				  sizeof(toolbox_menu_items[0]));
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

#ifndef GNOME
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
    gtk_item_factory_dump_rc (accelfilename, NULL, TRUE);
    g_free (accelfilename);
  }
  return TRUE;
}

static void
menus_create_item (GtkItemFactory       *item_factory,
		   GtkItemFactoryEntry  *entry,
		   gpointer              callback_data,
		   guint                 callback_type)
{
  GtkWidget *menu_item;

  if (!(strstr (entry->path, "tearoff1")))
    menus_create_branches (item_factory, entry);

  gtk_item_factory_create_item (item_factory,
				(GtkItemFactoryEntry *) entry,
				callback_data,
				callback_type);

  menu_item = gtk_item_factory_get_item (item_factory, entry->path);
}

static void
menus_create_items (GtkItemFactory       *item_factory,
		    guint                 n_entries,
		    GtkItemFactoryEntry *entries,
		    gpointer              callback_data,
		    guint                 callback_type)
{
  gint i;

  for (i = 0; i < n_entries; i++)
    {
      menus_create_item (item_factory,
			 entries + i,
			 callback_data,
			 callback_type);
    }
}

static void
menus_create_branches (GtkItemFactory       *item_factory,
		       GtkItemFactoryEntry  *entry)
{
  GString *tearoff_path;
  gint factory_length;
  gchar *p;
  gchar *path;

  tearoff_path = g_string_new ("");

  path = entry->path;
  p = strchr (path, '/');
  factory_length = p - path;

  /*  skip the first slash  */
  if (p)
    p = strchr (p + 1, '/');

  while (p)
    {
      g_string_assign (tearoff_path, path + factory_length);
      g_string_truncate (tearoff_path, p - path - factory_length);

      if (!gtk_item_factory_get_widget (item_factory, tearoff_path->str))
	{
	  GtkItemFactoryEntry branch_entry =
	  { NULL, NULL, NULL, 0, "<Branch>" };

	  branch_entry.path = tearoff_path->str;
	  gtk_object_set_data (GTK_OBJECT (item_factory), "complete", path);
	  menus_create_item (item_factory, &branch_entry, NULL, 2);
	  gtk_object_remove_data (GTK_OBJECT (item_factory), "complete");
	}

      g_string_append (tearoff_path, "/tearoff1");

      if (!gtk_item_factory_get_widget (item_factory, tearoff_path->str))
	{
	  GtkItemFactoryEntry tearoff_entry =
	  { NULL, NULL, NULL, 0, "<Tearoff>" };

	  tearoff_entry.path = tearoff_path->str;
	  menus_create_item (item_factory, &tearoff_entry, NULL, 2);
	}

      p = strchr (p + 1, '/');
    }

  g_string_free (tearoff_path, TRUE);
}

static void
menus_last_opened_cmd_callback (GtkWidget *widget,
                                gpointer   callback_data,
                                guint      num)
{
  /* FIXME: This is GIMP code. We should use dia code here! 
  gchar *filename, *raw_filename;
  guint num_entries;
  gint  status;

  num_entries = g_slist_length (last_opened_raw_filenames); 
  if (num >= num_entries)
    return;

  raw_filename =
    ((GString *) g_slist_nth_data (last_opened_raw_filenames, num))->str;
  filename = g_basename (raw_filename);

  status = file_open (raw_filename, raw_filename);

  if (status != PDB_SUCCESS &&
      status != PDB_CANCEL)
    {
      g_message (_("Error opening file: %s\n"), raw_filename);
    } */
}

static void
menus_init_mru (void)
{
  /* FIXME: This is GIMP code. We should use dia code here! 
  
  GtkItemFactoryEntry *last_opened_entries;
  GtkWidget	      *menu_item;
  gchar	*paths;
  gchar *accelerators;
  gint	 i;
  
  last_opened_entries = g_new (GtkItemFactoryEntry, last_opened_size);

  paths = g_new (gchar, last_opened_size * MRU_MENU_ENTRY_SIZE);
  accelerators = g_new (gchar, 9 * MRU_MENU_ACCEL_SIZE);

  for (i = 0; i < last_opened_size; i++)
    {
      gchar *path, *accelerator;

      path = &paths[i * MRU_MENU_ENTRY_SIZE];
      if (i < 9)
        accelerator = &accelerators[i * MRU_MENU_ACCEL_SIZE];
      else
        accelerator = NULL;
    
      last_opened_entries[i].path = path;
      last_opened_entries[i].accelerator = accelerator;
      last_opened_entries[i].callback =
	(GtkItemFactoryCallback) menus_last_opened_cmd_callback;
      last_opened_entries[i].callback_action = i;
      last_opened_entries[i].item_type = NULL;

      g_snprintf (path, MRU_MENU_ENTRY_SIZE, "/File/MRU%02d", i + 1);
      if (accelerator != NULL)
	g_snprintf (accelerator, MRU_MENU_ACCEL_SIZE, "<control>%d", i + 1);
    }

  menus_create_items (toolbox_factory, last_opened_size,
		      last_opened_entries, NULL, 2);
  
  for (i=0; i < last_opened_size; i++)
    {
      menu_item =
	gtk_item_factory_get_widget (toolbox_factory,
				     last_opened_entries[i].path);
      gtk_widget_hide (menu_item);
    }

  menu_item = gtk_item_factory_get_widget (toolbox_factory, "/File/---MRU");
  if (menu_item && menu_item->parent)
    gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, -1);
  gtk_widget_hide (menu_item);

  menu_item = gtk_item_factory_get_widget (toolbox_factory, "/File/Quit");
  if (menu_item && menu_item->parent)
    gtk_menu_reorder_child (GTK_MENU (menu_item->parent), menu_item, -1);

  g_free (paths);
  g_free (accelerators);
  g_free (last_opened_entries); */
}

static void
menus_init(void)
{
  GtkWidget *menuitem;
  GString *path;
  gchar *accelfilename;
  gint i, len;

  if (!initialise)
    return;
  
  initialise = FALSE;

#ifdef GNOME
  /* the toolbox menu */
  toolbox_accels = gtk_accel_group_new();
  toolbox_menubar = gtk_menu_bar_new();
  gnome_app_fill_menu(GTK_MENU_SHELL(toolbox_menubar), toolbox_menu,
		      toolbox_accels, TRUE, 0);

  /* the display menu */
  display_menus = gnome_popup_menu_new(display_menu);
  display_accels = gnome_popup_menu_get_accel_group(GTK_MENU(display_menus));
  menuitem = gtk_tearoff_menu_item_new();
  gtk_menu_prepend(GTK_MENU(display_menus), menuitem);
  gtk_widget_show(menuitem);
#else
  /* the toolbox menu */
  toolbox_accels = gtk_accel_group_new();
  toolbox_item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<Toolbox>",
					      toolbox_accels);
  gtk_object_set_data (GTK_OBJECT (toolbox_item_factory), "factory_path",
		       (gpointer) "toolbox");
  gtk_item_factory_set_translate_func (toolbox_item_factory, menu_translate,
				       "<Toolbox>", NULL);
  menus_create_items (toolbox_item_factory,
		      toolbox_nmenu_items,
		      toolbox_menu_items,
		      NULL, 2);
  
  menus_init_mru ();

  toolbox_menubar = gtk_item_factory_get_widget(toolbox_item_factory,
						"<Toolbox>");

  /* the display menu */
  display_accels = gtk_accel_group_new();
  display_item_factory = gtk_item_factory_new(GTK_TYPE_MENU, "<Display>",
					      display_accels);
  gtk_item_factory_set_translate_func (display_item_factory, menu_translate,
				       "<Display>", NULL);
  menus_create_items (display_item_factory,
		      display_nmenu_items,
		      display_menu_items,
		      NULL, 2);

  display_menus = gtk_item_factory_get_widget(display_item_factory,
					      "<Display>");
#endif

  path = g_string_new("<Display>/Tools/");
  len = path->len;
  for (i = 0; i < num_tools; i++) {
    g_string_append(path, tool_data[i].menuitem_name);
    menuitem = menus_get_item_from_path(path->str);
    if (menuitem != NULL)
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			 GTK_SIGNAL_FUNC(tool_menu_select),
			 &tool_data[i].callback_data);
    g_string_truncate(path, len);
  }
  g_string_free(path, TRUE);

  gtk_menu_set_title(GTK_MENU(display_menus), _("Diagram Menu"));
  
  accelfilename = dia_config_filename("menurc");
  
  if (accelfilename) {
    gtk_item_factory_parse_rc(accelfilename);
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

GtkWidget *
menus_get_item_from_path (char *path)
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
    
    parentw = gnome_app_find_menu_pos(display_menus, path, &pos);
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

