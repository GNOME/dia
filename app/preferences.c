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
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif
#include <locale.h>

#ifdef GNOME
#  include <gnome.h>
#else
#  include <gtk/gtk.h>
#endif

#include "intl.h"
#include "widgets.h"
#include "diagram.h"
#include "message.h"
#include "preferences.h"
#include "dia_dirs.h"
#include "diagramdata.h"
#ifdef PREF_CHOICE
#include "paper.h"
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
#ifdef PREF_CHOICE
  PREF_CHOICE,
#endif
  PREF_STRING
};

struct DiaPrefsData {
  char *name;
  enum DiaPrefType type;
  int offset;
  void *default_value;
  int tab;
  char *label_text;
  GtkWidget *widget;
  gboolean hidden;
#ifdef PREF_CHOICE
  char *(*choice_list_function)();
#endif
};

static int default_true = 1;
static int default_false = 0;
static real default_real_one = 1.0;
static real default_real_zoom = 100.0;
static int default_int_w = 500;
static int default_int_h = 400;
static int default_undo_depth = 15;
static guint default_recent_documents = 5;
static Color default_colour = { 0.85, .90, .90 }; /* Grid colour */
static Color pbreak_colour = { 0.0, 0.0, 0.6 }; 
static guint default_dtree_width = 220;
static guint default_dtree_height = 100;
static guint default_dtree_dia_sort = DIA_TREE_SORT_INSERT;
static guint default_dtree_obj_sort = DIA_TREE_SORT_INSERT;

#ifdef PREF_CHOICE
static PaperInfo default_paper =
{ "A4", 2.82, 2.82, 2.82, 2.82, TRUE, 100.0, FALSE, 1, 1 };
/*
  static gboolean default_is_portrait = FALSE;
  static gboolean default_fitto = FALSE;
  static gfloat default_scaling = 100.0;
  static gint default_fitwidth = 1;
  static gint default_fitheight = 1;
  static gfloat default_tmargin = 2.82;
  static gfloat default_bmargin = 2.82;
  static gfloat default_lmargin = 2.82;
  static gfloat default_rmargin = 2.82;
  static gchar *default_papertype = "A4";
*/
#endif

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

struct DiaPrefsData prefs_data[] =
{
  { "reset_tools_after_create", PREF_BOOLEAN, PREF_OFFSET(reset_tools_after_create), &default_true, 0, N_("Reset tools after create:") },
  { "compress_save", PREF_BOOLEAN, PREF_OFFSET(compress_save), &default_true, 0, N_("Compress saved files:") },
  { "undo_depth", PREF_UINT, PREF_OFFSET(undo_depth), &default_undo_depth, 0, N_("Number of undo levels:") },
  { "reverse_rubberbanding_intersects", PREF_BOOLEAN, PREF_OFFSET(reverse_rubberbanding_intersects), &default_true, 0, N_("Reverse dragging selects\nintersecting objects:") },
  { "recent_documents_list_size", PREF_UINT, PREF_OFFSET(recent_documents_list_size), &default_recent_documents, 0, N_("Recent documents list size:") },

#ifdef PREF_CHOICE
  { NULL, PREF_NONE, 0, NULL, 1, N_("New diagram:") },
  { "new_diagram_papertype", PREF_CHOICE, PREF_OFFSET(new_paper.name), &default_paper.name, 1, N_("Paper type:"), NULL, FALSE, get_paper_name_list },
#endif

  { NULL, PREF_NONE, 0, NULL, 1, N_("New window:") },
  { "new_view_width", PREF_UINT, PREF_OFFSET(new_view.width), &default_int_w, 1, N_("Width:") },
  { "new_view_height", PREF_UINT, PREF_OFFSET(new_view.height), &default_int_h, 1, N_("Height:") },
  { "new_view_zoom", PREF_UREAL, PREF_OFFSET(new_view.zoom), &default_real_zoom, 1, N_("Magnify:") },
  { "use_menu_bar", PREF_BOOLEAN, PREF_OFFSET(new_view.use_menu_bar), &default_false, 0, N_("Use menu bar:") },

