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

#define MRU_MENU_ENTRY_SIZE (strlen ("/File/MRU00 ") + 1)
#define MRU_MENU_ACCEL_SIZE sizeof ("<control>0")

static GtkItemFactoryEntry toolbox_menu_items[] =
{
  {N_("/_File"),               NULL,         NULL,       0,    "<Branch>" },
  {   "/File/tearoff",         NULL,         NULL,       0,   "<Tearoff>" },
  {N_("/File/_New"),           "<control>N", file_new_callback,         0,
      "<StockItem>", GTK_STOCK_NEW },
  {N_("/File/_Open..."),       "<control>O", file_open_callback,        0,
      "<StockItem>", GTK_STOCK_OPEN },
/*  {N_("/Open _Recent"),           NULL,         NULL,           0, "<Branch>"}, */
/*  {   "/Open Recent/tearoff",     NULL,         NULL,         0, "<Tearoff>" }, */
  {N_("/File/---"),            NULL,         NULL,       0, "<Separator>" },
  {N_("/File/_Diagram tree"),  "F8",         diagtree_show_callback,    0,
   "<ToggleItem>" },
  {N_("/File/Sheets and Objects..."),
                               "F9",         sheets_dialog_show_callback, 0 },
  {N_("/File/---"),            NULL,         NULL,       0, "<Separator>" },
 {N_("/File/_Preferences..."),NULL,         file_preferences_callback, 0,
      "<StockItem>", GTK_STOCK_PREFERENCES },
   {N_("/File/P_lugins..."),   NULL,         file_plugins_callback,     0,
      "<StockItem>", GTK_STOCK_EXECUTE },
  {N_("/File/---"),            NULL,         NULL,       0, "<Separator>" },
    /* recent file list is dynamically inserted here */
  {N_("/File/---"),            NULL,         NULL,       0, "<Separator>" },
  {N_("/File/_Quit"),          "<control>Q", file_quit_callback,        0,
      "<StockItem>", GTK_STOCK_QUIT },
  {N_("/_Help"),               NULL,         NULL,       0,    "<Branch>" },
  {   "/Help/tearoff",         NULL,         NULL,       0,   "<Tearoff>" },
  {N_("/Help/_Contents"),        "F1",         help_manual_callback,      0,
      "<StockItem>", GTK_STOCK_HELP },
  {N_("/Help/---"),            NULL,         NULL,       0, "<Separator>" },
  {N_("/Help/_About..."),      NULL,         help_about_callback,       0 }
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
  {N_("/File/_Open..."),          "<control>O", file_open_callback,         0,
      "<StockItem>", GTK_STOCK_OPEN },
/*  {N_("/Open _Recent"),           NULL,         NULL,           0, "<Branch>"}, */
/*  {   "/Open Recent/tearoff",     NULL,         NULL,         0, "<Tearoff>" }, */
  {N_("/File/---"),               NULL,         NULL,        0, "<Separator>"},
  {N_("/File/_Save"),             "<control>S", file_save_callback,         0,
      "<StockItem>", GTK_STOCK_SAVE },
  {N_("/File/Save _As..."),       "<control><shift>S", file_save_as_callback,      0,
      "<StockItem>", GTK_STOCK_SAVE_AS },
  {N_("/File/_Export..."),        NULL,         file_export_callback,         0,
      "<StockItem>", GTK_STOCK_CONVERT},
  {N_("/File/---"),               NULL,         NULL,        0, "<Separator>"},
  {N_("/File/Page Set_up..."),    NULL,         file_pagesetup_callback,    0},
  {N_("/File/_Print Diagram..."), "<control>P", file_print_callback,        0,
      "<StockItem>", GTK_STOCK_PRINT },
  {N_("/File/---"),               NULL,         NULL,        0, "<Separator>"},
  {N_("/File/_Close"),            "<control>W",         file_close_callback,        0,
      "<StockItem>", GTK_STOCK_CLOSE },
  {   "/File/---MRU",             NULL,         NULL,        0, "<Separator>"},
  {N_("/File/_Quit"),              "<control>Q", file_quit_callback,        0,
      "<StockItem>", GTK_STOCK_QUIT},
  {N_("/_Edit"),                  NULL,         NULL,           0, "<Branch>"},
  {   "/Edit/tearoff",            NULL,         NULL,         0, "<Tearoff>" },
  {N_("/Edit/_Undo"),             "<control>Z", edit_undo_callback,         0,
      "<StockItem>", GTK_STOCK_UNDO },
  {N_("/Edit/_Redo"),             "<control><shift>Z", edit_redo_callback,         0,
      "<StockItem>", GTK_STOCK_REDO },
  {N_("/Edit/---"),            NULL,         NULL,       0, "<Separator>" },
  {N_("/Edit/_Copy"),             "<control>C", edit_copy_callback,         0,
      "<StockItem>", GTK_STOCK_COPY },
  {N_("/Edit/C_ut"),              "<control>X", edit_cut_callback,          0,
      "<StockItem>", GTK_STOCK_CUT },
  {N_("/Edit/_Paste"),            "<control>V", edit_paste_callback,        0,
      "<StockItem>", GTK_STOCK_PASTE },
  {N_("/Edit/_Duplicate"),        "<control>D", edit_duplicate_callback, 0, },
  {N_("/Edit/_Delete"),           "Delete", edit_delete_callback,       0,
      "<StockItem>", GTK_STOCK_DELETE },
  {N_("/Edit/---"),            NULL,         NULL,       0, "<Separator>" },
  {N_("/Edit/Copy Text"),      "<control><shift>C",         edit_copy_text_callback,    0,
      "<StockItem>", GTK_STOCK_COPY },
  {N_("/Edit/Cut Text"),       "<control><shift>X",         edit_cut_text_callback,     0,
      "<StockItem>", GTK_STOCK_CUT },
  {N_("/Edit/Paste _Text"),    "<control><shift>V",         edit_paste_text_callback,   0,
      "<StockItem>", GTK_STOCK_PASTE },
  {N_("/_Diagram"),               NULL,         NULL,           0, "<Branch>"},
  {   "/Diagram/tearoff",            NULL,         NULL,         0, "<Tearoff>" },
  {N_("/Diagram/_Properties..."), NULL,         view_diagram_properties_callback,          0,
      "<StockItem>", GTK_STOCK_PROPERTIES},
  {N_("/Diagram/_Layers..."),     NULL,     dialogs_layers_callback,        0},
  {N_("/_View"),                  NULL,         NULL,           0, "<Branch>"},
  {   "/View/tearoff",            NULL,         NULL,         0, "<Tearoff>" },
  {N_("/View/Zoom _In"),          "<control>+", view_zoom_in_callback,     0,
      "<StockItem>", GTK_STOCK_ZOOM_IN },
  {N_("/View/Zoom _Out"),         "<control>-", view_zoom_out_callback,    0,
      "<StockItem>", GTK_STOCK_ZOOM_OUT },
  {N_("/View/_Zoom"),             NULL,         NULL,           0, "<Branch>"},
  {   "/View/Zoom/tearoff",       NULL,         NULL,         0, "<Tearoff>" },
  {N_("/View/Zoom/1600%"),        NULL,         view_zoom_set_callback,  16000},
  {N_("/View/Zoom/800%"),         NULL,         view_zoom_set_callback,  8000},
  {N_("/View/Zoom/400%"),         "<alt>4",     view_zoom_set_callback,  4000},
  {N_("/View/Zoom/283%"),         NULL,         view_zoom_set_callback,  2828},
  {N_("/View/Zoom/200%"),         "<alt>2",     view_zoom_set_callback,  2000},
  {N_("/View/Zoom/141%"),         NULL,         view_zoom_set_callback,  1414},
  {N_("/View/Zoom/100%"),         "<alt>1",     view_zoom_set_callback,  1000,
      "<StockItem>", GTK_STOCK_ZOOM_100 },
  {N_("/View/Zoom/85%"),          NULL,         view_zoom_set_callback,   850},
  {N_("/View/Zoom/70.7%"),        NULL,         view_zoom_set_callback,   707},
  {N_("/View/Zoom/50%"),          "<alt>5",     view_zoom_set_callback,   500},
  {N_("/View/Zoom/35.4%"),        NULL,         view_zoom_set_callback,   354},
  {N_("/View/Zoom/25%"),          NULL,         view_zoom_set_callback,   250},
  {N_("/View/---"),            NULL,         NULL,       0, "<Separator>" },
  {N_("/View/Fullscr_een"),      "F11",        view_fullscreen_callback,   0, "<ToggleItem>"},
#ifdef HAVE_LIBART  
  {N_("/View/_AntiAliased"),      NULL,         view_aa_callback,           0, "<ToggleItem>"},
#endif
  {N_("/View/Show _Grid"),     NULL,         view_visible_grid_callback, 0, "<ToggleItem>"},
  {N_("/View/_Snap To Grid"),     NULL,         view_snap_to_grid_callback, 0, "<ToggleItem>"},
  {N_("/View/Show _Rulers"),      NULL,         view_toggle_rulers_callback,0, "<ToggleItem>"},
  {N_("/View/Show _Connection Points"),	  NULL,		view_show_cx_pts_callback,   0,	"<ToggleItem>"},
  {N_("/View/---"),               NULL,         NULL,        0, "<Separator>"},
  {N_("/View/New _View"),         NULL, view_new_view_callback,     0},
  /* Show All, Best Fit.  Same as the Gimp, Ctrl+E */
  {N_("/View/Show _All"),         "<control>E", view_show_all_callback,     0,
      "<StockItem>", GTK_STOCK_ZOOM_FIT },
  {N_("/View/Re_draw"),           NULL,         view_redraw_callback,       0,
      "<StockItem>", GTK_STOCK_REFRESH },
  {N_("/_Objects"),               NULL,         NULL,           0, "<Branch>"},
  {   "/Objects/tearoff",         NULL,         NULL,         0, "<Tearoff>" },
  {N_("/Objects/Send to _Back"),  "<control>B",objects_place_under_callback, 0,
      "<StockItem>", GTK_STOCK_GOTO_BOTTOM },
  {N_("/Objects/Bring to _Front"),"<control>F", objects_place_over_callback,0,
      "<StockItem>", GTK_STOCK_GOTO_TOP},
  {N_("/Objects/Send Backwards"),  "<control><shift>B",objects_place_down_callback,0,
      "<StockItem>", GTK_STOCK_GO_DOWN},
  {N_("/Objects/Bring Forwards"),"<control><shift>F", objects_place_up_callback,0,
      "<StockItem>", GTK_STOCK_GO_UP},
  {N_("/Objects/---"),            NULL,         NULL,        0, "<Separator>"},
  {N_("/Objects/_Group"),         "<control>G", objects_group_callback,     0},
  /* deliberately not using Ctrl+U for Ungroup */
  {N_("/Objects/_Ungroup"),       "<control><shift>G", objects_ungroup_callback,   0}, 
  {N_("/Objects/---"),            NULL,         NULL,        0, "<Separator>"},
  {N_("/Objects/_Parent"),         "<control>L", objects_parent_callback,     0},
  {N_("/Objects/_Unparent"),       "<control><shift>L", objects_unparent_callback,   0},
  {N_("/Objects/_Unparent Children"),       NULL, objects_unparent_children_callback,   0},
  {N_("/Objects/---"),            NULL,         NULL,        0, "<Separator>"},
  {N_("/Objects/Align"),       NULL, NULL,          0, "<Branch>"},
  {   "/Objects/Align/tearoff", NULL, NULL,        0, "<Tearoff>" },
  {N_("/Objects/Align/Left"),   NULL, objects_align_h_callback,  DIA_ALIGN_LEFT,
      "<StockItem>", GTK_STOCK_JUSTIFY_LEFT },
  {N_("/Objects/Align/Center"), NULL, objects_align_h_callback,  DIA_ALIGN_CENTER,
      "<StockItem>", GTK_STOCK_JUSTIFY_CENTER},
  {N_("/Objects/Align/Right"),  NULL, objects_align_h_callback,  DIA_ALIGN_RIGHT,
      "<StockItem>", GTK_STOCK_JUSTIFY_RIGHT},
  {N_("/Objects/Align/---"),            NULL,         NULL,        0, "<Separator>"},
  {N_("/Objects/Align/Top"),      NULL, objects_align_v_callback,  DIA_ALIGN_TOP},
  {N_("/Objects/Align/Middle"),   NULL, objects_align_v_callback,  DIA_ALIGN_CENTER},
  {N_("/Objects/Align/Bottom"),   NULL, objects_align_v_callback,  DIA_ALIGN_BOTTOM},
  {N_("/Objects/Align/---"),             NULL,         NULL,        0, "<Separator>"},
  {N_("/Objects/Align/Spread Out Horizontally"), NULL, objects_align_h_callback,    DIA_ALIGN_EQUAL},
  {N_("/Objects/Align/Spread Out Vertically"),   NULL, objects_align_v_callback,    DIA_ALIGN_EQUAL},
  {N_("/Objects/Align/Adjacent"), NULL, objects_align_h_callback,    DIA_ALIGN_ADJACENT},
  {N_("/Objects/Align/Stacked"), NULL, objects_align_v_callback,  DIA_ALIGN_ADJACENT},
  {N_("/Objects/---"),            NULL,     NULL,                           0, "<Separator>"},
  {N_("/Objects/_Properties..."), NULL,     dialogs_properties_callback,         0,
      "<StockItem>", GTK_STOCK_PROPERTIES},
  {N_("/_Select"),                NULL,         NULL,           0, "<Branch>"},
  {   "/Select/tearoff",          NULL,         NULL,         0, "<Tearoff>" },
  {N_("/Select/All"),             "<control>A", select_all_callback,        0},
  {N_("/Select/None"),            "<control><shift>A", select_none_callback,   0},
  {N_("/Select/Invert"),          "<control>I",         select_invert_callback,     0},
  {N_("/Select/---"),             NULL,         NULL,        0, "<Separator>"},
  {N_("/Select/Transitive"),      "<control>T", select_transitive_callback, 0},
  {N_("/Select/Connected"),       "<control><shift>T",         select_connected_callback,  0},
  {N_("/Select/Same Type"),       NULL,         select_same_type_callback,  0},
  {N_("/Select/---"),             NULL,         NULL,        0, "<Separator>"},
  {N_("/Select/Replace"),       NULL, select_style_callback,
   SELECT_REPLACE, "<RadioItem>"},
  {N_("/Select/Union"),     NULL, select_style_callback,
   SELECT_UNION, "/Select/Replace"},
  {N_("/Select/Intersection"), NULL, select_style_callback,
   SELECT_INTERSECTION, "/Select/Replace"},
  {N_("/Select/Remove"),    NULL, select_style_callback,
   SELECT_REMOVE, "/Select/Replace"},
  /* Cannot also be called Invert, duplicate names caused keybinding problems */
  {N_("/Select/Inverse"),    NULL, select_style_callback,
   SELECT_INVERT, "/Select/Replace"},
  {N_("/_Tools"),                 NULL,     NULL,               0, "<Branch>"},
  {   "/Tools/tearoff",           NULL,     NULL,         0, "<Tearoff>" },
  {N_("/Tools/Modify"),           "<alt>.", NULL,                         0},
  {N_("/Tools/Magnify"),          "<alt>M", NULL,                           0},
  {N_("/Tools/Scroll"),           "<alt>S", NULL,                           0},
  {N_("/Tools/Text"),             "<alt>T", NULL,                           0},
  {N_("/Tools/Box"),              "<alt>R", NULL,                           0},
  {N_("/Tools/Ellipse"),          "<alt>E", NULL,                           0},
  {N_("/Tools/Polygon"),          "<alt>P", NULL,                           0},
  {N_("/Tools/Beziergon"),        "<alt>B", NULL,                           0},
  {N_("/Tools/---"),              NULL,     NULL,       0, "<Separator>" },
  {N_("/Tools/Line"),             "<alt>L", NULL,                           0},
  {N_("/Tools/Arc"),              "<alt>A", NULL,                           0},
  {N_("/Tools/Zigzagline"),       "<alt>Z", NULL,                           0},
  {N_("/Tools/Polyline"),         NULL,     NULL,                           0},
  {N_("/Tools/Bezierline"),       "<alt>C", NULL,                           0},
  {N_("/Tools/---"),              NULL,     NULL,       0, "<Separator>" },
  {N_("/Tools/Image"),            "<alt>I", NULL,                           0},
  {N_("/_Input Methods"),         NULL,     NULL,               0, "<Branch>"},
  {   "/Input Methods/tearoff",   NULL,     NULL,               0, "<Tearoff>" },
  {N_("/_Help"),               NULL,         NULL,       0,    "<Branch>" },
  {   "/Help/tearoff",         NULL,         NULL,       0,   "<Tearoff>" },
  {N_("/Help/_Contents"),        "F1",         help_manual_callback,      0,
      "<StockItem>", GTK_STOCK_HELP },
  {N_("/Help/---"),            NULL,         NULL,       0, "<Separator>" },
  {N_("/Help/_About..."),      NULL,         help_about_callback,       0,
   /* FOR 2.6 "<StockItem>", GTK_STOCK_ABOUT */},
};

