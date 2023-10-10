/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <gtk/gtk.h>

#include "diagram.h"
#include "message.h"
#include "preferences.h"
#include "dia_dirs.h"
#include "diagramdata.h"
#include "paper.h"
#include "interface.h"
#include "lib/prefs.h"
#include "persistence.h"
#include "filter.h"
#include "dia-builder.h"
#include "dia-colour-selector.h"
#include "units.h"


enum {
  COL_NAME,
  COL_UNIT,
  N_COLS,
};


struct DiaPreferences prefs;


struct _DiaPreferencesDialog {
  /*< private >*/
  GtkDialog parent;

  GtkWidget *gl_dynamic;
  GtkWidget *gl_manual;
  GtkWidget *manual_props;
  GtkWidget *gl_hex;
  GtkWidget *gl_hex_size;
};


G_DEFINE_TYPE (DiaPreferencesDialog, dia_preferences_dialog, GTK_TYPE_DIALOG)


static void
dia_preferences_dialog_response (GtkDialog *dialog, int response)
{
  diagram_redraw_all ();

  gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
dia_preferences_dialog_class_init (DiaPreferencesDialogClass *klass)
{
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  dialog_class->response = dia_preferences_dialog_response;
}


static void
ui_reset_tools_toggled (GtkCheckButton *check,
                        gpointer        data)
{
  prefs.reset_tools_after_create = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  persistence_set_boolean ("reset_tools_after_create",
                           prefs.reset_tools_after_create);
}


static void
ui_undo_spin_changed (GtkSpinButton *spin,
                      gpointer       data)
{
  prefs.undo_depth = gtk_spin_button_get_value (spin);
  persistence_set_integer ("undo_depth", prefs.undo_depth);
}


static void
ui_reverse_drag_toggled (GtkCheckButton *check,
                         gpointer        data)
{
  prefs.reverse_rubberbanding_intersects = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  persistence_set_boolean ("reverse_rubberbanding_intersects",
                           prefs.reverse_rubberbanding_intersects);
}


static void
ui_recent_spin_changed (GtkSpinButton *spin,
                        gpointer       data)
{
  prefs.recent_documents_list_size = gtk_spin_button_get_value (spin);
  persistence_set_integer ("recent_documents_list_size",
                           prefs.recent_documents_list_size);
}


static void
ui_length_unit_changed (GtkComboBox *combo,
                        gpointer     data)
{
  GEnumClass *unit_class = g_type_class_ref (DIA_TYPE_UNIT);
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (combo, &iter)) {
    GtkTreeModel *model = gtk_combo_box_get_model (combo);
    DiaUnit unit;

    gtk_tree_model_get (model, &iter, COL_UNIT, &unit, -1);

    prefs_set_length_unit (unit);
    persistence_set_string ("length-unit",
                            g_enum_get_value (unit_class, unit)->value_nick);
  }
}


static void
ui_font_unit_changed (GtkComboBox *combo,
                      gpointer     data)
{
  GEnumClass *unit_class = g_type_class_ref (DIA_TYPE_UNIT);
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (combo, &iter)) {
    GtkTreeModel *model = gtk_combo_box_get_model (combo);
    DiaUnit unit;

    gtk_tree_model_get (model, &iter, COL_UNIT, &unit, -1);

    prefs_set_fontsize_unit (unit);
    persistence_set_string ("font-unit",
                            g_enum_get_value (unit_class, unit)->value_nick);
  }
}


static void
ui_snap_distance_changed (GtkSpinButton *spin,
                          gpointer       data)
{
  prefs.snap_distance = gtk_spin_button_get_value (spin);
  persistence_set_integer ("snap_distance", prefs.snap_distance);
}


static void
update_floating_toolbox (void)
{
  if (!app_is_interactive ()) {
    return;
  }

  if (prefs.toolbox_on_top) {
    /* Go through all diagrams and set toolbox transient for all displays */
    GList *diagrams;
    for (diagrams = dia_open_diagrams ();
         diagrams != NULL;
         diagrams = g_list_next (diagrams)) {
      Diagram *diagram = (Diagram *) diagrams->data;
      GSList *displays;
      for (displays = diagram->displays;
           displays != NULL;
           displays = g_slist_next (displays)) {
        DDisplay *ddisp = (DDisplay *) displays->data;
        gtk_window_set_transient_for (GTK_WINDOW (interface_get_toolbox_shell ()),
                                      GTK_WINDOW (ddisp->shell));
      }
    }
  } else {
    GtkWindow *shell = GTK_WINDOW (interface_get_toolbox_shell ());
    if (shell) {
      gtk_window_set_transient_for (shell, NULL);
    }
  }
}


