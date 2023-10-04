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

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "dia-font-selector.h"
#include "font.h"
#include "persistence.h"

#define PERSIST_NAME "font-menu"

struct _DiaFontSelector {
  GtkBox hbox;
};


typedef struct _DiaFontSelectorPrivate DiaFontSelectorPrivate;
struct _DiaFontSelectorPrivate {
  GtkWidget    *fonts;
  GtkTreeStore *fonts_store;
  GtkTreeIter   fonts_default_end;
  GtkTreeIter   fonts_custom_end;
  GtkTreeIter   fonts_other;
  GtkTreeIter   fonts_reset;

  const char   *looking_for;

  GtkWidget    *styles;
  GtkListStore *styles_store;

  char         *current;
  int           current_style;
};

G_DEFINE_TYPE_WITH_PRIVATE (DiaFontSelector, dia_font_selector, GTK_TYPE_BOX)

enum {
  VALUE_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


/* New and improved font selector:  Contains the three standard fonts
 * and an 'Other fonts...' entry that opens the font dialog.  The fonts
 * selected in the font dialog are persistently added to the menu.
 *
 * +----------------+
 * | Sans           |
 * | Serif          |
 * | Monospace      |
 * | -------------- |
 * | Bodini         |
 * | CurlyGothic    |
 * | OldWestern     |
 * | -------------- |
 * | Other fonts... |
 * +----------------+
 */

enum {
  STYLE_COL_LABEL,
  STYLE_COL_ID,
  STYLE_N_COL,
};


enum {
  FONT_COL_FAMILY,
  FONT_N_COL,
};


static void
dia_font_selector_finalize (GObject *object)
{
  DiaFontSelector *self = DIA_FONT_SELECTOR (object);
  DiaFontSelectorPrivate *priv = dia_font_selector_get_instance_private (self);

  g_clear_object (&priv->fonts_store);
  g_clear_object (&priv->styles_store);

  g_clear_pointer (&priv->current, g_free);

  G_OBJECT_CLASS (dia_font_selector_parent_class)->finalize (object);
}


static void
dia_font_selector_class_init (DiaFontSelectorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dia_font_selector_finalize;

  signals[VALUE_CHANGED] = g_signal_new ("value-changed",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_FIRST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);
}


static int
sort_fonts (const void *p1, const void *p2)
{
  const gchar *n1 = pango_font_family_get_name (PANGO_FONT_FAMILY (*(void**)p1));
  const gchar *n2 = pango_font_family_get_name (PANGO_FONT_FAMILY (*(void**)p2));
  return g_ascii_strcasecmp (n1, n2);
}


static char *style_labels[] = {
  "Normal",
  "Oblique",
  "Italic",
  "Ultralight",
  "Ultralight-Oblique",
  "Ultralight-Italic",
  "Light",
  "Light-Oblique",
  "Light-Italic",
  "Medium",
  "Medium-Oblique",
  "Medium-Italic",
  "Demibold",
  "Demibold-Oblique",
  "Demibold-Italic",
  "Bold",
  "Bold-Oblique",
  "Bold-Italic",
  "Ultrabold",
  "Ultrabold-Oblique",
  "Ultrabold-Italic",
  "Heavy",
  "Heavy-Oblique",
  "Heavy-Italic"
};


static PangoFontFamily *
get_family_from_name (GtkWidget *widget, const gchar *fontname)
{
  PangoFontFamily **families;
  int n_families, i;

  pango_context_list_families (dia_font_get_context(),
                               &families, &n_families);
  /* Doing it the slow way until I find a better way */
  for (i = 0; i < n_families; i++) {
    if (!(g_ascii_strcasecmp (pango_font_family_get_name (families[i]), fontname))) {
      PangoFontFamily *fam = families[i];
      g_clear_pointer (&families, g_free);
      return fam;
    }
  }
  g_warning (_("Couldn't find font family for %s\n"), fontname);
  g_clear_pointer (&families, g_free);
  return NULL;
}


static void
set_styles (DiaFontSelector *fs,
            const gchar     *name,
            DiaFontStyle     dia_style)
{
  PangoFontFamily *pff;
  DiaFontSelectorPrivate *priv;
  PangoFontFace **faces = NULL;
  int nfaces = 0;
  int i = 0;
  long stylebits = 0;

  g_return_if_fail (DIA_IS_FONT_SELECTOR (fs));

  priv = dia_font_selector_get_instance_private (fs);

  pff = get_family_from_name (GTK_WIDGET (fs), name);

  pango_font_family_list_faces (pff, &faces, &nfaces);

  for (i = 0; i < nfaces; i++) {
    PangoFontDescription *pfd = pango_font_face_describe (faces[i]);
    PangoStyle style = pango_font_description_get_style (pfd);
    PangoWeight weight = pango_font_description_get_weight (pfd);
    /*
     * This is a quick and dirty way to pick the styles present,
     * sort them and avoid duplicates.
     * We set a bit for each style present, bit (weight*3+style)
     * From style_labels, we pick #(weight*3+style)
     * where weight and style are the Dia types.
     */
    /* Account for DIA_WEIGHT_NORMAL hack */
    int weightnr = (weight-200)/100;
    if (weightnr < 2) weightnr ++;
    else if (weightnr == 2) weightnr = 0;
    stylebits |= 1 << (3*weightnr + style);
    pango_font_description_free (pfd);
  }

  g_clear_pointer (&faces, g_free);

  if (stylebits == 0) {
    g_warning ("'%s' has no style!",
               pango_font_family_get_name (pff) ? pango_font_family_get_name (pff) : "(null font)");
  }

  gtk_list_store_clear (priv->styles_store);

  for (i = DIA_FONT_NORMAL; i <= (DIA_FONT_HEAVY | DIA_FONT_ITALIC); i+=4) {
    GtkTreeIter iter;

    /*
     * bad hack continued ...
     */
    int weight = DIA_FONT_STYLE_GET_WEIGHT (i) >> 4;
    int slant = DIA_FONT_STYLE_GET_SLANT (i) >> 2;

    if (DIA_FONT_STYLE_GET_SLANT (i) > DIA_FONT_ITALIC) {
      continue;
    }

    if (!(stylebits & (1 << (3 * weight + slant)))) {
      continue;
    }

    gtk_list_store_append (priv->styles_store, &iter);
    gtk_list_store_set (priv->styles_store,
                        &iter,
                        STYLE_COL_LABEL, style_labels[3 * weight + slant],
                        STYLE_COL_ID, i,
                        -1);

    if (dia_style == i || (i == DIA_FONT_NORMAL && dia_style == -1)) {
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->styles), &iter);
    }
  }

  gtk_widget_set_sensitive (GTK_WIDGET (priv->styles),
                            gtk_tree_model_iter_n_children (GTK_TREE_MODEL (priv->styles_store), NULL) > 1);
}


