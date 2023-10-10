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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

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
#include "widgets.h"
#include "preferences.h"
#include "filter.h"
#include "toolbox.h"
#include "diagram_tree.h"
#include "dia-builder.h"


#define DIA_STOCK_GROUP "dia-group"
#define DIA_STOCK_UNGROUP "dia-ungroup"
#define DIA_STOCK_LAYER_ADD "dia-layer-add"
#define DIA_STOCK_LAYER_RENAME "dia-layer-rename"
#define DIA_STOCK_OBJECTS_LAYER_ABOVE "dia-objects-layer-above"
#define DIA_STOCK_OBJECTS_LAYER_BELOW "dia-objects-layer-below"
#define DIA_STOCK_LAYERS "dia-layers"

/* Integrated UI Toolbar Constants */
#define DIA_INTEGRATED_TOOLBAR_ZOOM_COMBO  "dia-integrated-toolbar-zoom-combo_entry"
#define DIA_INTEGRATED_TOOLBAR_SNAP_GRID   "dia-integrated-toolbar-snap-grid"
#define DIA_INTEGRATED_TOOLBAR_OBJECT_SNAP "dia-integrated-toolbar-object-snap"
#define DIA_INTEGRATED_TOOLBAR_GUIDES_SNAP "dia-integrated-toolbar-guides-snap"

static void plugin_callback (GtkWidget *widget, gpointer data);

static GtkWidget *create_integrated_ui_toolbar (void);

static void add_plugin_actions (GtkUIManager *ui_manager, const char *base_path);

/* Active/inactive state is set in diagram_update_menu_sensitivity()
 * in diagram.c */

#ifdef HAVE_MAC_INTEGRATION
# define FIRST_MODIFIER "<Primary>"
/* On OSX/Quartz the unmodified tool accelerators are in conflict with the global menu */
# define TOOL_MODIFIER "<Control>"
#else
# define FIRST_MODIFIER "<Control>"
/**/
# define TOOL_MODIFIER ""
#endif

/* Actions common to toolbox and diagram window */
static const GtkActionEntry common_entries[] =
{
  { "File", NULL, N_("_File"), NULL, NULL, NULL },
    { "FileNew", GTK_STOCK_NEW, N_("_New"), FIRST_MODIFIER "N", N_("Create a new diagram"), G_CALLBACK (file_new_callback) },
    { "FileOpen", GTK_STOCK_OPEN, N_("_Open…"), FIRST_MODIFIER "O", N_("Open a diagram file"), G_CALLBACK (file_open_callback) },
    { "FileQuit", GTK_STOCK_QUIT, N_("_Quit"), FIRST_MODIFIER "Q", N_("Quit Dia"), G_CALLBACK (file_quit_callback) },
  { "Help", NULL, N_("_Help"), NULL, NULL, NULL },
    { "HelpContents", GTK_STOCK_HELP, N_("_Help"), "F1", N_("Dia help"), G_CALLBACK (help_manual_callback) },
    { "HelpAbout", GTK_STOCK_ABOUT, N_("_About"), NULL, N_("Dia version, authors, license"), G_CALLBACK (help_about_callback) }
};

/* Actions for toolbox menu */
static const GtkActionEntry toolbox_entries[] =
{
    { "FileSheets", NULL, N_("S_heets and Objects"), "F9", N_("Manage sheets and their objects"), G_CALLBACK (sheets_dialog_show_callback) },
    { "FilePrefs", GTK_STOCK_PREFERENCES, N_("P_references"), NULL, N_("Dia preferences"), G_CALLBACK (file_preferences_callback) },
    { "FilePlugins", NULL, N_("P_lugins…"), NULL, N_("Manage plug-ins"), G_CALLBACK (file_plugins_callback) },
    { "FileTree", NULL, N_("Diagram _Tree"), "F8", N_("Tree representation of diagrams"), G_CALLBACK (diagram_tree_show) }
};

static const GtkToggleActionEntry integrated_ui_view_toggle_entries[] =
{
    { VIEW_MAIN_TOOLBAR_ACTION, NULL, N_("Show _Toolbar"), NULL, N_("Show or hide the toolbar"), G_CALLBACK (view_main_toolbar_callback) },
    { VIEW_MAIN_STATUSBAR_ACTION, NULL, N_("Show _Statusbar"), NULL, N_("Show or hide the statusbar"), G_CALLBACK (view_main_statusbar_callback) },
    { VIEW_LAYERS_ACTION, NULL, N_("Show _Layers"), FIRST_MODIFIER "L", N_("Show or hide the layers toolwindow"), G_CALLBACK (view_layers_callback) }
};