static void
ui_menubar_toggled (GtkCheckButton *check,
                    gpointer        data)
{
  prefs.new_view.use_menu_bar = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  persistence_set_boolean ("use_menu_bar", prefs.new_view.use_menu_bar);
  update_floating_toolbox ();
}


static void
ui_toolbox_above_toggled (GtkCheckButton *check,
                          gpointer        data)
{
  prefs.toolbox_on_top = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  persistence_set_boolean ("toolbox_on_top", prefs.toolbox_on_top);
  update_floating_toolbox ();
}


struct SetCurrentUnit {
  DiaUnit    to_set;
  GtkWidget *combo;
};


static gboolean
set_current_unit (GtkTreeModel *model,
                  GtkTreePath  *path,
                  GtkTreeIter  *iter,
                  gpointer      data)
{
  struct SetCurrentUnit *find = data;
  DiaUnit unit;

  gtk_tree_model_get (model, iter, COL_UNIT, &unit, -1);

  if (unit == find->to_set) {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (find->combo), iter);

    return TRUE;
  }

  return FALSE;
}


static void
dd_portrait_toggled (GtkCheckButton *check,
                     gpointer        data)
{
  prefs.new_diagram.is_portrait = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  persistence_set_boolean ("is_portrait", prefs.new_diagram.is_portrait);
}


static void
dd_type_changed (GtkComboBox *combo,
                 gpointer     data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  char *paper;

  model = gtk_combo_box_get_model (combo);

  if (gtk_combo_box_get_active_iter (combo, &iter)) {
    gtk_tree_model_get (model, &iter, COL_NAME, &paper, -1);

    g_clear_pointer (&prefs.new_diagram.papertype, g_free);
    prefs.new_diagram.papertype = paper;

    persistence_set_string ("new_diagram_papertype", paper);
  }
}


static void
dd_background_changed (DiaColourSelector *selector,
                       gpointer           data)
{
  dia_colour_selector_get_colour (selector, &prefs.new_diagram.bg_color);
  persistence_set_color ("new_diagram_bgcolour", &prefs.new_diagram.bg_color);
}


static void
dd_compress_toggled (GtkCheckButton *check,
                     gpointer        data)
{
  prefs.new_diagram.compress_save = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  persistence_set_boolean ("compress_save", prefs.new_diagram.compress_save);
}


static void
dd_cp_visible_toggled (GtkCheckButton *check,
                       gpointer        data)
{
  prefs.show_cx_pts = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  persistence_set_boolean ("show_cx_pts", prefs.show_cx_pts);
}


static void
dd_cp_snap_toggled (GtkCheckButton *check,
                    gpointer        data)
{
  prefs.snap_object = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  persistence_set_boolean ("snap_object", prefs.snap_object);
}


struct SetCurrentPaper {
  const char *to_set;
  GtkWidget  *combo;
};


static gboolean
set_current_paper (GtkTreeModel *model,
                   GtkTreePath  *path,
                   GtkTreeIter  *iter,
                   gpointer      data)
{
  struct SetCurrentPaper *find = data;
  char *paper;

  gtk_tree_model_get (model, iter, COL_NAME, &paper, -1);

  if (g_strcmp0 (paper, find->to_set) == 0) {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (find->combo), iter);

    g_clear_pointer (&paper, g_free);

    return TRUE;
  }

  g_clear_pointer (&paper, g_free);

  return FALSE;
}


static void
vd_width_value_changed (GtkSpinButton *spin,
                        gpointer       data)
{
  prefs.new_view.width = gtk_spin_button_get_value (spin);
  persistence_set_integer ("new_view_width", prefs.new_view.width);
}


static void
vd_height_value_changed (GtkSpinButton *spin,
                         gpointer       data)
{
  prefs.new_view.height = gtk_spin_button_get_value (spin);
  persistence_set_integer ("new_view_height", prefs.new_view.height);
}


static void
vd_zoom_value_changed (GtkSpinButton *spin,
                       gpointer       data)
{
  prefs.new_view.zoom = gtk_spin_button_get_value (spin);
  persistence_set_real ("new_view_zoom", prefs.new_view.zoom);
}


static void
vd_antialiased_toggled (GtkCheckButton *check,
                        gpointer        data)
{
  prefs.view_antialiased = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  persistence_set_boolean ("view_antialiased", prefs.view_antialiased);
}


static void
vd_pb_visible_toggled (GtkCheckButton *check,
                       gpointer        data)
{
  prefs.pagebreak.visible = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  persistence_set_boolean ("pagebreak_visible", prefs.pagebreak.visible);
}


static void
vd_pb_colour_changed (DiaColourSelector *selector,
                      gpointer           data)
{
  dia_colour_selector_get_colour (selector, &prefs.new_diagram.pagebreak_color);
  persistence_set_color ("pagebreak_colour", &prefs.new_diagram.pagebreak_color);
}