static void
font_changed (GtkComboBox     *widget,
              DiaFontSelector *self)
{
  DiaFontSelectorPrivate *priv;
  GtkTreeIter active;
  GtkTreePath *active_path;
  GtkTreePath *path;
  char *family = NULL;

  g_return_if_fail (DIA_IS_FONT_SELECTOR (self));

  priv = dia_font_selector_get_instance_private (self);

  gtk_combo_box_get_active_iter (widget, &active);

  active_path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->fonts_store), &active);
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->fonts_store), &priv->fonts_reset);

  if (gtk_tree_path_compare (path, active_path) == 0) {
    GtkTreeIter iter;
    GtkTreePath *end_path;
    DiaFont *font;

    persistent_list_clear (PERSIST_NAME);

    path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->fonts_store), &priv->fonts_default_end);

    // Move over the separator
    gtk_tree_path_next (path);
    gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->fonts_store), &iter, path);

    end_path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->fonts_store), &priv->fonts_custom_end);

    while (gtk_tree_path_compare (path, end_path) != 0) {
      gtk_tree_store_remove (priv->fonts_store, &iter);

      gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->fonts_store), &iter, path);

      gtk_tree_path_free (end_path);
      end_path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->fonts_store), &priv->fonts_custom_end);
    }

    gtk_tree_path_free (path);
    gtk_tree_path_free (end_path);
    gtk_tree_path_free (active_path);

    if (priv->current) {
      font = dia_font_new (priv->current, priv->current_style, 1.0);
      dia_font_selector_set_font (self, font);
      g_clear_object (&font);
    } else {
      gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->fonts_store), &iter);
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->fonts), &iter);
    }

    return;
  }

  gtk_tree_model_get (GTK_TREE_MODEL (priv->fonts_store),
                      &active,
                      FONT_COL_FAMILY, &family,
                      -1);

  g_clear_pointer (&priv->current, g_free);
  priv->current = g_strdup (family);

  set_styles (self, family, -1);
  g_signal_emit (G_OBJECT (self), signals[VALUE_CHANGED], 0);

  if (g_strcmp0 (family, "sans") != 0 &&
      g_strcmp0 (family, "serif") != 0 &&
      g_strcmp0 (family, "monospace") != 0 &&
      !persistent_list_add (PERSIST_NAME, family)) {
    GtkTreeIter iter;

    gtk_tree_store_insert_before (priv->fonts_store,
                                  &iter,
                                  NULL,
                                  &priv->fonts_custom_end);
    gtk_tree_store_set (priv->fonts_store,
                        &iter,
                        FONT_COL_FAMILY, family,
                        -1);

    gtk_combo_box_set_active_iter (widget, &iter);
  }

  gtk_tree_path_free (path);
  gtk_tree_path_free (active_path);
  g_clear_pointer (&family, g_free);
}


