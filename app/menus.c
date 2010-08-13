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

#undef GTK_DISABLE_DEPRECATED /* watch out sheets.h dragging in gnome.h */
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
#include "find-and-replace.h"
#include "plugin-manager.h"
#include "select.h"
#include "dia_dirs.h"
#include "object_ops.h"
#include "sheets.h"
#include "dia-app-icons.h"
#include "widgets.h"
#include "preferences.h"

#define DIA_STOCK_GROUP "dia-stock-group"
#define DIA_STOCK_UNGROUP "dia-stock-ungroup"
#define DIA_STOCK_LAYER_ADD "dia-stock-layer-add"
#define DIA_STOCK_LAYER_RENAME "dia-stock-layer-rename"
#define DIA_STOCK_OBJECTS_LAYER_ABOVE "dia-stock-objects-layer-above"
#define DIA_STOCK_OBJECTS_LAYER_BELOW "dia-stock-objects-layer-below"
#define DIA_STOCK_LAYERS "dia-stock-layers"

#define DIA_SHOW_TEAROFFS TRUE

/* Integrated UI Toolbar Constants */
#define DIA_INTEGRATED_TOOLBAR_ZOOM_COMBO  "dia-integrated-toolbar-zoom-combo_entry"
#define DIA_INTEGRATED_TOOLBAR_SNAP_GRID   "dia-integrated-toolbar-snap-grid"
#define DIA_INTEGRATED_TOOLBAR_OBJECT_SNAP "dia-integrated-toolbar-object-snap"

#define ZOOM_FIT        _("Fit")

static void plugin_callback (GtkWidget *widget, gpointer data);

static GtkWidget * 
create_integrated_ui_toolbar (void);

static void add_plugin_actions (GtkUIManager *ui_manager, const char *base_path);

gchar *build_ui_filename (const gchar* name);

/* Active/inactive state is set in diagram_update_menu_sensitivity()
 * in diagram.c */

/* Actions common to toolbox and diagram window */
static const GtkActionEntry common_entries[] =
{
  { "File", NULL, N_("_File"), NULL, NULL, NULL },
    { "FileNew", GTK_STOCK_NEW, NULL, "<control>N", N_("Create a new diagram"), G_CALLBACK (file_new_callback) },
    { "FileOpen", GTK_STOCK_OPEN, NULL,"<control>O", N_("Open a diagram file"), G_CALLBACK (file_open_callback) },
    { "FileQuit", GTK_STOCK_QUIT, NULL, "<control>Q", NULL, G_CALLBACK (file_quit_callback) }, 
  { "Help", NULL, N_("_Help"), NULL, NULL, NULL },
    { "HelpContents", GTK_STOCK_HELP, NULL, "F1", NULL, G_CALLBACK (help_manual_callback) },
    { "HelpAbout", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (help_about_callback) }
};

extern void diagram_tree_show(void);

/* Actions for toolbox menu */
static const GtkActionEntry toolbox_entries[] = 
{
    { "FileSheets", NULL, N_("Sheets and Objects…"), "F9", NULL, G_CALLBACK (sheets_dialog_show_callback) },
    { "FilePrefs", GTK_STOCK_PREFERENCES, NULL, NULL, NULL, G_CALLBACK (file_preferences_callback) },
    { "FilePlugins", NULL, N_("Plugins…"), NULL, NULL, G_CALLBACK (file_plugins_callback) },
    { "FileTree", NULL, N_("_Diagram Tree…"), "F8", NULL, G_CALLBACK (diagram_tree_show) }
};

/* Toggle-Actions for toolbox menu */
static const GtkToggleActionEntry integrated_ui_view_toggle_entries[] = 
{
    { VIEW_MAIN_TOOLBAR_ACTION,   NULL, N_("Show Toolbar"),   NULL, NULL, G_CALLBACK (view_main_toolbar_callback) },
    { VIEW_MAIN_STATUSBAR_ACTION, NULL, N_("Show Statusbar"), NULL, NULL, G_CALLBACK (view_main_statusbar_callback) },
    { VIEW_LAYERS_ACTION,    NULL, N_("Show Layers"), "<control>L", NULL, G_CALLBACK (view_layers_callback) }
};