  { NULL, PREF_NONE, 0, NULL, 1, N_("Connection Points:") },
  { "show_cx_pts", PREF_BOOLEAN, PREF_OFFSET(show_cx_pts), &default_true, 1, N_("Visible:") },

  { NULL, PREF_NONE, 0, NULL, 2, N_("Page breaks:") },
  { "pagebreak_visible", PREF_BOOLEAN, PREF_OFFSET(pagebreak.visible), &default_true, 2, N_("Visible:") },
  { "pagebreak_colour", PREF_COLOUR, PREF_OFFSET(pagebreak.colour), &pbreak_colour, 2, N_("Colour:") },
  { "pagebreak_solid", PREF_BOOLEAN, PREF_OFFSET(pagebreak.solid), &default_true, 2, N_("Solid lines:") },
  
  /*{ NULL, PREF_NONE, 0, NULL, 3, N_("Grid:") }, */
  { "grid_visible", PREF_BOOLEAN, PREF_OFFSET(grid.visible), &default_true, 3, N_("Visible:") },
  { "grid_snap", PREF_BOOLEAN, PREF_OFFSET(grid.snap), &default_false, 3, N_("Snap to:") },
  { "grid_x", PREF_UREAL, PREF_OFFSET(grid.x), &default_real_one, 3, N_("X Size:") },
  { "grid_y", PREF_UREAL, PREF_OFFSET(grid.y), &default_real_one, 3, N_("Y Size:") },
  { "grid_colour", PREF_COLOUR, PREF_OFFSET(grid.colour), &default_colour, 3, N_("Colour:") },
  { "grid_solid", PREF_BOOLEAN, PREF_OFFSET(grid.solid), &default_true, 3, N_("Solid lines:") },  

  { "render_bounding_boxes", PREF_BOOLEAN,PREF_OFFSET(render_bounding_boxes),
    &default_false,0,"render bounding boxes:",NULL, TRUE},

  { "pretty_formated_xml", PREF_BOOLEAN,PREF_OFFSET(pretty_formated_xml),
    &default_false,0,"pretty formated xml:",NULL, TRUE},

  { "prefer_psprint", PREF_BOOLEAN,PREF_OFFSET(prefer_psprint),
    &default_false,0,"prefer psprint:", NULL, TRUE},

  { NULL, PREF_NONE, 0, NULL, 4, N_("Diagram tree window:") },
  { "show_diagram_tree", PREF_BOOLEAN, PREF_OFFSET(dia_tree.show_tree),
    &default_false, 4, N_("Show at startup:")},
  { "diagram_tree_width", PREF_UINT, PREF_OFFSET(dia_tree.width),
    &default_dtree_width, 4, N_("Default width:")},
  { "diagram_tree_height", PREF_UINT, PREF_OFFSET(dia_tree.height),
    &default_dtree_height, 4, N_("Default height:")},
  { "diagram_tree_save_size", PREF_BOOLEAN, PREF_OFFSET(dia_tree.save_size),
    &default_false, 4, N_("Remember last size:")},
  { "diagram_tree_save_hidden", PREF_BOOLEAN, PREF_OFFSET(dia_tree.save_hidden),
    &default_false, 4, N_("Save hidden object types:")},
  { "diagram_tree_dia_sort", PREF_UINT, PREF_OFFSET(dia_tree.dia_sort),
    &default_dtree_dia_sort, 4, "default diagram sort order", NULL, TRUE},
  { "diagram_tree_obj_sort", PREF_UINT, PREF_OFFSET(dia_tree.obj_sort),
    &default_dtree_obj_sort, 4, "default object sort order", NULL, TRUE},
  { "diagram_tree_hidden", PREF_STRING, PREF_OFFSET(dia_tree.hidden),
    &DIA_TREE_DEFAULT_HIDDEN, 4, "hidden type list", NULL, TRUE},
};

#define NUM_PREFS_DATA (sizeof(prefs_data)/sizeof(struct DiaPrefsData))

