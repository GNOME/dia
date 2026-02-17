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

#include "dia-colour-cell-renderer.h"
#include "dia-colour-selector-private.h"
#include "dia-lib-private-enums.h"
#include "diamarshal.h"
#include "persistence.h"

#include "dia-colour-selector.h"

#define PERSIST_NAME "color-menu"


struct _DiaColourSelector {
  GtkBox          hbox;

  DiaColour      *current;
  gboolean        use_alpha;

  GtkWidget      *combo;
  GtkListStore   *colour_store;
  GtkTreeIter     colour_default_end;
  GtkTreeIter     colour_custom_end;
  GtkTreeIter     colour_other;
  GtkTreeIter     colour_reset;

  GtkWidget      *dialog;
};


G_DEFINE_TYPE (DiaColourSelector, dia_colour_selector, GTK_TYPE_BOX)


enum {
  PROP_0,
  PROP_CURRENT,
  PROP_USE_ALPHA,
  N_PROPS,
};
static GParamSpec *pspecs[N_PROPS];


enum {
  VALUE_CHANGED,
  N_SIGNALS,
};
static guint signals[N_SIGNALS] = { 0 };


enum {
  COL_COLOUR,
  COL_TEXT,
  COL_SPECIAL,
  N_COL
};


static void
dia_colour_selector_dispose (GObject *object)
{
  DiaColourSelector *self = DIA_COLOUR_SELECTOR (object);

  if (self->dialog) {
    gtk_widget_destroy (self->dialog);
    g_clear_weak_pointer (&self->dialog);
  }

  g_clear_pointer (&self->current, dia_colour_free);

  G_OBJECT_CLASS (dia_colour_selector_parent_class)->dispose (object);
}


static void
dia_colour_selector_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  DiaColourSelector *self = DIA_COLOUR_SELECTOR (object);
  DiaColour colour;

  switch (property_id) {
    case PROP_CURRENT:
      dia_colour_selector_get_colour (self, &colour);
      g_value_set_boxed (value, &colour);
      break;
    case PROP_USE_ALPHA:
      g_value_set_boolean (value, self->use_alpha);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
dia_colour_selector_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  DiaColourSelector *self = DIA_COLOUR_SELECTOR (object);

  switch (property_id) {
    case PROP_CURRENT:
      dia_colour_selector_set_colour (self, g_value_get_boxed (value));
      break;
    case PROP_USE_ALPHA:
      dia_colour_selector_set_use_alpha (self, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
add_colour (DiaColourSelector *self, const char *hex)
{
  GtkTreeIter iter;
  DiaColour colour;

  dia_colour_parse (&colour, hex);
  gtk_list_store_append (self->colour_store, &iter);
  gtk_list_store_set (self->colour_store,
                      &iter,
                      COL_COLOUR, &colour,
                      COL_TEXT, hex,
                      COL_SPECIAL, DIA_SELECTOR_ITEM_COLOUR,
                      -1);
}


static void
colour_response (GtkDialog *dialog, int response, gpointer user_data)
{
  DiaColourSelector *self = DIA_COLOUR_SELECTOR (user_data);

  if (response == GTK_RESPONSE_OK) {
    GdkRGBA rgba;
    DiaColour colour;

    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (self->dialog),
                                &rgba);
    dia_colour_from_gdk (&colour, &rgba);

    dia_colour_selector_set_colour (self, &colour);
  } else if (self->current) {
    dia_colour_selector_set_colour (self, self->current);
  } else {
    DiaColour *colour = dia_colour_new_rgb (0, 0, 0);

    g_critical ("Huh, how'd this happen?");

    dia_colour_selector_set_colour (self, colour);

    g_clear_pointer (&colour, dia_colour_free);
  }

  gtk_widget_destroy (self->dialog);
  g_clear_weak_pointer (&self->dialog);
}


static inline void
more_colours (DiaColourSelector *self)
{
  GString *palette = g_string_new ("");
  GtkWidget *dialog;
  GtkWidget *parent;
  GList *tmplist;
  GdkRGBA rgba;

  parent = gtk_widget_get_toplevel (GTK_WIDGET (self));

  dialog = gtk_color_chooser_dialog_new (_("Select color"),
                                         GTK_WINDOW (parent));
  g_set_weak_pointer (&self->dialog, dialog);

  g_object_bind_property (self, "use-alpha",
                          dialog, "use-alpha",
                          G_BINDING_SYNC_CREATE);

  dia_colour_as_gdk (self->current, &rgba);
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (self->dialog), &rgba);

  /* avoid crashing if the property dialog is closed before the color dialog */
  if (parent) {
    gtk_window_set_transient_for (GTK_WINDOW (self->dialog),
                                  GTK_WINDOW (parent));
    gtk_window_set_destroy_with_parent (GTK_WINDOW (self->dialog), TRUE);
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
    DiaColour colour;

    dia_colour_parse (&colour, tmplist->data);
    dia_colour_as_gdk (&colour, &rgba);

    spec = gdk_rgba_to_string (&rgba);

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
  g_string_free (palette, TRUE);

  g_signal_connect_object (G_OBJECT (self->dialog),
                           "response",
                           G_CALLBACK (colour_response),
                           self,
                           G_CONNECT_DEFAULT);

  gtk_widget_show (GTK_WIDGET (self->dialog));
}


static void
changed (GtkComboBox *widget, gpointer user_data)
{
  DiaColourSelector *self = DIA_COLOUR_SELECTOR (user_data);
  GtkTreeIter active;
  GtkTreeIter iter;
  GtkTreePath *path;
  GtkTreePath *end_path;
  DiaColourSelectorItem special;

  if (!gtk_combo_box_get_active_iter (widget, &active)) {
    return;
  }

  gtk_tree_model_get (GTK_TREE_MODEL (self->colour_store),
                      &active,
                      COL_SPECIAL, &special,
                      -1);

  switch (special) {
    case DIA_SELECTOR_ITEM_COLOUR:
      g_clear_pointer (&self->current, dia_colour_free);
      gtk_tree_model_get (GTK_TREE_MODEL (self->colour_store),
                          &active,
                          COL_COLOUR, &self->current,
                          -1);

      /* Normal colour */
      g_signal_emit (self, signals[VALUE_CHANGED], 0);
      g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_CURRENT]);
      break;
    case DIA_SELECTOR_ITEM_MORE:
      more_colours (self);
      break;
    case DIA_SELECTOR_ITEM_RESET:
      persistent_list_clear (PERSIST_NAME);

      path = gtk_tree_model_get_path (GTK_TREE_MODEL (self->colour_store),
                                      &self->colour_default_end);

      // Move over the separator
      gtk_tree_path_next (path);
      gtk_tree_model_get_iter (GTK_TREE_MODEL (self->colour_store), &iter, path);

      end_path = gtk_tree_model_get_path (GTK_TREE_MODEL (self->colour_store),
                                          &self->colour_custom_end);

      while (gtk_tree_path_compare (path, end_path) != 0) {
        gtk_list_store_remove (self->colour_store, &iter);

        gtk_tree_model_get_iter (GTK_TREE_MODEL (self->colour_store), &iter, path);

        gtk_tree_path_free (end_path);
        end_path = gtk_tree_model_get_path (GTK_TREE_MODEL (self->colour_store),
                                            &self->colour_custom_end);
      }

      gtk_tree_path_free (path);
      gtk_tree_path_free (end_path);

      if (self->current) {
        dia_colour_selector_set_colour (self, self->current);
      } else {
        gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->colour_store), &iter);
        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self), &iter);
      }
      return;
    case DIA_SELECTOR_ITEM_SEPARATOR:
    default:
      g_return_if_reached ();
  }
}


