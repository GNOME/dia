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

#include "config.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "dia-colour-selector.h"
#include "dia-colour-cell-renderer.h"
#include "persistence.h"

#define PERSIST_NAME "color-menu"

enum {
  SPECIAL_NOT,
  SPECIAL_SEPARATOR,
  SPECIAL_MORE,
  SPECIAL_RESET,
};

enum {
  COL_COLOUR,
  COL_TEXT,
  COL_SPECIAL,
  N_COL
};

struct _DiaColourSelector {
  GtkBox          hbox;

  gboolean        use_alpha;

  GtkWidget      *combo;
  GtkListStore   *colour_store;
  GtkTreeIter     colour_default_end;
  GtkTreeIter     colour_custom_end;
  GtkTreeIter     colour_other;
  GtkTreeIter     colour_reset;

  Color          *current;

  const Color    *looking_for;
  gboolean        found;

  GtkWidget      *dialog;
};

G_DEFINE_TYPE (DiaColourSelector, dia_colour_selector, GTK_TYPE_BOX)

enum {
  DIA_COLORSEL_VALUE_CHANGED,
  DIA_COLORSEL_LAST_SIGNAL
};
static guint dia_colorsel_signals[DIA_COLORSEL_LAST_SIGNAL] = { 0 };


static void
dia_colour_selector_finalize (GObject *object)
{
  DiaColourSelector *self = DIA_COLOUR_SELECTOR (object);

  g_clear_object (&self->colour_store);
  g_clear_pointer (&self->current, dia_colour_free);

  G_OBJECT_CLASS (dia_colour_selector_parent_class)->finalize (object);
}


static void
dia_colour_selector_class_init (DiaColourSelectorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dia_colour_selector_finalize;

  dia_colorsel_signals[DIA_COLORSEL_VALUE_CHANGED]
      = g_signal_new ("value_changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      0, NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
}


static void
add_colour (DiaColourSelector *cs, char *hex)
{
  GtkTreeIter iter;
  Color colour;

  dia_colour_parse (&colour, hex);
  gtk_list_store_append (cs->colour_store, &iter);
  gtk_list_store_set (cs->colour_store,
                      &iter,
                      COL_COLOUR, &colour,
                      COL_TEXT, hex,
                      COL_SPECIAL, SPECIAL_NOT,
                      -1);
}


static void
colour_response (GtkDialog *dialog, int response, gpointer user_data)
{
  DiaColourSelector *cs = DIA_COLOUR_SELECTOR (user_data);

  if (response == GTK_RESPONSE_OK) {
    GdkRGBA gcol;
    Color colour;

    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (cs->dialog), &gcol);

    GDK_COLOR_TO_DIA (gcol, colour);

    dia_colour_selector_set_colour (cs, &colour);
  } else {
    dia_colour_selector_set_colour (cs, cs->current);
  }

  gtk_widget_destroy (cs->dialog);
  cs->dialog = NULL;
}


static void
more_colours (DiaColourSelector *cs)
{
  GString *palette = g_string_new ("");
  GtkWidget *parent;
  GList *tmplist;
  GdkRGBA gdk_color;

  parent = gtk_widget_get_toplevel (GTK_WIDGET (cs));

  cs->dialog = gtk_color_chooser_dialog_new (_("Select color"), GTK_WINDOW(parent));

  color_convert (cs->current, &gdk_color);
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (cs->dialog), &gdk_color);

  /* avoid crashing if the property dialog is closed before the color dialog */
  if (parent) {
    gtk_window_set_transient_for (GTK_WINDOW (cs->dialog), GTK_WINDOW (parent));
    gtk_window_set_destroy_with_parent (GTK_WINDOW (cs->dialog), TRUE);
  }

  // Default colours
  g_string_append (palette, "#000000");
  g_string_append (palette, ":");
  g_string_append (palette, "#FFFFFF");
  g_string_append (palette, ":");
  g_string_append (palette, "#FF0000");
  g_string_append (palette, ":");
  g_string_append (palette, "#00FF00");
  g_string_append (palette, ":");
  g_string_append (palette, "#0000FF");
  g_string_append (palette, ":");

  for (tmplist = persistent_list_get_glist (PERSIST_NAME);
        tmplist != NULL;
        tmplist = g_list_next (tmplist)) {
    char *spec;
    Color colour;

    dia_colour_parse (&colour, tmplist->data);

    DIA_COLOR_TO_GDK (colour, gdk_color);

    spec = gdk_rgba_to_string (&gdk_color);

    g_string_append (palette, spec);
    g_string_append (palette, ":");

    g_clear_pointer (&spec, g_free);
  }