static gboolean
is_separator (GtkTreeModel *model,
              GtkTreeIter  *iter,
              gpointer      data)
{
  gboolean result;
  char *family;

  gtk_tree_model_get (model, iter, FONT_COL_FAMILY, &family, -1);

  result = g_strcmp0 (family, "separator") == 0;

  g_clear_pointer (&family, g_free);

  return result;
}


static void
is_sensitive (GtkCellLayout   *cell_layout,
              GtkCellRenderer *cell,
              GtkTreeModel    *tree_model,
              GtkTreeIter     *iter,
              gpointer         data)
{
  gboolean sensitive;

  sensitive = !gtk_tree_model_iter_has_child (tree_model, iter);

  g_object_set (cell, "sensitive", sensitive, NULL);
}


static void
style_changed (GtkComboBox     *widget,
               DiaFontSelector *self)
{
  DiaFontSelectorPrivate *priv;
  GtkTreeIter active;

  g_return_if_fail (DIA_IS_FONT_SELECTOR (self));

  priv = dia_font_selector_get_instance_private (self);

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->styles), &active)) {
    gtk_tree_model_get (GTK_TREE_MODEL (priv->styles_store),
                        &active,
                        STYLE_COL_ID, &priv->current_style,
                        -1);
  } else {
    priv->current_style = 0;
  }

  g_signal_emit (G_OBJECT (self), signals[VALUE_CHANGED], 0);
}


static void
dia_font_selector_init (DiaFontSelector *fs)
{
  DiaFontSelectorPrivate *priv;
  PangoFontFamily **families;
  int n_families,i;
  GtkCellRenderer *renderer;
  GtkTreeIter iter;
  GList *tmplist;

  g_return_if_fail (DIA_IS_FONT_SELECTOR (fs));

  priv = dia_font_selector_get_instance_private (fs);

  priv->fonts_store = gtk_tree_store_new (FONT_N_COL, G_TYPE_STRING);

  gtk_tree_store_append (priv->fonts_store, &iter, NULL);
  gtk_tree_store_set (priv->fonts_store,
                      &iter,
                      FONT_COL_FAMILY, "sans",
                      -1);
  gtk_tree_store_append (priv->fonts_store, &iter, NULL);
  gtk_tree_store_set (priv->fonts_store,
                      &iter,
                      FONT_COL_FAMILY, "serif",
                      -1);
  gtk_tree_store_append (priv->fonts_store, &iter, NULL);
  gtk_tree_store_set (priv->fonts_store,
                      &iter,
                      FONT_COL_FAMILY, "monospace",
                      -1);

  gtk_tree_store_append (priv->fonts_store, &priv->fonts_default_end, NULL);
  gtk_tree_store_set (priv->fonts_store,
                      &priv->fonts_default_end,
                      FONT_COL_FAMILY, "separator",
                      -1);

  persistence_register_list (PERSIST_NAME);

  for (tmplist = persistent_list_get_glist (PERSIST_NAME);
       tmplist != NULL; tmplist = g_list_next (tmplist)) {
    gtk_tree_store_append (priv->fonts_store, &iter, NULL);
    gtk_tree_store_set (priv->fonts_store,
                        &iter,
                        FONT_COL_FAMILY, tmplist->data,
                        -1);
  }

  gtk_tree_store_append (priv->fonts_store, &priv->fonts_custom_end, NULL);
  gtk_tree_store_set (priv->fonts_store,
                      &priv->fonts_custom_end,
                      FONT_COL_FAMILY, "separator",
                      -1);

  gtk_tree_store_append (priv->fonts_store, &priv->fonts_other, NULL);
  gtk_tree_store_set (priv->fonts_store,
                      &priv->fonts_other,
                      FONT_COL_FAMILY, _("Other Fonts"),
                      -1);

  gtk_tree_store_append (priv->fonts_store, &priv->fonts_reset, NULL);
  gtk_tree_store_set (priv->fonts_store,
                      &priv->fonts_reset,
                      FONT_COL_FAMILY, _("Reset Menu"),
                      -1);

  priv->fonts = gtk_combo_box_new_with_model (GTK_TREE_MODEL (priv->fonts_store));
  gtk_widget_set_hexpand (priv->fonts, TRUE);
  gtk_widget_show (priv->fonts);

  g_signal_connect (priv->fonts,
                    "changed",
                    G_CALLBACK (font_changed),
                    fs);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->fonts), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->fonts), renderer,
                                  "text", FONT_COL_FAMILY,
                                  "family", FONT_COL_FAMILY,
                                  NULL);

  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (priv->fonts),
                                        is_separator, NULL, NULL);
  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (priv->fonts),
                                      renderer,
                                      is_sensitive,
                                      NULL, NULL);

  priv->styles_store = gtk_list_store_new (STYLE_N_COL,
                                           G_TYPE_STRING,
                                           G_TYPE_INT);
  priv->styles = gtk_combo_box_new_with_model (GTK_TREE_MODEL (priv->styles_store));
  gtk_widget_show (priv->styles);

  g_signal_connect (priv->styles,
                    "changed",
                    G_CALLBACK (style_changed),
                    fs);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->styles), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->styles), renderer,
                                  "text", STYLE_COL_LABEL,
                                  NULL);

  pango_context_list_families (dia_font_get_context (),
                               &families,
                               &n_families);

  qsort (families,
         n_families,
         sizeof (PangoFontFamily *),
         sort_fonts);

  /* Doing it the slow way until I find a better way */
  for (i = 0; i < n_families; i++) {
    gtk_tree_store_append (priv->fonts_store,
                           &iter,
                           &priv->fonts_other);
    gtk_tree_store_set (priv->fonts_store,
                        &iter,
                        FONT_COL_FAMILY, pango_font_family_get_name (families[i]),
                        -1);
  }
  g_clear_pointer (&families, g_free);

  gtk_box_pack_start (GTK_BOX (fs), GTK_WIDGET (priv->fonts), FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (fs), GTK_WIDGET (priv->styles), FALSE, TRUE, 0);
}