static const GScannerConfig     dia_prefs_scanner_config =
{
  (
   " \t\n"
   )                    /* cset_skip_characters */,
  (
   G_CSET_a_2_z
   "_"
   G_CSET_A_2_Z
   )                    /* cset_identifier_first */,
  (
   G_CSET_a_2_z
   "_-0123456789"
   G_CSET_A_2_Z
   )                    /* cset_identifier_nth */,
  ( "#\n" )             /* cpair_comment_single */,
  
  TRUE                  /* case_sensitive */,
  
  FALSE                 /* skip_comment_multi */,
  TRUE                  /* skip_comment_single */,
  FALSE                 /* scan_comment_multi */,
  TRUE                  /* scan_identifier */,
  TRUE                  /* scan_identifier_1char */,
  FALSE                 /* scan_identifier_NULL */,
  TRUE                  /* scan_symbols */,
  FALSE                 /* scan_binary */,
  FALSE                 /* scan_octal */,
  TRUE                  /* scan_float */,
  TRUE                  /* scan_hex */,
  FALSE                 /* scan_hex_dollar */,
  FALSE                 /* scan_string_sq */,
  TRUE                  /* scan_string_dq */,
  TRUE                  /* numbers_2_int */,
  FALSE                 /* int_2_float */,
  FALSE                 /* identifier_2_string */,
  TRUE                  /* char_2_token */,
  FALSE                 /* symbol_2_token */,
  FALSE                 /* scope_0_fallback */,
};

typedef enum {
  DIA_PREFS_TOKEN_BOOLEAN = G_TOKEN_LAST
} DiaPrefsTokenType;

static void prefs_create_dialog(void);
static void prefs_set_value_in_widget(GtkWidget * widget, enum DiaPrefType type,  char *ptr);
static void prefs_get_value_from_widget(GtkWidget * widget, enum DiaPrefType type, char *ptr);
static void prefs_update_dialog_from_prefs(void);
static void prefs_update_prefs_from_dialog(void);
static gint prefs_apply(GtkWidget *widget, gpointer data);


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

  for (i=0;i<NUM_PREFS_DATA;i++) {
    ptr = (char *)&prefs + prefs_data[i].offset;

    switch (prefs_data[i].type) {
    case PREF_BOOLEAN:
    case PREF_INT:
    case PREF_UINT:
      *(int *)ptr = *(int *)prefs_data[i].default_value;
      break;
    case PREF_REAL:
    case PREF_UREAL:
      *(real *)ptr = *(real *)prefs_data[i].default_value;
      break;
    case PREF_COLOUR:
      *(Color *)ptr = *(Color *)prefs_data[i].default_value;
      break;
#ifdef PREF_CHOICE
    case PREF_CHOICE:
#endif
    case PREF_STRING:
      *(gchar **)ptr = *(gchar **)prefs_data[i].default_value;
      break;
    case PREF_NONE:
      break;
    }
  }
}

void
prefs_save(void)
{
  int i;
  char *ptr;
  gchar *filename;
  char *old_locale;
  FILE *file;

  filename = dia_config_filename("diarc");

  file = fopen(filename, "w");

  if (!file) {
    message_error(_("Could not open `%s' for writing"), filename);
    g_free(filename);
    return;
  }
  
  g_free(filename);

  old_locale = setlocale(LC_NUMERIC, "C");
  fprintf(file, "# Note: This file is automatically generated by Dia\n");
  for (i=0;i<NUM_PREFS_DATA;i++) {
    if (prefs_data[i].type == PREF_NONE)
      continue;
    
    fprintf(file, "%s=", prefs_data[i].name);
    
    ptr = (char *)&prefs + prefs_data[i].offset;

    switch (prefs_data[i].type) {
    case PREF_BOOLEAN:
      fprintf(file, (*(int *)ptr)?"true\n":"false\n");
      break;
    case PREF_INT:
    case PREF_UINT:
      fprintf(file, "%d\n", *(int *)ptr);
      break;
    case PREF_REAL:
    case PREF_UREAL:
      
      fprintf(file, "%f\n", (double) *(real *)ptr);
      break;
    case PREF_COLOUR:
      fprintf(file, "%f %f %f\n", (double) ((Color *)ptr)->red,
	      (double) ((Color *)ptr)->green, (double) ((Color *)ptr)->blue);
      break;
#ifdef PREF_CHOICE
    case PREF_CHOICE:
#endif
    case PREF_STRING:
      fprintf(file, "\"%s\"\n", *(gchar **)ptr);
      break;
    case PREF_NONE:
      break;
    }
  }
  setlocale(LC_NUMERIC, old_locale);
  fclose(file);
}