static void
vd_pb_solid_toggled (GtkCheckButton *check,
                     gpointer        data)
{
  prefs.pagebreak.solid = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  persistence_set_boolean ("pagebreak_solid", prefs.pagebreak.solid);
}


static void
vd_guide_visible_toggled (GtkCheckButton *check,
                          gpointer        data)
{
  prefs.guides_visible = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  persistence_set_boolean ("show_guides", prefs.guides_snap);
}


static void
vd_guide_snap_toggled (GtkCheckButton *check,
                       gpointer        data)
{
  prefs.guides_snap = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  persistence_set_boolean ("snap_to_guides", prefs.guides_snap);
}


static void
vd_guide_colour_changed (DiaColourSelector *selector,
                         gpointer           data)
{
  dia_colour_selector_get_colour (selector, &prefs.new_diagram.guide_color);
  persistence_set_color ("guide_colour", &prefs.new_diagram.guide_color);
}


static void
fv_png_changed (GtkComboBox *combo,
                gpointer     data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  char *filter;

  model = gtk_combo_box_get_model (combo);

  if (gtk_combo_box_get_active_iter (combo, &iter)) {
    gtk_tree_model_get (model, &iter, COL_UNIT, &filter, -1);

    g_clear_pointer (&prefs.favored_filter.png, g_free);
    prefs.favored_filter.png = filter;
    filter_set_favored_export ("PNG", filter);
    persistence_set_string ("favored_png_export", filter);
  }
}


static void
fv_svg_changed (GtkComboBox *combo,
                gpointer     data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  char *filter;

  model = gtk_combo_box_get_model (combo);

  if (gtk_combo_box_get_active_iter (combo, &iter)) {
    gtk_tree_model_get (model, &iter, COL_UNIT, &filter, -1);

    g_clear_pointer (&prefs.favored_filter.svg, g_free);
    prefs.favored_filter.svg = filter;
    filter_set_favored_export ("SVG", filter);
    persistence_set_string ("favored_svg_export", filter);
  }
}


static void
fv_ps_changed (GtkComboBox *combo,
               gpointer     data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  char *filter;

  model = gtk_combo_box_get_model (combo);

  if (gtk_combo_box_get_active_iter (combo, &iter)) {
    gtk_tree_model_get (model, &iter, COL_UNIT, &filter, -1);

    g_clear_pointer (&prefs.favored_filter.ps, g_free);
    prefs.favored_filter.ps = filter;
    filter_set_favored_export ("PS", filter);
    persistence_set_string ("favored_ps_export", filter);
  }
}


static void
fv_wmf_changed (GtkComboBox *combo,
                gpointer     data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  char *filter;

  model = gtk_combo_box_get_model (combo);

  if (gtk_combo_box_get_active_iter (combo, &iter)) {
    gtk_tree_model_get (model, &iter, COL_UNIT, &filter, -1);

    g_clear_pointer (&prefs.favored_filter.wmf, g_free);
    prefs.favored_filter.wmf = filter;
    filter_set_favored_export ("WMF", filter);
    persistence_set_string ("favored_wmf_export", filter);
  }
}


static void
fv_emf_changed (GtkComboBox *combo,
                gpointer     data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  char *filter;

  model = gtk_combo_box_get_model (combo);

  if (gtk_combo_box_get_active_iter (combo, &iter)) {
    gtk_tree_model_get (model, &iter, COL_UNIT, &filter, -1);

    g_clear_pointer (&prefs.favored_filter.emf, g_free);
    prefs.favored_filter.emf = filter;
    filter_set_favored_export ("EMF", filter);
    persistence_set_string ("favored_emf_export", filter);
  }
}


struct SetCurrentFilter {
  const char *to_set;
  GtkWidget  *combo;
};


static gboolean
set_current_filter (GtkTreeModel *model,
                    GtkTreePath  *path,
                    GtkTreeIter  *iter,
                    gpointer      data)
{
  struct SetCurrentFilter *find = data;
  char *filter;

  gtk_tree_model_get (model, iter, COL_UNIT, &filter, -1);

  if (g_strcmp0 (filter, find->to_set) == 0) {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (find->combo), iter);

    g_clear_pointer (&filter, g_free);

    return TRUE;
  }

  g_clear_pointer (&filter, g_free);

  return FALSE;
}


static void
gl_visible_toggled (GtkCheckButton *check,
                    gpointer        data)
{
  prefs.grid.visible = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  persistence_set_boolean ("grid_visible", prefs.grid.visible);
}


