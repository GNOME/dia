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

#pragma once

#include <gtk/gtk.h>

#include "diagram.h"

G_BEGIN_DECLS


#define DIA_TYPE_LAYER_EDITOR dia_layer_editor_get_type ()
G_DECLARE_DERIVABLE_TYPE (DiaLayerEditor, dia_layer_editor, DIA, LAYER_EDITOR, GtkBox)

struct _DiaLayerEditorClass {
  /*< private >*/
  GtkBoxClass parent;
};

GtkWidget *dia_layer_editor_new         (void);
void       dia_layer_editor_set_diagram (DiaLayerEditor *self,
                                         Diagram        *dia);
Diagram   *dia_layer_editor_get_diagram (DiaLayerEditor *self);

G_END_DECLS
