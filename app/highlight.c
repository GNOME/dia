/* Dia -- an diagram creation/manipulation program -*- c -*-
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

#include "highlight.h"

#include <glib.h>

#include "diagram.h"
#include "diagramdata.h"
#include "object.h"
#include "object_ops.h"
#include "group.h"
#include "dia-layer.h"

/* One could argue that the actual setting of the object's field
 * should happen inside lib rather than app.  I think that'd be overkill.
 * -Lars
 */

/* The highlighting must happen within a certain small area around the bbox.
 * Highlighting sets the bbox to ... some more.
 * Problem:  The bbox is the same for all views, but highlighting size should
 * depend on the zoom level.  The renderer obviously can figure out to make
 * it a few more pixels or something, but we need the bbox to also be
 * enlarged by a bit.  I guess the object_add_updates call must handle that,
 * as it knows about the conversion.
 */

void
highlight_object(DiaObject *obj, DiaHighlightType type, Diagram *dia)
{
  data_highlight_add(dia->data, obj, type);

  object_add_updates(obj, dia);
}

void
highlight_object_off(DiaObject *obj, Diagram *dia)
{
  object_add_updates(obj, dia);
  data_highlight_remove(dia->data, obj);
}


/**
 * highlight_reset_objects:
 * @objects: objects to reset
 * @dia: the #Diagram
 *
 * Resets all highlighting in this layer. Helper function for
 * highlight_reset_all
 *
 * Since: dawn-of-time
 */
static void
highlight_reset_objects (GList *objects, Diagram *dia)
{
  for (; objects != NULL; objects = g_list_next (objects)) {
    DiaObject *object = DIA_OBJECT (objects->data);
    highlight_object_off (object, dia);
    if (IS_GROUP (object)) {
      highlight_reset_objects (group_objects (object), dia);
    }
  }
}


/* Currently does a brute-force run-through of all objects.
 * If this is too slow, we could create a list of currently highlighted
 * objects in the diagram and traverse that before killing it.
 * Note that we're not assuming that all highlighted objects are in
 * the active layer.
 */
void
highlight_reset_all (Diagram *dia)
{
  DIA_FOR_LAYER_IN_DIAGRAM (DIA_DIAGRAM_DATA (dia), layer, i, {
    highlight_reset_objects (dia_layer_get_object_list (layer), dia);
  });
}
