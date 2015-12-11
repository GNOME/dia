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
#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#include <gtk/gtk.h>

#include "intl.h"
#include "widgets.h"
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

#ifdef G_OS_WIN32
#include <io.h> /* open, close */
#endif

struct DiaPreferences prefs;

enum DiaPrefType {
  PREF_NONE,
  PREF_BOOLEAN,
  PREF_INT,
  PREF_UINT,
  PREF_REAL,
  PREF_UREAL,
  PREF_COLOUR,
  PREF_CHOICE,
  PREF_STRING,
  PREF_END_GROUP
};

typedef struct _DiaPrefData {
  char *name;
  enum DiaPrefType type;
  int offset;
  const void *default_value;
  int tab;
  char *label_text;
  GtkWidget *widget;
  gboolean hidden;
  GList *(*choice_list_function)(struct _DiaPrefData *pref);
  /** A function to call after a preference item has been updated. */
  void (*update_function)(struct _DiaPrefData *pref, gpointer ptr);
  const char *key;
} DiaPrefData;

static void update_floating_toolbox(DiaPrefData *pref, gpointer ptr);
static void update_internal_prefs(DiaPrefData *pref, gpointer ptr);

static int default_true = 1;
static int default_false = 0;
static int default_int_vis_x = 1;
static int default_int_vis_y = 1;
static int default_major_lines = 5;
static real default_real_one = 1.0;
static real default_real_zoom = 100.0;
static int default_int_w = 500;
static int default_int_h = 400;
static int default_undo_depth = 15;
static guint default_recent_documents = 5;
static Color default_colour = DEFAULT_GRID_COLOR;
static Color pbreak_colour = DEFAULT_PAGEBREAK_COLOR;
static const gchar *default_paper_name = NULL;
static const gchar *default_length_unit = "Centimeter";
static const gchar *default_fontsize_unit = "Point";

static const char *default_favored_filter = N_("any");

struct DiaPrefsTab {
  char *title;
  GtkTable *table;
  int row;
};

typedef enum {
  UI_TAB,
  DIA_TAB,
  VIEW_TAB,
  FAVOR_TAB,
  GRID_TAB
} TabIndex;

struct DiaPrefsTab prefs_tabs[] =
{
  {N_("User Interface"), NULL, 0},
  {N_("Diagram Defaults"), NULL, 0},
  {N_("View Defaults"), NULL, 0},
  {N_("Favorites"), NULL, 0},
  {N_("Grid Lines"), NULL, 0},
};

#define NUM_PREFS_TABS (sizeof(prefs_tabs)/sizeof(struct DiaPrefsTab))

static GList *
_get_units_name_list(DiaPrefData *pref)
{
  g_return_val_if_fail(pref->key == NULL, NULL);
  return get_units_name_list();
}
static GList *
_get_paper_name_list(DiaPrefData *pref)
{
  g_return_val_if_fail(pref->key == NULL, NULL);
  return get_paper_name_list();
}
static GList *
get_exporter_names (DiaPrefData *pref)
{
  GList *list = filter_get_unique_export_names(pref->key);
  list = g_list_prepend (list, N_("any"));
  return list;
}

static void
set_favored_exporter (DiaPrefData *pref, gpointer ptr)
{
  char *val = *((gchar **)ptr);
  filter_set_favored_export(pref->key, val);
}

/* retrive a structure offset */
#ifdef offsetof
#define PREF_OFFSET(field)        ((int) offsetof (struct DiaPreferences, field))
#else /* !offsetof */
#define PREF_OFFSET(field)        ((int) ((char*) &((struct DiaPreferences *) 0)->field))
#endif /* !offsetof */