// Gtk3 disabled -- Hub
//  g_object_set (gtk_widget_get_settings (GTK_WIDGET (colorsel)),
//                "gtk-color-palette",
//                palette->str,
//                NULL);
//  gtk_color_selection_set_has_palette (GTK_COLOR_SELECTION (colorsel), TRUE);
//  g_string_free (palette, TRUE);

  g_signal_connect (G_OBJECT (cs->dialog),
                    "response",
                    G_CALLBACK (colour_response),
                    cs);

  gtk_widget_show (GTK_WIDGET (cs->dialog));
}


static void
changed (GtkComboBox *widget, gpointer user_data)
{
  DiaColourSelector *cs = DIA_COLOUR_SELECTOR (user_data);
  GtkTreeIter active;
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkTreePath *end_path;
  int special;

  gtk_combo_box_get_active_iter (widget, &active);

  gtk_tree_model_get (GTK_TREE_MODEL (cs->colour_store),
                      &active,
                      COL_SPECIAL, &special,
                      -1);

  switch (special) {
    case SPECIAL_NOT:
      g_clear_pointer (&cs->current, dia_colour_free);
      gtk_tree_model_get (GTK_TREE_MODEL (cs->colour_store),
                          &active,
                          COL_COLOUR, &cs->current,
                          -1);

      // Normal colour
      g_signal_emit (G_OBJECT (cs),
                     dia_colorsel_signals[DIA_COLORSEL_VALUE_CHANGED],
                     0);
      break;
    case SPECIAL_MORE:
      more_colours (cs);
      break;
    case SPECIAL_RESET:
      persistent_list_clear (PERSIST_NAME);

      path = gtk_tree_model_get_path (GTK_TREE_MODEL (cs->colour_store), &cs->colour_default_end);

      // Move over the separator
      gtk_tree_path_next (path);
      gtk_tree_model_get_iter (GTK_TREE_MODEL (cs->colour_store), &iter, path);

      end_path = gtk_tree_model_get_path (GTK_TREE_MODEL (cs->colour_store),
                                          &cs->colour_custom_end);

      while (gtk_tree_path_compare (path, end_path) != 0) {
        gtk_list_store_remove (cs->colour_store, &iter);

        gtk_tree_model_get_iter (GTK_TREE_MODEL (cs->colour_store), &iter, path);

        gtk_tree_path_free (end_path);
        end_path = gtk_tree_model_get_path (GTK_TREE_MODEL (cs->colour_store),
                                            &cs->colour_custom_end);
      }

      gtk_tree_path_free (path);
      gtk_tree_path_free (end_path);

      if (cs->current) {
        dia_colour_selector_set_colour (cs, cs->current);
      } else {
        gtk_tree_model_get_iter_first (GTK_TREE_MODEL (cs->colour_store), &iter);
        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (cs->combo), &iter);
      }

      return;
    case SPECIAL_SEPARATOR:
    default:
      g_return_if_reached ();
  }
}


static gboolean
is_separator (GtkTreeModel *model,
              GtkTreeIter  *iter,
              gpointer      data)
{
  int result;

  gtk_tree_model_get (model, iter, COL_SPECIAL, &result, -1);

  return result == SPECIAL_SEPARATOR;
}