static guint
prefs_parse_line(GScanner *scanner)
{
  guint token;
  int symbol_nr;
  char *ptr;
  
  token = g_scanner_get_next_token(scanner);
  if (token != G_TOKEN_SYMBOL)
    return G_TOKEN_SYMBOL;

  symbol_nr = GPOINTER_TO_INT(scanner->value.v_symbol);
  
  token = g_scanner_get_next_token(scanner);
  if (token != G_TOKEN_EQUAL_SIGN)
    return G_TOKEN_EQUAL_SIGN;

  token = g_scanner_get_next_token(scanner);

  ptr = (unsigned char *)&prefs + prefs_data[symbol_nr].offset;
  
  switch (prefs_data[symbol_nr].type) {
  case PREF_BOOLEAN:
    if (token != G_TOKEN_IDENTIFIER)
      return G_TOKEN_IDENTIFIER;
    
    if (strcmp(scanner->value.v_string, "true")==0)
      *(int *)ptr = 1;
    else
    *(int *)ptr = 0;
    break;
    
  case PREF_INT:
  case PREF_UINT:
    if (token != G_TOKEN_INT)
      return G_TOKEN_INT;
    
    *(int *)ptr = scanner->value.v_int;
    break;

  case PREF_REAL:
  case PREF_UREAL:
    if (token != G_TOKEN_FLOAT)
      return G_TOKEN_FLOAT;
    
    *(real *)ptr = scanner->value.v_float;
    break;
  case PREF_COLOUR:
    if (token != G_TOKEN_FLOAT)
      return G_TOKEN_FLOAT;
    ((Color *)ptr)->red = scanner->value.v_float;

    token = g_scanner_get_next_token(scanner);
    if (token != G_TOKEN_FLOAT)
      return G_TOKEN_FLOAT;
    ((Color *)ptr)->green = scanner->value.v_float;

    token = g_scanner_get_next_token(scanner);
    if (token != G_TOKEN_FLOAT)
      return G_TOKEN_FLOAT;
    ((Color *)ptr)->blue = scanner->value.v_float;

    break;
#ifdef PREF_CHOICE
  case PREF_CHOICE:
#endif
  case PREF_STRING:
    if (token != G_TOKEN_STRING)
      return G_TOKEN_STRING;

    *(char **)ptr = g_strdup(scanner->value.v_string);
    break;
  case PREF_NONE:
    break;
  }

  return G_TOKEN_NONE;
}