DiaPrefData prefs_data[] =
{
  { "reset_tools_after_create", PREF_BOOLEAN, PREF_OFFSET(reset_tools_after_create), 
    &default_true, UI_TAB, N_("Reset tools after create") },

  { "undo_depth",               PREF_UINT,    PREF_OFFSET(undo_depth), 
    &default_undo_depth, UI_TAB, N_("Number of undo levels:") },

  { "reverse_rubberbanding_intersects", PREF_BOOLEAN, PREF_OFFSET(reverse_rubberbanding_intersects), 
    &default_true, UI_TAB, N_("Reverse dragging selects\nintersecting objects") },

  { "recent_documents_list_size", PREF_UINT, PREF_OFFSET(recent_documents_list_size), 
    &default_recent_documents, 0, N_("Recent documents list size:") },

  { "use_menu_bar", PREF_BOOLEAN, PREF_OFFSET(new_view.use_menu_bar), 
    &default_true, UI_TAB, N_("Use menu bar") },

  { "toolbox_on_top", PREF_BOOLEAN, PREF_OFFSET(toolbox_on_top),
    &default_false, UI_TAB, N_("Keep tool box on top of diagram windows"),
    NULL, FALSE, NULL, update_floating_toolbox},
  { "length_unit", PREF_CHOICE, PREF_OFFSET(length_unit),
    &default_length_unit, UI_TAB, N_("Length unit:"), NULL, FALSE,
    _get_units_name_list, update_internal_prefs },
  { "fontsize_unit", PREF_CHOICE, PREF_OFFSET(fontsize_unit),
    &default_fontsize_unit, UI_TAB, N_("Font size unit:"), NULL, FALSE,
    _get_units_name_list, update_internal_prefs },
  
  { NULL, PREF_NONE, 0, NULL, DIA_TAB, N_("New diagram:") },
  { "is_portrait", PREF_BOOLEAN, PREF_OFFSET(new_diagram.is_portrait), &default_true, DIA_TAB, N_("Portrait") },
  { "new_diagram_papertype", PREF_CHOICE, PREF_OFFSET(new_diagram.papertype),
    &default_paper_name, DIA_TAB, N_("Paper type:"), NULL, FALSE, _get_paper_name_list },
  { "new_diagram_bgcolour", PREF_COLOUR, PREF_OFFSET(new_diagram.bg_color),
    &color_white, DIA_TAB, N_("Background Color:") },
  { "compress_save",            PREF_BOOLEAN, PREF_OFFSET(new_diagram.compress_save), 
    &default_true, DIA_TAB, N_("Compress saved files") },
  { NULL, PREF_END_GROUP, 0, NULL, DIA_TAB, NULL },

  { NULL, PREF_NONE, 0, NULL, DIA_TAB, N_("Connection Points:") },
  { "show_cx_pts", PREF_BOOLEAN, PREF_OFFSET(show_cx_pts), &default_true, DIA_TAB, N_("Visible") },
  { "snap_object", PREF_BOOLEAN, PREF_OFFSET(snap_object), &default_true, DIA_TAB, N_("Snap to object") },
  { NULL, PREF_END_GROUP, 0, NULL, DIA_TAB, NULL },

  { NULL, PREF_NONE, 0, NULL, VIEW_TAB, N_("New window:") },
  { "new_view_width", PREF_UINT, PREF_OFFSET(new_view.width), &default_int_w, VIEW_TAB, N_("Width:") },
  { "new_view_height", PREF_UINT, PREF_OFFSET(new_view.height), &default_int_h, VIEW_TAB, N_("Height:") },
  { "new_view_zoom", PREF_UREAL, PREF_OFFSET(new_view.zoom), &default_real_zoom, VIEW_TAB, N_("Magnify:") },
  { NULL, PREF_END_GROUP, 0, NULL, 1, NULL },

  { NULL, PREF_NONE, 0, NULL, VIEW_TAB, N_("Page breaks:") },
  { "pagebreak_visible", PREF_BOOLEAN, PREF_OFFSET(pagebreak.visible), &default_true, VIEW_TAB, N_("Visible") },
  { "pagebreak_colour", PREF_COLOUR, PREF_OFFSET(new_diagram.pagebreak_color), &pbreak_colour, VIEW_TAB, N_("Color:") },
  { "pagebreak_solid", PREF_BOOLEAN, PREF_OFFSET(pagebreak.solid), &default_true, VIEW_TAB, N_("Solid lines") },
  { NULL, PREF_END_GROUP, 0, NULL, VIEW_TAB, NULL },
  
  { NULL, PREF_NONE, 0, NULL, VIEW_TAB, N_("Antialias:") },
  { "view_antialiased", PREF_BOOLEAN, PREF_OFFSET(view_antialiased), &default_false, VIEW_TAB, N_("view antialiased") },
  { NULL, PREF_END_GROUP, 0, NULL, VIEW_TAB, NULL },

  /* Favored Filter */
  { NULL, PREF_NONE, 0, NULL, FAVOR_TAB, N_("Export") },
  { "favored_png_export", PREF_CHOICE, PREF_OFFSET(favored_filter.png), &default_favored_filter, 
    FAVOR_TAB, N_("Portable Network Graphics"), NULL, FALSE, get_exporter_names, set_favored_exporter, "PNG" },
  { "favored_svg_export", PREF_CHOICE, PREF_OFFSET(favored_filter.svg), &default_favored_filter, 
    FAVOR_TAB, N_("Scalable Vector Graphics"), NULL, FALSE, get_exporter_names, set_favored_exporter, "SVG" },
  { "favored_ps_export", PREF_CHOICE, PREF_OFFSET(favored_filter.ps), &default_favored_filter, 
    FAVOR_TAB, N_("PostScript"), NULL, FALSE, get_exporter_names, set_favored_exporter, "PS" },
  { "favored_wmf_export", PREF_CHOICE, PREF_OFFSET(favored_filter.wmf), &default_favored_filter, 
    FAVOR_TAB, N_("Windows Metafile"), NULL, FALSE, get_exporter_names, set_favored_exporter, "WMF" },
  { "favored_emf_export", PREF_CHOICE, PREF_OFFSET(favored_filter.emf), &default_favored_filter, 
    FAVOR_TAB, N_("Enhanced Metafile"), NULL, FALSE, get_exporter_names, set_favored_exporter, "EMF" },
  { NULL, PREF_END_GROUP, 0, NULL, FAVOR_TAB, NULL },

  /*{ NULL, PREF_NONE, 0, NULL, 3, N_("Grid:") }, */
  { "grid_visible", PREF_BOOLEAN, PREF_OFFSET(grid.visible), &default_true, GRID_TAB, N_("Visible") },
  { "grid_snap", PREF_BOOLEAN, PREF_OFFSET(grid.snap), &default_false, GRID_TAB, N_("Snap to") },
  { "grid_dynamic", PREF_BOOLEAN, PREF_OFFSET(grid.dynamic), &default_true, GRID_TAB, N_("Dynamic grid resizing") },
  { "grid_x", PREF_UREAL, PREF_OFFSET(grid.x), &default_real_one, GRID_TAB, N_("X Size:") },
  { "grid_y", PREF_UREAL, PREF_OFFSET(grid.y), &default_real_one, GRID_TAB, N_("Y Size:") },
  { "grid_vis_x", PREF_UINT, PREF_OFFSET(grid.vis_x), &default_int_vis_x, GRID_TAB, N_("Visual Spacing X:") },
  { "grid_vis_y", PREF_UINT, PREF_OFFSET(grid.vis_y), &default_int_vis_y, GRID_TAB, N_("Visual Spacing Y:") },
  { "grid_colour", PREF_COLOUR, PREF_OFFSET(new_diagram.grid_color), &default_colour, GRID_TAB, N_("Color:") },
  { "grid_major", PREF_UINT, PREF_OFFSET(grid.major_lines), &default_major_lines, GRID_TAB, N_("Lines per major line") },
  { "grid_hex", PREF_BOOLEAN, PREF_OFFSET(grid.hex), &default_false, GRID_TAB, N_("Hex grid") },
  { "grid_w", PREF_UREAL, PREF_OFFSET(grid.w), &default_real_one, GRID_TAB, N_("Hex Size:") },
  /*  { "grid_solid", PREF_BOOLEAN, PREF_OFFSET(grid.solid), &default_true, 3, N_("Solid lines:") },  */

  { "fixed_icon_size", PREF_BOOLEAN,PREF_OFFSET(fixed_icon_size),
    &default_true,0,"ensure fixed icon size",NULL, TRUE},

};