static void
gl_snap_toggled (GtkCheckButton *check,
                 gpointer        data)
{
  prefs.grid.snap = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check));
  persistence_set_boolean ("grid_snap", prefs.grid.snap);
}


static void
gl_color_changed (DiaColourSelector *selector,
                  gpointer           data)
{
  dia_colour_selector_get_colour (selector, &prefs.new_diagram.grid_color);
  persistence_set_color ("grid_colour", &prefs.new_diagram.grid_color);
}


static void
gl_lines_value_changed (GtkSpinButton *spin,
                        gpointer       data)
{
  prefs.grid.major_lines = gtk_spin_button_get_value (spin);
  persistence_set_integer ("grid_major", prefs.grid.major_lines);
}


static void
gl_man_cs_value_changed (GtkSpinButton *spin,
                         gpointer       data)
{
  prefs.grid.x = gtk_spin_button_get_value (spin);
  persistence_set_real ("grid_x", prefs.grid.x);
}


static void
gl_man_rs_value_changed (GtkSpinButton *spin,
                         gpointer       data)
{
  prefs.grid.y = gtk_spin_button_get_value (spin);
  persistence_set_real ("grid_y", prefs.grid.y);
}


static void
gl_man_cvs_value_changed (GtkSpinButton *spin,
                          gpointer       data)
{
  prefs.grid.vis_x = gtk_spin_button_get_value (spin);
  persistence_set_integer ("grid_vis_x", prefs.grid.vis_x);
}


static void
gl_man_rvs_value_changed (GtkSpinButton *spin,
                          gpointer       data)
{
  prefs.grid.vis_y = gtk_spin_button_get_value (spin);
  persistence_set_integer ("grid_vis_y", prefs.grid.vis_y);
}


static void
gl_hex_size_value_changed (GtkSpinButton *spin,
                           gpointer       data)
{
  prefs.grid.hex_size = gtk_spin_button_get_value (spin);
  persistence_set_real ("grid_hex_size", prefs.grid.hex_size);
}


static void
gl_update_sensitive (GtkRadioButton *radio,
                     gpointer        data)
{
  DiaPreferencesDialog *self = DIA_PREFERENCES_DIALOG (data);
  gboolean manual_grid;

  prefs.grid.dynamic =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->gl_dynamic));
  prefs.grid.hex =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->gl_hex));
  manual_grid =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->gl_manual));

  persistence_set_boolean ("grid_dynamic", prefs.grid.dynamic);
  persistence_set_boolean ("grid_hex", prefs.grid.hex);

  gtk_widget_set_sensitive (self->manual_props, manual_grid);
  gtk_widget_set_sensitive (self->gl_hex_size, prefs.grid.hex);
}


static void
fill_exporter_list (GtkListStore *store, const char *type)
{
  GtkTreeIter iter;
  GList *list = filter_get_unique_export_names (type);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      // TRANSLATORS: The user hasn't set a preferred filter
                      COL_NAME, _("Unset"),
                      COL_UNIT, "any",
                      -1);

  for (GList *l = list; l; l = g_list_next (l)) {
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
                        COL_NAME, l->data,
                        COL_UNIT, l->data,
                        -1);
  }
}


