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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include "config.h"
#include "intl.h"
#include "widgets.h"
#include "preferences.h"

struct DiaPreferences prefs;

enum DiaPrefType {
  PREF_NONE,
  PREF_BOOLEAN,
  PREF_INT,
  PREF_UINT,
  PREF_REAL,
  PREF_UREAL,
  PREF_COLOUR
};

struct DiaPrefsData {
  char *name;
  enum DiaPrefType type;
  int offset;
  void *default_value;
  int tab;
  char *label_text;
  GtkWidget *widget;
};

static int default_true = 1;
static int default_false = 0;
static real default_real_one = 1.0;
static real default_real_zoom = 100.0;
static int default_int_w = 500;
static int default_int_h = 400;
static int default_undo_depth = 15;
static Color default_colour = { 0.5, 0.5, 0.5 };

struct DiaPrefsTab {
  char *title;
  GtkTable *table;
  int row;
};

struct DiaPrefsTab prefs_tabs[] =
{
  {N_("User Interface"), NULL, 0},
  {N_("View Defaults"), NULL, 0},
};

#define NUM_PREFS_TABS (sizeof(prefs_tabs)/sizeof(struct DiaPrefsTab))

struct DiaPrefsData prefs_data[] =
{
  { "reset_tools_after_create", PREF_BOOLEAN, PREF_OFFSET(reset_tools_after_create), &default_false, 0, N_("Reset tools after create:") },
  { "compress_save", PREF_BOOLEAN, PREF_OFFSET(compress_save), &default_true, 0, N_("Compress saved files:") },
  { "undo_depth", PREF_UINT, PREF_OFFSET(undo_depth), &default_undo_depth, 0, N_("Number of undo levels:") },

  { NULL, PREF_NONE, 0, NULL, 1, N_("Grid:") },
  { "grid_visible", PREF_BOOLEAN, PREF_OFFSET(grid.visible), &default_true, 1, N_("Visible:") },
  { "grid_snap", PREF_BOOLEAN, PREF_OFFSET(grid.snap), &default_false, 1, N_("Snap to:") },
  { "grid_x", PREF_UREAL, PREF_OFFSET(grid.x), &default_real_one, 1, N_("X Size:") },
  { "grid_y", PREF_UREAL, PREF_OFFSET(grid.y), &default_real_one, 1, N_("Y Size:") },
  { "grid_colour", PREF_COLOUR, PREF_OFFSET(grid.colour), &default_colour, 1, N_("Colour") },
  { "grid_solid", PREF_BOOLEAN, PREF_OFFSET(grid.solid), &default_true, 1, N_("Solid lines") },
  
  { NULL, PREF_NONE, 0, NULL, 1, N_("New window:") },
  { "new_view_width", PREF_UINT, PREF_OFFSET(new_view.width), &default_int_w, 1, N_("Width:") },
  { "new_view_height", PREF_UINT, PREF_OFFSET(new_view.height), &default_int_h, 1, N_("Height:") },
  { "new_view_zoom", PREF_UREAL, PREF_OFFSET(new_view.zoom), &default_real_zoom, 1, N_("Magnify:") },

  { NULL, PREF_NONE, 0, NULL, 1, N_("Connection Points:") },
  { "show_cx_pts", PREF_BOOLEAN, PREF_OFFSET(show_cx_pts), &default_true, 1, N_("Visible:") },

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
  char filename[512];
  FILE *file;

  strncpy(filename, getenv("HOME"), 512);
  filename[511] = 0;
  strncat(filename, "/.dia/diarc",512);
  filename[511] = 0;
  
  file = fopen(filename, "wt");
  
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
    case PREF_NONE:
      break;
    }
  }
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
  case PREF_NONE:
    break;
  }

  return G_TOKEN_NONE;
}


void
prefs_load(void)
{
  int i;
  char filename[512];
  int fd;
  GScanner *scanner;
  guint expected_token;

  strncpy(filename, getenv("HOME"), 512);
  filename[511] = 0;
  strncat(filename, "/.dia/diarc",512);
  filename[511] = 0;
  
  fd = open(filename, O_RDONLY);

  if (fd < 0) {
    strncpy(filename, getenv("HOME"), 512);
    filename[511] = 0;
    strncat(filename, "/.diarc",512);
    filename[511] = 0;
    fd = open(filename, O_RDONLY);
  }

  prefs_set_defaults();

  if (fd < 0) {
    return;
  }

  scanner = g_scanner_new ((GScannerConfig *) &dia_prefs_scanner_config);
 
  g_scanner_input_file (scanner, fd);

  scanner->input_name = filename;

  g_scanner_freeze_symbol_table(scanner);
  for (i = 0; i < NUM_PREFS_DATA; i++)
    if (prefs_data[i].type != PREF_NONE) {
      g_scanner_add_symbol(scanner, prefs_data[i].name,
			   GINT_TO_POINTER(i));
    }
  g_scanner_thaw_symbol_table(scanner);
  
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
  GtkWidget *button;
  GtkWidget *dialog_vbox;
  GtkWidget *notebook;
  GtkWidget *table;
  int i;

  if (prefs_dialog != NULL)
    return;

  prefs_dialog = gtk_dialog_new();
  
  gtk_window_set_title (GTK_WINDOW (prefs_dialog), _("Preferences"));
  gtk_window_set_wmclass (GTK_WINDOW (prefs_dialog),
			  "preferences_window", "Dia");
  gtk_window_set_policy (GTK_WINDOW (prefs_dialog),
			 FALSE, TRUE, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (prefs_dialog), 2);

  dialog_vbox = GTK_DIALOG (prefs_dialog)->vbox;

  button = gtk_button_new_with_label( _("OK") );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC(prefs_okay),
			     GTK_OBJECT(prefs_dialog));
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label( _("Apply") );
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(prefs_apply),
		      NULL);
  gtk_widget_grab_default (button);
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

  gtk_signal_connect (GTK_OBJECT (prefs_dialog), "delete_event",
		      GTK_SIGNAL_FUNC(gtk_widget_hide), NULL);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs_dialog)->vbox),
		      notebook, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (notebook), 2);
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
    widget = prefs_data[i].widget;
    ptr = (char *)&prefs + prefs_data[i].offset;
    
    prefs_set_value_in_widget(widget, prefs_data[i].type,  ptr);
  }
}