/* Actions for diagram window */
static const GtkActionEntry display_entries[] =
{
    { "FileSave", GTK_STOCK_SAVE, NULL, "<control>S", N_("Save the diagram"), G_CALLBACK (file_save_callback) },
    { "FileSaveas", GTK_STOCK_SAVE_AS, NULL, "<control><shift>S", N_("Save the diagram with a new name"), G_CALLBACK (file_save_as_callback) },
    { "FileExport", GTK_STOCK_CONVERT, N_("_Export…"), NULL, N_("Export the diagram"), G_CALLBACK (file_export_callback) },
    { "DiagramProperties", GTK_STOCK_PROPERTIES, N_("_Diagram Properties"), "<shift><alt>Return", NULL, G_CALLBACK (view_diagram_properties_callback) },
    { "FilePagesetup", NULL, N_("Page Set_up…"), NULL, NULL, G_CALLBACK (file_pagesetup_callback) },
    { "FilePrint", GTK_STOCK_PRINT, NULL, "<control>P", N_("Print the diagram"), G_CALLBACK (file_print_callback) },
    { "FileClose", GTK_STOCK_CLOSE, NULL, "<control>W", NULL, G_CALLBACK (file_close_callback) },

  { "Edit", NULL, N_("_Edit"), NULL, NULL, NULL },
    { "EditUndo", GTK_STOCK_UNDO, NULL, "<control>Z", NULL, G_CALLBACK (edit_undo_callback) },
    { "EditRedo", GTK_STOCK_REDO, NULL, "<control><shift>Z", NULL, G_CALLBACK (edit_redo_callback) },

    { "EditCopy", GTK_STOCK_COPY, NULL, "<control>C", N_("Copy selection"), G_CALLBACK (edit_copy_callback) },
    { "EditCut", GTK_STOCK_CUT, NULL, "<control>X", N_("Cut selection"), G_CALLBACK (edit_cut_callback) },
    { "EditPaste", GTK_STOCK_PASTE, NULL, "<control>V", N_("Paste selection"), G_CALLBACK (edit_paste_callback) },
    { "EditDuplicate", NULL, N_("_Duplicate"), "<control>D", NULL, G_CALLBACK (edit_duplicate_callback) },
    { "EditDelete", GTK_STOCK_DELETE, NULL, "Delete", NULL, G_CALLBACK (edit_delete_callback) },

    { "EditFind", GTK_STOCK_FIND, NULL, "<control>F", NULL, G_CALLBACK (edit_find_callback) },
    { "EditReplace", GTK_STOCK_FIND_AND_REPLACE, NULL, "<control>H", NULL, G_CALLBACK (edit_replace_callback) },

    /* the following used to bind to <control><shift>C which collides with Unicode input. 
     * <control>>alt> doesn't work either */
    { "EditCopytext", NULL, N_("Copy Text"), NULL, NULL, G_CALLBACK (edit_copy_text_callback) },
    { "EditCuttext", NULL, N_("Cut Text"), "<control><shift>X", NULL, G_CALLBACK (edit_cut_text_callback) },
    { "EditPastetext", NULL, N_("Paste _Text"), "<control><shift>V", NULL, G_CALLBACK (edit_paste_text_callback) },

  { "Layers", NULL, N_("_Layers"), NULL, NULL, NULL }, 
    { "LayerAdd", DIA_STOCK_LAYER_ADD, N_("Add Layer…"), NULL, NULL, G_CALLBACK (layers_add_layer_callback) },
    { "LayerRename", DIA_STOCK_LAYER_RENAME, N_("Rename Layer…"), NULL, NULL, G_CALLBACK (layers_rename_layer_callback) },
    { "ObjectsLayerAbove", DIA_STOCK_OBJECTS_LAYER_ABOVE, N_("Move selection to layer above"), NULL, NULL, G_CALLBACK (objects_move_up_layer) },
    { "ObjectsLayerBelow", DIA_STOCK_OBJECTS_LAYER_BELOW, N_("Move selection to layer below"), NULL, NULL, G_CALLBACK (objects_move_down_layer) },
    { "DiagramLayers", DIA_STOCK_LAYERS, N_("_Layers…"), "<control>L", NULL, G_CALLBACK (dialogs_layers_callback) },

  { "View", NULL, N_("_View"), NULL, NULL, NULL },
    { "ViewZoomin", GTK_STOCK_ZOOM_IN, NULL, "<control>plus", N_("Zoom in"), G_CALLBACK (view_zoom_in_callback) },
    { "ViewZoomout", GTK_STOCK_ZOOM_OUT, NULL, "<control>minus", N_("Zoom out"), G_CALLBACK (view_zoom_out_callback) },
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
    /* Show All, Best Fit.  Same as the Gimp, Ctrl+E */
    { "ViewShowall", GTK_STOCK_ZOOM_FIT, NULL, "<control>E", N_("Zoom fit"), G_CALLBACK (view_show_all_callback) },

  /* "display_toggle_entries" items go here */

    { "ViewNewview", NULL, N_("New _View"), NULL, NULL, G_CALLBACK (view_new_view_callback) },
    { "ViewCloneview", NULL, N_("C_lone View"), NULL, NULL, G_CALLBACK (view_clone_view_callback) },
    { "ViewRedraw", GTK_STOCK_REFRESH, NULL, NULL, NULL, G_CALLBACK (view_redraw_callback) },

  { "Objects", NULL, N_("_Objects"), NULL, NULL },
    { "ObjectsSendtoback", GTK_STOCK_GOTO_BOTTOM, N_("Send to _Back"), "<control><shift>B", NULL, G_CALLBACK (objects_place_under_callback) },
    { "ObjectsBringtofront", GTK_STOCK_GOTO_TOP, N_("Bring to _Front"), "<control><shift>F", NULL, G_CALLBACK (objects_place_over_callback) },
    { "ObjectsSendbackwards", GTK_STOCK_GO_DOWN, N_("Send Backwards"), NULL, NULL, G_CALLBACK (objects_place_down_callback) },
    { "ObjectsBringforwards", GTK_STOCK_GO_UP, N_("Bring Forwards"), NULL, NULL, G_CALLBACK (objects_place_up_callback) },

    { "ObjectsGroup", DIA_STOCK_GROUP, N_("_Group"), "<control>G", NULL, G_CALLBACK (objects_group_callback) },
    /* deliberately not using Ctrl+U for Ungroup */
    { "ObjectsUngroup", DIA_STOCK_UNGROUP, N_("_Ungroup"), "<control><shift>G", NULL, G_CALLBACK (objects_ungroup_callback) }, 

    { "ObjectsParent", NULL, N_("_Parent"), "<control>K", NULL, G_CALLBACK (objects_parent_callback) },
    { "ObjectsUnparent", NULL, N_("_Unparent"), "<control><shift>K", NULL, G_CALLBACK (objects_unparent_callback) },
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
    { "SelectNone", NULL, N_("None"), "<control><shift>A", NULL, G_CALLBACK (select_none_callback) },
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
    { "ToolsModify", NULL, N_("Modify"), "N", NULL, NULL },
    { "ToolsMagnify", NULL, N_("Magnify"), "M", NULL, NULL },
    { "ToolsTextedit", NULL, N_("Edit text"), "F2", NULL, NULL },
    { "ToolsScroll", NULL, N_("Scroll"), "S", NULL, NULL },
    { "ToolsText", NULL, N_("Text"), "T", NULL, NULL },
    { "ToolsBox", NULL, N_("Box"), "R", NULL, NULL },
    { "ToolsEllipse", NULL, N_("Ellipse"), "E", NULL, NULL },
    { "ToolsPolygon", NULL, N_("Polygon"), "P", NULL, NULL },
    { "ToolsBeziergon", NULL, N_("Beziergon"), "B", NULL, NULL },

    { "ToolsLine", NULL, N_("Line"), "L", NULL, NULL },
    { "ToolsArc", NULL, N_("Arc"), "A", NULL, NULL },
    { "ToolsZigzagline", NULL, N_("Zigzagline"), "Z", NULL, NULL },
    { "ToolsPolyline", NULL, N_("Polyline"), "Y", NULL },
    { "ToolsBezierline", NULL, N_("Bezierline"), "C", NULL, NULL },
    { "ToolsOutline", NULL, N_("Outline"), "O", NULL, NULL },

    { "ToolsImage", NULL, N_("Image"), "I", NULL, NULL },
};