static void
dia_preferences_dialog_init (DiaPreferencesDialog *self)
{
  struct SetCurrentUnit find_unit;
  struct SetCurrentPaper find_paper;
  struct SetCurrentFilter find_filter;
  GtkWidget *dialog_vbox;
  DiaBuilder *builder;
  GtkWidget *action_area;
  GtkWidget *content;
  GtkListStore *units;
  GtkListStore *paper;
  GtkListStore *pngs;
  GtkListStore *svgs;
  GtkListStore *pss;
  GtkListStore *wmfs;
  GtkListStore *emfs;
  GtkWidget *ui_reset_tools;
  GtkAdjustment *ui_undo_spin_adj;
  GtkWidget *ui_reverse_drag;
  GtkAdjustment *ui_recent_spin_adj;
  GtkWidget *ui_length_unit;
  GtkWidget *ui_font_unit;
  GtkAdjustment *ui_snap_distance_adj;
  GtkWidget *ui_menubar;
  GtkWidget *ui_toolbox_above;
  GtkWidget *dd_portrait;
  GtkWidget *dd_type;
  GtkWidget *dd_background;
  GtkWidget *dd_compress;
  GtkWidget *dd_cp_visible;
  GtkWidget *dd_cp_snap;
  GtkAdjustment *vd_width_adj;
  GtkAdjustment *vd_height_adj;
  GtkAdjustment *vd_zoom_adj;
  GtkWidget *vd_antialiased;
  GtkWidget *vd_pb_visible;
  GtkWidget *vd_pb_colour;
  GtkWidget *vd_pb_solid;
  GtkWidget *vd_guide_visible;
  GtkWidget *vd_guide_snap;
  GtkWidget *vd_guide_colour;
  GtkWidget *fv_png;
  GtkWidget *fv_svg;
  GtkWidget *fv_ps;
  GtkWidget *fv_wmf;
  GtkWidget *fv_emf;
  GtkWidget *gl_visible;
  GtkWidget *gl_snap;
  GtkWidget *gl_color;
  GtkAdjustment *gl_lines_adj;
  GtkAdjustment *gl_man_cs_adj;
  GtkAdjustment *gl_man_rs_adj;
  GtkAdjustment *gl_man_cvs_adj;
  GtkAdjustment *gl_man_rvs_adj;
  GtkAdjustment *gl_hex_size_adj;
  int j = 0;
  const char *name;

  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          _("_Done"), GTK_RESPONSE_OK,
                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);

  gtk_window_set_role (GTK_WINDOW (self), "preferences_window");
  gtk_window_set_title (GTK_WINDOW (self), _("Preferences"));

  action_area = gtk_dialog_get_action_area (GTK_DIALOG (self));
  gtk_widget_set_margin_bottom (action_area, 6);
  gtk_widget_set_margin_top (action_area, 6);
  gtk_widget_set_margin_start (action_area, 6);
  gtk_widget_set_margin_end (action_area, 6);

  dialog_vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));

  builder = dia_builder_new ("ui/preferences-dialog.ui");

  dia_builder_get (builder,
                   "content", &content,
                   "units", &units,
                   "paper", &paper,
                   "pngs", &pngs,
                   "svgs", &svgs,
                   "pss", &pss,
                   "wmfs", &wmfs,
                   "emfs", &emfs,
                   /* User Interface */
                   "ui_reset_tools", &ui_reset_tools,
                   "ui_undo_spin_adj", &ui_undo_spin_adj,
                   "ui_reverse_drag", &ui_reverse_drag,
                   "ui_recent_spin_adj", &ui_recent_spin_adj,
                   "ui_length_unit", &ui_length_unit,
                   "ui_font_unit", &ui_font_unit,
                   "ui_snap_distance_adj", &ui_snap_distance_adj,
                   "ui_menubar", &ui_menubar,
                   "ui_toolbox_above", &ui_toolbox_above,
                   /* Diagram Defaults */
                   "dd_portrait", &dd_portrait,
                   "dd_type", &dd_type,
                   "dd_background", &dd_background,
                   "dd_compress", &dd_compress,
                   "dd_cp_visible", &dd_cp_visible,
                   "dd_cp_snap", &dd_cp_snap,
                   /* View Defaults */
                   "vd_width_adj", &vd_width_adj,
                   "vd_height_adj", &vd_height_adj,
                   "vd_zoom_adj", &vd_zoom_adj,
                   "vd_antialiased", &vd_antialiased,
                   "vd_pb_visible", &vd_pb_visible,
                   "vd_pb_colour", &vd_pb_colour,
                   "vd_pb_solid", &vd_pb_solid,
                   "vd_guide_visible", &vd_guide_visible,
                   "vd_guide_snap", &vd_guide_snap,
                   "vd_guide_colour", &vd_guide_colour,
                   /* Favorites */
                   "fv_png", &fv_png,
                   "fv_svg", &fv_svg,
                   "fv_ps", &fv_ps,
                   "fv_wmf", &fv_wmf,
                   "fv_emf", &fv_emf,
                   /* Grid Lines */
                   "gl_visible", &gl_visible,
                   "gl_snap", &gl_snap,
                   "gl_color", &gl_color,
                   "gl_lines_adj", &gl_lines_adj,
                   "gl_dynamic", &self->gl_dynamic,
                   "gl_manual", &self->gl_manual,
                   "manual_props", &self->manual_props,
                   "gl_man_cs_adj", &gl_man_cs_adj,
                   "gl_man_rs_adj", &gl_man_rs_adj,
                   "gl_man_cvs_adj", &gl_man_cvs_adj,
                   "gl_man_rvs_adj", &gl_man_rvs_adj,
                   "gl_hex", &self->gl_hex,
                   "gl_hex_size", &self->gl_hex_size,
                   "gl_hex_size_adj", &gl_hex_size_adj,
                   NULL);

  for (DiaUnit unit = 0; unit < DIA_LAST_UNIT; unit++) {
    GtkTreeIter iter;

    gtk_list_store_append (units, &iter);
    gtk_list_store_set (units, &iter,
                        COL_NAME, dia_unit_get_name (unit),
                        COL_UNIT, unit,
                        -1);
  }

  while ((name = dia_paper_get_name (j))) {
    GtkTreeIter iter;

    gtk_list_store_append (paper, &iter);
    gtk_list_store_set (paper, &iter,
                        COL_NAME, name,
                        COL_UNIT, j,
                        -1);

    j++;
  }

  fill_exporter_list (pngs, "PNG");
  fill_exporter_list (svgs, "SVG");
  fill_exporter_list (pss, "PS");
  fill_exporter_list (wmfs, "WMF");
  fill_exporter_list (emfs, "EMF");

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ui_reset_tools),
                                prefs.reset_tools_after_create);
  gtk_adjustment_set_value (ui_undo_spin_adj, prefs.undo_depth);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ui_reverse_drag),
                                prefs.reverse_rubberbanding_intersects);
  gtk_adjustment_set_value (ui_recent_spin_adj,
                            prefs.recent_documents_list_size);
  find_unit.combo = ui_length_unit;
  find_unit.to_set = prefs_get_length_unit ();
  gtk_tree_model_foreach (GTK_TREE_MODEL (units), set_current_unit, &find_unit);
  find_unit.combo = ui_font_unit;
  find_unit.to_set = prefs_get_fontsize_unit ();
  gtk_tree_model_foreach (GTK_TREE_MODEL (units), set_current_unit, &find_unit);
  gtk_adjustment_set_value (ui_snap_distance_adj, prefs.snap_distance);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ui_menubar),
                                prefs.new_view.use_menu_bar);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ui_toolbox_above),
                                prefs.toolbox_on_top);


  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dd_portrait),
                                prefs.new_diagram.is_portrait);
  find_paper.combo = dd_type;
  find_paper.to_set = prefs.new_diagram.papertype;
  gtk_tree_model_foreach (GTK_TREE_MODEL (paper),
                          set_current_paper,
                          &find_paper);
  dia_colour_selector_set_colour (DIA_COLOUR_SELECTOR (dd_background),
                                  &prefs.new_diagram.bg_color);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dd_compress),
                                prefs.new_diagram.compress_save);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dd_cp_visible),
                                prefs.show_cx_pts);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dd_cp_snap),
                                prefs.snap_object);


  gtk_adjustment_set_value (vd_width_adj, prefs.new_view.width);
  gtk_adjustment_set_value (vd_height_adj, prefs.new_view.height);
  gtk_adjustment_set_value (vd_zoom_adj, prefs.new_view.zoom);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (vd_antialiased),
                                prefs.view_antialiased);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (vd_pb_visible),
                                prefs.pagebreak.visible);
  dia_colour_selector_set_colour (DIA_COLOUR_SELECTOR (vd_pb_colour),
                                  &prefs.new_diagram.pagebreak_color);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (vd_pb_solid),
                                prefs.pagebreak.visible);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (vd_guide_visible),
                                prefs.guides_visible);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (vd_guide_snap),
                                prefs.guides_snap);
  dia_colour_selector_set_colour (DIA_COLOUR_SELECTOR (vd_guide_colour),
                                  &prefs.new_diagram.guide_color);


  find_filter.combo = fv_png;
  find_filter.to_set = prefs.favored_filter.png;
  gtk_tree_model_foreach (GTK_TREE_MODEL (pngs),
                          set_current_filter,
                          &find_filter);
  find_filter.combo = fv_svg;
  find_filter.to_set = prefs.favored_filter.svg;
  gtk_tree_model_foreach (GTK_TREE_MODEL (svgs),
                          set_current_filter,
                          &find_filter);
  find_filter.combo = fv_ps;
  find_filter.to_set = prefs.favored_filter.ps;
  gtk_tree_model_foreach (GTK_TREE_MODEL (pss),
                          set_current_filter,
                          &find_filter);
  find_filter.combo = fv_wmf;
  find_filter.to_set = prefs.favored_filter.wmf;
  gtk_tree_model_foreach (GTK_TREE_MODEL (wmfs),
                          set_current_filter,
                          &find_filter);
  find_filter.combo = fv_emf;
  find_filter.to_set = prefs.favored_filter.emf;
  gtk_tree_model_foreach (GTK_TREE_MODEL (emfs),
                          set_current_filter,
                          &find_filter);


  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gl_visible),
                                prefs.grid.visible);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gl_snap),
                                prefs.grid.snap);
  dia_colour_selector_set_colour (DIA_COLOUR_SELECTOR (gl_color),
                                  &prefs.new_diagram.grid_color);
  gtk_adjustment_set_value (gl_lines_adj,
                             prefs.grid.major_lines);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->gl_dynamic),
                                prefs.grid.dynamic);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->gl_manual),
                                !prefs.grid.dynamic && !prefs.grid.hex);
  gtk_adjustment_set_value (gl_man_cs_adj, prefs.grid.x);
  gtk_adjustment_set_value (gl_man_rs_adj, prefs.grid.y);
  gtk_adjustment_set_value (gl_man_cvs_adj, prefs.grid.vis_x);
  gtk_adjustment_set_value (gl_man_rvs_adj, prefs.grid.vis_y);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->gl_hex),
                                prefs.grid.hex);
  gtk_adjustment_set_value (gl_hex_size_adj, prefs.grid.hex_size);


  gl_update_sensitive (NULL, self);


  dia_builder_connect (builder,
                       self,
                       /* User Interface */
                       "ui_reset_tools_toggled", G_CALLBACK (ui_reset_tools_toggled),
                       "ui_undo_spin_changed", G_CALLBACK (ui_undo_spin_changed),
                       "ui_reverse_drag_toggled", G_CALLBACK (ui_reverse_drag_toggled),
                       "ui_recent_spin_changed", G_CALLBACK (ui_recent_spin_changed),
                       "ui_length_unit_changed", G_CALLBACK (ui_length_unit_changed),
                       "ui_font_unit_changed", G_CALLBACK (ui_font_unit_changed),
                       "ui_snap_distance_changed", G_CALLBACK (ui_snap_distance_changed),
                       "ui_menubar_toggled", G_CALLBACK (ui_menubar_toggled),
                       "ui_toolbox_above_toggled", G_CALLBACK (ui_toolbox_above_toggled),
                       /* Diagram Defaults */
                       "dd_portrait_toggled", G_CALLBACK (dd_portrait_toggled),
                       "dd_type_changed", G_CALLBACK (dd_type_changed),
                       "dd_background_changed", G_CALLBACK (dd_background_changed),
                       "dd_compress_toggled", G_CALLBACK (dd_compress_toggled),
                       "dd_cp_visible_toggled", G_CALLBACK (dd_cp_visible_toggled),
                       "dd_cp_snap_toggled", G_CALLBACK (dd_cp_snap_toggled),
                       /* View Defaults */
                       "vd_width_value_changed", G_CALLBACK (vd_width_value_changed),
                       "vd_height_value_changed", G_CALLBACK (vd_height_value_changed),
                       "vd_zoom_value_changed", G_CALLBACK (vd_zoom_value_changed),
                       "vd_antialiased_toggled", G_CALLBACK (vd_antialiased_toggled),
                       "vd_pb_visible_toggled", G_CALLBACK (vd_pb_visible_toggled),
                       "vd_pb_colour_changed", G_CALLBACK (vd_pb_colour_changed),
                       "vd_pb_solid_toggled", G_CALLBACK (vd_pb_solid_toggled),
                       "vd_guide_visible_toggled", G_CALLBACK (vd_guide_visible_toggled),
                       "vd_guide_snap_toggled", G_CALLBACK (vd_guide_snap_toggled),
                       "vd_guide_colour_changed", G_CALLBACK (vd_guide_colour_changed),
                       /* Favorites */
                       "fv_png_changed", G_CALLBACK (fv_png_changed),
                       "fv_svg_changed", G_CALLBACK (fv_svg_changed),
                       "fv_ps_changed", G_CALLBACK (fv_ps_changed),
                       "fv_wmf_changed", G_CALLBACK (fv_wmf_changed),
                       "fv_emf_changed", G_CALLBACK (fv_emf_changed),
                       /* Grid Lines */
                       "gl_visible_toggled", G_CALLBACK (gl_visible_toggled),
                       "gl_snap_toggled", G_CALLBACK (gl_snap_toggled),
                       "gl_color_changed", G_CALLBACK (gl_color_changed),
                       "gl_lines_value_changed", G_CALLBACK (gl_lines_value_changed),
                       "gl_man_cs_value_changed", G_CALLBACK (gl_man_cs_value_changed),
                       "gl_man_rs_value_changed", G_CALLBACK (gl_man_rs_value_changed),
                       "gl_man_cvs_value_changed", G_CALLBACK (gl_man_cvs_value_changed),
                       "gl_man_rvs_value_changed", G_CALLBACK (gl_man_rvs_value_changed),
                       "gl_hex_size_value_changed", G_CALLBACK (gl_hex_size_value_changed),
                       "gl_update_sensitive", G_CALLBACK (gl_update_sensitive),
                       NULL);

  gtk_container_add (GTK_CONTAINER (dialog_vbox), content);

  g_clear_object (&builder);
}