/* Actions for diagram window */
static const GtkActionEntry display_entries[] =
{
    { "FileSave", GTK_STOCK_SAVE, N_("_Save"), FIRST_MODIFIER "S", N_("Save the diagram"), G_CALLBACK (file_save_callback) },
    { "FileSaveas", GTK_STOCK_SAVE_AS, N_("Save _As…"), FIRST_MODIFIER "<shift>S", N_("Save the diagram with a new name"), G_CALLBACK (file_save_as_callback) },
    { "FileExport", GTK_STOCK_CONVERT, N_("_Export…"), NULL, N_("Export the diagram"), G_CALLBACK (file_export_callback) },
    { "DiagramProperties", GTK_STOCK_PROPERTIES, N_("_Diagram Properties"), "<shift><alt>Return", N_("Modify diagram properties (grid, background)"), G_CALLBACK (view_diagram_properties_callback) },
    { "FilePagesetup", NULL, N_("Page Set_up…"), NULL, N_("Modify the diagram pagination"), G_CALLBACK (file_pagesetup_callback) },
    { "FilePrint", GTK_STOCK_PRINT, N_("_Print…"), FIRST_MODIFIER "P", N_("Print the diagram"), G_CALLBACK (file_print_callback) },
    { "FileClose", GTK_STOCK_CLOSE, N_("_Close"), FIRST_MODIFIER "W", N_("Close the diagram"), G_CALLBACK (file_close_callback) },

  { "Edit", NULL, N_("_Edit"), NULL, NULL, NULL },
    { "EditUndo", GTK_STOCK_UNDO, N_("_Undo"), FIRST_MODIFIER "Z", N_("Undo"), G_CALLBACK (edit_undo_callback) },
    { "EditRedo", GTK_STOCK_REDO, N_("_Redo"), FIRST_MODIFIER "<shift>Z", N_("Redo"), G_CALLBACK (edit_redo_callback) },

    { "EditCopy", GTK_STOCK_COPY, N_("_Copy"), FIRST_MODIFIER "C", N_("Copy selection"), G_CALLBACK (edit_copy_callback) },
    { "EditCut", GTK_STOCK_CUT, N_("Cu_t"), FIRST_MODIFIER "X", N_("Cut selection"), G_CALLBACK (edit_cut_callback) },
    { "EditPaste", GTK_STOCK_PASTE, N_("_Paste"), FIRST_MODIFIER "V", N_("Paste selection"), G_CALLBACK (edit_paste_callback) },
    { "EditDuplicate", NULL, N_("_Duplicate"), FIRST_MODIFIER "D", N_("Duplicate selection"), G_CALLBACK (edit_duplicate_callback) },
    { "EditDelete", GTK_STOCK_DELETE, N_("D_elete"), "Delete", N_("Delete selection"), G_CALLBACK (edit_delete_callback) },

    { "EditFind", GTK_STOCK_FIND, N_("_Find…"), FIRST_MODIFIER "F", N_("Search for text"), G_CALLBACK (edit_find_callback) },
    { "EditReplace", GTK_STOCK_FIND_AND_REPLACE, N_("_Replace…"), FIRST_MODIFIER "H", N_("Search and replace text"), G_CALLBACK (edit_replace_callback) },

    /* the following used to bind to <control><shift>C which collides with Unicode input.
     * <control><alt> doesn't work either */
    { "EditCopytext", NULL, N_("C_opy Text"), NULL, N_("Copy object's text to clipboard"), G_CALLBACK (edit_copy_text_callback) },
    { "EditCuttext", NULL, N_("C_ut Text"), FIRST_MODIFIER "<shift>X", N_("Cut object's text to clipboard"), G_CALLBACK (edit_cut_text_callback) },
    { "EditPastetext", NULL, N_("P_aste Text"), FIRST_MODIFIER "<shift>V", N_("Insert text from clipboard"), G_CALLBACK (edit_paste_text_callback) } ,

    { "EditPasteImage", NULL, N_("Paste _Image"), FIRST_MODIFIER "<alt>V", N_("Insert image from clipboard"), G_CALLBACK (edit_paste_image_callback) },

  { "Layers", NULL, N_("_Layers"), NULL, NULL, NULL },
    { "LayerAdd", DIA_STOCK_LAYER_ADD, N_("_Add Layer…"), NULL, NULL, G_CALLBACK (layers_add_layer_callback) },
    { "LayerRename", DIA_STOCK_LAYER_RENAME, N_("_Rename Layer…"), NULL, NULL, G_CALLBACK (layers_rename_layer_callback) },
    { "ObjectsLayerAbove", DIA_STOCK_OBJECTS_LAYER_ABOVE, N_("_Move Selection to Layer above"), NULL, NULL, G_CALLBACK (objects_move_up_layer) },
    { "ObjectsLayerBelow", DIA_STOCK_OBJECTS_LAYER_BELOW, N_("Move _Selection to Layer below"), NULL, NULL, G_CALLBACK (objects_move_down_layer) },
    { "DiagramLayers", DIA_STOCK_LAYERS, N_("_Layers…"), FIRST_MODIFIER "L", NULL, G_CALLBACK (dialogs_layers_callback) },

  { "View", NULL, N_("_View"), NULL, NULL, NULL },
    { "ViewZoomin", GTK_STOCK_ZOOM_IN, N_("Zoom _In"), FIRST_MODIFIER "plus", N_("Zoom in"), G_CALLBACK (view_zoom_in_callback) },
    { "ViewZoomout", GTK_STOCK_ZOOM_OUT, N_("Zoom _Out"), FIRST_MODIFIER "minus", N_("Zoom out"), G_CALLBACK (view_zoom_out_callback) },
    { "ViewZoom", NULL, N_("_Zoom"), NULL, NULL, NULL },
      { "ViewZoom16000", NULL, N_("1600%"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom8000", NULL, N_("800%"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom4000", NULL, N_("400%"), "<alt>4", NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom2830", NULL, N_("283"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom2000", NULL, N_("200"), "<alt>2", NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom1410", NULL, N_("141"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom1000", GTK_STOCK_ZOOM_100, N_("_Normal Size"), "<alt>1", NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom850", NULL, N_("85"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom707", NULL, N_("70.7"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom500", NULL, N_("50"), "<alt>5", NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom354", NULL, N_("35.4"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },
      { "ViewZoom250", NULL, N_("25"), NULL, NULL, G_CALLBACK (view_zoom_set_callback) },
    /* Show All, Best Fit.  Same as the Gimp, Ctrl+E */
    { "ViewShowall", GTK_STOCK_ZOOM_FIT, N_("Best _Fit"), FIRST_MODIFIER "E", N_("Zoom fit"), G_CALLBACK (view_show_all_callback) },

  /* "display_toggle_entries" items go here */

    { "ViewNewview", NULL, N_("New _View"), NULL, NULL, G_CALLBACK (view_new_view_callback) },
    { "ViewCloneview", NULL, N_("C_lone View"), NULL, NULL, G_CALLBACK (view_clone_view_callback) },
    { "ViewRedraw", GTK_STOCK_REFRESH, N_("_Refresh"), NULL, NULL, G_CALLBACK (view_redraw_callback) },

    { "ViewGuides", NULL, N_("_Guides"), NULL, NULL, NULL },
      { "ViewNewguide", NULL, N_("_New Guide..."), NULL, NULL, G_CALLBACK (view_new_guide_callback) },

  { "Objects", NULL, N_("_Objects"), NULL, NULL },
    { "ObjectsSendtoback", GTK_STOCK_GOTO_BOTTOM, N_("Send to _Back"), FIRST_MODIFIER "<shift>B", N_("Move selection to the bottom"), G_CALLBACK (objects_place_under_callback) },
    { "ObjectsBringtofront", GTK_STOCK_GOTO_TOP, N_("Bring to _Front"), FIRST_MODIFIER "<shift>F", N_("Move selection to the top"), G_CALLBACK (objects_place_over_callback) },
    { "ObjectsSendbackwards", GTK_STOCK_GO_DOWN, N_("Send Ba_ckwards"), NULL, NULL, G_CALLBACK (objects_place_down_callback) },
    { "ObjectsBringforwards", GTK_STOCK_GO_UP, N_("Bring F_orwards"), NULL, NULL, G_CALLBACK (objects_place_up_callback) },

    { "ObjectsGroup", DIA_STOCK_GROUP, N_("_Group"), FIRST_MODIFIER "G", N_("Group selected objects"), G_CALLBACK (objects_group_callback) },
    /* deliberately not using Ctrl+U for Ungroup */
    { "ObjectsUngroup", DIA_STOCK_UNGROUP, N_("_Ungroup"), FIRST_MODIFIER "<shift>G", N_("Ungroup selected groups"), G_CALLBACK (objects_ungroup_callback) },

    { "ObjectsParent", NULL, N_("_Parent"), FIRST_MODIFIER "K", NULL, G_CALLBACK (objects_parent_callback) },
    { "ObjectsUnparent", NULL, N_("_Unparent"), FIRST_MODIFIER "<shift>K", NULL, G_CALLBACK (objects_unparent_callback) },
    { "ObjectsUnparentchildren", NULL, N_("_Unparent Children"), NULL, NULL, G_CALLBACK (objects_unparent_children_callback) },

    { "ObjectsAlign", NULL, N_("_Align"), NULL, NULL, NULL },
      { "ObjectsAlignLeft", GTK_STOCK_JUSTIFY_LEFT, N_("_Left"), "<alt><shift>L", NULL, G_CALLBACK (objects_align_h_callback) },
      { "ObjectsAlignCenter", GTK_STOCK_JUSTIFY_CENTER, N_("_Center"), "<alt><shift>C", NULL, G_CALLBACK (objects_align_h_callback) },
      { "ObjectsAlignRight", GTK_STOCK_JUSTIFY_RIGHT, N_("_Right"), "<alt><shift>R", NULL, G_CALLBACK (objects_align_h_callback) },

      { "ObjectsAlignTop", NULL, N_("_Top"), "<alt><shift>T", NULL, G_CALLBACK (objects_align_v_callback) },
      { "ObjectsAlignMiddle", NULL, N_("_Middle"), "<alt><shift>M", NULL, G_CALLBACK (objects_align_v_callback) },
      { "ObjectsAlignBottom", NULL, N_("_Bottom"), "<alt><shift>B", NULL, G_CALLBACK (objects_align_v_callback) },

      { "ObjectsAlignSpreadouthorizontally", NULL, N_("Spread Out _Horizontally"), "<alt><shift>H", NULL, G_CALLBACK (objects_align_h_callback) },
      { "ObjectsAlignSpreadoutvertically", NULL, N_("Spread Out _Vertically"), "<alt><shift>V", NULL, G_CALLBACK (objects_align_v_callback) },
      { "ObjectsAlignAdjacent", NULL, N_("_Adjacent"), "<alt><shift>A", NULL, G_CALLBACK (objects_align_h_callback) },
      { "ObjectsAlignStacked", NULL, N_("_Stacked"), "<alt><shift>S", NULL, G_CALLBACK (objects_align_v_callback) },
      { "ObjectsAlignConnected", NULL, N_("_Connected"), "<alt><shift>O", NULL, G_CALLBACK (objects_align_connected_callback) },

      { "ObjectsProperties", GTK_STOCK_PROPERTIES, N_("_Properties"), "<alt>Return", NULL, G_CALLBACK (dialogs_properties_callback) },

  { "Select", NULL, N_("_Select"), NULL, NULL, NULL },
    { "SelectAll", NULL, N_("_All"), FIRST_MODIFIER "A", NULL, G_CALLBACK (select_all_callback) },
    { "SelectNone", NULL, N_("_None"), FIRST_MODIFIER "<shift>A", NULL, G_CALLBACK (select_none_callback) },
    { "SelectInvert", NULL, N_("_Invert"), FIRST_MODIFIER "I", NULL, G_CALLBACK (select_invert_callback) },

    { "SelectTransitive", NULL, N_("_Transitive"), FIRST_MODIFIER "T", NULL, G_CALLBACK (select_transitive_callback) },
    { "SelectConnected", NULL, N_("_Connected"), FIRST_MODIFIER "<shift>T", NULL, G_CALLBACK (select_connected_callback) },
    { "SelectSametype", NULL, N_("Same _Type"), NULL, NULL, G_CALLBACK (select_same_type_callback) },

    /* display_select_radio_entries go here */

    { "SelectBy", NULL, N_("_Select By"), NULL, NULL, NULL },

  /* For placement of the toplevel Layout menu and it's accelerator */
  { "Layout", NULL, N_("L_ayout"), NULL, NULL, NULL },

  { "Dialogs", NULL, N_("D_ialogs"), NULL, NULL, NULL },

  { "Debug", NULL, N_("D_ebug"), NULL, NULL, NULL }
};

/* Standard-Tool entries */
static const GtkActionEntry tool_entries[] =
{
  { "Tools", NULL, N_("_Tools"), NULL, NULL, NULL },
    { "ToolsModify", NULL, N_("_Modify"), TOOL_MODIFIER "N", NULL, NULL },
    { "ToolsMagnify", NULL, N_("_Magnify"), TOOL_MODIFIER "M", NULL, NULL },
    { "ToolsTextedit", NULL, N_("_Edit Text"), "F2", NULL, NULL },
    { "ToolsScroll", NULL, N_("_Scroll"), TOOL_MODIFIER "S", NULL, NULL },
    { "ToolsText", NULL, N_("_Text"), TOOL_MODIFIER "T", NULL, NULL },
    { "ToolsBox", NULL, N_("_Box"), TOOL_MODIFIER "R", NULL, NULL },
    { "ToolsEllipse", NULL, N_("_Ellipse"), TOOL_MODIFIER "E", NULL, NULL },
    { "ToolsPolygon", NULL, N_("_Polygon"), TOOL_MODIFIER "P", NULL, NULL },
    { "ToolsBeziergon", NULL, N_("_Beziergon"), TOOL_MODIFIER "B", NULL, NULL },

    { "ToolsLine", NULL, N_("_Line"), TOOL_MODIFIER "L", NULL, NULL },
    { "ToolsArc", NULL, N_("_Arc"), TOOL_MODIFIER "A", NULL, NULL },
    { "ToolsZigzagline", NULL, N_("_Zigzagline"), TOOL_MODIFIER "Z", NULL, NULL },
    { "ToolsPolyline", NULL, N_("_Polyline"), TOOL_MODIFIER "Y", NULL },
    { "ToolsBezierline", NULL, N_("_Bezierline"), TOOL_MODIFIER "C", NULL, NULL },
    { "ToolsOutline", NULL, N_("_Outline"), TOOL_MODIFIER "O", NULL, NULL },

    { "ToolsImage", NULL, N_("_Image"), TOOL_MODIFIER "I", NULL, NULL },
};

/* Toggle-Actions for diagram window */
static const GtkToggleActionEntry display_toggle_entries[] =
{
    { "ViewFullscreen", GTK_STOCK_FULLSCREEN, N_("_Fullscreen"), "F11", NULL, G_CALLBACK (view_fullscreen_callback) },
    { "ViewAntialiased", NULL, N_("_Antialiased"), NULL, NULL, G_CALLBACK (view_aa_callback) },
    { "ViewShowgrid", NULL, N_("Show _Grid"), NULL, NULL, G_CALLBACK (view_visible_grid_callback) },
    { "ViewSnaptogrid", NULL, N_("_Snap to Grid"), NULL, NULL, G_CALLBACK (view_snap_to_grid_callback) },
    { "ViewShowguides", NULL, N_("_Show Guides"), NULL, NULL, G_CALLBACK (view_visible_guides_callback) },
    { "ViewSnaptoguides", NULL, N_("Snap to _Guides"), NULL, NULL, G_CALLBACK (view_snap_to_guides_callback) },
    { "ViewRemoveallguides", NULL, N_("_Remove all Guides"), NULL, NULL, G_CALLBACK (view_remove_all_guides_callback) },
    { "ViewSnaptoobjects", NULL, N_("Snap to _Objects"), NULL, NULL, G_CALLBACK (view_snap_to_objects_callback) },
    { "ViewShowrulers", NULL, N_("Show _Rulers"), NULL, NULL, G_CALLBACK (view_toggle_rulers_callback)  },
    { "ViewShowscrollbars", NULL, N_("Show Scroll_bars"), NULL, N_("Show or hide the toolbar"), G_CALLBACK (view_toggle_scrollbars_callback) },
    { "ViewShowconnectionpoints", NULL, N_("Show _Connection Points"), NULL, NULL, G_CALLBACK (view_show_cx_pts_callback) }
};

/* Radio-Actions for the diagram window's "Select"-Menu */
static const GtkRadioActionEntry display_select_radio_entries[] =
{
  { "SelectReplace", NULL, N_("_Replace"), NULL, NULL, SELECT_REPLACE },
  { "SelectUnion", NULL, N_("_Union"), NULL, NULL, SELECT_UNION },
  { "SelectIntersection", NULL, N_("I_ntersection"), NULL, NULL, SELECT_INTERSECTION },
  { "SelectRemove", NULL, N_("R_emove"), NULL, NULL, SELECT_REMOVE },
  /* Cannot also be called Invert, duplicate names caused keybinding problems */
  { "SelectInverse", NULL, N_("In_verse"), NULL, NULL, SELECT_INVERT }
};

#define ZOOM_FIT -1.0

enum {
  COL_DISPLAY,
  COL_AMOUNT,
  N_COL,
};

/* need initialisation? */
static gboolean initialise = TRUE;

/* shared between toolbox and integrated code

 - One ui_manager from toolbox_ui_manager and integrated_ui_manager
 - Two action groups for toolbox and display. In the integrated ui case
   the latter should be disabled (or hidden) if there is no diagram

 */
static GtkUIManager *_ui_manager = NULL;

/* toolbox */
static GtkActionGroup *toolbox_actions = NULL;

static GtkActionGroup *tool_actions = NULL;
GtkActionGroup *recent_actions = NULL;
static GSList *recent_merge_ids = NULL;

/* diagram */
static GtkUIManager *display_ui_manager = NULL;
static GtkActionGroup *display_actions = NULL;
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
    if (term == trans) {
      trans = dgettext ("gtk20", term);
    }
  }
  return trans;
}

static void
tool_menu_select (GtkWidget *w,
                  gpointer   data) {
  ToolButtonData *tooldata = (ToolButtonData *) data;

  if (tooldata == NULL) {
    g_warning (_("NULL tooldata in tool_menu_select"));
    return;
  }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tooldata->widget), TRUE);
}

