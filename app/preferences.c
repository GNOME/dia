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

#include "persistence.h"

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
  void *default_value;
  int tab;
  char *label_text;
  GtkWidget *widget;
  gboolean hidden;
  GList *(*choice_list_function)();
  /** A function to call after a preference item has been updated. */
  void (*update_function)(struct _DiaPrefData *pref, char *ptr);
} DiaPrefData;

static void update_floating_toolbox(DiaPrefData *pref, char *ptr);

static int default_true = 1;
static int default_false = 0;
static int default_major_lines = 5;
static real default_real_one = 1.0;
static real default_real_zoom = 100.0;
static int default_int_w = 500;
static int default_int_h = 400;
static int default_undo_depth = 15;
static guint default_recent_documents = 5;
static Color default_colour = DEFAULT_GRID_COLOR;
static Color pbreak_colour = DEFAULT_PAGEBREAK_COLOR;
static guint default_dtree_dia_sort = DIA_TREE_SORT_INSERT;
static guint default_dtree_obj_sort = DIA_TREE_SORT_INSERT;
static const gchar *default_paper_name = NULL;

struct DiaPrefsTab {
  char *title;
  GtkTable *table;
  int row;
};

struct DiaPrefsTab prefs_tabs[] =
{
  {N_("User Interface"), NULL, 0},
  {N_("Diagram Defaults"), NULL, 0},
  {N_("View Defaults"), NULL, 0},
  {N_("Grid Lines"), NULL, 0},
  {N_("Diagram Tree"), NULL, 0},
};

#define NUM_PREFS_TABS (sizeof(prefs_tabs)/sizeof(struct DiaPrefsTab))

/* retrive a structure offset */
#ifdef offsetof
#define PREF_OFFSET(field)        ((int) offsetof (struct DiaPreferences, field))
#else /* !offsetof */
#define PREF_OFFSET(field)        ((int) ((char*) &((struct DiaPreferences *) 0)->field))
#endif /* !offsetof */