/* Toggle-Actions for diagram window */
static const GtkToggleActionEntry display_toggle_entries[] = 
{
#ifdef GTK_STOCK_FULLSCREEN /* added with gtk+-2.8, but no reason to depend on it */
    { "ViewFullscreen", GTK_STOCK_FULLSCREEN, NULL, "F11", NULL, G_CALLBACK (view_fullscreen_callback) },
#else
    { "ViewFullscreen", NULL, N_("Fullscr_een"), "F11", NULL, G_CALLBACK (view_fullscreen_callback) },
#endif
    { "ViewAntialiased", NULL, N_("_AntiAliased"), NULL, NULL, G_CALLBACK (view_aa_callback) },
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

/* integrated ui */
static GtkUIManager *integrated_ui_manager = NULL;
static GtkActionGroup *integrated_ui_actions = NULL;
static GtkActionGroup *integrated_ui_tool_actions = NULL;
static GtkAccelGroup *integrated_ui_accels = NULL;
static GtkWidget *integrated_ui_menubar = NULL;
static GtkWidget *integrated_ui_toolbar = NULL;

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

static gchar*
_dia_translate (const gchar* term, gpointer data)
{
  gchar* trans = (gchar*) term;
  
  if (term && *term) {
    /* first try our own ... */
    trans = dgettext (GETTEXT_PACKAGE, term);
    /* ... than gtk */
    if (term == trans)
      trans = dgettext ("gtk20", term);
#if 0
    /* FIXME: final fallback */
    if (term == trans) { /* FIXME: translation to be updated */
      gchar* kludge = g_strdup_printf ("/%s", term);
      trans = dgettext (GETTEXT_PACKAGE, kludge);
      if (kludge == trans)
	trans = term;
      else
	++trans;
      g_free (kludge);
    }
    if (term == trans)
      trans = g_strdup_printf ("XXX: %s", term);
#endif
  }
  return trans;
}

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

/**
 * Synchronized the Object snap property button with the display.
 * @param param Display to synchronize to.
 */
void
integrated_ui_toolbar_object_snap_synchronize_to_display(gpointer param)
{
  DDisplay *ddisp = param;
  if (ddisp && ddisp->common_toolbar)
  {
    GtkToggleButton *b = g_object_get_data (G_OBJECT (ddisp->common_toolbar), 
                                            DIA_INTEGRATED_TOOLBAR_OBJECT_SNAP);
    gboolean active = ddisp->mainpoint_magnetism? TRUE : FALSE;
    gtk_toggle_button_set_active (b, active);
  }
}

/**
 * Sets the Object-snap property for the active display.
 * @param b Object snap toggle button.
 * @param not_used
 */
static void
integrated_ui_toolbar_object_snap_toggle(GtkToggleButton *b, gpointer *not_used)
{
  DDisplay *ddisp = ddisplay_active ();
  if (ddisp)
  {
    ddisplay_set_snap_to_objects (ddisp, gtk_toggle_button_get_active (b));
  }
}

/**
 * Synchronizes the Snap-to-grid property button with the display.
 * @param param Display to synchronize to.
 */
void
integrated_ui_toolbar_grid_snap_synchronize_to_display(gpointer param)
{
  DDisplay *ddisp = param;
  if (ddisp && ddisp->common_toolbar)
  {
    GtkToggleButton *b = g_object_get_data (G_OBJECT (ddisp->common_toolbar), 
                                            DIA_INTEGRATED_TOOLBAR_SNAP_GRID);
    gboolean active = ddisp->grid.snap? TRUE : FALSE;
    gtk_toggle_button_set_active (b, active);
  }
}

/**
 * Sets the Snap-to-grid property for the active display.
 * @param b Snap to grid toggle button.
 * @param not_used
 */
static void
integrated_ui_toolbar_grid_snap_toggle(GtkToggleButton *b, gpointer *not_used)
{
  DDisplay *ddisp = ddisplay_active ();
  if (ddisp)
  {
    ddisplay_set_snap_to_grid (ddisp, gtk_toggle_button_get_active (b));
  }
}

/**
 * Sets the zoom text for the toolbar
 * @param toolbar Integrated UI toolbar.
 * @param text Current zoom percentage for the active window 
 */
void integrated_ui_toolbar_set_zoom_text (GtkToolbar *toolbar, const gchar * text)
{
  if (toolbar)
  {
    GtkComboBoxEntry *combo_entry = g_object_get_data (G_OBJECT (toolbar), 
                                                       DIA_INTEGRATED_TOOLBAR_ZOOM_COMBO);
    
    if (combo_entry)
    {
        GtkWidget * entry = gtk_bin_get_child (GTK_BIN (combo_entry));

        gtk_entry_set_text (GTK_ENTRY (entry), text);
    }
  }
}

/** 
 * Adds a widget to the toolbar making sure that it doesn't take any excess space, and
 * vertically centers it.
 * @param toolbar The toolbar to add the widget to.
 * @param w       The widget to add to the toolbar.
 */
static void integrated_ui_toolbar_add_custom_item (GtkToolbar *toolbar, GtkWidget *w)
{
    GtkToolItem *tool_item; 
    GtkWidget   *c; /* container */

    tool_item = gtk_tool_item_new ();
    c = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (tool_item), c);
    gtk_box_set_homogeneous (GTK_BOX (c), TRUE);            /* Centers the button */
    gtk_box_pack_start (GTK_BOX (c), w, FALSE, FALSE, 0);
    gtk_toolbar_insert (toolbar, tool_item, -1);
    gtk_widget_show (GTK_WIDGET (tool_item));
    gtk_widget_show (c);
    gtk_widget_show (w);
}

