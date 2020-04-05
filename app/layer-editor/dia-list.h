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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#include "dia-layer-widget.h"

G_BEGIN_DECLS

#define DIA_TYPE_LIST dia_list_get_type ()

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkContainer, g_object_unref)

G_DECLARE_DERIVABLE_TYPE (DiaList, dia_list, DIA, LIST, GtkContainer)


struct _DiaListClass {
  GtkContainerClass parent_class;
};


GtkWidget      *dia_list_new            (void);
void            dia_list_insert_items   (DiaList        *self,
                                         GList          *items,
                                         int             position);
void            dia_list_append_items   (DiaList        *self,
                                         GList          *items);
void            dia_list_prepend_items  (DiaList        *self,
                                         GList          *items);
void            dia_list_remove_items   (DiaList        *self,
                                         GList          *items);
void            dia_list_clear_items    (DiaList        *self,
                                         int             start,
                                         int             end);
void            dia_list_select_item    (DiaList        *self,
                                         int             item);
void            dia_list_select_child   (DiaList        *self,
                                         DiaLayerWidget *item);
void            dia_list_unselect_child (DiaList        *self,
                                         DiaLayerWidget *item);
int             dia_list_child_position (DiaList        *self,
                                         DiaLayerWidget *item);
DiaLayerWidget *dia_list_get_selected   (DiaList        *self);

G_END_DECLS