DiaPrefData prefs_data[] =
{
  { "reset_tools_after_create", PREF_BOOLEAN, PREF_OFFSET(reset_tools_after_create), &default_true, 0, N_("Reset tools after create") },
  { "compress_save", PREF_BOOLEAN, PREF_OFFSET(new_diagram.compress_save), &default_true, 0, N_("Compress saved files") },
  { "undo_depth", PREF_UINT, PREF_OFFSET(undo_depth), &default_undo_depth, 0, N_("Number of undo levels:") },
  { "reverse_rubberbanding_intersects", PREF_BOOLEAN, PREF_OFFSET(reverse_rubberbanding_intersects), &default_true, 0, N_("Reverse dragging selects\nintersecting objects") },
  { "recent_documents_list_size", PREF_UINT, PREF_OFFSET(recent_documents_list_size), &default_recent_documents, 0, N_("Recent documents list size:") },
  { "use_menu_bar", PREF_BOOLEAN, PREF_OFFSET(new_view.use_menu_bar), &default_true, 0, N_("Use menu bar") },
  { "toolbox_on_top", PREF_BOOLEAN, PREF_OFFSET(toolbox_on_top),
    &default_false, 0, N_("Keep tool box on top of diagram windows"),
    NULL, FALSE, NULL, update_floating_toolbox},
  
  { NULL, PREF_NONE, 0, NULL, 1, N_("New diagram:") },
  { "is_portrait", PREF_BOOLEAN, PREF_OFFSET(new_diagram.is_portrait), &default_true, 1, N_("Portrait") },
  { "new_diagram_papertype", PREF_CHOICE, PREF_OFFSET(new_diagram.papertype),
    &default_paper_name, 1, N_("Paper type:"), NULL, FALSE,
    get_paper_name_list },
  { "new_diagram_bgcolour", PREF_COLOUR, PREF_OFFSET(new_diagram.bg_color),
    &color_white, 1, N_("Background Color:") },
  { NULL, PREF_END_GROUP, 0, NULL, 1, NULL },

  { NULL, PREF_NONE, 0, NULL, 1, N_("New window:") },
  { "new_view_width", PREF_UINT, PREF_OFFSET(new_view.width), &default_int_w, 1, N_("Width:") },
  { "new_view_height", PREF_UINT, PREF_OFFSET(new_view.height), &default_int_h, 1, N_("Height:") },
  { "new_view_zoom", PREF_UREAL, PREF_OFFSET(new_view.zoom), &default_real_zoom, 1, N_("Magnify:") },
  { NULL, PREF_END_GROUP, 0, NULL, 1, NULL },

  { NULL, PREF_NONE, 0, NULL, 1, N_("Connection Points:") },
  { "show_cx_pts", PREF_BOOLEAN, PREF_OFFSET(show_cx_pts), &default_true, 1, N_("Visible") },
  { NULL, PREF_END_GROUP, 0, NULL, 1, NULL },

  { NULL, PREF_NONE, 0, NULL, 2, N_("Page breaks:") },
  { "pagebreak_visible", PREF_BOOLEAN, PREF_OFFSET(pagebreak.visible), &default_true, 2, N_("Visible") },
  { "pagebreak_colour", PREF_COLOUR, PREF_OFFSET(new_diagram.pagebreak_color), &pbreak_colour, 2, N_("Color:") },
  { "pagebreak_solid", PREF_BOOLEAN, PREF_OFFSET(pagebreak.solid), &default_true, 2, N_("Solid lines") },
  { NULL, PREF_END_GROUP, 0, NULL, 2, NULL },
  
  /*{ NULL, PREF_NONE, 0, NULL, 3, N_("Grid:") }, */
  { "grid_visible", PREF_BOOLEAN, PREF_OFFSET(grid.visible), &default_true, 3, N_("Visible") },
  { "grid_snap", PREF_BOOLEAN, PREF_OFFSET(grid.snap), &default_false, 3, N_("Snap to") },
  { "grid_dynamic", PREF_BOOLEAN, PREF_OFFSET(grid.dynamic), &default_true, 3, N_("Dynamic grid resizing") },
  { "grid_x", PREF_UREAL, PREF_OFFSET(grid.x), &default_real_one, 3, N_("X Size:") },
  { "grid_y", PREF_UREAL, PREF_OFFSET(grid.y), &default_real_one, 3, N_("Y Size:") },
  { "grid_colour", PREF_COLOUR, PREF_OFFSET(new_diagram.grid_color), &default_colour, 3, N_("Color:") },
  { "grid_major", PREF_UINT, PREF_OFFSET(grid.major_lines), &default_major_lines, 3, N_("Lines per major line") },
  { "grid_hex", PREF_BOOLEAN, PREF_OFFSET(grid.hex), &default_false, 3, N_("Hex grid") },
  { "grid_w", PREF_UREAL, PREF_OFFSET(grid.w), &default_real_one, 3, N_("Hex Size:")
 },
  /*  { "grid_solid", PREF_BOOLEAN, PREF_OFFSET(grid.solid), &default_true, 3, N_("Solid lines:") },  */

  { "render_bounding_boxes", PREF_BOOLEAN,PREF_OFFSET(render_bounding_boxes),
    &default_false,0,"render bounding boxes",NULL, TRUE},

  /* There's really no reason to not pertty format it, and allowing non-pretty
     can lead to problems with long lines, CVS etc. 
  { "pretty_formated_xml", PREF_BOOLEAN,PREF_OFFSET(pretty_formated_xml),
    &default_true,0,"pretty formated xml",NULL, TRUE},
  */

  { "prefer_psprint", PREF_BOOLEAN,PREF_OFFSET(prefer_psprint),
    &default_false,0,"prefer psprint", NULL, TRUE},

  { NULL, PREF_NONE, 0, NULL, 4, N_("Diagram tree window:") },
  { "diagram_tree_save_hidden", PREF_BOOLEAN, PREF_OFFSET(dia_tree.save_hidden),
    &default_false, 4, N_("Save hidden object types")},
  { "diagram_tree_dia_sort", PREF_UINT, PREF_OFFSET(dia_tree.dia_sort),
    &default_dtree_dia_sort, 4, "default diagram sort order", NULL, TRUE},
  { "diagram_tree_obj_sort", PREF_UINT, PREF_OFFSET(dia_tree.obj_sort),
    &default_dtree_obj_sort, 4, "default object sort order", NULL, TRUE},
  { NULL, PREF_END_GROUP, 0, NULL, 4, NULL },
};

#define NUM_PREFS_DATA (sizeof(prefs_data)/sizeof(DiaPrefData))

static void prefs_create_dialog(void);
static void prefs_set_value_in_widget(GtkWidget * widget, DiaPrefData *data,  char *ptr);
static void prefs_get_value_from_widget(GtkWidget * widget, DiaPrefData *data, char *ptr);
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
  char *ptr;

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
  }
}

void
prefs_save(void)
{
  int i;
  char *ptr;
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

  render_bounding_boxes = prefs.render_bounding_boxes;
}