#define NUM_PREFS_DATA (sizeof(prefs_data)/sizeof(DiaPrefData))

static void prefs_create_dialog(void);
static void prefs_set_value_in_widget(GtkWidget * widget, DiaPrefData *data,  gpointer ptr);
static void prefs_get_value_from_widget(GtkWidget * widget, DiaPrefData *data, gpointer ptr);
static void prefs_update_dialog_from_prefs(void);
static void prefs_update_prefs_from_dialog(void);
/* static gint prefs_apply(GtkWidget *widget, gpointer data); */


static GtkWidget *prefs_dialog = NULL;

void
prefs_show(void)
{
  prefs_create_dialog();
  gtk_widget_show(prefs_dialog);
  
  prefs_update_dialog_from_prefs();
}

void
prefs_set_defaults(void)
{
  int i;
  gpointer ptr;

  /* Since we can't call this in static initialization, we have to
   * do it here.
   */
  if (default_paper_name == NULL)
    default_paper_name = get_paper_name(get_default_paper());

  for (i=0;i<NUM_PREFS_DATA;i++) {
    ptr = (char *)&prefs + prefs_data[i].offset;

    switch (prefs_data[i].type) {
    case PREF_BOOLEAN:
      *(int *)ptr = *(int *)prefs_data[i].default_value;
      *(int *)ptr = persistence_register_boolean(prefs_data[i].name, *(int *)ptr);
      break;
    case PREF_INT:
    case PREF_UINT:
      *(int *)ptr = *(int *)prefs_data[i].default_value;
      *(int *)ptr = persistence_register_integer(prefs_data[i].name, *(int *)ptr);
      break;
    case PREF_REAL:
    case PREF_UREAL:
      *(real *)ptr = *(real *)prefs_data[i].default_value;
      *(real *)ptr = persistence_register_real(prefs_data[i].name, *(real *)ptr);
      break;
    case PREF_COLOUR:
      *(Color *)ptr = *(Color *)prefs_data[i].default_value;
      *(Color *)ptr = *persistence_register_color(prefs_data[i].name, (Color *)ptr);
      break;
    case PREF_CHOICE:
    case PREF_STRING: 
      *(gchar **)ptr = *(gchar **)prefs_data[i].default_value;
      *(gchar **)ptr = persistence_register_string(prefs_data[i].name, *(gchar **)ptr);
      break;
    case PREF_NONE:
    case PREF_END_GROUP:
      break;
    }
    /* set initial preferences, but dont talk about restarting */
    if (prefs_data[i].update_function)
      (prefs_data[i].update_function)(&prefs_data[i], ptr);
  }
  update_internal_prefs(&prefs_data[i], NULL);
}