static void
dia_colour_selector_class_init (DiaColourSelectorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = dia_colour_selector_dispose;
  object_class->get_property = dia_colour_selector_get_property;
  object_class->set_property = dia_colour_selector_set_property;

  pspecs[PROP_CURRENT] =
    g_param_spec_boxed ("current", NULL, NULL,
                        DIA_TYPE_COLOUR,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  pspecs[PROP_USE_ALPHA] =
    g_param_spec_boolean ("use-alpha", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, pspecs);


  signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  dia_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  g_signal_set_va_marshaller (signals[VALUE_CHANGED],
                              G_TYPE_FROM_CLASS (klass),
                              dia_marshal_VOID__VOIDv);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               DIA_APPLICATION_PATH "dia-colour-selector.ui");

  gtk_widget_class_bind_template_child (widget_class, DiaColourSelector, combo);
  gtk_widget_class_bind_template_child (widget_class, DiaColourSelector, colour_store);

  gtk_widget_class_bind_template_callback (widget_class, changed);

  g_type_ensure (DIA_TYPE_COLOUR_CELL_RENDERER);
  g_type_ensure (DIA_TYPE_COLOUR_SELECTOR_ITEM);
  g_type_ensure (DIA_TYPE_COLOUR);
}


static gboolean
is_separator (GtkTreeModel *model,
              GtkTreeIter  *iter,
              gpointer      data)
{
  DiaColourSelectorItem result;

  gtk_tree_model_get (model, iter, COL_SPECIAL, &result, -1);

  return result == DIA_SELECTOR_ITEM_SEPARATOR;
}


