/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 * Copyright (C) 2006 Robert Staudinger <robert.staudinger@gmail.com>
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
#include "dia-app-icons.h"

#define DIA_STOCK_GROUP "dia-stock-group"
#define DIA_STOCK_UNGROUP "dia-stock-ungroup"

#define DIA_SHOW_TEAROFFS TRUE

static void plugin_callback (GtkWidget *widget, gpointer data);

/* Actions common to toolbox and diagram window */
static const GtkActionEntry common_entries[] =
{
  { "File", NULL, N_("_File"), NULL, NULL, NULL },
    { "FileNew", GTK_STOCK_NEW, NULL, "<control>N", NULL, G_CALLBACK (file_new_callback) },
    { "FileOpen", GTK_STOCK_OPEN, NULL,"<control>O", NULL, G_CALLBACK (file_open_callback) },
    { "FileQuit", GTK_STOCK_QUIT, NULL, "<control>Q", NULL, G_CALLBACK (file_quit_callback) }, 
  { "Help", NULL, N_("_Help"), NULL, NULL, NULL },
    { "HelpContents", GTK_STOCK_HELP, NULL, NULL, NULL, G_CALLBACK (help_manual_callback) },
    { "HelpAbout", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (help_about_callback) }
};

/* Actions for toolbox menu */
static const GtkActionEntry toolbox_entries[] = 
{
    { "FileSheets", NULL, N_("Sheets and Objects..."), "F9", NULL, G_CALLBACK (sheets_dialog_show_callback) },
    { "FilePrefs", GTK_STOCK_PREFERENCES, NULL, NULL, NULL, G_CALLBACK (file_preferences_callback) },
    { "FilePlugins", NULL, N_("Plugins..."), NULL, NULL, G_CALLBACK (file_plugins_callback) }
};

/* Toggle-Actions for toolbox menu */
static const GtkToggleActionEntry toolbox_toggle_entries[] = 
{
    { "FileTree", NULL, N_("_Diagram tree..."), "F8", NULL, G_CALLBACK (diagtree_show_callback) }
};