static void
load_accels (void)
{
  gchar *accelfilename;
  /* load accelerators and prepare to later save them */
  accelfilename = dia_config_filename ("menurc");

  if (accelfilename) {
    gtk_accel_map_load (accelfilename);
    g_clear_pointer (&accelfilename, g_free);
  }
}


/**
 * integrated_ui_toolbar_object_snap_synchronize_to_display:
 * @param: #DDisplay to synchronize to.
 *
 * Synchronized the Object snap property button with the display.
 */
void
integrated_ui_toolbar_object_snap_synchronize_to_display (gpointer param)
{
  DDisplay *ddisp = param;
  if (ddisp && ddisp->common_toolbar) {
    GtkToggleButton *b = g_object_get_data (G_OBJECT (ddisp->common_toolbar),
                                            DIA_INTEGRATED_TOOLBAR_OBJECT_SNAP);
    gboolean active = ddisp->mainpoint_magnetism? TRUE : FALSE;
    gtk_toggle_button_set_active (b, active);
  }
}


/**
 * integrated_ui_toolbar_guides_snap_synchronize_to_display:
 * @param: #DDisplay to synchronize to.
 *
 * Synchronized the snap-to-guide property button with the display.
 *
 * Since: dawn-of-time
 */
void
integrated_ui_toolbar_guides_snap_synchronize_to_display (gpointer param)
{
  DDisplay *ddisp = param;
  if (ddisp && ddisp->common_toolbar) {
    GtkToggleButton *b = g_object_get_data (G_OBJECT (ddisp->common_toolbar),
                                            DIA_INTEGRATED_TOOLBAR_GUIDES_SNAP);
    gboolean active = ddisp->guides_snap? TRUE : FALSE;
    gtk_toggle_button_set_active (b, active);
  }
}