GtkWidget *
dia_font_selector_new (void)
{
  return g_object_new (DIA_TYPE_FONT_SELECTOR, NULL);
}


static gboolean
set_font (GtkTreeModel *model,
          GtkTreePath  *path,
          GtkTreeIter  *iter,
          gpointer      data)
{
  DiaFontSelector *self = DIA_FONT_SELECTOR (data);
  DiaFontSelectorPrivate *priv = dia_font_selector_get_instance_private (self);
  char *font;
  gboolean res = FALSE;

  gtk_tree_model_get (model,
                      iter,
                      FONT_COL_FAMILY, &font,
                      -1);

  res = g_strcmp0 (priv->looking_for, font) == 0;
  if (res) {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->fonts), iter);
  }

  g_clear_pointer (&font, g_free);

  return res;
}


/**
 * dia_font_selector_set_font:
 *
 * Set the current font to be shown in the font selector.
 */
void
dia_font_selector_set_font (DiaFontSelector *self, DiaFont *font)
{
  DiaFontSelectorPrivate *priv;
  const gchar *fontname = dia_font_get_family (font);

  g_return_if_fail (DIA_IS_FONT_SELECTOR (self));

  priv = dia_font_selector_get_instance_private (self);

  priv->looking_for = fontname;
  gtk_tree_model_foreach (GTK_TREE_MODEL (priv->fonts_store), set_font, self);
  priv->looking_for = NULL;

  set_styles (self, fontname, dia_font_get_style (font));
}


DiaFont *
dia_font_selector_get_font (DiaFontSelector *self)
{
  DiaFontSelectorPrivate *priv;
  DiaFontStyle style;
  DiaFont *font;
  GtkTreeIter iter;
  char *fontname = NULL;

  g_return_val_if_fail (DIA_IS_FONT_SELECTOR (self), NULL);

  priv = dia_font_selector_get_instance_private (self);

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->fonts), &iter)) {
    gtk_tree_model_get (GTK_TREE_MODEL (priv->fonts_store),
                        &iter,
                        FONT_COL_FAMILY, &fontname,
                        -1);
  } else {
    g_warning ("No font selected");
  }

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->styles), &iter)) {
    gtk_tree_model_get (GTK_TREE_MODEL (priv->styles_store),
                        &iter,
                        STYLE_COL_ID, &style,
                        -1);
  } else {
    style = 0;
  }

  font = dia_font_new (fontname, style, 1.0);

  g_clear_pointer (&fontname, g_free);

  return font;
}