void
prefs_save(void)
{
  int i;
  gpointer ptr;
  for (i=0;i<NUM_PREFS_DATA;i++) {
    if ((prefs_data[i].type == PREF_NONE) || (prefs_data[i].type == PREF_END_GROUP))
      continue;
    
    ptr = (char *)&prefs + prefs_data[i].offset;

    switch (prefs_data[i].type) {
    case PREF_BOOLEAN:
      persistence_set_boolean(prefs_data[i].name, *(gint *)ptr);
      break;
    case PREF_INT:
    case PREF_UINT:
      persistence_set_integer(prefs_data[i].name, *(gint *)ptr);
      break;
    case PREF_REAL:
    case PREF_UREAL:
      
      persistence_set_real(prefs_data[i].name, *(real *)ptr);
      break;
    case PREF_COLOUR:
      persistence_set_color(prefs_data[i].name, (Color *)ptr);
      break;
    case PREF_CHOICE:
    case PREF_STRING:
      persistence_set_string(prefs_data[i].name, *(gchar **)ptr);
      break;
    case PREF_NONE:
    case PREF_END_GROUP:
      break;
    }
  }
}



void
prefs_init(void)
{
  prefs_set_defaults();
}

static void
prefs_set_value_in_widget(GtkWidget * widget, DiaPrefData *data,
			  gpointer ptr)
{
  switch(data->type) {
  case PREF_BOOLEAN:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), *((int *)ptr));
    break;
  case PREF_INT:
  case PREF_UINT:
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),
			      (gfloat) (*((int *)ptr)));
    break;
  case PREF_REAL:
  case PREF_UREAL: 
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget),
			      (gfloat) (*((real *)ptr)));
    break;
  case PREF_COLOUR:
    dia_color_selector_set_color(widget, (Color *)ptr);
    break;
  case PREF_CHOICE: {
    GList *names = (data->choice_list_function)(data);
    int index;
    char *val = *((gchar**)ptr);
    for (index = 0; names != NULL; names = g_list_next(names), index++) {
      if (!val || !strcmp(val, (gchar *)names->data))
	break;
    }
    if (names == NULL) return;
    gtk_combo_box_set_active (GTK_COMBO_BOX (widget), index);
    break;
  }
  case PREF_STRING:
    gtk_entry_set_text(GTK_ENTRY(widget), (gchar *)(*((gchar **)ptr)));
    break;
  case PREF_NONE:
  case PREF_END_GROUP:
    break;
  }
}