static void
dia_colour_selector_init (DiaColourSelector *self)
{
  GList *tmplist;

  gtk_widget_init_template (GTK_WIDGET (self));

  add_colour (self, "#000000");
  add_colour (self, "#FFFFFF");
  add_colour (self, "#FF0000");
  add_colour (self, "#00FF00");
  add_colour (self, "#0000FF");

  gtk_list_store_append (self->colour_store, &self->colour_default_end);
  gtk_list_store_set (self->colour_store,
                      &self->colour_default_end,
                      COL_COLOUR, NULL,
                      COL_TEXT, NULL,
                      COL_SPECIAL, DIA_SELECTOR_ITEM_SEPARATOR,
                      -1);

  persistence_register_list (PERSIST_NAME);

  for (tmplist = persistent_list_get_glist (PERSIST_NAME);
       tmplist != NULL; tmplist = g_list_next (tmplist)) {
    add_colour (self, tmplist->data);
  }

  gtk_list_store_append (self->colour_store, &self->colour_custom_end);
  gtk_list_store_set (self->colour_store,
                      &self->colour_custom_end,
                      COL_COLOUR, NULL,
                      COL_TEXT, NULL,
                      COL_SPECIAL, DIA_SELECTOR_ITEM_SEPARATOR,
                      -1);

  gtk_list_store_append (self->colour_store, &self->colour_other);
  gtk_list_store_set (self->colour_store,
                      &self->colour_other,
                      COL_COLOUR, NULL,
                      COL_TEXT, _("More Colors…"),
                      COL_SPECIAL, DIA_SELECTOR_ITEM_MORE,
                      -1);

  gtk_list_store_append (self->colour_store, &self->colour_reset);
  gtk_list_store_set (self->colour_store,
                      &self->colour_reset,
                      COL_COLOUR, NULL,
                      COL_TEXT, _("Reset Menu"),
                      COL_SPECIAL, DIA_SELECTOR_ITEM_RESET,
                      -1);

  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (self->combo),
                                        is_separator, NULL, NULL);
}


void
dia_colour_selector_get_colour (DiaColourSelector  *self,
                                DiaColour          *colour)
{
  g_return_if_fail (DIA_IS_COLOUR_SELECTOR (self));
  g_return_if_fail (colour != NULL);

  if (G_UNLIKELY (!self->current)) {
    colour->red = colour->green = colour->blue = 0.0;
    colour->alpha = 1.0;
    return;
  }

  colour->red = self->current->red;
  colour->blue = self->current->blue;
  colour->green = self->current->green;
  colour->alpha = self->current->alpha;
}


typedef struct _FindColour FindColour;
struct _FindColour {
  DiaColour *looking_for;
  gboolean found;
  GtkTreeIter iter;
};


static gboolean
set_colour (GtkTreeModel *model,
            GtkTreePath  *path,
            GtkTreeIter  *iter,
            gpointer      user_data)
{
  FindColour *data = user_data;
  DiaColour *colour = NULL;
  gboolean res = FALSE;

  gtk_tree_model_get (model,
                      iter,
                      COL_COLOUR, &colour,
                      -1);
  if (!colour) {
    goto out;
  }

  res = color_equals (colour, data->looking_for);
  if (res) {
    data->iter = *iter;
    data->found = TRUE;
  }

out:
  g_clear_pointer (&colour, dia_colour_free);

  return res;
}


void
dia_colour_selector_set_colour (DiaColourSelector  *self,
                                DiaColour          *colour)
{
  FindColour data = { 0, };

  g_return_if_fail (DIA_IS_COLOUR_SELECTOR (self));
  g_return_if_fail (colour != NULL);

  if (self->current && color_equals (self->current, colour)) {
    return;
  }

  data.looking_for = colour;
  data.found = FALSE;

  gtk_tree_model_foreach (GTK_TREE_MODEL (self->colour_store),
                          set_colour,
                          &data);

  if (G_LIKELY (data.found)) {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self->combo),
                                   &data.iter);
  } else {
    GtkTreeIter iter;
    char *text = dia_colour_to_string (colour);

    persistent_list_add (PERSIST_NAME, text);

    gtk_list_store_insert_before (self->colour_store,
                                  &iter,
                                  &self->colour_custom_end);
    gtk_list_store_set (self->colour_store,
                        &iter,
                        COL_COLOUR, colour,
                        COL_SPECIAL, DIA_SELECTOR_ITEM_COLOUR,
                        COL_TEXT, text,
                        -1);

    g_clear_pointer (&text, g_free);

    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self->combo), &iter);
  }
}


void
dia_colour_selector_set_use_alpha (DiaColourSelector *self,
                                   gboolean           use_alpha)
{
  g_return_if_fail (DIA_IS_COLOUR_SELECTOR (self));

  if (self->use_alpha == use_alpha) {
    return;
  }

  self->use_alpha = use_alpha;
  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_USE_ALPHA]);
}


GtkWidget *
dia_colour_selector_new (void)
{
  return g_object_new (DIA_TYPE_COLOUR_SELECTOR, NULL);
}