/* Actions for diagram window */
static const GtkActionEntry display_entries[] =
{
    { "FileSave", GTK_STOCK_SAVE, NULL, "<control>S", NULL, G_CALLBACK (file_save_callback) },
    { "FileSaveas", GTK_STOCK_SAVE_AS, NULL, "<control><shift>S", NULL, G_CALLBACK (file_save_as_callback) },
    { "FileExport", GTK_STOCK_CONVERT, N_("_Export ..."), NULL, NULL, G_CALLBACK (file_export_callback) },
    { "FilePagesetup", NULL, N_("Page Set_up..."), NULL, NULL, G_CALLBACK (file_pagesetup_callback) },
    { "FilePrint", GTK_STOCK_PRINT, NULL, "<control>P", NULL, G_CALLBACK (file_print_callback) },
    { "FileClose", GTK_STOCK_CLOSE, NULL, "<control>W", NULL, G_CALLBACK (file_close_callback) },

  { "Edit", NULL, N_("_Edit"), NULL, NULL, NULL },
    { "EditUndo", GTK_STOCK_UNDO, NULL, "<control>Z", NULL, G_CALLBACK (edit_undo_callback) },
    { "EditRedo", GTK_STOCK_REDO, NULL, "<control><shift>Z", NULL, G_CALLBACK (edit_redo_callback) },

    { "EditCopy", GTK_STOCK_COPY, NULL, "<control>C", NULL, G_CALLBACK (edit_copy_callback) },
    { "EditCut", GTK_STOCK_CUT, NULL, "<control>X", NULL, G_CALLBACK (edit_cut_callback) },
    { "EditPaste", GTK_STOCK_PASTE, NULL, "<control>V", NULL, G_CALLBACK (edit_paste_callback) },
    { "EditDuplicate", NULL, N_("_Duplicate"), "<control>D", NULL, G_CALLBACK (edit_duplicate_callback) },
    { "EditDelete", GTK_STOCK_DELETE, NULL, "Delete", NULL, G_CALLBACK (edit_delete_callback) },

    /* the following used to bind to <control><shift>C which collides with Unicode input. 
     * <control>>alt> doesn't work either */
    { "EditCopytext", NULL, N_("Copy Text"), NULL, NULL, G_CALLBACK (edit_copy_text_callback) },
    { "EditCuttext", NULL, N_("Cut Text"), "<control><shift>X", NULL, G_CALLBACK (edit_cut_text_callback) },
    { "EditPastetext", NULL, N_("Paste _Text"), "<control><shift>V", NULL, G_CALLBACK (edit_paste_text_callback) },

  { "Diagram", NULL, N_("_Diagram"), NULL, NULL, NULL }, 
    { "DiagramProperties", GTK_STOCK_PROPERTIES, NULL, "<shift><alt>Return", NULL, G_CALLBACK (view_diagram_properties_callback) },
    { "DiagramLayers", NULL, N_("_Layers..."), NULL, NULL, G_CALLBACK (dialogs_layers_callback) },

  { "View", NULL, N_("_View"), NULL, NULL, NULL },
    { "ViewZoomin", GTK_STOCK_ZOOM_IN, NULL, "<control>plus", NULL, G_CALLBACK (view_zoom_in_callback) },
    { "ViewZoomout", GTK_STOCK_ZOOM_OUT, NULL, "<control>minus", NULL, G_CALLBACK (view_zoom_out_callback) },
    { "ViewZoom", NULL, N_("_Zoom"), NULL, NULL, NULL },
      { "ViewZoom16000", NULL, N_("1600%"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom8000", NULL, N_("800%"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom4000", NULL, N_("400%"), "<alt>4", NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom2830", NULL, N_("283"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom2000", NULL, N_("200"), "<alt>2", NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom1410", NULL, N_("141"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom1000", GTK_STOCK_ZOOM_100, NULL, "<alt>1", NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom850", NULL, N_("85"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom707", NULL, N_("70.7"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom500", NULL, N_("50"), "<alt>5", NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom354", NULL, N_("35.4"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom250", NULL, N_("25"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },

  /* "display_toggle_entries" items go here */

    { "ViewNewview", NULL, N_("New _View"), NULL, NULL, G_CALLBACK (view_new_view_callback) },
    { "ViewCloneview", NULL, N_("C_lone View"), NULL, NULL, G_CALLBACK (view_clone_view_callback) },
    /* Show All, Best Fit.  Same as the Gimp, Ctrl+E */
    { "ViewShowall", GTK_STOCK_ZOOM_FIT, NULL, "<control>E", NULL, G_CALLBACK (view_show_all_callback) },
    { "ViewRedraw", GTK_STOCK_REFRESH, NULL, NULL, NULL, G_CALLBACK (view_redraw_callback) },

  { "Objects", NULL, N_("_Objects"), NULL, NULL },
    { "ObjectsSendtoback", GTK_STOCK_GOTO_BOTTOM, N_("Send to _Back"), "<control>B", NULL, G_CALLBACK (objects_place_under_callback) },
    { "ObjectsBringtofront", GTK_STOCK_GOTO_TOP, N_("Bring to _Front"), "<control>F", NULL, G_CALLBACK (objects_place_over_callback) },
    { "ObjectsSendbackwards", GTK_STOCK_GO_DOWN, N_("Send Backwards"), NULL, NULL, G_CALLBACK (objects_place_down_callback) },
    { "ObjectsBringforwards", GTK_STOCK_GO_UP, N_("Bring Forwards"), NULL, NULL, G_CALLBACK (objects_place_up_callback) },

    { "ObjectsGroup", DIA_STOCK_GROUP, N_("_Group"), "<control>G", NULL, G_CALLBACK (objects_group_callback) },
    /* deliberately not using Ctrl+U for Ungroup */
    { "ObjectsUngroup", DIA_STOCK_UNGROUP, N_("_Ungroup"), "<control><shift>G", NULL, G_CALLBACK (objects_ungroup_callback) }, 

    { "ObjectsParent", NULL, N_("_Parent"), "<control>L", NULL, G_CALLBACK (objects_parent_callback) },
    { "ObjectsUnparent", NULL, N_("_Unparent"), "<control><shift>L", NULL, G_CALLBACK (objects_unparent_callback) },
    { "ObjectsUnparentchildren", NULL, N_("_Unparent Children"), NULL, NULL, G_CALLBACK (objects_unparent_children_callback) },

    { "ObjectsAlign", NULL, N_("Align"), NULL, NULL, NULL },
      { "ObjectsAlignLeft", GTK_STOCK_JUSTIFY_LEFT, NULL, "<alt><shift>L", NULL, G_CALLBACK (objects_align_h_callback) },
      { "ObjectsAlignCenter", GTK_STOCK_JUSTIFY_CENTER, NULL, "<alt><shift>C", NULL, G_CALLBACK (objects_align_h_callback) },
      { "ObjectsAlignRight", GTK_STOCK_JUSTIFY_RIGHT, NULL, "<alt><shift>R", NULL, G_CALLBACK (objects_align_h_callback) },

      { "ObjectsAlignTop", NULL, N_("Top"), "<alt><shift>T", NULL, G_CALLBACK (objects_align_v_callback) },
      { "ObjectsAlignMiddle", NULL, N_("Middle"), "<alt><shift>M", NULL, G_CALLBACK (objects_align_v_callback) },
      { "ObjectsAlignBottom", NULL, N_("Bottom"), "<alt><shift>B", NULL, G_CALLBACK (objects_align_v_callback) },

      { "ObjectsAlignSpreadouthorizontally", NULL, N_("Spread Out Horizontally"), "<alt><shift>H", NULL, G_CALLBACK (objects_align_h_callback) },
      { "ObjectsAlignSpreadoutvertically", NULL, N_("Spread Out Vertically"), "<alt><shift>V", NULL, G_CALLBACK (objects_align_v_callback) },
      { "ObjectsAlignAdjacent", NULL, N_("Adjacent"), "<alt><shift>A", NULL, G_CALLBACK (objects_align_h_callback) },
      { "ObjectsAlignStacked", NULL, N_("Stacked"), "<alt><shift>S", NULL, G_CALLBACK (objects_align_v_callback) },

      { "ObjectsProperties", GTK_STOCK_PROPERTIES, NULL, "<alt>Return", NULL, G_CALLBACK (dialogs_properties_callback) },

  { "Select", NULL, N_("_Select"), NULL, NULL, NULL },
    { "SelectAll", NULL, N_("All"), "<control>A", NULL, G_CALLBACK (select_all_callback) },
    { "SelectNone", NULL, N_("None"), NULL, NULL, G_CALLBACK (select_none_callback) },
    { "SelectInvert", NULL, N_("Invert"), "<control>I", NULL, G_CALLBACK (select_invert_callback) },

    { "SelectTransitive", NULL, N_("Transitive"), "<control>T", NULL, G_CALLBACK (select_transitive_callback) },
    { "SelectConnected", NULL, N_("Connected"), "<control><shift>T", NULL, G_CALLBACK (select_connected_callback) },
    { "SelectSametype", NULL, N_("Same Type"), NULL, NULL, G_CALLBACK (select_same_type_callback) },

    /* display_select_radio_entries go here */

    { "SelectBy", NULL, N_("Select By"), NULL, NULL, NULL },

  { "InputMethods", NULL, N_("_Input Methods"), NULL, NULL, NULL },

  { "Dialogs", NULL, N_("D_ialogs"), NULL, NULL, NULL },

  { "Debug", NULL, N_("D_ebug"), NULL, NULL, NULL }
};

/* Standard-Tool entries */
static const GtkActionEntry tool_entries[] = 
{
  { "Tools", NULL, N_("_Tools"), NULL, NULL, NULL },
    { "ToolsModify", NULL, N_("Modify"), "<alt>N", NULL, NULL },
    { "ToolsMagnify", NULL, N_("Magnify"), "<alt>M", NULL, NULL },
    { "ToolsScroll", NULL, N_("Scroll"), "<alt>S", NULL, NULL },
    { "ToolsText", NULL, N_("Text"), "<alt>T", NULL, NULL },
    { "ToolsBox", NULL, N_("Box"), "<alt>R", NULL, NULL },
    { "ToolsEllipse", NULL, N_("Ellipse"), "<alt>E", NULL, NULL },
    { "ToolsPolygon", NULL, N_("Polygon"), "<alt>P", NULL, NULL },
    { "ToolsBeziergon", NULL, N_("Beziergon"), "<alt>B", NULL, NULL },

    { "ToolsLine", NULL, N_("Line"), "<alt>L", NULL, NULL },
    { "ToolsArc", NULL, N_("Arc"), "<alt>A", NULL, NULL },
    { "ToolsZigzagline", NULL, N_("Zigzagline"), "<alt>Z", NULL, NULL },
    { "ToolsPolyline", NULL, N_("Polyline"), NULL, NULL },
    { "ToolsBezierline", NULL, N_("Bezierline"), "<alt>C", NULL, NULL },

    { "ToolsImage", NULL, N_("Image"), "<alt>I", NULL, NULL },
};

/* Toggle-Actions for diagram window */
static const GtkToggleActionEntry display_toggle_entries[] = 
{
#ifdef GTK_STOCK_FULLSCREEN /* added with gtk+-2.8, but no reason to depend on it */
    { "ViewFullscreen", GTK_STOCK_FULLSCREEN, NULL, "F11", NULL, G_CALLBACK (view_fullscreen_callback) },
#else
    { "ViewFullscreen", NULL, N_("Fullscr_een"), "F11", NULL, G_CALLBACK (view_fullscreen_callback) },
#endif
#ifdef HAVE_LIBART  
    { "ViewAntialiased", NULL, N_("_AntiAliased"), NULL, NULL, G_CALLBACK (view_aa_callback) },
#endif
    { "ViewShowgrid", NULL, N_("Show _Grid"), NULL, NULL, G_CALLBACK (view_visible_grid_callback) },
    { "ViewSnaptogrid", NULL, N_("_Snap To Grid"), NULL, NULL, G_CALLBACK (view_snap_to_grid_callback) },
    { "ViewSnaptoobjects", NULL, N_("Snap To _Objects"), NULL, NULL, G_CALLBACK (view_snap_to_objects_callback) },
    { "ViewShowrulers", NULL, N_("Show _Rulers"), NULL, NULL, G_CALLBACK (view_toggle_rulers_callback)  },
    { "ViewShowconnectionpoints", NULL, N_("Show _Connection Points"), NULL, NULL, G_CALLBACK (view_show_cx_pts_callback) }
};

/* Radio-Actions for the diagram window's "Select"-Menu */
static const GtkRadioActionEntry display_select_radio_entries[] =
{
  { "SelectReplace", NULL, N_("Replace"), NULL, NULL, SELECT_REPLACE },
  { "SelectUnion", NULL, N_("Union"), NULL, NULL, SELECT_UNION },
  { "SelectIntersection", NULL, N_("Intersection"), NULL, NULL, SELECT_INTERSECTION },
  { "SelectRemove", NULL, N_("Remove"), NULL, NULL, SELECT_REMOVE },
  /* Cannot also be called Invert, duplicate names caused keybinding problems */
  { "SelectInverse", NULL, N_("Inverse"), NULL, NULL, SELECT_INVERT }
};

/* need initialisation? */
static gboolean initialise = TRUE;

/* toolbox */
static GtkUIManager *toolbox_ui_manager = NULL;
static GtkActionGroup *toolbox_actions = NULL;
static GtkAccelGroup *toolbox_accels = NULL;
static GtkWidget *toolbox_menubar = NULL;

GtkActionGroup *recent_actions = NULL;
static GSList *recent_merge_ids = NULL;

/* diagram */
static GtkUIManager *display_ui_manager = NULL;
static GtkActionGroup *display_actions = NULL;
static GtkActionGroup *display_tool_actions = NULL;
static GtkAccelGroup *display_accels = NULL;
static GtkWidget *display_menubar = NULL;

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
 * Initialise tool actions.
 * The caller owns the return value.
 */
static GtkActionGroup *
create_tool_actions (void)
{
  GtkActionGroup *actions;
  GtkAction      *action;
  int           i;
  gchar          *name;
  static guint    cnt = 0;

  name = g_strdup_printf ("tool-actions-%d", cnt);
  actions = gtk_action_group_new (name);
  gtk_action_group_set_translation_domain (actions, NULL);
  g_free (name);
  name = NULL;
  cnt++;

  gtk_action_group_add_actions (actions, tool_entries, 
				G_N_ELEMENTS (tool_entries), NULL);

  for (i = 0; i < num_tools; i++) {
    action = gtk_action_group_get_action (actions, tool_data[i].action_name);
    if (action != NULL) {
      g_signal_connect (G_OBJECT (action), "activate",
			G_CALLBACK (tool_menu_select),
			&tool_data[i].callback_data);
    }
    else {
      g_warning ("couldn't find tool menu item %s", tool_data[i].action_name);
    }
  }
  return actions;
}

/* initialize callbacks from plug-ins */
static void
add_plugin_actions (GtkUIManager *ui_manager)
{
  GtkActionGroup    *actions;
  GtkAction         *action;
  GList             *cblist;
  DiaCallbackFilter *cbf = NULL;
  gchar             *name;
  guint              id;
  static guint       cnt = 0;

  name = g_strdup_printf ("plugin-actions-%d", cnt);
  actions = gtk_action_group_new (name);
  gtk_action_group_set_translation_domain (actions, NULL);
  g_free (name);
  name = NULL;
  cnt++;

  gtk_ui_manager_insert_action_group (ui_manager, actions, 5 /* "back" */);
  g_object_unref (actions);

  for (cblist = filter_get_callbacks(); cblist; cblist = cblist->next) {

    cbf = cblist->data;

    if (cbf == NULL) {
      g_warning ("missing DiaCallbackFilter instance");
      continue;
    }

    if (cbf->action == NULL) {
      g_warning ("Plugin '%s': doesn't specify action. Loading failed.", 
		cbf->description);
      continue;
    }

    if (0 == strncmp (cbf->menupath, TOOLBOX_MENU, strlen (TOOLBOX_MENU))) {
      /* hook for toolbox, skip */
      continue;
    }

    action = gtk_action_new (cbf->action, cbf->description, NULL, NULL);
    g_signal_connect (G_OBJECT (action), "activate", 
		      G_CALLBACK (plugin_callback), (gpointer) cbf);

    gtk_action_group_add_action (actions, action);
    g_object_unref (G_OBJECT (action));

    id = gtk_ui_manager_new_merge_id (ui_manager);
    gtk_ui_manager_add_ui (ui_manager, id, 
			   cbf->menupath, 
			   cbf->description, 
			   cbf->action, 
			   GTK_UI_MANAGER_AUTO, 
			   FALSE);
  }
}

static void
register_stock_icons (void)
{
  GtkIconFactory *factory;
  GtkIconSet     *set;
  GdkPixbuf      *pixbuf;
  GError         *err = NULL;

  factory = gtk_icon_factory_new ();

  pixbuf = gdk_pixbuf_new_from_inline (sizeof (dia_group_icon), dia_group_icon, FALSE, &err);
  if (err) {
    g_warning (err->message);
    g_error_free (err);
    err = NULL;
  }
  set = gtk_icon_set_new_from_pixbuf (pixbuf);
  gtk_icon_factory_add (factory, DIA_STOCK_GROUP, set);
  g_object_unref (pixbuf);
  pixbuf = NULL;

  pixbuf = gdk_pixbuf_new_from_inline (sizeof (dia_ungroup_icon), dia_ungroup_icon, FALSE, &err);
  if (err) {
    g_warning (err->message);
    g_error_free (err);
    err = NULL;
  }
  set = gtk_icon_set_new_from_pixbuf (pixbuf);
  gtk_icon_factory_add (factory, DIA_STOCK_UNGROUP, set);
  g_object_unref (pixbuf); 
  pixbuf = NULL;

  gtk_icon_factory_add_default (factory);
  g_object_unref (factory);
  factory = NULL;
}

static gchar*
build_ui_filename (const gchar* name)
{
  gchar* uifile;

  if (g_getenv ("DIA_BASE_PATH") != NULL) {
    /* a small hack cause the final destination and the local path differ */
    const gchar* p = strrchr (name, '/');
    if (p != NULL)
      name = p+1;
    uifile = g_build_filename (g_getenv ("DIA_BASE_PATH"), "data", name, NULL);
  } else
    uifile = dia_get_data_directory (name);

  return uifile;
}

static void
menus_init(void)
{
  DiaCallbackFilter 	*cbf;
  GtkActionGroup 	*plugin_actions;
  GtkAction 		*action;
  gchar 		*accelfilename;
  GList 		*cblist;
  guint 		 id;
  GError 		*error = NULL;
  gchar			*uifile;

  if (!initialise)
    return;
  
  initialise = FALSE;

  register_stock_icons ();

  /* the toolbox menu */
  toolbox_actions = gtk_action_group_new ("toolbox-actions");
  gtk_action_group_set_translation_domain (toolbox_actions, NULL);
  gtk_action_group_add_actions (toolbox_actions, common_entries, 
                G_N_ELEMENTS (common_entries), NULL);
  gtk_action_group_add_actions (toolbox_actions, toolbox_entries, 
                G_N_ELEMENTS (toolbox_entries), NULL);
  gtk_action_group_add_toggle_actions (toolbox_actions, toolbox_toggle_entries,
                G_N_ELEMENTS (toolbox_toggle_entries), 
                NULL);

  toolbox_ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_set_add_tearoffs (toolbox_ui_manager, DIA_SHOW_TEAROFFS);
  gtk_ui_manager_insert_action_group (toolbox_ui_manager, toolbox_actions, 0);
  uifile = build_ui_filename ("ui/toolbox-ui.xml");
  if (!gtk_ui_manager_add_ui_from_file (toolbox_ui_manager, uifile, &error)) {
    g_warning ("building menus failed: %s", error->message);
    g_error_free (error);
    error = NULL;
  }
  g_free (uifile);

  toolbox_accels = gtk_ui_manager_get_accel_group (toolbox_ui_manager);
  toolbox_menubar = gtk_ui_manager_get_widget (toolbox_ui_manager, "/ToolboxMenu");


  /* the display menu */
  display_actions = gtk_action_group_new ("display-actions");
  gtk_action_group_set_translation_domain (display_actions, NULL);
  gtk_action_group_add_actions (display_actions, common_entries, 
                G_N_ELEMENTS (common_entries), NULL);
  gtk_action_group_add_actions (display_actions, display_entries, 
                G_N_ELEMENTS (display_entries), NULL);
  gtk_action_group_add_toggle_actions (display_actions, display_toggle_entries,
                G_N_ELEMENTS (display_toggle_entries), 
                NULL);
  gtk_action_group_add_radio_actions (display_actions,
                display_select_radio_entries,
                G_N_ELEMENTS (display_select_radio_entries),
                1,
                G_CALLBACK (select_style_callback),
                NULL);

  display_tool_actions = create_tool_actions ();

  display_ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_set_add_tearoffs (display_ui_manager, DIA_SHOW_TEAROFFS);
  gtk_ui_manager_insert_action_group (display_ui_manager, display_actions, 0);
  gtk_ui_manager_insert_action_group (display_ui_manager, display_tool_actions, 0);
  uifile = build_ui_filename ("ui/popup-ui.xml");
  /* TODO it would be more elegant if we had only one definition of the 
   * menu hierarchy and merge it into a popup somehow. */
  if (!gtk_ui_manager_add_ui_from_file (display_ui_manager, uifile, &error)) {
    g_warning ("building menus failed: %s", error->message);
    g_error_free (error);
    error = NULL;
  }
  g_free (uifile);

  display_accels = gtk_ui_manager_get_accel_group (display_ui_manager);
  display_menubar = gtk_ui_manager_get_widget (display_ui_manager, "/DisplayMenu");
  g_assert (display_menubar);


  /* plugin menu hooks */  
  plugin_actions = gtk_action_group_new ("toolbox-plugin-actions");
  gtk_action_group_set_translation_domain (plugin_actions, NULL);
  gtk_ui_manager_insert_action_group (toolbox_ui_manager, 
                    plugin_actions, 5 /* "back" */);
  g_object_unref (plugin_actions);

  for (cblist = filter_get_callbacks(); cblist; cblist = cblist->next) {

    cbf = cblist->data;

    if (cbf == NULL) {
      g_warning ("missing DiaCallbackFilter instance");
      continue;
    }

    if (cbf->action == NULL) {
      g_warning ("Plugin '%s': doesn't specify action. Loading failed.", 
		cbf->description);
      continue;
    }

    if (0 != strncmp (cbf->menupath, TOOLBOX_MENU, strlen (TOOLBOX_MENU))) {
      /* hook for display, skip */
      continue;
    }

    action = gtk_action_new (cbf->action, cbf->description, 
                             NULL, NULL);
    g_signal_connect (G_OBJECT (action), "activate", 
                      G_CALLBACK (plugin_callback), 
                      (gpointer) cbf);

    gtk_action_group_add_action (plugin_actions, action);
    g_object_unref (G_OBJECT (action));

    id = gtk_ui_manager_new_merge_id (toolbox_ui_manager);
    gtk_ui_manager_add_ui (toolbox_ui_manager, id, 
                           cbf->menupath, 
                           cbf->description, 
                           cbf->action, 
                           GTK_UI_MANAGER_AUTO, 
                           FALSE);
  }

  add_plugin_actions (display_ui_manager);

  /* load accelerators and prepare to later save them */
  accelfilename = dia_config_filename("menurc");
  
  if (accelfilename) {
    gtk_accel_map_load(accelfilename);
    g_free(accelfilename);
  }
  gtk_quit_add(1, save_accels, NULL);
}

void
menus_get_toolbox_menubar (GtkWidget     **menubar,
			   GtkAccelGroup **accel_group)
{
  if (initialise)
    menus_init();

  if (menubar)
    *menubar = toolbox_menubar;
  if (accel_group)
    *accel_group = toolbox_accels;
}

GtkWidget * 
menus_get_display_popup (void)
{
  if (initialise)
    menus_init();

  return display_menubar;
}

GtkAccelGroup * 
menus_get_display_accels (void)
{
  if (initialise)
    menus_init();

  return display_accels;
}

GtkWidget *
menus_create_display_menubar (GtkUIManager   **ui_manager, 
			      GtkActionGroup **actions)
{
  GtkActionGroup *tool_actions;
  GtkWidget      *menu_bar;
  GError         *error = NULL;
  gchar          *uifile;

  *actions = gtk_action_group_new ("display-actions");
  gtk_action_group_set_translation_domain (*actions, NULL);

  gtk_action_group_add_actions (*actions, common_entries, 
                G_N_ELEMENTS (common_entries), NULL);
  gtk_action_group_add_actions (*actions, display_entries, 
                G_N_ELEMENTS (display_entries), NULL);
  gtk_action_group_add_toggle_actions (*actions, display_toggle_entries,
                G_N_ELEMENTS (display_toggle_entries), 
                NULL);
  gtk_action_group_add_radio_actions (*actions,
                    display_select_radio_entries,
                    G_N_ELEMENTS (display_select_radio_entries),
                    1,
                    G_CALLBACK (select_style_callback),
                    NULL);

  tool_actions = create_tool_actions (); 

  *ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_set_add_tearoffs (*ui_manager, DIA_SHOW_TEAROFFS);
  gtk_ui_manager_insert_action_group (*ui_manager, *actions, 0);
  gtk_ui_manager_insert_action_group (*ui_manager, tool_actions, 0);
  g_object_unref (G_OBJECT (tool_actions));
  
  uifile = build_ui_filename ("ui/display-ui.xml");
  if (!gtk_ui_manager_add_ui_from_file (*ui_manager, uifile, &error)) {
    g_warning ("building menus failed: %s", error->message);
    g_error_free (error);
    g_free (uifile);
    return NULL;
  }
  g_free (uifile);

  add_plugin_actions (*ui_manager);

  menu_bar = gtk_ui_manager_get_widget (*ui_manager, "/DisplayMenu");
  return menu_bar;
}

GtkAccelGroup *
menus_get_accel_group ()
{
  return toolbox_accels;
}

GtkActionGroup *
menus_get_action_group ()
{
  return toolbox_actions;
}

GtkAction *
menus_get_action (const gchar *name)
{
  GtkAction *action;

  action = gtk_action_group_get_action (toolbox_actions, name);
  if (action == NULL) {
    action = gtk_action_group_get_action (display_actions, name);
  }

  return action;
}

void
menus_set_recent (GtkActionGroup *actions)
{
  GList *list;
  guint id;

  if (recent_actions) {
    menus_clear_recent ();
  }

  list = gtk_action_group_list_actions (actions);
  g_return_if_fail (list);

  recent_actions = actions;
  g_object_ref (G_OBJECT (recent_actions));
  gtk_ui_manager_insert_action_group (toolbox_ui_manager, 
                    recent_actions, 
                    10 /* insert at back */ );

  do {
    id = gtk_ui_manager_new_merge_id (toolbox_ui_manager);
    recent_merge_ids = g_slist_prepend (recent_merge_ids, (gpointer) id);

    gtk_ui_manager_add_ui (toolbox_ui_manager, id, 
                 "/ToolboxMenu/File/FileRecentEnd", 
                 gtk_action_get_name (GTK_ACTION (list->data)), 
                 gtk_action_get_name (GTK_ACTION (list->data)), 
                 GTK_UI_MANAGER_AUTO, 
                 TRUE);

  } while (NULL != (list = list->next));
}

void
menus_clear_recent (void)
{
  GSList *id;

  g_return_if_fail (recent_merge_ids);

  id = recent_merge_ids;
  do {
    gtk_ui_manager_remove_ui (toolbox_ui_manager, (guint) id->data);

  } while (NULL != (id = id->next));

  g_slist_free (recent_merge_ids);
  recent_merge_ids = NULL;

  gtk_ui_manager_remove_action_group (toolbox_ui_manager, recent_actions);
  g_object_unref (G_OBJECT (recent_actions));
  recent_actions = NULL;
}

void 
menus_initialize_updatable_items (UpdatableMenuItems *items, GtkActionGroup *actions)
{
  if (actions == NULL) {
    actions = display_actions;
  }

    items->copy = gtk_action_group_get_action (actions, "EditCopy");
    items->cut = gtk_action_group_get_action (actions, "EditCut");
    items->paste = gtk_action_group_get_action (actions, "EditPaste");
    items->edit_delete = gtk_action_group_get_action (actions, "EditDelete");
    items->edit_duplicate = gtk_action_group_get_action (actions, "EditDuplicate");

    items->copy_text = gtk_action_group_get_action (actions, "EditCopytext");
    items->cut_text = gtk_action_group_get_action (actions, "EditCuttext");
    items->paste_text = gtk_action_group_get_action (actions, "EditPastetext");

    items->send_to_back = gtk_action_group_get_action (actions, "ObjectsSendtoback");
    items->bring_to_front = gtk_action_group_get_action (actions, "ObjectsBringtofront");
    items->send_backwards = gtk_action_group_get_action (actions, "ObjectsSendbackwards");
    items->bring_forwards = gtk_action_group_get_action (actions, "ObjectsBringforwards");
  
    items->group = gtk_action_group_get_action (actions, "ObjectsGroup");
    items->ungroup = gtk_action_group_get_action (actions, "ObjectsUngroup");

    items->parent = gtk_action_group_get_action (actions, "ObjectsParent");
    items->unparent = gtk_action_group_get_action (actions, "ObjectsUnparent");
    items->unparent_children = gtk_action_group_get_action (actions, "ObjectsUnparentchildren");

    items->align_h_l = gtk_action_group_get_action (actions, "ObjectsAlignLeft");
    items->align_h_c = gtk_action_group_get_action (actions, "ObjectsAlignCenter");
    items->align_h_r = gtk_action_group_get_action (actions, "ObjectsAlignRight");
    items->align_h_e = gtk_action_group_get_action (actions, "ObjectsAlignSpreadouthorizontally");
    items->align_h_a = gtk_action_group_get_action (actions, "ObjectsAlignAdjacent");
    items->align_v_t = gtk_action_group_get_action (actions, "ObjectsAlignTop");
    items->align_v_c = gtk_action_group_get_action (actions, "ObjectsAlignMiddle");
    items->align_v_b = gtk_action_group_get_action (actions, "ObjectsAlignBottom");
    items->align_v_e = gtk_action_group_get_action (actions, "ObjectsAlignSpreadoutvertically");
    items->align_v_a = gtk_action_group_get_action (actions, "ObjectsAlignStacked");

    items->properties = gtk_action_group_get_action (actions, "ObjectsProperties");
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