void
prefs_load(void)
{
  int i;
  gchar *filename;
  int fd;
  GScanner *scanner;
  guint expected_token;

  filename = dia_config_filename("diarc");

  fd = open(filename, O_RDONLY);

  if (fd < 0) {
    char *homedir = g_get_home_dir();

    g_free(filename);
    filename = g_strconcat(homedir, G_DIR_SEPARATOR_S ".diarc", NULL);
    fd = open(filename, O_RDONLY);
  }
  g_free(filename);

  prefs_set_defaults();

  if (fd < 0) {
    return;
  }

  scanner = g_scanner_new ((GScannerConfig *) &dia_prefs_scanner_config);
 
  g_scanner_input_file (scanner, fd);

  scanner->input_name = filename;
#if !GLIB_CHECK_VERSION (1,3,2)
  g_scanner_freeze_symbol_table(scanner);
#endif
  for (i = 0; i < NUM_PREFS_DATA; i++)
    if (prefs_data[i].type != PREF_NONE) {
      g_scanner_add_symbol(scanner, prefs_data[i].name,
			   GINT_TO_POINTER(i));
    }
#if !GLIB_CHECK_VERSION (1,3,2)
  g_scanner_thaw_symbol_table(scanner);
#endif  
  while (1) {
    if (g_scanner_peek_next_token(scanner) == G_TOKEN_EOF) {
      break;
    } 

    expected_token = prefs_parse_line(scanner);
      
    if (expected_token != G_TOKEN_NONE) {
      gchar *symbol_name;
      gchar *msg;
      
      msg = NULL;
      symbol_name = NULL;
      g_scanner_unexp_token (scanner,
			     expected_token,
			     NULL,
			     "keyword",
			     symbol_name,
			     msg,
			     TRUE);
    }
  }
  
  g_scanner_destroy (scanner);
 
  close(fd);
  render_bounding_boxes = prefs.render_bounding_boxes;
  pretty_formated_xml = prefs.pretty_formated_xml;
}

static gint
prefs_okay(GtkWidget *widget, gpointer data)
{
  gint ret = prefs_apply(widget,data);
  gtk_widget_hide(widget);
  return ret;
}

static gint
prefs_apply(GtkWidget *widget, gpointer data)
{
  prefs_update_prefs_from_dialog();
  prefs_save();
  diagram_redraw_all();
  return 1;
}

static void
prefs_set_value_in_widget(GtkWidget * widget, enum DiaPrefType type,
			  char *ptr)
{
  switch(type) {
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
    dia_color_selector_set_color(DIACOLORSELECTOR(widget), (Color *)ptr);
    break;
#ifdef PREF_CHOICE
  case PREF_CHOICE:
    gtk_
    break;
#endif
  case PREF_STRING:
    gtk_entry_set_text(GTK_ENTRY(widget), (gchar *)(*((gchar **)ptr)));
    break;
  case PREF_NONE:
    break;
  }
}

static void
prefs_get_value_from_widget(GtkWidget * widget, enum DiaPrefType type,
			    char *ptr)
{
  switch(type) {
  case PREF_BOOLEAN:
    *((int *)ptr) = GTK_TOGGLE_BUTTON(widget)->active;    
    break;
  case PREF_INT:
  case PREF_UINT:
    *((int *)ptr) =
      gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
    break;
  case PREF_REAL:
  case PREF_UREAL:
    *((real *)ptr) = (real)
      gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(widget));
    break;
  case PREF_COLOUR:
    dia_color_selector_get_color(DIACOLORSELECTOR(widget), (Color *)ptr);
    break;
#ifdef PREF_CHOICE
  case PREF_CHOICE:
    break;
#endif
  case PREF_STRING:
    *((gchar **)ptr) = (gchar *)
      gtk_entry_get_text(GTK_ENTRY(widget));
    break;
  case PREF_NONE:
    break;
  }
}

static void
prefs_boolean_toggle(GtkWidget *widget, gpointer data)
{
  guint active = GTK_TOGGLE_BUTTON(widget)->active;
  GtkWidget *label = GTK_BUTTON(widget)->child;
  gtk_label_set(GTK_LABEL(label), active ? _("Yes") : _("No"));
}

static GtkWidget *
prefs_get_property_widget(enum DiaPrefType type)
{
  GtkWidget *widget = NULL;
  GtkAdjustment *adj;
  
  switch(type) {
  case PREF_BOOLEAN:
    widget = gtk_toggle_button_new_with_label (_("No"));
    gtk_signal_connect (GTK_OBJECT (widget), "toggled",
			GTK_SIGNAL_FUNC (prefs_boolean_toggle), NULL);
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
  case PREF_NONE:
    widget = NULL;
    break;
  }
  if (widget != NULL)
    gtk_widget_show(widget);
  return widget;
}

