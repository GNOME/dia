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
#ifndef LAYER_DIALOG_H
#define LAYER_DIALOG_H

#include <gtk/gtk.h>
#include "diagram.h"


void layer_dialog_create(void);
void layer_dialog_update_diagram_list(void);
void layer_dialog_show(void);
void layer_dialog_set_diagram(Diagram *dia);

/* Integrated UI component */
GtkWidget * create_layer_view_widget (void);

typedef struct _DiaLayerWidget      DiaLayerWidget;

void diagram_edit_layer(Diagram *dia, Layer *layer);

#endif /* LAYER_DIALOG_H */