static void
prefs_get_value_from_widget(GtkWidget * widget, DiaPrefData *data,
			    gpointer ptr)
{
  gboolean changed = FALSE;
  switch(data->type) {
  case PREF_BOOLEAN: {
      int prev = *((int *)ptr);
      *((int *)ptr) = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
      changed = (prev != *((int *)ptr));
    }
    break;
  case PREF_INT:
  case PREF_UINT: {
      int prev = *((int *)ptr);
      *((int *)ptr) = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
      changed = (prev != *((int *)ptr));
    }
    break;
  case PREF_REAL:
  case PREF_UREAL: {
      real prev = *((real *)ptr);
      *((real *)ptr) = (real)
        gtk_spin_button_get_value (GTK_SPIN_BUTTON(widget));
      changed = (prev != *((real *)ptr));
    }
    break;
  case PREF_COLOUR: {
      Color prev = *(Color *)ptr;
      dia_color_selector_get_color(widget, (Color *)ptr);
      changed = memcmp (&prev, ptr, sizeof(Color));
    }
    break;
  case PREF_CHOICE: {
    int index = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
    GList *names = (data->choice_list_function)(data);
    *((gchar **)ptr) = g_strdup((gchar *)g_list_nth_data(names, index));
    /* XXX changed */
    changed = TRUE;
    break;
  }
  case PREF_STRING:
    *((gchar **)ptr) = (gchar *)gtk_entry_get_text(GTK_ENTRY(widget));
    /* XXX changed */
    changed = TRUE;
    break;
  case PREF_NONE:
  case PREF_END_GROUP:
    break;
  }
  if (changed && data->update_function != NULL) {
    (data->update_function)(data, ptr);
  }
}

static void
prefs_boolean_toggle(GtkWidget *widget, gpointer data)
{
  gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(widget));
  gtk_button_set_label(GTK_BUTTON(widget), active ? _("Yes") : _("No"));
}

static GtkWidget *
prefs_get_property_widget(DiaPrefData *data)
{
  GtkWidget *widget = NULL;
  GtkAdjustment *adj;
  
  switch(data->type) {
  case PREF_BOOLEAN:
    widget = gtk_toggle_button_new_with_label (_("No"));
    g_signal_connect (G_OBJECT (widget), "toggled",
		      G_CALLBACK (prefs_boolean_toggle), NULL);
    break;
  case PREF_INT:
    adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0,
					    G_MININT, G_MAXINT,
					    1.0, 10.0, 0));
    widget = gtk_spin_button_new (adj, 1.0, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(widget), TRUE);
    gtk_widget_set_size_request (widget, 80, -1);
    break;
  case PREF_UINT:
    adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0,
					    0.0, G_MAXINT,
					    1.0, 10.0, 0));
    widget = gtk_spin_button_new (adj, 1.0, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(widget), TRUE);
    gtk_widget_set_size_request (widget, 80, -1);
    break;
  case PREF_REAL:
    adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0,
					    G_MINFLOAT, G_MAXFLOAT,
					    1.0, 10.0, 0));
    widget = gtk_spin_button_new (adj, 1.0, 3);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(widget), TRUE);
    gtk_widget_set_size_request (widget, 80, -1);
    break;
  case PREF_UREAL:
    adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0,
					    0.0, G_MAXFLOAT,
					    1.0, 10.0, 0 ));
    widget = gtk_spin_button_new (adj, 1.0, 3);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(widget), TRUE);
    gtk_widget_set_size_request (widget, 80, -1);
    break;
  case PREF_COLOUR:
    widget = dia_color_selector_new();
    break;
  case PREF_STRING:
    widget = gtk_entry_new();
    break;
  case PREF_CHOICE: {
    GList *names;
#if GTK_CHECK_VERSION(2,24,0)
    widget = gtk_combo_box_text_new ();
#else
    widget = gtk_combo_box_new_text ();
#endif
    for (names = (data->choice_list_function)(data); 
         names != NULL;
	 names = g_list_next(names)) {
#if GTK_CHECK_VERSION(2,24,0)
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (widget), (gchar *)names->data);
#else
      gtk_combo_box_append_text (GTK_COMBO_BOX (widget), (gchar *)names->data);
#endif
    }
    break;
  }
  case PREF_NONE:
  case PREF_END_GROUP:
    widget = NULL;
    break;
  }
  if (widget != NULL)
    gtk_widget_show(widget);
  return widget;
}