/**
 * integrated_ui_toolbar_object_snap_toggle:
 * @b: Object snap toggle button.
 * @not_used: unused
 *
 * Sets the Object-snap property for the active display.
 *
 * Since: dawn-of-time
 */
static void
integrated_ui_toolbar_object_snap_toggle (GtkToggleButton *b,
                                          gpointer *not_used)
{
  DDisplay *ddisp = ddisplay_active ();
  if (ddisp) {
    ddisplay_set_snap_to_objects (ddisp, gtk_toggle_button_get_active (b));
  }
}


/**
 * integrated_ui_toolbar_guide_snap_toggle:
 * @b: Guide snap toggle button.
 * @not_used: unused
 *
 * Sets the Guide-snap property for the active display.
 *
 * Since: dawn-of-time
 */
static void
integrated_ui_toolbar_guide_snap_toggle (GtkToggleButton *b, gpointer *not_used)
{
  DDisplay *ddisp = ddisplay_active ();
  if (ddisp) {
    ddisplay_set_snap_to_guides (ddisp, gtk_toggle_button_get_active (b));
  }
}


/**
 * integrated_ui_toolbar_grid_snap_synchronize_to_display:
 * @param: #DDisplay to synchronize to.
 *
 * Synchronizes the Snap-to-grid property button with the display.
 *
 * Since: dawn-of-time
 */