void
dia_preferences_dialog_show (void)
{
  static GtkWidget *prefs_dialog = NULL;

  if (prefs_dialog == NULL) {
    prefs_dialog = g_object_new (DIA_TYPE_PREFERENCES_DIALOG, NULL);
    gtk_window_set_transient_for (GTK_WINDOW (prefs_dialog),
                                  GTK_WINDOW (interface_get_toolbox_shell ()));
    g_signal_connect (G_OBJECT (prefs_dialog),
                      "destroy",
                      G_CALLBACK (gtk_widget_destroyed),
                      &prefs_dialog);
  }

  gtk_widget_show (prefs_dialog);
}


void
dia_preferences_init (void)
{
  GEnumClass *unit_class = g_type_class_ref (DIA_TYPE_UNIT);
  Color default_bg = { 1.0, 1.0, 1.0, 1.0 };
  Color break_bg = { 0.0, 0.0, 0.6, 1.0 };
  Color guide_bg = { 0.0, 1.0, 0.0, 1.0 };
  Color grid_bg = { 0.85, .90, .90, 1.0 };

  prefs.reset_tools_after_create = persistence_register_boolean ("reset_tools_after_create", TRUE);
  prefs.undo_depth = persistence_register_integer ("undo_depth", 15);
  prefs.reverse_rubberbanding_intersects = persistence_register_boolean ("reverse_rubberbanding_intersects", TRUE);
  prefs.recent_documents_list_size = persistence_register_integer ("recent_documents_list_size", 5);
  /* This used to be length_unit and font_unit but the underlying representation changed */
  prefs_set_length_unit (g_enum_get_value_by_nick (unit_class, persistence_register_string ("length-unit", "centimetre"))->value);
  prefs_set_fontsize_unit (g_enum_get_value_by_nick (unit_class, persistence_register_string ("font-unit", "point"))->value);
  prefs.snap_distance = persistence_register_integer ("snap_distance", 10);
  prefs.new_view.use_menu_bar = persistence_register_boolean ("use_menu_bar", TRUE);
  prefs.toolbox_on_top = persistence_register_boolean ("toolbox_on_top", FALSE);

  prefs.new_diagram.is_portrait = persistence_register_boolean ("is_portrait", TRUE);
  prefs.new_diagram.papertype = persistence_register_string ("new_diagram_papertype", dia_paper_get_name (get_default_paper ()));
  prefs.new_diagram.bg_color = *persistence_register_color ("new_diagram_bgcolour", &default_bg);
  prefs.new_diagram.compress_save = persistence_register_boolean ("compress_save", FALSE);
  prefs.show_cx_pts = persistence_register_boolean ("show_cx_pts", TRUE);
  prefs.snap_object = persistence_register_boolean ("snap_object", TRUE);

  prefs.new_view.width = persistence_register_integer ("new_view_width", 500);
  prefs.new_view.height = persistence_register_integer ("new_view_height", 400);
  prefs.new_view.zoom = persistence_register_real ("new_view_zoom", 100);
  prefs.view_antialiased = persistence_register_boolean ("view_antialiased", TRUE);
  prefs.pagebreak.visible = persistence_register_boolean ("pagebreak_visible", TRUE);
  prefs.new_diagram.pagebreak_color = *persistence_register_color ("pagebreak_colour", &break_bg);
  prefs.pagebreak.solid = persistence_register_boolean ("pagebreak_solid", TRUE);
  prefs.guides_visible = persistence_register_boolean ("show_guides", TRUE);
  prefs.guides_snap = persistence_register_boolean ("snap_to_guides", TRUE);
  prefs.new_diagram.guide_color = *persistence_register_color ("guide_colour", &guide_bg);

  prefs.favored_filter.png = persistence_register_string ("favored_png_export", "any");
  prefs.favored_filter.svg = persistence_register_string ("favored_svg_export", "any");
  prefs.favored_filter.ps = persistence_register_string ("favored_ps_export", "any");
  prefs.favored_filter.wmf = persistence_register_string ("favored_wmf_export", "any");
  prefs.favored_filter.emf = persistence_register_string ("favored_emf_export", "any");

  prefs.grid.visible = persistence_register_boolean ("grid_visible", TRUE);
  prefs.grid.snap = persistence_register_boolean ("grid_snap", FALSE);
  prefs.new_diagram.grid_color = *persistence_register_color ("grid_colour", &grid_bg);
  prefs.grid.major_lines = persistence_register_integer ("grid_major", 5);
  prefs.grid.dynamic = persistence_register_boolean ("grid_dynamic", TRUE);
  prefs.grid.x = persistence_register_integer ("grid_x", 1);
  prefs.grid.y = persistence_register_integer ("grid_y", 1);
  prefs.grid.vis_x = persistence_register_real ("grid_vis_x", 1);
  prefs.grid.vis_y = persistence_register_real ("grid_vis_y", 1);
  prefs.grid.hex = persistence_register_boolean ("grid_hex", FALSE);
  prefs.grid.hex_size = persistence_register_real ("grid_hex_size", 1);
}