static int display_nmenu_items = (sizeof(display_menu_items) /
				  sizeof(display_menu_items[0]));


/* need initialisation? */
static gboolean initialise = TRUE;

/* the actual menus */
static GtkWidget *toolbox_menubar = NULL;
static GtkAccelGroup *toolbox_accels = NULL;
static GtkWidget *display_menus = NULL;
static GtkAccelGroup *display_accels = NULL;

static GtkItemFactory *toolbox_item_factory = NULL;
static GtkItemFactory *display_item_factory = NULL;

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


/*
  Purpose: set the generic callback for all the items in the menu "Tools"
 */
static void 
menus_set_tools_callback (const char * menu_name, GtkItemFactory *item_factory)
{
    gint i, len;
    GString *path;
    GtkMenuItem *menuitem;
    
    path = g_string_new(menu_name);
    g_string_append(path, "/Tools/");
    len = path->len;
    for (i = 0; i < num_tools; i++) {
	g_string_append(path, tool_data[i].menuitem_name);
	menuitem = menus_get_item_from_path(path->str, item_factory);
	if (menuitem != NULL)
	    g_signal_connect(GTK_OBJECT(menuitem), "activate",
			     G_CALLBACK(tool_menu_select),
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
  GtkWidget *menuitem;
  gchar *accelfilename;
  GList *cblist;

  if (!initialise)
    return;
  
  initialise = FALSE;

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
    g_signal_connect(GTK_OBJECT(newitem), "activate",
		     G_CALLBACK(plugin_callback), cbf);
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
	menus_set_tools_callback ("<DisplayMBar>", *display_mbar_item_factory);
    }
}


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


/*
  Get a menu item widget from a path. 
  In case of an item in the <DisplayMBar> menu bar, provide the corresponding 
  item factory: ddisp->mbar_item_factory.
 */
GtkMenuItem *
menus_get_item_from_path (char *path, GtkItemFactory *item_factory)
{
  GtkWidget *wid = NULL;
  GtkMenuItem *widget = NULL;

  if (display_item_factory) {
    wid =
      gtk_item_factory_get_item(display_item_factory, path);
    if (wid != NULL) widget = GTK_MENU_ITEM(wid);
  }
  
  if ((widget == NULL) && (item_factory)) {
      wid = 
	gtk_item_factory_get_item(item_factory, path);
    if (wid != NULL) widget = GTK_MENU_ITEM(wid);
  }
  
  if (widget == NULL) {
    wid = 
      gtk_item_factory_get_item(toolbox_item_factory, path);
    if (wid != NULL) widget = GTK_MENU_ITEM(wid);
  }

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
    
    path = g_string_new (display);
    g_string_append (path,"/Edit/Copy");
    items->copy = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Edit/Cut");
    items->cut = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Edit/Paste");
    items->paste = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Edit/Delete");
    items->edit_delete = menus_get_item_from_path(path->str, factory);

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
    g_string_append (g_string_assign(path, display),"/Objects/Send Backwards");
    items->send_backwards = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Bring Forwards");
    items->bring_forwards = menus_get_item_from_path(path->str, factory);
  
    g_string_append (g_string_assign(path, display),"/Objects/Group");
    items->group = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Ungroup");
    items->ungroup = menus_get_item_from_path(path->str, factory);

    g_string_append (g_string_assign(path, display),"/Objects/Parent");
    items->parent = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Unparent");
    items->unparent = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Unparent Children");
    items->unparent_children = menus_get_item_from_path(path->str, factory);

    g_string_append (g_string_assign(path, display),"/Objects/Align/Left");
    items->align_h_l = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align/Center");
    items->align_h_c = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align/Right");
    items->align_h_r = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align/Spread Out Horizontally");
    items->align_h_e = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align/Adjacent");
    items->align_h_a = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align/Top");
    items->align_v_t = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align/Middle");
    items->align_v_c = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align/Bottom");
    items->align_v_b = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align/Spread Out Vertically");
    items->align_v_e = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Align/Stacked");
    items->align_v_a = menus_get_item_from_path(path->str, factory);
    g_string_append (g_string_assign(path, display),"/Objects/Properties...");
    items->properties = menus_get_item_from_path(path->str, factory);
    
    g_string_free (path,TRUE);
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