static void
prefs_set_value_in_widget(GtkWidget * widget, DiaPrefData *data,
			  char *ptr)
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
    GList *names = (data->choice_list_function)();
    int index;
    for (index = 0; names != NULL; names = g_list_next(names), index++) {
      if (!strcmp(*((gchar**)ptr), (gchar *)names->data))
	break;
    }
    if (names == NULL) return;
    gtk_option_menu_set_history(GTK_OPTION_MENU(widget), index);
    gtk_menu_set_active(GTK_MENU(gtk_option_menu_get_menu(GTK_OPTION_MENU(widget))), index);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_menu_get_active(GTK_MENU(gtk_option_menu_get_menu(GTK_OPTION_MENU(widget))))), TRUE);
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
			    char *ptr)
{
  switch(data->type) {
  case PREF_BOOLEAN:
    *((int *)ptr) = GTK_TOGGLE_BUTTON(widget)->active;    
    break;
  case PREF_INT:
  case PREF_UINT:
    *((int *)ptr) = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
    break;
  case PREF_REAL:
  case PREF_UREAL:
    *((real *)ptr) = (real)
      gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(widget));
    break;
  case PREF_COLOUR:
    dia_color_selector_get_color(widget, (Color *)ptr);
    break;
  case PREF_CHOICE: {
    int index = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
    GList *names = (data->choice_list_function)();
    *((gchar **)ptr) = g_strdup((gchar *)g_list_nth_data(names, index));
    break;
  }
  case PREF_STRING:
    *((gchar **)ptr) = (gchar *)gtk_entry_get_text(GTK_ENTRY(widget));
    break;
  case PREF_NONE:
  case PREF_END_GROUP:
    break;
  }
  if (data->update_function != NULL) {
    (data->update_function)(data, ptr);
  }
}

static void
prefs_boolean_toggle(GtkWidget *widget, gpointer data)
{
  guint active = GTK_TOGGLE_BUTTON(widget)->active;
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
    g_signal_connect (GTK_OBJECT (widget), "toggled",
		      G_CALLBACK (prefs_boolean_toggle), NULL);
    break;
  case PREF_INT:
    adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0,
					    G_MININT, G_MAXINT,
					    1.0, 10.0, 10.0 ));
    widget = gtk_spin_button_new (adj, 1.0, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(widget), TRUE);
    gtk_widget_set_usize(widget, 80, -1);
    break;
  case PREF_UINT:
    adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0,
					    0.0, G_MAXINT,
					    1.0, 10.0, 10.0 ));
    widget = gtk_spin_button_new (adj, 1.0, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(widget), TRUE);
    gtk_widget_set_usize(widget, 80, -1);
    break;
  case PREF_REAL:
    adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0,
					    G_MINFLOAT, G_MAXFLOAT,
					    1.0, 10.0, 10.0 ));
    widget = gtk_spin_button_new (adj, 1.0, 3);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(widget), TRUE);
    gtk_widget_set_usize(widget, 80, -1);
    break;
  case PREF_UREAL:
    adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0,
					    0.0, G_MAXFLOAT,
					    1.0, 10.0, 10.0 ));
    widget = gtk_spin_button_new (adj, 1.0, 3);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(widget), TRUE);
    gtk_widget_set_usize(widget, 80, -1);
    break;
  case PREF_COLOUR:
    widget = dia_color_selector_new();
    break;
  case PREF_STRING:
    widget = gtk_entry_new();
    break;
  case PREF_CHOICE: {
    GtkWidget *menu;
    GList *names;
    GSList *group = NULL;
    widget = gtk_option_menu_new();
    menu = gtk_menu_new();
    for (names = (data->choice_list_function)(); names != NULL;
	 names = g_list_next(names)) {
      GtkWidget *menuitem =
	gtk_radio_menu_item_new_with_label(group, (gchar *)names->data);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
      group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(widget), menu);
    gtk_widget_show_all(menu);
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

  dialog_vbox = GTK_DIALOG (prefs_dialog)->vbox;
  
  gtk_window_set_role (GTK_WINDOW (prefs_dialog), "preferences_window");

  g_signal_connect(G_OBJECT (prefs_dialog), "response",
                   G_CALLBACK (prefs_respond), NULL);

  g_signal_connect (GTK_OBJECT (prefs_dialog), "delete_event",
		    G_CALLBACK(gtk_widget_hide), NULL);
  g_signal_connect (GTK_OBJECT (prefs_dialog), "destroy",
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
    gtk_viewport_set_shadow_type(GTK_VIEWPORT(GTK_BIN(notebook_page)->child),
				 GTK_SHADOW_NONE);
#endif /* SCROLLED_PAGES */

  }

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
  char *ptr;
  
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
  char *ptr;
  
  for (i=0;i<NUM_PREFS_DATA;i++) {
    if (prefs_data[i].hidden) continue;
    widget = prefs_data[i].widget;
    ptr = (char *)&prefs + prefs_data[i].offset;
    
    prefs_set_value_in_widget(widget, &prefs_data[i],  ptr);
  }
}

static void
update_floating_toolbox(DiaPrefData *pref, char *ptr)
{
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
    gtk_window_set_transient_for(GTK_WINDOW(interface_get_toolbox_shell()),
				 NULL);
  }
}
