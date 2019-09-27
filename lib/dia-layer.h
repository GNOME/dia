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

#include "diatypes.h"
#include "diarenderer.h"

#pragma once

G_BEGIN_DECLS

#define DIA_TYPE_LAYER dia_layer_get_type()
G_DECLARE_DERIVABLE_TYPE (DiaLayer, dia_layer, DIA, LAYER, GObject)

struct _DiaLayerClass
{
  GObjectClass parent_class;
};

DiaLayer    *dia_layer_new                                 (const char       *name,
                                                            DiagramData      *parent);
DiaLayer    *dia_layer_new_from_layer                      (DiaLayer         *old);
void         dia_layer_render                              (DiaLayer         *layer,
                                                            DiaRenderer      *renderer,
                                                            DiaRectangle     *update,
                                                            ObjectRenderer    obj_renderer /* Can be NULL */,
                                                            gpointer          data,
                                                            int               active_layer);
int          dia_layer_object_get_index                    (DiaLayer         *layer,
                                                            DiaObject        *obj);
DiaObject   *dia_layer_object_get_nth                      (DiaLayer         *layer,
                                                            guint             index);
int          dia_layer_object_count                        (DiaLayer         *layer);
const char  *dia_layer_get_name                            (DiaLayer         *layer);
void         dia_layer_add_object                          (DiaLayer         *layer,
                                                            DiaObject        *obj);
void         dia_layer_add_object_at                       (DiaLayer         *layer,
                                                            DiaObject        *obj,
                                                            int               pos);
void         dia_layer_add_objects                         (DiaLayer         *layer,
                                                            GList            *obj_list);
void         dia_layer_add_objects_first                   (DiaLayer         *layer,
                                                            GList            *obj_list);
void         dia_layer_remove_object                       (DiaLayer         *layer,
                                                            DiaObject        *obj);
void         dia_layer_remove_objects                      (DiaLayer         *layer,
                                                            GList            *obj_list);
GList       *dia_layer_find_objects_intersecting_rectangle (DiaLayer         *layer,
                                                            DiaRectangle     *rect);
GList       *dia_layer_find_objects_in_rectangle           (DiaLayer         *layer,
                                                            DiaRectangle     *rect);
GList       *dia_layer_find_objects_containing_rectangle   (DiaLayer         *layer,
                                                            DiaRectangle     *rect);
DiaObject   *dia_layer_find_closest_object                 (DiaLayer         *layer,
                                                            Point            *pos,
                                                            real              maxdist);
DiaObject   *dia_layer_find_closest_object_except          (DiaLayer         *layer,
                                                            Point            *pos,
                                                            real              maxdist,
                                                            GList            *avoid);
real         dia_layer_find_closest_connectionpoint        (DiaLayer         *layer,
                                                            ConnectionPoint **closest,
                                                            Point            *pos,
                                                            DiaObject        *notthis);
int          dia_layer_update_extents                      (DiaLayer         *layer); /* returns true if changed. */
void         dia_layer_replace_object_with_list            (DiaLayer         *layer,
                                                            DiaObject        *remove_obj,
                                                            GList            *insert_list);
void         dia_layer_set_object_list                     (DiaLayer         *layer,
                                                            GList            *list);
GList       *dia_layer_get_object_list                     (DiaLayer         *layer);
DiagramData *dia_layer_get_parent_diagram                  (DiaLayer         *layer);
void         dia_layer_set_parent_diagram                  (DiaLayer         *layer,
                                                            DiagramData      *diagram);
gboolean     dia_layer_is_connectable                      (DiaLayer         *self);
void         dia_layer_set_connectable                     (DiaLayer         *self,
                                                            gboolean          connectable);
gboolean     dia_layer_is_visible                          (DiaLayer         *self);
void         dia_layer_set_visible                         (DiaLayer         *self,
                                                            gboolean          visible);
void         dia_layer_get_extents                         (DiaLayer         *self,
                                                            DiaRectangle     *rect);

G_END_DECLS