static void
integrated_ui_toolbar_zoom_activate (GtkWidget *item, 
                                     gpointer   user_data)
{
    const gchar *text =  gtk_entry_get_text (GTK_ENTRY (item));
    float        zoom_percent;

    if (sscanf (text, "%f", &zoom_percent) == 1)
    {
        view_zoom_set (10.0 * zoom_percent);
    }
}

static void
integrated_ui_toolbar_zoom_combo_selection_changed (GtkComboBox *combo,
                                                    gpointer     user_data)
{
    /* 
     * We call gtk_combo_get_get_active() so that typing in the combo entry
     * doesn't get handled as a selection change                           
     */
    if (gtk_combo_box_get_active (combo) != -1)
    {
        float zoom_percent;
        gchar * text = gtk_combo_box_get_active_text (combo);

        if (sscanf (text, "%f", &zoom_percent) == 1)
        {
            view_zoom_set (zoom_percent * 10.0);
        }
        else if (g_ascii_strcasecmp (text, ZOOM_FIT) == 0)
        {
            view_show_all_callback (NULL);
        }
        
        g_free (text);
    }
}

static guint
ensure_menu_path (GtkUIManager *ui_manager, GtkActionGroup *actions, const gchar *path, gboolean end)
{
  guint id = gtk_ui_manager_new_merge_id (ui_manager);

  if (!gtk_ui_manager_get_widget (ui_manager, path)) {
    gchar *subpath = g_strdup (path);

    if (strrchr (subpath, '/')) {
      const gchar *action_name;
      gchar *sep;

      GtkAction *action;

      sep = strrchr (subpath, '/');
      *sep = '\0'; /* cut subpath */
      action_name = sep + 1;
      
      ensure_menu_path (ui_manager, actions, subpath, FALSE);

      action = gtk_action_new (action_name, sep + 1, NULL, NULL);
      if (!gtk_action_group_get_action (actions, action_name))
        gtk_action_group_add_action (actions, action);
      g_object_unref (G_OBJECT (action));

      gtk_ui_manager_add_ui (ui_manager, id, subpath, 
	                     action_name, action_name,
			     end ? GTK_UI_MANAGER_SEPARATOR : GTK_UI_MANAGER_MENU,
			     FALSE); /* FALSE=add-to-end */
    } else {
      g_warning ("ensure_menu_path() invalid menu path: %s.", subpath ? subpath : "NULL");
    }
    g_free (subpath);
  }
  return id;
}

