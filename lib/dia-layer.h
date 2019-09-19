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

#include <glib.h>

#pragma once

G_BEGIN_DECLS

/*!
 * \brief A diagram consists of layers holding objects
 *
 * \ingroup DiagramStructure
 * \todo : make this a GObject as well
 */
struct _Layer {
  char *name;             /*!< The name of the layer */
  Rectangle extents;      /*!< The extents of the layer */

  GList *objects;         /*!< List of objects in the layer,
			     sorted by decreasing z-value,
			     objects can ONLY be connected to objects
			     in the same layer! */

  gboolean visible;       /*!< The visibility of the layer */
  gboolean connectable;   /*!< Whether the layer can currently be connected to.
			     The selected layer is by default connectable */

  DiagramData *parent_diagram; /*!< Back-pointer to the diagram.  This
				  must only be set by functions internal
				  to the diagram, and accessed via
				  layer_get_parent_diagram() */
};

Layer       *dia_layer_new                                 (char             *name,
                                                            DiagramData      *parent);
void         dia_layer_destroy                             (Layer            *layer);
void         dia_layer_render                              (Layer            *layer,
                                                            DiaRenderer      *renderer,
                                                            Rectangle        *update,
                                                            ObjectRenderer    obj_renderer /* Can be NULL */,
                                                            gpointer          data,
                                                            int               active_layer);
int          dia_layer_object_get_index                    (Layer            *layer,
                                                            DiaObject        *obj);
DiaObject   *dia_layer_object_get_nth                      (Layer            *layer,
                                                            guint             index);
int          dia_layer_object_count                        (Layer            *layer);
gchar       *dia_layer_get_name                            (Layer            *layer);
void         dia_layer_add_object                          (Layer            *layer,
                                                            DiaObject        *obj);
void         dia_layer_add_object_at                       (Layer            *layer,
                                                            DiaObject        *obj,
                                                            int               pos);
void         dia_layer_add_objects                         (Layer            *layer,
                                                            GList            *obj_list);
void         dia_layer_add_objects_first                   (Layer            *layer,
                                                            GList            *obj_list);
void         dia_layer_remove_object                       (Layer            *layer,
                                                            DiaObject        *obj);
void         dia_layer_remove_objects                      (Layer            *layer,
                                                            GList            *obj_list);
GList       *dia_layer_find_objects_intersecting_rectangle (Layer            *layer,
                                                            Rectangle        *rect);
GList       *dia_layer_find_objects_in_rectangle           (Layer            *layer,
                                                            Rectangle        *rect);
GList       *dia_layer_find_objects_containing_rectangle   (Layer            *layer,
                                                            Rectangle        *rect);
DiaObject   *dia_layer_find_closest_object                 (Layer            *layer,
                                                            Point            *pos,
                                                            real              maxdist);
DiaObject   *dia_layer_find_closest_object_except          (Layer            *layer,
                                                            Point            *pos,
                                                            real              maxdist,
                                                            GList            *avoid);
real         dia_layer_find_closest_connectionpoint        (Layer            *layer,
                                                            ConnectionPoint **closest,
                                                            Point            *pos,
                                                            DiaObject        *notthis);
int          dia_layer_update_extents                      (Layer            *layer); /* returns true if changed. */
void         dia_layer_replace_object_with_list            (Layer            *layer,
                                                            DiaObject        *obj,
                                                            GList            *list);
void         dia_layer_set_object_list                     (Layer            *layer,
                                                            GList            *list);
DiagramData *dia_layer_get_parent_diagram                  (Layer            *layer);

G_END_DECLS