static gint
prefs_respond(GtkWidget *widget, 
                   gint       response_id,
                   gpointer   data)
{
  if (   response_id == GTK_RESPONSE_APPLY 
      || response_id == GTK_RESPONSE_OK) {
    prefs_update_prefs_from_dialog();
    prefs_save();
    diagram_redraw_all();
  }

  if (response_id != GTK_RESPONSE_APPLY)
    gtk_widget_hide(widget);

  return 0;
}

static void
prefs_create_dialog(void)
{
  GtkWidget *label;
  GtkWidget *dialog_vbox;
  GtkWidget *notebook;
  GtkTable *top_table = NULL; /* top level table for the tab */
  GtkTable *current_table = NULL;
  int i;
  int tab_idx = -1;

  if (prefs_dialog != NULL)
    return;

  prefs_dialog = gtk_dialog_new_with_buttons(
			_("Preferences"),
			GTK_WINDOW(interface_get_toolbox_shell()),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);
  gtk_dialog_set_default_response (GTK_DIALOG(prefs_dialog), GTK_RESPONSE_OK);
  gtk_window_set_resizable (GTK_WINDOW (prefs_dialog), TRUE);

  dialog_vbox = gtk_dialog_get_content_area (GTK_DIALOG (prefs_dialog));
  
  gtk_window_set_role (GTK_WINDOW (prefs_dialog), "preferences_window");

  g_signal_connect(G_OBJECT (prefs_dialog), "response",
                   G_CALLBACK (prefs_respond), NULL);

  g_signal_connect (G_OBJECT (prefs_dialog), "delete_event",
		    G_CALLBACK(gtk_widget_hide), NULL);
  g_signal_connect (G_OBJECT (prefs_dialog), "destroy",
		    G_CALLBACK(gtk_widget_destroyed), &prefs_dialog);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), notebook, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (notebook), 2);
  gtk_widget_show (notebook);

  for (i=0;i<NUM_PREFS_TABS;i++) {
    GtkWidget *table;
    GtkWidget *notebook_page;

    label = gtk_label_new(gettext(prefs_tabs[i].title));
    gtk_widget_show(label);

    table = gtk_table_new (9, 2, FALSE);
    prefs_tabs[i].table = GTK_TABLE(table);
    gtk_widget_set_size_request(table, -1, -1);
    gtk_widget_show(table);
    
#ifdef SCROLLED_PAGES
    notebook_page = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (notebook_page),
				    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_show(notebook_page);
#else
    notebook_page = table;
#endif/* SCROLLED_PAGES */

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), notebook_page, label);

#ifdef SCROLLED_PAGES
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(notebook_page),
					  table);
    gtk_viewport_set_shadow_type(GTK_VIEWPORT(gtk_bin_get_child(GTK_BIN(notebook_page))),
				 GTK_SHADOW_NONE);