void
integrated_ui_toolbar_grid_snap_synchronize_to_display (gpointer param)
{
  DDisplay *ddisp = param;
  if (ddisp && ddisp->common_toolbar) {
    GtkToggleButton *b = g_object_get_data (G_OBJECT (ddisp->common_toolbar),
                                            DIA_INTEGRATED_TOOLBAR_SNAP_GRID);
    gboolean active = ddisp->grid.snap? TRUE : FALSE;
    gtk_toggle_button_set_active (b, active);
  }
}


/**
 * integrated_ui_toolbar_grid_snap_toggle:
 * @b: Snap to grid toggle button.
 * @not_used: unused
 *
 * Sets the Snap-to-grid property for the active display.
 *
 * Since: dawn-of-time
 */
static void
integrated_ui_toolbar_grid_snap_toggle (GtkToggleButton *b, gpointer *not_used)
{
  DDisplay *ddisp = ddisplay_active ();
  if (ddisp) {
    ddisplay_set_snap_to_grid (ddisp, gtk_toggle_button_get_active (b));
  }
}


/**
 * integrated_ui_toolbar_set_zoom_text:
 * @toolbar: Integrated UI toolbar.
 * @text: Current zoom percentage for the active window
 *
 * Sets the zoom text for the toolbar
 *
 * Since: dawn-of-time
 */
void
integrated_ui_toolbar_set_zoom_text (GtkToolbar *toolbar, const char * text)
{
  if (toolbar) {
    GtkComboBox *combo_entry = g_object_get_data (G_OBJECT (toolbar),
                                                  DIA_INTEGRATED_TOOLBAR_ZOOM_COMBO);

    if (combo_entry) {
      GtkWidget * entry = gtk_bin_get_child (GTK_BIN (combo_entry));

      gtk_entry_set_text (GTK_ENTRY (entry), text);
    }
  }
}


/**
 * integrated_ui_toolbar_add_custom_item:
 * @toolbar: The #GtkToolbar to add the widget to.
 * @w: The widget to add to the @toolbar.
 *
 * Adds a widget to the toolbar making sure that it doesn't take any excess space, and
 * vertically centers it.
 *
 * Since: dawn-of-time
 */
static void
integrated_ui_toolbar_add_custom_item (GtkToolbar *toolbar, GtkWidget *w)
{
  GtkToolItem *tool_item;
  GtkWidget   *c; /* container */

  tool_item = gtk_tool_item_new ();
  c = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
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
  const gchar *text = gtk_entry_get_text (GTK_ENTRY (item));
  double       zoom_percent = parse_zoom (text);

  if (zoom_percent > 0) {
    view_zoom_set (zoom_percent);
  }
}


/* "DiaZoomCombo" probably could work for both UI cases */
static void
integrated_ui_toolbar_zoom_combo_selection_changed (GtkComboBox *combo,
                                                    gpointer     user_data)
{
  GtkTreeIter iter;
  GtkTreeModel *store = gtk_combo_box_get_model (combo);

  /*
    * We call gtk_combo_get_get_active() so that typing in the combo entry
    * doesn't get handled as a selection change
    */
  if (gtk_combo_box_get_active_iter (combo, &iter)) {
    double zoom_percent;

    gtk_tree_model_get (store, &iter,
                        COL_AMOUNT, &zoom_percent,
                        -1);

    if (zoom_percent < 0) {
      view_show_all_callback (NULL);
    } else {
      view_zoom_set (zoom_percent);
    }
  }
}


static guint
ensure_menu_path (GtkUIManager   *ui_manager,
                  GtkActionGroup *actions,
                  const gchar    *path,
                  gboolean        end)
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
      if (!gtk_action_group_get_action (actions, action_name)) {
        gtk_action_group_add_action (actions, action);
      }
      g_clear_object (&action);

      gtk_ui_manager_add_ui (ui_manager,
                             id,
                             subpath,
                             action_name,
                             action_name,
                             end ? GTK_UI_MANAGER_SEPARATOR : GTK_UI_MANAGER_MENU,
                             FALSE); /* FALSE=add-to-end */
    } else {
      g_warning ("ensure_menu_path() invalid menu path: %s.", subpath ? subpath : "NULL");
    }
    g_clear_pointer (&subpath, g_free);
  }
  return id;
}


/**
 * create_integrated_ui_toolbar:
 *
 * Create the toolbar for the integrated UI
 *
 * Returns: Main toolbar #GtkToolbar for the integrated UI main window
 *
 * Since: dawn-of-time
 */