/**
 * Create the toolbar for the integrated UI
 * @return Main toolbar (GtkToolbar*) for the integrated UI main window
 */
static GtkWidget * 
create_integrated_ui_toolbar (void)
{
  GtkToolbar  *toolbar;
  GtkToolItem *sep;
  GtkWidget   *w;
  GError      *error = NULL;
  gchar *uifile;

  uifile = build_ui_filename ("ui/toolbar-ui.xml");
  if (!gtk_ui_manager_add_ui_from_file (integrated_ui_manager, uifile, &error)) {
    g_warning ("building menus failed: %s", error->message);
    g_error_free (error);
    error = NULL;
    toolbar = GTK_TOOLBAR (gtk_toolbar_new ());
  }
  else {
    toolbar =  GTK_TOOLBAR(gtk_ui_manager_get_widget (integrated_ui_manager, "/Toolbar"));
  }
  g_free (uifile);  

  /* Zoom Combo Box Entry */
  w = gtk_combo_box_entry_new_text ();

  g_object_set_data (G_OBJECT (toolbar), 
                     DIA_INTEGRATED_TOOLBAR_ZOOM_COMBO,
                     w);
  integrated_ui_toolbar_add_custom_item (toolbar, w);
 
  gtk_combo_box_append_text (GTK_COMBO_BOX (w), ZOOM_FIT);
  gtk_combo_box_append_text (GTK_COMBO_BOX (w), _("800%"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (w), _("400%"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (w), _("300%"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (w), _("200%"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (w), _("150%"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (w), _("100%"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (w), _("75%"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (w), _("50%"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (w), _("25%"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (w), _("10%"));

  g_signal_connect (G_OBJECT (w), 
                    "changed",
		            G_CALLBACK (integrated_ui_toolbar_zoom_combo_selection_changed), 
                    NULL);
  
  /* Get the combo's GtkEntry child to set the width for the widget */
  w = gtk_bin_get_child (GTK_BIN (w));
  gtk_entry_set_width_chars (GTK_ENTRY (w), 6);
  
  g_signal_connect (GTK_OBJECT (w), "activate",
		            G_CALLBACK(integrated_ui_toolbar_zoom_activate),
		            NULL);
  
  /* Seperator */
  sep = gtk_separator_tool_item_new ();
  gtk_toolbar_insert (toolbar, sep, -1);
  gtk_widget_show (GTK_WIDGET (sep));

  /* Snap to grid */
  w = dia_toggle_button_new_with_icons (dia_on_grid_icon,
                                        dia_off_grid_icon);
  g_signal_connect (G_OBJECT (w), "toggled",
		   G_CALLBACK (integrated_ui_toolbar_grid_snap_toggle), toolbar);
  gtk_widget_set_tooltip_text (w, _("Toggles snap-to-grid."));
  g_object_set_data (G_OBJECT (toolbar), 
                     DIA_INTEGRATED_TOOLBAR_SNAP_GRID,
                     w);
  integrated_ui_toolbar_add_custom_item (toolbar, w);
 
  /* Object Snapping */
  w = dia_toggle_button_new_with_icons (dia_mainpoints_on_icon,
                                        dia_mainpoints_off_icon);
  g_signal_connect (G_OBJECT (w), "toggled",
		   G_CALLBACK (integrated_ui_toolbar_object_snap_toggle), toolbar);
  gtk_widget_set_tooltip_text (w, _("Toggles object snapping."));
  g_object_set_data (G_OBJECT (toolbar), 
                     DIA_INTEGRATED_TOOLBAR_OBJECT_SNAP,
                     w);
  integrated_ui_toolbar_add_custom_item (toolbar, w);

  sep = gtk_separator_tool_item_new ();
  gtk_toolbar_insert (toolbar, sep, -1);
  gtk_widget_show (GTK_WIDGET (sep));

  return GTK_WIDGET (toolbar);
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
  gtk_action_group_set_translate_func (actions, _dia_translate, NULL, NULL);
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
add_plugin_actions (GtkUIManager *ui_manager, const gchar *base_path)
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
  gtk_action_group_set_translate_func (actions, _dia_translate, NULL, NULL);
  g_free (name);
  name = NULL;
  cnt++;

  gtk_ui_manager_insert_action_group (ui_manager, actions, 5 /* "back" */);
  g_object_unref (actions);

  for (cblist = filter_get_callbacks(); cblist; cblist = cblist->next) {
    gchar *menu_path = NULL;

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

    if (   base_path != NULL 
        && strncmp (cbf->menupath, base_path, strlen (base_path)) != 0) {
      /* hook for wrong base path, skip */
      continue;
    } else if (!base_path) {
      /* only replace what we know */
      if (strncmp (cbf->menupath, DISPLAY_MENU, strlen (DISPLAY_MENU)) == 0)
        menu_path = g_strdup_printf ("%s%s", INTEGRATED_MENU, cbf->menupath + strlen (DISPLAY_MENU));
      else if (strncmp (cbf->menupath, TOOLBOX_MENU, strlen (TOOLBOX_MENU)) == 0)
        menu_path = g_strdup_printf ("%s%s", INTEGRATED_MENU, cbf->menupath + strlen (TOOLBOX_MENU));
    }

    action = gtk_action_new (cbf->action, cbf->description, NULL, NULL);
    g_signal_connect (G_OBJECT (action), "activate", 
		      G_CALLBACK (plugin_callback), (gpointer) cbf);

    gtk_action_group_add_action (actions, action);
    g_object_unref (G_OBJECT (action));

    id = ensure_menu_path (ui_manager, actions, menu_path ? menu_path : cbf->menupath, TRUE);
    gtk_ui_manager_add_ui (ui_manager, id, 
			   menu_path ? menu_path : cbf->menupath, 
			   cbf->description, 
			   cbf->action, 
			   GTK_UI_MANAGER_AUTO, 
			   FALSE);
    g_free (menu_path);
  }
}

static void
_add_stock_icon (GtkIconFactory *factory, const char *name, const guint8 *data, const size_t size)
{
  GdkPixbuf      *pixbuf;
  GtkIconSet     *set;
  GError         *err = NULL;

  pixbuf = gdk_pixbuf_new_from_inline (size, data, FALSE, &err);
  if (err) {
    g_warning ("%s", err->message);
    g_error_free (err);
    err = NULL;
  }
  set = gtk_icon_set_new_from_pixbuf (pixbuf);
  gtk_icon_factory_add (factory, name, set);
  g_object_unref (pixbuf);
  pixbuf = NULL;
}

static void
register_stock_icons (void)
{
  GtkIconFactory *factory;

  factory = gtk_icon_factory_new ();

  _add_stock_icon (factory, DIA_STOCK_GROUP, dia_group_icon, sizeof(dia_group_icon));
  _add_stock_icon (factory, DIA_STOCK_UNGROUP, dia_ungroup_icon, sizeof(dia_ungroup_icon));

  _add_stock_icon (factory, DIA_STOCK_LAYER_ADD, dia_layer_add, sizeof(dia_layer_add));
  _add_stock_icon (factory, DIA_STOCK_LAYER_RENAME, dia_layer_rename, sizeof(dia_layer_rename));
  _add_stock_icon (factory, DIA_STOCK_OBJECTS_LAYER_ABOVE, dia_objects_layer_above, sizeof(dia_objects_layer_above));
  _add_stock_icon (factory, DIA_STOCK_OBJECTS_LAYER_BELOW, dia_objects_layer_below, sizeof(dia_objects_layer_below));
  _add_stock_icon (factory, DIA_STOCK_LAYERS, dia_layers, sizeof(dia_layers));

  gtk_icon_factory_add_default (factory);
  g_object_unref (factory);
  factory = NULL;
}

gchar*
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

/*!
 * Not sure why this service is not provided by GTK+. 
 * We are passing tooltips into the actions (especially recent file menu).
 * But they were not shown without explicit setting on connect.
 */
static void
_ui_manager_connect_proxy (GtkUIManager *manager,
                           GtkAction    *action,
                           GtkWidget    *proxy)
{
  if (GTK_IS_MENU_ITEM (proxy))
    {
      gchar *tooltip;

      g_object_get (action, "tooltip", &tooltip, NULL);

      if (tooltip)
        {
	  gtk_widget_set_tooltip_text (proxy, tooltip);
	  g_free (tooltip);
	}
      else
	{
	  const gchar *name = gtk_action_get_name (action);

	  g_print ("Action '%s' missing tooltip\n", name);
	}
    }
}

static GtkActionGroup *
create_or_ref_display_actions (void)
{
  if (display_actions)
    return g_object_ref (display_actions);
  display_actions = gtk_action_group_new ("display-actions");
  gtk_action_group_set_translation_domain (display_actions, NULL);
  gtk_action_group_set_translate_func (display_actions, _dia_translate, NULL, NULL);
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
                0, /* SELECT_REPLACE - first radio entry */
                G_CALLBACK (select_style_callback),
                NULL);
  /* the initial reference */
  return display_actions;
}

/* Very minimal fallback menu info for ui-files missing 
 * as well as to register the InvisibleMenu */
static const gchar *ui_info =
"<ui>\n"
"  <popup name=\"InvisibleMenu\">\n"
"    <menu name=\"File\" action=\"File\">\n"
"       <menuitem name=\"FilePrint\" action=\"FilePrint\" />\n"
"       <menuitem name=\"FileQuit\" action=\"FileQuit\" />\n"
"    </menu>\n"
"  </popup>\n"
"</ui>";

static void
menus_init(void)
{
  gchar                 *accelfilename;
  GError 		*error = NULL;
  gchar			*uifile;

  if (!initialise)
    return;
  
  initialise = FALSE;

  register_stock_icons ();

  /* the toolbox menu */
  toolbox_actions = gtk_action_group_new ("toolbox-actions");
  gtk_action_group_set_translation_domain (toolbox_actions, NULL);
  gtk_action_group_set_translate_func (toolbox_actions, _dia_translate, NULL, NULL);
  gtk_action_group_add_actions (toolbox_actions, common_entries, 
                G_N_ELEMENTS (common_entries), NULL);
  gtk_action_group_add_actions (toolbox_actions, toolbox_entries, 
                G_N_ELEMENTS (toolbox_entries), NULL);

  toolbox_ui_manager = gtk_ui_manager_new ();
  g_signal_connect (G_OBJECT (toolbox_ui_manager), 
                    "connect_proxy",
		    G_CALLBACK (_ui_manager_connect_proxy),
		    NULL);
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
  toolbox_menubar = gtk_ui_manager_get_widget (toolbox_ui_manager, TOOLBOX_MENU);

  /* the display menu */
  display_actions = create_or_ref_display_actions ();
  display_tool_actions = create_tool_actions ();

  display_ui_manager = gtk_ui_manager_new ();
  g_signal_connect (G_OBJECT (display_ui_manager), 
                    "connect_proxy",
		    G_CALLBACK (_ui_manager_connect_proxy),
		    NULL);
  gtk_ui_manager_set_add_tearoffs (display_ui_manager, DIA_SHOW_TEAROFFS);
  gtk_ui_manager_insert_action_group (display_ui_manager, display_actions, 0);
  gtk_ui_manager_insert_action_group (display_ui_manager, display_tool_actions, 0);
  if (!gtk_ui_manager_add_ui_from_string (display_ui_manager, ui_info, -1, &error)) {
    g_warning ("built-in menus failed: %s", error->message);
    g_error_free (error);
    error = NULL;
  }

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
  display_menubar = gtk_ui_manager_get_widget (display_ui_manager, DISPLAY_MENU);
  g_assert (display_menubar);

  add_plugin_actions (toolbox_ui_manager, TOOLBOX_MENU);
  add_plugin_actions (display_ui_manager, DISPLAY_MENU);
  add_plugin_actions (display_ui_manager, INVISIBLE_MENU);

  /* load accelerators and prepare to later save them */
  accelfilename = dia_config_filename("menurc");
  
  if (accelfilename) {
    gtk_accel_map_load(accelfilename);
    g_free(accelfilename);
  }
  gtk_quit_add(1, save_accels, NULL);
}

void
menus_get_integrated_ui_menubar (GtkWidget     **menubar,
                                 GtkWidget     **toolbar,
			         GtkAccelGroup **accel_group)
{
  GError *error = NULL;
  gchar  *uifile;

  if (initialise)
    menus_init();

  /* the integrated ui menu */
  integrated_ui_actions = gtk_action_group_new ("integrated-ui-actions");
  gtk_action_group_set_translation_domain (integrated_ui_actions, NULL);
  gtk_action_group_set_translate_func (integrated_ui_actions, _dia_translate, NULL, NULL);
  gtk_action_group_add_actions (integrated_ui_actions, common_entries, 
                G_N_ELEMENTS (common_entries), NULL);
  gtk_action_group_add_actions (integrated_ui_actions, toolbox_entries, 
                G_N_ELEMENTS (toolbox_entries), NULL);
  gtk_action_group_add_actions (integrated_ui_actions, display_entries, 
                G_N_ELEMENTS (display_entries), NULL);
  gtk_action_group_add_toggle_actions (integrated_ui_actions, integrated_ui_view_toggle_entries, 
                G_N_ELEMENTS (integrated_ui_view_toggle_entries), NULL);
  gtk_action_group_add_toggle_actions (integrated_ui_actions, display_toggle_entries,
                G_N_ELEMENTS (display_toggle_entries), 
                NULL);
  gtk_action_group_add_radio_actions (integrated_ui_actions,
                display_select_radio_entries,
                G_N_ELEMENTS (display_select_radio_entries),
                1,
                G_CALLBACK (select_style_callback),
                NULL);

  integrated_ui_tool_actions = create_tool_actions ();

  integrated_ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_set_add_tearoffs (integrated_ui_manager, DIA_SHOW_TEAROFFS);
  gtk_ui_manager_insert_action_group (integrated_ui_manager, integrated_ui_actions, 0);
  gtk_ui_manager_insert_action_group (integrated_ui_manager, integrated_ui_tool_actions, 0);

  uifile = build_ui_filename ("ui/integrated-ui.xml");
  if (!gtk_ui_manager_add_ui_from_file (integrated_ui_manager, uifile, &error)) {
    g_warning ("building integrated ui menus failed: %s", error->message);
    g_error_free (error);
    error = NULL;
  }
  g_free (uifile);
  if (!gtk_ui_manager_add_ui_from_string (integrated_ui_manager, ui_info, -1, &error)) {
    g_warning ("built-in menus failed: %s", error->message);
    g_error_free (error);
    error = NULL;
  }

  add_plugin_actions (integrated_ui_manager, NULL);

  integrated_ui_accels = gtk_ui_manager_get_accel_group (integrated_ui_manager);
  integrated_ui_menubar = gtk_ui_manager_get_widget (integrated_ui_manager, INTEGRATED_MENU);
  integrated_ui_toolbar = create_integrated_ui_toolbar ();

  if (menubar)
    *menubar = integrated_ui_menubar;
  if (toolbar)
    *toolbar = integrated_ui_toolbar;
  if (accel_group)
    *accel_group = integrated_ui_accels;
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

  if (is_integrated_ui ()) 
    return integrated_ui_accels;

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

  
  *actions = create_or_ref_display_actions ();
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

  add_plugin_actions (*ui_manager, DISPLAY_MENU);
  menu_bar = gtk_ui_manager_get_widget (*ui_manager, DISPLAY_MENU);
  return menu_bar;
}

GtkAccelGroup *
menus_get_accel_group ()
{
  return is_integrated_ui ()? integrated_ui_accels : toolbox_accels;
}

GtkActionGroup *
menus_get_action_group ()
{
  return is_integrated_ui ()? integrated_ui_actions : toolbox_actions;
}

GtkAction *
menus_get_action (const gchar *name)
{
  GtkAction *action;

  if (is_integrated_ui ())
  {
    action = gtk_action_group_get_action (integrated_ui_actions, name);
    if (action == NULL) {
      action = gtk_action_group_get_action (integrated_ui_tool_actions, name);
    }
  }
  else
  {
    action = gtk_action_group_get_action (toolbox_actions, name);
    if (action == NULL) {
      action = gtk_action_group_get_action (display_actions, name);
    }
    if (action == NULL) {
      action = gtk_action_group_get_action (display_tool_actions, name);
    }
  }
  if (!action) {
    GList *groups, *list;

    /* search everthing there is, could probably replace the above */
    groups = gtk_ui_manager_get_action_groups (display_ui_manager);
    for (list = groups; list != NULL; list = list->next) {
      action = gtk_action_group_get_action (GTK_ACTION_GROUP (list->data), name);
      if (action)
	break;
    }
  }

  return action;
}

void
menus_set_recent (GtkActionGroup *actions)
{
  GList *list;
  guint id;
  GtkUIManager *ui_manager;
  const char   *recent_path;

  if (is_integrated_ui ())
  {
    ui_manager  = integrated_ui_manager;
    recent_path = INTEGRATED_MENU "/File/FileRecentEnd";
  }
  else
  {
    ui_manager  = toolbox_ui_manager;
    recent_path = TOOLBOX_MENU "/File/FileRecentEnd";
  }

  if (recent_actions) {
    menus_clear_recent ();
  }

  list = gtk_action_group_list_actions (actions);
  g_return_if_fail (list);

  recent_actions = actions;
  g_object_ref (G_OBJECT (recent_actions));
  gtk_ui_manager_insert_action_group (ui_manager, 
                    recent_actions, 
                    10 /* insert at back */ );

  do {
    id = gtk_ui_manager_new_merge_id (ui_manager);
    recent_merge_ids = g_slist_prepend (recent_merge_ids, (gpointer) id);

    gtk_ui_manager_add_ui (ui_manager, id, 
                 recent_path, 
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
  GtkUIManager *ui_manager;

  if (is_integrated_ui ())
    ui_manager  = integrated_ui_manager;
  else
    ui_manager  = toolbox_ui_manager;

  if (recent_merge_ids) {
    id = recent_merge_ids;
    do {
      gtk_ui_manager_remove_ui (ui_manager, (guint) id->data);
      
    } while (NULL != (id = id->next));
    
    g_slist_free (recent_merge_ids);
    recent_merge_ids = NULL;
  }

  if (recent_actions) {
    gtk_ui_manager_remove_action_group (ui_manager, recent_actions);
    g_object_unref (G_OBJECT (recent_actions));
    recent_actions = NULL;
  }
}

static void
plugin_callback (GtkWidget *widget, gpointer data)
{
  DiaCallbackFilter *cbf = data;

  /* and finally invoke it */
  if (cbf->callback) {
    DDisplay *ddisp = NULL;
    DiagramData* diadata = NULL;
    /* stuff from the toolbox menu should never get a diagram to modify */
    if (strncmp (cbf->menupath, TOOLBOX_MENU, strlen (TOOLBOX_MENU)) != 0) {
      ddisp = ddisplay_active();
      diadata = ddisp ? ddisp->diagram->data : NULL;
    }
    cbf->callback (diadata, ddisp ? ddisp->diagram->filename : NULL, 0, cbf->user_data);
  }
}