#endif /* SCROLLED_PAGES */

  }
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_LEFT);

  tab_idx = -1;
  for (i=0;i<NUM_PREFS_DATA;i++) {
    GtkWidget *widget = NULL;
    int row;

    if (prefs_data[i].hidden) 
      continue;

    if (tab_idx != prefs_data[i].tab) {
      tab_idx = prefs_data[i].tab;
      top_table = prefs_tabs[prefs_data[i].tab].table;
      current_table = top_table;
    }
    row = prefs_tabs[tab_idx].row++;
    switch(prefs_data[i].type) {
    case PREF_NONE:
      widget = gtk_frame_new(gettext(prefs_data[i].label_text));
      gtk_widget_show (widget);
      gtk_table_attach (current_table, widget, 0, 2,
			row, row + 1,
			GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);
      current_table = GTK_TABLE(gtk_table_new (9, 2, FALSE));
      gtk_container_add(GTK_CONTAINER(widget), GTK_WIDGET(current_table));
      gtk_widget_show(GTK_WIDGET(current_table));
      break;
    case PREF_END_GROUP:
      current_table = top_table;
      break;
    case PREF_BOOLEAN:
      widget = gtk_check_button_new_with_label (gettext(prefs_data[i].label_text));
      gtk_widget_show (widget);
      gtk_table_attach (current_table, widget, 0, 2,
			row, row + 1,
			GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);
      break;
    default:
      label = gtk_label_new (gettext(prefs_data[i].label_text));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.3);
      gtk_widget_show (label);
      
      gtk_table_attach (current_table, label, 0, 1,
			row, row + 1,
			GTK_FILL | GTK_EXPAND, GTK_FILL, 1, 1);
      
      widget = prefs_get_property_widget(&prefs_data[i]);
      if (widget != NULL) {
	gtk_table_attach (current_table, widget, 1, 2,
			  row, row + 1,
			  GTK_FILL, GTK_FILL, 1, 1);
      }
      break;
    }
    prefs_data[i].widget = widget;
    
  }

  gtk_widget_show (prefs_dialog);
}

static void
prefs_update_prefs_from_dialog(void)
{
  GtkWidget *widget;
  int i;
  gpointer ptr;
  
  for (i=0;i<NUM_PREFS_DATA;i++) {
    if (prefs_data[i].hidden) continue;
    widget = prefs_data[i].widget;
    ptr = (char *)&prefs + prefs_data[i].offset;
    
    prefs_get_value_from_widget(widget, &prefs_data[i],  ptr);
  }
}

static void
prefs_update_dialog_from_prefs(void)
{
  GtkWidget *widget;
  int i;
  gpointer ptr;
  
  for (i=0;i<NUM_PREFS_DATA;i++) {
    if (prefs_data[i].hidden) continue;
    widget = prefs_data[i].widget;
    ptr = (char *)&prefs + prefs_data[i].offset;
    
    prefs_set_value_in_widget(widget, &prefs_data[i],  ptr);
  }
}

/** Updates certain preferences that are kept in lib.  Both args
 *  are currently unused and may be null.
 */
static void
update_internal_prefs(DiaPrefData *pref, gpointer ptr)
{
#if 0
  char *val = NULL;
  
  if (!ptr)
    return;
  val = *(char **)ptr;
#endif
  if (prefs.length_unit)
    prefs_set_length_unit(prefs.length_unit);
  if (prefs.fontsize_unit)
    prefs_set_fontsize_unit(prefs.fontsize_unit);
}

static void
update_floating_toolbox(DiaPrefData *pref, gpointer ptr)
{
  g_return_if_fail (pref->key == NULL);

  if (!app_is_interactive())
    return;

  if (prefs.toolbox_on_top) {
    /* Go through all diagrams and set toolbox transient for all displays */
    GList *diagrams;
    for (diagrams = dia_open_diagrams(); diagrams != NULL; 
	 diagrams = g_list_next(diagrams)) {
      Diagram *diagram = (Diagram *)diagrams->data;
      GSList *displays;
      for (displays = diagram->displays; displays != NULL; 
	   displays = g_slist_next(displays)) {
	DDisplay *ddisp = (DDisplay *)displays->data;
	gtk_window_set_transient_for(GTK_WINDOW(interface_get_toolbox_shell()),
				     GTK_WINDOW(ddisp->shell));
      }
    }
  } else {
    GtkWindow *shell = GTK_WINDOW(interface_get_toolbox_shell());
    if (shell)
      gtk_window_set_transient_for(shell, NULL);
  }
}

