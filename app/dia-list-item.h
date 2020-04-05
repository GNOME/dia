/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DIA_TYPE_LIST_ITEM dia_list_item_get_type ()

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkBin, g_object_unref)

G_DECLARE_DERIVABLE_TYPE (DiaListItem, dia_list_item, DIA, LIST_ITEM, GtkBin)


struct _DiaListItemClass {
  GtkBinClass parent_class;

  void (*select)          (DiaListItem   *list_item);
  void (*deselect)        (DiaListItem   *list_item);
  void (*scroll_vertical) (DiaListItem   *list_item,
                           GtkScrollType  scroll_type,
                           gfloat         position);
};

G_END_DECLS