static void
prefs_create_dialog(void)
{
  GtkWidget *notebook_page;
  GtkWidget *label;
#ifndef GNOME
  GtkWidget *button;
#endif
  GtkWidget *dialog_vbox;
  GtkWidget *notebook;
  GtkWidget *table;
  int i;

  if (prefs_dialog != NULL)
    return;

#ifdef GNOME
  prefs_dialog = gnome_dialog_new(_("Preferences"),
				  GNOME_STOCK_BUTTON_OK,
				  GNOME_STOCK_BUTTON_APPLY,
				  GNOME_STOCK_BUTTON_CLOSE, NULL);
  gnome_dialog_set_default(GNOME_DIALOG(prefs_dialog), 1);

  dialog_vbox = GNOME_DIALOG(prefs_dialog)->vbox;
#else
  prefs_dialog = gtk_dialog_new();
  gtk_window_set_title (GTK_WINDOW (prefs_dialog), _("Preferences"));
  gtk_container_set_border_width (GTK_CONTAINER (prefs_dialog), 2);
  gtk_window_set_policy (GTK_WINDOW (prefs_dialog),
			 FALSE, TRUE, FALSE);

  dialog_vbox = GTK_DIALOG (prefs_dialog)->vbox;
#endif
  
  gtk_window_set_wmclass (GTK_WINDOW (prefs_dialog),
			  "preferences_window", "Dia");


#ifdef GNOME
  gnome_dialog_button_connect_object(GNOME_DIALOG(prefs_dialog), 0,
				     GTK_SIGNAL_FUNC(prefs_okay),
				     GTK_OBJECT(prefs_dialog));
  gnome_dialog_button_connect_object(GNOME_DIALOG(prefs_dialog), 1,
				     GTK_SIGNAL_FUNC(prefs_apply),
				     GTK_OBJECT(prefs_dialog));
  gnome_dialog_button_connect_object(GNOME_DIALOG(prefs_dialog), 2,
				     GTK_SIGNAL_FUNC(gtk_widget_hide),
				     GTK_OBJECT(prefs_dialog));
#else
  button = gtk_button_new_with_label( _("OK") );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC(prefs_okay),
			     GTK_OBJECT(prefs_dialog));
  gtk_widget_show (button);

  button = gtk_button_new_with_label( _("Apply") );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(prefs_apply),
		      NULL);
  gtk_widget_show (button);

  button = gtk_button_new_with_label( _("Close") );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC(gtk_widget_hide),
			     GTK_OBJECT(prefs_dialog));
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
#endif

  gtk_signal_connect (GTK_OBJECT (prefs_dialog), "delete_event",
		      GTK_SIGNAL_FUNC(gtk_widget_hide), NULL);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (dialog_vbox), notebook, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (notebook), 2);
  gtk_widget_show (notebook);

  for (i=0;i<NUM_PREFS_TABS;i++) {
    label = gtk_label_new(gettext(prefs_tabs[i].title));
    gtk_widget_show(label);

    table = gtk_table_new (9, 2, FALSE);
    prefs_tabs[i].table = GTK_TABLE(table);
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

  for (i=0;i<NUM_PREFS_DATA;i++) {
    GtkTable *table;
    GtkWidget *widget;
    int row;

    if (prefs_data[i].hidden) continue;

    label = gtk_label_new (gettext(prefs_data[i].label_text));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.3);
    gtk_widget_show (label);

    table = prefs_tabs[prefs_data[i].tab].table;
    row = prefs_tabs[prefs_data[i].tab].row++;
    gtk_table_attach (table, label, 0, 1,
		      row, row + 1,
		      GTK_FILL, GTK_FILL, 1, 1);

    widget = prefs_get_property_widget(prefs_data[i].type);
    prefs_data[i].widget = widget;
    if (widget != NULL) {
      gtk_table_attach (table, widget, 1, 2,
			row, row + 1,
			GTK_EXPAND | GTK_FILL, GTK_FILL, 1, 1);
    }
    
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
    
    prefs_get_value_from_widget(widget, prefs_data[i].type,  ptr);
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
    
    prefs_set_value_in_widget(widget, prefs_data[i].type,  ptr);
  }
}