static GtkWidget *
create_integrated_ui_toolbar (void)
{
  GtkToolbar   *toolbar;
  GtkToolItem  *sep;
  GtkListStore *store;
  GtkTreeIter   iter;
  GtkWidget    *w;
  GError       *error = NULL;
  gchar        *uifile;

  uifile = dia_builder_path ("ui/toolbar-ui.xml");
  if (!gtk_ui_manager_add_ui_from_file (_ui_manager, uifile, &error)) {
    g_critical ("building menus failed: %s", error->message);
    g_clear_error (&error);
    toolbar = GTK_TOOLBAR (gtk_toolbar_new ());
  } else {
    toolbar = GTK_TOOLBAR (gtk_ui_manager_get_widget (_ui_manager, "/Toolbar"));
  }
  g_clear_pointer (&uifile, g_free);

  store = gtk_list_store_new (N_COL, G_TYPE_STRING, G_TYPE_DOUBLE);

  /* Zoom Combo Box Entry */
  w = gtk_combo_box_new_with_model_and_entry (GTK_TREE_MODEL (store));
  gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (w), COL_DISPLAY);

  g_object_set_data (G_OBJECT (toolbar),
                     DIA_INTEGRATED_TOOLBAR_ZOOM_COMBO,
                     w);
  integrated_ui_toolbar_add_custom_item (toolbar, w);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      COL_DISPLAY, _("Fit"),
                      COL_AMOUNT, ZOOM_FIT,
                      -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      COL_DISPLAY, _("800%"),
                      COL_AMOUNT, 8000.0,
                      -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      COL_DISPLAY, _("400%"),
                      COL_AMOUNT, 4000.0,
                      -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      COL_DISPLAY, _("300%"),
                      COL_AMOUNT, 3000.0,
                      -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      COL_DISPLAY, _("200%"),
                      COL_AMOUNT, 2000.0,
                      -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      COL_DISPLAY, _("150%"),
                      COL_AMOUNT, 1500.0,
                      -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      COL_DISPLAY, _("100%"),
                      COL_AMOUNT, 1000.0,
                      -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      COL_DISPLAY, _("75%"),
                      COL_AMOUNT, 750.0,
                      -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      COL_DISPLAY, _("50%"),
                      COL_AMOUNT, 500.0,
                      -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      COL_DISPLAY, _("25%"),
                      COL_AMOUNT, 250.0,
                      -1);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      COL_DISPLAY, _("10%"),
                      COL_AMOUNT, 100.0,
                      -1);

  g_signal_connect (G_OBJECT (w),
                    "changed",
                    G_CALLBACK (integrated_ui_toolbar_zoom_combo_selection_changed),
                    NULL);

  /* Get the combo's GtkEntry child to set the width for the widget */
  w = gtk_bin_get_child (GTK_BIN (w));
  gtk_entry_set_width_chars (GTK_ENTRY (w), 6);

  g_signal_connect (G_OBJECT (w), "activate",
                    G_CALLBACK (integrated_ui_toolbar_zoom_activate), NULL);

  /* Seperator */
  sep = gtk_separator_tool_item_new ();
  gtk_toolbar_insert (toolbar, sep, -1);
  gtk_widget_show (GTK_WIDGET (sep));

  /* Snap to grid */
  w = dia_toggle_button_new_with_icon_names ("dia-grid-on",
                                             "dia-grid-off");

  g_signal_connect (G_OBJECT (w), "toggled",
                    G_CALLBACK (integrated_ui_toolbar_grid_snap_toggle),
                    toolbar);
  gtk_widget_set_tooltip_text (w, _("Toggles snap-to-grid."));
  g_object_set_data (G_OBJECT (toolbar),
                     DIA_INTEGRATED_TOOLBAR_SNAP_GRID,
                     w);
  integrated_ui_toolbar_add_custom_item (toolbar, w);

  /* Object Snapping */
  w = dia_toggle_button_new_with_icon_names ("dia-mainpoints-on",
                                             "dia-mainpoints-off");
  g_signal_connect (G_OBJECT (w), "toggled",
                    G_CALLBACK (integrated_ui_toolbar_object_snap_toggle),
                    toolbar);
  gtk_widget_set_tooltip_text (w, _("Toggles object snapping."));
  g_object_set_data (G_OBJECT (toolbar),
                     DIA_INTEGRATED_TOOLBAR_OBJECT_SNAP,
                     w);
  integrated_ui_toolbar_add_custom_item (toolbar, w);

  /* Guide Snapping */
  w = dia_toggle_button_new_with_icon_names ("dia-guides-snap-on",
                                             "dia-guides-snap-off");
  g_signal_connect (G_OBJECT (w),
                    "toggled",
                    G_CALLBACK (integrated_ui_toolbar_guide_snap_toggle),
                    toolbar);
  gtk_widget_set_tooltip_text (w, _("Toggles guide snapping."));
  g_object_set_data (G_OBJECT (toolbar),
                     DIA_INTEGRATED_TOOLBAR_GUIDES_SNAP,
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
create_or_ref_tool_actions (void)
{
  GtkIconFactory *icon_factory;
  GtkActionGroup *actions;
  GtkAction      *action;
  int           i;

  if (tool_actions) {
    return g_object_ref (tool_actions);
  }

  actions = gtk_action_group_new ("tool-actions");
  gtk_action_group_set_translation_domain (actions, NULL);
  gtk_action_group_set_translate_func (actions, _dia_translate, NULL, NULL);

  gtk_action_group_add_actions (actions,
                                tool_entries,
                                G_N_ELEMENTS (tool_entries),
                                NULL);

  icon_factory = gtk_icon_factory_new ();

  for (i = 0; i < num_tools; i++) {
    action = gtk_action_group_get_action (actions, tool_data[i].action_name);
    if (action != NULL) {
      g_signal_connect (G_OBJECT (action), "activate",
                        G_CALLBACK (tool_menu_select),
                        &tool_data[i].callback_data);

      gtk_action_set_tooltip (action, _(tool_data[i].tool_desc));

      {
        GdkPixbuf *pb = tool_get_pixbuf (&tool_data[i]);
        GtkIconSet *is = gtk_icon_set_new_from_pixbuf (pb);

        /* not sure if the action name is unique enough */
        gtk_icon_factory_add (icon_factory, tool_data[i].action_name, is);
        gtk_action_set_stock_id (action, tool_data[i].action_name);

        g_clear_object (&pb);
      }
    } else {
      g_warning ("couldn't find tool menu item %s", tool_data[i].action_name);
    }
  }
  gtk_icon_factory_add_default (icon_factory);
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

  g_clear_pointer (&name, g_free);
  cnt++;

  gtk_ui_manager_insert_action_group (ui_manager, actions, 5 /* "back" */);

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

    action = gtk_action_new (cbf->action, gettext (cbf->description), NULL, NULL);
    g_signal_connect (G_OBJECT (action),
                      "activate",
                      G_CALLBACK (plugin_callback),
                      (gpointer) cbf);

    gtk_action_group_add_action (actions, action);
    g_clear_object (&action);

    id = ensure_menu_path (ui_manager, actions, menu_path ? menu_path : cbf->menupath, TRUE);
    gtk_ui_manager_add_ui (ui_manager, id,
                           menu_path ? menu_path : cbf->menupath,
                           cbf->description,
                           cbf->action,
                           GTK_UI_MANAGER_AUTO,
                           FALSE);
    g_clear_pointer (&menu_path, g_free);
  }

  g_clear_object (&actions);
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
  if (GTK_IS_MENU_ITEM (proxy)) {
    gchar *tooltip;

    g_object_get (action, "tooltip", &tooltip, NULL);

    if (tooltip) {
      gtk_widget_set_tooltip_text (proxy, tooltip);
      g_clear_pointer (&tooltip, g_free);
    }
  }
}