static void
dia_colour_selector_init (DiaColourSelector *cs)
{
  GtkCellRenderer *renderer;
  GList *tmplist;

  cs->colour_store = gtk_list_store_new (N_COL,
                                         DIA_TYPE_COLOUR,
                                         G_TYPE_STRING,
                                         G_TYPE_INT);

  add_colour (cs, "#000000");
  add_colour (cs, "#FFFFFF");
  add_colour (cs, "#FF0000");
  add_colour (cs, "#00FF00");
  add_colour (cs, "#0000FF");

  gtk_list_store_append (cs->colour_store, &cs->colour_default_end);
  gtk_list_store_set (cs->colour_store,
                      &cs->colour_default_end,
                      COL_COLOUR, NULL,
                      COL_TEXT, NULL,
                      COL_SPECIAL, SPECIAL_SEPARATOR,
                      -1);

  persistence_register_list (PERSIST_NAME);

  for (tmplist = persistent_list_get_glist (PERSIST_NAME);
       tmplist != NULL; tmplist = g_list_next (tmplist)) {
    add_colour (cs, tmplist->data);
  }

  gtk_list_store_append (cs->colour_store, &cs->colour_custom_end);
  gtk_list_store_set (cs->colour_store,
                      &cs->colour_custom_end,
                      COL_COLOUR, NULL,
                      COL_TEXT, NULL,
                      COL_SPECIAL, SPECIAL_SEPARATOR,
                      -1);


  gtk_list_store_append (cs->colour_store, &cs->colour_other);
  gtk_list_store_set (cs->colour_store,
                      &cs->colour_other,
                      COL_COLOUR, NULL,
                      COL_TEXT, _("More Colors…"),
                      COL_SPECIAL, SPECIAL_MORE,
                      -1);

  gtk_list_store_append (cs->colour_store, &cs->colour_reset);
  gtk_list_store_set (cs->colour_store,
                      &cs->colour_reset,
                      COL_COLOUR, NULL,
                      COL_TEXT, _("Reset Menu"),
                      COL_SPECIAL, SPECIAL_RESET,
                      -1);


  cs->combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (cs->colour_store));
  g_signal_connect (cs->combo,
                    "changed",
                    G_CALLBACK (changed),
                    cs);
  gtk_widget_set_hexpand (GTK_WIDGET (cs->combo), TRUE);
  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (cs->combo),
                                        is_separator, NULL, NULL);

  renderer = dia_colour_cell_renderer_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (cs->combo), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (cs->combo),
                                  renderer,
                                  "colour", COL_COLOUR,
                                  "text", COL_TEXT,
                                  NULL);

  gtk_box_pack_start (GTK_BOX (cs), cs->combo, FALSE, TRUE, 0);
  gtk_widget_show (cs->combo);
}


void
dia_colour_selector_set_use_alpha (DiaColourSelector *cs, gboolean use_alpha)
{
  cs->use_alpha = use_alpha;
}


GtkWidget *
dia_colour_selector_new (void)
{
  return g_object_new (DIA_TYPE_COLOUR_SELECTOR, NULL);
}


void
dia_colour_selector_get_colour (DiaColourSelector *cs, Color *color)
{
  GtkTreeIter iter;
  Color *colour;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (cs->combo), &iter)) {
    gtk_tree_model_get (GTK_TREE_MODEL (cs->colour_store),
                        &iter,
                        COL_COLOUR, &colour,
                        -1);
  } else {
    g_warning ("No colour selected");

    colour = color_new_rgb (0, 0, 0);
  }

  color->red = colour->red;
  color->blue = colour->blue;
  color->green = colour->green;
  color->alpha = colour->alpha;

  dia_colour_free (colour);
}


static gboolean
set_colour (GtkTreeModel *model,
            GtkTreePath  *path,
            GtkTreeIter  *iter,
            gpointer      data)
{
  DiaColourSelector *self = DIA_COLOUR_SELECTOR (data);
  Color *colour;
  gboolean res = FALSE;

  gtk_tree_model_get (model,
                      iter,
                      COL_COLOUR, &colour,
                      -1);

  if (!colour) {
    return FALSE;
  }

  res = color_equals (colour, self->looking_for);
  if (res) {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self->combo), iter);

    self->found = TRUE;
  }

  dia_colour_free (colour);

  return res;
}


void
dia_colour_selector_set_colour (DiaColourSelector *cs,
                                const Color       *color)
{
  cs->looking_for = color;
  cs->found = FALSE;
  gtk_tree_model_foreach (GTK_TREE_MODEL (cs->colour_store), set_colour, cs);
  if (!cs->found) {
    GtkTreeIter iter;
    char *text = dia_colour_to_string ((Color *) color);

    persistent_list_add (PERSIST_NAME, text);

    gtk_list_store_insert_before (cs->colour_store,
                                  &iter,
                                  &cs->colour_custom_end);
    gtk_list_store_set (cs->colour_store,
                        &iter,
                        COL_COLOUR, color,
                        COL_SPECIAL, SPECIAL_NOT,
                        COL_TEXT, text,
                        -1);

    g_clear_pointer (&text, g_free);

    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (cs->combo), &iter);
  }

  cs->looking_for = NULL;
  cs->found = FALSE;
}
