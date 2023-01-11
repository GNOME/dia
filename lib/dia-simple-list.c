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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "dia-simple-list.h"


typedef struct _DiaSimpleListPrivate DiaSimpleListPrivate;
struct _DiaSimpleListPrivate {
  GtkListStore     *store;
  GtkTreeSelection *selection;
};


G_DEFINE_TYPE_WITH_PRIVATE (DiaSimpleList, dia_simple_list, GTK_TYPE_TREE_VIEW)


enum {
  SELECTION_CHANGED,
  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0, };


static void
dia_simple_list_finalize (GObject *object)
{
  DiaSimpleList *self = DIA_SIMPLE_LIST (object);
  DiaSimpleListPrivate *priv = dia_simple_list_get_instance_private (self);

  g_clear_object (&priv->store);

  G_OBJECT_CLASS (dia_simple_list_parent_class)->finalize (object);
}


static void
dia_simple_list_class_init (DiaSimpleListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dia_simple_list_finalize;

  signals[SELECTION_CHANGED] = g_signal_new ("selection-changed",
                                             G_TYPE_FROM_CLASS (klass),
                                             G_SIGNAL_RUN_FIRST,
                                             0,
                                             NULL, NULL, NULL,
                                             G_TYPE_NONE,
                                             0);
}


static void
selection_changed (GtkTreeSelection *treeselection,
                   DiaSimpleList    *self)
{
  g_signal_emit (self, signals[SELECTION_CHANGED], 0);
}


static void
dia_simple_list_init (DiaSimpleList *self)
{
  DiaSimpleListPrivate *priv = dia_simple_list_get_instance_private (self);

  priv->store = gtk_list_store_new (1, G_TYPE_STRING);

  gtk_tree_view_set_model (GTK_TREE_VIEW (self), GTK_TREE_MODEL (priv->store));

  priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));

  gtk_tree_selection_unselect_all (priv->selection);
  gtk_tree_selection_set_mode (priv->selection,
                               GTK_SELECTION_BROWSE);

  g_signal_connect (priv->selection,
                    "changed",
                    G_CALLBACK (selection_changed),
                    self);
}


GtkWidget *
dia_simple_list_new (void)
{
  return g_object_new (DIA_TYPE_SIMPLE_LIST, NULL);
}


void
dia_simple_list_empty (DiaSimpleList *self)
{
  DiaSimpleListPrivate *priv;

  g_return_if_fail (DIA_IS_SIMPLE_LIST (self));

  priv = dia_simple_list_get_instance_private (self);

  gtk_list_store_clear (priv->store);
}


void
dia_simple_list_append (DiaSimpleList *self,
                        const char    *item)
{
  DiaSimpleListPrivate *priv;
  GtkTreeIter iter;

  g_return_if_fail (DIA_IS_SIMPLE_LIST (self));

  priv = dia_simple_list_get_instance_private (self);

  gtk_list_store_append (priv->store, &iter);
  gtk_list_store_set (priv->store, &iter, 0, item, -1);
}


void
dia_simple_list_select (DiaSimpleList *self,
                        int            n)
{
  DiaSimpleListPrivate *priv;
  GtkTreeIter iter;

  g_return_if_fail (DIA_IS_SIMPLE_LIST (self));

  priv = dia_simple_list_get_instance_private (self);

  if (n == -1) {
    gtk_tree_selection_unselect_all (priv->selection);
    return;
  }

  if (!gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (priv->store),
                                      &iter,
                                      NULL,
                                      n)) {
    g_warning ("Can't select %i", n);
    return;
  }

  gtk_tree_selection_select_iter (priv->selection, &iter);
}


int
dia_simple_list_get_selected (DiaSimpleList *self)
{
  DiaSimpleListPrivate *priv;
  GtkTreeIter iter;
  GtkTreePath *path;
  int res;

  g_return_val_if_fail (DIA_IS_SIMPLE_LIST (self), -1);

  priv = dia_simple_list_get_instance_private (self);

  if (!gtk_tree_selection_get_selected (priv->selection, NULL, &iter)) {
    return -1;
  }

  path = gtk_tree_model_get_path (GTK_TREE_MODEL (self), &iter);

  g_return_val_if_fail (gtk_tree_path_get_depth (path) == 1, -1);

  res = gtk_tree_path_get_indices (path)[0];

  gtk_tree_path_free (path);

  return res;
}