static GtkActionGroup *
create_or_ref_display_actions (gboolean include_common)
{
  if (display_actions)
    return g_object_ref (display_actions);
  display_actions = gtk_action_group_new ("display-actions");
  gtk_action_group_set_translation_domain (display_actions, NULL);
  gtk_action_group_set_translate_func (display_actions, _dia_translate, NULL, NULL);
  if (include_common)
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
_action_start (GtkActionGroup *action_group,
	       GtkAction *action,
	       gpointer user_data)
{
  dia_log_message ("Start '%s'", gtk_action_get_name (action));
}
static void
_action_done (GtkActionGroup *action_group,
	       GtkAction *action,
	       gpointer user_data)
{
  dia_log_message ("Done '%s'", gtk_action_get_name (action));
}

static void
_setup_global_actions (void)
{
  if (tool_actions)
    return;
  g_return_if_fail (_ui_manager == NULL);
  g_return_if_fail (toolbox_actions == NULL);

  /* for the toolbox menu */
  toolbox_actions = gtk_action_group_new ("toolbox-actions");
  gtk_action_group_set_translation_domain (toolbox_actions, NULL);
  gtk_action_group_set_translate_func (toolbox_actions, _dia_translate, NULL, NULL);
  gtk_action_group_add_actions (toolbox_actions, common_entries,
                G_N_ELEMENTS (common_entries), NULL);
  gtk_action_group_add_actions (toolbox_actions, toolbox_entries,
                G_N_ELEMENTS (toolbox_entries), NULL);

  _ui_manager = gtk_ui_manager_new ();
  g_signal_connect (G_OBJECT (_ui_manager),
                    "connect_proxy",
		    G_CALLBACK (_ui_manager_connect_proxy),
		    NULL);
  g_signal_connect (G_OBJECT (_ui_manager),
                    "pre-activate",
		    G_CALLBACK (_action_start),
		    NULL);
  g_signal_connect (G_OBJECT (_ui_manager),
                    "post-activate",
		    G_CALLBACK (_action_done),
		    NULL);

  gtk_ui_manager_insert_action_group (_ui_manager, toolbox_actions, 0);

  tool_actions = create_or_ref_tool_actions ();
}

static void
menus_init(void)
{
  GError *error = NULL;
  gchar  *uifile;

  if (!initialise)
    return;

  initialise = FALSE;
  _setup_global_actions ();

  uifile = dia_builder_path ("ui/toolbox-ui.xml");
  if (!gtk_ui_manager_add_ui_from_file (_ui_manager, uifile, &error)) {
    g_warning ("building menus failed: %s", error->message);
    g_clear_error (&error);
  }
  g_clear_pointer (&uifile, g_free);

  /* the display menu */
  display_actions = create_or_ref_display_actions (TRUE);

  display_ui_manager = gtk_ui_manager_new ();
  g_signal_connect (G_OBJECT (display_ui_manager),
                    "connect_proxy",
		    G_CALLBACK (_ui_manager_connect_proxy),
		    NULL);
  gtk_ui_manager_insert_action_group (display_ui_manager, display_actions, 0);
  gtk_ui_manager_insert_action_group (display_ui_manager, tool_actions, 0);
  if (!gtk_ui_manager_add_ui_from_string (display_ui_manager, ui_info, -1, &error)) {
    g_warning ("built-in menus failed: %s", error->message);
    g_clear_error (&error);
  }

  uifile = dia_builder_path ("ui/popup-ui.xml");
  /* TODO it would be more elegant if we had only one definition of the
   * menu hierarchy and merge it into a popup somehow. */
  if (!gtk_ui_manager_add_ui_from_file (display_ui_manager, uifile, &error)) {
    g_warning ("building menus failed: %s", error->message);
    g_clear_error (&error);
  }
  g_clear_pointer (&uifile, g_free);

  display_accels = gtk_ui_manager_get_accel_group (display_ui_manager);
  display_menubar = gtk_ui_manager_get_widget (display_ui_manager, DISPLAY_MENU);
  g_assert (display_menubar);

  add_plugin_actions (_ui_manager, TOOLBOX_MENU);
  add_plugin_actions (display_ui_manager, DISPLAY_MENU);
  add_plugin_actions (display_ui_manager, INVISIBLE_MENU);

  /* after creating all menu items */
  load_accels ();
}

void
menus_get_integrated_ui_menubar (GtkWidget     **menubar,
                                 GtkWidget     **toolbar,
			         GtkAccelGroup **accel_group)
{
  GError *error = NULL;
  gchar  *uifile;

  _setup_global_actions ();
  g_return_if_fail (_ui_manager != NULL);

  /* the integrated ui menu */
  display_actions = create_or_ref_display_actions (FALSE);
  g_return_if_fail (tool_actions != NULL);

  /* maybe better to put this into toolbox_actions? */
  gtk_action_group_add_toggle_actions (display_actions, integrated_ui_view_toggle_entries,
				       G_N_ELEMENTS (integrated_ui_view_toggle_entries), NULL);

  /* for stand-alone they are per display */
  gtk_ui_manager_insert_action_group (_ui_manager, display_actions, 0);
  tool_actions = create_or_ref_tool_actions ();
  gtk_ui_manager_insert_action_group (_ui_manager, tool_actions, 0);

  uifile = dia_builder_path ("ui/integrated-ui.xml");
  if (!gtk_ui_manager_add_ui_from_file (_ui_manager, uifile, &error)) {
    g_warning ("building integrated ui menus failed: %s", error->message);
    g_clear_error (&error);
  }
  g_clear_pointer (&uifile, g_free);

  if (!gtk_ui_manager_add_ui_from_string (_ui_manager, ui_info, -1, &error)) {
    g_warning ("built-in menus failed: %s", error->message);
    g_clear_error (&error);
  }

  add_plugin_actions (_ui_manager, NULL);
  /* after creating all menu items */
  load_accels ();

  if (menubar)
    *menubar = gtk_ui_manager_get_widget (_ui_manager, INTEGRATED_MENU);
  if (toolbar)
    *toolbar = create_integrated_ui_toolbar ();
  if (accel_group)
    *accel_group = gtk_ui_manager_get_accel_group (_ui_manager);
}

void
menus_get_toolbox_menubar (GtkWidget     **menubar,
			   GtkAccelGroup **accel_group)
{
  if (initialise)
    menus_init();

  if (menubar)
    *menubar = gtk_ui_manager_get_widget (_ui_manager, TOOLBOX_MENU);
  if (accel_group)
    *accel_group = gtk_ui_manager_get_accel_group (_ui_manager);
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
  /* for integrated-ui the accels are delivered by menus_get_integrated_ui_menubar() */
  g_return_val_if_fail (is_integrated_ui () == FALSE, NULL);

  if (initialise)
    menus_init();

  return display_accels;
}

GtkWidget *
menus_create_display_menubar (GtkUIManager   **ui_manager,
			      GtkActionGroup **actions)
{
  GtkWidget      *menu_bar;
  GError         *error = NULL;
  gchar          *uifile;


  *actions = create_or_ref_display_actions (TRUE);
  tool_actions = create_or_ref_tool_actions ();

  *ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (*ui_manager, *actions, 0);
  gtk_ui_manager_insert_action_group (*ui_manager, tool_actions, 0);
  g_clear_object (&tool_actions);

  uifile = dia_builder_path ("ui/display-ui.xml");
  if (!gtk_ui_manager_add_ui_from_file (*ui_manager, uifile, &error)) {
    g_warning ("building menus failed: %s", error->message);
    g_clear_error (&error);
    g_clear_pointer (&uifile, g_free);
    return NULL;
  }
  g_clear_pointer (&uifile, g_free);

  add_plugin_actions (*ui_manager, DISPLAY_MENU);
  menu_bar = gtk_ui_manager_get_widget (*ui_manager, DISPLAY_MENU);
  return menu_bar;
}

GtkActionGroup *
menus_get_tool_actions (void)
{
  return tool_actions;
}

GtkActionGroup *
menus_get_display_actions (void)
{
  return display_actions;
}

GtkAction *
menus_get_action (const gchar *name)
{
  GtkAction *action;

  action = gtk_action_group_get_action (toolbox_actions, name);
  if (!action)
    action = gtk_action_group_get_action (display_actions, name);
  if (!action)
    action = gtk_action_group_get_action (tool_actions, name);
  if (!action) {
    GList *groups, *list;

    /* search everything there is, could probably replace the above */
    if (display_ui_manager) /* classic mode */
      groups = gtk_ui_manager_get_action_groups (display_ui_manager);
    else
      groups = gtk_ui_manager_get_action_groups (_ui_manager);
    for (list = groups; list != NULL; list = list->next) {
      action = gtk_action_group_get_action (GTK_ACTION_GROUP (list->data), name);
      if (action)
	break;
    }
  }

  return action;
}

GtkWidget *
menus_get_widget (const gchar *name)
{
  g_return_val_if_fail (_ui_manager != NULL, NULL);

  return gtk_ui_manager_get_widget (_ui_manager, name);
}

static int
cmp_action_names (const void *a, const void *b)
{
  return strcmp (gtk_action_get_name (GTK_ACTION (a)), gtk_action_get_name (GTK_ACTION (b)));
}

void
menus_set_recent (GtkActionGroup *actions)
{
  GList *list;
  guint id;
  const char   *recent_path;

  if (is_integrated_ui ())
    recent_path = INTEGRATED_MENU "/File/FileRecentEnd";
  else
    recent_path = TOOLBOX_MENU "/File/FileRecentEnd";

  if (recent_actions) {
    menus_clear_recent ();
  }

  list = gtk_action_group_list_actions (actions);
  g_return_if_fail (list);

  /* sort it by the action name to preserve out order */
  list = g_list_sort (list, cmp_action_names);

  recent_actions = actions;
  g_object_ref (G_OBJECT (recent_actions));
  gtk_ui_manager_insert_action_group (_ui_manager,
                    recent_actions,
                    10 /* insert at back */ );

  do {
    const gchar* aname = gtk_action_get_name (GTK_ACTION (list->data));

    id = gtk_ui_manager_new_merge_id (_ui_manager);
    recent_merge_ids = g_slist_prepend (recent_merge_ids, GUINT_TO_POINTER (id));

    gtk_ui_manager_add_ui (_ui_manager, id,
                 recent_path,
                 aname,
                 aname,
                 GTK_UI_MANAGER_AUTO,
                 TRUE);

  } while (NULL != (list = list->next));
}

void
menus_clear_recent (void)
{
  GSList *id;

  if (recent_merge_ids) {
    id = recent_merge_ids;
    do {
      gtk_ui_manager_remove_ui (_ui_manager, GPOINTER_TO_UINT (id->data));

    } while (NULL != (id = id->next));

    g_slist_free (recent_merge_ids);
    recent_merge_ids = NULL;
  }

  if (recent_actions) {
    gtk_ui_manager_remove_action_group (_ui_manager, recent_actions);
    g_clear_object (&recent_actions);
  }
}

static void
plugin_callback (GtkWidget *widget, gpointer data)
{
  DiaCallbackFilter *cbf = data;

  /* check if the callback filter is still available */
  if (!g_list_find (filter_get_callbacks (), cbf)) {
    message_error (_("The function is not available anymore."));
    return;
  }
  /* and finally invoke it */
  if (cbf->callback) {
    DDisplay *ddisp = NULL;
    DiagramData* diadata = NULL;
    DiaObjectChange *change;

    /* stuff from the toolbox menu should never get a diagram to modify */
    if (strncmp (cbf->menupath, TOOLBOX_MENU, strlen (TOOLBOX_MENU)) != 0) {
      ddisp = ddisplay_active();
      diadata = ddisp ? ddisp->diagram->data : NULL;
    }

    change = cbf->callback (diadata,
                            ddisp ? ddisp->diagram->filename : NULL,
                            0,
                            cbf->user_data);

    if (change != NULL) {
      if (ddisp) {
        dia_object_change_change_new (ddisp->diagram, NULL, change);
        /*
         * - can not call object_add_update() w/o object
         * - could call object_add_updates_list() with the selected objects,
         *   but that would just be an educated guess (layout working on selection)
         */
        diagram_add_update_all (ddisp->diagram);
        diagram_modified (ddisp->diagram);
        diagram_update_extents (ddisp->diagram);
        undo_set_transactionpoint (ddisp->diagram->undo);
      } else {
        /* no diagram to keep the change, throw it away */
        g_clear_pointer (&change, dia_object_change_unref);
      }
    }
  }
}

